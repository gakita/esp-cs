#pragma once
#include <atomic>
#include <cstdint>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <windows.h>

namespace vanta::process {

struct ModuleInfo {
    std::string name;      // e.g. "client.dll"
    std::uintptr_t base;   // base address inside the target process
    std::size_t   size;    // image size in bytes
};

// Wraps an OpenProcess handle plus per-module cache. Thread-safe for reads.
// All read_* calls return false on failure; no exceptions on read errors
// (so hot paths can keep running during process restarts).
class ProcessReader {
public:
    ProcessReader() = default;
    ~ProcessReader();

    ProcessReader(const ProcessReader&) = delete;
    ProcessReader& operator=(const ProcessReader&) = delete;

    // Finds the first process whose image name matches `exe_name`
    // (case-insensitive). Caches the pid and opens a handle with PROCESS_VM_READ
    // and PROCESS_QUERY_LIMITED_INFORMATION. Returns false if the process
    // is not running or cannot be opened.
    bool attach(std::wstring_view exe_name);

    // Closes the handle and clears caches.
    void detach();

    bool is_attached() const { return m_handle != nullptr; }
    DWORD pid() const { return m_pid; }

    // True iff the kernel still reports the process alive.
    bool is_process_alive() const;

    // Refreshes the module cache from Toolhelp32. Call after attach() and
    // whenever the target reloads modules.
    bool refresh_modules();

    // Returns a copy of the current module list.
    std::vector<ModuleInfo> modules() const;

    // Finds a module by case-insensitive name match (e.g. "client.dll").
    std::optional<ModuleInfo> find_module(std::string_view name) const;

    // Low-level ReadProcessMemory. Returns true iff all `size` bytes were read.
    bool read_raw(std::uintptr_t address, void* out, std::size_t size) const;

    // Typed read; returns std::nullopt on failure.
    template <typename T>
    std::optional<T> read(std::uintptr_t address) const {
        T v{};
        if (!read_raw(address, &v, sizeof(T))) return std::nullopt;
        return v;
    }

    // Reads a bounded ASCII C-string at `address` (max 255 chars).
    std::string read_string(std::uintptr_t address, std::size_t max_len = 256) const;

    // Counters for diagnostics.
    std::uint64_t read_count() const { return m_read_count.load(std::memory_order_relaxed); }
    std::uint64_t read_fail_count() const { return m_read_fail.load(std::memory_order_relaxed); }

private:
    HANDLE m_handle{nullptr};
    DWORD  m_pid{0};
    mutable std::atomic<std::uint64_t> m_read_count{0};
    mutable std::atomic<std::uint64_t> m_read_fail{0};
    std::vector<ModuleInfo> m_modules;
};

} // namespace vanta::process
