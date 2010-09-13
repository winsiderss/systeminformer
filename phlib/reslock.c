/*
 * Process Hacker - 
 *   resource lock
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
#include <phsync.h>

#define PH_RESOURCE_WAITER_FIRST 1
#define PH_RESOURCE_WAITER_LAST_EXCLUSIVE 2
#define PH_RESOURCE_WAITER_LAST 3

VOID FASTCALL PhfInitializeResourceLock(
    __out PPH_RESOURCE_LOCK Lock
    )
{
    Lock->Value = 0;
    Lock->SpinCount = (ULONG)PhSystemBasicInformation.NumberOfProcessors > 1 ? 4000 : 0;
    PhInitializeQueuedLock(&Lock->WakeEvent);

    PhInitializeQueuedLock(&Lock->WaiterListLock);
    InitializeListHead(&Lock->WaiterListHead);
    Lock->FirstSharedWaiter = &Lock->WaiterListHead;
}

FORCEINLINE VOID PhpInsertResourceWaiter(
    __inout PPH_RESOURCE_LOCK Lock,
    __inout PPH_RESOURCE_WAITER Waiter,
    __in ULONG Position
    )
{
    switch (Position)
    {
    case PH_RESOURCE_WAITER_FIRST:
        InsertHeadList(&Lock->WaiterListHead, &Waiter->ListEntry);
        break;
    case PH_RESOURCE_WAITER_LAST_EXCLUSIVE:
        InsertTailList(Lock->FirstSharedWaiter, &Waiter->ListEntry);
        break;
    case PH_RESOURCE_WAITER_LAST:
        InsertTailList(&Lock->WaiterListHead, &Waiter->ListEntry);
        break;
    default:
        ASSUME_NO_DEFAULT;
    }
}

VOID FASTCALL PhpfBlockOnResourceWaiter(
    __inout PPH_RESOURCE_LOCK Lock,
    __inout PPH_RESOURCE_WAITER Waiter
    )
{
    PH_QUEUED_WAIT_BLOCK waitBlock;
    ULONG i;

    // Spin for a while.
    for (i = Lock->SpinCount; i != 0; i--)
    {
        if (*(volatile ULONG *)&Waiter->Flags & PH_RESOURCE_WAITER_SIGNALED)
            return;

        YieldProcessor();
    }

    do
    {
        PhQueueWakeEvent(&Lock->WakeEvent, &waitBlock);

        if (*(volatile ULONG *)&Waiter->Flags & PH_RESOURCE_WAITER_SIGNALED)
        {
            PhSetWakeEvent(&Lock->WakeEvent, &waitBlock);
            break;
        }

        PhWaitForWakeEvent(&Lock->WakeEvent, &waitBlock, FALSE, NULL);
    } while (!(*(volatile ULONG *)&Waiter->Flags & PH_RESOURCE_WAITER_SIGNALED));
}

FORCEINLINE VOID PhpUnblockResourceWaiter(
    __inout PPH_RESOURCE_WAITER Waiter
    )
{
    _interlockedbittestandset((PLONG)&Waiter->Flags, PH_RESOURCE_WAITER_SIGNALED_SHIFT);
}

VOID FASTCALL PhpfWakeResourceLock(
    __inout PPH_RESOURCE_LOCK Lock
    )
{
    LIST_ENTRY wakeListHead;
    PLIST_ENTRY listEntry;
    PPH_RESOURCE_WAITER waiter;
    PPH_RESOURCE_WAITER exclusiveWaiter;
    BOOLEAN first;
    PLIST_ENTRY flink;
    BOOLEAN wake;

    InitializeListHead(&wakeListHead);
    exclusiveWaiter = NULL;
    first = TRUE;

    PhAcquireQueuedLockExclusive(&Lock->WaiterListLock);

    while (TRUE)
    {
        listEntry = Lock->WaiterListHead.Flink;

        if (listEntry == &Lock->WaiterListHead)
        {
            // No more waiters. Clear the waiters bit.
            _interlockedbittestandreset((PLONG)&Lock->Value, PH_RESOURCE_LOCK_WAITERS_SHIFT);
            break;
        }

        waiter = CONTAINING_RECORD(listEntry, PH_RESOURCE_WAITER, ListEntry);

        // If this is an exclusive waiter, don't wake anyone else.
        if (first && (waiter->Flags & PH_RESOURCE_WAITER_EXCLUSIVE))
        {
            RemoveEntryList(&waiter->ListEntry);
            exclusiveWaiter = waiter;
            break;
        }

        // Remove the (shared) waiter and add it to the wake list.
        RemoveEntryList(&waiter->ListEntry);
        InsertTailList(&wakeListHead, &waiter->ListEntry);

        first = FALSE;
    }

    // If we removed shared waiters, we removed all of them. 
    // Reset the first shared waiter pointer.
    // Note that this also applies if we haven't woken anyone 
    // at all; this just becomes a redundant assignment.
    if (!exclusiveWaiter)
    {
        Lock->FirstSharedWaiter = &Lock->WaiterListHead;
    }

    PhReleaseQueuedLockExclusive(&Lock->WaiterListLock);

    wake = FALSE;

    if (exclusiveWaiter)
    {
        // If we removed one exclusive waiter, unblock it.
        PhpUnblockResourceWaiter(exclusiveWaiter);
        wake = TRUE;
    }
    else
    {
        // Carefully traverse the wake list and wake each shared waiter.

        listEntry = wakeListHead.Flink;

        while (listEntry != &wakeListHead)
        {
            flink = listEntry->Flink;
            waiter = CONTAINING_RECORD(listEntry, PH_RESOURCE_WAITER, ListEntry);
            PhpUnblockResourceWaiter(waiter);
            listEntry = flink;

            wake = TRUE;
        }
    }

    if (wake)
        PhSetWakeEvent(&Lock->WakeEvent, NULL);
}

VOID FASTCALL PhpfWakeResourceLockExclusive(
    __inout PPH_RESOURCE_LOCK Lock
    )
{
    PLIST_ENTRY listEntry;
    PPH_RESOURCE_WAITER waiter;
    PPH_RESOURCE_WAITER exclusiveWaiter;

    exclusiveWaiter = NULL;

    PhAcquireQueuedLockExclusive(&Lock->WaiterListLock);

    listEntry = Lock->WaiterListHead.Flink;

    if (listEntry != &Lock->WaiterListHead)
    {
        waiter = CONTAINING_RECORD(listEntry, PH_RESOURCE_WAITER, ListEntry);

        if (waiter->Flags & PH_RESOURCE_WAITER_EXCLUSIVE)
        {
            RemoveEntryList(&waiter->ListEntry);
            exclusiveWaiter = waiter;

            listEntry = Lock->WaiterListHead.Flink;
        }
    }

    if (listEntry == &Lock->WaiterListHead)
    {
        // No more waiters. Clear the waiters bit.
        _interlockedbittestandreset((PLONG)&Lock->Value, PH_RESOURCE_LOCK_WAITERS_SHIFT);
    }

    PhReleaseQueuedLockExclusive(&Lock->WaiterListLock);

    if (exclusiveWaiter)
    {
        PhpUnblockResourceWaiter(exclusiveWaiter);
        PhSetWakeEvent(&Lock->WakeEvent, NULL);
    }
}

VOID FASTCALL PhpfWakeResourceLockShared(
    __inout PPH_RESOURCE_LOCK Lock
    )
{
    LIST_ENTRY wakeListHead;
    PLIST_ENTRY listEntry;
    PPH_RESOURCE_WAITER waiter;
    PLIST_ENTRY flink;

    InitializeListHead(&wakeListHead);

    PhAcquireQueuedLockExclusive(&Lock->WaiterListLock);

    listEntry = Lock->FirstSharedWaiter;

    while (TRUE)
    {
        if (listEntry == &Lock->WaiterListHead)
        {
            // No more waiters. Clear the waiters bit.
            _interlockedbittestandreset((PLONG)&Lock->Value, PH_RESOURCE_LOCK_WAITERS_SHIFT);
            break;
        }

        waiter = CONTAINING_RECORD(listEntry, PH_RESOURCE_WAITER, ListEntry);
        flink = listEntry->Flink;

        // Remove the shared waiter and add it to the wake list.
        RemoveEntryList(&waiter->ListEntry);
        InsertTailList(&wakeListHead, &waiter->ListEntry);

        listEntry = flink;
    }

    // Reset the first shared waiter pointer.
    Lock->FirstSharedWaiter = &Lock->WaiterListHead;

    PhReleaseQueuedLockExclusive(&Lock->WaiterListLock);

    if (!IsListEmpty(&wakeListHead))
    {
        // Carefully traverse the wake list and wake each shared waiter.

        listEntry = wakeListHead.Flink;

        while (listEntry != &wakeListHead)
        {
            flink = listEntry->Flink;
            waiter = CONTAINING_RECORD(listEntry, PH_RESOURCE_WAITER, ListEntry);
            PhpUnblockResourceWaiter(waiter);
            listEntry = flink;
        }

        PhSetWakeEvent(&Lock->WakeEvent, NULL);
    }
}

FORCEINLINE BOOLEAN PhpPrepareToInsertResourceWaiter(
    __inout PPH_RESOURCE_LOCK Lock,
    __in ULONG Value
    )
{
    PhAcquireQueuedLockExclusive(&Lock->WaiterListLock);

    // Try to set the waiters bit.
    if (_InterlockedCompareExchange(
        &Lock->Value,
        Value | PH_RESOURCE_LOCK_WAITERS,
        Value
        ) != Value)
    {
        // Unfortunately we have to go back. This is 
        // very wasteful since the waiter list lock 
        // must be released again, but must happen since 
        // the lock may have been released.
        PhReleaseQueuedLockExclusive(&Lock->WaiterListLock);
        return FALSE;
    }

    return TRUE;
}

VOID FASTCALL PhfAcquireResourceLockExclusive(
    __inout PPH_RESOURCE_LOCK Lock
    )
{
    ULONG value;
    ULONG i;
    ULONG spinCount;

    i = 0;
    spinCount = Lock->SpinCount;

    while (TRUE)
    {
        value = Lock->Value;

        if (!(value & PH_RESOURCE_LOCK_OWNED))
        {
            if (_InterlockedCompareExchange(
                &Lock->Value,
                value + PH_RESOURCE_LOCK_OWNED,
                value
                ) == value)
                break;
        }
        else if (i >= spinCount)
        {
            PH_RESOURCE_WAITER waiter;

            waiter.Flags = PH_RESOURCE_WAITER_EXCLUSIVE;

            if (PhpPrepareToInsertResourceWaiter(Lock, value))
            {
                // Put our wait block behind other exclusive waiters but 
                // in front of all shared waiters.
                PhpInsertResourceWaiter(Lock, &waiter, PH_RESOURCE_WAITER_LAST_EXCLUSIVE);
                PhReleaseQueuedLockExclusive(&Lock->WaiterListLock);

                PhpfBlockOnResourceWaiter(Lock, &waiter);
            }

            continue;
        }

        i++;
    }
}

VOID FASTCALL PhfAcquireResourceLockShared(
    __inout PPH_RESOURCE_LOCK Lock
    )
{
    ULONG value;
    ULONG i;
    ULONG spinCount;

    i = 0;
    spinCount = Lock->SpinCount;

    while (TRUE)
    {
        value = Lock->Value;

        // Note that we don't acquire if there are waiters and 
        // the lock is already owned in shared mode, in order to 
        // give exclusive acquires precedence.
        if (
            !(value & PH_RESOURCE_LOCK_OWNED) ||
            (!(value & PH_RESOURCE_LOCK_WAITERS) && ((value >> PH_RESOURCE_LOCK_SHARED_SHIFT) & PH_RESOURCE_LOCK_SHARED_MASK) != 0)
            )
        {
            if (_InterlockedCompareExchange(
                &Lock->Value,
                (value | PH_RESOURCE_LOCK_OWNED) + PH_RESOURCE_LOCK_SHARED_INC,
                value
                ) == value)
                break;
        }
        else if (i >= spinCount)
        {
            PH_RESOURCE_WAITER waiter;

            waiter.Flags = 0;

            if (PhpPrepareToInsertResourceWaiter(Lock, value))
            {
                // Put our wait block behind other waiters.
                PhpInsertResourceWaiter(Lock, &waiter, PH_RESOURCE_WAITER_LAST);

                // Set the first shared waiter pointer.
                if (Lock->FirstSharedWaiter == &Lock->WaiterListHead)
                {
                    Lock->FirstSharedWaiter = &waiter.ListEntry;
                }

                PhReleaseQueuedLockExclusive(&Lock->WaiterListLock);

                PhpfBlockOnResourceWaiter(Lock, &waiter);
            }

            continue;
        }

        i++;
    }
}

VOID FASTCALL PhfReleaseResourceLockExclusive(
    __inout PPH_RESOURCE_LOCK Lock
    )
{
    ULONG value;

    while (TRUE)
    {
        value = Lock->Value;

        if (_InterlockedCompareExchange(
            &Lock->Value,
            value - PH_RESOURCE_LOCK_OWNED,
            value
            ) == value)
        {
            if (value & PH_RESOURCE_LOCK_WAITERS)
                PhpfWakeResourceLock(Lock);

            break;
        }
    }
}

VOID FASTCALL PhfReleaseResourceLockShared(
    __inout PPH_RESOURCE_LOCK Lock
    )
{
    ULONG value;
    ULONG newValue;
    ULONG sharedOwners;

    while (TRUE)
    {
        value = Lock->Value;
        sharedOwners = (value >> PH_RESOURCE_LOCK_SHARED_SHIFT) & PH_RESOURCE_LOCK_SHARED_MASK;

        if (sharedOwners > 1)
            newValue = value - PH_RESOURCE_LOCK_SHARED_INC;
        else
            newValue = value - PH_RESOURCE_LOCK_OWNED - PH_RESOURCE_LOCK_SHARED_INC;

        if (_InterlockedCompareExchange(
            &Lock->Value,
            newValue,
            value
            ) == value)
        {
            // Only wake if we are the last out.
            if ((value & PH_RESOURCE_LOCK_WAITERS) && sharedOwners == 1)
                PhpfWakeResourceLockExclusive(Lock);

            break;
        }
    }
}

VOID FASTCALL PhfConvertResourceLockExclusiveToShared(
    __inout PPH_RESOURCE_LOCK Lock
    )
{
    ULONG value;

    while (TRUE)
    {
        value = Lock->Value;

        if (_InterlockedCompareExchange(
            &Lock->Value,
            value + PH_RESOURCE_LOCK_SHARED_INC,
            value
            ) == value)
        {
            if (value & PH_RESOURCE_LOCK_WAITERS)
                PhpfWakeResourceLockShared(Lock);

            break;
        }
    }
}

VOID FASTCALL PhfConvertResourceLockSharedToExclusive(
    __inout PPH_RESOURCE_LOCK Lock
    )
{
    ULONG value;
    ULONG i;
    ULONG spinCount;

    i = 0;
    spinCount = Lock->SpinCount;

    while (TRUE)
    {
        value = Lock->Value;

        // Are we the only shared waiter? If so, acquire in exclusive mode, 
        // otherwise wait.
        if (((value >> PH_RESOURCE_LOCK_SHARED_SHIFT) & PH_RESOURCE_LOCK_SHARED_MASK) == 1)
        {
            if (_InterlockedCompareExchange(
                &Lock->Value,
                value - PH_RESOURCE_LOCK_SHARED_INC,
                value
                ) == value)
                break;
        }
        else if (i >= spinCount)
        {
            PH_RESOURCE_WAITER waiter;

            waiter.Flags = PH_RESOURCE_WAITER_EXCLUSIVE;

            if (PhpPrepareToInsertResourceWaiter(Lock, value))
            {
                // Put our wait block ahead of all other waiters.
                PhpInsertResourceWaiter(Lock, &waiter, PH_RESOURCE_WAITER_FIRST);
                PhReleaseQueuedLockExclusive(&Lock->WaiterListLock);

                PhpfBlockOnResourceWaiter(Lock, &waiter);
            }

            continue;
        }

        i++;
    }
}
