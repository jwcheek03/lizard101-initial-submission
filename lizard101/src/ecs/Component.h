#pragma once
#include <SDL3/SDL.h>

// Forward declare the engine's Entity type
struct Entity;

namespace ecs {

    class Component {
    public:
        virtual ~Component() {}
        virtual void onStart() {}
        virtual void onUpdate(float /*dt*/) {}
        virtual void onRender(SDL_Renderer* /*renderer*/) {}

        Entity* getOwner() const { return owner_; }

    private:
        friend struct ::Entity;
        void attachOwner(::Entity* e) { owner_ = e; }

        Entity* owner_ = nullptr;
    };

} // namespace ecs
