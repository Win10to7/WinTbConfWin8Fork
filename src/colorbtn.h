#pragma once

#include "tbconf.h"

void SetColorButtonColor(HWND hWnd, int controlId, HBITMAP *hBmp, COLORREF color);
BOOL PickColor(HWND hWnd, _Out_ COLORREF *crColor);