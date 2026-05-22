#include "math.hpp"

std::optional<Vec2> world_to_screen(const Matrix4& vm,
                                    const Vec3& world,
                                    int screen_w,
                                    int screen_h)
{
    const float* m = vm.m;

    float clip_x = m[0]  * world.x + m[1]  * world.y + m[2]  * world.z + m[3];
    float clip_y = m[4]  * world.x + m[5]  * world.y + m[6]  * world.z + m[7];
    float clip_w = m[12] * world.x + m[13] * world.y + m[14] * world.z + m[15];

    if (clip_w < 0.1f) return std::nullopt;

    float ndc_x = clip_x / clip_w;
    float ndc_y = clip_y / clip_w;

    Vec2 out;
    out.x = (screen_w * 0.5f) + (ndc_x * screen_w * 0.5f);
    out.y = (screen_h * 0.5f) - (ndc_y * screen_h * 0.5f);

    if (out.x < 0 || out.x > screen_w || out.y < 0 || out.y > screen_h)
        return std::nullopt;

    return out;
}
