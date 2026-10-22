#pragma once
#include "process/ProcessReader.hpp"

#include <string_view>
#include <vector>

namespace vanta::process {

// Required modules the CS2 process must expose for Vanta to work.
// Resolved via ProcessReader::find_module after refresh_modules().
struct RequiredModule {
    std::string_view name;
    bool required;        // if false, warn-only
};

inline constexpr RequiredModule kCS2Modules[] = {
    {"client.dll",   true},
    {"engine2.dll",  true},
    {"server.dll",   false}, // only present in server-attached builds
    {"schemasystem.dll", true},
    {"tier0.dll",    true},
};

struct ModuleCheck {
    std::string name;
    bool required;
    bool present;
    std::uintptr_t base;
    std::size_t    size;
};

// Returns one entry per module in kCS2Modules.
std::vector<ModuleCheck> verify_modules(const ProcessReader& r);

// True iff every required module in kCS2Modules is present.
bool required_modules_ok(const std::vector<ModuleCheck>& v);

} // namespace vanta::process
