// Entry для dodsterp.dll. Manual map'ом или LoadLibrary'ем — нам всё равно,
// логика идентична. Разница только в том, что при manual map PEB-список
// модулей нас не содержит и hModule == nullptr (то что прислал loader).

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "overlay/hook.hpp"

namespace {

HMODULE g_self = nullptr;

DWORD WINAPI bootstrap(LPVOID) {
    // Дать игре устаканиться после маппинга. CS2 в момент DllMain может
    // быть в состоянии "swapchain ещё пересоздаётся" (resize окна и т.п.).
    Sleep(1500);

    if (!overlay::install_hook()) {
        // тихо умрём, всё равно лога нет (manual map = no console)
        return 1;
    }
    return 0;
}

} // namespace

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = inst;
        DisableThreadLibraryCalls(inst);

        HANDLE t = CreateThread(nullptr, 0, bootstrap, nullptr, 0, nullptr);
        if (t) CloseHandle(t);
    }
    return TRUE;
}
