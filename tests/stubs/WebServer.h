#pragma once
#include "Arduino.h"
#include <map>
#include <functional>
#define HTTP_GET 0
#define HTTP_POST 1
#define UPLOAD_FILE_START 0
#define UPLOAD_FILE_WRITE 1
#define UPLOAD_FILE_END 2
#define UPLOAD_FILE_ABORTED 3
struct HTTPUpload {int status=0;String filename="test.bin";uint8_t buf[32]={};size_t currentSize=32,totalSize=32;};
struct WebServer {
    std::map<std::string,std::function<void()>> routes,uploads;
    std::map<std::string,String> args;int code=0;String body;HTTPUpload currentUpload;
    std::string key(const char* path,int method){return std::to_string(method)+path;}
    void on(const char* path,int method,std::function<void()> callback){routes[key(path,method)]=callback;}
    void on(const char* path,int method,std::function<void()> callback,std::function<void()> upload){on(path,method,callback);uploads[key(path,method)]=upload;}
    String arg(const char* name){return args[name];}
    void sendHeader(const char*,const char*){}
    void send(int c,const char*,const String& text){code=c;body=text;}
    void begin(){}void stop(){}void handleClient(){}
    HTTPUpload& upload(){return currentUpload;}
    void request(const char* path,int method){routes.at(key(path,method))();}
};
