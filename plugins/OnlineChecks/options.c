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
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_VIRUSTOTAL),
                PhGetIntegerSetting(SETTING_NAME_VIRUSTOTAL_SCAN_ENABLED) ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_IDC_ENABLE_VIRUSTOTAL_HIGHLIGHT),
                PhGetIntegerSetting(SETTING_NAME_VIRUSTOTAL_HIGHLIGHT_DETECTIONS) ? BST_CHECKED : BST_UNCHECKED);
        }
        break;
    case WM_DESTROY:
        {
            PhSetIntegerSetting(SETTING_NAME_VIRUSTOTAL_SCAN_ENABLED,
                Button_GetCheck(GetDlgItem(hwndDlg, IDC_ENABLE_VIRUSTOTAL)) == BST_CHECKED ? 1 : 0);
        }
        break;
    }

    return FALSE;
}
