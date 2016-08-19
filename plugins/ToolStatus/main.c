/*
 * Process Hacker ToolStatus -
 *   main program
 *
 * Copyright (C) 2011-2016 dmex
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

#include "toolstatus.h"

PPH_STRING GetSearchboxText(
    VOID
    );

VOID RegisterTabSearch(
    _In_ INT TabIndex,
    _In_ PWSTR BannerText
    );

PTOOLSTATUS_TAB_INFO RegisterTabInfo(
    _In_ INT TabIndex
    );

TOOLSTATUS_CONFIG ToolStatusConfig = { 0 };
HWND ProcessTreeNewHandle = NULL;
HWND ServiceTreeNewHandle = NULL;
HWND NetworkTreeNewHandle = NULL;
INT SelectedTabIndex;
BOOLEAN UpdateAutomatically = TRUE;
BOOLEAN UpdateGraphs = TRUE;
TOOLBAR_THEME ToolBarTheme = TOOLBAR_THEME_NONE;
TOOLBAR_DISPLAY_STYLE DisplayStyle = TOOLBAR_DISPLAY_STYLE_SELECTIVETEXT;
SEARCHBOX_DISPLAY_MODE SearchBoxDisplayMode = SEARCHBOX_DISPLAY_MODE_ALWAYSSHOW;
REBAR_DISPLAY_LOCATION RebarDisplayLocation = REBAR_DISPLAY_LOCATION_TOP;
HWND RebarHandle = NULL;
HWND ToolBarHandle = NULL;
HWND SearchboxHandle = NULL;
HMENU MainMenu = NULL;
HACCEL AcceleratorTable = NULL;
PPH_STRING SearchboxText = NULL;
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
    GetSearchboxText,
    WordMatchStringRef,
    RegisterTabSearch,
    &SearchChangedEvent,
    RegisterTabInfo
};

static ULONG TargetingMode = 0;
static BOOLEAN TargetingWindow = FALSE;
static BOOLEAN TargetingCurrentWindowDraw = FALSE;
static BOOLEAN TargetingCompleted = FALSE;
static HWND TargetingCurrentWindow = NULL;
static PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
static PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;
static PH_CALLBACK_REGISTRATION LayoutPaddingCallbackRegistration;
static PH_CALLBACK_REGISTRATION TabPageCallbackRegistration;
static PH_CALLBACK_REGISTRATION ProcessTreeNewInitializingCallbackRegistration;
static PH_CALLBACK_REGISTRATION ServiceTreeNewInitializingCallbackRegistration;
static PH_CALLBACK_REGISTRATION NetworkTreeNewInitializingCallbackRegistration;

PPH_STRING GetSearchboxText(
    VOID
    )
{
    return SearchboxText;
}

VOID NTAPI ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    ProcessesUpdatedCount++;

    if (ProcessesUpdatedCount < 2)
        return;

    PhPluginGetSystemStatistics(&SystemStatistics);

    if (UpdateGraphs)
        ToolbarUpdateGraphs();

    if (ToolStatusConfig.StatusBarEnabled)
        StatusBarUpdate(FALSE);
}

VOID NTAPI TreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    *(HWND *)Context = ((PPH_PLUGIN_TREENEW_INFORMATION)Parameter)->TreeNewHandle;
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
    VOID
    )
{
    POINT cursorPos;
    PPH_EMENU menu;
    PPH_EMENU_ITEM selectedItem;

    GetCursorPos(&cursorPos);

    menu = PhCreateEMenu();
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, COMMAND_ID_ENABLE_MENU, L"Main menu (auto-hide)", NULL, NULL), -1);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, COMMAND_ID_ENABLE_SEARCHBOX, L"Search box", NULL, NULL), -1);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, COMMAND_ID_ENABLE_CPU_GRAPH, L"CPU history", NULL, NULL), -1);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, COMMAND_ID_ENABLE_IO_GRAPH, L"I/O history", NULL, NULL), -1);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, COMMAND_ID_ENABLE_MEMORY_GRAPH, L"Physical memory history", NULL, NULL), -1);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, COMMAND_ID_ENABLE_COMMIT_GRAPH, L"Commit charge history", NULL, NULL), -1);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(PH_EMENU_SEPARATOR, 0, NULL, NULL, NULL), -1);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, COMMAND_ID_TOOLBAR_LOCKUNLOCK, L"Lock the toolbar", NULL, NULL), -1);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, COMMAND_ID_TOOLBAR_CUSTOMIZE, L"Customize...", NULL, NULL), -1);
    
    if (ToolStatusConfig.AutoHideMenu)
    {
        PhSetFlagsEMenuItem(menu, COMMAND_ID_ENABLE_MENU, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
    }

    if (ToolStatusConfig.SearchBoxEnabled)
    {
        PhSetFlagsEMenuItem(menu, COMMAND_ID_ENABLE_SEARCHBOX, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
    }

    if (ToolStatusConfig.CpuGraphEnabled)
    {
        PhSetFlagsEMenuItem(menu, COMMAND_ID_ENABLE_CPU_GRAPH, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
    }

    if (ToolStatusConfig.MemGraphEnabled)
    {
        PhSetFlagsEMenuItem(menu, COMMAND_ID_ENABLE_MEMORY_GRAPH, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
    }

    if (ToolStatusConfig.CommitGraphEnabled)
    {
        PhSetFlagsEMenuItem(menu, COMMAND_ID_ENABLE_COMMIT_GRAPH, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
    }

    if (ToolStatusConfig.IoGraphEnabled)
    {
        PhSetFlagsEMenuItem(menu, COMMAND_ID_ENABLE_IO_GRAPH, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
    }

    if (ToolStatusConfig.ToolBarLocked)
    {
        PhSetFlagsEMenuItem(menu, COMMAND_ID_TOOLBAR_LOCKUNLOCK, PH_EMENU_CHECKED, PH_EMENU_CHECKED);
    }

    selectedItem = PhShowEMenu(
        menu,
        PhMainWndHandle,
        PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_TOP,
        cursorPos.x,
        cursorPos.y
        );

    if (selectedItem && selectedItem->Id != -1)
    {
        switch (selectedItem->Id)
        {
        case COMMAND_ID_ENABLE_MENU:
            {
                ToolStatusConfig.AutoHideMenu = !ToolStatusConfig.AutoHideMenu;

                PhSetIntegerSetting(SETTING_NAME_TOOLSTATUS_CONFIG, ToolStatusConfig.Flags);

                if (ToolStatusConfig.AutoHideMenu)
                {
                    SetMenu(PhMainWndHandle, NULL);
                }
                else
                {
                    SetMenu(PhMainWndHandle, MainMenu);
                    DrawMenuBar(PhMainWndHandle);
                }
            }
            break;
        case COMMAND_ID_ENABLE_SEARCHBOX:
            {
                ToolStatusConfig.SearchBoxEnabled = !ToolStatusConfig.SearchBoxEnabled;

                PhSetIntegerSetting(SETTING_NAME_TOOLSTATUS_CONFIG, ToolStatusConfig.Flags);

                ToolbarLoadSettings();
                ReBarSaveLayoutSettings();

                if (ToolStatusConfig.SearchBoxEnabled)
                {
                    // Adding the Searchbox makes it focused,
                    // reset the focus back to the main window.
                    SetFocus(PhMainWndHandle);
                }
            }
            break;
        case COMMAND_ID_ENABLE_CPU_GRAPH:
            {
                ToolStatusConfig.CpuGraphEnabled = !ToolStatusConfig.CpuGraphEnabled;

                PhSetIntegerSetting(SETTING_NAME_TOOLSTATUS_CONFIG, ToolStatusConfig.Flags);

                ToolbarLoadSettings();
                ReBarSaveLayoutSettings();
            }
            break;
        case COMMAND_ID_ENABLE_MEMORY_GRAPH:
            {
                ToolStatusConfig.MemGraphEnabled = !ToolStatusConfig.MemGraphEnabled;

                PhSetIntegerSetting(SETTING_NAME_TOOLSTATUS_CONFIG, ToolStatusConfig.Flags);

                ToolbarLoadSettings();
                ReBarSaveLayoutSettings();
            }
            break;
        case COMMAND_ID_ENABLE_COMMIT_GRAPH:
            {
                ToolStatusConfig.CommitGraphEnabled = !ToolStatusConfig.CommitGraphEnabled;

                PhSetIntegerSetting(SETTING_NAME_TOOLSTATUS_CONFIG, ToolStatusConfig.Flags);

                ToolbarLoadSettings();
                ReBarSaveLayoutSettings();
            }
            break;
        case COMMAND_ID_ENABLE_IO_GRAPH:
            {
                ToolStatusConfig.IoGraphEnabled = !ToolStatusConfig.IoGraphEnabled;

                PhSetIntegerSetting(SETTING_NAME_TOOLSTATUS_CONFIG, ToolStatusConfig.Flags);

                ToolbarLoadSettings();
                ReBarSaveLayoutSettings();
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

                        rebarBandInfo.fStyle |= RBBS_GRIPPERALWAYS;

                        SendMessage(RebarHandle, RB_SETBANDINFO, bandIndex, (LPARAM)&rebarBandInfo);

                        rebarBandInfo.fStyle &= ~RBBS_GRIPPERALWAYS;
                    }

                    if (rebarBandInfo.fStyle & RBBS_NOGRIPPER)
                    {
                        rebarBandInfo.fStyle &= ~RBBS_NOGRIPPER;
                    }
                    else
                    {
                        rebarBandInfo.fStyle |= RBBS_NOGRIPPER;
                    }

                    SendMessage(RebarHandle, RB_SETBANDINFO, bandIndex, (LPARAM)&rebarBandInfo);
                }

                ToolStatusConfig.ToolBarLocked = !ToolStatusConfig.ToolBarLocked;

                PhSetIntegerSetting(SETTING_NAME_TOOLSTATUS_CONFIG, ToolStatusConfig.Flags);

                ToolbarLoadSettings();
            }
            break;
        case COMMAND_ID_TOOLBAR_CUSTOMIZE:
            {
                ToolBarShowCustomizeDialog();
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
}

VOID NTAPI LayoutPaddingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_LAYOUT_PADDING_DATA layoutPadding = Parameter;

    if (RebarHandle && ToolStatusConfig.ToolBarEnabled)
    {
        RECT rebarRect;
        //RECT clientRect;
        //INT x, y, cx, cy;

        SendMessage(RebarHandle, WM_SIZE, 0, 0);

        // TODO: GetClientRect with PhMainWndHandle causes crash.
        //GetClientRect(PhMainWndHandle, &clientRect);
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

BOOLEAN NTAPI MessageLoopFilter(
    _In_ PMSG Message,
    _In_ PVOID Context
    )
{
    if (
        Message->hwnd == PhMainWndHandle ||
        IsChild(PhMainWndHandle, Message->hwnd)
        )
    {
        if (TranslateAccelerator(PhMainWndHandle, AcceleratorTable, Message))
            return TRUE;

        if (Message->message == WM_SYSCHAR && ToolStatusConfig.AutoHideMenu && !GetMenu(PhMainWndHandle))
        {
            ULONG key = (ULONG)Message->wParam;

            if (key == 'h' || key == 'v' || key == 't' || key == 'u' || key == 'e')
            {
                SetMenu(PhMainWndHandle, MainMenu);
                DrawMenuBar(PhMainWndHandle);
                SendMessage(PhMainWndHandle, WM_SYSCHAR, Message->wParam, Message->lParam);
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

    GetWindowRect(hWnd, &rect);
    hdc = GetWindowDC(hWnd);

    if (hdc)
    {
        INT penWidth;
        INT oldDc;
        HPEN pen;
        HBRUSH brush;

        penWidth = GetSystemMetrics(SM_CXBORDER) * 3;
        oldDc = SaveDC(hdc);

        // Get an inversion effect.
        SetROP2(hdc, R2_NOT);

        pen = CreatePen(PS_INSIDEFRAME, penWidth, RGB(0x00, 0x00, 0x00));
        SelectPen(hdc, pen);

        brush = GetStockBrush(NULL_BRUSH);
        SelectBrush(hdc, brush);

        // Draw the rectangle.
        Rectangle(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top);

        // Cleanup.
        DeleteObject(pen);

        RestoreDC(hdc, oldDc);
        ReleaseDC(hWnd, hdc);
    }
}

LRESULT CALLBACK MainWndSubclassProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam,
    _In_ UINT_PTR uIdSubclass,
    _In_ ULONG_PTR dwRefData
    )
{
    switch (uMsg)
    {
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_CMD(wParam, lParam))
            {
            case EN_CHANGE:
                {
                    PPH_STRING newSearchboxText;

                    if (GET_WM_COMMAND_HWND(wParam, lParam) != SearchboxHandle)
                        break;

                    newSearchboxText = PH_AUTO(PhGetWindowText(SearchboxHandle));

                    if (!PhEqualString(SearchboxText, newSearchboxText, FALSE))
                    {
                        // Cache the current search text for our callback.
                        PhSwapReference(&SearchboxText, newSearchboxText);

                        if (!PhIsNullOrEmptyString(SearchboxText))
                        {
                            // Expand the nodes to ensure that they will be visible to the user.
                            PhExpandAllProcessNodes(TRUE);
                            PhDeselectAllProcessNodes();
                            PhDeselectAllServiceNodes();
                        }

                        PhApplyTreeNewFilters(PhGetFilterSupportProcessTreeList());
                        PhApplyTreeNewFilters(PhGetFilterSupportServiceTreeList());
                        PhApplyTreeNewFilters(PhGetFilterSupportNetworkTreeList());

                        PhInvokeCallback(&SearchChangedEvent, SearchboxText);
                    }

                    goto DefaultWndProc;
                }
                break;
            case EN_KILLFOCUS:
                {
                    if (GET_WM_COMMAND_HWND(wParam, lParam) != SearchboxHandle)
                        break;

                    if (SearchBoxDisplayMode != SEARCHBOX_DISPLAY_MODE_HIDEINACTIVE)
                        break;

                    if (SearchboxText->Length == 0)
                    {
                        if (RebarBandExists(REBAR_BAND_ID_SEARCHBOX))
                            RebarBandRemove(REBAR_BAND_ID_SEARCHBOX);
                    }
                }
                break;
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
                }
                break;
            case ID_SEARCH:
                {
                    // handle keybind Ctrl + K
                    if (SearchboxHandle && ToolStatusConfig.SearchBoxEnabled)
                    {
                        SetFocus(SearchboxHandle);
                        Edit_SetSel(SearchboxHandle, 0, -1);
                    }

                    goto DefaultWndProc;
                }
                break;
            case ID_SEARCH_CLEAR:
                {
                    if (SearchboxHandle && ToolStatusConfig.SearchBoxEnabled)
                    {
                        SetFocus(SearchboxHandle);
                        Static_SetText(SearchboxHandle, L"");
                    }

                    goto DefaultWndProc;
                }
                break;
            case PHAPP_ID_VIEW_ALWAYSONTOP:
                {
                    // Let Process Hacker perform the default processing.
                    DefSubclassProc(hWnd, uMsg, wParam, lParam);

                    // Query the settings.
                    BOOLEAN isAlwaysOnTopEnabled = (BOOLEAN)PhGetIntegerSetting(L"MainWindowAlwaysOnTop");

                    // Set the pressed button state.
                    SendMessage(ToolBarHandle, TB_PRESSBUTTON, (WPARAM)PHAPP_ID_VIEW_ALWAYSONTOP, (LPARAM)(MAKELONG(isAlwaysOnTopEnabled, 0)));

                    goto DefaultWndProc;
                }
                break;
            case PHAPP_ID_UPDATEINTERVAL_FAST:
            case PHAPP_ID_UPDATEINTERVAL_NORMAL:
            case PHAPP_ID_UPDATEINTERVAL_BELOWNORMAL:
            case PHAPP_ID_UPDATEINTERVAL_SLOW:
            case PHAPP_ID_UPDATEINTERVAL_VERYSLOW:
                {
                    // Let Process Hacker perform the default processing.
                    DefSubclassProc(hWnd, uMsg, wParam, lParam);

                    StatusBarUpdate(TRUE);

                    goto DefaultWndProc;
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
                        // Invoke the LayoutPaddingCallback.
                        SendMessage(PhMainWndHandle, WM_SIZE, 0, 0);
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
                            if (SendMessage(ToolBarHandle, TB_GETITEMRECT, index, (LPARAM)&buttonRect) == -1)
                                break;

                            if (buttonRect.right <= toolbarRect.right)
                                continue;

                            // Get extended button information.
                            if (SendMessage(ToolBarHandle, TB_GETBUTTONINFO, index, (LPARAM)&buttonInfo) == -1)
                                break;

                            if (buttonInfo.fsStyle == BTNS_SEP)
                            {
                                // Add separators to menu.
                                PhInsertEMenuItem(menu, PhCreateEMenuItem(PH_EMENU_SEPARATOR, 0, NULL, NULL, NULL), -1);
                            }
                            else
                            {
                                PPH_EMENU_ITEM menuItem;

                                if (PhGetOwnTokenAttributes().Elevated && buttonInfo.idCommand == PHAPP_ID_HACKER_SHOWDETAILSFORALLPROCESSES)
                                {
                                    // Don't show the 'Show Details for All Processes' button in the
                                    //  dropdown menu when we're elevated.
                                    continue;
                                }

                                // Add buttons to menu.
                                menuItem = PhCreateEMenuItem(0, buttonInfo.idCommand, ToolbarGetText(buttonInfo.idCommand), NULL, NULL);

                                menuItem->Flags |= PH_EMENU_BITMAP_OWNED;
                                menuItem->Bitmap = ToolbarGetImage(buttonInfo.idCommand);

                                switch (buttonInfo.idCommand)
                                {
                                case PHAPP_ID_VIEW_ALWAYSONTOP:
                                    {
                                        // Set the pressed state.
                                        if (PhGetIntegerSetting(L"MainWindowAlwaysOnTop"))
                                            menuItem->Flags |= PH_EMENU_CHECKED;
                                    }
                                    break;
                                case TIDC_FINDWINDOW:
                                case TIDC_FINDWINDOWTHREAD:
                                case TIDC_FINDWINDOWKILL:
                                    {
                                        // Note: These buttons are incompatible with menus.
                                        menuItem->Flags |= PH_EMENU_DISABLED;
                                    }
                                    break;
                                case TIDC_POWERMENUDROPDOWN:
                                    {
                                        // Create the sub-menu...
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_LOCK, L"&Lock", NULL, NULL), -1);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_LOGOFF, L"Log o&ff", NULL, NULL), -1);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(PH_EMENU_SEPARATOR, 0, NULL, NULL, NULL), -1);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_SLEEP, L"&Sleep", NULL, NULL), -1);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_HIBERNATE, L"&Hibernate", NULL, NULL), -1);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(PH_EMENU_SEPARATOR, 0, NULL, NULL, NULL), -1);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_RESTART, L"R&estart", NULL, NULL), -1);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_RESTARTBOOTOPTIONS, L"Restart to boot &options", NULL, NULL), -1);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_SHUTDOWN, L"Shu&t down", NULL, NULL), -1);
                                        PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_SHUTDOWNHYBRID, L"H&ybrid shut down", NULL, NULL), -1);
                                    }
                                    break;
                                }

                                PhInsertEMenuItem(menu, menuItem, -1);
                            }
                        }

                        MapWindowPoints(RebarHandle, NULL, (LPPOINT)&rebar->rc, 2);

                        selectedItem = PhShowEMenu(
                            menu,
                            hWnd,
                            PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP,
                            rebar->rc.left,
                            rebar->rc.bottom
                            );

                        if (selectedItem && selectedItem->Id != -1)
                        {
                            SendMessage(PhMainWndHandle, WM_COMMAND, MAKEWPARAM(selectedItem->Id, BN_CLICKED), 0);
                        }

                        PhDestroyEMenu(menu);
                    }
                    break;
                case RBN_LAYOUTCHANGED:
                    {
                        ReBarSaveLayoutSettings();
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

                        if (toolbarDisplayInfo->dwMask & TBNF_IMAGE)
                        {
                            BOOLEAN found = FALSE;

                            // Try to find the cached bitmap index.
                            // NOTE: The TBNF_DI_SETITEM flag below will cache the index so we only get called once.
                            //       However, when adding buttons from the customize dialog we get called a second time,
                            //       so we cache the index in our ToolbarButtons array to prevent ToolBarImageList from growing.
                            for (INT i = 0; i < ARRAYSIZE(ToolbarButtons); i++)
                            {
                                if (ToolbarButtons[i].idCommand == toolbarDisplayInfo->idCommand)
                                {
                                    if (ToolbarButtons[i].iBitmap != I_IMAGECALLBACK)
                                    {
                                        found = TRUE;

                                        // Cache the bitmap index.
                                        toolbarDisplayInfo->dwMask |= TBNF_DI_SETITEM;
                                        // Set the bitmap index.
                                        toolbarDisplayInfo->iImage = ToolbarButtons[i].iBitmap;
                                    }
                                    break;
                                }
                            }

                            if (!found)
                            {
                                // We didn't find a cached bitmap index...
                                // Load the button bitmap and cache the index.
                                for (INT i = 0; i < ARRAYSIZE(ToolbarButtons); i++)
                                {
                                    if (ToolbarButtons[i].idCommand == toolbarDisplayInfo->idCommand)
                                    {
                                        HBITMAP buttonImage;

                                        buttonImage = ToolbarGetImage(toolbarDisplayInfo->idCommand);

                                        // Cache the bitmap index.
                                        toolbarDisplayInfo->dwMask |= TBNF_DI_SETITEM;
                                        // Add the image, cache the value in the ToolbarButtons array, set the bitmap index.
                                        toolbarDisplayInfo->iImage = ToolbarButtons[i].iBitmap = ImageList_Add(
                                            ToolBarImageList,
                                            buttonImage,
                                            NULL
                                            );

                                        DeleteObject(buttonImage);
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
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_LOCK, L"&Lock", NULL, NULL), -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_LOGOFF, L"Log o&ff", NULL, NULL), -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(PH_EMENU_SEPARATOR, 0, NULL, NULL, NULL), -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_SLEEP, L"&Sleep", NULL, NULL), -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_HIBERNATE, L"&Hibernate", NULL, NULL), -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(PH_EMENU_SEPARATOR, 0, NULL, NULL, NULL), -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_RESTART, L"R&estart", NULL, NULL), -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_RESTARTBOOTOPTIONS, L"Restart to boot &options", NULL, NULL), -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_SHUTDOWN, L"Shu&t down", NULL, NULL), -1);
                        PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_ID_COMPUTER_SHUTDOWNHYBRID, L"H&ybrid shut down", NULL, NULL), -1);

                        MapWindowPoints(ToolBarHandle, NULL, (LPPOINT)&toolbar->rcButton, 2);

                        selectedItem = PhShowEMenu(
                            menu,
                            hWnd,
                            PH_EMENU_SHOW_LEFTRIGHT,
                            PH_ALIGN_LEFT | PH_ALIGN_TOP,
                            toolbar->rcButton.left,
                            toolbar->rcButton.bottom
                            );

                        if (selectedItem && selectedItem->Id != -1)
                        {
                            SendMessage(PhMainWndHandle, WM_COMMAND, MAKEWPARAM(selectedItem->Id, BN_CLICKED), 0);
                        }

                        PhDestroyEMenu(menu);
                    }
                    return TBDDRET_DEFAULT;
                case NM_LDOWN:
                    {
                        LPNMCLICK toolbar = (LPNMCLICK)hdr;
                        ULONG id = (ULONG)toolbar->dwItemSpec;

                        if (id == -1)
                            break;

                        if (id == TIDC_FINDWINDOW || id == TIDC_FINDWINDOWTHREAD || id == TIDC_FINDWINDOWKILL)
                        {
                            // Direct all mouse events to this window.
                            SetCapture(hWnd);

                            // Set the cursor.
                            SetCursor(LoadCursor(NULL, IDC_CROSS));

                            // Send the window to the bottom.
                            SetWindowPos(hWnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

                            TargetingWindow = TRUE;
                            TargetingCurrentWindow = NULL;
                            TargetingCurrentWindowDraw = FALSE;
                            TargetingCompleted = FALSE;
                            TargetingMode = id;

                            SendMessage(hWnd, WM_MOUSEMOVE, 0, 0);
                        }
                    }
                    break;
                case NM_RCLICK:
                    {
                        ShowCustomizeMenu();
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
                        StatusBarShowMenu();
                    }
                    break;
                }

                goto DefaultWndProc;
            }
            else if (
                CpuGraphHandle && hdr->hwndFrom == CpuGraphHandle ||
                MemGraphHandle && hdr->hwndFrom == MemGraphHandle ||
                CommitGraphHandle && hdr->hwndFrom == CommitGraphHandle ||
                IoGraphHandle && hdr->hwndFrom == IoGraphHandle
                )
            {
                ToolbarUpdateGraphsInfo(hdr);

                goto DefaultWndProc;
            }
        }
        break;
    case WM_MOUSEMOVE:
        {
            if (TargetingWindow)
            {
                POINT cursorPos;
                HWND windowOverMouse;
                ULONG processId;
                ULONG threadId;

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
                        threadId = GetWindowThreadProcessId(windowOverMouse, &processId);

                        // Draw a rectangle over the current window (but not if it's one of our own).
                        if (UlongToHandle(processId) != NtCurrentProcessId())
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
                ULONG processId;
                ULONG threadId;

                TargetingCompleted = TRUE;

                // Reset the original cursor.
                SetCursor(LoadCursor(NULL, IDC_ARROW));

                // Bring the window back to the top, and preserve the Always on Top setting.
                SetWindowPos(PhMainWndHandle, PhGetIntegerSetting(L"MainWindowAlwaysOnTop") ? HWND_TOPMOST : HWND_TOP,
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
                        // This is an undocumented function exported by user32.dll that
                        // retrieves the hung window represented by a ghost window.
                        static HWND (WINAPI *HungWindowFromGhostWindow_I)(
                            _In_ HWND hWnd
                            );

                        if (!HungWindowFromGhostWindow_I)
                            HungWindowFromGhostWindow_I = PhGetModuleProcAddress(L"user32.dll", "HungWindowFromGhostWindow");

                        if (HungWindowFromGhostWindow_I)
                        {
                            HWND hungWindow = HungWindowFromGhostWindow_I(TargetingCurrentWindow);

                            // The call will have failed if the window wasn't actually a ghost
                            // window.
                            if (hungWindow)
                                TargetingCurrentWindow = hungWindow;
                        }
                    }

                    threadId = GetWindowThreadProcessId(TargetingCurrentWindow, &processId);

                    if (threadId && processId && UlongToHandle(processId) != NtCurrentProcessId())
                    {
                        PPH_PROCESS_NODE processNode;

                        processNode = PhFindProcessNode(UlongToHandle(processId));

                        if (processNode)
                        {
                            ProcessHacker_SelectTabPage(hWnd, 0);
                            ProcessHacker_SelectProcessNode(hWnd, processNode);
                        }

                        switch (TargetingMode)
                        {
                        case TIDC_FINDWINDOWTHREAD:
                            {
                                PPH_PROCESS_PROPCONTEXT propContext;
                                PPH_PROCESS_ITEM processItem;

                                if (processItem = PhReferenceProcessItem(UlongToHandle(processId)))
                                {
                                    if (propContext = PhCreateProcessPropContext(hWnd, processItem))
                                    {
                                        PhSetSelectThreadIdProcessPropContext(propContext, UlongToHandle(threadId));
                                        PhShowProcessProperties(propContext);
                                        PhDereferenceObject(propContext);
                                    }

                                    PhDereferenceObject(processItem);
                                }
                                else
                                {
                                    PhShowError(hWnd, L"The process (PID %lu) does not exist.", processId);
                                }
                            }
                            break;
                        case TIDC_FINDWINDOWKILL:
                            {
                                PPH_PROCESS_ITEM processItem;

                                if (processItem = PhReferenceProcessItem(UlongToHandle(processId)))
                                {
                                    PhUiTerminateProcesses(hWnd, &processItem, 1);
                                    PhDereferenceObject(processItem);
                                }
                                else
                                {
                                    PhShowError(hWnd, L"The process (PID %lu) does not exist.", processId);
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

                // Remove the border on the currently selected window.
                if (TargetingCurrentWindow)
                {
                    if (TargetingCurrentWindowDraw)
                    {
                        // Remove the border on the window we found.
                        DrawWindowBorderForTargeting(TargetingCurrentWindow);
                    }
                }

                SetWindowPos(PhMainWndHandle, PhGetIntegerSetting(L"MainWindowAlwaysOnTop") ? HWND_TOPMOST : HWND_TOP,
                    0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

                TargetingCompleted = TRUE;
            }
        }
        break;
    case WM_SIZE:
        // Resize PH main window client-area.
        ProcessHacker_InvalidateLayoutPadding(hWnd);
        break;
    case WM_SETTINGCHANGE:
        // Forward to the Searchbox so we can reinitialize the settings...
        SendMessage(SearchboxHandle, WM_SETTINGCHANGE, 0, 0);
        break;
    case WM_SHOWWINDOW:
        {
            UpdateGraphs = (BOOLEAN)wParam;
        }
        break;
    case WM_SYSCOMMAND:
        {
            if ((wParam & 0xFFF0) == SC_KEYMENU && lParam == 0)
            {
                if (!ToolStatusConfig.AutoHideMenu)
                    break;

                if (GetMenu(PhMainWndHandle))
                {
                    SetMenu(PhMainWndHandle, NULL);
                }
                else
                {
                    SetMenu(PhMainWndHandle, MainMenu);
                    DrawMenuBar(PhMainWndHandle);
                }
            }
            else if ((wParam & 0xFFF0) == SC_MINIMIZE)
            {
                UpdateGraphs = FALSE;
            }
            else if ((wParam & 0xFFF0) == SC_RESTORE)
            {
                UpdateGraphs = TRUE;
            }
        }
        break;
    case WM_EXITMENULOOP:
        {
            if (!ToolStatusConfig.AutoHideMenu)
                break;

            if (GetMenu(PhMainWndHandle))
            {
                SetMenu(PhMainWndHandle, NULL);
            }
        }
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);

DefaultWndProc:
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

VOID NTAPI MainWindowShowingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PhRegisterMessageLoopFilter(MessageLoopFilter, NULL);
    PhRegisterCallback(
        ProcessHacker_GetCallbackLayoutPadding(PhMainWndHandle),
        LayoutPaddingCallback,
        NULL,
        &LayoutPaddingCallbackRegistration
        );
    SetWindowSubclass(PhMainWndHandle, MainWndSubclassProc, 0, 0);

    ToolbarLoadSettings();
    ReBarLoadLayoutSettings();
    StatusBarLoadSettings();

    MainMenu = GetMenu(PhMainWndHandle);
    if (ToolStatusConfig.AutoHideMenu)
    {
        SetMenu(PhMainWndHandle, NULL);
    }
}

VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    ToolStatusConfig.Flags = PhGetIntegerSetting(SETTING_NAME_TOOLSTATUS_CONFIG);
    ToolBarTheme = (TOOLBAR_THEME)PhGetIntegerSetting(SETTING_NAME_TOOLBAR_THEME);
    DisplayStyle = (TOOLBAR_DISPLAY_STYLE)PhGetIntegerSetting(SETTING_NAME_TOOLBARDISPLAYSTYLE);
    SearchBoxDisplayMode = (SEARCHBOX_DISPLAY_MODE)PhGetIntegerSetting(SETTING_NAME_SEARCHBOXDISPLAYMODE);
    UpdateGraphs = !PhGetIntegerSetting(L"StartHidden");
}

VOID NTAPI ShowOptionsCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    ShowOptionsDialog(Parameter);
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
                { IntegerSettingType, SETTING_NAME_TOOLSTATUS_CONFIG, L"1F" },
                { IntegerSettingType, SETTING_NAME_TOOLBAR_THEME, L"0" },
                { IntegerSettingType, SETTING_NAME_TOOLBARDISPLAYSTYLE, L"1" },
                { IntegerSettingType, SETTING_NAME_SEARCHBOXDISPLAYMODE, L"0" },
                { StringSettingType, SETTING_NAME_REBAR_CONFIG, L"" },
                { StringSettingType, SETTING_NAME_TOOLBAR_CONFIG, L"" },
                { StringSettingType, SETTING_NAME_STATUSBAR_CONFIG, L"" }
            };

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Toolbar and Status Bar";
            info->Author = L"dmex, wj32";
            info->Description = L"Adds a Toolbar, Status Bar and Search box.\r\n\r\nModern Toolbar icons by http://www.icons8.com";
            info->Url = L"https://wj32.org/processhacker/forums/viewtopic.php?t=1119";
            info->HasOptions = TRUE;
            info->Interface = &PluginInterface;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackShowOptions),
                ShowOptionsCallback,
                NULL,
                &PluginShowOptionsCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainWindowShowing),
                MainWindowShowingCallback,
                NULL,
                &MainWindowShowingCallbackRegistration
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

            PhAddSettings(settings, ARRAYSIZE(settings));

            AcceleratorTable = LoadAccelerators(
                Instance,
                MAKEINTRESOURCE(IDR_MAINWND_ACCEL)
                );

            TabInfoHashtable = PhCreateSimpleHashtable(3);
        }
        break;
    }

    return TRUE;
}