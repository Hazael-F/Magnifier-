/*===============================================
                MAGNIFIER+
  ===============================================
  A lightweight screen magnifier with:
  - Smooth 5-level zoom (1x to 5x)
  - Position adjustment controls
  - System tray accessibility
  - Custom targeting reticle
  - Configurable settings via INI/dialog
===============================================*/

#include <windows.h>
#include <magnification.h>
#include <tchar.h>
#include <shlwapi.h>

// Link necessary libraries
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "Magnification.lib")

// Control IDs for the configuration dialog
#define IDC_WIDTH         1001
#define IDC_HEIGHT        1002
#define IDC_ZOOM_SIZE     1003
#define IDC_OFFSET_X      1004
#define IDC_OFFSET_Y      1005
#define IDC_FPS           1006
#define IDC_OFFSET_STEP   1007
#define IDOK              1
#define IDCANCEL          2

/*============== CONFIGURATION ==============*/
// Default values
#define DEFAULT_WINDOW_WIDTH      300
#define DEFAULT_WINDOW_HEIGHT     300
#define DEFAULT_BASE_ZOOM_SIZE    100
#define DEFAULT_INITIAL_LEFT_OFFSET -100
#define DEFAULT_INITIAL_DOWN_OFFSET -100
#define DEFAULT_TARGET_FPS        71
#define DEFAULT_OFFSET_STEP       5

// Global config variables
int windowWidth = DEFAULT_WINDOW_WIDTH;
int windowHeight = DEFAULT_WINDOW_HEIGHT;
int baseZoomSize = DEFAULT_BASE_ZOOM_SIZE;
int initialLeftOffset = DEFAULT_INITIAL_LEFT_OFFSET;
int initialDownOffset = DEFAULT_INITIAL_DOWN_OFFSET;
int targetFPS = DEFAULT_TARGET_FPS;
int offsetStep = DEFAULT_OFFSET_STEP;

// INI file path
WCHAR configPath[MAX_PATH];

/*============== GLOBAL STATE ==============*/
float zoomLevels[] = { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f };
int currentZoomLevel = 1;
int currentLeftOffset = DEFAULT_INITIAL_LEFT_OFFSET;
int currentDownOffset = DEFAULT_INITIAL_DOWN_OFFSET;
RECT sourceRects[5];

// Window handles
HWND hwndMagnifier;
HWND hwndMag;

// Input tracking
bool isRightMouseDown = false;
HHOOK hMouseHook;
HHOOK hKeyboardHook;

// System tray icon
NOTIFYICONDATA nid = { 0 };

/*============== CONFIGURATION FUNCTIONS ==============*/

void LoadConfig() {
    GetModuleFileName(NULL, configPath, MAX_PATH);
    PathRemoveFileSpec(configPath);
    PathAppend(configPath, L"MagnifierPlus.ini");

    windowWidth = GetPrivateProfileInt(L"Settings", L"WindowWidth", DEFAULT_WINDOW_WIDTH, configPath);
    windowHeight = GetPrivateProfileInt(L"Settings", L"WindowHeight", DEFAULT_WINDOW_HEIGHT, configPath);
    baseZoomSize = GetPrivateProfileInt(L"Settings", L"BaseZoomSize", DEFAULT_BASE_ZOOM_SIZE, configPath);
    initialLeftOffset = GetPrivateProfileInt(L"Settings", L"InitialLeftOffset", DEFAULT_INITIAL_LEFT_OFFSET, configPath);
    initialDownOffset = GetPrivateProfileInt(L"Settings", L"InitialDownOffset", DEFAULT_INITIAL_DOWN_OFFSET, configPath);
    targetFPS = GetPrivateProfileInt(L"Settings", L"TargetFPS", DEFAULT_TARGET_FPS, configPath);
    offsetStep = GetPrivateProfileInt(L"Settings", L"OffsetStep", DEFAULT_OFFSET_STEP, configPath);

    currentLeftOffset = initialLeftOffset;
    currentDownOffset = initialDownOffset;
}

void SaveConfig() {
    WCHAR buffer[32];

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
}

/*============== DIALOG FUNCTIONS ==============*/

// Modified dialog procedure to fix the warning
INT_PTR CALLBACK ConfigDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG:
        SetDlgItemInt(hDlg, IDC_WIDTH, windowWidth, FALSE);
        SetDlgItemInt(hDlg, IDC_HEIGHT, windowHeight, FALSE);
        SetDlgItemInt(hDlg, IDC_ZOOM_SIZE, baseZoomSize, FALSE);
        SetDlgItemInt(hDlg, IDC_OFFSET_X, initialLeftOffset, TRUE);
        SetDlgItemInt(hDlg, IDC_OFFSET_Y, initialDownOffset, TRUE);
        SetDlgItemInt(hDlg, IDC_FPS, targetFPS, FALSE);
        SetDlgItemInt(hDlg, IDC_OFFSET_STEP, offsetStep, FALSE);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            windowWidth = GetDlgItemInt(hDlg, IDC_WIDTH, NULL, FALSE);
            windowHeight = GetDlgItemInt(hDlg, IDC_HEIGHT, NULL, FALSE);
            baseZoomSize = GetDlgItemInt(hDlg, IDC_ZOOM_SIZE, NULL, FALSE);
            initialLeftOffset = GetDlgItemInt(hDlg, IDC_OFFSET_X, NULL, TRUE);
            initialDownOffset = GetDlgItemInt(hDlg, IDC_OFFSET_Y, NULL, TRUE);
            targetFPS = GetDlgItemInt(hDlg, IDC_FPS, NULL, FALSE);
            offsetStep = GetDlgItemInt(hDlg, IDC_OFFSET_STEP, NULL, FALSE);

            currentLeftOffset = initialLeftOffset;
            currentDownOffset = initialDownOffset;

            SaveConfig();
            EndDialog(hDlg, IDOK);
            return (INT_PTR)TRUE;

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

/*============== CORE FUNCTIONS ==============*/

void CalculateSourceRects() {
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    POINT center = { screenWidth / 2, screenHeight / 2 };

    for (int i = 0; i < 5; i++) {
        float zoom = zoomLevels[i];
        int scaledSize = (int)(baseZoomSize / zoom);

        sourceRects[i] = {
            center.x - (scaledSize / 2) + (int)(currentLeftOffset / zoom),
            center.y - (scaledSize / 2) + (int)(currentDownOffset / zoom),
            center.x + (scaledSize / 2) + (int)(currentLeftOffset / zoom),
            center.y + (scaledSize / 2) + (int)(currentDownOffset / zoom)
        };
    }
}

HICON LoadProgramIcon() {
    WCHAR exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);
    PathRemoveFileSpec(exePath);
    PathAppend(exePath, L"reticle.ico");

    HICON hIcon = (HICON)LoadImage(
        NULL, exePath, IMAGE_ICON,
        32, 32, LR_LOADFROMFILE | LR_DEFAULTSIZE
    );

    if (!hIcon) {
        HDC hdc = GetDC(NULL);
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hColorBmp = CreateCompatibleBitmap(hdc, 32, 32);
        SelectObject(hdcMem, hColorBmp);

        HPEN hPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
        SelectObject(hdcMem, hPen);
        MoveToEx(hdcMem, 6, 16, NULL);
        LineTo(hdcMem, 26, 16);
        MoveToEx(hdcMem, 16, 6, NULL);
        LineTo(hdcMem, 16, 26);

        ICONINFO ii = { 0 };
        ii.fIcon = TRUE;
        ii.hbmColor = hColorBmp;
        ii.hbmMask = CreateBitmap(32, 32, 1, 1, NULL);
        hIcon = CreateIconIndirect(&ii);

        DeleteObject(hPen);
        DeleteObject(hColorBmp);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdc);
    }

    return hIcon;
}

/*============== INPUT HANDLING ==============*/

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && isRightMouseDown) {
        KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;
        bool offsetChanged = false;

        switch (wParam) {
        case WM_KEYDOWN:
            switch (pKey->vkCode) {
            case VK_LEFT:  currentLeftOffset -= offsetStep; offsetChanged = true; break;
            case VK_RIGHT: currentLeftOffset += offsetStep; offsetChanged = true; break;
            case VK_UP:    currentDownOffset -= offsetStep; offsetChanged = true; break;
            case VK_DOWN:  currentDownOffset += offsetStep; offsetChanged = true; break;
            }
            break;
        }

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
                int delta = GET_WHEEL_DELTA_WPARAM(pMouse->mouseData);
                currentZoomLevel = delta > 0 ? min(currentZoomLevel + 1, 5)
                    : max(currentZoomLevel - 1, 1);

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

/*============== WINDOW PROCEDURE ==============*/
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
        PostQuitMessage(0);
        break;

    case WM_USER + 1:
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

    // For simplicity, we'll use MessageBox for configuration in this version
    // In a real application, you would create a proper dialog resource
    if (MessageBox(NULL,
        L"Would you like to configure Magnifier+ settings?\n(No will use saved/default settings)",
        L"Magnifier+ Configuration",
        MB_YESNO | MB_ICONQUESTION) == IDYES) {

        // Simple configuration - in real app use proper dialog
        WCHAR buffer[256];
        swprintf(buffer, 256,
            L"Current Settings:\n\n"
            L"Window Width: %d\n"
            L"Window Height: %d\n"
            L"Zoom Area: %d\n"
            L"X Offset: %d\n"
            L"Y Offset: %d\n"
            L"FPS: %d\n"
            L"Move Step: %d\n\n"
            L"Edit MagnifierPlus.ini to change these values.",
            windowWidth, windowHeight, baseZoomSize,
            initialLeftOffset, initialDownOffset,
            targetFPS, offsetStep);

        MessageBox(NULL, buffer, L"Magnifier+ Settings", MB_OK | MB_ICONINFORMATION);
    }

    // Initialize zoom areas
    CalculateSourceRects();

    if (!MagInitialize()) {
        MessageBox(NULL, _T("Failed to initialize magnification API!"),
            _T("Magnifier+ Error"), MB_ICONERROR);
        return -1;
    }

    hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, hInstance, 0);
    hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, hInstance, 0);

    if (!hMouseHook || !hKeyboardHook) {
        MessageBox(NULL, _T("Failed to initialize input hooks!"),
            _T("Magnifier+ Error"), MB_ICONERROR);
        MagUninitialize();
        return -1;
    }

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

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    hwndMagnifier = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        _T("MagnifierWindowClass"),
        _T("Magnifier+ (RMB+Scroll=Zoom | RMB+Arrows=Adjust)"),
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

    SetWindowLong(hwndMagnifier, GWL_EXSTYLE,
        GetWindowLong(hwndMagnifier, GWL_EXSTYLE) |
        WS_EX_TRANSPARENT | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwndMagnifier, 0, 255, LWA_ALPHA);

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

    MAGTRANSFORM matrix = { {
        {zoomLevels[currentZoomLevel - 1], 0, 0},
        {0, zoomLevels[currentZoomLevel - 1], 0},
        {0, 0, 1}
    } };
    MagSetWindowTransform(hwndMag, &matrix);

    nid.cbSize = sizeof(nid);
    nid.hWnd = hwndMagnifier;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = LoadProgramIcon();
    wcscpy_s(nid.szTip, L"Magnifier+ (Right-click to exit)");
    Shell_NotifyIcon(NIM_ADD, &nid);

    SetTimer(hwndMagnifier, 1, 1000 / targetFPS, NULL);
    ShowWindow(hwndMagnifier, SW_HIDE);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    KillTimer(hwndMagnifier, 1);
    MagUninitialize();
    return (int)msg.wParam;
}