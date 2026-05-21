// Stage 1 sanity scan: подключаемся к cs2.exe, читаем base client.dll,
// печатаем local + всех живых pawn'ов раз в секунду.
//
// Запуск от админа. Сначала запусти CS2, зайди в Practice with bots.

#include <chrono>
#include <iostream>
#include <thread>

#include "process.hpp"
#include "offsets.hpp"
#include "sdk/player.hpp"

int wmain() {
    using namespace std::chrono_literals;

    cs2::Process proc;
    std::cout << "[*] waiting for cs2.exe...\n";
    while (!proc.attach(L"cs2.exe")) {
        std::this_thread::sleep_for(500ms);
    }
    std::cout << "[+] attached pid=" << proc.pid() << "\n";

    std::uintptr_t client = 0;
    while ((client = proc.module_base(L"client.dll")) == 0) {
        std::this_thread::sleep_for(500ms);
    }
    std::cout << "[+] client.dll @ 0x" << std::hex << client << std::dec << "\n";

    if (!cs2::offsets::load_from_files("data")) {
        std::cerr << "[-] offsets not loaded. drop offsets.json + "
                     "client_dll.json from a2x/cs2-dumper into ./data/\n";
        return 1;
    }
    std::cout << "[+] offsets ready\n";

    cs2::sdk::Snapshot snap(proc, client);

    while (true) {
        cs2::sdk::PlayerSnapshot me{};
        if (snap.read_local(me)) {
            std::cout << "\n[me] hp=" << me.health
                      << " team=" << me.team
                      << " pos=(" << me.origin.x << "," << me.origin.y
                      << "," << me.origin.z << ")\n";
        } else {
            std::cout << "\n[me] not alive / not in match\n";
        }

        auto all = snap.read_all();
        std::cout << "[ents] " << all.size() << " alive pawns\n";
        for (const auto& p : all) {
            std::cout << "  pawn=0x" << std::hex << p.pawn_addr << std::dec
                      << " hp=" << p.health
                      << " team=" << p.team
                      << " pos=(" << p.origin.x << "," << p.origin.y
                      << "," << p.origin.z << ")\n";
        }

        std::this_thread::sleep_for(1s);
    }
    return 0;
}
