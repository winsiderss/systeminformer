#ifndef QUEUEDLOCK_H
#define QUEUEDLOCK_H

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

BOOLEAN PhQueuedLockInitialization();

FORCEINLINE VOID PhInitializeQueuedLock(
    __out PPH_QUEUED_LOCK QueuedLock
    )
{
    QueuedLock->Value = 0;
}

#define PhAcquireQueuedLockExclusive PhfAcquireQueuedLockExclusive
VOID FASTCALL PhfAcquireQueuedLockExclusive(
    __inout PPH_QUEUED_LOCK QueuedLock
    );

#define PhAcquireQueuedLockShared PhfAcquireQueuedLockShared
VOID FASTCALL PhfAcquireQueuedLockShared(
    __inout PPH_QUEUED_LOCK QueuedLock
    );

#define PhReleaseQueuedLockExclusive PhfReleaseQueuedLockExclusive
VOID FASTCALL PhfReleaseQueuedLockExclusive(
    __inout PPH_QUEUED_LOCK QueuedLock
    );

#define PhReleaseQueuedLockShared PhfReleaseQueuedLockShared
VOID FASTCALL PhfReleaseQueuedLockShared(
    __inout PPH_QUEUED_LOCK QueuedLock
    );

#define PhTryWakeQueuedLock PhfTryWakeQueuedLock
VOID FASTCALL PhfTryWakeQueuedLock(
    __inout PPH_QUEUED_LOCK QueuedLock
    );

#define PhPulseCondition PhfPulseCondition
VOID FASTCALL PhfPulseCondition(
    __inout PPH_QUEUED_LOCK Condition
    );

#define PhPulseAllCondition PhfPulseAllCondition
VOID FASTCALL PhfPulseAllCondition(
    __inout PPH_QUEUED_LOCK Condition
    );

#define PhWaitForCondition PhfWaitForCondition
VOID FASTCALL PhfWaitForCondition(
    __inout PPH_QUEUED_LOCK Condition,
    __inout_opt PPH_QUEUED_LOCK Lock,
    __in_opt PLARGE_INTEGER Timeout
    );

#define PhQueueWakeEvent PhfQueueWakeEvent
VOID FASTCALL PhfQueueWakeEvent(
    __inout PPH_QUEUED_LOCK WakeEvent,
    __out PPH_QUEUED_WAIT_BLOCK WaitBlock
    );

#define PhSetWakeEvent PhfSetWakeEvent
VOID FASTCALL PhfSetWakeEvent(
    __inout PPH_QUEUED_LOCK WakeEvent
    );

#define PhWaitForWakeEvent PhfWaitForWakeEvent
NTSTATUS FASTCALL PhfWaitForWakeEvent(
    __inout PPH_QUEUED_LOCK WakeEvent,
    __inout PPH_QUEUED_WAIT_BLOCK WaitBlock,
    __in_opt PLARGE_INTEGER Timeout
    );

// Inline functions

FORCEINLINE VOID PhAcquireQueuedLockExclusiveFast(
    __inout PPH_QUEUED_LOCK QueuedLock
    )
{
#ifdef _M_IX86
    if (_interlockedbittestandset((PLONG)&QueuedLock->Value, PH_QUEUED_LOCK_OWNED_SHIFT))
#else
    if (_interlockedbittestandset64((PLONG64)&QueuedLock->Value, PH_QUEUED_LOCK_OWNED_SHIFT))
#endif
    {
        // Owned bit was already set. Slow path.
        PhAcquireQueuedLockExclusive(QueuedLock);
    }
}

FORCEINLINE VOID PhAcquireQueuedLockSharedFast(
    __inout PPH_QUEUED_LOCK QueuedLock
    )
{
    if ((ULONG_PTR)_InterlockedCompareExchangePointer(
        (PPVOID)&QueuedLock->Value,
        (PVOID)(PH_QUEUED_LOCK_OWNED | PH_QUEUED_LOCK_SHARED_INC),
        (PVOID)0
        ) != 0)
    {
        PhAcquireQueuedLockShared(QueuedLock);
    }
}

FORCEINLINE BOOLEAN PhTryAcquireQueuedLockExclusive(
    __inout PPH_QUEUED_LOCK QueuedLock
    )
{
#ifdef _M_IX86
    if (!_interlockedbittestandset((PLONG)&QueuedLock->Value, PH_QUEUED_LOCK_OWNED_SHIFT))
#else
    if (!_interlockedbittestandset64((PLONG64)&QueuedLock->Value, PH_QUEUED_LOCK_OWNED_SHIFT))
#endif
    {
        return TRUE; 
    }
    else
    {
        return FALSE;
    }
}

FORCEINLINE VOID PhReleaseQueuedLockExclusiveFast(
    __inout PPH_QUEUED_LOCK QueuedLock
    )
{
    ULONG_PTR value;

    value = (ULONG_PTR)_InterlockedExchangeAddPointer((PLONG_PTR)&QueuedLock->Value, -(LONG_PTR)PH_QUEUED_LOCK_OWNED);

    // Only check for waiters here, not the traversing bit, since 
    // that isn't common.
    if (value & PH_QUEUED_LOCK_WAITERS)
    {
        PhfTryWakeQueuedLock(QueuedLock);
    }
}

FORCEINLINE VOID PhReleaseQueuedLockSharedFast(
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
        PhReleaseQueuedLockShared(QueuedLock);
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
        PhAcquireQueuedLockExclusiveFast(QueuedLock);
        PhReleaseQueuedLockExclusiveFast(QueuedLock);
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

FORCEINLINE VOID PhSetWakeEventFast(
    __inout PPH_QUEUED_LOCK WakeEvent
    )
{
    if (WakeEvent->Value)
        PhSetWakeEvent(WakeEvent);
}

#endif
