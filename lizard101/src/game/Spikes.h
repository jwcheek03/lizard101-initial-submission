#pragma once
#include "Entity.h"
#include <SDL3/SDL.h>

class Spikes : public Entity {
public:
    Spikes(std::string name, float x, float y, int w, int h)
        : Entity(std::move(name), x, y, w, h,
            /*tex*/ nullptr,
            /*physics*/ false,
            /*isSolid*/ false,
            "HAZARD") {
        setTag("HAZARD");
    }
    ~Spikes() override = default;

    void update(float dt) override {
        updateComponents(dt);
    }
};
