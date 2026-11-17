#ifndef DEVWEBOTA_H
#define DEVWEBOTA_H

#include <Arduino.h>
#include <Preferences.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>

class DevWebOTA {
 private:
  WebServer *server;
  bool initialized;
  bool routesRegistered;
  bool identifyActive = false;
  bool otaComplete = false;
  unsigned long identifyUntil = 0;
  bool restartPending = false;
  unsigned long restartAt = 0;
  String selectedWiFiSSID;

 public:
  DevWebOTA(WebServer *srv);

  void begin();
  void handle();
  void end();
  bool isRestartPending() const { return restartPending; }
  void setWiFiSelection(const String& ssid) { selectedWiFiSSID = ssid; }
};

#endif
