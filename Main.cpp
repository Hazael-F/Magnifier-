/*===============================================
                MAGNIFIER+
  ===============================================
  A lightweight screen magnifier with:
  - Smooth 5-level zoom (1x to 5x)
  - Position adjustment controls
  - System tray accessibility
  - Precise offset configuration
  - Configurable settings via INI file
  - Optional circular window shape
===============================================*/

#include <windows.h>
#include <magnification.h>
#include <tchar.h>
#include <shlwapi.h>
#include <math.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Magnification.lib")

// Configuration defaults
#define DEFAULT_WINDOW_WIDTH      300
#define DEFAULT_WINDOW_HEIGHT     300
#define DEFAULT_BASE_ZOOM_SIZE    100
#define DEFAULT_TARGET_FPS        60
#define DEFAULT_OFFSET_STEP       5
#define DEFAULT_CIRCULAR_MODE     false

// Default manual adjustments (in steps)
#define DEFAULT_HORIZONTAL_ADJUST  13
#define DEFAULT_VERTICAL_ADJUST    13

// Global variables
int windowWidth = DEFAULT_WINDOW_WIDTH;
int windowHeight = DEFAULT_WINDOW_HEIGHT;
int baseZoomSize = DEFAULT_BASE_ZOOM_SIZE;
int targetFPS = DEFAULT_TARGET_FPS;
int offsetStep = DEFAULT_OFFSET_STEP;
int horizontalAdjust = DEFAULT_HORIZONTAL_ADJUST;
int verticalAdjust = DEFAULT_VERTICAL_ADJUST;
bool circularMode = DEFAULT_CIRCULAR_MODE;

float zoomLevels[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
int currentZoomLevel = 1;
POINT currentOffset = { 0, 0 };
RECT sourceRects[5];

HWND hwndMagnifier, hwndMag;
bool isRightMouseDown = false;
HHOOK hMouseHook, hKeyboardHook;
NOTIFYICONDATA nid = { 0 };
HRGN hCircleRegion = NULL;

/*============== CREATE CIRCULAR REGION ==============*/
void CreateCircularRegion() {
    if (hCircleRegion) {
        DeleteObject(hCircleRegion);
    }

    // Create a circular region
    hCircleRegion = CreateEllipticRgn(0, 0, windowWidth, windowHeight);
}

/*============== PRECISE OFFSET CALCULATION ==============*/
void CalculateInitialOffset() {
    // Base offset calculation
    if (windowWidth == 300 && windowHeight == 300) {
        currentOffset.x = -100;
        currentOffset.y = -100;
    }
    else if (windowWidth == 600 && windowHeight == 600) {
        currentOffset.x = -250;
        currentOffset.y = -250;
    }
    else {
        // Linear scaling for other sizes
        float scale = (float)windowWidth / 300.0f;
        currentOffset.x = (int)(-100 * scale);
        currentOffset.y = currentOffset.x;
    }

    // Apply manual adjustments (in steps)
    currentOffset.x += (horizontalAdjust * offsetStep);
    currentOffset.y += (verticalAdjust * offsetStep);
}

/*============== MAGNIFICATION SOURCE CALCULATION ==============*/
void CalculateSourceRects() {
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    POINT center = { screenWidth / 2, screenHeight / 2 };

    for (int i = 0; i < 5; i++) {
        float zoom = zoomLevels[i];
        int scaledSize = (int)(baseZoomSize / zoom);

        sourceRects[i] = {
            center.x - (scaledSize / 2) + (int)(currentOffset.x / zoom),
            center.y - (scaledSize / 2) + (int)(currentOffset.y / zoom),
            center.x + (scaledSize / 2) + (int)(currentOffset.x / zoom),
            center.y + (scaledSize / 2) + (int)(currentOffset.y / zoom)
        };
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
        L"- Window: %dx%d pixels\n"
        L"- Shape: %s\n"
        L"- Zoom Area: %d pixels\n"
        L"- Initial Offset: (%d, %d)\n"
        L"- Adjustments: %dH, %dV steps\n"
        L"- Move Step: %d pixels\n"
        L"- Refresh Rate: %d FPS\n\n"
        L"Controls:\n"
        L"1. Right-click + Scroll: Zoom (1x-5x)\n"
        L"2. Right-click + Arrows: Move view\n"
        L"3. Right-click tray icon: Exit",
        path, windowWidth, windowHeight,
        circularMode ? L"Circle" : L"Square",
        baseZoomSize,
        currentOffset.x, currentOffset.y,
        horizontalAdjust, verticalAdjust,
        offsetStep, targetFPS);

    MessageBox(NULL, message, L"Magnifier+ Ready", MB_OK | MB_ICONINFORMATION);
}

/*============== MAIN WINDOW PROCEDURE ==============*/
LRESULT CALLBACK MagnifierWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_TIMER:
        if (isRightMouseDown && currentZoomLevel > 1) {
            MagSetWindowSource(hwndMag, sourceRects[currentZoomLevel - 1]);
        }
        break;

    case WM_DESTROY:
        UnhookWindowsHookEx(hMouseHook);
        UnhookWindowsHookEx(hKeyboardHook);
        Shell_NotifyIcon(NIM_DELETE, &nid);
        if (hCircleRegion) DeleteObject(hCircleRegion);
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
            case VK_LEFT:  currentOffset.x -= offsetStep; changed = true; break;
            case VK_RIGHT: currentOffset.x += offsetStep; changed = true; break;
            case VK_UP:    currentOffset.y -= offsetStep; changed = true; break;
            case VK_DOWN:  currentOffset.y += offsetStep; changed = true; break;
            }
        }

        if (changed) {
            CalculateSourceRects();
            if (currentZoomLevel > 1) {
                MagSetWindowSource(hwndMag, sourceRects[currentZoomLevel - 1]);
                UpdateWindow(hwndMag);
            }
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
            if (currentZoomLevel > 1) ShowWindow(hwndMagnifier, SW_SHOW);
            break;

        case WM_RBUTTONUP:
            isRightMouseDown = false;
            ShowWindow(hwndMagnifier, SW_HIDE);
            break;

        case WM_MOUSEWHEEL:
            if (isRightMouseDown) {
                int delta = GET_WHEEL_DELTA_WPARAM(pMouse->mouseData);
                currentZoomLevel = delta > 0 ? min(currentZoomLevel + 1, 5) : max(currentZoomLevel - 1, 1);

                if (currentZoomLevel > 1) {
                    MAGTRANSFORM matrix = { {
                        {zoomLevels[currentZoomLevel - 1], 0, 0},
                        {0, zoomLevels[currentZoomLevel - 1], 0},
                        {0, 0, 1}
                    } };
                    MagSetWindowTransform(hwndMag, &matrix);
                    ShowWindow(hwndMagnifier, SW_SHOW);
                }
                else {
                    ShowWindow(hwndMagnifier, SW_HIDE);
                }
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
    baseZoomSize = GetPrivateProfileInt(L"Zoom", L"Size", DEFAULT_BASE_ZOOM_SIZE, configPath);
    targetFPS = GetPrivateProfileInt(L"Performance", L"FPS", DEFAULT_TARGET_FPS, configPath);
    offsetStep = GetPrivateProfileInt(L"Movement", L"StepSize", DEFAULT_OFFSET_STEP, configPath);
    horizontalAdjust = GetPrivateProfileInt(L"Adjustments", L"Horizontal", DEFAULT_HORIZONTAL_ADJUST, configPath);
    verticalAdjust = GetPrivateProfileInt(L"Adjustments", L"Vertical", DEFAULT_VERTICAL_ADJUST, configPath);

    // Load circular mode setting
    circularMode = GetPrivateProfileInt(L"Window", L"Circular", DEFAULT_CIRCULAR_MODE, configPath) != 0;

    CalculateInitialOffset();
    CalculateSourceRects();
}

/*============== APPLICATION ENTRY POINT ==============*/
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPTSTR lpCmdLine, int nCmdShow) {
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
        (screenWidth - windowWidth) / 2,
        (screenHeight - windowHeight) / 2,
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

    // Set initial transform
    MAGTRANSFORM matrix = { {
        {zoomLevels[currentZoomLevel - 1], 0, 0},
        {0, zoomLevels[currentZoomLevel - 1], 0},
        {0, 0, 1}
    } };
    MagSetWindowTransform(hwndMag, &matrix);

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
    SetTimer(hwndMagnifier, 1, 1000 / targetFPS, NULL);
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