/*
 * Process Hacker Plugins -
 *   Hardware Devices Plugin
 *
 * Copyright (C) 2015-2016 dmex
 * Copyright (C) 2016 wj32
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

#ifndef _DEVICES_H_
#define _DEVICES_H_

#pragma comment(lib, "Iphlpapi.lib")
//#pragma comment(lib, "Ws2_32.lib")
//#pragma comment(lib, "dnsapi.lib")

#define PLUGIN_NAME L"ProcessHacker.HardwareDevices"
#define SETTING_NAME_ENABLE_NDIS (PLUGIN_NAME L".EnableNDIS")
#define SETTING_NAME_ENABLE_HIDDEN_ADAPTERS (PLUGIN_NAME L".EnableHiddenAdapters")
#define SETTING_NAME_INTERFACE_LIST (PLUGIN_NAME L".NetworkList")
#define SETTING_NAME_DISK_LIST (PLUGIN_NAME L".DiskList")

#define CINTERFACE
#define COBJMACROS
#include <phdk.h>
#include <phappresource.h>
#include <phgui.h>

#include <windowsx.h>
#include <Uxtheme.h>
#include <ws2def.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <nldef.h>
#include <netioapi.h>
//#include <WinSock2.h>

#include "resource.h"

#define WM_SHOWDIALOG (WM_APP + 1)
#define UPDATE_MSG (WM_APP + 2)

extern PPH_PLUGIN PluginInstance;

extern PPH_OBJECT_TYPE NetAdapterEntryType;
extern PPH_LIST NetworkAdaptersList;
extern PH_QUEUED_LOCK NetworkAdaptersListLock;

extern PPH_OBJECT_TYPE DiskDriveEntryType;
extern PPH_LIST DiskDrivesList;
extern PH_QUEUED_LOCK DiskDrivesListLock;

// Network Adapters

typedef struct _DV_NETADAPTER_ID
{
    NET_IFINDEX InterfaceIndex;
    IF_LUID InterfaceLuid;
    PPH_STRING InterfaceGuid;
} DV_NETADAPTER_ID, *PDV_NETADAPTER_ID;

typedef struct _DV_NETADAPTER_ENTRY
{
    DV_NETADAPTER_ID Id;
    PPH_STRING AdapterName;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG UserReference : 1;
            ULONG HaveFirstSample : 1;
            ULONG CheckedDeviceSupport : 1;
            ULONG DeviceSupported : 1;
            ULONG Spare : 28;
        };
    };

    //ULONG64 LinkSpeed;
    ULONG64 InboundValue;
    ULONG64 OutboundValue;
    ULONG64 LastInboundValue;
    ULONG64 LastOutboundValue;

    PH_CIRCULAR_BUFFER_ULONG64 InboundBuffer;
    PH_CIRCULAR_BUFFER_ULONG64 OutboundBuffer;
} DV_NETADAPTER_ENTRY, *PDV_NETADAPTER_ENTRY;

typedef struct _DV_NETADAPTER_SYSINFO_CONTEXT
{
    PDV_NETADAPTER_ENTRY AdapterEntry;
    PPH_STRING SectionName;

    HWND WindowHandle;
    HWND PanelWindowHandle;
    HWND GraphHandle;
    
    PPH_SYSINFO_SECTION SysinfoSection;
    PH_GRAPH_STATE GraphState;
    PH_LAYOUT_MANAGER LayoutManager;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
} DV_NETADAPTER_SYSINFO_CONTEXT, *PDV_NETADAPTER_SYSINFO_CONTEXT;

typedef struct _DV_NETADAPTER_DETAILS_CONTEXT
{
    PPH_STRING AdapterName;
    DV_NETADAPTER_ID AdapterId;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG UserReference : 1;
            ULONG HaveFirstSample : 1;
            ULONG HaveCheckedDeviceSupport : 1;
            ULONG HaveDeviceSupport : 1;
            ULONG Spare : 28;
        };
    };

    HWND WindowHandle;
    HWND ParentHandle;
    HWND ListViewHandle;
   
    HANDLE NotifyHandle;

    PH_LAYOUT_MANAGER LayoutManager;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

    ULONG64 LastDetailsInboundValue;
    ULONG64 LastDetailsIOutboundValue;
} DV_NETADAPTER_DETAILS_CONTEXT, *PDV_NETADAPTER_DETAILS_CONTEXT;

typedef struct _DV_NETADAPTER_CONTEXT
{
    HWND ListViewHandle;
    BOOLEAN OptionsChanged;
} DV_NETADAPTER_CONTEXT, *PDV_NETADAPTER_CONTEXT;

VOID NetAdaptersLoadList(
    VOID
    );

VOID ShowOptionsDialog(
    _In_ HWND ParentHandle
    );

VOID NetAdapterSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers,
    _In_ _Assume_refs_(1) PDV_NETADAPTER_ENTRY AdapterInfo
    );

// adapter.c

VOID NetAdaptersInitialize(
    VOID
    );

VOID NetAdaptersUpdate(
    VOID
    );

VOID InitializeNetAdapterId(
    _Out_ PDV_NETADAPTER_ID Id,
    _In_ NET_IFINDEX InterfaceIndex,
    _In_ IF_LUID InterfaceLuid,
    _In_ PPH_STRING InterfaceGuid
    );

VOID CopyNetAdapterId(
    _Out_ PDV_NETADAPTER_ID Destination,
    _In_ PDV_NETADAPTER_ID Source
    );

VOID DeleteNetAdapterId(
    _Inout_ PDV_NETADAPTER_ID Id
    );

BOOLEAN EquivalentNetAdapterId(
    _In_ PDV_NETADAPTER_ID Id1,
    _In_ PDV_NETADAPTER_ID Id2
    );

PDV_NETADAPTER_ENTRY CreateNetAdapterEntry(
    _In_ PDV_NETADAPTER_ID Id
    );

// dialog.c

typedef enum _NETADAPTER_DETAILS_CATEGORY
{
    NETADAPTER_DETAILS_CATEGORY_ADAPTER,
    NETADAPTER_DETAILS_CATEGORY_UNICAST,
    NETADAPTER_DETAILS_CATEGORY_BROADCAST,
    NETADAPTER_DETAILS_CATEGORY_MULTICAST,
    NETADAPTER_DETAILS_CATEGORY_ERRORS
} NETADAPTER_DETAILS_CATEGORY;

typedef enum _NETADAPTER_DETAILS_INDEX
{
    NETADAPTER_DETAILS_INDEX_STATE,
    //NETADAPTER_DETAILS_INDEX_CONNECTIVITY,
    
    NETADAPTER_DETAILS_INDEX_IPADDRESS,
    NETADAPTER_DETAILS_INDEX_SUBNET,
    NETADAPTER_DETAILS_INDEX_GATEWAY,
    NETADAPTER_DETAILS_INDEX_DOMAIN,

    NETADAPTER_DETAILS_INDEX_LINKSPEED,
    NETADAPTER_DETAILS_INDEX_SENT,
    NETADAPTER_DETAILS_INDEX_RECEIVED,
    NETADAPTER_DETAILS_INDEX_TOTAL,
    NETADAPTER_DETAILS_INDEX_SENDING,
    NETADAPTER_DETAILS_INDEX_RECEIVING,
    NETADAPTER_DETAILS_INDEX_UTILIZATION,

    NETADAPTER_DETAILS_INDEX_UNICAST_SENTPKTS,
    NETADAPTER_DETAILS_INDEX_UNICAST_RECVPKTS,
    NETADAPTER_DETAILS_INDEX_UNICAST_TOTALPKTS,
    NETADAPTER_DETAILS_INDEX_UNICAST_SENT,
    NETADAPTER_DETAILS_INDEX_UNICAST_RECEIVED,
    NETADAPTER_DETAILS_INDEX_UNICAST_TOTAL,
    //NETADAPTER_DETAILS_INDEX_UNICAST_SENDING,
    //NETADAPTER_DETAILS_INDEX_UNICAST_RECEIVING,
    //NETADAPTER_DETAILS_INDEX_UNICAST_UTILIZATION,

    NETADAPTER_DETAILS_INDEX_BROADCAST_SENTPKTS,
    NETADAPTER_DETAILS_INDEX_BROADCAST_RECVPKTS,
    NETADAPTER_DETAILS_INDEX_BROADCAST_TOTALPKTS,
    NETADAPTER_DETAILS_INDEX_BROADCAST_SENT,
    NETADAPTER_DETAILS_INDEX_BROADCAST_RECEIVED,
    NETADAPTER_DETAILS_INDEX_BROADCAST_TOTAL,

    NETADAPTER_DETAILS_INDEX_MULTICAST_SENTPKTS,
    NETADAPTER_DETAILS_INDEX_MULTICAST_RECVPKTS,
    NETADAPTER_DETAILS_INDEX_MULTICAST_TOTALPKTS,
    NETADAPTER_DETAILS_INDEX_MULTICAST_SENT,
    NETADAPTER_DETAILS_INDEX_MULTICAST_RECEIVED,
    NETADAPTER_DETAILS_INDEX_MULTICAST_TOTAL,

    NETADAPTER_DETAILS_INDEX_ERRORS_SENTPKTS,
    NETADAPTER_DETAILS_INDEX_ERRORS_RECVPKTS,
    NETADAPTER_DETAILS_INDEX_ERRORS_TOTALPKTS,
    NETADAPTER_DETAILS_INDEX_ERRORS_SENT,
    NETADAPTER_DETAILS_INDEX_ERRORS_RECEIVED,
    NETADAPTER_DETAILS_INDEX_ERRORS_TOTAL
} NETADAPTER_DETAILS_INDEX;

VOID ShowDetailsDialog(
    _In_ PDV_NETADAPTER_SYSINFO_CONTEXT Context
    );

typedef NETIO_STATUS (WINAPI* _ConvertLengthToIpv4Mask)(
    _In_ ULONG MaskLength,
    _Out_ PULONG Mask
    );
typedef NETIO_STATUS (WINAPI* _CancelMibChangeNotify2)(
    _In_ HANDLE NotificationHandle
    );

typedef NETIO_STATUS (WINAPI* _NotifyIpInterfaceChange)(
    _In_ ADDRESS_FAMILY Family,
    _In_ PIPINTERFACE_CHANGE_CALLBACK Callback,
    _In_opt_ PVOID CallerContext,
    _In_ BOOLEAN InitialNotification,
    _Inout_ HANDLE *NotificationHandle
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
extern _NotifyIpInterfaceChange NotifyIpInterfaceChange_I;
extern _CancelMibChangeNotify2 CancelMibChangeNotify2_I;
extern _ConvertLengthToIpv4Mask ConvertLengthToIpv4Mask_I;

NTSTATUS NetworkAdapterCreateHandle(
    _Out_ PHANDLE DeviceHandle,
    _In_ PPH_STRING InterfaceGuid
    );

BOOLEAN NetworkAdapterQuerySupported(
    _In_ HANDLE DeviceHandle
    );

BOOLEAN NetworkAdapterQueryNdisVersion(
    _In_ HANDLE DeviceHandle,
    _Out_opt_ PUINT MajorVersion,
    _Out_opt_ PUINT MinorVersion
    );

PPH_STRING NetworkAdapterQueryName(
    _In_ HANDLE DeviceHandle,
    _In_ PPH_STRING InterfaceGuid
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
    _In_ PDV_NETADAPTER_ID Id
    );

MIB_IFROW QueryInterfaceRowXP(
    _In_ PDV_NETADAPTER_ID Id
    );




// options.c

INT_PTR CALLBACK NetworkAdapterOptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK DiskDriveOptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );



// Disk Drives

typedef struct _DV_DISK_ID
{
    PPH_STRING DevicePath;
} DV_DISK_ID, *PDV_DISK_ID;

typedef struct _DV_DISK_ENTRY
{
    DV_DISK_ID Id;

    PPH_STRING DiskName;
    PPH_STRING DiskIndexName;
    ULONG DiskIndex;

    union
    {
        ULONG Flags;
        struct
        {
            ULONG UserReference : 1;
            ULONG HaveFirstSample : 1;
            ULONG Spare : 30;
        };
    };

    PH_CIRCULAR_BUFFER_ULONG64 ReadBuffer;
    PH_CIRCULAR_BUFFER_ULONG64 WriteBuffer;

    PH_UINT64_DELTA BytesReadDelta;
    PH_UINT64_DELTA BytesWrittenDelta;
    PH_UINT64_DELTA ReadTimeDelta;
    PH_UINT64_DELTA WriteTimeDelta;
    PH_UINT64_DELTA IdleTimeDelta;
    PH_UINT32_DELTA ReadCountDelta;
    PH_UINT32_DELTA WriteCountDelta;
    PH_UINT64_DELTA QueryTimeDelta;

    FLOAT ResponseTime;
    FLOAT ActiveTime;
    ULONG QueueDepth;
    ULONG SplitCount;
} DV_DISK_ENTRY, *PDV_DISK_ENTRY;

typedef struct _DV_DISK_SYSINFO_CONTEXT
{
    PDV_DISK_ENTRY AdapterEntry;
    PPH_STRING SectionName;

    HWND WindowHandle;
    HWND PanelWindowHandle;
    HWND GraphHandle;

    PPH_SYSINFO_SECTION SysinfoSection;
    PH_GRAPH_STATE GraphState;
    PH_LAYOUT_MANAGER LayoutManager;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
} DV_DISK_SYSINFO_CONTEXT, *PDV_DISK_SYSINFO_CONTEXT;

typedef struct _DV_DISK_OPTIONS_CONTEXT
{
    HWND ListViewHandle;
    BOOLEAN OptionsChanged;
} DV_DISK_OPTIONS_CONTEXT, *PDV_DISK_OPTIONS_CONTEXT;



VOID DiskDrivesInitialize(VOID);
VOID DiskDrivesLoadList(VOID);
VOID DiskDrivesUpdate(VOID);



VOID InitializeDiskId(
    _Out_ PDV_DISK_ID Id,
    _In_ PPH_STRING DevicePath
    );
VOID CopyDiskId(
    _Out_ PDV_DISK_ID Destination,
    _In_ PDV_DISK_ID Source
    );
VOID DeleteDiskId(
    _Inout_ PDV_DISK_ID Id
    );
BOOLEAN EquivalentDiskId(
    _In_ PDV_DISK_ID Id1,
    _In_ PDV_DISK_ID Id2
    );
PDV_DISK_ENTRY CreateDiskEntry(
    _In_ PDV_DISK_ID Id
    );



VOID DiskDriveSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers,
    _In_ PDV_DISK_ENTRY DiskEntry
    );


// storage.c

NTSTATUS DiskDriveCreateHandle(
    _Out_ PHANDLE DeviceHandle,
    _In_ PPH_STRING DevicePath
    );

PPH_STRING DiskDriveQueryDosMountPoints(
    _In_ ULONG DeviceNumber
    );

BOOLEAN DiskDriveQueryDeviceInformation(
    _In_ HANDLE DeviceHandle,
    _Out_opt_ PPH_STRING* DiskVendor,
    _Out_opt_ PPH_STRING* DiskModel,
    _Out_opt_ PPH_STRING* DiskRevision,
    _Out_opt_ PPH_STRING* DiskSerial
    );

NTSTATUS DiskDriveQueryDeviceTypeAndNumber(
    _In_ HANDLE DeviceHandle,
    _Out_opt_ PULONG DeviceNumber,
    _Out_opt_ DEVICE_TYPE* DeviceType
    );

PPH_STRING DiskDriveQueryGeometry(
    _In_ HANDLE DeviceHandle
    );

NTSTATUS DiskDriveQueryStatistics(
    _In_ HANDLE DeviceHandle,
    _Out_ PDISK_PERFORMANCE Info
    );

// disknotify.c

VOID SetupDeviceChangeCallback(
    VOID
    );

#endif _DEVICES_H_