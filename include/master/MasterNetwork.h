#pragma once
#include <WebServer.h>
#include <functional>
#include "config.h"
#include "protocol.h"
#include "matrix_map.h"
#include "esp_now.h"

class MasterNetwork
{
public:
    MasterNetwork();
    void begin();
    void handle();

    // Интерфейс отправки команд слейвам
    bool sendServoMaskCommand(uint8_t slaveId, uint8_t servoMask, uint16_t holdMs);
    void broadcastMatrix(uint8_t m0, uint8_t m1, uint8_t m2, uint8_t m3, uint16_t hold);
    void setCell(uint8_t row, uint8_t col, uint16_t holdMs);

    // Регистрация коллбэков для связи веб-сервера и менеджера анимаций
    void registerAnimationCallbacks(
        std::function<void()> onWave,
        std::function<void()> onRows,
        std::function<void()> onExplosion,
        std::function<void()> onSlave2);

    std::function<void()> _onExplosionStart = nullptr;

private:
    WebServer _server;

    std::function<void()> _onWaveStart = nullptr;
    std::function<void()> _onRowsStart = nullptr;
    std::function<void()> _onSlave2Start = nullptr;

    void setupWiFi();
    void setupEspNow();
    void registerHttpRoutes();
    bool addPeer(const uint8_t mac[6]);

    // Необходим статический метод для коллбэка отправки ESP-NOW
    static void onDataSent(const esp_now_send_info_t *tx_info, esp_now_send_status_t status);
};