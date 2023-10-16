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

typedef struct _KPH_CLIENT
{
    LIST_ENTRY Entry;
    PKPH_PROCESS_CONTEXT Process;
    PFLT_PORT Port;
    KPH_REFERENCE DriverUnloadProtectionRef;
} KPH_CLIENT, *PKPH_CLIENT;

typedef
_Function_class_(KPHM_HANDLER)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
KSIAPI
KPHM_HANDLER (
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_MESSAGE Message
    );
typedef KPHM_HANDLER *PKPHM_HANDLER;

#define KPHM_DEFINE_HANDLER(__Name__)                                         \
_Function_class_(KPHM_HANDLER)                                                \
_IRQL_requires_max_(PASSIVE_LEVEL)                                            \
_Must_inspect_result_                                                         \
NTSTATUS __Name__(                                                            \
    _In_ PKPH_CLIENT Client,                                                  \
    _Inout_ PKPH_MESSAGE Message                                              \
    )

typedef
_Function_class_(KPHM_REQUIRED_STATE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
KPH_PROCESS_STATE
KSIAPI
KPHM_REQUIRED_STATE (
    _In_ PKPH_CLIENT Client,
    _In_ PCKPH_MESSAGE Message
    );
typedef KPHM_REQUIRED_STATE *PKPHM_REQUIRED_STATE;

#define KPHM_DEFINE_REQUIRED_STATE(__Name__)                                  \
_Function_class_(KPHM_REQUIRED_STATE)                                         \
_IRQL_requires_max_(PASSIVE_LEVEL)                                            \
_Must_inspect_result_                                                         \
KPH_PROCESS_STATE __Name__(                                                   \
    _In_ PKPH_CLIENT Client,                                                  \
    _In_ PCKPH_MESSAGE Message                                                \
    )

typedef struct _KPH_MESSAGE_HANDLER
{
    KPH_MESSAGE_ID MessageId;
    PKPHM_HANDLER Handler;
    PKPHM_REQUIRED_STATE RequiredState;
} KPH_MESSAGE_HANDLER, *PKPH_MESSAGE_HANDLER;

extern const KPH_MESSAGE_HANDLER KphCommsMessageHandlers[];
extern const ULONG KphCommsMessageHandlerCount;

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
ULONG KphGetConnectedClientCount(
    VOID
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphGetMessageTimeouts(
    _Out_ PKPH_MESSAGE_TIMEOUTS Timeouts
    );

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphSetMessageTimeouts(
    _In_ PKPH_MESSAGE_TIMEOUTS Timeouts
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

_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphCommsSendMessage(
    _In_ PKPH_MESSAGE Message,
    _Out_opt_ PKPH_MESSAGE Reply
    );

_IRQL_requires_max_(APC_LEVEL)
VOID KphCommsSendMessageAsync(
    _In_aliasesMem_ PKPH_MESSAGE Message
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphCommsSendNPagedMessageAsync(
    _In_aliasesMem_ PKPH_MESSAGE Message
    );

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphCaptureStackInMessage(
    _Inout_ PKPH_MESSAGE Message
    );
