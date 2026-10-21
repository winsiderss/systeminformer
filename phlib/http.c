/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017-2026
 *
 */

#include <ph.h>
#include <phnet.h>

#include <winhttp.h>
#include <apiimport.h>
#include <mapldr.h>

static const PH_FLAG_MAPPING PhpHttpRequestFlagMappings[] =
{
    { PH_HTTP_FLAG_SECURE, WINHTTP_FLAG_SECURE },
    { PH_HTTP_FLAG_REFRESH, WINHTTP_FLAG_REFRESH }
};

static const PH_FLAG_MAPPING PhpHttpHeaderQueryMappings[] =
{
    { PH_HTTP_QUERY_CONTENT_LENGTH, WINHTTP_QUERY_CONTENT_LENGTH },
    { PH_HTTP_QUERY_STATUS_CODE, WINHTTP_QUERY_STATUS_CODE },
};

static const PH_FLAG_MAPPING PhpHttpFeatureMappings[] =
{
    { PH_HTTP_FEATURE_REDIRECTS, WINHTTP_DISABLE_REDIRECTS },
    { PH_HTTP_FEATURE_KEEP_ALIVE, WINHTTP_DISABLE_KEEP_ALIVE },
};

static const PH_FLAG_MAPPING PhpHttpSecurityFlagsMappings[] =
{
    { PH_HTTP_SECURITY_IGNORE_UNKNOWN_CA, SECURITY_FLAG_IGNORE_UNKNOWN_CA },
    { PH_HTTP_SECURITY_IGNORE_CERT_DATE_INVALID, SECURITY_FLAG_IGNORE_CERT_DATE_INVALID },
};

NTSTATUS PhHttpErrorToNtStatus(
    _In_ ULONG WinhttpError
    );

VOID CALLBACK PhWinHttpStatusCallback(
    _In_ HINTERNET InternetHandle,
    _In_ ULONG_PTR Context,
    _In_ ULONG InternetStatus,
    _In_opt_ PVOID StatusInformation,
    _In_ ULONG StatusInformationLength
    );

NTSTATUS PhGetLastWinHttpErrorAsNtStatus(
    VOID
    )
{
    return PhHttpErrorToNtStatus(PhGetLastError());
}

PPH_STRING PhWinHttpUserAgentString(
    VOID
    )
{
    PH_FORMAT format[4];
    SIZE_T returnLength;
    WCHAR formatBuffer[260];

#ifndef PHAPP_VERSION_MAJOR
#define PHAPP_VERSION_MAJOR 0
#endif

#ifndef PHAPP_VERSION_MINOR
#define PHAPP_VERSION_MINOR 0
#endif

    PhInitFormatS(&format[0], L"SystemInformer_A2D1C96D_D25915D9_");
    PhInitFormatU(&format[1], PHAPP_VERSION_MAJOR);
    PhInitFormatC(&format[2], L'_');
    PhInitFormatU(&format[3], PHAPP_VERSION_MINOR);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &returnLength))
    {
        PH_STRINGREF stringFormat;

        stringFormat.Buffer = formatBuffer;
        stringFormat.Length = returnLength - sizeof(UNICODE_NULL);

        return PhCreateString2(&stringFormat);
    }

    return PhFormat(format, RTL_NUMBER_OF(format), 0);
}

static NTSTATUS PhpHttpSetOptionString(
    _In_ PVOID HttpHandle,
    _In_ ULONG Option,
    _In_ PPH_STRINGREF Value
    )
{
    ULONG optionLength = (ULONG)Value->Length;

    if (WinHttpSetOption(
        HttpHandle,
        Option,
        Value->Buffer,
        optionLength
        ))
    {
        return STATUS_SUCCESS;
    }

    return PhGetLastWinHttpErrorAsNtStatus();
}

static NTSTATUS PhpHttpInvokeEventCallback(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PHHTTP_EVENT_TYPE Event,
    _In_opt_ PVOID Parameter
    )
{
    if (HttpContext->Callback)
        return HttpContext->Callback(Event, Parameter, HttpContext->Context);

    return STATUS_SUCCESS;
}

static NTSTATUS PhWinHttpOpen(
    _Out_ HINTERNET* SessionHandle
    )
{
    NTSTATUS status;
    HINTERNET sessionHandle;
    PPH_STRING httpUserAgent;
    ULONG httpAccessType;

    httpUserAgent = PhWinHttpUserAgentString();

    if (WindowsVersion >= WINDOWS_8_1)
        httpAccessType = WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY;
    else
        httpAccessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;

    if (sessionHandle = WinHttpOpen(
        PhGetString(httpUserAgent),
        httpAccessType,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
        ))
    {
        *SessionHandle = sessionHandle;
        status = STATUS_SUCCESS;
    }
    else
    {
        *SessionHandle = NULL;
        status = PhGetLastWinHttpErrorAsNtStatus();
    }

    PhDereferenceObject(httpUserAgent);

    return status;
}

static NTSTATUS PhWinHttpConnect(
    _In_ HINTERNET SessionHandle,
    _In_ PCWSTR ServerName,
    _In_ USHORT ServerPort,
    _Out_ HINTERNET* ConnectionHandle
    )
{
    HINTERNET connectionHandle;

    if (connectionHandle = WinHttpConnect(
        SessionHandle,
        ServerName,
        ServerPort,
        0
        ))
    {
        *ConnectionHandle = connectionHandle;
        return STATUS_SUCCESS;
    }

    return PhGetLastWinHttpErrorAsNtStatus();
}

static NTSTATUS PhWinHttpOpenRequest(
    _In_ HINTERNET ConnectionHandle,
    _In_ PCWSTR RequestMethod,
    _In_ PCWSTR RequestPath,
    _In_ ULONG Flags,
    _Out_ HINTERNET* RequestHandle
    )
{
    HINTERNET requestHandle;
    ULONG httpFlags = 0;
    //ULONG httpOptions;

    PhMapFlags1(
        &httpFlags,
        Flags,
        PhpHttpRequestFlagMappings,
        RTL_NUMBER_OF(PhpHttpRequestFlagMappings)
        );

    if (requestHandle = WinHttpOpenRequest(
        ConnectionHandle,
        RequestMethod,
        RequestPath,
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        httpFlags
        ))
    {
        *RequestHandle = requestHandle;
        return STATUS_SUCCESS;
    }

    return PhGetLastWinHttpErrorAsNtStatus();
}

NTSTATUS PhHttpInitialize(
    _Out_ PPH_HTTP_CONTEXT *HttpContext
    )
{
    NTSTATUS status;
    PPH_HTTP_CONTEXT httpContext;
    HINTERNET sessionHandle;

    status = PhWinHttpOpen(&sessionHandle);

    if (!NT_SUCCESS(status))
        return status;

    httpContext = PhAllocateZero(sizeof(PH_HTTP_CONTEXT));
    httpContext->SessionHandle = sessionHandle;

    if (WindowsVersion < WINDOWS_8_1)
    {
        PhHttpSetOption(sessionHandle, WINHTTP_OPTION_SECURE_PROTOCOLS, WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2);
    }
    else
    {
        PhHttpSetOption(sessionHandle, WINHTTP_OPTION_SECURE_PROTOCOLS, WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3);
        PhHttpSetOption(sessionHandle, WINHTTP_OPTION_DECOMPRESSION, WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE);
        PhHttpSetOption(sessionHandle, WINHTTP_OPTION_ASSURED_NON_BLOCKING_CALLBACKS, TRUE);

        if (WindowsVersion >= WINDOWS_10)
        {
            PhHttpSetOption(sessionHandle, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, WINHTTP_PROTOCOL_FLAG_HTTP2);
            //PhHttpSetOption(sessionHandle, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, WINHTTP_PROTOCOL_FLAG_HTTP2 | WINHTTP_PROTOCOL_FLAG_HTTP3);
        }

        if (WindowsVersion >= WINDOWS_10_RS5)
        {
            PhHttpSetOption(sessionHandle, WINHTTP_OPTION_IPV6_FAST_FALLBACK, TRUE);
            PhHttpSetOption(sessionHandle, WINHTTP_OPTION_DISABLE_STREAM_QUEUE, TRUE);
        }

        if (WindowsVersion >= WINDOWS_11)
        {
            PhHttpSetOption(sessionHandle, WINHTTP_OPTION_DISABLE_GLOBAL_POOLING, TRUE);
            PhHttpSetOption(sessionHandle, WINHTTP_OPTION_TLS_FALSE_START, TRUE);
            PhHttpSetOption(sessionHandle, WINHTTP_OPTION_TCP_FAST_OPEN, TRUE);
        }

        if (WindowsVersion >= WINDOWS_11_24H2)
        {
            PhHttpSetOption(sessionHandle, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, WINHTTP_PROTOCOL_FLAG_HTTP2 | WINHTTP_PROTOCOL_FLAG_HTTP3);
            PhHttpSetOption(sessionHandle, WINHTTP_OPTION_HTTP3_HANDSHAKE_TIMEOUT, 5000); // 5 second timeout before reverting to HTTP2
        }
    }

    *HttpContext = httpContext;
    return STATUS_SUCCESS;
}

VOID PhHttpDestroy(
    _In_ _Maybenull_ _Frees_ptr_ PPH_HTTP_CONTEXT HttpContext
    )
{
    if (!HttpContext)
        return;

    // if (HttpContext->SessionHandle && HttpContext->EventCallbackRegistered)
    // {
    //     // WinHTTP does not synchronize WinHttpSetStatusCallback with worker threads.
    //     // If a callback originating in another thread is in progress when an application calls WinHttpSetStatusCallback,
    //     // the application still receives a callback notification even after WinHttpSetStatusCallback successfully
    //     // sets the callback function to NULL and returns.
    //
    //     PhHttpSetCallback(HttpContext->SessionHandle, NULL, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS);
    // }

    PhHttpClose(HttpContext, ULONG_MAX);

    PhFree(HttpContext);
}

VOID PhHttpClose(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PH_HTTP_SOCKET_CLOSE_TYPE Type
    )
{
    if (Type & PH_HTTP_SOCKET_CLOSE_SESSION)
    {
        if (HttpContext->SessionHandle)
        {
            WinHttpCloseHandle(HttpContext->SessionHandle);
            HttpContext->SessionHandle = NULL;
        }
    }

    if (Type & PH_HTTP_SOCKET_CLOSE_CONNECTION)
    {
        if (HttpContext->ConnectionHandle)
        {
            WinHttpCloseHandle(HttpContext->ConnectionHandle);
            HttpContext->ConnectionHandle = NULL;
        }
    }

    if (Type & PH_HTTP_SOCKET_CLOSE_REQUEST)
    {
        if (HttpContext->RequestHandle)
        {
            WinHttpCloseHandle(HttpContext->RequestHandle);
            HttpContext->RequestHandle = NULL;
        }
    }
}

NTSTATUS PhHttpConnect(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PCWSTR ServerName,
    _In_ USHORT ServerPort
    )
{
    NTSTATUS status;
    HINTERNET connectionHandle;

    if (!NT_SUCCESS(status = PhpHttpInvokeEventCallback(HttpContext, PHHTTP_EVENT_CONNECTING, NULL)))
        return status;

    status = PhWinHttpConnect(
        HttpContext->SessionHandle,
        ServerName,
        ServerPort,
        &connectionHandle
        );

    if (NT_SUCCESS(status))
    {
        HttpContext->ConnectionHandle = connectionHandle;
    }

    //PDNS_RECORD dnsRecordList = PhDnsQuery(ServerName, DNS_TYPE_A);
    //
    //if (dnsRecordList)
    //{
    //    for (PDNS_RECORD dnsRecord = dnsRecordList; dnsRecord; dnsRecord = dnsRecord->pNext)
    //    {
    //        switch (dnsRecord->wType)
    //        {
    //        case DNS_TYPE_A:
    //            {
    //                WCHAR ipAddressString[INET_ADDRSTRLEN];
    //
    //                RtlIpv4AddressToString((PIN_ADDR)&dnsRecord->Data.A.IpAddress, ipAddressString);
    //
    //                HttpContext->ServerName = ServerName;
    //                HttpContext->ConnectionHandle = WinHttpConnect(
    //                    HttpContext->SessionHandle,
    //                    ipAddressString,
    //                    ServerPort,
    //                    0
    //                    );
    //            }
    //            break;
    //        case DNS_TYPE_AAAA:
    //            {
    //                WCHAR ipAddressString[INET6_ADDRSTRLEN];
    //
    //                RtlIpv6AddressToString((PIN6_ADDR)&dnsRecord->Data.AAAA.Ip6Address, ipAddressString);
    //
    //                HttpContext->ServerName = ServerName;
    //                HttpContext->ConnectionHandle = WinHttpConnect(
    //                    HttpContext->SessionHandle,
    //                    ipAddressString,
    //                    ServerPort,
    //                    0
    //                    );
    //            }
    //            break;
    //        }
    //    }
    //
    //    DnsFree(dnsRecordList, DnsFreeRecordList);
    //}

    return status;
}

NTSTATUS PhHttpBeginRequest(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_opt_ PCWSTR RequestMethod,
    _In_ PCWSTR RequestPath,
    _In_ ULONG Flags
    )
{
    NTSTATUS status;
    HINTERNET requestHandle;

    status = PhWinHttpOpenRequest(
        HttpContext->ConnectionHandle,
        RequestMethod,
        RequestPath,
        Flags,
        &requestHandle
        );

    if (NT_SUCCESS(status))
    {
        HttpContext->RequestHandle = requestHandle;

        //PhHttpSetContext(requestHandle, HttpContext);
    }

    return status;
}

NTSTATUS PhHttpSendRequest(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_opt_ PCWSTR HeadersBuffer,
    _In_ ULONG HeadersLength,
    _In_opt_ PVOID OptionalBuffer,
    _In_ ULONG OptionalLength,
    _In_ ULONG TotalLength
    )
{
    NTSTATUS status;

    if (!NT_SUCCESS(status = PhpHttpInvokeEventCallback(HttpContext, PHHTTP_EVENT_SENDING_REQUEST, NULL)))
        return status;

    if (WinHttpSendRequest(
        HttpContext->RequestHandle,
        HeadersBuffer,
        HeadersLength,
        OptionalBuffer,
        OptionalLength,
        TotalLength,
        (ULONG_PTR)HttpContext
        ))
    {
        return STATUS_SUCCESS;
    }

    return PhGetLastWinHttpErrorAsNtStatus();
}

NTSTATUS PhHttpReceiveResponse(
    _In_ PPH_HTTP_CONTEXT HttpContext
    )
{
    NTSTATUS status;

    if (!NT_SUCCESS(status = PhpHttpInvokeEventCallback(HttpContext, PHHTTP_EVENT_RECEIVING_RESPONSE, NULL)))
        return status;

    if (WinHttpReceiveResponse(HttpContext->RequestHandle, NULL))
    {
        //ULONG connectionInfoLength;
        //WINHTTP_CONNECTION_INFO connectionInfo;
        //
        //connectionInfoLength = sizeof(WINHTTP_CONNECTION_INFO);
        //memset(&connectionInfo, 0, connectionInfoLength);
        //connectionInfo.cbSize = connectionInfoLength;
        //
        //if (WinHttpQueryOption(
        //    HttpContext->RequestHandle,
        //    WINHTTP_OPTION_CONNECTION_INFO,
        //    &connectionInfo,
        //    &connectionInfoLength
        //    ))
        //{
        //
        //}
#if DEBUG
        {
            ULONG option = 0;
            ULONG optionLength = sizeof(ULONG);

            if (WinHttpQueryOption(
                HttpContext->RequestHandle,
                WINHTTP_OPTION_HTTP_PROTOCOL_USED,
                &option,
                &optionLength
                ))
            {
                if (option & WINHTTP_PROTOCOL_FLAG_HTTP3)
                {
                    dprintf("[PH_HTTP] %s", "HTTP3 socket\n");
                }

                if (option & WINHTTP_PROTOCOL_FLAG_HTTP2)
                {
                    dprintf("[PH_HTTP] %s", "HTTP2 socket\n");
                }
            }
        }
#endif

        status = PhpHttpInvokeEventCallback(HttpContext, PHHTTP_EVENT_RESPONSE_RECEIVED, NULL);

        if (!NT_SUCCESS(status))
            return status;

        return STATUS_SUCCESS;
    }

    return PhGetLastWinHttpErrorAsNtStatus();
}

NTSTATUS PhHttpReadData(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG BytesCopied
    )
{
    if (WinHttpReadData(
        HttpContext->RequestHandle,
        Buffer,
        BufferLength,
        BytesCopied
        ))
    {
        return STATUS_SUCCESS;
    }

    return PhGetLastWinHttpErrorAsNtStatus();
}

NTSTATUS PhHttpWriteData(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG BytesCopied
    )
{
    if (WinHttpWriteData(
        HttpContext->RequestHandle,
        Buffer,
        BufferLength,
        BytesCopied
        ))
    {
        return STATUS_SUCCESS;
    }

    return PhGetLastWinHttpErrorAsNtStatus();
}

NTSTATUS PhHttpAddRequestHeaders(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PCWSTR Headers,
    _In_opt_ ULONG HeadersLength
    )
{
    if (WinHttpAddRequestHeaders(
        HttpContext->RequestHandle,
        Headers,
        HeadersLength ? HeadersLength : ULONG_MAX,
        WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE
        ))
    {
        return STATUS_SUCCESS;
    }

    return PhGetLastWinHttpErrorAsNtStatus();
}

PPH_STRING PhHttpQueryHeaders(
    _In_ PPH_HTTP_CONTEXT HttpContext
    )
{
    ULONG bufferLength = 0;
    PPH_STRING stringBuffer;

    if (!WinHttpQueryHeaders(
        HttpContext->RequestHandle,
        WINHTTP_QUERY_RAW_HEADERS_CRLF,
        NULL,
        WINHTTP_NO_OUTPUT_BUFFER,
        &bufferLength,
        WINHTTP_NO_HEADER_INDEX
        ))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return NULL;
    }

    stringBuffer = PhCreateStringEx(NULL, bufferLength);

    if (!WinHttpQueryHeaders(
        HttpContext->RequestHandle,
        WINHTTP_QUERY_RAW_HEADERS_CRLF,
        NULL,
        stringBuffer->Buffer,
        &bufferLength,
        WINHTTP_NO_HEADER_INDEX
        ))
    {
        PhDereferenceObject(stringBuffer);
        return NULL;
    }

    return stringBuffer;
}

PPH_STRING PhHttpQueryHeaderString(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PCWSTR HeaderString
    )
{
    ULONG bufferLength = 0;
    PPH_STRING stringBuffer;

    if (!WinHttpQueryHeaders(
        HttpContext->RequestHandle,
        WINHTTP_QUERY_CUSTOM,
        HeaderString,
        WINHTTP_NO_OUTPUT_BUFFER,
        &bufferLength, // includes null
        WINHTTP_NO_HEADER_INDEX
        ))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return NULL;
    }

    stringBuffer = PhCreateStringEx(NULL, bufferLength);

    if (!WinHttpQueryHeaders(
        HttpContext->RequestHandle,
        WINHTTP_QUERY_CUSTOM,
        HeaderString,
        stringBuffer->Buffer,
        &bufferLength, // excludes null
        WINHTTP_NO_HEADER_INDEX
        ))
    {
        PhDereferenceObject(stringBuffer);
        return NULL;
    }

    PhTrimToNullTerminatorString(stringBuffer);
    //stringBuffer->Length = bufferLength;

    return stringBuffer;
}

NTSTATUS PhHttpQueryHeaderUlong(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ ULONG QueryValue,
    _Out_ PULONG HeaderValue
    )
{
    //ULONG queryFlags = 0;
    //ULONG headerValue = 0;
    //ULONG valueLength = sizeof(ULONG);
    //
    //PhMapFlags1(
    //    &queryFlags,
    //    QueryValue,
    //    PhpHttpHeaderQueryMappings,
    //    RTL_NUMBER_OF(PhpHttpHeaderQueryMappings)
    //    );
    //
    //if (WinHttpQueryHeaders(
    //    HttpContext->RequestHandle,
    //    queryFlags | WINHTTP_QUERY_FLAG_NUMBER,
    //    WINHTTP_HEADER_NAME_BY_INDEX,
    //    &headerValue,
    //    &valueLength,
    //    WINHTTP_NO_HEADER_INDEX
    //    ))
    //{
    //    *HeaderValue = headerValue;
    //    return TRUE;
    //}

    NTSTATUS status;
    ULONG64 headerValue;

    status = PhHttpQueryHeaderUlong64(
        HttpContext,
        QueryValue,
        &headerValue
        );

    if (NT_SUCCESS(status))
    {
        if (headerValue <= ULONG_MAX)
        {
            *HeaderValue = (ULONG)headerValue;
            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_INTEGER_OVERFLOW;
        }
    }

    return status;
}

NTSTATUS PhHttpQueryHeaderUlong64(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ ULONG QueryValue,
    _Out_ PULONG64 HeaderValue
    )
{
    ULONG queryFlags = 0;
    ULONG valueLength = 0x100;
    WCHAR valueBuffer[0x100];

    PhMapFlags1(
        &queryFlags,
        QueryValue,
        PhpHttpHeaderQueryMappings,
        RTL_NUMBER_OF(PhpHttpHeaderQueryMappings)
        );

    // Note: The WINHTTP_QUERY_FLAG_NUMBER flag returns invalid integers when
    // querying some types with ULONG64 like WINHTTP_QUERY_STATUS_CODE. So we'll
    // do the conversion for improved reliability and performance. (dmex)

    if (WinHttpQueryHeaders(
        HttpContext->RequestHandle,
        queryFlags,
        WINHTTP_HEADER_NAME_BY_INDEX,
        valueBuffer,
        &valueLength,
        WINHTTP_NO_HEADER_INDEX
        ))
    {
        if (valueLength != 0)
        {
            PH_STRINGREF string;
            ULONG64 integer;

            string.Buffer = valueBuffer;
            string.Length = valueLength;

            if (PhStringToUInt64(&string, 10, &integer))
            {
                *HeaderValue = integer;
                return STATUS_SUCCESS;
            }
            else
            {
                return STATUS_INTEGER_OVERFLOW;
            }
        }
        else
        {
            return STATUS_DATA_LATE_ERROR;
        }
    }

    return PhGetLastWinHttpErrorAsNtStatus();
}

//ULONG PhHttpQueryStatusCode(
//    _In_ PPH_HTTP_CONTEXT HttpContext
//    )
//{
//    ULONG headerValue = 0;
//    ULONG valueLength = sizeof(ULONG);
//
//    if (WinHttpQueryHeaders(
//        HttpContext->RequestHandle,
//        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
//        WINHTTP_HEADER_NAME_BY_INDEX,
//        &headerValue,
//        &valueLength,
//        WINHTTP_NO_HEADER_INDEX
//        ))
//    {
//        return headerValue;
//    }
//
//    return ULONG_MAX;
//}

ULONG PhHttpQueryStatusCode(
    _In_ PPH_HTTP_CONTEXT HttpContext
    )
{
    ULONG headerValue = 0;
    ULONG valueLength = sizeof(ULONG);

    if (WinHttpQueryHeaders(
        HttpContext->RequestHandle,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &headerValue,
        &valueLength,
        WINHTTP_NO_HEADER_INDEX
        ))
    {
        return headerValue;
    }

    return ULONG_MAX;
}

PVOID PhHttpQueryOption(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ BOOLEAN SessionOption,
    _In_ ULONG QueryOption
    )
{
    HINTERNET queryHandle;
    ULONG bufferLength = 0;
    PVOID optionBuffer;

    if (SessionOption)
        queryHandle = HttpContext->SessionHandle;
    else
        queryHandle = HttpContext->RequestHandle;

    if (!WinHttpQueryOption(
        queryHandle,
        QueryOption,
        NULL,
        &bufferLength
        ))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return NULL;
    }

    optionBuffer = PhAllocate(bufferLength);
    memset(optionBuffer, 0, bufferLength);

    if (!WinHttpQueryOption(
        queryHandle,
        QueryOption,
        optionBuffer,
        &bufferLength
        ))
    {
        PhFree(optionBuffer);
        return NULL;
    }

    return optionBuffer;
}

PPH_STRING PhHttpQueryOptionString(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ BOOLEAN SessionOption,
    _In_ ULONG QueryOption
    )
{
    PVOID optionBuffer;
    PPH_STRING stringBuffer = NULL;

    optionBuffer = PhHttpQueryOption(
        HttpContext,
        SessionOption,
        QueryOption
        );

    if (optionBuffer)
    {
        stringBuffer = PhCreateString(optionBuffer);
        PhFree(optionBuffer);
    }

    return stringBuffer;
}

// Upper bound for HTTP downloads and in-memory responses to avoid unbounded transfer sizes.
#define PH_HTTP_MAX_DOWNLOAD_LENGTH (2ULL * 1024 * 1024 * 1024)

NTSTATUS PhHttpReadDataToBuffer(
    _In_ PVOID RequestHandle,
    _In_opt_ ULONG TotalLength,
    _Out_ PVOID *Buffer,
    _Out_ ULONG *BufferLength
    )
{
    PSTR data;
    SIZE_T allocatedLength;
    SIZE_T dataLength;
    ULONG returnLength;
    NTSTATUS status;
    BYTE buffer[PAGE_SIZE];

    // Reject a declared length we would refuse to buffer anyway. (dmex)
    if (TotalLength > PH_HTTP_MAX_DOWNLOAD_LENGTH)
        return STATUS_BUFFER_OVERFLOW;

    if (TotalLength != 0)
    {
        allocatedLength = (SIZE_T)TotalLength + 1;
        data = (PSTR)PhAllocate(allocatedLength);
    }
    else
    {
        allocatedLength = sizeof(buffer);
        data = (PSTR)PhAllocate(allocatedLength);
    }

    dataLength = 0;

    while (TRUE)
    {
        SIZE_T requiredLength;

        if (!WinHttpReadData(RequestHandle, buffer, PAGE_SIZE, &returnLength))
        {
            status = PhGetLastWinHttpErrorAsNtStatus();
            PhFree(data);
            return status;
        }

        if (returnLength == 0)
            break;

        // Checked growth against a hard ceiling instead of unbounded doubling. (dmex)
        requiredLength = dataLength + returnLength;

        if (requiredLength > PH_HTTP_MAX_DOWNLOAD_LENGTH)
        {
            PhFree(data);
            return STATUS_BUFFER_OVERFLOW;
        }

        if (allocatedLength < requiredLength)
        {
            do
            {
                allocatedLength *= 2;
            } while (allocatedLength < requiredLength);

            if (allocatedLength > PH_HTTP_MAX_DOWNLOAD_LENGTH)
                allocatedLength = PH_HTTP_MAX_DOWNLOAD_LENGTH;

            data = PhReAllocate(data, allocatedLength);
        }

        memcpy(data + dataLength, buffer, returnLength);

        dataLength = requiredLength;
    }

    if (allocatedLength < dataLength + 1)
    {
        allocatedLength++;
        data = PhReAllocate(data, allocatedLength);
    }

    data[dataLength] = ANSI_NULL;

    if (dataLength)
    {
        if (Buffer)
            *Buffer = data;
        else
            PhFree(data);

        if (BufferLength)
            *BufferLength = (ULONG)dataLength; // bounded by PH_HTTP_MAX_DOWNLOAD_LENGTH (dmex)

        return STATUS_SUCCESS;
    }
    else
    {
        PhFree(data);

        return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS PhHttpDownloadString(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ BOOLEAN Unicode,
    _Out_ PVOID* StringBuffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferLength;

    status = PhHttpReadDataToBuffer(
        HttpContext->RequestHandle,
        0,
        &buffer,
        &bufferLength
        );

    if (NT_SUCCESS(status))
    {
        if (Unicode)
            *StringBuffer = PhConvertUtf8ToUtf16Ex(buffer, bufferLength);
        else
            *StringBuffer = PhCreateBytesEx(buffer, bufferLength);

        PhFree(buffer);
    }

    return status;
}

NTSTATUS PhHttpDownloadToFile(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PPH_STRINGREF FileName,
    _In_opt_ PPH_HTTPDOWNLOAD_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    return PhHttpDownloadToFileEx(HttpContext, FileName, PH_HTTP_MAX_DOWNLOAD_LENGTH, Callback, Context);
}

NTSTATUS PhHttpDownloadToFileEx(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PPH_STRINGREF FileName,
    _In_ ULONG64 MaximumLength,
    _In_opt_ PPH_HTTPDOWNLOAD_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    PH_HTTPDOWNLOAD_CONTEXT context = { 0 };
    NTSTATUS status;
    HANDLE fileHandle;
    LARGE_INTEGER fileSize;
    ULONG64 numberOfBytesTotal = 0;
    ULONG numberOfBytesRead = 0;
    ULONG64 numberOfBytesReadTotal = 0;
    ULONG64 timeTicks;
    LARGE_INTEGER timeNow;
    LARGE_INTEGER timeStart;
    PPH_STRING fileName;
    IO_STATUS_BLOCK isb;
    BYTE buffer[PAGE_SIZE];

    if (MaximumLength > MAXLONGLONG)
        return STATUS_INVALID_PARAMETER;

    PhQueryUnbiasedInterruptTime(&timeStart);

    // Content-Length is optional; servers using chunked transfer encoding omit it.
    // If absent, proceed without a pre-allocated file size hint.
    PhHttpQueryHeaderUlong64(
        HttpContext,
        PH_HTTP_QUERY_CONTENT_LENGTH,
        &numberOfBytesTotal
        );

    if (numberOfBytesTotal > MaximumLength)
        return STATUS_FILE_TOO_LARGE;

    fileName = PhGetTemporaryDirectoryRandomAlphaFileName();

    if (PhIsNullOrEmptyString(fileName))
        return STATUS_UNSUCCESSFUL;

    if (numberOfBytesTotal)
        fileSize.QuadPart = (LONGLONG)numberOfBytesTotal;

    status = PhCreateFileWin32Ex(
        &fileHandle,
        PhGetString(fileName),
        FILE_GENERIC_WRITE,
        numberOfBytesTotal ? &fileSize : NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        FILE_CREATE,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(fileName);
        return status;
    }

    memset(buffer, 0, sizeof(buffer));

    while (TRUE)
    {
        status = PhHttpReadData(
            HttpContext,
            buffer,
            PAGE_SIZE,
            &numberOfBytesRead
            );

        if (!NT_SUCCESS(status))
            break;
        if (numberOfBytesRead == 0)
            break;

        if (numberOfBytesRead > MaximumLength - numberOfBytesReadTotal)
        {
            status = STATUS_FILE_TOO_LARGE;
            break;
        }

        status = NtWriteFile(
            fileHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            buffer,
            numberOfBytesRead,
            NULL,
            NULL
            );

        if (!NT_SUCCESS(status))
            break;
        if (isb.Information == 0)
            break;

        if (numberOfBytesRead != isb.Information)
        {
            status = STATUS_UNSUCCESSFUL;
            break;
        }

        numberOfBytesReadTotal += isb.Information;

        if (Callback)
        {
            PhQueryUnbiasedInterruptTime(&timeNow);
            timeTicks = (ULONG64)(timeNow.QuadPart - timeStart.QuadPart);

            context.ReadLength = numberOfBytesReadTotal;
            context.TotalLength = numberOfBytesTotal;
            context.BitsPerSecond = timeTicks ? numberOfBytesReadTotal * PH_TICKS_PER_SEC / timeTicks : 0;
            context.Percent = numberOfBytesTotal ? (((DOUBLE)numberOfBytesReadTotal / (DOUBLE)numberOfBytesTotal) * 100) : 0;

            if (!Callback(&context, Context))
                break;
        }
    }

    if (NT_SUCCESS(status) && numberOfBytesTotal && numberOfBytesReadTotal != numberOfBytesTotal)
    {
        status = STATUS_UNSUCCESSFUL;
    }

    if (status != STATUS_SUCCESS)
    {
        PhSetFileDelete(fileHandle);
    }

    NtClose(fileHandle);

    if (NT_SUCCESS(status))
    {
        status = PhCreateDirectoryFullPathWin32(FileName);

        if (NT_SUCCESS(status))
        {
            status = PhMoveFileWin32(PhGetString(fileName), PhGetStringRefZ(FileName), FALSE);
        }
    }

    PhDereferenceObject(fileName);

    return status;
}

NTSTATUS PhHttpDownloadUrl(
    _In_ PPH_STRING Url,
    _In_opt_ PPH_HTTP_DOWNLOAD_OPTIONS Options
    )
{
    PH_HTTP_DOWNLOAD_OPTIONS defaultOptions = { 0 };
    PH_HTTPDOWNLOAD_CALLBACK_CONTEXT callbackContext = { 0 };
    NTSTATUS status;
    PPH_HTTP_CONTEXT httpContext = NULL;
    PPH_STRING hostPart = NULL;
    PPH_STRING pathPart = NULL;
    USHORT portPart = 0;
    ULONG statusCode = 0;
    ULONG64 numberOfBytesTotal = 0;
    ULONG numberOfBytesRead = 0;
    ULONG64 numberOfBytesReadTotal = 0;
    ULONG64 timeTicks;
    LARGE_INTEGER timeNow;
    LARGE_INTEGER timeStart;
    BYTE buffer[PAGE_SIZE];

    if (!Options)
        Options = &defaultOptions;

    if (!NT_SUCCESS(status = PhHttpCrackUrl(Url, &hostPart, &pathPart, &portPart)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpInitialize(&httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpSetEventCallback(httpContext, Options->EventCallback, Options->Context)))
        goto CleanupExit;

    if (Options->UserAgent)
    {
        PH_STRINGREF userAgent;

        PhInitializeStringRefLongHint(&userAgent, Options->UserAgent->Buffer);

        if (!NT_SUCCESS(status = PhpHttpSetOptionString(httpContext->SessionHandle, WINHTTP_OPTION_USER_AGENT, &userAgent)))
            goto CleanupExit;
    }

    if (Options->ProtocolFlags)
    {
        if (!NT_SUCCESS(status = PhHttpSetProtocol(
            httpContext,
            TRUE,
            Options->ProtocolFlags,
            Options->ProtocolTimeout
            )))
        {
            goto CleanupExit;
        }
    }

    if (!NT_SUCCESS(status = PhHttpConnect(httpContext, PhGetString(hostPart), portPart)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpBeginRequest(
        httpContext,
        Options->RequestMethod,
        PhGetString(pathPart),
        Options->RequestFlags
        )))
    {
        goto CleanupExit;
    }

    if (Options->EnableFeatures)
    {
        if (!NT_SUCCESS(status = PhHttpSetFeature(httpContext, Options->EnableFeatures, TRUE)))
            goto CleanupExit;
    }

    if (Options->DisableFeatures)
    {
        if (!NT_SUCCESS(status = PhHttpSetFeature(httpContext, Options->DisableFeatures, FALSE)))
            goto CleanupExit;
    }

    if (Options->SecurityFlags)
    {
        if (!NT_SUCCESS(status = PhHttpSetSecurity(httpContext, Options->SecurityFlags)))
            goto CleanupExit;
    }

    if (!NT_SUCCESS(status = PhHttpSendRequest(httpContext, PH_HTTP_NO_ADDITIONAL_HEADERS, 0, PH_HTTP_NO_REQUEST_DATA, 0, 0)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpReceiveResponse(httpContext)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = PhHttpQueryResponseStatus(httpContext)))
        goto CleanupExit;

    PhHttpQueryHeaderUlong(httpContext, PH_HTTP_QUERY_STATUS_CODE, &statusCode);
    PhHttpQueryHeaderUlong64(httpContext, PH_HTTP_QUERY_CONTENT_LENGTH, &numberOfBytesTotal);

    if (numberOfBytesTotal > PH_HTTP_MAX_DOWNLOAD_LENGTH)
    {
        status = STATUS_FILE_TOO_LARGE;
        goto CleanupExit;
    }

    callbackContext.StatusCode = statusCode;
    callbackContext.TotalLength = numberOfBytesTotal;

    if (Options->DownloadCallback)
    {
        if (!NT_SUCCESS(status = Options->DownloadCallback(PH_HTTPDOWNLOAD_EVENT_BEGIN, &callbackContext, Options->Context)))
            goto CleanupExit;
    }

    PhQueryUnbiasedInterruptTime(&timeStart);

    while (TRUE)
    {
        status = PhHttpReadData(httpContext, buffer, sizeof(buffer), &numberOfBytesRead);

        if (!NT_SUCCESS(status))
            goto CleanupExit;
        if (numberOfBytesRead == 0)
            break;

        if (numberOfBytesReadTotal + numberOfBytesRead > PH_HTTP_MAX_DOWNLOAD_LENGTH)
        {
            status = STATUS_FILE_TOO_LARGE;
            goto CleanupExit;
        }

        numberOfBytesReadTotal += numberOfBytesRead;

        PhQueryUnbiasedInterruptTime(&timeNow);
        timeTicks = (ULONG64)(timeNow.QuadPart - timeStart.QuadPart);

        callbackContext.Buffer = buffer;
        callbackContext.BufferLength = numberOfBytesRead;
        callbackContext.ReadLength = numberOfBytesReadTotal;
        callbackContext.TotalLength = numberOfBytesTotal;
        callbackContext.BitsPerSecond = timeTicks ? numberOfBytesReadTotal * PH_TICKS_PER_SEC / timeTicks : 0;
        callbackContext.Percent = numberOfBytesTotal ? (((DOUBLE)numberOfBytesReadTotal / (DOUBLE)numberOfBytesTotal) * 100) : 0;

        if (Options->DownloadCallback)
        {
            if (!NT_SUCCESS(status = Options->DownloadCallback(PH_HTTPDOWNLOAD_EVENT_DATA, &callbackContext, Options->Context)))
                goto CleanupExit;
            if (!NT_SUCCESS(status = Options->DownloadCallback(PH_HTTPDOWNLOAD_EVENT_PROGRESS, &callbackContext, Options->Context)))
                goto CleanupExit;
        }
    }

    callbackContext.Buffer = NULL;
    callbackContext.BufferLength = 0;
    callbackContext.ReadLength = numberOfBytesReadTotal;
    callbackContext.TotalLength = numberOfBytesTotal;

    if (Options->DownloadCallback)
    {
        if (!NT_SUCCESS(status = Options->DownloadCallback(PH_HTTPDOWNLOAD_EVENT_END, &callbackContext, Options->Context)))
            goto CleanupExit;
    }

    status = STATUS_SUCCESS;

CleanupExit:
    if (pathPart)
        PhDereferenceObject(pathPart);
    if (hostPart)
        PhDereferenceObject(hostPart);
    if (httpContext)
        PhHttpDestroy(httpContext);

    return status;
}

NTSTATUS PhHttpSetFeature(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ ULONG Feature,
    _In_ BOOLEAN Enable
    )
{
    ULONG featureValue = 0;

    PhMapFlags1(
        &featureValue,
        Feature,
        PhpHttpFeatureMappings,
        RTL_NUMBER_OF(PhpHttpFeatureMappings)
        );

    if (WinHttpSetOption(
        HttpContext->RequestHandle,
        Enable ? WINHTTP_OPTION_ENABLE_FEATURE : WINHTTP_OPTION_DISABLE_FEATURE,
        &featureValue,
        sizeof(ULONG)
        ))
    {
        return STATUS_SUCCESS;
    }

    return PhGetLastWinHttpErrorAsNtStatus();
}

NTSTATUS PhHttpSetOption(
    _In_ PVOID HttpHandle,
    _In_ ULONG Option,
    _In_ ULONG Value
    )
{
    ULONG optionValue = Value;

    if (WinHttpSetOption(
        HttpHandle,
        Option,
        &optionValue,
        sizeof(ULONG)
        ))
    {
        return STATUS_SUCCESS;
    }

    return PhGetLastWinHttpErrorAsNtStatus();
}

NTSTATUS PhHttpSetOptionString(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ ULONG Option,
    _In_ PPH_STRINGREF Value
    )
{
    return PhpHttpSetOptionString(HttpContext->RequestHandle, Option, Value);
}

NTSTATUS PhHttpSetCallback(
    _In_ PVOID HttpHandle,
    _In_ PPH_HTTP_STATUS_CALLBACK Callback,
    _In_ ULONG NotificationFlags
    )
{
    PPH_HTTP_STATUS_CALLBACK previousCallback;

    previousCallback = WinHttpSetStatusCallback(
        HttpHandle,
        Callback,
        NotificationFlags,
        0
        );

    if (previousCallback == WINHTTP_INVALID_STATUS_CALLBACK)
    {
        return PhGetLastWinHttpErrorAsNtStatus();
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhHttpGetContext(
    _In_ PVOID HttpHandle,
    _In_ PVOID Context
    )
{
    ULONG bufferLength = sizeof(ULONG_PTR);

    if (WinHttpQueryOption(HttpHandle, WINHTTP_OPTION_CONTEXT_VALUE, Context, &bufferLength))
        return STATUS_SUCCESS;

    return PhGetLastWinHttpErrorAsNtStatus();
}

NTSTATUS PhHttpSetContext(
    _In_ PVOID HttpHandle,
    _In_ PVOID Context
    )
{
    if (WinHttpSetOption(HttpHandle, WINHTTP_OPTION_CONTEXT_VALUE, Context, sizeof(ULONG_PTR)))
        return STATUS_SUCCESS;

    return PhGetLastWinHttpErrorAsNtStatus();
}

NTSTATUS PhHttpSetSecurity(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ ULONG Feature
    )
{
    ULONG featureValue = 0;

    PhMapFlags1(
        &featureValue,
        Feature,
        PhpHttpSecurityFlagsMappings,
        RTL_NUMBER_OF(PhpHttpSecurityFlagsMappings)
        );

    if (WinHttpSetOption(
        HttpContext->RequestHandle,
        WINHTTP_OPTION_SECURITY_FLAGS,
        &featureValue,
        sizeof(ULONG)
        ))
    {
        return STATUS_SUCCESS;
    }

    return PhGetLastWinHttpErrorAsNtStatus();
}

NTSTATUS PhHttpSetProtocol(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ BOOLEAN Session,
    _In_ ULONG Protocol,
    _In_ ULONG Timeout
    )
{
    NTSTATUS status;

    if (Session)
    {
        status = PhHttpSetOption(HttpContext->SessionHandle, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, Protocol);

        if (!NT_SUCCESS(status))
            return status;

        if (FlagOn(Protocol, PH_HTTP_PROTOCOL_FLAG_HTTP3))
        {
            status = PhHttpSetOption(HttpContext->SessionHandle, WINHTTP_OPTION_HTTP3_HANDSHAKE_TIMEOUT, Timeout);
        }
    }
    else
    {
        status = PhHttpSetOption(HttpContext->RequestHandle, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, Protocol);

        if (!NT_SUCCESS(status))
            return status;

        if (FlagOn(Protocol, PH_HTTP_PROTOCOL_FLAG_HTTP3))
        {
            status = PhHttpSetOption(HttpContext->RequestHandle, WINHTTP_OPTION_HTTP3_HANDSHAKE_TIMEOUT, Timeout);
        }
    }

    return status;
}

NTSTATUS PhHttpCrackUrl(
    _In_ PPH_STRING Url,
    _Out_ PPH_STRING *HostPart,
    _Out_ PPH_STRING *PathPart,
    _Out_ PUSHORT PortPart
    )
{
    URL_COMPONENTS httpParts;
    PPH_STRING hostPart = NULL;
    PPH_STRING pathPart = NULL;

    memset(&httpParts, 0, sizeof(URL_COMPONENTS));
    httpParts.dwStructSize = sizeof(URL_COMPONENTS);
    httpParts.dwHostNameLength = ULONG_MAX;
    httpParts.dwUrlPathLength = ULONG_MAX;

    if (!WinHttpCrackUrl(
        Url->Buffer,
        (ULONG)Url->Length / sizeof(WCHAR),
        0,
        &httpParts
        ))
    {
        return PhGetLastWinHttpErrorAsNtStatus();
    }

    if (httpParts.dwHostNameLength)
    {
        hostPart = PhCreateStringEx(httpParts.lpszHostName, httpParts.dwHostNameLength * sizeof(WCHAR));
    }

    if (httpParts.dwUrlPathLength)
    {
        pathPart = PhCreateStringEx(httpParts.lpszUrlPath, httpParts.dwUrlPathLength * sizeof(WCHAR));
    }

    if (hostPart && pathPart)
    {
        *HostPart = hostPart;
        *PathPart = pathPart;

        if (PortPart)
        {
            *PortPart = httpParts.nPort;
        }

        return STATUS_SUCCESS;
    }
    else
    {
        PhClearReference(&hostPart);
        PhClearReference(&pathPart);
        return STATUS_UNSUCCESSFUL;
    }
}

PPH_STRING PhHttpGetErrorMessage(
    _In_ ULONG ErrorCode
    )
{
    PVOID winhttpHandle;
    PPH_STRING message = NULL;

    if (!(winhttpHandle = PhGetLoaderEntryDllBaseZ(L"winhttp.dll")))
        return NULL;

    if (message = PhGetMessage(winhttpHandle, 0xb, PhGetUserDefaultLangID(), ErrorCode))
    {
        PhTrimToNullTerminatorString(message);
    }

    // Remove any trailing newline
    if (message && message->Length >= 2 * sizeof(WCHAR) &&
        message->Buffer[message->Length / sizeof(WCHAR) - 2] == L'\r' &&
        message->Buffer[message->Length / sizeof(WCHAR) - 1] == L'\n')
    {
        PhMoveReference(&message, PhCreateStringEx(message->Buffer, message->Length - 2 * sizeof(WCHAR)));
    }

    return message;
}

NTSTATUS PhHttpGetCertificateInfo(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _Out_ PPH_HTTP_CERTIFICATE_INFO Certificate
    )
{
    ULONG certificateInfoLength;
    WINHTTP_CERTIFICATE_INFO certificateInfoBuffer;

    certificateInfoLength = sizeof(WINHTTP_CERTIFICATE_INFO);
    RtlZeroMemory(&certificateInfoBuffer, sizeof(WINHTTP_CERTIFICATE_INFO));

    if (WinHttpQueryOption(
        HttpContext->RequestHandle,
        WINHTTP_OPTION_SECURITY_CERTIFICATE_STRUCT,
        &certificateInfoBuffer,
        &certificateInfoLength
        ))
    {
        RtlZeroMemory(Certificate, sizeof(PH_HTTP_CERTIFICATE_INFO));
        Certificate->Expiry.LowPart = certificateInfoBuffer.ftExpiry.dwLowDateTime;
        Certificate->Expiry.HighPart = certificateInfoBuffer.ftExpiry.dwHighDateTime;
        Certificate->Start.LowPart = certificateInfoBuffer.ftStart.dwLowDateTime;
        Certificate->Start.HighPart = certificateInfoBuffer.ftStart.dwHighDateTime;
        Certificate->SubjectInfo = certificateInfoBuffer.lpszSubjectInfo;
        Certificate->IssuerInfo = certificateInfoBuffer.lpszIssuerInfo;
        Certificate->ProtocolName = certificateInfoBuffer.lpszProtocolName;
        Certificate->SignatureAlgName = certificateInfoBuffer.lpszSignatureAlgName;
        Certificate->EncryptionAlgName = certificateInfoBuffer.lpszEncryptionAlgName;
        Certificate->KeySize = certificateInfoBuffer.dwKeySize;

        return STATUS_SUCCESS;
    }

    RtlZeroMemory(Certificate, sizeof(PH_HTTP_CERTIFICATE_INFO));
    return PhGetLastWinHttpErrorAsNtStatus();
}

NTSTATUS PhHttpGetTimingInfo(
    _In_ PPH_HTTP_CONTEXT HttpContext
    )
{
    ULONG requestTimeLength;
    WINHTTP_REQUEST_TIMES requestTimeBuffer;

    requestTimeLength = sizeof(WINHTTP_REQUEST_TIMES);
    memset(&requestTimeBuffer, 0, sizeof(WINHTTP_REQUEST_TIMES));
    requestTimeBuffer.cTimes = WinHttpRequestTimeMax;

    if (WinHttpQueryOption(
        HttpContext->RequestHandle,
        WINHTTP_OPTION_REQUEST_TIMES,
        &requestTimeBuffer,
        &requestTimeLength
        ))
    {
        for (ULONG i = 0; i < WinHttpRequestTimeMax; i++)
        {
            dprintf("%lu: %lu\n", i, requestTimeBuffer.rgullTimes[i]);
        }

        return STATUS_SUCCESS;
    }

    return PhGetLastWinHttpErrorAsNtStatus();
}

NTSTATUS PhHttpGetStatistics(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _Out_writes_bytes_to_opt_(*BufferLength, *BufferLength) PVOID Buffer,
    _Inout_ PULONG BufferLength
    )
{
    // WINHTTP_OPTION_CONNECTION_STATS_V0
    // WINHTTP_OPTION_CONNECTION_STATS_V1
    // WINHTTP_OPTION_CONNECTION_STATS_V2

    if (WinHttpQueryOption(
        HttpContext->RequestHandle,
        WINHTTP_OPTION_CONNECTION_STATS_V0,
        Buffer,
        BufferLength
        ))
    {
        return STATUS_SUCCESS;
    }

    return PhGetLastWinHttpErrorAsNtStatus();

    //ULONG statisticsInfoLength = sizeof(TCP_INFO_v0);
    //TCP_INFO_v0 statisticsInfo = { 0 };
    //dprintf("State: %d\n", statisticsInfo.State);
    //dprintf("Mss: %lu\n", statisticsInfo.Mss);
    //dprintf("ConnectionTimeMs: %llu\n", statisticsInfo.ConnectionTimeMs);
    //dprintf("TimestampsEnabled: %d\n", statisticsInfo.TimestampsEnabled);
    //dprintf("RttUs: %lu\n", statisticsInfo.RttUs);
    //dprintf("MinRttUs: %lu\n", statisticsInfo.MinRttUs);
    //dprintf("BytesInFlight: %lu\n", statisticsInfo.BytesInFlight);
    //dprintf("Cwnd: %lu\n", statisticsInfo.Cwnd);
    //dprintf("SndWnd: %lu\n", statisticsInfo.SndWnd);
    //dprintf("RcvWnd: %lu\n", statisticsInfo.RcvWnd);
    //dprintf("RcvBuf: %lu\n", statisticsInfo.RcvBuf);
    //dprintf("BytesOut: %llu\n", statisticsInfo.BytesOut);
    //dprintf("BytesIn: %llu\n", statisticsInfo.BytesIn);
    //dprintf("BytesReordered: %lu\n", statisticsInfo.BytesReordered);
    //dprintf("BytesRetrans: %lu\n", statisticsInfo.BytesRetrans);
    //dprintf("FastRetrans: %lu\n", statisticsInfo.FastRetrans);
    //dprintf("DupAcksIn: %lu\n", statisticsInfo.DupAcksIn);
    //dprintf("TimeoutEpisodes: %lu\n", statisticsInfo.TimeoutEpisodes);
    //dprintf("SynRetrans: %u\n", statisticsInfo.SynRetrans);
}

ULONG PhHttpGetExtendedStatusCode(
    _In_ PPH_HTTP_CONTEXT HttpContext
    )
{
    ULONG bufferLength = sizeof(ULONG);
    ULONG socketcode = ULONG_MAX;

    WinHttpQueryOption(
        HttpContext->RequestHandle,
        WINHTTP_OPTION_EXTENDED_ERROR,
        &socketcode,
        &bufferLength
        );

    return socketcode;
}

NTSTATUS PhHttpSetCredentials(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PCWSTR Name,
    _In_ PCWSTR Value
    )
{
    if (WinHttpSetCredentials(
        HttpContext->RequestHandle,
        WINHTTP_AUTH_TARGET_SERVER,
        WINHTTP_AUTH_SCHEME_BASIC,
        Name,
        Value,
        NULL
        ))
    {
        return STATUS_SUCCESS;
    }

    return PhGetLastWinHttpErrorAsNtStatus();
}

NTSTATUS PhHttpSetEventCallback(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PPH_HTTP_EVENT_CALLBACK EventCallback,
    _In_opt_ PVOID Context
    )
{
    HttpContext->Callback = EventCallback;
    HttpContext->Context = Context;

    //PhHttpSetContext(HttpContext->SessionHandle, HttpContext);
    //PhHttpSetCallback(HttpContext->SessionHandle, PhWinHttpStatusCallback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS);

    if (HttpContext->Callback)
    {
        return HttpContext->Callback(PHHTTP_EVENT_INITIALIZING, NULL, Context);
    }

    return STATUS_SUCCESS;
}

VOID CALLBACK PhWinHttpStatusCallback(
    _In_ HINTERNET InternetHandle,
    _In_ ULONG_PTR Context,
    _In_ ULONG InternetStatus,
    _In_opt_ PVOID StatusInformation,
    _In_ ULONG StatusInformationLength
    )
{
    PPH_HTTP_CONTEXT context = NULL;

    if (!Context)
    {
        PVOID httpContext;

        if (NT_SUCCESS(PhHttpGetContext(InternetHandle, &httpContext)))
        {
            context = (PPH_HTTP_CONTEXT)httpContext;
        }
    }
    else
    {
        context = (PPH_HTTP_CONTEXT)Context;
    }

    if (!context)
        return;

    switch (InternetStatus)
    {
    case WINHTTP_CALLBACK_STATUS_RESOLVING_NAME:
        {
            PH_STRINGREF string;

            string.Buffer = (PWSTR)StatusInformation;
            string.Length = (StatusInformationLength - 1) * sizeof(WCHAR);

            if (context->Callback)
            {
                context->Callback(PHHTTP_EVENT_RESOLVING_NAME, &string, context->Context);
            }
        }
        break;
    case WINHTTP_CALLBACK_STATUS_NAME_RESOLVED:
        {
            PH_STRINGREF string;

            string.Buffer = (PWSTR)StatusInformation;
            string.Length = (StatusInformationLength - 1) * sizeof(WCHAR);

            if (context->Callback)
            {
                context->Callback(PHHTTP_EVENT_NAME_RESOLVED, &string, context->Context);
            }
        }
        break;
    case WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER:
        {
            PH_STRINGREF string;

            string.Buffer = (PWSTR)StatusInformation;
            string.Length = (StatusInformationLength - 1) * sizeof(WCHAR);

            if (context->Callback)
            {
                context->Callback(PHHTTP_EVENT_CONNECTING, &string, context->Context);
            }
        }
        break;
    case WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER:
        {
            PH_STRINGREF string;

            string.Buffer = (PWSTR)StatusInformation;
            string.Length = (StatusInformationLength - 1) * sizeof(WCHAR);

            if (context->Callback)
            {
                context->Callback(InternetStatus, &string, context->Context);
            }
        }
        break;
    case WINHTTP_CALLBACK_STATUS_SENDING_REQUEST:
        {
            dprintf("WINHTTP_CALLBACK_STATUS_SENDING_REQUEST\n");
        }
        break;
    case WINHTTP_CALLBACK_STATUS_REQUEST_SENT:
        {
            ULONG numberOfBytesSent = *(PULONG)StatusInformation;
            dprintf("WINHTTP_CALLBACK_STATUS_REQUEST_SENT: %lu\n", numberOfBytesSent);
        }
        break;
    case WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE:
        {
            dprintf("WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE\n");
        }
        break;
    case WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED:
        {
            ULONG numberOfBytesReceived = *(PULONG)StatusInformation;
            dprintf("WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED: %lu\n", numberOfBytesReceived);
        }
        break;
    case WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION:
        {
            dprintf("WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION\n");
        }
        break;
    case WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED:
        {
            dprintf("WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED\n");
        }
        break;
    case WINHTTP_CALLBACK_STATUS_HANDLE_CREATED:
        {
            HINTERNET handle = (HINTERNET)*(PULONG_PTR)StatusInformation;
            ULONG option = 0;
            ULONG optionLength = sizeof(ULONG);

            dprintf("WINHTTP_CALLBACK_STATUS_HANDLE_CREATED: 0x%llx ", handle);

            if (WinHttpQueryOption(
                handle,
                WINHTTP_OPTION_HANDLE_TYPE,
                &option,
                &optionLength
                ))
            {
                switch (option)
                {
                case WINHTTP_HANDLE_TYPE_SESSION:
                    dprintf("[%s]\n", "Session handle");
                    break;
                case WINHTTP_HANDLE_TYPE_CONNECT:
                    dprintf("[%s]\n", "Connect handle");
                    break;
                case WINHTTP_HANDLE_TYPE_REQUEST:
                    dprintf("[%s]\n", "Request handle");
                    break;
                case WINHTTP_HANDLE_TYPE_PROXY_RESOLVER:
                    dprintf("[%s]\n", "Proxy handle");
                    break;
                case WINHTTP_HANDLE_TYPE_WEBSOCKET:
                    dprintf("[%s]\n", "Websocket handle");
                    break;
                case WINHTTP_HANDLE_TYPE_PROTOCOL:
                    dprintf("[%s]\n", "Protocol handle");
                    break;
                }
            }
        }
        break;
    case WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING:
        {
            HINTERNET handle = (HINTERNET)*(PULONG_PTR)StatusInformation;

            dprintf("WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING: 0x%llx\n", handle);
        }
        break;
    case WINHTTP_CALLBACK_STATUS_DETECTING_PROXY:
        {
            dprintf("WINHTTP_CALLBACK_STATUS_DETECTING_PROXY\n");
        }
        break;
    case WINHTTP_CALLBACK_STATUS_REDIRECT:
        {
            dprintf("WINHTTP_CALLBACK_STATUS_REDIRECT\n");
        }
        break;
    case WINHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE:
        {
            dprintf("WINHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE\n");
        }
        break;
    case WINHTTP_CALLBACK_STATUS_SECURE_FAILURE:
        {
            dprintf("WINHTTP_CALLBACK_STATUS_SECURE_FAILURE\n");
        }
        break;
    case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
        {
            dprintf("WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE\n");
        }
        break;
    case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
        {
            dprintf("WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE\n");
        }
        break;
    case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
        {
            dprintf("WINHTTP_CALLBACK_STATUS_READ_COMPLETE\n");
        }
        break;
    case WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE:
        {
            dprintf("WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE\n");
        }
        break;
    case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
        {
            dprintf("WINHTTP_CALLBACK_STATUS_REQUEST_ERROR\n");
        }
        break;
    case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
        {
            dprintf("WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE\n");
        }
        break;
    case WINHTTP_CALLBACK_STATUS_GETPROXYFORURL_COMPLETE:
        {
            dprintf("WINHTTP_CALLBACK_STATUS_GETPROXYFORURL_COMPLETE\n");
        }
        break;
    case WINHTTP_CALLBACK_STATUS_CLOSE_COMPLETE:
        {
            dprintf("WINHTTP_CALLBACK_STATUS_CLOSE_COMPLETE\n");
        }
        break;
    case WINHTTP_CALLBACK_STATUS_SHUTDOWN_COMPLETE:
        {
            dprintf("WINHTTP_CALLBACK_STATUS_SHUTDOWN_COMPLETE\n");
        }
        break;
    case WINHTTP_CALLBACK_STATUS_GETPROXYSETTINGS_COMPLETE:
        {
            dprintf("WINHTTP_CALLBACK_STATUS_GETPROXYSETTINGS_COMPLETE\n");
        }
        break;
    case WINHTTP_CALLBACK_STATUS_SETTINGS_WRITE_COMPLETE:
        {
            dprintf("WINHTTP_CALLBACK_STATUS_SETTINGS_WRITE_COMPLETE\n");
        }
        break;
    case WINHTTP_CALLBACK_STATUS_SETTINGS_READ_COMPLETE:
        {
            dprintf("WINHTTP_CALLBACK_STATUS_SETTINGS_READ_COMPLETE\n");
        }
        break;
    }
}

NTSTATUS PhHttpErrorToNtStatus(
    _In_ ULONG WinhttpError
    )
{
    switch (WinhttpError)
    {
    case ERROR_WINHTTP_OUT_OF_HANDLES: return STATUS_NO_MEMORY;
    case ERROR_WINHTTP_TIMEOUT: return STATUS_TIMEOUT;
    case ERROR_WINHTTP_INTERNAL_ERROR: return STATUS_INTERNAL_ERROR;
    case ERROR_WINHTTP_INVALID_URL: return STATUS_OBJECT_PATH_INVALID;
    case ERROR_WINHTTP_UNRECOGNIZED_SCHEME: return STATUS_OBJECT_NAME_INVALID;
    case ERROR_WINHTTP_NAME_NOT_RESOLVED: return STATUS_OBJECT_NAME_NOT_FOUND;
    case ERROR_WINHTTP_INVALID_OPTION:
    case ERROR_WINHTTP_OPTION_NOT_SETTABLE: return STATUS_INVALID_DEVICE_REQUEST;
    case ERROR_WINHTTP_SHUTDOWN:  return STATUS_SYSTEM_SHUTDOWN;
    case ERROR_WINHTTP_LOGIN_FAILURE: return STATUS_LOGON_FAILURE;
    case ERROR_WINHTTP_OPERATION_CANCELLED: return STATUS_CANCELLED;
    case ERROR_WINHTTP_INCORRECT_HANDLE_TYPE:
    case ERROR_WINHTTP_INCORRECT_HANDLE_STATE: return STATUS_INVALID_HANDLE;
    case ERROR_WINHTTP_CANNOT_CONNECT: return STATUS_CONNECTION_REFUSED;
    case ERROR_WINHTTP_CONNECTION_ERROR: return STATUS_CONNECTION_ABORTED;
    case ERROR_WINHTTP_RESEND_REQUEST: return STATUS_RETRY;
    case ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED: return STATUS_PKINIT_FAILURE;
    case ERROR_WINHTTP_CANNOT_CALL_BEFORE_OPEN:
    case ERROR_WINHTTP_CANNOT_CALL_BEFORE_SEND:
    case ERROR_WINHTTP_CANNOT_CALL_AFTER_SEND:
    case ERROR_WINHTTP_CANNOT_CALL_AFTER_OPEN: return STATUS_INVALID_DEVICE_STATE;
    case ERROR_WINHTTP_HEADER_NOT_FOUND: return STATUS_OBJECT_NAME_NOT_FOUND;
    case ERROR_WINHTTP_INVALID_SERVER_RESPONSE: return STATUS_INVALID_NETWORK_RESPONSE;
    case ERROR_WINHTTP_INVALID_HEADER:
    case ERROR_WINHTTP_INVALID_QUERY_REQUEST: return STATUS_INVALID_PARAMETER;
    case ERROR_WINHTTP_HEADER_ALREADY_EXISTS: return STATUS_OBJECT_NAME_COLLISION;
    case ERROR_WINHTTP_REDIRECT_FAILED: return STATUS_UNSUCCESSFUL;
    case ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR: return STATUS_LOGON_FAILURE;
    case ERROR_WINHTTP_BAD_AUTO_PROXY_SCRIPT:
    case ERROR_WINHTTP_UNABLE_TO_DOWNLOAD_SCRIPT:
    case ERROR_WINHTTP_UNHANDLED_SCRIPT_TYPE:
    case ERROR_WINHTTP_SCRIPT_EXECUTION_ERROR: return STATUS_INVALID_PARAMETER;
    case ERROR_WINHTTP_NOT_INITIALIZED: return STATUS_INVALID_DEVICE_STATE;
    case ERROR_WINHTTP_SECURE_FAILURE: return STATUS_ENCRYPTION_FAILED;
    case ERROR_WINHTTP_SECURE_CERT_DATE_INVALID:
    case ERROR_WINHTTP_SECURE_CERT_CN_INVALID:
    case ERROR_WINHTTP_SECURE_INVALID_CA:
    case ERROR_WINHTTP_SECURE_CERT_REV_FAILED:
    case ERROR_WINHTTP_SECURE_CHANNEL_ERROR:
    case ERROR_WINHTTP_SECURE_INVALID_CERT:
    case ERROR_WINHTTP_SECURE_CERT_REVOKED:
    case ERROR_WINHTTP_SECURE_CERT_WRONG_USAGE:
    case ERROR_WINHTTP_AUTODETECTION_FAILED: return STATUS_NO_SECURITY_CONTEXT;
    case ERROR_WINHTTP_HEADER_COUNT_EXCEEDED:
    case ERROR_WINHTTP_HEADER_SIZE_OVERFLOW:
    case ERROR_WINHTTP_CHUNKED_ENCODING_HEADER_SIZE_OVERFLOW:
    case ERROR_WINHTTP_RESPONSE_DRAIN_OVERFLOW: return STATUS_BUFFER_OVERFLOW;
    case ERROR_WINHTTP_CLIENT_CERT_NO_PRIVATE_KEY:
    case ERROR_WINHTTP_CLIENT_CERT_NO_ACCESS_PRIVATE_KEY: return STATUS_INVALID_PARAMETER;
    case ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED_PROXY: return STATUS_NO_SECURITY_CONTEXT;
    case ERROR_WINHTTP_SECURE_FAILURE_PROXY:
    case ERROR_WINHTTP_RESERVED_189:
    case ERROR_WINHTTP_HTTP_PROTOCOL_MISMATCH:
    case ERROR_WINHTTP_GLOBAL_CALLBACK_FAILED: return STATUS_INVALID_PARAMETER;
    case ERROR_WINHTTP_FEATURE_DISABLED:
    case ERROR_WINHTTP_FAST_FORWARDING_NOT_SUPPORTED: return STATUS_NOT_SUPPORTED;
    }

    if (
        WinhttpError < WINHTTP_ERROR_BASE ||
        WinhttpError > ERROR_WINHTTP_FAST_FORWARDING_NOT_SUPPORTED
        )
    {
        return PhDosErrorToNtStatus(WinhttpError);
    }

    return STATUS_UNSUCCESSFUL;
}

//
// IPv4 and IPv6
//

static PWSTR PhIpAddressWriteUInt32(
    _Out_ PWSTR Buffer,
    _In_ ULONG Value,
    _In_ ULONG Base
    )
{
    WCHAR digits[16];
    ULONG count = 0;

    do
    {
        digits[count++] = L"0123456789abcdef"[Value % Base];
        Value /= Base;
    } while (Value);

    while (count)
    {
        *Buffer++ = digits[--count];
    }

    return Buffer;
}

static BOOLEAN PhpParseIpComponent(
    _Inout_ PCWSTR* AddressString,
    _In_ ULONG Base,
    _Out_ PULONG Value
    )
{
    PCWSTR string = *AddressString;
    ULONG value = 0;
    ULONG digit;
    BOOLEAN found = FALSE;

    while (*string)
    {
        if (*string >= L'0' && *string <= L'9')
            digit = *string - L'0';
        else if (*string >= L'a' && *string <= L'f')
            digit = *string - L'a' + 10;
        else if (*string >= L'A' && *string <= L'F')
            digit = *string - L'A' + 10;
        else
            break;

        if (digit >= Base || value > (MAXULONG - digit) / Base)
            return FALSE;

        value = value * Base + digit;
        found = TRUE;
        string++;
    }

    if (!found)
        return FALSE;

    *AddressString = string;
    *Value = value;
    return TRUE;
}

static NTSTATUS PhpIpv4StringToAddress(
    _In_ PCWSTR AddressString,
    _In_ BOOLEAN Strict,
    _Out_opt_ PCWSTR* Terminator,
    _Out_ PIN_ADDR Address,
    _Out_opt_ PUSHORT Port
    )
{
    PCWSTR string = AddressString;
    ULONG fields[4];
    ULONG count = 0;
    ULONG base;
    ULONG value;
    IN_ADDR address;

    if (!AddressString || !Address)
        return STATUS_INVALID_PARAMETER;

    for (;;)
    {
        base = 10;

        if (*string == L'0')
        {
            if (string[1] == L'x' || string[1] == L'X')
            {
                if (Strict)
                    goto InvalidAddress;

                base = 16;
                string += 2;
            }
            else if (string[1] >= L'0' && string[1] <= L'9')
            {
                if (Strict)
                    goto InvalidAddress;

                base = 8;
                string++;
            }
        }

        if (!PhpParseIpComponent(&string, base, &fields[count]))
            goto InvalidAddress;

        count++;

        if (*string != L'.')
            break;
        if (count == RTL_NUMBER_OF(fields))
            goto InvalidAddress;

        string++;
    }

    if (Strict && count != 4)
        goto InvalidAddress;

    switch (count)
    {
    case 1:
        value = fields[0];
        address.S_un.S_un_b.s_b1 = (UCHAR)(value >> 24);
        address.S_un.S_un_b.s_b2 = (UCHAR)(value >> 16);
        address.S_un.S_un_b.s_b3 = (UCHAR)(value >> 8);
        address.S_un.S_un_b.s_b4 = (UCHAR)value;
        break;
    case 2:
        if (fields[0] > MAXUCHAR || fields[1] > 0xffffff)
            goto InvalidAddress;
        address.S_un.S_un_b.s_b1 = (UCHAR)fields[0];
        address.S_un.S_un_b.s_b2 = (UCHAR)(fields[1] >> 16);
        address.S_un.S_un_b.s_b3 = (UCHAR)(fields[1] >> 8);
        address.S_un.S_un_b.s_b4 = (UCHAR)fields[1];
        break;
    case 3:
        if (fields[0] > MAXUCHAR || fields[1] > MAXUCHAR || fields[2] > MAXUSHORT)
            goto InvalidAddress;
        address.S_un.S_un_b.s_b1 = (UCHAR)fields[0];
        address.S_un.S_un_b.s_b2 = (UCHAR)fields[1];
        address.S_un.S_un_b.s_b3 = (UCHAR)(fields[2] >> 8);
        address.S_un.S_un_b.s_b4 = (UCHAR)fields[2];
        break;
    case 4:
        if (fields[0] > MAXUCHAR || fields[1] > MAXUCHAR || fields[2] > MAXUCHAR || fields[3] > MAXUCHAR)
            goto InvalidAddress;
        address.S_un.S_un_b.s_b1 = (UCHAR)fields[0];
        address.S_un.S_un_b.s_b2 = (UCHAR)fields[1];
        address.S_un.S_un_b.s_b3 = (UCHAR)fields[2];
        address.S_un.S_un_b.s_b4 = (UCHAR)fields[3];
        break;
    default:
        goto InvalidAddress;
    }

    if (Terminator)
        *Terminator = string;

    if (Port)
    {
        *Port = 0;

        if (*string == L':')
        {
            string++;

            if (!PhpParseIpComponent(&string, 10, &value) || value == 0 || value > MAXUSHORT || *string)
                goto InvalidAddress;

            *Port = RtlUshortByteSwap((USHORT)value);
        }
        else if (*string)
        {
            goto InvalidAddress;
        }
    }

    *Address = address;
    return STATUS_SUCCESS;

InvalidAddress:
    if (Terminator)
        *Terminator = string;
    return STATUS_INVALID_PARAMETER;
}

static PWSTR PhpIpv4AddressToString(
    _In_ PCIN_ADDR Address,
    _Out_writes_(16) PWSTR AddressString
    )
{
    PWSTR string = AddressString;

    string = PhIpAddressWriteUInt32(string, Address->S_un.S_un_b.s_b1, 10); *string++ = L'.';
    string = PhIpAddressWriteUInt32(string, Address->S_un.S_un_b.s_b2, 10); *string++ = L'.';
    string = PhIpAddressWriteUInt32(string, Address->S_un.S_un_b.s_b3, 10); *string++ = L'.';
    string = PhIpAddressWriteUInt32(string, Address->S_un.S_un_b.s_b4, 10); *string = UNICODE_NULL;

    return string;
}

NTSTATUS PhIpv4AddressToStringEx(
    _In_ PCIN_ADDR Address,
    _In_ USHORT Port,
    _Out_writes_to_(*AddressStringLength, *AddressStringLength) PWSTR AddressString,
    _Inout_ PULONG AddressStringLength
    )
{
#ifndef PH_NATIVE_IPADDRESS_STRING
    WCHAR buffer[22];
    PWSTR string;
    ULONG length;

    if (!Address || !AddressString || !AddressStringLength)
        return STATUS_INVALID_PARAMETER;

    string = PhpIpv4AddressToString(Address, buffer);

    if (Port)
    {
        *string++ = L':';
        string = PhIpAddressWriteUInt32(string, RtlUshortByteSwap(Port), 10);
        *string = UNICODE_NULL;
    }

    length = (ULONG)(string - buffer) + 1;

    if (*AddressStringLength < length)
    {
        *AddressStringLength = length;
        return STATUS_INVALID_PARAMETER;
    }

    memcpy(AddressString, buffer, length * sizeof(WCHAR));
    *AddressStringLength = length;
    return STATUS_SUCCESS;
#else
    return RtlIpv4AddressToStringEx(Address, Port, AddressString, AddressStringLength);
#endif
}

static PWSTR PhpIpv6AddressToString(
    _In_ PCIN6_ADDR Address,
    _Out_writes_(46) PWSTR AddressString
    )
{
    USHORT words[8];
    ULONG index;
    ULONG bestStart = 8;
    ULONG bestLength = 0;
    ULONG currentStart;
    ULONG currentLength;
    ULONG ipv6End = 8;
    BOOLEAN embeddedIpv4;
    PWSTR string = AddressString;

    for (index = 0; index < RTL_NUMBER_OF(words); index++)
        words[index] = RtlUshortByteSwap(Address->u.Word[index]);

    embeddedIpv4 =
        (words[5] == 0x5efe && !(words[4] & ~0x200)) ||
        (!words[0] && !words[1] && !words[2] && !words[3] &&
            (!words[4] || words[4] == 0xffff) &&
            ((!words[4] && (!words[5] || words[5] == 0xffff)) || (words[4] == 0xffff && !words[5])) &&
            words[6]);

    if (embeddedIpv4)
        ipv6End = 6;

    for (index = 0; index < ipv6End;)
    {
        if (words[index])
        {
            index++;
            continue;
        }

        currentStart = index;
        while (index < ipv6End && !words[index])
            index++;
        currentLength = index - currentStart;

        if (currentLength > bestLength)
        {
            bestStart = currentStart;
            bestLength = currentLength;
        }
    }

    if (bestLength < 2)
        bestStart = ipv6End;

    for (index = 0; index < ipv6End; index++)
    {
        if (index == bestStart)
        {
            *string++ = L':';
            *string++ = L':';
            index += bestLength - 1;
            continue;
        }

        if (index && index != bestStart + bestLength)
            *string++ = L':';

        string = PhIpAddressWriteUInt32(string, words[index], 16);
    }

    if (embeddedIpv4)
    {
        if (string != AddressString && string[-1] != L':')
            *string++ = L':';
        string = PhpIpv4AddressToString((PCIN_ADDR)&Address->u.Byte[12], string);
    }

    *string = UNICODE_NULL;
    return string;
}

NTSTATUS PhIpv6AddressToStringEx(
    _In_ PCIN6_ADDR Address,
    _In_ ULONG ScopeId,
    _In_ USHORT Port,
    _Out_writes_to_(*AddressStringLength, *AddressStringLength) PWSTR AddressString,
    _Inout_ PULONG AddressStringLength
    )
{
#ifndef PH_NATIVE_IPADDRESS_STRING
    WCHAR buffer[65];
    PWSTR string = buffer;
    ULONG length;

    if (!Address || !AddressString || !AddressStringLength)
        return STATUS_INVALID_PARAMETER;

    if (Port)
        *string++ = L'[';

    string = PhpIpv6AddressToString(Address, string);

    if (ScopeId)
    {
        *string++ = L'%';
        string = PhIpAddressWriteUInt32(string, ScopeId, 10);
    }

    if (Port)
    {
        *string++ = L']';
        *string++ = L':';
        string = PhIpAddressWriteUInt32(string, RtlUshortByteSwap(Port), 10);
    }

    *string = UNICODE_NULL;
    length = (ULONG)(string - buffer) + 1;

    if (*AddressStringLength < length)
    {
        *AddressStringLength = length;
        return STATUS_INVALID_PARAMETER;
    }

    memcpy(AddressString, buffer, length * sizeof(WCHAR));
    *AddressStringLength = length;
    return STATUS_SUCCESS;
#else
    return RtlIpv6AddressToStringEx(Address, ScopeId, Port, AddressString, AddressStringLength);
#endif
}

static NTSTATUS PhpIpv4StringToAddressBasic(
    _In_ PCWSTR AddressString,
    _In_ BOOLEAN Strict,
    _Out_ PCWSTR* Terminator,
    _Out_ PIN_ADDR Address
    )
{
    return PhpIpv4StringToAddress(AddressString, Strict, Terminator, Address, NULL);
}

NTSTATUS PhIpv4StringToAddressEx(
    _In_ PCWSTR AddressString,
    _In_ BOOLEAN Strict,
    _Out_ PIN_ADDR Address,
    _Out_ PUSHORT Port
    )
{
#ifndef PH_NATIVE_IPADDRESS_STRING
    if (!Port)
        return STATUS_INVALID_PARAMETER;

    return PhpIpv4StringToAddress(AddressString, Strict, NULL, Address, Port);
#else
    return RtlIpv4StringToAddressEx(AddressString, Strict, Address, Port);
#endif
}

static NTSTATUS PhpIpv6StringToAddress(
    _In_ PCWSTR AddressString,
    _In_ BOOLEAN Extended,
    _Out_opt_ PCWSTR* Terminator,
    _Out_ PIN6_ADDR Address,
    _Out_opt_ PULONG ScopeId,
    _Out_opt_ PUSHORT Port
    )
{
    PCWSTR string = AddressString;
    IN6_ADDR address = { 0 };
    UCHAR bytes[16];
    ULONG byteCount = 0;
    LONG gap = -1;
    ULONG value;
    ULONG digits;
    PCWSTR component;
    ULONG scope = 0;
    USHORT port = 0;
    BOOLEAN bracketed = FALSE;

    if (!AddressString || !Address)
        return STATUS_INVALID_PARAMETER;

    if (*string == L'[')
    {
        if (!Extended)
            goto InvalidAddress;
        bracketed = TRUE;
        string++;
    }

    if (*string == L':')
    {
        if (string[1] != L':')
            goto InvalidAddress;
        gap = 0;
        string += 2;
    }

    while (*string && *string != L'%' && *string != L']')
    {
        component = string;
        digits = 0;
        value = 0;

        while (((*string >= L'0' && *string <= L'9') || (*string >= L'a' && *string <= L'f') || (*string >= L'A' && *string <= L'F')) && digits < 8)
        {
            ULONG digit = *string <= L'9' ? *string - L'0' : ((*string | 0x20) - L'a' + 10);
            value = value * 16 + digit;
            string++;
            digits++;
        }

        if (*string == L'.')
        {
            IN_ADDR address4;
            PCWSTR terminator4;

            if (byteCount > 12 || !NT_SUCCESS(PhpIpv4StringToAddress(component, TRUE, &terminator4, &address4, NULL)))
                goto InvalidAddress;

            memcpy(bytes + byteCount, &address4, sizeof(address4));
            byteCount += sizeof(address4);
            string = terminator4;
            break;
        }

        if (!digits || digits > 4 || value > MAXUSHORT || byteCount > 14)
            goto InvalidAddress;

        bytes[byteCount++] = (UCHAR)(value >> 8);
        bytes[byteCount++] = (UCHAR)value;

        if (*string != L':')
            break;

        string++;
        if (*string == L':')
        {
            if (gap != -1)
                goto InvalidAddress;
            gap = (LONG)byteCount;
            string++;
            if (!*string || *string == L'%' || *string == L']')
                break;
        }
    }

    if (gap >= 0)
    {
        ULONG tailLength;

        if (byteCount >= sizeof(bytes))
            goto InvalidAddress;

        tailLength = byteCount - gap;
        memmove(bytes + sizeof(bytes) - tailLength, bytes + gap, tailLength);
        memset(bytes + gap, 0, sizeof(bytes) - byteCount);
        byteCount = sizeof(bytes);
    }

    if (byteCount != sizeof(bytes))
        goto InvalidAddress;

    if (Extended && *string == L'%')
    {
        string++;
        if (!PhpParseIpComponent(&string, 10, &scope))
            goto InvalidAddress;
    }

    if (bracketed)
    {
        if (*string++ != L']')
            goto InvalidAddress;

        if (*string == L':')
        {
            string++;
            if (!PhpParseIpComponent(&string, 10, &value) || value > MAXUSHORT)
                goto InvalidAddress;
            port = RtlUshortByteSwap((USHORT)value);
        }
    }

    if (Extended && *string)
        goto InvalidAddress;

    memcpy(&address, bytes, sizeof(address));
    *Address = address;
    if (ScopeId) *ScopeId = scope;
    if (Port) *Port = port;
    if (Terminator) *Terminator = string;
    return STATUS_SUCCESS;

InvalidAddress:
    if (Terminator) *Terminator = string;
    return STATUS_INVALID_PARAMETER;
}

static NTSTATUS PhpIpv6StringToAddressBasic(
    _In_ PCWSTR AddressString,
    _Out_ PCWSTR* Terminator,
    _Out_ PIN6_ADDR Address
    )
{
    return PhpIpv6StringToAddress(AddressString, FALSE, Terminator, Address, NULL, NULL);
}

NTSTATUS PhIpv6StringToAddressEx(
    _In_ PCWSTR AddressString,
    _Out_ PIN6_ADDR Address,
    _Out_ PULONG ScopeId,
    _Out_ PUSHORT Port
    )
{
#ifndef PH_NATIVE_IPADDRESS_STRING
    if (!ScopeId || !Port)
        return STATUS_INVALID_PARAMETER;

    return PhpIpv6StringToAddress(AddressString, TRUE, NULL, Address, ScopeId, Port);
#else
    return RtlIpv6StringToAddressEx(AddressString, Address, ScopeId, Port);
#endif
}

NTSTATUS PhIpv4AddressToString(
    _In_ PCIN_ADDR Address,
    _In_ USHORT Port,
    _Out_writes_to_(*AddressStringLength, *AddressStringLength) PWSTR AddressString,
    _Inout_ PULONG AddressStringLength
    )
{
#ifndef PH_NATIVE_IPADDRESS_STRING
    return PhIpv4AddressToStringEx(Address, Port, AddressString, AddressStringLength);
#else
    return RtlIpv4AddressToStringEx(Address, Port, AddressString, AddressStringLength);
#endif
}

NTSTATUS PhIpv4StringToAddress(
    _In_ PCWSTR AddressString,
    _In_ BOOLEAN Strict,
    _Out_ PIN_ADDR Address,
    _Out_ PUSHORT Port
    )
{
#ifndef PH_NATIVE_IPADDRESS_STRING
    return PhIpv4StringToAddressEx(AddressString, Strict, Address, Port);
#else
    return RtlIpv4StringToAddressEx(AddressString, Strict, Address, Port);
#endif
}

NTSTATUS PhIpv6AddressToString(
    _In_ PCIN6_ADDR Address,
    _In_ ULONG ScopeId,
    _In_ USHORT Port,
    _Out_writes_to_(*AddressStringLength, *AddressStringLength) PWSTR AddressString,
    _Inout_ PULONG AddressStringLength
    )
{
#ifndef PH_NATIVE_IPADDRESS_STRING
    return PhIpv6AddressToStringEx(Address, ScopeId, Port, AddressString, AddressStringLength);
#else
    return RtlIpv6AddressToStringEx(Address, ScopeId, Port, AddressString, AddressStringLength);
#endif
}

NTSTATUS PhIpv6StringToAddress(
    _In_ PCWSTR AddressString,
    _Out_ PIN6_ADDR Address,
    _Out_ PULONG ScopeId,
    _Out_ PUSHORT Port
    )
{
#ifndef PH_NATIVE_IPADDRESS_STRING
    return PhIpv6StringToAddressEx(AddressString, Address, ScopeId, Port);
#else
    return RtlIpv6StringToAddressEx(AddressString, Address, ScopeId, Port);
#endif
}

//
// DOH
//

HINTERNET PhCreateDohConnectionHandle(
    _In_opt_ PCWSTR DnsServerAddress
    )
{
    static HINTERNET httpSessionHandle = NULL;
    static HINTERNET httpConnectionHandle = NULL;
    static PH_INITONCE initOnce = PH_INITONCE_INIT;

    if (PhBeginInitOnce(&initOnce))
    {
        NTSTATUS status;

        status = PhWinHttpOpen(&httpSessionHandle);

        if (NT_SUCCESS(status))
        {
            if (WindowsVersion < WINDOWS_8_1)
            {
                PhHttpSetOption(httpSessionHandle, WINHTTP_OPTION_SECURE_PROTOCOLS, WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2);
            }
            else
            {
                PhHttpSetOption(httpSessionHandle, WINHTTP_OPTION_SECURE_PROTOCOLS, WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3);
                PhHttpSetOption(httpSessionHandle, WINHTTP_OPTION_DECOMPRESSION, WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE);

                if (WindowsVersion >= WINDOWS_10)
                {
                    PhHttpSetOption(httpSessionHandle, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, WINHTTP_PROTOCOL_FLAG_HTTP2);
                    //PhHttpSetOption(sessionHandle, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, WINHTTP_PROTOCOL_FLAG_HTTP2 | WINHTTP_PROTOCOL_FLAG_HTTP3);
                }

                if (WindowsVersion >= WINDOWS_10_RS5)
                {
                    PhHttpSetOption(httpSessionHandle, WINHTTP_OPTION_IPV6_FAST_FALLBACK, TRUE);
                    PhHttpSetOption(httpSessionHandle, WINHTTP_OPTION_DISABLE_STREAM_QUEUE, TRUE);
                }

                if (WindowsVersion >= WINDOWS_11)
                {
                    PhHttpSetOption(httpSessionHandle, WINHTTP_OPTION_DISABLE_GLOBAL_POOLING, TRUE);
                    PhHttpSetOption(httpSessionHandle, WINHTTP_OPTION_TLS_FALSE_START, TRUE);
                    PhHttpSetOption(httpSessionHandle, WINHTTP_OPTION_TCP_FAST_OPEN, TRUE);
                }

                if (WindowsVersion >= WINDOWS_11_24H2)
                {
                    PhHttpSetOption(httpSessionHandle, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, WINHTTP_PROTOCOL_FLAG_HTTP2 | WINHTTP_PROTOCOL_FLAG_HTTP3);
                    PhHttpSetOption(httpSessionHandle, WINHTTP_OPTION_HTTP3_HANDSHAKE_TIMEOUT, 5000); // 5 second timeout before reverting to HTTP2
                }
            }

            PhHttpSetOption(httpSessionHandle, WINHTTP_OPTION_MAX_CONNS_PER_SERVER, 1);

            if (WindowsVersion >= WINDOWS_10)
            {
                httpConnectionHandle = WinHttpConnect(
                    httpSessionHandle,
                    DnsServerAddress ? DnsServerAddress : L"1.1.1.1",
                    INTERNET_DEFAULT_HTTPS_PORT,
                    0
                    );
            }
        }

        PhEndInitOnce(&initOnce);
    }

    if (WindowsVersion < WINDOWS_10)
    {
        httpConnectionHandle = WinHttpConnect(
            httpSessionHandle,
            DnsServerAddress ? DnsServerAddress : L"1.1.1.1",
            INTERNET_DEFAULT_HTTPS_PORT,
            0
            );
    }

    return httpConnectionHandle;
}

HINTERNET PhCreateDohRequestHandle(
    _In_ HINTERNET HttpConnectionHandle
    )
{
    static PCWSTR httpAcceptTypes[2] = { L"application/dns-message", NULL };
    HINTERNET httpRequestHandle;
    ULONG httpOptions;

    if (!(httpRequestHandle = WinHttpOpenRequest(
        HttpConnectionHandle,
        L"POST",
        L"/dns-query",
        NULL,
        WINHTTP_NO_REFERER,
        httpAcceptTypes,
        WINHTTP_FLAG_SECURE
        )))
    {
        return NULL;
    }

    WinHttpAddRequestHeaders(
        httpRequestHandle,
        L"Content-Type: application/dns-message",
        ULONG_MAX,
        WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE
        );

    if (WindowsVersion <= WINDOWS_8)
    {
        // Winhttp on Windows 7 doesn't correctly validate the certificate CN for connections using an IP address. (dmex)
        httpOptions = SECURITY_FLAG_IGNORE_CERT_CN_INVALID;

        WinHttpSetOption(
            httpRequestHandle,
            WINHTTP_OPTION_SECURITY_FLAGS,
            &httpOptions,
            sizeof(ULONG)
            );
    }

    return httpRequestHandle;
}

PPH_STRING PhDnsReverseLookupNameFromAddress(
    _In_ ULONG Type,
    _In_ PVOID Address
    )
{
#define IP4_REVERSE_DOMAIN_STRING_LENGTH (IP4_ADDRESS_STRING_LENGTH + sizeof(DNS_IP4_REVERSE_DOMAIN_STRING_W) + 1)
#define IP6_REVERSE_DOMAIN_STRING_LENGTH (IP6_ADDRESS_STRING_LENGTH + sizeof(DNS_IP6_REVERSE_DOMAIN_STRING_W) + 1)

    switch (Type)
    {
    case PH_NETWORK_TYPE_IPV4:
        {
            static CONST PH_STRINGREF reverseLookupDomainName = PH_STRINGREF_INIT(DNS_IP4_REVERSE_DOMAIN_STRING);
            PIN_ADDR inAddrV4 = Address;
            PH_FORMAT format[9];
            SIZE_T returnLength;
            WCHAR reverseNameBuffer[IP4_REVERSE_DOMAIN_STRING_LENGTH];

            PhInitFormatU(&format[0], inAddrV4->s_impno);
            PhInitFormatC(&format[1], L'.');
            PhInitFormatU(&format[2], inAddrV4->s_lh);
            PhInitFormatC(&format[3], L'.');
            PhInitFormatU(&format[4], inAddrV4->s_host);
            PhInitFormatC(&format[5], L'.');
            PhInitFormatU(&format[6], inAddrV4->s_net);
            PhInitFormatC(&format[7], L'.');
            PhInitFormatSR(&format[8], reverseLookupDomainName);

            if (PhFormatToBuffer(
                format,
                RTL_NUMBER_OF(format),
                reverseNameBuffer,
                sizeof(reverseNameBuffer),
                &returnLength
                ))
            {
                PH_STRINGREF reverseNameString;

                reverseNameString.Buffer = reverseNameBuffer;
                reverseNameString.Length = returnLength - sizeof(UNICODE_NULL);

                return PhCreateString2(&reverseNameString);
            }
            else
            {
                return PhFormat(format, RTL_NUMBER_OF(format), IP4_REVERSE_DOMAIN_STRING_LENGTH);
            }
        }
    case PH_NETWORK_TYPE_IPV6:
        {
            static CONST PH_STRINGREF reverseLookupDomainName = PH_STRINGREF_INIT(DNS_IP6_REVERSE_DOMAIN_STRING);
            PIN6_ADDR inAddrV6 = Address;
            PH_STRING_BUILDER stringBuilder;

            // DNS_MAX_IP6_REVERSE_NAME_LENGTH
            PhInitializeStringBuilder(&stringBuilder, IP6_REVERSE_DOMAIN_STRING_LENGTH);

            for (LONG i = sizeof(IN6_ADDR) - 1; i >= 0; i--)
            {
                PH_FORMAT format[4];
                SIZE_T returnLength;
                WCHAR reverseNameBuffer[IP6_REVERSE_DOMAIN_STRING_LENGTH];

                PhInitFormatX(&format[0], inAddrV6->s6_addr[i] & 0xF);
                PhInitFormatC(&format[1], L'.');
                PhInitFormatX(&format[2], (inAddrV6->s6_addr[i] >> 4) & 0xF);
                PhInitFormatC(&format[3], L'.');

                if (PhFormatToBuffer(
                    format,
                    RTL_NUMBER_OF(format),
                    reverseNameBuffer,
                    sizeof(reverseNameBuffer),
                    &returnLength
                    ))
                {
                    PhAppendStringBuilderEx(
                        &stringBuilder,
                        reverseNameBuffer,
                        returnLength - sizeof(UNICODE_NULL)
                        );
                }
                else
                {
                    PhAppendFormatStringBuilder(
                        &stringBuilder,
                        L"%hhx.%hhx.",
                        inAddrV6->s6_addr[i] & 0xF,
                        (inAddrV6->s6_addr[i] >> 4) & 0xF
                        );
                }
            }

            PhAppendStringBuilder(&stringBuilder, &reverseLookupDomainName);

            return PhFinalStringBuilderString(&stringBuilder);
        }
    default:
        return NULL;
    }
}

static typeof(&DnsQuery_W) DnsQuery_W_I = NULL;
#if defined(PHNT_DNSQUERY_FUTURE)
static typeof(&DnsQueryEx) DnsQueryEx_I = NULL;
static typeof(&DnsCancelQuery) DnsCancelQuery_I = NULL;
#endif
static typeof(&DnsExtractRecordsFromMessage_W) DnsExtractRecordsFromMessage_W_I = NULL;
#ifdef PH_NATIVE_DNS_WRITE_QUESTION
static typeof(&DnsWriteQuestionToBuffer_W) DnsWriteQuestionToBuffer_W_I = NULL;
#endif
static typeof(&DnsFree) DnsFree_I = NULL;

static BOOLEAN PhDnsApiInitialized(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOLEAN initialized = FALSE;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"dnsapi.dll"))
        {
            DnsQuery_W_I = PhGetDllBaseProcedureAddress(baseAddress, "DnsQuery_W", 0);
#if defined(PHNT_DNSQUERY_FUTURE)
            DnsQueryEx_I = PhGetDllBaseProcedureAddress(baseAddress, "DnsQueryEx", 0);
            DnsCancelQuery_I = PhGetDllBaseProcedureAddress(baseAddress, "DnsCancelQuery", 0);
#endif
            DnsExtractRecordsFromMessage_W_I = PhGetDllBaseProcedureAddress(baseAddress, "DnsExtractRecordsFromMessage_W", 0);
#ifdef PH_NATIVE_DNS_WRITE_QUESTION
            DnsWriteQuestionToBuffer_W_I = PhGetDllBaseProcedureAddress(baseAddress, "DnsWriteQuestionToBuffer_W", 0);
#endif
            DnsFree_I = PhGetDllBaseProcedureAddress(baseAddress, "DnsFree", 0);
        }

        if (
            DnsQuery_W_I &&
#if defined(PHNT_DNSQUERY_FUTURE)
            DnsQueryEx_I &&
            DnsCancelQuery_I &&
#endif
            DnsExtractRecordsFromMessage_W_I &&
#ifdef PH_NATIVE_DNS_WRITE_QUESTION
            DnsWriteQuestionToBuffer_W_I &&
#endif
            DnsFree_I
            )
        {
            initialized = TRUE;
        }

        PhEndInitOnce(&initOnce);
    }

    return initialized;
}

static BOOLEAN PhDnsIsWideStringAscii(
    _In_ PCWSTR String
    )
{
    while (*String)
    {
        if (*String >= 0x80)
            return FALSE;
        String++;
    }

    return TRUE;
}

static PUCHAR PhDnsWriteDottedNameToPacket(
    _Out_writes_bytes_(BufferLength) PUCHAR Buffer,
    _In_ ULONG BufferLength,
    _In_opt_ PCWSTR Name
    )
{
    PUCHAR bufferEnd = Buffer + BufferLength;
    UCHAR labelLength = 0;
    ULONG totalLength = 0;
    SIZE_T nameLength;
    CHAR utf8Buffer[256];
    PCSTR namePtr;

    if (BufferLength == 0)
        return NULL;

    if (Name)
    {
        nameLength = PhCountStringZ(Name);

        if (nameLength >= 256)
            return NULL;

        PCWCH sourceBuffer = Name;
        ULONG sourceLength = (ULONG)(nameLength * sizeof(WCHAR));
        ULONG utf8Length = 0;
        WCHAR canonicalBuffer[256];

        if (!PhDnsIsWideStringAscii(Name))
        {
            UNICODE_STRING sourceString;
            UNICODE_STRING canonicalString;

            sourceString.Buffer = (PWCH)Name;
            sourceString.Length = (USHORT)sourceLength;
            sourceString.MaximumLength = sourceString.Length;

            canonicalString.Buffer = canonicalBuffer;
            canonicalString.Length = 0;
            canonicalString.MaximumLength = sizeof(canonicalBuffer);

            if (!NT_SUCCESS(RtlDowncaseUnicodeString(&canonicalString, &sourceString, FALSE)))
                return NULL;

            sourceBuffer = canonicalString.Buffer;
            sourceLength = canonicalString.Length;
        }

        if (!NT_SUCCESS(RtlUnicodeToUTF8N(
            utf8Buffer,
            sizeof(utf8Buffer) - 1,
            &utf8Length,
            sourceBuffer,
            sourceLength
            )))
        {
            return NULL;
        }

        utf8Buffer[utf8Length] = ANSI_NULL;
        namePtr = utf8Buffer;

        if (*namePtr != '.')
        {
            PUCHAR labelLengthPtr = Buffer;
            PUCHAR dest = Buffer + 1;
            UCHAR ch = (UCHAR)*namePtr;

            while (ch != '\0')
            {
                namePtr++;

                if (dest >= bufferEnd)
                    return NULL;

                if (ch == '.')
                {
                    if (labelLength == 0 || labelLength > 63 || totalLength > 255)
                        return NULL;

                    *labelLengthPtr = labelLength;
                    totalLength++;
                    labelLengthPtr = dest++;
                    labelLength = 0;
                }
                else
                {
                    *dest++ = ch;
                    labelLength++;
                    totalLength++;
                }

                ch = (UCHAR)*namePtr;
            }

            if (totalLength <= 255 && labelLength <= 63)
            {
                if (labelLength != 0)
                {
                    *labelLengthPtr = labelLength;
                    if (dest >= bufferEnd)
                        return NULL;
                    *dest++ = 0;
                }
                else
                {
                    *labelLengthPtr = 0;
                }

                return dest;
            }

            return NULL;
        }

        if (namePtr[1] != '\0')
            return NULL;
    }

    *Buffer = 0;
    return Buffer + 1;
}

static PUCHAR PhDnsWriteQuestionToPacket(
    _Out_writes_bytes_(BufferLength) PUCHAR Buffer,
    _In_ ULONG BufferLength,
    _In_opt_ PCWSTR Name,
    _In_ USHORT Type,
    _In_ USHORT Class
    )
{
    PUCHAR bufferEnd = Buffer + BufferLength;
    PUCHAR current;
    PDNS_WIRE_QUESTION question;

    if (!(current = PhDnsWriteDottedNameToPacket(Buffer, BufferLength, Name)))
        return NULL;

    if ((SIZE_T)(bufferEnd - current) < sizeof(DNS_WIRE_QUESTION))
        return NULL;

    question = (PDNS_WIRE_QUESTION)current;
    question->QuestionType = _byteswap_ushort(Type);
    question->QuestionClass = _byteswap_ushort(Class);

    return PTR_ADD_OFFSET(current, sizeof(DNS_WIRE_QUESTION));
}

// If the buffer is too small, BufferSize receives the required size and the function
// returns FALSE (matches the dnsapi DnsWriteQuestionToBuffer_W resize protocol).
BOOLEAN PhpDnsWriteQuestionToBuffer(
    _Inout_ PDNS_MESSAGE_BUFFER DnsBuffer,
    _Inout_ PULONG BufferSize,
    _In_opt_ PCWSTR Name,
    _In_ USHORT Type,
    _In_ USHORT Class,
    _In_ USHORT Xid,
    _In_ BOOLEAN RecursionDesired
    )
{
    ULONG bufferSize;
    PUCHAR current;

    bufferSize = *BufferSize;

    if (bufferSize < 0x5C0)
    {
        *BufferSize = 0x5C0;
        return FALSE;
    }

    memset(&DnsBuffer->MessageHead, 0, sizeof(DNS_HEADER));

    current = PhDnsWriteQuestionToPacket(
        (PUCHAR)DnsBuffer + sizeof(DNS_HEADER),
        bufferSize - (ULONG)sizeof(DNS_HEADER),
        Name,
        Type,
        Class
        );

    if (current)
    {
        DnsBuffer->MessageHead.QuestionCount = _byteswap_ushort(1);
        DnsBuffer->MessageHead.RecursionDesired = RecursionDesired ? 1 : 0;
        DnsBuffer->MessageHead.Xid = _byteswap_ushort(Xid);

        *BufferSize = RtlPointerToOffset(DnsBuffer, current);
        return TRUE;
    }

    return FALSE;
}

// Signature-compatible replacement for dnsapi DnsWriteQuestionToBuffer_W (class is always IN).
// Used by the test projects to validate output against the OS implementation.
BOOLEAN WINAPI PhDnsWriteQuestionToBuffer_W(
    _Inout_ PDNS_MESSAGE_BUFFER DnsBuffer,
    _Inout_ PULONG BufferSize,
    _In_opt_ PCWSTR Name,
    _In_ USHORT Type,
    _In_ USHORT Xid,
    _In_ BOOLEAN RecursionDesired
    )
{
    return PhpDnsWriteQuestionToBuffer(
        DnsBuffer,
        BufferSize,
        Name,
        Type,
        DNS_CLASS_INTERNET,
        Xid,
        RecursionDesired
        );
}


_Success_(return)
static NTSTATUS PhCreateDnsMessageBuffer(
    _In_ PCWSTR Message,
    _In_ USHORT MessageType,
    _In_ USHORT MessageClass,
    _In_ USHORT MessageId,
    _Outptr_opt_result_maybenull_ PVOID* Buffer,
    _Out_opt_ ULONG* BufferLength
    )
{
    BOOLEAN status;
    ULONG dnsBufferLength;
    PDNS_MESSAGE_BUFFER buffer;

    dnsBufferLength = PAGE_SIZE;
    buffer = PhAllocateSafe(dnsBufferLength);
    if (!buffer) return STATUS_NO_MEMORY;

#ifndef PH_NATIVE_DNS_WRITE_QUESTION
    if (!(status = PhpDnsWriteQuestionToBuffer(
        buffer,
        &dnsBufferLength,
        Message,
        MessageType,
        MessageClass,
        MessageId,
        TRUE
        )))
    {
        PhFree(buffer);
        buffer = PhAllocateSafe(dnsBufferLength);
        if (!buffer) return STATUS_NO_MEMORY;

        status = PhpDnsWriteQuestionToBuffer(
            buffer,
            &dnsBufferLength,
            Message,
            MessageType,
            MessageClass,
            MessageId,
            TRUE
            );
    }
#else
    if (!(status = !!DnsWriteQuestionToBuffer_W_I(
        buffer,
        &dnsBufferLength,
        Message,
        MessageType,
        MessageId,
        TRUE
        )))
    {
        PhFree(buffer);
        buffer = PhAllocateSafe(dnsBufferLength);
        if (!buffer) return STATUS_NO_MEMORY;

        status = !!DnsWriteQuestionToBuffer_W_I(
            buffer,
            &dnsBufferLength,
            Message,
            MessageType,
            MessageId,
            TRUE
            );
    }
#endif

    if (status)
    {
        if (Buffer)
            *Buffer = buffer;
        else
            PhFree(buffer);

        if (BufferLength)
            *BufferLength = dnsBufferLength;

        return STATUS_SUCCESS;
    }
    else
    {
        if (buffer)
            PhFree(buffer);

        return STATUS_UNSUCCESSFUL;
    }
}

_Success_(return)
static NTSTATUS PhParseDnsMessageBuffer(
    _In_ USHORT Xid,
    _In_ PDNS_MESSAGE_BUFFER DnsReplyBuffer,
    _In_ ULONG DnsReplyBufferLength,
    _Outptr_opt_result_maybenull_ PVOID* DnsRecordList
    )
{
    DNS_STATUS status;
    PDNS_RECORD dnsRecordList = NULL;
    PDNS_HEADER dnsRecordHeader;

    if (DnsReplyBufferLength > USHRT_MAX)
        return STATUS_NO_MEMORY;

    // DNS_BYTE_FLIP_HEADER_COUNTS
    dnsRecordHeader = &DnsReplyBuffer->MessageHead;
    dnsRecordHeader->Xid = _byteswap_ushort(dnsRecordHeader->Xid);
    dnsRecordHeader->QuestionCount = _byteswap_ushort(dnsRecordHeader->QuestionCount);
    dnsRecordHeader->AnswerCount = _byteswap_ushort(dnsRecordHeader->AnswerCount);
    dnsRecordHeader->NameServerCount = _byteswap_ushort(dnsRecordHeader->NameServerCount);
    dnsRecordHeader->AdditionalCount = _byteswap_ushort(dnsRecordHeader->AdditionalCount);

    if (dnsRecordHeader->Xid != Xid)
        return STATUS_FAIL_CHECK;

    status = DnsExtractRecordsFromMessage_W_I(
        DnsReplyBuffer,
        (USHORT)DnsReplyBufferLength,
        &dnsRecordList
        );

    //status == ERROR_SUCCESS
    //status == DNS_ERROR_RCODE_NAME_ERROR

    if (dnsRecordList)
    {
        if (DnsRecordList)
            *DnsRecordList = dnsRecordList;

        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

// Cloudflare DNS over HTTPs (DoH)
// https://developers.cloudflare.com/1.1.1.1/dns-over-https/wireformat/
// host: cloudflare-dns.com
// host: one.one.one.one
// 1.1.1.1
// 1.0.0.1
// 2606:4700:4700::1111
// 2606:4700:4700::1001
//
// Google DNS over HTTPs (DoH)
// https://developers.google.com/speed/public-dns/docs/doh/
// host: dns.google
// 8.8.4.4
// 8.8.8.8
// 2001:4860:4860::8888
// 2001:4860:4860::8844
//
NTSTATUS PhHttpDnsQuery(
    _In_opt_ PCWSTR DnsServerAddress,
    _In_ PCWSTR DnsQueryMessage,
    _In_ USHORT DnsQueryMessageType,
    _Out_ PDNS_RECORD* DnsQueryRecord
    )
{
    return PhHttpDnsQueryEx(
        DnsServerAddress,
        DnsQueryMessage,
        DnsQueryMessageType,
        DNS_CLASS_INTERNET,
        DnsQueryRecord
        );
}

NTSTATUS PhHttpDnsQueryEx(
    _In_opt_ PCWSTR DnsServerAddress,
    _In_ PCWSTR DnsQueryMessage,
    _In_ USHORT DnsQueryMessageType,
    _In_ USHORT DnsQueryMessageClass,
    _Out_ PDNS_RECORD* DnsQueryRecord
    )
{
    static volatile USHORT seed = 0;
    NTSTATUS status;
    HINTERNET httpConnectionHandle = NULL;
    HINTERNET httpRequestHandle = NULL;
    PDNS_MESSAGE_BUFFER dnsSendBuffer = NULL;
    PDNS_MESSAGE_BUFFER dnsReceiveBuffer = NULL;
    PDNS_RECORD dnsRecordList = NULL;
    ULONG dnsSendBufferLength;
    ULONG dnsReceiveBufferLength;
    USHORT dnsQueryId;

    if (!PhDnsApiInitialized())
        return STATUS_UNSUCCESSFUL;

    dnsQueryId = _InterlockedIncrement16(&seed);

    status = PhCreateDnsMessageBuffer(
        DnsQueryMessage,
        DnsQueryMessageType,
        DnsQueryMessageClass,
        dnsQueryId,
        &dnsSendBuffer,
        &dnsSendBufferLength
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    if (!(httpConnectionHandle = PhCreateDohConnectionHandle(DnsServerAddress)))
        goto CleanupExit;

    if (!(httpRequestHandle = PhCreateDohRequestHandle(httpConnectionHandle)))
        goto CleanupExit;

    if (!WinHttpSendRequest(
        httpRequestHandle,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        dnsSendBuffer,
        dnsSendBufferLength,
        dnsSendBufferLength,
        0
        ))
    {
        status = PhGetLastWinHttpErrorAsNtStatus();
        goto CleanupExit;
    }

    if (!WinHttpReceiveResponse(httpRequestHandle, NULL))
    {
        status = PhGetLastWinHttpErrorAsNtStatus();
        goto CleanupExit;
    }

#if DEBUG
    {
        ULONG option = 0;
        ULONG optionLength = sizeof(ULONG);

        if (WinHttpQueryOption(
            httpRequestHandle,
            WINHTTP_OPTION_HTTP_PROTOCOL_USED,
            &option,
            &optionLength
            ))
        {
            if (option & WINHTTP_PROTOCOL_FLAG_HTTP3)
            {
                dprintf("[DOH] %s", "HTTP3 socket\n");
            }

            if (option & WINHTTP_PROTOCOL_FLAG_HTTP2)
            {
                dprintf("[DOH] %s", "HTTP2 socket\n");
            }
        }
    }
#endif

    status = PhHttpReadDataToBuffer(
        httpRequestHandle,
        0,
        &dnsReceiveBuffer,
        &dnsReceiveBufferLength
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    status = PhParseDnsMessageBuffer(
        dnsQueryId,
        dnsReceiveBuffer,
        dnsReceiveBufferLength,
        &dnsRecordList
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

CleanupExit:
    if (httpRequestHandle)
        WinHttpCloseHandle(httpRequestHandle);
    if (dnsReceiveBuffer)
        PhFree(dnsReceiveBuffer);
    if (dnsSendBuffer)
        PhFree(dnsSendBuffer);

    if (NT_SUCCESS(status))
    {
        *DnsQueryRecord = dnsRecordList;
    }

    return status;
}

PDNS_RECORD PhDnsQuery(
    _In_opt_ PCWSTR DnsServerAddress,
    _In_ PCWSTR DnsQueryMessage,
    _In_ USHORT DnsQueryMessageType
    )
{
    NTSTATUS status;
    PDNS_RECORD dnsRecordList = NULL;

    if (!PhDnsApiInitialized())
        return NULL;

    status = PhHttpDnsQuery(
        DnsServerAddress,
        DnsQueryMessage,
        DnsQueryMessageType,
        &dnsRecordList
        );

    if (!NT_SUCCESS(status))
    {
        if (DnsServerAddress)
        {
            IP4_ARRAY dnsServerAddressList;
            IN_ADDR dnsQueryServerAddressIpv4;
            USHORT dnsQueryServerAddressPort;

            status = RtlIpv4StringToAddressEx(
                DnsServerAddress,
                TRUE,
                &dnsQueryServerAddressIpv4,
                &dnsQueryServerAddressPort
                );
            dnsServerAddressList.AddrCount = !!NT_SUCCESS(status);
            dnsServerAddressList.AddrArray[0] = dnsQueryServerAddressIpv4.s_addr;

            DnsQuery_W_I(
                DnsQueryMessage,
                DnsQueryMessageType,
                DNS_QUERY_BYPASS_CACHE | DNS_QUERY_NO_HOSTS_FILE | DNS_QUERY_NO_MULTICAST,
                &dnsServerAddressList,
                &dnsRecordList,
                NULL
                );
        }
        else
        {
            DnsQuery_W_I(
                DnsQueryMessage,
                DnsQueryMessageType,
                DNS_QUERY_BYPASS_CACHE | DNS_QUERY_NO_HOSTS_FILE | DNS_QUERY_NO_MULTICAST,
                NULL,
                &dnsRecordList,
                NULL
                );
        }
    }

    return dnsRecordList;
}

PDNS_RECORD PhDnsQuery2(
    _In_opt_ PCWSTR DnsServerAddress,
    _In_ PCWSTR DnsQueryMessage,
    _In_ USHORT DnsQueryMessageType,
    _In_ USHORT DnsQueryMessageOptions
    )
{
    PDNS_RECORD dnsRecordList = NULL;

    if (PhDnsApiInitialized())
    {
        if (DnsServerAddress)
        {
            NTSTATUS status;
            IP4_ARRAY dnsServerAddressList;
            IN_ADDR dnsQueryServerAddressIpv4 = { 0 };
            USHORT dnsQueryServerAddressPort = 0;

            status = RtlIpv4StringToAddressEx(
                DnsServerAddress,
                FALSE,
                &dnsQueryServerAddressIpv4,
                &dnsQueryServerAddressPort
                );
            dnsServerAddressList.AddrCount = !!NT_SUCCESS(status);
            dnsServerAddressList.AddrArray[0] = dnsQueryServerAddressIpv4.s_addr;

            DnsQuery_W_I(
                DnsQueryMessage,
                DnsQueryMessageType,
                DNS_QUERY_BYPASS_CACHE | DNS_QUERY_NO_HOSTS_FILE | DNS_QUERY_NO_MULTICAST,
                &dnsServerAddressList,
                &dnsRecordList,
                NULL
                );
        }
        else
        {
            DnsQuery_W_I(
                DnsQueryMessage,
                DnsQueryMessageType,
                DnsQueryMessageOptions | DNS_QUERY_NO_HOSTS_FILE | DNS_QUERY_NO_MULTICAST,
                NULL,
                &dnsRecordList,
                NULL
                );
        }
    }

    return dnsRecordList;
}

VOID PhDnsFree(
    _In_ PDNS_RECORD DnsRecordList
    )
{
    if (!PhDnsApiInitialized())
        return;

    DnsFree_I(DnsRecordList, DnsFreeRecordList);
}

#if defined(PHNT_DNSQUERY_FUTURE)
typedef struct _PH_DNS_QUERY_CONTEXT
{
    volatile LONG RefCount;
    DNS_QUERY_RESULT QueryResults;
    DNS_QUERY_CANCEL QueryCancelContext;
    HANDLE QueryCompletedEvent;
    PDNS_RECORD QueryRecords;
} PH_DNS_QUERY_CONTEXT, *PPH_DNS_QUERY_CONTEXT;

VOID PhDnsAddReferenceQueryContext(
    _Inout_ PPH_DNS_QUERY_CONTEXT QueryContext
    )
{
    InterlockedIncrement(&QueryContext->RefCount);
}

VOID PhDnsDeReferenceQueryContext(
    _Inout_ PPH_DNS_QUERY_CONTEXT* QueryContext
    )
{
    PPH_DNS_QUERY_CONTEXT dnsQueryContext = *QueryContext;

    if (InterlockedDecrement(&dnsQueryContext->RefCount) == 0)
    {
        if (dnsQueryContext->QueryCompletedEvent)
        {
            NtClose(dnsQueryContext->QueryCompletedEvent);
        }

        PhFree(dnsQueryContext);
        *QueryContext = NULL;
    }
}

NTSTATUS PhDnsAllocateQueryContext(
    _Out_ PPH_DNS_QUERY_CONTEXT* QueryContext
    )
{
    NTSTATUS status;
    PPH_DNS_QUERY_CONTEXT context;
    HANDLE eventHandle;

    status = PhCreateEvent(
        &eventHandle,
        EVENT_ALL_ACCESS,
        SynchronizationEvent,
        FALSE
        );

    if (!NT_SUCCESS(status))
        return status;

    context = PhAllocateZero(sizeof(PH_DNS_QUERY_CONTEXT));
    PhDnsAddReferenceQueryContext(context);
    context->QueryResults.Version = DNS_QUERY_RESULTS_VERSION1;
    context->QueryCompletedEvent = eventHandle;

    *QueryContext = context;
    return status;
}

NTSTATUS PhDnsCreateDnsServerList(
    _In_ PCWSTR AddressString,
    _Inout_ PDNS_ADDR_ARRAY DnsQueryServerList)
{
    NTSTATUS status;
    IN_ADDR dnsQueryServerAddressIpv4;
    IN6_ADDR dnsQueryServerAddressIpv6;
    ULONG dnsQueryServerAddressScope;
    USHORT dnsQueryServerAddressPort;

    status = RtlIpv4StringToAddressEx(
        AddressString,
        TRUE,
        &dnsQueryServerAddressIpv4,
        &dnsQueryServerAddressPort
        );

    if (NT_SUCCESS(status))
    {
        memset(DnsQueryServerList, 0, sizeof(DNS_ADDR_ARRAY));
        DnsQueryServerList->MaxCount = 1;
        DnsQueryServerList->AddrCount = 1;

        ((PSOCKADDR_IN)&DnsQueryServerList->AddrArray[0])->sin_family = AF_INET;
        ((PSOCKADDR_IN)&DnsQueryServerList->AddrArray[0])->sin_addr = dnsQueryServerAddressIpv4;
        return status;
    }

    status = RtlIpv6StringToAddressEx(
        AddressString,
        &dnsQueryServerAddressIpv6,
        &dnsQueryServerAddressScope,
        &dnsQueryServerAddressPort
        );

    if (NT_SUCCESS(status))
    {
        memset(DnsQueryServerList, 0, sizeof(DNS_ADDR_ARRAY));
        DnsQueryServerList->MaxCount = 1;
        DnsQueryServerList->AddrCount = 1;

        ((PSOCKADDR_IN6)&DnsQueryServerList->AddrArray[0])->sin6_family = AF_INET6;
        ((PSOCKADDR_IN6)&DnsQueryServerList->AddrArray[0])->sin6_addr = dnsQueryServerAddressIpv6;
        return status;
    }

    return status;
}
// HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Services\Dnscache\Parameters\DohWellKnownServers
NTSTATUS PhDnsCreateCustomDnsServerList(
    _In_ PCWSTR AddressString,
    _Inout_ DNS_CUSTOM_SERVER* DnsCustomServerList
    )
{
    NTSTATUS status;
    IN_ADDR dnsQueryServerAddressIpv4;
    IN6_ADDR dnsQueryServerAddressIpv6;
    ULONG dnsQueryServerAddressScope;
    USHORT dnsQueryServerAddressPort;

    memset(DnsCustomServerList, 0, sizeof(DNS_CUSTOM_SERVER));
    DnsCustomServerList->dwServerType = DNS_CUSTOM_SERVER_TYPE_DOH;
    DnsCustomServerList->ullFlags = DNS_CUSTOM_SERVER_UDP_FALLBACK;
    DnsCustomServerList->pwszTemplate = L"https://cloudflare-dns.com/dns-query";

    status = RtlIpv4StringToAddressEx(
        AddressString,
        TRUE,
        &dnsQueryServerAddressIpv4,
        &dnsQueryServerAddressPort
        );

    if (NT_SUCCESS(status))
    {
        DnsCustomServerList->ServerAddr.si_family = AF_INET;
        DnsCustomServerList->ServerAddr.Ipv4.sin_family = AF_INET;
        DnsCustomServerList->ServerAddr.Ipv4.sin_addr = dnsQueryServerAddressIpv4;
        return status;
    }

    status = RtlIpv6StringToAddressEx(
        AddressString,
        &dnsQueryServerAddressIpv6,
        &dnsQueryServerAddressScope,
        &dnsQueryServerAddressPort
        );

    if (NT_SUCCESS(status))
    {
        DnsCustomServerList->ServerAddr.si_family = AF_INET6;
        DnsCustomServerList->ServerAddr.Ipv6.sin6_family = AF_INET6;
        DnsCustomServerList->ServerAddr.Ipv6.sin6_addr = dnsQueryServerAddressIpv6;
        return status;
    }

    return status;
}

VOID WINAPI PhDnsQueryCompleteCallback(
    _In_ PVOID Context,
    _Inout_ PDNS_QUERY_RESULT QueryResults
    )
{
    PPH_DNS_QUERY_CONTEXT context = (PPH_DNS_QUERY_CONTEXT)Context;

    context->QueryRecords = QueryResults->pQueryRecords;

    //if (QueryResults->QueryStatus != ERROR_SUCCESS)
    //{
    //    dprintf("DnsQuery failed: %%lu\n", QueryResults->QueryStatus);
    //}
    //
    //if (QueryResults->pQueryRecords)
    //{
    //    DnsRecordListFree(QueryResults->pQueryRecords, DnsFreeRecordList);
    //}

    NtSetEvent(context->QueryCompletedEvent, NULL);

    PhDnsDeReferenceQueryContext(&context);
}

PDNS_RECORD PhDnsQuery3(
    _In_opt_ PCWSTR DnsServerAddress,
    _In_ PCWSTR DnsQueryMessage,
    _In_ USHORT DnsQueryMessageType,
    _In_ USHORT DnsQueryMessageOptions
    )
{
    ULONG status;
    PDNS_RECORD dnsQueryRecords = NULL;
    PPH_DNS_QUERY_CONTEXT dnsQueryContext = NULL;
    DNS_ADDR_ARRAY dnsQueryServerList;
    DNS_CUSTOM_SERVER dnsCustomServerList;
    DNS_QUERY_REQUEST3 dnsQueryRequest;
    LARGE_INTEGER timeout;

    status = PhDnsAllocateQueryContext(&dnsQueryContext);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    memset(&dnsQueryRequest, 0, sizeof(dnsQueryRequest));
    dnsQueryRequest.Version = WindowsVersion < WINDOWS_11_22H2 ? DNS_QUERY_REQUEST_VERSION1 : DNS_QUERY_REQUEST_VERSION3;
    dnsQueryRequest.QueryName = DnsQueryMessage;
    dnsQueryRequest.QueryType = DnsQueryMessageType;
    dnsQueryRequest.QueryOptions = (ULONG64)DnsQueryMessageOptions;
    dnsQueryRequest.pQueryContext = dnsQueryContext;
    dnsQueryRequest.pQueryCompletionCallback = PhDnsQueryCompleteCallback;

    if (DnsServerAddress)
    {
        {
            status = PhDnsCreateDnsServerList(DnsServerAddress, &dnsQueryServerList);

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            dnsQueryRequest.pDnsServerList = &dnsQueryServerList;
        }

        {
            status = PhDnsCreateCustomDnsServerList(DnsServerAddress, &dnsCustomServerList);

            if (!NT_SUCCESS(status))
                goto CleanupExit;

            dnsQueryRequest.cCustomServers = 1; // PhFinalArrayCount()
            dnsQueryRequest.pCustomServers = &dnsCustomServerList; // PhFinalArrayItems()
        }
    }

    PhDnsAddReferenceQueryContext(dnsQueryContext);

    status = DnsQueryEx_I(
        (PDNS_QUERY_REQUEST)&dnsQueryRequest,
        &dnsQueryContext->QueryResults,
        &dnsQueryContext->QueryCancelContext
        );

    if (status != DNS_REQUEST_PENDING)
    {
        PhDnsQueryCompleteCallback(dnsQueryContext, &dnsQueryContext->QueryResults);
        goto CleanupExit;
    }

    if (NtWaitForSingleObject(
        dnsQueryContext->QueryCompletedEvent,
        FALSE,
        PhTimeoutFromMilliseconds(&timeout, 5000)
        ) == WAIT_TIMEOUT)
    {
        DnsCancelQuery_I(&dnsQueryContext->QueryCancelContext);

        NtWaitForSingleObject(
            dnsQueryContext->QueryCompletedEvent,
            FALSE,
            PhTimeoutFromMilliseconds(&timeout, INFINITE)
            );
    }

CleanupExit:

    dnsQueryRecords = dnsQueryContext->QueryRecords;

    //if (dnsQueryContext->pQueryRecords)
    //{
    //    DnsRecordListFree(dnsQueryContext->pQueryRecords, DnsFreeRecordList);
    //}

    if (dnsQueryContext)
    {
        PhDnsDeReferenceQueryContext(&dnsQueryContext);
    }

    return dnsQueryRecords;
}
#endif
