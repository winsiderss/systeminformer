/*
 * Process Hacker Extra Plugins -
 *   Network Extras Plugin
 *
 * Copyright (C) 2015 dmex
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

#ifndef _EXTRA_H_
#define _EXTRA_H_

#define PLUGIN_NAME L"dmex.NetworkExtrasPlugin"

#define CINTERFACE
#define COBJMACROS
#include <phdk.h>
#include <phappresource.h>
#include <colmgr.h>

#include "resource.h"

typedef struct _NETWORK_EXTRA_CONTEXT
{
    PH_LAYOUT_MANAGER LayoutManager;
} NETWORK_EXTRA_CONTEXT, *PNETWORK_EXTRA_CONTEXT;

typedef struct _NETWORK_EXTENSION
{
    BOOLEAN LocalValid;
    BOOLEAN RemoteValid;
    PPH_STRING LocalServiceName;
    PPH_STRING RemoteServiceName;
} NETWORK_EXTENSION, *PNETWORK_EXTENSION;

typedef enum _NETWORK_COLUMN_ID
{
    NETWORK_COLUMN_ID_LOCAL_SERVICE = 1,
    NETWORK_COLUMN_ID_REMOTE_SERVICE = 2
} NETWORK_COLUMN_ID;

typedef struct _RESOLVED_PORT
{
    PWSTR Name;
    USHORT Port;
} RESOLVED_PORT;

extern RESOLVED_PORT ResolvedPortsTable[6265];

VOID UpdateNetworkNode(
    _In_ NETWORK_COLUMN_ID ColumnID,
    _In_ PPH_NETWORK_NODE Node,
    _In_ PNETWORK_EXTENSION Extension
    );

#endif