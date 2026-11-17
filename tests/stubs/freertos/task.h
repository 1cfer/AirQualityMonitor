#pragma once
#include "FreeRTOS.h"
#include <thread>
using TaskHandle_t = void*;
inline int xTaskCreate(void(*fn)(void*),const char*,int,void* arg,int,TaskHandle_t* task) {
    *task=reinterpret_cast<void*>(1);std::thread(fn,arg).detach();return pdPASS;
}
