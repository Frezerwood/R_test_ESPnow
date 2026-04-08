#pragma once
#include <Arduino.h>

static constexpr uint8_t ESPNOW_CHANNEL = 1;

#ifndef SLAVE_ID
#define SLAVE_ID 0
#endif

static constexpr uint8_t THIS_SLAVE_ID = SLAVE_ID;

// ===== MAC-адреса 4 слейвов =====

static const uint8_t SLAVE_MACS[4][6] = {
    {0xB0, 0xA6, 0x04, 0x06, 0xD4, 0x74}, // slave0
    {0x08, 0x92, 0x72, 0x84, 0x39, 0xA4}, // slave1
    {0x88, 0x56, 0xA6, 0x78, 0xA4, 0xB4}, // slave2
    {0x08, 0x92, 0x72, 0x84, 0x3E, 0x58}  // slave3
};

// ===== Пины 4 серв на одном ESP32-C3 =====
// Подставь свои реальные GPIO
// Локальный порядок серво:
// index 0 -> row 0, col 0
// index 1 -> row 0, col 1
// index 2 -> row 1, col 0
// index 3 -> row 1, col 1
static constexpr int SERVO_PINS[4] = {2, 3, 4, 5};

// ===== Углы =====
static constexpr uint8_t CENTER_ANGLE = 90;
static constexpr uint8_t OPEN_DELTA   = 35;

// ===== Тайминги =====
static constexpr uint16_t HOLD_MS       = 220;
static constexpr uint16_t STEP_DELAY_MS = 260;

// ===== Глобальная матрица =====
static constexpr uint8_t GLOBAL_ROWS = 4;
static constexpr uint8_t GLOBAL_COLS = 4;
static constexpr uint8_t LOCAL_ROWS  = 2;
static constexpr uint8_t LOCAL_COLS  = 2;