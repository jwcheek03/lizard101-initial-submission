// ecs/Tag.h
#pragma once
#include <string>

namespace ecs {
    struct Tag {
        std::string value;
        Tag() = default;
        explicit Tag(const std::string& v) : value(v) {}
        bool is(const std::string& s) const { return value == s; }
        bool empty() const { return value.empty(); }
    };
} // namespace ecs
