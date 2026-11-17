#pragma once
#include "Arduino.h"
#define UPDATE_SIZE_UNKNOWN 0
struct TestUpdate {
    bool running=false,error=false,allowEnd=true;
    bool begin(size_t){error=false;running=true;return true;}
    bool isRunning(){return running;}bool hasError(){return error;}
    size_t write(uint8_t*,size_t n){return n;}
    size_t progress(){return 32;}size_t size(){return 32;}
    bool end(bool){running=false;error=!allowEnd;return allowEnd;}
    void abort(){running=false;error=true;}
    void printError(TestSerial&){}
};
inline TestUpdate Update;
