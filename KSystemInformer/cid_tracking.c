/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2023
 *
 */

#include <kph.h>
#include <cid_table.h>
#include <dyndata.h>

#include <trace.h>

static BOOLEAN KphpCidTrackingInitialized = FALSE;
static CID_TABLE KphpCidTable;
static volatile LONG KphpCidPopulated = 0;
static KEVENT KphpCidPopulatedEvent;

typedef struct _KPH_CID_APC
{
    KSI_KAPC Apc;
    PKPH_THREAD_CONTEXT Thread;
} KPH_CID_APC, *PKPH_CID_APC;

static PKPH_NPAGED_LOOKASIDE_OBJECT KphpCidApcLookaside = NULL;

static UNICODE_STRING KphpProcessContextTypeName = RTL_CONSTANT_STRING(L"KphProcessContext");
static UNICODE_STRING KphpThreadContextTypeName = RTL_CONSTANT_STRING(L"KphThreadContext");

static PKPH_PAGED_LOOKASIDE_OBJECT KphpProcessContextLookaside = NULL;
static PKPH_PAGED_LOOKASIDE_OBJECT KphpThreadContextLookaside = NULL;

PKPH_OBJECT_TYPE KphProcessContextType = NULL;
PKPH_OBJECT_TYPE KphThreadContextType = NULL;

static volatile LONG KphpLsassIsKnown = 0;

PAGED_FILE();

/**
 * \brief Marks the CID tracking populated, which unblocks public tracking APIs.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCidMarkPopulated(
    VOID
    )
{
    PAGED_CODE_PASSIVE();

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
    PAGED_CODE_PASSIVE();

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
 * \brief Allocates the CID APC.
 *
 * \return Pointer to CID APC, null on failure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Return_allocatesMem_
PKPH_CID_APC KphpAllocateCidApc(
    VOID
    )
{
    PKPH_CID_APC apc;

    PAGED_CODE_PASSIVE();

    NT_ASSERT(KphpCidApcLookaside);

    apc = KphAllocateFromNPagedLookasideObject(KphpCidApcLookaside);
    if (apc)
    {
        KphReferenceObject(KphpCidApcLookaside);
    }

    return apc;
}

/**
 * \brief Frees a previously allocated the CID APC.
 *
 * \param[in] Apc The CID APC to free.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphpFreeCidApc(
    _In_freesMem_ PKPH_CID_APC Apc
    )
{
    PAGED_CODE();

    NT_ASSERT(KphpCidApcLookaside);

    if (Apc->Thread)
    {
        KphDereferenceObject(Apc->Thread);
    }

    KphFreeToNPagedLookasideObject(KphpCidApcLookaside, Apc);
    KphDereferenceObject(KphpCidApcLookaside);
}

/**
 * \brief APC cleanup routine for APC APCs.
 *
 * \param[in] Apc The ACP to clean up.
 * \param[in] Reason Unused.
 */
_Function_class_(KSI_KCLEANUP_ROUTINE)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_same_
VOID KSIAPI KphpCidApcCleanup(
    _In_ PKSI_KAPC Apc,
    _In_ KSI_KAPC_CLEANUP_REASON Reason
    )
{
    PKPH_CID_APC apc;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Apc);
    DBG_UNREFERENCED_PARAMETER(Reason);

    apc = CONTAINING_RECORD(Apc, KPH_CID_APC, Apc);

    KphpFreeCidApc(apc);
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

    PAGED_CODE_PASSIVE();

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
    PEPROCESS processObject;
    HANDLE processHandle;
    PROCESS_EXTENDED_BASIC_INFORMATION basicInfo;

    PAGED_CODE();

    process = Object;
    processObject = Parameter;

    status = ObOpenObjectByPointer(processObject,
                                   OBJ_KERNEL_HANDLE,
                                   NULL,
                                   PROCESS_ALL_ACCESS,
                                   *PsProcessType,
                                   KernelMode,
                                   &processHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      TRACKING,
                      "ObOpenObjectByPointer failed: %!STATUS!",
                      status);

        processHandle = NULL;
        goto Exit;
    }

    status = ZwQueryInformationProcess(processHandle,
                                       ProcessSubsystemInformation,
                                       &process->SubsystemType,
                                       sizeof(SUBSYSTEM_INFORMATION_TYPE),
                                       NULL);
    if (status == STATUS_INVALID_INFO_CLASS)
    {
        process->SubsystemType = SubsystemInformationTypeWin32;
    }
    else if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      TRACKING,
                      "ProcessSubsystemInformation failed: %!STATUS!",
                      status);
        goto Exit;
    }

    basicInfo.Size = sizeof(PROCESS_EXTENDED_BASIC_INFORMATION);
    status = ZwQueryInformationProcess(processHandle,
                                       ProcessBasicInformation,
                                       &basicInfo,
                                       sizeof(PROCESS_EXTENDED_BASIC_INFORMATION),
                                       NULL);
    if (status == STATUS_INVALID_INFO_CLASS)
    {
        process->IsSubsystemProcess = FALSE;
    }
    else if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      TRACKING,
                      "ProcessBasicInformation failed: %!STATUS!",
                      status);
        goto Exit;
    }
    else
    {
        process->IsSubsystemProcess = basicInfo.IsSubsystemProcess;
    }

    process->EProcess = processObject;
    ObReferenceObject(process->EProcess);

    process->ProcessId = PsGetProcessId(process->EProcess);

#if _WIN64
    if (PsGetProcessWow64Process(process->EProcess))
    {
        process->IsWow64 = TRUE;
    }
    else
#endif
    {
        process->IsWow64 = FALSE;
    }

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
    if (NT_SUCCESS(status))
    {
        status = KphGetFileNameFinalComponent(process->ImageFileName,
                                              &process->ImageName);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_WARNING,
                          TRACKING,
                          "KphGetFileNameFinalComponent failed: %!STATUS!",
                          status);

            NT_ASSERT(process->ImageName.Length == 0);
        }
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_WARNING,
                      TRACKING,
                      "SeLocateProcessImageName failed: %!STATUS!",
                      status);

        process->ImageFileName = NULL;
    }

    if (process->ImageName.Length == 0)
    {
        status = KphGetProcessImageName(process->EProcess,
                                        &process->ImageName);
        if (NT_SUCCESS(status))
        {
            process->AllocatedImageName = TRUE;
        }
        else
        {
            KphTracePrint(TRACE_LEVEL_WARNING,
                          TRACKING,
                          "KphGetProcessImageName failed: %!STATUS!",
                          status);

            NT_ASSERT(process->ImageName.Length == 0);
        }
    }

    if (!KphpLsassIsKnown && KphProcessIsLsass(process->EProcess))
    {
        InterlockedExchange(&KphpLsassIsKnown, 1);
        process->IsLsass = TRUE;
    }

    status = STATUS_SUCCESS;

Exit:

    if (processHandle)
    {
        ObCloseHandle(processHandle, KernelMode);
    }

    return status;
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

    if (process->IsLsass)
    {
        InterlockedExchange(&KphpLsassIsKnown, 0);
    }

    if (process->Protected)
    {
        KphStopProtectingProcess(process);
    }

    if (process->ImageFileName)
    {
#pragma warning(suppress: 4995) // intentional use of ExFreePool
        ExFreePool(process->ImageFileName);
    }

    if (process->AllocatedImageName)
    {
        KphFreeProcessImageName(&process->ImageName);
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

    PAGED_CODE_PASSIVE();

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
 * \brief Preforms thread context initialization for a WSL thread.
 *
 * \param[in] ThreadContext The thread context to initialize.
 */
_IRQL_requires_(APC_LEVEL)
VOID KphpInitializeWSLThreadContext(
    _In_ PKPH_THREAD_CONTEXT ThreadContext
    )
{
    PVOID picoContext;
    PVOID value;

    PAGED_CODE();

    NT_ASSERT(ThreadContext->EThread == KeGetCurrentThread());
    NT_ASSERT(KphDynLxpThreadGetCurrent);
    NT_ASSERT(KphDynLxPicoProc != ULONG_MAX);
    NT_ASSERT(KphDynLxPicoProcInfo != ULONG_MAX);
    NT_ASSERT(KphDynLxPicoProcInfoPID != ULONG_MAX);
    NT_ASSERT(KphDynLxPicoThrdInfo != ULONG_MAX);
    NT_ASSERT(KphDynLxPicoThrdInfoTID != ULONG_MAX);
    NT_ASSERT(ThreadContext->SubsystemType == SubsystemInformationTypeWSL);

    ThreadContext->InitApcExecuted = TRUE;

    if (!KphDynLxpThreadGetCurrent(&picoContext))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      TRACKING,
                      "LxpThreadGetCurrent failed");
        return;
    }

    NT_ASSERT(!ThreadContext->WSL.ValidThreadId);

    value = *(PVOID*)Add2Ptr(picoContext, KphDynLxPicoThrdInfo);
    ThreadContext->WSL.ThreadId =
        *(PULONG)Add2Ptr(value, KphDynLxPicoThrdInfoTID);
    ThreadContext->WSL.ValidThreadId = TRUE;

    if (!ThreadContext->ProcessContext->WSL.ValidProcessId)
    {
        value = *(PVOID*)Add2Ptr(picoContext, KphDynLxPicoProc);
        value = *(PVOID*)Add2Ptr(value, KphDynLxPicoProcInfo);
        ThreadContext->ProcessContext->WSL.ProcessId =
            *(PULONG)Add2Ptr(value, KphDynLxPicoProcInfoPID);
        ThreadContext->ProcessContext->WSL.ValidProcessId = TRUE;
    }
}

/**
 * \brief APC routine for thread tracking.
 *
 * \param[in] Apc The ACP executed, contained within the CID APC.
 * \param[in] NormalRoutine Unused.
 * \param[in] NormalContext Unused.
 * \param[in] SystemArgument1 Unused.
 * \param[in] SystemArgument2 Unused.
 */
_Function_class_(KSI_KKERNEL_ROUTINE)
_IRQL_requires_(APC_LEVEL)
_IRQL_requires_same_
VOID KSIAPI KphpInitializeThreadContextSpecialApc(
    _In_ PKSI_KAPC Apc,
    _Inout_ _Deref_pre_maybenull_ PKNORMAL_ROUTINE* NormalRoutine,
    _Inout_ _Deref_pre_maybenull_ PVOID* NormalContext,
    _Inout_ _Deref_pre_maybenull_ PVOID* SystemArgument1,
    _Inout_ _Deref_pre_maybenull_ PVOID* SystemArgument2
    )
{
    PKPH_CID_APC apc;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(NormalRoutine);
    UNREFERENCED_PARAMETER(NormalContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    apc = CONTAINING_RECORD(Apc, KPH_CID_APC, Apc);

    NT_ASSERT(apc->Thread->EThread == PsGetCurrentThread());

    if (KphDynLxpThreadGetCurrent &&
        (KphDynLxPicoProc != ULONG_MAX) &&
        (KphDynLxPicoProcInfo != ULONG_MAX) &&
        (KphDynLxPicoProcInfoPID != ULONG_MAX) &&
        (KphDynLxPicoThrdInfo != ULONG_MAX) &&
        (KphDynLxPicoThrdInfoTID != ULONG_MAX) &&
        (apc->Thread->SubsystemType == SubsystemInformationTypeWSL))
    {
        KphpInitializeWSLThreadContext(apc->Thread);
    }
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
    NTSTATUS status;
    PKPH_THREAD_CONTEXT thread;
    PETHREAD threadObject;
    HANDLE threadHandle;
    BOOLEAN needsApc;
    PKPH_CID_APC apc;

    PAGED_CODE();

    thread = Object;
    threadObject = Parameter;

    status = ObOpenObjectByPointer(threadObject,
                                   OBJ_KERNEL_HANDLE,
                                   NULL,
                                   THREAD_ALL_ACCESS,
                                   *PsThreadType,
                                   KernelMode,
                                   &threadHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      TRACKING,
                      "ObOpenObjectByPointer failed %!STATUS!",
                      status);

        threadHandle = NULL;
        goto Exit;
    }

    status = ZwQueryInformationThread(threadHandle,
                                      ThreadSubsystemInformation,
                                      &thread->SubsystemType,
                                      sizeof(SUBSYSTEM_INFORMATION_TYPE),
                                      NULL);
    if (status == STATUS_INVALID_INFO_CLASS)
    {
        thread->SubsystemType = SubsystemInformationTypeWin32;
    }
    else if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      TRACKING,
                      "ThreadSubsystemInformation failed %!STATUS!",
                      status);
        goto Exit;
    }

    thread->EThread = threadObject;
    ObReferenceObject(thread->EThread);

    thread->ClientId.UniqueThread = PsGetThreadId(thread->EThread);
    thread->ClientId.UniqueProcess = PsGetThreadProcessId(thread->EThread);

    NT_ASSERT(!thread->ProcessContext);
    thread->ProcessContext = KphGetProcessContext(thread->ClientId.UniqueProcess);

    if (thread->ProcessContext)
    {
        KphAcquireRWLockExclusive(&thread->ProcessContext->ThreadListLock);

        if (!thread->ProcessContext->InitialThread)
        {
            thread->ProcessContext->InitialThread = thread;
            KphReferenceObject(thread);
        }

        InsertTailList(&thread->ProcessContext->ThreadListHead,
                       &thread->ThreadListEntry);
        thread->ProcessContext->NumberOfThreads++;
        thread->InThreadList = TRUE;
        KphReferenceObject(thread);

        KphReleaseRWLock(&thread->ProcessContext->ThreadListLock);
    }

    status = STATUS_SUCCESS;
    needsApc = FALSE;

    if (KphDynLxpThreadGetCurrent &&
        (KphDynLxPicoProc != ULONG_MAX) &&
        (KphDynLxPicoProcInfo != ULONG_MAX) &&
        (KphDynLxPicoProcInfoPID != ULONG_MAX) &&
        (KphDynLxPicoThrdInfo != ULONG_MAX) &&
        (KphDynLxPicoThrdInfoTID != ULONG_MAX) &&
        (thread->SubsystemType == SubsystemInformationTypeWSL))
    {
        //
        // We use an APC here to reach into the thread pico context. We could
        // reach directly into the pico context here, but reversing shows
        // intent for possible other pico subsystem providers in the future.
        // So, we use some "undocumented" APIs in the APC to ask "nicely" for
        // the correct pico context.
        //
        needsApc = TRUE;
    }

    if (!needsApc)
    {
        goto Exit;
    }

    //
    // Thread needs a APC to finish initialization within the original thread.
    // If this APC fails to be queued, it isn't errant for initialization.
    //

    apc = KphpAllocateCidApc();
    if (!apc)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      TRACKING,
                      "KphpAllocateCidApc failed");
        goto Exit;
    }

    apc->Thread = thread;
    KphReferenceObject(apc->Thread);

    KsiInitializeApc(&apc->Apc,
                     KphDriverObject,
                     thread->EThread,
                     OriginalApcEnvironment,
                     KphpInitializeThreadContextSpecialApc,
                     KphpCidApcCleanup,
                     NULL,
                     KernelMode,
                     NULL);
    if (!KsiInsertQueueApc(&apc->Apc, NULL, NULL, IO_NO_INCREMENT))
    {
        KphTracePrint(TRACE_LEVEL_ERROR, TRACKING, "KsiInsertQueueApc failed");
        KphpFreeCidApc(apc);
        goto Exit;
    }

    thread->InitApcQueued = TRUE;

Exit:

    if (threadHandle)
    {
        ObCloseHandle(threadHandle, KernelMode);
    }

    return status;
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

    PAGED_CODE_PASSIVE();

    status = CidTableCreate(&KphpCidTable);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    KeInitializeEvent(&KphpCidPopulatedEvent, NotificationEvent, FALSE);

    status = KphCreateNPagedLookasideObject(&KphpCidApcLookaside,
                                            sizeof(KPH_CID_APC),
                                            KPH_TAG_CID_APC);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      TRACKING,
                      "KphCreateNPagedLookasideObject failed: %!STATUS!",
                      status);

        KphpCidApcLookaside = NULL;
        goto Exit;
    }

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

        if (KphpCidApcLookaside)
        {
            KphDereferenceObject(KphpCidApcLookaside);
            KphpCidApcLookaside = NULL;
        }
    }

    return status;
}

/**
 * \brief Unlinks thread contexts from a process context.
 *
 * \param[in] Process The process context to unlink thread contexts from.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphpUnlinkProcessContextThreadContexts(
    _In_ PKPH_PROCESS_CONTEXT Process
    )
{
    PAGED_CODE();

    KphAcquireRWLockExclusive(&Process->ThreadListLock);

    while (!IsListEmpty(&Process->ThreadListHead))
    {
        PKPH_THREAD_CONTEXT thread;

        thread = CONTAINING_RECORD(RemoveHeadList(&Process->ThreadListHead),
                                   KPH_THREAD_CONTEXT,
                                   ThreadListEntry);
        NT_ASSERT(thread->InThreadList);

        Process->NumberOfThreads--;
        Process->NumberOfUnlinkedThreads++;

        thread->InThreadList = FALSE;
        KphDereferenceObject(thread);
    }

    if (Process->InitialThread)
    {
        KphDereferenceObject(Process->InitialThread);
        Process->InitialThread = NULL;
    }

    KphReleaseRWLock(&Process->ThreadListLock);
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

    PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Parameter);

    NT_ASSERT(Entry->Object & ~CID_LOCKED_OBJECT_MASK);

    object = (PVOID)(Entry->Object & CID_LOCKED_OBJECT_MASK);

    CidAssignObject(Entry, NULL);

    if (KphGetObjectType(object) == KphProcessContextType)
    {
        PKPH_PROCESS_CONTEXT process;

        process = object;

        KphpUnlinkProcessContextThreadContexts(process);
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
    PAGED_CODE_PASSIVE();

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

    PAGED_CODE_PASSIVE();

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
        KphTracePrint(TRACE_LEVEL_ERROR,
                      TRACKING,
                      "KphCreateObject (%lu) failed: %!STATUS!",
                      HandleToULong(Cid),
                      status);

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

    PAGED_CODE_PASSIVE();

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
    PKPH_OBJECT_TYPE objectType;
    PKPH_PROCESS_CONTEXT process;

    PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Parameter);

    objectType = KphGetObjectType(Context);

    if (objectType == KphProcessContextType)
    {
        process = Context;
        KphVerifyProcessAndProtectIfAppropriate(process);
    }

    return FALSE;
}

// from phnative.h
#define KPH_FIRST_PROCESS(Processes) ((PSYSTEM_PROCESS_INFORMATION)(Processes))
#define KPH_NEXT_PROCESS(Process) (                                           \
    ((PSYSTEM_PROCESS_INFORMATION)(Process))->NextEntryOffset ?               \
    (PSYSTEM_PROCESS_INFORMATION)Add2Ptr((Process),                           \
    ((PSYSTEM_PROCESS_INFORMATION)(Process))->NextEntryOffset) :              \
    NULL                                                                      \
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

    PAGED_CODE_PASSIVE();

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
            LARGE_INTEGER timeout;

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

            timeout.QuadPart = 0;
            status = KeWaitForSingleObject(processObject,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           &timeout);
            if (status != STATUS_TIMEOUT)
            {
                KphTracePrint(TRACE_LEVEL_WARNING,
                              TRACKING,
                              "KeWaitForSingleObject(processObject) "
                              "reported: %!STATUS!",
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
                          "KphpTrackContext failed (process %lu)",
                          HandleToULong(info->UniqueProcessId));
            continue;
        }

        process->CreatorClientId.UniqueProcess = NtCurrentProcess();
        process->CreatorClientId.UniqueThread = NtCurrentThread();

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "Tracking process %wZ (%lu)",
                      &process->ImageName,
                      HandleToULong(process->ProcessId));

        for (ULONG i = 0; i < info->NumberOfThreads; i++)
        {
            PKPH_THREAD_CONTEXT thread;
            PETHREAD threadObject;
            PSYSTEM_THREAD_INFORMATION threadInfo;

            threadInfo = &info->Threads[i];

            status = PsLookupThreadByThreadId(threadInfo->ClientId.UniqueThread,
                                              &threadObject);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_WARNING,
                              TRACKING,
                              "PsLookupThreadByThreadId failed "
                              "(thread %lu in process %wZ (%lu))): %!STATUS!",
                              HandleToULong(threadInfo->ClientId.UniqueThread),
                              &process->ImageName,
                              HandleToULong(process->ProcessId),
                              status);

                continue;
            }

            if (PsIsThreadTerminating(threadObject))
            {
                KphTracePrint(TRACE_LEVEL_WARNING,
                              TRACKING,
                              "PsIsThreadTerminating reported TRUE "
                              "(thread %lu in process %wZ (%lu)))",
                              HandleToULong(threadInfo->ClientId.UniqueThread),
                              &process->ImageName,
                              HandleToULong(process->ProcessId));

                ObDereferenceObject(threadObject);
                continue;
            }

            thread = KphpTrackContext(threadInfo->ClientId.UniqueThread,
                                      KphThreadContextType,
                                      sizeof(KPH_THREAD_CONTEXT),
                                      threadObject);

            ObDereferenceObject(threadObject);
            threadObject = NULL;

            if (!thread)
            {
                KphTracePrint(TRACE_LEVEL_ERROR,
                              TRACKING,
                              "KphpTrackContext failed "
                              "(thread %lu in process %wZ (%lu))",
                              HandleToULong(threadInfo->ClientId.UniqueThread),
                              &process->ImageName,
                              HandleToULong(process->ProcessId));
                continue;
            }

            thread->CreatorClientId.UniqueProcess = NtCurrentProcess();
            thread->CreatorClientId.UniqueThread = NtCurrentThread();

            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          TRACKING,
                          "Tracking thread %lu in process %wZ (%lu)",
                          HandleToULong(thread->ClientId.UniqueThread),
                          &process->ImageName,
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
    PAGED_CODE_PASSIVE();

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
    PKPH_PROCESS_CONTEXT process;

    PAGED_CODE_PASSIVE();

    KphpCidWaitForPopulate();

    process = KphpUntrackContext(ProcessId, KphProcessContextType);
    if (!process)
    {
        return NULL;
    }

    KphpUnlinkProcessContextThreadContexts(process);

    return process;
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
    PAGED_CODE_PASSIVE();

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
    PKPH_THREAD_CONTEXT thread;

    PAGED_CODE_PASSIVE();

    KphpCidWaitForPopulate();

    thread = KphpUntrackContext(ThreadId, KphThreadContextType);
    if (!thread)
    {
        return NULL;
    }

    if (thread->ProcessContext)
    {
        KphAcquireRWLockExclusive(&thread->ProcessContext->ThreadListLock);

        if (thread->InThreadList)
        {
            RemoveEntryList(&thread->ThreadListEntry);
            thread->InThreadList = FALSE;
            thread->ProcessContext->NumberOfThreads--;
            KphDereferenceObject(thread);
        }

        if (thread->ProcessContext->InitialThread == thread)
        {
            thread->ProcessContext->InitialThread = NULL;
            KphDereferenceObject(thread);
        }

        KphReleaseRWLock(&thread->ProcessContext->ThreadListLock);
    }

    return thread;
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

/**
 * \brief Checks the APC no-op routine for a given process.
 *
 * \param[in] Process The context of a process to check the routine of.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCheckProcessApcNoopRoutine(
    _In_ PKPH_PROCESS_CONTEXT Process
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    PROCESS_MITIGATION_POLICY_INFORMATION policyInfo;

    PAGED_CODE_PASSIVE();

    if (Process->ApcNoopRoutine)
    {
        return STATUS_SUCCESS;
    }

    status = ObOpenObjectByPointer(Process->EProcess,
                                   OBJ_KERNEL_HANDLE,
                                   NULL,
                                   PROCESS_ALL_ACCESS,
                                   *PsProcessType,
                                   KernelMode,
                                   &processHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      TRACKING,
                      "ObOpenObjectByPointer failed: %!STATUS!",
                      status);

        processHandle = NULL;
        goto Exit;
    }

    policyInfo.Policy = ProcessControlFlowGuardPolicy;
    policyInfo.ControlFlowGuardPolicy.Flags = 0;
    status = ZwQueryInformationProcess(processHandle,
                                       ProcessMitigationPolicy,
                                       &policyInfo,
                                       sizeof(policyInfo),
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      TRACKING,
                      "ZwQueryInformationProcess failed: %!STATUS!",
                      status);
        goto Exit;
    }

    if (policyInfo.ControlFlowGuardPolicy.EnableXfg)
    {
        status = KphDisableXfgOnTarget(processHandle, KphNtDllRtlSetBits);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          TRACKING,
                          "KphDisableXfgOnTarget failed (0x%08lx): %!STATUS!",
                          policyInfo.ControlFlowGuardPolicy.Flags,
                          status);

            goto Exit;
        }
    }

    if (policyInfo.ControlFlowGuardPolicy.EnableExportSuppression)
    {
        status = KphGuardGrantSuppressedCallAccess(processHandle,
                                                   KphNtDllRtlSetBits);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          TRACKING,
                          "KphGuardGrantSuppressedCallAccess failed "
                          "(0x%08lx): %!STATUS!",
                          policyInfo.ControlFlowGuardPolicy.Flags,
                          status);

            goto Exit;
        }
    }

    Process->ApcNoopRoutine = (PKNORMAL_ROUTINE)KphNtDllRtlSetBits;
    status = STATUS_SUCCESS;

Exit:

    if (processHandle)
    {
        ObCloseHandle(processHandle, KernelMode);
    }

    return status;
}

/**
 * \brief Performs actions to verify a process and begin protecting it. Process
 * protection is only started processes that meet the necessary requirements.
 *
 * \param[in] Process The context of a process verify and protect.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphVerifyProcessAndProtectIfAppropriate(
    _In_ PKPH_PROCESS_CONTEXT Process
    )
{
    NTSTATUS status;
    KPH_PROCESS_STATE processState;

    PAGED_CODE_PASSIVE();

    if (Process->ImageFileName && !Process->VerifiedProcess)
    {
        status = KphVerifyFile(Process->ImageFileName);

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      VERIFY,
                      "KphVerifyFile: %lu \"%wZ\": %!STATUS!",
                      HandleToULong(Process->ProcessId),
                      Process->ImageFileName,
                      status);

        if (NT_SUCCESS(status))
        {
            Process->VerifiedProcess = TRUE;
        }
    }

    processState = KphGetProcessState(Process);
    if ((processState & KPH_PROCESS_STATE_LOW) == KPH_PROCESS_STATE_LOW)
    {
        ACCESS_MASK processAllowedMask;
        ACCESS_MASK threadAllowedMask;

        if (KphProtectionsSuppressed())
        {
            //
            // Allow all access, but still exercise the code by registering.
            //
            processAllowedMask = ((ACCESS_MASK)-1);
            threadAllowedMask = ((ACCESS_MASK)-1);
        }
        else
        {
            processAllowedMask = KPH_PROTECTED_PROCESS_MASK;
            threadAllowedMask = KPH_PROTECTED_THREAD_MASK;
        }

        status = KphStartProtectingProcess(Process,
                                           processAllowedMask,
                                           threadAllowedMask);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          PROTECTION,
                          "KphStartProtectingProcess failed: %!STATUS!",
                          status);

            NT_ASSERT(!Process->Protected);
        }
    }
}

/**
 * \brief Gets the process image name for a given thread.
 *
 * \details The image name of a thread might not be available if there is no
 * process context associated with the thread. This is very unlikely but
 * possible if resources are constrained on the system. If there is no process
 * context associated with the thread, this function will return NULL.
 *
 * \param[in] Thread The thread context to get the process image name of.
 *
 * \return Image name for the process of the given thread, otherwise NULL.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
_Maybenull_
PUNICODE_STRING KphGetThreadImageName(
    _In_ PKPH_THREAD_CONTEXT Thread
    )
{
    PAGED_CODE();

    if (!Thread->ProcessContext)
    {
        return NULL;
    }

    return &Thread->ProcessContext->ImageName;
}
