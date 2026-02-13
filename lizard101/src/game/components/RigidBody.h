#pragma once
#include "ecs/Component.h"
#include "entity.h"

// Lightweight physics marker/config. Uses the existing PhysicsSystem:
// - If dynamic = true: enables physics on the owner (physicsEnabled = true).
// - gravityScale is available for future extendability
class RigidBody : public ecs::Component {
public:
    explicit RigidBody(bool dynamic = true, float gravityScale = 1.0f)
        : dynamic_(dynamic), gravityScale_(gravityScale) {
    }

    void onStart() override {
        if (dynamic_) {
            // Make sure the physics system considers this entity.
            if (auto* e = getOwner()) {
                e->physicsEnabled = true;
            }
        }
    }

    // Simple API if you want to toggle later
    void setDynamic(bool dyn) {
        dynamic_ = dyn;
        if (auto* e = getOwner()) {
            e->physicsEnabled = dyn;
        }
    }
    bool isDynamic() const { return dynamic_; }

    void setGravityScale(float g) { gravityScale_ = g; }
    float gravityScale() const { return gravityScale_; }

private:
    bool  dynamic_ = true;
    float gravityScale_ = 1.0f;
};
