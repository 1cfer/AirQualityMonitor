#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>

class WiFiManager {
private:
    Preferences prefs;
    const char *apPass = "12345678";
    unsigned long lastReconnectAttempt = 0;
    static const unsigned long RECONNECT_INTERVAL = 30000;
    bool selecting = false;
    unsigned long selectionStarted = 0;
    String selectionSSID, selectionPass;

    // Busca si una red está en el llavero de 3 slots
    int findSlot(String targetSSID) {
        if (targetSSID.length() == 0) return -1;
        for (int i = 0; i < 3; i++) {
            String k = "s" + String(i);
            if (prefs.getString(k.c_str(), "") == targetSSID) return i;
        }
        return -1;
    }

public:
    enum SelectionStatus { SELECTING, SELECTED, SELECTION_FAILED };

    bool hasPasswordFor(const String& ssid) {
        prefs.begin("agevital", true);
        int slot = findSlot(ssid);
        bool saved = slot >= 0 && prefs.getString(("p" + String(slot)).c_str(), "").length() > 0;
        prefs.end();
        return saved;
    }

    bool startSelection(const String& ssid, bool open) {
        if (!ssid.length() || ssid.length() > 32) return false;
        selectionPass = "";
        if (!open) {
            prefs.begin("agevital", true);
            int slot = findSlot(ssid);
            if (slot >= 0) selectionPass = prefs.getString(("p" + String(slot)).c_str(), "");
            prefs.end();
            if (!selectionPass.length()) return false;
        }
        selectionSSID = ssid;
        selecting = true;
        selectionStarted = millis();
        WiFi.persistent(false);
        WiFi.mode((WiFi.getMode() & WIFI_AP) ? WIFI_AP_STA : WIFI_STA);
        WiFi.setAutoReconnect(true);
        WiFi.disconnect(false, false);
        WiFi.begin(selectionSSID.c_str(), selectionPass.c_str());
        lastReconnectAttempt = millis();
        return true;
    }

    SelectionStatus pollSelection() {
        if (!selecting) return SELECTION_FAILED;
        if (isConnected() && WiFi.SSID() == selectionSSID) {
            selecting = false;
            saveCredentials(selectionSSID, selectionPass);
            selectionPass = "";
            return SELECTED;
        }
        if (millis() - selectionStarted < 12000UL) return SELECTING;
        cancelSelection();
        connect(0); // Recupera la última red exitosa; no guarda la que falló.
        return SELECTION_FAILED;
    }

    void cancelSelection() {
        if (selecting) WiFi.disconnect(false, false);
        selecting = false;
        selectionPass = "";
    }

    // Auto-conexión general al arrancar (Usa la última exitosa)
    void migrateLegacyCredentials() {
        prefs.begin("agevital", false);
        String legacy = prefs.getString("ssid", "");
        if (legacy.length() && !prefs.getString("lastSSID", "").length()) {
            String pass = prefs.getString("pass", "");
            prefs.putString("lastSSID", legacy); prefs.putString("lastPASS", pass);
            prefs.putString("s0", legacy); prefs.putString("p0", pass);
        }
        prefs.end();
    }
    bool connect(int maxAttempts = 16) {
        migrateLegacyCredentials();
        if (isConnected()) return true;
        WiFi.persistent(false);
        WiFi.setAutoReconnect(true);
        WiFi.mode((WiFi.getMode() & WIFI_AP) ? WIFI_AP_STA : WIFI_STA);
        prefs.begin("agevital", true);
        String ssid = prefs.getString("lastSSID", prefs.getString("ssid", "").c_str());
        String pass = prefs.getString("lastPASS", prefs.getString("pass", "").c_str());
        prefs.end();

        if (ssid.length() == 0) return false;

        Serial.printf("[WiFiManager] Autoconectando a %s...\n", ssid.c_str());
        WiFi.begin(ssid.c_str(), pass.c_str());
        lastReconnectAttempt = millis();

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
            delay(500);
            Serial.print(".");
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) return true;
        return false;
    }

    // Conectar a una red ESPECÍFICA desde la lista OLED
    bool connectTo(String targetSSID, int maxAttempts = 16) {
        prefs.begin("agevital", true);
        int slot = findSlot(targetSSID);
        if (slot == -1) { prefs.end(); return false; }
        String pass = prefs.getString(("p" + String(slot)).c_str(), "");
        prefs.end();

        Serial.printf("[WiFiManager] Conectando a %s...\n", targetSSID.c_str());
        WiFi.mode((WiFi.getMode() & WIFI_AP) ? WIFI_AP_STA : WIFI_STA);
        WiFi.setAutoReconnect(true);
        WiFi.begin(targetSSID.c_str(), pass.c_str());
        lastReconnectAttempt = millis();

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
            delay(500);
            Serial.print(".");
            attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            // Si tiene éxito, la marcamos como red prioritaria
            prefs.begin("agevital", false);
            prefs.putString("lastSSID", targetSSID);
            prefs.putString("lastPASS", pass);
            prefs.end();
            return true;
        }
        return false;
    }

    void createAP(const String &hostname) {
        String apSSID = ("TARS-" + hostname).substring(0, 32);
        WiFi.mode(WIFI_AP_STA);
        if (!WiFi.softAP(apSSID.c_str(), apPass)) Serial.println("[WiFi] Error al crear AP");
    }

    void maintainConnection() {
        if (selecting) return;
        if (WiFi.status() == WL_CONNECTED) return;
        unsigned long now = millis();
        if (now - lastReconnectAttempt >= RECONNECT_INTERVAL) {
            lastReconnectAttempt = now;
            prefs.begin("agevital", true);
            if (prefs.getString("lastSSID", prefs.getString("ssid", "").c_str()).length() > 0) {
                String ssid = prefs.getString("lastSSID", prefs.getString("ssid", "").c_str());
                String pass = prefs.getString("lastPASS", prefs.getString("pass", "").c_str());
                WiFi.begin(ssid.c_str(), pass.c_str());
            }
            prefs.end();
        }
    }

    // Guarda múltiples redes sin borrar las anteriores
    void saveCredentials(const String &newSSID, const String &newPass) {
        if (newSSID.length() == 0 || newSSID.length() > 32 || newPass.length() > 63) return;
        prefs.begin("agevital", false);
        prefs.putString("lastSSID", newSSID);
        prefs.putString("lastPASS", newPass);

        bool found = false;
        int freeSlot = 0;
        for(int i=0; i<3; i++) {
            String s = prefs.getString(("s"+String(i)).c_str(), "");
            if(s == newSSID) { found = true; prefs.putString(("p"+String(i)).c_str(), newPass); break; }
            if(s == "") freeSlot = i;
        }
        if(!found) {
            prefs.putString(("s"+String(freeSlot)).c_str(), newSSID);
            prefs.putString(("p"+String(freeSlot)).c_str(), newPass);
        }
        prefs.end();
    }

    void reset() {
        prefs.begin("agevital", false);
        prefs.clear();
        prefs.end();
    }

    // Averigua si el llavero tiene la clave para dibujar el Asterisco (*)
    bool hasCredentialsFor(String targetSSID) {
        prefs.begin("agevital", true);
        int slot = findSlot(targetSSID);
        prefs.end();
        return slot != -1;
    }

    bool isConnected() { return WiFi.status() == WL_CONNECTED; }
    String getIP() {
        if (WiFi.status() == WL_CONNECTED) return WiFi.localIP().toString();
        if (WiFi.getMode() & WIFI_AP) return WiFi.softAPIP().toString();
        return "No conectado";
    }
    String getSSID() {
        if (WiFi.status() == WL_CONNECTED) return WiFi.SSID();
        return "";
    }
};

extern WiFiManager wifiManager;
#endif
