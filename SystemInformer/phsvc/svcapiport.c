/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *     dmex    2017-2023
 *
 */

#include <phapp.h>
#include <phsvc.h>
#include <verify.h>

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PhSvcApiRequestThreadStart(
    _In_ PVOID Parameter
    );

extern HANDLE PhSvcTimeoutStandbyEventHandle;
extern HANDLE PhSvcTimeoutCancelEventHandle;

ULONG PhSvcApiThreadContextTlsIndex;
HANDLE PhSvcApiPortHandle;
volatile LONG PhSvcApiNumberOfClients = 0;

NTSTATUS PhSvcApiPortInitialization(
    _In_ PUNICODE_STRING PortName
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    PSECURITY_DESCRIPTOR securityDescriptor;
    ULONG sdAllocationLength;
    PSID administratorsSid;
    PACL dacl;
    ULONG i;

    // Create the API port.

    administratorsSid = PhSeAdministratorsSid();

    sdAllocationLength = SECURITY_DESCRIPTOR_MIN_LENGTH +
        (ULONG)sizeof(ACL) +
        (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
        PhLengthSid(administratorsSid) +
        (ULONG)sizeof(ACCESS_ALLOWED_ACE) +
        PhLengthSid((PSID)&PhSeEveryoneSid);

    securityDescriptor = PhAllocateZero(sdAllocationLength);
    dacl = (PACL)PTR_ADD_OFFSET(securityDescriptor, SECURITY_DESCRIPTOR_MIN_LENGTH);

    PhCreateSecurityDescriptor(securityDescriptor, SECURITY_DESCRIPTOR_REVISION);
    PhCreateAcl(dacl, sdAllocationLength - SECURITY_DESCRIPTOR_MIN_LENGTH, ACL_REVISION);
    PhAddAccessAllowedAce(dacl, ACL_REVISION, PORT_ALL_ACCESS, administratorsSid);
    PhAddAccessAllowedAce(dacl, ACL_REVISION, PORT_CONNECT, (PSID)&PhSeEveryoneSid);
    PhSetDaclSecurityDescriptor(securityDescriptor, TRUE, dacl, FALSE);

    InitializeObjectAttributes(
        &objectAttributes,
        PortName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        securityDescriptor
        );

    status = NtCreatePort(
        &PhSvcApiPortHandle,
        &objectAttributes,
        sizeof(PHSVC_API_CONNECTINFO),
        PhIsExecutingInWow64() ? sizeof(PHSVC_API_MSG64) : sizeof(PHSVC_API_MSG),
        0
        );
    PhFree(securityDescriptor);

    if (!NT_SUCCESS(status))
        return status;

    // Start the API threads.

    PhSvcApiThreadContextTlsIndex = PhTlsAlloc();

    for (i = 0; i < 2; i++)
    {
        PhCreateThread2(PhSvcApiRequestThreadStart, NULL);
    }

    return status;
}

PPHSVC_THREAD_CONTEXT PhSvcGetCurrentThreadContext(
    VOID
    )
{
    return (PPHSVC_THREAD_CONTEXT)PhTlsGetValue(PhSvcApiThreadContextTlsIndex);
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS PhSvcApiRequestThreadStart(
    _In_ PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    NTSTATUS status;
    PHSVC_THREAD_CONTEXT threadContext;
    HANDLE portHandle;
    PVOID portContext;
    SIZE_T messageSize;
    PPORT_MESSAGE receiveMessage;
    PPORT_MESSAGE replyMessage;
    CSHORT messageType;
    PPHSVC_CLIENT client;
    PPHSVC_API_PAYLOAD payload;

    PhInitializeAutoPool(&autoPool);

    threadContext.CurrentClient = NULL;
    threadContext.OldClient = NULL;

    PhTlsSetValue(PhSvcApiThreadContextTlsIndex, &threadContext);

    portHandle = PhSvcApiPortHandle;
    messageSize = PhIsExecutingInWow64() ? sizeof(PHSVC_API_MSG64) : sizeof(PHSVC_API_MSG);
    receiveMessage = PhAllocatePageZero(messageSize);
    replyMessage = NULL;

    if (!receiveMessage)
    {
        PhDeleteAutoPool(&autoPool);
        return STATUS_NO_MEMORY;
    }

    while (TRUE)
    {
        status = NtReplyWaitReceivePort(
            portHandle,
            &portContext,
            replyMessage,
            receiveMessage
            );

        portHandle = PhSvcApiPortHandle;
        replyMessage = NULL;

        if (!NT_SUCCESS(status))
        {
            // Client probably died.
            continue;
        }

        messageType = receiveMessage->u2.s2.Type;

        if (messageType == LPC_CONNECTION_REQUEST)
        {
            PhSvcHandleConnectionRequest(receiveMessage);
            continue;
        }

        if (!portContext)
            continue;

        client = portContext;
        threadContext.CurrentClient = client;
        PhWaitForEvent(&client->ReadyEvent, NULL);

        if (messageType == LPC_REQUEST)
        {
            if (PhIsExecutingInWow64())
                payload = &((PPHSVC_API_MSG64)receiveMessage)->p;
            else
                payload = &((PPHSVC_API_MSG)receiveMessage)->p;

            PhSvcDispatchApiCall(client, payload, &portHandle);
            replyMessage = receiveMessage;
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
        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);
}

BOOLEAN PhSvcHandleVerify(
    _In_ PCPH_STRINGREF FileName
    )
{
    BOOLEAN status = FALSE;
    HANDLE fileHandle;

    if (NT_SUCCESS(PhCreateFile(
        &fileHandle,
        FileName,
        FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE | DELETE,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        FILE_OPEN,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
        )))
    {
        VERIFY_RESULT result;
        PH_VERIFY_FILE_INFO info;

        memset(&info, 0, sizeof(PH_VERIFY_FILE_INFO));
        info.Flags = PH_VERIFY_PREVENT_NETWORK_ACCESS;
        info.FileHandle = fileHandle;

        if (NT_SUCCESS(PhVerifyFileEx(&info, &result, NULL, NULL)))
        {
            if (result == VrTrusted)
            {
                status = TRUE;
            }
        }

        NtClose(fileHandle);
    }

    return status;
}

VOID PhSvcHandleConnectionRequest(
    _In_ PPORT_MESSAGE PortMessage
    )
{
    NTSTATUS status;
    PPHSVC_API_MSG message;
    PPHSVC_API_MSG64 message64;
    CLIENT_ID clientId;
    PPHSVC_CLIENT client;
    HANDLE portHandle;
    REMOTE_PORT_VIEW clientView;
    REMOTE_PORT_VIEW64 clientView64;
    PREMOTE_PORT_VIEW actualClientView;

    message = (PPHSVC_API_MSG)PortMessage;
    message64 = (PPHSVC_API_MSG64)PortMessage;

    if (PhIsExecutingInWow64())
    {
        clientId.UniqueProcess = (HANDLE)message64->h.ClientId.UniqueProcess;
        clientId.UniqueThread = (HANDLE)message64->h.ClientId.UniqueThread;

#if defined(PH_BUILD_API)
        PPH_STRING remoteFileName;

        remoteFileName = NULL;
        PhGetProcessImageFileNameByProcessId(clientId.UniqueProcess, &remoteFileName);
        PH_AUTO(remoteFileName);

        if (PhIsNullOrEmptyString(remoteFileName) || !PhSvcHandleVerify(&remoteFileName->sr))
        {
            NtAcceptConnectPort(&portHandle, NULL, PortMessage, FALSE, NULL, NULL);
            return;
        }
#endif // PH_BUILD_API
    }
    else
    {
#if defined(DEBUG) || defined(PH_BUILD_API)
        PPH_STRING remoteFileName;
#endif
#if defined(DEBUG)
        PPH_STRING referenceFileName;

        clientId = message->h.ClientId;

        // Make sure that the remote process is System Informer and not some other program.

        referenceFileName = NULL;
        PhGetProcessImageFileNameByProcessId(NtCurrentProcessId(), &referenceFileName);
        PH_AUTO(referenceFileName);

        remoteFileName = NULL;
        PhGetProcessImageFileNameByProcessId(clientId.UniqueProcess, &remoteFileName);
        PH_AUTO(remoteFileName);

        if (PhIsNullOrEmptyString(referenceFileName) || PhIsNullOrEmptyString(remoteFileName) || !PhEqualString(referenceFileName, remoteFileName, FALSE))
        {
            NtAcceptConnectPort(&portHandle, NULL, PortMessage, FALSE, NULL, NULL);
            return;
        }
#endif // DEBUG
#if defined(PH_BUILD_API)
        remoteFileName = NULL;
        clientId = message->h.ClientId;
        PhGetProcessImageFileNameByProcessId(clientId.UniqueProcess, &remoteFileName);
        PH_AUTO(remoteFileName);

        if (PhIsNullOrEmptyString(remoteFileName) || !PhSvcHandleVerify(&remoteFileName->sr))
        {
            NtAcceptConnectPort(&portHandle, NULL, PortMessage, FALSE, NULL, NULL);
            return;
        }
#endif // PH_BUILD_API
    }

    client = PhSvcCreateClient(&clientId);

    if (!client)
    {
        NtAcceptConnectPort(&portHandle, NULL, PortMessage, FALSE, NULL, NULL);
        return;
    }

    if (PhIsExecutingInWow64())
    {
        message64->p.ConnectInfo.ServerProcessId = HandleToUlong(NtCurrentProcessId());

        memset(&clientView64, 0, sizeof(REMOTE_PORT_VIEW64));
        clientView64.Length = sizeof(REMOTE_PORT_VIEW64);
        clientView64.ViewSize = 0;
        clientView64.ViewBase = 0;
        actualClientView = (PREMOTE_PORT_VIEW)&clientView64;
    }
    else
    {
        message->p.ConnectInfo.ServerProcessId = HandleToUlong(NtCurrentProcessId());

        memset(&clientView, 0, sizeof(REMOTE_PORT_VIEW));
        clientView.Length = sizeof(REMOTE_PORT_VIEW);
        clientView.ViewSize = 0;
        clientView.ViewBase = NULL;
        actualClientView = &clientView;
    }

    status = NtAcceptConnectPort(
        &portHandle,
        client,
        PortMessage,
        TRUE,
        NULL,
        actualClientView
        );

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(client);
        return;
    }

    // IMPORTANT: Since Vista, NtCompleteConnectPort does not do anything and simply returns STATUS_SUCCESS.
    // We will call it anyway (for completeness), but we need to use an event to ensure that other threads don't try
    // to process requests before we have finished setting up the client object.

    client->PortHandle = portHandle;

    if (PhIsExecutingInWow64())
    {
        client->ClientViewBase = (PVOID)clientView64.ViewBase;
        client->ClientViewLimit = PTR_ADD_OFFSET(clientView64.ViewBase, clientView64.ViewSize);
    }
    else
    {
        client->ClientViewBase = clientView.ViewBase;
        client->ClientViewLimit = PTR_ADD_OFFSET(clientView.ViewBase, clientView.ViewSize);
    }

    //NtCompleteConnectPort(portHandle); // (dmex)
    PhSetEvent(&client->ReadyEvent);

    if (_InterlockedIncrement(&PhSvcApiNumberOfClients) == 1)
    {
        NtSetEvent(PhSvcTimeoutCancelEventHandle, NULL);
    }
}
