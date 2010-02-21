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

VOID FASTCALL PhpfOptimizeQueuedLockList(
    __inout PPH_QUEUED_LOCK QueuedLock,
    __in ULONG_PTR Value
    )
{
    
}

FORCEINLINE BOOLEAN PhpPushQueuedWaitBlock(
    __inout PPH_QUEUED_LOCK QueuedLock,
    __in ULONG_PTR Value,
    __in BOOLEAN Exclusive,
    __out PPH_QUEUED_WAIT_BLOCK WaitBlock,
    __out PBOOLEAN Optimize
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
        WaitBlock->Last = WaitBlock;

        if (Exclusive)
        {
            // We're the first waiter. Save the shared owners count.
            WaitBlock->SharedOwners = PhGetQueuedLockSharedOwners(Value);

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
            // be any shared owners.
            WaitBlock->SharedOwners = 0;

            newValue = ((ULONG_PTR)WaitBlock) | PH_QUEUED_LOCK_OWNED |
                PH_QUEUED_LOCK_WAITERS;
        }
    }

    return (ULONG_PTR)_InterlockedCompareExchangePointer(
        (PPVOID)&QueuedLock->Value,
        (PVOID)newValue,
        (PVOID)Value
        ) == Value;
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

VOID FASTCALL PhfAcquireQueuedLockExclusive(
    __inout PPH_QUEUED_LOCK QueuedLock
    )
{
    ULONG_PTR value;
    ULONG_PTR newValue;
    BOOLEAN optimize;
    PH_QUEUED_WAIT_BLOCK waitBlock;

    while (TRUE)
    {
        value = QueuedLock->Value;

        if (!(value & PH_QUEUED_LOCK_OWNED))
        {
            if ((ULONG_PTR)_InterlockedCompareExchangePointer(
                (PPVOID)&QueuedLock->Value,
                (PVOID)(value + PH_QUEUED_LOCK_OWNED),
                (PVOID)value
                ) == value)
                break;
        }
        else
        {
            if (PhpPushQueuedWaitBlock(
                QueuedLock,
                value,
                TRUE,
                &waitBlock,
                &optimize
                ))
            {
                if (optimize)
                    PhpfOptimizeQueuedLockList(QueuedLock, newValue);

                PhpBlockOnQueuedWaitBlock(&waitBlock);
            }
        }
    }
}

VOID FASTCALL PhfAcquireQueuedLockShared(
    __inout PPH_QUEUED_LOCK QueuedLock
    )
{
    ULONG_PTR value;
    ULONG_PTR newValue;
    BOOLEAN optimize;
    PH_QUEUED_WAIT_BLOCK waitBlock;

    while (TRUE)
    {
        value = QueuedLock->Value;

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

            if ((ULONG_PTR)_InterlockedCompareExchangePointer(
                (PPVOID)&QueuedLock->Value,
                (PVOID)newValue,
                (PVOID)value
                ) == value)
                break;
        }
        else
        {
            if (PhpPushQueuedWaitBlock(
                QueuedLock,
                value,
                FALSE,
                &waitBlock,
                &optimize
                ))
            {
                if (optimize)
                    PhpfOptimizeQueuedLockList(QueuedLock, newValue);

                PhpBlockOnQueuedWaitBlock(&waitBlock);
            }
        }
    }
}

VOID FASTCALL PhfReleaseQueuedLockExclusive(
    __inout PPH_QUEUED_LOCK QueuedLock
    )
{

}

VOID FASTCALL PhfReleaseQueuedLockShared(
    __inout PPH_QUEUED_LOCK QueuedLock
    )
{

}
