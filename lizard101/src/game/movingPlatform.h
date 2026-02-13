#pragma once
#include "entity.h"

class MovingPlatform : public Entity {
public:
    MovingPlatform(std::string name, float x, float y, int w, int h,
        bool horizontal, float speed, float range);

    void update(float deltaTime) override;

private:
    bool  isMoving = true;
    bool  moveHorizontal = true;
    float moveSpeed = 100.0f;
    float moveRange = 200.0f;
    float startX = 0.f;
    float startY = 0.f;
    int   direction = 1; // 1 or -1
};
