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

/**
 * Initializes an event object.
 *
 * \param Event A pointer to an event object.
 */
VOID FASTCALL PhfInitializeEvent(
    __out PPH_EVENT Event
    )
{
    Event->Value = PH_EVENT_REFCOUNT_INC;
    Event->EventHandle = NULL;
}

FORCEINLINE VOID PhpDereferenceEvent(
    __inout PPH_EVENT Event
    )
{
    ULONG value;

    value = _InterlockedExchangeAdd(&Event->Value, -PH_EVENT_REFCOUNT_INC) - PH_EVENT_REFCOUNT_INC;

    // See if the reference count has become 0.
    if ((value >> PH_EVENT_REFCOUNT_SHIFT) == 0)
    {
        if (Event->EventHandle)
        {
            NtClose(Event->EventHandle);
            Event->EventHandle = NULL;
        }
    }
}

FORCEINLINE VOID PhpReferenceEvent(
    __inout PPH_EVENT Event
    )
{
    _InterlockedExchangeAdd(&Event->Value, PH_EVENT_REFCOUNT_INC);
}

/**
 * Sets an event object.
 * Any threads waiting on the event will be released.
 *
 * \param Event A pointer to an event object.
 */
VOID FASTCALL PhfSetEvent(
    __inout PPH_EVENT Event
    )
{
    HANDLE eventHandle;

    // Only proceed if the event isn't set already.
    if (!_interlockedbittestandset((PLONG)&Event->Value, PH_EVENT_SET_SHIFT))
    {
        // Do an up-to-date read.
        eventHandle = *(volatile HANDLE *)&Event->EventHandle;

        if (eventHandle)
        {
            NtSetEvent(eventHandle, NULL);
        }

        PhpDereferenceEvent(Event);
    }
}

/**
 * Waits for an event object to be set.
 *
 * \param Event A pointer to an event object.
 * \param Timeout The timeout value.
 *
 * \return TRUE if the event object was set before the 
 * timeout period expired, otherwise FALSE.
 *
 * \remarks To test the event, use PhTestEvent() instead 
 * of using a timeout of zero.
 */
BOOLEAN FASTCALL PhfWaitForEvent(
    __inout PPH_EVENT Event,
    __in_opt PLARGE_INTEGER Timeout
    )
{
    BOOLEAN result;
    ULONG value;
    HANDLE eventHandle;

    value = Event->Value;

    // Shortcut: if the event is set, return immediately.
    if (value & PH_EVENT_SET)
        return TRUE;

    // Shortcut: if the timeout is 0, return immediately 
    // if the event isn't set.
    if (Timeout && Timeout->QuadPart == 0)
        return FALSE;

    // Prevent the event from being invalidated.
    PhpReferenceEvent(Event);

    eventHandle = *(volatile HANDLE *)&Event->EventHandle;

    // Don't bother creating an event if we already have one.
    if (!eventHandle)
    {
        NtCreateEvent(&eventHandle, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
        assert(eventHandle);

        // Try to set the event handle to our event.
        if (_InterlockedCompareExchangePointer(
            &Event->EventHandle,
            eventHandle,
            NULL
            ) != NULL)
        {
            // Someone else set the event before we did.
            NtClose(eventHandle);
        }
    }

    // Essential: check the event one last time to see if 
    // it is set.
    if (!(*(volatile ULONG *)&Event->Value & PH_EVENT_SET))
    {
        result = NtWaitForSingleObject(Event->EventHandle, FALSE, Timeout) == STATUS_WAIT_0;
    }
    else
    {
        result = TRUE;
    }

    PhpDereferenceEvent(Event);

    return result;
}

/**
 * Resets an event's state.
 *
 * \param Event A pointer to an event object.
 *
 * \remarks This function is not thread-safe.
 * Make sure no other threads are using the 
 * event when you call this function.
 */
VOID FASTCALL PhfResetEvent(
    __inout PPH_EVENT Event
    )
{
    assert(!Event->EventHandle);

    if (PhTestEvent(Event))
        Event->Value = PH_EVENT_REFCOUNT_INC;
}

VOID FASTCALL PhfInitializeBarrier(
    __out PPH_BARRIER Barrier,
    __in ULONG_PTR Target
    )
{
    Barrier->Value = Target << PH_BARRIER_TARGET_SHIFT;
    PhInitializeQueuedLock(&Barrier->WakeEvent);
}

FORCEINLINE VOID PhpBlockOnBarrier(
    __inout PPH_BARRIER Barrier,
    __in ULONG Role,
    __in BOOLEAN Spin
    )
{
    PH_QUEUED_WAIT_BLOCK waitBlock;
    ULONG_PTR cancel;

    PhQueueWakeEvent(&Barrier->WakeEvent, &waitBlock);

    cancel = 0;

    switch (Role)
    {
    case PH_BARRIER_MASTER:
        cancel = ((Barrier->Value >> PH_BARRIER_COUNT_SHIFT) & PH_BARRIER_COUNT_MASK) == 1;
        break;
    case PH_BARRIER_SLAVE:
        cancel = Barrier->Value & PH_BARRIER_WAKING;
        break;
    case PH_BARRIER_OBSERVER:
        cancel = !(Barrier->Value & PH_BARRIER_WAKING);
        break;
    }

    if (cancel)
    {
        PhSetWakeEvent(&Barrier->WakeEvent, &waitBlock);
        return;
    }

    PhWaitForWakeEvent(&Barrier->WakeEvent, &waitBlock, Spin, NULL);
}

/**
 * Waits until all threads are blocking on the barrier, and resets 
 * the state of the barrier.
 *
 * \param Barrier A barrier.
 * \param Spin TRUE to spin on the barrier before blocking, FALSE 
 * to block immediately.
 *
 * \return TRUE for an unspecified thread after each phase, and FALSE 
 * for all other threads.
 *
 * \remarks By checking the return value of the function, in each 
 * phase an action can be performed exactly once. This could, for 
 * example, involve merging the results of calculations.
 */
BOOLEAN FASTCALL PhfWaitForBarrier(
    __inout PPH_BARRIER Barrier,
    __in BOOLEAN Spin
    )
{
    ULONG_PTR value;
    ULONG_PTR newValue;
    ULONG_PTR count;
    ULONG_PTR target;

    value = Barrier->Value;

    while (TRUE)
    {
        if (!(value & PH_BARRIER_WAKING))
        {
            count = (value >> PH_BARRIER_COUNT_SHIFT) & PH_BARRIER_COUNT_MASK;
            target = (value >> PH_BARRIER_TARGET_SHIFT) & PH_BARRIER_TARGET_MASK;
            assert(count != target);

            count++;

            if (count != target)
                newValue = value + PH_BARRIER_COUNT_INC;
            else
                newValue = value + PH_BARRIER_COUNT_INC + PH_BARRIER_WAKING;

            if ((newValue = (ULONG_PTR)_InterlockedCompareExchangePointer(
                (PPVOID)&Barrier->Value,
                (PVOID)newValue,
                (PVOID)value
                )) == value)
            {
                if (count != target)
                {
                    // Wait for the master signal (the last thread to reach the barrier). 
                    // Once we get it, decrement the count to allow the master to continue.

                    do
                    {
                        PhpBlockOnBarrier(Barrier, PH_BARRIER_SLAVE, Spin);
                    } while (!(Barrier->Value & PH_BARRIER_WAKING));

                    value = _InterlockedExchangeAddPointer((PLONG_PTR)&Barrier->Value, -PH_BARRIER_COUNT_INC);

                    if (((value >> PH_BARRIER_COUNT_SHIFT) & PH_BARRIER_COUNT_MASK) - 1 == 1)
                    {
                        PhSetWakeEvent(&Barrier->WakeEvent, NULL); // for the master
                    }

                    return FALSE;
                }
                else
                {
                    // We're the last one to reach the barrier, so we become the master.
                    // Wake the slaves and wait for them to decrease the count to 1. This 
                    // is so that we know the slaves have woken and we don't clear the waking 
                    // bit before they wake.

                    PhSetWakeEvent(&Barrier->WakeEvent, NULL); // for slaves

                    do
                    {
                        PhpBlockOnBarrier(Barrier, PH_BARRIER_MASTER, Spin);
                    } while (((Barrier->Value >> PH_BARRIER_COUNT_SHIFT) & PH_BARRIER_COUNT_MASK) != 1);

                    _InterlockedExchangeAddPointer((PLONG_PTR)&Barrier->Value, -(PH_BARRIER_WAKING + PH_BARRIER_COUNT_INC));
                    PhSetWakeEvent(&Barrier->WakeEvent, NULL); // for observers

                    return TRUE;
                }
            }
        }
        else
        {
            // We're too early; other threads are still waking. Wait for them to finish.

            PhpBlockOnBarrier(Barrier, PH_BARRIER_OBSERVER, Spin);
            newValue = Barrier->Value;
        }

        value = newValue;
    }
}

VOID FASTCALL PhfInitializeRundownProtection(
    __out PPH_RUNDOWN_PROTECT Protection
    )
{
    Protection->Value = 0;
}

BOOLEAN FASTCALL PhfAcquireRundownProtection(
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

VOID FASTCALL PhfReleaseRundownProtection(
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

VOID FASTCALL PhfWaitForRundownProtection(
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
                PhWaitForEvent(&waitBlock.WakeEvent, NULL);

            break;
        }
    }
}

VOID FASTCALL PhfInitializeInitOnce(
    __out PPH_INITONCE InitOnce
    )
{
    InitOnce->State = PH_INITONCE_UNINITIALIZED;
    PhInitializeEvent(&InitOnce->WakeEvent);
}

BOOLEAN FASTCALL PhfBeginInitOnce(
    __inout PPH_INITONCE InitOnce
    )
{
    LONG oldState;

    // Quick check first.

    if (InitOnce->State == PH_INITONCE_INITIALIZED)
        return FALSE;

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
        PhWaitForEvent(&InitOnce->WakeEvent, NULL);
        return FALSE;
    default:
        assert(FALSE);
        return FALSE;
    }
}

VOID FASTCALL PhfEndInitOnce(
    __inout PPH_INITONCE InitOnce
    )
{
    _InterlockedExchange(&InitOnce->State, PH_INITONCE_INITIALIZED);
    PhSetEvent(&InitOnce->WakeEvent);
}
