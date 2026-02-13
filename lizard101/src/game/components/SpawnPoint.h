#pragma once
#include "ecs/Component.h"
#include "Entity.h"

// SpawnPoint: marker component for spawn locations
class SpawnPoint : public ecs::Component {
    
public:
    SpawnPoint() {}
    void onStart() override {}
    void onUpdate(float dt) override {}
};
