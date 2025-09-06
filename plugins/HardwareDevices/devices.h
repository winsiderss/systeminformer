/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2015-2024
 *     jxy-s   2022
 *
 */

#ifndef _DEVICES_H_
#define _DEVICES_H_

#define PLUGIN_NAME L"ProcessHacker.HardwareDevices"
#define SETTING_NAME_ENABLE_NDIS (PLUGIN_NAME L".EnableNDIS")
#define SETTING_NAME_INTERFACE_LIST (PLUGIN_NAME L".NetworkList")
#define SETTING_NAME_NETWORK_POSITION (PLUGIN_NAME L".NetworkWindowPosition")
#define SETTING_NAME_NETWORK_SIZE (PLUGIN_NAME L".NetworkWindowSize")
#define SETTING_NAME_NETWORK_COLUMNS (PLUGIN_NAME L".NetworkListColumns")
#define SETTING_NAME_NETWORK_SORTCOLUMN (PLUGIN_NAME L".NetworkListSort")
#define SETTING_NAME_DISK_LIST (PLUGIN_NAME L".DiskList")
#define SETTING_NAME_DISK_POSITION (PLUGIN_NAME L".DiskWindowPosition")
#define SETTING_NAME_DISK_SIZE (PLUGIN_NAME L".DiskWindowSize")
#define SETTING_NAME_DISK_COUNTERS_COLUMNS (PLUGIN_NAME L".DiskListColumns")
#define SETTING_NAME_SMART_COUNTERS_COLUMNS (PLUGIN_NAME L".SmartListColumns")
#define SETTING_NAME_RAPL_LIST (PLUGIN_NAME L".RaplList")
#define SETTING_NAME_GRAPHICS_LIST (PLUGIN_NAME L".GraphicsList")
#define SETTING_NAME_GRAPHICS_DETAILS_WINDOW_POSITION (PLUGIN_NAME L".GraphicsDetailsWindowPosition")
#define SETTING_NAME_GRAPHICS_DETAILS_WINDOW_SIZE (PLUGIN_NAME L".GraphicsDetailsWindowSize")
#define SETTING_NAME_GRAPHICS_NODES_WINDOW_POSITION (PLUGIN_NAME L".GraphicsNodesWindowPosition")
#define SETTING_NAME_GRAPHICS_NODES_WINDOW_SIZE (PLUGIN_NAME L".GraphicsNodesWindowSize")
#define SETTING_NAME_GRAPHICS_UNIQUE_INDICES (PLUGIN_NAME L".GraphicsUniqueIndices")
#define SETTING_NAME_DEVICE_TREE_AUTO_REFRESH (PLUGIN_NAME L".DeviceTreeAutoRefresh")
#define SETTING_NAME_DEVICE_TREE_SHOW_DISCONNECTED (PLUGIN_NAME L".DeviceTreeShowDisconnected")
#define SETTING_NAME_DEVICE_TREE_HIGHLIGHT_UPPER_FILTERED (PLUGIN_NAME L".DeviceTreeHighlightUpperFiltered")
#define SETTING_NAME_DEVICE_TREE_HIGHLIGHT_LOWER_FILTERED (PLUGIN_NAME L".DeviceTreeHighlightLowerFiltered")
#define SETTING_NAME_DEVICE_TREE_SORT (PLUGIN_NAME L".DeviceTreeSort")
#define SETTING_NAME_DEVICE_TREE_COLUMNS (PLUGIN_NAME L".DeviceTreeColumns")
#define SETTING_NAME_DEVICE_PROBLEM_COLOR (PLUGIN_NAME L".ColorDeviceProblem")
#define SETTING_NAME_DEVICE_DISABLED_COLOR (PLUGIN_NAME L".ColorDeviceDisabled")
#define SETTING_NAME_DEVICE_DISCONNECTED_COLOR (PLUGIN_NAME L".ColorDeviceDisconnected")
#define SETTING_NAME_DEVICE_HIGHLIGHT_COLOR (PLUGIN_NAME L".ColorDeviceHighlight")
#define SETTING_NAME_DEVICE_INTERFACE_COLOR (PLUGIN_NAME L".ColorDeviceInterface")
#define SETTING_NAME_DEVICE_DISABLED_INTERFACE_COLOR (PLUGIN_NAME L".ColorDisabledDeviceInterface")
#define SETTING_NAME_DEVICE_SORT_CHILDREN_BY_NAME (PLUGIN_NAME L".SortDeviceChildrenByName")
#define SETTING_NAME_DEVICE_SHOW_ROOT (PLUGIN_NAME L".ShowRootDevice")
#define SETTING_NAME_DEVICE_SHOW_SOFTWARE_COMPONENTS (PLUGIN_NAME L".ShowSoftwareComponents")
#define SETTING_NAME_DEVICE_SHOW_DEVICE_INTERFACES (PLUGIN_NAME L".ShowDeviceInterfaces")
#define SETTING_NAME_DEVICE_SHOW_DISABLED_DEVICE_INTERFACES (PLUGIN_NAME L".ShowDisabledDeviceInterfaces")
#define SETTING_NAME_DEVICE_PROPERTIES_POSITION (PLUGIN_NAME L".DevicePropertiesPosition")
#define SETTING_NAME_DEVICE_PROPERTIES_SIZE (PLUGIN_NAME L".DevicePropertiesSize")
#define SETTING_NAME_DEVICE_GENERAL_COLUMNS (PLUGIN_NAME L".DeviceGeneralColumns")
#define SETTING_NAME_DEVICE_PROPERTIES_COLUMNS (PLUGIN_NAME L".DevicePropertiesColumns")
#define SETTING_NAME_DEVICE_INTERFACES_COLUMNS (PLUGIN_NAME L".DeviceInterfacesColumns")
#define SETTING_NAME_DEVICE_ARRIVED_COLOR (PLUGIN_NAME L".ColorDeviceArrived")
#define SETTING_NAME_DEVICE_HIGHLIGHTING_DURATION (PLUGIN_NAME L".DeviceHighlightingDuration")
#define SETTING_NAME_DEVICE_SORT_CHILD_DEVICES (PLUGIN_NAME L".SortChildDevices")
#define SETTING_NAME_DEVICE_SORT_ROOT_DEVICES (PLUGIN_NAME L".SortRootDevices")

#include <phdk.h>
#include <phappresource.h>
#include <settings.h>
#include <searchbox.h>
#include <workqueue.h>
#include <mapldr.h>

#include <cfgmgr32.h>
#include <nvme.h>

#include <phnet.h>

// d3dkmddi requires the WDK (dmex)
#if defined(NTDDI_WIN10_CO) && (NTDDI_VERSION >= NTDDI_WIN10_CO)
#ifdef __has_include
#if __has_include (<dxmini.h>) && \
__has_include (<d3dkmddi.h>) && \
__has_include (<d3dkmthk.h>)
#include <dxmini.h>
#include <d3dkmddi.h>
#include <d3dkmthk.h>
#else
#include "d3dkmt/d3dkmthk.h"
#endif
#else
#include "d3dkmt/d3dkmthk.h"
#endif
#else
#include "d3dkmt/d3dkmthk.h"
#endif

#include "resource.h"
#include "prpsh.h"

extern PPH_PLUGIN PluginInstance;
extern BOOLEAN NetAdapterEnableNdis;
extern ULONG NetWindowsVersion;
extern ULONG NetUpdateInterval;

extern PPH_OBJECT_TYPE NetworkDeviceEntryType;
extern PPH_LIST NetworkDevicesList;
extern PH_QUEUED_LOCK NetworkDevicesListLock;

extern PPH_OBJECT_TYPE DiskDeviceEntryType;
extern PPH_LIST DiskDevicesList;
extern PH_QUEUED_LOCK DiskDevicesListLock;

extern PPH_OBJECT_TYPE RaplDeviceEntryType;
extern PPH_LIST RaplDevicesList;
extern PH_QUEUED_LOCK RaplDevicesListLock;

extern PPH_OBJECT_TYPE GraphicsDeviceEntryType;
extern PPH_LIST GraphicsDevicesList;
extern PH_QUEUED_LOCK GraphicsDevicesListLock;

#ifdef _DEBUG
//#define FORCE_DELAY_LABEL_WORKQUEUE
#endif

// main.c

BOOLEAN HardwareDeviceEnableDisable(
    _In_ HWND ParentWindow,
    _In_ PPH_STRING DeviceInstance,
    _In_ BOOLEAN Enable
    );

BOOLEAN HardwareDeviceRestart(
    _In_ HWND ParentWindow,
    _In_ PPH_STRING DeviceInstance
    );

BOOLEAN HardwareDeviceUninstall(
    _In_ HWND ParentWindow,
    _In_ PPH_STRING DeviceInstance
    );

_Success_(return)
BOOLEAN HardwareDeviceShowSecurity(
    _In_ HWND ParentWindow,
    _In_ PPH_STRING DeviceInstance
    );

BOOLEAN HardwareDeviceShowProperties(
    _In_ HWND WindowHandle,
    _In_ PPH_STRING DeviceInstance
    );

#define HW_KEY_INDEX_HARDWARE 4
#define HW_KEY_INDEX_SOFTWARE 5
#define HW_KEY_INDEX_USER 6
#define HW_KEY_INDEX_CONFIG 7

BOOLEAN HardwareDeviceOpenKey(
    _In_ HWND ParentWindow,
    _In_ PPH_STRING DeviceInstance,
    _In_ ULONG KeyIndex
    );

VOID ShowDeviceMenu(
    _In_ HWND ParentWindow,
    _In_ PPH_STRING DeviceInstance
    );

// adapter.c

typedef struct _DV_NETADAPTER_ID
{
    NET_IFINDEX InterfaceIndex;
    IF_LUID InterfaceLuid; // NET_LUID
    GUID InterfaceGuid;
    PPH_STRING InterfaceGuidString;
    PPH_STRING InterfacePath;
} DV_NETADAPTER_ID, *PDV_NETADAPTER_ID;

typedef struct _DV_NETADAPTER_ENTRY
{
    DV_NETADAPTER_ID AdapterId;
    PPH_STRING AdapterName;
    PPH_STRING AdapterAlias;

    union
    {
        BOOLEAN BitField;
        struct
        {
            BOOLEAN UserReference : 1;
            BOOLEAN HaveFirstSample : 1;
            BOOLEAN CheckedDeviceSupport : 1;
            BOOLEAN DeviceSupported : 1;
            BOOLEAN DevicePresent : 1;
            BOOLEAN PendingQuery : 1;
            BOOLEAN Spare : 2;
        };
    };

    ULONG64 NetworkReceiveRaw;
    ULONG64 NetworkSendRaw;
    ULONG64 CurrentNetworkReceive;
    ULONG64 CurrentNetworkSend;

    PH_UINT64_DELTA NetworkReceiveDelta;
    PH_UINT64_DELTA NetworkSendDelta;

    PH_CIRCULAR_BUFFER_ULONG64 InboundBuffer;
    PH_CIRCULAR_BUFFER_ULONG64 OutboundBuffer;

    volatile LONG JustProcessed;
} DV_NETADAPTER_ENTRY, *PDV_NETADAPTER_ENTRY;

typedef struct _DV_NETADAPTER_SYSINFO_CONTEXT
{
    PDV_NETADAPTER_ENTRY AdapterEntry;

    HWND WindowHandle;
    HWND PanelWindowHandle;

    HWND LabelSendHandle;
    HWND LabelReceiveHandle;
    HWND GroupSendHandle;
    HWND GroupReceiveHandle;
    HWND GraphSendHandle;
    HWND GraphReceiveHandle;

    HANDLE DetailsWindowThreadHandle;
    HWND DetailsWindowDialogHandle;
    PH_EVENT DetailsWindowInitializedEvent;

    PPH_SYSINFO_SECTION SysinfoSection;
    PH_LAYOUT_MANAGER LayoutManager;
    RECT GraphMargin;
    LONG GraphPadding;

    PH_GRAPH_STATE GraphSendState;
    PH_GRAPH_STATE GraphReceiveState;

    HWND AdapterNameLabel;
    HWND AdapterTextLabel;
    HWND NetAdapterPanelSentLabel;
    HWND NetAdapterPanelReceivedLabel;
    HWND NetAdapterPanelTotalLabel;
    HWND NetAdapterPanelStateLabel;
    HWND NetAdapterPanelSpeedLabel;
    HWND NetAdapterPanelBytesLabel;
} DV_NETADAPTER_SYSINFO_CONTEXT, *PDV_NETADAPTER_SYSINFO_CONTEXT;

typedef struct _DV_NETADAPTER_DETAILS_CONTEXT
{
    PPH_STRING AdapterName;
    DV_NETADAPTER_ID AdapterId;

    union
    {
        BOOLEAN BitField;
        struct
        {
            BOOLEAN HaveFirstSample : 1;
            BOOLEAN CheckedDeviceSupport : 1;
            BOOLEAN DeviceSupported : 1;
            BOOLEAN Spare : 5;
        };
    };

    HWND WindowHandle;
    HWND ParentHandle;
    HWND ListViewHandle;

    HANDLE NotifyHandle;
    PDV_NETADAPTER_SYSINFO_CONTEXT SysInfoContext;

    PH_LAYOUT_MANAGER LayoutManager;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

    ULONG64 LastDetailsInboundValue;
    ULONG64 LastDetailsIOutboundValue;
} DV_NETADAPTER_DETAILS_CONTEXT, *PDV_NETADAPTER_DETAILS_CONTEXT;

typedef struct _DV_NETADAPTER_CONTEXT
{
    HWND ListViewHandle;

    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN OptionsChanged : 1;
            BOOLEAN UseAlternateMethod : 1;
            BOOLEAN ShowHardwareAdapters : 1;
            BOOLEAN Spare : 5;
        };
    };

    PH_LAYOUT_MANAGER LayoutManager;
    IListView* ListViewClass;
} DV_NETADAPTER_CONTEXT, *PDV_NETADAPTER_CONTEXT;

VOID NetAdaptersLoadList(
    VOID
    );

// adapter.c

VOID NetworkDevicesInitialize(
    VOID
    );

VOID NetworkDevicesUpdate(
    _In_ ULONG RunCount
    );

VOID NetworkDeviceUpdateDeviceInfo(
    _In_opt_ HANDLE DeviceHandle,
    _In_ PDV_NETADAPTER_ENTRY AdapterEntry
    );

VOID InitializeNetAdapterId(
    _Out_ PDV_NETADAPTER_ID Id,
    _In_ NET_IFINDEX InterfaceIndex,
    _In_ IF_LUID InterfaceLuid,
    _In_ PPH_STRING InterfaceGuidString
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

    NETADAPTER_DETAILS_INDEX_IPV4ADDRESS,
    NETADAPTER_DETAILS_INDEX_IPV4SUBNET,
    NETADAPTER_DETAILS_INDEX_IPV4GATEWAY,
    NETADAPTER_DETAILS_INDEX_IPV4DNS,

    NETADAPTER_DETAILS_INDEX_IPV6ADDRESS,
    NETADAPTER_DETAILS_INDEX_IPV6TEMPADDRESS,
    NETADAPTER_DETAILS_INDEX_IPV6GATEWAY,
    NETADAPTER_DETAILS_INDEX_IPV6DNS,

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

VOID ShowNetAdapterDetailsDialog(
    _In_ PDV_NETADAPTER_SYSINFO_CONTEXT Context
    );

// ndis.c

#define BITS_IN_ONE_BYTE 8
#define NDIS_UNIT_OF_MEASUREMENT 100

BOOLEAN NetworkAdapterQuerySupported(
    _In_ HANDLE DeviceHandle
    );

_Success_(return)
BOOLEAN NetworkAdapterQueryNdisVersion(
    _In_ HANDLE DeviceHandle,
    _Out_opt_ PUINT MajorVersion,
    _Out_opt_ PUINT MinorVersion
    );

PPH_STRING NetworkAdapterQueryNameFromInterfaceGuid(
    _In_ PGUID InterfaceGuid
    );

PPH_STRING NetworkAdapterQueryNameFromDeviceGuid(
    _In_ PGUID InterfaceGuid
    );

PPH_STRING NetworkAdapterGetInterfaceAliasFromLuid(
    _In_ PDV_NETADAPTER_ID Id
    );

PPH_STRING NetworkAdapterQueryName(
    _In_ HANDLE DeviceHandle
    );

NTSTATUS NetworkAdapterQueryStatistics(
    _In_ HANDLE DeviceHandle,
    _Out_ PNDIS_STATISTICS_INFO Info
    );

NTSTATUS NetworkAdapterQueryLinkState(
    _In_ HANDLE DeviceHandle,
    _Out_ PNDIS_LINK_STATE State
    );

_Success_(return)
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

_Success_(return)
BOOLEAN NetworkAdapterQueryInterfaceRow(
    _In_ PDV_NETADAPTER_ID Id,
    _In_ MIB_IF_ENTRY_LEVEL Level,
    _Out_ PMIB_IF_ROW2 InterfaceRow
    );

PCWSTR MediumTypeToString(
    _In_ NDIS_PHYSICAL_MEDIUM MediumType
    );

PPH_STRING NetAdapterFormatBitratePrefix(
    _In_ ULONG64 Value
    );

// netoptions.c

INT_PTR CALLBACK NetworkAdapterOptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// diskoptions.c

INT_PTR CALLBACK DiskDriveOptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// disk.c

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
        BOOLEAN BitField;
        struct
        {
            BOOLEAN UserReference : 1;
            BOOLEAN HaveFirstSample : 1;
            BOOLEAN DevicePresent : 1;
            BOOLEAN PendingQuery : 1;
            BOOLEAN Spare : 4;
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

    volatile LONG JustProcessed;
} DV_DISK_ENTRY, *PDV_DISK_ENTRY;

typedef struct _DV_DISK_SYSINFO_CONTEXT
{
    PDV_DISK_ENTRY DiskEntry;

    HWND WindowHandle;
    HWND PanelWindowHandle;

    HWND LabelReadHandle;
    HWND LabelWriteHandle;
    HWND GroupReadHandle;
    HWND GroupWriteHandle;
    HWND GraphReadHandle;
    HWND GraphWriteHandle;

    HANDLE DetailsWindowThreadHandle;
    HWND DetailsWindowDialogHandle;
    PH_EVENT DetailsWindowInitializedEvent;

    PPH_SYSINFO_SECTION SysinfoSection;
    PH_LAYOUT_MANAGER LayoutManager;
    RECT GraphMargin;
    LONG GraphPadding;

    PH_GRAPH_STATE GraphReadState;
    PH_GRAPH_STATE GraphWriteState;

    HWND DiskPathLabel;
    HWND DiskNameLabel;
    HWND DiskDevicePanelReadLabel;
    HWND DiskDevicePanelWriteLabel;
    HWND DiskDevicePanelTotalLabel;
    HWND DiskDevicePanelActiveLabel;
    HWND DiskDevicePanelTimeLabel;
    HWND DiskDevicePanelBytesLabel;
} DV_DISK_SYSINFO_CONTEXT, *PDV_DISK_SYSINFO_CONTEXT;

typedef struct _DV_DISK_OPTIONS_CONTEXT
{
    HWND ListViewHandle;
    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN OptionsChanged : 1;
            BOOLEAN UseAlternateMethod : 1;
            BOOLEAN Spare : 6;
        };
    };
    PH_LAYOUT_MANAGER LayoutManager;
} DV_DISK_OPTIONS_CONTEXT, *PDV_DISK_OPTIONS_CONTEXT;

VOID DiskDevicesInitialize(VOID);
VOID DiskDrivesLoadList(VOID);
VOID DiskDevicesUpdate(_In_ ULONG RunCount);

VOID DiskDeviceUpdateDeviceInfo(
    _In_opt_ HANDLE DeviceHandle,
    _In_ PDV_DISK_ENTRY DiskEntry
    );

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

// diskdetails.c

typedef struct _COMMON_PAGE_CONTEXT
{
    //HWND ParentHandle;

    PPH_STRING DiskName;
    DV_DISK_ID DiskId;
    ULONG DiskIndex;
    HANDLE DeviceHandle;
    PDV_DISK_SYSINFO_CONTEXT SysInfoContext;
} COMMON_PAGE_CONTEXT, *PCOMMON_PAGE_CONTEXT;

typedef struct _DV_DISK_PAGE_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;

    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

    PCOMMON_PAGE_CONTEXT PageContext;
} DV_DISK_PAGE_CONTEXT, *PDV_DISK_PAGE_CONTEXT;

typedef enum _DISKDRIVE_DETAILS_INDEX
{
    DISKDRIVE_DETAILS_INDEX_FS_CREATION_TIME,
    DISKDRIVE_DETAILS_INDEX_SERIAL_NUMBER,
    DISKDRIVE_DETAILS_INDEX_UNIQUEID,
    DISKDRIVE_DETAILS_INDEX_PARTITIONID,
    DISKDRIVE_DETAILS_INDEX_FILE_SYSTEM,
    DISKDRIVE_DETAILS_INDEX_FS_VERSION,
    DISKDRIVE_DETAILS_INDEX_LFS_VERSION,

    DISKDRIVE_DETAILS_INDEX_TOTAL_SIZE,
    DISKDRIVE_DETAILS_INDEX_TOTAL_FREE,
    DISKDRIVE_DETAILS_INDEX_TOTAL_USED,
    DISKDRIVE_DETAILS_INDEX_TOTAL_SECTORS,
    DISKDRIVE_DETAILS_INDEX_TOTAL_CLUSTERS,
    DISKDRIVE_DETAILS_INDEX_FREE_CLUSTERS,
    DISKDRIVE_DETAILS_INDEX_TOTAL_RESERVED,
    DISKDRIVE_DETAILS_INDEX_TOTAL_BYTES_PER_SECTOR,
    DISKDRIVE_DETAILS_INDEX_TOTAL_BYTES_PER_CLUSTER,
    DISKDRIVE_DETAILS_INDEX_TOTAL_BYTES_PER_RECORD,
    DISKDRIVE_DETAILS_INDEX_TOTAL_CLUSTERS_PER_RECORD,

    DISKDRIVE_DETAILS_INDEX_MFT_RECORDS,
    DISKDRIVE_DETAILS_INDEX_MFT_SIZE,
    DISKDRIVE_DETAILS_INDEX_MFT_START,
    DISKDRIVE_DETAILS_INDEX_MFT_ZONE,
    DISKDRIVE_DETAILS_INDEX_MFT_ZONE_SIZE,
    DISKDRIVE_DETAILS_INDEX_MFT_MIRROR_START,

    DISKDRIVE_DETAILS_INDEX_FILE_READS,
    DISKDRIVE_DETAILS_INDEX_FILE_WRITES,
    DISKDRIVE_DETAILS_INDEX_DISK_READS,
    DISKDRIVE_DETAILS_INDEX_DISK_WRITES,
    DISKDRIVE_DETAILS_INDEX_FILE_READ_BYTES,
    DISKDRIVE_DETAILS_INDEX_FILE_WRITE_BYTES,

    DISKDRIVE_DETAILS_INDEX_METADATA_READS,
    DISKDRIVE_DETAILS_INDEX_METADATA_WRITES,
    DISKDRIVE_DETAILS_INDEX_METADATA_DISK_READS,
    DISKDRIVE_DETAILS_INDEX_METADATA_DISK_WRITES,
    DISKDRIVE_DETAILS_INDEX_METADATA_READ_BYTES,
    DISKDRIVE_DETAILS_INDEX_METADATA_WRITE_BYTES,

    DISKDRIVE_DETAILS_INDEX_MFT_READS,
    DISKDRIVE_DETAILS_INDEX_MFT_WRITES,
    DISKDRIVE_DETAILS_INDEX_MFT_READ_BYTES,
    DISKDRIVE_DETAILS_INDEX_MFT_WRITE_BYTES,

    //DISKDRIVE_DETAILS_INDEX_ROOT_INDEX_READS,
    //DISKDRIVE_DETAILS_INDEX_ROOT_INDEX_WRITES,
    //DISKDRIVE_DETAILS_INDEX_ROOT_INDEX_READ_BYTES,
    //DISKDRIVE_DETAILS_INDEX_ROOT_INDEX_WRITE_BYTES,

    /*DISKDRIVE_DETAILS_INDEX_BITMAP_READS,
    DISKDRIVE_DETAILS_INDEX_BITMAP_WRITES,
    DISKDRIVE_DETAILS_INDEX_BITMAP_READ_BYTES,
    DISKDRIVE_DETAILS_INDEX_BITMAP_WRITE_BYTES,

    DISKDRIVE_DETAILS_INDEX_MFT_BITMAP_READS,
    DISKDRIVE_DETAILS_INDEX_MFT_BITMAP_WRITES,
    DISKDRIVE_DETAILS_INDEX_MFT_BITMAP_READ_BYTES,
    DISKDRIVE_DETAILS_INDEX_MFT_BITMAP_WRITE_BYTES,

    DISKDRIVE_DETAILS_INDEX_USER_INDEX_READS,
    DISKDRIVE_DETAILS_INDEX_USER_INDEX_WRITES,
    DISKDRIVE_DETAILS_INDEX_USER_INDEX_READ_BYTES,
    DISKDRIVE_DETAILS_INDEX_USER_INDEX_WRITE_BYTES,

    DISKDRIVE_DETAILS_INDEX_LOGFILE_READS,
    DISKDRIVE_DETAILS_INDEX_LOGFILE_WRITES,
    DISKDRIVE_DETAILS_INDEX_LOGFILE_READ_BYTES,
    DISKDRIVE_DETAILS_INDEX_LOGFILE_WRITE_BYTES,

    DISKDRIVE_DETAILS_INDEX_MFT_USER_LEVEL_WRITE,
    DISKDRIVE_DETAILS_INDEX_MFT_USER_LEVEL_CREATE,
    DISKDRIVE_DETAILS_INDEX_MFT_USER_LEVEL_SETINFO,
    DISKDRIVE_DETAILS_INDEX_MFT_USER_LEVEL_FLUSH,

    DISKDRIVE_DETAILS_INDEX_MFT_WRITES_FLUSH_LOGFILE,
    DISKDRIVE_DETAILS_INDEX_MFT_WRITES_LAZY_WRITER,
    DISKDRIVE_DETAILS_INDEX_MFT_WRITES_USER_REQUEST,

    DISKDRIVE_DETAILS_INDEX_MFT2_WRITES,
    DISKDRIVE_DETAILS_INDEX_MFT2_WRITE_BYTES,

    DISKDRIVE_DETAILS_INDEX_MFT2_USER_LEVEL_WRITE,
    DISKDRIVE_DETAILS_INDEX_MFT2_USER_LEVEL_CREATE,
    DISKDRIVE_DETAILS_INDEX_MFT2_USER_LEVEL_SETINFO,
    DISKDRIVE_DETAILS_INDEX_MFT2_USER_LEVEL_FLUSH,

    DISKDRIVE_DETAILS_INDEX_MFT2_WRITES_FLUSH_LOGFILE,
    DISKDRIVE_DETAILS_INDEX_MFT2_WRITES_LAZY_WRITER,
    DISKDRIVE_DETAILS_INDEX_MFT2_WRITES_USER_REQUEST,

    DISKDRIVE_DETAILS_INDEX_BITMAP_WRITES_FLUSH_LOGFILE,
    DISKDRIVE_DETAILS_INDEX_BITMAP_WRITES_LAZY_WRITER,
    DISKDRIVE_DETAILS_INDEX_BITMAP_WRITES_USER_REQUEST,

    DISKDRIVE_DETAILS_INDEX_BITMAP_WRITES_WRITE,
    DISKDRIVE_DETAILS_INDEX_BITMAP_WRITES_CREATE,
    DISKDRIVE_DETAILS_INDEX_BITMAP_WRITES_SETINFO,

    DISKDRIVE_DETAILS_INDEX_MFT_BITMAP_FLUSH_LOGFILE,
    DISKDRIVE_DETAILS_INDEX_MFT_BITMAP_LAZY_WRITER,
    DISKDRIVE_DETAILS_INDEX_MFT_BITMAP_USER_REQUEST,

    DISKDRIVE_DETAILS_INDEX_MFT_BITMAP_USER_LEVEL_WRITE,
    DISKDRIVE_DETAILS_INDEX_MFT_BITMAP_USER_LEVEL_CREATE,
    DISKDRIVE_DETAILS_INDEX_MFT_BITMAP_USER_LEVEL_SETINFO,
    DISKDRIVE_DETAILS_INDEX_MFT_BITMAP_USER_LEVEL_FLUSH,

    DISKDRIVE_DETAILS_INDEX_ALLOCATE_CALLS,
    DISKDRIVE_DETAILS_INDEX_ALLOCATE_CLUSTERS,
    DISKDRIVE_DETAILS_INDEX_ALLOCATE_HINTS,
    DISKDRIVE_DETAILS_INDEX_ALLOCATE_RUNS_RETURNED,
    DISKDRIVE_DETAILS_INDEX_ALLOCATE_HITS_HONORED,
    DISKDRIVE_DETAILS_INDEX_ALLOCATE_HITS_CLUSTERS,
    DISKDRIVE_DETAILS_INDEX_ALLOCATE_CACHE,
    DISKDRIVE_DETAILS_INDEX_ALLOCATE_CACHE_CLUSTERS,
    DISKDRIVE_DETAILS_INDEX_ALLOCATE_CACHE_MISS,
    DISKDRIVE_DETAILS_INDEX_ALLOCATE_CACHE_MISS_CLUSTERS,*/

    DISKDRIVE_DETAILS_INDEX_OTHER_EXCEPTIONS,

    DISKDRIVE_DETAILS_INDEX_RESOURCES_EXHAUSTED,
    DISKDRIVE_DETAILS_INDEX_VOLUME_TRIM_COUNT,
    DISKDRIVE_DETAILS_INDEX_VOLUME_TRIM_TIME,
    DISKDRIVE_DETAILS_INDEX_VOLUME_TRIM_BYTES,
    DISKDRIVE_DETAILS_INDEX_VOLUME_TRIM_SKIPPED_COUNT,
    DISKDRIVE_DETAILS_INDEX_VOLUME_TRIM_SKIPPED_BYTES,
    DISKDRIVE_DETAILS_INDEX_FILE_TRIM_COUNT,
    DISKDRIVE_DETAILS_INDEX_FILE_TRIM_TIME,
    DISKDRIVE_DETAILS_INDEX_FILE_TRIM_BYTES,
} DISKDRIVE_DETAILS_INDEX;

VOID ShowDiskDeviceDetailsDialog(
    _In_ PDV_DISK_SYSINFO_CONTEXT Context
    );

// disknotify.c

VOID AddRemoveDeviceChangeCallback(
    VOID
    );

// storage.c

PPH_STRING DiskDriveQueryDosMountPoints(
    _In_ ULONG DeviceNumber
    );

typedef struct _DISK_HANDLE_ENTRY
{
    WCHAR DeviceLetter;
    HANDLE DeviceHandle;
} DISK_HANDLE_ENTRY, *PDISK_HANDLE_ENTRY;

PPH_LIST DiskDriveQueryMountPointHandles(
    _In_ ULONG DeviceNumber
    );

_Success_(return)
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

NTSTATUS DiskDriveQueryStatistics(
    _In_ HANDLE DeviceHandle,
    _Out_ PDISK_PERFORMANCE Info
    );

PPH_STRING DiskDriveQueryGeometry(
    _In_ HANDLE DeviceHandle
    );

NTSTATUS DiskDriveQueryImminentFailure(
    _In_ HANDLE DeviceHandle,
    _Out_ PPH_LIST* DiskSmartAttributes
    );

NTSTATUS DiskDriveQueryNvmeHealthInfo(
    _In_ HANDLE DeviceHandle,
    _Out_ PNVME_HEALTH_INFO_LOG HealthInfo
    );

typedef DECLSPEC_ALIGN(64) struct _NTFS_FILESYSTEM_STATISTICS
{
    FILESYSTEM_STATISTICS FileSystemStatistics;
    NTFS_STATISTICS NtfsStatistics;
} NTFS_FILESYSTEM_STATISTICS, *PNTFS_FILESYSTEM_STATISTICS;

typedef DECLSPEC_ALIGN(64) struct _FAT_FILESYSTEM_STATISTICS
{
    FILESYSTEM_STATISTICS FileSystemStatistics;
    NTFS_STATISTICS FatStatistics;
} FAT_FILESYSTEM_STATISTICS, *PFAT_FILESYSTEM_STATISTICS;

typedef DECLSPEC_ALIGN(64) struct _EXFAT_FILESYSTEM_STATISTICS
{
    FILESYSTEM_STATISTICS FileSystemStatistics;
    EXFAT_STATISTICS ExFatStatistics;
} EXFAT_FILESYSTEM_STATISTICS, *PEXFAT_FILESYSTEM_STATISTICS;

_Success_(return)
BOOLEAN DiskDriveQueryFileSystemInfo(
    _In_ HANDLE DeviceHandle,
    _Out_ USHORT* FileSystemType,
    _Out_ PVOID* FileSystemStatistics
    );

typedef DECLSPEC_ALIGN(64) struct _NTFS_FILESYSTEM_STATISTICS_EX
{
    FILESYSTEM_STATISTICS_EX FileSystemStatistics;
    NTFS_STATISTICS_EX NtfsStatistics;
} NTFS_FILESYSTEM_STATISTICS_EX, *PNTFS_FILESYSTEM_STATISTICS_EX;

typedef DECLSPEC_ALIGN(64) struct _FAT_FILESYSTEM_STATISTICS_EX
{
    FILESYSTEM_STATISTICS_EX FileSystemStatistics;
    NTFS_STATISTICS_EX FatStatistics;
} FAT_FILESYSTEM_STATISTICS_EX, *PFAT_FILESYSTEM_STATISTICS_EX;

typedef DECLSPEC_ALIGN(64) struct _EXFAT_FILESYSTEM_STATISTICS_EX
{
    FILESYSTEM_STATISTICS_EX FileSystemStatistics;
    EXFAT_STATISTICS ExFatStatistics;
} EXFAT_FILESYSTEM_STATISTICS_EX, *PEXFAT_FILESYSTEM_STATISTICS_EX;

_Success_(return)
BOOLEAN DiskDriveQueryFileSystemInfoEx(
    _In_ HANDLE DosDeviceHandle,
    _Out_ PUSHORT FileSystemType,
    _Out_ PVOID* FileSystemStatistics
    );

typedef struct _NTFS_VOLUME_INFO
{
    NTFS_VOLUME_DATA_BUFFER VolumeData;
    NTFS_EXTENDED_VOLUME_DATA ExtendedVolumeData;
} NTFS_VOLUME_INFO, *PNTFS_VOLUME_INFO;

_Success_(return)
BOOLEAN DiskDriveQueryNtfsVolumeInfo(
    _In_ HANDLE DosDeviceHandle,
    _Out_ PNTFS_VOLUME_INFO VolumeInfo
    );

_Success_(return)
BOOLEAN DiskDriveQueryRefsVolumeInfo(
    _In_ HANDLE DosDeviceHandle,
    _Out_ PREFS_VOLUME_DATA_BUFFER VolumeInfo
    );

NTSTATUS DiskDriveQueryVolumeInformation(
    _In_ HANDLE DosDeviceHandle,
    _Out_ PFILE_FS_VOLUME_INFORMATION* VolumeInfo
    );

NTSTATUS DiskDriveQueryUniqueId(
    _In_ HANDLE DeviceHandle,
    _Out_ PPH_STRING* UniqueId,
    _Out_ PPH_STRING* PartitionId
    );

// https://en.wikipedia.org/wiki/Self-Monitoring,_Analysis_and_Reporting_Technology#Known_ATA_S.M.A.R.T._attributes
typedef enum _SMART_ATTRIBUTE_ID
{
    SMART_ATTRIBUTE_ID_READ_ERROR_RATE = 0x01,
    SMART_ATTRIBUTE_ID_THROUGHPUT_PERFORMANCE = 0x02,
    SMART_ATTRIBUTE_ID_SPIN_UP_TIME = 0x03,
    SMART_ATTRIBUTE_ID_START_STOP_COUNT = 0x04,
    SMART_ATTRIBUTE_ID_REALLOCATED_SECTORS_COUNT = 0x05,
    SMART_ATTRIBUTE_ID_READ_CHANNEL_MARGIN = 0x06,
    SMART_ATTRIBUTE_ID_SEEK_ERROR_RATE = 0x07,
    SMART_ATTRIBUTE_ID_SEEK_TIME_PERFORMANCE = 0x08,
    SMART_ATTRIBUTE_ID_POWER_ON_HOURS = 0x09,
    SMART_ATTRIBUTE_ID_SPIN_RETRY_COUNT = 0x0A,
    SMART_ATTRIBUTE_ID_CALIBRATION_RETRY_COUNT = 0x0B,
    SMART_ATTRIBUTE_ID_POWER_CYCLE_COUNT = 0x0C,
    SMART_ATTRIBUTE_ID_SOFT_READ_ERROR_RATE = 0x0D,
    // TODO: Add values 14-21
    SMART_ATTRIBUTE_ID_CURRENT_HELIUM_LEVEL = 0x16,
    SMART_ATTRIBUTE_ID_HELIUM_CONDITION_LOWER = 0x17,
    SMART_ATTRIBUTE_ID_HELIUM_CONDITION_UPPER = 0x18,
    // TODO: Add values 25-169
    SMART_ATTRIBUTE_ID_AVAILABLE_RESERVED_SPACE = 0xAA,
    SMART_ATTRIBUTE_ID_SSD_PROGRAM_FAIL_COUNT = 0xAB,
    SMART_ATTRIBUTE_ID_SSD_ERASE_FAIL_COUNT = 0xAC,
    SMART_ATTRIBUTE_ID_SSD_WEAR_LEVELING_COUNT = 0xAD,
    SMART_ATTRIBUTE_ID_UNEXPECTED_POWER_LOSS = 0xAE,
    SMART_ATTRIBUTE_ID_POWER_LOSS_PROTECTION_FAILURE = 0xAF,
    SMART_ATTRIBUTE_ID_ERASE_FAIL_COUNT = 0xB0,
    SMART_ATTRIBUTE_ID_WEAR_RANGE_DELTA = 0xB1,
    SMART_ATTRIBUTE_ID_USED_RESERVED_BLOCK_COUNT = 0xB2,
    SMART_ATTRIBUTE_ID_USED_RESERVED_BLOCK_TOTAL = 0xB3,
    SMART_ATTRIBUTE_ID_UNUSED_RESERVED_BLOCK_TOTAL = 0xB4,
    SMART_ATTRIBUTE_ID_SSD_PROGRAM_FAIL_COUNT_TOTAL = 0xB5,
    SMART_ATTRIBUTE_ID_ERASE_FAIL_COUNT_SAMSUNG = 0xB6,
    SMART_ATTRIBUTE_ID_SATA_DOWNSHIFT_ERROR_COUNT = 0xB7,
    SMART_ATTRIBUTE_ID_END_TO_END_ERROR = 0xB8,
    SMART_ATTRIBUTE_ID_HEAD_STABILITY = 0xB9,
    SMART_ATTRIBUTE_ID_INDUCED_OP_VIBRATION_DETECTION = 0xBA,
    SMART_ATTRIBUTE_ID_REPORTED_UNCORRECTABLE_ERRORS = 0xBB,
    SMART_ATTRIBUTE_ID_COMMAND_TIMEOUT = 0xBC,
    SMART_ATTRIBUTE_ID_HIGH_FLY_WRITES = 0xBD,
    SMART_ATTRIBUTE_ID_TEMPERATURE_DIFFERENCE_FROM_100 = 0xBE, // AirflowTemperature
    SMART_ATTRIBUTE_ID_GSENSE_ERROR_RATE = 0xBF,
    SMART_ATTRIBUTE_ID_POWER_OFF_RETRACT_COUNT = 0xC0,
    SMART_ATTRIBUTE_ID_LOAD_CYCLE_COUNT = 0xC1,
    SMART_ATTRIBUTE_ID_TEMPERATURE = 0xC2,
    SMART_ATTRIBUTE_ID_HARDWARE_ECC_RECOVERED = 0xC3,
    SMART_ATTRIBUTE_ID_REALLOCATION_EVENT_COUNT = 0xC4,
    SMART_ATTRIBUTE_ID_CURRENT_PENDING_SECTOR_COUNT = 0xC5,
    SMART_ATTRIBUTE_ID_UNCORRECTABLE_SECTOR_COUNT = 0xC6,
    SMART_ATTRIBUTE_ID_ULTRADMA_CRC_ERROR_COUNT = 0xC7,
    SMART_ATTRIBUTE_ID_MULTI_ZONE_ERROR_RATE = 0xC8,
    SMART_ATTRIBUTE_ID_OFFTRACK_SOFT_READ_ERROR_RATE = 0xC9,
    SMART_ATTRIBUTE_ID_DATA_ADDRESS_MARK_ERRORS = 0xCA,
    SMART_ATTRIBUTE_ID_RUN_OUT_CANCEL = 0xCB,
    SMART_ATTRIBUTE_ID_SOFT_ECC_CORRECTION = 0xCC,
    SMART_ATTRIBUTE_ID_THERMAL_ASPERITY_RATE_TAR = 0xCD,
    SMART_ATTRIBUTE_ID_FLYING_HEIGHT = 0xCE,
    SMART_ATTRIBUTE_ID_SPIN_HIGH_CURRENT = 0xCF,
    SMART_ATTRIBUTE_ID_SPIN_BUZZ = 0xD0,
    SMART_ATTRIBUTE_ID_OFFLINE_SEEK_PERFORMANCE = 0xD1,
    SMART_ATTRIBUTE_ID_VIBRATION_DURING_WRITE = 0xD3,
    SMART_ATTRIBUTE_ID_SHOCK_DURING_WRITE = 0xD4,
    SMART_ATTRIBUTE_ID_DISK_SHIFT = 0xDC,
    SMART_ATTRIBUTE_ID_GSENSE_ERROR_RATE_ALT = 0xDD,
    SMART_ATTRIBUTE_ID_LOADED_HOURS = 0xDE,
    SMART_ATTRIBUTE_ID_LOAD_UNLOAD_RETRY_COUNT = 0xDF,
    SMART_ATTRIBUTE_ID_LOAD_FRICTION = 0xE0,
    SMART_ATTRIBUTE_ID_LOAD_UNLOAD_CYCLE_COUNT = 0xE1,
    SMART_ATTRIBUTE_ID_LOAD_IN_TIME = 0xE2,
    SMART_ATTRIBUTE_ID_TORQUE_AMPLIFICATION_COUNT = 0xE3,
    SMART_ATTRIBUTE_ID_POWER_OFF_RETRACT_CYCLE = 0xE4,
    // TODO: Add value 229
    SMART_ATTRIBUTE_ID_GMR_HEAD_AMPLITUDE = 0xE6,
    SMART_ATTRIBUTE_ID_DRIVE_TEMPERATURE = 0xE7,
    SMART_ATTRIBUTE_ID_ENDURACE_REMAINING = 0xE8,
    SMART_ATTRIBUTE_ID_SSD_MEDIA_WEAROUT_INDICATOR = 0xE9,
    SMART_ATTRIBUTE_ID_SSD_ERASE_COUNT = 0xEA,
    SMART_ATTRIBUTE_ID_GOOD_BLOCK_COUNT_AND_SYSTEM_BLOCK_COUNT = 0xEB,
    // TODO: Add values 236-239
    SMART_ATTRIBUTE_ID_HEAD_FLYING_HOURS = 0xF0,
    SMART_ATTRIBUTE_ID_TOTAL_HOST_WRITES = 0xF1,
    SMART_ATTRIBUTE_ID_TOTAL_HOST_READS = 0xF2,
    SMART_ATTRIBUTE_ID_TOTAL_HOST_WRITES_EXPANDED = 0xF3,
    SMART_ATTRIBUTE_ID_TOTAL_HOST_READS_EXPANDED = 0xF4,
    SMART_ATTRIBUTE_ID_REMAINING_RATED_WRITE_ENDURANCE = 0xF5,
    SMART_ATTRIBUTE_ID_CUMULATIVE_HOST_SECTORS_WRITTEN = 0xF6,
    SMART_ATTRIBUTE_ID_HOST_PROGRAM_PAGE_COUNT = 0xF7,
    SMART_ATTRIBUTE_ID_BACKGROUND_PROGRAM_PAGE_COUNT = 0xF8,
    SMART_ATTRIBUTE_ID_NAND_WRITES = 0xF9,
    SMART_ATTRIBUTE_ID_READ_ERROR_RETY_RATE = 0xFA,
    SMART_ATTRIBUTE_ID_MIN_SPARES_REMAINING = 0xFB,
    SMART_ATTRIBUTE_ID_NEWLY_ADDED_BAD_FLASH_BLOCK = 0xFC,
    // TODO: Add value 253
    SMART_ATTRIBUTE_ID_FREE_FALL_PROTECTION = 0xFE,
} SMART_ATTRIBUTE_ID;


#define SMART_HEADER_SIZE 2

#include <pshpack1.h>
typedef struct _SMART_ATTRIBUTE
{
    BYTE Id;
    USHORT Flags;
    BYTE CurrentValue;
    BYTE WorstValue;
    BYTE RawValue[6];
    BYTE Reserved;
} SMART_ATTRIBUTE, *PSMART_ATTRIBUTE;
#include <poppack.h>

typedef struct _SMART_ATTRIBUTES
{
    SMART_ATTRIBUTE_ID AttributeId;
    UINT CurrentValue;
    UINT WorstValue;
    UINT RawValue;

    // Pre-fail/Advisory bit
    // This bit is applicable only when the value of this attribute is less than or equal to its threshold.
    // 0 : Advisory: The device has exceeded its intended design life period.
    // 1 : Pre-failure notification : Failure is predicted within 24 hours.
    BOOLEAN Advisory;
    BOOLEAN FailureImminent;

    // Online data collection bit
    // 0 : This value of this attribute is only updated during offline activities.
    // 1 : The value of this attribute is updated during both normal operation and offline activities.
    BOOLEAN OnlineDataCollection;

    // TRUE: This attribute characterizes a performance aspect of the drive,
    //   degradation of which may indicate imminent drive failure, such as data throughput, seektimes, spin up time, etc.
    BOOLEAN Performance;

    // TRUE: This attribute is based on the expected, non-fatal errors that are inherent in disk drives,
    //    increases in which may indicate imminent drive failure, such as ECC errors, seek errors, etc.
    BOOLEAN ErrorRate;

    // TRUE: This attribute counts events, of which an excessive number of which may
    //       indicate imminent drive failure, such as number of re-allocated sectors, etc.
    BOOLEAN EventCount;

    // TRUE: This type is used to specify an attribute that is collected and saved by the drive automatically.
    BOOLEAN SelfPreserving;
} SMART_ATTRIBUTES, *PSMART_ATTRIBUTES;

PCWSTR SmartAttributeGetText(
    _In_ SMART_ATTRIBUTE_ID AttributeId
    );

// diskgraph.c

VOID DiskDeviceQueueNameUpdate(
    _In_ PDV_DISK_ENTRY DiskEntry
    );

VOID DiskDeviceSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers,
    _In_ _Assume_refs_(1) PDV_DISK_ENTRY DiskEntry
    );

// netgraph.c

VOID NetworkDeviceSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers,
    _In_ _Assume_refs_(1) PDV_NETADAPTER_ENTRY AdapterEntry
    );

// power.c

typedef enum _EV_EMI_DEVICE_INDEX
{
    EV_EMI_DEVICE_INDEX_PACKAGE,
    EV_EMI_DEVICE_INDEX_CORE,
    EV_EMI_DEVICE_INDEX_DIMM,
    EV_EMI_DEVICE_INDEX_GPUDISCRETE,
    EV_EMI_DEVICE_INDEX_CPUDISCRETE, // Pseudo
    EV_EMI_DEVICE_INDEX_MAX // Pseudo
} EV_EMI_DEVICE_INDEX;

typedef struct _EV_MEASUREMENT_DATA
{
    ULONGLONG AbsoluteEnergy;
    ULONGLONG AbsoluteTime;
} EV_MEASUREMENT_DATA;

typedef struct _DV_RAPL_ID
{
    PPH_STRING DevicePath;
} DV_RAPL_ID, *PDV_RAPL_ID;

typedef struct _DV_RAPL_ENTRY
{
    DV_RAPL_ID Id;
    PPH_STRING DeviceName;
    HANDLE DeviceHandle;

    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN UserReference : 1;
            BOOLEAN CheckedDeviceSupport : 1;
            BOOLEAN DeviceSupported : 1;
            BOOLEAN DevicePresent : 1;
            BOOLEAN Spare : 4;
        };
    };

    PH_CIRCULAR_BUFFER_FLOAT PackageBuffer;
    PH_CIRCULAR_BUFFER_FLOAT CoreBuffer;
    PH_CIRCULAR_BUFFER_FLOAT DimmBuffer;
    PH_CIRCULAR_BUFFER_FLOAT TotalBuffer;

    FLOAT CurrentProcessorPower;
    FLOAT CurrentCorePower;
    FLOAT CurrentDramPower;
    FLOAT CurrentDiscreteGpuPower;
    FLOAT CurrentComponentPower;
    FLOAT CurrentTotalPower;

    PVOID ChannelDataBuffer;
    ULONG ChannelDataBufferLength;
    ULONG ChannelIndex[EV_EMI_DEVICE_INDEX_MAX];
    EV_MEASUREMENT_DATA ChannelData[EV_EMI_DEVICE_INDEX_MAX];
} DV_RAPL_ENTRY, *PDV_RAPL_ENTRY;

typedef struct _DV_RAPL_SYSINFO_CONTEXT
{
    PDV_RAPL_ENTRY DeviceEntry;

    HWND WindowHandle;
    HWND ProcessorGraphHandle;
    HWND CoreGraphHandle;
    HWND DimmGraphHandle;
    HWND TotalGraphHandle;

    HWND PackageGraphLabelHandle;
    HWND CoreGraphLabelHandle;
    HWND DimmGraphLabelHandle;
    HWND TotalGraphLabelHandle;

    PPH_SYSINFO_SECTION SysinfoSection;
    PH_LAYOUT_MANAGER LayoutManager;
    RECT GraphMargin;
    LONG GraphPadding;

    PH_GRAPH_STATE ProcessorGraphState;
    PH_GRAPH_STATE CoreGraphState;
    PH_GRAPH_STATE DimmGraphState;
    PH_GRAPH_STATE TotalGraphState;

    HWND RaplDevicePanel;
    HWND RaplDeviceProcessorUsageLabel;
    HWND RaplDeviceCoreUsageLabel;
    HWND RaplDeviceDimmUsageLabel;
    HWND RaplDeviceGpuLimitLabel;
    HWND RaplDeviceComponentUsageLabel;
    HWND RaplDeviceTotalUsageLabel;
} DV_RAPL_SYSINFO_CONTEXT, *PDV_RAPL_SYSINFO_CONTEXT;

typedef struct _DV_RAPL_OPTIONS_CONTEXT
{
    HWND ListViewHandle;
    BOOLEAN OptionsChanged;
    PH_LAYOUT_MANAGER LayoutManager;
} DV_RAPL_OPTIONS_CONTEXT, *PDV_RAPL_OPTIONS_CONTEXT;

VOID RaplDeviceInitialize(
    VOID
    );

VOID RaplDevicesLoadList(
    VOID
    );

VOID RaplDevicesUpdate(
    _In_ ULONG RunCount
    );

VOID InitializeRaplDeviceId(
    _Out_ PDV_RAPL_ID Id,
    _In_ PPH_STRING DevicePath
    );

VOID CopyRaplDeviceId(
    _Out_ PDV_RAPL_ID Destination,
    _In_ PDV_RAPL_ID Source
    );

VOID DeleteRaplDeviceId(
    _Inout_ PDV_RAPL_ID Id
    );

BOOLEAN EquivalentRaplDeviceId(
    _In_ PDV_RAPL_ID Id1,
    _In_ PDV_RAPL_ID Id2
    );

PDV_RAPL_ENTRY CreateRaplDeviceEntry(
    _In_ PDV_RAPL_ID Id
    );

VOID RaplDeviceSampleData(
    _In_ PDV_RAPL_ENTRY DeviceEntry,
    _In_ PVOID MeasurementData,
    _In_ EV_EMI_DEVICE_INDEX DeviceIndex
    );

// poweroptions.c

_Success_(return)
BOOLEAN QueryRaplDeviceInterfaceDescription(
    _In_ PWSTR DeviceInterface,
    _Out_ PPH_STRING* DeviceDescription
    );

INT_PTR CALLBACK RaplDeviceOptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// powergraph.c

#define RAPL_GRAPH_PADDING 3

VOID RaplDeviceSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers,
    _In_ _Assume_refs_(1) PDV_RAPL_ENTRY DiskEntry
    );

// gpu.c

// Undocumented device properties (Win10 only)
DEFINE_DEVPROPKEY(DEVPKEY_Gpu_Luid, 0x60b193cb, 0x5276, 0x4d0f, 0x96, 0xfc, 0xf1, 0x73, 0xab, 0xad, 0x3e, 0xc6, 2); // DEVPROP_TYPE_UINT64
DEFINE_DEVPROPKEY(DEVPKEY_Gpu_PhysicalAdapterIndex, 0x60b193cb, 0x5276, 0x4d0f, 0x96, 0xfc, 0xf1, 0x73, 0xab, 0xad, 0x3e, 0xc6, 3); // DEVPROP_TYPE_UINT32
DEFINE_GUID(GUID_COMPUTE_DEVICE_ARRIVAL, 0x1024e4c9, 0x47c9, 0x48d3, 0xb4, 0xa8, 0xf9, 0xdf, 0x78, 0x52, 0x3b, 0x53);

typedef struct _DV_GPU_ID
{
    PPH_STRING DevicePath;
} DV_GPU_ID, *PDV_GPU_ID;

typedef struct _DV_GPU_ENTRY
{
    DV_GPU_ID Id;

    ULONG64 CommitLimit;
    ULONG64 DedicatedLimit;
    ULONG64 SharedLimit;
    ULONG NumberOfSegments;
    ULONG NumberOfNodes;

    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN UserReference : 1;
            BOOLEAN DeviceSupported : 1;
            BOOLEAN DevicePresent : 1;
            BOOLEAN Spare : 5;
        };
    };

    PH_UINT64_DELTA TotalRunningTimeDelta;
    PPH_UINT64_DELTA TotalRunningTimeNodesDelta;
    PPH_CIRCULAR_BUFFER_FLOAT GpuNodesHistory;

    FLOAT CurrentGpuUsage;
    ULONG64 CurrentDedicatedUsage;
    ULONG64 CurrentSharedUsage;
    ULONG64 CurrentCommitUsage;
    FLOAT CurrentPowerUsage;
    FLOAT CurrentTemperature;
    ULONG CurrentFanRPM;

    PH_CIRCULAR_BUFFER_FLOAT GpuUsageHistory;
    PH_CIRCULAR_BUFFER_ULONG64 DedicatedHistory;
    PH_CIRCULAR_BUFFER_ULONG64 SharedHistory;
    PH_CIRCULAR_BUFFER_ULONG64 CommitHistory;
    PH_CIRCULAR_BUFFER_FLOAT PowerHistory;
    PH_CIRCULAR_BUFFER_FLOAT TemperatureHistory;
    PH_CIRCULAR_BUFFER_ULONG FanHistory;
} DV_GPU_ENTRY, *PDV_GPU_ENTRY;

typedef struct _DV_GPU_NODES_WINDOW_CONTEXT
{
    HWND WindowHandle;
    HWND ParentWindowHandle;
    PH_LAYOUT_MANAGER LayoutManager;
    RECT LayoutMargin;
    RECT MinimumSize;
    HWND* GraphHandle;
    PPH_GRAPH_STATE GraphState;
    PPH_LIST NodeNameList;
    ULONG NumberOfNodes;
    PPH_STRING Description;
    PDV_GPU_ENTRY DeviceEntry;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;
} DV_GPU_NODES_WINDOW_CONTEXT, *PDV_GPU_NODES_WINDOW_CONTEXT;

typedef struct _DV_GPU_SYSINFO_CONTEXT
{
    PDV_GPU_ENTRY DeviceEntry;

    PPH_SYSINFO_SECTION SysinfoSection;
    PH_LAYOUT_MANAGER LayoutManager;
    RECT GraphMargin;

    PPH_STRING Description;
    LONG SysInfoGraphPadding;

    HWND NodeWindowHandle;
    HANDLE NodeWindowThreadHandle;
    PH_EVENT NodeWindowInitializedEvent;

    HWND GpuDialog;
    PH_LAYOUT_MANAGER GpuLayoutManager;
    RECT GpuGraphMargin;
    HWND GpuGraphHandle;
    PH_GRAPH_STATE GpuGraphState;
    HWND DedicatedGraphHandle;
    PH_GRAPH_STATE DedicatedGraphState;
    HWND SharedGraphHandle;
    PH_GRAPH_STATE SharedGraphState;
    HWND PowerUsageGraphHandle;
    PH_GRAPH_STATE PowerUsageGraphState;
    HWND TemperatureGraphHandle;
    PH_GRAPH_STATE TemperatureGraphState;
    HWND FanRpmGraphHandle;
    PH_GRAPH_STATE FanRpmGraphState;
    HWND GpuPanel;
    HWND GpuPanelDedicatedUsageLabel;
    HWND GpuPanelDedicatedLimitLabel;
    HWND GpuPanelSharedUsageLabel;
    HWND GpuPanelSharedLimitLabel;
} DV_GPU_SYSINFO_CONTEXT, *PDV_GPU_SYSINFO_CONTEXT;

typedef struct _DV_GPU_OPTIONS_CONTEXT
{
    HWND ListViewHandle;
    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN OptionsChanged : 1;
            BOOLEAN UseAlternateMethod : 1;
            BOOLEAN Spare : 6;
        };
    };
    PH_LAYOUT_MANAGER LayoutManager;
} DV_GPU_OPTIONS_CONTEXT, *PDV_GPU_OPTIONS_CONTEXT;

extern BOOLEAN GraphicsGraphShowText;
extern BOOLEAN GraphicsEnableAvxSupport;
extern BOOLEAN GraphicsEnableScaleGraph;
extern BOOLEAN GraphicsEnableScaleText;
extern BOOLEAN GraphicsPropagateCpuUsage;

VOID GraphicsDeviceInitialize(
    VOID
    );

VOID GraphicsDevicesLoadList(
    VOID
    );

VOID GraphicsDevicesUpdate(
    _In_ ULONG RunCount
    );

VOID InitializeGraphicsDeviceId(
    _Out_ PDV_GPU_ID Id,
    _In_ PPH_STRING DevicePath
    );

VOID CopyGraphicsDeviceId(
    _Out_ PDV_GPU_ID Destination,
    _In_ PDV_GPU_ID Source
    );

VOID DeleteGraphicsDeviceId(
    _Inout_ PDV_GPU_ID Id
    );

BOOLEAN EquivalentGraphicsDeviceId(
    _In_ PDV_GPU_ID Id1,
    _In_ PDV_GPU_ID Id2
    );

PDV_GPU_ENTRY CreateGraphicsDeviceEntry(
    _In_ PDV_GPU_ID Id
    );

FLOAT GraphicsDevicePluginInterfaceGetGpuAdapterUtilization(
    _In_ LUID AdapterLuid
    );

ULONG64 GraphicsDevicePluginInterfaceGetGpuAdapterDedicated(
    _In_ LUID AdapterLuid
    );

ULONG64 GraphicsDevicePluginInterfaceGetGpuAdapterShared(
    _In_ LUID AdapterLuid
    );

FLOAT GraphicsDevicePluginInterfaceGetGpuAdapterEngineUtilization(
    _In_ LUID AdapterLuid,
    _In_ ULONG EngineId
    );

// gpuoptions.c

INT_PTR CALLBACK GraphicsDeviceOptionsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// gpugraph.c

#define GPU_GRAPH_PADDING 3

VOID GraphicsDeviceSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers,
    _In_ PDV_GPU_ENTRY DiskEntry
    );

_Function_class_(PH_SYSINFO_SECTION_CALLBACK)
BOOLEAN GraphicsDeviceSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2
    );

INT_PTR CALLBACK GraphicsDeviceDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

INT_PTR CALLBACK GraphicsDevicePanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

VOID GraphicsDeviceUpdateGraphs(
    _In_ PDV_GPU_SYSINFO_CONTEXT Context
    );

VOID GraphicsDeviceUpdatePanel(
    _In_ PDV_GPU_SYSINFO_CONTEXT Context
    );

PPH_STRING GraphicsQueryDeviceDescription(
    _In_ DEVINST DeviceHandle
    );

PPH_STRING GraphicsQueryDeviceInterfaceDescription(
    _In_opt_ PWSTR DeviceInterface
    );

PPH_STRING GraphicsQueryDevicePropertyString(
    _In_ DEVINST DeviceHandle,
    _In_ CONST DEVPROPKEY* DeviceProperty
    );

_Success_(return)
BOOLEAN GraphicsQueryDeviceInterfaceLuid(
    _In_ PCWSTR DeviceInterface,
    _Out_ LUID* AdapterLuid
    );

_Success_(return)
BOOLEAN GraphicsQueryDeviceInterfaceAdapterIndex(
    _In_ PCWSTR DeviceInterface,
    _Out_ PULONG PhysicalAdapterIndex
    );

PPH_LIST GraphicsQueryDeviceNodeList(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ ULONG NodeCount
    );

NTSTATUS GraphicsQueryPropertyString(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ PPH_STRINGREF PropertyName,
    _Out_ PPH_STRING* String
    );

DEFINE_GUID(DXCORE_ADAPTER_ATTRIBUTE_D3D11_GRAPHICS, 0x8c47866b, 0x7583, 0x450d, 0xf0, 0xf0, 0x6b, 0xad, 0xa8, 0x95, 0xaf, 0x4b);
DEFINE_GUID(DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS, 0x0c9ece4d, 0x2f6e, 0x4f01, 0x8c, 0x96, 0xe8, 0x9e, 0x33, 0x1b, 0x47, 0xb1);
DEFINE_GUID(DXCORE_ADAPTER_ATTRIBUTE_D3D12_CORE_COMPUTE, 0x248e2800, 0xa793, 0x4724, 0xab, 0xaa, 0x23, 0xa6, 0xde, 0x1b, 0xe0, 0x90);
DEFINE_GUID(DXCORE_ADAPTER_ATTRIBUTE_D3D12_GENERIC_ML, 0xb71b0d41, 0x1088, 0x422f, 0xa2, 0x7c, 0x2, 0x50, 0xb7, 0xd3, 0xa9, 0x88);
DEFINE_GUID(DXCORE_ADAPTER_ATTRIBUTE_D3D12_GENERIC_MEDIA, 0x8eb2c848, 0x82f6, 0x4b49, 0xaa, 0x87, 0xae, 0xcf, 0xcf, 0x1, 0x74, 0xc6);
// rev
DEFINE_GUID(DXCORE_ADAPTER_ATTRIBUTE_WSL, 0x42696D9D, 0xD678, 0x4006, 0xA9, 0x3D, 0x30, 0x0C, 0x35, 0x65, 0xBB, 0xE5);

DEFINE_GUID(DXCORE_HARDWARE_TYPE_ATTRIBUTE_GPU, 0xb69eb219, 0x3ded, 0x4464, 0x97, 0x9f, 0xa0, 0xb, 0xd4, 0x68, 0x70, 0x6);
DEFINE_GUID(DXCORE_HARDWARE_TYPE_ATTRIBUTE_COMPUTE_ACCELERATOR, 0xe0b195da, 0x58ef, 0x4a22, 0x90, 0xf1, 0x1f, 0x28, 0x16, 0x9c, 0xab, 0x8d);
DEFINE_GUID(DXCORE_HARDWARE_TYPE_ATTRIBUTE_NPU, 0xd46140c4, 0xadd7, 0x451b, 0x9e, 0x56, 0x6, 0xfe, 0x8c, 0x3b, 0x58, 0xed);
DEFINE_GUID(DXCORE_HARDWARE_TYPE_ATTRIBUTE_MEDIA_ACCELERATOR, 0x66bdb96a, 0x50b, 0x44c7, 0xa4, 0xfd, 0xd1, 0x44, 0xce, 0xa, 0xb4, 0x43);

typedef union _GX_ADAPTER_ATTRIBUTES
{
    struct
    {
        ULONG TypeGpu : 1;                // DXCORE_HARDWARE_TYPE_ATTRIBUTE_GPU
        ULONG TypeComputeAccelerator : 1; // DXCORE_HARDWARE_TYPE_ATTRIBUTE_COMPUTE_ACCELERATOR
        ULONG TypeNpu : 1;                // DXCORE_HARDWARE_TYPE_ATTRIBUTE_NPU
        ULONG TypeMediaAccelerator : 1;   // DXCORE_HARDWARE_TYPE_ATTRIBUTE_MEDIA_ACCELERATOR
        ULONG D3D11Graphics : 1;          // DXCORE_ADAPTER_ATTRIBUTE_D3D11_GRAPHICS
        ULONG D3D12Graphics : 1;          // DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS
        ULONG D3D12CoreCompute : 1;       // DXCORE_ADAPTER_ATTRIBUTE_D3D12_CORE_COMPUTE
        ULONG D3D12GenericML : 1;         // DXCORE_ADAPTER_ATTRIBUTE_D3D12_GENERIC_ML
        ULONG D3D12GenericMedia : 1;      // DXCORE_ADAPTER_ATTRIBUTE_D3D12_GENERIC_MEDIA
        ULONG WSL : 1;                    // DXCORE_ADAPTER_ATTRIBUTE_WSL
        ULONG Spare : 22;
    };

    ULONG Flags;
} GX_ADAPTER_ATTRIBUTES, *PGX_ADAPTER_ATTRIBUTES;

NTSTATUS GraphicsQueryAdapterAttributes(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _Out_ PGX_ADAPTER_ATTRIBUTES Attributes
    );

// graphics.c

typedef struct _D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1
{
    ULONG CommitLimit;
    ULONG BytesCommitted;
    ULONG BytesResident;
    D3DKMT_QUERYSTATISTICS_MEMORY Memory;
    ULONG Aperture; // boolean
    ULONGLONG TotalBytesEvictedByPriority[D3DKMT_MaxAllocationPriorityClass];
    ULONG64 SystemMemoryEndAddress;
    struct
    {
        ULONG64 PreservedDuringStandby : 1;
        ULONG64 PreservedDuringHibernate : 1;
        ULONG64 PartiallyPreservedDuringHibernate : 1;
        ULONG64 Reserved : 61;
    } PowerFlags;
    ULONG64 Reserved[7];
} D3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1, *PD3DKMT_QUERYSTATISTICS_SEGMENT_INFORMATION_V1;

NTSTATUS GraphicsOpenAdapterFromDeviceName(
    _Out_ D3DKMT_HANDLE* AdapterHandle,
    _Out_opt_ PLUID AdapterLuid,
    _In_ PCWSTR DeviceName
    );

BOOLEAN GraphicsCloseAdapterHandle(
    _In_ D3DKMT_HANDLE AdapterHandle
    );

NTSTATUS GraphicsQueryAdapterInformation(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ KMTQUERYADAPTERINFOTYPE InformationClass,
    _Out_writes_bytes_opt_(InformationLength) PVOID Information,
    _In_ UINT32 InformationLength
    );

NTSTATUS GraphicsQueryAdapterNodeInformation(
    _In_ LUID AdapterLuid,
    _Out_ PULONG NumberOfSegments,
    _Out_ PULONG NumberOfNodes
    );

NTSTATUS GraphicsQueryAdapterSegmentLimits(
    _In_ LUID AdapterLuid,
    _In_ ULONG NumberOfSegments,
    _Out_opt_ PULONG64 SharedUsage,
    _Out_opt_ PULONG64 SharedCommit,
    _Out_opt_ PULONG64 SharedLimit,
    _Out_opt_ PULONG64 DedicatedUsage,
    _Out_opt_ PULONG64 DedicatedCommit,
    _Out_opt_ PULONG64 DedicatedLimit
    );

NTSTATUS GraphicsQueryAdapterNodeRunningTime(
    _In_ LUID AdapterLuid,
    _In_ ULONG NodeId,
    _Out_ PULONG64 RunningTime
    );

NTSTATUS GraphicsQueryAdapterDevicePerfData(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _Out_ PFLOAT PowerUsage,
    _Out_ PFLOAT Temperature,
    _Out_ PULONG FanRPM
    );

NTSTATUS GraphicsQueryAdapterDeviceNodePerfData(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ ULONG NodeOrdinal,
    _Out_opt_ PULONG64 Frequency,
    _Out_opt_ PULONG64 MaxFrequency,
    _Out_opt_ PULONG64 MaxFrequencyOC,
    _Out_opt_ PULONG Voltage,
    _Out_opt_ PULONG VoltageMax,
    _Out_opt_ PULONG VoltageMaxOC,
    _Out_opt_ PULONG64 MaxTransitionLatency
    );

_Success_(return)
BOOLEAN GraphicsQueryDeviceProperties(
    _In_ PCWSTR DeviceInterface,
    _Out_opt_ PPH_STRING* Description,
    _Out_opt_ PPH_STRING* DriverDate,
    _Out_opt_ PPH_STRING* DriverVersion,
    _Out_opt_ PPH_STRING* LocationInfo,
    _Out_opt_ PULONG64 InstalledMemory,
    _Out_opt_ LUID* AdapterLuid
    );

// gpunodes.c

VOID GraphicsDeviceShowNodesDialog(
    _In_ PDV_GPU_SYSINFO_CONTEXT Context,
    _In_ HWND ParentWindowHandle
    );

VOID GraphicsDeviceShowDetailsDialog(
    _In_ PDV_GPU_SYSINFO_CONTEXT Context,
    _In_ HWND ParentWindowHandle
    );

// devicetree.c

VOID InitializeDevicesTab(
    VOID
    );

typedef struct DEVICE_PROPERTY_TABLE_ENTRY
{
    PH_DEVICE_PROPERTY_CLASS PropClass;
    PWSTR ColumnName;
    BOOLEAN ColumnVisible;
    ULONG ColumnWidth;
    ULONG ColumnTextFlags;
} DEVICE_PROPERTY_TABLE_ENTRY, *PDEVICE_PROPERTY_TABLE_ENTRY;

extern const DEVICE_PROPERTY_TABLE_ENTRY DeviceItemPropertyTable[];
extern const ULONG DeviceItemPropertyTableCount;

// deviceprops.c

BOOLEAN DeviceShowProperties(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_DEVICE_ITEM DeviceItem
    );

typedef struct _DEVICE_RESOURCE
{
    PCWSTR Type;
    PPH_STRING Setting;
} DEVICE_RESOURCE, * PDEVICE_RESOURCE;

VOID DeviceGetAllocatedResourcesList(
    _In_ PPH_DEVICE_ITEM DeviceItem,
    _Out_ PPH_LIST* List
    );

VOID DeviceFreeAllocatedResourcesList(
    _In_ PPH_LIST List
    );

#endif
