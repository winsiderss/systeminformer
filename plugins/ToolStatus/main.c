/*
 * Process Hacker ToolStatus -
 *   main program
 *
 * Copyright (C) 2010-2012 wj32
 * Copyright (C) 2011-2012 dmex
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

static HWND ReBarHandle = NULL;
static HWND TextboxHandle = NULL;
static HWND ToolBarHandle = NULL;
static HFONT FontHandle = NULL;
static HIMAGELIST ToolBarImageList = NULL;
static HACCEL AcceleratorTable = NULL;

static RECT ReBarRect = { 0 };
static ULONG TargetingMode = 0;
static HWND TargetingCurrentWindow = NULL;
static BOOLEAN TargetingWindow = FALSE;
static BOOLEAN TargetingCurrentWindowDraw = FALSE;
static BOOLEAN TargetingCompleted = FALSE;

#define ID_SEARCH_CLEAR (WM_USER + 1)

static VOID RebarCreate(
    __in HWND ParentHandle
    )
{
    REBARINFO rebarInfo = { sizeof(REBARINFO) };

    // Create the rebar
    ReBarHandle = CreateWindowEx(
        WS_EX_TOOLWINDOW,
        REBARCLASSNAME,
        NULL,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CCS_NODIVIDER | CCS_TOP | RBS_DBLCLKTOGGLE | RBS_VARHEIGHT,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        ParentHandle,
        NULL,
        (HINSTANCE)PluginInstance->DllBase,
        NULL
        );

    // no imagelist to attach to rebar
    PostMessage(ReBarHandle, RB_SETBARINFO, 0, (LPARAM)&rebarInfo);
}

static VOID StatusBarCreate(
    __in HWND ParentHandle
    )
{
    StatusBarHandle = CreateWindowEx(
        0,
        STATUSCLASSNAME,
        NULL,
        WS_CHILD | CCS_BOTTOM | SBARS_SIZEGRIP | SBARS_TOOLTIPS,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        ParentHandle,
        NULL,
        (HINSTANCE)PluginInstance->DllBase,
        NULL
        );
}

static VOID ToolBarCreate(
    __in HWND ParentHandle
    )
{
    ToolBarHandle = CreateWindowEx(
        0,
        TOOLBARCLASSNAME,
        NULL,
        WS_CHILD | WS_VISIBLE | CCS_NORESIZE | CCS_NODIVIDER | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        ParentHandle,
        NULL,
        (HINSTANCE)PluginInstance->DllBase,
        NULL
        );

    // Set the toolbar struct size.
    SendMessage(ToolBarHandle, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    // Set the extended toolbar styles.
    SendMessage(ToolBarHandle, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_MIXEDBUTTONS);
}

static VOID ToolbarCreateSearch(
    __in HWND ParentHandle
    )
{
    TextboxHandle = CreateWindowEx(
        WS_EX_STATICEDGE,
        WC_EDIT,
        NULL,
        WS_CHILD | ES_LEFT,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        ParentHandle,
        NULL,
        (HINSTANCE)PluginInstance->DllBase,
        NULL
        );

    // Set Searchbox control font
    SendMessage(TextboxHandle, WM_SETFONT, (WPARAM)FontHandle, MAKELPARAM(TRUE, 0));

    // Set initial text
    Edit_SetCueBannerText(TextboxHandle, L"Search Processes (Ctrl+K)");

    // insert a paint region into the edit control NC window area       
    InsertButton(TextboxHandle, ID_SEARCH_CLEAR, 25);

    PhAddTreeNewFilter(PhGetFilterSupportProcessTreeList(), (PPH_TN_FILTER_FUNCTION)ProcessTreeFilterCallback, TextboxHandle);
    PhAddTreeNewFilter(PhGetFilterSupportServiceTreeList(), (PPH_TN_FILTER_FUNCTION)ServiceTreeFilterCallback, TextboxHandle);
    PhAddTreeNewFilter(PhGetFilterSupportNetworkTreeList(), (PPH_TN_FILTER_FUNCTION)NetworkTreeFilterCallback, TextboxHandle);  
}

static VOID ToolBarCreateImageList(
    __in HWND WindowHandle
    )
{
    // Create the toolbar imagelist
    ToolBarImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 0);
    // Set the number of images
    ImageList_SetImageCount(ToolBarImageList, 7);
    // Add the images to the imagelist - same index as the first tbButtonArray field
    PhSetImageListBitmap(ToolBarImageList, 0, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_ARROW_REFRESH));
    PhSetImageListBitmap(ToolBarImageList, 1, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_COG_EDIT));
    PhSetImageListBitmap(ToolBarImageList, 2, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_FIND));
    PhSetImageListBitmap(ToolBarImageList, 3, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_CHART_LINE));
    PhSetImageListBitmap(ToolBarImageList, 4, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_APPLICATION));
    PhSetImageListBitmap(ToolBarImageList, 5, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_APPLICATION_GO));
    PhSetImageListBitmap(ToolBarImageList, 6, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_CROSS));

    // Configure the toolbar imagelist
    PostMessage(WindowHandle, TB_SETIMAGELIST, 0, (LPARAM)ToolBarImageList); 
}

static VOID RebarAddMenuItem(
    __in HWND WindowHandle,
    __in HWND ChildHandle,
    __in UINT ID,
    __in UINT cyMinChild,
    __in UINT cxMinChild
    )
{
    REBARBANDINFO rebarBandInfo = { 0 }; 

    rebarBandInfo.cbSize = REBARBANDINFO_V6_SIZE;
    rebarBandInfo.fMask = RBBIM_STYLE | RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE;
    rebarBandInfo.fStyle = RBBS_HIDETITLE | RBBS_CHILDEDGE | RBBS_NOGRIPPER | RBBS_FIXEDSIZE;
    
    rebarBandInfo.wID = ID;
    rebarBandInfo.hwndChild = ChildHandle;
    rebarBandInfo.cyMinChild = cyMinChild;
    rebarBandInfo.cxMinChild = cxMinChild;

    SendMessage(WindowHandle, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rebarBandInfo);
}

static VOID ToolbarAddMenuItems(
    __in HWND WindowHandle
    )
{
    TBBUTTON tbButtonArray[] =
    {
        { 0, PHAPP_ID_VIEW_REFRESH, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR)L"Refresh" },
        { 1, PHAPP_ID_HACKER_OPTIONS, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR)L"Options" },
        { 0, 0, 0, BTNS_SEP, { 0 }, 0, 0 },
        { 2, PHAPP_ID_HACKER_FINDHANDLESORDLLS, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR)L"Find Handles or DLLs" },
        { 3, PHAPP_ID_VIEW_SYSTEMINFORMATION, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR)L"System Information" },
        { 0, 0, 0, BTNS_SEP, { 0 }, 0, 0 },
        { 4, TIDC_FINDWINDOW, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR)L"Find Window" },
        { 5, TIDC_FINDWINDOWTHREAD, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR)L"Find Window and Thread" },
        { 6, TIDC_FINDWINDOWKILL, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR)L"Find Window and Kill" }
    };

    // Add the buttons to the toolbar
    SendMessage(WindowHandle, TB_ADDBUTTONS, _countof(tbButtonArray), (LPARAM)tbButtonArray);
}

static VOID NTAPI ProcessesUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    ProcessesUpdatedCount++;

    if (EnableStatusBar)
        UpdateStatusBar();
}

static VOID NTAPI TabPageUpdatedCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    INT index = (INT)Parameter;

    if (TextboxHandle)
    {
        switch (index)
        {
        case 0:
            Edit_SetCueBannerText(TextboxHandle, L"Search Processes (Ctrl+K)");
            break;
        case 1:
            Edit_SetCueBannerText(TextboxHandle, L"Search Services (Ctrl+K)");
            break;
        case 2:
            Edit_SetCueBannerText(TextboxHandle, L"Search Network (Ctrl+K)");
            break;
        default:
            {
                // Disable the textbox if we're on an unsupported tab.
                Edit_SetCueBannerText(TextboxHandle, L"Search Disabled");
            }
            break;
        }
    }
}

static VOID NTAPI LayoutPaddingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_LAYOUT_PADDING_DATA data = (PPH_LAYOUT_PADDING_DATA)Parameter;

    if (EnableToolBar)
    {
        data->Padding.top += ReBarRect.bottom; // Width
    }

    if (EnableStatusBar)
    {
        data->Padding.bottom += StatusBarRect.bottom;
    }
}

static BOOLEAN NTAPI MessageLoopFilter(
    __in PMSG Message,
    __in PVOID Context
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

static VOID ApplyToolbarSettings(
    VOID
    )
{
    if (StatusBarHandle)
    {
        if (EnableStatusBar)
            ShowWindow(StatusBarHandle, SW_SHOW);          
        else
            ShowWindow(StatusBarHandle, SW_HIDE);
    }

    if (ToolBarHandle)
    {        
        if (ReBarHandle)
            ShowWindow(ReBarHandle, SW_SHOW);
        else
            ShowWindow(ReBarHandle, SW_HIDE);

        if (EnableToolBar)
            ShowWindow(ToolBarHandle, SW_SHOW);
        else
            ShowWindow(ToolBarHandle, SW_HIDE);
    }

    if (TextboxHandle)
    {
        if (EnableSearch)
            ShowWindow(TextboxHandle, SW_SHOW);
        else
        {
            ShowWindow(TextboxHandle, SW_HIDE);

            // Clear searchbox
            Edit_SetSel(TextboxHandle, 0, -1);    
            SetWindowText(TextboxHandle, L"");
        }
    }

    {
        ULONG i = 0;
        ULONG buttonCount = 0;

        buttonCount = SendMessage(ToolBarHandle, TB_BUTTONCOUNT, 0L, 0L);

        for (i = 0; i < buttonCount; i++)
        {
            TBBUTTONINFO button = { sizeof(TBBUTTONINFO) };
            button.dwMask = TBIF_BYINDEX | TBIF_STYLE | TBIF_COMMAND;

            // Get settings for first button
            SendMessage(ToolBarHandle, TB_GETBUTTONINFO, i, (LPARAM)&button);

            // Skip separator buttons
            if (button.fsStyle != BTNS_SEP)
            {
                switch (DisplayStyle)
                {
                case ImageOnly:
                    button.fsStyle = button.fsStyle | BTNS_AUTOSIZE;
                    break;
                case SelectiveText:
                    {
                        button.fsStyle = button.fsStyle | BTNS_AUTOSIZE;

                        switch (button.idCommand)
                        {
                        case PHAPP_ID_VIEW_REFRESH:
                        case PHAPP_ID_HACKER_OPTIONS: 
                        case PHAPP_ID_HACKER_FINDHANDLESORDLLS:
                        case PHAPP_ID_VIEW_SYSTEMINFORMATION:
                            button.fsStyle = BTNS_SHOWTEXT;
                            break;
                        }
                    }
                    break;
                default:
                    button.fsStyle = BTNS_SHOWTEXT;
                    break;
                }

                // Set updated button info
                SendMessage(ToolBarHandle, TB_SETBUTTONINFO, i, (LPARAM)&button);
            }
        }
    }
}

static VOID DrawWindowBorderForTargeting(
    __in HWND hWnd
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
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam,
    __in UINT_PTR uIdSubclass,
    __in DWORD_PTR dwRefData
    )
{
    switch (uMsg)
    {
    case WM_COMMAND:
        {
            ULONG id = (ULONG)(USHORT)LOWORD(wParam);
     
            switch (HIWORD(wParam))
            {
            case BN_CLICKED:
                {
                    if (id != ID_SEARCH_CLEAR)
                        break;

                    SetFocus(TextboxHandle);
                    Edit_SetSel(TextboxHandle, 0, -1);
                    SetWindowText(TextboxHandle, L"");

                    goto DefaultWndProc;
                }
                break;
            case EN_CHANGE:
                {
                    PhApplyTreeNewFilters(PhGetFilterSupportProcessTreeList());
                    PhApplyTreeNewFilters(PhGetFilterSupportServiceTreeList());
                    PhApplyTreeNewFilters(PhGetFilterSupportNetworkTreeList());
                    goto DefaultWndProc;
                }
            }

            if (EnableToolBar && id == ID_SEARCH)
            {
                SetFocus(TextboxHandle);
                Edit_SetSel(TextboxHandle, 0, -1);
            }

            // If we're targeting and the user presses the Esc key, cancel the targeting.
            // We also make sure the window doesn't get closed, by filtering out the message.
            if (id == PHAPP_ID_ESC_EXIT && TargetingWindow)
            {
                TargetingWindow = FALSE;
                ReleaseCapture();

                goto DefaultWndProc;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR hdr = (LPNMHDR)lParam;

            if (hdr->hwndFrom == ReBarHandle)
            {
                if (hdr->code == RBN_HEIGHTCHANGE)
                {
                    // HACK: Invoke LayoutPaddingCallback and adjust rebar for multiple rows.
                    PostMessage(PhMainWndHandle, WM_SIZE, 0L, 0L);
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

                    if (PhGetIntegerSetting(L"ProcessHacker.ToolStatus.ResolveGhostWindows"))
                    {
                        // This is an undocumented function exported by user32.dll that
                        // retrieves the hung window represented by a ghost window.
                        static HWND (WINAPI *HungWindowFromGhostWindow_I)(
                            __in HWND hWnd
                            );

                        if (!HungWindowFromGhostWindow_I)
                            HungWindowFromGhostWindow_I = PhGetProcAddress(L"user32.dll", "HungWindowFromGhostWindow");

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
                                    PhShowError(hWnd, L"The process (PID %u) does not exist.", processId);
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
                                    PhShowError(hWnd, L"The process (PID %u) does not exist.", processId);
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
        {
            if (EnableToolBar)
            {
                SendMessage(ReBarHandle, WM_SIZE, 0, 0);
                GetClientRect(ReBarHandle, &ReBarRect);
                
            }

            if (EnableStatusBar)
            {
                SendMessage(StatusBarHandle, WM_SIZE, 0, 0);
                GetClientRect(StatusBarHandle, &StatusBarRect);
                
            }

            ProcessHacker_InvalidateLayoutPadding(hWnd);
        }
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);

DefaultWndProc:
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static VOID NTAPI MainWindowShowingCallback(
     __in_opt PVOID Parameter,
     __in_opt PVOID Context
    )
{       
    if (EnableToolBar)
    {
        RebarCreate(PhMainWndHandle);

        ToolBarCreate(PhMainWndHandle);       
        ToolBarCreateImageList(ToolBarHandle);
        ToolbarAddMenuItems(ToolBarHandle);

        // inset the toolbar into the rebar control
        RebarAddMenuItem(ReBarHandle, ToolBarHandle, 0, 22, 0);

        if (EnableSearch)
        {
            ToolbarCreateSearch(ToolBarHandle);
            // inset the edit control into the rebar control
            RebarAddMenuItem(ReBarHandle, TextboxHandle, 0, 22, 200);
        }
    }

    if (EnableStatusBar)
    {
        StatusBarCreate(PhMainWndHandle);
    }

    ApplyToolbarSettings();

    PhRegisterMessageLoopFilter(MessageLoopFilter, NULL);
    PhRegisterCallback(
        ProcessHacker_GetCallbackLayoutPadding(PhMainWndHandle), 
        LayoutPaddingCallback, 
        NULL, 
        &LayoutPaddingCallbackRegistration
        );

    SetWindowSubclass(PhMainWndHandle, MainWndSubclassProc, 0, 0);
}

static VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    LOGFONT logFont;

    memset(&logFont, 0, sizeof(LOGFONT));

    logFont.lfHeight = 14;
    logFont.lfWeight = FW_NORMAL;

    wcscpy_s(
        logFont.lfFaceName, 
        _countof(logFont.lfFaceName), 
        L"MS Shell Dlg 2"
        );

    // Create the font handle
    FontHandle = CreateFontIndirect(&logFont);

    EnableToolBar = !!PhGetIntegerSetting(L"ProcessHacker.ToolStatus.EnableToolBar");
    EnableSearch = !!PhGetIntegerSetting(L"ProcessHacker.ToolStatus.EnableSearch"); 
    EnableStatusBar = !!PhGetIntegerSetting(L"ProcessHacker.ToolStatus.EnableStatusBar"); 

    StatusMask = PhGetIntegerSetting(L"ProcessHacker.ToolStatus.StatusMask");
    DisplayStyle = (TOOLBAR_DISPLAY_STYLE)PhGetIntegerSetting(L"ProcessHacker.ToolStatus.ToolbarDisplayStyle");     
}

static VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    DialogBox(
        (HINSTANCE)PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS),
        (HWND)Parameter,
        OptionsDlgProc
        );
}

LOGICAL DllMain(
    __in HINSTANCE Instance,
    __in ULONG Reason,
    __reserved PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PPH_PLUGIN_INFORMATION info;
            PH_SETTING_CREATE settings[] =
            {
                { IntegerSettingType, L"ProcessHacker.ToolStatus.EnableToolBar", L"1" },
                { IntegerSettingType, L"ProcessHacker.ToolStatus.EnableStatusBar", L"1" },
                { IntegerSettingType, L"ProcessHacker.ToolStatus.EnableSearch", L"1" },
                { IntegerSettingType, L"ProcessHacker.ToolStatus.ResolveGhostWindows", L"1" },
                { IntegerSettingType, L"ProcessHacker.ToolStatus.StatusMask", L"d" },
                { IntegerSettingType, L"ProcessHacker.ToolStatus.ToolbarDisplayStyle", L"1" }
            };

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.ToolStatus", Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Toolbar and Status Bar";
            info->Author = L"dmex & wj32";
            info->Description = L"Adds a toolbar and a status bar.";
            info->HasOptions = TRUE;

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
            
            PhAddSettings(settings, _countof(settings));

            AcceleratorTable = LoadAccelerators(
                Instance,
                MAKEINTRESOURCE(IDR_MAINWND_ACCEL)
                );
        }
        break;
    }

    return TRUE;
}