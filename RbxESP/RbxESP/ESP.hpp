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

// ---- buscar descendiente por nombre (recursivo, max depth 4) ----
inline uintptr_t FindFirstDescendant(uintptr_t inst, const char* name, int depth = 0) {
    if (depth > 4 || !inst) return 0;
    for (uintptr_t child : GetChildren(inst)) {
        if (GetInstanceName(child) == name) return child;
        uintptr_t found = FindFirstDescendant(child, name, depth + 1);
        if (found) return found;
    }
    return 0;
}

// ---- DataModel (cadena directa con FakeDataModel::Pointer) ----
inline uintptr_t GetDataModel() {
    uintptr_t fdm = mem.Read<uintptr_t>(mem.base + Offsets::FakeDataModel::Pointer);
    if (!fdm) return 0;
    return mem.Read<uintptr_t>(fdm + Offsets::FakeDataModel::RealDataModel);
}

// ---- ViewProjection matrix ----
// g_cachedDm, g_width, g_height son globales definidas en RbxESP.cpp / Overlay.hpp
extern uintptr_t g_cachedDm;
extern int g_width, g_height;

// Valida que una ViewMatrix tenga valores razonables (no todo ceros/NaN)
static inline bool IsValidVM(const ViewMatrix_t& vm) {
    float s = 0.f;
    for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) s += fabsf(vm.m[i][j]);
    return s > 0.1f && s < 1e10f;
}

inline ViewMatrix_t GetViewMatrix() {
    ViewMatrix_t vp{};

    // ── Método 1: leer ViewMatrix directo del VisualEngine (no depende de offsets de cámara) ──
    if (Offsets::VisualEngine::Pointer && Offsets::VisualEngine::ViewMatrix) {
        uintptr_t ve = mem.Read<uintptr_t>(mem.base + Offsets::VisualEngine::Pointer);
        if (ve) {
            ViewMatrix_t vm = mem.Read<ViewMatrix_t>(ve + Offsets::VisualEngine::ViewMatrix);
            if (IsValidVM(vm)) return vm;
        }
    }

    // ── Método 2: calcular desde Camera Rotation + Position + FOV (fallback) ──
    uintptr_t dm = g_cachedDm;
    if (!dm) return vp;
    uintptr_t ws = mem.Read<uintptr_t>(dm + Offsets::DataModel::Workspace);
    if (!ws) return vp;
    uintptr_t cam = mem.Read<uintptr_t>(ws + Offsets::Workspace::CurrentCamera);
    if (!cam) return vp;

    // Roblox Camera layout: Rotation matrix (9 floats) at CFrame offset, Position (3 floats) at Position offset
    float R[9]{}, P[3]{};
    ReadProcessMemory(mem.proc, (LPCVOID)(cam + Offsets::Camera::CFrame), R, 36, nullptr);
    ReadProcessMemory(mem.proc, (LPCVOID)(cam + Offsets::Camera::Position), P, 12, nullptr);

    float fov = mem.Read<float>(cam + Offsets::Camera::FieldOfView);
    float fovRad = (fov > 3.2f) ? (fov * 3.14159265f / 180.f) : fov;
    if (fovRad < 0.01f || fovRad > 3.14f) fovRad = 1.2217f;
    float aspect = (g_width > 0 && g_height > 0) ? (float)g_width / (float)g_height : 16.f/9.f;
    float f = 1.f / tanf(fovRad * 0.5f);
    float nearP = 0.1f, farP = 10000.f;

    // R row-major: R[0..2]=right, R[3..5]=up, R[6..8]=back(-look)
    // View = R^T, translation = -R^T * P
    float vr0=R[0], vr1=R[3], vr2=R[6];
    float vu0=R[1], vu1=R[4], vu2=R[7];
    float vf0=R[2], vf1=R[5], vf2=R[8];
    float tx = -(vr0*P[0] + vr1*P[1] + vr2*P[2]);
    float ty = -(vu0*P[0] + vu1*P[1] + vu2*P[2]);
    float tz = -(vf0*P[0] + vf1*P[1] + vf2*P[2]);

    float A = -(farP + nearP) / (farP - nearP);
    float B = -2.f * farP * nearP / (farP - nearP);

    vp.m[0][0] = (f/aspect)*vr0; vp.m[0][1] = (f/aspect)*vr1; vp.m[0][2] = (f/aspect)*vr2; vp.m[0][3] = (f/aspect)*tx;
    vp.m[1][0] = f*vu0;          vp.m[1][1] = f*vu1;          vp.m[1][2] = f*vu2;          vp.m[1][3] = f*ty;
    vp.m[2][0] = A*vf0;          vp.m[2][1] = A*vf1;          vp.m[2][2] = A*vf2;          vp.m[2][3] = A*tz + B;
    vp.m[3][0] = -vf0;           vp.m[3][1] = -vf1;           vp.m[3][2] = -vf2;           vp.m[3][3] = -tz;

    return vp;
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
