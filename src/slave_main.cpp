#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>
#include <esp_idf_version.h>
#include "config.h"
#include "protocol.h"
#include <Adafruit_NeoPixel.h>

#define LED_PIN     10 
#define NUM_LEDS    4  
#define LED_BRIGHT  150 

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Цвет для открытого состояния (R, G, B)
uint32_t activeColor = strip.Color(255, 100, 0); // Оранжевый/Теплый
uint32_t offColor    = strip.Color(0, 0, 0);       // Выключено

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
    bool needShow = false;

    for (int i = 0; i < 4; i++) {
        if (states[i].phase == IDLE) continue;

        if (states[i].phase == MOVING_OPEN || states[i].phase == MOVING_CLOSE) {
            float progress = (float)(now - states[i].startTime) / states[i].duration;
            
            if (progress >= 1.0f) {
                progress = 1.0f;
                if (states[i].phase == MOVING_OPEN) {
                    states[i].phase = HOLDING;
                    states[i].startTime = now;
                } else {
                    states[i].phase = IDLE;
                    // ГАСИМ ДИОД, когда закрытие завершено
                    strip.setPixelColor(i, strip.Color(0, 0, 0));
                    needShow = true;
                }
            }

            float ease = fEaseInOut(progress);
            float angleF = (float)states[i].startAngle + (float)(states[i].targetAngle - states[i].startAngle) * ease;
            currentAngles[i] = (uint8_t)round(angleF);
            servos[i].write(currentAngles[i]);
        }
        
        else if (states[i].phase == HOLDING) {
            if (now - states[i].startTime >= states[i].holdTime) {
                states[i].phase = MOVING_CLOSE;
                states[i].startAngle = currentAngles[i];
                states[i].targetAngle = getClosedAngle(i);
                states[i].startTime = now;
                states[i].duration = 500;
                // Можно погасить диод уже тут, если хочешь, чтобы он гас в начале закрытия
            }
        }
    }

    if (needShow) {
        strip.show(); // Обновляем ленту только если какой-то диод реально выключился
    }
}

// --------------------------------------------------
// Обработка команд
// --------------------------------------------------

void executeCommand(const ServoCommand& cmd) {
    for (int i = 0; i < 4; i++) {
        if ((cmd.servoMask >> i) & 0x01) {
            states[i].startAngle = currentAngles[i];
            states[i].targetAngle = getOpenAngle(i);
            states[i].startTime = millis();
            states[i].holdTime = cmd.holdMs;
            states[i].duration = 450;
            states[i].phase = MOVING_OPEN;

            // ЗАЖИГАЕМ СРАЗУ
            strip.setPixelColor(i, strip.Color(255, 100, 0)); 
        }
    }
    strip.show(); // Один раз обновили ленту после обработки всей маски
}

// --------------------------------------------------
// ESP-NOW Callbacks (Обновленный для Broadcast)
// --------------------------------------------------

#if ESP_IDF_VERSION_MAJOR >= 5
void onDataRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
#else
void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) {
#endif
    // Вариант 1: Пришла команда только для этого слейва (адресная)
    if (len == sizeof(ServoCommand)) {
        ServoCommand cmd;
        memcpy(&cmd, data, sizeof(cmd));
        executeCommand(cmd);
    } 
    // Вариант 2: Пришел общий пакет для всей матрицы (широковещательный)
    else if (len == sizeof(FullMatrixCommand)) {
        FullMatrixCommand fullCmd;
        memcpy(&fullCmd, data, sizeof(fullCmd));

        // Вытаскиваем маску именно для этого ID (0, 1, 2 или 3)
        uint8_t myMask = fullCmd.masks[THIS_SLAVE_ID];

        if (myMask > 0) {
            ServoCommand tempCmd;
            tempCmd.servoMask = myMask;
            tempCmd.holdMs = fullCmd.holdMs;
            executeCommand(tempCmd);
        }
    }
}

// --------------------------------------------------
// Setup & Loop
// --------------------------------------------------

void setup() {
    Serial.begin(115200);
    // --- 1. Инициализация ленты (ДОБАВИТЬ ЭТО) ---
    strip.begin();
    strip.setBrightness(LED_BRIGHT);
    strip.show(); // Все диоды выключены при старте
    
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