#ifndef SENSORMANAGER_H
#define SENSORMANAGER_H

#include <Arduino.h>

#include "StateMachine.h"

class SensorManager {
 private:
  bool airQualityOk = false;

  // Acumulador del intervalo actual (se resetea con cada envío)
  float accPm1_0 = 0.0f;
  float accPm2_5 = 0.0f;
  float accPm10 = 0.0f;
  int sampleCount = 0;
  uint16_t minPm2_5 = UINT16_MAX;
  uint16_t maxPm2_5 = 0;

  // Historial para gráfico de tendencia (NO se resetea con el envío)
  static const int PM25_HISTORY_SIZE = 20;
  uint16_t pm25History[PM25_HISTORY_SIZE] = {0};
  int pm25HistoryIndex = 0;
  int pm25HistoryCount = 0;

 public:
  void begin();
  void read();
  SensorData getAverages();
  void resetAccumulator();
  int getSampleCount() const { return sampleCount; }
  bool isAirQualityOk() const { return airQualityOk; }

  uint16_t getMinPm2_5() const { return minPm2_5 == UINT16_MAX ? 0 : minPm2_5; }
  uint16_t getMaxPm2_5() const { return maxPm2_5; }

  const uint16_t* getPm25History() const { return pm25History; }
  int getPm25HistoryIndex() const { return pm25HistoryIndex; }
  int getPm25HistoryCount() const { return pm25HistoryCount; }
  static int getPm25HistorySize() { return PM25_HISTORY_SIZE; }
};

extern SensorManager sensorManager;

#endif