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

#pragma once
#include <kphmsg.h>

typedef
_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
KSIAPI
KPHM_HANDLER (
    _Inout_ PKPH_MESSAGE Message
    );
typedef KPHM_HANDLER *PKPHM_HANDLER;

#define KPHM_DEFINE_HANDLER(__Name__)\
_Function_class_(KPHM_HANDLER)\
_IRQL_requires_max_(PASSIVE_LEVEL) \
_Must_inspect_result_ \
NTSTATUS __Name__( \
    _Inout_ PKPH_MESSAGE Message \
    )

typedef
_Function_class_(KPHM_REQUIRED_STATE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
KPH_PROCESS_STATE
KSIAPI
KPHM_REQUIRED_STATE (
    _In_ PCKPH_MESSAGE Message
    );
typedef KPHM_REQUIRED_STATE *PKPHM_REQUIRED_STATE;

#define KPHM_DEFINE_REQUIRED_STATE(__Name__)\
_Function_class_(KPHM_REQUIRED_STATE)\
_IRQL_requires_max_(PASSIVE_LEVEL) \
_Must_inspect_result_ \
KPH_PROCESS_STATE __Name__( \
    _In_ PCKPH_MESSAGE Message \
    )

typedef struct _KPH_MESSAGE_HANDLER
{
    KPH_MESSAGE_ID MessageId;
    PKPHM_HANDLER Handler;
    PKPHM_REQUIRED_STATE RequiredState;

} KPH_MESSAGE_HANDLER, *PKPH_MESSAGE_HANDLER;

extern KPH_MESSAGE_HANDLER KphCommsMessageHandlers[];
extern ULONG KphCommsMessageHandlerCount;

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCommsStart(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCommsStop(
    VOID
    );

_IRQL_requires_max_(APC_LEVEL)
_Return_allocatesMem_
PKPH_MESSAGE KphAllocateMessage(
    VOID
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphFreeMessage(
    _In_freesMem_ PKPH_MESSAGE Message
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
_Return_allocatesMem_
PKPH_MESSAGE KphAllocateNPagedMessage(
    VOID
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphFreeNPagedMessage(
    _In_freesMem_ PKPH_MESSAGE Message
    );

#define KPH_COMMS_SHORT_TIMEOUT (1000)
#define KPH_COMMS_DEFAULT_TIMEOUT (3 * 1000)
#define KPH_COMMS_LONG_TIMEOUT (10 * 1000)

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphCommsSendMessage(
    _In_ PKPH_MESSAGE Message,
    _Out_opt_ PKPH_MESSAGE Reply,
    _In_ ULONG TimeoutMs
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphCommsSendMessageAsync(
    _In_aliasesMem_ PKPH_MESSAGE Message
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphCommsSendNPagedMessageAsync(
    _In_aliasesMem_ PKPH_MESSAGE Message
    );
