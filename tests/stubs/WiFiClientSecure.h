#pragma once
#include "WiFiClient.h"
struct WiFiClientSecure : WiFiClient {
    WiFiClientSecure() { tls=true; }
    void setHandshakeTimeout(int) {}
    void setInsecure() {}
};
