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
