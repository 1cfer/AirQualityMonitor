#pragma once
#include <Arduino.h>
#include "Measurement.h"
class SensorManager {
    bool airQualityOk = false, hasReading = false;
    unsigned long lastValid = 0;
    SampleBatch accumulated;
    static const int PM25_HISTORY_SIZE = 60;
    uint16_t pm25History[PM25_HISTORY_SIZE] = {};
    int pm25HistoryIndex = 0, pm25HistoryCount = 0;
public:
    void begin();
    void read();
    bool isAirQualityOk() const { return airQualityOk; }
    bool hasValidReading() const { return hasReading; }
    unsigned long ageMs() const { return millis() - lastValid; }
    int getSampleCount() const { return accumulated.count; }
    SensorData getAverages() const { return accumulated.average(); }
    SampleBatch takeBatch() { SampleBatch b = accumulated; accumulated = {}; return b; }
    void restoreBatch(const SampleBatch& b) { accumulated.merge(b); }
    const uint16_t* getPm25History() const { return pm25History; }
    int getPm25HistoryIndex() const { return pm25HistoryIndex; }
    int getPm25HistoryCount() const { return pm25HistoryCount; }
    static int getPm25HistorySize() { return PM25_HISTORY_SIZE; }
};
extern SensorManager sensorManager;
