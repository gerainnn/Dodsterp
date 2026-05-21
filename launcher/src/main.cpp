// Launcher Dodsterp.
//   1. Поднимает CS2 через Steam (steam://rungameid/730).
//   2. Ждёт cs2.exe в process list.
//   3. Ждёт пока подгрузится client.dll.
//   4. Стаб времени на стабилизацию рендера.
//   5. Manual map'ит dodsterp.dll прямо в cs2.exe.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <shellapi.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>
#include <thread>

#include "inject/manual_map.hpp"

namespace fs = std::filesystem;

namespace {

constexpr wchar_t  CS2_EXE[]    = L"cs2.exe";
constexpr wchar_t  CS2_MODULE[] = L"client.dll";
constexpr wchar_t  STEAM_URL[]  = L"steam://rungameid/730";
constexpr wchar_t  DLL_NAME[]   = L"dodsterp.dll";

constexpr int      WAIT_PROC_SEC = 240;
constexpr int      WAIT_MOD_SEC  = 60;
constexpr int      POLL_MS       = 500;

enum Color : int { CGray = 8, CRed = 12, CGreen = 10, CYellow = 14, CWhite = 15 };

void cprint(int color, const char* tag, const char* fmt, ...) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, static_cast<WORD>(color));
    std::printf("%s ", tag);
    SetConsoleTextAttribute(h, CWhite);

    va_list ap; va_start(ap, fmt);
    std::vprintf(fmt, ap);
    va_end(ap);
    std::printf("\n");
}

DWORD find_process(const wchar_t* exe) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W e{}; e.dwSize = sizeof(e);
    DWORD pid = 0;
    if (Process32FirstW(snap, &e)) {
        do {
            if (_wcsicmp(e.szExeFile, exe) == 0) { pid = e.th32ProcessID; break; }
        } while (Process32NextW(snap, &e));
    }
    CloseHandle(snap);
    return pid;
}

bool module_loaded(DWORD pid, const wchar_t* mod_name) {
    HANDLE proc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                              FALSE, pid);
    if (!proc) return false;

    HMODULE mods[1024]; DWORD needed = 0;
    bool found = false;

    if (EnumProcessModulesEx(proc, mods, sizeof(mods), &needed,
                             LIST_MODULES_ALL)) {
        const std::size_t count = needed / sizeof(HMODULE);
        wchar_t name[MAX_PATH];
        for (std::size_t i = 0; i < count; ++i) {
            if (GetModuleBaseNameW(proc, mods[i], name,
                                   sizeof(name) / sizeof(wchar_t))) {
                if (_wcsicmp(name, mod_name) == 0) { found = true; break; }
            }
        }
    }
    CloseHandle(proc);
    return found;
}

bool launch_via_steam() {
    HINSTANCE r = ShellExecuteW(nullptr, L"open", STEAM_URL, nullptr, nullptr,
                                SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(r) > 32;
}

DWORD wait_for_cs2() {
    using namespace std::chrono;
    auto deadline = steady_clock::now() + seconds(WAIT_PROC_SEC);
    while (steady_clock::now() < deadline) {
        if (DWORD pid = find_process(CS2_EXE); pid) return pid;
        std::this_thread::sleep_for(milliseconds(POLL_MS));
    }
    return 0;
}

bool wait_for_module(DWORD pid) {
    using namespace std::chrono;
    auto deadline = steady_clock::now() + seconds(WAIT_MOD_SEC);
    while (steady_clock::now() < deadline) {
        if (module_loaded(pid, CS2_MODULE)) return true;
        std::this_thread::sleep_for(milliseconds(POLL_MS));
    }
    return false;
}

fs::path self_dir() {
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return fs::path(buf).parent_path();
}

} // namespace

int wmain() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleW(L"DODSTERP // launcher");

    cprint(CGray, "[*]", "DODSTERP launcher");
    cprint(CGray, "[*]", "self dir : %ls", self_dir().c_str());

    auto dll_path = self_dir() / DLL_NAME;
    if (!fs::exists(dll_path)) {
        cprint(CRed, "[-]", "%ls not found next to launcher", DLL_NAME);
        std::system("pause");
        return 10;
    }
    cprint(CGray, "[*]", "dll path : %ls", dll_path.c_str());

    DWORD pid = find_process(CS2_EXE);
    if (pid) {
        cprint(CYellow, "[!]", "cs2.exe already running, pid=%lu", pid);
    } else {
        cprint(CGray, "[*]", "launching CS2 via Steam...");
        if (!launch_via_steam()) {
            cprint(CRed, "[-]", "ShellExecute(steam://) failed. Steam installed?");
            std::system("pause");
            return 1;
        }
        cprint(CGray, "[*]", "waiting for cs2.exe (timeout %ds)...",
               WAIT_PROC_SEC);
        pid = wait_for_cs2();
        if (!pid) {
            cprint(CRed, "[-]", "cs2.exe did not start in time");
            std::system("pause");
            return 2;
        }
    }
    cprint(CGreen, "[+]", "cs2.exe attached, pid=%lu", pid);

    cprint(CGray, "[*]", "waiting for client.dll (timeout %ds)...",
           WAIT_MOD_SEC);
    if (!wait_for_module(pid)) {
        cprint(CRed, "[-]", "client.dll never appeared");
        std::system("pause");
        return 3;
    }
    cprint(CGreen, "[+]", "client.dll loaded");

    cprint(CGray, "[*]", "stabilization grace (5s)...");
    std::this_thread::sleep_for(std::chrono::seconds(5));

    cprint(CGray, "[*]", "manual mapping %ls into cs2.exe...", DLL_NAME);
    if (!inject::manual_map(pid, dll_path.wstring())) {
        cprint(CRed, "[-]", "inject failed: %s", inject::last_error());
        std::system("pause");
        return 4;
    }
    cprint(CGreen, "[+]", "injected. press INSERT in CS2 to toggle menu.");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return 0;
}
