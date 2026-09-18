#pragma once
#include "Memory.hpp"
#include "Offsets.hpp"
#include "Types.hpp"
#include <vector>
#include <string>

// ---- nombre de una instancia ----
inline std::string GetInstanceName(uintptr_t inst) {
    if (!inst) return {};
    uintptr_t nc = mem.Read<uintptr_t>(inst + Offsets::Instance::NameContainer);
    if (!nc) return {};
    std::string s = mem.ReadRbxString(nc + Offsets::Instance::Name);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\0')) s.pop_back();
    return s;
}

// ---- hijos de una instancia (doble indirección) ----
inline std::vector<uintptr_t> GetChildren(uintptr_t inst) {
    std::vector<uintptr_t> out;
    if (!inst) return out;
    uintptr_t container = mem.Read<uintptr_t>(inst + Offsets::Instance::ChildrenStart);
    if (!container) return out;
    uintptr_t ls = mem.Read<uintptr_t>(container);
    uintptr_t le = mem.Read<uintptr_t>(container + 8);
    if (!ls || !le || le <= ls || le - ls > 0x10000) return out;
    for (uintptr_t p = ls; p < le; p += 16) {
        uintptr_t child = mem.Read<uintptr_t>(p);
        if (child && child > 0x10000000000ULL) out.push_back(child);
    }
    return out;
}

// ---- encontrar hijo por nombre ----
inline uintptr_t FindFirstChild(uintptr_t inst, const char* name) {
    for (uintptr_t child : GetChildren(inst))
        if (GetInstanceName(child) == name) return child;
    return 0;
}

// ---- DataModel (cadena directa con FakeDataModel::Pointer) ----
inline uintptr_t GetDataModel() {
    uintptr_t fdm = mem.Read<uintptr_t>(mem.base + Offsets::FakeDataModel::Pointer);
    if (!fdm) return 0;
    return mem.Read<uintptr_t>(fdm + Offsets::FakeDataModel::RealDataModel);
}

// ---- ViewMatrix del VisualEngine ----
inline ViewMatrix_t GetViewMatrix() {
    ViewMatrix_t vm{};
    uintptr_t ve = mem.Read<uintptr_t>(mem.base + Offsets::VisualEngine::Pointer);
    if (!ve) return vm;
    mem.Read<ViewMatrix_t>(ve + Offsets::VisualEngine::ViewMatrix);
    ReadProcessMemory(mem.proc, (LPCVOID)(ve + Offsets::VisualEngine::ViewMatrix),
        &vm, sizeof(vm), nullptr);
    return vm;
}

// ---- posición 3D de un BasePart ----
inline Vector3 GetPartPosition(uintptr_t part) {
    uintptr_t prim = mem.Read<uintptr_t>(part + Offsets::BasePart::Primitive);
    if (!prim) return {};
    return mem.Read<Vector3>(prim + Offsets::Primitive::Position);
}

// ---- tamaño de un BasePart ----
inline Vector3 GetPartSize(uintptr_t part) {
    uintptr_t prim = mem.Read<uintptr_t>(part + Offsets::BasePart::Primitive);
    if (!prim) return {};
    return mem.Read<Vector3>(prim + Offsets::Primitive::Size);
}

// ---- Players service (por nombre o por LocalPlayer en +0x130) ----
inline uintptr_t GetPlayersService(uintptr_t dm) {
    uintptr_t byName = FindFirstChild(dm, "Players");
    if (byName) return byName;
    // fallback: buscar el child que tiene un LocalPlayer válido en +0x130
    for (uintptr_t child : GetChildren(dm)) {
        uintptr_t lp = mem.Read<uintptr_t>(child + Offsets::Players::LocalPlayer);
        if (lp < 0x10000000000ULL) continue;
        uintptr_t nc = mem.Read<uintptr_t>(lp + Offsets::Instance::NameContainer);
        if (!nc) continue;
        std::string nm = mem.ReadRbxString(nc + Offsets::Instance::Name);
        if (nm.size() >= 3 && nm.size() <= 30) return child;
    }
    return 0;
}

// ---- color de equipo (BrickColor → COLORREF) ----
// Arsenal tiene 4 equipos; usamos el BrickColor number del Team
inline COLORREF BrickColorToRGB(uint32_t bc) {
    switch (bc) {
        case 21:  return RGB(196, 40,  28);   // Bright red
        case 23:  return RGB( 13,105,172);    // Bright blue
        case 37:  return RGB( 75,151, 75);    // Bright green
        case 24:  return RGB(245,205, 48);    // Bright yellow
        case 26:  return RGB(227,127, 26);    // Bright orange
        case 104: return RGB(107, 50,124);    // Bright violet / purple
        case 45:  return RGB(  0,  0,255);    // Blue
        case 1026:return RGB(  0,255,255);    // Cyan
        default:  return RGB(255, 0,  0);     // fallback rojo
    }
}

inline COLORREF GetTeamColor(uintptr_t player) {
    // método 1: leer TeamColor (BrickColor uint32) directo del Player
    uint32_t bc = mem.Read<uint32_t>(player + Offsets::Player::TeamColor);
    if (bc > 0 && bc < 1200) return BrickColorToRGB(bc);
    // método 2: ir al Team instance y leer su BrickColor
    uintptr_t team = mem.Read<uintptr_t>(player + Offsets::Player::Team);
    if (team && team > 0x10000000000ULL) {
        bc = mem.Read<uint32_t>(team + Offsets::Team::BrickColor);
        if (bc > 0 && bc < 1200) return BrickColorToRGB(bc);
    }
    return RGB(255, 0, 0);
}

// ---- local player ----
inline uintptr_t GetLocalPlayer(uintptr_t playersService) {
    if (!playersService) return 0;
    return mem.Read<uintptr_t>(playersService + Offsets::Players::LocalPlayer);
}

// ---- lista de todos los jugadores ----
inline std::vector<uintptr_t> GetAllPlayers(uintptr_t playersService) {
    return GetChildren(playersService);
}

struct PlayerInfo {
    uintptr_t inst = 0;
    uintptr_t character = 0;
    uintptr_t humanoid = 0;
    uintptr_t hrp = 0;
    std::string name;
    float health = 0.f;
    float maxHealth = 0.f;
    Vector3 pos{};
    bool valid = false;
};

inline PlayerInfo ReadPlayer(uintptr_t player) {
    PlayerInfo info;
    info.inst = player;
    if (!player) return info;

    info.name = GetInstanceName(player);
    if (info.name.empty()) return info;

    info.character = mem.Read<uintptr_t>(player + Offsets::Player::ModelInstance);
    if (!info.character || info.character < 0x10000000000ULL) {
        // Fallback: Buscar el modelo del personaje directamente en Workspace por el nombre del jugador
        uintptr_t dm = GetDataModel();
        if (dm) {
            uintptr_t ws = FindFirstChild(dm, "Workspace");
            if (ws) {
                uintptr_t charInWs = FindFirstChild(ws, info.name.c_str());
                if (charInWs && charInWs > 0x10000000000ULL) {
                    info.character = charInWs;
                }
            }
        }
    }
    if (!info.character) return info;

    // intentar via Humanoid primero
    info.humanoid = FindFirstChild(info.character, "Humanoid");
    if (info.humanoid) {
        info.health    = mem.Read<float>(info.humanoid + Offsets::Humanoid::Health);
        info.maxHealth = mem.Read<float>(info.humanoid + Offsets::Humanoid::MaxHealth);
        info.hrp = mem.Read<uintptr_t>(info.humanoid + Offsets::Humanoid::HumanoidRootPart);

        // Si la vida es <= 0.01f -> jugador muerto (cuerpo o ragdoll en el suelo), ignorar
        if (info.health <= 0.01f) {
            info.valid = false;
            return info;
        }
    }

    // fallback: buscar HRP directamente en el character
    if (!info.hrp) info.hrp = FindFirstChild(info.character, "HumanoidRootPart");

    // fallback final: usar PrimaryPart del Model (siempre apunta al HRP)
    if (!info.hrp) {
        uintptr_t pp = mem.Read<uintptr_t>(info.character + Offsets::Model::PrimaryPart);
        if (pp && pp > 0x10000000000ULL) info.hrp = pp;
    }

    // fallback adicional: buscar Head o Torso si no hay HRP
    if (!info.hrp) {
        uintptr_t head = FindFirstChild(info.character, "Head");
        if (head) info.hrp = head;
        else {
            uintptr_t torso = FindFirstChild(info.character, "Torso");
            if (!torso) torso = FindFirstChild(info.character, "UpperTorso");
            if (torso) info.hrp = torso;
        }
    }

    if (!info.hrp) return info;

    info.pos = GetPartPosition(info.hrp);

    // validar que la posición no sea origen ni basura
    float px = info.pos.x, py = info.pos.y, pz = info.pos.z;
    if (px*px + py*py + pz*pz < 1.f) return info;
    if (fabsf(px) > 1000000.f || fabsf(py) > 1000000.f || fabsf(pz) > 1000000.f) return info;

    info.valid = true;
    return info;
}
