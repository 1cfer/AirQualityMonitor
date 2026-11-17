#include "SensorManager.h"
#include "StateMachine.h"
#include <Wire.h>
extern StateMachine stateMachine;
// SEN0460 standard mass registers, same scale as the original firmware.
// Protocol: https://github.com/DFRobot/DFRobot_AirQualitySensor
static bool readConcentration(uint8_t reg, uint16_t& value) {
    Wire.beginTransmission(0x19); Wire.write(reg);
    if (Wire.endTransmission() != 0) return false;
    if (Wire.requestFrom(uint8_t(0x19), uint8_t(2)) != 2 || Wire.available() < 2) {
        while (Wire.available()) Wire.read();
        return false;
    }
    value = uint16_t(Wire.read()) << 8;
    value |= uint8_t(Wire.read());
    return value != UINT16_MAX;
}
void SensorManager::begin() {
    Wire.beginTransmission(0x19); airQualityOk = Wire.endTransmission() == 0;
}
void SensorManager::read() {
    SensorData sample;
    airQualityOk = readConcentration(0x05, sample.pm1_0) &&
                   readConcentration(0x07, sample.pm2_5) && readConcentration(0x09, sample.pm10);
    if (!airQualityOk) return; // Retry each interval, including after unplugging.
    stateMachine.sensors = sample; hasReading = true; lastValid = millis();
    accumulated.add(sample);
    pm25History[pm25HistoryIndex] = sample.pm2_5;
    pm25HistoryIndex = (pm25HistoryIndex + 1) % PM25_HISTORY_SIZE;
    if (pm25HistoryCount < PM25_HISTORY_SIZE) ++pm25HistoryCount;
}
