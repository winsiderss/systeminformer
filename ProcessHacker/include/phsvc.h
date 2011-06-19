#ifndef PHSVC_H
#define PHSVC_H

#include <phsvcapi.h>

#define PHSVC_SHARED_SECTION_SIZE (512 * 1024)

// svcmain

typedef struct _PHSVC_STOP
{
    BOOLEAN Stop;
    HANDLE Event1;
    HANDLE Event2;
} PHSVC_STOP, *PPHSVC_STOP;

NTSTATUS PhSvcMain(
    __in_opt PPH_STRINGREF PortName,
    __in_opt PLARGE_INTEGER Timeout,
    __inout_opt PPHSVC_STOP Stop
    );

// svcclient

typedef struct _PHSVC_CLIENT
{
    LIST_ENTRY ListEntry;

    CLIENT_ID ClientId;
    HANDLE PortHandle;
    PVOID ClientViewBase;
    PVOID ClientViewLimit;

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
    __in HANDLE Handle,
    __in_opt PPH_OBJECT_TYPE ObjectType,
    __in_opt ACCESS_MASK DesiredAccess,
    __out PVOID *Object
    );

// svcapiport

typedef struct _PHSVC_THREAD_CONTEXT
{
    PPHSVC_CLIENT CurrentClient;
    PPHSVC_CLIENT OldClient;
} PHSVC_THREAD_CONTEXT, *PPHSVC_THREAD_CONTEXT;

NTSTATUS PhSvcApiPortInitialization(
    __in PPH_STRINGREF PortName
    );

PPHSVC_THREAD_CONTEXT PhSvcGetCurrentThreadContext();

VOID PhSvcHandleConnectionRequest(
    __in PPHSVC_API_MSG Message
    );

// svcapi

NTSTATUS PhSvcApiInitialization();

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

NTSTATUS PhSvcCaptureBuffer(
    __in PPH_RELATIVE_STRINGREF String,
    __in BOOLEAN AllowNull,
    __out PVOID *CapturedBuffer
    );

NTSTATUS PhSvcCaptureString(
    __in PPH_RELATIVE_STRINGREF String,
    __in BOOLEAN AllowNull,
    __out PPH_STRING *CapturedString
    );

NTSTATUS PhSvcCaptureSid(
    __in PPH_RELATIVE_STRINGREF String,
    __in BOOLEAN AllowNull,
    __out PSID *CapturedSid
    );

NTSTATUS PhSvcApiClose(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiExecuteRunAsCommand(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiUnloadDriver(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiControlProcess(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiControlService(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiCreateService(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiChangeServiceConfig(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiChangeServiceConfig2(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiSetTcpEntry(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiControlThread(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiAddAccountRight(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiInvokeRunAsService(
    __in PPHSVC_CLIENT Client,
    __inout PPHSVC_API_MSG Message
    );

#endif
