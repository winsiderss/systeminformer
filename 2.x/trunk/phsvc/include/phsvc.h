#ifndef PHSVC_H
#define PHSVC_H

#include <ph.h>
#include <phsvcapi.h>

#define PHSVC_PORT_NAME (L"\\BaseNamedObjects\\PhSvcApiPort")

typedef enum _PHSVC_ACCESS_MODE
{
    PhSvcServerMode,
    PhSvcClientMode
} PHSVC_ACCESS_MODE, *PPHSVC_ACCESS_MODE;

// client

typedef struct _PHSVC_CLIENT
{
    LIST_ENTRY ListEntry;

    CLIENT_ID ClientId;
    HANDLE PortHandle;

    PPH_HANDLE_TABLE HandleTable;
} PHSVC_CLIENT, *PPHSVC_CLIENT;

NTSTATUS PhSvcClientInitialization();

PPHSVC_CLIENT PhSvcCreateClient(
    __in_opt PCLIENT_ID ClientId
    );

PPHSVC_CLIENT PhSvcReferenceClientByClientId(
    __in PCLIENT_ID ClientId
    );

PPHSVC_CLIENT PhSvcGetCurrentClient();

BOOLEAN PhSvcAttachClient(
    __in PPHSVC_CLIENT Client
    );

VOID PhSvcDetachClient(
    __in PPHSVC_CLIENT Client
    );

NTSTATUS PhSvcCreateHandle(
    __out PHANDLE Handle,
    __in PVOID Object,
    __in ACCESS_MASK GrantedAccess
    );

NTSTATUS PhSvcCloseHandle(
    __in HANDLE Handle
    );

NTSTATUS PhSvcReferenceObjectByHandle(
    __out PPVOID Object,
    __in HANDLE Handle,
    __in_opt PPH_OBJECT_TYPE ObjectType,
    __in_opt ACCESS_MASK DesiredAccess,
    __in PHSVC_ACCESS_MODE AccessMode
    );

// apiport

typedef struct _PHSVC_THREAD_CONTEXT
{
    PPHSVC_CLIENT CurrentClient;
    PPHSVC_CLIENT OldClient;
} PHSVC_THREAD_CONTEXT, *PPHSVC_THREAD_CONTEXT;

NTSTATUS PhApiPortInitialization();

PPHSVC_THREAD_CONTEXT PhSvcGetCurrentThreadContext();

VOID PhHandleConnectionRequest(
    __in PPHSVC_API_MSG Message
    );

// api

VOID PhSvcApiInitialization();

typedef NTSTATUS (NTAPI *PPHSVC_API_PROCEDURE)(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    );

VOID PhSvcDispatchApiCall(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message,
    __out PPHSVC_API_MSG *ReplyMessage,
    __out PHANDLE ReplyPortHandle
    );

NTSTATUS PhSvcApiClose(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiTestOpen(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    );

#endif
