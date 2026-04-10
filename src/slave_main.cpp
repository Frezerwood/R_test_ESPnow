#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>
#include <esp_idf_version.h>
#include "config.h"
#include "protocol.h"

Servo servos[4];
uint8_t currentAngles[4] = {
    CENTER_ANGLE,
    CENTER_ANGLE,
    CENTER_ANGLE,
    CENTER_ANGLE
};

// --------------------------------------------------
// Servo helpers
// --------------------------------------------------

uint8_t clampAngle(int value) {
    if (value < 0) return 0;
    if (value > 180) return 180;
    return static_cast<uint8_t>(value);
}

uint8_t getClosedAngle(uint8_t servoIndex) {
    if (servoIndex >= 4) return CENTER_ANGLE;
    return clampAngle(CENTER_ANGLE + SERVO_OFFSETS[servoIndex]);
}

uint8_t getOpenAngle(uint8_t servoIndex) {
    if (servoIndex >= 4) return CENTER_ANGLE;
    return clampAngle(CENTER_ANGLE + SERVO_OFFSETS[servoIndex] + OPEN_DELTA);
}

void writeServoImmediate(uint8_t servoIndex, uint8_t angle) {
    if (servoIndex >= 4) return;

    uint8_t safeAngle = clampAngle(angle);
    servos[servoIndex].write(safeAngle);
    currentAngles[servoIndex] = safeAngle;
}

void openServosByMask(uint8_t servoMask) {
    bool finished = false;

    while (!finished) {
        finished = true;

        for (int i = 0; i < 4; i++) {
            if (((servoMask >> i) & 0x01) == 0) {
                continue;
            }

            uint8_t target = getOpenAngle(i);

            if (currentAngles[i] < target) {
                currentAngles[i]++;
                servos[i].write(currentAngles[i]);
                finished = false;
            } else if (currentAngles[i] > target) {
                currentAngles[i]--;
                servos[i].write(currentAngles[i]);
                finished = false;
            }
        }

        delay(8);
    }
}

void closeServosByMask(uint8_t servoMask) {
    bool finished = false;

    while (!finished) {
        finished = true;

        for (int i = 0; i < 4; i++) {
            if (((servoMask >> i) & 0x01) == 0) {
                continue;
            }

            uint8_t target = getClosedAngle(i);

            if (currentAngles[i] < target) {
                currentAngles[i]++;
                servos[i].write(currentAngles[i]);
                finished = false;
            } else if (currentAngles[i] > target) {
                currentAngles[i]--;
                servos[i].write(currentAngles[i]);
                finished = false;
            }
        }

        delay(8);
    }
}

void initServos() {
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);

    for (int i = 0; i < 4; i++) {
        servos[i].setPeriodHertz(50);
        servos[i].attach(SERVO_PINS[i], 500, 2400);
        writeServoImmediate(i, getClosedAngle(i));
    }

    delay(300);
}

// --------------------------------------------------
// Command execution
// --------------------------------------------------

void executeCommand(const ServoCommand& cmd) {
    if ((cmd.servoMask & 0x0F) == 0) {
        Serial.println("Empty servo mask");
        return;
    }

    Serial.printf("Recv: mask=0x%02X hold=%u\n", cmd.servoMask, cmd.holdMs);

    openServosByMask(cmd.servoMask);
    delay(cmd.holdMs);
    closeServosByMask(cmd.servoMask);

    Serial.printf("Done: mask=0x%02X\n", cmd.servoMask);
}

// --------------------------------------------------
// ESP-NOW callback
// --------------------------------------------------

#if ESP_IDF_VERSION_MAJOR >= 5
void onDataRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
    (void)info;

    if (len != sizeof(ServoCommand)) {
        Serial.printf("Wrong packet size: %d\n", len);
        return;
    }

    ServoCommand cmd;
    memcpy(&cmd, data, sizeof(cmd));
    executeCommand(cmd);
}
#else
void onDataRecv(const uint8_t* mac_addr, const uint8_t* data, int len) {
    (void)mac_addr;

    if (len != sizeof(ServoCommand)) {
        Serial.printf("Wrong packet size: %d\n", len);
        return;
    }

    ServoCommand cmd;
    memcpy(&cmd, data, sizeof(cmd));
    executeCommand(cmd);
}
#endif

// --------------------------------------------------
// setup / loop
// --------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.printf("\n=== SLAVE START, ID=%u ===\n", THIS_SLAVE_ID);

    initServos();

    WiFi.mode(WIFI_STA);

    Serial.print("Slave MAC: ");
    Serial.println(WiFi.macAddress());

    if (esp_now_init() != ESP_OK) {
        Serial.println("esp_now_init failed");
        while (true) {
            delay(1000);
        }
    }

    esp_now_register_recv_cb(onDataRecv);
    Serial.println("ESP-NOW ready");
}

void loop() {
}