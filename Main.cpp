/*===============================================
                MAGNIFIER+
  ===============================================
  A lightweight screen magnifier with:
  - Smooth 5-level zoom (1x to 5x)
  - Position adjustment controls
  - System tray accessibility
  - Custom targeting reticle
  - Configurable settings via INI file
===============================================*/

// Windows headers
#include <windows.h>
#include <magnification.h>  // Magnification API
#include <tchar.h>          // Unicode support
#include <shlwapi.h>        // Path manipulation

// Link necessary libraries
#pragma comment(lib, "shlwapi.lib")      // For Path functions
#pragma comment(lib, "Magnification.lib") // For magnification API

// Control IDs for configuration dialog
#define IDC_WIDTH         1001
#define IDC_HEIGHT        1002
#define IDC_ZOOM_SIZE     1003
#define IDC_OFFSET_X      1004
#define IDC_OFFSET_Y      1005
#define IDC_FPS           1006
#define IDC_OFFSET_STEP   1007
#define IDOK              1
#define IDCANCEL          2

/*============== CONFIGURATION DEFAULTS ==============*/
#define DEFAULT_WINDOW_WIDTH      300     // Default window width
#define DEFAULT_WINDOW_HEIGHT     300     // Default window height
#define DEFAULT_BASE_ZOOM_SIZE    100     // Base size of zoom area
#define DEFAULT_INITIAL_LEFT_OFFSET -100  // Initial X offset
#define DEFAULT_INITIAL_DOWN_OFFSET -100  // Initial Y offset
#define DEFAULT_TARGET_FPS        71      // Refresh rate
#define DEFAULT_OFFSET_STEP       5       // Move step size

/*============== GLOBAL VARIABLES ==============*/
// Configuration settings
int windowWidth = DEFAULT_WINDOW_WIDTH;
int windowHeight = DEFAULT_WINDOW_HEIGHT;
int baseZoomSize = DEFAULT_BASE_ZOOM_SIZE;
int initialLeftOffset = DEFAULT_INITIAL_LEFT_OFFSET;
int initialDownOffset = DEFAULT_INITIAL_DOWN_OFFSET;
int targetFPS = DEFAULT_TARGET_FPS;
int offsetStep = DEFAULT_OFFSET_STEP;

// INI file management
WCHAR configPath[MAX_PATH];  // Path to config file
bool iniFileExists = false;  // Flag if config exists

// Zoom levels (1x to 5x)
float zoomLevels[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
int currentZoomLevel = 1;     // Current zoom level index

// Position tracking
int currentLeftOffset = DEFAULT_INITIAL_LEFT_OFFSET;
int currentDownOffset = DEFAULT_INITIAL_DOWN_OFFSET;
RECT sourceRects[5];          // Source rectangles for each zoom level

// Window handles
HWND hwndMagnifier;          // Main window handle
HWND hwndMag;                // Magnifier control handle

// Input tracking
bool isRightMouseDown = false;  // Right mouse button state
HHOOK hMouseHook;             // Mouse hook handle
HHOOK hKeyboardHook;          // Keyboard hook handle

// System tray icon
NOTIFYICONDATA nid = { 0 };

/*============== CONFIGURATION FUNCTIONS ==============*/

// Show comprehensive startup information
void ShowStartupInfo() {
    WCHAR message[1024];
    WCHAR exePath[MAX_PATH];

    // Get executable path
    GetModuleFileName(NULL, exePath, MAX_PATH);

    // Format information message
    swprintf(message, 1024,
        L"Magnifier+ Initialization\n\n"
        L"Executable path: %s\n"
        L"Configuration file: %s\n\n"
        L"Current Settings:\n"
        L"- Window Size: %dx%d pixels\n"
        L"- Zoom Area: %d pixels\n"
        L"- Initial Offset: (%d, %d)\n"
        L"- Refresh Rate: %d FPS\n"
        L"- Move Step Size: %d pixels\n\n"
        L"Controls:\n"
        L"1. Right-click + Mouse Wheel: Change zoom level\n"
        L"2. Right-click + Arrow Keys: Move viewport\n"
        L"3. Right-click tray icon: Exit application\n\n"
        L"Configuration: %s",
        exePath,
        configPath,
        windowWidth, windowHeight,
        baseZoomSize,
        initialLeftOffset, initialDownOffset,
        targetFPS,
        offsetStep,
        iniFileExists ? L"Loaded from INI file" : L"Using default values");

    // Show information message box
    MessageBox(NULL, message, L"Magnifier+ Information", MB_OK | MB_ICONINFORMATION);
}

// Load configuration from INI file or use defaults
void LoadConfig() {
    // Get executable path and construct INI file path
    GetModuleFileName(NULL, configPath, MAX_PATH);
    PathRemoveFileSpec(configPath);
    PathAppend(configPath, L"MagnifierPlus.ini");

    // Check if INI file exists
    iniFileExists = PathFileExists(configPath);

    // Load values from INI or use defaults
    windowWidth = GetPrivateProfileInt(L"Settings", L"WindowWidth", DEFAULT_WINDOW_WIDTH, configPath);
    windowHeight = GetPrivateProfileInt(L"Settings", L"WindowHeight", DEFAULT_WINDOW_HEIGHT, configPath);
    baseZoomSize = GetPrivateProfileInt(L"Settings", L"BaseZoomSize", DEFAULT_BASE_ZOOM_SIZE, configPath);
    initialLeftOffset = GetPrivateProfileInt(L"Settings", L"InitialLeftOffset", DEFAULT_INITIAL_LEFT_OFFSET, configPath);
    initialDownOffset = GetPrivateProfileInt(L"Settings", L"InitialDownOffset", DEFAULT_INITIAL_DOWN_OFFSET, configPath);
    targetFPS = GetPrivateProfileInt(L"Settings", L"TargetFPS", DEFAULT_TARGET_FPS, configPath);
    offsetStep = GetPrivateProfileInt(L"Settings", L"OffsetStep", DEFAULT_OFFSET_STEP, configPath);

    // Set current offsets to initial values
    currentLeftOffset = initialLeftOffset;
    currentDownOffset = initialDownOffset;
}

// Save current configuration to INI file
void SaveConfig() {
    WCHAR buffer[32];

    // Write all settings to INI file
    swprintf(buffer, 32, L"%d", windowWidth);
    WritePrivateProfileString(L"Settings", L"WindowWidth", buffer, configPath);

    swprintf(buffer, 32, L"%d", windowHeight);
    WritePrivateProfileString(L"Settings", L"WindowHeight", buffer, configPath);

    swprintf(buffer, 32, L"%d", baseZoomSize);
    WritePrivateProfileString(L"Settings", L"BaseZoomSize", buffer, configPath);

    swprintf(buffer, 32, L"%d", initialLeftOffset);
    WritePrivateProfileString(L"Settings", L"InitialLeftOffset", buffer, configPath);

    swprintf(buffer, 32, L"%d", initialDownOffset);
    WritePrivateProfileString(L"Settings", L"InitialDownOffset", buffer, configPath);

    swprintf(buffer, 32, L"%d", targetFPS);
    WritePrivateProfileString(L"Settings", L"TargetFPS", buffer, configPath);

    swprintf(buffer, 32, L"%d", offsetStep);
    WritePrivateProfileString(L"Settings", L"OffsetStep", buffer, configPath);

    // Show confirmation
    MessageBox(NULL, L"Settings saved successfully!", L"Configuration Saved", MB_OK | MB_ICONINFORMATION);
}

/*============== CORE FUNCTIONS ==============*/

// Calculate source rectangles for all zoom levels
void CalculateSourceRects() {
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    POINT center = { screenWidth / 2, screenHeight / 2 };

    // Calculate aspect ratio compensation
    float windowAspect = (float)windowWidth / windowHeight;
    float baseAspect = 1.0f; // Original was square (300x300)

    // Calculate effective size maintaining original proportions
    int effectiveSize = (int)(baseZoomSize * (windowAspect > 1.0f ? 1.0f / windowAspect : 1.0f));

    for (int i = 0; i < 5; i++) {
        float zoom = zoomLevels[i];
        int scaledWidth = (int)(effectiveSize / zoom);
        int scaledHeight = (int)(effectiveSize / zoom);

        // Adjust for actual window dimensions
        if (windowAspect > 1.0f) {
            scaledHeight = (int)(scaledHeight / windowAspect); // Wider window
        }
        else {
            scaledWidth = (int)(scaledWidth * windowAspect);  // Taller window
        }

        // Calculate rectangle for this zoom level
        sourceRects[i] = {
            center.x - (scaledWidth / 2) + (int)(currentLeftOffset / zoom),
            center.y - (scaledHeight / 2) + (int)(currentDownOffset / zoom),
            center.x + (scaledWidth / 2) + (int)(currentLeftOffset / zoom),
            center.y + (scaledHeight / 2) + (int)(currentDownOffset / zoom)
        };
    }
}

// Load or create the program icon
HICON LoadProgramIcon() {
    WCHAR exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);
    PathRemoveFileSpec(exePath);
    PathAppend(exePath, L"reticle.ico");

    // Try to load icon from file
    HICON hIcon = (HICON)LoadImage(
        NULL, exePath, IMAGE_ICON,
        32, 32, LR_LOADFROMFILE | LR_DEFAULTSIZE
    );

    // If file not found, create a simple crosshair icon
    if (!hIcon) {
        HDC hdc = GetDC(NULL);
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hColorBmp = CreateCompatibleBitmap(hdc, 32, 32);
        SelectObject(hdcMem, hColorBmp);

        // Draw red crosshair
        HPEN hPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
        SelectObject(hdcMem, hPen);
        MoveToEx(hdcMem, 6, 16, NULL);
        LineTo(hdcMem, 26, 16);
        MoveToEx(hdcMem, 16, 6, NULL);
        LineTo(hdcMem, 16, 26);

        // Create icon from bitmap
        ICONINFO ii = { 0 };
        ii.fIcon = TRUE;
        ii.hbmColor = hColorBmp;
        ii.hbmMask = CreateBitmap(32, 32, 1, 1, NULL);
        hIcon = CreateIconIndirect(&ii);

        // Cleanup
        DeleteObject(hPen);
        DeleteObject(hColorBmp);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdc);
    }

    return hIcon;
}

/*============== INPUT HANDLING ==============*/

// Keyboard hook procedure for arrow key navigation
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && isRightMouseDown) {
        KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;
        bool offsetChanged = false;

        // Handle arrow key presses
        if (wParam == WM_KEYDOWN) {
            switch (pKey->vkCode) {
            case VK_LEFT:  currentLeftOffset -= offsetStep; offsetChanged = true; break;
            case VK_RIGHT: currentLeftOffset += offsetStep; offsetChanged = true; break;
            case VK_UP:    currentDownOffset -= offsetStep; offsetChanged = true; break;
            case VK_DOWN:  currentDownOffset += offsetStep; offsetChanged = true; break;
            }
        }

        // Update view if position changed
        if (offsetChanged) {
            CalculateSourceRects();
            if (currentZoomLevel > 1) {
                MagSetWindowSource(hwndMag, sourceRects[currentZoomLevel - 1]);
                UpdateWindow(hwndMag);
            }
        }
    }
    return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

// Mouse hook procedure for zoom control
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;

        switch (wParam) {
        case WM_RBUTTONDOWN:
            isRightMouseDown = true;
            if (currentZoomLevel > 1) {
                ShowWindow(hwndMagnifier, SW_SHOW);
            }
            break;

        case WM_RBUTTONUP:
            isRightMouseDown = false;
            ShowWindow(hwndMagnifier, SW_HIDE);
            break;

        case WM_MOUSEWHEEL:
            if (isRightMouseDown) {
                // Change zoom level based on wheel direction
                int delta = GET_WHEEL_DELTA_WPARAM(pMouse->mouseData);
                currentZoomLevel = delta > 0 ? min(currentZoomLevel + 1, 5)
                    : max(currentZoomLevel - 1, 1);

                if (currentZoomLevel > 1) {
                    // Apply zoom transform
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

/*============== WINDOW PROCEDURE ==============*/
LRESULT CALLBACK MagnifierWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_TIMER:
        // Update magnifier source while active
        if (isRightMouseDown && currentZoomLevel > 1) {
            MagSetWindowSource(hwndMag, sourceRects[currentZoomLevel - 1]);
        }
        break;

    case WM_DESTROY:
        // Cleanup hooks and tray icon
        UnhookWindowsHookEx(hMouseHook);
        UnhookWindowsHookEx(hKeyboardHook);
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
        break;

    case WM_USER + 1:  // Tray icon messages
        if (lParam == WM_RBUTTONUP) {
            DestroyWindow(hwnd);
        }
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

/*============== ENTRY POINT ==============*/
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPTSTR lpCmdLine, int nCmdShow) {
    // Load configuration
    LoadConfig();

    // Show startup information
    ShowStartupInfo();

    // Initialize zoom areas
    CalculateSourceRects();

    // Initialize magnification API
    if (!MagInitialize()) {
        MessageBox(NULL, _T("Failed to initialize magnification API!"),
            _T("Magnifier+ Error"), MB_ICONERROR);
        return -1;
    }

    // Set up input hooks
    hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, hInstance, 0);
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, hInstance, 0);

    if (!hMouseHook || !hKeyboardHook) {
        MessageBox(NULL, _T("Failed to initialize input hooks!"),
            _T("Magnifier+ Error"), MB_ICONERROR);
        MagUninitialize();
        return -1;
    }

    // Register window class
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = MagnifierWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = _T("MagnifierWindowClass");
    wc.hIcon = LoadProgramIcon();
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, _T("Window registration failed!"),
            _T("Magnifier+ Error"), MB_ICONERROR);
        MagUninitialize();
        return -1;
    }

    // Get screen dimensions for centering
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Create main window
    hwndMagnifier = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        _T("MagnifierWindowClass"),
        _T("Magnifier+ (Right-click + Scroll: Zoom | Right-click + Arrows: Move)"),
        WS_POPUP,
        (screenWidth - windowWidth) / 2,
        (screenHeight - windowHeight) / 2,
        windowWidth, windowHeight,
        NULL, NULL, hInstance, NULL
    );

    if (!hwndMagnifier) {
        MessageBox(NULL, _T("Window creation failed!"),
            _T("Magnifier+ Error"), MB_ICONERROR);
        MagUninitialize();
        return -1;
    }

    // Set window attributes
    SetWindowLong(hwndMagnifier, GWL_EXSTYLE,
        GetWindowLong(hwndMagnifier, GWL_EXSTYLE) |
        WS_EX_TRANSPARENT | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwndMagnifier, 0, 255, LWA_ALPHA);

    // Create magnifier control
    hwndMag = CreateWindow(
        WC_MAGNIFIER, NULL,
        WS_CHILD | WS_VISIBLE,
        0, 0, windowWidth, windowHeight,
        hwndMagnifier, NULL, hInstance, NULL
    );

    if (!hwndMag) {
        MessageBox(NULL, _T("Failed to create magnifier control!"),
            _T("Magnifier+ Error"), MB_ICONERROR);
        MagUninitialize();
        return -1;
    }

    // Set initial zoom transform
    MAGTRANSFORM matrix = { {
        {zoomLevels[currentZoomLevel - 1], 0, 0},
        {0, zoomLevels[currentZoomLevel - 1], 0},
        {0, 0, 1}
    } };
    MagSetWindowTransform(hwndMag, &matrix);

    // Set up system tray icon
    nid.cbSize = sizeof(nid);
    nid.hWnd = hwndMagnifier;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = LoadProgramIcon();
    wcscpy_s(nid.szTip, L"Magnifier+ (Right-click to exit)");
    Shell_NotifyIcon(NIM_ADD, &nid);

    // Set refresh timer
    SetTimer(hwndMagnifier, 1, 1000 / targetFPS, NULL);
    ShowWindow(hwndMagnifier, SW_HIDE);  // Start hidden

    // Main message loop
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