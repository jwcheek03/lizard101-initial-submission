// Entity.h
#pragma once
#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include "ecs/Component.h"
#include "ecs/ComponentType.h"
#include "ecs/Tag.h"

struct Entity {
    // identity & transform
    std::string name;
    std::string type;
    float x = 0.0f;
    float y = 0.0f;
    int   width = 32;
    int   height = 32;

    // physics flags
    bool  physicsEnabled = false;
    float velY = 0.0f;
    bool  isSolid = false;

    // tag (mirrors type)
    ecs::Tag tag;

    // Primary ctor
    Entity(std::string name, float posX, float posY, int w, int h,
        bool physicsEnabled = false, bool isSolid = false,
        std::string entType = "GENERIC")
        : name(std::move(name)), type(std::move(entType)),
        x(posX), y(posY), width(w), height(h),
        physicsEnabled(physicsEnabled), isSolid(isSolid) {
    }

    // COMPAT ctor to keep existing code compiling (ignores 'tex')
    Entity(std::string name, float posX, float posY, int w, int h,
        SDL_Texture* /*tex*/, bool physicsEnabled = false, bool isSolid = false,
        std::string entType = "GENERIC")
        : Entity(std::move(name), posX, posY, w, h, physicsEnabled, isSolid, std::move(entType)) {
    }

    virtual ~Entity() = default;

    virtual void update(float deltaTime) {
        (void)deltaTime;
        updateComponents(deltaTime);
    }

    // tags
    void setTag(const std::string& t) { tag = ecs::Tag(t); type = t; }
    const ecs::Tag& getTag() const { return tag; }
    bool hasTag(const std::string& t) const { return tag.is(t); }

    // component management
    template <typename T, typename... Args>
    T& addComponent(Args&&... args) {
        const ComponentTypeId tid = componentTypeId<T>();
        auto it = compIndex.find(tid);
        if (it != compIndex.end()) return *static_cast<T*>(it->second);

        std::unique_ptr<T> ptr(new T(std::forward<Args>(args)...));
        T* raw = ptr.get();
        raw->attachOwner(this);
        components.emplace_back(std::move(ptr));
        compIndex[tid] = raw;
        raw->onStart();
        return *raw;
    }

    template <typename T>
    T* getComponent() const {
        const ComponentTypeId tid = componentTypeId<T>();
        auto it = compIndex.find(tid);
        if (it == compIndex.end()) return nullptr;
        return static_cast<T*>(it->second);
    }

    void updateComponents(float dt) {
        for (auto& c : components) c->onUpdate(dt);
    }

    void renderComponents(SDL_Renderer* renderer) {
        for (auto& c : components) c->onRender(renderer);
    }

    // for components
    float getX() const { return x; }
    float getY() const { return y; }
    int   getWidth() const { return width; }
    int   getHeight() const { return height; }

private:
    std::vector<std::unique_ptr<ecs::Component>> components;
    std::unordered_map<ComponentTypeId, ecs::Component*> compIndex;
};

// helpers
inline SDL_FRect getBounds(const Entity& e) {
    return SDL_FRect{ e.x, e.y, static_cast<float>(e.width), static_cast<float>(e.height) };
}

// collision helper (decl only, impl in entity.cpp)
bool collisionDetection(Entity& a, Entity& b);
