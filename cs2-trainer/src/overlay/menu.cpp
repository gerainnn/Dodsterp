// Визуальное меню Dodsterp. Все контролы — стабы (без логики), задача
// этого файла: layout + стиль. Когда визуал согласуем — навешиваем
// настоящие фичи на эти же контролы.

#include "menu.hpp"
#include "theme.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <ctime>
#include <cmath>
#include <cstdio>

namespace overlay::menu {

namespace {

// ---- состояние UI (стабы) -------------------------------------------------

enum class Tab { Visuals, Aim, World, Misc, Config, _Count };

struct Ui {
    Tab  active = Tab::Visuals;

    // visuals
    bool   esp_enable        = true;
    int    esp_box_style     = 0;          // 0=2D, 1=Corner, 2=Filled
    bool   esp_health        = true;
    bool   esp_name          = false;
    bool   esp_distance      = false;
    bool   esp_weapon        = false;
    bool   esp_skeleton      = false;
    bool   esp_team_check    = true;
    float  esp_max_distance  = 100.0f;
    ImVec4 esp_enemy_visible = ImVec4(1.00f, 0.18f, 0.25f, 1.00f);
    ImVec4 esp_enemy_hidden  = ImVec4(1.00f, 0.55f, 0.27f, 1.00f);
    ImVec4 esp_team          = ImVec4(0.30f, 0.78f, 0.96f, 1.00f);

    // aim
    bool   aim_enable        = false;
    int    aim_mode          = 0;           // 0=assist, 1=silent, 2=triggerbot
    int    aim_bone          = 0;           // 0=head, 1=neck, 2=chest, 3=closest
    float  aim_fov           = 5.0f;
    float  aim_smooth        = 3.0f;
    bool   aim_visible_only  = true;
    bool   aim_rcs           = false;
    float  aim_rcs_x         = 1.0f;
    float  aim_rcs_y         = 1.0f;

    // world
    bool   bhop              = false;
    bool   no_flash          = false;
    float  no_flash_alpha    = 0.0f;
    bool   thirdperson       = false;
    bool   crosshair         = false;

    // misc
    bool   watermark         = true;
    bool   spectator_list    = true;
    bool   bomb_timer        = false;
    int    fps_cap           = 0;

    // config
    char   config_name[64]   = "default";
} ui;

// ---- хелперы --------------------------------------------------------------

void push_acc()  { ImGui::PushStyleColor(ImGuiCol_Text, theme::pal.acc); }
void push_dim()  { ImGui::PushStyleColor(ImGuiCol_Text, theme::pal.fg_dim); }
void pop_color() { ImGui::PopStyleColor(); }

// заголовок секции с тонкой акцентной линией снизу
void section(const char* label) {
    ImGui::Spacing();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    push_dim();
    ImGui::Text("%s", label);
    pop_color();
    ImVec2 e = ImGui::GetItemRectMax();
    dl->AddLine(ImVec2(p.x, e.y + 3), ImVec2(p.x + 28, e.y + 3),
                ImGui::GetColorU32(theme::pal.acc), 1.5f);
    dl->AddLine(ImVec2(p.x + 30, e.y + 3),
                ImVec2(p.x + ImGui::GetContentRegionAvail().x, e.y + 3),
                ImGui::GetColorU32(theme::pal.border), 1.0f);
    ImGui::Dummy(ImVec2(0, 4));
}

// карточка-контейнер
void begin_card(const char* id, float height = 0.0f) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::pal.bg2);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_Border, theme::pal.border);
    ImGui::BeginChild(id, ImVec2(0, height), ImGuiChildFlags_Border |
                                              ImGuiChildFlags_AutoResizeY);
    ImGui::Dummy(ImVec2(0, 4));
    ImGui::Indent(14);
}

void end_card() {
    ImGui::Unindent(14);
    ImGui::Dummy(ImVec2(0, 6));
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
    ImGui::Dummy(ImVec2(0, 6));
}

// кастомный toggle — нарисованная "пилюля" как у современных платных меню
bool toggle(const char* label, bool* v) {
    ImGuiWindow* w = ImGui::GetCurrentWindow();
    if (w->SkipItems) return false;

    const ImGuiID id = w->GetID(label);
    const float   h  = ImGui::GetFrameHeight() * 0.85f;
    const float   ww = h * 1.9f;

    ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
    ImVec2 pos = w->DC.CursorPos;
    ImVec2 total(ImGui::GetContentRegionAvail().x, ImMax(label_size.y, h));
    ImRect bb(pos, ImVec2(pos.x + total.x, pos.y + total.y));
    ImGui::ItemSize(bb, 0);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    if (pressed) *v = !*v;

    // label слева
    ImGui::RenderText(ImVec2(pos.x, pos.y + (total.y - label_size.y) * 0.5f),
                      label);

    // pill справа
    ImVec2 pill_p(pos.x + total.x - ww, pos.y + (total.y - h) * 0.5f);
    ImVec2 pill_e(pill_p.x + ww, pill_p.y + h);

    ImU32 track_off = ImGui::GetColorU32(theme::pal.bg3);
    ImU32 track_on  = ImGui::GetColorU32(theme::pal.acc);
    ImU32 track_col = *v ? track_on : track_off;
    if (hovered) track_col = ImAlphaBlendColors(track_col, IM_COL32(255,255,255,16));

    w->DrawList->AddRectFilled(pill_p, pill_e, track_col, h * 0.5f);
    if (!*v) {
        w->DrawList->AddRect(pill_p, pill_e,
                             ImGui::GetColorU32(theme::pal.border),
                             h * 0.5f, 0, 1.0f);
    }

    float pad = 2.0f;
    float kr  = (h - pad * 2) * 0.5f;
    float kx  = *v ? (pill_e.x - pad - kr) : (pill_p.x + pad + kr);
    float ky  = pill_p.y + h * 0.5f;
    w->DrawList->AddCircleFilled(ImVec2(kx, ky), kr,
                                 IM_COL32(245, 245, 245, 255));
    return pressed;
}

// ---- title bar ------------------------------------------------------------

void draw_titlebar(ImVec2 win_p, ImVec2 win_e) {
    ImDrawList* dl = ImGui::GetWindowDrawList();

    const float h = 44.0f;
    ImVec2 e(win_e.x, win_p.y + h);

    // gradient
    dl->AddRectFilledMultiColor(
        win_p, e,
        IM_COL32(20, 20, 20, 255), IM_COL32(14, 14, 14, 255),
        IM_COL32(14, 14, 14, 255), IM_COL32(20, 20, 20, 255));

    // нижняя тонкая линия
    dl->AddLine(ImVec2(win_p.x, e.y - 1), ImVec2(e.x, e.y - 1),
                ImGui::GetColorU32(theme::pal.border), 1.0f);

    // pulsing red dot
    float t = (float)ImGui::GetTime();
    float pulse = 0.6f + 0.4f * std::sinf(t * 3.0f);
    ImVec2 dot(win_p.x + 18, win_p.y + h * 0.5f);
    dl->AddCircleFilled(dot, 4.5f,
        IM_COL32(255, 46, 63, (int)(255 * pulse)));
    dl->AddCircle(dot, 7.0f,
        IM_COL32(255, 46, 63, (int)(80 * pulse)), 24, 1.5f);

    // title
    ImVec2 tp(win_p.x + 36, win_p.y + (h - ImGui::GetFontSize()) * 0.5f);
    dl->AddText(tp, IM_COL32(245, 245, 245, 255), "DODSTERP");
    ImVec2 sz = ImGui::CalcTextSize("DODSTERP");
    dl->AddText(ImVec2(tp.x + sz.x + 6, tp.y),
                IM_COL32(120, 120, 120, 255), "// CS2 TRAINER");

    // build tag right
    char build[64];
    std::time_t now = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &now);
#else
    tm = *std::localtime(&now);
#endif
    std::snprintf(build, sizeof(build), "v0.1  //  %02d:%02d:%02d",
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
    ImVec2 bsz = ImGui::CalcTextSize(build);
    dl->AddText(ImVec2(e.x - bsz.x - 18, tp.y),
                IM_COL32(110, 110, 110, 255), build);
}

// ---- sidebar --------------------------------------------------------------

const char* tab_label(Tab t) {
    switch (t) {
        case Tab::Visuals: return "VISUALS";
        case Tab::Aim:     return "AIM";
        case Tab::World:   return "WORLD";
        case Tab::Misc:    return "MISC";
        case Tab::Config:  return "CONFIG";
        default: return "?";
    }
}

const char* tab_glyph(Tab t) {
    // используем простые ASCII-маркеры, чтобы не таскать иконочный шрифт
    switch (t) {
        case Tab::Visuals: return "[V]";
        case Tab::Aim:     return "[A]";
        case Tab::World:   return "[W]";
        case Tab::Misc:    return "[M]";
        case Tab::Config:  return "[C]";
        default: return "[?]";
    }
}

void draw_sidebar(ImVec2 p, float w, float h) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h),
                      ImGui::GetColorU32(theme::pal.bg1));
    dl->AddLine(ImVec2(p.x + w, p.y), ImVec2(p.x + w, p.y + h),
                ImGui::GetColorU32(theme::pal.border), 1.0f);

    const float row_h = 44.0f;
    const float pad   = 14.0f;

    for (int i = 0; i < (int)Tab::_Count; ++i) {
        Tab t = (Tab)i;
        ImVec2 r0(p.x, p.y + pad + i * row_h);
        ImVec2 r1(p.x + w, r0.y + row_h);

        ImGui::SetCursorScreenPos(r0);
        ImGui::PushID(i);
        bool clicked = ImGui::InvisibleButton("##tab", ImVec2(w, row_h));
        bool hovered = ImGui::IsItemHovered();
        ImGui::PopID();
        if (clicked) ui.active = t;

        bool active = ui.active == t;
        ImU32 bg = active
            ? ImGui::GetColorU32(theme::pal.bg2)
            : (hovered
                ? IM_COL32(20, 20, 20, 255)
                : ImGui::GetColorU32(theme::pal.bg1));
        dl->AddRectFilled(r0, r1, bg);

        if (active) {
            dl->AddRectFilled(r0, ImVec2(r0.x + 3, r1.y),
                              ImGui::GetColorU32(theme::pal.acc));
        }

        // glyph
        ImU32 gcol = active
            ? IM_COL32(255, 46, 63, 255)
            : IM_COL32(120, 120, 120, 255);
        dl->AddText(ImVec2(r0.x + 18, r0.y + (row_h - ImGui::GetFontSize()) * 0.5f),
                    gcol, tab_glyph(t));

        // label
        ImU32 tcol = active ? IM_COL32(245, 245, 245, 255)
                            : IM_COL32(170, 170, 170, 255);
        dl->AddText(ImVec2(r0.x + 56, r0.y + (row_h - ImGui::GetFontSize()) * 0.5f),
                    tcol, tab_label(t));
    }

    // подпись внизу sidebar'а
    push_dim();
    ImGui::SetCursorScreenPos(ImVec2(p.x + 14, p.y + h - 36));
    ImGui::TextUnformatted("INSERT — toggle");
    ImGui::SetCursorScreenPos(ImVec2(p.x + 14, p.y + h - 20));
    ImGui::TextUnformatted("END    — unload");
    pop_color();
}

// ---- табы -----------------------------------------------------------------

void tab_visuals() {
    section("PLAYER ESP");
    begin_card("##esp");
    toggle("Enable",            &ui.esp_enable);
    {
        const char* items[] = { "2D Box", "Corner Box", "Filled" };
        ImGui::SetNextItemWidth(160);
        ImGui::Combo("Box style", &ui.esp_box_style, items, IM_ARRAYSIZE(items));
    }
    toggle("Show health",       &ui.esp_health);
    toggle("Show name",         &ui.esp_name);
    toggle("Show distance",     &ui.esp_distance);
    toggle("Show weapon",       &ui.esp_weapon);
    toggle("Skeleton",          &ui.esp_skeleton);
    toggle("Team check",        &ui.esp_team_check);
    ImGui::SetNextItemWidth(220);
    ImGui::SliderFloat("Max distance (m)", &ui.esp_max_distance, 1.0f, 200.0f, "%.0f");
    end_card();

    section("COLORS");
    begin_card("##col");
    ImGui::ColorEdit4("Enemy (visible)", (float*)&ui.esp_enemy_visible,
                      ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::ColorEdit4("Enemy (hidden)",  (float*)&ui.esp_enemy_hidden,
                      ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::ColorEdit4("Team",            (float*)&ui.esp_team,
                      ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    end_card();
}

void tab_aim() {
    section("AIM ASSIST");
    begin_card("##aim");
    toggle("Enable", &ui.aim_enable);
    {
        const char* modes[] = { "Aim assist", "Silent", "Triggerbot" };
        ImGui::SetNextItemWidth(160);
        ImGui::Combo("Mode", &ui.aim_mode, modes, IM_ARRAYSIZE(modes));
    }
    {
        const char* bones[] = { "Head", "Neck", "Chest", "Closest" };
        ImGui::SetNextItemWidth(160);
        ImGui::Combo("Target bone", &ui.aim_bone, bones, IM_ARRAYSIZE(bones));
    }
    ImGui::SetNextItemWidth(220);
    ImGui::SliderFloat("FOV (deg)",     &ui.aim_fov,    0.0f, 30.0f, "%.1f");
    ImGui::SetNextItemWidth(220);
    ImGui::SliderFloat("Smoothness",    &ui.aim_smooth, 1.0f, 20.0f, "%.1f");
    toggle("Visible only", &ui.aim_visible_only);
    end_card();

    section("RECOIL CONTROL");
    begin_card("##rcs");
    toggle("Enable RCS", &ui.aim_rcs);
    ImGui::SetNextItemWidth(220);
    ImGui::SliderFloat("Horizontal", &ui.aim_rcs_x, 0.0f, 2.0f, "%.2f");
    ImGui::SetNextItemWidth(220);
    ImGui::SliderFloat("Vertical",   &ui.aim_rcs_y, 0.0f, 2.0f, "%.2f");
    end_card();
}

void tab_world() {
    section("MOVEMENT");
    begin_card("##mov");
    toggle("Bunnyhop",     &ui.bhop);
    toggle("Third person", &ui.thirdperson);
    end_card();

    section("VISIBILITY");
    begin_card("##vis");
    toggle("No flash", &ui.no_flash);
    ImGui::SetNextItemWidth(220);
    ImGui::SliderFloat("No flash alpha", &ui.no_flash_alpha, 0.0f, 1.0f, "%.2f");
    toggle("Custom crosshair", &ui.crosshair);
    end_card();
}

void tab_misc() {
    section("HUD");
    begin_card("##hud");
    toggle("Watermark",       &ui.watermark);
    toggle("Spectator list",  &ui.spectator_list);
    toggle("Bomb timer",      &ui.bomb_timer);
    end_card();

    section("PERFORMANCE");
    begin_card("##perf");
    ImGui::SetNextItemWidth(160);
    ImGui::SliderInt("FPS cap (0 = off)", &ui.fps_cap, 0, 480);
    end_card();
}

void tab_config() {
    section("CONFIG");
    begin_card("##cfg");
    ImGui::SetNextItemWidth(220);
    ImGui::InputText("Name", ui.config_name, IM_ARRAYSIZE(ui.config_name));
    if (ImGui::Button("Save",   ImVec2(80, 0))) {}
    ImGui::SameLine();
    if (ImGui::Button("Load",   ImVec2(80, 0))) {}
    ImGui::SameLine();
    if (ImGui::Button("Reset",  ImVec2(80, 0))) {}
    end_card();

    section("DANGER ZONE");
    begin_card("##danger");
    if (ImGui::Button("Unload trainer", ImVec2(160, 0))) {}
    push_dim();
    ImGui::TextUnformatted("Unload снимет хуки и выгрузит DLL из cs2.exe.");
    pop_color();
    end_card();
}

// ---- status bar -----------------------------------------------------------

void draw_statusbar(ImVec2 p0, ImVec2 p1) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p0, p1, ImGui::GetColorU32(theme::pal.bg1));
    dl->AddLine(ImVec2(p0.x, p0.y), ImVec2(p1.x, p0.y),
                ImGui::GetColorU32(theme::pal.border), 1.0f);

    float fy = p0.y + ((p1.y - p0.y) - ImGui::GetFontSize()) * 0.5f;

    // statе атача (стаб — пока всегда attached, потом подцепим к offsets state)
    dl->AddCircleFilled(ImVec2(p0.x + 16, fy + ImGui::GetFontSize() * 0.5f),
                        4.0f, IM_COL32(88, 222, 96, 255));
    dl->AddText(ImVec2(p0.x + 28, fy),
                IM_COL32(180, 180, 180, 255), "cs2.exe attached");

    // FPS
    char fps_s[32];
    std::snprintf(fps_s, sizeof(fps_s), "%.0f fps", ImGui::GetIO().Framerate);
    ImVec2 sz = ImGui::CalcTextSize(fps_s);
    dl->AddText(ImVec2(p1.x - sz.x - 18, fy),
                IM_COL32(180, 180, 180, 255), fps_s);

    // hotkey подсказка — по центру
    const char* hint = "[INSERT] toggle    [END] unload";
    ImVec2 hsz = ImGui::CalcTextSize(hint);
    dl->AddText(ImVec2(p0.x + ((p1.x - p0.x) - hsz.x) * 0.5f, fy),
                IM_COL32(110, 110, 110, 255), hint);
}

} // namespace

// ============================================================== entry point

void draw() {
    const ImVec2 sz(720, 520);
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        ImVec2(vp->WorkPos.x + (vp->WorkSize.x - sz.x) * 0.5f,
               vp->WorkPos.y + (vp->WorkSize.y - sz.y) * 0.5f),
        ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(sz, ImGuiCond_FirstUseEver);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##dodsterp", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse);

    ImVec2 p = ImGui::GetWindowPos();
    ImVec2 wsz = ImGui::GetWindowSize();
    ImVec2 e(p.x + wsz.x, p.y + wsz.y);

    const float titlebar_h = 44.0f;
    const float status_h   = 28.0f;
    const float sidebar_w  = 168.0f;

    // полная "заглушка" окна выше дефолтной отрисовки imgui — рисуем сами
    draw_titlebar(p, e);
    draw_sidebar(ImVec2(p.x, p.y + titlebar_h), sidebar_w,
                 wsz.y - titlebar_h - status_h);
    draw_statusbar(ImVec2(p.x, e.y - status_h), e);

    // drag-zone — вся титульная полоса
    ImGui::SetCursorScreenPos(p);
    ImGui::InvisibleButton("##drag", ImVec2(wsz.x, titlebar_h));
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        ImVec2 d = ImGui::GetIO().MouseDelta;
        ImGui::SetWindowPos(ImVec2(p.x + d.x, p.y + d.y));
    }

    // content area
    ImGui::SetCursorScreenPos(ImVec2(p.x + sidebar_w, p.y + titlebar_h));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, theme::pal.bg2);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 16));
    ImGui::BeginChild("##content",
        ImVec2(wsz.x - sidebar_w, wsz.y - titlebar_h - status_h),
        ImGuiChildFlags_None,
        ImGuiWindowFlags_NoScrollbar);

    switch (ui.active) {
        case Tab::Visuals: tab_visuals(); break;
        case Tab::Aim:     tab_aim();     break;
        case Tab::World:   tab_world();   break;
        case Tab::Misc:    tab_misc();    break;
        case Tab::Config:  tab_config();  break;
        default: break;
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace overlay::menu
