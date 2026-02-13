#pragma once
#include <variant>
#include <string>

struct Entity; // forward declaration

// ---- Event taxonomy ----
enum class EventType { Input, Collision, Death, Spawn };
enum class EventPriority : int { High = 0, Normal = 1, Low = 2 };

// ---- Payloads ----
struct InputAction {
    enum class Kind { None, MoveLeft, MoveRight, Jump, DashLeft, DashRight } action;
    bool pressed; // true on press, false on release
    std::string playerName;
};

struct CollisionInfo { Entity* a; Entity* b; };
struct DeathInfo { Entity* victim; };
struct SpawnInfo { std::string archetype; float x, y; };

// Mouse-based drag input (for click-dragging the player, etc.)
struct DragInfo {
    enum class Phase { Start, Move, End } phase;
    float x;
    float y;
};

// ---- Event envelope ----
using EventPayload = std::variant<
    InputAction,
    CollisionInfo,
    DeathInfo,
    SpawnInfo,
    DragInfo
>;

struct Event {
    EventType type;
    EventPriority priority;
    float timestamp;     // populated from Timeline in EventManager::raise()
    EventPayload payload;
};
