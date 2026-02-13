// ecs/ComponentType.h
#pragma once
#include <cstddef>

using ComponentTypeId = std::size_t;

inline ComponentTypeId nextComponentTypeId() {
    static ComponentTypeId id = 0;
    return id++;
}

template <typename T>
ComponentTypeId componentTypeId() {
    static ComponentTypeId id = nextComponentTypeId();
    return id;
}
