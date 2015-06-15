/*
 * Process Hacker ToolStatus -
 *   main toolbar
 *
 * Copyright (C) 2011-2015 dmex
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

BOOLEAN ToolbarInitialized = FALSE;
HIMAGELIST ToolBarImageList = NULL;

TBBUTTON ToolbarButtons[] =
{
    // Default toolbar buttons (displayed)
    { 0, PHAPP_ID_VIEW_REFRESH, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, { 0 }, 0, 0 },
    { 1, PHAPP_ID_HACKER_OPTIONS, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, { 0 }, 0, 0 },
    { 0, 0, 0, BTNS_SEP, { 0 }, 0, 0 },
    { 2, PHAPP_ID_HACKER_FINDHANDLESORDLLS, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, { 0 }, 0, 0 },
    { 3, PHAPP_ID_VIEW_SYSTEMINFORMATION, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, { 0 }, 0, 0 },
    { 0, 0, 0, BTNS_SEP, { 0 }, 0, 0 },
    { 4, TIDC_FINDWINDOW, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, { 0 }, 0, 0 },
    { 5, TIDC_FINDWINDOWTHREAD, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, { 0 }, 0, 0 },
    { 6, TIDC_FINDWINDOWKILL, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, { 0 }, 0, 0 },
    // Available toolbar buttons (hidden)
    { 7, PHAPP_ID_VIEW_ALWAYSONTOP, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, { 0 }, 0, 0 }
};

// NOTE: This Registry key is never created or used unless the Toolbar is customized.
TBSAVEPARAMSW ToolbarSaveParams =
{
    HKEY_CURRENT_USER,
    L"Software\\ProcessHacker",
    L"ToolbarSettings"
};

VOID RebarBandInsert(
    _In_ UINT BandID,
    _In_ HWND HwndChild,
    _In_ UINT cyMinChild,
    _In_ UINT cxMinChild
    )
{
    REBARBANDINFO rebarBandInfo = { REBARBANDINFO_V6_SIZE };
    rebarBandInfo.fMask = RBBIM_STYLE | RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE;
    rebarBandInfo.fStyle = RBBS_NOGRIPPER | RBBS_USECHEVRON; // | RBBS_HIDETITLE | RBBS_TOPALIGN;

    rebarBandInfo.wID = BandID;
    rebarBandInfo.hwndChild = HwndChild;
    rebarBandInfo.cyMinChild = cyMinChild;
    rebarBandInfo.cxMinChild = cxMinChild;

    if (BandID == BandID_SearchBox)
    {
        rebarBandInfo.fStyle |= RBBS_FIXEDSIZE;
    }

    SendMessage(RebarHandle, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rebarBandInfo);
}

VOID RebarBandRemove(
    _In_ UINT BandID
    )
{
    INT index = (INT)SendMessage(RebarHandle, RB_IDTOINDEX, (WPARAM)BandID, 0);

    if (index == -1)
        return;

    SendMessage(RebarHandle, RB_DELETEBAND, (WPARAM)index, 0);
}

BOOLEAN RebarBandExists(
    _In_ UINT BandID
    )
{
    INT index = (INT)SendMessage(RebarHandle, RB_IDTOINDEX, (WPARAM)BandID, 0);

    if (index != -1)
        return TRUE;

    return FALSE;
}

static VOID RebarLoadSettings(
    VOID
    )
{
    // Initialize the Toolbar Imagelist.
    if (EnableToolBar && !ToolBarImageList)
    {
        HBITMAP iconBitmap = NULL;

        // Create the toolbar imagelist
        ToolBarImageList = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK, 0, 0);
        // Set the number of images
        ImageList_SetImageCount(ToolBarImageList, 8);

        // Add the images to the imagelist
        if (iconBitmap = LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_ARROW_REFRESH)))
        {
            ImageList_Replace(ToolBarImageList, 0, iconBitmap, NULL);
            DeleteObject(iconBitmap);
        }
        else
        {
            PhSetImageListBitmap(ToolBarImageList, 0, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_ARROW_REFRESH_BMP));
        }

        if (iconBitmap = LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_COG_EDIT)))
        {
            ImageList_Replace(ToolBarImageList, 1, iconBitmap, NULL);
            DeleteObject(iconBitmap);
        }
        else
        {
            PhSetImageListBitmap(ToolBarImageList, 1, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_COG_EDIT_BMP));
        }

        if (iconBitmap = LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_FIND)))
        {
            ImageList_Replace(ToolBarImageList, 2, iconBitmap, NULL);
            DeleteObject(iconBitmap);
        }
        else
        {
            PhSetImageListBitmap(ToolBarImageList, 2, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_FIND_BMP));
        }

        if (iconBitmap = LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_CHART_LINE)))
        {
            ImageList_Replace(ToolBarImageList, 3, iconBitmap, NULL);
            DeleteObject(iconBitmap);
        }
        else
        {
            PhSetImageListBitmap(ToolBarImageList, 3, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_CHART_LINE_BMP));
        }

        if (iconBitmap = LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_APPLICATION)))
        {
            ImageList_Replace(ToolBarImageList, 4, iconBitmap, NULL);
            DeleteObject(iconBitmap);
        }
        else
        {
            PhSetImageListBitmap(ToolBarImageList, 4, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_APPLICATION_BMP));
        }

        if (iconBitmap = LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_APPLICATION_GO)))
        {
            ImageList_Replace(ToolBarImageList, 5, iconBitmap, NULL);
            DeleteObject(iconBitmap);
        }
        else
        {
            PhSetImageListBitmap(ToolBarImageList, 5, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_APPLICATION_GO_BMP));
        }

        if (iconBitmap = LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_CROSS)))
        {
            ImageList_Replace(ToolBarImageList, 6, iconBitmap, NULL);
            DeleteObject(iconBitmap);
        }
        else
        {
            PhSetImageListBitmap(ToolBarImageList, 6, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_CROSS_BMP));
        }

        if (iconBitmap = LoadImageFromResources(16, 16, MAKEINTRESOURCE(IDB_APPLICATION_GET)))
        {
            ImageList_Replace(ToolBarImageList, 7, iconBitmap, NULL);
            DeleteObject(iconBitmap);
        }
        else
        {
            PhSetImageListBitmap(ToolBarImageList, 7, PluginInstance->DllBase, MAKEINTRESOURCE(IDB_APPLICATION_GET_BMP));
        }
    }

    // Initialize the Rebar and Toolbar controls.
    if (EnableToolBar && !RebarHandle)
    {
        REBARINFO rebarInfo = { sizeof(REBARINFO) };

        // Create the ReBar window.
        RebarHandle = CreateWindowEx(
            WS_EX_TOOLWINDOW,
            REBARCLASSNAME,
            NULL,
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CCS_NODIVIDER | CCS_TOP | RBS_VARHEIGHT | RBS_AUTOSIZE, // CCS_NOPARENTALIGN | RBS_FIXEDORDER
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            PhMainWndHandle,
            NULL,
            NULL,
            NULL
            );

        // Set the toolbar info with no imagelist.
        SendMessage(RebarHandle, RB_SETBARINFO, 0, (LPARAM)&rebarInfo);

        // Create the ToolBar window.
        ToolBarHandle = CreateWindowEx(
            0,
            TOOLBARCLASSNAME,
            NULL,
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CCS_NORESIZE | CCS_NOPARENTALIGN | CCS_NODIVIDER | CCS_ADJUSTABLE | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS | TBSTYLE_AUTOSIZE, // TBSTYLE_ALTDRAG
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            RebarHandle,
            NULL,
            NULL,
            NULL
            );

        // Manually add button strings via TB_ADDSTRING.
        // NOTE: The Toolbar will sometimes decide to free strings hard-coded via (INT_PTR)L"String"
        //       in the ToolbarButtons array causing random crashes unless we manually add the strings
        //       into the Toolbar string pool (this bug only affects 64bit Windows)... WTF???
        ToolbarButtons[0].iString = SendMessage(ToolBarHandle, TB_ADDSTRING, 0, (LPARAM)L"Refresh");
        ToolbarButtons[1].iString = SendMessage(ToolBarHandle, TB_ADDSTRING, 0, (LPARAM)L"Options");
        ToolbarButtons[3].iString = SendMessage(ToolBarHandle, TB_ADDSTRING, 0, (LPARAM)L"Find Handles or DLLs");
        ToolbarButtons[4].iString = SendMessage(ToolBarHandle, TB_ADDSTRING, 0, (LPARAM)L"System Information");
        ToolbarButtons[6].iString = SendMessage(ToolBarHandle, TB_ADDSTRING, 0, (LPARAM)L"Find Window");
        ToolbarButtons[7].iString = SendMessage(ToolBarHandle, TB_ADDSTRING, 0, (LPARAM)L"Find Window and Thread");
        ToolbarButtons[8].iString = SendMessage(ToolBarHandle, TB_ADDSTRING, 0, (LPARAM)L"Find Window and Kill");
        ToolbarButtons[9].iString = SendMessage(ToolBarHandle, TB_ADDSTRING, 0, (LPARAM)L"Always on Top");

        // Set the toolbar struct size.
        SendMessage(ToolBarHandle, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        // Set the toolbar extended toolbar styles.
        SendMessage(ToolBarHandle, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DOUBLEBUFFER | TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_HIDECLIPPEDBUTTONS);
        // Configure the toolbar imagelist.
        SendMessage(ToolBarHandle, TB_SETIMAGELIST, 0, (LPARAM)ToolBarImageList);
        // Add the buttons to the toolbar (also specifying the default number of items to display).
        SendMessage(ToolBarHandle, TB_ADDBUTTONS, MAX_DEFAULT_TOOLBAR_ITEMS, (LPARAM)ToolbarButtons);
        // Restore the toolbar settings (Note: This will invoke the TBN_ENDADJUST notification).
        SendMessage(ToolBarHandle, TB_SAVERESTORE, FALSE, (LPARAM)&ToolbarSaveParams);

        // Enable theming:
        //SendMessage(RebarHandle, RB_SETWINDOWTHEME, 0, (LPARAM)L"Media"); //Media/Communications/BrowserTabBar/Help
        //SendMessage(ToolBarHandle, TB_SETWINDOWTHEME, 0, (LPARAM)L"Media"); //Media/Communications/BrowserTabBar/Help

        // HACK: Query the toolbar width/height.
        ULONG toolbarButtonSize = (ULONG)SendMessage(ToolBarHandle, TB_GETBUTTONSIZE, 0, 0);

        // Inset the toolbar into the rebar control.
        RebarBandInsert(BandID_ToolBar, ToolBarHandle, HIWORD(toolbarButtonSize), LOWORD(toolbarButtonSize));

        ToolbarInitialized = TRUE;
    }

    // Initialize the Searchbox and TreeNewFilters.
    if (EnableSearchBox && !SearchboxHandle)
    {
        SearchboxText = PhReferenceEmptyString();

        ProcessTreeFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportProcessTreeList(), (PPH_TN_FILTER_FUNCTION)ProcessTreeFilterCallback, NULL);
        ServiceTreeFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportServiceTreeList(), (PPH_TN_FILTER_FUNCTION)ServiceTreeFilterCallback, NULL);
        NetworkTreeFilterEntry = PhAddTreeNewFilter(PhGetFilterSupportNetworkTreeList(), (PPH_TN_FILTER_FUNCTION)NetworkTreeFilterCallback, NULL);

        // Create the Searchbox control.
        SearchboxHandle = CreateSearchControl(ID_SEARCH_CLEAR);
    }

    // Initialize the Statusbar control.
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
            NULL,
            NULL
            );
    }

    // Hide or show controls (Note: don't unload or remove at runtime).
    if (EnableToolBar)
    {
        if (RebarHandle && !IsWindowVisible(RebarHandle))
            ShowWindow(RebarHandle, SW_SHOW);
    }
    else
    {
        if (RebarHandle && IsWindowVisible(RebarHandle))
            ShowWindow(RebarHandle, SW_HIDE);
    }

    if (EnableSearchBox)
    {
        // Add the Searchbox band into the rebar control.
        if (!RebarBandExists(BandID_SearchBox))
            RebarBandInsert(BandID_SearchBox, SearchboxHandle, 20, 180);

        if (SearchboxHandle && !IsWindowVisible(SearchboxHandle))
            ShowWindow(SearchboxHandle, SW_SHOW);
    }
    else
    {
        // Remove the Searchbox band from the rebar control.
        if (RebarBandExists(BandID_SearchBox))
            RebarBandRemove(BandID_SearchBox);

        if (SearchboxHandle)
        {
            // Clear search text and reset search filters.
            SetFocus(SearchboxHandle);
            Static_SetText(SearchboxHandle, L"");

            if (IsWindowVisible(SearchboxHandle))
                ShowWindow(SearchboxHandle, SW_HIDE);
        }
    }

    // TODO: Fix above code...
    if (SearchBoxDisplayMode == SearchBoxDisplayHideInactive)
    {
        if (RebarBandExists(BandID_SearchBox))
            RebarBandRemove(BandID_SearchBox);
    }
    else
    {
        if (!RebarBandExists(BandID_SearchBox))
            RebarBandInsert(BandID_SearchBox, SearchboxHandle, 20, 180);
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
            button.dwMask = TBIF_BYINDEX | TBIF_STYLE | TBIF_COMMAND | TBIF_STATE;

            // Get settings for first button
            if (SendMessage(ToolBarHandle, TB_GETBUTTONINFO, index, (LPARAM)&button) == -1)
                break;

            // Skip separator buttons
            if (button.fsStyle == BTNS_SEP)
                continue;


            // TODO: We manually add the text above using TB_ADDSTRING,
            //       why do we need to set the button text again when changing TBIF_STYLE?
            button.dwMask |= TBIF_TEXT;
            button.pszText = ToolbarGetText(button.idCommand);


            if (button.idCommand == PHAPP_ID_VIEW_ALWAYSONTOP)
            {
                // Set the pressed state
                if (PhGetIntegerSetting(L"MainWindowAlwaysOnTop"))
                {
                    button.fsState |= TBSTATE_PRESSED;
                }
            }

            switch (DisplayStyle)
            {
            case ToolbarDisplayImageOnly:
                button.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
                break;
            case ToolbarDisplaySelectiveText:
                {
                    switch (button.idCommand)
                    {
                    case PHAPP_ID_VIEW_REFRESH:
                    case PHAPP_ID_HACKER_OPTIONS:
                    case PHAPP_ID_HACKER_FINDHANDLESORDLLS:
                    case PHAPP_ID_VIEW_SYSTEMINFORMATION:
                        button.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT;
                        break;
                    default:
                        button.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
                        break;
                    }
                }
                break;
            case ToolbarDisplayAllText:
                button.fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT;
                break;
            }

            // Set updated button info
            SendMessage(ToolBarHandle, TB_SETBUTTONINFO, index, (LPARAM)&button);
        }

        // Resize the toolbar
        SendMessage(ToolBarHandle, TB_AUTOSIZE, 0, 0);
        //InvalidateRect(ToolBarHandle, NULL, TRUE);
    }

    if (EnableToolBar && RebarHandle)
    {
        INT index;
        REBARBANDINFO rebarBandInfo = { REBARBANDINFO_V6_SIZE };
        rebarBandInfo.fMask = RBBIM_IDEALSIZE;

        index = (INT)SendMessage(RebarHandle, RB_IDTOINDEX, (WPARAM)BandID_ToolBar, 0);

        // Get settings for Rebar band.
        if (SendMessage(RebarHandle, RB_GETBANDINFO, index, (LPARAM)&rebarBandInfo) != -1)
        {
            SIZE idealWidth;

            // Reset the cxIdeal for the Chevron
            SendMessage(ToolBarHandle, TB_GETIDEALSIZE, FALSE, (LPARAM)&idealWidth);

            rebarBandInfo.cxIdeal = idealWidth.cx;

            SendMessage(RebarHandle, RB_SETBANDINFO, index, (LPARAM)&rebarBandInfo);
        }
    }

    // Invoke the LayoutPaddingCallback.
    SendMessage(PhMainWndHandle, WM_SIZE, 0, 0);
}

VOID ResetToolbarSettings(
    VOID
    )
{
    // Remove all the user customizations.
    INT buttonCount = (INT)SendMessage(ToolBarHandle, TB_BUTTONCOUNT, 0, 0);
    while (buttonCount--)
        SendMessage(ToolBarHandle, TB_DELETEBUTTON, (WPARAM)buttonCount, 0);

    // Re-add the original buttons.
    SendMessage(ToolBarHandle, TB_ADDBUTTONS, MAX_DEFAULT_TOOLBAR_ITEMS, (LPARAM)ToolbarButtons);

}

PWSTR ToolbarGetText(
    _In_ INT CommandID
    )
{
    switch (CommandID)
    {
    case PHAPP_ID_VIEW_REFRESH:
        return L"Refresh";
    case PHAPP_ID_HACKER_OPTIONS:
        return L"Options";
    case PHAPP_ID_HACKER_FINDHANDLESORDLLS:
        return L"Find Handles or DLLs";
    case PHAPP_ID_VIEW_SYSTEMINFORMATION:
        return L"System Information";
    case TIDC_FINDWINDOW:
        return L"Find Window";
    case TIDC_FINDWINDOWTHREAD:
        return L"Find Window and Thread";
    case TIDC_FINDWINDOWKILL:
        return L"Find Window and Kill";
    case PHAPP_ID_VIEW_ALWAYSONTOP:
        return L"Always on Top";
    }

    return L"Error";
}