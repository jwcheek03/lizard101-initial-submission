#include "events/EventManager.h"

EventManager::EventManager(Timeline& timeline) : timeline_(timeline) {}

EventManager::ListenerId EventManager::subscribe(EventType type, Listener cb) {
    const auto id = nextId_++;
    listeners_[type].emplace(id, std::move(cb));
    return id;
}

void EventManager::unsubscribe(EventType type, ListenerId id) {
    auto it = listeners_.find(type);
    if (it != listeners_.end()) {
        it->second.erase(id);
    }
}

void EventManager::raise(Event e) {
    if (e.timestamp <= 0.0f) {
        // NOTE: Timeline::getElapsedTime() is used to time-stamp events
        e.timestamp = timeline_.getElapsedTime();
    }
    queue_.push(QItem{ std::move(e), nextSeq_++ });
}

void EventManager::dispatch() {
    while (!queue_.empty()) {
        auto q = queue_.top(); queue_.pop();
        auto it = listeners_.find(q.event.type);
        if (it == listeners_.end()) continue;

        // deliver to a snapshot of listeners (stable under modifications)
        auto snapshot = it->second;
        for (auto& kv : snapshot) {
            kv.second(q.event);
        }
    }
}
