#pragma once
#include <windows.h>
#include <magnification.h>

struct MagnifierData {
    HWND hwndMag = nullptr;
    HWND hwndHost = nullptr;
    int currentZoomLevel = 0;
    POINT currentAdjustment = { 0, 0 };
    RECT sourceRects[5] = { 0 };
    POINT lastMousePos = { 0, 0 };
    HRGN hCircleRegion = nullptr;

    // Configuration
    int windowWidth = 0;
    int windowHeight = 0;
    int zoomAreaSize = 0;
    int moveStep = 0;
    int horizontalOffset = 0;
    int verticalOffset = 0;
    int refreshRate = 60;
    bool mouseTracking = false;
    bool circularMode = false;
};

bool InitializeMagnifier(MagnifierData& data, HWND hwndHost);
void UpdateMagnifierContent(MagnifierData& data);
void ApplyZoomLevel(MagnifierData& data, int newZoomLevel);
void UpdateMagnifierPosition(MagnifierData& data);
void CalculateSourceRects(MagnifierData& data);
void CalculateInitialAdjustment(MagnifierData& data);
void CleanupMagnifier(MagnifierData& data);