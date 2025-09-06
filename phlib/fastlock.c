/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2010
 *
 */

#include <phbase.h>
#include <fastlock.h>

// FastLock is a fast resource (reader-writer) lock.
//
// The code is a direct port of FastResourceLock from PH 1.x. Please see FastResourceLock.cs in PH
// 1.x for details.
//
// The fast lock is around 7% faster than the critical section when there is no contention, when
// used solely for mutual exclusion. It is also much smaller than the critical section.
//
// There are three types of acquire methods:
// * Normal methods (AcquireExclusive, AcquireShared) are preferred for general purpose use.
// * Busy wait methods (SpinAcquireExclusive, SpinAcquireShared) are preferred if
// very little time is spent while the lock is acquired. However, these do not give exclusive
// acquires precedence over shared acquires.
// * Try methods (TryAcquireExclusive, TryAcquireShared) can be used to quickly test if the lock is available.
// 
// Note that all three types of functions can be used concurrently by the same FastLock instance.
//
// Details
//
// Resource lock value width: 32 bits.
// Lock owned (either exclusive or shared): L (1 bit).
// Exclusive waking: W (1 bit).
// Shared owners count: SC (10 bits).
// Shared waiters count: SW (10 bits).
// Exclusive waiters count: EW (10 bits).
//
// Acquire exclusive:
// {L=0,W=0,SC=0,SW,EW=0} -> {L=1,W=0,SC=0,SW,EW=0}
// {L=0,W=1,SC=0,SW,EW} or {L=1,W,SC,SW,EW} ->
//     {L,W,SC,SW,EW+1},
//     wait on event,
//     {L=0,W=1,SC=0,SW,EW} -> {L=1,W=0,SC=0,SW,EW}
//
// Acquire shared:
// {L=0,W=0,SC=0,SW,EW=0} -> {L=1,W=0,SC=1,SW,EW=0}
// {L=1,W=0,SC>0,SW,EW=0} -> {L=1,W=0,SC+1,SW,EW=0}
// {L=1,W=0,SC=0,SW,EW=0} or {L,W=1,SC,SW,EW} or
//     {L,W,SC,SW,EW>0} -> {L,W,SC,SW+1,EW},
//     wait on event,
//     retry.
//
// Release exclusive:
// {L=1,W=0,SC=0,SW,EW>0} ->
//     {L=0,W=1,SC=0,SW,EW-1},
//     release one exclusive waiter.
// {L=1,W=0,SC=0,SW,EW=0} ->
//     {L=0,W=0,SC=0,SW=0,EW=0},
//     release all shared waiters.
//
// Note that we never do a direct acquire when W=1 
// (i.e. L=0 if W=1), so here we don't have to check 
// the value of W.
//
// Release shared:
// {L=1,W=0,SC>1,SW,EW} -> {L=1,W=0,SC-1,SW,EW}
// {L=1,W=0,SC=1,SW,EW=0} -> {L=0,W=0,SC=0,SW,EW=0}
// {L=1,W=0,SC=1,SW,EW>0} ->
//     {L=0,W=1,SC=0,SW,EW-1},
//     release one exclusive waiter.
//
// Again, we don't need to check the value of W.
//
// Convert exclusive to shared:
// {L=1,W=0,SC=0,SW,EW} ->
//     {L=1,W=0,SC=1,SW=0,EW},
//     release all shared waiters.
//
// Convert shared to exclusive:
// {L=1,W=0,SC=1,SW,EW} ->
//     {L=1,W=0,SC=0,SW,EW}
//

// Lock owned: 1 bit.
#define PH_LOCK_OWNED 0x1
// Exclusive waking: 1 bit.
#define PH_LOCK_EXCLUSIVE_WAKING 0x2
// Shared owners count: 10 bits.
#define PH_LOCK_SHARED_OWNERS_SHIFT 2
#define PH_LOCK_SHARED_OWNERS_MASK 0x3fful
#define PH_LOCK_SHARED_OWNERS_INC 0x4
// Shared waiters count: 10 bits.
#define PH_LOCK_SHARED_WAITERS_SHIFT 12
#define PH_LOCK_SHARED_WAITERS_MASK 0x3fful
#define PH_LOCK_SHARED_WAITERS_INC 0x1000
// Exclusive waiters count: 10 bits.
#define PH_LOCK_EXCLUSIVE_WAITERS_SHIFT 22
#define PH_LOCK_EXCLUSIVE_WAITERS_MASK 0x3fful
#define PH_LOCK_EXCLUSIVE_WAITERS_INC 0x400000

#define PH_LOCK_EXCLUSIVE_MASK \
    (PH_LOCK_EXCLUSIVE_WAKING | \
    (PH_LOCK_EXCLUSIVE_WAITERS_MASK << PH_LOCK_EXCLUSIVE_WAITERS_SHIFT))

VOID PhInitializeFastLock(
    _Out_ PPH_FAST_LOCK FastLock
    )
{
    FastLock->Value = 0;
    FastLock->ExclusiveWakeEvent = NULL;
    FastLock->SharedWakeEvent = NULL;
}

VOID PhDeleteFastLock(
    _Inout_ PPH_FAST_LOCK FastLock
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
    _Inout_ PHANDLE Handle
    )
{
    HANDLE semaphoreHandle;
    OBJECT_ATTRIBUTES objectAttributes;

    if (ReadPointerAcquire(Handle))
        return;

    InitializeObjectAttributes(
        &objectAttributes,
        NULL,
        OBJ_EXCLUSIVE,
        NULL,
        NULL
        );

    NtCreateSemaphore(
        &semaphoreHandle,
        SEMAPHORE_ALL_ACCESS,
        &objectAttributes,
        0,
        MAXLONG
        );

    assert(semaphoreHandle);

    if (_InterlockedCompareExchangePointer(
        Handle,
        semaphoreHandle,
        NULL
        ) != NULL)
    {
        NtClose(semaphoreHandle);
    }
}

FORCEINLINE ULONG PhpGetSpinCount(
    VOID
    )
{
    if (PhSystemBasicInformation.NumberOfProcessors > 1)
        return 4000;
    else
        return 0;
}

_May_raise_
_Acquires_exclusive_lock_(*FastLock)
VOID FASTCALL PhfAcquireFastLockExclusive(
    _Inout_ PPH_FAST_LOCK FastLock
    )
{
    ULONG value;
    ULONG i = 0;
    ULONG spinCount;

    spinCount = PhpGetSpinCount();

    while (TRUE)
    {
        value = ReadULongAcquire(&FastLock->Value);

        if (!(value & (PH_LOCK_OWNED | PH_LOCK_EXCLUSIVE_WAKING)))
        {
            if (_InterlockedCompareExchange(
                &FastLock->Value,
                value + PH_LOCK_OWNED,
                value
                ) == value)
                break;
        }
        else if (i >= spinCount)
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

_May_raise_
_Acquires_shared_lock_(*FastLock)
VOID FASTCALL PhfAcquireFastLockShared(
    _Inout_ PPH_FAST_LOCK FastLock
    )
{
    ULONG value;
    ULONG i = 0;
    ULONG spinCount;

    spinCount = PhpGetSpinCount();

    while (TRUE)
    {
        value = ReadULongAcquire(&FastLock->Value);

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
        else if (i >= spinCount)
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

_Releases_exclusive_lock_(*FastLock)
VOID FASTCALL PhfReleaseFastLockExclusive(
    _Inout_ PPH_FAST_LOCK FastLock
    )
{
    ULONG value;

    while (TRUE)
    {
        value = ReadULongAcquire(&FastLock->Value);

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

_Releases_shared_lock_(*FastLock)
VOID FASTCALL PhfReleaseFastLockShared(
    _Inout_ PPH_FAST_LOCK FastLock
    )
{
    ULONG value;

    while (TRUE)
    {
        value = ReadULongAcquire(&FastLock->Value);

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

_When_(return != 0, _Acquires_exclusive_lock_(*FastLock))
BOOLEAN FASTCALL PhfTryAcquireFastLockExclusive(
    _Inout_ PPH_FAST_LOCK FastLock
    )
{
    ULONG value;

    value = ReadULongAcquire(&FastLock->Value);

    if (value & (PH_LOCK_OWNED | PH_LOCK_EXCLUSIVE_WAKING))
        return FALSE;

    return _InterlockedCompareExchange(
        &FastLock->Value,
        value + PH_LOCK_OWNED,
        value
        ) == value;
}

_When_(return != 0, _Acquires_shared_lock_(*FastLock))
BOOLEAN FASTCALL PhfTryAcquireFastLockShared(
    _Inout_ PPH_FAST_LOCK FastLock
    )
{
    ULONG value;

    value = ReadULongAcquire(&FastLock->Value);

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
