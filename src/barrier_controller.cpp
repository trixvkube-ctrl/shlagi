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

  open_filter_.stable = openLimitRaw();
  open_filter_.last_raw = open_filter_.stable;
  close_filter_.stable = closeLimitRaw();
  close_filter_.last_raw = close_filter_.stable;
  open_filter_.changed_at_ms = millis();
  close_filter_.changed_at_ms = millis();

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

bool BarrierController::readDebounced(uint8_t pin, bool invert, DebouncedInput& filter) const {
  bool raw = digitalRead(pin) == LOW;
  if (invert) raw = !raw;

  const uint32_t now = millis();
  if (raw != filter.last_raw) {
    filter.last_raw = raw;
    filter.changed_at_ms = now;
  }

  if (filter.stable != raw && now - filter.changed_at_ms >= logic_.debounce_ms) {
    filter.stable = raw;
  }
  return filter.stable;
}

bool BarrierController::openLimit() const {
  return readDebounced(Pins::OPEN_LIMIT, logic_.open_limit_invert, open_filter_);
}

bool BarrierController::closeLimit() const {
  return readDebounced(Pins::CLOSE_LIMIT, logic_.close_limit_invert, close_filter_);
}

String BarrierController::position() const {
  const bool o = openLimit();
  const bool c = closeLimit();
  if (o && !c) return "OPEN";
  if (c && !o) return "CLOSED";
  if (o && c) return "INVALID_BOTH_ACTIVE";
  return "TRANSIT/UNKNOWN";
}

void BarrierController::setRelay(bool enabled) {
  relay_state_ = enabled;
  const bool pin_level = logic_.relay_invert ? !enabled : enabled;
  digitalWrite(Pins::RELAY_OPEN, pin_level ? HIGH : LOW);
}

void BarrierController::setState(BarrierState state, const String& reason) {
  state_ = state;
  state_started_ms_ = millis();
  logger_.log("state", "barrier", String(barrierStateToString(state)) + (reason.length() ? (" | " + reason) : ""));
}

void BarrierController::fail(const String& message, const String& source) {
  last_error_ = message;
  setRelay(false);
  logger_.log("error", source, message);
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
  wait_until_ms_ = 0;
  setRelay(true);
  relay_until_ms_ = millis() + (logic_.open_mode == OpenMode::PULSE ? logic_.open_pulse_ms : logic_.open_timeout_ms);
  setState(BarrierState::OPENING, "open command");
  return true;
}

bool BarrierController::requestClose() {
  if (!commandAllowed()) {
    return false;
  }
  setRelay(false);
  wait_until_ms_ = 0;
  setState(BarrierState::CLOSING, "close command");
  return true;
}

bool BarrierController::resetError() {
  last_error_ = "";
  retry_counter_ = 0;
  wait_until_ms_ = 0;
  setRelay(false);
  setState(closeLimit() ? BarrierState::IDLE_CLOSED : BarrierState::IDLE_UNKNOWN, "manual reset");
  return true;
}

void BarrierController::testRelay(uint32_t duration_ms) {
  setRelay(true);
  relay_until_ms_ = millis() + duration_ms;
  logger_.log("control", "relay", String("test ") + duration_ms + "ms");
}

void BarrierController::update() {
  const uint32_t now = millis();
  const bool open = openLimit();
  const bool close = closeLimit();

  if (open && close) {
    if (dual_active_since_ms_ == 0) {
      dual_active_since_ms_ = now;
    }
    if (now - dual_active_since_ms_ > logic_.dual_limit_error_ms) {
      fail("open+close limits active", "limits");
      return;
    }
  } else {
    dual_active_since_ms_ = 0;
  }

  if (relay_state_ && relay_until_ms_ != 0 && (int32_t)(now - relay_until_ms_) >= 0 && state_ != BarrierState::OPENING) {
    setRelay(false);
    relay_until_ms_ = 0;
  }

  switch (state_) {
    case BarrierState::BOOT:
      setState(close ? BarrierState::IDLE_CLOSED : BarrierState::IDLE_UNKNOWN, "boot done");
      break;

    case BarrierState::IDLE_UNKNOWN:
    case BarrierState::IDLE_CLOSED:
      break;

    case BarrierState::OPENING:
      if (logic_.open_mode == OpenMode::PULSE && (int32_t)(now - relay_until_ms_) >= 0) {
        setRelay(false);
      }
      if (open) {
        setRelay(false);
        setState(BarrierState::OPENED_HOLD, "open limit reached");
      } else if (logic_.open_mode == OpenMode::HOLD_UNTIL_LIMIT && (int32_t)(now - relay_until_ms_) >= 0) {
        fail("open timeout", "opening");
      } else if (logic_.open_mode == OpenMode::PULSE && now - state_started_ms_ >= logic_.open_timeout_ms) {
        fail("open timeout after pulse", "opening");
      }
      break;

    case BarrierState::OPENED_HOLD:
      if (now - state_started_ms_ >= logic_.open_hold_time_ms) {
        setState(BarrierState::CLOSING, "open hold done");
      }
      break;

    case BarrierState::CLOSING:
      if (close) {
        retry_counter_ = 0;
        wait_until_ms_ = 0;
        setState(BarrierState::IDLE_CLOSED, "close limit reached");
      } else if (wait_until_ms_ != 0) {
        if ((int32_t)(now - wait_until_ms_) >= 0) {
          wait_until_ms_ = 0;
          setRelay(true);
          relay_until_ms_ = now + logic_.open_pulse_ms;
          setState(BarrierState::OPENING, "retry pulse");
        }
      } else if (now - state_started_ms_ >= logic_.close_timeout_ms) {
        if (retry_counter_ < logic_.close_retries) {
          retry_counter_++;
          wait_until_ms_ = now + logic_.extra_wait_before_retry_ms;
          logger_.log("retry", "closing", String("close timeout, retry=") + retry_counter_);
        } else {
          fail("close timeout", "closing");
        }
      }
      break;

    case BarrierState::ERROR:
      if (logic_.auto_recover_from_error && close) {
        resetError();
      }
      break;
  }

  if (relay_state_ && relay_until_ms_ != 0 && (int32_t)(now - relay_until_ms_) >= 0 && logic_.open_mode == OpenMode::PULSE && state_ == BarrierState::OPENING) {
    setRelay(false);
    relay_until_ms_ = 0;
  }
}
