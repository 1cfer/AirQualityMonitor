#pragma once
#include "Arduino.h"
#include <map>
class Preferences {
    std::string ns;
    static inline std::map<std::string,std::map<std::string,std::string>> db;
public:
    bool begin(const char* n,bool) { ns=n; return true; }
    void end() {}
    void clear() { db[ns].clear(); }
    String getString(const char* k,const char* fallback="") { auto& m=db[ns]; return m.count(k)?m[k]:fallback; }
    size_t putString(const char* k,const String& v) { db[ns][k]=v.c_str(); return v.length(); }
    bool getBool(const char* k,bool v) { return getULong(k,v)!=0; }
    void putBool(const char* k,bool v) { putULong(k,v); }
    unsigned long getULong(const char* k,unsigned long v) { auto& m=db[ns];return m.count(k)?std::stoul(m[k]):v; }
    void putULong(const char* k,unsigned long v) { if(std::strlen(k)<=15)db[ns][k]=std::to_string(v); }
};
