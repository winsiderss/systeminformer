#ifndef _NTKEAPI_H
#define _NTKEAPI_H

#if (PHNT_MODE != PHNT_MODE_KERNEL)
#define LOW_PRIORITY 0 // Lowest thread priority level
#define LOW_REALTIME_PRIORITY 16 // Lowest realtime priority level
#define HIGH_PRIORITY 31 // Highest thread priority level
#define MAXIMUM_PRIORITY 32 // Number of thread priority levels
#endif

typedef enum _KTHREAD_STATE
{
    Initialized,
    Ready,
    Running,
    Standby,
    Terminated,
    Waiting,
    Transition,
    DeferredReady,
    GateWait,
    MaximumThreadState
} KTHREAD_STATE, *PKTHREAD_STATE;

#if (PHNT_MODE != PHNT_MODE_KERNEL)

typedef enum _KWAIT_REASON
{
    Executive,
    FreePage,
    PageIn,
    PoolAllocation,
    DelayExecution,
    Suspended,
    UserRequest,
    WrExecutive,
    WrFreePage,
    WrPageIn,
    WrPoolAllocation,
    WrDelayExecution,
    WrSuspended,
    WrUserRequest,
    WrEventPair,
    WrQueue,
    WrLpcReceive,
    WrLpcReply,
    WrVirtualMemory,
    WrPageOut,
    WrRendezvous,
    WrKeyedEvent,
    WrTerminated,
    WrProcessInSwap,
    WrCpuRateControl,
    WrCalloutStack,
    WrKernel,
    WrResource,
    WrPushLock,
    WrMutex,
    WrQuantumEnd,
    WrDispatchInt,
    WrPreempted,
    WrYieldExecution,
    WrFastMutex,
    WrGuardedMutex,
    WrRundown,
    MaximumWaitReason
} KWAIT_REASON, *PKWAIT_REASON;

typedef enum _KPROFILE_SOURCE
{
    ProfileTime,
    ProfileAlignmentFixup,
    ProfileTotalIssues,
    ProfilePipelineDry,
    ProfileLoadInstructions,
    ProfilePipelineFrozen,
    ProfileBranchInstructions,
    ProfileTotalNonissues,
    ProfileDcacheMisses,
    ProfileIcacheMisses,
    ProfileCacheMisses,
    ProfileBranchMispredictions,
    ProfileStoreInstructions,
    ProfileFpInstructions,
    ProfileIntegerInstructions,
    Profile2Issue,
    Profile3Issue,
    Profile4Issue,
    ProfileSpecialInstructions,
    ProfileTotalCycles,
    ProfileIcacheIssues,
    ProfileDcacheAccesses,
    ProfileMemoryBarrierCycles,
    ProfileLoadLinkedIssues,
    ProfileMaximum
} KPROFILE_SOURCE;

#endif

#if (PHNT_MODE != PHNT_MODE_KERNEL)

NTSYSCALLAPI
NTSTATUS
NTAPI
NtCallbackReturn(
    __in_bcount_opt(OutputLength) PVOID OutputBuffer,
    __in ULONG OutputLength,
    __in NTSTATUS Status
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDebugFilterState(
    __in ULONG ComponentId,
    __in ULONG Level
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetDebugFilterState(
    __in ULONG ComponentId,
    __in ULONG Level,
    __in BOOLEAN State
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtYieldExecution(
    VOID
    );

#if (PHNT_VERSION >= PHNT_VISTA)
// winnt:FlushProcessWriteBuffers
NTSYSCALLAPI
VOID
NTAPI
NtFlushProcessWriteBuffers(
    VOID
    );
#endif

#endif

#endif
