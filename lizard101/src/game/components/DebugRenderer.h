#pragma once
#include "ecs/Component.h"
#include <SDL3/SDL.h>
#include "../Camera.h"

// DebugRenderer: draws a filled rectangle of a given color for testing hidden objects
class DebugRenderer : public ecs::Component {
public:
    DebugRenderer(SDL_Color color = {255,0,255,128}) : color_(color) {}

    static void setEnabled(bool e) { 
        enabled_ = e; 
    }

    static bool isEnabled() { 
        return enabled_; 
    }

    // Render a filled rectangle over the owner's position
    void onRender(SDL_Renderer* renderer) override {
    if (!enabled_) return;
    Entity* owner = getOwner();
    if (!owner) 
        return;

    SDL_FRect worldDst{ owner->getX(), owner->getY(), static_cast<float>(owner->getWidth()), static_cast<float>(owner->getHeight()) };
    SDL_FRect dstF = Camera::getInstance().worldToScreen(worldDst);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, color_.r, color_.g, color_.b, color_.a);
    SDL_RenderFillRect(renderer, &dstF);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    }

private:
    SDL_Color color_;
    static inline bool enabled_ = true;
};
