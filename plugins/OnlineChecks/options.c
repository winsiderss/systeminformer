/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2016
 *
 */

#include "onlnchk.h"

BOOLEAN OnlineChecksPreEnableUi = FALSE;
static BOOLEAN OnlineChecksRestartRequired = FALSE;
static BOOLEAN OptionsHybridKeyConfigured = FALSE;
static BOOLEAN OptionsVirusTotalKeyConfigured = FALSE;
static PPH_STRING OptionsInitialHybridPat = NULL;
static PPH_STRING OptionsInitialVirusTotalPat = NULL;
static PH_LAYOUT_MANAGER OptionsLayoutManager;

static VOID OptionsRefreshKeyStatus(
    _In_ HWND WindowHandle,
    _In_ INT LabelId,
    _In_ PCWSTR SettingName,
    _Out_ PBOOLEAN Configured
    )
{
    PPH_STRING key = PhGetStringSetting(SettingName);

    *Configured = !PhIsNullOrEmptyString(key);
    PhDereferenceObject(key);

    PhSetDialogItemText(WindowHandle, LabelId,
        *Configured ? L"Set - using your key" : L"Unset - optional");
    InvalidateRect(GetDlgItem(WindowHandle, LabelId), NULL, TRUE);
}

VOID ShowHybridAnalysisConfigDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PVOID Parameter
    );

static VOID OptionsSaveCheckRestart(
    _In_ HWND WindowHandle,
    _In_ INT ControlId,
    _In_ PCWSTR SettingName
    )
{
    BOOLEAN oldValue = !!PhGetIntegerSetting(SettingName);
    BOOLEAN newValue = Button_GetCheck(GetDlgItem(WindowHandle, ControlId)) == BST_CHECKED;

    if (newValue != oldValue)
        OnlineChecksRestartRequired = TRUE;

    PhSetIntegerSetting(SettingName, newValue);
}

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            BOOLEAN preEnable = OnlineChecksPreEnableUi;

            OnlineChecksPreEnableUi = FALSE;

            Button_SetCheck(GetDlgItem(WindowHandle, IDC_ENABLE_SCANNING),
                (preEnable || PhGetIntegerSetting(SETTING_NAME_SCAN_ENABLED)) ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(WindowHandle, IDC_HA_LOOKUPS),
                (preEnable || PhGetIntegerSetting(SETTING_NAME_HYBRIDANALYSIS_LOOKUPS_ENABLED)) ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(WindowHandle, IDC_HA_SUBMIT),
                (preEnable || PhGetIntegerSetting(SETTING_NAME_HYBRIDANALYSIS_SUBMIT_ENABLED)) ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(WindowHandle, IDC_VT_LOOKUPS),
                PhGetIntegerSetting(SETTING_NAME_VIRUSTOTAL_LOOKUPS_ENABLED) ? BST_CHECKED : BST_UNCHECKED);

            SetDlgItemInt(WindowHandle, IDC_SCAN_DELAY, PhGetIntegerSetting(SETTING_NAME_SCAN_STARTUP_DELAY), FALSE);

            PhMoveReference(&OptionsInitialHybridPat, PhGetStringSetting(SETTING_NAME_HYBRIDANALYSIS_DEFAULT_PAT));
            PhMoveReference(&OptionsInitialVirusTotalPat, PhGetStringSetting(SETTING_NAME_VIRUSTOTAL_DEFAULT_PAT));

            OptionsRefreshKeyStatus(WindowHandle, IDC_HA_KEY_STATUS, SETTING_NAME_HYBRIDANALYSIS_DEFAULT_PAT, &OptionsHybridKeyConfigured);
            OptionsRefreshKeyStatus(WindowHandle, IDC_VT_KEY_STATUS, SETTING_NAME_VIRUSTOTAL_DEFAULT_PAT, &OptionsVirusTotalKeyConfigured);

            ScanExclusionsPopulateListBox(GetDlgItem(WindowHandle, IDC_EXCLUDE_LIST));

            PhInitializeLayoutManager(&OptionsLayoutManager, WindowHandle);
            PhAddLayoutItem(&OptionsLayoutManager, GetDlgItem(WindowHandle, IDC_ENABLE_SCANNING), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP);
            PhAddLayoutItem(&OptionsLayoutManager, GetDlgItem(WindowHandle, IDC_SCAN_DELAY_LABEL), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&OptionsLayoutManager, GetDlgItem(WindowHandle, IDC_SCAN_DELAY), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&OptionsLayoutManager, GetDlgItem(WindowHandle, IDC_HA_GROUP), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&OptionsLayoutManager, GetDlgItem(WindowHandle, IDC_APIKEYIDBTN), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP);
            PhAddLayoutItem(&OptionsLayoutManager, GetDlgItem(WindowHandle, IDC_VT_GROUP), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&OptionsLayoutManager, GetDlgItem(WindowHandle, IDC_VTKEYIDBTN), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP);
            PhAddLayoutItem(&OptionsLayoutManager, GetDlgItem(WindowHandle, IDC_EXCLUDE_GROUP), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&OptionsLayoutManager, GetDlgItem(WindowHandle, IDC_EXCLUDE_LIST), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&OptionsLayoutManager, GetDlgItem(WindowHandle, IDC_EXCLUDE_REMOVE), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&OptionsLayoutManager, GetDlgItem(WindowHandle, IDC_EXCLUDE_TEXT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&OptionsLayoutManager, GetDlgItem(WindowHandle, IDC_EXCLUDE_ADD), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
        }
        break;
    case WM_DESTROY:
        {
            PPH_STRING hybridPat;
            PPH_STRING virusTotalPat;

            OptionsSaveCheckRestart(WindowHandle, IDC_ENABLE_SCANNING, SETTING_NAME_SCAN_ENABLED);
            OptionsSaveCheckRestart(WindowHandle, IDC_HA_LOOKUPS, SETTING_NAME_HYBRIDANALYSIS_LOOKUPS_ENABLED);
            OptionsSaveCheckRestart(WindowHandle, IDC_HA_SUBMIT, SETTING_NAME_HYBRIDANALYSIS_SUBMIT_ENABLED);
            OptionsSaveCheckRestart(WindowHandle, IDC_VT_LOOKUPS, SETTING_NAME_VIRUSTOTAL_LOOKUPS_ENABLED);

            PhSetIntegerSetting(SETTING_NAME_SCAN_STARTUP_DELAY, GetDlgItemInt(WindowHandle, IDC_SCAN_DELAY, NULL, FALSE));

            hybridPat = PhGetStringSetting(SETTING_NAME_HYBRIDANALYSIS_DEFAULT_PAT);
            virusTotalPat = PhGetStringSetting(SETTING_NAME_VIRUSTOTAL_DEFAULT_PAT);

            if (!PhEqualString(hybridPat, OptionsInitialHybridPat, FALSE) ||
                !PhEqualString(virusTotalPat, OptionsInitialVirusTotalPat, FALSE))
            {
                OnlineChecksRestartRequired = TRUE;
            }

            PhDereferenceObject(hybridPat);
            PhDereferenceObject(virusTotalPat);
            PhClearReference(&OptionsInitialHybridPat);
            PhClearReference(&OptionsInitialVirusTotalPat);

            ScanExclusionsSaveListBox(GetDlgItem(WindowHandle, IDC_EXCLUDE_LIST));

            PhDeleteLayoutManager(&OptionsLayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_APIKEYIDBTN:
                ShowHybridAnalysisConfigDialog(WindowHandle, UlongToPtr(IDC_APIKEYIDBTN));
                OptionsRefreshKeyStatus(WindowHandle, IDC_HA_KEY_STATUS, SETTING_NAME_HYBRIDANALYSIS_DEFAULT_PAT, &OptionsHybridKeyConfigured);
                break;
            case IDC_VTKEYIDBTN:
                ShowHybridAnalysisConfigDialog(WindowHandle, UlongToPtr(IDC_VTKEYIDBTN));
                OptionsRefreshKeyStatus(WindowHandle, IDC_VT_KEY_STATUS, SETTING_NAME_VIRUSTOTAL_DEFAULT_PAT, &OptionsVirusTotalKeyConfigured);
                break;
            case IDC_EXCLUDE_ADD:
                ScanExclusionsAddFromEdit(WindowHandle, GetDlgItem(WindowHandle, IDC_EXCLUDE_LIST), GetDlgItem(WindowHandle, IDC_EXCLUDE_TEXT));
                break;
            case IDC_EXCLUDE_REMOVE:
                {
                    HWND listBoxHandle = GetDlgItem(WindowHandle, IDC_EXCLUDE_LIST);
                    INT index = ListBox_GetCurSel(listBoxHandle);

                    if (index != LB_ERR)
                        ListBox_DeleteString(listBoxHandle, index);
                }
                break;
            }
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&OptionsLayoutManager);
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        {
            HWND control = (HWND)lParam;

            if (control == GetDlgItem(WindowHandle, IDC_HA_KEY_STATUS) ||
                control == GetDlgItem(WindowHandle, IDC_VT_KEY_STATUS))
            {
                HDC hdc = (HDC)wParam;
                BOOLEAN configured = control == GetDlgItem(WindowHandle, IDC_HA_KEY_STATUS)
                    ? OptionsHybridKeyConfigured : OptionsVirusTotalKeyConfigured;
                HBRUSH brush = PhWindowThemeControlColor(WindowHandle, hdc, control, CTLCOLOR_STATIC);

                SetTextColor(hdc, configured ? RGB(0, 153, 51) : RGB(128, 128, 128));
                SetBkMode(hdc, TRANSPARENT);
                return (INT_PTR)brush;
            }

            return HANDLE_WM_CTLCOLORSTATIC(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
        }
    }

    return FALSE;
}

INT_PTR CALLBACK OptionsGeoLiteDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            ULONG id = PtrToUlong((PVOID)lParam);

            PhSetDialogContext(WindowHandle, UlongToPtr(id));

            PhCenterWindow(WindowHandle, GetParent(WindowHandle));

            if (id == IDC_APIKEYIDBTN)
            {
                PhSetDialogItemText(WindowHandle, IDC_KEYTEXT_L, L"Paste the license key here:");
                PhSetDialogItemText(WindowHandle, IDC_KEY_EDIT, PhaGetStringSetting(SETTING_NAME_HYBRIDANALYSIS_DEFAULT_PAT)->Buffer);
            }
            else
            {
                PhSetDialogItemText(WindowHandle, IDC_KEYTEXT_L, L"Paste the license key here:");
                PhSetDialogItemText(WindowHandle, IDC_KEY_EDIT, PhaGetStringSetting(SETTING_NAME_VIRUSTOTAL_DEFAULT_PAT)->Buffer);
            }

            PhSetDialogFocus(WindowHandle, GetDlgItem(WindowHandle, IDC_KEY_EDIT));

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveDialogContext(WindowHandle);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(WindowHandle, IDCANCEL);
                break;
            case IDYES:
                {
                    ULONG id = PtrToUlong(PhGetDialogContext(WindowHandle));
                    PPH_STRING string;

                    string = PhGetWindowText(GetDlgItem(WindowHandle, IDC_KEY_EDIT));
                    PhSetStringSetting(id == IDC_APIKEYIDBTN ? SETTING_NAME_HYBRIDANALYSIS_DEFAULT_PAT : SETTING_NAME_VIRUSTOTAL_DEFAULT_PAT, PhGetStringOrEmpty(string));
                    PhClearReference(&string);

                    EndDialog(WindowHandle, IDOK);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case NM_CLICK:
            case NM_RETURN:
                {
                    PNMLINK link = (PNMLINK)lParam;
                    LITEM item = link->item;

                    if (header->idFrom == IDC_HELPLINK && item.iLink == 0)
                    {
                        ULONG id = PtrToUlong(PhGetDialogContext(WindowHandle));

                        if (id == IDC_APIKEYIDBTN)
                        {
                            PhShellExecute(WindowHandle, L"https://hybrid-analysis.com/docs/api/v2", NULL);
                        }
                        else
                        {
                            PhShellExecute(WindowHandle, L"https://docs.virustotal.com/docs/please-give-me-an-api-key", NULL);
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(WindowHandle, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

VOID ShowHybridAnalysisConfigDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PVOID Parameter
    )
{
    PhDialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONSKEY),
        ParentWindowHandle,
        OptionsGeoLiteDlgProc,
        Parameter
        );
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI OptionsSettingsUpdatedCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    ScanLoadExclusions();

    if (OnlineChecksRestartRequired)
    {
        *(PBOOLEAN)Parameter = TRUE;
        OnlineChecksRestartRequired = FALSE;
    }
}
