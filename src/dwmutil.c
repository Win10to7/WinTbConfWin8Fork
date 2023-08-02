/*
 * COPYRIGHT: See COPYING in the top level directory
 * PURPOSE:   Destop Window Manager utilities
 *
 * PROGRAMMER: Franco Tortoriello (torto09@gmail.com)
 */

#include "dwmutil.h"

typedef struct
{
    /* These values are saved to the registry to:
     * HKCU\SOFTWARE\Microsoft\Windows\DWM
     */

    /* ColorizationColor: Main DWM color. */
    COLORREF clrColor;

    /* ColorizationAfterglow - no effect on 8+ */
    COLORREF clrAfterGlow;

    /* ColorizationColorBalance (0 - 100): Transparency level.
     * On 8+, same effect as higher luminescence.
     */
    UINT     nIntensity;

    /* ColorizationAfterglowBalance (0 - 115) - no effect on 8+ */
    UINT     nAfterGlowIntensity;

    /* ColorizationBlurBalance (0 - 115) - no effect on 8+ */
    UINT     nBlurIntensity;

    /* ColorizationGlassReflectionIntensity (0 - 100) */
    UINT     nGlassReflectionIntensity;

    /* Disable transparency - Always opaque on 8+ */
    BOOL     fOpaque;
} DWMCOLOR;

static DWMCOLOR g_dwmColor;

typedef struct {
    COLORREF color1;
    COLORREF color2;
} ACCENTCOLORS;

static HMODULE g_hUxThemeDll = NULL;
static HMODULE g_hDwmDll = NULL;

static HRESULT(WINAPI *DwmSetColorizationParameters)
    (DWMCOLOR *colorParams, BOOL fDontSave) = NULL;
static HRESULT(WINAPI *DwmGetColorizationParameters)
    (DWMCOLOR *colorParams) = NULL;

static HRESULT(WINAPI *GetUserColorPreference)
    (ACCENTCOLORS *pcpPreference, BOOL fForceReload) = NULL;
static HRESULT(WINAPI *SetUserColorPreference)
    (const ACCENTCOLORS *cpcpPreference, BOOL fForceCommit) = NULL;

#define RGB2BGR(c) RGB(GetBValue(c), GetGValue(c), GetRValue(c))

COLORREF GetDwmColor(void)
{
    COLORREF crDwmColor = 0;

    DwmGetColorizationParameters(&g_dwmColor);
    /* BGR -> RGB */
    crDwmColor = g_dwmColor.clrColor & 0x00FFFFFF;
    crDwmColor = RGB2BGR(crDwmColor);

    return crDwmColor;
}

void SetDwmColor(COLORREF color)
{
    /* RGB -> BGR */
    color = 0xC4000000 | RGB2BGR(color);

    g_dwmColor.clrColor = color;
    g_dwmColor.clrAfterGlow = color;
    g_dwmColor.nIntensity = 100;
    DwmSetColorizationParameters(&g_dwmColor, FALSE);
}

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

BOOL LoadColorPreferenceFuncs(void)
{
    g_hUxThemeDll = LoadLibrary(TEXT("uxtheme.dll"));
    if (!g_hUxThemeDll) return FALSE;

    *(FARPROC *)&GetUserColorPreference =
        GetProcAddress(g_hUxThemeDll, "GetUserColorPreference");
    if (!GetUserColorPreference)
    {
        UnloadColorPreferenceFuncs();
        return FALSE;
    }

    *(FARPROC *)&SetUserColorPreference =
        GetProcAddress(g_hUxThemeDll, MAKEINTRESOURCEA(122));
    if (!SetUserColorPreference)
    {
        UnloadColorPreferenceFuncs();
        return FALSE;
    }

    return TRUE;
}

BOOL LoadDwmColorizationFuncs(void)
{
    g_hDwmDll = LoadLibrary(TEXT("dwmapi.dll"));
    if (!g_hDwmDll)
        return FALSE;

    *(FARPROC *)&DwmGetColorizationParameters =
        GetProcAddress(g_hDwmDll, MAKEINTRESOURCEA(127));
    if (!DwmGetColorizationParameters)
    {
        UnloadDwmColorizationFuncs();
        return FALSE;
    }

    *(FARPROC *)&DwmSetColorizationParameters =
        GetProcAddress(g_hDwmDll, MAKEINTRESOURCEA(131));
    if (!DwmSetColorizationParameters)
    {
        UnloadDwmColorizationFuncs();
        return FALSE;
    }

    return TRUE;
}

void UnloadColorPreferenceFuncs(void)
{
    FreeLibrary(g_hUxThemeDll);
    g_hUxThemeDll = NULL;
}

void UnloadDwmColorizationFuncs(void)
{
    FreeLibrary(g_hDwmDll);
    g_hDwmDll = NULL;
}
