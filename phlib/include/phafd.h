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

#include <ph.h>
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

NTSTATUS
NTAPI
PhAfdQueryOption(
    _In_ HANDLE SocketHandle,
    _In_ ULONG Level,
    _In_ ULONG OptionName,
    _Out_ PULONG OptionValue
    );

NTSTATUS
NTAPI
PhAfdQueryTdiHandle(
    _In_ HANDLE SocketHandle,
    _In_ ULONG QueryMode,
    _Out_ PHANDLE TdiHandle
    );

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

NTSTATUS
NTAPI
PhAfdQueryFormatAddress(
    _In_ HANDLE SocketHandle,
    _In_ BOOLEAN Remote,
    _Out_ PPH_STRING *AddressString,
    _In_ BOOLEAN Simplify
    );

PPH_STRING
NTAPI
PhAfdFormatSocketState(
    _In_ SOCKET_STATE SocketState
    );

PPH_STRING
NTAPI
PhAfdFormatSocketType(
    _In_ LONG SocketType
    );

PPH_STRING
NTAPI
PhAfdFormatAddressFamily(
    _In_ LONG AddressFamily
    );

PPH_STRING
NTAPI
PhAfdFormatProtocol(
    _In_ LONG AddressFamily,
    _In_ LONG Protocol
    );

PPH_STRING
NTAPI
PhAfdFormatProviderFlags(
    _In_ ULONG ProviderFlags
    );

PPH_STRING
NTAPI
PhAfdFormatServiceFlags(
    _In_ ULONG ServiceFlags
    );

PPH_STRING
NTAPI
PhAfdFormatCreationFlags(
    _In_ ULONG CreationFlags
    );

PPH_STRING
NTAPI
PhAfdFormatSharedInfoFlags(
    _In_ PSOCK_SHARED_INFO SharedInfo
    );

_Maybenull_
PPH_STRING
NTAPI
PhAfdFormatSocketBestName(
    _In_ HANDLE SocketHandle
    );

EXTERN_C_END

#endif _PH_PHAFD_H
