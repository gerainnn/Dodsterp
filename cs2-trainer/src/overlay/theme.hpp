#pragma once

#include <imgui.h>

namespace overlay::theme {

// Палитра — единая точка правды для меню и ESP.
// "BG" = фоны, "FG" = текст/линии, "ACC" = красный акцент Dodsterp.
struct Palette {
    ImVec4 bg0      = ImVec4(0.027f, 0.027f, 0.027f, 0.98f); // самый тёмный (window)
    ImVec4 bg1      = ImVec4(0.055f, 0.055f, 0.055f, 1.00f); // sidebar
    ImVec4 bg2      = ImVec4(0.078f, 0.078f, 0.078f, 1.00f); // content
    ImVec4 bg3      = ImVec4(0.118f, 0.118f, 0.118f, 1.00f); // card / elevated
    ImVec4 border   = ImVec4(0.157f, 0.157f, 0.157f, 1.00f);
    ImVec4 fg       = ImVec4(0.910f, 0.910f, 0.910f, 1.00f);
    ImVec4 fg_dim   = ImVec4(0.420f, 0.420f, 0.420f, 1.00f);
    ImVec4 fg_dim2  = ImVec4(0.280f, 0.280f, 0.280f, 1.00f);
    ImVec4 acc      = ImVec4(1.000f, 0.180f, 0.247f, 1.00f); // #ff2e3f
    ImVec4 acc_dim  = ImVec4(0.535f, 0.094f, 0.133f, 1.00f);
    ImVec4 ok       = ImVec4(0.345f, 0.871f, 0.376f, 1.00f);
    ImVec4 warn     = ImVec4(0.980f, 0.694f, 0.180f, 1.00f);
};

extern Palette pal;

void apply(); // выставляет ImGui::GetStyle()

} // namespace overlay::theme
