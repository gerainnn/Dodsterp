#include "sdk.hpp"
#include "offsets.hpp"

bool World::init() {
    client_base_ = mem_.module_base(L"client.dll");
    return client_base_ != 0;
}

uintptr_t World::local_pawn() const {
    return mem_.read<uintptr_t>(client_base_ + off::client::dwLocalPlayerPawn);
}

Matrix4 World::view_matrix() const {
    Matrix4 m{};
    mem_.read_buf(client_base_ + off::client::dwViewMatrix, &m, sizeof(m));
    return m;
}

uintptr_t World::pawn_at_(int idx) const {
    // Source 2 two-level entity list:
    //   page = list_root[ 0x10 * (idx >> 9) + 0x10 ]
    //   ent  = page[ 0x78 * (idx & 0x1FF) ]
    uintptr_t list_root = mem_.read<uintptr_t>(client_base_ + off::client::dwEntityList);
    if (!list_root) return 0;

    uintptr_t page = mem_.read<uintptr_t>(
        list_root + 0x10 * (idx >> 9) + 0x10);
    if (!page) return 0;

    return mem_.read<uintptr_t>(
        page + static_cast<uintptr_t>(off::ENTITY_LIST_STRIDE) * (idx & 0x1FF));
}

std::vector<PawnSnapshot> World::enemies(int local_team, int max_ents) const {
    std::vector<PawnSnapshot> out;
    out.reserve(16);

    uintptr_t local = local_pawn();

    for (int i = 1; i < max_ents; ++i) {
        uintptr_t pawn = pawn_at_(i);
        if (!pawn || pawn == local) continue;

        int hp = mem_.read<int>(pawn + off::base_entity::m_iHealth);
        if (hp <= 0 || hp > 100) continue;

        // m_iTeamNum is uint8 in CS2 schema — reading as int would grab garbage
        int team = static_cast<int>(
            mem_.read<uint8_t>(pawn + off::base_entity::m_iTeamNum));
        if (team == local_team) continue;
        if (team != off::TEAM_T && team != off::TEAM_CT) continue;

        int life = static_cast<int>(
            mem_.read<uint8_t>(pawn + off::base_entity::m_lifeState));
        if (life != 0) continue;   // 0 = LIFE_ALIVE

        uintptr_t scene = mem_.read<uintptr_t>(pawn + off::base_entity::m_pGameSceneNode);
        if (!scene) continue;

        Vec3 origin{};
        mem_.read_buf(scene + off::scene_node::m_vecAbsOrigin, &origin, sizeof(Vec3));

        out.push_back(PawnSnapshot{ pawn, hp, team, life, origin });
    }
    return out;
}
