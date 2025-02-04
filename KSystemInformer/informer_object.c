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
#include <comms.h>
#include <kphmsgdyn.h>

#include <trace.h>

typedef union _KPH_OB_OPTIONS
{
    UCHAR Flags;
    struct
    {
        UCHAR PreEnabled : 1;
        UCHAR PostEnabled : 1;
        UCHAR EnableStackTraces : 1;
        UCHAR Spare : 5;
    };
} KPH_OB_OPTIONS, *PKPH_OB_OPTIONS;

typedef struct _KPH_OB_CALL_CONTEXT
{
    ULONG64 PreSequence;
    LARGE_INTEGER PreTimeStamp;
    KPH_OB_OPTIONS Options;
    ACCESS_MASK DesiredAccess;
    ACCESS_MASK OriginalDesiredAccess;
    HANDLE SourceProcessId;
    HANDLE TargetProcessId;
} KPH_OB_CALL_CONTEXT, *PKPH_OB_CALL_CONTEXT;

KPH_PROTECTED_DATA_SECTION_PUSH();
static PVOID KphpObRegistrationHandle = NULL;
KPH_PROTECTED_DATA_SECTION_POP();
static PAGED_LOOKASIDE_LIST KphpObCallContextLookaside = { 0 };

KPH_PAGED_FILE();

static ULONG64 KphpObSequence = 0;

/**
 * \brief Retrieves the object operation options.
 *
 * \param[in] Info Object manager pre-operation callback information.
 *
 * \return The object operation options.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
KPH_OB_OPTIONS KphpObGetOptions(
    _In_ POB_PRE_OPERATION_INFORMATION Info
    )
{
    KPH_OB_OPTIONS options;
    PKPH_PROCESS_CONTEXT process;

    KPH_PAGED_CODE_PASSIVE();

    options.Flags = 0;

    if (Info->KernelHandle)
    {
        process = KphGetSystemProcessContext();
    }
    else
    {
        process = KphGetCurrentProcessContext();
    }

#define KPH_OB_SETTING(name)                                                   \
    if (KphInformerEnabled(HandlePre##name, process))                          \
    {                                                                          \
        options.PreEnabled = TRUE;                                             \
    }                                                                          \
    if (KphInformerEnabled(HandlePost##name, process))                         \
    {                                                                          \
        options.PostEnabled = TRUE;                                            \
    }

    if (Info->Operation == OB_OPERATION_HANDLE_CREATE)
    {
        if (Info->ObjectType == *PsProcessType)
        {
            KPH_OB_SETTING(CreateProcess);
        }
        else if (Info->ObjectType == *PsThreadType)
        {
            KPH_OB_SETTING(CreateThread);
        }
        else
        {
            NT_ASSERT(Info->ObjectType == *ExDesktopObjectType);

            KPH_OB_SETTING(CreateDesktop);
        }
    }
    else
    {
        if (Info->ObjectType == *PsProcessType)
        {
            KPH_OB_SETTING(DuplicateProcess);
        }
        else if (Info->ObjectType == *PsThreadType)
        {
            KPH_OB_SETTING(DuplicateThread);
        }
        else
        {
            NT_ASSERT(Info->ObjectType == *ExDesktopObjectType);

            KPH_OB_SETTING(DuplicateDesktop);
        }
    }

    if (options.PreEnabled || options.PostEnabled)
    {
        options.EnableStackTraces = KphInformerEnabled(EnableStackTraces, process);
    }

    if (process)
    {
        KphDereferenceObject(process);
    }

    return options;
}

/**
 * \brief Retrieves the message ID for the object pre-operation callback.
 *
 * \param[in] Info Object manager pre-operation callback information.
 *
 * \return The message ID for the callback.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
KPH_MESSAGE_ID KphpObPreGetMessageId(
    _In_ POB_PRE_OPERATION_INFORMATION Info
    )
{
    KPH_MESSAGE_ID messageId;

    KPH_PAGED_CODE_PASSIVE();

    if (Info->Operation == OB_OPERATION_HANDLE_CREATE)
    {
        if (Info->ObjectType == *PsProcessType)
        {
            messageId = KphMsgHandlePreCreateProcess;
        }
        else if (Info->ObjectType == *PsThreadType)
        {
            messageId = KphMsgHandlePreCreateThread;
        }
        else
        {
            NT_ASSERT(Info->ObjectType == *ExDesktopObjectType);

            messageId = KphMsgHandlePreCreateDesktop;
        }
    }
    else
    {
        NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_DUPLICATE);

        if (Info->ObjectType == *PsProcessType)
        {
            messageId = KphMsgHandlePreDuplicateProcess;
        }
        else if (Info->ObjectType == *PsThreadType)
        {
            messageId = KphMsgHandlePreDuplicateThread;
        }
        else
        {
            NT_ASSERT(Info->ObjectType == *ExDesktopObjectType);

            messageId = KphMsgHandlePreDuplicateDesktop;
        }
    }

    return messageId;
}

/**
 * \brief Retrieves the message ID for the object post-operation callback.
 *
 * \param[in] Info Object manager post-operation callback information.
 *
 * \return The message ID for the callback.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
KPH_MESSAGE_ID KphpObPostGetMessageId(
    _In_ POB_POST_OPERATION_INFORMATION Info
    )
{
    KPH_MESSAGE_ID messageId;

    KPH_PAGED_CODE_PASSIVE();

    if (Info->Operation == OB_OPERATION_HANDLE_CREATE)
    {
        if (Info->ObjectType == *PsProcessType)
        {
            messageId = KphMsgHandlePostCreateProcess;
        }
        else if (Info->ObjectType == *PsThreadType)
        {
            messageId = KphMsgHandlePostCreateThread;
        }
        else
        {
            NT_ASSERT(Info->ObjectType == *ExDesktopObjectType);

            messageId = KphMsgHandlePostCreateDesktop;
        }
    }
    else
    {
        NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_DUPLICATE);

        if (Info->ObjectType == *PsProcessType)
        {
            messageId = KphMsgHandlePostDuplicateProcess;
        }
        else if (Info->ObjectType == *PsThreadType)
        {
            messageId = KphMsgHandlePostDuplicateThread;
        }
        else
        {
            NT_ASSERT(Info->ObjectType == *ExDesktopObjectType);

            messageId = KphMsgHandlePostDuplicateDesktop;
        }
    }

    return messageId;
}

/**
 * \brief Copies the object name into a message for registered object callbacks.
 *
 * \details Used to populate the desktop name for desktop object callbacks.
 *
 * \param[in,out] Message The message to populate.
 * \param[in] Object The object for which the name is populated in the message.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpObCopyObjectName(
    _Inout_ PKPH_MESSAGE Message,
    _In_ PVOID Object
    )
{
    NTSTATUS status;
    POBJECT_NAME_INFORMATION info;
    ULONG length;

    KPH_PAGED_CODE_PASSIVE();

    info = NULL;

    status = ObQueryNameString(Object, NULL, 0, &length);
    if ((status != STATUS_INFO_LENGTH_MISMATCH) &&
        (status != STATUS_BUFFER_TOO_SMALL))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "ObQueryNameString: %!STATUS!",
                      status);

        goto Exit;
    }

    info = KphAllocatePaged(length, KPH_TAG_OB_OBJECT_NAME);
    if (!info)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to allocate buffer for object name");

        goto Exit;
    }

    status = ObQueryNameString(Object, info, length, &length);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "ObQueryNameString failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphMsgDynAddUnicodeString(Message,
                                       KphMsgFieldObjectName,
                                       &info->Name);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphMsgDynAddUnicodeString failed: %!STATUS!",
                      status);
    }

Exit:

    if (info)
    {
        KphFree(info, KPH_TAG_OB_OBJECT_NAME);
    }
}

/**
 * \brief Fills the object pre-operation callback message.
 *
 * \param[in,out] Message The message to populate.
 * \param[in] Info Object manager pre-operation callback information.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpObPreFillMessage(
    _Inout_ PKPH_MESSAGE Message,
    _In_ POB_PRE_OPERATION_INFORMATION Info
    )
{
    KPH_PAGED_CODE_PASSIVE();

    Message->Kernel.Handle.ContextClientId.UniqueProcess = PsGetCurrentProcessId();
    Message->Kernel.Handle.ContextClientId.UniqueThread = PsGetCurrentThreadId();
    Message->Kernel.Handle.ContextProcessStartKey = KphGetCurrentProcessStartKey();
    Message->Kernel.Handle.Flags = Info->Flags;
    Message->Kernel.Handle.Object = Info->Object;

    if (Info->Operation == OB_OPERATION_HANDLE_CREATE)
    {
        POB_PRE_CREATE_HANDLE_INFORMATION params;

        params = &Info->Parameters->CreateHandleInformation;

        Message->Kernel.Handle.Pre.DesiredAccess = params->DesiredAccess;
        Message->Kernel.Handle.Pre.OriginalDesiredAccess = params->OriginalDesiredAccess;

        if (Info->ObjectType == *PsProcessType)
        {
            Message->Kernel.Handle.Pre.Create.Process.ProcessId =
                PsGetProcessId(Info->Object);
        }
        else if (Info->ObjectType == *PsThreadType)
        {
            Message->Kernel.Handle.Pre.Create.Thread.ClientId.UniqueProcess =
                PsGetProcessId(PsGetThreadProcess(Info->Object));

            Message->Kernel.Handle.Pre.Create.Thread.ClientId.UniqueThread =
                PsGetThreadId(Info->Object);

            Message->Kernel.Handle.Pre.Create.Thread.SubProcessTag =
                KphGetThreadSubProcessTag(Info->Object);
        }
        else
        {
            NT_ASSERT(Info->ObjectType == *ExDesktopObjectType);
            KphpObCopyObjectName(Message, Info->Object);
        }
    }
    else
    {
        POB_PRE_DUPLICATE_HANDLE_INFORMATION params;

        NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_DUPLICATE);

        params = &Info->Parameters->DuplicateHandleInformation;

        Message->Kernel.Handle.Duplicate = TRUE;

        Message->Kernel.Handle.Pre.DesiredAccess = params->DesiredAccess;
        Message->Kernel.Handle.Pre.OriginalDesiredAccess = params->OriginalDesiredAccess;

        Message->Kernel.Handle.Pre.Duplicate.SourceProcessId = PsGetProcessId(params->SourceProcess);
        Message->Kernel.Handle.Pre.Duplicate.TargetProcessId = PsGetProcessId(params->TargetProcess);

        if (Info->ObjectType == *PsProcessType)
        {
            Message->Kernel.Handle.Pre.Duplicate.Process.ProcessId =
                PsGetProcessId(Info->Object);
        }
        else if (Info->ObjectType == *PsThreadType)
        {
            Message->Kernel.Handle.Pre.Duplicate.Thread.ClientId.UniqueProcess =
                PsGetProcessId(PsGetThreadProcess(Info->Object));

            Message->Kernel.Handle.Pre.Duplicate.Thread.ClientId.UniqueThread =
                PsGetThreadId(Info->Object);

            Message->Kernel.Handle.Pre.Duplicate.Thread.SubProcessTag =
                KphGetThreadSubProcessTag(Info->Object);
        }
        else
        {
            NT_ASSERT(Info->ObjectType == *ExDesktopObjectType);
            KphpObCopyObjectName(Message, Info->Object);
        }
    }
}

/**
 * \brief Fills the object post-operation callback message.
 *
 * \param[in,out] Message The message to populate.
 * \param[in] Info Object manager post-operation callback information.
 * \param[in] Context The object call context.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpObPostFillMessage(
    _Inout_ PKPH_MESSAGE Message,
    _In_ POB_POST_OPERATION_INFORMATION Info,
    _In_ PKPH_OB_CALL_CONTEXT Context
    )
{
    KPH_PAGED_CODE_PASSIVE();

    Message->Kernel.Handle.ContextClientId.UniqueProcess = PsGetCurrentProcessId();
    Message->Kernel.Handle.ContextClientId.UniqueThread = PsGetCurrentThreadId();
    Message->Kernel.Handle.ContextProcessStartKey = KphGetCurrentProcessStartKey();
    Message->Kernel.Handle.ContextThreadSubProcessTag = KphGetCurrentThreadSubProcessTag();
    Message->Kernel.Handle.Flags = Info->Flags;
    Message->Kernel.Handle.Object = Info->Object;

    Message->Kernel.Handle.PostOperation = TRUE;
    Message->Kernel.Handle.Post.PreSequence = Context->PreSequence;
    Message->Kernel.Handle.Post.PreTimeStamp = Context->PreTimeStamp;
    Message->Kernel.Handle.Post.ReturnStatus = Info->ReturnStatus;
    Message->Kernel.Handle.Post.DesiredAccess = Context->DesiredAccess;
    Message->Kernel.Handle.Post.OriginalDesiredAccess = Context->OriginalDesiredAccess;

    if (Info->Operation == OB_OPERATION_HANDLE_CREATE)
    {
        POB_POST_CREATE_HANDLE_INFORMATION params;

        params = &Info->Parameters->CreateHandleInformation;

        Message->Kernel.Handle.Post.GrantedAccess = params->GrantedAccess;

        if (Info->ObjectType == *PsProcessType)
        {
            Message->Kernel.Handle.Post.Create.Process.ProcessId =
                PsGetProcessId(Info->Object);
        }
        else if (Info->ObjectType == *PsThreadType)
        {
            Message->Kernel.Handle.Post.Create.Thread.ClientId.UniqueProcess =
                PsGetProcessId(PsGetThreadProcess(Info->Object));

            Message->Kernel.Handle.Post.Create.Thread.ClientId.UniqueThread =
                PsGetThreadId(Info->Object);

            Message->Kernel.Handle.Post.Create.Thread.SubProcessTag =
                KphGetThreadSubProcessTag(Info->Object);
        }
        else
        {
            NT_ASSERT(Info->ObjectType == *ExDesktopObjectType);
            KphpObCopyObjectName(Message, Info->Object);
        }
    }
    else
    {
        POB_POST_DUPLICATE_HANDLE_INFORMATION params;

        NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_DUPLICATE);

        params = &Info->Parameters->DuplicateHandleInformation;

        Message->Kernel.Handle.Duplicate = TRUE;

        Message->Kernel.Handle.Post.GrantedAccess = params->GrantedAccess;

        Message->Kernel.Handle.Post.Duplicate.SourceProcessId = Context->SourceProcessId;
        Message->Kernel.Handle.Post.Duplicate.TargetProcessId = Context->TargetProcessId;

        if (Info->ObjectType == *PsProcessType)
        {
            Message->Kernel.Handle.Post.Duplicate.Process.ProcessId =
                PsGetProcessId(Info->Object);
        }
        else if (Info->ObjectType == *PsThreadType)
        {
            Message->Kernel.Handle.Post.Duplicate.Thread.ClientId.UniqueProcess =
                PsGetProcessId(PsGetThreadProcess(Info->Object));

            Message->Kernel.Handle.Post.Duplicate.Thread.ClientId.UniqueThread =
                PsGetThreadId(Info->Object);

            Message->Kernel.Handle.Post.Duplicate.Thread.SubProcessTag =
                KphGetThreadSubProcessTag(Info->Object);
        }
        else
        {
            NT_ASSERT(Info->ObjectType == *ExDesktopObjectType);
            KphpObCopyObjectName(Message, Info->Object);
        }
    }
}

/**
 * \brief Sends an object post-operation callback message.
 *
 * \param[in] Info Object manager post-operation callback information.
 * \param[in] Sequence The sequence number for the message.
 * \param[in] Context The object call context.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpObPostOpSend(
    _In_ POB_POST_OPERATION_INFORMATION Info,
    _In_ ULONG64 Sequence,
    _In_ PKPH_OB_CALL_CONTEXT Context
    )
{
    PKPH_MESSAGE msg;

    KPH_PAGED_CODE_PASSIVE();

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to allocate message");

        return;
    }

    KphMsgInit(msg, KphpObPostGetMessageId(Info));

    msg->Kernel.Handle.Sequence = Sequence;

    KphpObPostFillMessage(msg, Info, Context);

    if (Context->Options.EnableStackTraces)
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);
}

/**
 * \brief Object manager post-operation callback.
 *
 * \param[in] Context Not used.
 * \param[in] Info The object post-operation information.
 */
_Function_class_(POB_POST_OPERATION_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpObPostOp(
    _In_ PVOID Context,
    _In_ POB_POST_OPERATION_INFORMATION Info
    )
{
    PKPH_OB_CALL_CONTEXT context;
    ULONG64 sequence;

    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Context);

    sequence = InterlockedIncrementU64(&KphpObSequence);

    context = Info->CallContext;
    if (context)
    {
        KphpObPostOpSend(Info, sequence, context);

        KphFreeToPagedLookaside(&KphpObCallContextLookaside, context);
    }
}

/**
 * \brief Sets the object callback call context for the post-operation to use.
 *
 * \param[in] Info Object manager pre-operation callback information.
 * \param[in] Options Object operation options to use.
 * \param[in] Sequence The pre-operation sequence number.
 * \param[in] TimeStamp The pre-operation time stamp.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpObPreOpSetCallContext(
    _In_ POB_PRE_OPERATION_INFORMATION Info,
    _In_ PKPH_OB_OPTIONS Options,
    _In_ ULONG64 Sequence,
    _In_ PLARGE_INTEGER TimeStamp
    )
{
    PKPH_OB_CALL_CONTEXT context;

    KPH_PAGED_CODE_PASSIVE();

    context = KphAllocateFromPagedLookaside(&KphpObCallContextLookaside);
    if (!context)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to allocate call context");

        return;
    }

    context->PreSequence = Sequence;
    context->PreTimeStamp = *TimeStamp;
    context->Options.Flags = Options->Flags;

    if (Info->Operation == OB_OPERATION_HANDLE_CREATE)
    {
        POB_PRE_CREATE_HANDLE_INFORMATION params;

        params = &Info->Parameters->CreateHandleInformation;

        context->DesiredAccess = params->DesiredAccess;
        context->OriginalDesiredAccess = params->OriginalDesiredAccess;
    }
    else
    {
        POB_PRE_DUPLICATE_HANDLE_INFORMATION params;

        NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_DUPLICATE);

        params = &Info->Parameters->DuplicateHandleInformation;

        context->DesiredAccess = params->DesiredAccess;
        context->OriginalDesiredAccess = params->OriginalDesiredAccess;
        context->SourceProcessId = PsGetProcessId(params->SourceProcess);
        context->TargetProcessId = PsGetProcessId(params->TargetProcess);
    }

    Info->CallContext = context;
}

/**
 * \brief Sends an object pre-operation callback message.
 *
 * \param[in] Info Object manager pre-operation callback information.
 * \param[in] Options Object operation options to use.
 * \param[in] Sequence The sequence number for the message.
 * \param[out] TimeStamp Receives the time stamp of the pre-operation message.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpObPreOpSend(
    _In_ POB_PRE_OPERATION_INFORMATION Info,
    _In_ PKPH_OB_OPTIONS Options,
    _In_ ULONG64 Sequence,
    _Out_ PLARGE_INTEGER TimeStamp
    )
{
    PKPH_MESSAGE msg;

    KPH_PAGED_CODE_PASSIVE();

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to allocate message");

        KeQuerySystemTime(TimeStamp);
        return;
    }

    KphMsgInit(msg, KphpObPreGetMessageId(Info));

    *TimeStamp = msg->Header.TimeStamp;

    msg->Kernel.Handle.Sequence = Sequence;

    KphpObPreFillMessage(msg, Info);

    if (Options->EnableStackTraces)
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);
}

/**
 * \brief Object manager pre-operation callback.
 *
 * \param[in] Context Not used.
 * \param[in,out] Info The object pre-operation information.
 *
 * \return OB_PREOP_SUCCESS
 */
_Function_class_(POB_PRE_OPERATION_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
OB_PREOP_CALLBACK_STATUS KphpObPreOp(
    _In_ PVOID Context,
    _Inout_ POB_PRE_OPERATION_INFORMATION Info
    )
{
    KPH_OB_OPTIONS options;
    ULONG64 sequence;
    LARGE_INTEGER timeStamp;

    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Context);

    KphApplyObProtections(Info);

    sequence = InterlockedIncrementU64(&KphpObSequence);

    options = KphpObGetOptions(Info);

    if (options.PreEnabled)
    {
        KphpObPreOpSend(Info, &options, sequence, &timeStamp);
    }
    else
    {
        //
        // Pre operations aren't enabled, but we need the time stamp for the
        // post operation to use.
        //
        KeQuerySystemTime(&timeStamp);
    }

    if (options.PostEnabled)
    {
        KphpObPreOpSetCallContext(Info, &options, sequence, &timeStamp);
    }

    return OB_PREOP_SUCCESS;
}

/**
 * \brief Starts the object informer.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphObjectInformerStart(
    VOID
    )
{
    NTSTATUS status;
    OB_CALLBACK_REGISTRATION callbackRegistration;
    OB_OPERATION_REGISTRATION operationRegistration[3];

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(KphAltitude);

    RtlZeroMemory(operationRegistration, sizeof(operationRegistration));

    operationRegistration[0].ObjectType = PsProcessType;
    operationRegistration[0].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
    operationRegistration[0].PreOperation = KphpObPreOp;
    operationRegistration[0].PostOperation = KphpObPostOp;

    operationRegistration[1].ObjectType = PsThreadType;
    operationRegistration[1].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
    operationRegistration[1].PreOperation = KphpObPreOp;
    operationRegistration[1].PostOperation = KphpObPostOp;

    operationRegistration[2].ObjectType = ExDesktopObjectType;
    operationRegistration[2].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
    operationRegistration[2].PreOperation = KphpObPreOp;
    operationRegistration[2].PostOperation = KphpObPostOp;

    callbackRegistration.Version = OB_FLT_REGISTRATION_VERSION;
    callbackRegistration.OperationRegistrationCount = ARRAYSIZE(operationRegistration);
    callbackRegistration.Altitude.Buffer = KphAltitude->Buffer;
    callbackRegistration.Altitude.Length = KphAltitude->Length;
    callbackRegistration.Altitude.MaximumLength = KphAltitude->MaximumLength;
    callbackRegistration.RegistrationContext = NULL;
    callbackRegistration.OperationRegistration = operationRegistration;

    KphInitializePagedLookaside(&KphpObCallContextLookaside,
                                sizeof(KPH_OB_CALL_CONTEXT),
                                KPH_TAG_OB_CALL_CONTEXT);

    status = ObRegisterCallbacks(&callbackRegistration,
                                 &KphpObRegistrationHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "ObRegisterCallbacks failed: %!STATUS!",
                      status);

        KphDeletePagedLookaside(&KphpObCallContextLookaside);
        KphpObRegistrationHandle = NULL;
    }

    return status;
}

/**
 * \brief Stops the object informer.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphObjectInformerStop(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (KphpObRegistrationHandle)
    {
        ObUnRegisterCallbacks(KphpObRegistrationHandle);
        KphDeletePagedLookaside(&KphpObCallContextLookaside);
    }
}
