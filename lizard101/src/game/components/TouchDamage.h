#pragma once
#include "ecs/Component.h"
#include "EntityManager.h"
#include "Entity.h"

// Damages the Player's Health when this owner entity is touching the Player.
// Rate-limited by cooldownSeconds.
class TouchDamage : public ecs::Component {
public:
    TouchDamage(int damagePerHit = 1, float cooldownSeconds = 1.0f)
        : damagePerHit_(damagePerHit), cooldown_(cooldownSeconds) {
    }

    void onUpdate(float dt) override {
        timer_ += dt;

        // Find the player once per frame (fine for now, could cache later)
        Entity* player = EntityManager::getInstance().findEntityByName("Player");
        if (!player) return;

        // Owner is the spikes entity
        Entity* owner = getOwner();
        if (!owner) return;

        // Use your existing AABB check
        if (collisionDetection(*owner, *player)) {
            if (timer_ >= cooldown_) {
                // Damage if the player actually has a Health component
                if (auto* hp = player->getComponent<class Health>()) {
                    hp->damage(damagePerHit_);
                    SDL_Log("TouchDamage: Player HP -> %d", hp->get());
                }
                else {
                    SDL_Log("TouchDamage: Player has no Health component.");
                }
                timer_ = 0.0f; // reset cooldown
            }
        }
    }

private:
    int   damagePerHit_;
    float cooldown_;
    float timer_ = 0.0f;
};
