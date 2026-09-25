/*
 * Prefetcher and Superfetch support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTPFAPI_H
#define _NTPFAPI_H

//
// Prefetch
//

/**
 * The PREFETCHER_INFORMATION_CLASS enumeration selects a Prefetcher query or control operation.
 */
typedef enum _PREFETCHER_INFORMATION_CLASS
{
    PrefetcherRetrieveTrace = 1,                // q: PF_RETRIEVE_TRACE
    PrefetcherSystemParameters,                 // q: PF_SYSTEM_PREFETCH_PARAMETERS
    PrefetcherBootPhase,                        // s: ULONG (0..4; user mode accepts only 2 on build 26100)
    PrefetcherSpare1,                           // Formerly PrefetcherRetrieveBootLoaderTrace; not implemented on build 26100
    PrefetcherOperationProcess,                 // s: PF_OPERATION_PROCESS
    PrefetcherCacheEntryUpdate,                 // s: PF_CACHE_ENTRY_UPDATE
    PrefetcherSpare2,                           // Reserved information class; not implemented on the verified build.
    PrefetcherAppLaunchScenarioControl,         // s: PF_APP_LAUNCH_SCENARIO_CONTROL
    PrefetcherInformationMax
} PREFETCHER_INFORMATION_CLASS;

#define PREFETCHER_INFORMATION_VERSION 1 // Current Prefetcher envelope version (1).
#define PREFETCHER_INFORMATION_VERSION_LEGACY 23 // Historical Prefetcher envelope version (23); rejected by the verified build.
#define PREFETCHER_INFORMATION_MAGIC ('kuhC') // Prefetcher envelope signature, 0x6b756843.

/**
 * The PREFETCHER_INFORMATION structure supplies the versioned envelope for Prefetcher system-information requests.
 */
typedef struct _PREFETCHER_INFORMATION
{
    _In_ ULONG Version;                         // Envelope version: PREFETCHER_INFORMATION_VERSION (1).
    _In_ ULONG Magic;                           // Envelope signature: PREFETCHER_INFORMATION_MAGIC.
    _In_ PREFETCHER_INFORMATION_CLASS PrefetcherInformationClass; // Operation and payload selector.
    _Inout_ PVOID PrefetcherInformation;        // Pointer to the class-specific input/output buffer.
    _Inout_ ULONG PrefetcherInformationLength;  // Size of the class-specific buffer, in bytes.
} PREFETCHER_INFORMATION, *PPREFETCHER_INFORMATION;

// rev
#define PF_RETRIEVE_TRACE_VERSION 31
#define PF_RETRIEVE_TRACE_MAGIC ('CCSA') // Trace signature, 0x43435341; stored as the bytes ASCC.

// rev
/**
 * The PF_RETRIEVE_TRACE_PAGE_ENTRY structure contains an opaque page record from a completed Prefetcher trace.
 */
typedef struct DECLSPEC_ALIGN(8) _PF_RETRIEVE_TRACE_PAGE_ENTRY
{
    UCHAR Data[16];                             // Opaque 16-byte page record.
} PF_RETRIEVE_TRACE_PAGE_ENTRY, *PPF_RETRIEVE_TRACE_PAGE_ENTRY;

// rev
/**
 * The PF_RETRIEVE_TRACE_SECTION_ENTRY structure contains an opaque section record from a completed Prefetcher trace.
 */
typedef struct DECLSPEC_ALIGN(8) _PF_RETRIEVE_TRACE_SECTION_ENTRY
{
    UCHAR Data[24];                             // Opaque 24-byte section record.
} PF_RETRIEVE_TRACE_SECTION_ENTRY, *PPF_RETRIEVE_TRACE_SECTION_ENTRY;

// rev
/**
 * The PF_RETRIEVE_TRACE structure describes the fixed header of a completed Prefetcher trace.
 */
typedef struct DECLSPEC_ALIGN(8) _PF_RETRIEVE_TRACE
{
    ULONG Version;                              // PF_RETRIEVE_TRACE_VERSION
    ULONG Magic;                                // PF_RETRIEVE_TRACE_MAGIC
    ULONG Size;                                 // Total buffer size, including the variable payload that follows this header.
    ULONG Spare0;                               // Opaque trace state copied from the completed trace.
    UCHAR OpaqueHeader[0x40];                   // Opaque fixed fields copied from the completed trace.
    ULONG PageEntriesOffset;                    // Offset from the start of this structure to PF_RETRIEVE_TRACE_PAGE_ENTRY[PageEntriesCount].
    ULONG PageEntriesCount;                     // Count of 16-byte PF_RETRIEVE_TRACE_PAGE_ENTRY records.
    ULONG TraceLimitsMaxNumPages;               // Page limit recorded for this trace.
    ULONG TraceLimitsMaxNumSections;            // Section limit recorded for this trace.
    ULONG SectionEntriesOffset;                 // Offset from the start of this structure to PF_RETRIEVE_TRACE_SECTION_ENTRY[SectionEntriesCount].
    ULONG SectionEntriesCount;                  // Count of 24-byte PF_RETRIEVE_TRACE_SECTION_ENTRY records.
    UCHAR OpaqueTail[0x68];                     // Fixed header size is 0xd0; page/section payloads follow at their offsets.
} PF_RETRIEVE_TRACE, *PPF_RETRIEVE_TRACE;

/**
 * The PF_ENABLE_STATUS enumeration specifies whether a Prefetcher scenario is enabled.
 */
typedef enum _PF_ENABLE_STATUS
{
    PfSvNotSpecified,                           // 0: No explicit enable setting.
    PfSvEnabled,                                // 1: Prefetching is enabled.
    PfSvDisabled,                               // 2: Prefetching is disabled.
    PfSvMaxEnableStatus                         // 3: Maximum sentinel; not an enable setting.
} PF_ENABLE_STATUS;

// rev
/**
 * The PF_TRACE_LIMITS structure specifies page, section and timer limits for a Prefetcher trace.
 */
typedef struct _PF_TRACE_LIMITS
{
    ULONG MaxNumPages;                          // Maximum number of pages recorded by the trace.
    ULONG MaxNumSections;                       // Maximum number of sections recorded by the trace.
    LONGLONG TimerPeriod;                       // Signed trace timer interval.
} PF_TRACE_LIMITS, *PPF_TRACE_LIMITS;

// rev
/**
 * The PF_SYSTEM_PREFETCH_PARAMETERS structure contains the parameters returned by PrefetcherSystemParameters.
 */
typedef struct _PF_SYSTEM_PREFETCH_PARAMETERS
{
    PF_ENABLE_STATUS EnableStatus[2];           // Enable settings for application-launch and activity scenarios.
    PF_TRACE_LIMITS TraceLimits[2];             // Trace limits for application-launch and activity scenarios.
    ULONG MaxNumActiveTraces;                   // Maximum number of simultaneously active traces.
    ULONG MaxNumSavedTraces;                    // Maximum number of saved traces.
    WCHAR RootDirPath[48];                      // Prefetch root directory, stored in a 48-WCHAR array.
    WCHAR HostingApplicationList[128];          // Hosting-application list, stored in a 128-WCHAR array.
    ULONG PrefetchFlags;                        // 0..31
    ULONG NumTracePeriods;                      // 1..10
} PF_SYSTEM_PREFETCH_PARAMETERS, *PPF_SYSTEM_PREFETCH_PARAMETERS;

// Legacy boot phase identifiers; not the 0..4 values accepted by the current PrefetcherBootPhase handler.
#define PF_SN_BOOT_PHASE_USER_SHELL_READY 2 // Current user-shell-ready phase (2); the only phase accepted from user mode on the verified build.
#define PF_SN_BOOT_PHASE_MAX 5 // Exclusive upper bound (5) for current Prefetcher boot phases.

/**
 * The PF_BOOT_PHASE_ID enumeration identifies historical Prefetcher boot milestones.
 */
typedef enum _PF_BOOT_PHASE_ID
{
    PfKernelInitPhase = 0,                      // Kernel initialization milestone (0).
    PfBootDriverInitPhase = 90,                 // Boot-driver initialization milestone (90).
    PfSystemDriverInitPhase = 120,              // System-driver initialization milestone (120).
    PfSessionManagerInitPhase = 150,            // Session Manager initialization milestone (150).
    PfSMRegistryInitPhase = 180,                // Session Manager registry milestone (180).
    PfVideoInitPhase = 210,                     // Video initialization milestone (210).
    PfPostVideoInitPhase = 240,                 // Post-video initialization milestone (240).
    PfBootAcceptedRegistryInitPhase = 270,      // Boot-accepted registry milestone (270).
    PfUserShellReadyPhase = 300,                // User-shell-ready milestone (300).
    PfMaxBootPhaseId = 900                      // Maximum legacy milestone identifier (900).
} PF_BOOT_PHASE_ID;

#define PF_SN_OPERATION_PROCESS_VERSION 1

/**
 * The PF_OPERATION_PROCESS_ACTION enumeration selects whether a process operation scenario begins or ends.
 */
typedef enum _PF_OPERATION_PROCESS_ACTION
{
    PfSnOpProcessBegin = 0,                     // 0: Begin an operation scenario.
    PfSnOpProcessEnd = 1,                       // 1: End an operation scenario.
    PfSnOpProcessMax = 2                        // 2: Maximum sentinel; not an operation.
} PF_OPERATION_PROCESS_ACTION;

/**
 * The PF_OPERATION_PROCESS structure starts or ends an operation scenario for the calling process.
 */
typedef struct _PF_OPERATION_PROCESS
{
    UCHAR Version;                              // Request version: PF_SN_OPERATION_PROCESS_VERSION (1).
    UCHAR Action;                               // PF_OPERATION_PROCESS_ACTION
    USHORT Reserved;                            // Must be zero.
    ULONG OpFlags;                              // Begin: 0..4 (bit 2 cannot be combined with bits 0..1); end: 0..1.
    ULONG Value;                                // Operation-specific value incorporated into the scenario identifier.
} PF_OPERATION_PROCESS, *PPF_OPERATION_PROCESS;

#define PF_BOOT_CONTROL_VERSION 1

/**
 * The PF_BOOT_CONTROL structure contains the legacy boot-prefetch control payload.
 */
typedef struct _PF_BOOT_CONTROL
{
    ULONG Version;                              // Legacy request version: PF_BOOT_CONTROL_VERSION (1).
    ULONG DisableBootPrefetching;               // Legacy boot-prefetch disable setting.
} PF_BOOT_CONTROL, *PPF_BOOT_CONTROL;

#define PF_CACHE_ENTRY_UPDATE_VERSION 2

/**
 * The PF_CACHE_ENTRY_UPDATE structure supplies an update to a Prefetcher cache entry.
 */
typedef struct _PF_CACHE_ENTRY_UPDATE
{
    ULONG Version;                              // Request version: PF_CACHE_ENTRY_UPDATE_VERSION (2).
    UCHAR Name[64];                             // Fixed 64-byte cache-entry identifier.
    ULONG NewValue;                             // Replacement value for the selected cache entry.
} PF_CACHE_ENTRY_UPDATE, *PPF_CACHE_ENTRY_UPDATE;

#define PF_APP_LAUNCH_SCENARIO_CONTROL_VERSION 1

/**
 * The PF_APP_LAUNCH_SCENARIO_CONTROL structure starts an application-launch scenario for a process.
 */
typedef struct _PF_APP_LAUNCH_SCENARIO_CONTROL
{
    ULONG Version;                              // Request version: PF_APP_LAUNCH_SCENARIO_CONTROL_VERSION (1).
    ULONG Enable;                               // Must be nonzero; zero does not disable the scenario.
    HANDLE ProcessHandle;                       // Handle to the process whose application-launch scenario is started.
} PF_APP_LAUNCH_SCENARIO_CONTROL, *PPF_APP_LAUNCH_SCENARIO_CONTROL;

//
// Superfetch
//

// rev
/**
 * The SUPERFETCH_INFORMATION_CLASS enumeration selects a Superfetch query or control operation.
 */
typedef enum _SUPERFETCH_INFORMATION_CLASS
{
    SuperfetchRetrieveTrace = 1,                // q: PF_SYSTEM_SUPERFETCH_RETRIEVE_TRACE // PfGetCompletedTrace
    SuperfetchSystemParameters,                 // q: PF_SYSTEM_SUPERFETCH_PARAMETERS
    SuperfetchLogEvent,                         // s: PF_LOG_EVENT_DATA
    SuperfetchGenerateTrace,                    // s: PF_GENERATE_TRACE_CONTROL
    SuperfetchPrefetch,                         // s: PF_PREFETCH_REQUEST
    SuperfetchPfnQuery,                         // q: PF_PFN_PRIO_REQUEST
    SuperfetchPfnSetPriority,                   // s: PF_PFN_PRIO_REQUEST // MmSetPfnListInfo
    SuperfetchPrivSourceQuery,                  // q: PF_PRIVSOURCE_QUERY_REQUEST
    SuperfetchSequenceNumberQuery,              // q: PF_SEQUENCENUMBER_QUERY_REQUEST
    SuperfetchScenarioPhase,                    // s: PF_SCENARIO_PHASE_INFO // 10
    SuperfetchWorkerPriority,                   // s: PF_WORKER_PRIORITY_CONTROL
    SuperfetchScenarioQuery,                    // q: PF_SCENARIO_QUERY_INFO
    SuperfetchScenarioPrefetch,                 // s: PF_SCENARIO_PREFETCH_INFO
    SuperfetchRobustnessControl,                // s: PF_ROBUSTNESS_CONTROL
    SuperfetchTimeControl,                      // s: PF_TIME_CONTROL
    SuperfetchMemoryListQuery,                  // q: PF_MEMORY_LIST_INFO
    SuperfetchMemoryRangesQuery,                // q: PF_PHYSICAL_MEMORY_RANGE_INFO_V2 (V1 on older systems)
    SuperfetchTracingControl,                   // s: PF_ACCESS_TRACING_CONTROL
    SuperfetchTrimWhileAgingControl,            // s: PF_TRIM_WHILE_AGING_CONTROL_3
    SuperfetchRepurposedByPrefetch,             // q: PF_REPURPOSED_BY_PREFETCH_INFO // 20
    SuperfetchChannelPowerRequest,              // q: not implemented
    SuperfetchMovePages,                        // s: PF_PFN_PRIO_REQUEST // MmRelocatePfnList
    SuperfetchVirtualQuery,                     // q: PF_VIRTUAL_QUERY
    SuperfetchCombineStatsQuery,                // q: PF_PAGECOMBINE_AGGREGATE_STAT
    SuperfetchSetMinWsAgeRate,                  // s: PF_MIN_WS_AGE_RATE_CONTROL
    SuperfetchDeprioritizeOldPagesInWs,         // s: PF_DEPRIORITIZE_OLD_PAGES
    SuperfetchFileExtentsQuery,                 // q: PF_FILE_EXTENTS_INFO_V2
    SuperfetchGpuUtilizationQuery,              // q: PF_GPU_UTILIZATION_INFO
    SuperfetchPfnSet,                           // s: PF_PFN_PRIO_REQUEST // since WIN11
    SuperfetchInformationMax
} SUPERFETCH_INFORMATION_CLASS;

#define SUPERFETCH_INFORMATION_VERSION 45
#define SUPERFETCH_INFORMATION_MAGIC ('kuhC')

/**
 * The SUPERFETCH_INFORMATION structure supplies the versioned envelope for Superfetch system-information requests.
 */
typedef struct _SUPERFETCH_INFORMATION
{
    _In_ ULONG Version;                         // Envelope version: SUPERFETCH_INFORMATION_VERSION (45).
    _In_ ULONG Magic;                           // Envelope signature: SUPERFETCH_INFORMATION_MAGIC.
    _In_ SUPERFETCH_INFORMATION_CLASS SuperfetchInformationClass; // Operation and payload selector.
    _Inout_ PVOID SuperfetchInformation;        // Pointer to the class-specific input/output buffer.
    _Inout_ ULONG SuperfetchInformationLength;  // Size of the class-specific buffer, in bytes.
} SUPERFETCH_INFORMATION, *PSUPERFETCH_INFORMATION;

#define PF_SYSTEM_SUPERFETCH_RETRIEVE_TRACE_VERSION 2

/**
 * The PF_SYSTEM_SUPERFETCH_RETRIEVE_TRACE structure provides the input and output envelope for a completed Superfetch trace.
 */
typedef struct _PF_SYSTEM_SUPERFETCH_RETRIEVE_TRACE
{
    union
    {
        struct
        {
            ULONGLONG RequestType;              // Low USHORT is PF_SYSTEM_SUPERFETCH_RETRIEVE_TRACE_VERSION.
            ULONGLONG Reserved;                 // Ignored on input
            HANDLE PartitionHandle;             // Partition handle supplied on input and preserved on output.
        } Input;
        struct
        {
            ULONGLONG TypeFlags;                // 0x0000000000180002 on success
            ULONGLONG Timestamp;                // Scaled shared-user-data tick count.
            HANDLE PartitionHandle;             // Partition handle supplied on input and preserved on output.
        } Output;

        //
        // Raw view of the buffer for opaque access
        //
        UCHAR Buffer[ANYSIZE_ARRAY];            // Raw view of the envelope; additional storage is required for the trailing trace dump.
    };
} PF_SYSTEM_SUPERFETCH_RETRIEVE_TRACE, *PPF_SYSTEM_SUPERFETCH_RETRIEVE_TRACE;

// rev
/**
 * The PF_SYSTEM_SUPERFETCH_PARAMETERS structure contains the parameters returned by SuperfetchSystemParameters.
 */
typedef struct _PF_SYSTEM_SUPERFETCH_PARAMETERS
{
    ULONG EnabledComponents;                    // Bitmask of enabled Superfetch components.
    ULONG BootID;                               // Boot identifier associated with the Superfetch state.
    ULONG SavedSectInfoTracesMax;               // Maximum number of saved section-information traces.
    ULONG SavedPageAccessTracesMax;             // Maximum number of saved page-access traces.
    ULONG ScenarioPrefetchTimeoutStandby;       // Configured standby-scenario prefetch timeout.
    ULONG ScenarioPrefetchTimeoutHibernate;     // Configured hibernation-scenario prefetch timeout.
    ULONG ScenarioPrefetchTimeoutHiberBoot;     // Configured hybrid-boot-scenario prefetch timeout.
} PF_SYSTEM_SUPERFETCH_PARAMETERS, *PPF_SYSTEM_SUPERFETCH_PARAMETERS;

#define PF_GENERATE_TRACE_CONTROL_VERSION 1

/**
 * The PF_GENERATE_TRACE_CONTROL structure requests trace generation for a Superfetch partition.
 */
typedef struct _PF_GENERATE_TRACE_CONTROL
{
    ULONG Version;                              // PF_GENERATE_TRACE_CONTROL_VERSION
    ULONG Reserved;                             // Reserved field; initialize to zero and do not interpret on output.
    HANDLE PartitionHandle;                     // Handle identifying the partition whose trace is generated.
} PF_GENERATE_TRACE_CONTROL, *PPF_GENERATE_TRACE_CONTROL;

// rev
/**
 * The PF_PREFETCH_RANGE structure describes a byte range to prefetch from a file or private-page source.
 */
typedef struct _PF_PREFETCH_RANGE
{
    ULONGLONG Offset;                           // File byte offset or virtual address, not a PFN.
    ULONG Length;                               // Nonzero byte count.
    ULONG Reserved;                             // Reserved field; initialize to zero and do not interpret on output.
} PF_PREFETCH_RANGE, *PPF_PREFETCH_RANGE;

// rev
/**
 * The PF_PREFETCH_RANGE_INFO structure locates a variable array of prefetch byte ranges.
 */
typedef struct _PF_PREFETCH_RANGE_INFO
{
    ULONG RangeCount;                           // Number of PF_PREFETCH_RANGE records.
    ULONG Reserved;                             // Reserved field; initialize to zero and do not interpret on output.
    union
    {
        ULONGLONG RangesOffset;                 // Byte offset from PF_PREFETCH_REQUEST to the range array.
        PPF_PREFETCH_RANGE Ranges;              // Kernel-private patched view.
    };
} PF_PREFETCH_RANGE_INFO, *PPF_PREFETCH_RANGE_INFO;

// rev
/**
 * The PF_PREFETCH_PATH_INFO structure locates a NUL-terminated path within a prefetch request.
 */
typedef struct _PF_PREFETCH_PATH_INFO
{
    union
    {
        ULONGLONG PathOffset;                   // Byte offset from PF_PREFETCH_REQUEST to the NUL-terminated UTF-16 path.
        PWSTR Path;                             // Kernel-private patched view.
    };
    ULONG PathLength;                           // WCHAR count, excluding terminator; 1..0x7ffe when present.
    ULONG Reserved;                             // Reserved field; initialize to zero and do not interpret on output.
} PF_PREFETCH_PATH_INFO, *PPF_PREFETCH_PATH_INFO;

// rev
/**
 * The PF_PREFETCH_FILE_INFO structure describes a file and its ranges in a prefetch request.
 */
typedef struct _PF_PREFETCH_FILE_INFO
{
    ULONG Flags;                                // File-prefetch request/result flags.
    ULONG Reserved;                             // Reserved field; initialize to zero and do not interpret on output.
    ULONGLONG FileIndexNumber;                  // File identifier used for metadata prefetch and file-ID opens.
    PF_PREFETCH_RANGE_INFO Ranges;              // Descriptor of the byte ranges to prefetch.
    union
    {
        ULONGLONG PathOffset;                   // Byte offset from PF_PREFETCH_REQUEST to the optional UTF-16 path.
        PWSTR Path;                             // Kernel-private patched view.
    };
    ULONG PathLength;                           // WCHAR count, excluding terminator; 1..0x7ffe when present.
    ULONG Reserved1;                            // Reserved field; initialize to zero and do not interpret on output.
} PF_PREFETCH_FILE_INFO, *PPF_PREFETCH_FILE_INFO;

// rev
/**
 * The PF_PREFETCH_VOLUME_INFO structure describes a volume and its file records in a prefetch request.
 */
typedef struct _PF_PREFETCH_VOLUME_INFO
{
    LARGE_INTEGER VolumeCreationTime;           // Volume creation time used to verify volume identity.
    ULONG VolumeSerialNumber;                   // Volume serial number used to verify volume identity.
    union
    {
        ULONG Flags;                            // Packed power-state flag (bit 0) and file count (bits 1..31).
        struct
        {
            ULONG CheckPowerState : 1;          // Requests a volume power-state check before prefetching.
            ULONG PrefetchFileCount : 31;       // Number of PF_PREFETCH_FILE_INFO records for the volume.
        };
    };
    union
    {
        ULONGLONG PrefetchFileInfoOffset;       // Byte offset from PF_PREFETCH_REQUEST to the file-record array.
        PPF_PREFETCH_FILE_INFO PrefetchFileInfo; // Kernel-private patched view.
    };
    ULONG VolumePathLength;                     // WCHAR count, excluding terminator; 1..0x7ffe.
    ULONG Reserved;                             // Reserved field; initialize to zero and do not interpret on output.
    union
    {
        ULONGLONG VolumePathOffset;             // Byte offset from PF_PREFETCH_REQUEST to the UTF-16 volume path.
        PWSTR VolumePath;                       // Kernel-private patched view.
    };
} PF_PREFETCH_VOLUME_INFO, *PPF_PREFETCH_VOLUME_INFO;

// rev
/**
 * The PF_PREFETCH_SOURCE_INFO structure describes a kernel or process private-page prefetch source.
 */
typedef struct _PF_PREFETCH_SOURCE_INFO
{
    ULONG Type;                                 // PFS_PRIVATE_PAGE_SOURCE_TYPE: kernel (0) or process (2).
    ULONG ProcessId;                            // Process identifier when Type is PfsPrivateSourceProcess.
    ULONG ImagePathHash;                        // Image-path hash associated with the private-page source.
    ULONG Reserved;                             // Reserved field; initialize to zero and do not interpret on output.
    ULONGLONG UniqueProcessHash;                // Zero or the process ID/create-time hash.
    PF_PREFETCH_RANGE_INFO RangeInfo;           // Descriptor of the source virtual-address byte ranges.
} PF_PREFETCH_SOURCE_INFO, *PPF_PREFETCH_SOURCE_INFO;

#define PF_PREFETCH_REQUEST_VERSION 13

/**
 * The PF_PREFETCH_REQUEST structure supplies a variable-length file and private-page prefetch request.
 */
typedef struct _PF_PREFETCH_REQUEST
{
    ULONG Version;                              // PF_PREFETCH_REQUEST_VERSION
    ULONG Size;                                 // Entire buffer size, including the 0x80-byte header (x64).
    ULONG VolumeCount;                          // Number of PF_PREFETCH_VOLUME_INFO records.
    ULONG PrefetchFileCount;                    // Total number of PF_PREFETCH_FILE_INFO records.
    ULONG RangeCount;                           // Number of PF_PREFETCH_RANGE records, not paths.
    ULONG SourceCount;                          // Number of PF_PREFETCH_SOURCE_INFO records.
    ULONG UnicodeTextSize;                      // Bytes.
    USHORT PrefetchPriority;                    // 0..7.
    USHORT VolumePrefetchPriority;              // 0..7.
    ULONG VolumeInfoOffset;                     // Byte offset to the volume records; zero when VolumeCount is zero.
    ULONG Reserved0;                            // Reserved field; initialize to zero and do not interpret on output.
    ULONG PrefetchFileInfoOffset;               // Byte offset to the file records; zero when PrefetchFileCount is zero.
    ULONG Reserved1;                            // Reserved field; initialize to zero and do not interpret on output.
    ULONG RangeInfoOffset;                      // Byte offset to PF_PREFETCH_RANGE records; zero when RangeCount is zero.
    ULONG Reserved2;                            // Reserved field; initialize to zero and do not interpret on output.
    ULONG SourceInfoOffset;                     // Byte offset to private-source records; zero when SourceCount is zero.
    ULONG Reserved3;                            // Reserved field; initialize to zero and do not interpret on output.
    ULONG UnicodeTextOffset;                    // Byte offset to the UTF-16 text area; zero when UnicodeTextSize is zero.
    ULONG Reserved4;                            // Reserved field; initialize to zero and do not interpret on output.
    HANDLE CompletionEvent;                     // Optional event handle; captured as an object in the kernel's private copy.
    union
    {
        ULONG Flags;                            // Packed request flags and scenario selector.
        struct
        {
            ULONG ContinueIO : 1;               // Bit 0: Continue-I/O flag retained from the reconstructed layout.
            ULONG LowPowerMode : 1;             // Bit 1: Low-power flag retained from the reconstructed layout.
            ULONG UseScenarioTimeouts : 1;      // Bit 2: Selects scenario-timeout and trickled-prefetch processing.
            ULONG OtherFlags : 3;               // Accepted bits whose semantics are not yet identified.
            ULONG ReservedFlags : 2;            // Must be zero.
            ULONG ScenarioType : 8;             // PF_PHASED_SCENARIO_TYPE, 0..5.
            ULONG Spare : 16;                   // Reserved field; initialize to zero and do not interpret on output.
        };
    };
    ULONG FilePrefetchCount;                    // Output count of files considered for prefetching.
    ULONG MetadataPrefetchCount;                // Output count of metadata prefetch operations.
    ULONG PrivatePrefetchCount;                 // Output count of private-source prefetch operations.
    ULONG FilePageCount;                        // Output page count for the first file-prefetch pass.
    ULONG FilePageCountSecondPass;              // Output page count for the second file-prefetch pass.
    ULONG PrivatePageCount;                     // Output count of private pages submitted for prefetching.
    ULONG MetadataPrefetchTime;                 // Milliseconds.
    ULONG FilePrefetchTime;                     // Milliseconds.
    ULONG FilePrefetchTimeSecondPass;           // Milliseconds.
    ULONG PrivatePrefetchTime;                  // Milliseconds.
    ULONG Reserved5;                            // Reserved field; initialize to zero and do not interpret on output.
} PF_PREFETCH_REQUEST, *PPF_PREFETCH_REQUEST;

// rev
/**
 * The PF_EVENT_TYPE enumeration identifies a Prefetcher or Superfetch event category.
 */
typedef enum _PF_EVENT_TYPE
{
    PfEventTypeImageLoad = 0,                   // 0: Image load.
    PfEventTypeAppLaunch = 1,                   // 1: Application launch.
    PfEventTypeStartTrace = 2,                  // 2: Trace start.
    PfEventTypeEndTrace = 3,                    // 3: Trace end.
    PfEventTypeTimestamp = 4,                   // 4: Timestamp.
    PfEventTypeOperation = 5,                   // 5: Operation.
    PfEventTypeRepurpose = 6,                   // 6: Page repurposing.
    PfEventTypeForegroundProcess = 7,           // 7: Foreground process.
    PfEventTypeTimeRange = 8,                   // 8: Time range.
    PfEventTypeUserInput = 9,                   // 9: User input.
    PfEventTypeFileAccess = 10,                 // 10: File access.
    PfEventTypeUnmap = 11,                      // 11: Unmap (legacy value).
    PfEventTypeUtilization = 11,                // 11: Utilization (shares the declared value 11).
    PfEventTypeMemInfo = 12,                    // 12: Memory information.
    PfEventTypeFileDelete = 13,                 // 13: File deletion.
    PfEventTypeAppExit = 14,                    // 14: Application exit.
    PfEventTypeSystemTime = 15,                 // 15: System time.
    PfEventTypePower = 16,                      // 16: Power.
    PfEventTypeSessionChange = 17,              // 17: Session change.
    PfEventTypeHardFaultTimeStamp = 18,         // 18: Hard-fault timestamp.
    PfEventTypeVirtualFree = 19,                // 19: Virtual memory free.
    PfEventTypePerfInfo = 20,                   // 20: Performance information.
    PfEventTypeProcessSnapshot = 21,            // 21: Process snapshot.
    PfEventTypeUserSnapshot = 22,               // 22: User snapshot.
    PfEventTypeStreamSequenceNumber = 23,       // 23: Stream sequence number.
    PfEventTypeFileTruncate = 24,               // 24: File truncation.
    PfEventTypeFileRename = 25,                 // 25: File rename.
    PfEventTypeFileCreate = 26,                 // 26: File creation.
    PfEventTypeAgCxContext = 27,                // 27: AgCx context.
    PfEventTypePowerAction = 28,                // 28: Power action.
    PfEventTypeHardFaultTS = 29,                // 29: Hard-fault TS.
    PfEventTypeRobustInfo = 30,                 // 30: Robustness information.
    PfEventTypeFileDefrag = 31,                 // 31: File defragmentation.
    PfEventTypeMax = 32                         // 32: Maximum sentinel; not an event.
} PF_EVENT_TYPE;

#define PF_LOG_EVENT_DATA_VERSION 1

/**
 * The PF_LOG_EVENT_DATA structure supplies an event payload for SuperfetchLogEvent.
 */
typedef struct _PF_LOG_EVENT_DATA
{
    ULONG Version;                              // PF_LOG_EVENT_DATA_VERSION
    union
    {
        ULONG Packed;                           // [31:7]=DataSize, [6:5]=Flags, [4:0]=EventType (PF_EVENT_TYPE)
        struct
        {
            ULONG EventType : 5;                // PF_EVENT_TYPE: 2, 3, 5 or 27.
            ULONG Flags : 2;                    // Two event-logging flag bits, packed at bits 5..6.
            ULONG DataSize : 25;                // Bytes; < 0xff0. Minimum: 36 for 2/3, 12 for 5, 16 for 27.
        };
    };
    HANDLE PartitionHandle;                     // Input partition handle at offset 8 on x64.
    PVOID EventData;                            // Pointer to the input event payload at offset 16 on x64.
} PF_LOG_EVENT_DATA, *PPF_LOG_EVENT_DATA;

/**
 * The PFN_TRIPLET structure contains a page-frame entry for a Superfetch PFN update or relocation.
 */
typedef struct _PFN_TRIPLET
{
    ULONGLONG MaskOrKey;                        // Compared against identity with 0x1FFFFFFFFFFFE00 mask
    ULONGLONG Pfn;                              // Page frame number
    ULONGLONG Flags;                            // Request/result flags
} PFN_TRIPLET, *PPFN_TRIPLET;

#define PF_PFN_PRIO_REQUEST_VERSION 1
#define PF_PFN_PRIO_REQUEST_QUERY_MEMORY_LIST 0x1 // Bit 0: Also request memory-list information with a PFN query.
#define PF_PFN_PRIO_REQUEST_VALID_FLAGS 0x1 // Valid request flag mask (0x1) for PFN query and priority classes 6 and 7.

/**
 * The PF_PFN_PRIO_REQUEST structure queries or updates a variable array of page-frame entries.
 */
typedef struct _PF_PFN_PRIO_REQUEST
{
    ULONG Version;                              // Request version: PF_PFN_PRIO_REQUEST_VERSION (1).
    ULONG RequestFlags;                         // Classes 6/7: bit 0 only; class 22 passes the low/high USHORTs to MmRelocatePfnList.
    SIZE_T PfnCount;                            // Nonzero; buffer size is FIELD_OFFSET(..., PageData) + PfnCount * sizeof(MMPFN_IDENTITY).
    SYSTEM_MEMORY_LIST_INFORMATION MemInfo;     // Memory-list statistics associated with the request.
    union
    {
        // Class 6: input PFNs, output identities. Class 16 uses PF_MEMORY_LIST_INFO instead.
        MMPFN_IDENTITY PageIdentities[256];     // Variable-length array with PfnCount entries.
        MMPFN_IDENTITY PageData[256];           // Compatibility name.
        // Classes 7, 22 and 29: caller-supplied entries; class 22 also copies results back.
        PFN_TRIPLET Entries[256];               // Variable array of PfnCount update or relocation entries; 256 is the declared capacity, not a limit.
    };
} PF_PFN_PRIO_REQUEST, *PPF_PFN_PRIO_REQUEST;

/**
 * The PFS_PRIVATE_PAGE_SOURCE_TYPE enumeration identifies the kind of private-page source.
 */
typedef enum _PFS_PRIVATE_PAGE_SOURCE_TYPE
{
    PfsPrivateSourceKernel,                     // 0: Kernel private-page source.
    PfsPrivateSourceSession,                    // 1: Legacy session private-page source.
    PfsPrivateSourceProcess,                    // 2: Process private-page source.
    PfsPrivateSourceMax                         // 3: Maximum sentinel; not a source type.
} PFS_PRIVATE_PAGE_SOURCE_TYPE;

/**
 * The PFS_PRIVATE_PAGE_SOURCE structure identifies a private-page source and its process or session metadata.
 */
typedef struct _PFS_PRIVATE_PAGE_SOURCE
{
    PFS_PRIVATE_PAGE_SOURCE_TYPE Type;          // Source category selecting kernel, session or process interpretation.
    union
    {
        ULONG SessionId;                        // Session identifier for a session source.
        ULONG ProcessId;                        // Process identifier for a process source.
    };
    ULONG ImagePathHash;                        // Hash identifying the source image path.
    ULONG_PTR UniqueProcessHash;                // Hash derived from the process identifier and creation time.
} PFS_PRIVATE_PAGE_SOURCE, *PPFS_PRIVATE_PAGE_SOURCE;

/**
 * The PF_PRIVSOURCE_INFO structure contains identity and memory-use information for a private-page source.
 */
typedef struct _PF_PRIVSOURCE_INFO
{
    PFS_PRIVATE_PAGE_SOURCE DbInfo;             // Private-page source identity.
    union
    {
        PVOID EProcess;                         // Legacy name; NOT an EPROCESS address on build 26100.
        ULONG_PTR ProcessId;                    // Process ID, or 0xffffffff for the kernel entry.
    };
    SIZE_T WsPrivatePages;                      // Private pages resident in the source working set.
    SIZE_T TotalPrivatePages;                   // Total private-page count reported for the source.
    ULONG SessionID;                            // Session identifier associated with the source.
    UCHAR ImageName[16];                        // Short source image name in a 16-byte array.
    union
    {
        SIZE_T WsSwapPages;                     // Process working-set swap pages when PF_PRIVSOURCE_QUERY_REQUEST_FLAGS_QUERYWSPAGES is selected.
        SIZE_T SessionPagedPoolPages;           // Legacy session paged-pool page count.
        SIZE_T StoreSizePages;                  // Process store size, in pages, when PF_PRIVSOURCE_QUERY_REQUEST_FLAGS_QUERYCOMPRESSEDPAGES is selected.
    };
    SIZE_T WsTotalPages;                        // Total working-set page count for a process or legacy session source.
    ULONG DeepFreezeTimeMs;                     // Process freeze timestamp, in milliseconds, when DeepFrozen is set.
    ULONG ModernApp : 1;                        // Process has the application identity attribute checked by the kernel.
    ULONG DeepFrozen : 1;                       // Process only. If set, DeepFreezeTimeMs contains the time at which the freeze occurred.
    ULONG Foreground : 1;                       // Process is classified as foreground.
    ULONG PerProcessStore : 1;                  // Process has an associated memory store.
    ULONG HasVmContext : 1;                     // Process has a non-NULL VM context.
    ULONG Spare : 27;                           // Reserved field; initialize to zero and do not interpret on output.
} PF_PRIVSOURCE_INFO, *PPF_PRIVSOURCE_INFO;

// rev
#define PF_PRIVSOURCE_QUERY_REQUEST_VERSION 8
#define PF_PRIVSOURCE_QUERY_REQUEST_FLAGS_QUERYWSPAGES 0x1 // Bit 0: Query process working-set swap-page counts.
#define PF_PRIVSOURCE_QUERY_REQUEST_FLAGS_QUERYCOMPRESSEDPAGES 0x2 // Bit 1: Query process memory-store page counts.
#define PF_PRIVSOURCE_QUERY_REQUEST_FLAGS_SKIPMINIMALPROCESSES 0x4 // Bit 2: Omit minimal processes from the enumeration.
#define PF_PRIVSOURCE_QUERY_REQUEST_FLAGS_QUERYSKIPPAGES PF_PRIVSOURCE_QUERY_REQUEST_FLAGS_SKIPMINIMALPROCESSES // Compatibility name for PF_PRIVSOURCE_QUERY_REQUEST_FLAGS_SKIPMINIMALPROCESSES.
#define PF_PRIVSOURCE_QUERY_REQUEST_VALID_FLAGS 0x7 // Valid mask 0x7; QUERYWSPAGES and QUERYCOMPRESSEDPAGES are mutually exclusive.

// rev
/**
 * The PF_PRIVSOURCE_QUERY_REQUEST structure enumerates private-page sources and their memory-use information.
 */
typedef struct _PF_PRIVSOURCE_QUERY_REQUEST
{
    ULONG Version;                              // Request version: PF_PRIVSOURCE_QUERY_REQUEST_VERSION (8).
    ULONG Flags;                                // PF_PRIVSOURCE_QUERY_REQUEST_FLAGS_* options; valid mask 0x7.
    ULONG InfoCount;                            // Number of source records written to InfoArray.
    PF_PRIVSOURCE_INFO InfoArray[1];            // Variable array of returned source records; allocate using the required buffer length.
} PF_PRIVSOURCE_QUERY_REQUEST, *PPF_PRIVSOURCE_QUERY_REQUEST;

// rev
/**
 * The PF_SEQUENCENUMBER_QUERY_REQUEST structure receives the Superfetch sequence number.
 */
typedef struct _PF_SEQUENCENUMBER_QUERY_REQUEST
{
    ULONG SequenceNumber;                       // Output sequence number.
} PF_SEQUENCENUMBER_QUERY_REQUEST, *PPF_SEQUENCENUMBER_QUERY_REQUEST;

// rev
/**
 * The PF_PHASED_SCENARIO_TYPE enumeration identifies a phased Superfetch scenario.
 */
typedef enum _PF_PHASED_SCENARIO_TYPE
{
    PfScenarioTypeNone,                         // 0: No active phased scenario.
    PfScenarioTypeStandby,                      // 1: Standby scenario.
    PfScenarioTypeHibernate,                    // 2: Hibernation scenario.
    PfScenarioTypeFUS,                          // 3: Fast user switching scenario.
    PfScenarioTypeBoot,                         // 4: Boot scenario.
    PfScenarioTypeHiberBoot,                    // 5: Hybrid boot scenario.
    PfScenarioTypeMax                           // 6: Maximum sentinel; not a scenario.
} PF_PHASED_SCENARIO_TYPE;

// rev
#define PF_SCENARIO_PHASE_INFO_VERSION 4

/**
 * The PF_SCENARIO_PHASE_INFO structure notifies Superfetch of a scenario phase transition.
 */
typedef struct _PF_SCENARIO_PHASE_INFO
{
    ULONG Version;                              // Request version: PF_SCENARIO_PHASE_INFO_VERSION (4).
    PF_PHASED_SCENARIO_TYPE ScenType;           // Scenario category, 1..5.
    ULONG PhaseId;                              // Scenario-specific phase identifier.
    ULONG SequenceNumber;                       // Scenario sequence-number field.
    ULONG Flags;                                // Scenario-specific phase flags.
    ULONG Reserved;                             // Reserved field; initialize to zero and do not interpret on output.
    ULONGLONG FUSUserId;                        // Copied to/from the FUS scenario context at offset 0x18.
} PF_SCENARIO_PHASE_INFO, *PPF_SCENARIO_PHASE_INFO;

#define PF_WORKER_PRIORITY_CONTROL_VERSION 1

/**
 * The PF_WORKER_PRIORITY_CONTROL structure changes the trace worker priority for a Superfetch partition.
 */
typedef struct _PF_WORKER_PRIORITY_CONTROL
{
    ULONG Version;                              // Request version: PF_WORKER_PRIORITY_CONTROL_VERSION (1).
    KPRIORITY Priority;                         // 0..31; invalid priority or unavailable worker returns STATUS_TOO_LATE.
    HANDLE PartitionHandle;                     // Handle identifying the partition containing the trace worker.
} PF_WORKER_PRIORITY_CONTROL, *PPF_WORKER_PRIORITY_CONTROL;

// rev
#define PF_SCENARIO_QUERY_INFO_VERSION 4

/**
 * The PF_SCENARIO_QUERY_INFO structure receives the active Superfetch scenario and its sequence number.
 */
typedef struct _PF_SCENARIO_QUERY_INFO
{
    ULONG Version;                              // PF_SCENARIO_QUERY_INFO_VERSION
    PF_PHASED_SCENARIO_TYPE ScenType;           // Output active scenario category.
    ULONG Reserved0;                            // Reserved field; initialize to zero and do not interpret on output.
    ULONG SequenceNumber;                       // Output scenario sequence number.
    ULONG Reserved1;                            // Reserved field; initialize to zero and do not interpret on output.
    ULONG Reserved2;                            // Reserved field; initialize to zero and do not interpret on output.
    ULONGLONG FUSUserId;                        // Output only when ScenType == PfScenarioTypeFUS.
} PF_SCENARIO_QUERY_INFO, *PPF_SCENARIO_QUERY_INFO;

// rev
/**
 * The PF_MEMORY_LIST_NODE structure contains page counts for a memory-list node.
 */
typedef struct _PF_MEMORY_LIST_NODE
{
    ULONGLONG Node : 8;                         // Node identifier; the verified aggregate result uses 0.
    ULONGLONG Spare : 56;                       // Reserved field; initialize to zero and do not interpret on output.
    ULONGLONG StandbyLowPageCount;              // Pages on low-priority standby lists.
    ULONGLONG StandbyMediumPageCount;           // Pages on the medium-priority standby list.
    ULONGLONG StandbyHighPageCount;             // Pages on high-priority standby lists.
    ULONGLONG FreePageCount;                    // Combined zeroed and free-page count.
    ULONGLONG ModifiedPageCount;                // Combined modified-page count reported by the query.
} PF_MEMORY_LIST_NODE, *PPF_MEMORY_LIST_NODE;

// rev
/**
 * The PF_ROBUST_PROCESS_ENTRY structure identifies a process in a Superfetch robustness-control list.
 */
typedef struct _PF_ROBUST_PROCESS_ENTRY
{
    ULONG ImagePathHash;                        // Nonzero image-path hash.
    ULONG Pid;                                  // Process identifier associated with the hash.
} PF_ROBUST_PROCESS_ENTRY, *PPF_ROBUST_PROCESS_ENTRY;

// rev
/**
 * The PF_ROBUST_FILE_ENTRY structure identifies a file in a Superfetch robustness-control list.
 */
typedef struct _PF_ROBUST_FILE_ENTRY
{
    ULONGLONG FilePathHash;                     // Nonzero file-path hash.
} PF_ROBUST_FILE_ENTRY, *PPF_ROBUST_FILE_ENTRY;

// rev
/**
 * The PF_ROBUSTNESS_CONTROL_COMMAND enumeration selects a Superfetch robustness-control operation.
 */
typedef enum _PF_ROBUSTNESS_CONTROL_COMMAND
{
    PfRpControlUpdate = 0,                      // 0: Update the process and file lists.
    PfRpControlReset = 1,                       // 1: Reset the lists; all four entry counts must be zero.
    PfRpControlRobustAllStart = 2,              // 2: Start the robust-all mode.
    PfRpControlRobustAllStop = 3,               // 3: Stop the robust-all mode.
    PfRpControlCommandMax = 4                   // 4: Maximum sentinel; not a command.
} PF_ROBUSTNESS_CONTROL_COMMAND;

// rev
#define PF_ROBUSTNESS_CONTROL_VERSION 3

/**
 * The PF_ROBUSTNESS_CONTROL structure supplies process and file lists for Superfetch robustness control.
 */
typedef struct _PF_ROBUSTNESS_CONTROL
{
    union
    {
        ULONG VersionAndCommand;                // Packed version in bits 0..15 and command in bits 16..31.
        struct
        {
            USHORT Version;                     // PF_ROBUSTNESS_CONTROL_VERSION
            USHORT Command;                     // PF_ROBUSTNESS_CONTROL_COMMAND
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    ULONG DeprioProcessCount;                   // Number of deprioritized process entries.
    ULONG ExemptProcessCount;                   // Number of exempt process entries.
    ULONG DeprioFileCount;                      // Number of deprioritized file entries.
    ULONG ExemptFileCount;                      // Number of exempt file entries.
    ULONG Reserved;                             // Reserved field; initialize to zero and do not interpret on output.
    //
    // Pass FIELD_OFFSET(PF_ROBUSTNESS_CONTROL, Buffer) + 8 * the sum of all four counts.
    // Variable-size payload:
    //   PF_ROBUST_PROCESS_ENTRY ProcessEntries[DeprioProcessCount + ExemptProcessCount];
    //   PF_ROBUST_FILE_ENTRY FileEntries[DeprioFileCount + ExemptFileCount];
    //
    UCHAR Buffer[ANYSIZE_ARRAY];                // Variable payload of process entries followed by file entries.
} PF_ROBUSTNESS_CONTROL, *PPF_ROBUSTNESS_CONTROL;

#define PF_SCENARIO_PREFETCH_INFO_VERSION 1

/**
 * The PF_SCENARIO_PREFETCH_INFO structure sets the prefetch state of the active Superfetch scenario.
 */
typedef struct _PF_SCENARIO_PREFETCH_INFO
{
    USHORT Version;                             // PF_SCENARIO_PREFETCH_INFO_VERSION
    USHORT Reserved;                            // Reserved field; initialize to zero and do not interpret on output.
    ULONG State;                                // 1..5.
} PF_SCENARIO_PREFETCH_INFO, *PPF_SCENARIO_PREFETCH_INFO;

// rev
#define PF_TRIM_WHILE_AGING_CONTROL_VERSION_1 1

/**
 * The PF_TRIM_WHILE_AGING_STATE enumeration selects a trim-while-aging policy.
 */
typedef enum _PF_TRIM_WHILE_AGING_STATE
{
    PfTrimWhileAgingOff = 0,                    // 0: Disable trim-while-aging.
    PfTrimWhileAgingLowPriority = 1,            // 1: Select the low-priority policy.
    PfTrimWhileAgingPassive = 2,                // 2: Select the passive policy.
    PfTrimWhileAgingNormal = 3,                 // 3: Select the normal policy.
    PfTrimWhileAgingAggressive = 4,             // 4: Select the aggressive policy.
    PfTrimWhileAgingCustom = 5,                 // 5: Use explicit high and low thresholds in the version-3 request.
    PfTrimWhileAgingMax = 6                     // 6: Maximum sentinel; not a policy.
} PF_TRIM_WHILE_AGING_STATE, *PPF_TRIM_WHILE_AGING_STATE;

// rev
/**
 * The PF_TRIM_WHILE_AGING_CONTROL_1 structure contains the legacy version-1 trim-while-aging request.
 */
typedef struct _PF_TRIM_WHILE_AGING_CONTROL_1
{
    ULONG Version;                              // Legacy version: PF_TRIM_WHILE_AGING_CONTROL_VERSION_1 (1).
    PF_TRIM_WHILE_AGING_STATE TrimWhileAgingState; // Legacy trim-while-aging policy selector.
    BOOLEAN PrivatePageTrimAge;                 // Private-page age field in the legacy layout.
    BOOLEAN SharedPageTrimAge;                  // Shared-page age field in the legacy layout.
    USHORT Spare;                               // Reserved field; initialize to zero and do not interpret on output.
} PF_TRIM_WHILE_AGING_CONTROL_1, *PPF_TRIM_WHILE_AGING_CONTROL_1;

#define PF_TRIM_WHILE_AGING_CONTROL_VERSION_2 2

/**
 * The PF_TRIM_WHILE_AGING_CONTROL_2 structure contains the legacy version-2 trim-while-aging request.
 */
typedef struct _PF_TRIM_WHILE_AGING_CONTROL_2
{
    ULONG Version;                              // Legacy version: PF_TRIM_WHILE_AGING_CONTROL_VERSION_2 (2).
    PF_TRIM_WHILE_AGING_STATE TrimWhileAgingState; // Legacy trim-while-aging policy selector.
    UCHAR PrivatePageTrimAge;                   // 0..7
    UCHAR SharedPageTrimAge;                    // 0..7
    USHORT Spare;                               // Must be 0
} PF_TRIM_WHILE_AGING_CONTROL_2, *PPF_TRIM_WHILE_AGING_CONTROL_2;

#define PF_TRIM_WHILE_AGING_CONTROL_VERSION_3 3

/**
 * The PF_TRIM_WHILE_AGING_CONTROL_3 structure sets trim-while-aging policy and optional custom thresholds.
 */
typedef struct _PF_TRIM_WHILE_AGING_CONTROL_3
{
    ULONG Version;                              // PF_TRIM_WHILE_AGING_CONTROL_VERSION_3
    PF_TRIM_WHILE_AGING_STATE TrimWhileAgingState; // Policy selector, 0..5; custom thresholds apply only to state 5.
    UCHAR PrivatePageTrimAge;                   // 0..7.
    UCHAR SharedPageTrimAge;                    // 0..7.
    USHORT Spare;                               // Must be zero.
    ULONG Reserved;                             // Reserved field; initialize to zero and do not interpret on output.
    SIZE_T CustomHighThreshold;                 // Custom: nonzero and greater than CustomLowThreshold.
    SIZE_T CustomLowThreshold;                  // Both thresholds must be zero for states 0..4.
} PF_TRIM_WHILE_AGING_CONTROL_3, *PPF_TRIM_WHILE_AGING_CONTROL_3;

// rev
/**
 * The PF_TIME_CONTROL structure adjusts the Superfetch base-time value.
 */
typedef struct _PF_TIME_CONTROL
{
    LONG TimeAdjustment;                        // Signed adjustment added to the Superfetch base-time value.
} PF_TIME_CONTROL, *PPF_TIME_CONTROL;

#define PF_MEMORY_LIST_INFO_VERSION 1

/**
 * The PF_MEMORY_LIST_INFO structure receives a variable array of memory-list node statistics.
 */
typedef struct _PF_MEMORY_LIST_INFO
{
    ULONG Version;                              // Output format version: PF_MEMORY_LIST_INFO_VERSION (1).
    ULONG Size;                                 // Output size in bytes; 64 on the verified x64 build.
    ULONG NodeCount;                            // Number of returned node records; 1 on the verified build.
    PF_MEMORY_LIST_NODE Nodes[1];               // Variable array of NodeCount memory-list records.
} PF_MEMORY_LIST_INFO, *PPF_MEMORY_LIST_INFO;

/**
 * The PF_PHYSICAL_MEMORY_RANGE structure describes a contiguous physical-memory range in page-frame units.
 */
typedef struct _PF_PHYSICAL_MEMORY_RANGE
{
    ULONG_PTR BasePfn;                          // First page-frame number in the physical range.
    ULONG_PTR PageCount;                        // Number of consecutive pages in the range.
} PF_PHYSICAL_MEMORY_RANGE, *PPF_PHYSICAL_MEMORY_RANGE;

#define PF_PHYSICAL_MEMORY_RANGE_INFO_V1_VERSION 1

/**
 * The PF_PHYSICAL_MEMORY_RANGE_INFO_V1 structure contains the legacy version-1 physical-memory range query.
 */
typedef struct _PF_PHYSICAL_MEMORY_RANGE_INFO_V1
{
    ULONG Version;                              // Legacy version: PF_PHYSICAL_MEMORY_RANGE_INFO_V1_VERSION (1).
    ULONG RangeCount;                           // Number of physical-memory range records.
    PF_PHYSICAL_MEMORY_RANGE Ranges[1];         // Variable array containing RangeCount physical-memory ranges.
} PF_PHYSICAL_MEMORY_RANGE_INFO_V1, *PPF_PHYSICAL_MEMORY_RANGE_INFO_V1;

#define PF_PHYSICAL_MEMORY_RANGE_INFO_V2_VERSION 2
#define PF_PHYSICAL_MEMORY_RANGE_INFO_V2_VALID_FLAGS 0x1 // Valid physical-range query flag mask (0x1).

/**
 * The PF_PHYSICAL_MEMORY_RANGE_INFO_V2 structure receives physical-memory ranges from SuperfetchMemoryRangesQuery.
 */
typedef struct _PF_PHYSICAL_MEMORY_RANGE_INFO_V2
{
    ULONG Version;                              // Request version: PF_PHYSICAL_MEMORY_RANGE_INFO_V2_VERSION (2).
    ULONG Flags;                                // Query flags; only bit 0 is accepted.
    ULONG RangeCount;                           // The kernel writes only 32 bits.
    ULONG Reserved;                             // Reserved field; initialize to zero and do not interpret on output.
    PF_PHYSICAL_MEMORY_RANGE Ranges[1];         // Variable array containing RangeCount returned physical-memory ranges.
} PF_PHYSICAL_MEMORY_RANGE_INFO_V2, *PPF_PHYSICAL_MEMORY_RANGE_INFO_V2;

// rev
/**
 * The PF_START_TRACE_CONTROL structure contains the version-3 access-tracing control layout.
 */
typedef struct _PF_START_TRACE_CONTROL
{
    ULONG Version;                              // Request version: PF_ACCESS_TRACING_CONTROL_VERSION (3).
    ULONG Type;                                 // Operation: 0 enables selected tracing flags; 1 removes them.
    ULONG Flags;                                // Selected tracing flags; valid mask 0x3.
    ULONG Reserved;                             // Reserved field; initialize to zero and do not interpret on output.
    HANDLE PartitionHandle;                     // Input handle identifying the tracing partition.
    HANDLE TraceHandle;                         // Must be NULL on input for Type 0; receives the trace handle on success.
} PF_START_TRACE_CONTROL, *PPF_START_TRACE_CONTROL;

// rev
#define PF_ACCESS_TRACING_CONTROL_VERSION 3

/**
 * The PF_ACCESS_TRACING_CONTROL structure controls access tracing for a Superfetch partition.
 */
typedef struct _PF_ACCESS_TRACING_CONTROL
{
    ULONG Version;                              // Request version: PF_ACCESS_TRACING_CONTROL_VERSION (3).
    ULONG Type;                                 // Operation: 0 enables selected tracing flags; 1 removes them.
    ULONG Flags;                                // Selected tracing flags; valid mask 0x3.
    ULONG Reserved;                             // Reserved field; initialize to zero and do not interpret on output.
    HANDLE PartitionHandle;                     // Input handle identifying the tracing partition.
    HANDLE TraceHandle;                         // Must be NULL on input for Type 0; receives the trace handle on success.
} PF_ACCESS_TRACING_CONTROL, *PPF_ACCESS_TRACING_CONTROL;

// rev
#define PF_REPURPOSED_BY_PREFETCH_INFO_VERSION 1

/**
 * The PF_REPURPOSED_BY_PREFETCH_INFO structure receives the count of pages repurposed by prefetching.
 */
typedef struct _PF_REPURPOSED_BY_PREFETCH_INFO
{
    ULONG Version;                              // PF_REPURPOSED_BY_PREFETCH_INFO_VERSION
    ULONG Reserved;                             // Reserved field; initialize to zero and do not interpret on output.
    SIZE_T RepurposedByPrefetch;                // Output count of pages repurposed by prefetching.
} PF_REPURPOSED_BY_PREFETCH_INFO, *PPF_REPURPOSED_BY_PREFETCH_INFO;

// rev
#define PF_VIRTUAL_QUERY_VERSION 1

/**
 * The PF_VIRTUAL_QUERY structure queries extended working-set information for a process.
 */
typedef struct _PF_VIRTUAL_QUERY
{
    ULONG Version;                              // Request version: PF_VIRTUAL_QUERY_VERSION (1).
    union
    {
        ULONG Flags;                            // Mutually exclusive page-table options; valid mask 0x3.
        struct
        {
            ULONG FaultInPageTables : 1;        // Fault page-table state in for the queried user VAs.
            ULONG ReportPageTables : 1;         // Report page-table-related state for the queried user VAs in the returned MEMORY_WORKING_SET_EX_INFORMATION entries.
            ULONG Spare : 30;                   // Reserved field; initialize to zero and do not interpret on output.
        };
    };
    PVOID QueryBuffer;                          // MEMORY_WORKING_SET_EX_INFORMATION[NumberOfPages] (input: VirtualAddress[], output: VirtualAttributes[])
    SIZE_T QueryBufferSize;                     // NumberOfPages * sizeof(MEMORY_WORKING_SET_EX_INFORMATION)
    HANDLE ProcessHandle;                       // Handle identifying the process whose working set is queried.
} PF_VIRTUAL_QUERY, *PPF_VIRTUAL_QUERY;

// rev
#define PF_PAGECOMBINE_AGGREGATE_STAT_VERSION 1

/**
 * The PF_PAGECOMBINE_AGGREGATE_STAT structure receives aggregate page-combining statistics.
 */
typedef struct _PF_PAGECOMBINE_AGGREGATE_STAT
{
    ULONG Version;                              // Request version: PF_PAGECOMBINE_AGGREGATE_STAT_VERSION (1).
    ULONG CombineScanCount;                     // Number of page-combining scans.
    ULONG CombinedBlocksInUse;                  // Number of combined blocks currently in use.
    ULONG SumCombinedBlocksReferenceCount;      // Sum of the reference counts of combined blocks.
} PF_PAGECOMBINE_AGGREGATE_STAT, *PPF_PAGECOMBINE_AGGREGATE_STAT;

// rev
#define PF_MIN_WS_AGE_RATE_CONTROL_VERSION 1

/**
 * The PF_MIN_WS_AGE_RATE_CONTROL structure sets the minimum working-set page-aging rate.
 */
typedef struct _PF_MIN_WS_AGE_RATE_CONTROL
{
    ULONG Version;                              // Request version: PF_MIN_WS_AGE_RATE_CONTROL_VERSION (1).
    ULONG SecondsToOldestAgeRate;               // Requested aging interval to the oldest age, in seconds.
} PF_MIN_WS_AGE_RATE_CONTROL, *PPF_MIN_WS_AGE_RATE_CONTROL;

// rev
#define PF_DEPRIORITIZE_OLD_PAGES_VERSION_3 3
#define PF_DEPRIORITIZE_OLD_PAGES_VERSION 4

/**
 * The PF_DEPRIORITIZE_OLD_PAGES_V3 structure contains the legacy version-3 working-set deprioritization request.
 */
typedef struct _PF_DEPRIORITIZE_OLD_PAGES_V3
{
    ULONG Version;                              // Legacy version: PF_DEPRIORITIZE_OLD_PAGES_VERSION_3 (3).
    HANDLE ProcessHandle;                       // Handle to the target process.
    union
    {
        ULONG Flags;                            // Legacy packed priority and trim options.
        struct
        {
            ULONG TargetPriority : 4;           // Legacy target page-priority field.
            ULONG TrimPages : 2;                // Legacy trim-control field.
            ULONG Spare : 26;                   // Reserved field; initialize to zero and do not interpret on output.
        };
    };
} PF_DEPRIORITIZE_OLD_PAGES_V3, *PPF_DEPRIORITIZE_OLD_PAGES_V3;

/**
 * The PF_DEPRIORITIZE_OLD_PAGES structure deprioritizes or trims old pages in a process working set.
 */
typedef struct _PF_DEPRIORITIZE_OLD_PAGES
{
    ULONG Version;                              // PF_DEPRIORITIZE_OLD_PAGES_VERSION
    HANDLE ProcessHandle;                       // Requires PROCESS_SET_LIMITED_INFORMATION.
    union
    {
        ULONG Flags;                            // Packed priority, trim and age controls; valid mask 0x1ff.
        struct
        {
            ULONG TargetPriority : 4;           // 0..8; 8 requires nonzero TrimPages.
            ULONG TrimPages : 2;                // 0..2.
            ULONG Age : 3;                      // 0..7.
            ULONG Spare : 23;                   // Must be zero.
        };
    };
    SIZE_T NumberOfPages;                       // In: maximum pages (0 = unlimited); out: pages processed.
} PF_DEPRIORITIZE_OLD_PAGES, *PPF_DEPRIORITIZE_OLD_PAGES;

// rev
#define PF_FILE_EXTENTS_INFO_VERSION 1

/**
 * The PF_FILE_EXTENTS_INFO structure contains the legacy version-1 file-extents declaration.
 */
typedef struct _PF_FILE_EXTENTS_INFO
{
    ULONG Version;                              // Legacy version: PF_FILE_EXTENTS_INFO_VERSION (1).
    PWSTR FilePath;                             // Path pointer in the legacy declaration.
    ULONG FilePathSize;                         // File-path size field in the legacy declaration.
    ULONG VolumePathSize;                       // Volume-path size field in the legacy declaration.
    LARGE_INTEGER FileIndexNumber;              // File identifier in the legacy declaration.
    ULONG VolumeSerialNumber;                   // Volume serial number in the legacy declaration.
    RETRIEVAL_POINTERS_BUFFER ExtentsBuffer;    // Inline retrieval-pointer buffer in the legacy declaration; not the version-2 pointer field.
    ULONGLONG ExtentsBufferSize;                // Extent-buffer size field in the legacy declaration.
} PF_FILE_EXTENTS_INFO, *PPF_FILE_EXTENTS_INFO;

// rev
#define PF_FILE_EXTENTS_INFO_VERSION2 2

/**
 * The PF_FILE_EXTENTS_INFO_V2 structure queries file retrieval extents and volume identity.
 */
typedef struct _PF_FILE_EXTENTS_INFO_V2
{
    ULONG Version;                              // PF_FILE_EXTENTS_INFO_VERSION2
    ULONG Reserved0;                            // Reserved field; initialize to zero and do not interpret on output.
    PWSTR FilePath;                             // NUL-terminated native path.
    ULONG FilePathSize;                         // Bytes including NUL; even, 2..0x100000.
    ULONG VolumePathSize;                       // Even, nonzero byte offset of the backslash after the volume path; < FilePathSize.
    LARGE_INTEGER FileIndexNumber;              // Output for a file with no retrieval extents (STATUS_END_OF_FILE).
    ULONG VolumeSerialNumber;                   // Output.
    ULONG Reserved1;                            // Reserved field; initialize to zero and do not interpret on output.
    PRETRIEVAL_POINTERS_BUFFER ExtentsBuffer;   // Caller-owned output buffer, not an inline structure.
    ULONG ExtentsBufferSize;                    // In: capacity; out: required/returned bytes. Maximum allocation 0xa00000.
    ULONG Reserved2;                            // Reserved field; initialize to zero and do not interpret on output.
} PF_FILE_EXTENTS_INFO_V2, *PPF_FILE_EXTENTS_INFO_V2;

// rev
#define PF_GPU_UTILIZATION_INFO_VERSION 1

/**
 * The PF_GPU_UTILIZATION_INFO structure queries GPU time for a session.
 */
typedef struct _PF_GPU_UTILIZATION_INFO
{
    ULONG Version;                              // Request version: PF_GPU_UTILIZATION_INFO_VERSION (1).
    ULONG SessionId;                            // 0xffffffff selects the current process session.
    ULONGLONG GpuTime;                          // Output GPU-time value returned by the selected session callout.
} PF_GPU_UTILIZATION_INFO, *PPF_GPU_UTILIZATION_INFO;

#endif // _NTPFAPI_H
