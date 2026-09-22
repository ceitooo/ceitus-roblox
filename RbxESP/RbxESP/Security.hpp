#pragma once
#include <Windows.h>
#include <tlhelp32.h>
#include <string>
#include <intrin.h>

// ---- compile-time XOR string encryption ----
// Uso: auto s = ESTR("hello");  ->  devuelve std::string desencriptada en runtime
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
// wstring version: encripta como char, convierte a wstring en runtime
inline std::wstring EStrW(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}
#define EWSTR(s) EStrW(ESTR(s))

// ---- anti-debug ----

// 1. IsDebuggerPresent (básico pero rápido)
inline bool CheckIsDebuggerPresent() {
    return IsDebuggerPresent() != 0;
}

// 2. NtQueryInformationProcess — detecta debuggers que ocultan IsDebuggerPresent
inline bool CheckNtDebug() {
    typedef NTSTATUS(WINAPI* NtQIP)(HANDLE, UINT, PVOID, ULONG, PULONG);
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");
    if (!ntdll) return false;
    auto fn = (NtQIP)GetProcAddress(ntdll, "NtQueryInformationProcess");
    if (!fn) return false;
    DWORD debugPort = 0;
    fn(GetCurrentProcess(), 7 /*ProcessDebugPort*/, &debugPort, sizeof(debugPort), nullptr);
    return debugPort != 0;
}

// 3. Heap flags — CE y OllyDbg dejan rastros en el heap
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
    DWORD flags    = *(DWORD*)((BYTE*)heap + 0x70);
    DWORD forceFlags = *(DWORD*)((BYTE*)heap + 0x74);
    return (flags & 0x70) || (forceFlags != 0);
}

// 4. Timing check — un debugger con breakpoints ralentiza la ejecución
inline bool CheckTiming() {
    LARGE_INTEGER f, t1, t2;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&t1);
    volatile int x = 0;
    for (int i = 0; i < 100000; i++) x += i;
    QueryPerformanceCounter(&t2);
    double ms = (double)(t2.QuadPart - t1.QuadPart) / f.QuadPart * 1000.0;
    return ms > 200.0; // más de 200ms = hay breakpoints
}

// 5. Hardware breakpoints — detecta DR0-DR3 (usados por x64dbg, CE)
inline bool CheckHWBreakpoints() {
    CONTEXT ctx{};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (!GetThreadContext(GetCurrentThread(), &ctx)) return false;
    return ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3;
}

// ---- anti-VM ----

// CPUID hypervisor bit (bit 31 de ECX en leaf 1)
inline bool CheckCPUID_VM() {
    int info[4] = {};
    __cpuid(info, 1);
    return (info[2] >> 31) & 1;
}

// Registry VMware/VBox
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
            RegCloseKey(h);
            return true;
        }
    }
    return false;
}

// Nombre de proceso de VM
inline bool CheckVMProcesses() {
    const wchar_t* procs[] = { L"vmtoolsd.exe",L"vmwaretray.exe",L"vboxservice.exe",L"vboxtray.exe" };
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32W pe{ sizeof(pe) };
    bool found = false;
    if (Process32FirstW(snap, &pe)) {
        do {
            for (auto& p : procs)
                if (_wcsicmp(pe.szExeFile, p) == 0) { found = true; break; }
        } while (!found && Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return found;
}

// ---- check combinado — llama esto al inicio ----
// Retorna true si detecta debugger o VM, false = limpio
inline bool SecurityCheck(bool allowVM = false) {
    if (CheckIsDebuggerPresent()) return true;
    if (CheckNtDebug())           return true;
    if (CheckHeapFlags())         return true;
    if (CheckHWBreakpoints())     return true;
    if (!allowVM) {
        if (CheckCPUID_VM())      return true;
        if (CheckVMRegistry())    return true;
        if (CheckVMProcesses())   return true;
    }
    return false;
}

// ---- check periódico (llama en el loop cada N frames) ----
inline bool PeriodicSecurityCheck() {
    if (CheckIsDebuggerPresent()) return true;
    if (CheckNtDebug())           return true;
    if (CheckHWBreakpoints())     return true;
    return false;
}
