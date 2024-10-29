#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include "Overlay.h"

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "d3dcompiler.lib")

IDXGISwapChain* swapchain;
ID3D11Device* dev;
ID3D11DeviceContext* devcon;
ID3D11RenderTargetView* backbuffer;

void InitD3D(HWND hWnd);
void RenderFrame(void);
void CleanD3D(void);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void CreateOverlayWindow(HINSTANCE hInstance);

void InitD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC scd;
    ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

    scd.BufferCount = 1;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.Width = 800;
    scd.BufferDesc.Height = 600;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hWnd;
    scd.SampleDesc.Count = 4;
    scd.Windowed = TRUE;
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL, D3D11_SDK_VERSION,
        &scd, &swapchain, &dev, NULL, &devcon);

    ID3D11Texture2D* pBackBuffer;
    swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

    dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);
    pBackBuffer->Release();

    devcon->OMSetRenderTargets(1, &backbuffer, NULL);

    D3D11_VIEWPORT viewport;
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = 800;
    viewport.Height = 600;

    devcon->RSSetViewports(1, &viewport);
}

void RenderFrame(void) {
    // Define the clear color (Midnight Blue)
    const float clearColor[4] = { 0.1f, 0.1f, 0.4f, 1.0f };

    devcon->ClearRenderTargetView(backbuffer, clearColor);

    // Drawing code here
    // e.g. draw a red circle in the middle of the screen
    // For simplicity, let's just clear the screen to a color

    swapchain->Present(0, 0);
}

void CleanD3D(void) {
    swapchain->SetFullscreenState(FALSE, NULL);
    backbuffer->Release();
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

void CreateOverlayWindow(HINSTANCE hInstance) {
    WNDCLASSEX wc;
    ZeroMemory(&wc, sizeof(WNDCLASSEX));

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"WindowClass";

    RegisterClassEx(&wc);

    HWND hWnd = CreateWindowEx(NULL,
        L"WindowClass",
        L"Our DirectX Program",
        WS_OVERLAPPEDWINDOW,
        100, 100,
        800, 600,
        NULL,
        NULL,
        hInstance,
        NULL);

    ShowWindow(hWnd, SW_SHOW);

    InitD3D(hWnd);

    MSG msg;
    while (TRUE) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (msg.message == WM_QUIT) break;

        RenderFrame();
    }

    CleanD3D();
}