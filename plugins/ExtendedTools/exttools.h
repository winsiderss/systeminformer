/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2018-2024
 *
 */

#ifndef EXTTOOLS_H
#define EXTTOOLS_H

#include <phdk.h>
#include <phappresource.h>
#include <settings.h>
#include <mapldr.h>
#include <workqueue.h>

#include <math.h>

#include "resource.h"

#include "framemon.h"

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

#include <cfgmgr32.h>

// Undocumented device properties (Win10 only)
DEFINE_DEVPROPKEY(DEVPKEY_Gpu_Luid, 0x60b193cb, 0x5276, 0x4d0f, 0x96, 0xfc, 0xf1, 0x73, 0xab, 0xad, 0x3e, 0xc6, 2); // DEVPROP_TYPE_UINT64
DEFINE_DEVPROPKEY(DEVPKEY_Gpu_PhysicalAdapterIndex, 0x60b193cb, 0x5276, 0x4d0f, 0x96, 0xfc, 0xf1, 0x73, 0xab, 0xad, 0x3e, 0xc6, 3); // DEVPROP_TYPE_UINT32
DEFINE_GUID(GUID_COMPUTE_DEVICE_ARRIVAL, 0x1024e4c9, 0x47c9, 0x48d3, 0xb4, 0xa8, 0xf9, 0xdf, 0x78, 0x52, 0x3b, 0x53);

typedef D3DKMT_HANDLE* PD3DKMT_HANDLE;

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

#define PH_RECORD_MAX_USAGE 1

EXTERN_C PPH_PLUGIN PluginInstance;
extern LIST_ENTRY EtProcessBlockListHead;
extern LIST_ENTRY EtNetworkBlockListHead;
extern HWND ProcessTreeNewHandle;
extern HWND NetworkTreeNewHandle;

EXTERN_C ULONG EtWindowsVersion;
EXTERN_C BOOLEAN EtIsExecutingInWow64;
EXTERN_C BOOLEAN EtGpuFahrenheitEnabled;
EXTERN_C BOOLEAN EtNpuFahrenheitEnabled;
extern ULONG ProcessesUpdatedCount;
extern ULONG EtUpdateInterval;
extern USHORT EtMaxPrecisionUnit;
extern BOOLEAN EtGraphShowText;
extern BOOLEAN EtEnableScaleGraph;
extern BOOLEAN EtEnableScaleText;
extern BOOLEAN EtPropagateCpuUsage;
extern BOOLEAN EtEnableAvxSupport;

#define PLUGIN_NAME L"ProcessHacker.ExtendedTools"
#define SETTING_NAME_DISK_TREE_LIST_COLUMNS (PLUGIN_NAME L".DiskTreeListColumns")
#define SETTING_NAME_DISK_TREE_LIST_SORT (PLUGIN_NAME L".DiskTreeListSort")
#define SETTING_NAME_ENABLE_GPUPERFCOUNTERS (PLUGIN_NAME L".EnableGpuPerformanceCounters")
#define SETTING_NAME_ENABLE_NPUPERFCOUNTERS (PLUGIN_NAME L".EnableNpuPerformanceCounters")
#define SETTING_NAME_ENABLE_DISKPERFCOUNTERS (PLUGIN_NAME L".EnableDiskPerformanceCounters")
#define SETTING_NAME_ENABLE_ETW_MONITOR (PLUGIN_NAME L".EnableEtwMonitor")
#define SETTING_NAME_ENABLE_GPU_MONITOR (PLUGIN_NAME L".EnableGpuMonitor")
#define SETTING_NAME_ENABLE_NPU_MONITOR (PLUGIN_NAME L".EnableNpuMonitor")
#define SETTING_NAME_ENABLE_FPS_MONITOR (PLUGIN_NAME L".EnableFpsMonitor")
#define SETTING_NAME_ENABLE_SYSINFO_GRAPHS (PLUGIN_NAME L".EnableSysInfoGraphs")
#define SETTING_NAME_UNLOADED_WINDOW_POSITION (PLUGIN_NAME L".TracertWindowPosition")
#define SETTING_NAME_UNLOADED_WINDOW_SIZE (PLUGIN_NAME L".TracertWindowSize")
#define SETTING_NAME_UNLOADED_COLUMNS (PLUGIN_NAME L".UnloadedListColumns")
#define SETTING_NAME_MODULE_SERVICES_WINDOW_POSITION (PLUGIN_NAME L".ModuleServiceWindowPosition")
#define SETTING_NAME_MODULE_SERVICES_WINDOW_SIZE (PLUGIN_NAME L".ModuleServiceWindowSize")
#define SETTING_NAME_MODULE_SERVICES_COLUMNS (PLUGIN_NAME L".ModuleServiceListColumns")
#define SETTING_NAME_GPU_NODES_WINDOW_POSITION (PLUGIN_NAME L".GpuNodesWindowPosition")
#define SETTING_NAME_GPU_NODES_WINDOW_SIZE (PLUGIN_NAME L".GpuNodesWindowSize")
#define SETTING_NAME_NPU_NODES_WINDOW_POSITION (PLUGIN_NAME L".NpuNodesWindowPosition")
#define SETTING_NAME_NPU_NODES_WINDOW_SIZE (PLUGIN_NAME L".NpuNodesWindowSize")
#define SETTING_NAME_WSWATCH_WINDOW_POSITION (PLUGIN_NAME L".WsWatchWindowPosition")
#define SETTING_NAME_WSWATCH_WINDOW_SIZE (PLUGIN_NAME L".WsWatchWindowSize")
#define SETTING_NAME_WSWATCH_COLUMNS (PLUGIN_NAME L".WsWatchListColumns")
#define SETTING_NAME_TRAYICON_GUIDS (PLUGIN_NAME L".TrayIconGuids")
#define SETTING_NAME_ENABLE_FAHRENHEIT (PLUGIN_NAME L".EnableFahrenheit")
#define SETTING_NAME_FW_TREE_LIST_COLUMNS (PLUGIN_NAME L".FwTreeColumns")
#define SETTING_NAME_FW_TREE_LIST_SORT (PLUGIN_NAME L".FwTreeSort")
#define SETTING_NAME_FW_IGNORE_PORTSCAN (PLUGIN_NAME L".FwIgnorePortScan")
#define SETTING_NAME_FW_IGNORE_LOOPBACK (PLUGIN_NAME L".FwIgnoreLoopback")
#define SETTING_NAME_FW_IGNORE_ALLOW (PLUGIN_NAME L".FwIgnoreAllow")
#define SETTING_NAME_FW_SESSION_GUID (PLUGIN_NAME L".FwSessionGuid")
#define SETTING_NAME_SHOWSYSINFOGRAPH (PLUGIN_NAME L".ToolbarShowSystemInfoGraph")
#define SETTING_NAME_WCT_TREE_LIST_COLUMNS (PLUGIN_NAME L".WaitChainTreeListColumns")
#define SETTING_NAME_WCT_WINDOW_POSITION (PLUGIN_NAME L".WaitChainWindowPosition")
#define SETTING_NAME_WCT_WINDOW_SIZE (PLUGIN_NAME L".WaitChainWindowSize")
#define SETTING_NAME_REPARSE_WINDOW_POSITION (PLUGIN_NAME L".ReparseWindowPosition")
#define SETTING_NAME_REPARSE_WINDOW_SIZE (PLUGIN_NAME L".ReparseWindowSize")
#define SETTING_NAME_REPARSE_LISTVIEW_COLUMNS (PLUGIN_NAME L".ReparseListViewColumns")
#define SETTING_NAME_REPARSE_OBJECTID_LISTVIEW_COLUMNS (PLUGIN_NAME L".ReparseObjidListViewColumns")
#define SETTING_NAME_REPARSE_SD_LISTVIEW_COLUMNS (PLUGIN_NAME L".ReparseSdListViewColumns")
#define SETTING_NAME_PIPE_ENUM_WINDOW_POSITION (PLUGIN_NAME L".PipeEnumWindowPosition")
#define SETTING_NAME_PIPE_ENUM_WINDOW_SIZE (PLUGIN_NAME L".PipeEnumWindowSize")
#define SETTING_NAME_PIPE_ENUM_LISTVIEW_COLUMNS (PLUGIN_NAME L".PipeEnumListViewColumns")
#define SETTING_NAME_PIPE_ENUM_LISTVIEW_COLUMNS_WITH_KSI (PLUGIN_NAME L".PipeEnumListViewColumnsWithKsi")
#define SETTING_NAME_FIRMWARE_WINDOW_POSITION (PLUGIN_NAME L".FirmwareWindowPosition")
#define SETTING_NAME_FIRMWARE_WINDOW_SIZE (PLUGIN_NAME L".FirmwareWindowSize")
#define SETTING_NAME_FIRMWARE_LISTVIEW_COLUMNS (PLUGIN_NAME L".FirmwareListViewColumns")
#define SETTING_NAME_OBJMGR_WINDOW_POSITION (PLUGIN_NAME L".ObjectManagerWindowPosition")
#define SETTING_NAME_OBJMGR_WINDOW_SIZE (PLUGIN_NAME L".ObjectManagerWindowSize")
#define SETTING_NAME_OBJMGR_COLUMNS (PLUGIN_NAME L".ObjectManagerWindowColumns")
#define SETTING_NAME_POOL_WINDOW_POSITION (PLUGIN_NAME L".PoolWindowPosition")
#define SETTING_NAME_POOL_WINDOW_SIZE (PLUGIN_NAME L".PoolWindowSize")
#define SETTING_NAME_POOL_TREE_LIST_COLUMNS (PLUGIN_NAME L".PoolTreeViewColumns")
#define SETTING_NAME_POOL_TREE_LIST_SORT (PLUGIN_NAME L".PoolTreeViewSort")
#define SETTING_NAME_BIGPOOL_WINDOW_POSITION (PLUGIN_NAME L".BigPoolWindowPosition")
#define SETTING_NAME_BIGPOOL_WINDOW_SIZE (PLUGIN_NAME L".BigPoolWindowSize")
#define SETTING_NAME_TPM_WINDOW_POSITION (PLUGIN_NAME L".TpmWindowPosition")
#define SETTING_NAME_TPM_WINDOW_SIZE (PLUGIN_NAME L".TpmWindowSize")
#define SETTING_NAME_TPM_LISTVIEW_COLUMNS (PLUGIN_NAME L".TpmListViewColumns")

VOID EtLoadSettings(
    VOID
    );

// phsvc extensions

typedef enum _ET_PHSVC_API_NUMBER
{
    EtPhSvcGetProcessUnloadedDllsApiNumber = 1
} ET_PHSVC_API_NUMBER;

typedef union _ET_PHSVC_API_GETWOW64THREADAPPDOMAIN
{
    struct
    {
        ULONG ProcessId;
        PH_RELATIVE_STRINGREF Data; // out
    } i;
    struct
    {
        ULONG DataLength;
    } o;
} ET_PHSVC_API_GETWOW64THREADAPPDOMAIN, *PET_PHSVC_API_GETWOW64THREADAPPDOMAIN;

VOID DispatchPhSvcRequest(
    _In_ PVOID Parameter
    );

NTSTATUS CallGetProcessUnloadedDlls(
    _In_ HANDLE ProcessId,
    _Out_ PPH_STRING *UnloadedDlls
    );

// Disk item

#define HISTORY_SIZE 60

typedef struct _ET_DISK_ITEM
{
    LIST_ENTRY AgeListEntry;
    ULONG AddTime;
    ULONG FreshTime;

    HANDLE ProcessId;
    PVOID FileObject;
    PPH_STRING FileName;
    PPH_STRING FileNameWin32;
    PPH_STRING ProcessName;
    WCHAR ProcessIdString[PH_INT32_STR_LEN_1];

    PPH_PROCESS_ITEM ProcessItem;
    ULONG_PTR ProcessIconIndex;
    BOOLEAN ProcessIconValid;

    PPH_PROCESS_RECORD ProcessRecord;

    ULONG IoPriority;
    ULONG ResponseTimeCount;
    FLOAT ResponseTimeTotal; // in milliseconds
    FLOAT ResponseTimeAverage;

    ULONG64 ReadTotal;
    ULONG64 WriteTotal;
    ULONG64 ReadDelta;
    ULONG64 WriteDelta;
    ULONG64 ReadAverage;
    ULONG64 WriteAverage;

    ULONG64 ReadHistory[HISTORY_SIZE];
    ULONG64 WriteHistory[HISTORY_SIZE];
    ULONG HistoryCount;
    ULONG HistoryPosition;
} ET_DISK_ITEM, *PET_DISK_ITEM;

// Disk node

#define ETDSTNC_NAME 0
#define ETDSTNC_PID 1
#define ETDSTNC_FILE 2
#define ETDSTNC_READRATEAVERAGE 3
#define ETDSTNC_WRITERATEAVERAGE 4
#define ETDSTNC_TOTALRATEAVERAGE 5
#define ETDSTNC_IOPRIORITY 6
#define ETDSTNC_RESPONSETIME 7
#define ETDSTNC_ORIGINALNAME 8
#define ETDSTNC_MAXIMUM 9

typedef struct _ET_DISK_NODE
{
    PH_TREENEW_NODE Node;

    PET_DISK_ITEM DiskItem;

    PPH_STRING TooltipText;
    PPH_STRING ProcessNameText;
    PH_STRINGREF TextCache[ETDSTNC_MAXIMUM];

    WCHAR ReadRateAverageText[PH_INT64_STR_LEN_1 + 5];
    WCHAR WriteRateAverageText[PH_INT64_STR_LEN_1 + 5];
    WCHAR TotalRateAverageText[PH_INT64_STR_LEN_1 + 5];
    WCHAR ResponseTimeText[PH_INT64_STR_LEN_1 + 5];
} ET_DISK_NODE, *PET_DISK_NODE;

// Process tree columns

#define ETPRTNC_DISKREADS 1
#define ETPRTNC_DISKWRITES 2
#define ETPRTNC_DISKREADBYTES 3
#define ETPRTNC_DISKWRITEBYTES 4
#define ETPRTNC_DISKTOTALBYTES 5
#define ETPRTNC_DISKREADSDELTA 6
#define ETPRTNC_DISKWRITESDELTA 7
#define ETPRTNC_DISKREADBYTESDELTA 8
#define ETPRTNC_DISKWRITEBYTESDELTA 9
#define ETPRTNC_DISKTOTALBYTESDELTA 10
#define ETPRTNC_NETWORKRECEIVES 11
#define ETPRTNC_NETWORKSENDS 12
#define ETPRTNC_NETWORKRECEIVEBYTES 13
#define ETPRTNC_NETWORKSENDBYTES 14
#define ETPRTNC_NETWORKTOTALBYTES 15
#define ETPRTNC_NETWORKRECEIVESDELTA 16
#define ETPRTNC_NETWORKSENDSDELTA 17
#define ETPRTNC_NETWORKRECEIVEBYTESDELTA 18
#define ETPRTNC_NETWORKSENDBYTESDELTA 19
#define ETPRTNC_NETWORKTOTALBYTESDELTA 20
#define ETPRTNC_HARDFAULTS 21
#define ETPRTNC_HARDFAULTSDELTA 22
#define ETPRTNC_PEAKTHREADS 23
#define ETPRTNC_GPU 24
#define ETPRTNC_GPUDEDICATEDBYTES 25
#define ETPRTNC_GPUSHAREDBYTES 26
#define ETPRTNC_DISKREADRATE 27
#define ETPRTNC_DISKWRITERATE 28
#define ETPRTNC_DISKTOTALRATE 29
#define ETPRTNC_NETWORKRECEIVERATE 30
#define ETPRTNC_NETWORKSENDRATE 31
#define ETPRTNC_NETWORKTOTALRATE 32
#define ETPRTNC_FPS 33
#define ETPRTNC_NPU 34
#define ETPRTNC_NPUDEDICATEDBYTES 35
#define ETPRTNC_NPUSHAREDBYTES 36
#define ETPRTNC_MAXIMUM 36

// Network list columns

#define ETNETNC_RECEIVES 1
#define ETNETNC_SENDS 2
#define ETNETNC_RECEIVEBYTES 3
#define ETNETNC_SENDBYTES 4
#define ETNETNC_TOTALBYTES 5
#define ETNETNC_RECEIVESDELTA 6
#define ETNETNC_SENDSDELTA 7
#define ETNETNC_RECEIVEBYTESDELTA 8
#define ETNETNC_SENDBYTESDELTA 9
#define ETNETNC_TOTALBYTESDELTA 10
#define ETNETNC_FIREWALLSTATUS 11
#define ETNETNC_RECEIVERATE 12
#define ETNETNC_SENDRATE 13
#define ETNETNC_TOTALRATE 14
#define ETNETNC_MAXIMUM 14

typedef enum _ET_PROCESS_STATISTICS_CATEGORY
{
    ET_PROCESS_STATISTICS_CATEGORY_GPU,
    ET_PROCESS_STATISTICS_CATEGORY_DISK,
    ET_PROCESS_STATISTICS_CATEGORY_NETWORK,
    ET_PROCESS_STATISTICS_CATEGORY_NPU,
    ET_PROCESS_STATISTICS_CATEGORY_MAX
} ET_PROCESS_STATISTICS_CATEGORY;

typedef enum _ET_PROCESS_STATISTICS_INDEX
{
    ET_PROCESS_STATISTICS_INDEX_GPUTOTALDEDICATED,
    ET_PROCESS_STATISTICS_INDEX_GPUTOTALSHARED,
    ET_PROCESS_STATISTICS_INDEX_GPUTOTALCOMMIT,
    ET_PROCESS_STATISTICS_INDEX_GPUTOTAL,

    ET_PROCESS_STATISTICS_INDEX_DISKREADS,
    ET_PROCESS_STATISTICS_INDEX_DISKREADBYTES,
    ET_PROCESS_STATISTICS_INDEX_DISKREADBYTESDELTA,
    ET_PROCESS_STATISTICS_INDEX_DISKWRITES,
    ET_PROCESS_STATISTICS_INDEX_DISKWRITEBYTES,
    ET_PROCESS_STATISTICS_INDEX_DISKWRITEBYTESDELTA,
    ET_PROCESS_STATISTICS_INDEX_DISKTOTAL,
    ET_PROCESS_STATISTICS_INDEX_DISKTOTALBYTES,
    ET_PROCESS_STATISTICS_INDEX_DISKTOTALBYTESDELTA,

    ET_PROCESS_STATISTICS_INDEX_NETWORKREADS,
    ET_PROCESS_STATISTICS_INDEX_NETWORKREADBYTES,
    ET_PROCESS_STATISTICS_INDEX_NETWORKREADBYTESDELTA,
    ET_PROCESS_STATISTICS_INDEX_NETWORKWRITES,
    ET_PROCESS_STATISTICS_INDEX_NETWORKWRITEBYTES,
    ET_PROCESS_STATISTICS_INDEX_NETWORKWRITEBYTESDELTA,
    ET_PROCESS_STATISTICS_INDEX_NETWORKTOTAL,
    ET_PROCESS_STATISTICS_INDEX_NETWORKTOTALBYTES,
    ET_PROCESS_STATISTICS_INDEX_NETWORKTOTALBYTESDELTA,

    ET_PROCESS_STATISTICS_INDEX_NPUTOTALDEDICATED,
    ET_PROCESS_STATISTICS_INDEX_NPUTOTALSHARED,
    ET_PROCESS_STATISTICS_INDEX_NPUTOTALCOMMIT,
    ET_PROCESS_STATISTICS_INDEX_NPUTOTAL,

    ET_PROCESS_STATISTICS_INDEX_MAX
} ET_PROCESS_STATISTICS_INDEX;

// Firewall status

typedef enum _ET_FIREWALL_STATUS
{
    FirewallUnknownStatus,
    FirewallAllowedNotRestricted,
    FirewallAllowedRestricted,
    FirewallNotAllowedNotRestricted,
    FirewallNotAllowedRestricted,
    FirewallMaximumStatus
} ET_FIREWALL_STATUS;

// Object extensions

typedef struct _ET_PROCESS_BLOCK
{
    LIST_ENTRY ListEntry;
    PPH_PROCESS_ITEM ProcessItem;
    PPH_PROCESS_NODE ProcessNode;

    BOOLEAN HaveFirstSample;

    // Disk/Network

    ULONG64 DiskReadCount;
    ULONG64 DiskWriteCount;
    ULONG64 NetworkReceiveCount;
    ULONG64 NetworkSendCount;

    ULONG64 DiskReadRaw;
    ULONG64 DiskWriteRaw;
    //ULONG64 LastDiskReadValue;
    //ULONG64 LastDiskWriteValue;
    ULONG64 NetworkReceiveRaw;
    ULONG64 NetworkSendRaw;

    ULONG64 CurrentDiskRead;
    ULONG64 CurrentDiskWrite;
    ULONG64 CurrentNetworkSend;
    ULONG64 CurrentNetworkReceive;

    PH_CIRCULAR_BUFFER_ULONG64 DiskReadHistory;
    PH_CIRCULAR_BUFFER_ULONG64 DiskWriteHistory;
    PH_CIRCULAR_BUFFER_ULONG64 NetworkSendHistory;
    PH_CIRCULAR_BUFFER_ULONG64 NetworkReceiveHistory;

    PH_UINT64_DELTA DiskReadDelta;
    PH_UINT64_DELTA DiskReadRawDelta;
    PH_UINT64_DELTA DiskWriteDelta;
    PH_UINT64_DELTA DiskWriteRawDelta;
    PH_UINT64_DELTA NetworkReceiveDelta;
    PH_UINT64_DELTA NetworkReceiveRawDelta;
    PH_UINT64_DELTA NetworkSendDelta;
    PH_UINT64_DELTA NetworkSendRawDelta;

    // GPU

    PH_UINT64_DELTA GpuRunningTimeDelta;
    //PPH_UINT64_DELTA GpuTotalRunningTimeDelta;
    //PPH_CIRCULAR_BUFFER_FLOAT GpuTotalNodesHistory;

    FLOAT GpuCurrentUsage;
    ULONG GpuCurrentMemUsage;
    ULONG GpuCurrentMemSharedUsage;
    ULONG GpuCurrentCommitUsage;
    PH_CIRCULAR_BUFFER_FLOAT GpuHistory;
    PH_CIRCULAR_BUFFER_ULONG GpuMemoryHistory;
    PH_CIRCULAR_BUFFER_ULONG GpuMemorySharedHistory;
    PH_CIRCULAR_BUFFER_ULONG GpuCommittedHistory;

    FLOAT GpuNodeUtilization;
    ULONG64 GpuDedicatedUsage;
    ULONG64 GpuSharedUsage;
    ULONG64 GpuCommitUsage;

    // NPU

    PH_UINT64_DELTA NpuRunningTimeDelta;
    //PPH_UINT64_DELTA NpuTotalRunningTimeDelta;
    //PPH_CIRCULAR_BUFFER_FLOAT NpuTotalNodesHistory;

    FLOAT NpuCurrentUsage;
    ULONG NpuCurrentMemUsage;
    ULONG NpuCurrentMemSharedUsage;
    ULONG NpuCurrentCommitUsage;
    PH_CIRCULAR_BUFFER_FLOAT NpuHistory;
    PH_CIRCULAR_BUFFER_ULONG NpuMemoryHistory;
    PH_CIRCULAR_BUFFER_ULONG NpuMemorySharedHistory;
    PH_CIRCULAR_BUFFER_ULONG NpuCommittedHistory;

    FLOAT NpuNodeUtilization;
    ULONG64 NpuDedicatedUsage;
    ULONG64 NpuSharedUsage;
    ULONG64 NpuCommitUsage;

    // Frames

    FLOAT FramesPerSecond;
    FLOAT FramesLatency;
    FLOAT FramesMsBetweenPresents;
    FLOAT FramesMsInPresentApi;
    FLOAT FramesMsUntilRenderComplete;
    FLOAT FramesMsUntilDisplayed;
    FLOAT FramesDisplayLatency;
    USHORT FramesRuntime;
    USHORT FramesPresentMode;
    PH_CIRCULAR_BUFFER_FLOAT FramesPerSecondHistory;
    PH_CIRCULAR_BUFFER_FLOAT FramesLatencyHistory;
    PH_CIRCULAR_BUFFER_FLOAT FramesMsBetweenPresentsHistory;
    PH_CIRCULAR_BUFFER_FLOAT FramesMsInPresentApiHistory;
    PH_CIRCULAR_BUFFER_FLOAT FramesMsUntilRenderCompleteHistory;
    PH_CIRCULAR_BUFFER_FLOAT FramesMsUntilDisplayedHistory;
    PH_CIRCULAR_BUFFER_FLOAT FramesDisplayLatencyHistory;

    ULONG ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_MAX + 1];
    ULONG ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_MAX + 1];

    PH_QUEUED_LOCK TextCacheLock;
    BOOLEAN TextCacheValid[ETPRTNC_MAXIMUM + 1];
    SIZE_T TextCacheLength[ETPRTNC_MAXIMUM + 1];
    WCHAR TextCache[ETPRTNC_MAXIMUM + 1][64];
} ET_PROCESS_BLOCK, *PET_PROCESS_BLOCK;

typedef struct _ET_NETWORK_BLOCK
{
    LIST_ENTRY ListEntry;
    PPH_NETWORK_ITEM NetworkItem;

    ULONG64 ReceiveCount;
    ULONG64 SendCount;
    ULONG64 ReceiveRaw;
    ULONG64 SendRaw;

    union
    {
        struct
        {
            PH_UINT64_DELTA ReceiveDelta;
            PH_UINT64_DELTA ReceiveRawDelta;
            PH_UINT64_DELTA SendDelta;
            PH_UINT64_DELTA SendRawDelta;
        };
        PH_UINT64_DELTA Deltas[4];
    };

    ET_FIREWALL_STATUS FirewallStatus;
    BOOLEAN FirewallStatusValid;

    PH_QUEUED_LOCK TextCacheLock;
    SIZE_T TextCacheLength[ETNETNC_MAXIMUM + 1];
    BOOLEAN TextCacheValid[ETNETNC_MAXIMUM + 1];
    WCHAR TextCache[ETNETNC_MAXIMUM + 1][64];
} ET_NETWORK_BLOCK, *PET_NETWORK_BLOCK;

// main

PET_PROCESS_BLOCK EtGetProcessBlock(
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

PET_NETWORK_BLOCK EtGetNetworkBlock(
    _In_ PPH_NETWORK_ITEM NetworkItem
    );

// utils

VOID EtFormatInt64(
    _In_ ULONG64 Value,
    _In_ PET_PROCESS_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    );

VOID EtFormatNetworkInt64(
    _In_ ULONG64 Value,
    _In_ PET_NETWORK_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    );

VOID EtFormatSize(
    _In_ ULONG64 Value,
    _In_ PET_PROCESS_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    );

VOID EtFormatNetworkSize(
    _In_ ULONG64 Value,
    _In_ PET_NETWORK_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    );

VOID EtFormatDouble(
    _In_ DOUBLE Value,
    _In_ PET_PROCESS_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    );

VOID EtFormatRate(
    _In_ ULONG64 ValuePerPeriod,
    _In_ PET_PROCESS_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    );

VOID EtFormatNetworkRate(
    _In_ ULONG64 ValuePerPeriod,
    _In_ PET_NETWORK_BLOCK Block,
    _In_ PPH_PLUGIN_TREENEW_MESSAGE Message
    );

_Success_(return)
BOOLEAN EtOpenAdapterFromDeviceName(
    _Out_ PD3DKMT_HANDLE AdapterHandle,
    _In_ PWSTR DeviceName
    );

BOOLEAN EtCloseAdapterHandle(
    _In_ D3DKMT_HANDLE AdapterHandle
    );

NTSTATUS EtQueryAdapterInformation(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ KMTQUERYADAPTERINFOTYPE InformationClass,
    _Out_writes_bytes_opt_(InformationLength) PVOID Information,
    _In_ UINT32 InformationLength
    );

// HardwareDevices!_GX_ATTRIBUTES
typedef union _ET_ADAPTER_ATTRIBUTES
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
} ET_ADAPTER_ATTRIBUTES, *PET_ADAPTER_ATTRIBUTES;

NTSTATUS EtQueryAdapterAttributes(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _Out_ PET_ADAPTER_ATTRIBUTES Attributes
    );

_Success_(return)
BOOLEAN EtQueryDeviceProperties(
    _In_ PPH_STRING DeviceInterface,
    _Out_opt_ PPH_STRING *Description,
    _Out_opt_ PPH_STRING *DriverDate,
    _Out_opt_ PPH_STRING *DriverVersion,
    _Out_opt_ PPH_STRING *LocationInfo,
    _Out_opt_ ULONG64 *InstalledMemory
    );

PPH_STRING EtGetNodeEngineTypeString(
    _In_ D3DKMT_NODEMETADATA NodeMetaData
    );

BOOLEAN EtIsSoftwareDevice(
    _In_ D3DKMT_HANDLE AdapterHandle
    );

// etwmon

extern BOOLEAN EtEtwEnabled;
extern ULONG EtEtwStatus;

// etwstat

extern ULONG EtDiskReadCount;
extern ULONG EtDiskWriteCount;
extern ULONG EtNetworkReceiveCount;
extern ULONG EtNetworkSendCount;

extern PH_UINT32_DELTA EtDiskReadDelta;
extern PH_UINT32_DELTA EtDiskWriteDelta;
extern PH_UINT32_DELTA EtNetworkReceiveDelta;
extern PH_UINT32_DELTA EtNetworkSendDelta;

extern PH_UINT32_DELTA EtDiskReadCountDelta;
extern PH_UINT32_DELTA EtDiskWriteCountDelta;
extern PH_UINT32_DELTA EtNetworkReceiveCountDelta;
extern PH_UINT32_DELTA EtNetworkSendCountDelta;

extern PH_CIRCULAR_BUFFER_ULONG EtDiskReadHistory;
extern PH_CIRCULAR_BUFFER_ULONG EtDiskWriteHistory;
extern PH_CIRCULAR_BUFFER_ULONG EtNetworkReceiveHistory;
extern PH_CIRCULAR_BUFFER_ULONG EtNetworkSendHistory;
extern PH_CIRCULAR_BUFFER_ULONG EtMaxDiskHistory;
extern PH_CIRCULAR_BUFFER_ULONG EtMaxNetworkHistory;
#ifdef PH_RECORD_MAX_USAGE
extern PH_CIRCULAR_BUFFER_ULONG64 PhMaxDiskUsageHistory;
extern PH_CIRCULAR_BUFFER_ULONG64 PhMaxNetworkUsageHistory;
#endif

VOID EtEtwStatisticsInitialization(
    VOID
    );

VOID EtEtwStatisticsUninitialization(
    VOID
    );

// etwdisk

extern BOOLEAN EtDiskEnabled;
extern ULONG EtRunCount;

extern PPH_OBJECT_TYPE EtDiskItemType;
extern PH_CALLBACK EtDiskItemAddedEvent;
extern PH_CALLBACK EtDiskItemModifiedEvent;
extern PH_CALLBACK EtDiskItemRemovedEvent;
extern PH_CALLBACK EtDiskItemsUpdatedEvent;

VOID EtInitializeDiskInformation(
    VOID
    );

PET_DISK_ITEM EtCreateDiskItem(
    VOID
    );

PET_DISK_ITEM EtReferenceDiskItem(
    _In_ HANDLE ProcessId,
    _In_ PVOID FileObject
    );

PPH_STRING EtFileObjectToFileName(
    _In_ PVOID FileObject
    );

// etwprprp

VOID EtProcessEtwPropertiesInitializing(
    _In_ PVOID Parameter
    );

// disktab

VOID EtInitializeDiskTab(
    VOID
    );

VOID EtLoadSettingsDiskTreeList(
    _In_ HWND WindowHandle
    );

VOID EtSaveSettingsDiskTreeList(
    _In_ HWND WindowHandle
    );

// gpumon

typedef struct _ETP_GPU_ADAPTER
{
    LUID AdapterLuid;
    ULONG SegmentCount;
    ULONG NodeCount;
    ULONG FirstNodeIndex;

    PPH_STRING DeviceInterface;
    PPH_STRING Description;
    PPH_LIST NodeNameList;

    RTL_BITMAP ApertureBitMap;
    ULONG ApertureBitMapBuffer[1];
} ETP_GPU_ADAPTER, *PETP_GPU_ADAPTER;

extern BOOLEAN EtGpuEnabled;
extern BOOLEAN EtGpuSupported;
extern BOOLEAN EtGpuD3DEnabled;
EXTERN_C BOOLEAN EtFramesEnabled;
extern PPH_LIST EtpGpuAdapterList;

extern ULONG EtGpuTotalNodeCount;
extern ULONG EtGpuTotalSegmentCount;
extern ULONG64 EtGpuDedicatedLimit;
extern ULONG64 EtGpuSharedLimit;

extern PH_UINT64_DELTA EtGpuClockTotalRunningTimeDelta;
extern LARGE_INTEGER EtGpuClockTotalRunningTimeFrequency;
extern FLOAT EtGpuNodeUsage;
extern PH_CIRCULAR_BUFFER_FLOAT EtGpuNodeHistory;
extern PH_CIRCULAR_BUFFER_ULONG EtMaxGpuNodeHistory; // ID of max. GPU usage process
extern PH_CIRCULAR_BUFFER_FLOAT EtMaxGpuNodeUsageHistory;

extern PPH_UINT64_DELTA EtGpuNodesTotalRunningTimeDelta;
extern PPH_CIRCULAR_BUFFER_FLOAT EtGpuNodesHistory;

extern ULONG64 EtGpuDedicatedUsage;
extern ULONG64 EtGpuSharedUsage;
extern FLOAT EtGpuPowerUsageLimit;
extern FLOAT EtGpuPowerUsage;
extern FLOAT EtGpuTemperatureLimit;
extern FLOAT EtGpuTemperature;
extern ULONG64 EtGpuFanRpmLimit;
extern ULONG64 EtGpuFanRpm;
extern PH_CIRCULAR_BUFFER_ULONG64 EtGpuDedicatedHistory;
extern PH_CIRCULAR_BUFFER_ULONG64 EtGpuSharedHistory;
extern PH_CIRCULAR_BUFFER_FLOAT EtGpuPowerUsageHistory;
extern PH_CIRCULAR_BUFFER_FLOAT EtGpuTemperatureHistory;
extern PH_CIRCULAR_BUFFER_ULONG64 EtGpuFanRpmHistory;

VOID EtGpuMonitorInitialization(
    VOID
    );

typedef struct _ET_PROCESS_GPU_STATISTICS
{
    ULONG SegmentCount;
    ULONG NodeCount;

    ULONG64 DedicatedCommitted;
    ULONG64 SharedCommitted;

    ULONG64 BytesAllocated;
    ULONG64 BytesReserved;
    ULONG64 WriteCombinedBytesAllocated;
    ULONG64 WriteCombinedBytesReserved;
    ULONG64 CachedBytesAllocated;
    ULONG64 CachedBytesReserved;
    ULONG64 SectionBytesAllocated;
    ULONG64 SectionBytesReserved;

    ULONG64 RunningTime;
    ULONG64 ContextSwitches;
} ET_PROCESS_GPU_STATISTICS, *PET_PROCESS_GPU_STATISTICS;

VOID EtSaveGpuMonitorSettings(
    VOID
    );

ULONG EtGetGpuAdapterCount(
    VOID
    );

ULONG EtGetGpuAdapterIndexFromNodeIndex(
    _In_ ULONG NodeIndex
    );

PPH_STRING EtGetGpuAdapterNodeDescription(
    _In_ ULONG Index,
    _In_ ULONG NodeIndex
    );

PPH_STRING EtGetGpuAdapterDescription(
    _In_ ULONG Index
    );

VOID EtQueryProcessGpuStatistics(
    _In_ HANDLE ProcessHandle,
    _Out_ PET_PROCESS_GPU_STATISTICS Statistics
    );

// gpudetails

VOID EtShowGpuDetailsDialog(
    _In_ HWND ParentWindowHandle
    );

// gpuprprp

VOID EtProcessGpuPropertiesInitializing(
    _In_ PVOID Parameter
    );

// gpunodes

VOID EtShowGpuNodesDialog(
    _In_ HWND ParentWindowHandle
    );

// gpusys

VOID EtGpuSystemInformationInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers
    );

// gpumini

VOID EtGpuMiniInformationInitializing(
    _In_ PPH_PLUGIN_MINIINFO_POINTERS Pointers
    );

// npumon

typedef struct _ETP_NPU_ADAPTER
{
    LUID AdapterLuid;
    ULONG SegmentCount;
    ULONG NodeCount;
    ULONG FirstNodeIndex;

    PPH_STRING DeviceInterface;
    PPH_STRING Description;
    PPH_LIST NodeNameList;

    RTL_BITMAP ApertureBitMap;
    ULONG ApertureBitMapBuffer[1];
} ETP_NPU_ADAPTER, *PETP_NPU_ADAPTER;

extern BOOLEAN EtNpuEnabled;
extern BOOLEAN EtNpuSupported;
extern BOOLEAN EtNpuD3DEnabled;
extern PPH_LIST EtpNpuAdapterList;

extern ULONG EtNpuTotalNodeCount;
extern ULONG EtNpuTotalSegmentCount;
extern ULONG64 EtNpuDedicatedLimit;
extern ULONG64 EtNpuSharedLimit;

extern PH_UINT64_DELTA EtNpuClockTotalRunningTimeDelta;
extern LARGE_INTEGER EtNpuClockTotalRunningTimeFrequency;
extern FLOAT EtNpuNodeUsage;
extern PH_CIRCULAR_BUFFER_FLOAT EtNpuNodeHistory;
extern PH_CIRCULAR_BUFFER_ULONG EtMaxNpuNodeHistory; // ID of max. GPU usage process
extern PH_CIRCULAR_BUFFER_FLOAT EtMaxNpuNodeUsageHistory;

extern PPH_UINT64_DELTA EtNpuNodesTotalRunningTimeDelta;
extern PPH_CIRCULAR_BUFFER_FLOAT EtNpuNodesHistory;

extern ULONG64 EtNpuDedicatedUsage;
extern ULONG64 EtNpuSharedUsage;
extern FLOAT EtNpuPowerUsageLimit;
extern FLOAT EtNpuPowerUsage;
extern FLOAT EtNpuTemperatureLimit;
extern FLOAT EtNpuTemperature;
extern ULONG64 EtNpuFanRpmLimit;
extern ULONG64 EtNpuFanRpm;
extern PH_CIRCULAR_BUFFER_ULONG64 EtNpuDedicatedHistory;
extern PH_CIRCULAR_BUFFER_ULONG64 EtNpuSharedHistory;
extern PH_CIRCULAR_BUFFER_FLOAT EtNpuPowerUsageHistory;
extern PH_CIRCULAR_BUFFER_FLOAT EtNpuTemperatureHistory;
extern PH_CIRCULAR_BUFFER_ULONG64 EtNpuFanRpmHistory;

VOID EtNpuMonitorInitialization(
    VOID
    );

typedef struct _ET_PROCESS_NPU_STATISTICS
{
    ULONG SegmentCount;
    ULONG NodeCount;

    ULONG64 DedicatedCommitted;
    ULONG64 SharedCommitted;

    ULONG64 BytesAllocated;
    ULONG64 BytesReserved;
    ULONG64 WriteCombinedBytesAllocated;
    ULONG64 WriteCombinedBytesReserved;
    ULONG64 CachedBytesAllocated;
    ULONG64 CachedBytesReserved;
    ULONG64 SectionBytesAllocated;
    ULONG64 SectionBytesReserved;

    ULONG64 RunningTime;
    ULONG64 ContextSwitches;
} ET_PROCESS_NPU_STATISTICS, *PET_PROCESS_NPU_STATISTICS;

VOID EtSaveNpuMonitorSettings(
    VOID
    );

ULONG EtGetNpuAdapterCount(
    VOID
    );

ULONG EtGetNpuAdapterIndexFromNodeIndex(
    _In_ ULONG NodeIndex
    );

PPH_STRING EtGetNpuAdapterNodeDescription(
    _In_ ULONG Index,
    _In_ ULONG NodeIndex
    );

PPH_STRING EtGetNpuAdapterDescription(
    _In_ ULONG Index
    );

VOID EtQueryProcessNpuStatistics(
    _In_ HANDLE ProcessHandle,
    _Out_ PET_PROCESS_NPU_STATISTICS Statistics
    );

// npudetails

VOID EtShowNpuDetailsDialog(
    _In_ HWND ParentWindowHandle
    );

// npuprprp

VOID EtProcessNpuPropertiesInitializing(
    _In_ PVOID Parameter
    );

// npunodes

VOID EtShowNpuNodesDialog(
    _In_ HWND ParentWindowHandle
    );

// npusys

VOID EtNpuSystemInformationInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers
    );

// npumini

VOID EtNpuMiniInformationInitializing(
    _In_ PPH_PLUGIN_MINIINFO_POINTERS Pointers
    );

// treeext

VOID EtProcessTreeNewInitializing(
    _In_ PVOID Parameter
    );

VOID EtProcessTreeNewMessage(
    _In_ PVOID Parameter
    );

VOID EtNetworkTreeNewInitializing(
    _In_ PVOID Parameter
    );

VOID EtNetworkTreeNewMessage(
    _In_ PVOID Parameter
    );

ET_FIREWALL_STATUS EtQueryFirewallStatus(
    _In_ PPH_NETWORK_ITEM NetworkItem
    );

// etwsys

VOID EtEtwSystemInformationInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers
    );

// etwmini

VOID EtEtwMiniInformationInitializing(
    _In_ PPH_PLUGIN_MINIINFO_POINTERS Pointers
    );

// iconext

typedef struct _TB_GRAPH_CONTEXT
{
    LONG GraphDpi;
    ULONG GraphColor1;
    ULONG GraphColor2;
} TB_GRAPH_CONTEXT, *PTB_GRAPH_CONTEXT;

extern BOOLEAN EtTrayIconTransparencyEnabled;

VOID EtLoadTrayIconGuids(
    VOID
    );

VOID EtRegisterNotifyIcons(
    _In_ PPH_TRAY_ICON_POINTERS Pointers
    );

VOID EtRegisterToolbarGraphs(
    VOID
    );

// modsrv

VOID EtShowModuleServicesDialog(
    _In_ HWND ParentWindowHandle,
    _In_ HANDLE ProcessId,
    _In_ PPH_STRING ModuleName
    );

// objprp

VOID EtHandlePropertiesInitializing(
    _In_ PVOID Parameter
    );

// options

INT_PTR CALLBACK OptionsDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

// thrdact

BOOLEAN EtUiCancelIoThread(
    _In_ HWND hWnd,
    _In_ PPH_THREAD_ITEM Thread
    );

// unldll

VOID EtShowUnloadedDllsDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

// wswatch

VOID EtShowWsWatchDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

// counters

VOID EtPerfCounterInitialization(
    VOID
    );

FLOAT EtLookupProcessGpuUtilization(
    _In_ HANDLE ProcessId
    );

_Success_(return)
BOOLEAN EtLookupProcessGpuMemoryCounters(
    _In_opt_ HANDLE ProcessId,
    _Out_ PULONG64 SharedUsage,
    _Out_ PULONG64 DedicatedUsage,
    _Out_ PULONG64 CommitUsage
    );

FLOAT EtLookupTotalGpuUtilization(
    VOID
    );

FLOAT EtLookupTotalGpuEngineUtilization(
    _In_ ULONG EngineId
    );

ULONG64 EtLookupTotalGpuDedicated(
    VOID
    );

ULONG64 EtLookupTotalGpuShared(
    VOID
    );

FLOAT EtLookupProcessNpuUtilization(
    _In_ HANDLE ProcessId
    );

_Success_(return)
BOOLEAN EtLookupProcessNpuMemoryCounters(
    _In_opt_ HANDLE ProcessId,
    _Out_ PULONG64 SharedUsage,
    _Out_ PULONG64 DedicatedUsage,
    _Out_ PULONG64 CommitUsage
    );

FLOAT EtLookupTotalNpuUtilization(
    VOID
    );

FLOAT EtLookupTotalNpuEngineUtilization(
    _In_ ULONG EngineId
    );

ULONG64 EtLookupTotalNpuDedicated(
    VOID
    );

ULONG64 EtLookupTotalNpuShared(
    VOID
    );

// EXTENDEDTOOLS_INTERFACE
FLOAT EtLookupTotalGpuAdapterUtilization(
    _In_ LUID AdapterLuid
    );

// EXTENDEDTOOLS_INTERFACE
FLOAT EtLookupTotalGpuAdapterEngineUtilization(
    _In_ LUID AdapterLuid,
    _In_ ULONG EngineId
    );

// EXTENDEDTOOLS_INTERFACE
ULONG64 EtLookupTotalGpuAdapterDedicated(
    _In_ LUID AdapterLuid
    );

// EXTENDEDTOOLS_INTERFACE
ULONG64 EtLookupTotalGpuAdapterShared(
    _In_ LUID AdapterLuid
    );

// Firewall

extern BOOLEAN EtFwEnabled;
extern ULONG EtFwStatus;
extern ULONG FwRunCount;
extern HANDLE EtFwEngineHandle;
extern PH_CALLBACK FwItemAddedEvent;
extern PH_CALLBACK FwItemModifiedEvent;
extern PH_CALLBACK FwItemRemovedEvent;
extern PH_CALLBACK FwItemsUpdatedEvent;

typedef enum _FW_COLUMN_TYPE
{
    FW_COLUMN_NAME,
    FW_COLUMN_ACTION,
    FW_COLUMN_DIRECTION,
    FW_COLUMN_RULENAME,
    FW_COLUMN_RULEDESCRIPTION,
    FW_COLUMN_LOCALADDRESS,
    FW_COLUMN_LOCALPORT,
    FW_COLUMN_LOCALHOSTNAME,
    FW_COLUMN_REMOTEADDRESS,
    FW_COLUMN_REMOTEPORT,
    FW_COLUMN_REMOTEHOSTNAME,
    FW_COLUMN_PROTOCOL,
    FW_COLUMN_TIMESTAMP,
    FW_COLUMN_PROCESSFILENAME,
    FW_COLUMN_USER,
    FW_COLUMN_PACKAGE,
    FW_COLUMN_COUNTRY,
    FW_COLUMN_LOCALADDRESSCLASS,
    FW_COLUMN_REMOTEADDRESSCLASS,
    FW_COLUMN_LOCALADDRESSSSCOPE,
    FW_COLUMN_REMOTEADDRESSSCOPE,
    FW_COLUMN_ORIGINALNAME,
    FW_COLUMN_LOCALSERVICENAME,
    FW_COLUMN_REMOTESERVICENAME,
    FW_COLUMN_MAXIMUM
} FW_COLUMN_TYPE;

typedef enum _FW_EVENT_DIRECTION
{
    FW_EVENT_DIRECTION_NONE,
    FW_EVENT_DIRECTION_INBOUND,
    FW_EVENT_DIRECTION_OUTBOUND,
    FW_EVENT_DIRECTION_FORWARD,
    FW_EVENT_DIRECTION_BIDIRECTIONAL,
    FW_EVENT_DIRECTION_MAX
} FW_EVENT_DIRECTION;

typedef struct _FW_EVENT_ITEM
{
    PH_TREENEW_NODE Node;

    LIST_ENTRY AgeListEntry;
    ULONG RunId;
    ULONG64 Index;
    LARGE_INTEGER TimeStamp;

    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN Loopback : 1;
            BOOLEAN Spare : 3;
            BOOLEAN LocalPortServiceResolved : 1;
            BOOLEAN RemotePortServiceResolved : 1;
            BOOLEAN LocalHostnameResolved : 1;
            BOOLEAN RemoteHostnameResolved : 1;
        };
    };

    volatile LONG JustResolved;

    FW_EVENT_DIRECTION Direction;
    ULONG Type; // FWPM_NET_EVENT_TYPE
    ULONG IpProtocol;
    ULONG ScopeId;
    PH_IP_ENDPOINT LocalEndpoint;
    PH_IP_ENDPOINT RemoteEndpoint;

    PSID UserSid;
    //PSID PackageSid;
    PPH_STRING UserName;
    //PPH_STRING PackageName;

    PPH_PROCESS_ITEM ProcessItem;
    ULONG_PTR ProcessIconIndex;
    BOOLEAN ProcessIconValid;

    PPH_STRING ProcessFileName;
    PPH_STRING ProcessFileNameWin32;
    PPH_STRING ProcessBaseString;

    PPH_STRING LocalHostnameString;
    PPH_STRING RemoteHostnameString;

    ULONG LocalAddressStringLength;
    ULONG RemoteAddressStringLength;
    ULONG LocalPortStringLength;
    ULONG RemotePortStringLength;

    WCHAR LocalAddressString[INET6_ADDRSTRLEN];
    WCHAR RemoteAddressString[INET6_ADDRSTRLEN];
    WCHAR LocalPortString[PH_INT32_STR_LEN_1];
    WCHAR RemotePortString[PH_INT32_STR_LEN_1];

    PPH_STRING RuleName;
    PPH_STRING RuleDescription;
    PPH_STRING RemoteCountryName;
    INT32 CountryIconIndex;

    PPH_STRING TimeString;
    PPH_STRING TooltipText;

    PPH_STRINGREF LocalPortServiceName;
    PPH_STRINGREF RemotePortServiceName;

    PH_STRINGREF TextCache[FW_COLUMN_MAXIMUM];
} FW_EVENT_ITEM, *PFW_EVENT_ITEM;

ULONG EtFwMonitorInitialize(
    VOID
    );

VOID EtFwMonitorUninitialize(
    VOID
    );

ULONG EtFwMonitorEnumEvents(
    VOID
    );

VOID EtInitializeFirewallTab(
    VOID
    );

VOID InitializeFwTreeListDpi(
    _In_ HWND TreeNewHandle
    );

VOID LoadSettingsFwTreeList(
    _In_ HWND TreeNewHandle
    );

VOID SaveSettingsFwTreeList(
    VOID
    );

_Success_(return)
BOOLEAN EtFwLookupAddressClass(
    _In_ PPH_IP_ADDRESS Address,
    _Out_ PPH_STRINGREF ClassString
    );

_Success_(return)
BOOLEAN EtFwLookupAddressScope(
    _In_ PPH_IP_ADDRESS Address,
    _Out_ PPH_STRINGREF ScopeString
    );

PPH_STRING EtFwGetSidFullNameCachedSlow(
    _In_ PSID Sid
    );

VOID EtFwDrawCountryIcon(
    _In_ HDC hdc,
    _In_ RECT rect,
    _In_ INT Index
    );

VOID EtFwShowPingWindow(
    _In_ HWND ParentWindowHandle,
    _In_ PH_IP_ENDPOINT Endpoint
    );

VOID EtFwShowTracerWindow(
    _In_ HWND ParentWindowHandle,
    _In_ PH_IP_ENDPOINT Endpoint
    );

VOID EtFwShowWhoisWindow(
    _In_ HWND ParentWindowHandle,
    _In_ PH_IP_ENDPOINT Endpoint
    );

_Success_(return)
BOOLEAN EtFwLookupPortServiceName(
    _In_ ULONG Port,
    _Out_ PPH_STRINGREF* ServiceName
    );

typedef struct _SEC_WINNT_AUTH_IDENTITY_W SEC_WINNT_AUTH_IDENTITY_W, *PSEC_WINNT_AUTH_IDENTITY_W;
typedef struct FWPM_SESSION0_ FWPM_SESSION0;

typedef ULONG (WINAPI* _FwpmEngineOpen0)(
    _In_opt_ const wchar_t* serverName,
    _In_ UINT32 authnService,
    _In_opt_ SEC_WINNT_AUTH_IDENTITY_W* authIdentity,
    _In_opt_ const FWPM_SESSION0* session,
    _Out_ HANDLE* engineHandle
    );

typedef ULONG (WINAPI* _FwpmEngineClose0)(
    _Inout_ HANDLE engineHandle
    );

typedef VOID (WINAPI* _FwpmFreeMemory0)(
    _Inout_ PVOID* p
    );

typedef enum FWPM_ENGINE_OPTION_ FWPM_ENGINE_OPTION;
typedef struct FWP_VALUE0_ FWP_VALUE0;

typedef ULONG (WINAPI* _FwpmEngineSetOption0)(
    _In_ HANDLE engineHandle,
    _In_ FWPM_ENGINE_OPTION option,
    _In_ const FWP_VALUE0* newValue
    );

typedef struct FWPM_FILTER0_ FWPM_FILTER0;

typedef ULONG (WINAPI* _FwpmFilterGetById0)(
   _In_ HANDLE engineHandle,
   _In_ UINT64 id,
   _Outptr_ FWPM_FILTER0** filter
   );

typedef struct FWPM_LAYER0_ FWPM_LAYER0;

typedef ULONG (WINAPI* _FwpmLayerGetById0)(
   _In_ HANDLE engineHandle,
   _In_ UINT16 id,
   _Outptr_ FWPM_LAYER0** layer
   );

typedef ULONG (WINAPI* _FwpmNetEventSubscribe4)(
    _In_ HANDLE engineHandle,
    _In_ PVOID subscription,
    _In_ PVOID callback,
    _In_opt_ PVOID context,
    _Out_ HANDLE* eventsHandle
    );

typedef ULONG (WINAPI* _FwpmNetEventUnsubscribe0)(
    _In_ HANDLE engineHandle,
    _Inout_ HANDLE eventsHandle
    );

typedef struct FWPM_NET_EVENT_ENUM_TEMPLATE0_ FWPM_NET_EVENT_ENUM_TEMPLATE0;

typedef ULONG (WINAPI* _FwpmNetEventCreateEnumHandle0)(
    _In_ HANDLE engineHandle,
    _In_opt_ const FWPM_NET_EVENT_ENUM_TEMPLATE0* enumTemplate,
    _Out_ HANDLE* enumHandle
    );

typedef ULONG (WINAPI* _FwpmNetEventDestroyEnumHandle0)(
   _In_ HANDLE engineHandle,
   _Inout_ HANDLE enumHandle
   );

typedef ULONG (WINAPI* _FwpmNetEventEnum5)(
    _In_ HANDLE engineHandle,
    _In_ HANDLE enumHandle,
    _In_ UINT32 numEntriesRequested,
    _Out_ PVOID** entries,
    _Out_ UINT32* numEntriesReturned
    );

// ETW Microsoft-Windows-WFP::DirectionMap
#define FWP_DIRECTION_MAP_INBOUND 0x3900
#define FWP_DIRECTION_MAP_OUTBOUND 0x3901
#define FWP_DIRECTION_MAP_FORWARD 0x3902
#define FWP_DIRECTION_MAP_BIDIRECTIONAL 0x3903

VOID InitializeFwTreeList(
    _In_ HWND hwnd
    );

PFW_EVENT_ITEM AddFwNode(
    _In_ PFW_EVENT_ITEM FwItem
    );

VOID RemoveFwNode(
    _In_ PFW_EVENT_ITEM FwNode
    );

VOID UpdateFwNode(
    _In_ PFW_EVENT_ITEM FwNode
    );

BOOLEAN NTAPI FwTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_opt_ PVOID Context
    );

PFW_EVENT_ITEM EtFwGetSelectedFwItem(
    VOID
    );

_Success_(return)
BOOLEAN EtFwGetSelectedFwItems(
    _Out_ PFW_EVENT_ITEM **FwItems,
    _Out_ PULONG NumberOfFwItems
    );

VOID EtFwDeselectAllFwNodes(
    VOID
    );

VOID EtFwSelectAndEnsureVisibleFwNode(
    _In_ PFW_EVENT_ITEM FwNode
    );

VOID EtFwCopyFwList(
    VOID
    );

VOID EtFwWriteFwList(
    _In_ PPH_FILE_STREAM FileStream,
    _In_ ULONG Mode
    );

VOID EtFwHandleFwCommand(
    _In_ HWND TreeWindowHandle,
    _In_ ULONG Id
    );

VOID InitializeFwMenu(
    _In_ PPH_EMENU Menu,
    _In_ PFW_EVENT_ITEM *FwItems,
    _In_ ULONG NumberOfFwItems
    );

VOID ShowFwContextMenu(
    _In_ HWND TreeWindowHandle,
    _In_ PPH_TREENEW_CONTEXT_MENU TreeMouseEvent
    );

VOID NTAPI FwItemAddedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    );

VOID NTAPI FwItemModifiedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI FwItemRemovedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI FwItemsUpdatedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI OnFwItemsUpdated(
    _In_ ULONG RunId
    );

BOOLEAN NTAPI FwSearchFilterCallback(
    _In_ PPH_TREENEW_NODE Node,
    _In_opt_ PVOID Context
    );

VOID NTAPI FwSearchChangedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

VOID NTAPI FwToolStatusActivateContent(
    _In_ BOOLEAN Select
    );

HWND NTAPI FwToolStatusGetTreeNewHandle(
    VOID
    );

// frames

VOID EtProcessFramesPropertiesInitializing(
    _In_ PVOID Parameter
    );

// wct

VOID EtShowWaitChainProcessDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

VOID EtShowWaitChainThreadDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_THREAD_ITEM ThreadItem
    );

// reparse

VOID EtShowReparseDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PVOID Context
    );

// pipe_enum

VOID EtShowPipeEnumDialog(
    _In_ HWND ParentWindowHandle
    );

// firmware

typedef struct _UEFI_WINDOW_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HWND ParentWindowHandle;
    PH_LAYOUT_MANAGER LayoutManager;
} UEFI_WINDOW_CONTEXT, *PUEFI_WINDOW_CONTEXT;

typedef struct _EFI_ENTRY
{
    ULONG Attributes;
    ULONG Length;
    PPH_STRING Name;
    PPH_STRING GuidString;
} EFI_ENTRY, *PEFI_ENTRY;

VOID EtShowFirmwareEditDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PEFI_ENTRY Entry
    );

VOID EtShowFirmwareDialog(
    _In_ HWND ParentWindowHandle
    );

// objmgr

VOID EtShowObjectManagerDialog(
    _In_ HWND ParentWindowHandle
    );

// poolmon

VOID EtShowPoolTableDialog(
    _In_ HWND ParentWindowHandle
    );

// tpm

BOOLEAN EtTpmIsReady(
    VOID
    );

VOID EtShowTpmDialog(
    _In_ HWND ParentWindowHandle
    );

#endif
