/*
 * Process Hacker -
 *   Kernel executive support library
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

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
    GateWaitObsolete,
    WaitingForProcessInSwap,
    MaximumThreadState
} KTHREAD_STATE, *PKTHREAD_STATE;

// private
typedef enum _KHETERO_CPU_POLICY
{
    KHeteroCpuPolicyAll,
    KHeteroCpuPolicyLarge,
    KHeteroCpuPolicyLargeOrIdle,
    KHeteroCpuPolicySmall,
    KHeteroCpuPolicySmallOrIdle,
    KHeteroCpuPolicyDynamic,
    KHeteroCpuPolicyStaticMax,
    KHeteroCpuPolicyBiasedSmall,
    KHeteroCpuPolicyBiasedLarge,
    KHeteroCpuPolicyDefault,
    KHeteroCpuPolicyMax
} KHETERO_CPU_POLICY, *PKHETERO_CPU_POLICY;

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
    WrAlertByThreadId,
    WrDeferredPreempt,
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
    _In_reads_bytes_opt_(OutputLength) PVOID OutputBuffer,
    _In_ ULONG OutputLength,
    _In_ NTSTATUS Status
    );

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSCALLAPI
VOID
NTAPI
NtFlushProcessWriteBuffers(
    VOID
    );
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtQueryDebugFilterState(
    _In_ ULONG ComponentId,
    _In_ ULONG Level
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtSetDebugFilterState(
    _In_ ULONG ComponentId,
    _In_ ULONG Level,
    _In_ BOOLEAN State
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtYieldExecution(
    VOID
    );

#endif

#endif
