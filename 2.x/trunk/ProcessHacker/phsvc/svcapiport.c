/*
 * Process Hacker - 
 *   server API port
 * 
 * Copyright (C) 2011 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <phapp.h>
#include <phsvc.h>

NTSTATUS PhSvcApiRequestThreadStart(
    __in PVOID Parameter
    );

extern HANDLE PhSvcTimeoutStandbyEventHandle;
extern HANDLE PhSvcTimeoutCancelEventHandle;

ULONG PhSvcApiThreadContextTlsIndex;
HANDLE PhSvcApiPortHandle;
ULONG PhSvcApiNumberOfClients = 0;

NTSTATUS PhSvcApiPortInitialization(
    __in PPH_STRINGREF PortName
    )
{
    static SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING objectName;
    PSECURITY_DESCRIPTOR securityDescriptor;
    ULONG sdAllocationLength;
    UCHAR administratorsSidBuffer[FIELD_OFFSET(SID, SubAuthority) + sizeof(ULONG) * 2];
    PSID administratorsSid;
    PACL dacl;
    ULONG i;
    HANDLE threadHandle;

    // Create the API port.

    objectName = PortName->us;

    administratorsSid = (PSID)administratorsSidBuffer;
    RtlInitializeSid(administratorsSid, &ntAuthority, 2);
    *RtlSubAuthoritySid(administratorsSid, 0) = SECURITY_BUILTIN_DOMAIN_RID;
    *RtlSubAuthoritySid(administratorsSid, 1) = DOMAIN_ALIAS_RID_ADMINS;

    sdAllocationLength = SECURITY_DESCRIPTOR_MIN_LENGTH +
        (ULONG)sizeof(ACL) +
        (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
        RtlLengthSid(administratorsSid) +
        (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
        RtlLengthSid(&PhSeEveryoneSid);

    securityDescriptor = PhAllocate(sdAllocationLength);
    dacl = (PACL)((PCHAR)securityDescriptor + SECURITY_DESCRIPTOR_MIN_LENGTH);

    RtlCreateSecurityDescriptor(securityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    RtlCreateAcl(dacl, sdAllocationLength - SECURITY_DESCRIPTOR_MIN_LENGTH, ACL_REVISION);
    RtlAddAccessAllowedAce(dacl, ACL_REVISION, PORT_ALL_ACCESS, administratorsSid);
    RtlAddAccessAllowedAce(dacl, ACL_REVISION, PORT_CONNECT, &PhSeEveryoneSid);
    RtlSetDaclSecurityDescriptor(securityDescriptor, TRUE, dacl, FALSE);

    InitializeObjectAttributes(
        &objectAttributes,
        &objectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        securityDescriptor
        );

    status = NtCreatePort(
        &PhSvcApiPortHandle,
        &objectAttributes,
        sizeof(PHSVC_API_CONNECTINFO),
        sizeof(PHSVC_API_MSG),
        0
        );
    PhFree(securityDescriptor);

    if (!NT_SUCCESS(status))
        return status;

    // Start the API threads.

    PhSvcApiThreadContextTlsIndex = TlsAlloc();

    for (i = 0; i < 2; i++)
    {
        threadHandle = PhCreateThread(0, PhSvcApiRequestThreadStart, NULL);

        if (threadHandle)
            NtClose(threadHandle);
    }

    return status;
}

PPHSVC_THREAD_CONTEXT PhSvcGetCurrentThreadContext()
{
    return (PPHSVC_THREAD_CONTEXT)TlsGetValue(PhSvcApiThreadContextTlsIndex);
}

NTSTATUS PhSvcApiRequestThreadStart(
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
            (PPORT_MESSAGE)replyMessage,
            (PPORT_MESSAGE)&receiveMessage
            );

        portHandle = PhSvcApiPortHandle;
        replyMessage = NULL;

        if (!NT_SUCCESS(status))
        {
            // Client probably died.
            continue;
        }

        messageType = receiveMessage.h.u2.s2.Type;

        if (messageType == LPC_CONNECTION_REQUEST)
        {
            PhSvcHandleConnectionRequest(&receiveMessage);
            continue;
        }

        if (!portContext)
            continue;

        client = (PPHSVC_CLIENT)portContext;
        threadContext.CurrentClient = client;

        if (messageType == LPC_REQUEST)
        {
            PhSvcDispatchApiCall(client, &receiveMessage, &replyMessage, &portHandle);
        }
        else if (messageType == LPC_PORT_CLOSED)
        {
            PhDereferenceObject(client);

            if (_InterlockedDecrement(&PhSvcApiNumberOfClients) == 0)
            {
                NtSetEvent(PhSvcTimeoutStandbyEventHandle, NULL);
            }
        }

        assert(!threadContext.OldClient);
    }
}

VOID PhSvcHandleConnectionRequest(
    __in PPHSVC_API_MSG Message
    )
{
    NTSTATUS status;
    PPHSVC_CLIENT client;
    HANDLE portHandle;
    REMOTE_PORT_VIEW clientView;

    client = PhSvcCreateClient(&Message->h.ClientId);

    if (!client)
    {
        NtAcceptConnectPort(&portHandle, NULL, &Message->h, FALSE, NULL, NULL);
        return;
    }

    Message->ConnectInfo.ServerProcessId = NtCurrentProcessId();

    clientView.Length = sizeof(REMOTE_PORT_VIEW);
    clientView.ViewSize = 0;
    clientView.ViewBase = NULL;

    status = NtAcceptConnectPort(
        &portHandle,
        client,
        &Message->h,
        TRUE,
        NULL,
        &clientView
        );

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(client);
        return;
    }

    client->PortHandle = portHandle;
    client->ClientViewBase = clientView.ViewBase;
    client->ClientViewLimit = (PCHAR)clientView.ViewBase + clientView.ViewSize;

    status = NtCompleteConnectPort(portHandle);

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(client);
        return;
    }

    if (_InterlockedIncrement(&PhSvcApiNumberOfClients) == 1)
    {
        NtSetEvent(PhSvcTimeoutCancelEventHandle, NULL);
    }
}
