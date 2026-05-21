#include "offsets.hpp"

#include <fstream>
#include <iostream>

#ifdef HAS_NLOHMANN_JSON
#include <nlohmann/json.hpp>
#endif

namespace cs2::offsets {

Globals g;
Schema  s;

#ifdef HAS_NLOHMANN_JSON
static std::uintptr_t pick(const nlohmann::json& j, const char* key) {
    if (!j.contains(key)) return 0;
    const auto& v = j[key];
    if (v.is_number_integer() || v.is_number_unsigned()) {
        return static_cast<std::uintptr_t>(v.get<std::uint64_t>());
    }
    return 0;
}
#endif

bool load_from_files(const std::string& dir) {
#ifdef HAS_NLOHMANN_JSON
    try {
        std::ifstream off_in(dir + "/offsets.json");
        std::ifstream cli_in(dir + "/client_dll.json");
        if (!off_in || !cli_in) return false;

        nlohmann::json offs, cli;
        off_in >> offs;
        cli_in >> cli;

        // a2x/cs2-dumper layout:
        //   offsets.json -> { "client.dll": { "dwEntityList": 0x..., ... } }
        //   client_dll.json -> { "client.dll": { "classes": { "C_BaseEntity":
        //                       { "fields": { "m_iHealth": 0x... } } } } }

        const auto& client = offs["client.dll"];
        g.dwEntityList      = pick(client, "dwEntityList");
        g.dwLocalPlayerPawn = pick(client, "dwLocalPlayerPawn");
        g.dwLocalPlayerCtrl = pick(client, "dwLocalPlayerController");
        g.dwViewMatrix      = pick(client, "dwViewMatrix");
        g.dwGameRules       = pick(client, "dwGameRules");
        g.dwGlobalVars      = pick(client, "dwGlobalVars");

        const auto& classes = cli["client.dll"]["classes"];

        auto field = [&](const char* cls, const char* fld) -> std::uintptr_t {
            if (!classes.contains(cls)) return 0;
            const auto& c = classes[cls];
            if (!c.contains("fields")) return 0;
            return pick(c["fields"], fld);
        };

        s.m_iHealth        = field("C_BaseEntity",        "m_iHealth");
        s.m_iTeamNum       = field("C_BaseEntity",        "m_iTeamNum");
        s.m_lifeState      = field("C_BaseEntity",        "m_lifeState");
        s.m_pGameSceneNode = field("C_BaseEntity",        "m_pGameSceneNode");
        s.m_vecAbsOrigin   = field("CGameSceneNode",      "m_vecAbsOrigin");
        s.m_iszPlayerName  = field("CBasePlayerController","m_iszPlayerName");
        s.m_hPawn          = field("CCSPlayerController", "m_hPawn");
        s.m_modelState     = field("CSkeletonInstance",   "m_modelState");

        return ready();
    } catch (const std::exception& e) {
        std::cerr << "[offsets] parse error: " << e.what() << "\n";
        return false;
    }
#else
    (void)dir;
    std::cerr << "[offsets] built without nlohmann/json — install via vcpkg\n";
    return false;
#endif
}

bool ready() {
    return g.dwEntityList && g.dwLocalPlayerPawn && g.dwViewMatrix
        && s.m_iHealth && s.m_pGameSceneNode && s.m_vecAbsOrigin;
}

} // namespace cs2::offsets
