#include "Injection.h"
#include "Logging.h"
#include <Windows.h>
#include <TlHelp32.h>
#include "Obfuscator.h"
#include <fstream>
#include <vector>

// PE structures
typedef struct _IMAGE_RELOC {
    WORD offset : 12;
    WORD type : 4;
} IMAGE_RELOC, * PIMAGE_RELOC;

// Manual GetProcAddress
FARPROC ManualGetProcAddress(HMODULE hModule, const char* procName) {
    PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)hModule;
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + dosHeader->e_lfanew);
    PIMAGE_EXPORT_DIRECTORY exportDir = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)hModule + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

    DWORD* functions = (DWORD*)((BYTE*)hModule + exportDir->AddressOfFunctions);
    WORD* ordinals = (WORD*)((BYTE*)hModule + exportDir->AddressOfNameOrdinals);
    DWORD* names = (DWORD*)((BYTE*)hModule + exportDir->AddressOfNames);

    for (DWORD i = 0; i < exportDir->NumberOfNames; i++) {
        char* name = (char*)((BYTE*)hModule + names[i]);
        if (strcmp(name, procName) == 0) {
            return (FARPROC)((BYTE*)hModule + functions[ordinals[i]]);
        }
    }
    return NULL;
}

// Shellcode to load and execute DLL
unsigned char rawShellcode[] = {
    0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x08, 0x53, 0x56, 0x57, 0x8B, 0x7D, 0x08, 0x8B, 0x1F, 0x8B, 0x73,
    0x3C, 0x03, 0xF3, 0x8B, 0x76, 0x78, 0x03, 0xF3, 0x8B, 0x7E, 0x20, 0x03, 0xFB, 0x33, 0xC9, 0x89,
    0x4D, 0xF8, 0x8B, 0x17, 0x89, 0x55, 0xFC, 0x85, 0xD2, 0x74, 0x1F, 0x8B, 0x4A, 0x04, 0x8B, 0x42,
    0x08, 0x03, 0xC3, 0x50, 0x8B, 0x45, 0xFC, 0x50, 0xFF, 0x55, 0x08, 0x8B, 0x55, 0xFC, 0x83, 0xC2,
    0x14, 0x89, 0x55, 0xFC, 0x8B, 0x45, 0xF8, 0x40, 0x89, 0x45, 0xF8, 0xEB, 0xD8, 0x5F, 0x5E, 0x5B,
    0x8B, 0xE5, 0x5D, 0xC2, 0x04, 0x00
};

bool InjectDLL(DWORD pid, const char* dllPath) {
    return Obfuscator::ObfuscateControlFlow([&]() -> bool {
        MainLog(OBFUSCATE("Attempting to inject DLL: ") + std::string(dllPath));

        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        if (hProcess == NULL) {
            DWORD error = GetLastError();
            MainLog(OBFUSCATE("Failed to open target process. Error code: ") + std::to_string(error));
            return false;
        }

        LPVOID pDllPath = VirtualAllocEx(hProcess, 0, strlen(dllPath) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!pDllPath) {
            DWORD error = GetLastError();
            MainLog(OBFUSCATE("Failed to allocate memory in target process. Error code: ") + std::to_string(error));
            CloseHandle(hProcess);
            return false;
        }

        if (!WriteProcessMemory(hProcess, pDllPath, (LPVOID)dllPath, strlen(dllPath) + 1, 0)) {
            DWORD error = GetLastError();
            MainLog(OBFUSCATE("Failed to write DLL path to target process. Error code: ") + std::to_string(error));
            VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }

        HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
        if (!hKernel32) {
            DWORD error = GetLastError();
            MainLog(OBFUSCATE("Failed to get kernel32.dll handle. Error code: ") + std::to_string(error));
            VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }

        LPTHREAD_START_ROUTINE pLoadLibraryA = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryA");
        if (!pLoadLibraryA) {
            DWORD error = GetLastError();
            MainLog(OBFUSCATE("Failed to get LoadLibraryA address. Error code: ") + std::to_string(error));
            VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }

        HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pLoadLibraryA, pDllPath, 0, NULL);
        if (!hThread) {
            DWORD error = GetLastError();
            MainLog(OBFUSCATE("Failed to create remote thread. Error code: ") + std::to_string(error));
            VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }

        WaitForSingleObject(hThread, INFINITE);

        DWORD exitCode;
        GetExitCodeThread(hThread, &exitCode);
        if (exitCode == 0) {
            MainLog(OBFUSCATE("DLL injection failed. LoadLibraryA returned NULL."));
            CloseHandle(hThread);
            VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }

        MainLog(OBFUSCATE("DLL injected successfully."));
        CloseHandle(hThread);
        VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return true;
        })();
}

bool ManualInjectDLL(DWORD pid, const char* dllPath) {
    return Obfuscator::ObfuscateControlFlow([&]() -> bool {
        MainLog(OBFUSCATE("Attempting to manually inject DLL: ") + std::string(dllPath));

        // Open target process
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        if (hProcess == NULL) {
            MainLog(OBFUSCATE("Failed to open target process."));
            return false;
        }

        // Read DLL file
        std::ifstream file(dllPath, std::ios::binary | std::ios::ate);
        if (!file) {
            MainLog(OBFUSCATE("Failed to open DLL file."));
            CloseHandle(hProcess);
            return false;
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size)) {
            MainLog(OBFUSCATE("Failed to read DLL file."));
            CloseHandle(hProcess);
            return false;
        }

        // Parse PE headers
        PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)buffer.data();
        PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(buffer.data() + dosHeader->e_lfanew);

        // Allocate memory in target process
        LPVOID baseAddress = VirtualAllocEx(hProcess, NULL, ntHeaders->OptionalHeader.SizeOfImage,
            MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!baseAddress) {
            MainLog(OBFUSCATE("Failed to allocate memory in target process."));
            CloseHandle(hProcess);
            return false;
        }

        // Copy headers
        WriteProcessMemory(hProcess, baseAddress, buffer.data(), ntHeaders->OptionalHeader.SizeOfHeaders, NULL);

        // Copy sections
        PIMAGE_SECTION_HEADER sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
        for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++, sectionHeader++) {
            WriteProcessMemory(hProcess, (LPVOID)((LPBYTE)baseAddress + sectionHeader->VirtualAddress),
                buffer.data() + sectionHeader->PointerToRawData, sectionHeader->SizeOfRawData, NULL);
        }

        // Perform relocations
        DWORD deltaImageBase = (DWORD)baseAddress - ntHeaders->OptionalHeader.ImageBase;
        PIMAGE_DATA_DIRECTORY relocDir = &ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
        DWORD relocAddr = relocDir->VirtualAddress;
        while (relocAddr - relocDir->VirtualAddress < relocDir->Size) {
            PIMAGE_BASE_RELOCATION relocBlock = (PIMAGE_BASE_RELOCATION)(buffer.data() + relocAddr);
            relocAddr += sizeof(IMAGE_BASE_RELOCATION);
            DWORD relocCount = (relocBlock->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
            PIMAGE_RELOC reloc = (PIMAGE_RELOC)(buffer.data() + relocAddr);
            for (DWORD i = 0; i < relocCount; i++, reloc++) {
                if (reloc->type == IMAGE_REL_BASED_HIGHLOW) {
                    DWORD* patchAddr = (DWORD*)((LPBYTE)baseAddress + relocBlock->VirtualAddress + reloc->offset);
                    DWORD newAddr;
                    ReadProcessMemory(hProcess, patchAddr, &newAddr, sizeof(DWORD), NULL);
                    newAddr += deltaImageBase;
                    WriteProcessMemory(hProcess, patchAddr, &newAddr, sizeof(DWORD), NULL);
                }
            }
            relocAddr += relocCount * sizeof(WORD);
        }

        // Resolve imports
        PIMAGE_DATA_DIRECTORY importDir = &ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)(buffer.data() + importDir->VirtualAddress);
        for (; importDesc->Name; importDesc++) {
            LPCSTR libName = (LPCSTR)(buffer.data() + importDesc->Name);
            HMODULE hModule = LoadLibraryA(libName);
            if (!hModule) {
                MainLog(OBFUSCATE("Failed to load library: ") + std::string(libName));
                continue;
            }

            PIMAGE_THUNK_DATA thunk = (PIMAGE_THUNK_DATA)(buffer.data() + importDesc->FirstThunk);
            for (; thunk->u1.AddressOfData; thunk++) {
                if (IMAGE_SNAP_BY_ORDINAL(thunk->u1.Ordinal)) {
                    LPCSTR funcName = (LPCSTR)IMAGE_ORDINAL(thunk->u1.Ordinal);
                    thunk->u1.Function = (DWORD)ManualGetProcAddress(hModule, funcName);
                }
                else {
                    PIMAGE_IMPORT_BY_NAME import = (PIMAGE_IMPORT_BY_NAME)(buffer.data() + thunk->u1.AddressOfData);
                    thunk->u1.Function = (DWORD)ManualGetProcAddress(hModule, (LPCSTR)import->Name);
                }
                WriteProcessMemory(hProcess, (LPVOID)((LPBYTE)baseAddress + importDesc->FirstThunk + (thunk - (PIMAGE_THUNK_DATA)(buffer.data() + importDesc->FirstThunk)) * sizeof(IMAGE_THUNK_DATA)),
                    &thunk->u1.Function, sizeof(DWORD), NULL);
            }
        }

        // Inject and execute shellcode
        LPVOID shellcodeAddr = VirtualAllocEx(hProcess, NULL, sizeof(rawShellcode), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!shellcodeAddr) {
            MainLog(OBFUSCATE("Failed to allocate memory for shellcode."));
            VirtualFreeEx(hProcess, baseAddress, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }

        WriteProcessMemory(hProcess, shellcodeAddr, rawShellcode, sizeof(rawShellcode), NULL);

        HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)shellcodeAddr, baseAddress, 0, NULL);
        if (!hThread) {
            MainLog(OBFUSCATE("Failed to create remote thread for shellcode."));
            VirtualFreeEx(hProcess, baseAddress, 0, MEM_RELEASE);
            VirtualFreeEx(hProcess, shellcodeAddr, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            return false;
        }

        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);

        MainLog(OBFUSCATE("DLL manually injected successfully."));
        CloseHandle(hProcess);
        return true;
        })();
}