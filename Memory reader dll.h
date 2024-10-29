#pragma once

#include <Windows.h>
#include <vector>

#ifdef MEMORYREADERDLL_EXPORTS
#define MEMORYREADER_API __declspec(dllexport)
#else
#define MEMORYREADER_API __declspec(dllimport)
#endif

struct Vector3 {
    float x, y, z;
};

struct Player {
    Vector3 position;
    Vector3 head;
    Vector3 feet;
    int health;
    bool isAlive;
    int team;
};

struct ViewMatrix {
    float matrix[16];
};

struct GameData {
    std::vector<Player> players;
    ViewMatrix viewMatrix;
    float cameraYaw;
    float cameraPitch;
};

extern "C" {
    MEMORYREADER_API BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved);
    MEMORYREADER_API void StartMemoryReader();
    MEMORYREADER_API GameData* GetGameData();
    MEMORYREADER_API void StopMemoryReader();
}