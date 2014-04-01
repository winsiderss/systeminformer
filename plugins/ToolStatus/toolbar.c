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
    _In_ HWND WindowHandle,
    _In_ HWND HwndHandle,
    _In_ UINT cyMinChild,
    _In_ UINT cxMinChild
    )
{
    static UINT bandID = 0;

    REBARBANDINFO rebarBandInfo = { REBARBANDINFO_V6_SIZE };
    rebarBandInfo.fMask = RBBIM_STYLE | RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE;
    rebarBandInfo.fStyle = RBBS_NOGRIPPER | RBBS_FIXEDSIZE;

    rebarBandInfo.wID = bandID++;
    rebarBandInfo.hwndChild = HwndHandle;
    rebarBandInfo.cyMinChild = cyMinChild;
    rebarBandInfo.cxMinChild = cxMinChild;

    SendMessage(WindowHandle, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rebarBandInfo);
}

static VOID RebarRemoveMenuItem(
    _In_ HWND WindowHandle,
    _In_ UINT ID
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
        HBITMAP arrowIconBitmap = NULL;
        HBITMAP cogIconBitmap = NULL;
        HBITMAP findIconBitmap = NULL;
        HBITMAP chartIconBitmap = NULL;
        HBITMAP appIconBitmap = NULL;
        HBITMAP goIconBitmap = NULL;
        HBITMAP crossIconBitmap = NULL;

        // Create the toolbar imagelist
        ToolBarImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 0);
        // Set the number of images
        ImageList_SetImageCount(ToolBarImageList, 7);

        // Add the images to the imagelist
        if (arrowIconBitmap = LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_ARROW_REFRESH)))
        {
            ImageList_Replace(ToolBarImageList, 0, arrowIconBitmap, NULL);
            DeleteObject(arrowIconBitmap);
        }
        else
        {
            PhSetImageListBitmap(ToolBarImageList, 0, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_ARROW_REFRESH_BMP));
        }

        if (cogIconBitmap = LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_COG_EDIT)))  
        {
            ImageList_Replace(ToolBarImageList, 1, cogIconBitmap, NULL);
            DeleteObject(cogIconBitmap);
        }
        else
        {
            PhSetImageListBitmap(ToolBarImageList, 1, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_COG_EDIT_BMP));  
        }

        if (findIconBitmap = LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_FIND)))
        {
            ImageList_Replace(ToolBarImageList, 2, findIconBitmap, NULL);
            DeleteObject(findIconBitmap);
        }
        else
        {
            PhSetImageListBitmap(ToolBarImageList, 2, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_FIND_BMP));
        }

        if (chartIconBitmap = LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_CHART_LINE)))
        {
            ImageList_Replace(ToolBarImageList, 3, chartIconBitmap, NULL);
            DeleteObject(chartIconBitmap);
        }
        else
        {
            PhSetImageListBitmap(ToolBarImageList, 3, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_CHART_LINE_BMP));
        }

        if (appIconBitmap = LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_APPLICATION)))
        {
            ImageList_Replace(ToolBarImageList, 4, appIconBitmap, NULL);
            DeleteObject(appIconBitmap);
        }
        else
        {
            PhSetImageListBitmap(ToolBarImageList, 4, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_APPLICATION_BMP));
        }

        if (goIconBitmap = LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_APPLICATION_GO)))
        {
            ImageList_Replace(ToolBarImageList, 5, goIconBitmap, NULL);
            DeleteObject(goIconBitmap);
        }
        else
        {
            PhSetImageListBitmap(ToolBarImageList, 5, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_APPLICATION_GO_BMP));
        }

        if (crossIconBitmap = LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_CROSS)))
        {
            ImageList_Replace(ToolBarImageList, 6, crossIconBitmap, NULL);
            DeleteObject(crossIconBitmap);
        }
        else
        {
            PhSetImageListBitmap(ToolBarImageList, 6, (HINSTANCE)PluginInstance->DllBase, MAKEINTRESOURCE(IDB_CROSS_BMP));
        }
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
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CCS_NODIVIDER | CCS_TOP | RBS_VARHEIGHT, //RBS_FIXEDORDER | RBS_DBLCLKTOGGLE 
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            PhMainWndHandle,
            NULL,
            (HINSTANCE)PluginInstance->DllBase,
            NULL
            );

        // Create the ToolBar window.
        ToolBarHandle = CreateWindowEx(
            0,
            TOOLBARCLASSNAME,
            NULL,
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CCS_NORESIZE | CCS_NODIVIDER | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            ReBarHandle,
            NULL,
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
        // Enable theming:
        //SendMessage(ReBarHandle, RB_SETWINDOWTHEME, 0, (LPARAM)L"Communications"); //Media/Communications/BrowserTabBar/Help   
        //SendMessage(ToolBarHandle, TB_SETWINDOWTHEME, 0, (LPARAM)L"Communications"); //Media/Communications/BrowserTabBar/Help

        // HACK: Query the toolbar width/height.
        ULONG_PTR toolbarButtonSize = SendMessage(ToolBarHandle, TB_GETBUTTONSIZE, 0, 0);
        
        // Inset the toolbar into the rebar control.
        RebarAddMenuItem(
            ReBarHandle, 
            ToolBarHandle, 
            HIWORD(toolbarButtonSize), 
            LOWORD(toolbarButtonSize)
            );

        if (EnableSearchBox && !TextboxHandle)
        {    
            ProcessTreeFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportProcessTreeList(), (PPH_TN_FILTER_FUNCTION)ProcessTreeFilterCallback, NULL);
            ServiceTreeFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportServiceTreeList(), (PPH_TN_FILTER_FUNCTION)ServiceTreeFilterCallback, NULL);
            NetworkTreeFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportNetworkTreeList(), (PPH_TN_FILTER_FUNCTION)NetworkTreeFilterCallback, NULL);

            // Create the SearchBox window.
            TextboxHandle = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                WC_EDIT,
                NULL,
                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | ES_LEFT,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                ReBarHandle,
                NULL,
                (HINSTANCE)PluginInstance->DllBase,
                NULL
                );

            SearchboxText = PhReferenceEmptyString();

            // Insert a paint region into the edit control NC window area
            InsertButton(TextboxHandle, ID_SEARCH_CLEAR);            
   
            // Set  font
            SendMessage(TextboxHandle, WM_SETFONT, (WPARAM)PhApplicationFont, FALSE);
            // Reset the client area margins.
            SendMessage(TextboxHandle, EM_SETMARGINS, EC_LEFTMARGIN, MAKELPARAM(0, 0));
            // Set initial text
            Edit_SetCueBannerText(TextboxHandle, L"Search Processes (Ctrl+K)");

            // Insert the edit control into the rebar control
            RebarAddMenuItem(ReBarHandle, TextboxHandle, 20, 180);
        }
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

    if (EnableSearchBox)
    {
        if (TextboxHandle && !IsWindowVisible(TextboxHandle))
            ShowWindow(TextboxHandle, SW_SHOW);
    }
    else
    {
        if (TextboxHandle)
        {
            // Clear search text and reset search filters.
            SetFocus(TextboxHandle);
            Static_SetText(TextboxHandle, L"");

            if (IsWindowVisible(TextboxHandle))
                ShowWindow(TextboxHandle, SW_HIDE);
        }
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
            if (SendMessage(ToolBarHandle, TB_GETBUTTONINFO, index, (LPARAM)&button) == -1)
                break;

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
            case ToolbarDisplayImageOnly:
                button.fsStyle = BTNS_AUTOSIZE;
                break;
            case ToolbarDisplaySelectiveText:
                {
                    switch (button.idCommand)
                    {
                    case PHAPP_ID_VIEW_REFRESH:
                    case PHAPP_ID_HACKER_OPTIONS:
                    case PHAPP_ID_HACKER_FINDHANDLESORDLLS:
                    case PHAPP_ID_VIEW_SYSTEMINFORMATION:
                        button.fsStyle = BTNS_AUTOSIZE | BTNS_SHOWTEXT;
                        break;
                    default:
                        button.fsStyle = BTNS_AUTOSIZE;
                        break;
                    }
                }
                break;
            default:
                button.fsStyle = BTNS_AUTOSIZE | BTNS_SHOWTEXT;
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