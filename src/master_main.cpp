#include <Arduino.h>
#include "master/MasterNetwork.h"
#include "master/AnimationManager.h"

// Создаем объекты управления верхнего уровня абстракции
MasterNetwork network;
AnimationManager anim(network);

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== KINETIC WALL: MASTER BOOT ===");

    // Связываем триггеры веб-сервера сети напрямую с методами менеджера анимаций
    network.registerAnimationCallbacks(
        []() { anim.startWave(); },
        []() { anim.startRowsTest(); },
        []() { anim.runSlave2AllServos(); }
    );

    // Запуск сетевой периферии (Wi-Fi AP, ESP-NOW регистрация пиров, HTTP руты)
    network.begin();
    
    Serial.println("[System] Master successfully initialized.");
}

void loop() {
    network.handle(); // Обработка клиентов веб-сервера (Web Handlers)
    anim.update();    // Плавный расчет кадров анимации без задержек процессора
}