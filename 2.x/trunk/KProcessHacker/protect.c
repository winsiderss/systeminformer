/*
 * Process Hacker Driver - 
 *   process protection
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

#include "include/protect.h"

BOOLEAN KphpIsAccessAllowed(
    __in PVOID Object,
    __in KPROCESSOR_MODE AccessMode,
    __in ACCESS_MASK DesiredAccess
    );

BOOLEAN KphpIsCurrentProcessProtected();

VOID KphpProtectRemoveEntry(
    __in PKPH_PROCESS_ENTRY Entry
    );

/* ProtectedProcessRundownProtect
 * 
 * Rundown protection making sure this module doesn't deinitialize before all hook targets 
 * have finished executing and no one is accessing the lookaside list.
 */
static EX_RUNDOWN_REF ProtectedProcessRundownProtect;
/* ProtectedProcessListHead
 * 
 * The head of the process protection linked list. Each entry stores protection 
 * information for a process.
 */
static LIST_ENTRY ProtectedProcessListHead;
/* ProtectedProcessListLock
 * 
 * The spinlock which protects all accesses to the protected process list (even 
 * the individual entries)
 */
static KSPIN_LOCK ProtectedProcessListLock;
/* ProtectedProcessLookasideList
 * 
 * The lookaside list for protected process entries.
 */
static NPAGED_LOOKASIDE_LIST ProtectedProcessLookasideList;

static KPH_OB_OPEN_HOOK ProcessOpenHook = { 0 };
static KPH_OB_OPEN_HOOK ThreadOpenHook = { 0 };

/* KphProtectInit
 * 
 * Initializes process protection.
 * 
 * IRQL: <= APC_LEVEL
 */
NTSTATUS KphProtectInit()
{
    NTSTATUS status;
    
    /* Initialize rundown protection. */
    ExInitializeRundownProtection(&ProtectedProcessRundownProtect);
    /* Initialize list structures. */
    InitializeListHead(&ProtectedProcessListHead);
    KeInitializeSpinLock(&ProtectedProcessListLock);
    ExInitializeNPagedLookasideList(
        &ProtectedProcessLookasideList,
        NULL,
        NULL,
        0,
        sizeof(KPH_PROCESS_ENTRY),
        TAG_PROTECTION_ENTRY,
        0
        );
    
    /* Hook various functions. */
    /* Hooking the open procedure calls for processes and threads allows 
     * us to intercept handle creation/duplication/inheritance. */
    KphInitializeObOpenHook(&ProcessOpenHook, *PsProcessType, KphNewOpenProcedure51, KphNewOpenProcedure60);
    if (!NT_SUCCESS(status = KphObOpenHook(&ProcessOpenHook)))
        return status;
    KphInitializeObOpenHook(&ThreadOpenHook, *PsThreadType, KphNewOpenProcedure51, KphNewOpenProcedure60);
    if (!NT_SUCCESS(status = KphObOpenHook(&ThreadOpenHook)))
        return status;
    
    return STATUS_SUCCESS;
}

/* KphProtectDeinit
 * 
 * Removes process protection and frees associated structures.
 * 
 * IRQL: <= APC_LEVEL
 */
NTSTATUS KphProtectDeinit()
{
    NTSTATUS status = STATUS_SUCCESS;
    KIRQL oldIrql;
    LARGE_INTEGER waitLi;
    
    /* Unhook. */
    status = KphObOpenUnhook(&ProcessOpenHook);
    status = KphObOpenUnhook(&ThreadOpenHook);
    
    /* Wait for all activity to finish. */
    ExWaitForRundownProtectionRelease(&ProtectedProcessRundownProtect);
    /* Wait for a bit (some regions of hook target functions 
       are NOT guarded by rundown protection, e.g. 
       prologues and epilogues). */
    waitLi.QuadPart = KPH_REL_TIMEOUT_IN_SEC(1);
    KeDelayExecutionThread(KernelMode, FALSE, &waitLi);
    
    /* Free all process protection entries. */
    ExDeleteNPagedLookasideList(&ProtectedProcessLookasideList);
    
    return status;
}

/* KphNewOpenProcedure51
 * 
 * New process/thread open procedure for NT 5.1.
 */
NTSTATUS NTAPI KphNewOpenProcedure51(
    __in OB_OPEN_REASON OpenReason,
    __in PEPROCESS Process,
    __in PVOID Object,
    __in ACCESS_MASK GrantedAccess,
    __in ULONG HandleCount
    )
{
    /* Simply call the 6.0 open procedure. */
    /* NOTE: GrantedAccess is always 0 on XP... */
    return KphNewOpenProcedure60(
        OpenReason,
        /* Assume worst case. */
        UserMode,
        Process,
        Object,
        GrantedAccess,
        HandleCount
        );
}

/* KphNewOpenProcedure60
 * 
 * New process/thread open procedure for NT 6.0 and 6.1.
 */
NTSTATUS NTAPI KphNewOpenProcedure60(
    __in OB_OPEN_REASON OpenReason,
    __in KPROCESSOR_MODE AccessMode,
    __in PEPROCESS Process,
    __in PVOID Object,
    __in ACCESS_MASK GrantedAccess,
    __in ULONG HandleCount
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN accessAllowed = TRUE;
    
    /* Prevent the driver from unloading while this routine is executing. */
    if (!ExAcquireRundownProtection(&ProtectedProcessRundownProtect))
    {
        /* Should never happen. */
        return STATUS_INTERNAL_ERROR;
    }
    
    accessAllowed = KphpIsAccessAllowed(
        Object,
        AccessMode,
        /* Assume worst case if granted access not available. */
        !GrantedAccess ? (ACCESS_MASK)-1 : GrantedAccess
        );
    
    if (accessAllowed)
    {
        POBJECT_TYPE objectType = KphGetObjectTypeNt(Object);
        
        /* Call the original open procedure. There shouldn't be any for Windows XP, 
         * while on Windows Vista and 7 it is used for implementing protected 
         * processes (Big Content's DRM protection, not KProcessHacker's protection).
         */
        status = KphObOpenCall(
            objectType == *PsProcessType ? &ProcessOpenHook : &ThreadOpenHook,
            OpenReason,
            AccessMode,
            Process,
            Object,
            GrantedAccess,
            HandleCount
            );
    }
    else
    {
        dprintf("KphNewOpenProcedure60: Access denied.\n");
        status = STATUS_ACCESS_DENIED;
    }
    
    ExReleaseRundownProtection(&ProtectedProcessRundownProtect);
    
    return status;
}

/* KphProtectAddEntry
 * 
 * Protects the specified process.
 * 
 * Thread safety: Full
 * IRQL: <= DISPATCH_LEVEL
 */
PKPH_PROCESS_ENTRY KphProtectAddEntry(
    __in PEPROCESS Process,
    __in HANDLE Tag,
    __in LOGICAL AllowKernelMode,
    __in ACCESS_MASK ProcessAllowMask,
    __in ACCESS_MASK ThreadAllowMask
    )
{
    KIRQL oldIrql;
    PKPH_PROCESS_ENTRY entry;
    
    /* Prevent the lookaside list from being freed. */
    if (!ExAcquireRundownProtection(&ProtectedProcessRundownProtect))
        return NULL;
    
    entry = ExAllocateFromNPagedLookasideList(&ProtectedProcessLookasideList);
    /* Lookaside list no longer needed. */
    ExReleaseRundownProtection(&ProtectedProcessRundownProtect);
    
    if (!entry)
        return NULL;
    
    entry->Process = Process;
    entry->CreatorProcess = PsGetCurrentProcess();
    entry->Tag = Tag;
    entry->AllowKernelMode = AllowKernelMode;
    entry->ProcessAllowMask = ProcessAllowMask;
    entry->ThreadAllowMask = ThreadAllowMask;
    
    KeAcquireSpinLock(&ProtectedProcessListLock, &oldIrql);
    InsertHeadList(&ProtectedProcessListHead, &entry->ListEntry);
    KeReleaseSpinLock(&ProtectedProcessListLock, oldIrql);
    
    return entry;
}

/* KphProtectFindEntry
 * 
 * Finds process protection data.
 * 
 * Thread safety: Full/Limited. The returned pointer is not guaranteed to 
 * point to a valid process entry. However, the copied entry is safe to 
 * read.
 * IRQL: <= DISPATCH_LEVEL
 */
PKPH_PROCESS_ENTRY KphProtectFindEntry(
    __in PEPROCESS Process,
    __in HANDLE Tag,
    __out_opt PKPH_PROCESS_ENTRY ProcessEntryCopy
    )
{
    KIRQL oldIrql;
    PLIST_ENTRY entry = ProtectedProcessListHead.Flink;
    
    KeAcquireSpinLock(&ProtectedProcessListLock, &oldIrql);
    
    while (entry != &ProtectedProcessListHead)
    {
        PKPH_PROCESS_ENTRY processEntry = 
            CONTAINING_RECORD(entry, KPH_PROCESS_ENTRY, ListEntry);
        
        if (
            (Process != NULL && processEntry->Process == Process) || 
            (Tag != NULL && processEntry->Tag == Tag)
            )
        {
            /* Copy the entry if requested. */
            if (ProcessEntryCopy)
                memcpy(ProcessEntryCopy, processEntry, sizeof(KPH_PROCESS_ENTRY));
            
            KeReleaseSpinLock(&ProtectedProcessListLock, oldIrql);
            
            return processEntry;
        }
        
        entry = entry->Flink;
    }
    
    KeReleaseSpinLock(&ProtectedProcessListLock, oldIrql);
    
    return NULL;
}

/* KphProtectRemoveByProcess
 * 
 * Removes protection from the specified process.
 * 
 * Thread safety: Limited. Callers must synchronize remove calls such 
 * as KphProtectRemoveByProcess and KphProtectRemoveByTag.
 * IRQL: <= DISPATCH_LEVEL
 */
BOOLEAN KphProtectRemoveByProcess(
    __in PEPROCESS Process
    )
{
    PKPH_PROCESS_ENTRY entry = KphProtectFindEntry(Process, NULL, NULL);
    
    if (!entry)
        return FALSE;
    
    KphpProtectRemoveEntry(entry);
    
    return TRUE;
}

/* KphProtectRemoveByTag
 * 
 * Removes protection from all processes with the specified tag.
 * 
 * Thread safety: Limited. Callers must synchronize remove calls such 
 * as KphProtectRemoveByProcess and KphProtectRemoveByTag.
 * IRQL: <= DISPATCH_LEVEL
 */
ULONG KphProtectRemoveByTag(
    __in HANDLE Tag
    )
{
    KIRQL oldIrql;
    ULONG count = 0;
    PKPH_PROCESS_ENTRY entry;
    
    /* Keep removing entries until we can't find any more. */
    while (entry = KphProtectFindEntry(NULL, Tag, NULL))
    {
        KphpProtectRemoveEntry(entry);
        count++;
    }
    
    return count;
}

/* KphpIsAccessAllowed
 * 
 * Checks if the specified access is allowed, according to process 
 * protection rules.
 * 
 * Thread safety: Full
 * IRQL: <= DISPATCH_LEVEL
 */
BOOLEAN KphpIsAccessAllowed(
    __in PVOID Object,
    __in KPROCESSOR_MODE AccessMode,
    __in ACCESS_MASK DesiredAccess
    )
{
    POBJECT_TYPE objectType;
    PEPROCESS processObject;
    BOOLEAN isThread = FALSE;
    
    objectType = KphGetObjectTypeNt(Object);
    /* It doesn't matter if it isn't actually a process because we won't be 
       dereferencing it. */
    processObject = (PEPROCESS)Object;
    isThread = objectType == *PsThreadType;
    
    /* If this is a thread, get its parent process. */
    if (isThread)
        processObject = IoThreadToProcess((PETHREAD)Object);
    
    if (
        processObject != PsGetCurrentProcess() && /* let the caller open its own processes/threads */
        (objectType == *PsProcessType || objectType == *PsThreadType) /* only protect processes and threads */
        )
    {
        KPH_PROCESS_ENTRY processEntry;
        
        /* Search for and copy the corresponding process protection entry. */
        if (KphProtectFindEntry(processObject, NULL, &processEntry))
        {
            ACCESS_MASK mask = 
                isThread ? processEntry.ThreadAllowMask : processEntry.ProcessAllowMask;
            
            /* The process/thread is protected. Check if the requested access is allowed. */
            if (
                /* check if kernel-mode is exempt from protection */
                !(processEntry.AllowKernelMode && AccessMode == KernelMode) && 
                /* allow the creator of the rule to bypass protection */
                processEntry.CreatorProcess != PsGetCurrentProcess() && 
                (DesiredAccess & mask) != DesiredAccess
                )
            {
                /* Access denied. */
                dprintf(
                    "%d: Access denied: 0x%08x (%s)\n",
                    PsGetCurrentProcessId(),
                    DesiredAccess,
                    isThread ? "Thread" : "Process"
                    );
                
                return FALSE;
            }
        }
    }
    
    return TRUE;
}

/* KphpIsCurrentProcessProtected
 * 
 * Determines whether the current process is protected.
 * 
 * Thread safety: Full
 * IRQL: <= DISPATCH_LEVEL
 */
BOOLEAN KphpIsCurrentProcessProtected()
{
    return KphProtectFindEntry(PsGetCurrentProcess(), NULL, NULL) != NULL;
}

/* KphpProtectRemoveEntry
 * 
 * Removes and frees process protection data.
 * 
 * Thread safety: Full
 * IRQL: <= DISPATCH_LEVEL
 */
VOID KphpProtectRemoveEntry(
    __in PKPH_PROCESS_ENTRY Entry
    )
{
    KIRQL oldIrql;
    
    KeAcquireSpinLock(&ProtectedProcessListLock, &oldIrql);
    RemoveEntryList(&Entry->ListEntry);
    
    /* Prevent the lookaside list from being destroyed. */
    ExAcquireRundownProtection(&ProtectedProcessRundownProtect);
    ExFreeToNPagedLookasideList(
        &ProtectedProcessLookasideList,
        Entry
        );
    ExReleaseRundownProtection(&ProtectedProcessRundownProtect);
    
    KeReleaseSpinLock(&ProtectedProcessListLock, oldIrql);
}
