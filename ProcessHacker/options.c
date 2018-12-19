/*
 * Process Hacker -
 *   options window
 *
 * Copyright (C) 2010-2016 wj32
 * Copyright (C) 2017-2018 dmex
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

PPH_OPTIONS_SECTION PhOptionsCreateSection(
    _In_ PWSTR Name,
    _In_ PVOID Instance,
    _In_ PWSTR Template,
    _In_ DLGPROC DialogProc,
    _In_ PVOID Parameter
    );

PPH_OPTIONS_SECTION PhOptionsCreateSectionAdvanced(
    _In_ PWSTR Name,
    _In_ PVOID Instance,
    _In_ PWSTR Template,
    _In_ DLGPROC DialogProc,
    _In_ PVOID Parameter
    );

BOOLEAN PhpIsDefaultTaskManager(
    VOID
    );

VOID PhpSetDefaultTaskManager(
    _In_ HWND ParentWindowHandle
    );

static HWND PhOptionsWindowHandle = NULL;
static PH_LAYOUT_MANAGER WindowLayoutManager;

static PPH_LIST SectionList = NULL;
static PPH_OPTIONS_SECTION CurrentSection = NULL;
static HWND OptionsTreeControl = NULL;
static HWND ContainerControl = NULL;
static HIMAGELIST OptionsTreeImageList = NULL;

// All
static BOOLEAN RestartRequired = FALSE;

// General
static PH_STRINGREF CurrentUserRunKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run");
static BOOLEAN CurrentUserRunPresent = FALSE;
static HFONT CurrentFontInstance = NULL;
static PPH_STRING NewFontSelection = NULL;
static HIMAGELIST GeneralListviewImageList = NULL;

// Advanced
static PH_STRINGREF TaskMgrImageOptionsKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\taskmgr.exe");
static PPH_STRING OldTaskMgrDebugger = NULL;
static HWND WindowHandleForElevate = NULL;

// Highlighting
static HWND HighlightingListViewHandle = NULL;

VOID PhShowOptionsDialog(
    _In_ HWND ParentWindowHandle
    )
{
    if (PhStartupParameters.ShowOptions)
    {
        PhpSetDefaultTaskManager(ParentWindowHandle);
    }
    else
    {
        if (!PhOptionsWindowHandle)
        {
            PhOptionsWindowHandle = CreateDialog(
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_OPTIONS),
                NULL,
                PhOptionsDialogProc
                );

            PhRegisterDialog(PhOptionsWindowHandle);
            ShowWindow(PhOptionsWindowHandle, SW_SHOW);
        }

        if (IsIconic(PhOptionsWindowHandle))
            ShowWindow(PhOptionsWindowHandle, SW_RESTORE);
        else
            SetForegroundWindow(PhOptionsWindowHandle);
    }
}

static HTREEITEM PhpTreeViewInsertItem(
    _In_opt_ HTREEITEM HandleInsertAfter,
    _In_ PWSTR Text,
    _In_ PVOID Context
    )
{
    TV_INSERTSTRUCT insert;

    memset(&insert, 0, sizeof(TV_INSERTSTRUCT));

    insert.hParent = TVI_ROOT;
    insert.hInsertAfter = HandleInsertAfter;
    insert.item.mask = TVIF_TEXT | TVIF_PARAM;
    insert.item.pszText = Text;
    insert.item.lParam = (LPARAM)Context;

    return TreeView_InsertItem(OptionsTreeControl, &insert);
}

static VOID PhpOptionsShowHideTreeViewItem(
    _In_ BOOLEAN Hide
    )
{
    static PH_STRINGREF generalName = PH_STRINGREF_INIT(L"General");
    static PH_STRINGREF advancedName = PH_STRINGREF_INIT(L"Advanced");

    if (Hide)
    {
        PPH_OPTIONS_SECTION advancedSection;

        if (advancedSection = PhOptionsFindSection(&advancedName))
        {
            if (advancedSection->TreeItemHandle)
            {
                TreeView_DeleteItem(OptionsTreeControl, advancedSection->TreeItemHandle);
                advancedSection->TreeItemHandle = NULL;
            }
        }
    }
    else
    {
        PPH_OPTIONS_SECTION generalSection;
        PPH_OPTIONS_SECTION advancedSection;

        generalSection = PhOptionsFindSection(&generalName);
        advancedSection = PhOptionsFindSection(&advancedName);

        if (generalSection && advancedSection)
        {
            advancedSection->TreeItemHandle = PhpTreeViewInsertItem(
                generalSection->TreeItemHandle,
                advancedName.Buffer,
                advancedSection
                );
        }
    }
}

static PPH_OPTIONS_SECTION PhpTreeViewGetSelectedSection(
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
            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)PH_LOAD_SHARED_ICON_SMALL(PhInstanceHandle, MAKEINTRESOURCE(IDI_PROCESSHACKER)));
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)PH_LOAD_SHARED_ICON_LARGE(PhInstanceHandle, MAKEINTRESOURCE(IDI_PROCESSHACKER)));

            OptionsTreeImageList = ImageList_Create(2, 22, ILC_COLOR, 1, 1);
            OptionsTreeControl = GetDlgItem(hwndDlg, IDC_SECTIONTREE);
            ContainerControl = GetDlgItem(hwndDlg, IDD_CONTAINER);

            PhSetWindowStyle(GetDlgItem(hwndDlg, IDC_SEPARATOR), SS_OWNERDRAW, SS_OWNERDRAW);

            PhSetControlTheme(OptionsTreeControl, L"explorer");
            TreeView_SetExtendedStyle(OptionsTreeControl, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
            TreeView_SetImageList(OptionsTreeControl, OptionsTreeImageList, TVSIL_NORMAL);
            TreeView_SetBkColor(OptionsTreeControl, GetSysColor(COLOR_3DFACE));

            PhInitializeLayoutManager(&WindowLayoutManager, hwndDlg);
            PhAddLayoutItem(&WindowLayoutManager, OptionsTreeControl, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_SEPARATOR), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, ContainerControl, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_RESET), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_CLEANUP), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            //PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_APPLY), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            if (PhEnableThemeSupport) // TODO: fix options dialog theme (dmex)
                PhInitializeWindowTheme(hwndDlg, TRUE);

            {
                PPH_OPTIONS_SECTION section;

                SectionList = PhCreateList(8);
                CurrentSection = NULL;

                section = PhOptionsCreateSection(L"General", PhInstanceHandle, MAKEINTRESOURCE(IDD_OPTGENERAL), PhpOptionsGeneralDlgProc, NULL);
                PhOptionsCreateSectionAdvanced(L"Advanced", PhInstanceHandle, MAKEINTRESOURCE(IDD_OPTADVANCED), PhpOptionsAdvancedDlgProc, NULL);
                PhOptionsCreateSection(L"Highlighting", PhInstanceHandle, MAKEINTRESOURCE(IDD_OPTHIGHLIGHTING), PhpOptionsHighlightingDlgProc, NULL);
                PhOptionsCreateSection(L"Graphs", PhInstanceHandle, MAKEINTRESOURCE(IDD_OPTGRAPHS), PhpOptionsGraphsDlgProc, NULL);

                if (PhPluginsEnabled)
                {
                    PH_PLUGIN_OPTIONS_POINTERS pointers;

                    pointers.WindowHandle = hwndDlg;
                    pointers.CreateSection = PhOptionsCreateSection;
                    pointers.FindSection = PhOptionsFindSection;
                    pointers.EnterSectionView = PhOptionsEnterSectionView;

                    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackOptionsWindowInitializing), &pointers);
                }

                PhOptionsEnterSectionView(section);
                PhOptionsOnSize();
            }

            if (PhGetIntegerPairSetting(L"OptionsWindowPosition").X)
                PhLoadWindowPlacementFromSetting(L"OptionsWindowPosition", L"OptionsWindowSize", hwndDlg);
            else
                PhCenterWindow(hwndDlg, PhMainWndHandle);  
        }
        break;
    case WM_DESTROY:
        {
            ULONG i;
            PPH_OPTIONS_SECTION section;

            PhSaveWindowPlacementToSetting(L"OptionsWindowPosition", L"OptionsWindowSize", hwndDlg);

            PhDeleteLayoutManager(&WindowLayoutManager);

            for (i = 0; i < SectionList->Count; i++)
            {
                section = SectionList->Items[i];
                PhOptionsDestroySection(section);
            }

            PhDereferenceObject(SectionList);
            SectionList = NULL;

            if (OptionsTreeImageList) ImageList_Destroy(OptionsTreeImageList);

            PhUnregisterDialog(PhOptionsWindowHandle);
            PhOptionsWindowHandle = NULL;
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
                {
                    DestroyWindow(hwndDlg);
                }
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
            case IDC_CLEANUP:
                {
                    if (PhShowMessage2(
                        hwndDlg,
                        TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
                        TD_INFORMATION_ICON,
                        L"Do you want to clean up unused plugin settings?",
                        L""
                        ) == IDYES)
                    {
                        PhClearIgnoredSettings();
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

                if (PhEnableThemeSupport)
                {
                    switch (PhCsGraphColorMode)
                    {
                    case 0: // New colors
                        {
                            FillRect(drawInfo->hDC, &rect, GetSysColorBrush(COLOR_3DHIGHLIGHT));
                            rect.left += 1;
                            FillRect(drawInfo->hDC, &rect, GetSysColorBrush(COLOR_3DSHADOW));
                        }
                        break;
                    case 1: // Old colors
                        {
                            SetDCBrushColor(drawInfo->hDC, RGB(0, 0, 0));
                            FillRect(drawInfo->hDC, &rect, GetStockObject(DC_BRUSH));
                        }
                        break;
                    }
                }
                else
                {
                    FillRect(drawInfo->hDC, &rect, GetSysColorBrush(COLOR_3DHIGHLIGHT));
                    rect.left += 1;
                    FillRect(drawInfo->hDC, &rect, GetSysColorBrush(COLOR_3DSHADOW));
                }

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

                    if (section = PhpTreeViewGetSelectedSection(treeview->itemNew.hItem))
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

    section = PhAllocateZero(sizeof(PH_OPTIONS_SECTION));
    PhInitializeStringRefLongHint(&section->Name, Name);
    section->Instance = Instance;
    section->Template = Template;
    section->DialogProc = DialogProc;
    section->Parameter = Parameter;
    section->TreeItemHandle = PhpTreeViewInsertItem(TVI_LAST, Name, section);

    PhAddItemList(SectionList, section);

    return section;
}

PPH_OPTIONS_SECTION PhOptionsCreateSectionAdvanced(
    _In_ PWSTR Name,
    _In_ PVOID Instance,
    _In_ PWSTR Template,
    _In_ DLGPROC DialogProc,
    _In_ PVOID Parameter
    )
{
    PPH_OPTIONS_SECTION section;

    section = PhAllocateZero(sizeof(PH_OPTIONS_SECTION));
    PhInitializeStringRefLongHint(&section->Name, Name);
    section->Instance = Instance;
    section->Template = Template;
    section->DialogProc = DialogProc;
    section->Parameter = Parameter;

    PhAddItemList(SectionList, section);

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
    HDWP containerDeferHandle;

    if (CurrentSection == NewSection)
        return;

    oldSection = CurrentSection;
    CurrentSection = NewSection;

    containerDeferHandle = BeginDeferWindowPos(SectionList->Count);

    PhOptionsEnterSectionViewInner(NewSection, &containerDeferHandle);
    PhOptionsLayoutSectionView();

    for (i = 0; i < SectionList->Count; i++)
    {
        section = SectionList->Items[i];

        if (section != NewSection)
            PhOptionsEnterSectionViewInner(section, &containerDeferHandle);
    }

    EndDeferWindowPos(containerDeferHandle);

    if (NewSection->DialogHandle)
        RedrawWindow(NewSection->DialogHandle, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

VOID PhOptionsEnterSectionViewInner(
    _In_ PPH_OPTIONS_SECTION Section,
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

    PhInitializeWindowTheme(Section->DialogHandle, PhEnableThemeSupport);
}

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

    if (fontHexString->Length / sizeof(WCHAR) / 2 == sizeof(LOGFONT))
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
            PPH_STRING applicationFileName;

            PH_AUTO(value);

            if (PhParseCommandLineFuzzy(&value->sr, &fileName, &arguments, &fullFileName))
            {
                PH_AUTO(fullFileName);

                if (applicationFileName = PhGetApplicationFileName())
                {
                    if (fullFileName && PhEqualString(fullFileName, applicationFileName, TRUE))
                    {
                        CurrentUserRunPresent = TRUE;
                    }

                    PhDereferenceObject(applicationFileName);
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

    if (CurrentUserRunPresent == Present)
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
            PPH_STRING fileName;

            if (fileName = PhGetApplicationFileName())
            {
                value = PH_AUTO(PhConcatStrings(3, L"\"", PhGetStringOrEmpty(fileName), L"\""));

                if (StartHidden)
                    value = PhaConcatStrings2(value->Buffer, L" -hide");

                NtSetValueKey(
                    keyHandle,
                    &valueName,
                    0,
                    REG_SZ,
                    value->Buffer,
                    (ULONG)value->Length + sizeof(WCHAR)
                    );

                PhDereferenceObject(fileName);
            }
        }
        else
        {
            NtDeleteValueKey(keyHandle, &valueName);
        }

        NtClose(keyHandle);
    }
}

static BOOLEAN PathMatchesPh(
    _In_ PPH_STRING Path
    )
{
    BOOLEAN match = FALSE;
    PPH_STRING fileName;

    if (fileName = PhGetApplicationFileName())
    {
        if (PhEqualString(OldTaskMgrDebugger, fileName, TRUE))
        {
            match = TRUE;
        }
        // Allow for a quoted value.
        else if (
            OldTaskMgrDebugger->Length == fileName->Length + sizeof(WCHAR) * sizeof(WCHAR) &&
            OldTaskMgrDebugger->Buffer[0] == '"' &&
            OldTaskMgrDebugger->Buffer[OldTaskMgrDebugger->Length / sizeof(WCHAR) - 1] == '"'
            )
        {
            PH_STRINGREF partInside;

            partInside.Buffer = &OldTaskMgrDebugger->Buffer[1];
            partInside.Length = OldTaskMgrDebugger->Length - sizeof(WCHAR) * sizeof(WCHAR);

            if (PhEqualStringRef(&partInside, &fileName->sr, TRUE))
                match = TRUE;
        }

        PhDereferenceObject(fileName);
    }

    return match;
}

BOOLEAN PhpIsDefaultTaskManager(
    VOID
    )
{
    HANDLE taskmgrKeyHandle;
    BOOLEAN alreadyReplaced = FALSE;

    if (NT_SUCCESS(PhOpenKey(
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

    return alreadyReplaced;
}

VOID PhpSetDefaultTaskManager(
    _In_ HWND ParentWindowHandle
    )
{
    PWSTR message;

    if (PhpIsDefaultTaskManager())
        message = L"Do you want to restore the default Windows Task Manager?";
    else
        message = L"Do you want to make Process Hacker the default Windows Task Manager?";

    if (PhShowMessage2(
        ParentWindowHandle,
        TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
        TD_INFORMATION_ICON,
        L"",
        message
        ) == IDYES)
    {
        NTSTATUS status;
        HANDLE taskmgrKeyHandle;
        UNICODE_STRING valueName;

        status = PhCreateKey(
            &taskmgrKeyHandle,
            KEY_READ | KEY_WRITE,
            PH_KEY_LOCAL_MACHINE,
            &TaskMgrImageOptionsKeyName,
            OBJ_OPENIF,
            0,
            NULL
            );

        if (NT_SUCCESS(status))
        {
            RtlInitUnicodeString(&valueName, L"Debugger");

            if (PhpIsDefaultTaskManager())
            {
                status = NtDeleteValueKey(taskmgrKeyHandle, &valueName);
            }
            else
            {
                PPH_STRING quotedFileName;
                PPH_STRING applicationFileName;

                if (applicationFileName = PhGetApplicationFileName())
                {
                    quotedFileName = PH_AUTO(PhConcatStrings(3, L"\"", PhGetStringOrEmpty(applicationFileName), L"\""));

                    status = NtSetValueKey(
                        taskmgrKeyHandle, 
                        &valueName, 
                        0, 
                        REG_SZ, 
                        quotedFileName->Buffer, 
                        (ULONG)quotedFileName->Length + sizeof(WCHAR)
                        );

                    PhDereferenceObject(applicationFileName);
                }
            }

            NtClose(taskmgrKeyHandle);
        }

        if (!NT_SUCCESS(status))
            PhShowStatus(ParentWindowHandle, L"Unable to replace Task Manager", status, 0);

        if (PhSettingsFileName)
            PhSaveSettings(PhSettingsFileName->Buffer);
    }
}

VOID PhpRefreshTaskManagerState(
    _In_ HWND WindowHandle
    )
{
    if (!PhGetOwnTokenAttributes().Elevated)
    {
        Button_SetElevationRequiredState(GetDlgItem(WindowHandle, IDC_REPLACETASKMANAGER), TRUE);
    }

    if (PhpIsDefaultTaskManager())
    {
        Static_SetText(GetDlgItem(WindowHandle, IDC_DEFSTATE), L"Process Hacker is the default Task Manager:");
        Button_SetText(GetDlgItem(WindowHandle, IDC_REPLACETASKMANAGER), L"Restore default...");
    }
    else
    {
        Static_SetText(GetDlgItem(WindowHandle, IDC_DEFSTATE), L"Process Hacker is not the default Task Manager:");
        Button_SetText(GetDlgItem(WindowHandle, IDC_REPLACETASKMANAGER), L"Make default...");
    }
}

typedef enum _PHP_OPTIONS_INDEX
{
    PHP_OPTIONS_INDEX_SINGLE_INSTANCE,
    PHP_OPTIONS_INDEX_HIDE_WHENCLOSED,
    PHP_OPTIONS_INDEX_HIDE_WHENMINIMIZED,
    PHP_OPTIONS_INDEX_START_ATLOGON,
    PHP_OPTIONS_INDEX_START_HIDDEN,
    PHP_OPTIONS_INDEX_ENABLE_MINIINFO_WINDOW,
    PHP_OPTIONS_INDEX_ENABLE_DRIVER,
    PHP_OPTIONS_INDEX_ENABLE_WARNINGS,
    PHP_OPTIONS_INDEX_ENABLE_PLUGINS,
    PHP_OPTIONS_INDEX_ENABLE_UNDECORATE_SYMBOLS,
    PHP_OPTIONS_INDEX_ENABLE_CYCLE_CPU_USAGE,
    PHP_OPTIONS_INDEX_ENABLE_THEME_SUPPORT,
    PHP_OPTIONS_INDEX_ENABLE_NETWORK_RESOLVE,
    PHP_OPTIONS_INDEX_ENABLE_INSTANT_TOOLTIPS,
    PHP_OPTIONS_INDEX_ENABLE_STAGE2,
    PHP_OPTIONS_INDEX_ENABLE_SERVICE_STAGE2,
    PHP_OPTIONS_INDEX_COLLAPSE_SERVICES_ON_START,
    PHP_OPTIONS_INDEX_ICON_SINGLE_CLICK,
    PHP_OPTIONS_INDEX_ICON_TOGGLE_VISIBILITY,
    PHP_OPTIONS_INDEX_PROPAGATE_CPU_USAGE,
    PHP_OPTIONS_INDEX_SHOW_HEX_ID,
    PHP_OPTIONS_INDEX_SHOW_ADVANCED_OPTIONS
} PHP_OPTIONS_GENERAL_INDEX;

static VOID PhpAdvancedPageLoad(
    _In_ HWND hwndDlg
    )
{
    HWND listViewHandle;

    listViewHandle = GetDlgItem(hwndDlg, IDC_SETTINGS);

    PhSetDialogItemValue(hwndDlg, IDC_SAMPLECOUNT, PhGetIntegerSetting(L"SampleCount"), FALSE);
    SetDlgItemCheckForSetting(hwndDlg, IDC_SAMPLECOUNTAUTOMATIC, L"SampleCountAutomatic");

    if (PhGetIntegerSetting(L"SampleCountAutomatic"))
        EnableWindow(GetDlgItem(hwndDlg, IDC_SAMPLECOUNT), FALSE);

    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_SINGLE_INSTANCE, L"Allow only one instance", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_HIDE_WHENCLOSED, L"Hide when closed", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_HIDE_WHENMINIMIZED, L"Hide when minimized", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_START_ATLOGON, L"Start when I log on", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_START_HIDDEN, L"Start hidden", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_MINIINFO_WINDOW, L"Enable tray information window", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_DRIVER, L"Enable kernel-mode driver", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_WARNINGS, L"Enable warnings", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_PLUGINS, L"Enable plugins", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_UNDECORATE_SYMBOLS, L"Enable undecorated symbols", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_CYCLE_CPU_USAGE, L"Enable cycle-based CPU usage", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_THEME_SUPPORT, L"Enable theme support (experimental)", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_NETWORK_RESOLVE, L"Resolve network addresses", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_INSTANT_TOOLTIPS, L"Show tooltips instantly", NULL);  
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_STAGE2, L"Check images for digital signatures", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_SERVICE_STAGE2, L"Check services for digital signatures", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_COLLAPSE_SERVICES_ON_START, L"Collapse services on start", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ICON_SINGLE_CLICK, L"Single-click tray icons", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ICON_TOGGLE_VISIBILITY, L"Icon click toggles visibility", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_PROPAGATE_CPU_USAGE, L"Include usage of collapsed processes", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_SHOW_HEX_ID, L"Show hexadecimal IDs (experimental)", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_SHOW_ADVANCED_OPTIONS, L"Show advanced options (experimental)", NULL);

    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_SINGLE_INSTANCE, L"AllowOnlyOneInstance");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_HIDE_WHENCLOSED, L"HideOnClose");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_HIDE_WHENMINIMIZED, L"HideOnMinimize");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_START_HIDDEN, L"StartHidden");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_MINIINFO_WINDOW, L"MiniInfoWindowEnabled");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_DRIVER, L"EnableKph");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_WARNINGS, L"EnableWarnings");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_PLUGINS, L"EnablePlugins");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_UNDECORATE_SYMBOLS, L"DbgHelpUndecorate");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_CYCLE_CPU_USAGE, L"EnableCycleCpuUsage");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_THEME_SUPPORT, L"EnableThemeSupport");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_NETWORK_RESOLVE, L"EnableNetworkResolve");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_INSTANT_TOOLTIPS, L"EnableInstantTooltips");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_STAGE2, L"EnableStage2");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_SERVICE_STAGE2, L"EnableServiceStage2");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_COLLAPSE_SERVICES_ON_START, L"CollapseServicesOnStart");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ICON_SINGLE_CLICK, L"IconSingleClick");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ICON_TOGGLE_VISIBILITY, L"IconTogglesVisibility");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_PROPAGATE_CPU_USAGE, L"PropagateCpuUsage");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_SHOW_HEX_ID, L"ShowHexId");

    if (CurrentUserRunPresent)
        ListView_SetCheckState(listViewHandle, PHP_OPTIONS_INDEX_START_ATLOGON, TRUE);
}

static VOID PhpOptionsNotifyChangeCallback(
    _In_ PVOID Context
    )
{
    PhUpdateCachedSettings();
    ProcessHacker_SaveAllSettings(PhMainWndHandle);
    PhInvalidateAllProcessNodes();
    PhReloadSettingsProcessTreeList();
    PhSiNotifyChangeSettings();

    PhReInitializeWindowTheme(PhMainWndHandle);

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

static VOID PhpAdvancedPageSave(
    _In_ HWND hwndDlg
    )
{
    HWND listViewHandle;
    ULONG sampleCount;

    listViewHandle = GetDlgItem(hwndDlg, IDC_SETTINGS);
    sampleCount = PhGetDialogItemValue(hwndDlg, IDC_SAMPLECOUNT);

    SetSettingForDlgItemCheckRestartRequired(hwndDlg, IDC_SAMPLECOUNTAUTOMATIC, L"SampleCountAutomatic");

    if (sampleCount == 0)
        sampleCount = 1;

    if (sampleCount != PhGetIntegerSetting(L"SampleCount"))
        RestartRequired = TRUE;

    PhSetIntegerSetting(L"SampleCount", sampleCount);
    PhSetStringSetting2(L"SearchEngine", &PhaGetDlgItemText(hwndDlg, IDC_SEARCHENGINE)->sr);
    PhSetStringSetting2(L"ProgramInspectExecutables", &PhaGetDlgItemText(hwndDlg, IDC_PEVIEWER)->sr);
    PhSetIntegerSetting(L"MaxSizeUnit", PhMaxSizeUnit = ComboBox_GetCurSel(GetDlgItem(hwndDlg, IDC_MAXSIZEUNIT)));
    PhSetIntegerSetting(L"IconProcesses", PhGetDialogItemValue(hwndDlg, IDC_ICONPROCESSES));

    if (!PhEqualString(PhaGetDlgItemText(hwndDlg, IDC_DBGHELPSEARCHPATH), PhaGetStringSetting(L"DbgHelpSearchPath"), TRUE))
    {
        PhSetStringSetting2(L"DbgHelpSearchPath", &(PhaGetDlgItemText(hwndDlg, IDC_DBGHELPSEARCHPATH)->sr));
        RestartRequired = TRUE;
    }

    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_SINGLE_INSTANCE, L"AllowOnlyOneInstance");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_HIDE_WHENCLOSED, L"HideOnClose");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_HIDE_WHENMINIMIZED, L"HideOnMinimize");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_START_HIDDEN, L"StartHidden");
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_MINIINFO_WINDOW, L"MiniInfoWindowEnabled");
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_DRIVER, L"EnableKph");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_WARNINGS, L"EnableWarnings");
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_PLUGINS, L"EnablePlugins");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_UNDECORATE_SYMBOLS, L"DbgHelpUndecorate");
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_CYCLE_CPU_USAGE, L"EnableCycleCpuUsage");
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_THEME_SUPPORT, L"EnableThemeSupport");
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_NETWORK_RESOLVE, L"EnableNetworkResolve");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_INSTANT_TOOLTIPS, L"EnableInstantTooltips");
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_STAGE2, L"EnableStage2");
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_SERVICE_STAGE2, L"EnableServiceStage2");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_COLLAPSE_SERVICES_ON_START, L"CollapseServicesOnStart");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ICON_SINGLE_CLICK, L"IconSingleClick");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ICON_TOGGLE_VISIBILITY, L"IconTogglesVisibility");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_PROPAGATE_CPU_USAGE, L"PropagateCpuUsage");
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_SHOW_HEX_ID, L"ShowHexId");

    WriteCurrentUserRun(
        ListView_GetCheckState(listViewHandle, PHP_OPTIONS_INDEX_START_ATLOGON) == BST_CHECKED,
        ListView_GetCheckState(listViewHandle, PHP_OPTIONS_INDEX_START_HIDDEN) == BST_CHECKED
        );

    ProcessHacker_Invoke(PhMainWndHandle, PhpOptionsNotifyChangeCallback, NULL);
}

static NTSTATUS PhpElevateAdvancedThreadStart(
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

UINT_PTR CALLBACK PhpChooseFontDlgHookProc(
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
            PhCenterWindow(hwndDlg, PhOptionsWindowHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PhpOptionsGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    static PH_LAYOUT_MANAGER LayoutManager;
    static BOOLEAN GeneralListViewStateInitializing = FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND comboBoxHandle;
            HWND listviewHandle;
            ULONG i;
            LOGFONT font;

            comboBoxHandle = GetDlgItem(hwndDlg, IDC_MAXSIZEUNIT);
            listviewHandle = GetDlgItem(hwndDlg, IDC_SETTINGS);
            GeneralListviewImageList = ImageList_Create(1, 22, ILC_COLOR, 1, 1);

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_SEARCHENGINE), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_PEVIEWER), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, listviewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_DBGHELPSEARCHPATH), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);

            PhSetListViewStyle(listviewHandle, FALSE, TRUE);
            ListView_SetExtendedListViewStyleEx(listviewHandle, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
            ListView_SetImageList(listviewHandle, GeneralListviewImageList, LVSIL_SMALL);
            PhSetControlTheme(listviewHandle, L"explorer");
            PhAddListViewColumn(listviewHandle, 0, 0, 0, LVCFMT_LEFT, 250, L"Name");
            PhSetExtendedListView(listviewHandle);

            for (i = 0; i < RTL_NUMBER_OF(PhSizeUnitNames); i++)
                ComboBox_AddString(comboBoxHandle, PhSizeUnitNames[i]);

            if (PhMaxSizeUnit != ULONG_MAX)
                ComboBox_SetCurSel(comboBoxHandle, PhMaxSizeUnit);
            else
                ComboBox_SetCurSel(comboBoxHandle, RTL_NUMBER_OF(PhSizeUnitNames) - 1);

            PhSetDialogItemText(hwndDlg, IDC_SEARCHENGINE, PhaGetStringSetting(L"SearchEngine")->Buffer);
            PhSetDialogItemText(hwndDlg, IDC_PEVIEWER, PhaGetStringSetting(L"ProgramInspectExecutables")->Buffer);
            PhSetDialogItemValue(hwndDlg, IDC_ICONPROCESSES, PhGetIntegerSetting(L"IconProcesses"), FALSE);
            PhSetDialogItemText(hwndDlg, IDC_DBGHELPSEARCHPATH, PhaGetStringSetting(L"DbgHelpSearchPath")->Buffer);

            ReadCurrentUserRun();

            // Set the font of the button for a nice preview.
            if (GetCurrentFont(&font))
            {
                CurrentFontInstance = CreateFontIndirect(&font);

                if (CurrentFontInstance)
                {
                    SendMessage(OptionsTreeControl, WM_SETFONT, (WPARAM)CurrentFontInstance, TRUE); // HACK
                    SendMessage(listviewHandle, WM_SETFONT, (WPARAM)CurrentFontInstance, TRUE);
                    SendMessage(GetDlgItem(hwndDlg, IDC_FONT), WM_SETFONT, (WPARAM)CurrentFontInstance, TRUE);
                }
            }

            GeneralListViewStateInitializing = TRUE;
            PhpAdvancedPageLoad(hwndDlg);
            PhpRefreshTaskManagerState(hwndDlg);
            GeneralListViewStateInitializing = FALSE;
        }
        break;
    case WM_DESTROY:
        {
            if (NewFontSelection)
            {
                PhSetStringSetting2(L"Font", &NewFontSelection->sr);
                PostMessage(PhMainWndHandle, WM_PH_UPDATE_FONT, 0, 0);
            }

            PhpAdvancedPageSave(hwndDlg);

            if (CurrentFontInstance)
                DeleteObject(CurrentFontInstance);

            if (GeneralListviewImageList)
                ImageList_Destroy(GeneralListviewImageList);

            PhClearReference(&NewFontSelection);
            PhClearReference(&OldTaskMgrDebugger);

            PhDeleteLayoutManager(&LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
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
                    chooseFont.lpfnHook = PhpChooseFontDlgHookProc;
                    chooseFont.lpLogFont = &font;
                    chooseFont.Flags = CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_ENABLEHOOK | CF_SCREENFONTS;

                    if (ChooseFont(&chooseFont))
                    {
                        PhMoveReference(&NewFontSelection, PhBufferToHexString((PUCHAR)&font, sizeof(LOGFONT)));

                        // Update the button's font.

                        if (CurrentFontInstance)
                            DeleteObject(CurrentFontInstance);

                        CurrentFontInstance = CreateFontIndirect(&font);

                        SendMessage(OptionsTreeControl, WM_SETFONT, (WPARAM)CurrentFontInstance, TRUE); // HACK
                        SendMessage(GetDlgItem(hwndDlg, IDC_SETTINGS), WM_SETFONT, (WPARAM)CurrentFontInstance, TRUE);
                        SendMessage(GetDlgItem(hwndDlg, IDC_FONT), WM_SETFONT, (WPARAM)CurrentFontInstance, TRUE);
                        RestartRequired = TRUE; // HACK: Fix ToolStatus plugin toolbar resize on font change
                    }
                }
                break;
            case IDC_REPLACETASKMANAGER:
                {
                    WindowHandleForElevate = hwndDlg;

                    PhCreateThread2(PhpElevateAdvancedThreadStart, PhFormatString(
                        L"-showoptions -hwnd %Ix",
                        (ULONG_PTR)PhOptionsWindowHandle // GetParent(GetParent(hwndDlg))
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
            PhpRefreshTaskManagerState(hwndDlg);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&LayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case NM_CLICK:
            case NM_DBLCLK:
                {
                    LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)header;
                    LVHITTESTINFO lvHitInfo;

                    lvHitInfo.pt = itemActivate->ptAction;

                    if (ListView_HitTest(GetDlgItem(hwndDlg, IDC_SETTINGS), &lvHitInfo) != -1)
                    {
                        // Ignore click notifications for the listview checkbox region.
                        if (!(lvHitInfo.flags & LVHT_ONITEMSTATEICON))
                        {
                            BOOLEAN itemChecked;

                            // Emulate the checkbox control label click behavior and check/uncheck the checkbox when the listview item is clicked.
                            itemChecked = ListView_GetCheckState(GetDlgItem(hwndDlg, IDC_SETTINGS), itemActivate->iItem) == BST_CHECKED;
                            ListView_SetCheckState(GetDlgItem(hwndDlg, IDC_SETTINGS), itemActivate->iItem, !itemChecked);
                        }
                    }
                }
                break;
            case LVN_ITEMCHANGING:
                {
                    LPNM_LISTVIEW listView = (LPNM_LISTVIEW)lParam;

                    if (listView->uChanged & LVIF_STATE)
                    {
                        if (GeneralListViewStateInitializing)
                            break;

                        switch (listView->uNewState & LVIS_STATEIMAGEMASK)
                        {
                        case INDEXTOSTATEIMAGEMASK(1): // unchecked
                            {
                                switch (listView->iItem)
                                {
                                case PHP_OPTIONS_INDEX_ENABLE_DRIVER:
                                    {
                                        if (PhShowMessage2(
                                            PhOptionsWindowHandle,
                                            TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
                                            TD_WARNING_ICON,
                                            L"Are you sure you want to disable the kernel-mode driver?",
                                            L"You will be unable to use more advanced features, view details about system processes or terminate malicious software."
                                            ) == IDNO)
                                        {
                                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                                            return TRUE;
                                        }
                                    }
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
                break;
            case LVN_ITEMCHANGED:
                {
                    LPNM_LISTVIEW listView = (LPNM_LISTVIEW)lParam;

                    if (listView->uChanged & LVIF_STATE)
                    {
                        switch (listView->uNewState & LVIS_STATEIMAGEMASK)
                        {
                        case INDEXTOSTATEIMAGEMASK(2): // checked
                            {
                                switch (listView->iItem)
                                {
                                case PHP_OPTIONS_INDEX_SHOW_ADVANCED_OPTIONS:
                                    {
                                        PhpOptionsShowHideTreeViewItem(FALSE);
                                    }
                                    break;
                                }
                            }
                            break;
                        case INDEXTOSTATEIMAGEMASK(1): // unchecked
                            {
                                switch (listView->iItem)
                                {
                                case PHP_OPTIONS_INDEX_SHOW_ADVANCED_OPTIONS:
                                    {
                                        PhpOptionsShowHideTreeViewItem(TRUE);
                                    }
                                    break;
                                }
                            }
                            break;
                        }
                    }
                }
                break;
            }
        }
        break;
    //case WM_CTLCOLORSTATIC:
    //    {
    //        static HFONT BoldFont = NULL; // leak
    //        HWND control = (HWND)lParam;
    //        HDC hdc = (HDC)wParam;
    //
    //        if (GetDlgCtrlID(control) == IDC_DEFSTATE)
    //        {
    //            if (!BoldFont)
    //                BoldFont = PhDuplicateFontWithNewWeight((HFONT)SendMessage(control, WM_GETFONT, 0, 0), FW_BOLD);
    //
    //            SetBkMode(hdc, TRANSPARENT);
    //
    //            if (!PhpIsDefaultTaskManager())
    //            {
    //                SelectObject(hdc, BoldFont);
    //            }
    //        }
    //    }
    //    break;
    }

    return FALSE;
}

static BOOLEAN PhpOptionsSettingsCallback(
    _In_ PPH_SETTING Setting,
    _In_ PVOID Context
    )
{
    INT lvItemIndex;

    lvItemIndex = PhAddListViewItem(Context, MAXINT, Setting->Name.Buffer, Setting);

    switch (Setting->Type)
    {
    case StringSettingType:
        PhSetListViewSubItem(Context, lvItemIndex, 1, L"String");
        break;
    case IntegerSettingType:
        PhSetListViewSubItem(Context, lvItemIndex, 1, L"Integer");
        break;
    case IntegerPairSettingType:
        PhSetListViewSubItem(Context, lvItemIndex, 1, L"IntegerPair");
        break;
    case ScalableIntegerPairSettingType:
        PhSetListViewSubItem(Context, lvItemIndex, 1, L"ScalableIntegerPair");
        break;
    }

    PhSetListViewSubItem(Context, lvItemIndex, 2, PH_AUTO_T(PH_STRING, PhSettingToString(Setting->Type, Setting))->Buffer);
    PhSetListViewSubItem(Context, lvItemIndex, 3, Setting->DefaultValue.Buffer);

    return TRUE;
}

static INT_PTR CALLBACK PhpOptionsAdvancedEditDlgProc(
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
            PPH_SETTING setting = (PPH_SETTING)lParam;

            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)PH_LOAD_SHARED_ICON_SMALL(PhInstanceHandle, MAKEINTRESOURCE(IDI_PROCESSHACKER)));
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)PH_LOAD_SHARED_ICON_LARGE(PhInstanceHandle, MAKEINTRESOURCE(IDI_PROCESSHACKER)));

            PhSetWindowText(hwndDlg, L"Setting Editor");
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, setting);

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_NAME), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_VALUE), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            PhSetDialogItemText(hwndDlg, IDC_NAME, setting->Name.Buffer);
            PhSetDialogItemText(hwndDlg, IDC_VALUE, PH_AUTO_T(PH_STRING, PhSettingToString(setting->Type, setting))->Buffer);

            EnableWindow(GetDlgItem(hwndDlg, IDC_NAME), FALSE);

            PhSetDialogFocus(hwndDlg, GetDlgItem(hwndDlg, IDCANCEL));
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

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
            case IDCANCEL:
                EndDialog(hwndDlg, IDCANCEL);
                break;
            case IDOK:
                {
                    PPH_SETTING setting = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
                    PPH_STRING settingValue = PH_AUTO(PhGetWindowText(GetDlgItem(hwndDlg, IDC_VALUE)));

                    if (!PhSettingFromString(
                        setting->Type,
                        &settingValue->sr,
                        settingValue,
                        setting
                        ))
                    {
                        PhSettingFromString(
                            setting->Type,
                            &setting->DefaultValue,
                            NULL,
                            setting
                            );
                    }

                    EndDialog(hwndDlg, IDOK);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PhpOptionsAdvancedDlgProc(
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
            HWND listviewHandle;

            listviewHandle = GetDlgItem(hwndDlg, IDC_SETTINGS);

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, listviewHandle, NULL, PH_ANCHOR_ALL);

            PhSetListViewStyle(listviewHandle, FALSE, TRUE);
            PhSetControlTheme(listviewHandle, L"explorer");
            PhAddListViewColumn(listviewHandle, 0, 0, 0, LVCFMT_LEFT, 180, L"Name");
            PhAddListViewColumn(listviewHandle, 1, 1, 1, LVCFMT_LEFT, 80, L"Type");
            PhAddListViewColumn(listviewHandle, 2, 2, 2, LVCFMT_LEFT, 80, L"Value");
            PhAddListViewColumn(listviewHandle, 3, 3, 3, LVCFMT_LEFT, 80, L"Default");
            PhSetExtendedListView(listviewHandle);

            PhEnumSettings(PhpOptionsSettingsCallback, listviewHandle);
            ExtendedListView_SortItems(listviewHandle);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&LayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case NM_DBLCLK:
                {
                    if (header->idFrom == IDC_SETTINGS)
                    {
                        PPH_SETTING setting;
                        INT index;

                        if (setting = PhGetSelectedListViewItemParam(header->hwndFrom))
                        {
                            DialogBoxParam(
                                PhInstanceHandle,
                                MAKEINTRESOURCE(IDD_EDITENV),
                                hwndDlg,
                                PhpOptionsAdvancedEditDlgProc,
                                (LPARAM)setting
                                );

                            if ((index = PhFindListViewItemByFlags(header->hwndFrom, -1, LVNI_SELECTED)) != -1)
                            {
                                PhSetListViewSubItem(header->hwndFrom, index, 2, PH_AUTO_T(PH_STRING, PhSettingToString(setting->Type, setting))->Buffer);
                            }
                        }
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

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
    COLOR_ITEM(L"ColorInheritHandles", L"Inheritable handles", L"Handles that can be inherited by child processes."),

    COLOR_ITEM(L"ColorHandleFiltered", L"Filtered processes", L"Processes that are protected by handle object callbacks."),
    COLOR_ITEM(L"ColorUnknown", L"Untrusted DLLs and Services", L"Services and DLLs which are not digitally signed."),
    COLOR_ITEM(L"ColorServiceDisabled", L"Disabled Services", L"Services which have been disabled."),
    //COLOR_ITEM(L"ColorServiceStop", L"Stopped Services", L"Services that are not running.")
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

UINT_PTR CALLBACK PhpColorDlgHookProc(
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
            PhCenterWindow(hwndDlg, PhOptionsWindowHandle);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    }

    return FALSE;
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
            PhSetDialogItemValue(hwndDlg, IDC_HIGHLIGHTINGDURATION, PhCsHighlightingDuration, FALSE);

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

            for (ULONG i = 0; i < RTL_NUMBER_OF(ColorItems); i++)
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
            PH_SET_INTEGER_CACHED_SETTING(HighlightingDuration, PhGetDialogItemValue(hwndDlg, IDC_HIGHLIGHTINGDURATION));
            PH_SET_INTEGER_CACHED_SETTING(ColorNew, ColorBox_GetColor(GetDlgItem(hwndDlg, IDC_NEWOBJECTS)));
            PH_SET_INTEGER_CACHED_SETTING(ColorRemoved, ColorBox_GetColor(GetDlgItem(hwndDlg, IDC_REMOVEDOBJECTS)));

            for (ULONG i = 0; i < RTL_NUMBER_OF(ColorItems); i++)
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
                    for (ULONG i = 0; i < RTL_NUMBER_OF(ColorItems); i++)
                        ListView_SetCheckState(HighlightingListViewHandle, i, TRUE);
                }
                break;
            case IDC_DISABLEALL:
                {
                    for (ULONG i = 0; i < RTL_NUMBER_OF(ColorItems); i++)
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
                            chooseColor.lpfnHook = PhpColorDlgHookProc;
                            chooseColor.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_SOLIDCOLOR | CC_ENABLEHOOK | CC_RGBINIT;

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
