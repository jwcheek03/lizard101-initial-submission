#pragma once
#include <SDL3/SDL.h>
#include "Entity.h"
#include "EntityManager.h"

class PhysicsSystem {
public:

    PhysicsSystem(float gravity = 980.0f);
    void updatePhysics(EntityManager& manager, float deltaTime);
    void applyGravity(Entity& e, float deltaTime);
    bool checkCollision(const Entity& e);
    void setGravity(float g);
    float getGravity() const;

private:
    float gravity;
};