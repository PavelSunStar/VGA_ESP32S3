#pragma once

#include <cstdint>

#define LUT_SIZE 360
#define FAST_RND(n)    ((n) ? (millis() % (n)) : 0)
#define FAST_RND2(n)   ((millis()) & ((n) - 1))  // Только если n — степень двойки

class VGA_Math {
public:
    VGA_Math();
    ~VGA_Math();

    void LUT(int &x, int &y, int xx, int yy, int len, int angle);
    int xLUT(int x, int y, int len, int angle);
    int yLUT(int x, int y, int len, int angle);

    //fast Y
    int fast320(int y);
    int fast640(int y);
    int rnd(int min, int max);


    void startFPS();
    void calcFPS();
    float getFPS() const;

    int16_t _sinLUT[LUT_SIZE];
    int16_t _cosLUT[LUT_SIZE];

    unsigned long _frameCount = 0;
    unsigned long _fpsStartTime = 0;
    float _fps = 0.0f;

    int fastY_320[240];
    int fastY_640[480];

    int _buffSize = 0x800; //2048
    uint8_t *_buff = nullptr;
};

/*
#pragma once

#include <cstdint>  // Добавляем этот заголовок

#define LUT_SIZE 360
#define FAST_RND(n) ((n) ? (millis() % (n)) : 0)
#define FAST_RND2(n) ((millis()) & ((n) - 1))  // работает только если n — 2ⁿ



extern int16_t sinLUT[LUT_SIZE];  // Теперь это только объявление
extern int16_t cosLUT[LUT_SIZE];

// for tracking fps
extern float _fps;                  // Объявление глобальной переменной
extern unsigned long _frameCount;            // Объявление глобальной переменной
extern unsigned long _fpsStartTimeStamp;  // Объявление глобальной переменной

    void initLUT();
    void LUT(int &x, int &y, int xx, int yy, int len, int angle);
    int xLUT(int x, int y, int len, int angle);
    int yLUT(int x, int y, int len, int angle);
    uint16_t rnd16(uint16_t n);
    uint8_t rnd8(uint8_t);

    void startFPS();
    void calcFPS();

    // Swap any type
    //template <typename T> static inline void
    //swap(T& a, T& b) { T t = a; a = b; b = t; }	
*/

