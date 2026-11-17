#ifndef TOKENMANAGER_H
#define TOKENMANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "NetworkSettings.h"
#include "HttpTransport.h"

class TokenManager {
 private:
  String token;
  unsigned long tokenAcquiredMs = 0;
  unsigned long tokenTtlMs = 0;

  static String formEncode(const String& value) {
    String out;
    const char* hex = "0123456789ABCDEF";
    for (size_t i = 0; i < value.length(); ++i) {
      uint8_t c = value[i];
      if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') out += char(c);
      else { out += '%'; out += hex[c >> 4]; out += hex[c & 15]; }
    }
    return out;
  }

 public:
  bool requestToken(const NetworkSettings& config) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[TokenManager] Sin WiFi, no se puede pedir token");
      return false;
    }

    // 1. Blindaje de RAM: Evitar colapso si la memoria libre es muy baja
    uint32_t freeMem = ESP.getFreeHeap();
    if (freeMem < 25000) {
      Serial.printf("[TokenManager] Heap critico (%lu bytes). Postergando solicitud.\n", (unsigned long)freeMem);
      return false;
    }

    WiFiClient plain;
    WiFiClientSecure client;

    HTTPClient http;
    http.setTimeout(10000); // Timeout estricto de 10s para evitar cuelgues

    Serial.println("[TokenManager] Solicitando nuevo token a Keyrock...");
    Serial.printf("[TokenManager] URL: %s\n", config.tokenUrl.c_str());

    if (!beginHttp(http, plain, client, config.tokenUrl)) {
      Serial.println("[TokenManager] Error al iniciar conexion con Keyrock");
      client.stop();
      return false;
    }

    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String body = "grant_type=password";
    body += "&username=" + formEncode(config.keyrockUser);
    body += "&password=" + formEncode(config.keyrockPass);
    body += "&client_id=" + formEncode(config.clientId);
    body += "&client_secret=" + formEncode(config.clientSecret);

    int httpCode = http.POST(body);

    if (httpCode >= 200 && httpCode < 300) {
      String response = http.getString();
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, response);

      if (!error && doc["access_token"].is<String>() &&
          doc["access_token"].as<String>().length() > 0 &&
          (!doc["expires_in"].isNull() && doc["expires_in"].is<unsigned long>() &&
           doc["expires_in"].as<unsigned long>() > 0 &&
           doc["expires_in"].as<unsigned long>() <= 86400UL)) {
        token = doc["access_token"].as<String>();
        unsigned long expiresIn = doc["expires_in"].as<unsigned long>();

        // Renueva un 10% antes, con margen mÃ¡ximo de 600 s y sin underflow.
        unsigned long safetyMargin = min(600UL, expiresIn / 10);
        tokenTtlMs = (expiresIn - safetyMargin) * 1000UL;
        tokenAcquiredMs = millis();

        Serial.printf("[TokenManager] Token obtenido! TTL util: %lu s (Margen: %lu s)\n",
                      tokenTtlMs / 1000, safetyMargin);
        http.end();
        client.stop();
        return true;
      } else {
        Serial.println("[TokenManager] Error al parsear respuesta JSON");
      }
    } else {
      Serial.printf("[TokenManager] Error HTTP al pedir token: %d\n", httpCode);
    }

    http.end();
    client.stop();
    return false;
  }

  // Verifica validez evitando errores por desbordamiento de millis()
  bool hasToken() {
    if (token.length() == 0) return false;
    return (millis() - tokenAcquiredMs) < tokenTtlMs;
  }

  bool ensureValidToken(const NetworkSettings& config) {
    if (hasToken()) {
      return true;
    }
    Serial.println("[TokenManager] Token expirado o ausente. Solicitando renovacion...");
    return requestToken(config);
  }

  String getToken() { return token; }

  // Invalida el token inmediatamente ante un HTTP 401 / 403 / 302
  void clear() {
    token = "";
    tokenAcquiredMs = 0;
    tokenTtlMs = 0;
    Serial.println("[TokenManager] Token invalidado manualmente.");
  }
};

extern TokenManager tokenManager;

#endif
