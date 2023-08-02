/*
 * COPYRIGHT: See COPYING in the top level directory
 * PURPOSE:   Taskbar Properties dialog
 *
 * PROGRAMMER: Franco Tortoriello (torto09@gmail.com)
 */

#include "colorbtn.h"
#include "dwmutil.h"
#include "resource.h"

#include <shellapi.h>

extern HINSTANCE g_hInstance;

static const TCHAR g_TaskbarCfgKey[] =
    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced");
static const TCHAR g_StuckRects3Key[] =
    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects3");
static const TCHAR g_PersonalizeKey[] =
    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize");

static HWND hWndDlg;
static HWND hWndTaskbar;

/* Win8+ taskbar, start menu and action center. Also sets the DWM color. */
static const unsigned char COLOR_ACCENT = 0;

/* DWM window titlebars and borders */
static const unsigned char COLOR_DWM = 1;

static COLORREF crSysColor[2];
static HBITMAP hBmpColorBtn[2];

static void InitComboboxes(void)
{
    UINT iElement;
    TCHAR text[60];

#define INIT_COMBO(iControl, iString, nElements) \
    for (iElement = 0; iElement < nElements; iElement++) { \
        LoadString(g_hInstance, iString + iElement, (TCHAR *)&text, 59); \
        SendDlgItemMessage(hWndDlg, iControl, CB_ADDSTRING, 0, (LPARAM)&text); \
    }

    INIT_COMBO(IDC_TB_LOCATION, IDS_TB_POS_L, 4);
    INIT_COMBO(IDC_TB_COMBINEBTN, IDS_TB_COMB_YES, 3);
    INIT_COMBO(IDC_TB_MMDISPLAYS, IDS_TB_MMALL, 3);
    INIT_COMBO(IDC_TB_MMCOMBINEBTN, IDS_TB_COMB_YES, 3);
#undef INIT_COMBO
}

#define SET_CHECKED(iControl, bChecked) \
    SendDlgItemMessage(hWndDlg, iControl, BM_SETCHECK, TRUE == bChecked, 0)

#define SET_COMBO_INDEX(iControl, index) \
    SendDlgItemMessage(hWndDlg, iControl, CB_SETCURSEL, (WPARAM)index, 0)

#define SET_NOT_CHECKED(iControl, bChecked) \
    SendDlgItemMessage(hWndDlg, iControl, BM_SETCHECK, FALSE == bChecked, 0)

#define IF_READ_DWORD(value) \
    result = RegQueryValueEx(hKey, value, 0, &type, (BYTE *)&iData, &cbData); \
    if ((ERROR_SUCCESS == result) && (REG_DWORD == type))

#define DWORD_TO_COMBO(regValue, iControl, defaultIndex) \
    IF_READ_DWORD(regValue) \
        SET_COMBO_INDEX(iControl, iData); \
    else \
        SET_COMBO_INDEX(iControl, defaultIndex)

static void LoadTaskbarSettings(void)
{
    HKEY hKey = NULL;
    LSTATUS result = RegOpenKeyEx(
        HKEY_CURRENT_USER, g_TaskbarCfgKey, 0, KEY_READ, &hKey);

    if (ERROR_SUCCESS != result) return;
    
    DWORD type = REG_DWORD;
    DWORD cbData = sizeof(DWORD);
    DWORD iData;

    IF_READ_DWORD(TEXT("TaskbarSizeMove"))
        SET_NOT_CHECKED(IDC_TB_LOCK, iData);
    else
        SET_CHECKED(IDC_TB_LOCK, TRUE);

    IF_READ_DWORD(TEXT("TaskbarSmallIcons"))
        SET_CHECKED(IDC_TB_SMBTN, iData);

    IF_READ_DWORD(TEXT("DisablePreviewDesktop"))
        SET_NOT_CHECKED(IDC_TB_PEEK, iData);

    IF_READ_DWORD(TEXT("MMTaskbarEnabled"))
        SET_CHECKED(IDC_TB_ALLDISPLAYS, iData);
    else
        SET_CHECKED(IDC_TB_ALLDISPLAYS, TRUE);

    IF_READ_DWORD(TEXT("DontUsePowerShellOnWinX"))
        SET_NOT_CHECKED(IDC_TB_WINXPS, iData);
#if 0
    else
        /* Default on 1703+ */
        SET_CHECKED(IDC_TB_WINXPS, TRUE);
#endif

    /* Comboboxes */

    DWORD_TO_COMBO(TEXT("TaskbarGlomLevel"), IDC_TB_COMBINEBTN, 0);
    DWORD_TO_COMBO(TEXT("MMTaskbarMode"), IDC_TB_MMDISPLAYS, 0);
    DWORD_TO_COMBO(TEXT("MMTaskbarGlomLevel"), IDC_TB_MMCOMBINEBTN, 0);
#undef INIT_COMBO_REG

    RegCloseKey(hKey);
}

static void LoadPersonalizeSettings(void)
{
    HKEY hKey = NULL;
    LSTATUS result = RegOpenKeyEx(
        HKEY_CURRENT_USER, g_PersonalizeKey, 0, KEY_READ, &hKey);
    if (ERROR_SUCCESS != result) return;

    DWORD type = REG_DWORD;
    DWORD cbData = sizeof(DWORD);
    DWORD iData;

#if 0
    /* 1903 */
    IF_READ_DWORD(TEXT("SystemUsesLightTheme"))
        SET_NOT_CHECKED(IDC_TB_SYSDARK, iData);
    else
        SET_CHECKED(IDC_TB_SYSDARK, TRUE);

    IF_READ_DWORD(TEXT("AppsUseLightTheme"))
        SET_NOT_CHECKED(IDC_TB_APPDARK, iData);
#endif

    IF_READ_DWORD(TEXT("EnableTransparency"))
        SET_CHECKED(IDC_TB_TRANSP, iData);

    IF_READ_DWORD(TEXT("ColorPrevalence"))
        SET_CHECKED(IDC_TB_USEACCENTCOLOR, iData);

    RegCloseKey(hKey);
}

#if 0
static void LoadDwmSettings(void)
{
    HKEY hKey = NULL;
    LSTATUS result = RegOpenKeyEx(HKEY_CURRENT_USER,
        TEXT("SOFTWARE\\Microsoft\\Windows\\DWM"), 0, KEY_READ, &hKey);
    if (ERROR_SUCCESS != result) return;

    DWORD type = REG_DWORD;
    DWORD cbData = sizeof(DWORD);
    DWORD iData;

    IF_READ_DWORD(TEXT("ColorPrevalence"))
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
    LSTATUS result = RegOpenKeyEx(
        HKEY_CURRENT_USER, g_StuckRects3Key, 0, KEY_READ, &hKey);
    if (ERROR_SUCCESS != result)
        return;

    DWORD type = REG_DWORD;
    DWORD cbData = 48;
    BYTE stuckRects3[48];

    result = RegQueryValueEx(
        hKey, TEXT("Settings"), 0, &type, stuckRects3, &cbData);

    if ((ERROR_SUCCESS == result) && (REG_BINARY == type))
    {
        SendDlgItemMessage(
            hWndDlg, IDC_TB_AUTOHIDE, BM_SETCHECK, (stuckRects3[8] == 3), 0);

        SendDlgItemMessage(
            hWndDlg, IDC_TB_LOCATION, CB_SETCURSEL, stuckRects3[12], 0);
    }
    else
    {
        SendDlgItemMessage(
            hWndDlg, IDC_TB_AUTOHIDE, BM_SETCHECK, BST_UNCHECKED, 0);

        /* Default to bottom */
        SendDlgItemMessage(
            hWndDlg, IDC_TB_LOCATION, CB_SETCURSEL, 3, 0);
    }

    RegCloseKey(hKey);
}

static void Write_StuckRects3(UINT index, BYTE value)
{
    HKEY hKey = NULL;
    LSTATUS result = RegOpenKeyEx(
        HKEY_CURRENT_USER, g_StuckRects3Key, 0, KEY_READ | KEY_WRITE, &hKey);
    if (ERROR_SUCCESS != result)
        return;

    BYTE stuckRects3[48];
    DWORD type = REG_BINARY;
    DWORD cbData = 48;

    result = RegQueryValueEx(
        hKey, TEXT("Settings"), 0, &type, stuckRects3, &cbData);
    if ((ERROR_SUCCESS == result) && (REG_BINARY == type))
    {
        stuckRects3[index] = value;
        RegSetValueEx(hKey, TEXT("Settings"), 0, type, stuckRects3, cbData);
    }
    RegCloseKey(hKey); 
}

static void SetWndIcon(void)
{
    TCHAR szFilePath[60];
    HICON hIcon;

    if (!GetWindowsDirectory(szFilePath, 40))
        return;

    if (!lstrcat(szFilePath, TEXT("\\System32\\imageres.dll")))
        return;

    if (ExtractIconEx(szFilePath, 75, &hIcon, NULL, 1) <= 0)
        return;

    SendMessage(hWndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
}

static
void SetAccentButtonColor(void)
{
    SetColorButtonColor(hWndDlg, IDC_TB_ACCENTCOLOR,
        &hBmpColorBtn[COLOR_ACCENT], crSysColor[COLOR_ACCENT]);
}

static
void SetDwmButtonColor(void)
{
    SetColorButtonColor(hWndDlg, IDC_TB_DWMCOLOR,
        &hBmpColorBtn[COLOR_DWM], crSysColor[COLOR_DWM]);
}

static void InitTaskbarSettingsDlg(HWND hWnd)
{
    hWndDlg = hWnd;
    hWndTaskbar = FindWindow(TEXT("Shell_TrayWnd"), TEXT(""));

    SetWndIcon();
    InitComboboxes();

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
        SendDlgItemMessage(hWndDlg, IDC_TB_ACCENTCOLOR,
            BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)NULL);
        EnableWindow(GetDlgItem(hWndDlg, IDC_TB_ACCENTCOLOR), FALSE);
    }

    if (LoadDwmColorizationFuncs())
    {
        crSysColor[COLOR_DWM] = GetDwmColor();
        SetDwmButtonColor();
    }
    else
    {
        SendDlgItemMessage(hWndDlg, IDC_TB_DWMCOLOR, BM_SETIMAGE,
            (WPARAM)IMAGE_BITMAP, (LPARAM)NULL);

        EnableWindow(GetDlgItem(hWndDlg, IDC_TB_DWMCOLOR), FALSE);
    }
}

static void WriteDword(const TCHAR *key, const TCHAR *value, DWORD data)
{
    HKEY hKey;
    LSTATUS result = RegCreateKeyEx(
        HKEY_CURRENT_USER, key, 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);
    if (ERROR_SUCCESS != result)
        return;

    RegSetValueEx(hKey, value, 0, REG_DWORD, (BYTE *)&data, sizeof(DWORD));
    RegCloseKey(hKey);
}

static BOOL TaskbarSettingsDlgCommand(WORD control)
{
    BYTE iData;
    
#define GET_CHECKED() iData = \
    (BST_CHECKED == SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0))

#define GET_NOT_CHECKED() iData = \
    (BST_CHECKED != SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0))

    switch (control)
    {
    case IDOK:
    case IDCANCEL: /* close button and Escape key */
        EndDialog(hWndDlg, control);
        break;

    case IDC_TB_TRAYWND:
        ShellExecute(NULL, TEXT("open"), TEXT("explorer.exe"),
            TEXT("shell:::{05d7b0f4-2121-4eff-bf6b-ed3f69b894d9}"),
            NULL, SW_SHOWNORMAL);
        break;

    case IDC_TB_LOCK:
        GET_NOT_CHECKED();
        WriteDword(g_TaskbarCfgKey, TEXT("TaskbarSizeMove"), iData);
        SendMessage(hWndTaskbar, WM_USER + 458, 2, iData);
        break;

    case IDC_TB_SMBTN:
        GET_CHECKED();
        WriteDword(g_TaskbarCfgKey, TEXT("TaskbarSmallIcons"), iData);
        SendMessage(
            hWndTaskbar, WM_SETTINGCHANGE, 0L, (LPARAM)TEXT("TraySettings"));
        break;

    case IDC_TB_PEEK:
        GET_NOT_CHECKED();
        WriteDword(g_TaskbarCfgKey, TEXT("DisablePreviewDesktop"), iData);
        SendMessage(
            hWndTaskbar, WM_SETTINGCHANGE, 0L, (LPARAM)TEXT("TraySettings"));
        break;

    case IDC_TB_ALLDISPLAYS:
        GET_CHECKED();
        WriteDword(g_TaskbarCfgKey, TEXT("MMTaskbarEnabled"), iData);
        SendMessage(
            hWndTaskbar, WM_SETTINGCHANGE, 0L, (LPARAM)TEXT("TraySettings"));
        break;

    case IDC_TB_WINXPS:
        GET_NOT_CHECKED();
        /* TODO: Figure out how to set it programatically */
        WriteDword(g_TaskbarCfgKey, TEXT("DontUsePowerShellOnWinX"), iData);
        break;

#if 0
    case IDC_TB_SYSDARK:
        /* 1903 */
        GET_NOT_CHECKED();
        WriteDword(g_PersonalizeKey, TEXT("SystemUsesLightTheme"), iData);
        EnableWindow(hWndRestartExp, TRUE);
        break;

    case IDC_TB_APPDARK:
        GET_NOT_CHECKED();
        WriteDword(g_PersonalizeKey, TEXT("AppsUseLightTheme"), iData);
        EnableWindow(hWndRestartExp, TRUE);
        break;

    case IDC_TB_USECOLORTITLE:
        GET_CHECKED();
        WriteDword(TEXT("SOFTWARE\\Microsoft\\Windows\\DWM"),
            TEXT("ColorPrevalence"), iData);
        EnableWindow(hWndRestartExp, TRUE);
        break;
#endif

    case IDC_TB_TRANSP:
        GET_CHECKED();
        WriteDword(g_PersonalizeKey, TEXT("EnableTransparency"), iData);
        SendMessage(hWndTaskbar, WM_SETTINGCHANGE,
            0L, (LPARAM)TEXT("ImmersiveColorSet"));
        break;

    case IDC_TB_USEACCENTCOLOR:
        GET_CHECKED();
        WriteDword(g_PersonalizeKey, TEXT("ColorPrevalence"), iData);
        SendMessage(hWndTaskbar, WM_SETTINGCHANGE,
            0L, (LPARAM)TEXT("ImmersiveColorSet"));
        break;

    case IDC_TB_AUTOHIDE:
        GET_CHECKED();
        Write_StuckRects3(8, 2 + iData);
        SendMessage(hWndTaskbar, WM_USER + 458, 4, iData);
        break;

    case IDC_TB_ACCENTCOLOR:
        if (!PickColor(hWndDlg, &crSysColor[COLOR_ACCENT])) break;

        SetAccentColor(crSysColor[COLOR_ACCENT]);
        SetAccentButtonColor();
        /* it also sets the DWM color */
#if 0
        /* update the DWM color and button too */
        crSysColor[COLOR_DWM] = GetDwmColor();
        UpdateButtonColor(hWndDlg, IDC_TB_DWMCOLOR,
            hbmpBtnColor[COLOR_DWM], crSysColor[COLOR_DWM]);
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
    iPos = (BYTE)SendDlgItemMessage(hWndDlg, control, CB_GETCURSEL, 0, 0)

#define WRITE_COMBO_INDEX(value) \
    GET_COMBO_INDEX(); \
    WriteDword(g_TaskbarCfgKey, value, iPos)

    switch (control)
    {
    case IDC_TB_LOCATION:
        GET_COMBO_INDEX();
        Write_StuckRects3(12, iPos);
        SendMessage(hWndTaskbar, WM_USER + 458, 6, iPos);
        break;

    case IDC_TB_COMBINEBTN:
        WRITE_COMBO_INDEX(TEXT("TaskbarGlomLevel"));
        SendMessage(
            hWndTaskbar, WM_SETTINGCHANGE, 0L, (LPARAM)TEXT("TraySettings"));
        break;

    case IDC_TB_MMDISPLAYS:
        WRITE_COMBO_INDEX(TEXT("MMTaskbarMode"));
        SendMessage(
            hWndTaskbar, WM_SETTINGCHANGE, 0L, (LPARAM)TEXT("TraySettings"));
        break;

    case IDC_TB_MMCOMBINEBTN:
        WRITE_COMBO_INDEX(TEXT("MMTaskbarGlomLevel"));
        SendMessage(
            hWndTaskbar, WM_SETTINGCHANGE, 0L, (LPARAM)TEXT("TraySettings"));
        break;

    default:
        return FALSE;
    }

    return TRUE;
#undef GET_COMBO_INDEX
#undef WRITE_COMBO_INDEX
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
