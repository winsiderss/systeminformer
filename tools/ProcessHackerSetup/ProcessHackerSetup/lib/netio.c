#include <setup.h>
#include "netio.h"

static VOID HttpSocketFree(
    _In_ __deref_out PVOID Object
    )
{
    P_HTTP_SESSION httpSocket = (P_HTTP_SESSION)Object;

    if (httpSocket->RequestHandle)
        WinHttpCloseHandle(httpSocket->RequestHandle);

    if (httpSocket->ConnectionHandle)
        WinHttpCloseHandle(httpSocket->ConnectionHandle);

    if (httpSocket->SessionHandle)
        WinHttpCloseHandle(httpSocket->SessionHandle);
}

P_HTTP_SESSION HttpSocketCreate(VOID)
{
    P_HTTP_SESSION httpSocket;
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig = { 0 };
    
    httpSocket = (P_HTTP_SESSION)PhAllocate(sizeof(HTTP_SESSION));
    memset(httpSocket, 0, sizeof(HTTP_SESSION));

    WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig);

    httpSocket->SessionHandle = WinHttpOpen(
        NULL,
        proxyConfig.lpszProxy ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        proxyConfig.lpszProxy ? proxyConfig.lpszProxy : WINHTTP_NO_PROXY_NAME,
        proxyConfig.lpszProxy ? proxyConfig.lpszProxyBypass : WINHTTP_NO_PROXY_BYPASS,
        0
        );

    return httpSocket;
}

BOOLEAN HttpConnect(
    _Inout_ P_HTTP_SESSION HttpSocket,
    _In_ PCWSTR ServerName,
    _In_ INTERNET_PORT ServerPort
    )
{
    // Create the HTTP connection handle.
    HttpSocket->ConnectionHandle = WinHttpConnect(
        HttpSocket->SessionHandle,
        ServerName,
        ServerPort,
        0
        );

    if (HttpSocket->ConnectionHandle)
        return TRUE;

    return FALSE;
}

BOOLEAN HttpBeginRequest(
    _Inout_ P_HTTP_SESSION HttpSocket,
    _In_ PCWSTR MethodType,
    _In_ PCWSTR UrlPath,
    _In_ ULONG Flags
    )
{
    HttpSocket->RequestHandle = WinHttpOpenRequest(
        HttpSocket->ConnectionHandle,
        MethodType,
        UrlPath,
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        Flags
        );

    if (HttpSocket->RequestHandle)
        return TRUE;

    return FALSE;
}

BOOLEAN HttpSendRequest(
    _Inout_ P_HTTP_SESSION HttpSocket,
    _In_ ULONG TotalLength
    )
{
    return WinHttpSendRequest(
        HttpSocket->RequestHandle,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        TotalLength,
        0
        ) == TRUE;
}

BOOLEAN HttpEndRequest(
    _Inout_ P_HTTP_SESSION HttpSocket
    )
{
    return WinHttpReceiveResponse(HttpSocket->RequestHandle, NULL) == TRUE;
}

BOOLEAN HttpAddRequestHeaders(
    _Inout_ P_HTTP_SESSION HttpSocket,
    _In_ PCWSTR RequestHeaders
    )
{
    return WinHttpAddRequestHeaders(HttpSocket->RequestHandle, RequestHeaders, -1L, WINHTTP_ADDREQ_FLAG_ADD) == TRUE;
}

//PVOID HttpGetRequestHeaderValue(
//    _Inout_ P_HTTP_SESSION HttpSocket,
//    _In_ LPCWSTR RequestHeader,
//    _In_ ULONG Flags
//    )
//{
//    PVOID buffer = NULL;
//    ULONG bufferSize = 0;
//    LRESULT keyResult = NO_ERROR;
//
//    // Get the length of the data...
//    if (!WinHttpQueryHeaders(
//        HttpSocket->RequestHandle,
//        Flags,
//        RequestHeader,
//        WINHTTP_NO_OUTPUT_BUFFER,
//        &bufferSize,
//        WINHTTP_NO_HEADER_INDEX
//        ))
//    {
//        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
//            return NULL;
//    }
//
//    // Allocate the buffer...
//    buffer = PhAllocate(bufferSize, NULL);
//
//    // Query the value...
//    if (!WinHttpQueryHeaders(
//        HttpSocket->RequestHandle,
//        Flags,
//        RequestHeader,
//        buffer,
//        &bufferSize,
//        WINHTTP_NO_HEADER_INDEX
//        ))
//    {
//        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
//            return NULL;
//    }
//
//    if (buffer)
//        return buffer;
//
//    PhFree(buffer);
//    return NULL;
//}

PPH_STRING HttpGetRequestHeaderString(
    _Inout_ P_HTTP_SESSION HttpSocket,
    _In_ PCWSTR RequestHeader
    )
{
    ULONG bufferSize = 0;
    PPH_STRING stringBuffer = NULL;

    // Get the length of the data...
    if (!WinHttpQueryHeaders(
        HttpSocket->RequestHandle,
        WINHTTP_QUERY_CUSTOM,
        RequestHeader,
        WINHTTP_NO_OUTPUT_BUFFER,
        &bufferSize,
        WINHTTP_NO_HEADER_INDEX
        ))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return NULL;
    }

    // Allocate the buffer...
    stringBuffer = PhCreateStringEx(NULL, bufferSize);

    // Query the data value...
    if (WinHttpQueryHeaders(
        HttpSocket->RequestHandle,
        WINHTTP_QUERY_CUSTOM,
        RequestHeader,
        stringBuffer->Buffer,
        &bufferSize,
        WINHTTP_NO_HEADER_INDEX
        ))
    {
        return stringBuffer;
    }

    PhDereferenceObject(stringBuffer);
    return NULL;
}

ULONG HttpGetRequestHeaderDword(
    _Inout_ P_HTTP_SESSION HttpSocket,
    _In_ ULONG Flags
    )
{
    ULONG dwordResult = 0;
    ULONG dwordLength = sizeof(ULONG);
    ULONG dwordResultTemp = 0;

    if (WinHttpQueryHeaders(
        HttpSocket->RequestHandle,
        Flags | WINHTTP_QUERY_FLAG_NUMBER,
        NULL,
        &dwordResultTemp,
        &dwordLength,
        WINHTTP_NO_HEADER_INDEX
        ))
    {
        dwordResult = dwordResultTemp;
    }

    return dwordResult;
}

PPH_STRING HttpDownloadString(
    _Inout_ P_HTTP_SESSION HttpSocket
    )
{
    PSTR tempDataPtr = NULL;
    PPH_STRING tempHttpString = NULL;
    PPH_STRING hashETag = NULL;
    PPH_STRING finalHexString = NULL;

    ULONG dataLength = 0;
    ULONG returnLength = 0;
    ULONG allocatedLength = PAGE_SIZE;
    BYTE buffer[PAGE_SIZE];

    tempDataPtr = (PSTR)PhAllocate(allocatedLength);

    while (WinHttpReadData(HttpSocket->RequestHandle, buffer, PAGE_SIZE, &returnLength))
    {
        if (returnLength == 0)
            break;

        if (allocatedLength < dataLength + returnLength)
        {
            allocatedLength *= 2;
            tempDataPtr = (PSTR)PhReAllocate(tempDataPtr, allocatedLength);
        }

        memcpy(tempDataPtr + dataLength, buffer, returnLength);
        memset(buffer, 0, returnLength);

        dataLength += returnLength;
    }

    // Add space for the null terminator..
    if (allocatedLength < dataLength + 1)
    {
        allocatedLength++;
        tempDataPtr = (PSTR)PhReAllocate(tempDataPtr, allocatedLength);
    }

    // Ensure that the buffer is null-terminated.
    tempDataPtr[dataLength] = 0;

    tempHttpString = PhConvertMultiByteToUtf16(tempDataPtr);

    if (hashETag)
    {
        PhDereferenceObject(hashETag);
    }

    if (finalHexString)
    {
        PhDereferenceObject(finalHexString);
    }

    if (tempDataPtr)
    {
        PhFree(tempDataPtr);
    }

    return tempHttpString;
}

BOOLEAN HttpParseURL(
    _Inout_ P_HTTP_SESSION HttpSocket, 
    _In_ PCWSTR Url,
    _Out_ HTTP_PARSED_URL* HttpParsedUrl
    )
{
    URL_COMPONENTS httpUrlComponents;

    memset(&httpUrlComponents, 0, sizeof(URL_COMPONENTS));

    httpUrlComponents.dwStructSize = sizeof(URL_COMPONENTS);
    httpUrlComponents.dwSchemeLength = (ULONG)-1;
    httpUrlComponents.dwHostNameLength = (ULONG)-1;
    httpUrlComponents.dwUrlPathLength = (ULONG)-1;

    if (WinHttpCrackUrl(
        Url,
        0,//(ULONG)wcslen(Url),
        0,
        &httpUrlComponents
        ))
    {
        HTTP_PARSED_URL httpParsedUrl = PhAllocate(sizeof(struct _HTTP_PARSED_URL));
        memset(httpParsedUrl, 0, sizeof(struct _HTTP_PARSED_URL));

        wmemcpy(httpParsedUrl->HttpMethod, httpUrlComponents.lpszScheme, httpUrlComponents.dwSchemeLength);
        wmemcpy(httpParsedUrl->HttpServer, httpUrlComponents.lpszHostName, httpUrlComponents.dwHostNameLength);
        wmemcpy(httpParsedUrl->HttpPath, httpUrlComponents.lpszUrlPath, httpUrlComponents.dwUrlPathLength); 

        *HttpParsedUrl = httpParsedUrl;

        return TRUE;
    }

    return FALSE;
}