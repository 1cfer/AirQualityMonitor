#include "SensorManager.h"

#include <DFRobot_AirQualitySensor.h>

#include "StateMachine.h"

extern DFRobot_AirQualitySensor particle;
extern StateMachine stateMachine;

void SensorManager::begin() {
  airQualityOk = particle.begin();
  if (!airQualityOk) {
    Serial.println("[SensorManager] Error sensor calidad de aire");
  } else {
    Serial.println("[SensorManager] Sensor calidad de aire OK");
  }
}

void SensorManager::read() {
  if (!airQualityOk) {
    Serial.println("[SensorManager] Sensor de aire no disponible, sin lectura");
    return;
  }

  stateMachine.sensors.pm1_0 = particle.gainParticleConcentration_ugm3(PARTICLE_PM1_0_STANDARD);
  stateMachine.sensors.pm2_5 = particle.gainParticleConcentration_ugm3(PARTICLE_PM2_5_STANDARD);
  stateMachine.sensors.pm10  = particle.gainParticleConcentration_ugm3(PARTICLE_PM10_STANDARD);

  // Acumulador del intervalo actual
  accPm1_0 += stateMachine.sensors.pm1_0;
  accPm2_5 += stateMachine.sensors.pm2_5;
  accPm10 += stateMachine.sensors.pm10;
  sampleCount++;

  if (stateMachine.sensors.pm2_5 < minPm2_5) minPm2_5 = stateMachine.sensors.pm2_5;
  if (stateMachine.sensors.pm2_5 > maxPm2_5) maxPm2_5 = stateMachine.sensors.pm2_5;

  // Historial para el gráfico de tendencia (independiente del envío)
  pm25History[pm25HistoryIndex] = stateMachine.sensors.pm2_5;
  pm25HistoryIndex = (pm25HistoryIndex + 1) % PM25_HISTORY_SIZE;
  if (pm25HistoryCount < PM25_HISTORY_SIZE) pm25HistoryCount++;

  Serial.printf("PM1.0: %u | PM2.5: %u | PM10: %u | Muestras: %d\n",
                stateMachine.sensors.pm1_0, stateMachine.sensors.pm2_5,
                stateMachine.sensors.pm10, sampleCount);
}

SensorData SensorManager::getAverages() {
  if (sampleCount == 0) {
    Serial.println("[SensorManager] WARN: sin muestras acumuladas, usando último valor conocido");
    return stateMachine.sensors;
  }

  SensorData avg;
  avg.pm1_0 = accPm1_0 / sampleCount;
  avg.pm2_5 = accPm2_5 / sampleCount;
  avg.pm10 = accPm10 / sampleCount;

  Serial.printf("[SensorManager] Promedio de %d muestras — PM1.0: %u | PM2.5: %u | PM10: %u\n",
                sampleCount, avg.pm1_0, avg.pm2_5, avg.pm10);
  return avg;
}

void SensorManager::resetAccumulator() {
  accPm1_0 = 0.0f;
  accPm2_5 = 0.0f;
  accPm10 = 0.0f;
  sampleCount = 0;
  minPm2_5 = UINT16_MAX;   // NUEVO: min/max también se resetean con el envío
  maxPm2_5 = 0;
  Serial.println("[SensorManager] Acumulador reseteado");
}