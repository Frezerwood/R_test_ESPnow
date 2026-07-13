#include "indicator.h"

// Используем указанный пин для встроенного диода
#define LED_PIN 8 

void indicator_init() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH); // ИНВЕРСИЯ: HIGH выключает диод по умолчанию
}

void indicator_blink(uint16_t duration_ms) {
    digitalWrite(LED_PIN, LOW);  // ИНВЕРСИЯ: LOW включает диод
    delay(duration_ms);          // Короткая пауза для вспышки
    digitalWrite(LED_PIN, HIGH); // ИНВЕРСИЯ: возвращаем в выключенное состояние (HIGH)
}