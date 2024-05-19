/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *
 */

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
    /** A pointer to the next wait block, i.e. the wait block pushed onto the list before this one. */
    struct _PH_QUEUED_WAIT_BLOCK *Next;
    /**
     * A pointer to the previous wait block, i.e. the wait block pushed onto the list after this
     * one.
     */
    struct _PH_QUEUED_WAIT_BLOCK *Previous;
    /** A pointer to the last wait block, i.e. the first waiter pushed onto the list. */
    struct _PH_QUEUED_WAIT_BLOCK *Last;

    ULONG SharedOwners;
    ULONG Flags;
} PH_QUEUED_WAIT_BLOCK, *PPH_QUEUED_WAIT_BLOCK;

BOOLEAN PhQueuedLockInitialization(
    VOID
    );

// Queued lock

FORCEINLINE
VOID
PhInitializeQueuedLock(
    _Out_ PPH_QUEUED_LOCK QueuedLock
    )
{
    QueuedLock->Value = 0;
}

PHLIBAPI
VOID
FASTCALL
PhfAcquireQueuedLockExclusive(
    _Inout_ PPH_QUEUED_LOCK QueuedLock
    );

_Acquires_exclusive_lock_(*QueuedLock)
FORCEINLINE
VOID
PhAcquireQueuedLockExclusive(
    _Inout_ PPH_QUEUED_LOCK QueuedLock
    )
{
    if (_InterlockedBitTestAndSetPointer((PLONG_PTR)&QueuedLock->Value, PH_QUEUED_LOCK_OWNED_SHIFT))
    {
        // Owned bit was already set. Slow path.
        PhfAcquireQueuedLockExclusive(QueuedLock);
    }
}

PHLIBAPI
VOID
FASTCALL
PhfAcquireQueuedLockShared(
    _Inout_ PPH_QUEUED_LOCK QueuedLock
    );

_Acquires_shared_lock_(*QueuedLock)
FORCEINLINE
VOID
PhAcquireQueuedLockShared(
    _Inout_ PPH_QUEUED_LOCK QueuedLock
    )
{
    if ((ULONG_PTR)_InterlockedCompareExchangePointer(
        (PVOID *)&QueuedLock->Value,
        (PVOID)(PH_QUEUED_LOCK_OWNED | PH_QUEUED_LOCK_SHARED_INC),
        (PVOID)0
        ) != 0)
    {
        PhfAcquireQueuedLockShared(QueuedLock);
    }
}

_When_(return != 0, _Acquires_exclusive_lock_(*QueuedLock))
FORCEINLINE
BOOLEAN
PhTryAcquireQueuedLockExclusive(
    _Inout_ PPH_QUEUED_LOCK QueuedLock
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

PHLIBAPI
VOID
FASTCALL
PhfReleaseQueuedLockExclusive(
    _Inout_ PPH_QUEUED_LOCK QueuedLock
    );

PHLIBAPI
VOID
FASTCALL
PhfWakeForReleaseQueuedLock(
    _Inout_ PPH_QUEUED_LOCK QueuedLock,
    _In_ ULONG_PTR Value
    );

_Releases_exclusive_lock_(*QueuedLock)
FORCEINLINE
VOID
PhReleaseQueuedLockExclusive(
    _Inout_ PPH_QUEUED_LOCK QueuedLock
    )
{
    ULONG_PTR value;

    value = (ULONG_PTR)_InterlockedExchangeAddPointer((PLONG_PTR)&QueuedLock->Value, -(LONG_PTR)PH_QUEUED_LOCK_OWNED);

    if ((value & (PH_QUEUED_LOCK_WAITERS | PH_QUEUED_LOCK_TRAVERSING)) == PH_QUEUED_LOCK_WAITERS)
    {
        PhfWakeForReleaseQueuedLock(QueuedLock, value - PH_QUEUED_LOCK_OWNED);
    }
}

PHLIBAPI
VOID
FASTCALL
PhfReleaseQueuedLockShared(
    _Inout_ PPH_QUEUED_LOCK QueuedLock
    );

_Releases_shared_lock_(*QueuedLock)
FORCEINLINE
VOID
PhReleaseQueuedLockShared(
    _Inout_ PPH_QUEUED_LOCK QueuedLock
    )
{
    ULONG_PTR value;

    value = PH_QUEUED_LOCK_OWNED | PH_QUEUED_LOCK_SHARED_INC;

    if ((ULONG_PTR)_InterlockedCompareExchangePointer(
        (PVOID *)&QueuedLock->Value,
        (PVOID)0,
        (PVOID)value
        ) != value)
    {
        PhfReleaseQueuedLockShared(QueuedLock);
    }
}

FORCEINLINE
VOID
PhAcquireReleaseQueuedLockExclusive(
    _Inout_ PPH_QUEUED_LOCK QueuedLock
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

FORCEINLINE
BOOLEAN
PhTryAcquireReleaseQueuedLockExclusive(
    _Inout_ PPH_QUEUED_LOCK QueuedLock
    )
{
    BOOLEAN owned;

    // Need two memory barriers because we don't want the compiler re-ordering the following check
    // in either direction.
    MemoryBarrier();
    owned = !(QueuedLock->Value & PH_QUEUED_LOCK_OWNED);
    MemoryBarrier();

    return owned;
}

// Condition variable

typedef struct _PH_QUEUED_LOCK PH_CONDITION, *PPH_CONDITION;

#define PH_CONDITION_INIT PH_QUEUED_LOCK_INIT

FORCEINLINE
VOID
PhInitializeCondition(
    _Out_ PPH_CONDITION Condition
    )
{
    PhInitializeQueuedLock(Condition);
}

#define PhPulseCondition PhfPulseCondition
PHLIBAPI
VOID
FASTCALL
PhfPulseCondition(
    _Inout_ PPH_CONDITION Condition
    );

#define PhPulseAllCondition PhfPulseAllCondition
PHLIBAPI
VOID
FASTCALL
PhfPulseAllCondition(
    _Inout_ PPH_CONDITION Condition
    );

#define PhWaitForCondition PhfWaitForCondition
PHLIBAPI
VOID
FASTCALL
PhfWaitForCondition(
    _Inout_ PPH_CONDITION Condition,
    _Inout_ PPH_QUEUED_LOCK Lock,
    _In_opt_ PLARGE_INTEGER Timeout
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
    _Inout_ PPH_CONDITION Condition,
    _Inout_ PVOID Lock,
    _In_ ULONG Flags,
    _In_opt_ PLARGE_INTEGER Timeout
    );

// Wake event

typedef struct _PH_QUEUED_LOCK PH_WAKE_EVENT, *PPH_WAKE_EVENT;

#define PH_WAKE_EVENT_INIT PH_QUEUED_LOCK_INIT

FORCEINLINE
VOID
PhInitializeWakeEvent(
    _Out_ PPH_WAKE_EVENT WakeEvent
    )
{
    PhInitializeQueuedLock(WakeEvent);
}

#define PhQueueWakeEvent PhfQueueWakeEvent
PHLIBAPI
VOID
FASTCALL
PhfQueueWakeEvent(
    _Inout_ PPH_WAKE_EVENT WakeEvent,
    _Out_ PPH_QUEUED_WAIT_BLOCK WaitBlock
    );

PHLIBAPI
VOID
FASTCALL
PhfSetWakeEvent(
    _Inout_ PPH_WAKE_EVENT WakeEvent,
    _Inout_opt_ PPH_QUEUED_WAIT_BLOCK WaitBlock
    );

FORCEINLINE
VOID
PhSetWakeEvent(
    _Inout_ PPH_WAKE_EVENT WakeEvent,
    _Inout_opt_ PPH_QUEUED_WAIT_BLOCK WaitBlock
    )
{
    // The wake event is similar to a synchronization event in that it does not have thread-safe
    // pulsing; we can simply skip the function call if there's nothing to wake. However, if we're
    // cancelling a wait (WaitBlock != NULL) we need to make the call.

    if (WakeEvent->Value || WaitBlock)
        PhfSetWakeEvent(WakeEvent, WaitBlock);
}

#define PhWaitForWakeEvent PhfWaitForWakeEvent
PHLIBAPI
NTSTATUS
FASTCALL
PhfWaitForWakeEvent(
    _Inout_ PPH_WAKE_EVENT WakeEvent,
    _Inout_ PPH_QUEUED_WAIT_BLOCK WaitBlock,
    _In_ BOOLEAN Spin,
    _In_opt_ PLARGE_INTEGER Timeout
    );

#ifdef __cplusplus
}
#endif

#endif
