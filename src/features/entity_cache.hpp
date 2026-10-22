// src/features/entity_cache.hpp
#pragma once
#include "sdk/math.hpp"
#include "memory/process.hpp"
#include <array>
#include <atomic>
#include <mutex>
#include <thread>
#include <string>

namespace features {

struct PlayerEntry {
    bool            valid       = false;
    bool            alive       = false;
    bool            dormant     = false;
    std::uint8_t    team        = 0;
    std::int32_t    health      = 0;
    std::int32_t    max_health  = 100;
    std::int32_t    armor       = 0;
    sdk::Vec3       origin{};
    sdk::Vec3       head{};
    sdk::Vec3       bone_head{};
    sdk::Vec3       bone_chest{};
    sdk::Vec3       bone_pelvis{};
    bool            bones_valid = false;
    sdk::QAngle     eye_angles{};
    std::uint32_t   controller_idx = 0;
    std::string     name;
};

struct Snapshot {
    std::array<PlayerEntry, 64> players{};
    sdk::ViewMatrix             view{};
    std::uint8_t                local_team = 0;
    sdk::Vec3                   local_eye{};
    sdk::QAngle                 local_view_angles{};
    int                         screen_w = 1920;
    int                         screen_h = 1080;
    std::uint32_t               tick = 0;
};

// Per-tick rejection stats, aggregated by the cache thread and read by UI.
struct DebugStats {
    std::atomic<std::uint32_t> scanned{0};
    std::atomic<std::uint32_t> accepted{0};
    std::atomic<std::uint32_t> rej_handle{0};
    std::atomic<std::uint32_t> rej_hp{0};
    std::atomic<std::uint32_t> rej_life{0};
    std::atomic<std::uint32_t> rej_team{0};
    std::atomic<std::uint32_t> rej_scene{0};
    std::atomic<std::uint32_t> rej_dorm{0};
    std::atomic<std::uint32_t> rej_origin{0};
    std::atomic<std::uint32_t> last_hp_value{0};   // último hp VISTO (pra Sam ver se tá lixo)
    std::atomic<std::uint32_t> last_team_value{0};
    std::atomic<std::uint32_t> last_life_value{0};
};

class EntityCache {
public:
    void start(mem::Process& proc);
    void stop();
    const Snapshot& snapshot() const;

    // Debug knobs — mutáveis em tempo real pelo menu
    std::atomic<bool> cfg_ignore_dormant{false};
    std::atomic<bool> cfg_ignore_lifestate{false};
    std::atomic<bool> cfg_relax_hp_range{false};  // true = aceita 1..500 em vez de 1..100

    const DebugStats& stats() const { return stats_; }

private:
    void loop(mem::Process* proc);
    void poll_once(mem::Process& proc, std::uintptr_t client, std::uintptr_t engine, Snapshot& out);

    std::atomic<bool>  running_{false};
    std::jthread       thread_;
    mutable std::mutex mtx_;
    Snapshot           buf_[2]{};
    std::atomic<int>   front_{0};
    DebugStats         stats_;
};

}