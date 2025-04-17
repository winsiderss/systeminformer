/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2012-2024
 *
 */

#include "nettools.h"

CONST PPH_STRINGREF OptionsGeoLiteEdition[2] =
{
    SREF(L"GeoLite Country (Small)"),
    SREF(L"GeoLite City (Large)"),
};

VOID ShowGeoLiteConfigDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PVOID Parameter
    );

VOID GeoLiteUpdateFromConfigFile(
    _In_ PPH_STRING FileName
    );

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    static PH_LAYOUT_MANAGER LayoutManager;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            HWND comboHandle = GetDlgItem(WindowHandle, IDC_GEODBCOMBO);

            PhSetDialogItemValue(WindowHandle, IDC_PINGPACKETLENGTH, PhGetIntegerSetting(SETTING_NAME_PING_SIZE), FALSE);
            PhSetDialogItemValue(WindowHandle, IDC_MAXHOPS, PhGetIntegerSetting(SETTING_NAME_TRACERT_MAX_HOPS), FALSE);
            Button_SetCheck(GetDlgItem(WindowHandle, IDC_ENABLE_EXTENDED_TCP), PhGetIntegerSetting(SETTING_NAME_EXTENDED_TCP_STATS) ? BST_CHECKED : BST_UNCHECKED);

            PhSetDialogItemText(WindowHandle, IDC_KEYTEXT, PhGetStringOrEmpty(PhaGetStringSetting(SETTING_NAME_GEOLITE_API_KEY)));
            PhSetDialogItemText(WindowHandle, IDC_GEOIDTEXT, PhGetStringOrEmpty(PhaGetStringSetting(SETTING_NAME_GEOLITE_API_ID)));

            PhAddComboBoxStringRefs(comboHandle, OptionsGeoLiteEdition, RTL_NUMBER_OF(OptionsGeoLiteEdition));
            ComboBox_SetCurSel(comboHandle, PhGetIntegerSetting(SETTING_NAME_GEOLITE_DB_TYPE));

            PhInitializeLayoutManager(&LayoutManager, WindowHandle);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(WindowHandle, IDC_DATABASE), NULL, PH_ANCHOR_TOP | PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(WindowHandle, IDC_BROWSE), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
        }
        break;
    case WM_DESTROY:
        {
            GeoLiteDatabaseType = ComboBox_GetCurSel(GetDlgItem(WindowHandle, IDC_GEODBCOMBO));

            PhSetIntegerSetting(SETTING_NAME_PING_SIZE, PhGetDialogItemValue(WindowHandle, IDC_PINGPACKETLENGTH));
            PhSetIntegerSetting(SETTING_NAME_TRACERT_MAX_HOPS, PhGetDialogItemValue(WindowHandle, IDC_MAXHOPS));
            PhSetIntegerSetting(SETTING_NAME_EXTENDED_TCP_STATS, Button_GetCheck(GetDlgItem(WindowHandle, IDC_ENABLE_EXTENDED_TCP)) == BST_CHECKED);
            PhSetIntegerSetting(SETTING_NAME_GEOLITE_DB_TYPE, GeoLiteDatabaseType);

            PhDeleteLayoutManager(&LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_APIKEYBTN:
                {
                    PPH_STRING string;

                    if (!GeoLiteCheckUpdatePlatformSupported())
                        break;

                    ShowGeoLiteConfigDialog(WindowHandle, UlongToPtr(IDC_KEYTEXT));

                    string = PhaGetStringSetting(SETTING_NAME_GEOLITE_API_KEY);
                    PhSetDialogItemText(WindowHandle, IDC_KEYTEXT, PhGetStringOrEmpty(string));
                }
                break;
            case IDC_APIKEYIDBTN:
                {
                    PPH_STRING string;

                    if (!GeoLiteCheckUpdatePlatformSupported())
                        break;

                    ShowGeoLiteConfigDialog(WindowHandle, UlongToPtr(IDC_GEOIDTEXT));

                    string = PhaGetStringSetting(SETTING_NAME_GEOLITE_API_ID);
                    PhSetDialogItemText(WindowHandle, IDC_GEOIDTEXT, PhGetStringOrEmpty(string));
                }
                break;
            case IDRETRY:
                {
                    ShowGeoLiteUpdateDialog(WindowHandle);
                }
                break;
            case IDC_GEODBCOMBO:
                {
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                    case CBN_SELCHANGE:
                        {
                            GeoLiteDatabaseType = ComboBox_GetCurSel(GetDlgItem(WindowHandle, IDC_GEODBCOMBO));

                            PhSetIntegerSetting(SETTING_NAME_GEOLITE_DB_TYPE, GeoLiteDatabaseType);
                        }
                        break;
                    }
                }
                break;
            case IDC_GEOCONF:
                {
                    static PH_FILETYPE_FILTER filters[] =
                    {
                        { L"GeoIP.conf files (*.conf)", L"*.conf" },
                        { L"All files (*.*)", L"*.*" }
                    };
                    PVOID fileDialog;
                    PPH_STRING fileName = NULL;
                    PPH_STRING string;

                    fileDialog = PhCreateOpenFileDialog();
                    PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));

                    if (PhShowFileDialog(WindowHandle, fileDialog))
                    {
                        fileName = PH_AUTO(PhGetFileDialogFileName(fileDialog));
                    }

                    PhFreeFileDialog(fileDialog);

                    if (!PhIsNullOrEmptyString(fileName))
                    {
                        GeoLiteUpdateFromConfigFile(fileName);
                    }

                    string = PhaGetStringSetting(SETTING_NAME_GEOLITE_API_ID);
                    PhSetDialogItemText(WindowHandle, IDC_GEOIDTEXT, PhGetStringOrEmpty(string));
                    string = PhaGetStringSetting(SETTING_NAME_GEOLITE_API_KEY);
                    PhSetDialogItemText(WindowHandle, IDC_KEYTEXT, PhGetStringOrEmpty(string));
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

            PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, UlongToPtr(id));

            PhCenterWindow(WindowHandle, GetParent(WindowHandle));

            if (id == IDC_KEYTEXT)
            {
                PhSetDialogItemText(WindowHandle, IDC_KEYTEXT_L, L"Paste the license key here:");
                PhSetDialogItemText(WindowHandle, IDC_KEY_EDIT, PhaGetStringSetting(SETTING_NAME_GEOLITE_API_KEY)->Buffer);
            }
            else
            {
                PhSetDialogItemText(WindowHandle, IDC_KEYTEXT_L, L"Paste the account id here:");
                PhSetDialogItemText(WindowHandle, IDC_KEY_EDIT, PhaGetStringSetting(SETTING_NAME_GEOLITE_API_ID)->Buffer);
            }

            PhSetDialogFocus(WindowHandle, GetDlgItem(WindowHandle, IDC_KEY_EDIT));

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
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
                    ULONG id = PtrToUlong(PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT));
                    PPH_STRING string;

                    string = PhGetWindowText(GetDlgItem(WindowHandle, IDC_KEY_EDIT));
                    PhSetStringSetting(id == IDC_KEYTEXT ? SETTING_NAME_GEOLITE_API_KEY : SETTING_NAME_GEOLITE_API_ID, PhGetStringOrEmpty(string));
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
                        PhShellExecute(WindowHandle, item.szUrl, NULL);
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

VOID GeoLiteUpdateFromConfigFile(
    _In_ PPH_STRING FileName
    )
{
    static PH_STRINGREF skipFileLine = PH_STRINGREF_INIT(L"\n");
    PPH_STRING content;
    PH_STRINGREF firstPart;
    PH_STRINGREF remainingPart;
    PH_STRINGREF namePart;
    PH_STRINGREF valuePart;
    PPH_STRING accountString = NULL;
    PPH_STRING licenseString = NULL;

    if (!NT_SUCCESS(PhFileReadAllTextWin32(&content, PhGetString(FileName), TRUE)))
        return;

    if (PhIsNullOrEmptyString(content))
        return;

    remainingPart = PhGetStringRef(content);

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtString(&remainingPart, &skipFileLine, TRUE, &firstPart, &remainingPart);

        if (firstPart.Length != 0)
        {
            if (PhSplitStringRefAtChar(&firstPart, L' ', &namePart, &valuePart))
            {
                if (PhEqualStringRef2(&namePart, L"AccountID", TRUE))
                {
                    accountString = PhCreateString2(&valuePart);
                }

                if (PhEqualStringRef2(&namePart, L"LicenseKey", TRUE))
                {
                    licenseString = PhCreateString2(&valuePart);
                }

                if (
                    !PhIsNullOrEmptyString(accountString) &&
                    !PhIsNullOrEmptyString(licenseString)
                    )
                {
                    break;
                }
            }
        }
    }

    if (
        !PhIsNullOrEmptyString(accountString) &&
        !PhIsNullOrEmptyString(licenseString)
        )
    {
        PhSetStringSetting(SETTING_NAME_GEOLITE_API_ID, PhGetStringOrEmpty(accountString));
        PhSetStringSetting(SETTING_NAME_GEOLITE_API_KEY, PhGetStringOrEmpty(licenseString));
    }

    PhClearReference(&accountString);
    PhClearReference(&licenseString);
    PhClearReference(&content);
}

VOID ShowGeoLiteConfigDialog(
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
