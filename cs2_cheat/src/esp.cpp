#include "esp.hpp"
#include <d2d1.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

static ComPtr<ID2D1SolidColorBrush> make_brush(ID2D1HwndRenderTarget* rt,
                                               D2D1::ColorF color) {
    ComPtr<ID2D1SolidColorBrush> b;
    rt->CreateSolidColorBrush(color, &b);
    return b;
}

void Esp::render_frame(int local_team) {
    if (!enabled) return;

    Matrix4 vm = world_.view_matrix();
    auto enemies = world_.enemies(local_team);

    overlay_.frame([&](ID2D1HwndRenderTarget* rt) {
        for (const auto& e : enemies) {
            draw_player_(rt, vm, e);
        }
    });
}

void Esp::draw_player_(ID2D1HwndRenderTarget* rt,
                       const Matrix4& vm,
                       const PawnSnapshot& pawn)
{
    int W = overlay_.width();
    int H = overlay_.height();

    Vec3 feet = pawn.origin;
    Vec3 head = { feet.x, feet.y, feet.z + 72.0f };  // высота персонажа CS2

    auto sf = world_to_screen(vm, feet, W, H);
    auto sh = world_to_screen(vm, head, W, H);
    if (!sf || !sh) return;

    float box_h = sf->y - sh->y;
    float box_w = box_h * 0.4f;
    float x1 = sh->x - box_w * 0.5f;
    float x2 = sh->x + box_w * 0.5f;
    float y1 = sh->y;
    float y2 = sf->y;

    // основной бокс
    auto red = make_brush(rt, D2D1::ColorF(1.f, 0.15f, 0.15f, 1.f));
    auto bg  = make_brush(rt, D2D1::ColorF(0.f, 0.f, 0.f, 0.6f));
    auto hp_brush = make_brush(rt, D2D1::ColorF(0.2f, 0.95f, 0.2f, 1.f));

    D2D1_RECT_F box = D2D1::RectF(x1, y1, x2, y2);
    rt->DrawRectangle(box, red.Get(), 1.5f);

    // healthbar слева от бокса
    float hp_x1 = x1 - 6.f;
    float hp_x2 = x1 - 2.f;
    float hp_y1 = y1;
    float hp_y2 = y2;

    rt->FillRectangle(D2D1::RectF(hp_x1, hp_y1, hp_x2, hp_y2), bg.Get());
    float hp_ratio = static_cast<float>(pawn.health) / 100.f;
    if (hp_ratio < 0.f) hp_ratio = 0.f;
    if (hp_ratio > 1.f) hp_ratio = 1.f;

    float hp_top = hp_y2 - (hp_y2 - hp_y1) * hp_ratio;
    D2D1::ColorF c = (pawn.health > 60) ? D2D1::ColorF(0.2f, 0.95f, 0.2f, 1.f)
                    : (pawn.health > 25) ? D2D1::ColorF(0.95f, 0.85f, 0.2f, 1.f)
                    : D2D1::ColorF(0.95f, 0.2f, 0.2f, 1.f);
    auto hp_color = make_brush(rt, c);
    rt->FillRectangle(D2D1::RectF(hp_x1, hp_top, hp_x2, hp_y2), hp_color.Get());
}
