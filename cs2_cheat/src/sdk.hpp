#pragma once
#include "memory.hpp"
#include "math.hpp"
#include <vector>
#include <string>

struct PawnSnapshot {
    uintptr_t addr;
    int       health;
    int       team;
    int       life_state;
    Vec3      origin;
};

class World {
public:
    explicit World(Memory& mem) : mem_(mem) {}

    // Закешировать базы модулей. Вызвать после mem.attach().
    bool init();

    bool ok() const { return client_base_ != 0; }

    uintptr_t local_pawn() const;
    Matrix4   view_matrix() const;

    // Снимок всех живых противников локального игрока.
    std::vector<PawnSnapshot> enemies(int local_team, int max_ents = 64) const;

private:
    Memory&   mem_;
    uintptr_t client_base_ = 0;

    uintptr_t pawn_at_(int idx) const;
};
