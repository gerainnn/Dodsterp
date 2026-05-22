#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <windows.h>

// Минимальная обёртка над OpenProcess / ReadProcessMemory для CS2.
// Все указатели CS2 — 64-битные.
class Memory {
public:
    Memory() = default;
    ~Memory();

    Memory(const Memory&) = delete;
    Memory& operator=(const Memory&) = delete;

    // Найти процесс по имени exe (например, L"cs2.exe"), открыть handle.
    bool attach(std::wstring_view exe_name);

    // Базовый адрес загруженного модуля (например, L"client.dll").
    uintptr_t module_base(std::wstring_view module_name) const;

    // Вернуть HWND главного окна процесса (для прицепления overlay).
    HWND game_window() const { return hwnd_; }

    // Универсальное чтение
    template<typename T>
    T read(uintptr_t addr) const {
        T value{};
        SIZE_T got = 0;
        if (!ReadProcessMemory(handle_, reinterpret_cast<LPCVOID>(addr),
                               &value, sizeof(T), &got))
            return T{};
        return value;
    }

    bool read_buf(uintptr_t addr, void* dst, size_t size) const;

    bool ok() const { return handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE; }
    DWORD pid() const { return pid_; }

private:
    HANDLE handle_ = nullptr;
    DWORD  pid_    = 0;
    HWND   hwnd_   = nullptr;

    bool find_pid_(std::wstring_view exe_name, DWORD& out_pid);
    bool find_window_(DWORD pid, HWND& out_hwnd);
};
