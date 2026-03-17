#include "net_server.h"

#include <ArduinoJson.h>
#include <Hash.h>

#include "web_ui.h"

namespace {
String urlDecode(const String& src) {
  String out;
  out.reserve(src.length());
  for (size_t i = 0; i < src.length(); i++) {
    char c = src[i];
    if (c == '+') {
      out += ' ';
    } else if (c == '%' && i + 2 < src.length()) {
      char hex[3] = {src[i + 1], src[i + 2], 0};
      out += static_cast<char>(strtol(hex, nullptr, 16));
      i += 2;
    } else {
      out += c;
    }
  }
  return out;
}

String queryValue(const String& blob, const String& key) {
  int start = 0;
  while (start < static_cast<int>(blob.length())) {
    int amp = blob.indexOf('&', start);
    if (amp < 0) {
      amp = blob.length();
    }
    const String pair = blob.substring(start, amp);
    const int eq = pair.indexOf('=');
    const String k = urlDecode(eq >= 0 ? pair.substring(0, eq) : pair);
    if (k == key) {
      return urlDecode(eq >= 0 ? pair.substring(eq + 1) : "");
    }
    start = amp + 1;
  }
  return "";
}

String base64Decode(const String& in) {
  const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  String out;
  int val = 0;
  int valb = -8;
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    if (isspace(c) || c == '=') {
      continue;
    }
    const char* p = strchr(chars, c);
    if (!p) {
      continue;
    }
    val = (val << 6) + (p - chars);
    valb += 6;
    if (valb >= 0) {
      out += char((val >> valb) & 0xFF);
      valb -= 8;
    }
  }
  return out;
}

String jsonStatus(BarrierController& b, const AppConfig& cfg, const String& ip) {
  JsonDocument doc;
  doc["state"] = barrierStateToString(b.state());
  doc["position"] = b.position();
  doc["relay"] = b.relayState();
  doc["open_limit"] = b.openLimit();
  doc["close_limit"] = b.closeLimit();
  doc["open_mode"] = cfg.logic.open_mode == OpenMode::PULSE ? "PULSE" : "HOLD_UNTIL_LIMIT";
  doc["retry_counter"] = b.retryCounter();
  doc["last_error"] = b.lastError();
  doc["ip"] = ip;
  doc["uptime_ms"] = millis();
  String out;
  serializeJson(doc, out);
  return out;
}
}  // namespace

NetServer::NetServer(BarrierController& barrier, Storage& storage, Logger& logger, AppConfig& config)
    : barrier_(barrier), storage_(storage), logger_(logger), config_(config), netping_(barrier) {}

bool NetServer::begin() {
  byte mac[6] = {0x02, 0x60, 0xA4, 0x82, 0x11, 0x01};

  if (config_.net.dhcp) {
    if (Ethernet.begin(mac) == 0) {
      logger_.log("DHCP failed, fallback to static IP");
      Ethernet.begin(mac, config_.net.ip, config_.net.dns, config_.net.gateway, config_.net.subnet);
    }
  } else {
    Ethernet.begin(mac, config_.net.ip, config_.net.dns, config_.net.gateway, config_.net.subnet);
  }

  delete server_;
  server_ = new EthernetServer(config_.net.http_port);
  server_->begin();
  logger_.log(String("HTTP server on ") + ipString() + ":" + config_.net.http_port);
  return true;
}

String NetServer::ipString() const {
  return Ethernet.localIP().toString();
}

bool NetServer::parseRequest(EthernetClient& client, HttpRequest& req) {
  String line = client.readStringUntil('\n');
  line.trim();
  if (!line.length()) {
    return false;
  }

  const int sp1 = line.indexOf(' ');
  const int sp2 = line.indexOf(' ', sp1 + 1);
  if (sp1 < 0 || sp2 < 0) {
    return false;
  }
  req.method = line.substring(0, sp1);
  String url = line.substring(sp1 + 1, sp2);
  int q = url.indexOf('?');
  req.path = (q >= 0) ? url.substring(0, q) : url;
  req.query = (q >= 0) ? url.substring(q + 1) : "";

  int content_length = 0;
  while (client.connected()) {
    String h = client.readStringUntil('\n');
    h.trim();
    if (!h.length()) {
      break;
    }
    if (h.startsWith("Content-Length:")) {
      content_length = h.substring(strlen("Content-Length:")).toInt();
    } else if (h.startsWith("Authorization:")) {
      req.auth_header = h.substring(strlen("Authorization:"));
      req.auth_header.trim();
    }
  }

  while (content_length-- > 0 && client.available()) {
    req.body += static_cast<char>(client.read());
  }

  req.remote_ip = client.remoteIP().toString();
  return true;
}

bool NetServer::isAuthorized(const HttpRequest& req) const {
  if (config_.net.trusted_ip.length() && req.remote_ip == config_.net.trusted_ip) {
    return true;
  }

  const String token = queryValue(req.query, "token");
  if (token.length() && token == config_.net.shared_token) {
    return true;
  }

  if (req.auth_header.startsWith("Basic ")) {
    const String decoded = base64Decode(req.auth_header.substring(6));
    const int sep = decoded.indexOf(':');
    if (sep > 0) {
      const String user = decoded.substring(0, sep);
      const String pass = decoded.substring(sep + 1);
      if (user == config_.net.login && sha1(pass) == config_.net.password_hash) {
        return true;
      }
    }
  }

  return false;
}

String NetServer::formValue(const HttpRequest& req, const String& key) const {
  const String from_query = queryValue(req.query, key);
  if (from_query.length()) {
    return from_query;
  }
  return queryValue(req.body, key);
}

void NetServer::sendResponse(EthernetClient& client, int code, const String& content_type, const String& body) {
  client.print("HTTP/1.1 ");
  client.print(code);
  client.println(code == 200 ? " OK" : " ERROR");
  client.println("Connection: close");
  client.println("Cache-Control: no-store");
  client.print("Content-Type: ");
  client.println(content_type);
  client.print("Content-Length: ");
  client.println(body.length());
  client.println();
  client.print(body);
}

void NetServer::handleApiConfigUpdate(const HttpRequest& req) {
  String val;

  val = formValue(req, "open_mode");
  if (val.length()) config_.logic.open_mode = static_cast<OpenMode>(val.toInt());
  val = formValue(req, "open_pulse_ms");
  if (val.length()) config_.logic.open_pulse_ms = val.toInt();
  val = formValue(req, "open_timeout_ms");
  if (val.length()) config_.logic.open_timeout_ms = val.toInt();
  val = formValue(req, "open_hold_time_ms");
  if (val.length()) config_.logic.open_hold_time_ms = val.toInt();
  val = formValue(req, "close_timeout_ms");
  if (val.length()) config_.logic.close_timeout_ms = val.toInt();
  val = formValue(req, "extra_wait_before_retry_ms");
  if (val.length()) config_.logic.extra_wait_before_retry_ms = val.toInt();
  val = formValue(req, "close_retries");
  if (val.length()) config_.logic.close_retries = val.toInt();
  val = formValue(req, "relay_active_high");
  if (val.length()) config_.logic.relay_active_high = val.toInt();
  val = formValue(req, "open_limit_invert");
  if (val.length()) config_.logic.open_limit_invert = val.toInt();
  val = formValue(req, "close_limit_invert");
  if (val.length()) config_.logic.close_limit_invert = val.toInt();
  val = formValue(req, "debounce_ms");
  if (val.length()) config_.logic.debounce_ms = val.toInt();
  val = formValue(req, "allow_commands_in_error");
  if (val.length()) config_.logic.allow_commands_in_error = val.toInt();
  val = formValue(req, "auto_recover_from_error");
  if (val.length()) config_.logic.auto_recover_from_error = val.toInt();

  val = formValue(req, "dhcp");
  if (val.length()) config_.net.dhcp = val.toInt();
  val = formValue(req, "ip");
  if (val.length()) config_.net.ip.fromString(val);
  val = formValue(req, "subnet");
  if (val.length()) config_.net.subnet.fromString(val);
  val = formValue(req, "gateway");
  if (val.length()) config_.net.gateway.fromString(val);
  val = formValue(req, "dns");
  if (val.length()) config_.net.dns.fromString(val);
  val = formValue(req, "http_port");
  if (val.length()) config_.net.http_port = val.toInt();
  val = formValue(req, "trusted_ip");
  if (val.length()) config_.net.trusted_ip = val;
  val = formValue(req, "shared_token");
  if (val.length()) config_.net.shared_token = val;
  val = formValue(req, "login");
  if (val.length()) config_.net.login = val;
  val = formValue(req, "password");
  if (val.length()) config_.net.password_hash = sha1(val);

  barrier_.applyConfig(config_.logic);
  storage_.save(config_);
}

void NetServer::handleRequest(EthernetClient& client, const HttpRequest& req) {
  const bool is_get = req.method == "GET";

  if (req.path != "/" && req.path != "/io.cgi" && !isAuthorized(req)) {
    sendResponse(client, 401, "text/plain", "Unauthorized");
    return;
  }

  if (is_get && req.path == "/") {
    sendResponse(client, 200, "text/html", WebUi::pageStatus(barrier_, config_, ipString(), millis(), logger_));
    return;
  }
  if (is_get && req.path == "/control") {
    sendResponse(client, 200, "text/html", WebUi::pageControl());
    return;
  }
  if (is_get && req.path == "/logic") {
    sendResponse(client, 200, "text/html", WebUi::pageLogicSettings(config_));
    return;
  }
  if (is_get && req.path == "/network") {
    sendResponse(client, 200, "text/html", WebUi::pageNetworkSettings(config_));
    return;
  }
  if (req.path == "/io.cgi") {
    const int idx = req.query.indexOf("io1");
    bool has_value = false;
    String val;
    if (idx >= 0) {
      int eq = req.query.indexOf('=', idx);
      if (eq >= 0) {
        has_value = true;
        val = queryValue(req.query, "io1");
      }
    }
    sendResponse(client, 200, "text/plain", netping_.process(val, has_value));
    return;
  }

  if (is_get && req.path == "/api/status") {
    sendResponse(client, 200, "application/json", jsonStatus(barrier_, config_, ipString()));
    return;
  }
  if (req.method == "POST" && req.path == "/api/open") {
    const bool ok = barrier_.requestOpen();
    sendResponse(client, ok ? 200 : 409, "application/json", String("{\"ok\":") + (ok ? "true" : "false") + "}");
    return;
  }
  if (req.method == "POST" && req.path == "/api/close") {
    const bool ok = barrier_.requestClose();
    sendResponse(client, ok ? 200 : 409, "application/json", String("{\"ok\":") + (ok ? "true" : "false") + "}");
    return;
  }
  if (req.method == "POST" && req.path == "/api/reset_error") {
    barrier_.resetError();
    sendResponse(client, 200, "application/json", "{\"ok\":true}");
    return;
  }
  if (req.method == "POST" && req.path == "/api/test_relay") {
    uint32_t ms = formValue(req, "ms").toInt();
    if (ms == 0) ms = 1000;
    barrier_.testRelay(ms);
    sendResponse(client, 200, "application/json", "{\"ok\":true}");
    return;
  }
  if (is_get && req.path == "/api/config") {
    JsonDocument doc;
    doc["open_mode"] = static_cast<int>(config_.logic.open_mode);
    doc["open_pulse_ms"] = config_.logic.open_pulse_ms;
    doc["open_timeout_ms"] = config_.logic.open_timeout_ms;
    doc["open_hold_time_ms"] = config_.logic.open_hold_time_ms;
    doc["close_timeout_ms"] = config_.logic.close_timeout_ms;
    doc["close_retries"] = config_.logic.close_retries;
    doc["dhcp"] = config_.net.dhcp;
    doc["ip"] = config_.net.ip.toString();
    doc["http_port"] = config_.net.http_port;
    String out;
    serializeJson(doc, out);
    sendResponse(client, 200, "application/json", out);
    return;
  }
  if (req.method == "POST" && req.path == "/api/config") {
    handleApiConfigUpdate(req);
    sendResponse(client, 200, "application/json", "{\"ok\":true}");
    return;
  }

  sendResponse(client, 404, "text/plain", "Not found");
}

void NetServer::update() {
  if (!server_) {
    return;
  }

  EthernetClient client = server_->available();
  if (!client) {
    return;
  }

  client.setTimeout(200);
  HttpRequest req;
  if (parseRequest(client, req)) {
    handleRequest(client, req);
  }

  yield();
  client.stop();
}
