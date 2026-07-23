#include <windowsx.h>
#include <windows.h>
#include <d3d11.h>
#include <fstream>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "GuiStyle.h"
#include "GuiApp.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static ID3D11Device*           g_device        = nullptr;
static ID3D11DeviceContext*    g_deviceContext = nullptr;
static IDXGISwapChain*         g_swapChain     = nullptr;
static ID3D11RenderTargetView* g_renderTarget  = nullptr;
static GuiApp*                 g_app           = nullptr; // consulted by WM_NCHITTEST

static void CreateRenderTarget() {
    ID3D11Texture2D* backBuffer = nullptr;
    g_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    g_device->CreateRenderTargetView(backBuffer, nullptr, &g_renderTarget);
    backBuffer->Release();
}

static void CleanupRenderTarget() {
    if (g_renderTarget) { g_renderTarget->Release(); g_renderTarget = nullptr; }
}

static bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount       = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow      = hWnd;
    sd.SampleDesc.Count  = 1;
    sd.Windowed          = TRUE;
    sd.SwapEffect        = DXGI_SWAP_EFFECT_DISCARD;

    D3D_FEATURE_LEVEL featureLevel;
    D3D_FEATURE_LEVEL featureLevelArray[] = { D3D_FEATURE_LEVEL_11_0 };

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        featureLevelArray, 1, D3D11_SDK_VERSION,
        &sd, &g_swapChain, &g_device, &featureLevel, &g_deviceContext);
    if (FAILED(hr)) return false;

    CreateRenderTarget();
    return true;
}

static void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_swapChain)     { g_swapChain->Release();     g_swapChain     = nullptr; }
    if (g_deviceContext) { g_deviceContext->Release();  g_deviceContext = nullptr; }
    if (g_device)        { g_device->Release();         g_device        = nullptr; }
}

// Separate from ImGui's own ini (disabled below, since panel layout is fixed):
// this remembers only the OS window's screen position across launches.
static const char* kWindowPositionFile = "window_position.txt";

static void LoadWindowPosition(int& x, int& y) {
    std::ifstream in(kWindowPositionFile);
    if (in && (in >> x >> y)) return;
    x = 100;
    y = 100;
}

static void SaveWindowPosition(HWND hWnd) {
    RECT rc;
    if (!GetWindowRect(hWnd, &rc)) return;
    std::ofstream out(kWindowPositionFile);
    if (out) out << rc.left << " " << rc.top;
}

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;

    switch (msg) {
    case WM_SIZE:
        if (g_device && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_swapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) return 0; // disable ALT menu
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_NCHITTEST: {
        // WS_POPUP has no native border/title bar, so dragging is reimplemented
        // here: the top strip (minus the close button GuiApp reports) acts as a
        // drag handle. Resizing is intentionally not supported - fixed-size window.
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT  rc;
        GetWindowRect(hWnd, &rc);

        if (g_app && pt.y < rc.top + static_cast<int>(kTitleBarHeight)) {
            POINT client = pt;
            ScreenToClient(hWnd, &client);
            ImVec2 btnMin, btnMax;
            g_app->GetCloseButtonRect(btnMin, btnMax);
            bool overClose = client.x >= btnMin.x && client.x <= btnMax.x &&
                             client.y >= btnMin.y && client.y <= btnMax.y;
            if (!overClose) return HTCAPTION;
        }
        return HTCLIENT;
    }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    // Without this, Windows treats the process as DPI-unaware and bitmap-stretches
    // the whole rendered frame to match the display's scale factor - blurring
    // everything regardless of how crisply it was actually rendered. Deliberately
    // NOT scaling window/style/font sizes by the DPI factor here: GuiApp's panel
    // layout is hand-authored in fixed physical pixels, and scaling only some of
    // the pieces (not every hardcoded position in GuiApp.h) breaks that layout.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.style         = CS_CLASSDC | CS_DROPSHADOW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"0xeggGuiWindow";
    RegisterClassExW(&wc);

    int startX, startY;
    LoadWindowPosition(startX, startY);

    // WS_POPUP = borderless; GuiApp draws its own title bar, WM_NCHITTEST above
    // supplies the drag/resize behavior a native frame would normally provide.
    HWND hWnd = CreateWindowW(wc.lpszClassName, L"0xegg v" EGG_VERSION,
        WS_POPUP, startX, startY, 900, 800,
        nullptr, nullptr, wc.hInstance, nullptr);

    if (!CreateDeviceD3D(hWnd)) {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsLight();
    ApplyGlassStyle();

    // Default ImGui font is a small bitmap font (blocky at normal window scale);
    // Segoe UI is on effectively every Windows install and renders anti-aliased.
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // panel layout is fixed by GuiApp, not user-persisted
    ImFontConfig fontConfig;
    fontConfig.OversampleH = 2;
    fontConfig.OversampleV = 2;
    ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 21.0f, &fontConfig);
    if (!font) io.Fonts->AddFontDefault();

    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(g_device, g_deviceContext);

    GuiApp app(g_device);
    g_app = &app;
    bool running = true;

    while (running) {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) running = false;
        }
        if (!running || app.WantsClose()) break;
        if (app.ConsumeMinimizeRequest()) ShowWindow(hWnd, SW_MINIMIZE);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        app.Draw();

        ImGui::Render();
        const float clearColor[4] = { 0.94f, 0.95f, 0.97f, 1.0f };
        g_deviceContext->OMSetRenderTargets(1, &g_renderTarget, nullptr);
        g_deviceContext->ClearRenderTargetView(g_renderTarget, clearColor);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_swapChain->Present(1, 0);
    }

    SaveWindowPosition(hWnd);

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hWnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}
