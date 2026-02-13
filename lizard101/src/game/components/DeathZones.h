#pragma once
#include "ecs/Component.h"
#include "EntityManager.h"
#include "Entity.h"
#include "../Camera.h"
#include <string>
#include <cmath>
#include <limits>

// DeathZone component: when the player collides with the entity, the player will be teleported to a designated spawn point by name,
// or to the nearest Spawn entity if no name is provided or not found.
class DeathZone : public ecs::Component {

public:
	// If spawnName is empty, the nearest entity with tag "SPAWN" will be used.
	DeathZone(const std::string& spawnName = "") : spawnName_(spawnName) {}

	void setEventManager(EventManager* ev) { events_ = ev; }

	void onUpdate(float dt) override {
		Entity* player = EntityManager::getInstance().findEntityByName("Player");
		if (!player) return;

		Entity* owner = getOwner();
		if ( !owner ) return;

		// In networked mode (events_ is set), only raise events; don't teleport locally.
		// In single-player mode (events_ is nullptr), handle respawn locally.
		if (!events_) return;  // In networked context, skip local handling

			// Prevent retriggering if just respawned
			if (respawnCooldownActive_) {
				respawnTimer_ += dt;
				if (respawnTimer_ >= 0.5f) {
					respawnCooldownActive_ = false;
					respawnTimer_ = 0.0f;
				} else {
					return;
				}
			}
		const bool currentlyInside = collisionDetection(*owner, *player);
		if (currentlyInside && !playerWasInside_) {
			// Entered the zone this frame
			events_->raise(Event{
				EventType::Death,
				EventPriority::High,
				0.0f,
				EventPayload{ DeathInfo{ player } }
			});
			// Activate cooldown to prevent immediate re-triggering while server processes respawn
			respawnCooldownActive_ = true;
			respawnTimer_ = 0.0f;
		}
		playerWasInside_ = currentlyInside;
	}

	private:
		void teleportPlayer(Entity* player) {
			Entity* spawn = nullptr;

			// Try to find spawn by name
			if (!spawnName_.empty()) {
				spawn = EntityManager::getInstance().findEntityByName(spawnName_);
			}

			// If not found by name, find the nearest spawn with "SPAWN" tag
			if (!spawn) {

				// Find the nearest spawn point
				float nearestSpawn = std::numeric_limits<float>::max();

				for (auto* entities : EntityManager::getInstance().entities) {
					if (!entities) {
						continue;
					}
                    
					// Check for "SPAWN" tag
					if (entities->hasTag("SPAWN")) {
						float dx = (entities->x + entities->width*0.5f) - (player->x + player->width*0.5f);
						float dy = (entities->y + entities->height*0.5f) - (player->y + player->height*0.5f);
						float d = std::sqrt(dx*dx + dy*dy);
						if (d < nearestSpawn) { 
							nearestSpawn = d; 
							spawn = entities; 
						}
					}
				}
			}

			// Teleport player to spawn point
			if (spawn) {
				player->x = spawn->x;
				player->y = spawn->y;
				player->velY = 0.0f;
					respawnCooldownActive_ = true;
					respawnTimer_ = 0.0f;
				Camera::getInstance().smoothTo(spawn->x - 200.0f, spawn->y - 100.0f, 0.4f);
			}
		}

		std::string spawnName_;
		EventManager* events_ = nullptr;
		bool playerWasInside_ = false;
		bool respawnCooldownActive_ = false;
		float respawnTimer_ = 0.0f;
	};

