#pragma once
#include "Arduino.h"
#include "WiFiClient.h"
#include <atomic>
#include <thread>
#include <chrono>
#define HTTPC_DISABLE_FOLLOW_REDIRECTS 0
struct HTTPClient {
    static inline std::atomic<bool> block{false},entered{false};
    static inline bool lastTls=false;
    static inline String lastBody;
    static inline String response="{\"access_token\":\"test-token\",\"expires_in\":30}";
    static inline int patchCode=204;
    void setConnectTimeout(int) {}
    void setTimeout(int) {}
    void setFollowRedirects(int) {}
    bool begin(WiFiClient& c,const String&) { lastTls=c.tls;return true; }
    void addHeader(const char*,const char*) {}
    void addHeader(const char*,const String&) {}
    int PATCH(const String& body) { lastBody=body;entered=true;while(block)std::this_thread::sleep_for(std::chrono::milliseconds(1));return patchCode; }
    int POST(const String& body) { lastBody=body;return 200; }
    String getString() { return response; }
    void end() {}
};
