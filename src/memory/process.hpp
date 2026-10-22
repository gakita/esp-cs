#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>
#include <string_view>
#include <optional>

namespace mem {

class Process {
public:
    Process() = default;
    ~Process() { detach(); }
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;

    bool attach(std::wstring_view process_name);
    void detach();
    bool valid() const { return handle_ != nullptr; }

    std::uintptr_t module_base(std::wstring_view name) const;

    template <typename T>
    T read(std::uintptr_t addr) const {
        T v{}; SIZE_T got = 0;
        ::ReadProcessMemory(handle_, reinterpret_cast<LPCVOID>(addr), &v, sizeof(T), &got);
        return v;
    }

    template <typename T>
    std::optional<T> try_read(std::uintptr_t addr) const {
        T v{}; SIZE_T got = 0;
        if (!::ReadProcessMemory(handle_, reinterpret_cast<LPCVOID>(addr), &v, sizeof(T), &got)) return std::nullopt;
        if (got != sizeof(T)) return std::nullopt;
        return v;
    }

    bool read_raw(std::uintptr_t addr, void* dst, std::size_t n) const {
        SIZE_T got = 0;
        return ::ReadProcessMemory(handle_, reinterpret_cast<LPCVOID>(addr), dst, n, &got) && got == n;
    }

    std::string read_cstring(std::uintptr_t addr, std::size_t max = 128) const;

    HWND  window() const { return window_; }
    DWORD pid()    const { return pid_; }

private:
    HANDLE handle_{nullptr};
    DWORD  pid_{0};
    HWND   window_{nullptr};
};

} // namespace mem