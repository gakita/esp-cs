#pragma once
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>

namespace vanta::config {

// Opaque settings blob: UI pages write + read through this.
// Schema is a JSON object; migrations run on load.
class Settings {
public:
    static Settings& instance();

    void load();   // Loads from paths::settings_file(); creates default on miss.
    void save();   // Atomic save.

    nlohmann::json get(const std::string& dotted_key, const nlohmann::json& fallback) const;
    void set(const std::string& dotted_key, nlohmann::json value);

    nlohmann::json snapshot() const; // Thread-safe copy of the whole blob.

    static constexpr int kSchemaVersion = 2;

private:
    Settings() = default;
    void migrate(nlohmann::json& root);
    nlohmann::json* resolve(nlohmann::json& root, const std::string& key, bool create);
    const nlohmann::json* resolve_const(const nlohmann::json& root, const std::string& key) const;

    mutable std::mutex m_mx;
    nlohmann::json m_data;
};

} // namespace vanta::config
