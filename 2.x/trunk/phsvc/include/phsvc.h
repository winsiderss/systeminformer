#ifndef PHSVC_H
#define PHSVC_H

#include <ph.h>
#include <phsvcapi.h>

#define PHSVC_PORT_NAME (L"\\BaseNamedObjects\\PhSvcApiPort")

// client

typedef struct _PHSVC_CLIENT
{
    LIST_ENTRY ListEntry;

    CLIENT_ID ClientId;
    HANDLE PortHandle;
} PHSVC_CLIENT, *PPHSVC_CLIENT;

NTSTATUS PhSvcClientInitialization();

PPHSVC_CLIENT PhSvcCreateClient(
    __in_opt PCLIENT_ID ClientId
    );

// apiport

NTSTATUS PhApiPortInitialization();

VOID PhHandleConnectionRequest(
    __in PPHSVC_API_MSG Message
    );

#endif
