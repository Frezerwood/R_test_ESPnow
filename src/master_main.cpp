#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "config.h"
#include "protocol.h"

// --------------------------------------------------
// Настройки кнопки
// --------------------------------------------------

static constexpr uint8_t BUTTON_PIN = 18;

// текущий слейв (0..3)
uint8_t currentSlave = 0;

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

bool sendServoCommand(uint8_t slaveId, uint8_t servoIndex, uint16_t holdMs) {
    if (slaveId >= 4) {
        Serial.printf("Wrong slave id: %u\n", slaveId);
        return false;
    }

    ServoCommand cmd;
    cmd.servoIndex = servoIndex;
    cmd.holdMs = holdMs;

    esp_err_t result = esp_now_send(
        SLAVE_MACS[slaveId],
        reinterpret_cast<const uint8_t*>(&cmd),
        sizeof(cmd)
    );

    if (result != ESP_OK) {
        Serial.printf("Send failed: slave=%u err=%d\n",
                      slaveId, result);
        return false;
    }

    Serial.printf("Sent: slave=%u ALL servos hold=%u\n",
                  slaveId, holdMs);
    return true;
}

// --------------------------------------------------
// TEST: открыть все сервы на текущем слейве
// --------------------------------------------------

void runCurrentSlaveTest() {
    Serial.printf("TEST: slave %u -> ALL servos\n", currentSlave);

    sendServoCommand(currentSlave, SERVO_INDEX_ALL, HOLD_MS);

    // переключаем на следующий
    currentSlave = (currentSlave + 1) % 4;

    Serial.printf("Next slave will be: %u\n", currentSlave);
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
    Serial.println("Press button to cycle slaves");
}

void loop() {
    if (isButtonPressed()) {
        delay(30);

        if (isButtonPressed()) {
            Serial.println("Button pressed");

            runCurrentSlaveTest();

            while (isButtonPressed()) {
                delay(10);
            }

            delay(30);
            Serial.println("Button released");
        }
    }

    delay(5);
}