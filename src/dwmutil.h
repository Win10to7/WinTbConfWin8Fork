#pragma once
#if !defined(DWMUTIL_H)
#define DWMUTIL_H

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

COLORREF GetDwmColor(void);
void SetDwmColor(COLORREF color);
COLORREF GetAccentColor(void);
void SetAccentColor(COLORREF color);

BOOL LoadColorPreferenceFuncs(void);
BOOL LoadDwmColorizationFuncs(void);
void UnloadColorPreferenceFuncs(void);
void UnloadDwmColorizationFuncs(void);

#endif  /* !defined(DWMUTIL_H) */
