#pragma once
#include <Windows.h>
#include <d2d1.h>
#include <vector>
#include "DLLInterface.h"
#include "Utils.h"
#include "Globals.h"

typedef GameData* (*GetGameDataFunc)(void* userData);

void InitD3D(HWND hWnd);
void ResizeD3DResources();
void InitD2D();
void RenderFrame(GetGameDataFunc GetGameData, void* userData);
void CleanD3D();
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void CreateOverlayWindow(HINSTANCE hInstance, HWND hGameWnd, GetGameDataFunc GetGameData, void* userData);
void UpdateOverlayWindowSize(HWND hGameWnd);