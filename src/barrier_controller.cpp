#include "barrier_controller.h"

#include "pins.h"

BarrierController::BarrierController(Logger& logger) : logger_(logger) {}

const char* barrierStateToString(BarrierState state) {
  switch (state) {
    case BarrierState::BOOT:
      return "BOOT";
    case BarrierState::IDLE_UNKNOWN:
      return "IDLE_UNKNOWN";
    case BarrierState::IDLE_CLOSED:
      return "IDLE_CLOSED";
    case BarrierState::OPENING:
      return "OPENING";
    case BarrierState::OPENED_HOLD:
      return "OPENED_HOLD";
    case BarrierState::CLOSING:
      return "CLOSING";
    case BarrierState::ERROR:
      return "ERROR";
  }
  return "UNKNOWN";
}

void BarrierController::begin(const LogicConfig& logic) {
  logic_ = logic;
  pinMode(Pins::RELAY_OPEN, OUTPUT);
  pinMode(Pins::OPEN_LIMIT, INPUT_PULLUP);
  pinMode(Pins::CLOSE_LIMIT, INPUT_PULLUP);

  setRelay(false);
  setState(BarrierState::BOOT, "startup");
}

void BarrierController::applyConfig(const LogicConfig& logic) {
  logic_ = logic;
}

bool BarrierController::openLimitRaw() const {
  return digitalRead(Pins::OPEN_LIMIT) == LOW;
}

bool BarrierController::closeLimitRaw() const {
  return digitalRead(Pins::CLOSE_LIMIT) == LOW;
}

bool BarrierController::openLimit() const {
  return logic_.open_limit_invert ? !openLimitRaw() : openLimitRaw();
}

bool BarrierController::closeLimit() const {
  return logic_.close_limit_invert ? !closeLimitRaw() : closeLimitRaw();
}

String BarrierController::position() const {
  if (openLimit()) {
    return "OPEN";
  }
  if (closeLimit()) {
    return "CLOSED";
  }
  return "TRANSIT/UNKNOWN";
}

void BarrierController::setRelay(bool enabled) {
  relay_state_ = enabled;
  const bool pin_level = logic_.relay_active_high ? enabled : !enabled;
  digitalWrite(Pins::RELAY_OPEN, pin_level ? HIGH : LOW);
}

void BarrierController::setState(BarrierState state, const String& reason) {
  state_ = state;
  state_started_ms_ = millis();
  if (reason.length()) {
    logger_.log(String("state=") + barrierStateToString(state) + " reason=" + reason);
  } else {
    logger_.log(String("state=") + barrierStateToString(state));
  }
}

void BarrierController::fail(const String& message) {
  last_error_ = message;
  setRelay(false);
  setState(BarrierState::ERROR, message);
}

bool BarrierController::commandAllowed() const {
  return state_ != BarrierState::ERROR || logic_.allow_commands_in_error;
}

bool BarrierController::requestOpen() {
  if (!commandAllowed()) {
    return false;
  }
  retry_counter_ = 0;
  if (logic_.open_mode == OpenMode::PULSE) {
    setRelay(true);
    relay_until_ms_ = millis() + logic_.open_pulse_ms;
  } else {
    setRelay(true);
    relay_until_ms_ = millis() + logic_.open_timeout_ms;
  }
  setState(BarrierState::OPENING, "open command");
  return true;
}

bool BarrierController::requestClose() {
  if (!commandAllowed()) {
    return false;
  }
  setRelay(false);
  setState(BarrierState::CLOSING, "close command");
  return true;
}

bool BarrierController::resetError() {
  last_error_ = "";
  retry_counter_ = 0;
  setRelay(false);
  setState(closeLimit() ? BarrierState::IDLE_CLOSED : BarrierState::IDLE_UNKNOWN, "manual reset");
  return true;
}

void BarrierController::testRelay(uint32_t duration_ms) {
  setRelay(true);
  relay_until_ms_ = millis() + duration_ms;
  logger_.log(String("relay test ") + duration_ms + "ms");
}

void BarrierController::update() {
  const uint32_t now = millis();

  if (relay_state_ && relay_until_ms_ != 0 && static_cast<int32_t>(now - relay_until_ms_) >= 0 && state_ != BarrierState::OPENING) {
    setRelay(false);
    relay_until_ms_ = 0;
  }

  switch (state_) {
    case BarrierState::BOOT:
      setState(closeLimit() ? BarrierState::IDLE_CLOSED : BarrierState::IDLE_UNKNOWN, "boot done");
      break;

    case BarrierState::IDLE_UNKNOWN:
    case BarrierState::IDLE_CLOSED:
      if (!relay_state_) {
        relay_until_ms_ = 0;
      }
      break;

    case BarrierState::OPENING:
      if (logic_.open_mode == OpenMode::PULSE && static_cast<int32_t>(now - relay_until_ms_) >= 0) {
        setRelay(false);
      }
      if (openLimit()) {
        setRelay(false);
        setState(BarrierState::OPENED_HOLD, "open limit reached");
      } else if (logic_.open_mode == OpenMode::HOLD_UNTIL_LIMIT && static_cast<int32_t>(now - relay_until_ms_) >= 0) {
        fail("open timeout");
      } else if (logic_.open_mode == OpenMode::PULSE && now - state_started_ms_ > logic_.open_timeout_ms) {
        fail("open timeout after pulse");
      }
      break;

    case BarrierState::OPENED_HOLD:
      if (now - state_started_ms_ >= logic_.open_hold_time_ms) {
        setState(BarrierState::CLOSING, "hold done");
      }
      break;

    case BarrierState::CLOSING:
      if (closeLimit()) {
        retry_counter_ = 0;
        setState(BarrierState::IDLE_CLOSED, "close limit reached");
      } else if (now - state_started_ms_ > logic_.close_timeout_ms) {
        if (retry_counter_ < logic_.close_retries) {
          retry_counter_++;
          logger_.log(String("close retry ") + retry_counter_);
          setRelay(true);
          relay_until_ms_ = now + logic_.open_pulse_ms;
          setState(BarrierState::OPENING, "retry open impulse");
        } else {
          fail("close timeout");
        }
      }
      break;

    case BarrierState::ERROR:
      if (logic_.auto_recover_from_error && closeLimit()) {
        resetError();
      }
      break;
  }

  if (relay_state_ && relay_until_ms_ != 0 && static_cast<int32_t>(now - relay_until_ms_) >= 0 && state_ == BarrierState::OPENING && logic_.open_mode == OpenMode::PULSE) {
    setRelay(false);
    relay_until_ms_ = 0;
  }
}
