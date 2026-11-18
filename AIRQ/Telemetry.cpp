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
static int sendRequest(const NetworkSettings& c, TokenManager& tokens,
                       const String& url, const String& payload, bool post) {
    WiFiClient plain; WiFiClientSecure secure; HTTPClient http;
    if (!beginHttp(http, plain, secure, url)) return -1003;
    http.addHeader("Content-Type", "application/json");
    if (!c.useAgent && !c.skipToken) http.addHeader("Authorization", "Bearer " + tokens.getToken());
    int code = post ? http.POST(payload) : http.PATCH(payload);
    http.end();
    if (!c.useAgent && (code == 401 || code == 403)) tokens.clear();
    return code;
}
static bool entityCreationTarget(const String& url, String& collection, String& id) {
    int start = url.indexOf("/v2/entities/");
    if (start < 0 || !url.endsWith("/attrs")) return false;
    id = url.substring(start + 13, url.length() - 6);
    if (!id.length() || id.length() > 128) return false;
    for (size_t i = 0; i < id.length(); ++i) {
        char c = id[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.' || c == ':')) return false;
    }
    collection = url.substring(0, start) + "/v2/entities";
    return true;
}
static int sendJob(const NetworkJob& job, TokenManager& tokens) {
    const auto& c = job.config;
    if (WiFi.status() != WL_CONNECTED) return -1001;
    if (!c.useAgent && !c.skipToken && !tokens.ensureValidToken(c)) return -1002;
    String payload = construirPayload(job.average.pm1_0, job.average.pm2_5, job.average.pm10);
    int code = sendRequest(c, tokens, c.useAgent ? c.agentUrl : c.serverUrl, payload, c.useAgent);
    // One normal PATCH. Only an actual 404 allows one creation POST using the
    // same real readings and identity. No GET probes or placeholder values.
    if (!c.useAgent && code == 404) {
        String collection, id;
        if (entityCreationTarget(c.serverUrl, collection, id)) {
            Serial.printf("[ENVIO] Entity missing; registering %s with measured PM values\n", id.c_str());
            String entity = construirEntidadAire(id, job.average.pm1_0, job.average.pm2_5, job.average.pm10);
            code = sendRequest(c, tokens, collection, entity, true);
        }
    }
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
