#include "core/FilePaths.hpp"

#include <stdexcept>
#include <windows.h>
#include <shlobj.h>
#include <knownfolders.h>

namespace vanta::paths {
namespace fs = std::filesystem;

static fs::path known_local_appdata() {
    PWSTR raw = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &raw);
    if (FAILED(hr) || !raw) {
        if (raw) CoTaskMemFree(raw);
        throw std::runtime_error("SHGetKnownFolderPath(LocalAppData) failed");
    }
    fs::path p(raw);
    CoTaskMemFree(raw);
    return p;
}

fs::path app_data() {
    auto p = known_local_appdata() / L"VantaCS2";
    std::error_code ec;
    fs::create_directories(p, ec);
    if (ec && !fs::exists(p)) {
        throw std::runtime_error("Could not create " + p.string() + ": " + ec.message());
    }
    return p;
}

static fs::path sub(const wchar_t* name) {
    auto p = app_data() / name;
    std::error_code ec;
    fs::create_directories(p, ec);
    return p;
}

fs::path logs_dir()        { return sub(L"logs"); }
fs::path profiles_dir()    { return sub(L"profiles"); }
fs::path offsets_dir()     { return sub(L"offsets"); }
fs::path cache_dir()       { return sub(L"cache"); }

fs::path settings_file()   { return app_data() / L"settings.json"; }
fs::path sources_file()    { return app_data() / L"offset-sources.json"; }
fs::path active_offsets()  { return offsets_dir() / L"active.json"; }
fs::path offsets_history() { return offsets_dir() / L"history.json"; }

} // namespace vanta::paths
