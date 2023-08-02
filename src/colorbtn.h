#pragma once
#if !defined(COLORBTN_H)
#define COLORBTN_H

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

void SetColorButtonColor(
    HWND hWnd, int controlId, HBITMAP *hBmp, COLORREF color);

BOOL PickColor(HWND hWnd, _Out_ COLORREF *crColor);

#endif  /* !defined(COLORBTN_H) */
