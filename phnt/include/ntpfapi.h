/*
 * Prefetcher (Superfetch) support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTPFAPI_H
#define _NTPFAPI_H

// begin_private

// Prefetch

typedef enum _PF_BOOT_PHASE_ID
{
    PfKernelInitPhase = 0,
    PfBootDriverInitPhase = 90,
    PfSystemDriverInitPhase = 120,
    PfSessionManagerInitPhase = 150,
    PfSMRegistryInitPhase = 180,
    PfVideoInitPhase = 210,
    PfPostVideoInitPhase = 240,
    PfBootAcceptedRegistryInitPhase = 270,
    PfUserShellReadyPhase = 300,
    PfMaxBootPhaseId = 900
} PF_BOOT_PHASE_ID;

typedef enum _PF_ENABLE_STATUS
{
    PfSvNotSpecified,
    PfSvEnabled,
    PfSvDisabled,
    PfSvMaxEnableStatus
} PF_ENABLE_STATUS;

typedef struct _PF_TRACE_LIMITS
{
    ULONG MaxNumPages;
    ULONG MaxNumSections;
    LONGLONG TimerPeriod;
} PF_TRACE_LIMITS, *PPF_TRACE_LIMITS;

typedef struct _PF_SYSTEM_PREFETCH_PARAMETERS
{
    PF_ENABLE_STATUS EnableStatus[2];
    PF_TRACE_LIMITS TraceLimits[2];
    ULONG MaxNumActiveTraces;
    ULONG MaxNumSavedTraces;
    WCHAR RootDirPath[32];
    WCHAR HostingApplicationList[128];
} PF_SYSTEM_PREFETCH_PARAMETERS, *PPF_SYSTEM_PREFETCH_PARAMETERS;

#define PF_BOOT_CONTROL_VERSION 1

typedef struct _PF_BOOT_CONTROL
{
    ULONG Version;
    ULONG DisableBootPrefetching;
} PF_BOOT_CONTROL, *PPF_BOOT_CONTROL;

typedef enum _PREFETCHER_INFORMATION_CLASS
{
    PrefetcherRetrieveTrace = 1, // q: CHAR[]
    PrefetcherSystemParameters, // q: PF_SYSTEM_PREFETCH_PARAMETERS
    PrefetcherBootPhase, // s: PF_BOOT_PHASE_ID
    PrefetcherSpare1, // PrefetcherRetrieveBootLoaderTrace // q: CHAR[]
    PrefetcherBootControl, // s: PF_BOOT_CONTROL
    PrefetcherScenarioPolicyControl,
    PrefetcherSpare2,
    PrefetcherAppLaunchScenarioControl,
    PrefetcherInformationMax
} PREFETCHER_INFORMATION_CLASS;

#define PREFETCHER_INFORMATION_VERSION 23 // rev
#define PREFETCHER_INFORMATION_MAGIC ('kuhC') // rev

typedef struct _PREFETCHER_INFORMATION
{
    _In_ ULONG Version;
    _In_ ULONG Magic;
    _In_ PREFETCHER_INFORMATION_CLASS PrefetcherInformationClass;
    _Inout_ PVOID PrefetcherInformation;
    _Inout_ ULONG PrefetcherInformationLength;
} PREFETCHER_INFORMATION, *PPREFETCHER_INFORMATION;

// Superfetch

typedef struct _PF_SYSTEM_SUPERFETCH_PARAMETERS
{
    ULONG EnabledComponents;
    ULONG BootID;
    ULONG SavedSectInfoTracesMax;
    ULONG SavedPageAccessTracesMax;
    ULONG ScenarioPrefetchTimeoutStandby;
    ULONG ScenarioPrefetchTimeoutHibernate;
    ULONG ScenarioPrefetchTimeoutHiberBoot;
} PF_SYSTEM_SUPERFETCH_PARAMETERS, *PPF_SYSTEM_SUPERFETCH_PARAMETERS;

#define PF_PFN_PRIO_REQUEST_VERSION 1
#define PF_PFN_PRIO_REQUEST_QUERY_MEMORY_LIST 0x1
#define PF_PFN_PRIO_REQUEST_VALID_FLAGS 0x1

typedef struct _PF_PFN_PRIO_REQUEST
{
    ULONG Version;
    ULONG RequestFlags;
    ULONG_PTR PfnCount;
    SYSTEM_MEMORY_LIST_INFORMATION MemInfo;
    MMPFN_IDENTITY PageData[256];
} PF_PFN_PRIO_REQUEST, *PPF_PFN_PRIO_REQUEST;

typedef enum _PFS_PRIVATE_PAGE_SOURCE_TYPE
{
    PfsPrivateSourceKernel,
    PfsPrivateSourceSession,
    PfsPrivateSourceProcess,
    PfsPrivateSourceMax
} PFS_PRIVATE_PAGE_SOURCE_TYPE;

typedef struct _PFS_PRIVATE_PAGE_SOURCE
{
    PFS_PRIVATE_PAGE_SOURCE_TYPE Type;
    union
    {
        ULONG SessionId;
        ULONG ProcessId;
    };
    ULONG ImagePathHash;
    ULONG_PTR UniqueProcessHash;
} PFS_PRIVATE_PAGE_SOURCE, *PPFS_PRIVATE_PAGE_SOURCE;

typedef struct _PF_PRIVSOURCE_INFO
{
    PFS_PRIVATE_PAGE_SOURCE DbInfo;
    PVOID EProcess;
    SIZE_T WsPrivatePages;
    SIZE_T TotalPrivatePages;
    ULONG SessionID;
    CHAR ImageName[16];
    union {
        ULONG_PTR WsSwapPages;                 // process only PF_PRIVSOURCE_QUERY_WS_SWAP_PAGES.
        ULONG_PTR SessionPagedPoolPages;       // session only.
        ULONG_PTR StoreSizePages;              // process only PF_PRIVSOURCE_QUERY_STORE_INFO.
    };
    ULONG_PTR WsTotalPages;         // process/session only.
    ULONG DeepFreezeTimeMs;         // process only.
    ULONG ModernApp : 1;            // process only.
    ULONG DeepFrozen : 1;           // process only. If set, DeepFreezeTimeMs contains the time at which the freeze occurred
    ULONG Foreground : 1;           // process only.
    ULONG PerProcessStore : 1;      // process only.
    ULONG Spare : 28;
} PF_PRIVSOURCE_INFO, *PPF_PRIVSOURCE_INFO;

#define PF_PRIVSOURCE_QUERY_REQUEST_VERSION 8

typedef struct _PF_PRIVSOURCE_QUERY_REQUEST
{
    ULONG Version;
    ULONG Flags;
    ULONG InfoCount;
    PF_PRIVSOURCE_INFO InfoArray[1];
} PF_PRIVSOURCE_QUERY_REQUEST, *PPF_PRIVSOURCE_QUERY_REQUEST;

typedef enum _PF_PHASED_SCENARIO_TYPE
{
    PfScenarioTypeNone,
    PfScenarioTypeStandby,
    PfScenarioTypeHibernate,
    PfScenarioTypeFUS,
    PfScenarioTypeMax
} PF_PHASED_SCENARIO_TYPE;

#define PF_SCENARIO_PHASE_INFO_VERSION 4

typedef struct _PF_SCENARIO_PHASE_INFO
{
    ULONG Version;
    PF_PHASED_SCENARIO_TYPE ScenType;
    ULONG PhaseId;
    ULONG SequenceNumber;
    ULONG Flags;
    ULONG FUSUserId;
} PF_SCENARIO_PHASE_INFO, *PPF_SCENARIO_PHASE_INFO;

typedef struct _PF_MEMORY_LIST_NODE
{
    ULONGLONG Node : 8;
    ULONGLONG Spare : 56;
    ULONGLONG StandbyLowPageCount;
    ULONGLONG StandbyMediumPageCount;
    ULONGLONG StandbyHighPageCount;
    ULONGLONG FreePageCount;
    ULONGLONG ModifiedPageCount;
} PF_MEMORY_LIST_NODE, *PPF_MEMORY_LIST_NODE;

typedef struct _PF_TIME_CONTROL
{
	long TimeAdjustment;
} PF_TIME_CONTROL, *PPF_TIME_CONTROL;

#define PF_MEMORY_LIST_INFO_VERSION 1

typedef struct _PF_MEMORY_LIST_INFO
{
    ULONG Version;
    ULONG Size;
    ULONG NodeCount;
    PF_MEMORY_LIST_NODE Nodes[1];
} PF_MEMORY_LIST_INFO, *PPF_MEMORY_LIST_INFO;

typedef struct _PF_PHYSICAL_MEMORY_RANGE
{
    ULONG_PTR BasePfn;
    ULONG_PTR PageCount;
} PF_PHYSICAL_MEMORY_RANGE, *PPF_PHYSICAL_MEMORY_RANGE;

#define PF_PHYSICAL_MEMORY_RANGE_INFO_V1_VERSION 1

typedef struct _PF_PHYSICAL_MEMORY_RANGE_INFO_V1
{
    ULONG Version;
    ULONG RangeCount;
    PF_PHYSICAL_MEMORY_RANGE Ranges[1];
} PF_PHYSICAL_MEMORY_RANGE_INFO_V1, *PPF_PHYSICAL_MEMORY_RANGE_INFO_V1;

#define PF_PHYSICAL_MEMORY_RANGE_INFO_V2_VERSION 2

typedef struct _PF_PHYSICAL_MEMORY_RANGE_INFO_V2
{
    ULONG Version;
    ULONG Flags;
    ULONG RangeCount;
    PF_PHYSICAL_MEMORY_RANGE Ranges[ANYSIZE_ARRAY];
} PF_PHYSICAL_MEMORY_RANGE_INFO_V2, *PPF_PHYSICAL_MEMORY_RANGE_INFO_V2;

// begin_rev

#define PF_REPURPOSED_BY_PREFETCH_INFO_VERSION 1

typedef struct _PF_REPURPOSED_BY_PREFETCH_INFO
{
    ULONG Version;
    ULONG RepurposedByPrefetch;
} PF_REPURPOSED_BY_PREFETCH_INFO, *PPF_REPURPOSED_BY_PREFETCH_INFO;

// end_rev

typedef enum _SUPERFETCH_INFORMATION_CLASS
{
    SuperfetchRetrieveTrace = 1, // q: CHAR[]
    SuperfetchSystemParameters, // q: PF_SYSTEM_SUPERFETCH_PARAMETERS
    SuperfetchLogEvent,
    SuperfetchGenerateTrace,
    SuperfetchPrefetch,
    SuperfetchPfnQuery, // q: PF_PFN_PRIO_REQUEST
    SuperfetchPfnSetPriority,
    SuperfetchPrivSourceQuery, // q: PF_PRIVSOURCE_QUERY_REQUEST
    SuperfetchSequenceNumberQuery, // q: ULONG
    SuperfetchScenarioPhase, // 10
    SuperfetchWorkerPriority,
    SuperfetchScenarioQuery, // q: PF_SCENARIO_PHASE_INFO
    SuperfetchScenarioPrefetch,
    SuperfetchRobustnessControl,
    SuperfetchTimeControl, // PF_TIME_CONTROL
    SuperfetchMemoryListQuery, // q: PF_MEMORY_LIST_INFO
    SuperfetchMemoryRangesQuery, // q: PF_PHYSICAL_MEMORY_RANGE_INFO
    SuperfetchTracingControl,
    SuperfetchTrimWhileAgingControl,
    SuperfetchRepurposedByPrefetch, // q: PF_REPURPOSED_BY_PREFETCH_INFO // rev // 20
    SuperfetchChannelPowerRequest,
    SuperfetchMovePages,
    SuperfetchVirtualQuery,
    SuperfetchCombineStatsQuery,
    SuperfetchSetMinWsAgeRate,
    SuperfetchDeprioritizeOldPagesInWs,
    SuperfetchFileExtentsQuery,
    SuperfetchGpuUtilizationQuery, // PF_GPU_UTILIZATION_INFO
    SuperfetchInformationMax
} SUPERFETCH_INFORMATION_CLASS;

#define SUPERFETCH_INFORMATION_VERSION 45 // rev
#define SUPERFETCH_INFORMATION_MAGIC ('kuhC') // rev

typedef struct _SUPERFETCH_INFORMATION
{
    _In_ ULONG Version;
    _In_ ULONG Magic;
    _In_ SUPERFETCH_INFORMATION_CLASS SuperfetchInformationClass;
    _Inout_ PVOID SuperfetchInformation;
    _Inout_ ULONG SuperfetchInformationLength;
} SUPERFETCH_INFORMATION, *PSUPERFETCH_INFORMATION;

// end_private

#endif
