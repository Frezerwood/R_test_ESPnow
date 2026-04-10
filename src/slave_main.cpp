#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>
#include <esp_idf_version.h>
#include "config.h"
#include "protocol.h"

// Состояния каждой сервомашинки
enum ServoPhase {
    IDLE,
    MOVING_OPEN,
    HOLDING,
    MOVING_CLOSE
};

struct ServoState {
    ServoPhase phase = IDLE;
    uint8_t startAngle = CENTER_ANGLE;
    uint8_t targetAngle = CENTER_ANGLE;
    unsigned long startTime = 0;
    uint16_t duration = 450; // Время хода в мс
    uint16_t holdTime = 0;   // Время удержания из команды
};

Servo servos[4];
ServoState states[4];
uint8_t currentAngles[4] = {CENTER_ANGLE, CENTER_ANGLE, CENTER_ANGLE, CENTER_ANGLE};

// --------------------------------------------------
// Вспомогательные функции
// --------------------------------------------------

uint8_t clampAngle(int value) {
    return (uint8_t)constrain(value, 0, 180);
}

uint8_t getClosedAngle(uint8_t idx) {
    return clampAngle(CENTER_ANGLE + SERVO_OFFSETS[idx]);
}

uint8_t getOpenAngle(uint8_t idx) {
    return clampAngle(CENTER_ANGLE + SERVO_OFFSETS[idx] + OPEN_DELTA);
}

// Математика Ease-In-Out
float fEaseInOut(float t) {
    return 0.5f * (1.0f - cosf(t * PI));
}

// --------------------------------------------------
// Ядро управления (Non-blocking)
// --------------------------------------------------

void updateServos() {
    unsigned long now = millis();

    for (int i = 0; i < 4; i++) {
        if (states[i].phase == IDLE) continue;

        // --- Фазы движения (Открытие или Закрытие) ---
        if (states[i].phase == MOVING_OPEN || states[i].phase == MOVING_CLOSE) {
            float progress = (float)(now - states[i].startTime) / states[i].duration;
            
            if (progress >= 1.0f) {
                progress = 1.0f;
                // Переход к следующей фазе
                if (states[i].phase == MOVING_OPEN) {
                    states[i].phase = HOLDING;
                    states[i].startTime = now;
                } else {
                    states[i].phase = IDLE;
                }
            }

            float ease = fEaseInOut(progress);
            float angleF = (float)states[i].startAngle + (float)(states[i].targetAngle - states[i].startAngle) * ease;
            
            currentAngles[i] = (uint8_t)round(angleF);
            servos[i].write(currentAngles[i]);
        }
        
        // --- Фаза удержания (HOLD) ---
        else if (states[i].phase == HOLDING) {
            if (now - states[i].startTime >= states[i].holdTime) {
                // Начинаем закрытие
                states[i].phase = MOVING_CLOSE;
                states[i].startAngle = currentAngles[i];
                states[i].targetAngle = getClosedAngle(i);
                states[i].startTime = now;
                states[i].duration = 500; // Можно сделать закрытие чуть медленнее для эстетики
            }
        }
    }
}

// --------------------------------------------------
// Обработка команд
// --------------------------------------------------

void executeCommand(const ServoCommand& cmd) {
    for (int i = 0; i < 4; i++) {
        // Проверяем, касается ли команда этой серво (битмаска)
        if ((cmd.servoMask >> i) & 0x01) {
            states[i].startAngle = currentAngles[i]; // Начинаем из текущей позиции (важно!)
            states[i].targetAngle = getOpenAngle(i);
            states[i].startTime = millis();
            states[i].holdTime = cmd.holdMs;
            states[i].duration = 450;
            states[i].phase = MOVING_OPEN;
        }
    }
}

// --------------------------------------------------
// ESP-NOW Callbacks
// --------------------------------------------------

#if ESP_IDF_VERSION_MAJOR >= 5
void onDataRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
#else
void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) {
#endif
    if (len == sizeof(ServoCommand)) {
        ServoCommand cmd;
        memcpy(&cmd, data, sizeof(cmd));
        executeCommand(cmd);
    }
}

// --------------------------------------------------
// Setup & Loop
// --------------------------------------------------

void setup() {
    Serial.begin(115200);
    
    // Инициализация серво
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    for (int i = 0; i < 4; i++) {
        servos[i].setPeriodHertz(50);
        servos[i].attach(SERVO_PINS[i], 500, 2400);
        // Устанавливаем в закрытое положение сразу
        uint8_t start = getClosedAngle(i);
        servos[i].write(start);
        currentAngles[i] = start;
    }

    // WiFi и ESP-NOW
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW Init Failed");
        ESP.restart();
    }

    esp_now_register_recv_cb(onDataRecv);
    Serial.printf("Slave ID %d Ready. MAC: %s\n", THIS_SLAVE_ID, WiFi.macAddress().c_str());
}

void loop() {
    // Вызываем обновление физики как можно чаще
    updateServos();
}