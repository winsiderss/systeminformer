/*
 * Process Hacker - 
 *   fast resource lock
 * 
 * Copyright (C) 2009-2010 wj32
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

// FastLock is a port of FastResourceLock from PH 1.x.
//
// The code contains no comments because it is a direct 
// port. Please see FastResourceLock.cs in PH 1.x for 
// details.

// The fast lock is around 7% faster than the critical 
// section when there is no contention, when used 
// solely for mutual exclusion. It is also much smaller 
// than the critical section.

#define PH_LOCK_OWNED 0x1
#define PH_LOCK_EXCLUSIVE_WAKING 0x2

#define PH_LOCK_SHARED_OWNERS_SHIFT 2
#define PH_LOCK_SHARED_OWNERS_MASK 0x3ff
#define PH_LOCK_SHARED_OWNERS_INC 0x4

#define PH_LOCK_SHARED_WAITERS_SHIFT 12
#define PH_LOCK_SHARED_WAITERS_MASK 0x3ff
#define PH_LOCK_SHARED_WAITERS_INC 0x1000

#define PH_LOCK_EXCLUSIVE_WAITERS_SHIFT 22
#define PH_LOCK_EXCLUSIVE_WAITERS_MASK 0x3ff
#define PH_LOCK_EXCLUSIVE_WAITERS_INC 0x400000

#define PH_LOCK_EXCLUSIVE_MASK \
    (PH_LOCK_EXCLUSIVE_WAKING | \
    (PH_LOCK_EXCLUSIVE_WAITERS_MASK << PH_LOCK_EXCLUSIVE_WAITERS_SHIFT))

static ULONG PhFastLockSpinCount = 2000;

VOID PhFastLockInitialization(
    VOID
    )
{
    if ((ULONG)PhSystemBasicInformation.NumberOfProcessors > 1)
        PhFastLockSpinCount = 4000;
    else
        PhFastLockSpinCount = 0;
}

VOID PhInitializeFastLock(
    __out PPH_FAST_LOCK FastLock
    )
{
    FastLock->Value = 0;
    FastLock->ExclusiveWakeEvent = NULL;
    FastLock->SharedWakeEvent = NULL;
}

VOID PhDeleteFastLock(
    __inout PPH_FAST_LOCK FastLock
    )
{
    if (FastLock->ExclusiveWakeEvent)
    {
        NtClose(FastLock->ExclusiveWakeEvent);
        FastLock->ExclusiveWakeEvent = NULL;
    }

    if (FastLock->SharedWakeEvent)
    {
        NtClose(FastLock->SharedWakeEvent);
        FastLock->SharedWakeEvent = NULL;
    }
}

FORCEINLINE VOID PhpEnsureEventCreated(
    __inout PHANDLE Handle
    )
{
    HANDLE handle;

    if (*Handle != NULL)
        return;

    NtCreateSemaphore(&handle, SEMAPHORE_ALL_ACCESS, NULL, 0, MAXLONG);

    if (_InterlockedCompareExchangePointer(
        Handle,
        handle,
        NULL
        ) != NULL)
    {
        NtClose(handle);
    }
}

__mayRaise VOID FASTCALL PhfAcquireFastLockExclusive(
    __inout PPH_FAST_LOCK FastLock
    )
{
    ULONG value;
    ULONG i = 0;

    while (TRUE)
    {
        value = FastLock->Value;

        if (!(value & (PH_LOCK_OWNED | PH_LOCK_EXCLUSIVE_WAKING)))
        {
            if (_InterlockedCompareExchange(
                &FastLock->Value,
                value + PH_LOCK_OWNED,
                value
                ) == value)
                break;
        }
        else if (i >= PhFastLockSpinCount)
        {
            PhpEnsureEventCreated(&FastLock->ExclusiveWakeEvent);

            if (_InterlockedCompareExchange(
                &FastLock->Value,
                value + PH_LOCK_EXCLUSIVE_WAITERS_INC,
                value
                ) == value)
            {
                if (NtWaitForSingleObject(
                    FastLock->ExclusiveWakeEvent,
                    FALSE,
                    NULL
                    ) != STATUS_WAIT_0)
                    PhRaiseStatus(STATUS_UNSUCCESSFUL);

                do
                {
                    value = FastLock->Value;
                } while (_InterlockedCompareExchange(
                    &FastLock->Value,
                    value + PH_LOCK_OWNED - PH_LOCK_EXCLUSIVE_WAKING,
                    value
                    ) != value);

                break;
            }
        }

        i++;
        YieldProcessor();
    }
}

__mayRaise VOID FASTCALL PhfAcquireFastLockShared(
    __inout PPH_FAST_LOCK FastLock
    )
{
    ULONG value;
    ULONG i = 0;

    while (TRUE)
    {
        value = FastLock->Value;

        if (!(value & (
            PH_LOCK_OWNED |
            (PH_LOCK_SHARED_OWNERS_MASK << PH_LOCK_SHARED_OWNERS_SHIFT) |
            PH_LOCK_EXCLUSIVE_MASK
            )))
        {
            if (_InterlockedCompareExchange(
                &FastLock->Value,
                value + PH_LOCK_OWNED + PH_LOCK_SHARED_OWNERS_INC,
                value
                ) == value)
                break;
        }
        else if (
            (value & PH_LOCK_OWNED) &&
            ((value >> PH_LOCK_SHARED_OWNERS_SHIFT) & PH_LOCK_SHARED_OWNERS_MASK) > 0 &&
            !(value & PH_LOCK_EXCLUSIVE_MASK)
            )
        {
            if (_InterlockedCompareExchange(
                &FastLock->Value,
                value + PH_LOCK_SHARED_OWNERS_INC,
                value
                ) == value)
                break;
        }
        else if (i >= PhFastLockSpinCount)
        {
            PhpEnsureEventCreated(&FastLock->SharedWakeEvent);

            if (_InterlockedCompareExchange(
                &FastLock->Value,
                value + PH_LOCK_SHARED_WAITERS_INC,
                value
                ) == value)
            {
                if (NtWaitForSingleObject(
                    FastLock->SharedWakeEvent,
                    FALSE,
                    NULL
                    ) != STATUS_WAIT_0)
                    PhRaiseStatus(STATUS_UNSUCCESSFUL);

                continue;
            }
        }

        i++;
        YieldProcessor();
    }
}

VOID FASTCALL PhfReleaseFastLockExclusive(
    __inout PPH_FAST_LOCK FastLock
    )
{
    ULONG value;

    while (TRUE)
    {
        value = FastLock->Value;

        if ((value >> PH_LOCK_EXCLUSIVE_WAITERS_SHIFT) & PH_LOCK_EXCLUSIVE_WAITERS_MASK)
        {
            if (_InterlockedCompareExchange(
                &FastLock->Value,
                value - PH_LOCK_OWNED + PH_LOCK_EXCLUSIVE_WAKING - PH_LOCK_EXCLUSIVE_WAITERS_INC,
                value
                ) == value)
            {
                NtReleaseSemaphore(FastLock->ExclusiveWakeEvent, 1, NULL);

                break;
            }
        }
        else
        {
            ULONG sharedWaiters;

            sharedWaiters = (value >> PH_LOCK_SHARED_WAITERS_SHIFT) & PH_LOCK_SHARED_WAITERS_MASK;

            if (_InterlockedCompareExchange(
                &FastLock->Value,
                value & ~(PH_LOCK_OWNED | (PH_LOCK_SHARED_WAITERS_MASK << PH_LOCK_SHARED_WAITERS_SHIFT)),
                value
                ) == value)
            {
                if (sharedWaiters)
                    NtReleaseSemaphore(FastLock->SharedWakeEvent, sharedWaiters, 0);

                break;
            }
        }

        YieldProcessor();
    }
}

VOID FASTCALL PhfReleaseFastLockShared(
    __inout PPH_FAST_LOCK FastLock
    )
{
    ULONG value;

    while (TRUE)
    {
        value = FastLock->Value;

        if (((value >> PH_LOCK_SHARED_OWNERS_SHIFT) & PH_LOCK_SHARED_OWNERS_MASK) > 1)
        {
            if (_InterlockedCompareExchange(
                &FastLock->Value,
                value - PH_LOCK_SHARED_OWNERS_INC,
                value
                ) == value)
                break;
        }
        else if ((value >> PH_LOCK_EXCLUSIVE_WAITERS_SHIFT) & PH_LOCK_EXCLUSIVE_WAITERS_MASK)
        {
            if (_InterlockedCompareExchange(
                &FastLock->Value,
                value - PH_LOCK_OWNED + PH_LOCK_EXCLUSIVE_WAKING -
                PH_LOCK_SHARED_OWNERS_INC - PH_LOCK_EXCLUSIVE_WAITERS_INC,
                value
                ) == value)
            {
                NtReleaseSemaphore(FastLock->ExclusiveWakeEvent, 1, NULL);

                break;
            }
        }
        else
        {
            if (_InterlockedCompareExchange(
                &FastLock->Value,
                value - PH_LOCK_OWNED - PH_LOCK_SHARED_OWNERS_INC,
                value
                ) == value)
                break;
        }

        YieldProcessor();
    }
}

BOOLEAN FASTCALL PhfTryAcquireFastLockExclusive(
    __inout PPH_FAST_LOCK FastLock
    )
{
    ULONG value;

    value = FastLock->Value;

    if (value & (PH_LOCK_OWNED | PH_LOCK_EXCLUSIVE_WAKING))
        return FALSE;

    return _InterlockedCompareExchange(
        &FastLock->Value,
        value + PH_LOCK_OWNED,
        value
        ) == value;
}

BOOLEAN FASTCALL PhfTryAcquireFastLockShared(
    __inout PPH_FAST_LOCK FastLock
    )
{
    ULONG value;

    value = FastLock->Value;

    if (value & PH_LOCK_EXCLUSIVE_MASK)
        return FALSE;

    if (!(value & PH_LOCK_OWNED))
    {
        return _InterlockedCompareExchange(
            &FastLock->Value,
            value + PH_LOCK_OWNED + PH_LOCK_SHARED_OWNERS_INC,
            value
            ) == value;
    }
    else if ((value >> PH_LOCK_SHARED_OWNERS_SHIFT) & PH_LOCK_SHARED_OWNERS_MASK)
    {
        return _InterlockedCompareExchange(
            &FastLock->Value,
            value + PH_LOCK_SHARED_OWNERS_INC,
            value
            ) == value;
    }
    else
    {
        return FALSE;
    }
}
