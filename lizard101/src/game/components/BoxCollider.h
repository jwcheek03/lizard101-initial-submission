#pragma once
#include <SDL3/SDL.h>
#include <cmath>           // for std::fabs
#include "ecs/Component.h"
#include "entity.h"
#include "events/Event.h"
#include "events/EventManager.h"
#include "main.h"          // for EVENT_LOG toggle/helper

// Simple AABB collider tied to an Entity.
// By default, collider size == entity size, with an optional offset.
// You can also provide a custom width/height.
class BoxCollider : public ecs::Component {
public:
    // Uses entity's width/height with optional offsets
    explicit BoxCollider(float offsetX = 0.f, float offsetY = 0.f)
        : offsetX_(offsetX), offsetY_(offsetY), useCustom_(false) {
    }

    // Custom-size collider with offset
    BoxCollider(float offsetX, float offsetY, int width, int height)
        : offsetX_(offsetX), offsetY_(offsetY),
        customW_(width), customH_(height), useCustom_(true) {
    }

    // Inject the EventManager so we can raise Collision events
    void setEventManager(EventManager* ev) { events_ = ev; }

    // World-space AABB for this collider
    SDL_FRect aabb() const {
        const Entity* e = getOwner();
        SDL_FRect r{
            e->getX() + offsetX_,
            e->getY() + offsetY_,
            static_cast<float>(useCustom_ ? customW_ : e->getWidth()),
            static_cast<float>(useCustom_ ? customH_ : e->getHeight())
        };
        return r;
    }

    // Resolve overlap against another collider (using minimal push-out).
    // If vertical resolution occurs, zero out vertical velocity on the owner.
    // Also raises a Collision event via EventManager if available.
    bool resolveAgainst(const BoxCollider& other) {
        Entity* a = getOwner();
        Entity* b = other.getOwner();
        if (!a || !b) return false;

        SDL_FRect A = aabb();
        SDL_FRect B = other.aabb();

        // No overlap
        const bool sep =
            (A.x + A.w) <= B.x ||
            (B.x + B.w) <= A.x ||
            (A.y + A.h) <= B.y ||
            (B.y + B.h) <= A.y;
        if (sep) return false;

        // Compute minimal translation vector
        float axCenter = A.x + A.w * 0.5f;
        float ayCenter = A.y + A.h * 0.5f;
        float bxCenter = B.x + B.w * 0.5f;
        float byCenter = B.y + B.h * 0.5f;

        float dx = axCenter - bxCenter;
        float dy = ayCenter - byCenter;

        float px = (A.w * 0.5f + B.w * 0.5f) - std::fabs(dx);
        float py = (A.h * 0.5f + B.h * 0.5f) - std::fabs(dy);

        if (px < py) {
            // Resolve on X
            float sx = (dx < 0.f) ? -1.f : 1.f;
            a->x += sx * px;
        }
        else {
            // Resolve on Y
            float sy = (dy < 0.f) ? -1.f : 1.f;
            a->y += sy * py;
            a->velY = 0.f; // snap ground/ceiling stops vertical motion
        }

        // Raise a collision event (and print a short line if debug is enabled)
        if (events_) {
            EVENT_LOG(std::string("[raise] Collision ") + a->name + " vs " + b->name);
            events_->raise(Event{
                EventType::Collision,
                EventPriority::High,
                0.0f,
                EventPayload{ CollisionInfo{ a, b } }
                });
        }
        return true;
    }

private:
    float offsetX_ = 0.f, offsetY_ = 0.f;
    int   customW_ = 0, customH_ = 0;
    bool  useCustom_ = false;

    EventManager* events_ = nullptr;
};
