/*
 * Process Hacker Extra Plugins -
 *   Wait Chain Traversal (WCT) Plugin
 *
 * Copyright (C) 2014 dmex
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

#ifndef _WCT_H_
#define _WCT_H_

#define CINTERFACE
#define COBJMACROS
#include <phdk.h>
#include <phappresource.h>
#include <colmgr.h>
#include <wct.h>
#include <psapi.h>
#include "resource.h"
#include "wndtree.h"

#define IDD_WCT_MENUITEM 1000
#define WCT_GETINFO_ALL_FLAGS (WCT_OUT_OF_PROC_FLAG|WCT_OUT_OF_PROC_COM_FLAG|WCT_OUT_OF_PROC_CS_FLAG|WCT_NETWORK_IO_FLAG)

typedef struct _WCT_CONTEXT
{
    HWND DialogHandle;
    HWND TreeNewHandle;
    
    HWND HighlightingWindow;
    ULONG HighlightingWindowCount;
    WCT_TREE_CONTEXT TreeContext;
    PH_LAYOUT_MANAGER LayoutManager;
        
    BOOLEAN IsProcessItem;
    PPH_THREAD_ITEM ThreadItem;
    PPH_PROCESS_ITEM ProcessItem;

    HWCT WctSessionHandle;
    HMODULE Ole32ModuleHandle;
} WCT_CONTEXT, *PWCT_CONTEXT;    

BOOLEAN WaitChainRegisterCallbacks(
    __inout PWCT_CONTEXT Context
    );

VOID WaitChainCheckThread(
    __inout PWCT_CONTEXT Context,
    __in HANDLE ThreadId
    );

#endif