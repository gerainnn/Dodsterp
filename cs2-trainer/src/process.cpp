#include "process.hpp"

#include <tlhelp32.h>
#include <psapi.h>
#include <vector>

namespace cs2 {

Process::~Process() { detach(); }

void Process::detach() {
    if (handle_) {
        CloseHandle(handle_);
        handle_ = nullptr;
    }
    pid_ = 0;
}

bool Process::attach(std::wstring_view exe_name) {
    detach();

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    DWORD found = 0;
    if (Process32FirstW(snap, &entry)) {
        do {
            if (exe_name == entry.szExeFile) {
                found = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &entry));
    }
    CloseHandle(snap);

    if (!found) return false;

    handle_ = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
                          FALSE, found);
    if (!handle_) return false;

    pid_ = found;
    return true;
}

static std::uintptr_t find_module(HANDLE proc, std::wstring_view name,
                                  std::size_t* out_size) {
    HMODULE mods[1024];
    DWORD needed = 0;
    if (!EnumProcessModulesEx(proc, mods, sizeof(mods), &needed,
                              LIST_MODULES_ALL)) {
        return 0;
    }

    const std::size_t count = needed / sizeof(HMODULE);
    for (std::size_t i = 0; i < count; ++i) {
        wchar_t mod_name[MAX_PATH] = {};
        if (!GetModuleBaseNameW(proc, mods[i], mod_name,
                                sizeof(mod_name) / sizeof(wchar_t))) continue;

        if (name == mod_name) {
            MODULEINFO info{};
            if (GetModuleInformation(proc, mods[i], &info, sizeof(info))) {
                if (out_size) *out_size = info.SizeOfImage;
                return reinterpret_cast<std::uintptr_t>(info.lpBaseOfDll);
            }
        }
    }
    return 0;
}

std::uintptr_t Process::module_base(std::wstring_view module_name) const {
    if (!handle_) return 0;
    return find_module(handle_, module_name, nullptr);
}

std::size_t Process::module_size(std::wstring_view module_name) const {
    if (!handle_) return 0;
    std::size_t sz = 0;
    find_module(handle_, module_name, &sz);
    return sz;
}

bool Process::read_buffer(std::uintptr_t addr, void* dst, std::size_t size) const {
    if (!handle_) return false;
    SIZE_T bytes = 0;
    return ReadProcessMemory(handle_, reinterpret_cast<LPCVOID>(addr),
                             dst, size, &bytes) && bytes == size;
}

} // namespace cs2
