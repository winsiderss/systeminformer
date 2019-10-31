#ifndef EXTTOOLS_H
#define EXTTOOLS_H

#include <phdk.h>
#include <phappresource.h>
#include <settings.h>
#include <math.h>

#include "resource.h"

#include <d3d11.h>
#include "d3dkmt.h"

extern PPH_PLUGIN PluginInstance;
extern LIST_ENTRY EtProcessBlockListHead;
extern LIST_ENTRY EtNetworkBlockListHead;
extern HWND ProcessTreeNewHandle;
extern HWND NetworkTreeNewHandle;
extern ULONG ProcessesUpdatedCount;

#define PLUGIN_NAME L"ProcessHacker.ExtendedTools"
#define SETTING_NAME_DISK_TREE_LIST_COLUMNS (PLUGIN_NAME L".DiskTreeListColumns")
#define SETTING_NAME_DISK_TREE_LIST_SORT (PLUGIN_NAME L".DiskTreeListSort")
#define SETTING_NAME_ENABLE_D3DKMT (PLUGIN_NAME L".EnableD3DKMT")
#define SETTING_NAME_ENABLE_DISKEXT (PLUGIN_NAME L".EnableDiskExt")
#define SETTING_NAME_ENABLE_ETW_MONITOR (PLUGIN_NAME L".EnableEtwMonitor")
#define SETTING_NAME_ENABLE_GPU_MONITOR (PLUGIN_NAME L".EnableGpuMonitor")
#define SETTING_NAME_ENABLE_SYSINFO_GRAPHS (PLUGIN_NAME L".EnableSysInfoGraphs")
#define SETTING_NAME_GPU_NODE_BITMAP (PLUGIN_NAME L".GpuNodeBitmap")
#define SETTING_NAME_GPU_LAST_NODE_COUNT (PLUGIN_NAME L".GpuLastNodeCount")
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

// Window messages
#define ET_WM_SHOWDIALOG (WM_APP + 1)
#define ET_WM_UPDATE (WM_APP + 2)

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

    PPH_PROCESS_ITEM ProcessItem;
    HICON ProcessIcon;
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
#define ETDSTNC_MAXIMUM 7

typedef struct _ET_DISK_NODE
{
    PH_TREENEW_NODE Node;

    PET_DISK_ITEM DiskItem;

    PH_STRINGREF TextCache[ETDSTNC_MAXIMUM];

    PPH_STRING ProcessNameText;
    PPH_STRING ReadRateAverageText;
    PPH_STRING WriteRateAverageText;
    PPH_STRING TotalRateAverageText;
    PPH_STRING ResponseTimeText;

    PPH_STRING TooltipText;
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
#define ETPRTNC_MAXIMUM 32

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
    ET_PROCESS_STATISTICS_INDEX_RUNNINGTIME,
    ET_PROCESS_STATISTICS_INDEX_CONTEXTSWITCHES,
    //ET_PROCESS_STATISTICS_INDEX_TOTALNODES,
    //ET_PROCESS_STATISTICS_INDEX_TOTALSEGMENTS,
    ET_PROCESS_STATISTICS_INDEX_TOTALDEDICATED,
    ET_PROCESS_STATISTICS_INDEX_TOTALSHARED,
    ET_PROCESS_STATISTICS_INDEX_TOTALCOMMIT,

    ET_PROCESS_STATISTICS_INDEX_DISKREADS,
    ET_PROCESS_STATISTICS_INDEX_DISKREADBYTES,
    ET_PROCESS_STATISTICS_INDEX_DISKREADBYTESDELTA,
    ET_PROCESS_STATISTICS_INDEX_DISKWRITES,
    ET_PROCESS_STATISTICS_INDEX_DISKWRITEBYTES,
    ET_PROCESS_STATISTICS_INDEX_DISKWRITEBYTESDELTA,

    ET_PROCESS_STATISTICS_INDEX_NETWORKREADS,
    ET_PROCESS_STATISTICS_INDEX_NETWORKREADBYTES,
    ET_PROCESS_STATISTICS_INDEX_NETWORKREADBYTESDELTA,
    ET_PROCESS_STATISTICS_INDEX_NETWORKWRITES,
    ET_PROCESS_STATISTICS_INDEX_NETWORKWRITEBYTES,
    ET_PROCESS_STATISTICS_INDEX_NETWORKWRITEBYTESDELTA,

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

    FLOAT GpuNodeUsage;
    ULONG64 GpuDedicatedUsage;
    ULONG64 GpuSharedUsage;
    ULONG64 GpuCommitUsage;
    ULONG64 GpuContextSwitches;

    PH_UINT32_DELTA HardFaultsDelta;

    PH_QUEUED_LOCK TextCacheLock;
    PPH_STRING TextCache[ETPRTNC_MAXIMUM + 1];
    BOOLEAN TextCacheValid[ETPRTNC_MAXIMUM + 1];
    ULONG ListViewGroupCache[ET_PROCESS_STATISTICS_CATEGORY_MAX + 1];
    ULONG ListViewRowCache[ET_PROCESS_STATISTICS_INDEX_MAX + 1];
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
    PPH_STRING TextCache[ETNETNC_MAXIMUM + 1];
    BOOLEAN TextCacheValid[ETNETNC_MAXIMUM + 1];
} ET_NETWORK_BLOCK, *PET_NETWORK_BLOCK;

// main

PET_PROCESS_BLOCK EtGetProcessBlock(
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

PET_NETWORK_BLOCK EtGetNetworkBlock(
    _In_ PPH_NETWORK_ITEM NetworkItem
    );

// utils

VOID EtFormatRate(
    _In_ ULONG64 ValuePerPeriod,
    _Inout_ PPH_STRING *Buffer,
    _Out_opt_ PPH_STRINGREF String
    );

// etwmon

extern BOOLEAN EtEtwEnabled;

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

extern BOOLEAN EtGpuEnabled;
extern BOOLEAN EtD3DEnabled;
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
extern PH_CIRCULAR_BUFFER_ULONG64 EtGpuDedicatedHistory;
extern PH_CIRCULAR_BUFFER_ULONG64 EtGpuSharedHistory;

VOID EtGpuMonitorInitialization(
    VOID
    );

NTSTATUS EtQueryAdapterInformation(
    _In_ D3DKMT_HANDLE AdapterHandle,
    _In_ KMTQUERYADAPTERINFOTYPE InformationClass,
    _Out_writes_bytes_opt_(InformationLength) PVOID Information,
    _In_ UINT32 InformationLength
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
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

// wswatch

VOID EtShowWsWatchDialog(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_PROCESS_ITEM ProcessItem
    );

// counters

ULONG64 EtLookupProcessGpuEngineUtilization(
    _In_opt_ HANDLE ProcessId
    );

ULONG64 EtLookupProcessGpuDedicated(
    _In_opt_ HANDLE ProcessId
    );

ULONG64 EtLookupProcessGpuSharedUsage(
    _In_opt_ HANDLE ProcessId
    );

ULONG64 EtLookupProcessGpuCommitUsage(
    _In_opt_ HANDLE ProcessId
    );

#endif
