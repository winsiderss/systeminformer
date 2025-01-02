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

_Success_(return)
BOOLEAN PhHttpSocketCreate(
    _Out_ PPH_HTTP_CONTEXT *HttpContext,
    _In_opt_ PCWSTR HttpUserAgent
    )
{
    PPH_HTTP_CONTEXT httpContext;
    ULONG httpOptions;

    httpContext = PhAllocate(sizeof(PH_HTTP_CONTEXT));
    memset(httpContext, 0, sizeof(PH_HTTP_CONTEXT));

    httpContext->SessionHandle = WinHttpOpen(
        HttpUserAgent,
        WindowsVersion >= WINDOWS_8_1 ? WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
        );

    if (!httpContext->SessionHandle)
    {
        PhFree(httpContext);
        return FALSE;
    }

    if (WindowsVersion < WINDOWS_8_1)
    {
        httpOptions = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;

        WinHttpSetOption(
            httpContext->SessionHandle,
            WINHTTP_OPTION_SECURE_PROTOCOLS,
            &httpOptions,
            sizeof(ULONG)
            );
    }
    else
    {
        httpOptions = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;

        WinHttpSetOption(
            httpContext->SessionHandle,
            WINHTTP_OPTION_SECURE_PROTOCOLS,
            &httpOptions,
            sizeof(ULONG)
            );

        httpOptions = WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE;

        WinHttpSetOption(
            httpContext->SessionHandle,
            WINHTTP_OPTION_DECOMPRESSION,
            &httpOptions,
            sizeof(ULONG)
            );

        if (WindowsVersion >= WINDOWS_10)
        {
            httpOptions = WINHTTP_PROTOCOL_FLAG_HTTP2;

            WinHttpSetOption(
                httpContext->SessionHandle,
                WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL,
                &httpOptions,
                sizeof(ULONG)
                );
        }

        if (WindowsVersion >= WINDOWS_11)
        {
#ifdef WINHTTP_OPTION_DISABLE_GLOBAL_POOLING
            httpOptions = TRUE;

            WinHttpSetOption(
                httpContext->SessionHandle,
                WINHTTP_OPTION_DISABLE_GLOBAL_POOLING,
                &httpOptions,
                sizeof(ULONG)
                );
#endif
        }

        if (WindowsVersion >= WINDOWS_11)
        {
#ifdef WINHTTP_OPTION_TLS_FALSE_START
            httpOptions = TRUE;

            WinHttpSetOption(
                httpContext->SessionHandle,
                WINHTTP_OPTION_TLS_FALSE_START,
                &httpOptions,
                sizeof(ULONG)
                );
#endif
        }
    }

    *HttpContext = httpContext;

    return TRUE;
}

VOID PhHttpSocketDestroy(
    _In_ _Frees_ptr_ PPH_HTTP_CONTEXT HttpContext
    )
{
    if (!HttpContext)
        return;

    PhHttpSocketClose(HttpContext, ULONG_MAX);

    PhFree(HttpContext);
}

VOID PhHttpSocketClose(
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

BOOLEAN PhHttpSocketConnect(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PWSTR ServerName,
    _In_ USHORT ServerPort
    )
{
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
    //else
    {
        HttpContext->ConnectionHandle = WinHttpConnect(
            HttpContext->SessionHandle,
            ServerName,
            ServerPort,
            0
            );
    }

    if (HttpContext->ConnectionHandle)
        return TRUE;

    return FALSE;
}

BOOLEAN PhHttpSocketBeginRequest(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_opt_ PWSTR Method,
    _In_ PWSTR UrlPath,
    _In_ ULONG Flags
    )
{
    ULONG httpFlags = 0;
    //ULONG httpOptions;

    PhMapFlags1(
        &httpFlags,
        Flags,
        PhpHttpRequestFlagMappings,
        RTL_NUMBER_OF(PhpHttpRequestFlagMappings)
        );

    HttpContext->RequestHandle = WinHttpOpenRequest(
        HttpContext->ConnectionHandle,
        Method,
        UrlPath,
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        httpFlags
        );

    if (!HttpContext->RequestHandle)
        return FALSE;

    //if (HttpContext->ServerName)
    //{
    //    PPH_STRING headerHost;
    //
    //    headerHost = PhFormatString(L"Host: %s", HttpContext->ServerName);
    //    PhHttpSocketAddRequestHeaders(HttpContext, headerHost->Buffer, ULONG_MAX);
    //
    //    PhDereferenceObject(headerHost);
    //}

    PhHttpSocketSetFeature(HttpContext, PH_HTTP_FEATURE_KEEP_ALIVE, FALSE);
    //
    // httpOptions = WINHTTP_DISABLE_KEEP_ALIVE;
    //
    //WinHttpSetOption(
    //    HttpContext->RequestHandle,
    //    WINHTTP_OPTION_DISABLE_FEATURE,
    //    &httpOptions,
    //    sizeof(ULONG)
    //    );
    //
    // httpOptions = WINHTTP_PROTOCOL_FLAG_HTTP2;
    //
    //WinHttpSetOption(
    //    HttpContext->RequestHandle,
    //    WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL,
    //    &httpOptions,
    //    sizeof(ULONG)
    //    );

    return TRUE;
}

BOOLEAN PhHttpSocketSendRequest(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_opt_ PVOID RequestData,
    _In_opt_ ULONG RequestDataLength
    )
{
    return !!WinHttpSendRequest(
        HttpContext->RequestHandle,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        RequestData,
        RequestDataLength,
        RequestDataLength,
        0
        );
}

BOOLEAN PhHttpSocketEndRequest(
    _In_ PPH_HTTP_CONTEXT HttpContext
    )
{
    return !!WinHttpReceiveResponse(HttpContext->RequestHandle, NULL);
}

BOOLEAN PhHttpSocketReadData(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG BytesCopied
    )
{
    return !!WinHttpReadData(
        HttpContext->RequestHandle,
        Buffer,
        BufferLength,
        BytesCopied
        );
}

BOOLEAN PhHttpSocketWriteData(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG BytesCopied
    )
{
    return !!WinHttpWriteData(
        HttpContext->RequestHandle,
        Buffer,
        BufferLength,
        BytesCopied
        );
}

BOOLEAN PhHttpSocketAddRequestHeaders(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PWSTR Headers,
    _In_opt_ ULONG HeadersLength
    )
{
    return !!WinHttpAddRequestHeaders(
        HttpContext->RequestHandle,
        Headers,
        HeadersLength ? HeadersLength : ULONG_MAX,
        WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE
        );
}

PPH_STRING PhHttpSocketQueryHeaders(
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

PPH_STRING PhHttpSocketQueryHeaderString(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PWSTR HeaderString
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

_Success_(return)
BOOLEAN PhHttpSocketQueryHeaderUlong(
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

    ULONG64 headerValue;

    if (PhHttpSocketQueryHeaderUlong64(HttpContext, QueryValue, &headerValue))
    {
        if (headerValue <= ULONG_MAX)
        {
            *HeaderValue = (ULONG)headerValue;
            return TRUE;
        }
    }

    return FALSE;
}

_Success_(return)
BOOLEAN PhHttpSocketQueryHeaderUlong64(
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
        PH_STRINGREF string;
        ULONG64 integer;

        string.Buffer = valueBuffer;
        string.Length = valueLength;

        if (PhStringToInteger64(&string, 10, &integer))
        {
            *HeaderValue = integer;
            return TRUE;
        }
    }

    return FALSE;
}

//ULONG PhHttpSocketQueryStatusCode(
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

PVOID PhHttpSocketQueryOption(
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

PPH_STRING PhHttpSocketQueryOptionString(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ BOOLEAN SessionOption,
    _In_ ULONG QueryOption
    )
{
    PVOID optionBuffer;
    PPH_STRING stringBuffer = NULL;

    optionBuffer = PhHttpSocketQueryOption(
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

_Success_(return)
BOOLEAN PhHttpSocketReadDataToBuffer(
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

        return TRUE;
    }
    else
    {
        PhFree(data);

        return FALSE;
    }
}

PVOID PhHttpSocketDownloadString(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ BOOLEAN Unicode
    )
{
    PVOID result;
    PVOID buffer;
    ULONG bufferLength;

    if (!PhHttpSocketReadDataToBuffer(
        HttpContext->RequestHandle,
        &buffer,
        &bufferLength
        ))
    {
        return NULL;
    }

    if (Unicode)
        result = PhConvertUtf8ToUtf16Ex(buffer, bufferLength);
    else
        result = PhCreateBytesEx(buffer, bufferLength);

    PhFree(buffer);

    return result;
}

NTSTATUS PhHttpSocketDownloadToFile(
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

    if (!PhHttpSocketQueryHeaderUlong64(HttpContext, PH_HTTP_QUERY_CONTENT_LENGTH, &numberOfBytesTotal))
        return PhGetLastWin32ErrorAsNtStatus();

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

    while (PhHttpSocketReadData(HttpContext, buffer, PAGE_SIZE, &numberOfBytesRead))
    {
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

BOOLEAN PhHttpSocketSetFeature(
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

    return !!WinHttpSetOption(
        HttpContext->RequestHandle,
        Enable ? WINHTTP_OPTION_ENABLE_FEATURE : WINHTTP_OPTION_DISABLE_FEATURE,
        &featureValue,
        sizeof(ULONG)
        );
}

BOOLEAN PhHttpSocketSetSecurity(
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

    return !!WinHttpSetOption(
        HttpContext->RequestHandle,
        WINHTTP_OPTION_SECURITY_FLAGS,
        &featureValue,
        sizeof(ULONG)
        );
}

_Success_(return)
BOOLEAN PhHttpSocketParseUrl(
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
        return FALSE;
    }

    if (HostPart && httpParts.dwHostNameLength)
        *HostPart = PhCreateStringEx(httpParts.lpszHostName, httpParts.dwHostNameLength * sizeof(WCHAR));

    if (PathPart && httpParts.dwUrlPathLength)
        *PathPart = PhCreateStringEx(httpParts.lpszUrlPath, httpParts.dwUrlPathLength * sizeof(WCHAR));

    if (PortPart)
        *PortPart = httpParts.nPort;

    return TRUE;
}

PPH_STRING PhHttpSocketGetErrorMessage(
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

BOOLEAN PhHttpSocketSetCredentials(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PCWSTR Name,
    _In_ PCWSTR Value
    )
{
    return !!WinHttpSetCredentials(
        HttpContext->RequestHandle,
        WINHTTP_AUTH_TARGET_SERVER,
        WINHTTP_AUTH_SCHEME_BASIC,
        Name,
        Value,
        NULL
        );
}

HINTERNET PhCreateDohConnectionHandle(
    _In_opt_ PCWSTR DnsServerAddress
    )
{
    static HINTERNET httpSessionHandle = NULL;
    static HINTERNET httpConnectionHandle = NULL;
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    ULONG httpOptions;

    if (PhBeginInitOnce(&initOnce))
    {
        if (httpSessionHandle = WinHttpOpen(
            NULL,
            WindowsVersion >= WINDOWS_8_1 ? WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0
            ))
        {
            if (WindowsVersion < WINDOWS_8_1)
            {
                httpOptions = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;

                WinHttpSetOption(
                    httpSessionHandle,
                    WINHTTP_OPTION_SECURE_PROTOCOLS,
                    &httpOptions,
                    sizeof(ULONG)
                    );
            }
            else
            {
                httpOptions = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;

                WinHttpSetOption(
                    httpSessionHandle,
                    WINHTTP_OPTION_SECURE_PROTOCOLS,
                    &httpOptions,
                    sizeof(ULONG)
                    );

                httpOptions = WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE;

                WinHttpSetOption(
                    httpSessionHandle,
                    WINHTTP_OPTION_DECOMPRESSION,
                    &httpOptions,
                    sizeof(ULONG)
                    );

                if (WindowsVersion >= WINDOWS_10)
                {
                    httpOptions = WINHTTP_PROTOCOL_FLAG_HTTP2;

                    WinHttpSetOption(
                        httpSessionHandle,
                        WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL,
                        &httpOptions,
                        sizeof(ULONG)
                        );
                }

                if (WindowsVersion >= WINDOWS_11)
                {
#ifdef WINHTTP_OPTION_DISABLE_GLOBAL_POOLING
                    httpOptions = TRUE;

                    WinHttpSetOption(
                        httpSessionHandle,
                        WINHTTP_OPTION_DISABLE_GLOBAL_POOLING,
                        &httpOptions,
                        sizeof(ULONG)
                        );
#endif
                }


                if (WindowsVersion >= WINDOWS_11)
                {
#ifdef WINHTTP_OPTION_TLS_FALSE_START
                    httpOptions = TRUE;

                    WinHttpSetOption(
                        httpSessionHandle,
                        WINHTTP_OPTION_TLS_FALSE_START,
                        &httpOptions,
                        sizeof(ULONG)
                        );
#endif
                }
            }

            httpOptions = 1;

            WinHttpSetOption(
                httpSessionHandle,
                WINHTTP_OPTION_MAX_CONNS_PER_SERVER,
                &httpOptions, // HACK
                sizeof(ULONG)
                );

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

static typeof(&DnsQuery_W) DnsQuery_W_I = nullptr;
#if (PHNT_DNSQUERY_FUTURE)
static typeof(&DnsQueryEx) DnsQueryEx_I = nullptr;
static typeof(&DnsCancelQuery) DnsCancelQuery_I = nullptr;
#endif
static typeof(&DnsExtractRecordsFromMessage_W) DnsExtractRecordsFromMessage_W_I = nullptr;
static typeof(&DnsWriteQuestionToBuffer_W) DnsWriteQuestionToBuffer_W_I = nullptr;
static typeof(&DnsFree) DnsFree_I = nullptr;

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
static BOOLEAN PhCreateDnsMessageBuffer(
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

        return TRUE;
    }
    else
    {
        if (dnsBuffer)
            PhFree(dnsBuffer);

        return FALSE;
    }
}

_Success_(return)
static BOOLEAN PhParseDnsMessageBuffer(
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
        return FALSE;

    // DNS_BYTE_FLIP_HEADER_COUNTS
    dnsRecordHeader = &DnsReplyBuffer->MessageHead;
    dnsRecordHeader->Xid = _byteswap_ushort(dnsRecordHeader->Xid);
    dnsRecordHeader->QuestionCount = _byteswap_ushort(dnsRecordHeader->QuestionCount);
    dnsRecordHeader->AnswerCount = _byteswap_ushort(dnsRecordHeader->AnswerCount);
    dnsRecordHeader->NameServerCount = _byteswap_ushort(dnsRecordHeader->NameServerCount);
    dnsRecordHeader->AdditionalCount = _byteswap_ushort(dnsRecordHeader->AdditionalCount);

    if (dnsRecordHeader->Xid != Xid)
        return FALSE;

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

        return TRUE;
    }

    return FALSE;
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
PDNS_RECORD PhHttpDnsQuery(
    _In_opt_ PCWSTR DnsServerAddress,
    _In_ PCWSTR DnsQueryMessage,
    _In_ USHORT DnsQueryMessageType
    )
{
    static volatile USHORT seed = 0;
    HINTERNET httpConnectionHandle = NULL;
    HINTERNET httpRequestHandle = NULL;
    PDNS_MESSAGE_BUFFER dnsSendBuffer = NULL;
    PDNS_MESSAGE_BUFFER dnsReceiveBuffer = NULL;
    PDNS_RECORD dnsRecordList = NULL;
    ULONG dnsSendBufferLength;
    ULONG dnsReceiveBufferLength;
    USHORT dnsQueryId;

    if (!PhDnsApiInitialized())
        return FALSE;

    dnsQueryId = _InterlockedIncrement16(&seed);

    if (!PhCreateDnsMessageBuffer(
        DnsQueryMessage,
        DnsQueryMessageType,
        dnsQueryId,
        &dnsSendBuffer,
        &dnsSendBufferLength
        ))
    {
        goto CleanupExit;
    }

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
        goto CleanupExit;
    }

    if (!WinHttpReceiveResponse(httpRequestHandle, NULL))
        goto CleanupExit;

    //{
    //    ULONG option = 0;
    //    ULONG optionLength = sizeof(ULONG);
    //
    //    if (WinHttpQueryOption(
    //        httpRequestHandle,
    //        WINHTTP_OPTION_HTTP_PROTOCOL_USED,
    //        &option,
    //        &optionLength
    //        ))
    //    {
    //        if (option & WINHTTP_PROTOCOL_FLAG_HTTP3)
    //        {
    //            dprintf("HTTP3 socket\n");
    //        }
    //
    //        if (option & WINHTTP_PROTOCOL_FLAG_HTTP2)
    //        {
    //            dprintf("HTTP2 socket\n");
    //        }
    //    }
    //}

    if (!PhHttpSocketReadDataToBuffer(
        httpRequestHandle,
        &dnsReceiveBuffer,
        &dnsReceiveBufferLength
        ))
    {
        goto CleanupExit;
    }

    if (!PhParseDnsMessageBuffer(
        dnsQueryId,
        dnsReceiveBuffer,
        dnsReceiveBufferLength,
        &dnsRecordList
        ))
    {
        goto CleanupExit;
    }

CleanupExit:
    if (httpRequestHandle)
        WinHttpCloseHandle(httpRequestHandle);
    if (dnsReceiveBuffer)
        PhFree(dnsReceiveBuffer);
    if (dnsSendBuffer)
        PhFree(dnsSendBuffer);

    return dnsRecordList;
}

PDNS_RECORD PhDnsQuery(
    _In_opt_ PCWSTR DnsServerAddress,
    _In_ PCWSTR DnsQueryMessage,
    _In_ USHORT DnsQueryMessageType
    )
{
    PDNS_RECORD dnsRecordList;

    dnsRecordList = PhHttpDnsQuery(
        DnsServerAddress,
        DnsQueryMessage,
        DnsQueryMessageType
        );

    if (!dnsRecordList && PhDnsApiInitialized())
    {
        if (DnsServerAddress)
        {
            NTSTATUS status;
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
            PhTimeoutFromMilliseconds(&timeout, -MINLONGLONG)
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
