#ifndef _NTPSAPI_H
#define _NTPSAPI_H

#ifndef PROCESS_SET_PORT
#define PROCESS_SET_PORT 0x0800
#endif

#ifndef THREAD_ALERT
#define THREAD_ALERT 0x0004
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

typedef enum _PROCESS_INFORMATION_CLASS
{
    ProcessBasicInformation, // 0, q: PROCESS_BASIC_INFORMATION, PROCESS_EXTENDED_BASIC_INFORMATION
    ProcessQuotaLimits, // qs: QUOTA_LIMITS, QUOTA_LIMITS_EX
    ProcessIoCounters, // q: IO_COUNTERS
    ProcessVmCounters, // q: VM_COUNTERS, VM_COUNTERS_EX
    ProcessTimes, // q: KERNEL_USER_TIMES
    ProcessBasePriority, // s: KPRIORITY
    ProcessRaisePriority, // s: ULONG
    ProcessDebugPort, // q: HANDLE
    ProcessExceptionPort, // s: HANDLE
    ProcessAccessToken, // s: PROCESS_ACCESS_TOKEN
    ProcessLdtInformation, // 10
    ProcessLdtSize,
    ProcessDefaultHardErrorMode, // qs: ULONG
    ProcessIoPortHandlers,
    ProcessPooledUsageAndLimits, // q: POOLED_USAGE_AND_LIMITS
    ProcessWorkingSetWatch, // q: var PROCESS_WS_WATCH_INFORMATION[]; s: void
    ProcessUserModeIOPL,
    ProcessEnableAlignmentFaultFixup, // s: BOOLEAN
    ProcessPriorityClass, // qs: PROCESS_PRIORITY_CLASS
    ProcessWx86Information,
    ProcessHandleCount, // 20, q: ULONG, PROCESS_HANDLE_COUNT_EX
    ProcessAffinityMask, // s: KAFFINITY
    ProcessPriorityBoost, // qs: ULONG
    ProcessDeviceMap,
    ProcessSessionInformation, // q: PROCESS_SESSION_INFORMATION
    ProcessForegroundInformation, // s: PROCESS_FOREGROUND_BACKGROUND
    ProcessWow64Information, // q: ULONG_PTR
    ProcessImageFileName, // q: var UNICODE_STRING
    ProcessLUIDDeviceMapsEnabled, // q: ULONG
    ProcessBreakOnTermination, // qs: ULONG
    ProcessDebugObjectHandle, // 30, q: HANDLE
    ProcessDebugFlags, // qs: ULONG
    ProcessHandleTracing, // q: var PROCESS_HANDLE_TRACING_QUERY; s: size 0 disables, otherwise enables
    ProcessIoPriority, // qs: ULONG
    ProcessExecuteFlags, // qs: ULONG
    ProcessResourceManagement,
    ProcessCookie, // q: ULONG
    ProcessImageInformation, // q: SECTION_IMAGE_INFORMATION
    ProcessCycleTime, // q: PROCESS_CYCLE_TIME_INFORMATION
    ProcessPagePriority, // q: ULONG
    ProcessInstrumentationCallback, // 40
    ProcessThreadStackAllocation,
    ProcessWorkingSetWatchEx, // q: var PROCESS_WS_WATCH_INFORMATION_EX[]
    ProcessImageFileNameWin32, // q: var UNICODE_STRING
    ProcessImageFileMapping,
    ProcessAffinityUpdateMode,
    ProcessMemoryAllocationMode,
    ProcessGroupInformation, // q: var USHORT[]
    ProcessTokenVirtualizationEnabled,
    ProcessConsoleHostProcess,
    ProcessWindowInformation, // 50
    MaxProcessInfoClass
} PROCESS_INFORMATION_CLASS;

typedef enum _THREAD_INFORMATION_CLASS
{
    ThreadBasicInformation, // q: THREAD_BASIC_INFORMATION
    ThreadTimes, // q: KERNEL_USER_TIMES
    ThreadPriority, // s: KPRIORITY
    ThreadBasePriority, // s: LONG
    ThreadAffinityMask, // s: KAFFINITY
    ThreadImpersonationToken, // s: HANDLE
    ThreadDescriptorTableEntry,
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
    ThreadIsTerminated, // 20, q: ULONG
    ThreadLastSystemCall,
    ThreadIoPriority, // qs: ULONG
    ThreadCycleTime, // q: THREAD_CYCLE_TIME_INFORMATION
    ThreadPagePriority, // q: ULONG
    ThreadActualBasePriority,
    ThreadTebInformation,
    ThreadCSwitchMon,
    ThreadCSwitchPmu,
    ThreadWow64Context, // q: WOW64_CONTEXT
    ThreadGroupInformation, // 30, q: GROUP_AFFINITY
    ThreadUmsInformation,
    ThreadCounterProfiling,
    ThreadIdealProcessorEx,
    MaxThreadInfoClass
} THREAD_INFORMATION_CLASS;

// Use with both ProcessPagePriority and ThreadPagePriority
// ntddk
typedef struct _PAGE_PRIORITY_INFORMATION
{
    ULONG PagePriority;
} PAGE_PRIORITY_INFORMATION, *PPAGE_PRIORITY_INFORMATION;

// Process information structures

typedef struct _PROCESS_BASIC_INFORMATION
{
    NTSTATUS ExitStatus;
    PPEB PebBaseAddress;
    ULONG_PTR AffinityMask;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION;
typedef PROCESS_BASIC_INFORMATION *PPROCESS_BASIC_INFORMATION;

// ntddk
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
            ULONG SpareBits : 28;
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

typedef struct _PROCESS_WS_WATCH_INFORMATION
{
    PVOID FaultingPc;
    PVOID FaultingVa;
} PROCESS_WS_WATCH_INFORMATION, *PPROCESS_WS_WATCH_INFORMATION;

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

// begin_rev

#ifndef PROCESS_AFFINITY_ENABLE_AUTO_UPDATE
#define PROCESS_AFFINITY_ENABLE_AUTO_UPDATE 0x1
#endif
#define PROCESS_AFFINITY_PERMANENT 0x2

typedef struct _PROCESS_AFFINITY_UPDATE_MODE_INFORMATION
{
    ULONG AffinityUpdateMode;
} PROCESS_AFFINITY_UPDATE_MODE_INFORMATION, *PPROCESS_AFFINITY_UPDATE_MODE_INFORMATION;

// end_rev

// begin_rev

#define PROCESS_MEMORY_VM_TOP_DOWN 0x1

typedef struct _PROCESS_MEMORY_ALLOCATION_MODE_INFORMATION
{
    ULONG MemoryAllocationMode;
} PROCESS_MEMORY_ALLOCATION_MODE_INFORMATION, *PPROCESS_MEMORY_ALLOCATION_MODE_INFORMATION;

// end_rev

// rev
typedef struct _PROCESS_HANDLE_COUNT_EX
{
    ULONG HandleCount;
    ULONG PeakHandleCount;
} PROCESS_HANDLE_COUNT_EX, *PPROCESS_HANDLE_COUNT_EX;

// rev
typedef struct _PROCESS_CONSOLE_HOST_PROCESS_INFORMATION
{
    ULONG_PTR ConsoleHostProcess;
} PROCESS_CONSOLE_HOST_PROCESS_INFORMATION, *PPROCESS_CONSOLE_HOST_PROCESS_INFORMATION;

// rev
typedef struct _PROCESS_CYCLE_TIME_INFORMATION
{
    ULARGE_INTEGER AccumulatedCycles;
    ULARGE_INTEGER CurrentCycleCount;
} PROCESS_CYCLE_TIME_INFORMATION, *PPROCESS_CYCLE_TIME_INFORMATION;

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

// rev
typedef struct _THREAD_CYCLE_TIME_INFORMATION
{
    ULARGE_INTEGER AccumulatedCycles;
    ULARGE_INTEGER CurrentCycleCount;
} THREAD_CYCLE_TIME_INFORMATION, *PTHREAD_CYCLE_TIME_INFORMATION;

// System calls

// Processes

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateProcess(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in HANDLE ParentProcess,
    __in BOOLEAN InheritObjectTable,
    __in_opt HANDLE SectionHandle,
    __in_opt HANDLE DebugPort,
    __in_opt HANDLE ExceptionPort
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
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in HANDLE ParentProcess,
    __in ULONG Flags,
    __in_opt HANDLE SectionHandle,
    __in_opt HANDLE DebugPort,
    __in_opt HANDLE ExceptionPort,
    __in ULONG JobMemberLevel
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenProcess(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt PCLIENT_ID ClientId
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTerminateProcess(
    __in_opt HANDLE ProcessHandle,
    __in NTSTATUS ExitStatus
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSuspendProcess(
    __in HANDLE ProcessHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtResumeProcess(
    __in HANDLE ProcessHandle
    );

#define NtCurrentProcess() ((HANDLE)(LONG_PTR)-1)
#define ZwCurrentProcess() NtCurrentProcess()
#define NtCurrentThread() ((HANDLE)(LONG_PTR)-2)
#define ZwCurrentThread() NtCurrentThread()
#define NtCurrentPeb() (NtCurrentTeb()->ProcessEnvironmentBlock)

#define NtCurrentProcessId() (NtCurrentTeb()->ClientId.UniqueProcess)
#define NtCurrentThreadId() (NtCurrentTeb()->ClientId.UniqueThread)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationProcess(
    __in HANDLE ProcessHandle,
    __in PROCESS_INFORMATION_CLASS ProcessInformationClass,
    __out_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __out_opt PULONG ReturnLength
    );

#if (PHNT_VERSION >= PHNT_WS03)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetNextProcess(
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __in ULONG Flags,
    __out PHANDLE NewProcessHandle
    );
#else
typedef NTSTATUS (NTAPI *_NtGetNextProcess)(
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __in ULONG Flags,
    __out PHANDLE NewProcessHandle
    );
#endif

#if (PHNT_VERSION >= PHNT_WS03)
NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetNextThread(
    __in HANDLE ProcessHandle,
    __in HANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __in ULONG Flags,
    __out PHANDLE NewThreadHandle
    );
#else
typedef NTSTATUS (NTAPI *_NtGetNextThread)(
    __in HANDLE ProcessHandle,
    __in HANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __in ULONG Flags,
    __out PHANDLE NewThreadHandle
    );
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationProcess(
    __in HANDLE ProcessHandle,
    __in PROCESS_INFORMATION_CLASS ProcessInformationClass,
    __in_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryPortInformationProcess();

// Threads

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateThread(
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in HANDLE ProcessHandle,
    __out PCLIENT_ID ClientId,
    __in PCONTEXT ThreadContext,
    __in PINITIAL_TEB InitialTeb,
    __in BOOLEAN CreateSuspended
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenThread(
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in_opt PCLIENT_ID ClientId
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTerminateThread(
    __in_opt HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSuspendThread(
    __in HANDLE ThreadHandle,
    __out_opt PULONG PreviousSuspendCount
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtResumeThread(
    __in HANDLE ThreadHandle,
    __out_opt PULONG PreviousSuspendCount
    );

NTSYSCALLAPI
ULONG
NTAPI
NtGetCurrentProcessorNumber();

NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetContextThread(
    __in HANDLE ThreadHandle,
    __inout PCONTEXT ThreadContext
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetContextThread(
    __in HANDLE ThreadHandle,
    __in PCONTEXT ThreadContext
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationThread(
    __in HANDLE ThreadHandle,
    __in THREAD_INFORMATION_CLASS ThreadInformationClass,
    __out_bcount(ThreadInformationLength) PVOID ThreadInformation,
    __in ULONG ThreadInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationThread(
    __in HANDLE ThreadHandle,
    __in THREAD_INFORMATION_CLASS ThreadInformationClass,
    __in_bcount(ThreadInformationLength) PVOID ThreadInformation,
    __in ULONG ThreadInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlertThread(
    __in HANDLE ThreadHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAlertResumeThread(
    __in HANDLE ThreadHandle,
    __out_opt PULONG PreviousSuspendCount
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTestAlert();

NTSYSCALLAPI
NTSTATUS
NTAPI
NtImpersonateThread(
    __in HANDLE ServerThreadHandle,
    __in HANDLE ClientThreadHandle,
    __in PSECURITY_QUALITY_OF_SERVICE SecurityQos
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRegisterThreadTerminatePort(
    __in HANDLE PortHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetLdtEntries(
    __in ULONG Selector0,
    __in ULONG Entry0Low,
    __in ULONG Entry0Hi,
    __in ULONG Selector1,
    __in ULONG Entry1Low,
    __in ULONG Entry1Hi
    );

typedef VOID (*PPS_APC_ROUTINE)(
    __in_opt PVOID ApcArgument1,
    __in_opt PVOID ApcArgument2,
    __in_opt PVOID ApcArgument3
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueueApcThread(
    __in HANDLE ThreadHandle,
    __in PPS_APC_ROUTINE ApcRoutine,
    __in_opt PVOID ApcArgument1,
    __in_opt PVOID ApcArgument2,
    __in_opt PVOID ApcArgument3
    );

// User processes and threads

typedef struct _RTL_USER_PROCESS_PARAMETERS *PRTL_USER_PROCESS_PARAMETERS;

// begin_rev

// Attributes

#define PROCESS_ATTRIBUTE_NUMBER_MASK 0x0000ffff
#define PROCESS_ATTRIBUTE_THREAD 0x00010000 // can be used with threads
#define PROCESS_ATTRIBUTE_INPUT 0x00020000 // input only
#define PROCESS_ATTRIBUTE_UNKNOWN 0x00040000

typedef enum _PROCESS_ATTRIBUTE_NUMBER
{
    ProcessAttributeParentProcess = 0, // in HANDLE
    ProcessAttributeDebugPort = 1, // in HANDLE
    ProcessAttributeToken = 2, // in HANDLE
    ProcessAttributeClientId = 3, // out PCLIENT_ID
    ProcessAttributeTebBaseAddress = 4, // out PTEB *
    ProcessAttributeImagePath = 5, // in PWSTR
    ProcessAttributeImageInformation = 6, // out PSECTION_IMAGE_INFORMATION
    ProcessAttributeMemoryReserveList = 7, // in PPROCESS_MEMORY_RESERVE_RANGE
    ProcessAttributePriorityClass = 8, // in UCHAR
    ProcessAttributeDefaultHardErrorProcessing = 9, // in ULONG
    ProcessAttributeExtendedOptions = 10, // in PPROCESS_EXTENDED_OPTIONS
    ProcessAttributeHandleList = 11, // in PHANDLE
    ProcessAttributeGroupAffinity = 12, // in PGROUP_AFFINITY
    ProcessAttributePreferredNode = 13, // in PUSHORT
    ProcessAttributeIdealProcessor = 14, // in PPROCESSOR_NUMBER
    ProcessAttributeUmsThread = 15, // ? in PUMS_CREATE_THREAD_ATTRIBUTES
    ProcessAttributeExecuteOptions = 16, // in UCHAR
    ProcessAttributeMaximum
} PROCESS_ATTRIBUTE_NUMBER;

#define ProcessAttributeValue(Number, Thread, Input, Unknown) \
    (((Number) & PROCESS_ATTRIBUTE_NUMBER_MASK) | \
    ((Thread) ? PROCESS_ATTRIBUTE_THREAD : 0) | \
    ((Input) ? PROCESS_ATTRIBUTE_INPUT : 0) | \
    ((Unknown) ? PROCESS_ATTRIBUTE_UNKNOWN : 0))

#define PROCESS_ATTRIBUTE_PARENT_PROCESS \
    ProcessAttributeValue(ProcessAttributeParentProcess, FALSE, TRUE, TRUE)
#define PROCESS_ATTRIBUTE_DEBUG_PORT \
    ProcessAttributeValue(ProcessAttributeDebugPort, FALSE, TRUE, TRUE)
#define PROCESS_ATTRIBUTE_TOKEN \
    ProcessAttributeValue(ProcessAttributeToken, FALSE, TRUE, TRUE)
#define PROCESS_ATTRIBUTE_CLIENT_ID \
    ProcessAttributeValue(ProcessAttributeClientId, TRUE, FALSE, FALSE)
#define PROCESS_ATTRIBUTE_TEB_BASE_ADDRESS \
    ProcessAttributeValue(ProcessAttributeTebBaseAddress, TRUE, FALSE, FALSE)
#define PROCESS_ATTRIBUTE_IMAGE_INFORMATION \
    ProcessAttributeValue(ProcessAttributeImageInformation, FALSE, FALSE, FALSE)
#define PROCESS_ATTRIBUTE_MEMORY_RESERVE_LIST \
    ProcessAttributeValue(ProcessAttributeMemoryReserveList, FALSE, TRUE, FALSE)
#define PROCESS_ATTRIBUTE_IMAGE_PATH \
    ProcessAttributeValue(ProcessAttributeImagePath, FALSE, TRUE, FALSE)
#define PROCESS_ATTRIBUTE_PRIORITY_CLASS \
    ProcessAttributeValue(ProcessAttributePriorityClass, FALSE, TRUE, FALSE)
#define PROCESS_ATTRIBUTE_DEFAULT_HARD_ERROR_PROCESSING \
    ProcessAttributeValue(ProcessAttributeDefaultHardErrorProcessing, FALSE, TRUE, FALSE)
#define PROCESS_ATTRIBUTE_HANDLE_LIST \
    ProcessAttributeValue(ProcessAttributeHandleList, FALSE, TRUE, FALSE)
#define PROCESS_ATTRIBUTE_GROUP_AFFINITY \
    ProcessAttributeValue(ProcessAttributeGroupAffinity, TRUE, TRUE, FALSE)
#define PROCESS_ATTRIBUTE_PREFERRED_NODE \
    ProcessAttributeValue(ProcessAttributePreferredNode, FALSE, TRUE, FALSE)
#define PROCESS_ATTRIBUTE_IDEAL_PROCESSOR \
    ProcessAttributeValue(ProcessAttributeIdealProcessor, TRUE, TRUE, FALSE)
#define PROCESS_ATTRIBUTE_EXECUTE_OPTIONS \
    ProcessAttributeValue(ProcessAttributeExecuteOptions, FALSE, TRUE, TRUE)

typedef struct _PROCESS_ATTRIBUTE
{
    ULONG Attribute;
    SIZE_T Length;
    PVOID Value;
    PSIZE_T ReturnLength;
} PROCESS_ATTRIBUTE, *PPROCESS_ATTRIBUTE;

typedef struct _PROCESS_ATTRIBUTE_LIST
{
    SIZE_T Size;
    PROCESS_ATTRIBUTE Attributes[1];
} PROCESS_ATTRIBUTE_LIST, *PPROCESS_ATTRIBUTE_LIST;

typedef struct _PROCESS_MEMORY_RESERVE_RANGE
{
    PVOID BaseAddress;
    SIZE_T RegionSize;
} PROCESS_MEMORY_RESERVE_RANGE, *PPROCESS_MEMORY_RESERVE_RANGE;

#define PROCESS_INHERIT_STANDARD_INPUT_HANDLE 0x1
#define PROCESS_INHERIT_STANDARD_OUTPUT_HANDLE 0x2
#define PROCESS_INHERIT_STANDARD_ERROR_HANDLE 0x4

typedef struct _PROCESS_EXTENDED_OPTIONS
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG ImageStandardHandlesOptions : 2; // can't be 3
            ULONG StandardHandlesInheritMask : 3; // PROCESS_INHERIT_STANDARD_*
            ULONG Reserved : 27;
        };
    };
    ULONG SubSystemType;
} PROCESS_EXTENDED_OPTIONS, *PPROCESS_EXTENDED_OPTIONS;

typedef enum _PROCESS_CREATE_STATUS
{
    ProcessCreateStatusInvalid = 0,
    ProcessCreateStatusCantOpenFile = 1,
    ProcessCreateStatusCantCreateSection = 2,
    ProcessCreateStatusBadImage = 3,
    ProcessCreateStatusBadImageMachine = 4,
    ProcessCreateStatusDebuggerNeeded = 5,
    ProcessCreateStatusCreated = 6,
    MaximumProcessCreateStatus
} PROCESS_CREATE_STATUS;

typedef struct _PROCESS_CREATE_INFO_INPUT
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG FillCreateInfo : 1;
            ULONG LocateManifest : 1;
            ULONG Flags2 : 6;
            ULONG ImageFileOptionsLevel : 2;
            ULONG Flags10 : 6;
            ULONG ExtendedFlagsHiWord : 16;
        };
    };
    ACCESS_MASK FileDesiredAccess;
} PROCESS_CREATE_INFO_INPUT;

typedef struct _PROCESS_CREATE_INFO_CREATED
{
    union
    {
        ULONG Flags;
        struct
        {
            ULONG IsProtectedProcess : 1;
            ULONG OverrideAddressSpace : 1;
            ULONG DevOverrideEnable : 1; // from Image File Execution Options
            ULONG ManifestLocated : 1;
            ULONG Reserved : 28;
        };
    };
    HANDLE FileHandle;
    HANDLE SectionHandle;
    ULONG Unknown1;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    ULONG Reserved1;
    ULONG Unknown3;
    ULONG FlagsInProcessParameters;
    PPEB PebBaseAddress;
    ULONG Reserved2;
    ULONG Unknown4;
    ULONG Unknown5;
    LARGE_INTEGER ManifestResource;
    PVOID ManifestResource2;
    ULONG Unknown8;
} PROCESS_CREATE_INFO_CREATED;

typedef struct _PROCESS_CREATE_INFO
{
    SIZE_T Size;
    PROCESS_CREATE_STATUS Status;
    union
    {
        PROCESS_CREATE_INFO_INPUT Input;
        struct
        {
            HANDLE FileHandle;
        } CantCreateSection;
        struct
        {
            HANDLE ImageFileOptionsKeyHandle;
        } DebuggerNeeded;
        PROCESS_CREATE_INFO_CREATED Created;
    };
} PROCESS_CREATE_INFO, *PPROCESS_CREATE_INFO;

#if (PHNT_VERSION >= PHNT_VISTA)

// Extended PROCESS_CREATE_FLAGS_*
#define PROCESS_CREATE_FLAGS_LARGE_PAGE_SYSTEM_DLL 0x00000020
#define PROCESS_CREATE_FLAGS_PROTECTED_PROCESS 0x00000040
#define PROCESS_CREATE_FLAGS_CREATE_SESSION 0x00000080 // ?
#define PROCESS_CREATE_FLAGS_INHERIT_FROM_PARENT 0x00000100

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateUserProcess(
    __out PHANDLE ProcessHandle,
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK ProcessDesiredAccess,
    __in ACCESS_MASK ThreadDesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ProcessObjectAttributes,
    __in_opt POBJECT_ATTRIBUTES ThreadObjectAttributes,
    __in ULONG ProcessFlags, // PROCESS_CREATE_FLAGS_*
    __in ULONG ThreadFlags, // THREAD_CREATE_FLAGS_*
    __in_opt PRTL_USER_PROCESS_PARAMETERS ProcessParameters,
    __inout PPROCESS_CREATE_INFO CreateInfo,
    __in_opt PPROCESS_ATTRIBUTE_LIST AttributeList
    );

#define THREAD_CREATE_FLAGS_CREATE_SUSPENDED 0x00000001
#define THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH 0x00000002 // ?
#define THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER 0x00000004
#define THREAD_CREATE_FLAGS_HAS_SECURITY_DESCRIPTOR 0x00000010 // ?
#define THREAD_CREATE_FLAGS_ACCESS_CHECK_IN_TARGET 0x00000020 // ?
#define THREAD_CREATE_FLAGS_INITIAL_THREAD 0x00000080

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateThreadEx(
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in HANDLE ProcessHandle,
    __in PVOID StartRoutine,
    __in_opt PVOID StartContext,
    __in ULONG Flags, // THREAD_CREATE_FLAGS_*
    __in_opt ULONG_PTR StackZeroBits,
    __in_opt SIZE_T StackCommit,
    __in_opt SIZE_T StackReserve,
    __in_opt PPROCESS_ATTRIBUTE_LIST AttributeList
    );

#endif

// end_rev

// Reserve objects

// begin_rev

#if (PHNT_VERSION >= PHNT_WIN7)

#define USER_APC_RESERVE_TYPE 0
#define IO_COMPLETION_RESERVE_TYPE 1

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateReserveObject(
    __out PHANDLE MemoryReserveHandle,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in ULONG Type
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueueApcThreadEx(
    __in HANDLE ThreadHandle,
    __in_opt HANDLE UserApcReserveHandle,
    __in PPS_APC_ROUTINE ApcRoutine,
    __in_opt PVOID ApcArgument1,
    __in_opt PVOID ApcArgument2,
    __in_opt PVOID ApcArgument3
    );

#endif

// end_rev

// Job Objects

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateJobObject(
    __out PHANDLE JobHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenJobObject(
    __out PHANDLE JobHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAssignProcessToJobObject(
    __in HANDLE JobHandle,
    __in HANDLE ProcessHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtTerminateJobObject(
    __in HANDLE JobHandle,
    __in NTSTATUS ExitStatus
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtIsProcessInJob(
    __in HANDLE ProcessHandle,
    __in_opt HANDLE JobHandle
    ); 

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationJobObject(
    __in_opt HANDLE JobHandle,
    __in JOBOBJECTINFOCLASS JobObjectInformationClass,
    __out_bcount(JobObjectInformationLength) PVOID JobObjectInformation,
    __in ULONG JobObjectInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationJobObject(
    __in HANDLE JobHandle,
    __in JOBOBJECTINFOCLASS JobObjectInformationClass,
    __in_bcount(JobObjectInformationLength) PVOID JobObjectInformation,
    __in ULONG JobObjectInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateJobSet(
    __in ULONG NumJob,
    __in_ecount(NumJob) PJOB_SET_ARRAY UserJobSet,
    __in ULONG Flags
    );

#endif
