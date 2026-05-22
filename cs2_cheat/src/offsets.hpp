#pragma once
#include <cstdint>

// =============================================================================
// CS2 offsets — synced from a2x/cs2-dumper
// Last sync: 2026-05-19 (dump timestamp 2026-05-19 09:08 UTC)
//
// Update procedure after each CS2 patch:
//   1. Pull latest output/offsets.hpp and output/client_dll.hpp from
//      https://github.com/a2x/cs2-dumper
//   2. Replace numeric values below with new ones (names rarely change)
//   3. Re-build cs2_external.exe
// =============================================================================

namespace off {

namespace client {
    // From offsets.hpp -> namespace client_dll
    constexpr uintptr_t dwLocalPlayerPawn       = 0x2090880;
    constexpr uintptr_t dwEntityList            = 0x250C5B0;
    constexpr uintptr_t dwViewMatrix            = 0x236C2F0;
    constexpr uintptr_t dwLocalPlayerController = 0x2345D50;
    constexpr uintptr_t dwGameRules             = 0x2366788;
    constexpr uintptr_t dwGlobalVars            = 0x2085788;
}

// From client_dll.hpp -> namespace C_BaseEntity
namespace base_entity {
    constexpr uintptr_t m_iHealth        = 0x34C;   // int32
    constexpr uintptr_t m_iTeamNum       = 0x3EB;   // uint8 (!)
    constexpr uintptr_t m_lifeState      = 0x354;   // uint8 (!)
    constexpr uintptr_t m_pGameSceneNode = 0x330;   // CGameSceneNode*
}

// From client_dll.hpp -> namespace C_CSPlayerPawn
namespace pawn {
    constexpr uintptr_t m_iIDEntIndex  = 0x33FC;    // CEntityIndex (int)
    constexpr uintptr_t m_angEyeAngles = 0x3320;    // QAngle (3 floats)
    constexpr uintptr_t m_ArmorValue   = 0x1C7C;    // int32
    constexpr uintptr_t m_iShotsFired  = 0x1C64;    // int32
}

// From client_dll.hpp -> namespace CGameSceneNode
namespace scene_node {
    constexpr uintptr_t m_vecAbsOrigin = 0xC8;      // VectorWS
}

// From client_dll.hpp -> namespace CCSPlayerController
namespace controller {
    constexpr uintptr_t m_sSanitizedPlayerName = 0x860;  // CUtlString
    constexpr uintptr_t m_hPlayerPawn          = 0x90C;  // CHandle<C_CSPlayerPawn>
    constexpr uintptr_t m_iPing                = 0x828;  // uint32
}

// Source 2 entity list shape — двухуровневый
constexpr int ENTITY_LIST_STRIDE = 0x78;    // sizeof entry = 120
constexpr int ENTITY_LIST_PAGE   = 0x200;   // 512 entries per page

// Team constants
constexpr int TEAM_SPECTATOR = 1;
constexpr int TEAM_T         = 2;
constexpr int TEAM_CT        = 3;

} // namespace off
