#pragma once
#include "Arduino.h"
#include <array>
#include <vector>
#include <fstream>
#include <stdexcept>
#include "glcdfont.c"
#define SSD1306_WHITE 1
#define SSD1306_BLACK 0
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_DISPLAYOFF 0xAE
struct TestText { int x,y,size;std::string text; };
struct Adafruit_SSD1306 {
    int x=0,y=0,size=1,color=1;
    bool on=true,inverted=false;
    std::vector<TestText> texts;
    std::array<uint8_t,128*64> pixels{};
    void clearDisplay() { pixels.fill(0);texts.clear(); }
    void setTextWrap(bool) {}
    void setTextSize(int s) { size=s; }
    void setTextColor(int c) { color=c; }
    void setCursor(int a,int b) { x=a;y=b; }
    void pixel(int a,int b,int c) { if(a>=0&&a<128&&b>=0&&b<64)pixels[b*128+a]=uint8_t(c); }
    void fillRect(int a,int b,int w,int h,int c) { for(int j=b;j<b+h;++j)for(int i=a;i<a+w;++i)pixel(i,j,c); }
    void drawRect(int a,int b,int w,int h,int c) { fillRect(a,b,w,1,c);fillRect(a,b+h-1,w,1,c);fillRect(a,b,1,h,c);fillRect(a+w-1,b,1,h,c); }
    void drawLine(int a,int b,int x1,int y1,int c) {
        int dx=std::abs(x1-a),sx=a<x1?1:-1,dy=-std::abs(y1-b),sy=b<y1?1:-1,err=dx+dy;
        for(;;){pixel(a,b,c);if(a==x1&&b==y1)break;int e=2*err;if(e>=dy){err+=dy;a+=sx;}if(e<=dx){err+=dx;b+=sy;}}
    }
    void print(const String& value) {
        std::string text=value.c_str();
        if(x<0||y<0||x+int(text.size())*6*size>128||y+8*size>64) throw std::runtime_error("Texto fuera de OLED: "+text);
        for(const auto& t:texts) {
            if(x<t.x+int(t.text.size())*6*t.size&&x+int(text.size())*6*size>t.x&&y<t.y+8*t.size&&y+8*size>t.y)
                throw std::runtime_error("Textos superpuestos: "+t.text+" / "+text);
        }
        texts.push_back({x,y,size,text});
        for(unsigned char ch:text) {
            for(int col=0;col<5;++col)for(int row=0;row<8;++row)
                if(font[ch*5+col]&(1<<row))fillRect(x+col*size,y+row*size,size,size,color);
            x+=6*size;
        }
    }
    void print(const char* s) { print(String(s)); }
    void print(int n) { print(String(n)); }
    unsigned frames=0;
    void display() { ++frames; }
    void ssd1306_command(int c) { on=c==SSD1306_DISPLAYON; }
    void invertDisplay(bool b) { inverted=b; }
    void save(const char* filename) { std::ofstream f(filename);f<<"P1\n128 64\n";for(int p:pixels)f<<(p?0:1)<<' '; }
};
