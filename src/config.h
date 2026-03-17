#pragma once

#include <Arduino.h>
#include <IPAddress.h>

enum class OpenMode : uint8_t {
  PULSE = 0,
  HOLD_UNTIL_LIMIT = 1,
};

struct LogicConfig {
  OpenMode open_mode = OpenMode::PULSE;
  uint32_t open_pulse_ms = 700;
  uint32_t open_timeout_ms = 15000;
  uint32_t open_hold_time_ms = 3000;
  uint32_t close_timeout_ms = 20000;
  uint32_t extra_wait_before_retry_ms = 1200;
  uint8_t close_retries = 2;
  bool relay_invert = true;
  bool open_limit_invert = false;
  bool close_limit_invert = false;
  uint16_t debounce_ms = 40;
  uint16_t dual_limit_error_ms = 600;
  bool allow_commands_in_error = false;
  bool auto_recover_from_error = false;
};

struct NetworkConfig {
  bool dhcp = true;
  IPAddress ip{192, 168, 1, 90};
  IPAddress subnet{255, 255, 255, 0};
  IPAddress gateway{192, 168, 1, 1};
  IPAddress dns{8, 8, 8, 8};
  uint16_t http_port = 80;
  String trusted_ip;
  String shared_token = "changeme";
  String login = "admin";
  String password_hash;
};

struct AppConfig {
  LogicConfig logic;
  NetworkConfig net;
};

static constexpr char kConfigPath[] = "/config.json";
