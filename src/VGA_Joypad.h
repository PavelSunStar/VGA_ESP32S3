#ifndef VGA_JOYPAD_H
#define VGA_JOYPAD_H

#include <Arduino.h>  // Подключаем библиотеку (если используешь Arduino/ESP)

#define JOY_A 19
#define JOY_B 20
#define JOY_C 21
#define JOY_D 47
#define JOY_E 48
#define JOY_F 45
#define JOY_K 40
//#define JOY_X 38
//#define JOY_Y 39

// Глобальная переменная для хранения состояния кнопок
static uint8_t butt = 0;

// Инициализация кнопок
void initJoypad() {
    pinMode(JOY_A, INPUT_PULLUP);
    pinMode(JOY_B, INPUT_PULLUP);
    pinMode(JOY_C, INPUT_PULLUP);
    pinMode(JOY_D, INPUT_PULLUP);
    pinMode(JOY_E, INPUT_PULLUP);
    pinMode(JOY_F, INPUT_PULLUP);
    pinMode(JOY_K, INPUT_PULLUP);
}

bool joyPress() {
    uint8_t tmp = ((~digitalRead(JOY_A) & 1) << 6) |
                  ((~digitalRead(JOY_B) & 1) << 5) |
                  ((~digitalRead(JOY_C) & 1) << 4) |
                  ((~digitalRead(JOY_D) & 1) << 3) |
                  ((~digitalRead(JOY_E) & 1) << 2) |
                  ((~digitalRead(JOY_F) & 1) << 1) |
                   (~digitalRead(JOY_K) & 1);

    if (tmp == butt) return false; // Если состояние не изменилось — ничего не делаем

    butt = tmp; // Обновляем состояние
    return true;                    
}

bool joyA() { return butt & (1 << 6); }
bool joyB() { return butt & (1 << 5); }
bool joyC() { return butt & (1 << 4); }
bool joyD() { return butt & (1 << 3); }
bool joyE() { return butt & (1 << 2); }
bool joyF() { return butt & (1 << 1); }
bool joyK() { return butt & 1; }

#endif