#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "resource.h"

HINSTANCE hInstance;

INT_PTR CALLBACK TaskbarSettingsDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);