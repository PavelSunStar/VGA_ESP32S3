#pragma once

#include "VGA_esp32s3.h"  // Подключаем VGA_DMA_I2C
#include "VGA_Math.h"

class VGA_GFX : public VGA_esp32s3 {  
    public:
        VGA_GFX(VGA_esp32s3& vga);    // Конструктор
        ~VGA_GFX();                   // Деструктор

        void init();
        uint16_t getColor(uint8_t r, uint8_t g, uint8_t b);
        uint8_t bright8(uint8_t col, int8_t brightness);
        void cls(uint16_t col = 0);
        void putPixel(int x, int y, uint16_t col);
        void hLine(int x1, int y, int x2, uint16_t col);
        void vLine(int x, int y1, int y2, uint16_t col);
        void rect(int x1, int y1, int x2, int y2, uint16_t col);
        void fillRect(int x1, int y1, int x2, int y2, uint16_t col);        
        void line(int x1, int y1, int x2, int y2, uint16_t col);
        void lineAngle(int x, int y, int len, int angle, uint16_t col);
        void triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col);
        void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col);
        void circle(int x, int y, int r, uint16_t col);
        void fillCircle(int x, int y, int r, uint16_t col);
        void polygon(int x, int y, int radius, int sides, float rotation, uint16_t col);
        void fillPolygon(int x, int y, int radius, int sides, float rotation, uint16_t col);

        void fog(int x1, int y1, int x2, int y2); 
        void scrollScr(int x, int y);   

    protected:
        const uint8_t* _font = nullptr;
        uint8_t _charWidth = 0;
        uint8_t _charHeight = 0;
        
        uint32_t virtualY[600]; 

protected: 
    VGA_esp32s3& _vga;  // Ссылка на объект VGA_esp32s3, с которым работает VGA_GFX  
    VGA_Math _math;  
};