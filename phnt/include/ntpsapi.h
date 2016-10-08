#ifndef _NTPSAPI_H
#define _NTPSAPI_H

#if (PHNT_MODE == PHNT_MODE_KERNEL)
#define PROCESS_TERMINATE 0x0001
#define PROCESS_CREATE_THREAD 0x0002
#define PROCESS_SET_SESSIONID 0x0004
#define PROCESS_VM_OPERATION 0x0008
#define PROCESS_VM_READ 0x0010
#define PROCESS_VM_WRITE 0x0020
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
#define JOB_OBJECT_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1f)
#endif

#define GDI_HANDLE_BUFFER_SIZE32 34
#define GDI_HANDLE_BUFFER_SIZE64 60

#ifndef WIN64
#define GDI_HANDLE_BUFFER_SIZE GDI_HANDLE_BUFFER_SIZE32
#else
#define GDI_HANDLE_BUFFER_SIZE GDI_HANDLE_BUFFER_SIZE64
#endif

typedef ULONG GDI_HANDLE_BUFFER[GDI_HANDLE_BUFFER_SIZE];

typedef ULONG GDI_HANDLE_BUFFER32[GDI_HANDLE_BUFFER_SIZE32];
typedef ULONG GDI_HANDLE_BUFFER64[GDI_HANDLE_BUFFER_SIZE64];

#define FLS_MAXIMUM_AVAILABLE 128
#define TLS_MINIMUM_AVAILABLE 64
#define TLS_EXPANSION_SLOTS 1024

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

// source:http://www.microsoft.com/whdc/system/Sysinternals/MoreThan64proc.mspx

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
    ProcessExceptionPort, // s: HANDLE
    ProcessAccessToken, // s: PROCESS_ACCESS_TOKEN
    ProcessLdtInformation, // qs: PROCESS_LDT_INFORMATION // 10
    ProcessLdtSize, // s: PROCESS_LDT_SIZE
    ProcessDefaultHardErrorMode, // qs: ULONG
    ProcessIoPortHandlers, // (kernel-mode only)
    ProcessPooledUsageAndLimits, // q: POOLED_USAGE_AND_LIMITS
    ProcessWorkingSetWatch, // q: PROCESS_WS_WATCH_INFORMATION[]; s: void
    ProcessUserModeIOPL,
    ProcessEnableAlignmentFaultFixup, // s: BOOLEAN
    ProcessPriorityClass, // qs: PROCESS_PRIORITY_CLASS
    ProcessWx86Information,
    ProcessHandleCount, // q: ULONG, PROCESS_HANDLE_INFORMATION // 20
    ProcessAffinityMask, // s: KAFFINITY
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
    ProcessResourceManagement,
    ProcessCookie, // q: ULONG
    ProcessImageInformation, // q: SECTION_IMAGE_INFORMATION
    ProcessCycleTime, // q: PROCESS_CYCLE_TIME_INFORMATION // since VISTA
    ProcessPagePriority, // q: ULONG
    ProcessInstrumentationCallback, // 40
    ProcessThreadStackAllocation, // s: PROCESS_STACK_ALLOCATION_INFORMATION, PROCESS_STACK_ALLOCATION_INFORMATION_EX
    ProcessWorkingSetWatchEx, // q: PROCESS_WS_WATCH_INFORMATION_EX[]
    ProcessImageFileNameWin32, // q: UNICODE_STRING
    ProcessImageFileMapping, // q: HANDLE (input)
    ProcessAffinityUpdateMode, // qs: PROCESS_AFFINITY_UPDATE_MODE
    ProcessMemoryAllocationMode, // qs: PROCESS_MEMORY_ALLOCATION_MODE
    ProcessGroupInformation, // q: USHORT[]
    ProcessTokenVirtualizationEnabled, // s: ULONG
    ProcessConsoleHostProcess, // q: ULONG_PTR
    ProcessWindowInformation, // q: PROCESS_WINDOW_INFORMATION // 50
    ProcessHandleInformation, // q: PROCESS_HANDLE_SNAPSHOT_INFORMATION // since WIN8
    ProcessMitigationPolicy, // s: PROCESS_MITIGATION_POLICY_INFORMATION
    ProcessDynamicFunctionTableInformation,
    ProcessHandleCheckingMode,
    ProcessKeepAliveCount, // q: PROCESS_KEEPALIVE_COUNT_INFORMATION
    ProcessRevokeFileHandles, // s: PROCESS_REVOKE_FILE_HANDLES_INFORMATION
    ProcessWorkingSetControl, // s: PROCESS_WORKING_SET_CONTROL
    ProcessHandleTable, // since WINBLUE
    ProcessCheckStackExtentsMode,
    ProcessCommandLineInformation, // q: UNICODE_STRING // 60
    ProcessProtectionInformation, // q: PS_PROTECTION
    ProcessMemoryExhaustion, // PROCESS_MEMORY_EXHAUSTION_INFO // since THRESHOLD
    ProcessFaultInformation, // PROCESS_FAULT_INFORMATION
    ProcessTelemetryIdInformation, // PROCESS_TELEMETRY_ID_INFORMATION
    ProcessCommitReleaseInformation, // PROCESS_COMMIT_RELEASE_INFORMATION
    ProcessDefaultCpuSetsInformation,
    ProcessAllowedCpuSetsInformation,
    ProcessSubsystemProcess, // 68
    ProcessJobMemoryInformation, // PROCESS_JOB_MEMORY_INFO
    ProcessInPrivate, // since THRESHOLD2
    ProcessRaiseUMExceptionOnInvalidHandleClose,
    MaxProcessInfoClass
} PROCESSINFOCLASS;
#endif

#if (PHNT_MODE != PHNT_MODE_KERNEL)
typedef enum _THREADINFOCLASS
{
    ThreadBasicInformation, // q: THREAD_BASIC_INFORMATION
    ThreadTimes, // q: KERNEL_USER_TIMES
    ThreadPriority, // s: KPRIORITY
    ThreadBasePriority, // s: LONG
    ThreadAffinityMask, // s: KAFFINITY
    ThreadImpersonationToken, // s: HANDLE
    ThreadDescriptorTableEntry, // q: DESCRIPTOR_TABLE_ENTRY (or WOW64_DESCRIPTOR_TABLE_ENTRY)
    ThreadEnableAlignmentFaultFixup, // s: BOOLEAN
    ThreadEventPair,
    ThreadQuerySetWin32StartAddress, // q: PVOID
    ThreadZeroTlsCell, // 10
    ThreadPerformanceCount, // q: LARGE_INTEGER
    ThreadAmILastThread, // q: ULONG
    ThreadIdealProcessor, // s: ULONG
    ThreadPriorityBoost, // qs: ULONG
    ThreadSetTlsArrayAddress,
    ThreadIsIoPending, // q: ULONG
    ThreadHideFromDebugger, // s: void
    ThreadBreakOnTermination, // qs: ULONG
    ThreadSwitchLegacyState,
    ThreadIsTerminated, // q: ULONG // 20
    ThreadLastSystemCall, // q: THREAD_LAST_SYSCALL_INFORMATION
    ThreadIoPriority, // qs: IO_PRIORITY_HINT
    ThreadCycleTime, // q: THREAD_CYCLE_TIME_INFORMATION
    ThreadPagePriority, // q: ULONG
    ThreadActualBasePriority,
    ThreadTebInformation, // q: THREAD_TEB_INFORMATION (requires THREAD_GET_CONTEXT + THREAD_SET_CONTEXT)
    ThreadCSwitchMon,
    ThreadCSwitchPmu,
    ThreadWow64Context, // q: WOW64_CONTEXT
    ThreadGroupInformation, // q: GROUP_AFFINITY // 30
    ThreadUmsInformation, // q: THREAD_UMS_INFORMATION
    ThreadCounterProfiling,
    ThreadIdealProcessorEx, // q: PROCESSOR_NUMBER
    ThreadCpuAccountingInformation, // since WIN8
    ThreadSuspendCount, // since WINBLUE
    ThreadHeterogeneousCpuPolicy, // q: KHETERO_CPU_POLICY // since THRESHOLD
    ThreadContainerId, // q: GUID
    ThreadNameInformation, // qs: THREAD_NAME_INFORMATION
    ThreadSelectedCpuSets,
    ThreadSystemThreadInformation, // q: SYSTEM_THREAD_INFORMATION // 40
    ThreadActualGroupAffinity, // since THRESHOLD2
    ThreadDynamicCodePolicyInfo,
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
    ULONG_PTR AffinityMask;
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
            ULONG IsPicoProcess : 1;
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

typedef struct _PROCESS_HANDLE_TRACING_ENABLE
{
    ULONG Flags; // 0 to disable, 1 to enable
} PROCESS_HANDLE_TRACING_ENABLE, *PPROCESS_HANDLE_TRACING_ENABLE;

typedef struct _PROCESS_HANDLE_TRACING_ENABLE_EX
{
    ULONG Flags; // 0 to disable, 1 to enable
    ULONG TotalSlots;
} PROCESS_HANDLE_TRACING_ENABLE_EX, *PPROCESS_HANDLE_TRACING_ENABLE_EX;

#define PROCESS_HANDLE_TRACING_MAX_STACKS 16
#define HANDLE_TRACE_DB_OPEN 1
#define HANDLE_TRACE_DB_CLOSE 2
#define HANDLE_TRACE_DB_BADREF 3

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
    PROCESS_HANDLE_TRACING_ENTRY HandleTrace[1];
} PROCESS_HANDLE_TRACING_QUERY, *PPROCESS_HANDLE_TRACING_QUERY;

#endif

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
    WCHAR WindowTitle[1];
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
    PROCESS_HANDLE_TABLE_ENTRY_INFO Handles[1];
} PROCESS_HANDLE_SNAPSHOT_INFORMATION, *PPROCESS_HANDLE_SNAPSHOT_INFORMATION;

#if (PHNT_MODE != PHNT_MODE_KERNEL)

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
    PsProtectedSignerMax
} PS_PROTECTED_SIGNER;

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
        ULONG Spare : 31;
    };
    SIZE_T CommitDebt;
} PROCESS_COMMIT_RELEASE_INFORMATION, *PPROCESS_COMMIT_RELEASE_INFORMATION;

typedef struct _PROCESS_JOB_MEMORY_INFO
{
    ULONGLONG SharedCommitUsage;
    ULONGLONG PrivateCommitUsage;
    ULONGLONG PeakPrivateCommitUsage;
    ULONGLONG PrivateCommitLimit;
    ULONGLONG TotalCommitLimit;
} PROCESS_JOB_MEMORY_INFO, *PPROCESS_JOB_MEMORY_INFO;

// end_private

#endif

// Thread information structures

typedef struct _THREAD_BASIC_INFORMATION
{
    NTSTATUS ExitStatus;
    PTEB TebBaseAddress;
    CLIENT_ID ClientId;
    ULONG_PTR AffinityMask;
    KPRIORITY Priority;
    LONG BasePriority;
} THREAD_BASIC_INFORMATION, *PTHREAD_BASIC_INFORMATION;

// private
typedef struct _THREAD_LAST_SYSCALL_INFORMATION
{
    PVOID FirstArgument;
    USHORT SystemCallNumber;
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
    volatile ULONG ScheduledThread;
    volatile ULONG Suspended;
    volatile ULONG VolatileContext;
    volatile ULONG Terminated;
    volatile ULONG DebugActive;
    volatile ULONG RunningOnSelfThread;
    volatile ULONG DenyRunningOnSelfThread;
    volatile LONG Flags;
    volatile ULONG64 KernelUpdateLock;
    volatile ULONG64 PrimaryClientID;
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
    UmsInformationCommandQuery, // Index might be incorrect.
    UmsInformationCommandAttach,
    UmsInformationCommandDetach,
    UmsInformationCommandInvalid
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
    ULONG Flags;
    ULONG IsUmsSchedulerThread;
    ULONG IsUmsWorkerThread;
} THREAD_UMS_INFORMATION, *PTHREAD_UMS_INFORMATION;

// private
typedef struct _THREAD_NAME_INFORMATION
{
    UNICODE_STRING ThreadName;
} THREAD_NAME_INFORMATION, *PTHREAD_NAME_INFORMATION;

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
    _In_opt_ HANDLE ExceptionPort
    );

#define PROCESS_CREATE_FLAGS_BREAKAWAY 0x00000001
#define PROCESS_CREATE_FLAGS_NO_DEBUG_INHERIT 0x00000002
#define PROCESS_CREATE_FLAGS_INHERIT_HANDLES 0x00000004
#define PROCESS_CREATE_FLAGS_OVERRIDE_ADDRESS_SPACE 0x00000008
#define PROCESS_CREATE_FLAGS_LARGE_PAGES 0x00000010

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateProcessEx(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ParentProcess,
    _In_ ULONG Flags,
    _In_opt_ HANDLE SectionHandle,
    _In_opt_ HANDLE DebugPort,
    _In_opt_ HANDLE ExceptionPort,
    _In_ ULONG JobMemberLevel
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
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetNextProcess(
    _In_ HANDLE ProcessHandle,
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
    _In_ HANDLE ThreadHandle,
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

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryPortInformationProcess(
    VOID
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

typedef VOID (*PPS_APC_ROUTINE)(
    _In_opt_ PVOID ApcArgument1,
    _In_opt_ PVOID ApcArgument2,
    _In_opt_ PVOID ApcArgument3
    );

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
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueueApcThreadEx(
    _In_ HANDLE ThreadHandle,
    _In_opt_ HANDLE UserApcReserveHandle,
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

// Attributes

// begin_rev
#define PS_ATTRIBUTE_NUMBER_MASK 0x0000ffff
#define PS_ATTRIBUTE_THREAD 0x00010000 // can be used with threads
#define PS_ATTRIBUTE_INPUT 0x00020000 // input only
#define PS_ATTRIBUTE_UNKNOWN 0x00040000
// end_rev

// private
typedef enum _PS_ATTRIBUTE_NUM
{
    PsAttributeParentProcess, // in HANDLE
    PsAttributeDebugPort, // in HANDLE
    PsAttributeToken, // in HANDLE
    PsAttributeClientId, // out PCLIENT_ID
    PsAttributeTebAddress, // out PTEB *
    PsAttributeImageName, // in PWSTR
    PsAttributeImageInfo, // out PSECTION_IMAGE_INFORMATION
    PsAttributeMemoryReserve, // in PPS_MEMORY_RESERVE
    PsAttributePriorityClass, // in UCHAR
    PsAttributeErrorMode, // in ULONG
    PsAttributeStdHandleInfo, // 10, in PPS_STD_HANDLE_INFO
    PsAttributeHandleList, // in PHANDLE
    PsAttributeGroupAffinity, // in PGROUP_AFFINITY
    PsAttributePreferredNode, // in PUSHORT
    PsAttributeIdealProcessor, // in PPROCESSOR_NUMBER
    PsAttributeUmsThread, // ? in PUMS_CREATE_THREAD_ATTRIBUTES
    PsAttributeMitigationOptions, // in UCHAR
    PsAttributeProtectionLevel,
    PsAttributeSecureProcess, // since THRESHOLD
    PsAttributeJobList,
    PsAttributeChildProcessPolicy, // since THRESHOLD2
    PsAttributeMax
} PS_ATTRIBUTE_NUM;

// begin_rev

#define PsAttributeValue(Number, Thread, Input, Unknown) \
    (((Number) & PS_ATTRIBUTE_NUMBER_MASK) | \
    ((Thread) ? PS_ATTRIBUTE_THREAD : 0) | \
    ((Input) ? PS_ATTRIBUTE_INPUT : 0) | \
    ((Unknown) ? PS_ATTRIBUTE_UNKNOWN : 0))

#define PS_ATTRIBUTE_PARENT_PROCESS \
    PsAttributeValue(PsAttributeParentProcess, FALSE, TRUE, TRUE)
#define PS_ATTRIBUTE_DEBUG_PORT \
    PsAttributeValue(PsAttributeDebugPort, FALSE, TRUE, TRUE)
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
#define PS_ATTRIBUTE_MITIGATION_OPTIONS \
    PsAttributeValue(PsAttributeMitigationOptions, FALSE, TRUE, TRUE)

// end_rev

// begin_private

typedef struct _PS_ATTRIBUTE
{
    ULONG Attribute;
    SIZE_T Size;
    union
    {
        ULONG Value;
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

// Extended PROCESS_CREATE_FLAGS_*
// begin_rev
#define PROCESS_CREATE_FLAGS_LARGE_PAGE_SYSTEM_DLL 0x00000020
#define PROCESS_CREATE_FLAGS_PROTECTED_PROCESS 0x00000040
#define PROCESS_CREATE_FLAGS_CREATE_SESSION 0x00000080 // ?
#define PROCESS_CREATE_FLAGS_INHERIT_FROM_PARENT 0x00000100
// end_rev

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
#define THREAD_CREATE_FLAGS_CREATE_SUSPENDED 0x00000001
#define THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH 0x00000002 // ?
#define THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER 0x00000004
#define THREAD_CREATE_FLAGS_HAS_SECURITY_DESCRIPTOR 0x00000010 // ?
#define THREAD_CREATE_FLAGS_ACCESS_CHECK_IN_TARGET 0x00000020 // ?
#define THREAD_CREATE_FLAGS_INITIAL_THREAD 0x00000080
// end_rev

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateThreadEx(
    _Out_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ HANDLE ProcessHandle,
    _In_ PVOID StartRoutine, // PUSER_THREAD_START_ROUTINE
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

#endif

#endif
