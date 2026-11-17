#pragma once
#include "Arduino.h"
#include <vector>
struct TwoWire {
    uint8_t reg=0; int error=0, bytes=2; uint16_t pm1=5,pm25=12,pm10=20;
    size_t pos=0; std::vector<uint8_t> incoming;
    void beginTransmission(uint8_t) {}
    void write(uint8_t r) { reg=r; }
    int endTransmission(bool=true) { return error; }
    uint8_t requestFrom(uint8_t,uint8_t) {
        pos=0; uint16_t n=reg==0x05?pm1:reg==0x07?pm25:pm10;
        incoming={uint8_t(n>>8),uint8_t(n)}; if(bytes<2)incoming.resize(bytes);return uint8_t(bytes);
    }
    int available() { return int(incoming.size()-pos); }
    int read() { return incoming.at(pos++); }
};
inline TwoWire Wire;
