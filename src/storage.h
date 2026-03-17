#pragma once

#include "config.h"

class Storage {
 public:
  bool begin();
  bool load(AppConfig& cfg);
  bool save(const AppConfig& cfg);

 private:
  static String ipToString(const IPAddress& ip);
  static IPAddress stringToIp(const String& value, const IPAddress& fallback);
};
