// Лаунчер: поднимает CS2 через Steam (steam://rungameid/730),
// дожидается появления процесса cs2.exe и подгрузки client.dll,
// затем стартует cs2-trainer.exe рядом с собой.
//
// Сейчас работает как console app — видно весь процесс, удобно дебажить.
// Когда стабилизируется — пересобрать с /SUBSYSTEM:WINDOWS чтобы окна не
// мелькало, ошибки гнать через MessageBox.

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

namespace fs = std::filesystem;

namespace {

constexpr wchar_t  CS2_EXE[]       = L"cs2.exe";
constexpr wchar_t  CS2_MODULE[]    = L"client.dll";
constexpr wchar_t  STEAM_URL[]     = L"steam://rungameid/730";
constexpr wchar_t  TRAINER_EXE[]   = L"cs2-trainer.exe";

constexpr int      WAIT_PROC_SEC   = 240;  // макс ждать запуск CS2
constexpr int      WAIT_MOD_SEC    = 60;   // макс ждать client.dll
constexpr int      POLL_MS         = 500;

enum Color : int { CGray = 8, CRed = 12, CGreen = 10, CYellow = 14, CWhite = 15 };

void cprint(int color, const char* tag, const char* fmt, ...) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, static_cast<WORD>(color));
    std::printf("%s ", tag);
    SetConsoleTextAttribute(h, CWhite);

    va_list ap;
    va_start(ap, fmt);
    std::vprintf(fmt, ap);
    va_end(ap);
    std::printf("\n");
}

DWORD find_process(const wchar_t* exe) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W e{};
    e.dwSize = sizeof(e);
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

    HMODULE mods[1024];
    DWORD needed = 0;
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
    HINSTANCE r = ShellExecuteW(nullptr, L"open", STEAM_URL,
                                nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(r) > 32;
}

DWORD wait_for_cs2() {
    using namespace std::chrono;
    auto start = steady_clock::now();
    auto deadline = start + seconds(WAIT_PROC_SEC);

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

bool spawn_trainer(const fs::path& dir) {
    fs::path exe = dir / TRAINER_EXE;
    if (!fs::exists(exe)) {
        cprint(CRed, "[-]", "trainer not found: %ls", exe.c_str());
        return false;
    }

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    std::wstring cmd = L"\"" + exe.wstring() + L"\"";
    if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE,
                        0, nullptr, dir.wstring().c_str(), &si, &pi)) {
        cprint(CRed, "[-]", "CreateProcess failed: %lu", GetLastError());
        return false;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
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

    cprint(CGray,   "[*]", "DODSTERP CS2 trainer launcher");
    cprint(CGray,   "[*]", "self dir: %ls", self_dir().c_str());

    // Если CS2 уже запущен — не дёргаем Steam ещё раз.
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
        cprint(CGray, "[*]", "waiting for cs2.exe (timeout %ds)...", WAIT_PROC_SEC);
        pid = wait_for_cs2();
        if (!pid) {
            cprint(CRed, "[-]", "cs2.exe did not start in time");
            std::system("pause");
            return 2;
        }
    }
    cprint(CGreen, "[+]", "cs2.exe attached, pid=%lu", pid);

    cprint(CGray, "[*]", "waiting for client.dll to load (timeout %ds)...",
           WAIT_MOD_SEC);
    if (!wait_for_module(pid)) {
        cprint(CRed, "[-]", "client.dll never appeared, aborting");
        std::system("pause");
        return 3;
    }
    cprint(CGreen, "[+]", "client.dll loaded");

    // Дополнительный простой sleep — главное меню обычно полностью готово
    // через 4-6 секунд после подгрузки client.dll.
    cprint(CGray, "[*]", "stabilization grace (5s)...");
    std::this_thread::sleep_for(std::chrono::seconds(5));

    cprint(CGray, "[*]", "spawning trainer...");
    if (!spawn_trainer(self_dir())) {
        std::system("pause");
        return 4;
    }
    cprint(CGreen, "[+]", "trainer launched. exiting launcher.");
    return 0;
}
