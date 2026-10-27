/*
 * Kernel executive support library
 *
 * This file is part of System Informer.
 */

#ifndef _NTKEAPI_H
#define _NTKEAPI_H

#if (PHNT_MODE != PHNT_MODE_KERNEL)
#define LOW_PRIORITY 0 // Lowest thread priority level
#define IDLE_BASE_PRIORITY 1 // Idle thread priority level
#define BELOW_NORMAL_BASE_PRIORITY 6 // Below-normal thread base priority level
#define NORMAL_BASE_PRIORITY 8 // Normal thread base priority level
#define ABOVE_NORMAL_BASE_PRIORITY 10 // Above-normal thread base priority level
#define HIGH_BASE_PRIORITY 13 // High thread base priority level
#define TIME_CRITICAL_PRIORITY 15 // Highest variable thread priority level
#define LOW_REALTIME_PRIORITY 16 // Lowest realtime priority level
#define HIGH_PRIORITY 31 // Highest thread priority level
#define MAXIMUM_PRIORITY 32 // Number of thread priority levels
#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

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
    GateWaitObsolete,
    WaitingForProcessInSwap,
    MaximumThreadState
} KTHREAD_STATE, *PKTHREAD_STATE;

// private
typedef enum _KHETERO_CPU_POLICY
{
    KHeteroCpuPolicyAll = 0,
    KHeteroCpuPolicyLarge = 1,
    KHeteroCpuPolicyLargeOrIdle = 2,
    KHeteroCpuPolicySmall = 3,
    KHeteroCpuPolicySmallOrIdle = 4,
    KHeteroCpuPolicyDynamic = 5,
    KHeteroCpuPolicyStaticMax = 5, // valid
    KHeteroCpuPolicyBiasedSmall = 6,
    KHeteroCpuPolicyBiasedLarge = 7,
    KHeteroCpuPolicyDefault = 8,
    KHeteroCpuPolicyMax = 9
} KHETERO_CPU_POLICY, *PKHETERO_CPU_POLICY;

#if (PHNT_MODE != PHNT_MODE_KERNEL)
/**
 * KWAIT_REASON identifies the reasons for context switches or the current waiting state.
 */
typedef enum _KWAIT_REASON
{
    Executive,               // Waiting for an executive event.
    FreePage,                // Waiting for a free page.
    PageIn,                  // Waiting for a page to be read in.
    PoolAllocation,          // Waiting for a pool allocation.
    DelayExecution,          // Waiting due to a delay execution.           // NtDelayExecution
    Suspended,               // Waiting because the thread is suspended.    // NtSuspendThread
    UserRequest,             // Waiting due to a user request.              // NtWaitForSingleObject
    WrExecutive,             // Waiting for an executive event.
    WrFreePage,              // Waiting for a free page.
    WrPageIn,                // Waiting for a page to be read in.
    WrPoolAllocation,        // Waiting for a pool allocation.              // 10
    WrDelayExecution,        // Waiting due to a delay execution.
    WrSuspended,             // Waiting because the thread is suspended.
    WrUserRequest,           // Waiting due to a user request.
    WrEventPair,             // Waiting for an event pair.                  // NtCreateEventPair
    WrQueue,                 // Waiting for a queue.                        // NtRemoveIoCompletion
    WrLpcReceive,            // Waiting for an LPC receive.                 // NtReplyWaitReceivePort
    WrLpcReply,              // Waiting for an LPC reply.                   // NtRequestWaitReplyPort
    WrVirtualMemory,         // Waiting for virtual memory.
    WrPageOut,               // Waiting for a page to be written out.       // NtFlushVirtualMemory
    WrRendezvous,            // Waiting for a rendezvous.                   // 20
    WrKeyedEvent,            // Waiting for a keyed event.                  // NtCreateKeyedEvent
    WrTerminated,            // Waiting for thread termination.
    WrProcessInSwap,         // Waiting for a process to be swapped in.
    WrCpuRateControl,        // Waiting for CPU rate control.
    WrCalloutStack,          // Waiting for a callout stack.
    WrKernel,                // Waiting for a kernel event.
    WrResource,              // Waiting for a resource.
    WrPushLock,              // Waiting for a push lock.
    WrMutex,                 // Waiting for a mutex.
    WrQuantumEnd,            // Waiting for the end of a quantum.           // 30
    WrDispatchInt,           // Waiting for a dispatch interrupt.
    WrPreempted,             // Waiting because the thread was preempted.
    WrYieldExecution,        // Waiting to yield execution.
    WrFastMutex,             // Waiting for a fast mutex.
    WrGuardedMutex,          // Waiting for a guarded mutex.
    WrRundown,               // Waiting for a rundown.
    WrAlertByThreadId,       // Waiting for an alert by thread ID.
    WrDeferredPreempt,       // Waiting for a deferred preemption.
    WrPhysicalFault,         // Waiting for a physical fault.
    WrIoRing,                // Waiting for an I/O ring.                    // 40
    WrMdlCache,              // Waiting for an MDL cache.
    WrRcu,                   // Waiting for read-copy-update (RCU) synchronization.
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

#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

#if (PHNT_MODE != PHNT_MODE_KERNEL)

/**
 * The NtCallbackReturn routine returns from a user-mode callback to the kernel.
 *
 * \param OutputBuffer An optional pointer to a buffer containing the output data to be returned to the kernel.
 * \param OutputLength The length, in bytes, of the output buffer.
 * \param Status The status code to be returned to the kernel.
 * \return NTSTATUS Successful or errant status.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtCallbackReturn(
    _In_reads_bytes_opt_(OutputLength) PVOID OutputBuffer,
    _In_ ULONG OutputLength,
    _In_ NTSTATUS Status
    );

/**
 * The NtQueryDebugFilterState routine queries the state of a specific debug filter component and level.
 *
 * \param ComponentId The identifier of the component whose debug filter state is being queried.
 * \param Level The debug filter level being queried.
 * \return NTSTATUS Successful or errant status.
 * \remarks Returns the filter-state predicate as status value 0 or 1, not solely STATUS_SUCCESS. Levels 0 through 31 are converted to bit positions; higher
 * values are treated as masks. Querying filter state does not change debugger output policy.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDebugFilterState(
    _In_ ULONG ComponentId,
    _In_ ULONG Level
    );

/**
 * The NtSetDebugFilterState routine sets the state of a specific debug filter component and level.
 *
 * \param ComponentId The identifier of the component whose debug filter state is being set.
 * \param Level The debug filter level being set.
 * \param State TRUE to enable the debug filter, FALSE to disable it.
 * \return NTSTATUS Successful or errant status.
 * \remarks Mutates debugger filtering; the native user-mode entry checks SeDebugPrivilege
 * and returns STATUS_ACCESS_DENIED when the privilege check fails. Use NtQueryDebugFilterState for inspection
 * rather than toggling a filter merely to test availability.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetDebugFilterState(
    _In_ ULONG ComponentId,
    _In_ ULONG Level,
    _In_ BOOLEAN State
    );

/**
 * Offers the current thread's processor execution opportunity to an eligible
 * ready thread. This is a scheduler hint, not a delay of a specified duration,
 * an alertable object wait, or a synchronization operation.
 *
 * \return STATUS_SUCCESS (0) if the yield path succeeds, or the informational
 * STATUS_NO_YIELD_PERFORMED (0x40000024) if no yield is performed. Neither status
 * establishes that a particular other thread ran or completed work.
 * \remarks The x64 entry calls KeYieldExecution with flags 0
 * and returns its status. The scheduler inspects both the current processor's
 * ready summary and its shared ready queue; a no-yield result is not proof that
 * there are no runnable threads anywhere in the system. Use an actual waitable
 * object or another synchronization primitive when another thread's progress matters.
 */
_Kernel_entry_
NTSYSCALLAPI
NTSTATUS
NTAPI
NtYieldExecution(
    VOID
    );

#endif // (PHNT_MODE != PHNT_MODE_KERNEL)

#endif // _NTKEAPI_H
