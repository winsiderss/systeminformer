#ifndef PH_PHSVC_H
#define PH_PHSVC_H

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
    _In_opt_ PPH_STRING PortName,
    _Inout_opt_ PPHSVC_STOP Stop
    );

VOID PhSvcStop(
    _Inout_ PPHSVC_STOP Stop
    );

// svcclient

typedef struct _PHSVC_CLIENT
{
    LIST_ENTRY ListEntry;

    PH_EVENT ReadyEvent;
    CLIENT_ID ClientId;
    HANDLE PortHandle;
    PVOID ClientViewBase;
    PVOID ClientViewLimit;
} PHSVC_CLIENT, *PPHSVC_CLIENT;

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
    _In_ PPORT_MESSAGE PortMessage
    );

// svcapi

typedef NTSTATUS (NTAPI *PPHSVC_API_PROCEDURE)(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    );

VOID PhSvcDispatchApiCall(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload,
    _Out_ PHANDLE ReplyPortHandle
    );

PVOID PhSvcValidateString(
    _In_ PPH_RELATIVE_STRINGREF String,
    _In_ ULONG Alignment
    );

NTSTATUS PhSvcProbeBuffer(
    _In_ PPH_RELATIVE_STRINGREF String,
    _In_ ULONG Alignment,
    _In_ BOOLEAN AllowNull,
    _Out_ PVOID *Pointer
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

NTSTATUS PhSvcCaptureSecurityDescriptor(
    _In_ PPH_RELATIVE_STRINGREF String,
    _In_ BOOLEAN AllowNull,
    _In_ SECURITY_INFORMATION RequiredInformation,
    _Out_ PSECURITY_DESCRIPTOR *CapturedSecurityDescriptor
    );

NTSTATUS PhSvcApiDefault(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    );

NTSTATUS PhSvcApiPlugin(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    );

NTSTATUS PhSvcApiExecuteRunAsCommand(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    );

NTSTATUS PhSvcApiUnloadDriver(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    );

NTSTATUS PhSvcApiControlProcess(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    );

NTSTATUS PhSvcApiControlService(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    );

NTSTATUS PhSvcApiCreateService(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    );

NTSTATUS PhSvcApiChangeServiceConfig(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    );

NTSTATUS PhSvcApiChangeServiceConfig2(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    );

NTSTATUS PhSvcApiSetTcpEntry(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    );

NTSTATUS PhSvcApiControlThread(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    );

NTSTATUS PhSvcApiAddAccountRight(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    );

NTSTATUS PhSvcApiInvokeRunAsService(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    );

NTSTATUS PhSvcApiIssueMemoryListCommand(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    );

NTSTATUS PhSvcApiPostMessage(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    );

NTSTATUS PhSvcApiSendMessage(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    );

NTSTATUS PhSvcApiCreateProcessIgnoreIfeoDebugger(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    );

NTSTATUS PhSvcApiSetServiceSecurity(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    );

NTSTATUS PhSvcApiWriteMiniDumpProcess(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    );

NTSTATUS PhSvcApiQueryProcessHeapInformation(
    _In_ PPHSVC_CLIENT Client,
    _Inout_ PPHSVC_API_PAYLOAD Payload
    );

#endif
