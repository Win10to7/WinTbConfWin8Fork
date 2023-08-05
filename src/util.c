/*
 * COPYRIGHT: See COPYING in the top level directory
 * PURPOSE:   Provides auxiliary functions
 *
 * PROGRAMMERS: ReactOS Team
 *              Franco Tortoriello (torto09@gmail.com)
 */

#include "app.h"

#include <CommCtrl.h>

_Success_(return >= 0)
static
int LengthOfStrResource(HINSTANCE hInst, UINT id)
{
    if (!hInst)
        return -1;

    /* There are always blocks of 16 strings */
    TCHAR *name = MAKEINTRESOURCE((id >> 4) + 1);

    /* Find the string table block */
    HRSRC hrSrc = FindResource(hInst, name, RT_STRING);
    if (!hrSrc)
        return -1;

    HGLOBAL hRes = LoadResource(hInst, hrSrc);
    if (!hRes)
        return -1;

    /* Note: Always use WCHAR because the resources are in Unicode */
    WCHAR *pStrLen = (WCHAR *)LockResource(hRes);
    if (!pStrLen)
        return -1;

    /* Find the string we're looking for */
    id &= 0xF; /* Position in the block, same as % 16 */
    for (UINT x = 0; x < id; x++)
        pStrLen += (*pStrLen) + 1;

    /* Found the string */
    return (int)(*pStrLen);
}

_Success_(return > 0)
int AllocAndLoadString(_Out_ TCHAR **pTarget, UINT id)
{
    int len = LengthOfStrResource(g_propSheet.hInstance, id);
    if (len++ > 0)
    {
        (*pTarget) = (TCHAR *)Alloc(0, len * sizeof(TCHAR));
        if (*pTarget)
        {
            int ret = LoadString(g_propSheet.hInstance, id, *pTarget, len);
            if (ret > 0)
                return ret;

            /* Could not load the string */
            Free((HLOCAL)(*pTarget));
        }
    }

    *pTarget = NULL;
    return 0;
}

int ShowMessageFromResource(HWND hWnd, int msgId, int titleMsgId, UINT type)
{
    TCHAR *msg;
    TCHAR *msgTitle;

    AllocAndLoadString(&msg, msgId);
    AllocAndLoadString(&msgTitle, titleMsgId);

    int ret = MessageBox(hWnd, msg, msgTitle, MB_APPLMODAL | type);

    Free(msg);
    Free(msgTitle);
    return ret;
}
