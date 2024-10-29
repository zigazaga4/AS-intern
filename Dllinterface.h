#pragma once

#include <Windows.h>
#include <vector>
#include <cstdint>

#ifdef MEMORYREADERDLL_EXPORTS
#define MEMORYREADER_API __declspec(dllexport)
#else
#define MEMORYREADER_API __declspec(dllimport)
#endif

#pragma pack(push, 1)

struct Vector3 {
    float x, y, z;
};

struct Vector4 {
    float x, y, z, w;
};

struct Player {
    Vector3 position;
    Vector3 head;
    int health;
    bool isAlive;
    int team;
};

struct ViewMatrix {
    float matrix[16];
};

struct GameData {
    Player players[32];
    int playerCount;
    ViewMatrix viewMatrix;
    float cameraYaw;
    float cameraPitch;
    uint64_t dataId;
    int windowWidth;
    int windowHeight;
    int mainPlayerTeam;
};

#pragma pack(pop)

extern "C" {
    MEMORYREADER_API void StartMemoryReader();
    MEMORYREADER_API void StopMemoryReader();
    MEMORYREADER_API BOOL WINAPI IsDllReady();
}