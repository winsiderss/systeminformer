/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#include <kph.h>
#include <cid_table.h>

#include <trace.h>

static BOOLEAN KphpCidTrackingInitialized = FALSE;
static CID_TABLE KphpCidTable;
static volatile LONG KphpCidPopulated = 0;
static KEVENT KphpCidPopulatedEvent;

static UNICODE_STRING KphpProcessContextTypeName = RTL_CONSTANT_STRING(L"KphProcessContext");
static UNICODE_STRING KphpThreadContextTypeName = RTL_CONSTANT_STRING(L"KphThreadContext");

static PKPH_PAGED_LOOKASIDE_OBJECT KphpProcessContextLookaside = NULL;
static PKPH_PAGED_LOOKASIDE_OBJECT KphpThreadContextLookaside = NULL;

PKPH_OBJECT_TYPE KphProcessContextType = NULL;
PKPH_OBJECT_TYPE KphThreadContextType = NULL;

PAGED_FILE();

/**
 * \brief Marks the CID tracking populated, which unblocks public tracking APIs.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCidMarkPopulated(
    VOID
    )
{
    PAGED_PASSIVE();

    if (!KphpCidTrackingInitialized)
    {
        return;
    }

    if (InterlockedExchange(&KphpCidPopulated, 1))
    {
        return;
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  TRACKING,
                  "Marked CID tracking populated, unblocking routines.");

    KeSetEvent(&KphpCidPopulatedEvent, EVENT_INCREMENT, FALSE);
}

/**
 * \brief Waits for the CID tracking to be marked as populated.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpCidWaitForPopulate(
    VOID
    )
{
    PAGED_PASSIVE();

    if (KphpCidPopulated)
    {
        return;
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  TRACKING,
                  "Waiting for CID tracking population...");

    KeWaitForSingleObject(&KphpCidPopulatedEvent,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
}

/**
 * \brief Allocates a process context object.
 *
 * \param[in] Size The size requested from the object infrastructure.
 *
 * \return Allocated process context object, null on allocation failure.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Return_allocatesMem_size_(Size)
PVOID KSIAPI KphpAllocateProcessContext(
    _In_ SIZE_T Size
    )
{
    PVOID object;

    PAGED_PASSIVE();

    DBG_UNREFERENCED_PARAMETER(Size);
    NT_ASSERT(KphpProcessContextLookaside);
    NT_ASSERT(Size <= KphpProcessContextLookaside->L.Size);

    object = KphAllocateFromPagedLookasideObject(KphpProcessContextLookaside);
    if (object)
    {
        KphReferenceObject(KphpProcessContextLookaside);
    }

    return object;
}

/**
 * \brief Initializes a process context.
 *
 * \param[in] Object The process context object to initialize.
 * \param[in] Parameter The kernel process object associated with this context. 
 *
 * \return STATUS_SUCCESS
 */
_Function_class_(KPH_TYPE_INITIALIZE_PROCEDURE)
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpInitializeProcessContext(
    _Inout_ PVOID Object,
    _In_opt_ PVOID Parameter 
    )
{
    NTSTATUS status;
    PKPH_PROCESS_CONTEXT process;

    PAGED_CODE();

    process = Object;

    process->EProcess = Parameter;
    ObReferenceObject(process->EProcess);

    process->ProcessId = PsGetProcessId(process->EProcess);

    KphInitializeRWLock(&process->ThreadListLock);
    InitializeListHead(&process->ThreadListHead);

    KphInitializeRWLock(&process->ProtectionLock);

    status = PsReferenceProcessFilePointer(process->EProcess,
                                           &process->FileObject);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_WARNING,
                      TRACKING,
                      "PsReferenceProcessFilePointer failed: %!STATUS!",
                      status);

        process->FileObject = NULL;
    }

    status = SeLocateProcessImageName(process->EProcess,
                                      &process->ImageFileName);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_WARNING,
                      TRACKING,
                      "SeLocateProcessImageName failed: %!STATUS!",
                      status);

        process->ImageFileName = NULL;
    }

    if (process->ImageFileName)
    {
        status = KphVerifyFile(process->ImageFileName);

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      VERIFY,
                      "KphVerifyFile: %lu \"%wZ\": %!STATUS!",
                      HandleToULong(process->ProcessId),
                      process->ImageFileName,
                      status);

        if (NT_SUCCESS(status))
        {
            process->VerifiedProcess = TRUE;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Deletes a process context.
 *
 * \param[in] Object The process context object to delete.
 */
_Function_class_(KPH_TYPE_DELETE_PROCEDURE)
_IRQL_requires_max_(APC_LEVEL)
VOID KSIAPI KphpDeleteProcessContext(
    _Inout_ PVOID Object
    )
{
    PKPH_PROCESS_CONTEXT process;

    PAGED_CODE();

    process = Object;

    if (process->Protected)
    {
        KphStopProtectingProcess(process);
    }

    if (process->ImageFileName)
    {
        ExFreePool(process->ImageFileName);
    }

    if (process->FileObject)
    {
        ObDereferenceObject(process->FileObject);
    }

    KphDeleteRWLock(&process->ProtectionLock);

    NT_ASSERT(IsListEmpty(&process->ThreadListHead));
    NT_ASSERT(process->NumberOfThreads == 0);
    KphDeleteRWLock(&process->ThreadListLock);

    NT_ASSERT(process->EProcess);
    ObDereferenceObject(process->EProcess);
}

/**
 * \brief Frees a process context object.
 *
 * \param[in] Object The process context object to free.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_IRQL_requires_max_(APC_LEVEL)
VOID KSIAPI KphpFreeProcessContext(
    _In_freesMem_ PVOID Object
    )
{
    PAGED_CODE();

    NT_ASSERT(KphpProcessContextLookaside);

    KphFreeToPagedLookasideObject(KphpProcessContextLookaside, Object);
    KphDereferenceObject(KphpProcessContextLookaside);
}

/**
 * \brief Allocates a thread context object.
 *
 * \param[in] Size The size requested from the object infrastructure.
 *
 * \return Allocated thread context object, null on allocation failure.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Return_allocatesMem_size_(Size)
PVOID KSIAPI KphpAllocateThreadContext(
    _In_ SIZE_T Size
    )
{
    PVOID object;

    PAGED_PASSIVE();

    DBG_UNREFERENCED_PARAMETER(Size);
    NT_ASSERT(KphpThreadContextLookaside);
    NT_ASSERT(Size <= KphpThreadContextLookaside->L.Size);

    object = KphAllocateFromPagedLookasideObject(KphpThreadContextLookaside);
    if (object)
    {
        KphReferenceObject(KphpThreadContextLookaside);
    }

    return object;
}

/**
 * \brief Initializes a thread context.
 *
 * \param[in] Object The thread context object to initialize.
 * \param[in] Parameter Unused
 *
 * \return STATUS_SUCCESS
 */
_Function_class_(KPH_TYPE_INITIALIZE_PROCEDURE)
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpInitializeThreadContext(
    _Inout_ PVOID Object,
    _In_opt_ PVOID Parameter 
    )
{
    PKPH_THREAD_CONTEXT thread;

    PAGED_CODE();

    thread = Object;

    thread->EThread = Parameter;
    ObReferenceObject(thread->EThread);

    thread->ClientId.UniqueThread = PsGetThreadId(thread->EThread);
    thread->ClientId.UniqueProcess = PsGetThreadProcessId(thread->EThread);

    return STATUS_SUCCESS;
}

/**
 * \brief Deletes a thread context.
 *
 * \param[in] Object The thread context object to delete.
 */
_Function_class_(KPH_TYPE_DELETE_PROCEDURE)
_IRQL_requires_max_(APC_LEVEL)
VOID KSIAPI KphpDeleteThreadContext(
    _Inout_ PVOID Object
    )
{
    PKPH_THREAD_CONTEXT thread;

    PAGED_CODE();

    thread = Object;

    NT_ASSERT(!thread->InThreadList);

    NT_ASSERT(thread->EThread);
    ObDereferenceObject(thread->EThread);

    if (thread->ProcessContext)
    {
        KphDereferenceObject(thread->ProcessContext);
    }
}

/**
 * \brief Frees a thread context object.
 *
 * \param[in] Object The thread context object to free.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_IRQL_requires_max_(APC_LEVEL)
VOID KSIAPI KphpFreeThreadContext(
    _In_freesMem_ PVOID Object
    )
{
    PAGED_CODE();

    NT_ASSERT(KphpThreadContextLookaside);

    KphFreeToPagedLookasideObject(KphpThreadContextLookaside, Object);
    KphDereferenceObject(KphpThreadContextLookaside);
}

/**
 * \brief Initializes the CID tracking infrastructure.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCidInitialize(
    VOID
    )
{
    NTSTATUS status;
    KPH_OBJECT_TYPE_INFO typeInfo;

    PAGED_PASSIVE();

    status = CidTableCreate(&KphpCidTable);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    KeInitializeEvent(&KphpCidPopulatedEvent, NotificationEvent, FALSE);

    status = KphCreatePagedLookasideObject(&KphpProcessContextLookaside,
                                           KphAddObjectHeaderSize(sizeof(KPH_PROCESS_CONTEXT)),
                                           KPH_TAG_PROCESS_CONTEXT);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      TRACKING,
                      "KphCreatePagedLookasideObject failed: %!STATUS!",
                      status);

        KphpProcessContextLookaside = NULL;
        goto Exit;
    }

    status = KphCreatePagedLookasideObject(&KphpThreadContextLookaside,
                                           KphAddObjectHeaderSize(sizeof(KPH_THREAD_CONTEXT)),
                                           KPH_TAG_THREAD_CONTEXT);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      TRACKING,
                      "KphCreatePagedLookasideObject failed: %!STATUS!",
                      status);

        KphpThreadContextLookaside = NULL;
        goto Exit;
    }

    typeInfo.Allocate = KphpAllocateProcessContext;
    typeInfo.Initialize = KphpInitializeProcessContext;
    typeInfo.Delete = KphpDeleteProcessContext;
    typeInfo.Free = KphpFreeProcessContext;

    KphCreateObjectType(&KphpProcessContextTypeName,
                        &typeInfo,
                        &KphProcessContextType);

    typeInfo.Allocate = KphpAllocateThreadContext;
    typeInfo.Initialize = KphpInitializeThreadContext;
    typeInfo.Delete = KphpDeleteThreadContext;
    typeInfo.Free = KphpFreeThreadContext;

    KphCreateObjectType(&KphpThreadContextTypeName,
                        &typeInfo,
                        &KphThreadContextType);

    KphpCidTrackingInitialized = TRUE;
    status = STATUS_SUCCESS;

Exit:

    if (!NT_SUCCESS(status))
    {
        CidTableDelete(&KphpCidTable);

        if (KphpProcessContextLookaside)
        {
            KphDereferenceObject(KphpProcessContextLookaside);
            KphpProcessContextLookaside = NULL;
        }

        if (KphpThreadContextLookaside)
        {
            KphDereferenceObject(KphpThreadContextLookaside);
            KphpThreadContextLookaside = NULL;
        }
    }

    return status;
}

/**
 * \brief Cleanup callback for enumerating the CID tracking table.
 *
 * \param[in] Object The CID table entry to clean up.
 * \param[in] Parameter Unused
 * 
 * \return FALSE to continue enumerating.
 */
_Function_class_(CID_ENUMERATE_CALLBACK_EX)
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN CIDAPI KphpCidCleanupCallback(
    _In_ PCID_TABLE_ENTRY Entry,
    _In_opt_ PVOID Parameter
    )
{
    PVOID object;

    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(Parameter);

    NT_ASSERT(Entry->Object & ~CID_LOCKED_OBJECT_MASK);

    object = (PVOID)(Entry->Object & CID_LOCKED_OBJECT_MASK);

    CidAssignObject(Entry, NULL);

    if (KphGetObjectType(object) == KphProcessContextType)
    {
        PKPH_PROCESS_CONTEXT process;

        process = object;

        KphAcquireRWLockExclusive(&process->ThreadListLock);

        while (!IsListEmpty(&process->ThreadListHead))
        {
            PKPH_THREAD_CONTEXT thread;

            thread = CONTAINING_RECORD(RemoveHeadList(&process->ThreadListHead),
                                       KPH_THREAD_CONTEXT,
                                       ThreadListEntry);
            NT_ASSERT(thread->InThreadList);
            process->NumberOfThreads--;
            thread->InThreadList = FALSE;

            KphDereferenceObject(thread);
        }

        KphReleaseRWLock(&process->ThreadListLock);
    }

    KphDereferenceObject(object);

    return FALSE;
}

/**
 * \brief Cleans up the CID tracking infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCidCleanup(
    VOID
    )
{
    PAGED_PASSIVE();

    if (!KphpCidTrackingInitialized)
    {
        return;
    }

    CidEnumerateEx(&KphpCidTable, KphpCidCleanupCallback, NULL);

    CidTableDelete(&KphpCidTable);

    KphDereferenceObject(KphpThreadContextLookaside);
    KphDereferenceObject(KphpProcessContextLookaside);
}

/**
 * \brief Looks up a context object in the CID tracking.
 *
 * \param[in] Cid The CID of the object to look up.
 * \param[in] ObjectType The expected object type if the CID.
 *
 * \return Pointer to the context object, null if not found or the object is
 * not of the expected type. The caller *must* dereference the object when
 * they are through with it.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
PVOID KphpLookupContext(
    _In_ HANDLE Cid,
    _In_ PKPH_OBJECT_TYPE ObjectType
    )
{
    PVOID object;
    PCID_TABLE_ENTRY entry;

    PAGED_CODE();

    entry = CidGetEntry(Cid, &KphpCidTable);
    if (!entry)
    {
        return NULL;
    }

    CidAcquireObjectLock(entry);

    object = (PVOID)(entry->Object & CID_LOCKED_OBJECT_MASK);

    if (object)
    {
        KphReferenceObject(object);
    }

    CidReleaseObjectLock(entry);

    if (object && (KphGetObjectType(object) != ObjectType))
    {
        KphDereferenceObject(object);
        object = NULL;
    }

    return object;
}

/**
 * \brief Begins tracking a context object in the CID tracking. May return an
 * existing object if it is already being tracked.
 *
 * \param[in] Cid The CID of the object to being tracking.
 * \param[in] ObjectType The expected object type if the CID.
 * \param[in] ObjectBodySize The size of the context body.
 * \param[in] Parameter The parameter passed to the creation routine.
 *
 * \return Pointer to the context object, null on allocation failure or if the
 * object is already being tracked and is not of the expected type. The caller
 * *must* dereference the object when they are through with it.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
PVOID KphpTrackContext(
    _In_ HANDLE Cid,
    _In_ PKPH_OBJECT_TYPE ObjectType,
    _In_ ULONG ObjectBodySize,
    _In_ PVOID Parameter
    )
{
    NTSTATUS status;
    PCID_TABLE_ENTRY entry;
    PVOID object;

    PAGED_PASSIVE();

    entry = CidGetEntry(Cid, &KphpCidTable);
    if (!entry)
    {
        return NULL;
    }

    CidAcquireObjectLock(entry);

    object = (PVOID)(entry->Object & CID_LOCKED_OBJECT_MASK);

    if (object)
    {
        if (KphGetObjectType(object) == ObjectType)
        {
            KphReferenceObject(object);
        }
        else
        {
            object = NULL;
        }

        goto Exit;
    }

    status = KphCreateObject(ObjectType, ObjectBodySize, &object, Parameter);
    if (!NT_SUCCESS(status))
    {
        object = NULL;
        goto Exit;
    }

    //
    // We reference in the table and return a reference to the caller.
    // Object creation gives one reference, this is the caller reference.
    //
    KphReferenceObject(object);
    CidAssignObject(entry, object);

Exit:

    CidReleaseObjectLock(entry);

    return object;
}

/**
 * \brief Stops tracking a context object in the CID tracking.
 *
 * \param[in] Cid The CID of the object to being tracking.
 * \param[in] ObjectType The expected object type if the CID.
 * \param[in] ObjectBodySize The size of the context body.
 *
 * \return Pointer to the context object, null if not found or the object is
 * not of the expected type. The caller *must* dereference the object when they
 * are through with it.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
PVOID KphpUntrackContext(
    _In_ HANDLE Cid,
    _In_ PKPH_OBJECT_TYPE ObjectType
    )
{
    PCID_TABLE_ENTRY entry;
    PVOID object;

    PAGED_PASSIVE();

    entry = CidGetEntry(Cid, &KphpCidTable);
    if (!entry)
    {
        return NULL;
    }

    CidAcquireObjectLock(entry);

    object = (PVOID)(entry->Object & CID_LOCKED_OBJECT_MASK);

    if (!object)
    {
        goto Exit;
    }

    if (KphGetObjectType(object) != ObjectType)
    {
        object = NULL;
        goto Exit;
    }

    //
    // We do not release the in-table reference here. The caller is given
    // ownership over the reference, they are responsible for releasing it. 
    //
    CidAssignObject(entry, NULL);

Exit:

    CidReleaseObjectLock(entry);

    return object;
}

/**
 * \brief Performs actions post population of CID table.
 *
 * \param[in] Process The context being enumerated.
 * \param[in] Parameter Unused.
 *
 * \return FALSE
 */
_Function_class_(KPH_ENUM_CID_CONTEXTS_CALLBACK)
_Must_inspect_result_
BOOLEAN CIDAPI KphpCidEnumPostPopulate(
    _In_ PVOID Context,
    _In_opt_ PVOID Parameter
    )
{
    NTSTATUS status;
    PKPH_OBJECT_TYPE objectType;
    PKPH_PROCESS_CONTEXT process;
    KPH_PROCESS_STATE processState;

    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(Parameter);

    objectType = KphGetObjectType(Context);

    if (objectType == KphThreadContextType)
    {
        return FALSE;
    }

    NT_ASSERT(objectType == KphProcessContextType);

    process = Context;

    processState = KphGetProcessState(process);
    if ((processState & KPH_PROCESS_STATE_LOW) == KPH_PROCESS_STATE_LOW)
    {
        status = KphStartProtectingProcess(process,
                                           KPH_PROTECED_PROCESS_MASK,
                                           KPH_PROTECED_THREAD_MASK);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          PROTECTION,
                          "KphStartProtectingProcess failed: %!STATUS!",
                          status);

            NT_ASSERT(!process->Protected);
        }
    }

    return FALSE;
}

// from phnative.h
#define KPH_FIRST_PROCESS(Processes) ((PSYSTEM_PROCESS_INFORMATION)(Processes))
#define KPH_NEXT_PROCESS(Process) ( \
    ((PSYSTEM_PROCESS_INFORMATION)(Process))->NextEntryOffset ? \
    (PSYSTEM_PROCESS_INFORMATION)Add2Ptr((Process), \
    ((PSYSTEM_PROCESS_INFORMATION)(Process))->NextEntryOffset) : \
    NULL \
    )

/**
 * \brief Populates the CID tracking.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCidPopulate(
    VOID
    )
{
    NTSTATUS status;
    ULONG size;
    PVOID buffer;
    PSYSTEM_PROCESS_INFORMATION info;

    PAGED_PASSIVE();

    size = (PAGE_SIZE * 4);
    buffer = KphAllocatePaged(size, KPH_TAG_CID_POPULATE);
    if (!buffer)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      TRACKING,
                      "Failed to allocate buffer for process information.");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    for (;;)
    {
        status = ZwQuerySystemInformation(SystemProcessInformation,
                                          buffer,
                                          size,
                                          &size);
        if (NT_SUCCESS(status))
        {
            break;
        }

        if ((status != STATUS_BUFFER_TOO_SMALL) &&
            (status != STATUS_INFO_LENGTH_MISMATCH))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          TRACKING,
                          "ZwQuerySystemInformation failed: %!STATUS!",
                          status);

            goto Exit;
        }

        if (size == 0)
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          TRACKING,
                          "ZwQuerySystemInformation returned zero size!");

            NT_ASSERT(!NT_SUCCESS(status));
            goto Exit;
        }

        KphFree(buffer, KPH_TAG_CID_POPULATE);
        buffer = KphAllocatePaged(size, KPH_TAG_CID_POPULATE);
        if (!buffer)
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          TRACKING,
                          "Failed to allocate buffer for process information.");

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "Trying again with larger buffer (%lu)...",
                      size);
    }

    //
    // N.B. Because of the way we start up we guarantee the thread and process
    // notifications routines are blocked until we complete this work. This
    // gives us a guarantee that a new process/thread can't be fully created or
    // tracked before we get here. However, this is still the possibility of
    // a TOCTOU for an exiting process/thread. We need to take care here during
    // initialization. If the process/thread is already exited we should not 
    // track it.
    //

    NT_ASSERT(NT_SUCCESS(status));
    NT_ASSERT(size >= sizeof(SYSTEM_PROCESS_INFORMATION));

    for (info = KPH_FIRST_PROCESS(buffer); info; info = KPH_NEXT_PROCESS(info))
    {
        PKPH_PROCESS_CONTEXT process;
        PEPROCESS processObject;

        if (!info->UniqueProcessId)
        {
            //
            // Idle Process.
            // For now we aren't going to track it. There are "valid" watchdog 
            // threads in the idle process, but we can't look up their thread 
            // objects in a documented way. For simplicity, for now, we opt
            // not to track it.
            //
            continue;
        }

        if (info->UniqueProcessId == PsGetProcessId(PsInitialSystemProcess))
        {
            //
            // System Process
            //
            processObject = PsInitialSystemProcess;
            ObReferenceObject(processObject);
        }
        else
        {
            //
            // Check if we should track this process during population.
            // Ultimately here we're ensuring the process isn't already exited.
            // Otherwise there is the possibility of an object leak from TOCTOU.
            //

            status = PsLookupProcessByProcessId(info->UniqueProcessId,
                                                &processObject);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_WARNING,
                              TRACKING,
                              "PsLookupProcessByProcessId failed: %!STATUS!",
                              status);

                continue;
            }

            status = PsAcquireProcessExitSynchronization(processObject);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_WARNING,
                              TRACKING,
                              "PsAcquireProcessExitSynchronization failed: %!STATUS!",
                              status);


                ObDereferenceObject(processObject);
                continue;
            }

            status = PsGetProcessExitStatus(processObject);

            PsReleaseProcessExitSynchronization(processObject);

            if (status != STATUS_PENDING)
            {
                KphTracePrint(TRACE_LEVEL_WARNING,
                              TRACKING,
                              "PsGetProcessExitStatus reported: %!STATUS!",
                              status);

                ObDereferenceObject(processObject);
                continue;
            }
        }

        process = KphpTrackContext(info->UniqueProcessId,
                                   KphProcessContextType,
                                   sizeof(KPH_PROCESS_CONTEXT),
                                   processObject);

        ObDereferenceObject(processObject);
        processObject = NULL;

        if (!process)
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          TRACKING,
                          "KphpTrackContext failed: %!STATUS!",
                          status);

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        process->CreatorClientId.UniqueProcess = NtCurrentProcess();
        process->CreatorClientId.UniqueThread = NtCurrentThread();

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "Tracking process %lu",
                      HandleToULong(process->ProcessId));

        for (ULONG i = 0; i < info->NumberOfThreads; i++)
        {
            PKPH_THREAD_CONTEXT thread;
            PETHREAD threadObject;

            status = PsLookupThreadByThreadId(info->Threads[i].ClientId.UniqueThread,
                                              &threadObject);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_WARNING,
                              TRACKING,
                              "PsLookupThreadByThreadId failed: %!STATUS!",
                              status);

                continue;
            }

            status = KphGetThreadExitStatus(threadObject);
            if (status != STATUS_PENDING)
            {
                KphTracePrint(TRACE_LEVEL_WARNING,
                              TRACKING,
                              "KphGetThreadExitStatus reported: %!STATUS!",
                              status);

                ObDereferenceObject(threadObject);
                continue;
            }

            thread = KphpTrackContext(info->Threads[i].ClientId.UniqueThread,
                                      KphThreadContextType,
                                      sizeof(KPH_THREAD_CONTEXT),
                                      threadObject);

            ObDereferenceObject(threadObject);
            threadObject = NULL;

            if (!thread)
            {
                KphTracePrint(TRACE_LEVEL_ERROR,
                              TRACKING,
                              "KphpTrackContext failed: %!STATUS!",
                              status);

                KphDereferenceObject(process);
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
            }

            thread->CreatorClientId.UniqueProcess = NtCurrentProcess();
            thread->CreatorClientId.UniqueThread = NtCurrentThread();

            thread->ProcessContext = process;
            KphReferenceObject(thread->ProcessContext);

            KphAcquireRWLockExclusive(&process->ThreadListLock);

            NT_ASSERT(!thread->InThreadList);
            InsertTailList(&process->ThreadListHead, &thread->ThreadListEntry);
            process->NumberOfThreads++;
            thread->InThreadList = TRUE;

            KphReferenceObject(thread);

            KphReleaseRWLock(&process->ThreadListLock);

            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          TRACKING,
                          "Tracking thread %lu in process %lu",
                          HandleToULong(thread->ClientId.UniqueThread),
                          HandleToULong(thread->ClientId.UniqueProcess));

            KphDereferenceObject(thread);
        }

        KphDereferenceObject(process);
    }

Exit:

    KphCidMarkPopulated();

    if (buffer)
    {
        KphFree(buffer, KPH_TAG_CID_POPULATE);
    }

    if (NT_SUCCESS(status))
    {
        KphEnumerateCidContexts(KphpCidEnumPostPopulate, NULL);
    }

    return status;
}

/**
 * \brief Retrieves a process context.
 *
 * \param[in] ProcessId The process ID of the process to get the context for.
 *
 * \return Pointer to the process context, null if not found. The caller
 * *must* dereference the object when they are through with it.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
PKPH_PROCESS_CONTEXT KphGetProcessContext(
    _In_ HANDLE ProcessId
    )
{
    PAGED_CODE();
    
    return KphpLookupContext(ProcessId, KphProcessContextType);
}

/**
 * \brief Retrieves a thread context.
 *
 * \param[in] ThreadId The thread ID of the thread to get the context for.
 *
 * \return Pointer to the thread context, null if not found. The caller
 * *must* dereference the object when they are through with it.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
PKPH_THREAD_CONTEXT KphGetThreadContext(
    _In_ HANDLE ThreadId 
    )
{
    PAGED_CODE();

    return KphpLookupContext(ThreadId, KphThreadContextType);
}

/**
 * \brief Tracks a process context.
 *
 * \param[in] Process The process to start tracking.
 *
 * \return Pointer to the process context, null on allocation failure. May
 * return an existing process context if the process is already tracked. The
 * caller *must* dereference the object when they are through with it.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
PKPH_PROCESS_CONTEXT KphTrackProcessContext(
    _In_ PEPROCESS Process
    )
{
    PAGED_PASSIVE();

    KphpCidWaitForPopulate();

    return KphpTrackContext(PsGetProcessId(Process),
                           KphProcessContextType,
                           sizeof(KPH_PROCESS_CONTEXT),
                           Process);
}

/**
 * \brief Stops tracking a process context.
 *
 * \param[in] ProcessId The process ID of the process stop tracking.
 *
 * \return Pointer to the process context, null if not found. The caller *must*
 * dereference the object when they are through with it.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
PKPH_PROCESS_CONTEXT KphUntrackProcessContext(
    _In_ HANDLE ProcessId
    )
{
    PAGED_PASSIVE();

    KphpCidWaitForPopulate();

    return KphpUntrackContext(ProcessId, KphProcessContextType);
}

/**
 * \brief Tracks a thread context.
 *
 * \param[in] Thread The thread to start tracking.
 *
 * \return Pointer to the thread context, null on allocation failure. May
 * return an existing thread context if the thread is already tracked. The
 * caller *must* dereference the object when they are through with it.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
PKPH_THREAD_CONTEXT KphTrackThreadContext(
    _In_ PETHREAD Thread
    )
{
    PAGED_PASSIVE();

    KphpCidWaitForPopulate();

    return KphpTrackContext(PsGetThreadId(Thread),
                           KphThreadContextType,
                           sizeof(KPH_THREAD_CONTEXT),
                           Thread);
}

/**
 * \brief Stops tracking a thread context.
 *
 * \param[in] ThreadId The thread ID of the thread stop tracking.
 *
 * \return Pointer to the thread context, null if not found. The caller *must*
 * dereference the object when they are through with it.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
PKPH_THREAD_CONTEXT KphUntrackThreadContext(
    _In_ HANDLE ThreadId
    )
{
    PAGED_PASSIVE();

    KphpCidWaitForPopulate();

    return KphpUntrackContext(ThreadId, KphThreadContextType);
}

typedef struct _KPH_ENUM_CONTEXT
{
    PKPH_ENUM_CID_CONTEXTS_CALLBACK CidCallback;
    PKPH_ENUM_PROCESS_CONTEXTS_CALLBACK ProcessCallback;
    PKPH_ENUM_THREAD_CONTEXTS_CALLBACK ThreadCallback;
    PVOID Parameter;

} KPH_ENUM_CONTEXT, *PKPH_ENUM_CONTEXT;

/**
 * \brief Enumeration callback.
 *
 * \param[in] Object The object from the enumeration.
 * \param[in] Parameter The enumeration context from the caller.
 *
 * \return FALSE to keep enumerating if the object type is not what was asked
 * for or the return value from callers callback.
 */
_Function_class_(CID_ENUMERATE_CALLBACK)
BOOLEAN KSIAPI KphpEnumerateContexts(
    _In_ PVOID Object,
    _In_opt_ PVOID Parameter
    )
{
    PKPH_ENUM_CONTEXT context;
    PKPH_OBJECT_TYPE objectType;

    PAGED_CODE();

    NT_ASSERT(Parameter);

    context = Parameter;
    objectType = KphGetObjectType(Object);

    if (context->CidCallback)
    {
        NT_ASSERT((objectType == KphProcessContextType) ||
                  (objectType == KphThreadContextType));
        NT_ASSERT(!context->ProcessCallback);
        NT_ASSERT(!context->ThreadCallback);

        return context->CidCallback(Object, context->Parameter);
    }

    if (context->ProcessCallback)
    {
        NT_ASSERT(!context->CidCallback);
        NT_ASSERT(!context->ThreadCallback);

        if (objectType != KphProcessContextType)
        {
            return FALSE;
        }

        return context->ProcessCallback(Object, context->Parameter);
    }

    NT_ASSERT(!context->CidCallback);
    NT_ASSERT(!context->ProcessCallback);
    NT_ASSERT(context->ThreadCallback);

    if (objectType != KphThreadContextType)
    {
        return FALSE;
    }

    return context->ThreadCallback(Object, context->Parameter);
}

/**
 * \brief Enumerates process contexts.
 *
 * \param[in] Callback The callback to invoke for each process context. The
 * callback should return TRUE to stop enumerating or FALSE to continue.
 * \param[in] Parameter Optional parameter passed to the callback.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphEnumerateProcessContexts(
    _In_ PKPH_ENUM_PROCESS_CONTEXTS_CALLBACK Callback,
    _In_opt_ PVOID Parameter
    )
{
    KPH_ENUM_CONTEXT context;

    PAGED_CODE();

    context.CidCallback = NULL;
    context.ProcessCallback = Callback;
    context.ThreadCallback = NULL;
    context.Parameter = Parameter;

    CidEnumerate(&KphpCidTable, KphpEnumerateContexts, &context);
}

/**
 * \brief Enumerates thread contexts.
 *
 * \param[in] Callback The callback to invoke for each thread context. The
 * callback should return TRUE to stop enumerating or FALSE to continue.
 * \param[in] Parameter Optional parameter passed to the callback.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphEnumerateThreadContexts(
    _In_ PKPH_ENUM_THREAD_CONTEXTS_CALLBACK Callback,
    _In_opt_ PVOID Parameter
    )
{
    KPH_ENUM_CONTEXT context;

    PAGED_CODE();

    context.CidCallback = NULL;
    context.ProcessCallback = NULL;
    context.ThreadCallback = Callback;
    context.Parameter = Parameter;

    CidEnumerate(&KphpCidTable, KphpEnumerateContexts, &context);
}

/**
 * \brief Enumerates CID contexts.
 *
 * \param[in] Callback The callback to invoke for each CID context. The
 * callback should return TRUE to stop enumerating or FALSE to continue.
 * \param[in] Parameter Optional parameter passed to the callback.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphEnumerateCidContexts(
    _In_ KPH_ENUM_CID_CONTEXTS_CALLBACK Callback,
    _In_opt_ PVOID Parameter
    )
{
    KPH_ENUM_CONTEXT context;

    PAGED_CODE();

    context.CidCallback = Callback;
    context.ProcessCallback = NULL;
    context.ThreadCallback = NULL;
    context.Parameter = Parameter;

    CidEnumerate(&KphpCidTable, KphpEnumerateContexts, &context);
}
