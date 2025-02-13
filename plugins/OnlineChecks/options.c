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

VOID ShowHybridAnalysisConfigDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PVOID Parameter
    );

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
            Button_SetCheck(GetDlgItem(WindowHandle, IDC_ENABLE_VIRUSTOTAL),
                PhGetIntegerSetting(SETTING_NAME_VIRUSTOTAL_SCAN_ENABLED) ? BST_CHECKED : BST_UNCHECKED);
            //Button_SetCheck(GetDlgItem(WindowHandle, IDC_ENABLE_IDC_ENABLE_VIRUSTOTAL_HIGHLIGHT),
            //    PhGetIntegerSetting(SETTING_NAME_VIRUSTOTAL_HIGHLIGHT_DETECTIONS) ? BST_CHECKED : BST_UNCHECKED);

            PhSetDialogItemText(WindowHandle, IDC_HYBRIDTEXT, PhGetStringOrEmpty(PhaGetStringSetting(SETTING_NAME_HYBRIDANAL_DEFAULT_PAT)));
            PhSetDialogItemText(WindowHandle, IDC_VTEXT, PhGetStringOrEmpty(PhaGetStringSetting(SETTING_NAME_VIRUSTOTAL_DEFAULT_PAT)));
        }
        break;
    case WM_DESTROY:
        {
            PhSetIntegerSetting(SETTING_NAME_VIRUSTOTAL_SCAN_ENABLED,
                Button_GetCheck(GetDlgItem(WindowHandle, IDC_ENABLE_VIRUSTOTAL)) == BST_CHECKED ? 1 : 0);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_APIKEYIDBTN:
                {
                    PPH_STRING string;

                    ShowHybridAnalysisConfigDialog(WindowHandle, UlongToPtr(IDC_APIKEYIDBTN));

                    string = PhaGetStringSetting(SETTING_NAME_HYBRIDANAL_DEFAULT_PAT);
                    PhSetDialogItemText(WindowHandle, IDC_HYBRIDTEXT, PhGetStringOrEmpty(string));
                }
                break;
            case IDC_VTKEYIDBTN:
                {
                    PPH_STRING string;

                    ShowHybridAnalysisConfigDialog(WindowHandle, UlongToPtr(IDC_VTKEYIDBTN));

                    string = PhaGetStringSetting(SETTING_NAME_VIRUSTOTAL_DEFAULT_PAT);
                    PhSetDialogItemText(WindowHandle, IDC_VTEXT, PhGetStringOrEmpty(string));
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
                PhSetDialogItemText(WindowHandle, IDC_KEY_EDIT, PhaGetStringSetting(SETTING_NAME_HYBRIDANAL_DEFAULT_PAT)->Buffer);
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
                    PhSetStringSetting(id == IDC_APIKEYIDBTN ? SETTING_NAME_HYBRIDANAL_DEFAULT_PAT : SETTING_NAME_VIRUSTOTAL_DEFAULT_PAT, PhGetStringOrEmpty(string));
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
