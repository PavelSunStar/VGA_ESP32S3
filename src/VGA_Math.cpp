#include "VGA_Math.h"
#include <Arduino.h>
#include <math.h>

VGA_Math::VGA_Math() {
    //Init LUT
    for (int i = 0; i < LUT_SIZE; ++i) {
        _sinLUT[i] = (int16_t)(1000.0 * sin(i * DEG_TO_RAD));
        _cosLUT[i] = (int16_t)(1000.0 * cos(i * DEG_TO_RAD));
    }

    //Init fastY
    for (int i = 0; i < 240; i++) fastY_320[i] = 320 * i;
    for (int i = 0; i < 480; i++) fastY_640[i] = 640 * i;

    _buff = (uint8_t*)heap_caps_malloc(_buffSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
}

VGA_Math::~VGA_Math() {
    if (_buff) {
        heap_caps_free(_buff);
        _buff = nullptr;
    }
}

void VGA_Math::LUT(int &x, int &y, int xx, int yy, int len, int angle) {
    angle %= LUT_SIZE;
    if (angle < 0) angle += 360;
    x = xx + (len * _cosLUT[angle]) / 1000;
    y = yy + (len * _sinLUT[angle]) / 1000;
}

int VGA_Math::xLUT(int x, int y, int len, int angle) {
    angle %= LUT_SIZE;
    if (angle < 0) angle += 360;
    return x + (len * _cosLUT[angle]) / 1000;
}

int VGA_Math::yLUT(int x, int y, int len, int angle) {
    angle %= LUT_SIZE;
    if (angle < 0) angle += 360;
    return y + (len * _sinLUT[angle]) / 1000;
}

int VGA_Math::fast320(int y){
    return fastY_320[y];
}

int VGA_Math::fast640(int y){
    return fastY_640[y];
}

int VGA_Math::rnd(int min, int max) {
    if (max < min) std::swap(min, max);
    uint32_t range = (uint32_t)(max - min);
    if (range == 0) return min;

    uint8_t bits = 32 - __builtin_clz(range);
    uint32_t rnd;

    do {
        uint32_t t = micros();
        t ^= analogRead(0);
        t ^= t << 5;
        t ^= t >> 3;
        rnd = t & ((1UL << bits) - 1);
    } while (rnd >= range);

    return min + (int)rnd;
}

void VGA_Math::startFPS() {
    _frameCount = 0;
    _fpsStartTime = millis();
}

void VGA_Math::calcFPS() {
    _frameCount++;
    unsigned long t = millis();
    if (t - _fpsStartTime >= 1000) {
        _fps = (_frameCount * 1000.0f) / (t - _fpsStartTime);
        _frameCount = 0;
        _fpsStartTime = t;
    }
}

float VGA_Math::getFPS() const {
    return _fps;
}

/*
#include "VGA_Math.h"
#include <cstdint>  // Добавляем этот заголовок
#include <math.h>
#include <Arduino.h>

#ifndef PI
#define PI 3.14159265358979323846
#endif

#define LUT_SCALE 1024

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

    x = xx + (len * cosLUT[angle]) / 32767;
    y = yy + (len * sinLUT[angle]) / 32767;
}

int xLUT(int x, int y, int len, int angle){
    if (len <= 0) return x;

    angle %= 360;
    if (angle < 0) angle += 360;

    return x + (len * cosLUT[angle]) / 32767;
}

int yLUT(int x, int y, int len, int angle){
    if (len <= 0) return y;

    angle %= 360;
    if (angle < 0) angle += 360;

    return y + (len * sinLUT[angle]) / 32767;
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

uint16_t rnd16(uint16_t n) {
    if (!n) return 0;

    uint8_t bits = 16 - __builtin_clz(n);
    uint16_t mask = (1 << bits) - 1;
    uint16_t r = micros() & mask;

    return (r < n) ? r : r % n;
}

uint8_t rnd8(uint8_t n) {
    if (!n) return 0;

    uint8_t bits = 8 - __builtin_clz(n);
    uint8_t mask = (1 << bits) - 1;
    uint8_t r = micros() & mask;

    return (r < n) ? r : r % n;
}
*/