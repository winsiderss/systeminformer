/*
 * Process Hacker - 
 *   misc. synchronization utilities
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

VOID PhInitializeRundownProtection(
    __out PPH_RUNDOWN_PROTECT Protection
    )
{
    Protection->Value = 0;
}

BOOLEAN PhAcquireRundownProtection(
    __inout PPH_RUNDOWN_PROTECT Protection
    )
{
    ULONG_PTR value;

    // Increment the reference count only if rundown 
    // has not started.

    while (TRUE)
    {
        value = Protection->Value;

        if (value & PH_RUNDOWN_ACTIVE)
            return FALSE;

        if ((ULONG_PTR)_InterlockedCompareExchangePointer(
            (PPVOID)&Protection->Value,
            (PVOID)(value + PH_RUNDOWN_REF_INC),
            (PVOID)value
            ) == value)
            return TRUE;
    }
}

VOID PhReleaseRundownProtection(
    __inout PPH_RUNDOWN_PROTECT Protection
    )
{
    ULONG_PTR value;

    while (TRUE)
    {
        value = Protection->Value;

        if (value & PH_RUNDOWN_ACTIVE)
        {
            PPH_RUNDOWN_WAIT_BLOCK waitBlock;

            // Since rundown is active, the reference count has been 
            // moved to the waiter's wait block. If we are the last 
            // user, we must wake up the waiter.

            waitBlock = (PPH_RUNDOWN_WAIT_BLOCK)(value & ~PH_RUNDOWN_ACTIVE);

            if (_InterlockedDecrementPointer(&waitBlock->Count) == 0)
            {
                PhSetEvent(&waitBlock->WakeEvent);
            }

            break;
        }
        else
        {
            // Decrement the reference count normally.

            if ((ULONG_PTR)_InterlockedCompareExchangePointer(
                (PPVOID)&Protection->Value,
                (PVOID)(value - PH_RUNDOWN_REF_INC),
                (PVOID)value
                ) == value)
                break;
        }
    }
}

VOID PhWaitForRundownProtection(
    __inout PPH_RUNDOWN_PROTECT Protection
    )
{
    ULONG_PTR value;
    ULONG_PTR count;
    PH_RUNDOWN_WAIT_BLOCK waitBlock;
    BOOLEAN waitBlockInitialized;

    // Fast path. If the reference count is 0 or 
    // rundown has already been completed, return.
    value = (ULONG_PTR)_InterlockedCompareExchangePointer(
        (PPVOID)&Protection->Value,
        (PVOID)PH_RUNDOWN_ACTIVE,
        (PVOID)0
        );

    if (value == 0 || value == PH_RUNDOWN_ACTIVE)
        return;

    waitBlockInitialized = FALSE;

    while (TRUE)
    {
        value = Protection->Value;
        count = value >> PH_RUNDOWN_REF_SHIFT;

        // Initialize the wait block if necessary.
        if (count != 0 && !waitBlockInitialized)
        {
            PhInitializeEvent(&waitBlock.WakeEvent);
            waitBlockInitialized = TRUE;
        }

        // Save the existing reference count.
        waitBlock.Count = count;

        if ((ULONG_PTR)_InterlockedCompareExchangePointer(
            (PPVOID)&Protection->Value,
            (PVOID)((ULONG_PTR)&waitBlock | PH_RUNDOWN_ACTIVE),
            (PVOID)value
            ) == value)
        {
            if (count != 0)
                PhWaitForEvent(&waitBlock.WakeEvent, INFINITE);

            break;
        }
    }
}

VOID PhInitializeInitOnce(
    __out PPH_INITONCE InitOnce
    )
{
    InitOnce->State = PH_INITONCE_UNINITIALIZED;
    PhInitializeEvent(&InitOnce->WakeEvent);
}

BOOLEAN PhBeginInitOnce(
    __inout PPH_INITONCE InitOnce
    )
{
    LONG oldState;

    // Quick check first.

    oldState = InitOnce->State;

    if (oldState == PH_INITONCE_INITIALIZED)
    {
        return FALSE;
    }
    else if (oldState == PH_INITONCE_INITIALIZING)
    {
        PhWaitForEvent(&InitOnce->WakeEvent, INFINITE);
        return FALSE;
    }

    // Initializing path.

    oldState = _InterlockedCompareExchange(
        &InitOnce->State,
        PH_INITONCE_INITIALIZING,
        PH_INITONCE_UNINITIALIZED
        );

    switch (oldState)
    {
    case PH_INITONCE_UNINITIALIZED:
        return TRUE;
    case PH_INITONCE_INITIALIZED:
        return FALSE;
    case PH_INITONCE_INITIALIZING:
        PhWaitForEvent(&InitOnce->WakeEvent, INFINITE);
        return FALSE;
    default:
        assert(FALSE);
        return FALSE;
    }
}

VOID PhEndInitOnce(
    __inout PPH_INITONCE InitOnce
    )
{
    _InterlockedExchange(&InitOnce->State, PH_INITONCE_INITIALIZED);
    PhSetEvent(&InitOnce->WakeEvent);
}
