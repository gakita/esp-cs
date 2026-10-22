#pragma once
#include <cstddef>

namespace sdk::schema {

namespace GameSceneNode {
    constexpr std::ptrdiff_t m_nodeToWorld     = 0x10;   // CTransformWS 3x4
    constexpr std::ptrdiff_t m_vecAbsOrigin    = 0xC8;
    constexpr std::ptrdiff_t m_angAbsRotation  = 0xD4;
    constexpr std::ptrdiff_t m_bDormant        = 0x103;
    constexpr std::ptrdiff_t m_pSkeletonInst   = 0x160;  // TODO verify
}

namespace SkeletonInstance {
    constexpr std::ptrdiff_t m_modelState    = 0x170;    // TODO verify
}
namespace ModelState {
    constexpr std::ptrdiff_t m_pBoneArray    = 0x80;     // TODO verify
    constexpr std::size_t    kBoneStride     = 32;       // Vec3+Quat+float
}

enum BoneIdx : int {
    kBonePelvis = 0,
    kBoneChest  = 4,
    kBoneNeck   = 5,
    kBoneHead   = 6,  // TODO verify per model
};

namespace BaseEntity {
    constexpr std::ptrdiff_t m_pGameSceneNode  = 0x338;  // TODO verify
    constexpr std::ptrdiff_t m_iTeamNum        = 0x3E3;  // TODO verify
    constexpr std::ptrdiff_t m_iHealth         = 0x344;  // TODO verify
    constexpr std::ptrdiff_t m_iMaxHealth      = 0x340;  // TODO verify
    constexpr std::ptrdiff_t m_lifeState       = 0x37C;  // TODO verify
    constexpr std::ptrdiff_t m_hOwnerEntity    = 0x3C0;  // TODO verify
}

namespace BasePlayerPawn {
    constexpr std::ptrdiff_t m_pWeaponServices    = 0xBC8;
    constexpr std::ptrdiff_t m_pItemServices      = 0xBD0;
    constexpr std::ptrdiff_t m_pMovementServices  = 0xBF0;
    constexpr std::ptrdiff_t m_pCameraServices    = 0xBF8;
    constexpr std::ptrdiff_t m_pObserverServices  = 0xC00;
}

namespace CSPlayerPawnBase {
    constexpr std::ptrdiff_t m_iPlayerState    = 0x13FC;
    constexpr std::ptrdiff_t m_flFlashDuration = 0x1428;
    constexpr std::ptrdiff_t m_hOriginalCtrl   = 0x1478;
}

namespace CSPlayerPawn {
    constexpr std::ptrdiff_t m_ArmorValue    = 0x1CA4;
    constexpr std::ptrdiff_t m_bIsScoped     = 0x1C78;
    constexpr std::ptrdiff_t m_bIsDefusing   = 0x1C7A;
    constexpr std::ptrdiff_t m_iShotsFired   = 0x1C8C;
    constexpr std::ptrdiff_t m_flFlinchStack = 0x1C90;
    constexpr std::ptrdiff_t m_flVelocityMod = 0x1C94;
    constexpr std::ptrdiff_t m_angEyeAngles  = 0x3350;
    constexpr std::ptrdiff_t m_iIDEntIndex   = 0x342C;
    constexpr std::ptrdiff_t m_entitySpotted = 0x1C60;
}

namespace CSPlayerController {
    constexpr std::ptrdiff_t m_hPlayerPawn    = 0x7E8;   // TODO verify
    constexpr std::ptrdiff_t m_sSanitizedName = 0x640;
    constexpr std::ptrdiff_t m_iPing          = 0x8A4;
    constexpr std::ptrdiff_t m_iPawnHealth    = 0x840;
    constexpr std::ptrdiff_t m_bPawnIsAlive   = 0x83C;
}

namespace WeaponServices {
    constexpr std::ptrdiff_t m_hActiveWeapon = 0xB0;
}

namespace CameraServices {
    constexpr std::ptrdiff_t m_iFOV       = 0x290;
    constexpr std::ptrdiff_t m_iFOVStart  = 0x294;
    constexpr std::ptrdiff_t m_flFOVTime  = 0x298;
    constexpr std::ptrdiff_t m_hZoomOwner = 0x2A0;
}

namespace EntityList {
    constexpr std::ptrdiff_t entries_offset  = 0x10;
    constexpr std::size_t    chunk_slot_size = 120;
}

}