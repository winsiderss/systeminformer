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

#define SIMCP_META_PROTOCOL_VERSION "io.modelcontextprotocol/protocolVersion"
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

    // The modern path never sends initialize; it carries the version on every request instead.
    if (Envelope->Kind == SimcpEnvelopeRequest || Envelope->Kind == SimcpEnvelopeNotification)
    {
        PVOID params;

        if (params = PhGetJsonObject(message, "params"))
        {
            if (PhGetJsonObjectType(params) == PH_JSON_OBJECT_TYPE_OBJECT)
            {
                PVOID meta;

                if (meta = PhGetJsonObject(params, "_meta"))
                {
                    if (PhGetJsonObjectType(meta) == PH_JSON_OBJECT_TYPE_OBJECT)
                        Envelope->ModernMeta = !!PhGetJsonObject(meta, SIMCP_META_PROTOCOL_VERSION);
                }
            }
        }
    }

    // The negotiated version of a handshake reply; a new backend must not change it mid-session.
    if (Envelope->Kind == SimcpEnvelopeResponse)
    {
        PVOID result;

        if (result = PhGetJsonObject(message, "result"))
        {
            if (PhGetJsonObjectType(result) == PH_JSON_OBJECT_TYPE_OBJECT)
                Envelope->ProtocolVersion = PhGetJsonValueAsString(result, "protocolVersion");
        }
    }

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
    if (Envelope->ProtocolVersion)
        PhDereferenceObject(Envelope->ProtocolVersion);
    if (Envelope->CancelId)
        PhDereferenceObject(Envelope->CancelId);
    if (Envelope->Id)
        PhDereferenceObject(Envelope->Id);
    if (Envelope->Method)
        PhDereferenceObject(Envelope->Method);

    memset(Envelope, 0, sizeof(SIMCP_ENVELOPE));
}

/**
 * Rebuilds a line with a different id.
 *
 * Only the id is touched; every other member is carried across by the serialiser untouched.
 *
 * \param Buffer The line, without its terminator.
 * \param Length The length of the line in bytes.
 * \param IdString The replacement id, written as a JSON string.
 * \return The rewritten line, or NULL when the line is not an object.
 */
PPH_BYTES SimcpRewriteEnvelopeId(
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_ PCSTR IdString
    )
{
    NTSTATUS status;
    PPH_BYTES bytes;
    PPH_BYTES rewritten = NULL;
    PVOID message = NULL;

    bytes = PhCreateBytesEx(Buffer, Length);
    status = PhCreateJsonParserEx(&message, bytes, FALSE);
    PhDereferenceObject(bytes);

    if (!NT_SUCCESS(status) || !message)
        return NULL;

    if (PhGetJsonObjectType(message) == PH_JSON_OBJECT_TYPE_OBJECT)
    {
        PhAddJsonObject(message, "id", IdString);
        rewritten = PhGetJsonArrayString(message, FALSE);
    }

    PhFreeJsonObject(message);

    return rewritten;
}

/**
 * Rebuilds a line with a numeric id.
 *
 * System Informer matches a reply to its own request with a strict integer type test, so an id it
 * minted has to come back as a number and not as the string the broker sent out.
 *
 * \param Buffer The line, without its terminator.
 * \param Length The length of the line in bytes.
 * \param Id The replacement id.
 * \return The rewritten line, or NULL when the line is not an object.
 */
PPH_BYTES SimcpRewriteEnvelopeIdInteger(
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_ LONG64 Id
    )
{
    NTSTATUS status;
    PPH_BYTES bytes;
    PPH_BYTES rewritten = NULL;
    PVOID message = NULL;

    bytes = PhCreateBytesEx(Buffer, Length);
    status = PhCreateJsonParserEx(&message, bytes, FALSE);
    PhDereferenceObject(bytes);

    if (!NT_SUCCESS(status) || !message)
        return NULL;

    if (PhGetJsonObjectType(message) == PH_JSON_OBJECT_TYPE_OBJECT)
    {
        PhAddJsonObjectInt64(message, "id", Id);
        rewritten = PhGetJsonArrayString(message, FALSE);
    }

    PhFreeJsonObject(message);

    return rewritten;
}
