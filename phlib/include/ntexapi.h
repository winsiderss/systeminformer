#ifndef _NTEXAPI_H
#define _NTEXAPI_H

#include <ntkeapi.h>

// Thread execution

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDelayExecution(
    __in BOOLEAN Alertable,
    __in PLARGE_INTEGER DelayInterval
    );

// Environment values

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemEnvironmentValue(
    __in PUNICODE_STRING VariableName,
    __out_bcount(ValueLength) PWSTR VariableValue,
    __in USHORT ValueLength,
    __out_opt PUSHORT ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemEnvironmentValue(
    __in PUNICODE_STRING VariableName,
    __in PUNICODE_STRING VariableValue
    );

// Event

#ifndef EVENT_QUERY_STATE
#define EVENT_QUERY_STATE 0x0001
#endif

typedef enum _EVENT_INFORMATION_CLASS
{
    EventBasicInformation
} EVENT_INFORMATION_CLASS;

typedef struct _EVENT_BASIC_INFORMATION
{
    EVENT_TYPE EventType;
    LONG EventState;
} EVENT_BASIC_INFORMATION, *PEVENT_BASIC_INFORMATION;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateEvent(
    __out PHANDLE EventHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in EVENT_TYPE EventType,
    __in BOOLEAN InitialState
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenEvent(
    __out PHANDLE EventHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetEvent(
    __in HANDLE EventHandle,
    __out_opt PLONG PreviousState
    );

typedef NTSTATUS (NTAPI *_NtSetEventBoostPriority)(
    __in HANDLE EventHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtClearEvent(
    __in HANDLE EventHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtResetEvent(
    __in HANDLE EventHandle,
    __out_opt PLONG PreviousState
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPulseEvent(
    __in HANDLE EventHandle,
    __out_opt PLONG PreviousState
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryEvent(
    __in HANDLE EventHandle,
    __in EVENT_INFORMATION_CLASS EventInformationClass,
    __out_bcount(EventInformationLength) PVOID EventInformation,
    __in ULONG EventInformationLength,
    __out_opt PULONG ReturnLength
    );

// Event Pair

#define EVENT_PAIR_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateEventPair(
    __out PHANDLE EventPairHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenEventPair(
    __out PHANDLE EventPairHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetLowEventPair(
    __in HANDLE EventPairHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetHighEventPair(
    __in HANDLE EventPairHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitLowEventPair( 
    __in HANDLE EventPairHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitHighEventPair(
    __in HANDLE EventPairHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetLowWaitHighEventPair(
    __in HANDLE EventPairHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetHighWaitLowEventPair(
    __in HANDLE EventPairHandle
    );

// Mutant

typedef enum _MUTANT_INFORMATION_CLASS
{
    MutantBasicInformation,
    MutantOwnerInformation
} MUTANT_INFORMATION_CLASS;

typedef struct _MUTANT_BASIC_INFORMATION 
{
    LONG CurrentCount;
    BOOLEAN OwnedByCaller;
    BOOLEAN AbandonedState;
} MUTANT_BASIC_INFORMATION, *PMUTANT_BASIC_INFORMATION;

typedef struct _MUTANT_OWNER_INFORMATION
{
    CLIENT_ID ClientId;
} MUTANT_OWNER_INFORMATION, *PMUTANT_OWNER_INFORMATION;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateMutant(
    __out PHANDLE MutantHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in BOOLEAN InitialOwner
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenMutant(
    __out PHANDLE MutantHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReleaseMutant(
    __in HANDLE MutantHandle,
    __out_opt PLONG PreviousCount
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryMutant(
    __in HANDLE MutantHandle,
    __in MUTANT_INFORMATION_CLASS MutantInformationClass,
    __out_bcount(MutantInformationLength) PVOID MutantInformation,
    __in ULONG MutantInformationLength,
    __out_opt PULONG ReturnLength
    );

// Semaphore

#ifndef SEMAPHORE_QUERY_STATE
#define SEMAPHORE_QUERY_STATE 0x0001
#endif

typedef enum _SEMAPHORE_INFORMATION_CLASS
{
    SemaphoreBasicInformation
} SEMAPHORE_INFORMATION_CLASS;

typedef struct _SEMAPHORE_BASIC_INFORMATION
{
    LONG CurrentCount;
    LONG MaximumCount;
} SEMAPHORE_BASIC_INFORMATION, *PSEMAPHORE_BASIC_INFORMATION;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateSemaphore(
    __out PHANDLE SemaphoreHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in LONG InitialCount,
    __in LONG MaximumCount
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenSemaphore(
    __out PHANDLE SemaphoreHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReleaseSemaphore(
    __in HANDLE SemaphoreHandle,
    __in LONG ReleaseCount,
    __out_opt PLONG PreviousCount
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySemaphore(
    __in HANDLE SemaphoreHandle,
    __in SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
    __out_bcount(SemaphoreInformationLength) PVOID SemaphoreInformation,
    __in ULONG SemaphoreInformationLength,
    __out_opt PULONG ReturnLength
    );

// Timer

typedef enum _TIMER_INFORMATION_CLASS
{
    TimerBasicInformation
} TIMER_INFORMATION_CLASS;

typedef struct _TIMER_BASIC_INFORMATION
{
    LARGE_INTEGER RemainingTime;
    BOOLEAN TimerState;
} TIMER_BASIC_INFORMATION, *PTIMER_BASIC_INFORMATION;

typedef VOID (NTAPI *PTIMER_APC_ROUTINE)(
    __in PVOID TimerContext,
    __in ULONG TimerLowValue,
    __in LONG TimerHighValue
    );

// begin_ntddk

typedef enum _TIMER_SET_INFORMATION_CLASS
{
    TimerSetCoalescableTimer,
    MaxTimerInfoClass
} TIMER_SET_INFORMATION_CLASS;

#if (PHNT_VERSION >= PHNT_WIN7)
struct _COUNTED_REASON_CONTEXT;

typedef struct _TIMER_SET_COALESCABLE_TIMER_INFO
{
    __in LARGE_INTEGER DueTime;
    __in_opt PTIMER_APC_ROUTINE TimerApcRoutine;
    __in_opt PVOID TimerContext;
    __in_opt struct _COUNTED_REASON_CONTEXT *WakeContext;
    __in_opt ULONG Period;
    __in ULONG TolerableDelay;
    __out_opt PBOOLEAN PreviousState;
} TIMER_SET_COALESCABLE_TIMER_INFO, *PTIMER_SET_COALESCABLE_TIMER_INFO;
#endif

// end_ntddk

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateTimer(
    __out PHANDLE TimerHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in TIMER_TYPE TimerType
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenTimer(
    __out PHANDLE TimerHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetTimer(
    __in HANDLE TimerHandle,
    __in PLARGE_INTEGER DueTime,
    __in_opt PTIMER_APC_ROUTINE TimerApcRoutine,
    __in_opt PVOID TimerContext,
    __in BOOLEAN ResumeTimer,
    __in_opt LONG Period,
    __out_opt PBOOLEAN PreviousState
    );

#if (PHNT_VERSION >= PHNT_WIN7)
// ntddk:ZwSetTimerEx
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetTimerEx(
    __in HANDLE TimerHandle,
    __in TIMER_SET_INFORMATION_CLASS TimerSetInformationClass,
    __inout_bcount_opt(TimerSetInformationLength) PVOID TimerSetInformation,
    __in ULONG TimerSetInformationLength
    );
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCancelTimer(
    __in HANDLE TimerHandle,
    __out_opt PBOOLEAN CurrentState
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryTimer(
    __in HANDLE TimerHandle,
    __in TIMER_INFORMATION_CLASS TimerInformationClass,
    __out_bcount(TimerInformationLength) PVOID TimerInformation,
    __in ULONG TimerInformationLength,
    __out_opt PULONG ReturnLength
    );

// Profile

#define PROFILE_CONTROL 0x0001
#define PROFILE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | PROFILE_CONTROL)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateProfile(
    __out PHANDLE ProfileHandle,
    __in_opt HANDLE Process,
    __in PVOID ProfileBase,
    __in SIZE_T ProfileSize,
    __in ULONG BucketSize,
    __in PULONG Buffer,
    __in ULONG BufferSize,
    __in KPROFILE_SOURCE ProfileSource,
    __in KAFFINITY Affinity
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtStartProfile(
    __in HANDLE ProfileHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtStopProfile(
    __in HANDLE ProfileHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryIntervalProfile(
    __in KPROFILE_SOURCE ProfileSource,
    __out PULONG Interval
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetIntervalProfile(
    __in ULONG Interval,
    __in KPROFILE_SOURCE Source
    );

// Keyed Event

#define KEYEDEVENT_WAIT 0x0001
#define KEYEDEVENT_WAKE 0x0002
#define KEYEDEVENT_ALL_ACCESS \
    (STANDARD_RIGHTS_REQUIRED | KEYEDEVENT_WAIT | KEYEDEVENT_WAKE)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateKeyedEvent(
    __out PHANDLE KeyedEventHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in ULONG Flags
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtOpenKeyedEvent(
    __out PHANDLE KeyedEventHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReleaseKeyedEvent(
    __in HANDLE KeyedEventHandle,
    __in PVOID KeyValue,
    __in BOOLEAN Alertable,
    __in_opt PLARGE_INTEGER Timeout
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForKeyedEvent(
    __in HANDLE KeyedEventHandle,
    __in PVOID KeyValue,
    __in BOOLEAN Alertable,
    __in_opt PLARGE_INTEGER Timeout
    );

// Worker factory

#if (PHNT_VERSION >= PHNT_VISTA)

// begin_rev

typedef enum _WORKER_FACTORY_INFORMATION_CLASS
{
    WorkerFactoryBasicInformation,
    MaxWorkerFactoryInfoClass
} WORKER_FACTORY_INFORMATION_CLASS, *PWORKER_FACTORY_INFORMATION_CLASS;

// ?
typedef struct _WORKER_WORK
{
    PVOID Unknown1;
    PVOID Unknown2;
    PVOID Unknown3;
    PVOID Unknown4;
} WORKER_WORK, *PWORKER_WORK;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCreateWorkerFactory(
    __out PHANDLE WorkerFactoryHandle,
    __in ACCESS_MASK DesiredAccess,
    __in_opt POBJECT_ATTRIBUTES ObjectAttributes,
    __in HANDLE IoCompletionHandle,
    __in HANDLE ProcessHandle,
    __in PVOID WorkerThreadStart,
    __in_opt PVOID WorkerThreadContext,
    __in_opt ULONG MaximumCount, // unsure
    __in_opt SIZE_T SizeOfStackReserve,
    __in_opt SIZE_T SizeOfStackCommit
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationWorkerFactory(
    __in HANDLE WorkerFactoryHandle,
    __in WORKER_FACTORY_INFORMATION_CLASS WorkerFactoryInformationClass,
    __out_bcount(WorkerFactoryInformationLength) PVOID WorkerFactoryInformation,
    __in ULONG WorkerFactoryInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetInformationWorkerFactory(
    __in HANDLE WorkerFactoryHandle,
    __in WORKER_FACTORY_INFORMATION_CLASS WorkerFactoryInformationClass,
    __in_bcount(WorkerFactoryInformationLength) PVOID WorkerFactoryInformation,
    __in ULONG WorkerFactoryInformationLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtShutdownWorkerFactory(
    __in HANDLE WorkerFactoryHandle,
    __inout PULONG RefCount // unsure
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtReleaseWorkerFactoryWorker(
    __in HANDLE WorkerFactoryHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWorkerFactoryWorkerReady(
    __in HANDLE WorkerFactoryHandle
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtWaitForWorkViaWorkerFactory(
    __in HANDLE WorkerFactoryHandle,
    __out PWORKER_WORK Work // ?
    );

// end_rev

#endif

// Time

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemTime(
    __out PLARGE_INTEGER SystemTime
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemTime(
    __in_opt PLARGE_INTEGER SystemTime,
    __out_opt PLARGE_INTEGER PreviousTime
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryTimerResolution(
    __out PULONG MaximumTime,
    __out PULONG MinimumTime,
    __out PULONG CurrentTime
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetTimerResolution(
    __in ULONG DesiredTime,
    __in BOOLEAN SetResolution,
    __out PULONG ActualTime
    );

// Performance Counter

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryPerformanceCounter(
    __out PLARGE_INTEGER PerformanceCounter,
    __out_opt PLARGE_INTEGER PerformanceFrequency
    );

// LUIDs

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateLocallyUniqueId(
    __out PLUID Luid
    );

// UUIDs

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetUuidSeed(
    __in PCHAR Seed
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAllocateUuids(
    __out PULARGE_INTEGER Time,
    __out PULONG Range,
    __out PULONG Sequence,
    __out PCHAR Seed
    );

// System Information

// begin_rev
// source:http://www.7747.net/Article/200905/38047.html
// source:GetProcessorSystemCycleTime
// source:http://www.microsoft.com/whdc/system/Sysinternals/MoreThan64proc.mspx

typedef enum _SYSTEM_INFORMATION_CLASS
{
    SystemBasicInformation,
    SystemProcessorInformation,
    SystemPerformanceInformation,
    SystemTimeOfDayInformation,
    SystemPathInformation,
    SystemProcessInformation,
    SystemCallCountInformation,
    SystemDeviceInformation,
    SystemProcessorPerformanceInformation,
    SystemFlagsInformation,
    SystemCallTimeInformation, // 10
    SystemModuleInformation,
    SystemLocksInformation,
    SystemStackTraceInformation,
    SystemPagedPoolInformation,
    SystemNonPagedPoolInformation,
    SystemHandleInformation,
    SystemObjectInformation,
    SystemPageFileInformation,
    SystemVdmInstemulInformation,
    SystemVdmBopInformation, // 20
    SystemFileCacheInformation,
    SystemPoolTagInformation,
    SystemInterruptInformation,
    SystemDpcBehaviorInformation,
    SystemFullMemoryInformation,
    SystemLoadGdiDriverInformation,
    SystemUnloadGdiDriverInformation,
    SystemTimeAdjustmentInformation,
    SystemSummaryMemoryInformation,
    SystemMirrorMemoryInformation, // 30
    SystemPerformanceTraceInformation,
    SystemCrashDumpInformation,
    SystemExceptionInformation,
    SystemCrashDumpStateInformation,
    SystemKernelDebuggerInformation,
    SystemContextSwitchInformation,
    SystemRegistryQuotaInformation,
    SystemExtendServiceTableInformation, // used to be SystemLoadAndCallImage
    SystemPrioritySeparation,
    SystemVerifierAddDriverInformation, // 40
    SystemVerifierRemoveDriverInformation,
    SystemProcessorIdleInformation,
    SystemLegacyDriverInformation,
    SystemCurrentTimeZoneInformation,
    SystemLookasideInformation,
    SystemTimeSlipNotification,
    SystemSessionCreate,
    SystemSessionDetach,
    SystemSessionInformation,
    SystemRangeStartInformation, // 50
    SystemVerifierInformation,
    SystemVerifierThunkExtend,
    SystemSessionProcessInformation,
    SystemLoadGdiDriverInSystemSpace,
    SystemNumaProcessorMap,
    SystemPrefetcherInformation, // PfSnQueryPrefetcherInformation
    SystemExtendedProcessInformation,
    SystemRecommendedSharedDataAlignment,
    SystemComPlusPackage,
    SystemNumaAvailableMemory, // 60
    SystemProcessorPowerInformation,
    SystemEmulationBasicInformation,
    SystemEmulationProcessorInformation,
    SystemExtendedHandleInformation,
    SystemLostDelayedWriteInformation,
    SystemBigPoolInformation,
    SystemSessionPoolTagInformation,
    SystemSessionMappedViewInformation,
    SystemHotpatchInformation,
    SystemObjectSecurityMode, // 70
    SystemWatchdogTimerHandler, // doesn't seem to be implemented
    SystemWatchdogTimerInformation,
    SystemLogicalProcessorInformation,
    SystemWow64SharedInformation,
    SystemRegisterFirmwareTableInformationHandler,
    SystemFirmwareTableInformation,
    SystemModuleInformationEx,
    SystemVerifierTriageInformation,
    SystemSuperfetchInformation, // PfQuerySuperfetchInformation
    SystemMemoryListInformation, // 80
    SystemFileCacheInformationEx,
    SystemNotImplemented19,
    SystemProcessorIdleCycleTimeInformation, // array of ULARGE_INTEGERs, one for each idle thread (in System Idle Process)
    SystemVerifierInformation2,
    SystemNotImplemented20,
    SystemRefTraceInformation, // ObQueryRefTraceInformation
    SystemSpecialPoolTagInformation, // MmSpecialPoolTag, then MmSpecialPoolCatchOverruns != 0
    SystemProcessImageNameInformation,
    SystemNotImplemented21,
    SystemBootEnvironmentInformation, // 90
    SystemEnlightenmentInformation,
    SystemVerifierInformationEx,
    SystemNotImplemented22,
    SystemNotImplemented23,
    SystemCovInformation, // ExpCovQueryInformation
    SystemNotImplemented24,
    SystemNotImplemented25,
    SystemPartitionDeviceNameInformation,
    SystemDiskDeviceNameInformation, // this and SystemPartitionInformation both call IoQuerySystemDeviceName
    SystemProcessorPerformanceDistribution, // 100
    SystemNumaProximityNodeInformation,
    SystemTimeZoneInformation2,
    SystemCodeIntegrityInformation, // SeCodeIntegrityQueryInformation
    SystemProcessorMicroCodeUpdateInformation,
    SystemProcessorBrandStringInformation, // HalDispatchTable -> HalpGetProcessorBrandString, info class 23
    SystemVaInformation, // MmQuerySystemVaInformation
    SystemLogicalProcessorRelationshipInformation, // KeQueryLogicalProcessorRelationship
    SystemProcessorCycleTimeInformation, // array of ULARGE_INTEGERs, one for each processor
    SystemStoreInformation, // SmQueryStoreInformation
    SystemRegistryAppendStringInformation, // 110
    SystemNotImplemented28,
    SystemVhdBootInformation,
    SystemCpuQuotaInformation, // PsQueryCpuQuotaInformation
    SystemNotImplemented29,
    SystemNotImplemented30,
    SystemLowPriorityIoInformation,
    SystemTpmBootEntropyInformation, // ExQueryTpmBootEntropyInformation
    SystemVerifierInformation3,
    SystemWorkingSetInformation, // MmQuerySystemWorkingSetInformation
    SystemWorkingSetInformation2, // 120
    SystemNumaDistanceInformation,
    SystemProcessorBrandStringInformation2, // HalDispatchTable -> HalpGetProcessorBrandString, info class 26
    SystemMemoryCommitmentInformation,
    MaxSystemInfoClass
} SYSTEM_INFORMATION_CLASS;

// end_rev

typedef struct _SYSTEM_BASIC_INFORMATION
{
    ULONG Reserved;
    ULONG TimerResolution;
    ULONG PageSize;
    ULONG NumberOfPhysicalPages;
    ULONG LowestPhysicalPageNumber;
    ULONG HighestPhysicalPageNumber;
    ULONG AllocationGranularity;
    ULONG_PTR MinimumUserModeAddress;
    ULONG_PTR MaximumUserModeAddress;
    ULONG_PTR ActiveProcessorsAffinityMask;
    CCHAR NumberOfProcessors;
} SYSTEM_BASIC_INFORMATION, *PSYSTEM_BASIC_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_INFORMATION
{
    USHORT ProcessorArchitecture;
    USHORT ProcessorLevel;
    USHORT ProcessorRevision;
    USHORT Reserved;
    ULONG ProcessorFeatureBits;
} SYSTEM_PROCESSOR_INFORMATION, *PSYSTEM_PROCESSOR_INFORMATION;

typedef struct _SYSTEM_TIMEOFDAY_INFORMATION
{
    LARGE_INTEGER BootTime;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER TimeZoneBias;
    ULONG TimeZoneId;
    ULONG Reserved;
    ULONGLONG BootTimeBias;
    ULONGLONG SleepTimeBias;
} SYSTEM_TIMEOFDAY_INFORMATION, *PSYSTEM_TIMEOFDAY_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION
{
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER DpcTime;
    LARGE_INTEGER InterruptTime;
    ULONG InterruptCount;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION, *PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;

typedef struct _SYSTEM_PROCESSOR_IDLE_INFORMATION
{
    ULONGLONG IdleTime;
    ULONGLONG C1Time;
    ULONGLONG C2Time;
    ULONGLONG C3Time;
    ULONG C1Transitions;
    ULONG C2Transitions;
    ULONG C3Transitions;
    ULONG Padding;
} SYSTEM_PROCESSOR_IDLE_INFORMATION, *PSYSTEM_PROCESSOR_IDLE_INFORMATION;

typedef struct _SYSTEM_PERFORMANCE_INFORMATION
{
    LARGE_INTEGER IdleProcessTime;
    LARGE_INTEGER IoReadTransferCount;
    LARGE_INTEGER IoWriteTransferCount;
    LARGE_INTEGER IoOtherTransferCount;
    ULONG IoReadOperationCount;
    ULONG IoWriteOperationCount;
    ULONG IoOtherOperationCount;
    ULONG AvailablePages;
    ULONG CommittedPages;
    ULONG CommitLimit;
    ULONG PeakCommitment;
    ULONG PageFaultCount;
    ULONG CopyOnWriteCount;
    ULONG TransitionCount;
    ULONG CacheTransitionCount;
    ULONG DemandZeroCount;
    ULONG PageReadCount;
    ULONG PageReadIoCount;
    ULONG CacheReadCount;
    ULONG CacheIoCount;
    ULONG DirtyPagesWriteCount;
    ULONG DirtyWriteIoCount;
    ULONG MappedPagesWriteCount;
    ULONG MappedWriteIoCount;
    ULONG PagedPoolPages;
    ULONG NonPagedPoolPages;
    ULONG PagedPoolAllocs;
    ULONG PagedPoolFrees;
    ULONG NonPagedPoolAllocs;
    ULONG NonPagedPoolFrees;
    ULONG FreeSystemPtes;
    ULONG ResidentSystemCodePage;
    ULONG TotalSystemDriverPages;
    ULONG TotalSystemCodePages;
    ULONG NonPagedPoolLookasideHits;
    ULONG PagedPoolLookasideHits;
    ULONG AvailablePagedPoolPages;
    ULONG ResidentSystemCachePage;
    ULONG ResidentPagedPoolPage;
    ULONG ResidentSystemDriverPage;
    ULONG CcFastReadNoWait;
    ULONG CcFastReadWait;
    ULONG CcFastReadResourceMiss;
    ULONG CcFastReadNotPossible;
    ULONG CcFastMdlReadNoWait;
    ULONG CcFastMdlReadWait;
    ULONG CcFastMdlReadResourceMiss;
    ULONG CcFastMdlReadNotPossible;
    ULONG CcMapDataNoWait;
    ULONG CcMapDataWait;
    ULONG CcMapDataNoWaitMiss;
    ULONG CcMapDataWaitMiss;
    ULONG CcPinMappedDataCount;
    ULONG CcPinReadNoWait;
    ULONG CcPinReadWait;
    ULONG CcPinReadNoWaitMiss;
    ULONG CcPinReadWaitMiss;
    ULONG CcCopyReadNoWait;
    ULONG CcCopyReadWait;
    ULONG CcCopyReadNoWaitMiss;
    ULONG CcCopyReadWaitMiss;
    ULONG CcMdlReadNoWait;
    ULONG CcMdlReadWait;
    ULONG CcMdlReadNoWaitMiss;
    ULONG CcMdlReadWaitMiss;
    ULONG CcReadAheadIos;
    ULONG CcLazyWriteIos;
    ULONG CcLazyWritePages;
    ULONG CcDataFlushes;
    ULONG CcDataPages;
    ULONG ContextSwitches;
    ULONG FirstLevelTbFills;
    ULONG SecondLevelTbFills;
    ULONG SystemCalls;
} SYSTEM_PERFORMANCE_INFORMATION, *PSYSTEM_PERFORMANCE_INFORMATION;

typedef struct _SYSTEM_THREAD_INFORMATION
{
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER CreateTime;
    ULONG WaitTime;
    PVOID StartAddress;
    CLIENT_ID ClientId;
    KPRIORITY Priority;
    LONG BasePriority;
    ULONG ContextSwitches;
    ULONG ThreadState;
    KWAIT_REASON WaitReason;
} SYSTEM_THREAD_INFORMATION, *PSYSTEM_THREAD_INFORMATION;

typedef struct _TEB *PTEB;

typedef struct _SYSTEM_EXTENDED_THREAD_INFORMATION
{
    SYSTEM_THREAD_INFORMATION ThreadInfo;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID Win32StartAddress;
    PTEB TebBaseAddress; /* Windows 7 and above */
    ULONG_PTR Reserved2;
    ULONG_PTR Reserved3;
    ULONG_PTR Reserved4;
} SYSTEM_EXTENDED_THREAD_INFORMATION, *PSYSTEM_EXTENDED_THREAD_INFORMATION;

typedef struct _SYSTEM_PROCESS_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER SpareLi1;
    LARGE_INTEGER SpareLi2;
    LARGE_INTEGER SpareLi3;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SessionId;
    ULONG_PTR PageDirectoryBase;
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
    SIZE_T PrivatePageCount;
    LARGE_INTEGER ReadOperationCount;
    LARGE_INTEGER WriteOperationCount;
    LARGE_INTEGER OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;
    SYSTEM_THREAD_INFORMATION Threads[1];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

typedef struct _SYSTEM_SESSION_PROCESS_INFORMATION
{
    ULONG SessionId;
    ULONG SizeOfBuf;
    PVOID Buffer;
} SYSTEM_SESSION_PROCESS_INFORMATION, *PSYSTEM_SESSION_PROCESS_INFORMATION;

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO
{
    USHORT UniqueProcessId;
    USHORT CreatorBackTraceIndex;
    UCHAR ObjectTypeIndex;
    UCHAR HandleAttributes;
    USHORT HandleValue;
    PVOID Object;
    ULONG GrantedAccess;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO, *PSYSTEM_HANDLE_TABLE_ENTRY_INFO;

typedef struct _SYSTEM_HANDLE_INFORMATION
{
    ULONG NumberOfHandles;
    SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[1];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

typedef struct _SYSTEM_REGISTRY_QUOTA_INFORMATION
{
    ULONG RegistryQuotaAllowed;
    ULONG RegistryQuotaUsed;
    SIZE_T PagedPoolSize;
} SYSTEM_REGISTRY_QUOTA_INFORMATION, *PSYSTEM_REGISTRY_QUOTA_INFORMATION;

typedef struct _SYSTEM_OBJECTTYPE_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG NumberOfObjects;
    ULONG NumberOfHandles;
    ULONG TypeIndex;
    ULONG InvalidAttributes;
    GENERIC_MAPPING GenericMapping;
    ULONG ValidAccessMask;
    ULONG PoolType;
    BOOLEAN SecurityRequired;
    BOOLEAN WaitableObject;
    UNICODE_STRING TypeName;
} SYSTEM_OBJECTTYPE_INFORMATION, *PSYSTEM_OBJECTTYPE_INFORMATION;

typedef struct _SYSTEM_OBJECT_INFORMATION
{
    ULONG NextEntryOffset;
    PVOID Object;
    HANDLE CreatorUniqueProcess;
    USHORT CreatorBackTraceIndex;
    USHORT Flags;
    LONG PointerCount;
    LONG HandleCount;
    ULONG PagedPoolCharge;
    ULONG NonPagedPoolCharge;
    HANDLE ExclusiveProcessId;
    PVOID SecurityDescriptor;
    UNICODE_STRING NameInfo;
} SYSTEM_OBJECT_INFORMATION, *PSYSTEM_OBJECT_INFORMATION;

typedef struct _SYSTEM_PAGEFILE_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG TotalSize;
    ULONG TotalInUse;
    ULONG PeakUsage;
    UNICODE_STRING PageFileName;
} SYSTEM_PAGEFILE_INFORMATION, *PSYSTEM_PAGEFILE_INFORMATION;

#define MM_WORKING_SET_MAX_HARD_ENABLE 0x1
#define MM_WORKING_SET_MAX_HARD_DISABLE 0x2
#define MM_WORKING_SET_MIN_HARD_ENABLE 0x4
#define MM_WORKING_SET_MIN_HARD_DISABLE 0x8

typedef struct _SYSTEM_FILECACHE_INFORMATION
{
    SIZE_T CurrentSize;
    SIZE_T PeakSize;
    ULONG PageFaultCount;
    SIZE_T MinimumWorkingSet;
    SIZE_T MaximumWorkingSet;
    SIZE_T CurrentSizeIncludingTransitionInPages;
    SIZE_T PeakSizeIncludingTransitionInPages;
    ULONG TransitionRePurposeCount;
    ULONG Flags;
} SYSTEM_FILECACHE_INFORMATION, *PSYSTEM_FILECACHE_INFORMATION;

typedef struct _SYSTEM_POOL_ENTRY
{
    BOOLEAN Allocated;
    BOOLEAN Spare0;
    USHORT AllocatorBackTraceIndex;
    ULONG Size;
    union
    {
        UCHAR Tag[4];
        ULONG TagUlong;
        PVOID ProcessChargedQuota;
    };
} SYSTEM_POOL_ENTRY, *PSYSTEM_POOL_ENTRY;

typedef struct _SYSTEM_POOL_INFORMATION
{
    SIZE_T TotalSize;
    PVOID FirstEntry;
    USHORT EntryOverhead;
    BOOLEAN PoolTagPresent;
    BOOLEAN Spare0;
    ULONG NumberOfEntries;
    SYSTEM_POOL_ENTRY Entries[1];
} SYSTEM_POOL_INFORMATION, *PSYSTEM_POOL_INFORMATION;

typedef struct _SYSTEM_POOLTAG
{
    union
    {
        UCHAR Tag[4];
        ULONG TagUlong;
    };
    ULONG PagedAllocs;
    ULONG PagedFrees;
    SIZE_T PagedUsed;
    ULONG NonPagedAllocs;
    ULONG NonPagedFrees;
    SIZE_T NonPagedUsed;
} SYSTEM_POOLTAG, *PSYSTEM_POOLTAG;

typedef struct _SYSTEM_POOLTAG_INFORMATION
{
    ULONG Count;
    SYSTEM_POOLTAG TagInfo[1];
} SYSTEM_POOLTAG_INFORMATION, *PSYSTEM_POOLTAG_INFORMATION;

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX
{
    PVOID Object;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR HandleValue;
    ULONG GrantedAccess;
    USHORT CreatorBackTraceIndex;
    USHORT ObjectTypeIndex;
    ULONG HandleAttributes;
    ULONG Reserved;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX, *PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX;

typedef struct _SYSTEM_HANDLE_INFORMATION_EX
{
    ULONG_PTR NumberOfHandles;
    ULONG_PTR Reserved;
    SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handles[1];
} SYSTEM_HANDLE_INFORMATION_EX, *PSYSTEM_HANDLE_INFORMATION_EX;

typedef struct _SYSTEM_BIGPOOL_ENTRY
{
    union
    {
        PVOID VirtualAddress;
        ULONG_PTR NonPaged : 1;
    };
    SIZE_T SizeInBytes;
    union
    {
        UCHAR Tag[4];
        ULONG TagUlong;
    };
} SYSTEM_BIGPOOL_ENTRY, *PSYSTEM_BIGPOOL_ENTRY;

typedef struct _SYSTEM_BIGPOOL_INFORMATION
{
    ULONG Count;
    SYSTEM_BIGPOOL_ENTRY AllocatedInfo[1];
} SYSTEM_BIGPOOL_INFORMATION, *PSYSTEM_BIGPOOL_INFORMATION;

typedef struct _SYSTEM_SESSION_POOLTAG_INFORMATION
{
    SIZE_T NextEntryOffset;
    ULONG SessionId;
    ULONG Count;
    SYSTEM_POOLTAG TagInfo[1];
} SYSTEM_SESSION_POOLTAG_INFORMATION, *PSYSTEM_SESSION_POOLTAG_INFORMATION;

typedef struct _SYSTEM_SESSION_MAPPED_VIEW_INFORMATION
{
    SIZE_T NextEntryOffset;
    ULONG SessionId;
    ULONG ViewFailures;
    SIZE_T NumberOfBytesAvailable;
    SIZE_T NumberOfBytesAvailableContiguous;
} SYSTEM_SESSION_MAPPED_VIEW_INFORMATION, *PSYSTEM_SESSION_MAPPED_VIEW_INFORMATION;

typedef struct _SYSTEM_CONTEXT_SWITCH_INFORMATION
{
    ULONG ContextSwitches;
    ULONG FindAny;
    ULONG FindLast;
    ULONG FindIdeal;
    ULONG IdleAny;
    ULONG IdleCurrent;
    ULONG IdleLast;
    ULONG IdleIdeal;
    ULONG PreemptAny;
    ULONG PreemptCurrent;
    ULONG PreemptLast;
    ULONG SwitchToIdle;
} SYSTEM_CONTEXT_SWITCH_INFORMATION, *PSYSTEM_CONTEXT_SWITCH_INFORMATION;

typedef struct _SYSTEM_INTERRUPT_INFORMATION
{
    ULONG ContextSwitches;
    ULONG DpcCount;
    ULONG DpcRate;
    ULONG TimeIncrement;
    ULONG DpcBypassCount;
    ULONG ApcBypassCount;
} SYSTEM_INTERRUPT_INFORMATION, *PSYSTEM_INTERRUPT_INFORMATION;

typedef struct _SYSTEM_DPC_BEHAVIOR_INFORMATION
{
    ULONG Spare;
    ULONG DpcQueueDepth;
    ULONG MinimumDpcRate;
    ULONG AdjustDpcThreshold;
    ULONG IdealDpcRate;
} SYSTEM_DPC_BEHAVIOR_INFORMATION, *PSYSTEM_DPC_BEHAVIOR_INFORMATION;

typedef struct _SYSTEM_LOOKASIDE_INFORMATION
{
    USHORT CurrentDepth;
    USHORT MaximumDepth;
    ULONG TotalAllocates;
    ULONG AllocateMisses;
    ULONG TotalFrees;
    ULONG FreeMisses;
    ULONG Type;
    ULONG Tag;
    ULONG Size;
} SYSTEM_LOOKASIDE_INFORMATION, *PSYSTEM_LOOKASIDE_INFORMATION;

// Common structure used by a few info classes

// begin_rev

typedef enum _SYSTEM_EXTENDED_INFORMATION_TYPE
{
    ExtendedPrefetcherInformation = 23,
    ExtendedSuperfetchInformation = 45
} SYSTEM_EXTENDED_INFORMATION_TYPE;

#define SYSTEM_EXTENDED_INFORMATION_MAGIC ('kuhC') 

typedef struct _SYSTEM_EXTENDED_INFORMATION
{
    SYSTEM_EXTENDED_INFORMATION_TYPE Type;
    ULONG Magic;
    ULONG InformationClass;
    PVOID Buffer;
    ULONG BufferSize;
} SYSTEM_EXTENDED_INFORMATION, *PSYSTEM_EXTENDED_INFORMATION;

// end_rev

// begin_rev

typedef enum _PREFETCHER_INFORMATION_CLASS
{
    PrefetcherCompletedTraceInformation = 1,
    PrefetcherUnknownInformation = 2, // ???
    PrefetcherBootLoaderTraceInformation = 4
} PREFETCHER_INFORMATION_CLASS;

// end_rev

// begin_rev

#define SUPERFETCH_INFORMATION_REVISION 1

typedef enum _SUPERFETCH_INFORMATION_CLASS
{
    SuperfetchCompletedTraceInformation = 1,
    SuperfetchUnknownInformation2 = 2, // size 24
    SuperfetchPfnPrioRequestInformation = 6,
    SuperfetchPrivSourceInformation = 8,
    SuperfetchUnknownInformation9 = 9, // size 4
    SuperfetchScenarioInformation = 12,
    SuperfetchMemoryListInformation = 16,
    SuperfetchMemoryRangesInformation = 17,
    SuperfetchUnknownInformation20 = 20 // size 8
} SUPERFETCH_INFORMATION_CLASS;

typedef struct _PHYSICAL_MEMORY_RUN
{
    ULONG_PTR BasePage; // multiply by PAGE_SIZE to get base address
    ULONG_PTR PageCount; // multiply by PAGE_SIZE to get number of bytes
} PHYSICAL_MEMORY_RUN, *PPHYSICAL_MEMORY_RUN;

typedef struct _SUPERFETCH_MEMORY_RANGES_INFORMATION
{
    ULONG Revision;
    ULONG NumberOfRanges;
    PHYSICAL_MEMORY_RUN Ranges[1];
} SUPERFETCH_MEMORY_RANGES_INFORMATION, *PSUPERFETCH_MEMORY_RANGES_INFORMATION;

// end_rev

// rev
typedef struct _SYSTEM_PROCESS_IMAGE_NAME_INFORMATION
{
    HANDLE ProcessId;
    UNICODE_STRING ImageName;
} SYSTEM_PROCESS_IMAGE_NAME_INFORMATION, *PSYSTEM_PROCESS_IMAGE_NAME_INFORMATION;

// msdn
typedef struct _SYSTEM_VHD_BOOT_INFORMATION
{
    BOOLEAN OsDiskIsVhd;
    ULONG OsVhdFilePathOffset;
    WCHAR OsVhdParentVolume[ANYSIZE_ARRAY];
} SYSTEM_VHD_BOOT_INFORMATION, *PSYSTEM_VHD_BOOT_INFORMATION;

// http://www.msuiche.net/2009/04/01/low-priority-io-count-information-systemlowpriorityinformation/
typedef struct _SYSTEM_LOW_PRIORITY_IO_INFORMATION
{
    ULONG IoLowPriorityReadOperationCount;
    ULONG IoLowPriorityWriteOperationCount;
    ULONG IoKernelIssuedIoBoostedCount;
    ULONG IoPagingReadLowPriorityCount;
    ULONG IoPagingReadLowPriorityBumpedCount;
    ULONG IoPagingWriteLowPriorityCount;
    ULONG IoPagingWriteLowPriorityBumpedCount;
    ULONG IoBoostedThreadedIrpCount;
    ULONG IoBoostedPagingIrpCount;
    ULONG IoBlanketBoostCount;
} SYSTEM_LOW_PRIORITY_IO_INFORMATION, *PSYSTEM_LOW_PRIORITY_IO_INFORMATION;

// symbols
typedef enum _TPM_BOOT_ENTROPY_RESULT_CODE
{
    TpmBootEntropyStructureUninitialized,
    TpmBootEntropyDisabledByPolicy,
    TpmBootEntropyNoTpmFound,
    TpmBootEntropyTpmError,
    TpmBootEntropySuccess
} TPM_BOOT_ENTROPY_RESULT_CODE;

// Contents of KeLoaderBlock->Extension->TpmBootEntropyResult (TPM_BOOT_ENTROPY_LDR_RESULT).
// EntropyData is truncated to 40 bytes.

// symbols
typedef struct _SYSTEM_TPM_BOOT_ENTROPY_INFORMATION
{
    ULONGLONG Policy;
    TPM_BOOT_ENTROPY_RESULT_CODE ResultCode;
    NTSTATUS ResultStatus;
    ULONGLONG Time;
    ULONG EntropyLength;
    UCHAR EntropyData[40];
} SYSTEM_TPM_BOOT_ENTROPY_INFORMATION, *PSYSTEM_TPM_BOOT_ENTROPY_INFORMATION;

// rev
typedef struct _SYSTEM_MEMORY_COMMITMENT_INFORMATION
{
    ULONG AvailablePages;
    ULONG CommittedPages;
    ULONG CommitLimit;
    ULONG PeakCommitment;
} SYSTEM_MEMORY_COMMITMENT_INFORMATION, *PSYSTEM_MEMORY_COMMITMENT_INFORMATION;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemInformation(
    __in SYSTEM_INFORMATION_CLASS SystemInformationClass,
    __out_bcount_opt(SystemInformationLength) PVOID SystemInformation,
    __in ULONG SystemInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemInformationEx(
    __in SYSTEM_INFORMATION_CLASS SystemInformationClass,
    __in_bcount(QueryInfomationLength) PVOID QueryInformation,
    __in ULONG QueryInfomationLength,
    __out_bcount_opt(SystemInformationLength) PVOID SystemInformation,
    __in ULONG SystemInformationLength,
    __out_opt PULONG ReturnLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetSystemInformation(
    __in SYSTEM_INFORMATION_CLASS SystemInformationClass,
    __in_bcount_opt(SystemInformationLength) PVOID SystemInformation,
    __in ULONG SystemInformationLength
    );

// Kernel-user shared data

typedef enum _ALTERNATIVE_ARCHITECTURE_TYPE
{
    StandardDesign,
    NEC98x86,
    EndAlternatives
} ALTERNATIVE_ARCHITECTURE_TYPE;

#define PROCESSOR_FEATURE_MAX 64

#define MAX_WOW64_SHARED_ENTRIES 16

#define NX_SUPPORT_POLICY_ALWAYSOFF 0
#define NX_SUPPORT_POLICY_ALWAYSON 1
#define NX_SUPPORT_POLICY_OPTIN 2
#define NX_SUPPORT_POLICY_OPTOUT 3

typedef struct _KUSER_SHARED_DATA
{
    ULONG TickCountLowDeprecated;
    ULONG TickCountMultiplier;

    volatile KSYSTEM_TIME InterruptTime;
    volatile KSYSTEM_TIME SystemTime;
    volatile KSYSTEM_TIME TimeZoneBias;

    USHORT ImageNumberLow;
    USHORT ImageNumberHigh;

    WCHAR NtSystemRoot[260];

    ULONG MaxStackTraceDepth;

    ULONG CryptoExponent;

    ULONG TimeZoneId;
    ULONG LargePageMinimum;
    ULONG Reserved2[7];

    ULONG NtProductType;
    BOOLEAN ProductTypeIsValid;

    ULONG NtMajorVersion;
    ULONG NtMinorVersion;

    BOOLEAN ProcessorFeatures[PROCESSOR_FEATURE_MAX];

    ULONG Reserved1;
    ULONG Reserved3;

    volatile ULONG TimeSlip;

    ALTERNATIVE_ARCHITECTURE_TYPE AlternativeArchitecture;

    LARGE_INTEGER SystemExpirationDate;

    ULONG SuiteMask;

    BOOLEAN KdDebuggerEnabled;

    UCHAR NXSupportPolicy;

    volatile ULONG ActiveConsoleId;

    volatile ULONG DismountCount;

    ULONG ComPlusPackage;

    ULONG LastSystemRITEventTickCount;

    ULONG NumberOfPhysicalPages;

    BOOLEAN SafeBootMode;
    union
    {
        UCHAR TscQpcData;
        struct
        {
            UCHAR TscQpcEnabled : 1;
            UCHAR TscQpcSpareFlag : 1;
            UCHAR TscQpcShift : 6;
        };
    };
    UCHAR TscQpcPad[2];

    union
    {
        ULONG TraceLogging;
        ULONG SharedDataFlags;
        struct
        {
            ULONG DbgErrorPortPresent : 1;
            ULONG DbgElevationEnabled : 1;
            ULONG DbgVirtEnabled : 1;
            ULONG DbgInstallerDetectEnabled : 1;
            ULONG DbgSystemDllRelocated : 1;
            ULONG DbgDynProcessorEnabled : 1;
            ULONG DbgSEHValidationEnabled : 1;
            ULONG SpareBits : 25;
        };
    };
    ULONG DataFlagsPad[1];

    ULONGLONG TestRetInstruction;
    ULONG SystemCall;
    ULONG SystemCallReturn;
    ULONGLONG SystemCallPad[3];

    union
    {
        volatile KSYSTEM_TIME TickCount;
        volatile ULONG64 TickCountQuad;
        struct
        {
            ULONG ReservedTickCountOverlay[3];
            ULONG TickCountPad[1];
        };
    };

    ULONG Cookie;

    // Entries below all invalid below Windows Vista

    ULONG CookiePad[1];

    LONGLONG ConsoleSessionForegroundProcessId;

    ULONG Wow64SharedInformation[MAX_WOW64_SHARED_ENTRIES];

    USHORT UserModeGlobalLogger[16];
    ULONG ImageFileExecutionOptions;

    ULONG LangGenerationCount;

    union
    {
        ULONGLONG AffinityPad; // only valid on Windows Vista
        ULONG_PTR ActiveProcessorAffinity; // only valid on Windows Vista
        ULONGLONG Reserved5;
    };
    volatile ULONG64 InterruptTimeBias;
    volatile ULONG64 TscQpcBias;

    volatile ULONG ActiveProcessorCount;
    volatile USHORT ActiveGroupCount;
    USHORT Reserved4;

    volatile ULONG AitSamplingValue;
    volatile ULONG AppCompatFlag;

    ULONGLONG SystemDllNativeRelocation;
    ULONG SystemDllWowRelocation;

    ULONG XStatePad[1];
    XSTATE_CONFIGURATION XState;
} KUSER_SHARED_DATA, *PKUSER_SHARED_DATA;

C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TickCountMultiplier) == 0x4);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, InterruptTime) == 0x8);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, SystemTime) == 0x14);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TimeZoneBias) == 0x20);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, ImageNumberLow) == 0x2c);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, ImageNumberHigh) == 0x2e);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, NtSystemRoot) == 0x30);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, MaxStackTraceDepth) == 0x238);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, CryptoExponent) == 0x23c);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TimeZoneId) == 0x240);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, LargePageMinimum) == 0x244);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, Reserved2) == 0x248);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, NtProductType) == 0x264);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, ProductTypeIsValid) == 0x268);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, NtMajorVersion) == 0x26c);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, NtMinorVersion) == 0x270);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, ProcessorFeatures) == 0x274);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, Reserved1) == 0x2b4);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, Reserved3) == 0x2b8);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TimeSlip) == 0x2bc);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, AlternativeArchitecture) == 0x2c0);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, SystemExpirationDate) == 0x2c8);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, SuiteMask) == 0x2d0);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, KdDebuggerEnabled) == 0x2d4);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, NXSupportPolicy) == 0x2d5);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, ActiveConsoleId) == 0x2d8);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, DismountCount) == 0x2dC);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, ComPlusPackage) == 0x2e0);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, LastSystemRITEventTickCount) == 0x2e4);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, NumberOfPhysicalPages) == 0x2e8);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, SafeBootMode) == 0x2ec);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TraceLogging) == 0x2f0);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TestRetInstruction) == 0x2f8);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, SystemCall) == 0x300);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, SystemCallReturn) == 0x304);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, SystemCallPad) == 0x308);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TickCount) == 0x320);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TickCountQuad) == 0x320);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, Cookie) == 0x330);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, ConsoleSessionForegroundProcessId) == 0x338);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, Wow64SharedInformation) == 0x340);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, UserModeGlobalLogger) == 0x380);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, ImageFileExecutionOptions) == 0x3a0);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, LangGenerationCount) == 0x3a4);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, InterruptTimeBias) == 0x3b0);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, UserModeGlobalLogger) == 0x380);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, ImageFileExecutionOptions) == 0x3a0);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, LangGenerationCount) == 0x3a4);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, Reserved5) == 0x3a8);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, InterruptTimeBias) == 0x3b0);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, TscQpcBias) == 0x3b8);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, ActiveProcessorCount) == 0x3c0);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, ActiveGroupCount) == 0x3c4);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, Reserved4) == 0x3c6);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, AitSamplingValue) == 0x3c8);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, AppCompatFlag) == 0x3cc);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, SystemDllNativeRelocation) == 0x3d0);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, SystemDllWowRelocation) == 0x3d8);
C_ASSERT(FIELD_OFFSET(KUSER_SHARED_DATA, XState) == 0x3e0);

#ifdef _M_IX86
#define USER_SHARED_DATA ((KUSER_SHARED_DATA * const)0x7ffe0000)
#endif

#ifdef _M_X64
#define USER_SHARED_DATA ((KUSER_SHARED_DATA * const)0x7ffe0000)
#endif

FORCEINLINE ULONGLONG NtGetTickCount64()
{
    ULARGE_INTEGER tickCount;

#ifdef _M_X64

    tickCount.QuadPart = USER_SHARED_DATA->TickCountQuad;

#else

    do
    {
        tickCount.HighPart = (ULONG)USER_SHARED_DATA->TickCount.High1Time;
        tickCount.LowPart = USER_SHARED_DATA->TickCount.LowPart;
    } while (tickCount.HighPart != (ULONG)USER_SHARED_DATA->TickCount.High2Time);

#endif

    return (UInt32x32To64(tickCount.LowPart, USER_SHARED_DATA->TickCountMultiplier) >> 24) +
        (UInt32x32To64(tickCount.HighPart, USER_SHARED_DATA->TickCountMultiplier) << 8);
}

// Locale

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDefaultLocale(
    __in BOOLEAN UserProfile,
    __out PLCID DefaultLocaleId
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetDefaultLocale(
    __in BOOLEAN UserProfile,
    __in LCID DefaultLocaleId
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInstallUILanguage(
    __out LANGID *InstallUILanguageId
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDefaultUILanguage(
    __out LANGID *DefaultUILanguageId
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetDefaultUILanguage(
    __in LANGID DefaultUILanguageId
    );

// Global atoms

NTSYSCALLAPI
NTSTATUS
NTAPI
NtAddAtom(
    __in_bcount_opt(Length) PWSTR AtomName,
    __in ULONG Length,
    __out_opt PRTL_ATOM Atom
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtFindAtom(
    __in_bcount_opt(Length) PWSTR AtomName,
    __in ULONG Length,
    __out_opt PRTL_ATOM Atom
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDeleteAtom(
    __in RTL_ATOM Atom
    );

typedef enum _ATOM_INFORMATION_CLASS
{
    AtomBasicInformation,
    AtomTableInformation
} ATOM_INFORMATION_CLASS;

typedef struct _ATOM_BASIC_INFORMATION
{
    USHORT UsageCount;
    USHORT Flags;
    USHORT NameLength;
    WCHAR Name[1];
} ATOM_BASIC_INFORMATION, *PATOM_BASIC_INFORMATION;

typedef struct _ATOM_TABLE_INFORMATION
{
    ULONG NumberOfAtoms;
    RTL_ATOM Atoms[1];
} ATOM_TABLE_INFORMATION, *PATOM_TABLE_INFORMATION;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryInformationAtom(
    __in RTL_ATOM Atom,
    __in ATOM_INFORMATION_CLASS AtomInformationClass,
    __out_bcount(AtomInformationLength) PVOID AtomInformation,
    __in ULONG AtomInformationLength,
    __out_opt PULONG ReturnLength
    );

// Global flags

#define FLG_STOP_ON_EXCEPTION 0x00000001 // uk
#define FLG_SHOW_LDR_SNAPS 0x00000002 // uk
#define FLG_DEBUG_INITIAL_COMMAND 0x00000004 // k
#define FLG_STOP_ON_HUNG_GUI 0x00000008 // k

#define FLG_HEAP_ENABLE_TAIL_CHECK 0x00000010 // u
#define FLG_HEAP_ENABLE_FREE_CHECK 0x00000020 // u
#define FLG_HEAP_VALIDATE_PARAMETERS 0x00000040 // u
#define FLG_HEAP_VALIDATE_ALL 0x00000080 // u

#define FLG_APPLICATION_VERIFIER 0x00000100 // u
#define FLG_POOL_ENABLE_TAGGING 0x00000400 // k
#define FLG_HEAP_ENABLE_TAGGING 0x00000800 // u

#define FLG_USER_STACK_TRACE_DB 0x00001000 // u,32
#define FLG_KERNEL_STACK_TRACE_DB 0x00002000 // k,32
#define FLG_MAINTAIN_OBJECT_TYPELIST 0x00004000 // k
#define FLG_HEAP_ENABLE_TAG_BY_DLL 0x00008000 // u

#define FLG_DISABLE_STACK_EXTENSION 0x00010000 // u
#define FLG_ENABLE_CSRDEBUG 0x00020000 // k
#define FLG_ENABLE_KDEBUG_SYMBOL_LOAD 0x00040000 // k
#define FLG_DISABLE_PAGE_KERNEL_STACKS 0x00080000 // k

#define FLG_ENABLE_SYSTEM_CRIT_BREAKS 0x00100000 // u
#define FLG_HEAP_DISABLE_COALESCING 0x00200000 // u
#define FLG_ENABLE_CLOSE_EXCEPTIONS 0x00400000 // k
#define FLG_ENABLE_EXCEPTION_LOGGING 0x00800000 // k

#define FLG_ENABLE_HANDLE_TYPE_TAGGING 0x01000000 // k
#define FLG_HEAP_PAGE_ALLOCS 0x02000000 // u
#define FLG_DEBUG_INITIAL_COMMAND_EX 0x04000000 // k
#define FLG_DISABLE_DBGPRINT 0x08000000 // k

#define FLG_CRITSEC_EVENT_CREATION 0x10000000 // u
#define FLG_LDR_TOP_DOWN 0x20000000 // u,64
#define FLG_ENABLE_HANDLE_EXCEPTIONS 0x40000000 // k
#define FLG_DISABLE_PROTDLLS 0x80000000 // u

#define FLG_VALID_BITS 0xfffffdff

#define FLG_USERMODE_VALID_BITS (FLG_STOP_ON_EXCEPTION | \
    FLG_SHOW_LDR_SNAPS | \
    FLG_HEAP_ENABLE_TAIL_CHECK | \
    FLG_HEAP_ENABLE_FREE_CHECK | \
    FLG_HEAP_VALIDATE_PARAMETERS | \
    FLG_HEAP_VALIDATE_ALL | \
    FLG_APPLICATION_VERIFIER | \
    FLG_HEAP_ENABLE_TAGGING | \
    FLG_USER_STACK_TRACE_DB | \
    FLG_HEAP_ENABLE_TAG_BY_DLL | \
    FLG_DISABLE_STACK_EXTENSION | \
    FLG_ENABLE_SYSTEM_CRIT_BREAKS | \
    FLG_HEAP_DISABLE_COALESCING | \
    FLG_DISABLE_PROTDLLS | \
    FLG_HEAP_PAGE_ALLOCS | \
    FLG_CRITSEC_EVENT_CREATION | \
    FLG_LDR_TOP_DOWN)

#define FLG_BOOTONLY_VALID_BITS (FLG_KERNEL_STACK_TRACE_DB | \
    FLG_MAINTAIN_OBJECT_TYPELIST | \
    FLG_ENABLE_CSRDEBUG | \
    FLG_DEBUG_INITIAL_COMMAND | \
    FLG_DEBUG_INITIAL_COMMAND_EX | \
    FLG_DISABLE_PAGE_KERNEL_STACKS)

#define FLG_KERNELMODE_VALID_BITS (FLG_STOP_ON_EXCEPTION | \
    FLG_SHOW_LDR_SNAPS | \
    FLG_STOP_ON_HUNG_GUI | \
    FLG_POOL_ENABLE_TAGGING | \
    FLG_ENABLE_KDEBUG_SYMBOL_LOAD | \
    FLG_ENABLE_CLOSE_EXCEPTIONS | \
    FLG_ENABLE_EXCEPTION_LOGGING | \
    FLG_ENABLE_HANDLE_TYPE_TAGGING | \
    FLG_DISABLE_DBGPRINT | \
    FLG_ENABLE_HANDLE_EXCEPTIONS)

// Misc.

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetDefaultHardErrorPort(
    __in HANDLE DefaultHardErrorPort
    );

typedef enum _SHUTDOWN_ACTION
{
    ShutdownNoReboot,
    ShutdownReboot,
    ShutdownPowerOff
} SHUTDOWN_ACTION;

NTSYSCALLAPI
NTSTATUS
NTAPI
NtShutdownSystem(
    __in SHUTDOWN_ACTION Action
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtDisplayString(
    __in PUNICODE_STRING String
    );

#endif
