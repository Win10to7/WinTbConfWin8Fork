/*
 * COPYRIGHT: See COPYING in the top level directory
 * PURPOSE:   Taskbar property page
 *
 * PROGRAMMER: Franco Tortoriello (torto09@gmail.com)
 */

#include "app.h"
#include "resource.h"

#include <CommCtrl.h>
#include <shellapi.h>

static const TCHAR g_explorerKey[] =
    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced");

static const TCHAR g_stuckRectsKey[] =
    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects3");

static const TCHAR g_dwmKey[] =
    TEXT("SOFTWARE\\Microsoft\\Windows\\DWM");

typedef struct tagTBSETTINGS
{
    DWORD bLock;
    DWORD bAutoHide;
    DWORD bSmallButtons;
    DWORD bBadges;
    DWORD bPeek;
    DWORD bAllDisplays;
    BYTE iLocation;
    DWORD iCombineButtons;
    DWORD iMmDisplays;
    DWORD iMmCombineButtons;
} TBSETTINGS;

static const TBSETTINGS g_defSettings =
{
    TRUE,   /* bLock */
    FALSE,  /* bAutoHide */
    FALSE,  /* bSmallButtons */
    FALSE,  /* bBadges */
    FALSE,  /* bPeek */
    TRUE,   /* bAllDisplays */
    3,      /* iLocation - Bottom */
    0,      /* iCombineButtons */
    0,      /* iMmDisplays */
    0       /* iMmCombineButtons */
};

static TBSETTINGS g_oldSettings;
static TBSETTINGS g_newSettings;

static HWND g_hDlg;

#define SetChecked(iControl, bChecked) \
    SendDlgItemMessage(g_hDlg, iControl, \
        BM_SETCHECK, (WPARAM)(bChecked == TRUE), 0L)

#define SetComboIndex(iControl, index) \
    SendDlgItemMessage(g_hDlg, iControl, CB_SETCURSEL, (WPARAM)index, 0L)

static
void InitComboBoxes(void)
{
    int iElement;
    TCHAR text[60];

#define InitCombo(iControl, iString, nElements) \
    for (iElement = 0; iElement < nElements; iElement++) { \
        LoadString(g_propSheet.hInstance, iString + iElement, \
            (TCHAR *)&text, 59); \
        SendDlgItemMessage(g_hDlg, iControl, CB_ADDSTRING, 0L, (LPARAM)&text); \
    }

    InitCombo(IDC_TB_LOCATION, IDS_TB_POS_L, 4);
    InitCombo(IDC_TB_COMBINEBUTTONS, IDS_TB_COMB_YES, 3);
    InitCombo(IDC_TB_MMDISPLAYS, IDS_TB_MMALL, 3);
    InitCombo(IDC_TB_MMCOMBINEBUTTONS, IDS_TB_COMB_YES, 3);

#undef InitCombo
}

static
void LoadExplorerSettings(void)
{
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

#define ReadDwordMember(valueName, member) \
    ReadDword(valueName, g_oldSettings.member, g_defSettings.member)

#define ReadInvertedMember(valueName, member) \
    ReadDword(valueName, g_oldSettings.member, !g_defSettings.member); \
    g_oldSettings.member = !g_oldSettings.member;

    ReadInvertedMember(TEXT("TaskbarSizeMove"), bLock);
    ReadDwordMember(TEXT("TaskbarSmallIcons"), bSmallButtons);
    ReadDwordMember(TEXT("TaskbarBadges"), bBadges);
    ReadDwordMember(TEXT("TaskbarGlomLevel"), iCombineButtons);
    ReadInvertedMember(TEXT("DisablePreviewDesktop"), bPeek);
    ReadDwordMember(TEXT("MMTaskbarEnabled"), bAllDisplays);
    ReadDwordMember(TEXT("MMTaskbarMode"), iMmDisplays);
    ReadDwordMember(TEXT("MMTaskbarGlomLevel"), iMmCombineButtons);

#undef ReadInvertedMember
#undef ReadDwordMember
#undef ReadDword

    RegCloseKey(hKey);
}

static
void LoadStuckRectsSettings(void)
{
    HKEY hKey;
    LSTATUS status = RegOpenKeyEx(
        HKEY_CURRENT_USER, g_stuckRectsKey, 0, KEY_QUERY_VALUE, &hKey);
    if (status != ERROR_SUCCESS)
        return;

    DWORD dwType;
    DWORD cbData = 48;
    BYTE stuckRects3[48];

    status = RegQueryValueEx(
        hKey, TEXT("Settings"), 0, &dwType, stuckRects3, &cbData);

    if (status == ERROR_SUCCESS && dwType == REG_BINARY)
    {
        g_oldSettings.bAutoHide = (stuckRects3[8] == 3);
        g_oldSettings.iLocation = stuckRects3[12];
    }
    else
    {
        g_oldSettings.bAutoHide = g_defSettings.bAutoHide;
        g_oldSettings.iLocation = g_defSettings.iLocation;
    }

    RegCloseKey(hKey);
}

static
void LoadDwmSettings(void)
{
    HKEY hKey;
    LSTATUS status = RegOpenKeyEx(
        HKEY_CURRENT_USER, g_dwmKey, 0, KEY_QUERY_VALUE, &hKey);
    if (status != ERROR_SUCCESS)
        return;

    DWORD dwType;
    DWORD bData;
    DWORD cbData = sizeof(DWORD);

    status = RegQueryValueEx(
        hKey, TEXT("EnableAeroPeek"), 0, &dwType, (BYTE *)&bData, &cbData);
    if (status == ERROR_SUCCESS && dwType == REG_DWORD)
        g_oldSettings.bPeek &= bData;

    RegCloseKey(hKey);
}

static
void LoadSettings()
{
    g_oldSettings = g_defSettings;

    LoadExplorerSettings();
    LoadStuckRectsSettings();
    if (g_oldSettings.bPeek)
        LoadDwmSettings();

    g_newSettings = g_oldSettings;
}

static
void UpdateExplorerControls(void)
{
    SetChecked(IDC_TB_LOCK, g_oldSettings.bLock);
    SetChecked(IDC_TB_SMALLBUTTONS, g_oldSettings.bSmallButtons);
    SetChecked(IDC_TB_BADGES, g_oldSettings.bBadges);
    SetComboIndex(IDC_TB_COMBINEBUTTONS, g_oldSettings.iCombineButtons);
    SetChecked(IDC_TB_PEEK, g_oldSettings.bPeek);
    SetChecked(IDC_TB_ALLDISPLAYS, g_oldSettings.bAllDisplays);
    SetComboIndex(IDC_TB_MMDISPLAYS, g_oldSettings.iMmDisplays);
    SetComboIndex(IDC_TB_MMCOMBINEBUTTONS, g_oldSettings.iMmCombineButtons);

    EnableWindow(GetDlgItem(g_hDlg, IDC_TB_BADGES),
        !g_oldSettings.bSmallButtons);
}

static
void UpdateStuckRectsControls(void)
{
    SetChecked(IDC_TB_AUTOHIDE, g_oldSettings.bAutoHide);
    SetComboIndex(IDC_TB_LOCATION, g_oldSettings.iLocation);
}

static
void UpdateControls(void)
{
    UpdateExplorerControls();
    UpdateStuckRectsControls();
}

static
void InitPage(void)
{
    InitComboBoxes();

    LoadSettings();
    UpdateControls();
}

_Success_(return)
static
BOOL WriteStuckRects3(void)
{
    HKEY hKey;
    LSTATUS status = RegOpenKeyEx(HKEY_CURRENT_USER, g_stuckRectsKey, 0,
        KEY_QUERY_VALUE | KEY_SET_VALUE, &hKey);
    if (status != ERROR_SUCCESS)
        goto Error;

    BYTE stuckRects3[48];
    DWORD dwType;
    DWORD cbData = 48;

    status = RegQueryValueEx(
        hKey, TEXT("Settings"), 0, &dwType, stuckRects3, &cbData);
    if (status != ERROR_SUCCESS || dwType != REG_BINARY)
    {
        RegCloseKey(hKey);
        goto Error;
    }

    stuckRects3[8] = g_newSettings.bAutoHide ? 3 : 2;
    stuckRects3[12] = g_newSettings.iLocation;
    status = RegSetValueEx(
        hKey, TEXT("Settings"), 0, dwType, stuckRects3, cbData);

    RegCloseKey(hKey);
    if (status != ERROR_SUCCESS)
        goto Error;

    return TRUE;

Error:
    g_newSettings.bAutoHide = g_oldSettings.bAutoHide;
    g_newSettings.iLocation = g_oldSettings.iLocation;
    return FALSE;
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
        RestoreSetting(bLock);
        RestoreSetting(bSmallButtons);
        RestoreSetting(bBadges);
        RestoreSetting(iCombineButtons);
        RestoreSetting(bPeek);
        RestoreSetting(bAllDisplays);
        RestoreSetting(iMmDisplays);
        RestoreSetting(iMmCombineButtons);
        return FALSE;
    }

    BOOL ret = TRUE;
    DWORD bValue;

#define SetDword(value, data) \
    status = RegSetValueEx( \
        hKey, value, 0, REG_DWORD, (BYTE *)&data, sizeof(DWORD))

#define UpdateDword(value, member) \
    if (HasChanged(member)) { \
        SetDword(value, g_newSettings.member); \
        if (status != ERROR_SUCCESS) { \
            RestoreSetting(member); \
            ret = FALSE; \
        } \
    }

#define UpdateDwordInverted(value, member) \
    if (HasChanged(member)) { \
        bValue = !g_newSettings.member; \
        SetDword(value, bValue); \
        if (status != ERROR_SUCCESS) { \
            RestoreSetting(member); \
            ret = FALSE; \
        } \
    }

    UpdateDwordInverted(TEXT("TaskbarSizeMove"), bLock);
    UpdateDword(TEXT("TaskbarSmallIcons"), bSmallButtons);
    UpdateDword(TEXT("TaskbarBadges"), bBadges);
    UpdateDword(TEXT("TaskbarGlomLevel"), iCombineButtons);
    UpdateDwordInverted(TEXT("DisablePreviewDesktop"), bPeek);
    UpdateDword(TEXT("MMTaskbarEnabled"), bAllDisplays);
    UpdateDword(TEXT("MMTaskbarMode"), iMmDisplays);
    UpdateDword(TEXT("MMTaskbarGlomLevel"), iMmCombineButtons);

#undef UpdateDwordInverted
#undef UpdateDword
#undef SetDword

    RegCloseKey(hKey);

    return ret;

#undef RestoreSetting
}

static
void WriteDwmSettings(void)
{
    HKEY hKey;
    LSTATUS status = RegCreateKeyEx(HKEY_CURRENT_USER, g_dwmKey, 0, NULL, 0,
        KEY_SET_VALUE, NULL, &hKey, NULL);
    if (status != ERROR_SUCCESS)
        return;

    RegSetValueEx(hKey, TEXT("EnableAeroPeek"), 0, REG_DWORD,
        (BYTE *)&g_newSettings.bPeek, sizeof(DWORD));

    RegCloseKey(hKey);
}

static
void ApplyExplorerSettings(void)
{
    BOOL bSendSettingChange = HasChanged(bSmallButtons) ||
        HasChanged(bBadges) || HasChanged(iCombineButtons) ||
        HasChanged(bPeek) || HasChanged(bAllDisplays) ||
        HasChanged(iMmDisplays) || HasChanged(iMmCombineButtons);

    /* Apply them always, in case the Lock setting was changed externally. */
#if 0
    BOOL bExplorerSettingsChanged = bSendSettingChange || HasChanged(bLock);

    if (!bExplorerSettingsChanged)
        return;
#endif

    if (!WriteExplorerSettings())
    {
        /* An error has ocurred, show previous settings */
        UpdateExplorerControls();
        return;
    }

    if (HasChanged(bPeek) && g_newSettings.bPeek)
    {
        /* Always enable the "Visual Effects" Peek setting, as I think it makes
         * it less confusing.
         */
        WriteDwmSettings();
    }

    if (bSendSettingChange)
    {
        SendNotifyMessage(
            HWND_BROADCAST, WM_SETTINGCHANGE, 0L, (LPARAM)TEXT("TraySettings"));
    }

    if (HasChanged(bLock))
    {
        HWND hTaskbar = FindWindow(TEXT("Shell_TrayWnd"), TEXT(""));
        if (hTaskbar)
        {
            SendNotifyMessage(
                hTaskbar, WM_USER + 458, 2, (LPARAM)!g_newSettings.bLock);
        }
    }
}

static
void ApplyStuckRectsSettings(void)
{
    BOOL bStuckRectsSettingsChanged =
        HasChanged(bAutoHide) || HasChanged(iLocation);

    if (!bStuckRectsSettingsChanged)
        return;

    if (!WriteStuckRects3())
    {
        UpdateStuckRectsControls();
        return;
    }

    HWND hTaskbar = FindWindow(TEXT("Shell_TrayWnd"), TEXT(""));
    if (!hTaskbar)
        return;

    if (HasChanged(bAutoHide))
        SendNotifyMessage(hTaskbar, WM_USER + 458, 4, g_newSettings.bAutoHide);

    if (HasChanged(iLocation))
        SendNotifyMessage(hTaskbar, WM_USER + 458, 6, g_newSettings.iLocation);
}

static
void ApplySettings(void)
{
    ApplyExplorerSettings();
    ApplyStuckRectsSettings();
    g_oldSettings = g_newSettings;
}

#undef HasChanged

static
void HandleCommand(WORD iControl)
{
#define GetChecked() \
    (SendDlgItemMessage(g_hDlg, iControl, BM_GETCHECK, 0L, 0L) == BST_CHECKED)

    switch (iControl)
    {
    case IDC_TB_TRAYWND:
        ShellExecute(NULL, TEXT("open"), TEXT("explorer.exe"),
            TEXT("shell:::{05d7b0f4-2121-4eff-bf6b-ed3f69b894d9}"),
            NULL, SW_SHOWNORMAL);
        return;

    case IDC_TB_LOCK:
        g_newSettings.bLock = GetChecked();
        break;

    case IDC_TB_AUTOHIDE:
        g_newSettings.bAutoHide = GetChecked();
        break;

    case IDC_TB_SMALLBUTTONS:
        g_newSettings.bSmallButtons = GetChecked();
        EnableWindow(
            GetDlgItem(g_hDlg, IDC_TB_BADGES), !g_newSettings.bSmallButtons);
        break;

    case IDC_TB_BADGES:
        g_newSettings.bBadges = GetChecked();
        break;

    case IDC_TB_PEEK:
        g_newSettings.bPeek = GetChecked();
        break;

    case IDC_TB_ALLDISPLAYS:
        g_newSettings.bAllDisplays = GetChecked();
        break;

    default:
        return;
    }

    PropSheet_Changed(g_propSheet.hWnd, g_hDlg);

#undef GetChecked
}

static
void HandleComboBoxSelChange(WORD iControl)
{
#define GetComboIndex() \
    (BYTE)SendDlgItemMessage(g_hDlg, iControl, CB_GETCURSEL, 0L, 0L)

    switch (iControl)
    {
    case IDC_TB_LOCATION:
        g_newSettings.iLocation = GetComboIndex();
        break;

    case IDC_TB_COMBINEBUTTONS:
        g_newSettings.iCombineButtons = GetComboIndex();
        break;

    case IDC_TB_MMDISPLAYS:
        g_newSettings.iMmDisplays = GetComboIndex();
        break;

    case IDC_TB_MMCOMBINEBUTTONS:
        g_newSettings.iMmCombineButtons = GetComboIndex();
        break;

    default:
        return;
    }

    PropSheet_Changed(g_propSheet.hWnd, g_hDlg);

#undef GetComboIndex
}

INT_PTR CALLBACK TaskbarPageProc(
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

        case CBN_SELCHANGE:
            HandleComboBoxSelChange(LOWORD(wParam));
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
        }

        return 0;

    case WM_SETTINGCHANGE:
        if (lstrcmpi((TCHAR *)lParam, TEXT("TraySettings")) == 0)
        {
            LoadExplorerSettings();
            break;
        }
        break;

    case WM_QUERYENDSESSION:
        /* Contrary to what the documentation says, without this shutdown is
         * prevented, at least from W95 to 7...
         */
        return 0;

    case WM_CTLCOLORDLG:
        /* Returning any other value paints the property sheet pages with an
         * incorrect background color.
         */
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
