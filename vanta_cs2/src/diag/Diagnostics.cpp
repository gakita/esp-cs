#include "diag/Diagnostics.hpp"

#include "core/AtomicFile.hpp"
#include "core/FilePaths.hpp"

#include <nlohmann/json.hpp>
#include <windows.h>
#include <psapi.h>

#include <chrono>
#include <ctime>
#include <filesystem>

namespace vanta::diag {

namespace {
constexpr std::size_t kMaxFrames = 240;
constexpr std::size_t kMaxErrors = 128;
}

void Diagnostics::record_frame(float render_ms, float read_ms, std::uint32_t ents) {
    std::lock_guard lock(m_mx);
    Frame f;
    f.t = std::chrono::steady_clock::now();
    f.render_ms = render_ms;
    f.read_ms   = read_ms;
    f.entities  = ents;
    m_frames.push_back(f);
    while (m_frames.size() > kMaxFrames) m_frames.pop_front();
}

void Diagnostics::record_error(const std::string& text) {
    std::lock_guard lock(m_mx);
    m_errors.push_back(text);
    while (m_errors.size() > kMaxErrors) m_errors.pop_front();
}

Snapshot Diagnostics::compute(const process::ProcessReader& proc,
                              const offsets::OffsetManager& offs,
                              const std::vector<process::ModuleCheck>& modules,
                              const aim::AimStats& aim_s,
                              const esp::ESP::DrawStats& esp_s,
                              const extras::RadarStats& radar_s,
                              bool focused) const {
    Snapshot s;
    s.proc_attached = proc.is_attached();
    s.proc_alive    = proc.is_process_alive();
    s.game_focused  = focused;
    s.aim           = aim_s;
    s.esp           = esp_s;
    s.radar         = radar_s;
    s.read_count    = proc.read_count();
    s.read_fail     = proc.read_fail_count();

    std::uint32_t present = 0, required = 0;
    for (auto& m : modules) {
        if (m.required) required++;
        if (m.present && m.required) present++;
    }
    s.module_present  = present;
    s.module_required = required;

    if (auto active = offs.active_cached()) {
        s.offset_state = offsets::compat_label(active->overall());
    } else {
        s.offset_state = "none";
    }

    {
        std::lock_guard lock(m_mx);
        if (!m_frames.empty()) {
            const auto& front = m_frames.front();
            const auto& back  = m_frames.back();
            auto span = std::chrono::duration<float>(back.t - front.t).count();
            s.fps = span > 0.0f ? ((float)m_frames.size() - 1) / span : 0.0f;
            float rsum = 0.0f, dsum = 0.0f;
            std::uint64_t etot = 0;
            for (auto& f : m_frames) { rsum += f.render_ms; dsum += f.read_ms; etot += f.entities; }
            s.avg_render_ms = rsum / (float)m_frames.size();
            s.avg_read_ms   = dsum / (float)m_frames.size();
            s.total_entities = etot;
        }
        s.total_ticks = m_frames.size();
        for (auto& e : m_errors) s.recent_errors.push_back(e);
    }

    PROCESS_MEMORY_COUNTERS_EX pm{};
    if (::GetProcessMemoryInfo(::GetCurrentProcess(),
                               (PROCESS_MEMORY_COUNTERS*)&pm, sizeof(pm))) {
        s.memory_working_set_mb = pm.WorkingSetSize / (1024 * 1024);
    }
    return s;
}

bool Diagnostics::export_report(const std::string& reason) {
    using nlohmann::json;
    auto reports = paths::app_data() / L"reports";
    std::error_code ec;
    std::filesystem::create_directories(reports, ec);

    std::time_t t = std::time(nullptr);
    std::tm tmv{};
    localtime_s(&tmv, &t);
    char ts[32];
    std::strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", &tmv);
    auto path = reports / (std::string(ts) + ".json");

    json j;
    j["reason"] = reason;
    j["generated_at_unix"] = (std::uint64_t)t;
    j["version"] = "0.1.0";
    {
        std::lock_guard lock(m_mx);
        j["frames_window"] = m_frames.size();
        j["recent_errors"] = m_errors;
    }
    return io::write_atomic(path, j.dump(2));
}

} // namespace vanta::diag
