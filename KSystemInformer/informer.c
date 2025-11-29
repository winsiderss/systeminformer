/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023-2025
 *
 */

#include <kph.h>
#include <comms.h>
#include <informer.h>

#include <trace.h>

typedef struct _KPH_INFORMER_STATE
{
    KPH_INFORMER_OPTIONS Options;
    KPH_RATE_LIMIT RateLimit[KPH_INFORMER_COUNT];
} KPH_INFORMER_STATE, *PKPH_INFORMER_STATE;

KPH_PROTECTED_DATA_SECTION_PUSH();
static PKPH_OBJECT_TYPE KphpInformerStateType = NULL;
static PKPH_NPAGED_LOOKASIDE_OBJECT KphpInformerStateLookaside = NULL;
KPH_PROTECTED_DATA_SECTION_POP();
KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const UNICODE_STRING KphpInformerStateTypeName = RTL_CONSTANT_STRING(L"KphInformerState");
KPH_PROTECTED_DATA_SECTION_RO_POP();
static KPH_INFORMER_STATE_ATOMIC KphpInformerState = { .Atomic = KPH_ATOMIC_OBJECT_REF_INIT };

/**
 * \brief Allocates an informer state object.
 *
 * \return Pointer to informer state object, null on failure.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_IRQL_requires_max_(DISPATCH_LEVEL)
_Return_allocatesMem_size_(Size)
PVOID KSIAPI KphpAllocateInformerState(
    _In_ SIZE_T Size
    )
{
    PVOID object;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    NT_ASSERT(KphpInformerStateLookaside);
    NT_ASSERT(Size <= KphpInformerStateLookaside->L.Size);
    DBG_UNREFERENCED_PARAMETER(Size);

    object = KphAllocateFromNPagedLookasideObject(KphpInformerStateLookaside);
    if (object)
    {
        KphReferenceObject(KphpInformerStateLookaside);
    }

    return object;
}

/**
 * \brief Initializes an informer state object.
 *
 * \param[in] Object The informer state object to initialize.
 * \param[in] Parameter Optional settings to initialize with.
 *
 * \return STATUS_SUCCESS
 */
_Function_class_(KPH_TYPE_INITIALIZE_PROCEDURE)
_Must_inspect_result_
NTSTATUS
KSIAPI
KphpInitializeInformerState(
    _Inout_ PVOID Object,
    _In_opt_ PVOID Parameter
    )
{
    PKPH_INFORMER_STATE state;
    PKPH_INFORMER_SETTINGS settings;
    LARGE_INTEGER timeStamp;

    state = Object;
    settings = Parameter;

    KeQuerySystemTime(&timeStamp);

    if (settings)
    {
        state->Options.Flags = settings->Options.Flags;

        for (ULONG i = 0; i < KPH_INFORMER_COUNT; i++)
        {
            KphInitializeRateLimit(&settings->Policy[i],
                                   &timeStamp,
                                   &state->RateLimit[i]);
        }
    }
    else
    {
        KPH_RATE_LIMIT_POLICY policy = KPH_RATE_LIMIT_DENY_ALL;

        state->Options.Flags = 0;

        for (ULONG i = 0; i < KPH_INFORMER_COUNT; i++)
        {
            KphInitializeRateLimit(&policy, &timeStamp, &state->RateLimit[i]);
        }
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Frees an informer state object.
 *
 * \param[in] Object The informer state object to free.
 */
_Function_class_(KPH_TYPE_FREE_PROCEDURE)
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KSIAPI KphpFreeInformerState(
    _In_freesMem_ PVOID Object
    )
{
    KPH_NPAGED_CODE_DISPATCH_MAX();

    NT_ASSERT(KphpInformerStateLookaside);

    KphFreeToNPagedLookasideObject(KphpInformerStateLookaside, Object);
    KphDereferenceObject(KphpInformerStateLookaside);
}

/**
 * \brief Checks if an informer is allowed for a process.
 *
 * \param[in] Index The informer index to check.
 * \param[in] TimeStamp The current time stamp.
 * \param[in] Process The process context to check.
 *
 * \return TRUE if the informer is allowed, FALSE otherwise.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN KphInformerProcessAllowed(
    _In_ ULONG Index,
    _In_ PLARGE_INTEGER TimeStamp,
    _In_opt_ PKPH_PROCESS_CONTEXT Process
    )
{
    BOOLEAN allowed;
    PKPH_INFORMER_STATE state;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    if (!Process)
    {
        return TRUE;
    }

    if (!NT_VERIFY(Index < KPH_INFORMER_COUNT))
    {
        return FALSE;
    }

    state = KphAtomicReferenceObject(&Process->InformerState.Atomic);
    if (!state)
    {
        return TRUE;
    }

    allowed = KphRateLimitConsumeToken(&state->RateLimit[Index], TimeStamp);

    KphDereferenceObject(state);

    return allowed;
}

/**
 * \brief Checks if an informer is globally allowed.
 *
 * \param[in] Index The informer index to check.
 * \param[in] TimeStamp The current time stamp.
 *
 * \return TRUE if the informer is allowed, FALSE otherwise.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN KphInformerGlobalAllowed(
    _In_ ULONG Index,
    _In_ PLARGE_INTEGER TimeStamp
    )
{
    BOOLEAN allowed;
    PKPH_INFORMER_STATE state;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    if (!NT_VERIFY(Index < KPH_INFORMER_COUNT))
    {
        return FALSE;
    }

    state = KphAtomicReferenceObject(&KphpInformerState.Atomic);
    NT_ASSERT(state);

    allowed = KphRateLimitConsumeToken(&state->RateLimit[Index], TimeStamp);

    KphDereferenceObject(state);

    return allowed;
}

/**
 * \brief Checks if an informer is allowed.
 *
 * \param[in] Index The informer index to check.
 * \param[in] ActorProcess The actor process check.
 * \param[in] TargetProcess The target process check.
 *
 * \return TRUE if the informer is allowed, FALSE otherwise.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN KphInformerAllowed(
    _In_ ULONG Index,
    _In_opt_ PKPH_PROCESS_CONTEXT ActorProcess,
    _In_opt_ PKPH_PROCESS_CONTEXT TargetProcess
    )
{
    LARGE_INTEGER timeStamp;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    if (!KphGetConnectedClientCount())
    {
        return FALSE;
    }

    KeQuerySystemTime(&timeStamp);

    if (!KphInformerProcessAllowed(Index, &timeStamp, ActorProcess))
    {
        if (ActorProcess == TargetProcess)
        {
            return FALSE;
        }

        if (!KphInformerProcessAllowed(Index, &timeStamp, TargetProcess))
        {
            return FALSE;
        }
    }

    if (!KphInformerGlobalAllowed(Index, &timeStamp))
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * \brief Retrieves the active informer options.
 *
 * \param[in] ActorProcess The actor process check.
 * \param[in] TargetProcess The target process check.
 *
 * \return Active informer options.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
KPH_INFORMER_OPTIONS KphInformerOptions(
    _In_opt_ PKPH_PROCESS_CONTEXT ActorProcess,
    _In_opt_ PKPH_PROCESS_CONTEXT TargetProcess
    )
{
    PKPH_INFORMER_STATE state;
    KPH_INFORMER_OPTIONS options;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    options.Flags = 0;

    state = KphAtomicReferenceObject(&KphpInformerState.Atomic);
    NT_ASSERT(state);

    SetFlag(options.Flags, state->Options.Flags);

    KphDereferenceObject(state);

    if (ActorProcess)
    {
        state = KphAtomicReferenceObject(&ActorProcess->InformerState.Atomic);
        if (state)
        {
            SetFlag(options.Flags, state->Options.Flags);
            KphDereferenceObject(state);
        }
    }

    if (TargetProcess)
    {
        state = KphAtomicReferenceObject(&TargetProcess->InformerState.Atomic);
        if (state)
        {
            SetFlag(options.Flags, state->Options.Flags);
            KphDereferenceObject(state);
        }
    }

    return options;
}

KPH_PAGED_FILE();

/**
 * \brief Copies informer settings from state to mode.
 *
 * \param[out] Settings Receives the settings.
 * \param[in] State The informer state to copy from.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpInformerCopySettingsToMode(
    _Out_ PKPH_INFORMER_SETTINGS Settings,
    _In_ PKPH_INFORMER_STATE State,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;

    KPH_PAGED_CODE_PASSIVE();

    status = KphCopyToMode(Settings,
                           &State->Options,
                           sizeof(KPH_INFORMER_OPTIONS),
                           AccessMode);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    for (ULONG i = 0; i < KPH_INFORMER_COUNT; i++)
    {
        status = KphCopyToMode(&Settings->Policy[i],
                               &State->RateLimit[i].Policy,
                               sizeof(KPH_RATE_LIMIT_POLICY),
                               AccessMode);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Gets informer settings.
 *
 * \param[out] Settings Receives the settings.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetInformerSettings(
    _Out_ PKPH_INFORMER_SETTINGS Settings,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PKPH_INFORMER_STATE state;

    KPH_PAGED_CODE_PASSIVE();

    state = KphAtomicReferenceObject(&KphpInformerState.Atomic);
    NT_ASSERT(state);

    status = KphZeroModeMemory(Settings,
                               sizeof(KPH_INFORMER_SETTINGS),
                               AccessMode);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    status = KphpInformerCopySettingsToMode(Settings, state, AccessMode);

Exit:

    KphDereferenceObject(state);

    return status;
}

/**
 * \brief Sets informer settings.
 *
 * \param[in] Settings The settings to apply.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphSetInformerSettings(
    _In_ PKPH_INFORMER_SETTINGS Settings,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PKPH_INFORMER_SETTINGS settings;
    PKPH_INFORMER_STATE state;

    KPH_PAGED_CODE_PASSIVE();

    state = NULL;

    settings = KphAllocatePaged(sizeof(KPH_INFORMER_SETTINGS),
                                KPH_TAG_INFORMER_SETTINGS);
    if (!settings)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "Failed to allocate informer settings");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphCopyFromMode(settings,
                             Settings,
                             sizeof(KPH_INFORMER_SETTINGS),
                             AccessMode);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    status = KphCreateObject(KphpInformerStateType,
                             sizeof(KPH_INFORMER_STATE),
                             &state,
                             settings);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphCreateObject failed: %!STATUS!",
                      status);

        state = NULL;
        goto Exit;
    }

    KphAtomicAssignObjectReference(&KphpInformerState.Atomic, state);

Exit:

    if (state)
    {
        KphDereferenceObject(state);
    }

    if (settings)
    {
        KphFree(settings, KPH_TAG_INFORMER_SETTINGS);
    }

    return status;
}

/**
 * \brief Gets informer settings for a process.
 *
 * \param[in] ProcessHandle Handle to the process to get settings of.
 * \param[out] Settings Receives the settings.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetInformerProcessSettings(
    _In_ HANDLE ProcessHandle,
    _Out_ PKPH_INFORMER_SETTINGS Settings,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS processObject;
    PKPH_PROCESS_CONTEXT processContext;
    PKPH_INFORMER_STATE state;

    KPH_PAGED_CODE_PASSIVE();

    processObject = NULL;
    processContext = NULL;
    state = NULL;

    status = KphZeroModeMemory(Settings,
                               sizeof(KPH_INFORMER_SETTINGS),
                               AccessMode);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    status = ObReferenceObjectByHandle(ProcessHandle,
                                       0,
                                       *PsProcessType,
                                       AccessMode,
                                       &processObject,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        processObject = NULL;
        goto Exit;
    }

    processContext = KphGetEProcessContext(processObject);
    if (!processContext)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphGetEProcessContext return null.");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    state = KphAtomicReferenceObject(&processContext->InformerState.Atomic);
    if (!state)
    {
        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    status = KphpInformerCopySettingsToMode(Settings, state, AccessMode);

Exit:

    if (processContext)
    {
        KphDereferenceObject(processContext);
    }

    if (processObject)
    {
        ObDereferenceObject(processObject);
    }

    if (state)
    {
        KphDereferenceObject(state);
    }

    return status;
}

typedef struct _KPH_SET_INFORMER_PROCESS_SETTINGS_CONTEXT
{
    NTSTATUS Status;
    PKPH_INFORMER_SETTINGS Settings;
} KPH_SET_INFORMER_PROCESS_SETTINGS_CONTEXT, *PKPH_SET_INFORMER_PROCESS_SETTINGS_CONTEXT;

/**
 * \brief Callback for setting process informer settings.
 *
 * \param[in] Process The process to set the informer settings for.
 * \param[in] Parameter The settings to apply.
 *
 * \return FALSE
 */
_Function_class_(KPH_ENUM_PROCESS_CONTEXTS_CALLBACK)
_Must_inspect_result_
BOOLEAN KSIAPI KphpSetInformerProcessSettings(
    _In_ PKPH_PROCESS_CONTEXT Process,
    _In_opt_ PVOID Parameter
    )
{
    NTSTATUS status;
    PKPH_SET_INFORMER_PROCESS_SETTINGS_CONTEXT context;
    PKPH_PROCESS_STATE state;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Parameter);

    context = Parameter;

    status = KphCreateObject(KphpInformerStateType,
                             sizeof(KPH_INFORMER_STATE),
                             &state,
                             context->Settings);
    if (!NT_SUCCESS(status))
    {
        if (NT_SUCCESS(context->Status))
        {
            context->Status = status;
        }

        return FALSE;
    }

    KphAtomicAssignObjectReference(&Process->InformerState.Atomic, state);

    return FALSE;
}

/**
 * \brief Sets informer settings for a process.
 *
 * \param[in] ProcessHandle Optional handle to the process to set settings of.
 *  If not provided the settings are applied to all processes.
 * \param[in] Settings The settings to apply.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphSetInformerProcessSettings(
    _In_opt_ HANDLE ProcessHandle,
    _In_ PKPH_INFORMER_SETTINGS Settings,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS processObject;
    PKPH_PROCESS_CONTEXT processContext;
    PKPH_INFORMER_SETTINGS settings;
    KPH_SET_INFORMER_PROCESS_SETTINGS_CONTEXT context;

    KPH_PAGED_CODE_PASSIVE();

    processObject = NULL;
    processContext = NULL;

    settings = KphAllocatePaged(sizeof(KPH_INFORMER_SETTINGS),
                                KPH_TAG_INFORMER_SETTINGS);
    if (!settings)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "Failed to allocate informer settings");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphCopyFromMode(settings,
                             Settings,
                             sizeof(KPH_INFORMER_SETTINGS),
                             AccessMode);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    if (ProcessHandle)
    {
        status = ObReferenceObjectByHandle(ProcessHandle,
                                           0,
                                           *PsProcessType,
                                           AccessMode,
                                           &processObject,
                                           NULL);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "ObReferenceObjectByHandle failed: %!STATUS!",
                          status);

            processObject = NULL;
            goto Exit;
        }

        processContext = KphGetEProcessContext(processObject);
        if (!processContext)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "KphGetEProcessContext return null.");

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        context.Status = STATUS_SUCCESS;
        context.Settings = settings;

        (VOID)KphpSetInformerProcessSettings(processContext, &context);

        status = context.Status;
    }
    else
    {
        context.Status = STATUS_SUCCESS;
        context.Settings = settings;

        KphEnumerateProcessContexts(KphpSetInformerProcessSettings, &context);

        status = context.Status;
    }

Exit:

    if (processContext)
    {
        KphDereferenceObject(processContext);
    }

    if (processObject)
    {
        ObDereferenceObject(processObject);
    }

    if (settings)
    {
        KphFree(settings, KPH_TAG_INFORMER_SETTINGS);
    }

    return status;
}

/**
 * \brief Gets informer statistics.
 *
 * \param[in] ProcessHandle Optional handle to the process to get stats of. If
 *  not provided the global stats are returned.
 * \param[out] Stats Receives the informer statistics.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetInformerStats(
    _In_opt_ HANDLE ProcessHandle,
    _Out_ PKPH_INFORMER_STATS Stats,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PKPH_INFORMER_STATE state;
    PEPROCESS processObject;
    PKPH_PROCESS_CONTEXT processContext;

    KPH_PAGED_CODE_PASSIVE();

    state = NULL;
    processObject = NULL;
    processContext = NULL;

    status = KphZeroModeMemory(Stats, sizeof(KPH_INFORMER_STATS), AccessMode);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    if (ProcessHandle)
    {
        status = ObReferenceObjectByHandle(ProcessHandle,
                                           0,
                                           *PsProcessType,
                                           AccessMode,
                                           &processObject,
                                           NULL);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "ObReferenceObjectByHandle failed: %!STATUS!",
                          status);

            processObject = NULL;
            goto Exit;
        }

        processContext = KphGetEProcessContext(processObject);
        if (!processContext)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "KphGetEProcessContext return null.");

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        state = KphAtomicReferenceObject(&processContext->InformerState.Atomic);
        if (!state)
        {
            status = STATUS_NOT_FOUND;
            goto Exit;
        }
    }
    else
    {
        state = KphAtomicReferenceObject(&KphpInformerState.Atomic);
    }

    NT_ASSERT(state);

    for (ULONG i = 0; i < KPH_INFORMER_COUNT; i++)
    {
        status = KphCopyToMode(&Stats->RateLimit[i].Policy,
                               &state->RateLimit[i].Policy,
                               sizeof(KPH_RATE_LIMIT_POLICY),
                               AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }

        status = KphWriteLong64ToMode(&Stats->RateLimit[i].Allowed,
                                      ReadNoFence64(&state->RateLimit[i].Allowed),
                                      AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }

        status = KphWriteLong64ToMode(&Stats->RateLimit[i].Dropped,
                                      ReadNoFence64(&state->RateLimit[i].Dropped),
                                      AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }

        status = KphWriteLong64ToMode(&Stats->RateLimit[i].CasMiss,
                                      ReadNoFence64(&state->RateLimit[i].CasMiss),
                                      AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

Exit:

    if (processContext)
    {
        KphDereferenceObject(processContext);
    }

    if (processObject)
    {
        ObDereferenceObject(processObject);
    }

    if (state)
    {
        KphDereferenceObject(state);
    }

    return status;
}

/**
 * \brief Cleans up informer infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupInformer(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphAtomicAssignObjectReference(&KphpInformerState.Atomic, NULL);

    if (KphpInformerStateLookaside)
    {
        KphDereferenceObject(KphpInformerStateLookaside);
    }
}

/**
 * \brief Initialize the informer infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphInitializeInformer(
    VOID
    )
{
    NTSTATUS status;
    PKPH_INFORMER_STATE state;
    KPH_OBJECT_TYPE_INFO typeInfo;

    KPH_PAGED_CODE_PASSIVE();

    state = NULL;

    status = KphCreateNPagedLookasideObject(&KphpInformerStateLookaside,
                                            KphAddObjectHeaderSize(sizeof(KPH_INFORMER_STATE)),
                                            KPH_TAG_INFORMER_STATE);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "KphCreatePagedLookasideObject failed: %!STATUS!",
                      status);

        KphpInformerStateLookaside = NULL;
        goto Exit;
    }

    typeInfo.Allocate = KphpAllocateInformerState;
    typeInfo.Initialize = KphpInitializeInformerState;
    typeInfo.Delete = NULL;
    typeInfo.Free = KphpFreeInformerState;

    KphCreateObjectType(&KphpInformerStateTypeName,
                        &typeInfo,
                        &KphpInformerStateType);

    status = KphCreateObject(KphpInformerStateType,
                             sizeof(KPH_INFORMER_STATE),
                             &state,
                             NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      TRACKING,
                      "KphCreateObject failed: %!STATUS!",
                      status);

        state = NULL;
        goto Exit;
    }

    KphAtomicAssignObjectReference(&KphpInformerState.Atomic, state);

Exit:

    if (state)
    {
        KphDereferenceObject(state);
    }

    return status;
}
