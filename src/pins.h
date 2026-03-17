#pragma once

#include <Arduino.h>

namespace Pins {
static constexpr uint8_t ETH_CS = 4;         // D2
static constexpr uint8_t RELAY_OPEN = 16;    // D0
static constexpr uint8_t OPEN_LIMIT = 5;     // D1
static constexpr uint8_t CLOSE_LIMIT = 3;    // RX
}  // namespace Pins
