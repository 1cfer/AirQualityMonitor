#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <DFRobot_AirQualitySensor.h>
#include <EEPROM.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>

#include "AppConfig.h"
#include "ButtonHandler.h"
#include "DisplayManager.h"
#include "Estados.h"
#include "PayloadBuilder.h"
#include "SensorManager.h"
#include "State.h"
#include "StateMachine.h"
#include "TokenManager.h"
#include "WiFiManager.h"

// ===== HARDWARE =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define BUTTON_PIN 4          // GPIO 4: pull-up interno, botón entre este pin y GND
#define AIR_QUALITY_I2C_ADDR 0x19

// ===== OBJETOS GLOBALES =====
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
DFRobot_AirQualitySensor particle(&Wire, AIR_QUALITY_I2C_ADDR);
WebServer server(80);
AppConfig appConfig;
WiFiManager wifiManager;
TokenManager tokenManager;
ButtonHandler buttonHandler(BUTTON_PIN);
StateMachine stateMachine;
SensorManager sensorManager;

SET_LOOP_TASK_STACK_SIZE(16384);

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== TARS — Solo Calidad de Aire ===");
  Serial.println("Con Máquina de Estados Modular\n");

  Wire.begin(21, 22);
  Wire.setClock(50000);

  // Pantalla
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Error OLED");
    while (1);
  }

  // Botón
  buttonHandler.begin();

  // Configuración desde NVS
  appConfig.begin();

  // Splash screen
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(10, 10);
  display.println("AgeVital");
  display.setTextSize(1);
  display.setCursor(20, 35);
  display.println("TARS - Aire");
  display.setCursor(5, 50);
  display.print("Modulo: ");
  display.println(appConfig.hostname);
  display.display();

  // Sensor de calidad de aire
  sensorManager.begin();

  // Arrancar máquina de estados
  stateMachine.begin(new EstadoINICIO());
}

void loop() {
  ButtonHandler::Event event = buttonHandler.update();

  if (event == ButtonHandler::SHORT_PRESS) {
    stateMachine.clocks.ultima_interaccion = millis();

    if (!stateMachine.isDisplayOn) {
      stateMachine.isDisplayOn = true;
    } else {
      stateMachine.screenMode++;
      if (stateMachine.screenMode > 4) stateMachine.screenMode = 0;  // 5 pantallas: 0-4
    }
    stateMachine.needsUpdate = true;
    Serial.println("[Button] Short press -> cambiar pantalla");
  }

  if (event == ButtonHandler::LONG_PRESS) {
    if (stateMachine.flags.dev) {
      Serial.println("[Button] Long press -> salir de DESARROLLADOR");
      stateMachine.flags.dev = false;
    } else {
      Serial.println("[Button] Long press -> entrar a DESARROLLADOR");
      stateMachine.flags.dev = true;
    }
  }

  stateMachine.update();
}