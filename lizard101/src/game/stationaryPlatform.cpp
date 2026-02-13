#include "StationaryPlatform.h"
#include "EntityManager.h"

StationaryPlatform::StationaryPlatform(std::string name, float x, float y,
    int w, int h)
    : Entity(std::move(name), x, y, w, h,
        /*tex*/ nullptr,
        /*physics*/ false,
        /*solid*/ true,
        "STATIONARY_PLATFORM") {
}

void StationaryPlatform::update(float deltaTime) {
    (void)deltaTime;
    Entity::update(deltaTime);
}
