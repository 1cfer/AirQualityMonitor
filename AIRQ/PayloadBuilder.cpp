#include "PayloadBuilder.h"

#include <ArduinoJson.h>

String construirPayload(uint16_t pm1_0, uint16_t pm2_5, uint16_t pm10) {
  JsonDocument doc;

  JsonObject pm1 = doc["pm1_0"].to<JsonObject>();
  pm1["type"] = "Number";
  pm1["value"] = pm1_0;

  JsonObject pm25 = doc["pm2_5"].to<JsonObject>();
  pm25["type"] = "Number";
  pm25["value"] = pm2_5;

  JsonObject pm10obj = doc["pm10"].to<JsonObject>();
  pm10obj["type"] = "Number";
  pm10obj["value"] = pm10;

  String payload;
  serializeJson(doc, payload);
  return payload;
}

String construirEntidadAire(const String& id, uint16_t pm1_0, uint16_t pm2_5, uint16_t pm10) {
  JsonDocument doc;
  doc["id"] = id;
  doc["type"] = "AirQualityObserved";
  const char* names[] = {"pm1_0", "pm2_5", "pm10"};
  const uint16_t values[] = {pm1_0, pm2_5, pm10};
  for (unsigned i = 0; i < 3; ++i) {
    doc[names[i]]["type"] = "Number";
    doc[names[i]]["value"] = values[i];
  }
  String payload;
  serializeJson(doc, payload);
  return payload;
}
