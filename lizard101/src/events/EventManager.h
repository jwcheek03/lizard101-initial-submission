#pragma once
#include "events/Event.h"
#include "Timeline.h"
#include <queue>
#include <unordered_map>
#include <vector>
#include <functional>
#include <cstdint>

class EventManager {
public:
    using Listener = std::function<void(const Event&)>;
    using ListenerId = std::uint64_t;

    explicit EventManager(Timeline& timeline);

    // Registration
    ListenerId subscribe(EventType type, Listener cb);
    void unsubscribe(EventType type, ListenerId id);

    // Raising
    void raise(Event e);                 // queues event (timestamp auto-filled if <= 0)

    // Handling
    void dispatch();                     // drains queue in priority order

private:
    struct QItem {
        Event event;
        std::uint64_t seq; // monotonically increasing tie-breaker
    };

    struct Compare {
        bool operator()(const QItem& a, const QItem& b) const {
            // lower priority enum value = higher priority
            if (a.event.priority != b.event.priority) {
                return static_cast<int>(a.event.priority) > static_cast<int>(b.event.priority);
            }
            // older timestamp first
            if (a.event.timestamp != b.event.timestamp) {
                return a.event.timestamp > b.event.timestamp;
            }
            // finally FIFO among identical priority/timestamp
            return a.seq > b.seq;
        }
    };

    Timeline& timeline_;
    std::priority_queue<QItem, std::vector<QItem>, Compare> queue_;
    std::unordered_map<EventType, std::unordered_map<ListenerId, Listener>> listeners_;
    ListenerId nextId_ = 1;
    std::uint64_t nextSeq_ = 1;
};
