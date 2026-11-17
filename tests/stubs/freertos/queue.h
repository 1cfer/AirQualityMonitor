#pragma once
#include "FreeRTOS.h"
#include <mutex>
#include <condition_variable>
#include <deque>
#include <vector>
#include <cstring>
struct TestQueue { size_t capacity, size; std::mutex mutex; std::condition_variable cv; std::deque<std::vector<uint8_t>> items; };
using QueueHandle_t = TestQueue*;
inline QueueHandle_t xQueueCreate(size_t n,size_t size) { auto q=new TestQueue;q->capacity=n;q->size=size;return q; }
inline void vQueueDelete(QueueHandle_t q) { delete q; }
inline int xQueueSend(QueueHandle_t q,const void* data,uint32_t wait) {
    std::unique_lock<std::mutex> lock(q->mutex);
    if(wait==portMAX_DELAY)q->cv.wait(lock,[&]{return q->items.size()<q->capacity;});
    if(q->items.size()>=q->capacity)return 0;
    q->items.emplace_back(q->size);std::memcpy(q->items.back().data(),data,q->size);q->cv.notify_all();return pdPASS;
}
inline int xQueueReceive(QueueHandle_t q,void* data,uint32_t wait) {
    std::unique_lock<std::mutex> lock(q->mutex);
    if(wait==portMAX_DELAY)q->cv.wait(lock,[&]{return !q->items.empty();});
    if(q->items.empty())return 0;
    std::memcpy(data,q->items.front().data(),q->size);q->items.pop_front();q->cv.notify_all();return pdPASS;
}
