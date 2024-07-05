/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023
 *
 */

#include <kph.h>

#include <trace.h>

typedef struct _KPH_STACK_BACK_TRACE_OBJECT
{
    KSI_KAPC Apc;
    KEVENT CompletedEvent;
    ULONG FramesToSkip;
    ULONG FramesToCapture;
    ULONG CapturedFrames;
    ULONG BackTraceHash;
    ULONG Flags;
    BOOLEAN DoHash;
    PKPH_THREAD_CONTEXT Thread;
    PVOID BackTrace[ANYSIZE_ARRAY];
} KPH_STACK_BACK_TRACE_OBJECT, *PKPH_STACK_BACK_TRACE_OBJECT;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const UNICODE_STRING KphpStackBackTraceTypeName = RTL_CONSTANT_STRING(L"KphStackBackTrace");
KPH_PROTECTED_DATA_SECTION_RO_POP();
KPH_PROTECTED_DATA_SECTION_PUSH();
static PVOID KphpSelfImageBase = NULL;
static PVOID KphpSelfImageEnd = NULL;
static PVOID KphpKsiImageBase = NULL;
static PVOID KphpKsiImageEnd = NULL;
static PKPH_OBJECT_TYPE KphpStackBackTraceType = NULL;
KPH_PROTECTED_DATA_SECTION_POP();

//
// Sentinel value inserted into the stack back trace when part of the stack
// couldn't be captured for some reason.
//
#define KPH_STACK_SENTINEL_FRAME ((PVOID)(ULONG_PTR)0xbad)

/**
 * \brief Tries to add a sentinel frame to the stack back trace.
 *
 * \param[in] FramesToCapture The number of frames to capture.
 * \param[out] BackTrace Buffer to store the back trace in.
 * \param[in,out] Frames On input, the number of frames captured so far. If a
 * sentinel frame is added, this is incremented.
 * \param[in] Flags A combination of KPH_STACK_BACK_TRACE_* flags.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphpTryAddSentinelFrame(
    _In_ ULONG FramesToCapture,
    _Out_writes_(FramesToCapture) PVOID* BackTrace,
    _Inout_ PULONG Frames,
    _In_ ULONG Flags
    )
{
    NPAGED_CODE_DISPATCH_MAX();

    if (FlagOn(Flags, KPH_STACK_BACK_TRACE_NO_SENTINEL))
    {
        return;
    }

    if (*Frames < FramesToCapture)
    {
        BackTrace[*Frames] = KPH_STACK_SENTINEL_FRAME;
        (*Frames)++;
    }
}

/**
 * \brief Captures a stack back trace.
 *
 * \param[in] FramesToSkip The number of kernel frames to skip.
 * \param[in] FramesToCapture The number of frames to capture.
 * \param[out] BackTrace Buffer to store the back trace in.
 * \param[out] BackTraceHash Optionally receives a hash of the back trace.
 * \param[in] Flags A combination of KPH_STACK_BACK_TRACE_* flags.
 * \param[in] Thread Optional thread context of the target thread, used for
 * internal tracking to prevent accidental recursion.
 *
 * \return The number of captured frames.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Success_(return != 0)
ULONG KphpCaptureStackBackTrace(
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _Out_writes_(FramesToCapture) PVOID* BackTrace,
    _Out_opt_ PULONG BackTraceHash,
    _In_ ULONG Flags,
    _In_opt_ PKPH_THREAD_CONTEXT Thread
    )
{
    ULONG frames;
    ULONG skip;

    NPAGED_CODE_DISPATCH_MAX();

    FramesToSkip += 1;

    skip = (FramesToSkip << RTL_STACK_WALKING_MODE_FRAMES_TO_SKIP_SHIFT);

    frames = RtlWalkFrameChain(BackTrace, FramesToCapture, skip);

    if (frames <= FramesToSkip)
    {
        frames = 0;
        goto ContinueCapture;
    }

    frames -= FramesToSkip;

    if (FlagOn(Flags, KPH_STACK_BACK_TRACE_SKIP_KPH))
    {
        for (skip = 0; skip < frames; skip++)
        {
            if (((BackTrace[skip] < KphpSelfImageBase) ||
                 (BackTrace[skip] >= KphpSelfImageEnd)) &&
                ((BackTrace[skip] < KphpKsiImageBase) ||
                 (BackTrace[skip] >= KphpKsiImageEnd)))
            {
                break;
            }
        }

        if (frames <= skip)
        {
            frames = 0;
            goto ContinueCapture;
        }

        frames -= skip;
        RtlMoveMemory(BackTrace, &BackTrace[skip], frames * sizeof(PVOID));
    }

ContinueCapture:

    if (!FlagOn(Flags, KPH_STACK_BACK_TRACE_USER_MODE) ||
        (KeGetCurrentIrql() == DISPATCH_LEVEL))
    {
        goto Exit;
    }

    if (!Thread)
    {
        KphpTryAddSentinelFrame(FramesToCapture, BackTrace, &frames, Flags);
        goto Exit;
    }

    NT_ASSERT(Thread->EThread == PsGetCurrentThread());

    //
    // N.B. This path can become accidentally recursive if the user mode stack
    // needs to be paged in and another kernel callback lands here again. In
    // that case skip the capture.
    //
    if (Thread->CapturingUserModeStack)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "Skipping user mode stack capture for "
                      "thread %lu in process %wZ (%lu)",
                      HandleToULong(Thread->ClientId.UniqueThread),
                      KphGetThreadImageName(Thread),
                      HandleToULong(Thread->ClientId.UniqueProcess));

        KphpTryAddSentinelFrame(FramesToCapture, BackTrace, &frames, Flags);
        goto Exit;
    }

    Thread->CapturingUserModeStack = TRUE;

    __try
    {
        frames += RtlWalkFrameChain(&BackTrace[frames],
                                    FramesToCapture - frames,
                                    RTL_WALK_USER_MODE_STACK);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        KphpTryAddSentinelFrame(FramesToCapture, BackTrace, &frames, Flags);
    }

    Thread->CapturingUserModeStack = FALSE;

Exit:

    NT_ASSERT(frames <= FramesToCapture);

    if (BackTraceHash)
    {
        *BackTraceHash = 0;

        for (ULONG i = 0; i < frames; i++)
        {
#pragma prefast(suppress: 6385)
            *BackTraceHash += PtrToUlong(BackTrace[i]);
        }
    }

    return frames;
}

/**
 * \brief Captures a stack back trace.
 *
 * \param[in] FramesToSkip The number of kernel frames to skip.
 * \param[in] FramesToCapture The number of frames to capture.
 * \param[out] BackTrace Buffer to store the back trace in.
 * \param[out] BackTraceHash Optionally receives a hash of the back trace.
 * \param[in] Flags A combination of KPH_STACK_BACK_TRACE_* flags.
 *
 * \return The number of captured frames.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Success_(return != 0)
ULONG KphCaptureStackBackTrace(
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _Out_writes_(FramesToCapture) PVOID* BackTrace,
    _Out_opt_ PULONG BackTraceHash,
    _In_ ULONG Flags
    )
{
    ULONG frames;
    PKPH_THREAD_CONTEXT thread;

    NPAGED_CODE_DISPATCH_MAX();

    if (FlagOn(Flags, KPH_STACK_BACK_TRACE_USER_MODE) &&
        (KeGetCurrentIrql() < DISPATCH_LEVEL))
    {
        thread = KphGetCurrentThreadContext();
    }
    else
    {
        thread = NULL;
    }

    frames = KphpCaptureStackBackTrace(FramesToSkip,
                                       FramesToCapture,
                                       BackTrace,
                                       BackTraceHash,
                                       Flags,
                                       thread);

    if (thread)
    {
        KphDereferenceObject(thread);
    }

    return frames;
}

PAGED_FILE();

/**
 * \brief Captures the current stack back trace into a back trace object.
 *
 * \param[in,out] BackTrace The back trace object to populate.
 */
_IRQL_requires_(APC_LEVEL)
_IRQL_requires_same_
VOID KphpCaptureStackBackTraceIntoObject(
    _Inout_ PKPH_STACK_BACK_TRACE_OBJECT BackTrace
    )
{
    PULONG backTraceHash;
    ULONG capturedFrames;

    PAGED_CODE();

    if (BackTrace->DoHash)
    {
        backTraceHash = &BackTrace->BackTraceHash;
    }
    else
    {
        backTraceHash = NULL;
    }

    capturedFrames = KphpCaptureStackBackTrace(BackTrace->FramesToSkip,
                                               BackTrace->FramesToCapture,
                                               BackTrace->BackTrace,
                                               backTraceHash,
                                               BackTrace->Flags,
                                               BackTrace->Thread);

    BackTrace->CapturedFrames = capturedFrames;

    KeSetEvent(&BackTrace->CompletedEvent, EVENT_INCREMENT, FALSE);
}

/**
 * \brief APC routine for capturing the stack back trace of a thread.
 *
 * \param[in] Apc The ACP executed, contained within the back trace object.
 * \param[in] NormalRoutine Unused.
 * \param[in] NormalContext Unused.
 * \param[in] SystemArgument1 Unused.
 * \param[in] SystemArgument2 Unused.
 */
_Function_class_(KSI_KKERNEL_ROUTINE)
_IRQL_requires_(APC_LEVEL)
_IRQL_requires_same_
VOID KSIAPI KphpCaptureStackBackTraceThreadSpecialApc(
    _In_ PKSI_KAPC Apc,
    _Inout_ _Deref_pre_maybenull_ PKNORMAL_ROUTINE *NormalRoutine,
    _Inout_ _Deref_pre_maybenull_ PVOID *NormalContext,
    _Inout_ _Deref_pre_maybenull_ PVOID *SystemArgument1,
    _Inout_ _Deref_pre_maybenull_ PVOID *SystemArgument2
    )
{
    PKPH_STACK_BACK_TRACE_OBJECT backTrace;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(NormalRoutine);
    UNREFERENCED_PARAMETER(NormalContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    backTrace = CONTAINING_RECORD(Apc, KPH_STACK_BACK_TRACE_OBJECT, Apc);

    KphpCaptureStackBackTraceIntoObject(backTrace);
}

/**
 * \brief APC cleanup routine for stack back trace capture.
 *
 * \param[in] Apc The ACP to clean up.
 * \param[in] Reason Unused.
 */
_Function_class_(KSI_KCLEANUP_ROUTINE)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_same_
VOID KSIAPI KphpCaptureStackBackTraceThreadSpecialApcCleanup(
    _In_ PKSI_KAPC Apc,
    _In_ KSI_KAPC_CLEANUP_REASON Reason
    )
{
    PKPH_STACK_BACK_TRACE_OBJECT backTrace;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Apc);
    DBG_UNREFERENCED_PARAMETER(Reason);

    backTrace = CONTAINING_RECORD(Apc, KPH_STACK_BACK_TRACE_OBJECT, Apc);

    KphDereferenceObject(backTrace);
}

/**
 * \brief Allocates a stack back trace object.
 *
 * \param[in] Size The size to allocate.
 *
 * \return Allocated object, null on allocation failure.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_Return_allocatesMem_size_(Size)
PVOID KSIAPI KphpStackBackTraceAllocate(
    _In_ SIZE_T Size
    )
{
    PAGED_CODE();

    return KphAllocateNPaged(Size, KPH_TAG_BACK_TRACE_OBJECT);
}

/**
 * \brief Frees a stack back trace object.
 *
 * \param[in] Object The stack back trace object to free.
 */
_Function_class_(KPH_TYPE_FREE_PROCEDURE)
VOID KSIAPI KphpStackBackTraceFree(
    _In_freesMem_ PVOID Object
    )
{
    PAGED_CODE();

    KphFree(Object, KPH_TAG_BACK_TRACE_OBJECT);
}

/**
 * \brief Initializes a stack back trace object.
 *
 * \param[in,out] Object The stack back trace object to initialize.
 * \param[in] Parameter The thread to initialize the back trace object for.
 *
 * \return STATUS_SUCCESS
 */
_Function_class_(KPH_TYPE_INITIALIZE_PROCEDURE)
_Must_inspect_result_
NTSTATUS KSIAPI KphpStackBackTraceInitialize(
    _Inout_ PVOID Object,
    _In_opt_ PVOID Parameter
    )
{
    PKPH_STACK_BACK_TRACE_OBJECT backTrace;
    PETHREAD thread;

    PAGED_CODE();

    NT_ASSERT(Parameter);

    backTrace = Object;
    thread = Parameter;

    KsiInitializeApc(&backTrace->Apc,
                     KphDriverObject,
                     thread,
                     OriginalApcEnvironment,
                     KphpCaptureStackBackTraceThreadSpecialApc,
                     KphpCaptureStackBackTraceThreadSpecialApcCleanup,
                     NULL,
                     KernelMode,
                     NULL);

    KeInitializeEvent(&backTrace->CompletedEvent, NotificationEvent, FALSE);

    backTrace->Thread = KphGetEThreadContext(thread);

    return STATUS_SUCCESS;
}

/**
 * \brief Deletes a stack back trace object.
 *
 * \param[in] Object The stack back trace object to delete.
 */
_Function_class_(KPH_TYPE_DELETE_PROCEDURE)
VOID KSIAPI KphpStackBackTraceDelete(
    _Inout_ PVOID Object
    )
{
    PKPH_STACK_BACK_TRACE_OBJECT backTrace;

    PAGED_CODE();

    backTrace = Object;

    if (backTrace->Thread)
    {
        KphDereferenceObject(backTrace->Thread);
    }
}

/**
 * \brief Captures a stack back trace for a given thread.
 *
 * \param[in] Thread The thread to capture the stack back trace of.
 * \param[in] FramesToSkip The number of kernel frames to skip.
 * \param[in] FramesToCapture The number of frames to capture.
 * \param[out] BackTrace Buffer to store the back trace in.
 * \param[out] CapturedFrames Receives the number of captured frames.
 * \param[out] BackTraceHash Optionally receives a hash of the back trace.
 * \param[in] Flags A combination of KPH_STACK_BACK_TRACE_* flags.
 * \param[in] Timeout Optionally specifies a timeout for the capture operation.
 *
 * \return STATUS_SUCCES, STATUS_TIMEOUT, or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphCaptureStackBackTraceThread(
    _In_ PETHREAD Thread,
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _Out_writes_(FramesToCapture) PVOID* BackTrace,
    _Out_ PULONG CapturedFrames,
    _Out_opt_ PULONG BackTraceHash,
    _In_ ULONG Flags,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    NTSTATUS status;
    ULONG backTraceSize;
    PKPH_STACK_BACK_TRACE_OBJECT backTrace;

    PAGED_CODE();

    if (Thread == PsGetCurrentThread())
    {
        *CapturedFrames = KphCaptureStackBackTrace(FramesToSkip,
                                                   FramesToCapture,
                                                   BackTrace,
                                                   BackTraceHash,
                                                   Flags);
        return (*CapturedFrames ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
    }

    NT_ASSERT(KphpStackBackTraceType);

    *CapturedFrames = 0;

    if (BackTraceHash)
    {
        *BackTraceHash = 0;
    }

    backTrace = NULL;

    status = RtlULongAdd(FramesToCapture,
                         sizeof(KPH_STACK_BACK_TRACE_OBJECT),
                         &backTraceSize);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphCreateObject(KphpStackBackTraceType,
                             backTraceSize,
                             &backTrace,
                             Thread);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphCreateObject failed: %!STATUS!",
                      status);

        backTrace = NULL;
        goto Exit;
    }

    backTrace->FramesToCapture = FramesToCapture;
    backTrace->FramesToSkip = FramesToSkip;
    backTrace->Flags = Flags;
    backTrace->DoHash = (BackTraceHash != NULL);

    //
    // The APC could fire immediately we must reference before trying to queue.
    //
    KphReferenceObject(backTrace);
    if (!KsiInsertQueueApc(&backTrace->Apc, NULL, NULL, IO_NO_INCREMENT))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KsiInsertQueueApc failed");

        KphDereferenceObject(backTrace);
        status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    status = KeWaitForSingleObject(&backTrace->CompletedEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   Timeout);
    if (status != STATUS_SUCCESS)
    {
        NTSTATUS removeStatus;

        NT_ASSERT(status == STATUS_TIMEOUT);

        removeStatus = KsiRemoveQueueApc(&backTrace->Apc);

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KsiRemoveQueueApc: %!STATUS!",
                      removeStatus);

        goto Exit;
    }

    NT_ASSERT(backTrace->CapturedFrames <= FramesToCapture);

    RtlCopyMemory(BackTrace,
                  backTrace->BackTrace,
                  backTrace->CapturedFrames * sizeof(PVOID));

    *CapturedFrames = backTrace->CapturedFrames;

    if (BackTraceHash)
    {
        *BackTraceHash = backTrace->BackTraceHash;
    }

Exit:

    if (backTrace)
    {
        KphDereferenceObject(backTrace);
    }

    return status;
}

/**
 * \brief Initialized stack back trace infrastructure.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphInitializeStackBackTrace(
    VOID
    )
{
    NTSTATUS status;
    KPH_OBJECT_TYPE_INFO typeInfo;
    SYSTEM_SINGLE_MODULE_INFORMATION info;

    PAGED_CODE_PASSIVE();

    typeInfo.Allocate = KphpStackBackTraceAllocate;
    typeInfo.Initialize = KphpStackBackTraceInitialize;
    typeInfo.Delete = KphpStackBackTraceDelete;
    typeInfo.Free = KphpStackBackTraceFree;

    KphCreateObjectType(&KphpStackBackTraceTypeName,
                        &typeInfo,
                        &KphpStackBackTraceType);

    RtlZeroMemory(&info, sizeof(info));

    info.TargetModuleAddress = (PVOID)KphInitializeStackBackTrace;
    status = ZwQuerySystemInformation(SystemSingleModuleInformation,
                                      &info,
                                      sizeof(info),
                                      NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ZwQuerySystemInformation failed: %!STATUS!",
                      status);

        return status;
    }

    KphpSelfImageBase = info.ExInfo.BaseInfo.ImageBase;
    KphpSelfImageEnd = Add2Ptr(KphpSelfImageBase, info.ExInfo.BaseInfo.ImageSize);

    RtlZeroMemory(&info, sizeof(info));

    info.TargetModuleAddress = (PVOID)KsiInitializeApc;
    status = ZwQuerySystemInformation(SystemSingleModuleInformation,
                                      &info,
                                      sizeof(info),
                                      NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ZwQuerySystemInformation failed: %!STATUS!",
                      status);

        return status;
    }

    KphpKsiImageBase = info.ExInfo.BaseInfo.ImageBase;
    KphpKsiImageEnd = Add2Ptr(KphpKsiImageBase, info.ExInfo.BaseInfo.ImageSize);

    return STATUS_SUCCESS;
}
