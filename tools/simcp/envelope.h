/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#ifndef _SIMCP_ENVELOPE_H
#define _SIMCP_ENVELOPE_H

typedef enum _SIMCP_ENVELOPE_KIND
{
    SimcpEnvelopeUnparsed = 0,  // not JSON at all
    SimcpEnvelopeOther,         // valid JSON the broker does not track: a batch array, or an
                                // id-less, method-less object such as an error with a null id
    SimcpEnvelopeRequest,
    SimcpEnvelopeNotification,
    SimcpEnvelopeResponse,
} SIMCP_ENVELOPE_KIND;

typedef struct _SIMCP_ENVELOPE
{
    SIMCP_ENVELOPE_KIND Kind;
    PPH_BYTES Id;       // raw id JSON text, so a number and a string are both carried verbatim
    PPH_STRING Method;
    PPH_BYTES CancelId; // params.requestId of a notifications/cancelled, else NULL
    PPH_STRING ProtocolVersion; // result.protocolVersion of an initialize reply, else NULL
    BOOLEAN ModernMeta;         // the request carries the modern protocol version in params._meta
} SIMCP_ENVELOPE, *PSIMCP_ENVELOPE;

VOID SimcpParseEnvelope(
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_ PSIMCP_ENVELOPE Envelope
    );

VOID SimcpDeleteEnvelope(
    _Inout_ PSIMCP_ENVELOPE Envelope
    );

PPH_BYTES SimcpRewriteEnvelopeId(
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_ PCSTR IdString
    );

#endif
