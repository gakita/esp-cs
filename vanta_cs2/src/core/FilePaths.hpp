#pragma once
#include <filesystem>
#include <string>

namespace vanta::paths {

// Returns %LOCALAPPDATA%/VantaCS2, creating it if missing.
// Throws std::runtime_error if the folder cannot be created.
std::filesystem::path app_data();

std::filesystem::path logs_dir();
std::filesystem::path profiles_dir();
std::filesystem::path offsets_dir();
std::filesystem::path cache_dir();

std::filesystem::path settings_file();   // settings.json
std::filesystem::path sources_file();    // offset-sources.json
std::filesystem::path active_offsets();  // offsets/active.json
std::filesystem::path offsets_history(); // offsets/history.json

} // namespace vanta::paths
