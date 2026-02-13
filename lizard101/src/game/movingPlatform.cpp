#include "movingPlatform.h"
#include "EntityManager.h"

MovingPlatform::MovingPlatform(std::string name, float x, float y, int w, int h,
    bool horizontal, float speed, float range)
    : Entity(std::move(name), x, y, w, h,
        /*tex*/ nullptr,
        /*physics*/ false,
        /*solid*/ true,
        "MOVING_PLATFORM")
{
    isMoving = true;
    moveHorizontal = horizontal;
    moveSpeed = speed;
    moveRange = range;
    startX = x;
    startY = y;
    direction = 1;
}

void MovingPlatform::update(float deltaTime) {
    (void)deltaTime; // keep your existing update logic here if any

    if (!isMoving) return;

    if (moveHorizontal) {
        x += moveSpeed * deltaTime * direction;
        if (x >= startX + moveRange) { x = startX + moveRange; direction = -1; }
        else if (x <= startX) { x = startX;             direction = 1; }
    }
    else {
        y += moveSpeed * deltaTime * direction;
        if (y >= startY + moveRange) { y = startY + moveRange; direction = -1; }
        else if (y <= startY) { y = startY;             direction = 1; }
    }
}
