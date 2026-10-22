#include "process/ModuleEnum.hpp"

namespace vanta::process {

std::vector<ModuleCheck> verify_modules(const ProcessReader& r) {
    std::vector<ModuleCheck> out;
    out.reserve(std::size(kCS2Modules));
    for (auto& spec : kCS2Modules) {
        ModuleCheck c;
        c.name     = std::string(spec.name);
        c.required = spec.required;
        c.present  = false;
        c.base     = 0;
        c.size     = 0;
        if (auto m = r.find_module(spec.name)) {
            c.present = true;
            c.base    = m->base;
            c.size    = m->size;
        }
        out.push_back(std::move(c));
    }
    return out;
}

bool required_modules_ok(const std::vector<ModuleCheck>& v) {
    for (auto& c : v) if (c.required && !c.present) return false;
    return true;
}

} // namespace vanta::process
