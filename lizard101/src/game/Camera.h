#pragma once
#include <SDL3/SDL.h>

// Simple Camera singleton storing a world-to-screen offset (top-left of camera in world coords)
class Camera {

public:
    static Camera& getInstance() {
        static Camera inst;
        return inst;
    }

    float getX() const { 
        return x_; 
    }

    float getY() const { 
        return y_; 
    }

    void setPosition(float x, float y) { 
        x_ = clampX(x); y_ = clampY(y); 
    }

    void translate(float dx, float dy) { 
        x_ = clampX(x_ + dx); y_ = clampY(y_ + dy); 
    }

    // Functions to help the camera smoothly translate to a new position over durationSec seconds
    void smoothTranslate(float dx, float dy, float durationSec) {
        if (durationSec <= 0.0f) { 
            translate(dx, dy); 
            return; 
        }

        startX_ = x_; startY_ = y_;
        targetX_ = clampX(x_ + dx);
        targetY_ = clampY(y_ + dy);
        elapsed_ = 0.0f;
        duration_ = durationSec;
        smoothing_ = true;
    }

    void smoothTo(float x, float y, float durationSec) {
        if (durationSec <= 0.0f) { 
            setPosition(x, y); 
            return; 
        }

        startX_ = x_; startY_ = y_;
        targetX_ = clampX(x);
        targetY_ = clampY(y);
        elapsed_ = 0.0f;
        duration_ = durationSec;
        smoothing_ = true;
    }

    // Call every frame with delta time to progress smoothing
    void update(float dt) {
        if (!smoothing_) 
            return;

        elapsed_ += dt;
        float t = (duration_ > 0.0f) ? (elapsed_ / duration_) : 1.0f;
        if (t >= 1.0f) {
            x_ = targetX_; 
            y_ = targetY_; 
            smoothing_ = false; 
            return;
        }

        // simple ease-in-out (smoothstep)
        float u = t * t * (3.0f - 2.0f * t);
        x_ = startX_ + (targetX_ - startX_) * u;
        y_ = startY_ + (targetY_ - startY_) * u;
    }


    void setBounds(float minX, float minY, float maxX, float maxY) {
        minX_ = minX; 
        minY_ = minY; 
        maxX_ = maxX; 
        maxY_ = maxY;
        x_ = clampX(x_);
        y_ = clampY(y_);
    }

    // expose bounds
    float getMinX() const { 
        return minX_; 
    }

    float getMaxX() const { 
        return maxX_; 
    }

    float getMinY() const { 
        return minY_; 
    }

    float getMaxY() const { 
        return maxY_; 
    }

    // Convert world rect to screen rect
    SDL_FRect worldToScreen(const SDL_FRect& world) const {
        return SDL_FRect{ world.x - x_, world.y - y_, world.w, world.h };
    }

private:
    Camera() = default;
    float x_ = 0.0f;
    float y_ = 0.0f;
    float minX_ = 0.0f;
    float minY_ = 0.0f;
    float maxX_ = 1e9f;
    float maxY_ = 1e9f;

    // Store bounds and positions of the camera
    float clampX(float x) const {
        if (x < minX_) 
            return minX_;
        if (x > maxX_) 
            return maxX_;

        return x;
    }

    float clampY(float y) const {
        if (y < minY_) 
            return minY_;
        if (y > maxY_) 
            return maxY_;
        
        return y;
    }

    // smoothing state
    bool smoothing_ = false;
    float startX_ = 0.0f;
    float startY_ = 0.0f;
    float targetX_ = 0.0f;
    float targetY_ = 0.0f;
    float elapsed_ = 0.0f;
    float duration_ = 0.0f;
};
