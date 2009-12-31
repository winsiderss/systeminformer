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

#include <fastlock.h>

// FastLock is a port of FastResourceLock from PH 1.x.
//
// This code contains no comments because it is a direct 
// port. Please see FastResourceLock.cs in PH 1.x for 
// details.

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

static ULONG PhLockSpinCount;

VOID PhFastLockInitialization()
{
    SYSTEM_INFO info;

    GetSystemInfo(&info);

    if (info.dwNumberOfProcessors > 1)
        PhLockSpinCount = 4000;
    else
        PhLockSpinCount = 0;
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
        CloseHandle(FastLock->ExclusiveWakeEvent);
        FastLock->ExclusiveWakeEvent = NULL;
    }

    if (FastLock->SharedWakeEvent)
    {
        CloseHandle(FastLock->SharedWakeEvent);
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

    handle = CreateSemaphore(NULL, 0, MAXLONG, NULL);

    if (_InterlockedCompareExchangePointer(
        Handle,
        handle,
        NULL
        ) != NULL)
    {
        CloseHandle(handle);
    }
}

VOID PhAcquireFastLockExclusive(
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
        else if (i >= PhLockSpinCount)
        {
            PhpEnsureEventCreated(&FastLock->ExclusiveWakeEvent);

            if (_InterlockedCompareExchange(
                &FastLock->Value,
                value + PH_LOCK_EXCLUSIVE_WAITERS_INC,
                value
                ) == value)
            {
                if (WaitForSingleObject(
                    FastLock->ExclusiveWakeEvent,
                    INFINITE
                    ) != WAIT_OBJECT_0)
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

VOID PhAcquireFastLockShared(
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
        else if (i >= PhLockSpinCount)
        {
            PhpEnsureEventCreated(&FastLock->SharedWakeEvent);

            if (_InterlockedCompareExchange(
                &FastLock->Value,
                value + PH_LOCK_SHARED_WAITERS_INC,
                value
                ) == value)
            {
                if (WaitForSingleObject(
                    FastLock->SharedWakeEvent,
                    INFINITE
                    ) != WAIT_OBJECT_0)
                    PhRaiseStatus(STATUS_UNSUCCESSFUL);

                continue;
            }
        }

        i++;
        YieldProcessor();
    }
}

VOID PhReleaseFastLockExclusive(
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
                ReleaseSemaphore(FastLock->ExclusiveWakeEvent, 1, NULL);

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
                    ReleaseSemaphore(FastLock->SharedWakeEvent, sharedWaiters, 0);

                break;
            }
        }

        YieldProcessor();
    }
}

VOID PhReleaseFastLockShared(
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
                ReleaseSemaphore(FastLock->ExclusiveWakeEvent, 1, NULL);

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

BOOLEAN PhTryAcquireFastLockExclusive(
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

BOOLEAN PhTryAcquireFastLockShared(
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
