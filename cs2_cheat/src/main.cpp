// cs2_external — entry point
//
// Цикл:
//   attach к cs2.exe
//   создать transparent overlay привязанный к окну CS2
//   60 FPS: read entities + view matrix → render ESP
//
// Хоткеи:
//   INSERT — toggle ESP
//   END    — выход

#include "memory.hpp"
#include "sdk.hpp"
#include "overlay.hpp"
#include "esp.hpp"
#include "offsets.hpp"

#include <windows.h>
#include <chrono>
#include <thread>
#include <cstdio>

static bool key_just_pressed(int vk) {
    static bool was_down[256]{};
    bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool just = down && !was_down[vk];
    was_down[vk] = down;
    return just;
}

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    // консолька для отладки (не мешает overlay)
    AllocConsole();
    FILE* f = nullptr;
    freopen_s(&f, "CONOUT$", "w", stdout);
    SetConsoleTitleW(L"cs2_external — log");

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    Memory mem;
    wprintf(L"[*] waiting for cs2.exe...\n");
    while (!mem.attach(L"cs2.exe")) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    wprintf(L"[+] attached, pid=%lu\n", mem.pid());

    World world(mem);
    while (!world.init()) {
        wprintf(L"[*] waiting for client.dll...\n");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    wprintf(L"[+] client.dll ready\n");

    HWND target = mem.game_window();
    if (!target) {
        wprintf(L"[!] CS2 window not found\n");
        return 1;
    }

    Overlay overlay;
    if (!overlay.create(target)) {
        wprintf(L"[!] overlay init failed\n");
        return 1;
    }
    wprintf(L"[+] overlay up. INSERT toggle, END exit\n");

    Esp esp(world, overlay);

    using clock = std::chrono::steady_clock;
    auto next_frame = clock::now();
    const auto frame_dt = std::chrono::milliseconds(16);   // ~60 FPS

    while (true) {
        if (!overlay.pump()) break;

        if (key_just_pressed(VK_INSERT)) esp.enabled = !esp.enabled;
        if (key_just_pressed(VK_END))    break;

        if (GetForegroundWindow() == target || GetForegroundWindow() == overlay.hwnd()) {
            overlay.sync_to_target();

            uintptr_t local_pawn = world.local_pawn();
            int local_team = 0;
            if (local_pawn)
                local_team = mem.read<int>(local_pawn + off::base_entity::m_iTeamNum);

            esp.render_frame(local_team);
        } else {
            // CS2 не в фокусе — рисуем пустой кадр чтоб overlay сам не залип
            overlay.frame(nullptr);
        }

        next_frame += frame_dt;
        std::this_thread::sleep_until(next_frame);
    }

    overlay.destroy();
    wprintf(L"[+] bye\n");
    return 0;
}
