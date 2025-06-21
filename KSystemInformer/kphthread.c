/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2025
 *
 */

#include <kph.h>

#include <trace.h>

typedef struct _KPH_THREAD_START_CONTEXT
{
    PKPH_THREAD_START_ROUTINE StartRoutine;
    PVOID Parameter;
    UNICODE_STRING ThreadName;
} KPH_THREAD_START_CONTEXT, *PKPH_THREAD_START_CONTEXT;

KPH_PROTECTED_DATA_SECTION_PUSH();
static HANDLE KphpKsiSystemProcessHandle = NULL;
KPH_PROTECTED_DATA_SECTION_POP();

KPH_PAGED_FILE();

/**
 * \brief Thread start routine for system threads.
 *
 * \param[in] Context Pointer to a KPH_THREAD_START_CONTEXT.
 */
_Function_class_(KSTART_ROUTINE)
_IRQL_requires_same_
VOID KphpThreadStartRoutine(
    _In_ PVOID StartContext
    )
{
    NTSTATUS status;
    PKPH_THREAD_START_CONTEXT context;
    PKPH_THREAD_START_ROUTINE startRoutine;
    PVOID parameter;

    KPH_PAGED_CODE_PASSIVE();

    context = StartContext;

    startRoutine = context->StartRoutine;
    parameter = context->Parameter;

    if (context->ThreadName.Length && !KphParameterFlags.DisableThreadNames)
    {
        status = ZwSetInformationThread(ZwCurrentThread(),
                                        ThreadNameInformation,
                                        &context->ThreadName,
                                        sizeof(UNICODE_STRING));

        if (NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "Set thread (%lu) name \"%wZ\"",
                          HandleToULong(PsGetCurrentThreadId()),
                          &context->ThreadName);
        }
        else
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "Failed to set thread (%lu) name \"%wZ\": %!STATUS!",
                          HandleToULong(PsGetCurrentThreadId()),
                          &context->ThreadName,
                          status);
        }
    }

    KphFree(context, KPH_TAG_THREAD_START_CONTEXT);

    status = startRoutine(parameter);

    PsTerminateSystemThread(status);
}

/**
 * \brief Creates a system thread.
 *
 * \param[out] ThreadHandle Optionally receives a handle to the created thread.
 * The caller must close the handle once the handle is no longer in use. The
 * handle is a kernel handle with THREAD_ALL_ACCESS access rights.
 * \param[out] ThreadObject Optionally receives a the created thread object.
 * The caller must dereference the object once it is no longer in use.
 * \param[in] StartRoutine The entry point for the newly created system thread.
 * \param[in] Parameter Argument that is passed to the thread.
 * \param[in] ThreadName Optionally specifies a name for the thread.
 * \param[in] Flags Optional flags that control the thread creation.
 * KPH_CREATE_SYSTEM_THREAD_IN_KSI_PROCESS if possible the thread will be
 * created in a dedicated process that is used for system threads.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS KphCreateSystemThread(
    _Out_opt_ PHANDLE ThreadHandle,
    _Out_opt_ PETHREAD* ThreadObject,
    _In_ PKPH_THREAD_START_ROUTINE StartRoutine,
    _In_opt_ _When_(return >= 0, __drv_aliasesMem) PVOID Parameter,
    _In_opt_ PCUNICODE_STRING ThreadName,
    _In_ ULONG Flags
    )
{
    NTSTATUS status;
    HANDLE threadHandle;
    ULONG contextLength;
    PKPH_THREAD_START_CONTEXT context;
    HANDLE processHandle;

    KPH_PAGED_CODE_PASSIVE();

    if (ThreadHandle)
    {
        *ThreadHandle = NULL;
    }

    if (ThreadObject)
    {
        *ThreadObject = NULL;
    }

    threadHandle = NULL;

    contextLength = sizeof(KPH_THREAD_START_CONTEXT);
    if (ThreadName)
    {
        contextLength += ThreadName->Length;
    }

    context = KphAllocatePaged(contextLength, KPH_TAG_THREAD_START_CONTEXT);
    if (!context)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "Failed to allocate thread start context.");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    context->StartRoutine = StartRoutine;
    context->Parameter = Parameter;

    if (ThreadName)
    {
        context->ThreadName.MaximumLength = ThreadName->Length;
        context->ThreadName.Buffer = Add2Ptr(context, sizeof(KPH_THREAD_START_CONTEXT));
        RtlCopyUnicodeString(&context->ThreadName, ThreadName);
    }

    if (FlagOn(Flags, KPH_CREATE_SYSTEM_THREAD_IN_KSI_PROCESS))
    {
        processHandle = KphpKsiSystemProcessHandle;
    }
    else
    {
        processHandle = NULL;
    }

    status = PsCreateSystemThread(&threadHandle,
                                  THREAD_ALL_ACCESS,
                                  NULL,
                                  processHandle,
                                  NULL,
                                  KphpThreadStartRoutine,
                                  context);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "PsCreateSystemThread failed: %!STATUS!",
                      status);

        threadHandle = NULL;
        goto Exit;
    }

    context = NULL;

    if (ThreadObject)
    {
        NT_VERIFY(NT_SUCCESS(ObReferenceObjectByHandle(threadHandle,
                                                       THREAD_ALL_ACCESS,
                                                       *PsThreadType,
                                                       KernelMode,
                                                       ThreadObject,
                                                       NULL)));
    }

    if (ThreadHandle)
    {
        *ThreadHandle = threadHandle;
        threadHandle = NULL;
    }

Exit:

    if (threadHandle)
    {
        ObCloseHandle(threadHandle, KernelMode);
    }

    if (context)
    {
        KphFree(context, KPH_TAG_THREAD_START_CONTEXT);
    }

    return status;
}

/**
 * \brief Cleans up the threading subsystem.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupThreading(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (KphpKsiSystemProcessHandle)
    {
        ObCloseHandle(KphpKsiSystemProcessHandle, KernelMode);
    }
}

/**
 * \brief Initializes threading subsystem.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeThreading(
    VOID
    )
{
    NTSTATUS status;

    KPH_PAGED_CODE_PASSIVE();

    if (KphParameterFlags.DisableSystemProcess)
    {
        KphTracePrint(TRACE_LEVEL_INFORMATION,
                      GENERAL,
                      "System process disabled.");

        return;
    }

    status = KsiInitializeSystemProcess(KphSystemProcessName);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KsiInitializeSystemProcess failed: %!STATUS!",
                      status);

        return;
    }

    status = ObOpenObjectByPointer(KsiSystemProcess,
                                   OBJ_KERNEL_HANDLE,
                                   NULL,
                                   PROCESS_ALL_ACCESS,
                                   *PsProcessType,
                                   KernelMode,
                                   &KphpKsiSystemProcessHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "ObOpenObjectByPointer failed: %!STATUS!",
                      status);

        KphpKsiSystemProcessHandle = NULL;
    }
}
