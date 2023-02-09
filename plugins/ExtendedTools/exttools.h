/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2018-2023
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

#define PH_RECORD_MAX_USAGE 1

extern PPH_PLUGIN PluginInstance;
extern LIST_ENTRY EtProcessBlockListHead;
extern LIST_ENTRY EtNetworkBlockListHead;
extern HWND ProcessTreeNewHandle;
extern HWND NetworkTreeNewHandle;
extern ULONG ProcessesUpdatedCount;
extern ULONG EtUpdateInterval;
extern USHORT EtMaxPrecisionUnit;
extern BOOLEAN EtGraphShowText;
extern BOOLEAN EtEnableScaleGraph;
extern BOOLEAN EtEnableScaleText;
extern BOOLEAN EtPropagateCpuUsage;

#define PLUGIN_NAME L"ProcessHacker.ExtendedTools"
#define SETTING_NAME_DISK_TREE_LIST_COLUMNS (PLUGIN_NAME L".DiskTreeListColumns")
#define SETTING_NAME_DISK_TREE_LIST_SORT (PLUGIN_NAME L".DiskTreeListSort")
#define SETTING_NAME_ENABLE_GPUPERFCOUNTERS (PLUGIN_NAME L".EnableGpuPerformanceCounters")
#define SETTING_NAME_ENABLE_DISKPERFCOUNTERS (PLUGIN_NAME L".EnableDiskPerformanceCounters")
#define SETTING_NAME_ENABLE_ETW_MONITOR (PLUGIN_NAME L".EnableEtwMonitor")
#define SETTING_NAME_ENABLE_GPU_MONITOR (PLUGIN_NAME L".EnableGpuMonitor")
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
#define SETTING_NAME_WSWATCH_WINDOW_POSITION (PLUGIN_NAME L".WsWatchWindowPosition")
#define SETTING_NAME_WSWATCH_WINDOW_SIZE (PLUGIN_NAME L".WsWatchWindowSize")
#define SETTING_NAME_WSWATCH_COLUMNS (PLUGIN_NAME L".WsWatchListColumns")
#define SETTING_NAME_TRAYICON_GUIDS (PLUGIN_NAME L".TrayIconGuids")
#define SETTING_NAME_ENABLE_FAHRENHEIT (PLUGIN_NAME L".EnableFahrenheit")
#define SETTING_NAME_FW_TREE_LIST_COLUMNS (PLUGIN_NAME L".FwTreeColumns")
#define SETTING_NAME_FW_TREE_LIST_SORT (PLUGIN_NAME L".FwTreeSort")
#define SETTING_NAME_FW_IGNORE_PORTSCAN (PLUGIN_NAME L".FwIgnorePortScan")
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

VOID EtLoadSettings(
    VOID
    );

PPH_STRING PhGetSelectedListViewItemText(
    _In_ HWND hWnd
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
#define ETDSTNC_FILE 1
#define ETDSTNC_READRATEAVERAGE 2
#define ETDSTNC_WRITERATEAVERAGE 3
#define ETDSTNC_TOTALRATEAVERAGE 4
#define ETDSTNC_IOPRIORITY 5
#define ETDSTNC_RESPONSETIME 6
#define ETDSTNC_PID 7
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
#define ETPRTNC_MAXIMUM 33

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

    PH_UINT64_DELTA GpuRunningTimeDelta;
    //PPH_UINT64_DELTA GpuTotalRunningTimeDelta;
    //PPH_CIRCULAR_BUFFER_FLOAT GpuTotalNodesHistory;

    FLOAT CurrentGpuUsage;
    ULONG CurrentMemUsage;
    ULONG CurrentMemSharedUsage;
    ULONG CurrentCommitUsage;
    PH_CIRCULAR_BUFFER_FLOAT GpuHistory;
    PH_CIRCULAR_BUFFER_ULONG MemoryHistory;
    PH_CIRCULAR_BUFFER_ULONG MemorySharedHistory;
    PH_CIRCULAR_BUFFER_ULONG GpuCommittedHistory;

    FLOAT GpuNodeUtilization;
    ULONG64 GpuDedicatedUsage;
    ULONG64 GpuSharedUsage;
    ULONG64 GpuCommitUsage;

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
    _In_ PPH_STRING FileName
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
    VOID
    );

VOID EtSaveSettingsDiskTreeList(
    VOID
    );

// gpumon

typedef D3DKMT_HANDLE* PD3DKMT_HANDLE;

extern BOOLEAN EtGpuEnabled;
extern BOOLEAN EtGpuSupported;
extern BOOLEAN EtD3DEnabled;
EXTERN_C BOOLEAN EtFramesEnabled;
extern PPH_LIST EtpGpuAdapterList;

extern ULONG EtGpuTotalNodeCount;
extern ULONG EtGpuTotalSegmentCount;
extern ULONG64 EtGpuDedicatedLimit;
extern ULONG64 EtGpuSharedLimit;

extern PH_UINT64_DELTA EtClockTotalRunningTimeDelta;
extern LARGE_INTEGER EtClockTotalRunningTimeFrequency;
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

NTSTATUS EtQueryAdapterInformation(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ KMTQUERYADAPTERINFOTYPE InformationClass,
    _Out_writes_bytes_opt_(InformationLength) PVOID Information,
    _In_ UINT32 InformationLength
    );

_Success_(return)
BOOLEAN EtOpenAdapterFromDeviceName(
    _Out_ PD3DKMT_HANDLE AdapterHandle,
    _In_ PWSTR DeviceName
    );

BOOLEAN EtCloseAdapterHandle(
    _In_ D3DKMT_HANDLE AdapterHandle
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

// iconext

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
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
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

NTSTATUS EtUpdatePerfCounterData(
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

FLOAT EtLookupTotalGpuAdapterUtilization(
    _In_ LUID AdapterLuid
    );

FLOAT EtLookupTotalGpuAdapterEngineUtilization(
    _In_ LUID AdapterLuid,
    _In_ ULONG EngineId
    );

ULONG64 EtLookupTotalGpuDedicated(
    VOID
    );

ULONG64 EtLookupTotalGpuAdapterDedicated(
    _In_ LUID AdapterLuid
    );

ULONG64 EtLookupTotalGpuShared(
    VOID
    );

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
    FW_COLUMN_MAXIMUM
} FW_COLUMN_TYPE;

typedef struct _BOOT_WINDOW_CONTEXT
{
    HWND ListViewHandle;
    HWND SearchHandle;

    PH_LAYOUT_MANAGER LayoutManager;

    HFONT NormalFontHandle;
    HFONT BoldFontHandle;

    HWND PluginMenuActive;
    UINT PluginMenuActiveId;
} BOOT_WINDOW_CONTEXT, *PBOOT_WINDOW_CONTEXT;

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
            BOOLEAN Spare : 5;
            BOOLEAN LocalHostnameResolved : 1;
            BOOLEAN RemoteHostnameResolved : 1;
        };
    };

    LONG JustResolved;

    ULONG Direction;
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

    WCHAR LocalAddressString[INET6_ADDRSTRLEN];
    WCHAR RemoteAddressString[INET6_ADDRSTRLEN];
    WCHAR LocalPortString[PH_INT32_STR_LEN_1];
    WCHAR RemotePortString[PH_INT32_STR_LEN_1];

    PPH_STRING RuleName;
    PPH_STRING RuleDescription;
    PPH_STRING RemoteCountryName;
    INT CountryIconIndex;

    PPH_STRING TimeString;
    PPH_STRING TooltipText;

    PH_STRINGREF TextCache[FW_COLUMN_MAXIMUM];
} FW_EVENT_ITEM, *PFW_EVENT_ITEM;

ULONG EtFwMonitorInitialize(
    VOID
    );

VOID EtFwMonitorUninitialize(
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
    _In_ HWND TreeNewHandle
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

typedef ULONG (WINAPI* _FwpmNetEventSubscribe)(
    _In_ HANDLE engineHandle,
    _In_ PVOID subscription,
    _In_ PVOID callback,
    _In_opt_ PVOID context,
    _Out_ HANDLE* eventsHandle
    );

// ETW Microsoft-Windows-WFP::DirectionMap
#define FWP_DIRECTION_MAP_INBOUND 0x3900
#define FWP_DIRECTION_MAP_OUTBOUND 0x3901
#define FWP_DIRECTION_MAP_FORWARD 0x3902
#define FWP_DIRECTION_MAP_BIDIRECTIONAL 0x3903
EXTERN_C CONST DECLSPEC_SELECTANY IN6_ADDR in6addr_v4mappedprefix = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 };

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

PVOID EtWaitChainContextCreate(
    VOID
    );

VOID EtShowWaitChainDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PVOID Context
    );

VOID NTAPI WctProcessMenuInitializingCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    );
VOID NTAPI WctThreadMenuInitializingCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
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
    HWND ListViewHandle;
    HWND ParentWindowHandle;
    PH_LAYOUT_MANAGER LayoutManager;
} UEFI_WINDOW_CONTEXT, *PUEFI_WINDOW_CONTEXT;

typedef struct _EFI_ENTRY
{
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

#endif
