/*
 * COPYRIGHT: See COPYING in the top level directory
 * PURPOSE:   Program entry point
 *
 * PROGRAMMER: Franco Tortoriello (torto09@gmail.com)
 */

#include "app.h"
#include "resource.h"
#include "util.h"

#include <commctrl.h>
#include <initguid.h>
#include <shellapi.h>

/* https://www.geoffchappell.com/studies/windows/shell/shlwapi/api/winpolicy/policies.htm
 */
DEFINE_GUID(POLID_NoSetTaskbar,
    0xC67F73F8, 0xAB64, 0x422F, 0xB9, 0x52, 0x3C, 0x57, 0xAB, 0xC9, 0xC1, 0x37);
DEFINE_GUID(POLID_TaskbarLockAll,
    0xC79A44A1, 0xDB7C, 0x4212, 0xA8, 0x37, 0x4B, 0x07, 0x3F, 0x6C, 0x48, 0x15);

static HICON g_hiconLarge;
static HICON g_hiconSmall;

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

    UINT len = GetWindowsDirectory(szFilePath, MAX_PATH);
    if (len < 2 || len > MAX_PATH - 22)
        return;

    if (!lstrcat(szFilePath, TEXT("\\System32\\imageres.dll")))
        return;

    if (ExtractIconEx(szFilePath, 75, &g_hiconLarge, &g_hiconSmall, 1) == 0)
        return;

    SendMessage(g_propSheet.hWnd, WM_SETICON, ICON_BIG,   (LPARAM)g_hiconLarge);
    SendMessage(g_propSheet.hWnd, WM_SETICON, ICON_SMALL, (LPARAM)g_hiconSmall);
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

    if (g_hiconLarge)
        DestroyIcon(g_hiconLarge);
    if (g_hiconSmall)
        DestroyIcon(g_hiconSmall);

    if (ret < 0)
        goto Error;

    if (ret == 0)
        return RETURN_NO_CHANGES;

    return RETURN_CHANGES;

Error:
    ShowMessageFromAppResource(NULL, IDS_ERROR_GENERIC, IDS_ERROR, MB_OK);
    return RETURN_ERROR;
}

static
BOOL ShowRunningInstance(void)
{
    CreateMutex(0, TRUE, TEXT("TortoTbConfig"));
    if (GetLastError() != ERROR_ALREADY_EXISTS)
        return FALSE;

    HWND hExistingWnd;

    hExistingWnd = FindWindowEx(NULL, NULL,
        MAKEINTATOM(0x8002), TEXT("Taskbar Properties"));

    if (!hExistingWnd)
    {
        hExistingWnd = FindWindowEx(
            NULL, NULL, MAKEINTATOM(0x8002),
            TEXT("Propiedades de Barra de tareas"));
    }

    if (!hExistingWnd)
    {
        /* No window found... open it again */
        return FALSE;
    }

    SetForegroundWindow(hExistingWnd);
    return TRUE;
}

static
BOOL ShowRestrictedMessage(void)
{
    HMODULE hSystem32 = GetModuleHandle(TEXT("shell32.dll"));
    if (hSystem32 == NULL)
        return FALSE;

    int ret = ShowMessageFromResource(hSystem32, NULL, 9729, 9728,
        MB_OK | MB_ICONERROR);
    return (ret > 0);
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

static
BOOL IsAppRestricted(void)
{
    /* SHRestricted() could be used, but it is deprecated and does not
     * support newer restrictions */
    HMODULE hShlwapi = LoadLibrary(TEXT("shlwapi.dll"));
    if (!hShlwapi)
        return FALSE;

    BOOL(WINAPI *SHWindowsPolicy)(GUID *rpolid);

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
    *(FARPROC *)&SHWindowsPolicy =
        GetProcAddress(hShlwapi, MAKEINTRESOURCEA(618));
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

    /* "Prevent changes to Taskbar and Start Menu Settings",
     * "Lock all taskbar settings" */
    BOOL isRestricted = SHWindowsPolicy && (
        SHWindowsPolicy((GUID *)&POLID_NoSetTaskbar) ||
        SHWindowsPolicy((GUID *)&POLID_TaskbarLockAll));

    FreeLibrary(hShlwapi);
    return isRestricted;
}

_Success_(return == 0)
static
UINT InitProgram(void)
{
    if (ShowRunningInstance())
        return RETURN_EXISTING_INSTANCE;

    g_propSheet.heap = GetProcessHeap();
    if (!g_propSheet.heap)
        goto Error;

    g_propSheet.hInstance = GetModuleHandle(NULL);
    if (!g_propSheet.hInstance)
        goto Error;

    /* Respect user policies in Administrative Templates ->
     * Start Menu and Taskbar */
    if (IsAppRestricted())
    {
        if (!ShowRestrictedMessage())
            goto Error;
        return RETURN_ERROR;
    }

    return InitGUI(0);

Error:
    ShowMessageFromAppResource(NULL, IDS_ERROR_MEM, IDS_ERROR, MB_OK);
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
