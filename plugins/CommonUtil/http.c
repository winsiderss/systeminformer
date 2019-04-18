/*
 * Process Hacker -
 *   http socket wrapper
 *
 * Copyright (C) 2017 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

// dmex: This wrapper is utilizing a shared project until stable enough for inclusion with the base program.

#include <ph.h>
#include <phbasesup.h>
#include <http.h>
#include <winhttp.h>

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

BOOLEAN PhHttpSocketCreate(
    _Out_ PPH_HTTP_CONTEXT *HttpContext,
    _In_opt_ PWSTR HttpUserAgent
    )
{
    PPH_HTTP_CONTEXT httpContext;

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

    if (WindowsVersion >= WINDOWS_8_1)
    {
        WinHttpSetOption(
            httpContext->SessionHandle,
            WINHTTP_OPTION_DECOMPRESSION,
            &(ULONG){ WINHTTP_DECOMPRESSION_FLAG_GZIP | WINHTTP_DECOMPRESSION_FLAG_DEFLATE },
            sizeof(ULONG)
            );
    }

    *HttpContext = httpContext;

    return TRUE;
}

VOID PhHttpSocketDestroy(
    _Frees_ptr_opt_ PPH_HTTP_CONTEXT HttpContext
    )
{
    if (HttpContext->RequestHandle)
        WinHttpCloseHandle(HttpContext->RequestHandle);

    if (HttpContext->ConnectionHandle)
        WinHttpCloseHandle(HttpContext->ConnectionHandle);

    if (HttpContext->SessionHandle)
        WinHttpCloseHandle(HttpContext->SessionHandle);

    PhFree(HttpContext);
}

BOOLEAN PhHttpSocketConnect(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PWSTR ServerName,
    _In_ USHORT ServerPort
    )
{
    HttpContext->ConnectionHandle = WinHttpConnect(
        HttpContext->SessionHandle,
        ServerName,
        ServerPort,
        0
        );

    if (HttpContext->ConnectionHandle)
        return TRUE;
    else
        return FALSE;
}

BOOLEAN PhHttpSocketBeginRequest(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PWSTR Method,
    _In_ PWSTR UrlPath,
    _In_ ULONG Flags
    )
{
    ULONG httpFlags = 0;

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

    WinHttpSetOption(
        HttpContext->RequestHandle,
        WINHTTP_OPTION_DISABLE_FEATURE,
        &(ULONG){ WINHTTP_DISABLE_KEEP_ALIVE },
        sizeof(ULONG)
        );

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
    return !!WinHttpReadData(HttpContext->RequestHandle, Buffer, BufferLength, BytesCopied);
}

BOOLEAN PhHttpSocketWriteData(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG BytesCopied
    )
{
    return !!WinHttpWriteData(HttpContext->RequestHandle, Buffer, BufferLength, BytesCopied);
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
        HeadersLength ? HeadersLength : -1L, 
        WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE
        );
}

_Check_return_
_Ret_maybenull_
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
        WINHTTP_QUERY_CUSTOM,
        HeaderString,
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

BOOLEAN PhHttpSocketQueryHeaderUlong(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ ULONG QueryValue,
    _Out_ PULONG HeaderValue
    )
{
    ULONG queryFlags = 0;
    ULONG headerValue = 0;
    ULONG valueLength = sizeof(ULONG);

    PhMapFlags1(
        &queryFlags,
        QueryValue,
        PhpHttpHeaderQueryMappings,
        RTL_NUMBER_OF(PhpHttpHeaderQueryMappings)
        );

    if (WinHttpQueryHeaders(
        HttpContext->RequestHandle,
        queryFlags | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &headerValue,
        &valueLength,
        WINHTTP_NO_HEADER_INDEX
        ))
    {
        *HeaderValue = headerValue;
        return TRUE;
    }
 
    return FALSE;
}

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

_Check_return_
_Ret_maybenull_
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
        stringBuffer = PhCreateString(optionBuffer); // FIXME
        PhFree(optionBuffer);
    }

    return stringBuffer;
}

PVOID PhHttpSocketDownloadString(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ BOOLEAN Unicode
    )
{
    PSTR data = NULL;
    PVOID result = NULL;
    ULONG allocatedLength;
    ULONG dataLength;
    ULONG returnLength;
    BYTE buffer[PAGE_SIZE];

    allocatedLength = sizeof(buffer);
    data = (PSTR)PhAllocate(allocatedLength);
    dataLength = 0;

    while (WinHttpReadData(HttpContext->RequestHandle, buffer, PAGE_SIZE, &returnLength))
    {
        if (returnLength == 0)
            break;

        if (allocatedLength < dataLength + returnLength)
        {
            allocatedLength *= 2;
            data = (PSTR)PhReAllocate(data, allocatedLength);
        }

        memcpy(data + dataLength, buffer, returnLength);

        dataLength += returnLength;
    }

    if (allocatedLength < dataLength + 1)
    {
        allocatedLength++;
        data = (PSTR)PhReAllocate(data, allocatedLength);
    }

    // Ensure that the buffer is null-terminated.
    data[dataLength] = 0;

    if (Unicode)
        result = PhConvertUtf8ToUtf16Ex(data, dataLength);
    else
        result = PhCreateBytesEx(data, dataLength);

    PhFree(data);

    return result;
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

_Check_return_
_Ret_maybenull_
PPH_STRING PhHttpSocketGetErrorMessage(
    _In_ ULONG ErrorCode
    )
{
    PPH_STRING message = NULL;

    if (message = PhGetMessage(PhGetDllHandle(L"winhttp.dll"), 0xb, GetUserDefaultLangID(), ErrorCode))
    {
        PhTrimToNullTerminatorString(message);
    }

    // Remove any trailing newline
    if (message && message->Length >= 2 * sizeof(WCHAR) &&
        message->Buffer[message->Length / sizeof(WCHAR) - 2] == '\r' &&
        message->Buffer[message->Length / sizeof(WCHAR) - 1] == '\n')
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
    return WinHttpSetCredentials(
        HttpContext->RequestHandle, 
        WINHTTP_AUTH_TARGET_SERVER, 
        WINHTTP_AUTH_SCHEME_BASIC, 
        Name,
        Value,
        NULL
        );
}

