/*
 * Process Hacker Driver - 
 *   system service logging
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

/* ================ IMPORTANT ================
 * Please read the comments in KphpSsNewKiFastCallEntry to find out how 
 * KiFastCallEntry can be hooked.
 * 
 * Note that the ONLY SUPPORTED METHOD of hooking is KiFastCallEntry, 
 * which means you MUST be using a CPU which supports sysenter.
 * ===========================================
 */

#include "include/sysservicep.h"
#include "include/hook.h"
#include "include/sync.h"
#include "include/trace.h"

extern PDRIVER_OBJECT KphDriverObject;

/* A fast mutex guarding starting/stopping system service logging. */
FAST_MUTEX KphSsMutex;
/* Whether system service logging has been initialized. */
BOOLEAN KphSsInitialized = FALSE;
/* The KiFastCallEntry hook. */
KPH_HOOK KphSsKiFastCallEntryHook;
/* The number of active loggers. */
ULONG KphSsNumberOfActiveLoggers = 0;

/* The object type for client entries. */
PKPH_OBJECT_TYPE KphSsClientEntryType;
/* The object type for ruleset entries. */
PKPH_OBJECT_TYPE KphSsRuleSetEntryType;
/* The object type for rule entries. */
PKPH_OBJECT_TYPE KphSsRuleEntryType;

/* The list of ruleset entries. */
LIST_ENTRY KphSsRuleSetListHead;
/* A push lock guarding accesses to the ruleset list. */
EX_PUSH_LOCK KphSsRuleSetListPushLock;

/* KphSsLogInit
 * 
 * Initializes system service logging.
 */
NTSTATUS KphSsLogInit()
{
    NTSTATUS status = STATUS_SUCCESS;
    
    /* Initialize the system service call data. */
    KphSsDataInit();
    
    /* Initialize the ruleset list. */
    InitializeListHead(&KphSsRuleSetListHead);
    ExInitializeFastMutex(&KphSsMutex);
    ExInitializePushLock(&KphSsRuleSetListPushLock);
    
    /* Initialize the object types. */
    status = KphCreateObjectType(
        &KphSsClientEntryType,
        NonPagedPool,
        0,
        KphpSsClientEntryDeleteProcedure
        );
    
    if (!NT_SUCCESS(status))
        return status;
    
    status = KphCreateObjectType(
        &KphSsRuleSetEntryType,
        NonPagedPool,
        0,
        KphpSsRuleSetEntryDeleteProcedure
        );
    
    if (!NT_SUCCESS(status))
    {
        KphDereferenceObject(KphSsClientEntryType);
        return status;
    }
    
    status = KphCreateObjectType(
        &KphSsRuleEntryType,
        NonPagedPool,
        0,
        NULL
        );
    
    if (!NT_SUCCESS(status))
    {
        KphDereferenceObject(KphSsClientEntryType);
        KphDereferenceObject(KphSsRuleSetEntryType);
        return status;
    }
    
    return status;
}

/* KphSsLogDeinit
 * 
 * Frees system service logging data.
 */
NTSTATUS KphSsLogDeinit()
{
    KphSsDataDeinit();
    
    return STATUS_SUCCESS;
}

/* KphSsLogStart
 * 
 * Starts system service logging.
 */
NTSTATUS KphSsLogStart()
{
#ifdef _X86_
    NTSTATUS status = STATUS_SUCCESS;
    
    /* Make sure we have the KiFastCallEntry+x address. */
    if (!__KiFastCallEntry)
        return STATUS_NOT_SUPPORTED;
    
    ExAcquireFastMutex(&KphSsMutex);
    
    if (KphSsInitialized)
    {
        ExReleaseFastMutex(&KphSsMutex);
        return STATUS_UNSUCCESSFUL;
    }
    
    /* Hook KiFastCallEntry. Logging will start from now. */
    KphInitializeHook(
        &KphSsKiFastCallEntryHook,
        __KiFastCallEntry,
        KphpSsNewKiFastCallEntry
        );
    status = KphHook(&KphSsKiFastCallEntryHook);
    
    if (!NT_SUCCESS(status))
    {
        ExReleaseFastMutex(&KphSsMutex);
        return status;
    }
    
    KphSsInitialized = TRUE;
    
    ExReleaseFastMutex(&KphSsMutex);
    
    return status;
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

/* KphSsLogStop
 * 
 * Stops system service logging.
 */
NTSTATUS KphSsLogStop()
{
#ifdef _X86_
    NTSTATUS status = STATUS_SUCCESS;
    
    ExAcquireFastMutex(&KphSsMutex);
    
    if (!KphSsInitialized)
    {
        ExReleaseFastMutex(&KphSsMutex);
        return STATUS_UNSUCCESSFUL;
    }
    
    status = KphUnhook(&KphSsKiFastCallEntryHook);
    
    if (!NT_SUCCESS(status))
    {
        ExReleaseFastMutex(&KphSsMutex);
        return status;
    }
    
    /* Spin until the logger count reaches 0. */
    KphSpinUntilEqual(&KphSsNumberOfActiveLoggers, 0);
    
    KphSsInitialized = FALSE;
    
    ExReleaseFastMutex(&KphSsMutex);
    
    return status;
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

/* KphSsCreateClientEntry
 * 
 * Creates a client entry which describes a client of the 
 * system service logger. Clients receive system service log events. 
 * Note that a client may have several ruleset entries associated 
 * with it.
 * 
 * ClientEntry: A variable which receives a pointer to the client entry.
 * ProcessHandle: A handle to the client process, with PROCESS_VM_WRITE 
 * access.
 * ReadSemaphoreHandle: A handle to a semaphore which is released when an 
 * event is written to the client buffer. The client must wait for the 
 * semaphore when it is about to read a block.
 * WriteSemaphoreHandle: A handle to a semaphore which is acquired when an 
 * event is about to be written to the client buffer. If the semaphore 
 * cannot be acquired immediately, the event is dropped. The client must 
 * continually read the buffer and release the semaphore.
 * BufferBase: A pointer to a buffer in the client process.
 * BufferSize: The size of the buffer, in bytes.
 * AccessMode: The mode to use when probing arguments.
 */
NTSTATUS KphSsCreateClientEntry(
    __out PKPHSS_CLIENT_ENTRY *ClientEntry,
    __in HANDLE ProcessHandle,
    __in HANDLE ReadSemaphoreHandle,
    __in HANDLE WriteSemaphoreHandle,
    __in PVOID BufferBase,
    __in ULONG BufferSize,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PKPHSS_CLIENT_ENTRY clientEntry;
    PEPROCESS processObject;
    PKSEMAPHORE readSemaphore;
    PKSEMAPHORE writeSemaphore;
    
    /* Probe. */
    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(BufferBase, BufferSize, 1);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }
    
    /* Reference the client process. */
    status = ObReferenceObjectByHandle(
        ProcessHandle,
        PROCESS_VM_WRITE,
        *PsProcessType,
        AccessMode,
        &processObject,
        NULL
        );
    
    if (!NT_SUCCESS(status))
        return status;
    
    /* Reference the read semaphore. */
    status = ObReferenceObjectByHandle(
        ReadSemaphoreHandle,
        SEMAPHORE_MODIFY_STATE,
        *ExSemaphoreObjectType,
        AccessMode,
        &readSemaphore,
        NULL
        );
    
    if (!NT_SUCCESS(status))
    {
        ObDereferenceObject(processObject);
        return status;
    }
    
    /* Reference the write semaphore. */
    status = ObReferenceObjectByHandle(
        WriteSemaphoreHandle,
        SEMAPHORE_MODIFY_STATE,
        *ExSemaphoreObjectType,
        AccessMode,
        &writeSemaphore,
        NULL
        );
    
    if (!NT_SUCCESS(status))
    {
        ObDereferenceObject(processObject);
        ObDereferenceObject(readSemaphore);
        return status;
    }
    
    /* Create the client entry object. */
    status = KphCreateObject(
        &clientEntry,
        sizeof(KPHSS_CLIENT_ENTRY),
        0,
        KphSsClientEntryType,
        0
        );
    
    if (!NT_SUCCESS(status))
    {
        ObDereferenceObject(processObject);
        ObDereferenceObject(readSemaphore);
        ObDereferenceObject(writeSemaphore);
        
        return status;
    }
    
    clientEntry->Process = processObject;
    clientEntry->Enabled = TRUE;
    clientEntry->ReadSemaphore = readSemaphore;
    clientEntry->WriteSemaphore = writeSemaphore;
    ExInitializeFastMutex(&clientEntry->BufferMutex);
    clientEntry->BufferBase = BufferBase;
    clientEntry->BufferSize = BufferSize;
    clientEntry->BufferCursor = 0;
    clientEntry->NumberOfBlocksWritten = 0;
    clientEntry->NumberOfBlocksDropped = 0;
    
    *ClientEntry = clientEntry;
    
    return status;
}

/* KphSsEnableClientEntry
 * 
 * Enables or disables a client entry.
 */
NTSTATUS KphSsEnableClientEntry(
    __in PKPHSS_CLIENT_ENTRY ClientEntry,
    __in BOOLEAN Enable
    )
{
    if (Enable)
        ClientEntry->Enabled = TRUE;
    else
        ClientEntry->Enabled = FALSE;
    
    return STATUS_SUCCESS;
}

/* KphSsQueryClientEntry
 * 
 * Queries information about a client entry.
 */
NTSTATUS KphSsQueryClientEntry(
    __in PKPHSS_CLIENT_ENTRY ClientEntry,
    __out_bcount_opt(ClientInformationLength) PKPHSS_CLIENT_INFORMATION ClientInformation,
    __in ULONG ClientInformationLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    /* Probe the return length if necessary. */
    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(ReturnLength, sizeof(ULONG), 1);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }
    
    /* Check the length. */
    if (ClientInformationLength >= sizeof(KPHSS_CLIENT_INFORMATION))
    {
        if (ClientInformation)
        {
            __try
            {
                /* Probe the buffer if we're not from kernel-mode. */
                if (AccessMode != KernelMode)
                    ProbeForWrite(ClientInformation, sizeof(KPHSS_CLIENT_INFORMATION), 1);
                
                ClientInformation->ProcessId = PsGetProcessId(ClientEntry->Process);
                ClientInformation->BufferBase = ClientEntry->BufferBase;
                ClientInformation->BufferSize = ClientEntry->BufferSize;
                ClientInformation->NumberOfBlocksWritten = ClientEntry->NumberOfBlocksWritten;
                ClientInformation->NumberOfBlocksDropped = ClientEntry->NumberOfBlocksDropped;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
            }
        }
    }
    else
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }
    
    /* Pass the return length back if requested. */
    if (ReturnLength)
    {
        __try
        {
            *ReturnLength = sizeof(KPHSS_CLIENT_INFORMATION);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
        }
    }
    
    return status;
}

/* KphpSsClientEntryDeleteProcedure
 * 
 * Performs cleanup for a client entry.
 */
VOID NTAPI KphpSsClientEntryDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PKPHSS_CLIENT_ENTRY clientEntry = (PKPHSS_CLIENT_ENTRY)Object;
    
    ObDereferenceObject(clientEntry->Process);
    ObDereferenceObject(clientEntry->ReadSemaphore);
    ObDereferenceObject(clientEntry->WriteSemaphore);
}

/* KphSsCreateRuleSetEntry
 * 
 * Creates a ruleset entry which contains a list of rules 
 * and an action to perform.
 */
NTSTATUS KphSsCreateRuleSetEntry(
    __out PKPHSS_RULESET_ENTRY *RuleSetEntry,
    __in PKPHSS_CLIENT_ENTRY ClientEntry,
    __in KPHSS_FILTER_TYPE DefaultFilterType,
    __in KPHSS_RULESET_ACTION Action
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PKPHSS_RULESET_ENTRY ruleSetEntry;
    
    /* Make sure the action is valid. */
    if (Action < LogRuleSetAction || Action >= MaxRuleSetAction)
        return STATUS_INVALID_PARAMETER_3;
    
    /* Create the ruleset object. */
    status = KphCreateObject(
        &ruleSetEntry,
        sizeof(KPHSS_RULESET_ENTRY),
        0,
        KphSsRuleSetEntryType,
        0
        );
    
    if (!NT_SUCCESS(status))
        return status;
    
    /* Initialize the ruleset object. */
    KphReferenceObject(ClientEntry);
    ruleSetEntry->Client = ClientEntry;
    ruleSetEntry->DefaultFilterType = DefaultFilterType;
    ruleSetEntry->Action = Action;
    ruleSetEntry->NextRuleHandle = 4;
    ExInitializePushLock(&ruleSetEntry->RuleListPushLock);
    InitializeListHead(&ruleSetEntry->RuleListHead);
    
    /* Add the ruleset to the list. */
    KeEnterCriticalRegion();
    ExAcquirePushLockExclusive(&KphSsRuleSetListPushLock);
    InsertHeadList(&KphSsRuleSetListHead, &ruleSetEntry->RuleSetListEntry);
    ExReleasePushLock(&KphSsRuleSetListPushLock);
    KeLeaveCriticalRegion();
    
    *RuleSetEntry = ruleSetEntry;
    
    return status;
}

/* KphpSsRuleSetEntryDeleteProcedure
 * 
 * Performs cleanup for a ruleset entry.
 */
VOID NTAPI KphpSsRuleSetEntryDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PKPHSS_RULESET_ENTRY ruleSetEntry = (PKPHSS_RULESET_ENTRY)Object;
    PLIST_ENTRY currentRuleListEntry;
    
    /* Dereference the client entry. */
    KphDereferenceObject(ruleSetEntry->Client);
    
    KeEnterCriticalRegion();
    
    /* Dereference all rules in the ruleset. */
    ExAcquirePushLockExclusive(&ruleSetEntry->RuleListPushLock);
    
    currentRuleListEntry = ruleSetEntry->RuleListHead.Flink;
    
    while (currentRuleListEntry != &ruleSetEntry->RuleListHead)
    {
        PLIST_ENTRY nextEntry;
        
        /* Save the next entry pointer since currentRuleListEntry may 
         * be deallocated due to the dereference.
         */
        nextEntry = currentRuleListEntry->Flink;
        KphDereferenceObject(KPHSS_RULE_ENTRY(currentRuleListEntry));
        currentRuleListEntry = nextEntry;
    }
    
    ExReleasePushLock(&ruleSetEntry->RuleListPushLock);
    
    /* Remove the ruleset from the list. */
    ExAcquirePushLockExclusive(&KphSsRuleSetListPushLock);
    RemoveEntryList(&ruleSetEntry->RuleSetListEntry);
    ExReleasePushLock(&KphSsRuleSetListPushLock);
    
    KeLeaveCriticalRegion();
}

/* KphSsAddProcessIdRule
 * 
 * Adds a process ID rule entry to a ruleset entry.
 */
NTSTATUS KphSsAddProcessIdRule(
    __out PKPHSS_RULE_ENTRY *RuleEntry,
    __in PKPHSS_RULESET_ENTRY RuleSetEntry,
    __in KPHSS_FILTER_TYPE FilterType,
    __in HANDLE ProcessId
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PKPHSS_RULE_ENTRY ruleEntry;
    
    /* Add the rule. */
    status = KphpSsAddRule(&ruleEntry, RuleSetEntry, FilterType, ProcessIdRuleType);
    
    if (!NT_SUCCESS(status))
        return status;
    
    ruleEntry->ProcessIdRule.ProcessId = ProcessId;
    ruleEntry->Initialized = TRUE;
    
    *RuleEntry = ruleEntry;
    
    return status;
}

/* KphSsAddThreadIdRule
 * 
 * Adds a thread ID rule entry to a ruleset entry.
 */
NTSTATUS KphSsAddThreadIdRule(
    __out PKPHSS_RULE_ENTRY *RuleEntry,
    __in PKPHSS_RULESET_ENTRY RuleSetEntry,
    __in KPHSS_FILTER_TYPE FilterType,
    __in HANDLE ThreadId
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PKPHSS_RULE_ENTRY ruleEntry;
    
    /* Add the rule. */
    status = KphpSsAddRule(&ruleEntry, RuleSetEntry, FilterType, ThreadIdRuleType);
    
    if (!NT_SUCCESS(status))
        return status;
    
    ruleEntry->ThreadIdRule.ThreadId = ThreadId;
    ruleEntry->Initialized = TRUE;
    
    *RuleEntry = ruleEntry;
    
    return status;
}

/* KphSsAddPreviousModeRule
 * 
 * Adds a previous mode rule entry to a ruleset entry.
 */
NTSTATUS KphSsAddPreviousModeRule(
    __out PKPHSS_RULE_ENTRY *RuleEntry,
    __in PKPHSS_RULESET_ENTRY RuleSetEntry,
    __in KPHSS_FILTER_TYPE FilterType,
    __in KPROCESSOR_MODE PreviousMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PKPHSS_RULE_ENTRY ruleEntry;
    
    /* Add the rule. */
    status = KphpSsAddRule(&ruleEntry, RuleSetEntry, FilterType, PreviousModeRuleType);
    
    if (!NT_SUCCESS(status))
        return status;
    
    ruleEntry->PreviousModeRule.PreviousMode = PreviousMode;
    ruleEntry->Initialized = TRUE;
    
    *RuleEntry = ruleEntry;
    
    return status;
}

/* KphSsAddNumberRule
 * 
 * Adds a system service number rule entry to a ruleset entry.
 */
NTSTATUS KphSsAddNumberRule(
    __out PKPHSS_RULE_ENTRY *RuleEntry,
    __in PKPHSS_RULESET_ENTRY RuleSetEntry,
    __in KPHSS_FILTER_TYPE FilterType,
    __in ULONG Number
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PKPHSS_RULE_ENTRY ruleEntry;
    
    /* Add the rule. */
    status = KphpSsAddRule(&ruleEntry, RuleSetEntry, FilterType, NumberRuleType);
    
    if (!NT_SUCCESS(status))
        return status;
    
    ruleEntry->NumberRule.Number = Number;
    ruleEntry->Initialized = TRUE;
    
    *RuleEntry = ruleEntry;
    
    return status;
}

/* KphSsGetHandleRule
 * 
 * Gets the handle of a rule.
 */
HANDLE KphSsGetHandleRule(
    __in PKPHSS_RULE_ENTRY RuleEntry
    )
{
    return RuleEntry->Handle;
}

/* KphSsRemoveRule
 * 
 * Removes a rule entry from a ruleset entry.
 */
NTSTATUS KphSsRemoveRule(
    __in PKPHSS_RULESET_ENTRY RuleSetEntry,
    __in HANDLE RuleEntryHandle
    )
{
    PLIST_ENTRY currentListEntry;
    
    KeEnterCriticalRegion();
    ExAcquirePushLockExclusive(&RuleSetEntry->RuleListPushLock);
    
    /* Find the rule in the ruleset. */
    
    currentListEntry = RuleSetEntry->RuleListHead.Flink;
    
    while (currentListEntry != &RuleSetEntry->RuleListHead)
    {
        PKPHSS_RULE_ENTRY ruleEntry = KPHSS_RULE_ENTRY(currentListEntry);
        
        if (ruleEntry->Handle == RuleEntryHandle)
        {
            /* Remove the rule from the list. */
            RemoveEntryList(&ruleEntry->RuleListEntry);
            /* Dereference the rule (it was referenced when it 
             * got added to the list).
             */
            KphDereferenceObject(ruleEntry);
            
            ExReleasePushLock(&RuleSetEntry->RuleListPushLock);
            KeLeaveCriticalRegion();
            
            return STATUS_SUCCESS;
        }
        
        currentListEntry = currentListEntry->Flink;
    }
    
    ExReleasePushLock(&RuleSetEntry->RuleListPushLock);
    KeLeaveCriticalRegion();
    
    return STATUS_INVALID_PARAMETER_2;
}

/* KphpSsAddRule
 * 
 * Adds a rule entry to a ruleset entry.
 */
NTSTATUS KphpSsAddRule(
    __out PKPHSS_RULE_ENTRY *RuleEntry,
    __in PKPHSS_RULESET_ENTRY RuleSetEntry,
    __in KPHSS_FILTER_TYPE FilterType,
    __in KPHSS_RULE_TYPE RuleType
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PKPHSS_RULE_ENTRY ruleEntry;
    
    /* Make sure the filter/rule type is valid. */
    if (FilterType < IncludeFilterType || FilterType >= MaxFilterType)
        return STATUS_INVALID_PARAMETER_3;
    if (RuleType < ProcessIdRuleType || RuleType >= MaxRuleType)
        return STATUS_INVALID_PARAMETER_4;
    
    /* Create the rule entry object. */
    status = KphCreateObject(
        &ruleEntry,
        sizeof(KPHSS_RULE_ENTRY),
        0,
        KphSsRuleEntryType,
        0
        );
    
    if (!NT_SUCCESS(status))
        return status;
    
    /* Initialize the object. */
    ruleEntry->Initialized = FALSE;
    ruleEntry->FilterType = FilterType;
    ruleEntry->RuleType = RuleType;
    
    /* Get a handle for the rule. */
    ruleEntry->Handle = (HANDLE)(ULONG_PTR)InterlockedExchangeAdd(
        &RuleSetEntry->NextRuleHandle,
        KPHSS_RULE_HANDLE_INCREMENT
        );
    
    /* Add the rule to the ruleset. */
    KeEnterCriticalRegion();
    ExAcquirePushLockExclusive(&RuleSetEntry->RuleListPushLock);
    InsertTailList(&RuleSetEntry->RuleListHead, &ruleEntry->RuleListEntry);
    ExReleasePushLock(&RuleSetEntry->RuleListPushLock);
    KeLeaveCriticalRegion();
    /* Add a reference for the rule being on the list. */
    KphReferenceObject(ruleEntry);
    
    *RuleEntry = ruleEntry;
    
    return status;
}

/* KphpSsCreateEventBlock
 * 
 * Allocates and initializes an event block.
 * 
 * EventBlock: A variable which receives a pointer to the event block.
 * Thread: The thread for which the event is being generated.
 * Number: The system service number.
 * Arguments: A pointer to the caller-supplied arguments.
 * NumberOfArguments: The number of arguments, in ULONGs.
 */
NTSTATUS KphpSsCreateEventBlock(
    __out PKPHSS_EVENT_BLOCK *EventBlock,
    __in PKTHREAD Thread,
    __in ULONG Number,
    __in ULONG *Arguments,
    __in ULONG NumberOfArguments
    )
{
    PKPHSS_EVENT_BLOCK eventBlock;
    KPROCESSOR_MODE previousMode;
    ULONG eventBlockSize;
    ULONG argumentsSize;
    ULONG traceSize;
    PVOID stackTrace[MAX_STACK_DEPTH * 2];
    ULONG capturedFrames;
    
    /* Make sure the argument count isn't too large. */
    if (NumberOfArguments > MAX_USHORT)
        return STATUS_INVALID_PARAMETER;
    
    previousMode = ExGetPreviousMode();
    
    /* Capture kernel-mode and user-mode stack traces. 
     * We do this before we allocate the event block so 
     * we can calculate how large the block should be.
     */
    
    /* Get a kernel-mode stack trace. */
    capturedFrames = KphCaptureStackBackTrace(
        0,
        MAX_STACK_DEPTH - 1,
        0,
        stackTrace,
        NULL
        );
    
    if (PsGetCurrentProcess() != PsInitialSystemProcess)
    {
        /* Get a user-mode stack trace. */
        capturedFrames += KphCaptureStackBackTrace(
            0,
            MAX_STACK_DEPTH - 1,
            RTL_WALK_USER_MODE_STACK,
            &stackTrace[capturedFrames],
            NULL
            );
    }
    
    /* Calculate the size of the event block. */
    argumentsSize = NumberOfArguments * sizeof(ULONG);
    traceSize = capturedFrames * sizeof(PVOID);
    eventBlockSize = sizeof(KPHSS_EVENT_BLOCK) + argumentsSize + traceSize;
    
    /* Make sure the block size isn't too large. */
    if (eventBlockSize > MAX_USHORT)
        return STATUS_INVALID_PARAMETER;
    
    /* Allocate the event block. */
    eventBlock = ExAllocatePoolWithTag(PagedPool, eventBlockSize, TAG_EVENT_BLOCK);
    
    if (!eventBlock)
        return STATUS_INSUFFICIENT_RESOURCES;
    
    /* Initialize the event block. */
    eventBlock->Header.Size = (USHORT)eventBlockSize;
    eventBlock->Header.Type = EventBlockType;
    eventBlock->Flags = 0;
    KeQuerySystemTime(&eventBlock->Time);
    eventBlock->ClientId.UniqueThread = PsGetThreadId(Thread);
    eventBlock->ClientId.UniqueProcess = PsGetProcessId(IoThreadToProcess(Thread));
    eventBlock->Number = Number;
    eventBlock->NumberOfArguments = (USHORT)NumberOfArguments;
    eventBlock->ArgumentsOffset = sizeof(KPHSS_EVENT_BLOCK);
    eventBlock->TraceCount = (USHORT)capturedFrames;
    eventBlock->TraceOffset = (USHORT)(sizeof(KPHSS_EVENT_BLOCK) + argumentsSize);
    
    /* Set the flags according to the previous mode. */
    if (previousMode == UserMode)
        eventBlock->Flags |= KPHSS_EVENT_USER_MODE;
    else if (previousMode == KernelMode)
        eventBlock->Flags |= KPHSS_EVENT_KERNEL_MODE;
    
    /* Probe and copy the arguments. */
    if (previousMode != KernelMode)
    {
        __try
        {
            ProbeForRead(Arguments, argumentsSize, 4);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            eventBlock->Flags |= KPHSS_EVENT_PROBE_ARGUMENTS_FAILED;
        }
    }
    
    __try
    {
        /* Copy the arguments to the space immediately after the event block. */
        memcpy((PCHAR)eventBlock + eventBlock->ArgumentsOffset, Arguments, argumentsSize);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        eventBlock->Flags |= KPHSS_EVENT_COPY_ARGUMENTS_FAILED;
    }
    
    /* Copy the stack trace. */
    memcpy((PCHAR)eventBlock + eventBlock->TraceOffset, stackTrace, traceSize);
    
    /* Pass the pointer to the event block back. */
    *EventBlock = eventBlock;
    
    return STATUS_SUCCESS;
}

/* KphpSsFreeEventBlock
 * 
 * Frees an event block created by KphpSsCreateEventBlock.
 */
VOID KphpSsFreeEventBlock(
    __in PKPHSS_EVENT_BLOCK EventBlock
    )
{
    ExFreePoolWithTag(EventBlock, TAG_EVENT_BLOCK);
}

/* KphpSsCaptureSimpleArgument
 * 
 * Captures a simple (1-, 2-, 4- or 8-byte) argument.
 */
NTSTATUS KphpSsCaptureSimpleArgument(
    __out PKPHSS_ARGUMENT_BLOCK *ArgumentBlock,
    __in PVOID Argument,
    __in KPHSS_ARGUMENT_TYPE Type,
    __in KPROCESSOR_MODE PreviousMode
    )
{
    PKPHSS_ARGUMENT_BLOCK argumentBlock;
    ULONG size;
    LARGE_INTEGER value;
    
    /* Return if we have a NULL pointer. */
    if (!Argument)
        return STATUS_INVALID_PARAMETER_2;
    
    /* Get the proper argument size based on the argument type. */
    switch (Type)
    {
        case Int8Argument:
            size = sizeof(BOOLEAN);
            break;
        case Int16Argument:
            size = sizeof(SHORT);
            break;
        case Int32Argument:
            size = sizeof(LONG);
            break;
        case Int64Argument:
            size = sizeof(LARGE_INTEGER);
            break;
        default:
            return STATUS_INVALID_PARAMETER_3;
    }
    
    /* Probe and read the value. */
    __try
    {
        if (PreviousMode != KernelMode)
            ProbeForRead(Argument, size, 1);
        
        memcpy(&value, Argument, size);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }
    
    /* Allocate an argument block. */
    argumentBlock = KphpSsAllocateArgumentBlock(size, Type);
    
    if (!argumentBlock)
        return STATUS_INSUFFICIENT_RESOURCES;
    
    /* Copy the value into the argument block. */
    memcpy(&argumentBlock->Simple, &value, size);
    *ArgumentBlock = argumentBlock;
    
    return STATUS_SUCCESS;
}

/* KphpSsCaptureHandleArgument
 * 
 * Captures a handle argument.
 */
NTSTATUS KphpSsCaptureHandleArgument(
    __out PKPHSS_ARGUMENT_BLOCK *ArgumentBlock,
    __in HANDLE Argument,
    __in KPROCESSOR_MODE PreviousMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PKPHSS_ARGUMENT_BLOCK argumentBlock;
    ULONG bufferLength;
    PVOID object;
    POBJECT_TYPE objectType;
    PUNICODE_STRING objectTypeName;
    PUNICODE_STRING objectNameInfo;
    ULONG returnLength;
    PKPHSS_HANDLE handleInfo;
    PKPHSS_WSTRING wString;
    
    /* Return if we have a NULL handle. */
    if (!Argument)
        return STATUS_INVALID_PARAMETER_2;
    
    /* Make sure the handle isn't a kernel handle if we're 
     * from user-mode. We need exceptions for the process 
     * and thread pseudo-handles.
     */
    if (PreviousMode != KernelMode)
    {
        if (
            IsKernelHandle(Argument) && 
            Argument != NtCurrentProcess() && 
            Argument != NtCurrentThread()
            )
            return STATUS_INVALID_HANDLE;
    }
    
    /* Reference the object. */
    status = ObReferenceObjectByHandle(
        Argument,
        0,
        NULL,
        KernelMode,
        &object,
        NULL
        );
    
    if (!NT_SUCCESS(status))
        return status;
    
    /* Get a pointer to the UNICODE_STRING containing the 
     * object type name.
     */
    objectType = KphGetObjectTypeNt(object);
    objectTypeName = (PUNICODE_STRING)KVOFF(objectType, OffOtName);
    
    /* Allocate a buffer for name information. */
    objectNameInfo = (PUNICODE_STRING)ExAllocatePoolWithTag(
        PagedPool,
        CAPTURE_HANDLE_BUFFER_SIZE,
        TAG_CAPTURE_TEMP_BUFFER
        );
    
    if (!objectNameInfo)
        goto CleanupObject;
    
    /* Query the name of the object. */
    status = KphQueryNameObject(
        object,
        objectNameInfo,
        CAPTURE_HANDLE_BUFFER_SIZE,
        &returnLength
        );
    
    if (!NT_SUCCESS(status))
        goto CleanupName;
    
    /* Allocate an argument block. */
    argumentBlock = KphpSsAllocateArgumentBlock(
        sizeof(KPHSS_HANDLE) + sizeof(KPHSS_WSTRING) + sizeof(KPHSS_WSTRING) + 
        objectTypeName->Length + objectNameInfo->Length,
        HandleArgument
        );
    
    if (!argumentBlock)
        goto CleanupName;
    
    handleInfo = &argumentBlock->Handle;
    /* Calculate the offsets. */
    handleInfo->TypeNameOffset = sizeof(KPHSS_HANDLE);
    handleInfo->NameOffset = 
        handleInfo->TypeNameOffset + sizeof(KPHSS_WSTRING) + 
        objectTypeName->Length;
    
    /* Copy the object type name into the block. */
    wString = (PKPHSS_WSTRING)PTR_ADD_OFFSET(handleInfo, handleInfo->TypeNameOffset);
    wString->Length = objectTypeName->Length;
    memcpy(&wString->Buffer, objectTypeName->Buffer, wString->Length);
    
    /* Copy the object name into the block. */
    wString = (PKPHSS_WSTRING)PTR_ADD_OFFSET(handleInfo, handleInfo->NameOffset);
    wString->Length = objectNameInfo->Length;
    memcpy(&wString->Buffer, objectNameInfo->Buffer, wString->Length);
    
    /* We may be able to get additional information for the 
     * object.
     */
    
    handleInfo->ClientId.UniqueProcess = NULL;
    handleInfo->ClientId.UniqueThread = NULL;
    
    if (objectType == *PsProcessType)
    {
        handleInfo->ClientId.UniqueProcess = PsGetProcessId((PEPROCESS)object);
    }
    else if (objectType == *PsThreadType)
    {
        handleInfo->ClientId.UniqueThread = PsGetThreadId((PETHREAD)object);
        handleInfo->ClientId.UniqueProcess = PsGetProcessId(IoThreadToProcess((PETHREAD)object));
    }
    
    *ArgumentBlock = argumentBlock;
    
CleanupName:
    ExFreePoolWithTag(objectNameInfo, TAG_CAPTURE_TEMP_BUFFER);
CleanupObject:
    ObDereferenceObject(object);
    
    return status;
}

/* KphpSsCaptureUnicodeStringArgument
 * 
 * Captures a UNICODE_STRING argument.
 */
NTSTATUS KphpSsCaptureUnicodeStringArgument(
    __out PKPHSS_ARGUMENT_BLOCK *ArgumentBlock,
    __in PUNICODE_STRING Argument,
    __in KPROCESSOR_MODE PreviousMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PKPHSS_ARGUMENT_BLOCK argumentBlock;
    UNICODE_STRING unicodeString;
    
    /* Return if we have a NULL pointer. */
    if (!Argument)
        return STATUS_INVALID_PARAMETER_2;
    
    /* Probe and copy the UNICODE_STRING structure. */
    __try
    {
        if (PreviousMode != KernelMode)
            ProbeForRead(Argument, sizeof(UNICODE_STRING), 1);
        
        memcpy(&unicodeString, Argument, sizeof(UNICODE_STRING));
        
        /* Probe the buffer, if present. */
        if (unicodeString.Buffer && PreviousMode != KernelMode)
        {
            ProbeForRead(unicodeString.Buffer, unicodeString.Length, 1);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }
    
    /* Check if the string is too large. */
    if (unicodeString.Length > CAPTURE_UNICODE_STRING_MAX_SIZE)
        return STATUS_UNSUCCESSFUL;
    
    /* Allocate an argument block. */
    argumentBlock = KphpSsAllocateArgumentBlock(
        sizeof(KPHSS_UNICODE_STRING) + unicodeString.Length,
        UnicodeStringArgument
        );
    
    if (!argumentBlock)
        return STATUS_INSUFFICIENT_RESOURCES;
    
    /* Copy the string into the argument block. */
    argumentBlock->UnicodeString.Length = unicodeString.Length;
    argumentBlock->UnicodeString.MaximumLength = unicodeString.MaximumLength;
    argumentBlock->UnicodeString.Pointer = unicodeString.Buffer;
    
    if (unicodeString.Buffer)
    {
        __try
        {
            memcpy(argumentBlock->UnicodeString.Buffer, unicodeString.Buffer, unicodeString.Length);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            KphpSsFreeArgumentBlock(argumentBlock);
            return GetExceptionCode();
        }
    }
    
    *ArgumentBlock = argumentBlock;
    
    return status;
}

/* KphpSsCaptureObjectAttributesArgument
 * 
 * Captures an OBJECT_ATTRIBUTES argument.
 */
NTSTATUS KphpSsCaptureObjectAttributesArgument(
    __out PKPHSS_ARGUMENT_BLOCK *ArgumentBlock,
    __in POBJECT_ATTRIBUTES Argument,
    __in KPROCESSOR_MODE PreviousMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PKPHSS_ARGUMENT_BLOCK argumentBlock;
    OBJECT_ATTRIBUTES objectAttributes;
    PKPHSS_ARGUMENT_BLOCK rootDirectoryArgumentBlock = NULL;
    ULONG rootDirectoryArgumentBlockSize = 0;
    PKPHSS_ARGUMENT_BLOCK objectNameArgumentBlock = NULL;
    ULONG objectNameArgumentBlockSize = 0;
    
    /* Return if we have a NULL pointer. */
    if (!Argument)
        return STATUS_INVALID_PARAMETER_2;
    
    /* Probe and copy the OBJECT_ATTRIBUTES structure. */
    __try
    {
        if (PreviousMode != KernelMode)
            ProbeForRead(Argument, sizeof(OBJECT_ATTRIBUTES), 1);
        
        memcpy(&objectAttributes, Argument, sizeof(OBJECT_ATTRIBUTES));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }
    
    /* If we have a root directory, create an argument block from it
     * and copy it to our argument block.
     */
    if (objectAttributes.RootDirectory)
    {
        status = KphpSsCaptureHandleArgument(
            &rootDirectoryArgumentBlock,
            objectAttributes.RootDirectory,
            PreviousMode
            );
        
        /* If we created the argument block, we need to calculate 
         * the size of the KPHSS_HANDLE structure.
         */
        if (NT_SUCCESS(status))
        {
            rootDirectoryArgumentBlockSize = 
                rootDirectoryArgumentBlock->Header.Size - KPHSS_ARGUMENT_BLOCK_OVERHEAD;
        }
        else
        {
            rootDirectoryArgumentBlock = NULL;
        }
    }
    
    /* If we have a object name, create an argument block from it and 
     * copy it to our argument block.
     */
    if (objectAttributes.ObjectName)
    {
        status = KphpSsCaptureUnicodeStringArgument(
            &objectNameArgumentBlock,
            objectAttributes.ObjectName,
            PreviousMode
            );
        
        /* If we created the argument block, we need to calculate 
         * the size of the KPHSS_UNICODE_STRING structure.
         */
        if (NT_SUCCESS(status))
        {
            objectNameArgumentBlockSize = 
                objectNameArgumentBlock->Header.Size - KPHSS_ARGUMENT_BLOCK_OVERHEAD;
        }
        else
        {
            objectNameArgumentBlock = NULL;
        }
    }
    
    /* Allocate an argument block. */
    argumentBlock = KphpSsAllocateArgumentBlock(
        sizeof(KPHSS_OBJECT_ATTRIBUTES) + rootDirectoryArgumentBlockSize + objectNameArgumentBlockSize,
        ObjectAttributesArgument
        );
    
    argumentBlock->ObjectAttributes.RootDirectoryOffset = 0;
    argumentBlock->ObjectAttributes.ObjectNameOffset = 0;
    
    /* Copy the object attributes fields. */
    memcpy(
        &argumentBlock->ObjectAttributes.ObjectAttributes,
        &objectAttributes,
        sizeof(OBJECT_ATTRIBUTES)
        );
    
    /* Copy the root directory structure, if we have one. */
    if (rootDirectoryArgumentBlock)
    {
        ULONG rootDirectoryOffset;
        
        /* It will go directly after the KPHSS_OBJECT_ATTRIBUTES structure. */
        rootDirectoryOffset = sizeof(KPHSS_OBJECT_ATTRIBUTES);
        argumentBlock->ObjectAttributes.RootDirectoryOffset = (USHORT)rootDirectoryOffset;
        /* Copy it. */
        memcpy(
            PTR_ADD_OFFSET(&argumentBlock->ObjectAttributes, rootDirectoryOffset),
            &rootDirectoryArgumentBlock->Handle,
            rootDirectoryArgumentBlockSize
            );
        /* Free the block. */
        KphpSsFreeArgumentBlock(rootDirectoryArgumentBlock);
    }
    
    /* Copy the object name structure, if we have one. */
    if (objectNameArgumentBlock)
    {
        ULONG objectNameOffset;
        
        /* We'll place the structure after the root directory structure, 
         * if present.
         */
        objectNameOffset = sizeof(KPHSS_OBJECT_ATTRIBUTES) + rootDirectoryArgumentBlockSize;
        
        /* Make sure the offset isn't too large. */
        if (objectNameOffset <= MAX_USHORT)
        {
            argumentBlock->ObjectAttributes.ObjectNameOffset = (USHORT)objectNameOffset;
            /* Copy it. */
            memcpy(
                PTR_ADD_OFFSET(&argumentBlock->ObjectAttributes, objectNameOffset),
                &objectNameArgumentBlock->UnicodeString,
                objectNameArgumentBlockSize
                );
        }
        
        /* Free the block. */
        KphpSsFreeArgumentBlock(objectNameArgumentBlock);
    }
    
    *ArgumentBlock = argumentBlock;
    
    return status;
}

/* KphpSsCaptureClientIdArgument
 * 
 * Captures a CLIENT_ID argument.
 */
NTSTATUS KphpSsCaptureClientIdArgument(
    __out PKPHSS_ARGUMENT_BLOCK *ArgumentBlock,
    __in PCLIENT_ID Argument,
    __in KPROCESSOR_MODE PreviousMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    CLIENT_ID clientId;
    PKPHSS_ARGUMENT_BLOCK argumentBlock;
    
    /* Check if we have a NULL pointer. */
    if (!Argument)
        return STATUS_INVALID_PARAMETER_2;
    
    /* Probe and copy the CLIENT_ID structure. */
    __try
    {
        if (PreviousMode != KernelMode)
            ProbeForRead(Argument, sizeof(CLIENT_ID), 1);
        
        memcpy(&clientId, Argument, sizeof(CLIENT_ID));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }
    
    /* Allocate an argument block. */
    argumentBlock = KphpSsAllocateArgumentBlock(
        sizeof(CLIENT_ID),
        ClientIdArgument
        );
    
    if (!argumentBlock)
        return STATUS_INSUFFICIENT_RESOURCES;
    
    /* Fill in the argument block. */
    memcpy(&argumentBlock->ClientId, &clientId, sizeof(CLIENT_ID));
    
    *ArgumentBlock = argumentBlock;
    
    return status;
}

/* KphpSsCaptureBytesArgument
 * 
 * Captures a binary blob as an argument.
 */
NTSTATUS KphpSsCaptureBytesArgument(
    __out PKPHSS_ARGUMENT_BLOCK *ArgumentBlock,
    __in PVOID Argument,
    __in ULONG Length,
    __in KPROCESSOR_MODE PreviousMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PKPHSS_ARGUMENT_BLOCK argumentBlock;
    
    /* Check if we have a NULL pointer. */
    if (!Argument)
        return STATUS_INVALID_PARAMETER_2;
    
    /* Make sure the length isn't too big. */
    if (Length > CAPTURE_BYTES_MAX_SIZE)
        return STATUS_INVALID_PARAMETER_3;
    
    /* Probe the bytes. */
    __try
    {
        if (PreviousMode != KernelMode)
            ProbeForRead(Argument, Length, 1);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }
    
    /* Allocate an argument block. */
    argumentBlock = KphpSsAllocateArgumentBlock(
        sizeof(KPHSS_BYTES) + Length,
        BytesArgument
        );
    
    if (!argumentBlock)
        return STATUS_INSUFFICIENT_RESOURCES;
    
    /* Copy the bytes. */
    __try
    {
        memcpy(argumentBlock->Bytes.Buffer, Argument, Length);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        KphpSsFreeArgumentBlock(argumentBlock);
        return GetExceptionCode();
    }
    
    argumentBlock->Bytes.Length = (USHORT)Length;
    
    *ArgumentBlock = argumentBlock;
    
    return status;
}

/* KphpSsCreateArgumentBlock
 * 
 * Allocates and initializes an argument block.
 */
NTSTATUS KphpSsCreateArgumentBlock(
    __out PKPHSS_ARGUMENT_BLOCK *ArgumentBlock,
    __in ULONG Number,
    __in ULONG Argument,
    __in ULONG Index,
    __in_opt KPHSS_ARGUMENT_TYPE Type,
    __in_opt PVOID Context
    )
{
#ifdef _X86_
    NTSTATUS status = STATUS_SUCCESS;
    PKPHSS_ARGUMENT_BLOCK argumentBlock;
    KPROCESSOR_MODE previousMode;
    PKPHSS_CALL_ENTRY callEntry;
    KPHSS_ARGUMENT_TYPE argumentType;
    
    previousMode = ExGetPreviousMode();
    
    /* Get a pointer to the call entry for the system service. 
     * If we don't have one, we can't proceed.
     */
    callEntry = KphSsLookupCallEntry(Number);
    
    if (!callEntry)
        return STATUS_INVALID_PARAMETER_2;
    
    /* Validate the argument index. */
    if (Index >= callEntry->NumberOfArguments)
        return STATUS_INVALID_PARAMETER_3;
    
    if (Type != 0)
        argumentType = Type;
    else
        argumentType = callEntry->Arguments[Index];
    
    /* Is this a normal argument? If so, there's no point 
     * creating an argument block since the data is already 
     * in the event block.
     */
    if (argumentType == NormalArgument)
        return STATUS_UNSUCCESSFUL;
    
    /* Capture the argument. */
    
    switch (argumentType)
    {
        case Int8Argument:
        case Int16Argument:
        case Int32Argument:
        case Int64Argument:
            status = KphpSsCaptureSimpleArgument(
                &argumentBlock,
                (PVOID)Argument,
                argumentType,
                previousMode
                );
            break;
        case HandleArgument:
            status = KphpSsCaptureHandleArgument(
                &argumentBlock,
                (HANDLE)Argument,
                previousMode
                );
            break;
        case UnicodeStringArgument:
            status = KphpSsCaptureUnicodeStringArgument(
                &argumentBlock,
                (PUNICODE_STRING)Argument,
                previousMode
                );
            break;
        case ObjectAttributesArgument:
            status = KphpSsCaptureObjectAttributesArgument(
                &argumentBlock,
                (POBJECT_ATTRIBUTES)Argument,
                previousMode
                );
            break;
        case ClientIdArgument:
            status = KphpSsCaptureClientIdArgument(
                &argumentBlock,
                (PCLIENT_ID)Argument,
                previousMode
                );
            break;
        case BytesArgument:
            status = KphpSsCaptureBytesArgument(
                &argumentBlock,
                (PVOID)Argument,
                (ULONG)Context,
                previousMode
                );
            break;
        default:
            status = STATUS_NOT_IMPLEMENTED;
            break;
    }
    
    if (!NT_SUCCESS(status))
        return status;
    
    /* Put the index in. */
    argumentBlock->Index = (UCHAR)Index;
    
    *ArgumentBlock = argumentBlock;
    
    return status;
#else
    return STATUS_NOT_SUPPORTED;
#endif
}

/* KphpSsAllocateArgumentBlock
 * 
 * Allocates an argument block and initializes some fields.
 */
PKPHSS_ARGUMENT_BLOCK KphpSsAllocateArgumentBlock(
    __in ULONG InnerSize,
    __in KPHSS_ARGUMENT_TYPE Type
    )
{
    PKPHSS_ARGUMENT_BLOCK argumentBlock;
    ULONG size;
    
    size = KPHSS_ARGUMENT_BLOCK_SIZE(InnerSize);
    
    /* Make sure the size isn't too large. */
    if (size > MAX_USHORT)
        return NULL;
    
    argumentBlock = ExAllocatePoolWithTag(
        PagedPool,
        size,
        TAG_ARGUMENT_BLOCK
        );
    
    if (!argumentBlock)
        return NULL;
    
    argumentBlock->Header.Type = ArgumentBlockType;
    argumentBlock->Header.Size = (USHORT)size;
    argumentBlock->Type = Type;
    
    return argumentBlock;
}

/* KphpSsFreeArgumentBlock
 * 
 * Frees an argument block created by KphpSsCreateArgumentBlock.
 */
VOID KphpSsFreeArgumentBlock(
    __in PKPHSS_ARGUMENT_BLOCK ArgumentBlock
    )
{
    ExFreePoolWithTag(ArgumentBlock, TAG_ARGUMENT_BLOCK);
}

/* KphpSsWriteBlock
 * 
 * Writes a block into client memory.
 */
NTSTATUS KphpSsWriteBlock(
    __in PKPHSS_CLIENT_ENTRY ClientEntry,
    __in_opt PKPHSS_BLOCK_HEADER Block,
    __in KPHSS_SEQUENCE_MODE SequenceMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    LARGE_INTEGER zeroTimeout;
    KPH_ATTACH_STATE attachState;
    ULONG availableSpace;
    HANDLE dupHandleInClient = NULL;
    
    zeroTimeout.QuadPart = 0;
    
    /* Take care of the sequence mode. If it isn't 
     * NoSequence, it is effectively a way for the caller 
     * to control the buffer mutex.
     */
    if (SequenceMode == StartSequence)
    {
        ExAcquireFastMutex(&ClientEntry->BufferMutex);
        return STATUS_SUCCESS;
    }
    else if (SequenceMode == EndSequence)
    {
        ExReleaseFastMutex(&ClientEntry->BufferMutex);
        return STATUS_SUCCESS;
    }
    else
    {
        /* If we aren't manipulating the mutex, we need 
         * a block to write.
         */
        if (!Block)
            return STATUS_INVALID_PARAMETER_2;
        
        /* If we're in a sequence, don't acquire the mutex 
         * because the caller would have acquired it using 
         * StartSequence already.
         */
        if (SequenceMode != InSequence)
            ExAcquireFastMutex(&ClientEntry->BufferMutex);
    }
    
    /* Try to acquire the write semaphore. If we can't acquire 
     * it immediately, drop the block.
     */
    status = KeWaitForSingleObject(
        ClientEntry->WriteSemaphore,
        Executive,
        KernelMode,
        FALSE,
        &zeroTimeout
        );
    
    if (!KPHSS_BLOCK_SUCCESS(status))
    {
        if (status == STATUS_TIMEOUT)
        {
            dprintf("Ss: WARNING: Dropped block (server %#x).\n", ClientEntry->BufferCursor);
            ClientEntry->NumberOfBlocksDropped++;
        }
        
        goto CleanupBufferMutex;
    }
    
    availableSpace = ClientEntry->BufferSize - ClientEntry->BufferCursor;
    
    /* Blocks are recorded in a circular buffer. 
     * In the case that there is not enough space for an entire block, 
     * we will record a reset block that tells the client to reset 
     * its read cursor to 0. In the case that there is not enough 
     * space for a block header, it is implied that the client will 
     * reset its read cursor.
     */
    
    /* Check if we have enough space for a block header. */
    if (availableSpace < sizeof(KPHSS_BLOCK_HEADER))
    {
        /* Not enough space. Reset the cursor. */
        dprintf("Ss: Implicit cursor reset (server %#x).\n", ClientEntry->BufferCursor);
        ClientEntry->BufferCursor = 0;
        availableSpace = ClientEntry->BufferSize;
    }
    /* Check if we have enough space for the block. */
    else if (availableSpace < Block->Size)
    {
        KPHSS_RESET_BLOCK resetBlock;
        
        /* Not enough space for the block, but enough space 
         * for a reset block. Write the reset block and reset
         * the cursor.
         */
        resetBlock.Header.Size = sizeof(KPHSS_RESET_BLOCK);
        resetBlock.Header.Type = ResetBlockType;
        
        /* Attach to the client process and copy the block. */
        KphAttachProcess(ClientEntry->Process, &attachState);
        
        __try
        {
            memcpy(
                PTR_ADD_OFFSET(ClientEntry->BufferBase, ClientEntry->BufferCursor),
                &resetBlock,
                resetBlock.Header.Size
                );
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            KphDetachProcess(&attachState);
            status = GetExceptionCode();
            goto CleanupBufferMutex;
        }
        
        dprintf("Ss: Wrote reset block (server %#x).\n", ClientEntry->BufferCursor);
        KphDetachProcess(&attachState);
        ClientEntry->BufferCursor = 0;
        availableSpace = ClientEntry->BufferSize;
    }
    
    /* Now that we have dealt with any end-of-buffer issues, 
     * we still have to check if we have enough space for the 
     * event. We may have a huge event or the client may have a 
     * tiny buffer.
     */
    if (availableSpace < Block->Size)
    {
        dfprintf("Ss: WARNING: Insufficient buffer size (server %#x).\n", ClientEntry->BufferCursor);
        status = STATUS_BUFFER_TOO_SMALL;
        goto CleanupBufferMutex;
    }
    
    /* Time to copy the block into the buffer.
     */
    KphAttachProcess(ClientEntry->Process, &attachState);
    
    __try
    {
        memcpy(
            PTR_ADD_OFFSET(ClientEntry->BufferBase, ClientEntry->BufferCursor),
            Block,
            Block->Size
            );
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        dfprintf("Ss: ERROR: Could not write to the client buffer (server %#x)!\n", ClientEntry->BufferCursor);
        KphDetachProcess(&attachState);
        status = GetExceptionCode();
        goto CleanupBufferMutex;
    }
    
    KphDetachProcess(&attachState);
    
    /* Now that we have succesfully copied the block, we need to 
     * release the read semaphore to notify to the client that they have 
     * a block to read. We also need to advance our cursor.
     */
    
    /* May cause an exception (STATUS_SEMAPHORE_LIMIT_EXCEEDED). */
    __try
    {
        KeReleaseSemaphore(ClientEntry->ReadSemaphore, 2, 1, FALSE);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        dfprintf("Ss: ERROR: Could not release read semaphore (server %#x)!\n", ClientEntry->BufferCursor);
        status = GetExceptionCode();
        goto CleanupBufferMutex;
    }
    
    ClientEntry->BufferCursor += Block->Size;
    ClientEntry->NumberOfBlocksWritten++;
    
    dprintf("Ss: Wrote block (server %#x).\n", ClientEntry->BufferCursor);
    
CleanupBufferMutex:
    if (SequenceMode != InSequence)
        ExReleaseFastMutex(&ClientEntry->BufferMutex);
    
    return status;
}

/* KphpSsLogSystemServiceCall
 * 
 * Logs a system service.
 * 
 * WARNING: This function CANNOT make any system calls.
 * 
 * IRQL: <= APC_LEVEL
 */
VOID NTAPI KphpSsLogSystemServiceCall(
    __in ULONG Number,
    __in ULONG *Arguments,
    __in ULONG NumberOfArguments,
    __in PKSERVICE_TABLE_DESCRIPTOR ServiceTable,
    __in PKTHREAD Thread
    )
{
#ifdef _X86_
    NTSTATUS status = STATUS_SUCCESS;
    KPROCESSOR_MODE previousMode;
    PLIST_ENTRY currentListEntry;
    PKPHSS_RULESET_ENTRY ruleSetEntryArray[KPHSS_RULESET_ENTRY_LIMIT];
    ULONG ruleSetEntryCount;
    PKPHSS_EVENT_BLOCK eventBlock;
    PKPHSS_ARGUMENT_BLOCK argumentBlockArray[KPHSS_MAXIMUM_ARGUMENT_BLOCKS];
    ULONG i, j;
    
    previousMode = ExGetPreviousMode();
    /* Ignore the Thread argument. Replace it with our own. */
    Thread = KeGetCurrentThread();
    
    /* First, some checks.
     *   * We can't operate at IRQL > APC_LEVEL because 
     *     of restrictions on logging.
     *   * We can't operate on unknown service tables like the 
     *     shadow service table (yet).
     *   * We have to make sure we aren't attempting to log 
     *     a call to ZwContinue because we caused an exception 
     *     last time we were logging something. This will cause 
     *     a deadlock!
     */
    
    if (KeGetCurrentIrql() > APC_LEVEL)
        return;
    if (
        ServiceTable->Base != __KeServiceDescriptorTable->Base || 
        ServiceTable->Number != __KeServiceDescriptorTable->Number || 
        ServiceTable->Limit != __KeServiceDescriptorTable->Limit
        )
        return;
    
    /* Make sure we aren't logging ZwContinue if it's because 
     * we caused an exception somewhere. */
    if (
        ServiceTable->Base == __KeServiceDescriptorTable->Base && 
        Number == SsNtContinue && 
        NumberOfArguments == 2 && 
        previousMode == KernelMode
        )
    {
        /* "Reverse probe" the arguments. */
        if (
            (ULONG_PTR)Arguments > (ULONG_PTR)MmHighestUserAddress && 
            Arguments[0] > (ULONG_PTR)MmHighestUserAddress
            )
        {
            CONTEXT context;
            
            /* The first argument contains the context. */
            memcpy(&context, (PCONTEXT)Arguments[0], sizeof(CONTEXT));
            /* Check if the context Eip points into the KPH module.
             * If so, abort the logging.
             */
            if (
                context.Eip >= (ULONG_PTR)KphDriverObject->DriverStart && 
                context.Eip < (ULONG_PTR)KphDriverObject->DriverStart + KphDriverObject->DriverSize
                )
                return;
        }
    }
    
    /* Build the ruleset entry array by going through the ruleset 
     * list, referencing each relevant one and copying them into 
     * the local array. This we way don't hold the lock for too 
     * long.
     */
    
    KeEnterCriticalRegion();
    ExAcquirePushLockShared(&KphSsRuleSetListPushLock);
    
    currentListEntry = KphSsRuleSetListHead.Flink;
    ruleSetEntryCount = 0;
    
    while (
        currentListEntry != &KphSsRuleSetListHead && 
        ruleSetEntryCount < KPHSS_RULESET_ENTRY_LIMIT
        )
    {
        PKPHSS_RULESET_ENTRY ruleSetEntry = KPHSS_RULESET_ENTRY(currentListEntry);
        
        if (KphpSsMatchRuleSetEntry(
            ruleSetEntry,
            Number,
            Arguments,
            NumberOfArguments,
            ServiceTable,
            Thread,
            previousMode
            ))
        {
            /* Reference and store the ruleset entry in the local array. */
            if (KphReferenceObjectSafe(ruleSetEntry))
            {
                /* Make sure the client is enabled. */
                if (ruleSetEntry->Client->Enabled)
                {
                    ruleSetEntryArray[ruleSetEntryCount] = ruleSetEntry;
                    ruleSetEntryCount++;
                }
                else
                {
                    /* We need to use defer delete here because we hold the 
                     * ruleset list lock.
                     */
                    KphDereferenceObjectDeferDelete(ruleSetEntry);
                }
            }
        }
        
        currentListEntry = currentListEntry->Flink;
    }
    
    ExReleasePushLock(&KphSsRuleSetListPushLock);
    KeLeaveCriticalRegion();
    
    /* If we didn't find any ruleset entries, don't bother creating the 
     * event block.
     */
    if (ruleSetEntryCount == 0)
        return;
    
    /* We have work to do. Create an event block first. */
    if (!NT_SUCCESS(KphpSsCreateEventBlock(
        &eventBlock,
        Thread,
        Number,
        Arguments,
        NumberOfArguments
        )))
    {
        dfprintf("Ss: ERROR: Unable to create an event block!\n");
        return;
    }
    
    memset(argumentBlockArray, 0, sizeof(argumentBlockArray));
    
    /* Process specific argument blocks. */
    KphpSsProcessSpecificArguments(
        argumentBlockArray,
        Number,
        Arguments,
        NumberOfArguments,
        previousMode
        );
    
    /* Create the (generic) argument blocks. If we fail to create one, 
     * set the array entry to NULL and we'll skip it later.
     */
    
    for (i = 0; i < NumberOfArguments && i < KPHSS_MAXIMUM_ARGUMENT_BLOCKS; i++)
    {
        ULONG argument;
        
        /* If we already have a specific argument block already, skip this one. */
        if (argumentBlockArray[i])
            continue;
        
        __try
        {
            /* We'll assume the arguments have already been probed 
             * since we created the event block successfully.
             */
            argument = Arguments[i];
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            /* Silently skip this argument. Even though it is 99% likely 
             * that we will fail to read the next argument, continue 
             * anyway.
             */
            argumentBlockArray[i] = NULL;
            continue;
        }
        
        status = KphpSsCreateArgumentBlock(
            &argumentBlockArray[i],
            Number,
            argument,
            i,
            0,
            NULL
            );
        
        if (!NT_SUCCESS(status))
            argumentBlockArray[i] = NULL;
    }
    
    /* Go through the ruleset entry array and write the blocks to each 
     * client. While we're doing that we can also dereference each 
     * ruleset entry.
     */
    for (i = 0; i < ruleSetEntryCount; i++)
    {
        /* Begin a sequence. */
        status = KphpSsWriteBlock(ruleSetEntryArray[i]->Client, NULL, StartSequence);
        
        if (NT_SUCCESS(status))
        {
            /* Write the event block. */
            KphpSsWriteBlock(ruleSetEntryArray[i]->Client, &eventBlock->Header, InSequence);
            
            /* Write the argument blocks. */
            for (j = 0; j < NumberOfArguments && j < KPHSS_MAXIMUM_ARGUMENT_BLOCKS; j++)
            {
                if (argumentBlockArray[j])
                {
                    KphpSsWriteBlock(ruleSetEntryArray[i]->Client, &argumentBlockArray[j]->Header, InSequence);
                }
            }
            
            /* End the sequence. */
            KphpSsWriteBlock(ruleSetEntryArray[i]->Client, NULL, EndSequence);
        }
        
        KphDereferenceObject(ruleSetEntryArray[i]);
    }
    
    /* Free the event block. */
    KphpSsFreeEventBlock(eventBlock);
    
    /* Free the argument blocks. */
    for (i = 0; i < NumberOfArguments && i < KPHSS_MAXIMUM_ARGUMENT_BLOCKS; i++)
    {
        if (argumentBlockArray[i])
            KphpSsFreeArgumentBlock(argumentBlockArray[i]);
    }
#else
    KeBugCheck(STATUS_NOT_SUPPORTED);
#endif
}

#ifdef _X86_

/* KphpSsNewKiFastCallEntry
 * 
 * The hook function called from within the hooked KiFastCallEntry.
 */
__declspec(naked) VOID NTAPI KphpSsNewKiFastCallEntry()
{
    /* KiFastCallEntry handles system service calls. User-mode applications 
     * will perform system calls like this:
     * 
     * Nt*:
     * mov      eax, SystemServiceNumber
     * mov      edx, 0x7ffe0300 <-- at 0x7ffe0300 we have a pointer to KiFastSystemCall
     * call     [edx]
     * ret
     * 
     * At KiFastSystemCall:
     * mov      edx, esp
     * sysenter
     */
    /* This means that in KiFastCallEntry, eax will contain the system service 
     * number while edx will contain a pointer to the arguments for the system 
     * service. KiFastCallEntry will fill in edi with the service table, and 
     * esi will contain the caller KTHREAD.
     * 
     * We cannot hook KiFastCallEntry from the beginning because it starts on the DPC 
     * stack. KiFastCallEntry switches to the proper thread stack, and we want to 
     * hook it just after it switches to the stack. That way we can avoid having to 
     * manually switch the thread stack ourselves.
     * 
     * At this point: 
     *   * eax contains the system service number.
     *   * edx contains a pointer to the user-supplied arguments for 
     *     the system service.
     *   * edi contains a pointer to the service table associated with 
     *     the system service number.
     *   * esi contains a pointer to the KTHREAD of the caller.
     */
    /* Some context:
     * 
     * push     edx
     * push     eax
     * call     [_KeGdiFlushUserBatch]
     * pop      eax
     * pop      edx
     * inc      dword ptr fs:[PbSystemCalls] <-- this gets overwritten with a jmp to here
     * mov      edi, edx
     * mov      ebx, [edi+...]
     * ...
     */
    __asm
    {
        /* Save all registers first. */
        push    ebp
        push    edi
        push    esi
        push    edx
        push    ecx
        push    ebx
        push    eax
        
        /* Since we overwrite the inc instruction when we did the hook, 
         * perform the job now - we have to increment the system calls 
         * counter.
         */
        lea     ebx, KphSsKiFastCallEntryHook /* get a pointer to the hook structure */
        mov     ebx, dword ptr [ebx+KPH_HOOK.Bytes+3] /* get the PbSystemCalls offset from the original inc instruction */
        inc     dword ptr fs:[ebx] /* increment PbSystemCalls in the PRCB */
        
        /* Get the number of arguments for this system service. */
        mov     ebx, dword ptr [edi+KSERVICE_TABLE_DESCRIPTOR.Number] /* ebx = a pointer to the argument table */
        xor     ecx, ecx
        mov     cl, [ebx+eax] /* ecx = size of the arguments, in bytes. */
        shr     ecx, 2 /* divide by 2 to get the number of arguments (all ULONGs) */
        
        /* Call the KiFastCallEntry proc while maintaining the logger count 
         * so that the driver doesn't get unloaded while we're executing.
         */
        push    esi /* Thread */
        push    edi /* ServiceTable */
        push    ecx /* NumberOfArguments */
        push    edx /* Arguments */
        push    eax /* Number */
        lock inc dword ptr KphSsNumberOfActiveLoggers
        call    KphpSsLogSystemServiceCall
        lock dec dword ptr KphSsNumberOfActiveLoggers
        
        /* Restore the registers and resume execution in KiFastCallEntry. */
        pop     eax
        pop     ebx
        pop     ecx
        pop     edx
        pop     esi
        pop     edi
        pop     ebp
        
        /* Luckily, KiFastCallEntry will overwrite ebx when we jump back, so it's safe to use it. */
        lea     ebx, __KiFastCallEntry
        mov     ebx, [ebx] /* ebx = KiFastCallEntry at the inc instruction */
        add     ebx, 7 /* skip the inc instruction */
        jmp     ebx /* jump back */
    }
}

#else

VOID NTAPI KphpSsNewKiFastCallEntry()
{
    KeBugCheck(STATUS_NOT_SUPPORTED);
}

#endif
