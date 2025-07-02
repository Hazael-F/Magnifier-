#pragma once
#include <windows.h>
#include <shellapi.h>

struct UIData {
    NOTIFYICONDATA nid = { 0 };
};

HICON LoadCustomIcon();
void ShowAlreadyRunningMessage();
void ShowStartupInfo(int windowWidth, int windowHeight, bool circularMode,
    bool mouseTracking, int zoomAreaSize, POINT currentAdjustment,
    int horizontalOffset, int verticalOffset, int moveStep, int refreshRate);
void InitializeTrayIcon(UIData& uiData, HWND hwnd);
void CleanupTrayIcon(UIData& uiData);