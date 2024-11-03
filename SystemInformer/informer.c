/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2024
 *
 */
#include <phapp.h>
#include <kphcomms.h>

#include <informer.h>

static PPH_OBJECT_TYPE KsiMessageObjectType = NULL;

PH_CALLBACK_DECLARE(PhInformerCallback);

NTSTATUS PhInformerReply(
    _Inout_ PPH_INFORMER_CONTEXT Context,
    _In_ PKPH_MESSAGE ReplyMessage
    )
{
    NTSTATUS status;

    if (Context->Handled)
        return STATUS_INVALID_PARAMETER_1;

    if (NT_SUCCESS(status = KphCommsReplyMessage(Context->ReplyToken, ReplyMessage)))
        Context->Handled = TRUE;

    return status;
}

BOOLEAN PhInformerDispatch(
    _In_ ULONG_PTR ReplyToken,
    _In_ PCKPH_MESSAGE Message
    )
{
    PKPH_MESSAGE message;
    PH_INFORMER_CONTEXT context;

    message = PhCreateObject(Message->Header.Size, KsiMessageObjectType);
    memcpy(message, Message, Message->Header.Size);

    context.Message = message;
    context.ReplyToken = ReplyToken;
    context.Handled = FALSE;

    PhInvokeCallback(&PhInformerCallback, &context);

    PhDereferenceObject(message);

    return context.Handled;
}

VOID PhInformerInitialize(
    VOID
    )
{
    KsiMessageObjectType = PhCreateObjectType(L"KsiMessage", 0, NULL);
}
