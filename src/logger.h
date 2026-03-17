#pragma once

#include <Arduino.h>

class Logger {
 public:
  static constexpr size_t kCapacity = 64;

  void begin();
  void log(const String& message);
  String recentEvents(size_t max_items = 20) const;

 private:
  String lines_[kCapacity];
  size_t head_ = 0;
  size_t count_ = 0;
};
