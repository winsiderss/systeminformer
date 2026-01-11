/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2023
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
#include <notifico.h>
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

_Function_class_(PH_OPTIONS_ENTER_SECTION_VIEW)
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

_Function_class_(PH_OPTIONS_FIND_SECTION)
PPH_OPTIONS_SECTION PhOptionsFindSection(
    _In_ PCPH_STRINGREF Name
    );

VOID PhOptionsOnSize(
    VOID
    );

_Function_class_(PH_OPTIONS_CREATE_SECTION)
PPH_OPTIONS_SECTION PhOptionsCreateSection(
    _In_ PCWSTR Name,
    _In_ PVOID Instance,
    _In_ PCWSTR Template,
    _In_ DLGPROC DialogProc,
    _In_opt_ PVOID Parameter
    );

PPH_OPTIONS_SECTION PhOptionsCreateSectionAdvanced(
    _In_ PCWSTR Name,
    _In_ PVOID Instance,
    _In_ PCWSTR Template,
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

// All
static BOOLEAN RestartRequired = FALSE;

// General
static BOOLEAN GeneralListViewStateInitializing = FALSE;
static CONST PH_STRINGREF CurrentUserRunKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows\\CurrentVersion\\Run");
static BOOLEAN CurrentUserRunPresent = FALSE;
static HFONT CurrentFontInstance = NULL;
static HFONT CurrentFontMonospaceInstance = NULL;
static PPH_STRING NewFontSelection = NULL;
static PPH_STRING NewFontMonospaceSelection = NULL;

// Advanced
static CONST PH_STRINGREF TaskMgrImageOptionsKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Image File Execution Options\\taskmgr.exe");
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
    _In_ PCWSTR Text,
    _In_ PVOID Context
    )
{
    TV_INSERTSTRUCT insert;

    memset(&insert, 0, sizeof(TV_INSERTSTRUCT));

    insert.hParent = TVI_ROOT;
    insert.hInsertAfter = HandleInsertAfter;
    insert.item.mask = TVIF_TEXT | TVIF_PARAM;
    insert.item.pszText = (PWSTR)Text;
    insert.item.lParam = (LPARAM)Context;

    return TreeView_InsertItem(OptionsTreeControl, &insert);
}

static VOID PhpOptionsShowHideTreeViewItem(
    _In_ BOOLEAN Hide
    )
{
    static CONST PH_STRINGREF generalName = PH_STRINGREF_INIT(L"General");
    static CONST PH_STRINGREF advancedName = PH_STRINGREF_INIT(L"Advanced");

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

VOID PhpAdvancedPageSave(
    _In_ HWND hwndDlg
    );

VOID PhpAdvancedPageLoad(
    _In_ HWND hwndDlg,
    _In_ BOOLEAN ReloadOnly
    );

static VOID PhReloadGeneralSection(
    VOID
    )
{
    static PH_STRINGREF generalName = PH_STRINGREF_INIT(L"General");

    GeneralListViewStateInitializing = TRUE;
    PhpAdvancedPageLoad(PhOptionsFindSection(&generalName)->DialogHandle, TRUE);
    GeneralListViewStateInitializing = FALSE;
}

static VOID PhpOptionsSetImageList(
    _In_ HWND WindowHandle,
    _In_ BOOLEAN Treeview
    )
{
    HIMAGELIST imageListHandle;
    LONG dpiValue = PhGetWindowDpi(WindowHandle);

    if (Treeview)
        imageListHandle = TreeView_GetImageList(WindowHandle, TVSIL_NORMAL);
    else
        imageListHandle = ListView_GetImageList(WindowHandle, LVSIL_SMALL);

    if (imageListHandle)
    {
        PhImageListSetIconSize(imageListHandle, 2, PhGetDpi(24, dpiValue));

        if (Treeview)
            TreeView_SetImageList(WindowHandle, imageListHandle, TVSIL_NORMAL);
        else
            ListView_SetImageList(WindowHandle, imageListHandle, LVSIL_SMALL);
    }
    else
    {
        if (imageListHandle = PhImageListCreate(2, PhGetDpi(24, dpiValue), ILC_MASK | ILC_COLOR32, 1, 1))
        {
            if (Treeview)
                TreeView_SetImageList(WindowHandle, imageListHandle, TVSIL_NORMAL);
            else
                ListView_SetImageList(WindowHandle, imageListHandle, LVSIL_SMALL);
        }
    }
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

            PhpOptionsSetImageList(OptionsTreeControl, TRUE);

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

            if (PhValidWindowPlacementFromSetting(SETTING_OPTIONS_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_OPTIONS_WINDOW_POSITION, SETTING_OPTIONS_WINDOW_SIZE, hwndDlg);
            else
                PhCenterWindow(hwndDlg, PhMainWndHandle);
        }
        break;
    case WM_DESTROY:
        {
            ULONG i;
            PPH_OPTIONS_SECTION section;

            PhSaveWindowPlacementToSetting(SETTING_OPTIONS_WINDOW_POSITION, SETTING_OPTIONS_WINDOW_SIZE, hwndDlg);

            PhDeleteLayoutManager(&WindowLayoutManager);

            for (i = 0; i < SectionList->Count; i++)
            {
                section = SectionList->Items[i];

                if (PhEqualStringRef2(&section->Name, L"General", TRUE))
                {
                    PhpAdvancedPageSave(section->DialogHandle);
                }

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
            PhpOptionsSetImageList(OptionsTreeControl, TRUE);
            PhLayoutManagerUpdate(&WindowLayoutManager, LOWORD(wParam));
            PhOptionsOnSize();
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
                        TD_YES_BUTTON | TD_NO_BUTTON,
                        TD_WARNING_ICON,
                        L"Do you want to reset all settings and restart System Informer?",
                        L""
                        ) == IDYES)
                    {
                        SystemInformer_PrepareForEarlyShutdown();

                        PhResetSettings(PhMainWndHandle);
                        PhSaveSettings2(PhSettingsFileName);

                        if (NT_SUCCESS(PhShellProcessHacker(
                            PhMainWndHandle,
                            L"-v -newinstance",
                            SW_SHOW,
                            PH_SHELL_EXECUTE_DEFAULT,
                            PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
                            0,
                            NULL
                            )))
                        {
                            SystemInformer_Destroy();
                        }
                        else
                        {
                            SystemInformer_CancelEarlyShutdown();
                        }
                    }
                }
                break;
            case IDC_CLEANUP:
                {
                    if (PhShowMessage2(
                        hwndDlg,
                        TD_YES_BUTTON | TD_NO_BUTTON,
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
            //                FillRect(drawInfo->hDC, &rect, PhGetStockBrush(DC_BRUSH));
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
                    PPH_OPTIONS_SECTION section = (PPH_OPTIONS_SECTION)treeview->itemNew.lParam;

                    if (section)
                    {
                        PhOptionsEnterSectionView(section);
                    }
                }
                break;
            case NM_SETCURSOR:
                {
                    if (header->hwndFrom == OptionsTreeControl)
                    {
                        PhSetCursor(PhLoadArrowCursor());

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

_Function_class_(PH_OPTIONS_CREATE_SECTION)
PPH_OPTIONS_SECTION PhOptionsCreateSection(
    _In_ PCWSTR Name,
    _In_ PVOID Instance,
    _In_ PCWSTR Template,
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
    _In_ PCWSTR Name,
    _In_ PVOID Instance,
    _In_ PCWSTR Template,
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

_Function_class_(PH_OPTIONS_FIND_SECTION)
PPH_OPTIONS_SECTION PhOptionsFindSection(
    _In_ PCPH_STRINGREF Name
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

        if (!PhGetClientRect(ContainerControl, &clientRect))
            return;

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

_Function_class_(PH_OPTIONS_ENTER_SECTION_VIEW)
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
        fontHexString = PhaGetStringSetting(SETTING_FONT);

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
        fontHexString = PhaGetStringSetting(SETTING_FONT_MONOSPACE);

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

_Function_class_(PH_ENUM_KEY_CALLBACK)
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

        for (SIZE_T i = 0; i < PhFinalArrayCount(&keyEntryArray); i++)
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
        static CONST PH_STRINGREF valueName = PH_STRINGREF_INIT(L"System Informer");
        static CONST PH_STRINGREF separator = PH_STRINGREF_INIT(L"\"");

        if (Present)
        {
            PPH_STRING value;
            PPH_STRING fileName;

            if (fileName = PhGetApplicationFileNameWin32())
            {
                value = PH_AUTO(PhConcatStringRef3(&separator, &fileName->sr, &separator));

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

        if (OldTaskMgrDebugger = PhQueryRegistryStringZ(taskmgrKeyHandle, L"Debugger"))
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
    {
        message = L"Do you want to restore the default Windows Task Manager?";
    }
    else
    {
        message = L"Do you want to make System Informer the default Windows Task Manager?";

        // Warn the user when we're not installed into secure location. (dmex)
        if (!PhShowOptionsDefaultInstallLocation(ParentWindowHandle, L"Changing the default Task Manager"))
        {
            return;
        }
    }

    if (PhShowMessage2(
        ParentWindowHandle,
        TD_YES_BUTTON | TD_NO_BUTTON,
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
            static CONST PH_STRINGREF valueName = PH_STRINGREF_INIT(L"Debugger");
            static CONST PH_STRINGREF separator = PH_STRINGREF_INIT(L"\"");

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
                    quotedFileName = PH_AUTO(PhConcatStringRef3(&separator, &applicationFileName->sr, &separator));

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

        //PhSaveSettings2(PhSettingsFileName);
    }
}

//BOOLEAN PhpIsExploitProtectionEnabled(
//    VOID
//    )
//{
//    BOOLEAN enabled = FALSE;
//    HANDLE keyHandle;
//    PPH_STRING directory = NULL;
//    PPH_STRING fileName = NULL;
//    PPH_STRING keyName;
//
//    PhMoveReference(&directory, PhCreateString2(&TaskMgrImageOptionsKeyName));
//    PhMoveReference(&directory, PhGetBaseDirectory(directory));
//    PhMoveReference(&fileName, PhGetApplicationFileNameWin32());
//    PhMoveReference(&fileName, PhGetBaseName(fileName));
//    keyName = PhConcatStringRef3(&directory->sr, &PhNtPathSeparatorString, &fileName->sr);
//
//    if (NT_SUCCESS(PhOpenKey(
//        &keyHandle,
//        KEY_READ,
//        PH_KEY_LOCAL_MACHINE,
//        &keyName->sr,
//        0
//        )))
//    {
//        PH_STRINGREF valueName;
//        PKEY_VALUE_PARTIAL_INFORMATION buffer;
//
//        enabled = !(PhQueryRegistryUlong64(keyHandle, L"MitigationOptions") == ULLONG_MAX);
//        PhInitializeStringRef(&valueName, L"MitigationOptions");
//
//        if (NT_SUCCESS(PhQueryValueKey(keyHandle, &valueName, KeyValuePartialInformation, &buffer)))
//        {
//            if (buffer->Type == REG_BINARY && buffer->DataLength)
//            {
//                enabled = TRUE;
//            }
//
//            PhFree(buffer);
//        }
//
//        NtClose(keyHandle);
//    }
//
//    PhDereferenceObject(keyName);
//    PhDereferenceObject(fileName);
//    PhDereferenceObject(directory);
//
//    return enabled;
//}
//
//NTSTATUS PhpSetExploitProtectionEnabled(
//    _In_ BOOLEAN Enabled)
//{
//    static PH_STRINGREF replacementToken = PH_STRINGREF_INIT(L"Software\\");
//    static PH_STRINGREF wow6432Token = PH_STRINGREF_INIT(L"Software\\WOW6432Node\\");
//    static PH_STRINGREF valueName = PH_STRINGREF_INIT(L"MitigationOptions");
//    NTSTATUS status = STATUS_UNSUCCESSFUL;
//    HANDLE keyHandle;
//    PPH_STRING directory;
//    PPH_STRING fileName;
//    PPH_STRING keyName;
//
//    directory = PhCreateString2(&TaskMgrImageOptionsKeyName);
//    PhMoveReference(&directory, PhGetBaseDirectory(directory));
//    fileName = PhGetApplicationFileNameWin32();
//    PhMoveReference(&fileName, PhGetBaseName(fileName));
//    keyName = PhConcatStringRef3(&directory->sr, &PhNtPathSeparatorString, &fileName->sr);
//
//    //if (Enabled)
//    //{
//    //    status = PhCreateKey(
//    //        &keyHandle,
//    //        KEY_WRITE,
//    //        PH_KEY_LOCAL_MACHINE,
//    //        &keyName->sr,
//    //        OBJ_OPENIF,
//    //        0,
//    //        NULL
//    //        );
//    //
//    //    if (NT_SUCCESS(status))
//    //    {
//    //        status = PhSetValueKey(keyHandle, &valueName, REG_QWORD, &(ULONG64)
//    //        {
//    //            PROCESS_CREATION_MITIGATION_POLICY_HEAP_TERMINATE_ALWAYS_ON |
//    //            PROCESS_CREATION_MITIGATION_POLICY_BOTTOM_UP_ASLR_ALWAYS_ON |
//    //            PROCESS_CREATION_MITIGATION_POLICY_HIGH_ENTROPY_ASLR_ALWAYS_ON |
//    //            PROCESS_CREATION_MITIGATION_POLICY_EXTENSION_POINT_DISABLE_ALWAYS_ON |
//    //            PROCESS_CREATION_MITIGATION_POLICY_PROHIBIT_DYNAMIC_CODE_ALWAYS_ON |
//    //            PROCESS_CREATION_MITIGATION_POLICY_CONTROL_FLOW_GUARD_ALWAYS_ON |
//    //            PROCESS_CREATION_MITIGATION_POLICY_FONT_DISABLE_ALWAYS_ON |
//    //            PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_NO_REMOTE_ALWAYS_ON |
//    //            PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_NO_LOW_LABEL_ALWAYS_ON |
//    //            PROCESS_CREATION_MITIGATION_POLICY_IMAGE_LOAD_PREFER_SYSTEM32_ALWAYS_ON,
//    //        }, sizeof(ULONG64));
//    //
//    //        NtClose(keyHandle);
//    //    }
//    //}
//    //else
//    {
//        status = PhOpenKey(
//            &keyHandle,
//            KEY_WRITE,
//            PH_KEY_LOCAL_MACHINE,
//            &keyName->sr,
//            OBJ_OPENIF
//            );
//
//        if (NT_SUCCESS(status))
//        {
//            status = PhDeleteValueKey(keyHandle, &valueName);
//            NtClose(keyHandle);
//        }
//
//        if (status == STATUS_OBJECT_NAME_NOT_FOUND)
//            status = STATUS_SUCCESS;
//
//#ifdef _WIN64
//        if (NT_SUCCESS(status))
//        {
//            PH_STRINGREF stringBefore;
//            PH_STRINGREF stringAfter;
//
//            if (PhSplitStringRefAtString(&keyName->sr, &replacementToken, TRUE, &stringBefore, &stringAfter))
//            {
//                PhMoveReference(&keyName, PhConcatStringRef3(&stringBefore, &wow6432Token, &stringAfter));
//
//                status = PhOpenKey(
//                    &keyHandle,
//                    DELETE,
//                    PH_KEY_LOCAL_MACHINE,
//                    &keyName->sr,
//                    OBJ_OPENIF
//                    );
//
//                if (NT_SUCCESS(status))
//                {
//                    status = NtDeleteKey(keyHandle);
//                    NtClose(keyHandle);
//                }
//            }
//        }
//#endif
//    }
//
//    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
//        status = STATUS_SUCCESS;
//
//    PhDereferenceObject(keyName);
//    PhDereferenceObject(fileName);
//    PhDereferenceObject(directory);
//
//    return status;
//}

NTSTATUS PhpSetSilentProcessNotifyEnabled(
    _In_ BOOLEAN Enabled)
{
    static CONST PH_STRINGREF processExitKeyName = PH_STRINGREF_INIT(L"Software\\Microsoft\\Windows NT\\CurrentVersion\\SilentProcessExit");
    static CONST PH_STRINGREF valueModeName = PH_STRINGREF_INIT(L"ReportingMode");
    static ULONG valueMode = 4;
    //static CONST PH_STRINGREF valueSelfName = PH_STRINGREF_INIT(L"IgnoreSelfExits");
    //static ULONG valueSelf = 1;
    //static CONST PH_STRINGREF valueMonitorName = PH_STRINGREF_INIT(L"MonitorProcess");
    static CONST PH_STRINGREF valueGlobalName = PH_STRINGREF_INIT(L"GlobalFlag");
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
            &valueMode,
            sizeof(ULONG)
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        //PhSetValueKey(keyFilenameHandle, &valueSelfName, REG_DWORD, &valueSelf, sizeof(ULONG));
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
            keyName = PH_AUTO(PhConcatStringRef3(&directory->sr, &PhNtPathSeparatorString, &fileName->sr));

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
                ULONG globalFlags = PhQueryRegistryUlongZ(keyHandle, L"GlobalFlag");

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
            keyName = PH_AUTO(PhConcatStringRef3(&directory->sr, &PhNtPathSeparatorString, &fileName->sr));

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
    PHP_OPTIONS_INDEX_ENABLE_MONOSPACE,
    PHP_OPTIONS_INDEX_ENABLE_PLUGINS,
    PHP_OPTIONS_INDEX_ENABLE_AVX_EXTENSIONS,
    PHP_OPTIONS_INDEX_ENABLE_UNDECORATE_SYMBOLS,
    PHP_OPTIONS_INDEX_ENABLE_COLUMN_HEADER_TOTALS,
    PHP_OPTIONS_INDEX_ENABLE_CYCLE_CPU_USAGE,
    PHP_OPTIONS_INDEX_ENABLE_GRAPH_SCALING,
    PHP_OPTIONS_INDEX_ENABLE_MINIINFO_WINDOW,
    PHP_OPTIONS_INDEX_ENABLE_MEMSTRINGS_TREE,
    PHP_OPTIONS_INDEX_ENABLE_LASTTAB_SUPPORT,
    PHP_OPTIONS_INDEX_ENABLE_THEME_SUPPORT,
    PHP_OPTIONS_INDEX_ENABLE_START_ASADMIN,
    PHP_OPTIONS_INDEX_ENABLE_STREAM_MODE,
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
    _In_ HWND hwndDlg,
    _In_ BOOLEAN ReloadOnly
    )
{
    HWND listViewHandle;

    listViewHandle = GetDlgItem(hwndDlg, IDC_SETTINGS);

    PhSetDialogItemValue(hwndDlg, IDC_SAMPLECOUNT, PhGetIntegerSetting(SETTING_SAMPLE_COUNT), FALSE);
    SetDlgItemCheckForSetting(hwndDlg, IDC_SAMPLECOUNTAUTOMATIC, SETTING_SAMPLE_COUNT_AUTOMATIC);

    if (PhGetIntegerSetting(SETTING_SAMPLE_COUNT_AUTOMATIC))
        EnableWindow(GetDlgItem(hwndDlg, IDC_SAMPLECOUNT), FALSE);

    if (!ReloadOnly)
    {
        PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_SINGLE_INSTANCE, L"Allow only one instance", NULL);
        PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_HIDE_WHENCLOSED, L"Hide when closed", NULL);
        PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_HIDE_WHENMINIMIZED, L"Hide when minimized", NULL);
        PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_START_ATLOGON, L"Start when I log on", NULL);
        PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_START_HIDDEN, L"Start hidden", NULL);
        PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_WARNINGS, L"Enable warnings", NULL);
        PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_DRIVER, L"Enable kernel-mode driver", NULL);
        PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_MONOSPACE, L"Enable monospace fonts", NULL);
        PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_PLUGINS, L"Enable plugins", NULL);
        PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_UNDECORATE_SYMBOLS, L"Enable undecorated symbols", NULL);
        PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_AVX_EXTENSIONS, L"Enable AVX extensions (experimental)", NULL);
        PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_COLUMN_HEADER_TOTALS, L"Enable column header totals (experimental)", NULL);
        PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_CYCLE_CPU_USAGE, L"Enable cycle-based CPU usage", NULL);
        PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_GRAPH_SCALING, L"Enable fixed graph scaling (experimental)", NULL);
        PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_MINIINFO_WINDOW, L"Enable tray information window", NULL);
        PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_MEMSTRINGS_TREE, L"Enable new memory strings dialog", NULL);
        PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_LASTTAB_SUPPORT, L"Remember last selected window", NULL);
        PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_THEME_SUPPORT, L"Enable theme support (experimental)", NULL);
        PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_START_ASADMIN, L"Enable start as admin (experimental)", NULL);
        PhAddListViewItem(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_STREAM_MODE, L"Enable streamer mode (disable window capture) (experimental)", NULL);
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
    }

    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_SINGLE_INSTANCE, SETTING_ALLOW_ONLY_ONE_INSTANCE);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_HIDE_WHENCLOSED, SETTING_HIDE_ON_CLOSE);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_HIDE_WHENMINIMIZED, SETTING_HIDE_ON_MINIMIZE);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_START_HIDDEN, SETTING_START_HIDDEN);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_MINIINFO_WINDOW, SETTING_MINI_INFO_WINDOW_ENABLED);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_MEMSTRINGS_TREE, SETTING_ENABLE_MEM_STRINGS_TREE_DIALOG);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_LASTTAB_SUPPORT, SETTING_MAIN_WINDOW_TAB_RESTORE_ENABLED);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_DRIVER, SETTING_KSI_ENABLE);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_WARNINGS, SETTING_ENABLE_WARNINGS);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_PLUGINS, SETTING_ENABLE_PLUGINS);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_UNDECORATE_SYMBOLS, SETTING_DBGHELP_UNDECORATE);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_AVX_EXTENSIONS, SETTING_ENABLE_AVX_SUPPORT);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_COLUMN_HEADER_TOTALS, SETTING_TREE_LIST_ENABLE_HEADER_TOTALS);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_GRAPH_SCALING, SETTING_ENABLE_GRAPH_MAX_SCALE);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_CYCLE_CPU_USAGE, SETTING_ENABLE_CYCLE_CPU_USAGE);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_THEME_SUPPORT, SETTING_ENABLE_THEME_SUPPORT);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_START_ASADMIN, SETTING_ENABLE_START_AS_ADMIN);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_STREAM_MODE, SETTING_ENABLE_STREAMER_MODE);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_MONOSPACE, SETTING_ENABLE_MONOSPACE_FONT);
    //SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_LINUX_SUPPORT, SETTING_ENABLE_LINUX_SUBSYSTEM_SUPPORT);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_NETWORK_RESOLVE, SETTING_ENABLE_NETWORK_RESOLVE);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_NETWORK_RESOLVE_DOH, SETTING_ENABLE_NETWORK_RESOLVE_DOH);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_INSTANT_TOOLTIPS, SETTING_ENABLE_INSTANT_TOOLTIPS);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_IMAGE_COHERENCY, SETTING_ENABLE_IMAGE_COHERENCY_SUPPORT);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_STAGE2, SETTING_ENABLE_STAGE2);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_SERVICE_STAGE2, SETTING_ENABLE_SERVICE_STAGE2);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ICON_SINGLE_CLICK, SETTING_ICON_SINGLE_CLICK);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_ICON_TOGGLE_VISIBILITY, SETTING_ICON_TOGGLES_VISIBILITY);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_PROPAGATE_CPU_USAGE, SETTING_PROPAGATE_CPU_USAGE);
    SetLvItemCheckForSetting(listViewHandle, PHP_OPTIONS_INDEX_SHOW_ADVANCED_OPTIONS, SETTING_ENABLE_ADVANCED_OPTIONS);

    if (CurrentUserRunPresent)
        ListView_SetCheckState(listViewHandle, PHP_OPTIONS_INDEX_START_ATLOGON, TRUE);
    if (PhGetIntegerSetting(SETTING_ENABLE_ADVANCED_OPTIONS))
        PhpOptionsShowHideTreeViewItem(FALSE);
}

static VOID PhpOptionsNotifyChangeCallback(
    _In_ PVOID Context
    )
{
    PhUpdateCachedSettings();
    SystemInformer_SaveAllSettings();
    PhInvalidateAllProcessNodes();
    PhReloadSettingsProcessTreeList();
    PhSiNotifyChangeSettings();

    //PhReInitializeWindowTheme(PhMainWndHandle);

    PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackSettingsUpdated), NULL);

    if (RestartRequired)
    {
        if (PhShowMessage2(
            PhMainWndHandle,
            TD_YES_BUTTON | TD_NO_BUTTON,
            TD_INFORMATION_ICON,
            L"One or more options you have changed requires a restart of System Informer.",
            L"Do you want to restart System Informer now?"
            ) == IDYES)
        {
            SystemInformer_PrepareForEarlyShutdown();

            if (NT_SUCCESS(PhShellProcessHacker(
                PhMainWndHandle,
                L"-v -newinstance",
                SW_SHOW,
                PH_SHELL_EXECUTE_DEFAULT,
                PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
                0,
                NULL
                )))
            {
                SystemInformer_Destroy();
            }
            else
            {
                SystemInformer_CancelEarlyShutdown();
            }
        }
    }
}

VOID PhShowOptionsRestartRequired(
    _In_ HWND WindowHandle
    )
{
    if (PhShowMessage2(
        PhMainWndHandle,
        TD_YES_BUTTON | TD_NO_BUTTON,
        TD_INFORMATION_ICON,
        L"One or more options you have changed requires a restart of System Informer.",
        L"Do you want to restart System Informer now?"
        ) == IDYES)
    {
        SystemInformer_PrepareForEarlyShutdown();

        if (NT_SUCCESS(PhShellProcessHacker(
            WindowHandle,
            L"-v -newinstance",
            SW_SHOW,
            PH_SHELL_EXECUTE_DEFAULT,
            PH_SHELL_APP_PROPAGATE_PARAMETERS | PH_SHELL_APP_PROPAGATE_PARAMETERS_IGNORE_VISIBILITY,
            0,
            NULL
            )))
        {
            SystemInformer_Destroy();
        }
        else
        {
            SystemInformer_CancelEarlyShutdown();
        }
    }
}

BOOLEAN PhShowOptionsDefaultInstallLocation(
    _In_ HWND ParentWindowHandle,
    _In_ PCWSTR Message
    )
{
    RTL_ELEVATION_FLAGS flags;

    // Warn the user when we're not installed into secure location. (dmex)
    if (NT_SUCCESS(RtlQueryElevationFlags(&flags)) && flags.ElevationEnabled)
    {
        PPH_STRING applicationFileName;
        PPH_STRING programFilesPath;

        if (applicationFileName = PhGetApplicationFileNameWin32())
        {
            if (programFilesPath = PhExpandEnvironmentStringsZ(L"%ProgramFiles%\\"))
            {
                if (!PhStartsWithString(applicationFileName, programFilesPath, TRUE))
                {
                    if (PhShowMessage2(
                        ParentWindowHandle,
                        TD_YES_BUTTON | TD_NO_BUTTON,
                        TD_WARNING_ICON,
                        L"WARNING: You have not installed System Informer into a secure location.",
                        L"%s is not recommended when running System Informer from outside a secure location (e.g. Program Files).\r\n\r\nAre you sure you want to continue?",
                        Message
                        ) == IDNO)
                    {
                        PhDereferenceObject(programFilesPath);
                        PhDereferenceObject(applicationFileName);
                        return FALSE;
                    }
                }

                PhDereferenceObject(programFilesPath);
            }

            PhDereferenceObject(applicationFileName);
        }
    }

    return TRUE;
}

static VOID PhpAdvancedPageSave(
    _In_ HWND hwndDlg
    )
{
    HWND listViewHandle;
    ULONG sampleCount;

    listViewHandle = GetDlgItem(hwndDlg, IDC_SETTINGS);
    sampleCount = PhGetDialogItemValue(hwndDlg, IDC_SAMPLECOUNT);

    SetSettingForDlgItemCheckRestartRequired(hwndDlg, IDC_SAMPLECOUNTAUTOMATIC, SETTING_SAMPLE_COUNT_AUTOMATIC);

    if (sampleCount == 0)
        sampleCount = 1;

    if (sampleCount != PhGetIntegerSetting(SETTING_SAMPLE_COUNT))
        RestartRequired = TRUE;

    PhSetIntegerSetting(SETTING_SAMPLE_COUNT, sampleCount);
    PhSetStringSetting2(SETTING_SEARCH_ENGINE, &PhaGetDlgItemText(hwndDlg, IDC_SEARCHENGINE)->sr);
    PhSetStringSetting2(SETTING_PROGRAM_INSPECT_EXECUTABLES, &PhaGetDlgItemText(hwndDlg, IDC_PEVIEWER)->sr);
    PhSetIntegerSetting(SETTING_MAX_SIZE_UNIT, PhMaxSizeUnit = ComboBox_GetCurSel(GetDlgItem(hwndDlg, IDC_MAXSIZEUNIT)));
    PhSetIntegerSetting(SETTING_ICON_PROCESSES, PhGetDialogItemValue(hwndDlg, IDC_ICONPROCESSES));

    if (!PhEqualString(PhaGetDlgItemText(hwndDlg, IDC_DBGHELPSEARCHPATH), PhaGetStringSetting(SETTING_DBGHELP_SEARCH_PATH), TRUE))
    {
        PhSetStringSetting2(SETTING_DBGHELP_SEARCH_PATH, &PhaGetDlgItemText(hwndDlg, IDC_DBGHELPSEARCHPATH)->sr);
        RestartRequired = TRUE;
    }

    // When changing driver enabled setting, it only makes sense to require a restart if we're
    // already elevated. If we're not elevated and asked to restart, we would not connect to the
    // driver and the user has to elevate (restart) again anyway. (jxy-s)
    if (PhGetOwnTokenAttributes().Elevated)
    {
        SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_DRIVER, SETTING_KSI_ENABLE);
    }
    else
    {
        SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_DRIVER, SETTING_KSI_ENABLE);
    }

    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_SINGLE_INSTANCE, SETTING_ALLOW_ONLY_ONE_INSTANCE);
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_HIDE_WHENCLOSED, SETTING_HIDE_ON_CLOSE);
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_HIDE_WHENMINIMIZED, SETTING_HIDE_ON_MINIMIZE);
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_START_HIDDEN, SETTING_START_HIDDEN);
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_MINIINFO_WINDOW, SETTING_MINI_INFO_WINDOW_ENABLED);
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_MEMSTRINGS_TREE, SETTING_ENABLE_MEM_STRINGS_TREE_DIALOG);
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_LASTTAB_SUPPORT, SETTING_MAIN_WINDOW_TAB_RESTORE_ENABLED);
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_WARNINGS, SETTING_ENABLE_WARNINGS);
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_PLUGINS, SETTING_ENABLE_PLUGINS);
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_UNDECORATE_SYMBOLS, SETTING_DBGHELP_UNDECORATE);
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_AVX_EXTENSIONS, SETTING_ENABLE_AVX_SUPPORT);
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_COLUMN_HEADER_TOTALS, SETTING_TREE_LIST_ENABLE_HEADER_TOTALS);
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_GRAPH_SCALING, SETTING_ENABLE_GRAPH_MAX_SCALE);
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_CYCLE_CPU_USAGE, SETTING_ENABLE_CYCLE_CPU_USAGE);
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_THEME_SUPPORT, SETTING_ENABLE_THEME_SUPPORT);
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_START_ASADMIN, SETTING_ENABLE_START_AS_ADMIN);
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_STREAM_MODE, SETTING_ENABLE_STREAMER_MODE);
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_MONOSPACE, SETTING_ENABLE_MONOSPACE_FONT);
    //SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_LINUX_SUPPORT, SETTING_ENABLE_LINUX_SUBSYSTEM_SUPPORT);
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_NETWORK_RESOLVE, SETTING_ENABLE_NETWORK_RESOLVE);
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_NETWORK_RESOLVE_DOH, SETTING_ENABLE_NETWORK_RESOLVE_DOH);
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_INSTANT_TOOLTIPS, SETTING_ENABLE_INSTANT_TOOLTIPS);
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_IMAGE_COHERENCY, SETTING_ENABLE_IMAGE_COHERENCY_SUPPORT);
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_STAGE2, SETTING_ENABLE_STAGE2);
    SetSettingForLvItemCheckRestartRequired(listViewHandle, PHP_OPTIONS_INDEX_ENABLE_SERVICE_STAGE2, SETTING_ENABLE_SERVICE_STAGE2);
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ICON_SINGLE_CLICK, SETTING_ICON_SINGLE_CLICK);
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_ICON_TOGGLE_VISIBILITY, SETTING_ICON_TOGGLES_VISIBILITY);
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_PROPAGATE_CPU_USAGE, SETTING_PROPAGATE_CPU_USAGE);
    SetSettingForLvItemCheck(listViewHandle, PHP_OPTIONS_INDEX_SHOW_ADVANCED_OPTIONS, SETTING_ENABLE_ADVANCED_OPTIONS);

    if (PhGetIntegerSetting(SETTING_ENABLE_THEME_SUPPORT)) // PhGetIntegerSetting required (dmex)
    {
        PhSetIntegerSetting(SETTING_GRAPH_COLOR_MODE, 1); // HACK switch to dark theme. (dmex)
    }

    WriteCurrentUserRun(
        ListView_GetCheckState(listViewHandle, PHP_OPTIONS_INDEX_START_ATLOGON) == BST_CHECKED,
        ListView_GetCheckState(listViewHandle, PHP_OPTIONS_INDEX_START_HIDDEN) == BST_CHECKED
        );

    SystemInformer_Invoke(PhpOptionsNotifyChangeCallback, NULL);
}

_Function_class_(USER_THREAD_START_ROUTINE)
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

            PhpOptionsSetImageList(ListViewHandle, FALSE);

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

            PhSetDialogItemText(hwndDlg, IDC_SEARCHENGINE, PhaGetStringSetting(SETTING_SEARCH_ENGINE)->Buffer);
            PhSetDialogItemText(hwndDlg, IDC_PEVIEWER, PhaGetStringSetting(SETTING_PROGRAM_INSPECT_EXECUTABLES)->Buffer);
            PhSetDialogItemValue(hwndDlg, IDC_ICONPROCESSES, PhGetIntegerSetting(SETTING_ICON_PROCESSES), FALSE);
            PhSetDialogItemText(hwndDlg, IDC_DBGHELPSEARCHPATH, PhaGetStringSetting(SETTING_DBGHELP_SEARCH_PATH)->Buffer);

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
            PhpAdvancedPageLoad(hwndDlg, FALSE);
            PhpRefreshTaskManagerState(hwndDlg);
            GeneralListViewStateInitializing = FALSE;
        }
        break;
    case WM_DESTROY:
        {
            if (NewFontSelection)
            {
                PhSetStringSetting2(SETTING_FONT, &NewFontSelection->sr);
            }

            if (NewFontMonospaceSelection)
            {
                PhSetStringSetting2(SETTING_FONT_MONOSPACE, &NewFontMonospaceSelection->sr);
            }

            if (NewFontSelection || NewFontMonospaceSelection)
            {
                SystemInformer_UpdateFont();
            }

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
    case WM_DPICHANGED_AFTERPARENT:
        {
            PhLayoutManagerUpdate(&LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&LayoutManager);

            PhpOptionsSetImageList(ListViewHandle, FALSE);
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
                        GetObject(SystemInformer_GetFont(), sizeof(LOGFONT), &font);
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
                        ExtendedListView_SetRedraw(ListViewHandle, FALSE);
                        ListView_DeleteAllItems(ListViewHandle);
                        PhpAdvancedPageLoad(hwndDlg, FALSE);
                        ExtendedListView_SetRedraw(ListViewHandle, TRUE);
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
                        //GetObject(SystemInformer_GetFont(), sizeof(LOGFONT), &font);
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
                        ExtendedListView_SetRedraw(ListViewHandle, FALSE);
                        ListView_DeleteAllItems(ListViewHandle);
                        PhpAdvancedPageLoad(hwndDlg, FALSE);
                        ExtendedListView_SetRedraw(ListViewHandle, TRUE);
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

                    if (ListView_HitTest(ListViewHandle, &lvHitInfo) != -1)
                    {
                        // Ignore click notifications for the listview checkbox region.
                        if (!(lvHitInfo.flags & LVHT_ONITEMSTATEICON))
                        {
                            BOOLEAN itemChecked;

                            // Emulate the checkbox control label click behavior and check/uncheck the checkbox when the listview item is clicked.
                            itemChecked = ListView_GetCheckState(ListViewHandle, itemActivate->iItem) == BST_CHECKED;
                            ListView_SetCheckState(ListViewHandle, itemActivate->iItem, !itemChecked);
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
                        case INDEXTOSTATEIMAGEMASK(2): // checked
                            {
                                switch (listView->iItem)
                                {
                                case PHP_OPTIONS_INDEX_HIDE_WHENCLOSED:
                                case PHP_OPTIONS_INDEX_HIDE_WHENMINIMIZED:
                                case PHP_OPTIONS_INDEX_START_HIDDEN:
                                    {
                                        if (!PhNfIconsEnabled())
                                        {
                                            PhShowInformation2(
                                                PhOptionsWindowHandle,
                                                L"Unable to configure this option.",
                                                L"%s",
                                                L"You need to enable at minimum one tray icon (View menu > Tray Icons) before enabling the hide option."
                                                );
                                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                                            return TRUE;
                                        }
                                    }
                                    break;
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
                                            static const PH_STRINGREF separator = PH_STRINGREF_INIT(L"\"");
                                            HRESULT status;
                                            PPH_STRING quotedFileName;

                                            if (!PhShowOptionsDefaultInstallLocation(PhOptionsWindowHandle, L"Enabling the 'start as admin' option"))
                                            {
                                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                                                return TRUE;
                                            }

                                            quotedFileName = PH_AUTO(PhConcatStringRef3(
                                                &separator,
                                                &applicationFileName->sr,
                                                &separator
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
                                                    HRESULT_CODE(status)
                                                    );

                                                PhDereferenceObject(applicationFileName);
                                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);
                                                return TRUE;
                                            }

                                            PhDereferenceObject(applicationFileName);
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
                                        PhSetIntegerSetting(SETTING_ENABLE_ADVANCED_OPTIONS, TRUE);
                                        PhpOptionsShowHideTreeViewItem(FALSE);
                                    }
                                    break;
                                case PHP_OPTIONS_INDEX_ENABLE_START_ASADMIN:
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
                                        PhSetIntegerSetting(SETTING_ENABLE_ADVANCED_OPTIONS, FALSE);
                                        PhpOptionsShowHideTreeViewItem(TRUE);
                                    }
                                    break;
                                case PHP_OPTIONS_INDEX_ENABLE_START_ASADMIN:
                                    {
                                        if (PhGetIntegerSetting(SETTING_ENABLE_START_AS_ADMIN))
                                        {
                                            PhDeleteAdminTask(&SI_RUNAS_ADMIN_TASK_NAME);
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
    case WM_DPICHANGED_AFTERPARENT:
        {
            PhLayoutManagerUpdate(&LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&LayoutManager);
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

                    PhReloadGeneralSection();

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
    ULONG_PTR SearchMatchHandle;
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
#define BEGIN_SORT_FUNCTION(Column) static long __cdecl OptionsAdvancedTreeNewCompare##Column( \
    _In_ void *_context, \
    _In_ const void *_elem1, \
    _In_ const void *_elem2 \
    ) \
{ \
    PPH_OPTIONS_ADVANCED_ROOT_NODE node1 = *(PPH_OPTIONS_ADVANCED_ROOT_NODE*)_elem1; \
    PPH_OPTIONS_ADVANCED_ROOT_NODE node2 = *(PPH_OPTIONS_ADVANCED_ROOT_NODE*)_elem2; \
    LONG sortResult = 0;

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

    settings = PhGetStringSetting(SETTING_OPTIONS_WINDOW_ADVANCED_COLUMNS);
    Context->Flags = PhGetIntegerSetting(SETTING_OPTIONS_WINDOW_ADVANCED_FLAGS);
    PhCmLoadSettings(Context->TreeNewHandle, &settings->sr);
    PhDereferenceObject(settings);
}

VOID OptionsAdvancedSaveSettingsTreeList(
    _Inout_ PPH_OPTIONS_ADVANCED_CONTEXT Context
    )
{
    PPH_STRING settings;

    settings = PhCmSaveSettings(Context->TreeNewHandle);
    PhSetStringSetting2(SETTING_OPTIONS_WINDOW_ADVANCED_COLUMNS, &settings->sr);
    PhSetIntegerSetting(SETTING_OPTIONS_WINDOW_ADVANCED_FLAGS, Context->Flags);
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

_Function_class_(PH_HASHTABLE_EQUAL_FUNCTION)
BOOLEAN OptionsAdvancedNodeHashtableEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPH_OPTIONS_ADVANCED_ROOT_NODE node1 = *(PPH_OPTIONS_ADVANCED_ROOT_NODE*)Entry1;
    PPH_OPTIONS_ADVANCED_ROOT_NODE node2 = *(PPH_OPTIONS_ADVANCED_ROOT_NODE*)Entry2;

    return PhEqualStringRef(&node1->Name->sr, &node2->Name->sr, TRUE);
}

_Function_class_(PH_HASHTABLE_HASH_FUNCTION)
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
                long (__cdecl* sortFunction)(void*, const void*, const void*);

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
            PPH_TREENEW_SORT_CHANGED_EVENT sorting = Parameter1;

            context->TreeNewSortColumn = sorting->SortColumn;
            context->TreeNewSortOrder = sorting->SortOrder;

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
    TreeNew_SetRedraw(Context->TreeNewHandle, FALSE);
    TreeNew_SetCallback(Context->TreeNewHandle, OptionsAdvancedTreeNewCallback, Context);

    PhAddTreeNewColumnEx(Context->TreeNewHandle, PH_OPTIONS_ADVANCED_COLUMN_ITEM_NAME, TRUE, L"Name", 200, PH_ALIGN_LEFT, 0, 0, TRUE);
    PhAddTreeNewColumnEx(Context->TreeNewHandle, PH_OPTIONS_ADVANCED_COLUMN_ITEM_TYPE, TRUE, L"Type", 100, PH_ALIGN_LEFT, 1, 0, TRUE);
    PhAddTreeNewColumnEx(Context->TreeNewHandle, PH_OPTIONS_ADVANCED_COLUMN_ITEM_VALUE, TRUE, L"Value", 200, PH_ALIGN_LEFT, 2, 0, TRUE);
    PhAddTreeNewColumnEx(Context->TreeNewHandle, PH_OPTIONS_ADVANCED_COLUMN_ITEM_DEFAULT, TRUE, L"Default", 200, PH_ALIGN_LEFT, 3, 0, TRUE);

    PhInitializeTreeNewFilterSupport(&Context->TreeFilterSupport, Context->TreeNewHandle, Context->NodeList);

    TreeNew_SetTriState(Context->TreeNewHandle, TRUE);
    TreeNew_SetRedraw(Context->TreeNewHandle, TRUE);

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

    if (!context->SearchMatchHandle)
        return TRUE;

    if (PhSearchControlMatch(context->SearchMatchHandle, &node->Name->sr))
        return TRUE;
    if (PhSearchControlMatch(context->SearchMatchHandle, &node->DefaultString->sr))
        return TRUE;
    if (PhSearchControlMatch(context->SearchMatchHandle, &node->ValueString->sr))
        return TRUE;

    return FALSE;
}

VOID NTAPI PhpOptionsAdvancedSearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    PPH_OPTIONS_ADVANCED_CONTEXT context = Context;

    assert(context);

    context->SearchMatchHandle = MatchHandle;

    if (!context->SearchMatchHandle)
    {
        // Expand the nodes?
    }

    PhApplyTreeNewFilters(&context->TreeFilterSupport);
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

            PhCreateSearchControl(
                hwndDlg,
                context->SearchBoxHandle,
                L"Search settings...",
                PhpOptionsAdvancedSearchControlCallback,
                context
                );

            InitializeOptionsAdvancedTree(context);
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
            PhDeleteLayoutManager(&context->LayoutManager);

            PhRemoveTreeNewFilter(&context->TreeFilterSupport, context->TreeFilterEntry);
            DeleteOptionsAdvancedTree(context);

            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
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

                    if (!PhGetWindowRect(GetDlgItem(hwndDlg, IDC_FILTEROPTIONS), &rect))
                        break;

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
                        PhDialogBox(
                            PhInstanceHandle,
                            MAKEINTRESOURCE(IDD_EDITENV),
                            hwndDlg,
                            PhpOptionsAdvancedEditDlgProc,
                            node->Setting
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
            case IDC_RESET:
                {
                    PPH_OPTIONS_ADVANCED_ROOT_NODE* nodes;
                    ULONG numberOfNodes;
                    if (!GetSelectedOptionsAdvancedNodes(context, &nodes, &numberOfNodes))
                        break;
                    for (ULONG i = 0; i < numberOfNodes; i++)
                    {
                        PhSettingFromString(
                            nodes[i]->Setting->Type,
                            &nodes[i]->Setting->DefaultValue,
                            NULL,
                            nodes[i]->Setting
                            );
                        PhMoveReference(
                            &nodes[i]->ValueString,
                            PhSettingToString(nodes[i]->Setting->Type, nodes[i]->Setting)
                            );
                    }
                    TreeNew_NodesStructured(context->TreeNewHandle);
                    PhApplyTreeNewFilters(&context->TreeFilterSupport);

                    PhReloadGeneralSection();
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
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_RESET, L"&Reset", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
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
    PCWSTR SettingName;
    PCWSTR UseSettingName;
    PCWSTR Name;
    PCWSTR Description;

    BOOLEAN CurrentUse;
    COLORREF CurrentColor;
} COLOR_ITEM, *PCOLOR_ITEM;

#define COLOR_ITEM(SettingName, Name, Description) { SettingName, L"Use" SettingName, Name, Description }

static COLOR_ITEM ColorItems[] =
{
    COLOR_ITEM(SETTING_COLOR_OWN_PROCESSES, L"Own processes", L"Processes running under the same user account as System Informer."),
    COLOR_ITEM(SETTING_COLOR_SYSTEM_PROCESSES, L"System processes", L"Processes running under the NT AUTHORITY\\SYSTEM user account."),
    COLOR_ITEM(SETTING_COLOR_SERVICE_PROCESSES, L"Service processes", L"Processes which host one or more services."),
    COLOR_ITEM(SETTING_COLOR_BACKGROUND_PROCESSES, L"Background processes", L"Processes with a background scheduling priority."),
    COLOR_ITEM(SETTING_COLOR_JOB_PROCESSES, L"Job processes", L"Processes associated with a job."),
#ifdef _WIN64
    COLOR_ITEM(SETTING_COLOR_WOW64_PROCESSES, L"32-bit processes", L"Processes running under WOW64, i.e. 32-bit."),
#endif
    COLOR_ITEM(SETTING_COLOR_DEBUGGED_PROCESSES, L"Debugged processes", L"Processes that are currently being debugged."),
    COLOR_ITEM(SETTING_COLOR_ELEVATED_PROCESSES, L"Elevated processes", L"Processes with full privileges on a system with UAC enabled."),
    COLOR_ITEM(SETTING_COLOR_UI_ACCESS_PROCESSES, L"UIAccess processes", L"Processes with UIAccess privileges."),
    COLOR_ITEM(SETTING_COLOR_PICO_PROCESSES, L"Pico processes", L"Processes that belong to the Windows subsystem for Linux."),
    COLOR_ITEM(SETTING_COLOR_IMMERSIVE_PROCESSES, L"Immersive processes and DLLs", L"Processes and DLLs that belong to a Modern UI app."),
    COLOR_ITEM(SETTING_COLOR_SUSPENDED, L"Suspended processes and threads", L"Processes and threads that are suspended from execution."),
    COLOR_ITEM(SETTING_COLOR_PARTIALLY_SUSPENDED, L"Partially suspended processes and threads", L"Processes and threads that are partially suspended from execution."),
    COLOR_ITEM(SETTING_COLOR_DOT_NET, L".NET processes and DLLs", L".NET (i.e. managed) processes and DLLs."),
    COLOR_ITEM(SETTING_COLOR_PACKED, L"Packed processes", L"Executables are sometimes \"packed\" to reduce their size."),
    COLOR_ITEM(SETTING_COLOR_LOW_IMAGE_COHERENCY, L"Low process image coherency", L"The image file backing the process has low coherency when compared to the mapped image."),
    COLOR_ITEM(SETTING_COLOR_GUI_THREADS, L"GUI threads", L"Threads that have made at least one GUI-related system call."),
    COLOR_ITEM(SETTING_COLOR_RELOCATED_MODULES, L"Relocated DLLs", L"DLLs that were not loaded at their preferred image bases."),
    COLOR_ITEM(SETTING_COLOR_PROTECTED_HANDLES, L"Protected handles", L"Handles that are protected from being closed."),
    COLOR_ITEM(SETTING_COLOR_PROTECTED_PROCESS, L"Protected processes", L"Processes with built-in protection levels."),
    COLOR_ITEM(SETTING_COLOR_INHERIT_HANDLES, L"Inheritable handles", L"Handles that can be inherited by child processes."),
    COLOR_ITEM(SETTING_COLOR_HANDLE_FILTERED, L"Filtered processes", L"Processes that are protected by handle object callbacks."),
    COLOR_ITEM(SETTING_COLOR_UNKNOWN, L"Untrusted DLLs and Services", L"Services and DLLs which are not digitally signed."),
    COLOR_ITEM(SETTING_COLOR_SERVICE_DISABLED, L"Disabled Services", L"Services which have been disabled."),
    //COLOR_ITEM(SETTING_COLOR_SERVICE_STOP, L"Stopped Services", L"Services that are not running.")
    COLOR_ITEM(SETTING_COLOR_EFFICIENCY_MODE, L"Power efficiency", L"Processes and threads with power efficiency."),
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
    case WM_DPICHANGED_AFTERPARENT:
        {
            PhLayoutManagerUpdate(&LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&LayoutManager);
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

                            PhLoadCustomColorList(SETTING_OPTIONS_CUSTOM_COLOR_LIST, customColors, RTL_NUMBER_OF(customColors));

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

                            PhSaveCustomColorList(SETTING_OPTIONS_CUSTOM_COLOR_LIST, customColors, RTL_NUMBER_OF(customColors));

                            ListView_SetItemState(HighlightingListViewHandle, -1, 0, LVIS_SELECTED);
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
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == HighlightingListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PCOLOR_ITEM ColorItem;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(HighlightingListViewHandle, &point);

                if (ColorItem = PhGetSelectedListViewItemParam(HighlightingListViewHandle))
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_RESET, L"&Reset", NULL, NULL), ULONG_MAX);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item && item->Id == IDC_RESET)
                    {
                        PH_STRINGREF SettingName;
                        PH_STRINGREF UseSettingName;
                        PPH_SETTING Color;
                        PPH_SETTING UseColor;

                        PhInitializeStringRef(&SettingName, ColorItem->SettingName);
                        PhInitializeStringRef(&UseSettingName, ColorItem->UseSettingName);
                        Color = PhGetSetting(&SettingName);
                        UseColor = PhGetSetting(&UseSettingName);

                        PhSettingFromString(Color->Type, &Color->DefaultValue, NULL, Color);
                        PhSettingFromString(UseColor->Type, &UseColor->DefaultValue, NULL, UseColor);

                        ColorItem->CurrentColor = Color->u.Integer;
                        ColorItem->CurrentUse = !!UseColor->u.Integer;

                        INT index = PhFindListViewItemByParam(HighlightingListViewHandle, INT_ERROR, ColorItem);
                        ListView_SetCheckState(HighlightingListViewHandle, index, ColorItem->CurrentUse);
                        ListView_SetItemState(HighlightingListViewHandle, index, 0, LVIS_SELECTED);
                    }

                    PhDestroyEMenu(menu);
                }
            }
            else if ((HWND)wParam == GetDlgItem(hwndDlg, IDC_NEWOBJECTS) || (HWND)wParam == GetDlgItem(hwndDlg, IDC_REMOVEDOBJECTS))
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                menu = PhCreateEMenu();
                PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_RESET, L"&Reset", NULL, NULL), ULONG_MAX);

                item = PhShowEMenu(
                    menu,
                    hwndDlg,
                    PH_EMENU_SHOW_LEFTRIGHT,
                    PH_ALIGN_LEFT | PH_ALIGN_TOP,
                    point.x,
                    point.y
                    );

                if (item && item->Id == IDC_RESET)
                {
                    PH_STRINGREF SettingName;
                    PPH_SETTING Color;
                    BOOLEAN setNew = (HWND)wParam == GetDlgItem(hwndDlg, IDC_NEWOBJECTS);

                    PhInitializeStringRef(&SettingName, setNew ? SETTING_COLOR_NEW : SETTING_COLOR_REMOVED);
                    Color = PhGetSetting(&SettingName);

                    PhSettingFromString(Color->Type, &Color->DefaultValue, NULL, Color);

                    ColorBox_SetColor(GetDlgItem(hwndDlg, setNew ? IDC_NEWOBJECTS : IDC_REMOVEDOBJECTS), Color->u.Integer);
                    InvalidateRect(GetDlgItem(hwndDlg, setNew ? IDC_NEWOBJECTS : IDC_REMOVEDOBJECTS), NULL, TRUE);
                }

                PhDestroyEMenu(menu);
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

static COLOR_ITEM PhpOptionsGraphColorItems[] =
{
    COLOR_ITEM(SETTING_COLOR_CPU_KERNEL, L"CPU kernel", L"CPU kernel"),
    COLOR_ITEM(SETTING_COLOR_CPU_USER, L"CPU user", L"CPU user"),
    COLOR_ITEM(SETTING_COLOR_IO_READ_OTHER, L"I/O R+O", L"I/O R+O"),
    COLOR_ITEM(SETTING_COLOR_IO_WRITE, L"I/O W", L"I/O W"),
    COLOR_ITEM(SETTING_COLOR_PRIVATE, L"Private bytes", L"Private bytes"),
    COLOR_ITEM(SETTING_COLOR_PHYSICAL, L"Physical memory", L"Physical memory"),
    COLOR_ITEM(SETTING_COLOR_POWER_USAGE, L"Power usage", L"Power usage"),
    COLOR_ITEM(SETTING_COLOR_TEMPERATURE, L"Temperature", L"Temperature"),
    COLOR_ITEM(SETTING_COLOR_FAN_RPM, L"Fan RPM", L"Fan RPM"),
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
            SetDlgItemCheckForSetting(hwndDlg, IDC_SHOWTEXT, SETTING_GRAPH_SHOW_TEXT);
            SetDlgItemCheckForSetting(hwndDlg, IDC_USEOLDCOLORS, SETTING_GRAPH_COLOR_MODE);
            SetDlgItemCheckForSetting(hwndDlg, IDC_SHOWCOMMITINSUMMARY, SETTING_SHOW_COMMIT_IN_SUMMARY);

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

            if (PhGetIntegerSetting(SETTING_GRAPH_COLOR_MODE))
                EnableWindow(PhpGraphListViewHandle, TRUE);
            else if (PhEnableThemeSupport)
            {
                ShowWindow(PhpGraphListViewHandle, SW_HIDE);
                EnableWindow(PhpGraphListViewHandle, TRUE);
            }
        }
        break;
    case WM_DESTROY:
        {
            SetSettingForDlgItemCheck(hwndDlg, IDC_SHOWTEXT, SETTING_GRAPH_SHOW_TEXT);
            SetSettingForDlgItemCheck(hwndDlg, IDC_USEOLDCOLORS, SETTING_GRAPH_COLOR_MODE);
            SetSettingForDlgItemCheck(hwndDlg, IDC_SHOWCOMMITINSUMMARY, SETTING_SHOW_COMMIT_IN_SUMMARY);

            for (ULONG i = 0; i < RTL_NUMBER_OF(PhpOptionsGraphColorItems); i++)
            {
                PhSetIntegerSetting(PhpOptionsGraphColorItems[i].SettingName, PhpOptionsGraphColorItems[i].CurrentColor);
            }

            PhDeleteLayoutManager(&LayoutManager);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            PhLayoutManagerUpdate(&LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&LayoutManager);
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

                    if (PhEnableThemeSupport)
                        ShowWindow(PhpGraphListViewHandle, Button_GetCheck(GET_WM_COMMAND_HWND(wParam, lParam)) == BST_CHECKED ? SW_SHOW : SW_HIDE);
                    else
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

                            PhLoadCustomColorList(SETTING_OPTIONS_CUSTOM_COLOR_LIST, customColors, RTL_NUMBER_OF(customColors));

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

                            PhSaveCustomColorList(SETTING_OPTIONS_CUSTOM_COLOR_LIST, customColors, RTL_NUMBER_OF(customColors));

                            ListView_SetItemState(PhpGraphListViewHandle, -1, 0, LVIS_SELECTED);
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
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == PhpGraphListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PCOLOR_ITEM ColorItem;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(PhpGraphListViewHandle, &point);

                if (ColorItem = PhGetSelectedListViewItemParam(PhpGraphListViewHandle))
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_RESET, L"&Reset", NULL, NULL), ULONG_MAX);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item && item->Id == IDC_RESET)
                    {
                        PH_STRINGREF SettingName;
                        PPH_SETTING Color;

                        PhInitializeStringRef(&SettingName, ColorItem->SettingName);
                        Color = PhGetSetting(&SettingName);

                        PhSettingFromString(Color->Type, &Color->DefaultValue, NULL, Color);

                        ColorItem->CurrentColor = Color->u.Integer;

                        ListView_SetItemState(PhpGraphListViewHandle, -1, 0, LVIS_SELECTED);
                    }

                    PhDestroyEMenu(menu);
                }
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
