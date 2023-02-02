/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2015-2023
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
#define SETTING_NAME_GRAPHICS_NODES_WINDOW_POSITION (PLUGIN_NAME L".GraphicsNodesWindowPosition")
#define SETTING_NAME_GRAPHICS_NODES_WINDOW_SIZE (PLUGIN_NAME L".GraphicsNodesWindowSize")
#define SETTING_NAME_DEVICE_TREE_WINDOW_POSITION (PLUGIN_NAME L".DeviceTreeWindowPosition")
#define SETTING_NAME_DEVICE_TREE_WINDOW_SIZE (PLUGIN_NAME L".DeviceTreeWindowSize")
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
#define SETTING_NAME_DEVICE_SORT_CHILDREN_BY_NAME (PLUGIN_NAME L".SortDeviceChildrenByName")
#define SETTING_NAME_DEVICE_SHOW_ROOT (PLUGIN_NAME L".ShowRootDevice")
#define SETTING_NAME_DEVICE_SHOW_SOFTWARE_COMPONENTS (PLUGIN_NAME L".ShowSoftwareComponents")

#include <phdk.h>
#include <phappresource.h>
#include <settings.h>

#include <math.h>
#include <cfgmgr32.h>

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

extern PPH_OBJECT_TYPE NetAdapterEntryType;
extern PPH_LIST NetworkAdaptersList;
extern PH_QUEUED_LOCK NetworkAdaptersListLock;

extern PPH_OBJECT_TYPE DiskDriveEntryType;
extern PPH_LIST DiskDrivesList;
extern PH_QUEUED_LOCK DiskDrivesListLock;

extern PPH_OBJECT_TYPE RaplDeviceEntryType;
extern PPH_LIST RaplDevicesList;
extern PH_QUEUED_LOCK RaplDevicesListLock;

extern PPH_OBJECT_TYPE GraphicsDeviceEntryType;
extern PPH_LIST GraphicsDevicesList;
extern PH_QUEUED_LOCK GraphicsDevicesListLock;

// main.c

PPH_STRING TrimString(
    _In_ PPH_STRING String
    );

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
            BOOLEAN Spare : 3;
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
} DV_NETADAPTER_ENTRY, *PDV_NETADAPTER_ENTRY;

typedef struct _DV_NETADAPTER_SYSINFO_CONTEXT
{
    PDV_NETADAPTER_ENTRY AdapterEntry;
    PPH_STRING SectionName;

    HWND WindowHandle;
    HWND PanelWindowHandle;
    HWND GraphHandle;

    HANDLE DetailsWindowThreadHandle;
    HWND DetailsWindowDialogHandle;
    PH_EVENT DetailsWindowInitializedEvent;

    PPH_SYSINFO_SECTION SysinfoSection;
    PH_GRAPH_STATE GraphState;
    PH_LAYOUT_MANAGER LayoutManager;
    RECT GraphMargin;

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
} DV_NETADAPTER_CONTEXT, *PDV_NETADAPTER_CONTEXT;

VOID NetAdaptersLoadList(
    VOID
    );

// adapter.c

VOID NetAdaptersInitialize(
    VOID
    );

VOID NetAdaptersUpdate(
    VOID
    );

VOID NetAdapterUpdateDeviceInfo(
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

PWSTR MediumTypeToString(
    _In_ NDIS_PHYSICAL_MEDIUM MediumType
    );

_Success_(return)
BOOLEAN NetworkAdapterQueryWlanConfig(
    _In_ PGUID InterfaceGuid
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
            BOOLEAN Spare : 5;
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
    PDV_DISK_ENTRY DiskEntry;
    PPH_STRING SectionName;

    HWND WindowHandle;
    HWND PanelWindowHandle;
    HWND GraphHandle;

    PPH_SYSINFO_SECTION SysinfoSection;
    PH_GRAPH_STATE GraphState;
    PH_LAYOUT_MANAGER LayoutManager;
    RECT GraphMargin;

    HWND DiskPathLabel;
    HWND DiskNameLabel;
    HWND DiskDrivePanelReadLabel;
    HWND DiskDrivePanelWriteLabel;
    HWND DiskDrivePanelTotalLabel;
    HWND DiskDrivePanelActiveLabel;
    HWND DiskDrivePanelTimeLabel;
    HWND DiskDrivePanelBytesLabel;

    HANDLE DetailsWindowThreadHandle;
    HWND DetailsWindowDialogHandle;
    PH_EVENT DetailsWindowInitializedEvent;
} DV_DISK_SYSINFO_CONTEXT, *PDV_DISK_SYSINFO_CONTEXT;

typedef struct _DV_DISK_OPTIONS_CONTEXT
{
    HWND ListViewHandle;
    BOOLEAN OptionsChanged;
    PH_LAYOUT_MANAGER LayoutManager;
} DV_DISK_OPTIONS_CONTEXT, *PDV_DISK_OPTIONS_CONTEXT;

VOID DiskDrivesInitialize(VOID);
VOID DiskDrivesLoadList(VOID);
VOID DiskDrivesUpdate(VOID);

VOID DiskDriveUpdateDeviceInfo(
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
    HWND ParentHandle;

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

    PCOMMON_PAGE_CONTEXT PageContext;
} DV_DISK_PAGE_CONTEXT, *PDV_DISK_PAGE_CONTEXT;

typedef enum _DISKDRIVE_DETAILS_INDEX
{
    DISKDRIVE_DETAILS_INDEX_FS_CREATION_TIME,
    DISKDRIVE_DETAILS_INDEX_SERIAL_NUMBER,
    DISKDRIVE_DETAILS_INDEX_FILE_SYSTEM,
    DISKDRIVE_DETAILS_INDEX_FS_VERSION,
    DISKDRIVE_DETAILS_INDEX_LFS_VERSION,

    DISKDRIVE_DETAILS_INDEX_TOTAL_SIZE,
    DISKDRIVE_DETAILS_INDEX_TOTAL_FREE,
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

    DISKDRIVE_DETAILS_INDEX_OTHER_EXCEPTIONS
} DISKDRIVE_DETAILS_INDEX;

VOID ShowDiskDriveDetailsDialog(
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

typedef struct _NTFS_FILESYSTEM_STATISTICS
{
    FILESYSTEM_STATISTICS FileSystemStatistics;
    NTFS_STATISTICS NtfsStatistics;
} NTFS_FILESYSTEM_STATISTICS, *PNTFS_FILESYSTEM_STATISTICS;

typedef struct _FAT_FILESYSTEM_STATISTICS
{
    FILESYSTEM_STATISTICS FileSystemStatistics;
    NTFS_STATISTICS FatStatistics;
} FAT_FILESYSTEM_STATISTICS, *PFAT_FILESYSTEM_STATISTICS;

typedef struct _EXFAT_FILESYSTEM_STATISTICS
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

// https://en.wikipedia.org/wiki/S.M.A.R.T.#Known_ATA_S.M.A.R.T._attributes
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
    // TODO: Add values 14-170
    SMART_ATTRIBUTE_ID_SSD_PROGRAM_FAIL_COUNT = 0xAB,
    SMART_ATTRIBUTE_ID_SSD_ERASE_FAIL_COUNT = 0xAC,
    SMART_ATTRIBUTE_ID_SSD_WEAR_LEVELING_COUNT = 0xAD,
    SMART_ATTRIBUTE_ID_UNEXPECTED_POWER_LOSS = 0xAE,
    // TODO: Add values 175-176
    SMART_ATTRIBUTE_ID_WEAR_RANGE_DELTA = 0xB1,
    // TODO: Add values 178-180
    SMART_ATTRIBUTE_ID_SSD_PROGRAM_FAIL_COUNT_TOTAL = 0xB5,
    SMART_ATTRIBUTE_ID_ERASE_FAIL_COUNT = 0xB6,
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
    SMART_ATTRIBUTE_ID_POWER_OFF_RETTRACT_CYCLE = 0xE4,
    // TODO: Add value 229
    SMART_ATTRIBUTE_ID_GMR_HEAD_AMPLITUDE = 0xE6,
    SMART_ATTRIBUTE_ID_DRIVE_TEMPERATURE = 0xE7,
    // TODO: Add value 232
    SMART_ATTRIBUTE_ID_SSD_MEDIA_WEAROUT_HOURS = 0xE9,
    SMART_ATTRIBUTE_ID_SSD_ERASE_COUNT = 0xEA,
    // TODO: Add values 235-239
    SMART_ATTRIBUTE_ID_HEAD_FLYING_HOURS = 0xF0,
    SMART_ATTRIBUTE_ID_TOTAL_LBA_WRITTEN = 0xF1,
    SMART_ATTRIBUTE_ID_TOTAL_LBA_READ = 0xF2,
    // TODO: Add values 243-249
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
    // This bit is applicable only when the value of this attribute is less than or equal to its threshhold.
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

PWSTR SmartAttributeGetText(
    _In_ SMART_ATTRIBUTE_ID AttributeId
    );

// diskgraph.c

VOID DiskDriveSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers,
    _In_ _Assume_refs_(1) PDV_DISK_ENTRY DiskEntry
    );

// netgraph.c

VOID NetAdapterSysInfoInitializing(
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
    VOID
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
    VOID
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
    _In_ PWSTR DeviceName
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

#endif
