#include <Windows.h>
#include <iostream>
#include <string>
#include <conio.h>
#include <thread>
#include <fstream>
#include "DLLInterface.h"
#include "DirectX.h"
#include "Globals.h"
#include "Injection.h"
#include "Logging.h"
#include "ProcessUtils.h"
#include "Obfuscator.h"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd) {
    return Obfuscator::ObfuscateControlFlow([&]() -> int {
        AllocConsole();
        FILE* f;
        freopen_s(&f, "CONOUT$", "w", stdout);

        mainLogFile.open("main_program_log.txt", std::ios::app);
        MainLog(OBFUSCATE("Program started"));

        const wchar_t* gameName = L"ac_client.exe";

        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string exeDir = std::string(exePath);
        size_t lastSlash = exeDir.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            exeDir = exeDir.substr(0, lastSlash);
        }
        std::string dllPath = exeDir + "\\Memory reader.dll";
        MainLog(OBFUSCATE("DLL path: ") + dllPath);

        DWORD pid = Obfuscator::PolymorphicAdd<DWORD>(GetProcessIdByName(gameName), 0);
        if (pid == 0) {
            MainLog(OBFUSCATE("Failed to find game process."));
            MessageBoxA(NULL, "Failed to find game process", "Error", MB_OK | MB_ICONERROR);
            return 1;
        }
        MainLog(OBFUSCATE("Game process found. PID: ") + std::to_string(pid));

        if (!InjectDLL(pid, dllPath.c_str())) {
            MainLog(OBFUSCATE("Failed to inject DLL."));
            MessageBoxA(NULL, "Failed to inject DLL", "Error", MB_OK | MB_ICONERROR);
            return 1;
        }

        MainLog(OBFUSCATE("DLL injected successfully. Waiting for it to initialize..."));
        Obfuscator::InsertJunkCode();
        Sleep(2000);

        MainLog(OBFUSCATE("Attempting to open named pipe..."));
        HANDLE pipeHandle = CreateFileW(
            L"\\\\.\\pipe\\AssaultCubeDataPipe",
            GENERIC_READ,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );

        if (pipeHandle == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            MainLog(OBFUSCATE("Failed to open named pipe. Error code: ") + std::to_string(error));
            MessageBoxA(NULL, ("Failed to open named pipe. Error code: " + std::to_string(error)).c_str(), "Error", MB_OK | MB_ICONERROR);
            return 1;
        }

        MainLog(OBFUSCATE("Successfully connected to named pipe."));

        HWND hGameWindow = FindWindowA(NULL, "AssaultCube");
        if (hGameWindow == NULL) {
            MainLog(OBFUSCATE("Failed to find game window."));
            CloseHandle(pipeHandle);
            return 1;
        }
        MainLog(OBFUSCATE("Game window found."));

        RECT clientRect;
        GetClientRect(hGameWindow, &clientRect);
        g_screenWidth = Obfuscator::PolymorphicAdd<int>(clientRect.right, -clientRect.left);
        g_screenHeight = Obfuscator::PolymorphicAdd<int>(clientRect.bottom, -clientRect.top);
        MainLog(OBFUSCATE("Screen size: ") + std::to_string(g_screenWidth) + OBFUSCATE("x") + std::to_string(g_screenHeight));

        CreateOverlayWindow(hInstance, hGameWindow,
            [](void* userData) -> GameData* {
                static GameData gameData;
                DWORD bytesRead;
                HANDLE pipeHandle = static_cast<HANDLE>(userData);
                MainLog(OBFUSCATE("Attempting to read from pipe..."));
                if (ReadFile(pipeHandle, &gameData, sizeof(GameData), &bytesRead, NULL)) {
                    if (bytesRead > 0) {
                        MainLog(OBFUSCATE("Successfully read ") + std::to_string(bytesRead) + OBFUSCATE(" bytes from pipe."));
                        LogReceivedGameData(gameData);
                        return &gameData;
                    }
                    else {
                        MainLog(OBFUSCATE("Read 0 bytes from pipe."));
                    }
                }
                else {
                    DWORD error = GetLastError();
                    MainLog(OBFUSCATE("Failed to read from pipe. Error code: ") + std::to_string(error));
                }
                return nullptr;
            },
            pipeHandle);

        MainLog(OBFUSCATE("Entering message loop"));
        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        MainLog(OBFUSCATE("Exiting message loop"));
        CloseHandle(pipeHandle);
        if (f) fclose(f);
        FreeConsole();
        mainLogFile.close();

        return (int)msg.wParam;
         })();
}