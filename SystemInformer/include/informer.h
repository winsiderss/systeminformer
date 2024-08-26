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

#ifndef PH_KSIMSG_H
#define PH_KSIMSG_H

typedef struct _PH_INFORMER_REPLY_CONTEXT
{
    PKPH_MESSAGE Message;
    ULONG_PTR ReplyToken;
    BOOLEAN Handled;
} PH_INFORMER_REPLY_CONTEXT, *PPH_INFORMER_REPLY_CONTEXT;

/**
 * Callback registration for informer messages, these messages are dispatched
 * asynchronously from the primary message threads, meaning they can not be
 * replied to. This is the preferred method of registering for message since
 * it gets out of the way of the system.
 *
 * Receives PKSI_MESSAGE as the callback parameter, this is an object that can
 * be referenced.
 */
extern PH_CALLBACK PhInformerCallback;

/**
 * Callback registration for informer messages that can be replied to. Any
 * processing done by these callbacks **must** be brief as it is blocking
 * informer message handling on the system.
 *
 * Receives PPH_INFORMER_REPLY_CONTEXT as the callback parameter. The callback
 * may use PhInformerReply to reply to a message. After a successful call to
 * PhInformerReply the context is updated with Handled set to TRUE. Callbacks
 * later in the chain will still be called, but the message should not be
 * replied to. Callbacks should generally check if the message has already
 * been handled prior to doing work.
 */
extern PH_CALLBACK PhInformerReplyCallback;

NTSTATUS PhInformerReply(
    _Inout_ PPH_INFORMER_REPLY_CONTEXT Context,
    _In_ PKPH_MESSAGE ReplyMessage
    );

BOOLEAN PhInformerDispatch(
    _In_ ULONG_PTR ReplyToken,
    _In_ PCKPH_MESSAGE Message
    );

VOID PhInformerInitialize(
    VOID
    );

VOID PhInformerStop(
    VOID
    );

#endif
