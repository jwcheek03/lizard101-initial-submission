#include "Entity.h"

// AABB overlap test using floats, avoids SDL rect API differences
bool collisionDetection(Entity& a, Entity& b) {
    SDL_FRect A = getBounds(a);
    SDL_FRect B = getBounds(b);
    const bool sep =
        (A.x + A.w) <= B.x ||
        (B.x + B.w) <= A.x ||
        (A.y + A.h) <= B.y ||
        (B.y + B.h) <= A.y;
    return !sep;
}
