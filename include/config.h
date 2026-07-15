#pragma once
#include <Arduino.h>

static constexpr uint8_t ESPNOW_CHANNEL = 1;

#ifndef SLAVE_ID
#define SLAVE_ID 0
#endif

#if SLAVE_ID == 0
static constexpr int SERVO_OFFSETS[4] = {0, 0, 0, 0};
#elif SLAVE_ID == 1
static constexpr int SERVO_OFFSETS[4] = {0, 0, 0, 0};
#elif SLAVE_ID == 2
static constexpr int SERVO_OFFSETS[4] = {0, 0, 0, 0};
#elif SLAVE_ID == 3
static constexpr int SERVO_OFFSETS[4] = {0, 0, 0, 0};
#else
static constexpr int SERVO_OFFSETS[4] = {0, 0, 0, 0};
#endif

static constexpr uint8_t THIS_SLAVE_ID = SLAVE_ID;


// ===== MAC-адреса 4 слейвов =====

static const uint8_t SLAVE_MACS[4][6] = {
    {0x14, 0x63, 0x93, 0xC6, 0xD5, 0x10}, // slave0
   // {0x14, 0x63, 0x93, 0xC8, 0x9A, 0x90}, // slave0
    //{0xE0, 0x72, 0xA1, 0x71, 0xC7, 0x28}, // slave0
    {0xA0, 0xF2, 0x62, 0xB2, 0xF0, 0x8C}, // slave1
    {0x88, 0x56, 0xA6, 0x78, 0xA4, 0xB4}, // slave2
    {0x08, 0x92, 0x72, 0x84, 0x3E, 0x58}  // slave3
};

// ===== Пины 4 серв на одном ESP32-C3 =====
static constexpr int SERVO_PINS[4] = {2, 3, 4, 5};

// ===== Углы =====
static constexpr uint8_t CENTER_ANGLE = 90;
static constexpr uint8_t OPEN_DELTA   = 35;

// ===== Тайминги =====
static constexpr uint16_t HOLD_MS       = 240;
static constexpr uint16_t STEP_DELAY_MS = 260;

// ===== Глобальная матрица =====
static constexpr uint8_t GLOBAL_ROWS = 4;
static constexpr uint8_t GLOBAL_COLS = 4;
static constexpr uint8_t LOCAL_ROWS  = 2;
static constexpr uint8_t LOCAL_COLS  = 2;