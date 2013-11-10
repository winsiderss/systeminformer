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
    _In_opt_ PUNICODE_STRING PortName,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Inout_opt_ PPHSVC_STOP Stop
    );

VOID PhSvcStop(
    _Inout_ PPHSVC_STOP Stop
    );

// svcclient

typedef struct _PHSVC_CLIENT
{
    LIST_ENTRY ListEntry;

    CLIENT_ID ClientId;
    HANDLE PortHandle;
    PVOID ClientViewBase;
    PVOID ClientViewLimit;
} PHSVC_CLIENT, *PPHSVC_CLIENT;

NTSTATUS PhSvcClientInitialization(
    VOID
    );

PPHSVC_CLIENT PhSvcCreateClient(
    _In_opt_ PCLIENT_ID ClientId
    );

PPHSVC_CLIENT PhSvcReferenceClientByClientId(
    _In_ PCLIENT_ID ClientId
    );

PPHSVC_CLIENT PhSvcGetCurrentClient(
    VOID
    );

BOOLEAN PhSvcAttachClient(
    _In_ PPHSVC_CLIENT Client
    );

VOID PhSvcDetachClient(
    _In_ PPHSVC_CLIENT Client
    );

// svcapiport

typedef struct _PHSVC_THREAD_CONTEXT
{
    PPHSVC_CLIENT CurrentClient;
    PPHSVC_CLIENT OldClient;
} PHSVC_THREAD_CONTEXT, *PPHSVC_THREAD_CONTEXT;

NTSTATUS PhSvcApiPortInitialization(
    _In_ PUNICODE_STRING PortName
    );

PPHSVC_THREAD_CONTEXT PhSvcGetCurrentThreadContext(
    VOID
    );

VOID PhSvcHandleConnectionRequest(
    _In_ PPHSVC_API_MSG Message
    );

// svcapi

NTSTATUS PhSvcApiInitialization(
    VOID
    );

typedef NTSTATUS (NTAPI *PPHSVC_API_PROCEDURE)(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_MSG Message
    );

VOID PhSvcDispatchApiCall(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_MSG Message,
    _Out_ PPHSVC_API_MSG *ReplyMessage,
    _Out_ PHANDLE ReplyPortHandle
    );

NTSTATUS PhSvcCaptureBuffer(
    _In_ PPH_RELATIVE_STRINGREF String,
    _In_ BOOLEAN AllowNull,
    _Out_ PVOID *CapturedBuffer
    );

NTSTATUS PhSvcCaptureString(
    _In_ PPH_RELATIVE_STRINGREF String,
    _In_ BOOLEAN AllowNull,
    _Out_ PPH_STRING *CapturedString
    );

NTSTATUS PhSvcCaptureSid(
    _In_ PPH_RELATIVE_STRINGREF String,
    _In_ BOOLEAN AllowNull,
    _Out_ PSID *CapturedSid
    );

NTSTATUS PhSvcApiDefault(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiExecuteRunAsCommand(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiUnloadDriver(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiControlProcess(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiControlService(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiCreateService(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiChangeServiceConfig(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiChangeServiceConfig2(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiSetTcpEntry(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiControlThread(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiAddAccountRight(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiInvokeRunAsService(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiIssueMemoryListCommand(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiPostMessage(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_MSG Message
    );

NTSTATUS PhSvcApiSendMessage(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_MSG Message
    );

#endif
