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

extern PPH_PLUGIN PluginInstance;

typedef enum _PH_NETWORK_ACTION
{
    NETWORK_ACTION_PING = 1,
    NETWORK_ACTION_TRACEROUTE = 2,
    NETWORK_ACTION_WHOIS = 3
} PH_NETWORK_ACTION;

// output
#define NTM_RECEIVEDPING (WM_APP + NETWORK_ACTION_PING)
#define NTM_RECEIVEDTRACE (WM_APP + NETWORK_ACTION_TRACEROUTE)
#define NTM_RECEIVEDWHOIS (WM_APP + NETWORK_ACTION_WHOIS)

typedef struct _NETWORK_OUTPUT_CONTEXT
{
    PH_NETWORK_ACTION Action;
    HWND WindowHandle;
    HWND PingGraphHandle;
    HANDLE ThreadHandle;
    HANDLE PipeReadHandle;
    HANDLE ProcessHandle;
    ULONG RemoteAddrType;
    LONG64 CurrentPingMs;
    LONG64 PingMinMs;
    LONG64 PingMaxMs;
    LONG64 PingAvgMs;
    LONG64 PingSentCount;
    LONG64 PingRecvCount;
    LONG64 PingLossCount;
    PPH_NETWORK_ITEM NetworkItem;
    PH_LAYOUT_MANAGER LayoutManager;
    PH_GRAPH_STATE PingGraphState;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
    PH_CIRCULAR_BUFFER_ULONG64 PingHistory;
    PH_STRING_BUILDER ReceivedString;
    WCHAR addressString[65];
} NETWORK_OUTPUT_CONTEXT, *PNETWORK_OUTPUT_CONTEXT;

NTSTATUS PhNetworkPingDialogThreadStart(
    __in PVOID Parameter
    );

VOID PerformNetworkAction(
    __in PH_NETWORK_ACTION Action,
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
