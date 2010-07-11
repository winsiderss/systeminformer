/*
 * Process Hacker - 
 *   options window
 * 
 * Copyright (C) 2010 wj32
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

#include <phapp.h>
#include <settings.h>
#include <windowsx.h>

#define WM_PH_CHILD_EXIT (WM_APP + 301)

INT CALLBACK PhpOptionsPropSheetProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhpOptionsGeneralDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhpOptionsAdvancedDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhpOptionsSymbolsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

static BOOLEAN PageInit;
static BOOLEAN PressedOk;
static POINT StartLocation;

static PPH_STRING OldTaskMgrDebugger;
static BOOLEAN OldReplaceTaskMgr;
static HWND WindowHandleForElevate;

VOID PhShowOptionsDialog(
    __in HWND ParentWindowHandle
    )
{
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    PROPSHEETPAGE propSheetPage;
    HPROPSHEETPAGE pages[3];

    propSheetHeader.dwFlags =
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
        PSH_PROPTITLE |
        PSH_USECALLBACK |
        PSH_USEPSTARTPAGE;
    propSheetHeader.hwndParent = ParentWindowHandle;
    propSheetHeader.pszCaption = L"Options";
    propSheetHeader.nPages = 0;
    propSheetHeader.pStartPage = !PhStartupParameters.ShowOptions ? L"General" : L"Advanced";
    propSheetHeader.phpage = pages;
    propSheetHeader.pfnCallback = PhpOptionsPropSheetProc;

    if (!PhStartupParameters.ShowOptions)
    {
        // Disable all pages other than Advanced.
        // General page
        memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
        propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
        propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_OPTGENERAL);
        propSheetPage.pfnDlgProc = PhpOptionsGeneralDlgProc;
        pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);
    }

    // Advanced page
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_OPTADVANCED);
    propSheetPage.pfnDlgProc = PhpOptionsAdvancedDlgProc;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    if (!PhStartupParameters.ShowOptions)
    {
        // Symbols page
        memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
        propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
        propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_OPTSYMBOLS);
        propSheetPage.pfnDlgProc = PhpOptionsSymbolsDlgProc;
        pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);
    }

    PageInit = FALSE;
    PressedOk = FALSE;

    if (PhStartupParameters.ShowOptions)
        StartLocation = PhStartupParameters.Point;
    else
        StartLocation.x = MINLONG;

    OldTaskMgrDebugger = NULL;

    PropertySheet(&propSheetHeader);

    if (PressedOk)
    {
        if (!PhStartupParameters.ShowOptions)
        {
            ProcessHacker_SaveAllSettings(PhMainWndHandle);
            PhInvalidateAllProcessNodes();
        }
        else
        {
            // Main window not available.
            if (PhSettingsFileName)
                PhSaveSettings(PhSettingsFileName->Buffer);
        }
    }
}

INT CALLBACK PhpOptionsPropSheetProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case PSCB_BUTTONPRESSED:
        {
            if (lParam == PSBTN_OK)
            {
                PressedOk = TRUE;
            }
        }
        break;
    }

    return 0;
}

static VOID PhpPageInit(
    __in HWND hwndDlg
    )
{
    if (!PageInit)
    {
        // HACK

        if (StartLocation.x == MINLONG)
        {
            PhCenterWindow(GetParent(hwndDlg), GetParent(GetParent(hwndDlg)));
        }
        else
        {
            SetWindowPos(GetParent(hwndDlg), NULL, StartLocation.x, StartLocation.y, 0, 0,
                SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER);
        }

        SetWindowText(GetParent(hwndDlg), L"Options"); // so the title isn't "Options Properties"

        PageInit = TRUE;
    }
}

#define SetDlgItemCheckForSetting(hwndDlg, Id, Name) \
    Button_SetCheck(GetDlgItem(hwndDlg, Id), PhGetIntegerSetting(Name) ? BST_CHECKED : BST_UNCHECKED)
#define SetSettingForDlgItemCheck(hwndDlg, Id, Name) \
    PhSetIntegerSetting(Name, Button_GetCheck(GetDlgItem(hwndDlg, Id)) == BST_CHECKED)
#define DialogChanged PropSheet_Changed(GetParent(hwndDlg), hwndDlg)

INT_PTR CALLBACK PhpOptionsGeneralDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND comboBoxHandle;
            ULONG i;

            PhpPageInit(hwndDlg);

            comboBoxHandle = GetDlgItem(hwndDlg, IDC_MAXSIZEUNIT);

            for (i = 0; i < sizeof(PhSizeUnitNames) / sizeof(PWSTR); i++)
                ComboBox_AddString(comboBoxHandle, PhSizeUnitNames[i]);

            SetDlgItemText(hwndDlg, IDC_SEARCHENGINE, PHA_GET_STRING_SETTING(L"SearchEngine")->Buffer);
            SetDlgItemText(hwndDlg, IDC_PEVIEWER, PHA_GET_STRING_SETTING(L"ProgramInspectExecutables")->Buffer);

            if (PhMaxSizeUnit != -1)
                ComboBox_SetCurSel(comboBoxHandle, PhMaxSizeUnit);
            else
                ComboBox_SetCurSel(comboBoxHandle, sizeof(PhSizeUnitNames) / sizeof(PWSTR) - 1);

            SetDlgItemCheckForSetting(hwndDlg, IDC_ALLOWONLYONEINSTANCE, L"AllowOnlyOneInstance");
            SetDlgItemCheckForSetting(hwndDlg, IDC_HIDEONCLOSE, L"HideOnClose");
            SetDlgItemCheckForSetting(hwndDlg, IDC_HIDEONMINIMIZE, L"HideOnMinimize");
            SetDlgItemCheckForSetting(hwndDlg, IDC_STARTHIDDEN, L"StartHidden");
            SetDlgItemCheckForSetting(hwndDlg, IDC_COLLAPSESERVICES, L"CollapseServicesOnStart");
            SetDlgItemCheckForSetting(hwndDlg, IDC_ICONSINGLECLICK, L"IconSingleClick");
            SetDlgItemCheckForSetting(hwndDlg, IDC_ENABLEPROCDB, L"EnableProcDb");
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_APPLY:
                {
                    PhSetStringSetting2(L"SearchEngine", &(PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_SEARCHENGINE)->sr));
                    PhSetStringSetting2(L"ProgramInspectExecutables", &(PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_PEVIEWER)->sr));
                    PhSetIntegerSetting(L"MaxSizeUnit", PhMaxSizeUnit = ComboBox_GetCurSel(GetDlgItem(hwndDlg, IDC_MAXSIZEUNIT)));
                    SetSettingForDlgItemCheck(hwndDlg, IDC_ALLOWONLYONEINSTANCE, L"AllowOnlyOneInstance");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_HIDEONCLOSE, L"HideOnClose");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_HIDEONMINIMIZE, L"HideOnMinimize");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_STARTHIDDEN, L"StartHidden");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_COLLAPSESERVICES, L"CollapseServicesOnStart");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_ICONSINGLECLICK, L"IconSingleClick");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_ENABLEPROCDB, L"EnableProcDb");

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                }
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}

static BOOLEAN PathMatchesPh(
    __in PPH_STRING Path
    )
{
    BOOLEAN match = FALSE;

    if (PhStringEquals(OldTaskMgrDebugger, PhApplicationFileName, TRUE))
    {
        match = TRUE;
    }
    // Allow for a quoted value.
    else if (
        OldTaskMgrDebugger->Length == PhApplicationFileName->Length + 4 &&
        OldTaskMgrDebugger->Buffer[0] == '"' &&
        OldTaskMgrDebugger->Buffer[OldTaskMgrDebugger->Length / 2 - 1] == '"'
        )
    {
        PPH_STRING valueInside;

        valueInside = PhSubstring(OldTaskMgrDebugger, 1, OldTaskMgrDebugger->Length / 2 - 2);

        if (PhStringEquals(valueInside, PhApplicationFileName, TRUE))
            match = TRUE;

        PhDereferenceObject(valueInside);
    }

    return match;
}

VOID PhpAdvancedPageLoad(
    __in HWND hwndDlg
    )
{
    HWND changeButton;

    SetDlgItemCheckForSetting(hwndDlg, IDC_ENABLEWARNINGS, L"EnableWarnings");
    SetDlgItemCheckForSetting(hwndDlg, IDC_ENABLEKERNELMODEDRIVER, L"EnableKph");
    SetDlgItemCheckForSetting(hwndDlg, IDC_HIDEUNNAMEDHANDLES, L"HideUnnamedHandles");

    // Replace Task Manager

    changeButton = GetDlgItem(hwndDlg, IDC_CHANGE);

    if (PhElevated)
    {
        ShowWindow(changeButton, SW_HIDE);
    }
    else
    {
        SendMessage(changeButton, BCM_SETSHIELD, 0, TRUE);
    }

    {
        HKEY keyHandle;
        HKEY taskmgrKeyHandle = NULL;
        ULONG disposition;
        BOOLEAN success = FALSE;
        BOOLEAN alreadyReplaced = FALSE;

        // See if we can write to the key.
        if (RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options",
            0,
            KEY_CREATE_SUB_KEY,
            &keyHandle
            ) == ERROR_SUCCESS)
        {
            if (RegCreateKeyEx(
                keyHandle,
                L"taskmgr.exe",
                0,
                NULL,
                0,
                KEY_READ | KEY_WRITE,
                NULL,
                &taskmgrKeyHandle,
                &disposition
                ) == ERROR_SUCCESS)
            {
                success = TRUE;
            }

            RegCloseKey(keyHandle);
        }

        if (taskmgrKeyHandle || RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\taskmgr.exe",
            0,
            KEY_READ,
            &taskmgrKeyHandle
            ) == ERROR_SUCCESS)
        {
            if (OldTaskMgrDebugger)
                PhDereferenceObject(OldTaskMgrDebugger);

            if (OldTaskMgrDebugger = PhQueryRegistryString(taskmgrKeyHandle, L"Debugger"))
            {
                alreadyReplaced = PathMatchesPh(OldTaskMgrDebugger);
            }

            RegCloseKey(taskmgrKeyHandle);
        }

        if (!success)
            EnableWindow(GetDlgItem(hwndDlg, IDC_REPLACETASKMANAGER), FALSE);

        OldReplaceTaskMgr = alreadyReplaced;
        Button_SetCheck(GetDlgItem(hwndDlg, IDC_REPLACETASKMANAGER), alreadyReplaced ? BST_CHECKED : BST_UNCHECKED);
    }
}

VOID PhpAdvancedPageSave(
    __in HWND hwndDlg
    )
{
    SetSettingForDlgItemCheck(hwndDlg, IDC_ENABLEWARNINGS, L"EnableWarnings");
    SetSettingForDlgItemCheck(hwndDlg, IDC_ENABLEKERNELMODEDRIVER, L"EnableKph");
    SetSettingForDlgItemCheck(hwndDlg, IDC_HIDEUNNAMEDHANDLES, L"HideUnnamedHandles");

    // Replace Task Manager
    if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_REPLACETASKMANAGER)))
    {
        HKEY taskmgrKeyHandle;
        ULONG win32Result = 0;
        BOOLEAN replaceTaskMgr;

        replaceTaskMgr = Button_GetCheck(GetDlgItem(hwndDlg, IDC_REPLACETASKMANAGER)) == BST_CHECKED;

        if (OldReplaceTaskMgr != replaceTaskMgr)
        {
            if ((win32Result = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\taskmgr.exe",
                0,
                KEY_WRITE,
                &taskmgrKeyHandle
                )) == ERROR_SUCCESS)
            {
                if (replaceTaskMgr)
                {
                    PPH_STRING quotedFileName;

                    quotedFileName = PhConcatStrings(3, L"\"", PhApplicationFileName->Buffer, L"\"");
                    win32Result = RegSetValueEx(
                        taskmgrKeyHandle,
                        L"Debugger",
                        0,
                        REG_SZ,
                        (PBYTE)quotedFileName->Buffer,
                        quotedFileName->Length + 2
                        );
                    PhDereferenceObject(quotedFileName);
                }
                else
                {
                    RegDeleteValue(taskmgrKeyHandle, L"Debugger");
                }

                if (win32Result != 0)
                    PhShowStatus(hwndDlg, L"Unable to replace Task Manager", 0, win32Result);

                RegCloseKey(taskmgrKeyHandle);
            }
        }
    }
}

NTSTATUS PhpElevateAdvancedThreadStart(
    __in PVOID Parameter
    )
{
    PPH_STRING arguments;

    arguments = Parameter;
    PhShellExecuteEx(
        WindowHandleForElevate,
        PhApplicationFileName->Buffer,
        arguments->Buffer,
        SW_SHOW,
        PH_SHELL_EXECUTE_ADMIN,
        INFINITE,
        NULL
        );
    PhDereferenceObject(arguments);

    PostMessage(WindowHandleForElevate, WM_PH_CHILD_EXIT, 0, 0);

    return STATUS_SUCCESS;
}

INT_PTR CALLBACK PhpOptionsAdvancedDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhpPageInit(hwndDlg);
            PhpAdvancedPageLoad(hwndDlg);

            if (PhStartupParameters.ShowOptions)
            {
                // Disable all controls except for Replace Task Manager.
                EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLEWARNINGS), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLEKERNELMODEDRIVER), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_HIDEUNNAMEDHANDLES), FALSE);
            }
        }
        break;
    case WM_DESTROY:
        {
            if (OldTaskMgrDebugger)
                PhDereferenceObject(OldTaskMgrDebugger);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_CHANGE:
                {
                    RECT windowRect;

                    // Save the options so they don't get "overwritten" when 
                    // WM_PH_CHILD_EXIT gets sent.
                    PhpAdvancedPageSave(hwndDlg);

                    GetWindowRect(GetParent(hwndDlg), &windowRect);
                    WindowHandleForElevate = hwndDlg;
                    PhCreateThread(0, PhpElevateAdvancedThreadStart, PhFormatString(
                        L"-showoptions -hwnd %Ix -point %u,%u",
                        (ULONG_PTR)GetParent(hwndDlg),
                        windowRect.left + 20,
                        windowRect.top + 20
                        ));
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
            case PSN_APPLY:
                {
                    PhpAdvancedPageSave(hwndDlg);
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                }
                return TRUE;
            }
        }
        break;
    case WM_PH_CHILD_EXIT:
        {
            PhpAdvancedPageLoad(hwndDlg);
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PhpOptionsSymbolsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhpPageInit(hwndDlg);

            SetDlgItemText(hwndDlg, IDC_DBGHELPPATH, PHA_GET_STRING_SETTING(L"DbgHelpPath")->Buffer);
            SetDlgItemText(hwndDlg, IDC_DBGHELPSEARCHPATH, PHA_GET_STRING_SETTING(L"DbgHelpSearchPath")->Buffer);

            SetDlgItemCheckForSetting(hwndDlg, IDC_UNDECORATESYMBOLS, L"DbgHelpUndecorate");
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_BROWSE:
                {
                    static PH_FILETYPE_FILTER filters[] =
                    {
                        { L"dbghelp.dll", L"dbghelp.dll" },
                        { L"All files (*.*)", L"*.*" }
                    };
                    PVOID fileDialog;
                    PPH_STRING fileName;

                    fileDialog = PhCreateOpenFileDialog();
                    PhSetFileDialogFilter(fileDialog, filters, sizeof(filters) / sizeof(PH_FILETYPE_FILTER));

                    fileName = PhGetFileName(PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_DBGHELPPATH));
                    PhSetFileDialogFileName(fileDialog, fileName->Buffer);
                    PhDereferenceObject(fileName);

                    if (PhShowFileDialog(NULL, fileDialog))
                    {
                        fileName = PhGetFileDialogFileName(fileDialog);
                        SetDlgItemText(hwndDlg, IDC_DBGHELPPATH, fileName->Buffer);
                        PhDereferenceObject(fileName);
                    }

                    PhFreeFileDialog(fileDialog);
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
            case PSN_APPLY:
                {
                    PhSetStringSetting2(L"DbgHelpPath", &(PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_DBGHELPPATH)->sr));
                    PhSetStringSetting2(L"DbgHelpSearchPath", &(PHA_GET_DLGITEM_TEXT(hwndDlg, IDC_DBGHELPSEARCHPATH)->sr));
                    SetSettingForDlgItemCheck(hwndDlg, IDC_UNDECORATESYMBOLS, L"DbgHelpUndecorate");

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                }
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}
