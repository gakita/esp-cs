#pragma once
#include <nlohmann/json.hpp>

#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace vanta::profile {

// Profile = named snapshot of the whole settings tree. Stored as individual
// JSON files under %LOCALAPPDATA%/VantaCS2/profiles.
class ProfileManager {
public:
    void refresh();                      // rescan directory
    std::vector<std::string> list() const;

    bool create(const std::string& name, const nlohmann::json& data);
    bool rename(const std::string& old_name, const std::string& new_name);
    bool duplicate(const std::string& name, const std::string& new_name);
    bool remove(const std::string& name); // delete file atomically
    std::optional<nlohmann::json> load(const std::string& name) const;

    bool activate(const std::string& name);      // swap into live Settings
    bool save_active(const std::string& name);   // write current Settings into profile
    std::string active() const;

    nlohmann::json export_profile(const std::string& name) const;
    bool           import_profile(const std::string& name, const nlohmann::json& data);

    void restore_defaults();

private:
    std::string sanitize(const std::string& name) const;

    mutable std::mutex m_mx;
    std::vector<std::string> m_cached;
    std::string m_active{"default"};
};

} // namespace vanta::profile
