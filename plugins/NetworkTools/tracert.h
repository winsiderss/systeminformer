/*
 * Process Hacker Network Tools -
 *   Tracert header
 *
 * Copyright (C) 2015-2019 dmex
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

#ifndef _TRACERT_H_
#define _TRACERT_H_

#define DEFAULT_MAXIMUM_PINGS 4
#define DEFAULT_SEND_SIZE 64
#define DEFAULT_RECEIVE_SIZE ((sizeof(ICMP_ECHO_REPLY) + DEFAULT_SEND_SIZE + MAX_OPT_SIZE))
#define DEFAULT_TIMEOUT 1000
#define MIN_INTERVAL 500 //1000
#define TRACERT_SHOWCONTEXTMENU (WM_APP + 1)

typedef enum _TRACERT_TREE_COLUMN_ITEM_NAME
{
    TREE_COLUMN_ITEM_TTL,
    TREE_COLUMN_ITEM_PING1,
    TREE_COLUMN_ITEM_PING2,
    TREE_COLUMN_ITEM_PING3,
    TREE_COLUMN_ITEM_PING4,
    TREE_COLUMN_ITEM_IPADDR,
    TREE_COLUMN_ITEM_HOSTNAME,
    TREE_COLUMN_ITEM_COUNTRY,
    TREE_COLUMN_ITEM_MAXIMUM
} TRACERT_TREE_COLUMN_ITEM_NAME;

typedef struct _TRACERT_ROOT_NODE
{
    PH_TREENEW_NODE Node;

    ULONG UniqueId;
    ULONG TTL;

    ULONG PingStatus[DEFAULT_MAXIMUM_PINGS];
    ULONG PingList[DEFAULT_MAXIMUM_PINGS];

    PPH_STRING PingString[DEFAULT_MAXIMUM_PINGS];
    PPH_STRING PingMessage[DEFAULT_MAXIMUM_PINGS];

    INT CountryIconIndex;
    PPH_STRING TtlString;
    PPH_STRING HostnameString;
    PPH_STRING IpAddressString;
    PPH_STRING RemoteCountryCode;
    PPH_STRING RemoteCountryName;
    PH_STRINGREF TextCache[TREE_COLUMN_ITEM_MAXIMUM];
} TRACERT_ROOT_NODE, *PTRACERT_ROOT_NODE;

typedef struct _TRACERT_RESOLVE_WORKITEM
{
    ULONG Type;
    SOCKADDR_STORAGE SocketAddress;
    HWND WindowHandle;

    ULONG Index;
    WCHAR SocketAddressHostname[NI_MAXHOST];
} TRACERT_RESOLVE_WORKITEM, *PTRACERT_RESOLVE_WORKITEM;

VOID InitializeTracertTree(
    _Inout_ PNETWORK_TRACERT_CONTEXT Context
    );

VOID DeleteTracertTree(
    _In_ PNETWORK_TRACERT_CONTEXT Context
    );

PTRACERT_ROOT_NODE FindTracertNode(
    _In_ PNETWORK_TRACERT_CONTEXT Context,
    _In_ ULONG TTL
    );

PTRACERT_ROOT_NODE AddTracertNode(
    _Inout_ PNETWORK_TRACERT_CONTEXT Context,
    _In_ ULONG TTL
    );

VOID UpdateTracertNode(
    _In_ PNETWORK_TRACERT_CONTEXT Context,
    _In_ PTRACERT_ROOT_NODE WindowNode
    );

struct _PH_TN_FILTER_SUPPORT*
NTAPI
TracertGetFilterSupportTreeList(
    VOID
    );

VOID ClearTracertTree(
    _In_ PNETWORK_TRACERT_CONTEXT Context
    );

PTRACERT_ROOT_NODE GetSelectedTracertNode(
    _In_ PNETWORK_TRACERT_CONTEXT Context
    );

PPH_STRING TracertGetErrorMessage(
    _In_ IP_STATUS Result
    );

#endif
