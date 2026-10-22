#include "core/Config.hpp"

#include "core/AtomicFile.hpp"
#include "core/FilePaths.hpp"
#include "core/Log.hpp"

#include <sstream>

namespace vanta::config {

using nlohmann::json;

Settings& Settings::instance() {
    static Settings s;
    return s;
}

static json default_root() {
    return {
        {"schema",   Settings::kSchemaVersion},
        {"ui",       {{"page", "dashboard"}, {"dpi_scale", 1.0}}},
        {"esp",      {{"enabled", false}}},
        {"aim",      {{"enabled", false}}},
        {"trigger",  {{"enabled", false}}},
        {"recoil",   {{"enabled", false}}},
        {"radar",    {{"enabled", false}}},
        {"crosshair",{{"enabled", false}}},
        {"bhop",     {{"enabled", false}}},
        {"profiles", {{"active", "default"}}},
        {"hotkeys",  json::object()},
    };
}

void Settings::migrate(json& root) {
    int v = root.value("schema", 0);
    if (v == Settings::kSchemaVersion) return;

    if (v < 1) {
        // Pre-schema or missing: seed defaults for anything missing.
        auto d = default_root();
        for (auto it = d.begin(); it != d.end(); ++it) {
            if (!root.contains(it.key())) root[it.key()] = it.value();
        }
    }
    if (v < 2) {
        // v1->v2: rename "aimbot" to "aim" if present.
        if (root.contains("aimbot")) {
            root["aim"] = root["aimbot"];
            root.erase("aimbot");
        }
    }
    root["schema"] = Settings::kSchemaVersion;
}

void Settings::load() {
    std::lock_guard lock(m_mx);
    std::string raw;
    auto path = vanta::paths::settings_file();
    if (!io::read_file(path, raw)) {
        m_data = default_root();
        return;
    }
    try {
        m_data = json::parse(raw);
        if (!m_data.is_object()) throw std::runtime_error("not an object");
        migrate(m_data);
    } catch (const std::exception& e) {
        VLOG_WARN("settings.json corrupted ({}), restoring defaults", e.what());
        m_data = default_root();
    }
}

void Settings::save() {
    std::string s;
    {
        std::lock_guard lock(m_mx);
        s = m_data.dump(2);
    }
    if (!io::write_atomic(vanta::paths::settings_file(), s)) {
        VLOG_WARN("settings.json atomic save failed");
    }
}

static std::vector<std::string> split_dots(const std::string& k) {
    std::vector<std::string> parts;
    std::string cur;
    for (char c : k) {
        if (c == '.') { if (!cur.empty()) parts.push_back(std::move(cur)); cur.clear(); }
        else cur.push_back(c);
    }
    if (!cur.empty()) parts.push_back(std::move(cur));
    return parts;
}

json* Settings::resolve(json& root, const std::string& key, bool create) {
    json* cur = &root;
    for (auto& p : split_dots(key)) {
        if (!cur->is_object()) {
            if (!create) return nullptr;
            *cur = json::object();
        }
        if (!cur->contains(p)) {
            if (!create) return nullptr;
            (*cur)[p] = json::object();
        }
        cur = &(*cur)[p];
    }
    return cur;
}

const json* Settings::resolve_const(const json& root, const std::string& key) const {
    const json* cur = &root;
    for (auto& p : split_dots(key)) {
        if (!cur->is_object() || !cur->contains(p)) return nullptr;
        cur = &(*cur)[p];
    }
    return cur;
}

json Settings::get(const std::string& key, const json& fallback) const {
    std::lock_guard lock(m_mx);
    const json* n = resolve_const(m_data, key);
    return n ? *n : fallback;
}

void Settings::set(const std::string& key, json value) {
    std::lock_guard lock(m_mx);
    json* n = resolve(m_data, key, true);
    if (n) *n = std::move(value);
}

json Settings::snapshot() const {
    std::lock_guard lock(m_mx);
    return m_data;
}

} // namespace vanta::config
