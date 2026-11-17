#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H
#include <Arduino.h>

extern bool displayReady;
void displaySplash();
void setDisplayPower(bool enabled);
void identifyDisplay(bool enabled);
void updateDisplay();
void displayStateInfo(const char* estado);
void displayDeveloperInfo();

// Nuevas funciones para el menú de WiFi interactivo
void displayWiFiList(int selected, int offset, int total);
void displayAPAlert(const String& ssid = "");
void displayWiFiConnection(const String& ssid, int status);

#endif
