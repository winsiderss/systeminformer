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
    SimcpEnvelopeUnparsed = 0,
    SimcpEnvelopeOther,
    SimcpEnvelopeRequest,
    SimcpEnvelopeNotification,
    SimcpEnvelopeResponse,
} SIMCP_ENVELOPE_KIND;

typedef struct _SIMCP_ENVELOPE
{
    SIMCP_ENVELOPE_KIND Kind;
    PPH_BYTES Id;
    PPH_STRING Method;
    PPH_BYTES CancelId;
    PPH_STRING ProtocolVersion;
    BOOLEAN ModernMeta;
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

PPH_BYTES SimcpRewriteEnvelopeIdInteger(
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_ LONG64 Id
    );

#endif
