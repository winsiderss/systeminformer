/*
 * Process Hacker Extra Plugins -
 *   Taskbar Extensions
 *
 * Copyright (C) 2015 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "main.h"

static PH_KEY_VALUE_PAIR GraphTypePairs[] =
{
    { L"None", (PVOID)TASKBAR_ICON_NONE },
    { L"CPU History", (PVOID)TASKBAR_ICON_CPU_HISTORY },
    { L"CPU Usage", (PVOID)TASKBAR_ICON_CPU_USAGE },
    { L"I/O History", (PVOID)TASKBAR_ICON_IO_HISTORY },
    { L"Commit History", (PVOID)TASKBAR_ICON_COMMIT_HISTORY },
    { L"Physical Memory History", (PVOID)TASKBAR_ICON_PHYSICAL_HISTORY },
};

static WCHAR *GraphTypeStrings[] = 
{ 
    L"None", 
    L"CPU History", 
    L"CPU Usage", 
    L"I/O History",
    L"Commit History",
    L"Physical Memory History"
};

static PWSTR GraphTypeGetTypeString(
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

static ULONG GraphTypeGetTypeInteger(
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

static INT_PTR CALLBACK OptionsDlgProc(
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

            graphTypeHandle = GetDlgItem(hwndDlg, IDC_GRAPH_TYPE);

            PhAddComboBoxStrings(graphTypeHandle, GraphTypeStrings, _countof(GraphTypeStrings));
            PhSelectComboBoxString(graphTypeHandle, GraphTypeGetTypeString(PhGetIntegerSetting(SETTING_NAME_TASKBAR_ICON_TYPE)), FALSE);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    PPH_STRING graphTypeString;

                    graphTypeString = PhAutoDereferenceObject(PhGetWindowText(GetDlgItem(hwndDlg, IDC_GRAPH_TYPE)));             
                    PhSetIntegerSetting(SETTING_NAME_TASKBAR_ICON_TYPE, GraphTypeGetTypeInteger(graphTypeString->Buffer));

                    EndDialog(hwndDlg, IDOK);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

VOID ShowOptionsDialog(
    _In_ HWND ParentHandle
    )
{
    DialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS),
        ParentHandle,
        OptionsDlgProc
        );
}