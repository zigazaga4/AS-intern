#include "ProcessUtils.h"
#include "Logging.h"
#include <Windows.h>
#include <TlHelp32.h>
#include "Obfuscator.h"

DWORD GetProcessIdByName(const wchar_t* processName) {
    return Obfuscator::ObfuscateControlFlow([&]() -> DWORD {
        DWORD pid = 0;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32W processEntry = {};
        processEntry.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(snapshot, &processEntry)) {
            do {
                if (_wcsicmp(processEntry.szExeFile, processName) == 0) {
                    // Simple XOR obfuscation
                    pid = processEntry.th32ProcessID ^ 0xABCDEF;
                    pid ^= 0xABCDEF; // Deobfuscate
                    break;
                }
                Obfuscator::InsertJunkCode();
            } while (Process32NextW(snapshot, &processEntry));
        }
        CloseHandle(snapshot);
        MainLog(OBFUSCATE("GetProcessIdByName: ") + std::to_string(pid) + OBFUSCATE(" for ") + std::string(processName, processName + wcslen(processName)));
        return pid;
        })();
}

HMODULE GetRemoteModuleHandle(DWORD pid, const wchar_t* moduleName) {
    return Obfuscator::ObfuscateControlFlow([&]() -> HMODULE {
        HMODULE hModule = NULL;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        MODULEENTRY32W moduleEntry = {};
        moduleEntry.dwSize = sizeof(MODULEENTRY32W);

        if (Module32FirstW(snapshot, &moduleEntry)) {
            do {
                if (_wcsicmp(moduleEntry.szModule, moduleName) == 0) {
                    hModule = moduleEntry.hModule;
                    break;
                }
                Obfuscator::InsertJunkCode();
            } while (Module32NextW(snapshot, &moduleEntry));
        }
        CloseHandle(snapshot);
        MainLog(OBFUSCATE("GetRemoteModuleHandle: ") + std::to_string((uintptr_t)hModule) + OBFUSCATE(" for ") + std::string(moduleName, moduleName + wcslen(moduleName)));
        return hModule;
        })();
}