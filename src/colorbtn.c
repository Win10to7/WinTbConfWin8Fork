/*
 * COPYRIGHT:	See COPYING in the top level directory
 * PROJECT:	Taskbar Properties
 * PURPOSE:	Utility functions for color picker buttons
 *
 * Based on code from ReactOS
 * Original programmers:
 *     Trevor McCort (lycan359@gmail.com)
 *     Timo Kreuzer (timo[dot]kreuzer[at]web[dot]de)
 */

#include "dwmutil.h"
#include <commdlg.h>

static void DrawRectangle(HDC hdc, RECT rect, COLORREF color)
{
    HBRUSH hBrush = CreateSolidBrush(color);
    FillRect(hdc, &rect, hBrush);
    DeleteObject(hBrush);
}

/* Draw a color on a color picker button, creating the bitmap if it is NULL */
void SetColorButtonColor(HWND hWnd, int controlId, HBITMAP *hBmp, COLORREF color)
{
    static const LONG RECT_WIDTH = 28;
    static const LONG RECT_HEIGHT = 15;

    HWND hWndBtn = GetDlgItem(hWnd, controlId);
    if (!hWndBtn) return;

    HDC hdcBtn = GetDC(hWndBtn);
    if (!hdcBtn) return;

    /* Create a Device Context to draw on */
    HDC hdcCompat = CreateCompatibleDC(hdcBtn);
    if (!hdcCompat)
    {
	ReleaseDC(hWndBtn, hdcBtn);
	return;
    }

    RECT rect;
    HGDIOBJ hgdiTemp;

    if (!*hBmp)
    {
	/* First time drawing; create the button image */
	*hBmp = CreateCompatibleBitmap(hdcBtn, RECT_WIDTH, RECT_HEIGHT);
	ReleaseDC(hWndBtn, hdcBtn);
	if (!*hBmp)
	{
	    /* Failed */
	    DeleteDC(hdcCompat);
	    return;
	}
	
	/* Select the button image to the DC */
	hgdiTemp = SelectObject(hdcCompat, *hBmp);

	/* Draw the rectangle stroke only the first time */

	/* Button background color */
	rect.left = 0;
	rect.top = 0;
	rect.right = RECT_WIDTH;
	rect.bottom = RECT_HEIGHT;
	DrawRectangle(hdcCompat, rect, GetSysColor(COLOR_BTNFACE));

	/* "Stroke" */
	rect.left = 1;
	rect.top = 1;
	rect.right = RECT_WIDTH - 1;
	rect.bottom = RECT_HEIGHT - 1;
	DrawRectangle(hdcCompat, rect, GetSysColor(COLOR_BTNTEXT));
    }
    else
    {
	ReleaseDC(hWndBtn, hdcBtn);
	hgdiTemp = SelectObject(hdcCompat, *hBmp);
    }

    /* Fill the rectangle */
    rect.left = 2;
    rect.top = 2;
    rect.right = RECT_WIDTH - 2;
    rect.bottom = RECT_HEIGHT - 2;
    DrawRectangle(hdcCompat, rect, color);

    /* Cleanup */
    SelectObject(hdcCompat, hgdiTemp);
    DeleteDC(hdcCompat);

    SendDlgItemMessageW(hWnd, controlId, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)*hBmp);
}

/* Select a color using a color picker.
 * Returns if a different color was chosen.
 */
BOOL PickColor(HWND hWnd, _Out_ COLORREF *crColor)
{
    COLORREF crCustom[16] = { 0 };
    BOOL chosen = FALSE;

#ifdef SAVE_CUSTOM_COLORS
    HKEY hKey = NULL;

    /* Load custom colors from Registry */
    if (RegCreateKeyExW(HKEY_CURRENT_USER, g_AppearanceKey, 0, NULL,
	0, KEY_QUERY_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS)
	/* Now the key is either created or opened existing, if ERROR_SUCCESS */
    {
	/* Key opened */
	DWORD dwType = REG_BINARY;
	DWORD cbData = sizeof(crCustom);
	RegQueryValueExW(hKey, L"CustomColors", 0, &dwType, (BYTE *)crCustom, &cbData);
	RegCloseKey(hKey); hKey = NULL;
    }
#endif

    CHOOSECOLORW cc;

    /* Prepare cc structure */
    cc.lStructSize = sizeof(CHOOSECOLORW);
    cc.hwndOwner = hWnd;
    cc.hInstance = NULL;
    cc.rgbResult = *crColor;
    cc.lpCustColors = crCustom;
    cc.Flags = CC_SOLIDCOLOR | CC_FULLOPEN | CC_RGBINIT;
    cc.lCustData = 0;
    cc.lpfnHook = NULL;
    cc.lpTemplateName = NULL;

    /* Create the colorpicker */
    if (ChooseColorW(&cc) && (*crColor != cc.rgbResult))
    {
        chosen = TRUE;
        *crColor = cc.rgbResult;
    }

#ifdef SAVE_CUSTOM_COLORS
    /* Always save custom colors to reg. If the key did not exist previously, it was created above */
    if (RegOpenKeyExW(HKEY_CURRENT_USER, g_AppearanceKey, 0,
	KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
    {
	/* Key opened */
	RegSetValueExW(hKey, L"CustomColors", 0, REG_BINARY,
	    (BYTE *)crCustom, sizeof(crCustom));
	RegCloseKey(hKey);
	hKey = NULL;
    }
#endif

    return chosen;
}