#pragma once
#if !defined(UTIL_H)
#define UTIL_H

#include "config.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

_Success_(return > 0)
int AllocAndLoadString(_Out_ TCHAR **pTarget, UINT id);

_Success_(return != 0)
int ShowMessageFromResource(HWND hWnd, int msgId, int titleMsgId, UINT type);

_Success_(return)
BOOL SetCustomVisualFx(void);

#endif  /* !defined(UTIL_H) */
