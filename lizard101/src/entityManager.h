#pragma once

#include <vector>
#include <string>
#include <SDL3/SDL.h>

class Entity;

class EntityManager {
public:
    static EntityManager& getInstance();

    struct Deleter {
        void (*fn)(void* ctx, Entity* e);
        void* ctx;
    };

    ~EntityManager();

    void addEntity(Entity* entity);

    void addEntity(Entity* entity, Deleter deleter);

    void removeEntity(Entity* entity);

    void updateAll(float deltaTime);
    void renderAll(SDL_Renderer* renderer);

    void addEntityToFront(Entity* entity);
    void addEntityToFront(Entity* entity, Deleter deleter);

    Entity* findEntityByName(const std::string& name);

    void destroyAll();

    std::vector<Entity*> entities;

private:
    EntityManager() = default;
    EntityManager(const EntityManager&) = delete;
    EntityManager& operator=(const EntityManager&) = delete;

    struct Entry {
        Entity* ptr;
        Deleter deleter;
    };

    std::vector<Entry> entries_;

    static void defaultDelete(void* ctx, Entity* e);
    void refreshEntityView();
};
