/*
 * Process support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTPSAPI_H
#define _NTPSAPI_H

#if (PHNT_MODE == PHNT_MODE_KERNEL)
#define PROCESS_TERMINATE 0x0001
#define PROCESS_CREATE_THREAD 0x0002
#define PROCESS_SET_SESSIONID 0x0004
#define PROCESS_VM_OPERATION 0x0008
#define PROCESS_VM_READ 0x0010
#define PROCESS_VM_WRITE 0x0020
//#define PROCESS_DUP_HANDLE 0x0040
#define PROCESS_CREATE_PROCESS 0x0080
#define PROCESS_SET_QUOTA 0x0100
#define PROCESS_SET_INFORMATION 0x0200
#define PROCESS_QUERY_INFORMATION 0x0400
#define PROCESS_SET_PORT 0x0800
#define PROCESS_SUSPEND_RESUME 0x0800
#define PROCESS_QUERY_LIMITED_INFORMATION 0x1000
#else
#ifndef PROCESS_SET_PORT
#define PROCESS_SET_PORT 0x0800
#endif
#endif

#if (PHNT_MODE == PHNT_MODE_KERNEL)
#define THREAD_QUERY_INFORMATION 0x0040
#define THREAD_SET_THREAD_TOKEN 0x0080
#define THREAD_IMPERSONATE 0x0100
#define THREAD_DIRECT_IMPERSONATION 0x0200
#else
#ifndef THREAD_ALERT
#define THREAD_ALERT 0x0004
#endif
#endif

#if (PHNT_MODE == PHNT_MODE_KERNEL)
#define JOB_OBJECT_ASSIGN_PROCESS 0x0001
#define JOB_OBJECT_SET_ATTRIBUTES 0x0002
#define JOB_OBJECT_QUERY 0x0004
#define JOB_OBJECT_TERMINATE 0x0008
#define JOB_OBJECT_SET_SECURITY_ATTRIBUTES 0x0010
#define JOB_OBJECT_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x3F)
//#define JOB_OBJECT_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1f) // pre-Vista full access
#endif

#define GDI_HANDLE_BUFFER_SIZE32 34
#define GDI_HANDLE_BUFFER_SIZE64 60

#ifndef _WIN64
#define GDI_HANDLE_BUFFER_SIZE GDI_HANDLE_BUFFER_SIZE32
#else
#define GDI_HANDLE_BUFFER_SIZE GDI_HANDLE_BUFFER_SIZE64
#endif

typedef ULONG GDI_HANDLE_BUFFER[GDI_HANDLE_BUFFER_SIZE];

typedef ULONG GDI_HANDLE_BUFFER32[GDI_HANDLE_BUFFER_SIZE32];
typedef ULONG GDI_HANDLE_BUFFER64[GDI_HANDLE_BUFFER_SIZE64];

#ifndef FLS_MAXIMUM_AVAILABLE
#define FLS_MAXIMUM_AVAILABLE 128
#endif
#ifndef TLS_MINIMUM_AVAILABLE
#define TLS_MINIMUM_AVAILABLE 64
#endif
#ifndef TLS_EXPANSION_SLOTS
#define TLS_EXPANSION_SLOTS 1024
#endif

// symbols
typedef struct _PEB_LDR_DATA
{
    ULONG Length;
    BOOLEAN Initialized;
    HANDLE SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
    PVOID EntryInProgress;
    BOOLEAN ShutdownInProgress;
    HANDLE ShutdownThreadId;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _INITIAL_TEB
{
    struct
    {
        PVOID OldStackBase;
        PVOID OldStackLimit;
    } OldInitialTeb;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID StackAllocationBase;
} INITIAL_TEB, *PINITIAL_TEB;

typedef struct _WOW64_PROCESS
{
    PVOID Wow64;
} WOW64_PROCESS, *PWOW64_PROCESS;

#include <ntpebteb.h>

#if (PHNT_MODE != PHNT_MODE_KERNEL)
typedef enum _PROCESSINFOCLASS
{
    ProcessBasicInformation, // q: PROCESS_BASIC_INFORMATION, PROCESS_EXTENDED_BASIC_INFORMATION
    ProcessQuotaLimits, // qs: QUOTA_LIMITS, QUOTA_LIMITS_EX
    ProcessIoCounters, // q: IO_COUNTERS
    ProcessVmCounters, // q: VM_COUNTERS, VM_COUNTERS_EX, VM_COUNTERS_EX2
    ProcessTimes, // q: KERNEL_USER_TIMES
    ProcessBasePriority, // s: KPRIORITY
    ProcessRaisePriority, // s: ULONG
    ProcessDebugPort, // q: HANDLE
    ProcessExceptionPort, // s: PROCESS_EXCEPTION_PORT (requires SeTcbPrivilege)
    ProcessAccessToken, // s: PROCESS_ACCESS_TOKEN
    ProcessLdtInformation, // qs: PROCESS_LDT_INFORMATION // 10
    ProcessLdtSize, // s: PROCESS_LDT_SIZE
    ProcessDefaultHardErrorMode, // qs: ULONG
    ProcessIoPortHandlers, // (kernel-mode only) // PROCESS_IO_PORT_HANDLER_INFORMATION
    ProcessPooledUsageAndLimits, // q: POOLED_USAGE_AND_LIMITS
    ProcessWorkingSetWatch, // q: PROCESS_WS_WATCH_INFORMATION[]; s: void
    ProcessUserModeIOPL, // qs: ULONG (requires SeTcbPrivilege)
    ProcessEnableAlignmentFaultFixup, // s: BOOLEAN
    ProcessPriorityClass, // qs: PROCESS_PRIORITY_CLASS
    ProcessWx86Information, // qs: ULONG (requires SeTcbPrivilege) (VdmAllowed)
    ProcessHandleCount, // q: ULONG, PROCESS_HANDLE_INFORMATION // 20
    ProcessAffinityMask, // (q >WIN7)s: KAFFINITY, qs: GROUP_AFFINITY
    ProcessPriorityBoost, // qs: ULONG
    ProcessDeviceMap, // qs: PROCESS_DEVICEMAP_INFORMATION, PROCESS_DEVICEMAP_INFORMATION_EX
    ProcessSessionInformation, // q: PROCESS_SESSION_INFORMATION
    ProcessForegroundInformation, // s: PROCESS_FOREGROUND_BACKGROUND
    ProcessWow64Information, // q: ULONG_PTR
    ProcessImageFileName, // q: UNICODE_STRING
    ProcessLUIDDeviceMapsEnabled, // q: ULONG
    ProcessBreakOnTermination, // qs: ULONG
    ProcessDebugObjectHandle, // q: HANDLE // 30
    ProcessDebugFlags, // qs: ULONG
    ProcessHandleTracing, // q: PROCESS_HANDLE_TRACING_QUERY; s: size 0 disables, otherwise enables
    ProcessIoPriority, // qs: IO_PRIORITY_HINT
    ProcessExecuteFlags, // qs: ULONG
    ProcessTlsInformation, // PROCESS_TLS_INFORMATION // ProcessResourceManagement
    ProcessCookie, // q: ULONG
    ProcessImageInformation, // q: SECTION_IMAGE_INFORMATION
    ProcessCycleTime, // q: PROCESS_CYCLE_TIME_INFORMATION // since VISTA
    ProcessPagePriority, // qs: PAGE_PRIORITY_INFORMATION
    ProcessInstrumentationCallback, // s: PVOID or PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION // 40
    ProcessThreadStackAllocation, // s: PROCESS_STACK_ALLOCATION_INFORMATION, PROCESS_STACK_ALLOCATION_INFORMATION_EX
    ProcessWorkingSetWatchEx, // q: PROCESS_WS_WATCH_INFORMATION_EX[]
    ProcessImageFileNameWin32, // q: UNICODE_STRING
    ProcessImageFileMapping, // q: HANDLE (input)
    ProcessAffinityUpdateMode, // qs: PROCESS_AFFINITY_UPDATE_MODE
    ProcessMemoryAllocationMode, // qs: PROCESS_MEMORY_ALLOCATION_MODE
    ProcessGroupInformation, // q: USHORT[]
    ProcessTokenVirtualizationEnabled, // s: ULONG
    ProcessConsoleHostProcess, // qs: ULONG_PTR // ProcessOwnerInformation
    ProcessWindowInformation, // q: PROCESS_WINDOW_INFORMATION // 50
    ProcessHandleInformation, // q: PROCESS_HANDLE_SNAPSHOT_INFORMATION // since WIN8
    ProcessMitigationPolicy, // s: PROCESS_MITIGATION_POLICY_INFORMATION
    ProcessDynamicFunctionTableInformation,
    ProcessHandleCheckingMode, // qs: ULONG; s: 0 disables, otherwise enables
    ProcessKeepAliveCount, // q: PROCESS_KEEPALIVE_COUNT_INFORMATION
    ProcessRevokeFileHandles, // s: PROCESS_REVOKE_FILE_HANDLES_INFORMATION
    ProcessWorkingSetControl, // s: PROCESS_WORKING_SET_CONTROL (requires SeDebugPrivilege)
    ProcessHandleTable, // q: ULONG[] // since WINBLUE
    ProcessCheckStackExtentsMode, // qs: ULONG // KPROCESS->CheckStackExtents (CFG)
    ProcessCommandLineInformation, // q: UNICODE_STRING // 60
    ProcessProtectionInformation, // q: PS_PROTECTION
    ProcessMemoryExhaustion, // PROCESS_MEMORY_EXHAUSTION_INFO // since THRESHOLD
    ProcessFaultInformation, // PROCESS_FAULT_INFORMATION
    ProcessTelemetryIdInformation, // q: PROCESS_TELEMETRY_ID_INFORMATION
    ProcessCommitReleaseInformation, // PROCESS_COMMIT_RELEASE_INFORMATION
    ProcessDefaultCpuSetsInformation, // SYSTEM_CPU_SET_INFORMATION[5]
    ProcessAllowedCpuSetsInformation, // SYSTEM_CPU_SET_INFORMATION[5]
    ProcessSubsystemProcess,
    ProcessJobMemoryInformation, // q: PROCESS_JOB_MEMORY_INFO
    ProcessInPrivate, // s: void // ETW // since THRESHOLD2 // 70
    ProcessRaiseUMExceptionOnInvalidHandleClose, // qs: ULONG; s: 0 disables, otherwise enables
    ProcessIumChallengeResponse,
    ProcessChildProcessInformation, // q: PROCESS_CHILD_PROCESS_INFORMATION
    ProcessHighGraphicsPriorityInformation, // qs: BOOLEAN (requires SeTcbPrivilege)
    ProcessSubsystemInformation, // q: SUBSYSTEM_INFORMATION_TYPE // since REDSTONE2
    ProcessEnergyValues, // q: PROCESS_ENERGY_VALUES, PROCESS_EXTENDED_ENERGY_VALUES
    ProcessPowerThrottlingState, // qs: POWER_THROTTLING_PROCESS_STATE
    ProcessReserved3Information, // ProcessActivityThrottlePolicy // PROCESS_ACTIVITY_THROTTLE_POLICY
    ProcessWin32kSyscallFilterInformation, // q: WIN32K_SYSCALL_FILTER
    ProcessDisableSystemAllowedCpuSets, // 80
    ProcessWakeInformation, // PROCESS_WAKE_INFORMATION
    ProcessEnergyTrackingState, // qs: PROCESS_ENERGY_TRACKING_STATE
    ProcessManageWritesToExecutableMemory, // MANAGE_WRITES_TO_EXECUTABLE_MEMORY // since REDSTONE3
    ProcessCaptureTrustletLiveDump,
    ProcessTelemetryCoverage,
    ProcessEnclaveInformation,
    ProcessEnableReadWriteVmLogging, // PROCESS_READWRITEVM_LOGGING_INFORMATION
    ProcessUptimeInformation, // q: PROCESS_UPTIME_INFORMATION
    ProcessImageSection, // q: HANDLE
    ProcessDebugAuthInformation, // since REDSTONE4 // 90
    ProcessSystemResourceManagement, // PROCESS_SYSTEM_RESOURCE_MANAGEMENT
    ProcessSequenceNumber, // q: ULONGLONG
    ProcessLoaderDetour, // since REDSTONE5
    ProcessSecurityDomainInformation, // PROCESS_SECURITY_DOMAIN_INFORMATION
    ProcessCombineSecurityDomainsInformation, // PROCESS_COMBINE_SECURITY_DOMAINS_INFORMATION
    ProcessEnableLogging, // PROCESS_LOGGING_INFORMATION
    ProcessLeapSecondInformation, // PROCESS_LEAP_SECOND_INFORMATION
    ProcessFiberShadowStackAllocation, // PROCESS_FIBER_SHADOW_STACK_ALLOCATION_INFORMATION // since 19H1
    ProcessFreeFiberShadowStackAllocation, // PROCESS_FREE_FIBER_SHADOW_STACK_ALLOCATION_INFORMATION
    ProcessAltSystemCallInformation, // since 20H1 // 100
    ProcessDynamicEHContinuationTargets, // PROCESS_DYNAMIC_EH_CONTINUATION_TARGETS_INFORMATION
    ProcessDynamicEnforcedCetCompatibleRanges, // PROCESS_DYNAMIC_ENFORCED_ADDRESS_RANGE_INFORMATION // since 20H2
    ProcessCreateStateChange, // since WIN11
    ProcessApplyStateChange,
    ProcessEnableOptionalXStateFeatures, // ULONG64 // optional XState feature bitmask
    ProcessAltPrefetchParam, // since 22H1
    ProcessAssignCpuPartitions,
    ProcessPriorityClassEx, // s: PROCESS_PRIORITY_CLASS_EX
    ProcessMembershipInformation, // PROCESS_MEMBERSHIP_INFORMATION
    ProcessEffectiveIoPriority, // q: IO_PRIORITY_HINT
    ProcessEffectivePagePriority, // q: ULONG
    MaxProcessInfoClass
} PROCESSINFOCLASS;
#endif

#if (PHNT_MODE != PHNT_MODE_KERNEL)
typedef enum _THREADINFOCLASS
{
    ThreadBasicInformation, // q: THREAD_BASIC_INFORMATION
    ThreadTimes, // q: KERNEL_USER_TIMES
    ThreadPriority, // s: KPRIORITY (requires SeIncreaseBasePriorityPrivilege)
    ThreadBasePriority, // s: KPRIORITY
    ThreadAffinityMask, // s: KAFFINITY
    ThreadImpersonationToken, // s: HANDLE
    ThreadDescriptorTableEntry, // q: DESCRIPTOR_TABLE_ENTRY (or WOW64_DESCRIPTOR_TABLE_ENTRY)
    ThreadEnableAlignmentFaultFixup, // s: BOOLEAN
    ThreadEventPair,
    ThreadQuerySetWin32StartAddress, // q: ULONG_PTR
    ThreadZeroTlsCell, // s: ULONG // TlsIndex // 10
    ThreadPerformanceCount, // q: LARGE_INTEGER
    ThreadAmILastThread, // q: ULONG
    ThreadIdealProcessor, // s: ULONG
    ThreadPriorityBoost, // qs: ULONG
    ThreadSetTlsArrayAddress, // s: ULONG_PTR
    ThreadIsIoPending, // q: ULONG
    ThreadHideFromDebugger, // q: BOOLEAN; s: void
    ThreadBreakOnTermination, // qs: ULONG
    ThreadSwitchLegacyState, // s: void // NtCurrentThread // NPX/FPU
    ThreadIsTerminated, // q: ULONG // 20
    ThreadLastSystemCall, // q: THREAD_LAST_SYSCALL_INFORMATION
    ThreadIoPriority, // qs: IO_PRIORITY_HINT (requires SeIncreaseBasePriorityPrivilege)
    ThreadCycleTime, // q: THREAD_CYCLE_TIME_INFORMATION
    ThreadPagePriority, // qs: PAGE_PRIORITY_INFORMATION
    ThreadActualBasePriority, // s: LONG (requires SeIncreaseBasePriorityPrivilege)
    ThreadTebInformation, // q: THREAD_TEB_INFORMATION (requires THREAD_GET_CONTEXT + THREAD_SET_CONTEXT)
    ThreadCSwitchMon,
    ThreadCSwitchPmu,
    ThreadWow64Context, // qs: WOW64_CONTEX, ARM_NT_CONTEXT since 20H1
    ThreadGroupInformation, // qs: GROUP_AFFINITY // 30
    ThreadUmsInformation, // q: THREAD_UMS_INFORMATION
    ThreadCounterProfiling, // q: BOOLEAN; s: THREAD_PROFILING_INFORMATION?
    ThreadIdealProcessorEx, // qs: PROCESSOR_NUMBER; s: previous PROCESSOR_NUMBER on return
    ThreadCpuAccountingInformation, // q: BOOLEAN; s: HANDLE (NtOpenSession) // NtCurrentThread // since WIN8
    ThreadSuspendCount, // q: ULONG // since WINBLUE
    ThreadHeterogeneousCpuPolicy, // q: KHETERO_CPU_POLICY // since THRESHOLD
    ThreadContainerId, // q: GUID
    ThreadNameInformation, // qs: THREAD_NAME_INFORMATION
    ThreadSelectedCpuSets,
    ThreadSystemThreadInformation, // q: SYSTEM_THREAD_INFORMATION // 40
    ThreadActualGroupAffinity, // q: GROUP_AFFINITY // since THRESHOLD2
    ThreadDynamicCodePolicyInfo, // q: ULONG; s: ULONG (NtCurrentThread)
    ThreadExplicitCaseSensitivity, // qs: ULONG; s: 0 disables, otherwise enables
    ThreadWorkOnBehalfTicket, // RTL_WORK_ON_BEHALF_TICKET_EX
    ThreadSubsystemInformation, // q: SUBSYSTEM_INFORMATION_TYPE // since REDSTONE2
    ThreadDbgkWerReportActive, // s: ULONG; s: 0 disables, otherwise enables
    ThreadAttachContainer, // s: HANDLE (job object) // NtCurrentThread
    ThreadManageWritesToExecutableMemory, // MANAGE_WRITES_TO_EXECUTABLE_MEMORY // since REDSTONE3
    ThreadPowerThrottlingState, // POWER_THROTTLING_THREAD_STATE
    ThreadWorkloadClass, // THREAD_WORKLOAD_CLASS // since REDSTONE5 // 50
    ThreadCreateStateChange, // since WIN11
    ThreadApplyStateChange,
    ThreadStrongerBadHandleChecks, // since 22H1
    ThreadEffectiveIoPriority, // q: IO_PRIORITY_HINT
    ThreadEffectivePagePriority, // q: ULONG
    MaxThreadInfoClass
} THREADINFOCLASS;
#endif

#if (PHNT_MODE != PHNT_MODE_KERNEL)
// Use with both ProcessPagePriority and ThreadPagePriority
typedef struct _PAGE_PRIORITY_INFORMATION
{
    ULONG PagePriority;
} PAGE_PRIORITY_INFORMATION, *PPAGE_PRIORITY_INFORMATION;
#endif

// Process information structures

#if (PHNT_MODE != PHNT_MODE_KERNEL)

typedef struct _PROCESS_BASIC_INFORMATION
{
    NTSTATUS ExitStatus;
    PPEB PebBaseAddress;
    KAFFINITY AffinityMask;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

typedef struct _PROCESS_EXTENDED_BASIC_INFORMATION
{
    SIZE_T Size; // set to sizeof structure on input
    PROCESS_BASIC_INFORMATION BasicInfo;
    union
    {
        ULONG Flags;
        struct
        {
            ULONG IsProtectedProcess : 1;
            ULONG IsWow64Process : 1;
            ULONG IsProcessDeleting : 1;
            ULONG IsCrossSessionCreate : 1;
            ULONG IsFrozen : 1;
            ULONG IsBackground : 1;
            ULONG IsStronglyNamed : 1;
            ULONG IsSecureProcess : 1;
            ULONG IsSubsystemProcess : 1;
            ULONG SpareBits : 23;
        };
    };
} PROCESS_EXTENDED_BASIC_INFORMATION, *PPROCESS_EXTENDED_BASIC_INFORMATION;

typedef struct _VM_COUNTERS
{
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
} VM_COUNTERS, *PVM_COUNTERS;

typedef struct _VM_COUNTERS_EX
{
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivateUsage;
} VM_COUNTERS_EX, *PVM_COUNTERS_EX;

// private
typedef struct _VM_COUNTERS_EX2
{
    VM_COUNTERS_EX CountersEx;
    SIZE_T PrivateWorkingSetSize;
    SIZE_T SharedCommitUsage;
} VM_COUNTERS_EX2, *PVM_COUNTERS_EX2;

typedef struct _KERNEL_USER_TIMES
{
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER ExitTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
} KERNEL_USER_TIMES, *PKERNEL_USER_TIMES;

typedef struct _POOLED_USAGE_AND_LIMITS
{
    SIZE_T PeakPagedPoolUsage;
    SIZE_T PagedPoolUsage;
    SIZE_T PagedPoolLimit;
    SIZE_T PeakNonPagedPoolUsage;
    SIZE_T NonPagedPoolUsage;
    SIZE_T NonPagedPoolLimit;
    SIZE_T PeakPagefileUsage;
    SIZE_T PagefileUsage;
    SIZE_T PagefileLimit;
} POOLED_USAGE_AND_LIMITS, *PPOOLED_USAGE_AND_LIMITS;

#define PROCESS_EXCEPTION_PORT_ALL_STATE_BITS 0x00000003
#define PROCESS_EXCEPTION_PORT_ALL_STATE_FLAGS ((ULONG_PTR)((1UL << PROCESS_EXCEPTION_PORT_ALL_STATE_BITS) - 1))

typedef struct _PROCESS_EXCEPTION_PORT
{
    _In_ HANDLE ExceptionPortHandle; // Handle to the exception port. No particular access required.
    _Inout_ ULONG StateFlags; // Miscellaneous state flags to be cached along with the exception port in the kernel.
} PROCESS_EXCEPTION_PORT, *PPROCESS_EXCEPTION_PORT;

typedef struct _PROCESS_ACCESS_TOKEN
{
    HANDLE Token; // needs TOKEN_ASSIGN_PRIMARY access
    HANDLE Thread; // handle to initial/only thread; needs THREAD_QUERY_INFORMATION access
} PROCESS_ACCESS_TOKEN, *PPROCESS_ACCESS_TOKEN;

typedef struct _PROCESS_LDT_INFORMATION
{
    ULONG Start;
    ULONG Length;
    LDT_ENTRY LdtEntries[1];
} PROCESS_LDT_INFORMATION, *PPROCESS_LDT_INFORMATION;

typedef struct _PROCESS_LDT_SIZE
{
    ULONG Length;
} PROCESS_LDT_SIZE, *PPROCESS_LDT_SIZE;

typedef struct _PROCESS_WS_WATCH_INFORMATION
{
    PVOID FaultingPc;
    PVOID FaultingVa;
} PROCESS_WS_WATCH_INFORMATION, *PPROCESS_WS_WATCH_INFORMATION;

#endif

// psapi:PSAPI_WS_WATCH_INFORMATION_EX
typedef struct _PROCESS_WS_WATCH_INFORMATION_EX
{
    PROCESS_WS_WATCH_INFORMATION BasicInfo;
    ULONG_PTR FaultingThreadId;
    ULONG_PTR Flags;
} PROCESS_WS_WATCH_INFORMATION_EX, *PPROCESS_WS_WATCH_INFORMATION_EX;

#define PROCESS_PRIORITY_CLASS_UNKNOWN 0
#define PROCESS_PRIORITY_CLASS_IDLE 1
#define PROCESS_PRIORITY_CLASS_NORMAL 2
#define PROCESS_PRIORITY_CLASS_HIGH 3
#define PROCESS_PRIORITY_CLASS_REALTIME 4
#define PROCESS_PRIORITY_CLASS_BELOW_NORMAL 5
#define PROCESS_PRIORITY_CLASS_ABOVE_NORMAL 6

typedef struct _PROCESS_PRIORITY_CLASS
{
    BOOLEAN Foreground;
    UCHAR PriorityClass;
} PROCESS_PRIORITY_CLASS, *PPROCESS_PRIORITY_CLASS;

typedef struct _PROCESS_PRIORITY_CLASS_EX
{
    union
    {
        struct
        {
            USHORT ForegroundValid : 1;
            USHORT PriorityClassValid : 1;
        };
        USHORT AllFlags;
    };
    UCHAR PriorityClass;
    BOOLEAN Foreground;
} PROCESS_PRIORITY_CLASS_EX, *PPROCESS_PRIORITY_CLASS_EX;

typedef struct _PROCESS_FOREGROUND_BACKGROUND
{
    BOOLEAN Foreground;
} PROCESS_FOREGROUND_BACKGROUND, *PPROCESS_FOREGROUND_BACKGROUND;

#if (PHNT_MODE != PHNT_MODE_KERNEL)

typedef struct _PROCESS_DEVICEMAP_INFORMATION
{
    union
    {
        struct
        {
            HANDLE DirectoryHandle;
        } Set;
        struct
        {
            ULONG DriveMap;
            UCHAR DriveType[32];
        } Query;
    };
} PROCESS_DEVICEMAP_INFORMATION, *PPROCESS_DEVICEMAP_INFORMATION;

#define PROCESS_LUID_DOSDEVICES_ONLY 0x00000001

typedef struct _PROCESS_DEVICEMAP_INFORMATION_EX
{
    union
    {
        struct
        {
            HANDLE DirectoryHandle;
        } Set;
        struct
        {
            ULONG DriveMap;
            UCHAR DriveType[32];
        } Query;
    };
    ULONG Flags; // PROCESS_LUID_DOSDEVICES_ONLY
} PROCESS_DEVICEMAP_INFORMATION_EX, *PPROCESS_DEVICEMAP_INFORMATION_EX;

typedef struct _PROCESS_SESSION_INFORMATION
{
    ULONG SessionId;
} PROCESS_SESSION_INFORMATION, *PPROCESS_SESSION_INFORMATION;

#define PROCESS_HANDLE_EXCEPTIONS_ENABLED 0x00000001

#define PROCESS_HANDLE_RAISE_EXCEPTION_ON_INVALID_HANDLE_CLOSE_DISABLED 0x00000000
#define PROCESS_HANDLE_RAISE_EXCEPTION_ON_INVALID_HANDLE_CLOSE_ENABLED 0x00000001

typedef struct _PROCESS_HANDLE_TRACING_ENABLE
{
    ULONG Flags;
} PROCESS_HANDLE_TRACING_ENABLE, *PPROCESS_HANDLE_TRACING_ENABLE;

#define PROCESS_HANDLE_TRACING_MAX_SLOTS 0x20000

typedef struct _PROCESS_HANDLE_TRACING_ENABLE_EX
{
    ULONG Flags;
    ULONG TotalSlots;
} PROCESS_HANDLE_TRACING_ENABLE_EX, *PPROCESS_HANDLE_TRACING_ENABLE_EX;

#define PROCESS_HANDLE_TRACING_MAX_STACKS 16

#define PROCESS_HANDLE_TRACE_TYPE_OPEN 1
#define PROCESS_HANDLE_TRACE_TYPE_CLOSE 2
#define PROCESS_HANDLE_TRACE_TYPE_BADREF 3

typedef struct _PROCESS_HANDLE_TRACING_ENTRY
{
    HANDLE Handle;
    CLIENT_ID ClientId;
    ULONG Type;
    PVOID Stacks[PROCESS_HANDLE_TRACING_MAX_STACKS];
} PROCESS_HANDLE_TRACING_ENTRY, *PPROCESS_HANDLE_TRACING_ENTRY;

typedef struct _PROCESS_HANDLE_TRACING_QUERY
{
    HANDLE Handle;
    ULONG TotalTraces;
    _Field_size_(TotalTraces) PROCESS_HANDLE_TRACING_ENTRY HandleTrace[1];
} PROCESS_HANDLE_TRACING_QUERY, *PPROCESS_HANDLE_TRACING_QUERY;

#endif

// private
typedef struct _THREAD_TLS_INFORMATION
{
    ULONG Flags;
    PVOID NewTlsData;
    PVOID OldTlsData;
    HANDLE ThreadId;
} THREAD_TLS_INFORMATION, *PTHREAD_TLS_INFORMATION;

// private
typedef enum _PROCESS_TLS_INFORMATION_TYPE
{
    ProcessTlsReplaceIndex,
    ProcessTlsReplaceVector,
    MaxProcessTlsOperation
} PROCESS_TLS_INFORMATION_TYPE, *PPROCESS_TLS_INFORMATION_TYPE;

// private
typedef struct _PROCESS_TLS_INFORMATION
{
    ULONG Flags;
    ULONG OperationType;
    ULONG ThreadDataCount;
    ULONG TlsIndex;
    ULONG PreviousCount;
    _Field_size_(ThreadDataCount) THREAD_TLS_INFORMATION ThreadData[1];
} PROCESS_TLS_INFORMATION, *PPROCESS_TLS_INFORMATION;

// private
typedef struct _PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION
{
    ULONG Version;
    ULONG Reserved;
    PVOID Callback;
} PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION, *PPROCESS_INSTRUMENTATION_CALLBACK_INFORMATION;

// private
typedef struct _PROCESS_STACK_ALLOCATION_INFORMATION
{
    SIZE_T ReserveSize;
    SIZE_T ZeroBits;
    PVOID StackBase;
} PROCESS_STACK_ALLOCATION_INFORMATION, *PPROCESS_STACK_ALLOCATION_INFORMATION;

// private
typedef struct _PROCESS_STACK_ALLOCATION_INFORMATION_EX
{
    ULONG PreferredNode;
    ULONG Reserved0;
    ULONG Reserved1;
    ULONG Reserved2;
    PROCESS_STACK_ALLOCATION_INFORMATION AllocInfo;
} PROCESS_STACK_ALLOCATION_INFORMATION_EX, *PPROCESS_STACK_ALLOCATION_INFORMATION_EX;

// private
typedef union _PROCESS_AFFINITY_UPDATE_MODE
{
    ULONG Flags;
    struct
    {
        ULONG EnableAutoUpdate : 1;
        ULONG Permanent : 1;
        ULONG Reserved : 30;
    };
} PROCESS_AFFINITY_UPDATE_MODE, *PPROCESS_AFFINITY_UPDATE_MODE;

// private
typedef union _PROCESS_MEMORY_ALLOCATION_MODE
{
    ULONG Flags;
    struct
    {
        ULONG TopDown : 1;
        ULONG Reserved : 31;
    };
} PROCESS_MEMORY_ALLOCATION_MODE, *PPROCESS_MEMORY_ALLOCATION_MODE;

// private
typedef struct _PROCESS_HANDLE_INFORMATION
{
    ULONG HandleCount;
    ULONG HandleCountHighWatermark;
} PROCESS_HANDLE_INFORMATION, *PPROCESS_HANDLE_INFORMATION;

// private
typedef struct _PROCESS_CYCLE_TIME_INFORMATION
{
    ULONGLONG AccumulatedCycles;
    ULONGLONG CurrentCycleCount;
} PROCESS_CYCLE_TIME_INFORMATION, *PPROCESS_CYCLE_TIME_INFORMATION;

// private
typedef struct _PROCESS_WINDOW_INFORMATION
{
    ULONG WindowFlags;
    USHORT WindowTitleLength;
    _Field_size_bytes_(WindowTitleLength) WCHAR WindowTitle[1];
} PROCESS_WINDOW_INFORMATION, *PPROCESS_WINDOW_INFORMATION;

// private
typedef struct _PROCESS_HANDLE_TABLE_ENTRY_INFO
{
    HANDLE HandleValue;
    ULONG_PTR HandleCount;
    ULONG_PTR PointerCount;
    ULONG GrantedAccess;
    ULONG ObjectTypeIndex;
    ULONG HandleAttributes;
    ULONG Reserved;
} PROCESS_HANDLE_TABLE_ENTRY_INFO, *PPROCESS_HANDLE_TABLE_ENTRY_INFO;

// private
typedef struct _PROCESS_HANDLE_SNAPSHOT_INFORMATION
{
    ULONG_PTR NumberOfHandles;
    ULONG_PTR Reserved;
    _Field_size_(NumberOfHandles) PROCESS_HANDLE_TABLE_ENTRY_INFO Handles[1];
} PROCESS_HANDLE_SNAPSHOT_INFORMATION, *PPROCESS_HANDLE_SNAPSHOT_INFORMATION;

#if (PHNT_MODE != PHNT_MODE_KERNEL)

#if !defined(NTDDI_WIN10_FE) || (NTDDI_VERSION < NTDDI_WIN10_FE)
typedef struct _PROCESS_MITIGATION_REDIRECTION_TRUST_POLICY
{
    union {
        ULONG Flags;
        struct {
            ULONG EnforceRedirectionTrust : 1;
            ULONG AuditRedirectionTrust : 1;
            ULONG ReservedFlags : 30;
        };
    };
} PROCESS_MITIGATION_REDIRECTION_TRUST_POLICY, * PPROCESS_MITIGATION_REDIRECTION_TRUST_POLICY;
#endif

// private
typedef struct _PROCESS_MITIGATION_POLICY_INFORMATION
{
    PROCESS_MITIGATION_POLICY Policy;
    union
    {
        PROCESS_MITIGATION_ASLR_POLICY ASLRPolicy;
        PROCESS_MITIGATION_STRICT_HANDLE_CHECK_POLICY StrictHandleCheckPolicy;
        PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY SystemCallDisablePolicy;
        PROCESS_MITIGATION_EXTENSION_POINT_DISABLE_POLICY ExtensionPointDisablePolicy;
        PROCESS_MITIGATION_DYNAMIC_CODE_POLICY DynamicCodePolicy;
        PROCESS_MITIGATION_CONTROL_FLOW_GUARD_POLICY ControlFlowGuardPolicy;
        PROCESS_MITIGATION_BINARY_SIGNATURE_POLICY SignaturePolicy;
        PROCESS_MITIGATION_FONT_DISABLE_POLICY FontDisablePolicy;
        PROCESS_MITIGATION_IMAGE_LOAD_POLICY ImageLoadPolicy;
        PROCESS_MITIGATION_SYSTEM_CALL_FILTER_POLICY SystemCallFilterPolicy;
        PROCESS_MITIGATION_PAYLOAD_RESTRICTION_POLICY PayloadRestrictionPolicy;
        PROCESS_MITIGATION_CHILD_PROCESS_POLICY ChildProcessPolicy;
        PROCESS_MITIGATION_SIDE_CHANNEL_ISOLATION_POLICY SideChannelIsolationPolicy;
        PROCESS_MITIGATION_USER_SHADOW_STACK_POLICY UserShadowStackPolicy;
        PROCESS_MITIGATION_REDIRECTION_TRUST_POLICY RedirectionTrustPolicy;
        PROCESS_MITIGATION_USER_POINTER_AUTH_POLICY UserPointerAuthPolicy;
        PROCESS_MITIGATION_SEHOP_POLICY SEHOPPolicy;
    };
} PROCESS_MITIGATION_POLICY_INFORMATION, *PPROCESS_MITIGATION_POLICY_INFORMATION;

typedef struct _PROCESS_KEEPALIVE_COUNT_INFORMATION
{
    ULONG WakeCount;
    ULONG NoWakeCount;
} PROCESS_KEEPALIVE_COUNT_INFORMATION, *PPROCESS_KEEPALIVE_COUNT_INFORMATION;

typedef struct _PROCESS_REVOKE_FILE_HANDLES_INFORMATION
{
    UNICODE_STRING TargetDevicePath;
} PROCESS_REVOKE_FILE_HANDLES_INFORMATION, *PPROCESS_REVOKE_FILE_HANDLES_INFORMATION;

// begin_private

typedef enum _PROCESS_WORKING_SET_OPERATION
{
    ProcessWorkingSetSwap,
    ProcessWorkingSetEmpty,
    ProcessWorkingSetOperationMax
} PROCESS_WORKING_SET_OPERATION;

typedef struct _PROCESS_WORKING_SET_CONTROL
{
    ULONG Version;
    PROCESS_WORKING_SET_OPERATION Operation;
    ULONG Flags;
} PROCESS_WORKING_SET_CONTROL, *PPROCESS_WORKING_SET_CONTROL;

typedef enum _PS_PROTECTED_TYPE
{
    PsProtectedTypeNone,
    PsProtectedTypeProtectedLight,
    PsProtectedTypeProtected,
    PsProtectedTypeMax
} PS_PROTECTED_TYPE;

typedef enum _PS_PROTECTED_SIGNER
{
    PsProtectedSignerNone,
    PsProtectedSignerAuthenticode,
    PsProtectedSignerCodeGen,
    PsProtectedSignerAntimalware,
    PsProtectedSignerLsa,
    PsProtectedSignerWindows,
    PsProtectedSignerWinTcb,
    PsProtectedSignerWinSystem,
    PsProtectedSignerApp,
    PsProtectedSignerMax
} PS_PROTECTED_SIGNER;

#define PS_PROTECTED_SIGNER_MASK 0xFF
#define PS_PROTECTED_AUDIT_MASK 0x08
#define PS_PROTECTED_TYPE_MASK 0x07

// vProtectionLevel.Level = PsProtectedValue(PsProtectedSignerCodeGen, FALSE, PsProtectedTypeProtectedLight)
#define PsProtectedValue(aSigner, aAudit, aType) ( \
    ((aSigner & PS_PROTECTED_SIGNER_MASK) << 4) | \
    ((aAudit & PS_PROTECTED_AUDIT_MASK) << 3) | \
    (aType & PS_PROTECTED_TYPE_MASK)\
    )

// InitializePsProtection(&vProtectionLevel, PsProtectedSignerCodeGen, FALSE, PsProtectedTypeProtectedLight)
#define InitializePsProtection(aProtectionLevelPtr, aSigner, aAudit, aType) { \
    (aProtectionLevelPtr)->Signer = aSigner; \
    (aProtectionLevelPtr)->Audit = aAudit; \
    (aProtectionLevelPtr)->Type = aType; \
    }

typedef struct _PS_PROTECTION
{
    union
    {
        UCHAR Level;
        struct
        {
            UCHAR Type : 3;
            UCHAR Audit : 1;
            UCHAR Signer : 4;
        };
    };
} PS_PROTECTION, *PPS_PROTECTION;

typedef struct _PROCESS_FAULT_INFORMATION
{
    ULONG FaultFlags;
    ULONG AdditionalInfo;
} PROCESS_FAULT_INFORMATION, *PPROCESS_FAULT_INFORMATION;

typedef struct _PROCESS_TELEMETRY_ID_INFORMATION
{
    ULONG HeaderSize;
    ULONG ProcessId;
    ULONGLONG ProcessStartKey;
    ULONGLONG CreateTime;
    ULONGLONG CreateInterruptTime;
    ULONGLONG CreateUnbiasedInterruptTime;
    ULONGLONG ProcessSequenceNumber;
    ULONGLONG SessionCreateTime;
    ULONG SessionId;
    ULONG BootId;
    ULONG ImageChecksum;
    ULONG ImageTimeDateStamp;
    ULONG UserSidOffset;
    ULONG ImagePathOffset;
    ULONG PackageNameOffset;
    ULONG RelativeAppNameOffset;
    ULONG CommandLineOffset;
} PROCESS_TELEMETRY_ID_INFORMATION, *PPROCESS_TELEMETRY_ID_INFORMATION;

typedef struct _PROCESS_COMMIT_RELEASE_INFORMATION
{
    ULONG Version;
    struct
    {
        ULONG Eligible : 1;
        ULONG ReleaseRepurposedMemResetCommit : 1;
        ULONG ForceReleaseMemResetCommit : 1;
        ULONG Spare : 29;
    };
    SIZE_T CommitDebt;
    SIZE_T CommittedMemResetSize;
    SIZE_T RepurposedMemResetSize;
} PROCESS_COMMIT_RELEASE_INFORMATION, *PPROCESS_COMMIT_RELEASE_INFORMATION;

typedef struct _PROCESS_JOB_MEMORY_INFO
{
    ULONGLONG SharedCommitUsage;
    ULONGLONG PrivateCommitUsage;
    ULONGLONG PeakPrivateCommitUsage;
    ULONGLONG PrivateCommitLimit;
    ULONGLONG TotalCommitLimit;
} PROCESS_JOB_MEMORY_INFO, *PPROCESS_JOB_MEMORY_INFO;

typedef struct _PROCESS_CHILD_PROCESS_INFORMATION
{
    BOOLEAN ProhibitChildProcesses;
    BOOLEAN AlwaysAllowSecureChildProcess; // REDSTONE3
    BOOLEAN AuditProhibitChildProcesses;
} PROCESS_CHILD_PROCESS_INFORMATION, *PPROCESS_CHILD_PROCESS_INFORMATION;

#define POWER_THROTTLING_PROCESS_CURRENT_VERSION 1
#define POWER_THROTTLING_PROCESS_EXECUTION_SPEED 0x1
#define POWER_THROTTLING_PROCESS_DELAYTIMERS 0x2
#define POWER_THROTTLING_PROCESS_IGNORE_TIMER_RESOLUTION 0x4 // since WIN11
#define POWER_THROTTLING_PROCESS_VALID_FLAGS ((POWER_THROTTLING_PROCESS_EXECUTION_SPEED | POWER_THROTTLING_PROCESS_DELAYTIMERS | POWER_THROTTLING_PROCESS_IGNORE_TIMER_RESOLUTION))

typedef struct _POWER_THROTTLING_PROCESS_STATE
{
    ULONG Version;
    ULONG ControlMask;
    ULONG StateMask;
} POWER_THROTTLING_PROCESS_STATE, *PPOWER_THROTTLING_PROCESS_STATE;

// rev (tyranid)
#define WIN32K_SYSCALL_FILTER_STATE_ENABLE 0x1
#define WIN32K_SYSCALL_FILTER_STATE_AUDIT 0x2

// private
typedef struct _WIN32K_SYSCALL_FILTER
{
    ULONG FilterState;
    ULONG FilterSet;
} WIN32K_SYSCALL_FILTER, *PWIN32K_SYSCALL_FILTER;

typedef struct _JOBOBJECT_WAKE_FILTER *PJOBOBJECT_WAKE_FILTER; // from ntpsapi.h

typedef struct _PROCESS_WAKE_INFORMATION
{
    ULONGLONG NotificationChannel;
    ULONG WakeCounters[7];
    PJOBOBJECT_WAKE_FILTER WakeFilter;
} PROCESS_WAKE_INFORMATION, *PPROCESS_WAKE_INFORMATION;

typedef struct _PROCESS_ENERGY_TRACKING_STATE
{
    ULONG StateUpdateMask;
    ULONG StateDesiredValue;
    ULONG StateSequence;
    ULONG UpdateTag : 1;
    WCHAR Tag[64];
} PROCESS_ENERGY_TRACKING_STATE, *PPROCESS_ENERGY_TRACKING_STATE;

typedef struct _MANAGE_WRITES_TO_EXECUTABLE_MEMORY
{
    ULONG Version : 8;
    ULONG ProcessEnableWriteExceptions : 1;
    ULONG ThreadAllowWrites : 1;
    ULONG Spare : 22;
    PVOID KernelWriteToExecutableSignal; // 19H1
} MANAGE_WRITES_TO_EXECUTABLE_MEMORY, *PMANAGE_WRITES_TO_EXECUTABLE_MEMORY;

#define POWER_THROTTLING_THREAD_CURRENT_VERSION 1
#define POWER_THROTTLING_THREAD_EXECUTION_SPEED 0x1
#define POWER_THROTTLING_THREAD_VALID_FLAGS (POWER_THROTTLING_THREAD_EXECUTION_SPEED)

typedef struct _POWER_THROTTLING_THREAD_STATE
{
    ULONG Version;
    ULONG ControlMask;
    ULONG StateMask;
} POWER_THROTTLING_THREAD_STATE, *PPOWER_THROTTLING_THREAD_STATE;

#define PROCESS_READWRITEVM_LOGGING_ENABLE_READVM 1
#define PROCESS_READWRITEVM_LOGGING_ENABLE_WRITEVM 2
#define PROCESS_READWRITEVM_LOGGING_ENABLE_READVM_V 1UL
#define PROCESS_READWRITEVM_LOGGING_ENABLE_WRITEVM_V 2UL

typedef union _PROCESS_READWRITEVM_LOGGING_INFORMATION
{
    UCHAR Flags;
    struct
    {
        UCHAR EnableReadVmLogging : 1;
        UCHAR EnableWriteVmLogging : 1;
        UCHAR Unused : 6;
    };
} PROCESS_READWRITEVM_LOGGING_INFORMATION, *PPROCESS_READWRITEVM_LOGGING_INFORMATION;

typedef struct _PROCESS_UPTIME_INFORMATION
{
    ULONGLONG QueryInterruptTime;
    ULONGLONG QueryUnbiasedTime;
    ULONGLONG EndInterruptTime;
    ULONGLONG TimeSinceCreation;
    ULONGLONG Uptime;
    ULONGLONG SuspendedTime;
    union
    {
        ULONG HangCount : 4;
        ULONG GhostCount : 4;
        ULONG Crashed : 1;
        ULONG Terminated : 1;
    };
} PROCESS_UPTIME_INFORMATION, *PPROCESS_UPTIME_INFORMATION;

typedef union _PROCESS_SYSTEM_RESOURCE_MANAGEMENT
{
    ULONG Flags;
    struct
    {
        ULONG Foreground : 1;
        ULONG Reserved : 31;
    };
} PROCESS_SYSTEM_RESOURCE_MANAGEMENT, *PPROCESS_SYSTEM_RESOURCE_MANAGEMENT;

// private
typedef struct _PROCESS_SECURITY_DOMAIN_INFORMATION
{
    ULONGLONG SecurityDomain;
} PROCESS_SECURITY_DOMAIN_INFORMATION, *PPROCESS_SECURITY_DOMAIN_INFORMATION;

// private
typedef struct _PROCESS_COMBINE_SECURITY_DOMAINS_INFORMATION
{
    HANDLE ProcessHandle;
} PROCESS_COMBINE_SECURITY_DOMAINS_INFORMATION, *PPROCESS_COMBINE_SECURITY_DOMAINS_INFORMATION;

// private
typedef union _PROCESS_LOGGING_INFORMATION
{
    ULONG Flags;
    struct
    {
        ULONG EnableReadVmLogging : 1;
        ULONG EnableWriteVmLogging : 1;
        ULONG EnableProcessSuspendResumeLogging : 1;
        ULONG EnableThreadSuspendResumeLogging : 1;
        ULONG Reserved : 28;
    };
} PROCESS_LOGGING_INFORMATION, *PPROCESS_LOGGING_INFORMATION;

// private
typedef struct _PROCESS_LEAP_SECOND_INFORMATION
{
    ULONG Flags;
    ULONG Reserved;
} PROCESS_LEAP_SECOND_INFORMATION, *PPROCESS_LEAP_SECOND_INFORMATION;

// private
typedef struct _PROCESS_FIBER_SHADOW_STACK_ALLOCATION_INFORMATION
{
    ULONGLONG ReserveSize;
    ULONGLONG CommitSize;
    ULONG PreferredNode;
    ULONG Reserved;
    PVOID Ssp;
} PROCESS_FIBER_SHADOW_STACK_ALLOCATION_INFORMATION, *PPROCESS_FIBER_SHADOW_STACK_ALLOCATION_INFORMATION;

// private
typedef struct _PROCESS_FREE_FIBER_SHADOW_STACK_ALLOCATION_INFORMATION
{
    PVOID Ssp;
} PROCESS_FREE_FIBER_SHADOW_STACK_ALLOCATION_INFORMATION, *PPROCESS_FREE_FIBER_SHADOW_STACK_ALLOCATION_INFORMATION;

//// private
//typedef struct _PROCESS_DYNAMIC_ENFORCED_ADDRESS_RANGE
//{
//    ULONG_PTR BaseAddress;
//    SIZE_T Size;
//    ULONG Flags;
//} PROCESS_DYNAMIC_ENFORCED_ADDRESS_RANGE, *PPROCESS_DYNAMIC_ENFORCED_ADDRESS_RANGE;
//
//// private
//typedef struct _PROCESS_DYNAMIC_ENFORCED_ADDRESS_RANGES_INFORMATION
//{
//    USHORT NumberOfRanges;
//    USHORT Reserved;
//    ULONG Reserved2;
//    PPROCESS_DYNAMIC_ENFORCED_ADDRESS_RANGE Ranges;
//} PROCESS_DYNAMIC_ENFORCED_ADDRESS_RANGES_INFORMATION, *PPROCESS_DYNAMIC_ENFORCED_ADDRESS_RANGES_INFORMATION;

// private
typedef struct _PROCESS_MEMBERSHIP_INFORMATION
{
    ULONG ServerSiloId;
} PROCESS_MEMBERSHIP_INFORMATION, *PPROCESS_MEMBERSHIP_INFORMATION;

// end_private

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryPortInformationProcess(
    VOID
    );

#endif

// Thread information structures

typedef struct _THREAD_BASIC_INFORMATION
{
    NTSTATUS ExitStatus;
    PTEB TebBaseAddress;
    CLIENT_ID ClientId;
    KAFFINITY AffinityMask;
    KPRIORITY Priority;
    KPRIORITY BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

// private
typedef struct _THREAD_LAST_SYSCALL_INFORMATION
{
    PVOID FirstArgument;
    USHORT SystemCallNumber;
#ifdef WIN64
    USHORT Pad[0x3]; // since REDSTONE2
#else
    USHORT Pad[0x1]; // since REDSTONE2
#endif
    ULONG64 WaitTime;
} THREAD_LAST_SYSCALL_INFORMATION, *PTHREAD_LAST_SYSCALL_INFORMATION;

// private
typedef struct _THREAD_CYCLE_TIME_INFORMATION
{
    ULONGLONG AccumulatedCycles;
    ULONGLONG CurrentCycleCount;
} THREAD_CYCLE_TIME_INFORMATION, *PTHREAD_CYCLE_TIME_INFORMATION;

// private
typedef struct _THREAD_TEB_INFORMATION
{
    PVOID TebInformation; // buffer to place data in
    ULONG TebOffset; // offset in TEB to begin reading from
    ULONG BytesToRead; // number of bytes to read
} THREAD_TEB_INFORMATION, *PTHREAD_TEB_INFORMATION;

// symbols
typedef struct _COUNTER_READING
{
    HARDWARE_COUNTER_TYPE Type;
    ULONG Index;
    ULONG64 Start;
    ULONG64 Total;
} COUNTER_READING, *PCOUNTER_READING;

// symbols
typedef struct _THREAD_PERFORMANCE_DATA
{
    USHORT Size;
    USHORT Version;
    PROCESSOR_NUMBER ProcessorNumber;
    ULONG ContextSwitches;
    ULONG HwCountersCount;
    ULONG64 UpdateCount;
    ULONG64 WaitReasonBitMap;
    ULONG64 HardwareCounters;
    COUNTER_READING CycleTime;
    COUNTER_READING HwCounters[MAX_HW_COUNTERS];
} THREAD_PERFORMANCE_DATA, *PTHREAD_PERFORMANCE_DATA;

// private
typedef struct _THREAD_PROFILING_INFORMATION
{
    ULONG64 HardwareCounters;
    ULONG Flags;
    ULONG Enable;
    PTHREAD_PERFORMANCE_DATA PerformanceData;
} THREAD_PROFILING_INFORMATION, *PTHREAD_PROFILING_INFORMATION;

// private
typedef struct _RTL_UMS_CONTEXT
{
    SINGLE_LIST_ENTRY Link;
    CONTEXT Context;
    PVOID Teb;
    PVOID UserContext;
    volatile ULONG ScheduledThread : 1;
    volatile ULONG Suspended : 1;
    volatile ULONG VolatileContext : 1;
    volatile ULONG Terminated : 1;
    volatile ULONG DebugActive : 1;
    volatile ULONG RunningOnSelfThread : 1;
    volatile ULONG DenyRunningOnSelfThread : 1;
    volatile LONG Flags;
    volatile ULONG64 KernelUpdateLock : 2;
    volatile ULONG64 PrimaryClientID : 62;
    volatile ULONG64 ContextLock;
    struct _RTL_UMS_CONTEXT* PrimaryUmsContext;
    ULONG SwitchCount;
    ULONG KernelYieldCount;
    ULONG MixedYieldCount;
    ULONG YieldCount;
} RTL_UMS_CONTEXT, *PRTL_UMS_CONTEXT;

// private
typedef enum _THREAD_UMS_INFORMATION_COMMAND
{
    UmsInformationCommandInvalid,
    UmsInformationCommandAttach,
    UmsInformationCommandDetach,
    UmsInformationCommandQuery
} THREAD_UMS_INFORMATION_COMMAND;

// private
typedef struct _RTL_UMS_COMPLETION_LIST
{
    PSINGLE_LIST_ENTRY ThreadListHead;
    PVOID CompletionEvent;
    ULONG CompletionFlags;
    SINGLE_LIST_ENTRY InternalListHead;
} RTL_UMS_COMPLETION_LIST, *PRTL_UMS_COMPLETION_LIST;

// private
typedef struct _THREAD_UMS_INFORMATION
{
    THREAD_UMS_INFORMATION_COMMAND Command;
    PRTL_UMS_COMPLETION_LIST CompletionList;
    PRTL_UMS_CONTEXT UmsContext;
    union
    {
        ULONG Flags;
        struct
        {
            ULONG IsUmsSchedulerThread : 1;
            ULONG IsUmsWorkerThread : 1;
            ULONG SpareBits : 30;
        };
    };
} THREAD_UMS_INFORMATION, *PTHREAD_UMS_INFORMATION;

// private
typedef struct _THREAD_NAME_INFORMATION
{
    UNICODE_STRING ThreadName;
} THREAD_NAME_INFORMATION, *PTHREAD_NAME_INFORMATION;

// private
typedef struct _ALPC_WORK_ON_BEHALF_TICKET
{
    ULONG ThreadId;
    ULONG ThreadCreationTimeLow;
} ALPC_WORK_ON_BEHALF_TICKET, *PALPC_WORK_ON_BEHALF_TICKET;

// private
typedef struct _RTL_WORK_ON_BEHALF_TICKET_EX
{
    ALPC_WORK_ON_BEHALF_TICKET Ticket;
    union
    {
        ULONG Flags;
        struct
        {
            ULONG CurrentThread : 1;
            ULONG Reserved1 : 31;
        };
    };
    ULONG Reserved2;
} RTL_WORK_ON_BEHALF_TICKET_EX, *PRTL_WORK_ON_BEHALF_TICKET_EX;

#if (PHNT_MODE != PHNT_MODE_KERNEL)
// private
typedef enum _SUBSYSTEM_INFORMATION_TYPE
{
    SubsystemInformationTypeWin32,
    SubsystemInformationTypeWSL,
    MaxSubsystemInformationType
} SUBSYSTEM_INFORMATION_TYPE;
#endif

// private
typedef enum _THREAD_WORKLOAD_CLASS
{
    ThreadWorkloadClassDefault,
    ThreadWorkloadClassGraphics,
    MaxThreadWorkloadClass
} THREAD_WORKLOAD_CLASS;

#if defined(_ARM64_)

#define CONTEXT_ARM   0x00200000L

#define CONTEXT_ARM_CONTROL (CONTEXT_ARM | 0x1L)
#define CONTEXT_ARM_INTEGER (CONTEXT_ARM | 0x2L)
#define CONTEXT_ARM_FLOATING_POINT  (CONTEXT_ARM | 0x4L)
#define CONTEXT_ARM_DEBUG_REGISTERS (CONTEXT_ARM | 0x8L)

#define CONTEXT_ARM_FULL (CONTEXT_ARM_CONTROL | CONTEXT_ARM_INTEGER | CONTEXT_ARM_FLOATING_POINT)

#define CONTEXT_ARM_ALL (CONTEXT_ARM_CONTROL | CONTEXT_ARM_INTEGER | CONTEXT_ARM_FLOATING_POINT | CONTEXT_ARM_DEBUG_REGISTERS)

#define ARM_MAX_BREAKPOINTS     8
#define ARM_MAX_WATCHPOINTS     1

typedef struct _ARM_NT_NEON128 {
    ULONGLONG Low;
    LONGLONG High;
} ARM_NT_NEON128, *PARM_NT_NEON128;

typedef struct DECLSPEC_ALIGN(8) DECLSPEC_NOINITALL _ARM_NT_CONTEXT {

    //
    // Control flags.
    //

    DWORD ContextFlags;

    //
    // Integer registers
    //

    DWORD R0;
    DWORD R1;
    DWORD R2;
    DWORD R3;
    DWORD R4;
    DWORD R5;
    DWORD R6;
    DWORD R7;
    DWORD R8;
    DWORD R9;
    DWORD R10;
    DWORD R11;
    DWORD R12;

    //
    // Control Registers
    //

    DWORD Sp;
    DWORD Lr;
    DWORD Pc;
    DWORD Cpsr;

    //
    // Floating Point/NEON Registers
    //

    DWORD Fpscr;
    DWORD Padding;
    union {
        ARM_NT_NEON128 Q[16];
        ULONGLONG D[32];
        DWORD S[32];
    } DUMMYUNIONNAME;

    //
    // Debug registers
    //

    DWORD Bvr[ARM_MAX_BREAKPOINTS];
    DWORD Bcr[ARM_MAX_BREAKPOINTS];
    DWORD Wvr[ARM_MAX_WATCHPOINTS];
    DWORD Wcr[ARM_MAX_WATCHPOINTS];

    DWORD Padding2[2];

} ARM_NT_CONTEXT, *PARM_NT_CONTEXT;

#endif

// Processes

#if (PHNT_MODE != PHNT_MODE_KERNEL)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ParentProcess,
    _In_ BOOLEAN InheritObjectTable,
    _In_opt_ HANDLE SectionHandle,
    _In_opt_ HANDLE DebugPort,
    _In_opt_ HANDLE TokenHandle
    );

// begin_rev
#define PROCESS_CREATE_FLAGS_BREAKAWAY 0x00000001 // NtCreateProcessEx & NtCreateUserProcess
#define PROCESS_CREATE_FLAGS_NO_DEBUG_INHERIT 0x00000002 // NtCreateProcessEx & NtCreateUserProcess
#define PROCESS_CREATE_FLAGS_INHERIT_HANDLES 0x00000004 // NtCreateProcessEx & NtCreateUserProcess
#define PROCESS_CREATE_FLAGS_OVERRIDE_ADDRESS_SPACE 0x00000008 // NtCreateProcessEx only
#define PROCESS_CREATE_FLAGS_LARGE_PAGES 0x00000010 // NtCreateProcessEx only, requires SeLockMemory
#define PROCESS_CREATE_FLAGS_LARGE_PAGE_SYSTEM_DLL 0x00000020 // NtCreateProcessEx only, requires SeLockMemory
#define PROCESS_CREATE_FLAGS_PROTECTED_PROCESS 0x00000040 // NtCreateUserProcess only
#define PROCESS_CREATE_FLAGS_CREATE_SESSION 0x00000080 // NtCreateProcessEx & NtCreateUserProcess, requires SeLoadDriver
#define PROCESS_CREATE_FLAGS_INHERIT_FROM_PARENT 0x00000100 // NtCreateProcessEx & NtCreateUserProcess
#define PROCESS_CREATE_FLAGS_SUSPENDED 0x00000200 // NtCreateProcessEx & NtCreateUserProcess
#define PROCESS_CREATE_FLAGS_FORCE_BREAKAWAY 0x00000400 // NtCreateProcessEx & NtCreateUserProcess, requires SeTcb
#define PROCESS_CREATE_FLAGS_MINIMAL_PROCESS 0x00000800 // NtCreateProcessEx only
#define PROCESS_CREATE_FLAGS_RELEASE_SECTION 0x00001000 // NtCreateProcessEx & NtCreateUserProcess
#define PROCESS_CREATE_FLAGS_CLONE_MINIMAL 0x00002000 // NtCreateProcessEx only
#define PROCESS_CREATE_FLAGS_CLONE_MINIMAL_REDUCED_COMMIT 0x00004000 //
#define PROCESS_CREATE_FLAGS_AUXILIARY_PROCESS 0x00008000 // NtCreateProcessEx & NtCreateUserProcess, requires SeTcb
#define PROCESS_CREATE_FLAGS_CREATE_STORE 0x00020000 // NtCreateProcessEx & NtCreateUserProcess
#define PROCESS_CREATE_FLAGS_USE_PROTECTED_ENVIRONMENT 0x00040000 // NtCreateProcessEx & NtCreateUserProcess
// end_rev

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateProcessEx(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ParentProcess,
    _In_ ULONG Flags, // PROCESS_CREATE_FLAGS_*
    _In_opt_ HANDLE SectionHandle,
    _In_opt_ HANDLE DebugPort,
    _In_opt_ HANDLE TokenHandle,
    _Reserved_ ULONG Reserved // JobMemberLevel
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PCLIENT_ID ClientId
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTerminateProcess(
    _In_opt_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSuspendProcess(
    _In_ HANDLE ProcessHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtResumeProcess(
    _In_ HANDLE ProcessHandle
    );

#define NtCurrentProcess() ((HANDLE)(LONG_PTR)-1)
#define ZwCurrentProcess() NtCurrentProcess()
#define NtCurrentThread() ((HANDLE)(LONG_PTR)-2)
#define ZwCurrentThread() NtCurrentThread()
#define NtCurrentSession() ((HANDLE)(LONG_PTR)-3)
#define ZwCurrentSession() NtCurrentSession()
#define NtCurrentPeb() (NtCurrentTeb()->ProcessEnvironmentBlock)

// Windows 8 and above
#define NtCurrentProcessToken() ((HANDLE)(LONG_PTR)-4) // NtOpenProcessToken(NtCurrentProcess())
#define NtCurrentThreadToken() ((HANDLE)(LONG_PTR)-5) // NtOpenThreadToken(NtCurrentThread())
#define NtCurrentThreadEffectiveToken() ((HANDLE)(LONG_PTR)-6) // NtOpenThreadToken(NtCurrentThread()) + NtOpenProcessToken(NtCurrentProcess())

#define NtCurrentSilo() ((HANDLE)(LONG_PTR)-1)

// Not NT, but useful.
#define NtCurrentProcessId() (NtCurrentTeb()->ClientId.UniqueProcess)
#define NtCurrentThreadId() (NtCurrentTeb()->ClientId.UniqueThread)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_writes_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

#if (PHNT_VERSION >= PHNT_WS03)

#define PROCESS_GET_NEXT_FLAGS_PREVIOUS_PROCESS 0x00000001

NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetNextProcess(
    _In_opt_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG HandleAttributes,
    _In_ ULONG Flags,
    _Out_ PHANDLE NewProcessHandle
    );
#endif

#if (PHNT_VERSION >= PHNT_WS03)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetNextThread(
    _In_ HANDLE ProcessHandle,
    _In_opt_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG HandleAttributes,
    _In_ ULONG Flags,
    _Out_ PHANDLE NewThreadHandle
    );
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _In_reads_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength
    );

#endif

#define STATECHANGE_SET_ATTRIBUTES 0x0001

typedef enum _PROCESS_STATE_CHANGE_TYPE
{
    ProcessStateChangeSuspend,
    ProcessStateChangeResume,
    ProcessStateChangeMax,
} PROCESS_STATE_CHANGE_TYPE, *PPROCESS_STATE_CHANGE_TYPE;

#if (PHNT_VERSION >= PHNT_WIN11)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateProcessStateChange(
    _Out_ PHANDLE ProcessStateChangeHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ProcessHandle,
    _In_opt_ ULONG64 Reserved
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtChangeProcessState(
    _In_ HANDLE ProcessStateChangeHandle,
    _In_ HANDLE ProcessHandle,
    _In_ PROCESS_STATE_CHANGE_TYPE StateChangeType,
    _In_opt_ PVOID ExtendedInformation,
    _In_opt_ SIZE_T ExtendedInformationLength,
    _In_opt_ ULONG64 Reserved
    );

#endif

typedef enum _THREAD_STATE_CHANGE_TYPE
{
    ThreadStateChangeSuspend,
    ThreadStateChangeResume,
    ThreadStateChangeMax,
} THREAD_STATE_CHANGE_TYPE, *PTHREAD_STATE_CHANGE_TYPE;

#if (PHNT_VERSION >= PHNT_WIN11)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateThreadStateChange(
    _Out_ PHANDLE ThreadStateChangeHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ThreadHandle,
    _In_opt_ ULONG64 Reserved
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtChangeThreadState(
    _In_ HANDLE ThreadStateChangeHandle,
    _In_ HANDLE ThreadHandle,
    _In_ THREAD_STATE_CHANGE_TYPE StateChangeType,
    _In_opt_ PVOID ExtendedInformation,
    _In_opt_ SIZE_T ExtendedInformationLength,
    _In_opt_ ULONG64 Reserved
    );

#endif

// Threads

#if (PHNT_MODE != PHNT_MODE_KERNEL)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ProcessHandle,
    _Out_ PCLIENT_ID ClientId,
    _In_ PCONTEXT ThreadContext,
    _In_ PINITIAL_TEB InitialTeb,
    _In_ BOOLEAN CreateSuspended
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PCLIENT_ID ClientId
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTerminateThread(
    _In_opt_ HANDLE ThreadHandle,
    _In_ NTSTATUS ExitStatus
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSuspendThread(
    _In_ HANDLE ThreadHandle,
    _Out_opt_ PULONG PreviousSuspendCount
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtResumeThread(
    _In_ HANDLE ThreadHandle,
    _Out_opt_ PULONG PreviousSuspendCount
    );

NTSYSCALLAPI
ULONG
NTAPI
NtGetCurrentProcessorNumber(
    VOID
    );

#if (PHNT_VERSION >= PHNT_WIN7)
NTSYSCALLAPI
ULONG
NTAPI
NtGetCurrentProcessorNumberEx(
    _Out_opt_ PPROCESSOR_NUMBER ProcessorNumber
    );
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetContextThread(
    _In_ HANDLE ThreadHandle,
    _Inout_ PCONTEXT ThreadContext
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetContextThread(
    _In_ HANDLE ThreadHandle,
    _In_ PCONTEXT ThreadContext
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ THREADINFOCLASS ThreadInformationClass,
    _Out_writes_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ THREADINFOCLASS ThreadInformationClass,
    _In_reads_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlertThread(
    _In_ HANDLE ThreadHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlertResumeThread(
    _In_ HANDLE ThreadHandle,
    _Out_opt_ PULONG PreviousSuspendCount
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTestAlert(
    VOID
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtImpersonateThread(
    _In_ HANDLE ServerThreadHandle,
    _In_ HANDLE ClientThreadHandle,
    _In_ PSECURITY_QUALITY_OF_SERVICE SecurityQos
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRegisterThreadTerminatePort(
    _In_ HANDLE PortHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetLdtEntries(
    _In_ ULONG Selector0,
    _In_ ULONG Entry0Low,
    _In_ ULONG Entry0Hi,
    _In_ ULONG Selector1,
    _In_ ULONG Entry1Low,
    _In_ ULONG Entry1Hi
    );

typedef VOID (NTAPI* PPS_APC_ROUTINE)(
    _In_opt_ PVOID ApcArgument1,
    _In_opt_ PVOID ApcArgument2,
    _In_opt_ PVOID ApcArgument3
    );

#define Wow64EncodeApcRoutine(ApcRoutine) \
    ((PVOID)((0 - ((LONG_PTR)(ApcRoutine))) << 2))

#define Wow64DecodeApcRoutine(ApcRoutine) \
    ((PVOID)(0 - (((LONG_PTR)(ApcRoutine)) >> 2)))

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueueApcThread(
    _In_ HANDLE ThreadHandle,
    _In_ PPS_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcArgument1,
    _In_opt_ PVOID ApcArgument2,
    _In_opt_ PVOID ApcArgument3
    );

#if (PHNT_VERSION >= PHNT_WIN7)

#define APC_FORCE_THREAD_SIGNAL ((HANDLE)1) // ReserveHandle

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueueApcThreadEx(
    _In_ HANDLE ThreadHandle,
    _In_opt_ HANDLE ReserveHandle, // NtAllocateReserveObject
    _In_ PPS_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcArgument1,
    _In_opt_ PVOID ApcArgument2,
    _In_opt_ PVOID ApcArgument3
    );
#endif

#if (PHNT_VERSION >= PHNT_WIN11)

// QUEUE_USER_APC_FLAGS enum (dmex)
#define QUEUE_USER_APC_FLAGS_NONE               0x0
#define QUEUE_USER_APC_FLAGS_SPECIAL_USER_APC   0x1
#define QUEUE_USER_APC_CALLBACK_DATA_CONTEXT    0x00010000 // APC_CALLBACK_DATA

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueueApcThreadEx2(
    _In_ HANDLE ThreadHandle,
    _In_opt_ HANDLE ReserveHandle, // NtAllocateReserveObject
    _In_ ULONG ApcFlags, // QUEUE_USER_APC_FLAGS
    _In_ PPS_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcArgument1,
    _In_opt_ PVOID ApcArgument2,
    _In_opt_ PVOID ApcArgument3
    );

#endif

#if (PHNT_VERSION >= PHNT_WIN8)

// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlertThreadByThreadId(
    _In_ HANDLE ThreadId
    );

// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForAlertByThreadId(
    _In_ PVOID Address,
    _In_opt_ PLARGE_INTEGER Timeout
    );

#endif

#endif

// User processes and threads

#if (PHNT_MODE != PHNT_MODE_KERNEL)

// Attributes (Win32 CreateProcess)

// PROC_THREAD_ATTRIBUTE_NUM (dmex)
#define ProcThreadAttributeParentProcess 0 // in HANDLE
#define ProcThreadAttributeExtendedFlags 1 // in ULONG (EXTENDED_PROCESS_CREATION_FLAG_*)
#define ProcThreadAttributeHandleList 2 // in HANDLE[]
#define ProcThreadAttributeGroupAffinity 3 // in GROUP_AFFINITY // since WIN7
#define ProcThreadAttributePreferredNode 4 // in USHORT
#define ProcThreadAttributeIdealProcessor 5 // in PROCESSOR_NUMBER
#define ProcThreadAttributeUmsThread 6 // in UMS_CREATE_THREAD_ATTRIBUTES
#define ProcThreadAttributeMitigationPolicy 7 // in ULONG, ULONG64, or ULONG64[2]
#define ProcThreadAttributePackageFullName 8 // in WCHAR[] // since WIN8
#define ProcThreadAttributeSecurityCapabilities 9 // in SECURITY_CAPABILITIES
#define ProcThreadAttributeConsoleReference 10 // BaseGetConsoleReference (kernelbase.dll)
#define ProcThreadAttributeProtectionLevel 11 // in ULONG (PROTECTION_LEVEL_*) // since WINBLUE
#define ProcThreadAttributeOsMaxVersionTested 12 // in MAXVERSIONTESTED_INFO // since THRESHOLD // (from exe.manifest)
#define ProcThreadAttributeJobList 13 // in HANDLE[]
#define ProcThreadAttributeChildProcessPolicy 14 // in ULONG (PROCESS_CREATION_CHILD_PROCESS_*) // since THRESHOLD2
#define ProcThreadAttributeAllApplicationPackagesPolicy 15 // in ULONG (PROCESS_CREATION_ALL_APPLICATION_PACKAGES_*) // since REDSTONE
#define ProcThreadAttributeWin32kFilter 16 // in WIN32K_SYSCALL_FILTER
#define ProcThreadAttributeSafeOpenPromptOriginClaim 17 // in SE_SAFE_OPEN_PROMPT_RESULTS
#define ProcThreadAttributeDesktopAppPolicy 18 // in ULONG (PROCESS_CREATION_DESKTOP_APP_*) // since RS2
#define ProcThreadAttributeBnoIsolation 19 // in PROC_THREAD_BNOISOLATION_ATTRIBUTE
#define ProcThreadAttributePseudoConsole 22 // in HANDLE (HPCON) // since RS5
#define ProcThreadAttributeIsolationManifest 23 // in ISOLATION_MANIFEST_PROPERTIES // rev (diversenok) // since 19H2+
#define ProcThreadAttributeMitigationAuditPolicy 24 // in ULONG, ULONG64, or ULONG64[2] // since 21H1
#define ProcThreadAttributeMachineType 25 // in USHORT // since 21H2
#define ProcThreadAttributeComponentFilter 26 // in ULONG
#define ProcThreadAttributeEnableOptionalXStateFeatures 27 // in ULONG64 // since WIN11
#define ProcThreadAttributeCreateStore 28 // ULONG // rev (diversenok)
#define ProcThreadAttributeTrustedApp 29

#ifndef PROC_THREAD_ATTRIBUTE_EXTENDED_FLAGS
#define PROC_THREAD_ATTRIBUTE_EXTENDED_FLAGS \
    ProcThreadAttributeValue(ProcThreadAttributeExtendedFlags, FALSE, TRUE, TRUE)
#endif
#ifndef PROC_THREAD_ATTRIBUTE_PACKAGE_FULL_NAME
#define PROC_THREAD_ATTRIBUTE_PACKAGE_FULL_NAME \
    ProcThreadAttributeValue(ProcThreadAttributePackageFullName, FALSE, TRUE, FALSE)
#endif
#ifndef PROC_THREAD_ATTRIBUTE_CONSOLE_REFERENCE
#define PROC_THREAD_ATTRIBUTE_CONSOLE_REFERENCE \
    ProcThreadAttributeValue(ProcThreadAttributeConsoleReference, FALSE, TRUE, FALSE)
#endif
#ifndef PROC_THREAD_ATTRIBUTE_OSMAXVERSIONTESTED
#define PROC_THREAD_ATTRIBUTE_OSMAXVERSIONTESTED \
    ProcThreadAttributeValue(ProcThreadAttributeOsMaxVersionTested, FALSE, TRUE, FALSE)
#endif
#ifndef PROC_THREAD_ATTRIBUTE_SAFE_OPEN_PROMPT_ORIGIN_CLAIM
#define PROC_THREAD_ATTRIBUTE_SAFE_OPEN_PROMPT_ORIGIN_CLAIM \
    ProcThreadAttributeValue(ProcThreadAttributeSafeOpenPromptOriginClaim, FALSE, TRUE, FALSE)
#endif
#ifndef PROC_THREAD_ATTRIBUTE_BNO_ISOLATION
#define PROC_THREAD_ATTRIBUTE_BNO_ISOLATION \
    ProcThreadAttributeValue(ProcThreadAttributeBnoIsolation, FALSE, TRUE, FALSE)
#endif
#ifndef PROC_THREAD_ATTRIBUTE_ISOLATION_MANIFEST
#define PROC_THREAD_ATTRIBUTE_ISOLATION_MANIFEST \
    ProcThreadAttributeValue(ProcThreadAttributeIsolationManifest, FALSE, TRUE, FALSE)
#endif
#ifndef PROC_THREAD_ATTRIBUTE_CREATE_STORE
#define PROC_THREAD_ATTRIBUTE_CREATE_STORE \
    ProcThreadAttributeValue(ProcThreadAttributeCreateStore, FALSE, TRUE, FALSE)
#endif

// private
typedef struct _PROC_THREAD_ATTRIBUTE
{
    ULONG_PTR Attribute;
    SIZE_T Size;
    ULONG_PTR Value;
} PROC_THREAD_ATTRIBUTE, *PPROC_THREAD_ATTRIBUTE;

// private
typedef struct _PROC_THREAD_ATTRIBUTE_LIST
{
    ULONG PresentFlags;
    ULONG AttributeCount;
    ULONG LastAttribute;
    ULONG SpareUlong0;
    PPROC_THREAD_ATTRIBUTE ExtendedFlagsAttribute;
    _Field_size_(AttributeCount) PROC_THREAD_ATTRIBUTE Attributes[1];
} PROC_THREAD_ATTRIBUTE_LIST, *PPROC_THREAD_ATTRIBUTE_LIST;

// private
#define EXTENDED_PROCESS_CREATION_FLAG_ELEVATION_HANDLED 0x00000001
#define EXTENDED_PROCESS_CREATION_FLAG_FORCELUA 0x00000002
#define EXTENDED_PROCESS_CREATION_FLAG_FORCE_BREAKAWAY 0x00000004 // requires SeTcbPrivilege // since WINBLUE

// private
#define PROTECTION_LEVEL_WINTCB_LIGHT 0x00000000
#define PROTECTION_LEVEL_WINDOWS 0x00000001
#define PROTECTION_LEVEL_WINDOWS_LIGHT 0x00000002
#define PROTECTION_LEVEL_ANTIMALWARE_LIGHT 0x00000003
#define PROTECTION_LEVEL_LSA_LIGHT 0x00000004
#define PROTECTION_LEVEL_WINTCB 0x00000005
#define PROTECTION_LEVEL_CODEGEN_LIGHT 0x00000006
#define PROTECTION_LEVEL_AUTHENTICODE 0x00000007

// private
typedef enum _SE_SAFE_OPEN_PROMPT_EXPERIENCE_RESULTS
{
    SeSafeOpenExperienceNone = 0x00,
    SeSafeOpenExperienceCalled = 0x01,
    SeSafeOpenExperienceAppRepCalled = 0x02,
    SeSafeOpenExperiencePromptDisplayed = 0x04,
    SeSafeOpenExperienceUAC = 0x08,
    SeSafeOpenExperienceUninstaller = 0x10,
    SeSafeOpenExperienceIgnoreUnknownOrBad = 0x20,
    SeSafeOpenExperienceDefenderTrustedInstaller = 0x40,
    SeSafeOpenExperienceMOTWPresent = 0x80
} SE_SAFE_OPEN_PROMPT_EXPERIENCE_RESULTS;

// private
typedef struct _SE_SAFE_OPEN_PROMPT_RESULTS
{
    SE_SAFE_OPEN_PROMPT_EXPERIENCE_RESULTS Results;
    WCHAR Path[MAX_PATH];
} SE_SAFE_OPEN_PROMPT_RESULTS, *PSE_SAFE_OPEN_PROMPT_RESULTS;

typedef struct _PROC_THREAD_BNOISOLATION_ATTRIBUTE
{
    BOOL IsolationEnabled;
    WCHAR IsolationPrefix[0x88];
} PROC_THREAD_BNOISOLATION_ATTRIBUTE, *PPROC_THREAD_BNOISOLATION_ATTRIBUTE;

// private
typedef struct _ISOLATION_MANIFEST_PROPERTIES
{
    UNICODE_STRING InstancePath;
    UNICODE_STRING FriendlyName;
    UNICODE_STRING Description;
    ULONG_PTR Level;
} ISOLATION_MANIFEST_PROPERTIES, *PISOLATION_MANIFEST_PROPERTIES;

#ifndef PROC_THREAD_ATTRIBUTE_EXTENDED_FLAGS
#define PROC_THREAD_ATTRIBUTE_EXTENDED_FLAGS \
    ProcThreadAttributeValue(ProcThreadAttributeExtendedFlags, FALSE, TRUE, TRUE)
#endif
#ifndef PROC_THREAD_ATTRIBUTE_CONSOLE_REFERENCE
#define PROC_THREAD_ATTRIBUTE_CONSOLE_REFERENCE \
    ProcThreadAttributeValue(ProcThreadAttributeConsoleReference, FALSE, TRUE, FALSE)
#endif
#ifndef PROC_THREAD_ATTRIBUTE_OSMAXVERSIONTESTED
#define PROC_THREAD_ATTRIBUTE_OSMAXVERSIONTESTED \
    ProcThreadAttributeValue(ProcThreadAttributeOsMaxVersionTested, FALSE, TRUE, FALSE)
#endif
#ifndef PROC_THREAD_ATTRIBUTE_SAFE_OPEN_PROMPT_ORIGIN_CLAIM
#define PROC_THREAD_ATTRIBUTE_SAFE_OPEN_PROMPT_ORIGIN_CLAIM \
    ProcThreadAttributeValue(ProcThreadAttributeSafeOpenPromptOriginClaim, FALSE, TRUE, FALSE)
#endif
#ifndef PROC_THREAD_ATTRIBUTE_BNO_ISOLATION
#define PROC_THREAD_ATTRIBUTE_BNO_ISOLATION \
    ProcThreadAttributeValue(ProcThreadAttributeBnoIsolation, FALSE, TRUE, FALSE)
#endif

// Attributes (Native)

// private
typedef enum _PS_ATTRIBUTE_NUM
{
    PsAttributeParentProcess, // in HANDLE
    PsAttributeDebugObject, // in HANDLE
    PsAttributeToken, // in HANDLE
    PsAttributeClientId, // out PCLIENT_ID
    PsAttributeTebAddress, // out PTEB *
    PsAttributeImageName, // in PWSTR
    PsAttributeImageInfo, // out PSECTION_IMAGE_INFORMATION
    PsAttributeMemoryReserve, // in PPS_MEMORY_RESERVE
    PsAttributePriorityClass, // in UCHAR
    PsAttributeErrorMode, // in ULONG
    PsAttributeStdHandleInfo, // 10, in PPS_STD_HANDLE_INFO
    PsAttributeHandleList, // in HANDLE[]
    PsAttributeGroupAffinity, // in PGROUP_AFFINITY
    PsAttributePreferredNode, // in PUSHORT
    PsAttributeIdealProcessor, // in PPROCESSOR_NUMBER
    PsAttributeUmsThread, // ? in PUMS_CREATE_THREAD_ATTRIBUTES
    PsAttributeMitigationOptions, // in PPS_MITIGATION_OPTIONS_MAP (PROCESS_CREATION_MITIGATION_POLICY_*) // since WIN8
    PsAttributeProtectionLevel, // in PS_PROTECTION // since WINBLUE
    PsAttributeSecureProcess, // in PPS_TRUSTLET_CREATE_ATTRIBUTES, since THRESHOLD
    PsAttributeJobList, // in HANDLE[]
    PsAttributeChildProcessPolicy, // 20, in PULONG (PROCESS_CREATION_CHILD_PROCESS_*) // since THRESHOLD2
    PsAttributeAllApplicationPackagesPolicy, // in PULONG (PROCESS_CREATION_ALL_APPLICATION_PACKAGES_*) // since REDSTONE
    PsAttributeWin32kFilter, // in PWIN32K_SYSCALL_FILTER
    PsAttributeSafeOpenPromptOriginClaim, // in
    PsAttributeBnoIsolation, // in PPS_BNO_ISOLATION_PARAMETERS // since REDSTONE2
    PsAttributeDesktopAppPolicy, // in PULONG (PROCESS_CREATION_DESKTOP_APP_*)
    PsAttributeChpe, // in BOOLEAN // since REDSTONE3
    PsAttributeMitigationAuditOptions, // in PPS_MITIGATION_AUDIT_OPTIONS_MAP (PROCESS_CREATION_MITIGATION_AUDIT_POLICY_*) // since 21H1
    PsAttributeMachineType, // in WORD // since 21H2
    PsAttributeComponentFilter,
    PsAttributeEnableOptionalXStateFeatures, // since WIN11
    PsAttributeMax
} PS_ATTRIBUTE_NUM;

// private
#define PS_ATTRIBUTE_NUMBER_MASK 0x0000ffff
#define PS_ATTRIBUTE_THREAD 0x00010000 // may be used with thread creation
#define PS_ATTRIBUTE_INPUT 0x00020000 // input only
#define PS_ATTRIBUTE_ADDITIVE 0x00040000 // "accumulated" e.g. bitmasks, counters, etc.

// begin_rev

#define PsAttributeValue(Number, Thread, Input, Additive) \
    (((Number) & PS_ATTRIBUTE_NUMBER_MASK) | \
    ((Thread) ? PS_ATTRIBUTE_THREAD : 0) | \
    ((Input) ? PS_ATTRIBUTE_INPUT : 0) | \
    ((Additive) ? PS_ATTRIBUTE_ADDITIVE : 0))

#define PS_ATTRIBUTE_PARENT_PROCESS \
    PsAttributeValue(PsAttributeParentProcess, FALSE, TRUE, TRUE)
#define PS_ATTRIBUTE_DEBUG_OBJECT \
    PsAttributeValue(PsAttributeDebugObject, FALSE, TRUE, TRUE)
#define PS_ATTRIBUTE_TOKEN \
    PsAttributeValue(PsAttributeToken, FALSE, TRUE, TRUE)
#define PS_ATTRIBUTE_CLIENT_ID \
    PsAttributeValue(PsAttributeClientId, TRUE, FALSE, FALSE)
#define PS_ATTRIBUTE_TEB_ADDRESS \
    PsAttributeValue(PsAttributeTebAddress, TRUE, FALSE, FALSE)
#define PS_ATTRIBUTE_IMAGE_NAME \
    PsAttributeValue(PsAttributeImageName, FALSE, TRUE, FALSE)
#define PS_ATTRIBUTE_IMAGE_INFO \
    PsAttributeValue(PsAttributeImageInfo, FALSE, FALSE, FALSE)
#define PS_ATTRIBUTE_MEMORY_RESERVE \
    PsAttributeValue(PsAttributeMemoryReserve, FALSE, TRUE, FALSE)
#define PS_ATTRIBUTE_PRIORITY_CLASS \
    PsAttributeValue(PsAttributePriorityClass, FALSE, TRUE, FALSE)
#define PS_ATTRIBUTE_ERROR_MODE \
    PsAttributeValue(PsAttributeErrorMode, FALSE, TRUE, FALSE)
#define PS_ATTRIBUTE_STD_HANDLE_INFO \
    PsAttributeValue(PsAttributeStdHandleInfo, FALSE, TRUE, FALSE)
#define PS_ATTRIBUTE_HANDLE_LIST \
    PsAttributeValue(PsAttributeHandleList, FALSE, TRUE, FALSE)
#define PS_ATTRIBUTE_GROUP_AFFINITY \
    PsAttributeValue(PsAttributeGroupAffinity, TRUE, TRUE, FALSE)
#define PS_ATTRIBUTE_PREFERRED_NODE \
    PsAttributeValue(PsAttributePreferredNode, FALSE, TRUE, FALSE)
#define PS_ATTRIBUTE_IDEAL_PROCESSOR \
    PsAttributeValue(PsAttributeIdealProcessor, TRUE, TRUE, FALSE)
#define PS_ATTRIBUTE_UMS_THREAD \
    PsAttributeValue(PsAttributeUmsThread, TRUE, TRUE, FALSE)
#define PS_ATTRIBUTE_MITIGATION_OPTIONS \
    PsAttributeValue(PsAttributeMitigationOptions, FALSE, TRUE, FALSE)
#define PS_ATTRIBUTE_PROTECTION_LEVEL \
    PsAttributeValue(PsAttributeProtectionLevel, FALSE, TRUE, TRUE)
#define PS_ATTRIBUTE_SECURE_PROCESS \
    PsAttributeValue(PsAttributeSecureProcess, FALSE, TRUE, FALSE)
#define PS_ATTRIBUTE_JOB_LIST \
    PsAttributeValue(PsAttributeJobList, FALSE, TRUE, FALSE)
#define PS_ATTRIBUTE_CHILD_PROCESS_POLICY \
    PsAttributeValue(PsAttributeChildProcessPolicy, FALSE, TRUE, FALSE)
#define PS_ATTRIBUTE_ALL_APPLICATION_PACKAGES_POLICY \
    PsAttributeValue(PsAttributeAllApplicationPackagesPolicy, FALSE, TRUE, FALSE)
#define PS_ATTRIBUTE_WIN32K_FILTER \
    PsAttributeValue(PsAttributeWin32kFilter, FALSE, TRUE, FALSE)
#define PS_ATTRIBUTE_SAFE_OPEN_PROMPT_ORIGIN_CLAIM \
    PsAttributeValue(PsAttributeSafeOpenPromptOriginClaim, FALSE, TRUE, FALSE)
#define PS_ATTRIBUTE_BNO_ISOLATION \
    PsAttributeValue(PsAttributeBnoIsolation, FALSE, TRUE, FALSE)
#define PS_ATTRIBUTE_DESKTOP_APP_POLICY \
    PsAttributeValue(PsAttributeDesktopAppPolicy, FALSE, TRUE, FALSE)
#define PS_ATTRIBUTE_CHPE \
    PsAttributeValue(PsAttributeChpe, FALSE, TRUE, TRUE)
#define PS_ATTRIBUTE_MITIGATION_AUDIT_OPTIONS \
    PsAttributeValue(PsAttributeMitigationAuditOptions, FALSE, TRUE, FALSE)
#define PS_ATTRIBUTE_MACHINE_TYPE \
    PsAttributeValue(PsAttributeMachineType, FALSE, TRUE, TRUE)
#define PS_ATTRIBUTE_COMPONENT_FILTER \
    PsAttributeValue(PsAttributeComponentFilter, FALSE, TRUE, FALSE)
#define PS_ATTRIBUTE_ENABLE_OPTIONAL_XSTATE_FEATURES \
    PsAttributeValue(PsAttributeEnableOptionalXStateFeatures, TRUE, TRUE, FALSE)

// end_rev

// begin_private

typedef struct _PS_ATTRIBUTE
{
    ULONG_PTR Attribute;
    SIZE_T Size;
    union
    {
        ULONG_PTR Value;
        PVOID ValuePtr;
    };
    PSIZE_T ReturnLength;
} PS_ATTRIBUTE, *PPS_ATTRIBUTE;

typedef struct _PS_ATTRIBUTE_LIST
{
    SIZE_T TotalLength;
    PS_ATTRIBUTE Attributes[1];
} PS_ATTRIBUTE_LIST, *PPS_ATTRIBUTE_LIST;

typedef struct _PS_MEMORY_RESERVE
{
    PVOID ReserveAddress;
    SIZE_T ReserveSize;
} PS_MEMORY_RESERVE, *PPS_MEMORY_RESERVE;

typedef enum _PS_STD_HANDLE_STATE
{
    PsNeverDuplicate,
    PsRequestDuplicate, // duplicate standard handles specified by PseudoHandleMask, and only if StdHandleSubsystemType matches the image subsystem
    PsAlwaysDuplicate, // always duplicate standard handles
    PsMaxStdHandleStates
} PS_STD_HANDLE_STATE;

// begin_rev
#define PS_STD_INPUT_HANDLE 0x1
#define PS_STD_OUTPUT_HANDLE 0x2
#define PS_STD_ERROR_HANDLE 0x4
// end_rev

typedef struct _PS_STD_HANDLE_INFO
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG StdHandleState : 2; // PS_STD_HANDLE_STATE
            ULONG PseudoHandleMask : 3; // PS_STD_*
        };
    };
    ULONG StdHandleSubsystemType;
} PS_STD_HANDLE_INFO, *PPS_STD_HANDLE_INFO;

typedef union _PS_TRUSTLET_ATTRIBUTE_ACCESSRIGHTS
{
    UCHAR Trustlet : 1;
    UCHAR Ntos : 1;
    UCHAR WriteHandle : 1;
    UCHAR ReadHandle : 1;
    UCHAR Reserved : 4;
    UCHAR AccessRights;
} PS_TRUSTLET_ATTRIBUTE_ACCESSRIGHTS, *PPS_TRUSTLET_ATTRIBUTE_ACCESSRIGHTS;

typedef struct _PS_TRUSTLET_ATTRIBUTE_TYPE
{
    union
    {
        struct
        {
            UCHAR Version;
            UCHAR DataCount;
            UCHAR SemanticType;
            PS_TRUSTLET_ATTRIBUTE_ACCESSRIGHTS AccessRights;
        };
        ULONG AttributeType;
    };
} PS_TRUSTLET_ATTRIBUTE_TYPE, *PPS_TRUSTLET_ATTRIBUTE_TYPE;

typedef struct _PS_TRUSTLET_ATTRIBUTE_HEADER
{
    PS_TRUSTLET_ATTRIBUTE_TYPE AttributeType;
    ULONG InstanceNumber : 8;
    ULONG Reserved : 24;
} PS_TRUSTLET_ATTRIBUTE_HEADER, *PPS_TRUSTLET_ATTRIBUTE_HEADER;

typedef struct _PS_TRUSTLET_ATTRIBUTE_DATA
{
    PS_TRUSTLET_ATTRIBUTE_HEADER Header;
    ULONGLONG Data[1];
} PS_TRUSTLET_ATTRIBUTE_DATA, *PPS_TRUSTLET_ATTRIBUTE_DATA;

typedef struct _PS_TRUSTLET_CREATE_ATTRIBUTES
{
    ULONGLONG TrustletIdentity;
    PS_TRUSTLET_ATTRIBUTE_DATA Attributes[1];
} PS_TRUSTLET_CREATE_ATTRIBUTES, *PPS_TRUSTLET_CREATE_ATTRIBUTES;

// private
typedef struct _PS_BNO_ISOLATION_PARAMETERS
{
    UNICODE_STRING IsolationPrefix;
    ULONG HandleCount;
    PVOID *Handles;
    BOOLEAN IsolationEnabled;
} PS_BNO_ISOLATION_PARAMETERS, *PPS_BNO_ISOLATION_PARAMETERS;

// private
typedef enum _PS_MITIGATION_OPTION
{
    PS_MITIGATION_OPTION_NX,
    PS_MITIGATION_OPTION_SEHOP,
    PS_MITIGATION_OPTION_FORCE_RELOCATE_IMAGES,
    PS_MITIGATION_OPTION_HEAP_TERMINATE,
    PS_MITIGATION_OPTION_BOTTOM_UP_ASLR,
    PS_MITIGATION_OPTION_HIGH_ENTROPY_ASLR,
    PS_MITIGATION_OPTION_STRICT_HANDLE_CHECKS,
    PS_MITIGATION_OPTION_WIN32K_SYSTEM_CALL_DISABLE,
    PS_MITIGATION_OPTION_EXTENSION_POINT_DISABLE,
    PS_MITIGATION_OPTION_PROHIBIT_DYNAMIC_CODE,
    PS_MITIGATION_OPTION_CONTROL_FLOW_GUARD,
    PS_MITIGATION_OPTION_BLOCK_NON_MICROSOFT_BINARIES,
    PS_MITIGATION_OPTION_FONT_DISABLE,
    PS_MITIGATION_OPTION_IMAGE_LOAD_NO_REMOTE,
    PS_MITIGATION_OPTION_IMAGE_LOAD_NO_LOW_LABEL,
    PS_MITIGATION_OPTION_IMAGE_LOAD_PREFER_SYSTEM32,
    PS_MITIGATION_OPTION_RETURN_FLOW_GUARD,
    PS_MITIGATION_OPTION_LOADER_INTEGRITY_CONTINUITY,
    PS_MITIGATION_OPTION_STRICT_CONTROL_FLOW_GUARD,
    PS_MITIGATION_OPTION_RESTRICT_SET_THREAD_CONTEXT,
    PS_MITIGATION_OPTION_ROP_STACKPIVOT, // since REDSTONE3
    PS_MITIGATION_OPTION_ROP_CALLER_CHECK,
    PS_MITIGATION_OPTION_ROP_SIMEXEC,
    PS_MITIGATION_OPTION_EXPORT_ADDRESS_FILTER,
    PS_MITIGATION_OPTION_EXPORT_ADDRESS_FILTER_PLUS,
    PS_MITIGATION_OPTION_RESTRICT_CHILD_PROCESS_CREATION,
    PS_MITIGATION_OPTION_IMPORT_ADDRESS_FILTER,
    PS_MITIGATION_OPTION_MODULE_TAMPERING_PROTECTION,
    PS_MITIGATION_OPTION_RESTRICT_INDIRECT_BRANCH_PREDICTION,
    PS_MITIGATION_OPTION_SPECULATIVE_STORE_BYPASS_DISABLE, // since REDSTONE5
    PS_MITIGATION_OPTION_ALLOW_DOWNGRADE_DYNAMIC_CODE_POLICY,
    PS_MITIGATION_OPTION_CET_USER_SHADOW_STACKS,
    PS_MITIGATION_OPTION_USER_CET_SET_CONTEXT_IP_VALIDATION, // since 21H1
    PS_MITIGATION_OPTION_BLOCK_NON_CET_BINARIES,
    PS_MITIGATION_OPTION_CET_DYNAMIC_APIS_OUT_OF_PROC_ONLY,
    PS_MITIGATION_OPTION_REDIRECTION_TRUST, // since 22H1
} PS_MITIGATION_OPTION;

// windows-internals-book:"Chapter 5"
typedef enum _PS_CREATE_STATE
{
    PsCreateInitialState,
    PsCreateFailOnFileOpen,
    PsCreateFailOnSectionCreate,
    PsCreateFailExeFormat,
    PsCreateFailMachineMismatch,
    PsCreateFailExeName, // Debugger specified
    PsCreateSuccess,
    PsCreateMaximumStates
} PS_CREATE_STATE;

typedef struct _PS_CREATE_INFO
{
    SIZE_T Size;
    PS_CREATE_STATE State;
    union
    {
        // PsCreateInitialState
        struct
        {
            union
            {
                ULONG InitFlags;
                struct
                {
                    UCHAR WriteOutputOnExit : 1;
                    UCHAR DetectManifest : 1;
                    UCHAR IFEOSkipDebugger : 1;
                    UCHAR IFEODoNotPropagateKeyState : 1;
                    UCHAR SpareBits1 : 4;
                    UCHAR SpareBits2 : 8;
                    USHORT ProhibitedImageCharacteristics : 16;
                };
            };
            ACCESS_MASK AdditionalFileAccess;
        } InitState;

        // PsCreateFailOnSectionCreate
        struct
        {
            HANDLE FileHandle;
        } FailSection;

        // PsCreateFailExeFormat
        struct
        {
            USHORT DllCharacteristics;
        } ExeFormat;

        // PsCreateFailExeName
        struct
        {
            HANDLE IFEOKey;
        } ExeName;

        // PsCreateSuccess
        struct
        {
            union
            {
                ULONG OutputFlags;
                struct
                {
                    UCHAR ProtectedProcess : 1;
                    UCHAR AddressSpaceOverride : 1;
                    UCHAR DevOverrideEnabled : 1; // from Image File Execution Options
                    UCHAR ManifestDetected : 1;
                    UCHAR ProtectedProcessLight : 1;
                    UCHAR SpareBits1 : 3;
                    UCHAR SpareBits2 : 8;
                    USHORT SpareBits3 : 16;
                };
            };
            HANDLE FileHandle;
            HANDLE SectionHandle;
            ULONGLONG UserProcessParametersNative;
            ULONG UserProcessParametersWow64;
            ULONG CurrentParameterFlags;
            ULONGLONG PebAddressNative;
            ULONG PebAddressWow64;
            ULONGLONG ManifestAddress;
            ULONG ManifestSize;
        } SuccessState;
    };
} PS_CREATE_INFO, *PPS_CREATE_INFO;

// end_private

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateUserProcess(
    _Out_ PHANDLE ProcessHandle,
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK ProcessDesiredAccess,
    _In_ ACCESS_MASK ThreadDesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ProcessObjectAttributes,
    _In_opt_ POBJECT_ATTRIBUTES ThreadObjectAttributes,
    _In_ ULONG ProcessFlags, // PROCESS_CREATE_FLAGS_*
    _In_ ULONG ThreadFlags, // THREAD_CREATE_FLAGS_*
    _In_opt_ PVOID ProcessParameters, // PRTL_USER_PROCESS_PARAMETERS
    _Inout_ PPS_CREATE_INFO CreateInfo,
    _In_opt_ PPS_ATTRIBUTE_LIST AttributeList
    );
#endif

// begin_rev
#define THREAD_CREATE_FLAGS_NONE 0x00000000
#define THREAD_CREATE_FLAGS_CREATE_SUSPENDED 0x00000001 // NtCreateUserProcess & NtCreateThreadEx
#define THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH 0x00000002 // NtCreateThreadEx only
#define THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER 0x00000004 // NtCreateThreadEx only
#define THREAD_CREATE_FLAGS_LOADER_WORKER 0x00000010 // NtCreateThreadEx only
#define THREAD_CREATE_FLAGS_SKIP_LOADER_INIT 0x00000020 // NtCreateThreadEx only
#define THREAD_CREATE_FLAGS_BYPASS_PROCESS_FREEZE 0x00000040 // NtCreateThreadEx only
// end_rev

#if (PHNT_VERSION >= PHNT_VISTA)

typedef NTSTATUS (NTAPI *PUSER_THREAD_START_ROUTINE)(
    _In_ PVOID ThreadParameter
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateThreadEx(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ProcessHandle,
    _In_ PUSER_THREAD_START_ROUTINE StartRoutine,
    _In_opt_ PVOID Argument,
    _In_ ULONG CreateFlags, // THREAD_CREATE_FLAGS_*
    _In_ SIZE_T ZeroBits,
    _In_ SIZE_T StackSize,
    _In_ SIZE_T MaximumStackSize,
    _In_opt_ PPS_ATTRIBUTE_LIST AttributeList
    );

#endif

#endif

// Job objects

#if (PHNT_MODE != PHNT_MODE_KERNEL)

// JOBOBJECTINFOCLASS
// Note: We don't use an enum since it conflicts with the Windows SDK.
#define JobObjectBasicAccountingInformation 1 // JOBOBJECT_BASIC_ACCOUNTING_INFORMATION
#define JobObjectBasicLimitInformation 2 // JOBOBJECT_BASIC_LIMIT_INFORMATION
#define JobObjectBasicProcessIdList 3 // JOBOBJECT_BASIC_PROCESS_ID_LIST
#define JobObjectBasicUIRestrictions 4 // JOBOBJECT_BASIC_UI_RESTRICTIONS
#define JobObjectSecurityLimitInformation 5 // JOBOBJECT_SECURITY_LIMIT_INFORMATION
#define JobObjectEndOfJobTimeInformation 6 // JOBOBJECT_END_OF_JOB_TIME_INFORMATION
#define JobObjectAssociateCompletionPortInformation 7 // JOBOBJECT_ASSOCIATE_COMPLETION_PORT
#define JobObjectBasicAndIoAccountingInformation 8 // JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION
#define JobObjectExtendedLimitInformation 9 // JOBOBJECT_EXTENDED_LIMIT_INFORMATION
#define JobObjectJobSetInformation 10 // JOBOBJECT_JOBSET_INFORMATION
#define JobObjectGroupInformation 11 // USHORT
#define JobObjectNotificationLimitInformation 12 // JOBOBJECT_NOTIFICATION_LIMIT_INFORMATION
#define JobObjectLimitViolationInformation 13 // JOBOBJECT_LIMIT_VIOLATION_INFORMATION
#define JobObjectGroupInformationEx 14 // GROUP_AFFINITY (ARRAY)
#define JobObjectCpuRateControlInformation 15 // JOBOBJECT_CPU_RATE_CONTROL_INFORMATION
#define JobObjectCompletionFilter 16
#define JobObjectCompletionCounter 17
#define JobObjectFreezeInformation 18 // JOBOBJECT_FREEZE_INFORMATION
#define JobObjectExtendedAccountingInformation 19 // JOBOBJECT_EXTENDED_ACCOUNTING_INFORMATION
#define JobObjectWakeInformation 20 // JOBOBJECT_WAKE_INFORMATION
#define JobObjectBackgroundInformation 21
#define JobObjectSchedulingRankBiasInformation 22
#define JobObjectTimerVirtualizationInformation 23
#define JobObjectCycleTimeNotification 24
#define JobObjectClearEvent 25
#define JobObjectInterferenceInformation 26 // JOBOBJECT_INTERFERENCE_INFORMATION
#define JobObjectClearPeakJobMemoryUsed 27
#define JobObjectMemoryUsageInformation 28 // JOBOBJECT_MEMORY_USAGE_INFORMATION // JOBOBJECT_MEMORY_USAGE_INFORMATION_V2
#define JobObjectSharedCommit 29
#define JobObjectContainerId 30
#define JobObjectIoRateControlInformation 31
#define JobObjectNetRateControlInformation 32 // JOBOBJECT_NET_RATE_CONTROL_INFORMATION
#define JobObjectNotificationLimitInformation2 33 // JOBOBJECT_NOTIFICATION_LIMIT_INFORMATION_2
#define JobObjectLimitViolationInformation2 34 // JOBOBJECT_LIMIT_VIOLATION_INFORMATION_2
#define JobObjectCreateSilo 35
#define JobObjectSiloBasicInformation 36 // SILOOBJECT_BASIC_INFORMATION
#define JobObjectSiloRootDirectory 37 // SILOOBJECT_ROOT_DIRECTORY
#define JobObjectServerSiloBasicInformation 38 // SERVERSILO_BASIC_INFORMATION
#define JobObjectServerSiloUserSharedData 39 // SILO_USER_SHARED_DATA
#define JobObjectServerSiloInitialize 40
#define JobObjectServerSiloRunningState 41
#define JobObjectIoAttribution 42
#define JobObjectMemoryPartitionInformation 43
#define JobObjectContainerTelemetryId 44
#define JobObjectSiloSystemRoot 45
#define JobObjectEnergyTrackingState 46 // JOBOBJECT_ENERGY_TRACKING_STATE
#define JobObjectThreadImpersonationInformation 47
#define JobObjectIoPriorityLimit 48
#define JobObjectPagePriorityLimit 49
#define MaxJobObjectInfoClass 50

// private
typedef struct _JOBOBJECT_EXTENDED_ACCOUNTING_INFORMATION
{
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION BasicInfo;
    IO_COUNTERS IoInfo;
    PROCESS_DISK_COUNTERS DiskIoInfo;
    ULONG64 ContextSwitches;
    LARGE_INTEGER TotalCycleTime;
    ULONG64 ReadyTime;
    PROCESS_ENERGY_VALUES EnergyValues;
} JOBOBJECT_EXTENDED_ACCOUNTING_INFORMATION, *PJOBOBJECT_EXTENDED_ACCOUNTING_INFORMATION;

// private
typedef struct _JOBOBJECT_WAKE_INFORMATION
{
    HANDLE NotificationChannel;
    ULONG64 WakeCounters[7];
} JOBOBJECT_WAKE_INFORMATION, *PJOBOBJECT_WAKE_INFORMATION;

// private
typedef struct _JOBOBJECT_WAKE_INFORMATION_V1
{
    HANDLE NotificationChannel;
    ULONG64 WakeCounters[4];
} JOBOBJECT_WAKE_INFORMATION_V1, *PJOBOBJECT_WAKE_INFORMATION_V1;

// private
typedef struct _JOBOBJECT_INTERFERENCE_INFORMATION
{
    ULONG64 Count;
} JOBOBJECT_INTERFERENCE_INFORMATION, *PJOBOBJECT_INTERFERENCE_INFORMATION;

// private
typedef struct _JOBOBJECT_WAKE_FILTER
{
    ULONG HighEdgeFilter;
    ULONG LowEdgeFilter;
} JOBOBJECT_WAKE_FILTER, *PJOBOBJECT_WAKE_FILTER;

// private
typedef struct _JOBOBJECT_FREEZE_INFORMATION
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG FreezeOperation : 1;
            ULONG FilterOperation : 1;
            ULONG SwapOperation : 1;
            ULONG Reserved : 29;
        };
    };
    BOOLEAN Freeze;
    BOOLEAN Swap;
    UCHAR Reserved0[2];
    JOBOBJECT_WAKE_FILTER WakeFilter;
} JOBOBJECT_FREEZE_INFORMATION, *PJOBOBJECT_FREEZE_INFORMATION;

// private
typedef struct _JOBOBJECT_MEMORY_USAGE_INFORMATION
{
    ULONG64 JobMemory;
    ULONG64 PeakJobMemoryUsed;
} JOBOBJECT_MEMORY_USAGE_INFORMATION, *PJOBOBJECT_MEMORY_USAGE_INFORMATION;

// private
typedef struct _JOBOBJECT_MEMORY_USAGE_INFORMATION_V2
{
    JOBOBJECT_MEMORY_USAGE_INFORMATION BasicInfo;
    ULONG64 JobSharedMemory;
    ULONG64 Reserved[2];
} JOBOBJECT_MEMORY_USAGE_INFORMATION_V2, *PJOBOBJECT_MEMORY_USAGE_INFORMATION_V2;

// private
typedef struct _SILO_USER_SHARED_DATA
{
    ULONG ServiceSessionId;
    ULONG ActiveConsoleId;
    LONGLONG ConsoleSessionForegroundProcessId;
    NT_PRODUCT_TYPE NtProductType;
    ULONG SuiteMask;
    ULONG SharedUserSessionId; // since RS2
    BOOLEAN IsMultiSessionSku;
    WCHAR NtSystemRoot[260];
    USHORT UserModeGlobalLogger[16];
    ULONG TimeZoneId; // since 21H2
    LONG TimeZoneBiasStamp;
    KSYSTEM_TIME TimeZoneBias;
    LARGE_INTEGER TimeZoneBiasEffectiveStart;
    LARGE_INTEGER TimeZoneBiasEffectiveEnd;
} SILO_USER_SHARED_DATA, *PSILO_USER_SHARED_DATA;

// private
typedef struct _SILOOBJECT_ROOT_DIRECTORY
{
    ULONG ControlFlags;
    UNICODE_STRING Path;
} SILOOBJECT_ROOT_DIRECTORY, *PSILOOBJECT_ROOT_DIRECTORY;

// private
typedef struct _JOBOBJECT_ENERGY_TRACKING_STATE
{
    ULONG64 Value;
    ULONG UpdateMask;
    ULONG DesiredState;
} JOBOBJECT_ENERGY_TRACKING_STATE, *PJOBOBJECT_ENERGY_TRACKING_STATE;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateJobObject(
    _Out_ PHANDLE JobHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenJobObject(
    _Out_ PHANDLE JobHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAssignProcessToJobObject(
    _In_ HANDLE JobHandle,
    _In_ HANDLE ProcessHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTerminateJobObject(
    _In_ HANDLE JobHandle,
    _In_ NTSTATUS ExitStatus
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtIsProcessInJob(
    _In_ HANDLE ProcessHandle,
    _In_opt_ HANDLE JobHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationJobObject(
    _In_opt_ HANDLE JobHandle,
    _In_ JOBOBJECTINFOCLASS JobObjectInformationClass,
    _Out_writes_bytes_(JobObjectInformationLength) PVOID JobObjectInformation,
    _In_ ULONG JobObjectInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationJobObject(
    _In_ HANDLE JobHandle,
    _In_ JOBOBJECTINFOCLASS JobObjectInformationClass,
    _In_reads_bytes_(JobObjectInformationLength) PVOID JobObjectInformation,
    _In_ ULONG JobObjectInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateJobSet(
    _In_ ULONG NumJob,
    _In_reads_(NumJob) PJOB_SET_ARRAY UserJobSet,
    _In_ ULONG Flags
    );

#if (PHNT_VERSION >= PHNT_THRESHOLD)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRevertContainerImpersonation(
    VOID
    );
#endif

#endif

// Reserve objects

#if (PHNT_MODE != PHNT_MODE_KERNEL)

// private
typedef enum _MEMORY_RESERVE_TYPE
{
    MemoryReserveUserApc,
    MemoryReserveIoCompletion,
    MemoryReserveTypeMax
} MEMORY_RESERVE_TYPE;

#if (PHNT_VERSION >= PHNT_WIN7)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateReserveObject(
    _Out_ PHANDLE MemoryReserveHandle,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ MEMORY_RESERVE_TYPE Type
    );
#endif

// Process snapshotting

#if (PHNT_VERSION >= PHNT_WINBLUE)
// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
PssNtCaptureSnapshot(
    _Out_ PHANDLE SnapshotHandle,
    _In_ HANDLE ProcessHandle,
    _In_ ULONG CaptureFlags,
    _In_ ULONG ThreadContextFlags
    );
#endif

#endif

#endif
