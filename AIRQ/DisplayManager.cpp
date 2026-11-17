#include "DisplayManager.h"
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <math.h>
#include "AppConfig.h"
#include "FirmwareInfo.h"
#include "StateMachine.h"
#include "WiFiManager.h"
#include "SensorManager.h"
#include "Telemetry.h"

extern Adafruit_SSD1306 display;
extern StateMachine stateMachine;
bool displayReady = false;

static String clipped(String text, unsigned limit = 21) {
    if (text.length() > limit) text = text.substring(0, limit - 1) + "~";
    return text;
}

static void line(int y, const String& text) {
    display.setTextSize(1);
    display.setCursor(0, y);
    display.print(clipped(text));
}

static void header(const char* title, int page = -1) {
    display.clearDisplay();
    display.setTextWrap(false);
    display.fillRect(0, 0, 128, 11, SSD1306_WHITE);
    display.setTextSize(1);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(2, 2);
    display.print(clipped(title, 13));
    display.setCursor(86, 2);
    if (page >= 0) {
        String status = WiFi.status() == WL_CONNECTED ? "W " : "- ";
        status += telemetry.busy() ? "TX" : telemetry.lastCode >= 200 && telemetry.lastCode < 300 ? "OK" : telemetry.lastCode ? "!!" : "--";
        status += " " + String(page + 1);
        display.print(status);
    } else display.print(WiFi.status() == WL_CONNECTED ? "WiFi" : "--");
    display.setTextColor(SSD1306_WHITE);
}

void setDisplayPower(bool enabled) {
    if (!displayReady) return;
    display.ssd1306_command(enabled ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
}

void identifyDisplay(bool enabled) {
    if (displayReady) display.invertDisplay(enabled);
}

void displaySplash() {
    if (!displayReady) return;
    display.clearDisplay();
    display.setTextWrap(false);
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(3);
    display.setCursor(28, 2);
    display.print("TARS");
    line(33, appConfig.hostname);
    line(50, String("FW ") + TARS_FIRMWARE_VERSION);
    display.display();
}

static bool fresh() {
    return sensorManager.hasValidReading() && sensorManager.isAirQualityOk() &&
        sensorManager.ageMs() <= max(10000UL, appConfig.intervaloLectura * 3);
}
static String pm(uint16_t value) { return fresh() ? String(value) : "--"; }
static void footer() {
    if (!sensorManager.hasValidReading()) line(56, "Sin lectura valida");
    else if (!fresh()) line(56, "Sin sensor - " + String(sensorManager.ageMs() / 1000) + "s");
    else if (stateMachine.flags.dev) line(56, "Abriendo panel...");
    else line(56, "Hace " + String(sensorManager.ageMs() / 1000) + "s  Clic:pagina");
}
static uint16_t historyValue(int i) {
    int count = sensorManager.getPm25HistoryCount();
    int size = sensorManager.getPm25HistorySize();
    int start = (sensorManager.getPm25HistoryIndex() + size - count) % size;
    return sensorManager.getPm25History()[(start + i) % size];
}
void updateDisplay() {
    if (!displayReady || !stateMachine.isDisplayOn) return;
    auto s = stateMachine.sensors;
    int page = stateMachine.screenMode;
    switch (page) {
    case 0:
        header("PM2.5", page);
        display.setTextSize(3); display.setCursor(0, 19); display.print(pm(s.pm2_5));
        display.setTextSize(1); display.setCursor(96, 33); display.print("ug/m3");
        line(46, "PM1 " + pm(s.pm1_0) + "  PM10 " + pm(s.pm10));
        footer(); break;
    case 1: {
        header("PM ug/m3", page);
        const char* names[] = {"PM1.0", "PM2.5", "PM10"};
        uint16_t values[] = {s.pm1_0, s.pm2_5, s.pm10};
        for (int i = 0; i < 3; ++i) {
            line(23 + 16 * i, names[i]);
            display.setTextSize(2); display.setCursor(68, 16 + 16 * i); display.print(pm(values[i]));
        }
        break;
    }
    case 2: {
        header("TENDENCIA", page);
        int count = sensorManager.getPm25HistoryCount();
        if (count < 2) { line(24, "Reuniendo muestras"); footer(); break; }
        uint16_t peak = 1;
        for (int i = 0; i < count; ++i) peak = max(peak, historyValue(i));
        line(18, "PM2.5  max " + String(peak));
        display.drawRect(0, 29, 128, 24, SSD1306_WHITE);
        int px = 1, py = 51 - uint32_t(historyValue(0)) * 21 / peak;
        for (int i = 1; i < count; ++i) {
            int x = 1 + i * 125 / (count - 1);
            int y = 51 - uint32_t(historyValue(i)) * 21 / peak;
            display.drawLine(px, py, x, y, SSD1306_WHITE); px = x; py = y;
        }
        line(56, String(count) + " lecturas  ug/m3"); break;
    }
    case 3: {
        header("RESUMEN PM2.5", page);
        int count = sensorManager.getPm25HistoryCount();
        if (!count) { line(24, "Sin muestras"); footer(); break; }
        uint16_t lo = UINT16_MAX, hi = 0; uint32_t sum = 0;
        for (int i = 0; i < count; ++i) {
            uint16_t v = historyValue(i); lo = min(lo, v); hi = max(hi, v); sum += v;
        }
        line(18, "Min " + String(lo) + "  Max " + String(hi));
        line(30, "Media " + String(sum / count) + " ug/m3");
        line(42, "Ultimas " + String(count) + " lecturas");
        footer(); break;
    }
    default:
        header("RED / ENVIO", 4);
        line(18, WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "Sin WiFi");
        line(30, "IP:" + wifiManager.getIP());
        line(42, "OK:" + String(telemetry.sent) + " Error:" + String(telemetry.failed));
        if (telemetry.busy()) line(56, "Enviando en fondo");
        else if (!telemetry.ready()) line(56, "Sin tarea de red");
        else if (!telemetry.lastCode) line(56, "Sin envios aun");
        else if (telemetry.lastCode == -1002) line(56, "Fallo autenticacion");
        else if (telemetry.lastCode == -1003) line(56, "URL no valida");
        else line(56, "HTTP:" + String(telemetry.lastCode));
        break;
    }
    display.display();
}

void displayStateInfo(const char* estado) {
    if (!displayReady) return;
    header("ESTADO");
    line(22, estado);
    line(44, "IP: " + wifiManager.getIP());
    display.display();
}

void displayDeveloperInfo() {
    if (!displayReady) return;
    header("CONFIGURACION");
    line(14, appConfig.hostname);
    bool apActive = (WiFi.getMode() & WIFI_AP) != 0;
    line(24, apActive ? "AP:" + WiFi.softAPIP().toString() : "IP:" + wifiManager.getIP());
    if (apActive) line(34, WiFi.softAPSSID());
    else if (WiFi.status() == WL_CONNECTED) line(34, "Red:" + WiFi.SSID());
    else line(34, "Sin conexion WiFi");
    line(44, "Clic: buscar redes");
    line(54, "4s: volver a medir");
    display.display();
}

void displayWiFiList(int selected, int offset, int total) {
    if (!displayReady) return;
    header("REDES WIFI");
    if (total <= 0) {
        line(24, "No se hallaron redes");
        line(48, "Doble clic: volver");
    }
    for (int i = 0; i < 3 && i + offset < total; ++i) {
        int index = i + offset;
        String ssid = WiFi.SSID(index);
        bool locked = WiFi.encryptionType(index) != WIFI_AUTH_OPEN;
        bool saved = locked ? wifiManager.hasPasswordFor(ssid) : wifiManager.hasCredentialsFor(ssid);
        String row = index == selected ? ">" : " ";
        row += locked ? "C" : "A";
        row += saved ? "* " : "  ";
        line(14 + i * 10, row + clipped(ssid, 17));
    }
    if (total > 0) {
        line(46, "A:libre C:clave *=G");
        line(56, "4s:elegir 2x:volver");
    }
    display.display();
}

void displayAPAlert(const String& ssid) {
    if (!displayReady) return;
    header("RED CON CLAVE");
    line(14, ssid.length() ? ssid : "Configurar WiFi");
    line(24, "Clave no guardada");
    line(34, "AP:" + WiFi.softAPSSID());
    line(44, "AP clave:12345678");
    line(54, "192.168.4.1");
    display.display();
}

void displayWiFiConnection(const String& ssid, int status) {
    if (!displayReady) return;
    header(status == 0 ? "CONECTANDO" : "RED WIFI");
    line(16, ssid);
    line(28, status == 0 ? "Espere hasta 12 s" : status > 0 ? "Conectado" : "No se pudo conectar");
    if (status > 0) line(40, "IP:" + wifiManager.getIP());
    else if (status < 0) line(40, "Revise clave / senal");
    line(54, status == 0 ? "2x:cancelar" : "Clic:volver 4s:medir");
    display.display();
}
