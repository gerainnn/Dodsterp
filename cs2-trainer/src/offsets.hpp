#pragma once

#include <cstdint>
#include <string>

// Все оффсеты грузим из data/offsets.json + data/client_dll.json
// (формат a2x/cs2-dumper). При старте — load_from_files().
//
// Хардкод оффсетов здесь нужен только как fallback на случай отсутствия JSON.
// Любой апдейт CS2 ломает значения — обновлять дампер, не код.

namespace cs2::offsets {

struct Globals {
    // libclient.so / client.dll relative
    std::uintptr_t dwEntityList        = 0;
    std::uintptr_t dwLocalPlayerPawn   = 0;
    std::uintptr_t dwLocalPlayerCtrl   = 0;
    std::uintptr_t dwViewMatrix        = 0;
    std::uintptr_t dwGameRules         = 0;
    std::uintptr_t dwGlobalVars        = 0;
};

struct Schema {
    // C_BaseEntity / C_BasePlayerPawn / C_CSPlayerPawn
    std::uintptr_t m_iHealth           = 0;
    std::uintptr_t m_iTeamNum          = 0;
    std::uintptr_t m_lifeState         = 0;
    std::uintptr_t m_pGameSceneNode    = 0;
    std::uintptr_t m_vOldOrigin        = 0;
    std::uintptr_t m_iszPlayerName     = 0; // в Controller
    std::uintptr_t m_hPawn             = 0; // в Controller -> handle на Pawn

    // CSkeletonInstance / CGameSceneNode
    std::uintptr_t m_modelState        = 0;
    std::uintptr_t m_vecAbsOrigin      = 0;
};

extern Globals g;
extern Schema  s;

// true если все нужные оффсеты ненулевые
bool load_from_files(const std::string& dir);
bool ready();

} // namespace cs2::offsets
