#pragma once
#include <windows.h>

struct InputData {
    bool isRightMouseDown = false;
    HHOOK hMouseHook = nullptr;
    HHOOK hKeyboardHook = nullptr;
};

void InitializeInputHooks(InputData& inputData, HINSTANCE hInstance);
void CleanupInputHooks(InputData& inputData);
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam);