#pragma once
#include <stdint.h>

struct SensorData {
    uint16_t pm1_0 = 0, pm2_5 = 0, pm10 = 0;
};

// Exact integer sums; bounded rolling weight after long offline periods.
// At 65,536 samples the worst uint16_t sum still fits uint32_t.
struct SampleBatch {
    uint32_t pm1 = 0, pm25 = 0, pm10 = 0, count = 0;
    void halve() {
        uint32_t next = count / 2;
        if (!count) return;
        pm1 = uint32_t(uint64_t(pm1) * next / count);
        pm25 = uint32_t(uint64_t(pm25) * next / count);
        pm10 = uint32_t(uint64_t(pm10) * next / count);
        count = next;
    }
    void compact() {
        if (count >= 65536) halve();
    }
    void add(const SensorData& s) {
        compact(); pm1 += s.pm1_0; pm25 += s.pm2_5; pm10 += s.pm10; ++count;
    }
    void merge(SampleBatch other) {
        while (count + other.count > 65536) {
            halve(); other.halve();
        }
        pm1 += other.pm1; pm25 += other.pm25; pm10 += other.pm10; count += other.count;
    }
    SensorData average() const {
        SensorData s;
        if (count) { s.pm1_0 = uint16_t(pm1 / count); s.pm2_5 = uint16_t(pm25 / count); s.pm10 = uint16_t(pm10 / count); }
        return s;
    }
};

inline bool deadlineReached(uint32_t now, uint32_t deadline) {
    return int32_t(now - deadline) >= 0;
}
