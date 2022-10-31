#pragma once

#include "tbconf.h"

COLORREF GetDwmColor(void);
void SetDwmColor(COLORREF color);
COLORREF GetAccentColor(void);
void SetAccentColor(COLORREF color);

BOOL LoadColorPreferenceFuncs(void);
BOOL LoadDwmColorizationFuncs(void);
void UnloadColorPreferenceFuncs(void);
void UnloadDwmColorizationFuncs(void);