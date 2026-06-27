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
 * until needed. Additionally, the kernel event object is automatically freed when no longer needed.
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
#include <mapldr.h>

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

#pragma warning(push)
#pragma warning(disable : 6387)
    WritePointerRelease(&Event->EventHandle, NULL);
#pragma warning(pop)
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
    // Only proceed if the event isn't set already.
    if (!_InterlockedBitTestAndSetPointer((PLONG_PTR)&Event->Value, PH_EVENT_SET_SHIFT))
    {
        HANDLE eventHandle;

        eventHandle = ReadPointerAcquire(&Event->EventHandle);

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
 * \return TRUE if the event object was set before the timeout period expired, otherwise FALSE.
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

    value = ReadULongPtrAcquire(&Event->Value);

    // Shortcut: if the event is set, return immediately.
    if (value & PH_EVENT_SET)
        return TRUE;

    // Shortcut: if the timeout is 0, return immediately if the event isn't set.
    if (Timeout && Timeout->QuadPart == 0)
        return FALSE;

    // Prevent the event from being invalidated.
    PhpReferenceEvent(Event);

    eventHandle = ReadPointerAcquire(&Event->EventHandle);

    if (!eventHandle)
    {
        OBJECT_ATTRIBUTES objectAttributes;
        InitializeObjectAttributes(&objectAttributes, NULL, OBJ_EXCLUSIVE, NULL, NULL);
        NtCreateEvent(&eventHandle, EVENT_ALL_ACCESS, &objectAttributes, NotificationEvent, FALSE);
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
            eventHandle = ReadPointerAcquire(&Event->EventHandle);
        }
    }

    // Essential: check the event one last time to see if it is set.
    if (!(ReadULongPtrAcquire(&Event->Value) & PH_EVENT_SET))
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
 * \remarks This function is not thread-safe. Make sure no other threads are using the event when
 * you call this function.
 * \remarks Safe to call concurrently with PhWaitForEvent. The PH_EVENT_SET bit is cleared
 * atomically while preserving the refcount field, so waiters that have already incremented
 * the refcount are not stomped. A waiter currently inside RtlWaitOnAddress has captured a
 * value with PH_EVENT_SET clear (otherwise it would have broken out of the loop), so
 * clearing the bit does not invalidate its undesired value — no spurious wake is needed.
 */
VOID FASTCALL PhfResetEvent(
    _Inout_ PPH_EVENT Event
    )
{
#if defined(PH_EVENT_UNSAFE_RESET)

    assert(!Event->EventHandle);

    if (PhTestEvent(Event))
    {
        WriteULongPtrRelease(&Event->Value, PH_EVENT_REFCOUNT_INC);
    }

#else

    ULONG_PTR oldValue;
    ULONG_PTR newValue;

    do
    {
        oldValue = ReadULongPtrAcquire(&Event->Value);

        if (!(oldValue & PH_EVENT_SET))
            return;

        newValue = oldValue & ~(ULONG_PTR)PH_EVENT_SET;
    } while ((ULONG_PTR)_InterlockedCompareExchangePointer(
        (PVOID *)&Event->Value,
        (PVOID)newValue,
        (PVOID)oldValue
        ) != oldValue);

#endif
}

/**
 * Initializes a barrier object.
 *
 * \param Barrier A pointer to a barrier object.
 * \param Target The number of participants required to release the barrier.
 */
VOID FASTCALL PhfInitializeBarrier(
    _Out_ PPH_BARRIER Barrier,
    _In_ ULONG_PTR Target
    )
{
    Barrier->Value = Target << PH_BARRIER_TARGET_SHIFT;
    PhInitializeWakeEvent(&Barrier->WakeEvent);
}

/**
 * Blocks on a barrier according to the participant role.
 *
 * \param Barrier A barrier.
 * \param Role The participant role.
 * \param Spin TRUE to spin on the barrier before blocking, FALSE to block immediately.
 */
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
 * \return TRUE for an unspecified thread after each phase, and FALSE for all other threads.
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

    value = ReadULongPtrAcquire(&Barrier->Value);

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

/**
 * Initializes a rundown protection object.
 *
 * \param Protection A pointer to a rundown protection object.
 */
VOID FASTCALL PhfInitializeRundownProtection(
    _Out_ PPH_RUNDOWN_PROTECT Protection
    )
{
    Protection->Value = 0;
}

/**
 * Attempts to acquire rundown protection.
 *
 * \param Protection A rundown protection object.
 * \return TRUE if rundown protection was acquired, otherwise FALSE.
 */
BOOLEAN FASTCALL PhfAcquireRundownProtection(
    _Inout_ PPH_RUNDOWN_PROTECT Protection
    )
{
    ULONG_PTR value;

    // Increment the reference count only if rundown has not started.

    while (TRUE)
    {
        value = ReadULongPtrAcquire(&Protection->Value);

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

/**
 * Releases rundown protection.
 *
 * \param Protection A rundown protection object.
 */
VOID FASTCALL PhfReleaseRundownProtection(
    _Inout_ PPH_RUNDOWN_PROTECT Protection
    )
{
    ULONG_PTR value;

    while (TRUE)
    {
        value = ReadULongPtrAcquire(&Protection->Value);

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

/**
 * Starts rundown and waits for all protected users to finish.
 *
 * \param Protection A rundown protection object.
 */
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
        value = ReadULongPtrAcquire(&Protection->Value);
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

/**
 * Initializes an init-once object.
 *
 * \param InitOnce A pointer to an init-once object.
 */
VOID FASTCALL PhfInitializeInitOnce(
    _Out_ PPH_INITONCE InitOnce
    )
{
    PhInitializeEvent(&InitOnce->Event);
}

/**
 * Begins one-time initialization.
 *
 * \param InitOnce An init-once object.
 * \return TRUE if the caller should perform initialization, otherwise FALSE.
 */
BOOLEAN FASTCALL PhfBeginInitOnce(
    _Inout_ PPH_INITONCE InitOnce
    )
{
    if (!_InterlockedBitTestAndSetPointer(&InitOnce->Event.Value, PH_INITONCE_INITIALIZING_SHIFT))
        return TRUE;

    PhWaitForEvent(&InitOnce->Event, NULL);

    return FALSE;
}

/**
 * Completes one-time initialization and releases waiting threads.
 *
 * \param InitOnce An init-once object.
 */
VOID FASTCALL PhfEndInitOnce(
    _Inout_ PPH_INITONCE InitOnce
    )
{
    PhSetEvent(&InitOnce->Event);
}

//
// Wait Completion Packets
//

/*
 * The Windows executive exposes NtWaitForMultipleObjects, which allows a thread
 * to wait on the state of multiple dispatcher objects. This interface is
 * inherently limited by MAXIMUM_WAIT_OBJECTS (64), a constraint derived from the
 * fixed wait block storage associated with KTHREAD.
 *
 * Applications that need to monitor hundreds or thousands of handles are
 * traditionally forced to shard the wait set across many threads, introduce
 * additional synchronization, or use per-handle thread-pool waits. These
 * approaches scale poorly: they increase thread count, memory usage, scheduler
 * activity, context switching, and per-handle registration/dispatch overhead.
 *
 * Windows 8 introduced wait completion packets. A wait completion packet can be
 * associated with a waitable object and an I/O completion port, causing the
 * kernel to queue a completion when the target object becomes signaled. This
 * allows a single consumer thread to receive wait completions for large wait
 * sets without being constrained by the 64-object NtWaitForMultipleObjects
 * limit.
 *
 * Wait completion packet associations are one-shot. After a completion is
 * removed from the completion port, the association must be re-established if
 * further notifications are required.
 *
 * This implementation uses wait completion packets and an I/O completion port to
 * support scalable waiting over arbitrary-size handle arrays, bounded by normal
 * object, handle, and memory limits. This avoids wait-set sharding and greatly
 * reduces the thread, stack, dispatcher, and scheduling overhead required by
 * traditional multi-threaded wait models.
 *
 * Statistics for one thread using PhWaitForManyObjects and wait completion
 * packets (not optimized):
 *
 *        50 objects: alloc=000.39 KB   avg=000.13 ms
 *       100 objects: alloc=000.78 KB   avg=000.25 ms
 *       500 objects: alloc=003.91 KB   avg=001.46 ms
 *      1000 objects: alloc=007.81 KB   avg=002.92 ms
 *      5000 objects: alloc=039.06 KB   avg=013.70 ms
 *     10000 objects: alloc=078.12 KB   avg=031.16 ms
 *     20000 objects: alloc=156.25 KB   avg=054.91 ms
 *     50000 objects: alloc=390.62 KB   avg=133.89 ms
 *
 * Notes:
 *  - One consumer thread.
 *  - Linear O(n) setup cost, approximately 2.7-3.0 us per object.
 *  - 8 bytes of user-mode memory per wait object.
 *  - No wait-sharding thread amplification.
 *  - Minimal and predictable linear setup cost.
 *
 * Monitoring 50,000 wait objects with NtWaitForMultipleObjects would require:
 *  - 782 wait groups of MAXIMUM_WAIT_OBJECTS (64) handles.
 *  - 782 user threads, 782 thread handles, 782 stacks, 782 ETHREADs, and 782 KTHREADs.
 *  - Multiple SRW locks for ynchronization to merge group completions.
 *  - Constant scheduler and context switching across many waiter threads.
 *  - Significantly more memory and dispatch overhead.
 *
 * Wait completion packets are a significant scalability improvement over
 * sharded NtWaitForMultipleObjects waits. The native APIs are documented by
 * Microsoft and exported by ntdll.dll, but they have no associated import
 * library or SDK header. Despite being available since Windows 8, no usage of
 * wait completion packets has been found outside the Windows thread pool 
 * and System Informer.
 */

typeof(&NtCreateWaitCompletionPacket) NtCreateWaitCompletionPacket_I = NULL;
typeof(&NtAssociateWaitCompletionPacket) NtAssociateWaitCompletionPacket_I = NULL;

BOOLEAN PhInitializeWaitCompletionImports(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if ((baseAddress = PhGetDllHandle(L"ntdll.dll")))
        {
            NtCreateWaitCompletionPacket_I = (typeof(&NtCreateWaitCompletionPacket))PhGetDllBaseProcedureAddress(baseAddress, "NtCreateWaitCompletionPacket", 0);
            NtAssociateWaitCompletionPacket_I = (typeof(&NtAssociateWaitCompletionPacket))PhGetDllBaseProcedureAddress(baseAddress, "NtAssociateWaitCompletionPacket", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (NtCreateWaitCompletionPacket_I && NtAssociateWaitCompletionPacket_I)
        return TRUE;

    return FALSE;
}

/**
 * Creates a wait completion packet.
 *
 * \param WaitCompletionPacketHandle A variable which receives a handle to the wait completion packet.
 * \param DesiredAccess The desired access to the wait completion packet.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/devnotes/ntcreatewaitcompletionpacket
 */
NTSTATUS PhCreateWaitCompletionPacket(
    _Out_ PHANDLE WaitCompletionPacketHandle,
    _In_ ACCESS_MASK DesiredAccess
    )
{
    OBJECT_ATTRIBUTES objectAttributes;

    if (!PhInitializeWaitCompletionImports())
        return STATUS_PROCEDURE_NOT_FOUND;

    InitializeObjectAttributes(
        &objectAttributes,
        NULL,
        OBJ_EXCLUSIVE,
        NULL,
        NULL
        );

    return NtCreateWaitCompletionPacket_I(
        WaitCompletionPacketHandle,
        DesiredAccess,
        &objectAttributes
        );
}

/**
 * Associates a wait completion packet with a waitable object and an I/O completion object.
 *
 * \param WaitCompletionPacketHandle A handle to the wait completion packet.
 * \param IoCompletionHandle A handle to the I/O completion object.
 * \param TargetObjectHandle A handle to the target waitable object.
 * \param KeyContext The key context queued to the I/O completion object.
 * \param ApcContext The APC context queued to the I/O completion object.
 * \param IoStatus The status queued to the I/O completion object.
 * \param IoStatusInformation The information value queued to the I/O completion object.
 * \param AlreadySignaled A variable which receives whether the target object was already signaled.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/devnotes/ntassociatewaitcompletionpacket
 */
NTSTATUS PhAssociateWaitCompletionPacket(
    _In_ HANDLE WaitCompletionPacketHandle,
    _In_ HANDLE IoCompletionHandle,
    _In_ HANDLE TargetObjectHandle,
    _In_opt_ PVOID KeyContext,
    _In_opt_ PVOID ApcContext,
    _In_ NTSTATUS IoStatus,
    _In_ ULONG_PTR IoStatusInformation,
    _Out_opt_ PBOOLEAN AlreadySignaled
    )
{
    if (!PhInitializeWaitCompletionImports())
        return STATUS_PROCEDURE_NOT_FOUND;

    return NtAssociateWaitCompletionPacket_I(
        WaitCompletionPacketHandle,
        IoCompletionHandle,
        TargetObjectHandle,
        KeyContext,
        ApcContext,
        IoStatus,
        IoStatusInformation,
        AlreadySignaled
        );
}

//
// After a wait completion packet fires it is consumed and stops monitoring
// its target. For WaitAll across auto-reset events / semaphores this means
// subsequent state transitions of an object are silently lost. This helper
// closes the spent packet at the given slot and arms a fresh one for the
// same target so the IOCP continues to receive notifications. WaitAny does
// not need this — the first fire already completes the wait.
//
// Note: Auto-reset objects (e.g., auto-reset events, semaphores whose count may drop to
// zero and rise again) are handled by re-associating a fresh wait completion packet
// each time a packet fires in WaitAll mode, so subsequent state transitions of the
// same object are still observed. The completion criterion remains "each object
// observed signaled at least once during the wait" — this is weaker than the kernel's
// NtWaitForMultipleObjects(WaitAll), which atomically holds each object's signaled
// state across the wait boundary. The kernel guarantee cannot be reproduced from user
// mode through WCPs because each WCP consumes its target's signal at fire time.
// One-shot objects (processes, threads, manual-reset events) are fully supported.
static NTSTATUS PhpReassociateWaitCompletionPacket(
    _Inout_ PHANDLE WaitPacketSlot,
    _In_ HANDLE IoCompletionHandle,
    _In_ HANDLE TargetHandle,
    _In_ ULONG ObjectIndex
    )
{
    NTSTATUS status;
    HANDLE newPacket = NULL;
    BOOLEAN alreadySignaled = FALSE;

    if (*WaitPacketSlot)
    {
        NtClose(*WaitPacketSlot);
        *WaitPacketSlot = NULL;
    }

    status = PhCreateWaitCompletionPacket(
        &newPacket,
        WAIT_COMPLETION_PACKET_ALL_ACCESS
    );

    if (!NT_SUCCESS(status))
        return status;

    status = PhAssociateWaitCompletionPacket(
        newPacket,
        IoCompletionHandle,
        TargetHandle,
        UlongToPtr(ObjectIndex),
        UlongToPtr(ObjectIndex),
        STATUS_SUCCESS,
        STATUS_SUCCESS,
        &alreadySignaled
    );

    if (!NT_SUCCESS(status))
    {
        NtClose(newPacket);
        return status;
    }

    *WaitPacketSlot = newPacket;
    return STATUS_SUCCESS;
}

/**
 * Waits for all or any of the specified objects to become signaled using I/O completion
 * port wait completion packets. Supports waiting on more than MAXIMUM_WAIT_OBJECTS (64)
 * handles (Tested with 5000 wait objects). Use this function instead of NtWaitForMultipleObjects
 * when the thread needs to wait on more than MAXIMUM_WAIT_OBJECTS and improve performance.
 *
 * \param[in] ObjectCount The number of object handles in the Handles array.
 * \param[in] Handles An array of handles to waitable objects.
 * \param[in] WaitForAll If TRUE, the function waits until all objects are signaled.
 *        If FALSE, the function returns when any one object is signaled.
 * \param[in] Alertable If TRUE, the wait is alertable and may return STATUS_USER_APC or
 *        STATUS_ALERTED. Only effective when ObjectCount <= MAXIMUM_WAIT_OBJECTS, or when
 *        using the NtRemoveIoCompletionEx path (PH_IOCPWAIT_REMOVESINGLE not defined).
 * \param[in] Timeout Optional pointer to a timeout value. If NULL, the wait is infinite.
 *        Relative timeouts (negative QuadPart) are converted internally to an absolute
 *        deadline before the dequeue loop so that the timeout is not re-armed on each call.
 * \param[out] SignaledIndex Optional pointer to a variable that receives the zero-based
 *        index of the first signaled object when WaitAll is FALSE. When WaitAll is TRUE,
 *        this value is set to zero on success.
 * \return NTSTATUS Successful or errant status.
 * \remarks This function requires Windows 8 or later. Each handle in the array must
 *          be a waitable kernel object (e.g., event, process, thread, semaphore).
 *          Auto-reset objects are not fully supported for WaitAll; see file-level remarks.
 *          For WaitAll, a STATUS_USER_APC or STATUS_ALERTED return discards any
 *          partial progress accumulated during the dequeue loop; the caller must
 *          re-enter the wait from scratch to resume.
 */
NTSTATUS PhWaitForManyObjects(
    _In_ ULONG ObjectCount,
    _In_reads_(ObjectCount) PHANDLE Handles,
    _In_ BOOLEAN WaitForAll,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Out_opt_ PULONG SignaledIndex
    )
{
    NTSTATUS status;
    HANDLE ioCompletionHandle = NULL;
    HANDLE waitPacketHandlesStack[MAXIMUM_WAIT_OBJECTS * 2];
    PHANDLE waitPacketHandles = NULL;
    PULONG signaledBitmapBuffer = NULL;
    RTL_BITMAP signaledBitmap;
    LARGE_INTEGER absoluteTimeout;
    PLARGE_INTEGER effectiveTimeout;
    ULONG signaledCount;
    ULONG i;

    if (ObjectCount == 0)
        return STATUS_INVALID_PARAMETER_1;

    if (SignaledIndex)
        *SignaledIndex = ULONG_MAX;

    //
    // For small counts that fit within the fixed-size, delegate to
    // the standard NtWaitForMultipleObjects API. (dmex)
    //

    if (ObjectCount <= MAXIMUM_WAIT_OBJECTS)
    {
        status = NtWaitForMultipleObjects(
            ObjectCount,
            Handles,
            WaitForAll ? WaitAll : WaitAny,
            Alertable,
            Timeout
            );

        if ((status >= STATUS_WAIT_0 && status < (NTSTATUS)(STATUS_WAIT_0 + ObjectCount)) ||
            (status >= STATUS_ABANDONED_WAIT_0 && status < (NTSTATUS)(STATUS_ABANDONED_WAIT_0 + ObjectCount)))
        {
            if (SignaledIndex)
            {
                if (!WaitForAll)
                {
                    if (status >= STATUS_ABANDONED_WAIT_0)
                        *SignaledIndex = (ULONG)(status - STATUS_ABANDONED_WAIT_0);
                    else
                        *SignaledIndex = (ULONG)(status - STATUS_WAIT_0);
                }
                else
                {
                    *SignaledIndex = 0;
                }
            }
        }
        else if (status != STATUS_TIMEOUT)
        {
            // dprintf("[skipped: status=0x%x, objCount=%u]\n", status, ObjectCount);
        }

        return status;
    }

    //
    // Convert a relative timeout to an absolute deadline before entering the
    // dequeue loop. NtRemoveIoCompletion[Ex] treats positive QuadPart values as
    // absolute system time, so passing a pre-computed deadline means the kernel
    // does not re-arm the full duration on every call—preventing the total wait
    // from silently exceeding the caller's intent.
    //
    // NT timeout convention:
    //   NULL           -> infinite wait
    //   QuadPart < 0   -> relative (duration in 100ns units from now)
    //   QuadPart >= 0  -> absolute (100ns units since Jan 1, 1601)
    //

    if (Timeout && Timeout->QuadPart < 0)
    {
        LARGE_INTEGER currentTime;
        NtQuerySystemTime(&currentTime);
        // Subtract the negative value to add the positive duration.
        absoluteTimeout.QuadPart = currentTime.QuadPart - Timeout->QuadPart;
        effectiveTimeout = &absoluteTimeout;
    }
    else
    {
        // NULL (infinite) or already absolute: pass through unchanged.
        effectiveTimeout = Timeout;
    }

    //
    // Create an I/O completion port to receive wait notifications.
    // IO_COMPLETION_MODIFY_STATE is sufficient for queue/dequeue access.
    //

    status = NtCreateIoCompletion(
        &ioCompletionHandle,
        IO_COMPLETION_MODIFY_STATE,
        NULL,
        1
        );

    if (!NT_SUCCESS(status))
        return status;

    //
    // Allocate arrays for packet handles and (for WaitAll) signaled tracking.
    //

    if (ObjectCount <= RTL_NUMBER_OF(waitPacketHandlesStack))
    {
        waitPacketHandles = waitPacketHandlesStack;
        memset(waitPacketHandles, 0, sizeof(waitPacketHandlesStack));
    }
    else
    {
        waitPacketHandles = PhAllocateZero(ObjectCount * sizeof(HANDLE));

        if (!waitPacketHandles)
        {
            status = STATUS_NO_MEMORY;
            goto CleanupExit;
        }
    }

    if (WaitForAll)
    {
        SIZE_T bitmapBufferSize = ((SIZE_T)((ObjectCount + 31) / 32)) * sizeof(ULONG);

        signaledBitmapBuffer = PhAllocateZero(bitmapBufferSize);

        if (!signaledBitmapBuffer)
        {
            status = STATUS_NO_MEMORY;
            goto CleanupExit;
        }

        RtlInitializeBitMap(&signaledBitmap, signaledBitmapBuffer, ObjectCount);
    }

    //
    // Create and associate a wait completion packet for each object.
    //

    for (i = 0; i < ObjectCount; i++)
    {
        BOOLEAN alreadySignaled = FALSE;

        status = PhCreateWaitCompletionPacket(
            &waitPacketHandles[i],
            WAIT_COMPLETION_PACKET_ALL_ACCESS
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhAssociateWaitCompletionPacket(
            waitPacketHandles[i],
            ioCompletionHandle,
            Handles[i],
            UlongToPtr(i),
            UlongToPtr(i),
            STATUS_SUCCESS,
            STATUS_SUCCESS,
            &alreadySignaled
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;
    }

    //
    // Dequeue completion packets until the wait condition is satisfied.
    // EffectiveTimeout is either NULL (infinite) or an absolute deadline,
    // so it is safe to reuse across iterations without re-arming the duration.
    //

    signaledCount = 0;

    while (TRUE)
    {
#if !defined(PH_IOCPWAIT_REMOVESINGLE)
        // Note: NtRemoveIoCompletionEx allocates an extra
        // internal array when Count > 16 (dmex)
        FILE_IO_COMPLETION_INFORMATION completionInfo[16];
        ULONG entriesRemoved = 0;
        ULONG j;

        status = NtRemoveIoCompletionEx(
            ioCompletionHandle,
            completionInfo,
            RTL_NUMBER_OF(completionInfo),
            &entriesRemoved,
            effectiveTimeout,
            Alertable
            );

        if (status != STATUS_SUCCESS)
            goto CleanupExit;

        //
        // Contract: when NtRemoveIoCompletionEx returns STATUS_SUCCESS,
        // entriesRemoved >= 1. Treat zero entries as a hard error rather
        // than spinning, so a future kernel bug surfaces as a failure
        // instead of an infinite busy-loop.
        //

        if (entriesRemoved == 0)
        {
            status = STATUS_UNSUCCESSFUL;
            goto CleanupExit;
        }

        for (j = 0; j < entriesRemoved; j++)
        {
            ULONG objectIndex = PtrToUlong(completionInfo[j].KeyContext);

            if (WaitForAll)
            {
                //
                // Wait-all: track which objects have been signaled, then re-arm
                // a fresh packet on this target so subsequent transitions (e.g.
                // an auto-reset event that pulses again) are still observable.
                //

                if (objectIndex < ObjectCount)
                {
                    if (!RtlTestBit(&signaledBitmap, objectIndex))
                    {
                        RtlSetBit(&signaledBitmap, objectIndex);
                        signaledCount++;
                    }

                    if (signaledCount < ObjectCount)
                    {
                        status = PhpReassociateWaitCompletionPacket(
                            &waitPacketHandles[objectIndex],
                            ioCompletionHandle,
                            Handles[objectIndex],
                            objectIndex
                            );

                        if (!NT_SUCCESS(status))
                            goto CleanupExit;
                    }
                }
            }
            else
            {
                //
                // Wait-any: the first signaled object satisfies the wait.
                //

                if (SignaledIndex)
                    *SignaledIndex = objectIndex;

                status = STATUS_SUCCESS;
                goto CleanupExit;
            }
        }
#else
        //
        // NOTE: NtRemoveIoCompletion has no alertable parameter and the
        // Alertable argument to PhWaitForManyObjects is silently ignored
        // for ObjectCount > MAXIMUM_WAIT_OBJECTS. Use the NtRemoveIoCompletionEx
        // path if alertable waits are required.
        //

        PVOID keyContext = NULL;
        PVOID apcContext = NULL;
        IO_STATUS_BLOCK ioStatusBlock;
        ULONG objectIndex;

        status = NtRemoveIoCompletion(
            ioCompletionHandle,
            &keyContext,
            &apcContext,
            &ioStatusBlock,
            effectiveTimeout
            );

        if (status != STATUS_SUCCESS)
            goto CleanupExit;

        objectIndex = PtrToUlong(keyContext);

        if (!WaitForAll)
        {
            //
            // Wait-any: the first signaled object satisfies the wait.
            //

            if (SignaledIndex)
                *SignaledIndex = objectIndex;

            status = STATUS_SUCCESS;
            goto CleanupExit;
        }

        //
        // Wait-all: track which objects have been signaled, then re-arm
        // a fresh packet on this target so subsequent transitions (e.g.
        // an auto-reset event that pulses again) are still observable.
        //

        if (objectIndex < ObjectCount)
        {
            if (!RtlTestBit(&signaledBitmap, objectIndex))
            {
                RtlSetBit(&signaledBitmap, objectIndex);
                signaledCount++;
            }

            if (signaledCount < ObjectCount)
            {
                status = PhpReassociateWaitCompletionPacket(
                    &waitPacketHandles[objectIndex],
                    ioCompletionHandle,
                    Handles[objectIndex],
                    objectIndex
                    );

                if (!NT_SUCCESS(status))
                    goto CleanupExit;
            }
        }
#endif

        if (signaledCount >= ObjectCount)
        {
            //
            // All objects are signaled.
            //

            if (SignaledIndex)
                *SignaledIndex = 0;

            status = STATUS_SUCCESS;
            goto CleanupExit;
        }
    }

CleanupExit:

    if (waitPacketHandles)
    {
        for (i = 0; i < ObjectCount; i++)
        {
            if (waitPacketHandles[i])
            {
                NtCancelWaitCompletionPacket(waitPacketHandles[i], TRUE);
                NtClose(waitPacketHandles[i]);
            }
        }

        if (waitPacketHandles != waitPacketHandlesStack)
            PhFree(waitPacketHandles);
    }

    if (signaledBitmapBuffer)
        PhFree(signaledBitmapBuffer);

    if (ioCompletionHandle)
        NtClose(ioCompletionHandle);

    return status;
}

/**
 * Waits for any one of the specified objects to become signaled using I/O completion
 * port wait completion packets.
 *
 * This function supports waiting on more than MAXIMUM_WAIT_OBJECTS (64) handles by
 * creating an I/O completion port and associating a wait completion packet with each
 * object. When any object becomes signaled, the corresponding completion packet is
 * delivered through the I/O completion port.
 *
 * \param[in] ObjectCount The number of object handles in the Handles array.
 * \param[in] Handles An array of handles to waitable objects.
 * \param[in] Timeout Optional pointer to a timeout value. If NULL, the wait is infinite.
 *        Relative timeouts (negative QuadPart) are converted to an absolute deadline
 *        internally and are not re-armed per dequeue call.
 * \param[out] SignaledIndex Optional pointer to a variable that receives the zero-based
 *        index of the signaled object in the Handles array.
 * \return NTSTATUS Successful or errant status.
 * \remarks This function requires Windows 8 or later. Each handle in the array must
 *          be a waitable kernel object (e.g., event, process, thread, semaphore).
 *          Auto-reset objects are not fully supported; see file-level remarks.
 */
NTSTATUS PhWaitForAnyObjects(
    _In_ ULONG ObjectCount,
    _In_reads_(ObjectCount) PHANDLE Handles,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Out_opt_ PULONG SignaledIndex
    )
{
    NTSTATUS status;
    HANDLE ioCompletionHandle = NULL;
    HANDLE waitPacketHandlesStack[MAXIMUM_WAIT_OBJECTS * 2];
    PHANDLE waitPacketHandles = NULL;
    PVOID keyContext = NULL;
    PVOID apcContext = NULL;
    IO_STATUS_BLOCK ioStatusBlock;
    LARGE_INTEGER absoluteTimeout;
    PLARGE_INTEGER effectiveTimeout;
    ULONG i;

    if (ObjectCount == 0)
        return STATUS_INVALID_PARAMETER_1;

    if (SignaledIndex)
        *SignaledIndex = ULONG_MAX;

    //
    // Convert a relative timeout to an absolute deadline for parity with
    // PhWaitForManyObjects. NtRemoveIoCompletion is called only once here so
    // the relative form would also be correct, but keeping the two paths
    // symmetric avoids a regression if this is ever changed to a dequeue loop.
    //

    if (Timeout && Timeout->QuadPart < 0)
    {
        LARGE_INTEGER currentTime;
        NtQuerySystemTime(&currentTime);
        absoluteTimeout.QuadPart = currentTime.QuadPart - Timeout->QuadPart;
        effectiveTimeout = &absoluteTimeout;
    }
    else
    {
        effectiveTimeout = Timeout;
    }

    //
    // Create an I/O completion port to receive wait notifications.
    // IO_COMPLETION_MODIFY_STATE is sufficient for queue/dequeue access.
    //

    status = NtCreateIoCompletion(
        &ioCompletionHandle,
        IO_COMPLETION_MODIFY_STATE,
        NULL,
        1
        );

    if (!NT_SUCCESS(status))
        return status;

    //
    // Allocate an array of wait completion packet handles. Zero-init so the
    // cleanup loop can distinguish slots that were never populated from
    // slots that hold a real handle.
    //

    if (ObjectCount <= RTL_NUMBER_OF(waitPacketHandlesStack))
    {
        waitPacketHandles = waitPacketHandlesStack;
        memset(waitPacketHandles, 0, sizeof(waitPacketHandlesStack));
    }
    else
    {
        waitPacketHandles = PhAllocateZero(ObjectCount * sizeof(HANDLE));

        if (!waitPacketHandles)
        {
            status = STATUS_NO_MEMORY;
            goto CleanupExit;
        }
    }

    //
    // Create a wait completion packet for each object and associate
    // it with the I/O completion port. The packet key context carries
    // the index so we can identify which object was signaled.
    //

    for (i = 0; i < ObjectCount; i++)
    {
        BOOLEAN alreadySignaled = FALSE;

        status = PhCreateWaitCompletionPacket(
            &waitPacketHandles[i],
            WAIT_COMPLETION_PACKET_ALL_ACCESS
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        status = PhAssociateWaitCompletionPacket(
            waitPacketHandles[i],
            ioCompletionHandle,
            Handles[i],
            UlongToPtr(i),
            UlongToPtr(i),
            STATUS_SUCCESS,
            STATUS_SUCCESS,
            &alreadySignaled
            );

        if (!NT_SUCCESS(status))
            goto CleanupExit;

        //
        // If the object was already signaled at the time of association,
        // the completion packet has already been queued. We can continue
        // associating remaining packets; the IOCP dequeue below will
        // pick up the signaled one.
        //
    }

    //
    // Wait for any object to become signaled by dequeuing a completion packet.
    //

    status = NtRemoveIoCompletion(
        ioCompletionHandle,
        &keyContext,
        &apcContext,
        &ioStatusBlock,
        effectiveTimeout
        );

    if (status == STATUS_SUCCESS)
    {
        if (SignaledIndex)
        {
            *SignaledIndex = PtrToUlong(keyContext);
        }
    }

CleanupExit:
    if (waitPacketHandles)
    {
        for (i = 0; i < ObjectCount; i++)
        {
            if (waitPacketHandles[i])
            {
                NtCancelWaitCompletionPacket(waitPacketHandles[i], TRUE);
                NtClose(waitPacketHandles[i]);
            }
        }

        if (waitPacketHandles != waitPacketHandlesStack)
            PhFree(waitPacketHandles);
    }

    if (ioCompletionHandle)
    {
        NtClose(ioCompletionHandle);
    }

    return status;
}

/**
 * Demonstrates Windows 10+ IO completion wait multiplexing by waiting on an
 * I/O completion object handle and a caller-supplied termination object in a
 * single dispatcher wait.
 *
 * When the wait is satisfied by the I/O completion handle, this routine issues
 * a non-blocking NtRemoveIoCompletion to dequeue one packet. If another thread
 * drained the queue between wait wake and dequeue, the routine retries the wait
 * until timeout/termination.
 *
 * \\param[in] IoCompletionHandle A handle to an I/O completion object opened with SYNCHRONIZE.
 * \\param[in] TerminationHandle A handle to a waitable object used to terminate the loop.
 * \\param[in] Alertable If TRUE, the wait is alertable and may return STATUS_USER_APC or STATUS_ALERTED.
 * \\param[in] Timeout Optional timeout. Relative values are converted to an absolute deadline.
 * \\param[out] KeyContext Optional pointer that receives the dequeued completion key.
 * \\param[out] ApcContext Optional pointer that receives the dequeued APC context.
 * \\param[out] IoStatusBlock Optional pointer that receives the dequeued I/O status block.
 * \\param[out] Terminated Optional pointer that receives TRUE if TerminationHandle satisfied the wait.
 * \\return STATUS_SUCCESS if a completion packet was dequeued, STATUS_CANCELLED if
 *         TerminationHandle was signaled, or a propagated wait/dequeue status.
 */
NTSTATUS PhWaitForIoCompletionAndTermination(
    _In_ HANDLE IoCompletionHandle,
    _In_ HANDLE TerminationHandle,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Out_opt_ PVOID* KeyContext,
    _Out_opt_ PVOID* ApcContext,
    _Out_opt_ PIO_STATUS_BLOCK IoStatusBlock,
    _Out_opt_ PBOOLEAN Terminated
    )
{
    NTSTATUS status;
    HANDLE waitHandles[2];
    LARGE_INTEGER absoluteTimeout;
    PLARGE_INTEGER effectiveTimeout;

    if (!IoCompletionHandle)
        return STATUS_INVALID_HANDLE;

    if (!TerminationHandle)
        return STATUS_INVALID_HANDLE;

    if (KeyContext)
        *KeyContext = NULL;

    if (ApcContext)
        *ApcContext = NULL;

    if (IoStatusBlock)
        memset(IoStatusBlock, 0, sizeof(IO_STATUS_BLOCK));

    if (Terminated)
        *Terminated = FALSE;

    if (Timeout && Timeout->QuadPart < 0)
    {
        LARGE_INTEGER currentTime;
        NtQuerySystemTime(&currentTime);
        absoluteTimeout.QuadPart = currentTime.QuadPart - Timeout->QuadPart;
        effectiveTimeout = &absoluteTimeout;
    }
    else
    {
        effectiveTimeout = Timeout;
    }

    waitHandles[0] = IoCompletionHandle;
    waitHandles[1] = TerminationHandle;

    while (TRUE)
    {
        status = NtWaitForMultipleObjects(
            RTL_NUMBER_OF(waitHandles),
            waitHandles,
            WaitAny,
            Alertable,
            effectiveTimeout
            );

        if (status == STATUS_WAIT_0)
        {
            PVOID localKeyContext = NULL;
            PVOID localApcContext = NULL;
            IO_STATUS_BLOCK localIoStatusBlock;
            LARGE_INTEGER nonBlockingTimeout;

            nonBlockingTimeout.QuadPart = 0;
            memset(&localIoStatusBlock, 0, sizeof(IO_STATUS_BLOCK));

            status = NtRemoveIoCompletion(
                IoCompletionHandle,
                &localKeyContext,
                &localApcContext,
                &localIoStatusBlock,
                &nonBlockingTimeout
                );

            if (status == STATUS_TIMEOUT)
                continue;

            if (NT_SUCCESS(status))
            {
                if (KeyContext)
                    *KeyContext = localKeyContext;

                if (ApcContext)
                    *ApcContext = localApcContext;

                if (IoStatusBlock)
                    *IoStatusBlock = localIoStatusBlock;
            }

            return status;
        }

        if (status == STATUS_WAIT_1)
        {
            if (Terminated)
                *Terminated = TRUE;

            return STATUS_CANCELLED;
        }

        return status;
    }
}
