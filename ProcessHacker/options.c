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
#include <colorbox.h>
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

INT_PTR CALLBACK PhpOptionsHighlightingDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhpOptionsGraphsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

static BOOLEAN ColorBoxInitialized = FALSE;

static BOOLEAN PageInit;
static BOOLEAN PressedOk;
static POINT StartLocation;

static PPH_STRING OldTaskMgrDebugger;
static BOOLEAN OldReplaceTaskMgr;
static HWND WindowHandleForElevate;

static HWND HighlightingListViewHandle;
static HIMAGELIST HighlightingImageListHandle;

VOID PhShowOptionsDialog(
    __in HWND ParentWindowHandle
    )
{
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    PROPSHEETPAGE propSheetPage;
    HPROPSHEETPAGE pages[5];

    if (!ColorBoxInitialized)
    {
        PhColorBoxInitialization();
        ColorBoxInitialized = TRUE;
    }

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

    if (!PhStartupParameters.ShowOptions)
    {
        // Highlighting page
        memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
        propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
        propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_OPTHIGHLIGHTING);
        propSheetPage.pfnDlgProc = PhpOptionsHighlightingDlgProc;
        pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);
    }

    if (!PhStartupParameters.ShowOptions)
    {
        // Graphs page
        memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
        propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
        propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_OPTGRAPHS);
        propSheetPage.pfnDlgProc = PhpOptionsGraphsDlgProc;
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
            PhUpdateCachedSettings();
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

            SetDlgItemInt(hwndDlg, IDC_ICONPROCESSES, PhGetIntegerSetting(L"IconProcesses"), FALSE);

            SetDlgItemCheckForSetting(hwndDlg, IDC_ALLOWONLYONEINSTANCE, L"AllowOnlyOneInstance");
            SetDlgItemCheckForSetting(hwndDlg, IDC_HIDEONCLOSE, L"HideOnClose");
            SetDlgItemCheckForSetting(hwndDlg, IDC_HIDEONMINIMIZE, L"HideOnMinimize");
            SetDlgItemCheckForSetting(hwndDlg, IDC_STARTHIDDEN, L"StartHidden");
            SetDlgItemCheckForSetting(hwndDlg, IDC_COLLAPSESERVICES, L"CollapseServicesOnStart");
            SetDlgItemCheckForSetting(hwndDlg, IDC_ICONSINGLECLICK, L"IconSingleClick");
            SetDlgItemCheckForSetting(hwndDlg, IDC_ENABLEPROCDB, L"EnableProcDb");
            SetDlgItemCheckForSetting(hwndDlg, IDC_ENABLEPLUGINS, L"EnablePlugins");
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
                    PhSetIntegerSetting(L"IconProcesses", GetDlgItemInt(hwndDlg, IDC_ICONPROCESSES, NULL, FALSE));
                    SetSettingForDlgItemCheck(hwndDlg, IDC_ALLOWONLYONEINSTANCE, L"AllowOnlyOneInstance");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_HIDEONCLOSE, L"HideOnClose");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_HIDEONMINIMIZE, L"HideOnMinimize");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_STARTHIDDEN, L"StartHidden");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_COLLAPSESERVICES, L"CollapseServicesOnStart");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_ICONSINGLECLICK, L"IconSingleClick");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_ENABLEPROCDB, L"EnableProcDb");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_ENABLEPLUGINS, L"EnablePlugins");

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

    if (PhEqualString(OldTaskMgrDebugger, PhApplicationFileName, TRUE))
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

        if (PhEqualString(valueInside, PhApplicationFileName, TRUE))
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
    SetDlgItemCheckForSetting(hwndDlg, IDC_ENABLESTAGE2, L"EnableStage2");

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
    SetSettingForDlgItemCheck(hwndDlg, IDC_ENABLESTAGE2, L"EnableStage2");

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
                EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLESTAGE2), FALSE);
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

#define CROSS_INDEX 0
#define TICK_INDEX 1

typedef struct _COLOR_ITEM
{
    PWSTR SettingName;
    PWSTR UseSettingName;
    PWSTR Name;
    PWSTR Description;

    BOOLEAN CurrentUse;
    COLORREF CurrentColor;
} COLOR_ITEM, *PCOLOR_ITEM;

#define COLOR_ITEM(SettingName, Name, Description) { SettingName, L"Use" SettingName, Name, Description }

static COLOR_ITEM ColorItems[] =
{
    COLOR_ITEM(L"ColorOwnProcesses", L"Own Processes", L"Processes running under the same user account as Process Hacker."),
    COLOR_ITEM(L"ColorSystemProcesses", L"System Processes", L"Processes running under the NT AUTHORITY\\SYSTEM user account."),
    COLOR_ITEM(L"ColorServiceProcesses", L"Service Processes", L"Processes which host one or more services."),
    COLOR_ITEM(L"ColorJobProcesses", L"Job Processes", L"Processes associated with a job."),
#ifdef _M_X64
    COLOR_ITEM(L"ColorWow64Processes", L"32-bit Processes", L"Processes running under WOW64, i.e. 32-bit."),
#endif
    COLOR_ITEM(L"ColorPosixProcesses", L"POSIX Processes", L"Processes running under the POSIX subsystem."),
    COLOR_ITEM(L"ColorDebuggedProcesses", L"Debugged Processes", L"Processes that are currently being debugged."),
    COLOR_ITEM(L"ColorElevatedProcesses", L"Elevated Processes", L"Processes with full privileges on a Windows Vista system with UAC enabled."),
    COLOR_ITEM(L"ColorSuspended", L"Suspended Processes and Threads", L"Processes and threads that are suspended from execution."),
    COLOR_ITEM(L"ColorDotNet", L".NET Processes and DLLs", L".NET, or managed processes and DLLs."),
    COLOR_ITEM(L"ColorPacked", L"Packed Processes", L"Executables are sometimes \"packed\" to reduce their size."),
    COLOR_ITEM(L"ColorGuiThreads", L"GUI Threads", L"Threads that have made at least one GUI-related system call."),
    COLOR_ITEM(L"ColorRelocatedModules", L"Relocated DLLs", L"DLLs that were not loaded at their preferred image bases."),
    COLOR_ITEM(L"ColorProtectedHandles", L"Protected Handles", L"Handles that are protected from being closed."),
    COLOR_ITEM(L"ColorInheritHandles", L"Inheritable Handles", L"Handles that can be inherited by child processes.")
};

VOID PhpRefreshColorItems()
{
    ULONG i;

    for (i = 0; i < sizeof(ColorItems) / sizeof(COLOR_ITEM); i++)
    {
        PCOLOR_ITEM item = &ColorItems[i];

        PhSetListViewItemImageIndex(HighlightingListViewHandle, i,
            item->CurrentUse ? TICK_INDEX : CROSS_INDEX);
    }

    InvalidateRect(HighlightingListViewHandle, NULL, TRUE);
}

COLORREF NTAPI PhpColorItemColorFunction(
    __in INT Index,
    __in PVOID Param,
    __in PVOID Context
    )
{
    PCOLOR_ITEM item = Param;

    return item->CurrentColor;
}

INT_PTR CALLBACK PhpOptionsHighlightingDlgProc(
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
            HWND hwnd;
            ULONG i;

            PhpPageInit(hwndDlg);

            // Highlighting Duration
            SetDlgItemInt(hwndDlg, IDC_HIGHLIGHTINGDURATION, PhCsHighlightingDuration, FALSE);

            // New Objects
            hwnd = PhCreateColorBoxControl(hwndDlg, IDC_NEWOBJECTS);
            PhCopyControlRectangle(hwndDlg, GetDlgItem(hwndDlg, IDC_NEWOBJECTS_LAYOUT), hwnd);
            ShowWindow(hwnd, SW_SHOW);
            ColorBox_SetColor(hwnd, PhCsColorNew);

            // Removed Objects
            hwnd = PhCreateColorBoxControl(hwndDlg, IDC_REMOVEDOBJECTS);
            PhCopyControlRectangle(hwndDlg, GetDlgItem(hwndDlg, IDC_REMOVEDOBJECTS_LAYOUT), hwnd);
            ShowWindow(hwnd, SW_SHOW);
            ColorBox_SetColor(hwnd, PhCsColorRemoved);

            // Highlighting
            HighlightingListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(HighlightingListViewHandle, FALSE, TRUE);
            PhAddListViewColumn(HighlightingListViewHandle, 0, 0, 0, LVCFMT_LEFT, 240, L"Name");
            PhSetExtendedListView(HighlightingListViewHandle);
            ExtendedListView_SetItemColorFunction(HighlightingListViewHandle, PhpColorItemColorFunction);

            HighlightingImageListHandle = ImageList_Create(16, 16, ILC_COLOR32, 0, 0);
            ImageList_SetImageCount(HighlightingImageListHandle, 2);
            PhSetImageListBitmap(HighlightingImageListHandle, CROSS_INDEX, PhInstanceHandle, MAKEINTRESOURCE(IDB_CROSS));
            PhSetImageListBitmap(HighlightingImageListHandle, TICK_INDEX, PhInstanceHandle, MAKEINTRESOURCE(IDB_TICK));
            ListView_SetImageList(HighlightingListViewHandle, HighlightingImageListHandle, LVSIL_SMALL);

            for (i = 0; i < sizeof(ColorItems) / sizeof(COLOR_ITEM); i++)
            {
                INT lvItemIndex;

                lvItemIndex = PhAddListViewItem(HighlightingListViewHandle, MAXINT, ColorItems[i].Name, &ColorItems[i]);
                ColorItems[i].CurrentColor = PhGetIntegerSetting(ColorItems[i].SettingName);
                ColorItems[i].CurrentUse = !!PhGetIntegerSetting(ColorItems[i].UseSettingName);
            }

            PhpRefreshColorItems();
            EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE), FALSE);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_ENABLE:
                {
                    PCOLOR_ITEM item;

                    if (item = PhGetSelectedListViewItemParam(HighlightingListViewHandle))
                        item->CurrentUse = !item->CurrentUse;

                    PhpRefreshColorItems();
                }
                break;
            case IDC_ENABLEALL:
                {
                    ULONG i;

                    for (i = 0; i < sizeof(ColorItems) / sizeof(COLOR_ITEM); i++)
                        ColorItems[i].CurrentUse = TRUE;

                    PhpRefreshColorItems();
                }
                break;
            case IDC_DISABLEALL:
                {
                    ULONG i;

                    for (i = 0; i < sizeof(ColorItems) / sizeof(COLOR_ITEM); i++)
                        ColorItems[i].CurrentUse = FALSE;

                    PhpRefreshColorItems();
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
                    ULONG i;

                    PH_SET_INTEGER_CACHED_SETTING(HighlightingDuration, GetDlgItemInt(hwndDlg, IDC_HIGHLIGHTINGDURATION, NULL, FALSE));
                    PH_SET_INTEGER_CACHED_SETTING(ColorNew, ColorBox_GetColor(GetDlgItem(hwndDlg, IDC_NEWOBJECTS)));
                    PH_SET_INTEGER_CACHED_SETTING(ColorRemoved, ColorBox_GetColor(GetDlgItem(hwndDlg, IDC_REMOVEDOBJECTS)));

                    for (i = 0; i < sizeof(ColorItems) / sizeof(COLOR_ITEM); i++)
                    {
                        PhSetIntegerSetting(ColorItems[i].SettingName, ColorItems[i].CurrentColor);
                        PhSetIntegerSetting(ColorItems[i].UseSettingName, ColorItems[i].CurrentUse);
                    }

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                }
                return TRUE;
            case NM_DBLCLK:
                {
                    if (header->hwndFrom == HighlightingListViewHandle)
                    {
                        PCOLOR_ITEM item;

                        if (item = PhGetSelectedListViewItemParam(HighlightingListViewHandle))
                        {
                            CHOOSECOLOR chooseColor = { sizeof(chooseColor) };
                            COLORREF customColors[16] = { 0 };

                            chooseColor.hwndOwner = hwndDlg;
                            chooseColor.rgbResult = item->CurrentColor;
                            chooseColor.lpCustColors = customColors;
                            chooseColor.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_RGBINIT;

                            if (ChooseColor(&chooseColor))
                            {
                                item->CurrentColor = chooseColor.rgbResult;
                                PhpRefreshColorItems();
                            }
                        }
                    }
                }
                break;
            case LVN_ITEMCHANGED:
                {
                    if (header->hwndFrom == HighlightingListViewHandle)
                    {
                        PCOLOR_ITEM item;

                        if (item = PhGetSelectedListViewItemParam(HighlightingListViewHandle))
                        {
                            SetWindowText(GetDlgItem(hwndDlg, IDC_ENABLE),
                                item->CurrentUse ? L"Disable" : L"Enable");
                        }
                        else
                        {
                            SetWindowText(GetDlgItem(hwndDlg, IDC_ENABLE), L"Enable");
                        }

                        EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE), ListView_GetSelectedCount(HighlightingListViewHandle) == 1);
                    }
                }
                break;
            case LVN_GETINFOTIP:
                {
                    if (header->hwndFrom == HighlightingListViewHandle)
                    {
                        NMLVGETINFOTIP *getInfoTip = (NMLVGETINFOTIP *)lParam;
                        PH_STRINGREF tip;

                        PhInitializeStringRef(&tip, ColorItems[getInfoTip->iItem].Description);
                        PhCopyListViewInfoTip(getInfoTip, &tip); 
                    }
                }
                break;
            }
        }
        break;
    }

    REFLECT_MESSAGE_DLG(hwndDlg, HighlightingListViewHandle, uMsg, wParam, lParam);

    return FALSE;
}

INT_PTR CALLBACK PhpOptionsGraphsDlgProc(
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
            HWND hwnd;

            PhpPageInit(hwndDlg);

            // Show Text
            SetDlgItemCheckForSetting(hwndDlg, IDC_SHOWTEXT, L"GraphShowText");

            // CPU User
            hwnd = PhCreateColorBoxControl(hwndDlg, IDC_CPUUSER);
            PhCopyControlRectangle(hwndDlg, GetDlgItem(hwndDlg, IDC_CPUUSER_LAYOUT), hwnd);
            ShowWindow(hwnd, SW_SHOW);
            ColorBox_SetColor(hwnd, PhCsColorCpuUser);

            // CPU Kernel
            hwnd = PhCreateColorBoxControl(hwndDlg, IDC_CPUKERNEL);
            PhCopyControlRectangle(hwndDlg, GetDlgItem(hwndDlg, IDC_CPUKERNEL_LAYOUT), hwnd);
            ShowWindow(hwnd, SW_SHOW);
            ColorBox_SetColor(hwnd, PhCsColorCpuKernel);

            // I/O R+O
            hwnd = PhCreateColorBoxControl(hwndDlg, IDC_IORO);
            PhCopyControlRectangle(hwndDlg, GetDlgItem(hwndDlg, IDC_IORO_LAYOUT), hwnd);
            ShowWindow(hwnd, SW_SHOW);
            ColorBox_SetColor(hwnd, PhCsColorIoReadOther);

            // I/O W
            hwnd = PhCreateColorBoxControl(hwndDlg, IDC_IOW);
            PhCopyControlRectangle(hwndDlg, GetDlgItem(hwndDlg, IDC_IOW_LAYOUT), hwnd);
            ShowWindow(hwnd, SW_SHOW);
            ColorBox_SetColor(hwnd, PhCsColorIoWrite);

            // Private
            hwnd = PhCreateColorBoxControl(hwndDlg, IDC_PRIVATE);
            PhCopyControlRectangle(hwndDlg, GetDlgItem(hwndDlg, IDC_PRIVATE_LAYOUT), hwnd);
            ShowWindow(hwnd, SW_SHOW);
            ColorBox_SetColor(hwnd, PhCsColorPrivate);

            // Physical
            hwnd = PhCreateColorBoxControl(hwndDlg, IDC_PHYSICAL);
            PhCopyControlRectangle(hwndDlg, GetDlgItem(hwndDlg, IDC_PHYSICAL_LAYOUT), hwnd);
            ShowWindow(hwnd, SW_SHOW);
            ColorBox_SetColor(hwnd, PhCsColorPhysical);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_APPLY:
                {
                    SetSettingForDlgItemCheck(hwndDlg, IDC_SHOWTEXT, L"GraphShowText");
                    PH_SET_INTEGER_CACHED_SETTING(ColorCpuUser, ColorBox_GetColor(GetDlgItem(hwndDlg, IDC_CPUUSER)));
                    PH_SET_INTEGER_CACHED_SETTING(ColorCpuKernel, ColorBox_GetColor(GetDlgItem(hwndDlg, IDC_CPUKERNEL)));
                    PH_SET_INTEGER_CACHED_SETTING(ColorIoReadOther, ColorBox_GetColor(GetDlgItem(hwndDlg, IDC_IORO)));
                    PH_SET_INTEGER_CACHED_SETTING(ColorIoWrite, ColorBox_GetColor(GetDlgItem(hwndDlg, IDC_IOW)));
                    PH_SET_INTEGER_CACHED_SETTING(ColorPrivate, ColorBox_GetColor(GetDlgItem(hwndDlg, IDC_PRIVATE)));
                    PH_SET_INTEGER_CACHED_SETTING(ColorPhysical, ColorBox_GetColor(GetDlgItem(hwndDlg, IDC_PHYSICAL)));

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                }
                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}
