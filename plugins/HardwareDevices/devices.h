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

#define PLUGIN_NAME L"ProcessHacker.HardwareDevices"
#define SETTING_NAME_ENABLE_NDIS (PLUGIN_NAME L".EnableNDIS")
#define SETTING_NAME_INTERFACE_LIST (PLUGIN_NAME L".NetworkList")
#define SETTING_NAME_DISK_LIST (PLUGIN_NAME L".DiskList")

#define CINTERFACE
#define COBJMACROS
#include <phdk.h>
#include <phappresource.h>

#include <windowsx.h>
#include <uxtheme.h>
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

// main.c

PPH_STRING TrimString(
    _In_ PPH_STRING String
    );

INT AddListViewGroup(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _In_ PWSTR Text
    );

INT AddListViewItemGroupId(
    _In_ HWND ListViewHandle,
    _In_ INT GroupId,
    _In_ INT Index,
    _In_ PWSTR Text,
    _In_opt_ PVOID Param
    );

ULONG64 QueryRegistryUlong64(
    _In_ HANDLE KeyHandle,
    _In_ PWSTR ValueName
    );

VOID ShowDeviceMenu(
    _In_ HWND ParentWindow,
    _In_ PPH_STRING DeviceInstance
    );

// adapter.c

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

    PH_LAYOUT_MANAGER LayoutManager;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

    ULONG64 LastDetailsInboundValue;
    ULONG64 LastDetailsIOutboundValue;
} DV_NETADAPTER_DETAILS_CONTEXT, *PDV_NETADAPTER_DETAILS_CONTEXT;

typedef struct _DV_NETADAPTER_CONTEXT
{
    HWND ListViewHandle;
    //HIMAGELIST ImageList;
    BOOLEAN OptionsChanged;
    BOOLEAN UseAlternateMethod;
} DV_NETADAPTER_CONTEXT, *PDV_NETADAPTER_CONTEXT;

VOID NetAdaptersLoadList(
    VOID
    );

VOID ShowOptionsDialog(
    _In_ HWND ParentHandle
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
    NETADAPTER_DETAILS_INDEX_DNS,
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

// dmex: rev
typedef ULONG (WINAPI* _GetInterfaceDescriptionFromGuid)(
    _Inout_ PGUID InterfaceGuid,
    _Out_opt_ PWSTR InterfaceDescription,
    _Inout_ PSIZE_T LengthAddress,
    PVOID Unknown1,
    PVOID Unknown2
    );

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

BOOLEAN QueryInterfaceRow(
    _In_ PDV_NETADAPTER_ID Id,
    _Out_ PMIB_IF_ROW2 InterfaceRow
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
} DV_DISK_SYSINFO_CONTEXT, *PDV_DISK_SYSINFO_CONTEXT;

typedef struct _DV_DISK_OPTIONS_CONTEXT
{
    HWND ListViewHandle;
    //HIMAGELIST ImageList;
    BOOLEAN OptionsChanged;
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

VOID ShowDiskDriveDetailsDialog(
    _In_ PDV_DISK_SYSINFO_CONTEXT Context
    );

// disknotify.c

VOID AddRemoveDeviceChangeCallback(
    VOID
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

NTSTATUS DiskDriveQueryStatistics(
    _In_ HANDLE DeviceHandle,
    _Out_ PDISK_PERFORMANCE Info
    );

PPH_STRING DiskDriveQueryGeometry(
    _In_ HANDLE DeviceHandle
    );

BOOLEAN DiskDriveQueryImminentFailure(
    _In_ HANDLE DeviceHandle,
    _Out_ PPH_LIST* DiskSmartAttributes
    );

typedef struct _DISK_HANDLE_ENTRY
{
    WCHAR DeviceLetter;
    HANDLE DeviceHandle;
} DISK_HANDLE_ENTRY, *PDISK_HANDLE_ENTRY;

PPH_LIST DiskDriveQueryMountPointHandles(
    _In_ ULONG DeviceNumber
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

BOOLEAN DiskDriveQueryNtfsVolumeInfo(
    _In_ HANDLE DosDeviceHandle,
    _Out_ PNTFS_VOLUME_INFO VolumeInfo
    );

BOOLEAN DiskDriveQueryRefsVolumeInfo(
    _In_ HANDLE DosDeviceHandle,
    _Out_ PREFS_VOLUME_DATA_BUFFER VolumeInfo
    );

NTSTATUS DiskDriveQueryVolumeInformation(
    _In_ HANDLE DosDeviceHandle,
    _Out_ PFILE_FS_VOLUME_INFORMATION* VolumeInfo
    );

NTSTATUS DiskDriveQueryVolumeAttributes(
    _In_ HANDLE DosDeviceHandle,
    _Out_ PFILE_FS_ATTRIBUTE_INFORMATION* AttributeInfo
    );

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
    // Unknown values 14-182
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
    // Unknown values 229
    SMART_ATTRIBUTE_ID_GMR_HEAD_AMPLITUDE = 0xE6,
    SMART_ATTRIBUTE_ID_DRIVE_TEMPERATURE = 0xE7,
    // Unknown values 232-239
    SMART_ATTRIBUTE_ID_HEAD_FLYING_HOURS = 0xF0,
    SMART_ATTRIBUTE_ID_TOTAL_LBA_WRITTEN = 0xF1,
    SMART_ATTRIBUTE_ID_TOTAL_LBA_READ = 0xF2,
    // Unknown values 243-249
    SMART_ATTRIBUTE_ID_READ_ERROR_RETY_RATE = 0xFA,
    // Unknown values 251-253
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

PWSTR SmartAttributeGetDescription(
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

#endif _DEVICES_H_