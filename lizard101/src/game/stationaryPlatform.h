#pragma once
#include "entity.h"

class StationaryPlatform : public Entity {
public:
    StationaryPlatform(std::string name, float x, float y, int w, int h);
    void update(float deltaTime) override;
};
