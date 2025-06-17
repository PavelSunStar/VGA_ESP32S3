#include <Arduino.h>
#include "VGA_Sprite.h"
#include "VGA_Math.h"
#include <cstring> // Для memset

#include <dspm_mult.h>  // SIMD-умножение матриц
#include <dspm_matrix.h> // Операции с матрицами
#include <dsps_mul.h>    // Умножение массивов
#include <esp_dsp.h>

// Конструкторы
// Конструктор с передачей ссылки на VGA
VGA_Sprite::VGA_Sprite(VGA_esp32s3& vga)
    : _vga(vga),
      _scrWidth(0), _scrHeight(0), 
      _width(0), _height(0), _size(0),
      _colorBits(0), _maskColor(0),
      _img(nullptr), _img16(nullptr),
      _bg(false), _created(false),
      _offsetX(0), _offsetY(0) {
    
    initLUT();
}

// Деструктор
VGA_Sprite::~VGA_Sprite() {
    destroy();
}

bool VGA_Sprite::allocateMemory() {
    switch (_colorBits) {
        case 16:
            _size = _width * _height * sizeof(uint16_t);
            _img16 = (uint16_t *)heap_caps_malloc(_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (_img16 == nullptr) return false;
            break;

        case 8:
            _size = _width * _height;
            _img = (uint8_t *)heap_caps_malloc(_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (_img == nullptr) return false;
            break;
    }
    return true;
}

// Создание буфера спрайта
bool VGA_Sprite::create(int xx, int yy) {
    if (_created || xx == 0 || yy == 0) return false; 

    _scrWidth = _vga.frameWidth();
    _scrHeight = _vga.frameHeight();
    _width = xx;
    _height = yy;
    _colorBits = _vga.getColorBits();

    // Выделение памяти для спрайта
    if (!allocateMemory()) {
        _scrWidth = _scrHeight = _width = _height = _size = _colorBits = _maskColor = 0;
        _img = nullptr;
        _img16 = nullptr;
        return false; // Ошибка выделения памяти
    }

    _created = true;
    return _created;
}

// Уничтожение спрайта
void VGA_Sprite::destroy() {
    if (_created) {        
        if (_colorBits == 16) {
            heap_caps_free(_img16);
            _img16 = nullptr;
        } else if (_colorBits == 8) {
            heap_caps_free(_img);
            _img = nullptr;            
        }

        // Сброс значений
        _scrWidth = _scrHeight = _width = _height = _size = _colorBits = _offsetX = _offsetY = 0;
        _created = _bg = false;
    }
}

bool VGA_Sprite::loadFromMem(const uint8_t *img, int index, uint16_t maskColor, bool isBackground) {
    if (img == nullptr || index < 0 || index >= img[1]) return false;

    int bit = img[0];
    int ind = img[1];

    int pos1 = img[index * 4 + 2];
    int pos2 = img[index * 4 + 3];
    int pos3 = img[index * 4 + 4];
    int pos4 = img[index * 4 + 5];
    int pos = 2 + ind * 4 + ((pos1 << 24) | (pos2 << 16) | (pos3 << 8) | pos4);

    int xx = (img[pos++] << 8) | img[pos++];  // Ширина
    int yy = (img[pos++] << 8) | img[pos++];  // Высота

    if (isBackground) {
        if (xx < _scrWidth || yy < _scrHeight) return false;
        _bg = isBackground;
    }

    // Выделяем память под изображение
    if (!create(xx, yy)) return false;

    if ((_vga.getColorBits() == 16) && (bit == 16)) {
        memcpy(_img16, img + pos, _size);
        _maskColor = maskColor;
    } else if ((_vga.getColorBits() == 16) && (bit == 8)) { 
        int cont = 0;
        Serial.println(img[pos]);
        for (int y = 0; y < yy; y++) 
            for (int x = 0; x < xx; x++) {
                int col = img[pos++];
                int r = (col >> 5) & 0b111;
                int g = (col >> 2) & 0b111;
                int b = col & 0b11;
                _img16[cont] = ((r * 31 / 7) & 0b11111) << 11 |
                               ((g * 63 / 7) & 0b111111) << 5 |
                               ((b * 31 / 3) & 0b11111);
                //_img16[cont] = ((uint16_t)map(r, 0, 7, 0, 31) & 0b11111) << 11 |
                //               ((uint16_t)map(g, 0, 7, 0, 63) & 0b111111) << 5 |
                //               ((uint16_t)map(b, 0, 3, 0, 31) & 0b11111);
                cont++;
        }

        int col = maskColor;
        int r = (col >> 5) & 0b111;
        int g = (col >> 2) & 0b111;
        int b = col & 0b11;
        _maskColor = ((r * 31 / 7) & 0b11111) << 11 |
                     ((g * 63 / 7) & 0b111111) << 5 |
                     ((b * 31 / 3) & 0b11111);
    } else if ((_vga.getColorBits() == 8) && (bit == 16)) {
        int cont = 0;

        for (int y = 0; y < yy; y++) 
            for (int x = 0; x < xx; x++) {
                int col = (img[pos + 1] << 8) | (img[pos]);                
                int r = (col >> 11) & 0b11111;
                int g = (col >> 5)  & 0b111111;
                int b = col         & 0b11111;
                _img[cont] = ((r * 7 / 31) & 0b111) << 5 |
                             ((g * 7 / 63) & 0b111) << 2 |
                             ((b * 3 / 31) & 0b11);
                //_img[cont] = ((uint8_t)map(r, 0, 31, 0, 7) & 0b111) << 5 |
                //             ((uint8_t)map(g, 0, 63, 0, 7) & 0b111) << 2 |
                //             ((uint8_t)map(b, 0, 31, 0, 3) & 0b11);
                cont++;
                pos+=2;
        }

        int col = maskColor;                
        int r = (col >> 11) & 0b11111;
        int g = (col >> 5)  & 0b111111;
        int b = col         & 0b11111;
       _maskColor = (((r * 7 / 31) & 0b111) << 5 |
                    ((g * 7 / 63) & 0b111) << 2 |
                    ((b * 3 / 31) & 0b11)) & 0xFF;
    } else if ((_vga.getColorBits() == 8) && (bit == 8)) {
        memcpy(_img, img + pos, _size);
        _maskColor = maskColor & 0xFF;
    } else {
        return false; // Если формат не поддерживается
    }

    return true;
}

//Draw from PROGMEM
void VGA_Sprite::putBGMem(const uint8_t *img, int index, int x, int y, bool repeat, bool mirror_y){
    if (img == nullptr || index < 0 || index >= img[1]) return;

    int bit = img[0];
    int ind = img[1];
    int pos1 = img[index * 4 + 2];
    int pos2 = img[index * 4 + 3];
    int pos3 = img[index * 4 + 4];
    int pos4 = img[index * 4 + 5];
    int pos = 2 + ind * 4 + ((pos1 << 24) | (pos2 << 16) | (pos3 << 8) | pos4);

    int xx = (img[pos++] << 8) | img[pos++];  // Ширина
    int yy = (img[pos++] << 8) | img[pos++];  // Высота
    int _screenWidth = _vga.frameWidth();
    int _screenHeight = _vga.frameHeight();
    if (_vga.getColorBits() != bit || xx < _screenWidth || yy < _screenHeight) return;

    x = repeat ? (x % xx + xx) % xx : std::clamp(x, 0, xx - _screenWidth);
    y = repeat ? (y % yy + yy) % yy : std::clamp(y, 0, yy - _screenHeight);

    int sizey = _screenHeight;
    uint8_t* src = (uint8_t*)(img + pos);

    if (bit == 16) {
        uint16_t* dest = _vga.getDrawBuffer16();
        for (int row = 0; row < _screenHeight; row++) {
            int yOffset = mirror_y ? (yy - 1 - ((y + row) % yy)) : ((y + row) % yy);
            uint16_t* srcRow = (uint16_t*)(src + yOffset * xx * 2);
            
            if (repeat && x + _screenWidth > xx) {
                int firstPart = xx - x;
                memcpy(dest, srcRow + x, firstPart * 2);
                memcpy(dest + firstPart, srcRow, (_screenWidth - firstPart) * 2);
            } else {
                memcpy(dest, srcRow + x, _screenWidth * 2);
            }
            dest += _screenWidth;
        }
    } else {
        uint8_t* dest = _vga.getDrawBuffer();
        for (int row = 0; row < _screenHeight; row++) {
            int yOffset = mirror_y ? (yy - 1 - ((y + row) % yy)) : ((y + row) % yy);
            uint8_t* srcRow = src + yOffset * xx;
            
            if (repeat && x + _screenWidth > xx) {
                int firstPart = xx - x;
                memcpy(dest, srcRow + x, firstPart);
                memcpy(dest + firstPart, srcRow, _screenWidth - firstPart);
            } else {
                memcpy(dest, srcRow + x, _screenWidth);
            }
            dest += _screenWidth;
        }
    }
}

void VGA_Sprite::putImageMem(const uint8_t *img, int index, int x, int y, bool mirror){
    if (img == nullptr || index < 0 || index >= img[1] || _vga.getColorBits() != img[0]) return;

    int bit = img[0];
    int ind = img[1];

    int pos1 = img[index * 4 + 2];
    int pos2 = img[index * 4 + 3];
    int pos3 = img[index * 4 + 4];
    int pos4 = img[index * 4 + 5];
    int pos = 2 + ind * 4 + ((pos1 << 24) | (pos2 << 16) | (pos3 << 8) | pos4);

    int xx = (img[pos++] << 8) | img[pos++];  // Ширина
    int yy = (img[pos++] << 8) | img[pos++];  // Высота

    int x2 = x + xx - 1;
    int y2 = y + yy - 1;  

    int vx1 = _vga.getvX1(), vx2 = _vga.getvX2();
    int vy1 = _vga.getvY1(), vy2 = _vga.getvY2();
    if (x > vx2 || y > vy2 || x2 < vx1 || y2 < vy1) return;

    int dx = std::max(x, vx1);
    int dy = std::max(y, vy1);
    int sx = dx - x;
    int sy = dy - y;

    int sizex = std::min(x2, vx2) - dx + 1;
    int sizey = std::min(y2, vy2) - dy + 1;

    int scrWidth = _vga.frameWidth();
    img += pos + xx * (mirror ? (yy - 1 - sy) : sy) + sx;

    if (bit == 16){
        uint16_t* dest = &_vga.getDrawBuffer16()[scrWidth * dy + dx];
        sizex <<= 1;
        xx <<= 1;

        while (sizey-- > 0) {
            memcpy(dest, img, sizex);
            dest += scrWidth;
            img += mirror ? -xx : xx;
        }  
    } else {
        uint8_t* dest = &_vga.getDrawBuffer()[scrWidth * dy + dx];

        while (sizey-- > 0) {
            memcpy(dest, img, sizex);
            dest += scrWidth;
            img += mirror ? -xx : xx;
        }        
    }    
}

void VGA_Sprite::putSpriteMem(const uint8_t *img, int index, int x, int y, uint16_t maskColor, bool mirror_x, bool mirror_y){
    if (img == nullptr || index < 0 || index >= img[1] || _vga.getColorBits() != img[0]) return;

    int bit = img[0];
    int ind = img[1];

    int pos1 = img[index * 4 + 2];
    int pos2 = img[index * 4 + 3];
    int pos3 = img[index * 4 + 4];
    int pos4 = img[index * 4 + 5];
    int pos = 2 + ind * 4 + ((pos1 << 24) | (pos2 << 16) | (pos3 << 8) | pos4);

    int xx = (img[pos++] << 8) | img[pos++];  // Ширина
    int yy = (img[pos++] << 8) | img[pos++];  // Высота

    int x2 = x + xx - 1;
    int y2 = y + yy - 1;  

    int vx1 = _vga.getvX1(), vx2 = _vga.getvX2();
    int vy1 = _vga.getvY1(), vy2 = _vga.getvY2();
    if (x > vx2 || y > vy2 || x2 < vx1 || y2 < vy1) return;

    int dx = std::max(x, vx1);
    int dy = std::max(y, vy1);
    int sx = dx - x;
    int sy = dy - y;

    int sizex = std::min(x2, vx2) - dx + 1;
    int sizey = std::min(y2, vy2) - dy + 1;

    int scrWidth = _vga.frameWidth();
    const uint8_t *src_img = img + pos + xx * (mirror_y ? (yy - 1 - sy) : sy);
    
    if (bit == 16){
        uint16_t* dest = &_vga.getDrawBuffer16()[scrWidth * dy + dx];
        sizex <<= 1;
        xx <<= 1;

        while (sizey-- > 0) {
            const uint16_t *src = (const uint16_t *)(src_img + (mirror_x ? xx - 2 - sx * 2 : sx * 2));
            for (int i = 0; i < sizex / 2; i++) {
                if (*src != maskColor) *dest = *src;
                dest++;
                src += mirror_x ? -1 : 1;
            }
            dest += scrWidth - sizex / 2;
            src_img += mirror_y ? -xx : xx;
        }  
    } else {
        uint8_t* dest = &_vga.getDrawBuffer()[scrWidth * dy + dx];

        while (sizey-- > 0) {
            const uint8_t *src = src_img + (mirror_x ? xx - 1 - sx : sx);
            for (int i = 0; i < sizex; i++){
                if (*src != maskColor) *dest = *src;
                dest++;
                src += mirror_x ? -1 : 1;
            }
            dest += scrWidth - sizex;
            src_img += mirror_y ? -xx : xx;
        }        
    }    
}

void VGA_Sprite::putSpriteAngleMem(const uint8_t *img, int index, int x, int y, int angle, uint16_t maskColor) {
    if (!img || index < 0 || x >= _vga.getvX2() || y >= _vga.getvY2()) return; 
    
    uint8_t bit = *img++;  // Битность изображения (8 бит)
    uint8_t ind = *img++;  // Количество кадров в спрайте
    if (_vga.getColorBits() != bit || index >= ind) return;

    // Пропускаем индексы до нужного кадра
    img += index * 4;
    uint32_t pos = ((uint32_t) *img++ << 24) | ((uint32_t) *img++ << 16) | 
                   ((uint32_t) *img++ << 8)  | ((uint32_t) *img++);
    
    img += (ind - index - 1) * 4 + pos;  // Переход к данным изображения

    int width = (*img++ << 8) | *img++;
    int height = (*img++ << 8) | *img++;
    if (x + width - 1 < _vga.getvX1() || y + height - 1 < _vga.getvY1()) return;

    int cx = width >> 1;
    int cy = height >> 1;

    float sinAngle = sinLUT[angle % 360];
    float cosAngle = cosLUT[angle % 360];

    //int radius = sqrt(cx * cx + cy * cy);
    int offsetX = max(cx + 1, cy + 1);//radius;
    int offsetY = max(cx + 1, cy + 1);//radius;

    uint8_t* dest8 = _vga.getDrawBuffer();  // Буфер для 8 бит

    // Корректная обрезка границ экрана
    int slx = (x < _vga.getvX1()) ? _vga.getvX1() - x : 0;
    int suy = (y < _vga.getvY1()) ? _vga.getvY1() - y : 0;
    int srx = (x + width > _vga.getvX2()) ? (x + width - _vga.getvX2()) : 0;
    int sdy = (y + height > _vga.getvY2()) ? (y + height - _vga.getvY2()) : 0;

    for (int screenY = -offsetY + suy; screenY < offsetY - sdy; screenY++) {
        int sinY = sinAngle * screenY + cx;
        int cosY = cosAngle * screenY + cy;

        for (int screenX = -offsetX + slx; screenX < offsetX - srx; screenX++) {
            int srcX = round(cosAngle * screenX + sinY);
            int srcY = round(-sinAngle * screenX + cosY);

            // Проверка выхода за границы
            if (srcX >= 0 && srcX < width && srcY >= 0 && srcY < height) {
                const uint8_t* srcPtr = img + srcY * width + srcX;
                uint8_t* destPtr = dest8 + (y + screenY) * _vga.frameWidth() + (x + screenX);

                if (*srcPtr != maskColor) *destPtr = *srcPtr;
            }
        }
    }
}


/*
    Serial.print(width); Serial.print("x"); Serial.println(height);
    delay(500);
    */

//Image-------------------------------------------------------------------------------------------------------------------------------------
void VGA_Sprite::putBG(int x, int y, bool repeat, bool mirror_y) {
    if (!_created || !_bg) return;

    x = repeat ? (x % _width + _width) % _width : std::clamp(x, 0, _width - _scrWidth);
    y = repeat ? (y % _height + _height) % _height : std::clamp(y, 0, _height - _scrHeight);

    if (_colorBits == 16) {
        uint16_t* dest = _vga.getDrawBuffer16();

        for (int row = 0; row < _scrHeight; row++) {
            int srcY = mirror_y ? (_height - 1 - ((y + row) % _height)) : ((y + row) % _height);
            uint16_t* sour = &_img16[_width * srcY];

            if (repeat && x + _scrWidth > _width) {
                int firstPart = _width - x;
                memcpy(dest, &sour[x], firstPart * sizeof(uint16_t));
                memcpy(dest + firstPart, sour, (_scrWidth - firstPart) * sizeof(uint16_t));
            } else {
                memcpy(dest, &sour[x], _scrWidth * sizeof(uint16_t));
            }

            dest += _scrWidth;
        }
    } else if (_colorBits == 8) {
        uint8_t* dest = _vga.getDrawBuffer();

        for (int row = 0; row < _scrHeight; row++) {
            int srcY = mirror_y ? (_height - 1 - ((y + row) % _height)) : ((y + row) % _height);
            uint8_t* sour = &_img[_width * srcY];

            if (repeat && x + _scrWidth > _width) {
                int firstPart = _width - x;
                memcpy(dest, &sour[x], firstPart);
                memcpy(dest + firstPart, sour, _scrWidth - firstPart);
            } else {
                memcpy(dest, &sour[x], _scrWidth);
            }

            dest += _scrWidth;
        }
    }
}

/*
void VGA_Sprite::putBG(int x, int y, bool repeat) {
    if (!_created) return;

    x = std::clamp(x, 0, _width - _scrWidth);
    y = std::clamp(y, 0, _height - _scrHeight);

    int sizey = _scrHeight;

    if (_colorBits == 16) {
        uint16_t* dest = _vga.getDrawBuffer16();
        uint16_t* sour = &_img16[_width * y + x];

        while (sizey--) {
            memcpy(dest, sour, _scrWidth * sizeof(uint16_t));
            dest += _scrWidth;
            sour += _width;
        }
    } else if (_colorBits == 8){
        uint8_t* dest = _vga.getDrawBuffer();
        uint8_t* sour = &_img[_width * y + x];

        while (sizey--) {
            memcpy(dest, sour, _scrWidth);
            dest += _scrWidth;
            sour += _width;
        }
    }
}
*/
void VGA_Sprite::putImage(int x, int y, bool mirror) {
    if (!_created) return;

    int x2 = x + _width - 1;
    int y2 = y + _height - 1;

    int vx1 = _vga.getvX1(), vx2 = _vga.getvX2();
    int vy1 = _vga.getvY1(), vy2 = _vga.getvY2();

    if (x > vx2 || y > vy2 || x2 < vx1 || y2 < vy1) return;

    int dx = std::max(x, vx1);
    int dy = std::max(y, vy1);
    int sx = dx - x;
    int sy = dy - y;

    int sizex = std::min(x2, vx2) - dx + 1;
    int sizey = std::min(y2, vy2) - dy + 1;

    if (_colorBits == 16) {
        uint16_t* dest = &_vga.getDrawBuffer16()[_scrWidth * dy + dx];
        uint16_t* sour = &_img16[_width * (mirror ? (_height - 1 - sy) : sy) + sx];

        while (sizey-- > 0) {
            memcpy(dest, sour, sizex * sizeof(uint16_t));
            dest += _scrWidth;
            sour = (mirror) ? sour - _width : sour + _width;
        }
    } else { // 8-битный режим
        uint8_t* dest = &_vga.getDrawBuffer()[_scrWidth * dy + dx];
        uint8_t* sour = &_img[_width * (mirror ? (_height - 1 - sy) : sy) + sx];

        while (sizey-- > 0) {
            memcpy(dest, sour, sizex);
            dest += _scrWidth;
            sour = (mirror) ? sour - _width : sour + _width;
        }
    }
}

void VGA_Sprite::putSprite(int x, int y, bool mirror_x, bool mirror_y, uint16_t maskColor) {
    if (!_created) return;

    int x2 = x + _width - 1;
    int y2 = y + _height - 1;

    int vx1 = _vga.getvX1(), vx2 = _vga.getvX2();
    int vy1 = _vga.getvY1(), vy2 = _vga.getvY2();
    if (x > vx2 || y > vy2 || x2 < vx1 || y2 < vy1) return;

    int dx = std::max(x, vx1);
    int dy = std::max(y, vy1);
    int sx = dx - x;
    int sy = dy - y;
    int sizex = std::min(x2, vx2) - dx + 1;
    int sizey = std::min(y2, vy2) - dy + 1;
    if (_maskColor != 0) maskColor = _maskColor;

    if (_colorBits == 16) {
        drawSprite<uint16_t>(_vga.getDrawBuffer16(), _img16, dx, dy, sx, sy, sizex, sizey, maskColor, mirror_x, mirror_y);
    } else {
        drawSprite<uint8_t>(_vga.getDrawBuffer(), _img, dx, dy, sx, sy, sizex, sizey, maskColor, mirror_x, mirror_y);
    }
}

// Универсальная функция для 8/16 бит
template <typename T>
void VGA_Sprite::drawSprite(T* vgaBuffer, T* imgBuffer, int dx, int dy, int sx, int sy, 
                            int sizex, int sizey, T maskColor, bool mirror_x, bool mirror_y) {
    T* dest = &vgaBuffer[_scrWidth * dy + dx];
    T* sour = &imgBuffer[_width * (mirror_y ? (_height - 1 - sy) : sy) + (mirror_x ? (_width - 1 - sx) : sx)];

    int stepX = mirror_x ? -1 : 1;
    int stepY = mirror_y ? -_width : _width;

    while (sizey-- > 0) {
        T* d = dest;
        T* s = sour;

        for (int i = 0; i < sizex; i++) {
            if (*s != maskColor) *d = *s;
            d++;
            s += stepX;
        }
        dest += _scrWidth;
        sour += stepY;
    }
}

void VGA_Sprite::putSpriteAngle(int x, int y, int angle, uint16_t maskColor) {
    if (!_created || !_img) return;

    int vx1 = _vga.getvX1(), vx2 = _vga.getvX2();
    int vy1 = _vga.getvY1(), vy2 = _vga.getvY2();
    int cx = _width >> 1, cy = _height >> 1;

    int sinAngle = sinLUT[angle % 360];
    int cosAngle = cosLUT[angle % 360];

    int offsetX = max(cx + 1, cy + 1);
    int offsetY = offsetX;

    uint8_t* dest8 = _vga.getDrawBuffer();
    
    for (int screenY = -offsetY; screenY <= offsetY; screenY++) {
        int destY = y + screenY;
        if (destY < vy1 || destY > vy2) continue;
    
        int sinScrY = round((sinAngle * screenY) >> 15) + cx;
        int cosScrY = round((cosAngle * screenY) >> 15) + cy;
    
        uint8_t* destPtr = dest8 + destY * _scrWidth + (x - offsetX);
        for (int screenX = -offsetX; screenX <= offsetX; screenX++) {
            int destX = x + screenX;
            if (destX < vx1 || destX > vx2) continue;
    
            int srcX = ((cosAngle * screenX) >> 15) + sinScrY;
            int srcY = ((-sinAngle * screenX) >> 15) + cosScrY;  // Предвычисленный baseSrcY
    
            if (srcX >= 0 && srcX < _width && srcY >= 0 && srcY < _height) {
                uint8_t* srcPtr = _img + srcY * _width + srcX;
                if (*srcPtr != maskColor) *(destPtr + screenX + offsetX) = *srcPtr;
            }
        }
    }   
}




/*
void VGA_Sprite::putSpriteAngle(int x, int y, int angle, uint16_t maskColor) {
    if (!_created || !_img) return;

    int vx1 = _vga.getvX1(), vx2 = _vga.getvX2();
    int vy1 = _vga.getvY1(), vy2 = _vga.getvY2();
    int cx = _width >> 1, cy = _height >> 1;  // Центр спрайта

    float cosA = cosf(angle * M_PI / 180.0);
    float sinA = sinf(angle * M_PI / 180.0);
    float matrix[4] = { cosA, -sinA, sinA, cosA };  // Матрица поворота

    int offsetX = max(cx + 1, cy + 1);
    int offsetY = offsetX;

    uint8_t* dest8 = _vga.getDrawBuffer();

    for (int screenY = -offsetY; screenY <= offsetY; screenY++) {
        int destY = y + screenY;
        if (destY < vy1 || destY > vy2) continue;

        uint8_t* destPtr = dest8 + destY * _scrWidth + (x - offsetX);

        // Буферы для SIMD-умножения
        float xBuf[4], yBuf[4], xRot[4], yRot[4];

        for (int screenX = -offsetX; screenX <= offsetX; screenX += 4) {
            int destX = x + screenX;
            if (destX < vx1 || destX > vx2) continue;

            // Переводим координаты в систему центра спрайта
            for (int i = 0; i < 4; i++) {
                xBuf[i] = (screenX + i) - cx;
                yBuf[i] = screenY - cy;
            }

            // Умножаем через ESP-DSP
            dsps_mul_f32_ae32(xBuf, matrix, xRot, 4, 1, 2, 1);
            dsps_mul_f32_ae32(yBuf, matrix + 2, yRot, 4, 1, 2, 1);

            for (int i = 0; i < 4; i++) {
                // Возвращаем координаты в область спрайта
                int srcX = (int)(xRot[i] + cx);
                int srcY = (int)(yRot[i] + cy);

                if (srcX >= 0 && srcX < _width && srcY >= 0 && srcY < _height) {
                    uint8_t* srcPtr = _img + srcY * _width + srcX;
                    if (*srcPtr != maskColor) *(destPtr + screenX + i + offsetX) = *srcPtr;
                }
            }
        }
    }
}


/*
работает
void VGA_Sprite::putSpriteAngle(int x, int y, int angle, uint16_t maskColor) {
    if (!_created || !_img) return;

    int vx1 = _vga.getvX1(), vx2 = _vga.getvX2();
    int vy1 = _vga.getvY1(), vy2 = _vga.getvY2();
    int cx = _width >> 1, cy = _height >> 1;

    int sinAngle = sinLUT[angle % 360];
    int cosAngle = cosLUT[angle % 360];

    int offsetX = max(cx + 1, cy + 1);
    int offsetY = offsetX;

    uint8_t* dest8 = _vga.getDrawBuffer();
    
    for (int screenY = -offsetY; screenY <= offsetY; screenY++) {
        int destY = y + screenY;
        if (destY < vy1 || destY > vy2) continue;
    
        int sinScrY = ((sinAngle * screenY) >> 15) + cx;
        int cosScrY = ((cosAngle * screenY) >> 15) + cy;
    
        uint8_t* destPtr = dest8 + destY * _scrWidth + (x - offsetX);
        for (int screenX = -offsetX; screenX <= offsetX; screenX++) {
            int destX = x + screenX;
            if (destX < vx1 || destX > vx2) continue;
    
            int srcX = ((cosAngle * screenX) >> 15) + sinScrY;
            int srcY = ((-sinAngle * screenX) >> 15) + cosScrY;  // Предвычисленный baseSrcY
    
            if (srcX >= 0 && srcX < _width && srcY >= 0 && srcY < _height) {
                uint8_t* srcPtr = _img + srcY * _width + srcX;
                if (*srcPtr != maskColor) *(destPtr + screenX + offsetX) = *srcPtr;
            }
        }
    }   
}

/*
void VGA_Sprite::putSpriteAngle(int x, int y, int angle, uint16_t maskColor) {
    if (!_created || !_img) return;

    int vx1 = _vga.getvX1(), vx2 = _vga.getvX2();
    int vy1 = _vga.getvY1(), vy2 = _vga.getvY2();
    int cx = _width >> 1, cy = _height >> 1;

    int sinAngle = sinLUT[angle % 360];
    int cosAngle = cosLUT[angle % 360];

    int offsetX = max(cx + 1, cy + 1);
    int offsetY = offsetX;

    uint8_t* dest8 = _vga.getDrawBuffer();
    
    for (int screenY = -offsetY; screenY <= offsetY; screenY++) {
        int destY = y + screenY;
        if (destY < vy1 || destY > vy2) continue;
        
        int sinScrY = ((sinAngle * screenY) >> 15) + cx;
        int cosScrY = ((cosAngle * screenY) >> 15) + cy;

        uint8_t* destPtr = dest8 + destY * _scrWidth + (x - offsetX);

        for (int screenX = -offsetX; screenX <= offsetX; screenX++) {
            int destX = x + screenX;
            if (destX < vx1 || destX > vx2) continue;
            
            // Умножение с фиксированной точкой (Q16.16)
            int srcX = ((cosAngle * screenX) >> 15) + sinScrY;
            int srcY = ((-sinAngle * screenX) >> 15) + cosScrY;
            
            if (srcX >= 0 && srcX < _width && srcY >= 0 && srcY < _height) {
                uint8_t* srcPtr = _img + srcY * _width + srcX;
                if (*srcPtr != maskColor) *(destPtr + screenX + offsetX) = *srcPtr;
            }
        }
    }
}

/*
void VGA_Sprite::putSprite(int x, int y, uint16_t maskColor, bool mirror_x, bool mirror_y){
    if (!_created) return;

    int x2 = x + _width - 1;
    int y2 = y + _height - 1;

    int vx1 = _vga.getvX1(), vx2 = _vga.getvX2();
    int vy1 = _vga.getvY1(), vy2 = _vga.getvY2();

    if (x > vx2 || y > vy2 || x2 < vx1 || y2 < vy1) return;

    int dx = std::max(x, vx1);
    int dy = std::max(y, vy1);
    int sx = dx - x;
    int sy = dy - y;

    int sizex = std::min(x2, vx2) - dx + 1;
    int sizey = std::min(y2, vy2) - dy + 1;

    if (_colorBits == 16) {
        uint16_t* dest = &_vga.getDrawBuffer16()[_scrWidth * dy + dx];
        uint16_t* sour = &_img16[_width * (mirror_y ? (_height - 1 - sy) : sy) + (mirror_x ? (_width - 1 - sx) : sx)];

        uint16_t* savePos = nullptr;
        uint16_t* savePosImg = nullptr;

        while (sizey-- > 0) {
            savePos = dest;
            savePosImg = sour; 

            for (int i = 0; i < sizex; i++){
                if (*sour != maskColor) {
                    *dest++ = (mirror_x) ? *sour-- : *sour++;
                } else {
                    dest++;
                    if (mirror_x) sour--; else sour++;
                }
            }
            
            dest = savePos + _scrWidth;
            sour = savePosImg + ((mirror_y) ? -_width : _width);            
        }                                 
    } else { // 8-битный режим
        uint8_t* dest = &_vga.getDrawBuffer()[_scrWidth * dy + dx];
        uint8_t* sour = &_img[_width * (mirror_y ? (_height - 1 - sy) : sy) + (mirror_x ? (_width - 1 - sx) : sx)];

        uint8_t* savePos = nullptr;
        uint8_t* savePosImg = nullptr;

        while (sizey-- > 0) {
            savePos = dest;
            savePosImg = sour; 

            for (int i = 0; i < sizex; i++){
                if (*sour != maskColor) {
                    *dest++ = (mirror_x) ? *sour-- : *sour++;
                } else {
                    dest++;
                    if (mirror_x) sour--; else sour++;
                }
            }
            
            dest = savePos + _scrWidth;
            sour = savePosImg + ((mirror_y) ? -_width : _width);            
        }      
    }   
}
*/
void VGA_Sprite::putSpriteFade(int x, int y, uint16_t maskColor, uint8_t fade_r, uint8_t fade_g, uint8_t fade_b) {
    if (!_created) return;

    int x2 = x + _width - 1;
    int y2 = y + _height - 1;

    int vx1 = _vga.getvX1(), vx2 = _vga.getvX2();
    int vy1 = _vga.getvY1(), vy2 = _vga.getvY2();

    if (x > vx2 || y > vy2 || x2 < vx1 || y2 < vy1) return;

    int dx = std::max(x, vx1);
    int dy = std::max(y, vy1);
    int sx = dx - x;
    int sy = dy - y;

    int sizex = std::min(x2, vx2) - dx + 1;
    int sizey = std::min(y2, vy2) - dy + 1;

    int frameWidth = _vga.frameWidth();
    int colorBits = _vga.getColorBits();

    if (colorBits == 16){
        uint8_t fadeR = map(fade_r, 0, 256, 0, 32);  // Корректный диапазон для 5 бит
        uint8_t fadeG = map(fade_g, 0, 256, 0, 64);  // Корректный диапазон для 6 бит
        uint8_t fadeB = map(fade_b, 0, 256, 0, 32);  // Корректный диапазон для 5 бит

        uint16_t* dest = &_vga.getDrawBuffer16()[frameWidth * dy + dx];
        uint16_t* sour = &_img16[_width * sy + sx];

        while (sizey-- > 0) {
            uint16_t* savePos = dest;
            uint16_t* savePosImg = sour;
            for (int i = 0; i < sizex; i++) {
                if (*sour != maskColor) {
                    uint16_t col = *sour;

                    uint8_t r = (col >> 11) & 0b11111;
                    uint8_t g = (col >> 5)  & 0b111111;
                    uint8_t b = (col)       & 0b11111;

                    r = (r > fadeR) ? (r - fadeR) : 0;
                    g = (g > fadeG) ? (g - fadeG) : 0;
                    b = (b > fadeB) ? (b - fadeB) : 0;

                    *dest = (r << 11) | (g << 5) | b;
                }
                dest++;
                sour++;
            }
            dest = savePos + frameWidth;
            sour = savePosImg + _width;
        }
    } else {
        uint8_t fadeR = map(fade_r, 0, 256, 0, 8);  // Корректный диапазон для 5 бит
        uint8_t fadeG = map(fade_g, 0, 256, 0, 8);  // Корректный диапазон для 6 бит
        uint8_t fadeB = map(fade_b, 0, 256, 0, 4);  // Корректный диапазон для 5 бит

        uint8_t* dest = &_vga.getDrawBuffer()[frameWidth * dy + dx];
        uint8_t* sour = &_img[_width * sy + sx];

        while (sizey-- > 0) {
            uint8_t* savePos = dest;
            uint8_t* savePosImg = sour;
            for (int i = 0; i < sizex; i++) {
                if (*sour != maskColor) {
                    uint8_t col = *sour;

                    uint8_t r = (col >> 5) & 0b111;
                    uint8_t g = (col >> 2)  & 0b111;
                    uint8_t b = (col)       & 0b11;

                    r = (r > fadeR) ? (r - fadeR) : 0;
                    g = (g > fadeG) ? (g - fadeG) : 0;
                    b = (b > fadeB) ? (b - fadeB) : 0;

                    *dest = (r << 5) | (g << 2) | b;
                }
                dest++;
                sour++;
            }
            dest = savePos + frameWidth;
            sour = savePosImg + _width;
        }        
    }
}


//Draw functions-----------------------------------------------------------------------------------------------
void VGA_Sprite::cls(uint16_t col) { 
    if (!_created) return;

    if (_vga.getColorBits() == 16) {
        uint16_t* img = _img16;
        uint16_t* buff = (uint16_t*)heap_caps_malloc(_width * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
        assert(buff);

        std::fill_n(buff, _width, col);

        for (int i = 0; i < _height; i++) {
            memcpy(img, buff, _width * sizeof(uint16_t));
            img += _width;
        }

        heap_caps_free(buff);
    } else {
        memset(_img, (uint8_t)col, _size);
    }
}

void VGA_Sprite::putPixel(int x, int y, uint16_t col) {
    if (x < 0 || y < 0 || x >= _width || y >= _height || !_created) return;

    if (_vga.getColorBits() == 16) {
        _img16[_width * y + x] = col;  
    } else {
        _img[_width * y + x] = (uint8_t) col;  
    }
}


void VGA_Sprite::hLine(int x1, int y, int x2, uint16_t col) {
    if (x2 < x1) std::swap(x1, x2);
    if (x2 < 0 || y < 0 || x1 >= _width || y >= _height || !_created) return;

    x1 = std::max(x1, 0);
    x2 = std::min(x2, _width - 1);
    int size = x2 - x1 + 1; 

    if (_vga.getColorBits() == 16) {
        std::fill_n(_img16 + _width * y + x1, size, col);
    } else {
        memset(_img + _width * y + x1, (uint8_t)col, size);
    }
}


void VGA_Sprite::vLine(int x, int y1, int y2, uint16_t col) {
    if (y2 < y1) std::swap(y1, y2);
    if (x < 0 || x >= _width || y1 >= _height || y2 < 0 || !_created) return;

    y1 = std::max(y1, 0);
    y2 = std::min(y2, _height - 1);
    int size = y2 - y1 + 1;

    if (_vga.getColorBits() == 16) {
        uint16_t* ptr = _img16 + _width * y1 + x; // Создаём локальный указатель
        while (size-- > 0) {
            *ptr = col;
            ptr += _width;
        }
    } else {
        uint8_t* ptr = _img + _width * y1 + x; // Локальный указатель
        while (size-- > 0) {
            *ptr = (uint8_t)col;
            ptr += _width;
        }
    }
}

void VGA_Sprite::rect(int x1, int y1, int x2, int y2, uint16_t col){
    hLine(x1, y1, x2, col);  // Верхняя граница
    hLine(x1, y2, x2, col);  // Нижняя граница
    vLine(x1, y1, y2, col);  // Левая граница
    vLine(x2, y1, y2, col);  // Правая граница
}

/*moj kod++
void VGA_Sprite::putImage(int x, int y) {
    if (!_created) return;

    int xx = _width - 1;  int x2 = x + xx;
    int yy = _height - 1; int y2 = y + yy;    

    if ((x > _vga.getvX2()) || (y > _vga.getvY2()) || 
        (x2 < _vga.getvX1()) || (y2 < _vga.getvY1())) 
    return;

    int dx = (x < _vga.getvX1()) ? _vga.getvX1() : x;
    int dy = (y < _vga.getvY1()) ? _vga.getvY1() : y;
    uint8_t* dest = &_vga.getDrawBuffer()[_vga.frameWidth() * dy + dx];

    int sx = (x < _vga.getvX1()) ? _vga.getvX1() - x : 0;
    int sy = (y < _vga.getvY1()) ? _vga.getvY1() - y : 0;
    uint8_t* sour = (uint8_t*) &_img[_width * sy + sx];

    int slx = (x2 > _vga.getvX2()) ? x2 - _vga.getvX2() : 0;
    int sly = (y2 > _vga.getvY2()) ? y2 - _vga.getvY2() : 0;

    int sizex = _width - sx - slx;
    int sizey = _height - sy - sly;

    while (sizey-- > 0){
        memcpy(dest, sour, sizex);
        dest += _vga.frameWidth();
        sour += _width;
    }
}
*/
