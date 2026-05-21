#include "theme.hpp"

namespace overlay::theme {

Palette pal;

void apply() {
    ImGuiStyle& s = ImGui::GetStyle();

    s.WindowRounding    = 6.0f;
    s.ChildRounding     = 5.0f;
    s.FrameRounding     = 4.0f;
    s.PopupRounding     = 4.0f;
    s.GrabRounding      = 4.0f;
    s.ScrollbarRounding = 4.0f;
    s.TabRounding       = 4.0f;

    s.WindowPadding     = ImVec2(0, 0); // сами рисуем layout
    s.FramePadding      = ImVec2(10, 6);
    s.ItemSpacing       = ImVec2(10, 8);
    s.ItemInnerSpacing  = ImVec2(6, 4);
    s.IndentSpacing     = 16.0f;
    s.ScrollbarSize     = 12.0f;
    s.GrabMinSize       = 10.0f;

    s.WindowBorderSize  = 1.0f;
    s.ChildBorderSize   = 1.0f;
    s.FrameBorderSize   = 0.0f;
    s.PopupBorderSize   = 1.0f;

    auto& c = s.Colors;
    c[ImGuiCol_WindowBg]          = pal.bg0;
    c[ImGuiCol_ChildBg]           = pal.bg2;
    c[ImGuiCol_PopupBg]           = pal.bg1;
    c[ImGuiCol_Border]            = pal.border;
    c[ImGuiCol_BorderShadow]      = ImVec4(0, 0, 0, 0);

    c[ImGuiCol_Text]              = pal.fg;
    c[ImGuiCol_TextDisabled]      = pal.fg_dim;

    c[ImGuiCol_FrameBg]           = pal.bg3;
    c[ImGuiCol_FrameBgHovered]    = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    c[ImGuiCol_FrameBgActive]     = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

    c[ImGuiCol_TitleBg]           = pal.bg1;
    c[ImGuiCol_TitleBgActive]     = pal.bg1;
    c[ImGuiCol_TitleBgCollapsed]  = pal.bg1;

    c[ImGuiCol_Button]            = pal.bg3;
    c[ImGuiCol_ButtonHovered]     = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    c[ImGuiCol_ButtonActive]      = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);

    c[ImGuiCol_Header]            = pal.bg3;
    c[ImGuiCol_HeaderHovered]     = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    c[ImGuiCol_HeaderActive]      = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);

    c[ImGuiCol_CheckMark]         = pal.acc;
    c[ImGuiCol_SliderGrab]        = pal.acc;
    c[ImGuiCol_SliderGrabActive]  = ImVec4(1.00f, 0.32f, 0.40f, 1.00f);

    c[ImGuiCol_Separator]         = pal.border;
    c[ImGuiCol_SeparatorHovered]  = pal.acc_dim;
    c[ImGuiCol_SeparatorActive]   = pal.acc;

    c[ImGuiCol_Tab]               = pal.bg2;
    c[ImGuiCol_TabHovered]        = pal.bg3;
    c[ImGuiCol_TabActive]         = pal.bg3;
    c[ImGuiCol_TabUnfocused]      = pal.bg2;
    c[ImGuiCol_TabUnfocusedActive]= pal.bg3;

    c[ImGuiCol_ScrollbarBg]       = pal.bg1;
    c[ImGuiCol_ScrollbarGrab]     = pal.bg3;
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);

    c[ImGuiCol_ResizeGrip]        = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ResizeGripHovered] = pal.acc_dim;
    c[ImGuiCol_ResizeGripActive]  = pal.acc;
}

} // namespace overlay::theme
