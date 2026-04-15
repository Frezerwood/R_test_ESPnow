#pragma once
#include <Arduino.h>

// Для адресных команд (одному слейву)
struct ServoCommand {
    uint8_t servoMask;
    uint16_t holdMs;
};

// Для широковещательных команд (всем сразу)
struct FullMatrixCommand {
    uint8_t masks[4]; // Маски для слейвов 0, 1, 2, 3
    uint16_t holdMs;
};