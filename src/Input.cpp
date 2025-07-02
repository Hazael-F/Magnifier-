#include "Input.h"
#include "Magnifier.h"
#include "Config.h"

extern MagnifierData g_magnifierData;
extern InputData g_inputData;

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && g_inputData.isRightMouseDown) {
        KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;
        bool changed = false;

        if (wParam == WM_KEYDOWN) {
            switch (pKey->vkCode) {
            case VK_LEFT:  g_magnifierData.currentAdjustment.x -= g_magnifierData.moveStep; changed = true; break;
            case VK_RIGHT: g_magnifierData.currentAdjustment.x += g_magnifierData.moveStep; changed = true; break;
            case VK_UP:    g_magnifierData.currentAdjustment.y -= g_magnifierData.moveStep; changed = true; break;
            case VK_DOWN:  g_magnifierData.currentAdjustment.y += g_magnifierData.moveStep; changed = true; break;
            }
        }

        if (changed && g_magnifierData.currentZoomLevel > 0) {
            CalculateSourceRects(g_magnifierData);
            UpdateMagnifierContent(g_magnifierData);
        }
    }
    return CallNextHookEx(g_inputData.hKeyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;

        switch (wParam) {
        case WM_RBUTTONDOWN:
            g_inputData.isRightMouseDown = true;
            if (g_magnifierData.currentZoomLevel > 0) {  // Only show if already zoomed in
                ShowWindow(g_magnifierData.hwndHost, SW_SHOW);
                if (g_magnifierData.mouseTracking) {
                    g_magnifierData.lastMousePos = pMouse->pt;
                    UpdateMagnifierPosition(g_magnifierData);
                }
            }
            break;

        case WM_RBUTTONUP:
            g_inputData.isRightMouseDown = false;
            if (g_magnifierData.currentZoomLevel > 0) {  // Only hide if zoomed in
                ShowWindow(g_magnifierData.hwndHost, SW_HIDE);
            }
            break;

        case WM_MOUSEWHEEL:
            if (g_inputData.isRightMouseDown) {
                int delta = GET_WHEEL_DELTA_WPARAM(pMouse->mouseData);
                int newZoomLevel = delta > 0 ?
                    min(g_magnifierData.currentZoomLevel + 1, 5) :
                    max(g_magnifierData.currentZoomLevel - 1, 0);

                ApplyZoomLevel(g_magnifierData, newZoomLevel);

                // Only show/hide if actually changing zoom levels
                if (newZoomLevel != g_magnifierData.currentZoomLevel) {
                    ShowWindow(g_magnifierData.hwndHost, newZoomLevel > 0 ? SW_SHOW : SW_HIDE);
                }

                if (g_magnifierData.mouseTracking) return 1;
            }
            break;

        case WM_MOUSEMOVE:
            if (g_magnifierData.mouseTracking && g_inputData.isRightMouseDown &&
                g_magnifierData.currentZoomLevel > 0) {
                g_magnifierData.lastMousePos = pMouse->pt;
                UpdateMagnifierPosition(g_magnifierData);
                UpdateMagnifierContent(g_magnifierData);
            }
            break;
        }
    }
    return CallNextHookEx(g_inputData.hMouseHook, nCode, wParam, lParam);
}

void InitializeInputHooks(InputData& inputData, HINSTANCE hInstance) {
    inputData.hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookProc, hInstance, 0);
    inputData.hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, hInstance, 0);
}

void CleanupInputHooks(InputData& inputData) {
    if (inputData.hMouseHook) UnhookWindowsHookEx(inputData.hMouseHook);
    if (inputData.hKeyboardHook) UnhookWindowsHookEx(inputData.hKeyboardHook);
}