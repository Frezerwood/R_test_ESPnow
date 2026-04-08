// #pragma once
// #include <Arduino.h>

// struct MultiServoCommand {
//     uint8_t slaveId;     // 0..3
//     uint8_t servoMask;   // биты: 0b0001 = servo0, 0b0010 = servo1, ..., 0b1111 = все 4
//     uint16_t holdMs;
// };
//-------------------------
// #pragma once
// #include <Arduino.h>

// static constexpr uint8_t SERVO_INDEX_ALL = 255;

// struct ServoCommand {
//     uint8_t servoIndex;   // 0..3 или SERVO_INDEX_ALL
//     uint16_t holdMs;      // сколько держать открытой
// };
//------------------------------
#pragma once
#include <Arduino.h>

struct ServoCommand {
    uint8_t servoMask;   // битовая маска серв:
                         // bit0 -> servo 0
                         // bit1 -> servo 1
                         // bit2 -> servo 2
                         // bit3 -> servo 3
    uint16_t holdMs;     // сколько держать открытыми
};