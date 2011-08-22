#ifndef _PH_QUEUEDLOCK_H
#define _PH_QUEUEDLOCK_H

#ifdef __cplusplus
extern "C" {
#endif

#define PH_QUEUED_LOCK_OWNED ((ULONG_PTR)0x1)
#define PH_QUEUED_LOCK_OWNED_SHIFT 0
#define PH_QUEUED_LOCK_WAITERS ((ULONG_PTR)0x2)

// Valid only if Waiters = 0
#define PH_QUEUED_LOCK_SHARED_INC ((ULONG_PTR)0x4)
#define PH_QUEUED_LOCK_SHARED_SHIFT 2

// Valid only if Waiters = 1
#define PH_QUEUED_LOCK_TRAVERSING ((ULONG_PTR)0x4)
#define PH_QUEUED_LOCK_MULTIPLE_SHARED ((ULONG_PTR)0x8)

#define PH_QUEUED_LOCK_FLAGS ((ULONG_PTR)0xf)

#define PhGetQueuedLockSharedOwners(Value) \
    ((ULONG_PTR)(Value) >> PH_QUEUED_LOCK_SHARED_SHIFT)
#define PhGetQueuedLockWaitBlock(Value) \
    ((PPH_QUEUED_WAIT_BLOCK)((ULONG_PTR)(Value) & ~PH_QUEUED_LOCK_FLAGS))

typedef struct _PH_QUEUED_LOCK
{
    ULONG_PTR Value;
} PH_QUEUED_LOCK, *PPH_QUEUED_LOCK;

#define PH_QUEUED_LOCK_INIT { 0 }

#define PH_QUEUED_WAITER_EXCLUSIVE 0x1
#define PH_QUEUED_WAITER_SPINNING 0x2
#define PH_QUEUED_WAITER_SPINNING_SHIFT 1

typedef struct DECLSPEC_ALIGN(16) _PH_QUEUED_WAIT_BLOCK
{
    /** A pointer to the next wait block, i.e. the 
     * wait block pushed onto the list before this 
     * one.
     */
    struct _PH_QUEUED_WAIT_BLOCK *Next;
    /** A pointer to the previous wait block, i.e. the 
     * wait block pushed onto the list after this 
     * one.
     */
    struct _PH_QUEUED_WAIT_BLOCK *Previous;
    /** A pointer to the last wait block, i.e. the 
     * first waiter pushed onto the list.
     */
    struct _PH_QUEUED_WAIT_BLOCK *Last;

    ULONG SharedOwners;
    ULONG Flags;
} PH_QUEUED_WAIT_BLOCK, *PPH_QUEUED_WAIT_BLOCK;

BOOLEAN PhQueuedLockInitialization(
    VOID
    );

FORCEINLINE VOID PhInitializeQueuedLock(
    __out PPH_QUEUED_LOCK QueuedLock
    )
{
    QueuedLock->Value = 0;
}

PHLIBAPI
VOID
FASTCALL
PhfAcquireQueuedLockExclusive(
    __inout PPH_QUEUED_LOCK QueuedLock
    );

PHLIBAPI
VOID
FASTCALL
PhfAcquireQueuedLockShared(
    __inout PPH_QUEUED_LOCK QueuedLock
    );

PHLIBAPI
VOID
FASTCALL
PhfReleaseQueuedLockExclusive(
    __inout PPH_QUEUED_LOCK QueuedLock
    );

PHLIBAPI
VOID
FASTCALL
PhfReleaseQueuedLockShared(
    __inout PPH_QUEUED_LOCK QueuedLock
    );

PHLIBAPI
VOID
FASTCALL
PhfTryWakeQueuedLock(
    __inout PPH_QUEUED_LOCK QueuedLock
    );

PHLIBAPI
VOID
FASTCALL
PhfWakeForReleaseQueuedLock(
    __inout PPH_QUEUED_LOCK QueuedLock,
    __in ULONG_PTR Value
    );

#define PhPulseCondition PhfPulseCondition
PHLIBAPI
VOID
FASTCALL
PhfPulseCondition(
    __inout PPH_QUEUED_LOCK Condition
    );

#define PhPulseAllCondition PhfPulseAllCondition
PHLIBAPI
VOID
FASTCALL
PhfPulseAllCondition(
    __inout PPH_QUEUED_LOCK Condition
    );

#define PhWaitForCondition PhfWaitForCondition
PHLIBAPI
VOID
FASTCALL
PhfWaitForCondition(
    __inout PPH_QUEUED_LOCK Condition,
    __inout PPH_QUEUED_LOCK Lock,
    __in_opt PLARGE_INTEGER Timeout
    );

#define PH_CONDITION_WAIT_QUEUED_LOCK 0x1
#define PH_CONDITION_WAIT_CRITICAL_SECTION 0x2
#define PH_CONDITION_WAIT_FAST_LOCK 0x4
#define PH_CONDITION_WAIT_LOCK_TYPE_MASK 0xfff

#define PH_CONDITION_WAIT_SHARED 0x1000
#define PH_CONDITION_WAIT_SPIN 0x2000

#define PhWaitForConditionEx PhfWaitForConditionEx
PHLIBAPI
VOID
FASTCALL
PhfWaitForConditionEx(
    __inout PPH_QUEUED_LOCK Condition,
    __inout PVOID Lock,
    __in ULONG Flags,
    __in_opt PLARGE_INTEGER Timeout
    );

#define PhQueueWakeEvent PhfQueueWakeEvent
PHLIBAPI
VOID
FASTCALL
PhfQueueWakeEvent(
    __inout PPH_QUEUED_LOCK WakeEvent,
    __out PPH_QUEUED_WAIT_BLOCK WaitBlock
    );

PHLIBAPI
VOID
FASTCALL
PhfSetWakeEvent(
    __inout PPH_QUEUED_LOCK WakeEvent,
    __inout_opt PPH_QUEUED_WAIT_BLOCK WaitBlock
    );

#define PhWaitForWakeEvent PhfWaitForWakeEvent
PHLIBAPI
NTSTATUS
FASTCALL
PhfWaitForWakeEvent(
    __inout PPH_QUEUED_LOCK WakeEvent,
    __inout PPH_QUEUED_WAIT_BLOCK WaitBlock,
    __in BOOLEAN Spin,
    __in_opt PLARGE_INTEGER Timeout
    );

// Inline functions

FORCEINLINE VOID PhAcquireQueuedLockExclusive(
    __inout PPH_QUEUED_LOCK QueuedLock
    )
{
    if (_InterlockedBitTestAndSetPointer((PLONG_PTR)&QueuedLock->Value, PH_QUEUED_LOCK_OWNED_SHIFT))
    {
        // Owned bit was already set. Slow path.
        PhfAcquireQueuedLockExclusive(QueuedLock);
    }
}

FORCEINLINE VOID PhAcquireQueuedLockShared(
    __inout PPH_QUEUED_LOCK QueuedLock
    )
{
    if ((ULONG_PTR)_InterlockedCompareExchangePointer(
        (PPVOID)&QueuedLock->Value,
        (PVOID)(PH_QUEUED_LOCK_OWNED | PH_QUEUED_LOCK_SHARED_INC),
        (PVOID)0
        ) != 0)
    {
        PhfAcquireQueuedLockShared(QueuedLock);
    }
}

FORCEINLINE BOOLEAN PhTryAcquireQueuedLockExclusive(
    __inout PPH_QUEUED_LOCK QueuedLock
    )
{
    if (!_InterlockedBitTestAndSetPointer((PLONG_PTR)&QueuedLock->Value, PH_QUEUED_LOCK_OWNED_SHIFT))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

FORCEINLINE VOID PhReleaseQueuedLockExclusive(
    __inout PPH_QUEUED_LOCK QueuedLock
    )
{
    ULONG_PTR value;

    value = (ULONG_PTR)_InterlockedExchangeAddPointer((PLONG_PTR)&QueuedLock->Value, -(LONG_PTR)PH_QUEUED_LOCK_OWNED);

    if ((value & (PH_QUEUED_LOCK_WAITERS | PH_QUEUED_LOCK_TRAVERSING)) == PH_QUEUED_LOCK_WAITERS)
    {
        PhfWakeForReleaseQueuedLock(QueuedLock, value - PH_QUEUED_LOCK_OWNED);
    }
}

FORCEINLINE VOID PhReleaseQueuedLockShared(
    __inout PPH_QUEUED_LOCK QueuedLock
    )
{
    ULONG_PTR value;

    value = PH_QUEUED_LOCK_OWNED | PH_QUEUED_LOCK_SHARED_INC;

    if ((ULONG_PTR)_InterlockedCompareExchangePointer(
        (PPVOID)&QueuedLock->Value,
        (PVOID)0,
        (PVOID)value
        ) != value)
    {
        PhfReleaseQueuedLockShared(QueuedLock);
    }
}

FORCEINLINE VOID PhAcquireReleaseQueuedLockExclusive(
    __inout PPH_QUEUED_LOCK QueuedLock
    )
{
    BOOLEAN owned;

    MemoryBarrier();
    owned = !!(QueuedLock->Value & PH_QUEUED_LOCK_OWNED);
    MemoryBarrier();

    if (owned)
    {
        PhAcquireQueuedLockExclusive(QueuedLock);
        PhReleaseQueuedLockExclusive(QueuedLock);
    }
}

FORCEINLINE BOOLEAN PhTryAcquireReleaseQueuedLockExclusive(
    __inout PPH_QUEUED_LOCK QueuedLock
    )
{
    BOOLEAN owned;

    // Need two memory barriers because we don't want the 
    // compiler re-ordering the following check in either 
    // direction.
    MemoryBarrier();
    owned = !(QueuedLock->Value & PH_QUEUED_LOCK_OWNED);
    MemoryBarrier();

    return owned;
}

FORCEINLINE VOID PhSetWakeEvent(
    __inout PPH_QUEUED_LOCK WakeEvent,
    __inout_opt PPH_QUEUED_WAIT_BLOCK WaitBlock
    )
{
    // The wake event is similar to a synchronization event 
    // in that it does not have thread-safe pulsing; we can 
    // simply skip the function call if there's nothing to 
    // wake. However, if we're cancelling a wait 
    // (WaitBlock != NULL) we need to make the call.

    if (WakeEvent->Value || WaitBlock)
        PhfSetWakeEvent(WakeEvent, WaitBlock);
}

#ifdef __cplusplus
}
#endif

#endif
