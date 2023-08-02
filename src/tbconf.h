#pragma once
#if !defined(TBCONF_H)
#define TBCONF_H

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "resource.h"

HINSTANCE hInstance;

INT_PTR CALLBACK TaskbarSettingsDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif  /* !defined(TBCONF_H) */
