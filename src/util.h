#pragma once
#if !defined(UTIL_H)
#define UTIL_H

#include "config.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

_Success_(return > 0)
int AllocAndLoadString(_Out_ TCHAR **pTarget, UINT id);

int ShowMessageFromResource(HWND hWnd, int msgId, int titleMsgId, UINT type);

#endif  /* !defined(UTIL_H) */
