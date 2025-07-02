#pragma once

// Configuration defaults
#define DEFAULT_WINDOW_WIDTH      300
#define DEFAULT_WINDOW_HEIGHT     300
#define DEFAULT_ZOOM_AREA_SIZE    100
#define DEFAULT_REFRESH_RATE      60
#define DEFAULT_MOVE_STEP         5
#define DEFAULT_CIRCULAR_MODE     true
#define DEFAULT_MOUSE_TRACKING    false

// Default manual adjustments (in steps)
#define DEFAULT_HORIZONTAL_OFFSET  0
#define DEFAULT_VERTICAL_OFFSET    0

// Zoom levels
extern const float zoomLevels[];

void LoadConfig(int& windowWidth, int& windowHeight, int& zoomAreaSize, int& refreshRate,
    int& moveStep, int& horizontalOffset, int& verticalOffset,
    bool& circularMode, bool& mouseTracking);