/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2016-2024
 *
 */

#include "exttools.h"

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
            Button_SetCheck(GetDlgItem(WindowHandle, IDC_ENABLEETWMONITOR), PhGetIntegerSetting(SETTING_NAME_ENABLE_ETW_MONITOR) ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(WindowHandle, IDC_ENABLEGPUMONITOR), PhGetIntegerSetting(SETTING_NAME_ENABLE_GPU_MONITOR) ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(WindowHandle, IDC_ENABLENPUMONITOR), PhGetIntegerSetting(SETTING_NAME_ENABLE_NPU_MONITOR) ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(WindowHandle, IDC_ENABLESYSINFOGRAPHS), PhGetIntegerSetting(SETTING_NAME_ENABLE_SYSINFO_GRAPHS) ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(WindowHandle, IDC_ENABLEFAHRENHEITTEMP), PhGetIntegerSetting(SETTING_NAME_ENABLE_FAHRENHEIT) ? BST_CHECKED : BST_UNCHECKED);
            Button_SetCheck(GetDlgItem(WindowHandle, IDC_ENABLEFPSMONITOR), PhGetIntegerSetting(SETTING_NAME_ENABLE_FPS_MONITOR) ? BST_CHECKED : BST_UNCHECKED);
        }
        break;
    case WM_DESTROY:
        {
            PhSetIntegerSetting(SETTING_NAME_ENABLE_ETW_MONITOR, Button_GetCheck(GetDlgItem(WindowHandle, IDC_ENABLEETWMONITOR)) == BST_CHECKED);
            PhSetIntegerSetting(SETTING_NAME_ENABLE_GPU_MONITOR, Button_GetCheck(GetDlgItem(WindowHandle, IDC_ENABLEGPUMONITOR)) == BST_CHECKED);
            PhSetIntegerSetting(SETTING_NAME_ENABLE_NPU_MONITOR, Button_GetCheck(GetDlgItem(WindowHandle, IDC_ENABLENPUMONITOR)) == BST_CHECKED);
            PhSetIntegerSetting(SETTING_NAME_ENABLE_SYSINFO_GRAPHS, Button_GetCheck(GetDlgItem(WindowHandle, IDC_ENABLESYSINFOGRAPHS)) == BST_CHECKED);
            PhSetIntegerSetting(SETTING_NAME_ENABLE_FAHRENHEIT, Button_GetCheck(GetDlgItem(WindowHandle, IDC_ENABLEFAHRENHEITTEMP)) == BST_CHECKED);
            PhSetIntegerSetting(SETTING_NAME_ENABLE_FPS_MONITOR, Button_GetCheck(GetDlgItem(WindowHandle, IDC_ENABLEFPSMONITOR)) == BST_CHECKED);
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
