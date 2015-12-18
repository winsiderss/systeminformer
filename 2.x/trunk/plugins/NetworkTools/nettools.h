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

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

#define CINTERFACE
#define COBJMACROS
#include <windowsx.h>
#include <phdk.h>
#include <phappresource.h>
#include <windowsx.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>

#include "resource.h"

#define PLUGIN_NAME L"ProcessHacker.NetworkTools"
#define SETTING_NAME_TRACERT_WINDOW_POSITION (PLUGIN_NAME L".WindowPosition")
#define SETTING_NAME_TRACERT_WINDOW_SIZE (PLUGIN_NAME L".WindowSize")
#define SETTING_NAME_PING_WINDOW_POSITION (PLUGIN_NAME L".PingWindowPosition")
#define SETTING_NAME_PING_WINDOW_SIZE (PLUGIN_NAME L".PingWindowSize")
#define SETTING_NAME_PING_TIMEOUT (PLUGIN_NAME L".PingMaxTimeout")

// ICMP Packet Length: (msdn: IcmpSendEcho2/Icmp6SendEcho2)
// The buffer must be large enough to hold at least one ICMP_ECHO_REPLY or ICMPV6_ECHO_REPLY structure
//       + the number of bytes of data specified in the RequestSize parameter.
// This buffer should also be large enough to also hold 8 more bytes of data (the size of an ICMP error message)
//       + space for an IO_STATUS_BLOCK structure.
#define ICMP_BUFFER_SIZE(EchoReplyLength, Buffer) (ULONG)(((EchoReplyLength) + (Buffer)->Length) + 8 + sizeof(IO_STATUS_BLOCK))

extern PPH_PLUGIN PluginInstance;

typedef enum _PH_NETWORK_ACTION
{
    NETWORK_ACTION_PING,
    NETWORK_ACTION_TRACEROUTE,
    NETWORK_ACTION_WHOIS,
    NETWORK_ACTION_FINISH,
    NETWORK_ACTION_PATHPING
} PH_NETWORK_ACTION;

// output
#define NTM_RECEIVEDTRACE (WM_APP + NETWORK_ACTION_TRACEROUTE)
#define NTM_RECEIVEDWHOIS (WM_APP + NETWORK_ACTION_WHOIS)
#define NTM_RECEIVEDFINISH (WM_APP + NETWORK_ACTION_FINISH)

typedef struct _NETWORK_OUTPUT_CONTEXT
{
    PH_NETWORK_ACTION Action;
    PH_LAYOUT_MANAGER LayoutManager;
    PH_WORK_QUEUE PingWorkQueue;
    PH_GRAPH_STATE PingGraphState;
    PH_CIRCULAR_BUFFER_ULONG PingHistory;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

    HWND WindowHandle;
    HWND ParentHandle;
    HWND StatusHandle;
    HWND PingGraphHandle;
    HWND OutputHandle;
    HANDLE PipeReadHandle;
    HANDLE ProcessHandle;
    HFONT FontHandle;
    HICON IconHandle;

    ULONG CurrentPingMs;
    ULONG MaxPingTimeout;
    ULONG HashFailCount;
    ULONG UnknownAddrCount;
    ULONG PingMinMs;
    ULONG PingMaxMs;
    ULONG PingSentCount;
    ULONG PingRecvCount;
    ULONG PingLossCount;

    PPH_NETWORK_ITEM NetworkItem;
    PH_IP_ADDRESS IpAddress;
    WCHAR IpAddressString[INET6_ADDRSTRLEN];
} NETWORK_OUTPUT_CONTEXT, *PNETWORK_OUTPUT_CONTEXT;

NTSTATUS PhNetworkPingDialogThreadStart(
    _In_ PVOID Parameter
    );

VOID PerformNetworkAction(
    _In_ PH_NETWORK_ACTION Action,
    _In_ PPH_NETWORK_ITEM NetworkItem
    );

NTSTATUS NetworkPingThreadStart(
    _In_ PVOID Parameter
    );

NTSTATUS NetworkTracertThreadStart(
    _In_ PVOID Parameter
    );

NTSTATUS NetworkWhoisThreadStart(
    _In_ PVOID Parameter
    );

VOID NTAPI ShowOptionsCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

INT_PTR CALLBACK NetworkOutputDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

#endif
