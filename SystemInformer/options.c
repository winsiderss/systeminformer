/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2022
 *
 */

#include <phapp.h>

#include <commdlg.h>
#include <colmgr.h>
#include <colorbox.h>
#include <cpysave.h>
#include <settings.h>
#include <emenu.h>

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
    _In_opt_ PVOID Parameter
    );

PPH_OPTIONS_SECTION PhOptionsCreateSectionAdvanced(
    _In_ PWSTR Name,
    _In_ PVOID Instance,
    _In_ PWSTR Template,
    _In_ DLGPROC DialogProc,
    _In_opt_ PVOID Parameter
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
static HFONT CurrentFontMonospaceInstance = NULL;
static PPH_STRING NewFontSelection = NULL;
static PPH_STRING NewFontMonospaceSelection = NULL;
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
            PhOptionsWindowHandle = PhCreateDialog(
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_OPTIONS),
                NULL,
                PhOptionsDialogProc,
                NULL
                );

            PhRegisterDialog(PhOptionsWindowHandle);
            ShowWindow(PhOptionsWindowHandle, SW_SHOW);
        }

        if (IsMinimized(PhOptionsWindowHandle))
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

        if (generalSection && advancedSection && !advancedSection->TreeItemHandle)
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

static VOID PhpOptionsSetImageList(
    _In_ HWND WindowHandle,
    _Inout_ HIMAGELIST* Imagelist,
    _In_ BOOLEAN Treeview
    )
{
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(WindowHandle);

    *Imagelist = PhImageListCreate(2, PhGetDpi(22, dpiValue), ILC_MASK | ILC_COLOR32, 1, 1);

    if (Treeview)
        TreeView_SetImageList(WindowHandle, *Imagelist, TVSIL_NORMAL);
    else
        ListView_SetImageList(WindowHandle, *Imagelist, LVSIL_SMALL);
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
            OptionsTreeControl = GetDlgItem(hwndDlg, IDC_SECTIONTREE);
            ContainerControl = GetDlgItem(hwndDlg, IDD_CONTAINER);

            PhSetApplicationWindowIcon(hwndDlg);

            PhpOptionsSetImageList(OptionsTreeControl, &OptionsTreeImageList, TRUE);

            //PhSetWindowStyle(GetDlgItem(hwndDlg, IDC_SEPARATOR), SS_OWNERDRAW, SS_OWNERDRAW);
            PhSetControlTheme(OptionsTreeControl, L"explorer");
            TreeView_SetExtendedStyle(OptionsTreeControl, TVS_EX_DOUBLEBUFFER, TVS_EX_DOUBLEBUFFER);
            TreeView_SetBkColor(OptionsTreeControl, GetSysColor(COLOR_3DFACE));

            PhInitializeLayoutManager(&WindowLayoutManager, hwndDlg);
            PhAddLayoutItem(&WindowLayoutManager, OptionsTreeControl, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_BOTTOM);
            //PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_SEPARATOR), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, ContainerControl, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_RESET), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_CLEANUP), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            //PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_APPLY), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            PhRegisterWindowCallback(hwndDlg, PH_PLUGIN_WINDOW_EVENT_TYPE_TOPMOST, NULL);

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
                PhOptionsCreateSection(L"Plugins", PhInstanceHandle, MAKEINTRESOURCE(IDD_PLUGINS), PhPluginsDlgProc, NULL);

                if (PhPluginsEnabled)
                {
                    PH_PLUGIN_OPTIONS_POINTERS pointers;

                    pointers.WindowHandle = hwndDlg;
                    pointers.CreateSection = PhOptionsCreateSection;
                    pointers.FindSection = PhOptionsFindSection;
                    pointers.EnterSectionView = PhOptionsEnterSectionView;

                    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackOptionsWindowInitializing), &pointers);
                }

                TreeView_SelectItem(OptionsTreeControl, section->TreeItemHandle);
                SetFocus(OptionsTreeControl);
                //PhOptionsEnterSectionView(section);
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

            PhUnregisterWindowCallback(hwndDlg);

            PhUnregisterDialog(PhOptionsWindowHandle);
            PhOptionsWindowHandle = NULL;
        }
        break;
    case WM_DPICHANGED:
        {
            PhpOptionsSetImageList(OptionsTreeControl, &OptionsTreeImageList, TRUE);
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
                        L"Do you want to reset all settings and restart System Informer?",
                        L""
                        ) == IDYES)
                    {
                        ProcessHacker_PrepareForEarlyShutdown();

                        PhResetSettings(hwndDlg);

                        if (!PhIsNullOrEmptyString(PhSettingsFileName))
                            PhSaveSettings(&PhSettingsFileName->sr);

                        PhShellProcessHacker(
                            PhMainWndHandle,
                            L"-v -newinstance",
                            SW_SHOW,
                            0,
                            PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
                            0,
                            NULL
                            );
                        ProcessHacker_Destroy();
                    }
                }
                break;
            case IDC_CLEANUP:
                {
                    if (PhShowMessage2(
                        hwndDlg,
                        TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
                        TD_INFORMATION_ICON,
                        L"Do you want to clean up unused settings?",
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

            //if (drawInfo->CtlID == IDC_SEPARATOR)
            //{
            //    RECT rect;
            //
            //    rect = drawInfo->rcItem;
            //    rect.right = 2;
            //
            //    if (PhEnableThemeSupport)
            //    {
            //        switch (PhCsGraphColorMode)
            //        {
            //        case 0: // New colors
            //            {
            //                FillRect(drawInfo->hDC, &rect, GetSysColorBrush(COLOR_3DHIGHLIGHT));
            //                rect.left += 1;
            //                FillRect(drawInfo->hDC, &rect, GetSysColorBrush(COLOR_3DSHADOW));
            //            }
            //            break;
            //        case 1: // Old colors
            //            {
            //                SetDCBrushColor(drawInfo->hDC, RGB(0, 0, 0));
            //                FillRect(drawInfo->hDC, &rect, GetStockBrush(DC_BRUSH));
            //            }
            //            break;
            //        }
            //    }
            //    else
            //    {
            //        FillRect(drawInfo->hDC, &rect, GetSysColorBrush(COLOR_3DHIGHLIGHT));
            //        rect.left += 1;
            //        FillRect(drawInfo->hDC, &rect, GetSysColorBrush(COLOR_3DSHADOW));
            //    }
            //
            //    return TRUE;
            //}
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
    _In_opt_ PVOID Parameter
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
    _In_opt_ PVOID Parameter
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

_Success_(return)
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

_Success_(return)
static BOOLEAN GetCurrentFontMonospace(
    _Out_ PLOGFONT Font
    )
{
    BOOLEAN result;
    PPH_STRING fontHexString;

    if (NewFontMonospaceSelection)
        fontHexString = NewFontMonospaceSelection;
    else
        fontHexString = PhaGetStringSetting(L"FontMonospace");

    if (fontHexString->Length / sizeof(WCHAR) / 2 == sizeof(LOGFONT))
        result = PhHexStringToBuffer(&fontHexString->sr, (PUCHAR)Font);
    else
        result = FALSE;

    return result;
}

typedef struct _PHP_HKURUN_ENTRY
{
    PPH_STRING Value;
    //PPH_STRING Name;
} PHP_HKURUN_ENTRY, *PPHP_HKURUN_ENTRY;

BOOLEAN NTAPI PhpReadCurrentRunCallback(
    _In_ HANDLE RootDirectory,
    _In_ PKEY_VALUE_FULL_INFORMATION Information,
    _In_opt_ PVOID Context
    )
{
    if (Context && Information->Type == REG_SZ)
    {
        PHP_HKURUN_ENTRY entry;

        if (Information->DataLength > sizeof(UNICODE_NULL))
            entry.Value = PhCreateStringEx(PTR_ADD_OFFSET(Information, Information->DataOffset), Information->DataLength);
        else
            entry.Value = PhReferenceEmptyString();

        //entry.Name = PhCreateStringEx(Information->Name, Information->NameLength);

        PhAddItemArray(Context, &entry);
    }

    return TRUE;
}

static VOID ReadCurrentUserRun(
    VOID
    )
{
    HANDLE keyHandle;

    CurrentUserRunPresent = FALSE;

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_CURRENT_USER,
        &CurrentUserRunKeyName,
        0
        )))
    {
        PH_ARRAY keyEntryArray;

        PhInitializeArray(&keyEntryArray, sizeof(PHP_HKURUN_ENTRY), 20);
        PhEnumerateValueKey(keyHandle, KeyValueFullInformation, PhpReadCurrentRunCallback, &keyEntryArray);

        for (SIZE_T i = 0; i < keyEntryArray.Count; i++)
        {
            PPHP_HKURUN_ENTRY entry = PhItemArray(&keyEntryArray, i);
            PH_STRINGREF fileName;
            PH_STRINGREF arguments;
            PPH_STRING fullFileName;
            PPH_STRING applicationFileName;

            if (PhParseCommandLineFuzzy(&entry->Value->sr, &fileName, &arguments, &fullFileName))
            {
                if (applicationFileName = PhGetApplicationFileNameWin32())
                {
                    PhMoveReference(&applicationFileName, PhGetBaseName(applicationFileName));

                    if (fullFileName && PhEndsWithString(fullFileName, applicationFileName, TRUE))
                    {
                        CurrentUserRunPresent = TRUE;
                    }

                    PhDereferenceObject(applicationFileName);
                }

                if (fullFileName) PhDereferenceObject(fullFileName);
            }

            PhDereferenceObject(entry->Value);
        }

        PhDeleteArray(&keyEntryArray);

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
        static PH_STRINGREF valueName = PH_STRINGREF_INIT(L"System Informer");
        static PH_STRINGREF seperator = PH_STRINGREF_INIT(L"\"");

        if (Present)
        {
            PPH_STRING value;
            PPH_STRING fileName;

            if (fileName = PhGetApplicationFileNameWin32())
            {
                value = PH_AUTO(PhConcatStringRef3(&seperator, &fileName->sr, &seperator));

                if (StartHidden)
                    value = PH_AUTO(PhConcatStringRefZ(&value->sr, L" -hide"));

                PhSetValueKey(
                    keyHandle,
                    &valueName,
                    REG_SZ,
                    value->Buffer,
                    (ULONG)value->Length + sizeof(UNICODE_NULL)
                    );

                PhDereferenceObject(fileName);
            }
        }
        else
        {
            PhDeleteValueKey(keyHandle, &valueName);
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

    if (!(fileName = PhGetApplicationFileNameWin32()))
        return FALSE;

    if (PhEqualString(OldTaskMgrDebugger, fileName, TRUE))
    {
        match = TRUE;
    }
    // Allow for a quoted value.
    else if (
        OldTaskMgrDebugger->Length == fileName->Length + 4 &&
        OldTaskMgrDebugger->Buffer[0] == L'"' &&
        OldTaskMgrDebugger->Buffer[OldTaskMgrDebugger->Length / sizeof(WCHAR) - 1] == L'"'
        )
    {
        PH_STRINGREF partInside;

        partInside.Buffer = &OldTaskMgrDebugger->Buffer[1];
        partInside.Length = OldTaskMgrDebugger->Length - 2 * sizeof(WCHAR);

        if (PhEqualStringRef(&partInside, &fileName->sr, TRUE))
            match = TRUE;
    }

    PhDereferenceObject(fileName);

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
        message = L"Do you want to make System Informer the default Windows Task Manager?";

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
            static PH_STRINGREF valueName = PH_STRINGREF_INIT(L"Debugger");
            static PH_STRINGREF seperator = PH_STRINGREF_INIT(L"\"");

            if (PhpIsDefaultTaskManager())
            {
                status = PhDeleteValueKey(taskmgrKeyHandle, &valueName);
            }
            else
            {
                PPH_STRING quotedFileName;
                PPH_STRING applicationFileName;

                if (applicationFileName = PhGetApplicationFileNameWin32())
                {
                    quotedFileName = PH_AUTO(PhConcatStringRef3(&seperator, &applicationFileName->sr, &seperator));

                    status = PhSetValueKey(
                        taskmgrKeyHandle,
                        &valueName,
                        REG_SZ,
                        quotedFileName->Buffer,
                        (ULONG)quotedFileName->Length + sizeof(UNICODE_NULL)
                        );

                    PhDereferenceObject(applicationFileName);
                }
            }

            NtClose(taskmgrKeyHandle);
        }

        if (!NT_SUCCESS(status))
            PhShowStatus(ParentWindowHandle, L"Unable to replace Task Manager", status, 0);

        if (!PhIsNullOrEmptyString(PhSettingsFileName))
            PhSaveSettings(&PhSettingsFileName->sr);
    }
}

BOOLEAN PhpIsExploitProtectionEnabled(
    VOID
    )
{
    BOOLEAN enabled = FALSE;
    HANDLE keyHandle;
    PPH_STRING directory = NULL;
    PPH_STRING fileName = NULL;
    PPH_STRING keyName;

    PhMoveReference(&directory, PhCreateString2(&TaskMgrImageOptionsKeyName));
    PhMoveReference(&directory, PhGetBaseDirectory(directory));
    PhMoveReference(&fileName, PhGetApplicationFileNameWin32());
    PhMoveReference(&fileName, PhGetBaseName(fileName));
    keyName = PhConcatStringRef3(&directory->sr, &PhNtPathSeperatorString, &fileName->sr);

    if (NT_SUCCESS(PhOpenKey(
        &keyHandle,
        KEY_READ,
        PH_KEY_LOCAL_MACHINE,
        &keyName->sr,
        0
        )))
    {
        PH_STRINGREF valueName;
        PKEY_VALUE_PARTIAL_INFORMATION buffer;

        enabled = !(PhQueryRegistryUlong64(keyHandle, L"MitigationOptions") == ULLONG_MAX);
        PhInitializeStringRef(&valueName, L"MitigationOptions");

        if (NT_SUCCESS(PhQueryValueKey(keyHandle, &valueName, KeyValuePartialInformation, &buffer)))
        {
            if (buffer->Type == REG_BINARY && buffer->DataLength)
            {
                enabled = TRUE;
            }

            PhFree(buffer);
        }

        NtClose(keyHandle);
    }

    PhDereferenceObject(keyName);
    PhDereferenceObject(fileName);
    PhDereferenceObject(directory);

    return enabled;
}

NTSTATUS PhpSetExploitProtectionEnabled(
    _In_ BOOLEAN Enabled)
{
    static PH_STRINGREF replacementToken = PH_STRINGREF_INIT(L"Software\\");
    static PH_STRINGREF wow6432Token = PH_STRINGREF_INIT(L"Software\\WOW6432Node\\");
    static PH_STRINGREF valueName = PH_STRINGREF_INIT(L"MitigationOptions");
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    HANDLE keyHandle;
    PPH_STRING directory;
    PPH_STRING fileName;
    PPH_STRING keyName;

    directory = PH_AUTO(PhCreateString2(&TaskMgrImageOptionsKeyName));
    directory = PH_AUTO(PhGetBaseDirectory(directory));
    fileName = PH_AUTO(PhGetApplicationFileNameWin32());
    fileName = PH_AUTO(PhGetBaseName(fileName));
    keyName = PH_AUTO(PhConcatStringRef3(&directory->sr, &PhNtPathSeperatorString, &fileName->sr));

    if (Enabled)
    {
        status = PhCreateKey(
            &keyHandle,
            KEY_WRITE,
            PH_KEY_LOCAL_MACHINE,
            &keyName->sr,
            OBJ_OPENIF,
            0,
            NULL
            );

        if (NT_SUCCESS(status))
        {
            status = PhSetValueKey(keyHandle, &valueName, REG_QWORD, &(ULONG64)
            {
                PROCESS_CREATION_MITIGATION_POLICY_HEAP_TERMINATE_ALWAYS_ON |
                PROCESS_CREATION_MITIGATION_POLICY_BOTTOM_UP_ASLR_ALWAYS_ON |
                PROCESS_CREATION_MITIGATION_POLICY_HIGH_ENTROPY_ASLR_ALWAYS_ON |
                PROCESS_CREATION_MITIGATION_POLICY_EXTENSION_POINT_DISABLE_ALWAYS_ON |
                PROCESS_CREATION_MITIGATION_POLICY_PROHIBIT_DYNAMIC_CODE_ALWAYS_ON |
                PROCESS_CREATION_MITIGATION_POLICY_CONTROL_FLOW_GUARD_ALWAYS_ON |
                PROCESS_CREATION_MITIGATION_POLICY_FONT_DISABLE_ALWAYS_ON |
                PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_NO_REMOTE_ALWAYS_ON |
                PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_NO_LOW_LABEL_ALWAYS_ON |
                PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_PREFER_SYSTEM32_ALWAYS_ON,
            }, sizeof(ULONG64));

            NtClose(keyHandle);
        }
    }
    else
    {
        status = PhOpenKey(
            &keyHandle,
            KEY_WRITE,
            PH_KEY_LOCAL_MACHINE,
            &keyName->sr,
            OBJ_OPENIF
            );

        if (NT_SUCCESS(status))
        {
            status = PhDeleteValueKey(keyHandle, &valueName);
            NtClose(keyHandle);
        }

        if (status == STATUS_OBJECT_NAME_NOT_FOUND)
            status = STATUS_SUCCESS;

#ifdef _WIN64
        if (NT_SUCCESS(status))
        {
            PH_STRINGREF stringBefore;
            PH_STRINGREF stringAfter;

            if (PhSplitStringRefAtString(&keyName->sr, &replacementToken, TRUE, &stringBefore, &stringAfter))
            {
                keyName = PH_AUTO(PhConcatStringRef3(&stringBefore, &wow6432Token, &stringAfter));

                status = PhOpenKey(
                    &keyHandle,
                    DELETE,
                    PH_KEY_LOCAL_MACHINE,
                    &keyName->sr,
                    OBJ_OPENIF
                    );

                if (NT_SUCCESS(status))
                {
                    status = NtDeleteKey(keyHandle);
                    NtClose(keyHandle);
                }
            }
        }
#endif
    }

    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
        status = STATUS_SUCCESS;

    return status;
}

NTSTATUS PhpSetSilentProcessNotifyEnabled(
    _In_ BOOLEAN Enabled)
{
    static PH_STRINGREF processExitKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\SilentProcessExit");
    static PH_STRINGREF valueModeName = PH_STRINGREF_INIT(L"ReportingMode");
    //static PH_STRINGREF valueSelfName = PH_STRINGREF_INIT(L"IgnoreSelfExits");
    //static PH_STRINGREF valueMonitorName = PH_STRINGREF_INIT(L"MonitorProcess");
    static PH_STRINGREF valueGlobalName = PH_STRINGREF_INIT(L"GlobalFlag");
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    HANDLE keyDirectoryHandle = NULL;
    HANDLE keyFilenameHandle = NULL;
    PPH_STRING filename;
    PPH_STRING baseName;

    filename = PH_AUTO(PhGetApplicationFileNameWin32());
    baseName = PH_AUTO(PhGetBaseName(filename));

    if (Enabled)
    {
        status = PhCreateKey(
            &keyDirectoryHandle,
            KEY_WRITE,
            PH_KEY_LOCAL_MACHINE,
            &processExitKeyName,
            OBJ_OPENIF,
            0,
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhCreateKey(
            &keyFilenameHandle,
            KEY_WRITE,
            keyDirectoryHandle,
            &baseName->sr,
            OBJ_OPENIF,
            0,
            NULL
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhSetValueKey(
            keyFilenameHandle,
            &valueModeName,
            REG_DWORD,
            &(ULONG){ 4 },
            sizeof(ULONG)
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        //PhSetValueKey(keyFilenameHandle, &valueSelfName, REG_DWORD, &(ULONG){ 1 }, sizeof(ULONG));
        //PhSetValueKey(keyFilenameHandle, &valueMonitorName, REG_SZ, filename->Buffer, (ULONG)filename->Length + sizeof(UNICODE_NULL));

        if (NT_SUCCESS(status))
        {
            HANDLE keyHandle;
            PPH_STRING directory;
            PPH_STRING fileName;
            PPH_STRING keyName;

            directory = PH_AUTO(PhCreateString2(&TaskMgrImageOptionsKeyName));
            directory = PH_AUTO(PhGetBaseDirectory(directory));
            fileName = PH_AUTO(PhGetApplicationFileNameWin32());
            fileName = PH_AUTO(PhGetBaseName(fileName));
            keyName = PH_AUTO(PhConcatStringRef3(&directory->sr, &PhNtPathSeperatorString, &fileName->sr));

            status = PhCreateKey(
                &keyHandle,
                KEY_WRITE,
                PH_KEY_LOCAL_MACHINE,
                &keyName->sr,
                OBJ_OPENIF,
                0,
                NULL
                );

            if (NT_SUCCESS(status))
            {
                ULONG globalFlags = PhQueryRegistryUlong(keyHandle, L"GlobalFlag");

                if (globalFlags == ULONG_MAX) globalFlags = 0;
                globalFlags = globalFlags | FLG_MONITOR_SILENT_PROCESS_EXIT;

                status = PhSetValueKey(keyHandle, &valueGlobalName, REG_DWORD, &globalFlags, sizeof(ULONG));
                NtClose(keyHandle);
            }
        }
    }
    else
    {
        if (NT_SUCCESS(PhOpenKey(
            &keyDirectoryHandle,
            DELETE,
            PH_KEY_LOCAL_MACHINE,
            &processExitKeyName,
            0
            )))
        {
            if (NT_SUCCESS(PhOpenKey(
                &keyFilenameHandle,
                DELETE,
                keyDirectoryHandle,
                &baseName->sr,
                0
                )))
            {
                NtDeleteKey(keyFilenameHandle);
            }
        }

        {
            HANDLE keyHandle;
            PPH_STRING directory;
            PPH_STRING fileName;
            PPH_STRING keyName;

            directory = PH_AUTO(PhCreateString2(&TaskMgrImageOptionsKeyName));
            directory = PH_AUTO(PhGetBaseDirectory(directory));
            fileName = PH_AUTO(PhGetApplicationFileNameWin32());
            fileName = PH_AUTO(PhGetBaseName(fileName));
            keyName = PH_AUTO(PhConcatStringRef3(&directory->sr, &PhNtPathSeperatorString, &fileName->sr));

            status = PhOpenKey(
                &keyHandle,
                KEY_WRITE,
                PH_KEY_LOCAL_MACHINE,
                &keyName->sr,
                0
                );

            if (NT_SUCCESS(status))
            {
                status = PhDeleteValueKey(keyHandle, &valueGlobalName);
                NtClose(keyHandle);
            }
        }
    }

CleanupExit:
    if (keyDirectoryHandle)
        NtClose(keyDirectoryHandle);
    if (keyFilenameHandle)
        NtClose(keyFilenameHandle);

    return status;
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
        PhSetWindowText(GetDlgItem(WindowHandle, IDC_DEFSTATE), L"System Informer is the default Task Manager:");
        PhSetWindowText(GetDlgItem(WindowHandle, IDC_REPLACETASKMANAGER), L"Restore default...");
    }
    else
    {
        PhSetWindowText(GetDlgItem(WindowHandle, IDC_DEFSTATE), L"System Informer is not the default Task Manager:");
        PhSetWindowText(GetDlgItem(WindowHandle, IDC_REPLACETASKMANAGER), L"Make default...");
    }
}

typedef enum _PHP_OPTIONS_INDEX
{
    PHP_OPTIONS_INDEX_SINGLE_INSTANCE,
    PHP_OPTIONS_INDEX_HIDE_WHENCLOSED,
    PHP_OPTIONS_INDEX_HIDE_WHENMINIMIZED,
    PHP_OPTIONS_INDEX_START_ATLOGON,
    PHP_OPTIONS_INDEX_START_HIDDEN,
    PHP_OPTIONS_INDEX_ENABLE_WARNINGS,
    PHP_OPTIONS_INDEX_ENABLE_DRIVER,
    PHP_OPTIONS_INDEX_ENABLE_MITIGATION,
    PHP_OPTIONS_INDEX_ENABLE_MONOSPACE,
    PHP_OPTIONS_INDEX_ENABLE_PLUGINS,
    PHP_OPTIONS_INDEX_ENABLE_UNDECORATE_SYMBOLS,
    PHP_OPTIONS_INDEX_ENABLE_COLUMN_HEADER_TOTALS,
    PHP_OPTIONS_INDEX_ENABLE_CYCLE_CPU_USAGE,
    PHP_OPTIONS_INDEX_ENABLE_GRAPH_SCALING,
    PHP_OPTIONS_INDEX_ENABLE_MINIINFO_WINDOW,
    PHP_OPTIONS_INDEX_ENABLE_LASTTAB_SUPPORT,
    PHP_OPTIONS_INDEX_ENABLE_THEME_SUPPORT,
    PHP_OPTIONS_INDEX_ENABLE_START_ASADMIN,
    PHP_OPTIONS_INDEX_ENABLE_SILENT_CRASH_NOTIFY,
    PHP_OPTIONS_INDEX_ENABLE_NETWORK_RESOLVE,
    PHP_OPTIONS_INDEX_ENABLE_NETWORK_RESOLVE_DOH,
    PHP_OPTIONS_INDEX_ENABLE_INSTANT_TOOLTIPS,
    PHP_OPTIONS_INDEX_ENABLE_IMAGE_COHERENCY,
    PHP_OPTIONS_INDEX_ENABLE_STAGE2,
    PHP_OPTIONS_INDEX_ENABLE_SERVICE_STAGE2,
    PHP_OPTIONS_INDEX_ICON_SINGLE_CLICK,
    PHP_OPTIONS_INDEX_ICON_TOGGLE_VISIBILITY,
    PHP_OPTIONS_INDEX_PROPAGATE_CPU_USAGE,
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
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_WARNINGS, L"Enable warnings", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_DRIVER, L"Enable kernel-mode driver", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_MITIGATION, L"Enable mitigation policy", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_MONOSPACE, L"Enable monospace fonts", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_PLUGINS, L"Enable plugins", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_UNDECORATE_SYMBOLS, L"Enable undecorated symbols", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_COLUMN_HEADER_TOTALS, L"Enable column header totals (experimental)", NULL);
#ifdef _ARM64_
    // see: PhpEstimateIdleCyclesForARM (jxy-s)
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_CYCLE_CPU_USAGE, L"Enable cycle-based CPU usage (experimental)", NULL);
#else
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_CYCLE_CPU_USAGE, L"Enable cycle-based CPU usage", NULL);
#endif
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_GRAPH_SCALING, L"Enable fixed graph scaling (experimental)", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_MINIINFO_WINDOW, L"Enable tray information window", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_LASTTAB_SUPPORT, L"Remember last selected window", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_THEME_SUPPORT, L"Enable theme support (experimental)", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_START_ASADMIN, L"Enable start as admin (experimental)", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_SILENT_CRASH_NOTIFY, L"Enable silent crash notification (experimental)", NULL);
    //PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_LINUX_SUPPORT, L"Enable Windows subsystem for Linux support", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_NETWORK_RESOLVE, L"Resolve network addresses", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_NETWORK_RESOLVE_DOH, L"Resolve DNS over HTTPS (DoH)", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_INSTANT_TOOLTIPS, L"Show tooltips instantly", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_IMAGE_COHERENCY, L"Check images for coherency", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_STAGE2, L"Check images for digital signatures", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_SERVICE_STAGE2, L"Check services for digital signatures", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ICON_SINGLE_CLICK, L"Single-click tray icons", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ICON_TOGGLE_VISIBILITY, L"Icon click toggles visibility", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_PROPAGATE_CPU_USAGE, L"Include usage of collapsed processes", NULL);
    PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_SHOW_ADVANCED_OPTIONS, L"Show advanced options", NULL);

    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_SINGLE_INSTANCE, L"AllowOnlyOneInstance");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_HIDE_WHENCLOSED, L"HideOnClose");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_HIDE_WHENMINIMIZED, L"HideOnMinimize");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_START_HIDDEN, L"StartHidden");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_MINIINFO_WINDOW, L"MiniInfoWindowEnabled");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_LASTTAB_SUPPORT, L"MainWindowTabRestoreEnabled");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_DRIVER, L"EnableKph");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_WARNINGS, L"EnableWarnings");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_PLUGINS, L"EnablePlugins");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_UNDECORATE_SYMBOLS, L"DbgHelpUndecorate");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_COLUMN_HEADER_TOTALS, L"TreeListEnableHeaderTotals");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_GRAPH_SCALING, L"EnableGraphMaxScale");
#ifdef _ARM64_
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_CYCLE_CPU_USAGE, L"EnableArmCycleCpuUsage");
#else
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_CYCLE_CPU_USAGE, L"EnableCycleCpuUsage");
#endif
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_THEME_SUPPORT, L"EnableThemeSupport");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_START_ASADMIN, L"EnableStartAsAdmin");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_SILENT_CRASH_NOTIFY, L"EnableSilentCrashNotify");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_MONOSPACE, L"EnableMonospaceFont");
    //SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_LINUX_SUPPORT, L"EnableLinuxSubsystemSupport");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_NETWORK_RESOLVE, L"EnableNetworkResolve");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_NETWORK_RESOLVE_DOH, L"EnableNetworkResolveDoH");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_INSTANT_TOOLTIPS, L"EnableInstantTooltips");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_IMAGE_COHERENCY, L"EnableImageCoherencySupport");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_STAGE2, L"EnableStage2");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_SERVICE_STAGE2, L"EnableServiceStage2");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ICON_SINGLE_CLICK, L"IconSingleClick");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ICON_TOGGLE_VISIBILITY, L"IconTogglesVisibility");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_PROPAGATE_CPU_USAGE, L"PropagateCpuUsage");
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_SHOW_ADVANCED_OPTIONS, L"EnableAdvancedOptions");

    if (CurrentUserRunPresent)
        ListView_SetCheckState(listViewHandle, PHP_OPTIONS_INDEX_START_ATLOGON, TRUE);
    if (PhpIsExploitProtectionEnabled())
        ListView_SetCheckState(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_MITIGATION, TRUE);
    if (PhGetIntegerSetting(L"EnableAdvancedOptions"))
        PhpOptionsShowHideTreeViewItem(FALSE);
}

static VOID PhpOptionsNotifyChangeCallback(
    _In_ PVOID Context
    )
{
    PhUpdateCachedSettings();
    ProcessHacker_SaveAllSettings();
    PhInvalidateAllProcessNodes();
    PhReloadSettingsProcessTreeList();
    PhSiNotifyChangeSettings();

    //PhReInitializeWindowTheme(PhMainWndHandle);

    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackSettingsUpdated), NULL);

    if (RestartRequired)
    {
        if (PhShowMessage2(
            PhMainWndHandle,
            TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
            TD_INFORMATION_ICON,
            L"One or more options you have changed requires a restart of System Informer.",
            L"Do you want to restart System Informer now?"
            ) == IDYES)
        {
            ProcessHacker_PrepareForEarlyShutdown();
            PhShellProcessHacker(
                PhMainWndHandle,
                L"-v -newinstance",
                SW_SHOW,
                0,
                PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
                0,
                NULL
                );
            ProcessHacker_Destroy();
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
        PhSetStringSetting2(L"DbgHelpSearchPath", &PhaGetDlgItemText(hwndDlg, IDC_DBGHELPSEARCHPATH)->sr);
        RestartRequired = TRUE;
    }

    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_SINGLE_INSTANCE, L"AllowOnlyOneInstance");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_HIDE_WHENCLOSED, L"HideOnClose");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_HIDE_WHENMINIMIZED, L"HideOnMinimize");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_START_HIDDEN, L"StartHidden");
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_MINIINFO_WINDOW, L"MiniInfoWindowEnabled");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_LASTTAB_SUPPORT, L"MainWindowTabRestoreEnabled");
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_DRIVER, L"EnableKph");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_WARNINGS, L"EnableWarnings");
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_PLUGINS, L"EnablePlugins");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_UNDECORATE_SYMBOLS, L"DbgHelpUndecorate");
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_COLUMN_HEADER_TOTALS, L"TreeListEnableHeaderTotals");
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_GRAPH_SCALING, L"EnableGraphMaxScale");
#ifdef _ARM64_
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_CYCLE_CPU_USAGE, L"EnableArmCycleCpuUsage");
#else
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_CYCLE_CPU_USAGE, L"EnableCycleCpuUsage");
#endif
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_THEME_SUPPORT, L"EnableThemeSupport");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_START_ASADMIN, L"EnableStartAsAdmin");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_SILENT_CRASH_NOTIFY, L"EnableSilentCrashNotify");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_MONOSPACE, L"EnableMonospaceFont");
    //SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_LINUX_SUPPORT, L"EnableLinuxSubsystemSupport");
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_NETWORK_RESOLVE, L"EnableNetworkResolve");
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_NETWORK_RESOLVE_DOH, L"EnableNetworkResolveDoH");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_INSTANT_TOOLTIPS, L"EnableInstantTooltips");
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_IMAGE_COHERENCY, L"EnableImageCoherencySupport");
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_STAGE2, L"EnableStage2");
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_SERVICE_STAGE2, L"EnableServiceStage2");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ICON_SINGLE_CLICK, L"IconSingleClick");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ICON_TOGGLE_VISIBILITY, L"IconTogglesVisibility");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_PROPAGATE_CPU_USAGE, L"PropagateCpuUsage");
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_SHOW_ADVANCED_OPTIONS, L"EnableAdvancedOptions");

    if (PhGetIntegerSetting(L"EnableThemeSupport"))
    {
        PhSetIntegerSetting(L"GraphColorMode", 1); // HACK switch to dark theme. (dmex)
    }

    WriteCurrentUserRun(
        ListView_GetCheckState(listViewHandle, PHP_OPTIONS_INDEX_START_ATLOGON) == BST_CHECKED,
        ListView_GetCheckState(listViewHandle, PHP_OPTIONS_INDEX_START_HIDDEN) == BST_CHECKED
        );

    ProcessHacker_Invoke(PhpOptionsNotifyChangeCallback, NULL);
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
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
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
    static HWND ListViewHandle = NULL;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            HWND comboBoxHandle;
            ULONG i;
            LOGFONT font;

            comboBoxHandle = GetDlgItem(hwndDlg, IDC_MAXSIZEUNIT);
            ListViewHandle = GetDlgItem(hwndDlg, IDC_SETTINGS);

            PhpOptionsSetImageList(ListViewHandle, &GeneralListviewImageList, FALSE);

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_SEARCHENGINE), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_PEVIEWER), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&LayoutManager, ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDC_DBGHELPSEARCHPATH), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);

            PhSetListViewStyle(ListViewHandle, FALSE, TRUE);
            ListView_SetExtendedListViewStyleEx(ListViewHandle, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
            PhSetControlTheme(ListViewHandle, L"explorer");
            PhAddListViewColumn(ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 250, L"Name");
            PhSetExtendedListView(ListViewHandle);

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
                    SetWindowFont(OptionsTreeControl, CurrentFontInstance, TRUE); // HACK
                    SetWindowFont(ListViewHandle, CurrentFontInstance, TRUE);
                    SetWindowFont(GetDlgItem(hwndDlg, IDC_FONT), CurrentFontInstance, TRUE);
                }
            }

            if (GetCurrentFontMonospace(&font))
            {
                CurrentFontMonospaceInstance = CreateFontIndirect(&font);

                if (CurrentFontMonospaceInstance)
                {
                    SetWindowFont(GetDlgItem(hwndDlg, IDC_FONTMONOSPACE), CurrentFontMonospaceInstance, TRUE);
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
                ProcessHacker_UpdateFont();
            }

            if (NewFontMonospaceSelection)
            {
                PhSetStringSetting2(L"FontMonospace", &NewFontMonospaceSelection->sr);
                //ProcessHacker_UpdateFont();
            }

            PhpAdvancedPageSave(hwndDlg);

            if (CurrentFontInstance)
                DeleteFont(CurrentFontInstance);

            if (CurrentFontMonospaceInstance)
                DeleteFont(CurrentFontMonospaceInstance);

            PhClearReference(&NewFontSelection);
            PhClearReference(&NewFontMonospaceSelection);
            PhClearReference(&OldTaskMgrDebugger);

            PhDeleteLayoutManager(&LayoutManager);
        }
        break;
    case WM_DPICHANGED:
        {
            PhpOptionsSetImageList(ListViewHandle, &GeneralListviewImageList, FALSE);
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
                        GetObject(ProcessHacker_GetFont(), sizeof(LOGFONT), &font);
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
                        if (CurrentFontInstance) DeleteFont(CurrentFontInstance);
                        CurrentFontInstance = CreateFontIndirect(&font);

                        SetWindowFont(OptionsTreeControl, CurrentFontInstance, TRUE); // HACK
                        SetWindowFont(GetDlgItem(hwndDlg, IDC_SETTINGS), CurrentFontInstance, TRUE);
                        SetWindowFont(GetDlgItem(hwndDlg, IDC_FONT), CurrentFontInstance, TRUE);

                        // Re-add the listview items for the new font (dmex)
                        GeneralListViewStateInitializing = TRUE;
                        HWND listviewHandle = GetDlgItem(hwndDlg, IDC_SETTINGS);
                        ExtendedListView_SetRedraw(listviewHandle, FALSE);
                        ListView_DeleteAllItems(listviewHandle);
                        PhpAdvancedPageLoad(hwndDlg);
                        ExtendedListView_SetRedraw(listviewHandle, TRUE);
                        GeneralListViewStateInitializing = FALSE;

                        RestartRequired = TRUE; // HACK: Fix ToolStatus plugin toolbar resize on font change
                    }
                }
                break;
            case IDC_FONTMONOSPACE:
                {
                    LOGFONT font;
                    CHOOSEFONT chooseFont;

                    if (!GetCurrentFontMonospace(&font))
                    {
                        // Can't get LOGFONT from the existing setting, probably
                        // because the user hasn't ever chosen a font before.
                        // Set the font to something familiar.
                        //GetObject(ProcessHacker_GetFont(), sizeof(LOGFONT), &font);
                        GetObject(PhMonospaceFont, sizeof(LOGFONT), &font);
                    }

                    memset(&chooseFont, 0, sizeof(CHOOSEFONT));
                    chooseFont.lStructSize = sizeof(CHOOSEFONT);
                    chooseFont.hwndOwner = hwndDlg;
                    chooseFont.lpfnHook = PhpChooseFontDlgHookProc;
                    chooseFont.lpLogFont = &font;
                    chooseFont.Flags = CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_ENABLEHOOK | CF_SCREENFONTS | CF_FIXEDPITCHONLY;

                    if (ChooseFont(&chooseFont))
                    {
                        PhMoveReference(&NewFontMonospaceSelection, PhBufferToHexString((PUCHAR)&font, sizeof(LOGFONT)));

                        // Update the button's font.
                        if (CurrentFontMonospaceInstance) DeleteFont(CurrentFontMonospaceInstance);
                        CurrentFontMonospaceInstance = CreateFontIndirect(&font);

                        SetWindowFont(GetDlgItem(hwndDlg, IDC_FONTMONOSPACE), CurrentFontMonospaceInstance, TRUE);

                        // Re-add the listview items for the new font (dmex)
                        GeneralListViewStateInitializing = TRUE;
                        HWND listviewHandle = GetDlgItem(hwndDlg, IDC_SETTINGS);
                        ExtendedListView_SetRedraw(listviewHandle, FALSE);
                        ListView_DeleteAllItems(listviewHandle);
                        PhpAdvancedPageLoad(hwndDlg);
                        ExtendedListView_SetRedraw(listviewHandle, TRUE);
                        GeneralListViewStateInitializing = FALSE;

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
                                case PHP_OPTIONS_INDEX_ENABLE_MITIGATION:
                                    {
                                        if (!PhGetOwnTokenAttributes().Elevated)
                                        {
                                            PhShowInformation2(
                                                PhOptionsWindowHandle,
                                                L"Unable to change mitigation policy.",
                                                L"%s",
                                                L"You need to disable this option with administrative privileges."
                                                );

                                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                                            return TRUE;
                                        }
                                    }
                                    break;
                                case PHP_OPTIONS_INDEX_ENABLE_SILENT_CRASH_NOTIFY:
                                    {
                                        if (!PhGetOwnTokenAttributes().Elevated)
                                        {
                                            PhShowInformation2(
                                                PhOptionsWindowHandle,
                                                L"Unable to change process exit notification.",
                                                L"%s",
                                                L"You need to disable this option with administrative privileges."
                                                );

                                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                                            return TRUE;
                                        }
                                    }
                                    break;
                                }
                            }
                            break;
                        case INDEXTOSTATEIMAGEMASK(2): // checked
                            {
                                switch (listView->iItem)
                                {
                                case PHP_OPTIONS_INDEX_ENABLE_START_ASADMIN:
                                    {
                                        PPH_STRING applicationFileName;

                                        if (!PhGetOwnTokenAttributes().Elevated)
                                        {
                                            PhShowInformation2(
                                                PhOptionsWindowHandle,
                                                L"Unable to enable option start as admin.",
                                                L"%s",
                                                L"You need to enable this option with administrative privileges."
                                                );

                                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                                            return TRUE;
                                        }

                                        if (applicationFileName = PhGetApplicationFileNameWin32())
                                        {
                                            static PH_STRINGREF seperator = PH_STRINGREF_INIT(L"\"");
                                            HRESULT status;
                                            PPH_STRING quotedFileName;
                                            RTL_ELEVATION_FLAGS flags;

                                            if (NT_SUCCESS(RtlQueryElevationFlags(&flags)) && flags.ElevationEnabled)
                                            {
                                                static PH_STRINGREF programFilesPathSr = PH_STRINGREF_INIT(L"%ProgramFiles%\\");
                                                PPH_STRING programFilesPath;

                                                if (programFilesPath = PhExpandEnvironmentStrings(&programFilesPathSr))
                                                {
                                                    if (!PhStartsWithString(applicationFileName, programFilesPath, TRUE))
                                                    {
                                                        if (PhShowMessage2(
                                                            PhOptionsWindowHandle,
                                                            TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
                                                            TD_WARNING_ICON,
                                                            L"WARNING: You have not installed System Informer into a secure location.",
                                                            L"Enabling the 'start as admin' option is not recommended when running System Informer from outside a secure location (e.g. Program Files).\r\n\r\nAre you sure you want to continue?"
                                                            ) == IDNO)
                                                        {
                                                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                                                            return TRUE;
                                                        }
                                                    }
                                                }
                                            }

                                            quotedFileName = PH_AUTO(PhConcatStringRef3(
                                                &seperator,
                                                &applicationFileName->sr,
                                                &seperator
                                                ));

                                            status = PhCreateAdminTask(
                                                &SI_RUNAS_ADMIN_TASK_NAME,
                                                &quotedFileName->sr
                                                );

                                            if (FAILED(status))
                                            {
                                                PhShowStatus(
                                                    PhOptionsWindowHandle,
                                                    L"Unable to enable start as admin.",
                                                    0,
                                                    HRESULT_CODE(status) // HACK
                                                    );

                                                PhDereferenceObject(applicationFileName);
                                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                                                return TRUE;
                                            }

                                            PhDereferenceObject(applicationFileName);
                                        }
                                    }
                                    break;
                                case PHP_OPTIONS_INDEX_ENABLE_MITIGATION:
                                    {
                                        NTSTATUS status;

                                        if (!PhGetOwnTokenAttributes().Elevated)
                                        {
                                            PhShowInformation2(
                                                PhOptionsWindowHandle,
                                                L"Unable to enable mitigation policy.",
                                                L"%s",
                                                L"You need to enable this option with administrative privileges."
                                                );

                                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                                            return TRUE;
                                        }

                                        status = PhpSetExploitProtectionEnabled(TRUE);

                                        if (NT_SUCCESS(status))
                                        {
                                            RestartRequired = TRUE;
                                        }
                                        else
                                        {
                                            PhShowStatus(hwndDlg, L"Unable to change mitigation policy.", status, 0);
                                        }
                                    }
                                    break;
                                case PHP_OPTIONS_INDEX_ENABLE_SILENT_CRASH_NOTIFY:
                                    {
                                        NTSTATUS status;

                                        if (!PhGetOwnTokenAttributes().Elevated)
                                        {
                                            PhShowInformation2(
                                                PhOptionsWindowHandle,
                                                L"Unable to change process exit notification.",
                                                L"%s",
                                                L"You need to enable this option with administrative privileges."
                                                );

                                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                                            return TRUE;
                                        }

                                        status = PhpSetSilentProcessNotifyEnabled(TRUE);

                                        if (!NT_SUCCESS(status))
                                        {
                                            PhShowStatus(hwndDlg, L"Unable to change process exit notification.", status, 0);
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
                        if (GeneralListViewStateInitializing)
                            break;

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
                                case PHP_OPTIONS_INDEX_ENABLE_START_ASADMIN:
                                    break;
                                case PHP_OPTIONS_INDEX_ENABLE_MITIGATION:
                                    break;
                                case PHP_OPTIONS_INDEX_ENABLE_SILENT_CRASH_NOTIFY:
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
                                case PHP_OPTIONS_INDEX_ENABLE_START_ASADMIN:
                                    {
                                        if (!!PhGetIntegerSetting(L"EnableStartAsAdmin"))
                                        {
                                            PhDeleteAdminTask(&SI_RUNAS_ADMIN_TASK_NAME);
                                        }
                                    }
                                    break;
                                case PHP_OPTIONS_INDEX_ENABLE_MITIGATION:
                                    {
                                        NTSTATUS status;

                                        status = PhpSetExploitProtectionEnabled(FALSE);

                                        if (NT_SUCCESS(status))
                                        {
                                            RestartRequired = TRUE;
                                        }
                                        else
                                        {
                                            PhShowStatus(hwndDlg, L"Unable to change mitigation policy.", status, 0);
                                        }
                                    }
                                    break;
                                case PHP_OPTIONS_INDEX_ENABLE_SILENT_CRASH_NOTIFY:
                                    {
                                        NTSTATUS status;

                                        status = PhpSetSilentProcessNotifyEnabled(FALSE);

                                        if (!NT_SUCCESS(status))
                                        {
                                            PhShowStatus(hwndDlg, L"Unable to change process exit notification.", status, 0);
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
    //                BoldFont = PhDuplicateFontWithNewWeight(GetWindowFont(control), FW_BOLD);
    //
    //            SetBkMode(hdc, TRANSPARENT);
    //
    //            if (!PhpIsDefaultTaskManager())
    //            {
    //                SelectFont(hdc, BoldFont);
    //            }
    //        }
    //    }
    //    break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
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

            PhSetApplicationWindowIcon(hwndDlg);

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

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
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
                    LONG dpiValue;

                    dpiValue = PhGetWindowDpi(hwndDlg);

                    if (!PhSettingFromString(
                        setting->Type,
                        &settingValue->sr,
                        settingValue,
                        dpiValue,
                        setting
                        ))
                    {
                        PhSettingFromString(
                            setting->Type,
                            &setting->DefaultValue,
                            NULL,
                            dpiValue,
                            setting
                            );
                    }

                    EndDialog(hwndDlg, IDOK);
                }
                break;
            }
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

#pragma region Plugin TreeList

#define WM_PH_OPTIONS_ADVANCED (WM_APP + 451)

typedef struct _PH_OPTIONS_ADVANCED_CONTEXT
{
    PH_LAYOUT_MANAGER LayoutManager;

    HWND WindowHandle;
    HWND TreeNewHandle;
    HWND SearchBoxHandle;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG HideModified : 1;
            ULONG HideDefault : 1;
            ULONG HighlightModified : 1;
            ULONG HighlightDefault : 1;
            ULONG Spare : 28;
        };
    };

    ULONG TreeNewSortColumn;
    PH_SORT_ORDER TreeNewSortOrder;
    PH_TN_FILTER_SUPPORT TreeFilterSupport;
    PPH_TN_FILTER_ENTRY TreeFilterEntry;
    PPH_HASHTABLE NodeHashtable;
    PPH_LIST NodeList;
    PPH_STRING SearchBoxText;
} PH_OPTIONS_ADVANCED_CONTEXT, *PPH_OPTIONS_ADVANCED_CONTEXT;

typedef enum _PH_OPTIONS_ADVANCED_TREE_ITEM_MENU
{
    PH_OPTIONS_ADVANCED_TREE_ITEM_MENU_HIDE_MODIFIED = 1,
    PH_OPTIONS_ADVANCED_TREE_ITEM_MENU_HIDE_DEFAULT,
    PH_OPTIONS_ADVANCED_TREE_ITEM_MENU_HIGHLIGHT_MODIFIED,
    PH_OPTIONS_ADVANCED_TREE_ITEM_MENU_HIGHLIGHT_DEFAULT,
} PH_OPTIONS_ADVANCED_ITEM_MENU;

typedef enum _PH_OPTIONS_ADVANCED_COLUMN_ITEM
{
    PH_OPTIONS_ADVANCED_COLUMN_ITEM_NAME,
    PH_OPTIONS_ADVANCED_COLUMN_ITEM_TYPE,
    PH_OPTIONS_ADVANCED_COLUMN_ITEM_VALUE,
    PH_OPTIONS_ADVANCED_COLUMN_ITEM_DEFAULT,
    PH_OPTIONS_ADVANCED_COLUMN_ITEM_MAXIMUM
} PH_OPTIONS_ADVANCED_COLUMN_ITEM;

typedef struct _PH_OPTIONS_ADVANCED_ROOT_NODE
{
    PH_TREENEW_NODE Node;

    PH_SETTING_TYPE Type;
    PPH_SETTING Setting;
    PPH_STRING Name;
    PPH_STRING ValueString;
    PPH_STRING DefaultString;

    PH_STRINGREF TextCache[PH_OPTIONS_ADVANCED_COLUMN_ITEM_MAXIMUM];
} PH_OPTIONS_ADVANCED_ROOT_NODE, *PPH_OPTIONS_ADVANCED_ROOT_NODE;


#define SORT_FUNCTION(Column) OptionsAdvancedTreeNewCompare##Column
#define BEGIN_SORT_FUNCTION(Column) static int __cdecl OptionsAdvancedTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPH_OPTIONS_ADVANCED_ROOT_NODE node1 = *(PPH_OPTIONS_ADVANCED_ROOT_NODE*)_elem1; \
    PPH_OPTIONS_ADVANCED_ROOT_NODE node2 = *(PPH_OPTIONS_ADVANCED_ROOT_NODE*)_elem2; \
    int sortResult = 0;

#define END_SORT_FUNCTION \
    if (sortResult == 0) \
        sortResult = uintptrcmp((ULONG_PTR)node1->Node.Index, (ULONG_PTR)node2->Node.Index); \
    \
    return PhModifySort(sortResult, ((PPH_OPTIONS_ADVANCED_CONTEXT)_context)->TreeNewSortOrder); \
}

BEGIN_SORT_FUNCTION(Name)
{
    sortResult = PhCompareString(node1->Name, node2->Name, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Type)
{
    sortResult = uintcmp(node1->Type, node2->Type);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Value)
{
    sortResult = PhCompareString(node1->ValueString, node2->ValueString, TRUE);
}
END_SORT_FUNCTION

BEGIN_SORT_FUNCTION(Default)
{
    sortResult = PhCompareString(node1->DefaultString, node2->DefaultString, TRUE);
}
END_SORT_FUNCTION

VOID OptionsAdvancedLoadSettingsTreeList(
    _Inout_ PPH_OPTIONS_ADVANCED_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhGetStringSetting(L"OptionsWindowAdvancedColumns");
    Context->Flags = PhGetIntegerSetting(L"OptionsWindowAdvancedFlags");
    PhCmLoadSettings(Context->TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);
}

VOID OptionsAdvancedSaveSettingsTreeList(
    _Inout_ PPH_OPTIONS_ADVANCED_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(L"OptionsWindowAdvancedColumns", &settings->sr);
    PhSetIntegerSetting(L"OptionsWindowAdvancedFlags", Context->Flags);
    PhDereferenceObject(settings);
}

VOID OptionsAdvancedSetOptionsTreeList(
    _Inout_ PPH_OPTIONS_ADVANCED_CONTEXT Context,
    _In_ ULONG Options
    )
{
    switch (Options)
    {
    case PH_OPTIONS_ADVANCED_TREE_ITEM_MENU_HIDE_MODIFIED:
        Context->HideModified = !Context->HideModified;
        break;
    case PH_OPTIONS_ADVANCED_TREE_ITEM_MENU_HIDE_DEFAULT:
        Context->HideDefault = !Context->HideDefault;
        break;
    case PH_OPTIONS_ADVANCED_TREE_ITEM_MENU_HIGHLIGHT_MODIFIED:
        Context->HighlightModified = !Context->HighlightModified;
        break;
    case PH_OPTIONS_ADVANCED_TREE_ITEM_MENU_HIGHLIGHT_DEFAULT:
        Context->HighlightDefault = !Context->HighlightDefault;
        break;
    }
}

BOOLEAN OptionsAdvancedNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_OPTIONS_ADVANCED_ROOT_NODE node1 = *(PPH_OPTIONS_ADVANCED_ROOT_NODE*)Entry1;
    PPH_OPTIONS_ADVANCED_ROOT_NODE node2 = *(PPH_OPTIONS_ADVANCED_ROOT_NODE*)Entry2;

    return PhEqualStringRef(&node1->Name->sr, &node2->Name->sr, TRUE);
}

ULONG OptionsAdvancedNodeHashtableHashFunction(
    _In_ PVOID Entry
    )
{
    return PhHashStringRefEx(&(*(PPH_OPTIONS_ADVANCED_ROOT_NODE*)Entry)->Name->sr, TRUE, PH_STRING_HASH_X65599);
}

VOID DestroyOptionsAdvancedNode(
    _In_ PPH_OPTIONS_ADVANCED_ROOT_NODE Node
    )
{
    PhClearReference(&Node->Name);
    PhClearReference(&Node->ValueString);
    PhClearReference(&Node->DefaultString);

    PhFree(Node);
}

PPH_OPTIONS_ADVANCED_ROOT_NODE AddOptionsAdvancedNode(
    _Inout_ PPH_OPTIONS_ADVANCED_CONTEXT Context,
    _In_ PPH_SETTING Setting
    )
{
    PPH_OPTIONS_ADVANCED_ROOT_NODE node;

    node = PhAllocate(sizeof(PH_OPTIONS_ADVANCED_ROOT_NODE));
    memset(node, 0, sizeof(PH_OPTIONS_ADVANCED_ROOT_NODE));

    PhInitializeTreeNewNode(&node->Node);

    memset(node->TextCache, 0, sizeof(PH_STRINGREF) * PH_OPTIONS_ADVANCED_COLUMN_ITEM_MAXIMUM);
    node->Node.TextCache = node->TextCache;
    node->Node.TextCacheSize = PH_OPTIONS_ADVANCED_COLUMN_ITEM_MAXIMUM;

    node->Setting = Setting;
    node->Type = Setting->Type;
    node->Name = PhCreateString2(&Setting->Name);
    node->ValueString = PhSettingToString(Setting->Type, Setting);
    node->DefaultString = PhCreateString2(&Setting->DefaultValue);

    PhAddEntryHashtable(Context->NodeHashtable, &node);
    PhAddItemList(Context->NodeList, node);

    if (Context->TreeFilterSupport.FilterList)
        node->Node.Visible = PhApplyTreeNewFiltersToNode(&Context->TreeFilterSupport, &node->Node);

    return node;
}

PPH_OPTIONS_ADVANCED_ROOT_NODE FindOptionsAdvancedNode(
    _In_ PPH_OPTIONS_ADVANCED_CONTEXT Context,
    _In_ PPH_STRING Name
    )
{
    PH_OPTIONS_ADVANCED_ROOT_NODE lookupPluginsNode;
    PPH_OPTIONS_ADVANCED_ROOT_NODE lookupPluginsNodePtr = &lookupPluginsNode;
    PPH_OPTIONS_ADVANCED_ROOT_NODE* pluginsNode;

    lookupPluginsNode.Name = Name;

    pluginsNode = (PPH_OPTIONS_ADVANCED_ROOT_NODE*)PhFindEntryHashtable(
        Context->NodeHashtable,
        &lookupPluginsNodePtr
        );

    if (pluginsNode)
        return *pluginsNode;
    else
        return NULL;
}

VOID RemoveOptionsAdvancedNode(
    _In_ PPH_OPTIONS_ADVANCED_CONTEXT Context,
    _In_ PPH_OPTIONS_ADVANCED_ROOT_NODE Node
    )
{
    ULONG index = 0;

    PhRemoveEntryHashtable(Context->NodeHashtable, &Node);

    if ((index = PhFindItemList(Context->NodeList, Node)) != ULONG_MAX)
    {
        PhRemoveItemList(Context->NodeList, index);
    }

    DestroyOptionsAdvancedNode(Node);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

VOID UpdateOptionsAdvancedNode(
    _In_ PPH_OPTIONS_ADVANCED_CONTEXT Context,
    _In_ PPH_OPTIONS_ADVANCED_ROOT_NODE Node
    )
{
    memset(Node->TextCache, 0, sizeof(PH_STRINGREF) * PH_OPTIONS_ADVANCED_COLUMN_ITEM_MAXIMUM);

    PhInvalidateTreeNewNode(&Node->Node, TN_CACHE_COLOR);
    TreeNew_NodesStructured(Context->TreeNewHandle);
}

BOOLEAN NTAPI OptionsAdvancedTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    PPH_OPTIONS_ADVANCED_CONTEXT context = Context;
    PPH_OPTIONS_ADVANCED_ROOT_NODE node;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            node = (PPH_OPTIONS_ADVANCED_ROOT_NODE)getChildren->Node;

            if (!getChildren->Node)
            {
                static PVOID sortFunctions[] =
                {
                    SORT_FUNCTION(Name),
                    SORT_FUNCTION(Type),
                    SORT_FUNCTION(Value),
                    SORT_FUNCTION(Default),
                };
                int(__cdecl * sortFunction)(void*, const void*, const void*);

                if (context->TreeNewSortColumn < PH_OPTIONS_ADVANCED_COLUMN_ITEM_MAXIMUM)
                    sortFunction = sortFunctions[context->TreeNewSortColumn];
                else
                    sortFunction = NULL;

                if (sortFunction)
                {
                    qsort_s(context->NodeList->Items, context->NodeList->Count, sizeof(PVOID), sortFunction, context);
                }

                getChildren->Children = (PPH_TREENEW_NODE*)context->NodeList->Items;
                getChildren->NumberOfChildren = context->NodeList->Count;
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = (PPH_TREENEW_IS_LEAF)Parameter1;
            node = (PPH_OPTIONS_ADVANCED_ROOT_NODE)isLeaf->Node;

            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewGetCellText:
        {
            PPH_TREENEW_GET_CELL_TEXT getCellText = (PPH_TREENEW_GET_CELL_TEXT)Parameter1;
            node = (PPH_OPTIONS_ADVANCED_ROOT_NODE)getCellText->Node;

            switch (getCellText->Id)
            {
            case PH_OPTIONS_ADVANCED_COLUMN_ITEM_NAME:
                getCellText->Text = PhGetStringRef(node->Name);
                break;
            case PH_OPTIONS_ADVANCED_COLUMN_ITEM_TYPE:
                {
                    switch (node->Type)
                    {
                    case StringSettingType:
                        PhInitializeStringRef(&getCellText->Text, L"String");
                        break;
                    case IntegerSettingType:
                        PhInitializeStringRef(&getCellText->Text, L"Integer");
                        break;
                    case IntegerPairSettingType:
                        PhInitializeStringRef(&getCellText->Text, L"IntegerPair");
                        break;
                    case ScalableIntegerPairSettingType:
                        PhInitializeStringRef(&getCellText->Text, L"ScalableIntegerPair");
                        break;
                    }
                }
                break;
            case PH_OPTIONS_ADVANCED_COLUMN_ITEM_VALUE:
                getCellText->Text = PhGetStringRef(node->ValueString);
                break;
            case PH_OPTIONS_ADVANCED_COLUMN_ITEM_DEFAULT:
                getCellText->Text = PhGetStringRef(node->DefaultString);
                break;
            default:
                return FALSE;
            }

            //getCellText->Flags = TN_CACHE;
        }
        return TRUE;
    case TreeNewGetNodeColor:
        {
            PPH_TREENEW_GET_NODE_COLOR getNodeColor = Parameter1;
            node = (PPH_OPTIONS_ADVANCED_ROOT_NODE)getNodeColor->Node;

            switch (node->Type)
            {
            case StringSettingType:
            case IntegerPairSettingType:
            case ScalableIntegerPairSettingType:
                {
                    if (PhEqualString(node->DefaultString, node->ValueString, TRUE))
                    {
                        if (context->HighlightDefault)
                        {
                            getNodeColor->BackColor = PhCsColorServiceProcesses;
                        }
                    }
                    else
                    {
                        if (context->HighlightModified)
                        {
                            getNodeColor->BackColor = PhCsColorSystemProcesses;
                        }
                    }
                }
                break;
            case IntegerSettingType:
                {
                    ULONG64 integer;

                    if (PhStringToInteger64(&node->DefaultString->sr, 16, &integer))
                    {
                        if (node->Setting->u.Integer == (ULONG)integer)
                        {
                            if (context->HighlightDefault)
                            {
                                getNodeColor->BackColor = PhCsColorServiceProcesses;
                            }
                        }
                        else
                        {
                            if (context->HighlightModified)
                            {
                                getNodeColor->BackColor = PhCsColorSystemProcesses;
                            }
                        }
                    }
                }
                break;
            }

            getNodeColor->Flags = TN_AUTO_FORECOLOR;
        }
        return TRUE;
    case TreeNewSortChanged:
        {
            TreeNew_GetSort(hwnd, &context->TreeNewSortColumn, &context->TreeNewSortOrder);
            // Force a rebuild to sort the items.
            TreeNew_NodesStructured(hwnd);
        }
        return TRUE;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;

            switch (keyEvent->VirtualKey)
            {
            case 'C':
                if (GetKeyState(VK_CONTROL) < 0)
                    SendMessage(context->WindowHandle, WM_COMMAND, IDC_COPY, 0);
                break;
            case 'A':
                if (GetKeyState(VK_CONTROL) < 0)
                    TreeNew_SelectRange(context->TreeNewHandle, 0, -1);
                break;
            }
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            PPH_TREENEW_MOUSE_EVENT mouseEvent = Parameter1;

            SendMessage(context->WindowHandle, WM_COMMAND, WM_PH_OPTIONS_ADVANCED, (LPARAM)mouseEvent);
        }
        return TRUE;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenuEvent = Parameter1;

            SendMessage(context->WindowHandle, WM_CONTEXTMENU, (WPARAM)hwnd, (LPARAM)contextMenuEvent);
        }
        return TRUE;
    case TreeNewHeaderRightClick:
        {
            PH_TN_COLUMN_MENU_DATA data;

            data.TreeNewHandle = hwnd;
            data.MouseEvent = Parameter1;
            data.DefaultSortColumn = 0;
            data.DefaultSortOrder = AscendingSortOrder;
            PhInitializeTreeNewColumnMenuEx(&data, PH_TN_COLUMN_MENU_SHOW_RESET_SORT);

            data.Selection = PhShowEMenu(data.Menu, hwnd, PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, data.MouseEvent->ScreenLocation.x, data.MouseEvent->ScreenLocation.y);
            PhHandleTreeNewColumnMenu(&data);
            PhDeleteTreeNewColumnMenu(&data);
        }
        return TRUE;
    }

    return FALSE;
}

VOID ClearOptionsAdvancedTree(
    _In_ PPH_OPTIONS_ADVANCED_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        DestroyOptionsAdvancedNode(Context->NodeList->Items[i]);

    PhClearHashtable(Context->NodeHashtable);
    PhClearList(Context->NodeList);

    TreeNew_NodesStructured(Context->TreeNewHandle);
}

PPH_OPTIONS_ADVANCED_ROOT_NODE GetSelectedOptionsAdvancedNode(
    _In_ PPH_OPTIONS_ADVANCED_CONTEXT Context
    )
{
    PPH_OPTIONS_ADVANCED_ROOT_NODE optionsNode = NULL;

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        optionsNode = Context->NodeList->Items[i];

        if (optionsNode->Node.Selected)
            return optionsNode;
    }

    return NULL;
}

_Success_(return)
BOOLEAN GetSelectedOptionsAdvancedNodes(
    _In_ PPH_OPTIONS_ADVANCED_CONTEXT Context,
    _Out_ PPH_OPTIONS_ADVANCED_ROOT_NODE** Nodes,
    _Out_ PULONG NumberOfNodes
    )
{
    PPH_LIST list = PhCreateList(2);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
    {
        PPH_OPTIONS_ADVANCED_ROOT_NODE node = Context->NodeList->Items[i];

        if (node->Node.Selected)
        {
            PhAddItemList(list, node);
        }
    }

    if (list->Count)
    {
        *Nodes = PhAllocateCopy(list->Items, sizeof(PVOID) * list->Count);
        *NumberOfNodes = list->Count;

        PhDereferenceObject(list);
        return TRUE;
    }

    PhDereferenceObject(list);
    return FALSE;
}

VOID InitializeOptionsAdvancedTree(
    _Inout_ PPH_OPTIONS_ADVANCED_CONTEXT Context
    )
{
    Context->NodeList = PhCreateList(100);
    Context->NodeHashtable = PhCreateHashtable(
        sizeof(PPH_OPTIONS_ADVANCED_ROOT_NODE),
        OptionsAdvancedNodeHashtableEqualFunction,
        OptionsAdvancedNodeHashtableHashFunction,
        100
        );

    PhSetControlTheme(Context->TreeNewHandle, L"explorer");

    TreeNew_SetCallback(Context->TreeNewHandle, OptionsAdvancedTreeNewCallback, Context);

    PhAddTreeNewColumnEx(Context->TreeNewHandle, PH_OPTIONS_ADVANCED_COLUMN_ITEM_NAME, TRUE, L"Name", 200, PH_ALIGN_LEFT, 0, 0, TRUE);
    PhAddTreeNewColumnEx(Context->TreeNewHandle, PH_OPTIONS_ADVANCED_COLUMN_ITEM_TYPE, TRUE, L"Type", 100, PH_ALIGN_LEFT, 1, 0, TRUE);
    PhAddTreeNewColumnEx(Context->TreeNewHandle, PH_OPTIONS_ADVANCED_COLUMN_ITEM_VALUE, TRUE, L"Value", 200, PH_ALIGN_LEFT, 2, 0, TRUE);
    PhAddTreeNewColumnEx(Context->TreeNewHandle, PH_OPTIONS_ADVANCED_COLUMN_ITEM_DEFAULT, TRUE, L"Default", 200, PH_ALIGN_LEFT, 3, 0, TRUE);

    TreeNew_SetTriState(Context->TreeNewHandle, TRUE);

    PhInitializeTreeNewFilterSupport(&Context->TreeFilterSupport, Context->TreeNewHandle, Context->NodeList);

    OptionsAdvancedLoadSettingsTreeList(Context);
}

VOID DeleteOptionsAdvancedTree(
    _In_ PPH_OPTIONS_ADVANCED_CONTEXT Context
    )
{
    PhDeleteTreeNewFilterSupport(&Context->TreeFilterSupport);

    OptionsAdvancedSaveSettingsTreeList(Context);

    for (ULONG i = 0; i < Context->NodeList->Count; i++)
        DestroyOptionsAdvancedNode(Context->NodeList->Items[i]);

    PhDereferenceObject(Context->NodeHashtable);
    PhDereferenceObject(Context->NodeList);
}

#pragma endregion

static BOOLEAN PhpOptionsSettingsCallback(
    _In_ PPH_SETTING Setting,
    _In_ PVOID Context
    )
{
    PPH_OPTIONS_ADVANCED_CONTEXT context = Context;

    AddOptionsAdvancedNode(context, Setting);
    return TRUE;
}

BOOLEAN PhpOptionsAdvancedTreeFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_ PVOID Context
    )
{
    PPH_OPTIONS_ADVANCED_ROOT_NODE node = (PPH_OPTIONS_ADVANCED_ROOT_NODE)Node;
    PPH_OPTIONS_ADVANCED_CONTEXT context = Context;

    if (context->HideModified)
    {
        switch (node->Type)
        {
        case StringSettingType:
        case IntegerPairSettingType:
        case ScalableIntegerPairSettingType:
            {
                if (PhEqualString(node->DefaultString, node->ValueString, TRUE))
                {
                    if (context->HideDefault)
                    {
                        return FALSE;
                    }
                }
                else
                {
                    if (context->HideModified)
                    {
                        return FALSE;
                    }
                }
            }
            break;
        case IntegerSettingType:
            {
                ULONG64 integer;

                if (PhStringToInteger64(&node->DefaultString->sr, 16, &integer))
                {
                    if (node->Setting->u.Integer == (ULONG)integer)
                    {
                        if (context->HideDefault)
                        {
                            return FALSE;
                        }
                    }
                    else
                    {
                        if (context->HideModified)
                        {
                            return FALSE;
                        }
                    }
                }
            }
            break;
        }
    }

    if (context->HideDefault)
    {
        switch (node->Type)
        {
        case StringSettingType:
        case IntegerPairSettingType:
        case ScalableIntegerPairSettingType:
            {
                if (PhEqualString(node->DefaultString, node->ValueString, TRUE))
                {
                    if (context->HideDefault)
                    {
                        return FALSE;
                    }
                }
                else
                {
                    if (context->HideModified)
                    {
                        return FALSE;
                    }
                }
            }
            break;
        case IntegerSettingType:
            {
                ULONG64 integer;

                if (PhStringToInteger64(&node->DefaultString->sr, 16, &integer))
                {
                    if (node->Setting->u.Integer == (ULONG)integer)
                    {
                        if (context->HideDefault)
                        {
                            return FALSE;
                        }
                    }
                    else
                    {
                        if (context->HideModified)
                        {
                            return FALSE;
                        }
                    }
                }
            }
            break;
        }
    }

    if (PhIsNullOrEmptyString(context->SearchBoxText))
        return TRUE;

    if (PhWordMatchStringRef(&context->SearchBoxText->sr, &node->Name->sr))
        return TRUE;
    if (PhWordMatchStringRef(&context->SearchBoxText->sr, &node->DefaultString->sr))
        return TRUE;
    if (PhWordMatchStringRef(&context->SearchBoxText->sr, &node->ValueString->sr))
        return TRUE;

    return FALSE;
}

INT_PTR CALLBACK PhpOptionsAdvancedDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_OPTIONS_ADVANCED_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PH_OPTIONS_ADVANCED_CONTEXT));
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->TreeNewHandle = GetDlgItem(hwndDlg, IDC_SETTINGS);
            context->SearchBoxHandle = GetDlgItem(hwndDlg, IDC_SEARCH);

            PhCreateSearchControl(hwndDlg, context->SearchBoxHandle, L"Search settings...");
            InitializeOptionsAdvancedTree(context);
            context->SearchBoxText = PhReferenceEmptyString();
            context->TreeFilterEntry = PhAddTreeNewFilter(&context->TreeFilterSupport, PhpOptionsAdvancedTreeFilterCallback, context);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->SearchBoxHandle, NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->TreeNewHandle, NULL, PH_ANCHOR_ALL);

            PhEnumSettings(PhpOptionsSettingsCallback, context);
            TreeNew_NodesStructured(context->TreeNewHandle);
        }
        break;
    case WM_DESTROY:
        {
            if (context->SearchBoxText)
                PhDereferenceObject(context->SearchBoxText);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhRemoveTreeNewFilter(&context->TreeFilterSupport, context->TreeFilterEntry);
            DeleteOptionsAdvancedTree(context);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDOK:
            case IDCANCEL:
                {
                    DestroyWindow(hwndDlg);
                }
                break;
            case IDC_FILTEROPTIONS:
                {
                    RECT rect;
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM hidemodifiedMenuItem;
                    PPH_EMENU_ITEM hidedefaultMenuItem;
                    PPH_EMENU_ITEM highlightmodifiedMenuItem;
                    PPH_EMENU_ITEM highlightdefaultMenuItem;
                    PPH_EMENU_ITEM selectedItem;

                    GetWindowRect(GetDlgItem(hwndDlg, IDC_FILTEROPTIONS), &rect);

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, hidemodifiedMenuItem = PhCreateEMenuItem(0, PH_OPTIONS_ADVANCED_TREE_ITEM_MENU_HIDE_MODIFIED, L"Hide modified", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, hidedefaultMenuItem = PhCreateEMenuItem(0, PH_OPTIONS_ADVANCED_TREE_ITEM_MENU_HIDE_DEFAULT, L"Hide default", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhInsertEMenuItem(menu, highlightmodifiedMenuItem = PhCreateEMenuItem(0, PH_OPTIONS_ADVANCED_TREE_ITEM_MENU_HIGHLIGHT_MODIFIED, L"Highlight modified", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, highlightdefaultMenuItem = PhCreateEMenuItem(0, PH_OPTIONS_ADVANCED_TREE_ITEM_MENU_HIGHLIGHT_DEFAULT, L"Highlight default", NULL, NULL), ULONG_MAX);

                    if (context->HideModified)
                        hidemodifiedMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HideDefault)
                        hidedefaultMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HighlightModified)
                        highlightmodifiedMenuItem->Flags |= PH_EMENU_CHECKED;
                    if (context->HighlightDefault)
                        highlightdefaultMenuItem->Flags |= PH_EMENU_CHECKED;

                    selectedItem = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        rect.left,
                        rect.bottom
                        );

                    if (selectedItem && selectedItem->Id)
                    {
                        OptionsAdvancedSetOptionsTreeList(context, selectedItem->Id);

                        PhApplyTreeNewFilters(&context->TreeFilterSupport);
                    }

                    PhDestroyEMenu(menu);
                }
                break;
            case IDC_REFRESH:
                {
                    ClearOptionsAdvancedTree(context);
                    PhEnumSettings(PhpOptionsSettingsCallback, context);
                    TreeNew_NodesStructured(context->TreeNewHandle);

                    PhApplyTreeNewFilters(&context->TreeFilterSupport);
                }
                break;
            case WM_PH_OPTIONS_ADVANCED:
                {
                    PPH_OPTIONS_ADVANCED_ROOT_NODE node;

                    if (node = GetSelectedOptionsAdvancedNode(context))
                    {
                        DialogBoxParam(
                            PhInstanceHandle,
                            MAKEINTRESOURCE(IDD_EDITENV),
                            hwndDlg,
                            PhpOptionsAdvancedEditDlgProc,
                            (LPARAM)node->Setting
                            );

                        PhMoveReference(
                            &node->ValueString,
                            PhSettingToString(node->Setting->Type, node->Setting)
                            );

                        TreeNew_NodesStructured(context->TreeNewHandle);
                    }
                }
                break;
            case IDC_COPY:
                {
                    PPH_STRING text;

                    text = PhGetTreeNewText(context->TreeNewHandle, 0);
                    PhSetClipboardString(context->TreeNewHandle, &text->sr);
                    PhDereferenceObject(text);
                }
                break;
            }

            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case EN_CHANGE:
                {
                    PPH_STRING newSearchboxText;

                    if (!context->SearchBoxHandle)
                        break;

                    if (GET_WM_COMMAND_HWND(wParam, lParam) != context->SearchBoxHandle)
                        break;

                    newSearchboxText = PH_AUTO(PhGetWindowText(context->SearchBoxHandle));

                    if (!PhEqualString(context->SearchBoxText, newSearchboxText, FALSE))
                    {
                        PhSwapReference(&context->SearchBoxText, newSearchboxText);

                        if (!PhIsNullOrEmptyString(context->SearchBoxText))
                        {
                            // Expand the nodes?
                        }

                        PhApplyTreeNewFilters(&context->TreeFilterSupport);
                    }
                }
                break;
            }
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->TreeNewHandle)
            {
                PPH_TREENEW_CONTEXT_MENU contextMenuEvent = (PPH_TREENEW_CONTEXT_MENU)lParam;
                PPH_OPTIONS_ADVANCED_ROOT_NODE* nodes;
                ULONG numberOfNodes;

                if (!GetSelectedOptionsAdvancedNodes(context, &nodes, &numberOfNodes))
                    break;

                if (numberOfNodes != 0)
                {
                    PPH_EMENU menu;
                    PPH_EMENU_ITEM item;

                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy\bCtrl+C", NULL, NULL), ULONG_MAX);
                    PhInsertCopyCellEMenuItem(menu, IDC_COPY, context->TreeNewHandle, contextMenuEvent->Column);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        contextMenuEvent->Location.x,
                        contextMenuEvent->Location.y
                        );

                    if (item)
                    {
                        if (!PhHandleCopyCellEMenuItem(item))
                        {
                            SendMessage(hwndDlg, WM_COMMAND, item->Id, 0);
                        }
                    }

                    PhDestroyEMenu(menu);
                }

                PhFree(nodes);
            }
        }
        break;
    case WM_NOTIFY:
        {
            REFLECT_MESSAGE_DLG(hwndDlg, context->TreeNewHandle, uMsg, wParam, lParam);
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
    COLOR_ITEM(L"ColorOwnProcesses", L"Own processes", L"Processes running under the same user account as System Informer."),
    COLOR_ITEM(L"ColorSystemProcesses", L"System processes", L"Processes running under the NT AUTHORITY\\SYSTEM user account."),
    COLOR_ITEM(L"ColorServiceProcesses", L"Service processes", L"Processes which host one or more services."),
    COLOR_ITEM(L"ColorBackgroundProcesses", L"Background processes", L"Processes with a background scheduling priority."),
    COLOR_ITEM(L"ColorJobProcesses", L"Job processes", L"Processes associated with a job."),
#ifdef _WIN64
    COLOR_ITEM(L"ColorWow64Processes", L"32-bit processes", L"Processes running under WOW64, i.e. 32-bit."),
#endif
    COLOR_ITEM(L"ColorDebuggedProcesses", L"Debugged processes", L"Processes that are currently being debugged."),
    COLOR_ITEM(L"ColorElevatedProcesses", L"Elevated processes", L"Processes with full privileges on a system with UAC enabled."),
    COLOR_ITEM(L"ColorPicoProcesses", L"Pico processes", L"Processes that belong to the Windows subsystem for Linux."),
    COLOR_ITEM(L"ColorImmersiveProcesses", L"Immersive processes and DLLs", L"Processes and DLLs that belong to a Modern UI app."),
    COLOR_ITEM(L"ColorSuspended", L"Suspended processes and threads", L"Processes and threads that are suspended from execution."),
    COLOR_ITEM(L"ColorPartiallySuspended", L"Partially suspended processes and threads", L"Processes and threads that are partially suspended from execution."),
    COLOR_ITEM(L"ColorDotNet", L".NET processes and DLLs", L".NET (i.e. managed) processes and DLLs."),
    COLOR_ITEM(L"ColorPacked", L"Packed processes", L"Executables are sometimes \"packed\" to reduce their size."),
    COLOR_ITEM(L"ColorLowImageCoherency", L"Low process image coherency", L"The image file backing the process has low coherency when compared to the mapped image."),
    COLOR_ITEM(L"ColorGuiThreads", L"GUI threads", L"Threads that have made at least one GUI-related system call."),
    COLOR_ITEM(L"ColorRelocatedModules", L"Relocated DLLs", L"DLLs that were not loaded at their preferred image bases."),
    COLOR_ITEM(L"ColorProtectedHandles", L"Protected handles", L"Handles that are protected from being closed."),
    COLOR_ITEM(L"ColorProtectedProcess", L"Protected processes", L"Processes with built-in protection levels."),
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
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
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
            ColorBox_ThemeSupport(GetDlgItem(hwndDlg, IDC_NEWOBJECTS), PhEnableThemeSupport);

            // Removed Objects
            ColorBox_SetColor(GetDlgItem(hwndDlg, IDC_REMOVEDOBJECTS), PhCsColorRemoved);
            ColorBox_ThemeSupport(GetDlgItem(hwndDlg, IDC_REMOVEDOBJECTS), PhEnableThemeSupport);

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
                            CHOOSECOLOR chooseColor;
                            COLORREF customColors[16] = { 0 };

                            PhLoadCustomColorList(L"OptionsCustomColorList", customColors, RTL_NUMBER_OF(customColors));

                            memset(&chooseColor, 0, sizeof(CHOOSECOLOR));
                            chooseColor.lStructSize = sizeof(CHOOSECOLOR);
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

                            PhSaveCustomColorList(L"OptionsCustomColorList", customColors, RTL_NUMBER_OF(customColors));
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

            REFLECT_MESSAGE_DLG(hwndDlg, HighlightingListViewHandle, uMsg, wParam, lParam);
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

static COLOR_ITEM PhpOptionsGraphColorItems[] =
{
    COLOR_ITEM(L"ColorCpuKernel", L"CPU kernel", L"CPU kernel"),
    COLOR_ITEM(L"ColorCpuUser", L"CPU user", L"CPU user"),
    COLOR_ITEM(L"ColorIoReadOther", L"I/O R+O", L"I/O R+O"),
    COLOR_ITEM(L"ColorIoWrite", L"I/O W", L"I/O W"),
    COLOR_ITEM(L"ColorPrivate", L"Private bytes", L"Private bytes"),
    COLOR_ITEM(L"ColorPhysical", L"Physical memory", L"Physical memory"),
    COLOR_ITEM(L"ColorPowerUsage", L"Power usage", L"Power usage"),
    COLOR_ITEM(L"ColorTemperature", L"Temperature", L"Temperature"),
    COLOR_ITEM(L"ColorFanRpm", L"Fan RPM", L"Fan RPM"),
};
static HWND PhpGraphListViewHandle = NULL;

INT_PTR CALLBACK PhpOptionsGraphsDlgProc(
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
            // Show Text
            SetDlgItemCheckForSetting(hwndDlg, IDC_SHOWTEXT, L"GraphShowText");
            SetDlgItemCheckForSetting(hwndDlg, IDC_USEOLDCOLORS, L"GraphColorMode");
            SetDlgItemCheckForSetting(hwndDlg, IDC_SHOWCOMMITINSUMMARY, L"ShowCommitInSummary");

            // Highlighting
            PhpGraphListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);
            PhSetListViewStyle(PhpGraphListViewHandle, FALSE, TRUE);
            PhAddListViewColumn(PhpGraphListViewHandle, 0, 0, 0, LVCFMT_LEFT, 240, L"Name");
            PhSetExtendedListView(PhpGraphListViewHandle);
            ExtendedListView_SetItemColorFunction(PhpGraphListViewHandle, PhpColorItemColorFunction);

            for (ULONG i = 0; i < RTL_NUMBER_OF(PhpOptionsGraphColorItems); i++)
            {
                INT lvItemIndex = PhAddListViewItem(PhpGraphListViewHandle, MAXINT, PhpOptionsGraphColorItems[i].Name, &PhpOptionsGraphColorItems[i]);
                PhpOptionsGraphColorItems[i].CurrentColor = PhGetIntegerSetting(PhpOptionsGraphColorItems[i].SettingName);
            }

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, PhpGraphListViewHandle, NULL, PH_ANCHOR_ALL);

            if (PhGetIntegerSetting(L"GraphColorMode"))
                EnableWindow(PhpGraphListViewHandle, TRUE);
        }
        break;
    case WM_DESTROY:
        {
            SetSettingForDlgItemCheck(hwndDlg, IDC_SHOWTEXT, L"GraphShowText");
            SetSettingForDlgItemCheck(hwndDlg, IDC_USEOLDCOLORS, L"GraphColorMode");
            SetSettingForDlgItemCheck(hwndDlg, IDC_SHOWCOMMITINSUMMARY, L"ShowCommitInSummary");

            for (ULONG i = 0; i < RTL_NUMBER_OF(PhpOptionsGraphColorItems); i++)
            {
                PhSetIntegerSetting(PhpOptionsGraphColorItems[i].SettingName, PhpOptionsGraphColorItems[i].CurrentColor);
            }

            PhDeleteLayoutManager(&LayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&LayoutManager);

            ExtendedListView_SetColumnWidth(PhpGraphListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_USEOLDCOLORS:
                {
                    ListView_SetItemState(PhpGraphListViewHandle, -1, 0, LVIS_SELECTED); // deselect all items

                    EnableWindow(PhpGraphListViewHandle, Button_GetCheck(GET_WM_COMMAND_HWND(wParam, lParam)) == BST_CHECKED);
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
                    if (header->hwndFrom == PhpGraphListViewHandle)
                    {
                        PCOLOR_ITEM item;

                        if (item = PhGetSelectedListViewItemParam(PhpGraphListViewHandle))
                        {
                            CHOOSECOLOR chooseColor;
                            COLORREF customColors[16] = { 0 };

                            PhLoadCustomColorList(L"OptionsCustomColorList", customColors, RTL_NUMBER_OF(customColors));

                            memset(&chooseColor, 0, sizeof(CHOOSECOLOR));
                            chooseColor.lStructSize = sizeof(CHOOSECOLOR);
                            chooseColor.hwndOwner = hwndDlg;
                            chooseColor.rgbResult = item->CurrentColor;
                            chooseColor.lpCustColors = customColors;
                            chooseColor.lpfnHook = PhpColorDlgHookProc;
                            chooseColor.Flags = CC_ANYCOLOR | CC_FULLOPEN | CC_SOLIDCOLOR | CC_ENABLEHOOK | CC_RGBINIT;

                            if (ChooseColor(&chooseColor))
                            {
                                item->CurrentColor = chooseColor.rgbResult;
                                InvalidateRect(PhpGraphListViewHandle, NULL, TRUE);
                            }

                            PhSaveCustomColorList(L"OptionsCustomColorList", customColors, RTL_NUMBER_OF(customColors));
                        }
                    }
                }
                break;
            case LVN_GETINFOTIP:
                {
                    if (header->hwndFrom == PhpGraphListViewHandle)
                    {
                        NMLVGETINFOTIP* getInfoTip = (NMLVGETINFOTIP*)lParam;
                        PH_STRINGREF tip;

                        PhInitializeStringRefLongHint(&tip, ColorItems[getInfoTip->iItem].Description);
                        PhCopyListViewInfoTip(getInfoTip, &tip);
                    }
                }
                break;
            }

            if (IsWindowEnabled(PhpGraphListViewHandle)) // HACK: Move to WM_COMMAND (dmex)
            {
                REFLECT_MESSAGE_DLG(hwndDlg, PhpGraphListViewHandle, uMsg, wParam, lParam);
            }
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
