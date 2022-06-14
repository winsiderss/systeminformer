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

/*
 * Queued lock, a.k.a. push lock (kernel-mode) or slim reader-writer lock (user-mode).
 *
 * The queued lock is:
 * * Around 10% faster than the fast lock.
 * * Only the size of a pointer.
 * * Low on resource usage (no additional kernel objects are created for blocking).
 *
 * The usual flags are used for contention-free acquire/release. When there is contention,
 * stack-based wait blocks are chained. The first wait block contains the shared owners count which
 * is decremented by shared releasers.
 *
 * Naturally these wait blocks would be chained in FILO order, but list optimization is done for two
 * purposes:
 * * Finding the last wait block (where the shared owners count is stored). This is implemented by
 *   the Last pointer.
 * * Unblocking the wait blocks in FIFO order. This is implemented by the Previous pointer.
 *
 * The optimization is incremental - each optimization run will stop at the first optimized wait
 * block. Any needed optimization is completed just before waking waiters.
 *
 * The waiters list/chain has the following restrictions:
 * * At any time wait blocks may be pushed onto the list.
 * * While waking waiters, the list may not be traversed nor optimized.
 * * When there are multiple shared owners, shared releasers may traverse the list (to find the last
 *   wait block). This is not an issue because waiters wouldn't be woken until there are no more
 *   shared owners.
 * * List optimization may be done at any time except for when someone else is waking waiters. This
 *   is controlled by the traversing bit.
 *
 * The traversing bit has the following rules:
 * * The list may be optimized only after the traversing bit is set, checking that it wasn't set
 *   already. If it was set, it would indicate that someone else is optimizing the list or waking
 *   waiters.
 * * Before waking waiters the traversing bit must be set. If it was set already, just clear the
 *   owned bit.
 * * If during list optimization the owned bit is detected to be cleared, the function begins waking
 *   waiters. This is because the owned bit is cleared when a releaser fails to set the traversing
 *   bit.
 *
 * Blocking is implemented through a process-wide keyed event. A spin count is also used before
 * blocking on the keyed event.
 *
 * Queued locks can act as condition variables, with wait, pulse and pulse all support. Waiters are
 * released in FIFO order.
 *
 * Queued locks can act as wake events. These are designed for tiny one-bit locks which share a
 * single event to block on. Spurious wake-ups are a part of normal operation.
 */

#include <phbase.h>

#include <fastlock.h>
#include <phintrnl.h>
#include <queuedlock.h>

VOID FASTCALL PhpfOptimizeQueuedLockList(
    _Inout_ PPH_QUEUED_LOCK QueuedLock,
    _In_ ULONG_PTR Value
    );

VOID FASTCALL PhpfWakeQueuedLock(
    _Inout_ PPH_QUEUED_LOCK QueuedLock,
    _In_ ULONG_PTR Value
    );

VOID FASTCALL PhpfWakeQueuedLockEx(
    _Inout_ PPH_QUEUED_LOCK QueuedLock,
    _In_ ULONG_PTR Value,
    _In_ BOOLEAN IgnoreOwned,
    _In_ BOOLEAN WakeAll
    );

static HANDLE PhQueuedLockKeyedEventHandle;
static ULONG PhQueuedLockSpinCount = 2000;

BOOLEAN PhQueuedLockInitialization(
    VOID
    )
{
    if (!NT_SUCCESS(NtCreateKeyedEvent(
        &PhQueuedLockKeyedEventHandle,
        KEYEDEVENT_ALL_ACCESS,
        NULL,
        0
        )))
        return FALSE;

    if ((ULONG)PhSystemBasicInformation.NumberOfProcessors > 1)
        PhQueuedLockSpinCount = 4000;
    else
        PhQueuedLockSpinCount = 0;

    return TRUE;
}

/**
 * Pushes a wait block onto a queued lock's waiters list.
 *
 * \param QueuedLock A queued lock.
 * \param Value The current value of the queued lock.
 * \param Exclusive Whether the wait block is in exclusive mode.
 * \param WaitBlock A variable which receives the resulting wait block structure.
 * \param Optimize A variable which receives a boolean indicating whether to optimize the waiters
 * list.
 * \param NewValue The old value of the queued lock. This value is useful only if the function
 * returns FALSE.
 * \param CurrentValue The new value of the queued lock. This value is useful only if the function
 * returns TRUE.
 *
 * \return TRUE if the wait block was pushed onto the waiters list, otherwise FALSE.
 *
 * \remarks
 * \li The function assumes the following flags are set:
 * \ref PH_QUEUED_LOCK_OWNED.
 * \li Do not move the wait block location after this function is called.
 * \li The \a Optimize boolean is a hint to call PhpfOptimizeQueuedLockList() if the function
 * succeeds. It is recommended, but not essential that this occurs.
 * \li Call PhpBlockOnQueuedWaitBlock() to wait for the wait block to be unblocked.
 */
FORCEINLINE BOOLEAN PhpPushQueuedWaitBlock(
    _Inout_ PPH_QUEUED_LOCK QueuedLock,
    _In_ ULONG_PTR Value,
    _In_ BOOLEAN Exclusive,
    _Out_ PPH_QUEUED_WAIT_BLOCK WaitBlock,
    _Out_ PBOOLEAN Optimize,
    _Out_ PULONG_PTR NewValue,
    _Out_ PULONG_PTR CurrentValue
    )
{
    ULONG_PTR newValue;
    BOOLEAN optimize;

    WaitBlock->Previous = NULL; // set later by optimization
    optimize = FALSE;

    if (Exclusive)
        WaitBlock->Flags = PH_QUEUED_WAITER_EXCLUSIVE | PH_QUEUED_WAITER_SPINNING;
    else
        WaitBlock->Flags = PH_QUEUED_WAITER_SPINNING;

    if (Value & PH_QUEUED_LOCK_WAITERS)
    {
        // We're not the first waiter.

        WaitBlock->Last = NULL; // set later by optimization
        WaitBlock->Next = PhGetQueuedLockWaitBlock(Value);
        WaitBlock->SharedOwners = 0;

        // Push our wait block onto the list.
        // Set the traversing bit because we'll be optimizing the list.
        newValue = ((ULONG_PTR)WaitBlock) | (Value & PH_QUEUED_LOCK_FLAGS) |
            PH_QUEUED_LOCK_TRAVERSING;

        if (!(Value & PH_QUEUED_LOCK_TRAVERSING))
            optimize = TRUE;
    }
    else
    {
        // We're the first waiter.

        WaitBlock->Last = WaitBlock; // indicate that this is the last wait block

        if (Exclusive)
        {
            // We're the first waiter. Save the shared owners count.
            WaitBlock->SharedOwners = (ULONG)PhGetQueuedLockSharedOwners(Value);

            if (WaitBlock->SharedOwners > 1)
            {
                newValue = ((ULONG_PTR)WaitBlock) | PH_QUEUED_LOCK_OWNED |
                    PH_QUEUED_LOCK_WAITERS | PH_QUEUED_LOCK_MULTIPLE_SHARED;
            }
            else
            {
                newValue = ((ULONG_PTR)WaitBlock) | PH_QUEUED_LOCK_OWNED |
                    PH_QUEUED_LOCK_WAITERS;
            }
        }
        else
        {
            // We're waiting in shared mode, which means there can't be any shared owners (otherwise
            // we would've acquired the lock already).

            WaitBlock->SharedOwners = 0;

            newValue = ((ULONG_PTR)WaitBlock) | PH_QUEUED_LOCK_OWNED |
                PH_QUEUED_LOCK_WAITERS;
        }
    }

    *Optimize = optimize;
    *CurrentValue = newValue;

    newValue = (ULONG_PTR)_InterlockedCompareExchangePointer(
        (PVOID *)&QueuedLock->Value,
        (PVOID)newValue,
        (PVOID)Value
        );

    *NewValue = newValue;

    return newValue == Value;
}

/**
 * Finds the last wait block in the waiters list.
 *
 * \param Value The current value of the queued lock.
 *
 * \return A pointer to the last wait block.
 *
 * \remarks The function assumes the following flags are set:
 * \ref PH_QUEUED_LOCK_WAITERS,
 * \ref PH_QUEUED_LOCK_MULTIPLE_SHARED or
 * \ref PH_QUEUED_LOCK_TRAVERSING.
 */
FORCEINLINE PPH_QUEUED_WAIT_BLOCK PhpFindLastQueuedWaitBlock(
    _In_ ULONG_PTR Value
    )
{
    PPH_QUEUED_WAIT_BLOCK waitBlock;
    PPH_QUEUED_WAIT_BLOCK lastWaitBlock;

    waitBlock = PhGetQueuedLockWaitBlock(Value);

    // Traverse the list until we find the last wait block.
    // The Last pointer should be set by list optimization, allowing us to skip all, if not most of
    // the wait blocks.

    while (TRUE)
    {
        lastWaitBlock = waitBlock->Last;

        if (lastWaitBlock)
        {
            // Follow the Last pointer. This can mean two things: the pointer was set by list
            // optimization, or this wait block is actually the last wait block (set when it was
            // pushed onto the list).
            waitBlock = lastWaitBlock;
            break;
        }

        waitBlock = waitBlock->Next;
    }

    return waitBlock;
}

/**
 * Waits for a wait block to be unblocked.
 *
 * \param WaitBlock A wait block.
 * \param Spin TRUE to spin, FALSE to block immediately.
 * \param Timeout A timeout value.
 */
_May_raise_ FORCEINLINE NTSTATUS PhpBlockOnQueuedWaitBlock(
    _Inout_ PPH_QUEUED_WAIT_BLOCK WaitBlock,
    _In_ BOOLEAN Spin,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    NTSTATUS status;
    ULONG i;

    if (Spin)
    {
        PHLIB_INC_STATISTIC(QlBlockSpins);

        for (i = PhQueuedLockSpinCount; i != 0; i--)
        {
            if (!(*(volatile ULONG *)&WaitBlock->Flags & PH_QUEUED_WAITER_SPINNING))
                return STATUS_SUCCESS;

            YieldProcessor();
        }
    }

    if (_interlockedbittestandreset((PLONG)&WaitBlock->Flags, PH_QUEUED_WAITER_SPINNING_SHIFT))
    {
        PHLIB_INC_STATISTIC(QlBlockWaits);

        status = NtWaitForKeyedEvent(
            PhQueuedLockKeyedEventHandle,
            WaitBlock,
            FALSE,
            Timeout
            );

        // If an error occurred (timeout is not an error), raise an exception as it is nearly
        // impossible to recover from this situation.
        if (!NT_SUCCESS(status))
            PhRaiseStatus(status);
    }
    else
    {
        status = STATUS_SUCCESS;
    }

    return status;
}

/**
 * Unblocks a wait block.
 *
 * \param WaitBlock A wait block.
 *
 * \remarks The wait block is in an undefined state after it is unblocked. Do not attempt to read
 * any values from it. All relevant information should be saved before unblocking the wait block.
 */
_May_raise_ FORCEINLINE VOID PhpUnblockQueuedWaitBlock(
    _Inout_ PPH_QUEUED_WAIT_BLOCK WaitBlock
    )
{
    NTSTATUS status;

    if (!_interlockedbittestandreset((PLONG)&WaitBlock->Flags, PH_QUEUED_WAITER_SPINNING_SHIFT))
    {
        if (!NT_SUCCESS(status = NtReleaseKeyedEvent(
            PhQueuedLockKeyedEventHandle,
            WaitBlock,
            FALSE,
            NULL
            )))
            PhRaiseStatus(status);
    }
}

/**
 * Optimizes a queued lock waiters list.
 *
 * \param QueuedLock A queued lock.
 * \param Value The current value of the queued lock.
 * \param IgnoreOwned TRUE to ignore lock state, FALSE to conduct normal checks.
 *
 * \remarks The function assumes the following flags are set:
 * \ref PH_QUEUED_LOCK_WAITERS, \ref PH_QUEUED_LOCK_TRAVERSING.
 */
FORCEINLINE VOID PhpOptimizeQueuedLockListEx(
    _Inout_ PPH_QUEUED_LOCK QueuedLock,
    _In_ ULONG_PTR Value,
    _In_ BOOLEAN IgnoreOwned
    )
{
    ULONG_PTR value;
    ULONG_PTR newValue;
    PPH_QUEUED_WAIT_BLOCK waitBlock;
    PPH_QUEUED_WAIT_BLOCK firstWaitBlock;
    PPH_QUEUED_WAIT_BLOCK lastWaitBlock;
    PPH_QUEUED_WAIT_BLOCK previousWaitBlock;

    value = Value;

    while (TRUE)
    {
        assert(value & PH_QUEUED_LOCK_TRAVERSING);

        if (!IgnoreOwned && !(value & PH_QUEUED_LOCK_OWNED))
        {
            // Someone has requested that we wake waiters.
            PhpfWakeQueuedLock(QueuedLock, value);
            break;
        }

        // Perform the optimization.

        waitBlock = PhGetQueuedLockWaitBlock(value);
        firstWaitBlock = waitBlock;

        while (TRUE)
        {
            lastWaitBlock = waitBlock->Last;

            if (lastWaitBlock)
            {
                // Save a pointer to the last wait block in the first wait block and stop
                // optimizing.
                //
                // We don't need to continue setting Previous pointers because the last optimization
                // run would have set them already.

                firstWaitBlock->Last = lastWaitBlock;
                break;
            }

            previousWaitBlock = waitBlock;
            waitBlock = waitBlock->Next;
            waitBlock->Previous = previousWaitBlock;
        }

        // Try to clear the traversing bit.

        newValue = value - PH_QUEUED_LOCK_TRAVERSING;

        if ((newValue = (ULONG_PTR)_InterlockedCompareExchangePointer(
            (PVOID *)&QueuedLock->Value,
            (PVOID)newValue,
            (PVOID)value
            )) == value)
            break;

        // Either someone pushed a wait block onto the list or someone released ownership. In either
        // case we need to go back.

        value = newValue;
    }
}

/**
 * Optimizes a queued lock waiters list.
 *
 * \param QueuedLock A queued lock.
 * \param Value The current value of the queued lock.
 *
 * \remarks The function assumes the following flags are set:
 * \ref PH_QUEUED_LOCK_WAITERS, \ref PH_QUEUED_LOCK_TRAVERSING.
 */
VOID FASTCALL PhpfOptimizeQueuedLockList(
    _Inout_ PPH_QUEUED_LOCK QueuedLock,
    _In_ ULONG_PTR Value
    )
{
    PhpOptimizeQueuedLockListEx(QueuedLock, Value, FALSE);
}

/**
 * Dequeues the appropriate number of wait blocks in a queued lock.
 *
 * \param QueuedLock A queued lock.
 * \param Value The current value of the queued lock.
 * \param IgnoreOwned TRUE to ignore lock state, FALSE to conduct normal checks.
 * \param WakeAll TRUE to remove all wait blocks, FALSE to decide based on the wait block type.
 */
FORCEINLINE PPH_QUEUED_WAIT_BLOCK PhpPrepareToWakeQueuedLock(
    _Inout_ PPH_QUEUED_LOCK QueuedLock,
    _In_ ULONG_PTR Value,
    _In_ BOOLEAN IgnoreOwned,
    _In_ BOOLEAN WakeAll
    )
{
    ULONG_PTR value;
    ULONG_PTR newValue;
    PPH_QUEUED_WAIT_BLOCK waitBlock;
    PPH_QUEUED_WAIT_BLOCK firstWaitBlock;
    PPH_QUEUED_WAIT_BLOCK lastWaitBlock;
    PPH_QUEUED_WAIT_BLOCK previousWaitBlock;

    value = Value;

    while (TRUE)
    {
        // If there are multiple shared owners, no one is going to wake waiters since the lock would
        // still be owned. Also if there are multiple shared owners they may be traversing the list.
        // While that is safe when done concurrently with list optimization, we may be removing and
        // waking waiters.
        assert(!(value & PH_QUEUED_LOCK_MULTIPLE_SHARED));
        assert(IgnoreOwned || (value & PH_QUEUED_LOCK_TRAVERSING));

        // There's no point in waking a waiter if the lock is owned. Clear the traversing bit.
        while (!IgnoreOwned && (value & PH_QUEUED_LOCK_OWNED))
        {
            newValue = value - PH_QUEUED_LOCK_TRAVERSING;

            if ((newValue = (ULONG_PTR)_InterlockedCompareExchangePointer(
                (PVOID *)&QueuedLock->Value,
                (PVOID)newValue,
                (PVOID)value
                )) == value)
                return NULL;

            value = newValue;
        }

        // Finish up any needed optimization (setting the Previous pointers) while finding the last
        // wait block.

        waitBlock = PhGetQueuedLockWaitBlock(value);
        firstWaitBlock = waitBlock;

        while (TRUE)
        {
            lastWaitBlock = waitBlock->Last;

            if (lastWaitBlock)
            {
                waitBlock = lastWaitBlock;
                break;
            }

            previousWaitBlock = waitBlock;
            waitBlock = waitBlock->Next;
            waitBlock->Previous = previousWaitBlock;
        }

        // Unlink the relevant wait blocks and clear the traversing bit before we wake waiters.

        if (
            !WakeAll &&
            (waitBlock->Flags & PH_QUEUED_WAITER_EXCLUSIVE) &&
            (previousWaitBlock = waitBlock->Previous)
            )
        {
            // We have an exclusive waiter and there are multiple waiters. We'll only be waking this
            // waiter.

            // Unlink the wait block from the list. Although other wait blocks may have their Last
            // pointers set to this wait block, the algorithm to find the last wait block will stop
            // here. Likewise the Next pointers are never followed beyond this point, so we don't
            // need to clear those.
            firstWaitBlock->Last = previousWaitBlock;

            // Make sure we only wake this waiter.
            waitBlock->Previous = NULL;

            if (!IgnoreOwned)
            {
                // Clear the traversing bit.
                _InterlockedExchangeAddPointer((PLONG_PTR)&QueuedLock->Value, -(LONG_PTR)PH_QUEUED_LOCK_TRAVERSING);
            }

            break;
        }
        else
        {
            // We're waking an exclusive waiter and there is only one waiter, or we are waking a
            // shared waiter and possibly others.

            newValue = 0;

            if ((newValue = (ULONG_PTR)_InterlockedCompareExchangePointer(
                (PVOID *)&QueuedLock->Value,
                (PVOID)newValue,
                (PVOID)value
                )) == value)
                break;

            // Someone changed the lock (acquired it or pushed a wait block).

            value = newValue;
        }
    }

    return waitBlock;
}

/**
 * Wakes waiters in a queued lock.
 *
 * \param QueuedLock A queued lock.
 * \param Value The current value of the queued lock.
 *
 * \remarks The function assumes the following flags are set:
 * \ref PH_QUEUED_LOCK_WAITERS, \ref PH_QUEUED_LOCK_TRAVERSING. The function assumes the following
 * flags are not set:
 * \ref PH_QUEUED_LOCK_MULTIPLE_SHARED.
 */
VOID FASTCALL PhpfWakeQueuedLock(
    _Inout_ PPH_QUEUED_LOCK QueuedLock,
    _In_ ULONG_PTR Value
    )
{
    PPH_QUEUED_WAIT_BLOCK waitBlock;
    PPH_QUEUED_WAIT_BLOCK previousWaitBlock;

    waitBlock = PhpPrepareToWakeQueuedLock(QueuedLock, Value, FALSE, FALSE);

    // Wake waiters.

    while (waitBlock)
    {
        previousWaitBlock = waitBlock->Previous;
        PhpUnblockQueuedWaitBlock(waitBlock);
        waitBlock = previousWaitBlock;
    }
}

/**
 * Wakes waiters in a queued lock.
 *
 * \param QueuedLock A queued lock.
 * \param Value The current value of the queued lock.
 * \param IgnoreOwned TRUE to ignore lock state, FALSE to conduct normal checks.
 * \param WakeAll TRUE to wake all waiters, FALSE to decide based on the wait block type.
 *
 * \remarks The function assumes the following flags are set:
 * \ref PH_QUEUED_LOCK_WAITERS, \ref PH_QUEUED_LOCK_TRAVERSING. The function assumes the following
 * flags are not set:
 * \ref PH_QUEUED_LOCK_MULTIPLE_SHARED.
 */
VOID FASTCALL PhpfWakeQueuedLockEx(
    _Inout_ PPH_QUEUED_LOCK QueuedLock,
    _In_ ULONG_PTR Value,
    _In_ BOOLEAN IgnoreOwned,
    _In_ BOOLEAN WakeAll
    )
{
    PPH_QUEUED_WAIT_BLOCK waitBlock;
    PPH_QUEUED_WAIT_BLOCK previousWaitBlock;

    waitBlock = PhpPrepareToWakeQueuedLock(QueuedLock, Value, IgnoreOwned, WakeAll);

    // Wake waiters.

    while (waitBlock)
    {
        previousWaitBlock = waitBlock->Previous;
        PhpUnblockQueuedWaitBlock(waitBlock);
        waitBlock = previousWaitBlock;
    }
}

/**
 * Acquires a queued lock in exclusive mode.
 *
 * \param QueuedLock A queued lock.
 */
VOID FASTCALL PhfAcquireQueuedLockExclusive(
    _Inout_ PPH_QUEUED_LOCK QueuedLock
    )
{
    ULONG_PTR value;
    ULONG_PTR newValue;
    ULONG_PTR currentValue;
    BOOLEAN optimize;
    PH_QUEUED_WAIT_BLOCK waitBlock;

    value = QueuedLock->Value;

    while (TRUE)
    {
        if (!(value & PH_QUEUED_LOCK_OWNED))
        {
            if ((newValue = (ULONG_PTR)_InterlockedCompareExchangePointer(
                (PVOID *)&QueuedLock->Value,
                (PVOID)(value + PH_QUEUED_LOCK_OWNED),
                (PVOID)value
                )) == value)
                break;
        }
        else
        {
            if (PhpPushQueuedWaitBlock(
                QueuedLock,
                value,
                TRUE,
                &waitBlock,
                &optimize,
                &newValue,
                &currentValue
                ))
            {
                if (optimize)
                    PhpfOptimizeQueuedLockList(QueuedLock, currentValue);

                PHLIB_INC_STATISTIC(QlAcquireExclusiveBlocks);
                PhpBlockOnQueuedWaitBlock(&waitBlock, TRUE, NULL);
            }
        }

        value = newValue;
    }
}

/**
 * Acquires a queued lock in shared mode.
 *
 * \param QueuedLock A queued lock.
 */
VOID FASTCALL PhfAcquireQueuedLockShared(
    _Inout_ PPH_QUEUED_LOCK QueuedLock
    )
{
    ULONG_PTR value;
    ULONG_PTR newValue;
    ULONG_PTR currentValue;
    BOOLEAN optimize;
    PH_QUEUED_WAIT_BLOCK waitBlock;

    value = QueuedLock->Value;

    while (TRUE)
    {
        // We can't acquire if there are waiters for two reasons:
        //
        // We want to prioritize exclusive acquires over shared acquires. There's currently no fast,
        // safe way of finding the last wait block and incrementing the shared owners count here.
        if (
            !(value & PH_QUEUED_LOCK_WAITERS) &&
            (!(value & PH_QUEUED_LOCK_OWNED) || (PhGetQueuedLockSharedOwners(value) > 0))
            )
        {
            newValue = (value + PH_QUEUED_LOCK_SHARED_INC) | PH_QUEUED_LOCK_OWNED;

            if ((newValue = (ULONG_PTR)_InterlockedCompareExchangePointer(
                (PVOID *)&QueuedLock->Value,
                (PVOID)newValue,
                (PVOID)value
                )) == value)
                break;
        }
        else
        {
            if (PhpPushQueuedWaitBlock(
                QueuedLock,
                value,
                FALSE,
                &waitBlock,
                &optimize,
                &newValue,
                &currentValue
                ))
            {
                if (optimize)
                    PhpfOptimizeQueuedLockList(QueuedLock, currentValue);

                PHLIB_INC_STATISTIC(QlAcquireSharedBlocks);
                PhpBlockOnQueuedWaitBlock(&waitBlock, TRUE, NULL);
            }
        }

        value = newValue;
    }
}

/**
 * Releases a queued lock in exclusive mode.
 *
 * \param QueuedLock A queued lock.
 */
VOID FASTCALL PhfReleaseQueuedLockExclusive(
    _Inout_ PPH_QUEUED_LOCK QueuedLock
    )
{
    ULONG_PTR value;
    ULONG_PTR newValue;
    ULONG_PTR currentValue;

    value = QueuedLock->Value;

    while (TRUE)
    {
        assert(value & PH_QUEUED_LOCK_OWNED);
        assert((value & PH_QUEUED_LOCK_WAITERS) || (PhGetQueuedLockSharedOwners(value) == 0));

        if ((value & (PH_QUEUED_LOCK_WAITERS | PH_QUEUED_LOCK_TRAVERSING)) != PH_QUEUED_LOCK_WAITERS)
        {
            // There are no waiters or someone is traversing the list.
            //
            // If there are no waiters, we're simply releasing ownership. If someone is traversing
            // the list, clearing the owned bit is a signal for them to wake waiters.

            newValue = value - PH_QUEUED_LOCK_OWNED;

            if ((newValue = (ULONG_PTR)_InterlockedCompareExchangePointer(
                (PVOID *)&QueuedLock->Value,
                (PVOID)newValue,
                (PVOID)value
                )) == value)
                break;
        }
        else
        {
            // We need to wake waiters and no one is traversing the list.
            // Try to set the traversing bit and wake waiters.

            newValue = value - PH_QUEUED_LOCK_OWNED + PH_QUEUED_LOCK_TRAVERSING;
            currentValue = newValue;

            if ((newValue = (ULONG_PTR)_InterlockedCompareExchangePointer(
                (PVOID *)&QueuedLock->Value,
                (PVOID)newValue,
                (PVOID)value
                )) == value)
            {
                PhpfWakeQueuedLock(QueuedLock, currentValue);
                break;
            }
        }

        value = newValue;
    }
}

/**
 * Wakes waiters in a queued lock for releasing it in exclusive mode.
 *
 * \param QueuedLock A queued lock.
 * \param Value The current value of the queued lock.
 *
 * \remarks The function assumes the following flags are set:
 * \ref PH_QUEUED_LOCK_WAITERS. The function assumes the following flags are not set:
 * \ref PH_QUEUED_LOCK_MULTIPLE_SHARED, \ref PH_QUEUED_LOCK_TRAVERSING.
 */
VOID FASTCALL PhfWakeForReleaseQueuedLock(
    _Inout_ PPH_QUEUED_LOCK QueuedLock,
    _In_ ULONG_PTR Value
    )
{
    ULONG_PTR newValue;

    newValue = Value + PH_QUEUED_LOCK_TRAVERSING;

    if ((ULONG_PTR)_InterlockedCompareExchangePointer(
        (PVOID *)&QueuedLock->Value,
        (PVOID)newValue,
        (PVOID)Value
        ) == Value)
    {
        PhpfWakeQueuedLock(QueuedLock, newValue);
    }
}

/**
 * Releases a queued lock in shared mode.
 *
 * \param QueuedLock A queued lock.
 */
VOID FASTCALL PhfReleaseQueuedLockShared(
    _Inout_ PPH_QUEUED_LOCK QueuedLock
    )
{
    ULONG_PTR value;
    ULONG_PTR newValue;
    ULONG_PTR currentValue;
    PPH_QUEUED_WAIT_BLOCK waitBlock;

    value = QueuedLock->Value;

    while (!(value & PH_QUEUED_LOCK_WAITERS))
    {
        assert(value & PH_QUEUED_LOCK_OWNED);
        assert((value & PH_QUEUED_LOCK_WAITERS) || (PhGetQueuedLockSharedOwners(value) > 0));

        if (PhGetQueuedLockSharedOwners(value) > 1)
            newValue = value - PH_QUEUED_LOCK_SHARED_INC;
        else
            newValue = 0;

        if ((newValue = (ULONG_PTR)_InterlockedCompareExchangePointer(
            (PVOID *)&QueuedLock->Value,
            (PVOID)newValue,
            (PVOID)value
            )) == value)
            return;

        value = newValue;
    }

    if (value & PH_QUEUED_LOCK_MULTIPLE_SHARED)
    {
        // Unfortunately we have to find the last wait block and decrement the shared owners count.
        waitBlock = PhpFindLastQueuedWaitBlock(value);

        if ((ULONG)_InterlockedDecrement((PLONG)&waitBlock->SharedOwners) > 0)
            return;
    }

    while (TRUE)
    {
        if (value & PH_QUEUED_LOCK_TRAVERSING)
        {
            newValue = value & ~(PH_QUEUED_LOCK_OWNED | PH_QUEUED_LOCK_MULTIPLE_SHARED);

            if ((newValue = (ULONG_PTR)_InterlockedCompareExchangePointer(
                (PVOID *)&QueuedLock->Value,
                (PVOID)newValue,
                (PVOID)value
                )) == value)
                break;
        }
        else
        {
            newValue = (value & ~(PH_QUEUED_LOCK_OWNED | PH_QUEUED_LOCK_MULTIPLE_SHARED)) |
                PH_QUEUED_LOCK_TRAVERSING;
            currentValue = newValue;

            if ((newValue = (ULONG_PTR)_InterlockedCompareExchangePointer(
                (PVOID *)&QueuedLock->Value,
                (PVOID)newValue,
                (PVOID)value
                )) == value)
            {
                PhpfWakeQueuedLock(QueuedLock, currentValue);
                break;
            }
        }

        value = newValue;
    }
}

/**
 * Wakes one thread sleeping on a condition variable.
 *
 * \param Condition A condition variable.
 *
 * \remarks The associated lock must be acquired before calling the function.
 */
VOID FASTCALL PhfPulseCondition(
    _Inout_ PPH_CONDITION Condition
    )
{
    if (Condition->Value & PH_QUEUED_LOCK_WAITERS)
        PhpfWakeQueuedLockEx(Condition, Condition->Value, TRUE, FALSE);
}

/**
 * Wakes all threads sleeping on a condition variable.
 *
 * \param Condition A condition variable.
 *
 * \remarks The associated lock must be acquired before calling the function.
 */
VOID FASTCALL PhfPulseAllCondition(
    _Inout_ PPH_CONDITION Condition
    )
{
    if (Condition->Value & PH_QUEUED_LOCK_WAITERS)
        PhpfWakeQueuedLockEx(Condition, Condition->Value, TRUE, TRUE);
}

/**
 * Sleeps on a condition variable.
 *
 * \param Condition A condition variable.
 * \param Lock A queued lock to release/acquire in exclusive mode.
 * \param Timeout Not implemented.
 *
 * \remarks The associated lock must be acquired before calling the function.
 */
VOID FASTCALL PhfWaitForCondition(
    _Inout_ PPH_CONDITION Condition,
    _Inout_ PPH_QUEUED_LOCK Lock,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    ULONG_PTR value;
    ULONG_PTR currentValue;
    PH_QUEUED_WAIT_BLOCK waitBlock;
    BOOLEAN optimize;

    value = Condition->Value;

    while (TRUE)
    {
        if (PhpPushQueuedWaitBlock(
            Condition,
            value,
            TRUE,
            &waitBlock,
            &optimize,
            &value,
            &currentValue
            ))
        {
            if (optimize)
            {
                PhpOptimizeQueuedLockListEx(Condition, currentValue, TRUE);
            }

            PhReleaseQueuedLockExclusive(Lock);

            PhpBlockOnQueuedWaitBlock(&waitBlock, FALSE, NULL);

            // Don't use the inline variant; it is extremely likely that the lock is still owned.
            PhfAcquireQueuedLockExclusive(Lock);

            break;
        }
    }
}

/**
 * Sleeps on a condition variable.
 *
 * \param Condition A condition variable.
 * \param Lock A pointer to a lock.
 * \param Flags A combination of flags controlling the operation.
 * \param Timeout Not implemented.
 */
VOID FASTCALL PhfWaitForConditionEx(
    _Inout_ PPH_CONDITION Condition,
    _Inout_ PVOID Lock,
    _In_ ULONG Flags,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    ULONG_PTR value;
    ULONG_PTR currentValue;
    PH_QUEUED_WAIT_BLOCK waitBlock;
    BOOLEAN optimize;

    value = Condition->Value;

    while (TRUE)
    {
        if (PhpPushQueuedWaitBlock(
            Condition,
            value,
            TRUE,
            &waitBlock,
            &optimize,
            &value,
            &currentValue
            ))
        {
            if (optimize)
            {
                PhpOptimizeQueuedLockListEx(Condition, currentValue, TRUE);
            }

            switch (Flags & PH_CONDITION_WAIT_LOCK_TYPE_MASK)
            {
            case PH_CONDITION_WAIT_QUEUED_LOCK:
                if (!(Flags & PH_CONDITION_WAIT_SHARED))
                    PhReleaseQueuedLockExclusive((PPH_QUEUED_LOCK)Lock);
                else
                    PhReleaseQueuedLockShared((PPH_QUEUED_LOCK)Lock);
                break;
            case PH_CONDITION_WAIT_CRITICAL_SECTION:
                RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)Lock);
                break;
            case PH_CONDITION_WAIT_FAST_LOCK:
                if (!(Flags & PH_CONDITION_WAIT_SHARED))
                    PhReleaseFastLockExclusive((PPH_FAST_LOCK)Lock);
                else
                    PhReleaseFastLockShared((PPH_FAST_LOCK)Lock);
                break;
            }

            if (!(Flags & PH_CONDITION_WAIT_SPIN))
            {
                PhpBlockOnQueuedWaitBlock(&waitBlock, FALSE, NULL);
            }
            else
            {
                PhpBlockOnQueuedWaitBlock(&waitBlock, TRUE, NULL);
            }

            switch (Flags & PH_CONDITION_WAIT_LOCK_TYPE_MASK)
            {
            case PH_CONDITION_WAIT_QUEUED_LOCK:
                if (!(Flags & PH_CONDITION_WAIT_SHARED))
                    PhfAcquireQueuedLockExclusive((PPH_QUEUED_LOCK)Lock);
                else
                    PhfAcquireQueuedLockShared((PPH_QUEUED_LOCK)Lock);
                break;
            case PH_CONDITION_WAIT_CRITICAL_SECTION:
                RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)Lock);
                break;
            case PH_CONDITION_WAIT_FAST_LOCK:
                if (!(Flags & PH_CONDITION_WAIT_SHARED))
                    PhAcquireFastLockExclusive((PPH_FAST_LOCK)Lock);
                else
                    PhAcquireFastLockShared((PPH_FAST_LOCK)Lock);
                break;
            }

            break;
        }
    }
}

/**
 * Queues a wait block to a wake event.
 *
 * \param WakeEvent A wake event.
 * \param WaitBlock A wait block.
 *
 * \remarks If you later determine that the wait should not occur, you must call PhfSetWakeEvent()
 * to dequeue the wait block.
 */
VOID FASTCALL PhfQueueWakeEvent(
    _Inout_ PPH_WAKE_EVENT WakeEvent,
    _Out_ PPH_QUEUED_WAIT_BLOCK WaitBlock
    )
{
    PPH_QUEUED_WAIT_BLOCK value;
    PPH_QUEUED_WAIT_BLOCK newValue;

    WaitBlock->Flags = PH_QUEUED_WAITER_SPINNING;

    value = (PPH_QUEUED_WAIT_BLOCK)WakeEvent->Value;

    while (TRUE)
    {
        WaitBlock->Next = value;

        if ((newValue = _InterlockedCompareExchangePointer(
            (PVOID *)&WakeEvent->Value,
            WaitBlock,
            value
            )) == value)
            break;

        value = newValue;
    }
}

/**
 * Sets a wake event, unblocking all queued wait blocks.
 *
 * \param WakeEvent A wake event.
 * \param WaitBlock A wait block for a cancelled wait, otherwise NULL.
 */
VOID FASTCALL PhfSetWakeEvent(
    _Inout_ PPH_WAKE_EVENT WakeEvent,
    _Inout_opt_ PPH_QUEUED_WAIT_BLOCK WaitBlock
    )
{
    PPH_QUEUED_WAIT_BLOCK waitBlock;
    PPH_QUEUED_WAIT_BLOCK nextWaitBlock;

    // Pop all waiters and unblock them.

    waitBlock = _InterlockedExchangePointer((PVOID *)&WakeEvent->Value, NULL);

    while (waitBlock)
    {
        nextWaitBlock = waitBlock->Next;
        PhpUnblockQueuedWaitBlock(waitBlock);
        waitBlock = nextWaitBlock;
    }

    if (WaitBlock)
    {
        // We're cancelling a wait; the thread called this function instead of PhfWaitForWakeEvent.
        // This will remove all waiters from the list. However, we may not have popped and unblocked
        // the cancelled wait block ourselves. Another thread may have popped all waiters but not
        // unblocked them yet at this point:
        //
        // 1. This thread: calls PhfQueueWakeEvent.
        // 2. This thread: code determines that the wait should be cancelled.
        // 3. Other thread: calls PhfSetWakeEvent and pops our wait block off. It hasn't unblocked
        //    any wait blocks yet.
        // 4. This thread: calls PhfSetWakeEvent. Since all wait blocks have been popped, we don't
        //    do anything. The caller function exits, making our wait block invalid.
        // 5. Other thread: tries to unblock our wait block. Anything could happen, since our caller
        //    already returned.
        //
        // The solution is to (always) wait for an unblock. Note that the check below for the
        // spinning flag is not required, but it is a small optimization. If the wait block has been
        // unblocked (i.e. the spinning flag is cleared), then there's no danger.

        if (WaitBlock->Flags & PH_QUEUED_WAITER_SPINNING)
            PhpBlockOnQueuedWaitBlock(WaitBlock, FALSE, NULL);
    }
}

/**
 * Waits for a wake event to be set.
 *
 * \param WakeEvent A wake event.
 * \param WaitBlock A wait block previously queued to the wake event using PhfQueueWakeEvent().
 * \param Spin TRUE to spin on the wake event before blocking, FALSE to block immediately.
 * \param Timeout A timeout value.
 *
 * \remarks Wake events are subject to spurious wakeups. You should call this function in a loop
 * which checks a predicate.
 */
NTSTATUS FASTCALL PhfWaitForWakeEvent(
    _Inout_ PPH_WAKE_EVENT WakeEvent,
    _Inout_ PPH_QUEUED_WAIT_BLOCK WaitBlock,
    _In_ BOOLEAN Spin,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    NTSTATUS status;

    status = PhpBlockOnQueuedWaitBlock(WaitBlock, Spin, Timeout);

    if (status != STATUS_SUCCESS)
    {
        // Probably a timeout. There's no way of unlinking the wait block safely, so just wake
        // everyone.
        PhSetWakeEvent(WakeEvent, WaitBlock);
    }

    return status;
}
