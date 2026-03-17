#include "logger.h"

void Logger::begin() {
  head_ = 0;
  count_ = 0;
}

void Logger::log(const String& event_type, const String& source, const String& message) {
  LogRecord& rec = records_[head_];
  rec.timestamp_ms = millis();
  rec.event_type = event_type;
  rec.source = source;
  rec.message = message;

  head_ = (head_ + 1) % kCapacity;
  if (count_ < kCapacity) count_++;

  Serial.println(String(rec.timestamp_ms) + "," + rec.event_type + "," + rec.source + "," + rec.message);
}

String Logger::recentEvents(size_t max_items) const {
  String out;
  const size_t items = min(max_items, count_);
  const size_t start = (head_ + kCapacity - items) % kCapacity;
  for (size_t i = 0; i < items; i++) {
    const LogRecord& rec = records_[(start + i) % kCapacity];
    out += String(rec.timestamp_ms) + "\t" + rec.event_type + "\t" + rec.source + "\t" + rec.message + "\n";
  }
  return out;
}
