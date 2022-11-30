/*
 * COPYRIGHT:	See COPYING in the top level directory
 * PROJECT:	Taskbar Properties
 * PURPOSE:	Taskbar Properties dialog
 *
 * PROGRAMMER:	Franco Tortoriello (torto09@gmail.com)
 */

#include "colorbtn.h"
#include "dwmutil.h"
#include <psapi.h>    /* for killing explorer.exe */
#include <shellapi.h> /* for starting explorer.exe */

static const WCHAR g_TaskbarCfgKey[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";
static const WCHAR g_StuckRects3Key[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects3";
static const WCHAR g_PersonalizeKey[] = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";

static HWND hWndDlg;
static HWND hWndTaskbar;
static HWND hWndRestartExp;

static const unsigned char COLOR_ACCENT = 0; /* Win8+ taskbar, start menu and action center. Also sets the DWM color. */
static const unsigned char COLOR_DWM = 1;    /* DWM window titlebars and borders */
COLORREF crSysColor[2];
HBITMAP hBmpColorBtn[2];

static void InitComboboxes(void)
{
    UINT iElement;
    UINT iListIndex;
    WCHAR szText[60];

#define INIT_COMBO(__control, __stringid, __nelements) \
    for (iElement = 0; iElement < __nelements; iElement++) { \
        LoadStringW(hInstance, __stringid + iElement, (WCHAR*)&szText, 59); \
        iListIndex = (UINT)SendDlgItemMessageW(hWndDlg, __control, CB_ADDSTRING, 0, (LPARAM)&szText); \
    }

    INIT_COMBO(IDC_TB_LOCATION, IDS_TB_POS_L, 4);
    INIT_COMBO(IDC_TB_COMBINEBTN, IDS_TB_COMB_YES, 3);
    INIT_COMBO(IDC_TB_MMCOMBINEBTN, IDS_TB_COMB_YES, 3);
    INIT_COMBO(IDC_TB_MMDISPLAYS, IDS_TB_MMALL, 3);
#undef INIT_COMBO
}

#define SET_CHECKED(__control, __checked) \
    SendDlgItemMessageW(hWndDlg, __control, BM_SETCHECK, TRUE == __checked, 0)

#define SET_COMBO_INDEX(__control, __index) \
    SendDlgItemMessageW(hWndDlg, __control, CB_SETCURSEL, __index, 0)

#define SET_NOT_CHECKED(__control, __checked) \
    SendDlgItemMessageW(hWndDlg, __control, BM_SETCHECK, FALSE == __checked, 0)

#define IF_READ_DWORD(__value) \
    result = RegQueryValueExW(hKey, __value, 0, &valueType, (BYTE *)&iData, &cbData); \
    if ((ERROR_SUCCESS == result) && (REG_DWORD == valueType))

#define DWORD_TO_COMBO(__regvalue, __control) \
    IF_READ_DWORD(__regvalue) \
        SET_COMBO_INDEX(__control, iData);
    /*else \
        SET_COMBO_INDEX(__control, 0)*/

static void LoadTaskbarSettings(void)
{
    HKEY hKey = NULL;
    LSTATUS result = RegOpenKeyExW(HKEY_CURRENT_USER, g_TaskbarCfgKey, 0, KEY_READ, &hKey);
    if (ERROR_SUCCESS != result) return;
    
    DWORD valueType = REG_DWORD;
    DWORD cbData = sizeof(DWORD);
    DWORD iData;

    IF_READ_DWORD(L"TaskbarSizeMove")
        SET_NOT_CHECKED(IDC_TB_LOCK, iData);
    else
        /* Default to checked */
        SET_CHECKED(IDC_TB_LOCK, TRUE);

    IF_READ_DWORD(L"TaskbarSmallIcons")
        SET_CHECKED(IDC_TB_SMBTN, iData);

    IF_READ_DWORD(L"DisablePreviewDesktop")
        SET_NOT_CHECKED(IDC_TB_PEEK, iData);

    IF_READ_DWORD(L"MMTaskbarEnabled")
        SET_CHECKED(IDC_TB_ALLDISPLAYS, iData);

    IF_READ_DWORD(L"DontUsePowerShellOnWinX")
        SET_NOT_CHECKED(IDC_TB_WINXPS, iData);
#if 0
    else
        /* Default on 1703+ */
        SET_CHECKED(IDC_TB_WINXPS, TRUE);
#endif

    /* Comboboxes */

    DWORD_TO_COMBO(L"TaskbarGlomLevel", IDC_TB_COMBINEBTN);
    DWORD_TO_COMBO(L"MMTaskbarGlomLevel", IDC_TB_MMCOMBINEBTN);
    DWORD_TO_COMBO(L"MMTaskbarMode", IDC_TB_MMDISPLAYS);
#undef INIT_COMBO_REG

    RegCloseKey(hKey);
}

static void LoadPersonalizeSettings(void)
{
    HKEY hKey = NULL;
    LSTATUS result = RegOpenKeyExW(HKEY_CURRENT_USER, g_PersonalizeKey, 0, KEY_READ, &hKey);
    if (ERROR_SUCCESS != result) return;

    DWORD valueType = REG_DWORD;
    DWORD cbData = sizeof(DWORD);
    DWORD iData;

#if 0
    /* 1903 */
    IF_READ_DWORD(L"SystemUsesLightTheme")
        SET_NOT_CHECKED(IDC_TB_SYSDARK, iData);
    else
        SET_CHECKED(IDC_TB_SYSDARK, TRUE);

    IF_READ_DWORD(L"AppsUseLightTheme")
        SET_NOT_CHECKED(IDC_TB_APPDARK, iData);
#endif

    IF_READ_DWORD(L"EnableTransparency")
        SET_CHECKED(IDC_TB_TRANSP, iData);

    IF_READ_DWORD(L"ColorPrevalence")
        SET_CHECKED(IDC_TB_USEACCENTCOLOR, iData);

    RegCloseKey(hKey);
}

#if 0
static void LoadDwmSettings(void)
{
    HKEY hKey = NULL;
    LSTATUS result = RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Microsoft\\Windows\\DWM", 0, KEY_READ, &hKey);
    if (ERROR_SUCCESS != result) return;

    DWORD valueType = REG_DWORD;
    DWORD cbData = sizeof(DWORD);
    DWORD iData;

    IF_READ_DWORD(L"ColorPrevalence")
        SET_CHECKED(IDC_TB_USECOLORTITLE, iData);

    RegCloseKey(hKey);
}
#endif

#undef SET_CHECKED
#undef SET_NOT_CHECKED
#undef SET_COMBO_INDEX
#undef IF_READ_DWORD
#undef DWORD_TO_COMBO

static void LoadStuckRects3(void)
{
    HKEY hKey = NULL;
    LSTATUS result = RegOpenKeyExW(HKEY_CURRENT_USER, g_StuckRects3Key, 0, KEY_READ, &hKey);
    if (ERROR_SUCCESS != result)
        return;

    DWORD valueType = REG_DWORD;
    DWORD cbData = 48;
    BYTE stuckRects3[48];

    result = RegQueryValueExW(hKey, L"Settings", 0, &valueType, stuckRects3, &cbData);

    if ((ERROR_SUCCESS == result) && (REG_BINARY == valueType))
    {
        SendDlgItemMessageW(hWndDlg, IDC_TB_AUTOHIDE, BM_SETCHECK, (stuckRects3[8] == 3), 0);
        SendDlgItemMessageW(hWndDlg, IDC_TB_LOCATION, CB_SETCURSEL, stuckRects3[12], 0);
    }
    else
    {
        SendDlgItemMessageW(hWndDlg, IDC_TB_AUTOHIDE, BM_SETCHECK, BST_UNCHECKED, 0);
        SendDlgItemMessageW(hWndDlg, IDC_TB_LOCATION, CB_SETCURSEL, 3, 0); /* bottom */
    }

    RegCloseKey(hKey);
}

static void Write_StuckRects3(UINT index, BYTE value)
{
    HKEY hKey = NULL;
    LSTATUS result = RegOpenKeyExW(HKEY_CURRENT_USER, g_StuckRects3Key, 0, KEY_READ | KEY_WRITE, &hKey);
    if (ERROR_SUCCESS != result)
        return;

    BYTE stuckRects3[48];
    DWORD valueType = REG_BINARY;
    DWORD cbData = 48;

    result = RegQueryValueExW(hKey, L"Settings", 0, &valueType, stuckRects3, &cbData);
    if ((ERROR_SUCCESS == result) && (REG_BINARY == valueType))
    {
        stuckRects3[index] = value;
        RegSetValueExW(hKey, L"Settings", 0, valueType, stuckRects3, cbData);
    }
    RegCloseKey(hKey); 
}

static void SetWndIcon(void)
{
    WCHAR szFilePath[60];
    HICON hIcon;

    if (!GetWindowsDirectoryW(szFilePath, 40)) return;
    if (!lstrcatW(szFilePath, L"\\System32\\imageres.dll")) return;
    if (ExtractIconExW(szFilePath, 75, &hIcon, NULL, 1) <= 0) return;
    SendMessageW(hWndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
}

static inline void SetAccentButtonColor(void)
{
    SetColorButtonColor(hWndDlg, IDC_TB_ACCENTCOLOR, &hBmpColorBtn[COLOR_ACCENT], crSysColor[COLOR_ACCENT]);
}

static inline void SetDwmButtonColor(void)
{
    SetColorButtonColor(hWndDlg, IDC_TB_DWMCOLOR, &hBmpColorBtn[COLOR_DWM], crSysColor[COLOR_DWM]);
}

static void InitTaskbarSettingsDlg(HWND hWnd)
{
    hWndDlg = hWnd;
    hWndTaskbar = FindWindowW(L"Shell_TrayWnd", L"");
    hWndRestartExp = GetDlgItem(hWndDlg, IDC_RESTARTEXP);

    SetWndIcon();
    InitComboboxes();

    /* Disable the Restart Explorer button; it will be enabled when a setting requires it */
    EnableWindow(hWndRestartExp, FALSE);

    LoadTaskbarSettings();
    LoadPersonalizeSettings();
    /*LoadDwmSettings();*/
    LoadStuckRects3();

    if (LoadColorPreferenceFuncs())
    {
        crSysColor[COLOR_ACCENT] = GetAccentColor();
        SetAccentButtonColor();
    }
    else
    {
        /* The functions could not be loaded, disable the button */
        SendDlgItemMessageW(hWndDlg, IDC_TB_ACCENTCOLOR, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)NULL);
        EnableWindow(GetDlgItem(hWndDlg, IDC_TB_ACCENTCOLOR), FALSE);
    }

    if (LoadDwmColorizationFuncs())
    {
        crSysColor[COLOR_DWM] = GetDwmColor();
        SetDwmButtonColor();
    }
    else
    {
        SendDlgItemMessageW(hWndDlg, IDC_TB_DWMCOLOR, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)NULL);
        EnableWindow(GetDlgItem(hWndDlg, IDC_TB_DWMCOLOR), FALSE);
    }
}

static void RestartExplorer(void)
{
    DWORD procId[1024];
    DWORD cbNeeded;

    if (!EnumProcesses(procId, sizeof(procId), &cbNeeded))
        return;

    /* number of process IDs returned */
    DWORD cProc = cbNeeded / sizeof(DWORD);

    HANDLE hProc;
    HMODULE hMod;
    WCHAR szProcessName[13] = L"";
    BOOL bKilled = FALSE;

    for (UINT i = 0; i < cProc; i++)
    {
        hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_TERMINATE, FALSE, procId[i]);
        if (!hProc)
            continue;

        if (EnumProcessModules(hProc, &hMod, sizeof(hMod), &cbNeeded))
        {
            GetModuleBaseNameW(hProc, hMod, szProcessName, sizeof(szProcessName) / sizeof(WCHAR));
            if (0 == lstrcmpiW(szProcessName, L"explorer.exe"))
            {
                bKilled = TerminateProcess(hProc, 1);
#if 0
                /* close only the first instance; if folders are opened in
                    separate processes, try to keep them open
                    (does not work if explorer was already killed, as the
                    shell process will have a higher PID than File Explorer)
                    */
                CloseHandle(hProc);
                break;
#endif
            }
        }
        CloseHandle(hProc);
    }

    /*if (bKilled)*/
    {
        ShellExecuteW(NULL, L"open", L"explorer.exe", NULL, NULL, SW_SHOWNORMAL);
        Sleep(200);
        hWndTaskbar = FindWindowW(L"Shell_TrayWnd", L"");
    }
}

static void WriteDword(LPCWSTR key, LPCWSTR value, DWORD data)
{
    HKEY hKey;
    LSTATUS result = RegCreateKeyExW(HKEY_CURRENT_USER, key, 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);
    if (ERROR_SUCCESS != result)
        return;

    RegSetValueExW(hKey, value, 0, REG_DWORD, (BYTE *)&data, sizeof(DWORD));
    RegCloseKey(hKey);
}

static BOOL TaskbarSettingsDlgCommand(WORD control)
{
    BYTE iData;

#define GET_CHECKED() \
    iData = (BST_CHECKED == SendDlgItemMessageW(hWndDlg, control, BM_GETCHECK, 0, 0))

#define GET_NOT_CHECKED() \
    iData = (BST_CHECKED != SendDlgItemMessageW(hWndDlg, control, BM_GETCHECK, 0, 0))

    switch (control)
    {
    case IDOK:
    case IDCANCEL: /* close button and Escape key */
        EndDialog(hWndDlg, control);
        break;

    case IDC_TB_TRAYWND:
        ShellExecuteW(NULL, L"open", L"explorer.exe", L"shell:::{05d7b0f4-2121-4eff-bf6b-ed3f69b894d9}", NULL, SW_SHOWNORMAL);
        break;

    case IDC_RESTARTEXP:
        RestartExplorer();
        break;

    case IDC_TB_LOCK:
        GET_NOT_CHECKED();
        WriteDword(g_TaskbarCfgKey, L"TaskbarSizeMove", iData);
        SendMessageW(hWndTaskbar, WM_USER + 458, 2, iData);
        break;

    case IDC_TB_SMBTN:
        GET_CHECKED();
        WriteDword(g_TaskbarCfgKey, L"TaskbarSmallIcons", iData);
        EnableWindow(hWndRestartExp, TRUE);
        break;

    case IDC_TB_PEEK:
        GET_NOT_CHECKED();
        WriteDword(g_TaskbarCfgKey, L"DisablePreviewDesktop", iData);
        EnableWindow(hWndRestartExp, TRUE);
        break;

    case IDC_TB_ALLDISPLAYS:
        GET_CHECKED();
        WriteDword(g_TaskbarCfgKey, L"MMTaskbarEnabled", iData);
        EnableWindow(hWndRestartExp, TRUE);
        break;

    case IDC_TB_WINXPS:
        GET_NOT_CHECKED();
        WriteDword(g_TaskbarCfgKey, L"DontUsePowerShellOnWinX", iData);
        EnableWindow(hWndRestartExp, TRUE);
        break;

#if 0
    case IDC_TB_SYSDARK:
        /* 1903 */
        GET_NOT_CHECKED();
        WriteDword(g_PersonalizeKey, L"SystemUsesLightTheme", iData);
        EnableWindow(hWndRestartExp, TRUE);
        break;

    case IDC_TB_APPDARK:
        GET_NOT_CHECKED();
        WriteDword(g_PersonalizeKey, L"AppsUseLightTheme", iData);
        EnableWindow(hWndRestartExp, TRUE);
        break;

    case IDC_TB_USECOLORTITLE:
        GET_CHECKED();
        WriteDword(L"SOFTWARE\\Microsoft\\Windows\\DWM", L"ColorPrevalence", iData);
        EnableWindow(hWndRestartExp, TRUE);
        break;
#endif

    case IDC_TB_TRANSP:
        GET_CHECKED();
        WriteDword(g_PersonalizeKey, L"EnableTransparency", iData);
        EnableWindow(hWndRestartExp, TRUE);
        break;

    case IDC_TB_USEACCENTCOLOR:
        GET_CHECKED();
        WriteDword(g_PersonalizeKey, L"ColorPrevalence", iData);
        EnableWindow(hWndRestartExp, TRUE);
        break;

    case IDC_TB_AUTOHIDE:
        GET_CHECKED();
        Write_StuckRects3(8, 2 + iData);
        SendMessageW(hWndTaskbar, WM_USER + 458, 4, iData);
        break;

    case IDC_TB_ACCENTCOLOR:
        if (!PickColor(hWndDlg, &crSysColor[COLOR_ACCENT])) break;

        SetAccentColor(crSysColor[COLOR_ACCENT]);
        SetAccentButtonColor();
        /* it also sets the DWM color */
#if 0
        /* update the DWM color and button too */
        crSysColor[COLOR_DWM] = GetDwmColor();
        UpdateButtonColor(hWndDlg, IDC_TB_DWMCOLOR, hbmpBtnColor[COLOR_DWM], crSysColor[COLOR_DWM]);
#else
        /* leave the DWM color as it was */
        SetDwmColor(crSysColor[COLOR_DWM]);
#endif
        break;

    case IDC_TB_DWMCOLOR:
        if (!PickColor(hWndDlg, &crSysColor[COLOR_DWM])) break;

        SetDwmColor(crSysColor[COLOR_DWM]);
        SetDwmButtonColor();
        break;

    default:
        return FALSE;
    }

    return TRUE;

#undef GET_CHECKED
#undef GET_NOT_CHECKED
}

static BOOL TaskbarSettingsDlgCombo(WORD control)
{
    BYTE iPos;

#define GET_COMBO_INDEX() \
    iPos = (BYTE)SendDlgItemMessageW(hWndDlg, control, CB_GETCURSEL, 0, 0)

#define WRITE_COMBO_INDEX(__value) \
    GET_COMBO_INDEX(); \
    WriteDword(g_TaskbarCfgKey, __value, iPos)

    switch (control)
    {
    case IDC_TB_LOCATION:
        GET_COMBO_INDEX();
        Write_StuckRects3(12, iPos);
        SendMessageW(hWndTaskbar, WM_USER + 458, 6, iPos);
        break;

    case IDC_TB_COMBINEBTN:
        WRITE_COMBO_INDEX(L"TaskbarGlomLevel");
        EnableWindow(hWndRestartExp, TRUE);
        break;

    case IDC_TB_MMCOMBINEBTN:
        WRITE_COMBO_INDEX(L"MMTaskbarGlomLevel");
        EnableWindow(hWndRestartExp, TRUE);
        break;

    case IDC_TB_MMDISPLAYS:
        WRITE_COMBO_INDEX(L"MMTaskbarMode");
        EnableWindow(hWndRestartExp, TRUE);
        break;

    default:
        return FALSE;
    }

    return TRUE;
#undef GET_COMBO_INDEX
#undef WRITE_COMBO_INDEX
}

INT_PTR CALLBACK TaskbarSettingsDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        InitTaskbarSettingsDlg(hWnd);
        break;

    case WM_DESTROY:
        UnloadColorPreferenceFuncs();
        UnloadDwmColorizationFuncs();
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