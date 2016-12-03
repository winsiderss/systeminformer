/*
 * Process Hacker Network Tools -
 *   Tracert header
 *
 * Copyright (C) 2015-2016 dmex
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

#define POOL_TABLE_SHOWCONTEXTMENU (WM_APP + 9849)

typedef enum _POOLTAG_TREE_COLUMN_ITEM_NAME
{
    TREE_COLUMN_ITEM_TTL,
    TREE_COLUMN_ITEM_HOSTNAME,
    TREE_COLUMN_ITEM_PING1,
    TREE_COLUMN_ITEM_PING2,
    TREE_COLUMN_ITEM_PING3,
    TREE_COLUMN_ITEM_PING4,
    TREE_COLUMN_ITEM_IPADDR,
    TREE_COLUMN_ITEM_COUNTRY,
    TREE_COLUMN_ITEM_MAXIMUM
} POOLTAG_TREE_COLUMN_ITEM_NAME;

typedef struct _POOLTAG_ROOT_NODE
{
    PH_TREENEW_NODE Node;
    HICON CountryIcon;

    ULONG TTL;
    ULONG Ping1;
    ULONG Ping2;
    ULONG Ping3;
    ULONG Ping4;

    PPH_STRING TtlString;
    PPH_STRING Ping1String;
    PPH_STRING Ping2String;
    PPH_STRING Ping3String;
    PPH_STRING Ping4String;
    PPH_STRING CountryString;
    PPH_STRING HostnameString;
    PPH_STRING IpAddressString;

    PPH_STRING RemoteCountryCode;
    PPH_STRING RemoteCountryName;

    PH_STRINGREF TextCache[TREE_COLUMN_ITEM_MAXIMUM];
} POOLTAG_ROOT_NODE, *PPOOLTAG_ROOT_NODE;

VOID PmLoadSettingsTreeList(
    _Inout_ PNETWORK_TRACERT_CONTEXT Context
    );

VOID PmSaveSettingsTreeList(
    _Inout_ PNETWORK_TRACERT_CONTEXT Context
    );

VOID PmInitializePoolTagTree(
    _Inout_ PNETWORK_TRACERT_CONTEXT Context
    );

VOID PmDeletePoolTagTree(
    _In_ PNETWORK_TRACERT_CONTEXT Context
    );

PPOOLTAG_ROOT_NODE PmFindPoolTagNode(
    _In_ PNETWORK_TRACERT_CONTEXT Context,
    _In_ ULONG PoolTag
    );

PPOOLTAG_ROOT_NODE PmAddPoolTagNode(
    _Inout_ PNETWORK_TRACERT_CONTEXT Context,
    _In_ ULONG TTL
    );

VOID PmUpdatePoolTagNode(
    _In_ PNETWORK_TRACERT_CONTEXT Context,
    _In_ PPOOLTAG_ROOT_NODE WindowNode
    );

struct _PH_TN_FILTER_SUPPORT*
NTAPI
PmGetFilterSupportTreeList(
    VOID
    );

PPOOLTAG_ROOT_NODE PmGetSelectedPoolTagNode(
    _In_ PNETWORK_TRACERT_CONTEXT Context
    );

#endif