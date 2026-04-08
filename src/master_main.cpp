#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "config.h"
#include "protocol.h"

// --------------------------------------------------
// Настройки кнопки
// --------------------------------------------------

static constexpr uint8_t BUTTON_PIN = 18;

// текущий ряд 0..3
uint8_t currentRow = 0;

// --------------------------------------------------
// ESP-NOW helpers
// --------------------------------------------------

bool addPeer(const uint8_t mac[6]) {
    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = 0;
    peer.encrypt = false;

    esp_err_t result = esp_now_add_peer(&peer);
    return (result == ESP_OK || result == ESP_ERR_ESPNOW_EXIST);
}

void onDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
    (void)mac_addr;
    Serial.printf("Send status: %s\n",
                  status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

bool sendServoMaskCommand(uint8_t slaveId, uint8_t servoMask, uint16_t holdMs) {
    if (slaveId >= 4) {
        Serial.printf("Wrong slave id: %u\n", slaveId);
        return false;
    }

    ServoCommand cmd;
    cmd.servoMask = servoMask;
    cmd.holdMs = holdMs;

    esp_err_t result = esp_now_send(
        SLAVE_MACS[slaveId],
        reinterpret_cast<const uint8_t*>(&cmd),
        sizeof(cmd)
    );

    if (result != ESP_OK) {
        Serial.printf("Send failed: slave=%u mask=0x%02X err=%d\n",
                      slaveId, servoMask, result);
        return false;
    }

    Serial.printf("Sent: slave=%u mask=0x%02X hold=%u\n",
                  slaveId, servoMask, holdMs);
    return true;
}

// --------------------------------------------------
// Ряды глобальной матрицы 4x4
//
// Физическая схема слейвов:
// [0][1]
// [2][3]
//
// Локальные сервы на слейве:
// 0 1
// 2 3
//
// Тогда:
// row 0 -> slave0: 0,1   + slave1: 0,1
// row 1 -> slave0: 2,3   + slave1: 2,3
// row 2 -> slave2: 0,1   + slave3: 0,1
// row 3 -> slave2: 2,3   + slave3: 2,3
// --------------------------------------------------

void runCurrentRowTest() {
    Serial.printf("TEST: global row %u\n", currentRow);

    switch (currentRow) {
        case 0:
            sendServoMaskCommand(0, 0b00000011, HOLD_MS); // servos 0,1
            sendServoMaskCommand(1, 0b00000011, HOLD_MS); // servos 0,1
            break;

        case 1:
            sendServoMaskCommand(0, 0b00001100, HOLD_MS); // servos 2,3
            sendServoMaskCommand(1, 0b00001100, HOLD_MS); // servos 2,3
            break;

        case 2:
            sendServoMaskCommand(2, 0b00000011, HOLD_MS); // servos 0,1
            sendServoMaskCommand(3, 0b00000011, HOLD_MS); // servos 0,1
            break;

        case 3:
            sendServoMaskCommand(2, 0b00001100, HOLD_MS); // servos 2,3
            sendServoMaskCommand(3, 0b00001100, HOLD_MS); // servos 2,3
            break;
    }

    currentRow = (currentRow + 1) % 4;
    Serial.printf("Next row will be: %u\n", currentRow);
}

// --------------------------------------------------
// Button helpers
// --------------------------------------------------

bool isButtonPressed() {
    return digitalRead(BUTTON_PIN) == LOW;
}

// --------------------------------------------------
// setup / loop
// --------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n=== MASTER START ===");

    pinMode(BUTTON_PIN, INPUT_PULLUP);

    WiFi.mode(WIFI_STA);

    Serial.print("Master MAC: ");
    Serial.println(WiFi.macAddress());

    if (esp_now_init() != ESP_OK) {
        Serial.println("esp_now_init failed");
        while (true) {
            delay(1000);
        }
    }

    esp_now_register_send_cb(onDataSent);

    for (int i = 0; i < 4; i++) {
        if (!addPeer(SLAVE_MACS[i])) {
            Serial.printf("addPeer failed: %d\n", i);
            while (true) {
                delay(1000);
            }
        }
    }

    Serial.println("ESP-NOW ready");
    Serial.println("Press button to open horizontal rows from top to bottom");
}

void loop() {
    if (isButtonPressed()) {
        delay(30);

        if (isButtonPressed()) {
            Serial.println("Button pressed");

            runCurrentRowTest();

            while (isButtonPressed()) {
                delay(10);
            }

            delay(30);
            Serial.println("Button released");
        }
    }

    delay(5);
}