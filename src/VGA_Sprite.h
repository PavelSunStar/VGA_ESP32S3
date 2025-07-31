#pragma once

#include "VGA_esp32s3.h" // Подключение библиотеки VGA
#include "VGA_Math.h"

class VGA_Sprite : public VGA_Math{
public:
    // Конструкторы и деструктор
    VGA_Sprite(VGA_esp32s3& vga);
    ~VGA_Sprite();

    int getWidth(){return _width;}
    int getHeight(){return _height;}
    int getSize(){return _size;}
    //int getBit(){return _bit;}
    bool isCreated(){return _created;}

    bool create(int x, int y, uint16_t col = 0);
    void destroy(); 

    void cls(uint16_t col = 0);
    void putPixel(int x, int y, uint16_t col);
    void hLine(int x1, int y, int x2, uint16_t col);
    void vLine(int x, int y1, int y2, uint16_t col);
     
    bool loadImage(const uint8_t *img, int index);
    void putImage(int x, int y, bool mirrorY = false);
    void putSprite(int x, int y, uint16_t col = 0, bool mirrorY = false);
    void putAniImage(int x, int y, uint8_t index = 0, uint16_t col = 0, bool mirrorY = false);
    void putAniSprite(int x, int y, uint8_t index = 0, uint16_t col = 0, bool mirrorY = false);


private:

protected:
    bool allocateMemory(int x, int y);
    //void calcImage(int x, int y, bool mirrorX, bool mirrorY);
    //void calcAni(int x, int y, bool mirrorX, bool mirrorY);

    //int _scrWidth, _scrHeight;
    int _width, _height;       // Размеры спрайта
    int _size;              // Полный размер буфера (в байтах)
    //int _bit;
    bool _created = false;
    uint8_t *_img8;             // Буфер изображения в PSRAM
    uint16_t *_img16;          // Буфер изображения в PSRAM
    //int* _fastY;

    //calc image
    int _scrWidth;
    int _xx, _yy;
    int _aniSize;

    VGA_esp32s3& _vga;          // Ссылка на VGA
    VGA_Math _math;
}; 
//Image-------------------------------------------------------------------------------------------------------------------------------------
/*
    void putImage(int x, int y, bool mirror = false);
    void putSprite(int x, int y, bool mirror_x = false, bool mirror_y = false, uint16_t maskColor = 0);
    void putSpriteAngle(int x, int y, int angle, uint16_t maskColor = 0);
    void putSpriteFade(int x, int y, uint16_t maskColor, uint8_t fade_r = 0, uint8_t fade_g = 0, uint8_t fade_b = 0);
    void putBG(int x, int y, bool repeat = false, bool mirror_y = false);

    // Методы управления памятью
    bool create(int xx, int yy); // Создание спрайта
    void destroy();              // Удаление спрайта

    //Draw from PROGMEM
    bool loadFromMem(const uint8_t *img, int index, uint16_t maskColor, bool isBackground = false);
    void putBGMem(const uint8_t *img, int index, int x, int y, bool repeat = false, bool mirror_y = false);
    void putImageMem(const uint8_t *img, int index, int x, int y, bool mirror = false);
    void putSpriteMem(const uint8_t *img, int index, 
                      int x, int y, uint16_t maskColor = 0, bool mirror_x = false, bool mirror_y = false);
    void putSpriteAngleMem(const uint8_t *img, int index, int x, int y, int angle, uint16_t maskColor = 0);                      

    void cls(uint16_t col = 0);
    void putPixel(int x, int y, uint16_t col);
    void hLine(int x1, int y, int x2, uint16_t col);
    void vLine(int x, int y1, int y2, uint16_t col);
    void rect(int x1, int y1, int x2, int y2, uint16_t col);

    // Получение параметров
    int getWidth(){return _width;};
    int getHeight(){return _height;};
    size_t getSize(){return _size;};
    int getColorBits(){return _colorBits;}
    bool isBackground(){return _bg;};
    bool isCreated(){return _created;}; 

    uint8_t* getImageAddr();
    uint16_t* getImageAddr16();    
*/




/*
//ScrollBar
    bool _sbCreated = false;
    uint8_t  _sbBg, _sbGridCol;
    int _sbGridStep;
    int _sbOffsetX;
    int _oldVal, _oldVal1, _oldVal2;

//Sprite
    int _scrWidth, _scrHeight;
    int _width, _height;       // Размеры спрайта
    int _size;              // Полный размер буфера (в байтах)
    uint16_t _colorBits, _maskColor;
    uint8_t *_img;             // Буфер изображения в PSRAM
    uint16_t *_img16;          // Буфер изображения в PSRAM
    bool _bg, _created;        // Флаг для фонового спрайта
    int _offsetX, _offsetY;

    bool allocateMemory();
    template <typename T>
    void drawSprite(T* vgaBuffer, T* imgBuffer, int dx, int dy, int sx, int sy, 
                            int sizex, int sizey, T maskColor, bool mirror_x, bool mirror_y);
*/
   