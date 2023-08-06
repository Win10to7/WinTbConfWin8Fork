/*
 * COPYRIGHT: See COPYING in the top level directory
 * PURPOSE:   Program entry point
 *
 * PROGRAMMER: Franco Tortoriello (torto09@gmail.com)
 */

#include "app.h"
#include "resource.h"
#include "util.h"

#include <CommCtrl.h>
#include <shellapi.h>

PROPSHEET g_propSheet;

/* Property sheet dialog proc forward definitions */
INT_PTR CALLBACK GeneralPageProc(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AdvancedPageProc(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static
void InitPage(PROPSHEETHEADER *ppsh, WORD idDlg, DLGPROC dlgProc)
{
    PROPSHEETPAGE psp;

    memset(&psp, 0, sizeof(PROPSHEETPAGE));
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT;
    psp.hInstance = g_propSheet.hInstance;
    psp.pszTemplate = MAKEINTRESOURCE(idDlg);
    psp.pfnDlgProc = dlgProc;

    HPROPSHEETPAGE hPage = CreatePropertySheetPage(&psp);
    if (hPage)
        ppsh->phpage[ppsh->nPages++] = hPage;
}

static
void SetIcon(void)
{
    TCHAR szFilePath[MAX_PATH];

    if (!GetWindowsDirectory(szFilePath, MAX_PATH))
        return;

    if (!lstrcat(szFilePath, TEXT("\\System32\\imageres.dll")))
        return;

    HICON hiconLarge;
    HICON hiconSmall;
    if (ExtractIconEx(szFilePath, 75, &hiconLarge, &hiconSmall, 1) <= 0)
        return;

    SendMessage(g_propSheet.hWnd, WM_SETICON, ICON_BIG, (LPARAM)hiconLarge);
    SendMessage(g_propSheet.hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hiconSmall);
}

static
int CALLBACK PropSheetProc(HWND hWnd, UINT uMsg, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
    case PSCB_INITIALIZED:
        g_propSheet.hWnd = hWnd;
        SetIcon();
    }

    return 0;
}

_Success_(return < RETURN_ERROR)
    static
    UINT DisplayPropSheet(UINT nStartPage)
{
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE hpsp[2];

    memset(&psh, 0, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =
        PSH_USECALLBACK | PSH_PROPTITLE | PSH_USEICONID | PSH_NOCONTEXTHELP;
    psh.hInstance = g_propSheet.hInstance;
    psh.pszCaption = MAKEINTRESOURCE(IDS_PROPSHEET_NAME);
    psh.nPages = 0;
    psh.nStartPage = nStartPage;
    psh.phpage = hpsp;
    psh.pfnCallback = PropSheetProc;

    InitPage(&psh, IDD_TB, GeneralPageProc);
    InitPage(&psh, IDD_ADV, AdvancedPageProc);

    INT_PTR ret = PropertySheet(&psh);

    if (ret < 0)
        goto Error;

    if (ret == 0)
        return RETURN_NO_CHANGES;

    return RETURN_CHANGES;

Error:
    ShowMessageFromResource(NULL, IDS_ERROR_GENERIC, IDS_ERROR, MB_OK);
    return RETURN_ERROR;
}

_Success_(return < RETURN_ERROR)
    static
    UINT InitGUI(UINT nStartPage)
{
    INITCOMMONCONTROLSEX icce;
    icce.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icce.dwICC = ICC_STANDARD_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icce);

    return DisplayPropSheet(nStartPage);
}

_Success_(return == 0)
static
UINT InitProgram(void)
{
    g_propSheet.heap = GetProcessHeap();
    if (!g_propSheet.heap)
        goto Error;

    g_propSheet.hInstance = GetModuleHandle(NULL);
    if (!g_propSheet.hInstance)
        goto Error;

    return InitGUI(0);

Error:
    ShowMessageFromResource(NULL, IDS_ERROR_MEM, IDS_ERROR, MB_OK);
    return RETURN_ERROR;
}

#if defined(__MINGW64__)
void WINAPI __main(void)
#else
void WINAPI _main(void)
#endif
{
    ExitProcess(InitProgram());
}
