#pragma once
#include "ecs/Component.h"
#include "../Camera.h"

// ScreenAnchor: keeps an entity at a fixed screen position by updating its world position each frame
class ScreenAnchor : public ecs::Component {

public:
    ScreenAnchor(float screenX = 0.0f, float screenY = 0.0f) : screenX_(screenX), screenY_(screenY) {}

    void onUpdate(float dt) override {

        // Update the owner's world position based on the screen position and camera offset
        Entity* owner = getOwner(); 
        
        if (!owner) {
            return;
        }

        owner->x = screenX_ + Camera::getInstance().getX();
        owner->y = screenY_ + Camera::getInstance().getY();
    }

    float getScreenX() const { 
        return screenX_; 
    }

    float getScreenY() const { 
        return screenY_; 
    }

    void setScreenX(float sx) { 
        screenX_ = sx; 
    }

    void setScreenY(float sy) { 
        screenY_ = sy; 
    }

private:
    float screenX_ = 0.0f;
    float screenY_ = 0.0f;
};
