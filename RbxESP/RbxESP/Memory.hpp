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

    bool Attach(const wchar_t* name) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32W pe{ sizeof(pe) };
        while (Process32NextW(snap, &pe)) {
            if (!wcscmp(pe.szExeFile, name)) {
                CloseHandle(snap);
                proc = OpenProcess(PROCESS_VM_READ, FALSE, pe.th32ProcessID);
                base = ModBase(pe.th32ProcessID, name);
                return proc != nullptr;
            }
        }
        CloseHandle(snap);
        return false;
    }

    template<typename T>
    T Read(uintptr_t addr) const {
        T v{};
        ReadProcessMemory(proc, (LPCVOID)addr, &v, sizeof(T), nullptr);
        return v;
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
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
        MODULEENTRY32W me{ sizeof(me) };
        while (Module32NextW(snap, &me)) {
            if (!wcscmp(me.szModule, mod)) {
                CloseHandle(snap);
                return (uintptr_t)me.modBaseAddr;
            }
        }
        CloseHandle(snap);
        return 0;
    }
};

inline Memory mem;