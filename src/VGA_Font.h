#pragma once

#include "VGA_esp32s3.h"
#include "VGA_Math.h"

class VGA_Font {
public:
    VGA_Font(VGA_esp32s3& vga);    // Конструктор
    ~VGA_Font();                   // Деструктор

    void setFont(const uint8_t* font);
    void putChar(int x, int y, char ch, uint16_t col, bool trans = true, uint16_t bgCol = 0);
    void putInt(int x, int y, int num, uint16_t col, bool trans = true, uint16_t bgCol = 0);
    void putFloat(int x, int y, float num, uint16_t col, bool trans = true, uint16_t bgCol = 0);

    void drawChar(int x, int y, char c, uint16_t col);
    void drawText(int x, int y, const char* text, uint16_t color);

    int getWidth(){return _charWidth;}
    int getHeight(){return _charHeight;}
    int getSize(){return _size;}
    
protected:
    uint8_t* _font = nullptr;
    int _charWidth;
    int _charHeight;
    int _xx, _yy;
    int _size;

    VGA_esp32s3& _vga;        
    VGA_Math _math;
};
