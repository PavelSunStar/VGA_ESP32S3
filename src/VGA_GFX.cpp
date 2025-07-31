#include <Arduino.h>
#include "VGA_GFX.h"

VGA_GFX::VGA_GFX(VGA_esp32s3& vga) : _vga(vga) {
    // Конструктор, инициализация ссылки на объект VGA_esp32s3    
}

VGA_GFX::~VGA_GFX() {
    // Деструктор (если нужно освободить ресурсы)
}

void VGA_GFX::init(){
    for (int i = 0; i < 600; i++) virtualY[i] = _vga.frameWidth() * i;
}

uint16_t VGA_GFX::getColor(uint8_t r, uint8_t g, uint8_t b) {
    switch (_vga.getColorBits()) {
    case 16:
        return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3); // RGB565
    case 8:
        return ((r >> 5) << 5) | ((g >> 5) << 2) | (b >> 6); // RGB332
    default:
        return 0;
    }
}

uint8_t VGA_GFX::bright8(uint8_t col, int8_t brightness) {
    if (brightness == 0) return col;

    uint8_t r = (col >> 5) & 0b111;
    uint8_t g = (col >> 2) & 0b111;
    uint8_t b =  col       & 0b11;

    if (brightness > 0) {
        // Осветляем — логика через __builtin_clz
        uint8_t add = __builtin_clz((uint8_t)brightness) - 24;
        r = (r + add > 7) ? 7 : r + add;
        g = (g + add > 7) ? 7 : g + add;
        b = (b + add > 3) ? 3 : b + add;
    } else {
        // Затемняем
        uint8_t sub = __builtin_clz((uint8_t)(-brightness)) - 24;
        r = (r < sub) ? 0 : r - sub;
        g = (g < sub) ? 0 : g - sub;
        b = (b < sub) ? 0 : b - sub;
    }

    return (r << 5) | (g << 2) | b;
}

/*
uint8_t VGA_GFX::setBrightness8(uint8_t col, int8_t brightness) {
    if (brightness < 0x20) return col; // не трогаем

    uint8_t r = (col >> 5) & 0b111;
    uint8_t g = (col >> 2) & 0b111;
    uint8_t b =  col       & 0b11;

    int8_t bla;

    // Осветляем
    uint8_t add = __builtin_clz(brightness) - 24; // 32-битное число
    r = (r + add > 7) ? 7 : r + add;
    g = (g + add > 7) ? 7 : g + add;
    b = (b + add > 3) ? 3 : b + add;

    return (r << 5) | (g << 2) | b;
}
*/

void VGA_GFX::cls(uint16_t col) { 
    if (_vga.getColorBits() == 16) {
        uint16_t* scr = &_vga.getDrawBuffer16()[_vga.frameWidth() * _vga.getvY1() + _vga.getvX1()];
        uint16_t* buff = (uint16_t*)heap_caps_malloc(_vga.getvWidth() * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
        if (!buff) return;  // Проверка выделения памяти

        for (int x = 0; x < _vga.getvWidth(); x++) buff[x] = col;

        int lines = _vga.getvHeight();
        int copyBytes = _vga.getvWidth() * sizeof(uint16_t);
        int skip = _vga.frameWidth();
        while (lines-- > 0) {
            memcpy(scr, buff, copyBytes);
            scr += skip;
        }

        heap_caps_free(buff);  // Освобождаем память
    } 
    else if (_vga.getColorBits() == 8) {
        int size = _vga.frameWidth() * _vga.frameHeight();
        memset(_vga.getDrawBuffer(), (uint8_t)col, size);
    }
}

void VGA_GFX::putPixel(int x, int y, uint16_t col) {
    if (x > _vga.getvX2() || y > _vga.getvY2() || x < _vga.getvX1() || y <_vga.getvY1()) return;

    int* fastY = (_vga.frameWidth() == 640) ? _math.fastY_640 : _math.fastY_320;
    if (_vga.getColorBits() == 16){
        uint16_t* scr = _vga.getDrawBuffer16() + fastY[y] + x;
        *scr = col;
    } else {
        uint8_t* scr = _vga.getDrawBuffer() + fastY[y] + x;
        *scr = (uint8_t)(col & 0xFF);
    }
}

void VGA_GFX::hLine(int x1, int y, int x2, uint16_t col){
    if (x2 < x1) std::swap(x1, x2);

    int xx1 = _vga.getvX1();
    int xx2 = _vga.getvX2();

    if (x1 > xx2 || y > _vga.getvY2() || x2 < xx1 || y < _vga.getvY1()) return;   

    x1 = std::max(x1, xx1);
    x2 = std::min(x2, xx2);
    int size = x2 - x1 + 1;
    int* fastY = (_vga.frameWidth() == 640) ? _math.fastY_640 : _math.fastY_320;

    if (_vga.getColorBits() == 16){
        uint16_t* scr = _vga.getDrawBuffer16() + fastY[y] + x1; 
        while (size-- > 0) *scr++ = col;                     
    } else { 
        uint8_t* scr = _vga.getDrawBuffer() + fastY[y] + x1;
        memset(scr, col, size);                         
    }        
}

void VGA_GFX::vLine(int x, int y1, int y2, uint16_t col){
    if (y2 < y1) std::swap(y1, y2);

    int yy1 = _vga.getvY1();
    int yy2 = _vga.getvY2();

    if (x > _vga.getvX2() || y1 > yy2 || x < _vga.getvX1() || y2 < yy1) return;    

    y1 = std::max(y1, yy1);
    y2 = std::min(y2, yy2);
    int size = y2 - y1 + 1;    
    int width = _vga.frameWidth();
    int* fastY = (_vga.frameWidth() == 640) ? _math.fastY_640 : _math.fastY_320;

    if (_vga.getColorBits() == 16) {
        uint16_t* scr = _vga.getDrawBuffer16() + fastY[y1] + x;
        while (size-- > 0){
            *scr = col;
            scr += width;
        }    
    } else {
        uint8_t* scr = _vga.getDrawBuffer() + fastY[y1] + x;
        uint8_t color = (uint8_t)(col & 0xFF);
        

        while (size-- > 0){
            *scr = color;
            scr += width;
        }                
    }           
}

void VGA_GFX::rect(int x1, int y1, int x2, int y2, uint16_t col){
    if (x2 < x1) std::swap(x1, x2);
    if (y2 < y1) std::swap(y1, y2);
    
    int xx1 = _vga.getvX1();
    int yy1 = _vga.getvY1();
    int xx2 = _vga.getvX2();
    int yy2 = _vga.getvY2();

    if (x1 > xx2 || y1 > yy2 || x2 < xx1 || y2 < yy1) return;

    int size = x2 - x1;
    int sizex = _vga.frameWidth();
    int sizey = y2 - y1 - 1;
    int skip = sizex - x2 + x1;

    if ((x1 >=  xx1) && (y1 >= yy1) && (x2 <= xx2) && (y2 <= yy2)){        
        switch (_vga.getColorBits()){
            case 8:{
                uint8_t* scr = _vga.getDrawBuffer();
                scr += virtualY[y1];
                scr += x1;
                memset(scr, col, size + 1);                 
                scr += sizex;
                while (sizey-- > 0){
                    *scr = col;
                    scr += size;
                    *scr = col;
                    scr += skip;
                }      
                memset(scr, col, size + 1);               
                break;
            }  
            case 16:{

            }                     
        }
    } else {
        hLine(x1, y1, x2, col);  // Верхняя граница
        hLine(x1, y2, x2, col);  // Нижняя граница
        vLine(x1, y1, y2, col);  // Левая граница
        vLine(x2, y1, y2, col);  // Правая граница
    }    
}

void VGA_GFX::fillRect(int x1, int y1, int x2, int y2, uint16_t col) {
    if (x2 < x1) std::swap(x1, x2); 
    if (y2 < y1) std::swap(y1, y2);
    
    int xx1 = _vga.getvX1();
    int yy1 = _vga.getvY1();
    int xx2 = _vga.getvX2();
    int yy2 = _vga.getvY2();

    if (x1 > xx2 || y1 > yy2 || x2 < xx1 || y2 < yy1) return;

    if (x1 < xx1) x1 = xx1;
    if (y1 < yy1) y1 = yy1;    
    if (x2 > xx2) x2 = xx2;
    if (y2 > yy2) y2 = yy2;

    int size = x2 - x1 + 1; 
    int lines = y2 - y1 + 1;
    int skip = _vga.frameWidth();

    switch (_vga.getColorBits()){
        case 8:{
            uint8_t* scr = _vga.getDrawBuffer();
            scr += virtualY[y1] + x1;
            
            while (lines-- > 0) {
                memset(scr, col, size);
                scr += skip;
            }

            break;
        }

        case 16:{
            uint16_t* scr = &_vga.getDrawBuffer16()[_vga.frameWidth() * y1 + x1];
            uint16_t* buff = (uint16_t*)heap_caps_malloc(size << 1, MALLOC_CAP_SPIRAM);
            if (!buff) return;  // Проверка на выделение памяти

            for (int x = 0; x < size; x++) buff[x] = col;

            while (lines-- > 0) {
                memcpy(scr, buff, size << 1);  // Копируем DMA-оптимизированным способом
                scr += skip;
            }

            heap_caps_free(buff);  // Освобождаем память 
            break;
        }
    }        
}

void VGA_GFX::line(int x1, int y1, int x2, int y2, uint16_t col){
    if (x1 == x2) {
        vLine(x1, y1, y2, col);
    } else if (y1 == y2){
        hLine(x1, y1, x2, col);
    } else {
        // Вычисляем разницу по осям X и Y
        int dx = abs(x2 - x1);
        int dy = abs(y2 - y1);   

       // Определяем направление движения по осям
        int sx = (x1 < x2) ? 1 : -1;
        int sy = (y1 < y2) ? 1 : -1;
    
        // Инициализируем ошибку
        int err = dx - dy;   

        int cb = _vga.getColorBits();
        uint16_t* scr16 = (uint16_t*)_vga.getDrawBuffer16();
        uint8_t* scr8 = _vga.getDrawBuffer();
        uint8_t* null8 = scr8;
        uint16_t* null16 = scr16;

        int xx1 = _vga.getvX1();
        int yy1 = _vga.getvY1();
        int xx2 = _vga.getvX2();
        int yy2 = _vga.getvY2();

        while (true) {
            // Проверяем, если текущий пиксель в пределах экрана
            if (x1 >= xx1 && x1 <= xx2 && y1 >= yy1 && y1 < yy2){
                int index = virtualY[y1] + x1;                

                if (cb == 16) {
                    scr16 = null16 + index;
                    *scr16 = col;
                } else if (cb == 8) {
                    scr8 = null8 + index;
                    *scr8 = col;
                }
            }

            // Если достигли конечной точки, выходим
            if (x1 == x2 && y1 == y2) break;
        
            // Вычисляем наибольшую ошибку и корректируем координаты
            int e2 = err << 1;
            if (e2 > -dy) {
                err -= dy;
                x1 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y1 += sy;
            }            
        }    
    }       
}

void VGA_GFX::lineAngle(int x, int y, int len, int angle, uint16_t col){
    int x1 = _math.xLUT(x, y, len, angle);
    int y1 = _math.yLUT(x, y, len, angle);
    line(x, y, x1, y1, col);
}

void VGA_GFX::triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col){
    line(x1, y1, x2, y2, col);
    line(x2, y2, x3, y3, col);
    line(x3, y3, x1, y1, col);
}

void VGA_GFX::fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col){
    if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); }
    if (y2 > y3) { std::swap(x2, x3); std::swap(y2, y3); }

    int totalHeight = y3 - y1;
    for (int y = y1; y <= y3; y++) {
        bool secondHalf = y > y2 || y2 == y1;
        int segmentHeight = secondHalf ? y3 - y2 : y2 - y1;
        float alpha = (float)(y - y1) / totalHeight;
        float beta  = (float)(y - (secondHalf ? y2 : y1)) / segmentHeight;
        int ax = x1 + (x3 - x1) * alpha;
        int bx = secondHalf ? x2 + (x3 - x2) * beta : x1 + (x2 - x1) * beta;
        if (ax > bx) std::swap(ax, bx);
        hLine(ax, y, bx, col);
    }
}

void VGA_GFX::circle(int x, int y, int r, uint16_t col){
    if (r <= 0) return;

    int f     = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x0    = 0;
    int y0    = r;

    // Рисуем начальные точки окружности
    putPixel(x, y + r, col);
    putPixel(x, y - r, col);
    putPixel(x + r, y, col);
    putPixel(x - r, y, col);

    while (x0 < y0) {
        if (f >= 0) {
            y0--;
            ddF_y += 2;
            f += ddF_y;
        }

        putPixel(x + x0, y + y0, col);
        putPixel(x - x0, y + y0, col);
        putPixel(x + x0, y - y0, col);
        putPixel(x - x0, y - y0, col);
        
        putPixel(x + y0, y + x0, col);
        putPixel(x - y0, y + x0, col);
        putPixel(x + y0, y - x0, col);
        putPixel(x - y0, y - x0, col);

        x0++;
        ddF_x += 2;
        f += ddF_x;
    }
}

void VGA_GFX::fillCircle(int x, int y, int r, uint16_t col){
    if (r <= 0) return;

    int f     = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x0     = 0;
    int y0     = r;

    hLine(x - r, y, x + r, col); // Центральная горизонтальная линия

    while (x0 <= y0) {
        if (f >= 0) {
            y0--;
            ddF_y += 2;
            f += ddF_y;
        }
        x0++;
        ddF_x += 2;
        f += ddF_x;

        hLine(x - x0, y + y0, x + x0, col); 
        hLine(x - x0, y - y0, x + x0, col); 
        if (x0 != y0) { // Убедимся, что не рисуем те же линии дважды        
            hLine(x - y0, y + x0, x + y0, col); 
            hLine(x - y0, y - x0, x + y0, col);  
        }                  
    }
}

void VGA_GFX::polygon(int x, int y, int radius, int sides, float rotation, uint16_t col) {
    if (sides < 3) return; // Минимум 3 стороны

    float angleStep = 2 * M_PI / sides;
    int x1 = x + radius * cos(rotation);
    int y1 = y + radius * sin(rotation);

    for (int i = 1; i <= sides; i++) {
        float angle = i * angleStep + rotation;
        int x2 = x + radius * cos(angle);
        int y2 = y + radius * sin(angle);

        line(x1, y1, x2, y2, col);
        x1 = x2;
        y1 = y2;
    }
}

void VGA_GFX::fillPolygon(int x, int y, int radius, int sides, float rotation, uint16_t col) {
    if (sides < 3) return; // Минимум 3 стороны

    struct Edge {
        int yMin, yMax;
        float x, slope;
    };

    float angleStep = 2 * M_PI / sides;
    int vx[sides], vy[sides];

    // Вычисляем координаты вершин
    for (int i = 0; i < sides; i++) {
        float angle = i * angleStep + rotation;
        vx[i] = x + radius * cos(angle);
        vy[i] = y + radius * sin(angle);
    }

    // Формируем список рёбер
    std::vector<Edge> edges;
    for (int i = 0; i < sides; i++) {
        int x1 = vx[i], y1 = vy[i];
        int x2 = vx[(i + 1) % sides], y2 = vy[(i + 1) % sides];

        if (y1 == y2) continue; // Пропускаем горизонтальные линии

        if (y1 > y2){
            std::swap(x1, x2);
            std::swap(y1, y2);
        }
        edges.push_back({ y1, y2, (float)x1, (float)(x2 - x1) / (y2 - y1) });
    }

    // Сортируем рёбра по yMin
    std::sort(edges.begin(), edges.end(), [](const Edge &a, const Edge &b) {
        return a.yMin < b.yMin;
    });

    // Заполняем многоугольник
    int yMin = edges.front().yMin;
    int yMax = edges.back().yMax;
    
    std::vector<Edge> activeEdges;
    for (int y = yMin; y <= yMax; y++) {
        // Добавляем активные рёбра
        for (const auto &e : edges) {
            if (e.yMin == y) activeEdges.push_back(e);
        }

        // Удаляем рёбра, для которых yMax достигнут
        activeEdges.erase(std::remove_if(activeEdges.begin(), activeEdges.end(), 
                        [y](const Edge &e) { return e.yMax <= y; }), activeEdges.end());

        // Обновляем X-координаты
        for (auto &e : activeEdges) e.x += e.slope;

        // Сортируем активные рёбра по X
        std::sort(activeEdges.begin(), activeEdges.end(), [](const Edge &a, const Edge &b) {
            return a.x < b.x;
        });

        // Заполняем горизонтальные линии
        for (size_t i = 0; i < activeEdges.size(); i += 2) {
            if (i + 1 >= activeEdges.size()) break;
            hLine((int)activeEdges[i].x, y, (int)activeEdges[i + 1].x, col);
        }
    }
}

void VGA_GFX::fog(int x1, int y1, int x2, int y2) {
    if (_vga.getColorBits() == 16) {
        uint16_t* scr = _vga.getDrawBuffer16();
        scr += x1;
        uint16_t* line = scr + y1 * _vga.frameWidth();

        for (int y = y1; y < y2; y++) {
            uint16_t* curr = line;
            uint16_t* next = line + _vga.frameWidth();
            curr++; // сдвигаем указатель на x = x1 + 1

            for (int x = x1; x < x2; x++, curr++, next++) {
                uint16_t c0 = *curr;
                uint16_t c1 = *(next);
                uint16_t c2 = *(next - 1);
                uint16_t c3 = *(next + 1);

                int r = (((c0 >> 11) & 0x1F) + ((c1 >> 11) & 0x1F) + (((c2 >> 11) & 0x1F) + ((c3 >> 11) & 0x1F)) / 2) / 3;
                int g = (((c0 >> 5) & 0x3F) + ((c1 >> 5) & 0x3F) + (((c2 >> 5) & 0x3F) + ((c3 >> 5) & 0x3F)) / 2) / 3;
                int b = ((c0 & 0x1F) + (c1 & 0x1F) + ((c2 & 0x1F) + (c3 & 0x1F)) / 2) / 3;

                *curr = (r << 11) | (g << 5) | b;
            }

            line += _vga.frameWidth(); // переход на следующую строку
        }
    } else if (_vga.getColorBits() == 8){
        uint8_t* scr = _vga.getDrawBuffer();
        scr += x1;
        uint8_t* line = scr + y1 * _vga.frameWidth();

        for (int y = y1; y < y2; y++) {
            uint8_t* curr = line;
            uint8_t* next = line + _vga.frameWidth();
            curr++; // сдвигаем указатель на x = x1 + 1

            for (int x = x1; x < x2; x++, curr++, next++) {
                uint8_t c0 = *curr;
                uint8_t c1 = *(next);
                uint8_t c2 = *(next - 1);
                uint8_t c3 = *(next + 1);

                int r = (((c0 >> 5) & 0x07) + ((c1 >> 5) & 0x07) + (((c2 >> 5) & 0x07) + ((c3 >> 5) & 0x07)) / 2) / 3;
                int g = (((c0 >> 2) & 0x07) + ((c1 >> 2) & 0x07) + (((c2 >> 2) & 0x07) + ((c3 >> 2) & 0x07)) / 2) / 3;
                int b = ((c0 & 0x03) + (c1 & 0x03) + ((c2 & 0x03) + (c3 & 0x03)) / 2) / 3;
                
                *curr = (r << 5) | (g << 2) | b;  // Исправленный 8-битный формат
            }

            line += _vga.frameWidth(); // переход на следующую строку
        }
    }
}

void VGA_GFX::scrollScr(int x, int y) {
    if (x == 0 && y == 0) return; // Нет необходимости скроллить 

    int _scrWidth = _vga.frameWidth();
    int _scrHeight = _vga.frameHeight();
    int xx = x % _scrWidth;
    int yy = y % _scrHeight;

    uint8_t *_dest = _vga.getDrawBuffer();

    if (_vga.getColorBits() == 16) {
        // Поддержка 16-битного режима не реализована
    } else {
        uint8_t *_buff = (uint8_t *)heap_caps_malloc(_scrWidth * yy, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (!_buff) return; // Проверяем выделение памяти

        // --- Вертикальный скроллинг ---
        if (yy > 0) {
            memcpy(_buff, _dest, _scrWidth * yy);  // Сохраняем верхние строки
            memmove(_dest, _dest + (_scrWidth * yy), _scrWidth * (_scrHeight - yy));  // Сдвигаем вверх
            memcpy(_dest + (_scrWidth * (_scrHeight - yy)), _buff, _scrWidth * yy);  // Вставляем сохранённые строки в конец
        }

        // --- Горизонтальный скроллинг ---
        uint8_t *row_buff = (uint8_t *)heap_caps_malloc(_scrWidth, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (row_buff) {
            for (int i = 0; i < _scrHeight; i++) {
                memcpy(row_buff, _dest, _scrWidth);  // Копируем строку перед сдвигом
                memmove(_dest, _dest + xx, _scrWidth - xx);
                memcpy(_dest + (_scrWidth - xx), row_buff, xx);  // Копируем сдвинутые байты в конец
                _dest += _scrWidth;
            }
            heap_caps_free(row_buff);
        }

        heap_caps_free(_buff);
    }
}




/*
void VGA_GFX::init(){
    _scrWidth = _vga.getWidth();
    _scrHeight = _vga.getHeight();
    _scrSize = _vga.getSize();
    _scrBytesPerPixel = _vga.getBytesPerPixel();
    _scrColorBits = _vga.getColorBits();

    _vX1 = 0;
    _vY1 = 0;
    _vX2 = _scrWidth - 1;
    _vY2 = _scrHeight - 1;
    _vWidth = _vX2 - _vX1 + 1;
    _vHeight = _vY2 - _vY1 + 1;
}

//8 bit graphics-----------------------------------------------------------------------
uint16_t VGA_GFX::getColor(uint8_t r, uint8_t g, uint8_t b) {
    if (_scrColorBits == 16) {
        uint8_t rr = map(r, 0, 256, 0, 32);
        uint8_t gg = map(g, 0, 256, 0, 64);
        uint8_t bb = map(b, 0, 256, 0, 32);
        return (rr << 11) | (gg << 5) | bb; // Вернём 16-битный цвет
    } 
    
    if (_scrColorBits == 8) {
        uint8_t col = map(r, 0, 256, 0, 8) << 5;
        col |= map(g, 0, 256, 0, 8) << 2;
        col |= map(b, 0, 256, 0, 4);
        return col; // Вернём 8-битный цвет
    }

    return 0; // Возвращаем 0, если ни один из режимов не подошёл
}


void VGA_GFX::cls(uint16_t col){ 
    if (_scrColorBits == 16){
        uint16_t* scr = &_vga.getBuff16()[640 * _vY1 + _vX1];
        uint16_t* dma_buff = (uint16_t*)heap_caps_malloc(_vWidth << 1, MALLOC_CAP_DMA);

        if (!dma_buff) return;  // Проверка на выделение памяти

        for (int i = _vX1; i <= _vX2; i++) dma_buff[i] = col;
        int lines = _vHeight;

        while (lines-- > 0){
            memcpy(scr, dma_buff, _vWidth << 1);
            scr += _vWidth;
        }

        heap_caps_free(dma_buff);  // Освобождаем память
    } else if (_scrColorBits == 8){
        uint8_t* scr = &_vga.getBuff()[640 * _vY1 + _vX1]; 
        int lines = _vHeight;
        
        while (lines-- > 0){
            memset(scr, (uint8_t)col, _vWidth);
            scr += _vWidth;
        }    
    }
}

void VGA_GFX::putPixel(int x, int y, uint16_t col) {
    if (x < _vX1 || y < _vY1 || x > _vX2 || y > _vY2) return;

    if (_scrColorBits == 16) {
        uint16_t* scr = &_vga.getBuff16()[_scrWidth * y + x];
        *scr = col;
    } else {
        uint8_t* scr = &_vga.getBuff()[_scrWidth * y + x];
        *scr = (uint8_t)col;
    }
}


void VGA_GFX::hLine(int x1, int y, int x2, uint16_t col){
    if (x2 < x1) swap(x1, x2);
    if (y < _vY1 || y > _vY2 || x2 < _vX1 || x1 > _vX2) return;   

    x1 = max(_vX1, x1);
    x2 = min(_vX2, x2);
    int size = x2 - x1;

    if (_scrColorBits == 16){
        uint16_t* scr = &_vga.getBuff16()[_scrWidth * y + x1];
        while (size-- > 0) *scr++ = col;         
    } else if (_scrColorBits == 8){
        uint8_t* scr = &_vga.getBuff()[_scrWidth * y + x1]; 
        memset(scr, (uint8_t)col, size);
    }    
}

void VGA_GFX::vLine(int x, int y1, int y2, uint16_t col){
    if (y2 < y1) swap(y1, y2);
    if (x < _vX1 || x > _vX2 || y2 < _vY1 || y1 >= _vY2) return;    

    y1 = max(_vY1, y1);
    y2 = min(_vY2, y2);
    int size = y2 - y1;

    if (_scrColorBits == 16) {
        uint16_t* scr = &_vga.getBuff16()[_scrWidth * y1 + x];
        while (size-- >= 0){
            *scr = col;
            scr += _scrWidth;
        }    
    } else if (_scrColorBits == 8){
        uint8_t* scr = &_vga.getBuff()[_scrWidth * y1 + x];
        while (size-- >= 0){
            *scr = (uint8_t)col;
            scr += _scrWidth;
        }    
    }    
}

void VGA_GFX::line(int x1, int y1, int x2, int y2, uint16_t col){
    if (x1 == x2) {
        vLine(x1, y1, y2, col);
    } else if (y1 == y2){
        hLine(x1, y1, x2, col);
    } else {
        // Вычисляем разницу по осям X и Y
        int dx = abs(x2 - x1);
        int dy = abs(y2 - y1);   

       // Определяем направление движения по осям
        int sx = (x1 < x2) ? 1 : -1;
        int sy = (y1 < y2) ? 1 : -1;
    
        // Инициализируем ошибку
        int err = dx - dy;   

        uint16_t* scr16 = (uint16_t*)_vga.getBuff16();
        uint8_t* scr8 = _vga.getBuff();
        uint8_t* null8 = scr8;
        uint16_t* null16 = scr16;

        while (true) {
            // Проверяем, если текущий пиксель в пределах экрана
            if (x1 >= _vX1 && x1 <= _vX2 && y1 >= _vY1 && y1 <= _vY2) {
                int index = _scrWidth * y1 + x1;

                if (_scrColorBits == 16) {
                    scr16 = null16 + index;
                    *scr16 = col;
                } else if (_scrColorBits == 8) {
                    scr8 = null8 + index;
                    *scr8 = (uint8_t)col;
                }
            }

            // Если достигли конечной точки, выходим
            if (x1 == x2 && y1 == y2) break;
        
            // Вычисляем наибольшую ошибку и корректируем координаты
            int e2 = err * 2;
            if (e2 > -dy) {
                err -= dy;
                x1 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y1 += sy;
            }            
        }    
    }       
}

void VGA_GFX::lineAngle(int x, int y, int len, int angle, uint16_t col){
    if (len <= 0) return;

    angle %= 360;
    if (angle < 0) angle = 360 + angle;
    int x1 = x + len * cosLUT[angle];
    int y1 = y + len * sinLUT[angle];

    line(x, y, x1, y1, col);
}

void VGA_GFX::rect(int x1, int y1, int x2, int y2, uint16_t col){
    hLine(x1, y1, x2, col);  // Верхняя граница
    hLine(x1, y2, x2, col);  // Нижняя граница
    vLine(x1, y1, y2, col);  // Левая граница
    vLine(x2, y1, y2, col);  // Правая граница
}

void VGA_GFX::fillRect(int x1, int y1, int x2, int y2, uint16_t col) {
    if (x2 < x1) swap(x1, x2);
    if (y2 < y1) swap(y1, y2);
    if (x1 > _vX2 || y1 > _vY2 || x2 < _vX1 || y2 < _vY1) return;

    x1 = max(_vX1, x1);
    x2 = min(_vX2, x2);
    y1 = max(_vY1, y1);
    y2 = min(_vY2, y2);
    int size = x2 - x1 + 1;
    int lines = y2 - y1 + 1;

    if (_scrColorBits == 16) {
        uint16_t* scr = &_vga.getBuff16()[_scrWidth * y1 + x1];
        uint16_t* dma_buff = (uint16_t*)heap_caps_malloc(size * sizeof(uint16_t), MALLOC_CAP_DMA);
        if (!dma_buff) return;  // Проверка на выделение памяти

        for (int i = 0; i < size; i++) dma_buff[i] = col;

        while (lines-- > 0) {
            memcpy(scr, dma_buff, size << 1);  // Копируем DMA-оптимизированным способом
            scr += _scrWidth;
        }

        heap_caps_free(dma_buff);  // Освобождаем память
    } else if (_scrColorBits == 8) {
        uint8_t* scr = &_vga.getBuff()[_scrWidth * y1 + x1];
        while (lines-- > 0) {
            memset(scr, (uint8_t)col, size);
            scr += _scrWidth;
        }
    }
}



void VGA_GFX::fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col){
    if (y1 > y2) { swap(x1, x2); swap(y1, y2); }
    if (y2 > y3) { swap(x2, x3); swap(y2, y3); }

    int totalHeight = y3 - y1;
    for (int y = y1; y <= y3; y++) {
        bool secondHalf = y > y2 || y2 == y1;
        int segmentHeight = secondHalf ? y3 - y2 : y2 - y1;
        float alpha = (float)(y - y1) / totalHeight;
        float beta  = (float)(y - (secondHalf ? y2 : y1)) / segmentHeight;
        int ax = x1 + (x3 - x1) * alpha;
        int bx = secondHalf ? x2 + (x3 - x2) * beta : x1 + (x2 - x1) * beta;
        if (ax > bx) swap(ax, bx);
        hLine(ax, y, bx, col);
    }
}

void VGA_GFX::circle(int x, int y, int r, uint16_t col){
    if (r <= 0) return;

    int f     = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x0    = 0;
    int y0    = r;

    uint16_t* scr16 = (uint16_t*)_vga.getBuff16();
    uint8_t* scr = _vga.getBuff();
    uint8_t* null8 = scr;
    uint16_t* null16 = scr16;
    int index;

    // Рисуем начальные точки окружности
    if (_scrColorBits == 16){
        if (x >= _vX1 && x <= _vX2 && (y + r) >= _vY1 && (y + r) <= _vY2){
            index = _scrWidth * (y + r) + x; scr16 += index; *scr16 = col; scr16 = null16; 
        }
        if (x >= _vX1 && x <= _vX2 && (y - r) >= _vY1 && (y - r) <= _vY2){
            index = _scrWidth * (y - r) + x; scr16 += index; *scr16 = col; scr16 = null16;
        }
        if ((x + r) >= _vX1 && (x + r) <= _vX2 && y >= _vY1 && y <= _vY2){
            index = _scrWidth * y + (x + r); scr16 += index; *scr16 = col; scr16 = null16;    
        }
        if ((x - r) >= _vX1 && (x - r) <= _vX2 && y >= _vY1 && y <= _vY2){
            index = _scrWidth * y + (x - r); scr16 += index; *scr16 = col; scr16 = null16;    
        }

        while (x0 < y0) {
            if (f >= 0) {
                y0--;
                ddF_y += 2;
                f += ddF_y;
            }
        
            if ((x + x0) >= _vX1 && (x + x0) <= _vX2 && (y + y0) >= _vY1 && (y + y0) <= _vY2){
                index = _scrWidth * (y + y0) + (x + x0); scr16 += index; *scr16 = col; scr16 = null16; 
            }
            if ((x - x0) >= _vX1 && (x - x0) <= _vX2 && (y + y0) >= _vY1 && (y + y0) <= _vY2){
                index = _scrWidth * (y + y0) + (x - x0); scr16 += index; *scr16 = col; scr16 = null16; 
            }
            if ((x + x0) >= _vX1 && (x + x0) <= _vX2 && (y - y0) >= _vY1 && (y - y0) <= _vY2){
                index = _scrWidth * (y - y0) + (x + x0); scr16 += index; *scr16 = col; scr16 = null16; 
            }
            if ((x - x0) >= _vX1 && (x - x0) <= _vX2 && (y - y0) >= _vY1 && (y - y0) <= _vY2){
                index = _scrWidth * (y - y0) + (x - x0); scr16 += index; *scr16 = col; scr16 = null16; 
            }

            if ((x + y0) >= _vX1 && (x + y0) <= _vX2 && (y + x0) >= _vY1 && (y + x0) <= _vY2){
                index = _scrWidth * (y + x0) + (x + y0); scr16 += index; *scr16 = col; scr16 = null16; 
            }
            if ((x - y0) >= _vX1 && (x - y0) <= _vX2 && (y + x0) >= _vY1 && (y + x0) <= _vY2){
                index = _scrWidth * (y + x0) + (x - y0); scr16 += index; *scr16 = col; scr16 = null16; 
            }
            if ((x + y0) >= _vX1 && (x + y0) <= _vX2 && (y - x0) >= _vY1 && (y - x0) <= _vY2){
                index = _scrWidth * (y - x0) + (x + y0); scr16 += index; *scr16 = col; scr16 = null16; 
            }
            if ((x - y0) >= _vX1 && (x - y0) <= _vX2 && (y - x0) >= _vY1 && (y - x0) <= _vY2){
                index = _scrWidth * (y - x0) + (x - y0); scr16 += index; *scr16 = col; scr16 = null16; 
            }

            x0++;
            ddF_x += 2;
            f += ddF_x;
        }
    } else if (_scrColorBits == 8){
        if (x >= _vX1 && x <= _vX2 && (y + r) >= _vY1 && (y + r) <= _vY2){
            index = _scrWidth * (y + r) + x; scr += index; *scr = col; scr = null8; 
        }
        if (x >= _vX1 && x <= _vX2 && (y - r) >= _vY1 && (y - r) <= _vY2){
            index = _scrWidth * (y - r) + x; scr += index; *scr = col; scr = null8;
        }
        if ((x + r) >= _vX1 && (x + r) <= _vX2 && y >= _vY1 && y <= _vY2){
            index = _scrWidth * y + (x + r); scr += index; *scr = col; scr = null8;    
        }
        if ((x - r) >= _vX1 && (x - r) <= _vX2 && y >= _vY1 && y <= _vY2){
            index = _scrWidth * y + (x - r); scr += index; *scr = col; scr = null8;    
        }

        while (x0 < y0) {
            if (f >= 0) {
                y0--;
                ddF_y += 2;
                f += ddF_y;
            }
        
            if ((x + x0) >= _vX1 && (x + x0) <= _vX2 && (y + y0) >= _vY1 && (y + y0) <= _vY2){
                index = _scrWidth * (y + y0) + (x + x0); scr += index; *scr = col; scr = null8; 
            }
            if ((x - x0) >= _vX1 && (x - x0) <= _vX2 && (y + y0) >= _vY1 && (y + y0) <= _vY2){
                index = _scrWidth * (y + y0) + (x - x0); scr += index; *scr = col; scr = null8; 
            }
            if ((x + x0) >= _vX1 && (x + x0) <= _vX2 && (y - y0) >= _vY1 && (y - y0) <= _vY2){
                index = _scrWidth * (y - y0) + (x + x0); scr += index; *scr = col; scr = null8; 
            }
            if ((x - x0) >= _vX1 && (x - x0) <= _vX2 && (y - y0) >= _vY1 && (y - y0) <= _vY2){
                index = _scrWidth * (y - y0) + (x - x0); scr += index; *scr = col; scr = null8; 
            }

            if ((x + y0) >= _vX1 && (x + y0) <= _vX2 && (y + x0) >= _vY1 && (y + x0) <= _vY2){
                index = _scrWidth * (y + x0) + (x + y0); scr += index; *scr = col; scr = null8; 
            }
            if ((x - y0) >= _vX1 && (x - y0) <= _vX2 && (y + x0) >= _vY1 && (y + x0) <= _vY2){
                index = _scrWidth * (y + x0) + (x - y0); scr += index; *scr = col; scr = null8; 
            }
            if ((x + y0) >= _vX1 && (x + y0) <= _vX2 && (y - x0) >= _vY1 && (y - x0) <= _vY2){
                index = _scrWidth * (y - x0) + (x + y0); scr += index; *scr = col; scr = null8; 
            }
            if ((x - y0) >= _vX1 && (x - y0) <= _vX2 && (y - x0) >= _vY1 && (y - x0) <= _vY2){
                index = _scrWidth * (y - x0) + (x - y0); scr += index; *scr = col; scr = null8; 
            }

            x0++;
            ddF_x += 2;
            f += ddF_x;
        }
    }
}

void VGA_GFX::fillCircle(int x, int y, int r, uint16_t col){
    if (r <= 0) return;

    int f     = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x0     = 0;
    int y0     = r;

    hLine(x - r, y, x + r, col); // Центральная горизонтальная линия

    while (x0 <= y0) {
        if (f >= 0) {
            y0--;
            ddF_y += 2;
            f += ddF_y;
        }
        x0++;
        ddF_x += 2;
        f += ddF_x;

        hLine(x - x0, y + y0, x + x0, col); 
        hLine(x - x0, y - y0, x + x0, col); 
        if (x0 != y0) { // Убедимся, что не рисуем те же линии дважды        
            hLine(x - y0, y + x0, x + y0, col); 
            hLine(x - y0, y - x0, x + y0, col);  
        }                  
    }
}

void VGA_GFX::polygon(int x, int y, int radius, int sides, float rotation, uint16_t col) {
    if (sides < 5) return; // Минимум 3 стороны

    float angleStep = 2 * M_PI / sides;
    int x1 = x + radius * cos(rotation);
    int y1 = y + radius * sin(rotation);

    for (int i = 1; i <= sides; i++) {
        float angle = i * angleStep + rotation;
        int x2 = x + radius * cos(angle);
        int y2 = y + radius * sin(angle);

        line(x1, y1, x2, y2, col);
        x1 = x2;
        y1 = y2;
    }
}


//16 bit graphics-----------------------------------------------------------------------
/*
void VGA_GFX::fillRectDMA(int x1, int y1, int x2, int y2, uint16_t col) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = (x2 - x1 + 1) * (y2 - y1 + 1) * 16; // 16 бит на пиксель
    t.tx_buffer = &_vga.getBuff16()[_scrWidth * y1 + x1]; // Передаём указатель на буфер
    spi_device_transmit(spi_handle, &t); // Асинхронная передача через SPI DMA
}

#define SWAP16(x) (((x) << 8) | ((x) >> 8))
col = SWAP16(col);

uint16_t VGA_GFX::getPixel16(uint8_t r, uint8_t g, uint8_t b){
    uint8_t rr = map(r, 0, 256, 0, 32);
    uint8_t gg = map(g, 0, 256, 0, 64);
    uint8_t bb = map(b, 0, 256, 0, 32);

    uint16_t col = rr << 11 | gg << 5 | bb;

    return col;
}

void VGA_GFX::cls16(uint16_t col){
    uint16_t* memStart = &_vga.getBuff16()[0];
    std::fill(memStart, memStart + (_scrSize >> 1), col);   
}

void VGA_GFX::putPixel16(int x, int y, uint16_t col){
    if (x < _vX1 || y < _vY1 || x > _vX2 || y > _vY2) return; 
       
    uint16_t* memStart = &_vga.getBuff16()[_scrWidth * y + x];
    *memStart = col;
}

void VGA_GFX::hLine16(int x1, int y, int x2, uint16_t col){
    if (x2 < x1) swap(x1, x2);
    if (y < _vY1 || y > _vY2 || x2 < _vX1 || x1 > _vX2) return;   

    x1 = max(0, x1);
    x2 = min(_vX2, x2);
    int size = x2 - x1 + 1;
    
    // Используем указатель для начала строки y
    uint16_t* memStart = &_vga.getBuff16()[_scrWidth * y + x1];
    std::fill(memStart, memStart + size, col);
}

void VGA_GFX::vLine16(int x, int y1, int y2, uint16_t col){
    if (y2 < y1) swap(y1, y2);
    if (x < _vX1 || x > _vX2 || y2 < _vY1 || y1 >= _vY2) return;    

    y1 = max(0, y1);
    y2 = min(_vY2, y2);
    int size = y2 - y1 + 1;
    
    // Используем указатель на первую ячейку вертикальной линии
    uint16_t* memStart = &_vga.getBuff16()[_scrWidth * y1 + x];

    for (int i = 0; i < size; i++) {
        *memStart = col;  // Заполняем пиксель
        memStart += _scrWidth; // Переходим к следующему пикселю в колонке
    }
}

void VGA_GFX::rect16(int x1, int y1, int x2, int y2, uint16_t col){
    hLine16(x1, y1, x2, col);  // Верхняя граница
    hLine16(x1, y2, x2, col);  // Нижняя граница
    vLine16(x1, y1, y2, col);  // Левая граница
    vLine16(x2, y1, y2, col);  // Правая граница
}

void VGA_GFX::fillRect16(int x1, int y1, int x2, int y2, uint16_t col){
    if (x2 < x1) swap(x1, x2);
    if (y2 < y1) swap(y1, y2);

    x1 = max(0, min(x1, _vX2));
    y1 = max(0, min(y1, _vY2));
    x2 = max(0, min(x2, _vX2));
    y2 = max(0, min(y2, _vY2));

    int size = x2 - x1 + 1;

    uint16_t* memStart = &_vga.getBuff16()[_scrWidth * y1 + x1]; // Стартовый указатель для первой строки
    for (int y = y1; y <= y2; y++) {
        std::fill(memStart, memStart + size, col);
        memStart += _scrWidth;           // Переходим к следующей строке
    }
}


void VGA_GFX::fog(int x1, int y1, int x2, int y2) {
    if (_scrColorBits == 16){
        uint16_t* scr = _vga.getBuff16();
        scr += x1;
        uint16_t* line = scr + y1 * _scrWidth;

        for (int y = y1; y < y2; y++) {
            uint16_t* curr = line;
            uint16_t* next = line + _scrWidth;
            curr++; // сдвигаем указатель на x = x1 + 1

            for (int x = x1; x < x2; x++, curr++, next++) {
                uint16_t c0 = *curr;
                uint16_t c1 = *(next);
                uint16_t c2 = *(next - 1);
                uint16_t c3 = *(next + 1);

                int r = (((c0 >> 11) & 0x1F) + ((c1 >> 11) & 0x1F) + (((c2 >> 11) & 0x1F) + ((c3 >> 11) & 0x1F)) / 2) / 3;
                int g = (((c0 >> 5) & 0x3F) + ((c1 >> 5) & 0x3F) + (((c2 >> 5) & 0x3F) + ((c3 >> 5) & 0x3F)) / 2) / 3;
                int b = ((c0 & 0x1F) + (c1 & 0x1F) + ((c2 & 0x1F) + (c3 & 0x1F)) / 2) / 3;

                *curr = (r << 11) | (g << 5) | b;
            }

            line += _scrWidth; // переход на следующую строку
        }
    } else {
        uint8_t* scr = _vga.getBuff();
        scr += x1;
        uint8_t* line = scr + y1 * _scrWidth;

        for (int y = y1; y < y2; y++) {
            uint8_t* curr = line;
            uint8_t* next = line + _scrWidth;
            curr++; // сдвигаем указатель на x = x1 + 1

            for (int x = x1; x < x2; x++, curr++, next++) {
                uint8_t c0 = *curr;
                uint8_t c1 = *(next);
                uint8_t c2 = *(next - 1);
                uint8_t c3 = *(next + 1);

                int r = (((c0 >> 5) & 0x07) + ((c1 >> 5) & 0x07) + (((c2 >> 5) & 0x07) + ((c3 >> 5) & 0x07)) / 2) / 3;
                int g = (((c0 >> 2) & 0x07) + ((c1 >> 2) & 0x07) + (((c2 >> 2) & 0x07) + ((c3 >> 2) & 0x07)) / 2) / 3;
                int b = ((c0 & 0x03) + (c1 & 0x03) + ((c2 & 0x03) + (c3 & 0x03)) / 2) / 3;
                *curr = (r << 11) | (g << 5) | b;
            }

            line += _scrWidth; // переход на следующую строку
        }
    }
}

/*
void VGA_GFX::fog16(int x1, int y1, int x2, int y2) {
    if (_scrColorBits == 16) {
        uint16_t* scr = _vga.getBuff16();
        uint16_t* line = scr + y1 * _scrWidth;

        for (int y = y1; y < y2 - 1; y++) {
            uint16_t* curr = line + x1 + 1;
            uint16_t* next = curr + _scrWidth;

            for (int x = x1 + 1; x < x2 - 1; x++, curr++, next++) {
                uint16_t c0 = *curr;
                uint16_t c1 = *next;
                uint16_t c2 = *(next - 1);
                uint16_t c3 = *(next + 1);

                uint32_t r = (((c0 >> 11) & 0x1F) + ((c1 >> 11) & 0x1F) + (((c2 >> 11) & 0x1F) + ((c3 >> 11) & 0x1F)) >> 1)) * 341 >> 10;
                uint32_t g = (((c0 >> 5) & 0x3F) + ((c1 >> 5) & 0x3F) + (((c2 >> 5) & 0x3F) + ((c3 >> 5) & 0x3F)) >> 1)) * 341 >> 10;
                uint32_t b = (((c0 & 0x1F) + (c1 & 0x1F) + ((c2 & 0x1F) + (c3 & 0x1F)) >> 1)) * 341 >> 10;

                *curr = (r << 11) | (g << 5) | b;
            }
            line += _scrWidth;  // переход на следующую строку
        }
    } else {
        uint8_t* scr = _vga.getBuff();
        uint8_t* line = scr + y1 * _scrWidth;

        for (int y = y1; y < y2 - 1; y++) {
            uint8_t* curr = line + x1 + 1;
            uint8_t* next = curr + _scrWidth;

            for (int x = x1 + 1; x < x2 - 1; x++, curr++, next++) {
                uint8_t c0 = *curr;
                uint8_t c1 = *next;
                uint8_t c2 = *(next - 1);
                uint8_t c3 = *(next + 1);

                uint32_t r = (((c0 >> 5) & 0x07) + ((c1 >> 5) & 0x07) + (((c2 >> 5) & 0x07) + ((c3 >> 5) & 0x07)) >> 1) * 341 >> 10;
                uint32_t g = (((c0 >> 2) & 0x07) + ((c1 >> 2) & 0x07) + (((c2 >> 2) & 0x07) + ((c3 >> 2) & 0x07)) >> 1) * 341 >> 10;
                uint32_t b = (((c0 & 0x03) + (c1 & 0x03) + ((c2 & 0x03) + (c3 & 0x03)) >> 1)) * 341 >> 10;

                *curr = (r << 5) | (g << 2) | b;  // Исправленный 8-битный формат
            }
            line += _scrWidth;
        }
    }
}

void VGA_GFX::fog16() {
    uint16_t* scr = _vga.getBuff16();
    uint16_t* line = scr + _vY1 * _scrWidth;

    for (int y = _vY1; y < _vY2 - 9; y++) {
        uint16_t* curr = line + _vX1 + 1;
        uint16_t* next = curr + _scrWidth;
        
        for (int x = _vX1 + 1; x < _vX2 - 1; x += 2, curr += 2, next += 2) {
            uint32_t c0 = curr[0], c1 = next[0], c2 = next[-1], c3 = next[1];
            uint32_t c4 = curr[1], c5 = next[1], c6 = next[0], c7 = next[2];

            // Исправленный расчет средних значений цветов
            uint32_t r0 = (((c0 >> 11) & 0x1F) + ((c1 >> 11) & 0x1F) + (((c2 >> 11) & 0x1F) + ((c3 >> 11) & 0x1F)) >> 1) * 341 >> 10;
            uint32_t g0 = (((c0 >> 5) & 0x3F) + ((c1 >> 5) & 0x3F) + (((c2 >> 5) & 0x3F) + ((c3 >> 5) & 0x3F)) >> 1) * 341 >> 10;
            uint32_t b0 = ((c0 & 0x1F) + (c1 & 0x1F) + ((c2 & 0x1F) + (c3 & 0x1F)) >> 1) * 341 >> 10;

            uint32_t r1 = (((c4 >> 11) & 0x1F) + ((c5 >> 11) & 0x1F) + (((c6 >> 11) & 0x1F) + ((c7 >> 11) & 0x1F)) >> 1) * 341 >> 10;
            uint32_t g1 = (((c4 >> 5) & 0x3F) + ((c5 >> 5) & 0x3F) + (((c6 >> 5) & 0x3F) + ((c7 >> 5) & 0x3F)) >> 1) * 341 >> 10;
            uint32_t b1 = ((c4 & 0x1F) + (c5 & 0x1F) + ((c6 & 0x1F) + (c7 & 0x1F)) >> 1) * 341 >> 10;

            curr[0] = (r0 << 11) | (g0 << 5) | b0;
            curr[1] = (r1 << 11) | (g1 << 5) | b1;
        }
        line += _scrWidth;
    }
}


/*
void VGA_GFX::fog16() {
    uint16_t* scr = _vga.getBuff16();
    uint16_t* line = scr + _vY1 * _scrWidth;

    for (int y = _vY1; y < _vY2 - 9; y++) {
        uint16_t* curr = line;
        uint16_t* next = line + _scrWidth;
        curr++; // сдвигаем указатель на x = _vX1 + 1

        for (int x = _vX1 + 1; x < _vX2 - 1; x++, curr++, next++) {
            uint16_t c0 = *curr;
            uint16_t c1 = *(next);
            uint16_t c2 = *(next - 1);
            uint16_t c3 = *(next + 1);

            int r = (((c0 >> 11) & 0x1F) + ((c1 >> 11) & 0x1F) + (((c2 >> 11) & 0x1F) + ((c3 >> 11) & 0x1F)) / 2) / 3;
            int g = (((c0 >> 5) & 0x3F) + ((c1 >> 5) & 0x3F) + (((c2 >> 5) & 0x3F) + ((c3 >> 5) & 0x3F)) / 2) / 3;
            int b = ((c0 & 0x1F) + (c1 & 0x1F) + ((c2 & 0x1F) + (c3 & 0x1F)) / 2) / 3;

            *curr = (r << 11) | (g << 5) | b;
        }
        line += _scrWidth; // переход на следующую строку
    }
}
*/