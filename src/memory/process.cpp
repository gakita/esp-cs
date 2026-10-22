#include "process.hpp"
#include <TlHelp32.h>

namespace mem {

static DWORD find_pid(std::wstring_view name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe{sizeof(pe)}; DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do { if (_wcsicmp(pe.szExeFile, name.data()) == 0) { pid = pe.th32ProcessID; break; } }
        while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

bool Process::attach(std::wstring_view process_name) {
    detach();
    pid_ = find_pid(process_name);
    if (!pid_) return false;
    handle_ = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION, FALSE, pid_);
    if (!handle_) { pid_ = 0; return false; }

    struct Ctx { DWORD pid; HWND hwnd; } ctx{pid_, nullptr};
    EnumWindows([](HWND h, LPARAM lp) -> BOOL {
        auto* c = reinterpret_cast<Ctx*>(lp);
        DWORD wpid = 0; GetWindowThreadProcessId(h, &wpid);
        if (wpid != c->pid) return TRUE;
        if (!IsWindowVisible(h)) return TRUE;
        wchar_t cls[64]{}; GetClassNameW(h, cls, 64);
        if (wcsstr(cls, L"SDL_app") || wcsstr(cls, L"Valve")) { c->hwnd = h; return FALSE; }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&ctx));
    window_ = ctx.hwnd;
    return true;
}

void Process::detach() {
    if (handle_) CloseHandle(handle_);
    handle_ = nullptr; pid_ = 0; window_ = nullptr;
}

std::uintptr_t Process::module_base(std::wstring_view name) const {
    if (!pid_) return 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid_);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    std::uintptr_t base = 0; MODULEENTRY32W me{sizeof(me)};
    if (Module32FirstW(snap, &me)) {
        do { if (_wcsicmp(me.szModule, name.data()) == 0) { base = reinterpret_cast<std::uintptr_t>(me.modBaseAddr); break; } }
        while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return base;
}

std::string Process::read_cstring(std::uintptr_t addr, std::size_t max) const {
    std::string out; out.resize(max); SIZE_T got = 0;
    if (!ReadProcessMemory(handle_, reinterpret_cast<LPCVOID>(addr), out.data(), max, &got)) return {};
    out.resize(got);
    if (auto nul = out.find('\0'); nul != std::string::npos) out.resize(nul);
    return out;
}

} // namespace mem