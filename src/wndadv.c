/*
 * COPYRIGHT: See COPYING in the top level directory
 * PURPOSE:   Advanced property page
 *
 * PROGRAMMER: Franco Tortoriello (torto09@gmail.com)
 */

#include "app.h"
#include "resource.h"
#include "util.h"

#include <commctrl.h>
#include <shellapi.h>

#include <psapi.h>

#define EnableApply() \
    SendMessage(g_propSheet.hWnd, PSM_CHANGED, (WPARAM)g_hDlg, 0L)

static const TCHAR g_explorerKey[] =
    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced");

static const TCHAR g_dwmKey[] =
    TEXT("Software\\Microsoft\\Windows\\DWM");

static const TCHAR g_explorerPatcherKey[] =
    TEXT("Software\\ExplorerPatcher");

typedef struct tagTBSETTINGS
{
    BOOL bAnimations;
    BOOL bSaveThumbnails;
    BOOL bWinXPowerShell;
    BOOL bShowDesktop;
    BOOL bToggleAutoHide;
} TBSETTINGS;

static TBSETTINGS g_oldSettings;
static TBSETTINGS g_newSettings;

static HWND g_hDlg;

static
void LoadDefaultSettings(void)
{
    g_oldSettings.bAnimations     = TRUE;
    g_oldSettings.bSaveThumbnails = FALSE;
    g_oldSettings.bWinXPowerShell = FALSE;  /* TRUE starting from 1703 */
    g_oldSettings.bShowDesktop    = TRUE;
    g_oldSettings.bToggleAutoHide = FALSE;
}

static
void LoadRegSettings(void)
{
    HKEY hKey;
    LSTATUS status;
    DWORD dwType;
    DWORD dwData = 0;
    DWORD dwSize;

#define ReadDword(valueName) \
    dwSize = sizeof(DWORD); \
    status = RegQueryValueEx(hKey, valueName, 0, \
        &dwType, (BYTE *)&dwData, &dwSize)

#define ReadInt(valueName, member) \
    ReadDword(valueName); \
    if (status == ERROR_SUCCESS && dwType == REG_DWORD) \
        g_oldSettings.member = (int)dwData

#define ReadInvertedBool(valueName, member) \
    ReadDword(valueName); \
    if (status == ERROR_SUCCESS && dwType == REG_DWORD) \
        g_oldSettings.member = !dwData

    status = RegOpenKeyEx(HKEY_CURRENT_USER, g_explorerKey, 0,
        KEY_QUERY_VALUE, &hKey);
    if (status == ERROR_SUCCESS)
    {
        ReadInt(TEXT("TaskbarAnimations"), bAnimations);
        ReadInvertedBool(TEXT("DontUsePowerShellOnWinX"), bWinXPowerShell);
        ReadInt(TEXT("TaskbarSd"), bShowDesktop);
        RegCloseKey(hKey);
    }

    status = RegOpenKeyEx(HKEY_CURRENT_USER, g_dwmKey, 0,
        KEY_QUERY_VALUE, &hKey);
    if (status == ERROR_SUCCESS)
    {
        ReadInt(TEXT("AlwaysHibernateThumbnails"), bSaveThumbnails);
        RegCloseKey(hKey);
    }

    status = RegOpenKeyEx(HKEY_CURRENT_USER, g_explorerPatcherKey, 0,
        KEY_QUERY_VALUE, &hKey);
    if (status == ERROR_SUCCESS)
    {
        ReadInt(TEXT("TaskbarAutohideOnDoubleClick"), bToggleAutoHide);
        RegCloseKey(hKey);
    }
    
#undef ReadInvertedBool
#undef ReadInt
#undef ReadDword
}

static
void LoadSettings(void)
{
    LoadDefaultSettings();
    LoadRegSettings();

    g_newSettings = g_oldSettings;
}

static
void UpdateControls(void)
{
#define SetChecked(iControl, bChecked) \
    SendDlgItemMessage(g_hDlg, iControl, \
        BM_SETCHECK, (WPARAM)(bChecked == 1), 0L)

    SetChecked(IDC_ADV_ANIMATIONS,     g_oldSettings.bAnimations);
    SetChecked(IDC_ADV_SAVETHUMBNAILS, g_oldSettings.bSaveThumbnails);
    SetChecked(IDC_ADV_WINXPOWERSHELL, g_oldSettings.bWinXPowerShell);
    SetChecked(IDC_ADV_SHOWDESKTOP,    g_oldSettings.bShowDesktop);
    SetChecked(IDC_ADV_TOGGLEAUTOHIDE, g_oldSettings.bToggleAutoHide);

#undef SetChecked
}

static
void InitPage(void)
{
    LoadSettings();
    UpdateControls();
}

#define HasChanged(member) \
    (g_newSettings.member != g_oldSettings.member)

_Success_(return)
static
BOOL WriteRegSettings(void)
{
    HKEY hKey;
    LSTATUS status;
    BOOL ret = TRUE;
    DWORD dwData;

#define RestoreSetting(member) \
    g_newSettings.member = g_oldSettings.member

#define SetDword(valueName) \
    status = RegSetValueEx( \
        hKey, valueName, 0, REG_DWORD, (BYTE *)&dwData, sizeof(DWORD))

#define UpdateDword(valueName, member) \
    if (HasChanged(member)) { \
        dwData = (DWORD)g_newSettings.member; \
        SetDword(valueName); \
        if (status != ERROR_SUCCESS) { \
            RestoreSetting(member); \
            ret = FALSE; \
        } \
    }

#define UpdateDwordInverted(valueName, member) \
    if (HasChanged(member)) { \
        dwData = (DWORD)!g_newSettings.member; \
        SetDword(valueName); \
        if (status != ERROR_SUCCESS) { \
            RestoreSetting(member); \
            ret = FALSE; \
        } \
    }

    if (HasChanged(bAnimations) || HasChanged(bWinXPowerShell) ||
        HasChanged(bShowDesktop))
    {
        status = RegCreateKeyEx(HKEY_CURRENT_USER, g_explorerKey, 0, NULL, 0,
            KEY_SET_VALUE, NULL, &hKey, NULL);
        if (status == ERROR_SUCCESS)
        {
            UpdateDword(TEXT("TaskbarAnimations"), bAnimations);
            UpdateDwordInverted(TEXT("DontUsePowerShellOnWinX"),
                bWinXPowerShell);
            UpdateDword(TEXT("TaskbarSd"), bShowDesktop);
            RegCloseKey(hKey);
        }
        else
        {
            RestoreSetting(bAnimations);
            RestoreSetting(bWinXPowerShell);
            RestoreSetting(bShowDesktop);
            ret = FALSE;
        }
    }

    if (HasChanged(bSaveThumbnails))
    {
        status = RegCreateKeyEx(HKEY_CURRENT_USER, g_dwmKey, 0, NULL,
            0, KEY_SET_VALUE, NULL, &hKey, NULL);
        if (status == ERROR_SUCCESS)
        {
            UpdateDword(TEXT("AlwaysHibernateThumbnails"), bSaveThumbnails);
            RegCloseKey(hKey);
        }
        else
        {
            RestoreSetting(bSaveThumbnails);
            ret = FALSE;
        }
    }

    if (HasChanged(bToggleAutoHide))
    {
        status = RegCreateKeyEx(HKEY_CURRENT_USER, g_explorerPatcherKey, 0, NULL,
            0, KEY_SET_VALUE, NULL, &hKey, NULL);
        if (status == ERROR_SUCCESS)
        {
            UpdateDword(TEXT("TaskbarAutohideOnDoubleClick"), bToggleAutoHide);
            RegCloseKey(hKey);
        }
        else
        {
            RestoreSetting(bToggleAutoHide);
            ret = FALSE;
        }
    }

    if (HasChanged(bAnimations) || HasChanged(bSaveThumbnails))
        SetCustomVisualFx();

#undef UpdateDwordInverted
#undef UpdateDword
#undef SetDword
#undef RestoreSetting

    return ret;
}

static
void ApplySettings(void)
{
    BOOL bSendSettingChange = HasChanged(bAnimations) ||
        HasChanged(bShowDesktop) || HasChanged(bToggleAutoHide);

    if (!WriteRegSettings())
    {
        /* An error has ocurred, show previous settings */
        UpdateControls();
    }

    g_oldSettings = g_newSettings;

    if (bSendSettingChange)
    {
        SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE,
            0L, (LPARAM)TEXT("TraySettings"));
    }
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
        Sleep(800);
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
    case IDC_ADV_ANIMATIONS:
        g_newSettings.bAnimations = GetChecked();
        break;

    case IDC_ADV_SAVETHUMBNAILS:
        g_newSettings.bSaveThumbnails = GetChecked();
        break;

    case IDC_ADV_WINXPOWERSHELL:
        g_newSettings.bWinXPowerShell = GetChecked();
        break;

    case IDC_ADV_SHOWDESKTOP:
        g_newSettings.bShowDesktop = GetChecked();
        break;

    case IDC_ADV_TOGGLEAUTOHIDE:
        g_newSettings.bToggleAutoHide = GetChecked();
        break;

    default:
        return;
    }

    EnableApply();

#undef GetChecked
}

INT_PTR CALLBACK AdvancedPageProc(
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
    }

    return 0;
}
