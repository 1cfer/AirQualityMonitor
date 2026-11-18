#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <Arduino.h>
#include <Preferences.h>

class AppConfig {
 private:
  Preferences prefs;

 public:
  // ===== CONFIGURACION DE ENVIO DE DATOS (ORION) =====
  String serverUrl = "https://api.moreha.com.co/v2/entities/airq1/attrs";
  String hostname = "tars-new";  // Default genÃ©rico, se configura en primer boot
  unsigned long intervaloEnvio = 15000;
  unsigned long intervaloLectura = 2000;
  unsigned long intervaloReintento = 20000;
  unsigned long tiempoInactividad = 180000;

  // ===== CREDENCIALES KEYROCK =====
  String tokenUrl = "https://idm.moreha.com.co/oauth2/token";
  String clientId = "3b3153a0-6577-461a-be41-4113f500b896";
  String clientSecret = "";
  String keyrockUser = "tars@moreha.com";
  String keyrockPass = "";
  bool skipToken = false;

  // ===== CONFIGURACION AGENTE =====
  String agentUrl = "http://10.38.35.216:5000/v1/agent/tars1";
  bool useAgent = false;

  // ===== PRIMER BOOT =====
  bool isConfigured = false;  // false = primer boot, forzar setup desde OTA

  void begin() {
    prefs.begin("appconfig", false);
    isConfigured = prefs.getBool("isConfigured", isConfigured);
    serverUrl = prefs.getString("serverUrl", serverUrl.c_str());
    hostname = prefs.getString("hostname", hostname.c_str());
    intervaloEnvio = prefs.getULong("intervaloEnvio", intervaloEnvio);
    intervaloLectura = prefs.getULong("readMs", intervaloLectura);
    intervaloReintento = prefs.getULong("retryMs", intervaloReintento);
    tiempoInactividad = prefs.getULong("screenMs", tiempoInactividad);

    tokenUrl = prefs.getString("tokenUrl", tokenUrl.c_str());
    clientId = prefs.getString("clientId", clientId.c_str());
    clientSecret = prefs.getString("clientSecret", clientSecret.c_str());
    keyrockUser = prefs.getString("keyrockUser", keyrockUser.c_str());
    keyrockPass = prefs.getString("keyrockPass", keyrockPass.c_str());
    skipToken = prefs.getBool("skipToken", skipToken);

    agentUrl = prefs.getString("agentUrl", agentUrl.c_str());
    useAgent = prefs.getBool("useAgent", useAgent);
    // Migrate the destination once, preserving saved credentials and WiFi.
    // A later URL explicitly saved by the owner remains authoritative.
    if (!prefs.getBool("morehaCloud", false)) {
      serverUrl = "https://api.moreha.com.co/v2/entities/airq1/attrs";
      tokenUrl = "https://idm.moreha.com.co/oauth2/token";
      skipToken = false;
      useAgent = false;
      prefs.putString("serverUrl", serverUrl);
      prefs.putString("tokenUrl", tokenUrl);
      prefs.putBool("skipToken", false);
      prefs.putBool("useAgent", false);
      prefs.putBool("morehaCloud", true);
      Serial.println("[Config] MOREHA: airq1, Keyrock on, direct delivery");
    }
    prefs.end();
    intervaloEnvio = constrain(intervaloEnvio, 10000UL, 86400000UL);
    intervaloLectura = constrain(intervaloLectura, 500UL, 60000UL);
    intervaloReintento = constrain(intervaloReintento, 10000UL, 86400000UL);
    tiempoInactividad = constrain(tiempoInactividad, 10000UL, 86400000UL);

    Serial.println("[AppConfig] Configuracion cargada:");
    Serial.printf("  isConfigured: %s\n", isConfigured ? "true" : "false");
    Serial.printf("  hostname: %s\n", hostname.c_str());
    Serial.printf("  serverUrl: %s\n", serverUrl.c_str());
    Serial.printf("  intervaloEnvio: %lu ms\n", intervaloEnvio);
    Serial.printf("  intervaloLectura: %lu ms\n", intervaloLectura);
    Serial.printf("  agentUrl: %s\n", agentUrl.c_str());
    Serial.printf("  useAgent: %s\n", useAgent ? "true" : "false");
    Serial.printf("  skipToken: %s\n", skipToken ? "true" : "false");
  }

  void save() {
    prefs.begin("appconfig", false);
    prefs.putBool("morehaCloud", true);
    prefs.putBool("isConfigured", true);  // Si se guarda config, ya estÃ¡ configurado
    prefs.putString("serverUrl", serverUrl.c_str());
    prefs.putString("hostname", hostname.c_str());
    prefs.putULong("intervaloEnvio", intervaloEnvio);
    prefs.putULong("readMs", intervaloLectura);
    prefs.putULong("retryMs", intervaloReintento);
    prefs.putULong("screenMs", tiempoInactividad);

    prefs.putString("tokenUrl", tokenUrl.c_str());
    prefs.putString("clientId", clientId.c_str());
    prefs.putString("clientSecret", clientSecret.c_str());
    prefs.putString("keyrockUser", keyrockUser.c_str());
    prefs.putString("keyrockPass", keyrockPass.c_str());
    prefs.putBool("skipToken", skipToken);

    prefs.putString("agentUrl", agentUrl.c_str());
    prefs.putBool("useAgent", useAgent);
    prefs.end();
    isConfigured = true;
    Serial.println("[AppConfig] Configuracion guardada en NVS");
  }

  void reset() {
    prefs.begin("appconfig", false);
    prefs.clear();
    prefs.end();
    serverUrl = "https://api.moreha.com.co/v2/entities/airq1/attrs";
    hostname = "tars-new";
    intervaloEnvio = 15000;
    intervaloLectura = 2000;
    intervaloReintento = 20000;
    tiempoInactividad = 180000;

    tokenUrl = "https://idm.moreha.com.co/oauth2/token";
    clientId = "3b3153a0-6577-461a-be41-4113f500b896";
    clientSecret = "";
    keyrockUser = "tars@moreha.com";
    keyrockPass = "";
    skipToken = false;

    agentUrl = "http://10.38.35.216:5000/v1/agent/tars1";
    useAgent = false;
    isConfigured = false;
    Serial.println("[AppConfig] Configuracion restaurada a defaults");
  }
};

extern AppConfig appConfig;

#endif
