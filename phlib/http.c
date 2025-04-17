/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2017-2023
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

NTSTATUS PhGetLastWinHttpErrorAsNtStatus(
    VOID
    )
{
    return PhHttpErrorToNtStatus(PhGetLastError());
}

VOID CALLBACK PhWinHttpStatusCallback(
    _In_ HINTERNET InternetHandle,
    _In_ ULONG_PTR Context,
    _In_ ULONG InternetStatus,
    _In_opt_ PVOID StatusInformation,
    _In_ ULONG StatusInformationLength
    )
{
    //switch (InternetStatus)
    //{
    //case WINHTTP_CALLBACK_STATUS_RESOLVING_NAME:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_RESOLVING_NAME\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_NAME_RESOLVED:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_NAME_RESOLVED\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_SENDING_REQUEST:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_SENDING_REQUEST\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_REQUEST_SENT:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_REQUEST_SENT\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_HANDLE_CREATED:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_HANDLE_CREATED\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_DETECTING_PROXY:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_DETECTING_PROXY\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_REDIRECT:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_REDIRECT\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_SECURE_FAILURE:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_SECURE_FAILURE\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_READ_COMPLETE\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_REQUEST_ERROR\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_GETPROXYFORURL_COMPLETE:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_GETPROXYFORURL_COMPLETE\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_CLOSE_COMPLETE:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_CLOSE_COMPLETE\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_SHUTDOWN_COMPLETE:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_SHUTDOWN_COMPLETE\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_GETPROXYSETTINGS_COMPLETE:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_GETPROXYSETTINGS_COMPLETE\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_SETTINGS_WRITE_COMPLETE:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_SETTINGS_WRITE_COMPLETE\n");
    //    }
    //    break;
    //case WINHTTP_CALLBACK_STATUS_SETTINGS_READ_COMPLETE:
    //    {
    //        dprintf("WINHTTP_CALLBACK_STATUS_SETTINGS_READ_COMPLETE\n");
    //    }
    //    break;
    //}
}

static NTSTATUS PhWinHttpOpen(
    _Out_ HINTERNET* SessionHandle
    )
{
    HINTERNET sessionHandle;
    ULONG httpAccessType;

    if (WindowsVersion >= WINDOWS_8_1)
        httpAccessType = WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY;
    else
        httpAccessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;

    if (sessionHandle = WinHttpOpen(
        L"SystemInformer_3.2_A2D1C96D_D25915D9",
        httpAccessType,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
        ))
    {
        *SessionHandle = sessionHandle;
        return STATUS_SUCCESS;
    }

    return PhGetLastWinHttpErrorAsNtStatus();
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

#if DEBUG
    PhHttpSetContext(sessionHandle, httpContext);
    PhHttpSetCallback(sessionHandle, PhWinHttpStatusCallback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS);
#endif

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
    _In_ _Frees_ptr_ PPH_HTTP_CONTEXT HttpContext
    )
{
    if (!HttpContext)
        return;

    if (HttpContext->SessionHandle)
    {
        // WinHTTP does not synchronize WinHttpSetStatusCallback with worker threads.
        // If a callback originating in another thread is in progress when an application calls WinHttpSetStatusCallback,
        // the application still receives a callback notification even after WinHttpSetStatusCallback successfully
        // sets the callback function to NULL and returns.

        PhHttpSetCallback(HttpContext->SessionHandle, NULL, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS);
    }

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
    _In_ PCWSTR RequestMethod,
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
    }

    return status;
}

NTSTATUS PhHttpSendRequest(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_opt_ PVOID OptionalBuffer,
    _In_opt_ ULONG OptionalLength,
    _In_opt_ ULONG TotalLength
    )
{
    if (WinHttpSendRequest(
        HttpContext->RequestHandle,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        OptionalBuffer,
        OptionalLength,
        TotalLength,
        0
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

    // Retrieves additional information about the status of a response that might not be reflected by the response status code.

    if (WinHttpQueryHeaders(
        HttpContext->RequestHandle,
        WINHTTP_QUERY_WARNING,
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

NTSTATUS PhHttpReadDataToBuffer(
    _In_ PVOID RequestHandle,
    _Out_ PVOID *Buffer,
    _Out_ ULONG *BufferLength
    )
{
    PSTR data;
    ULONG allocatedLength;
    ULONG dataLength;
    ULONG returnLength;
    BYTE buffer[PAGE_SIZE];

    allocatedLength = sizeof(buffer);
    data = (PSTR)PhAllocate(allocatedLength);
    dataLength = 0;

    while (WinHttpReadData(RequestHandle, buffer, PAGE_SIZE, &returnLength))
    {
        if (returnLength == 0)
            break;

        if (allocatedLength < dataLength + returnLength)
        {
            allocatedLength *= 2;
            data = PhReAllocate(data, allocatedLength);
        }

        memcpy(data + dataLength, buffer, returnLength);

        dataLength += returnLength;
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
            *BufferLength = dataLength;

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
    _In_ PPH_HTTPDOWNLOAD_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
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
    PH_HTTPDOWNLOAD_CONTEXT context;
    IO_STATUS_BLOCK isb;
    BYTE buffer[PAGE_SIZE];

    PhQuerySystemTime(&timeStart);

    status = PhHttpQueryHeaderUlong64(
        HttpContext,
        PH_HTTP_QUERY_CONTENT_LENGTH,
        &numberOfBytesTotal
        );

    if (!NT_SUCCESS(status))
        return status;

    if (numberOfBytesTotal == 0)
        return STATUS_UNSUCCESSFUL;

    fileName = PhGetTemporaryDirectoryRandomAlphaFileName();
    fileSize.QuadPart = (LONGLONG)numberOfBytesTotal;

    if (PhIsNullOrEmptyString(fileName))
        return STATUS_UNSUCCESSFUL;

    status = PhCreateFileWin32Ex(
        &fileHandle,
        PhGetString(fileName),
        FILE_GENERIC_WRITE,
        &fileSize,
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

        if (numberOfBytesRead != isb.Information)
        {
            status = STATUS_UNSUCCESSFUL;
            break;
        }

        if (Callback)
        {
            PhQuerySystemTime(&timeNow);
            timeTicks = (timeNow.QuadPart - timeStart.QuadPart) / PH_TICKS_PER_SEC;

            numberOfBytesReadTotal += isb.Information;
            context.ReadLength = numberOfBytesReadTotal;
            context.TotalLength = numberOfBytesTotal;
            context.BitsPerSecond = numberOfBytesReadTotal / __max(timeTicks, 1);
            context.Percent = (((DOUBLE)numberOfBytesReadTotal / (DOUBLE)numberOfBytesTotal) * 100);

            if (!Callback(&context, Context))
                break;
        }
    }

    if (numberOfBytesReadTotal != numberOfBytesTotal)
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
    ULONG optionLength = (ULONG)Value->Length;

    if (WinHttpSetOption(
        HttpContext->RequestHandle,
        Option,
        Value->Buffer,
        optionLength
        ))
    {
        return STATUS_SUCCESS;
    }

    return PhGetLastWinHttpErrorAsNtStatus();
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

    if (previousCallback = WINHTTP_INVALID_STATUS_CALLBACK)
    {
        return PhGetLastWinHttpErrorAsNtStatus();
    }

    return STATUS_SUCCESS;
}

NTSTATUS PhHttpSetContext(
    _In_ PVOID HttpHandle,
    _In_ PVOID Context
    )
{
    if (WinHttpSetOption(
        HttpHandle,
        WINHTTP_OPTION_CONTEXT_VALUE,
        Context,
        sizeof(ULONG_PTR)
        ))
    {
        return STATUS_SUCCESS;
    }

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

NTSTATUS PhHttpSetProtocal(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ BOOLEAN Session,
    _In_ ULONG Protocal,
    _In_ ULONG Timeout
    )
{
    NTSTATUS status;

    if (Session)
    {
        status = PhHttpSetOption(HttpContext->SessionHandle, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, Protocal);

        if (!NT_SUCCESS(status))
            return status;

        if (FlagOn(Protocal, PH_HTTP_PROTOCOL_FLAG_HTTP3))
        {
            status = PhHttpSetOption(HttpContext->SessionHandle, WINHTTP_OPTION_HTTP3_HANDSHAKE_TIMEOUT, Timeout);
        }
    }
    else
    {
        status = PhHttpSetOption(HttpContext->RequestHandle, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, Protocal);

        if (!NT_SUCCESS(status))
            return status;
        
        if (FlagOn(Protocal, PH_HTTP_PROTOCOL_FLAG_HTTP3))
        {
            status = PhHttpSetOption(HttpContext->RequestHandle, WINHTTP_OPTION_HTTP3_HANDSHAKE_TIMEOUT, Timeout);
        }
    }

    return status;
}

NTSTATUS PhHttpCrackUrl(
    _In_ PPH_STRING Url,
    _Out_opt_ PPH_STRING *HostPart,
    _Out_opt_ PPH_STRING *PathPart,
    _Out_opt_ PUSHORT PortPart
    )
{
    URL_COMPONENTS httpParts;

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

    if (HostPart && httpParts.dwHostNameLength)
        *HostPart = PhCreateStringEx(httpParts.lpszHostName, httpParts.dwHostNameLength * sizeof(WCHAR));

    if (PathPart && httpParts.dwUrlPathLength)
        *PathPart = PhCreateStringEx(httpParts.lpszUrlPath, httpParts.dwUrlPathLength * sizeof(WCHAR));

    if (PortPart)
        *PortPart = httpParts.nPort;

    return STATUS_SUCCESS;
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
        HttpContext->SessionHandle,
        WINHTTP_OPTION_EXTENDED_ERROR,
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
        HttpContext->SessionHandle,
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
        HttpContext->SessionHandle,
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
        HttpContext->SessionHandle,
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

//
// IPv4 and IPv6
//

NTSTATUS PhIpv4AddressToString(
    _In_ PCIN_ADDR Address,
    _In_ USHORT Port,
    _Out_writes_to_(*AddressStringLength, *AddressStringLength) PWSTR AddressString,
    _Inout_ PULONG AddressStringLength
    )
{
    return RtlIpv4AddressToStringEx(Address, Port, AddressString, AddressStringLength);
}

NTSTATUS PhIpv4StringToAddress(
    _In_ PCWSTR AddressString,
    _In_ BOOLEAN Strict,
    _Out_ PIN_ADDR Address,
    _Out_ PUSHORT Port
    )
{
    return RtlIpv4StringToAddressEx(AddressString, Strict, Address, Port);
}

NTSTATUS PhIpv6AddressToString(
    _In_ PCIN6_ADDR Address,
    _In_ ULONG ScopeId,
    _In_ USHORT Port,
    _Out_writes_to_(*AddressStringLength, *AddressStringLength) PWSTR AddressString,
    _Inout_ PULONG AddressStringLength
    )
{
    return RtlIpv6AddressToStringEx(Address, ScopeId, Port, AddressString, AddressStringLength);
}

NTSTATUS PhIpv6StringToAddress(
    _In_ PCWSTR AddressString,
    _Out_ PIN6_ADDR Address,
    _Out_ PULONG ScopeId,
    _Out_ PUSHORT Port
    )
{
    return RtlIpv6StringToAddressEx(AddressString, Address, ScopeId, Port);
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

static __typeof__(&DnsQuery_W) DnsQuery_W_I = NULL;
#if (PHNT_DNSQUERY_FUTURE)
static __typeof__(&DnsQueryEx) DnsQueryEx_I = NULL;
static __typeof__(&DnsCancelQuery) DnsCancelQuery_I = NULL;
#endif
static __typeof__(&DnsExtractRecordsFromMessage_W) DnsExtractRecordsFromMessage_W_I = NULL;
static __typeof__(&DnsWriteQuestionToBuffer_W) DnsWriteQuestionToBuffer_W_I = NULL;
static __typeof__(&DnsFree) DnsFree_I = NULL;

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
#if (PHNT_DNSQUERY_FUTURE)
            DnsQueryEx_I = PhGetDllBaseProcedureAddress(baseAddress, "DnsQueryEx", 0);
            DnsCancelQuery_I = PhGetDllBaseProcedureAddress(baseAddress, "DnsCancelQuery", 0);
#endif
            DnsExtractRecordsFromMessage_W_I = PhGetDllBaseProcedureAddress(baseAddress, "DnsExtractRecordsFromMessage_W", 0);
            DnsWriteQuestionToBuffer_W_I = PhGetDllBaseProcedureAddress(baseAddress, "DnsWriteQuestionToBuffer_W", 0);
            DnsFree_I = PhGetDllBaseProcedureAddress(baseAddress, "DnsFree", 0);
        }

        if (
            DnsQuery_W_I &&
#if (PHNT_DNSQUERY_FUTURE)
            DnsQueryEx_I &&
            DnsCancelQuery_I &&
#endif
            DnsExtractRecordsFromMessage_W_I &&
            DnsWriteQuestionToBuffer_W_I &&
            DnsFree_I
            )
        {
            initialized = TRUE;
        }

        PhEndInitOnce(&initOnce);
    }

    return initialized;
}

_Success_(return)
static NTSTATUS PhCreateDnsMessageBuffer(
    _In_ PCWSTR Message,
    _In_ USHORT MessageType,
    _In_ USHORT MessageId,
    _Outptr_opt_result_maybenull_ PVOID* Buffer,
    _Out_opt_ ULONG* BufferLength
    )
{
    BOOLEAN status;
    ULONG dnsBufferLength;
    PDNS_MESSAGE_BUFFER dnsBuffer;

    dnsBufferLength = PAGE_SIZE;
    dnsBuffer = PhAllocate(dnsBufferLength);

    if (!(status = !!DnsWriteQuestionToBuffer_W_I(
        dnsBuffer,
        &dnsBufferLength,
        Message,
        MessageType,
        MessageId,
        TRUE
        )))
    {
        PhFree(dnsBuffer);
        dnsBuffer = PhAllocate(dnsBufferLength);

        status = !!DnsWriteQuestionToBuffer_W_I(
            dnsBuffer,
            &dnsBufferLength,
            Message,
            MessageType,
            MessageId,
            TRUE
            );
    }

    if (status)
    {
        if (Buffer)
            *Buffer = dnsBuffer;
        else
            PhFree(dnsBuffer);

        if (BufferLength)
            *BufferLength = dnsBufferLength;

        return STATUS_SUCCESS;
    }
    else
    {
        if (dnsBuffer)
            PhFree(dnsBuffer);

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

    if (NT_SUCCESS(status))
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

#if (PHNT_DNSQUERY_FUTURE)
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
