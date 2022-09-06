/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2020-2022
 *
 */

#include <peview.h>

typedef enum _PHP_OPTIONS_INDEX
{
    PHP_OPTIONS_INDEX_ENABLE_THEME_SUPPORT,
    PHP_OPTIONS_INDEX_ENABLE_LEGACY_TABS,
    PHP_OPTIONS_INDEX_ENABLE_THEME_BORDER,
    PHP_OPTIONS_INDEX_ENABLE_LASTTAB_SUPPORT,
} PHP_OPTIONS_GENERAL_INDEX;

typedef struct _PVP_PE_OPTIONS_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HWND ComboHandle;
    PH_LAYOUT_MANAGER LayoutManager;
} PVP_PE_OPTIONS_CONTEXT, *PPVP_PE_OPTIONS_CONTEXT;

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
    static PH_STRINGREF seperator = PH_STRINGREF_INIT(L"\"");
    BOOLEAN result;
    PPH_STRING filename;
    PPH_STRING parameters;

    if (!(filename = PhGetApplicationFileNameWin32()))
        return FALSE;

    parameters = PhConcatStringRef3(
        &seperator,
        &PvFileName->sr,
        &seperator
        );

    result = PhShellExecuteEx(
        WindowHandle,
        PhGetString(filename),
        PhGetString(parameters),
        SW_SHOW,
        0,
        0,
        NULL
        );

    PhDereferenceObject(parameters);
    PhDereferenceObject(filename);

    return result;
}

VOID PvLoadGeneralPage(
    _In_ PPVP_PE_OPTIONS_CONTEXT Context
    )
{
    PhSetDialogItemText(Context->WindowHandle, IDC_DBGHELPSEARCHPATH, PhaGetStringSetting(L"DbgHelpSearchPath")->Buffer);

    //PhAddListViewItem(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_WARNINGS, L"Enable warnings", NULL);
    //PhAddListViewItem(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_PLUGINS, L"Enable plugins", NULL);
    //PhAddListViewItem(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_UNDECORATE_SYMBOLS, L"Enable undecorated symbols", NULL);
    PhAddListViewItem(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_THEME_SUPPORT, L"Enable theme support", NULL);
    //PhAddListViewItem(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_START_ASADMIN, L"Enable start as admin", NULL);
    //PhAddListViewItem(Context->ListViewHandle, PHP_OPTIONS_INDEX_SHOW_ADVANCED_OPTIONS, L"Show advanced options", NULL);
    PhAddListViewItem(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_LEGACY_TABS, L"Enable legacy properties window", NULL);
    PhAddListViewItem(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_THEME_BORDER, L"Enable view borders", NULL);
    PhAddListViewItem(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_LASTTAB_SUPPORT, L"Remember last selected window", NULL);

    //SetLvItemCheckForSetting(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_WARNINGS, L"EnableWarnings");
    //SetLvItemCheckForSetting(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_PLUGINS, L"EnablePlugins");
    //SetLvItemCheckForSetting(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_UNDECORATE_SYMBOLS, L"DbgHelpUndecorate");
    SetLvItemCheckForSetting(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_THEME_SUPPORT, L"EnableThemeSupport");
    //SetLvItemCheckForSetting(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_START_ASADMIN, L"EnableStartAsAdmin");
    SetLvItemCheckForSetting(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_LEGACY_TABS, L"EnableLegacyPropertiesDialog");
    SetLvItemCheckForSetting(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_THEME_BORDER, L"EnableTreeListBorder");
    SetLvItemCheckForSetting(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_LASTTAB_SUPPORT, L"MainWindowPageRestoreEnabled");
}

VOID PvGeneralPageSave(
    _In_ PPVP_PE_OPTIONS_CONTEXT Context
    )
{
    //PhSetStringSetting2(L"SearchEngine", &PhaGetDlgItemText(WindowHandle, IDC_SEARCHENGINE)->sr);
    
    if (ComboBox_GetCurSel(GetDlgItem(Context->WindowHandle, IDC_MAXSIZEUNIT)) != PhGetIntegerSetting(L"MaxSizeUnit"))
    {
        PhSetIntegerSetting(L"MaxSizeUnit", ComboBox_GetCurSel(GetDlgItem(Context->WindowHandle, IDC_MAXSIZEUNIT)));
        RestartRequired = TRUE;
    }

    if (!PhEqualString(PhaGetDlgItemText(Context->WindowHandle, IDC_DBGHELPSEARCHPATH), PhaGetStringSetting(L"DbgHelpSearchPath"), TRUE))
    {
        PhSetStringSetting2(L"DbgHelpSearchPath", &(PhaGetDlgItemText(Context->WindowHandle, IDC_DBGHELPSEARCHPATH)->sr));
        RestartRequired = TRUE;
    }

    //SetSettingForLvItemCheck(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_WARNINGS, L"EnableWarnings");
    //SetSettingForLvItemCheckRestartRequired(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_PLUGINS, L"EnablePlugins");
    //SetSettingForLvItemCheck(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_UNDECORATE_SYMBOLS, L"DbgHelpUndecorate");
    SetSettingForLvItemCheckRestartRequired(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_THEME_SUPPORT, L"EnableThemeSupport");
    //SetSettingForLvItemCheck(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_START_ASADMIN, L"EnableStartAsAdmin");
    SetSettingForLvItemCheckRestartRequired(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_LASTTAB_SUPPORT, L"MainWindowPageRestoreEnabled");
    SetSettingForLvItemCheckRestartRequired(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_LEGACY_TABS, L"EnableLegacyPropertiesDialog");
    SetSettingForLvItemCheckRestartRequired(Context->ListViewHandle, PHP_OPTIONS_INDEX_ENABLE_THEME_BORDER, L"EnableTreeListBorder");

    PvUpdateCachedSettings();
    PvSaveSettings();

    if (RestartRequired)
    {
        if (PhShowMessage2(
            Context->WindowHandle,
            TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
            TD_INFORMATION_ICON,
            L"One or more options you have changed requires a restart of PE Viewer.",
            L"Do you want to restart PE Viewer now?"
            ) == IDYES)
        {
            if (PvShellExecuteRestart(Context->WindowHandle))
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
    PPVP_PE_OPTIONS_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocateZero(sizeof(PVP_PE_OPTIONS_CONTEXT));
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
            HIMAGELIST listViewImageList;
            HICON smallIcon;
            HICON largeIcon;

            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_SETTINGS);
            context->ComboHandle = GetDlgItem(hwndDlg, IDC_MAXSIZEUNIT);
            listViewImageList = PhImageListCreate(1, PV_SCALE_DPI(22), ILC_MASK | ILC_COLOR, 1, 1);

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhGetStockApplicationIcon(&smallIcon, &largeIcon);
            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)smallIcon);
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)largeIcon);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_DBGHELPSEARCHPATH), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDRETRY), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDYES), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDCANCEL), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            ListView_SetExtendedListViewStyleEx(context->ListViewHandle, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
            ListView_SetImageList(context->ListViewHandle, listViewImageList, LVSIL_SMALL);
            PhSetControlTheme(context->ListViewHandle, L"explorer");

            for (ULONG i = 0; i < RTL_NUMBER_OF(PhSizeUnitNames); i++)
                ComboBox_AddString(context->ComboHandle, PhSizeUnitNames[i]);

            if (PhMaxSizeUnit != ULONG_MAX)
                ComboBox_SetCurSel(context->ComboHandle, PhMaxSizeUnit);
            else
                ComboBox_SetCurSel(context->ComboHandle, RTL_NUMBER_OF(PhSizeUnitNames) - 1);

            PvLoadGeneralPage(context);

            PhInitializeWindowTheme(hwndDlg, PeEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            NOTHING;
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
                    PvGeneralPageSave(context);

                    EndDialog(hwndDlg, IDOK);
                }
                break;
            case IDRETRY:
                {
                    if (PhShowMessage2(
                        hwndDlg,
                        TDCBF_YES_BUTTON | TDCBF_NO_BUTTON,
                        TD_WARNING_ICON,
                        L"Do you want to reset all settings and restart PE Viewer?",
                        L""
                        ) == IDYES)
                    {
                        PhResetSettings();

                        PvSaveSettings();

                        if (PvShellExecuteRestart(hwndDlg))
                        {
                            RtlExitUserProcess(STATUS_SUCCESS);
                        }
                    }
                }
                break;
            case IDYES:
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
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
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
            }
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, RGB(0, 0, 0));
            SetDCBrushColor((HDC)wParam, RGB(255, 255, 255));
            return (INT_PTR)GetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}

VOID PvShowOptionsWindow(
    _In_ HWND ParentWindow
    )
{
    DialogBox(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_OPTIONS),
        ParentWindow,
        PvOptionsWndProc
        );
}
