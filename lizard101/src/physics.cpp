#include "physics.h"

EntityManager& manager = EntityManager::getInstance();


PhysicsSystem::PhysicsSystem(float gravity) : gravity(gravity) { }

void PhysicsSystem::updatePhysics(EntityManager& manager, float deltaTime) {
	for (auto* e : manager.entities) {
		if (e->physicsEnabled) {
			applyGravity(*e, deltaTime);
		}
	}
}

void PhysicsSystem::applyGravity(Entity& e, float deltaTime) {
	e.velY += gravity * deltaTime;

	e.y += e.velY * deltaTime;

	if (checkCollision(e)) {
		e.y = 1080.0f - e.height; // Reset position to ground level
		e.velY = 0.0f; // Reset vertical velocity
	}

}

bool PhysicsSystem::checkCollision(const Entity& e) {
	
	return (e.y + e.height) >= 1080.0f; // Temporarily set ground level at y = 1080 (bottom of the window)
}

void PhysicsSystem::setGravity(float g) {
	gravity = g;
}

float PhysicsSystem::getGravity() const {
	return gravity;
}