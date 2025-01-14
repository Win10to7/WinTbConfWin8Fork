#pragma once
#if !defined(UTIL_H)
#define UTIL_H

#include "config.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

_Success_(return > 0)
int AllocAndLoadString(
    HMODULE hModule,
    UINT id,
    _Out_ TCHAR **pTarget
);

#define AllocAndLoadAppString(id, pTarget) \
    AllocAndLoadString(g_propSheet.hInstance, id, pTarget)

_Success_(return != 0)
int ShowMessageFromResource(
    HMODULE hModule,
    HWND hWnd,
    int msgId,
    int titleMsgId,
    UINT type
);

#define ShowMessageFromAppResource(hWnd, msgId, titleMsgId, type) \
    ShowMessageFromResource(g_propSheet.hInstance, hWnd, \
        msgId, titleMsgId, type)

_Success_(return)
BOOL SetCustomVisualFx(void);

#endif  /* !defined(UTIL_H) */
