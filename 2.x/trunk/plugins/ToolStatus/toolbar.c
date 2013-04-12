/*
 * Process Hacker ToolStatus -
 *   toolbar code
 *
 * Copyright (C) 2011-2013 dmex
 * Copyright (C) 2010-2012 wj32
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
        WS_CHILD | WS_VISIBLE | ES_LEFT,
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
    SendMessage(TextboxHandle, EM_SETCUEBANNER, 0, (LPARAM)L"Search Processes (Ctrl+ K)");

    if (WindowsVersion < WINDOWS_VISTA)
    {
        // Fixup the cue banner region - recalculate margins using WM_NCCALCSIZE
        SendMessage(TextboxHandle, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(0, 0));
    }

    // insert a paint region into the edit control NC window area       
    InsertButton(TextboxHandle, ID_SEARCH_CLEAR);

    PhAddTreeNewFilter(PhGetFilterSupportProcessTreeList(), (PPH_TN_FILTER_FUNCTION)ProcessTreeFilterCallback, TextboxHandle);
    PhAddTreeNewFilter(PhGetFilterSupportServiceTreeList(), (PPH_TN_FILTER_FUNCTION)ServiceTreeFilterCallback, TextboxHandle);
    PhAddTreeNewFilter(PhGetFilterSupportNetworkTreeList(), (PPH_TN_FILTER_FUNCTION)NetworkTreeFilterCallback, TextboxHandle);  
}