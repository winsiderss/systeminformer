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
//#pragma comment(lib, "Ws2_32.lib")
//#pragma comment(lib, "dnsapi.lib")

#define PLUGIN_NAME L"ProcessHacker.NetAdapters"
#define SETTING_NAME_ENABLE_NDIS (PLUGIN_NAME L".EnableNDIS")
#define SETTING_NAME_INTERFACE_LIST (PLUGIN_NAME L".InterfaceList")

#define CINTERFACE
#define COBJMACROS
#include <phdk.h>
#include <phappresource.h>

#include <ws2def.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <nldef.h>
#include <netioapi.h>
//#include <WinSock2.h>

#include "resource.h"

#define MSG_UPDATE (WM_APP + 1)

extern PPH_PLUGIN PluginInstance;
extern PPH_LIST NetworkAdaptersList;

typedef struct _PH_NETADAPTER_ENTRY
{
    NET_IFINDEX InterfaceIndex;
    IF_LUID InterfaceLuid;
    PPH_STRING InterfaceGuid;
} PH_NETADAPTER_ENTRY, *PPH_NETADAPTER_ENTRY;

typedef struct _PH_NETADAPTER_CONTEXT
{
    HWND ListViewHandle;
    PPH_LIST NetworkAdaptersListEdited;
} PH_NETADAPTER_CONTEXT, *PPH_NETADAPTER_CONTEXT;

typedef struct _PH_NETADAPTER_SYSINFO_CONTEXT
{
    BOOLEAN HaveFirstSample;
    BOOLEAN HaveFirstDetailsSample;

    //ULONG64 LinkSpeed;
    ULONG64 InboundValue;
    ULONG64 OutboundValue;
    ULONG64 LastInboundValue;
    ULONG64 LastOutboundValue;

    //ULONG64 LastDetailsInboundValue;
    //ULONG64 LastDetailsIOutboundValue;
    //ULONG64 LastDetailsInboundUnicastValue;
    //ULONG64 LastDetailsIOutboundUnicastValue;

    PPH_STRING AdapterName;
    PPH_NETADAPTER_ENTRY AdapterEntry;

    HWND WindowHandle;
    HWND DetailsHandle;
    HWND DetailsLvHandle;
    HWND PanelWindowHandle;
    HWND GraphHandle;

    HANDLE DeviceHandle;

    PPH_SYSINFO_SECTION SysinfoSection;
    PH_GRAPH_STATE GraphState;
    PH_LAYOUT_MANAGER LayoutManager;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

    PH_CIRCULAR_BUFFER_ULONG64 InboundBuffer;
    PH_CIRCULAR_BUFFER_ULONG64 OutboundBuffer;
} PH_NETADAPTER_SYSINFO_CONTEXT, *PPH_NETADAPTER_SYSINFO_CONTEXT;

//typedef struct _PH_NETADAPTER_CONFIG
//{
//    ULONG AddressType;
//    ULONG EnableDHCP;
//    ULONG IsServerNapAware;
//
//    LARGE_INTEGER interfaceLease;
//    LARGE_INTEGER interfaceLeaseObtainedTime;
//    LARGE_INTEGER interfaceLeaseTerminatesTime;
//
//    PPH_STRING Domain;
//    PPH_STRING IPAddress;
//    PPH_STRING NameServer;
//    PPH_STRING SubnetMask;
//    PPH_STRING DefaultGateway;
//} PH_NETADAPTER_CONFIG, *PPH_NETADAPTER_CONFIG;

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

// dialog.c

VOID ShowDetailsDialog(
    _In_opt_ PPH_NETADAPTER_SYSINFO_CONTEXT Context
    );

// ndis.c

#define BITS_IN_ONE_BYTE 8
#define NDIS_UNIT_OF_MEASUREMENT 100

typedef ULONG (WINAPI* _GetIfEntry2)(
    _Inout_ PMIB_IF_ROW2 Row
    );

// dmex: rev
typedef ULONG (WINAPI* _GetInterfaceDescriptionFromGuid)(
    _Inout_ PGUID InterfaceGuid,
    _Out_opt_ PWSTR InterfaceDescription,
    _Inout_ PSIZE_T LengthAddress,
    PVOID Unknown1,
    PVOID Unknown2
    );

extern PVOID IphlpHandle;
extern _GetIfEntry2 GetIfEntry2_I;
extern _GetInterfaceDescriptionFromGuid GetInterfaceDescriptionFromGuid_I;

BOOLEAN NetworkAdapterQuerySupported(
    _In_ HANDLE DeviceHandle
    );

BOOLEAN NetworkAdapterQueryNdisVersion(
    _In_ HANDLE DeviceHandle,
    _Out_opt_ PUINT MajorVersion,
    _Out_opt_ PUINT MinorVersion
    );

PPH_STRING NetworkAdapterQueryName(
    _Inout_ PPH_NETADAPTER_SYSINFO_CONTEXT Context
    );

NTSTATUS NetworkAdapterQueryStatistics(
    _In_ HANDLE DeviceHandle,
    _Out_ PNDIS_STATISTICS_INFO Info
    );

NTSTATUS NetworkAdapterQueryLinkState(
    _In_ HANDLE DeviceHandle,
    _Out_ PNDIS_LINK_STATE State
    );

BOOLEAN NetworkAdapterQueryMediaType(
    _In_ HANDLE DeviceHandle,
    _Out_ PNDIS_PHYSICAL_MEDIUM Medium
    );

NTSTATUS NetworkAdapterQueryLinkSpeed(
    _In_ HANDLE DeviceHandle,
    _Out_ PULONG64 LinkSpeed
    );

ULONG64 NetworkAdapterQueryValue(
    _In_ HANDLE DeviceHandle,
    _In_ NDIS_OID OpCode
    );

MIB_IF_ROW2 QueryInterfaceRowVista(
    _Inout_ PPH_NETADAPTER_SYSINFO_CONTEXT Context
    );

MIB_IFROW QueryInterfaceRowXP(
    _Inout_ PPH_NETADAPTER_SYSINFO_CONTEXT Context
    );

#endif _NETADAPTER_H_