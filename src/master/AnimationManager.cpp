#include "master/AnimationManager.h"

// Тайминги из оригинального файла
static constexpr uint16_t WAVE_STEP_DELAY_MS = 370;

AnimationManager::AnimationManager(MasterNetwork& network) : _net(network) {}

void AnimationManager::update() {
    if (_currentAnim == ANIM_NONE) return;

    switch (_currentAnim) {
        case ANIM_WAVE:
            tickWave();
            break;
        case ANIM_ROWS:
            tickRows();
            break;
        default:
            break;
    }
}

void AnimationManager::startWave() {
    Serial.println("[Anim] Run WAVE start");
    _currentAnim = ANIM_WAVE;
    _currentStep = 0;
    _lastStepTime = millis();
    tickWave(); // Запускаем первый шаг немедленно
}

void AnimationManager::startRowsTest() {
    Serial.println("[Anim] Run ROWS x3 start");
    _currentAnim = ANIM_ROWS;
    _currentStep = 0;
    _repeatCount = 0;
    _lastStepTime = millis();
    tickRows();
}

void AnimationManager::runSlave2AllServos() {
    Serial.println("[Anim] Triggering all servos on Slave 2");
    _net.sendServoMaskCommand(2, 0b00001111, HOLD_MS);
}

void AnimationManager::stopAll() {
    _currentAnim = ANIM_NONE;
    _net.broadcastMatrix(0, 0, 0, 0, 0);
}

void AnimationManager::tickWave() {
    // Проверка интервала времени для неблокирующего шага волны
    if (_currentStep > 0 && (millis() - _lastStepTime < WAVE_STEP_DELAY_MS)) {
        return;
    }

    // Для матрицы 4x4 сумма индексов (row + col) дает диагональ (от 0 до 6)
    if (_currentStep < 7) {
        for (uint8_t r = 0; r < 4; r++) {
            for (uint8_t c = 0; c < 4; c++) {
                if (r + c == _currentStep) {
                    _net.setCell(r, c, HOLD_MS);
                }
            }
        }
        _lastStepTime = millis();
        _currentStep++;
    } else {
        Serial.println("[Anim] Wave finished");
        _currentAnim = ANIM_NONE;
    }
}

void AnimationManager::tickRows() {
    // Проверка интервала времени между рядами
    if (_currentStep > 0 && (millis() - _lastStepTime < STEP_DELAY_MS)) {
        return;
    }

    if (_repeatCount < 3) {
        if (_currentStep < 4) {
            // Активируем всю строку целиком на текущем шаге
            for (uint8_t c = 0; c < 4; c++) {
                _net.setCell(_currentStep, c, HOLD_MS);
            }
            _lastStepTime = millis();
            _currentStep++;
        } else {
            _currentStep = 0;
            _repeatCount++;
        }
    } else {
        Serial.println("[Anim] Rows test finished");
        _currentAnim = ANIM_NONE;
    }
}