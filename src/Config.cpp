#include "Config.h"
#include <windows.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

const float zoomLevels[] = { 1.25f, 2.0f, 3.0f, 4.0f, 5.0f };

void LoadConfig(int& windowWidth, int& windowHeight, int& zoomAreaSize, int& refreshRate,
    int& moveStep, int& horizontalOffset, int& verticalOffset,
    bool& circularMode, bool& mouseTracking) {

    WCHAR configPath[MAX_PATH];
    GetModuleFileName(NULL, configPath, MAX_PATH);
    PathRemoveFileSpec(configPath);
    PathAppend(configPath, L"MagnifierPlus.ini");

    windowWidth = GetPrivateProfileInt(L"Window", L"Width", DEFAULT_WINDOW_WIDTH, configPath);
    windowHeight = GetPrivateProfileInt(L"Window", L"Height", DEFAULT_WINDOW_HEIGHT, configPath);
    zoomAreaSize = GetPrivateProfileInt(L"Zoom", L"AreaSize", DEFAULT_ZOOM_AREA_SIZE, configPath);
    refreshRate = GetPrivateProfileInt(L"Performance", L"RefreshRate", DEFAULT_REFRESH_RATE, configPath);
    moveStep = GetPrivateProfileInt(L"Movement", L"StepSize", DEFAULT_MOVE_STEP, configPath);
    horizontalOffset = GetPrivateProfileInt(L"Adjustments", L"Horizontal", DEFAULT_HORIZONTAL_OFFSET, configPath);
    verticalOffset = GetPrivateProfileInt(L"Adjustments", L"Vertical", DEFAULT_VERTICAL_OFFSET, configPath);

    circularMode = GetPrivateProfileInt(L"Window", L"Circular", DEFAULT_CIRCULAR_MODE, configPath) != 0;
    mouseTracking = GetPrivateProfileInt(L"Tracking", L"Mouse", DEFAULT_MOUSE_TRACKING, configPath) != 0;
}