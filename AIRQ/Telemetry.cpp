#include "Telemetry.h"
#include "AppConfig.h"
#include "SensorManager.h"
#include "StateMachine.h"
#include "NetworkSettings.h"
#include "TokenManager.h"
#include "PayloadBuilder.h"
#include "HttpTransport.h"
#include <new>
extern StateMachine stateMachine;
Telemetry telemetry;
struct NetworkJob { NetworkSettings config; SensorData average; };
struct NetworkResult { int code; };
bool Telemetry::begin() {
    requests = xQueueCreate(1, sizeof(NetworkJob*));
    results = xQueueCreate(1, sizeof(NetworkResult));
    if (requests && results && xTaskCreate(run, "air-network", 12288, this, 1, &task) == pdPASS) return true;
    if (requests) vQueueDelete(requests);
    if (results) vQueueDelete(results);
    requests = results = nullptr; task = nullptr; return false;
}
bool Telemetry::start() {
    if (!ready() || inFlight || !sensorManager.getSampleCount()) return false;
    NetworkJob* job = new (std::nothrow) NetworkJob;
    if (!job) return false;
    auto& c = job->config;
    c.serverUrl = appConfig.serverUrl; c.agentUrl = appConfig.agentUrl;
    c.tokenUrl = appConfig.tokenUrl; c.clientId = appConfig.clientId;
    c.clientSecret = appConfig.clientSecret; c.keyrockUser = appConfig.keyrockUser;
    c.keyrockPass = appConfig.keyrockPass; c.skipToken = appConfig.skipToken; c.useAgent = appConfig.useAgent;
    pending = sensorManager.takeBatch(); job->average = pending.average();
    if (xQueueSend(requests, &job, 0) != pdPASS) {
        sensorManager.restoreBatch(pending); pending = {}; delete job; return false;
    }
    inFlight = true; return true;
}
void Telemetry::poll() {
    NetworkResult result;
    if (!inFlight || xQueueReceive(results, &result, 0) != pdPASS) return;
    inFlight = false; lastCode = result.code;
    bool success = result.code >= 200 && result.code < 300;
    if (success) { ++sent; lastSuccess = millis(); }
    else { ++failed; sensorManager.restoreBatch(pending); }
    pending = {};
    stateMachine.clocks.proximo_envio = millis() + (success ? appConfig.intervaloEnvio : appConfig.intervaloReintento);
    stateMachine.needsUpdate = true;
    Serial.printf("[ENVIO] HTTP %d; OK %lu; fallos %lu\n", lastCode, (unsigned long)sent, (unsigned long)failed);
}
static int sendJob(const NetworkJob& job, TokenManager& tokens) {
    const auto& c = job.config;
    if (WiFi.status() != WL_CONNECTED) return -1001;
    if (!c.useAgent && !c.skipToken && !tokens.ensureValidToken(c)) return -1002;
    WiFiClient plain; WiFiClientSecure secure; HTTPClient http;
    if (!beginHttp(http, plain, secure, c.useAgent ? c.agentUrl : c.serverUrl)) return -1003;
    http.addHeader("Content-Type", "application/json");
    if (!c.useAgent && !c.skipToken) http.addHeader("Authorization", "Bearer " + tokens.getToken());
    String payload = construirPayload(job.average.pm1_0, job.average.pm2_5, job.average.pm10);
    int code = c.useAgent ? http.POST(payload) : http.PATCH(payload);
    http.end();
    if (!c.useAgent && (code == 401 || code == 403)) tokens.clear();
    return code;
}
void Telemetry::run(void* context) {
    auto* self = static_cast<Telemetry*>(context);
    TokenManager tokens; NetworkSettings previous;
    for (;;) {
        NetworkJob* job = nullptr;
        if (xQueueReceive(self->requests, &job, portMAX_DELAY) != pdPASS) continue;
        const auto& c = job->config;
        if (c.tokenUrl != previous.tokenUrl || c.clientId != previous.clientId ||
            c.clientSecret != previous.clientSecret || c.keyrockUser != previous.keyrockUser ||
            c.keyrockPass != previous.keyrockPass) tokens.clear();
        previous = c;
        NetworkResult result { sendJob(*job, tokens) };
        delete job;
        xQueueSend(self->results, &result, portMAX_DELAY);
    }
}
