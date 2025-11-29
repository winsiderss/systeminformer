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

#pragma once
#include <kphmsg.h>

typedef struct _KPH_CLIENT_RATE_LIMITS
{
    KPH_RATE_LIMIT RateLimit[KPH_INFORMER_COUNT];
} KPH_CLIENT_RATE_LIMITS, *PKPH_CLIENT_RATE_LIMITS;

typedef union _KPH_CLIENT_RATE_LIMITS_ATOMIC
{
    struct _KPH_CLIENT_RATE_LIMITS* Limits;
    KPH_ATOMIC_OBJECT_REF Atomic;
} KPH_CLIENT_RATE_LIMITS_ATOMIC, *PKPH_CLIENT_RATE_LIMITS_ATOMIC;

typedef struct _KPH_CLIENT
{
    PKPH_PROCESS_CONTEXT Process;
    PFLT_PORT Port;
    KPH_REFERENCE DriverUnloadProtectionRef;
    KPH_MESSAGE_TIMEOUTS MessageTimeouts;
    KPH_CLIENT_RATE_LIMITS_ATOMIC RateLimits;
    PKPH_RING_BUFFER RingBuffer;
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
typedef KPHM_HANDLER* PKPHM_HANDLER;

#define KPHM_DEFINE_HANDLER(name)                                              \
_Function_class_(KPHM_HANDLER)                                                 \
_IRQL_requires_max_(PASSIVE_LEVEL)                                             \
_Must_inspect_result_                                                          \
NTSTATUS name(                                                                 \
    _In_ PKPH_CLIENT Client,                                                   \
    _Inout_ PKPH_MESSAGE Message                                               \
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
typedef KPHM_REQUIRED_STATE* PKPHM_REQUIRED_STATE;

#define KPHM_DEFINE_REQUIRED_STATE(name)                                       \
_Function_class_(KPHM_REQUIRED_STATE)                                          \
_IRQL_requires_max_(PASSIVE_LEVEL)                                             \
_Must_inspect_result_                                                          \
KPH_PROCESS_STATE name(                                                        \
    _In_ PKPH_CLIENT Client,                                                   \
    _In_ PCKPH_MESSAGE Message                                                 \
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

_IRQL_requires_max_(DISPATCH_LEVEL)
ULONG KphGetConnectedClientCount(
    VOID
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetMessageSettings(
    _In_ PKPH_CLIENT Client,
    _Out_ PKPH_MESSAGE_SETTINGS Settings
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphSetMessageSettings(
    _In_ PKPH_CLIENT Client,
    _In_ PKPH_MESSAGE_SETTINGS Settings
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetMessageStats(
    _In_ PKPH_CLIENT Client,
    _Out_ PKPH_INFORMER_STATS Stats
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
_Must_inspect_result_
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
