/*
 * Process Hacker - 
 *   queued lock
 * 
 * Copyright (C) 2010 wj32
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

/*
 * Queued lock, a.k.a. push lock (kernel-mode) or slim reader-writer lock 
 * (user-mode).
 *
 * The queued lock is:
 * * Around 10% faster than the fast lock.
 * * Only the size of a pointer.
 * * Low on resource usage (no additional kernel objects are 
 *   created for blocking).
 *
 * The usual flags are used for contention-free 
 * acquire/release. When there is contention, stack-based 
 * wait blocks are chained. The first wait block contains 
 * the shared owners count which is decremented by 
 * shared releasers.
 *
 * Naturally these wait blocks would be chained 
 * in FILO order, but list optimization is done for two purposes:
 * * Finding the last wait block (where the shared owners 
 *   count is stored). This is implemented by the Last pointer.
 * * Unblocking the wait blocks in FIFO order. This is 
 *   implemented by the Previous pointer.
 *
 * The optimization is incremental - each optimization run 
 * will stop at the first optimized wait block. Any needed 
 * optimization is completed just before waking waiters.
 *
 * The waiters list/chain has the following restrictions:
 * * At any time wait blocks may be pushed onto the list.
 * * While waking waiters, the list may not be traversed 
 *   nor optimized.
 * * When there are multiple shared owners, shared releasers 
 *   may traverse the list (to find the last wait block). 
 *   This is not an issue because waiters wouldn't be woken 
 *   until there are no more shared owners.
 * * List optimization may be done at any time except for 
 *   when someone else is waking waiters. This is controlled 
 *   by the traversing bit.
 *
 * The traversing bit has the following rules:
 * * The list may be optimized only after the traversing bit 
 *   is set, checking that it wasn't set already. 
 *   If it was set, it would indicate that someone else is 
 *   optimizing the list or waking waiters.
 * * Before waking waiters the traversing bit must be set. 
 *   If it was set already, just clear the owned bit.
 * * If during list optimization the owned bit is detected 
 *   to be cleared, the function begins waking waiters. This 
 *   is because the owned bit is cleared when a releaser 
 *   fails to set the traversing bit.
 *
 * Blocking is implemented through a process-wide keyed event. 
 * A spin count is also used before blocking on the keyed 
 * event.
 */

#include <phbase.h>

VOID FASTCALL PhpfOptimizeQueuedLockList(
    __inout PPH_QUEUED_LOCK QueuedLock,
    __in ULONG_PTR Value
    );

VOID FASTCALL PhpfWakeQueuedLock(
    __inout PPH_QUEUED_LOCK QueuedLock,
    __in ULONG_PTR Value
    );

VOID FASTCALL PhpfWakeQueuedLockEx(
    __inout PPH_QUEUED_LOCK QueuedLock,
    __in ULONG_PTR Value,
	__in BOOLEAN IgnoreOwned,
	__in BOOLEAN WakeAll
    );

HANDLE PhQueuedLockKeyedEventHandle;
ULONG PhQueuedLockSpinCount;

BOOLEAN PhQueuedLockInitialization()
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
 * \param Exclusive Whether the wait block is in exclusive 
 * mode.
 * \param WaitBlock A variable which receives the resulting 
 * wait block structure.
 * \param Optimize A variable which receives a boolean 
 * indicating whether to optimize the waiters list.
 * \param NewValue The old value of the queued lock. This 
 * value is useful only if the function returns FALSE.
 * \param CurrentValue The new value of the queued lock. This 
 * value is useful only if the function returns TRUE.
 *
 * \return TRUE if the wait block was pushed onto the waiters 
 * list, otherwise FALSE.
 *
 * \remarks
 * \li The function assumes the following flags are set: 
 * \ref PH_QUEUED_LOCK_OWNED.
 * \li Do not move the wait block location after this 
 * function is called.
 * \li The \a Optimize boolean is a hint to call 
 * PhpfOptimizeQueuedLockList() if the function succeeds. It is 
 * recommended, but not essential that this occurs.
 * \li Call PhpBlockOnQueuedWaitBlock() to wait for the wait 
 * block to be unblocked.
 */
FORCEINLINE BOOLEAN PhpPushQueuedWaitBlock(
    __inout PPH_QUEUED_LOCK QueuedLock,
    __in ULONG_PTR Value,
    __in BOOLEAN Exclusive,
    __out PPH_QUEUED_WAIT_BLOCK WaitBlock,
    __out PBOOLEAN Optimize,
    __out PULONG_PTR NewValue,
    __out PULONG_PTR CurrentValue
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
            // We're waiting in shared mode, which means there can't 
            // be any shared owners (otherwise we would've acquired 
            // the lock already).

            WaitBlock->SharedOwners = 0;

            newValue = ((ULONG_PTR)WaitBlock) | PH_QUEUED_LOCK_OWNED |
                PH_QUEUED_LOCK_WAITERS;
        }
    }

    *Optimize = optimize;
    *CurrentValue = newValue;

    newValue = (ULONG_PTR)_InterlockedCompareExchangePointer(
        (PPVOID)&QueuedLock->Value,
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
    __in ULONG_PTR Value
    )
{
    PPH_QUEUED_WAIT_BLOCK waitBlock;
    PPH_QUEUED_WAIT_BLOCK lastWaitBlock;

    waitBlock = PhGetQueuedLockWaitBlock(Value);

    // Traverse the list until we find the last wait block.
    // The Last pointer should be set by list optimization, 
    // allowing us to skip all, if not most of the wait blocks.

    while (TRUE)
    {
        lastWaitBlock = waitBlock->Last;

        if (lastWaitBlock)
        {
            // Follow the Last pointer. This can mean two 
            // things: the pointer was set by list optimization, 
            // or this wait block is actually the last wait block
            // (set when it was pushed onto the list).
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
 */
__mayRaise FORCEINLINE VOID PhpBlockOnQueuedWaitBlock(
    __inout PPH_QUEUED_WAIT_BLOCK WaitBlock
    )
{
    NTSTATUS status;
    ULONG i;

    for (i = 0; i < PhQueuedLockSpinCount; i++)
    {
        if (!((*(volatile ULONG *)&WaitBlock->Flags) & PH_QUEUED_WAITER_SPINNING))
        {
            break;
        }

        YieldProcessor();
    }

    if (_interlockedbittestandreset((PLONG)&WaitBlock->Flags, PH_QUEUED_WAITER_SPINNING_SHIFT))
    {
        if (!NT_SUCCESS(status = NtWaitForKeyedEvent(
            PhQueuedLockKeyedEventHandle,
            WaitBlock,
            FALSE,
            NULL
            )))
            PhRaiseStatus(status);
    }
}

/**
 * Unblocks a wait block.
 *
 * \param WaitBlock A wait block.
 */
__mayRaise FORCEINLINE VOID PhpUnblockQueuedWaitBlock(
    __inout PPH_QUEUED_WAIT_BLOCK WaitBlock
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
 *
 * \remarks The function assumes the following flags are set:
 * \ref PH_QUEUED_LOCK_WAITERS, \ref PH_QUEUED_LOCK_TRAVERSING.
 */
VOID FASTCALL PhpfOptimizeQueuedLockList(
    __inout PPH_QUEUED_LOCK QueuedLock,
    __in ULONG_PTR Value
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

        if (!(value & PH_QUEUED_LOCK_OWNED))
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
                // Save a pointer to the last wait block in 
                // the first wait block and stop optimizing.
                //
                // We don't need to continue setting Previous 
                // pointers because the last optimization run 
                // would have set them already.

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
            (PPVOID)&QueuedLock->Value,
            (PVOID)newValue,
            (PVOID)value
            )) == value)
            break;

        // Either someone pushed a wait block onto the list 
        // or someone released ownership. In either case we 
        // need to go back.

        value = newValue;
    }
}

/**
 * Dequeues the appropriate number of wait blocks in 
 * a queued lock.
 *
 * \param QueuedLock A queued lock.
 * \param Value The current value of the queued lock.
 * \param WakeAll TRUE to remove all wait blocks, FALSE 
 * to decide based on the wait block type.
 */
FORCEINLINE PPH_QUEUED_WAIT_BLOCK PhpPrepareToWakeQueuedLock(
	__inout PPH_QUEUED_LOCK QueuedLock,
	__in ULONG_PTR Value,
	__in BOOLEAN IgnoreOwned,
	__in BOOLEAN WakeAll
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
        // If there are multiple shared owners, no one is going 
        // to wake waiters since the lock would still be owned. 
        // Also if there are multiple shared owners they may be 
        // traversing the list. While that is safe when 
        // done concurrently with list optimization, we may be 
        // removing and waking waiters.
        assert(!(value & PH_QUEUED_LOCK_MULTIPLE_SHARED));
        assert(value & PH_QUEUED_LOCK_TRAVERSING);

        // There's no point in waking a waiter if the lock 
        // is owned. Clear the traversing bit.
        while (!IgnoreOwned && (value & PH_QUEUED_LOCK_OWNED))
        {
            newValue = value - PH_QUEUED_LOCK_TRAVERSING;

            if ((newValue = (ULONG_PTR)_InterlockedCompareExchangePointer(
                (PPVOID)&QueuedLock->Value,
                (PVOID)newValue,
                (PVOID)value
                )) == value)
                return NULL;

            value = newValue;
        }

        // Finish up any needed optimization (setting the 
        // Previous pointers) while finding the last wait 
        // block.

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

        // Unlink the relevant wait blocks and clear the 
        // traversing bit before we wake waiters.

        if (
			!WaitAll &&
            (waitBlock->Flags & PH_QUEUED_WAITER_EXCLUSIVE) &&
            (previousWaitBlock = waitBlock->Previous)
            )
        {
            // We have an exclusive waiter and there are 
            // multiple waiters.
            // We'll only be waking this waiter.

            // Unlink the wait block from the list. 
            // Although other wait blocks may have their 
            // Last pointers set to this wait block, 
            // the algorithm to find the last wait block 
            // will stop here. Likewise the Next pointers 
            // are never followed beyond this point, so 
            // we don't need to clear those.
            firstWaitBlock->Last = previousWaitBlock;

            // Make sure we only wake this waiter.
            waitBlock->Previous = NULL;

            // Clear the traversing bit.
#ifdef _M_IX86
            _InterlockedExchangeAdd((PLONG)&QueuedLock->Value, -(LONG)PH_QUEUED_LOCK_TRAVERSING);
#else
            _InterlockedExchangeAdd64((PLONG64)&QueuedLock->Value, -(LONG64)PH_QUEUED_LOCK_TRAVERSING);
#endif

            break;
        }
        else
        {
            // We're waking an exclusive waiter and there 
            // is only one waiter, or we are waking 
            // a shared waiter and possibly others.

            newValue = 0;

            if ((newValue = (ULONG_PTR)_InterlockedCompareExchangePointer(
                (PPVOID)&QueuedLock->Value,
                (PVOID)newValue,
                (PVOID)value
                )) == value)
                break;

            // Someone changed the lock (acquired it or 
            // pushed a wait block).

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
 * \ref PH_QUEUED_LOCK_WAITERS, \ref PH_QUEUED_LOCK_TRAVERSING.
 * The function assumes the following flags are not set:
 * \ref PH_QUEUED_LOCK_MULTIPLE_SHARED.
 */
VOID FASTCALL PhpfWakeQueuedLock(
    __inout PPH_QUEUED_LOCK QueuedLock,
    __in ULONG_PTR Value
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
 * \param WakeAll TRUE to wake all waiters, FALSE to 
 * decide based on the wait block type.
 *
 * \remarks The function assumes the following flags are set:
 * \ref PH_QUEUED_LOCK_WAITERS, \ref PH_QUEUED_LOCK_TRAVERSING.
 * The function assumes the following flags are not set:
 * \ref PH_QUEUED_LOCK_MULTIPLE_SHARED.
 */
VOID FASTCALL PhpfWakeQueuedLockEx(
    __inout PPH_QUEUED_LOCK QueuedLock,
    __in ULONG_PTR Value,
	__in BOOLEAN IgnoreOwned,
	__in BOOLEAN WakeAll
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
    __inout PPH_QUEUED_LOCK QueuedLock
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
                (PPVOID)&QueuedLock->Value,
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

                PhpBlockOnQueuedWaitBlock(&waitBlock);
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
    __inout PPH_QUEUED_LOCK QueuedLock
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
        // We want to prioritize exclusive acquires over shared acquires.
        // There's currently no fast, safe way of finding the last wait 
        // block and incrementing the shared owners count here.
        if (
            !(value & PH_QUEUED_LOCK_WAITERS) &&
            (!(value & PH_QUEUED_LOCK_OWNED) || (PhGetQueuedLockSharedOwners(value) > 0))
            )
        {
            newValue = (value + PH_QUEUED_LOCK_SHARED_INC) | PH_QUEUED_LOCK_OWNED;

            if ((newValue = (ULONG_PTR)_InterlockedCompareExchangePointer(
                (PPVOID)&QueuedLock->Value,
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

                PhpBlockOnQueuedWaitBlock(&waitBlock);
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
    __inout PPH_QUEUED_LOCK QueuedLock
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

        if (!(value & PH_QUEUED_LOCK_WAITERS) || (value & PH_QUEUED_LOCK_TRAVERSING))
        {
            // There are no waiters or someone is traversing the list.
            //
            // If there are no waiters, we're simply releasing ownership.
            // If someone is traversing the list, clearing the owned bit 
            // is a signal for them to wake waiters.

            newValue = value - PH_QUEUED_LOCK_OWNED;

            if ((newValue = (ULONG_PTR)_InterlockedCompareExchangePointer(
                (PPVOID)&QueuedLock->Value,
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
                (PPVOID)&QueuedLock->Value,
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
 * Releases a queued lock in shared mode.
 *
 * \param QueuedLock A queued lock.
 */
VOID FASTCALL PhfReleaseQueuedLockShared(
    __inout PPH_QUEUED_LOCK QueuedLock
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
            (PPVOID)&QueuedLock->Value,
            (PVOID)newValue,
            (PVOID)value
            )) == value)
            return;

        value = newValue;
    }

    if (value & PH_QUEUED_LOCK_MULTIPLE_SHARED)
    {
        // Unfortunately we have to find the last wait block and 
        // decrement the shared owners count.
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
                (PPVOID)&QueuedLock->Value,
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
                (PPVOID)&QueuedLock->Value,
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
 * Wakes waiters in a queued lock, making no assumptions 
 * about the state of the lock.
 *
 * \param QueuedLock A queued lock.
 */
VOID FASTCALL PhfTryWakePushLock(
    __inout PPH_QUEUED_LOCK QueuedLock
    )
{
    ULONG_PTR value;
    ULONG_PTR newValue;

    value = QueuedLock->Value;

    if (
        !(value & PH_QUEUED_LOCK_WAITERS) ||
        (value & PH_QUEUED_LOCK_TRAVERSING) ||
        (value & PH_QUEUED_LOCK_OWNED)
        )
        return;

    newValue = value + PH_QUEUED_LOCK_TRAVERSING;

    if ((ULONG_PTR)_InterlockedCompareExchangePointer(
        (PPVOID)&QueuedLock->Value,
        (PVOID)newValue,
        (PVOID)value
        ) == value)
    {
        PhpfWakeQueuedLock(QueuedLock, newValue);
    }
}

/**
 * Wakes one thread sleeping on a condition variable.
 *
 * \param Condition A condition variable.
 */
VOID FASTCALL PhfPulseCondition(
	__inout PPH_QUEUED_LOCK Condition
	)
{
	PhpfWakeQueuedLockEx(Condition, Condition->Value, TRUE, FALSE);
}

/**
 * Wakes all threads sleeping on a condition variable.
 *
 * \param Condition A condition variable.
 */
VOID FASTCALL PhfPulseAllCondition(
	__inout PPH_QUEUED_LOCK Condition
	)
{
	PhpfWakeQueuedLockEx(Condition, Condition->Value, TRUE, TRUE);
}
	
/**
 * Sleeps on a condition variable.
 *
 * \param Condition A condition variable.
 * \param Lock A queued lock to release/acquire.
 * \param Timeout Not implemented.
 */
VOID FASTCALL PhfWaitForCondition(
	__inout PPH_QUEUED_LOCK Condition,
	__inout_opt PPH_QUEUED_LOCK Lock,
	__in_opt PLARGE_INTEGER Timeout
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
				PhpfOptimizeQueuedLockList(Condition, currentValue);

			if (Lock)
			{
				PhReleaseQueuedLockExclusiveFast(Lock);
			}

			PhpBlockOnQueuedWaitBlock(&waitBlock);

			if (Lock)
			{
				// Don't use the fast variant; it is extremely likely 
				// that the lock is still owned.
				PhAcquireQueuedLockExclusive(Lock);
			}

			break;
		}
	}
}
