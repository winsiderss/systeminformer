/*
 * Process Hacker ToolStatus -
 *   main toolbar
 *
 * Copyright (C) 2013 dmex
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

static HIMAGELIST ToolBarImageList;
static TBBUTTON ButtonArray[9] =
{
    { 0, PHAPP_ID_VIEW_REFRESH, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0 },
    { 1, PHAPP_ID_HACKER_OPTIONS, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0 },
    { 0, 0, 0, BTNS_SEP, { 0 }, 0, 0 },
    { 2, PHAPP_ID_HACKER_FINDHANDLESORDLLS, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0 },
    { 3, PHAPP_ID_VIEW_SYSTEMINFORMATION, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0 },
    { 0, 0, 0, BTNS_SEP, { 0 }, 0, 0 },
    { 4, TIDC_FINDWINDOW, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0 },
    { 5, TIDC_FINDWINDOWTHREAD, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0 },
    { 6, TIDC_FINDWINDOWKILL, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0 }
};

static VOID RebarAddMenuItem(
    __in HWND WindowHandle,
    __in HWND HwndHandle,
    __in UINT BandID,
    __in UINT cyMinChild,
    __in UINT cxMinChild
    )
{
    REBARBANDINFO rebarBandInfo = { REBARBANDINFO_V6_SIZE };
    rebarBandInfo.fMask = RBBIM_STYLE | RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE;
    rebarBandInfo.fStyle = RBBS_NOGRIPPER | RBBS_FIXEDSIZE;

    rebarBandInfo.wID = BandID;
    rebarBandInfo.hwndChild = HwndHandle;
    rebarBandInfo.cyMinChild = cyMinChild;
    rebarBandInfo.cxMinChild = cxMinChild;

    SendMessage(WindowHandle, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rebarBandInfo);
}

static VOID RebarRemoveMenuItem(
    __in HWND WindowHandle,
    __in UINT ID
    )
{
    INT bandId = (INT)SendMessage(WindowHandle, RB_IDTOINDEX, (WPARAM)ID, 0);

    SendMessage(WindowHandle, RB_DELETEBAND, (WPARAM)bandId, 0);
}

static VOID RebarLoadSettings(
    VOID
    )
{
    if (!ToolBarImageList)
    {
        // Create the toolbar imagelist
        ToolBarImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 0);
        // Set the number of images
        ImageList_SetImageCount(ToolBarImageList, 7);
    }

    // Add the images to the imagelist
    if (EnableWicImaging)
    {
        ImageList_Replace(ToolBarImageList, 0, LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_ARROW_REFRESH)), NULL);
        ImageList_Replace(ToolBarImageList, 1, LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_COG_EDIT)), NULL);
        ImageList_Replace(ToolBarImageList, 2, LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_FIND)), NULL);
        ImageList_Replace(ToolBarImageList, 3, LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_CHART_LINE)), NULL);
        ImageList_Replace(ToolBarImageList, 4, LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_APPLICATION)), NULL);
        ImageList_Replace(ToolBarImageList, 5, LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_APPLICATION_GO)), NULL);
        ImageList_Replace(ToolBarImageList, 6, LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_CROSS)), NULL);
    }
    else
    {
        PhSetImageListBitmap(ToolBarImageList, 0, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_ARROW_REFRESH_BMP));
        PhSetImageListBitmap(ToolBarImageList, 1, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_COG_EDIT_BMP));
        PhSetImageListBitmap(ToolBarImageList, 2, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_FIND_BMP));
        PhSetImageListBitmap(ToolBarImageList, 3, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_CHART_LINE_BMP));
        PhSetImageListBitmap(ToolBarImageList, 4, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_APPLICATION_BMP));
        PhSetImageListBitmap(ToolBarImageList, 5, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_APPLICATION_GO_BMP));
        PhSetImageListBitmap(ToolBarImageList, 6, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_CROSS_BMP));
    }

    // Load the Rebar, Toolbar and Searchbox controls.
    if (EnableToolBar && !ReBarHandle)
    {
        REBARINFO rebarInfo = { sizeof(REBARINFO) };

        // Create the ReBar window.
        ReBarHandle = CreateWindowEx(
            WS_EX_TOOLWINDOW,
            REBARCLASSNAME,
            NULL,
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CCS_NODIVIDER | CCS_TOP | RBS_FIXEDORDER | RBS_VARHEIGHT, // | RBS_DBLCLKTOGGLE 
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            PhMainWndHandle,
            (HMENU)IDC_MENU_REBAR,
            (HINSTANCE)PluginInstance->DllBase,
            NULL
            );

        // Create the ToolBar window.
        ToolBarHandle = CreateWindowEx(
            0,
            TOOLBARCLASSNAME,
            NULL,
            WS_CHILD | WS_VISIBLE | CCS_NORESIZE | CCS_NODIVIDER | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            ReBarHandle,
            (HMENU)IDC_MENU_REBAR_TOOLBAR,
            (HINSTANCE)PluginInstance->DllBase,
            NULL
            );

        // Create the SearchBox window.
        TextboxHandle = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            WC_EDIT,
            NULL,
            WS_CHILD | WS_VISIBLE | ES_LEFT,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            ReBarHandle,
            (HMENU)IDC_MENU_REBAR_SEARCH,
            (HINSTANCE)PluginInstance->DllBase,
            NULL
            );

        // Set the toolbar info with no imagelist.
        SendMessage(ReBarHandle, RB_SETBARINFO, 0, (LPARAM)&rebarInfo);

        // Set the toolbar struct size.
        SendMessage(ToolBarHandle, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        // Set the toolbar extended toolbar styles.
        SendMessage(ToolBarHandle, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_HIDECLIPPEDBUTTONS);
        // Configure the toolbar imagelist.
        SendMessage(ToolBarHandle, TB_SETIMAGELIST, 0, (LPARAM)ToolBarImageList);
        // Add the buttons to the toolbar.
        SendMessage(ToolBarHandle, TB_ADDBUTTONS, _countof(ButtonArray), (LPARAM)ButtonArray);

        // Insert a paint region into the edit control NC window area
        InsertButton(TextboxHandle, ID_SEARCH_CLEAR);
       // Reset the client area margins.
        SendMessage(TextboxHandle, EM_SETMARGINS, EC_LEFTMARGIN, MAKELONG(0, 0));
        // Set initial text
        SendMessage(TextboxHandle, EM_SETCUEBANNER, 0, (LPARAM)L"Search Processes (Ctrl+K)");

        // Enable theming:
        //SendMessage(ReBarHandle, RB_SETWINDOWTHEME, 0, (LPARAM)L"Communications"); //Media/Communications/BrowserTabBar/Help   
        //SendMessage(ToolBarHandle, TB_SETWINDOWTHEME, 0, (LPARAM)L"Communications"); //Media/Communications/BrowserTabBar/Help

        // Inset the toolbar into the rebar control
        RebarAddMenuItem(ReBarHandle, ToolBarHandle, IDC_MENU_REBAR_TOOLBAR, 23, 0); // Toolbar width 400
        // Insert the edit control into the rebar control
        RebarAddMenuItem(ReBarHandle, TextboxHandle, IDC_MENU_REBAR_SEARCH, 20, 180);

        ProcessTreeFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportProcessTreeList(), (PPH_TN_FILTER_FUNCTION)ProcessTreeFilterCallback, NULL);
        ServiceTreeFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportServiceTreeList(), (PPH_TN_FILTER_FUNCTION)ServiceTreeFilterCallback, NULL);
        NetworkTreeFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportNetworkTreeList(), (PPH_TN_FILTER_FUNCTION)NetworkTreeFilterCallback, NULL);
    }

    // Load the Statusbar control.
    if (EnableStatusBar && !StatusBarHandle)
    {
        // Create the StatusBar window.
        StatusBarHandle = CreateWindowEx(
            0,
            STATUSCLASSNAME,
            NULL,
            WS_CHILD | WS_VISIBLE | CCS_BOTTOM | SBARS_SIZEGRIP | SBARS_TOOLTIPS,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            PhMainWndHandle,
            NULL,
            (HINSTANCE)PluginInstance->DllBase,
            NULL
            );
    }

    // Hide or show controls (Note: don't unload or remove at runtime).
    if (EnableToolBar)
    {
        if (ReBarHandle && !IsWindowVisible(ReBarHandle))
            ShowWindow(ReBarHandle, SW_SHOW);
    }
    else
    {
        if (ReBarHandle && IsWindowVisible(ReBarHandle))
            ShowWindow(ReBarHandle, SW_HIDE);
    }

    if (EnableStatusBar)
    {  
        if (StatusBarHandle && !IsWindowVisible(StatusBarHandle))
            ShowWindow(StatusBarHandle, SW_SHOW);
    }
    else
    {        
        if (StatusBarHandle && IsWindowVisible(StatusBarHandle))
            ShowWindow(StatusBarHandle, SW_HIDE);
    }
}

VOID LoadToolbarSettings(
    VOID
    )
{
    RebarLoadSettings();

    if (EnableToolBar && ToolBarHandle)
    {
        ULONG index = 0;
        ULONG buttonCount = 0;

        buttonCount = (ULONG)SendMessage(ToolBarHandle, TB_BUTTONCOUNT, 0, 0);

        for (index = 0; index < buttonCount; index++)
        {
            TBBUTTONINFO button = { sizeof(TBBUTTONINFO) };
            button.dwMask = TBIF_BYINDEX | TBIF_STYLE | TBIF_COMMAND | TBIF_TEXT;

            // Get settings for first button
            SendMessage(ToolBarHandle, TB_GETBUTTONINFO, index, (LPARAM)&button);

            // Skip separator buttons
            if (button.fsStyle == BTNS_SEP)
                continue;

            switch (button.idCommand)
            {
            case PHAPP_ID_VIEW_REFRESH:
                button.pszText = L"Refresh";
                break;
            case PHAPP_ID_HACKER_OPTIONS:
                button.pszText = L"Options";
                break;
            case PHAPP_ID_HACKER_FINDHANDLESORDLLS:
                button.pszText = L"Find Handles or DLLs";
                break;
            case PHAPP_ID_VIEW_SYSTEMINFORMATION:
                button.pszText = L"System Information";
                break;
            case TIDC_FINDWINDOW:
                button.pszText = L"Find Window";
                break;
            case TIDC_FINDWINDOWTHREAD:
                button.pszText = L"Find Window and Thread";
                break;
            case TIDC_FINDWINDOWKILL:
                button.pszText = L"Find Window and Kill";
                break;
            }

            switch (DisplayStyle)
            {
            case ImageOnly:
                button.fsStyle = BTNS_AUTOSIZE;
                break;
            case SelectiveText:
                {
                    switch (button.idCommand)
                    {
                    case PHAPP_ID_VIEW_REFRESH:
                    case PHAPP_ID_HACKER_OPTIONS:
                    case PHAPP_ID_HACKER_FINDHANDLESORDLLS:
                    case PHAPP_ID_VIEW_SYSTEMINFORMATION:
                        button.fsStyle = BTNS_SHOWTEXT;
                        break;
                    default:
                        button.fsStyle = BTNS_AUTOSIZE;
                        break;
                    }
                }
                break;
            default:
                button.fsStyle = BTNS_SHOWTEXT;
                break;
            }

            // Set updated button info
            SendMessage(ToolBarHandle, TB_SETBUTTONINFO, index, (LPARAM)&button);
        }
       
        // Resize the toolbar
        SendMessage(ToolBarHandle, TB_AUTOSIZE, 0, 0);
    }

    // Invoke the LayoutPaddingCallback.
    SendMessage(PhMainWndHandle, WM_SIZE, 0, 0);
}