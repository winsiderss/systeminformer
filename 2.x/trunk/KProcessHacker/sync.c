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

#include "include/sync.h"
#include "include/debug.h"

ULONG KphpCountBits(
    __in ULONG_PTR Number
    );

VOID KphpProcessorLockDpc(
    __in PKDPC Dpc,
    __in PVOID DeferredContext,
    __in PVOID SystemArgument1,
    __in PVOID SystemArgument2
    );

/* KphfAcquireGuardedLock
 * 
 * Acquires a guarded lock and raises the IRQL to APC_LEVEL.
 * 
 * IRQL: <= APC_LEVEL
 */
VOID FASTCALL KphfAcquireGuardedLock(
    __inout PKPH_GUARDED_LOCK Lock
    )
{
    KIRQL oldIrql;
    
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    
    /* Raise to APC_LEVEL. */
    oldIrql = KeRaiseIrql(APC_LEVEL, &oldIrql);
    
    /* Acquire the spinlock. */
    KphAcquireBitSpinLock(&Lock->Value, KPH_GUARDED_LOCK_ACTIVE_SHIFT);
    
    /* Now that we have the lock, we must save the old IRQL. */
    /* Clear the old IRQL. */
    Lock->Value &= KPH_GUARDED_LOCK_FLAGS;
    /* Set the new IRQL. */
    Lock->Value |= oldIrql;
}

/* KphfReleaseGuardedLock
 * 
 * Releases a guarded lock and restores the old IRQL.
 * 
 * IRQL: >= APC_LEVEL
 */
VOID FASTCALL KphfReleaseGuardedLock(
    __inout PKPH_GUARDED_LOCK Lock
    )
{
    KIRQL oldIrql;
    
    ASSERT(KeGetCurrentIrql() >= APC_LEVEL);
    
    /* Get the old IRQL. */
    oldIrql = (KIRQL)(Lock->Value & ~KPH_GUARDED_LOCK_FLAGS);
    /* Unlock the spinlock. */
    KphReleaseBitSpinLock(&Lock->Value, KPH_GUARDED_LOCK_ACTIVE_SHIFT);
    /* Restore the old IRQL. */
    KeLowerIrql(oldIrql);
}

/* KphAcquireProcessorLock
 * 
 * Raises the IRQL to DISPATCH_LEVEL and prevents threads from 
 * executing on other processors until the processor lock is released. 
 * Blocks if the supplied processor lock is already in use.
 * 
 * ProcessorLock: A processor lock structure that is present in 
 * non-paged memory.
 * 
 * Comments:
 * Here is how the processor lock works:
 *  1. Tries to acquire the mutex in the processor lock, and 
 *     blocks until it can be obtained.
 *  2. Initializes a DPC for each processor on the computer.
 *  3. Raises the IRQL to DISPATCH_LEVEL to make sure the 
 *     code is not interrupted by a context switch.
 *  4. Queues each of the previously-initialized DPCs, except if 
 *     it is targeted at the current processor.
 *  5. Since DPCs run at DISPATCH_LEVEL, they have exclusive 
 *     control of the processor. As each runs, they increment 
 *     a counter in the processor lock. They then enter a loop.
 *  6. The routine waits for the counter to become n - 1, 
 *     signaling that all (other) processors have been acquired 
 *     (where n is the number of processors).
 *  7. It returns. Any code from here will be running in 
 *     DISPATCH_LEVEL and will be the only code running on the 
 *     machine.
 * Thread safety: Full
 * IRQL: <= APC_LEVEL
 */
BOOLEAN KphAcquireProcessorLock(
    __inout PKPH_PROCESSOR_LOCK ProcessorLock
    )
{
    ULONG i;
    ULONG numberProcessors;
    ULONG currentProcessor;
    
    /* Acquire the processor lock guarded lock. */
    KphAcquireGuardedLock(&ProcessorLock->Lock);
    
    /* Reset some state. */
    ASSERT(ProcessorLock->AcquiredProcessors == 0);
    ProcessorLock->AcquiredProcessors = 0;
    ProcessorLock->ReleaseSignal = 0; /* IMPORTANT */
    
    /* Get the number of processors. */
    numberProcessors = KphpCountBits(KeQueryActiveProcessors());
    
    /* If there's only one processor we can simply raise the IRQL and exit. */
    if (numberProcessors == 1)
    {
        dprintf("KphAcquireProcessorLock: Only one processor, raising IRQL and exiting...\n");
        KeRaiseIrql(DISPATCH_LEVEL, &ProcessorLock->OldIrql);
        ProcessorLock->Acquired = TRUE;
        
        return TRUE;
    }
    
    /* Allocate storage for the DPCs. */
    ProcessorLock->Dpcs = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(KDPC) * numberProcessors,
        TAG_SYNC_DPC
        );
    
    if (!ProcessorLock->Dpcs)
    {
        dprintf("KphAcquireProcessorLock: Could not allocate storage for DPCs!\n");
        KphReleaseGuardedLock(&ProcessorLock->Lock);
        return FALSE;
    }
    
    /* Initialize the DPCs. */
    for (i = 0; i < numberProcessors; i++)
    {
        KeInitializeDpc(&ProcessorLock->Dpcs[i], KphpProcessorLockDpc, NULL);
        KeSetTargetProcessorDpc(&ProcessorLock->Dpcs[i], (CCHAR)i);
        KeSetImportanceDpc(&ProcessorLock->Dpcs[i], HighImportance);
    }
    
    /* Raise the IRQL to DISPATCH_LEVEL to prevent context switching. */
    KeRaiseIrql(DISPATCH_LEVEL, &ProcessorLock->OldIrql);
    /* Get the current processor number. */
    currentProcessor = KeGetCurrentProcessorNumber();
    
    /* Queue the DPCs (except on the current processor). */
    for (i = 0; i < numberProcessors; i++)
        if (i != currentProcessor)
            KeInsertQueueDpc(&ProcessorLock->Dpcs[i], ProcessorLock, NULL);
    
    /* Spinwait for all (other) processors to be acquired. */
    KphSpinUntilEqual(&ProcessorLock->AcquiredProcessors, numberProcessors - 1);
    
    dprintf("KphAcquireProcessorLock: All processors acquired.\n");
    ProcessorLock->Acquired = TRUE;
    
    return TRUE;
}

/* KphInitializeProcessorLock
 * 
 * Initializes a processor lock.
 * 
 * ProcessorLock: A processor lock structure that is present in 
 * non-paged memory.
 * 
 * IRQL: Any
 */
VOID KphInitializeProcessorLock(
    __out PKPH_PROCESSOR_LOCK ProcessorLock
    )
{
    KphInitializeGuardedLock(&ProcessorLock->Lock, FALSE);
    ProcessorLock->Dpcs = NULL;
    ProcessorLock->AcquiredProcessors = 0;
    ProcessorLock->ReleaseSignal = 0;
    ProcessorLock->OldIrql = PASSIVE_LEVEL;
    ProcessorLock->Acquired = FALSE;
}

/* KphReleaseProcessorLock
 * 
 * Allows threads to execute on other processors and restores the IRQL.
 * 
 * ProcessorLock: A processor lock structure that is present in 
 * non-paged memory.
 * 
 * Comments:
 * Here is how the processor lock is released:
 *  1. Sets the signal to release the processors. The DPCs that are 
 *     currently waiting for the signal will return and decrement 
 *     the acquired processors counter.
 *  2. Waits for the acquired processors counter to become zero.
 *  3. Restores the old IRQL. This will always be APC_LEVEL due to 
 *     the mutex.
 *  4. Frees the storage allocated for the DPCs.
 *  5. Releases the processor lock mutex. This will restore the IRQL 
 *     back to normal.
 * Thread safety: Full
 * IRQL: DISPATCH_LEVEL
 */
VOID KphReleaseProcessorLock(
    __inout PKPH_PROCESSOR_LOCK ProcessorLock
    )
{
    if (!ProcessorLock->Acquired)
        return;
    
    /* Signal for the acquired processors to be released. */
    InterlockedExchange(&ProcessorLock->ReleaseSignal, 1);
    
    /* Spinwait for all acquired processors to be released. */
    KphSpinUntilEqual(&ProcessorLock->AcquiredProcessors, 0);
    
    dprintf("KphReleaseProcessorLock: All processors released.\n");
    
    /* Restore the old IRQL (should always be APC_LEVEL due to the 
     * fast mutex). */
    KeLowerIrql(ProcessorLock->OldIrql);
    
    /* Free the DPCs if necessary. */
    if (ProcessorLock->Dpcs != NULL)
    {
        ExFreePoolWithTag(ProcessorLock->Dpcs, TAG_SYNC_DPC);
        ProcessorLock->Dpcs = NULL;
    }
    
    ProcessorLock->Acquired = FALSE;
    
    /* Release the processor lock guarded lock. This will restore the 
     * IRQL back to what it was before the processor lock was 
     * acquired.
     */
    KphReleaseGuardedLock(&ProcessorLock->Lock);
}

/* KphpCountBits
 * 
 * Counts the number of bits set in an integer.
 */
ULONG KphpCountBits(
    __in ULONG_PTR Number
    )
{
    ULONG count = 0;
    
    while (Number)
    {
        count++;
        Number &= Number - 1;
    }
    
    return count;
}

/* KphpProcessorLockDpc
 * 
 * The DPC routine which "locks" processors.
 * 
 * Thread safety: Full
 * IRQL: DISPATCH_LEVEL
 */
VOID KphpProcessorLockDpc(
    __in PKDPC Dpc,
    __in PVOID DeferredContext,
    __in PVOID SystemArgument1,
    __in PVOID SystemArgument2
    )
{
    PKPH_PROCESSOR_LOCK processorLock = (PKPH_PROCESSOR_LOCK)SystemArgument1;
    
    ASSERT(processorLock != NULL);
    
    dprintf("KphpProcessorLockDpc: Acquiring processor %d.\n", KeGetCurrentProcessorNumber());
    
    /* Increase the number of acquired processors. */
    InterlockedIncrement(&processorLock->AcquiredProcessors);
    
    /* Spin until we get the signal to release the processor. */
    KphSpinUntilNotEqual(&processorLock->ReleaseSignal, 0);
    
    /* Decrease the number of acquired processors. */
    InterlockedDecrement(&processorLock->AcquiredProcessors);
    
    dprintf("KphpProcessorLockDpc: Releasing processor %d.\n", KeGetCurrentProcessorNumber());
}
