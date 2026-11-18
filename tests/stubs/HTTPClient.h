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
    static inline String lastPostBody, lastPostUrl, lastPostAuth;
    static inline std::atomic<unsigned> posts{0}, entityPosts{0};
    String url, auth;
    static inline String response="{\"access_token\":\"test-token\",\"expires_in\":30}";
    static inline int patchCode=204;
    static inline int postCode=201;
    void setConnectTimeout(int) {}
    void setTimeout(int) {}
    void setFollowRedirects(int) {}
    bool begin(WiFiClient& c,const String& target) { url=target;lastTls=c.tls;return true; }
    void addHeader(const char* name,const char* value) { addHeader(name,String(value)); }
    void addHeader(const char* name,const String& value) { if(String(name)=="Authorization")auth=value; }
    int PATCH(const String& body) { lastBody=body;entered=true;while(block)std::this_thread::sleep_for(std::chrono::milliseconds(1));return patchCode; }
    int POST(const String& body) { lastBody=body;lastPostBody=body;lastPostUrl=url;lastPostAuth=auth;++posts;if(url.endsWith("/v2/entities"))++entityPosts;return postCode; }
    String getString() { return response; }
    void end() {}
};
