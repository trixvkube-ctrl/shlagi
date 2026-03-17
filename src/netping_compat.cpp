#include "netping_compat.h"

#include "barrier_controller.h"

String NetPingCompat::process(const String& io1_value, bool has_value) {
  if (!has_value || io1_value == "f") {
    return String("io_result('") + (barrier_.relayState() ? "1" : "0") + "')\r\n";
  }

  if (io1_value == "1") {
    const bool ok = barrier_.requestOpen();
    return ok ? "io_result('ok')\r\n" : "io_result('error')\r\n";
  }

  if (io1_value == "0") {
    const bool ok = barrier_.requestClose();
    return ok ? "io_result('ok')\r\n" : "io_result('error')\r\n";
  }

  if (io1_value.startsWith("f,")) {
    int sec = io1_value.substring(2).toInt();
    if (sec <= 0) sec = 1;
    barrier_.testRelay(static_cast<uint32_t>(sec) * 1000UL);
    return "io_result('ok')\r\n";
  }

  return "io_result('error')\r\n";
}
