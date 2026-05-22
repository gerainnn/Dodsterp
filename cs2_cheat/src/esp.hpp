#pragma once
#include "sdk.hpp"
#include "overlay.hpp"

class Esp {
public:
    Esp(World& world, Overlay& overlay) : world_(world), overlay_(overlay) {}

    bool enabled = true;

    // Один кадр: читает память, draws всё что надо.
    void render_frame(int local_team);

private:
    World&   world_;
    Overlay& overlay_;

    void draw_player_(ID2D1HwndRenderTarget* rt,
                      const Matrix4& vm,
                      const PawnSnapshot& pawn);
};
