#include "VGA_Font.h"
#include "Font/font_8x8.h"

VGA_Font::VGA_Font(VGA_esp32s3& vga) : _vga(vga) {
    _charWidth  = pgm_read_byte(_font8x8);      // 1-й байт 
    _xx = _charWidth - 1;
    _charHeight = pgm_read_byte(_font8x8 + 1);  // 2-й байт
    _yy = _charHeight - 1;

    // Всего символов: 0x7F - 0x20 + 1 = 96 символов
    // Загружаем _font из PROGMEM в SPIRAM
    int size = 95 * 8;
    _font = (uint8_t*)heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
    assert(_font);

    memcpy_P(_font, _font8x8 + 2, size);  // пропускаем 2 байта

    _size = size;
}

VGA_Font::~VGA_Font() {
    if (_font) {
        free(_font);
        _font = nullptr;
    }
}

void VGA_Font::setFont(const uint8_t* font) {
    if (_font != nullptr) {
        free(_font);
    }

    _charWidth = pgm_read_byte(font);
    _charHeight = pgm_read_byte(font + 1);

    // Всего символов: от U+0020 до U+007F (96 штук)
    const int glyphs = 0x7F - 0x20 + 1;
    const int totalSize = 2 + glyphs * _charHeight;

    // Копируем шрифт в RAM
    uint8_t* ramFont = (uint8_t*)heap_caps_malloc(totalSize, MALLOC_CAP_SPIRAM);
    assert(ramFont);

    for (int i = 0; i < totalSize; ++i) {
        ramFont[i] = pgm_read_byte(font + i);
    }

    _font = ramFont;
}

void VGA_Font::putChar(int x, int y, char ch, uint16_t col, bool trans, uint16_t bgCol){
    if (!_font || ch < 0x20 || ch > 0x7E) return;
    if (x < _vga.getvX1() || y < _vga.getvY1() || x + _charHeight > _vga.getvX2() || y + _charHeight > _vga.getvY2()) return;

    int index = (ch - 0x20) << 3;
    int width = _vga.frameWidth();
    uint8_t* dest = _vga.getDrawBuffer();
    uint8_t* sour = _font + index;

    dest += (width == 320 ? _math.fastY_320[y] : _math.fastY_640[y]) + x;
    int sizeY = _charHeight;
    int skip = width - _charWidth;

    while (sizeY-- > 0){
        uint8_t bit = *sour++;
        int sizeX = _charWidth;
        while (sizeX-- > 0){
            *dest++ = (bit & 1) ? col : (trans ? *dest : bgCol);            
            bit >>= 1;
        }
        dest += skip;
    }
}

void VGA_Font::putInt(int x, int y, int num, uint16_t col, bool trans, uint16_t bgCol) {
    char buf[12];  // Достаточно для любого int32: "-2147483648"
    itoa(num, buf, 10);  // Преобразуем число в строку

    const char* p = buf;
    while (*p) {
        x += 8;
        putChar(x, y, *p++, col, trans, bgCol);
    }
}

void VGA_Font::putFloat(int x, int y, float num, uint16_t col, bool trans, uint16_t bgCol) {
    char buf[20];  // Достаточно для "12345.6789" и подобного
    dtostrf(num, 0, 2, buf);  // (значение, минимальная ширина, точность, буфер)
    // 0 — без выравнивания, 2 — 2 цифры после точки

    const char* p = buf;
    while (*p) {
        x += 8;
        putChar(x, y, *p++, col, trans, bgCol);
    }
}


void VGA_Font::drawChar(int x, int y, char c, uint16_t col) {
    if (!_font || c < 0x20 || c > 0x7E) return;

    int index = c - 0x20;
    int width = _vga.frameWidth();
    uint8_t* scr = _vga.getDrawBuffer();

    for (int row = 0; row < 8; row++) {
        uint8_t bits = pgm_read_byte(&_font8x8[2 + index * 8 + row]);

        for (int bit = 0; bit < 8; bit++) {
            if (bits & (1 << bit)) {  // теперь читаем слева (LSB) направо
                int px = x + bit;  // инверсия по X
                //int px = x + (7 - bit);  // или _charWidth - 1 - bit
                int py = y + row;
                if (px >= 0 && py >= 0 && px < width && py < _vga.frameHeight()) {
                    scr[py * width + px] = col;
                }
            }
        }
    }
}

void VGA_Font::drawText(int x, int y, const char* text, uint16_t color) {
    while (*text) {
        drawChar(x, y, *text++, color);
        x += _charWidth;  // Отступ между символами
    }
}
