#include "VGA_Math.h"
#include <cstdint>  // Добавляем этот заголовок
#include <math.h>
#include <Arduino.h>

#ifndef PI
#define PI 3.14159265358979323846
#endif

float _fps = 0;                // Определение переменной _fps
unsigned long _frameCount = 0;          // Определение переменной _frameCount
unsigned long _fpsStartTimeStamp = 0; // Определение переменной _fpsStartTimeStamp

int16_t sinLUT[LUT_SIZE];  // Теперь массивы определены только здесь
int16_t cosLUT[LUT_SIZE];

void initLUT() {
    for (int i = 0; i < LUT_SIZE; i++) {
        float angle = i * (M_PI / 180.0f);
        sinLUT[i] = int16_t(sin(angle) * 32767);  // Фиксированная точка, если нужно
        cosLUT[i] = int16_t(cos(angle) * 32767);
    }
}

void LUT(int &x, int &y, int xx, int yy, int len, int angle) {
    if (len <= 0) return;

    angle %= 360;
    if (angle < 0) angle += 360;    

    x = xx + len * cosLUT[angle];
    y = yy + len * sinLUT[angle];
}


int xLUT(int x, int y, int len, int angle){
    if (len <= 0) return 0;

    angle %= 360;
    if (angle < 0) angle = 360 + angle;    
    int res = x + len * cosLUT[angle];

    return res; 
}

int yLUT(int x, int y, int len, int angle){
    if (len <= 0) return 0;

    angle %= 360;
    if (angle < 0) angle = 360 + angle;
    int res = y + len * sinLUT[angle];

    return res;
}

void startFPS(){
    _fpsStartTimeStamp = millis();    
}

void calcFPS() {    
    unsigned long elapsedTime = millis() - _fpsStartTimeStamp;
    _frameCount++;
    
    if (elapsedTime > 1000) {
        _fps = _frameCount / (elapsedTime / 1000.0);
        _fpsStartTimeStamp = millis();
        _frameCount = 0;
    }
}
