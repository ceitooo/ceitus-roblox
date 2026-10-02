#define NOMINMAX
#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <cstdint>
#include <string>
#include <algorithm>

struct Memory {
    HANDLE    proc = nullptr;
    uintptr_t base = 0;
    DWORD     pid  = 0;

    bool Attach(const wchar_t* name) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32W pe{ sizeof(pe) };
        if (Process32FirstW(snap, &pe)) do {
            if (!wcscmp(pe.szExeFile, name)) {
                CloseHandle(snap);
                pid  = pe.th32ProcessID;
                proc = OpenProcess(PROCESS_VM_READ, FALSE, pid);
                base = ModBase(pid, name);
                return proc != nullptr;
            }
        } while (Process32NextW(snap, &pe));
        CloseHandle(snap);
        return false;
    }

    template<typename T>
    T Read(uintptr_t addr) const {
        T v{};
        ReadProcessMemory(proc, (LPCVOID)addr, &v, sizeof(T), nullptr);
        return v;
    }

    template<typename T>
    bool Write(uintptr_t addr, const T& val) const {
        if (!pid) return false;
        HANDLE hw = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid);
        if (!hw) return false;
        SIZE_T written = 0;
        bool ok = WriteProcessMemory(hw, (LPVOID)addr, &val, sizeof(T), &written);
        CloseHandle(hw);
        return ok && written == sizeof(T);
    }

    bool WriteRaw(uintptr_t addr, const void* data, size_t sz) const {
        if (!pid) return false;
        HANDLE hw = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid);
        if (!hw) return false;
        SIZE_T written = 0;
        bool ok = WriteProcessMemory(hw, (LPVOID)addr, data, sz, &written);
        CloseHandle(hw);
        return ok;
    }

    std::string ReadRbxString(uintptr_t addr) const {
        if (!addr) return {};
        uint32_t len = Read<uint32_t>(addr + 0x10);
        if (len == 0 || len > 256) return {};
        char buf[257]{};
        uintptr_t strPtr = (len < 16) ? addr : Read<uintptr_t>(addr);
        ReadProcessMemory(proc, (LPCVOID)strPtr, buf, std::min(len, 256u), nullptr);
        return std::string(buf, len);
    }

private:
    uintptr_t ModBase(DWORD pid, const wchar_t* mod) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        MODULEENTRY32W me{ sizeof(me) };
        if (Module32FirstW(snap, &me)) do {
            if (!wcscmp(me.szModule, mod)) {
                CloseHandle(snap);
                return (uintptr_t)me.modBaseAddr;
            }
        } while (Module32NextW(snap, &me));
        CloseHandle(snap);
        return 0;
    }
};

inline Memory mem;