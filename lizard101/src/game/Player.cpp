#include "Player.h"
#include "Input.h"
#include "EntityManager.h"
#include "game/components/TextureRenderer.h"
#include "game/components/BoxCollider.h"
#include <variant>
#include <iostream>
#include <cmath>

Player::Player(std::string name, float x, float y, int w, int h, EventManager& evt)
    : Entity(std::move(name), x, y, w, h, /*physics*/true, /*solid*/false, "PLAYER"),
    eventMgr_(&evt) {
    // Subscribe to Input events (keyboard + drag)
    inputListenerId_ = eventMgr_->subscribe(EventType::Input, [this](const Event& e) {
        this->onEvent(e);
        });
}

void Player::startDash(int dir) {
    if (isDashing_) return;

    isDashing_ = true;
    dashTimer_ = dashDuration_;
    dashDir_ = dir;
}



void Player::onEvent(const Event& e) {
    if (e.type != EventType::Input) return;

    // Keyboard actions (WASD, jump, dash, etc.)
    if (const auto* act = std::get_if<InputAction>(&e.payload)) {
        using K = InputAction::Kind;
        switch (act->action) {
        case K::MoveLeft:
            controls_.left = act->pressed;
            break;

        case K::MoveRight:
            controls_.right = act->pressed;
            break;

        case K::Jump:
            if (act->pressed) {
                // edge-trigger jump request
                controls_.jumpRequest = true;
            }
            break;

            // Dash actions from input chord
        case K::DashLeft:
            if (act->pressed) {
                startDash(-1);
            }
            break;

        case K::DashRight:
            if (act->pressed) {
                startDash(+1);
            }
            break;

        case K::None:
        default:
            break;
        }
        return;
    }

    // Mouse dragging actions
    if (const auto* drag = std::get_if<DragInfo>(&e.payload)) {
        float halfW = width * 0.5f;
        float halfH = height * 0.5f;

        switch (drag->phase) {
        case DragInfo::Phase::Start: {
            const bool inside =
                (drag->x >= x && drag->x <= x + width &&
                    drag->y >= y && drag->y <= y + height);
            if (inside) {
                isDragging = true;
                physicsEnabled = false; // freeze physics while dragging
            }
            break;
        }
        case DragInfo::Phase::Move: {
            if (isDragging) {
                x = drag->x - halfW;
                y = drag->y - halfH;
            }
            break;
        }
        case DragInfo::Phase::End: {
            isDragging = false;
            physicsEnabled = true; // hand back to physics
            break;
        }
        }
    }
}

void Player::update(float deltaTime) {
    // --- Update dash timer ---
    if (isDashing_) {
        dashTimer_ -= deltaTime;
        if (dashTimer_ <= 0.0f) {
            isDashing_ = false;
            dashDir_ = 0;
        }
    }

    // Tick components (textures, colliders, etc.)
    Entity::update(deltaTime);

    physicsEnabled = !isDragging;

    const float jumpForce = jumpSpeed_;

    // --- Horizontal movement (dash overrides normal run) ---
    float horizVel = 0.0f;
    if (isDashing_) {
        horizVel = dashSpeed_ * static_cast<float>(dashDir_);
    }
    else {
        const int dir = (controls_.right ? 1 : 0) - (controls_.left ? 1 : 0);
        horizVel = static_cast<float>(dir) * moveSpeed_;
    }
    x += horizVel * deltaTime;

    // --- Grounded check ---
    grounded_ = false;
    float windowHeight = 1080.0f;

    if (auto* myCol = getComponent<BoxCollider>()) {
        static const char* kTags[] = { "GROUND", "PLATFORM", "MOVING_PLATFORM" };
        auto& mgr = EntityManager::getInstance();
        SDL_FRect myAABB = myCol->aabb();

        // Platform/ground check
        for (Entity* e : mgr.entities) {
            if (!e || e == this) continue;
            bool tagMatch = false;
            for (auto* t : kTags) { if (e->hasTag(t)) { tagMatch = true; break; } }
            if (!tagMatch) continue;

            if (auto* otherCol = e->getComponent<BoxCollider>()) {
                SDL_FRect otherAABB = otherCol->aabb();

                float feetY = myAABB.y + myAABB.h;
                bool horizontallyAligned =
                    (myAABB.x + myAABB.w > otherAABB.x) &&
                    (myAABB.x < otherAABB.x + otherAABB.w);
                bool feetOnPlatform =
                    horizontallyAligned &&
                    std::abs(feetY - otherAABB.y) < 2.0f &&
                    velY >= 0.0f;

                if (feetOnPlatform) {
                    grounded_ = true;
                    y = otherAABB.y - myAABB.h;
                    break;
                }
            }
        }

        // Window bottom check
        float feetY = myAABB.y + myAABB.h;
        if (!grounded_ && std::abs(feetY - windowHeight) < 2.0f && velY >= 0.0f) {
            grounded_ = true;
            y = windowHeight - myAABB.h;
        }
    }

    // std::cout << "[Player] velY: " << velY << " grounded: " << grounded_ << std::endl;

    // --- Jump edge-trigger
    if (controls_.jumpRequest && grounded_) {
        velY = jumpForce;
        controls_.jumpRequest = false;
        grounded_ = false;
    }

    // --- Gravity and vertical movement ---
    if (!grounded_) {
        velY += gravity_ * deltaTime;
    }
    else {
        velY = 0.0f;
    }
    y += velY * deltaTime;

    // --- Component-based collision vs. tagged platforms ---
    if (auto* myCol = getComponent<BoxCollider>()) {
        static const char* kTags[] = { "GROUND", "PLATFORM", "MOVING_PLATFORM" };
        auto& mgr = EntityManager::getInstance();
        for (Entity* e : mgr.entities) {
            if (!e || e == this) continue;
            bool tagMatch = false;
            for (auto* t : kTags) { if (e->hasTag(t)) { tagMatch = true; break; } }
            if (!tagMatch) continue;

            if (auto* otherCol = e->getComponent<BoxCollider>()) {
                myCol->resolveAgainst(*otherCol);
            }
        }
    }

    // --- Animation row selection ---
    int dirRow = 0;
    if (controls_.left)       dirRow = 1;
    else if (controls_.right) dirRow = 2;
    else                      dirRow = 0;

    if (auto* tr = getComponent<TextureRenderer>()) {
        tr->setRow(dirRow);
    }
}
