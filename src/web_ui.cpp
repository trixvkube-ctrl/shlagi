#include "web_ui.h"

namespace {
String htmlHead(const String& title) {
  return "<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
         "<style>body{font-family:Arial;margin:18px}table{border-collapse:collapse}td,th{border:1px solid #ccc;padding:6px}"
         "input,select{width:100%;padding:6px}button{padding:8px 12px;margin:4px}</style>"
         "<title>" +
         title + "</title></head><body><h1>" + title + "</h1><nav><a href='/'>Status</a> | <a href='/control'>Control</a> | <a href='/logic'>Logic settings</a> | <a href='/network'>Network settings</a></nav><hr>";
}

String boolText(bool value) { return value ? "ON" : "OFF"; }
}  // namespace

String WebUi::pageStatus(const BarrierController& barrier, const AppConfig& cfg, const String& ip, uint32_t uptime_ms, const Logger& logger) {
  String s = htmlHead("Barrier status");
  s += "<table>";
  s += "<tr><th>Current state</th><td>" + String(barrierStateToString(barrier.state())) + "</td></tr>";
  s += "<tr><th>Barrier position</th><td>" + barrier.position() + "</td></tr>";
  s += "<tr><th>Relay state</th><td>" + boolText(barrier.relayState()) + "</td></tr>";
  s += "<tr><th>Open limit status</th><td>" + boolText(barrier.openLimit()) + "</td></tr>";
  s += "<tr><th>Close limit status</th><td>" + boolText(barrier.closeLimit()) + "</td></tr>";
  s += "<tr><th>Open mode</th><td>" + String(cfg.logic.open_mode == OpenMode::PULSE ? "PULSE" : "HOLD_UNTIL_LIMIT") + "</td></tr>";
  s += "<tr><th>Retry counter</th><td>" + String(barrier.retryCounter()) + "</td></tr>";
  s += "<tr><th>Last error</th><td>" + barrier.lastError() + "</td></tr>";
  s += "<tr><th>IP address</th><td>" + ip + "</td></tr>";
  s += "<tr><th>Uptime</th><td>" + String(uptime_ms / 1000) + " sec</td></tr>";
  s += "</table><h3>Last events</h3><pre>timestamp\tevent\tsource\tmessage\n" + logger.recentEvents(20) + "</pre>";
  s += "</body></html>";
  return s;
}

String WebUi::pageControl() {
  String s = htmlHead("Barrier control");
  s += "<form method='post' action='/api/open'><button>Open barrier</button></form>";
  s += "<form method='post' action='/api/close'><button>Close barrier</button></form>";
  s += "<form method='post' action='/api/reset_error'><button>Reset error</button></form>";
  s += "<form method='post' action='/api/test_relay'><input name='ms' value='1000'><button>Test relay</button></form>";
  s += "</body></html>";
  return s;
}

String WebUi::pageLogicSettings(const AppConfig& cfg) {
  String s = htmlHead("Logic settings");
  s += "<form method='post' action='/api/config'>";
  s += "Open mode<select name='open_mode'><option value='0'" + String(cfg.logic.open_mode == OpenMode::PULSE ? " selected" : "") + ">PULSE</option><option value='1'" + String(cfg.logic.open_mode == OpenMode::HOLD_UNTIL_LIMIT ? " selected" : "") + ">HOLD_UNTIL_LIMIT</option></select><br>";
  s += "Open pulse duration (ms)<input name='open_pulse_ms' value='" + String(cfg.logic.open_pulse_ms) + "'><br>";
  s += "Open timeout (ms)<input name='open_timeout_ms' value='" + String(cfg.logic.open_timeout_ms) + "'><br>";
  s += "Open hold time (ms)<input name='open_hold_time_ms' value='" + String(cfg.logic.open_hold_time_ms) + "'><br>";
  s += "Close timeout (ms)<input name='close_timeout_ms' value='" + String(cfg.logic.close_timeout_ms) + "'><br>";
  s += "Extra wait before retry (ms)<input name='extra_wait_before_retry_ms' value='" + String(cfg.logic.extra_wait_before_retry_ms) + "'><br>";
  s += "Close retries<input name='close_retries' value='" + String(cfg.logic.close_retries) + "'><br>";
  s += "Relay logic<select name='relay_invert'><option value='0'" + String(!cfg.logic.relay_invert ? " selected" : "") + ">Active HIGH</option><option value='1'" + String(cfg.logic.relay_invert ? " selected" : "") + ">Active LOW</option></select><br>";
  s += "Open limit invert<select name='open_limit_invert'><option value='0'" + String(!cfg.logic.open_limit_invert ? " selected" : "") + ">No</option><option value='1'" + String(cfg.logic.open_limit_invert ? " selected" : "") + ">Yes</option></select><br>";
  s += "Close limit invert<select name='close_limit_invert'><option value='0'" + String(!cfg.logic.close_limit_invert ? " selected" : "") + ">No</option><option value='1'" + String(cfg.logic.close_limit_invert ? " selected" : "") + ">Yes</option></select><br>";
  s += "Debounce time (ms)<input name='debounce_ms' value='" + String(cfg.logic.debounce_ms) + "'><br>";
  s += "Dual limit error time (ms)<input name='dual_limit_error_ms' value='" + String(cfg.logic.dual_limit_error_ms) + "'><br>";
  s += "Allow commands in ERROR<select name='allow_commands_in_error'><option value='0'" + String(!cfg.logic.allow_commands_in_error ? " selected" : "") + ">No</option><option value='1'" + String(cfg.logic.allow_commands_in_error ? " selected" : "") + ">Yes</option></select><br>";
  s += "Auto recover from ERROR<select name='auto_recover_from_error'><option value='0'" + String(!cfg.logic.auto_recover_from_error ? " selected" : "") + ">No</option><option value='1'" + String(cfg.logic.auto_recover_from_error ? " selected" : "") + ">Yes</option></select><br>";
  s += "<button>Save</button></form></body></html>";
  return s;
}

String WebUi::pageNetworkSettings(const AppConfig& cfg) {
  String s = htmlHead("Network settings");
  s += "<form method='post' action='/api/config'>";
  s += "DHCP<select name='dhcp'><option value='1'" + String(cfg.net.dhcp ? " selected" : "") + ">Enabled</option><option value='0'" + String(!cfg.net.dhcp ? " selected" : "") + ">Disabled</option></select><br>";
  s += "Static IP<input name='ip' value='" + cfg.net.ip.toString() + "'><br>";
  s += "Subnet mask<input name='subnet' value='" + cfg.net.subnet.toString() + "'><br>";
  s += "Gateway<input name='gateway' value='" + cfg.net.gateway.toString() + "'><br>";
  s += "DNS<input name='dns' value='" + cfg.net.dns.toString() + "'><br>";
  s += "HTTP port<input name='http_port' value='" + String(cfg.net.http_port) + "'><br>";
  s += "Trusted IP<input name='trusted_ip' value='" + cfg.net.trusted_ip + "'><br>";
  s += "Shared token<input name='shared_token' value='" + cfg.net.shared_token + "'><br>";
  s += "Login<input name='login' value='" + cfg.net.login + "'><br>";
  s += "Password (stored as hash)<input name='password' type='password'><br>";
  s += "<button>Save</button></form></body></html>";
  return s;
}
