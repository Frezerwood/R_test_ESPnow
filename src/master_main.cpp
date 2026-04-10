#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "config.h"
#include "protocol.h"

// --------------------------------------------------
// Настройки кнопки
// --------------------------------------------------

static constexpr uint8_t BUTTON_PIN = 18;

// пауза между диагоналями wave
static constexpr uint16_t WAVE_STEP_DELAY_MS = 370;

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
// Mapping: global 4x4 -> slave + local servo
//
// Раскладка слейвов:
// [0][1]
// [2][3]
//
// Локальные сервы на каждом слейве:
// 0 1
// 2 3
// --------------------------------------------------

void mapGlobalCellToSlave(
    uint8_t row,
    uint8_t col,
    uint8_t& slaveId,
    uint8_t& servoIndex
) {
    slaveId = (row / 2) * 2 + (col / 2);

    uint8_t localRow = row % 2;
    uint8_t localCol = col % 2;

    servoIndex = localRow * 2 + localCol;
}

// --------------------------------------------------
// Wave: диагонали сверху-слева -> вниз-вправо
// --------------------------------------------------

void runWaveOnce() {
    Serial.println("Run WAVE start");

    // Для матрицы 4x4 диагоналей 7: sum = 0..6
    for (uint8_t diag = 0; diag <= 6; diag++) {
        uint8_t slaveMasks[4] = {0, 0, 0, 0};

        // Собираем все клетки, где row + col == diag
        for (uint8_t row = 0; row < GLOBAL_ROWS; row++) {
            for (uint8_t col = 0; col < GLOBAL_COLS; col++) {
                if ((row + col) != diag) {
                    continue;
                }

                uint8_t slaveId = 0;
                uint8_t servoIndex = 0;
                mapGlobalCellToSlave(row, col, slaveId, servoIndex);

                slaveMasks[slaveId] |= (1 << servoIndex);
            }
        }

        Serial.printf("Diag %u -> masks: [%02X %02X %02X %02X]\n",
                      diag,
                      slaveMasks[0],
                      slaveMasks[1],
                      slaveMasks[2],
                      slaveMasks[3]);

        for (uint8_t slaveId = 0; slaveId < 4; slaveId++) {
            if (slaveMasks[slaveId] != 0) {
                sendServoMaskCommand(slaveId, slaveMasks[slaveId], HOLD_MS);
            }
        }

        delay(WAVE_STEP_DELAY_MS);
    }

    Serial.println("Run WAVE done");
}

// --------------------------------------------------
// All On 2 slave
// --------------------------------------------------
void runSlave2AllServos() {
    Serial.println("Run SLAVE 2 ALL SERVOS");
    sendServoMaskCommand(2, 0b00001111, HOLD_MS);
} 

// --------------------------------------------------
// Row test
// --------------------------------------------------
void runRows3Times() {
    Serial.println("Run ROWS x3 start");

    for (int repeat = 0; repeat < 3; repeat++) {
        Serial.printf("Cycle %d\n", repeat + 1);

        for (uint8_t row = 0; row < 4; row++) {

            switch (row) {
                case 0:
                    sendServoMaskCommand(0, 0b00000011, HOLD_MS);
                    sendServoMaskCommand(1, 0b00000011, HOLD_MS);
                    break;

                case 1:
                    sendServoMaskCommand(0, 0b00001100, HOLD_MS);
                    sendServoMaskCommand(1, 0b00001100, HOLD_MS);
                    break;

                case 2:
                    sendServoMaskCommand(2, 0b00000011, HOLD_MS);
                    sendServoMaskCommand(3, 0b00000011, HOLD_MS);
                    break;

                case 3:
                    sendServoMaskCommand(2, 0b00001100, HOLD_MS);
                    sendServoMaskCommand(3, 0b00001100, HOLD_MS);
                    break;
            }

            delay(900); // можно подкрутить
        }
    }

    Serial.println("Run ROWS x3 done");
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
    Serial.println("Press button to run wave");
}

void loop() {
    if (isButtonPressed()) {
        delay(30);

        if (isButtonPressed()) {
            Serial.println("Button pressed");

            runRows3Times();

            while (isButtonPressed()) {
                delay(10);
            }

            delay(30);
            Serial.println("Button released");
        }
    }

    delay(5);
}