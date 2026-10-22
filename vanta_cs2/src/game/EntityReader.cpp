#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdio>

#include "game/EntityReader.hpp"
#include "core/Log.hpp"

// --- Debug counters (visible from App.cpp via extern) ---------------------
std::uint32_t  g_dbg_scanned     = 0;
std::uint32_t  g_dbg_accepted    = 0;
std::uintptr_t g_dbg_system      = 0;
std::uintptr_t g_dbg_page0       = 0;
std::uintptr_t g_dbg_ent0        = 0;
// Rejection reason counters (per-tick, reset each tick)
std::uint32_t  g_dbg_rej_hp      = 0;
std::uint32_t  g_dbg_rej_life    = 0;
std::uint32_t  g_dbg_rej_team    = 0;
std::uint32_t  g_dbg_rej_scene   = 0;
std::uint32_t  g_dbg_rej_dormant = 0;
std::uint32_t  g_dbg_rej_origin  = 0;

namespace vanta::game {

using clock_t_ = std::chrono::steady_clock;

EntityReader::EntityReader(process::ProcessReader& p,
                           offsets::OffsetManager& o,
                           GameState& g,
                           std::atomic<bool>& focused)
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
        snap->captured_at_ns = (std::uint64_t)
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                t0.time_since_epoch()).count();
        snap->game_focused   = m_focused.load();
        snap->attached       = m_proc.is_attached() && m_proc.is_process_alive();

        if (snap->attached) do_one_tick(snap);
        m_out.publish(snap);

        const auto dt = std::chrono::duration_cast<std::chrono::microseconds>(
            clock_t_::now() - t0);
        m_last_tick_us.store((std::uint64_t)dt.count());

        next += frame;
        auto now = clock_t_::now();
        if (next > now) std::this_thread::sleep_until(next);
        else next = now;
    }
}

std::uintptr_t EntityReader::resolve_handle(std::uintptr_t system,
                                            std::uint32_t  handle) const {
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

    if (auto vp_base = resolve_mod("dwViewMatrix")) {
        read_view_projection(vp_base, snap->view_projection);
    }

    // Reset per-tick rejection counters.
    g_dbg_rej_hp = g_dbg_rej_life = g_dbg_rej_team = 0;
    g_dbg_rej_scene = g_dbg_rej_dormant = g_dbg_rej_origin = 0;

    // ---- local pawn ----
    std::uintptr_t local_pawn = 0;
    if (a_localpawn_ptr) {
        local_pawn = m_proc.read<std::uintptr_t>(a_localpawn_ptr).value_or(0);
        if (local_pawn) {
            Entity local;
            local.pawn_address = local_pawn;
            local.index        = 0;
            local.is_local     = true;
            if (read_entity_pawn(local)) {
                read_bones(local, local);
                snap->local = local;
            }
        }
    }

    g_dbg_scanned  = 0;
    g_dbg_accepted = 0;
    if (!a_entitylist) { m_total_entities.fetch_add((snap->local ? 1u : 0u)); return; }

    const auto system = m_proc.read<std::uintptr_t>(a_entitylist).value_or(0);
    g_dbg_system = system;
    if (!system) { m_total_entities.fetch_add((snap->local ? 1u : 0u)); return; }

    bool ok_hpawn = false;
    const auto o_hpawn = m_offs.resolve("CCSPlayerController::m_hPlayerPawn", &ok_hpawn);
    if (!ok_hpawn) { m_total_entities.fetch_add((snap->local ? 1u : 0u)); return; }

    if (auto s0 = m_proc.read<std::uintptr_t>(system + 0x10).value_or(0)) {
        g_dbg_page0 = s0;
        g_dbg_ent0  = m_proc.read<std::uintptr_t>(s0).value_or(0);
    }

    for (std::uint32_t ctrl_idx = 1; ctrl_idx <= 64; ++ctrl_idx) {
        const auto ctrl = resolve_handle(system, ctrl_idx);
        if (!ctrl) continue;

        const auto pawn_handle =
            m_proc.read<std::uint32_t>(ctrl + o_hpawn).value_or(0xFFFFFFFFu);
        if (pawn_handle == 0xFFFFFFFFu || (pawn_handle & 0x7FFF) == 0) continue;

        const auto pawn = resolve_handle(system, pawn_handle);
        if (!pawn || pawn == local_pawn) continue;

        g_dbg_scanned++;
        Entity e;
        e.pawn_address = pawn;
        e.index        = ctrl_idx;
        if (read_entity_pawn(e)) {
            g_dbg_accepted++;
            read_bones(e, e);
            snap->players.push_back(std::move(e));
        }
    }

    // -------- DEBUG PROBE: dump first 60 ticks worth of raw numbers --------
    // Once you see the Log output, you can rip this block out.
    {
        const auto tick = snap->tick_index;
        if (tick <= 60 && tick % 10 == 0) {
            core::Log::info(
                "[VANTA-PROBE] tick=%llu scan=%u acc=%u rej[hp=%u life=%u team=%u scene=%u dorm=%u orig=%u] players=%zu",
                (unsigned long long)tick,
                g_dbg_scanned, g_dbg_accepted,
                g_dbg_rej_hp, g_dbg_rej_life, g_dbg_rej_team,
                g_dbg_rej_scene, g_dbg_rej_dormant, g_dbg_rej_origin,
                snap->players.size());

            // dump first player's raw bone + origin
            if (!snap->players.empty()) {
                const auto& p = snap->players.front();
                core::Log::info(
                    "[VANTA-PROBE] p0 pawn=0x%llx origin=(%.1f %.1f %.1f) "
                    "head=(%.1f %.1f %.1f) chest=(%.1f %.1f %.1f) "
                    "hp=%d team=%u",
                    (unsigned long long)p.pawn_address,
                    p.origin.x, p.origin.y, p.origin.z,
                    p.bones[(int)Bone::Head].pos.x,
                    p.bones[(int)Bone::Head].pos.y,
                    p.bones[(int)Bone::Head].pos.z,
                    p.bones[(int)Bone::Chest].pos.x,
                    p.bones[(int)Bone::Chest].pos.y,
                    p.bones[(int)Bone::Chest].pos.z,
                    p.health, (unsigned)p.team);
            }
        }
    }

    // ---- bomb ----
    if (auto bomb_ptr = resolve_mod("dwPlantedC4")) {
        auto bomb = m_proc.read<std::uintptr_t>(bomb_ptr).value_or(0);
        if (bomb) {
            BombState b;
            bool ok1=false, ok2=false, ok3=false, ok4=false;
            auto o_tick    = m_offs.resolve("C_PlantedC4::m_bBombTicking",     &ok1);
            auto o_blow    = m_offs.resolve("C_PlantedC4::m_flC4Blow",         &ok2);
            auto o_defcd   = m_offs.resolve("C_PlantedC4::m_flDefuseCountDown",&ok3);
            auto o_defused = m_offs.resolve("C_PlantedC4::m_bBombDefused",     &ok4);
            if (ok1 && ok2) {
                const auto ticking = m_proc.read<bool>(bomb + o_tick).value_or(false);
                const auto blow    = m_proc.read<float>(bomb + o_blow).value_or(0.0f);
                const auto now_s   = snap->captured_at_ns / 1e9f;
                b.planted             = ticking;
                b.time_until_detonate = std::max(0.0f, blow - now_s);
            }
            if (ok3) {
                const auto def = m_proc.read<float>(bomb + o_defcd).value_or(0.0f);
                b.defusing           = def > 0.0f;
                b.time_until_defused = std::max(0.0f, def);
            }
            if (ok4) {
                if (m_proc.read<bool>(bomb + o_defused).value_or(false)) b.planted = false;
            }
            snap->bomb = b;
        }
    }

    m_total_entities.fetch_add(
        (std::uint64_t)snap->players.size() + (snap->local ? 1u : 0u));
}

bool EntityReader::read_view_projection(std::uintptr_t base, math::Mat4& out) {
    float raw[16]{};
    if (!m_proc.read_raw(base, raw, sizeof(raw))) return false;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            out(r, c) = raw[r * 4 + c];
    return true;
}

bool EntityReader::read_entity_pawn(Entity& out) {
    if (!out.pawn_address) return false;

    bool ok_team=false, ok_hp=false, ok_scene=false, ok_origin=false, ok_life=false;
    const auto o_team    = m_offs.resolve("C_BaseEntity::m_iTeamNum",       &ok_team);
    const auto o_hp      = m_offs.resolve("C_BaseEntity::m_iHealth",        &ok_hp);
    const auto o_scene   = m_offs.resolve("C_BaseEntity::m_pGameSceneNode", &ok_scene);
    const auto o_origin  = m_offs.resolve("CGameSceneNode::m_vecAbsOrigin", &ok_origin);
    const auto o_life    = m_offs.resolve("C_BaseEntity::m_lifeState",      &ok_life);
    const auto o_dormant = m_offs.resolve("CGameSceneNode::m_bDormant");
    const auto o_armor   = m_offs.resolve("C_CSPlayerPawnBase::m_ArmorValue");
    const auto o_vel     = m_offs.resolve("C_CSPlayerPawnBase::m_vecVelocity");

    if (!ok_hp) return false;
    const int hp = m_proc.read<int>(out.pawn_address + o_hp).value_or(-1);
    if (hp <= 0 || hp > 100) { g_dbg_rej_hp++; return false; }

    if (ok_life) {
        const auto life = m_proc.read<std::uint8_t>(out.pawn_address + o_life).value_or(255);
        if (life != 0) { g_dbg_rej_life++; return false; }
    }

    if (!ok_team) return false;
    const auto team = m_proc.read<std::uint8_t>(out.pawn_address + o_team).value_or(0);
    if (team != 2 && team != 3) { g_dbg_rej_team++; return false; }

    if (!ok_scene) return false;
    const auto scene = m_proc.read<std::uintptr_t>(out.pawn_address + o_scene).value_or(0);
    if (!scene) { g_dbg_rej_scene++; return false; }

    if (o_dormant) {
        const bool dormant = m_proc.read<std::uint8_t>(scene + o_dormant).value_or(1) != 0;
        if (dormant) { g_dbg_rej_dormant++; return false; }
    }

    if (!ok_origin) return false;
    const auto origin = m_proc.read<math::Vec3>(scene + o_origin);
    if (!origin) { g_dbg_rej_origin++; return false; }
    if (std::abs(origin->x) > 16384.f ||
        std::abs(origin->y) > 16384.f ||
        std::abs(origin->z) > 16384.f) { g_dbg_rej_origin++; return false; }

    out.team    = (Team)team;
    out.health  = hp;
    out.alive   = true;
    out.origin  = *origin;
    out.eye_pos = out.origin;
    out.eye_pos.z += 64.0f;

    if (o_armor) out.armor = m_proc.read<int>(out.pawn_address + o_armor).value_or(0);
    if (o_vel) {
        auto v = m_proc.read<math::Vec3>(out.pawn_address + o_vel);
        if (v) out.vel = *v;
    }
    return true;
}

bool EntityReader::read_bones(const Entity& in, Entity& out) {
    bool ok_scene = false;
    const auto o_scene         = m_offs.resolve("C_BaseEntity::m_pGameSceneNode", &ok_scene);
    const auto o_model         = m_offs.resolve("CGameSceneNode::m_modelState");
    const auto o_skel          = m_offs.resolve("CGameSceneNode::m_pSkeletonInstance");
    const auto o_mstate_in_sk  = m_offs.resolve("CSkeletonInstance::m_modelState");
    const auto o_node_to_world = m_offs.resolve("CGameSceneNode::m_nodeToWorld");
    if (!ok_scene) return false;

    const auto scene = m_proc.read<std::uintptr_t>(in.pawn_address + o_scene).value_or(0);
    if (!scene) return false;

    std::uintptr_t bone_array = 0;
    if (o_model) {
        bone_array = m_proc.read<std::uintptr_t>(scene + o_model + 0x80).value_or(0);
    }
    if (!bone_array && o_skel) {
        auto skel = m_proc.read<std::uintptr_t>(scene + o_skel).value_or(0);
        if (skel) {
            const auto mstate = (o_mstate_in_sk ? skel + o_mstate_in_sk : skel);
            bone_array = m_proc.read<std::uintptr_t>(mstate + 0x80).value_or(0);
        }
    }
    if (!bone_array) return false;

    float ntw[12]{};
    bool have_ntw = false;
    if (o_node_to_world) {
        have_ntw = m_proc.read_raw(scene + o_node_to_world, ntw, sizeof(ntw));
    }
    auto xform = [&](const math::Vec3& v) -> math::Vec3 {
        return {
            ntw[0]*v.x + ntw[1]*v.y + ntw[2]*v.z  + ntw[3],
            ntw[4]*v.x + ntw[5]*v.y + ntw[6]*v.z  + ntw[7],
            ntw[8]*v.x + ntw[9]*v.y + ntw[10]*v.z + ntw[11],
        };
    };

    constexpr std::size_t kBoneStride = 32;
    const char* names[] = { "bone.head", "bone.neck", "bone.chest", "bone.pelvis" };

    // DEBUG: dump raw first-player first-tick to prove layout
    const bool do_dump = (in.index != 0) && (m_tick_count.load() <= 5);

    for (std::size_t b = 0; b < (std::size_t)Bone::Count; ++b) {
        bool ok = false;
        const auto idx = m_offs.resolve(names[b], &ok);
        BoneTransform bt;
        if (ok) {
            auto p = m_proc.read<math::Vec3>(bone_array + idx * kBoneStride);
            if (p) {
                math::Vec3 pos = *p;

                if (do_dump && b == 0) {
                    core::Log::info(
                        "[VANTA-BONE] ba=0x%llx idx=%llu raw=(%.1f %.1f %.1f) origin=(%.1f %.1f %.1f) ntw=%d",
                        (unsigned long long)bone_array,
                        (unsigned long long)idx,
                        pos.x, pos.y, pos.z,
                        in.origin.x, in.origin.y, in.origin.z,
                        have_ntw ? 1 : 0);
                }

                const float dx = pos.x - in.origin.x;
                const float dy = pos.y - in.origin.y;
                const float dz = pos.z - in.origin.z;
                const bool looks_local = (std::abs(dx) > 100.f ||
                                          std::abs(dy) > 100.f ||
                                          std::abs(dz) > 150.f);
                if (looks_local && have_ntw) {
                    pos = xform(pos);
                }

                const bool finite_ok = std::isfinite(pos.x) &&
                                       std::isfinite(pos.y) &&
                                       std::isfinite(pos.z);
                const bool range_ok  = std::abs(pos.x) < 16384.f &&
                                       std::abs(pos.y) < 16384.f &&
                                       std::abs(pos.z) < 16384.f;
                if (finite_ok && range_ok) {
                    bt.pos   = pos;
                    bt.valid = true;
                }
            }
        }
        out.bones[b] = bt;
    }
    return true;
}

} // namespace vanta::game