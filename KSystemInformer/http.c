/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023
 *
 */

#include <kph.h>
#include <dyndata.h>

#include <trace.h>

PAGED_FILE();

typedef struct _KPH_HTTP_RESPONSE_CODE_ENTRY
{
    ANSI_STRING Text;
    USHORT StatusCode;
} KPH_HTTP_RESPONSE_CODE_ENTRY, *PKPH_HTTP_RESPONSE_CODE_ENTRY;

typedef struct _KPH_HTTP_VERSION_ENTRY
{
    ANSI_STRING Text;
    KPH_HTTP_VERSION Version;
} KPH_HTTP_VERSION_ENTRY, *PKPH_HTTP_VERSION_ENTRY;

//
// Supported HTTP Protocol Versions
// We do not yet support HTTP/2.0 or HTTP/3.0. The processing of the protocol
// streams will introduce significant complexity and is left for future work.
//
static KPH_HTTP_VERSION_ENTRY KphpHttpVersions[] =
{
    { RTL_CONSTANT_STRING("HTTP/1.0"), KphHttpVersion10 },
    { RTL_CONSTANT_STRING("HTTP/1.1"), KphHttpVersion11 },
};

//
// Known and Supported HTTP Response Codes
// We are explicit about the status codes to help ensure we're consuming valid
// information. The preceding space is intentional for efficient parsing by
// aligning to 4-bytes.
//
static KPH_HTTP_RESPONSE_CODE_ENTRY KphpHttpResponseCodes[] =
{
{ RTL_CONSTANT_STRING(" 100"), 100 }, // Continue
{ RTL_CONSTANT_STRING(" 101"), 101 }, // Switching Protocols
{ RTL_CONSTANT_STRING(" 102"), 102 }, // Processing
{ RTL_CONSTANT_STRING(" 103"), 103 }, // Early Hints
{ RTL_CONSTANT_STRING(" 200"), 200 }, // OK
{ RTL_CONSTANT_STRING(" 201"), 201 }, // Created
{ RTL_CONSTANT_STRING(" 202"), 202 }, // Accepted
{ RTL_CONSTANT_STRING(" 203"), 203 }, // Non-Authoritative Information
{ RTL_CONSTANT_STRING(" 204"), 204 }, // No Content
{ RTL_CONSTANT_STRING(" 205"), 205 }, // Reset Content
{ RTL_CONSTANT_STRING(" 206"), 206 }, // Partial Content
{ RTL_CONSTANT_STRING(" 207"), 207 }, // Multi-Status
{ RTL_CONSTANT_STRING(" 208"), 208 }, // Already Reported
{ RTL_CONSTANT_STRING(" 226"), 226 }, // IM Used
{ RTL_CONSTANT_STRING(" 300"), 300 }, // Multiple Choices
{ RTL_CONSTANT_STRING(" 301"), 301 }, // Moved Permanently
{ RTL_CONSTANT_STRING(" 302"), 302 }, // Found
{ RTL_CONSTANT_STRING(" 303"), 303 }, // See Other
{ RTL_CONSTANT_STRING(" 304"), 304 }, // Not Modified
{ RTL_CONSTANT_STRING(" 305"), 305 }, // Use Proxy
{ RTL_CONSTANT_STRING(" 306"), 306 }, // unused
{ RTL_CONSTANT_STRING(" 307"), 307 }, // Temporary Redirect
{ RTL_CONSTANT_STRING(" 308"), 308 }, // Permanent Redirect
{ RTL_CONSTANT_STRING(" 400"), 400 }, // Bad Request
{ RTL_CONSTANT_STRING(" 401"), 401 }, // Unauthorized
{ RTL_CONSTANT_STRING(" 402"), 402 }, // Payment Required
{ RTL_CONSTANT_STRING(" 403"), 403 }, // Forbidden
{ RTL_CONSTANT_STRING(" 404"), 404 }, // Not Found
{ RTL_CONSTANT_STRING(" 405"), 405 }, // Method Not Allowed
{ RTL_CONSTANT_STRING(" 406"), 406 }, // Not Acceptable
{ RTL_CONSTANT_STRING(" 407"), 407 }, // Proxy Authentication Required
{ RTL_CONSTANT_STRING(" 408"), 408 }, // Request Timeout
{ RTL_CONSTANT_STRING(" 409"), 409 }, // Conflict
{ RTL_CONSTANT_STRING(" 410"), 410 }, // Gone
{ RTL_CONSTANT_STRING(" 411"), 411 }, // Length Required
{ RTL_CONSTANT_STRING(" 412"), 412 }, // Precondition Failed
{ RTL_CONSTANT_STRING(" 413"), 413 }, // Payload Too Large
{ RTL_CONSTANT_STRING(" 414"), 414 }, // URI Too Long
{ RTL_CONSTANT_STRING(" 415"), 415 }, // Unsupported Media Type
{ RTL_CONSTANT_STRING(" 416"), 416 }, // Range Not Satisfiable
{ RTL_CONSTANT_STRING(" 417"), 417 }, // Expectation Failed
{ RTL_CONSTANT_STRING(" 418"), 418 }, // I'm a teapot
{ RTL_CONSTANT_STRING(" 421"), 421 }, // Misdirected Request
{ RTL_CONSTANT_STRING(" 422"), 422 }, // Unprocessable Content
{ RTL_CONSTANT_STRING(" 423"), 423 }, // Locked
{ RTL_CONSTANT_STRING(" 424"), 424 }, // Failed Dependency
{ RTL_CONSTANT_STRING(" 425"), 425 }, // Too Early
{ RTL_CONSTANT_STRING(" 426"), 426 }, // Upgrade Required
{ RTL_CONSTANT_STRING(" 428"), 428 }, // Precondition Required
{ RTL_CONSTANT_STRING(" 429"), 429 }, // Too Many Requests
{ RTL_CONSTANT_STRING(" 431"), 431 }, // Request Header Fields Too Large
{ RTL_CONSTANT_STRING(" 451"), 451 }, // Unavailable For Legal Reasons
{ RTL_CONSTANT_STRING(" 500"), 500 }, // Internal Server Error
{ RTL_CONSTANT_STRING(" 501"), 501 }, // Not Implemented
{ RTL_CONSTANT_STRING(" 502"), 502 }, // Bad Gateway
{ RTL_CONSTANT_STRING(" 503"), 503 }, // Service Unavailable
{ RTL_CONSTANT_STRING(" 504"), 504 }, // Gateway Timeout
{ RTL_CONSTANT_STRING(" 505"), 505 }, // HTTP Version Not Supported
{ RTL_CONSTANT_STRING(" 506"), 506 }, // Variant Also Negotiates
{ RTL_CONSTANT_STRING(" 507"), 507 }, // Insufficient Storage
{ RTL_CONSTANT_STRING(" 508"), 508 }, // Loop Detected
{ RTL_CONSTANT_STRING(" 510"), 510 }, // Not Extended
{ RTL_CONSTANT_STRING(" 511"), 511 }, // Network Authentication Required
};

//
// HTTP Line Ending
// The specification is explicit about CRLF being the standard.
//
static ANSI_STRING KphpHttpHeaderLineEnding = RTL_CONSTANT_STRING("\r\n");

//
// HTTP Header Separator
// The specification is not explicit about the header separator, but it is
// common practice to use a colon and single space.
//
static ANSI_STRING KphpHttpHeaderItemSeparator = RTL_CONSTANT_STRING(": ");

/**
 * \brief Trims whitespace from the beginning and end of a string.
 *
 * \details Internally here we reference information in the input buffers, we
 * don't rely on ever having to free the buffer pointer in the string, so we
 * can safely trim this way.
 *
 * \param[in,out] String The string to trim.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphpHttpTrimWhitespace(
    _Inout_ PANSI_STRING String
    )
{
    CHAR c;

    PAGED_CODE();

    while (String->Length)
    {
        c = String->Buffer[0];

        if ((c == ' ') || (c == '\t'))
        {
            String->Buffer++;
            String->Length--;
            String->MaximumLength--;
        }
        else
        {
            break;
        }
    }

    while (String->Length)
    {
        c = String->Buffer[String->Length - 1];

        if ((c == ' ') || (c == '\t'))
        {
            String->Length--;
        }
        else
        {
            break;
        }
    }
}

/**
 * \brief Parses an HTTP response status line.
 *
 * \param[in] Buffer The buffer containing the status line.
 * \param[in] Length The length of the buffer.
 * \param[out] Version Populated with the HTTP version.
 * \param[out] StatusCode Populated with the HTTP status code.
 * \param[out] StatusMessage Populated with the HTTP status message.
 * \param[out] ParsedLength Populated with the length of the status line.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHttpParseResponseStatusLine(
    _In_ PVOID Buffer,
    _In_ ULONG Length,
    _Out_ PKPH_HTTP_VERSION Version,
    _Out_ PUSHORT StatusCode,
    _Out_ PANSI_STRING StatusMessage,
    _Out_ PULONG ParsedLength
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG remaining;
    PVOID end;

    PAGED_CODE();

    buffer = Buffer;
    remaining = Length;

    *Version = InvalidKphHttpVersion;
    *StatusCode = 0;
    RtlZeroMemory(StatusMessage, sizeof(*StatusMessage));
    *ParsedLength = 0;

    end = KphSearchMemory(Buffer,
                          Length,
                          KphpHttpHeaderLineEnding.Buffer,
                          KphpHttpHeaderLineEnding.Length);
    if (!end)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Failed to find HTTP line ending.");

        return STATUS_INVALID_PARAMETER;
    }

    remaining = PtrOffset(Buffer, end);

    *ParsedLength = (remaining + KphpHttpHeaderLineEnding.Length);

    for (ULONG i = 0; i < ARRAYSIZE(KphpHttpVersions); i++)
    {
        PKPH_HTTP_VERSION_ENTRY versionEntry;

        versionEntry = &KphpHttpVersions[i];

        if (remaining < versionEntry->Text.Length)
        {
            continue;
        }

        if (KphEqualMemory(buffer,
                           versionEntry->Text.Buffer,
                           versionEntry->Text.Length))
        {
            *Version = versionEntry->Version;
            buffer = Add2Ptr(buffer, versionEntry->Text.Length);
            remaining -= versionEntry->Text.Length;
            break;
        }
    }

    if (*Version == InvalidKphHttpVersion)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Unsupported HTTP version.");

        return STATUS_NOT_SUPPORTED;
    }

    for (ULONG i = 0; i < ARRAYSIZE(KphpHttpResponseCodes); i++)
    {
        PKPH_HTTP_RESPONSE_CODE_ENTRY codeEntry;

        codeEntry = &KphpHttpResponseCodes[i];

        if (remaining < codeEntry->Text.Length)
        {
            continue;
        }

        if (KphEqualMemory(buffer,
                           codeEntry->Text.Buffer,
                           codeEntry->Text.Length))
        {
            *StatusCode = codeEntry->StatusCode;
            buffer = Add2Ptr(buffer, codeEntry->Text.Length);
            remaining -= codeEntry->Text.Length;
            break;
        }
    }

    if (*StatusCode == 0)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Failed to locate known HTTP status code.");

        return STATUS_INVALID_PARAMETER;
    }

    status = RtlULongPtrToUShort(PtrOffset(StatusMessage->Buffer, end),
                                 &StatusMessage->Length);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "RtlULongPtrToUShort failed: %!STATUS!",
                      status);

        StatusMessage->Length = 0;
        return status;
    }

    status = RtlULongPtrToUShort(PtrOffset(buffer, end),
                                 &StatusMessage->Length);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "RtlULongPtrToUShort failed: %!STATUS!",
                      status);

        StatusMessage->Length = 0;
        return status;
    }

    StatusMessage->Buffer = buffer;
    StatusMessage->MaximumLength = StatusMessage->Length;

    KphpHttpTrimWhitespace(StatusMessage);

    return STATUS_SUCCESS;
}

/**
 * \brief Parses an HTTP response header line.
 *
 * \param[in] Buffer The buffer containing the header line.
 * \param[in] Length The length of the buffer.
 * \param[out] Item Populated with the header item.
 * \param[out] ParsedLength Populated with the length of the header line.i
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHttpParseResponseHeaderLine(
    _In_ PVOID Buffer,
    _In_ ULONG Length,
    _Out_ PKPH_HTTP_HEADER_ITEM Item,
    _Out_ PULONG ParsedLength
    )
{
    NTSTATUS status;
    PVOID end;
    PVOID separator;
    ULONG remaining;

    PAGED_CODE();

    RtlZeroMemory(Item, sizeof(*Item));
    *ParsedLength = 0;

    end = KphSearchMemory(Buffer,
                          Length,
                          KphpHttpHeaderLineEnding.Buffer,
                          KphpHttpHeaderLineEnding.Length);
    if (!end)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Failed to find HTTP line ending.");

        return STATUS_INVALID_PARAMETER;
    }

    remaining = PtrOffset(Buffer, end);

    *ParsedLength = (remaining + KphpHttpHeaderLineEnding.Length);

    if (!remaining)
    {
        return STATUS_SUCCESS;
    }

    //
    // N.B. Wile by convention most implementations use a colon and single
    // space. The HTTP specification isn't specific on the white space around
    // a header key and value. So we just search for the required separator
    // character and trim afterwards.
    //
    separator = KphSearchMemory(Buffer, remaining, ":", 1);
    if (separator)
    {
        status = RtlULongPtrToUShort(PtrOffset(Buffer, separator),
                                     &Item->Key.Length);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "RtlULongPtrToUShort failed: %!STATUS!",
                          status);

            Item->Key.Length = 0;
            return status;
        }

        Item->Key.Buffer = Buffer;
        Item->Key.MaximumLength = Item->Key.Length;

        status = RtlULongPtrToUShort(PtrOffset(Item->Value.Buffer, end),
                                     &Item->Value.Length);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "RtlULongPtrToUShort failed: %!STATUS!",
                          status);

            Item->Value.Length = 0;
            return status;
        }

        Item->Value.Buffer = Add2Ptr(separator,
                                     KphpHttpHeaderItemSeparator.Length);

        status = RtlULongPtrToUShort(PtrOffset(Item->Value.Buffer, end),
                                     &Item->Value.Length);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "RtlULongPtrToUShort failed: %!STATUS!",
                          status);

            Item->Value.Buffer = NULL;
            Item->Value.Length = 0;
            return status;
        }

        Item->Value.MaximumLength = Item->Value.Length;
    }
    else
    {
        status = RtlULongToUShort(Length, &Item->Key.Length);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "RtlULongToUShort failed: %!STATUS!",
                          status);

            Item->Key.Length = 0;
            return status;
        }

        Item->Key.Buffer = Buffer;
        Item->Key.MaximumLength = Item->Key.Length;
    }

    KphpHttpTrimWhitespace(&Item->Key);
    KphpHttpTrimWhitespace(&Item->Value);

    return STATUS_SUCCESS;
}

/**
 * \brief Parses HTTP response headers.
 *
 * \param[in] Buffer The buffer containing the headers.
 * \param[in] Length The length of the buffer.
 * \param[out] Items Populated with the header items.
 * \param[in,out] ItemCount On input, the number of header items to populate.
 * On output, set to the number of header items in the buffer.
 * \param[out] ParsedLength Populated with the length of the headers.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHttpParseResponseHeaders(
    _In_ PVOID Buffer,
    _In_ ULONG Length,
    _Out_writes_to_opt_(*ItemCount, *ItemCount) PKPH_HTTP_HEADER_ITEM Items,
    _Inout_ PULONG ItemCount,
    _Out_ PULONG ParsedLength
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG remaining;
    ULONG inputCount;
    ULONG count;

    PAGED_CODE();

    buffer = Buffer;
    remaining = Length;
    inputCount = *ItemCount;

    if (Items)
    {
        RtlZeroMemory(Items, sizeof(*Items) * inputCount);
    }

    *ParsedLength = 0;

    for (count = 0;; count++)
    {
        ULONG parsedLength;
        KPH_HTTP_HEADER_ITEM item;

        status = KphpHttpParseResponseHeaderLine(buffer,
                                                 remaining,
                                                 &item,
                                                 &parsedLength);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "KphpHttpParseResponseHeaderLine failed: %!STATUS!",
                          status);

            return status;
        }

        NT_ASSERT(parsedLength <= remaining);

        buffer = Add2Ptr(buffer, parsedLength);
        remaining -= parsedLength;

        if (!item.Key.Length && !item.Value.Length)
        {
            //
            // We've reached a blank line (double CLRF).
            //
            break;
        }

        if (Items && (count < inputCount))
        {
            Items[count] = item;
        }
    }

    *ItemCount = count;
    *ParsedLength = (Length - remaining);

    if (!Items || (count > inputCount))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Frees an HTTP response.
 *
 * \param[in] Response The response to free.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphHttpFreeResponse(
    _In_freesMem_ PKPH_HTTP_RESPONSE Response
    )
{
    PAGED_CODE();

    KphFree(Response, KPH_TAG_HTTP_RESPONSE);
}

/**
 * \brief Parses an HTTP response.
 *
 * \details The returned response object describes and references information
 * in the input buffer. The input buffer *must* outlive the response object.
 * For example, the body pointer in the response object points to the location
 * where the body is in the input buffer, likewise with other header items.
 *
 * \param[in] Buffer The buffer containing the response.
 * \param[in] Length The length of the buffer.
 * \param[out] Response Populated with a pointer to the parsed response
 * information. The caller should free the response information with
 * KphHttpFreeResponse.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphHttpParseResponse(
    _In_ PVOID Buffer,
    _In_ ULONG Length,
    _Outptr_allocatesMem_ PKPH_HTTP_RESPONSE* Response
    )
{
    NTSTATUS status;
    PKPH_HTTP_RESPONSE response;
    PVOID buffer;
    ULONG remaining;
    KPH_HTTP_VERSION version;
    USHORT statusCode;
    ANSI_STRING statusMessage;
    ULONG parsedLength;
    ULONG headerItemCount;
    ULONG needed;

    PAGED_CODE();

    response = NULL;
    buffer = Buffer;
    remaining = Length;

    *Response = NULL;

    status = KphpHttpParseResponseStatusLine(buffer,
                                             remaining,
                                             &version,
                                             &statusCode,
                                             &statusMessage,
                                             &parsedLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphpHttpParseResponseStatusLine failed: %!STATUS!",
                      status);

        goto Exit;
    }

    NT_ASSERT(parsedLength <= remaining);

    buffer = Add2Ptr(buffer, parsedLength);
    remaining -= parsedLength;

    headerItemCount = 0;
    parsedLength = 0;

    status = KphpHttpParseResponseHeaders(buffer,
                                          remaining,
                                          NULL,
                                          &headerItemCount,
                                          &parsedLength);
    if (status != STATUS_BUFFER_TOO_SMALL)
    {
        NT_ASSERT(!NT_SUCCESS(status));

        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphpHttpParseResponseHeaders failed: %!STATUS!",
                      status);

        goto Exit;
    }

    NT_ASSERT(parsedLength <= remaining);

    if (!headerItemCount)
    {
        headerItemCount = 1;
    }

    needed = sizeof(KPH_HTTP_RESPONSE);
    needed += (sizeof(KPH_HTTP_HEADER_ITEM) * ((SIZE_T)headerItemCount - 1));

    response = KphAllocatePaged(needed, KPH_TAG_HTTP_RESPONSE);
    if (!response)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "Failed to allocate response object.");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    response->Version = version;
    response->StatusCode = statusCode;
    response->StatusMessage = statusMessage;

    response->HeaderItemCount = headerItemCount;
    parsedLength = 0;

    status = KphpHttpParseResponseHeaders(buffer,
                                          remaining,
                                          response->HeaderItems,
                                          &response->HeaderItemCount,
                                          &parsedLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphpHttpParseResponseHeaders failed: %!STATUS!",
                      status);

        goto Exit;
    }

    NT_ASSERT(headerItemCount == response->HeaderItemCount);
    NT_ASSERT(parsedLength <= remaining);

    buffer = Add2Ptr(buffer, parsedLength);
    remaining -= parsedLength;

    response->Body = buffer;
    response->BodyLength = remaining;

    *Response = response;
    response = NULL;

    status = STATUS_SUCCESS;

Exit:

    if (response)
    {
        KphFree(response, KPH_TAG_HTTP_RESPONSE);
    }

    return status;
}

/**
 * \brief Builds an HTTP request.
 *
 * \details This function build an HTTP/1.1 request. It includes the host in
 * the HTTP headers automatically it is not necessary to also specify the host
 * in the header items. The parameters to the path are broken out and will be
 * appended to the path. In example:
 *
 * Method    Path              Parameters
 * |-| |---------------||--------------------------|
 * GET /path/to/resource?param1=value1&param2=value2 HTTP/1.1
 * Host: example.com
 *       |---------|
 *          Host
 *
 * \param[in] Method The HTTP method.
 * \param[in] Host The HTTP host.
 * \param[in] Path The HTTP path.
 * \param[in] Parameters The parameters appended to the path.
 * \param[in] HeaderItems Optional header items.
 * \param[in] HeaderItemCount The number of header items.
 * \param[in] Body Optional body to append to the request.
 * \param[in] BodyLength The length of the body.
 * \param[out] Buffer Optionally populated with the request.
 * \param[in,out] Length On input, the length of the buffer. On output, the
 * number of bytes written to the buffer or the number of bytes required.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphHttpBuildRequest(
    _In_ PANSI_STRING Method,
    _In_ PANSI_STRING Host,
    _In_ PANSI_STRING Path,
    _In_ PANSI_STRING Parameters,
    _In_opt_ PKPH_HTTP_HEADER_ITEM HeaderItems,
    _In_ ULONG HeaderItemCount,
    _In_opt_ PVOID Body,
    _In_ ULONG BodyLength,
    _Out_writes_bytes_to_opt_(*Length, *Length) PVOID Buffer,
    _Inout_ PULONG Length
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG remaining;
    ULONG length;

    PAGED_CODE();

    buffer = Buffer;
    remaining = *Length;
    length = 0;

#define KphpHttpBuildReqCopyMem(p, l)                                         \
    if (buffer && (l <= remaining))                                           \
    {                                                                         \
        RtlCopyMemory(buffer, p, l);                                          \
        buffer = Add2Ptr(buffer, l);                                          \
        remaining -= l;                                                       \
    }                                                                         \
    status = RtlULongAdd(length, l, &length);                                 \
    if (!NT_SUCCESS(status))                                                  \
    {                                                                         \
        return status;                                                        \
    }

    KphpHttpBuildReqCopyMem(Method->Buffer, Method->Length);
    KphpHttpBuildReqCopyMem(" ", 1);
    KphpHttpBuildReqCopyMem(Path->Buffer, Path->Length);
    KphpHttpBuildReqCopyMem(Parameters->Buffer, Parameters->Length);
    KphpHttpBuildReqCopyMem(" HTTP/1.1", 9);
    KphpHttpBuildReqCopyMem(KphpHttpHeaderLineEnding.Buffer,
                            KphpHttpHeaderLineEnding.Length);

    KphpHttpBuildReqCopyMem("Host: ", 6);
    KphpHttpBuildReqCopyMem(Host->Buffer, Host->Length);
    KphpHttpBuildReqCopyMem(KphpHttpHeaderLineEnding.Buffer,
                            KphpHttpHeaderLineEnding.Length);

    if (HeaderItems)
    {
        for (ULONG i = 0; i < HeaderItemCount; i++)
        {
            PKPH_HTTP_HEADER_ITEM item;

            item = &HeaderItems[i];

            KphpHttpBuildReqCopyMem(item->Key.Buffer, item->Key.Length);

            KphpHttpBuildReqCopyMem(KphpHttpHeaderItemSeparator.Buffer,
                                    KphpHttpHeaderItemSeparator.Length);

            KphpHttpBuildReqCopyMem(item->Value.Buffer, item->Value.Length);

            KphpHttpBuildReqCopyMem(KphpHttpHeaderLineEnding.Buffer,
                                    KphpHttpHeaderLineEnding.Length);
        }
    }

    KphpHttpBuildReqCopyMem(KphpHttpHeaderLineEnding.Buffer,
                            KphpHttpHeaderLineEnding.Length);

    if (Body)
    {
        KphpHttpBuildReqCopyMem(Body, BodyLength);
    }

    if (!Buffer || (length > *Length))
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }

    *Length = length;

    return status;
}
