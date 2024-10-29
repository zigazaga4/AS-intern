#include "DirectX.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include "ESP.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d2d1.lib")

using namespace Microsoft::WRL;

IDXGISwapChain* swapchain = nullptr;
ID3D11Device* dev = nullptr;
ID3D11DeviceContext* devcon = nullptr;
ComPtr<ID3D11RenderTargetView> backbuffer;
ComPtr<ID2D1Factory1> pFactory;
ComPtr<ID2D1DeviceContext> pRenderTarget;
ComPtr<ID2D1SolidColorBrush> pBrush;
ComPtr<IDXGISurface> pBackBuffer;
HWND hOverlayWnd = nullptr;

void InitD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 2;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.Width = g_screenWidth;
    scd.BufferDesc.Height = g_screenHeight;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hWnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr, 0, D3D11_SDK_VERSION, &scd, &swapchain, &dev, nullptr, &devcon);

    ResizeD3DResources();
}

void ResizeD3DResources() {
    if (backbuffer) backbuffer.Reset();
    if (pRenderTarget) pRenderTarget.Reset();

    swapchain->ResizeBuffers(0, g_screenWidth, g_screenHeight, DXGI_FORMAT_UNKNOWN, 0);

    ComPtr<ID3D11Texture2D> pBackBufferTexture;
    swapchain->GetBuffer(0, IID_PPV_ARGS(&pBackBufferTexture));
    dev->CreateRenderTargetView(pBackBufferTexture.Get(), nullptr, backbuffer.GetAddressOf());

    devcon->OMSetRenderTargets(1, backbuffer.GetAddressOf(), nullptr);

    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = static_cast<FLOAT>(g_screenWidth);
    viewport.Height = static_cast<FLOAT>(g_screenHeight);

    devcon->RSSetViewports(1, &viewport);

    InitD2D();
}

void InitD2D() {
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, pFactory.GetAddressOf());
    swapchain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));

    ComPtr<ID2D1RenderTarget> pRT;
    pFactory->CreateDxgiSurfaceRenderTarget(pBackBuffer.Get(), &props, &pRT);
    pRT.As(&pRenderTarget);
    pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), pBrush.GetAddressOf());
}

std::ofstream logFile("game_data_log.txt", std::ios::app);

void LogGameData(const GameData* gameData) {
    if (!logFile.is_open()) {
        std::cerr << "Failed to open log file." << std::endl;
        return;
    }

    logFile << "--- New Frame ---" << std::endl;
    logFile << "Player Count: " << gameData->playerCount << std::endl;

    // Log player positions
    for (int i = 0; i < gameData->playerCount; ++i) {
        const Player& player = gameData->players[i];
        logFile << "Player " << i << " Position: ("
            << std::fixed << std::setprecision(2)
            << player.position.x << ", "
            << player.position.y << ", "
            << player.position.z << ")" << std::endl;
    }

    // Log view matrix
    logFile << "View Matrix:" << std::endl;
    for (int i = 0; i < 4; ++i) {
        logFile << "[ ";
        for (int j = 0; j < 4; ++j) {
            logFile << std::setw(10) << std::setprecision(6) << std::fixed
                << gameData->viewMatrix.matrix[i * 4 + j] << " ";
        }
        logFile << "]" << std::endl;
    }

    logFile << std::endl;
    logFile.flush();
}

void RenderFrame(GetGameDataFunc GetGameData, void* userData) {
    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    devcon->ClearRenderTargetView(backbuffer.Get(), clearColor);

    if (pRenderTarget) {
        pRenderTarget->BeginDraw();
        pRenderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

        GameData* gameData = GetGameData(userData);
        if (gameData) {
            // Log game data
            LogGameData(gameData);

            RenderESP(pRenderTarget.Get(), pBrush.Get(), gameData->viewMatrix, gameData->players, gameData->playerCount, g_screenWidth, g_screenHeight, gameData->mainPlayerTeam);
        }

        pRenderTarget->EndDraw();
    }

    swapchain->Present(1, 0);
}

void CleanD3D() {
    swapchain->SetFullscreenState(FALSE, NULL);
    swapchain->Release();
    dev->Release();
    devcon->Release();
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void CreateOverlayWindow(HINSTANCE hInstance, HWND hGameWnd, GetGameDataFunc GetGameData, void* userData) {
    // Register the window class
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = L"OverlayClass";

    RegisterClassEx(&wc);

    // Get the game window's client area size
    RECT clientRect;
    GetClientRect(hGameWnd, &clientRect);
    g_screenWidth = clientRect.right - clientRect.left;
    g_screenHeight = clientRect.bottom - clientRect.top;

    // Get the game window's position
    RECT windowRect;
    GetWindowRect(hGameWnd, &windowRect);

    // Calculate the difference between window and client area
    int borderWidth = (windowRect.right - windowRect.left) - g_screenWidth;
    int titleHeight = (windowRect.bottom - windowRect.top) - g_screenHeight;

    // Create the overlay window
    hOverlayWnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
        wc.lpszClassName,
        L"Overlay",
        WS_POPUP,
        windowRect.left + (borderWidth / 2), // Account for left border
        windowRect.top + titleHeight,        // Account for titlebar
        g_screenWidth,
        g_screenHeight,
        NULL, NULL, hInstance, NULL
    );

    if (!hOverlayWnd) {
        std::cerr << "Failed to create overlay window. Error code: " << GetLastError() << std::endl;
        return;
    }

    // Make the window transparent
    SetLayeredWindowAttributes(hOverlayWnd, RGB(0, 0, 0), 255, LWA_COLORKEY | LWA_ALPHA);

    // Enable the window to pass through mouse and keyboard events
    LONG_PTR exStyle = GetWindowLongPtr(hOverlayWnd, GWL_EXSTYLE);
    SetWindowLongPtr(hOverlayWnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);

    // Enable DWM transitions for smoother updates
    BOOL enable = TRUE;
    DwmSetWindowAttribute(hOverlayWnd, DWMWA_TRANSITIONS_FORCEDISABLED, &enable, sizeof(enable));

    ShowWindow(hOverlayWnd, SW_SHOW);
    UpdateWindow(hOverlayWnd);

    InitD3D(hOverlayWnd);

    // Main message loop
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            GameData* gameData = GetGameData(userData);
            if (gameData) {
                // Check if window size has changed
                if (gameData->windowWidth != g_screenWidth || gameData->windowHeight != g_screenHeight) {
                    g_screenWidth = gameData->windowWidth;
                    g_screenHeight = gameData->windowHeight;
                    ResizeD3DResources();

                    // Update overlay window position and size
                    RECT updatedWindowRect;
                    GetWindowRect(hGameWnd, &updatedWindowRect);
                    SetWindowPos(hOverlayWnd, HWND_TOPMOST,
                        updatedWindowRect.left + (borderWidth / 2),
                        updatedWindowRect.top + titleHeight,
                        g_screenWidth, g_screenHeight,
                        SWP_NOACTIVATE | SWP_SHOWWINDOW);
                }

                RenderFrame(GetGameData, userData);
            }
        }
    }

    CleanD3D();
}

void UpdateOverlayPosition(HWND hGameWnd) {
    if (!hOverlayWnd || !hGameWnd) return;

    RECT windowRect;
    GetWindowRect(hGameWnd, &windowRect);

    RECT clientRect;
    GetClientRect(hGameWnd, &clientRect);

    int borderWidth = (windowRect.right - windowRect.left) - (clientRect.right - clientRect.left);
    int titleHeight = (windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top);

    SetWindowPos(hOverlayWnd, HWND_TOPMOST,
        windowRect.left + (borderWidth / 2),
        windowRect.top + titleHeight,
        g_screenWidth, g_screenHeight,
        SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

void UpdateOverlayWindowSize(HWND hGameWnd) {
    RECT rect;
    GetWindowRect(hGameWnd, &rect);
    SetWindowPos(hOverlayWnd, HWND_TOPMOST, rect.left, rect.top, g_screenWidth, g_screenHeight, SWP_SHOWWINDOW);
}