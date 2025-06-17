#include <cstdint>  // Добавляем этот заголовок

#ifndef VGA_MATH_H
#define VGA_MATH_H

#define LUT_SIZE 360

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

    void startFPS();
    void calcFPS();

    // Swap any type
    //template <typename T> static inline void
    //swap(T& a, T& b) { T t = a; a = b; b = t; }	

#endif
