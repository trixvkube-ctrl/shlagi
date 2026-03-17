#pragma once

#include <Arduino.h>

struct LogRecord {
  uint32_t timestamp_ms = 0;
  String event_type;
  String source;
  String message;
};

class Logger {
 public:
  static constexpr size_t kCapacity = 80;

  void begin();
  void log(const String& event_type, const String& source, const String& message);
  String recentEvents(size_t max_items = 20) const;

 private:
  LogRecord records_[kCapacity];
  size_t head_ = 0;
  size_t count_ = 0;
};
