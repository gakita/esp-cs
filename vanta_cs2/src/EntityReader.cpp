#include <algorithm>
#include "game/EntityReader.hpp"

#include "core/Log.hpp"

#include <chrono>

// --- Debug counters (visible from App.cpp via extern) ---------------------
std::uint32_t g_dbg_scanned  = 0;
std::uint32_t g_dbg_accepted = 0;
std::uintptr_t g_dbg_system  = 0;
std::uintptr_t g_dbg_page0   = 0;
std::uintptr_t g_dbg_ent0    = 0;

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
        snap->tick_index = m_tick_count.fetch_add(1) + 1;
        snap->captured_at_ns = (std::uint64_t)
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                t0.time_since_epoch()).count();
        snap->game_focused = m_focused.load();
        snap->attached = m_proc.is_attached() && m_proc.is_process_alive();

        if (snap->attached) {
            do_one_tick(snap);
        }
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

    // Local player pawn (direct global deref).
    if (a_localpawn_ptr) {
        auto pawn = m_proc.read<std::uintptr_t>(a_localpawn_ptr).value_or(0);
        if (pawn) {
            Entity local;
            local.pawn_address = pawn;
            local.index = 0;
            local.is_local = true;
            if (read_entity(a_entitylist, 0, local)) {
                read_bones(local, local);
                snap->local = local;
            }
        }
    }

    // Entity list walking. CS2 CGameEntitySystem lives via DOUBLE-DEREF:
    //   client.dll + dwEntityList -> *(g_pGameEntitySystem)
    // Inside, chunk pointers at +0x10 + chunk*8. Each chunk is an array of
    // CEntityIdentity (size 120). CEntityIdentity::m_pInstance is the first
    // field (offset 0), giving the CEntityInstance pointer.
    g_dbg_scanned  = 0;
    g_dbg_accepted = 0;
    if (a_entitylist) {
        auto system = m_proc.read<std::uintptr_t>(a_entitylist).value_or(0);
        g_dbg_system = system;
        if (system) {
            constexpr std::size_t kPages    = 64;
            constexpr std::size_t kPerPage  = 64;
            constexpr std::size_t kIdentSz  = 120;
            constexpr std::size_t kIdentOfs = 0;

            for (std::size_t page = 0; page < kPages; ++page) {
                auto page_ptr = m_proc.read<std::uintptr_t>(system + 0x10 + page * 8).value_or(0);
                if (page == 0) g_dbg_page0 = page_ptr;
                if (!page_ptr) continue;

                for (std::size_t i = 0; i < kPerPage; ++i) {
                    auto pawn = m_proc.read<std::uintptr_t>(
                        page_ptr + i * kIdentSz + kIdentOfs).value_or(0);
                    if (page == 0 && i == 0) g_dbg_ent0 = pawn;
                    if (!pawn) continue;

                    g_dbg_scanned++;
                    const auto global_idx = (std::uint32_t)(page * kPerPage + i);
                    if (snap->local && pawn == snap->local->pawn_address) continue;

                    Entity e;
                    e.pawn_address = pawn;
                    e.index = global_idx;
                    if (read_entity(a_entitylist, global_idx, e)) {
                        g_dbg_accepted++;
                        read_bones(e, e);
                        snap->players.push_back(std::move(e));
                    }
                }
            }
        }
    }

    // Bomb state.
    if (auto bomb_ptr = resolve_mod("dwPlantedC4")) {
        auto bomb = m_proc.read<std::uintptr_t>(bomb_ptr).value_or(0);
        if (bomb) {
            BombState b;
            bool ok1=false, ok2=false, ok3=false, ok4=false;
            auto o_tick    = m_offs.resolve("C_PlantedC4::m_bBombTicking", &ok1);
            auto o_blow    = m_offs.resolve("C_PlantedC4::m_flC4Blow", &ok2);
            auto o_defcd   = m_offs.resolve("C_PlantedC4::m_flDefuseCountDown", &ok3);
            auto o_defused = m_offs.resolve("C_PlantedC4::m_bBombDefused", &ok4);
            if (ok1 && ok2) {
                const auto ticking = m_proc.read<bool>(bomb + o_tick).value_or(false);
                const auto blow    = m_proc.read<float>(bomb + o_blow).value_or(0.0f);
                const auto now_s   = snap->captured_at_ns / 1e9f;
                b.planted = ticking;
                b.time_until_detonate = std::max(0.0f, blow - now_s);
            }
            if (ok3) {
                const auto def = m_proc.read<float>(bomb + o_defcd).value_or(0.0f);
                b.defusing = def > 0.0f;
                b.time_until_defused = std::max(0.0f, def);
            }
            if (ok4) {
                if (m_proc.read<bool>(bomb + o_defused).value_or(false)) {
                    b.planted = false;
                }
            }
            snap->bomb = b;
        }
    }

    m_total_entities.fetch_add((std::uint64_t)snap->players.size() + (snap->local ? 1 : 0));
}

bool EntityReader::read_view_projection(std::uintptr_t base, math::Mat4& out) {
    float raw[16]{};
    if (!m_proc.read_raw(base, raw, sizeof(raw))) return false;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            out(r, c) = raw[r * 4 + c];
    return true;
}

bool EntityReader::read_entity(std::uintptr_t /*list_base*/, std::uint32_t index, Entity& out) {
    out.index = index;
    if (!out.pawn_address) return false;

    bool ok_team=false, ok_hp=false, ok_scene=false, ok_origin=false;
    const auto o_team   = m_offs.resolve("C_BaseEntity::m_iTeamNum", &ok_team);
    const auto o_hp     = m_offs.resolve("C_BaseEntity::m_iHealth",  &ok_hp);
    const auto o_scene  = m_offs.resolve("C_BaseEntity::m_pGameSceneNode", &ok_scene);
    const auto o_origin = m_offs.resolve("CGameSceneNode::m_vecAbsOrigin", &ok_origin);
    const auto o_armor  = m_offs.resolve("C_CSPlayerPawnBase::m_ArmorValue");
    const auto o_vel    = m_offs.resolve("C_CSPlayerPawnBase::m_vecVelocity");

    if (ok_team) out.team = (Team)m_proc.read<std::uint8_t>(out.pawn_address + o_team).value_or(0);
    if (ok_hp)   out.health = m_proc.read<int>(out.pawn_address + o_hp).value_or(0);
    out.alive  = out.health > 0;
    if (o_armor) out.armor = m_proc.read<int>(out.pawn_address + o_armor).value_or(0);
    if (o_vel) {
        auto v = m_proc.read<math::Vec3>(out.pawn_address + o_vel);
        if (v) out.vel = *v;
    }

    if (ok_scene && ok_origin) {
        auto scene = m_proc.read<std::uintptr_t>(out.pawn_address + o_scene).value_or(0);
        if (scene) {
            auto o = m_proc.read<math::Vec3>(scene + o_origin);
            if (o) out.origin = *o;
        }
    }
    out.eye_pos = out.origin;
    out.eye_pos.z += 64.0f;

    // Accept any entry that has a plausible team (2 = T, 3 = CT) OR has
    // positive health. This is deliberately loose so world/weapon entities
    // get filtered without rejecting edge cases of player pawns.
    const bool team_ok   = (out.team == Team::Terrorist || out.team == Team::CounterTerr);
    const bool health_ok = (out.health > 0 && out.health <= 200);
    if (!team_ok && !health_ok) return false;
    return true;
}

bool EntityReader::read_bones(const Entity& in, Entity& out) {
    bool ok_scene=false;
    const auto o_scene = m_offs.resolve("C_BaseEntity::m_pGameSceneNode", &ok_scene);
    const auto o_model = m_offs.resolve("CGameSceneNode::m_modelState");
    if (!ok_scene || !o_model) return false;

    auto scene = m_proc.read<std::uintptr_t>(in.pawn_address + o_scene).value_or(0);
    if (!scene) return false;

    const auto model_state = scene + o_model;
    auto bone_array = m_proc.read<std::uintptr_t>(model_state + 0x80).value_or(0);
    if (!bone_array) return false;

    constexpr std::size_t kBoneStride = 32;
    for (std::size_t b = 0; b < (std::size_t)Bone::Count; ++b) {
        const char* names[] = { "bone.head", "bone.neck", "bone.chest", "bone.pelvis" };
        bool ok=false;
        const auto idx = m_offs.resolve(names[b], &ok);
        BoneTransform bt;
        if (ok) {
            auto p = m_proc.read<math::Vec3>(bone_array + idx * kBoneStride);
            if (p) { bt.pos = *p; bt.valid = true; }
        }
        out.bones[b] = bt;
    }
    return true;
}

} // namespace vanta::game
