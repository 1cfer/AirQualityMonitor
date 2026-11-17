#pragma once
struct TestMdns { bool begin(const char*) { return true; } void end() {} void addService(const char*,const char*,int) {} };
inline TestMdns MDNS;
