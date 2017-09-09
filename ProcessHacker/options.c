/*
 * Process Hacker -
 *   options window
 *
 * Copyright (C) 2010-2016 wj32
 * Copyright (C) 2017 dmex
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

#include <commdlg.h>
#include <windowsx.h>
#include <uxtheme.h>
#include <vssym32.h>

#include <colorbox.h>
#include <settings.h>

#include <mainwnd.h>
#include <proctree.h>
#include <phplug.h>
#include <phsettings.h>

#define WM_PH_CHILD_EXIT (WM_APP + 301)

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

INT_PTR CALLBACK PhOptionsDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID PhOptionsDestroySection(
    _In_ PPH_OPTIONS_SECTION Section
    );

VOID PhOptionsEnterSectionView(
    _In_ PPH_OPTIONS_SECTION NewSection
    );

VOID PhOptionsLayoutSectionView(
    VOID
    );

VOID PhOptionsEnterSectionViewInner(
    _In_ PPH_OPTIONS_SECTION Section,
    _Inout_ HDWP *DeferHandle,
    _Inout_ HDWP *ContainerDeferHandle
    );

VOID PhOptionsCreateSectionDialog(
    _In_ PPH_OPTIONS_SECTION Section
    );

PPH_OPTIONS_SECTION PhOptionsFindSection(
    _In_ PPH_STRINGREF Name
    );

VOID PhOptionsOnSize(
    VOID
    );

PPH_OPTIONS_SECTION PhOptionsCreateInternalSection(
    _In_ PWSTR Name,
    _In_ PVOID Instance,
    _In_ PWSTR Template,
    _In_ DLGPROC DialogProc,
    _In_ PVOID Parameter
    );

PPH_OPTIONS_SECTION PhOptionsCreateSection(
    _In_ PWSTR Name,
    _In_ PVOID Instance,
    _In_ PWSTR Template,
    _In_ DLGPROC DialogProc,
    _In_ PVOID Parameter
    );

static HWND PhOptionsWindowHandle = NULL;
static PPH_LIST PhOptionsDialogList = NULL;
static PH_LAYOUT_MANAGER WindowLayoutManager;

static PPH_LIST SectionList = NULL;
static PPH_OPTIONS_SECTION CurrentSection = NULL;
static HWND OptionsTreeControl = NULL;
static HWND ContainerControl = NULL;

// All
static BOOLEAN RestartRequired = FALSE;
static POINT StartLocation;

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
    if (PhStartupParameters.ShowOptions)
        StartLocation = PhStartupParameters.Point;
    else
        StartLocation.x = MINLONG;

    DialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_OPTIONS),
        ParentWindowHandle,
        PhOptionsDialogProc
        );

    if (!PhStartupParameters.ShowOptions)
    {
        PhUpdateCachedSettings();
        ProcessHacker_SaveAllSettings(PhMainWndHandle);
        PhInvalidateAllProcessNodes();
        PhReloadSettingsProcessTreeList();
        PhSiNotifyChangeSettings();

        if (RestartRequired)
        {
            if (PhShowMessage2(
                PhMainWndHandle,
                TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
                TD_INFORMATION_ICON,
                L"One or more options you have changed requires a restart of Process Hacker.",
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

static HTREEITEM PhOptionsTreeViewAddItem(
    _In_ PWSTR Text,
    _In_ PVOID Context
    )
{
    TV_INSERTSTRUCT insert;

    memset(&insert, 0, sizeof(TV_INSERTSTRUCT));

    insert.item.mask = TVIF_TEXT | TVIF_PARAM;
    insert.hInsertAfter = TVI_LAST;
    insert.hParent = TVI_ROOT;
    insert.item.pszText = Text;
    insert.item.lParam = (LPARAM)Context;

    return TreeView_InsertItem(OptionsTreeControl, &insert);
}

static PPH_OPTIONS_SECTION GetSelectedSectionTreeView(
    _In_ HTREEITEM SelectedTreeItem
    )
{
    TVITEM item;

    if (!SelectedTreeItem)
        return NULL;

    item.mask = TVIF_PARAM | TVIF_HANDLE;
    item.hItem = SelectedTreeItem;

    if (!TreeView_GetItem(OptionsTreeControl, &item))
        return NULL;

    return (PPH_OPTIONS_SECTION)item.lParam;
}

INT_PTR CALLBACK PhOptionsDialogProc(
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
            PhOptionsWindowHandle = hwndDlg;

            SendMessage(PhOptionsWindowHandle, WM_SETICON, ICON_SMALL, (LPARAM)PH_LOAD_SHARED_ICON_SMALL(PhInstanceHandle, MAKEINTRESOURCE(IDI_PROCESSHACKER)));
            SendMessage(PhOptionsWindowHandle, WM_SETICON, ICON_BIG, (LPARAM)PH_LOAD_SHARED_ICON_LARGE(PhInstanceHandle, MAKEINTRESOURCE(IDI_PROCESSHACKER)));

            // Set the location of the options window.
            if (StartLocation.x == MINLONG)
            {
                PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            }
            else
            {
                SetWindowPos(hwndDlg, NULL, StartLocation.x, StartLocation.y, 0, 0,
                    SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER);
            }

            OptionsTreeControl = GetDlgItem(PhOptionsWindowHandle, IDC_SECTIONTREE);
            ContainerControl = GetDlgItem(PhOptionsWindowHandle, IDD_CONTAINER);

            PhSetWindowStyle(GetDlgItem(hwndDlg, IDC_SEPARATOR), SS_OWNERDRAW, SS_OWNERDRAW);

            PhSetControlTheme(OptionsTreeControl, L"explorer");
            TreeView_SetExtendedStyle(OptionsTreeControl, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
            TreeView_SetImageList(OptionsTreeControl, ImageList_Create(2, 22, ILC_COLOR32, 1, 1), TVSIL_NORMAL); // leak
            TreeView_SetBkColor(OptionsTreeControl, GetSysColor(COLOR_3DFACE));

            PhInitializeLayoutManager(&WindowLayoutManager, PhOptionsWindowHandle);

            PhAddLayoutItem(&WindowLayoutManager, OptionsTreeControl, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_SEPARATOR), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, ContainerControl, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(PhOptionsWindowHandle, IDC_RESET), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(PhOptionsWindowHandle, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            EnableThemeDialogTexture(ContainerControl, ETDT_ENABLETAB);

            {
                PPH_OPTIONS_SECTION section;

                SectionList = PhCreateList(8);
                CurrentSection = NULL;

                if (PhStartupParameters.ShowOptions)
                {
                    // Disable all pages other than Advanced.
                    section = PhOptionsCreateInternalSection(L"Advanced", PhInstanceHandle, MAKEINTRESOURCE(IDD_OPTADVANCED), PhpOptionsAdvancedDlgProc, NULL);
                }
                else
                {
                    section = PhOptionsCreateInternalSection(L"General", PhInstanceHandle, MAKEINTRESOURCE(IDD_OPTGENERAL), PhpOptionsGeneralDlgProc, NULL);
                    PhOptionsCreateInternalSection(L"Advanced", PhInstanceHandle, MAKEINTRESOURCE(IDD_OPTADVANCED), PhpOptionsAdvancedDlgProc, NULL);
                    PhOptionsCreateInternalSection(L"Symbols", PhInstanceHandle, MAKEINTRESOURCE(IDD_OPTSYMBOLS), PhpOptionsSymbolsDlgProc, NULL);
                    PhOptionsCreateInternalSection(L"Highlighting", PhInstanceHandle, MAKEINTRESOURCE(IDD_OPTHIGHLIGHTING), PhpOptionsHighlightingDlgProc, NULL);
                    PhOptionsCreateInternalSection(L"Graphs", PhInstanceHandle, MAKEINTRESOURCE(IDD_OPTGRAPHS), PhpOptionsGraphsDlgProc, NULL);

                    if (PhPluginsEnabled)
                    {
                        PH_PLUGIN_OPTIONS_POINTERS pointers;

                        pointers.WindowHandle = PhOptionsWindowHandle;
                        pointers.CreateSection = PhOptionsCreateSection;
                        pointers.FindSection = PhOptionsFindSection;
                        pointers.EnterSectionView = PhOptionsEnterSectionView;

                        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackOptionsWindowInitializing), &pointers);
                    }
                }

                PhOptionsEnterSectionView(section);
                PhOptionsOnSize();
            }
        }
        break;
    case WM_NCDESTROY:
        {
            ULONG i;
            PPH_OPTIONS_SECTION section;

            for (i = 0; i < SectionList->Count; i++)
            {
                section = SectionList->Items[i];
                PhOptionsDestroySection(section);
            }

            PhDereferenceObject(SectionList);
            SectionList = NULL;

            PhDeleteLayoutManager(&WindowLayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhOptionsOnSize();
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            case IDC_RESET:
                {
                    if (PhShowMessage2(
                        hwndDlg,
                        TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
                        TD_WARNING_ICON,
                        L"Do you want to reset all settings and restart Process Hacker?",
                        L""
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
    case WM_DRAWITEM:
        {
            PDRAWITEMSTRUCT drawInfo = (PDRAWITEMSTRUCT)lParam;
            
            if (drawInfo->CtlID == IDC_SEPARATOR)
            {
                RECT rect;

                rect = drawInfo->rcItem;
                rect.right = 2;
                FillRect(drawInfo->hDC, &rect, GetSysColorBrush(COLOR_3DHIGHLIGHT));
                rect.left += 1;
                FillRect(drawInfo->hDC, &rect, GetSysColorBrush(COLOR_3DSHADOW));
                return TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case TVN_SELCHANGED:
                {
                    LPNMTREEVIEW treeview = (LPNMTREEVIEW)lParam;
                    PPH_OPTIONS_SECTION section;
 
                    if (section = GetSelectedSectionTreeView(treeview->itemNew.hItem))
                    {
                        PhOptionsEnterSectionView(section);
                    }
                }
                break;
            case NM_SETCURSOR:
                {
                    if (header->hwndFrom == OptionsTreeControl)
                    {
                        HCURSOR cursor = (HCURSOR)LoadImage(
                            NULL,
                            IDC_ARROW,
                            IMAGE_CURSOR,
                            0,
                            0,
                            LR_SHARED
                            );

                        if (cursor != GetCursor())
                        {
                            SetCursor(cursor);
                        }

                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                        return TRUE;
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

VOID PhOptionsOnSize(
    VOID
    )
{
    PhLayoutManagerLayout(&WindowLayoutManager);

    if (SectionList && SectionList->Count != 0)
    {
        PhOptionsLayoutSectionView();
    }
}

PPH_OPTIONS_SECTION PhOptionsCreateSection(
    _In_ PWSTR Name,
    _In_ PVOID Instance,
    _In_ PWSTR Template,
    _In_ DLGPROC DialogProc,
    _In_ PVOID Parameter
    )
{
    PPH_OPTIONS_SECTION section;

    section = PhAllocate(sizeof(PH_OPTIONS_SECTION));
    memset(section, 0, sizeof(PH_OPTIONS_SECTION));

    PhInitializeStringRefLongHint(&section->Name, Name);

    section->Instance = Instance;
    section->Template = Template;
    section->DialogProc = DialogProc;
    section->Parameter = Parameter;

    PhAddItemList(SectionList, section);

    PhOptionsTreeViewAddItem(Name, section);

    return section;
}

PPH_OPTIONS_SECTION PhOptionsCreateInternalSection(
    _In_ PWSTR Name,
    _In_ PVOID Instance,
    _In_ PWSTR Template,
    _In_ DLGPROC DialogProc,
    _In_ PVOID Parameter
    )
{
    PPH_OPTIONS_SECTION section;

    section = PhOptionsCreateSection(
        Name, 
        Instance, 
        Template, 
        DialogProc, 
        Parameter
        );

    section->DialogHandle = PhCreateDialogFromTemplate(
        ContainerControl,
        DS_SETFONT | DS_FIXEDSYS | DS_CONTROL | WS_CHILD,
        Instance,
        Template,
        DialogProc,
        Parameter
        );

    return section;
}

VOID PhOptionsDestroySection(
    _In_ PPH_OPTIONS_SECTION Section
    )
{
    PhFree(Section);
}

PPH_OPTIONS_SECTION PhOptionsFindSection(
    _In_ PPH_STRINGREF Name
    )
{
    ULONG i;
    PPH_OPTIONS_SECTION section;

    for (i = 0; i < SectionList->Count; i++)
    {
        section = SectionList->Items[i];

        if (PhEqualStringRef(&section->Name, Name, TRUE))
            return section;
    }

    return NULL;
}

VOID PhOptionsLayoutSectionView(
    VOID
    )
{
    if (CurrentSection && CurrentSection->DialogHandle)
    {
        RECT clientRect;

        GetClientRect(ContainerControl, &clientRect);

        SetWindowPos(
            CurrentSection->DialogHandle,
            NULL,
            0,
            0,
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
    }
}

VOID PhOptionsEnterSectionView(
    _In_ PPH_OPTIONS_SECTION NewSection
    )
{
    ULONG i;
    PPH_OPTIONS_SECTION section;
    PPH_OPTIONS_SECTION oldSection;
    HDWP deferHandle;
    HDWP containerDeferHandle;

    if (CurrentSection == NewSection)
        return;

    oldSection = CurrentSection;
    CurrentSection = NewSection;

    deferHandle = BeginDeferWindowPos(SectionList->Count);
    containerDeferHandle = BeginDeferWindowPos(SectionList->Count);

    PhOptionsEnterSectionViewInner(NewSection, &deferHandle, &containerDeferHandle);
    PhOptionsLayoutSectionView();

    for (i = 0; i < SectionList->Count; i++)
    {
        section = SectionList->Items[i];

        if (section != NewSection)
            PhOptionsEnterSectionViewInner(section, &deferHandle, &containerDeferHandle);
    }

    EndDeferWindowPos(deferHandle);
    EndDeferWindowPos(containerDeferHandle);

    if (NewSection->DialogHandle)
        RedrawWindow(NewSection->DialogHandle, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

VOID PhOptionsEnterSectionViewInner(
    _In_ PPH_OPTIONS_SECTION Section,
    _Inout_ HDWP *DeferHandle,
    _Inout_ HDWP *ContainerDeferHandle
    )
{
    if (Section == CurrentSection && !Section->DialogHandle)
        PhOptionsCreateSectionDialog(Section);

    if (Section->DialogHandle)
    {
        if (Section == CurrentSection)
            *ContainerDeferHandle = DeferWindowPos(*ContainerDeferHandle, Section->DialogHandle, NULL, 0, 0, 0, 0, SWP_SHOWWINDOW_ONLY | SWP_NOREDRAW);
        else
            *ContainerDeferHandle = DeferWindowPos(*ContainerDeferHandle, Section->DialogHandle, NULL, 0, 0, 0, 0, SWP_HIDEWINDOW_ONLY | SWP_NOREDRAW);
    }
}

VOID PhOptionsCreateSectionDialog(
    _In_ PPH_OPTIONS_SECTION Section
    )
{
    Section->DialogHandle = PhCreateDialogFromTemplate(
        ContainerControl,
        DS_SETFONT | DS_FIXEDSYS | DS_CONTROL | WS_CHILD,
        Section->Instance,
        Section->Template,
        Section->DialogProc,
        Section->Parameter
        );
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
        if (value = PhQueryRegistryString(keyHandle, L"Process Hacker"))
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

        RtlInitUnicodeString(&valueName, L"Process Hacker");

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

            if (CurrentFontInstance)
                DeleteObject(CurrentFontInstance);

            PhClearReference(&NewFontSelection);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
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
        }
        break;
    case WM_DESTROY:
        {
            PhpAdvancedPageSave(hwndDlg);

            PhClearReference(&OldTaskMgrDebugger);
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

                    GetWindowRect(GetParent(GetParent(hwndDlg)), &windowRect);
                    WindowHandleForElevate = hwndDlg;

                    PhCreateThread2(PhpElevateAdvancedThreadStart, PhFormatString(
                        L"-showoptions -hwnd %Ix -point %u,%u",
                        (ULONG_PTR)GetParent(GetParent(hwndDlg)),
                        windowRect.left + 20,
                        windowRect.top + 20
                        ));
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
    static PH_LAYOUT_MANAGER LayoutManager;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            SetDlgItemText(hwndDlg, IDC_DBGHELPPATH, PhaGetStringSetting(L"DbgHelpPath")->Buffer);
            SetDlgItemText(hwndDlg, IDC_DBGHELPSEARCHPATH, PhaGetStringSetting(L"DbgHelpSearchPath")->Buffer);

            SetDlgItemCheckForSetting(hwndDlg, IDC_UNDECORATESYMBOLS, L"DbgHelpUndecorate");

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_DBGHELPPATH), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_BROWSE), NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_DBGHELPSEARCHPATH), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
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
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&LayoutManager);
        }
        break;
    case WM_DESTROY:
        {
            PPH_STRING dbgHelpPath = PhaGetDlgItemText(hwndDlg, IDC_DBGHELPPATH);

            if (!PhEqualString(dbgHelpPath, PhaGetStringSetting(L"DbgHelpPath"), TRUE))
                RestartRequired = TRUE;

            PhSetStringSetting2(L"DbgHelpPath", &dbgHelpPath->sr);
            PhSetStringSetting2(L"DbgHelpSearchPath", &(PhaGetDlgItemText(hwndDlg, IDC_DBGHELPSEARCHPATH)->sr));
            SetSettingForDlgItemCheck(hwndDlg, IDC_UNDECORATESYMBOLS, L"DbgHelpUndecorate");

            PhDeleteLayoutManager(&LayoutManager);
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
    COLOR_ITEM(L"ColorPicoProcesses", L"Pico processes", L"Processes that belong to the Windows subsystem for Linux."),
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
    static PH_LAYOUT_MANAGER LayoutManager;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
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

            for (ULONG i = 0; i < ARRAYSIZE(ColorItems); i++)
            {
                INT lvItemIndex;

                lvItemIndex = PhAddListViewItem(HighlightingListViewHandle, MAXINT, ColorItems[i].Name, &ColorItems[i]);
                ColorItems[i].CurrentColor = PhGetIntegerSetting(ColorItems[i].SettingName);
                ColorItems[i].CurrentUse = !!PhGetIntegerSetting(ColorItems[i].UseSettingName);
                ListView_SetCheckState(HighlightingListViewHandle, lvItemIndex, ColorItems[i].CurrentUse);
            }

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, HighlightingListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_INFO), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT | PH_LAYOUT_FORCE_INVALIDATE);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_ENABLEALL), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_DISABLEALL), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
        }
        break;
    case WM_DESTROY:
        {
            PH_SET_INTEGER_CACHED_SETTING(HighlightingDuration, GetDlgItemInt(hwndDlg, IDC_HIGHLIGHTINGDURATION, NULL, FALSE));
            PH_SET_INTEGER_CACHED_SETTING(ColorNew, ColorBox_GetColor(GetDlgItem(hwndDlg, IDC_NEWOBJECTS)));
            PH_SET_INTEGER_CACHED_SETTING(ColorRemoved, ColorBox_GetColor(GetDlgItem(hwndDlg, IDC_REMOVEDOBJECTS)));

            for (ULONG i = 0; i < ARRAYSIZE(ColorItems); i++)
            {
                ColorItems[i].CurrentUse = !!ListView_GetCheckState(HighlightingListViewHandle, i);
                PhSetIntegerSetting(ColorItems[i].SettingName, ColorItems[i].CurrentColor);
                PhSetIntegerSetting(ColorItems[i].UseSettingName, ColorItems[i].CurrentUse);
            }

            PhDeleteLayoutManager(&LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&LayoutManager);

            ExtendedListView_SetColumnWidth(HighlightingListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_ENABLEALL:
                {
                    for (ULONG i = 0; i < ARRAYSIZE(ColorItems); i++)
                        ListView_SetCheckState(HighlightingListViewHandle, i, TRUE);
                }
                break;
            case IDC_DISABLEALL:
                {
                    for (ULONG i = 0; i < ARRAYSIZE(ColorItems); i++)
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
            case NM_DBLCLK:
                {
                    if (header->hwndFrom == HighlightingListViewHandle)
                    {
                        PCOLOR_ITEM item;

                        if (item = PhGetSelectedListViewItemParam(HighlightingListViewHandle))
                        {
                            CHOOSECOLOR chooseColor = { sizeof(CHOOSECOLOR) };
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
    case WM_DESTROY:
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
        }
        break;
    }

    return FALSE;
}
