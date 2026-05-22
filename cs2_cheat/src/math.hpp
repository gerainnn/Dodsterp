#pragma once
#include <cmath>
#include <optional>

struct Vec3 {
    float x{}, y{}, z{};
    Vec3() = default;
    Vec3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}
};

// 4x4 row-major view-projection matrix as CS2 stores it
struct Matrix4 {
    float m[16];
};

struct Vec2 {
    float x{}, y{};
};

// World position to screen pixels for a CS2-style row-major view-proj matrix.
// Returns nullopt if point is behind camera or off-screen.
std::optional<Vec2> world_to_screen(const Matrix4& vm,
                                    const Vec3& world,
                                    int screen_w,
                                    int screen_h);
