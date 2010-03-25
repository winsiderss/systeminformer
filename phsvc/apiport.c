#include <phsvc.h>

NTSTATUS PhApiRequestThreadStart(
    __in PVOID Parameter
    );

ULONG PhSvcApiThreadContextTlsIndex;
HANDLE PhSvcApiPortHandle;

NTSTATUS PhApiPortInitialization()
{
    static SID_IDENTIFIER_AUTHORITY worldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;

    NTSTATUS status;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING portName;
    PSECURITY_DESCRIPTOR securityDescriptor;
    ULONG sdAllocationLength;
    PACL dacl;
    PSID worldSid;
    ULONG i;

    RtlInitUnicodeString(&portName, PHSVC_PORT_NAME);

    worldSid = PhAllocate(RtlLengthRequiredSid(1));
    RtlInitializeSid(worldSid, &worldSidAuthority, 1);
    *(RtlSubAuthoritySid(worldSid, 0)) = SECURITY_WORLD_RID;

    sdAllocationLength = SECURITY_DESCRIPTOR_MIN_LENGTH +
        (ULONG)sizeof(ACL) +
        (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
        RtlLengthSid(worldSid) +
        32;

    securityDescriptor = PhAllocate(sdAllocationLength);
    dacl = (PACL)PTR_ADD_OFFSET(securityDescriptor, SECURITY_DESCRIPTOR_MIN_LENGTH);

    RtlCreateSecurityDescriptor(securityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    RtlCreateAcl(dacl, sdAllocationLength - SECURITY_DESCRIPTOR_MIN_LENGTH, ACL_REVISION2);

    RtlAddAccessAllowedAce(dacl, ACL_REVISION2, PORT_ALL_ACCESS, worldSid);
    RtlSetDaclSecurityDescriptor(securityDescriptor, TRUE, dacl, FALSE);

    InitializeObjectAttributes(
        &oa,
        &portName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        securityDescriptor
        );

    status = NtCreatePort(
        &PhSvcApiPortHandle,
        &oa,
        sizeof(PHSVC_API_CONNECTINFO),
        sizeof(PHSVC_API_MSG),
        0
        );

    PhFree(securityDescriptor);
    PhFree(worldSid);

    PhSvcApiThreadContextTlsIndex = TlsAlloc();

    for (i = 0; i < 8; i++)
    {
        PhCreateThread(0, PhApiRequestThreadStart, NULL);
    }

    return status;
}

PPHSVC_THREAD_CONTEXT PhSvcGetCurrentThreadContext()
{
    return (PPHSVC_THREAD_CONTEXT)TlsGetValue(PhSvcApiThreadContextTlsIndex);
}

NTSTATUS PhApiRequestThreadStart(
    __in PVOID Parameter
    )
{
    NTSTATUS status;
    PHSVC_THREAD_CONTEXT threadContext;
    HANDLE portHandle;
    PVOID portContext;
    PHSVC_API_MSG receiveMessage;
    PPHSVC_API_MSG replyMessage;
    CSHORT messageType;
    PPHSVC_CLIENT client;

    threadContext.CurrentClient = NULL;
    threadContext.OldClient = NULL;

    TlsSetValue(PhSvcApiThreadContextTlsIndex, &threadContext);

    portHandle = PhSvcApiPortHandle;
    replyMessage = NULL;

    while (TRUE)
    {
        status = NtReplyWaitReceivePort(
            portHandle,
            &portContext,
            (PPORT_MESSAGE)&receiveMessage,
            (PPORT_MESSAGE)replyMessage
            );

        portHandle = PhSvcApiPortHandle;
        replyMessage = NULL;

        if (status != STATUS_SUCCESS)
        {
            // Client probably died.
            continue;
        }

        messageType = receiveMessage.h.u2.s2.Type;

        if (messageType == LPC_CONNECTION_REQUEST)
        {
            PhHandleConnectionRequest(&receiveMessage);
            continue;
        }

        if (!portContext)
            continue;

        client = (PPHSVC_CLIENT)portContext;
        threadContext.CurrentClient = client;

        PhSvcDispatchApiCall(client, &receiveMessage, &replyMessage, &portHandle);

        assert(!threadContext.OldClient);
    }
}

VOID PhHandleConnectionRequest(
    __in PPHSVC_API_MSG Message
    )
{
    NTSTATUS status;
    PPHSVC_CLIENT client;
    HANDLE portHandle;

    client = PhSvcCreateClient(&Message->h.ClientId);

    if (!client)
    {
        NtAcceptConnectPort(&portHandle, NULL, &Message->h, FALSE, NULL, NULL);
        return;
    }

    status = NtAcceptConnectPort(
        &portHandle,
        client,
        &Message->h,
        TRUE,
        NULL,
        NULL
        );

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(client);
        return;
    }

    client->PortHandle = portHandle;

    status = NtCompleteConnectPort(portHandle);

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(client);
        return;
    }
}
