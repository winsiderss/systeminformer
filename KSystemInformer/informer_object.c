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
#include <dyndata.h>
#include <comms.h>
#include <kphmsgdyn.h>

#include <trace.h>

PAGED_FILE();

static PVOID KphpObRegistrationHandle = NULL;

/**
 * \brief Populates object name in a message for registered object callbacks.
 *
 * \details Used to populate the desktop name for desktop object callbacks.
 *
 * \param[in,out] Message The message to populate.
 * \param[in] Object The object for which the name is populated in the message.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpPopulateObjectNameInMessage(
    _Inout_ PKPH_MESSAGE Message,
    _In_ PVOID Object
    )
{
    NTSTATUS status;
    BYTE stackBuffer[MAX_PATH + sizeof(OBJECT_NAME_INFORMATION)];
    PBYTE buffer;
    POBJECT_NAME_INFORMATION info;
    ULONG length;

    PAGED_PASSIVE();

    buffer = NULL;

    info = (POBJECT_NAME_INFORMATION)stackBuffer;
    length = ARRAYSIZE(stackBuffer);

    status = ObQueryNameString(Object, info, length, &length);
    if ((status == STATUS_INFO_LENGTH_MISMATCH) ||
        (status == STATUS_BUFFER_TOO_SMALL))
    {
        buffer = KphAllocatePaged(length, KPH_TAG_INFORMER_OB_NAME);
        if (!buffer)
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          INFORMER,
                          "Failed to allocate buffer for object name");

            goto Exit;
        }

        info = (POBJECT_NAME_INFORMATION)buffer;

        status = ObQueryNameString(Object, info, length, &length);
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
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
        KphTracePrint(TRACE_LEVEL_WARNING,
                      INFORMER,
                      "KphMsgDynAddUnicodeString failed: %!STATUS!",
                      status);
    }

Exit:

    if (buffer)
    {
        KphFree(buffer, KPH_TAG_INFORMER_OB_NAME);
    }
}

/**
 * \brief Object manager pre-operation process handle create callback.
 *
 * \param[in] Context Not used.
 * \param[in,out] Info The object pre-operation information.
 *
 * \return OB_PREOP_SUCCESS
 */
_Function_class_(POB_PRE_OPERATION_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
OB_PREOP_CALLBACK_STATUS KphpObPreProcessHandleCreate(
    _In_ PVOID Context,
    _Inout_ POB_PRE_OPERATION_INFORMATION Info
    )
{
    PKPH_MESSAGE msg;

    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(Context);

    NT_ASSERT(Info->ObjectType == *PsProcessType);
    NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_CREATE);

    KphApplyObProtections(Info);

    if (!KphInformerSettings.ProcessHandlePreCreate)
    {
        return OB_PREOP_SUCCESS;
    }

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "Failed to allocate message");

        return OB_PREOP_SUCCESS;
    }

    KphMsgInit(msg, KphMsgProcessHandlePreCreate);

    msg->Kernel.ProcessHandlePreCreate.ContextClientId.UniqueProcess =
        PsGetCurrentProcessId();
    msg->Kernel.ProcessHandlePreCreate.ContextClientId.UniqueThread =
        PsGetCurrentThreadId();
    msg->Kernel.ProcessHandlePreCreate.Flags = Info->Flags;
    msg->Kernel.ProcessHandlePreCreate.DesiredAccess =
        Info->Parameters->CreateHandleInformation.DesiredAccess;
    msg->Kernel.ProcessHandlePreCreate.OriginalDesiredAccess =
        Info->Parameters->CreateHandleInformation.OriginalDesiredAccess;
    msg->Kernel.ProcessHandlePreCreate.ObjectProcessId =
        PsGetProcessId(Info->Object);

    if (KphInformerSettings.EnableStackTraces)
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);

    return OB_PREOP_SUCCESS;
}

/**
 * \brief Object manager post-operation process handle create callback.
 *
 * \param[in] Context Not used.
 * \param[in] Info The object post-operation information.
 */
_Function_class_(POB_POST_OPERATION_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpObPostProcessHandleCreate(
    _In_ PVOID Context,
    _In_ POB_POST_OPERATION_INFORMATION Info
    )
{
    PKPH_MESSAGE msg;

    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(Context);

    NT_ASSERT(Info->ObjectType == *PsProcessType);
    NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_CREATE);

    if (!KphInformerSettings.ProcessHandlePostCreate)
    {
        return;
    }

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "Failed to allocate message");

        return;
    }

    KphMsgInit(msg, KphMsgProcessHandlePostCreate);

    msg->Kernel.ProcessHandlePostCreate.ContextClientId.UniqueProcess =
        PsGetCurrentProcessId();
    msg->Kernel.ProcessHandlePostCreate.ContextClientId.UniqueThread =
        PsGetCurrentThreadId();
    msg->Kernel.ProcessHandlePostCreate.Flags = Info->Flags;
    msg->Kernel.ProcessHandlePostCreate.ReturnStatus = Info->ReturnStatus;
    msg->Kernel.ProcessHandlePostCreate.GrantedAccess =
        Info->Parameters->CreateHandleInformation.GrantedAccess;
    msg->Kernel.ProcessHandlePostCreate.ObjectProcessId =
        PsGetProcessId(Info->Object);

    if (KphInformerSettings.EnableStackTraces)
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);
}

/**
 * \brief Object manager pre-operation process handle duplicate callback.
 *
 * \param[in] Context Not used.
 * \param[in,out] Info The object pre-operation information.
 *
 * \return OB_PREOP_SUCCESS
 */
_Function_class_(POB_PRE_OPERATION_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
OB_PREOP_CALLBACK_STATUS KphpObPreProcessHandleDuplicate(
    _In_ PVOID Context,
    _Inout_ POB_PRE_OPERATION_INFORMATION Info
    )
{
    PKPH_MESSAGE msg;

    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(Context);

    NT_ASSERT(Info->ObjectType == *PsProcessType);
    NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_DUPLICATE);

    KphApplyObProtections(Info);

    if (!KphInformerSettings.ProcessHandlePreDuplicate)
    {
        return OB_PREOP_SUCCESS;
    }

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "Failed to allocate message");

        return OB_PREOP_SUCCESS;
    }

    KphMsgInit(msg, KphMsgProcessHandlePreDuplicate);

    msg->Kernel.ProcessHandlePreDuplicate.ContextClientId.UniqueProcess =
        PsGetCurrentProcessId();
    msg->Kernel.ProcessHandlePreDuplicate.ContextClientId.UniqueThread =
        PsGetCurrentThreadId();
    msg->Kernel.ProcessHandlePreDuplicate.Flags = Info->Flags;
    msg->Kernel.ProcessHandlePreDuplicate.DesiredAccess =
        Info->Parameters->DuplicateHandleInformation.DesiredAccess;
    msg->Kernel.ProcessHandlePreDuplicate.OriginalDesiredAccess =
        Info->Parameters->DuplicateHandleInformation.OriginalDesiredAccess;
    msg->Kernel.ProcessHandlePreDuplicate.SourceProcessId =
        PsGetProcessId(Info->Parameters->DuplicateHandleInformation.SourceProcess);
    msg->Kernel.ProcessHandlePreDuplicate.TargetProcessId =
        PsGetProcessId(Info->Parameters->DuplicateHandleInformation.TargetProcess);
    msg->Kernel.ProcessHandlePreDuplicate.ObjectProcessId =
        PsGetProcessId(Info->Object);

    if (KphInformerSettings.EnableStackTraces)
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);

    return OB_PREOP_SUCCESS;
}

/**
 * \brief Object manager post-operation process handle duplicate callback.
 *
 * \param[in] Context Not used.
 * \param[in] Info The object post-operation information.
 */
_Function_class_(POB_POST_OPERATION_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpObPostProcessHandleDuplicate(
    _In_ PVOID Context,
    _In_ POB_POST_OPERATION_INFORMATION Info
    )
{
    PKPH_MESSAGE msg;

    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(Context);

    NT_ASSERT(Info->ObjectType == *PsProcessType);
    NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_DUPLICATE);

    if (!KphInformerSettings.ProcessHandlePostDuplicate)
    {
        return;
    }

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "Failed to allocate message");

        return;
    }

    KphMsgInit(msg, KphMsgProcessHandlePostDuplicate);

    msg->Kernel.ProcessHandlePostDuplicate.ContextClientId.UniqueProcess =
        PsGetCurrentProcessId();
    msg->Kernel.ProcessHandlePostDuplicate.ContextClientId.UniqueThread =
        PsGetCurrentThreadId();
    msg->Kernel.ProcessHandlePostDuplicate.Flags = Info->Flags;
    msg->Kernel.ProcessHandlePostDuplicate.ReturnStatus = Info->ReturnStatus;
    msg->Kernel.ProcessHandlePostDuplicate.GrantedAccess =
        Info->Parameters->CreateHandleInformation.GrantedAccess;
    msg->Kernel.ProcessHandlePostDuplicate.ObjectProcessId =
        PsGetProcessId(Info->Object);

    if (KphInformerSettings.EnableStackTraces)
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);
}

/**
 * \brief Object manager pre-operation thread handle create callback.
 *
 * \param[in] Context Not used.
 * \param[in,out] Info The object pre-operation information.
 *
 * \return OB_PREOP_SUCCESS
 */
_Function_class_(POB_PRE_OPERATION_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
OB_PREOP_CALLBACK_STATUS KphpObPreThreadHandleCreate(
    _In_ PVOID Context,
    _Inout_ POB_PRE_OPERATION_INFORMATION Info
    )
{
    PKPH_MESSAGE msg;

    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(Context);

    KphApplyObProtections(Info);

    NT_ASSERT(Info->ObjectType == *PsThreadType);
    NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_CREATE);

    if (!KphInformerSettings.ThreadHandlePreCreate)
    {
        return OB_PREOP_SUCCESS;
    }

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "Failed to allocate message");

        return OB_PREOP_SUCCESS;
    }

    KphMsgInit(msg, KphMsgThreadHandlePreCreate);

    msg->Kernel.ThreadHandlePreCreate.ContextClientId.UniqueProcess =
        PsGetCurrentProcessId();
    msg->Kernel.ThreadHandlePreCreate.ContextClientId.UniqueThread =
        PsGetCurrentThreadId();
    msg->Kernel.ThreadHandlePreCreate.Flags = Info->Flags;
    msg->Kernel.ThreadHandlePreCreate.DesiredAccess =
        Info->Parameters->CreateHandleInformation.DesiredAccess;
    msg->Kernel.ThreadHandlePreCreate.OriginalDesiredAccess =
        Info->Parameters->CreateHandleInformation.OriginalDesiredAccess;
    msg->Kernel.ThreadHandlePreCreate.ObjectThreadId =
        PsGetThreadId(Info->Object);

    if (KphInformerSettings.EnableStackTraces)
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);

    return OB_PREOP_SUCCESS;
}

/**
 * \brief Object manager post-operation thread handle create callback.
 *
 * \param[in] Context Not used.
 * \param[in] Info The object post-operation information.
 */
_Function_class_(POB_POST_OPERATION_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpObPostThreadHandleCreate(
    _In_ PVOID Context,
    _In_ POB_POST_OPERATION_INFORMATION Info
    )
{
    PKPH_MESSAGE msg;

    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(Context);

    NT_ASSERT(Info->ObjectType == *PsThreadType);
    NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_CREATE);

    if (!KphInformerSettings.ThreadHandlePostCreate)
    {
        return;
    }

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "Failed to allocate message");

        return;
    }

    KphMsgInit(msg, KphMsgThreadHandlePostCreate);

    msg->Kernel.ThreadHandlePostCreate.ContextClientId.UniqueProcess =
        PsGetCurrentProcessId();
    msg->Kernel.ThreadHandlePostCreate.ContextClientId.UniqueThread =
        PsGetCurrentThreadId();
    msg->Kernel.ThreadHandlePostCreate.Flags = Info->Flags;
    msg->Kernel.ThreadHandlePostCreate.ReturnStatus = Info->ReturnStatus;
    msg->Kernel.ThreadHandlePostCreate.GrantedAccess =
        Info->Parameters->CreateHandleInformation.GrantedAccess;
    msg->Kernel.ThreadHandlePostCreate.ObjectThreadId =
        PsGetThreadId(Info->Object);

    if (KphInformerSettings.EnableStackTraces)
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);
}

/**
 * \brief Object manager pre-operation thread handle duplicate callback.
 *
 * \param[in] Context Not used.
 * \param[in,out] Info The object pre-operation information.
 *
 * \return OB_PREOP_SUCCESS
 */
_Function_class_(POB_PRE_OPERATION_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
OB_PREOP_CALLBACK_STATUS KphpObPreThreadHandleDuplicate(
    _In_ PVOID Context,
    _Inout_ POB_PRE_OPERATION_INFORMATION Info
    )
{
    PKPH_MESSAGE msg;

    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(Context);

    KphApplyObProtections(Info);

    NT_ASSERT(Info->ObjectType == *PsThreadType);
    NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_DUPLICATE);

    if (!KphInformerSettings.ThreadHandlePreDuplicate)
    {
        return OB_PREOP_SUCCESS;
    }

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "Failed to allocate message");

        return OB_PREOP_SUCCESS;
    }

    KphMsgInit(msg, KphMsgThreadHandlePreDuplicate);

    msg->Kernel.ThreadHandlePreDuplicate.ContextClientId.UniqueProcess =
        PsGetCurrentProcessId();
    msg->Kernel.ThreadHandlePreDuplicate.ContextClientId.UniqueThread =
        PsGetCurrentThreadId();
    msg->Kernel.ThreadHandlePreDuplicate.Flags = Info->Flags;
    msg->Kernel.ThreadHandlePreDuplicate.DesiredAccess =
        Info->Parameters->DuplicateHandleInformation.DesiredAccess;
    msg->Kernel.ThreadHandlePreDuplicate.OriginalDesiredAccess =
        Info->Parameters->DuplicateHandleInformation.OriginalDesiredAccess;
    msg->Kernel.ThreadHandlePreDuplicate.SourceProcessId =
        PsGetProcessId(Info->Parameters->DuplicateHandleInformation.SourceProcess);
    msg->Kernel.ThreadHandlePreDuplicate.TargetProcessId =
        PsGetProcessId(Info->Parameters->DuplicateHandleInformation.TargetProcess);
    msg->Kernel.ThreadHandlePreDuplicate.ObjectThreadId =
        PsGetThreadId(Info->Object);

    if (KphInformerSettings.EnableStackTraces)
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);

    return OB_PREOP_SUCCESS;
}

/**
 * \brief Object manager post-operation thread handle duplicate callback.
 *
 * \param[in] Context Not used.
 * \param[in] Info The object post-operation information.
 */
_Function_class_(POB_POST_OPERATION_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpObPostThreadHandleDuplicate(
    _In_ PVOID Context,
    _In_ POB_POST_OPERATION_INFORMATION Info
    )
{
    PKPH_MESSAGE msg;

    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(Context);

    NT_ASSERT(Info->ObjectType == *PsThreadType);
    NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_DUPLICATE);

    if (!KphInformerSettings.ThreadHandlePostDuplicate)
    {
        return;
    }

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "Failed to allocate message");

        return;
    }

    KphMsgInit(msg, KphMsgThreadHandlePostDuplicate);

    msg->Kernel.ThreadHandlePostDuplicate.ContextClientId.UniqueProcess =
        PsGetCurrentProcessId();
    msg->Kernel.ThreadHandlePostDuplicate.ContextClientId.UniqueThread =
        PsGetCurrentThreadId();
    msg->Kernel.ThreadHandlePostDuplicate.Flags = Info->Flags;
    msg->Kernel.ThreadHandlePostDuplicate.ReturnStatus = Info->ReturnStatus;
    msg->Kernel.ThreadHandlePostDuplicate.GrantedAccess =
        Info->Parameters->CreateHandleInformation.GrantedAccess;
    msg->Kernel.ThreadHandlePostDuplicate.ObjectThreadId =
        PsGetThreadId(Info->Object);

    if (KphInformerSettings.EnableStackTraces)
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);
}

/**
 * \brief Object manager pre-operation desktop handle create callback.
 *
 * \param[in] Context Not used.
 * \param[in,out] Info The object pre-operation information.
 *
 * \return OB_PREOP_SUCCESS
 */
_Function_class_(POB_PRE_OPERATION_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
OB_PREOP_CALLBACK_STATUS KphpObPreDesktopHandleCreate(
    _In_ PVOID Context,
    _Inout_ POB_PRE_OPERATION_INFORMATION Info
    )
{
    PKPH_MESSAGE msg;

    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(Context);

    NT_ASSERT(Info->ObjectType == *ExDesktopObjectType);
    NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_CREATE);

    if (!KphInformerSettings.DesktopHandlePreCreate)
    {
        return OB_PREOP_SUCCESS;
    }

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "Failed to allocate message");

        return OB_PREOP_SUCCESS;
    }

    KphMsgInit(msg, KphMsgDesktopHandlePreCreate);

    msg->Kernel.DesktopHandlePreCreate.ContextClientId.UniqueProcess =
        PsGetCurrentProcessId();
    msg->Kernel.DesktopHandlePreCreate.ContextClientId.UniqueThread =
        PsGetCurrentThreadId();
    msg->Kernel.DesktopHandlePreCreate.Flags = Info->Flags;
    msg->Kernel.DesktopHandlePreCreate.DesiredAccess =
        Info->Parameters->CreateHandleInformation.DesiredAccess;
    msg->Kernel.DesktopHandlePreCreate.OriginalDesiredAccess =
        Info->Parameters->CreateHandleInformation.OriginalDesiredAccess;

    KphpPopulateObjectNameInMessage(msg, Info->Object);

    if (KphInformerSettings.EnableStackTraces)
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);

    return OB_PREOP_SUCCESS;
}

/**
 * \brief Object manager post-operation desktop handle create callback.
 *
 * \param[in] Context Not used.
 * \param[in] Info The object post-operation information.
 */
_Function_class_(POB_POST_OPERATION_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpObPostDesktopHandleCreate(
    _In_ PVOID Context,
    _In_ POB_POST_OPERATION_INFORMATION Info
    )
{
    PKPH_MESSAGE msg;

    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(Context);

    NT_ASSERT(Info->ObjectType == *ExDesktopObjectType);
    NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_CREATE);

    if (!KphInformerSettings.DesktopHandlePostCreate)
    {
        return;
    }

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "Failed to allocate message");

        return;
    }

    KphMsgInit(msg, KphMsgDesktopHandlePostCreate);

    msg->Kernel.DesktopHandlePostCreate.ContextClientId.UniqueProcess =
        PsGetCurrentProcessId();
    msg->Kernel.DesktopHandlePostCreate.ContextClientId.UniqueThread =
        PsGetCurrentThreadId();
    msg->Kernel.DesktopHandlePostCreate.Flags = Info->Flags;
    msg->Kernel.DesktopHandlePostCreate.ReturnStatus = Info->ReturnStatus;
    msg->Kernel.DesktopHandlePostCreate.GrantedAccess =
        Info->Parameters->CreateHandleInformation.GrantedAccess;

    KphpPopulateObjectNameInMessage(msg, Info->Object);

    if (KphInformerSettings.EnableStackTraces)
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);
}

/**
 * \brief Object manager pre-operation desktop handle duplicate callback.
 *
 * \param[in] Context Not used.
 * \param[in,out] Info The object pre-operation information.
 *
 * \return OB_PREOP_SUCCESS
 */
_Function_class_(POB_PRE_OPERATION_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
OB_PREOP_CALLBACK_STATUS KphpObPreDesktopHandleDuplicate(
    _In_ PVOID Context,
    _Inout_ POB_PRE_OPERATION_INFORMATION Info
    )
{
    PKPH_MESSAGE msg;

    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(Context);

    NT_ASSERT(Info->ObjectType == *ExDesktopObjectType);
    NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_DUPLICATE);

    if (!KphInformerSettings.DesktopHandlePreDuplicate)
    {
        return OB_PREOP_SUCCESS;
    }

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "Failed to allocate message");

        return OB_PREOP_SUCCESS;
    }

    KphMsgInit(msg, KphMsgDesktopHandlePreDuplicate);

    msg->Kernel.DesktopHandlePreDuplicate.ContextClientId.UniqueProcess =
        PsGetCurrentProcessId();
    msg->Kernel.DesktopHandlePreDuplicate.ContextClientId.UniqueThread =
        PsGetCurrentThreadId();
    msg->Kernel.DesktopHandlePreDuplicate.Flags = Info->Flags;
    msg->Kernel.DesktopHandlePreDuplicate.DesiredAccess =
        Info->Parameters->DuplicateHandleInformation.DesiredAccess;
    msg->Kernel.DesktopHandlePreDuplicate.OriginalDesiredAccess =
        Info->Parameters->DuplicateHandleInformation.OriginalDesiredAccess;
    msg->Kernel.DesktopHandlePreDuplicate.SourceProcessId =
        PsGetProcessId(Info->Parameters->DuplicateHandleInformation.SourceProcess);
    msg->Kernel.DesktopHandlePreDuplicate.TargetProcessId =
        PsGetProcessId(Info->Parameters->DuplicateHandleInformation.TargetProcess);

    KphpPopulateObjectNameInMessage(msg, Info->Object);

    if (KphInformerSettings.EnableStackTraces)
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);

    return OB_PREOP_SUCCESS;
}

/**
 * \brief Object manager post-operation desktop handle duplicate callback.
 *
 * \param[in] Context Not used.
 * \param[in] Info The object post-operation information.
 */
_Function_class_(POB_POST_OPERATION_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpObPostDesktopHandleDuplicate(
    _In_ PVOID Context,
    _In_ POB_POST_OPERATION_INFORMATION Info
    )
{
    PKPH_MESSAGE msg;

    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(Context);

    NT_ASSERT(Info->ObjectType == *ExDesktopObjectType);
    NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_DUPLICATE);

    if (!KphInformerSettings.DesktopHandlePostDuplicate)
    {
        return;
    }

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "Failed to allocate message");

        return;
    }

    KphMsgInit(msg, KphMsgDesktopHandlePostDuplicate);

    msg->Kernel.DesktopHandlePostDuplicate.ContextClientId.UniqueProcess =
        PsGetCurrentProcessId();
    msg->Kernel.DesktopHandlePostDuplicate.ContextClientId.UniqueThread =
        PsGetCurrentThreadId();
    msg->Kernel.DesktopHandlePostDuplicate.Flags = Info->Flags;
    msg->Kernel.DesktopHandlePostDuplicate.ReturnStatus = Info->ReturnStatus;
    msg->Kernel.DesktopHandlePostDuplicate.GrantedAccess =
        Info->Parameters->CreateHandleInformation.GrantedAccess;

    KphpPopulateObjectNameInMessage(msg, Info->Object);

    if (KphInformerSettings.EnableStackTraces)
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
OB_PREOP_CALLBACK_STATUS KphpObPreCallback(
    _In_ PVOID Context,
    _Inout_ POB_PRE_OPERATION_INFORMATION Info
    )
{
    PAGED_PASSIVE();

    if (Info->ObjectType == *PsProcessType)
    {
        if (Info->Operation == OB_OPERATION_HANDLE_CREATE)
        {
            return KphpObPreProcessHandleCreate(Context, Info);
        }

        NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_DUPLICATE);

        return KphpObPreProcessHandleDuplicate(Context, Info);
    }

    if (Info->ObjectType == *PsThreadType)
    {
        if (Info->Operation == OB_OPERATION_HANDLE_CREATE)
        {
            return KphpObPreThreadHandleCreate(Context, Info);
        }

        NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_DUPLICATE);

        return KphpObPreThreadHandleDuplicate(Context, Info);
    }

    NT_ASSERT(Info->ObjectType == *ExDesktopObjectType);

    if (Info->Operation == OB_OPERATION_HANDLE_CREATE)
    {
        return KphpObPreDesktopHandleCreate(Context, Info);
    }

    NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_DUPLICATE);

    return KphpObPreDesktopHandleDuplicate(Context, Info);
}

/**
 * \brief Object manager post-operation callback.
 *
 * \param[in] Context Not used.
 * \param[in] Info The object post-operation information.
 */
_Function_class_(POB_POST_OPERATION_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpObPostCallback(
    _In_ PVOID Context,
    _In_ POB_POST_OPERATION_INFORMATION Info
    )
{
    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(Context);

    if (Info->ObjectType == *PsProcessType)
    {
        if (Info->Operation == OB_OPERATION_HANDLE_CREATE)
        {
            KphpObPostProcessHandleCreate(Context, Info);
            return;
        }

        NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_DUPLICATE);

        KphpObPostProcessHandleDuplicate(Context, Info);
        return;
    }

    if (Info->ObjectType == *PsThreadType)
    {
        if (Info->Operation == OB_OPERATION_HANDLE_CREATE)
        {
            KphpObPostThreadHandleCreate(Context, Info);
            return;
        }

        NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_DUPLICATE);

        KphpObPostThreadHandleDuplicate(Context, Info);
        return;
    }

    NT_ASSERT(Info->ObjectType == *ExDesktopObjectType);

    if (Info->Operation == OB_OPERATION_HANDLE_CREATE)
    {
        KphpObPostDesktopHandleCreate(Context, Info);
        return;
    }

    NT_ASSERT(Info->Operation == OB_OPERATION_HANDLE_DUPLICATE);

    KphpObPostDesktopHandleDuplicate(Context, Info);
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

    PAGED_PASSIVE();

    NT_ASSERT(KphDynAltitude);

    RtlZeroMemory(operationRegistration, sizeof(operationRegistration));

    operationRegistration[0].ObjectType = PsProcessType;
    operationRegistration[0].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
    operationRegistration[0].PreOperation = KphpObPreCallback;
    operationRegistration[0].PostOperation = KphpObPostCallback;

    operationRegistration[1].ObjectType = PsThreadType;
    operationRegistration[1].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
    operationRegistration[1].PreOperation = KphpObPreCallback;
    operationRegistration[1].PostOperation = KphpObPostCallback;

    operationRegistration[2].ObjectType = ExDesktopObjectType;
    operationRegistration[2].Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;
    operationRegistration[2].PreOperation = KphpObPreCallback;
    operationRegistration[2].PostOperation = KphpObPostCallback;

    callbackRegistration.Version = OB_FLT_REGISTRATION_VERSION;
    callbackRegistration.OperationRegistrationCount = ARRAYSIZE(operationRegistration);
    callbackRegistration.Altitude.Buffer = KphDynAltitude->Buffer;
    callbackRegistration.Altitude.Length = KphDynAltitude->Length;
    callbackRegistration.Altitude.MaximumLength = KphDynAltitude->MaximumLength;
    callbackRegistration.RegistrationContext = NULL;
    callbackRegistration.OperationRegistration = operationRegistration;

    status = ObRegisterCallbacks(&callbackRegistration,
                                 &KphpObRegistrationHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "ObRegisterCallbacks failed: %!STATUS!",
                      status);

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
    PAGED_PASSIVE();

    if (KphpObRegistrationHandle)
    {
        ObUnRegisterCallbacks(KphpObRegistrationHandle);
        KphpObRegistrationHandle = NULL;
    }
}
