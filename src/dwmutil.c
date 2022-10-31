/*
 * COPYRIGHT:	See COPYING in the top level directory
 * PROJECT:	Taskbar Properties
 * PURPOSE:	Destop Window Manager utilities
 *
 * PROGRAMMER:	Franco Tortoriello (torto09@gmail.com)
 */

#include "dwmutil.h"

/* sources:
 * https://www.codeproject.com/Tips/610909/Changing-Windows-Aero-Color
 * https://github.com/m417z/Windows-10-Color-Control
 */

typedef struct
{
    COLORREF clrColor;        /* ColorizationColor */
    COLORREF clrAfterGlow;    /* ColorizationAfterglow */
    UINT     nIntensity;      /* ColorizationColorBalance (0-100) */
    COLORREF clrAfterGlowBal; /* ColorizationAfterglowBalance */
    UINT     clrBlurBal;      /* ColorizationBlurBalance */
    UINT     clrGlassReflInt; /* ColorizationGlassReflectionIntensity */
    BOOL     fOpaque;
} DWMCOLOR;
DWMCOLOR dwmColor;

typedef struct {
    COLORREF color1;
    COLORREF color2;
} ACCENTCOLORS;

static HMODULE hUxThemeDll = NULL;
static HMODULE hDwmDll = NULL;

static HRESULT(WINAPI *DwmSetColorizationParameters) (DWMCOLOR *colorParams, BOOL fDontSave) = NULL;
static HRESULT(WINAPI *DwmGetColorizationParameters) (DWMCOLOR *colorParams) = NULL;

static HRESULT(WINAPI *GetUserColorPreference)(ACCENTCOLORS *pcpPreference, BOOL fForceReload) = NULL;
static HRESULT(WINAPI *SetUserColorPreference)(const ACCENTCOLORS *cpcpPreference, BOOL fForceCommit) = NULL;


#define RGB2BGR(c) RGB(GetBValue(c), GetGValue(c), GetRValue(c))

COLORREF GetDwmColor(void)
{
    COLORREF crDwmColor = 0;

    DwmGetColorizationParameters(&dwmColor);
    /* BGR -> RGB */
    crDwmColor = dwmColor.clrColor & 0x00FFFFFF;
    crDwmColor = RGB2BGR(crDwmColor);

    return crDwmColor;
}

void SetDwmColor(COLORREF color)
{
    /* RGB -> BGR */
    color = 0xC4000000 | RGB2BGR(color);

    dwmColor.clrColor = color;
    dwmColor.clrAfterGlow = color;
    dwmColor.nIntensity = 100; /* on Win8+: no transparency, same effect as higher luminescence */
    DwmSetColorizationParameters(&dwmColor, 0);
}

#undef RGB2BGR


COLORREF GetAccentColor(void)
{
    COLORREF color = 0;
    ACCENTCOLORS accentColors;

    if SUCCEEDED(GetUserColorPreference(&accentColors, 0))
    {
        color = accentColors.color2 & 0x00FFFFFF;
    }
    return color;
}

void SetAccentColor(COLORREF color)
{
    HRESULT hr;
    ACCENTCOLORS accentColors;

    hr = GetUserColorPreference(&accentColors, 0);
    if (!SUCCEEDED(hr)) return;

    accentColors.color2 = color;
    hr = SetUserColorPreference(&accentColors, 1);

    if (!SUCCEEDED(hr)) return;
}


/* Load undocumented functions */

BOOL LoadColorPreferenceFuncs(void)
{
    hUxThemeDll = LoadLibraryW(L"uxtheme.dll");
    if (!hUxThemeDll) return FALSE;

    *(FARPROC *)&GetUserColorPreference = GetProcAddress(hUxThemeDll, "GetUserColorPreference");
    if (!GetUserColorPreference)
    {
        UnloadColorPreferenceFuncs();
        return FALSE;
    }

    *(FARPROC *)&SetUserColorPreference = GetProcAddress(hUxThemeDll, (LPCSTR)122);
    if (!SetUserColorPreference)
    {
        UnloadColorPreferenceFuncs();
        return FALSE;
    }

    return TRUE;
}

BOOL LoadDwmColorizationFuncs(void)
{
    hDwmDll = LoadLibraryW(L"dwmapi.dll");
    if (!hDwmDll) return FALSE;

    *(FARPROC *)&DwmGetColorizationParameters = GetProcAddress(hDwmDll, (LPCSTR)127);
    if (!DwmGetColorizationParameters)
    {
        UnloadDwmColorizationFuncs();
        return FALSE;
    }

    *(FARPROC *)&DwmSetColorizationParameters = GetProcAddress(hDwmDll, (LPCSTR)131);
    if (!DwmSetColorizationParameters)
    {
        UnloadDwmColorizationFuncs();
        return FALSE;
    }

    return TRUE;
}

void UnloadColorPreferenceFuncs(void)
{
    FreeLibrary(hUxThemeDll);
    hUxThemeDll = NULL;
}

void UnloadDwmColorizationFuncs(void)
{
    FreeLibrary(hDwmDll);
    hDwmDll = NULL;
}