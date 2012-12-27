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
    PostMessage(ReBarHandle, RB_SETBARINFO, 0, (LPARAM)&rebarInfo);
}

VOID RebarAddMenuItem(
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