// game/EntityReader.hpp
#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>

#include "process/ProcessReader.hpp"
#include "offsets/OffsetManager.hpp"
#include "game/GameState.hpp"
#include "math/Mat4.hpp"

namespace vanta::game {

class EntityReader {
public:
    EntityReader(process::ProcessReader& proc,
                 offsets::OffsetManager& offs,
                 GameState& out,
                 std::atomic<bool>& focused);
    ~EntityReader();

    EntityReader(const EntityReader&)            = delete;
    EntityReader& operator=(const EntityReader&) = delete;

    void start(int hz);
    void stop();

    // Diagnostics
    std::uint64_t last_tick_us()   const { return m_last_tick_us.load(); }
    std::uint64_t total_entities() const { return m_total_entities.load(); }
    std::uint64_t tick_count()     const { return m_tick_count.load(); }

    // Convenience: last tick duration as a chrono value — this is what
    // App.cpp wants when it formats read_ms.
    std::chrono::microseconds last_tick_duration() const {
        return std::chrono::microseconds(m_last_tick_us.load());
    }

private:
    void loop(int target_hz);
    void do_one_tick(std::shared_ptr<Snapshot>& snap);
    bool read_view_projection(std::uintptr_t base, math::Mat4& out);
    bool read_entity_pawn(Entity& out);
    bool read_bones(const Entity& in, Entity& out);
    std::uintptr_t resolve_handle(std::uintptr_t system, std::uint32_t handle) const;

    process::ProcessReader& m_proc;
    offsets::OffsetManager& m_offs;
    GameState&              m_out;
    std::atomic<bool>&      m_focused;

    std::atomic<bool>       m_running{false};
    std::thread             m_thread;

    std::atomic<std::uint64_t> m_tick_count{0};
    std::atomic<std::uint64_t> m_last_tick_us{0};
    std::atomic<std::uint64_t> m_total_entities{0};
};

} // namespace vanta::game