#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <string>

#define LOW 0
#define HIGH 1
#define INPUT_PULLUP 2
#define OUTPUT 3
#define PROGMEM
using std::min;
using std::max;
using std::isfinite;
using byte = uint8_t;
template<class T> T constrain(T x,T lo,T hi) { return min(hi,max(lo,x)); }
inline uint32_t testTime = 0;
inline int testButton = HIGH, testAdc = 1000;
inline unsigned long millis() { return testTime; }
inline void delay(unsigned long ms) { testTime += ms; }
inline void pinMode(int,int) {}
inline void digitalWrite(int,int) {}
inline int digitalRead(int) { return testButton; }
inline int analogRead(int) { return testAdc; }

class String {
    std::string value;
public:
    String() = default;
    String(const char* s) : value(s ? s : "") {}
    String(const std::string& s) : value(s) {}
    String(float n,unsigned decimals=2) {
        std::ostringstream out; out << std::fixed << std::setprecision(decimals) << n; value=out.str();
    }
    String(int n) : value(std::to_string(n)) {}
    String(unsigned n) : value(std::to_string(n)) {}
    String(unsigned long n) : value(std::to_string(n)) {}
    size_t length() const { return value.length(); }
    const char* c_str() const { return value.c_str(); }
    bool concat(const char* s) { value+=s;return true; }
    bool startsWith(const char* s) const { return value.rfind(s,0)==0; }
    String substring(size_t a,size_t b=std::string::npos) const { return value.substr(a,b==std::string::npos?b:b-a); }
    long toInt() const {return value.empty()?0:std::stol(value);}
    int indexOf(char c) const { auto pos=value.find(c);return pos==std::string::npos?-1:int(pos); }
    void replace(const String& from,const String& to) {size_t pos=0;while((pos=value.find(from.value,pos))!=std::string::npos){value.replace(pos,from.value.size(),to.value);pos+=to.value.size();}}
    char operator[](size_t i) const { return value[i]; }
    String& operator+=(const String& s) { value+=s.value; return *this; }
    String& operator+=(char c) { value+=c; return *this; }
    friend String operator+(String a,const String& b) { return a+=b; }
    friend bool operator==(const String& a,const String& b) { return a.value==b.value; }
    friend bool operator!=(const String& a,const String& b) { return !(a==b); }
};
struct TestSerial {
    template<class T> void print(const T&) {}
    template<class T> void println(const T&) {}
    void println() {}
    template<class... T> void printf(const char*,T...) {}
};
inline TestSerial Serial;
struct TestEsp {
    int restarts=0;void restart(){++restarts;}
    unsigned getFreeHeap() { return 100000; }
    uint64_t getEfuseMac() { return 0x123456789abcULL; }
};
inline TestEsp ESP;
