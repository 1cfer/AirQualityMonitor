#include "Estados.h"
#include <WiFiClientSecure.h>
#include <ESPmDNS.h>

#include "AppConfig.h"
#include "DevWebOTA.h"
#include "TokenManager.h"
#include "WiFiManager.h"
#include "DisplayManager.h"
#include "HttpTransport.h"
#include <Update.h>
#include "Telemetry.h"

DevWebOTA *devWeb = nullptr;
extern Adafruit_SSD1306 display;
extern WebServer server;
extern SensorManager sensorManager;

// ========================================
// IMPLEMENTACIÓN ESTADO INICIO
// ========================================
void EstadoINICIO::onEnter() {
    Serial.println("===Entrando en Estado INICIO===");
    statemachine->flags.inicio = true;
    firstRun = true;
}

void EstadoINICIO::execute() {
    if (!firstRun) return;

    if (!appConfig.isConfigured) {
        Serial.println("[INICIO] Primer boot detectado — forzando modo DESARROLLADOR para setup");
        statemachine->flags.dev = true;
        statemachine->ChangeState(new EstadoDESARROLLADOR());
        return;
    }

    if (statemachine->flags.dev) {
        Serial.println("Cambiando a Estado DESARROLLADOR desde INICIO");
        statemachine->ChangeState(new EstadoDESARROLLADOR());
        return;
    }

    Serial.println("Estado: INICIO");

    if (wifiManager.connect(0)) {
        Serial.println("[INICIO] WiFi listo para envío de datos");
        MDNS.begin(appConfig.hostname.c_str());
        Serial.printf("[mDNS] Activo en http://%s.local\n", appConfig.hostname.c_str());
    } else {
        Serial.println("[INICIO] Sin WiFi, autoReconnect activo");
    }

    statemachine->flags.inicio = false;
    statemachine->clocks.proximo_envio = millis() + appConfig.intervaloEnvio;
    statemachine->flags.envio_programado = true;

    statemachine->ChangeState(new EstadoLECTURA());
    firstRun = false;
}

void EstadoINICIO::onExit() {
    Serial.println("===Saliendo de Estado INICIO===");
    statemachine->flags.inicio = false;
    firstRun = false;
}

const char *EstadoINICIO::getName() { return "INICIO"; }

// ========================================
// IMPLEMENTACIÓN ESTADO LECTURA
// ========================================
void EstadoLECTURA::onEnter() {
    Serial.println("===Entrando en Estado LECTURA===");
    statemachine->flags.lectura = true;
    statemachine->flags.inicio = false;
    statemachine->flags.envio = false;
    statemachine->needsUpdate = true;
}

void EstadoLECTURA::execute() {
    uint32_t now = millis();
    auto btn = statemachine->currentEvent;
    if (btn == ButtonHandler::SHORT_PRESS) {
        statemachine->screenMode = (statemachine->screenMode + 1) % 5;
        statemachine->needsUpdate = true;
    } else if (btn == ButtonHandler::DOUBLE_CLICK) {
        statemachine->screenMode = 0; statemachine->needsUpdate = true;
    } else if (btn == ButtonHandler::VERY_LONG_PRESS_4S) {
        statemachine->flags.dev = true;
    }
    // Finish the single in-flight job before opening configuration / OTA.
    if (statemachine->flags.dev && !telemetry.busy()) {
        statemachine->ChangeState(new EstadoDESARROLLADOR()); return;
    }
    wifiManager.maintainConnection();
    if (now - statemachine->clocks.tiempo_lectura >= appConfig.intervaloLectura) {
        sensorManager.read(); statemachine->clocks.tiempo_lectura = now;
        statemachine->needsUpdate = true;
    }
    if (!statemachine->flags.dev && !telemetry.busy() &&
        deadlineReached(now, statemachine->clocks.proximo_envio)) {
        if (!wifiManager.isConnected() || !telemetry.start())
            statemachine->clocks.proximo_envio = now + appConfig.intervaloReintento;
        statemachine->needsUpdate = true;
    }
    if (statemachine->isDisplayOn && now - statemachine->clocks.ultima_interaccion > appConfig.tiempoInactividad) {
        setDisplayPower(false); statemachine->isDisplayOn = false;
    }
    static uint32_t lastFrame = 0;
    if (statemachine->isDisplayOn && (statemachine->needsUpdate || now - lastFrame >= 1000)) {
        updateDisplay(); statemachine->needsUpdate = false; lastFrame = now;
    }
}

void EstadoLECTURA::onExit() {
    Serial.println("===Saliendo de Estado LECTURA===");
    statemachine->flags.lectura = false;
}

const char *EstadoLECTURA::getName() { return "LECTURA"; }

// ========================================
// IMPLEMENTACIÓN ESTADO ENVIO
// ========================================
// Compatibility state: networking is scheduled from LECTURA, never replaces OLED.
void EstadoENVIO::onEnter() {}
void EstadoENVIO::execute() { statemachine->ChangeState(new EstadoLECTURA()); }
void EstadoENVIO::onExit() {}
const char* EstadoENVIO::getName() { return "ENVIO"; }

// ========================================
// IMPLEMENTACIÓN ESTADO DESARROLLADOR
// ========================================
void EstadoDESARROLLADOR::onEnter() {
    Serial.println("=== ENTRANDO A ESTADO: DESARROLLADOR ===");
    statemachine->flags.dev = true;
    statemachine->flags.inicio = false;
    statemachine->flags.lectura = false;
    primera_vez = true;

    // Reiniciamos las variables de navegación al entrar
    subMode = 0;
    selectedNetwork = 0;
    scrollOffset = 0;
    totalNetworks = 0;
    statemachine->isDisplayOn = displayReady;
    setDisplayPower(true);

    displayDeveloperInfo();
    if (wifiManager.isConnected()) MDNS.begin(appConfig.hostname.c_str());
}

void EstadoDESARROLLADOR::execute() {
    ButtonHandler::Event btn = statemachine->currentEvent;
    unsigned long now = millis();

    if (primera_vez) {
        if (!devWeb) devWeb = new DevWebOTA(&server);
        devWeb->begin();
        primera_vez = false;
        displayDeveloperInfo();
    }
    devWeb->handle();
    if (Update.isRunning() || devWeb->isRestartPending()) return;
    if (!statemachine->flags.dev) {
        statemachine->ChangeState(new EstadoINICIO()); return;
    }
    if (subMode != 1) wifiManager.maintainConnection();
    if (now - statemachine->clocks.tiempo_lectura >= appConfig.intervaloLectura) {
        sensorManager.read();
        statemachine->clocks.tiempo_lectura = millis();
    }
    if (btn == ButtonHandler::VERY_LONG_PRESS_4S && (subMode != 2 || totalNetworks == 0)) {
        if (!appConfig.isConfigured) appConfig.save();
        statemachine->ChangeState(new EstadoINICIO());
        return;
    }

    // ===== SUB-MODO 0: MENÚ PRINCIPAL =====
    if (subMode == 0) {
        if (now - lastDisplayRefresh >= 2000) {
            displayDeveloperInfo();
            lastDisplayRefresh = now;
        }

        if (btn == ButtonHandler::SHORT_PRESS) {
            subMode = 1;
            WiFi.scanDelete();
            WiFi.scanNetworks(true);
            scanStarted = now;
            displayStateInfo("Buscando WiFi...");
        }
    }
    // ===== SUB-MODO 1: ESCANEANDO =====
    else if (subMode == 1) {
        int n = WiFi.scanComplete();
        if (btn == ButtonHandler::DOUBLE_CLICK || n == WIFI_SCAN_FAILED || now - scanStarted > 15000) {
            WiFi.scanDelete();
            subMode = 0;
            displayDeveloperInfo();
        } else if (n >= 0) {
            totalNetworks = n;
            selectedNetwork = 0;
            scrollOffset = 0;
            subMode = 2;
            displayWiFiList(selectedNetwork, scrollOffset, totalNetworks);
        }
    }
    // ===== SUB-MODO 2: LISTA PAGINADA =====
    else if (subMode == 2) {
        if (btn == ButtonHandler::DOUBLE_CLICK) {
            WiFi.scanDelete();
            subMode = 0;
            displayDeveloperInfo();
        }
        else if (btn == ButtonHandler::SHORT_PRESS && totalNetworks > 0) {
            selectedNetwork++;
            if (selectedNetwork >= totalNetworks) selectedNetwork = 0;
            if (selectedNetwork >= scrollOffset + 3) scrollOffset = selectedNetwork - 2;
            if (selectedNetwork < scrollOffset) scrollOffset = selectedNetwork;
            displayWiFiList(selectedNetwork, scrollOffset, totalNetworks);
        }
        else if (btn == ButtonHandler::VERY_LONG_PRESS_4S && totalNetworks > 0) {
            String ssid = WiFi.SSID(selectedNetwork);
            bool open = WiFi.encryptionType(selectedNetwork) == WIFI_AUTH_OPEN;
            selectedSSID = ssid;
            if (open || wifiManager.hasPasswordFor(ssid)) {
                bool started = wifiManager.startSelection(ssid, open);
                WiFi.scanDelete();
                subMode = started ? 4 : 5;
                displayWiFiConnection(ssid, started ? 0 : -1);
            } else {
                subMode = 3;
                wifiManager.createAP(appConfig.hostname);
                MDNS.begin(appConfig.hostname.c_str());
                devWeb->setWiFiSelection(ssid);
                WiFi.scanDelete();
                displayAPAlert(ssid);
            }
        }
    }
    // ===== SUB-MODO 3: ALERTA AP Y SERVIDOR LOCAL =====
    else if (subMode == 3) {
        if (btn == ButtonHandler::DOUBLE_CLICK) {
            WiFi.enableAP(false);
            WiFi.mode(WIFI_STA);

            subMode = 0;
            displayDeveloperInfo();

            if(WiFi.status() != WL_CONNECTED) {
                displayStateInfo("Aplicando red...");
                wifiManager.connect(0);
                displayDeveloperInfo();
            }
        }
    }
    else if (subMode == 4) {
        if (btn == ButtonHandler::DOUBLE_CLICK) {
            wifiManager.cancelSelection();
            wifiManager.connect(0);
            subMode = 0;
            displayDeveloperInfo();
        } else {
            WiFiManager::SelectionStatus result = wifiManager.pollSelection();
            if (result != WiFiManager::SELECTING) {
                subMode = 5;
                displayWiFiConnection(selectedSSID, result == WiFiManager::SELECTED ? 1 : -1);
            }
        }
    } else if (subMode == 5 && (btn == ButtonHandler::SHORT_PRESS || btn == ButtonHandler::DOUBLE_CLICK)) {
        subMode = 0;
        displayDeveloperInfo();
    }
}

void EstadoDESARROLLADOR::onExit() {
    wifiManager.cancelSelection();
    statemachine->flags.dev = false;
    primera_vez = true;
    WiFi.scanDelete();
    WiFi.enableAP(false);
    WiFi.mode(WIFI_STA);
    if (devWeb) { devWeb->end(); }
}

const char *EstadoDESARROLLADOR::getName() { return "DESARROLLADOR"; }
