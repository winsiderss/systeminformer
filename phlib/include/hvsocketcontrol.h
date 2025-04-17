/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023
 *
 */

#ifndef _PH_HVSOCKETCONTROL_H
#define _PH_HVSOCKETCONTROL_H

#include <hvsocket.h>

EXTERN_C_START

#ifdef _WIN64

PHLIBAPI
NTSTATUS
NTAPI
PhHvSocketOpenSystemControl(
    _Out_ PHANDLE SystemHandle,
    _In_opt_ const GUID* VmId
    );

// rev
typedef struct _HVSOCKET_LISTENER
{
    union // HV_GUID_VSOCK_TEMPLATE
    {
        ULONG Port;
        GUID ServiceId;
    };
    GUID VmId;
    ULONG ProcessId;
    LARGE_INTEGER TimeStamp;
    BYTE Unknown; // Type?
} HVSOCKET_LISTENER, *PHVSOCKET_LISTENER;
C_ASSERT(FIELD_OFFSET(HVSOCKET_LISTENER, ProcessId) == 32);
C_ASSERT(sizeof(HVSOCKET_LISTENER) == 56);

// rev
typedef struct _HVSOCKET_LISTENERS
{
    ULONG Count;
    HVSOCKET_LISTENER Listener[ANYSIZE_ARRAY];
} HVSOCKET_LISTENERS, *PHVSOCKET_LISTENERS;
C_ASSERT(FIELD_OFFSET(HVSOCKET_LISTENERS, Listener) == 8);
C_ASSERT(sizeof(HVSOCKET_LISTENERS) == 64);

PHLIBAPI
NTSTATUS
NTAPI
PhHvSocketGetListeners(
    _In_ HANDLE SystemHandle,
    _In_ const GUID* VmId,
    _In_opt_ PHVSOCKET_LISTENERS Listeners,
    _In_ ULONG ListenersLength,
    _Out_opt_ PULONG ReturnLength
    );

// rev
typedef struct _HVSOCKET_CONNECTION
{
    union // HV_GUID_VSOCK_TEMPLATE
    {
        ULONG Port;
        GUID ServiceId;
    };
    GUID VmId;
    ULONG ProcessId;
    LARGE_INTEGER TimeStamp;
    ULONG Type; // 1 or 2?
} HVSOCKET_CONNECTION, *PHVSOCKET_CONNECTION;
C_ASSERT(FIELD_OFFSET(HVSOCKET_CONNECTION, ProcessId) == 32);
C_ASSERT(sizeof(HVSOCKET_CONNECTION) == 56);

// rev
typedef struct _HVSOCKET_CONNECTIONS
{
    ULONG Count;
    HVSOCKET_CONNECTION Connection[ANYSIZE_ARRAY];
} HVSOCKET_CONNECTIONS, *PHVSOCKET_CONNECTIONS;
C_ASSERT(FIELD_OFFSET(HVSOCKET_CONNECTIONS, Connection) == 8);
C_ASSERT(sizeof(HVSOCKET_CONNECTIONS) == 64);

PHLIBAPI
NTSTATUS
NTAPI
PhHvSocketGetConnections(
    _In_ HANDLE SystemHandle,
    _In_ const GUID* VmId,
    _In_opt_ PHVSOCKET_CONNECTIONS Connections,
    _In_ ULONG ConnectionsLength,
    _Out_opt_ PULONG ReturnLength
    );

// rev
typedef struct _HVSOCKET_SERVICE_INFO
{
    GUID Unknown1;
    ULONG Unknown2;
    ULONG Unknown3;
} HVSOCKET_SERVICE_INFO, *PHVSOCKET_SERVICE_INFO;
C_ASSERT(sizeof(HVSOCKET_SERVICE_INFO) == 24);

PHLIBAPI
NTSTATUS
NTAPI
PhHvSocketGetServiceInfo(
    _In_ HANDLE SystemHandle,
    _In_ const GUID* ServiceId,
    _Out_ PHVSOCKET_SERVICE_INFO ServiceInfo
    );

#endif // _WIN64

PHLIBAPI
BOOLEAN
NTAPI
PhHvSocketIsVSockTemplate(
    _In_ PGUID ServiceId
    );

PHLIBAPI
_Maybenull_
PPH_STRING
NTAPI
PhHvSocketGetVmName(
    _In_ PGUID VmId
    );

PHLIBAPI
_Maybenull_
PPH_STRING
NTAPI
PhHvSocketGetServiceName(
    _In_ PGUID ServiceId
    );

PHLIBAPI
PPH_STRING
NTAPI
PhHvSocketAddressString(
    _In_ PGUID Address
    );

EXTERN_C_END

#endif // _PH_HVSOCKETCONTROL_H

