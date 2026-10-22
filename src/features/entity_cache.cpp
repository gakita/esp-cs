// src/features/entity_cache.cpp
#include "entity_cache.hpp"
#include "sdk/offsets.hpp"
#include "sdk/schema.hpp"
#include <Windows.h>
#include <chrono>
#include <cmath>

namespace features {
using namespace sdk;
using namespace std::chrono_literals;

void EntityCache::start(mem::Process& proc) {
    if (running_.exchange(true)) return;
    thread_ = std::jthread([this, p = &proc] { loop(p); });
}
void EntityCache::stop() {
    running_.store(false);
    if (thread_.joinable()) thread_.join();
}
const Snapshot& EntityCache::snapshot() const {
    std::lock_guard lk(mtx_);
    return buf_[front_.load()];
}

static std::uintptr_t resolve_entity(const mem::Process& p, std::uintptr_t ent_list, std::uint32_t handle) {
    const std::uint32_t idx = handle & 0x7FFF;
    const std::uintptr_t chunk_ptr = p.read<std::uintptr_t>(
        ent_list + schema::EntityList::entries_offset + 8 * (idx >> 9));
    if (!chunk_ptr) return 0;
    return p.read<std::uintptr_t>(chunk_ptr + schema::EntityList::chunk_slot_size * (idx & 0x1FF));
}

static bool read_node_to_world(const mem::Process& p, std::uintptr_t scene, float out[12]) {
    return p.read_raw(scene + schema::GameSceneNode::m_nodeToWorld, out, sizeof(float)*12);
}

static Vec3 xform(const float m[12], const Vec3& v) {
    return {
        m[0]*v.x + m[1]*v.y + m[2]*v.z  + m[3],
        m[4]*v.x + m[5]*v.y + m[6]*v.z  + m[7],
        m[8]*v.x + m[9]*v.y + m[10]*v.z + m[11],
    };
}

static Vec3 read_bone(const mem::Process& p, std::uintptr_t scene, int idx) {
    const auto skel = p.read<std::uintptr_t>(scene + schema::GameSceneNode::m_pSkeletonInst);
    if (!skel) return {};
    const auto bones = p.read<std::uintptr_t>(
        skel + schema::SkeletonInstance::m_modelState + schema::ModelState::m_pBoneArray);
    if (!bones) return {};
    return p.read<Vec3>(bones + idx * schema::ModelState::kBoneStride);
}

void EntityCache::poll_once(mem::Process& p, std::uintptr_t client, std::uintptr_t engine, Snapshot& out) {
    out = {};

    // reset per-tick counters
    stats_.scanned.store(0);
    stats_.accepted.store(0);
    stats_.rej_handle.store(0);
    stats_.rej_hp.store(0);
    stats_.rej_life.store(0);
    stats_.rej_team.store(0);
    stats_.rej_scene.store(0);
    stats_.rej_dorm.store(0);
    stats_.rej_origin.store(0);

    // auto-detect resolution from CS2 client rect
    RECT cr{};
    HWND game_hwnd = p.window();
    if (game_hwnd && GetClientRect(game_hwnd, &cr)) {
        out.screen_w = cr.right  - cr.left;
        out.screen_h = cr.bottom - cr.top;
    } else {
        out.screen_w = p.read<int>(engine + off::engine::dwWindowWidth);
        out.screen_h = p.read<int>(engine + off::engine::dwWindowHeight);
    }
    if (out.screen_w < 320) out.screen_w = 1920;
    if (out.screen_h < 240) out.screen_h = 1080;

    p.read_raw(client + off::client::dwViewMatrix, &out.view, sizeof(out.view));

    const std::uintptr_t local_pawn = p.read<std::uintptr_t>(client + off::client::dwLocalPlayerPawn);
    if (!local_pawn) return;

    const auto local_scene = p.read<std::uintptr_t>(local_pawn + schema::BaseEntity::m_pGameSceneNode);
    if (local_scene) {
        out.local_eye = p.read<Vec3>(local_scene + schema::GameSceneNode::m_vecAbsOrigin);
        out.local_eye.z += 64.f;
    }
    out.local_team = p.read<std::uint8_t>(local_pawn + schema::BaseEntity::m_iTeamNum);
    p.read_raw(client + off::client::dwViewAngles, &out.local_view_angles, sizeof(out.local_view_angles));

    const std::uintptr_t ent_list = p.read<std::uintptr_t>(client + off::client::dwEntityList);
    if (!ent_list) return;

    const bool ignore_dorm   = cfg_ignore_dormant.load();
    const bool ignore_life   = cfg_ignore_lifestate.load();
    const bool relax_hp      = cfg_relax_hp_range.load();
    const int  hp_max        = relax_hp ? 500 : 100;

    for (std::uint32_t i = 1; i <= 64; ++i) {
        auto& slot = out.players[i - 1];
        slot.valid = false;

        const std::uintptr_t ctrl = resolve_entity(p, ent_list, i);
        if (!ctrl) continue;

        const auto pawn_handle = p.read<std::uint32_t>(
            ctrl + schema::CSPlayerController::m_hPlayerPawn);
        if (pawn_handle == 0xFFFFFFFF || (pawn_handle & 0x7FFF) == 0) {
            stats_.rej_handle.fetch_add(1);
            continue;
        }

        const std::uintptr_t pawn = resolve_entity(p, ent_list, pawn_handle);
        if (!pawn || pawn == local_pawn) continue;

        stats_.scanned.fetch_add(1);

        // ----- HP -----
        const int hp = p.read<int>(pawn + schema::BaseEntity::m_iHealth);
        stats_.last_hp_value.store((std::uint32_t)hp);
        if (hp <= 0 || hp > hp_max) {
            stats_.rej_hp.fetch_add(1);
            continue;
        }

        // ----- lifeState -----
        const std::uint8_t life = p.read<std::uint8_t>(pawn + schema::BaseEntity::m_lifeState);
        stats_.last_life_value.store((std::uint32_t)life);
        if (!ignore_life && life != 0) {
            stats_.rej_life.fetch_add(1);
            continue;
        }

        // ----- team -----
        const std::uint8_t team = p.read<std::uint8_t>(pawn + schema::BaseEntity::m_iTeamNum);
        stats_.last_team_value.store((std::uint32_t)team);
        if (team != 2 && team != 3) {
            stats_.rej_team.fetch_add(1);
            continue;
        }

        // ----- scene node -----
        const std::uintptr_t scene =
            p.read<std::uintptr_t>(pawn + schema::BaseEntity::m_pGameSceneNode);
        if (!scene) {
            stats_.rej_scene.fetch_add(1);
            continue;
        }

        // ----- dormant -----
        const bool dormant = p.read<std::uint8_t>(scene + schema::GameSceneNode::m_bDormant) != 0;
        if (!ignore_dorm && dormant) {
            stats_.rej_dorm.fetch_add(1);
            continue;
        }

        // ----- origin -----
        const Vec3 origin = p.read<Vec3>(scene + schema::GameSceneNode::m_vecAbsOrigin);
        if (std::abs(origin.x) > 16384.f || std::abs(origin.y) > 16384.f || std::abs(origin.z) > 16384.f) {
            stats_.rej_origin.fetch_add(1);
            continue;
        }

        // ----- hydrate -----
        slot.origin  = origin;
        slot.head    = { origin.x, origin.y, origin.z + 64.f };
        slot.health  = hp;
        slot.team    = team;
        slot.alive   = true;
        slot.dormant = dormant;
        slot.armor   = p.read<int>(pawn + schema::CSPlayerPawn::m_ArmorValue);
        p.read_raw(pawn + schema::CSPlayerPawn::m_angEyeAngles, &slot.eye_angles, sizeof(slot.eye_angles));
        slot.controller_idx = i;

        // ----- bones (world-space with fallback) -----
        slot.bone_head   = read_bone(p, scene, schema::kBoneHead);
        slot.bone_chest  = read_bone(p, scene, schema::kBoneChest);
        slot.bone_pelvis = read_bone(p, scene, schema::kBonePelvis);

        const float dxy = Vec3{slot.bone_head.x - origin.x, slot.bone_head.y - origin.y, 0}.length_2d();
        const float dz  = std::abs(slot.bone_head.z - origin.z);
        slot.bones_valid = (dxy < 100.f && dz < 150.f && dxy > 0.0001f);

        if (!slot.bones_valid) {
            float m[12]{};
            if (read_node_to_world(p, scene, m)) {
                slot.bone_head   = xform(m, slot.bone_head);
                slot.bone_chest  = xform(m, slot.bone_chest);
                slot.bone_pelvis = xform(m, slot.bone_pelvis);
                const float dxy2 = Vec3{slot.bone_head.x - origin.x, slot.bone_head.y - origin.y, 0}.length_2d();
                const float dz2  = std::abs(slot.bone_head.z - origin.z);
                slot.bones_valid = (dxy2 < 100.f && dz2 < 150.f);
            }
        }
        if (!slot.bones_valid) {
            slot.bone_head   = { origin.x, origin.y, origin.z + 64.f };
            slot.bone_chest  = { origin.x, origin.y, origin.z + 48.f };
            slot.bone_pelvis = { origin.x, origin.y, origin.z + 32.f };
        }

        slot.valid = true;
        stats_.accepted.fetch_add(1);
    }

    out.tick++;
}

void EntityCache::loop(mem::Process* proc) {
    while (running_.load()) {
        const auto t0 = std::chrono::steady_clock::now();
        const auto client = proc->module_base(L"client.dll");
        const auto engine = proc->module_base(L"engine2.dll");
        if (!client || !engine) { std::this_thread::sleep_for(100ms); continue; }
        Snapshot& back = buf_[1 - front_.load()];
        poll_once(*proc, client, engine, back);
        { std::lock_guard lk(mtx_); front_.store(1 - front_.load()); }
        std::this_thread::sleep_until(t0 + 1ms);
    }
}

}