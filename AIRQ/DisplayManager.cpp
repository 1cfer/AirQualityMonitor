#include "DisplayManager.h"

#include <Adafruit_SSD1306.h>
#include <WiFi.h>

#include "AppConfig.h"
#include "SensorManager.h"
#include "StateMachine.h"
#include "WiFiManager.h"

extern Adafruit_SSD1306 display;
extern StateMachine stateMachine;
extern WiFiManager wifiManager;

const char* categoriaPm25(uint16_t pm25) {
  if (pm25 <= 12) return "Buena";
  if (pm25 <= 35) return "Moderada";
  if (pm25 <= 55) return "Danina (sensib.)";
  if (pm25 <= 150) return "Danina";
  if (pm25 <= 250) return "Muy danina";
  return "Peligrosa";
}

// Barra horizontal con etiqueta y valor. maxScale define el 100% de la barra.
void drawBar(int y, const char* label, uint16_t value, uint16_t maxScale) {
  display.setCursor(0, y);
  display.print(label);

  int barX = 34;
  int barWidth = 70;
  int barHeight = 7;

  display.drawRect(barX, y - 1, barWidth, barHeight, SSD1306_WHITE);
  int fillWidth = map(min(value, maxScale), 0, maxScale, 0, barWidth - 2);
  if (fillWidth > 0) {
    display.fillRect(barX + 1, y, fillWidth, barHeight - 2, SSD1306_WHITE);
  }

  display.setCursor(barX + barWidth + 4, y);
  display.print(value);
}

// Carita simple según categoría: feliz / neutral / triste
void drawFace(int cx, int cy, int r, uint16_t pm25) {
  display.drawCircle(cx, cy, r, SSD1306_WHITE);

  // Ojos
  display.fillCircle(cx - r / 2, cy - r / 3, 2, SSD1306_WHITE);
  display.fillCircle(cx + r / 2, cy - r / 3, 2, SSD1306_WHITE);

  // Boca según nivel: buena=sonrisa, moderada=neutral, mala=triste
  if (pm25 <= 12) {
    display.drawLine(cx - r / 2, cy + r / 3, cx, cy + r / 2, SSD1306_WHITE);
    display.drawLine(cx, cy + r / 2, cx + r / 2, cy + r / 3, SSD1306_WHITE);
  } else if (pm25 <= 35) {
    display.drawLine(cx - r / 2, cy + r / 2, cx + r / 2, cy + r / 2, SSD1306_WHITE);
  } else {
    display.drawLine(cx - r / 2, cy + r / 2, cx, cy + r / 3, SSD1306_WHITE);
    display.drawLine(cx, cy + r / 3, cx + r / 2, cy + r / 2, SSD1306_WHITE);
  }
}

void updateDisplay() {
  if (!stateMachine.isDisplayOn) return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  displayStateInfo("LECTURA");

  switch (stateMachine.screenMode) {
    case 0: {  // Calidad de aire — barras
      display.setTextSize(1);
      display.setCursor(0, 13);
      display.println("CALIDAD DE AIRE");

      drawBar(26, "PM1.0", stateMachine.sensors.pm1_0, 150);
      drawBar(40, "PM2.5", stateMachine.sensors.pm2_5, 150);
      drawBar(54, "PM10 ", stateMachine.sensors.pm10, 150);
      break;
    }

    case 1: {  // PM2.5 + carita
      display.setTextSize(1);
      display.setCursor(0, 13);
      display.print("PM2.5:");

      display.setTextSize(2);
      display.setCursor(0, 24);
      display.print(stateMachine.sensors.pm2_5);
      display.setTextSize(1);
      display.print(" ug/m3");

      drawFace(100, 40, 14, stateMachine.sensors.pm2_5);

      display.setCursor(0, 55);
      display.print(categoriaPm25(stateMachine.sensors.pm2_5));
      break;
    }

    case 2: {  // Stats del intervalo actual (reset con cada envío)
      display.setTextSize(1);
      display.setCursor(0, 13);
      display.println("INTERVALO ACTUAL");
      display.drawLine(0, 21, 128, 21, SSD1306_WHITE);

      display.setCursor(0, 25);
      display.print("Min:  ");
      display.print(sensorManager.getMinPm2_5());
      display.println(" ug/m3");

      display.setCursor(0, 36);
      display.print("Max:  ");
      display.print(sensorManager.getMaxPm2_5());
      display.println(" ug/m3");

      display.setCursor(0, 47);
      display.print("Muestras: ");
      display.println(sensorManager.getSampleCount());

      unsigned long restante = 0;
      if (stateMachine.clocks.proximo_envio > stateMachine.clocks.tiempo_actual) {
        restante = (stateMachine.clocks.proximo_envio - stateMachine.clocks.tiempo_actual) / 1000;
      }
      display.setCursor(0, 57);
      display.print("Prox. envio: ");
      display.print(restante);
      display.println("s");
      break;
    }

    case 3: {  // Tendencia — sparkline
      display.setTextSize(1);
      display.setCursor(0, 13);
      display.println("TENDENCIA PM2.5");

      int graphX = 0, graphY = 24, graphW = 128, graphH = 34;
      display.drawRect(graphX, graphY, graphW, graphH, SSD1306_WHITE);

      int count = sensorManager.getPm25HistoryCount();
      if (count >= 2) {
        const uint16_t* hist = sensorManager.getPm25History();
        int size = sensorManager.getPm25HistorySize();
        int startIdx = sensorManager.getPm25HistoryIndex();  // más antiguo si buffer lleno

        uint16_t maxVal = 1;
        for (int i = 0; i < count; i++) {
          int idx = (startIdx + i) % size;
          if (hist[idx] > maxVal) maxVal = hist[idx];
        }

        int prevX = -1, prevY = -1;
        for (int i = 0; i < count; i++) {
          int idx = (startIdx + (size - count) + i) % size;
          int x = graphX + (i * (graphW - 2)) / max(count - 1, 1) + 1;
          int y = graphY + graphH - 2 - map(hist[idx], 0, maxVal, 0, graphH - 4);
          if (prevX >= 0) display.drawLine(prevX, prevY, x, y, SSD1306_WHITE);
          prevX = x;
          prevY = y;
        }
      }
      break;
    }

    case 4: {  // Red WiFi
      display.setTextSize(1);
      display.setCursor(0, 13);
      display.println("RED WIFI");
      display.drawLine(0, 21, 128, 21, SSD1306_WHITE);

      display.setCursor(0, 25);
      display.print("SSID: ");
      if (WiFi.status() == WL_CONNECTED) {
        String ssid = WiFi.SSID();
        if (ssid.length() > 14) ssid = ssid.substring(0, 14);
        display.println(ssid);
      } else {
        display.println("Desconectado");
      }

      display.setCursor(0, 36);
      display.print("RSSI: ");
      display.print(WiFi.RSSI());
      display.println(" dBm");

      display.setCursor(0, 47);
      display.print("Host: ");
      display.println(appConfig.hostname);
      break;
    }
  }

  display.display();
}

void displayStateInfo(const char* estado) {
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Estado: ");
  display.println(estado);
  display.drawLine(0, 9, 128, 9, SSD1306_WHITE);
}

void displayDeveloperInfo() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("DESARROLLADOR");
  display.drawLine(0, 9, 128, 9, SSD1306_WHITE);

  display.setCursor(0, 12);
  display.print("IP:");
  display.println(wifiManager.getIP());

  display.setCursor(0, 22);
  if (WiFi.status() == WL_CONNECTED) {
    display.print("Red:");
    String ssid = WiFi.SSID();
    if (ssid.length() > 17) ssid = ssid.substring(0, 17);
    display.println(ssid);
  } else if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    display.print("AP:");
    String apName = "TARS-" + appConfig.hostname;
    if (apName.length() > 18) apName = apName.substring(0, 18);
    display.println(apName);
  } else {
    display.println("Sin conexion");
  }

  display.setCursor(0, 32);
  display.print("Key:");
  display.print(appConfig.skipToken ? "OFF" : "ON ");
  display.print(" E:");
  if (appConfig.intervaloEnvio >= 60000) {
    display.print(appConfig.intervaloEnvio / 60000);
    display.println("m");
  } else {
    display.print(appConfig.intervaloEnvio / 1000);
    display.println("s");
  }

  display.drawLine(0, 41, 128, 41, SSD1306_WHITE);

  display.setCursor(0, 44);
  String hostLine = appConfig.hostname;
  if (hostLine.length() > 15) hostLine = hostLine.substring(0, 15);
  display.print(hostLine);
  display.println(".local");

  display.setCursor(0, 54);
  display.print("RSSI:");
  display.print(WiFi.RSSI());
  display.print("dBm Heap:");
  display.print(ESP.getFreeHeap() / 1024);
  display.println("K");

  display.display();
}