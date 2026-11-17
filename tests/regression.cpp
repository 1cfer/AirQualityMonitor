#include <cassert>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <functional>
#include "StateMachine.h"
#include "SensorManager.h"
#include "AppConfig.h"
#include "WiFiManager.h"
#include "DisplayManager.h"
#include "Telemetry.h"
#include "TokenManager.h"
#include "PayloadBuilder.h"
#include "Estados.h"
#include "DevWebOTA.h"
#include <Adafruit_SSD1306.h>
#include <Wire.h>
StateMachine stateMachine;
SensorManager sensorManager;
AppConfig appConfig;
WiFiManager wifiManager;
Adafruit_SSD1306 display;
WebServer server;
TokenManager tokenManager;
static void waitUntil(const std::function<bool()>& predicate) {
    auto end=std::chrono::steady_clock::now()+std::chrono::seconds(3);
    while(!predicate()) {
        assert(std::chrono::steady_clock::now()<end);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
static void finishJob() { waitUntil([] {telemetry.poll();return !telemetry.busy();}); }
static void batchTests() {
    SampleBatch b; for(int i=0;i<200000;++i)b.add({65534,65534,65534});
    assert(b.count<=65536&&b.average().pm2_5==65534);
    SampleBatch odd;for(int i=0;i<65535;++i)odd.add({65534,65534,65534});
    SampleBatch odd2=odd;odd.merge(odd2);assert(odd.average().pm2_5==65534);
    SampleBatch c;for(int i=0;i<40000;++i)c.add({65534,65534,65534});
    b.merge(c);assert(b.count<=65536&&b.average().pm10==65534);
    assert(deadlineReached(10,UINT32_MAX-5));assert(!deadlineReached(UINT32_MAX-5,10));
    appConfig.intervaloLectura=3456;appConfig.intervaloReintento=21000;appConfig.tiempoInactividad=43000;
    appConfig.save();AppConfig restored;restored.begin();
    assert(restored.intervaloLectura==3456&&restored.intervaloReintento==21000&&restored.tiempoInactividad==43000);
}
static void buttonTests() {
    ButtonHandler button(4);button.begin();
    auto tick=[&](uint32_t t,int level){testTime=t;testButton=level;return button.update();};
    tick(100,LOW);tick(151,LOW);assert(tick(4151,LOW)==ButtonHandler::VERY_LONG_PRESS_4S);
    assert(tick(4500,LOW)==ButtonHandler::NONE);tick(4600,HIGH);assert(tick(4651,HIGH)==ButtonHandler::NONE);
    tick(5000,LOW);tick(5051,LOW);tick(5100,HIGH);tick(5151,HIGH);
    assert(tick(5402,HIGH)==ButtonHandler::SHORT_PRESS);
    tick(6000,LOW);tick(6051,LOW);tick(6100,HIGH);tick(6151,HIGH);
    tick(6200,LOW);tick(6251,LOW);tick(6300,HIGH);assert(tick(6351,HIGH)==ButtonHandler::DOUBLE_CLICK);
}
static void tokenTests() {
    NetworkSettings c;c.tokenUrl="http://example.test/token";c.keyrockUser="a+b@x";c.keyrockPass="x&y=1";
    WiFi.state=WL_CONNECTED;testTime=0;TokenManager tokens;
    assert(tokens.requestToken(c)&&!HTTPClient::lastTls);
    assert(std::string(HTTPClient::lastBody.c_str()).find("a%2Bb%40x")!=std::string::npos);
    testTime=27000;assert(!tokens.hasToken());
    testTime=UINT32_MAX-1000;c.tokenUrl="https://example.test/token";assert(tokens.requestToken(c)&&HTTPClient::lastTls);
    testTime=1000;assert(tokens.hasToken());
    HTTPClient::response="{\"access_token\":\"\",\"expires_in\":30}";tokens.clear();assert(!tokens.requestToken(c));
    JsonDocument d;assert(!deserializeJson(d,construirPayload(1,2,3).c_str()));
    assert(d["pm1_0"]["value"]==1&&d["pm2_5"]["value"]==2&&d["pm10"]["value"]==3);
}
static void sensorTests() {
    sensorManager.begin();sensorManager.read();int n=sensorManager.getSampleCount();
    Wire.bytes=1;sensorManager.read();assert(!sensorManager.isAirQualityOk()&&sensorManager.getSampleCount()==n);
    Wire.bytes=2;Wire.error=1;sensorManager.read();assert(sensorManager.getSampleCount()==n);
    Wire.error=0;sensorManager.read();assert(sensorManager.isAirQualityOk()&&sensorManager.getSampleCount()==n+1);
    for(int i=0;i<75;++i){Wire.pm25=uint16_t(i);sensorManager.read();}
    assert(sensorManager.getPm25HistoryCount()==60);
}
static void displayTests() {
    displayReady=true;stateMachine.isDisplayOn=true;
    for(uint16_t value : {uint16_t(0),uint16_t(9),uint16_t(999),uint16_t(65534)}) {
        Wire.pm1=Wire.pm25=Wire.pm10=value;sensorManager.read();
        for(int page=0;page<5;++page) {
            stateMachine.screenMode=page;updateDisplay();
            if(value==999)display.save((std::string(".validation/screens/pagina-")+std::to_string(page)+".pbm").c_str());
        }
    }
    for(int i=0;i<60;++i) { Wire.pm1=9;Wire.pm10=44;Wire.pm25=uint16_t(12+(i%20));sensorManager.read(); }
    Wire.pm25=27;sensorManager.read();
    for(int page=0;page<5;++page){stateMachine.screenMode=page;updateDisplay();display.save((std::string(".validation/screens/pagina-")+std::to_string(page)+".pbm").c_str());}
    Wire.error=1;sensorManager.read();stateMachine.screenMode=0;updateDisplay();display.save(".validation/screens/sin-sensor.pbm");Wire.error=0;
    displaySplash();display.save(".validation/screens/arranque.pbm");
    displayDeveloperInfo();display.save(".validation/screens/desarrollador.pbm");
    displayWiFiList(0,0,1);display.save(".validation/screens/wifi.pbm");
    displayAPAlert("Una red con un nombre muy largo");displayWiFiConnection("Red de prueba",0);
    setDisplayPower(false);assert(!display.on);setDisplayPower(true);assert(display.on);
}
static void panelTests() {
    WebServer local;DevWebOTA panel(&local);WiFi.state=WL_CONNECTED;panel.begin();
    size_t routes=local.routes.size();panel.end();panel.begin();assert(local.routes.size()==routes);
    panel.setWiFiSelection("x\"<script>&");local.request("/",HTTP_GET);
    std::string html=local.body.c_str();assert(html.find("x&amp;")==std::string::npos);
    assert(html.find("&quot;&lt;script&gt;&amp;")!=std::string::npos);
    assert(html.find("%HOSTNAME%")==std::string::npos&&html.find("%serverUrl%")==std::string::npos);
    local.args={{"hostname","bad name"},{"intervaloLectura","600"}};
    auto before=appConfig.intervaloLectura;local.request("/config",HTTP_POST);assert(local.code==400&&appConfig.intervaloLectura==before);
    local.args={{"hostname","air-lab"},{"intervaloLectura","600"}};local.request("/config",HTTP_POST);
    assert(local.code==200&&appConfig.intervaloLectura==600);
    Wire.error=1;sensorManager.read();local.request("/status",HTTP_GET);JsonDocument d;
    assert(!deserializeJson(d,local.body.c_str())&&!d["valid"].as<bool>()&&d["pm2_5"].isNull());Wire.error=0;
    auto upload=local.uploads.at(local.key("/ota",HTTP_POST));
    Update.allowEnd=false;local.currentUpload.status=UPLOAD_FILE_START;upload();local.currentUpload.status=UPLOAD_FILE_END;upload();
    int restarts=ESP.restarts;local.request("/ota",HTTP_POST);assert(local.code==500&&ESP.restarts==restarts);
    Update.allowEnd=true;local.currentUpload.status=UPLOAD_FILE_START;upload();local.currentUpload.status=UPLOAD_FILE_ABORTED;upload();
    local.request("/ota",HTTP_POST);assert(local.code==500&&ESP.restarts==restarts);
    local.currentUpload.status=UPLOAD_FILE_START;upload();local.currentUpload.status=UPLOAD_FILE_END;upload();local.request("/ota",HTTP_POST);
    assert(local.code==200&&ESP.restarts==restarts);testTime+=1001;panel.handle();assert(ESP.restarts==restarts+1);
    panel.end();appConfig.hostname="air-lab";
}
static void transitionTests() {
    struct Next : State {
        void onEnter() override {} void execute() override {} void onExit() override {}
        const char* getName() override {return "next";}
    };
    struct First : Next {
        bool* destroyed;
        explicit First(bool* flag):destroyed(flag){}
        ~First() override {*destroyed=true;}
        void execute() override { statemachine->ChangeState(new Next());assert(!*destroyed); }
    };
    bool destroyed=false;StateMachine machine;machine.begin(new First(&destroyed));machine.update();assert(destroyed);
    Preferences prefs;prefs.begin("agevital",false);prefs.putString("ssid","Original");prefs.putString("pass","original123");prefs.end();
    wifiManager.connect(0);assert(wifiManager.hasPasswordFor("Original"));
    assert(wifiManager.startSelection("Original",false));WiFi.state=WL_CONNECTED;
    assert(wifiManager.pollSelection()==WiFiManager::SELECTED);
    assert(wifiManager.startSelection("Abierta",true));assert(WiFi.lastPass=="");
    testTime+=13000;assert(wifiManager.pollSelection()==WiFiManager::SELECTION_FAILED);
}
static void flowTests() {
    testTime=100000;WiFi.state=WL_CONNECTED;appConfig.isConfigured=true;appConfig.skipToken=true;
    appConfig.intervaloLectura=500;appConfig.tiempoInactividad=180000;
    stateMachine.clocks.ultima_interaccion=testTime;
    stateMachine.begin(new EstadoINICIO());stateMachine.update();
    assert(std::string(stateMachine.getCurrentStateName())=="LECTURA");
    assert(telemetry.ready());HTTPClient::patchCode=500;HTTPClient::block=true;HTTPClient::entered=false;
    int count=sensorManager.getSampleCount();assert(telemetry.start());
    waitUntil([]{return HTTPClient::entered.load();});
    assert(!telemetry.start()); // At most one request.
    unsigned frames=display.frames;
    for(int i=0;i<6;++i){testTime+=600;stateMachine.currentEvent=ButtonHandler::SHORT_PRESS;stateMachine.update();}
    stateMachine.currentEvent=ButtonHandler::NONE;
    assert(display.frames>frames&&sensorManager.getSampleCount()==6);
    assert(std::string(stateMachine.getCurrentStateName())=="LECTURA");
    stateMachine.screenMode=0;updateDisplay();display.save(".validation/screens/enviando.pbm");
    HTTPClient::block=false;finishJob();assert(telemetry.failed==1&&sensorManager.getSampleCount()==count+6);
    assert(stateMachine.clocks.proximo_envio==millis()+appConfig.intervaloReintento);
    HTTPClient::patchCode=204;HTTPClient::block=true;HTTPClient::entered=false;
    assert(telemetry.start());waitUntil([]{return HTTPClient::entered.load();});
    sensorManager.read();stateMachine.currentEvent=ButtonHandler::VERY_LONG_PRESS_4S;stateMachine.update();
    assert(std::string(stateMachine.getCurrentStateName())=="LECTURA"); // Wait safely before OTA.
    stateMachine.currentEvent=ButtonHandler::NONE;HTTPClient::block=false;finishJob();
    assert(telemetry.sent==1&&sensorManager.getSampleCount()==1);
    stateMachine.update();assert(std::string(stateMachine.getCurrentStateName())=="DESARROLLADOR");
    stateMachine.currentEvent=ButtonHandler::VERY_LONG_PRESS_4S;stateMachine.update();
    stateMachine.currentEvent=ButtonHandler::NONE;stateMachine.update();
    assert(std::string(stateMachine.getCurrentStateName())=="LECTURA");
}
int main() {
    batchTests();buttonTests();tokenTests();sensorTests();assert(telemetry.begin());displayTests();panelTests();transitionTests();flowTests();
    std::cout<<"PASS: bounded batches, NVS, button, OAuth, sensor failures, 128x64 geometry, real worker thread, retry retention and developer transition"<<std::endl;
    std::quick_exit(0); // The embedded worker deliberately has firmware lifetime.
}
