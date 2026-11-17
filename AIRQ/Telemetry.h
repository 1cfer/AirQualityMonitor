#pragma once
#include <Arduino.h>
#include "Measurement.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
// Only loop() owns sensors, OLED, config and public status. Immutable jobs/results
// cross queues; the worker never touches I2C, NVS, UI or application states.
class Telemetry {
    QueueHandle_t requests = nullptr, results = nullptr;
    TaskHandle_t task = nullptr;
    SampleBatch pending;
    bool inFlight = false;
    static void run(void* context);
public:
    int lastCode = 0;
    uint32_t sent = 0, failed = 0, lastSuccess = 0;
    bool begin();
    bool start();
    void poll();
    bool busy() const { return inFlight; }
    bool ready() const { return task != nullptr; }
};
extern Telemetry telemetry;
