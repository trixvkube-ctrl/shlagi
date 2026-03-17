#pragma once

#include <Arduino.h>

#include "barrier_controller.h"
#include "config.h"
#include "logger.h"

class WebUi {
 public:
  static String pageStatus(const BarrierController& barrier, const AppConfig& cfg, const String& ip, uint32_t uptime_ms, const Logger& logger);
  static String pageControl();
  static String pageLogicSettings(const AppConfig& cfg);
  static String pageNetworkSettings(const AppConfig& cfg);
};
