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

#ifndef _PH_KPHCOMMS_H
#define _PH_KPHCOMMS_H

#include <kphmsg.h>

NTSTATUS KphFilterLoadUnload(
    _In_ PPH_STRINGREF ServiceName,
    _In_ BOOLEAN LoadDriver
    );

/**
 * \brief Callback for handling messages from the kernel.
 *
 * \details Synchronous messages expecting a reply must always be handled. The
 * callback may return FALSE even when the kernel is expecting a reply. In this
 * case, the internals of the system will handle the reply on behalf of the
 * callback.
 *
 * \param[in] ReplyToken The token used to reply to kernel messages. If zero
 * the kernel is not expecting a reply. Use KphCommsReplyMessage to reply.
 * \param[in] Message The message from the kernel.
 *
 * \return TRUE if the message was handled by this callback, FALSE otherwise.
 */
typedef
BOOLEAN (NTAPI *PKPH_COMMS_CALLBACK)(
    _In_ ULONG_PTR ReplyToken,
    _In_ PCKPH_MESSAGE Message
    );

_Must_inspect_result_
NTSTATUS KphCommsStart(
    _In_ PPH_STRINGREF PortName,
    _In_opt_ PKPH_COMMS_CALLBACK Callback
    );

VOID KphCommsStop(
    VOID
    );

BOOLEAN KphCommsIsConnected(
    VOID
    );

NTSTATUS KphCommsReplyMessage(
    _In_ ULONG_PTR ReplyToken,
    _In_ PKPH_MESSAGE Message
    );

_Must_inspect_result_
NTSTATUS KphCommsSendMessage(
    _Inout_ PKPH_MESSAGE Message
    );

#endif
