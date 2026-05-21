#include "player.hpp"
#include "../offsets.hpp"

#include <vector>

namespace cs2::sdk {

namespace o = cs2::offsets;

// Entity list в Source 2 — двухуровневая таблица:
// dwEntityList -> CEntitySystem
//   +0x10  -> m_EntityIdentities (CEntityIdentity*)
//   индекс берётся как: list[(idx >> 9) & 0x7F] -> chunk[idx & 0x1FF]
// чанк хранит 64 записи по 0x78 байт, поле +0 -> CEntityInstance*.
// Это эмпирическая структура из публичных дампов, может уточняться.
static constexpr std::uintptr_t ENT_CHUNK_SHIFT = 9;
static constexpr std::uintptr_t ENT_CHUNK_MASK  = 0x7F;
static constexpr std::uintptr_t ENT_SLOT_MASK   = 0x1FF;
static constexpr std::uintptr_t ENT_CHUNK_PTR_OFF = 0x10;
static constexpr std::uintptr_t ENT_SLOT_SIZE   = 0x78;

Snapshot::Snapshot(const Process& proc, std::uintptr_t client_base)
    : proc_(proc), client_(client_base) {}

static bool fill_player(const Process& p, std::uintptr_t pawn,
                        PlayerSnapshot& out) {
    if (!pawn) return false;
    out.pawn_addr = pawn;

    auto hp = p.read<int>(pawn + o::s.m_iHealth);
    if (!hp) return false;
    out.health = *hp;

    auto team = p.read<int>(pawn + o::s.m_iTeamNum);
    if (team) out.team = *team;

    auto life = p.read<std::uint8_t>(pawn + o::s.m_lifeState);
    if (life) out.life_state = *life;

    // origin живёт в CGameSceneNode->m_vecAbsOrigin
    auto scene_ptr = p.read<std::uintptr_t>(pawn + o::s.m_pGameSceneNode);
    if (!scene_ptr || !*scene_ptr) return false;

    auto origin = p.read<Vec3>(*scene_ptr + o::s.m_vecAbsOrigin);
    if (!origin) return false;
    out.origin = *origin;

    return true;
}

bool Snapshot::read_local(PlayerSnapshot& out) const {
    auto pawn = proc_.read<std::uintptr_t>(client_ + o::g.dwLocalPlayerPawn);
    if (!pawn || !*pawn) return false;
    return fill_player(proc_, *pawn, out);
}

std::vector<PlayerSnapshot> Snapshot::read_all() const {
    std::vector<PlayerSnapshot> result;

    auto ent_sys = proc_.read<std::uintptr_t>(client_ + o::g.dwEntityList);
    if (!ent_sys || !*ent_sys) return result;

    auto identities = proc_.read<std::uintptr_t>(*ent_sys + ENT_CHUNK_PTR_OFF);
    if (!identities || !*identities) return result;

    for (int i = 1; i <= 64; ++i) {
        auto chunk = proc_.read<std::uintptr_t>(
            *identities + 8 * ((i >> ENT_CHUNK_SHIFT) & ENT_CHUNK_MASK));
        if (!chunk || !*chunk) continue;

        auto ent = proc_.read<std::uintptr_t>(
            *chunk + (i & ENT_SLOT_MASK) * ENT_SLOT_SIZE);
        if (!ent || !*ent) continue;

        PlayerSnapshot snap;
        if (fill_player(proc_, *ent, snap) && snap.health > 0) {
            result.push_back(std::move(snap));
        }
    }
    return result;
}

} // namespace cs2::sdk
