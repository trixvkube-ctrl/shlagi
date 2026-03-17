#pragma once

#include <Arduino.h>

#include "config.h"
#include "logger.h"

enum class BarrierState : uint8_t {
  BOOT,
  IDLE_UNKNOWN,
  IDLE_CLOSED,
  OPENING,
  OPENED_HOLD,
  CLOSING,
  ERROR
};

class BarrierController {
 public:
  explicit BarrierController(Logger& logger);

  void begin(const LogicConfig& logic);
  void update();

  bool requestOpen();
  bool requestClose();
  bool resetError();
  void testRelay(uint32_t duration_ms);

  void applyConfig(const LogicConfig& logic);
  const LogicConfig& config() const { return logic_; }

  BarrierState state() const { return state_; }
  bool relayState() const { return relay_state_; }
  bool openLimitRaw() const;
  bool closeLimitRaw() const;
  bool openLimit() const;
  bool closeLimit() const;
  String position() const;
  String lastError() const { return last_error_; }
  uint8_t retryCounter() const { return retry_counter_; }

 private:
  void setRelay(bool enabled);
  void setState(BarrierState state, const String& reason = "");
  void fail(const String& message);
  bool commandAllowed() const;

  Logger& logger_;
  LogicConfig logic_;
  BarrierState state_ = BarrierState::BOOT;
  bool relay_state_ = false;
  String last_error_;
  uint32_t state_started_ms_ = 0;
  uint32_t relay_until_ms_ = 0;
  uint8_t retry_counter_ = 0;
};

const char* barrierStateToString(BarrierState state);
