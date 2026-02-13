#pragma once
#include "../entity.h"
#include "events/Event.h"
#include "events/EventManager.h"

class Player : public Entity {
public:
    Player(std::string name, float x, float y, int w, int h, EventManager& evt);

    void update(float deltaTime) override;

    // Event callback
    void onEvent(const Event& e);

    // Allow external systems (physics) to set grounded state
    void setGrounded(bool g) { grounded_ = g; }


    void setLeft(bool v) { controls_.left = v; }
    void setRight(bool v) { controls_.right = v; }
    void setJumpRequest(bool v) { controls_.jumpRequest = v; }

private:
    // Event-driven input state
    struct ControlState {
        bool left = false;
        bool right = false;
        bool jumpRequest = false; // edge-triggered
    } controls_;

    void startDash(int dir); // -1 or +1


    // Drag state (driven by DragInfo event)
    bool isDragging = false;

    // Movement tuning
    float moveSpeed_ = 300.0f;   // px/s
    float jumpSpeed_ = -600.0f;  // px/s
    float gravity_ = 900.0f;  // px/s^2 (unused unless using custom gravity)

    float vx_ = 0.0f;
    float vy_ = 0.0f;
    bool grounded_ = true;

    // Dash tuning
    float dashSpeed_ = 2000.0f;  // horizontal speed during dash
    float dashDuration_ = 0.20f;   // seconds
    float dashTimer_ = 0.0f;
    int   dashDir_ = 0;       // -1 = left, +1 = right, 0 = no dash
    bool  isDashing_ = false;


    EventManager* eventMgr_ = nullptr;
    EventManager::ListenerId inputListenerId_ = 0;
};
