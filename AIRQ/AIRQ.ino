#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

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
#include "FirmwareInfo.h"
#include "Telemetry.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define BUTTON_PIN 27

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1, 50000, 50000);


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
  Serial.printf("TARS firmware %s\n", TARS_FIRMWARE_VERSION);

  Wire.begin(21, 22);
  Wire.setTimeOut(50);
  Wire.setClock(50000);

  // 1. Cargar configuración desde memoria EEPROM/NVS
  appConfig.begin();

  // 2. Inicializar pantalla OLED y mostrar portada de inmediato
  uint8_t oledAddress = 0x3C;
  Wire.beginTransmission(oledAddress);
  bool oledFound = Wire.endTransmission() == 0;
  if (!oledFound) {
    oledAddress = 0x3D;
    Wire.beginTransmission(oledAddress);
    oledFound = Wire.endTransmission() == 0;
  }
  displayReady = oledFound && display.begin(SSD1306_SWITCHCAPVCC, oledAddress, true, false);
  stateMachine.isDisplayOn = displayReady;
  if (displayReady) {
    displaySplash();
    delay(1500);
  } else {
    Serial.println("[DISPLAY] Error al iniciar SSD1306");
  }

  Wire.setTimeOut(50);
  Wire.setClock(50000);
  if (!telemetry.begin()) Serial.println("[ENVIO] No se pudo crear tarea de red");

  // 4. Inicializar subsistemas y máquina de estados
  buttonHandler.begin();
  sensorManager.begin();
  sensorManager.read();
  stateMachine.clocks.tiempo_lectura = millis();
  stateMachine.clocks.ultima_interaccion = millis();
  stateMachine.begin(new EstadoINICIO());
}

void loop() {
  telemetry.poll();
  ButtonHandler::Event event = buttonHandler.update();

  if (event != ButtonHandler::NONE) {
    // Actualizamos el reloj de interacción siempre que se presione el botón
    stateMachine.clocks.ultima_interaccion = millis();

    // Si la pantalla estaba apagada, la despertamos
    if (displayReady && !stateMachine.isDisplayOn) {
      stateMachine.isDisplayOn = true;
      setDisplayPower(true);
      stateMachine.needsUpdate = true;

      // Si el evento fue un clic corto para despertar la pantalla,
      // lo anulamos para que NO avance a la siguiente vista de sensores
      if (event == ButtonHandler::SHORT_PRESS || event == ButtonHandler::DOUBLE_CLICK) {
        event = ButtonHandler::NONE;
      }
    }
  }

  // Pasamos el evento a los estados
  stateMachine.currentEvent = event;
  stateMachine.update();
  stateMachine.currentEvent = ButtonHandler::NONE; // Limpiamos el evento

  delay(2); // Yield CPU to WiFi and the network worker.
}
