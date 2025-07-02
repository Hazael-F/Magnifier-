#include "UI.h"
#include <windows.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

HICON LoadCustomIcon() {
    WCHAR iconPath[MAX_PATH];
    GetModuleFileName(NULL, iconPath, MAX_PATH);
    PathRemoveFileSpec(iconPath);
    PathAppend(iconPath, L"reticle.ico");

    HICON hIcon = (HICON)LoadImage(
        NULL, iconPath, IMAGE_ICON,
        0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE | LR_SHARED
    );

    if (!hIcon) {
        hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }

    return hIcon;
}

void ShowAlreadyRunningMessage() {
    MessageBox(NULL,
        L"Magnifier+ is already running.\n\n"
        L"Only one instance of the application can run at a time.\n"
        L"Check your system tray for the running instance.",
        L"Magnifier+ Already Running",
        MB_OK | MB_ICONINFORMATION);
}

void ShowStartupInfo(int windowWidth, int windowHeight, bool circularMode,
    bool mouseTracking, int zoomAreaSize, POINT currentAdjustment,
    int horizontalOffset, int verticalOffset, int moveStep, int refreshRate) {

    WCHAR path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    PathRemoveFileSpec(path);
    PathAppend(path, L"MagnifierPlus.ini");

    WCHAR message[1024];
    wsprintf(message,
        L"Magnifier+ Initialized\n\n"
        L"Configuration File:\n%s\n\n"
        L"Current Settings:\n"
        L"- Window Size: %dx%d pixels\n"
        L"- Window Shape: %s\n"
        L"- Tracking Mode: %s\n"
        L"- Zoom Area Size: %d pixels\n"
        L"- Initial Adjustment: (%d, %d)\n"
        L"- Manual Offsets: %dH, %dV steps\n"
        L"- Move Step Size: %d pixels\n"
        L"- Refresh Rate: %d FPS\n\n"
        L"Controls:\n"
        L"1. Right-click + Scroll: Zoom (1.25x-2x-3x-4x-5x)\n"
        L"2. Right-click + Arrows: Move view\n"
        L"3. Right-click tray icon: Exit",
        path, windowWidth, windowHeight,
        circularMode ? L"Circle" : L"Square",
        mouseTracking ? L"Mouse" : L"Screen Center",
        zoomAreaSize,
        currentAdjustment.x, currentAdjustment.y,
        horizontalOffset, verticalOffset,
        moveStep, refreshRate);

    MessageBox(NULL, message, L"Magnifier+ Ready", MB_OK | MB_ICONINFORMATION);
}

void InitializeTrayIcon(UIData& uiData, HWND hwnd) {
    uiData.nid.cbSize = sizeof(uiData.nid);
    uiData.nid.hWnd = hwnd;
    uiData.nid.uID = 1;
    uiData.nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    uiData.nid.uCallbackMessage = WM_USER + 1;
    uiData.nid.hIcon = LoadCustomIcon();
    wcscpy_s(uiData.nid.szTip, L"Magnifier+ (Right-click to exit)");
    Shell_NotifyIcon(NIM_ADD, &uiData.nid);
}

void CleanupTrayIcon(UIData& uiData) {
    Shell_NotifyIcon(NIM_DELETE, &uiData.nid);
}