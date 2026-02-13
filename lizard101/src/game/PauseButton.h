#pragma once
#include "Entity.h"
#include "Timeline.h"
#include <functional>

class PauseButton : public Entity {
public:
    PauseButton(std::string name, float screenX, float screenY, int w, int h, Timeline* timeline);

    void update(float deltaTime) override;

private:
    Timeline* timeline = nullptr;
    bool wasPressed = false;
    // desired fixed screen position for this UI button
    float screenX_ = 0.0f;
    float screenY_ = 0.0f;

public:
    static std::function<void()> onPauseRequested;
};
