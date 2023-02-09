/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2017-2023
 *
 */

#ifndef _PH_PHNET_H
#define _PH_PHNET_H

EXTERN_C_START

#define __WINDOT11_H__ // temporary preprocessor workaround (dmex)

#ifndef UM_NDIS60
#define UM_NDIS60 1
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2def.h>
#include <windns.h>
#include <nldef.h>
#include <iphlpapi.h>
#include <mstcpip.h>
#include <icmpapi.h>

#define PH_IPV4_NETWORK_TYPE 0x1
#define PH_IPV6_NETWORK_TYPE 0x2
#define PH_NETWORK_TYPE_MASK 0x3

#define PH_TCP_PROTOCOL_TYPE 0x10
#define PH_UDP_PROTOCOL_TYPE 0x20
#define PH_PROTOCOL_TYPE_MASK 0x30

#define PH_NO_NETWORK_PROTOCOL 0x0
#define PH_TCP4_NETWORK_PROTOCOL (PH_IPV4_NETWORK_TYPE | PH_TCP_PROTOCOL_TYPE)
#define PH_TCP6_NETWORK_PROTOCOL (PH_IPV6_NETWORK_TYPE | PH_TCP_PROTOCOL_TYPE)
#define PH_UDP4_NETWORK_PROTOCOL (PH_IPV4_NETWORK_TYPE | PH_UDP_PROTOCOL_TYPE)
#define PH_UDP6_NETWORK_PROTOCOL (PH_IPV6_NETWORK_TYPE | PH_UDP_PROTOCOL_TYPE)

typedef struct _PH_IP_ADDRESS
{
    ULONG Type;
    union
    {
        ULONG Ipv4;
        IN_ADDR InAddr;
        UCHAR Ipv6[16];
        IN6_ADDR In6Addr;
    };
} PH_IP_ADDRESS, *PPH_IP_ADDRESS;

FORCEINLINE BOOLEAN PhEqualIpAddress(
    _In_ PPH_IP_ADDRESS Address1,
    _In_ PPH_IP_ADDRESS Address2
    )
{
    if ((Address1->Type | Address2->Type) == 0) // don't check addresses if both are invalid
        return TRUE;
    if (Address1->Type != Address2->Type)
        return FALSE;

    if (Address1->Type == PH_IPV4_NETWORK_TYPE)
    {
        return IN4_ADDR_EQUAL(&Address1->InAddr, &Address2->InAddr);
        // return Address1->Ipv4 == Address2->Ipv4;
    }
    else
    {
        return IN6_ADDR_EQUAL(&Address1->In6Addr, &Address2->In6Addr);
//#ifdef _WIN64
//        return
//            *(PULONG64)(Address1->Ipv6) == *(PULONG64)(Address2->Ipv6) &&
//            *(PULONG64)(Address1->Ipv6 + 8) == *(PULONG64)(Address2->Ipv6 + 8);
//#else
//        return
//            *(PULONG)(Address1->Ipv6) == *(PULONG)(Address2->Ipv6) &&
//            *(PULONG)(Address1->Ipv6 + 4) == *(PULONG)(Address2->Ipv6 + 4) &&
//            *(PULONG)(Address1->Ipv6 + 8) == *(PULONG)(Address2->Ipv6 + 8) &&
//            *(PULONG)(Address1->Ipv6 + 12) == *(PULONG)(Address2->Ipv6 + 12);
//#endif
    }
}

FORCEINLINE ULONG PhHashIpAddress(
    _In_ PPH_IP_ADDRESS Address
    )
{
    ULONG hash = 0;

    if (Address->Type == 0)
        return 0;

    hash = Address->Type | (Address->Type << 16);

    if (Address->Type == PH_IPV4_NETWORK_TYPE)
    {
        hash ^= Address->Ipv4;
    }
    else
    {
        hash += *(PULONG)(Address->Ipv6);
        hash ^= *(PULONG)(Address->Ipv6 + 4);
        hash += *(PULONG)(Address->Ipv6 + 8);
        hash ^= *(PULONG)(Address->Ipv6 + 12);
    }

    return hash;
}

FORCEINLINE BOOLEAN PhIsNullIpAddress(
    _In_ PPH_IP_ADDRESS Address
    )
{
    if (Address->Type == 0)
    {
        return TRUE;
    }
    else if (Address->Type == PH_IPV4_NETWORK_TYPE)
    {
        return IN4_IS_ADDR_UNSPECIFIED(&Address->InAddr);
        //return Address->Ipv4 == 0;
    }
    else if (Address->Type == PH_IPV6_NETWORK_TYPE)
    {
        return IN6_IS_ADDR_UNSPECIFIED(&Address->In6Addr);
//#ifdef _WIN64
//        return (*(PULONG64)(Address->Ipv6) | *(PULONG64)(Address->Ipv6 + 8)) == 0;
//#else
//        return (*(PULONG)(Address->Ipv6) | *(PULONG)(Address->Ipv6 + 4) |
//            *(PULONG)(Address->Ipv6 + 8) | *(PULONG)(Address->Ipv6 + 12)) == 0;
//#endif
    }
    else
    {
        return TRUE;
    }
}

typedef struct _PH_IP_ENDPOINT
{
    PH_IP_ADDRESS Address;
    ULONG Port;
} PH_IP_ENDPOINT, *PPH_IP_ENDPOINT;

FORCEINLINE BOOLEAN PhEqualIpEndpoint(
    _In_ PPH_IP_ENDPOINT Endpoint1,
    _In_ PPH_IP_ENDPOINT Endpoint2
    )
{
    return
        PhEqualIpAddress(&Endpoint1->Address, &Endpoint2->Address) &&
        Endpoint1->Port == Endpoint2->Port;
}

FORCEINLINE ULONG PhHashIpEndpoint(
    _In_ PPH_IP_ENDPOINT Endpoint
    )
{
    return PhHashIpAddress(&Endpoint->Address) ^ Endpoint->Port;
}

// DOH/HTTP/HTTP2 support (dmex)

typedef struct _PH_HTTP_CONTEXT
{
    PVOID SessionHandle;
    PVOID ConnectionHandle;
    PVOID RequestHandle;
} PH_HTTP_CONTEXT, *PPH_HTTP_CONTEXT;

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhHttpSocketCreate(
    _Out_ PPH_HTTP_CONTEXT *HttpContext,
    _In_opt_ PWSTR HttpUserAgent
    );

PHLIBAPI
VOID
NTAPI
PhHttpSocketDestroy(
    _In_ _Frees_ptr_ PPH_HTTP_CONTEXT HttpContext
    );

typedef enum _PH_HTTP_SOCKET_CLOSE_TYPE
{
    PH_HTTP_SOCKET_CLOSE_SESSION = 0x1,
    PH_HTTP_SOCKET_CLOSE_CONNECTION = 0x2,
    PH_HTTP_SOCKET_CLOSE_REQUEST = 0x4,
} PH_HTTP_SOCKET_CLOSE_TYPE;

PHLIBAPI
VOID
NTAPI
PhHttpSocketClose(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PH_HTTP_SOCKET_CLOSE_TYPE Type
    );

#define PH_HTTP_DEFAULT_PORT 0 // use the protocol-specific default port
#define PH_HTTP_DEFAULT_HTTP_PORT 80
#define PH_HTTP_DEFAULT_HTTPS_PORT 443

PHLIBAPI
BOOLEAN
NTAPI
PhHttpSocketConnect(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PWSTR ServerName,
    _In_ USHORT ServerPort
    );

#define PH_HTTP_FLAG_SECURE 0x1
#define PH_HTTP_FLAG_REFRESH 0x2

PHLIBAPI
BOOLEAN
NTAPI
PhHttpSocketBeginRequest(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_opt_ PWSTR Method,
    _In_ PWSTR UrlPath,
    _In_ ULONG Flags
    );

PHLIBAPI
BOOLEAN
NTAPI
PhHttpSocketSendRequest(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_opt_ PVOID RequestData,
    _In_opt_ ULONG RequestDataLength
    );

PHLIBAPI
BOOLEAN
NTAPI
PhHttpSocketEndRequest(
    _In_ PPH_HTTP_CONTEXT HttpContext
    );

PHLIBAPI
BOOLEAN
NTAPI
PhHttpSocketReadData(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG BytesCopied
    );

PHLIBAPI
BOOLEAN
NTAPI
PhHttpSocketWriteData(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG BytesCopied
    );

PHLIBAPI
BOOLEAN
NTAPI
PhHttpSocketAddRequestHeaders(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PWSTR Headers,
    _In_opt_ ULONG HeadersLength
    );

PHLIBAPI
PPH_STRING
NTAPI
PhHttpSocketQueryHeaders(
    _In_ PPH_HTTP_CONTEXT HttpContext
    );

PHLIBAPI
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

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhHttpSocketQueryHeaderUlong(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ ULONG QueryValue,
    _Out_ PULONG HeaderValue
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhHttpSocketQueryHeaderUlong64(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ ULONG QueryValue,
    _Out_ PULONG64 HeaderValue
    );

PHLIBAPI
PPH_STRING
NTAPI
PhHttpSocketQueryOptionString(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ BOOLEAN SessionOption,
    _In_ ULONG QueryOption
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhHttpSocketReadDataToBuffer(
    _In_ PVOID RequestHandle,
    _Out_ PVOID* Buffer,
    _Out_ ULONG* BufferLength
    );

PHLIBAPI
PVOID
NTAPI
PhHttpSocketDownloadString(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ BOOLEAN Unicode
    );

typedef struct _PH_HTTPDOWNLOAD_CONTEXT
{
    ULONG64 ReadLength;
    ULONG64 TotalLength;
    ULONG64 BitsPerSecond;
    DOUBLE Percent;
} PH_HTTPDOWNLOAD_CONTEXT, *PPH_HTTPDOWNLOAD_CONTEXT;

typedef BOOLEAN (NTAPI *PPH_HTTPDOWNLOAD_CALLBACK)(
    _In_opt_ PPH_HTTPDOWNLOAD_CONTEXT Parameter,
    _In_opt_ PVOID Context
    );

PHLIBAPI
NTSTATUS
NTAPI
PhHttpSocketDownloadToFile(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PPH_STRINGREF FileName,
    _In_ PPH_HTTPDOWNLOAD_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

#define PH_HTTP_FEATURE_REDIRECTS 0x1
#define PH_HTTP_FEATURE_KEEP_ALIVE 0x2

PHLIBAPI
BOOLEAN
NTAPI
PhHttpSocketSetFeature(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ ULONG Feature,
    _In_ BOOLEAN Enable
    );

#define PH_HTTP_SECURITY_IGNORE_UNKNOWN_CA 0x1
#define PH_HTTP_SECURITY_IGNORE_CERT_DATE_INVALID 0x2

PHLIBAPI
BOOLEAN
NTAPI
PhHttpSocketSetSecurity(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ ULONG Feature
    );

_Success_(return)
PHLIBAPI
BOOLEAN
NTAPI
PhHttpSocketParseUrl(
    _In_ PPH_STRING Url,
    _Out_opt_ PPH_STRING *Host,
    _Out_opt_ PPH_STRING *Path,
    _Out_opt_ PUSHORT Port
    );

PHLIBAPI
PPH_STRING
NTAPI
PhHttpSocketGetErrorMessage(
    _In_ ULONG ErrorCode
    );

PHLIBAPI
BOOLEAN
NTAPI
PhHttpSocketSetCredentials(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PCWSTR Name,
    _In_ PCWSTR Value
    );

// DNS

PHLIBAPI
PDNS_RECORD
NTAPI
PhHttpDnsQuery(
    _In_opt_ PWSTR DnsServerAddress,
    _In_ PWSTR DnsQueryMessage,
    _In_ USHORT DnsQueryMessageType
    );

PHLIBAPI
PDNS_RECORD
NTAPI
PhDnsQuery(
    _In_opt_ PWSTR DnsServerAddress,
    _In_ PWSTR DnsQueryMessage,
    _In_ USHORT DnsQueryMessageType
    );

PHLIBAPI
PDNS_RECORD
NTAPI
PhDnsQuery2(
    _In_opt_ PWSTR DnsServerAddress,
    _In_ PWSTR DnsQueryMessage,
    _In_ USHORT DnsQueryMessageType,
    _In_ USHORT DnsQueryMessageOptions
    );

PHLIBAPI
VOID
NTAPI
PhDnsFree(
    _In_ PDNS_RECORD DnsRecordList
    );

EXTERN_C_END

#endif
