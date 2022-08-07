#pragma once
#include <ph.h>
#include <kphmsg.h>

typedef
VOID (NTAPI *PKPH_COMMS_CALLBACK)(
    _In_ ULONG_PTR ReplyToken,
    _In_ PCKPH_MESSAGE Message
    );

PPH_STRING KphCommGetMessagePortName(
    VOID
    );

_Must_inspect_result_
NTSTATUS KphCommsStart(
    _In_ PPH_STRING PortName,
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
