/*
 * COPYRIGHT: See COPYING in the top level directory
 * PURPOSE:   Application entry point
 *
 * PROGRAMMER: Franco Tortoriello (torto09@gmail.com)
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "resource.h"

/* In wndtb.c */
INT_PTR CALLBACK TaskbarSettingsDlgProc(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

HINSTANCE g_hInstance;

#if defined(__MINGW64__)
void WINAPI __main(void)
#else
void WINAPI _main(void)
#endif
{
    g_hInstance = GetModuleHandle(NULL);

    DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_TASKBAR),
        NULL, TaskbarSettingsDlgProc, 0);
    ExitProcess(0);
}
