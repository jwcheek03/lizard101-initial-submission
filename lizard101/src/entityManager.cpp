#include "EntityManager.h"
#include "Entity.h"
#include <algorithm>

EntityManager& EntityManager::getInstance() {
    static EntityManager instance;
    return instance;
}

EntityManager::~EntityManager() {
    destroyAll();
}

void EntityManager::defaultDelete(void* /*ctx*/, Entity* e) {
    delete e;
}

void EntityManager::refreshEntityView() {
    entities.clear();
    entities.reserve(entries_.size());
    for (auto& entry : entries_) {
        entities.push_back(entry.ptr);
    }
}

void EntityManager::destroyAll() {
    for (auto& entry : entries_) {
        if (entry.ptr && entry.deleter.fn) {
            entry.deleter.fn(entry.deleter.ctx, entry.ptr);
        }
    }
    entries_.clear();
    entities.clear();
}

void EntityManager::addEntity(Entity* entity) {
    addEntity(entity, Deleter{ &EntityManager::defaultDelete, nullptr });
}

void EntityManager::addEntity(Entity* entity, Deleter deleter) {
    entries_.push_back(Entry{ entity, deleter });
    refreshEntityView();
}

void EntityManager::addEntityToFront(Entity* entity) {
    addEntityToFront(entity, Deleter{ &EntityManager::defaultDelete, nullptr });
}

void EntityManager::addEntityToFront(Entity* entity, Deleter deleter) {
    entries_.insert(entries_.begin(), Entry{ entity, deleter });
    refreshEntityView();
}

void EntityManager::updateAll(float deltaTime) {
    for (auto& entry : entries_) {
        Entity* e = entry.ptr;
        if (!e) continue;
        e->update(deltaTime);
        e->updateComponents(deltaTime);
    }
}

void EntityManager::renderAll(SDL_Renderer* renderer) {
    for (auto& entry : entries_) {
        Entity* e = entry.ptr;
        if (!e) continue;
        e->renderComponents(renderer);
    }
}

void EntityManager::removeEntity(Entity* entity) {
    auto it = std::find_if(entries_.begin(), entries_.end(),
        [entity](const Entry& entry) { return entry.ptr == entity; });

    if (it != entries_.end()) {
        if (it->ptr && it->deleter.fn) {
            it->deleter.fn(it->deleter.ctx, it->ptr);
        }
        entries_.erase(it);
        refreshEntityView();
    }
}

Entity* EntityManager::findEntityByName(const std::string& name) {
    for (auto& entry : entries_) {
        Entity* e = entry.ptr;
        if (e && e->name == name) {
            return e;
        }
    }
    return nullptr;
}
