#pragma once
#include "Arduino.h"
#include <vector>
#define WL_CONNECTED 3
#define WIFI_STA 1
#define WIFI_AP 2
#define WIFI_AP_STA 3
#define WIFI_SCAN_RUNNING -1
#define WIFI_SCAN_FAILED -2
#define WIFI_AUTH_OPEN 0
#define WIFI_AUTH_WPA2_PSK 3
struct TestIp { String toString() { return "192.168.4.1"; } };
struct TestWiFi {
    int state=0, currentMode=WIFI_STA, begins=0;
    bool immediate=false;
    int scanResult=0;
    String currentSsid="AgeVital-Lab", apName, lastPass;
    std::vector<String> networks={"Red de prueba con nombre muy largo"};
    std::vector<int> security={WIFI_AUTH_WPA2_PSK};
    void persistent(bool) {}
    void setAutoReconnect(bool) {}
    void mode(int m) { currentMode=m; }
    int getMode() { return currentMode; }
    int status() { return state; }
    void begin(const char* s,const char* p) { ++begins;currentSsid=s;lastPass=p;state=immediate?WL_CONNECTED:0; }
    void disconnect(bool,bool) { state=0; }
    void reconnect() { ++begins; }
    bool softAP(const char* s,const char*) { apName=s;return true; }
    String softAPSSID() { return apName; }
    TestIp softAPIP() { return {}; }
    TestIp localIP() { return {}; }
    String SSID(int index=-1) { return index<0?currentSsid:networks.at(index); }
    int encryptionType(int index) { return security.at(index); }
    int RSSI() { return -55; }
    void scanNetworks(bool) {}
    int scanComplete() { return scanResult; }
    void scanDelete() {}
    void enableAP(bool enabled) { if(!enabled)currentMode=WIFI_STA; }
};
inline TestWiFi WiFi;
