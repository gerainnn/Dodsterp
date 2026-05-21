#pragma once

#include <array>
#include <cmath>

namespace cs2 {

struct Vec3 {
    float x = 0, y = 0, z = 0;
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    float length() const { return std::sqrt(x * x + y * y + z * z); }
};

// View matrix CS2 хранится как row-major 4x4 (16 float).
using ViewMatrix = std::array<float, 16>;

// Source 2 world -> screen.
// Возвращает true если точка перед камерой.
inline bool world_to_screen(const ViewMatrix& m, const Vec3& world,
                            int screen_w, int screen_h,
                            float& out_x, float& out_y) {
    const float w = m[12] * world.x + m[13] * world.y + m[14] * world.z + m[15];
    if (w < 0.001f) return false;

    const float x = m[0] * world.x + m[1] * world.y + m[2] * world.z + m[3];
    const float y = m[4] * world.x + m[5] * world.y + m[6] * world.z + m[7];

    out_x = (screen_w * 0.5f) + (x / w) * (screen_w * 0.5f);
    out_y = (screen_h * 0.5f) - (y / w) * (screen_h * 0.5f);
    return true;
}

} // namespace cs2
