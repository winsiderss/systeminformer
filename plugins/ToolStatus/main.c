/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2011-2023
 *
 */

#include "toolstatus.h"

TOOLSTATUS_CONFIG ToolStatusConfig = { 0 };
HWND ProcessTreeNewHandle = NULL;
HWND ServiceTreeNewHandle = NULL;
HWND NetworkTreeNewHandle = NULL;
INT SelectedTabIndex = 0;
ULONG MaxInitializationDelay = 3;
BOOLEAN UpdateAutomatically = TRUE;
BOOLEAN UpdateGraphs = TRUE;
BOOLEAN EnableThemeSupport = FALSE;
BOOLEAN IsWindowSizeMove = FALSE;
BOOLEAN IsWindowMinimized = FALSE;
BOOLEAN IsWindowMaximized = FALSE;
BOOLEAN IconSingleClick = FALSE;
BOOLEAN EnableAvxSupport = FALSE;
BOOLEAN EnableGraphMaxScale = FALSE;
BOOLEAN RestoreRowAfterSearch = FALSE;
TOOLBAR_DISPLAY_STYLE DisplayStyle = TOOLBAR_DISPLAY_STYLE_SELECTIVETEXT;
SEARCHBOX_DISPLAY_MODE SearchBoxDisplayMode = SEARCHBOX_DISPLAY_MODE_ALWAYSSHOW;
REBAR_DISPLAY_LOCATION RebarDisplayLocation = REBAR_DISPLAY_LOCATION_TOP;
HWND RebarHandle = NULL;
HWND ToolBarHandle = NULL;
HWND SearchboxHandle = NULL;
WNDPROC MainWindowHookProc = NULL;
HWND MainWindowHandle = NULL;
HMENU MainMenu = NULL;
HACCEL AcceleratorTable = NULL;
ULONG_PTR SearchMatchHandle = 0;
ULONG RestoreSearchSelectedProcessId = ULONG_MAX;
PH_PLUGIN_SYSTEM_STATISTICS SystemStatistics = { 0 };
PH_CALLBACK_DECLARE(SearchChangedEvent);
PPH_HASHTABLE TabInfoHashtable;
PPH_TN_FILTER_ENTRY ProcessTreeFilterEntry = NULL;
PPH_TN_FILTER_ENTRY ServiceTreeFilterEntry = NULL;
PPH_TN_FILTER_ENTRY NetworkTreeFilterEntry = NULL;
PPH_PLUGIN PluginInstance = NULL;
TOOLSTATUS_INTERFACE PluginInterface =
{
    TOOLSTATUS_INTERFACE_VERSION,
    GetSearchMatchHandle,
    WordMatchStringRef,
    RegisterTabSearch,
    &SearchChangedEvent,
    RegisterTabInfo,
    ToolbarRegisterGraph
};

static ULONG TargetingMode = 0;
static BOOLEAN TargetingWindow = FALSE;
static BOOLEAN TargetingCurrentWindowDraw = FALSE;
static BOOLEAN TargetingCompleted = FALSE;
static HWND TargetingCurrentWindow = NULL;
static PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
static PH_CALLBACK_REGISTRATION MainMenuInitializingCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
static PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;
static PH_CALLBACK_REGISTRATION SettingsUpdatedCallbackRegistration;
static PH_CALLBACK_REGISTRATION LayoutPaddingCallbackRegistration;
static PH_CALLBACK_REGISTRATION TabPageCallbackRegistration;
static PH_CALLBACK_REGISTRATION ProcessTreeNewInitializingCallbackRegistration;
static PH_CALLBACK_REGISTRATION ServiceTreeNewInitializingCallbackRegistration;
static PH_CALLBACK_REGISTRATION NetworkTreeNewInitializingCallbackRegistration;

ULONG_PTR GetSearchMatchHandle(
    VOID
    )
{
    return SearchMatchHandle;
}

VOID NTAPI ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (PtrToUlong(Parameter) < MaxInitializationDelay)
        return;
    if (TaskbarMainWndExiting)
        return;

    PhPluginGetSystemStatistics(&SystemStatistics);

    if (ToolStatusConfig.ToolBarEnabled && ToolBarHandle && UpdateGraphs)
        ToolbarUpdateGraphs();

    if (ToolStatusConfig.StatusBarEnabled)
        StatusBarUpdate(FALSE);

    TaskbarUpdateEvents();
}

VOID NTAPI TreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (Context && Parameter)
    {
        *(HWND*)Context = ((PPH_PLUGIN_TREENEW_INFORMATION)Parameter)->TreeNewHandle;
    }
}

VOID RegisterTabSearch(
    _In_ INT TabIndex,
    _In_ PWSTR BannerText
    )
{
    PTOOLSTATUS_TAB_INFO tabInfo;

    tabInfo = RegisterTabInfo(TabIndex);
    tabInfo->BannerText = BannerText;
}

PTOOLSTATUS_TAB_INFO RegisterTabInfo(
    _In_ INT TabIndex
    )
{
    PTOOLSTATUS_TAB_INFO tabInfoCopy;
    PVOID *entry;

    tabInfoCopy = PhCreateAlloc(sizeof(TOOLSTATUS_TAB_INFO));
    memset(tabInfoCopy, 0, sizeof(TOOLSTATUS_TAB_INFO));

    if (!PhAddItemSimpleHashtable(TabInfoHashtable, IntToPtr(TabIndex), tabInfoCopy))
    {
        PhClearReference(&tabInfoCopy);

        if (entry = PhFindItemSimpleHashtable(TabInfoHashtable, IntToPtr(TabIndex)))
            tabInfoCopy = *entry;
    }

    return tabInfoCopy;
}

PTOOLSTATUS_TAB_INFO FindTabInfo(
    _In_ INT TabIndex
    )
{
    PVOID *entry;

    if (entry = PhFindItemSimpleHashtable(TabInfoHashtable, IntToPtr(TabIndex)))
        return *entry;

    return NULL;
}

HWND GetCurrentTreeNewHandle(
    VOID
    )
{
    HWND treeNewHandle = NULL;

    switch (SelectedTabIndex)
    {
    case 0:
        treeNewHandle = ProcessTreeNewHandle;
        break;
    case 1:
        treeNewHandle = ServiceTreeNewHandle;
        break;
    case 2:
        treeNewHandle = NetworkTreeNewHandle;
        break;
    default:
        {
            PTOOLSTATUS_TAB_INFO tabInfo;

            if ((tabInfo = FindTabInfo(SelectedTabIndex)) && tabInfo->GetTreeNewHandle)
            {
                treeNewHandle = tabInfo->GetTreeNewHandle();
            }
        }
        break;
    }

    return treeNewHandle;
}

VOID ShowCustomizeMenu(
    _In_ HWND WindowHandle
    )
{
    POINT cursorPos;
    PPH_EMENU menu;
    PPH_EMENU_ITEM mainMenuItem;
    PPH_EMENU_ITEM searchMenuItem;
    PPH_EMENU_ITEM lockMenuItem;
    PPH_EMENU_ITEM selectedItem;

    GetCursorPos(&cursorPos);

    menu = PhCreateEMenu();
    PhInsertEMenuItem(menu, mainMenuItem = PhCreateEMenuItem(0, COMMAND_ID_ENABLE_MENU, L"Main menu (auto-hide)", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, searchMenuItem = PhCreateEMenuItem(0, COMMAND_ID_ENABLE_SEARCHBOX, L"Search box", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    ToolbarGraphCreateMenu(menu, COMMAND_ID_GRAPHS_CUSTOMIZE);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, lockMenuItem = PhCreateEMenuItem(0, COMMAND_ID_TOOLBAR_LOCKUNLOCK, L"Lock the toolbar", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, COMMAND_ID_TOOLBAR_CUSTOMIZE, L"Customize...", NULL, NULL), ULONG_MAX);

    if (ToolStatusConfig.AutoHideMenu)
        mainMenuItem->Flags |= PH_EMENU_CHECKED;
    if (ToolStatusConfig.SearchBoxEnabled)
        searchMenuItem->Flags |= PH_EMENU_CHECKED;
    if (ToolStatusConfig.ToolBarLocked)
        lockMenuItem->Flags |= PH_EMENU_CHECKED;

    selectedItem = PhShowEMenu(
        menu,
        WindowHandle,
        PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_TOP,
        cursorPos.x,
        cursorPos.y
        );

    if (selectedItem && selectedItem->Id != ULONG_MAX)
    {
        switch (selectedItem->Id)
        {
        case COMMAND_ID_ENABLE_MENU:
            {
                ToolStatusConfig.AutoHideMenu = !ToolStatusConfig.AutoHideMenu;

                PhSetIntegerSetting(SETTING_NAME_TOOLSTATUS_CONFIG, ToolStatusConfig.Flags);

                if (ToolStatusConfig.AutoHideMenu)
                {
                    SetMenu(WindowHandle, NULL);
                }
                else
                {
                    SetMenu(WindowHandle, MainMenu);
                    DrawMenuBar(WindowHandle);
                }
            }
            break;
        case COMMAND_ID_ENABLE_SEARCHBOX:
            {
                ToolStatusConfig.SearchBoxEnabled = !ToolStatusConfig.SearchBoxEnabled;

                PhSetIntegerSetting(SETTING_NAME_TOOLSTATUS_CONFIG, ToolStatusConfig.Flags);

                ToolbarLoadSettings(FALSE);
                ReBarSaveLayoutSettings();

                if (ToolStatusConfig.SearchBoxEnabled) // && !ToolStatusConfig.SearchAutoFocus)
                {
                    // The window focus was reset by Windows while creating child
                    // windows in ToolbarLoadSettings, so make sure we reset the focus
                    // otherwise you can't navigate with the keyboard. (dmex)
                    SetFocus(WindowHandle);
                }
            }
            break;
        case COMMAND_ID_TOOLBAR_LOCKUNLOCK:
            {
                UINT bandCount;
                UINT bandIndex;

                bandCount = (UINT)SendMessage(RebarHandle, RB_GETBANDCOUNT, 0, 0);

                for (bandIndex = 0; bandIndex < bandCount; bandIndex++)
                {
                    REBARBANDINFO rebarBandInfo =
                    {
                        sizeof(REBARBANDINFO),
                        RBBIM_STYLE
                    };

                    SendMessage(RebarHandle, RB_GETBANDINFO, bandIndex, (LPARAM)&rebarBandInfo);

                    if (!(rebarBandInfo.fStyle & RBBS_GRIPPERALWAYS))
                    {
                        // Removing the RBBS_NOGRIPPER style doesn't remove the gripper padding,
                        // So we toggle the RBBS_GRIPPERALWAYS style to make the Toolbar remove the padding.

                        SetFlag(rebarBandInfo.fStyle, RBBS_GRIPPERALWAYS);

                        SendMessage(RebarHandle, RB_SETBANDINFO, bandIndex, (LPARAM)&rebarBandInfo);

                        ClearFlag(rebarBandInfo.fStyle, RBBS_GRIPPERALWAYS);
                    }

                    if (rebarBandInfo.fStyle & RBBS_NOGRIPPER)
                    {
                        ClearFlag(rebarBandInfo.fStyle, RBBS_NOGRIPPER);
                    }
                    else
                    {
                        SetFlag(rebarBandInfo.fStyle, RBBS_NOGRIPPER);
                    }

                    SendMessage(RebarHandle, RB_SETBANDINFO, bandIndex, (LPARAM)&rebarBandInfo);
                }

                ToolStatusConfig.ToolBarLocked = !ToolStatusConfig.ToolBarLocked;

                PhSetIntegerSetting(SETTING_NAME_TOOLSTATUS_CONFIG, ToolStatusConfig.Flags);

                ToolbarLoadSettings(FALSE);
            }
            break;
        case COMMAND_ID_TOOLBAR_CUSTOMIZE:
            {
                ToolBarShowCustomizeDialog(WindowHandle);
            }
            break;
        case COMMAND_ID_GRAPHS_CUSTOMIZE:
            {
                PPH_TOOLBAR_GRAPH icon;

                if (!selectedItem->Context)
                    break;

                icon = selectedItem->Context;
                ToolbarSetVisibleGraph(icon, !(icon->Flags & PH_NF_ICON_ENABLED));

                ToolbarGraphSaveSettings();
                ReBarSaveLayoutSettings();
            }
            break;
        }
    }

    PhDestroyEMenu(menu);
}

VOID NTAPI TabPageUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    INT tabIndex = PtrToInt(Parameter);

    SelectedTabIndex = tabIndex;

    if (!SearchboxHandle)
        return;

    switch (tabIndex)
    {
    case 0:
        Edit_SetCueBannerText(SearchboxHandle, L"Search Processes (Ctrl+K)");
        break;
    case 1:
        Edit_SetCueBannerText(SearchboxHandle, L"Search Services (Ctrl+K)");
        break;
    case 2:
        Edit_SetCueBannerText(SearchboxHandle, L"Search Network (Ctrl+K)");
        break;
    default:
        {
            PTOOLSTATUS_TAB_INFO tabInfo;

            if ((tabInfo = FindTabInfo(tabIndex)) && tabInfo->BannerText)
            {
                Edit_SetCueBannerText(SearchboxHandle, PhaConcatStrings2(tabInfo->BannerText, L" (Ctrl+K)")->Buffer);
            }
            else
            {
                // Disable the textbox if we're on an unsupported tab.
                Edit_SetCueBannerText(SearchboxHandle, L"Search disabled");
            }
        }
        break;
    }

    //if (ToolStatusConfig.SearchAutoFocus)
    //    SetFocus(SearchboxHandle);
}

VOID NTAPI LayoutPaddingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_LAYOUT_PADDING_DATA layoutPadding = Parameter;

    if (RebarHandle && ToolStatusConfig.ToolBarEnabled)
    {
        RECT rebarRect;

        SendMessage(RebarHandle, WM_SIZE, 0, 0);

        GetClientRect(RebarHandle, &rebarRect);

        // Adjust the PH client area and exclude the rebar width.
        layoutPadding->Padding.top += rebarRect.bottom;

        // TODO: Replace CCS_TOP with CCS_NOPARENTALIGN and use below code
        //switch (RebarDisplayLocation)
        //{
        //case RebarLocationLeft:
        //    {
        //        //x = 0;
        //        //y = 0;
        //        //cx = rebarRect.right - rebarRect.left;
        //        //cy = clientRect.bottom - clientRect.top;
        //    }
        //    break;
        //case RebarLocationTop:
        //    {
        //        //x = 0;
        //        //y = 0;
        //        //cx = clientRect.right - clientRect.left;
        //        //cy = clientRect.bottom - clientRect.top;
        //
        //        // Adjust the PH client area and exclude the rebar height.
        //        //layoutPadding->Padding.top += rebarRect.bottom;
        //    }
        //    break;
        //case RebarLocationRight:
        //    {
        //        //x = clientRect.right - (rebarRect.right - rebarRect.left);
        //        //y = 0;
        //        //cx = rebarRect.right - rebarRect.left;
        //        //cy = clientRect.bottom - clientRect.top;
        //    }
        //    break;
        //case RebarLocationBottom:
        //    {
        //        //x = 0;
        //        //y = clientRect.bottom - (rebarRect.bottom - rebarRect.top) - (StatusBarEnabled ? rebarRect.bottom + 1 : 0);
        //        //cx = clientRect.right - clientRect.left;
        //        //cy = rebarRect.bottom - rebarRect.top;
        //
        //        // Adjust the PH client area and exclude the rebar width.
        //        //layoutPadding->Padding.bottom += rebarRect.bottom;
        //    }
        //    break;
        //}
        //MoveWindow(RebarHandle, x, y, cx, cy, TRUE);

        //if (SearchBoxDisplayStyle == SearchBoxDisplayAutoHide)
        //{
        //    static BOOLEAN isSearchboxVisible = FALSE;
        //    SIZE idealWidth;
        //
        //    // Query the Toolbar ideal width
        //    SendMessage(ToolBarHandle, TB_GETIDEALSIZE, FALSE, (LPARAM)&idealWidth);
        //
        //    // Hide the Searcbox band if the window size is too small...
        //    if (rebarRect.right > idealWidth.cx)
        //    {
        //        if (isSearchboxVisible)
        //        {
        //            if (!RebarBandExists(REBAR_BAND_ID_SEARCHBOX))
        //                RebarBandInsert(REBAR_BAND_ID_SEARCHBOX, SearchboxHandle, 180, 20);
        //
        //            isSearchboxVisible = FALSE;
        //        }
        //    }
        //    else
        //    {
        //        if (!isSearchboxVisible)
        //        {
        //            if (RebarBandExists(REBAR_BAND_ID_SEARCHBOX))
        //                RebarBandRemove(REBAR_BAND_ID_SEARCHBOX);
        //
        //            isSearchboxVisible = TRUE;
        //        }
        //    }
        //}
    }

    if (StatusBarHandle && ToolStatusConfig.StatusBarEnabled)
    {
        RECT statusBarRect;

        SendMessage(StatusBarHandle, WM_SIZE, 0, 0);

        GetClientRect(StatusBarHandle, &statusBarRect);

        // Adjust the PH client area and exclude the StatusBar width.
        layoutPadding->Padding.bottom += statusBarRect.bottom;

        //InvalidateRect(StatusBarHandle, NULL, TRUE);
    }
}

BOOLEAN CheckRebarLastRedrawMessage(
    VOID
    )
{
    static LARGE_INTEGER lastUpdateTimeTicks = { 0 };
    LARGE_INTEGER currentUpdateTimeTicks;
    LARGE_INTEGER currentUpdateTimeFrequency;

    PhQueryPerformanceCounter(&currentUpdateTimeTicks);
    PhQueryPerformanceFrequency(&currentUpdateTimeFrequency);

    if (lastUpdateTimeTicks.QuadPart == 0)
        lastUpdateTimeTicks.QuadPart = currentUpdateTimeTicks.QuadPart;

    if (((currentUpdateTimeTicks.QuadPart - lastUpdateTimeTicks.QuadPart) / currentUpdateTimeFrequency.QuadPart) > 1)
    {
        lastUpdateTimeTicks.QuadPart = currentUpdateTimeTicks.QuadPart;
        return TRUE;
    }

    lastUpdateTimeTicks.QuadPart = currentUpdateTimeTicks.QuadPart;
    return FALSE;
}

VOID InvalidateMainWindowLayout(
    VOID
    )
{
    // Invalidate plugin window layout.
    SystemInformer_InvalidateLayoutPadding();

    // Invalidate the main window layout.
    MainWindowHookProc(MainWindowHandle, WM_SIZE, 0, 0);
}

VOID UpdateDpiMetrics(
    _In_ PVOID InvokeContext
    )
{
    // Update fonts/sizes for new DPI.
    ToolbarWindowFont = SystemInformer_GetFont();

    if (RebarHandle)
    {
        SetWindowFont(RebarHandle, ToolbarWindowFont, TRUE);
    }

    if (ToolBarHandle)
    {
        SetWindowFont(ToolBarHandle, ToolbarWindowFont, TRUE);
    }

    if (StatusBarHandle)
    {
        SetWindowFont(StatusBarHandle, ToolbarWindowFont, TRUE);
    }

    ToolbarLoadSettings(TRUE);

    // Update fonts/sizes for new DPI.
    if (ToolBarHandle)
    {
        LONG toolbarButtonHeight;

        USHORT toolbarButtonSize = HIWORD((ULONG)SendMessage(ToolBarHandle, TB_GETBUTTONSIZE, 0, 0));
        toolbarButtonHeight = ToolStatusGetWindowFontSize(ToolBarHandle, ToolbarWindowFont);
        toolbarButtonHeight = __max(toolbarButtonSize, toolbarButtonHeight);

        if (toolbarButtonHeight < 22)
            toolbarButtonHeight = 22; // 22/default toolbar button height

        SendMessage(ToolBarHandle, TB_SETBUTTONSIZE, 0, MAKELPARAM(0, toolbarButtonHeight));
        RebarAdjustBandHeightLayout(toolbarButtonHeight);
    }

    if (RebarHandle)
    {
        SendMessage(RebarHandle, WM_SIZE, 0, 0);
    }

    if (StatusBarHandle)
    {
        LONG statusbarButtonHeight;

        statusbarButtonHeight = ToolStatusGetWindowFontSize(StatusBarHandle, ToolbarWindowFont);
        statusbarButtonHeight = __max(23, statusbarButtonHeight); // 23/default statusbar height

        SendMessage(StatusBarHandle, SB_SETMINHEIGHT, statusbarButtonHeight, 0);
        SendMessage(StatusBarHandle, WM_SIZE, 0, 0); // redraw
        StatusBarUpdate(TRUE);
    }

    ToolbarGraphsInitializeDpi();

    InvalidateMainWindowLayout();
}

VOID UpdateLayoutMetrics(
    VOID
    )
{
    ToolbarWindowFont = SystemInformer_GetFont();

    if (ToolBarHandle)
    {
        SetWindowFont(ToolBarHandle, ToolbarWindowFont, TRUE);
    }

    if (StatusBarHandle)
    {
        SetWindowFont(StatusBarHandle, ToolbarWindowFont, TRUE);
    }

    if (ToolBarHandle)
    {
        //ULONG toolbarButtonSize = (ULONG)SendMessage(ToolBarHandle, TB_GETBUTTONSIZE, 0, 0);
        LONG toolbarButtonHeight = ToolStatusGetWindowFontSize(ToolBarHandle, ToolbarWindowFont);
        toolbarButtonHeight = __max(22, toolbarButtonHeight); // 22/default toolbar button height

        RebarAdjustBandHeightLayout(toolbarButtonHeight);
        SendMessage(ToolBarHandle, TB_SETBUTTONSIZE, 0, MAKELPARAM(0, toolbarButtonHeight));
    }

    if (StatusBarHandle)
    {
        LONG statusbarButtonHeight = ToolStatusGetWindowFontSize(StatusBarHandle, ToolbarWindowFont);
        statusbarButtonHeight = __max(23, statusbarButtonHeight); // 23/default statusbar height

        SendMessage(StatusBarHandle, SB_SETMINHEIGHT, statusbarButtonHeight, 0);
        //SendMessage(StatusBarHandle, WM_SIZE, 0, 0); // redraw
        StatusBarUpdate(TRUE);
    }

    ToolbarLoadSettings(FALSE);
}

BOOLEAN NTAPI MessageLoopFilter(
    _In_ PMSG Message,
    _In_ PVOID Context
    )
{
    if (
        Message->hwnd == MainWindowHandle ||
        Message->hwnd && IsChild(MainWindowHandle, Message->hwnd)
        )
    {
        if (TranslateAccelerator(MainWindowHandle, AcceleratorTable, Message))
            return TRUE;

        if (Message->message == WM_SYSCHAR && ToolStatusConfig.AutoHideMenu && !GetMenu(MainWindowHandle))
        {
            ULONG key = (ULONG)Message->wParam;

            if (key == 'h' || key == 'v' || key == 't' || key == 'u' || key == 'e')
            {
                SetMenu(MainWindowHandle, MainMenu);
                DrawMenuBar(MainWindowHandle);
                SendMessage(MainWindowHandle, WM_SYSCHAR, Message->wParam, Message->lParam);
                return TRUE;
            }
        }
    }

    return FALSE;
}

VOID DrawWindowBorderForTargeting(
    _In_ HWND hWnd
    )
{
    RECT rect;
    HDC hdc;
    LONG dpiValue;

    GetWindowRect(hWnd, &rect);
    hdc = GetWindowDC(hWnd);

    if (hdc)
    {
        INT penWidth;
        INT oldDc;
        HPEN pen;
        HBRUSH brush;

        dpiValue = PhGetWindowDpi(hWnd);
        penWidth = PhGetSystemMetrics(SM_CXBORDER, dpiValue) * 3;

        oldDc = SaveDC(hdc);

        // Get an inversion effect.
        SetROP2(hdc, R2_NOT);

        pen = CreatePen(PS_INSIDEFRAME, penWidth, RGB(0x00, 0x00, 0x00));
        SelectPen(hdc, pen);

        brush = PhGetStockBrush(NULL_BRUSH);
        SelectBrush(hdc, brush);

        // Draw the rectangle.
        Rectangle(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top);

        // Cleanup.
        DeletePen(pen);

        RestoreDC(hdc, oldDc);
        ReleaseDC(hWnd, hdc);
    }
}

VOID NTAPI SearchControlCallback(
    _In_ ULONG_PTR MatchHandle,
    _In_opt_ PVOID Context
    )
{
    SearchMatchHandle = MatchHandle;

    // Expand the nodes to ensure that they will be visible to the user.
    PhExpandAllProcessNodes(TRUE);
    PhDeselectAllProcessNodes();
    PhDeselectAllServiceNodes();

    PhApplyTreeNewFilters(PhGetFilterSupportProcessTreeList());
    PhApplyTreeNewFilters(PhGetFilterSupportServiceTreeList());
    PhApplyTreeNewFilters(PhGetFilterSupportNetworkTreeList());

    PhInvokeCallback(&SearchChangedEvent, (PVOID)SearchMatchHandle);
}

VOID SetSearchFocus(
    _In_ HWND hWnd,
    _In_ BOOLEAN Focus
    )
{
    if (SearchboxHandle && ToolStatusConfig.SearchBoxEnabled)
    {
        if (Focus)
        {
            if (SearchBoxDisplayMode == SEARCHBOX_DISPLAY_MODE_HIDEINACTIVE)
            {
                LONG dpiValue;

                dpiValue = SystemInformer_GetWindowDpi();

                if (!RebarBandExists(REBAR_BAND_ID_SEARCHBOX))
                    RebarBandInsert(REBAR_BAND_ID_SEARCHBOX, SearchboxHandle, PhGetDpi(180, dpiValue), 22);

                if (!IsWindowVisible(SearchboxHandle))
                    ShowWindow(SearchboxHandle, SW_SHOW);
            }

            SetFocus(SearchboxHandle);
            Edit_SetSel(SearchboxHandle, 0, -1);
        }
        else
        {
            HWND tnHandle;

            // Return focus to the treelist.
            if (tnHandle = GetCurrentTreeNewHandle())
            {
                ULONG tnCount = TreeNew_GetFlatNodeCount(tnHandle);

                for (ULONG i = 0; i < tnCount; i++)
                {
                    PPH_TREENEW_NODE node = TreeNew_GetFlatNode(tnHandle, i);

                    // Select the first visible node.
                    if (node->Visible)
                    {
                        TreeNew_FocusMarkSelectNode(tnHandle, node);
                        break;
                    }
                }
            }
        }
    }
}

VOID ToggleSearchFocus(
    _In_ HWND hWnd
    )
{
    // Check if the searchbox is already focused.
    SetSearchFocus(hWnd, GetFocus() != SearchboxHandle);
}

LRESULT CALLBACK MainWindowCallbackProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (WindowMessage)
    {
    case WM_NCCREATE:
        {
            MainWindowHandle = WindowHandle;
        }
        break;
    case WM_DESTROY:
        {
            TaskbarMainWndExiting = TRUE;

            SystemInformer_SetWindowProcedure(MainWindowHookProc);
            PhSetWindowProcedure(WindowHandle, MainWindowHookProc);
        }
        break;
    case WM_ENDSESSION:
        {
            TaskbarMainWndExiting = TRUE;
        }
        break;
    case WM_DPICHANGED:
        {
            // Let System Informer perform the default processing.
            LRESULT result = MainWindowHookProc(WindowHandle, WindowMessage, wParam, lParam);

            SystemInformer_Invoke(UpdateDpiMetrics, NULL);

            return result;
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case EN_SETFOCUS:
                {
                    if (!SearchboxHandle)
                        break;

                    if (GET_WM_COMMAND_HWND(wParam, lParam) != SearchboxHandle)
                        break;

                    if (RestoreRowAfterSearch)
                    {
                        PPH_PROCESS_ITEM processItem = PhGetSelectedProcessItem();

                        if (processItem)
                            RestoreSearchSelectedProcessId = HandleToUlong(processItem->ProcessId);
                        else
                            RestoreSearchSelectedProcessId = ULONG_MAX;
                    }
                }
                break;
            case EN_KILLFOCUS:
                {
                    if (!SearchboxHandle)
                        break;

                    if (GET_WM_COMMAND_HWND(wParam, lParam) != SearchboxHandle)
                        break;

                    if (RestoreRowAfterSearch && !SearchMatchHandle)
                    {
                        if (RestoreSearchSelectedProcessId != ULONG_MAX)
                        {
                            PPH_PROCESS_NODE node;

                            if (node = PhFindProcessNode(UlongToHandle(RestoreSearchSelectedProcessId)))
                            {
                                PhSelectAndEnsureVisibleProcessNode(node);
                            }

                            RestoreSearchSelectedProcessId = ULONG_MAX;
                        }
                    }

                    if (SearchBoxDisplayMode == SEARCHBOX_DISPLAY_MODE_HIDEINACTIVE && !SearchMatchHandle)
                    {
                        if (RebarBandExists(REBAR_BAND_ID_SEARCHBOX))
                            RebarBandRemove(REBAR_BAND_ID_SEARCHBOX);
                    }
                }
                goto DefaultWndProc;
            }

            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case PHAPP_ID_ESC_EXIT:
                {
                    // If we're targeting and the user presses the Esc key, cancel the targeting.
                    // We also make sure the window doesn't get closed, by filtering out the message.
                    if (TargetingWindow)
                    {
                        TargetingWindow = FALSE;
                        ReleaseCapture();

                        goto DefaultWndProc;
                    }

                    if (SearchboxHandle && (GetFocus() == SearchboxHandle))
                    {
                        SendMessage(SearchboxHandle, WM_KEYDOWN, VK_ESCAPE, 0);
                        SetSearchFocus(WindowHandle, FALSE);

                        goto DefaultWndProc;
                    }
                }
                break;
            case ID_SEARCH:
                // handle keybind Ctrl + K
                ToggleSearchFocus(WindowHandle);
                goto DefaultWndProc;
            case ID_SEARCH_TAB:
                // handle tab when the searchbox is focused
                if (SearchboxHandle && (GetFocus() == SearchboxHandle))
                    SetSearchFocus(WindowHandle, FALSE);
                goto DefaultWndProc;
            case PHAPP_ID_VIEW_ALWAYSONTOP:
                {
                    // Let System Informer perform the default processing.
                    LRESULT result = MainWindowHookProc(WindowHandle, WindowMessage, wParam, lParam);

                    // Query the settings.
                    BOOLEAN isAlwaysOnTopEnabled = !!PhGetIntegerSetting(L"MainWindowAlwaysOnTop");

                    // Set the pressed button state.
                    SendMessage(ToolBarHandle, TB_PRESSBUTTON, (WPARAM)PHAPP_ID_VIEW_ALWAYSONTOP, (LPARAM)(MAKELONG(isAlwaysOnTopEnabled, 0)));

                    return result;
                }
                break;
            case PHAPP_ID_UPDATEINTERVAL_FAST:
            case PHAPP_ID_UPDATEINTERVAL_NORMAL:
            case PHAPP_ID_UPDATEINTERVAL_BELOWNORMAL:
            case PHAPP_ID_UPDATEINTERVAL_SLOW:
            case PHAPP_ID_UPDATEINTERVAL_VERYSLOW:
                {
                    // Let System Informer perform the default processing.
                    LRESULT result = MainWindowHookProc(WindowHandle, WindowMessage, wParam, lParam);

                    StatusBarUpdate(TRUE);

                    return result;
                }
                break;
            case PHAPP_ID_VIEW_UPDATEAUTOMATICALLY:
                {
                    UpdateAutomatically = !UpdateAutomatically;

                    StatusBarUpdate(TRUE);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lParam;

            if (RebarHandle && hdr->hwndFrom == RebarHandle)
            {
                switch (hdr->code)
                {
                case RBN_HEIGHTCHANGE:
                    {
                        // Note: RB_INSERTBAND sends RBN_HEIGHTCHANGE during initialization and we skip the first
                        // InvalidateMainWindowLayout since we invalidate layout during initialization from ToolbarLoadSettings (dmex)
                        if (ToolbarInitialized)
                        {
                            InvalidateMainWindowLayout();
                        }
                    }
                    break;
                case RBN_CHEVRONPUSHED:
                    {
                        LPNMREBARCHEVRON rebar;
                        ULONG index = 0;
                        ULONG buttonCount = 0;
                        RECT toolbarRect;
                        PPH_EMENU menu;
                        PPH_EMENU_ITEM selectedItem;

                        rebar = (LPNMREBARCHEVRON)lParam;
                        menu = PhCreateEMenu();

                        GetClientRect(ToolBarHandle, &toolbarRect);

                        buttonCount = (ULONG)SendMessage(ToolBarHandle, TB_BUTTONCOUNT, 0, 0);

                        for (index = 0; index < buttonCount; index++)
                        {
                            RECT buttonRect;
                            TBBUTTONINFO buttonInfo =
                            {
                                sizeof(TBBUTTONINFO),
                                TBIF_BYINDEX | TBIF_STYLE | TBIF_COMMAND | TBIF_IMAGE
                            };

                            // Get the client coordinates of the button.
                            if (SendMessage(ToolBarHandle, TB_GETITEMRECT, index, (LPARAM)&buttonRect) == 0)
                                break;

                            if (buttonRect.right <= toolbarRect.right)
                                continue;

                            // Get extended button information.
                            if (SendMessage(ToolBarHandle, TB_GETBUTTONINFO, index, (LPARAM)&buttonInfo) == -1)
                                break;

                            if (buttonInfo.fsStyle == BTNS_SEP)
                            {
                                // Add separators to menu.
                                PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                            }
                            else
                            {
                                PPH_EMENU_ITEM menuItem;
                                LONG dpiValue;

                                dpiValue = SystemInformer_GetWindowDpi();

                                // Add toolbar buttons to the context menu.
                                menuItem = PhCreateEMenuItem(0, buttonInfo.idCommand, ToolbarGetText(buttonInfo.idCommand), NULL, NULL);

                                // Add the button image to the context menu.
                                menuItem->Flags |= PH_EMENU_BITMAP_OWNED;
                                menuItem->Bitmap = ToolbarGetImage(buttonInfo.idCommand, dpiValue);

                                switch (buttonInfo.idCommand)
                                {
                                case TIDC_FINDWINDOW:
                                case TIDC_FINDWINDOWTHREAD:
                                case TIDC_FINDWINDOWKILL:
                                    {
                                        // Note: These buttons are incompatible with the context menu window messages.
                                        PhSetDisabledEMenuItem(menuItem);
                                    }
                                    break;
                                case PHAPP_ID_VIEW_ALWAYSONTOP:
                                    {
                                        // Set the pressed state.
                                        if (PhGetIntegerSetting(L"MainWindowAlwaysOnTop"))
                                            menuItem->Flags |= PH_EMENU_CHECKED;
                                    }
                                    break;
                                case PHAPP_ID_HACKER_SHOWDETAILSFORALLPROCESSES:
                                    {
                                        if (PhGetOwnTokenAttributes().Elevated)
                                        {
                                            // Disable the 'Show Details for All Processes' button when we're elevated.
                                            PhSetDisabledEMenuItem(menuItem);
                                        }
                                    }
                                    break;
                                case TIDC_POWERMENUDROPDOWN:
                                    {
                                        // Create the sub-menu...
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_LOCK, L"&Lock", NULL, NULL), ULONG_MAX);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_LOGOFF, L"Log o&ff", NULL, NULL), ULONG_MAX);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuSeparator(), ULONG_MAX);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_SLEEP, L"&Sleep", NULL, NULL), ULONG_MAX);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_HIBERNATE, L"&Hibernate", NULL, NULL), ULONG_MAX);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuSeparator(), ULONG_MAX);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_RESTART_UPDATE, L"Update and restart", NULL, NULL), ULONG_MAX);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_SHUTDOWN_UPDATE, L"Update and shut down", NULL, NULL), ULONG_MAX);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuSeparator(), ULONG_MAX);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_RESTART, L"R&estart", NULL, NULL), ULONG_MAX);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_RESTARTADVOPTIONS, L"Restart to advanced options", NULL, NULL), ULONG_MAX);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_RESTARTBOOTOPTIONS, L"Restart to boot &options", NULL, NULL), ULONG_MAX);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(PhGetOwnTokenAttributes().Elevated ? 0 : PH_EMENU_DISABLED, PHAPP_ID_COMPUTER_RESTARTFWOPTIONS, L"Restart to firmware options", NULL, NULL), ULONG_MAX);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuSeparator(), ULONG_MAX);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_SHUTDOWN, L"Shu&t down", NULL, NULL), ULONG_MAX);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_SHUTDOWNHYBRID, L"H&ybrid shut down", NULL, NULL), ULONG_MAX);

                                        if (PhWindowsVersion < WINDOWS_8)
                                        {
                                            PPH_EMENU_ITEM menuItemRemove;

                                            if (menuItemRemove = PhFindEMenuItem(menuItem, PH_EMENU_FIND_DESCEND, NULL, PHAPP_ID_COMPUTER_RESTARTBOOTOPTIONS))
                                                PhDestroyEMenuItem(menuItemRemove);
                                            if (menuItemRemove = PhFindEMenuItem(menuItem, PH_EMENU_FIND_DESCEND, NULL, PHAPP_ID_COMPUTER_SHUTDOWNHYBRID))
                                                PhDestroyEMenuItem(menuItemRemove);
                                        }
                                    }
                                    break;
                                }

                                PhInsertEMenuItem(menu, menuItem, ULONG_MAX);
                            }
                        }

                        MapWindowRect(RebarHandle, NULL, &rebar->rc);

                        selectedItem = PhShowEMenu(
                            menu,
                            WindowHandle,
                            PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP,
                            rebar->rc.left,
                            rebar->rc.bottom
                            );

                        if (selectedItem && selectedItem->Id != ULONG_MAX)
                        {
                            MainWindowHookProc(WindowHandle, WM_COMMAND, MAKEWPARAM(selectedItem->Id, BN_CLICKED), 0);
                        }

                        PhDestroyEMenu(menu);
                    }
                    break;
                case RBN_LAYOUTCHANGED:
                    {
                        ReBarSaveLayoutSettings();
                    }
                    break;
                case NM_CUSTOMDRAW:
                    {
                        if (EnableThemeSupport)
                            return PhThemeWindowDrawRebar((LPNMCUSTOMDRAW)lParam);
                    }
                    break;
                }

                goto DefaultWndProc;
            }
            else if (ToolBarHandle && hdr->hwndFrom == ToolBarHandle)
            {
                switch (hdr->code)
                {
                case TBN_GETDISPINFO:
                    {
                        LPNMTBDISPINFO toolbarDisplayInfo = (LPNMTBDISPINFO)lParam;

                        if (FlagOn(toolbarDisplayInfo->dwMask, TBNF_IMAGE))
                        {
                            // Try to find the cached bitmap index.
                            // NOTE: The TBNF_DI_SETITEM flag below will cache the index so we only get called once.
                            //       However, when adding buttons from the customize dialog we get called a second time,
                            //       so we cache the index in our ToolbarButtons array to prevent ToolBarImageList from growing.
                            for (UINT i = 0; i < ARRAYSIZE(ToolbarButtons); i++)
                            {
                                if (ToolbarButtons[i].idCommand == toolbarDisplayInfo->idCommand)
                                {
                                    // Cache the bitmap index.
                                    SetFlag(toolbarDisplayInfo->dwMask, TBNF_DI_SETITEM);
                                    // Set the bitmap index.
                                    toolbarDisplayInfo->iImage = ToolbarButtons[i].iBitmap;
                                    break;
                                }
                            }

                            if (toolbarDisplayInfo->iImage == I_IMAGECALLBACK)
                            {
                                LONG dpiValue = SystemInformer_GetWindowDpi();

                                // We didn't find a cached bitmap index...
                                // Load the button bitmap and cache the index.
                                for (UINT i = 0; i < ARRAYSIZE(ToolbarButtons); i++)
                                {
                                    if (ToolbarButtons[i].idCommand == toolbarDisplayInfo->idCommand)
                                    {
                                        HBITMAP buttonImage;

                                        if (buttonImage = ToolbarGetImage(toolbarDisplayInfo->idCommand, dpiValue))
                                        {
                                            // Cache the bitmap index.
                                            SetFlag(toolbarDisplayInfo->dwMask, TBNF_DI_SETITEM);

                                            // Add the image, cache the value in the ToolbarButtons array, set the bitmap index.
                                            toolbarDisplayInfo->iImage = ToolbarButtons[i].iBitmap = PhImageListAddBitmap(
                                                ToolBarImageList,
                                                buttonImage,
                                                NULL
                                                );

                                            DeleteBitmap(buttonImage);
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    break;
                case TBN_DROPDOWN:
                    {
                        LPNMTOOLBAR toolbar = (LPNMTOOLBAR)hdr;
                        PPH_EMENU menu;
                        PPH_EMENU_ITEM selectedItem;

                        if (toolbar->iItem != TIDC_POWERMENUDROPDOWN)
                            break;

                        menu = PhCreateEMenu();

                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_LOCK, L"&Lock", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_LOGOFF, L"Log o&ff", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_SLEEP, L"&Sleep", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_HIBERNATE, L"&Hibernate", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_RESTART_UPDATE, L"Update and restart", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_SHUTDOWN_UPDATE, L"Update and shut down", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_RESTART, L"R&estart", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_RESTARTADVOPTIONS, L"Restart to advanced options", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_RESTARTBOOTOPTIONS, L"Restart to boot &options", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(PhGetOwnTokenAttributes().Elevated ? 0 : PH_EMENU_DISABLED, PHAPP_ID_COMPUTER_RESTARTFWOPTIONS, L"Restart to firmware options", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_SHUTDOWN, L"Shu&t down", NULL, NULL), ULONG_MAX);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_SHUTDOWNHYBRID, L"H&ybrid shut down", NULL, NULL), ULONG_MAX);

                        if (PhWindowsVersion < WINDOWS_8)
                        {
                            PPH_EMENU_ITEM menuItemRemove;

                            if (menuItemRemove = PhFindEMenuItem(menu, PH_EMENU_FIND_DESCEND, NULL, PHAPP_ID_COMPUTER_RESTARTBOOTOPTIONS))
                                PhDestroyEMenuItem(menuItemRemove);
                            if (menuItemRemove = PhFindEMenuItem(menu, PH_EMENU_FIND_DESCEND, NULL, PHAPP_ID_COMPUTER_SHUTDOWNHYBRID))
                                PhDestroyEMenuItem(menuItemRemove);
                        }

                        MapWindowRect(ToolBarHandle, NULL, &toolbar->rcButton);

                        selectedItem = PhShowEMenu(
                            menu,
                            WindowHandle,
                            PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP,
                            toolbar->rcButton.left,
                            toolbar->rcButton.bottom
                            );

                        if (selectedItem && selectedItem->Id != ULONG_MAX)
                        {
                            MainWindowHookProc(WindowHandle, WM_COMMAND, MAKEWPARAM(selectedItem->Id, BN_CLICKED), 0);
                        }

                        PhDestroyEMenu(menu);
                    }
                    return TBDDRET_DEFAULT;
                case NM_LDOWN:
                    {
                        LPNMCLICK toolbar = (LPNMCLICK)hdr;
                        ULONG id = (ULONG)toolbar->dwItemSpec;

                        if (id == ULONG_MAX)
                            break;

                        if (id == TIDC_FINDWINDOW || id == TIDC_FINDWINDOWTHREAD || id == TIDC_FINDWINDOWKILL)
                        {
                            // Direct all mouse events to this window.
                            SetCapture(WindowHandle);

                            // Set the cursor.
                            PhSetCursor(PhLoadCursor(NULL, IDC_CROSS));

                            // Send the window to the bottom.
                            SetWindowPos(WindowHandle, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

                            TargetingWindow = TRUE;
                            TargetingCurrentWindow = NULL;
                            TargetingCurrentWindowDraw = FALSE;
                            TargetingCompleted = FALSE;
                            TargetingMode = id;

                            SendMessage(WindowHandle, WM_MOUSEMOVE, 0, 0);
                        }
                    }
                    break;
                case NM_RCLICK:
                    {
                        ShowCustomizeMenu(WindowHandle);
                    }
                    break;
                case NM_CUSTOMDRAW:
                    {
                        if (EnableThemeSupport)
                            return PhThemeWindowDrawToolbar((LPNMTBCUSTOMDRAW)lParam);
                    }
                    break;
                }

                goto DefaultWndProc;
            }
            else if (StatusBarHandle && hdr->hwndFrom == StatusBarHandle)
            {
                switch (hdr->code)
                {
                case NM_RCLICK:
                    {
                        StatusBarShowMenu(WindowHandle);
                    }
                    break;
                }

                goto DefaultWndProc;
            }
            else
            {
                if (
                    ToolStatusConfig.ToolBarEnabled &&
                    ToolBarHandle &&
                    ToolbarUpdateGraphsInfo(WindowHandle, hdr)
                    )
                {
                    goto DefaultWndProc;
                }
            }
        }
        break;
    case WM_MOUSEMOVE:
        {
            if (TargetingWindow)
            {
                POINT cursorPos;
                HWND windowOverMouse;
                CLIENT_ID clientId;

                GetCursorPos(&cursorPos);
                windowOverMouse = WindowFromPoint(cursorPos);

                if (TargetingCurrentWindow != windowOverMouse)
                {
                    if (TargetingCurrentWindow && TargetingCurrentWindowDraw)
                    {
                        // Invert the old border (to remove it).
                        DrawWindowBorderForTargeting(TargetingCurrentWindow);
                    }

                    if (windowOverMouse)
                    {
                        PhGetWindowClientId(windowOverMouse, &clientId);

                        // Draw a rectangle over the current window (but not if it's one of our own).
                        if (clientId.UniqueProcess != NtCurrentProcessId())
                        {
                            DrawWindowBorderForTargeting(windowOverMouse);
                            TargetingCurrentWindowDraw = TRUE;
                        }
                        else
                        {
                            TargetingCurrentWindowDraw = FALSE;
                        }
                    }

                    TargetingCurrentWindow = windowOverMouse;
                }

                goto DefaultWndProc;
            }
        }
        break;
    case WM_LBUTTONUP:
        {
            if (TargetingWindow)
            {
                CLIENT_ID clientId;

                TargetingCompleted = TRUE;

                // Reset the original cursor.
                PhSetCursor(PhLoadCursor(NULL, IDC_ARROW));

                // Bring the window back to the top, and preserve the Always on Top setting.
                SetWindowPos(WindowHandle, PhGetIntegerSetting(L"MainWindowAlwaysOnTop") ? HWND_TOPMOST : HWND_TOP,
                    0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

                TargetingWindow = FALSE;
                ReleaseCapture();

                if (TargetingCurrentWindow)
                {
                    if (TargetingCurrentWindowDraw)
                    {
                        // Remove the border on the window we found.
                        DrawWindowBorderForTargeting(TargetingCurrentWindow);
                    }

                    if (ToolStatusConfig.ResolveGhostWindows)
                    {
                        HWND hungWindow = PhHungWindowFromGhostWindow(TargetingCurrentWindow);

                        if (hungWindow)
                            TargetingCurrentWindow = hungWindow;
                    }

                    PhGetWindowClientId(TargetingCurrentWindow, &clientId);

                    if (clientId.UniqueThread && clientId.UniqueProcess && clientId.UniqueProcess != NtCurrentProcessId())
                    {
                        PPH_PROCESS_NODE processNode;

                        processNode = PhFindProcessNode(clientId.UniqueProcess);

                        if (processNode)
                        {
                            SystemInformer_SelectTabPage(0);
                            SystemInformer_SelectProcessNode(processNode);
                        }

                        switch (TargetingMode)
                        {
                        case TIDC_FINDWINDOWTHREAD:
                            {
                                PPH_PROCESS_PROPCONTEXT propContext;
                                PPH_PROCESS_ITEM processItem;

                                if (processItem = PhReferenceProcessItem(clientId.UniqueProcess))
                                {
                                    if (propContext = PhCreateProcessPropContext(WindowHandle, processItem))
                                    {
                                        PhSetSelectThreadIdProcessPropContext(propContext, clientId.UniqueThread);
                                        PhShowProcessProperties(propContext);
                                        PhDereferenceObject(propContext);
                                    }

                                    PhDereferenceObject(processItem);
                                }
                                else
                                {
                                    PhShowError2(WindowHandle, SystemInformer_GetWindowName(), L"The process (PID %lu) does not exist.", HandleToUlong(clientId.UniqueProcess));
                                }
                            }
                            break;
                        case TIDC_FINDWINDOWKILL:
                            {
                                PPH_PROCESS_ITEM processItem;

                                if (processItem = PhReferenceProcessItem(clientId.UniqueProcess))
                                {
                                    PhUiTerminateProcesses(WindowHandle, &processItem, 1);
                                    PhDereferenceObject(processItem);
                                }
                                else
                                {
                                    PhShowError2(WindowHandle, SystemInformer_GetWindowName(), L"The process (PID %lu) does not exist.", HandleToUlong(clientId.UniqueProcess));
                                }
                            }
                            break;
                        }
                    }
                }

                goto DefaultWndProc;
            }
        }
        break;
    case WM_CAPTURECHANGED:
        {
            if (!TargetingCompleted)
            {
                // The user cancelled the targeting, probably by pressing the Esc key.

                TargetingCompleted = TRUE;

                // Remove the border on the currently selected window.
                if (TargetingCurrentWindow)
                {
                    if (TargetingCurrentWindowDraw)
                    {
                        // Remove the border on the window we found.
                        DrawWindowBorderForTargeting(TargetingCurrentWindow);
                    }
                }

                SetWindowPos(WindowHandle, PhGetIntegerSetting(L"MainWindowAlwaysOnTop") ? HWND_TOPMOST : HWND_TOP,
                    0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

                TargetingWindow = FALSE;
            }
        }
        break;
    case WM_SIZE:
        {
            // Invalidate plugin window layouts.
            SystemInformer_InvalidateLayoutPadding();
        }
        break;
    case WM_SETTINGCHANGE:
        {
            if (SearchboxHandle)
            {
                // Forward to the Searchbox so we can reinitialize the settings. (dmex)
                SendMessage(SearchboxHandle, WM_SETTINGCHANGE, 0, 0);
            }
        }
        break;
    case WM_SHOWWINDOW:
        {
            UpdateGraphs = !!(BOOL)wParam;
        }
        break;
    case WM_ENTERSIZEMOVE:
        {
            IsWindowSizeMove = TRUE;
        }
        break;
    case WM_EXITSIZEMOVE:
        {
            IsWindowSizeMove = FALSE;
        }
        break;
    case WM_WINDOWPOSCHANGED:
        {
            if (IsWindowSizeMove)
                break;

            // Note: The toolbar graphs sometimes stop updating after restoring
            // the window because WindowsNT doesn't always send a SC_RESTORE message...
            // The below code is an attempt at working around this issue. (dmex)

            BOOLEAN minimized = !!IsMinimized(WindowHandle);
            BOOLEAN maximized = !!IsMaximized(WindowHandle);

            if (IsWindowMinimized != minimized)
            {
                IsWindowMinimized = minimized;
                // Make sure graph drawing is enabled when not minimized. (dmex)
                UpdateGraphs = !minimized;

                if (UpdateGraphs && RebarHandle && ToolbarGraphsEnabled())// && CheckRebarLastRedrawMessage())
                {
                    // See notes in SC_RESTORE (dmex)
                    ToolbarUpdateGraphVisualStates();
                    SetWindowPos(RebarHandle, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING);
                }
            }

            if (IsWindowMaximized != maximized)
            {
                IsWindowMaximized = minimized;

                if (UpdateGraphs && RebarHandle && ToolbarGraphsEnabled())// && CheckRebarLastRedrawMessage())
                {
                    // See notes in SC_RESTORE (dmex)
                    ToolbarUpdateGraphVisualStates();
                    SetWindowPos(RebarHandle, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING);
                }
            }
        }
        break;
    case WM_SYSCOMMAND:
        {
            UINT command = (wParam & 0xFFF0);

            switch (command)
            {
            case SC_KEYMENU:
                {
                    if (lParam != 0)
                        break;

                    if (!ToolStatusConfig.AutoHideMenu)
                        break;

                    if (GetMenu(WindowHandle))
                    {
                        SetMenu(WindowHandle, NULL);
                    }
                    else
                    {
                        SetMenu(WindowHandle, MainMenu);
                        DrawMenuBar(WindowHandle);
                    }
                }
                break;
            case SC_MINIMIZE:
                {
                    UpdateGraphs = FALSE;
                }
                break;
            case SC_RESTORE:
                {
                    UpdateGraphs = TRUE;

                    //if (RebarHandle && CheckRebarLastRedrawMessage())
                    //{
                    //    ToolbarUpdateGraphVisualStates();
                    //
                    //    // NOTE: Maximizing and restoring the window when updating is paused will cause the graphs to leave
                    //    // artifacts on the window because the rebar control doesn't redraw... so we'll force the rebar to redraw.
                    //    // TODO: Figure out the exact SetWindowPos flags required. (dmex)
                    //    SetWindowPos(RebarHandle, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING);
                    //    //RedrawWindow(hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
                    //}
                }
                break;
            }
        }
        break;
    case WM_EXITMENULOOP:
        {
            if (!ToolStatusConfig.AutoHideMenu)
                break;

            if (GetMenu(WindowHandle))
            {
                SetMenu(WindowHandle, NULL);
            }
        }
        break;
    case WM_PH_NOTIFY_ICON_MESSAGE:
        {
            // Don't do anything when search autofocus disabled.
            if (!ToolStatusConfig.SearchAutoFocus)
                break;

            // Let System Informer perform the default processing.
            LRESULT result = MainWindowHookProc(WindowHandle, WindowMessage, wParam, lParam);

            // This fixes the search focus for the 'Hide when closed' option. See GH #663 (dmex)
            switch (LOWORD(lParam))
            {
            case WM_LBUTTONDOWN:
                {
                    if (SearchboxHandle && IconSingleClick)
                    {
                        if (IsWindowVisible(WindowHandle))
                        {
                            SetFocus(SearchboxHandle);
                        }
                    }
                }
                break;
            case WM_LBUTTONDBLCLK:
                {
                    if (SearchboxHandle && !IconSingleClick)
                    {
                        if (IsWindowVisible(WindowHandle))
                        {
                            SetFocus(SearchboxHandle);
                        }
                    }
                }
                break;
            }

            return result;
        }
        break;
    case WM_PH_ACTIVATE:
        {
            // Don't do anything when search autofocus disabled. (dmex)
            if (!ToolStatusConfig.SearchAutoFocus)
                break;

            // Let System Informer perform the default processing. (dmex)
            LRESULT result = MainWindowHookProc(WindowHandle, WindowMessage, wParam, lParam);

            // Re-focus the searchbox when we're already running and we're moved
            // into the foreground by the new instance. Fixes GH #178 (dmex)
            if (SearchboxHandle && result == PH_ACTIVATE_REPLY)
            {
                if (IsWindowVisible(WindowHandle))
                {
                    SetFocus(SearchboxHandle);
                }
            }

            return result;
        }
        break;
    }

    return MainWindowHookProc(WindowHandle, WindowMessage, wParam, lParam);

DefaultWndProc:
    return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
}

VOID NTAPI MainWindowShowingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    AcceleratorTable = LoadAccelerators(PluginInstance->DllBase, MAKEINTRESOURCE(IDR_MAINWND_ACCEL));
    PhRegisterMessageLoopFilter(MessageLoopFilter, NULL);

    PhRegisterCallback(
        SystemInformer_GetCallbackLayoutPadding(),
        LayoutPaddingCallback,
        NULL,
        &LayoutPaddingCallbackRegistration
        );

    ToolbarLoadSettings(FALSE);
    ToolbarCreateGraphs();
    ReBarLoadLayoutSettings();
    StatusBarLoadSettings();
    TaskbarInitialize();

    MainMenu = GetMenu(MainWindowHandle);
    if (ToolStatusConfig.AutoHideMenu)
    {
        SetMenu(MainWindowHandle, NULL);
    }

    if (ToolStatusConfig.SearchBoxEnabled && ToolStatusConfig.SearchAutoFocus && SearchboxHandle)
        SetFocus(SearchboxHandle);
}

VOID NTAPI MainMenuInitializingCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    ULONG insertIndex;
    PPH_EMENU_ITEM menu;
    PPH_EMENU_ITEM menuItem;
    PPH_EMENU_ITEM mainMenuItem;
    PPH_EMENU_ITEM searchMenuItem;
    PPH_EMENU_ITEM lockMenuItem;

    if (menuInfo->u.MainMenu.SubMenuIndex != PH_MENU_ITEM_LOCATION_VIEW)
        return;

    if (menuItem = PhFindEMenuItem(menuInfo->Menu, 0, NULL, PHAPP_ID_VIEW_TRAYICONS))
        insertIndex = PhIndexOfEMenuItem(menuInfo->Menu, menuItem) + 1;
    else
        insertIndex = ULONG_MAX;

    menu = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"Toolbar", NULL);
    PhInsertEMenuItem(menu, mainMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, COMMAND_ID_ENABLE_MENU, L"Main menu (auto-hide)", NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, searchMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, COMMAND_ID_ENABLE_SEARCHBOX, L"Search box", NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    ToolbarGraphCreatePluginMenu(menu, COMMAND_ID_GRAPHS_CUSTOMIZE);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, lockMenuItem = PhPluginCreateEMenuItem(PluginInstance, 0, COMMAND_ID_TOOLBAR_LOCKUNLOCK, L"Lock the toolbar", NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhPluginCreateEMenuItem(PluginInstance, 0, COMMAND_ID_TOOLBAR_CUSTOMIZE, L"Customize...", NULL), ULONG_MAX);

    if (ToolStatusConfig.AutoHideMenu)
        mainMenuItem->Flags |= PH_EMENU_CHECKED;
    if (ToolStatusConfig.SearchBoxEnabled)
        searchMenuItem->Flags |= PH_EMENU_CHECKED;
    if (ToolStatusConfig.ToolBarLocked)
        lockMenuItem->Flags |= PH_EMENU_CHECKED;

    PhInsertEMenuItem(menuInfo->Menu, menu, insertIndex);
}

VOID UpdateCachedSettings(
    VOID
    )
{
    IconSingleClick = !!PhGetIntegerSetting(L"IconSingleClick");
    EnableAvxSupport = !!PhGetIntegerSetting(L"EnableAvxSupport");
    EnableGraphMaxScale = !!PhGetIntegerSetting(L"EnableGraphMaxScale");

    if (ToolbarInitialized)
    {
        SystemInformer_Invoke(UpdateDpiMetrics, NULL);
    }
}

VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    ToolStatusConfig.Flags = PhGetIntegerSetting(SETTING_NAME_TOOLSTATUS_CONFIG);
    DisplayStyle = PhGetIntegerSetting(SETTING_NAME_TOOLBARDISPLAYSTYLE);
    SearchBoxDisplayMode = PhGetIntegerSetting(SETTING_NAME_SEARCHBOXDISPLAYMODE);
    TaskbarListIconType = PhGetIntegerSetting(SETTING_NAME_TASKBARDISPLAYSTYLE);
    RestoreRowAfterSearch = !!PhGetIntegerSetting(SETTING_NAME_RESTOREROWAFTERSEARCH);
    EnableThemeSupport = !!PhGetIntegerSetting(L"EnableThemeSupport");
    UpdateGraphs = !PhGetIntegerSetting(L"StartHidden");
    TabInfoHashtable = PhCreateSimpleHashtable(3);

    // Note: The initialization delay improves performance during application
    // launch and was made configurable per feature request. (dmex)
    MaxInitializationDelay = PhGetIntegerSetting(SETTING_NAME_DELAYED_INITIALIZATION_MAX);
    MaxInitializationDelay = __max(0, __min(MaxInitializationDelay, 5));

    MainWindowHookProc = SystemInformer_GetWindowProcedure();
    SystemInformer_SetWindowProcedure(MainWindowCallbackProc);

    UpdateCachedSettings();

    ToolbarGraphsInitialize();
}

VOID NTAPI SettingsUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    UpdateCachedSettings();
}

VOID NTAPI MenuItemCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = (PPH_PLUGIN_MENU_ITEM)Parameter;

    if (menuItem && menuItem->Id != ULONG_MAX)
    {
        switch (menuItem->Id)
        {
        case COMMAND_ID_ENABLE_MENU:
            {
                ToolStatusConfig.AutoHideMenu = !ToolStatusConfig.AutoHideMenu;

                PhSetIntegerSetting(SETTING_NAME_TOOLSTATUS_CONFIG, ToolStatusConfig.Flags);

                if (ToolStatusConfig.AutoHideMenu)
                {
                    SetMenu(menuItem->OwnerWindow, NULL);
                }
                else
                {
                    SetMenu(menuItem->OwnerWindow, MainMenu);
                    DrawMenuBar(menuItem->OwnerWindow);
                }
            }
            break;
        case COMMAND_ID_ENABLE_SEARCHBOX:
            {
                ToolStatusConfig.SearchBoxEnabled = !ToolStatusConfig.SearchBoxEnabled;

                PhSetIntegerSetting(SETTING_NAME_TOOLSTATUS_CONFIG, ToolStatusConfig.Flags);

                ToolbarLoadSettings(FALSE);
                ReBarSaveLayoutSettings();

                if (ToolStatusConfig.SearchBoxEnabled)
                {
                    // Adding the Searchbox makes it focused,
                    // reset the focus back to the main window.
                    SetFocus(menuItem->OwnerWindow);
                }
            }
            break;
        case COMMAND_ID_TOOLBAR_LOCKUNLOCK:
            {
                UINT bandCount;
                UINT bandIndex;

                bandCount = (UINT)SendMessage(RebarHandle, RB_GETBANDCOUNT, 0, 0);

                for (bandIndex = 0; bandIndex < bandCount; bandIndex++)
                {
                    REBARBANDINFO rebarBandInfo =
                    {
                        sizeof(REBARBANDINFO),
                        RBBIM_STYLE
                    };

                    SendMessage(RebarHandle, RB_GETBANDINFO, bandIndex, (LPARAM)&rebarBandInfo);

                    if (!FlagOn(rebarBandInfo.fStyle, RBBS_GRIPPERALWAYS))
                    {
                        // Removing the RBBS_NOGRIPPER style doesn't remove the gripper padding,
                        // So we toggle the RBBS_GRIPPERALWAYS style to make the Toolbar remove the padding.

                        SetFlag(rebarBandInfo.fStyle, RBBS_GRIPPERALWAYS);

                        SendMessage(RebarHandle, RB_SETBANDINFO, bandIndex, (LPARAM)&rebarBandInfo);

                        ClearFlag(rebarBandInfo.fStyle, RBBS_GRIPPERALWAYS);
                    }

                    if (FlagOn(rebarBandInfo.fStyle, RBBS_NOGRIPPER))
                    {
                        ClearFlag(rebarBandInfo.fStyle, RBBS_NOGRIPPER);
                    }
                    else
                    {
                        SetFlag(rebarBandInfo.fStyle, RBBS_NOGRIPPER);
                    }

                    SendMessage(RebarHandle, RB_SETBANDINFO, bandIndex, (LPARAM)&rebarBandInfo);
                }

                ToolStatusConfig.ToolBarLocked = !ToolStatusConfig.ToolBarLocked;

                PhSetIntegerSetting(SETTING_NAME_TOOLSTATUS_CONFIG, ToolStatusConfig.Flags);

                ToolbarLoadSettings(FALSE);
            }
            break;
        case COMMAND_ID_TOOLBAR_CUSTOMIZE:
            {
                ToolBarShowCustomizeDialog(menuItem->OwnerWindow);
            }
            break;
        case COMMAND_ID_GRAPHS_CUSTOMIZE:
            {
                PPH_TOOLBAR_GRAPH icon;

                icon = menuItem->Context;
                ToolbarSetVisibleGraph(icon, !(icon->Flags & PH_NF_ICON_ENABLED));

                ToolbarGraphSaveSettings();
                ReBarSaveLayoutSettings();
            }
            break;
        }
    }
}

VOID NTAPI ShowOptionsCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_OPTIONS_POINTERS optionsEntry = (PPH_PLUGIN_OPTIONS_POINTERS)Parameter;

    optionsEntry->CreateSection(
        L"ToolStatus",
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS),
        OptionsDlgProc,
        NULL
        );
}

LOGICAL DllMain(
    _In_ HINSTANCE Instance,
    _In_ ULONG Reason,
    _Reserved_ PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PPH_PLUGIN_INFORMATION info;
            PH_SETTING_CREATE settings[] =
            {
                { IntegerSettingType, SETTING_NAME_TOOLSTATUS_CONFIG, L"3F" },
                { IntegerSettingType, SETTING_NAME_TOOLBARDISPLAYSTYLE, L"1" },
                { IntegerSettingType, SETTING_NAME_SEARCHBOXDISPLAYMODE, L"0" },
                { IntegerSettingType, SETTING_NAME_TASKBARDISPLAYSTYLE, L"0" },
                { IntegerSettingType, SETTING_NAME_SHOWSYSINFOGRAPH, L"1" },
                { IntegerSettingType, SETTING_NAME_DELAYED_INITIALIZATION_MAX, L"3" },
                { StringSettingType, SETTING_NAME_REBAR_CONFIG, L"" },
                { StringSettingType, SETTING_NAME_TOOLBAR_CONFIG, L"" },
                { StringSettingType, SETTING_NAME_STATUSBAR_CONFIG, L"" },
                { StringSettingType, SETTING_NAME_TOOLBAR_GRAPH_CONFIG, L"" },
                { IntegerSettingType, SETTING_NAME_RESTOREROWAFTERSEARCH, L"0" },
            };

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Toolbar and Status Bar";
            info->Description = L"Adds a Toolbar, Status Bar and Search box.";
            info->Interface = (PVOID)&PluginInterface;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackMenuItem),
                MenuItemCallback,
                NULL,
                &PluginMenuItemCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainWindowShowing),
                MainWindowShowingCallback,
                NULL,
                &MainWindowShowingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainMenuInitializing),
                MainMenuInitializingCallback,
                NULL,
                &MainMenuInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackOptionsWindowInitializing),
                ShowOptionsCallback,
                NULL,
                &PluginShowOptionsCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackSettingsUpdated),
                SettingsUpdatedCallback,
                NULL,
                &SettingsUpdatedCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessesUpdated),
                ProcessesUpdatedCallback,
                NULL,
                &ProcessesUpdatedCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainWindowTabChanged),
                TabPageUpdatedCallback,
                NULL,
                &TabPageCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessTreeNewInitializing),
                TreeNewInitializingCallback,
                &ProcessTreeNewHandle,
                &ProcessTreeNewInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackServiceTreeNewInitializing),
                TreeNewInitializingCallback,
                &ServiceTreeNewHandle,
                &ServiceTreeNewInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackNetworkTreeNewInitializing),
                TreeNewInitializingCallback,
                &NetworkTreeNewHandle,
                &NetworkTreeNewInitializingCallbackRegistration
                );

            PhAddSettings(settings, RTL_NUMBER_OF(settings));
        }
        break;
    }

    return TRUE;
}
