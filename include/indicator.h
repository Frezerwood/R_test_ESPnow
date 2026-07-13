#pragma once
#include <Arduino.h>

// Инициализация пина светодиода
void indicator_init();

// Функция для однократного мигания (блокирующая или быстрая для коллбэка)
void indicator_blink(uint16_t duration_ms = 50);