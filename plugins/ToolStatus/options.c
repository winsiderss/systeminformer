/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2011-2022
 *
 */

#include "toolstatus.h"

static PH_KEY_VALUE_PAIR GraphTypePairs[] =
{
    { L"None", (PVOID)TASKBAR_ICON_NONE },
    { L"CPU History", (PVOID)TASKBAR_ICON_CPU_HISTORY },
    { L"CPU Usage", (PVOID)TASKBAR_ICON_CPU_USAGE },
    { L"I/O History", (PVOID)TASKBAR_ICON_IO_HISTORY },
    { L"Commit History", (PVOID)TASKBAR_ICON_COMMIT_HISTORY },
    { L"Physical Memory History", (PVOID)TASKBAR_ICON_PHYSICAL_HISTORY },
};

static PWSTR GraphTypeStrings[] =
{
    L"None",
    L"CPU History",
    L"CPU Usage",
    L"I/O History",
    L"Commit History",
    L"Physical Memory History"
};

PWSTR GraphTypeGetTypeString(
    _In_ ULONG SidType
    )
{
    PWSTR string;

    if (PhFindStringSiKeyValuePairs(
        GraphTypePairs,
        sizeof(GraphTypePairs),
        SidType,
        &string
        ))
    {
        return string;
    }

    return L"None";
}

ULONG GraphTypeGetTypeInteger(
    _In_ PWSTR SidType
    )
{
    ULONG integer;

    if (PhFindIntegerSiKeyValuePairs(
        GraphTypePairs,
        sizeof(GraphTypePairs),
        SidType,
        &integer
        ))
    {
        return integer;
    }

    return 0;
}

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND graphTypeHandle;

            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_TOOLBAR), ToolStatusConfig.ToolBarEnabled ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_STATUSBAR), ToolStatusConfig.StatusBarEnabled ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_RESOLVEGHOSTWINDOWS), ToolStatusConfig.ResolveGhostWindows ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_AUTOHIDE_MENU), ToolStatusConfig.AutoHideMenu ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_AUTOFOCUS_SEARCH), ToolStatusConfig.SearchAutoFocus ? BST_CHECKED : BST_UNCHECKED);

            graphTypeHandle = GetDlgItem(hwndDlg, IDC_CURRENT);
            PhAddComboBoxStrings(graphTypeHandle, GraphTypeStrings, RTL_NUMBER_OF(GraphTypeStrings));
            PhSelectComboBoxString(graphTypeHandle, GraphTypeGetTypeString(PhGetIntegerSetting(SETTING_NAME_TASKBARDISPLAYSTYLE)), FALSE);
        }
        break;
    case WM_DESTROY:
        {
            PPH_STRING graphTypeString;

            ToolStatusConfig.ToolBarEnabled = Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_TOOLBAR)) == BST_CHECKED;
            ToolStatusConfig.StatusBarEnabled = Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_STATUSBAR)) == BST_CHECKED;
            ToolStatusConfig.ResolveGhostWindows = Button_GetCheck(GetDlgItem(hwndDlg, IDC_RESOLVEGHOSTWINDOWS)) == BST_CHECKED;
            ToolStatusConfig.AutoHideMenu = Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_AUTOHIDE_MENU)) == BST_CHECKED;
            ToolStatusConfig.SearchAutoFocus = Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_AUTOFOCUS_SEARCH)) == BST_CHECKED;

            PhSetIntegerSetting(SETTING_NAME_TOOLSTATUS_CONFIG, ToolStatusConfig.Flags);

            ToolbarLoadSettings(FALSE);
            ToolbarCreateGraphs();

            if (ToolStatusConfig.AutoHideMenu)
            {
                SetMenu(PhMainWndHandle, NULL);
            }
            else
            {
                SetMenu(PhMainWndHandle, MainMenu);
                DrawMenuBar(PhMainWndHandle);
            }

            if (ToolStatusConfig.SearchBoxEnabled && ToolStatusConfig.SearchAutoFocus && SearchboxHandle)
                SetFocus(SearchboxHandle);

            graphTypeString = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_CURRENT)));
            PhSetIntegerSetting(SETTING_NAME_TASKBARDISPLAYSTYLE, GraphTypeGetTypeInteger(graphTypeString->Buffer));
            TaskbarListIconType = PhGetIntegerSetting(SETTING_NAME_TASKBARDISPLAYSTYLE);
            TaskbarIsDirty = TRUE;

            TaskbarInitialize();
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}
