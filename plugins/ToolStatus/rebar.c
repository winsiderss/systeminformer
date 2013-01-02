/*
 * Process Hacker ToolStatus -
 *   rebar code
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
#include "rebar.h"

HWND ReBarHandle = NULL;
RECT ReBarRect = { 0, 0, 0, 0 };

VOID RebarCreate(
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
    SendMessage(ReBarHandle, RB_SETBARINFO, 0, (LPARAM)&rebarInfo);
    //SendMessage(ReBarHandle, RB_SETWINDOWTHEME, 0, (LPARAM)L"Communications"); //Media/Communications/BrowserTabBar/Help
}

VOID RebarDestroy(
    VOID
    )
{
    DestroyWindow(ReBarHandle);
    ReBarHandle = NULL;
}

VOID RebarAddMenuItem(
    __in HWND WindowHandle,
    __in HWND ChildHandle,
    __in UINT ID,
    __in UINT cyMinChild,
    __in UINT cxMinChild
    )
{
    REBARBANDINFO rebarBandInfo = { REBARBANDINFO_V6_SIZE }; 
    rebarBandInfo.fMask = RBBIM_STYLE | RBBIM_ID | RBBIM_CHILD | RBBIM_CHILDSIZE;
    rebarBandInfo.fStyle = RBBS_NOGRIPPER;
    
    rebarBandInfo.wID = ID;
    rebarBandInfo.hwndChild = ChildHandle;
    rebarBandInfo.cyMinChild = cyMinChild;
    rebarBandInfo.cxMinChild = cxMinChild;

    SendMessage(WindowHandle, RB_INSERTBAND, (WPARAM)-1, (LPARAM)&rebarBandInfo);
}