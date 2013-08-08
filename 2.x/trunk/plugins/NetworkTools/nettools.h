/*
 * Process Hacker ToolStatus -
 *   toolstatus header
 *
 * Copyright (C) 2010-2013 wj32
 * Copyright (C) 2012-2013 dmex
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

#ifndef NETTOOLS_H
#define NETTOOLS_H

#define CINTERFACE
#define COBJMACROS
#include <windowsx.h>
#include <phdk.h>
#include <phapppub.h>
#include <phplug.h>
#include <phappresource.h>
#include <windowsx.h>

#include "resource.h"

// output
#define NETWORK_ACTION_PING 1
#define NETWORK_ACTION_TRACEROUTE 2
#define NETWORK_ACTION_WHOIS 3
#define NTM_RECEIVEDPING (WM_APP + NETWORK_ACTION_PING)
#define NTM_RECEIVEDTRACE (WM_APP + NETWORK_ACTION_TRACEROUTE)
#define NTM_RECEIVEDWHOIS (WM_APP + NETWORK_ACTION_WHOIS)

extern PPH_PLUGIN PluginInstance;

typedef struct _NETWORK_OUTPUT_CONTEXT
{
    ULONG Action;
    PPH_NETWORK_ITEM NetworkItem;
    PH_LAYOUT_MANAGER LayoutManager;
    HWND WindowHandle;
    BOOLEAN UseOldColors;
    PH_QUEUED_LOCK TextBufferLock;

    HANDLE ThreadHandle;
    HANDLE PipeReadHandle;
    HANDLE ProcessHandle;
    WCHAR addressString[65];
    PH_STRING_BUILDER ReceivedString;
} NETWORK_OUTPUT_CONTEXT, *PNETWORK_OUTPUT_CONTEXT;

VOID PerformNetworkAction(
    __in ULONG Action,
    __in PPH_NETWORK_ITEM NetworkItem
    );

NTSTATUS NetworkPingThreadStart(
    __in PVOID Parameter
    );

NTSTATUS NetworkTracertThreadStart(
    __in PVOID Parameter
    );

NTSTATUS NetworkWhoisThreadStart(
    __in PVOID Parameter
    );

VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

INT_PTR CALLBACK NetworkOutputDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK OptionsDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

#endif
