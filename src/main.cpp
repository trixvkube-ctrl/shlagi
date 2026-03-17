#include <Arduino.h>
#include <SPI.h>
#include <EthernetENC.h>
#include <Hash.h>

#include "barrier_controller.h"
#include "config.h"
#include "logger.h"
#include "net_server.h"
#include "pins.h"
#include "storage.h"

Logger logger;
Storage storage;
AppConfig config;
BarrierController barrier(logger);
NetServer* net_server = nullptr;

void setup() {
  Serial.begin(115200);
  logger.begin();
  logger.log("booting");

  if (!storage.begin()) {
    logger.log("LittleFS mount failed");
  }
  if (storage.load(config)) {
    logger.log("config loaded");
  } else {
    logger.log("using default config");
    config.net.password_hash = sha1(String("admin"));
    storage.save(config);
  }

  SPI.begin();
  Ethernet.init(Pins::ETH_CS);

  barrier.begin(config.logic);

  net_server = new NetServer(barrier, storage, logger, config);
  net_server->begin();
}

void loop() {
  barrier.update();
  if (net_server) {
    net_server->update();
  }
  yield();
}
