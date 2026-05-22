#pragma once
#include <cstdint>

// =============================================================================
// CS2 offsets — обновляй после каждого игрового патча из:
//   https://github.com/a2x/cs2-dumper
//
// Конкретные значения ниже — пример, валидный на момент написания.
// При первом запуске СВЕРИ их со свежими файлами:
//   output/offsets.hpp        → блок client_dll
//   output/client.dll.hpp     → классы C_BaseEntity / C_BasePlayerPawn /
//                                C_CSPlayerPawn / CGameSceneNode
// =============================================================================

namespace off {

namespace client {
    // указатели в client.dll (offsets.hpp → namespace client_dll)
    constexpr uintptr_t dwLocalPlayerPawn = 0x182C0F8;
    constexpr uintptr_t dwEntityList      = 0x1A1C788;
    constexpr uintptr_t dwViewMatrix      = 0x1ADD500;
    constexpr uintptr_t dwLocalPlayerController = 0x1A2EFB8;
}

// schema поля внутри объектов
namespace base_entity {
    constexpr uintptr_t m_iHealth        = 0x344;
    constexpr uintptr_t m_iTeamNum       = 0x3E3;
    constexpr uintptr_t m_lifeState      = 0x340;
    constexpr uintptr_t m_pGameSceneNode = 0x328;
}

namespace pawn {
    constexpr uintptr_t m_iIDEntIndex = 0x1554;   // entity на которую смотрит
}

namespace scene_node {
    constexpr uintptr_t m_vecAbsOrigin = 0xC8;   // Vec3
};

namespace controller {
    constexpr uintptr_t m_sSanitizedPlayerName = 0x640;
};

// размер записи в EntityList Source 2 (двухуровневый массив)
constexpr int ENTITY_LIST_STRIDE = 120;
constexpr int ENTITY_LIST_PAGE   = 0x200;

} // namespace off
