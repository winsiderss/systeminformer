/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     diversenok   2025
 *
 */

#ifndef _PH_PHAFD_H
#define _PH_PHAFD_H

#include <ntafd.h>

EXTERN_C_START

PHLIBAPI
BOOLEAN
NTAPI
PhAfdIsSocketObjectName(
    _In_opt_ PPH_STRING ObjectName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhAfdIsSocketHandle(
    _In_ HANDLE Handle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhAfdDeviceIoControl(
    _In_ HANDLE SocketHandle,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_(InBufferSize) PVOID InBuffer,
    _In_ ULONG InBufferSize,
    _Out_writes_bytes_to_opt_(OutputBufferSize, *BytesReturned) PVOID OutputBuffer,
    _In_ ULONG OutputBufferSize,
    _Out_opt_ PULONG BytesReturned,
    _Inout_opt_ LPOVERLAPPED Overlapped
    );

PHLIBAPI
NTSTATUS
NTAPI
PhAfdQuerySharedInfo(
    _In_ HANDLE SocketHandle,
    _Out_ PSOCK_SHARED_INFO SharedInfo
    );

PHLIBAPI
NTSTATUS
NTAPI
PhAfdQuerySimpleInfo(
    _In_ HANDLE SocketHandle,
    _In_ ULONG InformationType,
    _Out_ PAFD_INFORMATION Information
    );

PHLIBAPI
NTSTATUS
NTAPI
PhAfdQueryOption(
    _In_ HANDLE SocketHandle,
    _In_ ULONG Level,
    _In_ ULONG OptionName,
    _Out_ PULONG OptionValue
    );

typedef struct _TCP_INFO_v2 TCP_INFO_v2, *PTCP_INFO_v2;

PHLIBAPI
NTSTATUS
NTAPI
PhAfdQueryTcpInfo(
    _In_ HANDLE SocketHandle,
    _Out_ PTCP_INFO_v2 TcpInfo,
    _Out_ PULONG TcpInfoVersion
    );

PHLIBAPI
NTSTATUS
NTAPI
PhAfdQueryTdiHandle(
    _In_ HANDLE SocketHandle,
    _In_ ULONG QueryMode,
    _Out_ PHANDLE TdiHandle
    );

PHLIBAPI
NTSTATUS
NTAPI
PhAfdQueryFormatTdiDeviceName(
    _In_ HANDLE SocketHandle,
    _In_ ULONG QueryMode,
    _Outptr_ PPH_STRING* TdiDeviceName
    );

PHLIBAPI
NTSTATUS
NTAPI
PhAfdQueryAddress(
    _In_ HANDLE SocketHandle,
    _In_ BOOLEAN Remote,
    _Out_ PSOCKADDR_STORAGE Address
    );

// Address formatting flags
#define PH_AFD_ADDRESS_SIMPLIFY 0x01 // Print addresses as human-readable

PHLIBAPI
NTSTATUS
NTAPI
PhAfdQueryFormatAddress(
    _In_ HANDLE SocketHandle,
    _In_ BOOLEAN Remote,
    _Out_ PPH_STRING *AddressString,
    _In_ ULONG Flags
    );

PHLIBAPI
PPH_STRING
NTAPI
PhAfdFormatSocketState(
    _In_ SOCKET_STATE SocketState
    );

PHLIBAPI
PPH_STRING
NTAPI
PhAfdFormatSocketType(
    _In_ LONG SocketType
    );

PHLIBAPI
PPH_STRING
NTAPI
PhAfdFormatAddressFamily(
    _In_ LONG AddressFamily
    );

PHLIBAPI
PPH_STRING
NTAPI
PhAfdFormatProtocol(
    _In_ LONG AddressFamily,
    _In_ LONG Protocol
    );

PHLIBAPI
PPH_STRING
NTAPI
PhAfdFormatProviderFlags(
    _In_ ULONG ProviderFlags
    );

PHLIBAPI
PPH_STRING
NTAPI
PhAfdFormatServiceFlags(
    _In_ ULONG ServiceFlags
    );

PHLIBAPI
PPH_STRING
NTAPI
PhAfdFormatCreationFlags(
    _In_ ULONG CreationFlags
    );

PHLIBAPI
PPH_STRING
NTAPI
PhAfdFormatSharedInfoFlags(
    _In_ PSOCK_SHARED_INFO SharedInfo
    );

PHLIBAPI
PPH_STRING
NTAPI
PhAfdFormatGroupType(
    _In_ AFD_GROUP_TYPE GroupType
    );

PHLIBAPI
PPH_STRING
NTAPI
PhAfdFormatProtectionLevel(
    _In_ ULONG ProtectionLevel
    );

PHLIBAPI
PPH_STRING
NTAPI
PhAfdFormatMtuDiscoveryMode(
    _In_ ULONG MtuDiscover
    );

typedef enum _TCPSTATE TCPSTATE;

PHLIBAPI
PPH_STRING
NTAPI
PhAfdFormatTcpState(
    _In_ TCPSTATE TcpState
    );

PHLIBAPI
PPH_STRING
NTAPI
PhAfdFormatInterfaceOption(
    _In_ ULONG Interface
    );

_Maybenull_
PPH_STRING
NTAPI
PhAfdFormatSocketBestName(
    _In_ HANDLE SocketHandle
    );

EXTERN_C_END

#endif // _PH_PHAFD_H
