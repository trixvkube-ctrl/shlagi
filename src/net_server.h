#pragma once

#include <EthernetENC.h>

#include "barrier_controller.h"
#include "config.h"
#include "logger.h"
#include "netping_compat.h"
#include "storage.h"

class NetServer {
 public:
  NetServer(BarrierController& barrier, Storage& storage, Logger& logger, AppConfig& config);

  bool begin();
  void update();
  String ipString() const;

 private:
  struct HttpRequest {
    String method;
    String path;
    String query;
    String body;
    String remote_ip;
    String auth_header;
  };

  bool parseRequest(EthernetClient& client, HttpRequest& req);
  void handleRequest(EthernetClient& client, const HttpRequest& req);
  void sendResponse(EthernetClient& client, int code, const String& content_type, const String& body);
  bool isAuthorized(const HttpRequest& req) const;
  String formValue(const HttpRequest& req, const String& key) const;
  void handleApiConfigUpdate(const HttpRequest& req);

  BarrierController& barrier_;
  Storage& storage_;
  Logger& logger_;
  AppConfig& config_;
  NetPingCompat netping_;
  EthernetServer* server_ = nullptr;
};
