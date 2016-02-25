/*
 * Process Hacker -
 *   options window
 *
 * Copyright (C) 2010-2016 wj32
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
#include <proctree.h>
#include <settings.h>
#include <colorbox.h>
#include <sysinfo.h>
#include <commdlg.h>
#include <windowsx.h>

#define WM_PH_CHILD_EXIT (WM_APP + 301)

INT CALLBACK PhpOptionsPropSheetProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ LPARAM lParam
    );

LRESULT CALLBACK PhpOptionsWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpOptionsGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpOptionsAdvancedDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpOptionsSymbolsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpOptionsHighlightingDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK PhpOptionsGraphsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// All
static BOOLEAN PageInit;
static BOOLEAN PressedOk;
static BOOLEAN RestartRequired;
static POINT StartLocation;
static WNDPROC OldWndProc;

// General
static PH_STRINGREF CurrentUserRunKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run");
static BOOLEAN CurrentUserRunPresent;
static BOOLEAN CurrentUserRunStartHidden;
static HFONT CurrentFontInstance;
static PPH_STRING NewFontSelection;

// Advanced
static PH_STRINGREF TaskMgrImageOptionsKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\taskmgr.exe");
static PPH_STRING OldTaskMgrDebugger;
static BOOLEAN OldReplaceTaskMgr;
static HWND WindowHandleForElevate;

// Highlighting
static HWND HighlightingListViewHandle;

VOID PhShowOptionsDialog(
    _In_ HWND ParentWindowHandle
    )
{
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    PROPSHEETPAGE propSheetPage;
    HPROPSHEETPAGE pages[5];

    propSheetHeader.dwFlags =
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP |
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
    RestartRequired = FALSE;

    if (PhStartupParameters.ShowOptions)
        StartLocation = PhStartupParameters.Point;
    else
        StartLocation.x = MINLONG;

    OldTaskMgrDebugger = NULL;

    PhModalPropertySheet(&propSheetHeader);

    if (PressedOk)
    {
        if (!PhStartupParameters.ShowOptions)
        {
            PhUpdateCachedSettings();
            ProcessHacker_SaveAllSettings(PhMainWndHandle);
            PhInvalidateAllProcessNodes();
            PhReloadSettingsProcessTreeList();
            PhSiNotifyChangeSettings();

            if (RestartRequired)
            {
                if (PhShowMessage(
                    PhMainWndHandle,
                    MB_ICONQUESTION | MB_YESNO,
                    L"One or more options you have changed requires a restart of Process Hacker. "
                    L"Do you want to restart Process Hacker now?"
                    ) == IDYES)
                {
                    ProcessHacker_PrepareForEarlyShutdown(PhMainWndHandle);
                    PhShellProcessHacker(
                        PhMainWndHandle,
                        L"-v",
                        SW_SHOW,
                        0,
                        PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
                        0,
                        NULL
                        );
                    ProcessHacker_Destroy(PhMainWndHandle);
                }
            }
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
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ LPARAM lParam
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
    _In_ HWND hwndDlg
    )
{
    if (!PageInit)
    {
        HWND optionsWindow;
        HWND resetButton;
        RECT clientRect;
        RECT rect;

        optionsWindow = GetParent(hwndDlg);
        OldWndProc = (WNDPROC)GetWindowLongPtr(optionsWindow, GWLP_WNDPROC);
        SetWindowLongPtr(optionsWindow, GWLP_WNDPROC, (LONG_PTR)PhpOptionsWndProc);

        // Create the Reset button.
        GetClientRect(optionsWindow, &clientRect);
        GetWindowRect(GetDlgItem(optionsWindow, IDCANCEL), &rect);
        MapWindowPoints(NULL, optionsWindow, (POINT *)&rect, 2);
        resetButton = CreateWindowEx(
            WS_EX_NOPARENTNOTIFY,
            L"BUTTON",
            L"Reset",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            clientRect.right - rect.right,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            optionsWindow,
            (HMENU)IDC_RESET,
            PhInstanceHandle,
            NULL
            );
        SendMessage(resetButton, WM_SETFONT, SendMessage(GetDlgItem(optionsWindow, IDCANCEL), WM_GETFONT, 0, 0), TRUE);

        if (PhStartupParameters.ShowOptions)
            ShowWindow(resetButton, SW_HIDE);

        // Set the location of the options window.
        if (StartLocation.x == MINLONG)
        {
            PhCenterWindow(optionsWindow, GetParent(optionsWindow));
        }
        else
        {
            SetWindowPos(optionsWindow, NULL, StartLocation.x, StartLocation.y, 0, 0,
                SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER);
        }

        PageInit = TRUE;
    }
}

LRESULT CALLBACK PhpOptionsWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_RESET:
                {
                    if (PhShowMessage(
                        hwnd,
                        MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2,
                        L"Do you want to reset all settings and restart Process Hacker?"
                        ) == IDYES)
                    {
                        ProcessHacker_PrepareForEarlyShutdown(PhMainWndHandle);

                        PhResetSettings();

                        if (PhSettingsFileName)
                            PhSaveSettings(PhSettingsFileName->Buffer);

                        PhShellProcessHacker(
                            PhMainWndHandle,
                            L"-v",
                            SW_SHOW,
                            0,
                            PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
                            0,
                            NULL
                            );
                        ProcessHacker_Destroy(PhMainWndHandle);
                    }
                }
                break;
            }
        }
        break;
    }

    return CallWindowProc(OldWndProc, hwnd, uMsg, wParam, lParam);
}

#define SetDlgItemCheckForSetting(hwndDlg, Id, Name) \
    Button_SetCheck(GetDlgItem(hwndDlg, Id), PhGetIntegerSetting(Name) ? BST_CHECKED : BST_UNCHECKED)
#define SetSettingForDlgItemCheck(hwndDlg, Id, Name) \
    PhSetIntegerSetting(Name, Button_GetCheck(GetDlgItem(hwndDlg, Id)) == BST_CHECKED)
#define SetSettingForDlgItemCheckRestartRequired(hwndDlg, Id, Name) \
    do { \
        BOOLEAN __oldValue = !!PhGetIntegerSetting(Name); \
        BOOLEAN __newValue = Button_GetCheck(GetDlgItem(hwndDlg, Id)) == BST_CHECKED; \
        if (__newValue != __oldValue) \
            RestartRequired = TRUE; \
        PhSetIntegerSetting(Name, __newValue); \
    } while (0)
#define DialogChanged PropSheet_Changed(GetParent(hwndDlg), hwndDlg)

static BOOLEAN GetCurrentFont(
    _Out_ PLOGFONT Font
    )
{
    BOOLEAN result;
    PPH_STRING fontHexString;

    if (NewFontSelection)
        fontHexString = NewFontSelection;
    else
        fontHexString = PhaGetStringSetting(L"Font");

    if (fontHexString->Length / 2 / 2 == sizeof(LOGFONT))
        result = PhHexStringToBuffer(&fontHexString->sr, (PUCHAR)Font);
    else
        result = FALSE;

    return result;
}

static VOID ReadCurrentUserRun(
    VOID
    )
{
    HANDLE keyHandle;
    PPH_STRING value;

    CurrentUserRunPresent = FALSE;
    CurrentUserRunStartHidden = FALSE;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_CURRENT_USER,
        &CurrentUserRunKeyName,
        0
        )))
    {
        if (value = PhQueryRegistryString(keyHandle, L"Process Hacker 2"))
        {
            PH_STRINGREF fileName;
            PH_STRINGREF arguments;
            PPH_STRING fullFileName;

            PH_AUTO(value);

            if (PhParseCommandLineFuzzy(&value->sr, &fileName, &arguments, &fullFileName))
            {
                PH_AUTO(fullFileName);

                if (fullFileName && PhEqualString(fullFileName, PhApplicationFileName, TRUE))
                {
                    CurrentUserRunPresent = TRUE;
                    CurrentUserRunStartHidden = PhEqualStringRef2(&arguments, L"-hide", FALSE);
                }
            }
        }

        NtClose(keyHandle);
    }
}

static VOID WriteCurrentUserRun(
    _In_ BOOLEAN Present,
    _In_ BOOLEAN StartHidden
    )
{
    HANDLE keyHandle;

    if (CurrentUserRunPresent == Present && (!Present || CurrentUserRunStartHidden == StartHidden))
        return;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_WRITE,
        PH_KEY_CURRENT_USER,
        &CurrentUserRunKeyName,
        0
        )))
    {
        UNICODE_STRING valueName;

        RtlInitUnicodeString(&valueName, L"Process Hacker 2");

        if (Present)
        {
            PPH_STRING value;

            value = PH_AUTO(PhConcatStrings(3, L"\"", PhApplicationFileName->Buffer, L"\""));

            if (StartHidden)
                value = PhaConcatStrings2(value->Buffer, L" -hide");

            NtSetValueKey(keyHandle, &valueName, 0, REG_SZ, value->Buffer, (ULONG)value->Length + 2);
        }
        else
        {
            NtDeleteValueKey(keyHandle, &valueName);
        }

        NtClose(keyHandle);
    }
}


INT_PTR CALLBACK PhpOptionsGeneralDlgProc(
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
            ULONG i;
            LOGFONT font;

            PhpPageInit(hwndDlg);

            comboBoxHandle = GetDlgItem(hwndDlg, IDC_MAXSIZEUNIT);

            for (i = 0; i < sizeof(PhSizeUnitNames) / sizeof(PWSTR); i++)
                ComboBox_AddString(comboBoxHandle, PhSizeUnitNames[i]);

            SetDlgItemText(hwndDlg, IDC_SEARCHENGINE, PhaGetStringSetting(L"SearchEngine")->Buffer);
            SetDlgItemText(hwndDlg, IDC_PEVIEWER, PhaGetStringSetting(L"ProgramInspectExecutables")->Buffer);

            if (PhMaxSizeUnit != -1)
                ComboBox_SetCurSel(comboBoxHandle, PhMaxSizeUnit);
            else
                ComboBox_SetCurSel(comboBoxHandle, sizeof(PhSizeUnitNames) / sizeof(PWSTR) - 1);

            SetDlgItemInt(hwndDlg, IDC_ICONPROCESSES, PhGetIntegerSetting(L"IconProcesses"), FALSE);

            SetDlgItemCheckForSetting(hwndDlg, IDC_ALLOWONLYONEINSTANCE, L"AllowOnlyOneInstance");
            SetDlgItemCheckForSetting(hwndDlg, IDC_HIDEONCLOSE, L"HideOnClose");
            SetDlgItemCheckForSetting(hwndDlg, IDC_HIDEONMINIMIZE, L"HideOnMinimize");
            SetDlgItemCheckForSetting(hwndDlg, IDC_COLLAPSESERVICES, L"CollapseServicesOnStart");
            SetDlgItemCheckForSetting(hwndDlg, IDC_ICONSINGLECLICK, L"IconSingleClick");
            SetDlgItemCheckForSetting(hwndDlg, IDC_ICONTOGGLESVISIBILITY, L"IconTogglesVisibility");
            SetDlgItemCheckForSetting(hwndDlg, IDC_ENABLEPLUGINS, L"EnablePlugins");

            ReadCurrentUserRun();

            if (CurrentUserRunPresent)
            {
                Button_SetCheck(GetDlgItem(hwndDlg, IDC_STARTATLOGON), BST_CHECKED);

                if (CurrentUserRunStartHidden)
                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_STARTHIDDEN), BST_CHECKED);
            }
            else
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_STARTHIDDEN), FALSE);
            }

            // Set the font of the button for a nice preview.
            if (GetCurrentFont(&font))
            {
                CurrentFontInstance = CreateFontIndirect(&font);

                if (CurrentFontInstance)
                    SendMessage(GetDlgItem(hwndDlg, IDC_FONT), WM_SETFONT, (WPARAM)CurrentFontInstance, TRUE);
            }
        }
        break;
    case WM_DESTROY:
        {
            if (CurrentFontInstance)
                DeleteObject(CurrentFontInstance);

            PhClearReference(&NewFontSelection);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_STARTATLOGON:
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_STARTHIDDEN), Button_GetCheck(GetDlgItem(hwndDlg, IDC_STARTATLOGON)) == BST_CHECKED);
                }
                break;
            case IDC_FONT:
                {
                    LOGFONT font;
                    CHOOSEFONT chooseFont;

                    if (!GetCurrentFont(&font))
                    {
                        // Can't get LOGFONT from the existing setting, probably
                        // because the user hasn't ever chosen a font before.
                        // Set the font to something familiar.
                        GetObject((HFONT)SendMessage(PhMainWndHandle, WM_PH_GET_FONT, 0, 0), sizeof(LOGFONT), &font);
                    }

                    memset(&chooseFont, 0, sizeof(CHOOSEFONT));
                    chooseFont.lStructSize = sizeof(CHOOSEFONT);
                    chooseFont.hwndOwner = hwndDlg;
                    chooseFont.lpLogFont = &font;
                    chooseFont.Flags = CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;

                    if (ChooseFont(&chooseFont))
                    {
                        PhMoveReference(&NewFontSelection, PhBufferToHexString((PUCHAR)&font, sizeof(LOGFONT)));

                        // Update the button's font.

                        if (CurrentFontInstance)
                            DeleteObject(CurrentFontInstance);

                        CurrentFontInstance = CreateFontIndirect(&font);
                        SendMessage(GetDlgItem(hwndDlg, IDC_FONT), WM_SETFONT, (WPARAM)CurrentFontInstance, TRUE);
                    }
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
                    BOOLEAN startAtLogon;
                    BOOLEAN startHidden;

                    PhSetStringSetting2(L"SearchEngine", &(PhaGetDlgItemText(hwndDlg, IDC_SEARCHENGINE)->sr));
                    PhSetStringSetting2(L"ProgramInspectExecutables", &(PhaGetDlgItemText(hwndDlg, IDC_PEVIEWER)->sr));
                    PhSetIntegerSetting(L"MaxSizeUnit", PhMaxSizeUnit = ComboBox_GetCurSel(GetDlgItem(hwndDlg, IDC_MAXSIZEUNIT)));
                    PhSetIntegerSetting(L"IconProcesses", GetDlgItemInt(hwndDlg, IDC_ICONPROCESSES, NULL, FALSE));
                    SetSettingForDlgItemCheck(hwndDlg, IDC_ALLOWONLYONEINSTANCE, L"AllowOnlyOneInstance");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_HIDEONCLOSE, L"HideOnClose");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_HIDEONMINIMIZE, L"HideOnMinimize");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_COLLAPSESERVICES, L"CollapseServicesOnStart");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_ICONSINGLECLICK, L"IconSingleClick");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_ICONTOGGLESVISIBILITY, L"IconTogglesVisibility");
                    SetSettingForDlgItemCheckRestartRequired(hwndDlg, IDC_ENABLEPLUGINS, L"EnablePlugins");

                    startAtLogon = Button_GetCheck(GetDlgItem(hwndDlg, IDC_STARTATLOGON)) == BST_CHECKED;
                    startHidden = Button_GetCheck(GetDlgItem(hwndDlg, IDC_STARTHIDDEN)) == BST_CHECKED;
                    WriteCurrentUserRun(startAtLogon, startHidden);

                    if (NewFontSelection)
                    {
                        PhSetStringSetting2(L"Font", &NewFontSelection->sr);
                        PostMessage(PhMainWndHandle, WM_PH_UPDATE_FONT, 0, 0);
                    }

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
    _In_ PPH_STRING Path
    )
{
    BOOLEAN match = FALSE;

    if (PhEqualString(OldTaskMgrDebugger, PhApplicationFileName, TRUE))
    {
        match = TRUE;
    }
    // Allow for a quoted value.
    else if (
        OldTaskMgrDebugger->Length == PhApplicationFileName->Length + sizeof(WCHAR) * 2 &&
        OldTaskMgrDebugger->Buffer[0] == '"' &&
        OldTaskMgrDebugger->Buffer[OldTaskMgrDebugger->Length / sizeof(WCHAR) - 1] == '"'
        )
    {
        PH_STRINGREF partInside;

        partInside.Buffer = &OldTaskMgrDebugger->Buffer[1];
        partInside.Length = OldTaskMgrDebugger->Length - sizeof(WCHAR) * 2;

        if (PhEqualStringRef(&partInside, &PhApplicationFileName->sr, TRUE))
            match = TRUE;
    }

    return match;
}

VOID PhpAdvancedPageLoad(
    _In_ HWND hwndDlg
    )
{
    HWND changeButton;

    SetDlgItemCheckForSetting(hwndDlg, IDC_ENABLEWARNINGS, L"EnableWarnings");
    SetDlgItemCheckForSetting(hwndDlg, IDC_ENABLEKERNELMODEDRIVER, L"EnableKph");
    SetDlgItemCheckForSetting(hwndDlg, IDC_HIDEUNNAMEDHANDLES, L"HideUnnamedHandles");
    SetDlgItemCheckForSetting(hwndDlg, IDC_ENABLESTAGE2, L"EnableStage2");
    SetDlgItemCheckForSetting(hwndDlg, IDC_ENABLENETWORKRESOLVE, L"EnableNetworkResolve");
    SetDlgItemCheckForSetting(hwndDlg, IDC_PROPAGATECPUUSAGE, L"PropagateCpuUsage");
    SetDlgItemCheckForSetting(hwndDlg, IDC_ENABLEINSTANTTOOLTIPS, L"EnableInstantTooltips");

    if (WindowsVersion >= WINDOWS_7)
        SetDlgItemCheckForSetting(hwndDlg, IDC_ENABLECYCLECPUUSAGE, L"EnableCycleCpuUsage");

    SetDlgItemInt(hwndDlg, IDC_SAMPLECOUNT, PhGetIntegerSetting(L"SampleCount"), FALSE);
    SetDlgItemCheckForSetting(hwndDlg, IDC_SAMPLECOUNTAUTOMATIC, L"SampleCountAutomatic");

    if (PhGetIntegerSetting(L"SampleCountAutomatic"))
        EnableWindow(GetDlgItem(hwndDlg, IDC_SAMPLECOUNT), FALSE);

    // Replace Task Manager

    changeButton = GetDlgItem(hwndDlg, IDC_CHANGE);

    if (PhGetOwnTokenAttributes().Elevated)
    {
        ShowWindow(changeButton, SW_HIDE);
    }
    else
    {
        SendMessage(changeButton, BCM_SETSHIELD, 0, TRUE);
    }

    {
        HANDLE taskmgrKeyHandle = NULL;
        ULONG disposition;
        BOOLEAN success = FALSE;
        BOOLEAN alreadyReplaced = FALSE;

        // See if we can write to the key.
        if (NT_SUCCESS(PhCreateKey(
            &taskmgrKeyHandle,
            KEY_READ | KEY_WRITE,
            PH_KEY_LOCAL_MACHINE,
            &TaskMgrImageOptionsKeyName,
            0,
            0,
            &disposition
            )))
        {
            success = TRUE;
        }

        if (taskmgrKeyHandle || NT_SUCCESS(PhOpenKey(
            &taskmgrKeyHandle,
            KEY_READ,
            PH_KEY_LOCAL_MACHINE,
            &TaskMgrImageOptionsKeyName,
            0
            )))
        {
            PhClearReference(&OldTaskMgrDebugger);

            if (OldTaskMgrDebugger = PhQueryRegistryString(taskmgrKeyHandle, L"Debugger"))
            {
                alreadyReplaced = PathMatchesPh(OldTaskMgrDebugger);
            }

            NtClose(taskmgrKeyHandle);
        }

        if (!success)
            EnableWindow(GetDlgItem(hwndDlg, IDC_REPLACETASKMANAGER), FALSE);

        OldReplaceTaskMgr = alreadyReplaced;
        Button_SetCheck(GetDlgItem(hwndDlg, IDC_REPLACETASKMANAGER), alreadyReplaced ? BST_CHECKED : BST_UNCHECKED);
    }
}

VOID PhpAdvancedPageSave(
    _In_ HWND hwndDlg
    )
{
    ULONG sampleCount;

    SetSettingForDlgItemCheck(hwndDlg, IDC_ENABLEWARNINGS, L"EnableWarnings");
    SetSettingForDlgItemCheckRestartRequired(hwndDlg, IDC_ENABLEKERNELMODEDRIVER, L"EnableKph");
    SetSettingForDlgItemCheck(hwndDlg, IDC_HIDEUNNAMEDHANDLES, L"HideUnnamedHandles");
    SetSettingForDlgItemCheckRestartRequired(hwndDlg, IDC_ENABLESTAGE2, L"EnableStage2");
    SetSettingForDlgItemCheckRestartRequired(hwndDlg, IDC_ENABLENETWORKRESOLVE, L"EnableNetworkResolve");
    SetSettingForDlgItemCheck(hwndDlg, IDC_PROPAGATECPUUSAGE, L"PropagateCpuUsage");
    SetSettingForDlgItemCheck(hwndDlg, IDC_ENABLEINSTANTTOOLTIPS, L"EnableInstantTooltips");

    if (WindowsVersion >= WINDOWS_7)
        SetSettingForDlgItemCheckRestartRequired(hwndDlg, IDC_ENABLECYCLECPUUSAGE, L"EnableCycleCpuUsage");

    sampleCount = GetDlgItemInt(hwndDlg, IDC_SAMPLECOUNT, NULL, FALSE);
    SetSettingForDlgItemCheckRestartRequired(hwndDlg, IDC_SAMPLECOUNTAUTOMATIC, L"SampleCountAutomatic");

    if (sampleCount == 0)
        sampleCount = 1;

    if (sampleCount != PhGetIntegerSetting(L"SampleCount"))
        RestartRequired = TRUE;

    PhSetIntegerSetting(L"SampleCount", sampleCount);

    // Replace Task Manager
    if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_REPLACETASKMANAGER)))
    {
        NTSTATUS status;
        HANDLE taskmgrKeyHandle;
        BOOLEAN replaceTaskMgr;
        UNICODE_STRING valueName;

        replaceTaskMgr = Button_GetCheck(GetDlgItem(hwndDlg, IDC_REPLACETASKMANAGER)) == BST_CHECKED;

        if (OldReplaceTaskMgr != replaceTaskMgr)
        {
            // We should have created the key back in PhpAdvancedPageLoad, which is why
            // we're opening the key here.
            if (NT_SUCCESS(PhOpenKey(
                &taskmgrKeyHandle,
                KEY_WRITE,
                PH_KEY_LOCAL_MACHINE,
                &TaskMgrImageOptionsKeyName,
                0
                )))
            {
                RtlInitUnicodeString(&valueName, L"Debugger");

                if (replaceTaskMgr)
                {
                    PPH_STRING quotedFileName;

                    quotedFileName = PH_AUTO(PhConcatStrings(3, L"\"", PhApplicationFileName->Buffer, L"\""));
                    status = NtSetValueKey(taskmgrKeyHandle, &valueName, 0, REG_SZ, quotedFileName->Buffer, (ULONG)quotedFileName->Length + 2);
                }
                else
                {
                    status = NtDeleteValueKey(taskmgrKeyHandle, &valueName);
                }

                if (!NT_SUCCESS(status))
                    PhShowStatus(hwndDlg, L"Unable to replace Task Manager", status, 0);

                NtClose(taskmgrKeyHandle);
            }
        }
    }
}

NTSTATUS PhpElevateAdvancedThreadStart(
    _In_ PVOID Parameter
    )
{
    PPH_STRING arguments;

    arguments = Parameter;
    PhShellProcessHacker(
        WindowHandleForElevate,
        arguments->Buffer,
        SW_SHOW,
        PH_SHELL_EXECUTE_ADMIN,
        PH_SHELL_APP_PROPAGATE_PARAMETERS,
        INFINITE,
        NULL
        );
    PhDereferenceObject(arguments);

    PostMessage(WindowHandleForElevate, WM_PH_CHILD_EXIT, 0, 0);

    return STATUS_SUCCESS;
}

INT_PTR CALLBACK PhpOptionsAdvancedDlgProc(
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
            PhpPageInit(hwndDlg);
            PhpAdvancedPageLoad(hwndDlg);

            if (PhStartupParameters.ShowOptions)
            {
                // Disable all controls except for Replace Task Manager.
                EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLEWARNINGS), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLEKERNELMODEDRIVER), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_HIDEUNNAMEDHANDLES), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLESTAGE2), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLENETWORKRESOLVE), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_PROPAGATECPUUSAGE), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLEINSTANTTOOLTIPS), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLECYCLECPUUSAGE), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_SAMPLECOUNTLABEL), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_SAMPLECOUNT), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_SAMPLECOUNTAUTOMATIC), FALSE);
            }
            else
            {
                if (WindowsVersion < WINDOWS_7)
                    EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLECYCLECPUUSAGE), FALSE); // cycle-based CPU usage not available before Windows 7
            }
        }
        break;
    case WM_DESTROY:
        {
            PhClearReference(&OldTaskMgrDebugger);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_CHANGE:
                {
                    HANDLE threadHandle;
                    RECT windowRect;

                    // Save the options so they don't get "overwritten" when
                    // WM_PH_CHILD_EXIT gets sent.
                    PhpAdvancedPageSave(hwndDlg);

                    GetWindowRect(GetParent(hwndDlg), &windowRect);
                    WindowHandleForElevate = hwndDlg;
                    threadHandle = PhCreateThread(0, PhpElevateAdvancedThreadStart, PhFormatString(
                        L"-showoptions -hwnd %Ix -point %u,%u",
                        (ULONG_PTR)GetParent(hwndDlg),
                        windowRect.left + 20,
                        windowRect.top + 20
                        ));

                    if (threadHandle)
                        NtClose(threadHandle);
                }
                break;
            case IDC_SAMPLECOUNTAUTOMATIC:
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_SAMPLECOUNT), Button_GetCheck(GetDlgItem(hwndDlg, IDC_SAMPLECOUNTAUTOMATIC)) != BST_CHECKED);
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
            PhpPageInit(hwndDlg);

            SetDlgItemText(hwndDlg, IDC_DBGHELPPATH, PhaGetStringSetting(L"DbgHelpPath")->Buffer);
            SetDlgItemText(hwndDlg, IDC_DBGHELPSEARCHPATH, PhaGetStringSetting(L"DbgHelpSearchPath")->Buffer);

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

                    fileName = PH_AUTO(PhGetFileName(PhaGetDlgItemText(hwndDlg, IDC_DBGHELPPATH)));
                    PhSetFileDialogFileName(fileDialog, fileName->Buffer);

                    if (PhShowFileDialog(hwndDlg, fileDialog))
                    {
                        fileName = PH_AUTO(PhGetFileDialogFileName(fileDialog));
                        SetDlgItemText(hwndDlg, IDC_DBGHELPPATH, fileName->Buffer);
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
                    PPH_STRING dbgHelpPath = PhaGetDlgItemText(hwndDlg, IDC_DBGHELPPATH);

                    if (!PhEqualString(dbgHelpPath, PhaGetStringSetting(L"DbgHelpPath"), TRUE))
                        RestartRequired = TRUE;

                    PhSetStringSetting2(L"DbgHelpPath", &dbgHelpPath->sr);
                    PhSetStringSetting2(L"DbgHelpSearchPath", &(PhaGetDlgItemText(hwndDlg, IDC_DBGHELPSEARCHPATH)->sr));
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
    COLOR_ITEM(L"ColorOwnProcesses", L"Own processes", L"Processes running under the same user account as Process Hacker."),
    COLOR_ITEM(L"ColorSystemProcesses", L"System processes", L"Processes running under the NT AUTHORITY\\SYSTEM user account."),
    COLOR_ITEM(L"ColorServiceProcesses", L"Service processes", L"Processes which host one or more services."),
    COLOR_ITEM(L"ColorJobProcesses", L"Job processes", L"Processes associated with a job."),
#ifdef _WIN64
    COLOR_ITEM(L"ColorWow64Processes", L"32-bit processes", L"Processes running under WOW64, i.e. 32-bit."),
#endif
    COLOR_ITEM(L"ColorDebuggedProcesses", L"Debugged processes", L"Processes that are currently being debugged."),
    COLOR_ITEM(L"ColorElevatedProcesses", L"Elevated processes", L"Processes with full privileges on a system with UAC enabled."),
    COLOR_ITEM(L"ColorImmersiveProcesses", L"Immersive processes and DLLs", L"Processes and DLLs that belong to a Modern UI app."),
    COLOR_ITEM(L"ColorSuspended", L"Suspended processes and threads", L"Processes and threads that are suspended from execution."),
    COLOR_ITEM(L"ColorDotNet", L".NET processes and DLLs", L".NET (i.e. managed) processes and DLLs."),
    COLOR_ITEM(L"ColorPacked", L"Packed processes", L"Executables are sometimes \"packed\" to reduce their size."),
    COLOR_ITEM(L"ColorGuiThreads", L"GUI threads", L"Threads that have made at least one GUI-related system call."),
    COLOR_ITEM(L"ColorRelocatedModules", L"Relocated DLLs", L"DLLs that were not loaded at their preferred image bases."),
    COLOR_ITEM(L"ColorProtectedHandles", L"Protected handles", L"Handles that are protected from being closed."),
    COLOR_ITEM(L"ColorInheritHandles", L"Inheritable handles", L"Handles that can be inherited by child processes.")
};

COLORREF NTAPI PhpColorItemColorFunction(
    _In_ INT Index,
    _In_ PVOID Param,
    _In_opt_ PVOID Context
    )
{
    PCOLOR_ITEM item = Param;

    return item->CurrentColor;
}

INT_PTR CALLBACK PhpOptionsHighlightingDlgProc(
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
            ULONG i;

            PhpPageInit(hwndDlg);

            // Highlighting Duration
            SetDlgItemInt(hwndDlg, IDC_HIGHLIGHTINGDURATION, PhCsHighlightingDuration, FALSE);

            // New Objects
            ColorBox_SetColor(GetDlgItem(hwndDlg, IDC_NEWOBJECTS), PhCsColorNew);

            // Removed Objects
            ColorBox_SetColor(GetDlgItem(hwndDlg, IDC_REMOVEDOBJECTS), PhCsColorRemoved);

            // Highlighting
            HighlightingListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(HighlightingListViewHandle, FALSE, TRUE);
            ListView_SetExtendedListViewStyleEx(HighlightingListViewHandle, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
            PhAddListViewColumn(HighlightingListViewHandle, 0, 0, 0, LVCFMT_LEFT, 240, L"Name");
            PhSetExtendedListView(HighlightingListViewHandle);
            ExtendedListView_SetItemColorFunction(HighlightingListViewHandle, PhpColorItemColorFunction);

            for (i = 0; i < sizeof(ColorItems) / sizeof(COLOR_ITEM); i++)
            {
                INT lvItemIndex;

                lvItemIndex = PhAddListViewItem(HighlightingListViewHandle, MAXINT, ColorItems[i].Name, &ColorItems[i]);
                ColorItems[i].CurrentColor = PhGetIntegerSetting(ColorItems[i].SettingName);
                ColorItems[i].CurrentUse = !!PhGetIntegerSetting(ColorItems[i].UseSettingName);
                ListView_SetCheckState(HighlightingListViewHandle, lvItemIndex, ColorItems[i].CurrentUse);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_ENABLEALL:
                {
                    ULONG i;

                    for (i = 0; i < sizeof(ColorItems) / sizeof(COLOR_ITEM); i++)
                        ListView_SetCheckState(HighlightingListViewHandle, i, TRUE);
                }
                break;
            case IDC_DISABLEALL:
                {
                    ULONG i;

                    for (i = 0; i < sizeof(ColorItems) / sizeof(COLOR_ITEM); i++)
                        ListView_SetCheckState(HighlightingListViewHandle, i, FALSE);
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
                        ColorItems[i].CurrentUse = !!ListView_GetCheckState(HighlightingListViewHandle, i);
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
                                InvalidateRect(HighlightingListViewHandle, NULL, TRUE);
                            }
                        }
                    }
                }
                break;
            case LVN_GETINFOTIP:
                {
                    if (header->hwndFrom == HighlightingListViewHandle)
                    {
                        NMLVGETINFOTIP *getInfoTip = (NMLVGETINFOTIP *)lParam;
                        PH_STRINGREF tip;

                        PhInitializeStringRefLongHint(&tip, ColorItems[getInfoTip->iItem].Description);
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
            PhpPageInit(hwndDlg);

            // Show Text
            SetDlgItemCheckForSetting(hwndDlg, IDC_SHOWTEXT, L"GraphShowText");
            SetDlgItemCheckForSetting(hwndDlg, IDC_USEOLDCOLORS, L"GraphColorMode");
            SetDlgItemCheckForSetting(hwndDlg, IDC_SHOWCOMMITINSUMMARY, L"ShowCommitInSummary");

            ColorBox_SetColor(GetDlgItem(hwndDlg, IDC_CPUUSER), PhCsColorCpuUser);
            ColorBox_SetColor(GetDlgItem(hwndDlg, IDC_CPUKERNEL), PhCsColorCpuKernel);
            ColorBox_SetColor(GetDlgItem(hwndDlg, IDC_IORO), PhCsColorIoReadOther);
            ColorBox_SetColor(GetDlgItem(hwndDlg, IDC_IOW), PhCsColorIoWrite);
            ColorBox_SetColor(GetDlgItem(hwndDlg, IDC_PRIVATE), PhCsColorPrivate);
            ColorBox_SetColor(GetDlgItem(hwndDlg, IDC_PHYSICAL), PhCsColorPhysical);
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
                    SetSettingForDlgItemCheck(hwndDlg, IDC_USEOLDCOLORS, L"GraphColorMode");
                    SetSettingForDlgItemCheck(hwndDlg, IDC_SHOWCOMMITINSUMMARY, L"ShowCommitInSummary");
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
