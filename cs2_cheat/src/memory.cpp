#include "memory.hpp"
#include <tlhelp32.h>
#include <psapi.h>
#include <vector>

Memory::~Memory() {
    if (handle_ && handle_ != INVALID_HANDLE_VALUE)
        CloseHandle(handle_);
}

bool Memory::find_pid_(std::wstring_view exe_name, DWORD& out_pid) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);
    bool found = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (exe_name == pe.szExeFile) {
                out_pid = pe.th32ProcessID;
                found = true;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

struct EnumWndCtx { DWORD pid; HWND result; };

static BOOL CALLBACK enum_wnd_proc(HWND hwnd, LPARAM lParam) {
    auto* ctx = reinterpret_cast<EnumWndCtx*>(lParam);
    DWORD wnd_pid = 0;
    GetWindowThreadProcessId(hwnd, &wnd_pid);
    if (wnd_pid == ctx->pid && IsWindowVisible(hwnd) && GetWindow(hwnd, GW_OWNER) == nullptr) {
        ctx->result = hwnd;
        return FALSE;
    }
    return TRUE;
}

bool Memory::find_window_(DWORD pid, HWND& out_hwnd) {
    EnumWndCtx ctx{ pid, nullptr };
    EnumWindows(enum_wnd_proc, reinterpret_cast<LPARAM>(&ctx));
    out_hwnd = ctx.result;
    return ctx.result != nullptr;
}

bool Memory::attach(std::wstring_view exe_name) {
    if (!find_pid_(exe_name, pid_)) return false;

    handle_ = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
                          FALSE, pid_);
    if (!handle_) return false;

    find_window_(pid_, hwnd_);   // не критично если не нашлось — overlay сам поищет
    return true;
}

uintptr_t Memory::module_base(std::wstring_view module_name) const {
    if (!handle_) return 0;

    HMODULE modules[1024];
    DWORD needed = 0;
    if (!EnumProcessModulesEx(handle_, modules, sizeof(modules), &needed,
                              LIST_MODULES_64BIT))
        return 0;

    DWORD count = needed / sizeof(HMODULE);
    for (DWORD i = 0; i < count; ++i) {
        wchar_t name[MAX_PATH]{};
        if (GetModuleBaseNameW(handle_, modules[i], name, MAX_PATH)) {
            if (module_name == name)
                return reinterpret_cast<uintptr_t>(modules[i]);
        }
    }
    return 0;
}

bool Memory::read_buf(uintptr_t addr, void* dst, size_t size) const {
    if (!handle_) return false;
    SIZE_T got = 0;
    return ReadProcessMemory(handle_, reinterpret_cast<LPCVOID>(addr),
                             dst, size, &got) && got == size;
}
