// src/game/components/Health.h
#pragma once
#include "ecs/Component.h"

class Health : public ecs::Component {
public:
    explicit Health(int hp = 100) : hp_(hp) {}

    int  get()     const { return hp_; }
    void damage(int d) { hp_ = (hp_ - d < 0) ? 0 : hp_ - d; }

private:
    int hp_;
};
