#include "DevWebOTA.h"

#include <ESPmDNS.h>

#include "AppConfig.h"
#include "TokenManager.h"
#include "WiFiManager.h"
#include "DisplayManager.h"
#include "FirmwareInfo.h"
#include "Telemetry.h"
#include "SensorManager.h"
#include "StateMachine.h"
#include <ArduinoJson.h>
extern StateMachine stateMachine;

static bool validUrl(const String& url) {
  return (url.startsWith("http://") || url.startsWith("https://")) &&
         url.length() <= 512 && url.indexOf(' ') < 0 && url.indexOf('\r') < 0 &&
         url.indexOf('\n') < 0 && url.length() > (url.startsWith("https://") ? 8U : 7U);
}

static String htmlAttribute(String text) {
  text.replace("&", "&amp;"); text.replace("\"", "&quot;");
  text.replace("<", "&lt;"); text.replace(">", "&gt;");
  return text;
}

static bool validHostname(const String& host) {
  if (!host.length() || host.length() > 32 || host[0] == '-' || host[host.length()-1] == '-') return false;
  for (size_t i = 0; i < host.length(); ++i) {
    char c = host[i];
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '-')) return false;
  }
  return true;
}

static bool validInterval(const String& text, unsigned long low, unsigned long high) {
  if (!text.length()) return true; // Campo vacío: conservar valor.
  unsigned long value = 0;
  for (size_t i = 0; i < text.length(); ++i) {
    if (text[i] < '0' || text[i] > '9') return false;
    unsigned digit = text[i] - '0';
    if (value > (high - digit) / 10) return false;
    value = value * 10 + digit;
  }
  return value >= low && value <= high;
}

const char HTML_PAGE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<meta http-equiv="Cache-Control" content="no-cache, no-store, must-revalidate">
<meta http-equiv="Pragma" content="no-cache">
<meta http-equiv="Expires" content="0">
<title>AgeVital TARS - %HOSTNAME%</title>
<style>
body{font-family:Arial;margin:20px auto;padding:0 16px;max-width:720px;background:#edf2f4;color:#182c32}
.card{background:white;padding:20px;margin:10px 0;border-radius:10px;box-shadow:0 2px 5px rgba(0,0,0,0.1)}
h1{color:#087f8c;text-align:center}
h2{color:#333;border-bottom:2px solid #087f8c;padding-bottom:10px}
label{display:block;margin-top:10px;font-weight:bold;color:#555;font-size:14px}
input,button{width:100%;padding:12px;margin:5px 0;border:1px solid #ddd;border-radius:5px;box-sizing:border-box}
button{background:#087f8c;color:white;border:none;cursor:pointer;font-weight:bold}
button:hover{background:#07616b}
.btn-danger{background:#f44336}
.btn-danger:hover{background:#d32f2f}
#status{margin-top:10px;padding:10px;border-radius:5px;display:none}
.success{background:#4CAF50;color:white;display:block}
.error{background:#f44336;color:white;display:block}
.info{background:#e3f2fd;padding:15px;border-radius:5px;margin-bottom:15px;color:#07616b}
</style>
</head>
<body>
<h1>🔧 AgeVital TARS / %HOSTNAME%</h1>
<div class="card info">
<strong>Modo Desarrollador</strong><br>
Firmware %VERSION% · Configura WiFi, parametros del sistema o actualiza el firmware
</div>

<div class="card">
<h2>Lectura local</h2><p id="readings">Consultando el equipo...</p>
<p id="delivery"></p><small>Concentraciones instantaneas, no indice AQI. Este panel consulta solo el ESP32.</small>
<button type="button" onclick="fetch('/identify',{method:'POST'})">Identificar OLED</button>
<form action="/exit" method="POST"><button>Volver a medir</button></form>
</div>
<div class="card">
<h2>⚙️ Configuracion WiFi</h2>
<form action="/wifi" method="POST">
<label>Nombre de Red (SSID)</label>
<input type="text" name="ssid" value="%SELECTED_SSID%" placeholder="Nombre de Red (SSID)" maxlength="32" required>
<label>Contrasena WiFi</label>
<input type="password" name="pass" placeholder="Dejar vacio para redes abiertas">
<small style="color:#888;font-size:12px">Dejar vacio si la red no tiene contrasena</small>
<button type="submit">💾 Guardar y Reiniciar</button>
</form>
</div>

<div class="card">
<h2>🌐 Configuracion del Sistema</h2>
<form action="/config" method="POST">
<label>URL del Servidor</label>
<input type="text" name="serverUrl" value="%serverUrl%" placeholder="http://servidor/ruta">
<label>Hostname (nombre en red local)</label>
<input type="text" name="hostname" value="%hostname%" placeholder="tars-n (usa un hostname unico para cada tars)" >
<label>Intervalo de Envio (ms)</label>
<input type="number" name="intervaloEnvio" value="%intervaloEnvio%" placeholder="15000" min="10000">
<label>Intervalo de Lectura (ms)</label>
<input type="number" name="intervaloLectura" value="%intervaloLectura%" placeholder="2000" min="500">
<label>Intervalo de Reintento (ms)</label>
<input type="number" name="intervaloReintento" value="%intervaloReintento%" placeholder="20000" min="10000">
<label>Tiempo de Inactividad Pantalla (ms)</label>
<input type="number" name="tiempoInactividad" value="%tiempoInactividad%" placeholder="10000" min="10000">
<button type="submit">💾 Guardar Configuracion</button>
</form>
</div>

<div class="card">
<h2>🔑 Credenciales Keyrock</h2>
<form action="/keyrock" method="POST">
<label>URL del Token</label>
<input type="text" name="tokenUrl" value="%tokenUrl%" placeholder="http://servidor:3001/oauth2/token">
<label>Client ID</label>
<input type="text" name="clientId" value="%clientId%" placeholder="client_id">
<label>Client Secret</label>
<input type="password" name="clientSecret" placeholder="client_secret">
<label>Usuario Keyrock (email)</label>
<input type="text" name="keyrockUser" value="%keyrockUser%" placeholder="usuario@email.com">
<label>Password Keyrock</label>
<input type="password" name="keyrockPass" placeholder="password">
<label>Saltar validación de token (pruebas)</label>
<select name="skipToken" style="width:100%;padding:12px;margin:5px 0;border:1px solid #ddd;border-radius:5px;box-sizing:border-box">
  <option value="0" %SKIP_OFF%>Desactivado</option>
  <option value="1" %SKIP_ON%>Activado</option>
</select>
<button type="submit">💾 Guardar Credenciales</button>
</form>
</div>

<div class="card">
<h2>🤖 Agente Flask (opcional)</h2>
<p>Envio opcional del mismo promedio de particulas mediante POST.</p>
<form action="/agent" method="POST">
<label>URL del Agente</label>
<input type="text" name="agentUrl" value="%agentUrl%" placeholder="http://servidor:5000/v1/agent/tars1">
<label>Activar envío al agente</label>
<select name="useAgent" style="width:100%;padding:12px;margin:5px 0;border:1px solid #ddd;border-radius:5px;box-sizing:border-box">
  <option value="0" %AGENT_OFF%>Desactivado</option>
  <option value="1" %AGENT_ON%>Activado</option>
</select>
<button type="submit">💾 Guardar Agente</button>
</form>
</div>

<div class="card">
<h2>⚠️ Restaurar Valores a Quemados</h2>
<p style="color:#666;font-size:14px">Esto restaurara <strong>toda</strong> la configuracion a valores por defecto: WiFi, configuracion del sistema y credenciales de Keyrock.</p>
<form action="/config/reset" method="POST">
<button type="submit" class="btn-danger">🔄 Restaurar Todo a Defaults</button>
</form>
</div>

<div class="card">
<h2>🚀 Actualizacion OTA</h2>
<label>Selecciona el archivo .bin</label>
<input type="file" id="file" accept=".bin">
<button onclick="upload()">📤 Subir Firmware</button>
<div id="status"></div>
</div>

<script>
async function refresh(){
 if(document.hidden)return;
 try{const r=await fetch('/status',{cache:'no-store'});if(!r.ok)throw Error();const d=await r.json();
 document.getElementById('readings').textContent=d.valid ? `PM1.0: ${d.pm1_0} | PM2.5: ${d.pm2_5} | PM10: ${d.pm10} ug/m3` : 'Sin lectura actual del sensor';
 document.getElementById('delivery').textContent=`WiFi: ${d.wifi?'conectado':'sin conexion'} · Envios OK: ${d.sent} · Fallos: ${d.failed} · HTTP: ${d.http}. En configuracion el envio esta pausado.`;
 }catch{document.getElementById('readings').textContent='Equipo sin respuesta. Reconecta o recarga el panel.';}
}
refresh();setInterval(refresh,3000);

function upload(){
const f=document.getElementById('file').files[0];
const s=document.getElementById('status');
if(!f){s.className='error';s.textContent='Selecciona un archivo .bin';s.style.display='block';return;}
const fd=new FormData();
fd.append('file',f);
s.textContent='Subiendo firmware...';
s.style.display='block';
s.className='';
fetch('/ota',{method:'POST',body:fd})
.then(r=>{
if(r.ok){s.className='success';s.textContent='Actualizacion exitosa! Reiniciando...';}
else{s.className='error';s.textContent='Error en la actualizacion';}
})
.catch(()=>{s.className='error';s.textContent='Error de conexion';});
}
</script>
</body>
</html>
)=====";

DevWebOTA::DevWebOTA(WebServer *srv) : server(srv), initialized(false), routesRegistered(false) {}

void DevWebOTA::begin() {
  if (initialized) return;
  Serial.println("\n=== MODO DESARROLLADOR ACTIVADO ===\n");

  if (!wifiManager.connect(0)) {
    wifiManager.createAP(appConfig.hostname);
  }
  MDNS.begin(appConfig.hostname.c_str());

  // Las rutas SOLO se registran una vez (WebServer no soporta removeHandler,
  // cada .on() crea un handler en heap que se acumula si se repite)
  if (!routesRegistered) {

    server->on("/", HTTP_GET, [this]() {
      Serial.println("RESPUESTA GET / EXITOSA");
      server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
      server->sendHeader("Pragma", "no-cache");
      server->sendHeader("Expires", "0");
      // Inyectar hostname dinámico en el HTML
      String page = String(HTML_PAGE);
      page.replace("%HOSTNAME%", appConfig.hostname);
      page.replace("%VERSION%", TARS_FIRMWARE_VERSION);
      page.replace("%serverUrl%", htmlAttribute(String(appConfig.serverUrl)));
      page.replace("%hostname%", htmlAttribute(String(appConfig.hostname)));
      page.replace("%intervaloEnvio%", htmlAttribute(String(appConfig.intervaloEnvio)));
      page.replace("%intervaloLectura%", htmlAttribute(String(appConfig.intervaloLectura)));
      page.replace("%intervaloReintento%", htmlAttribute(String(appConfig.intervaloReintento)));
      page.replace("%tiempoInactividad%", htmlAttribute(String(appConfig.tiempoInactividad)));
      page.replace("%tokenUrl%", htmlAttribute(String(appConfig.tokenUrl)));
      page.replace("%clientId%", htmlAttribute(String(appConfig.clientId)));
      page.replace("%keyrockUser%", htmlAttribute(String(appConfig.keyrockUser)));
      page.replace("%agentUrl%", htmlAttribute(String(appConfig.agentUrl)));
      page.replace("%SKIP_OFF%", appConfig.skipToken ? "" : "selected");
      page.replace("%SKIP_ON%", appConfig.skipToken ? "selected" : "");
      page.replace("%AGENT_OFF%", appConfig.useAgent ? "" : "selected");
      page.replace("%AGENT_ON%", appConfig.useAgent ? "selected" : "");
      page.replace("%SELECTED_SSID%", htmlAttribute(selectedWiFiSSID));
      server->send(200, "text/html", page);
    });

    server->on("/status", HTTP_GET, [this]() {
      JsonDocument d;
      bool valid = sensorManager.hasValidReading() && sensorManager.isAirQualityOk() &&
          sensorManager.ageMs() <= max(10000UL, appConfig.intervaloLectura * 3);
      d["valid"] = valid;
      if (valid) {
        d["pm1_0"] = stateMachine.sensors.pm1_0; d["pm2_5"] = stateMachine.sensors.pm2_5;
        d["pm10"] = stateMachine.sensors.pm10;
      }
      d["wifi"] = wifiManager.isConnected(); d["sent"] = telemetry.sent;
      d["failed"] = telemetry.failed; d["http"] = telemetry.lastCode;
      String body; serializeJson(d, body);
      server->sendHeader("Cache-Control", "no-store"); server->send(200, "application/json", body);
    });
    server->on("/exit", HTTP_POST, [this]() {
      if (Update.isRunning() || restartPending) { server->send(409, "text/plain", "OTA o reinicio en curso"); return; }
      if (!appConfig.isConfigured) appConfig.save();
      stateMachine.flags.dev = false;
      server->send(200, "text/plain", "Volviendo a las mediciones. El panel se cerrara.");
    });

    server->on("/wifi", HTTP_POST, [this]() {
      String newssid = server->arg("ssid");
      String newpass = server->arg("pass");

      if (newssid.length() > 0 && newssid.length() <= 32 && (newpass.length() == 0 || (newpass.length() >= 8 && newpass.length() <= 63))) {
        wifiManager.saveCredentials(newssid, newpass);
        appConfig.save();
        Serial.printf("WiFi guardado: %s\n", newssid.c_str());

        server->send(200, "text/html",
                     "<!DOCTYPE html><html><head><meta charset='UTF-8'></head>"
                     "<body "
                     "style='font-family:Arial;text-align:center;padding:50px;background:#"
                     "f0f0f0'>"
                     "<div style='background:white;padding:40px;border-radius:10px'>"
                     "<h1 style='color:#4CAF50'>Configuracion Guardada</h1>"
                     "<p>El dispositivo se reiniciara...</p>"
                     "</div></body></html>");
        restartPending = true; restartAt = millis() + 1000;
      } else {
        server->send(400, "text/plain", "SSID o clave WiFi no validos");
      }
    });

    server->on("/config", HTTP_POST, [this]() {
      String newUrl = server->arg("serverUrl");
      String newHostname = server->arg("hostname");
      String newEnvio = server->arg("intervaloEnvio");
      String newLectura = server->arg("intervaloLectura");
      String newReintento = server->arg("intervaloReintento");
      String newInactividad = server->arg("tiempoInactividad");

      if ((newUrl.length() && !validUrl(newUrl)) ||
          (newHostname.length() && !validHostname(newHostname)) ||
          !validInterval(newEnvio, 10000, 86400000) ||
          !validInterval(newLectura, 500, 60000) ||
          !validInterval(newReintento, 10000, 86400000) ||
          !validInterval(newInactividad, 10000, 86400000)) {
        server->send(400, "text/plain", "URL, hostname o intervalos no validos; no se guardo ningun cambio");
        return;
      }

      if (newUrl.length() > 0) appConfig.serverUrl = newUrl;
      if (newHostname.length() > 0) appConfig.hostname = newHostname;
      if (newEnvio.length() > 0) appConfig.intervaloEnvio = newEnvio.toInt();
      if (newLectura.length() > 0) appConfig.intervaloLectura = newLectura.toInt();
      if (newReintento.length() > 0) appConfig.intervaloReintento = newReintento.toInt();
      if (newInactividad.length() > 0) appConfig.tiempoInactividad = newInactividad.toInt();

      appConfig.save();

      MDNS.end();
      MDNS.begin(appConfig.hostname.c_str());
      MDNS.addService("http", "tcp", 80);

      Serial.printf("[Config] serverUrl: %s\n", appConfig.serverUrl.c_str());
      Serial.printf("[Config] intervaloEnvio: %lu\n", appConfig.intervaloEnvio);
      Serial.printf("[Config] intervaloLectura: %lu\n", appConfig.intervaloLectura);
      Serial.printf("[Config] intervaloReintento: %lu\n", appConfig.intervaloReintento);
      Serial.printf("[Config] tiempoInactividad: %lu\n", appConfig.tiempoInactividad);
      Serial.printf("[Config] hostname: %s\n", appConfig.hostname.c_str());

      server->send(200, "text/html",
                   "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                   "<meta http-equiv='refresh' content='2;url=/'></head>"
                   "<body "
                   "style='font-family:Arial;text-align:center;padding:50px;background:#"
                   "f0f0f0'>"
                   "<div style='background:white;padding:40px;border-radius:10px'>"
                   "<h1 style='color:#4CAF50'>Configuracion Guardada</h1>"
                   "<p>Volviendo al panel...</p>"
                   "</div></body></html>");
    });

    server->on("/config/reset", HTTP_POST, [this]() {
      appConfig.reset();

      wifiManager.reset();

      Serial.printf("[Config] serverUrl: %s\n", appConfig.serverUrl.c_str());
      Serial.printf("[Config] intervaloEnvio: %lu\n", appConfig.intervaloEnvio);
      Serial.printf("[Config] intervaloLectura: %lu\n", appConfig.intervaloLectura);
      Serial.printf("[Config] intervaloReintento: %lu\n", appConfig.intervaloReintento);
      Serial.printf("[Keyrock] tokenUrl: %s\n", appConfig.tokenUrl.c_str());
      Serial.printf("[Keyrock] clientId: %s\n", appConfig.clientId.c_str());
      Serial.printf("[Keyrock] keyrockUser: %s\n", appConfig.keyrockUser.c_str());
      server->send(200, "text/html",
                   "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                   "<meta http-equiv='refresh' content='2;url=/'></head>"
                   "<body "
                   "style='font-family:Arial;text-align:center;padding:50px;background:#"
                   "f0f0f0'>"
                   "<div style='background:white;padding:40px;border-radius:10px'>"
                   "<h1 style='color:#FF9800'>Defaults Restaurados</h1>"
                   "<p>Reiniciando. Conecta a la red TARS del equipo para configurarlo.</p>"
                   "</div></body></html>");
      restartPending = true; restartAt = millis() + 1000;
    });

    // ===== Ruta Keyrock =====
    server->on("/keyrock", HTTP_POST, [this]() {
      String newTokenUrl = server->arg("tokenUrl");
      String newClientId = server->arg("clientId");
      String newClientSecret = server->arg("clientSecret");
      String newKeyrockUser = server->arg("keyrockUser");
      String newKeyrockPass = server->arg("keyrockPass");
      String newSkipToken = server->arg("skipToken");
      if ((newTokenUrl.length() && !validUrl(newTokenUrl)) ||
          newClientId.length() > 256 || newClientSecret.length() > 256 ||
          newKeyrockUser.length() > 256 || newKeyrockPass.length() > 256 ||
          (newSkipToken != "0" && newSkipToken != "1")) {
        server->send(400, "text/plain", "URL de token no valida");
        return;
      }

      if (newTokenUrl.length() > 0) appConfig.tokenUrl = newTokenUrl;
      if (newClientId.length() > 0) appConfig.clientId = newClientId;
      if (newClientSecret.length() > 0) appConfig.clientSecret = newClientSecret;
      if (newKeyrockUser.length() > 0) appConfig.keyrockUser = newKeyrockUser;
      if (newKeyrockPass.length() > 0) appConfig.keyrockPass = newKeyrockPass;
      appConfig.skipToken = (newSkipToken == "1");

      appConfig.save();


      Serial.printf("[Keyrock] tokenUrl: %s\n", appConfig.tokenUrl.c_str());
      Serial.printf("[Keyrock] clientId: %s\n", appConfig.clientId.c_str());
      Serial.printf("[Keyrock] keyrockUser: %s\n", appConfig.keyrockUser.c_str());

      server->send(200, "text/html",
                   "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                   "<meta http-equiv='refresh' content='2;url=/'></head>"
                   "<body "
                   "style='font-family:Arial;text-align:center;padding:50px;background:#"
                   "f0f0f0'>"
                   "<div style='background:white;padding:40px;border-radius:10px'>"
                   "<h1 style='color:#4CAF50'>Credenciales Keyrock Guardadas</h1>"
                   "<p>Volviendo al panel...</p>"
                   "</div></body></html>");
    });

    // ===== Ruta Agente  =====
    server->on("/agent", HTTP_POST, [this]() {
      String newAgentUrl = server->arg("agentUrl");
      String newUseAgent = server->arg("useAgent");
      if ((newAgentUrl.length() && !validUrl(newAgentUrl)) || (newUseAgent != "0" && newUseAgent != "1")) {
        server->send(400, "text/plain", "URL de agente no valida"); return;
      }

      if (newAgentUrl.length() > 0) appConfig.agentUrl = newAgentUrl;
      appConfig.useAgent = (newUseAgent == "1");

      appConfig.save();

      Serial.printf("[Agente] agentUrl: %s\n", appConfig.agentUrl.c_str());
      Serial.printf("[Agente] useAgent: %s\n", appConfig.useAgent ? "true" : "false");

      server->send(200, "text/html",
                   "<!DOCTYPE html><html><head><meta charset='UTF-8'>"
                   "<meta http-equiv='refresh' content='2;url=/'></head>"
                   "<body "
                   "style='font-family:Arial;text-align:center;padding:50px;background:#"
                   "f0f0f0'>"
                   "<div style='background:white;padding:40px;border-radius:10px'>"
                   "<h1 style='color:#4CAF50'>Agente Flask Guardado</h1>"
                   "<p>Volviendo al panel...</p>"
                   "</div></body></html>");
    });

    // ===== Ruta Identificar Dispositivo =====
    server->on("/identify", HTTP_POST, [this]() {
      if (!displayReady) { server->send(503, "text/plain", "OLED no disponible"); return; }
      identifyUntil = millis() + 1500;
      identifyActive = true;
      identifyDisplay(true);
      server->send(200, "text/plain", "OLED invertida durante 1.5 s");
    });

    // Ruta para OTA -- Esto no se toca a menos que quieras personalizar el
    // proceso de actualización
    server->on(
        "/ota", HTTP_POST,
        [this]() {
          server->sendHeader("Connection", "close");
          bool ok = otaComplete && !Update.hasError();
          otaComplete = false;
          server->send(ok ? 200 : 500, "text/plain", ok ? "OK" : "FAIL");
          if (!ok) return;
          restartPending = true; restartAt = millis() + 1000;
        },
        [this]() {
          HTTPUpload &upload = server->upload();
          if (upload.status == UPLOAD_FILE_START) {
            otaComplete = false;
            Serial.printf("Iniciando OTA: %s\n", upload.filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
              Update.printError(Serial);
            }
          } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
              Update.printError(Serial);
            } else {
              Serial.printf("  Progreso: %d%%\r", (Update.progress() * 100) / Update.size());
            }
          } else if (upload.status == UPLOAD_FILE_END) {
            otaComplete = Update.end(true);
            if (otaComplete) {
              Serial.printf("\nOTA completado: %u bytes\n", upload.totalSize);
            } else {
              Serial.println("\nError en OTA");
              Update.printError(Serial);
            }
          } else if (upload.status == UPLOAD_FILE_ABORTED) {
            otaComplete = false;
            Update.abort();
          }
        });

    server->begin();
    MDNS.addService("http", "tcp", 80); // Service discovery para mDNS
    Serial.println("Servidor web iniciado en puerto 80\n");
    routesRegistered = true;
  } else {
    // Rutas ya registradas, solo re-arrancar el servidor
    server->begin();
    MDNS.addService("http", "tcp", 80);
    Serial.println("Servidor web re-iniciado en puerto 80\n");
  }
  initialized = true;
}

void DevWebOTA::handle() {
  server->handleClient();
  if (restartPending && deadlineReached(millis(), restartAt)) ESP.restart();
  if (identifyActive && (long)(millis() - identifyUntil) >= 0) {
    identifyDisplay(false);
    identifyActive = false;
  }
}

void DevWebOTA::end() {
  identifyDisplay(false);
  identifyActive = false;
  if (initialized) {
    server->stop();
    MDNS.end();
    initialized = false;
    Serial.println("[DevWebOTA] Servidor y mDNS detenidos");
  }
}
