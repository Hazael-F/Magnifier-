#include "Magnifier.h"
#include "Config.h"
#include <windows.h>
#include <magnification.h>

bool InitializeMagnifier(MagnifierData& data, HWND hwndHost) {
    data.hwndHost = hwndHost;
    data.hwndMag = CreateWindow(
        WC_MAGNIFIER, NULL,
        WS_CHILD | WS_VISIBLE,
        0, 0, data.windowWidth, data.windowHeight,
        hwndHost, NULL, GetModuleHandle(NULL), NULL
    );

    if (!data.hwndMag) return false;

    if (data.circularMode) {
        data.hCircleRegion = CreateEllipticRgn(0, 0, data.windowWidth, data.windowHeight);
        SetWindowRgn(hwndHost, data.hCircleRegion, TRUE);
    }

    return true;
}

void UpdateMagnifierContent(MagnifierData& data) {
    if (data.currentZoomLevel > 0) {
        if (data.mouseTracking) {
            float zoom = zoomLevels[data.currentZoomLevel - 1];
            int scaledSize = (int)(data.zoomAreaSize / zoom);

            RECT sourceRect = {
                data.lastMousePos.x - (scaledSize / 2) + (int)(data.currentAdjustment.x / zoom),
                data.lastMousePos.y - (scaledSize / 2) + (int)(data.currentAdjustment.y / zoom),
                data.lastMousePos.x + (scaledSize / 2) + (int)(data.currentAdjustment.x / zoom),
                data.lastMousePos.y + (scaledSize / 2) + (int)(data.currentAdjustment.y / zoom)
            };
            MagSetWindowSource(data.hwndMag, sourceRect);
        }
        else {
            MagSetWindowSource(data.hwndMag, data.sourceRects[data.currentZoomLevel - 1]);
        }
    }
}

void ApplyZoomLevel(MagnifierData& data, int newZoomLevel) {
    if (newZoomLevel == data.currentZoomLevel) return;

    data.currentZoomLevel = newZoomLevel;

    if (data.currentZoomLevel > 0) {
        float zoom = zoomLevels[data.currentZoomLevel - 1];
        MAGTRANSFORM matrix = { {
            {zoom, 0, 0},
            {0, zoom, 0},
            {0, 0, 1}
        } };
        MagSetWindowTransform(data.hwndMag, &matrix);

        CalculateSourceRects(data);
        UpdateMagnifierContent(data);
        ShowWindow(data.hwndHost, SW_SHOW);
    }
    else {
        ShowWindow(data.hwndHost, SW_HIDE);
    }
}

void UpdateMagnifierPosition(MagnifierData& data) {
    if (data.mouseTracking && data.currentZoomLevel > 0) {
        SetWindowPos(
            data.hwndHost,
            HWND_TOPMOST,
            data.lastMousePos.x - data.windowWidth / 2,
            data.lastMousePos.y - data.windowHeight / 2,
            0, 0,
            SWP_NOSIZE | SWP_NOACTIVATE
        );
    }
}

void CalculateSourceRects(MagnifierData& data) {
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    POINT center = data.mouseTracking ? data.lastMousePos : POINT{ screenWidth / 2, screenHeight / 2 };

    for (int i = 0; i < 5; i++) {
        float zoom = zoomLevels[i];
        int scaledSize = (int)(data.zoomAreaSize / zoom);

        data.sourceRects[i] = {
            center.x - (scaledSize / 2) + (int)(data.currentAdjustment.x / zoom),
            center.y - (scaledSize / 2) + (int)(data.currentAdjustment.y / zoom),
            center.x + (scaledSize / 2) + (int)(data.currentAdjustment.x / zoom),
            center.y + (scaledSize / 2) + (int)(data.currentAdjustment.y / zoom)
        };
    }
}

void CalculateInitialAdjustment(MagnifierData& data) {
    if (data.windowWidth == 300 && data.windowHeight == 300) {
        data.currentAdjustment.x = -100;
        data.currentAdjustment.y = -100;
    }
    else if (data.windowWidth == 600 && data.windowHeight == 600) {
        data.currentAdjustment.x = -250;
        data.currentAdjustment.y = -250;
    }
    else {
        float scale = (float)data.windowWidth / 300.0f;
        data.currentAdjustment.x = (int)(-100 * scale);
        data.currentAdjustment.y = data.currentAdjustment.x;
    }

    data.currentAdjustment.x += (data.horizontalOffset * data.moveStep);
    data.currentAdjustment.y += (data.verticalOffset * data.moveStep);
}

void CleanupMagnifier(MagnifierData& data) {
    if (data.hCircleRegion) {
        DeleteObject(data.hCircleRegion);
        data.hCircleRegion = nullptr;
    }
}