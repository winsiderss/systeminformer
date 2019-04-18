#ifndef _PH_NETIO_H
#define _PH_NETIO_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PH_HTTP_CONTEXT
{
    PVOID SessionHandle;
    PVOID ConnectionHandle;
    PVOID RequestHandle;
} PH_HTTP_CONTEXT, *PPH_HTTP_CONTEXT;

BOOLEAN
NTAPI
PhHttpSocketCreate(
    _Out_ PPH_HTTP_CONTEXT *HttpContext,
    _In_opt_ PWSTR HttpUserAgent
    );

VOID
NTAPI
PhHttpSocketDestroy(
    _Frees_ptr_opt_ PPH_HTTP_CONTEXT HttpContext
    );

#define PH_HTTP_DEFAULT_PORT 0 // use the protocol-specific default
#define PH_HTTP_DEFAULT_HTTP_PORT 80
#define PH_HTTP_DEFAULT_HTTPS_PORT 443

BOOLEAN
NTAPI
PhHttpSocketConnect(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PWSTR ServerName,
    _In_ USHORT ServerPort
    );

#define PH_HTTP_FLAG_SECURE 0x1
#define PH_HTTP_FLAG_REFRESH 0x2

BOOLEAN
NTAPI
PhHttpSocketBeginRequest(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PWSTR Method,
    _In_ PWSTR UrlPath,
    _In_ ULONG Flags
    );

BOOLEAN
NTAPI
PhHttpSocketSendRequest(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_opt_ PVOID RequestData,
    _In_opt_ ULONG RequestDataLength
    );

BOOLEAN
NTAPI
PhHttpSocketEndRequest(
    _In_ PPH_HTTP_CONTEXT HttpContext
    );

BOOLEAN
NTAPI
PhHttpSocketReadData(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG BytesCopied
    );

BOOLEAN
NTAPI
PhHttpSocketWriteData(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG BytesCopied
    );

BOOLEAN
NTAPI
PhHttpSocketAddRequestHeaders(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PWSTR Headers,
    _In_opt_ ULONG HeadersLength
    );

_Check_return_
_Ret_maybenull_
PPH_STRING
NTAPI
PhHttpSocketQueryHeaderString(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PWSTR HeaderString
    );

// status codes
#define PH_HTTP_STATUS_OK 200 // request completed
#define PH_HTTP_STATUS_CREATED 201
#define PH_HTTP_STATUS_REDIRECT_METHOD 303 // redirection w/ new access method
#define PH_HTTP_STATUS_REDIRECT 302 // object temporarily moved

// header query flags
#define PH_HTTP_QUERY_CONTENT_LENGTH 0x1
#define PH_HTTP_QUERY_STATUS_CODE 0x2

BOOLEAN
NTAPI
PhHttpSocketQueryHeaderUlong(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ ULONG QueryValue,
    _Out_ PULONG HeaderValue
    );

_Check_return_
_Ret_maybenull_
PPH_STRING
NTAPI
PhHttpSocketQueryOptionString(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ BOOLEAN SessionOption,
    _In_ ULONG QueryOption
    );

PVOID
NTAPI
PhHttpSocketDownloadString(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ BOOLEAN Unicode
    );

#define PH_HTTP_FEATURE_REDIRECTS 0x1
#define PH_HTTP_FEATURE_KEEP_ALIVE 0x2

BOOLEAN
NTAPI
PhHttpSocketSetFeature(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ ULONG Feature,
    _In_ BOOLEAN Enable
    );

BOOLEAN
NTAPI
PhHttpSocketParseUrl(
    _In_ PPH_STRING Url,
    _Out_opt_ PPH_STRING *Host,
    _Out_opt_ PPH_STRING *Path,
    _Out_opt_ PUSHORT Port
    );

_Check_return_
_Ret_maybenull_
PPH_STRING
NTAPI
PhHttpSocketGetErrorMessage(
    _In_ ULONG ErrorCode
    );

_Check_return_
BOOLEAN 
NTAPI
PhHttpSocketSetCredentials(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PCWSTR Name,
    _In_ PCWSTR Value
    );

#ifdef __cplusplus
}
#endif

#endif
