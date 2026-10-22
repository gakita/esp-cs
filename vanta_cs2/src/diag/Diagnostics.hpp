#pragma once
#include "aim/AimConfig.hpp"
#include "esp/ESP.hpp"
#include "extras/Radar.hpp"
#include "game/EntityReader.hpp"
#include "offsets/OffsetManager.hpp"
#include "process/ModuleEnum.hpp"
#include "process/ModuleEnum.hpp"
#include "process/ProcessReader.hpp"

#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <string>

namespace vanta::diag {

struct Frame {
    std::chrono::steady_clock::time_point t;
    float render_ms{0};
    float read_ms{0};
    std::uint32_t entities{0};
};

struct Snapshot {
    float fps{0};
    float avg_render_ms{0};
    float avg_read_ms{0};
    std::uint64_t total_entities{0};
    std::uint64_t total_ticks{0};
    std::uint64_t read_count{0};
    std::uint64_t read_fail{0};
    std::uint64_t memory_working_set_mb{0};
    std::uint32_t module_present{0};
    std::uint32_t module_required{0};
    std::string offset_state;   // "unknown"/"confirmed"/"suspect"/...
    bool proc_attached{false};
    bool proc_alive{false};
    bool game_focused{false};
    aim::AimStats aim;
    esp::ESP::DrawStats esp;
    extras::RadarStats radar;
    std::vector<std::string> recent_errors;
};

// Rolling frame buffer plus error log. Thread-safe.
class Diagnostics {
public:
    void record_frame(float render_ms, float read_ms, std::uint32_t entities);
    void record_error(const std::string& text);

    Snapshot compute(const process::ProcessReader& proc,
                     const offsets::OffsetManager& offs,
                     const std::vector<process::ModuleCheck>& modules,
                     const aim::AimStats& aim,
                     const esp::ESP::DrawStats& esp,
                     const extras::RadarStats& radar,
                     bool game_focused) const;

    // Dumps the full in-memory diagnostics plus the last N log lines to a JSON
    // report at %LOCALAPPDATA%/VantaCS2/reports/<ts>.json.
    bool export_report(const std::string& reason);

private:
    mutable std::mutex m_mx;
    std::deque<Frame> m_frames;
    std::deque<std::string> m_errors;
};

} // namespace vanta::diag
