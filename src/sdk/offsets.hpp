#pragma once
#include <cstddef>
#include <cstdint>

namespace sdk::off {
namespace client {
    constexpr std::ptrdiff_t dwCSGOInput              = 0x257B640;
    constexpr std::ptrdiff_t dwEntityList             = 0x271A908;
    constexpr std::ptrdiff_t dwGameEntitySystem       = 0x271A908;
    constexpr std::ptrdiff_t dwGameEntitySystem_hi    = 0x2120;
    constexpr std::ptrdiff_t dwGameRules              = 0x25651D0;
    constexpr std::ptrdiff_t dwGlobalVars             = 0x2230090;
    constexpr std::ptrdiff_t dwGlowManager            = 0x2561EF0;
    constexpr std::ptrdiff_t dwLocalPlayerController  = 0x253CB78;
    constexpr std::ptrdiff_t dwLocalPlayerPawn        = 0x2565718;
    constexpr std::ptrdiff_t dwPlantedC4              = 0x24CB3C0;
    constexpr std::ptrdiff_t dwPrediction             = 0x2565620;
    constexpr std::ptrdiff_t dwViewAngles             = 0x257BCC8;
    constexpr std::ptrdiff_t dwViewMatrix             = 0x256B020;
    constexpr std::ptrdiff_t dwViewRender             = 0x256B9E8;
    constexpr std::ptrdiff_t dwWeaponC4               = 0x24C9B30;
    constexpr std::ptrdiff_t dwSensitivity            = 0x23A2228; // TODO verify
    constexpr std::ptrdiff_t dwSensitivity_val        = 0x58;      // TODO verify
}
namespace engine {
    constexpr std::ptrdiff_t dwBuildNumber   = 0x61CFEC;
    constexpr std::ptrdiff_t dwWindowWidth   = 0x91F338;
    constexpr std::ptrdiff_t dwWindowHeight  = 0x91F33C;
}
}