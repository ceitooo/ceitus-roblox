#define NOMINMAX
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "winhttp.lib")
#include "Memory.hpp"
#include "Math.hpp"
#include "ESP.hpp"
#include "Overlay.hpp"
#include "OffsetUpdater.hpp"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <thread>
#include <chrono>
#include <string>
#include <TlHelp32.h>
#include <algorithm>
#include <fstream>
#include <functional>
#include <sstream>
#include <iomanip>
#include <wininet.h>
#include <urlmon.h>
#include <winhttp.h>
#include <atomic>
#include <mutex>

// ---- versión actual ----
#define CEITUS_VERSION "1.0.1"

// ---- URLs GitHub (reemplazar USER/REPO con tu repo real) ----
#define GITHUB_USER      "ceitooo"
#define GITHUB_REPO      "ceitus-roblox"
#define URL_VERSION      "https://raw.githubusercontent.com/" GITHUB_USER "/" GITHUB_REPO "/main/version.txt"
#define URL_BANNED_HWIDS "https://raw.githubusercontent.com/" GITHUB_USER "/" GITHUB_REPO "/main/banned.txt"
#define URL_EXE          "https://github.com/" GITHUB_USER "/" GITHUB_REPO "/releases/latest/download/RbxESP.exe"

// ---- salt interno para derivar keys (no cambiar después de distribuir) ----
static const uint32_t KEY_SALT = 0xCE17A5B3u;

// ---- HWID: número de serie del disco C:\ + nombre de máquina hasheado ----
static uint32_t GetHWID() {
    DWORD serial = 0;
    GetVolumeInformationW(L"C:\\", nullptr, 0, &serial, nullptr, nullptr, nullptr, 0);
    wchar_t compName[MAX_COMPUTERNAME_LENGTH + 1] = { 0 };
    DWORD compSize = sizeof(compName) / sizeof(wchar_t);
    GetComputerNameW(compName, &compSize);
    for (DWORD i = 0; i < compSize; i++) {
        serial = (serial * 33) ^ compName[i];
    }
    serial ^= (serial >> 16);
    serial *= 0x45d9f3bu;
    serial ^= (serial >> 16);
    return serial;
}

// ---- Anti-Debugging & Seguridad ----
static bool PerformSecurityCheck() {
    if (IsDebuggerPresent()) return false;

    BOOL isRemoteDebugger = FALSE;
    if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &isRemoteDebugger) && isRemoteDebugger)
        return false;

    typedef NTSTATUS(NTAPI* pfnNtQueryInformationProcess)(
        HANDLE ProcessHandle, ULONG ProcessInformationClass,
        PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength
    );
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (hNtdll) {
        pfnNtQueryInformationProcess NtQueryInfo =
            (pfnNtQueryInformationProcess)GetProcAddress(hNtdll, "NtQueryInformationProcess");
        if (NtQueryInfo) {
            DWORD_PTR debugPort = 0;
            NTSTATUS status = NtQueryInfo(GetCurrentProcess(), 7, &debugPort, sizeof(debugPort), NULL);
            if (status == 0 && debugPort != 0) return false;
        }
    }
    return true;
}

static std::string HWIDString() {
    uint32_t h = GetHWID();
    char buf[16];
    snprintf(buf, sizeof(buf), "%04X-%04X", (h >> 16) & 0xFFFF, h & 0xFFFF);
    return buf;
}

// ---- derivar key válida para un HWID (usar en tu generador externo) ----
static std::string DeriveKey(uint32_t hwid) {
    uint32_t k = hwid ^ KEY_SALT;
    k *= 0x9e3779b9u;
    k ^= (k >> 13);
    k *= 0x85ebca6bu;
    k ^= (k >> 16);
    char buf[16];
    snprintf(buf, sizeof(buf), "%04X-%04X", (k >> 16) & 0xFFFF, k & 0xFFFF);
    return buf;
}

static bool ValidateKey(const std::string& key) {
    return key == DeriveKey(GetHWID());
}

// ---- HTTP GET simple (WinINet) ----
static std::string HttpGet(const char* url, int timeoutMs = 5000) {
    std::string result;
    HINTERNET hNet = InternetOpenA("ceitus/1.0", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hNet) return result;
    HINTERNET hUrl = InternetOpenUrlA(hNet, url, nullptr, 0,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE |
        INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID, 0);
    if (hUrl) {
        char buf[512]; DWORD read = 0;
        while (InternetReadFile(hUrl, buf, sizeof(buf)-1, &read) && read > 0) {
            buf[read] = 0; result += buf;
        }
        InternetCloseHandle(hUrl);
    }
    InternetCloseHandle(hNet);
    // trim whitespace
    while (!result.empty() && (result.back()=='\r'||result.back()=='\n'||result.back()==' ')) result.pop_back();
    return result;
}

// ---- backend ceitus ----
static std::string HttpPostJSON(const wchar_t* host, const wchar_t* path, const std::string& body) {
    std::string result;
    HINTERNET hSes = WinHttpOpen(L"ceitus/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSes) return result;
    HINTERNET hCon = WinHttpConnect(hSes, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hCon) { WinHttpCloseHandle(hSes); return result; }
    HINTERNET hReq = WinHttpOpenRequest(hCon, L"POST", path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!hReq) { WinHttpCloseHandle(hCon); WinHttpCloseHandle(hSes); return result; }
    BOOL sent = WinHttpSendRequest(hReq, L"Content-Type: application/json\r\n", (DWORD)-1,
        (LPVOID)body.c_str(), (DWORD)body.size(), (DWORD)body.size(), 0);
    if (sent && WinHttpReceiveResponse(hReq, nullptr)) {
        DWORD read = 0; char buf[512];
        while (WinHttpReadData(hReq, buf, sizeof(buf)-1, &read) && read > 0) {
            buf[read] = 0; result += buf;
        }
    }
    WinHttpCloseHandle(hReq); WinHttpCloseHandle(hCon); WinHttpCloseHandle(hSes);
    return result;
}

// ---- estado de update y ban ----
static std::atomic<bool> g_updateAvailable{ false };
static std::atomic<bool> g_hwIdBanned{ false };
static std::string       g_latestVersion;

// ---- sistema de keys ----
enum class LoginState { KeyInput, LoggedIn };
static LoginState   g_loginState = LoginState::KeyInput;
static std::string  g_activeKey;   // key activa
static std::string  g_uid;         // ID dorado en info

static uint32_t SimpleHash(const std::string& s) {
    uint32_t h = 0x811c9dc5u;
    for (unsigned char c : s) { h ^= c; h *= 0x01000193u; }
    return h;
}

// UID único basado en key + HWID
static std::string MakeUID(const std::string& key, uint32_t hwid) {
    uint32_t h = SimpleHash(key) ^ hwid ^ 0xB7E15163u;
    h *= 0x6c62272eu; h ^= (h >> 17);
    char buf[12]; snprintf(buf, sizeof(buf), "C-%06X", h & 0xFFFFFF);
    return buf;
}

static std::string LicensePath() { return "ceitus_license.cfg"; }

static std::string TodayStr() {
    time_t now = time(nullptr);
    struct tm t{}; localtime_s(&t, &now);
    char buf[16]; strftime(buf, sizeof(buf), "%Y-%m-%d", &t);
    return buf;
}

// Guardar key + HWID + fecha de canje + dias localmente
static void SaveLicense(const std::string& key, uint32_t hwid, int days) {
    std::ofstream f(LicensePath());
    if (f) f << key << "\n" << hwid << "\n" << days << "\n" << TodayStr() << "\n";
}

// Cargar y verificar: devuelve false si venció
static bool LoadLicense(std::string& keyOut) {
    std::ifstream f(LicensePath());
    if (!f) return false;
    std::string k, redeemedAt; uint32_t hw = 0; int days = 0;
    if (!(f >> k >> hw >> days >> redeemedAt)) return false;
    if (hw != GetHWID()) return false;
    // verificar vencimiento
    if (days > 0 && !redeemedAt.empty() && redeemedAt != "NONE") {
        struct tm r = {};
        sscanf_s(redeemedAt.c_str(), "%d-%d-%d", &r.tm_year, &r.tm_mon, &r.tm_mday);
        r.tm_year -= 1900; r.tm_mon -= 1;
        time_t expT = mktime(&r) + (time_t)days * 86400;
        if (time(nullptr) > expT) return false; // venció
    }
    keyOut = k;
    return true;
}

// Verificar key en backend. Retorna: 0=permanente, >0=dias, -1=invalida, -2=canjeada por otro
static int CheckKeyOnline(const std::string& key) {
    std::string hwidStr = HWIDString();
    std::string body = "{\"action\":\"ceitus-verify\",\"key\":\"" + key + "\",\"hwid\":\"" + hwidStr + "\"}";
    std::string resp = HttpPostJSON(L"ceitotweaks-backend.vercel.app", L"/api/owner-stats", body);
    if (resp.empty()) return -1;
    if (resp.find("\"valid\":true") == std::string::npos) {
        // Detectar rechazo por HWID distinto (daysLeft:-2)
        auto p2 = resp.find("\"daysLeft\":");
        if (p2 != std::string::npos) {
            int d2 = 0; sscanf_s(resp.c_str() + p2 + 11, "%d", &d2);
            if (d2 == -2) return -2;
        }
        return -1;
    }
    auto pos = resp.find("\"daysLeft\":");
    if (pos == std::string::npos) return 0;
    pos += 11;
    if (pos < resp.size() && resp[pos] == 'n') return 0;
    int d = 0; sscanf_s(resp.c_str() + pos, "%d", &d);
    return d >= 0 ? d : 0;
}

static std::atomic<bool> g_checking{ false };
static std::atomic<bool> g_checkDone{ false };
static std::atomic<bool> g_checkOk{ false };

static void NetworkThread() {
    Sleep(2000);
    std::string hwidStr = HWIDString();

    std::string banned = HttpGet(URL_BANNED_HWIDS);
    if (!banned.empty()) {
        std::istringstream ss(banned);
        std::string line;
        while (std::getline(ss, line)) {
            while (!line.empty() && (line.back()=='\r'||line.back()=='\n'||line.back()==' ')) line.pop_back();
            if (line == hwidStr) { g_hwIdBanned = true; break; }
        }
    }

    std::string latest = HttpGet(URL_VERSION);
    if (!latest.empty() && latest != CEITUS_VERSION) {
        g_latestVersion = latest;
        g_updateAvailable = true;
    }
}

ID3D11Device*           g_dev   = nullptr;
ID3D11DeviceContext*    g_ctx   = nullptr;
IDXGISwapChain*         g_chain = nullptr;
ID3D11RenderTargetView* g_rtv   = nullptr;

bool bESP        = true;
bool bBoxes      = true;
bool bSnaplines  = false;
bool bNames      = true;
bool bDistance   = true;
bool bSkeleton   = true;
int  espColorMode = 0; // 0: Por defecto/equipo, 1: Rojo, 2: Azul, 3: Naranja, 4: Verde, 5: Rainbow
bool bHealthBar  = true;
bool bAimbot     = false;
bool bBhop       = false;
bool bTriggerbot = false;
bool bRadar      = false;
bool bPrediction = false;
float fAimFov      = 80.f;
float fAimSmooth   = 60.f;
float fTriggerFov  = 8.f;   // px radius para triggerbot
float fRadarRange  = 150.f; // studs en el radar
float fPrediction  = 0.08f; // segundos de predicción
float fAimMaxDist       = 200.f; // distancia máxima en studs para aimbot
bool  bAimbotTeamFilter = false; // ignorar compañeros de equipo en aimbot
bool  bAimbotNoFov = false;      // apuntar al más cercano sin restricción de FOV

// ---- cache ESP: ESPScanThread actualiza, render solo lee snapshot ----
struct CachedPlayerESP {
    Vector3     pos, headPos, feetPos;
    Vector3     bones[21];
    bool        boneOk[21];
    float       health, maxHealth;
    std::string name;
    COLORREF    col;
    uintptr_t   character;
    uintptr_t   hrp; // puntero al HumanoidRootPart — para leer posicion fresca en render sin FindFirstChild
};
static std::vector<CachedPlayerESP> g_espCache;
static std::mutex                   g_espMtx;
static Vector3                      g_cachedLocalPosESP{};
static uintptr_t                    g_cachedLp = 0;
static uintptr_t                    g_cachedDm = 0;

static COLORREF HSVtoRGB(float h, float s, float v) {
    float r = 0, g = 0, b = 0;
    int i = (int)(h * 6.0f);
    float f = h * 6.0f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);
    switch (i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }
    return RGB((BYTE)(r * 255.f), (BYTE)(g * 255.f), (BYTE)(b * 255.f));
}

static COLORREF GetESPColor(COLORREF defaultCol, int mode) {
    switch (mode) {
        case 1: return RGB(255, 40, 40);   // Rojo
        case 2: return RGB(40, 140, 255);  // Azul
        case 3: return RGB(255, 140, 0);   // Naranja
        case 4: return RGB(40, 220, 40);   // Verde
        case 5: {                          // Rainbow / Multicolor
            float hue = fmodf((float)(GetTickCount() % 3000) / 3000.f, 1.0f);
            return HSVtoRGB(hue, 1.0f, 1.0f);
        }
        default: return defaultCol;
    }
}
bool  bAntiFlash = false;
bool  bAntiSmoke = false;
int   aimbotKey  = 'X';
int   menuKey    = VK_INSERT;
int   gameMode   = 0;
bool  bTeamFilter = false;
const char* gameModeNames[] = { "General", "Counterblox", "Duels", "Arsenal", "Rivals" };

// shared state entre threads
ViewMatrix_t lastVm{};
uintptr_t    lastPs = 0;
uintptr_t    g_cachedAimTarget = 0; // target actual del aimbot (para FOV dinámico)

// radar: lista de entradas para dibujar (llenada en ESP loop, leída en render)
struct RadarEntry { float dx, dz; COLORREF col; };
static std::vector<RadarEntry> g_radarEntries;
static Vector3 g_localPos{};
static int64_t  g_placeId = 0;       // PlaceId del juego actual
static int64_t  g_rivalsPlaceId = 0; // PlaceId de Rivals (guardado por el usuario)

// debug
int   dbgPlayers    = 0;
int   dbgValid      = 0;
float dbgEnemyHp    = -1.f;
float dbgEnemyMaxHp = -1.f;
float dbgVmSum      = 0.f;
bool      dbgDM         = false;
bool      dbgPS         = false;
uintptr_t dbgDMAddr     = 0;
uintptr_t dbgPSAddr     = 0;
int       dbgPSChildren = 0;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (ImGui_ImplWin32_WndProcHandler(h, m, w, l)) return true;
    if (m == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcW(h, m, w, l);
}

HWND FindRobloxWindow() {
    struct ED { DWORD pid; HWND hwnd; };
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W pe{ sizeof(pe) };
    ED ed{};
    while (Process32NextW(snap, &pe))
        if (!wcscmp(pe.szExeFile, L"RobloxPlayerBeta.exe")) { ed.pid = pe.th32ProcessID; break; }
    CloseHandle(snap);
    EnumWindows([](HWND h, LPARAM lp) -> BOOL {
        auto* d = reinterpret_cast<ED*>(lp);
        DWORD p = 0; GetWindowThreadProcessId(h, &p);
        if (p == d->pid && IsWindowVisible(h)) { d->hwnd = h; return FALSE; }
        return TRUE; }, (LPARAM)&ed);
    return ed.hwnd;
}

static void BoneLine(const ViewMatrix_t& vm, Vector3 a, Vector3 b,
                     float sw, float sh, COLORREF col) {
    Vector2 sa, sb;
    if (WorldToScreen(vm, a, sa, sw, sh) && WorldToScreen(vm, b, sb, sw, sh))
        DrawLine(sa, sb, col, 2);
}

// Skeleton estilo stickman: círculo para la cabeza + líneas de huesos
static void DrawSkeleton(uintptr_t character, const ViewMatrix_t& vm,
                         float sw, float sh, COLORREF col) {
    col = GetESPColor(col, espColorMode);
    const char* boneNames[] = {
        "Head",        // 0
        "UpperTorso",  // 1  (R15)
        "LowerTorso",  // 2  (R15)
        "HumanoidRootPart", // 3
        "LeftUpperArm","LeftLowerArm","LeftHand",         // 4 5 6
        "RightUpperArm","RightLowerArm","RightHand",      // 7 8 9
        "LeftUpperLeg","LeftLowerLeg","LeftFoot",         // 10 11 12
        "RightUpperLeg","RightLowerLeg","RightFoot",      // 13 14 15
        // R6 fallback
        "Torso",       // 16
        "Left Arm",    // 17
        "Right Arm",   // 18
        "Left Leg",    // 19
        "Right Leg",   // 20
    };
    const int N = 21;
    Vector3 pos[N]{};
    bool ok[N]{};
    for (int i = 0; i < N; i++) {
        uintptr_t part = FindFirstChild(character, boneNames[i]);
        if (part) { pos[i] = GetPartPosition(part); ok[i] = true; }
    }

    bool isR15 = ok[1] || ok[2]; // tiene UpperTorso/LowerTorso

    if (isR15) {
        // cabeza: círculo centrado en la posición de Head
        if (ok[0]) {
            Vector2 hsc;
            if (WorldToScreen(vm, pos[0], hsc, sw, sh))
                DrawEllipse(hsc, 7.f, 7.f, col);
        }
        // columna
        if (ok[0] && ok[1]) BoneLine(vm, pos[0], pos[1], sw, sh, col);
        if (ok[1] && ok[2]) BoneLine(vm, pos[1], pos[2], sw, sh, col);
        // brazos
        if (ok[1] && ok[4]) BoneLine(vm, pos[1], pos[4], sw, sh, col);
        if (ok[4] && ok[5]) BoneLine(vm, pos[4], pos[5], sw, sh, col);
        if (ok[5] && ok[6]) BoneLine(vm, pos[5], pos[6], sw, sh, col);
        if (ok[1] && ok[7]) BoneLine(vm, pos[1], pos[7], sw, sh, col);
        if (ok[7] && ok[8]) BoneLine(vm, pos[7], pos[8], sw, sh, col);
        if (ok[8] && ok[9]) BoneLine(vm, pos[8], pos[9], sw, sh, col);
        // piernas
        if (ok[2] && ok[10]) BoneLine(vm, pos[2], pos[10], sw, sh, col);
        if (ok[10] && ok[11]) BoneLine(vm, pos[10], pos[11], sw, sh, col);
        if (ok[11] && ok[12]) BoneLine(vm, pos[11], pos[12], sw, sh, col);
        if (ok[2] && ok[13]) BoneLine(vm, pos[2], pos[13], sw, sh, col);
        if (ok[13] && ok[14]) BoneLine(vm, pos[13], pos[14], sw, sh, col);
        if (ok[14] && ok[15]) BoneLine(vm, pos[14], pos[15], sw, sh, col);
    } else {
        // R6
        if (ok[0]) {
            Vector2 hsc;
            if (WorldToScreen(vm, pos[0], hsc, sw, sh))
                DrawEllipse(hsc, 7.f, 7.f, col);
        }
        if (ok[0] && ok[16]) BoneLine(vm, pos[0], pos[16], sw, sh, col);
        if (ok[16] && ok[17]) BoneLine(vm, pos[16], pos[17], sw, sh, col);
        if (ok[16] && ok[18]) BoneLine(vm, pos[16], pos[18], sw, sh, col);
        if (ok[16] && ok[19]) BoneLine(vm, pos[16], pos[19], sw, sh, col);
        if (ok[16] && ok[20]) BoneLine(vm, pos[16], pos[20], sw, sh, col);
    }
}

// ---- hilo de lectura de ViewMatrix a 200Hz ----
static void VMReaderThread() {
    timeBeginPeriod(1);
    while (true) {
        ViewMatrix_t vmFresh = GetViewMatrix();
        float s = 0.f;
        for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) s += fabsf(vmFresh.m[r][c]);
        if (s >= 0.001f) lastVm = vmFresh;
        Sleep(5); // 200Hz
    }
}

// ---- hilo del aimbot ----
static void AimbotThread() {
    timeBeginPeriod(1);

    uintptr_t cachedPlayer   = 0;
    uintptr_t cachedHeadPart = 0;
    uintptr_t cachedHRP      = 0;
    uintptr_t cachedChar     = 0;
    float     lockedHRPY    = 0.f;
    DWORD     lastScanTime   = 0;
    bool      hasSmooth      = false;
    uint8_t   lastHumState   = 0;
    // exclusión de jugador muerto: bloquea el PLAYER pointer (no el char) por 1.5s tras detectar muerte
    // así el respawn con nuevo character también queda bloqueado
    uintptr_t lastDeadPlayer = 0;
    DWORD     lastDeadTime   = 0;

    // predicción de movimiento
    Vector3 lastHeadW{};
    DWORD   lastHeadTime = 0;
    Vector3 headVel{};

    // triggerbot: evitar spam de clicks
    DWORD lastTriggerFire = 0;

    while (true) {
        Sleep(8);

        // ---- bunny hop (escribe Humanoid.Jump directamente en memoria) ----
        if (bBhop && (GetAsyncKeyState(VK_SPACE) & 0x8000)) {
            uintptr_t dm = GetDataModel();
            uintptr_t ps = dm ? GetPlayersService(dm) : 0;
            uintptr_t lp = ps ? GetLocalPlayer(ps) : 0;
            if (lp) {
                uintptr_t lchar = mem.Read<uintptr_t>(lp + Offsets::Player::ModelInstance);
                if (lchar) {
                    uintptr_t lhum = FindFirstChild(lchar, "Humanoid");
                    if (lhum) {
                        uintptr_t lhrp = mem.Read<uintptr_t>(lhum + Offsets::Humanoid::HumanoidRootPart);
                        if (!lhrp) lhrp = FindFirstChild(lchar, "HumanoidRootPart");
                        if (lhrp) {
                            uintptr_t prim = mem.Read<uintptr_t>(lhrp + Offsets::BasePart::Primitive);
                            if (prim) {
                                float vy = mem.Read<float>(prim + Offsets::Primitive::AssemblyLinearVelocity + 4);
                                // vy ~0 = en el suelo. Escribir Jump=true directo en memoria
                                if (fabsf(vy) < 1.5f) {
                                    uint8_t jmp = 1;
                                    WriteProcessMemory(mem.proc,
                                        (LPVOID)(lhum + Offsets::Humanoid::Jump),
                                        &jmp, 1, nullptr);
                                }
                                lastHumState = (vy < -2.f) ? 1 : 0;
                            }
                        }
                    }
                }
            }
        } else { lastHumState = 0; }

        if (!bAimbot && !bTriggerbot) {
            hasSmooth = false; cachedPlayer = 0; g_cachedAimTarget = 0; continue;
        }

        float sw = (float)g_width, sh = (float)g_height;
        float cx = sw * 0.5f, cy = sh * 0.5f;
        ViewMatrix_t vm = lastVm;
        DWORD now = GetTickCount();

        // --- limpiar si el target murió o respawneó ---
        if (cachedPlayer) {
            uintptr_t curChar = mem.Read<uintptr_t>(cachedPlayer + Offsets::Player::ModelInstance);
            bool charChanged = (curChar != cachedChar) || (curChar == 0);
            bool dead = false;
            if (!charChanged && curChar) {
                // detección por HRP: si bajó >35 studs desde el lock = enviado al spawn
                if (cachedHRP && lockedHRPY != 0.f) {
                    Vector3 hrpNow = GetPartPosition(cachedHRP);
                    if (hrpNow.y < lockedHRPY - 35.f)
                        dead = true;
                }
            }
            if (charChanged || dead) {
                if (cachedPlayer) { lastDeadPlayer = cachedPlayer; lastDeadTime = GetTickCount(); }
                cachedPlayer = 0; cachedHeadPart = 0; cachedHRP = 0; cachedChar = 0;
                g_cachedAimTarget = 0; lastHeadTime = 0; headVel = {}; lockedHRPY = 0.f;
            }
        }

        // --- verificar si el objetivo sigue en el FOV ---
        bool targetValid = false;
        float cachedDist = 99999.f;
        if (cachedPlayer) {
            Vector3 hw{};
            if (cachedHeadPart) { hw = GetPartPosition(cachedHeadPart); }
            else if (cachedHRP)  { hw = GetPartPosition(cachedHRP); hw.y += 2.3f; }
            if (hw.x != 0.f || hw.z != 0.f) {
                // si está debajo del mapa → soltar inmediatamente (umbral absoluto)
                if (hw.y < -1000.f) {
                    if (cachedPlayer) { lastDeadPlayer = cachedPlayer; lastDeadTime = GetTickCount(); }
                    cachedPlayer = 0; cachedHeadPart = 0; cachedHRP = 0; cachedChar = 0;
                    g_cachedAimTarget = 0; lastHeadTime = 0; headVel = {}; lockedHRPY = 0.f;
                } else {
                    // chequeo de distancia 3D — si está más lejos que el límite, soltar
                    float llen2 = g_localPos.x*g_localPos.x + g_localPos.y*g_localPos.y + g_localPos.z*g_localPos.z;
                    float dist3d = (llen2 > 1.f) ? Dist3D(g_localPos, hw) : 0.f;
                    bool tooFar = (llen2 > 1.f && dist3d > fAimMaxDist);
                    if (!tooFar) {
                        Vector2 hs;
                        if (WorldToScreen(vm, hw, hs, sw, sh)) {
                            float dx2 = hs.x - cx, dy2 = hs.y - cy;
                            float screenDist2 = sqrtf(dx2*dx2 + dy2*dy2);
                            if (screenDist2 < fAimFov) { // soltar si sale del FOV
                                cachedDist = (dist3d >= 0.f) ? dist3d : screenDist2;
                                targetValid = true;
                            }
                        }
                    }
                }
            }
        }

        // --- scan: buscar mejor target ---
        if (now - lastScanTime > 100 || !targetValid) {
            uintptr_t dm = GetDataModel();
            uintptr_t ps = dm ? GetPlayersService(dm) : 0;
            if (!ps && lastPs) { auto t = GetAllPlayers(lastPs); if (!t.empty()) ps = lastPs; }
            uintptr_t lp = ps ? GetLocalPlayer(ps) : 0;
            uintptr_t localTeam = lp ? mem.Read<uintptr_t>(lp + Offsets::Player::Team) : 0;

            float bestDist = targetValid ? cachedDist * 0.75f : 99999.f;
            uintptr_t newPlayer = 0, newHP = 0, newHRP = 0;

            if (ps) {
                for (uintptr_t player : GetAllPlayers(ps)) {
                    if (player == lp || player == cachedPlayer) continue;
                    uintptr_t pTeam = mem.Read<uintptr_t>(player + Offsets::Player::Team);
                    bool aMate = (localTeam > 0x10000000000ULL && pTeam > 0x10000000000ULL && pTeam == localTeam);
                    if (bAimbotTeamFilter && aMate) continue;

                    uintptr_t chr = mem.Read<uintptr_t>(player + Offsets::Player::ModelInstance);
                    if (!chr) continue;
                    // no re-adquirir al mismo JUGADOR durante 1.5 segundos tras su muerte
                    if (player == lastDeadPlayer && (GetTickCount() - lastDeadTime) < 1500) continue;

                    uintptr_t hp  = FindFirstChild(chr, "Head");
                    uintptr_t hum = FindFirstChild(chr, "Humanoid");
                    // siempre buscar HRP (necesario para detección de muerte)
                    uintptr_t hrp = hum ? mem.Read<uintptr_t>(hum + Offsets::Humanoid::HumanoidRootPart) : 0;
                    if (!hrp) hrp = FindFirstChild(chr, "HumanoidRootPart");
                    if (!hp && !hrp) continue;
                    // nota: no usamos health/state del scan — offset puede ser incorrecto según el juego
                    // la detección de muerte se hace en el retention check (Y < -1000) y lastDeadPlayer

                    Vector3 hw{};
                    if (hp) hw = GetPartPosition(hp);
                    else  { hw = GetPartPosition(hrp); hw.y += 2.3f; }
                    if (hw.y < -1000.f || fabsf(hw.y) > 1000000.f) continue;
                    if (hw.y < -200.f) continue;  // debajo del mapa (muertos enviados al spawn underground)

                    float llen2 = g_localPos.x*g_localPos.x + g_localPos.y*g_localPos.y + g_localPos.z*g_localPos.z;
                    float dist3d = (llen2 > 1.f) ? Dist3D(g_localPos, hw) : -1.f;
                    if (dist3d > 0.f && dist3d > fAimMaxDist) continue;

                    Vector2 hs;
                    if (!WorldToScreen(vm, hw, hs, sw, sh)) continue;
                    float dx = hs.x - cx, dy = hs.y - cy;
                    float screenDist = sqrtf(dx*dx + dy*dy);
                    if (screenDist > fAimFov) continue; // solo dentro del circulo FOV
                    // dentro del FOV: priorizar por distancia 3D (o pantalla si localPos no valida)
                    float metric = (dist3d >= 0.f) ? dist3d : screenDist;
                    if (metric < bestDist) { bestDist = metric; newPlayer = player; newHP = hp; newHRP = hrp; }
                }
            }
            lastScanTime = now;

            if (newPlayer) {
                cachedPlayer = newPlayer; cachedHeadPart = newHP; cachedHRP = newHRP;
                cachedChar = mem.Read<uintptr_t>(newPlayer + Offsets::Player::ModelInstance);
                lastHeadTime = 0; headVel = {};
                // guardar Y del HRP para detectar si lo envían al spawn
                lockedHRPY = newHRP ? GetPartPosition(newHRP).y
                           : (newHP  ? GetPartPosition(newHP).y : 0.f);
            }
        }

        if (!targetValid && !cachedPlayer) { g_cachedAimTarget = 0; continue; }
        g_cachedAimTarget = cachedPlayer;
        if (!cachedPlayer || !targetValid) continue; // no mover mouse si está fuera del FOV

        Vector3 headW{};
        if (cachedHeadPart) { headW = GetPartPosition(cachedHeadPart); headW.y += 0.3f; }
        else if (cachedHRP) { headW = GetPartPosition(cachedHRP); headW.y += 2.8f; }
        else continue;
        if (fabsf(headW.y) > 1000000.f) continue;

        // ---- predicción de movimiento ----
        if (lastHeadTime && now > lastHeadTime) {
            float dt = (now - lastHeadTime) / 1000.f;
            if (dt > 0.001f && dt < 0.12f) {
                // promedio ponderado de velocidad para suavizar
                float vx = (headW.x - lastHeadW.x) / dt;
                float vy = (headW.y - lastHeadW.y) / dt;
                float vz = (headW.z - lastHeadW.z) / dt;
                headVel.x = headVel.x * 0.6f + vx * 0.4f;
                headVel.y = headVel.y * 0.6f + vy * 0.4f;
                headVel.z = headVel.z * 0.6f + vz * 0.4f;
            }
        }
        lastHeadW = headW; lastHeadTime = now;
        if (bPrediction) {
            headW.x += headVel.x * fPrediction;
            headW.y += headVel.y * fPrediction;
            headW.z += headVel.z * fPrediction;
        }

        Vector2 headS;
        if (!WorldToScreen(vm, headW, headS, sw, sh)) continue;

        float dx = headS.x - cx, dy = headS.y - cy;
        float dist = sqrtf(dx*dx + dy*dy);

        // ---- triggerbot ----
        if (bTriggerbot && dist < fTriggerFov && now - lastTriggerFire > 150) {
            INPUT inp{}; inp.type = INPUT_MOUSE;
            inp.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; SendInput(1, &inp, sizeof(INPUT));
            Sleep(40);
            inp.mi.dwFlags = MOUSEEVENTF_LEFTUP;   SendInput(1, &inp, sizeof(INPUT));
            lastTriggerFire = now;
        }

        // ---- aimbot ----
        bool keyHeld = bAimbot && aimbotKey && (GetAsyncKeyState(aimbotKey) & 0x8000);
        static float accumX = 0.f, accumY = 0.f;
        if (!keyHeld) {
            // solo limpiar acumulador — mantener el lock para re-engancharse rapido
            accumX = 0.f; accumY = 0.f;
        }
        if (keyHeld) {
            if (dist < 1.5f) continue; // deadzone mínimo

            // gain lineal: 1=muy lento, 80=max (>80 oscila por sensibilidad del juego)
            float gain = std::clamp(fAimSmooth / 400.f, 0.01f, 0.20f);

            accumX += dx * gain; accumY += dy * gain;
            LONG mx = (LONG)accumX, my = (LONG)accumY;
            accumX -= mx; accumY -= my;
            if (mx || my) {
                INPUT inp{}; inp.type = INPUT_MOUSE;
                inp.mi.dwFlags = MOUSEEVENTF_MOVE;
                inp.mi.dx = mx; inp.mi.dy = my;
                SendInput(1, &inp, sizeof(INPUT));
            }
        }
    }
    timeEndPeriod(1);
}

static void ESPScanThread() {
    const char* boneNames[] = {
        "Head","UpperTorso","LowerTorso","HumanoidRootPart",
        "LeftUpperArm","LeftLowerArm","LeftHand",
        "RightUpperArm","RightLowerArm","RightHand",
        "LeftUpperLeg","LeftLowerLeg","LeftFoot",
        "RightUpperLeg","RightLowerLeg","RightFoot",
        "Torso","Left Arm","Right Arm","Left Leg","Right Leg",
    };
    DWORD boneTickLast = 0;
    while (true) {
        Sleep(8); // ~120Hz
        if (!bESP) {
            std::lock_guard<std::mutex> lk(g_espMtx);
            g_espCache.clear();
            continue;
        }
        uintptr_t dm = GetDataModel();
        uintptr_t ps = dm ? GetPlayersService(dm) : 0;
        if (!ps) ps = lastPs;
        if (!ps) { std::lock_guard<std::mutex> lk(g_espMtx); g_espCache.clear(); continue; }
        uintptr_t lp = GetLocalPlayer(ps);
        g_cachedLp = lp;
        g_cachedDm = dm;
        uintptr_t localTeam = lp ? mem.Read<uintptr_t>(lp + Offsets::Player::Team) : 0;

        Vector3 localPos{};
        if (lp) {
            uintptr_t lc = mem.Read<uintptr_t>(lp + Offsets::Player::ModelInstance);
            if (lc) {
                uintptr_t lh = FindFirstChild(lc, "Humanoid");
                if (lh) {
                    uintptr_t lhrp = mem.Read<uintptr_t>(lh + Offsets::Humanoid::HumanoidRootPart);
                    if (!lhrp) lhrp = FindFirstChild(lc, "HumanoidRootPart");
                    if (lhrp) localPos = GetPartPosition(lhrp);
                }
            }
        }
        g_cachedLocalPosESP = localPos;

        DWORD now = GetTickCount();
        bool doBonesThisTick = bSkeleton && (now - boneTickLast >= 16); // ~60Hz
        if (doBonesThisTick) boneTickLast = now;

        auto allP = GetAllPlayers(ps);
        std::vector<CachedPlayerESP> newCache;
        newCache.reserve(allP.size());

        // copiar huesos viejos para los ticks sin bone-scan
        std::vector<CachedPlayerESP> oldSnap;
        if (!doBonesThisTick) {
            std::lock_guard<std::mutex> lk(g_espMtx);
            oldSnap = g_espCache;
        }

        for (uintptr_t player : allP) {
            if (player == lp) continue;
            PlayerInfo pi = ReadPlayer(player);
            if (!pi.valid || fabsf(pi.pos.y) > 1000000.f) continue;
            uintptr_t pTeam = mem.Read<uintptr_t>(player + Offsets::Player::Team);
            bool isTeammate = (localTeam > 0x10000000000ULL && pTeam > 0x10000000000ULL && pTeam == localTeam);
            if (bTeamFilter && isTeammate) continue;

            CachedPlayerESP ce{};
            ce.pos       = pi.pos;
            ce.health    = pi.health;
            ce.maxHealth = pi.maxHealth;
            ce.name      = pi.name;
            ce.character = pi.character;
            ce.hrp       = pi.hrp;

            ce.headPos = pi.pos; ce.headPos.y += 3.0f;
            ce.feetPos = pi.pos; ce.feetPos.y -= 3.0f;
            if (ce.headPos.y < ce.feetPos.y - 1.f) continue;

            COLORREF col;
            if (gameMode == 3) {
                static const COLORREF tp[] = { RGB(220,50,50),RGB(50,150,255),RGB(50,220,50),RGB(220,180,50) };
                int idx = pTeam ? (int)((pTeam>>12)%4) : 0;
                col = tp[idx];
                if (isTeammate) col = RGB(std::min(255,(int)GetRValue(col)+80),std::min(255,(int)GetGValue(col)+80),std::min(255,(int)GetBValue(col)+80));
            } else if (isTeammate) { col = RGB(50,150,255); } else { col = RGB(50,220,50); }
            ce.col = GetESPColor(col, espColorMode);

            if (doBonesThisTick && pi.character) {
                for (int i = 0; i < 21; i++) {
                    uintptr_t part = FindFirstChild(pi.character, boneNames[i]);
                    if (part) { ce.bones[i] = GetPartPosition(part); ce.boneOk[i] = true; }
                }
            } else {
                for (auto& old : oldSnap) {
                    if (old.character == ce.character) {
                        memcpy(ce.bones, old.bones, sizeof(ce.bones));
                        memcpy(ce.boneOk, old.boneOk, sizeof(ce.boneOk));
                        break;
                    }
                }
            }
            newCache.push_back(std::move(ce));
        }
        std::lock_guard<std::mutex> lk(g_espMtx);
        g_espCache = std::move(newCache);
    }
}

static const char* CFG_GLOBAL = "ceitus.cfg";
static const char* CFG_PER_MODE[] = { "ceitus_general.cfg", "ceitus_counterblox.cfg", "ceitus_duels.cfg", "ceitus_arsenal.cfg", "ceitus_rivals.cfg" };

static void SaveConfig(int mode = -1) {
    // guarda siempre el modo actual en el archivo global
    std::ofstream fg(CFG_GLOBAL);
    if (fg) fg << gameMode << "\n";
    // guarda settings en el archivo del modo indicado (o el actual)
    int m = (mode >= 0 && mode <= 4) ? mode : gameMode;
    std::ofstream f(CFG_PER_MODE[m]);
    if (!f) return;
    f << bESP       << "\n" << bBoxes    << "\n" << bSnaplines << "\n"
      << bNames     << "\n" << bDistance << "\n" << bSkeleton  << "\n"
      << bHealthBar << "\n" << bAimbot   << "\n"
      << fAimFov    << "\n" << fAimSmooth << "\n" << aimbotKey << "\n"
      << bTeamFilter << "\n" << bBhop << "\n"
      << bTriggerbot << "\n" << fTriggerFov << "\n"
      << bRadar << "\n" << fRadarRange << "\n"
      << bPrediction << "\n" << fPrediction << "\n"
      << fAimMaxDist << "\n" << bAimbotTeamFilter << "\n"
      << menuKey << "\n"
      << bAntiFlash << "\n" << bAntiSmoke << "\n"
      << bAimbotNoFov << "\n" << espColorMode << "\n";
}

static void LoadConfig(int mode = -1) {
    // leer último modo usado del archivo global
    int m = 0;
    { std::ifstream fg(CFG_GLOBAL); if (fg) fg >> m; }
    if (mode >= 0 && mode <= 4) m = mode;
    gameMode = m;
    // cargar settings del modo
    std::ifstream f(CFG_PER_MODE[m]);
    if (!f) return;
    f >> bESP >> bBoxes >> bSnaplines >> bNames >> bDistance >> bSkeleton
      >> bHealthBar >> bAimbot >> fAimFov >> fAimSmooth >> aimbotKey
      >> bTeamFilter >> bBhop
      >> bTriggerbot >> fTriggerFov
      >> bRadar >> fRadarRange
      >> bPrediction >> fPrediction
      >> fAimMaxDist >> bAimbotTeamFilter
      >> menuKey
      >> bAntiFlash >> bAntiSmoke;
    if (!f.eof()) f >> bAimbotNoFov;
    if (!f.eof()) f >> espColorMode;
    // clamp para evitar valores absurdos de configs viejas
    if (fAimMaxDist < 1.f) fAimMaxDist = 200.f;
    if (fAimFov > 300.f || fAimFov < 5.f) fAimFov = 80.f;
    if (fAimSmooth > 80.f || fAimSmooth < 0.1f) fAimSmooth = 60.f;
}

int main() {
    // Verificación de seguridad y Anti-Debugging
    if (!PerformSecurityCheck()) {
        ExitProcess(0);
        return 0;
    }

    // solo una instancia a la vez
    HANDLE hMutex = CreateMutexW(nullptr, TRUE, L"CeitusRbxESP_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) { CloseHandle(hMutex); return 0; }

    FreeConsole();

    // auto-login: si hay license local válida para este HWID y no venció, entrar directo
    {
        std::string k;
        if (LoadLicense(k)) {
            g_activeKey  = k;
            g_uid        = MakeUID(k, GetHWID());
            g_loginState = LoginState::LoggedIn;
        } else {
            // si existe el archivo pero falló (venció o HWID distinto), borrarlo
            std::remove(LicensePath().c_str());
        }
    }

    // migrar cfg viejo "ceitus_auto.cfg" → "ceitus_general.cfg" si el nuevo no existe
    { std::ifstream chk(CFG_PER_MODE[0]);
      if (!chk) {
          std::ifstream old("ceitus_auto.cfg");
          if (old) {
              std::ofstream nw(CFG_PER_MODE[0]);
              nw << old.rdbuf();
          }
      }
    }
    LoadConfig();
    { std::ifstream f("ceitus_rivals_placeid.cfg"); if (f) f >> g_rivalsPlaceId; }

    // hilo de red: ban check + version check (en background, no bloquea)
    std::thread(NetworkThread).detach();

    HWND rbxWnd = nullptr;

    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = WndProc; wc.lpszClassName = L"RbxMenu"; wc.hInstance = GetModuleHandleW(nullptr);
    RegisterClassExW(&wc);
    HWND menuWnd = CreateWindowExW(WS_EX_TOPMOST, L"RbxMenu", L"ceitus",
        WS_OVERLAPPEDWINDOW, 50, 50, 500, 780, nullptr, nullptr, wc.hInstance, nullptr);
    ShowWindow(menuWnd, SW_SHOW);

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = menuWnd;
    sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION, &sd, &g_chain, &g_dev, nullptr, &g_ctx);
    ID3D11Texture2D* buf = nullptr;
    g_chain->GetBuffer(0, IID_PPV_ARGS(&buf));
    g_dev->CreateRenderTargetView(buf, nullptr, &g_rtv); buf->Release();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io2 = ImGui::GetIO();
    io2.IniFilename = nullptr;
    io2.Fonts->AddFontDefault();
    // escala global de la UI
    ImGui::GetStyle().ScaleAllSizes(1.4f);
    io2.FontGlobalScale = 1.4f;
    ImGui_ImplWin32_Init(menuWnd);
    ImGui_ImplDX11_Init(g_dev, g_ctx);

    MSG msg{};
    while (msg.message != WM_QUIT) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg); DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) break;
        }

        // toggle del panel con la tecla configurable
        {
            static bool lastMenuKey = false;
            bool curMenuKey = (GetAsyncKeyState(menuKey) & 0x8000) != 0;
            if (curMenuKey && !lastMenuKey) {
                bool vis = IsWindowVisible(menuWnd) != 0;
                ShowWindow(menuWnd, vis ? SW_HIDE : SW_SHOW);
            }
            lastMenuKey = curMenuKey;
        }

        if (rbxWnd) {
            UpdateOverlayPos(rbxWnd);
            // forzar overlay siempre encima (por si Roblox lo tapa)
            if (g_overlay) SetWindowPos(g_overlay, HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }
        ClearOverlay();

        static DWORD dmFailSince = 0;
        {
            uintptr_t dm = GetDataModel();
            // si DM falla 3 segundos seguidos → re-attach al proceso nuevo (teleport)
            if (!dm) {
                if (!dmFailSince) dmFailSince = GetTickCount();
                else if (GetTickCount() - dmFailSince > 1000) {
                    if (mem.Attach(L"RobloxPlayerBeta.exe")) {
                        lastPs = 0; dmFailSince = 0;
                    }
                }
            } else { dmFailSince = 0; }
            uintptr_t ps = dm ? GetPlayersService(dm) : 0;
            if (ps) {
                lastPs = ps;
            } else if (lastPs) {
                // validar que lastPs sigue siendo un Players service real
                auto test = GetAllPlayers(lastPs);
                if (test.empty()) lastPs = 0; // stale, descartar
                ps = lastPs;
            }
            dbgDM = (dm != 0); dbgDMAddr = dm;
            dbgPS = (ps != 0); dbgPSAddr = ps;
            // leer PlaceId para auto-detección de modo
            if (dm) g_placeId = mem.Read<int64_t>(dm + Offsets::DataModel::PlaceId);
            if (ps) dbgPSChildren = (int)GetAllPlayers(ps).size(); else dbgPSChildren = 0;
            float vmSum = 0.f;
            for (int r=0;r<4;r++) for (int c=0;c<4;c++) vmSum += fabsf(lastVm.m[r][c]);
            dbgVmSum = vmSum;
            if (ps) {
                auto allP = GetAllPlayers(ps);
                dbgPlayers = (int)allP.size();
                // actualizar posición local siempre (para filtro de distancia del aimbot)
                uintptr_t lp = GetLocalPlayer(ps);
                if (lp) {
                    uintptr_t lchar = mem.Read<uintptr_t>(lp + Offsets::Player::ModelInstance);
                    if (lchar) {
                        uintptr_t lhum = FindFirstChild(lchar, "Humanoid");
                        if (lhum) {
                            uintptr_t lhrp = mem.Read<uintptr_t>(lhum + Offsets::Humanoid::HumanoidRootPart);
                            if (!lhrp) lhrp = FindFirstChild(lchar, "HumanoidRootPart");
                            if (lhrp) g_localPos = GetPartPosition(lhrp);
                        }
                    }
                }
            }
        }
        if (bESP) {
            float sw = (float)g_width;
            float sh = (float)g_height;
            ViewMatrix_t vm = lastVm;
            Vector3 localPos = g_cachedLocalPosESP;

            dbgValid = 0;
            g_radarEntries.clear();

            // anti-flash/anti-smoke usa punteros cacheados — sin RPM extra en render
            if (bAntiFlash || bAntiSmoke) {
                static const uintptr_t transpOffsets[] = { 0x174, 0x188, 0x19C, 0x1B0, 0x1C4 };
                uintptr_t lp = g_cachedLp;
                uintptr_t dm = g_cachedDm;
                if (bAntiFlash && lp) {
                    uintptr_t pgui = FindFirstChild(lp, "PlayerGui");
                    if (pgui) {
                        std::function<void(uintptr_t,int)> scanFlash = [&](uintptr_t node, int d) {
                            if (d > 4) return;
                            for (uintptr_t ch : GetChildren(node)) {
                                for (uintptr_t off : transpOffsets) {
                                    float val = mem.Read<float>(ch + off);
                                    if (val >= 0.f && val < 0.7f) { float one=1.f; WriteProcessMemory(mem.proc,(LPVOID)(ch+off),&one,4,nullptr); }
                                }
                                scanFlash(ch, d+1);
                            }
                        };
                        scanFlash(pgui, 0);
                    }
                }
                if (bAntiSmoke && dm) {
                    uintptr_t ws = FindFirstChild(dm, "Workspace");
                    if (ws) {
                        std::function<void(uintptr_t,int)> scanSmoke = [&](uintptr_t node, int d) {
                            if (d > 6) return;
                            std::string n = GetInstanceName(node);
                            if (n == "SmokeEmitter" || n == "Smoke" || n == "SmokeParticle") { uint8_t zero=0; WriteProcessMemory(mem.proc,(LPVOID)(node+0x188),&zero,1,nullptr); }
                            for (uintptr_t ch : GetChildren(node)) scanSmoke(ch, d+1);
                        };
                        scanSmoke(ws, 0);
                    }
                }
            }

            // snapshot del cache — posición se lee fresca desde HRP cada frame (2 RPM), sin FindFirstChild
            std::vector<CachedPlayerESP> snap;
            { std::lock_guard<std::mutex> lk(g_espMtx); snap = g_espCache; }

            for (auto& ce : snap) {
                // leer posición fresca desde HRP cacheado — 2 RPM, sin FindFirstChild
                Vector3 pos = ce.pos;
                if (ce.hrp) {
                    uintptr_t prim = mem.Read<uintptr_t>(ce.hrp + Offsets::BasePart::Primitive);
                    // validar que el primitive es un puntero Roblox real; si es basura → stale
                    if (prim > 0x10000000000ULL) {
                        Vector3 fp = mem.Read<Vector3>(prim + Offsets::Primitive::Position);
                        if (fabsf(fp.y) < 100000.f)
                            pos = fp;
                    }
                }
                Vector3 headPos = pos; headPos.y += 3.0f;
                Vector3 feetPos = pos; feetPos.y -= 3.0f;

                dbgValid++;
                if (dbgValid == 1) {
                    dbgEnemyHp = ce.health; dbgEnemyMaxHp = ce.maxHealth;
                }

                if (bRadar) {
                    RadarEntry re; re.dx = pos.x - localPos.x; re.dz = pos.z - localPos.z; re.col = ce.col;
                    g_radarEntries.push_back(re);
                }

                Vector2 headSc, feetSc, centerSc;
                bool headFront = WorldToScreen(vm, headPos, headSc, sw, sh);
                bool feetFront = WorldToScreen(vm, feetPos, feetSc, sw, sh);
                WorldToScreen(vm, pos, centerSc, sw, sh);
                if (!headFront && !feetFront) continue;

                if (bSkeleton) {
                    BoneLine(vm, ce.bones[0], ce.bones[1], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[1], ce.bones[2], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[1], ce.bones[4], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[4], ce.bones[5], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[5], ce.bones[6], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[1], ce.bones[7], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[7], ce.bones[8], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[8], ce.bones[9], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[2], ce.bones[10], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[10], ce.bones[11], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[11], ce.bones[12], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[2], ce.bones[13], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[13], ce.bones[14], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[14], ce.bones[15], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[0], ce.bones[16], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[16], ce.bones[17], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[16], ce.bones[18], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[16], ce.bones[19], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[16], ce.bones[20], sw, sh, ce.col);
                }

                if (!headFront && feetFront) headSc = { feetSc.x, feetSc.y - 80.f };
                if (headFront && !feetFront) feetSc = { headSc.x, headSc.y + 80.f };

                if (headFront || feetFront) {
                    float topY    = std::clamp(std::min(headSc.y, feetSc.y), 0.f, sh);
                    float bottomY = std::clamp(std::max(headSc.y, feetSc.y), 0.f, sh);
                    float visH    = bottomY - topY;
                    float midX    = (headSc.x + feetSc.x) * 0.5f;
                    float boxW    = std::max(visH * 0.38f, 4.f);
                    Vector2 tl    = { midX - boxW * 0.5f, topY };

                    if (visH >= 2.f) {
                        if (bBoxes) DrawBox(tl, boxW, visH, ce.col);
                        if (bHealthBar) {
                            float maxHp = ce.maxHealth > 0.f ? ce.maxHealth : 100.f;
                            float pct = std::clamp(ce.health / maxHp, 0.f, 1.f);
                            DrawHealthBar(tl, visH, pct);
                        }
                        if (bSnaplines && feetFront && OnScreen(feetSc, sw, sh))
                            DrawLine({ sw * 0.5f, sh }, feetSc, ce.col);
                    } else {
                        DrawDot({ midX, (topY + bottomY) * 0.5f }, 3, ce.col);
                    }
                    if (bNames) {
                        std::string label = ce.name;
                        if (bDistance) {
                            float dist = Dist3D(localPos, pos);
                            char dbuf[32]; snprintf(dbuf, sizeof(dbuf), " [%.0fm]", dist);
                            label += dbuf;
                        }
                        DrawText2D({ midX, topY - 16.f }, label, ce.col);
                    }
                } else if (OnScreen(centerSc, sw, sh)) {
                    DrawDot(centerSc, 3, ce.col);
                    if (bNames) {
                        std::string label = ce.name;
                        if (bDistance) {
                            float dist = Dist3D(localPos, pos);
                            char dbuf[32]; snprintf(dbuf, sizeof(dbuf), " [%.0fm]", dist);
                            label += dbuf;
                        }
                        DrawText2D({ centerSc.x - 20.f, centerSc.y - 12.f }, label, ce.col);
                    }
                }
            }

            // círculo de FOV dinámico: verde si hay target, blanco si no
            if (bAimbot) {
                COLORREF fovCol = g_cachedAimTarget ? RGB(0,220,80) : RGB(200,200,200);
                DrawEllipse({sw * 0.5f, sh * 0.5f}, fAimFov, fAimFov, fovCol);
            }

            // ---- radar ----
            if (bRadar) {
                float rr  = 100.f;
                float rcx = sw - rr - 15.f;
                float rcy = sh - rr - 15.f;
                DrawFilledCircleBG({rcx, rcy}, rr);
                DrawRadarCross({rcx, rcy}, rr);
                DrawDot({rcx, rcy}, 5, RGB(0,255,100));
                float scale = rr / fRadarRange;
                for (auto& e : g_radarEntries) {
                    float ex = rcx + e.dx * scale;
                    float ey = rcy - e.dz * scale;
                    float ddx = ex - rcx, ddy = ey - rcy;
                    float dd  = sqrtf(ddx*ddx + ddy*ddy);
                    if (dd > rr - 4.f) { float f = (rr-4.f)/dd; ex = rcx+ddx*f; ey = rcy+ddy*f; }
                    DrawDot({ex, ey}, 4, e.col);
                }
                DrawEllipse({rcx, rcy}, rr, rr, RGB(80,80,120));
            }
        }

        PresentOverlay();

        // ImGui menu
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // ---- estilo visual ----
        {
            ImGuiStyle& st = ImGui::GetStyle();
            st.WindowRounding  = 10.f; st.FrameRounding  = 6.f;
            st.GrabRounding    = 6.f;  st.ScrollbarRounding = 6.f;
            st.ItemSpacing     = ImVec2(8, 7);
            st.FramePadding    = ImVec2(8, 5);
            ImVec4* c = st.Colors;
            c[ImGuiCol_WindowBg]       = ImVec4(0.07f,0.07f,0.10f,0.97f);
            c[ImGuiCol_TitleBg]        = ImVec4(0.10f,0.06f,0.20f,1.f);
            c[ImGuiCol_TitleBgActive]  = ImVec4(0.16f,0.09f,0.32f,1.f);
            c[ImGuiCol_FrameBg]        = ImVec4(0.14f,0.14f,0.22f,1.f);
            c[ImGuiCol_FrameBgHovered] = ImVec4(0.22f,0.18f,0.36f,1.f);
            c[ImGuiCol_FrameBgActive]  = ImVec4(0.28f,0.22f,0.45f,1.f);
            c[ImGuiCol_CheckMark]      = ImVec4(0.45f,0.85f,1.f, 1.f);
            c[ImGuiCol_SliderGrab]     = ImVec4(0.45f,0.75f,1.f, 1.f);
            c[ImGuiCol_SliderGrabActive]= ImVec4(0.6f,0.9f,1.f,1.f);
            c[ImGuiCol_Button]         = ImVec4(0.18f,0.25f,0.50f,1.f);
            c[ImGuiCol_ButtonHovered]  = ImVec4(0.28f,0.40f,0.75f,1.f);
            c[ImGuiCol_ButtonActive]   = ImVec4(0.38f,0.55f,0.95f,1.f);
            c[ImGuiCol_Tab]            = ImVec4(0.12f,0.10f,0.20f,1.f);
            c[ImGuiCol_TabHovered]     = ImVec4(0.28f,0.22f,0.50f,1.f);
            c[ImGuiCol_TabSelected]    = ImVec4(0.22f,0.16f,0.42f,1.f);
            c[ImGuiCol_Header]         = ImVec4(0.20f,0.15f,0.38f,1.f);
            c[ImGuiCol_HeaderHovered]  = ImVec4(0.28f,0.22f,0.50f,1.f);
            c[ImGuiCol_SeparatorActive]= ImVec4(0.45f,0.35f,0.80f,1.f);
            c[ImGuiCol_ScrollbarBg]    = ImVec4(0.05f,0.05f,0.08f,1.f);
            c[ImGuiCol_ScrollbarGrab]  = ImVec4(0.25f,0.20f,0.45f,1.f);
        }

        // ---- pantalla de ban ----
        if (g_hwIdBanned) {
            ImGui::SetNextWindowSize({ 490, 200 }, ImGuiCond_Always);
            ImGui::SetNextWindowPos({ 0, 0 }, ImGuiCond_Always);
            ImGui::Begin("  ceitus", nullptr, ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse);
            ImGui::TextColored(ImVec4(1,0.2f,0.2f,1), "HWID BANEADO");
            ImGui::TextColored(ImVec4(0.7f,0.7f,0.7f,1), "Tu PC fue baneada. Contacta al soporte.");
            ImGui::Text("HWID: %s", HWIDString().c_str());
            ImGui::End();
            ImGui::Render();
            const float clr[4]{};
            g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
            g_ctx->ClearRenderTargetView(g_rtv, clr);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            g_chain->Present(0, 0); Sleep(16);
            continue;
        }

        // ---- pantalla de key ----
        if (g_loginState != LoginState::LoggedIn) {
            static char keyBuf[32] = {};
            static std::string keyMsg;
            static bool keyMsgBad = false;
            static bool checking = false;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  ImVec2(24, 20));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,    ImVec2(8, 10));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,   ImVec2(10, 8));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,  8.f);
            ImGui::PushStyleColor(ImGuiCol_WindowBg,       ImVec4(0.08f,0.08f,0.13f,1.f));
            ImGui::PushStyleColor(ImGuiCol_TitleBg,        ImVec4(0.08f,0.08f,0.13f,1.f));
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive,  ImVec4(0.08f,0.08f,0.13f,1.f));
            ImGui::PushStyleColor(ImGuiCol_Button,         ImVec4(0.12f,0.38f,0.22f,1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.18f,0.55f,0.32f,1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.25f,0.70f,0.42f,1.f));
            ImGui::PushStyleColor(ImGuiCol_FrameBg,        ImVec4(0.13f,0.13f,0.20f,1.f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.20f,0.20f,0.32f,1.f));
            ImGui::PushStyleColor(ImGuiCol_Border,         ImVec4(0.25f,0.25f,0.45f,0.7f));

            auto CenterText = [](const char* txt, ImVec4 col){
                float w = ImGui::CalcTextSize(txt).x;
                ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - w) * 0.5f + ImGui::GetCursorPosX());
                ImGui::TextColored(col, "%s", txt);
            };

            ImGui::SetNextWindowSize({500, 780}, ImGuiCond_Always);
            ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Always);
            ImGui::Begin("##keywin", nullptr,
                ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|
                ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoTitleBar);

            // centrar verticalmente (~280px de contenido)
            ImGui::Dummy({0, (780.f - 280.f) * 0.5f - 20.f});

            CenterText("ceitus", ImVec4(0.55f,0.75f,1.f,1.f));
            ImGui::Spacing();
            CenterText("Bienvenido", ImVec4(0.88f,0.88f,0.88f,1.f));
            ImGui::Spacing(); ImGui::Spacing();

            // linea decorativa
            {
                ImVec2 p = ImGui::GetCursorScreenPos();
                float lw = ImGui::GetContentRegionAvail().x;
                ImGui::GetWindowDrawList()->AddRectFilled({p.x,p.y},{p.x+lw,p.y+1}, IM_COL32(80,100,200,130));
                ImGui::Dummy({0,10});
            }

            CenterText("Ingresa tu key de acceso", ImVec4(0.55f,0.60f,0.72f,1.f));
            ImGui::Spacing(); ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f,0.65f,0.85f,1.f));
            ImGui::Text("Key:");
            ImGui::PopStyleColor();
            float copyBtnW = 60.f;
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - copyBtnW - 6.f);
            bool enterPressed = ImGui::InputText("##key", keyBuf, sizeof(keyBuf),
                ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::SameLine(0, 6);
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.15f,0.18f,0.30f,1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f,0.28f,0.46f,1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.30f,0.38f,0.60f,1.f));
            ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(0.75f,0.85f,1.f,1.f));
            if (ImGui::Button("Pegar", {copyBtnW, 0})) {
                if (OpenClipboard(nullptr)) {
                    HANDLE hData = GetClipboardData(CF_TEXT);
                    if (hData) {
                        char* pText = static_cast<char*>(GlobalLock(hData));
                        if (pText) {
                            strncpy_s(keyBuf, sizeof(keyBuf), pText, sizeof(keyBuf)-1);
                            GlobalUnlock(hData);
                        }
                    }
                    CloseClipboard();
                }
            }
            ImGui::PopStyleColor(4);
            ImGui::Spacing(); ImGui::Spacing();

            bool doActivate = enterPressed;
            if (checking) {
                CenterText("Verificando...", ImVec4(0.6f,0.7f,1.f,1.f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,1,1,1));
                if (ImGui::Button("Activar", {-1, 42})) doActivate = true;
                ImGui::PopStyleColor();
            }

            if (doActivate && !checking) {
                std::string k(keyBuf);
                for (auto& c : k) c = (char)toupper(c);
                if (k.size() < 18) { keyMsg="Key demasiado corta."; keyMsgBad=true; }
                else {
                    checking = true; keyMsg = ""; keyMsgBad = false;
                    std::thread([k](){
                        int days = CheckKeyOnline(k);
                        if (days >= 0) {
                            SaveLicense(k, GetHWID(), days);
                            g_activeKey  = k;
                            g_uid        = MakeUID(k, GetHWID());
                            g_loginState = LoginState::LoggedIn;
                        } else if (days == -2) {
                            keyMsg    = "Key ya canjeada por otro usuario.";
                            keyMsgBad = true;
                        } else {
                            keyMsg    = "Key invalida o no encontrada.";
                            keyMsgBad = true;
                        }
                        g_checking = false;
                        checking   = false;
                    }).detach();
                }
            }

            if (!keyMsg.empty()) {
                ImGui::Spacing();
                if (keyMsgBad) ImGui::TextColored(ImVec4(1,0.35f,0.35f,1), "  %s", keyMsg.c_str());
                else ImGui::TextColored(ImVec4(0.3f,1,0.5f,1), "  %s", keyMsg.c_str());
            }

            ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
            CenterText("v" CEITUS_VERSION, ImVec4(0.25f,0.25f,0.35f,1.f));

            ImGui::End();
            ImGui::PopStyleColor(9);
            ImGui::PopStyleVar(5);

            ImGui::Render();
            const float clr[4]{};
            g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
            g_ctx->ClearRenderTargetView(g_rtv, clr);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            g_chain->Present(0, 0); Sleep(16);
            continue;
        }

        // ---- una vez logueado, adjuntar Roblox y arrancar hilos ----
        {
            static bool robloxStarted = false;
            if (!robloxStarted) {
                robloxStarted = true;
                // esperar Roblox en hilo para no bloquear el render
                std::thread([&](){
                    while (!mem.Attach(L"RobloxPlayerBeta.exe"))
                        std::this_thread::sleep_for(std::chrono::seconds(2));
                    while (!rbxWnd) { rbxWnd = FindRobloxWindow(); std::this_thread::sleep_for(std::chrono::milliseconds(500)); }
                    StartOffsetUpdater(L"roblox");
                    std::thread(VMReaderThread).detach();
                    std::thread(AimbotThread).detach();
                    std::thread(ESPScanThread).detach();
                }).detach();
            }
            if (rbxWnd && !g_overlay) {
                CreateOverlay(rbxWnd);
            }
        }

        ImGui::SetNextWindowSize({ 490, 760 }, ImGuiCond_Always);
        ImGui::SetNextWindowPos({ 0, 0 }, ImGuiCond_Always);
        ImGui::Begin("  ceitus", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        // ---- selector de juego ----
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f,0.7f,1.f,1.f));
        ImGui::Text("JUEGO");
        ImGui::PopStyleColor();
        ImGui::SetNextItemWidth(-1);
        { // auto-detección por PlaceId — siempre activa, sobreescribe el modo guardado
          static int64_t lastAutoPlace = 0;
          if (g_placeId != 0 && g_placeId != lastAutoPlace) {
              lastAutoPlace = g_placeId;
              int detected = 0;
              if      (g_placeId == 286090429LL)    detected = 3; // Arsenal
              else if (g_placeId == 2626726391LL)   detected = 1; // Counterblox
              else if (g_placeId == 17625359962LL)  detected = 4; // Rivals (id conocido)
              else if (g_rivalsPlaceId != 0 && g_placeId == g_rivalsPlaceId) detected = 4; // Rivals (guardado)
              else                                  detected = 0; // juego desconocido → Auto
              int prev = gameMode;
              if (detected != prev) { SaveConfig(prev); LoadConfig(detected); gameMode = detected; }
          }
          int prev = gameMode;
          ImGui::Combo("##modo", &gameMode, gameModeNames, 5);
          if (gameMode != prev) { SaveConfig(prev); LoadConfig(gameMode); } }
        ImGui::Checkbox("Ignorar equipo", &bTeamFilter);
        ImGui::Spacing();

        // ---- tabs ----
        if (ImGui::BeginTabBar("##tabs")) {

            // ======= TAB: VISUAL =======
            if (ImGui::BeginTabItem("  Visual  ")) {
                ImGui::Spacing();
                ImGui::Checkbox("ESP activado", &bESP);
                if (bESP) {
                    ImGui::Indent(10.f);
                    ImGui::SetNextItemWidth(200.f);
                    static const char* espColors[] = { "Por equipo", "Rojo", "Azul", "Naranja", "Verde", "Multicolor (Rainbow)" };
                    ImGui::Combo("Color ESP", &espColorMode, espColors, 6);
                    ImGui::Spacing();
                    ImGui::Checkbox("Cajas",        &bBoxes);
                    ImGui::Checkbox("Barra de vida",&bHealthBar);
                    ImGui::Checkbox("Skeleton",     &bSkeleton);
                    ImGui::Checkbox("Nombres",      &bNames);
                    ImGui::Checkbox("Distancia",    &bDistance);
                    ImGui::Checkbox("Snaplines",    &bSnaplines);
                    ImGui::Unindent(10.f);
                }
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::Checkbox("Radar (mini-mapa)", &bRadar);
                if (bRadar) {
                    ImGui::Indent(10.f);
                    ImGui::SliderFloat("Rango (studs)", &fRadarRange, 50.f, 500.f, "%.0f");
                    ImGui::Unindent(10.f);
                }
                // opciones Counterblox
                if (gameMode == 1) {
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.7f,0.7f,1.f,1.f), "Counterblox");
                    ImGui::Checkbox("Anti-Flash", &bAntiFlash);
                    ImGui::Checkbox("Anti-Humo",  &bAntiSmoke);
                }
                ImGui::EndTabItem();
            }

            // ======= TAB: COMBATE =======
            if (ImGui::BeginTabItem("  Combate  ")) {
                ImGui::Spacing();
                // label dinámico de aimbot
                char aimbotLabel[48];
                if (aimbotKey == VK_XBUTTON1) snprintf(aimbotLabel,sizeof(aimbotLabel),"Aimbot [Mouse4]");
                else if (aimbotKey == VK_XBUTTON2) snprintf(aimbotLabel,sizeof(aimbotLabel),"Aimbot [Mouse5]");
                else if (aimbotKey == VK_MBUTTON)  snprintf(aimbotLabel,sizeof(aimbotLabel),"Aimbot [Mouse3]");
                else if (aimbotKey >= 'A' && aimbotKey <= 'Z') snprintf(aimbotLabel,sizeof(aimbotLabel),"Aimbot [%c]",(char)aimbotKey);
                else if (aimbotKey >= VK_F1 && aimbotKey <= VK_F12) snprintf(aimbotLabel,sizeof(aimbotLabel),"Aimbot [F%d]",aimbotKey-VK_F1+1);
                else snprintf(aimbotLabel,sizeof(aimbotLabel),"Aimbot [VK%d]",aimbotKey);
                ImGui::Checkbox(aimbotLabel, &bAimbot);
                if (bAimbot) {
                    ImGui::Indent(10.f);
                    ImGui::SliderFloat("FOV px",       &fAimFov,      30.f, 400.f, "%.0f");
                    ImGui::SliderFloat("Dist max (m)", &fAimMaxDist,  30.f, 1000000.f, "%.0f", ImGuiSliderFlags_Logarithmic);
                    ImGui::SliderFloat("Velocidad",    &fAimSmooth,    1.f,   80.f, "%.0f%%");
                    ImGui::Checkbox("Ignorar equipo (aimbot)", &bAimbotTeamFilter);
                    ImGui::Checkbox("Apuntar a todos (sin FOV)", &bAimbotNoFov);
                    // cambiar tecla
                    static bool waitingForKey = false;
                    static DWORD waitStartTime = 0;
                    if (waitingForKey) {
                        ImGui::TextColored(ImVec4(1,1,0,1), "Presiona tecla... (ESC cancela)");
                        if (GetTickCount() - waitStartTime > 300) {
                            int mouseVKs[] = { VK_XBUTTON1, VK_XBUTTON2, VK_MBUTTON };
                            for (int mv : mouseVKs)
                                if (GetAsyncKeyState(mv)&0x8000){aimbotKey=mv;waitingForKey=false;break;}
                            if (waitingForKey) for (int vk=0x08;vk<0xFE;vk++){
                                if (vk==VK_ESCAPE){if(GetAsyncKeyState(vk)&0x8000){waitingForKey=false;break;}continue;}
                                if (vk==VK_LBUTTON||vk==VK_RBUTTON) continue;
                                if (GetAsyncKeyState(vk)&0x8000){aimbotKey=vk;waitingForKey=false;break;}
                            }
                        }
                    } else {
                        char btnLabel[48];
                        snprintf(btnLabel,sizeof(btnLabel),"Cambiar tecla: %s",
                            aimbotKey==VK_XBUTTON1?"Mouse4":aimbotKey==VK_XBUTTON2?"Mouse5":
                            aimbotKey==VK_MBUTTON?"Mouse3":aimbotLabel+7);
                        if (ImGui::Button(btnLabel,ImVec2(-1,0))){waitingForKey=true;waitStartTime=GetTickCount();}
                    }
                    ImGui::Unindent(10.f);
                }
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::Checkbox("Triggerbot (auto-disparo)", &bTriggerbot);
                if (bTriggerbot) {
                    ImGui::Indent(10.f);
                    ImGui::SliderFloat("Radio disparo px", &fTriggerFov, 2.f, 40.f, "%.0f");
                    ImGui::Unindent(10.f);
                }
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::Checkbox("Prediccion de movimiento", &bPrediction);
                if (bPrediction) {
                    ImGui::Indent(10.f);
                    ImGui::SliderFloat("Tiempo (s)", &fPrediction, 0.01f, 0.3f, "%.2f");
                    ImGui::Unindent(10.f);
                }
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::Checkbox("Bunny Hop [Space]", &bBhop);
                ImGui::EndTabItem();
            }

            // ======= TAB: INFO =======
            if (ImGui::BeginTabItem("  Info  ")) {
                ImGui::Spacing();
                // UID del usuario en dorado
                if (!g_uid.empty()) {
                    ImGui::TextColored(ImVec4(1.f,0.82f,0.1f,1.f), "ID: %s", g_uid.c_str());
                    ImGui::Separator();
                }
                ImGui::Spacing();
                ImGui::TextColored(dbgDM?ImVec4(0.2f,1.f,0.4f,1.f):ImVec4(1.f,0.2f,0.2f,1.f),
                    "DataModel:  %s", dbgDM?"OK":"FAIL");
                ImGui::TextColored(dbgPS?ImVec4(0.2f,1.f,0.4f,1.f):ImVec4(1.f,0.2f,0.2f,1.f),
                    "Players:    %s", dbgPS?"OK":"FAIL");
                ImGui::TextColored(dbgVmSum>0.f?ImVec4(0.2f,1.f,0.4f,1.f):ImVec4(1.f,0.2f,0.2f,1.f),
                    "ViewMatrix: %.2f", dbgVmSum);
                ImGui::Separator();
                ImGui::Text("Jugadores detectados: %d", dbgPlayers);
                ImGui::Text("Visibles en pantalla: %d", dbgValid);
                ImGui::Text("Target aimbot: %s", g_cachedAimTarget?"LOCKED":"---");
                if (dbgEnemyHp >= 0.f) {
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(1.f,1.f,0.2f,1.f),
                        "HP enemigo: %.1f / %.1f", dbgEnemyHp, dbgEnemyMaxHp);
                    ImGui::TextColored(ImVec4(0.6f,0.6f,0.6f,1.f),
                        "Off Health: 0x%llX  MaxHP: 0x%llX",
                        Offsets::Humanoid::Health, Offsets::Humanoid::MaxHealth);
                }
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.9f,0.8f,0.2f,1.f),
                    "PlaceId actual: %lld", g_placeId);
                ImGui::TextColored(ImVec4(0.6f,0.6f,0.8f,1.f),
                    "Rivals PlaceId: %lld", g_rivalsPlaceId ? g_rivalsPlaceId : 17625359962LL);
                if (ImGui::Button("Guardar PlaceId actual como Rivals")) {
                    g_rivalsPlaceId = g_placeId;
                    std::ofstream f("ceitus_rivals_placeid.cfg");
                    if (f) f << g_rivalsPlaceId;
                }
                ImGui::TextColored(ImVec4(0.6f,0.6f,0.6f,1.f),
                    "DM:  %llX", dbgDMAddr);
                ImGui::TextColored(ImVec4(0.6f,0.6f,0.6f,1.f),
                    "PS:  %llX", dbgPSAddr);
                ImGui::TextColored(ImVec4(0.6f,0.6f,0.6f,1.f),
                    "Off: %s", g_offsetStatus.c_str());
                ImGui::Separator();
                ImGui::Spacing();
                // ---- version y update ----
                ImGui::TextColored(ImVec4(0.6f,0.6f,0.8f,1.f), "Version: " CEITUS_VERSION);
                ImGui::Text("HWID: %s", HWIDString().c_str());
                if (g_updateAvailable) {
                    ImGui::TextColored(ImVec4(0.2f,1.f,0.4f,1.f),
                        "Nueva version: %s", g_latestVersion.c_str());
                    if (ImGui::Button("Actualizar ahora", ImVec2(-1,0))) {
                        // descargar en background para no bloquear UI
                        std::thread([](){
                            char exePath[MAX_PATH];
                            GetModuleFileNameA(nullptr, exePath, MAX_PATH);
                            std::string newPath = std::string(exePath) + ".new";
                            if (URLDownloadToFileA(nullptr, URL_EXE, newPath.c_str(), 0, nullptr) == S_OK) {
                                // bat que reemplaza el exe y reinicia
                                std::string batPath = std::string(exePath) + "_upd.bat";
                                std::ofstream bat(batPath);
                                bat << "@echo off\r\ntimeout /t 2 /nobreak >nul\r\n"
                                    << "move /y \"" << newPath << "\" \"" << exePath << "\"\r\n"
                                    << "start \"\" \"" << exePath << "\"\r\n"
                                    << "del \"%~f0\"\r\n";
                                bat.close();
                                ShellExecuteA(nullptr,"open",batPath.c_str(),nullptr,nullptr,SW_HIDE);
                                ExitProcess(0);
                            }
                        }).detach();
                    }
                } else {
                    ImGui::TextColored(ImVec4(0.5f,0.5f,0.5f,1.f), "Version actualizada");
                }
                ImGui::Separator();
                ImGui::Spacing();
                // binder de tecla del panel
                static bool waitMenuKey = false;
                static DWORD waitMenuStart = 0;
                if (waitMenuKey) {
                    ImGui::TextColored(ImVec4(1,1,0,1), "Presiona tecla para el panel... (ESC cancela)");
                    if (GetTickCount() - waitMenuStart > 300) {
                        for (int vk = 0x08; vk < 0xFE; vk++) {
                            if (vk == VK_LBUTTON || vk == VK_RBUTTON) continue;
                            if (vk == VK_ESCAPE) { if (GetAsyncKeyState(vk)&0x8000) { waitMenuKey=false; break; } continue; }
                            if (GetAsyncKeyState(vk)&0x8000) { menuKey=vk; waitMenuKey=false; break; }
                        }
                    }
                } else {
                    char mkLabel[64];
                    snprintf(mkLabel, sizeof(mkLabel), "Tecla panel: 0x%02X", menuKey);
                    if (ImGui::Button(mkLabel, ImVec2(-1,0))) { waitMenuKey=true; waitMenuStart=GetTickCount(); }
                }
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
        ImGui::End();

        ImGui::Render();
        const float clear[4]{};
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_chain->Present(0, 0);
        Sleep(6); // cap ~144fps, reduce CPU en PCs lentas

        // guardar config cada 5 segundos
        static DWORD lastSave = 0;
        DWORD nowSave = GetTickCount();
        if (nowSave - lastSave > 5000) { SaveConfig(); lastSave = nowSave; }
    }

    SaveConfig();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    return 0;
}
