#pragma once

#include <cstdint>
#include <string>
#include "../process.hpp"
#include "../math.hpp"

namespace cs2::sdk {

struct PlayerSnapshot {
    std::uintptr_t pawn_addr   = 0;
    int            health      = 0;
    int            team        = 0;
    std::uint8_t   life_state  = 0;
    Vec3           origin{};
    std::string    name;
};

// Один кадр: считать локального игрока, всех живых pawn'ов из entity list.
// Возвращает то что удалось прочитать. Тихо игнорит дохлых/невалидных.
class Snapshot {
public:
    explicit Snapshot(const Process& proc, std::uintptr_t client_base);

    bool read_local(PlayerSnapshot& out) const;
    std::vector<PlayerSnapshot> read_all() const;

private:
    const Process& proc_;
    std::uintptr_t client_;
};

} // namespace cs2::sdk
