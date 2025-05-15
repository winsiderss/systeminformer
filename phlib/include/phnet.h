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

#ifndef __WINDOT11_H__
#define __WINDOT11_H__ // Workaround windot11.h C2288 - WinSDK 10.0.22621 (dmex)
#endif

#ifndef PIO_APC_ROUTINE_DEFINED
#define PIO_APC_ROUTINE_DEFINED 1
#endif

#ifndef ALIGN_SIZE
#define ALIGN_SIZE 0x00000008
#endif

#ifndef UM_NDIS60
#define UM_NDIS60 1
#endif

// #include <rtinfo.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ws2def.h>
#include <windns.h>
#include <nldef.h>
#include <netiodef.h>
#include <iphlpapi.h>
#include <mstcpip.h>
#include <icmpapi.h>
#include <hvsocket.h>

EXTERN_C CONST DECLSPEC_SELECTANY IN_ADDR  inaddr_any             = { 0x00 };
EXTERN_C CONST DECLSPEC_SELECTANY IN6_ADDR in6addr_any            = { 0x00 };
EXTERN_C CONST DECLSPEC_SELECTANY IN6_ADDR in6addr_v4mappedprefix = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 };

#define PH_NETWORK_TYPE_NONE 0x0
#define PH_NETWORK_TYPE_IPV4 0x1
#define PH_NETWORK_TYPE_IPV6 0x2
#define PH_NETWORK_TYPE_HYPERV 0x4
#define PH_NETWORK_TYPE_MASK 0x8

#define PH_PROTOCOL_TYPE_NONE 0x0
#define PH_PROTOCOL_TYPE_TCP  0x10
#define PH_PROTOCOL_TYPE_UDP  0x20
#define PH_PROTOCOL_TYPE_MASK 0x30

#define PH_NETWORK_PROTOCOL_NONE 0x0
#define PH_NETWORK_PROTOCOL_TCP4 (PH_NETWORK_TYPE_IPV4 | PH_PROTOCOL_TYPE_TCP)
#define PH_NETWORK_PROTOCOL_TCP6 (PH_NETWORK_TYPE_IPV6 | PH_PROTOCOL_TYPE_TCP)
#define PH_NETWORK_PROTOCOL_UDP4 (PH_NETWORK_TYPE_IPV4 | PH_PROTOCOL_TYPE_UDP)
#define PH_NETWORK_PROTOCOL_UDP6 (PH_NETWORK_TYPE_IPV6 | PH_PROTOCOL_TYPE_UDP)
#define PH_NETWORK_PROTOCOL_HYPERV (PH_NETWORK_TYPE_HYPERV)

typedef struct _PH_IP_ADDRESS
{
    ULONG Type;
    union
    {
        UCHAR Ipv4[4];
        IN_ADDR InAddr;
        UCHAR Ipv6[16];
        IN6_ADDR In6Addr;
        GUID HvAddr;
    };
} PH_IP_ADDRESS, *PPH_IP_ADDRESS;

typedef struct _PH_IP_ENDPOINT
{
    PH_IP_ADDRESS Address;
    ULONG Port;
} PH_IP_ENDPOINT, *PPH_IP_ENDPOINT;

FORCEINLINE
BOOLEAN
PhEqualIpAddress(
    _In_ PPH_IP_ADDRESS Address1,
    _In_ PPH_IP_ADDRESS Address2
    )
{
    if ((Address1->Type | Address2->Type) == 0) // don't check addresses if both are invalid
        return TRUE;
    if (Address1->Type != Address2->Type)
        return FALSE;

    switch (Address1->Type)
    {
    case PH_NETWORK_TYPE_IPV4:
        {
            return IN4_ADDR_EQUAL(&Address1->InAddr, &Address2->InAddr);
        }
        break;
    case PH_NETWORK_TYPE_IPV6:
        {
            return IN6_ADDR_EQUAL(&Address1->In6Addr, &Address2->In6Addr);
//#ifdef _WIN64
//            return
//                *(PULONG64)(Address1->Ipv6) == *(PULONG64)(Address2->Ipv6) &&
//                *(PULONG64)(Address1->Ipv6 + 8) == *(PULONG64)(Address2->Ipv6 + 8);
//#else
//            return
//                *(PULONG)(Address1->Ipv6) == *(PULONG)(Address2->Ipv6) &&
//                *(PULONG)(Address1->Ipv6 + 4) == *(PULONG)(Address2->Ipv6 + 4) &&
//                *(PULONG)(Address1->Ipv6 + 8) == *(PULONG)(Address2->Ipv6 + 8) &&
//                *(PULONG)(Address1->Ipv6 + 12) == *(PULONG)(Address2->Ipv6 + 12);
//#endif
        }
        break;
    case PH_NETWORK_TYPE_HYPERV:
        {
            //return IsEqualGUID(&Address1->HvAddr, &Address2->HvAddr);
            return !memcmp(&Address1->HvAddr, &Address2->HvAddr, sizeof(GUID));
        }
        break;
    }

    return FALSE;
}

FORCEINLINE
ULONG
PhHashIpAddress(
    _In_ PPH_IP_ADDRESS Address
    )
{
    if (Address->Type == PH_NETWORK_TYPE_NONE)
        return 0;

    switch (Address->Type)
    {
    case PH_NETWORK_TYPE_IPV4:
        {
            ULONG hash;

            hash = Address->Type | (Address->Type << 16);
            hash ^= *(PULONG)(Address->Ipv4);

            return hash;
        }
        break;
    case PH_NETWORK_TYPE_IPV6:
        {
            ULONG hash;

            hash = Address->Type | (Address->Type << 16);
            hash += *(PULONG)(Address->Ipv6);
            hash ^= *(PULONG)(Address->Ipv6 + 4);
            hash += *(PULONG)(Address->Ipv6 + 8);
            hash ^= *(PULONG)(Address->Ipv6 + 12);

            return hash;
        }
        break;
    case PH_NETWORK_TYPE_HYPERV:
        {
            ULONG hash;

            hash = Address->Type | (Address->Type << 16);
            hash += Address->HvAddr.Data1;
            hash ^= *(PULONG)(&Address->HvAddr.Data2);
            hash ^= *(PULONG)(Address->HvAddr.Data4);
            hash ^= *(PULONG)(Address->HvAddr.Data4 + 4);

            return hash;
        }
        break;
    }

    return 0;
}

FORCEINLINE
BOOLEAN
PhIsNullIpAddress(
    _In_ PPH_IP_ADDRESS Address
    )
{
    if (Address->Type == PH_NETWORK_TYPE_NONE)
        return TRUE;

    switch (Address->Type)
    {
    case PH_NETWORK_TYPE_IPV4:
        {
            return IN4_IS_ADDR_UNSPECIFIED(&Address->InAddr);
        }
        break;
    case PH_NETWORK_TYPE_IPV6:
        {
            return IN6_IS_ADDR_UNSPECIFIED(&Address->In6Addr);
//#ifdef _WIN64
//            return (*(PULONG64)(Address->Ipv6) | *(PULONG64)(Address->Ipv6 + 8)) == 0;
//#else
//            return (*(PULONG)(Address->Ipv6) | *(PULONG)(Address->Ipv6 + 4) |
//                *(PULONG)(Address->Ipv6 + 8) | *(PULONG)(Address->Ipv6 + 12)) == 0;
//#endif
        }
        break;
    case PH_NETWORK_TYPE_HYPERV:
        {
            //return IsEqualGUID(&Address->HvAddr, &HV_GUID_ZERO);
            return !memcmp(&Address->HvAddr, &HV_GUID_ZERO, sizeof(GUID));
        }
        break;
    }

    return TRUE;
}

FORCEINLINE
BOOLEAN
PhEqualIpEndpoint(
    _In_ PPH_IP_ENDPOINT Endpoint1,
    _In_ PPH_IP_ENDPOINT Endpoint2
    )
{
    return PhEqualIpAddress(&Endpoint1->Address, &Endpoint2->Address) &&
        Endpoint1->Port == Endpoint2->Port;
}

FORCEINLINE
ULONG
PhHashIpEndpoint(
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

PHLIBAPI
NTSTATUS
NTAPI
PhHttpInitialize(
    _Out_ PPH_HTTP_CONTEXT *HttpContext
    );

PHLIBAPI
VOID
NTAPI
PhHttpDestroy(
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
PhHttpClose(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PH_HTTP_SOCKET_CLOSE_TYPE Type
    );

#define PH_HTTP_DEFAULT_PORT 0 // use the protocol-specific default port
#define PH_HTTP_DEFAULT_HTTP_PORT 80
#define PH_HTTP_DEFAULT_HTTPS_PORT 443

PHLIBAPI
NTSTATUS
NTAPI
PhHttpConnect(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PCWSTR ServerName,
    _In_ USHORT ServerPort
    );

#define PH_HTTP_FLAG_SECURE 0x1
#define PH_HTTP_FLAG_REFRESH 0x2

PHLIBAPI
NTSTATUS
NTAPI
PhHttpBeginRequest(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_opt_ PCWSTR RequestMethod,
    _In_ PCWSTR RequestPath,
    _In_ ULONG Flags
    );

PHLIBAPI
NTSTATUS
NTAPI
PhHttpSendRequest(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_opt_ PVOID OptionalBuffer,
    _In_opt_ ULONG OptionalLength,
    _In_opt_ ULONG TotalLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhHttpReceiveResponse(
    _In_ PPH_HTTP_CONTEXT HttpContext
    );

PHLIBAPI
NTSTATUS
NTAPI
PhHttpReadData(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG BytesCopied
    );

PHLIBAPI
NTSTATUS
NTAPI
PhHttpWriteData(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG BytesCopied
    );

PHLIBAPI
NTSTATUS
NTAPI
PhHttpAddRequestHeaders(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PCWSTR Headers,
    _In_opt_ ULONG HeadersLength
    );

FORCEINLINE
NTSTATUS
PhHttpAddRequestHeadersSR(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PCPH_STRINGREF Headers
    )
{
    return PhHttpAddRequestHeaders(
        HttpContext,
        Headers->Buffer,
        (ULONG)Headers->Length / sizeof(WCHAR)
        );
}

PHLIBAPI
PPH_STRING
NTAPI
PhHttpQueryHeaders(
    _In_ PPH_HTTP_CONTEXT HttpContext
    );

PHLIBAPI
PPH_STRING
NTAPI
PhHttpQueryHeaderString(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PCWSTR HeaderString
    );

// status codes
#define PH_HTTP_STATUS_OK 200 // request completed
#define PH_HTTP_STATUS_CREATED 201
#define PH_HTTP_STATUS_REDIRECT_METHOD 303 // redirection w/ new access method
#define PH_HTTP_STATUS_REDIRECT 302 // object temporarily moved

// header query flags
#define PH_HTTP_QUERY_CONTENT_LENGTH 0x1
#define PH_HTTP_QUERY_STATUS_CODE 0x2

PHLIBAPI
NTSTATUS
NTAPI
PhHttpQueryHeaderUlong(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ ULONG QueryValue,
    _Out_ PULONG HeaderValue
    );

PHLIBAPI
NTSTATUS
NTAPI
PhHttpQueryHeaderUlong64(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ ULONG QueryValue,
    _Out_ PULONG64 HeaderValue
    );

FORCEINLINE
NTSTATUS
NTAPI
PhHttpQueryResponseStatus(
    _In_ PPH_HTTP_CONTEXT HttpContext
    )
{
    NTSTATUS status;
    ULONG httpStatus = PH_HTTP_STATUS_OK;

    status = PhHttpQueryHeaderUlong(HttpContext, PH_HTTP_QUERY_STATUS_CODE, &httpStatus);

    if (NT_SUCCESS(status))
    {
        switch (httpStatus)
        {
        case 301:
        case 308:
            //status = STATUS_MORE_ENTRIES;
        case PH_HTTP_STATUS_OK:
            status = STATUS_SUCCESS;
            break;
        case 401:
        case 403:
            status = STATUS_ACCESS_DENIED;
            break;
        case 404:
            status = STATUS_OBJECT_NAME_NOT_FOUND;
            break;
        default:
            status = STATUS_UNEXPECTED_NETWORK_ERROR;
            break;
        }
    }

    return status;
}

PHLIBAPI
PPH_STRING
NTAPI
PhHttpQueryOptionString(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ BOOLEAN SessionOption,
    _In_ ULONG QueryOption
    );

PHLIBAPI
NTSTATUS
NTAPI
PhHttpReadDataToBuffer(
    _In_ PVOID RequestHandle,
    _Out_ PVOID* Buffer,
    _Out_ ULONG* BufferLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhHttpDownloadString(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ BOOLEAN Unicode,
    _Out_ PVOID* StringBuffer
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
PhHttpDownloadToFile(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PPH_STRINGREF FileName,
    _In_ PPH_HTTPDOWNLOAD_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

#define PH_HTTP_FEATURE_REDIRECTS 0x1
#define PH_HTTP_FEATURE_KEEP_ALIVE 0x2

PHLIBAPI
NTSTATUS
NTAPI
PhHttpSetFeature(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ ULONG Feature,
    _In_ BOOLEAN Enable
    );

PHLIBAPI
NTSTATUS
NTAPI
PhHttpSetOption(
    _In_ PVOID HttpHandle,
    _In_ ULONG Option,
    _In_ ULONG Value
    );

PHLIBAPI
NTSTATUS
NTAPI
PhHttpSetOptionString(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ ULONG Option,
    _In_ PPH_STRINGREF Value
    );

typedef _Function_class_(PH_HTTP_STATUS_CALLBACK)
VOID CALLBACK PH_HTTP_STATUS_CALLBACK(
    _In_ PVOID InternetHandle,
    _In_ ULONG_PTR Context,
    _In_ ULONG InternetStatus,
    _In_opt_ LPVOID StatusInformation,
    _In_ ULONG StatusInformationLength
    );
typedef PH_HTTP_STATUS_CALLBACK* PPH_HTTP_STATUS_CALLBACK;

PHLIBAPI
NTSTATUS
NTAPI
PhHttpSetCallback(
    _In_ PVOID HttpHandle,
    _In_ PPH_HTTP_STATUS_CALLBACK Callback,
    _In_ ULONG NotificationFlags
    );

PHLIBAPI
NTSTATUS
NTAPI
PhHttpSetContext(
    _In_ PVOID HttpHandle,
    _In_ PVOID Context
    );

FORCEINLINE
NTSTATUS
PhWinHttpSetOptionStringZ(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ ULONG Option,
    _In_ PCWSTR Value
    )
{
    PH_STRINGREF string;

    PhInitializeStringRef(&string, Value);

    return PhHttpSetOptionString(HttpContext, Option, &string);
}

#define PH_HTTP_SECURITY_IGNORE_UNKNOWN_CA 0x1
#define PH_HTTP_SECURITY_IGNORE_CERT_DATE_INVALID 0x2

PHLIBAPI
NTSTATUS
NTAPI
PhHttpSetSecurity(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ ULONG Feature
    );

#define PH_HTTP_PROTOCOL_FLAG_HTTP2 0x1
#define PH_HTTP_PROTOCOL_FLAG_HTTP3 0x2

PHLIBAPI
NTSTATUS
NTAPI
PhHttpSetProtocol(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ BOOLEAN Session,
    _In_ ULONG Protocol,
    _In_ ULONG Timeout
    );

PHLIBAPI
NTSTATUS
NTAPI
PhHttpCrackUrl(
    _In_ PPH_STRING Url,
    _Out_opt_ PPH_STRING *Host,
    _Out_opt_ PPH_STRING *Path,
    _Out_opt_ PUSHORT Port
    );

PHLIBAPI
PPH_STRING
NTAPI
PhHttpGetErrorMessage(
    _In_ ULONG ErrorCode
    );

typedef struct _PH_HTTP_CERTIFICATE_INFO
{
    LARGE_INTEGER Expiry;
    LARGE_INTEGER Start;
    PWSTR SubjectInfo;
    PWSTR IssuerInfo;
    PWSTR ProtocolName;
    PWSTR SignatureAlgName;
    PWSTR EncryptionAlgName;
    ULONG KeySize;
} PH_HTTP_CERTIFICATE_INFO, *PPH_HTTP_CERTIFICATE_INFO;

PHLIBAPI
NTSTATUS
NTAPI
PhHttpGetCertificateInfo(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _Out_ PPH_HTTP_CERTIFICATE_INFO Certificate
    );

PHLIBAPI
NTSTATUS
NTAPI
PhHttpGetStatistics(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _Out_writes_bytes_to_opt_(*BufferLength, *BufferLength) PVOID Buffer,
    _Inout_ PULONG BufferLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhHttpSetCredentials(
    _In_ PPH_HTTP_CONTEXT HttpContext,
    _In_ PCWSTR Name,
    _In_ PCWSTR Value
    );

//
// IPv4 and IPv6
//

PHLIBAPI
NTSTATUS
NTAPI
PhIpv4AddressToString(
    _In_ PCIN_ADDR Address,
    _In_ USHORT Port,
    _Out_writes_to_(*AddressStringLength, *AddressStringLength) PWSTR AddressString,
    _Inout_ PULONG AddressStringLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhIpv4StringToAddress(
    _In_ PCWSTR AddressString,
    _In_ BOOLEAN Strict,
    _Out_ PIN_ADDR Address,
    _Out_ PUSHORT Port
    );

PHLIBAPI
NTSTATUS
NTAPI
PhIpv6AddressToString(
    _In_ PCIN6_ADDR Address,
    _In_ ULONG ScopeId,
    _In_ USHORT Port,
    _Out_writes_to_(*AddressStringLength, *AddressStringLength) PWSTR AddressString,
    _Inout_ PULONG AddressStringLength
    );

PHLIBAPI
NTSTATUS
NTAPI
PhIpv6StringToAddress(
    _In_ PCWSTR AddressString,
    _Out_ PIN6_ADDR Address,
    _Out_ PULONG ScopeId,
    _Out_ PUSHORT Port
    );

//
// DNS
//

PHLIBAPI
PPH_STRING
NTAPI
PhDnsReverseLookupNameFromAddress(
    _In_ ULONG Type,
    _In_ PVOID Address
    );

PHLIBAPI
NTSTATUS
NTAPI
PhHttpDnsQuery(
    _In_opt_ PCWSTR DnsServerAddress,
    _In_ PCWSTR DnsQueryMessage,
    _In_ USHORT DnsQueryMessageType,
    _Out_ PDNS_RECORD* DnsQueryRecord
    );

PHLIBAPI
PDNS_RECORD
NTAPI
PhDnsQuery(
    _In_opt_ PCWSTR DnsServerAddress,
    _In_ PCWSTR DnsQueryMessage,
    _In_ USHORT DnsQueryMessageType
    );

PHLIBAPI
PDNS_RECORD
NTAPI
PhDnsQuery2(
    _In_opt_ PCWSTR DnsServerAddress,
    _In_ PCWSTR DnsQueryMessage,
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
