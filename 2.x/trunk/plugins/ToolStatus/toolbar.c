/*
 * Process Hacker ToolStatus -
 *   toolbar code
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
#include "toolbar.h"

#define ID_SEARCH_CLEAR (WM_USER + 1)

HWND ToolBarHandle;
HIMAGELIST ToolBarImageList;
HWND TextboxHandle;
HFONT TextboxFontHandle;

VOID ToolBarCreate(
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

    //SendMessage(ReBarHandle, RB_SETWINDOWTHEME, 0, (LPARAM)L"Media"); //Media/Communications/BrowserTabBar/Help
    //SendMessage(ToolBarHandle, TB_SETWINDOWTHEME, 0, (LPARAM)L"Media"); //Media/Communications/BrowserTabBar/Help
}

VOID ToolbarCreateSearch(
    __in HWND ParentHandle
    )
{ 
    LOGFONT logFont;
    memset(&logFont, 0, sizeof(LOGFONT));
        
    logFont.lfHeight = WindowsVersion > WINDOWS_XP ? -11 : -12;
    logFont.lfWeight = FW_NORMAL;   

    wcscpy_s(
        logFont.lfFaceName, 
        _countof(logFont.lfFaceName), 
        WindowsVersion > WINDOWS_XP ? L"MS Shell Dlg 2" : L"MS Shell Dlg"
        );

    TextboxHandle = CreateWindowEx(
        0,
        WC_EDIT,
        NULL,
        WS_CHILD | ES_LEFT,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        ParentHandle,
        NULL,
        (HINSTANCE)PluginInstance->DllBase,
        NULL
        );

    // Create the font handle
    TextboxFontHandle = CreateFontIndirect(&logFont);

    // Set Searchbox control font
    SendMessage(TextboxHandle, WM_SETFONT, (WPARAM)TextboxFontHandle, MAKELPARAM(TRUE, 0));

    // Set initial text
    Edit_SetCueBannerText(TextboxHandle, L"Search Processes (Ctrl+ K)");
    
    // Fixup the cue banner region - recalculate margins using WM_NCCALCSIZE
    SendMessage(TextboxHandle, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(0, 0));

    // insert a paint region into the edit control NC window area       
    InsertButton(TextboxHandle, ID_SEARCH_CLEAR, 25);

    PhAddTreeNewFilter(PhGetFilterSupportProcessTreeList(), (PPH_TN_FILTER_FUNCTION)ProcessTreeFilterCallback, TextboxHandle);
    PhAddTreeNewFilter(PhGetFilterSupportServiceTreeList(), (PPH_TN_FILTER_FUNCTION)ServiceTreeFilterCallback, TextboxHandle);
    PhAddTreeNewFilter(PhGetFilterSupportNetworkTreeList(), (PPH_TN_FILTER_FUNCTION)NetworkTreeFilterCallback, TextboxHandle);  
}

VOID ToolBarCreateImageList(
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
    SendMessage(WindowHandle, TB_SETIMAGELIST, 0, (LPARAM)ToolBarImageList); 
}

VOID ToolbarAddMenuItems(
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
