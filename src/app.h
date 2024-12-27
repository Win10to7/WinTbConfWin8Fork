#pragma once
#if !defined(APP_H)
#define APP_H

#include "config.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Program exit codes */
enum returnCode
{
    RETURN_NO_CHANGES,
    RETURN_CHANGES,
    RETURN_ERROR,
    RETURN_USAGE,
    RETURN_EXISTING_INSTANCE
};

typedef struct tagPROPSHEET
{
    HINSTANCE hInstance;
    HWND hWnd;
    HANDLE heap;
} PROPSHEET;

extern PROPSHEET g_propSheet;

#define Alloc(flags, size) \
    HeapAlloc(g_propSheet.heap, flags, size);

#define Free(pMem) \
    HeapFree(g_propSheet.heap, 0, pMem);

#endif  /* !defined(APP_H) */
