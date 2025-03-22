/*
 * Process support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTPSAPI_H
#define _NTPSAPI_H

#include <ntpebteb.h>

//
// Process Object Specific Access Rights
//

#ifndef PROCESS_TERMINATE
#define PROCESS_TERMINATE 0x0001
#endif
#ifndef PROCESS_CREATE_THREAD
#define PROCESS_CREATE_THREAD 0x0002
#endif
#ifndef PROCESS_SET_SESSIONID
#define PROCESS_SET_SESSIONID 0x0004
#endif
#ifndef PROCESS_VM_OPERATION
#define PROCESS_VM_OPERATION 0x0008
#endif
#ifndef PROCESS_VM_READ
#define PROCESS_VM_READ 0x0010
#endif
#ifndef PROCESS_VM_WRITE
#define PROCESS_VM_WRITE 0x0020
#endif
#ifndef PROCESS_DUP_HANDLE
#define PROCESS_DUP_HANDLE 0x0040
#endif
#ifndef PROCESS_CREATE_PROCESS
#define PROCESS_CREATE_PROCESS 0x0080
#endif
#ifndef PROCESS_SET_QUOTA
#define PROCESS_SET_QUOTA 0x0100
#endif
#ifndef PROCESS_SET_INFORMATION
#define PROCESS_SET_INFORMATION 0x0200
#endif
#ifndef PROCESS_QUERY_INFORMATION
#define PROCESS_QUERY_INFORMATION 0x0400
#endif
#ifndef PROCESS_SET_PORT
#define PROCESS_SET_PORT 0x0800
#endif
#ifndef PROCESS_SUSPEND_RESUME
#define PROCESS_SUSPEND_RESUME 0x0800
#endif
#ifndef PROCESS_QUERY_LIMITED_INFORMATION
#define PROCESS_QUERY_LIMITED_INFORMATION 0x1000
#endif
#ifndef PROCESS_SET_LIMITED_INFORMATION
#define PROCESS_SET_LIMITED_INFORMATION 0x2000
#endif
#ifndef PROCESS_ALL_ACCESS
#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)
#define PROCESS_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | SPECIFIC_RIGHTS_ALL)
#else
#define PROCESS_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFF)
#endif
#endif

//
// Thread Object Specific Access Rights
//

#ifndef THREAD_TERMINATE
#define THREAD_TERMINATE 0x0001
#endif
#ifndef THREAD_SUSPEND_RESUME
#define THREAD_SUSPEND_RESUME 0x0002
#endif
#ifndef THREAD_ALERT
#define THREAD_ALERT 0x0004
#endif
#ifndef THREAD_GET_CONTEXT
#define THREAD_GET_CONTEXT 0x0008
#endif
#ifndef THREAD_SET_CONTEXT
#define THREAD_SET_CONTEXT 0x0010
#endif
#ifndef THREAD_SET_INFORMATION
#define THREAD_SET_INFORMATION 0x0020
#endif
#ifndef THREAD_QUERY_INFORMATION
#define THREAD_QUERY_INFORMATION 0x0040
#endif
#ifndef THREAD_SET_THREAD_TOKEN
#define THREAD_SET_THREAD_TOKEN 0x0080
#endif
#ifndef THREAD_IMPERSONATE
#define THREAD_IMPERSONATE 0x0100
#endif
#ifndef THREAD_DIRECT_IMPERSONATION
#define THREAD_DIRECT_IMPERSONATION 0x0200
#endif
#ifndef THREAD_SET_LIMITED_INFORMATION
#define THREAD_SET_LIMITED_INFORMATION 0x0400
#endif
#ifndef THREAD_QUERY_LIMITED_INFORMATION
#define THREAD_QUERY_LIMITED_INFORMATION 0x0800
#endif
#ifndef THREAD_RESUME
#define THREAD_RESUME 0x1000
#endif
#ifndef THREAD_ALL_ACCESS
#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)
#define THREAD_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | SPECIFIC_RIGHTS_ALL)
#else
#define THREAD_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x3FF)
#endif
#endif

//
// Job Object Specific Access Rights
//

#ifndef JOB_OBJECT_ASSIGN_PROCESS
#define JOB_OBJECT_ASSIGN_PROCESS 0x0001
#endif
#ifndef JOB_OBJECT_SET_ATTRIBUTES
#define JOB_OBJECT_SET_ATTRIBUTES 0x0002
#endif
#ifndef JOB_OBJECT_QUERY
#define JOB_OBJECT_QUERY 0x0004
#endif
#ifndef JOB_OBJECT_TERMINATE
#define JOB_OBJECT_TERMINATE 0x0008
#endif
#ifndef JOB_OBJECT_SET_SECURITY_ATTRIBUTES
#define JOB_OBJECT_SET_SECURITY_ATTRIBUTES 0x0010
#endif
#ifndef JOB_OBJECT_ALL_ACCESS
#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)
#define JOB_OBJECT_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x3F)
#else
#define JOB_OBJECT_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1f) // pre-Vista full access
#endif
#endif

//
// Process information structures
//

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
    ProcessIoPortHandlers, // (kernel-mode only) // s: PROCESS_IO_PORT_HANDLER_INFORMATION
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
    ProcessHandleTracing, // q: PROCESS_HANDLE_TRACING_QUERY; s: PROCESS_HANDLE_TRACING_ENABLE[_EX] or void to disable
    ProcessIoPriority, // qs: IO_PRIORITY_HINT
    ProcessExecuteFlags, // qs: ULONG (MEM_EXECUTE_OPTION_*)
    ProcessTlsInformation, // PROCESS_TLS_INFORMATION // ProcessResourceManagement
    ProcessCookie, // q: ULONG
    ProcessImageInformation, // q: SECTION_IMAGE_INFORMATION
    ProcessCycleTime, // q: PROCESS_CYCLE_TIME_INFORMATION // since VISTA
    ProcessPagePriority, // qs: PAGE_PRIORITY_INFORMATION
    ProcessInstrumentationCallback, // s: PVOID or PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION // 40
    ProcessThreadStackAllocation, // s: PROCESS_STACK_ALLOCATION_INFORMATION, PROCESS_STACK_ALLOCATION_INFORMATION_EX
    ProcessWorkingSetWatchEx, // q: PROCESS_WS_WATCH_INFORMATION_EX[]; s: void
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
    ProcessDynamicFunctionTableInformation, // s: PROCESS_DYNAMIC_FUNCTION_TABLE_INFORMATION
    ProcessHandleCheckingMode, // qs: ULONG; s: 0 disables, otherwise enables
    ProcessKeepAliveCount, // q: PROCESS_KEEPALIVE_COUNT_INFORMATION
    ProcessRevokeFileHandles, // s: PROCESS_REVOKE_FILE_HANDLES_INFORMATION
    ProcessWorkingSetControl, // s: PROCESS_WORKING_SET_CONTROL
    ProcessHandleTable, // q: ULONG[] // since WINBLUE
    ProcessCheckStackExtentsMode, // qs: ULONG // KPROCESS->CheckStackExtents (CFG)
    ProcessCommandLineInformation, // q: UNICODE_STRING // 60
    ProcessProtectionInformation, // q: PS_PROTECTION
    ProcessMemoryExhaustion, // s: PROCESS_MEMORY_EXHAUSTION_INFO // since THRESHOLD
    ProcessFaultInformation, // s: PROCESS_FAULT_INFORMATION
    ProcessTelemetryIdInformation, // q: PROCESS_TELEMETRY_ID_INFORMATION
    ProcessCommitReleaseInformation, // qs: PROCESS_COMMIT_RELEASE_INFORMATION
    ProcessDefaultCpuSetsInformation, // qs: SYSTEM_CPU_SET_INFORMATION[5]
    ProcessAllowedCpuSetsInformation, // qs: SYSTEM_CPU_SET_INFORMATION[5]
    ProcessSubsystemProcess,
    ProcessJobMemoryInformation, // q: PROCESS_JOB_MEMORY_INFO
    ProcessInPrivate, // q: BOOLEAN; s: void // ETW // since THRESHOLD2 // 70
    ProcessRaiseUMExceptionOnInvalidHandleClose, // qs: ULONG; s: 0 disables, otherwise enables
    ProcessIumChallengeResponse,
    ProcessChildProcessInformation, // q: PROCESS_CHILD_PROCESS_INFORMATION
    ProcessHighGraphicsPriorityInformation, // qs: BOOLEAN (requires SeTcbPrivilege)
    ProcessSubsystemInformation, // q: SUBSYSTEM_INFORMATION_TYPE // since REDSTONE2
    ProcessEnergyValues, // q: PROCESS_ENERGY_VALUES, PROCESS_EXTENDED_ENERGY_VALUES
    ProcessPowerThrottlingState, // qs: POWER_THROTTLING_PROCESS_STATE
    ProcessReserved3Information, // ProcessActivityThrottlePolicy // PROCESS_ACTIVITY_THROTTLE_POLICY
    ProcessWin32kSyscallFilterInformation, // q: WIN32K_SYSCALL_FILTER
    ProcessDisableSystemAllowedCpuSets, // s: BOOLEAN // 80
    ProcessWakeInformation, // q: PROCESS_WAKE_INFORMATION
    ProcessEnergyTrackingState, // qs: PROCESS_ENERGY_TRACKING_STATE
    ProcessManageWritesToExecutableMemory, // MANAGE_WRITES_TO_EXECUTABLE_MEMORY // since REDSTONE3
    ProcessCaptureTrustletLiveDump,
    ProcessTelemetryCoverage, // q: TELEMETRY_COVERAGE_HEADER; s: TELEMETRY_COVERAGE_POINT
    ProcessEnclaveInformation,
    ProcessEnableReadWriteVmLogging, // qs: PROCESS_READWRITEVM_LOGGING_INFORMATION
    ProcessUptimeInformation, // q: PROCESS_UPTIME_INFORMATION
    ProcessImageSection, // q: HANDLE
    ProcessDebugAuthInformation, // since REDSTONE4 // 90
    ProcessSystemResourceManagement, // s: PROCESS_SYSTEM_RESOURCE_MANAGEMENT
    ProcessSequenceNumber, // q: ULONGLONG
    ProcessLoaderDetour, // since REDSTONE5
    ProcessSecurityDomainInformation, // q: PROCESS_SECURITY_DOMAIN_INFORMATION
    ProcessCombineSecurityDomainsInformation, // s: PROCESS_COMBINE_SECURITY_DOMAINS_INFORMATION
    ProcessEnableLogging, // qs: PROCESS_LOGGING_INFORMATION
    ProcessLeapSecondInformation, // qs: PROCESS_LEAP_SECOND_INFORMATION
    ProcessFiberShadowStackAllocation, // s: PROCESS_FIBER_SHADOW_STACK_ALLOCATION_INFORMATION // since 19H1
    ProcessFreeFiberShadowStackAllocation, // s: PROCESS_FREE_FIBER_SHADOW_STACK_ALLOCATION_INFORMATION
    ProcessAltSystemCallInformation, // s: PROCESS_SYSCALL_PROVIDER_INFORMATION // since 20H1 // 100
    ProcessDynamicEHContinuationTargets, // s: PROCESS_DYNAMIC_EH_CONTINUATION_TARGETS_INFORMATION
    ProcessDynamicEnforcedCetCompatibleRanges, // s: PROCESS_DYNAMIC_ENFORCED_ADDRESS_RANGE_INFORMATION // since 20H2
    ProcessCreateStateChange, // since WIN11
    ProcessApplyStateChange,
    ProcessEnableOptionalXStateFeatures, // s: ULONG64 // optional XState feature bitmask
    ProcessAltPrefetchParam, // qs: OVERRIDE_PREFETCH_PARAMETER // App Launch Prefetch (ALPF) // since 22H1
    ProcessAssignCpuPartitions, // HANDLE
    ProcessPriorityClassEx, // s: PROCESS_PRIORITY_CLASS_EX
    ProcessMembershipInformation, // q: PROCESS_MEMBERSHIP_INFORMATION
    ProcessEffectiveIoPriority, // q: IO_PRIORITY_HINT // 110
    ProcessEffectivePagePriority, // q: ULONG
    ProcessSchedulerSharedData, // SCHEDULER_SHARED_DATA_SLOT_INFORMATION // since 24H2
    ProcessSlistRollbackInformation,
    ProcessNetworkIoCounters, // q: PROCESS_NETWORK_COUNTERS
    ProcessFindFirstThreadByTebValue, // PROCESS_TEB_VALUE_INFORMATION
    ProcessEnclaveAddressSpaceRestriction, // since 25H2
    ProcessAvailableCpus,
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
    ThreadEventPair, // Obsolete
    ThreadQuerySetWin32StartAddress, // q: ULONG_PTR
    ThreadZeroTlsCell, // s: ULONG // TlsIndex // 10
    ThreadPerformanceCount, // q: LARGE_INTEGER
    ThreadAmILastThread, // q: ULONG
    ThreadIdealProcessor, // s: ULONG
    ThreadPriorityBoost, // qs: ULONG
    ThreadSetTlsArrayAddress, // s: ULONG_PTR // Obsolete
    ThreadIsIoPending, // q: ULONG
    ThreadHideFromDebugger, // q: BOOLEAN; s: void
    ThreadBreakOnTermination, // qs: ULONG
    ThreadSwitchLegacyState, // s: void // NtCurrentThread // NPX/FPU
    ThreadIsTerminated, // q: ULONG // 20
    ThreadLastSystemCall, // q: THREAD_LAST_SYSCALL_INFORMATION
    ThreadIoPriority, // qs: IO_PRIORITY_HINT (requires SeIncreaseBasePriorityPrivilege)
    ThreadCycleTime, // q: THREAD_CYCLE_TIME_INFORMATION (requires THREAD_QUERY_LIMITED_INFORMATION)
    ThreadPagePriority, // qs: PAGE_PRIORITY_INFORMATION
    ThreadActualBasePriority, // s: LONG (requires SeIncreaseBasePriorityPrivilege)
    ThreadTebInformation, // q: THREAD_TEB_INFORMATION (requires THREAD_GET_CONTEXT + THREAD_SET_CONTEXT)
    ThreadCSwitchMon, // Obsolete
    ThreadCSwitchPmu, // Obsolete
    ThreadWow64Context, // qs: WOW64_CONTEXT, ARM_NT_CONTEXT since 20H1
    ThreadGroupInformation, // qs: GROUP_AFFINITY // 30
    ThreadUmsInformation, // q: THREAD_UMS_INFORMATION // Obsolete
    ThreadCounterProfiling, // q: BOOLEAN; s: THREAD_PROFILING_INFORMATION?
    ThreadIdealProcessorEx, // qs: PROCESSOR_NUMBER; s: previous PROCESSOR_NUMBER on return
    ThreadCpuAccountingInformation, // q: BOOLEAN; s: HANDLE (NtOpenSession) // NtCurrentThread // since WIN8
    ThreadSuspendCount, // q: ULONG // since WINBLUE
    ThreadHeterogeneousCpuPolicy, // q: KHETERO_CPU_POLICY // since THRESHOLD
    ThreadContainerId, // q: GUID
    ThreadNameInformation, // qs: THREAD_NAME_INFORMATION (requires THREAD_SET_LIMITED_INFORMATION)
    ThreadSelectedCpuSets, // q: ULONG[]
    ThreadSystemThreadInformation, // q: SYSTEM_THREAD_INFORMATION // 40
    ThreadActualGroupAffinity, // q: GROUP_AFFINITY // since THRESHOLD2
    ThreadDynamicCodePolicyInfo, // q: ULONG; s: ULONG (NtCurrentThread)
    ThreadExplicitCaseSensitivity, // qs: ULONG; s: 0 disables, otherwise enables // (requires SeDebugPrivilege and PsProtectedSignerAntimalware)
    ThreadWorkOnBehalfTicket, // RTL_WORK_ON_BEHALF_TICKET_EX
    ThreadSubsystemInformation, // q: SUBSYSTEM_INFORMATION_TYPE // since REDSTONE2
    ThreadDbgkWerReportActive, // s: ULONG; s: 0 disables, otherwise enables
    ThreadAttachContainer, // s: HANDLE (job object) // NtCurrentThread
    ThreadManageWritesToExecutableMemory, // MANAGE_WRITES_TO_EXECUTABLE_MEMORY // since REDSTONE3
    ThreadPowerThrottlingState, // qs: POWER_THROTTLING_THREAD_STATE // since REDSTONE3 (set), WIN11 22H2 (query)
    ThreadWorkloadClass, // THREAD_WORKLOAD_CLASS // since REDSTONE5 // 50
    ThreadCreateStateChange, // since WIN11
    ThreadApplyStateChange,
    ThreadStrongerBadHandleChecks, // s: ULONG // NtCurrentThread // since 22H1
    ThreadEffectiveIoPriority, // q: IO_PRIORITY_HINT
    ThreadEffectivePagePriority, // q: ULONG
    ThreadUpdateLockOwnership, // THREAD_LOCK_OWNERSHIP // since 24H2
    ThreadSchedulerSharedDataSlot, // SCHEDULER_SHARED_DATA_SLOT_INFORMATION
    ThreadTebInformationAtomic, // q: THREAD_TEB_INFORMATION (requires THREAD_GET_CONTEXT + THREAD_QUERY_INFORMATION)
    ThreadIndexInformation, // THREAD_INDEX_INFORMATION
    MaxThreadInfoClass
} THREADINFOCLASS;
#endif

#if (PHNT_MODE != PHNT_MODE_KERNEL)

// Use with both ProcessPagePriority and ThreadPagePriority
typedef struct _PAGE_PRIORITY_INFORMATION
{
    ULONG PagePriority;
} PAGE_PRIORITY_INFORMATION, *PPAGE_PRIORITY_INFORMATION;

//
// Process information structures
//

/**
 * The PROCESS_BASIC_INFORMATION structure contains basic information about a process.
 *
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryinformationprocess#process_basic_information
 */
typedef struct _PROCESS_BASIC_INFORMATION
{
    NTSTATUS ExitStatus;                    // The exit status of the process. (GetExitCodeProcess)
    PPEB PebBaseAddress;                    // A pointer to the process environment block (PEB) of the process.
    KAFFINITY AffinityMask;                 // The affinity mask of the process. (GetProcessAffinityMask) (deprecated)
    KPRIORITY BasePriority;                 // The base priority of the process. (GetPriorityClass)
    HANDLE UniqueProcessId;                 // The unique identifier of the process. (GetProcessId)
    HANDLE InheritedFromUniqueProcessId;    // The unique identifier of the parent process.
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

/**
 * The PROCESS_EXTENDED_BASIC_INFORMATION structure contains extended basic information about a process.
 */
typedef struct _PROCESS_EXTENDED_BASIC_INFORMATION
{
    _In_ SIZE_T Size; // The size of the structure, in bytes. This member must be set to sizeof(PROCESS_EXTENDED_BASIC_INFORMATION).
    union
    {
        PROCESS_BASIC_INFORMATION BasicInfo;
        struct
        {
            NTSTATUS ExitStatus;    // The exit status of the process. (GetExitCodeProcess)
            PPEB PebBaseAddress;    // A pointer to the process environment block (PEB) of the process.
            KAFFINITY AffinityMask; // The affinity mask of the process. (GetProcessAffinityMask) (deprecated)
            KPRIORITY BasePriority; // The base priority of the process. (GetPriorityClass)
            HANDLE UniqueProcessId; // The unique identifier of the process. (GetProcessId)
            HANDLE InheritedFromUniqueProcessId; // The unique identifier of the parent process.
        };
    };
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
            ULONG IsBackground : 1; // WIN://BGKD
            ULONG IsStronglyNamed : 1; // WIN://SYSAPPID
            ULONG IsSecureProcess : 1;
            ULONG IsSubsystemProcess : 1;
            ULONG IsTrustedApp : 1; // since 24H2
            ULONG SpareBits : 22;
        };
    };
} PROCESS_EXTENDED_BASIC_INFORMATION, *PPROCESS_EXTENDED_BASIC_INFORMATION;

/**
 * The VM_COUNTERS structure contains various memory usage statistics for a process.
 *
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-process_memory_counters
 */
typedef struct _VM_COUNTERS
{
    SIZE_T PeakVirtualSize;             // The peak virtual address space size of this process, in bytes.
    SIZE_T VirtualSize;                 // The virtual address space size of this process, in bytes.
    ULONG PageFaultCount;               // The number of page faults.
    SIZE_T PeakWorkingSetSize;          // The peak working set size, in bytes.
    SIZE_T WorkingSetSize;              // The current working set size, in bytes
    SIZE_T QuotaPeakPagedPoolUsage;     // The peak paged pool usage, in bytes.
    SIZE_T QuotaPagedPoolUsage;         // The current paged pool usage, in bytes.
    SIZE_T QuotaPeakNonPagedPoolUsage;  // The peak non-paged pool usage, in bytes.
    SIZE_T QuotaNonPagedPoolUsage;      // The current non-paged pool usage, in bytes.
    SIZE_T PagefileUsage;               // The Commit Charge value in bytes for this process. Commit Charge is the total amount of private memory that the memory manager has committed for a running process.
    SIZE_T PeakPagefileUsage;           // The peak value in bytes of the Commit Charge during the lifetime of this process.
} VM_COUNTERS, *PVM_COUNTERS;

/**
 * The VM_COUNTERS_EX structure extends VM_COUNTERS to include private memory usage.
 *
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-process_memory_counters_ex2
 */
typedef struct _VM_COUNTERS_EX
{
    SIZE_T PeakVirtualSize;             // The peak virtual address space size of this process, in bytes.
    SIZE_T VirtualSize;                 // The virtual address space size of this process, in bytes.
    ULONG PageFaultCount;               // The number of page faults.
    SIZE_T PeakWorkingSetSize;          // The peak working set size, in bytes.
    SIZE_T WorkingSetSize;              // The current working set size, in bytes
    SIZE_T QuotaPeakPagedPoolUsage;     // The peak paged pool usage, in bytes.
    SIZE_T QuotaPagedPoolUsage;         // The current paged pool usage, in bytes.
    SIZE_T QuotaPeakNonPagedPoolUsage;  // The peak non-paged pool usage, in bytes.
    SIZE_T QuotaNonPagedPoolUsage;      // The current non-paged pool usage, in bytes.
    SIZE_T PagefileUsage;               // The Commit Charge value in bytes for this process. Commit Charge is the total amount of private memory that the memory manager has committed for a running process.
    SIZE_T PeakPagefileUsage;           // The peak value in bytes of the Commit Charge during the lifetime of this process.
    SIZE_T PrivateUsage;                // Same as PagefileUsage. The Commit Charge value in bytes for this process. Commit Charge is the total amount of private memory that the memory manager has committed for a running process.
} VM_COUNTERS_EX, *PVM_COUNTERS_EX;

/**
 * The VM_COUNTERS_EX2 structure extends VM_COUNTERS_EX to include private working set size and shared commit usage.
 *
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-process_memory_counters_ex2
 */
typedef struct _VM_COUNTERS_EX2
{
    union
    {
        VM_COUNTERS_EX CountersEx;
        struct
        {
            SIZE_T PeakVirtualSize;             // The peak virtual address space size of this process, in bytes.
            SIZE_T VirtualSize;                 // The virtual address space size of this process, in bytes.
            ULONG PageFaultCount;               // The number of page faults.
            SIZE_T PeakWorkingSetSize;          // The peak working set size, in bytes.
            SIZE_T WorkingSetSize;              // The current working set size, in bytes
            SIZE_T QuotaPeakPagedPoolUsage;     // The peak paged pool usage, in bytes.
            SIZE_T QuotaPagedPoolUsage;         // The current paged pool usage, in bytes.
            SIZE_T QuotaPeakNonPagedPoolUsage;  // The peak non-paged pool usage, in bytes.
            SIZE_T QuotaNonPagedPoolUsage;      // The current non-paged pool usage, in bytes.
            SIZE_T PagefileUsage;               // The Commit Charge value in bytes for this process. Commit Charge is the total amount of private memory that the memory manager has committed for a running process.
            SIZE_T PeakPagefileUsage;           // The peak value in bytes of the Commit Charge during the lifetime of this process.
            SIZE_T PrivateUsage;                // Same as PagefileUsage. The Commit Charge value in bytes for this process. Commit Charge is the total amount of private memory that the memory manager has committed for a running process.
        };
    };
    SIZE_T PrivateWorkingSetSize;               // The current private working set size, in bytes.
    SIZE_T SharedCommitUsage;                   // The current shared commit usage, in bytes.
} VM_COUNTERS_EX2, *PVM_COUNTERS_EX2;

/**
 * The KERNEL_USER_TIMES structure contains timing information for a process or thread.
 *
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadtimes
 */
typedef struct _KERNEL_USER_TIMES
{
    LARGE_INTEGER CreateTime;        // The creation time of the process or thread.
    LARGE_INTEGER ExitTime;          // The exit time of the process or thread.
    LARGE_INTEGER KernelTime;        // The amount of time the process has executed in kernel mode.
    LARGE_INTEGER UserTime;          // The amount of time the process has executed in user mode.
} KERNEL_USER_TIMES, *PKERNEL_USER_TIMES;

/**
 * The POOLED_USAGE_AND_LIMITS structure contains information about the usage and limits of paged and non-paged pool memory.
 */
typedef struct _POOLED_USAGE_AND_LIMITS
{
    SIZE_T PeakPagedPoolUsage;       // The peak paged pool usage.
    SIZE_T PagedPoolUsage;           // The current paged pool usage.
    SIZE_T PagedPoolLimit;           // The limit on paged pool usage.
    SIZE_T PeakNonPagedPoolUsage;    // The peak non-paged pool usage.
    SIZE_T NonPagedPoolUsage;        // The current non-paged pool usage.
    SIZE_T NonPagedPoolLimit;        // The limit on non-paged pool usage.
    SIZE_T PeakPagefileUsage;        // The peak pagefile usage.
    SIZE_T PagefileUsage;            // The current pagefile usage.
    SIZE_T PagefileLimit;            // The limit on pagefile usage.
} POOLED_USAGE_AND_LIMITS, *PPOOLED_USAGE_AND_LIMITS;

#define PROCESS_EXCEPTION_PORT_ALL_STATE_BITS 0x00000003
#define PROCESS_EXCEPTION_PORT_ALL_STATE_FLAGS ((ULONG_PTR)((1UL << PROCESS_EXCEPTION_PORT_ALL_STATE_BITS) - 1))

/**
 * The PROCESS_EXCEPTION_PORT structure is used to manage exception ports for a process.
 */
typedef struct _PROCESS_EXCEPTION_PORT
{
    //
    // Handle to the exception port. No particular access required.
    //
    _In_ HANDLE ExceptionPortHandle;

    //
    // Miscellaneous state flags to be cached along with the exception
    // port in the kernel.
    //
    _Inout_ ULONG StateFlags;

} PROCESS_EXCEPTION_PORT, *PPROCESS_EXCEPTION_PORT;

/**
 * The PROCESS_ACCESS_TOKEN structure is used to manage the security context of a process or thread.
 *
 * A process's access token can only be changed if the process has no threads or a single thread that has not yet begun execution.
 */
typedef struct _PROCESS_ACCESS_TOKEN
{
    //
    // Handle to Primary token to assign to the process.
    // TOKEN_ASSIGN_PRIMARY access to this token is needed.
    //
    HANDLE Token;

    //
    // Handle to the initial thread of the process.
    // THREAD_QUERY_INFORMATION access to this thread is needed.
    //
    // N.B. This field is unused.
    //
    HANDLE Thread;

} PROCESS_ACCESS_TOKEN, *PPROCESS_ACCESS_TOKEN;

#ifndef _LDT_ENTRY_DEFINED
#define _LDT_ENTRY_DEFINED
typedef struct _LDT_ENTRY
{
    USHORT LimitLow;
    USHORT BaseLow;
    union
    {
        struct
        {
            UCHAR BaseMid;
            UCHAR Flags1;
            UCHAR Flags2;
            UCHAR BaseHi;
        } Bytes;
        struct
        {
            ULONG BaseMid : 8;
            ULONG Type : 5;
            ULONG Dpl : 2;
            ULONG Pres : 1;
            ULONG LimitHi : 4;
            ULONG Sys : 1;
            ULONG Reserved_0 : 1;
            ULONG Default_Big : 1;
            ULONG Granularity : 1;
            ULONG BaseHi : 8;
        } Bits;
    } HighWord;
} LDT_ENTRY, *PLDT_ENTRY;
#endif

/**
 * The PROCESS_LDT_INFORMATION structure is used to manage Local Descriptor Table (LDT) entries for a process.
 */
typedef struct _PROCESS_LDT_INFORMATION
{
    ULONG Start;
    ULONG Length;
    LDT_ENTRY LdtEntries[1];
} PROCESS_LDT_INFORMATION, *PPROCESS_LDT_INFORMATION;

/**
 * The PROCESS_LDT_SIZE structure is used to specify the size of the Local Descriptor Table (LDT) for a process.
 */
typedef struct _PROCESS_LDT_SIZE
{
    ULONG Length;
} PROCESS_LDT_SIZE, *PPROCESS_LDT_SIZE;

/**
 * The PROCESS_WS_WATCH_INFORMATION structure is used to store information about working set watch events for a process.
 *
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-psapi_ws_watch_information
 */
typedef struct _PROCESS_WS_WATCH_INFORMATION
{
    PVOID FaultingPc; // A pointer to the instruction that caused the page fault.
    PVOID FaultingVa; // A pointer to the page that was added to the working set.
} PROCESS_WS_WATCH_INFORMATION, *PPROCESS_WS_WATCH_INFORMATION;

#endif

/**
 * The PROCESS_WS_WATCH_INFORMATION_EX structure contains extended information about a page added to a process working set.
 *
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/psapi/ns-psapi-psapi_ws_watch_information_ex
 */
typedef struct _PROCESS_WS_WATCH_INFORMATION_EX
{
    union
    {
        PROCESS_WS_WATCH_INFORMATION BasicInfo;
        struct
        {
            PVOID FaultingPc;   // The address of the instruction that caused the page fault.
            PVOID FaultingVa;   // The virtual address that caused the page fault.
        };
    };
    HANDLE FaultingThreadId;    // The identifier of the thread that caused the page fault.
    ULONG_PTR Flags;            // This member is reserved for future use.
} PROCESS_WS_WATCH_INFORMATION_EX, *PPROCESS_WS_WATCH_INFORMATION_EX;

#define PROCESS_PRIORITY_CLASS_UNKNOWN 0
#define PROCESS_PRIORITY_CLASS_IDLE 1
#define PROCESS_PRIORITY_CLASS_NORMAL 2
#define PROCESS_PRIORITY_CLASS_HIGH 3
#define PROCESS_PRIORITY_CLASS_REALTIME 4
#define PROCESS_PRIORITY_CLASS_BELOW_NORMAL 5
#define PROCESS_PRIORITY_CLASS_ABOVE_NORMAL 6

/**
 * The PROCESS_PRIORITY_CLASS structure is used to manage the priority class of a process.
 */
typedef struct _PROCESS_PRIORITY_CLASS
{
    BOOLEAN Foreground;
    UCHAR PriorityClass;
} PROCESS_PRIORITY_CLASS, *PPROCESS_PRIORITY_CLASS;

/**
 * The PROCESS_PRIORITY_CLASS_EX structure extends PROCESS_PRIORITY_CLASS to include validity flags.
 */
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

/**
 * The PROCESS_FOREGROUND_BACKGROUND structure is used to manage the the priority class of a process, specifically whether it runs in the foreground or background.
 */
typedef struct _PROCESS_FOREGROUND_BACKGROUND
{
    BOOLEAN Foreground;
} PROCESS_FOREGROUND_BACKGROUND, *PPROCESS_FOREGROUND_BACKGROUND;

#if (PHNT_MODE != PHNT_MODE_KERNEL)

// DriveType
#define DRIVE_UNKNOWN     0
#define DRIVE_NO_ROOT_DIR 1
#define DRIVE_REMOVABLE   2
#define DRIVE_FIXED       3
#define DRIVE_REMOTE      4
#define DRIVE_CDROM       5
#define DRIVE_RAMDISK     6

/**
 * The PROCESS_DEVICEMAP_INFORMATION structure contains information about a process's device map.
 */
typedef struct _PROCESS_DEVICEMAP_INFORMATION
{
    union
    {
        struct
        {
            HANDLE DirectoryHandle; // A handle to a directory object that can be set as the new device map for the process. This handle must have DIRECTORY_TRAVERSE access.
        } Set;
        struct
        {
            ULONG DriveMap;         // A bitmask that indicates which drive letters are currently in use in the process's device map.
            UCHAR DriveType[32];    // A value that indicates the type of each drive (e.g., local disk, network drive, etc.). // DRIVE_* WinBase.h
        } Query;
    };
} PROCESS_DEVICEMAP_INFORMATION, *PPROCESS_DEVICEMAP_INFORMATION;

#define PROCESS_LUID_DOSDEVICES_ONLY 0x00000001

/**
 * The _PROCESS_DEVICEMAP_INFORMATION_EX structure contains information about a process's device map.
 */
typedef struct _PROCESS_DEVICEMAP_INFORMATION_EX
{
    union
    {
        struct
        {
            HANDLE DirectoryHandle; // A handle to a directory object that can be set as the new device map for the process. This handle must have DIRECTORY_TRAVERSE access.
        } Set;
        struct
        {
            ULONG DriveMap;         // A bitmask that indicates which drive letters are currently in use in the process's device map.
            UCHAR DriveType[32];    // A value that indicates the type of each drive (e.g., local disk, network drive, etc.). // DRIVE_* WinBase.h
        } Query;
    };
    ULONG Flags; // PROCESS_LUID_DOSDEVICES_ONLY
} PROCESS_DEVICEMAP_INFORMATION_EX, *PPROCESS_DEVICEMAP_INFORMATION_EX;

/**
 * The PROCESS_SESSION_INFORMATION structure is used to store information about the session ID of a process.
 */
typedef struct _PROCESS_SESSION_INFORMATION
{
    ULONG SessionId;
} PROCESS_SESSION_INFORMATION, *PPROCESS_SESSION_INFORMATION;

#define PROCESS_HANDLE_EXCEPTIONS_ENABLED 0x00000001
#define PROCESS_HANDLE_RAISE_EXCEPTION_ON_INVALID_HANDLE_CLOSE_DISABLED 0x00000000
#define PROCESS_HANDLE_RAISE_EXCEPTION_ON_INVALID_HANDLE_CLOSE_ENABLED 0x00000001

/**
 * The PROCESS_HANDLE_TRACING_ENABLE structure is used to enable handle tracing for a process.
 */
typedef struct _PROCESS_HANDLE_TRACING_ENABLE
{
    ULONG Flags; // Flags that control handle tracing.
} PROCESS_HANDLE_TRACING_ENABLE, *PPROCESS_HANDLE_TRACING_ENABLE;

#define PROCESS_HANDLE_TRACING_MAX_SLOTS 0x20000

/**
 * The PROCESS_HANDLE_TRACING_ENABLE_EX structure extends PROCESS_HANDLE_TRACING_ENABLE to include the total number of slots.
 */
typedef struct _PROCESS_HANDLE_TRACING_ENABLE_EX
{
    ULONG Flags;        // Flags that control handle tracing.
    ULONG TotalSlots;   // Total number of handle tracing slots.
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
    _In_opt_ HANDLE Handle;
    _Out_ ULONG TotalTraces;
    _Out_ _Field_size_(TotalTraces) PROCESS_HANDLE_TRACING_ENTRY HandleTrace[1];
} PROCESS_HANDLE_TRACING_QUERY, *PPROCESS_HANDLE_TRACING_QUERY;

#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

/**
 * The THREAD_TLS_INFORMATION structure contains information about the Thread Local Storage (TLS) data for a thread.
 */
typedef struct _THREAD_TLS_INFORMATION
{
    ULONG Flags;         // Flags that provide additional information about the TLS data.
    PVOID NewTlsData;    // Pointer to the new TLS data.
    PVOID OldTlsData;    // Pointer to the old TLS data.
    HANDLE ThreadId;     // Handle to the thread associated with the TLS data.
} THREAD_TLS_INFORMATION, *PTHREAD_TLS_INFORMATION;

/**
 * The PROCESS_TLS_INFORMATION_TYPE enumeration defines the types of TLS operations that can be performed on a process.
 */
typedef enum _PROCESS_TLS_INFORMATION_TYPE
{
    ProcessTlsReplaceIndex,     // Replace the TLS index.
    ProcessTlsReplaceVector,    // Replace the TLS vector.
    MaxProcessTlsOperation      // Maximum value for the enumeration.
} PROCESS_TLS_INFORMATION_TYPE, *PPROCESS_TLS_INFORMATION_TYPE;

/**
 * The PROCESS_TLS_INFORMATION structure contains information about the TLS operations for a process.
 */
typedef struct _PROCESS_TLS_INFORMATION
{
    ULONG Flags;                // Flags that provide additional information about the TLS operation.
    ULONG OperationType;        // The type of TLS operation to be performed.
    ULONG ThreadDataCount;      // The number of THREAD_TLS_INFORMATION structures in the ThreadData array.
    ULONG TlsIndex;             // The TLS index to be replaced.
    ULONG PreviousCount;        // The previous count of TLS data.
    _Field_size_(ThreadDataCount) THREAD_TLS_INFORMATION ThreadData[1]; // Array of THREAD_TLS_INFORMATION structures.
} PROCESS_TLS_INFORMATION, *PPROCESS_TLS_INFORMATION;

/**
 * The PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION structure contains information about the instrumentation callback for a process.
 */
typedef struct _PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION
{
    ULONG Version;  // The version of the instrumentation callback information.
    ULONG Reserved; // Reserved for future use.
    PVOID Callback; // Pointer to the callback function.
} PROCESS_INSTRUMENTATION_CALLBACK_INFORMATION, *PPROCESS_INSTRUMENTATION_CALLBACK_INFORMATION;

/**
 * The PROCESS_STACK_ALLOCATION_INFORMATION structure contains information about the stack allocation for a process.
 */
typedef struct _PROCESS_STACK_ALLOCATION_INFORMATION
{
    SIZE_T ReserveSize; // The size of the stack to be reserved.
    SIZE_T ZeroBits;    // The number of zero bits in the stack base address.
    PVOID StackBase;    // Pointer to the base of the stack.
} PROCESS_STACK_ALLOCATION_INFORMATION, *PPROCESS_STACK_ALLOCATION_INFORMATION;

/**
 * The PROCESS_STACK_ALLOCATION_INFORMATION_EX structure extends PROCESS_STACK_ALLOCATION_INFORMATION to include additional fields.
 */
typedef struct _PROCESS_STACK_ALLOCATION_INFORMATION_EX
{
    ULONG PreferredNode; // The preferred NUMA node for the stack allocation.
    ULONG Reserved0;     // Reserved for future use.
    ULONG Reserved1;     // Reserved for future use.
    ULONG Reserved2;     // Reserved for future use.
    PROCESS_STACK_ALLOCATION_INFORMATION AllocInfo; // The stack allocation information.
} PROCESS_STACK_ALLOCATION_INFORMATION_EX, *PPROCESS_STACK_ALLOCATION_INFORMATION_EX;
/**
 * The PROCESS_AFFINITY_UPDATE_MODE union is used to specify the affinity update mode for a process.
 */
typedef union _PROCESS_AFFINITY_UPDATE_MODE
{
    ULONG Flags;
    struct
    {
        ULONG EnableAutoUpdate : 1; // Indicates whether auto-update of affinity is enabled.
        ULONG Permanent : 1;        // Indicates whether the affinity update is permanent.
        ULONG Reserved : 30;        // Reserved for future use.
    };
} PROCESS_AFFINITY_UPDATE_MODE, *PPROCESS_AFFINITY_UPDATE_MODE;

/**
 * The PROCESS_MEMORY_ALLOCATION_MODE union is used to specify the memory allocation mode for a process.
 */
typedef union _PROCESS_MEMORY_ALLOCATION_MODE
{
    ULONG Flags;
    struct
    {
        ULONG TopDown : 1;      // Indicates whether memory allocation should be top-down.
        ULONG Reserved : 31;    // Reserved for future use.
    };
} PROCESS_MEMORY_ALLOCATION_MODE, *PPROCESS_MEMORY_ALLOCATION_MODE;

/**
 * The PROCESS_HANDLE_INFORMATION structure contains information about the handles of a process.
 */
typedef struct _PROCESS_HANDLE_INFORMATION
{
    ULONG HandleCount;              // The number of handles in the process.
    ULONG HandleCountHighWatermark; // The highest number of handles that the process has had.
} PROCESS_HANDLE_INFORMATION, *PPROCESS_HANDLE_INFORMATION;

/**
 * The PROCESS_CYCLE_TIME_INFORMATION structure contains information about the cycle time of a process.
 */
typedef struct _PROCESS_CYCLE_TIME_INFORMATION
{
    ULONGLONG AccumulatedCycles; // The total number of cycles accumulated by the process.
    ULONGLONG CurrentCycleCount; // The current cycle count of the process.
} PROCESS_CYCLE_TIME_INFORMATION, *PPROCESS_CYCLE_TIME_INFORMATION;

/**
 * The PROCESS_WINDOW_INFORMATION structure contains information about the windows of a process.
 */
typedef struct _PROCESS_WINDOW_INFORMATION
{
    ULONG WindowFlags;          // Flags that provide information about the window.
    USHORT WindowTitleLength;   // The length of the window title.
    _Field_size_bytes_(WindowTitleLength) WCHAR WindowTitle[1]; // The title of the window.
} PROCESS_WINDOW_INFORMATION, *PPROCESS_WINDOW_INFORMATION;

/**
 * The PROCESS_HANDLE_TABLE_ENTRY_INFO structure contains information about a handle table entry of a process.
 */
typedef struct _PROCESS_HANDLE_TABLE_ENTRY_INFO
{
    HANDLE HandleValue;         // The value of the handle.
    SIZE_T HandleCount;         // The number of references to the handle.
    SIZE_T PointerCount;        // The number of pointers to the handle.
    ACCESS_MASK GrantedAccess;  // The access rights granted to the handle.
    ULONG ObjectTypeIndex;      // The index of the object type.
    ULONG HandleAttributes;     // The attributes of the handle.
    ULONG Reserved;             // Reserved for future use.
} PROCESS_HANDLE_TABLE_ENTRY_INFO, *PPROCESS_HANDLE_TABLE_ENTRY_INFO;

/**
 * The PROCESS_HANDLE_SNAPSHOT_INFORMATION structure contains information about the handle snapshot of a process.
 */
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
} PROCESS_MITIGATION_REDIRECTION_TRUST_POLICY, *PPROCESS_MITIGATION_REDIRECTION_TRUST_POLICY;
#endif // NTDDI_WIN10_FE

#if !defined(NTDDI_WIN10_NI) || (NTDDI_VERSION < NTDDI_WIN10_NI)
typedef struct _PROCESS_MITIGATION_USER_POINTER_AUTH_POLICY {
    union {
        ULONG Flags;
        struct {
            ULONG EnablePointerAuthUserIp : 1;
            ULONG ReservedFlags : 31;
        };
    };
} PROCESS_MITIGATION_USER_POINTER_AUTH_POLICY, *PPROCESS_MITIGATION_USER_POINTER_AUTH_POLICY;

typedef struct _PROCESS_MITIGATION_SEHOP_POLICY {
    union {
        ULONG Flags;
        struct {
            ULONG EnableSehop : 1;
            ULONG ReservedFlags : 31;
        };
    };
} PROCESS_MITIGATION_SEHOP_POLICY, *PPROCESS_MITIGATION_SEHOP_POLICY;
#endif // NTDDI_WIN10_NI

typedef struct _PROCESS_MITIGATION_ACTIVATION_CONTEXT_TRUST_POLICY2
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG AssemblyManifestRedirectionTrust : 1;
            ULONG ReservedFlags : 31;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} PROCESS_MITIGATION_ACTIVATION_CONTEXT_TRUST_POLICY2, *PPROCESS_MITIGATION_ACTIVATION_CONTEXT_TRUST_POLICY2;

#if defined(_PHLIB_)
// enum PROCESS_MITIGATION_POLICY
#define PROCESS_MITIGATION_POLICY ULONG
#define ProcessDEPPolicy 0
#define ProcessASLRPolicy 1
#define ProcessDynamicCodePolicy 2
#define ProcessStrictHandleCheckPolicy 3
#define ProcessSystemCallDisablePolicy 4
#define ProcessMitigationOptionsMask 5
#define ProcessExtensionPointDisablePolicy 6
#define ProcessControlFlowGuardPolicy 7
#define ProcessSignaturePolicy 8
#define ProcessFontDisablePolicy 9
#define ProcessImageLoadPolicy 10
#define ProcessSystemCallFilterPolicy 11
#define ProcessPayloadRestrictionPolicy 12
#define ProcessChildProcessPolicy 13
#define ProcessSideChannelIsolationPolicy 14
#define ProcessUserShadowStackPolicy 15
#define ProcessRedirectionTrustPolicy 16
#define ProcessUserPointerAuthPolicy 17
#define ProcessSEHOPPolicy 18
#define ProcessActivationContextTrustPolicy 19
#define MaxProcessMitigationPolicy 20
#endif // _PHLIB_

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

// private
typedef struct _PROCESS_DYNAMIC_FUNCTION_TABLE_INFORMATION
{
    struct _DYNAMIC_FUNCTION_TABLE* DynamicFunctionTable;
    BOOLEAN Remove;
} PROCESS_DYNAMIC_FUNCTION_TABLE_INFORMATION, *PPROCESS_DYNAMIC_FUNCTION_TABLE_INFORMATION;

typedef struct _PROCESS_KEEPALIVE_COUNT_INFORMATION
{
    ULONG WakeCount;
    ULONG NoWakeCount;
} PROCESS_KEEPALIVE_COUNT_INFORMATION, *PPROCESS_KEEPALIVE_COUNT_INFORMATION;

typedef struct _PROCESS_REVOKE_FILE_HANDLES_INFORMATION
{
    UNICODE_STRING TargetDevicePath;
} PROCESS_REVOKE_FILE_HANDLES_INFORMATION, *PPROCESS_REVOKE_FILE_HANDLES_INFORMATION;

// rev
#define PROCESS_WORKING_SET_CONTROL_VERSION 3

/**
 * The PROCESS_WORKING_SET_OPERATION enumeration defines the operation to perform on a process's working set.
 */
typedef enum _PROCESS_WORKING_SET_OPERATION
{
    ProcessWorkingSetSwap,              // Swap the working set of a process to disk. // (requires SeDebugPrivilege)
    ProcessWorkingSetEmpty,             // Remove all pages from the working set of a process.
    ProcessWorkingSetEmptyPrivatePages, // Remove private pages from the working set of a process.
    ProcessWorkingSetOperationMax
} PROCESS_WORKING_SET_OPERATION;

/**
 * The PROCESS_WORKING_SET_FLAG_EMPTY_PRIVATE_PAGES flag indicates that the operation should target private pages in the working set.
 * Private pages are those that are not shared with other processes.
 */
#define PROCESS_WORKING_SET_FLAG_EMPTY_PRIVATE_PAGES 0x01
/**
 * The PROCESS_WORKING_SET_FLAG_EMPTY_SHARED_PAGES flag indicates that the operation should target shared pages in the working set.
 * Shared pages are those that are shared between multiple processes.
 */
#define PROCESS_WORKING_SET_FLAG_EMPTY_SHARED_PAGES  0x02
/**
 * The PROCESS_WORKING_SET_FLAG_COMPRESS flag indicates that the operation should compress the pages before they are removed from the working set.
 * Compression is typically used in conjunction with other flags to specify that the pages should be compressed as part of the operation.
 */
#define PROCESS_WORKING_SET_FLAG_COMPRESS            0x08
/**
 * The PROCESS_WORKING_SET_FLAG_STORE flag indicates that the operation should store the compressed pages.
 * This is useful when the compressed data might be needed later, allowing for efficient retrieval and decompression when required.
 * This flag is typically used in conjunction with the PROCESS_WORKING_SET_FLAG_COMPRESS flag to specify that the compressed pages should be stored.
 */
#define PROCESS_WORKING_SET_FLAG_STORE               0x10

/**
 * The PROCESS_WORKING_SET_CONTROL structure is used to control the working set of a process.
 */
typedef struct _PROCESS_WORKING_SET_CONTROL
{
    ULONG Version;
    PROCESS_WORKING_SET_OPERATION Operation;
    ULONG Flags;
} PROCESS_WORKING_SET_CONTROL, *PPROCESS_WORKING_SET_CONTROL;

/**
 * The PS_PROTECTED_TYPE enumeration defines the types of protection that can be applied to a process.
 */
typedef enum _PS_PROTECTED_TYPE
{
    PsProtectedTypeNone,            // No protection.
    PsProtectedTypeProtectedLight,  // Light protection.
    PsProtectedTypeProtected,       // Full protection.
    PsProtectedTypeMax
} PS_PROTECTED_TYPE;

/**
 * The PS_PROTECTED_SIGNER enumeration defines the types of signers that can be associated with a protected process.
 */
typedef enum _PS_PROTECTED_SIGNER
{
    PsProtectedSignerNone,          // No signer.
    PsProtectedSignerAuthenticode,  // Authenticode signer.
    PsProtectedSignerCodeGen,       // Code generation signer.
    PsProtectedSignerAntimalware,   // Antimalware signer.
    PsProtectedSignerLsa,           // Local Security Authority signer.
    PsProtectedSignerWindows,       // Windows signer.
    PsProtectedSignerWinTcb,        // Windows Trusted Computing Base signer.
    PsProtectedSignerWinSystem,     // Windows system signer.
    PsProtectedSignerApp,           // Application signer.
    PsProtectedSignerMax
} PS_PROTECTED_SIGNER;

#define PS_PROTECTED_SIGNER_MASK 0xFF
#define PS_PROTECTED_AUDIT_MASK 0x08
#define PS_PROTECTED_TYPE_MASK 0x07

// ProtectionLevel.Level = PsProtectedValue(PsProtectedSignerCodeGen, FALSE, PsProtectedTypeProtectedLight)
#define PsProtectedValue(PsSigner, PsAudit, PsType) ( \
    (((PsSigner) & PS_PROTECTED_SIGNER_MASK) << 4) | \
    (((PsAudit) & PS_PROTECTED_AUDIT_MASK) << 3) | \
    (((PsType) & PS_PROTECTED_TYPE_MASK)) \
    )

// InitializePsProtection(&ProtectionLevel, PsProtectedSignerCodeGen, FALSE, PsProtectedTypeProtectedLight)
#define InitializePsProtection(PsProtectionLevel, PsSigner, PsAudit, PsType) { \
    (PsProtectionLevel)->Signer = (PsSigner); \
    (PsProtectionLevel)->Audit = (PsAudit); \
    (PsProtectionLevel)->Type = (PsType); \
    }

/**
 * The PS_PROTECTION structure is used to define the protection level of a process.
 */
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

/**
 * The PROCESS_FAULT_INFORMATION structure contains information about process faults.
 */
typedef struct _PROCESS_FAULT_INFORMATION
{
    ULONG FaultFlags;       // Flags that provide additional information about the fault.
    ULONG AdditionalInfo;   // Additional information about the fault.
} PROCESS_FAULT_INFORMATION, *PPROCESS_FAULT_INFORMATION;

/**
 * The PROCESS_TELEMETRY_ID_INFORMATION structure contains telemetry information about a process.
 */
typedef struct _PROCESS_TELEMETRY_ID_INFORMATION
{
    ULONG HeaderSize;                       // The size of the structure, in bytes.
    ULONG ProcessId;                        // The ID of the process.
    ULONGLONG ProcessStartKey;              // The start key of the process.
    ULONGLONG CreateTime;                   // The creation time of the process.
    ULONGLONG CreateInterruptTime;          // The interrupt time at creation.
    ULONGLONG CreateUnbiasedInterruptTime;  // The unbiased interrupt time at creation.
    ULONGLONG ProcessSequenceNumber;        // The monotonic sequence number of the process.
    ULONGLONG SessionCreateTime;            // The session creation time.
    ULONG SessionId;                        // The ID of the session.
    ULONG BootId;                           // The boot ID.
    ULONG ImageChecksum;                    // The checksum of the process image.
    ULONG ImageTimeDateStamp;               // The timestamp of the process image.
    ULONG UserSidOffset;                    // The offset to the user SID.
    ULONG ImagePathOffset;                  // The offset to the image path.
    ULONG PackageNameOffset;                // The offset to the package name.
    ULONG RelativeAppNameOffset;            // The offset to the relative application name.
    ULONG CommandLineOffset;                // The offset to the command line.
} PROCESS_TELEMETRY_ID_INFORMATION, *PPROCESS_TELEMETRY_ID_INFORMATION;

/**
 * The PROCESS_COMMIT_RELEASE_INFORMATION structure contains information about the commit and release of memory for a process.
 */
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

/**
 * The PROCESS_JOB_MEMORY_INFO structure contains Represents app memory usage at a single point in time.
 *
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/ns-processthreadsapi-app_memory_information
 */
typedef struct _PROCESS_JOB_MEMORY_INFO
{
    ULONGLONG SharedCommitUsage;        // The current shared commit usage, in bytes.
    ULONGLONG PrivateCommitUsage;       // The current private commit usage, in bytes.
    ULONGLONG PeakPrivateCommitUsage;   // The peak private commit usage, in bytes.
    ULONGLONG PrivateCommitLimit;       // The private commit limit, in bytes.
    ULONGLONG TotalCommitLimit;         // The total commit limit, in bytes.
} PROCESS_JOB_MEMORY_INFO, *PPROCESS_JOB_MEMORY_INFO;

/**
 * The PROCESS_CHILD_PROCESS_INFORMATION structure contains information about child process policies.
 */
typedef struct _PROCESS_CHILD_PROCESS_INFORMATION
{
    BOOLEAN ProhibitChildProcesses;         // Child processes are prohibited.
    BOOLEAN AlwaysAllowSecureChildProcess;  // Secure child processes are always allowed.
    BOOLEAN AuditProhibitChildProcesses;    // Child processes are audited.
} PROCESS_CHILD_PROCESS_INFORMATION, *PPROCESS_CHILD_PROCESS_INFORMATION;

#define POWER_THROTTLING_PROCESS_CURRENT_VERSION 1

#define POWER_THROTTLING_PROCESS_EXECUTION_SPEED 0x1
#define POWER_THROTTLING_PROCESS_DELAYTIMERS 0x2
#define POWER_THROTTLING_PROCESS_IGNORE_TIMER_RESOLUTION 0x4 // since WIN11

#define POWER_THROTTLING_PROCESS_VALID_FLAGS \
    ((POWER_THROTTLING_PROCESS_EXECUTION_SPEED | POWER_THROTTLING_PROCESS_DELAYTIMERS | POWER_THROTTLING_PROCESS_IGNORE_TIMER_RESOLUTION))

/**
 * The POWER_THROTTLING_PROCESS_STATE structure is used to manage the power throttling state of a process.
 */
typedef struct _POWER_THROTTLING_PROCESS_STATE
{
    ULONG Version;       // The version of the structure.
    ULONG ControlMask;   // A mask that specifies the control settings for power throttling.
    ULONG StateMask;     // A mask that specifies the current state of power throttling.
} POWER_THROTTLING_PROCESS_STATE, *PPOWER_THROTTLING_PROCESS_STATE;

// rev (tyranid)
#define WIN32K_SYSCALL_FILTER_STATE_ENABLE 0x1
#define WIN32K_SYSCALL_FILTER_STATE_AUDIT 0x2

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
    struct
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

typedef struct _PROCESS_SECURITY_DOMAIN_INFORMATION
{
    ULONGLONG SecurityDomain;
} PROCESS_SECURITY_DOMAIN_INFORMATION, *PPROCESS_SECURITY_DOMAIN_INFORMATION;

typedef struct _PROCESS_COMBINE_SECURITY_DOMAINS_INFORMATION
{
    HANDLE ProcessHandle;
} PROCESS_COMBINE_SECURITY_DOMAINS_INFORMATION, *PPROCESS_COMBINE_SECURITY_DOMAINS_INFORMATION;

typedef union _PROCESS_LOGGING_INFORMATION
{
    ULONG Flags;
    struct
    {
        ULONG EnableReadVmLogging : 1;
        ULONG EnableWriteVmLogging : 1;
        ULONG EnableProcessSuspendResumeLogging : 1;
        ULONG EnableThreadSuspendResumeLogging : 1;
        ULONG EnableLocalExecProtectVmLogging : 1;
        ULONG EnableRemoteExecProtectVmLogging : 1;
        ULONG EnableImpersonationLogging : 1;
        ULONG Reserved : 25;
    };
} PROCESS_LOGGING_INFORMATION, *PPROCESS_LOGGING_INFORMATION;

typedef struct _PROCESS_LEAP_SECOND_INFORMATION
{
    ULONG Flags;
    ULONG Reserved;
} PROCESS_LEAP_SECOND_INFORMATION, *PPROCESS_LEAP_SECOND_INFORMATION;

typedef struct _PROCESS_FIBER_SHADOW_STACK_ALLOCATION_INFORMATION
{
    ULONGLONG ReserveSize;
    ULONGLONG CommitSize;
    ULONG PreferredNode;
    ULONG Reserved;
    PVOID Ssp;
} PROCESS_FIBER_SHADOW_STACK_ALLOCATION_INFORMATION, *PPROCESS_FIBER_SHADOW_STACK_ALLOCATION_INFORMATION;

typedef struct _PROCESS_FREE_FIBER_SHADOW_STACK_ALLOCATION_INFORMATION
{
    PVOID Ssp;
} PROCESS_FREE_FIBER_SHADOW_STACK_ALLOCATION_INFORMATION, *PPROCESS_FREE_FIBER_SHADOW_STACK_ALLOCATION_INFORMATION;

typedef struct _PROCESS_SYSCALL_PROVIDER_INFORMATION
{
    GUID ProviderId;
    UCHAR Level;
} PROCESS_SYSCALL_PROVIDER_INFORMATION, *PPROCESS_SYSCALL_PROVIDER_INFORMATION;

//typedef struct _PROCESS_DYNAMIC_ENFORCED_ADDRESS_RANGE
//{
//    ULONG_PTR BaseAddress;
//    SIZE_T Size;
//    ULONG Flags;
//} PROCESS_DYNAMIC_ENFORCED_ADDRESS_RANGE, *PPROCESS_DYNAMIC_ENFORCED_ADDRESS_RANGE;
//
//typedef struct _PROCESS_DYNAMIC_ENFORCED_ADDRESS_RANGES_INFORMATION
//{
//    USHORT NumberOfRanges;
//    USHORT Reserved;
//    ULONG Reserved2;
//    PPROCESS_DYNAMIC_ENFORCED_ADDRESS_RANGE Ranges;
//} PROCESS_DYNAMIC_ENFORCED_ADDRESS_RANGES_INFORMATION, *PPROCESS_DYNAMIC_ENFORCED_ADDRESS_RANGES_INFORMATION;

typedef struct _PROCESS_MEMBERSHIP_INFORMATION
{
    ULONG ServerSiloId;
} PROCESS_MEMBERSHIP_INFORMATION, *PPROCESS_MEMBERSHIP_INFORMATION;

#if !defined(NTDDI_WIN11_GE) || (NTDDI_VERSION < NTDDI_WIN11_GE)
typedef struct _PROCESS_NETWORK_COUNTERS
{
    ULONG64 BytesIn;
    ULONG64 BytesOut;
} PROCESS_NETWORK_COUNTERS, *PPROCESS_NETWORK_COUNTERS;
#endif

typedef struct _PROCESS_TEB_VALUE_INFORMATION
{
    ULONG ThreadId;
    ULONG TebOffset;
    ULONG_PTR Value;
} PROCESS_TEB_VALUE_INFORMATION, *PPROCESS_TEB_VALUE_INFORMATION;

/**
 * The NtQueryPortInformationProcess function retrieves the status of the current process exception port.
 *
 * @return LOGICAL If TRUE, the process exception port is valid.
 */
NTSYSCALLAPI
LOGICAL
NTAPI
NtQueryPortInformationProcess(
    VOID
    );

#endif // PHNT_MODE != PHNT_MODE_KERNEL

//
// Thread information structures
//

typedef struct _THREAD_BASIC_INFORMATION
{
    NTSTATUS ExitStatus;
    PTEB TebBaseAddress;
    CLIENT_ID ClientId;
    KAFFINITY AffinityMask;
    KPRIORITY Priority;
    KPRIORITY BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

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

/**
 * The THREAD_CYCLE_TIME_INFORMATION structure contains information about the cycle time of a thread.
 */
typedef struct _THREAD_CYCLE_TIME_INFORMATION
{
    ULONGLONG AccumulatedCycles;        // The total number of cycles accumulated by the thread.
    ULONGLONG CurrentCycleCount;        // The current cycle count of the thread.
} THREAD_CYCLE_TIME_INFORMATION, *PTHREAD_CYCLE_TIME_INFORMATION;

// RtlAbPostRelease / ReleaseAllUserModeAutoBoostLockHandles
typedef struct _THREAD_LOCK_OWNERSHIP
{
    ULONG SrwLock[1];
} THREAD_LOCK_OWNERSHIP, *PTHREAD_LOCK_OWNERSHIP;

typedef enum _SCHEDULER_SHARED_DATA_SLOT_ACTION
{
    SchedulerSharedSlotAssign,
    SchedulerSharedSlotFree,
    SchedulerSharedSlotQuery
} SCHEDULER_SHARED_DATA_SLOT_ACTION;

typedef struct _SCHEDULER_SHARED_DATA_SLOT_INFORMATION
{
    SCHEDULER_SHARED_DATA_SLOT_ACTION Action;
    PVOID SchedulerSharedDataHandle;
    PVOID Slot;
} SCHEDULER_SHARED_DATA_SLOT_INFORMATION, *PSCHEDULER_SHARED_DATA_SLOT_INFORMATION;

typedef struct _THREAD_TEB_INFORMATION
{
    _Inout_bytecount_(BytesToRead) PVOID TebInformation; // Buffer to write data into.
    _In_ ULONG TebOffset;                                // Offset in TEB to begin reading from.
    _In_ ULONG BytesToRead;                              // Number of bytes to read.
} THREAD_TEB_INFORMATION, *PTHREAD_TEB_INFORMATION;

/**
 * The COUNTER_READING structure is used to store individual counter data from a hardware counter.
 *
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-hardware_counter_data
 */
typedef struct _COUNTER_READING
{
    HARDWARE_COUNTER_TYPE Type;     // Specifies the type of hardware counter data collected.
    ULONG Index;                    // An identifier for the specific counter.
    ULONG64 Start;                  // The initial value of the counter when measurement started.
    ULONG64 Total;                  // The accumulated value of the counter over the measurement period.
} COUNTER_READING, *PCOUNTER_READING;

#ifndef THREAD_PERFORMANCE_DATA_VERSION
#define THREAD_PERFORMANCE_DATA_VERSION 1
#endif

/**
 * The THREAD_PERFORMANCE_DATA structure aggregates various performance metrics for a thread.
 *
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-performance_data
 */
typedef struct _THREAD_PERFORMANCE_DATA
{
    USHORT Size;                                    // The size of the structure.
    USHORT Version;                                 // The version of the structure. Must be set to \ref THREAD_PERFORMANCE_DATA_VERSION.
    PROCESSOR_NUMBER ProcessorNumber;               // The processor number that identifies where the thread is running.
    ULONG ContextSwitches;                          // The number of context switches that occurred from the time profiling was enabled.
    ULONG HwCountersCount;                          // The number of array elements in the HwCounters array that contain hardware counter data.
    ULONG64 UpdateCount;                            // The number of times that the read operation read the data to ensure a consistent snapshot of the data.
    ULONG64 WaitReasonBitMap;                       // A bitmask of \ref KWAIT_REASON that identifies the reasons for the context switches that occurred since the last time the data was read.
    ULONG64 HardwareCounters;                       // A bitmask of hardware counters used to collect counter data.
    COUNTER_READING CycleTime;                      // The cycle time of the thread (excludes the time spent interrupted) from the time profiling was enabled.
    COUNTER_READING HwCounters[MAX_HW_COUNTERS];    // The \ref COUNTER_READING structure that contains hardware counter data.
} THREAD_PERFORMANCE_DATA, *PTHREAD_PERFORMANCE_DATA;

#ifndef THREAD_PROFILING_FLAG_DISPATCH
#define THREAD_PROFILING_FLAG_DISPATCH 0x00000001
#endif

#ifndef THREAD_PROFILING_FLAG_HARDWARE_COUNTERS
#define THREAD_PROFILING_FLAG_HARDWARE_COUNTERS 0x00000002
#endif

/**
 * The THREAD_PROFILING_INFORMATION structure contains profiling information and references to performance data.
 *
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-readthreadprofilingdata
 */
typedef struct _THREAD_PROFILING_INFORMATION
{
    // To receive hardware performance counter data, set this parameter to a bitmask that identifies the hardware counters to collect.
    // You can specify up to 16 performance counters. Each bit relates directly to the zero-based hardware counter index for the hardware
    // performance counters that you configured. Set to zero if you are not collecting hardware counter data.
    // If you set a bit for a hardware counter that has not been configured, the counter value that is read for that counter is zero.
    ULONG64 HardwareCounters;
    // To receive thread profiling data such as context switch count, set this parameter to \ref THREAD_PROFILING_FLAG_DISPATCH.
    ULONG Flags;
    // Enable or disable thread profiling on the specified thread.
    ULONG Enable;
    // The PERFORMANCE_DATA structure that contains thread profiling and hardware counter data.
    PTHREAD_PERFORMANCE_DATA PerformanceData;
} THREAD_PROFILING_INFORMATION, *PTHREAD_PROFILING_INFORMATION;

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

typedef enum _THREAD_UMS_INFORMATION_COMMAND
{
    UmsInformationCommandInvalid,
    UmsInformationCommandAttach,
    UmsInformationCommandDetach,
    UmsInformationCommandQuery
} THREAD_UMS_INFORMATION_COMMAND;

typedef struct _RTL_UMS_COMPLETION_LIST
{
    PSINGLE_LIST_ENTRY ThreadListHead;
    PVOID CompletionEvent;
    ULONG CompletionFlags;
    SINGLE_LIST_ENTRY InternalListHead;
} RTL_UMS_COMPLETION_LIST, *PRTL_UMS_COMPLETION_LIST;

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

/**
 * The THREAD_NAME_INFORMATION structure assigns a description to a thread.
 *
 * @remarks The handle must have THREAD_SET_LIMITED_INFORMATION access.
 * @remarks https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreaddescription
 */
typedef struct _THREAD_NAME_INFORMATION
{
    UNICODE_STRING ThreadName;
} THREAD_NAME_INFORMATION, *PTHREAD_NAME_INFORMATION;

typedef struct _ALPC_WORK_ON_BEHALF_TICKET
{
    ULONG ThreadId;
    ULONG ThreadCreationTimeLow;
} ALPC_WORK_ON_BEHALF_TICKET, *PALPC_WORK_ON_BEHALF_TICKET;

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
typedef enum _SUBSYSTEM_INFORMATION_TYPE
{
    SubsystemInformationTypeWin32,
    SubsystemInformationTypeWSL,
    MaxSubsystemInformationType
} SUBSYSTEM_INFORMATION_TYPE;
#endif // PHNT_MODE != PHNT_MODE_KERNEL

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

#endif // _ARM64_

// private
typedef struct _THREAD_INDEX_INFORMATION
{
    ULONG Index;
    ULONG Sequence;
} THREAD_INDEX_INFORMATION, *PTHREAD_INDEX_INFORMATION;

//
// Processes
//

#if (PHNT_MODE != PHNT_MODE_KERNEL)

/**
 * Creates a new process.
 *
 * @param ProcessHandle A pointer to a handle that receives the process object handle.
 * @param DesiredAccess The access rights desired for the process object.
 * @param ObjectAttributes Optional. A pointer to an OBJECT_ATTRIBUTES structure that specifies the attributes of the new process.
 * @param ParentProcess A handle to the parent process.
 * @param InheritObjectTable If TRUE, the new process inherits the object table of the parent process.
 * @param SectionHandle Optional. A handle to a section object to be used for the new process.
 * @param DebugPort Optional. A handle to a debug port to be used for the new process.
 * @param TokenHandle Optional. A handle to an access token to be used for the new process.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ParentProcess,
    _In_ BOOLEAN InheritObjectTable,
    _In_opt_ HANDLE SectionHandle,
    _In_opt_ HANDLE DebugPort,
    _In_opt_ HANDLE TokenHandle
    );

// begin_rev
#define PROCESS_CREATE_FLAGS_NONE 0x00000000
#define PROCESS_CREATE_FLAGS_BREAKAWAY 0x00000001 // NtCreateProcessEx & NtCreateUserProcess
#define PROCESS_CREATE_FLAGS_NO_DEBUG_INHERIT 0x00000002 // NtCreateProcessEx & NtCreateUserProcess
#define PROCESS_CREATE_FLAGS_INHERIT_HANDLES 0x00000004 // NtCreateProcessEx & NtCreateUserProcess
#define PROCESS_CREATE_FLAGS_OVERRIDE_ADDRESS_SPACE 0x00000008 // NtCreateProcessEx only
#define PROCESS_CREATE_FLAGS_LARGE_PAGES 0x00000010 // NtCreateProcessEx only (requires SeLockMemoryPrivilege)
#define PROCESS_CREATE_FLAGS_LARGE_PAGE_SYSTEM_DLL 0x00000020 // NtCreateProcessEx only (requires SeLockMemoryPrivilege)
#define PROCESS_CREATE_FLAGS_PROTECTED_PROCESS 0x00000040 // NtCreateUserProcess only
#define PROCESS_CREATE_FLAGS_CREATE_SESSION 0x00000080 // NtCreateProcessEx & NtCreateUserProcess (requires SeLoadDriverPrivilege)
#define PROCESS_CREATE_FLAGS_INHERIT_FROM_PARENT 0x00000100 // NtCreateProcessEx & NtCreateUserProcess
#define PROCESS_CREATE_FLAGS_CREATE_SUSPENDED 0x00000200 // NtCreateProcessEx & NtCreateUserProcess
#define PROCESS_CREATE_FLAGS_FORCE_BREAKAWAY 0x00000400 // NtCreateProcessEx & NtCreateUserProcess (requires SeTcbPrivilege)
#define PROCESS_CREATE_FLAGS_MINIMAL_PROCESS 0x00000800 // NtCreateProcessEx only
#define PROCESS_CREATE_FLAGS_RELEASE_SECTION 0x00001000 // NtCreateProcessEx & NtCreateUserProcess
#define PROCESS_CREATE_FLAGS_CLONE_MINIMAL 0x00002000 // NtCreateProcessEx only
#define PROCESS_CREATE_FLAGS_CLONE_MINIMAL_REDUCED_COMMIT 0x00004000
#define PROCESS_CREATE_FLAGS_AUXILIARY_PROCESS 0x00008000 // NtCreateProcessEx & NtCreateUserProcess (requires SeTcbPrivilege)
#define PROCESS_CREATE_FLAGS_CREATE_STORE 0x00020000 // NtCreateProcessEx & NtCreateUserProcess
#define PROCESS_CREATE_FLAGS_USE_PROTECTED_ENVIRONMENT 0x00040000 // NtCreateProcessEx & NtCreateUserProcess
#define PROCESS_CREATE_FLAGS_IMAGE_EXPANSION_MITIGATION_DISABLE 0x00080000
#define PROCESS_CREATE_FLAGS_PARTITION_CREATE_SLAB_IDENTITY 0x00400000 // NtCreateProcessEx & NtCreateUserProcess (requires SeLockMemoryPrivilege)
// end_rev

/**
 * Creates a new process with extended options.
 *
 * @param ProcessHandle A pointer to a handle that receives the process object handle.
 * @param DesiredAccess The access rights desired for the process object.
 * @param ObjectAttributes Optional. A pointer to an OBJECT_ATTRIBUTES structure that specifies the attributes of the new process.
 * @param ParentProcess A handle to the parent process.
 * @param Flags Flags that control the creation of the process. These flags are defined as PROCESS_CREATE_FLAGS_*.
 * @param SectionHandle Optional. A handle to a section object to be used for the new process.
 * @param DebugPort Optional. A handle to a debug port to be used for the new process.
 * @param TokenHandle Optional. A handle to an access token to be used for the new process.
 * @param Reserved Reserved for future use. Must be zero.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateProcessEx(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ParentProcess,
    _In_ ULONG Flags, // PROCESS_CREATE_FLAGS_*
    _In_opt_ HANDLE SectionHandle,
    _In_opt_ HANDLE DebugPort,
    _In_opt_ HANDLE TokenHandle,
    _Reserved_ ULONG Reserved // JobMemberLevel
    );

/**
 * Opens an existing process object.
 *
 * @param ProcessHandle A pointer to a handle that receives the process object handle.
 * @param DesiredAccess The access rights desired for the process object.
 * @param ObjectAttributes A pointer to an OBJECT_ATTRIBUTES structure that specifies the attributes of the new process.
 * @param ClientId Optional. A pointer to a CLIENT_ID structure that specifies the client ID of the process to be opened.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PCLIENT_ID ClientId
    );

/**
 * Terminates the specified process.
 *
 * @param ProcessHandle Optional. A handle to the process to be terminated. If this parameter is NULL, the calling process is terminated.
 * @param ExitStatus The exit status to be used by the process and the process's termination status.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtTerminateProcess(
    _In_opt_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus
    );

/**
 * Suspends the specified process.
 *
 * @param ProcessHandle A handle to the process to be suspended.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSuspendProcess(
    _In_ HANDLE ProcessHandle
    );

/**
 * Resumes the specified process.
 *
 * @param ProcessHandle A handle to the process to be resumed.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtResumeProcess(
    _In_ HANDLE ProcessHandle
    );

//
// Macros
//

#define NtCurrentProcess() ((HANDLE)(LONG_PTR)-1)
#define ZwCurrentProcess() NtCurrentProcess()
#define NtCurrentThread() ((HANDLE)(LONG_PTR)-2)
#define ZwCurrentThread() NtCurrentThread()
#define NtCurrentSession() ((HANDLE)(LONG_PTR)-3)
#define ZwCurrentSession() NtCurrentSession()

#define NtCurrentPeb() (NtCurrentTeb()->ProcessEnvironmentBlock)

#define NtCurrentProcessId() (NtCurrentTeb()->ClientId.UniqueProcess)
#define NtCurrentThreadId() (NtCurrentTeb()->ClientId.UniqueThread)

// Windows 8 and above
#define NtCurrentProcessToken() ((HANDLE)(LONG_PTR)-4) // NtOpenProcessToken(NtCurrentProcess())
#define NtCurrentThreadToken() ((HANDLE)(LONG_PTR)-5) // NtOpenThreadToken(NtCurrentThread())
#define NtCurrentThreadEffectiveToken() ((HANDLE)(LONG_PTR)-6) // NtOpenThreadToken(NtCurrentThread()) + NtOpenProcessToken(NtCurrentProcess())
#define NtCurrentSilo() ((HANDLE)(LONG_PTR)-1)

EXTERN_C CONST IMAGE_DOS_HEADER __ImageBase;
#define NtCurrentImageBase() ((PIMAGE_DOS_HEADER)&__ImageBase)

#define NtCurrentSessionId() (RtlGetActiveConsoleId()) // USER_SHARED_DATA->ActiveConsoleId
#define NtCurrentLogonId() (NtCurrentPeb()->LogonId)

/**
 * Retrieves information about the specified process.
 *
 * @param ProcessHandle A handle to the process.
 * @param ProcessInformationClass The type of process information to be retrieved.
 * @param ProcessInformation A pointer to a buffer that receives the process information.
 * @param ProcessInformationLength The size of the buffer pointed to by the ProcessInformation parameter.
 * @param ReturnLength An optional pointer to a variable that receives the size of the data returned.
 * @return NTSTATUS Successful or errant status.
 */
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

// rev
NTSYSCALLAPI
NTSTATUS
NTAPI
NtWow64QueryInformationProcess64(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_writes_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength
    );

/**
 * Sets information for the specified process.
 *
 * @param ProcessHandle A handle to the process.
 * @param ProcessInformationClass The type of process information to be set.
 * @param ProcessInformation A pointer to a buffer that contains the process information.
 * @param ProcessInformationLength The size of the buffer pointed to by the ProcessInformation parameter.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _In_reads_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength
    );

#define PROCESS_GET_NEXT_FLAGS_PREVIOUS_PROCESS 0x00000001

#if (PHNT_VERSION >= PHNT_WINDOWS_SERVER_2003)
/**
 * Retrieves a handle to the next process in the system.
 *
 * @param ProcessHandle An optional handle to a process. If this parameter is NULL, the function retrieves the first process in the system.
 * @param DesiredAccess The access rights desired for the new process handle.
 * @param HandleAttributes The attributes for the new process handle.
 * @param Flags Flags that modify the behavior of the function. This can be a combination of the following flags:
 * - \ref PROCESS_GET_NEXT_FLAGS_PREVIOUS_PROCESS (0x00000001): Retrieve the previous process in the system.
 * @param NewProcessHandle A pointer to a variable that receives the handle to the next process.
 * @return NTSTATUS Successful or errant status.
 */
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

/**
 * Retrieves a handle to the next thread in the system.
 *
 * @param ProcessHandle A handle to the process for enumerateration of threads.
 * @param ThreadHandle An optional handle to a thread. If this parameter is NULL, the function retrieves the first thread in the process.
 * @param DesiredAccess The access rights desired for the new thread handle.
 * @param HandleAttributes The attributes for the new thread handle.
 * @param Flags Flags that modify the behavior of the function. This can be a combination of the following flags:
 * - \ref THREAD_GET_NEXT_FLAGS_PREVIOUS_THREAD (0x00000001): Retrieve the previous thread in the process.
 * @param NewThreadHandle A pointer to a variable that receives the handle to the next thread.
 * @return NTSTATUS Successful or errant status.
 */
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
#endif // PHNT_VERSION >= PHNT_WINDOWS_SERVER_2003

#endif // PHNT_MODE != PHNT_MODE_KERNEL

#define STATECHANGE_SET_ATTRIBUTES 0x0001

typedef enum _PROCESS_STATE_CHANGE_TYPE
{
    ProcessStateChangeSuspend,
    ProcessStateChangeResume,
    ProcessStateChangeMax,
} PROCESS_STATE_CHANGE_TYPE, *PPROCESS_STATE_CHANGE_TYPE;

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * Creates a state change handle for changing the suspension state of a process.
 *
 * @param ProcessStateChangeHandle A pointer to a variable that receives the handle.
 * @param DesiredAccess The access rights desired for the handle.
 * @param ObjectAttributes Optional attributes for the handle.
 * @param ProcessHandle A handle to the process.
 * @param Reserved Reserved for future use.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateProcessStateChange(
    _Out_ PHANDLE ProcessStateChangeHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ProcessHandle,
    _In_opt_ _Reserved_ ULONG64 Reserved
    );

/**
 * Changes the suspension state of a process.
 *
 * @param ProcessStateChangeHandle A handle to the process state change object.
 * @param ProcessHandle A handle to the process.
 * @param StateChangeType The type of state change.
 * @param ExtendedInformation Optional extended information.
 * @param ExtendedInformationLength The length of the extended information.
 * @param Reserved Reserved for future use.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtChangeProcessState(
    _In_ HANDLE ProcessStateChangeHandle,
    _In_ HANDLE ProcessHandle,
    _In_ PROCESS_STATE_CHANGE_TYPE StateChangeType,
    _In_opt_ _Reserved_ PVOID ExtendedInformation,
    _In_opt_ _Reserved_ SIZE_T ExtendedInformationLength,
    _In_opt_ _Reserved_ ULONG64 Reserved
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11

typedef enum _THREAD_STATE_CHANGE_TYPE
{
    ThreadStateChangeSuspend,
    ThreadStateChangeResume,
    ThreadStateChangeMax,
} THREAD_STATE_CHANGE_TYPE, *PTHREAD_STATE_CHANGE_TYPE;

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * Creates a state change handle for changing the suspension state of a thread.
 *
 * @param ThreadStateChangeHandle A pointer to a variable that receives the handle.
 * @param DesiredAccess The access rights desired for the handle.
 * @param ObjectAttributes Optional attributes for the handle.
 * @param ThreadHandle A handle to the thread.
 * @param Reserved Reserved for future use.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateThreadStateChange(
    _Out_ PHANDLE ThreadStateChangeHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ThreadHandle,
    _In_opt_ ULONG64 Reserved
    );

/**
 * Changes the suspension state of a thread.
 *
 * @param ThreadStateChangeHandle A handle to the thread state change object.
 * @param ThreadHandle A handle to the thread.
 * @param StateChangeType The type of state change.
 * @param ExtendedInformation Optional extended information.
 * @param ExtendedInformationLength The length of the extended information.
 * @param Reserved Reserved for future use.
 * @return NTSTATUS Successful or errant status.
 */
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
#endif // PHNT_VERSION >= PHNT_WINDOWS_11

//
// Threads
//

#if (PHNT_MODE != PHNT_MODE_KERNEL)
/**
 * Creates a new thread in the specified process.
 *
 * @param ThreadHandle A pointer to a handle that receives the thread object handle.
 * @param DesiredAccess The access rights desired for the thread object.
 * @param ObjectAttributes Optional. A pointer to an OBJECT_ATTRIBUTES structure that specifies the attributes of the new thread.
 * @param ProcessHandle A handle to the process in which the thread is to be created.
 * @param ClientId A pointer to a CLIENT_ID structure that receives the client ID of the new thread.
 * @param ThreadContext A pointer to a CONTEXT structure that specifies the initial context of the new thread.
 * @param InitialTeb A pointer to an INITIAL_TEB structure that specifies the initial stack limits of the new thread.
 * @param CreateSuspended If TRUE, the thread is created in a suspended state.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ProcessHandle,
    _Out_ PCLIENT_ID ClientId,
    _In_ PCONTEXT ThreadContext,
    _In_ PINITIAL_TEB InitialTeb,
    _In_ BOOLEAN CreateSuspended
    );

/**
 * Opens an existing thread object.
 *
 * @param ThreadHandle A pointer to a handle that receives the thread object handle.
 * @param DesiredAccess The access rights desired for the thread object.
 * @param ObjectAttributes Optional. A pointer to an OBJECT_ATTRIBUTES structure that specifies the attributes of the new thread.
 * @param ClientId Optional. A pointer to a CLIENT_ID structure that specifies the client ID of the thread to be opened.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenThread(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PCLIENT_ID ClientId
    );

/**
 * Terminates the specified thread.
 *
 * @param ThreadHandle Optional. A handle to the thread to be terminated. If this parameter is NULL, the calling thread is terminated.
 * @param ExitStatus The exit status to be used by the thread and the thread's termination status.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtTerminateThread(
    _In_opt_ HANDLE ThreadHandle,
    _In_ NTSTATUS ExitStatus
    );

/**
 * Suspends the specified thread.
 *
 * @param ThreadHandle A handle to the thread to be suspended.
 * @param PreviousSuspendCount Optional. A pointer to a variable that receives the thread's previous suspend count.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSuspendThread(
    _In_ HANDLE ThreadHandle,
    _Out_opt_ PULONG PreviousSuspendCount
    );

/**
 * Resumes the specified thread.
 *
 * @param ThreadHandle A handle to the thread to be resumed.
 * @param PreviousSuspendCount Optional. A pointer to a variable that receives the thread's previous suspend count.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtResumeThread(
    _In_ HANDLE ThreadHandle,
    _Out_opt_ PULONG PreviousSuspendCount
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_SERVER_2003)
/**
 * Retrieves the number of the current processor.
 *
 * @return ULONG The number of the current processor.
 */
NTSYSCALLAPI
ULONG
NTAPI
NtGetCurrentProcessorNumber(
    VOID
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_SERVER_2003

#if (PHNT_VERSION >= PHNT_WINDOWS_7)
/**
 * Retrieves the number of the current processor.
 *
 * @param ProcessorNumber An optional pointer to a PROCESSOR_NUMBER structure that receives the processor number.
 * @return ULONG The number of the current processor.
 */
NTSYSCALLAPI
ULONG
NTAPI
NtGetCurrentProcessorNumberEx(
    _Out_opt_ PPROCESSOR_NUMBER ProcessorNumber
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_7

/**
 * Retrieves the context of the specified thread.
 *
 * @param ThreadHandle A handle to the thread.
 * @param ThreadContext A pointer to a CONTEXT structure that receives the thread context.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetContextThread(
    _In_ HANDLE ThreadHandle,
    _Inout_ PCONTEXT ThreadContext
    );

/**
 * Sets the context of the specified thread.
 *
 * @param ThreadHandle A handle to the thread.
 * @param ThreadContext A pointer to a CONTEXT structure that specifies the thread context.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetContextThread(
    _In_ HANDLE ThreadHandle,
    _In_ PCONTEXT ThreadContext
    );
/**
 * Retrieves information about the specified thread.
 *
 * @param ThreadHandle A handle to the thread.
 * @param ThreadInformationClass The type of thread information to be retrieved.
 * @param ThreadInformation A pointer to a buffer that receives the thread information.
 * @param ThreadInformationLength The size of the buffer pointed to by the ThreadInformation parameter.
 * @param ReturnLength An optional pointer to a variable that receives the size of the data returned.
 * @return NTSTATUS Successful or errant status.
 */
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

/**
 * Sets information for the specified thread.
 *
 * @param ThreadHandle A handle to the thread.
 * @param ThreadInformationClass The type of thread information to be set.
 * @param ThreadInformation A pointer to a buffer that contains the thread information.
 * @param ThreadInformationLength The size of the buffer pointed to by the ThreadInformation parameter.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ THREADINFOCLASS ThreadInformationClass,
    _In_reads_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength
    );

/**
 * Sends an alert to the specified thread.
 *
 * @param ThreadHandle A handle to the thread to be alerted.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlertThread(
    _In_ HANDLE ThreadHandle
    );

/**
 * Resumes a thread that was previously suspended and sends an alert to it.
 *
 * @param ThreadHandle A handle to the thread to be resumed and alerted.
 * @param PreviousSuspendCount An optional pointer to a variable that receives the thread's previous suspend count.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlertResumeThread(
    _In_ HANDLE ThreadHandle,
    _Out_opt_ PULONG PreviousSuspendCount
    );

/**
 * Tests whether the current thread has an alert pending.
 *
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtTestAlert(
    VOID
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// rev
/**
 * Sends an alert to the specified thread.
 *
 * @param ThreadId The thread ID of the thread to be alerted.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlertThreadByThreadId(
    _In_ HANDLE ThreadId
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * Sends an alert to the specified thread by its thread ID, with an optional lock.
 *
 * @param ThreadId The thread ID of the thread to be alerted.
 * @param Lock An optional pointer to an SRW lock to be used during the alert.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlertThreadByThreadIdEx(
    _In_ HANDLE ThreadId,
    _In_opt_ PRTL_SRWLOCK Lock
    );

/**
 * Sends an alert to multiple threads by their thread IDs.
 *
 * @param MultipleThreadId A pointer to an array of thread IDs to be alerted.
 * @param Count The number of thread IDs in the array.
 * @param Boost A pointer to a boost value to be applied to the threads.
 * @param BoostCount The number of boost values in the array.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlertMultipleThreadByThreadId(
    _In_ PHANDLE MultipleThreadId,
    _In_ ULONG Count,
    _In_ PVOID Boost,
    _In_ ULONG BoostCount
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// rev
/**
 * Waits for an alert to be delivered to the specified thread.
 *
 * @param Address The address to wait for an alert on.
 * @param Timeout The timeout value for waiting, or NULL for no timeout.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForAlertByThreadId(
    _In_opt_ PVOID Address,
    _In_opt_ PLARGE_INTEGER Timeout
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8

/**
 * Impersonates a client thread.
 *
 * @param ServerThreadHandle A handle to the server thread.
 * @param ClientThreadHandle A handle to the client thread.
 * @param SecurityQos A pointer to a SECURITY_QUALITY_OF_SERVICE structure that specifies the impersonation level and context tracking mode.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtImpersonateThread(
    _In_ HANDLE ServerThreadHandle,
    _In_ HANDLE ClientThreadHandle,
    _In_ PSECURITY_QUALITY_OF_SERVICE SecurityQos
    );

/**
 * Registers a thread termination port.
 *
 * @param PortHandle A handle to the port to be registered.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRegisterThreadTerminatePort(
    _In_ HANDLE PortHandle
    );

/**
 * Sets LDT (Local Descriptor Table) entries.
 *
 * @param Selector0 The first selector.
 * @param Entry0Low The low part of the first entry.
 * @param Entry0Hi The high part of the first entry.
 * @param Selector1 The second selector.
 * @param Entry1Low The low part of the second entry.
 * @param Entry1Hi The high part of the second entry.
 * @return NTSTATUS Successful or errant status.
 */
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

/**
 * Dispatches the Asynchronous Procedure Call (APC) from the NtQueueApc* functions to the specified routine.
 *
 * @param ApcRoutine A pointer to the APC routine to be executed.
 * @param Parameter Optional. A pointer to a parameter to be passed to the APC routine.
 * @param ActxContext Optional. A handle to an activation context.
 */
NTSYSAPI
VOID
NTAPI
RtlDispatchAPC(
    _In_ PAPCFUNC ApcRoutine,
    _In_opt_ PVOID Parameter,
    _In_opt_ HANDLE ActxContext
    );

/**
 * A pointer to a function that serves as an APC routine.
 *
 * @param ApcArgument1 Optional. A pointer to the first argument to be passed to the APC routine.
 * @param ApcArgument2 Optional. A pointer to the second argument to be passed to the APC routine.
 * @param ApcArgument3 Optional. A pointer to the third argument to be passed to the APC routine.
 */
typedef VOID (NTAPI* PPS_APC_ROUTINE)(
    _In_opt_ PVOID ApcArgument1,
    _In_opt_ PVOID ApcArgument2,
    _In_opt_ PVOID ApcArgument3
    );

/**
 * Encodes an APC routine pointer for use in a WOW64 environment.
 *
 * @param ApcRoutine The APC routine pointer to be encoded.
 * @return PVOID The encoded APC routine pointer.
 */
#define Wow64EncodeApcRoutine(ApcRoutine) \
    ((PVOID)((0 - ((LONG_PTR)(ApcRoutine))) << 2))

/**
 * Decodes an APC routine pointer that was encoded for use in a WOW64 environment.
 *
 * @param ApcRoutine The encoded APC routine pointer to be decoded.
 * @return PVOID The decoded APC routine pointer.
 */
#define Wow64DecodeApcRoutine(ApcRoutine) \
    ((PVOID)(0 - (((LONG_PTR)(ApcRoutine)) >> 2)))

/**
 * Queues an APC (Asynchronous Procedure Call) to a thread.
 *
 * @param ThreadHandle Handle to the thread to which the APC is to be queued.
 * @param ApcRoutine A pointer to the RtlDispatchAPC function or custom APC routine to be executed.
 * @param ApcArgument1 Optional first argument to be passed to the APC routine.
 * @param ApcArgument2 Optional second argument to be passed to the APC routine.
 * @param ApcArgument3 Optional third argument to be passed to the APC routine.
 * @return NTSTATUS Successful or errant status.
 * @remarks The APC will be executed in the context of the specified thread when the thread enters an alertable wait state or when any
 * process calls the NtTestAlert, NtAlertThread, NtAlertResumeThread or NtAlertThreadByThreadId functions.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueueApcThread(
    _In_ HANDLE ThreadHandle,
    _In_ PPS_APC_ROUTINE ApcRoutine, // RtlDispatchAPC
    _In_opt_ PVOID ApcArgument1,
    _In_opt_ PVOID ApcArgument2,
    _In_opt_ PVOID ApcArgument3
    );

/**
 * A special handle value used to queue a user APC (Asynchronous Procedure Call).
 */
#define QUEUE_USER_APC_SPECIAL_USER_APC ((HANDLE)0x1)

#if (PHNT_VERSION >= PHNT_WINDOWS_7)
/**
 * Queues an APC (Asynchronous Procedure Call) to a thread.
 *
 * @param ThreadHandle Handle to the thread to which the APC is to be queued.
 * @param ReserveHandle Optional handle to a reserve object. This can be QUEUE_USER_APC_SPECIAL_USER_APC or a handle returned by NtAllocateReserveObject.
 * @param ApcRoutine A pointer to the RtlDispatchAPC function or custom APC routine to be executed.
 * @param ApcArgument1 Optional first argument to be passed to the APC routine.
 * @param ApcArgument2 Optional second argument to be passed to the APC routine.
 * @param ApcArgument3 Optional third argument to be passed to the APC routine.
 * @return NTSTATUS Successful or errant status.
 * @remarks The APC will be executed in the context of the specified thread after the thread enters an alertable wait state or immediately
 * when QUEUE_USER_APC_SPECIAL_USER_APC is used or NtTestAlert, NtAlertThread, NtAlertResumeThread or NtAlertThreadByThreadId are called.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueueApcThreadEx(
    _In_ HANDLE ThreadHandle,
    _In_opt_ HANDLE ReserveHandle, // NtAllocateReserveObject // QUEUE_USER_APC_SPECIAL_USER_APC
    _In_ PPS_APC_ROUTINE ApcRoutine, // RtlDispatchAPC
    _In_opt_ PVOID ApcArgument1,
    _In_opt_ PVOID ApcArgument2,
    _In_opt_ PVOID ApcArgument3
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_7

/**
 * The APC_CALLBACK_DATA_CONTEXT structure is used to pass information to the APC callback routine.
 */
typedef struct _APC_CALLBACK_DATA_CONTEXT
{
    ULONG_PTR Parameter;
    PCONTEXT ContextRecord;
    ULONG_PTR Reserved0;
    ULONG_PTR Reserved1;
} APC_CALLBACK_DATA_CONTEXT, *PAPC_CALLBACK_DATA_CONTEXT;

#define QUEUE_USER_APC_FLAGS_NONE 0x00000000
#define QUEUE_USER_APC_FLAGS_SPECIAL_USER_APC 0x00000001
#define QUEUE_USER_APC_FLAGS_CALLBACK_DATA_CONTEXT 0x00010000 // APC_CALLBACK_DATA_CONTEXT

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * Queues an Asynchronous Procedure Call (APC) to a specified thread.
 *
 * @param ThreadHandle A handle to the thread to which the APC is to be queued.
 * @param ReserveHandle An optional handle to a reserve object. This can be obtained using NtAllocateReserveObject.
 * @param ApcFlags Flags that control the behavior of the APC. These flags are defined in QUEUE_USER_APC_FLAGS.
 * @param ApcRoutine A pointer to the RtlDispatchAPC function or custom APC routine to be executed.
 * @param ApcArgument1 An optional argument to be passed to the APC routine.
 * @param ApcArgument2 An optional argument to be passed to the APC routine.
 * @param ApcArgument3 An optional argument to be passed to the APC routine.
 * @return NTSTATUS Successful or errant status.
 * @remarks The APC will be executed in the context of the specified thread when the thread enters an alertable wait state or immediately
 * when QUEUE_USER_APC_SPECIAL_USER_APC is used or any process calls the NtTestAlert, NtAlertThread,
 * NtAlertResumeThread or NtAlertThreadByThreadId functions.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueueApcThreadEx2(
    _In_ HANDLE ThreadHandle,
    _In_opt_ HANDLE ReserveHandle, // NtAllocateReserveObject
    _In_ ULONG ApcFlags, // QUEUE_USER_APC_FLAGS
    _In_ PPS_APC_ROUTINE ApcRoutine, // RtlDispatchAPC
    _In_opt_ PVOID ApcArgument1,
    _In_opt_ PVOID ApcArgument2,
    _In_opt_ PVOID ApcArgument3
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_11

#endif // PHNT_MODE != PHNT_MODE_KERNEL

//
// User processes and threads
//

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
#define ProcThreadAttributeSveVectorLength 30

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
#ifndef PROC_THREAD_ATTRIBUTE_TRUSTED_APP
#define PROC_THREAD_ATTRIBUTE_TRUSTED_APP \
    ProcThreadAttributeValue(ProcThreadAttributeTrustedApp, FALSE, TRUE, FALSE)
#endif

// private
typedef struct _PROC_THREAD_ATTRIBUTE
{
    ULONG_PTR Attribute;
    SIZE_T Size;
    ULONG_PTR Value;
} PROC_THREAD_ATTRIBUTE, *PPROC_THREAD_ATTRIBUTE;

/**
 * The PROC_THREAD_ATTRIBUTE_LIST structure contains the list of attributes for process and thread creation.
 */
typedef struct _PROC_THREAD_ATTRIBUTE_LIST
{
    ULONG PresentFlags;             // A bitmask of flags that indicate the attributes for process and thread creation.
    ULONG AttributeCount;           // The number of attributes in the list.
    ULONG LastAttribute;            // The index of the last attribute in the list.
    ULONG SpareUlong0;              // Reserved for future use.
    PPROC_THREAD_ATTRIBUTE ExtendedFlagsAttribute; // A pointer to the extended flags attribute.
    _Field_size_(AttributeCount) PROC_THREAD_ATTRIBUTE Attributes[1]; // An array of attributes.
} PROC_THREAD_ATTRIBUTE_LIST, *PPROC_THREAD_ATTRIBUTE_LIST;

// private
#define EXTENDED_PROCESS_CREATION_FLAG_ELEVATION_HANDLED 0x00000001
#define EXTENDED_PROCESS_CREATION_FLAG_FORCELUA 0x00000002
#define EXTENDED_PROCESS_CREATION_FLAG_FORCE_BREAKAWAY 0x00000004 // requires SeTcbPrivilege // since WINBLUE

#define PROTECTION_LEVEL_WINTCB_LIGHT       0x00000000
#define PROTECTION_LEVEL_WINDOWS            0x00000001
#define PROTECTION_LEVEL_WINDOWS_LIGHT      0x00000002
#define PROTECTION_LEVEL_ANTIMALWARE_LIGHT  0x00000003
#define PROTECTION_LEVEL_LSA_LIGHT          0x00000004
#define PROTECTION_LEVEL_WINTCB             0x00000005
#define PROTECTION_LEVEL_CODEGEN_LIGHT      0x00000006
#define PROTECTION_LEVEL_AUTHENTICODE       0x00000007
#define PROTECTION_LEVEL_PPL_APP            0x00000008

#define PROTECTION_LEVEL_SAME               0xFFFFFFFF
#define PROTECTION_LEVEL_NONE               0xFFFFFFFE

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
    SeSafeOpenExperienceMOTWPresent = 0x80,
    SeSafeOpenExperienceElevatedNoPropagation = 0x100
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
    PsAttributeSafeOpenPromptOriginClaim, // in SE_SAFE_OPEN_PROMPT_RESULTS
    PsAttributeBnoIsolation, // in PPS_BNO_ISOLATION_PARAMETERS // since REDSTONE2
    PsAttributeDesktopAppPolicy, // in PULONG (PROCESS_CREATION_DESKTOP_APP_*)
    PsAttributeChpe, // in BOOLEAN // since REDSTONE3
    PsAttributeMitigationAuditOptions, // in PPS_MITIGATION_AUDIT_OPTIONS_MAP (PROCESS_CREATION_MITIGATION_AUDIT_POLICY_*) // since 21H1
    PsAttributeMachineType, // in USHORT // since 21H2
    PsAttributeComponentFilter,
    PsAttributeEnableOptionalXStateFeatures, // since WIN11
    PsAttributeSupportedMachines, // since 24H2
    PsAttributeSveVectorLength, // PPS_PROCESS_CREATION_SVE_VECTOR_LENGTH
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
typedef union _PS_PROCESS_CREATION_SVE_VECTOR_LENGTH
{
    ULONG VectorLength : 24;
    ULONG FlagsReserved : 8;
} PS_PROCESS_CREATION_SVE_VECTOR_LENGTH, *PPS_PROCESS_CREATION_SVE_VECTOR_LENGTH;

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
    PS_MITIGATION_OPTION_RESTRICT_CORE_SHARING,
    PS_MITIGATION_OPTION_FSCTL_SYSTEM_CALL_DISABLE, // since 24H2
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

#if (PHNT_VERSION >= PHNT_WINDOWS_VISTA)
/**
 * Creates a new process and primary thread.
 *
 * @param ProcessHandle A pointer to a handle that receives the process object handle.
 * @param ThreadHandle A pointer to a handle that receives the thread object handle.
 * @param ProcessDesiredAccess The access rights desired for the process object.
 * @param ThreadDesiredAccess The access rights desired for the thread object.
 * @param ProcessObjectAttributes Optional. A pointer to an OBJECT_ATTRIBUTES structure that specifies the attributes of the new process.
 * @param ThreadObjectAttributes Optional. A pointer to an OBJECT_ATTRIBUTES structure that specifies the attributes of the new thread.
 * @param ProcessFlags Flags that control the creation of the process. These flags are defined as PROCESS_CREATE_FLAGS_*.
 * @param ThreadFlags Flags that control the creation of the thread. These flags are defined as THREAD_CREATE_FLAGS_*.
 * @param ProcessParameters Optional. A pointer to a RTL_USER_PROCESS_PARAMETERS structure that specifies the parameters for the new process.
 * @param CreateInfo A pointer to a PS_CREATE_INFO structure that specifies additional information for the process creation.
 * @param AttributeList Optional. A pointer to a list of attributes for the process and thread.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateUserProcess(
    _Out_ PHANDLE ProcessHandle,
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK ProcessDesiredAccess,
    _In_ ACCESS_MASK ThreadDesiredAccess,
    _In_opt_ PCOBJECT_ATTRIBUTES ProcessObjectAttributes,
    _In_opt_ PCOBJECT_ATTRIBUTES ThreadObjectAttributes,
    _In_ ULONG ProcessFlags, // PROCESS_CREATE_FLAGS_*
    _In_ ULONG ThreadFlags, // THREAD_CREATE_FLAGS_*
    _In_opt_ PRTL_USER_PROCESS_PARAMETERS ProcessParameters,
    _Inout_ PPS_CREATE_INFO CreateInfo,
    _In_opt_ PPS_ATTRIBUTE_LIST AttributeList
    );

// begin_rev
#define THREAD_CREATE_FLAGS_NONE 0x00000000
#define THREAD_CREATE_FLAGS_CREATE_SUSPENDED 0x00000001 // NtCreateUserProcess & NtCreateThreadEx
#define THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH 0x00000002 // NtCreateThreadEx only
#define THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER 0x00000004 // NtCreateThreadEx only
#define THREAD_CREATE_FLAGS_LOADER_WORKER 0x00000010 // NtCreateThreadEx only // since THRESHOLD
#define THREAD_CREATE_FLAGS_SKIP_LOADER_INIT 0x00000020 // NtCreateThreadEx only // since REDSTONE2
#define THREAD_CREATE_FLAGS_BYPASS_PROCESS_FREEZE 0x00000040 // NtCreateThreadEx only // since 19H1
// end_rev

/**
 * A pointer to a user-defined function that serves as the starting routine for a new thread.
 *
 * @param ThreadParameter A pointer to a variable to be passed to the thread.
 * @return NTSTATUS Successful or errant status.
 */
typedef NTSTATUS (NTAPI *PUSER_THREAD_START_ROUTINE)(
    _In_ PVOID ThreadParameter
    );

/**
 * Creates a new thread in the specified process.
 *
 * @param ThreadHandle A pointer to a handle that receives the thread object handle.
 * @param DesiredAccess The access rights desired for the thread object.
 * @param ObjectAttributes Optional. A pointer to an OBJECT_ATTRIBUTES structure that specifies the attributes of the new thread.
 * @param ProcessHandle A handle to the process in which the thread is to be created.
 * @param StartRoutine A pointer to the application-defined function to be executed by the thread.
 * @param Argument Optional. A pointer to a variable to be passed to the thread.
 * @param CreateFlags Flags that control the creation of the thread. These flags are defined as THREAD_CREATE_FLAGS_*.
 * @param ZeroBits The number of zero bits in the starting address of the thread's stack.
 * @param StackSize The initial size of the thread's stack, in bytes.
 * @param MaximumStackSize The maximum size of the thread's stack, in bytes.
 * @param AttributeList Optional. A pointer to a list of attributes for the thread.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateThreadEx(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ProcessHandle,
    _In_ PUSER_THREAD_START_ROUTINE StartRoutine,
    _In_opt_ PVOID Argument,
    _In_ ULONG CreateFlags, // THREAD_CREATE_FLAGS_*
    _In_ SIZE_T ZeroBits,
    _In_ SIZE_T StackSize,
    _In_ SIZE_T MaximumStackSize,
    _In_opt_ PPS_ATTRIBUTE_LIST AttributeList
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_VISTA

#endif // PHNT_MODE != PHNT_MODE_KERNEL

//
// Job objects
//

#if (PHNT_MODE != PHNT_MODE_KERNEL)

// JOBOBJECTINFOCLASS
// Note: We don't use an enum since it conflicts with the Windows SDK.
#define JobObjectBasicAccountingInformation 1 // q: JOBOBJECT_BASIC_ACCOUNTING_INFORMATION
#define JobObjectBasicLimitInformation 2 // q; s: JOBOBJECT_BASIC_LIMIT_INFORMATION
#define JobObjectBasicProcessIdList 3 // q: JOBOBJECT_BASIC_PROCESS_ID_LIST
#define JobObjectBasicUIRestrictions 4 // q; s: JOBOBJECT_BASIC_UI_RESTRICTIONS
#define JobObjectSecurityLimitInformation 5 // JOBOBJECT_SECURITY_LIMIT_INFORMATION
#define JobObjectEndOfJobTimeInformation 6 // q; s: JOBOBJECT_END_OF_JOB_TIME_INFORMATION
#define JobObjectAssociateCompletionPortInformation 7 // s: JOBOBJECT_ASSOCIATE_COMPLETION_PORT
#define JobObjectBasicAndIoAccountingInformation 8 // q: JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION
#define JobObjectExtendedLimitInformation 9 // q; s: JOBOBJECT_EXTENDED_LIMIT_INFORMATION[V2]
#define JobObjectJobSetInformation 10 // JOBOBJECT_JOBSET_INFORMATION
#define JobObjectGroupInformation 11 // USHORT
#define JobObjectNotificationLimitInformation 12 // JOBOBJECT_NOTIFICATION_LIMIT_INFORMATION
#define JobObjectLimitViolationInformation 13 // JOBOBJECT_LIMIT_VIOLATION_INFORMATION
#define JobObjectGroupInformationEx 14 // GROUP_AFFINITY (ARRAY)
#define JobObjectCpuRateControlInformation 15 // JOBOBJECT_CPU_RATE_CONTROL_INFORMATION
#define JobObjectCompletionFilter 16 // q; s: ULONG
#define JobObjectCompletionCounter 17 // q; s: ULONG
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
#define JobObjectContainerId 30 // JOBOBJECT_CONTAINER_IDENTIFIER_V2
#define JobObjectIoRateControlInformation 31 // JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE, JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE_V2, JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE_V3
#define JobObjectNetRateControlInformation 32 // JOBOBJECT_NET_RATE_CONTROL_INFORMATION
#define JobObjectNotificationLimitInformation2 33 // JOBOBJECT_NOTIFICATION_LIMIT_INFORMATION_2
#define JobObjectLimitViolationInformation2 34 // JOBOBJECT_LIMIT_VIOLATION_INFORMATION_2
#define JobObjectCreateSilo 35 // s: NULL
#define JobObjectSiloBasicInformation 36 // SILOOBJECT_BASIC_INFORMATION
#define JobObjectSiloRootDirectory 37 // SILOOBJECT_ROOT_DIRECTORY
#define JobObjectServerSiloBasicInformation 38 // SERVERSILO_BASIC_INFORMATION
#define JobObjectServerSiloUserSharedData 39 // SILO_USER_SHARED_DATA // NtQueryInformationJobObject(NULL, 39, Buffer, sizeof(SILO_USER_SHARED_DATA), 0);
#define JobObjectServerSiloInitialize 40 // SERVERSILO_INIT_INFORMATION
#define JobObjectServerSiloRunningState 41
#define JobObjectIoAttribution 42 // JOBOBJECT_IO_ATTRIBUTION_INFORMATION
#define JobObjectMemoryPartitionInformation 43
#define JobObjectContainerTelemetryId 44 // s: GUID // NtSetInformationJobObject(_In_ PGUID, 44, _In_ PGUID, sizeof(GUID)); // daxexec
#define JobObjectSiloSystemRoot 45 // s: UNICODE_STRING
#define JobObjectEnergyTrackingState 46 // JOBOBJECT_ENERGY_TRACKING_STATE
#define JobObjectThreadImpersonationInformation 47 // q; s: BOOLEAN
#define JobObjectIoPriorityLimit 48 // JOBOBJECT_IO_PRIORITY_LIMIT
#define JobObjectPagePriorityLimit 49 // JOBOBJECT_PAGE_PRIORITY_LIMIT
#define JobObjectServerSiloDiagnosticInformation 50 // SERVERSILO_DIAGNOSTIC_INFORMATION // since 24H2
#define JobObjectNetworkAccountingInformation 51 // JOBOBJECT_NETWORK_ACCOUNTING_INFORMATION
#define MaxJobObjectInfoClass 52

// rev // extended limit v2
#define JOB_OBJECT_LIMIT_SILO_READY 0x00400000

// private
typedef struct _JOBOBJECT_EXTENDED_LIMIT_INFORMATION_V2
{
    JOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimitInformation;
    IO_COUNTERS IoInfo;
    SIZE_T ProcessMemoryLimit;
    SIZE_T JobMemoryLimit;
    SIZE_T PeakProcessMemoryUsed;
    SIZE_T PeakJobMemoryUsed;
    SIZE_T JobTotalMemoryLimit;
} JOBOBJECT_EXTENDED_LIMIT_INFORMATION_V2, *PJOBOBJECT_EXTENDED_LIMIT_INFORMATION_V2;

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
typedef struct _JOBOBJECT_CONTAINER_IDENTIFIER_V2
{
    GUID ContainerId;
    GUID ContainerTelemetryId;
    ULONG JobId;
} JOBOBJECT_CONTAINER_IDENTIFIER_V2, *PJOBOBJECT_CONTAINER_IDENTIFIER_V2;

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
    BOOLEAN IsStateSeparationEnabled;
    WCHAR NtSystemRoot[260];
    USHORT UserModeGlobalLogger[16];
    ULONG TimeZoneId; // since 21H2
    LONG TimeZoneBiasStamp;
    KSYSTEM_TIME TimeZoneBias;
    LARGE_INTEGER TimeZoneBiasEffectiveStart;
    LARGE_INTEGER TimeZoneBiasEffectiveEnd;
} SILO_USER_SHARED_DATA, *PSILO_USER_SHARED_DATA;

// rev
#define SILO_OBJECT_ROOT_DIRECTORY_SHADOW_ROOT 0x00000001
#define SILO_OBJECT_ROOT_DIRECTORY_INITIALIZE 0x00000002
#define SILO_OBJECT_ROOT_DIRECTORY_SHADOW_DOS_DEVICES 0x00000004

// private
typedef struct _SILOOBJECT_ROOT_DIRECTORY
{
    union
    {
        ULONG ControlFlags; // SILO_OBJECT_ROOT_DIRECTORY_*
        UNICODE_STRING Path;
    };
} SILOOBJECT_ROOT_DIRECTORY, *PSILOOBJECT_ROOT_DIRECTORY;

// private
typedef struct _SERVERSILO_INIT_INFORMATION
{
    HANDLE DeleteEvent;
    BOOLEAN IsDownlevelContainer;
} SERVERSILO_INIT_INFORMATION, *PSERVERSILO_INIT_INFORMATION;

// private
typedef struct _JOBOBJECT_ENERGY_TRACKING_STATE
{
    ULONG64 Value;
    ULONG UpdateMask;
    ULONG DesiredState;
} JOBOBJECT_ENERGY_TRACKING_STATE, *PJOBOBJECT_ENERGY_TRACKING_STATE;

// private
typedef enum _JOBOBJECT_IO_PRIORITY_LIMIT_FLAGS
{
    JOBOBJECT_IO_PRIORITY_LIMIT_ENABLE = 0x1,
    JOBOBJECT_IO_PRIORITY_LIMIT_VALID_FLAGS = 0x1,
} JOBOBJECT_IO_PRIORITY_LIMIT_FLAGS;

// private
typedef struct _JOBOBJECT_IO_PRIORITY_LIMIT
{
    JOBOBJECT_IO_PRIORITY_LIMIT_FLAGS Flags;
    ULONG Priority;
} JOBOBJECT_IO_PRIORITY_LIMIT, *PJOBOBJECT_IO_PRIORITY_LIMIT;

// private
typedef enum _JOBOBJECT_PAGE_PRIORITY_LIMIT_FLAGS
{
    JOBOBJECT_PAGE_PRIORITY_LIMIT_ENABLE = 0x1,
    JOBOBJECT_PAGE_PRIORITY_LIMIT_VALID_FLAGS = 0x1,
} JOBOBJECT_PAGE_PRIORITY_LIMIT_FLAGS;

// private
typedef struct _JOBOBJECT_PAGE_PRIORITY_LIMIT
{
    JOBOBJECT_PAGE_PRIORITY_LIMIT_FLAGS Flags;
    ULONG Priority;
} JOBOBJECT_PAGE_PRIORITY_LIMIT, *PJOBOBJECT_PAGE_PRIORITY_LIMIT;

#if !defined(NTDDI_WIN11_GE) || (NTDDI_VERSION < NTDDI_WIN11_GE)
// private
typedef struct _SERVERSILO_DIAGNOSTIC_INFORMATION
{
    NTSTATUS ExitStatus;
    WCHAR CriticalProcessName[15];
} SERVERSILO_DIAGNOSTIC_INFORMATION, *PSERVERSILO_DIAGNOSTIC_INFORMATION;

// private
typedef struct _JOBOBJECT_NETWORK_ACCOUNTING_INFORMATION
{
    ULONG64 DataBytesIn;
    ULONG64 DataBytesOut;
} JOBOBJECT_NETWORK_ACCOUNTING_INFORMATION, *PJOBOBJECT_NETWORK_ACCOUNTING_INFORMATION;
#endif // !defined(NTDDI_WIN11_GE) || (NTDDI_VERSION < NTDDI_WIN11_GE)

/**
 * Creates or opens a job object.
 *
 * @param JobHandle A handle to the job object.
 * @param DesiredAccess The access rights desired for the thread object.
 * @param ObjectAttributes Optional. A pointer to an OBJECT_ATTRIBUTES structure that specifies the attributes of the new thread.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateJobObject(
    _Out_ PHANDLE JobHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ PCOBJECT_ATTRIBUTES ObjectAttributes
    );

/**
 * Opens an existing job object.
 *
 * @param JobHandle A handle to the job object.
 * @param DesiredAccess The access rights desired for the thread object.
 * @param ObjectAttributes Optional. A pointer to an OBJECT_ATTRIBUTES structure that specifies the attributes of the new thread.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenJobObject(
    _Out_ PHANDLE JobHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCOBJECT_ATTRIBUTES ObjectAttributes
    );

/**
 * Assigns a process to an existing job object.
 *
 * @param JobHandle A handle to the job object to which the process will be associated. The handle must have the JOB_OBJECT_ASSIGN_PROCESS access right.
 * @param ProcessHandle A handle to the process to associate with the job object. The handle must have the PROCESS_SET_QUOTA and PROCESS_TERMINATE access rights.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAssignProcessToJobObject(
    _In_ HANDLE JobHandle,
    _In_ HANDLE ProcessHandle
    );

/**
 * Terminates all processes associated with the job object. If the job is nested, all processes currently associated with the job and all child jobs in the hierarchy are terminated.
 *
 * @param JobHandle A handle to the job whose processes will be terminated. The handle must have the JOB_OBJECT_TERMINATE access right.
 * @param ExitStatus The exit status to be used by all processes and threads in the job object.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtTerminateJobObject(
    _In_ HANDLE JobHandle,
    _In_ NTSTATUS ExitStatus
    );

/**
 * Checks if a process is associated with a job object.
 *
 * @param ProcessHandle A handle to the process to be checked.
 * @param JobHandle An optional handle to the job object. If this parameter is NULL, the function checks if the process is associated with any job object.
 * @return NTSTATUS Successful or errant status.
 * @remarks This function can be used to determine if a process is running within a job object, which can be useful for managing process resources and constraints.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtIsProcessInJob(
    _In_ HANDLE ProcessHandle,
    _In_opt_ HANDLE JobHandle
    );

/**
 * Retrieves information about a job object.
 *
 * @param JobHandle An optional handle to the job object. If this parameter is NULL, the function retrieves information about the job object associated with the calling process.
 * @param JobObjectInformationClass The type of job object information to be retrieved.
 * @param JobObjectInformation A pointer to a buffer that receives the job object information.
 * @param JobObjectInformationLength The size of the buffer pointed to by the JobObjectInformation parameter.
 * @param ReturnLength An optional pointer to a variable that receives the size of the data returned.
 * @return NTSTATUS Successful or errant status.
 * @remarks This function can be used to query various types of information about a job object, such as accounting information, limit information, and process ID list.
 */
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

/**
 * Sets information for a job object.
 *
 * @param JobHandle A handle to the job object.
 * @param JobObjectInformationClass The type of job object information to be set.
 * @param JobObjectInformation A pointer to a buffer that contains the job object information.
 * @param JobObjectInformationLength The size of the buffer pointed to by the JobObjectInformation parameter.
 * @return NTSTATUS Successful or errant status.
 * @remarks This function can be used to set various types of information for a job object, such as limit information, UI restrictions, and security limit information.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationJobObject(
    _In_ HANDLE JobHandle,
    _In_ JOBOBJECTINFOCLASS JobObjectInformationClass,
    _In_reads_bytes_(JobObjectInformationLength) PVOID JobObjectInformation,
    _In_ ULONG JobObjectInformationLength
    );

/**
 * Creates a set of job objects.
 *
 * @param NumJob The number of job objects in the set.
 * @param UserJobSet A pointer to an array of JOB_SET_ARRAY structures that specify the job objects in the set.
 * @param Flags Reserved for future use. Must be zero.
 * @return NTSTATUS Successful or errant status.
 * @remarks This function can be used to create a set of job objects, which can be useful for managing groups of related processes.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateJobSet(
    _In_ ULONG NumJob,
    _In_reads_(NumJob) PJOB_SET_ARRAY UserJobSet,
    _In_ ULONG Flags
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtRevertContainerImpersonation(
    VOID
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_10)

#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

//
// Reserve objects
//

#if (PHNT_MODE != PHNT_MODE_KERNEL)

// private
typedef enum _MEMORY_RESERVE_TYPE
{
    MemoryReserveUserApc,
    MemoryReserveIoCompletion,
    MemoryReserveTypeMax
} MEMORY_RESERVE_TYPE;

#if (PHNT_VERSION >= PHNT_WINDOWS_7)
/**
 * Allocates a memory reserve object.
 *
 * @param MemoryReserveHandle Pointer to a variable that receives the memory reserve object handle.
 * @param ObjectAttributes Pointer to an object attributes structure.
 * @param Type The type of memory reserve.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateReserveObject(
    _Out_ PHANDLE MemoryReserveHandle,
    _In_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_ MEMORY_RESERVE_TYPE Type
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_7)

//
// Process snapshotting
//

// Capture/creation flags.
typedef enum _PSSNT_CAPTURE_FLAGS
{
    PSSNT_CAPTURE_NONE                                = 0x00000000,
    PSSNT_CAPTURE_VA_CLONE                            = 0x00000001,
    PSSNT_CAPTURE_RESERVED_00000002                   = 0x00000002,
    PSSNT_CAPTURE_HANDLES                             = 0x00000004,
    PSSNT_CAPTURE_HANDLE_NAME_INFORMATION             = 0x00000008,
    PSSNT_CAPTURE_HANDLE_BASIC_INFORMATION            = 0x00000010,
    PSSNT_CAPTURE_HANDLE_TYPE_SPECIFIC_INFORMATION    = 0x00000020,
    PSSNT_CAPTURE_HANDLE_TRACE                        = 0x00000040,
    PSSNT_CAPTURE_THREADS                             = 0x00000080,
    PSSNT_CAPTURE_THREAD_CONTEXT                      = 0x00000100,
    PSSNT_CAPTURE_THREAD_CONTEXT_EXTENDED             = 0x00000200,
    PSSNT_CAPTURE_RESERVED_00000400                   = 0x00000400,
    PSSNT_CAPTURE_VA_SPACE                            = 0x00000800,
    PSSNT_CAPTURE_VA_SPACE_SECTION_INFORMATION        = 0x00001000,
    PSSNT_CAPTURE_IPT_TRACE                           = 0x00002000,
    PSSNT_CAPTURE_RESERVED_00004000                   = 0x00004000,

    PSSNT_CREATE_BREAKAWAY_OPTIONAL                   = 0x04000000,
    PSSNT_CREATE_BREAKAWAY                            = 0x08000000,
    PSSNT_CREATE_FORCE_BREAKAWAY                      = 0x10000000,
    PSSNT_CREATE_USE_VM_ALLOCATIONS                   = 0x20000000,
    PSSNT_CREATE_MEASURE_PERFORMANCE                  = 0x40000000,
    PSSNT_CREATE_RELEASE_SECTION                      = 0x80000000
} PSSNT_CAPTURE_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(PSSNT_CAPTURE_FLAGS);

typedef enum _PSSNT_DUPLICATE_FLAGS
{
    PSSNT_DUPLICATE_NONE         = 0x00,
    PSSNT_DUPLICATE_CLOSE_SOURCE = 0x01
} PSSNT_DUPLICATE_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(PSSNT_DUPLICATE_FLAGS);

typedef enum _PSSNT_QUERY_INFORMATION_CLASS
{
    PSSNT_QUERY_PROCESS_INFORMATION = 0, // PSS_PROCESS_INFORMATION
    PSSNT_QUERY_VA_CLONE_INFORMATION = 1, // PSS_VA_CLONE_INFORMATION
    PSSNT_QUERY_AUXILIARY_PAGES_INFORMATION = 2, // PSS_AUXILIARY_PAGES_INFORMATION
    PSSNT_QUERY_VA_SPACE_INFORMATION = 3, // PSS_VA_SPACE_INFORMATION
    PSSNT_QUERY_HANDLE_INFORMATION = 4, // PSS_HANDLE_INFORMATION
    PSSNT_QUERY_THREAD_INFORMATION = 5, // PSS_THREAD_INFORMATION
    PSSNT_QUERY_HANDLE_TRACE_INFORMATION = 6, // PSS_HANDLE_TRACE_INFORMATION
    PSSNT_QUERY_PERFORMANCE_COUNTERS = 7 // PSS_PERFORMANCE_COUNTERS
} PSSNT_QUERY_INFORMATION_CLASS;

#define PSSNT_SIGNATURE_PSSD 'PSSD' // 0x50535344

#if (PHNT_VERSION >= PHNT_WINDOWS_8_1)
// rev
/**
 * Captures a snapshot of the specified process.
 *
 * @param SnapshotHandle Pointer to a variable that receives the snapshot handle.
 * @param ProcessHandle Handle to the process.
 * @param CaptureFlags Flags indicating what to capture.
 * @param ThreadContextFlags Optional flags for capturing thread context.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
PssNtCaptureSnapshot(
    _Out_ PHANDLE SnapshotHandle,
    _In_ HANDLE ProcessHandle,
    _In_ PSSNT_CAPTURE_FLAGS CaptureFlags,
    _In_opt_ ULONG ThreadContextFlags
    );

// rev
/**
 * Duplicates a process snapshot from one process to another.
 *
 * @param SourceProcessHandle Handle to the source process.
 * @param SnapshotHandle Handle to the snapshot to duplicate.
 * @param TargetProcessHandle Handle to the target process.
 * @param TargetSnapshotHandle Pointer to a variable that receives the duplicated snapshot handle.
 * @param Flags Optional flags for duplication.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
PssNtDuplicateSnapshot(
    _In_ HANDLE SourceProcessHandle,
    _In_ HANDLE SnapshotHandle,
    _In_ HANDLE TargetProcessHandle,
    _Out_ PHANDLE TargetSnapshotHandle,
    _In_opt_ PSSNT_DUPLICATE_FLAGS Flags
    );

// rev
/**
 * Frees a remote process snapshot.
 *
 * @param ProcessHandle A handle to the process that contains the snapshot. The handle must have PROCESS_VM_READ, PROCESS_VM_OPERATION, and PROCESS_DUP_HANDLE rights.
 * @param SnapshotHandle Handle to the snapshot to free.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
PssNtFreeSnapshot(
    _In_ HANDLE SnapshotHandle
    );

// rev
/**
 * Frees a snapshot.
 *
 * @param SnapshotHandle Handle to the snapshot to free.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
PssNtFreeRemoteSnapshot(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE SnapshotHandle
    );

// rev
/**
 * Queries information from a the specified snapshot.
 *
 * @param SnapshotHandle Handle to the snapshot.
 * @param InformationClass The information class to query.
 * @param Buffer Pointer to a buffer that receives the queried information.
 * @param BufferLength Length of the buffer.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
PssNtQuerySnapshot(
    _In_ HANDLE SnapshotHandle,
    _In_ PSSNT_QUERY_INFORMATION_CLASS InformationClass,
    _Out_writes_bytes_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_8_1)

// rev
/**
 * Flag indicating the type of bulk information to query.
 */
#define MEMORY_BULK_INFORMATION_FLAG_BASIC 0x00000001

// rev
/**
 * The NTPSS_MEMORY_BULK_INFORMATION structure is used to query basic memory information in bulk for a process.
 */
typedef struct _NTPSS_MEMORY_BULK_INFORMATION
{
    ULONG QueryFlags;
    ULONG NumberOfEntries;
    PVOID NextValidAddress;
} NTPSS_MEMORY_BULK_INFORMATION, *PNTPSS_MEMORY_BULK_INFORMATION;

#if (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)
// rev
/**
 * Captures virtual address space bulk information for a process.
 *
 * @param ProcessHandle Handle to the process.
 * @param BaseAddress Optional base address to start the capture.
 * @param BulkInformation Pointer to the memory bulk information structure.
 * @param BulkInformationLength Length of the memory bulk information structure.
 * @param ReturnLength Optional pointer to a variable that receives the length of the captured information.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSCALLAPI
NTSTATUS
NTAPI
NtPssCaptureVaSpaceBulk(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_ PNTPSS_MEMORY_BULK_INFORMATION BulkInformation,
    _In_ SIZE_T BulkInformationLength,
    _Out_opt_ PSIZE_T ReturnLength
    );
#endif // (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)

#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

#endif // _NTPSAPI_H
