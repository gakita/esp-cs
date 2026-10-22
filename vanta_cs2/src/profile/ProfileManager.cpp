#include "profile/ProfileManager.hpp"

#include "core/AtomicFile.hpp"
#include "core/Config.hpp"
#include "core/FilePaths.hpp"
#include "core/Log.hpp"

#include <algorithm>
#include <filesystem>

namespace vanta::profile {

using nlohmann::json;
namespace fs = std::filesystem;

std::string ProfileManager::sanitize(const std::string& name) const {
    std::string out;
    out.reserve(name.size());
    for (char c : name) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == ' ') {
            out.push_back(c);
        } else {
            out.push_back('_');
        }
    }
    return out;
}

void ProfileManager::refresh() {
    std::lock_guard lock(m_mx);
    m_cached.clear();
    std::error_code ec;
    for (auto& e : fs::directory_iterator(paths::profiles_dir(), ec)) {
        if (!e.is_regular_file()) continue;
        auto p = e.path();
        if (p.extension() == ".json") m_cached.push_back(p.stem().string());
    }
    std::sort(m_cached.begin(), m_cached.end());

    auto a = config::Settings::instance().get("profiles.active", "default").get<std::string>();
    m_active = a;
}

std::vector<std::string> ProfileManager::list() const {
    std::lock_guard lock(m_mx);
    return m_cached;
}

std::string ProfileManager::active() const {
    std::lock_guard lock(m_mx);
    return m_active;
}

bool ProfileManager::create(const std::string& name, const json& data) {
    auto path = paths::profiles_dir() / (sanitize(name) + ".json");
    if (fs::exists(path)) return false;
    if (!io::write_atomic(path, data.dump(2))) return false;
    refresh();
    return true;
}

bool ProfileManager::rename(const std::string& old_n, const std::string& new_n) {
    auto src = paths::profiles_dir() / (sanitize(old_n) + ".json");
    auto dst = paths::profiles_dir() / (sanitize(new_n) + ".json");
    std::error_code ec;
    if (!fs::exists(src) || fs::exists(dst)) return false;
    fs::rename(src, dst, ec);
    if (ec) return false;
    refresh();
    return true;
}

bool ProfileManager::duplicate(const std::string& name, const std::string& new_n) {
    auto data = load(name);
    if (!data) return false;
    return create(new_n, *data);
}

bool ProfileManager::remove(const std::string& name) {
    auto p = paths::profiles_dir() / (sanitize(name) + ".json");
    std::error_code ec;
    fs::remove(p, ec);
    refresh();
    return !ec;
}

std::optional<json> ProfileManager::load(const std::string& name) const {
    auto p = paths::profiles_dir() / (sanitize(name) + ".json");
    std::string raw;
    if (!io::read_file(p, raw)) return std::nullopt;
    try { return json::parse(raw); } catch (...) { return std::nullopt; }
}

bool ProfileManager::activate(const std::string& name) {
    auto d = load(name);
    if (!d) return false;
    auto& s = config::Settings::instance();
    for (auto it = d->begin(); it != d->end(); ++it) {
        s.set(it.key(), it.value());
    }
    s.set("profiles.active", name);
    s.save();
    {
        std::lock_guard lock(m_mx);
        m_active = name;
    }
    VLOG_INFO("activated profile '{}'", name);
    return true;
}

bool ProfileManager::save_active(const std::string& name) {
    auto snap = config::Settings::instance().snapshot();
    auto path = paths::profiles_dir() / (sanitize(name) + ".json");
    if (!io::write_atomic(path, snap.dump(2))) return false;
    refresh();
    return true;
}

json ProfileManager::export_profile(const std::string& name) const {
    auto d = load(name);
    return d.value_or(json::object());
}

bool ProfileManager::import_profile(const std::string& name, const json& data) {
    auto path = paths::profiles_dir() / (sanitize(name) + ".json");
    return io::write_atomic(path, data.dump(2));
}

void ProfileManager::restore_defaults() {
    // Wipes the active profile back to Settings defaults.
    auto& s = config::Settings::instance();
    s.set("schema", nullptr); // force migration path to seed defaults
    s.save();
    s.load();
}

} // namespace vanta::profile
