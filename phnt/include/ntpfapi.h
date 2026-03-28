/*
 * Prefetcher (Superfetch) support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTPFAPI_H
#define _NTPFAPI_H

// begin_private

//
// Prefetch
//

typedef enum _PREFETCHER_INFORMATION_CLASS
{
    PrefetcherRetrieveTrace = 1,            // q: PF_RETRIEVE_TRACE
    PrefetcherSystemParameters,             // q: PF_SYSTEM_PREFETCH_PARAMETERS
    PrefetcherBootPhase,                    // s: PF_BOOT_PHASE_ID
    PrefetcherSpare1,                       // q: PrefetcherRetrieveBootLoaderTrace
    PrefetcherOperationProcess,             // s: PF_OPERATION_PROCESS
    PrefetcherCacheEntryUpdate,             // s: PF_CACHE_ENTRY_UPDATE
    PrefetcherSpare2,
    PrefetcherAppLaunchScenarioControl,     // s: PF_APP_LAUNCH_SCENARIO_CONTROL
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

// rev
#define PF_RETRIEVE_TRACE_VERSION 31 // rev
#define PF_RETRIEVE_TRACE_MAGIC ('ASCC') // rev

// rev
typedef struct _PF_RETRIEVE_TRACE_PAGE_ENTRY
{
    UCHAR Data[16];
} PF_RETRIEVE_TRACE_PAGE_ENTRY, *PPF_RETRIEVE_TRACE_PAGE_ENTRY;

// rev
typedef struct _PF_RETRIEVE_TRACE_SECTION_ENTRY
{
    UCHAR Data[24];
} PF_RETRIEVE_TRACE_SECTION_ENTRY, *PPF_RETRIEVE_TRACE_SECTION_ENTRY;

// rev
typedef struct _PF_RETRIEVE_TRACE
{
    ULONG Version; // PF_RETRIEVE_TRACE_VERSION
    ULONG Magic; // PF_RETRIEVE_TRACE_MAGIC
    ULONG Size; // Total buffer size, including the variable payload that follows this header.
    ULONG Spare0; // Opaque trace state copied from the completed trace.
    UCHAR OpaqueHeader[0x40]; // Opaque fixed fields copied from the completed trace.
    ULONG PageEntriesOffset; // Offset from the start of this structure to PF_RETRIEVE_TRACE_PAGE_ENTRY[PageEntriesCount].
    ULONG PageEntriesCount; // Count of 16-byte PF_RETRIEVE_TRACE_PAGE_ENTRY records.
    ULONG TraceLimitsMaxNumPages;
    ULONG TraceLimitsMaxNumSections;
    ULONG SectionEntriesOffset; // Offset from the start of this structure to PF_RETRIEVE_TRACE_SECTION_ENTRY[SectionEntriesCount].
    ULONG SectionEntriesCount; // Count of 24-byte PF_RETRIEVE_TRACE_SECTION_ENTRY records.
    UCHAR OpaqueTail[0x30]; // Opaque fixed fields copied from the completed trace.
} PF_RETRIEVE_TRACE, *PPF_RETRIEVE_TRACE;

typedef enum _PF_ENABLE_STATUS
{
    PfSvNotSpecified,
    PfSvEnabled,
    PfSvDisabled,
    PfSvMaxEnableStatus
} PF_ENABLE_STATUS;

// rev
typedef struct _PF_TRACE_LIMITS
{
    ULONG MaxNumPages;
    ULONG MaxNumSections;
    LONGLONG TimerPeriod;
} PF_TRACE_LIMITS, *PPF_TRACE_LIMITS;

// rev
typedef struct _PF_SYSTEM_PREFETCH_PARAMETERS
{
    PF_ENABLE_STATUS EnableStatus[2];
    PF_TRACE_LIMITS TraceLimits[2];
    ULONG MaxNumActiveTraces;
    ULONG MaxNumSavedTraces;
    WCHAR RootDirPath[32];
    WCHAR HostingApplicationList[128];
} PF_SYSTEM_PREFETCH_PARAMETERS, *PPF_SYSTEM_PREFETCH_PARAMETERS;

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

#define PF_SN_OPERATION_PROCESS_VERSION 1

typedef enum _PF_OPERATION_PROCESS_ACTION
{
    PfSnOpProcessBegin = 0,
    PfSnOpProcessEnd = 1,
    PfSnOpProcessMax = 2
} PF_OPERATION_PROCESS_ACTION;

typedef struct _PF_OPERATION_PROCESS
{
    UCHAR Version;
    UCHAR Action; // PF_SYSTEM_OPERATION_PROCESS_ACTION
    USHORT Reserved;
    ULONG OpFlags;
    ULONG Value;
} PF_OPERATION_PROCESS, *PPF_OPERATION_PROCESS;

#define PF_BOOT_CONTROL_VERSION 1

typedef struct _PF_BOOT_CONTROL
{
    ULONG Version;
    ULONG DisableBootPrefetching;
} PF_BOOT_CONTROL, *PPF_BOOT_CONTROL;

#define PF_CACHE_ENTRY_UPDATE_VERSION 2

typedef struct _PF_CACHE_ENTRY_UPDATE
{
    ULONG Version;
    UCHAR Name[64];
    ULONG NewValue;
} PF_CACHE_ENTRY_UPDATE, *PPF_CACHE_ENTRY_UPDATE;

#define PF_APP_LAUNCH_SCENARIO_CONTROL_VERSION 1

typedef struct _PF_APP_LAUNCH_SCENARIO_CONTROL
{
    ULONG Version;
    ULONG Enable; // must be non-zero
    HANDLE ProcessHandle;
} PF_APP_LAUNCH_SCENARIO_CONTROL, *PPF_APP_LAUNCH_SCENARIO_CONTROL;

//
// Superfetch
//

// rev
typedef enum _SUPERFETCH_INFORMATION_CLASS
{
    SuperfetchRetrieveTrace = 1,               // q: PF_SYSTEM_SUPERFETCH_RETRIEVE_TRACE // PfGetCompletedTrace
    SuperfetchSystemParameters,                // q: PF_SYSTEM_SUPERFETCH_PARAMETERS
    SuperfetchLogEvent,                        // s: PF_LOG_EVENT_DATA
    SuperfetchGenerateTrace,                   // s: PF_GENERATE_TRACE_CONTROL
    SuperfetchPrefetch,                        // s: PF_PREFETCH_REQUEST
    SuperfetchPfnQuery,                        // q: PF_PFN_PRIO_REQUEST
    SuperfetchPfnSetPriority,                  // s: PF_PFN_PRIO_REQUEST // MmSetPfnListInfo
    SuperfetchPrivSourceQuery,                 // q: PF_PRIVSOURCE_QUERY_REQUEST
    SuperfetchSequenceNumberQuery,             // q: ULONG
    SuperfetchScenarioPhase,                   // s: PF_SCENARIO_PHASE_INFO // 10
    SuperfetchWorkerPriority,                  // s: PF_WORKER_PRIORITY_CONTROL
    SuperfetchScenarioQuery,                   // q: PF_SCENARIO_QUERY_INFO
    SuperfetchScenarioPrefetch,                // s: PF_SCENARIO_PREFETCH_INFO
    SuperfetchRobustnessControl,               // s: PF_ROBUSTNESS_CONTROL
    SuperfetchTimeControl,                     // s: PF_TIME_CONTROL
    SuperfetchMemoryListQuery,                 // q: PF_MEMORY_LIST_INFO
    SuperfetchMemoryRangesQuery,               // q: PF_PHYSICAL_MEMORY_RANGE_INFO_V1/V2
    SuperfetchTracingControl,                  // s: PF_ACCESS_TRACING_CONTROL
    SuperfetchTrimWhileAgingControl,           // s: PF_TRIM_WHILE_AGING_CONTROL
    SuperfetchRepurposedByPrefetch,            // q: PF_REPURPOSED_BY_PREFETCH_INFO // 20
    SuperfetchChannelPowerRequest,             // q: not implemented
    SuperfetchMovePages,                       // s: PF_PFN_PRIO_REQUEST // MmRelocatePfnList
    SuperfetchVirtualQuery,                    // q: PF_VIRTUAL_QUERY
    SuperfetchCombineStatsQuery,               // q: PF_PAGECOMBINE_AGGREGATE_STAT
    SuperfetchSetMinWsAgeRate,                 // s: PF_MIN_WS_AGE_RATE_CONTROL
    SuperfetchDeprioritizeOldPagesInWs,        // s: PF_DEPRIORITIZE_OLD_PAGES
    SuperfetchFileExtentsQuery,                // q: PF_FILE_EXTENTS_INFO
    SuperfetchGpuUtilizationQuery,             // q: PF_GPU_UTILIZATION_INFO
    SuperfetchPfnSet,                          // s: PF_PFN_PRIO_REQUEST // since WIN11
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

// rev
typedef struct _PF_SYSTEM_SUPERFETCH_RETRIEVE_TRACE
{
    union
    {
        struct
        {
            ULONGLONG RequestType;     // Must be 2 for "get completed trace"
            ULONGLONG Reserved;        // Ignored on input
            HANDLE    PartitionHandle;
        } Input;
        struct
        {
            ULONGLONG TypeFlags;       // 0x0000000000180002 on success
            ULONGLONG Timestamp;       // Scaled TSC value
            HANDLE    PartitionHandle;
        } Output;

        //
        // Raw view of the buffer for opaque access
        //
        UCHAR Buffer[ANYSIZE_ARRAY];
    };
} PF_SYSTEM_SUPERFETCH_RETRIEVE_TRACE, *PPF_SYSTEM_SUPERFETCH_RETRIEVE_TRACE;

// rev
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

// rev
typedef struct _PF_PREFETCH_RANGE_INFO
{
    ULONG RangeCount;
    ULONG Reserved;
    ULONG RangesOffset;
} PF_PREFETCH_RANGE_INFO, *PPF_PREFETCH_RANGE_INFO;

// rev
typedef struct _PF_PREFETCH_PATH_INFO
{
    ULONG PathOffset;
    ULONG Reserved;
    ULONG PathLength; // WCHAR count, excluding terminator
} PF_PREFETCH_PATH_INFO, *PPF_PREFETCH_PATH_INFO;

// rev
typedef struct _PF_PREFETCH_FILE_INFO
{
    ULONG Flags;
    ULONG Reserved0;
    ULONGLONG Reserved1; // non-zero for some file classes
    PF_PREFETCH_RANGE_INFO Ranges;
    ULONG Reserved2;
    PWSTR Path;          // patched from PathOffset during processing
    ULONG PathLength;    // WCHAR count, excluding terminator
    ULONG Reserved3;
} PF_PREFETCH_FILE_INFO, *PPF_PREFETCH_FILE_INFO;

// rev
typedef struct _PF_PREFETCH_VOLUME_INFO
{
    LARGE_INTEGER VolumeCreationTime;
    ULONG VolumeSerialNumber;
    ULONG Flags;
    ULONG PrefetchFileCountTimes2;   // number of PF_PREFETCH_FILE_INFO entries << 1
    ULONG PrefetchFileInfoOffset;    // offset to PF_PREFETCH_FILE_INFO array
    USHORT VolumePathLength;         // WCHAR count, excluding terminator
    USHORT Reserved0;
    ULONG Reserved1;
    PWSTR VolumePath;                // patched from VolumePathOffset during processing
} PF_PREFETCH_VOLUME_INFO, *PPF_PREFETCH_VOLUME_INFO;

// rev
typedef struct _PF_PREFETCH_SOURCE_INFO
{
    ULONG Reserved0;
    ULONG Reserved1;
    ULONG Reserved2;
    ULONG Reserved3;
    struct _PF_PHYSICAL_MEMORY_RANGE *Ranges; // patched from RangesOffset during processing
    PF_PREFETCH_RANGE_INFO RangeInfo;
    ULONG Reserved4;
} PF_PREFETCH_SOURCE_INFO, *PPF_PREFETCH_SOURCE_INFO;

#define PF_PREFETCH_REQUEST_VERSION 13

// rev
typedef struct _PF_PREFETCH_REQUEST
{
    ULONG Version;                   // PF_PREFETCH_REQUEST_VERSION
    ULONG Size;                      // entire buffer size
    ULONG VolumeCount;
    ULONG PrefetchFileCount;
    ULONG PathCount;
    ULONG SourceCount;
    ULONG UnicodeTextSize;
    USHORT PrefetchPriority;
    USHORT VolumePrefetchPriority;
    ULONG VolumeInfoOffset;
    ULONG Reserved0;
    ULONG PrefetchFileInfoOffset;
    ULONG Reserved1;
    ULONG PathInfoOffset;
    ULONG Reserved2;
    ULONG SourceInfoOffset;
    ULONG Reserved3;
    ULONG UnicodeTextOffset;
    ULONG Reserved4;
    PVOID CompletionEvent;           // optional KEVENT handle on input, object pointer after capture
    union
    {
        ULONG Flags;
        struct
        {
            ULONG ContinueIO : 1;
            ULONG LowPowerMode : 1;
            ULONG UseScenarioTimeouts : 1;
            ULONG ReservedFlags : 29;
        };
    };
    UCHAR ScenarioType;              // must be < 6
    UCHAR Reserved5[3];
    ULONG MetadataPrefetchCount;
    ULONG PrivatePrefetchCount;
    ULONG Reserved6;
    ULONG PrivatePageCount;
    ULONG MetadataPrefetchTime;
    ULONG Reserved7;
    ULONG Reserved8;
    ULONG PrivatePrefetchTime;
    ULONG Reserved9;
    ULONG Reserved10;
} PF_PREFETCH_REQUEST, *PPF_PREFETCH_REQUEST;

// rev
typedef enum _PF_EVENT_TYPE
{
    PfEventTypeImageLoad = 0,
    PfEventTypeAppLaunch = 1,
    PfEventTypeStartTrace = 2,
    PfEventTypeEndTrace = 3,
    PfEventTypeTimestamp = 4,
    PfEventTypeOperation = 5,
    PfEventTypeRepurpose = 6,
    PfEventTypeForegroundProcess = 7,
    PfEventTypeTimeRange = 8,
    PfEventTypeUserInput = 9,
    PfEventTypeFileAccess = 10,
    PfEventTypeUnmap = 11,
    PfEventTypeUtilization = 11,
    PfEventTypeMemInfo = 12,
    PfEventTypeFileDelete = 13,
    PfEventTypeAppExit = 14,
    PfEventTypeSystemTime = 15,
    PfEventTypePower = 16,
    PfEventTypeSessionChange = 17,
    PfEventTypeHardFaultTimeStamp = 18,
    PfEventTypeVirtualFree = 19,
    PfEventTypePerfInfo = 20,
    PfEventTypeProcessSnapshot = 21,
    PfEventTypeUserSnapshot = 22,
    PfEventTypeStreamSequenceNumber = 23,
    PfEventTypeFileTruncate = 24,
    PfEventTypeFileRename = 25,
    PfEventTypeFileCreate = 26,
    PfEventTypeAgCxContext = 27,
    PfEventTypePowerAction = 28,
    PfEventTypeHardFaultTS = 29,
    PfEventTypeRobustInfo = 30,
    PfEventTypeFileDefrag = 31,
    PfEventTypeMax = 32
} PF_EVENT_TYPE;

#define PF_LOG_EVENT_DATA_VERSION 1

typedef struct _PF_LOG_EVENT_DATA
{
    ULONG Version; // PF_LOG_EVENT_DATA_VERSION
    union
    {
        ULONG Packed; // [31:7]=DataSize, [6:5]=Flags, [4:0]=EventType (PF_EVENT_TYPE)
        struct
        {
            ULONG DataSize : 25; // in bytes
            ULONG Flags    : 2;
            ULONG EventType: 5; // 2,3,5,27 accepted by the handler // PF_EVENT_TYPE
        };
    };
    PVOID EventData;
    HANDLE PartitionHandle;
} PF_LOG_EVENT_DATA , *PPF_LOG_EVENT_DATA ;

typedef struct _PFN_TRIPLET
{
    ULONGLONG MaskOrKey;        // Compared against identity with 0x1FFFFFFFFFFFE00 mask
    ULONGLONG Pfn;              // Page frame number
    ULONGLONG Flags;            // Request/result flags
} PFN_TRIPLET, *PPFN_TRIPLET;

#define PF_PFN_PRIO_REQUEST_VERSION 1
#define PF_PFN_PRIO_REQUEST_QUERY_MEMORY_LIST 0x1
#define PF_PFN_PRIO_REQUEST_VALID_FLAGS 0x1

typedef struct _PF_PFN_PRIO_REQUEST
{
    ULONG Version;
    ULONG RequestFlags;
    SIZE_T PfnCount;
    SYSTEM_MEMORY_LIST_INFORMATION MemInfo;
    union
    {
        // Input: (class 6/16) MmQueryPfnList fills identities here
        MMPFN_IDENTITY PageIdentities[256]; // ANYSIZE_ARRAY
        // Output: (class 7/29/1D) caller supplies PFN_TRIPLETs here
        PFN_TRIPLET Entries[256]; // ANYSIZE_ARRAY
    };
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
    UCHAR ImageName[16];
    union
    {
        SIZE_T WsSwapPages;                 // process only PF_PRIVSOURCE_QUERY_WS_SWAP_PAGES.
        SIZE_T SessionPagedPoolPages;       // session only.
        SIZE_T StoreSizePages;              // process only PF_PRIVSOURCE_QUERY_STORE_INFO.
    };
    SIZE_T WsTotalPages;            // process/session only.
    ULONG DeepFreezeTimeMs;         // process only.
    ULONG ModernApp : 1;            // process only.
    ULONG DeepFrozen : 1;           // process only. If set, DeepFreezeTimeMs contains the time at which the freeze occurred
    ULONG Foreground : 1;           // process only.
    ULONG PerProcessStore : 1;      // process only.
    ULONG Spare : 28;
} PF_PRIVSOURCE_INFO, *PPF_PRIVSOURCE_INFO;

// rev
#define PF_PRIVSOURCE_QUERY_REQUEST_VERSION 8
#define PF_PRIVSOURCE_QUERY_REQUEST_FLAGS_QUERYWSPAGES 0x1
#define PF_PRIVSOURCE_QUERY_REQUEST_FLAGS_QUERYCOMPRESSEDPAGES 0x2
#define PF_PRIVSOURCE_QUERY_REQUEST_FLAGS_QUERYSKIPPAGES 0x4 // ??

// rev
typedef struct _PF_PRIVSOURCE_QUERY_REQUEST
{
    ULONG Version;
    ULONG Flags;
    ULONG InfoCount;
    PF_PRIVSOURCE_INFO InfoArray[1];
} PF_PRIVSOURCE_QUERY_REQUEST, *PPF_PRIVSOURCE_QUERY_REQUEST;

// rev
typedef enum _PF_PHASED_SCENARIO_TYPE
{
    PfScenarioTypeNone,
    PfScenarioTypeStandby,
    PfScenarioTypeHibernate,
    PfScenarioTypeFUS,
    PfScenarioTypeMax
} PF_PHASED_SCENARIO_TYPE;

// rev
#define PF_SCENARIO_PHASE_INFO_VERSION 4

// rev
typedef struct _PF_SCENARIO_PHASE_INFO
{
    ULONG Version;
    PF_PHASED_SCENARIO_TYPE ScenType;
    ULONG PhaseId;
    ULONG SequenceNumber;
    ULONG Flags;
    ULONG FUSUserId;
    ULONG Reserved0; // pad to 32 bytes (handler expects 32)
    ULONG Reserved1;
} PF_SCENARIO_PHASE_INFO, *PPF_SCENARIO_PHASE_INFO;

// rev
typedef struct _PF_WORKER_PRIORITY_CONTROL
{
    ULONG Version;
    KPRIORITY Priority; // 0..31 (STATUS_INVALID_PARAMETER if >31)
    HANDLE PartitionHandle;
} PF_WORKER_PRIORITY_CONTROL, *PPF_WORKER_PRIORITY_CONTROL;

// rev
#define PF_SCENARIO_QUERY_INFO_VERSION 4

// rev
typedef struct _PF_SCENARIO_QUERY_INFO
{
    ULONG_PTR Version;
    ULONG_PTR Field1;
    ULONG_PTR Field2;
    ULONG_PTR Field3;
} PF_SCENARIO_QUERY_INFO , *PPF_SCENARIO_QUERY_INFO;

// rev
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

// rev
typedef struct _PF_ROBUST_PROCESS_ENTRY
{
    ULONG ImagePathHash;
    ULONG Pid;
} PF_ROBUST_PROCESS_ENTRY, *PPF_ROBUST_PROCESS_ENTRY;

// rev
typedef struct _PF_ROBUST_FILE_ENTRY
{
    ULONGLONG FilePathHash;
} PF_ROBUST_FILE_ENTRY, *PPF_ROBUST_FILE_ENTRY;

// rev
typedef enum _PF_ROBUSTNESS_CONTROL_COMMAND
{
    PfRpControlUpdate = 0,
    PfRpControlReset = 1,
    PfRpControlRobustAllStart = 2,
    PfRpControlRobustAllStop = 3,
    PfRpControlCommandMax = 4
} PF_ROBUSTNESS_CONTROL_COMMAND;

// rev
#define PF_ROBUSTNESS_CONTROL_VERSION 3

// rev
typedef struct _PF_ROBUSTNESS_CONTROL
{
    union
    {
        ULONG VersionAndCommand;
        struct
        {
            USHORT Version; // PF_ROBUSTNESS_CONTROL_VERSION
            USHORT Command; // PF_ROBUSTNESS_CONTROL_COMMAND
        };
    };
    ULONG DeprioProcessCount;
    ULONG ExemptProcessCount;
    ULONG DeprioFileCount;
    ULONG ExemptFileCount;
    ULONG Reserved;
    //
    // Variable-size payload:
    //   PF_ROBUST_PROCESS_ENTRY ProcessEntries[DeprioProcessCount + ExemptProcessCount];
    //   PF_ROBUST_FILE_ENTRY FileEntries[DeprioFileCount + ExemptFileCount];
    //
    UCHAR Buffer[ANYSIZE_ARRAY];
} PF_ROBUSTNESS_CONTROL, *PPF_ROBUSTNESS_CONTROL;

// rev
typedef struct _PF_SCENARIO_PREFETCH_INFO
{
    ULONG Version;
    ULONG State;
} PF_SCENARIO_PREFETCH_INFO, *PPF_SCENARIO_PREFETCH_INFO;

// rev
#define PF_TRIM_WHILE_AGING_CONTROL_VERSION_1 1

// rev
typedef enum _PF_TRIM_WHILE_AGING_STATE
{
    PfTrimWhileAgingOff = 0,
    PfTrimWhileAgingLowPriority = 1,
    PfTrimWhileAgingPassive = 2,
    PfTrimWhileAgingNormal = 3,
    PfTrimWhileAgingAggressive = 4,
    PfTrimWhileAgingMax = 5,
} PF_TRIM_WHILE_AGING_STATE, *PPF_TRIM_WHILE_AGING_STATE;

// rev
typedef struct _PF_TRIM_WHILE_AGING_CONTROL_1
{
    ULONG Version;
    PF_TRIM_WHILE_AGING_STATE TrimWhileAgingState;
    BOOLEAN PrivatePageTrimAge;
    BOOLEAN SharedPageTrimAge;
    USHORT Spare;
} PF_TRIM_WHILE_AGING_CONTROL_1, *PPF_TRIM_WHILE_AGING_CONTROL_1;

#define PF_TRIM_WHILE_AGING_CONTROL_VERSION_2 2

// rev
typedef struct _PF_TRIM_WHILE_AGING_CONTROL_2
{
    ULONG Version;
    PF_TRIM_WHILE_AGING_STATE TrimWhileAgingState;
    UCHAR PrivatePageTrimAge;      // 0..7
    UCHAR SharedPageTrimAge;       // 0..7
    USHORT Spare;                  // must be 0
} PF_TRIM_WHILE_AGING_CONTROL_2, *PPF_TRIM_WHILE_AGING_CONTROL_2;

// rev
typedef struct _PF_TIME_CONTROL
{
    LONG TimeAdjustment;
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
    SIZE_T RangeCount;
    PF_PHYSICAL_MEMORY_RANGE Ranges[1];
} PF_PHYSICAL_MEMORY_RANGE_INFO_V2, *PPF_PHYSICAL_MEMORY_RANGE_INFO_V2;

// rev
typedef struct _PF_START_TRACE_CONTROL
{
    ULONG Version;             // 3
    ULONG Type;                // 0 or 1
    ULONG Flags;               // 0..3
    ULONG Reserved;
    HANDLE PartitionHandle;    // in
    HANDLE TraceHandle;        // in/out; output when Type == 0
} PF_START_TRACE_CONTROL, *PPF_START_TRACE_CONTROL;

// rev
#define PF_ACCESS_TRACING_CONTROL_VERSION 3

// rev
typedef struct _PF_ACCESS_TRACING_CONTROL
{
    ULONG Version;             // PF_ACCESS_TRACING_CONTROL_VERSION
    ULONG Type;                // 0 or 1
    ULONG Flags;               // 0..3
    ULONG Reserved;
    HANDLE PartitionHandle;    // in
    HANDLE TraceHandle;        // in/out; output when Type == 0
} PF_ACCESS_TRACING_CONTROL, *PPF_ACCESS_TRACING_CONTROL;

// rev
#define PF_REPURPOSED_BY_PREFETCH_INFO_VERSION 1

// rev
typedef struct _PF_REPURPOSED_BY_PREFETCH_INFO
{
    ULONG Version; // PF_REPURPOSED_BY_PREFETCH_INFO_VERSION
    ULONG Reserved;
    SIZE_T RepurposedByPrefetch;
} PF_REPURPOSED_BY_PREFETCH_INFO, *PPF_REPURPOSED_BY_PREFETCH_INFO;

// rev
#define PF_VIRTUAL_QUERY_VERSION 1

// rev
typedef struct _PF_VIRTUAL_QUERY
{
    ULONG Version;
    union
    {
        ULONG Flags;
        struct
        {
            /*
             * Fault page-table state in for the queried user VAs.
             * From MiWorkingSetInfoCheckPageTable, when the queried user address has a leaf PTE that is in transition or non-present with a paging-file offset,
             * this takes the MiMakeSystemAddressValid path to make the backing page-table state resident before retrying the query.
             * Use this for ordinary committed user pages when you want page-table faults resolved rather than merely reported.
             * Mutually exclusive with ReportPageTables.
             */
            ULONG FaultInPageTables : 1;
            /*
             * Report page-table-related state for the queried user VAs in the returned MEMORY_WORKING_SET_EX_INFORMATION entries.
             * From MiWorkingSetInfoCheckPageTable, this produces PageTable = 1 when the queried user address has a leaf PTE that is in transition or non-present with a paging-file offset,
             * i.e. ordinary committed user pages after trimming/page-out. Mutually exclusive with FaultInPageTables.
             */
            ULONG ReportPageTables : 1;
            ULONG Spare : 30;
        };
    };
    PVOID QueryBuffer; // MEMORY_WORKING_SET_EX_INFORMATION[NumberOfPages] (input: VirtualAddress[], output: VirtualAttributes[])
    SIZE_T QueryBufferSize; // NumberOfPages * sizeof(MEMORY_WORKING_SET_EX_INFORMATION)
    HANDLE ProcessHandle;
} PF_VIRTUAL_QUERY, *PPF_VIRTUAL_QUERY;

// rev
#define PF_PAGECOMBINE_AGGREGATE_STAT_VERSION 1

// rev
typedef struct _PF_PAGECOMBINE_AGGREGATE_STAT
{
    ULONG Version;
    ULONG CombineScanCount;
    ULONG CombinedBlocksInUse;
    ULONG SumCombinedBlocksReferenceCount;
} PF_PAGECOMBINE_AGGREGATE_STAT, *PPF_PAGECOMBINE_AGGREGATE_STAT;

// rev
#define PF_MIN_WS_AGE_RATE_CONTROL_VERSION 1

// rev
typedef struct _PF_MIN_WS_AGE_RATE_CONTROL
{
    ULONG Version;
    ULONG SecondsToOldestAgeRate;
} PF_MIN_WS_AGE_RATE_CONTROL, *PPF_MIN_WS_AGE_RATE_CONTROL;

// rev
#define PF_DEPRIORITIZE_OLD_PAGES_VERSION 3

// rev
typedef struct _PF_DEPRIORITIZE_OLD_PAGES
{
    ULONG Version;
    HANDLE ProcessHandle;
    union
    {
        ULONG Flags;
        struct
        {
            ULONG TargetPriority : 4;
            ULONG TrimPages : 2;
            ULONG Spare : 26;
        };
    };
} PF_DEPRIORITIZE_OLD_PAGES, *PPF_DEPRIORITIZE_OLD_PAGES;

// rev
#define PF_FILE_EXTENTS_INFO_VERSION 1

// rev
typedef struct _PF_FILE_EXTENTS_INFO
{
    ULONG Version;
    PWSTR FilePath;
    ULONG FilePathSize;
    ULONG VolumePathSize;
    LARGE_INTEGER FileIndexNumber;
    ULONG VolumeSerialNumber;
    RETRIEVAL_POINTERS_BUFFER ExtentsBuffer;
    ULONGLONG ExtentsBufferSize;
} PF_FILE_EXTENTS_INFO, *PPF_FILE_EXTENTS_INFO;

// rev
#define PF_FILE_EXTENTS_INFO_VERSION2 2

typedef struct _PF_FILE_EXTENTS_INFO_V2
{
    ULONG Version;      // must be 2
    ULONG Reserved0;
    PWSTR PathUtf16;

    ULONG PathBytes;    // must be even, within bounds
    ULONG PathMeta;     // used to compute an index (>>1) tested for '\\'
    ULONG ParamA;       // must be nonzero and < PathBytes (per checks)
    ULONG Reserved1;

    PVOID OutMeta0;
    PVOID OutBuffer;
    ULONG OutBufferBytesRequested;
    ULONG Reserved2;
} PF_FILE_EXTENTS_INFO_V2, *PPF_FILE_EXTENTS_INFO_V2;

// rev
#define PF_GPU_UTILIZATION_INFO_VERSION 1

// rev
typedef struct _PF_GPU_UTILIZATION_INFO
{
    ULONG Version;
    ULONG SessionId;
    ULONGLONG GpuTime;
} PF_GPU_UTILIZATION_INFO, *PPF_GPU_UTILIZATION_INFO;

// end_private

#endif // _NTPFAPI_H
