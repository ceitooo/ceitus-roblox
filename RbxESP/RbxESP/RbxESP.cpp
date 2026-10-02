#define NOMINMAX
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "comdlg32.lib")
#include "Security.hpp"
#include "Memory.hpp"
#include <commdlg.h>
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
#define CEITUS_VERSION "1.0.5"

// ---- URLs GitHub (reemplazar USER/REPO con tu repo real) ----
#define GITHUB_USER      "ceitooo"
#define GITHUB_REPO      "ceitus-roblox"
#define URL_VERSION      "https://raw.githubusercontent.com/" GITHUB_USER "/" GITHUB_REPO "/main/version.txt"
#define URL_MIN_VERSION  "https://raw.githubusercontent.com/" GITHUB_USER "/" GITHUB_REPO "/main/min_version.txt"
#define URL_BANNED_HWIDS "https://raw.githubusercontent.com/" GITHUB_USER "/" GITHUB_REPO "/main/banned.txt"
#define URL_EXE          "https://github.com/" GITHUB_USER "/" GITHUB_REPO "/releases/latest/download/RbxESP.exe"

// ---- salt interno para derivar keys (no cambiar después de distribuir) ----

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
    return SecurityCheck();
}

static std::string HWIDString() {
    uint32_t h = GetHWID();
    char buf[16];
    snprintf(buf, sizeof(buf), "%04X-%04X", (h >> 16) & 0xFFFF, h & 0xFFFF);
    return buf;
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

// compara versiones "X.Y.Z" — retorna true si a >= b
static bool VersionGTE(const std::string& a, const std::string& b) {
    int a0=0,a1=0,a2=0, b0=0,b1=0,b2=0;
    sscanf_s(a.c_str(), "%d.%d.%d", &a0, &a1, &a2);
    sscanf_s(b.c_str(), "%d.%d.%d", &b0, &b1, &b2);
    if (a0 != b0) return a0 > b0;
    if (a1 != b1) return a1 > b1;
    return a2 >= b2;
}

// ---- estado de update y ban ----
static std::atomic<bool> g_updateAvailable{ false };
static std::atomic<bool> g_hwIdBanned{ false };
static std::string       g_latestVersion;
static bool              g_showUpdatePopup = false;
static std::atomic<bool> g_canUpdate{ false }; // solo keys con "canUpdate":true en backend

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

static std::string LicensePath() { return ESTR("ceitus_license.cfg"); }

static std::string TodayStr() {
    time_t now = time(nullptr);
    struct tm t{}; localtime_s(&t, &now);
    char buf[16]; strftime(buf, sizeof(buf), "%Y-%m-%d", &t);
    return buf;
}

// Guardar key cifrada con XOR+HWID
static void SaveLicense(const std::string& key, uint32_t hwid, int days) {
    std::string plain = key + "\n" + std::to_string(hwid) + "\n" + std::to_string(days) + "\n" + TodayStr() + "\n";
    std::string enc = EncryptLicense(plain, hwid);
    std::ofstream f(LicensePath(), std::ios::binary);
    if (f) f.write(enc.data(), enc.size());
}

// Cargar y descifrar licencia
static bool LoadLicense(std::string& keyOut) {
    std::ifstream f(LicensePath(), std::ios::binary);
    if (!f) return false;
    std::string enc((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    if (enc.empty()) return false;
    std::string plain = DecryptLicense(enc, GetHWID());
    std::istringstream ss(plain);
    std::string k, redeemedAt; uint32_t hw = 0; int days = 0;
    if (!(ss >> k >> hw >> days >> redeemedAt)) return false;
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
    std::string body = ESTR("{\"action\":\"ceitus-verify\",\"key\":\"") + key + ESTR("\",\"hwid\":\"") + hwidStr + ESTR("\"}");
    std::wstring host = EWSTR("ceitotweaks-backend.vercel.app");
    std::wstring path = EWSTR("/api/owner-stats");
    std::string resp = HttpPostJSON(host.c_str(), path.c_str(), body);
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
    // key con permiso de actualizar (solo para la key oficial del owner)
    if (resp.find("\"canUpdate\":true") != std::string::npos)
        g_canUpdate = true;
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

    // ping al backend: registra este HWID apenas arranca el exe
    {
        std::string pingBody = ESTR("{\"action\":\"ceitus-ping\",\"hwid\":\"") + hwidStr + ESTR("\"}");
        std::wstring host = EWSTR("ceitotweaks-backend.vercel.app");
        std::wstring path = EWSTR("/api/owner-stats");
        HttpPostJSON(host.c_str(), path.c_str(), pingBody); // fire and forget
    }

    std::string banned = HttpGet(URL_BANNED_HWIDS);
    if (!banned.empty()) {
        std::istringstream ss(banned);
        std::string line;
        while (std::getline(ss, line)) {
            while (!line.empty() && (line.back()=='\r'||line.back()=='\n'||line.back()==' ')) line.pop_back();
            if (line == hwidStr) { g_hwIdBanned = true; break; }
        }
    }

    // solo recibir actualizaciones si esta version >= min_version requerida
    std::string minVer = HttpGet(URL_MIN_VERSION);
    bool eligible = minVer.empty() || VersionGTE(CEITUS_VERSION, minVer);

    if (eligible) {
        std::string latest = HttpGet(URL_VERSION);
        if (!latest.empty() && latest != CEITUS_VERSION) {
            g_latestVersion = latest;
            g_updateAvailable = true;
            g_showUpdatePopup = true;
        }
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
bool  bAimbotTeamFilter = false;
bool  bAimbotNoFov = false;
int   iAimBone = 0; // 0=Cabeza, 1=Cuello, 2=Pecho
bool  bPanicMode = false; // todo oculto
int   panicKey = VK_END; // tecla pánico por defecto: END
bool  bCrosshair = false;
int   iCrosshairStyle = 0; // 0=Cruz, 1=Punto, 2=Cruz+Punto
bool  bHitmarker = false;
bool  bKillfeed  = false;
bool  bChams     = false;
bool  bAntiAFK  = false;
bool  bUserFilter = false;
bool  bThirdPerson = false;
// nuevas características
bool  bNoclip          = false;
bool  bFly             = false;
float fFlySpeed        = 50.f;
bool  bWalkSpeed       = false;
float fWalkSpeed       = 16.f;
bool  bJumpPower       = false;
float fJumpPower       = 50.f;
bool  bGravity         = false;
float fGravity         = 196.2f;
bool  bVelocityESP     = false;
bool  bCornerBoxes     = false;
bool  bEspRangeEnabled = false;
float fEspMaxDist      = 500.f;
bool  bNoRecoil        = false;
float fNoRecoilStr     = 1.0f;
bool  bSilentAim       = false;
bool  bSpectatorList   = false;
static std::vector<std::string> g_spectatorList;
bool  bChatESP         = false;
struct ChatEntry { std::string name, msg; DWORD time; };
static std::vector<ChatEntry> g_chatLog;
static std::mutex g_chatMtx;

// ---- inyector de DLL ----
static char  g_dllPath[MAX_PATH] = "";
static std::string g_injectStatus;
static DWORD g_injectStatusTime = 0;

static bool InjectDLL(const char* dllPath) {
    // Abrir handle propio con permisos de inyección (mem.proc solo tiene VM_READ)
    HANDLE hProc = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
        FALSE, mem.pid);
    if (!hProc) {
        g_injectStatus = "Error: no se pudo abrir proceso (" + std::to_string(GetLastError()) + ")";
        g_injectStatusTime = GetTickCount();
        return false;
    }

    SIZE_T len = strlen(dllPath) + 1;
    LPVOID remote = VirtualAllocEx(hProc, nullptr, len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote) { CloseHandle(hProc); return false; }
    if (!WriteProcessMemory(hProc, remote, dllPath, len, nullptr)) {
        VirtualFreeEx(hProc, remote, 0, MEM_RELEASE); CloseHandle(hProc); return false;
    }
    HMODULE k32 = GetModuleHandleA("kernel32.dll");
    FARPROC loadLib = GetProcAddress(k32, "LoadLibraryA");
    HANDLE th = CreateRemoteThread(hProc, nullptr, 0,
        (LPTHREAD_START_ROUTINE)loadLib, remote, 0, nullptr);
    if (!th) {
        VirtualFreeEx(hProc, remote, 0, MEM_RELEASE); CloseHandle(hProc); return false;
    }
    WaitForSingleObject(th, 5000);
    CloseHandle(th);
    VirtualFreeEx(hProc, remote, 0, MEM_RELEASE);
    CloseHandle(hProc);
    return true;
} // solo mostrar ESP de usuarios específicos
std::vector<std::string> g_targetUsers; // lista de usernames objetivo
char  g_targetUserInput[64] = "";       // buffer para input de nuevo user

// hitmarker state
static DWORD g_hitmarkerTime = 0;
static const DWORD HITMARKER_DURATION = 300; // ms

// killfeed state
struct KillfeedEntry { std::string name; DWORD time; };
static std::vector<KillfeedEntry> g_killfeed;
static const DWORD KILLFEED_DURATION = 4000; // ms

// chams: track previous health to detect hits/kills
struct PlayerHealthTrack { uintptr_t player; float lastHealth; std::string name; };
static std::vector<PlayerHealthTrack> g_healthTrack;
static std::mutex g_healthMtx;

// ---- cache ESP: ESPScanThread actualiza, render solo lee snapshot ----
struct CachedPlayerESP {
    Vector3     pos, headPos, feetPos;
    Vector3     bones[21];
    Vector3     bonesPrev[21];
    bool        boneOk[21];
    uintptr_t   bonePtrs[21];
    uintptr_t   bonePrims[21]; // cached Primitive pointers — rarely change
    float       health, maxHealth;
    std::string name;
    COLORREF    col;
    uintptr_t   character;
    uintptr_t   hrp;
    uintptr_t   humanoid;
    DWORD       boneUpdateTime;
    Vector3     velocity;
    bool        hasChat;
    std::string chatMsg;
};
static std::vector<CachedPlayerESP> g_espCache;
static std::mutex                   g_espMtx;
static Vector3                      g_cachedLocalPosESP{};
static uintptr_t                    g_cachedLp = 0;
uintptr_t                    g_cachedDm = 0;

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
static HWND  g_rbxWnd = nullptr; // ventana de Roblox (global para threads)
ViewMatrix_t lastVm{};
uintptr_t    lastPs = 0;
uintptr_t    g_cachedAimTarget = 0; // target actual del aimbot (para FOV dinámico)
uintptr_t    g_cachedAimChar   = 0; // character del target (para snap indicator en overlay)
bool         bShowFov          = true; // mostrar círculo FOV independientemente del aimbot

// radar: lista de entradas para dibujar (llenada en ESP loop, leída en render)
struct RadarEntry { float dx, dz; COLORREF col; std::string name; float dist; };
static std::vector<RadarEntry> g_radarEntries;
static Vector3 g_localPos{};
static int64_t  g_placeId = 0;       // PlaceId del juego actual
static int64_t  g_rivalsPlaceId = 0; // PlaceId de Rivals (guardado por el usuario)

// reconexion manual
static std::atomic<bool> g_forceReattach{ false };
static std::atomic<bool> g_threadsStarted{ false };

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

static void DrawCornerBox(Vector2 tl, float w, float h, COLORREF col) {
    float cw = std::max(w * 0.22f, 5.f);
    float ch = std::max(h * 0.18f, 5.f);
    // top-left
    DrawLine(tl,                        {tl.x + cw, tl.y},        col);
    DrawLine(tl,                        {tl.x, tl.y + ch},        col);
    // top-right
    DrawLine({tl.x+w, tl.y},            {tl.x+w-cw, tl.y},        col);
    DrawLine({tl.x+w, tl.y},            {tl.x+w, tl.y+ch},        col);
    // bottom-left
    DrawLine({tl.x, tl.y+h},            {tl.x+cw, tl.y+h},        col);
    DrawLine({tl.x, tl.y+h},            {tl.x, tl.y+h-ch},        col);
    // bottom-right
    DrawLine({tl.x+w, tl.y+h},          {tl.x+w-cw, tl.y+h},      col);
    DrawLine({tl.x+w, tl.y+h},          {tl.x+w, tl.y+h-ch},      col);
}

static void BoneLine(const ViewMatrix_t& vm, Vector3 a, Vector3 b,
                     float sw, float sh, COLORREF col) {
    // skip si alguna posición es inválida (hueso no encontrado)
    if (a.x == 0.f && a.y == 0.f && a.z == 0.f) return;
    if (b.x == 0.f && b.y == 0.f && b.z == 0.f) return;
    Vector2 sa, sb;
    if (WorldToScreen(vm, a, sa, sw, sh) && WorldToScreen(vm, b, sb, sw, sh)) {
        DrawLine(sa, sb, RGB(0,0,0), 4);
        DrawLine(sa, sb, col, 2);
    }
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
    uintptr_t cachedUpperTorso = 0;
    uintptr_t cachedNeck     = 0; // simulated: between head and torso
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

        // ---- bunny hop: envía espacio al window de Roblox en el momento de tocar el suelo ----
        if (bBhop && (GetAsyncKeyState(VK_SPACE) & 0x8000)) {
            static uintptr_t bhopHRP = 0;
            static DWORD bhopScan = 0;
            static bool wasInAir = false;
            DWORD bnow = GetTickCount();
            if (bnow - bhopScan > 300 || !bhopHRP) {
                bhopScan = bnow;
                uintptr_t lp = g_cachedLp;
                if (lp) {
                    uintptr_t lchar = mem.Read<uintptr_t>(lp + Offsets::Player::ModelInstance);
                    if (lchar) {
                        uintptr_t lhum = FindFirstChild(lchar, "Humanoid");
                        bhopHRP = lhum ? mem.Read<uintptr_t>(lhum + Offsets::Humanoid::HumanoidRootPart) : 0;
                        if (!bhopHRP) bhopHRP = FindFirstChild(lchar, "HumanoidRootPart");
                    }
                }
            }
            bool onGround = false;
            if (bhopHRP) {
                uintptr_t prim = mem.Read<uintptr_t>(bhopHRP + Offsets::BasePart::Primitive);
                if (prim) {
                    float vy = mem.Read<float>(prim + Offsets::Primitive::AssemblyLinearVelocity + 4);
                    onGround = fabsf(vy) < 2.f;
                    lastHumState = (vy < -2.f) ? 1 : 0;
                }
            }
            // enviar salto al window de Roblox cuando toca el suelo (flanco bajada→suelo)
            if (onGround && wasInAir && g_rbxWnd) {
                PostMessage(g_rbxWnd, WM_KEYDOWN, VK_SPACE, 0x00390001);
                PostMessage(g_rbxWnd, WM_KEYUP,   VK_SPACE, 0xC0390001);
            }
            wasInAir = !onGround;
        } else { lastHumState = 0; }

        if (!bAimbot && !bTriggerbot) {
            hasSmooth = false; cachedPlayer = 0; g_cachedAimTarget = 0; g_cachedAimChar = 0; continue;
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
                cachedUpperTorso = 0;
                g_cachedAimTarget = 0; g_cachedAimChar = 0; lastHeadTime = 0; headVel = {}; lockedHRPY = 0.f;
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
                    g_cachedAimTarget = 0; g_cachedAimChar = 0; lastHeadTime = 0; headVel = {}; lockedHRPY = 0.f;
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

                    if (bUserFilter && !g_targetUsers.empty()) {
                        std::string pn = GetInstanceName(player);
                        bool found = false;
                        for (auto& u : g_targetUsers) if (_stricmp(pn.c_str(), u.c_str()) == 0) { found = true; break; }
                        if (!found) continue;
                    }

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
                g_cachedAimChar = cachedChar;
                cachedUpperTorso = cachedChar ? FindFirstChild(cachedChar, "UpperTorso") : 0;
                if (!cachedUpperTorso && cachedChar) cachedUpperTorso = FindFirstChild(cachedChar, "Torso");
                lastHeadTime = 0; headVel = {};
                // guardar Y del HRP para detectar si lo envían al spawn
                lockedHRPY = newHRP ? GetPartPosition(newHRP).y
                           : (newHP  ? GetPartPosition(newHP).y : 0.f);
            }
        }

        if (!targetValid && !cachedPlayer) { g_cachedAimTarget = 0; g_cachedAimChar = 0; continue; }
        g_cachedAimTarget = cachedPlayer;
        if (!cachedPlayer || !targetValid) continue; // no mover mouse si está fuera del FOV

        Vector3 headW{};
        if (iAimBone == 0) {
            // Cabeza
            if (cachedHeadPart) { headW = GetPartPosition(cachedHeadPart); headW.y += 0.3f; }
            else if (cachedHRP) { headW = GetPartPosition(cachedHRP); headW.y += 2.8f; }
            else continue;
        } else if (iAimBone == 1) {
            // Cuello (entre cabeza y torso)
            Vector3 hPos{}, tPos{};
            bool hOk = false, tOk = false;
            if (cachedHeadPart) { hPos = GetPartPosition(cachedHeadPart); hOk = true; }
            if (cachedUpperTorso) { tPos = GetPartPosition(cachedUpperTorso); tOk = true; }
            else if (cachedHRP) { tPos = GetPartPosition(cachedHRP); tOk = true; }
            if (hOk && tOk) { headW = { (hPos.x+tPos.x)*0.5f, (hPos.y+tPos.y)*0.5f, (hPos.z+tPos.z)*0.5f }; }
            else if (hOk) { headW = hPos; }
            else if (cachedHRP) { headW = GetPartPosition(cachedHRP); headW.y += 2.0f; }
            else continue;
        } else {
            // Pecho
            if (cachedUpperTorso) { headW = GetPartPosition(cachedUpperTorso); }
            else if (cachedHRP) { headW = GetPartPosition(cachedHRP); headW.y += 1.0f; }
            else continue;
        }
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
        static bool  skipNext = false; // espera 1 ciclo tras enviar para que la cámara actualice
        if (!keyHeld) {
            accumX = 0.f; accumY = 0.f; skipNext = false;
        }
        if (keyHeld) {
            if (dist < 1.5f) { accumX = 0.f; accumY = 0.f; skipNext = false; continue; }

            // skipNext: evita acumular doble movimiento antes de que Roblox actualice la cámara
            if (skipNext) { skipNext = false; continue; }

            // /300: 30% ≈ velocidad original, 60% = 2x más rápido, 100% = snap
            float gain = std::clamp(fAimSmooth / 300.f, 0.005f, 0.35f);

            // nunca mover más que la distancia restante (sin overshoot)
            float moveX = dx * gain;
            float moveY = dy * gain;
            if (fabsf(moveX) > fabsf(dx)) moveX = dx;
            if (fabsf(moveY) > fabsf(dy)) moveY = dy;

            accumX += moveX; accumY += moveY;
            LONG mx = (LONG)accumX, my = (LONG)accumY;
            accumX -= mx; accumY -= my;

            if (bSilentAim) {
                // silent aim: escribe Camera.CFrame directamente (sin mover el cursor)
                uintptr_t dmSA = GetDataModel();
                if (dmSA) {
                    uintptr_t wsSA = mem.Read<uintptr_t>(dmSA + Offsets::DataModel::Workspace);
                    if (!wsSA) wsSA = FindFirstChild(dmSA, "Workspace");
                    if (wsSA) {
                        uintptr_t camSA = mem.Read<uintptr_t>(wsSA + Offsets::Workspace::CurrentCamera);
                        if (camSA) {
                            Vector3 camPos = mem.Read<Vector3>(camSA + Offsets::Camera::Position);
                            float tx = headW.x - camPos.x, ty = headW.y - camPos.y, tz = headW.z - camPos.z;
                            float tLen = sqrtf(tx*tx + ty*ty + tz*tz);
                            if (tLen > 0.001f) {
                                tx /= tLen; ty /= tLen; tz /= tLen;
                                float lx = -tx, ly = -ty, lz = -tz;
                                float rx = lz, ry = 0.f, rz = -lx;
                                float rLen = sqrtf(rx*rx + ry*ry + rz*rz);
                                if (rLen > 0.001f) { rx/=rLen; ry/=rLen; rz/=rLen; }
                                float ux = ly*rz - lz*ry, uy = lz*rx - lx*rz, uz = lx*ry - ly*rx;
                                float mat[9] = { rx, ry, rz, ux, uy, uz, lx, ly, lz };
                                mem.WriteRaw(camSA + Offsets::Camera::CFrame, mat, sizeof(mat));
                            }
                        }
                    }
                }
            } else {
                if (mx || my) {
                    INPUT inp{}; inp.type = INPUT_MOUSE;
                    inp.mi.dwFlags = MOUSEEVENTF_MOVE;
                    inp.mi.dx = mx; inp.mi.dy = my;
                    SendInput(1, &inp, sizeof(INPUT));
                    skipNext = true; // dar 8ms para que Roblox procese el input
                }
            }
        }

        // no recoil: compensación downward cuando LMB presionado
        if (bNoRecoil && (GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
            static DWORD lastRecoilTick = 0;
            if (now - lastRecoilTick >= 8) {
                lastRecoilTick = now;
                LONG cy = (LONG)(fNoRecoilStr * 0.8f);
                if (cy > 0) {
                    INPUT inp{}; inp.type = INPUT_MOUSE;
                    inp.mi.dwFlags = MOUSEEVENTF_MOVE;
                    inp.mi.dx = 0; inp.mi.dy = cy;
                    SendInput(1, &inp, sizeof(INPUT));
                }
            }
        }
    }
    timeEndPeriod(1);
}

// Thread dedicado al fly/noclip a 4ms (250Hz) para ganarle a la física de Roblox (240Hz)
static void FlyNoclipThread() {
    timeBeginPeriod(1);
    while (true) {
        Sleep(4);
        if (!mem.proc) continue;
        uintptr_t lp = g_cachedLp;
        if (!lp) continue;
        uintptr_t lc = mem.Read<uintptr_t>(lp + Offsets::Player::ModelInstance);
        if (!lc || lc < 0x10000000000ULL) continue;

        uintptr_t lh = FindFirstChild(lc, "Humanoid");
        uintptr_t lhrp = lh ? mem.Read<uintptr_t>(lh + Offsets::Humanoid::HumanoidRootPart) : 0;
        if (!lhrp) lhrp = FindFirstChild(lc, "HumanoidRootPart");
        if (!lhrp) continue;
        uintptr_t prim = mem.Read<uintptr_t>(lhrp + Offsets::BasePart::Primitive);
        if (!prim || prim < 0x10000000000ULL) continue;

        {
            static bool  s_prevFly  = false;
            static DWORD s_flyStart = 0;
            static float s_curVelY  = 0.f;
            if (!s_prevFly && bFly) { s_flyStart = GetTickCount(); s_curVelY = 0.f; }
            s_prevFly = bFly;

            if (bFly) {
                Vector3 vel = mem.Read<Vector3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
                bool space   = (GetAsyncKeyState(VK_SPACE)  & 0x8000) != 0;
                bool shift   = (GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0;
                bool liftOff = (GetTickCount() - s_flyStart < 500);

                float targetY;
                if (space)        targetY = fFlySpeed;
                else if (shift)   targetY = -fFlySpeed;
                else if (liftOff) targetY = fFlySpeed * 0.6f;
                else              targetY = 0.f;

                // lerp suave: 30% del camino cada 4ms → natural sin saltos
                s_curVelY += (targetY - s_curVelY) * 0.30f;
                vel.y = s_curVelY;
                mem.Write(prim + Offsets::Primitive::AssemblyLinearVelocity, vel);
            }
        }

        if (bNoclip) {
            auto parts = GetChildren(lc);
            for (uintptr_t part : parts) {
                if (!part || part < 0x10000000000ULL) continue;
                uintptr_t pp = mem.Read<uintptr_t>(part + Offsets::BasePart::Primitive);
                if (!pp || pp < 0x10000000000ULL) continue;
                uint16_t flags = mem.Read<uint16_t>(pp + Offsets::Primitive::Flags);
                if (flags & (uint16_t)Offsets::PrimitiveFlags::CanCollide) {
                    flags &= ~(uint16_t)Offsets::PrimitiveFlags::CanCollide;
                    mem.Write(pp + Offsets::Primitive::Flags, flags);
                }
            }
            // Cancelar rebote de depenetración
            Vector3 vel = mem.Read<Vector3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
            if (fabsf(vel.x) > 30.f) vel.x = 0.f;
            if (fabsf(vel.z) > 30.f) vel.z = 0.f;
            mem.Write(prim + Offsets::Primitive::AssemblyLinearVelocity, vel);
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
    while (true) {
        Sleep(16); // ~60Hz
        uintptr_t dm = GetDataModel();
        dbgDM = (dm != 0); dbgDMAddr = dm;
        if (!bESP) {
            std::lock_guard<std::mutex> lk(g_espMtx);
            g_espCache.clear();
            continue;
        }
        uintptr_t ps = dm ? GetPlayersService(dm) : 0;
        if (ps) lastPs = ps;
        else ps = lastPs;
        dbgPS = (ps != 0); dbgPSAddr = ps;
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

                    // WalkSpeed / JumpPower
                    if (lh) {
                        if (bWalkSpeed) {
                            float cur = mem.Read<float>(lh + Offsets::Humanoid::WalkSpeed);
                            if (fabsf(cur - fWalkSpeed) > 0.5f)
                                mem.Write(lh + Offsets::Humanoid::WalkSpeed, fWalkSpeed);
                        }
                        if (bJumpPower) {
                            float cur = mem.Read<float>(lh + Offsets::Humanoid::JumpPower);
                            if (fabsf(cur - fJumpPower) > 0.5f)
                                mem.Write(lh + Offsets::Humanoid::JumpPower, fJumpPower);
                        }
                    }
                }
            }
        }

        // Gravity
        if (bGravity && dm) {
            uintptr_t ws = mem.Read<uintptr_t>(dm + Offsets::DataModel::Workspace);
            if (!ws) ws = FindFirstChild(dm, "Workspace");
            if (ws) {
                uintptr_t world = mem.Read<uintptr_t>(ws + Offsets::Workspace::World);
                if (world) {
                    float curG = mem.Read<float>(world + Offsets::World::Gravity);
                    if (fabsf(curG - fGravity) > 1.f)
                        mem.Write(world + Offsets::World::Gravity, fGravity);
                }
            }
        }

        g_cachedLocalPosESP = localPos;

        static DWORD lastBoneScan = 0;
        DWORD now = GetTickCount();
        bool doBoneLookup = bSkeleton && (now - lastBoneScan > 100);
        if (doBoneLookup) lastBoneScan = now;

        auto allP = GetAllPlayers(ps);
        std::vector<CachedPlayerESP> newCache;
        newCache.reserve(allP.size());

        std::vector<CachedPlayerESP> oldSnap;
        { std::lock_guard<std::mutex> lk(g_espMtx); oldSnap = g_espCache; }

        // detectar si hay más de 1 team (si solo hay 1, es lobby → no filtrar)
        bool multiTeam = false;
        if (bTeamFilter && localTeam > 0x10000000000ULL) {
            for (uintptr_t p : allP) {
                if (p == lp) continue;
                uintptr_t t = mem.Read<uintptr_t>(p + Offsets::Player::Team);
                if (t > 0x10000000000ULL && t != localTeam) { multiTeam = true; break; }
            }
        }

        for (uintptr_t player : allP) {
            if (player == lp) continue;
            PlayerInfo pi = ReadPlayer(player);
            if (!pi.valid || fabsf(pi.pos.y) > 1000000.f) continue;
            uintptr_t pTeam = mem.Read<uintptr_t>(player + Offsets::Player::Team);
            bool isTeammate = (localTeam > 0x10000000000ULL && pTeam > 0x10000000000ULL && pTeam == localTeam);
            if (bTeamFilter && multiTeam && isTeammate) continue;

            if (bUserFilter && !g_targetUsers.empty()) {
                bool found = false;
                for (auto& u : g_targetUsers) {
                    if (_stricmp(pi.name.c_str(), u.c_str()) == 0) { found = true; break; }
                }
                if (!found) continue;
            }

            // filtro de rango
            if (bEspRangeEnabled && fEspMaxDist > 0.f) {
                float d = Dist3D(localPos, pi.pos);
                if (d > fEspMaxDist) continue;
            }

            CachedPlayerESP ce{};
            ce.pos       = pi.pos;
            ce.health    = pi.health;
            ce.maxHealth = pi.maxHealth;
            ce.name      = pi.name;
            ce.character = pi.character;
            ce.hrp       = pi.hrp;
            ce.humanoid  = pi.humanoid;

            // velocidad del HRP
            if (bVelocityESP && pi.hrp) {
                uintptr_t prim = mem.Read<uintptr_t>(pi.hrp + Offsets::BasePart::Primitive);
                if (prim) ce.velocity = mem.Read<Vector3>(prim + Offsets::Primitive::AssemblyLinearVelocity);
            }

            // chat bubble: detectar BillboardGui en la Head del personaje
            if (bChatESP && pi.character) {
                uintptr_t head = FindFirstChild(pi.character, "Head");
                if (head) {
                    for (uintptr_t hc : GetChildren(head)) {
                        std::string hcn = GetInstanceName(hc);
                        if (hcn == "BillboardGui" || hcn.find("Chat") != std::string::npos || hcn.find("Bubble") != std::string::npos) {
                            ce.hasChat = true;
                            // intentar leer el texto (Frame > TextLabel)
                            for (uintptr_t frame : GetChildren(hc)) {
                                for (uintptr_t label : GetChildren(frame)) {
                                    std::string txt = mem.ReadRbxString(label + Offsets::TextLabel::Text);
                                    if (!txt.empty() && txt.size() < 200) {
                                        ce.chatMsg = txt;
                                        // agregar al log global si es nuevo
                                        std::lock_guard<std::mutex> lk(g_chatMtx);
                                        bool dup = false;
                                        for (auto& e : g_chatLog)
                                            if (e.name == pi.name && e.msg == txt) { dup = true; break; }
                                        if (!dup) {
                                            g_chatLog.push_back({pi.name, txt, GetTickCount()});
                                            if (g_chatLog.size() > 12) g_chatLog.erase(g_chatLog.begin());
                                        }
                                        break;
                                    }
                                }
                                if (!ce.chatMsg.empty()) break;
                            }
                            break;
                        }
                    }
                }
            }

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

            // copiar datos de huesos del snapshot anterior
            for (auto& old : oldSnap) {
                if (old.character == ce.character) {
                    memcpy(ce.bonePtrs, old.bonePtrs, sizeof(ce.bonePtrs));
                    memcpy(ce.bonePrims, old.bonePrims, sizeof(ce.bonePrims));
                    memcpy(ce.boneOk, old.boneOk, sizeof(ce.boneOk));
                    memcpy(ce.bones, old.bones, sizeof(ce.bones));
                    memcpy(ce.bonesPrev, old.bonesPrev, sizeof(ce.bonesPrev));
                    ce.boneUpdateTime = old.boneUpdateTime;
                    break;
                }
            }
            // FindFirstDescendant + cache Primitive pointers cada 100ms
            if (doBoneLookup && bSkeleton && pi.character) {
                for (int i = 0; i < 21; i++) {
                    uintptr_t part = FindFirstDescendant(pi.character, boneNames[i]);
                    if (part) {
                        ce.bonePtrs[i] = part;
                        ce.bonePrims[i] = mem.Read<uintptr_t>(part + Offsets::BasePart::Primitive);
                        ce.boneOk[i] = true;
                    } else {
                        ce.bonePtrs[i] = 0;
                        ce.bonePrims[i] = 0;
                        ce.boneOk[i] = false;
                    }
                }
            }
            // Primitive pointers se refrescan en FindFirstChild (cada 100ms)
            // posiciones frescas se leen en el render
            newCache.push_back(std::move(ce));
        }
        // detectar hits y kills comparando health con snapshot anterior
        if (bHitmarker || bKillfeed) {
            DWORD nowHK = GetTickCount();
            // hits: bajó la vida de un jugador que sigue vivo
            for (auto& ce : newCache) {
                for (auto& old : oldSnap) {
                    if (old.character != ce.character) continue;
                    if (old.health > 0.f && ce.health < old.health - 0.5f) {
                        if (bHitmarker) g_hitmarkerTime = nowHK;
                    }
                    break;
                }
            }
            // kills: jugador con vida > 0 que ya no aparece en el nuevo cache
            if (bKillfeed) {
                for (auto& old : oldSnap) {
                    if (old.health <= 0.f || old.name.empty()) continue;
                    bool still = false;
                    for (auto& ce : newCache)
                        if (ce.character == old.character) { still = true; break; }
                    if (!still) {
                        // evitar duplicados rápidos del mismo nombre
                        bool dup = false;
                        for (auto& kf : g_killfeed)
                            if (kf.name == old.name && nowHK - kf.time < 2000) { dup = true; break; }
                        if (!dup) g_killfeed.push_back({ old.name, nowHK });
                    }
                }
            }
        }
        // spectator list: jugadores sin character (muertos / spectating)
        if (bSpectatorList) {
            std::vector<std::string> specs;
            for (uintptr_t player : allP) {
                if (player == lp) continue;
                uintptr_t chr = mem.Read<uintptr_t>(player + Offsets::Player::ModelInstance);
                if (!chr || chr < 0x10000000000ULL) {
                    std::string nm = GetInstanceName(player);
                    if (!nm.empty()) specs.push_back(nm);
                }
            }
            std::lock_guard<std::mutex> lk(g_espMtx);
            g_spectatorList = std::move(specs);
            g_espCache = std::move(newCache);
        } else {
            std::lock_guard<std::mutex> lk(g_espMtx);
            g_espCache = std::move(newCache);
        }

        // debug info — actualizado desde scan thread, render solo lee
        dbgDM = (dm != 0); dbgDMAddr = dm;
        dbgPS = (ps != 0); dbgPSAddr = ps;
        dbgPSChildren = (int)allP.size();
        dbgPlayers = (int)allP.size();
        if (dm) g_placeId = mem.Read<int64_t>(dm + Offsets::DataModel::PlaceId);
        g_localPos = localPos;

        // tercera persona: escribe CameraType=0 (Classic) en el objeto Camera
        if (bThirdPerson && dm) {
            uintptr_t ws = mem.Read<uintptr_t>(dm + Offsets::DataModel::Workspace);
            if (!ws) ws = FindFirstChild(dm, "Workspace");
            if (ws) {
                uintptr_t cam = mem.Read<uintptr_t>(ws + Offsets::Workspace::CurrentCamera);
                if (cam) {
                    int32_t camType = mem.Read<int32_t>(cam + Offsets::Camera::CameraType);
                    if (camType != 0) { // 0 = Classic (tercera persona)
                        int32_t classic = 0;
                        mem.Write(cam + Offsets::Camera::CameraType, classic);
                    }
                }
            }
        }

        // re-attach si DM falla >1s (teleport detection)
        static DWORD dmFailSince = 0;

        // reconexion manual desde UI
        if (g_forceReattach.exchange(false)) {
            mem.Attach(L"RobloxPlayerBeta.exe");
            lastPs = 0; dmFailSince = 0;
        }
        if (!dm) {
            if (!dmFailSince) dmFailSince = GetTickCount();
            else if (GetTickCount() - dmFailSince > 1000) {
                if (mem.Attach(L"RobloxPlayerBeta.exe")) { lastPs = 0; dmFailSince = 0; }
            }
        } else { dmFailSince = 0; }

        // anti-flash / anti-smoke — runs at scan rate, NOT per render frame
        if (bAntiFlash || bAntiSmoke) {
            static const uintptr_t transpOffsets[] = { 0x174, 0x188, 0x19C, 0x1B0, 0x1C4 };
            if (bAntiFlash && lp) {
                uintptr_t pgui = FindFirstChild(lp, "PlayerGui");
                if (pgui) {
                    std::function<void(uintptr_t,int)> scanFlash = [&](uintptr_t node, int d) {
                        if (d > 4) return;
                        for (uintptr_t ch : GetChildren(node)) {
                            for (uintptr_t off : transpOffsets) {
                                float val = mem.Read<float>(ch + off);
                                if (val >= 0.f && val < 0.7f) { float one=1.f; mem.Write(ch+off, one); }
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
                        if (n == "SmokeEmitter" || n == "Smoke" || n == "SmokeParticle") { uint8_t zero=0; mem.Write(node+0x188, zero); }
                        for (uintptr_t ch : GetChildren(node)) scanSmoke(ch, d+1);
                    };
                    scanSmoke(ws, 0);
                }
            }
        }
    }
}

static const char* CFG_GLOBAL = "ceitus.cfg";
static const char* CFG_PER_MODE[] = { "ceitus_general.cfg", "ceitus_counterblox.cfg", "ceitus_duels.cfg", "ceitus_arsenal.cfg", "ceitus_rivals.cfg" };

static const char* VKName(int vk) {
    switch(vk) {
        case VK_INSERT: return "INSERT"; case VK_DELETE: return "DELETE";
        case VK_HOME:   return "HOME";   case VK_END:    return "END";
        case VK_PAUSE:  return "PAUSE";  case VK_NEXT:   return "PAGE DN";
        case VK_PRIOR:  return "PAGE UP";
        case VK_F1: return "F1"; case VK_F2: return "F2"; case VK_F3: return "F3";
        case VK_F4: return "F4"; case VK_F5: return "F5"; case VK_F6: return "F6";
        case VK_F7: return "F7"; case VK_F8: return "F8"; case VK_F9: return "F9";
        case VK_F10: return "F10"; case VK_F11: return "F11"; case VK_F12: return "F12";
        case VK_LBUTTON: return "MOUSE1"; case VK_RBUTTON: return "MOUSE2";
        case VK_MBUTTON: return "MOUSE3"; case VK_XBUTTON1: return "MOUSE4";
        case VK_XBUTTON2: return "MOUSE5"; case VK_SPACE: return "SPACE";
        case VK_SHIFT: return "SHIFT"; case VK_CONTROL: return "CTRL";
        case VK_MENU: return "ALT"; case VK_CAPITAL: return "CAPS";
        default:
            if (vk >= '0' && vk <= '9') { static char b[2]; b[0]=(char)vk; b[1]=0; return b; }
            if (vk >= 'A' && vk <= 'Z') { static char b[2]; b[0]=(char)vk; b[1]=0; return b; }
            static char hex[8]; snprintf(hex, sizeof(hex), "0x%02X", vk); return hex;
    }
}

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
      << bAimbotNoFov << "\n" << espColorMode << "\n"
      << iAimBone << "\n"
      << bCrosshair << "\n" << iCrosshairStyle << "\n"
      << bHitmarker << "\n" << bKillfeed << "\n" << bChams << "\n"
      << panicKey << "\n"
      << bAntiAFK << "\n"
      << bUserFilter << "\n"
      << (int)g_targetUsers.size() << "\n";
    for (auto& u : g_targetUsers) f << u << "\n";
    f << bThirdPerson << "\n"
      << bWalkSpeed   << "\n" << fWalkSpeed  << "\n"
      << bJumpPower   << "\n" << fJumpPower  << "\n"
      << bGravity     << "\n" << fGravity    << "\n"
      << bVelocityESP << "\n" << bCornerBoxes << "\n"
      << bEspRangeEnabled << "\n" << fEspMaxDist << "\n"
      << bNoRecoil    << "\n" << fNoRecoilStr << "\n"
      << bSilentAim   << "\n" << bSpectatorList << "\n"
      << bChatESP    << "\n"
      << bNoclip     << "\n"
      << bFly        << "\n" << fFlySpeed << "\n"
      << bShowFov    << "\n";
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
    if (!f.eof()) f >> iAimBone;
    if (iAimBone < 0 || iAimBone > 2) iAimBone = 0;
    if (!f.eof()) f >> bCrosshair;
    if (!f.eof()) f >> iCrosshairStyle;
    if (!f.eof()) f >> bHitmarker;
    if (!f.eof()) f >> bKillfeed;
    if (!f.eof()) f >> bChams;
    if (!f.eof()) f >> panicKey;
    if (!f.eof()) f >> bAntiAFK;
    if (!f.eof()) f >> bUserFilter;
    { int cnt = 0; if (!f.eof()) f >> cnt;
      g_targetUsers.clear();
      for (int i = 0; i < cnt && !f.eof(); i++) {
          std::string u; f >> u;
          if (!u.empty()) g_targetUsers.push_back(u);
      }
    }
    if (!f.eof()) f >> bThirdPerson;
    if (!f.eof()) f >> bWalkSpeed;
    if (!f.eof()) f >> fWalkSpeed;
    if (!f.eof()) f >> bJumpPower;
    if (!f.eof()) f >> fJumpPower;
    if (!f.eof()) f >> bGravity;
    if (!f.eof()) f >> fGravity;
    if (!f.eof()) f >> bVelocityESP;
    if (!f.eof()) f >> bCornerBoxes;
    if (!f.eof()) f >> bEspRangeEnabled;
    if (!f.eof()) f >> fEspMaxDist;
    if (!f.eof()) f >> bNoRecoil;
    if (!f.eof()) f >> fNoRecoilStr;
    if (!f.eof()) f >> bSilentAim;
    if (!f.eof()) f >> bSpectatorList;
    if (!f.eof()) f >> bChatESP;
    if (!f.eof()) f >> bNoclip;
    if (!f.eof()) f >> bFly;
    if (!f.eof()) f >> fFlySpeed;
    if (fFlySpeed < 5.f || fFlySpeed > 200.f) fFlySpeed = 50.f;
    if (!f.eof()) f >> bShowFov;
    // clamp para evitar valores absurdos de configs viejas
    if (fAimMaxDist < 1.f) fAimMaxDist = 200.f;
    if (fAimFov > 300.f || fAimFov < 5.f) fAimFov = 80.f;
    if (fAimSmooth > 100.f || fAimSmooth < 0.1f) fAimSmooth = 60.f;
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

    // ocultar thread del debugger
    HideThreadFromDebugger();
    // ErasePEHeader() removido: Windows 11 CFG necesita el PE header para crear threads

    // inicializar checksum del .text para detectar patches en runtime
    InitChecksum();

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
    // opciones que escriben en memoria del juego: siempre empiezan desactivadas
    bWalkSpeed = false; bJumpPower = false; bGravity = false; bNoclip = false; bFly = false;
    bThirdPerson = false; bSilentAim = false; bNoRecoil = false;
    bAntiFlash = false; bAntiSmoke = false;
    { std::ifstream f("ceitus_rivals_placeid.cfg"); if (f) f >> g_rivalsPlaceId; }

    // hilo de red: ban check + version check (en background, no bloquea)
    std::thread(NetworkThread).detach();

    HWND& rbxWnd = g_rbxWnd;

    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = WndProc; wc.lpszClassName = L"RbxMenu"; wc.hInstance = GetModuleHandleW(nullptr);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassExW(&wc);
    HWND menuWnd = CreateWindowExW(WS_EX_TOPMOST, L"RbxMenu", L"ceitus",
        WS_POPUP | WS_VISIBLE | WS_SYSMENU | WS_MINIMIZEBOX,
        80, 40, 530, 840, nullptr, nullptr, wc.hInstance, nullptr);
    // rounded corners on Windows 11
    {
        typedef HRESULT(WINAPI* pDwmSetWindowAttribute)(HWND,DWORD,LPCVOID,DWORD);
        HMODULE hDwm = LoadLibraryW(L"dwmapi.dll");
        if (hDwm) {
            auto fn = (pDwmSetWindowAttribute)GetProcAddress(hDwm,"DwmSetWindowAttribute");
            if (fn) { DWORD val = 2; fn(menuWnd, 33/*DWMWA_WINDOW_CORNER_PREFERENCE*/, &val, sizeof(val)); }
            FreeLibrary(hDwm);
        }
    }
    ShowWindow(menuWnd, SW_SHOW);

    // anti-screenshot: ventana invisible en OBS/capturas (WDA_EXCLUDEFROMCAPTURE = 0x11)
    typedef BOOL(WINAPI* pSWDA)(HWND, DWORD);
    HMODULE hUser = GetModuleHandleW(L"user32.dll");
    if (hUser) {
        auto fnSWDA = (pSWDA)GetProcAddress(hUser, "SetWindowDisplayAffinity");
        if (fnSWDA) fnSWDA(menuWnd, 0x11);
    }

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
    ImGui::GetStyle().ScaleAllSizes(1.35f);
    io2.FontGlobalScale = 1.35f;

    // dragging support for borderless window
    static HWND s_menuWnd = menuWnd;
    static bool s_dragging = false;
    static POINT s_dragStart{};

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

        // panic key: ocultar/mostrar todo
        {
            static bool lastPanicKey = false;
            bool curPanic = (GetAsyncKeyState(panicKey) & 0x8000) != 0;
            if (curPanic && !lastPanicKey) {
                bPanicMode = !bPanicMode;
                if (bPanicMode) {
                    ShowWindow(menuWnd, SW_HIDE);
                    if (g_overlay) ShowWindow(g_overlay, SW_HIDE);
                } else {
                    ShowWindow(menuWnd, SW_SHOW);
                    if (g_overlay) ShowWindow(g_overlay, SW_SHOW);
                }
            }
            lastPanicKey = curPanic;
        }
        // anti-AFK: escribe micro-velocidad en HRP (funciona sin foco, el servidor ve movimiento)
        if (bAntiAFK) {
            static DWORD lastAfk = 0;
            static int afkPhase = 0;
            DWORD nowAfk = GetTickCount();
            if (nowAfk - lastAfk > 55000) {
                lastAfk = nowAfk;
                uintptr_t glp = g_cachedLp;
                if (glp) {
                    uintptr_t lc = mem.Read<uintptr_t>(glp + Offsets::Player::ModelInstance);
                    if (lc) {
                        uintptr_t lh2 = FindFirstChild(lc, "Humanoid");
                        uintptr_t lhrp2 = lh2 ? mem.Read<uintptr_t>(lh2 + Offsets::Humanoid::HumanoidRootPart) : 0;
                        if (!lhrp2) lhrp2 = FindFirstChild(lc, "HumanoidRootPart");
                        if (lhrp2) {
                            uintptr_t prim = mem.Read<uintptr_t>(lhrp2 + Offsets::BasePart::Primitive);
                            if (prim) {
                                float vx = (afkPhase++ % 2 == 0) ? 1.f : -1.f;
                                Vector3 jiggle{vx, 0.f, 0.f};
                                mem.Write(prim + Offsets::Primitive::AssemblyLinearVelocity, jiggle);
                            }
                        }
                    }
                }
            }
        }

        // re-forzar topmost del menú cada 2s para que no quede detrás
        {
            static DWORD lastTopmost = 0;
            DWORD nowT = GetTickCount();
            if (nowT - lastTopmost > 2000) {
                lastTopmost = nowT;
                if (IsWindowVisible(menuWnd))
                    SetWindowPos(menuWnd, HWND_TOPMOST, 0, 0, 0, 0,
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            }
        }

        if (bPanicMode) {
            g_chain->Present(0, 0); Sleep(16);
            continue;
        }

        if (rbxWnd) {
            UpdateOverlayPos(rbxWnd);
        }
        if (g_overlay) SetWindowPos(g_overlay, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        ClearOverlay();

        // debug info — sin RPM, solo lee variables atómicas/cacheadas
        {
            float vmSum = 0.f;
            for (int r=0;r<4;r++) for (int c=0;c<4;c++) vmSum += fabsf(lastVm.m[r][c]);
            dbgVmSum = vmSum;
        }
        if (bESP) {
            float sw = (float)g_width;
            float sh = (float)g_height;
            ViewMatrix_t vm = lastVm;
            Vector3 localPos = g_cachedLocalPosESP;

            dbgValid = 0;
            g_radarEntries.clear();

            std::vector<CachedPlayerESP> snap;
            { std::lock_guard<std::mutex> lk(g_espMtx); snap = g_espCache; }

            if (snap.empty()) {
            }

            for (auto& ce : snap) {
                // salud fresca desde Humanoid cacheado — sin delay de scan
                if (bHealthBar && ce.humanoid > 0x10000000000ULL) {
                    float fh = mem.Read<float>(ce.humanoid + Offsets::Humanoid::Health);
                    if (fh > 0.f && fh <= 10000.f) ce.health = fh;
                    float fm = mem.Read<float>(ce.humanoid + Offsets::Humanoid::MaxHealth);
                    if (fm > 0.f && fm <= 10000.f) ce.maxHealth = fm;
                }
                // posición fresca desde HRP cacheado — 2 RPM rápidos, sin FindFirstChild
                Vector3 pos = ce.pos;
                if (ce.hrp) {
                    uintptr_t prim = mem.Read<uintptr_t>(ce.hrp + Offsets::BasePart::Primitive);
                    if (prim > 0x10000000000ULL) {
                        Vector3 fp = mem.Read<Vector3>(prim + Offsets::Primitive::Position);
                        if (fabsf(fp.y) < 100000.f) pos = fp;
                    }
                }
                Vector3 headPos = pos; headPos.y += 3.0f;
                Vector3 feetPos = pos; feetPos.y -= 3.0f;

                dbgValid++;
                if (dbgValid == 1) {
                    dbgEnemyHp = ce.health; dbgEnemyMaxHp = ce.maxHealth;
                }

                if (bRadar) {
                    float rdx = pos.x - localPos.x, rdz = pos.z - localPos.z;
                    float dist = sqrtf(rdx*rdx + rdz*rdz);
                    g_radarEntries.push_back({ rdx, rdz, ce.col, ce.name, dist });
                }

                Vector2 headSc, feetSc, centerSc;
                bool headFront = WorldToScreen(vm, headPos, headSc, sw, sh);
                bool feetFront = WorldToScreen(vm, feetPos, feetSc, sw, sh);
                WorldToScreen(vm, pos, centerSc, sw, sh);
                if (!headFront && !feetFront) continue;


                if (bSkeleton) {
                    // leer posiciones frescas usando bonePtrs
                    for (int bi = 0; bi < 21; bi++) {
                        if (ce.boneOk[bi] && ce.bonePtrs[bi]) {
                            Vector3 bp = GetPartPosition(ce.bonePtrs[bi]);
                            // solo filtrar posición nula (BoneLine también lo hace, doble seguridad)
                            if (!(bp.x==0.f && bp.y==0.f && bp.z==0.f))
                                ce.bones[bi] = bp;
                            else
                                ce.bones[bi] = ce.bones[bi]; // mantener último valor válido
                        }
                    }
                    // --- stickman style ---
                    // spine (más grueso)
                    BoneLine(vm, ce.bones[0], ce.bones[1], sw, sh, ce.col);  // head→upper torso
                    BoneLine(vm, ce.bones[1], ce.bones[2], sw, sh, ce.col);  // upper→lower torso
                    // brazos
                    BoneLine(vm, ce.bones[1], ce.bones[4], sw, sh, ce.col);  // shoulder L
                    BoneLine(vm, ce.bones[4], ce.bones[5], sw, sh, ce.col);  // upper arm L
                    BoneLine(vm, ce.bones[5], ce.bones[6], sw, sh, ce.col);  // lower arm L
                    BoneLine(vm, ce.bones[1], ce.bones[7], sw, sh, ce.col);  // shoulder R
                    BoneLine(vm, ce.bones[7], ce.bones[8], sw, sh, ce.col);  // upper arm R
                    BoneLine(vm, ce.bones[8], ce.bones[9], sw, sh, ce.col);  // lower arm R
                    // piernas
                    BoneLine(vm, ce.bones[2], ce.bones[10], sw, sh, ce.col); // hip L
                    BoneLine(vm, ce.bones[10], ce.bones[11], sw, sh, ce.col);// upper leg L
                    BoneLine(vm, ce.bones[11], ce.bones[12], sw, sh, ce.col);// lower leg L
                    BoneLine(vm, ce.bones[2], ce.bones[13], sw, sh, ce.col); // hip R
                    BoneLine(vm, ce.bones[13], ce.bones[14], sw, sh, ce.col);// upper leg R
                    BoneLine(vm, ce.bones[14], ce.bones[15], sw, sh, ce.col);// lower leg R
                    // R6 fallback
                    BoneLine(vm, ce.bones[0], ce.bones[16], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[16], ce.bones[17], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[16], ce.bones[18], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[16], ce.bones[19], sw, sh, ce.col);
                    BoneLine(vm, ce.bones[16], ce.bones[20], sw, sh, ce.col);

                    // cabeza: círculo estilo stickman
                    Vector2 headScBone;
                    if (ce.boneOk[0] && !(ce.bones[0].x == 0.f && ce.bones[0].y == 0.f && ce.bones[0].z == 0.f)) {
                        Vector3 headTop = ce.bones[0]; headTop.y += 0.6f;
                        Vector2 htSc;
                        if (WorldToScreen(vm, ce.bones[0], headScBone, sw, sh) &&
                            WorldToScreen(vm, headTop, htSc, sw, sh)) {
                            float headR = fabsf(headScBone.y - htSc.y);
                            if (headR < 2.f) headR = 2.f;
                            if (headR > 40.f) headR = 40.f;
                            // outline negro
                            DrawEllipse(headScBone, headR + 1.f, headR + 1.f, RGB(0, 0, 0));
                            // círculo color
                            DrawEllipse(headScBone, headR, headR, ce.col);
                        }
                    }

                    // dots en articulaciones (hombros, codos, rodillas, caderas)
                    static const int jointIdx[] = {1, 2, 4, 5, 7, 8, 10, 11, 13, 14};
                    for (int ji : jointIdx) {
                        Vector2 js;
                        if (ce.boneOk[ji] && !(ce.bones[ji].x == 0.f && ce.bones[ji].y == 0.f && ce.bones[ji].z == 0.f)
                            && WorldToScreen(vm, ce.bones[ji], js, sw, sh))
                            DrawDot(js, 2, ce.col);
                    }
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
                        // chams: fill solido + glow outline multicapa
                        if (bChams) {
                            COLORREF bc = ce.col;
                            BYTE r = GetRValue(bc), g2 = GetGValue(bc), b2 = GetBValue(bc);
                            // fill denso (cada 1px) con color oscuro
                            COLORREF darkFill = RGB(r/4, g2/4, b2/4);
                            HPEN fp = GetCachedPen(darkFill, 1);
                            HPEN oldPen = (HPEN)SelectObject(g_memDC, fp);
                            for (float fy = tl.y + 1.f; fy < tl.y + visH - 1.f; fy += 1.f) {
                                MoveToEx(g_memDC, (int)(tl.x+1), (int)fy, nullptr);
                                LineTo(g_memDC, (int)(tl.x+boxW-1), (int)fy);
                            }
                            SelectObject(g_memDC, oldPen);
                            // glow outlines: 4 capas (exterior oscuro → interior brillante)
                            struct { int pad; int thick; COLORREF col; } layers[] = {
                                { 6, 1, RGB(r/3, g2/3, b2/3) },
                                { 4, 1, RGB(r/2, g2/2, b2/2) },
                                { 2, 2, bc                    },
                                { 0, 3, RGB(std::min(255,(int)r+80), std::min(255,(int)g2+80), std::min(255,(int)b2+80)) },
                            };
                            HBRUSH oldBr = (HBRUSH)SelectObject(g_memDC, GetStockObject(NULL_BRUSH));
                            for (auto& lay : layers) {
                                HPEN lp = GetCachedPen(lay.col, lay.thick);
                                oldPen = (HPEN)SelectObject(g_memDC, lp);
                                ::Rectangle(g_memDC,
                                    (int)(tl.x-lay.pad), (int)(tl.y-lay.pad),
                                    (int)(tl.x+boxW+lay.pad), (int)(tl.y+visH+lay.pad));
                                SelectObject(g_memDC, oldPen);
                            }
                            SelectObject(g_memDC, oldBr);
                        }
                        if (bBoxes) {
                            if (bCornerBoxes) DrawCornerBox(tl, boxW, visH, ce.col);
                            else DrawBox(tl, boxW, visH, ce.col);
                        }
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
                    if (bChatESP && ce.hasChat) {
                        std::string chatLabel = ce.chatMsg.empty() ? "..." : ce.chatMsg;
                        if (chatLabel.size() > 30) chatLabel = chatLabel.substr(0, 30) + "..";
                        DrawText2D({ midX, topY - 28.f }, "[" + chatLabel + "]", RGB(255,240,100));
                    }
                    if (bVelocityESP) {
                        float spd = sqrtf(ce.velocity.x*ce.velocity.x + ce.velocity.z*ce.velocity.z);
                        char vbuf[32]; snprintf(vbuf, sizeof(vbuf), "%.0f m/s", spd);
                        float yOff = (bChatESP && ce.hasChat) ? -42.f : -28.f;
                        DrawText2D({ midX, topY + yOff }, vbuf, RGB(160,200,255));
                    }
                    if (bNames) {
                        std::string label = ce.name;
                        if (bDistance) {
                            float dist = Dist3D(localPos, pos);
                            char dbuf[32]; snprintf(dbuf, sizeof(dbuf), " [%.0fm]", dist);
                            label += dbuf;
                        }
                        // debug: contar huesos válidos
                        if (bSkeleton) {
                            int bc = 0; for (int i=0;i<21;i++) if(ce.boneOk[i]) bc++;
                            char bbuf[16]; snprintf(bbuf,sizeof(bbuf)," [b:%d]",bc);
                            label += bbuf;
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

            // spectator list: jugadores muertos / espectando
            if (bSpectatorList) {
                std::vector<std::string> specs;
                { std::lock_guard<std::mutex> lk(g_espMtx); specs = g_spectatorList; }
                if (!specs.empty()) {
                    float sx = sw - 175.f, sy = 10.f;
                    DrawText2D({sx, sy}, "Espectadores", RGB(180,180,180));
                    for (int i = 0; i < (int)specs.size() && i < 12; i++)
                        DrawText2D({sx, sy + 15.f*(i+1)}, specs[i], RGB(255,200,100));
                }
            }

            // panel de chat: mensajes recientes
            if (bChatESP) {
                std::vector<ChatEntry> log;
                { std::lock_guard<std::mutex> lk(g_chatMtx); log = g_chatLog; }
                if (!log.empty()) {
                    DWORD nowC = GetTickCount();
                    float cx = 10.f, cy = sh - 20.f;
                    for (int i = (int)log.size()-1; i >= 0 && cy > sh*0.5f; i--) {
                        if (nowC - log[i].time > 10000) break; // ocultar mensajes de más de 10s
                        std::string line = "[" + log[i].name + "] " + log[i].msg;
                        if (line.size() > 60) line = line.substr(0, 60) + "..";
                        DrawText2D({cx, cy}, line, RGB(255,240,100));
                        cy -= 15.f;
                    }
                }
            }

            // círculo de FOV: sigue el cursor (= centro pantalla en Roblox, cursor libre en otros juegos)
            if ((bAimbot && !bAimbotNoFov) || bShowFov) {
                POINT cur; GetCursorPos(&cur);
                float fcx = (float)cur.x, fcy = (float)cur.y;
                // fallback a centro si cursor parece fuera de pantalla
                if (fcx < 0 || fcx > sw || fcy < 0 || fcy > sh) { fcx = sw*0.5f; fcy = sh*0.5f; }

                bool locked = (g_cachedAimTarget != 0);
                COLORREF fovCol = locked ? RGB(0,220,80) : (bAimbot ? RGB(200,200,200) : RGB(120,120,120));
                DrawEllipse({fcx, fcy}, fAimFov, fAimFov, fovCol);

                // snap indicator: pequeño reticle en la cabeza del target cuando hay lock
                if (locked && g_cachedAimChar) {
                    uintptr_t ac = g_cachedAimChar;
                    for (auto& ce : snap) {
                        if (ce.character != ac) continue;
                        Vector2 hss;
                        if (WorldToScreen(vm, ce.headPos, hss, sw, sh)) {
                            DrawEllipse({hss.x, hss.y}, 12.f, 12.f, RGB(0,255,80));
                            // línea del centro al target
                            DrawLine({fcx, fcy}, {hss.x, hss.y}, RGB(0,200,60), 1);
                        }
                        break;
                    }
                }
            }

            // ---- radar mejorado ----
            if (bRadar) {
                float rr  = 110.f;
                float rcx = sw - rr - 20.f;
                float rcy = sh - rr - 20.f;

                // fondo con borde
                DrawFilledCircleBG({rcx, rcy}, rr);

                // anillos de distancia (25%, 50%, 75%)
                for (float frac : {0.25f, 0.5f, 0.75f})
                    DrawEllipse({rcx, rcy}, rr*frac, rr*frac, RGB(35,35,55));

                // cruz cardinal
                DrawRadarCross({rcx, rcy}, rr);

                // rotación según cámara (usar ViewMatrix forward)
                float camYaw = atan2f(vm.m[0][2], vm.m[0][0]);

                float scale = rr / fRadarRange;
                for (auto& e : g_radarEntries) {
                    // rotar según cámara
                    float cosY = cosf(-camYaw), sinY = sinf(-camYaw);
                    float rx = e.dx * cosY - e.dz * sinY;
                    float ry = e.dx * sinY + e.dz * cosY;

                    float ex = rcx + rx * scale;
                    float ey = rcy + ry * scale;

                    // clamp dentro del círculo
                    float ddx = ex - rcx, ddy = ey - rcy;
                    float dd  = sqrtf(ddx*ddx + ddy*ddy);
                    bool clamped = false;
                    if (dd > rr - 6.f) { float f = (rr-6.f)/dd; ex = rcx+ddx*f; ey = rcy+ddy*f; clamped = true; }

                    // tamaño del dot según distancia (más cerca = más grande)
                    int dotR = clamped ? 3 : (e.dist < 50.f ? 5 : 4);
                    DrawDot({ex, ey}, dotR, e.col);

                    // nombre si está cerca (<80 studs)
                    if (!clamped && e.dist < 80.f && !e.name.empty()) {
                        // nombre chiquito debajo del dot
                        SetBkMode(g_memDC, TRANSPARENT);
                        SetTextColor(g_memDC, e.col);
                        int len = (int)e.name.size();
                        SIZE sz{};
                        GetTextExtentPoint32A(g_memDC, e.name.c_str(), len, &sz);
                        TextOutA(g_memDC, (int)(ex - sz.cx/2), (int)(ey + dotR + 1), e.name.c_str(), len);
                    }
                }

                // borde exterior
                DrawEllipse({rcx, rcy}, rr, rr, RGB(80,100,160));

                // jugador local (triángulo apuntando arriba)
                DrawDot({rcx, rcy}, 4, RGB(0,255,100));
                // flecha arriba indicando "frente"
                DrawLine({rcx, rcy-7}, {rcx-4, rcy-2}, RGB(0,255,100), 2);
                DrawLine({rcx, rcy-7}, {rcx+4, rcy-2}, RGB(0,255,100), 2);

                // texto "RADAR" arriba
                DrawText2D({rcx, rcy - rr - 12.f}, "RADAR", RGB(80,100,160));
            }
        }

        // ---- crosshair personalizado ----
        if (bCrosshair) {
            float cx = (float)g_width * 0.5f, cy = (float)g_height * 0.5f;
            if (iCrosshairStyle == 0 || iCrosshairStyle == 2) {
                // cruz con outline
                DrawLine({cx-8,cy},{cx+8,cy}, RGB(0,0,0), 3);
                DrawLine({cx,cy-8},{cx,cy+8}, RGB(0,0,0), 3);
                DrawLine({cx-7,cy},{cx+7,cy}, RGB(0,255,100), 1);
                DrawLine({cx,cy-7},{cx,cy+7}, RGB(0,255,100), 1);
            }
            if (iCrosshairStyle == 1 || iCrosshairStyle == 2) {
                DrawDot({cx, cy}, 2, RGB(255,50,50));
            }
        }

        // ---- hitmarker ----
        if (bHitmarker && g_hitmarkerTime) {
            DWORD elapsed = GetTickCount() - g_hitmarkerTime;
            if (elapsed < HITMARKER_DURATION) {
                float cx = (float)g_width * 0.5f, cy = (float)g_height * 0.5f;
                float sz = 8.f;
                int alpha = 255 - (int)(255.f * elapsed / HITMARKER_DURATION);
                (void)alpha; // GDI no soporta alpha, usamos blanco que se desvanece no se puede, lo dejamos fijo
                COLORREF hcol = RGB(255, 255, 255);
                DrawLine({cx-sz,cy-sz},{cx-3,cy-3}, RGB(0,0,0), 3);
                DrawLine({cx+sz,cy-sz},{cx+3,cy-3}, RGB(0,0,0), 3);
                DrawLine({cx-sz,cy+sz},{cx-3,cy+3}, RGB(0,0,0), 3);
                DrawLine({cx+sz,cy+sz},{cx+3,cy+3}, RGB(0,0,0), 3);
                DrawLine({cx-sz,cy-sz},{cx-3,cy-3}, hcol, 2);
                DrawLine({cx+sz,cy-sz},{cx+3,cy-3}, hcol, 2);
                DrawLine({cx-sz,cy+sz},{cx-3,cy+3}, hcol, 2);
                DrawLine({cx+sz,cy+sz},{cx+3,cy+3}, hcol, 2);
            } else {
                g_hitmarkerTime = 0;
            }
        }

        // ---- killfeed ----
        if (bKillfeed) {
            DWORD now = GetTickCount();
            // limpiar entradas viejas
            g_killfeed.erase(std::remove_if(g_killfeed.begin(), g_killfeed.end(),
                [now](const KillfeedEntry& e) { return now - e.time > KILLFEED_DURATION; }),
                g_killfeed.end());
            float ky = 20.f;
            for (auto& kf : g_killfeed) {
                std::string txt = "ELIMINADO: " + kf.name;
                DrawText2D({(float)g_width - 150.f, ky}, txt, RGB(255, 60, 60));
                ky += 20.f;
            }
        }

        PresentOverlay();

        // ImGui menu
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // ---- estilo visual premium ----
        {
            ImGuiStyle& st = ImGui::GetStyle();
            st.WindowRounding    = 10.f;  st.FrameRounding    = 6.f;
            st.GrabRounding      = 6.f;   st.ScrollbarRounding = 6.f;
            st.TabRounding       = 6.f;   st.ChildRounding     = 6.f;
            st.PopupRounding     = 8.f;
            st.WindowBorderSize  = 0.f;   st.FrameBorderSize   = 0.f;
            st.ItemSpacing       = ImVec2(8, 7);
            st.FramePadding      = ImVec2(10, 6);
            st.WindowPadding     = ImVec2(16, 12);
            st.ScrollbarSize     = 8.f;
            st.GrabMinSize       = 8.f;
            st.TabBarBorderSize  = 0.f;
            ImVec4* c = st.Colors;
            c[ImGuiCol_WindowBg]         = ImVec4(0.05f,0.05f,0.08f,1.f);
            c[ImGuiCol_ChildBg]          = ImVec4(0.07f,0.07f,0.11f,1.f);
            c[ImGuiCol_PopupBg]          = ImVec4(0.07f,0.07f,0.11f,0.98f);
            c[ImGuiCol_Border]           = ImVec4(0.18f,0.18f,0.28f,0.6f);
            c[ImGuiCol_TitleBg]          = ImVec4(0.05f,0.05f,0.08f,1.f);
            c[ImGuiCol_TitleBgActive]    = ImVec4(0.05f,0.05f,0.08f,1.f);
            c[ImGuiCol_FrameBg]          = ImVec4(0.10f,0.10f,0.17f,1.f);
            c[ImGuiCol_FrameBgHovered]   = ImVec4(0.14f,0.14f,0.24f,1.f);
            c[ImGuiCol_FrameBgActive]    = ImVec4(0.18f,0.18f,0.30f,1.f);
            c[ImGuiCol_CheckMark]        = ImVec4(0.30f,0.85f,0.55f,1.f);
            c[ImGuiCol_SliderGrab]       = ImVec4(0.35f,0.60f,1.f,1.f);
            c[ImGuiCol_SliderGrabActive] = ImVec4(0.50f,0.75f,1.f,1.f);
            c[ImGuiCol_Button]           = ImVec4(0.13f,0.15f,0.26f,1.f);
            c[ImGuiCol_ButtonHovered]    = ImVec4(0.20f,0.26f,0.50f,1.f);
            c[ImGuiCol_ButtonActive]     = ImVec4(0.28f,0.36f,0.65f,1.f);
            c[ImGuiCol_Tab]              = ImVec4(0.07f,0.07f,0.12f,1.f);
            c[ImGuiCol_TabHovered]       = ImVec4(0.16f,0.20f,0.36f,1.f);
            c[ImGuiCol_TabSelected]      = ImVec4(0.12f,0.16f,0.30f,1.f);
            c[ImGuiCol_Header]           = ImVec4(0.12f,0.14f,0.24f,1.f);
            c[ImGuiCol_HeaderHovered]    = ImVec4(0.18f,0.22f,0.38f,1.f);
            c[ImGuiCol_HeaderActive]     = ImVec4(0.24f,0.28f,0.48f,1.f);
            c[ImGuiCol_Separator]        = ImVec4(0.14f,0.14f,0.22f,0.8f);
            c[ImGuiCol_SeparatorHovered] = ImVec4(0.28f,0.40f,0.70f,0.9f);
            c[ImGuiCol_SeparatorActive]  = ImVec4(0.38f,0.55f,1.f,1.f);
            c[ImGuiCol_ScrollbarBg]      = ImVec4(0.03f,0.03f,0.06f,1.f);
            c[ImGuiCol_ScrollbarGrab]    = ImVec4(0.18f,0.18f,0.32f,1.f);
            c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.26f,0.26f,0.46f,1.f);
            c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.33f,0.33f,0.55f,1.f);
            c[ImGuiCol_Text]             = ImVec4(0.90f,0.90f,0.94f,1.f);
            c[ImGuiCol_TextDisabled]     = ImVec4(0.38f,0.38f,0.48f,1.f);
        }

        // ---- pantalla de ban ----
        if (g_hwIdBanned) {
            ImGui::SetNextWindowSize({530, 840}, ImGuiCond_Always);
            ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Always);
            ImGui::Begin("##ban", nullptr,
                ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoTitleBar);
            ImDrawList* bdl = ImGui::GetWindowDrawList();
            ImVec2 bwp = ImGui::GetWindowPos(), bws = ImGui::GetWindowSize();
            bdl->AddRectFilled(bwp, {bwp.x+bws.x, bwp.y+3}, IM_COL32(220,50,50,255));
            ImGui::Dummy({0, bws.y * 0.35f});
            auto CenterBan = [](const char* t, ImVec4 col){
                float w = ImGui::CalcTextSize(t).x;
                ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - w)*0.5f + ImGui::GetCursorPosX());
                ImGui::TextColored(col, "%s", t);
            };
            CenterBan("ACCESO DENEGADO", ImVec4(1,0.25f,0.25f,1));
            ImGui::Spacing();
            CenterBan("Tu PC fue baneada.", ImVec4(0.65f,0.65f,0.72f,1));
            CenterBan("Contacta al soporte.", ImVec4(0.50f,0.50f,0.60f,1));
            ImGui::Spacing(); ImGui::Spacing();
            char hwidBuf[32]; snprintf(hwidBuf,sizeof(hwidBuf),"HWID: %s",HWIDString().c_str());
            CenterBan(hwidBuf, ImVec4(0.35f,0.35f,0.45f,1));
            ImGui::End();
            ImGui::Render();
            const float clr[4]{};
            g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
            g_ctx->ClearRenderTargetView(g_rtv, clr);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            g_chain->Present(0, 0); Sleep(1);
            continue;
        }

        // ---- pantalla de key ----
        if (g_loginState != LoginState::LoggedIn) {
            static char keyBuf[32] = {};
            static std::string keyMsg;
            static bool keyMsgBad = false;
            static bool checking = false;

            auto CenterText = [](const char* txt, ImVec4 col){
                float w = ImGui::CalcTextSize(txt).x;
                ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - w) * 0.5f + ImGui::GetCursorPosX());
                ImGui::TextColored(col, "%s", txt);
            };

            ImGui::SetNextWindowSize({530, 840}, ImGuiCond_Always);
            ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Always);
            ImGui::Begin("##keywin", nullptr,
                ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|
                ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoScrollbar);

            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 wp = ImGui::GetWindowPos();
            ImVec2 ws = ImGui::GetWindowSize();

            // gradient background top
            dl->AddRectFilledMultiColor(wp, {wp.x+ws.x, wp.y+220},
                IM_COL32(20,25,60,255), IM_COL32(15,20,50,255),
                IM_COL32(15,15,25,255), IM_COL32(15,15,25,255));

            // accent line at top
            dl->AddRectFilled(wp, {wp.x+ws.x, wp.y+3}, IM_COL32(80,140,255,200));

            // drag area
            {
                ImVec2 mpos = ImGui::GetMousePos();
                bool inDrag = (mpos.x >= wp.x && mpos.x <= wp.x+ws.x && mpos.y >= wp.y && mpos.y <= wp.y+60);
                if (inDrag && ImGui::IsMouseClicked(0) && !s_dragging) {
                    s_dragging = true; GetCursorPos(&s_dragStart);
                    RECT wr; GetWindowRect(s_menuWnd, &wr);
                    s_dragStart.x -= wr.left; s_dragStart.y -= wr.top;
                }
                if (s_dragging && ImGui::IsMouseDown(0)) {
                    POINT cur; GetCursorPos(&cur);
                    SetWindowPos(s_menuWnd, nullptr, cur.x-s_dragStart.x, cur.y-s_dragStart.y, 0, 0,
                        SWP_NOSIZE|SWP_NOZORDER);
                }
                if (!ImGui::IsMouseDown(0)) s_dragging = false;
            }

            // close button
            {
                ImVec2 cpos = {wp.x+ws.x-36, wp.y+8};
                ImVec2 mpos = ImGui::GetMousePos();
                bool hov = (mpos.x>=cpos.x && mpos.x<=cpos.x+26 && mpos.y>=cpos.y && mpos.y<=cpos.y+26);
                dl->AddRectFilled(cpos, {cpos.x+26,cpos.y+26},
                    hov ? IM_COL32(200,60,60,200) : IM_COL32(60,60,80,150), 6.f);
                dl->AddLine({cpos.x+7,cpos.y+7},{cpos.x+19,cpos.y+19}, IM_COL32(220,220,220,220), 2.f);
                dl->AddLine({cpos.x+19,cpos.y+7},{cpos.x+7,cpos.y+19}, IM_COL32(220,220,220,220), 2.f);
                if (hov && ImGui::IsMouseClicked(0)) PostQuitMessage(0);
            }

            ImGui::Dummy({0, 100});

            // logo
            ImGui::PushFont(ImGui::GetFont());
            float oldScale = ImGui::GetFont()->Scale;
            ImGui::GetFont()->Scale = 2.2f;
            ImGui::PushFont(ImGui::GetFont());
            CenterText("CEITUS", ImVec4(0.45f,0.70f,1.f,1.f));
            ImGui::PopFont();
            ImGui::GetFont()->Scale = oldScale;
            ImGui::PopFont();

            ImGui::Spacing();
            CenterText("Roblox External", ImVec4(0.45f,0.50f,0.65f,1.f));
            ImGui::Dummy({0, 40});

            // card container
            float cardW = 380.f;
            float padX = (ImGui::GetContentRegionAvail().x - cardW) * 0.5f;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + padX);
            ImGui::BeginChild("##logincard", {cardW, 320}, false, ImGuiWindowFlags_NoScrollbar);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0,0,0,0));

            // card background
            ImVec2 cp = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(
                {cp.x-10, cp.y-10}, {cp.x+cardW+10, cp.y+320},
                IM_COL32(18,18,30,240), 12.f);
            ImGui::GetWindowDrawList()->AddRect(
                {cp.x-10, cp.y-10}, {cp.x+cardW+10, cp.y+320},
                IM_COL32(60,80,160,80), 12.f, 0, 1.f);

            ImGui::Dummy({0, 20});
            CenterText("Ingresa tu key de acceso", ImVec4(0.55f,0.60f,0.72f,1.f));
            ImGui::Dummy({0, 16});

            ImGui::TextColored(ImVec4(0.50f,0.55f,0.70f,1.f), "KEY");
            ImGui::Spacing();
            float pasteW = 70.f;
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - pasteW - 8.f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 10));
            bool enterPressed = ImGui::InputText("##key", keyBuf, sizeof(keyBuf),
                ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::PopStyleVar();
            ImGui::SameLine(0, 8);
            ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.12f,0.14f,0.26f,1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered,  ImVec4(0.18f,0.22f,0.40f,1.f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive,   ImVec4(0.24f,0.30f,0.55f,1.f));
            if (ImGui::Button("Pegar", {pasteW, 0})) {
                if (OpenClipboard(nullptr)) {
                    HANDLE hData = GetClipboardData(CF_TEXT);
                    if (hData) {
                        char* pText = static_cast<char*>(GlobalLock(hData));
                        if (pText) { strncpy_s(keyBuf, sizeof(keyBuf), pText, sizeof(keyBuf)-1); GlobalUnlock(hData); }
                    }
                    CloseClipboard();
                }
            }
            ImGui::PopStyleColor(3);
            ImGui::Dummy({0, 12});

            bool doActivate = enterPressed;
            if (checking) {
                // animated dots
                int dots = (GetTickCount()/400) % 4;
                char loadTxt[24]; snprintf(loadTxt, sizeof(loadTxt), "Verificando%.*s", dots, "...");
                CenterText(loadTxt, ImVec4(0.45f,0.65f,1.f,1.f));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button,       ImVec4(0.20f,0.45f,0.85f,1.f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f,0.55f,0.95f,1.f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.35f,0.60f,1.f,1.f));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
                if (ImGui::Button("Activar", {-1, 0})) doActivate = true;
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(3);
            }

            if (doActivate && !checking) {
                std::string k(keyBuf);
                for (auto& ch : k) ch = (char)toupper(ch);
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
                if (keyMsgBad) ImGui::TextColored(ImVec4(1,0.35f,0.35f,1), "%s", keyMsg.c_str());
                else ImGui::TextColored(ImVec4(0.3f,1,0.5f,1), "%s", keyMsg.c_str());
            }

            ImGui::PopStyleColor();
            ImGui::EndChild();

            ImGui::Dummy({0, 20});
            CenterText("v" CEITUS_VERSION, ImVec4(0.22f,0.22f,0.32f,1.f));

            ImGui::End();

            ImGui::Render();
            const float clr[4]{};
            g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
            g_ctx->ClearRenderTargetView(g_rtv, clr);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            g_chain->Present(0, 0); Sleep(1);
            continue;
        }

        // ---- una vez logueado, adjuntar Roblox y arrancar hilos ----
        {
            static bool robloxStarted = false;
            if (!robloxStarted) {
                robloxStarted = true;
                // esperar Roblox y arrancar hilos — la ventana se busca en paralelo
                std::thread([&](){
                    while (!mem.Attach(L"RobloxPlayerBeta.exe"))
                        std::this_thread::sleep_for(std::chrono::seconds(2));
                    // hilos arrancan tan pronto como attach ok (no esperan la ventana)
                    if (!g_threadsStarted.exchange(true)) {
                        StartOffsetUpdater(L"roblox");
                        std::thread(VMReaderThread).detach();
                        std::thread(AimbotThread).detach();
                        std::thread(FlyNoclipThread).detach();
                        std::thread(ESPScanThread).detach();
                    }
                    // seguir buscando la ventana en segundo plano para el overlay
                    std::thread([&](){
                        while (!rbxWnd) { rbxWnd = FindRobloxWindow(); std::this_thread::sleep_for(std::chrono::milliseconds(500)); }
                    }).detach();
                }).detach();
            }
            if (rbxWnd && !g_overlay) {
                CreateOverlay(rbxWnd);
            }
        }

        ImGui::SetNextWindowSize({530, 840}, ImGuiCond_Always);
        ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Always);
        ImGui::Begin("##main", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 wp = ImGui::GetWindowPos();
        ImVec2 ws = ImGui::GetWindowSize();

        // header gradient - deeper, richer
        dl->AddRectFilledMultiColor(wp, {wp.x+ws.x, wp.y+72},
            IM_COL32(14,18,48,255), IM_COL32(10,14,38,255),
            IM_COL32(12,12,20,255), IM_COL32(12,12,20,255));
        // accent line top - cyan-blue gradient
        dl->AddRectFilledMultiColor(wp, {wp.x+ws.x, wp.y+3},
            IM_COL32(50,180,255,255), IM_COL32(80,140,255,255),
            IM_COL32(80,140,255,255), IM_COL32(50,180,255,255));
        // subtle bottom border on header
        dl->AddRectFilled({wp.x, wp.y+72}, {wp.x+ws.x, wp.y+73}, IM_COL32(40,50,90,180));

        // drag area
        {
            ImVec2 mpos = ImGui::GetMousePos();
            bool inDrag = (mpos.x >= wp.x && mpos.x <= wp.x+ws.x && mpos.y >= wp.y && mpos.y <= wp.y+70);
            if (inDrag && ImGui::IsMouseClicked(0) && !s_dragging) {
                s_dragging = true; GetCursorPos(&s_dragStart);
                RECT wr; GetWindowRect(s_menuWnd, &wr);
                s_dragStart.x -= wr.left; s_dragStart.y -= wr.top;
            }
            if (s_dragging && ImGui::IsMouseDown(0)) {
                POINT cur; GetCursorPos(&cur);
                SetWindowPos(s_menuWnd, nullptr, cur.x-s_dragStart.x, cur.y-s_dragStart.y, 0,0,
                    SWP_NOSIZE|SWP_NOZORDER);
            }
            if (!ImGui::IsMouseDown(0)) s_dragging = false;
        }

        // close / minimize buttons
        {
            // minimize
            ImVec2 mpos2 = {wp.x+ws.x-70, wp.y+8};
            ImVec2 mp = ImGui::GetMousePos();
            bool hMin = (mp.x>=mpos2.x && mp.x<=mpos2.x+26 && mp.y>=mpos2.y && mp.y<=mpos2.y+26);
            dl->AddRectFilled(mpos2, {mpos2.x+26,mpos2.y+26},
                hMin ? IM_COL32(80,80,120,200) : IM_COL32(50,50,70,150), 6.f);
            dl->AddLine({mpos2.x+6,mpos2.y+13},{mpos2.x+20,mpos2.y+13}, IM_COL32(200,200,220,220), 2.f);
            if (hMin && ImGui::IsMouseClicked(0)) ShowWindow(s_menuWnd, SW_MINIMIZE);
            // close
            ImVec2 cpos = {wp.x+ws.x-36, wp.y+8};
            bool hCl = (mp.x>=cpos.x && mp.x<=cpos.x+26 && mp.y>=cpos.y && mp.y<=cpos.y+26);
            dl->AddRectFilled(cpos, {cpos.x+26,cpos.y+26},
                hCl ? IM_COL32(200,60,60,200) : IM_COL32(50,50,70,150), 6.f);
            dl->AddLine({cpos.x+7,cpos.y+7},{cpos.x+19,cpos.y+19}, IM_COL32(220,220,220,220), 2.f);
            dl->AddLine({cpos.x+19,cpos.y+7},{cpos.x+7,cpos.y+19}, IM_COL32(220,220,220,220), 2.f);
            if (hCl && ImGui::IsMouseClicked(0)) PostQuitMessage(0);
        }

        // header title
        ImGui::SetCursorPos({18, 16});
        ImGui::PushFont(ImGui::GetFont());
        float oldS = ImGui::GetFont()->Scale;
        ImGui::GetFont()->Scale = 1.6f;
        ImGui::PushFont(ImGui::GetFont());
        ImGui::TextColored(ImVec4(0.40f,0.80f,1.f,1.f), "CEITUS");
        ImGui::PopFont();
        ImGui::GetFont()->Scale = oldS;
        ImGui::PopFont();
        ImGui::SameLine(0, 6);
        ImGui::SetCursorPosY(24);
        ImGui::TextColored(ImVec4(0.35f,0.40f,0.58f,1.f), "v" CEITUS_VERSION);

        // status pill
        {
            bool attached = (mem.proc != nullptr); // conectado si tenemos handle a Roblox
            // pill background
            ImVec2 pb = {wp.x + ws.x - 220, wp.y + 20};
            ImVec2 pe = {pb.x + 112,          pb.y + 22};
            dl->AddRectFilled(pb, pe,
                attached ? IM_COL32(20,55,30,200) : IM_COL32(55,20,20,200), 11.f);
            dl->AddRect(pb, pe,
                attached ? IM_COL32(40,180,80,120) : IM_COL32(180,40,40,120), 11.f);
            // dot
            ImVec2 dot = {pb.x + 14, pb.y + 11};
            dl->AddCircleFilled(dot, 4.5f, attached ? IM_COL32(50,220,100,255) : IM_COL32(220,60,60,255));
            // text
            ImVec2 tp = {pb.x + 24, pb.y + 4};
            dl->AddText(tp, attached ? IM_COL32(80,220,120,230) : IM_COL32(220,90,90,230),
                attached ? "Conectado" : "Sin Roblox");

            // player count badge (if attached)
            if (attached && dbgPlayers > 0) {
                char pc[16]; snprintf(pc, sizeof(pc), "%d jugadores", dbgPlayers);
                ImVec2 pp = {wp.x + ws.x - 220, wp.y + 48};
                dl->AddText(pp, IM_COL32(80,100,150,180), pc);
            }

            // boton Conectar cuando no está adjunto
            if (!attached) {
                ImGui::SetCursorPos({wp.x + ws.x - 216 - wp.x, 48});
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.50f,0.18f,0.18f,0.85f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.72f,0.22f,0.22f,0.95f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.90f,0.28f,0.28f,1.f));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
                if (ImGui::Button("Conectar##pill", {108, 20})) {
                    g_forceReattach = true;
                    // si los hilos no arrancaron aún (Roblox no estaba al inicio), intentar attach directo
                    if (!g_threadsStarted) {
                        std::thread([](){
                            if (mem.Attach(L"RobloxPlayerBeta.exe")) {
                                if (!g_threadsStarted.exchange(true)) {
                                    StartOffsetUpdater(L"roblox");
                                    std::thread(VMReaderThread).detach();
                                    std::thread(AimbotThread).detach();
                                    std::thread(FlyNoclipThread).detach();
                                    std::thread(ESPScanThread).detach();
                                }
                                std::thread([](){
                                    while (!g_rbxWnd) { g_rbxWnd = FindRobloxWindow(); Sleep(500); }
                                }).detach();
                            }
                        }).detach();
                    }
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(3);
            }
        }

        ImGui::SetCursorPos({16, 80});

        // helper: section header con barra de acento lateral
        auto SecHeader = [](ImDrawList* _dl, const char* label, ImU32 barCol = IM_COL32(60,160,255,220)) {
            ImGui::Dummy({0, 3});
            ImVec2 p = ImGui::GetCursorScreenPos();
            _dl->AddRectFilled(p, {p.x+3, p.y+15}, barCol, 2.f);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 9);
            ImGui::TextColored(ImVec4(0.55f,0.72f,1.f,1.f), "%s", label);
            ImGui::Dummy({0, 2});
        };

        // game selector section
        ImGui::BeginChild("##content", {ws.x - 32, ws.y - 92}, false);

        SecHeader(dl, "MODO DE JUEGO");
        ImGui::SetNextItemWidth(-1);
        { static int64_t lastAutoPlace = 0;
          if (g_placeId != 0 && g_placeId != lastAutoPlace) {
              lastAutoPlace = g_placeId;
              int detected = 0;
              if      (g_placeId == 286090429LL)    detected = 3;
              else if (g_placeId == 2626726391LL)   detected = 1;
              else if (g_placeId == 17625359962LL)  detected = 4;
              else if (g_rivalsPlaceId != 0 && g_placeId == g_rivalsPlaceId) detected = 4;
              else                                  detected = 0;
              int prev = gameMode;
              if (detected != prev) { SaveConfig(prev); LoadConfig(detected); gameMode = detected; }
          }
          int prev = gameMode;
          ImGui::Combo("##modo", &gameMode, gameModeNames, 5);
          if (gameMode != prev) { SaveConfig(prev); LoadConfig(gameMode); } }
        ImGui::Checkbox("Ignorar equipo", &bTeamFilter);
        ImGui::Checkbox("Solo usuarios especificos", &bUserFilter);
        if (bUserFilter) {
            ImGui::Indent(10.f);
            ImGui::InputText("##adduser", g_targetUserInput, sizeof(g_targetUserInput));
            ImGui::SameLine();
            if (ImGui::Button("Agregar") && g_targetUserInput[0]) {
                bool dup = false;
                for (auto& u : g_targetUsers) if (_stricmp(u.c_str(), g_targetUserInput) == 0) { dup = true; break; }
                if (!dup) g_targetUsers.push_back(g_targetUserInput);
                g_targetUserInput[0] = '\0';
            }
            for (int i = 0; i < (int)g_targetUsers.size(); i++) {
                ImGui::Text("%s", g_targetUsers[i].c_str());
                ImGui::SameLine();
                char btnId[32]; snprintf(btnId, sizeof(btnId), "X##del%d", i);
                if (ImGui::SmallButton(btnId)) { g_targetUsers.erase(g_targetUsers.begin()+i); i--; }
            }
            if (g_targetUsers.empty()) ImGui::TextDisabled("Sin usuarios, se muestran todos");
            ImGui::Unindent(10.f);
        }
        ImGui::Spacing();

        // ---- tabs ----
        ImGui::PushStyleColor(ImGuiCol_Tab,         ImVec4(0.07f,0.07f,0.12f,1.f));
        ImGui::PushStyleColor(ImGuiCol_TabHovered,  ImVec4(0.18f,0.22f,0.38f,1.f));
        ImGui::PushStyleColor(ImGuiCol_TabSelected, ImVec4(0.12f,0.18f,0.34f,1.f));
        if (ImGui::BeginTabBar("##tabs")) {

            // ======= TAB: VISUAL =======
            if (ImGui::BeginTabItem(" Visual ")) {
                ImGui::Spacing();
                SecHeader(dl, "ESP");
                ImGui::Checkbox("ESP activado", &bESP);
                if (bESP) {
                    ImGui::Indent(12.f);
                    ImGui::SetNextItemWidth(230.f);
                    static const char* espColors[] = { "Por equipo", "Rojo", "Azul", "Naranja", "Verde", "Rainbow" };
                    ImGui::Combo("Color##ec", &espColorMode, espColors, 6);
                    ImGui::Spacing();
                    // 2-column grid para opciones ESP
                    ImGui::BeginTable("##espopts", 2, ImGuiTableFlags_None, {0, 0});
                    ImGui::TableNextColumn(); ImGui::Checkbox("Cajas",        &bBoxes);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Nombres",      &bNames);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Barra de vida",&bHealthBar);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Distancia",    &bDistance);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Skeleton",     &bSkeleton);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Snaplines",    &bSnaplines);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Chams (glow)", &bChams);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Velocidad",    &bVelocityESP);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Corner Boxes", &bCornerBoxes);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Espectadores", &bSpectatorList);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Chat ESP",     &bChatESP);
                    ImGui::EndTable();
                    ImGui::Spacing();
                    ImGui::Checkbox("Filtro por rango##esprange", &bEspRangeEnabled);
                    if (bEspRangeEnabled) {
                        ImGui::Indent(12.f);
                        ImGui::SetNextItemWidth(210.f);
                        ImGui::SliderFloat("Dist max (m)##espr", &fEspMaxDist, 50.f, 2000.f, "%.0f");
                        ImGui::Unindent(12.f);
                    }
                    ImGui::Unindent(12.f);
                }
                ImGui::Spacing();
                SecHeader(dl, "RADAR", IM_COL32(80,200,120,220));
                ImGui::Checkbox("Radar (mini-mapa)", &bRadar);
                if (bRadar) {
                    ImGui::Indent(12.f);
                    ImGui::SetNextItemWidth(210.f);
                    ImGui::SliderFloat("Rango (studs)", &fRadarRange, 50.f, 500.f, "%.0f");
                    ImGui::Unindent(12.f);
                }
                if (gameMode == 1) {
                    ImGui::Spacing();
                    SecHeader(dl, "COUNTERBLOX", IM_COL32(255,160,40,220));
                    ImGui::TextColored(ImVec4(1.f,0.75f,0.2f,0.9f), "Escribe en memoria del juego — bajo tu propio riesgo");
                    ImGui::Spacing();
                    ImGui::BeginTable("##cbopts", 2, ImGuiTableFlags_None, {0, 0});
                    ImGui::TableNextColumn(); ImGui::Checkbox("Anti-Flash", &bAntiFlash);
                    ImGui::TableNextColumn(); ImGui::Checkbox("Anti-Humo",  &bAntiSmoke);
                    ImGui::EndTable();
                }
                ImGui::EndTabItem();
            }

            // ======= TAB: COMBATE =======
            if (ImGui::BeginTabItem(" Combate ")) {
                ImGui::Spacing();
                SecHeader(dl, "AIMBOT", IM_COL32(220,80,80,220));
                ImGui::Spacing();
                char aimbotLabel[48];
                if (aimbotKey == VK_XBUTTON1) snprintf(aimbotLabel,sizeof(aimbotLabel),"Aimbot [Mouse4]");
                else if (aimbotKey == VK_XBUTTON2) snprintf(aimbotLabel,sizeof(aimbotLabel),"Aimbot [Mouse5]");
                else if (aimbotKey == VK_MBUTTON)  snprintf(aimbotLabel,sizeof(aimbotLabel),"Aimbot [Mouse3]");
                else if (aimbotKey >= 'A' && aimbotKey <= 'Z') snprintf(aimbotLabel,sizeof(aimbotLabel),"Aimbot [%c]",(char)aimbotKey);
                else if (aimbotKey >= VK_F1 && aimbotKey <= VK_F12) snprintf(aimbotLabel,sizeof(aimbotLabel),"Aimbot [F%d]",aimbotKey-VK_F1+1);
                else snprintf(aimbotLabel,sizeof(aimbotLabel),"Aimbot [VK%d]",aimbotKey);
                ImGui::Checkbox(aimbotLabel, &bAimbot);
                if (bAimbot) {
                    ImGui::Indent(12.f);
                    ImGui::SetNextItemWidth(200.f);
                    ImGui::SliderFloat("FOV px",       &fAimFov,      30.f, 400.f, "%.0f");
                    ImGui::SetNextItemWidth(200.f);
                    ImGui::SliderFloat("Dist max (m)", &fAimMaxDist,  30.f, 1000000.f, "%.0f", ImGuiSliderFlags_Logarithmic);
                    ImGui::SetNextItemWidth(200.f);
                    ImGui::SliderFloat("Velocidad",    &fAimSmooth,    1.f,  100.f, "%.0f%%");
                    static const char* aimBoneNames[] = { "Cabeza", "Cuello", "Pecho" };
                    ImGui::SetNextItemWidth(200.f);
                    ImGui::Combo("Aimlock", &iAimBone, aimBoneNames, 3);
                    ImGui::Checkbox("Ignorar equipo", &bAimbotTeamFilter);
                    ImGui::SameLine(200);
                    ImGui::Checkbox("Sin FOV", &bAimbotNoFov);
                    ImGui::Checkbox("Mostrar FOV", &bShowFov);
                    ImGui::SameLine(200);
                    if (bShowFov) ImGui::TextColored(ImVec4(0.5f,0.8f,1.f,0.7f), "Sigue cursor • verde=lock");
                    static bool waitingForKey = false;
                    static DWORD waitStartTime = 0;
                    if (waitingForKey) {
                        ImGui::TextColored(ImVec4(0.45f,0.70f,1.f,1.f), "Presiona tecla... (ESC cancela)");
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
                    ImGui::Unindent(12.f);
                }
                ImGui::Spacing();
                SecHeader(dl, "TRIGGERBOT", IM_COL32(220,140,40,220));
                ImGui::Checkbox("Triggerbot (auto-disparo)", &bTriggerbot);
                if (bTriggerbot) {
                    ImGui::Indent(12.f);
                    ImGui::SetNextItemWidth(200.f);
                    ImGui::SliderFloat("Radio (px)", &fTriggerFov, 2.f, 40.f, "%.0f");
                    ImGui::Unindent(12.f);
                }
                ImGui::Spacing();
                SecHeader(dl, "MOVIMIENTO", IM_COL32(80,200,120,220));
                ImGui::Checkbox("Prediccion de movimiento", &bPrediction);
                if (bPrediction) {
                    ImGui::Indent(12.f);
                    ImGui::SetNextItemWidth(210.f);
                    ImGui::SliderFloat("Tiempo (s)##pred", &fPrediction, 0.01f, 0.3f, "%.2f");
                    ImGui::Unindent(12.f);
                }
                ImGui::BeginTable("##movopts", 2, ImGuiTableFlags_None, {0, 0});
                ImGui::TableNextColumn(); ImGui::Checkbox("Bunny Hop [Space]", &bBhop);
                ImGui::TableNextColumn(); ImGui::Checkbox("Anti-AFK",          &bAntiAFK);
                ImGui::TableNextColumn(); ImGui::Checkbox("Tercera Persona",   &bThirdPerson);
                ImGui::TableNextColumn(); ImGui::Checkbox("Noclip",            &bNoclip);
                ImGui::TableNextColumn(); ImGui::Checkbox("Fly",               &bFly);
                ImGui::EndTable();
                if (bFly) {
                    ImGui::Indent(12.f);
                    ImGui::SetNextItemWidth(200.f);
                    ImGui::SliderFloat("Velocidad vuelo##fly", &fFlySpeed, 5.f, 200.f, "%.0f");
                    ImGui::TextColored(ImVec4(0.5f,0.8f,1.f,0.8f), "Space=subir  Shift=bajar");
                    ImGui::Unindent(12.f);
                }
                ImGui::Spacing();
                SecHeader(dl, "JUGADOR", IM_COL32(80,220,180,220));
                ImGui::TextColored(ImVec4(1.f,0.75f,0.2f,0.9f), "Escribe en memoria del juego — bajo tu propio riesgo");
                ImGui::Spacing();
                ImGui::Checkbox("WalkSpeed personalizado", &bWalkSpeed);
                if (bWalkSpeed) {
                    ImGui::Indent(12.f);
                    ImGui::SetNextItemWidth(200.f);
                    ImGui::SliderFloat("Velocidad##ws", &fWalkSpeed, 1.f, 200.f, "%.0f");
                    ImGui::Unindent(12.f);
                }
                ImGui::Checkbox("JumpPower personalizado", &bJumpPower);
                if (bJumpPower) {
                    ImGui::Indent(12.f);
                    ImGui::SetNextItemWidth(200.f);
                    ImGui::SliderFloat("Fuerza salto##jp", &fJumpPower, 1.f, 500.f, "%.0f");
                    ImGui::Unindent(12.f);
                }
                ImGui::Checkbox("Gravedad personalizada", &bGravity);
                if (bGravity) {
                    ImGui::Indent(12.f);
                    ImGui::SetNextItemWidth(200.f);
                    ImGui::SliderFloat("Gravedad##grav", &fGravity, 0.f, 500.f, "%.0f");
                    ImGui::Unindent(12.f);
                }
                ImGui::Spacing();
                SecHeader(dl, "ARMAS", IM_COL32(220,80,120,220));
                ImGui::BeginTable("##wpnopts", 2, ImGuiTableFlags_None, {0, 0});
                ImGui::TableNextColumn(); ImGui::Checkbox("No Recoil",  &bNoRecoil);
                ImGui::TableNextColumn(); ImGui::Checkbox("Silent Aim", &bSilentAim);
                ImGui::EndTable();
                if (bNoRecoil) {
                    ImGui::Indent(12.f);
                    ImGui::SetNextItemWidth(200.f);
                    ImGui::SliderFloat("Fuerza##recoil", &fNoRecoilStr, 0.1f, 5.0f, "%.1f");
                    ImGui::Unindent(12.f);
                }
                ImGui::Spacing();
                SecHeader(dl, "HUD", IM_COL32(160,100,220,220));
                ImGui::Checkbox("Crosshair", &bCrosshair);
                if (bCrosshair) {
                    ImGui::Indent(12.f);
                    static const char* crossStyles[] = { "Cruz", "Punto", "Cruz + Punto" };
                    ImGui::SetNextItemWidth(200.f);
                    ImGui::Combo("Estilo##cross", &iCrosshairStyle, crossStyles, 3);
                    ImGui::Unindent(12.f);
                }
                ImGui::Checkbox("Hitmarker", &bHitmarker); ImGui::SameLine(200); ImGui::Checkbox("Kill Feed", &bKillfeed);
                ImGui::Spacing();
                SecHeader(dl, "PANIC KEY", IM_COL32(200,60,60,220));
                ImGui::Spacing();
                {
                    static bool waitPanicKey = false;
                    static DWORD waitPanicStart = 0;
                    if (waitPanicKey) {
                        ImGui::TextColored(ImVec4(0.45f,0.70f,1.f,1.f), "Presiona tecla... (ESC cancela)");
                        if (GetTickCount() - waitPanicStart > 300) {
                            for (int vk = 0x08; vk < 0xFE; vk++) {
                                if (vk == VK_LBUTTON || vk == VK_RBUTTON) continue;
                                if (vk == VK_ESCAPE) { if (GetAsyncKeyState(vk)&0x8000) { waitPanicKey=false; break; } continue; }
                                if (GetAsyncKeyState(vk)&0x8000) { panicKey=vk; waitPanicKey=false; break; }
                            }
                        }
                    } else {
                        char pkLabel[64];
                        snprintf(pkLabel, sizeof(pkLabel), "Tecla panico: %s", VKName(panicKey));
                        if (ImGui::Button(pkLabel, ImVec2(-1,0))) { waitPanicKey=true; waitPanicStart=GetTickCount(); }
                    }
                    ImGui::SameLine();
                    ImGui::TextColored(bPanicMode ? ImVec4(1.f,0.3f,0.3f,1.f) : ImVec4(0.4f,0.8f,0.4f,1.f),
                        bPanicMode ? "PANICO" : "");
                }
                ImGui::EndTabItem();
            }

            // ======= TAB: INFO =======
            if (ImGui::BeginTabItem(" Info ")) {
                ImGui::Spacing();
                SecHeader(dl, "ESTADO");
                {
                    ImDrawList* d = ImGui::GetWindowDrawList();
                    bool ok = dbgDM && dbgPS && dbgVmSum > 0.f;
                    // card de estado
                    ImVec2 cp = ImGui::GetCursorScreenPos();
                    d->AddRectFilled(cp, {cp.x + 490, cp.y + 52},
                        ok ? IM_COL32(15,40,22,220) : IM_COL32(40,14,14,220), 8.f);
                    d->AddRect(cp, {cp.x + 490, cp.y + 52},
                        ok ? IM_COL32(40,160,70,100) : IM_COL32(160,40,40,100), 8.f);
                    d->AddCircleFilled({cp.x+18, cp.y+14}, 6.f,
                        ok ? IM_COL32(50,220,90,255) : IM_COL32(220,60,60,255));
                    ImGui::Dummy({0, 4});
                    ImGui::Indent(32.f);
                    ImGui::TextColored(ok ? ImVec4(0.35f,0.90f,0.50f,1.f) : ImVec4(0.90f,0.30f,0.30f,1.f),
                        ok ? "Conectado a Roblox" : "Roblox no detectado");
                    ImGui::TextColored(ImVec4(0.45f,0.50f,0.65f,1.f), "Jugadores: %d  |  PlaceId: %lld", dbgPlayers, g_placeId);
                    ImGui::Unindent(32.f);
                    ImGui::Dummy({0, 8});
                }

                SecHeader(dl, "CUENTA");
                ImGui::TextColored(ImVec4(0.45f,0.50f,0.65f,1.f), "ID:    "); ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.70f,0.75f,0.90f,1.f), "%s", g_uid.empty() ? "-" : g_uid.c_str());
                ImGui::TextColored(ImVec4(0.45f,0.50f,0.65f,1.f), "HWID:  "); ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.50f,0.55f,0.70f,1.f), "%s", HWIDString().c_str());
                ImGui::Spacing();
                SecHeader(dl, "JUEGO", IM_COL32(200,160,50,220));
                ImGui::Spacing();
                if (ImGui::Button("Guardar PlaceId como Rivals", ImVec2(-1,0))) {
                    g_rivalsPlaceId = g_placeId;
                    std::ofstream f("ceitus_rivals_placeid.cfg");
                    if (f) f << g_rivalsPlaceId;
                }

                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.45f,0.50f,0.65f,1.f), "Version: " CEITUS_VERSION);

                if (g_updateAvailable && g_canUpdate) {
                    ImGui::Spacing();
                    ImGui::PushStyleColor(ImGuiCol_Button,       ImVec4(0.15f,0.50f,0.25f,1.f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f,0.60f,0.35f,1.f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.28f,0.72f,0.42f,1.f));
                    char updLabel[48]; snprintf(updLabel,sizeof(updLabel),"Actualizar a %s", g_latestVersion.c_str());
                    if (ImGui::Button(updLabel, ImVec2(-1,0))) {
                        std::thread([](){
                            char exePath[MAX_PATH];
                            GetModuleFileNameA(nullptr, exePath, MAX_PATH);
                            std::string newPath = std::string(exePath) + ".new";
                            if (URLDownloadToFileA(nullptr, URL_EXE, newPath.c_str(), 0, nullptr) == S_OK) {
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
                    ImGui::PopStyleColor(3);
                }

                ImGui::Spacing();
                static bool waitMenuKey = false;
                static DWORD waitMenuStart = 0;
                if (waitMenuKey) {
                    ImGui::TextColored(ImVec4(0.45f,0.70f,1.f,1.f), "Presiona tecla... (ESC cancela)");
                    if (GetTickCount() - waitMenuStart > 300) {
                        for (int vk = 0x08; vk < 0xFE; vk++) {
                            if (vk == VK_LBUTTON || vk == VK_RBUTTON) continue;
                            if (vk == VK_ESCAPE) { if (GetAsyncKeyState(vk)&0x8000) { waitMenuKey=false; break; } continue; }
                            if (GetAsyncKeyState(vk)&0x8000) { menuKey=vk; waitMenuKey=false; break; }
                        }
                    }
                } else {
                    char mkLabel[64];
                    snprintf(mkLabel, sizeof(mkLabel), "Tecla panel: %s", VKName(menuKey));
                    if (ImGui::Button(mkLabel, ImVec2(-1,0))) { waitMenuKey=true; waitMenuStart=GetTickCount(); }
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(" Scripts ")) {
                ImGui::Spacing();
                SecHeader(dl, "INYECTOR DE DLL", IM_COL32(140,80,220,220));
                ImGui::Spacing();

                ImGui::Text("DLL:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(-80.f);
                ImGui::InputText("##dllpath", g_dllPath, sizeof(g_dllPath));
                ImGui::SameLine();
                if (ImGui::Button("...##browse", ImVec2(70,0))) {
                    OPENFILENAMEA ofn{};
                    char buf[MAX_PATH] = "";
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = nullptr;
                    ofn.lpstrFilter = "DLL\0*.dll\0Todos\0*.*\0";
                    ofn.lpstrFile = buf;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                    if (GetOpenFileNameA(&ofn))
                        strncpy_s(g_dllPath, buf, MAX_PATH - 1);
                }

                ImGui::Spacing();
                bool canInject = g_dllPath[0] && mem.pid;
                if (!canInject) ImGui::BeginDisabled();
                if (ImGui::Button("Inyectar", ImVec2(-1, 0))) {
                    if (mem.pid) {
                        std::thread([]{
                            if (InjectDLL(g_dllPath)) {
                                g_injectStatus = "OK: DLL inyectada";
                            } else {
                                g_injectStatus = "Error: fallo la inyeccion";
                            }
                            g_injectStatusTime = GetTickCount();
                        }).detach();
                    }
                }
                if (!canInject) ImGui::EndDisabled();

                if (!g_injectStatus.empty() && GetTickCount() - g_injectStatusTime < 4000) {
                    ImGui::Spacing();
                    bool ok = g_injectStatus.rfind("OK",0) == 0;
                    ImGui::TextColored(ok ? ImVec4(0.2f,0.9f,0.3f,1.f) : ImVec4(0.9f,0.2f,0.2f,1.f),
                        "%s", g_injectStatus.c_str());
                } else if (!g_injectStatus.empty() && GetTickCount() - g_injectStatusTime >= 4000) {
                    g_injectStatus.clear();
                }

                if (!mem.proc) {
                    ImGui::Spacing();
                    ImGui::TextDisabled("(Roblox no detectado)");
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
        ImGui::PopStyleColor(3); // tab colors
        ImGui::EndChild();
        ImGui::End();

        // popup de actualización disponible (solo para keys con canUpdate:true)
        if (g_showUpdatePopup && g_canUpdate) {
            ImGui::OpenPopup("##updatepopup");
            g_showUpdatePopup = false;
        } else if (g_showUpdatePopup) {
            g_showUpdatePopup = false; // descartar silenciosamente para keys normales
        }
        if (ImGui::BeginPopupModal("##updatepopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar)) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.30f,0.70f,0.40f,1.f), "Nueva actualizacion disponible!");
            ImGui::Spacing();
            char msg[64]; snprintf(msg, sizeof(msg), "Version actual: " CEITUS_VERSION "  ->  %s", g_latestVersion.c_str());
            ImGui::Text("%s", msg);
            ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
            if (ImGui::Button("Actualizar ahora", ImVec2(200,0))) {
                ImGui::CloseCurrentPopup();
                std::thread([](){
                    char exePath[MAX_PATH];
                    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
                    std::string newPath = std::string(exePath) + ".new";
                    if (URLDownloadToFileA(nullptr, URL_EXE, newPath.c_str(), 0, nullptr) == S_OK) {
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
            ImGui::SameLine();
            if (ImGui::Button("Mas tarde", ImVec2(120,0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::Spacing();
            ImGui::EndPopup();
        }

        ImGui::Render();
        const float clear[4]{};
        g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
        g_ctx->ClearRenderTargetView(g_rtv, clear);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        HRESULT hr = g_chain->Present(0, 0);
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
            ImGui_ImplDX11_Shutdown();
            if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
            if (g_chain) { g_chain->Release(); g_chain = nullptr; }
            if (g_ctx) { g_ctx->Release(); g_ctx = nullptr; }
            if (g_dev) { g_dev->Release(); g_dev = nullptr; }
            DXGI_SWAP_CHAIN_DESC sd2{};
            sd2.BufferCount = 2; sd2.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            sd2.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd2.OutputWindow = menuWnd;
            sd2.SampleDesc.Count = 1; sd2.Windowed = TRUE; sd2.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
                nullptr, 0, D3D11_SDK_VERSION, &sd2, &g_chain, &g_dev, nullptr, &g_ctx);
            ID3D11Texture2D* buf2 = nullptr;
            g_chain->GetBuffer(0, IID_PPV_ARGS(&buf2));
            g_dev->CreateRenderTargetView(buf2, nullptr, &g_rtv); buf2->Release();
            ImGui_ImplDX11_Init(g_dev, g_ctx);
        }
        Sleep(1);

        // guardar config cada 5 segundos
        static DWORD lastSave = 0;
        DWORD nowSave = GetTickCount();
        if (nowSave - lastSave > 5000) { SaveConfig(); lastSave = nowSave; }

        // check periódico anti-debug (cada 7s) — detecta attach después del inicio
        static DWORD lastSecCheck = 0;
        if (nowSave - lastSecCheck > 7000) {
            lastSecCheck = nowSave;
            if (PeriodicSecurityCheck()) ExitProcess(0);
        }
    }

    SaveConfig();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    return 0;
}
