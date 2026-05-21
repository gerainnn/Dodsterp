#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <optional>
#include <windows.h>

namespace cs2 {

// Минимальная обёртка над OpenProcess + RPM/WPM.
// Все чтения возвращают std::optional<T> — нет валидации указателей в самой игре,
// поэтому каждое чтение может тихо упасть и это нормально.
class Process {
public:
    Process() = default;
    ~Process();

    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;

    bool attach(std::wstring_view exe_name);
    void detach();

    // Базовый адрес модуля внутри cs2.exe (например, "client.dll").
    std::uintptr_t module_base(std::wstring_view module_name) const;
    std::size_t    module_size(std::wstring_view module_name) const;

    template <typename T>
    std::optional<T> read(std::uintptr_t addr) const {
        T value{};
        SIZE_T bytes = 0;
        if (!ReadProcessMemory(handle_, reinterpret_cast<LPCVOID>(addr),
                               &value, sizeof(T), &bytes) || bytes != sizeof(T)) {
            return std::nullopt;
        }
        return value;
    }

    bool read_buffer(std::uintptr_t addr, void* dst, std::size_t size) const;

    DWORD pid() const { return pid_; }
    HANDLE handle() const { return handle_; }
    bool   ok() const { return handle_ != nullptr; }

private:
    DWORD  pid_    = 0;
    HANDLE handle_ = nullptr;
};

} // namespace cs2
