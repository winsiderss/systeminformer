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

static HFONT InitializeFont(
    __in HWND hwndDlg
    )
{
    LOGFONT logFont = { 0 };
    HFONT fontHandle = NULL;

    logFont.lfHeight = 14;
    logFont.lfWeight = FW_NORMAL;
    logFont.lfQuality = CLEARTYPE_QUALITY | ANTIALIASED_QUALITY;
    
    // GDI uses the first font that matches the above attributes.
    fontHandle = CreateFontIndirect(&logFont);

    if (fontHandle)
    {
        SendMessage(hwndDlg, WM_SETFONT, (WPARAM)fontHandle, FALSE);
        return fontHandle;
    }

    return NULL;
}

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

static VOID SetRebarMenuLayout(
    VOID
    )
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
                button.fsStyle = BTNS_AUTOSIZE;

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
        SendMessage(ToolBarHandle, TB_SETBUTTONINFO, index, (LPARAM)&button);
    }

    // Resize the toolbar
    SendMessage(ToolBarHandle, TB_AUTOSIZE, 0, 0);
}

VOID ApplyToolbarSettings(
    VOID
    )
{
    if (EnableToolBar)
    {
        if (!ReBarHandle)
        {
            REBARINFO rebarInfo = { sizeof(REBARINFO) };

            // Create the rebar
            ReBarHandle = CreateWindowEx(
                WS_EX_TOOLWINDOW,
                REBARCLASSNAME,
                NULL,
                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CCS_NODIVIDER | CCS_TOP | RBS_DBLCLKTOGGLE | RBS_VARHEIGHT | RBS_FIXEDORDER,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                PhMainWndHandle,
                (HMENU)IDC_MENU_REBAR,
                (HINSTANCE)PluginInstance->DllBase,
                NULL
                );

            // no imagelist to attach to rebar
            SendMessage(ReBarHandle, RB_SETBARINFO, 0, (LPARAM)&rebarInfo);
            //SendMessage(ReBarHandle, RB_SETWINDOWTHEME, 0, (LPARAM)L"Communications"); //Media/Communications/BrowserTabBar/Help

            PhAddLayoutItem(&LayoutManager, ReBarHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
        }

        if (!ToolBarHandle)
        {
            // Create the toolbar window
            ToolBarHandle = CreateWindowEx(
                0,
                TOOLBARCLASSNAME,
                NULL,
                WS_CHILD | WS_VISIBLE | CCS_NORESIZE | CCS_NODIVIDER | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                PhMainWndHandle,
                (HMENU)IDC_MENU_REBAR_TOOLBAR,
                (HINSTANCE)PluginInstance->DllBase,
                NULL
                );

            // Set the toolbar struct size
            SendMessage(ToolBarHandle, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
            // Set the extended toolbar styles
            SendMessage(ToolBarHandle, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_HIDECLIPPEDBUTTONS);
            // Set the window theme
            //SendMessage(ToolBarHandle, TB_SETWINDOWTHEME, 0, (LPARAM)L"Communications"); //Media/Communications/BrowserTabBar/Help

            // Create the toolbar imagelist
            ToolBarImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 0);
            // Set the number of images
            ImageList_SetImageCount(ToolBarImageList, 7);
            // Add the images to the imagelist - same index as the first tbButtonArray field
            ImageList_Replace(ToolBarImageList, 0, LoadScaledImageFromResources(16, 16, MAKEINTRESOURCE(IDB_ARROW_REFRESH), L"PNG"), NULL);
            ImageList_Replace(ToolBarImageList, 1, LoadScaledImageFromResources(16, 16, MAKEINTRESOURCE(IDB_COG_EDIT), L"PNG"), NULL);
            ImageList_Replace(ToolBarImageList, 2, LoadScaledImageFromResources(16, 16, MAKEINTRESOURCE(IDB_FIND), L"PNG"), NULL);
            ImageList_Replace(ToolBarImageList, 3, LoadScaledImageFromResources(16, 16, MAKEINTRESOURCE(IDB_CHART_LINE), L"PNG"), NULL);
            ImageList_Replace(ToolBarImageList, 4, LoadScaledImageFromResources(16, 16, MAKEINTRESOURCE(IDB_APPLICATION), L"PNG"), NULL);
            ImageList_Replace(ToolBarImageList, 5, LoadScaledImageFromResources(16, 16, MAKEINTRESOURCE(IDB_APPLICATION_GO), L"PNG"), NULL);
            ImageList_Replace(ToolBarImageList, 6, LoadScaledImageFromResources(16, 16, MAKEINTRESOURCE(IDB_CROSS), L"PNG"), NULL);
            // Configure the toolbar imagelist
            SendMessage(ToolBarHandle, TB_SETIMAGELIST, 0, (LPARAM)ToolBarImageList);
            // Add the buttons to the toolbar
            SendMessage(ToolBarHandle, TB_ADDBUTTONS, _countof(ButtonArray), (LPARAM)ButtonArray);

            // inset the toolbar into the rebar control
            RebarAddMenuItem(ReBarHandle, ToolBarHandle, IDC_MENU_REBAR_TOOLBAR, 23, 0); // Toolbar width 400
        }

        SetRebarMenuLayout();
    }
    else
    {
        // temp HACK
        EnableSearch = FALSE;

        if (ToolBarHandle)
        {
            DestroyWindow(ToolBarHandle);
            ToolBarHandle = NULL;

            RebarRemoveMenuItem(ReBarHandle, IDC_MENU_REBAR_TOOLBAR);
        }

        if (ToolBarImageList)
        {
            ImageList_Destroy(ToolBarImageList);
            ToolBarImageList = NULL;
        }

        if (ReBarHandle)
        {
            DestroyWindow(ReBarHandle);
            ReBarHandle = NULL;
        }
    }

    if (EnableSearch)
    {
        if (!TextboxHandle)
        {
            TextboxHandle = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                WC_EDIT,
                NULL,
                WS_CHILD | WS_VISIBLE | ES_LEFT, // WS_BORDER
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                ToolBarHandle,
                NULL,
                (HINSTANCE)PluginInstance->DllBase,
                NULL
                );

            // Set Searchbox control font
            TextboxFontHandle = InitializeFont(TextboxHandle);
            // Set initial text
            SendMessage(TextboxHandle, EM_SETCUEBANNER, 0, (LPARAM)L"Search Processes (Ctrl+K)");
            // Reset the client area margins.
            SendMessage(TextboxHandle, EM_SETMARGINS, EC_LEFTMARGIN, MAKELONG(0, 0));

            // insert a paint region into the edit control NC window area
            InsertButton(TextboxHandle, ID_SEARCH_CLEAR);

            // insert the edit control into the rebar control
            RebarAddMenuItem(ReBarHandle, TextboxHandle, IDC_MENU_REBAR_SEARCH, 20, 180);

            ProcessTreeFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportProcessTreeList(), (PPH_TN_FILTER_FUNCTION)ProcessTreeFilterCallback, TextboxHandle);
            ServiceTreeFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportServiceTreeList(), (PPH_TN_FILTER_FUNCTION)ServiceTreeFilterCallback, TextboxHandle);
            NetworkTreeFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportNetworkTreeList(), (PPH_TN_FILTER_FUNCTION)NetworkTreeFilterCallback, TextboxHandle);
        }
    }
    else
    {
        if (NetworkTreeFilterEntry)
        {
            PhRemoveTreeNewFilter(PhGetFilterSupportProcessTreeList(), NetworkTreeFilterEntry);
            NetworkTreeFilterEntry = NULL;
        }

        if (ServiceTreeFilterEntry)
        {
            PhRemoveTreeNewFilter(PhGetFilterSupportProcessTreeList(), ServiceTreeFilterEntry);
            ServiceTreeFilterEntry = NULL;
        }

        if (ProcessTreeFilterEntry)
        {
            PhRemoveTreeNewFilter(PhGetFilterSupportProcessTreeList(), ProcessTreeFilterEntry);
            ProcessTreeFilterEntry = NULL;
        }

        if (TextboxHandle)
        {
            // Clear searchbox - ensures treenew filters are inactive when the user disables the toolbar
            Edit_SetSel(TextboxHandle, 0, -1);
            Static_SetText(TextboxHandle, L"");

            DestroyWindow(TextboxHandle);
            TextboxHandle = NULL;

            RebarRemoveMenuItem(ReBarHandle, IDC_MENU_REBAR_SEARCH);
        }

        if (TextboxFontHandle)
        {
            DeleteObject(TextboxFontHandle);
            TextboxFontHandle = NULL;
        }
    }

    if (EnableStatusBar)
    {
        if (!StatusBarHandle)
        {
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

            PhAddLayoutItem(&LayoutManager, StatusBarHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
        }
    }
    else
    {
        if (StatusBarHandle)
        {
            DestroyWindow(StatusBarHandle);
            StatusBarHandle = NULL;
        }
    }
}
