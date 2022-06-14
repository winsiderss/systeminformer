/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *
 */

/*
 * This file contains code for several synchronization objects.
 *
 * Event. This is a lightweight notification event object that does not create a kernel event object
 * until needed. Additionally the kernel event object is automatically freed when no longer needed.
 * Note that PhfResetEvent is NOT thread-safe.
 *
 * Barrier. This is a non-traditional implementation of a barrier, built on the wake event object. I
 * have identified three types of participants in this process:
 * 1. The slaves, who wait for the master to release them. This is the main mechanism through which
 *    the threads are synchronized.
 * 2. The master, who is the last thread to wait on the barrier. This thread triggers the waking
 *    process, and waits until all slaves have woken.
 * 3. The observers, who are simply threads which were slaves before, were woken, and have tried to
 *    wait on the barrier again before all other slaves have woken.
 *
 * Rundown protection. This object allows a thread to wait until all other threads have finished
 * using a particular resource before freeing the resource.
 *
 * Init-once. This is a lightweight one-time initialization mechanism which uses the event object
 * for any required blocking. The overhead is very small - only a single inlined comparison.
 */

#include <phbase.h>

/**
 * Initializes an event object.
 *
 * \param Event A pointer to an event object.
 */
VOID FASTCALL PhfInitializeEvent(
    _Out_ PPH_EVENT Event
    )
{
    Event->Value = PH_EVENT_REFCOUNT_INC;
    Event->EventHandle = NULL;
}

/**
 * Dereferences the event object used by an event.
 *
 * \param Event A pointer to an event object.
 * \param EventHandle The current value of the event object.
 */
FORCEINLINE VOID PhpDereferenceEvent(
    _Inout_ PPH_EVENT Event,
    _In_opt_ HANDLE EventHandle
    )
{
    ULONG_PTR value;

    value = _InterlockedExchangeAddPointer((PLONG_PTR)&Event->Value, -PH_EVENT_REFCOUNT_INC);

    // See if the reference count has become 0.
    if (((value >> PH_EVENT_REFCOUNT_SHIFT) & PH_EVENT_REFCOUNT_MASK) - 1 == 0)
    {
        if (EventHandle)
        {
            NtClose(EventHandle);
            Event->EventHandle = NULL;
        }
    }
}

/**
 * References the event object used by an event.
 *
 * \param Event A pointer to an event object.
 */
FORCEINLINE VOID PhpReferenceEvent(
    _Inout_ PPH_EVENT Event
    )
{
    _InterlockedExchangeAddPointer((PLONG_PTR)&Event->Value, PH_EVENT_REFCOUNT_INC);
}

/**
 * Sets an event object. Any threads waiting on the event will be released.
 *
 * \param Event A pointer to an event object.
 */
VOID FASTCALL PhfSetEvent(
    _Inout_ PPH_EVENT Event
    )
{
    HANDLE eventHandle;

    // Only proceed if the event isn't set already.
    if (!_InterlockedBitTestAndSetPointer((PLONG_PTR)&Event->Value, PH_EVENT_SET_SHIFT))
    {
        eventHandle = Event->EventHandle;

        if (eventHandle)
        {
            NtSetEvent(eventHandle, NULL);
        }

        PhpDereferenceEvent(Event, eventHandle);
    }
}

/**
 * Waits for an event object to be set.
 *
 * \param Event A pointer to an event object.
 * \param Timeout The timeout value.
 *
 * \return TRUE if the event object was set before the timeout period expired, otherwise FALSE.
 *
 * \remarks To test the event, use PhTestEvent() instead of using a timeout of zero.
 */
BOOLEAN FASTCALL PhfWaitForEvent(
    _Inout_ PPH_EVENT Event,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    BOOLEAN result;
    ULONG_PTR value;
    HANDLE eventHandle;

    value = Event->Value;

    // Shortcut: if the event is set, return immediately.
    if (value & PH_EVENT_SET)
        return TRUE;

    // Shortcut: if the timeout is 0, return immediately if the event isn't set.
    if (Timeout && Timeout->QuadPart == 0)
        return FALSE;

    // Prevent the event from being invalidated.
    PhpReferenceEvent(Event);

    eventHandle = Event->EventHandle;

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
            eventHandle = Event->EventHandle;
        }
    }

    // Essential: check the event one last time to see if it is set.
    if (!(Event->Value & PH_EVENT_SET))
    {
        result = NtWaitForSingleObject(eventHandle, FALSE, Timeout) == STATUS_WAIT_0;
    }
    else
    {
        result = TRUE;
    }

    PhpDereferenceEvent(Event, eventHandle);

    return result;
}

/**
 * Resets an event's state.
 *
 * \param Event A pointer to an event object.
 *
 * \remarks This function is not thread-safe. Make sure no other threads are using the event when
 * you call this function.
 */
VOID FASTCALL PhfResetEvent(
    _Inout_ PPH_EVENT Event
    )
{
    assert(!Event->EventHandle);

    if (PhTestEvent(Event))
        Event->Value = PH_EVENT_REFCOUNT_INC;
}

VOID FASTCALL PhfInitializeBarrier(
    _Out_ PPH_BARRIER Barrier,
    _In_ ULONG_PTR Target
    )
{
    Barrier->Value = Target << PH_BARRIER_TARGET_SHIFT;
    PhInitializeWakeEvent(&Barrier->WakeEvent);
}

FORCEINLINE VOID PhpBlockOnBarrier(
    _Inout_ PPH_BARRIER Barrier,
    _In_ ULONG Role,
    _In_ BOOLEAN Spin
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
    default:
        ASSUME_NO_DEFAULT;
    }

    if (cancel)
    {
        PhSetWakeEvent(&Barrier->WakeEvent, &waitBlock);
        return;
    }

    PhWaitForWakeEvent(&Barrier->WakeEvent, &waitBlock, Spin, NULL);
}

/**
 * Waits until all threads are blocking on the barrier, and resets the state of the barrier.
 *
 * \param Barrier A barrier.
 * \param Spin TRUE to spin on the barrier before blocking, FALSE to block immediately.
 *
 * \return TRUE for an unspecified thread after each phase, and FALSE for all other threads.
 *
 * \remarks By checking the return value of the function, in each phase an action can be performed
 * exactly once. This could, for example, involve merging the results of calculations.
 */
BOOLEAN FASTCALL PhfWaitForBarrier(
    _Inout_ PPH_BARRIER Barrier,
    _In_ BOOLEAN Spin
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
                (PVOID *)&Barrier->Value,
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
                    // Wake the slaves and wait for them to decrease the count to 1. This is so that
                    // we know the slaves have woken and we don't clear the waking bit before they
                    // wake.

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
    _Out_ PPH_RUNDOWN_PROTECT Protection
    )
{
    Protection->Value = 0;
}

BOOLEAN FASTCALL PhfAcquireRundownProtection(
    _Inout_ PPH_RUNDOWN_PROTECT Protection
    )
{
    ULONG_PTR value;

    // Increment the reference count only if rundown has not started.

    while (TRUE)
    {
        value = Protection->Value;

        if (value & PH_RUNDOWN_ACTIVE)
            return FALSE;

        if ((ULONG_PTR)_InterlockedCompareExchangePointer(
            (PVOID *)&Protection->Value,
            (PVOID)(value + PH_RUNDOWN_REF_INC),
            (PVOID)value
            ) == value)
            return TRUE;
    }
}

VOID FASTCALL PhfReleaseRundownProtection(
    _Inout_ PPH_RUNDOWN_PROTECT Protection
    )
{
    ULONG_PTR value;

    while (TRUE)
    {
        value = Protection->Value;

        if (value & PH_RUNDOWN_ACTIVE)
        {
            PPH_RUNDOWN_WAIT_BLOCK waitBlock;

            // Since rundown is active, the reference count has been moved to the waiter's wait
            // block. If we are the last user, we must wake up the waiter.

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
                (PVOID *)&Protection->Value,
                (PVOID)(value - PH_RUNDOWN_REF_INC),
                (PVOID)value
                ) == value)
                break;
        }
    }
}

VOID FASTCALL PhfWaitForRundownProtection(
    _Inout_ PPH_RUNDOWN_PROTECT Protection
    )
{
    ULONG_PTR value;
    ULONG_PTR count;
    PH_RUNDOWN_WAIT_BLOCK waitBlock;
    BOOLEAN waitBlockInitialized;

    // Fast path. If the reference count is 0 or rundown has already been completed, return.
    value = (ULONG_PTR)_InterlockedCompareExchangePointer(
        (PVOID *)&Protection->Value,
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
            (PVOID *)&Protection->Value,
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
    _Out_ PPH_INITONCE InitOnce
    )
{
    PhInitializeEvent(&InitOnce->Event);
}

BOOLEAN FASTCALL PhfBeginInitOnce(
    _Inout_ PPH_INITONCE InitOnce
    )
{
    if (!_InterlockedBitTestAndSetPointer(&InitOnce->Event.Value, PH_INITONCE_INITIALIZING_SHIFT))
        return TRUE;

    PhWaitForEvent(&InitOnce->Event, NULL);

    return FALSE;
}

VOID FASTCALL PhfEndInitOnce(
    _Inout_ PPH_INITONCE InitOnce
    )
{
    PhSetEvent(&InitOnce->Event);
}
