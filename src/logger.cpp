#include "logger.h"

void Logger::begin() {
  head_ = 0;
  count_ = 0;
}

void Logger::log(const String& message) {
  const String line = String(millis()) + "ms | " + message;
  lines_[head_] = line;
  head_ = (head_ + 1) % kCapacity;
  if (count_ < kCapacity) {
    count_++;
  }
  Serial.println(line);
}

String Logger::recentEvents(size_t max_items) const {
  String out;
  const size_t items = min(max_items, count_);
  const size_t start = (head_ + kCapacity - items) % kCapacity;
  for (size_t i = 0; i < items; i++) {
    const size_t idx = (start + i) % kCapacity;
    out += lines_[idx];
    out += "\n";
  }
  return out;
}
