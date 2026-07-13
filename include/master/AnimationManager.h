#pragma once
#include "master/MasterNetwork.h"

enum AnimationType {
    ANIM_NONE,
    ANIM_WAVE,
    ANIM_ROWS
};

class AnimationManager {
public:
    AnimationManager(MasterNetwork& network);
    
    void update(); // Вызывать в основном loop()
    
    void startWave();
    void startRowsTest();
    void runSlave2AllServos();
    void stopAll();

private:
    MasterNetwork& _net;
    AnimationType _currentAnim = ANIM_NONE;
    
    unsigned long _lastStepTime = 0;
    int _currentStep = 0;
    int _repeatCount = 0;

    void tickWave();
    void tickRows();
};