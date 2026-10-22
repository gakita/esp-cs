#include <algorithm>
#include <cmath>
#include <chrono>

#include "game/EntityReader.hpp"
#include "core/Log.hpp"

std::uint32_t  g_dbg_scanned     = 0;
std::uint32_t  g_dbg_accepted    = 0;
std::uintptr_t g_dbg_system      = 0;
std::uintptr_t g_dbg_page0       = 0;
std::uintptr_t g_dbg_ent0        = 0;
std::uint32_t  g_dbg_rej_hp = 0, g_dbg_rej_life = 0, g_dbg_rej_team = 0;
std::uint32_t  g_dbg_rej_scene = 0, g_dbg_rej_dormant = 0, g_dbg_rej_origin = 0;

namespace vanta::game {
using clock_t_ = std::chrono::steady_clock;

EntityReader::EntityReader(process::ProcessReader& p, offsets::OffsetManager& o,
                           GameState& g, std::atomic<bool>& focused)
    : m_proc(p), m_offs(o), m_out(g), m_focused(focused) {}
EntityReader::~EntityReader() { stop(); }

void EntityReader::start(int hz) {
    if (m_running.exchange(true)) return;
    m_thread = std::thread([this, hz]{ loop(hz); });
}
void EntityReader::stop() {
    if (!m_running.exchange(false)) return;
    if (m_thread.joinable()) m_thread.join();
}
void EntityReader::loop(int target_hz) {
    const auto frame = std::chrono::microseconds(1'000'000 / std::max(1, target_hz));
    auto next = clock_t_::now();
    while (m_running.load()) {
        const auto t0 = clock_t_::now();
        auto snap = std::make_shared<Snapshot>();
        snap->tick_index     = m_tick_count.fetch_add(1) + 1;
        snap->captured_at_ns = (std::uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(t0.time_since_epoch()).count();
        snap->game_focused   = m_focused.load();
        snap->attached       = m_proc.is_attached() && m_proc.is_process_alive();
        if (snap->attached) do_one_tick(snap);
        m_out.publish(snap);
        const auto dt = std::chrono::duration_cast<std::chrono::microseconds>(clock_t_::now() - t0);
        m_last_tick_us.store((std::uint64_t)dt.count());
        next += frame;
        auto now = clock_t_::now();
        if (next > now) std::this_thread::sleep_until(next);
        else next = now;
    }
}

std::uintptr_t EntityReader::resolve_handle(std::uintptr_t system, std::uint32_t handle) const {
    const std::uint32_t idx = handle & 0x7FFF;
    const auto chunk = m_proc.read<std::uintptr_t>(system + 0x10 + 8 * (idx >> 9)).value_or(0);
    if (!chunk) return 0;
    return m_proc.read<std::uintptr_t>(chunk + 120 * (idx & 0x1FF)).value_or(0);
}

void EntityReader::do_one_tick(std::shared_ptr<Snapshot>& snap) {
    auto client = m_proc.find_module("client.dll");
    if (!client) return;

    auto resolve_mod = [&](const char* name) -> std::uintptr_t {
        bool ok = false;
        auto v = m_offs.resolve(name, &ok);
        return ok ? client->base + v : 0;
    };

    const auto a_entitylist    = resolve_mod("dwEntityList");
    const auto a_localpawn_ptr = resolve_mod("dwLocalPlayerPawn");
    const auto a_localctrl_ptr = resolve_mod("dwLocalPlayerController");

    if (auto vp_base = resolve_mod("dwViewMatrix")) {
        read_view_projection(vp_base, snap->view_projection);
    }

    const auto local_pawn = a_localpawn_ptr ? m_proc.read<std::uintptr_t>(a_localpawn_ptr).value_or(0) : 0;
    const auto local_ctrl = a_localctrl_ptr ? m_proc.read<std::uintptr_t>(a_localctrl_ptr).value_or(0) : 0;

    if (local_pawn) {
        Entity local;
        local.pawn_address = local_pawn;
        local.index = 0; local.is_local = true; local.alive = true;
        local.team = Team::CounterTerr; local.health = 100;
        bool ok_scene=false, ok_origin=false;
        const auto o_scene = m_offs.resolve("C_BaseEntity::m_pGameSceneNode", &ok_scene);
        const auto o_origin = m_offs.resolve("CGameSceneNode::m_vecAbsOrigin", &ok_origin);
        if (ok_scene && ok_origin) {
            const auto scene = m_proc.read<std::uintptr_t>(local_pawn + o_scene).value_or(0);
            if (scene) {
                auto o = m_proc.read<math::Vec3>(scene + o_origin);
                if (o) { local.origin = *o; local.eye_pos = *o; local.eye_pos.z += 64.0f; }
            }
        }
        snap->local = local;
    }

    const auto system = a_entitylist ? m_proc.read<std::uintptr_t>(a_entitylist).value_or(0) : 0;
    g_dbg_system = system;

    // ============ BRUTE-FORCE: scan ctrl+0..0x1000 pra achar m_hPlayerPawn ============
    if (snap->tick_index == 30 && system && local_ctrl && local_pawn) {
        VLOG_INFO("[BRUTE] system=0x{:x} local_ctrl=0x{:x} local_pawn=0x{:x}",
            (unsigned long long)system,
            (unsigned long long)local_ctrl,
            (unsigned long long)local_pawn);

        // Le 0x1000 bytes do controller de uma vez
        std::vector<std::uint8_t> buf(0x1000, 0);
        if (m_proc.read_raw(local_ctrl, buf.data(), buf.size())) {
            int matches = 0;
            for (std::size_t off = 0; off + 4 <= buf.size(); off += 4) {
                std::uint32_t handle = *reinterpret_cast<std::uint32_t*>(buf.data() + off);
                if (handle == 0 || handle == 0xFFFFFFFFu) continue;
                if ((handle & 0x7FFF) == 0) continue;

                const auto resolved = resolve_handle(system, handle);
                if (resolved == local_pawn) {
                    VLOG_INFO("[BRUTE] MATCH! ctrl+0x{:x} = handle 0x{:x} -> local_pawn",
                        off, handle);
                    matches++;
                    if (matches > 20) break;
                }
            }
            if (matches == 0) {
                VLOG_INFO("[BRUTE] NO MATCH found in ctrl+0..0x1000. Trying ctrl+0x1000..0x2000...");
                // Segunda passada 0x1000..0x2000
                std::vector<std::uint8_t> buf2(0x1000, 0);
                if (m_proc.read_raw(local_ctrl + 0x1000, buf2.data(), buf2.size())) {
                    for (std::size_t off = 0; off + 4 <= buf2.size(); off += 4) {
                        std::uint32_t handle = *reinterpret_cast<std::uint32_t*>(buf2.data() + off);
                        if (handle == 0 || handle == 0xFFFFFFFFu) continue;
                        if ((handle & 0x7FFF) == 0) continue;
                        const auto resolved = resolve_handle(system, handle);
                        if (resolved == local_pawn) {
                            VLOG_INFO("[BRUTE] MATCH! ctrl+0x{:x} = handle 0x{:x} -> local_pawn",
                                off + 0x1000, handle);
                            matches++;
                            if (matches > 20) break;
                        }
                    }
                }
                if (matches == 0) {
                    VLOG_INFO("[BRUTE] still NO MATCH in 0..0x2000. Something weird.");
                }
            }
        } else {
            VLOG_INFO("[BRUTE] read_raw failed on local_ctrl");
        }
    }

    g_dbg_scanned = 0; g_dbg_accepted = 0;
    m_total_entities.fetch_add((snap->local ? 1u : 0u));
}

bool EntityReader::read_view_projection(std::uintptr_t base, math::Mat4& out) {
    float raw[16]{};
    if (!m_proc.read_raw(base, raw, sizeof(raw))) return false;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            out(r, c) = raw[r * 4 + c];
    return true;
}

bool EntityReader::read_entity_pawn(Entity&) { return true; }
bool EntityReader::read_bones(const Entity&, Entity&) { return true; }

} // namespace vanta::game