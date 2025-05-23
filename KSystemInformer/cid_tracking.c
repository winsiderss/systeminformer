/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2025
 *
 */

#include <kph.h>
#include <informer.h>

#include <trace.h>

typedef struct _KPH_CID_APC
{
    KSI_KAPC Apc;
    KEVENT CompletedEvent;
    PKPH_THREAD_CONTEXT Thread;
} KPH_CID_APC, *PKPH_CID_APC;

KPH_PROTECTED_DATA_SECTION_PUSH();
static PKPH_OBJECT_TYPE KphpCidApcType = NULL;
PKPH_OBJECT_TYPE KphProcessContextType = NULL;
PKPH_OBJECT_TYPE KphThreadContextType = NULL;
static PKPH_NPAGED_LOOKASIDE_OBJECT KphpCidApcLookaside = NULL;
static PKPH_NPAGED_LOOKASIDE_OBJECT KphpProcessContextLookaside = NULL;
static PKPH_NPAGED_LOOKASIDE_OBJECT KphpThreadContextLookaside = NULL;
KPH_PROTECTED_DATA_SECTION_POP();
KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const UNICODE_STRING KphpCidApcTypeName = RTL_CONSTANT_STRING(L"KphCidApc");
static const UNICODE_STRING KphpProcessContextTypeName = RTL_CONSTANT_STRING(L"KphProcessContext");
static const UNICODE_STRING KphpThreadContextTypeName = RTL_CONSTANT_STRING(L"KphThreadContext");
static const LARGE_INTEGER KphpCidApcTimeout = KPH_TIMEOUT(3 * 1000);
KPH_PROTECTED_DATA_SECTION_RO_POP();
static BOOLEAN KphpCidTrackingInitialized = FALSE;
static KPH_CID_TABLE KphpCidTable;
static volatile LONG KphpCidPopulated = 0;
static KEVENT KphpCidPopulatedEvent;
static PKPH_PROCESS_CONTEXT KphpSystemProcessContext = NULL;
static volatile ULONG64 KphpProcessSequence = 0;

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
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
PVOID KphpLookupContext(
    _In_ HANDLE Cid,
    _In_ PKPH_OBJECT_TYPE ObjectType
    )
{
    PVOID object;
    PKPH_CID_TABLE_ENTRY entry;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    entry = KphCidGetEntry(Cid, &KphpCidTable);
    if (!entry)
    {
        return NULL;
    }

    object = KphCidReferenceObject(entry);
    if (object && (KphGetObjectType(object) != ObjectType))
    {
        KphDereferenceObject(object);
        object = NULL;
    }

    return object;
}

/**
 * \brief Retrieves the system process context.
 *
 * \return Pointer to the system process context, null if not found. The caller
 * *must* dereference the object when they are through with it.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
PKPH_PROCESS_CONTEXT KphGetSystemProcessContext(
    VOID
    )
{
    KPH_NPAGED_CODE_DISPATCH_MAX();

    if (KphpSystemProcessContext)
    {
        KphReferenceObject(KphpSystemProcessContext);
    }

    return KphpSystemProcessContext;
}

/**
 * \brief Retrieves a process context.
 *
 * \param[in] ProcessId The process ID of the process to get the context for.
 *
 * \return Pointer to the process context, null if not found. The caller
 * *must* dereference the object when they are through with it.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
PKPH_PROCESS_CONTEXT KphGetProcessContext(
    _In_ HANDLE ProcessId
    )
{
    KPH_NPAGED_CODE_DISPATCH_MAX();

    return KphpLookupContext(ProcessId, KphProcessContextType);
}

/**
 * \brief Retrieves a process context by process object.
 *
 * \param[in] Process The process object of the process to get the context for.
 *
 * \return Pointer to the process context, null if not found. The caller
 * *must* dereference the object when they are through with it.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
PKPH_PROCESS_CONTEXT KphGetEProcessContext(
    _In_ PEPROCESS Process
    )
{
    KPH_NPAGED_CODE_DISPATCH_MAX();

    if (Process == PsInitialSystemProcess)
    {
        return KphGetSystemProcessContext();
    }

    return KphGetProcessContext(PsGetProcessId(Process));
}

/**
 * \brief Retrieves a thread context.
 *
 * \param[in] ThreadId The thread ID of the thread to get the context for.
 *
 * \return Pointer to the thread context, null if not found. The caller
 * *must* dereference the object when they are through with it.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
PKPH_THREAD_CONTEXT KphGetThreadContext(
    _In_ HANDLE ThreadId
    )
{
    KPH_NPAGED_CODE_DISPATCH_MAX();

    return KphpLookupContext(ThreadId, KphThreadContextType);
}

KPH_PAGED_FILE();

/**
 * \brief Marks the CID tracking populated, which unblocks public tracking APIs.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCidMarkPopulated(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

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
    KPH_PAGED_CODE_PASSIVE();

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
 * \brief Allocates a CID APC object.
 *
 * \return Pointer to CID APC object, null on failure.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_IRQL_requires_max_(APC_LEVEL)
_Return_allocatesMem_size_(Size)
PVOID KSIAPI KphpAllocateCidApc(
    _In_ SIZE_T Size
    )
{
    PVOID object;

    KPH_PAGED_CODE();

    NT_ASSERT(KphpCidApcLookaside);
    NT_ASSERT(Size <= KphpCidApcLookaside->L.Size);
    DBG_UNREFERENCED_PARAMETER(Size);

    object = KphAllocateFromNPagedLookasideObject(KphpCidApcLookaside);
    if (object)
    {
        KphReferenceObject(KphpCidApcLookaside);
    }

    return object;
}

/**
 * \brief Initializes a CID APC object.
 *
 * \param[in] Object The CID APC object to initialize.
 * \param[in] Parameter Unused.
 *
 * \return STATUS_SUCCESS
 */
_Function_class_(KPH_TYPE_INITIALIZE_PROCEDURE)
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpInitializeCidApc(
    _Inout_ PVOID Object,
    _In_opt_ PVOID Parameter
    )
{
    PKPH_CID_APC apc;

    KPH_PAGED_CODE();

    UNREFERENCED_PARAMETER(Parameter);

    apc = Object;

    KeInitializeEvent(&apc->CompletedEvent, NotificationEvent, FALSE);

    return STATUS_SUCCESS;
}

/**
 * \brief Deletes a CID APC object.
 *
 * \param[in] Object The CID APC  object to delete.
 */
_Function_class_(KPH_TYPE_DELETE_PROCEDURE)
_IRQL_requires_max_(APC_LEVEL)
VOID KSIAPI KphpDeleteCidApc(
    _Inout_ PVOID Object
    )
{
    PKPH_CID_APC apc;

    KPH_PAGED_CODE();

    apc = Object;

    if (apc->Thread)
    {
        KphDereferenceObject(apc->Thread);
    }
}

/**
 * \brief Frees a CID APC object.
 *
 * \param[in] Object The CID APC object to free.
 */
_Function_class_(KPH_TYPE_FREE_PROCEDURE)
_IRQL_requires_max_(APC_LEVEL)
VOID KSIAPI KphpFreeCidApc(
    _In_freesMem_ PVOID Object
    )
{
    KPH_PAGED_CODE();

    NT_ASSERT(KphpCidApcLookaside);

    KphFreeToNPagedLookasideObject(KphpCidApcLookaside, Object);
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

    KPH_PAGED_CODE();

    UNREFERENCED_PARAMETER(Apc);
    DBG_UNREFERENCED_PARAMETER(Reason);

    apc = CONTAINING_RECORD(Apc, KPH_CID_APC, Apc);

    KphDereferenceObject(apc);
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

    KPH_PAGED_CODE_PASSIVE();

    DBG_UNREFERENCED_PARAMETER(Size);
    NT_ASSERT(KphpProcessContextLookaside);
    NT_ASSERT(Size <= KphpProcessContextLookaside->L.Size);

    object = KphAllocateFromNPagedLookasideObject(KphpProcessContextLookaside);
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
_IRQL_requires_max_(PASSIVE_LEVEL)
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

    KPH_PAGED_CODE_PASSIVE();

    process = Object;
    processObject = Parameter;

    process->SequenceNumber = InterlockedIncrementU64(&KphpProcessSequence);

    KphSetInformerSettings(&process->InformerFilter,
                           &KphDefaultInformerProcessFilter);

    status = ObOpenObjectByPointer(processObject,
                                   OBJ_KERNEL_HANDLE,
                                   NULL,
                                   PROCESS_ALL_ACCESS,
                                   *PsProcessType,
                                   KernelMode,
                                   &processHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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

    if (KphDynPsGetProcessStartKey)
    {
        process->ProcessStartKey = KphDynPsGetProcessStartKey(processObject);
    }
    else
    {
        PROCESS_TELEMETRY_ID_INFORMATION telemetryIdInfo;

        telemetryIdInfo.HeaderSize = 0;

        status = ZwQueryInformationProcess(processHandle,
                                           ProcessTelemetryIdInformation,
                                           &telemetryIdInfo,
                                           sizeof(PROCESS_TELEMETRY_ID_INFORMATION),
                                           NULL);

        if ((status == STATUS_BUFFER_OVERFLOW) &&
            RTL_CONTAINS_FIELD(&telemetryIdInfo,
                               telemetryIdInfo.HeaderSize,
                               ProcessStartKey))
        {
            status = STATUS_SUCCESS;
        }

        if (NT_SUCCESS(status))
        {
            process->ProcessStartKey = telemetryIdInfo.ProcessStartKey;
        }
        else
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          TRACKING,
                          "ProcessTelemetryIdInformation failed: %!STATUS!",
                          status);

            NT_ASSERT(process->ProcessStartKey == 0);
        }
    }

#ifdef _WIN64
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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "PsReferenceProcessFilePointer failed: %!STATUS!",
                      status);

        process->FileObject = NULL;
    }

    if (process->FileObject)
    {
        ULONG returnLength;

        status = KphQueryNameFileObject(process->FileObject,
                                        NULL,
                                        0,
                                        &returnLength);
        if (status == STATUS_BUFFER_TOO_SMALL)
        {
            POBJECT_NAME_INFORMATION nameInfo;

            nameInfo = KphAllocateNPaged(returnLength,
                                         KPH_TAG_PROCESS_IMAGE_FILE_NAME);
            if (nameInfo)
            {
                status = KphQueryNameFileObject(process->FileObject,
                                                nameInfo,
                                                returnLength,
                                                &returnLength);
                if (NT_SUCCESS(status))
                {
                    C_ASSERT(FIELD_OFFSET(OBJECT_NAME_INFORMATION, Name) == 0);
                    process->ImageFileName = (PUNICODE_STRING)nameInfo;
                }
                else
                {
                    KphTracePrint(TRACE_LEVEL_VERBOSE,
                                  TRACKING,
                                  "KphQueryNameFileObject failed: %!STATUS!",
                                  status);

                    KphFree(nameInfo, KPH_TAG_PROCESS_IMAGE_FILE_NAME);
                }
            }
            else
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              TRACKING,
                              "Failed to allocate process image file name.");
            }
        }
    }

    if (!process->ImageFileName)
    {
        status = SeLocateProcessImageName(process->EProcess,
                                          &process->ImageFileName);
        if (NT_SUCCESS(status))
        {
            process->SystemAllocatedImageFileName = TRUE;
        }
        else
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          TRACKING,
                          "SeLocateProcessImageName failed: %!STATUS!",
                          status);

            process->ImageFileName = NULL;
        }
    }

    if (process->ImageFileName)
    {
        status = KphGetFileNameFinalComponent(process->ImageFileName,
                                              &process->ImageName);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          TRACKING,
                          "KphGetFileNameFinalComponent failed: %!STATUS!",
                          status);

            NT_ASSERT(process->ImageName.Length == 0);
        }
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
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          TRACKING,
                          "KphGetProcessImageName failed: %!STATUS!",
                          status);

            NT_ASSERT(process->ImageName.Length == 0);
        }
    }

    if (process->EProcess == PsInitialSystemProcess)
    {
        NT_ASSERT(!KphpSystemProcessContext);
        KphReferenceObject(process);
        KphpSystemProcessContext = process;
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
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpDeleteProcessContext(
    _Inout_ PVOID Object
    )
{
    PKPH_PROCESS_CONTEXT process;

    KPH_PAGED_CODE_PASSIVE();

    process = Object;

    KphAtomicAssignObjectReference(&process->SessionToken.Atomic, NULL);

    if (process->Protected)
    {
        KphStopProtectingProcess(process);
    }

    if (process->ImageFileName)
    {
        if (process->SystemAllocatedImageFileName)
        {
            KphFreePool(process->ImageFileName);
        }
        else
        {
            KphFree(process->ImageFileName, KPH_TAG_PROCESS_IMAGE_FILE_NAME);
        }
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
_Function_class_(KPH_TYPE_FREE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpFreeProcessContext(
    _In_freesMem_ PVOID Object
    )
{
    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(KphpProcessContextLookaside);

    KphFreeToNPagedLookasideObject(KphpProcessContextLookaside, Object);
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

    KPH_PAGED_CODE_PASSIVE();

    DBG_UNREFERENCED_PARAMETER(Size);
    NT_ASSERT(KphpThreadContextLookaside);
    NT_ASSERT(Size <= KphpThreadContextLookaside->L.Size);

    object = KphAllocateFromNPagedLookasideObject(KphpThreadContextLookaside);
    if (object)
    {
        KphReferenceObject(KphpThreadContextLookaside);
    }

    return object;
}

/**
 * \brief Performs thread context initialization for a WSL thread.
 *
 * \param[in] Dyn Dynamic configuration.
 * \param[in] ThreadContext The thread context to initialize.
 */
_IRQL_requires_(APC_LEVEL)
VOID KphpInitializeWSLThreadContext(
    _In_ PKPH_DYN Dyn,
    _In_ PKPH_THREAD_CONTEXT ThreadContext
    )
{
    PVOID picoContext;
    PVOID value;

    KPH_PAGED_CODE();

    //
    // We use an APC here to reach into the thread pico context. We could
    // reach directly into the pico context elsewhere, but reversing shows
    // intent for possible other pico subsystem providers in the future.
    // So, we use some "undocumented" APIs in the APC to ask "nicely" for
    // the correct pico context.
    //

    if ((ThreadContext->SubsystemType != SubsystemInformationTypeWSL) ||
        !KphDynLxpThreadGetCurrent ||
        (Dyn->LxPicoThrdInfo == ULONG_MAX) ||
        (Dyn->LxPicoThrdInfoTID == ULONG_MAX) ||
        (Dyn->LxPicoProc == ULONG_MAX) ||
        (Dyn->LxPicoProcInfo == ULONG_MAX) ||
        (Dyn->LxPicoProcInfoPID == ULONG_MAX))
    {
        return;
    }

    if (!KphDynLxpThreadGetCurrent(&picoContext))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "LxpThreadGetCurrent failed");

        return;
    }

    if (!ThreadContext->WSL.ValidThreadId)
    {
        value = *(PVOID*)Add2Ptr(picoContext, Dyn->LxPicoThrdInfo);
        ThreadContext->WSL.ThreadId =
            *(PULONG)Add2Ptr(value, Dyn->LxPicoThrdInfoTID);

        ThreadContext->WSL.ValidThreadId = TRUE;
    }

    if (ThreadContext->ProcessContext &&
        !ThreadContext->ProcessContext->WSL.ValidProcessId)
    {
        value = *(PVOID*)Add2Ptr(picoContext, Dyn->LxPicoProc);
        value = *(PVOID*)Add2Ptr(value, Dyn->LxPicoProcInfo);
        ThreadContext->ProcessContext->WSL.ProcessId =
            *(PULONG)Add2Ptr(value, Dyn->LxPicoProcInfoPID);

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
    PKPH_DYN dyn;
    PTEB teb;

    KPH_PAGED_CODE();

    UNREFERENCED_PARAMETER(NormalRoutine);
    UNREFERENCED_PARAMETER(NormalContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    apc = CONTAINING_RECORD(Apc, KPH_CID_APC, Apc);

    NT_ASSERT(apc->Thread->EThread == KeGetCurrentThread());
#ifdef _WIN64
    C_ASSERT(FIELD_OFFSET(TEB, SubProcessTag) == 0x1720);
#endif

    teb = PsGetCurrentThreadTeb();
    if (teb)
    {
        __try
        {
            apc->Thread->SubProcessTag = RtlReadPointerFromUser(&teb->SubProcessTag);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          TRACKING,
                          "Failed to populate SubProcessTag: %!STATUS!",
                          GetExceptionCode());
        }
    }

    dyn = KphReferenceDynData();
    if (dyn)
    {
        KphpInitializeWSLThreadContext(dyn, apc->Thread);
        KphDereferenceObject(dyn);
    }

    KeSetEvent(&apc->CompletedEvent, EVENT_INCREMENT, FALSE);
}

/**
 * \brief Initializes thread context parts in the original thread environment.
 *
 * \param[in] ThreadContext The thread context to initialize.
 * \param[in] Wait If TRUE this routine will try to wait for the APC to
 * complete, otherwise this routine will not wait.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphpInitThreadContextInOriginalEnvironment(
    _In_ PKPH_THREAD_CONTEXT ThreadContext,
    _In_ BOOLEAN Wait
    )
{
    NTSTATUS status;
    PKPH_CID_APC apc;

    KPH_PAGED_CODE();

    status = KphCreateObject(KphpCidApcType, sizeof(KPH_CID_APC), &apc, NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "KphCreateObject failed: %!STATUS!",
                      status);

        goto Exit;
    }

    apc->Thread = ThreadContext;
    KphReferenceObject(apc->Thread);

    KsiInitializeApc(&apc->Apc,
                     KphDriverObject,
                     ThreadContext->EThread,
                     OriginalApcEnvironment,
                     KphpInitializeThreadContextSpecialApc,
                     KphpCidApcCleanup,
                     NULL,
                     KernelMode,
                     NULL);

    //
    // The APC could fire immediately we must reference before trying to queue.
    //
    KphReferenceObject(apc);
    if (!KsiInsertQueueApc(&apc->Apc, NULL, NULL, IO_NO_INCREMENT))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "KsiInsertQueueApc failed");

        KphDereferenceObject(apc);
        status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    if (!Wait)
    {
        status = STATUS_SUCCESS;
        goto Exit;
    }

    status = KeWaitForSingleObject(&apc->CompletedEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   (PLARGE_INTEGER)&KphpCidApcTimeout);
    if (status != STATUS_SUCCESS)
    {
        NTSTATUS removeStatus;

        NT_ASSERT(status == STATUS_TIMEOUT);

        removeStatus = KsiRemoveQueueApc(&apc->Apc);

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "KsiRemoveQueueApc: %!STATUS!",
                      removeStatus);

        goto Exit;
    }

Exit:

    if (apc)
    {
        KphDereferenceObject(apc);
    }

    if (!NT_SUCCESS(status) || (status == STATUS_TIMEOUT))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "KphpInitializeThreadContextApc failed: %!STATUS!",
                      status);
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
_IRQL_requires_max_(PASSIVE_LEVEL)
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

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Parameter);

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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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

    if (!PsIsSystemThread(thread->EThread))
    {
        KphpInitThreadContextInOriginalEnvironment(thread, FALSE);
    }

    status = STATUS_SUCCESS;

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
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpDeleteThreadContext(
    _Inout_ PVOID Object
    )
{
    PKPH_THREAD_CONTEXT thread;

    KPH_PAGED_CODE_PASSIVE();

    thread = Object;

    NT_ASSERT(!thread->InThreadList);

    KphAtomicAssignObjectReference(&thread->RequestSessionToken.Atomic, NULL);
    KphAtomicAssignObjectReference(&thread->SessionToken.Atomic, NULL);

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
_Function_class_(KPH_TYPE_FREE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpFreeThreadContext(
    _In_freesMem_ PVOID Object
    )
{
    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(KphpThreadContextLookaside);

    KphFreeToNPagedLookasideObject(KphpThreadContextLookaside, Object);
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

    KPH_PAGED_CODE_PASSIVE();

    status = KphCidTableCreate(&KphpCidTable);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    KeInitializeEvent(&KphpCidPopulatedEvent, NotificationEvent, FALSE);

    status = KphCreateNPagedLookasideObject(&KphpCidApcLookaside,
                                            KphAddObjectHeaderSize(sizeof(KPH_CID_APC)),
                                            KPH_TAG_CID_APC);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "KphCreateNPagedLookasideObject failed: %!STATUS!",
                      status);

        KphpCidApcLookaside = NULL;
        goto Exit;
    }

    status = KphCreateNPagedLookasideObject(&KphpProcessContextLookaside,
                                            KphAddObjectHeaderSize(sizeof(KPH_PROCESS_CONTEXT)),
                                            KPH_TAG_PROCESS_CONTEXT);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "KphCreatePagedLookasideObject failed: %!STATUS!",
                      status);

        KphpProcessContextLookaside = NULL;
        goto Exit;
    }

    status = KphCreateNPagedLookasideObject(&KphpThreadContextLookaside,
                                            KphAddObjectHeaderSize(sizeof(KPH_THREAD_CONTEXT)),
                                            KPH_TAG_THREAD_CONTEXT);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
    typeInfo.Flags = 0;
    typeInfo.DeferDelete = TRUE;

    KphCreateObjectType(&KphpProcessContextTypeName,
                        &typeInfo,
                        &KphProcessContextType);

    typeInfo.Allocate = KphpAllocateThreadContext;
    typeInfo.Initialize = KphpInitializeThreadContext;
    typeInfo.Delete = KphpDeleteThreadContext;
    typeInfo.Free = KphpFreeThreadContext;
    typeInfo.Flags = 0;
    typeInfo.DeferDelete = TRUE;

    KphCreateObjectType(&KphpThreadContextTypeName,
                        &typeInfo,
                        &KphThreadContextType);

    typeInfo.Allocate = KphpAllocateCidApc;
    typeInfo.Initialize = KphpInitializeCidApc;
    typeInfo.Delete = KphpDeleteCidApc;
    typeInfo.Free = KphpFreeCidApc;
    typeInfo.Flags = 0;

    KphCreateObjectType(&KphpCidApcTypeName,
                        &typeInfo,
                        &KphpCidApcType);

    KphpCidTrackingInitialized = TRUE;
    status = STATUS_SUCCESS;

Exit:

    if (!NT_SUCCESS(status))
    {
        KphCidTableDelete(&KphpCidTable);

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
    KPH_PAGED_CODE();

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
_Function_class_(KPH_CID_RUNDOWN_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KSIAPI KphpCidCleanupCallback(
    _In_ PVOID Object,
    _In_opt_ PVOID Parameter
    )
{
    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Parameter);

    if (KphGetObjectType(Object) == KphProcessContextType)
    {
        PKPH_PROCESS_CONTEXT process;

        process = Object;

        KphpUnlinkProcessContextThreadContexts(process);
    }

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
    KPH_PAGED_CODE_PASSIVE();

    if (!KphpCidTrackingInitialized)
    {
        return;
    }

    KphCidRundown(&KphpCidTable, KphpCidCleanupCallback, NULL);

    if (KphpSystemProcessContext)
    {
        KphDereferenceObject(KphpSystemProcessContext);
    }

    KphCidTableDelete(&KphpCidTable);

    KphDereferenceObject(KphpCidApcLookaside);
    KphDereferenceObject(KphpThreadContextLookaside);
    KphDereferenceObject(KphpProcessContextLookaside);
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
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
PVOID KphpTrackContext(
    _In_ HANDLE Cid,
    _In_ PKPH_OBJECT_TYPE ObjectType,
    _In_ ULONG ObjectBodySize,
    _In_ PVOID Parameter
    )
{
    NTSTATUS status;
    PKPH_CID_TABLE_ENTRY entry;
    PVOID object;

    KPH_PAGED_CODE_PASSIVE();

    entry = KphCidGetEntry(Cid, &KphpCidTable);
    if (!entry)
    {
        return NULL;
    }

    object = KphCidReferenceObject(entry);
    if (object)
    {
        if (KphGetObjectType(object) != ObjectType)
        {
            KphDereferenceObject(object);
            object = NULL;
        }

        return object;
    }

    status = KphCreateObject(ObjectType, ObjectBodySize, &object, Parameter);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "KphCreateObject (%lu) failed: %!STATUS!",
                      HandleToULong(Cid),
                      status);

        return NULL;
    }

    KphCidAssignObject(entry, object);

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
    PKPH_CID_TABLE_ENTRY entry;
    PVOID object;

    KPH_PAGED_CODE();

    entry = KphCidGetEntry(Cid, &KphpCidTable);
    if (!entry)
    {
        return NULL;
    }

    object = KphCidReferenceObject(entry);

    if (object && (KphGetObjectType(object) != ObjectType))
    {
        KphDereferenceObject(object);
        return NULL;
    }

    KphCidAssignObject(entry, NULL);

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
BOOLEAN KSIAPI KphpCidEnumPostPopulate(
    _In_ PVOID Context,
    _In_opt_ PVOID Parameter
    )
{
    PKPH_OBJECT_TYPE objectType;
    PKPH_PROCESS_CONTEXT process;

    KPH_PAGED_CODE_PASSIVE();

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
#define KPH_NEXT_PROCESS(Process) (                                            \
    ((PSYSTEM_PROCESS_INFORMATION)(Process))->NextEntryOffset ?                \
    (PSYSTEM_PROCESS_INFORMATION)Add2Ptr((Process),                            \
    ((PSYSTEM_PROCESS_INFORMATION)(Process))->NextEntryOffset) :               \
    NULL                                                                       \
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

    KPH_PAGED_CODE_PASSIVE();

    size = (PAGE_SIZE * 4);
    buffer = KphAllocatePaged(size, KPH_TAG_CID_POPULATE);
    if (!buffer)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          TRACKING,
                          "ZwQuerySystemInformation failed: %!STATUS!",
                          status);

            goto Exit;
        }

        if (size == 0)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          TRACKING,
                          "ZwQuerySystemInformation returned zero size!");

            NT_ASSERT(!NT_SUCCESS(status));
            goto Exit;
        }

        KphFree(buffer, KPH_TAG_CID_POPULATE);
        buffer = KphAllocatePaged(size, KPH_TAG_CID_POPULATE);
        if (!buffer)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
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
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              TRACKING,
                              "PsLookupProcessByProcessId failed: %!STATUS!",
                              status);

                continue;
            }

            if (PsGetProcessExitProcessCalled(processObject))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              TRACKING,
                              "PsGetProcessExitProcessCalled reported TRUE "
                              "(process %lu)",
                              HandleToULong(info->UniqueProcessId));

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
            KphTracePrint(TRACE_LEVEL_VERBOSE,
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
                KphTracePrint(TRACE_LEVEL_VERBOSE,
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
                KphTracePrint(TRACE_LEVEL_VERBOSE,
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
                KphTracePrint(TRACE_LEVEL_VERBOSE,
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
    KPH_PAGED_CODE_PASSIVE();

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

    KPH_PAGED_CODE_PASSIVE();

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
    KPH_PAGED_CODE_PASSIVE();

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

    KPH_PAGED_CODE_PASSIVE();

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
_Function_class_(KPH_CID_ENUMERATE_CALLBACK)
BOOLEAN KSIAPI KphpEnumerateContexts(
    _In_ PVOID Object,
    _In_opt_ PVOID Parameter
    )
{
    PKPH_ENUM_CONTEXT context;
    PKPH_OBJECT_TYPE objectType;

    KPH_PAGED_CODE();

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

    KPH_PAGED_CODE();

    context.CidCallback = NULL;
    context.ProcessCallback = Callback;
    context.ThreadCallback = NULL;
    context.Parameter = Parameter;

    KphCidEnumerate(&KphpCidTable, KphpEnumerateContexts, &context);
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

    KPH_PAGED_CODE();

    context.CidCallback = NULL;
    context.ProcessCallback = NULL;
    context.ThreadCallback = Callback;
    context.Parameter = Parameter;

    KphCidEnumerate(&KphpCidTable, KphpEnumerateContexts, &context);
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

    KPH_PAGED_CODE();

    context.CidCallback = Callback;
    context.ProcessCallback = NULL;
    context.ThreadCallback = NULL;
    context.Parameter = Parameter;

    KphCidEnumerate(&KphpCidTable, KphpEnumerateContexts, &context);
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

    KPH_PAGED_CODE_PASSIVE();

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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
                                       sizeof(PROCESS_MITIGATION_POLICY_INFORMATION),
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
            KphTracePrint(TRACE_LEVEL_VERBOSE,
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
            KphTracePrint(TRACE_LEVEL_VERBOSE,
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

    KPH_PAGED_CODE_PASSIVE();

    if (Process->ImageFileName &&
        Process->FileObject &&
        !Process->VerifiedProcess)
    {
        status = KphVerifyFile(Process->ImageFileName, Process->FileObject);

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

    if (KphTestProcessContextState(Process, KPH_PROCESS_STATE_LOW))
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
            KphTracePrint(TRACE_LEVEL_VERBOSE,
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
    KPH_PAGED_CODE();

    if (!Thread->ProcessContext)
    {
        return NULL;
    }

    return &Thread->ProcessContext->ImageName;
}

/**
 * \brief Retrieves the process state mask from a process.
 *
 * \param[in] Process The process to get the state from.
 *
 * \return State mask describing what state the process is in.
 */
_IRQL_requires_max_(APC_LEVEL)
KPH_PROCESS_STATE KphGetProcessState(
    _In_ PKPH_PROCESS_CONTEXT Process
    )
{
    KPH_PROCESS_STATE processState;

    KPH_PAGED_CODE();

    if (KphProtectionsSuppressed())
    {
        //
        // This ultimately permits low state callers into the driver. But still
        // check for verification. We still want to exercise the code below,
        // regardless.
        //
        processState = ~KPH_PROCESS_VERIFIED_PROCESS;
    }
    else
    {
        processState = 0;
    }

    if (Process->SecurelyCreated)
    {
        processState |= KPH_PROCESS_SECURELY_CREATED;
    }

    if (Process->VerifiedProcess)
    {
        processState |= KPH_PROCESS_VERIFIED_PROCESS;
    }

    if (Process->Protected)
    {
        processState |= KPH_PROCESS_PROTECTED_PROCESS;
    }

    if (Process->NumberOfUntrustedImageLoads == 0)
    {
        processState |= KPH_PROCESS_NO_UNTRUSTED_IMAGES;
    }

    if (!Process->StateTracking.Debugged)
    {
        if (!PsIsProcessBeingDebugged(Process->EProcess))
        {
            processState |= KPH_PROCESS_NOT_BEING_DEBUGGED;
        }
        else
        {
            Process->StateTracking.Debugged = TRUE;
        }
    }

    if (!Process->FileObject)
    {
        return processState;
    }

    processState |= KPH_PROCESS_HAS_FILE_OBJECT;

    if (!Process->StateTracking.FileObjectWritable)
    {
        if (!Process->FileObject->WriteAccess || !Process->FileObject->SharedWrite)
        {
            processState |= KPH_PROCESS_NO_WRITABLE_FILE_OBJECT;
        }
        else
        {
            Process->StateTracking.FileObjectWritable = TRUE;
        }
    }

    if (!Process->StateTracking.FileObjectTransaction)
    {
        if (!IoGetTransactionParameterBlock(Process->FileObject))
        {
            processState |= KPH_PROCESS_NO_FILE_TRANSACTION;
        }
        else
        {
            Process->StateTracking.FileObjectTransaction = TRUE;
        }
    }

    if (!Process->FileObject->SectionObjectPointer)
    {
        return processState;
    }

    processState |= KPH_PROCESS_HAS_SECTION_OBJECT_POINTERS;

    if (!Process->StateTracking.UserWritableReferences)
    {
        if (!MmDoesFileHaveUserWritableReferences(Process->FileObject->SectionObjectPointer))
        {
            processState |= KPH_PROCESS_NO_USER_WRITABLE_REFERENCES;
        }
        else
        {
            Process->StateTracking.UserWritableReferences = TRUE;
        }
    }

    return processState;
}

/**
 * \brief Queries information about a process context.
 *
 * \details Some information is not cached up front for one reason or another.
 * Usually because dynamic data isn't available. This path enables a generic
 * way to query information about a process context and try to populate and the
 * cache the information if it is not already available.
 *
 * \param[in] Process The process context to query.
 * \param[in] InformationClass The information class to query.
 * \param[out] Information Optional buffer to receive the information.
 * \param[in] InformationLength The size of the information buffer.
 * \param[out] ReturnLength Receives the number of bytes written or required.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryInformationProcessContext(
    _In_ PKPH_PROCESS_CONTEXT Process,
    _In_ KPH_PROCESS_CONTEXT_INFORMATION_CLASS InformationClass,
    _Out_writes_bytes_opt_(InformationLength) PVOID Information,
    _In_ ULONG InformationLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    ULONG returnLength;

    KPH_PAGED_CODE_PASSIVE();

    returnLength = 0;

    switch (InformationClass)
    {
        case KphProcessContextWSLProcessId:
        {
            if (Process->SubsystemType != SubsystemInformationTypeWSL)
            {
                status = STATUS_INVALID_PARAMETER;
                goto Exit;
            }

            returnLength = sizeof(ULONG);

            if (!Information || (InformationLength < sizeof(ULONG)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                goto Exit;
            }

            if (!Process->WSL.ValidProcessId)
            {
                PKPH_THREAD_CONTEXT thread;

                KphAcquireRWLockShared(&Process->ThreadListLock);

                thread = Process->InitialThread;
                if (thread)
                {
                    KphReferenceObject(thread);
                }

                KphReleaseRWLock(&Process->ThreadListLock);

                if (!thread)
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Exit;
                }

                KphpInitThreadContextInOriginalEnvironment(thread, TRUE);

                KphDereferenceObject(thread);
            }

            if (!Process->WSL.ValidProcessId)
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "WSL process ID is not valid.");

                status = STATUS_NOINTERFACE;
                goto Exit;
            }

            *(PULONG)Information = Process->WSL.ProcessId;

            status = STATUS_SUCCESS;
            break;
        }
        default:
        {
            status = STATUS_INVALID_INFO_CLASS;
            break;
        }
    }

Exit:

    if (ReturnLength)
    {
        *ReturnLength = returnLength;
    }

    return status;
}

/**
 * \brief Queries information about a thread context.
 *
 * \details Some information is not cached up front for one reason or another.
 * Usually because dynamic data isn't available. This path enables a generic
 * way to query information about a thread context and try to populate and the
 * cache the information if it is not already available.
 *
 * \param[in] Thread The thread context to query.
 * \param[in] InformationClass The information class to query.
 * \param[out] Information Optional buffer to receive the information.
 * \param[in] InformationLength The size of the information buffer.
 * \param[out] ReturnLength Receives the number of bytes written or required.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryInformationThreadContext(
    _In_ PKPH_THREAD_CONTEXT Thread,
    _In_ KPH_THREAD_CONTEXT_INFORMATION_CLASS InformationClass,
    _Out_writes_bytes_opt_(InformationLength) PVOID Information,
    _In_ ULONG InformationLength,
    _Out_opt_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    ULONG returnLength;

    KPH_PAGED_CODE_PASSIVE();

    returnLength = 0;

    switch (InformationClass)
    {
        case KphThreadContextWSLThreadId:
        {
            if (Thread->SubsystemType != SubsystemInformationTypeWSL)
            {
                status = STATUS_INVALID_HANDLE;
                goto Exit;
            }

            returnLength = sizeof(ULONG);

            if (!Information || (InformationLength < sizeof(ULONG)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                goto Exit;
            }

            if (!Thread->WSL.ValidThreadId)
            {
                KphpInitThreadContextInOriginalEnvironment(Thread, TRUE);
            }

            if (!Thread->WSL.ValidThreadId)
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "WSL thread ID is not valid.");

                status = STATUS_NOINTERFACE;
                goto Exit;
            }

            *(PULONG)Information = Thread->WSL.ThreadId;

            status = STATUS_SUCCESS;
            break;
        }
        default:
        {
            status = STATUS_INVALID_INFO_CLASS;
            break;
        }
    }

Exit:

    if (ReturnLength)
    {
        *ReturnLength = returnLength;
    }

    return status;
}
