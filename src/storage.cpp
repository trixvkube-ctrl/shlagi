#include "storage.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

bool Storage::begin() {
  return LittleFS.begin();
}

String Storage::ipToString(const IPAddress& ip) {
  return ip.toString();
}

IPAddress Storage::stringToIp(const String& value, const IPAddress& fallback) {
  IPAddress ip;
  if (ip.fromString(value)) {
    return ip;
  }
  return fallback;
}

bool Storage::load(AppConfig& cfg) {
  if (!LittleFS.exists(kConfigPath)) {
    return false;
  }

  File f = LittleFS.open(kConfigPath, "r");
  if (!f) {
    return false;
  }

  JsonDocument doc;
  auto err = deserializeJson(doc, f);
  f.close();
  if (err) {
    return false;
  }

  cfg.logic.open_mode = static_cast<OpenMode>(doc["logic"]["open_mode"] | static_cast<uint8_t>(cfg.logic.open_mode));
  cfg.logic.open_pulse_ms = doc["logic"]["open_pulse_ms"] | cfg.logic.open_pulse_ms;
  cfg.logic.open_timeout_ms = doc["logic"]["open_timeout_ms"] | cfg.logic.open_timeout_ms;
  cfg.logic.open_hold_time_ms = doc["logic"]["open_hold_time_ms"] | cfg.logic.open_hold_time_ms;
  cfg.logic.close_timeout_ms = doc["logic"]["close_timeout_ms"] | cfg.logic.close_timeout_ms;
  cfg.logic.extra_wait_before_retry_ms = doc["logic"]["extra_wait_before_retry_ms"] | cfg.logic.extra_wait_before_retry_ms;
  cfg.logic.close_retries = doc["logic"]["close_retries"] | cfg.logic.close_retries;
  cfg.logic.relay_active_high = doc["logic"]["relay_active_high"] | cfg.logic.relay_active_high;
  cfg.logic.open_limit_invert = doc["logic"]["open_limit_invert"] | cfg.logic.open_limit_invert;
  cfg.logic.close_limit_invert = doc["logic"]["close_limit_invert"] | cfg.logic.close_limit_invert;
  cfg.logic.debounce_ms = doc["logic"]["debounce_ms"] | cfg.logic.debounce_ms;
  cfg.logic.allow_commands_in_error = doc["logic"]["allow_commands_in_error"] | cfg.logic.allow_commands_in_error;
  cfg.logic.auto_recover_from_error = doc["logic"]["auto_recover_from_error"] | cfg.logic.auto_recover_from_error;

  cfg.net.dhcp = doc["net"]["dhcp"] | cfg.net.dhcp;
  cfg.net.ip = stringToIp(doc["net"]["ip"] | cfg.net.ip.toString(), cfg.net.ip);
  cfg.net.subnet = stringToIp(doc["net"]["subnet"] | cfg.net.subnet.toString(), cfg.net.subnet);
  cfg.net.gateway = stringToIp(doc["net"]["gateway"] | cfg.net.gateway.toString(), cfg.net.gateway);
  cfg.net.dns = stringToIp(doc["net"]["dns"] | cfg.net.dns.toString(), cfg.net.dns);
  cfg.net.http_port = doc["net"]["http_port"] | cfg.net.http_port;
  cfg.net.trusted_ip = String((const char*)doc["net"]["trusted_ip"] | cfg.net.trusted_ip);
  cfg.net.shared_token = String((const char*)doc["net"]["shared_token"] | cfg.net.shared_token);
  cfg.net.login = String((const char*)doc["net"]["login"] | cfg.net.login);
  cfg.net.password_hash = String((const char*)doc["net"]["password_hash"] | cfg.net.password_hash);

  return true;
}

bool Storage::save(const AppConfig& cfg) {
  JsonDocument doc;

  doc["logic"]["open_mode"] = static_cast<uint8_t>(cfg.logic.open_mode);
  doc["logic"]["open_pulse_ms"] = cfg.logic.open_pulse_ms;
  doc["logic"]["open_timeout_ms"] = cfg.logic.open_timeout_ms;
  doc["logic"]["open_hold_time_ms"] = cfg.logic.open_hold_time_ms;
  doc["logic"]["close_timeout_ms"] = cfg.logic.close_timeout_ms;
  doc["logic"]["extra_wait_before_retry_ms"] = cfg.logic.extra_wait_before_retry_ms;
  doc["logic"]["close_retries"] = cfg.logic.close_retries;
  doc["logic"]["relay_active_high"] = cfg.logic.relay_active_high;
  doc["logic"]["open_limit_invert"] = cfg.logic.open_limit_invert;
  doc["logic"]["close_limit_invert"] = cfg.logic.close_limit_invert;
  doc["logic"]["debounce_ms"] = cfg.logic.debounce_ms;
  doc["logic"]["allow_commands_in_error"] = cfg.logic.allow_commands_in_error;
  doc["logic"]["auto_recover_from_error"] = cfg.logic.auto_recover_from_error;

  doc["net"]["dhcp"] = cfg.net.dhcp;
  doc["net"]["ip"] = ipToString(cfg.net.ip);
  doc["net"]["subnet"] = ipToString(cfg.net.subnet);
  doc["net"]["gateway"] = ipToString(cfg.net.gateway);
  doc["net"]["dns"] = ipToString(cfg.net.dns);
  doc["net"]["http_port"] = cfg.net.http_port;
  doc["net"]["trusted_ip"] = cfg.net.trusted_ip;
  doc["net"]["shared_token"] = cfg.net.shared_token;
  doc["net"]["login"] = cfg.net.login;
  doc["net"]["password_hash"] = cfg.net.password_hash;

  File f = LittleFS.open(kConfigPath, "w");
  if (!f) {
    return false;
  }
  serializeJsonPretty(doc, f);
  f.close();
  return true;
}
