#include "offsets/OffsetCache.hpp"

#include "core/AtomicFile.hpp"
#include "core/FilePaths.hpp"
#include "core/Log.hpp"

namespace vanta::offsets {

using nlohmann::json;

static json load_json_or(const std::filesystem::path& p, const json& fallback) {
    std::string raw;
    if (!io::read_file(p, raw)) return fallback;
    try { return json::parse(raw); } catch (...) {
        VLOG_WARN("corrupt JSON at {}; restoring default", p.string());
        return fallback;
    }
}

static OffsetBundle bundle_from_json_safe(const json& j) {
    std::string err;
    auto b = OffsetBundle::from_json(j, &err);
    if (!err.empty()) VLOG_WARN("bundle parse: {}", err);
    return b;
}

std::optional<OffsetBundle> OffsetCache::active() const {
    auto j = load_json_or(paths::active_offsets(), json::object());
    if (j.empty() || !j.is_object() || !j.contains("fields")) return std::nullopt;
    return bundle_from_json_safe(j);
}

std::vector<OffsetBundle> OffsetCache::history() const {
    auto j = load_json_or(paths::offsets_history(), json::array());
    std::vector<OffsetBundle> out;
    if (!j.is_array()) return out;
    out.reserve(j.size());
    for (auto& item : j) out.push_back(bundle_from_json_safe(item));
    return out;
}

bool OffsetCache::commit_active(const OffsetBundle& b) {
    auto hist_json = load_json_or(paths::offsets_history(), json::array());
    if (!hist_json.is_array()) hist_json = json::array();

    // Fold previous active into history.
    if (auto prev = active()) {
        hist_json.push_back(prev->to_json());
        if (hist_json.size() > 32) {
            hist_json.erase(hist_json.begin(), hist_json.begin() + (hist_json.size() - 32));
        }
    }

    // Transactional: write history first, then active. If the second write
    // fails the pre-swap active stays on disk (we only appended history).
    if (!io::write_atomic(paths::offsets_history(), hist_json.dump(2))) {
        VLOG_ERROR("failed to persist offsets history");
        return false;
    }
    if (!io::write_atomic(paths::active_offsets(), b.to_json().dump(2))) {
        VLOG_ERROR("failed to persist active offsets");
        return false;
    }
    return true;
}

bool OffsetCache::rollback_to(std::size_t index) {
    auto hist = history();
    if (index >= hist.size()) return false;
    return commit_active(hist[index]);
}

std::optional<OffsetBundle> OffsetCache::by_hash(const std::string& h) const {
    if (auto a = active(); a && a->sha256_hex == h) return a;
    for (auto& b : history()) if (b.sha256_hex == h) return b;
    return std::nullopt;
}

void OffsetCache::prune(std::size_t keep) {
    auto hist = history();
    if (hist.size() <= keep) return;
    json arr = json::array();
    for (std::size_t i = hist.size() - keep; i < hist.size(); ++i) {
        arr.push_back(hist[i].to_json());
    }
    io::write_atomic(paths::offsets_history(), arr.dump(2));
}

} // namespace vanta::offsets
