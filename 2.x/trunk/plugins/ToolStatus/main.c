/*
 * Process Hacker ToolStatus -
 *   main program
 *
 * Copyright (C) 2011-2015 dmex
 * Copyright (C) 2010-2013 wj32
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

HWND ProcessTreeNewHandle;
HWND ServiceTreeNewHandle;
HWND NetworkTreeNewHandle;
INT SelectedTabIndex;
BOOLEAN EnableToolBar = FALSE;
BOOLEAN EnableSearchBox = FALSE;
BOOLEAN EnableStatusBar = FALSE;
TOOLBAR_DISPLAY_STYLE DisplayStyle = ToolbarDisplaySelectiveText;
SEARCHBOX_DISPLAY_MODE SearchBoxDisplayMode = SearchBoxDisplayAlwaysShow;
REBAR_DISPLAY_LOCATION RebarDisplayLocation = RebarLocationTop;
HWND RebarHandle = NULL;
HWND ToolBarHandle = NULL;
HWND SearchboxHandle = NULL;
HACCEL AcceleratorTable = NULL;
PPH_STRING SearchboxText = NULL;
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

static PPH_STRING GetSearchboxText(
    VOID
    )
{
    return SearchboxText;
}

static VOID NTAPI ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    ProcessesUpdatedCount++;

    if (EnableStatusBar)
        UpdateStatusBar();
}

VOID NTAPI TreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    *(HWND *)Context = ((PPH_PLUGIN_TREENEW_INFORMATION)Parameter)->TreeNewHandle;
}

static VOID RegisterTabSearch(
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

    if (!PhAddItemSimpleHashtable(TabInfoHashtable, (PVOID)TabIndex, tabInfoCopy))
    {
        PhClearReference(&tabInfoCopy);

        if (entry = PhFindItemSimpleHashtable(TabInfoHashtable, (PVOID)TabIndex))
            tabInfoCopy = *entry;
    }

    return tabInfoCopy;
}

PTOOLSTATUS_TAB_INFO FindTabInfo(
    _In_ INT TabIndex
    )
{
    PVOID *entry;

    if (entry = PhFindItemSimpleHashtable(TabInfoHashtable, (PVOID)TabIndex))
        return *entry;
    else
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

static VOID NTAPI TabPageUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    INT tabIndex = (INT)Parameter;

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
                Edit_SetCueBannerText(SearchboxHandle, L"Search Disabled");
            }
        }
        break;
    }
}

static VOID NTAPI LayoutPaddingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_LAYOUT_PADDING_DATA data = (PPH_LAYOUT_PADDING_DATA)Parameter;

    if (RebarHandle && EnableToolBar)
    {
        RECT rebarRect;

        SendMessage(RebarHandle, WM_SIZE, 0, 0);

        GetClientRect(RebarHandle, &rebarRect);

        // Adjust the PH client area and exclude the rebar width.
        data->Padding.top += rebarRect.bottom;

        //if (SearchBoxDisplayStyle == SearchBoxDisplayAutoHide)
        //{
        //    static BOOLEAN isSearchboxVisible = FALSE;
        //    SIZE idealWidth;
        //
        //    // Query the the Toolbar ideal width
        //    SendMessage(ToolBarHandle, TB_GETIDEALSIZE, FALSE, (LPARAM)&idealWidth);
        //
        //    // Hide the Searcbox band if the window size is too small...
        //    if (rebarRect.right > idealWidth.cx)
        //    {
        //        if (isSearchboxVisible)
        //        {
        //            if (!RebarBandExists(BandID_SearchBox))
        //                RebarBandInsert(BandID_SearchBox, SearchboxHandle, 20, 180);
        //
        //            isSearchboxVisible = FALSE;
        //        }
        //    }
        //    else
        //    {
        //        if (!isSearchboxVisible)
        //        {
        //            if (RebarBandExists(BandID_SearchBox))
        //                RebarBandRemove(BandID_SearchBox);
        //
        //            isSearchboxVisible = TRUE;
        //        }
        //    }
        //}
    }

    if (StatusBarHandle && EnableStatusBar)
    {
        RECT statusBarRect;

        SendMessage(StatusBarHandle, WM_SIZE, 0, 0);

        GetClientRect(StatusBarHandle, &statusBarRect);

        // Adjust the PH client area and exclude the StatusBar width.
        data->Padding.bottom += statusBarRect.bottom;
    }
}

static BOOLEAN NTAPI MessageLoopFilter(
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
    }

    return FALSE;
}

static VOID DrawWindowBorderForTargeting(
    _In_ HWND hWnd
    )
{
    RECT rect;
    HDC hdc;

    GetWindowRect(hWnd, &rect);
    hdc = GetWindowDC(hWnd);

    if (hdc)
    {
        ULONG penWidth = GetSystemMetrics(SM_CXBORDER) * 3;
        INT oldDc;
        HPEN pen;
        HBRUSH brush;

        oldDc = SaveDC(hdc);

        // Get an inversion effect.
        SetROP2(hdc, R2_NOT);

        pen = CreatePen(PS_INSIDEFRAME, penWidth, RGB(0x00, 0x00, 0x00));
        SelectObject(hdc, pen);

        brush = GetStockObject(NULL_BRUSH);
        SelectObject(hdc, brush);

        // Draw the rectangle.
        Rectangle(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top);

        // Cleanup.
        DeleteObject(pen);

        RestoreDC(hdc, oldDc);
        ReleaseDC(hWnd, hdc);
    }
}

static LRESULT CALLBACK MainWndSubclassProc(
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
                    // Cache the current search text for our callback.
                    PhMoveReference(&SearchboxText, PhGetWindowText(SearchboxHandle));

                    // Expand the nodes so we can search them
                    PhExpandAllProcessNodes(TRUE);
                    PhDeselectAllProcessNodes();
                    PhDeselectAllServiceNodes();

                    PhApplyTreeNewFilters(PhGetFilterSupportProcessTreeList());
                    PhApplyTreeNewFilters(PhGetFilterSupportServiceTreeList());
                    PhApplyTreeNewFilters(PhGetFilterSupportNetworkTreeList());
                    PhInvokeCallback(&SearchChangedEvent, SearchboxText);

                    goto DefaultWndProc;
                }
                break;
            case EN_KILLFOCUS:
                {
                    if (SearchBoxDisplayMode != SearchBoxDisplayHideInactive)
                        break;

                    if ((HWND)lParam != SearchboxHandle)
                        break;

                    if (SearchboxText->Length == 0)
                    {
                        if (RebarBandExists(BandID_SearchBox))
                            RebarBandRemove(BandID_SearchBox);
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
                    if (EnableToolBar && EnableSearchBox)
                    {
                        SetFocus(SearchboxHandle);
                        Edit_SetSel(SearchboxHandle, 0, -1);
                    }

                    goto DefaultWndProc;
                }
                break;
            case ID_SEARCH_CLEAR:
                {
                    if (EnableToolBar && EnableSearchBox)
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
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lParam;

            if (hdr->hwndFrom == RebarHandle)
            {
                switch (hdr->code)
                {
                case RBN_HEIGHTCHANGE:
                    {
                        // Invoke the LayoutPaddingCallback.
                        PostMessage(PhMainWndHandle, WM_SIZE, 0, 0);
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
                            TBBUTTONINFO button = { sizeof(TBBUTTONINFO) };
                            button.dwMask = TBIF_BYINDEX | TBIF_STYLE | TBIF_COMMAND | TBIF_IMAGE;

                            // Get the client coordinates of the button.
                            if (SendMessage(ToolBarHandle, TB_GETITEMRECT, index, (LPARAM)&buttonRect) == -1)
                                break;

                            if (buttonRect.right <= toolbarRect.right)
                                continue;

                            // Get extended button information.
                            if (SendMessage(ToolBarHandle, TB_GETBUTTONINFO, index, (LPARAM)&button) == -1)
                                break;

                            if (button.fsStyle == BTNS_SEP)
                            {
                                // Add separators to menu.
                                PhInsertEMenuItem(menu, PhCreateEMenuItem(PH_EMENU_SEPARATOR, 0, NULL, NULL, NULL), -1);
                            }
                            else
                            {
                                PPH_EMENU_ITEM menuItem;
                                HICON menuIcon;

                                // Add buttons to menu.
                                menuItem = PhCreateEMenuItem(0, button.idCommand, ToolbarGetText(button.idCommand), NULL, NULL);

                                menuIcon = ImageList_GetIcon(ToolBarImageList, button.iImage, ILD_NORMAL);
                                menuItem->Flags |= PH_EMENU_BITMAP_OWNED;
                                menuItem->Bitmap = PhIconToBitmap(menuIcon, 16, 16);
                                DestroyIcon(menuIcon);

                                if (button.idCommand == PHAPP_ID_VIEW_ALWAYSONTOP)
                                {
                                    // Set the pressed state.
                                    if (PhGetIntegerSetting(L"MainWindowAlwaysOnTop"))
                                        menuItem->Flags |= PH_EMENU_CHECKED;
                                }

                                // TODO: Temporarily disable some unsupported buttons.
                                if (button.idCommand == TIDC_FINDWINDOW ||
                                    button.idCommand == TIDC_FINDWINDOWTHREAD ||
                                    button.idCommand == TIDC_FINDWINDOWKILL)
                                {
                                    menuItem->Flags |= PH_EMENU_DISABLED;
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
                            SendMessage(PhMainWndHandle, WM_COMMAND, MAKEWPARAM(selectedItem->Id, 0), 0);
                        }

                        PhDestroyEMenu(menu);
                    }
                    break;
                }

                goto DefaultWndProc;
            }
            else if (hdr->hwndFrom == ToolBarHandle)
            {
                switch (hdr->code)
                {
                case TBN_BEGINDRAG:
                    {
                        LPNMTOOLBAR toolbar = (LPNMTOOLBAR)hdr;
                        ULONG id;

                        id = (ULONG)toolbar->iItem;

                        if (id == TIDC_FINDWINDOW || id == TIDC_FINDWINDOWTHREAD || id == TIDC_FINDWINDOWKILL)
                        {
                            // Direct all mouse events to this window.
                            SetCapture(hWnd);

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
                case TBN_INITCUSTOMIZE:
                    {
                        struct
                        {
                            NMHDR hdr;
                            HWND hDlg;     // handle of the customization dialog.
                        } *initcustomize = (PVOID)lParam;
                    }
                    return TBNRF_HIDEHELP;
                case TBN_QUERYINSERT:
                case TBN_QUERYDELETE:
                    return TRUE;
                case TBN_GETBUTTONINFO:
                    {
                        LPTBNOTIFY tbNotify = (LPTBNOTIFY)lParam;

                        if (tbNotify->iItem < _countof(ToolbarButtons))
                        {
                            tbNotify->tbButton = ToolbarButtons[tbNotify->iItem];
                            return TRUE;
                        }
                    }
                    return FALSE;
                case TBN_ENDADJUST:
                    {
                        if (!ToolbarInitialized)
                            break;

                        // Save the customization settings.
                        SendMessage(ToolBarHandle, TB_SAVERESTORE, TRUE, (LPARAM)&ToolbarSaveParams);

                        LoadToolbarSettings();
                        InvalidateRect(ToolBarHandle, NULL, TRUE);
                    }
                    break;
                case TBN_RESET:
                    {
                        ResetToolbarSettings();

                        // Re-load the original button settings.
                        LoadToolbarSettings();

                        InvalidateRect(ToolBarHandle, NULL, TRUE);

                        // Save the new settings as defaults.
                        SendMessage(ToolBarHandle, TB_SAVERESTORE, TRUE, (LPARAM)&ToolbarSaveParams);
                    }
                    return TBNRF_ENDCUSTOMIZE;
                }

                goto DefaultWndProc;
            }
            else if (hdr->hwndFrom == StatusBarHandle)
            {
                switch (hdr->code)
                {
                case NM_RCLICK:
                    {
                        POINT cursorPos;

                        GetCursorPos(&cursorPos);

                        ShowStatusMenu(&cursorPos);
                    }
                    break;
                }

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
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
        {
            if (TargetingWindow)
            {
                ULONG processId;
                ULONG threadId;

                TargetingCompleted = TRUE;

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

                    if (PhGetIntegerSetting(SETTING_NAME_ENABLE_RESOLVEGHOSTWINDOWS))
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
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);

DefaultWndProc:
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static VOID NTAPI MainWindowShowingCallback(
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

    LoadToolbarSettings();
}

static VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    EnableToolBar = !!PhGetIntegerSetting(SETTING_NAME_ENABLE_TOOLBAR);
    EnableSearchBox = !!PhGetIntegerSetting(SETTING_NAME_ENABLE_SEARCHBOX);
    EnableStatusBar = !!PhGetIntegerSetting(SETTING_NAME_ENABLE_STATUSBAR);

    StatusMask = PhGetIntegerSetting(SETTING_NAME_ENABLE_STATUSMASK);
    DisplayStyle = (TOOLBAR_DISPLAY_STYLE)PhGetIntegerSetting(SETTING_NAME_TOOLBARDISPLAYSTYLE);
    SearchBoxDisplayMode = (SEARCHBOX_DISPLAY_MODE)PhGetIntegerSetting(SETTING_NAME_SEARCHBOXDISPLAYMODE);
}

static VOID NTAPI ShowOptionsCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    DialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS),
        (HWND)Parameter,
        OptionsDlgProc
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
                { IntegerSettingType, SETTING_NAME_ENABLE_TOOLBAR, L"1" },
                { IntegerSettingType, SETTING_NAME_ENABLE_SEARCHBOX, L"1" },
                { IntegerSettingType, SETTING_NAME_ENABLE_STATUSBAR, L"1" },
                { IntegerSettingType, SETTING_NAME_ENABLE_RESOLVEGHOSTWINDOWS, L"1" },
                { IntegerSettingType, SETTING_NAME_ENABLE_STATUSMASK, L"d" },
                { IntegerSettingType, SETTING_NAME_TOOLBARDISPLAYSTYLE, L"1" },
                { IntegerSettingType, SETTING_NAME_SEARCHBOXDISPLAYMODE, L"0" }
            };

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Toolbar and Status Bar";
            info->Author = L"dmex, wj32";
            info->Description = L"Adds a toolbar and a status bar.";
            info->Url = L"http://processhacker.sf.net/forums/viewtopic.php?t=1119";
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

            PhAddSettings(settings, _countof(settings));

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