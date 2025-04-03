/*===============================================
                MAGNIFIER+
  ===============================================
  A lightweight screen magnifier with:
  - Instant zoom changes (1.25x, 2.0x, 3.0x, 4.0x, 5.0x)
  - Perfect live magnification in both modes
  - Smooth mouse tracking with continuous updates
  - Single instance enforcement
  - Position adjustment controls
  - System tray accessibility
  - Configurable settings via INI file
  - Optional circular window shape
  - Optional mouse tracking mode
===============================================*/

#include <windows.h>
#include <magnification.h>
#include <tchar.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Magnification.lib")

// Configuration defaults
#define DEFAULT_WINDOW_WIDTH      300
#define DEFAULT_WINDOW_HEIGHT     300
#define DEFAULT_ZOOM_AREA_SIZE    100
#define DEFAULT_REFRESH_RATE      60
#define DEFAULT_MOVE_STEP         5
#define DEFAULT_CIRCULAR_MODE     false
#define DEFAULT_MOUSE_TRACKING    false

// Default manual adjustments (in steps)
#define DEFAULT_HORIZONTAL_OFFSET  13
#define DEFAULT_VERTICAL_OFFSET    13

// Global variables
int windowWidth = DEFAULT_WINDOW_WIDTH;
int windowHeight = DEFAULT_WINDOW_HEIGHT;
int zoomAreaSize = DEFAULT_ZOOM_AREA_SIZE;
int refreshRate = DEFAULT_REFRESH_RATE;
int moveStep = DEFAULT_MOVE_STEP;
int horizontalOffset = DEFAULT_HORIZONTAL_OFFSET;
int verticalOffset = DEFAULT_VERTICAL_OFFSET;
bool circularMode = DEFAULT_CIRCULAR_MODE;
bool mouseTracking = DEFAULT_MOUSE_TRACKING;

// Zoom levels with new progression
float zoomLevels[] = { 1.25f, 2.0f, 3.0f, 4.0f, 5.0f };
int currentZoomLevel = 0;
POINT currentAdjustment = { 0, 0 };
RECT sourceRects[5];
POINT lastMousePos = { 0, 0 };

HWND hwndMagnifier, hwndMag;
bool isRightMouseDown = false;
HHOOK hMouseHook, hKeyboardHook;
NOTIFYICONDATA nid = { 0 };
HRGN hCircleRegion = NULL;
HANDLE hMutex = NULL;

/*============== CREATE CIRCULAR REGION ==============*/
void CreateCircularRegion() {
    if (hCircleRegion) DeleteObject(hCircleRegion);
    hCircleRegion = CreateEllipticRgn(0, 0, windowWidth, windowHeight);
}

/*============== PRECISE ADJUSTMENT CALCULATION ==============*/
void CalculateInitialAdjustment() {
    if (windowWidth == 300 && windowHeight == 300) {
        currentAdjustment.x = -100;
        currentAdjustment.y = -100;
    }
    else if (windowWidth == 600 && windowHeight == 600) {
        currentAdjustment.x = -250;
        currentAdjustment.y = -250;
    }
    else {
        float scale = (float)windowWidth / 300.0f;
        currentAdjustment.x = (int)(-100 * scale);
        currentAdjustment.y = currentAdjustment.x;
    }

    currentAdjustment.x += (horizontalOffset * moveStep);
    currentAdjustment.y += (verticalOffset * moveStep);
}

/*============== MAGNIFICATION SOURCE CALCULATION ==============*/
void CalculateSourceRects() {
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    POINT center = mouseTracking ? lastMousePos : POINT{ screenWidth / 2, screenHeight / 2 };

    for (int i = 0; i < 5; i++) {
        float zoom = zoomLevels[i];
        int scaledSize = (int)(zoomAreaSize / zoom);

        sourceRects[i] = {
            center.x - (scaledSize / 2) + (int)(currentAdjustment.x / zoom),
            center.y - (scaledSize / 2) + (int)(currentAdjustment.y / zoom),
            center.x + (scaledSize / 2) + (int)(currentAdjustment.x / zoom),
            center.y + (scaledSize / 2) + (int)(currentAdjustment.y / zoom)
        };
    }
}

/*============== UPDATE MAGNIFIER POSITION ==============*/
void UpdateMagnifierPosition() {
    if (mouseTracking && currentZoomLevel > 0) {
        SetWindowPos(
            hwndMagnifier,
            HWND_TOPMOST,
            lastMousePos.x - windowWidth / 2,
            lastMousePos.y - windowHeight / 2,
            0, 0,
            SWP_NOSIZE | SWP_NOACTIVATE
        );
    }
}

/*============== UPDATE MAGNIFIER CONTENT ==============*/
void UpdateMagnifierContent() {
    if (currentZoomLevel > 0) {
        if (mouseTracking) {
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            float zoom = zoomLevels[currentZoomLevel - 1];
            int scaledSize = (int)(zoomAreaSize / zoom);

            RECT sourceRect = {
                lastMousePos.x - (scaledSize / 2) + (int)(currentAdjustment.x / zoom),
                lastMousePos.y - (scaledSize / 2) + (int)(currentAdjustment.y / zoom),
                lastMousePos.x + (scaledSize / 2) + (int)(currentAdjustment.x / zoom),
                lastMousePos.y + (scaledSize / 2) + (int)(currentAdjustment.y / zoom)
            };
            MagSetWindowSource(hwndMag, sourceRect);
        }
        else {
            MagSetWindowSource(hwndMag, sourceRects[currentZoomLevel - 1]);
        }
    }
}

/*============== APPLY ZOOM LEVEL ==============*/
void ApplyZoomLevel(int newZoomLevel) {
    if (newZoomLevel == currentZoomLevel) return;

    currentZoomLevel = newZoomLevel;

    if (currentZoomLevel > 0) {
        float zoom = zoomLevels[currentZoomLevel - 1];

        // Apply zoom transform
        MAGTRANSFORM matrix = { {
            {zoom, 0, 0},
            {0, zoom, 0},
            {0, 0, 1}
        } };
        MagSetWindowTransform(hwndMag, &matrix);

        // Update source rectangles
        CalculateSourceRects();
        UpdateMagnifierContent();

        ShowWindow(hwndMagnifier, SW_SHOW);
        if (mouseTracking) {
            UpdateMagnifierPosition();
        }
    }
    else {
        ShowWindow(hwndMagnifier, SW_HIDE);
    }
}

/*============== LOAD CUSTOM ICON ==============*/
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

/*============== SHOW ALREADY RUNNING MESSAGE ==============*/
void ShowAlreadyRunningMessage() {
    MessageBox(NULL,
        L"Magnifier+ is already running.\n\n"
        L"Only one instance of the application can run at a time.\n"
        L"Check your system tray for the running instance.",
        L"Magnifier+ Already Running",
        MB_OK | MB_ICONINFORMATION);
}

/*============== STARTUP INFORMATION ==============*/
void ShowStartupInfo() {
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

/*============== MAIN WINDOW PROCEDURE ==============*/
LRESULT CALLBACK MagnifierWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_TIMER:
        UpdateMagnifierContent();
        break;

    case WM_DESTROY:
        UnhookWindowsHookEx(hMouseHook);
        UnhookWindowsHookEx(hKeyboardHook);
        Shell_NotifyIcon(NIM_DELETE, &nid);
        if (hCircleRegion) DeleteObject(hCircleRegion);
        if (hMutex) {
            CloseHandle(hMutex);
            hMutex = NULL;
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

/*============== INPUT HANDLERS ==============*/
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && isRightMouseDown) {
        KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;
        bool changed = false;

        if (wParam == WM_KEYDOWN) {
            switch (pKey->vkCode) {
            case VK_LEFT:  currentAdjustment.x -= moveStep; changed = true; break;
            case VK_RIGHT: currentAdjustment.x += moveStep; changed = true; break;
            case VK_UP:    currentAdjustment.y -= moveStep; changed = true; break;
            case VK_DOWN:  currentAdjustment.y += moveStep; changed = true; break;
            }
        }

        if (changed && currentZoomLevel > 0) {
            CalculateSourceRects();
            UpdateMagnifierContent();
            UpdateWindow(hwndMag);
        }
    }
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;

        switch (wParam) {
        case WM_RBUTTONDOWN:
            isRightMouseDown = true;
            if (currentZoomLevel > 0) {
                ShowWindow(hwndMagnifier, SW_SHOW);
                if (mouseTracking) {
                    lastMousePos = pMouse->pt;
                    UpdateMagnifierPosition();
                }
            }
            break;

        case WM_RBUTTONUP:
            isRightMouseDown = false;
            ShowWindow(hwndMagnifier, SW_HIDE);
            break;

        case WM_MOUSEWHEEL:
            if (isRightMouseDown) {
                int delta = GET_WHEEL_DELTA_WPARAM(pMouse->mouseData);
                int newZoomLevel = delta > 0 ? min(currentZoomLevel + 1, 5) : max(currentZoomLevel - 1, 0);
                ApplyZoomLevel(newZoomLevel);
                // Block wheel events from reaching other windows when tracking mouse
                if (mouseTracking) return 1;
            }
            break;

        case WM_MOUSEMOVE:
            if (mouseTracking && isRightMouseDown && currentZoomLevel > 0) {
                lastMousePos = pMouse->pt;
                UpdateMagnifierPosition();
            }
            break;
        }
    }
    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

/*============== LOAD CONFIGURATION ==============*/
void LoadConfig() {
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

    CalculateInitialAdjustment();
    CalculateSourceRects();
}

/*============== APPLICATION ENTRY POINT ==============*/
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPTSTR lpCmdLine, int nCmdShow) {
    // Check for existing instance
    hMutex = CreateMutex(NULL, TRUE, L"MagnifierPlusInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        ShowAlreadyRunningMessage();
        return 0;
    }

    // Load configuration
    LoadConfig();

    // Show startup information
    ShowStartupInfo();

    if (!MagInitialize()) {
        MessageBox(NULL, L"Failed to initialize magnification API!", L"Error", MB_ICONERROR);
        return -1;
    }

    // Set input hooks
    hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, hInstance, 0);
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, hInstance, 0);
    if (!hMouseHook || !hKeyboardHook) {
        MessageBox(NULL, L"Failed to set input hooks!", L"Error", MB_ICONERROR);
        MagUninitialize();
        return -1;
    }

    // Register window class
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

    // Create main window
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    hwndMagnifier = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        L"MagnifierWindowClass",
        L"Magnifier+ (Right-click + Scroll: Zoom | Arrows: Move)",
        WS_POPUP,
        mouseTracking ? 0 : (screenWidth - windowWidth) / 2,
        mouseTracking ? 0 : (screenHeight - windowHeight) / 2,
        windowWidth, windowHeight,
        NULL, NULL, hInstance, NULL
    );

    if (!hwndMagnifier) {
        MessageBox(NULL, L"Window creation failed!", L"Error", MB_ICONERROR);
        MagUninitialize();
        return -1;
    }

    // Create circular region if needed
    if (circularMode) {
        CreateCircularRegion();
        SetWindowRgn(hwndMagnifier, hCircleRegion, TRUE);
    }

    // Create magnifier control
    hwndMag = CreateWindow(
        WC_MAGNIFIER, NULL,
        WS_CHILD | WS_VISIBLE,
        0, 0, windowWidth, windowHeight,
        hwndMagnifier, NULL, hInstance, NULL
    );

    if (!hwndMag) {
        MessageBox(NULL, L"Failed to create magnifier control!", L"Error", MB_ICONERROR);
        DestroyWindow(hwndMagnifier);
        MagUninitialize();
        return -1;
    }

    // Set initial state
    ApplyZoomLevel(0);

    // Setup tray icon
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwndMagnifier;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = LoadCustomIcon();
    wcscpy_s(nid.szTip, L"Magnifier+ (Right-click to exit)");
    Shell_NotifyIcon(NIM_ADD, &nid);

    // Main loop
    SetTimer(hwndMagnifier, 1, 1000 / refreshRate, NULL);
    ShowWindow(hwndMagnifier, SW_HIDE);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    KillTimer(hwndMagnifier, 1);
    MagUninitialize();
    return (int)msg.wParam;
}