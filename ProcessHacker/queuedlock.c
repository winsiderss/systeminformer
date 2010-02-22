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

#include <phbase.h>

VOID FASTCALL PhpfOptimizeQueuedLockList(
    __inout PPH_QUEUED_LOCK QueuedLock,
    __in ULONG_PTR Value
    );

VOID FASTCALL PhpfWakeQueuedLock(
    __inout PPH_QUEUED_LOCK QueuedLock,
    __in ULONG_PTR Value
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
        // Set the Traversing bit because we'll be optimizing the list.
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

VOID FASTCALL PhpfWakeQueuedLock(
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
        // If there are multiple shared owners, no one is going 
        // to wake waiters since the lock would still be owned. 
        // Also if there are multiple shared owners they may be 
        // traversing the list, and we shouldn't be doing so.
        assert(!(value & PH_QUEUED_LOCK_MULTIPLE_SHARED));

        // There's no point in waking a waiter if the lock 
        // is owned. Clear the traversing bit.
        while (value & PH_QUEUED_LOCK_OWNED)
        {
            newValue = value - PH_QUEUED_LOCK_TRAVERSING;

            if ((newValue = (ULONG_PTR)_InterlockedCompareExchangePointer(
                (PPVOID)&QueuedLock->Value,
                (PVOID)newValue,
                (PVOID)value
                )) == value)
                return;

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

    // Wake waiters.

    do
    {
        previousWaitBlock = waitBlock->Previous;
        PhpUnblockQueuedWaitBlock(waitBlock);
        waitBlock = previousWaitBlock;
    } while (waitBlock);
}

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
        // Use the fast path only if the lock isn't owned 
        // OR it is owned in shared mode and there are no waiters.
        // The second condition ensures we prioritize exclusive mode 
        // over shared mode.
        if (
            !(value & PH_QUEUED_LOCK_OWNED) ||
            (!(value & PH_QUEUED_LOCK_WAITERS) && (PhGetQueuedLockSharedOwners(value) > 0))
            )
        {
            if (!(value & PH_QUEUED_LOCK_OWNED))
                newValue = value + PH_QUEUED_LOCK_OWNED + PH_QUEUED_LOCK_SHARED_INC;
            else
                newValue = value + PH_QUEUED_LOCK_SHARED_INC;

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

        if ((ULONG)_InterlockedDecrement((PULONG)&waitBlock->SharedOwners) > 0)
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
