#ifndef RAYLIB_COMPAT_TYPES_H
#define RAYLIB_COMPAT_TYPES_H

// Minimal stand-ins so Types.h / GameState.h compile without real
// ImGui or SFML headers when building the Raylib target.
// Field names/order match usage in Types.h and GameState.cpp.

struct ImVec4 {
    float x, y, z, w;
    ImVec4() : x(0), y(0), z(0), w(0) {}
    ImVec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
};

namespace sf {
    struct Vector2f {
        float x, y;
        Vector2f() : x(0), y(0) {}
        Vector2f(float _x, float _y) : x(_x), y(_y) {}
    };
}

#endif // RAYLIB_COMPAT_TYPES_H
