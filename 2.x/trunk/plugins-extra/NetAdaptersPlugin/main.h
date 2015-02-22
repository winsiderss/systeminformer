/*
 * Process Hacker Extra Plugins -
 *   Network Adapters Plugin
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

#ifndef _NETADAPTER_H_
#define _NETADAPTER_H_

#pragma comment(lib, "Iphlpapi.lib")

#define SETTING_PREFIX L"dmex.NetworkAdaptersPlugin"
#define SETTING_NAME_INTERFACE_LIST (SETTING_PREFIX L".NetworkInterfaceList")

#define CINTERFACE
#define COBJMACROS
#include <phdk.h>
#include <phappresource.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <ws2def.h>
#include <ws2ipdef.h>

#include "resource.h"

extern PPH_PLUGIN PluginInstance;
extern PPH_LIST NetworkAdaptersList;

typedef ULONG (WINAPI* _GetIfEntry2)(
    _Inout_ PMIB_IF_ROW2 Row
    );

typedef struct _PH_NETADAPTER_ENTRY
{
    IF_INDEX IfIndex;
    ULONG64 Luid64;
} PH_NETADAPTER_ENTRY, *PPH_NETADAPTER_ENTRY;

typedef struct _PH_NETADAPTER_CONTEXT
{
    HWND ListViewHandle;
    PPH_LIST NetworkAdaptersListEdited;
} PH_NETADAPTER_CONTEXT, *PPH_NETADAPTER_CONTEXT;

typedef struct _PH_NETADAPTER_SYSINFO_CONTEXT
{
    BOOLEAN HaveFirstSample;

    ULONG64 InboundValue;
    ULONG64 OutboundValue;
    ULONG64 MaxSendSpeed;
    ULONG64 MaxReceiveSpeed;
    ULONG64 LastInboundValue;
    ULONG64 LastOutboundValue;

    PPH_STRING AdapterName;
    PPH_NETADAPTER_ENTRY AdapterEntry;

    HWND WindowHandle;
    HWND PanelWindowHandle;
    HWND GraphHandle;
    HMODULE IphlpHandle;

    PPH_SYSINFO_SECTION SysinfoSection;
    PH_GRAPH_STATE GraphState;
    PH_LAYOUT_MANAGER LayoutManager;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

    PH_CIRCULAR_BUFFER_ULONG64 InboundBuffer;
    PH_CIRCULAR_BUFFER_ULONG64 OutboundBuffer;
} PH_NETADAPTER_SYSINFO_CONTEXT, *PPH_NETADAPTER_SYSINFO_CONTEXT;

VOID LoadAdaptersList(
    _Inout_ PPH_LIST FilterList,
    _In_ PPH_STRING String
    );

VOID ShowOptionsDialog(
    _In_ HWND ParentHandle
    );

VOID NetAdapterSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers,
    _In_ PPH_NETADAPTER_ENTRY AdapterInfo
    );

#endif _ROTHEADER_