/*
 * COPYRIGHT: See COPYING in the top level directory
 * PURPOSE:   Taskbar Properties dialog
 *
 * PROGRAMMER: Franco Tortoriello (torto09@gmail.com)
 */

#include "resource.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <psapi.h>
#include <shellapi.h>

extern HINSTANCE g_hInstance;

static const TCHAR g_taskbarKey[] =
    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced");

static const TCHAR g_stuckRectsKey[] =
    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects3");

static HWND g_hDlg;
static HWND g_hTaskbar;

static
void InitComboboxes(void)
{
    UINT iElement;
    TCHAR text[60];

#define InitCombo(iControl, iString, nElements) \
    for (iElement = 0; iElement < nElements; iElement++) { \
        LoadString(g_hInstance, iString + iElement, (TCHAR *)&text, 59); \
        SendDlgItemMessage(g_hDlg, iControl, CB_ADDSTRING, 0, (LPARAM)&text); \
    }

    InitCombo(IDC_TB_LOCATION, IDS_TB_POS_L, 4);
    InitCombo(IDC_TB_COMBINEBTN, IDS_TB_COMB_YES, 3);
    InitCombo(IDC_TB_MMDISPLAYS, IDS_TB_MMALL, 3);
    InitCombo(IDC_TB_MMCOMBINEBTN, IDS_TB_COMB_YES, 3);
#undef InitCombo
}

#define SetChecked(iControl, bChecked) \
    SendDlgItemMessage(g_hDlg, iControl, BM_SETCHECK, bChecked == TRUE, 0)

#define SetUnchecked(iControl, bChecked) \
    SendDlgItemMessage(g_hDlg, iControl, BM_SETCHECK, bChecked == FALSE, 0)

#define SetComboIndex(iControl, index) \
    SendDlgItemMessage(g_hDlg, iControl, CB_SETCURSEL, (WPARAM)index, 0)

#define IfReadDWord(value) \
    status = RegQueryValueEx(hKey, value, 0, &dwType, (BYTE *)&iData, &cbData); \
    if (status == ERROR_SUCCESS && dwType == REG_DWORD)

#define DWord2Combo(regValue, iControl, defaultIndex) \
    IfReadDWord(regValue) \
        SetComboIndex(iControl, iData); \
    else \
        SetComboIndex(iControl, defaultIndex)

static
void LoadTaskbarSettings(void)
{
    HKEY hKey = NULL;
    LSTATUS status = RegOpenKeyEx(
        HKEY_CURRENT_USER, g_taskbarKey, 0, KEY_READ, &hKey);

    if (status != ERROR_SUCCESS) return;
    
    DWORD dwType;
    DWORD cbData = sizeof(DWORD);
    DWORD iData;

    IfReadDWord(TEXT("TaskbarSizeMove"))
        SetUnchecked(IDC_TB_LOCK, iData);
    else
        SetChecked(IDC_TB_LOCK, TRUE);

    IfReadDWord(TEXT("TaskbarSmallIcons"))
        SetChecked(IDC_TB_SMBTN, iData);

    IfReadDWord(TEXT("DisablePreviewDesktop"))
        SetUnchecked(IDC_TB_PEEK, iData);

    IfReadDWord(TEXT("MMTaskbarEnabled"))
        SetChecked(IDC_TB_ALLDISPLAYS, iData);
    else
        SetChecked(IDC_TB_ALLDISPLAYS, TRUE);

    IfReadDWord(TEXT("DontUsePowerShellOnWinX"))
        SetUnchecked(IDC_TB_WINXPS, iData);
#if 0
    else
        /* Default on 1703+ */
        SET_CHECKED(IDC_TB_WINXPS, TRUE);
#endif

    /* Comboboxes */

    DWord2Combo(TEXT("TaskbarGlomLevel"), IDC_TB_COMBINEBTN, 0);
    DWord2Combo(TEXT("MMTaskbarMode"), IDC_TB_MMDISPLAYS, 0);
    DWord2Combo(TEXT("MMTaskbarGlomLevel"), IDC_TB_MMCOMBINEBTN, 0);
#undef INIT_COMBO_REG

    RegCloseKey(hKey);
}

#undef SetChecked
#undef SetUnchecked
#undef SetComboIndex
#undef IfReadDWord
#undef DWord2Combo

static
void LoadStuckRects3(void)
{
    HKEY hKey = NULL;
    LSTATUS status = RegOpenKeyEx(
        HKEY_CURRENT_USER, g_stuckRectsKey, 0, KEY_READ, &hKey);
    if (status != ERROR_SUCCESS)
        return;

    DWORD dwType;
    DWORD cbData = 48;
    BYTE stuckRects3[48];

    status = RegQueryValueEx(
        hKey, TEXT("Settings"), 0, &dwType, stuckRects3, &cbData);

    if (status == ERROR_SUCCESS && dwType == REG_BINARY)
    {
        SendDlgItemMessage(
            g_hDlg, IDC_TB_AUTOHIDE, BM_SETCHECK, (stuckRects3[8] == 3), 0);

        SendDlgItemMessage(
            g_hDlg, IDC_TB_LOCATION, CB_SETCURSEL, stuckRects3[12], 0);
    }
    else
    {
        SendDlgItemMessage(
            g_hDlg, IDC_TB_AUTOHIDE, BM_SETCHECK, BST_UNCHECKED, 0);

        /* Default to bottom */
        SendDlgItemMessage(
            g_hDlg, IDC_TB_LOCATION, CB_SETCURSEL, 3, 0);
    }

    RegCloseKey(hKey);
}

static void Write_StuckRects3(UINT index, BYTE value)
{
    HKEY hKey = NULL;
    LSTATUS status = RegOpenKeyEx(
        HKEY_CURRENT_USER, g_stuckRectsKey, 0, KEY_READ | KEY_WRITE, &hKey);
    if (status != ERROR_SUCCESS)
        return;

    BYTE stuckRects3[48];
    DWORD dwType;
    DWORD cbData = 48;

    status = RegQueryValueEx(
        hKey, TEXT("Settings"), 0, &dwType, stuckRects3, &cbData);
    if (status == ERROR_SUCCESS && dwType == REG_BINARY)
    {
        stuckRects3[index] = value;
        RegSetValueEx(hKey, TEXT("Settings"), 0, dwType, stuckRects3, cbData);
    }
    RegCloseKey(hKey); 
}

static
void SetWndIcon(void)
{
    TCHAR szFilePath[60];
    HICON hIcon;

    if (!GetWindowsDirectory(szFilePath, 40))
        return;

    if (!lstrcat(szFilePath, TEXT("\\System32\\imageres.dll")))
        return;

    if (ExtractIconEx(szFilePath, 75, &hIcon, NULL, 1) <= 0)
        return;

    SendMessage(g_hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
}

static
void InitTaskbarSettingsDlg(HWND hWnd)
{
    g_hDlg = hWnd;
    g_hTaskbar = FindWindow(TEXT("Shell_TrayWnd"), TEXT(""));

    SetWndIcon();
    InitComboboxes();

    /* Disable the Restart Explorer button; it will be enabled when a setting requires it */
    EnableWindow(GetDlgItem(hWnd, IDC_TB_RESTARTEXPLORER), FALSE);

    LoadTaskbarSettings();
    /*LoadDwmSettings();*/
    LoadStuckRects3();
}

static
void RestartExplorer(void)
{
    DWORD procId[1024];
    DWORD cbNeeded;

    if (!EnumProcesses(procId, sizeof(procId), &cbNeeded))
        return;

    /* Number of process IDs returned */
    DWORD cProc = cbNeeded / sizeof(DWORD);

#define PROCESSNAME_MAX_LENGTH 13
    TCHAR processName[PROCESSNAME_MAX_LENGTH] = TEXT("");

    HANDLE hProc;
    HMODULE hMod;
    BOOL bKilled = FALSE;

    for (UINT i = 0; i < cProc; i++)
    {
        hProc = OpenProcess(
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE,
            FALSE, procId[i]);
        if (!hProc)
            continue;

        if (EnumProcessModules(hProc, &hMod, sizeof(hMod), &cbNeeded))
        {
            GetModuleBaseName(hProc, hMod, processName, PROCESSNAME_MAX_LENGTH);
            if (lstrcmpi(processName, TEXT("explorer.exe")) == 0)
            {
                bKilled |= TerminateProcess(hProc, 1);
#if 0
                /* Close only the first instance; if folders are opened in
                 * separate processes, try to keep them open.
                 * This ddoes not work if explorer was already killed, as the
                 * shell process will have a higher PID than File Explorer...
                 */
                CloseHandle(hProc);
                break;
#endif
            }
        }

        CloseHandle(hProc);
    }

#undef PROCESSNAME_MAX_LENGTH

    if (!bKilled)
    {
        g_hTaskbar = FindWindow(TEXT("Shell_TrayWnd"), TEXT(""));
        if (g_hTaskbar)
        {
            /* Could not kill the process, and there is a taskbar */
            return;
        }
    }

    Sleep(200);
    ShellExecute(NULL, TEXT("open"), TEXT("explorer.exe"), NULL, NULL, SW_SHOWNORMAL);

    Sleep(1800);
    g_hTaskbar = FindWindow(TEXT("Shell_TrayWnd"), TEXT(""));
}

static
void WriteDword(const TCHAR *key, const TCHAR *value, DWORD data)
{
    HKEY hKey;
    LSTATUS status = RegCreateKeyEx(
        HKEY_CURRENT_USER, key, 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);
    if (status != ERROR_SUCCESS)
        return;

    RegSetValueEx(hKey, value, 0, REG_DWORD, (BYTE *)&data, sizeof(DWORD));
    RegCloseKey(hKey);
}

static
BOOL TaskbarSettingsDlgCommand(WORD control)
{
    BYTE iData;
    
#define GetChecked() iData = \
    (SendDlgItemMessage(g_hDlg, control, BM_GETCHECK, 0, 0) == BST_CHECKED)

#define GetUnchecked() iData = \
    (SendDlgItemMessage(g_hDlg, control, BM_GETCHECK, 0, 0) != BST_CHECKED)

    switch (control)
    {
    case IDOK:
    case IDCANCEL: /* close button and Escape key */
        EndDialog(g_hDlg, control);
        break;

    case IDC_TB_TRAYWND:
        ShellExecute(NULL, TEXT("open"), TEXT("explorer.exe"),
            TEXT("shell:::{05d7b0f4-2121-4eff-bf6b-ed3f69b894d9}"),
            NULL, SW_SHOWNORMAL);
        break;

    case IDC_TB_LOCK:
        GetUnchecked();
        WriteDword(g_taskbarKey, TEXT("TaskbarSizeMove"), iData);
        SendNotifyMessage(g_hTaskbar, WM_USER + 458, 2, iData);
        break;

    case IDC_TB_SMBTN:
        GetChecked();
        WriteDword(g_taskbarKey, TEXT("TaskbarSmallIcons"), iData);
        SendNotifyMessage(
            HWND_BROADCAST, WM_SETTINGCHANGE, 0L, (LPARAM)TEXT("TraySettings"));
        break;

    case IDC_TB_PEEK:
        GetUnchecked();
        WriteDword(g_taskbarKey, TEXT("DisablePreviewDesktop"), iData);
        SendNotifyMessage(
            HWND_BROADCAST, WM_SETTINGCHANGE, 0L, (LPARAM)TEXT("TraySettings"));
        break;

    case IDC_TB_ALLDISPLAYS:
        GetChecked();
        WriteDword(g_taskbarKey, TEXT("MMTaskbarEnabled"), iData);
        SendNotifyMessage(
            HWND_BROADCAST, WM_SETTINGCHANGE, 0L, (LPARAM)TEXT("TraySettings"));
        break;

    case IDC_TB_WINXPS:
        GetUnchecked();
        WriteDword(g_taskbarKey, TEXT("DontUsePowerShellOnWinX"), iData);
        EnableWindow(GetDlgItem(g_hDlg, IDC_TB_RESTARTEXPLORER), TRUE);
        break;

    case IDC_TB_AUTOHIDE:
        GetChecked();
        Write_StuckRects3(8, 2 + iData);
        SendNotifyMessage(g_hTaskbar, WM_USER + 458, 4, iData);
        break;

    case IDC_TB_RESTARTEXPLORER:
        RestartExplorer();
        break;

    default:
        return FALSE;
    }

    return TRUE;

#undef GetChecked
#undef GetUnchecked
}

static
BOOL TaskbarSettingsDlgCombo(WORD control)
{
    BYTE iPos;

#define GetComboIndex() \
    iPos = (BYTE)SendDlgItemMessage(g_hDlg, control, CB_GETCURSEL, 0, 0)

#define WriteComboIndex(value) \
    GetComboIndex(); \
    WriteDword(g_taskbarKey, value, iPos)

    switch (control)
    {
    case IDC_TB_LOCATION:
        GetComboIndex();
        Write_StuckRects3(12, iPos);
        SendNotifyMessage(g_hTaskbar, WM_USER + 458, 6, iPos);
        break;

    case IDC_TB_COMBINEBTN:
        WriteComboIndex(TEXT("TaskbarGlomLevel"));
        SendNotifyMessage(
            HWND_BROADCAST, WM_SETTINGCHANGE, 0L, (LPARAM)TEXT("TraySettings"));
        break;

    case IDC_TB_MMDISPLAYS:
        WriteComboIndex(TEXT("MMTaskbarMode"));
        SendNotifyMessage(
            HWND_BROADCAST, WM_SETTINGCHANGE, 0L, (LPARAM)TEXT("TraySettings"));
        break;

    case IDC_TB_MMCOMBINEBTN:
        WriteComboIndex(TEXT("MMTaskbarGlomLevel"));
        SendNotifyMessage(
            HWND_BROADCAST, WM_SETTINGCHANGE, 0L, (LPARAM)TEXT("TraySettings"));
        break;

    default:
        return FALSE;
    }

    return TRUE;
#undef GetComboIndex
#undef WriteComboIndex
}

INT_PTR CALLBACK TaskbarSettingsDlgProc(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        InitTaskbarSettingsDlg(hWnd);
        break;

    case WM_COMMAND:
        switch HIWORD(wParam)
        {
        case BN_CLICKED:
            return TaskbarSettingsDlgCommand(LOWORD(wParam));

        case CBN_SELCHANGE:
            return TaskbarSettingsDlgCombo(LOWORD(wParam));

        default:
            return FALSE;
        }

    default:
        return FALSE;
    }

    return TRUE;
}
