/*
 * COPYRIGHT: See COPYING in the top level directory
 * PURPOSE:   Misc property page
 *
 * PROGRAMMER: Franco Tortoriello (torto09@gmail.com)
 */

#include "app.h"
#include "resource.h"

#include <CommCtrl.h>
#include <psapi.h>
#include <shellapi.h>

static const TCHAR g_explorerKey[] =
    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced");

typedef struct tagTBSETTINGS
{
    DWORD bWinXPowerShell;
} TBSETTINGS;

static const TBSETTINGS g_defSettings =
{
    FALSE  /* bWinXPowerShell - TRUE starting on 1703 */
};

static TBSETTINGS g_oldSettings;
static TBSETTINGS g_newSettings;

static HWND g_hDlg;

#define SetChecked(iControl, bChecked) \
    SendDlgItemMessage(g_hDlg, iControl, \
        BM_SETCHECK, (WPARAM)(bChecked == TRUE), 0L)

static
void LoadExplorerSettings(void)
{
    g_oldSettings = g_defSettings;

    HKEY hKey;
    LSTATUS status = RegOpenKeyEx(
        HKEY_CURRENT_USER, g_explorerKey, 0, KEY_QUERY_VALUE, &hKey);
    if (status != ERROR_SUCCESS)
        return;

    DWORD dwType;
    DWORD cbData = sizeof(DWORD);

#define ReadDword(valueName, data, defValue) \
    status = RegQueryValueEx( \
        hKey, valueName, 0, &dwType, (BYTE *)&data, &cbData); \
    if (status != ERROR_SUCCESS || dwType != REG_DWORD) \
        data = defValue

#define ReadInvertedMember(valueName, member) \
    ReadDword(valueName, g_oldSettings.member, !g_defSettings.member); \
    g_oldSettings.member = !g_oldSettings.member;

    ReadInvertedMember(TEXT("DontUsePowerShellOnWinX"), bWinXPowerShell);

#undef ReadInvertedMember

    RegCloseKey(hKey);

    g_newSettings = g_oldSettings;
}

static
void UpdateExplorerControls(void)
{
    SetChecked(IDC_MISC_WINXPOWERSHELL, g_oldSettings.bWinXPowerShell);
}

static
void InitPage(void)
{
    LoadExplorerSettings();
    UpdateExplorerControls();
}

#define HasChanged(member) \
    g_newSettings.member != g_oldSettings.member

_Success_(return)
static
BOOL WriteExplorerSettings(void)
{
#define RestoreSetting(member) \
    g_newSettings.member = g_oldSettings.member

    HKEY hKey;
    LSTATUS status = RegCreateKeyEx(HKEY_CURRENT_USER, g_explorerKey, 0, NULL, 0,
        KEY_SET_VALUE, NULL, &hKey, NULL);
    if (status != ERROR_SUCCESS)
    {
        RestoreSetting(bWinXPowerShell);
        return FALSE;
    }

    BOOL ret = TRUE;
    DWORD bValue;

#define SetDword(value, data) \
    status = RegSetValueEx( \
        hKey, value, 0, REG_DWORD, (BYTE *)&data, sizeof(DWORD))

#define UpdateDwordInverted(value, member) \
    if (HasChanged(member)) { \
        bValue = !g_newSettings.member; \
        SetDword(value, bValue); \
        if (status != ERROR_SUCCESS) { \
            RestoreSetting(member); \
            ret = FALSE; \
        } \
    }

    UpdateDwordInverted(TEXT("DontUsePowerShellOnWinX"), bWinXPowerShell);

#undef UpdateDwordInverted
#undef SetDword

    RegCloseKey(hKey);

    return ret;

#undef RestoreSetting
}

static
void ApplySettings(void)
{
    if (HasChanged(bWinXPowerShell))
    {
        if (!WriteExplorerSettings())
        {
            UpdateExplorerControls();
            return;
        }
    }

    g_oldSettings = g_newSettings;
}

#undef HasChanged

/* If the Explorer "Launch folder windows in a separate process" setting is
 * enabled, this only kills the desktop process (which created the taskbar).
 */
_Success_(return)
static
BOOL KillExplorer(void)
{
    HWND hTaskbar = FindWindow(TEXT("Shell_TrayWnd"), TEXT(""));
    if (!hTaskbar)
        return FALSE;

    DWORD procId = 0;
    if (GetWindowThreadProcessId(hTaskbar, &procId) <= 0)
        return FALSE;

    HANDLE hProc = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE,
        FALSE, procId);
    if (!hProc)
        return FALSE;

    HMODULE hMod;
    DWORD cbNeeded;
    if (!EnumProcessModules(hProc, &hMod, sizeof(hMod), &cbNeeded))
        goto Error;

    /* Unneeded, but verify the name just in case */
#define PROCNAME_MAX_LENGTH 13
    TCHAR procName[13] = TEXT("");
    if (GetModuleBaseName(hProc, hMod, procName, PROCNAME_MAX_LENGTH) <= 0)
        goto Error;
#undef PROCNAME_MAX_LENGTH

    if (lstrcmpi(procName, TEXT("explorer.exe")) != 0)
        goto Error;

    if (!TerminateProcess(hProc, 1))
        goto Error;

    CloseHandle(hProc);
    return TRUE;

Error:
    CloseHandle(hProc);
    return FALSE;
}

static
void RestartExplorer(void)
{
    if (KillExplorer())
    {
        /* Wait a bit to avoid errors */
        Sleep(500);
    }

    if (FindWindow(TEXT("Shell_TrayWnd"), TEXT("")) == NULL)
    {
        /* No taskbar; launch Explorer */
        ShellExecute(NULL, TEXT("open"), TEXT("explorer.exe"), NULL, NULL,
            SW_SHOWNORMAL);
    }
}

static
void HandleCommand(WORD iControl)
{
#define GetChecked() \
    (SendDlgItemMessage(g_hDlg, iControl, BM_GETCHECK, 0L, 0L) == BST_CHECKED)

    switch (iControl)
    {
    case IDC_MISC_WINXPOWERSHELL:
        g_newSettings.bWinXPowerShell = GetChecked();
        break;

    default:
        return;
    }

    PropSheet_Changed(g_propSheet.hWnd, g_hDlg);

#undef GetChecked
}

INT_PTR CALLBACK MiscPageProc(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        g_hDlg = hWnd;
        InitPage();
        return 0;

    case WM_COMMAND:
        switch HIWORD(wParam)
        {
        case BN_CLICKED:
            HandleCommand(LOWORD(wParam));
            break;
        }

        return 0;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code)
        {
        case PSN_APPLY:
            ApplySettings();
            SetWindowLongPtr(hWnd, DWLP_MSGRESULT, (LONG_PTR)PSNRET_NOERROR);
            return TRUE;

        case PSN_KILLACTIVE:
            SetWindowLongPtr(hWnd, DWLP_MSGRESULT, (LONG_PTR)FALSE);
            return TRUE;

        case NM_CLICK:
        case NM_RETURN:
            if (lstrcmpW(((NMLINK *)lParam)->item.szID, L"restart") == 0)
                RestartExplorer();
            break;
        }

        return 0;

    case WM_QUERYENDSESSION:
        return 0;

    case WM_CTLCOLORDLG:
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
