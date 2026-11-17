#pragma once
#include <Arduino.h>
struct NetworkSettings {
    String serverUrl, tokenUrl, clientId, clientSecret, keyrockUser, keyrockPass, agentUrl;
    bool skipToken = false, useAgent = false;
};
