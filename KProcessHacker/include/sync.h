/*
 * Process Hacker Driver - 
 *   synchronization code
 * 
 * Copyright (C) 2009 wj32
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

#ifndef _SYNC_H
#define _SYNC_H

#include "kph.h"
#include "ex.h"

/* General synchronization macros */

/* KphEqualSpin
 * 
 * Spins until the first value is equal to the second 
 * value.
 */
FORCEINLINE VOID KphSpinUntilEqual(
    __inout PLONG Value,
    __in LONG Value2
    )
{
    while (InterlockedCompareExchange(
        Value,
        Value2,
        Value2
        ) != Value2)
        YieldProcessor();
}

/* KphNotEqualSpin
 * 
 * Spins until the first value is not equal to the second 
 * value.
 */
FORCEINLINE VOID KphSpinUntilNotEqual(
    __inout PLONG Value,
    __in LONG Value2
    )
{
    while (InterlockedCompareExchange(
        Value,
        Value2,
        Value2
        ) == Value2)
        YieldProcessor();
}

/* Spin Locks */

/* KphAcquireBitSpinLock
 * 
 * Uses the specified bit as a spinlock and acquires the 
 * lock in the given value.
 */
FORCEINLINE VOID KphAcquireBitSpinLock(
    __inout PLONG Value,
    __in LONG Bit
    )
{
    while (InterlockedBitTestAndSet(Value, Bit))
        YieldProcessor();
}

/* KphReleaseBitSpinLock
 * 
 * Uses the specified bit as a spinlock and releases the 
 * lock in the given value.
 */
FORCEINLINE VOID KphReleaseBitSpinLock(
    __inout PLONG Value,
    __in LONG Bit
    )
{
    InterlockedBitTestAndReset(Value, Bit);
}

/* Guarded Locks */
/* Guarded locks are small spinlocks. Code within 
 * synchronized regions run at APC_LEVEL. They also contain 
 * a signal which can used to implement rundown routines.
 */

#define KPH_GUARDED_LOCK_ACTIVE 0x80000000
#define KPH_GUARDED_LOCK_ACTIVE_SHIFT 31
#define KPH_GUARDED_LOCK_SIGNALED 0x40000000
#define KPH_GUARDED_LOCK_SIGNALED_SHIFT 30
#define KPH_GUARDED_LOCK_FLAGS 0xc0000000

typedef struct _KPH_GUARDED_LOCK
{
    LONG Value;
} KPH_GUARDED_LOCK, *PKPH_GUARDED_LOCK;

#define KphAcquireGuardedLock KphfAcquireGuardedLock
VOID FASTCALL KphfAcquireGuardedLock(
    __inout PKPH_GUARDED_LOCK Lock
    );

#define KphReleaseGuardedLock KphfReleaseGuardedLock
VOID FASTCALL KphfReleaseGuardedLock(
    __inout PKPH_GUARDED_LOCK Lock
    );

/* KphInitializeGuardedLock
 * 
 * Initializes a guarded lock.
 * 
 * IRQL: Any
 */
FORCEINLINE VOID KphInitializeGuardedLock(
    __out PKPH_GUARDED_LOCK Lock,
    __in BOOLEAN Signaled
    )
{
    Lock->Value = 0;
    
    if (Signaled)
        Lock->Value |= KPH_GUARDED_LOCK_SIGNALED;
}

/* KphClearGuardedLock
 * 
 * Clears the signal state of a guarded lock, assuming 
 * that the current thread has acquired it.
 * 
 * IRQL: Any
 */
FORCEINLINE VOID KphClearGuardedLock(
    __in PKPH_GUARDED_LOCK Lock
    )
{
    Lock->Value &= ~KPH_GUARDED_LOCK_SIGNALED;
}

/* KphSignalGuardedLock
 * 
 * Signals a guarded lock.
 * 
 * IRQL: Any
 */
FORCEINLINE VOID KphSignalGuardedLock(
    __in PKPH_GUARDED_LOCK Lock
    )
{
    Lock->Value |= KPH_GUARDED_LOCK_SIGNALED;
}

/* KphSignaledGuardedLock
 * 
 * Determines whether a guarded lock is signaled.
 * 
 * IRQL: Any
 */
FORCEINLINE BOOLEAN KphSignaledGuardedLock(
    __in PKPH_GUARDED_LOCK Lock
    )
{
    return !!(Lock->Value & KPH_GUARDED_LOCK_SIGNALED);
}

/* KphAcquireAndClearGuardedLock
 * 
 * Acquires a guarded lock, clear its signal, and raises the IRQL to APC_LEVEL.
 * 
 * IRQL: <= APC_LEVEL
 */
FORCEINLINE VOID KphAcquireAndClearGuardedLock(
    __inout PKPH_GUARDED_LOCK Lock
    )
{
    KphAcquireGuardedLock(Lock);
    KphClearGuardedLock(Lock);
}

/* KphAcquireAndSignalGuardedLock
 * 
 * Acquires a guarded lock, signals it, and raises the IRQL to APC_LEVEL.
 * 
 * IRQL: <= APC_LEVEL
 */
FORCEINLINE VOID KphAcquireAndSignalGuardedLock(
    __inout PKPH_GUARDED_LOCK Lock
    )
{
    KphAcquireGuardedLock(Lock);
    KphSignalGuardedLock(Lock);
}

/* KphAcquireNonSignaledGuardedLock
 * 
 * Acquires a guarded lock and raises the IRQL to APC_LEVEL, 
 * making sure the lock is not signaled. If it is, the 
 * lock is not acquired.
 * 
 * Return value: whether the lock was acquired.
 * IRQL: <= APC_LEVEL
 */
FORCEINLINE BOOLEAN KphAcquireNonSignaledGuardedLock(
    __inout PKPH_GUARDED_LOCK Lock
    )
{
    KphAcquireGuardedLock(Lock);
    
    if (Lock->Value & KPH_GUARDED_LOCK_SIGNALED)
    {
        KphReleaseGuardedLock(Lock);
        return FALSE;
    }
    
    return TRUE;
}

/* KphAcquireSignaledGuardedLock
 * 
 * Acquires a guarded lock and raises the IRQL to APC_LEVEL, 
 * making sure the lock is signaled. If it is not, the 
 * lock is not acquired.
 * 
 * Return value: whether the lock was acquired.
 * IRQL: <= APC_LEVEL
 */
FORCEINLINE BOOLEAN KphAcquireSignaledGuardedLock(
    __inout PKPH_GUARDED_LOCK Lock
    )
{
    KphAcquireGuardedLock(Lock);
    
    if (!(Lock->Value & KPH_GUARDED_LOCK_SIGNALED))
    {
        KphReleaseGuardedLock(Lock);
        return FALSE;
    }
    
    return TRUE;
}

/* KphReleaseAndClearGuardedLock
 * 
 * Releases a guarded lock, clears its signal, and restores the old IRQL.
 * 
 * IRQL: >= APC_LEVEL
 */
FORCEINLINE VOID KphReleaseAndClearGuardedLock(
    __inout PKPH_GUARDED_LOCK Lock
    )
{
    KphClearGuardedLock(Lock);
    KphReleaseGuardedLock(Lock);
}

/* KphReleaseAndSignalGuardedLock
 * 
 * Releases a guarded lock, signals it, and restores the old IRQL.
 * 
 * IRQL: >= APC_LEVEL
 */
FORCEINLINE VOID KphReleaseAndSignalGuardedLock(
    __inout PKPH_GUARDED_LOCK Lock
    )
{
    KphSignalGuardedLock(Lock);
    KphReleaseGuardedLock(Lock);
}

/* Processor Locks */
/* Processor locks prevent code from executing on all other 
 * processors. Code within synchronized regions run at 
 * DISPATCH_LEVEL.
 */

#define TAG_SYNC_DPC ('DShP')

typedef struct _KPH_PROCESSOR_LOCK
{
    /* Synchronizes access to the processor lock. */
    KPH_GUARDED_LOCK Lock;
    /* Storage allocated for DPCs. */
    PKDPC Dpcs;
    /* The number of currently acquired processors. */
    LONG AcquiredProcessors;
    /* The signal for acquired processors to be released. */
    LONG ReleaseSignal;
    /* The old IRQL. */
    KIRQL OldIrql;
    /* Whether the processor lock has been acquired. */
    BOOLEAN Acquired;
} KPH_PROCESSOR_LOCK, *PKPH_PROCESSOR_LOCK;

BOOLEAN KphAcquireProcessorLock(
    __inout PKPH_PROCESSOR_LOCK ProcessorLock
    );

VOID KphInitializeProcessorLock(
    __out PKPH_PROCESSOR_LOCK ProcessorLock
    );

VOID KphReleaseProcessorLock(
    __inout PKPH_PROCESSOR_LOCK ProcessorLock
    );

#endif
