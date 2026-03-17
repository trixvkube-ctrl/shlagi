#pragma once

#include <Arduino.h>

class BarrierController;

class NetPingCompat {
 public:
  explicit NetPingCompat(BarrierController& barrier) : barrier_(barrier) {}
  String process(const String& io1_value, bool has_value);

 private:
  BarrierController& barrier_;
};
