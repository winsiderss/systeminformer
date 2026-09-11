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

#include <ph.h>
#include <json.h>
#include <simcp.h>
#include "envelope.h"

/**
 * Reads the JSON-RPC envelope fields of one line.
 *
 * Only id and method are read; a tool payload is never walked. A line this cannot classify
 * leaves Kind as SimcpEnvelopeUnparsed, and the caller relays it unchanged.
 *
 * \param Buffer The line, without its terminator. Not null terminated.
 * \param Length The length of the line in bytes.
 * \param Envelope Receives the envelope fields.
 */
VOID SimcpParseEnvelope(
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_ PSIMCP_ENVELOPE Envelope
    )
{
    NTSTATUS status;
    PPH_BYTES bytes;
    PVOID message = NULL;
    PVOID idObject;

    memset(Envelope, 0, sizeof(SIMCP_ENVELOPE));

    // PhCreateJsonParser takes a null terminated string; the line is a range inside a larger
    // buffer, so it has to be the counted overload.
    bytes = PhCreateBytesEx(Buffer, Length);
    status = PhCreateJsonParserEx(&message, bytes, FALSE);
    PhDereferenceObject(bytes);

    if (!NT_SUCCESS(status) || !message)
        return;

    // A batch array or a bare scalar is relayed untracked rather than queried for members.
    if (PhGetJsonObjectType(message) != PH_JSON_OBJECT_TYPE_OBJECT)
    {
        Envelope->Kind = SimcpEnvelopeOther;
        PhFreeJsonObject(message);
        return;
    }

    Envelope->Kind = SimcpEnvelopeOther;

    Envelope->Method = PhGetJsonValueAsString(message, "method");

    idObject = PhGetJsonObject(message, "id");

    // An explicit null id is the same as no id at all.
    if (idObject && PhGetJsonObjectType(idObject) == PH_JSON_OBJECT_TYPE_NULL)
        idObject = NULL;

    if (idObject)
        Envelope->Id = PhGetJsonArrayString(idObject, FALSE);

    // Neither field leaves Kind as Other: an error with a null id is a normal message that
    // simply cannot be matched to a request.
    if (Envelope->Method)
        Envelope->Kind = Envelope->Id ? SimcpEnvelopeRequest : SimcpEnvelopeNotification;
    else if (Envelope->Id)
        Envelope->Kind = SimcpEnvelopeResponse;

    // The request a cancellation names is protocol metadata, not tool payload.
    if (Envelope->Kind == SimcpEnvelopeNotification &&
        PhEqualString2(Envelope->Method, L"notifications/cancelled", FALSE))
    {
        PVOID params;

        if (params = PhGetJsonObject(message, "params"))
        {
            if (PhGetJsonObjectType(params) == PH_JSON_OBJECT_TYPE_OBJECT)
            {
                PVOID requestId;

                if (requestId = PhGetJsonObject(params, "requestId"))
                {
                    if (PhGetJsonObjectType(requestId) != PH_JSON_OBJECT_TYPE_NULL)
                        Envelope->CancelId = PhGetJsonArrayString(requestId, FALSE);
                }
            }
        }
    }

    PhFreeJsonObject(message);
}

VOID SimcpDeleteEnvelope(
    _Inout_ PSIMCP_ENVELOPE Envelope
    )
{
    if (Envelope->CancelId)
        PhDereferenceObject(Envelope->CancelId);
    if (Envelope->Id)
        PhDereferenceObject(Envelope->Id);
    if (Envelope->Method)
        PhDereferenceObject(Envelope->Method);

    memset(Envelope, 0, sizeof(SIMCP_ENVELOPE));
}
