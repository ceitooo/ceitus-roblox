#pragma once
#include <Windows.h>
#include <tlhelp32.h>
#include <string>
#include <intrin.h>
#include <atomic>

// ============================================================
// compile-time XOR string encryption
// Uso: ESTR("hello") -> std::string desencriptada en runtime
// ============================================================
template<size_t N>
struct XorStr {
    char buf[N]{};
    constexpr XorStr(const char(&s)[N]) {
        for (size_t i = 0; i < N; i++)
            buf[i] = s[i] ^ (char)(0xCE ^ (i & 0xFF));
    }
    std::string str() const {
        std::string r(N - 1, '\0');
        for (size_t i = 0; i < N - 1; i++)
            r[i] = buf[i] ^ (char)(0xCE ^ (i & 0xFF));
        return r;
    }
};
#define ESTR(s) ([]{ constexpr XorStr<sizeof(s)> x(s); return x.str(); }())
inline std::wstring EStrW(const std::string& s) { return std::wstring(s.begin(), s.end()); }
#define EWSTR(s) EStrW(ESTR(s))

// ============================================================
// ANTI-DEBUG
// ============================================================

inline bool CheckIsDebuggerPresent() { return IsDebuggerPresent() != 0; }

inline bool CheckNtDebug() {
    typedef NTSTATUS(WINAPI* NtQIP)(HANDLE, UINT, PVOID, ULONG, PULONG);
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll) return false;
    auto fn = (NtQIP)GetProcAddress(ntdll, "NtQueryInformationProcess");
    if (!fn) return false;
    DWORD debugPort = 0;
    fn(GetCurrentProcess(), 7, &debugPort, sizeof(debugPort), nullptr);
    return debugPort != 0;
}

inline bool CheckHeapFlags() {
    PVOID peb = nullptr;
#ifdef _WIN64
    peb = (PVOID)__readgsqword(0x60);
#else
    peb = (PVOID)__readfsdword(0x30);
#endif
    if (!peb) return false;
    PVOID heap = *(PVOID*)((BYTE*)peb + 0x30);
    if (!heap) return false;
    DWORD flags      = *(DWORD*)((BYTE*)heap + 0x70);
    DWORD forceFlags = *(DWORD*)((BYTE*)heap + 0x74);
    return (flags & 0x70) || (forceFlags != 0);
}

inline bool CheckHWBreakpoints() {
    CONTEXT ctx{};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (!GetThreadContext(GetCurrentThread(), &ctx)) return false;
    return ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3;
}

// NtSetInformationThread HideFromDebugger — hace el thread invisible al debugger
inline void HideThreadFromDebugger() {
    typedef NTSTATUS(WINAPI* NtSIT)(HANDLE, UINT, PVOID, ULONG);
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll) return;
    auto fn = (NtSIT)GetProcAddress(ntdll, "NtSetInformationThread");
    if (fn) fn(GetCurrentThread(), 0x11 /*ThreadHideFromDebugger*/, nullptr, 0);
}

// ============================================================
// ANTI-VM
// ============================================================

inline bool CheckCPUID_VM() {
    int info[4] = {};
    __cpuid(info, 1);
    return (info[2] >> 31) & 1;
}

inline bool CheckVMRegistry() {
    const char* keys[] = {
        "SOFTWARE\\VMware, Inc.\\VMware Tools",
        "SOFTWARE\\Oracle\\VirtualBox Guest Additions",
        "SYSTEM\\CurrentControlSet\\Services\\VBoxGuest",
        "SYSTEM\\CurrentControlSet\\Services\\vmhgfs",
    };
    for (auto& k : keys) {
        HKEY h = nullptr;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, k, 0, KEY_READ, &h) == ERROR_SUCCESS) {
            RegCloseKey(h); return true;
        }
    }
    return false;
}

inline bool CheckVMProcesses() {
    const wchar_t* vms[] = { L"vmtoolsd.exe",L"vmwaretray.exe",L"vboxservice.exe",L"vboxtray.exe" };
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W pe{ sizeof(pe) };
    bool found = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            for (auto& p : vms)
                if (_wcsicmp(pe.szExeFile, p) == 0) { found = true; break; }
        } while (!found && Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

// ============================================================
// DETECCION DE HERRAMIENTAS DE REVERSING
// ============================================================

inline bool CheckReversingTools() {
    const wchar_t* badProcs[] = {
        L"x64dbg.exe", L"x32dbg.exe", L"ollydbg.exe",
        L"cheatengine-x86_64.exe", L"cheatengine-x86_64-SSE4-AVX2.exe",
        L"cheatengine.exe", L"ida64.exe", L"ida.exe",
        L"idaq.exe", L"idaq64.exe", L"idaw.exe",
        L"ghidra.exe", L"ghidraRun.exe",
        L"wireshark.exe", L"processhacker.exe",
        L"procmon.exe", L"procmon64.exe",
        L"pestudio.exe", L"dnspy.exe",
        L"scylla_x64.exe", L"scylla_x86.exe",
        L"HxD.exe", L"lordpe.exe", L"ImportREC.exe",
    };
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W pe{ sizeof(pe) };
    bool found = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            for (auto& p : badProcs)
                if (_wcsicmp(pe.szExeFile, p) == 0) { found = true; break; }
        } while (!found && Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    // tambien chequear titulos de ventanas
    if (!found) {
        const wchar_t* badTitles[] = { L"x64dbg", L"x32dbg", L"OllyDbg", L"Cheat Engine", L"IDA ", L"Ghidra" };
        for (auto& t : badTitles) {
            if (FindWindowW(nullptr, t)) { found = true; break; }
        }
    }
    return found;
}

// ============================================================
// SELF-CHECKSUM (detecta patches en memoria en runtime)
// ============================================================

static std::atomic<uint32_t> g_textChecksum{ 0 };

inline uint32_t ComputeModuleChecksum(size_t offset, size_t len) {
    HMODULE hMod = GetModuleHandleW(nullptr);
    if (!hMod) return 0;
    auto base = (const BYTE*)hMod;
    // encontrar .text section
    auto dos  = (IMAGE_DOS_HEADER*)base;
    auto nt   = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
    auto sec  = IMAGE_FIRST_SECTION(nt);
    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++) {
        if (memcmp(sec->Name, ".text", 5) == 0) {
            const BYTE* start = base + sec->VirtualAddress + offset;
            size_t maxLen = sec->Misc.VirtualSize > offset ? sec->Misc.VirtualSize - offset : 0;
            if (len > maxLen) len = maxLen;
            uint32_t crc = 0x811c9dc5u;
            for (size_t j = 0; j < len; j++) {
                crc ^= start[j];
                crc *= 0x01000193u;
            }
            return crc;
        }
    }
    return 0;
}

inline void InitChecksum() {
    g_textChecksum = ComputeModuleChecksum(0x200, 0x8000);
}

inline bool CheckIntegrity() {
    if (g_textChecksum == 0) return false; // no inicializado
    return ComputeModuleChecksum(0x200, 0x8000) != g_textChecksum;
}

// ============================================================
// ANTI-DUMP: borra el PE header en memoria
// ============================================================

inline void ErasePEHeader() {
    HMODULE hMod = GetModuleHandleW(nullptr);
    if (!hMod) return;
    DWORD old = 0;
    VirtualProtect(hMod, 0x1000, PAGE_READWRITE, &old);
    memset(hMod, 0, 0x1000);
    VirtualProtect(hMod, 0x1000, old, &old);
}

// ============================================================
// ENCRYPT LICENCIA con HWID como clave
// ============================================================

inline std::string EncryptLicense(const std::string& data, uint32_t hwid) {
    std::string out = data;
    uint32_t key = hwid ^ 0xDEADCE17u;
    for (size_t i = 0; i < out.size(); i++)
        out[i] ^= (char)((key >> ((i % 4) * 8)) & 0xFF);
    return out;
}
inline std::string DecryptLicense(const std::string& data, uint32_t hwid) {
    return EncryptLicense(data, hwid); // XOR es simétrico
}

// ============================================================
// TLS CALLBACK — corre ANTES de main()
// ============================================================

// forward declaration
void NTAPI CeitusTlsCallback(PVOID, DWORD, PVOID);

#pragma section(".CRT$XLB", read)
__declspec(allocate(".CRT$XLB"))
extern PIMAGE_TLS_CALLBACK _ceitus_tls_cb[] = { CeitusTlsCallback, nullptr };

#pragma comment(linker, "/INCLUDE:_tls_used")

inline void NTAPI CeitusTlsCallback(PVOID, DWORD reason, PVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        // checks antes de main() — el debugger no puede interceptarlos a tiempo
        if (IsDebuggerPresent()) ExitProcess(0);
        BOOL remote = FALSE;
        CheckRemoteDebuggerPresent(GetCurrentProcess(), &remote);
        if (remote) ExitProcess(0);
        HideThreadFromDebugger();
    }
    if (reason == DLL_THREAD_ATTACH) {
        HideThreadFromDebugger();
    }
}

// ============================================================
// CHECK COMBINADO INICIO
// ============================================================

inline bool SecurityCheck() {
    if (CheckIsDebuggerPresent()) return false;
    if (CheckNtDebug())           return false;
    if (CheckHeapFlags())         return false;
    if (CheckHWBreakpoints())     return false;
    // CheckCPUID_VM() removido: Windows 11 VBS/HVCI setea el hypervisor bit en hardware real
    if (CheckVMRegistry())        return false;
    if (CheckVMProcesses())       return false;
    if (CheckReversingTools())    return false;
    return true;
}

// ============================================================
// CHECK PERIODICO (llamar cada N segundos en el loop)
// ============================================================

inline bool PeriodicSecurityCheck() {
    if (CheckIsDebuggerPresent()) return true;
    if (CheckNtDebug())           return true;
    if (CheckHWBreakpoints())     return true;
    if (CheckReversingTools())    return true;
    if (CheckIntegrity())         return true;
    return false;
}
