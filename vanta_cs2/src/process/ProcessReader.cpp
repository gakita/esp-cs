#include "process/ProcessReader.hpp"

#include "core/Log.hpp"

#include <algorithm>
#include <cctype>
#include <cwctype>
#include <cwctype>
#include <tlhelp32.h>
#include <psapi.h>

namespace vanta::process {

static bool iequals_a(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) return false;
    }
    return true;
}

static bool iequals_w(std::wstring_view a, std::wstring_view b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (::towlower(a[i]) != ::towlower(b[i])) return false;
    }
    return true;
}

static std::string wide_to_utf8(std::wstring_view w) {
    if (w.empty()) return {};
    int sz = ::WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(),
                                   nullptr, 0, nullptr, nullptr);
    std::string out(sz, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(),
                          out.data(), sz, nullptr, nullptr);
    return out;
}

ProcessReader::~ProcessReader() { detach(); }

bool ProcessReader::attach(std::wstring_view exe) {
    detach();

    HANDLE snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    bool found = false;
    if (::Process32FirstW(snap, &entry)) {
        do {
            if (iequals_w(entry.szExeFile, exe)) {
                m_pid = entry.th32ProcessID;
                found = true;
                break;
            }
        } while (::Process32NextW(snap, &entry));
    }
    ::CloseHandle(snap);
    if (!found) return false;

    m_handle = ::OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION,
                             FALSE, m_pid);
    if (!m_handle) {
        VLOG_WARN("OpenProcess failed on pid {} (err {})", m_pid, ::GetLastError());
        m_pid = 0;
        return false;
    }

    if (!refresh_modules()) {
        VLOG_WARN("Attached to pid {} but module enum failed", m_pid);
    }
    VLOG_INFO("Attached to {} (pid {})", wide_to_utf8(exe), m_pid);
    return true;
}

void ProcessReader::detach() {
    if (m_handle) ::CloseHandle(m_handle);
    m_handle = nullptr;
    m_pid = 0;
    m_modules.clear();
}

bool ProcessReader::is_process_alive() const {
    if (!m_handle) return false;
    DWORD code = 0;
    if (!::GetExitCodeProcess(m_handle, &code)) return false;
    return code == STILL_ACTIVE;
}

bool ProcessReader::refresh_modules() {
    if (!m_handle) return false;

    HANDLE snap = ::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_pid);
    if (snap == INVALID_HANDLE_VALUE) return false;

    std::vector<ModuleInfo> list;
    MODULEENTRY32W me{};
    me.dwSize = sizeof(me);
    if (::Module32FirstW(snap, &me)) {
        do {
            ModuleInfo mi;
            mi.name = wide_to_utf8(me.szModule);
            mi.base = reinterpret_cast<std::uintptr_t>(me.modBaseAddr);
            mi.size = me.modBaseSize;
            list.push_back(std::move(mi));
        } while (::Module32NextW(snap, &me));
    }
    ::CloseHandle(snap);
    m_modules = std::move(list);
    return !m_modules.empty();
}

std::vector<ModuleInfo> ProcessReader::modules() const { return m_modules; }

std::optional<ModuleInfo> ProcessReader::find_module(std::string_view name) const {
    for (auto& m : m_modules) {
        if (iequals_a(m.name, name)) return m;
    }
    return std::nullopt;
}

bool ProcessReader::read_raw(std::uintptr_t address, void* out, std::size_t size) const {
    m_read_count.fetch_add(1, std::memory_order_relaxed);
    if (!m_handle || !address || !out || size == 0) {
        m_read_fail.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    SIZE_T got = 0;
    if (!::ReadProcessMemory(m_handle, reinterpret_cast<LPCVOID>(address),
                             out, size, &got) || got != size) {
        m_read_fail.fetch_add(1, std::memory_order_relaxed);
        return false;
    }
    return true;
}

std::string ProcessReader::read_string(std::uintptr_t address, std::size_t max_len) const {
    if (max_len == 0) return {};
    std::vector<char> buf(max_len, 0);
    if (!read_raw(address, buf.data(), max_len)) return {};
    buf.back() = 0;
    return std::string(buf.data());
}

} // namespace vanta::process
