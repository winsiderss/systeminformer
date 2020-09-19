/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2020 dmex
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

#include <peview.h>


typedef enum _PHP_OPTIONS_INDEX
{
   // PHP_OPTIONS_INDEX_ENABLE_WARNINGS,
    //PHP_OPTIONS_INDEX_ENABLE_PLUGINS,
    //PHP_OPTIONS_INDEX_ENABLE_UNDECORATE_SYMBOLS,
    PHP_OPTIONS_INDEX_ENABLE_THEME_SUPPORT,
    PHP_OPTIONS_INDEX_ENABLE_START_ASADMIN,
    PHP_OPTIONS_INDEX_SHOW_ADVANCED_OPTIONS
} PHP_OPTIONS_GENERAL_INDEX;

BOOLEAN RestartRequired = FALSE;

#define SetDlgItemCheckForSetting(hwndDlg, Id, Name) \
    Button_SetCheck(GetDlgItem(hwndDlg, Id), PhGetIntegerSetting(Name) ? BST_CHECKED : BST_UNCHECKED)
#define SetSettingForDlgItemCheck(hwndDlg, Id, Name) \
    PhSetIntegerSetting(Name, Button_GetCheck(GetDlgItem(hwndDlg, Id)) == BST_CHECKED)
#define SetSettingForDlgItemCheckRestartRequired(hwndDlg, Id, Name) \
{ \
    BOOLEAN __oldValue = !!PhGetIntegerSetting(Name); \
    BOOLEAN __newValue = Button_GetCheck(GetDlgItem(hwndDlg, Id)) == BST_CHECKED; \
    if (__newValue != __oldValue) \
        RestartRequired = TRUE; \
    PhSetIntegerSetting(Name, __newValue); \
}

#define SetLvItemCheckForSetting(ListViewHandle, Index, Name) \
    ListView_SetCheckState(ListViewHandle, Index, !!PhGetIntegerSetting(Name));
#define SetSettingForLvItemCheck(ListViewHandle, Index, Name) \
    PhSetIntegerSetting(Name, ListView_GetCheckState(ListViewHandle, Index) == BST_CHECKED)
#define SetSettingForLvItemCheckRestartRequired(ListViewHandle, Index, Name) \
{ \
    BOOLEAN __oldValue = !!PhGetIntegerSetting(Name); \
    BOOLEAN __newValue = ListView_GetCheckState(ListViewHandle, Index) == BST_CHECKED; \
    if (__newValue != __oldValue) \
        RestartRequired = TRUE; \
    PhSetIntegerSetting(Name, __newValue); \
}

_Success_(return)
static BOOLEAN GetCurrentFont(
    _Out_ PLOGFONT Font
    )
{
    BOOLEAN result;
    PPH_STRING fontHexString;

    fontHexString = PhaGetStringSetting(L"Font");

    if (fontHexString->Length / sizeof(WCHAR) / 2 == sizeof(LOGFONT))
        result = PhHexStringToBuffer(&fontHexString->sr, (PUCHAR)Font);
    else
        result = FALSE;

    return result;
}

VOID PvAppendCommandLineArgument(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ PWSTR Name,
    _In_ PPH_STRINGREF Value
    )
{
    PPH_STRING temp;

    PhAppendStringBuilder2(StringBuilder, L" -");
    PhAppendStringBuilder2(StringBuilder, Name);
    PhAppendStringBuilder2(StringBuilder, L" \"");
    temp = PhEscapeCommandLinePart(Value);
    PhAppendStringBuilder(StringBuilder, &temp->sr);
    PhDereferenceObject(temp);
    PhAppendCharStringBuilder(StringBuilder, L'\"');
}

BOOLEAN PvShellExecuteRestart(
    _In_opt_ HWND WindowHandle
    )
{
    BOOLEAN result;
    PPH_STRING filename;

    if (!(filename = PhGetApplicationFileName()))
        return FALSE;

    result = PhShellExecuteEx(
        WindowHandle,
        PhGetString(filename),
        PhGetString(PvFileName),
        SW_SHOW,
        0,
        0,
        NULL
        );

    PhDereferenceObject(filename);

    return result;
}

static VOID PvLoadGeneralPage(
    _In_ HWND WindowHandle
    )
{
    HWND listViewHandle = GetDlgItem(WindowHandle, IDC_SETTINGS);

    PhSetDialogItemText(WindowHandle, IDC_DBGHELPSEARCHPATH, PhaGetStringSetting(L"DbgHelpSearchPath")->Buffer);

    ListView_SetExtendedListViewStyleEx(listViewHandle, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    //PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_WARNINGS, L"Enable warnings", NULL);
    //PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_PLUGINS, L"Enable plugins", NULL);
    //PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_UNDECORATE_SYMBOLS, L"Enable undecorated symbols", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_THEME_SUPPORT, L"Enable theme support", NULL);
    //PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_START_ASADMIN, L"Enable start as admin", NULL);
    //PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_SHOW_ADVANCED_OPTIONS, L"Show advanced options", NULL);

    //SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_WARNINGS, L"EnableWarnings");
    //SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_PLUGINS, L"EnablePlugins");
    //SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_UNDECORATE_SYMBOLS, L"DbgHelpUndecorate");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_THEME_SUPPORT, L"EnableThemeSupport");
    //SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_START_ASADMIN, L"EnableStartAsAdmin");
}

static VOID PhpGeneralPageSave(
    _In_ HWND WindowHandle
    )
{
    HWND listViewHandle = GetDlgItem(WindowHandle, IDC_SETTINGS);

    //PhSetStringSetting2(L"SearchEngine", &PhaGetDlgItemText(WindowHandle, IDC_SEARCHENGINE)->sr);

    if (!PhEqualString(PhaGetDlgItemText(WindowHandle, IDC_DBGHELPSEARCHPATH), PhaGetStringSetting(L"DbgHelpSearchPath"), TRUE))
    {
        PhSetStringSetting2(L"DbgHelpSearchPath", &(PhaGetDlgItemText(WindowHandle, IDC_DBGHELPSEARCHPATH)->sr));
        //RestartRequired = TRUE;
    }

    //SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_WARNINGS, L"EnableWarnings");
    //SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_PLUGINS, L"EnablePlugins");
    //SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_UNDECORATE_SYMBOLS, L"DbgHelpUndecorate");
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_THEME_SUPPORT, L"EnableThemeSupport");
    //SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_START_ASADMIN, L"EnableStartAsAdmin");

    PhUpdateCachedSettings();
    PeSaveSettings();

    if (RestartRequired)
    {
        if (PhShowMessage2(
            WindowHandle,
            TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
            TD_INFORMATION_ICON,
            L"One or more options you have changed requires a restart of PE Viewer.",
            L"Do you want to restart PE Viewer now?"
            ) == IDYES)
        {
            if (PvShellExecuteRestart(WindowHandle))
            {
                RtlExitUserProcess(STATUS_SUCCESS);
            }
        }
    }
}

INT_PTR CALLBACK PvOptionsWndProc(
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
            HWND comboBoxHandle;

            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)PvImageSmallIcon);
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)PvImageLargeIcon);

            comboBoxHandle = GetDlgItem(hwndDlg, IDC_MAXSIZEUNIT);

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PvLoadGeneralPage(hwndDlg);

            for (ULONG i = 0; i < RTL_NUMBER_OF(PhSizeUnitNames); i++)
                ComboBox_AddString(comboBoxHandle, PhSizeUnitNames[i]);

            if (PhMaxSizeUnit != ULONG_MAX)
                ComboBox_SetCurSel(comboBoxHandle, PhMaxSizeUnit);
            else
                ComboBox_SetCurSel(comboBoxHandle, RTL_NUMBER_OF(PhSizeUnitNames) - 1);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    PhpGeneralPageSave(hwndDlg);

                    EndDialog(hwndDlg, IDOK);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
