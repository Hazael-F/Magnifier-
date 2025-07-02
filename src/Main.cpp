#include "Config.h"
#include "Magnifier.h"
#include "Input.h"
#include "UI.h"
#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Magnification.lib")

MagnifierData g_magnifierData;
InputData g_inputData;
UIData g_uiData;
HANDLE g_hMutex = NULL;

LRESULT CALLBACK MagnifierWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_TIMER:
        UpdateMagnifierContent(g_magnifierData);
        break;

    case WM_DESTROY:
        CleanupInputHooks(g_inputData);
        CleanupTrayIcon(g_uiData);
        CleanupMagnifier(g_magnifierData);
        if (g_hMutex) {
            CloseHandle(g_hMutex);
            g_hMutex = NULL;
        }
        PostQuitMessage(0);
        break;

    case WM_USER + 1:
        if (lParam == WM_RBUTTONUP) DestroyWindow(hwnd);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPTSTR lpCmdLine, int nCmdShow) {
    g_hMutex = CreateMutex(NULL, TRUE, L"MagnifierPlusInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        ShowAlreadyRunningMessage();
        return 0;
    }

    LoadConfig(g_magnifierData.windowWidth, g_magnifierData.windowHeight, g_magnifierData.zoomAreaSize,
        g_magnifierData.refreshRate, g_magnifierData.moveStep, g_magnifierData.horizontalOffset,
        g_magnifierData.verticalOffset, g_magnifierData.circularMode, g_magnifierData.mouseTracking);

    g_magnifierData.currentZoomLevel = 0; // Start at level 0 (hidden)
    CalculateInitialAdjustment(g_magnifierData);
    CalculateSourceRects(g_magnifierData);

    // Show startup info BEFORE initializing magnification
    ShowStartupInfo(g_magnifierData.windowWidth, g_magnifierData.windowHeight,
        g_magnifierData.circularMode, g_magnifierData.mouseTracking,
        g_magnifierData.zoomAreaSize, g_magnifierData.currentAdjustment,
        g_magnifierData.horizontalOffset, g_magnifierData.verticalOffset,
        g_magnifierData.moveStep, g_magnifierData.refreshRate);

    if (!MagInitialize()) {
        MessageBox(NULL, L"Failed to initialize magnification API!", L"Error", MB_ICONERROR);
        return -1;
    }

    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = MagnifierWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MagnifierWindowClass";
    wc.hIcon = LoadCustomIcon();
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, L"Window registration failed!", L"Error", MB_ICONERROR);
        MagUninitialize();
        return -1;
    }

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    HWND hwndMagnifier = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        L"MagnifierWindowClass",
        L"Magnifier+ (Right-click + Scroll: Zoom | Arrows: Move)",
        WS_POPUP,
        g_magnifierData.mouseTracking ? 0 : (screenWidth - g_magnifierData.windowWidth) / 2,
        g_magnifierData.mouseTracking ? 0 : (screenHeight - g_magnifierData.windowHeight) / 2,
        g_magnifierData.windowWidth, g_magnifierData.windowHeight,
        NULL, NULL, hInstance, NULL
    );

    if (!hwndMagnifier || !InitializeMagnifier(g_magnifierData, hwndMagnifier)) {
        MessageBox(NULL, L"Window creation failed!", L"Error", MB_ICONERROR);
        MagUninitialize();
        return -1;
    }

    InitializeInputHooks(g_inputData, hInstance);
    InitializeTrayIcon(g_uiData, hwndMagnifier);

    SetTimer(hwndMagnifier, 1, 1000 / g_magnifierData.refreshRate, NULL);
    ShowWindow(hwndMagnifier, SW_HIDE); // Start hidden

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    KillTimer(hwndMagnifier, 1);
    MagUninitialize();
    return (int)msg.wParam;
}