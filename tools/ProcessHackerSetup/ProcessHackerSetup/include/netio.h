#ifndef _PH_NETIO_H
#define _PH_NETIO_H

#pragma comment(lib, "Winhttp.lib")
#include <winhttp.h>

typedef struct _HTTP_SESSION
{
    HINTERNET SessionHandle;
    HINTERNET ConnectionHandle;
    HINTERNET RequestHandle;
} HTTP_SESSION, *P_HTTP_SESSION;

typedef struct _HTTP_PARSED_URL
{
    WCHAR HttpMethod[10];
    WCHAR HttpServer[200];
    WCHAR HttpPath[200];
} *HTTP_PARSED_URL;


P_HTTP_SESSION HttpSocketCreate(VOID);

BOOLEAN HttpConnect(
    _Inout_ P_HTTP_SESSION HttpSocket,
    _In_ PCWSTR ServerName,
    _In_ INTERNET_PORT ServerPort
    );

BOOLEAN HttpBeginRequest(
    _Inout_ P_HTTP_SESSION HttpSocket,
    _In_ PCWSTR MethodType,
    _In_ PCWSTR UrlPath,
    _In_ ULONG Flags
    );

BOOLEAN HttpSendRequest(
    _Inout_ P_HTTP_SESSION HttpSocket,
    _In_ ULONG TotalLength
    );

BOOLEAN HttpEndRequest(
    _Inout_ P_HTTP_SESSION HttpSocket
    );

BOOLEAN HttpAddRequestHeaders(
    _Inout_ P_HTTP_SESSION HttpSocket,
    _In_ PCWSTR RequestHeaders
    );

PPH_STRING HttpGetRequestHeaderString(
    _Inout_ P_HTTP_SESSION HttpSocket,
    _In_ PCWSTR RequestHeader
    );

ULONG HttpGetRequestHeaderDword(
    _Inout_ P_HTTP_SESSION HttpSocket,
    _In_ ULONG Flags
    );

PPH_STRING HttpDownloadString(
    _Inout_ P_HTTP_SESSION HttpSocket
    );

BOOLEAN HttpParseURL(
    _Inout_ P_HTTP_SESSION HttpSocket, 
    _In_ PCWSTR Url,
    _Out_ HTTP_PARSED_URL* HttpParsedUrl
    );

#endif