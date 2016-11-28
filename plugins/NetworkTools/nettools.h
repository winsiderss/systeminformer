/*
 * Process Hacker Network Tools -
 *   Main header
 *
 * Copyright (C) 2010-2013 wj32
 * Copyright (C) 2012-2016 dmex
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

#ifndef _NET_TOOLS_H_
#define _NET_TOOLS_H_

#define CINTERFACE
#define COBJMACROS
#include <windowsx.h>
#include <phdk.h>
#include <phappresource.h>
#include <workqueue.h>
#include <windowsx.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <winhttp.h>
#include <shlobj.h>

#include "resource.h"

#define PLUGIN_NAME L"ProcessHacker.NetworkTools"
#define SETTING_NAME_PING_WINDOW_POSITION (PLUGIN_NAME L".PingWindowPosition")
#define SETTING_NAME_PING_WINDOW_SIZE (PLUGIN_NAME L".PingWindowSize")
#define SETTING_NAME_PING_MINIMUM_SCALING (PLUGIN_NAME L".PingMinScaling")
#define SETTING_NAME_PING_SIZE (PLUGIN_NAME L".PingSize")
#define SETTING_NAME_TRACERT_WINDOW_POSITION (PLUGIN_NAME L".TracertWindowPosition")
#define SETTING_NAME_TRACERT_WINDOW_SIZE (PLUGIN_NAME L".TracertWindowSize")
#define SETTING_NAME_TRACERT_COLUMNS (PLUGIN_NAME L".TracertColumns")
#define SETTING_NAME_TRACERT_HISTORY (PLUGIN_NAME L".TracertAddresses")
#define SETTING_NAME_TRACERT_MAX_HOPS (PLUGIN_NAME L".TracertMaxHops")
#define SETTING_NAME_OUTPUT_WINDOW_POSITION (PLUGIN_NAME L".OutputWindowPosition")
#define SETTING_NAME_OUTPUT_WINDOW_SIZE (PLUGIN_NAME L".OutputWindowSize")

extern PPH_PLUGIN PluginInstance;
extern BOOLEAN GeoDbLoaded;
extern BOOLEAN GeoDbExpired;

// ICMP Packet Length: (msdn: IcmpSendEcho2/Icmp6SendEcho2)
// The buffer must be large enough to hold at least one ICMP_ECHO_REPLY or ICMPV6_ECHO_REPLY structure
//       + the number of bytes of data specified in the RequestSize parameter.
// This buffer should also be large enough to also hold 8 more bytes of data (the size of an ICMP error message)
//       + space for an IO_STATUS_BLOCK structure.
#define ICMP_BUFFER_SIZE(EchoReplyLength, Buffer) \
    (ULONG)(((EchoReplyLength) + (Buffer)->Length) + 8 + sizeof(IO_STATUS_BLOCK) + MAX_OPT_SIZE)

#define BITS_IN_ONE_BYTE 8
#define NDIS_UNIT_OF_MEASUREMENT 100

// The ICMPV6_ECHO_REPLY struct doesn't have a field to access the reply data,
// so copy the struct and add an additional Data field.
typedef struct _ICMPV6_ECHO_REPLY2
{
    IPV6_ADDRESS_EX Address;
    ULONG Status;
    unsigned int RoundTripTime;
    BYTE Data[ANYSIZE_ARRAY]; // custom
} ICMPV6_ECHO_REPLY2, *PICMPV6_ECHO_REPLY2;

typedef enum _PH_NETWORK_ACTION
{
    NETWORK_ACTION_PING = 1,
    NETWORK_ACTION_TRACEROUTE,
    NETWORK_ACTION_WHOIS,
    NETWORK_ACTION_FINISH,
    NETWORK_ACTION_PATHPING,

    MAINMENU_ACTION_PING,
    MAINMENU_ACTION_TRACERT,
    MAINMENU_ACTION_WHOIS,
    MAINMENU_ACTION_GEOIP_UPDATE,
} PH_NETWORK_ACTION;

#define NTM_RECEIVEDTRACE (WM_APP + NETWORK_ACTION_TRACEROUTE)
#define NTM_RECEIVEDWHOIS (WM_APP + NETWORK_ACTION_WHOIS)
#define NTM_RECEIVEDFINISH (WM_APP + NETWORK_ACTION_FINISH)
#define WM_TRACERT_ERROR (WM_APP + NETWORK_ACTION_TRACEROUTE + 1001)

#define UPDATE_MENUITEM    1005
#define PH_UPDATEISERRORED (WM_APP + 501)
#define PH_UPDATEISCURRENT (WM_APP + 503)
#define PH_UPDATENEWER     (WM_APP + 504)
#define PH_UPDATESUCCESS   (WM_APP + 505)
#define PH_UPDATEFAILURE   (WM_APP + 506)
#define WM_SHOWDIALOG      (WM_APP + 550)

typedef struct _NETWORK_OUTPUT_CONTEXT
{
    HWND WindowHandle;
    HWND StatusHandle;
    HWND PingGraphHandle;
    HWND WhoisHandle;
    HFONT FontHandle;

    ULONG CurrentPingMs;
    ULONG MaxPingTimeout;
    ULONG HashFailCount;
    ULONG UnknownAddrCount;
    ULONG PingMinMs;
    ULONG PingMaxMs;
    ULONG PingSentCount;
    ULONG PingRecvCount;
    ULONG PingLossCount;

    PH_NETWORK_ACTION Action;
    PH_LAYOUT_MANAGER LayoutManager;
    PH_WORK_QUEUE PingWorkQueue;
    PH_GRAPH_STATE PingGraphState;
    PH_CIRCULAR_BUFFER_ULONG PingHistory;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

    PH_IP_ENDPOINT RemoteEndpoint;
    WCHAR IpAddressString[INET6_ADDRSTRLEN + 1];
} NETWORK_OUTPUT_CONTEXT, *PNETWORK_OUTPUT_CONTEXT;

VOID PerformNetworkAction(
    _In_ PH_NETWORK_ACTION Action,
    _In_ PPH_NETWORK_ITEM NetworkItem
    );

NTSTATUS NetworkWhoisDialogThreadStart(
    _In_ PVOID Parameter
    );
NTSTATUS NetworkPingDialogThreadStart(
    _In_ PVOID Parameter
    );

VOID ShowOptionsDialog(
    _In_opt_ HWND Parent
    );

INT_PTR CALLBACK NetworkOutputDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID ShowTracertWindow(
    _In_ PPH_NETWORK_ITEM NetworkItem
    );

VOID ShowTracertWindowFromAddress(
    _In_ PH_IP_ENDPOINT RemoteEndpoint
    );

typedef struct _NETWORK_TRACERT_CONTEXT
{
    HWND WindowHandle;
    HWND ListviewHandle;
    HFONT FontHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    BOOLEAN Cancel;

    PH_IP_ENDPOINT RemoteEndpoint;
    WCHAR IpAddressString[INET6_ADDRSTRLEN + 1];
} NETWORK_TRACERT_CONTEXT, *PNETWORK_TRACERT_CONTEXT;

// country.c
typedef struct _NETWORK_EXTENSION
{
    BOOLEAN CountryValid;
    HICON CountryIcon;
    PPH_STRING RemoteCountryCode;
    PPH_STRING RemoteCountryName;
} NETWORK_EXTENSION, *PNETWORK_EXTENSION;

typedef enum _NETWORK_COLUMN_ID
{
    NETWORK_COLUMN_ID_REMOTE_COUNTRY = 1,
} NETWORK_COLUMN_ID;

// country.c
VOID LoadGeoLiteDb(VOID);
VOID FreeGeoLiteDb(VOID);

BOOLEAN LookupCountryCode(
    _In_ PH_IP_ADDRESS RemoteAddress,
    _Out_ PPH_STRING *CountryCode,
    _Out_ PPH_STRING *CountryName
    );

INT LookupResourceCode(
    _In_ PPH_STRING Name
    );

typedef struct _PH_UPDATER_CONTEXT
{
    BOOLEAN FixedWindowStyles;
    HWND DialogHandle;
    HICON IconSmallHandle;
    HICON IconLargeHandle;

    PPH_STRING FileDownloadUrl;
    PPH_STRING RevVersion;
    PPH_STRING Size;
    PPH_STRING SetupFilePath;
} PH_UPDATER_CONTEXT, *PPH_UPDATER_CONTEXT;

VOID TaskDialogLinkClicked(
    _In_ PPH_UPDATER_CONTEXT Context
    );

NTSTATUS GeoIPUpdateThread(
    _In_ PVOID Parameter
    );

VOID ShowUpdateDialog(
    _In_opt_ HWND Parent
    );

// page1.c
VOID ShowCheckForUpdatesDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    );

// page2.c
VOID ShowCheckingForUpdatesDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    );


// page5.c
VOID ShowInstallRestartDialog(
    _In_ PPH_UPDATER_CONTEXT Context
    );


// Note: Ipv6 versions are already available from ws2ipdef.h
#define INADDR_ANY (ULONG)0x00000000
#define INADDR_LOOPBACK 0x7f000001

FORCEINLINE BOOLEAN IN4_IS_ADDR_UNSPECIFIED(_In_ CONST IN_ADDR *a)
{
    return (BOOLEAN)(a->s_addr == INADDR_ANY);
}

FORCEINLINE BOOLEAN IN4_IS_ADDR_LOOPBACK(_In_ CONST IN_ADDR *a)
{
    return (BOOLEAN)(*((PUCHAR)a) == 0x7f); // 127/8
}

#endif