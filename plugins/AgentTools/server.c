/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include "agenttools.h"

static PPH_OBJECT_TYPE AtConnectionType = NULL;
static LIST_ENTRY AtConnectionList;
static PH_QUEUED_LOCK AtConnectionListLock = PH_QUEUED_LOCK_INIT;
static ULONG AtNextConnectionId = 1;

static PH_QUEUED_LOCK AtServerLock = PH_QUEUED_LOCK_INIT;
static AT_SERVER_STATE AtServerState = AtServerStopped;
static NTSTATUS AtServerStatus = STATUS_SUCCESS;
static BOOLEAN AtServerElevated = FALSE;
static LONG AtServerStopping = 0;
static HANDLE AtListenerThreadHandle = NULL;
static PPH_STRING AtPipeName = NULL;
static PSECURITY_DESCRIPTOR AtPipeSecurityDescriptor = NULL;

// S-1-15-2-1 ALL APPLICATION PACKAGES
static struct
{
    UCHAR Revision;
    UCHAR SubAuthorityCount;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
    ULONG SubAuthority[2];
} AtAllApplicationPackagesSid =
{
    SID_REVISION,
    2,
    SECURITY_APP_PACKAGE_AUTHORITY,
    { SECURITY_APP_PACKAGE_BASE_RID, SECURITY_BUILTIN_PACKAGE_ANY_PACKAGE }
};

static SID AtMediumLabelSid = { SID_REVISION, 1, SECURITY_MANDATORY_LABEL_AUTHORITY, { SECURITY_MANDATORY_MEDIUM_RID } };
static SID AtLowLabelSid = { SID_REVISION, 1, SECURITY_MANDATORY_LABEL_AUTHORITY, { SECURITY_MANDATORY_LOW_RID } };

_Function_class_(PH_TYPE_DELETE_PROCEDURE)
VOID NTAPI AtpConnectionDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PAT_CONNECTION connection = Object;

    AtMcpDeleteConnectionState(connection);

    PhClearReference(&connection->UserName);
    PhClearReference(&connection->LauncherImageName);
    PhClearReference(&connection->LauncherSignerName);
    PhClearReference(&connection->ClientName);
    PhClearReference(&connection->ClientVersion);
    PhClearReference(&connection->ProtocolVersion);
    PhClearReference(&connection->InFlightId);

    if (connection->ThreadHandle)
        NtClose(connection->ThreadHandle);
    if (connection->PipeHandle)
        NtClose(connection->PipeHandle);
}

NTSTATUS AtpCreatePipeSecurityDescriptor(
    _In_ BOOLEAN AllowSandboxedClients,
    _Out_ PSECURITY_DESCRIPTOR* SecurityDescriptor
    )
{
    NTSTATUS status;
    PH_TOKEN_USER tokenUser;
    UCHAR logonSidBuffer[sizeof(TOKEN_GROUPS) + SECURITY_MAX_SID_SIZE];
    PTOKEN_GROUPS logonSidGroups = (PTOKEN_GROUPS)logonSidBuffer;
    PSID accessSid;
    PSID labelSid;
    ULONG daclLength;
    ULONG saclLength;
    ULONG allocationLength;
    PSECURITY_DESCRIPTOR securityDescriptor;
    PACL dacl;
    PACL sacl;
    SYSTEM_MANDATORY_LABEL_ACE* labelAce;
    ULONG labelAceLength;

    status = PhGetTokenUser(NtCurrentProcessToken(), &tokenUser);

    if (!NT_SUCCESS(status))
        return status;

    accessSid = tokenUser.User.Sid;

    if (NT_SUCCESS(NtQueryInformationToken(
        NtCurrentProcessToken(),
        TokenLogonSid,
        logonSidBuffer,
        sizeof(logonSidBuffer),
        &(ULONG){ 0 }
        )) && logonSidGroups->GroupCount == 1)
    {
        accessSid = logonSidGroups->Groups[0].Sid;
    }

    labelSid = AllowSandboxedClients ? &AtLowLabelSid : &AtMediumLabelSid;

    daclLength = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(accessSid);

    if (AllowSandboxedClients)
        daclLength += sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(&AtAllApplicationPackagesSid);

    labelAceLength = FIELD_OFFSET(SYSTEM_MANDATORY_LABEL_ACE, SidStart) + RtlLengthSid(labelSid);
    saclLength = sizeof(ACL) + labelAceLength;

    allocationLength = SECURITY_DESCRIPTOR_MIN_LENGTH + daclLength + saclLength + labelAceLength;
    securityDescriptor = PhAllocateZero(allocationLength);
    dacl = PTR_ADD_OFFSET(securityDescriptor, SECURITY_DESCRIPTOR_MIN_LENGTH);
    sacl = PTR_ADD_OFFSET(dacl, daclLength);
    labelAce = PTR_ADD_OFFSET(sacl, saclLength);

    if (!NT_SUCCESS(status = RtlCreateSecurityDescriptor(securityDescriptor, SECURITY_DESCRIPTOR_REVISION)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = RtlCreateAcl(dacl, daclLength, ACL_REVISION)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = RtlAddAccessAllowedAce(dacl, ACL_REVISION, FILE_ALL_ACCESS, accessSid)))
        goto CleanupExit;

    if (AllowSandboxedClients)
    {
        if (!NT_SUCCESS(status = RtlAddAccessAllowedAce(
            dacl,
            ACL_REVISION,
            FILE_GENERIC_READ | FILE_GENERIC_WRITE | SYNCHRONIZE,
            &AtAllApplicationPackagesSid
            )))
        {
            goto CleanupExit;
        }
    }

    if (!NT_SUCCESS(status = RtlSetDaclSecurityDescriptor(securityDescriptor, TRUE, dacl, FALSE)))
        goto CleanupExit;

    // Explicit label rather than relying on the default.
    labelAce->Header.AceType = SYSTEM_MANDATORY_LABEL_ACE_TYPE;
    labelAce->Header.AceFlags = 0;
    labelAce->Header.AceSize = (USHORT)labelAceLength;
    labelAce->Mask = SYSTEM_MANDATORY_LABEL_NO_WRITE_UP;
    memcpy(&labelAce->SidStart, labelSid, RtlLengthSid(labelSid));

    if (!NT_SUCCESS(status = RtlCreateAcl(sacl, saclLength, ACL_REVISION)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = RtlAddAce(sacl, ACL_REVISION, MAXULONG, labelAce, labelAceLength)))
        goto CleanupExit;
    if (!NT_SUCCESS(status = RtlSetSaclSecurityDescriptor(securityDescriptor, TRUE, sacl, FALSE)))
        goto CleanupExit;

    assert(RtlValidSecurityDescriptor(securityDescriptor));

CleanupExit:
    if (NT_SUCCESS(status))
        *SecurityDescriptor = securityDescriptor;
    else
        PhFree(securityDescriptor);

    return status;
}

PPH_STRING AtpFormatPipeName(
    _In_ BOOLEAN Elevated
    )
{
    static CONST PH_STRINGREF protectedPrefix = PH_STRINGREF_INIT(SIMCP_PIPE_PROTECTED_PREFIX);
    static CONST PH_STRINGREF namePrefix = PH_STRINGREF_INIT(SIMCP_PIPE_NAME_PREFIX);
    PPH_STRING sessionName;
    PPH_STRING pipeName;
    PH_FORMAT format[1];

    PhInitFormatU(&format[0], NtCurrentPeb()->SessionId);
    sessionName = PhFormat(format, RTL_NUMBER_OF(format), 16);

    if (Elevated)
        pipeName = PhConcatStringRef3(&protectedPrefix, &namePrefix, &sessionName->sr);
    else
        pipeName = PhConcatStringRef2(&namePrefix, &sessionName->sr);

    PhDereferenceObject(sessionName);
    return pipeName;
}

NTSTATUS AtpCreatePipeInstance(
    _In_ BOOLEAN FirstInstance,
    _Out_ PHANDLE PipeHandle
    )
{
    return PhCreateNamedPipeEx(
        PipeHandle,
        &AtPipeName->sr,
        NULL,
        AtPipeSecurityDescriptor,
        FirstInstance ? FILE_CREATE : FILE_OPEN_IF,
        FILE_PIPE_BYTE_STREAM_TYPE,
        FILE_PIPE_UNLIMITED_INSTANCES
        );
}

NTSTATUS AtpReadExact(
    _In_ PAT_CONNECTION Connection,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    NTSTATUS status;
    ULONG offset = 0;

    while (offset < Length)
    {
        ULONG bytesRead = 0;

        if (AtConnectionIsClosing(Connection))
            return STATUS_CANCELLED;

        status = PhReadFile(Connection->PipeHandle, PTR_ADD_OFFSET(Buffer, offset), Length - offset, NULL, &bytesRead);

        if (!NT_SUCCESS(status))
            return status;
        if (bytesRead == 0)
            return STATUS_PIPE_BROKEN;

        offset += bytesRead;
    }

    return STATUS_SUCCESS;
}

NTSTATUS AtConnectionRead(
    _In_ PAT_CONNECTION Connection,
    _Out_ PSIMCP_HEADER Header,
    _Outptr_result_maybenull_ PVOID* Payload
    )
{
    NTSTATUS status;
    PVOID payload = NULL;

    status = AtpReadExact(Connection, Header, sizeof(SIMCP_HEADER));

    if (!NT_SUCCESS(status))
        return status;

    if (Header->Magic != SIMCP_MAGIC ||
        Header->Version != SIMCP_VERSION ||
        Header->Reserved != 0 ||
        Header->PayloadLength > SIMCP_MAX_PAYLOAD_LENGTH)
    {
        return STATUS_INVALID_NETWORK_RESPONSE;
    }

    if (Header->PayloadLength)
    {
        payload = PhAllocate(Header->PayloadLength);
        status = AtpReadExact(Connection, payload, Header->PayloadLength);

        if (!NT_SUCCESS(status))
        {
            PhFree(payload);
            return status;
        }
    }

    *Payload = payload;
    return STATUS_SUCCESS;
}

NTSTATUS AtConnectionPeek(
    _In_ PAT_CONNECTION Connection,
    _Out_ PBOOLEAN MessageAvailable
    )
{
    NTSTATUS status;
    ULONG available = 0;

    status = PhPeekNamedPipe(Connection->PipeHandle, NULL, 0, NULL, &available, NULL);

    if (NT_SUCCESS(status))
        *MessageAvailable = available >= sizeof(SIMCP_HEADER);

    return status;
}

NTSTATUS AtConnectionSend(
    _In_ PAT_CONNECTION Connection,
    _In_ USHORT Type,
    _In_reads_bytes_opt_(PayloadLength) PVOID Payload,
    _In_ ULONG PayloadLength
    )
{
    NTSTATUS status;
    SIMCP_HEADER header;

    memset(&header, 0, sizeof(SIMCP_HEADER));
    header.Magic = SIMCP_MAGIC;
    header.Version = SIMCP_VERSION;
    header.Type = Type;
    header.PayloadLength = PayloadLength;

    // All pipe I/O happens on the connection thread: the handle is synchronous, and a
    // synchronous file object serializes every operation on it, so a write from another thread
    // would block behind a pending read.
    assert(NtCurrentThreadId() == Connection->ThreadId);

    status = PhWriteFile(Connection->PipeHandle, &header, sizeof(SIMCP_HEADER), NULL, NULL);

    if (NT_SUCCESS(status) && Payload && PayloadLength)
    {
        status = PhWriteFile(Connection->PipeHandle, Payload, PayloadLength, NULL, NULL);
    }

    return status;
}

VOID AtpSendClose(
    _In_ PAT_CONNECTION Connection
    )
{
    SIMCP_CLOSE close;

    assert(NtCurrentThreadId() == Connection->ThreadId);

    memset(&close, 0, sizeof(SIMCP_CLOSE));
    close.Reason = (ULONG)ReadAcquire(&Connection->Closing);
    close.Detail = Connection->CloseDetail;

    AtConnectionSend(Connection, SimcpClose, &close, sizeof(SIMCP_CLOSE));
    Connection->CloseSent = TRUE;
}

VOID AtConnectionClose(
    _In_ PAT_CONNECTION Connection,
    _In_ SIMCP_CLOSE_REASON Reason,
    _In_ ULONG Detail
    )
{
    IO_STATUS_BLOCK isb;

    // The reason travels in the flag itself, so the first closer's reason is the one sent and a
    // concurrent loser cannot overwrite it.
    assert(Reason != 0);

    if (InterlockedCompareExchange(&Connection->Closing, (LONG)Reason, 0) != 0)
        return;

    Connection->CloseDetail = Detail;

    if (NtCurrentThreadId() == Connection->ThreadId)
    {
        AtpSendClose(Connection);
    }
    else if (Connection->ThreadHandle)
    {
        NtCancelSynchronousIoFile(Connection->ThreadHandle, NULL, &isb);
    }
}

BOOLEAN AtpRegisterConnection(
    _In_ PAT_CONNECTION Connection
    )
{
    BOOLEAN registered = FALSE;

    PhAcquireQueuedLockExclusive(&AtConnectionListLock);

    if (!ReadAcquire(&AtServerStopping))
    {
        InsertTailList(&AtConnectionList, &Connection->ListEntry);
        Connection->Registered = TRUE;
        registered = TRUE;
    }

    PhReleaseQueuedLockExclusive(&AtConnectionListLock);

    return registered;
}

VOID AtpUnregisterConnection(
    _In_ PAT_CONNECTION Connection
    )
{
    PhAcquireQueuedLockExclusive(&AtConnectionListLock);

    if (Connection->Registered)
    {
        RemoveEntryList(&Connection->ListEntry);
        Connection->Registered = FALSE;
    }

    PhReleaseQueuedLockExclusive(&AtConnectionListLock);
}

SIMCP_HELLO_STATUS AtpValidateBrokerImage(
    _In_ HANDLE ProcessHandle
    )
{
    static CONST PH_STRINGREF brokerFileName = PH_STRINGREF_INIT(SIMCP_BROKER_FILE_NAME);
    SIMCP_HELLO_STATUS result = SimcpHelloRejectedInternal;
    PPH_STRING remoteFileName = NULL;
    PPH_STRING directory = NULL;
    PPH_STRING expectedFileName = NULL;

    if (!NT_SUCCESS(PhGetProcessImageFileNameWin32(ProcessHandle, &remoteFileName)))
        goto CleanupExit;

    if (!(directory = PhGetApplicationDirectoryWin32()))
        goto CleanupExit;

    expectedFileName = PhConcatStringRef2(&directory->sr, &brokerFileName);

    if (!PhEqualString(remoteFileName, expectedFileName, TRUE))
    {
        result = SimcpHelloRejectedImage;
        goto CleanupExit;
    }

#if defined(PH_BUILD_API)
    if (PhVerifyFile(PhGetString(remoteFileName), NULL) != VrTrusted)
    {
        result = SimcpHelloRejectedSignature;
        goto CleanupExit;
    }
#endif

    result = SimcpHelloAccepted;

CleanupExit:
    PhClearReference(&expectedFileName);
    PhClearReference(&directory);
    PhClearReference(&remoteFileName);

    return result;
}

VOID AtpResolveLauncher(
    _In_ PAT_CONNECTION Connection,
    _In_ PSIMCP_HELLO Hello
    )
{
    HANDLE processHandle;
    KERNEL_USER_TIMES times;

    Connection->LauncherProcessId = Hello->LauncherProcessId;

    if (!Hello->LauncherProcessId)
        return;

    if (!NT_SUCCESS(PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_LIMITED_INFORMATION,
        UlongToHandle(Hello->LauncherProcessId)
        )))
    {
        return;
    }

    if (NT_SUCCESS(NtQueryInformationProcess(processHandle, ProcessTimes, &times, sizeof(times), NULL)) &&
        times.CreateTime.QuadPart == Hello->LauncherStartTime.QuadPart)
    {
        PhGetProcessImageFileNameWin32(processHandle, &Connection->LauncherImageName);
    }

    NtClose(processHandle);
}

SIMCP_HELLO_STATUS AtpAuthenticateClient(
    _In_ PAT_CONNECTION Connection,
    _In_ PSIMCP_HELLO Hello
    )
{
    NTSTATUS status;
    SIMCP_HELLO_STATUS result = SimcpHelloRejectedInternal;
    HANDLE tokenHandle = NULL;
    HANDLE processHandle = NULL;
    HANDLE clientProcessId;
    PH_TOKEN_USER clientUser;
    PH_TOKEN_USER ownUser;
    ULONG sessionId;
    ULONG isAppContainer;
    PWSTR integrityString = NULL;

    if (Hello->BrokerVersion != SIMCP_VERSION)
        return SimcpHelloRejectedVersion;

    // Capture the token, then revert. Every decision below is made from the captured token.

    status = PhImpersonateClientOfNamedPipe(Connection->PipeHandle);

    if (!NT_SUCCESS(status))
        return SimcpHelloRejectedInternal;

    status = NtOpenThreadToken(NtCurrentThread(), TOKEN_QUERY, TRUE, &tokenHandle);
    PhRevertImpersonationToken(NtCurrentThread());

    if (!NT_SUCCESS(status))
        return SimcpHelloRejectedInternal;

    // Same user as System Informer.

    if (!NT_SUCCESS(PhGetTokenUser(tokenHandle, &clientUser)) ||
        !NT_SUCCESS(PhGetTokenUser(NtCurrentProcessToken(), &ownUser)))
    {
        goto CleanupExit;
    }

    if (!RtlEqualSid(clientUser.User.Sid, ownUser.User.Sid))
    {
        result = SimcpHelloRejectedUser;
        goto CleanupExit;
    }

    // Same session as System Informer. Pipe names are global; the session in the name is
    // a convention, not a boundary.

    if (!NT_SUCCESS(NtQueryInformationToken(tokenHandle, TokenSessionId, &sessionId, sizeof(sessionId), &(ULONG){ 0 })) ||
        sessionId != NtCurrentPeb()->SessionId)
    {
        result = SimcpHelloRejectedUser;
        goto CleanupExit;
    }

    // Integrity level and AppContainer.

    if (!NT_SUCCESS(PhGetTokenIntegrityLevelRID(tokenHandle, &Connection->IntegrityRid, &integrityString)))
        goto CleanupExit;

    Connection->IntegrityString = integrityString;

    if (!NT_SUCCESS(NtQueryInformationToken(tokenHandle, TokenIsAppContainer, &isAppContainer, sizeof(isAppContainer), &(ULONG){ 0 })))
        isAppContainer = 0;

    Connection->IsAppContainer = !!isAppContainer;

    if (!PhGetIntegerSetting(SETTING_NAME_ALLOW_SANDBOXED_CLIENTS))
    {
        if (Connection->IntegrityRid < SECURITY_MANDATORY_MEDIUM_RID)
        {
            result = SimcpHelloRejectedIntegrity;
            goto CleanupExit;
        }

        if (Connection->IsAppContainer)
        {
            result = SimcpHelloRejectedAppContainer;
            goto CleanupExit;
        }
    }

    // The connecting binary is our own broker.

    if (!NT_SUCCESS(PhGetNamedPipeClientProcessId(Connection->PipeHandle, &clientProcessId)))
        goto CleanupExit;

    if (HandleToUlong(clientProcessId) != Hello->BrokerProcessId)
    {
        result = SimcpHelloRejectedProcessId;
        goto CleanupExit;
    }

    if (!NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, clientProcessId)))
        goto CleanupExit;

    result = AtpValidateBrokerImage(processHandle);

    if (result != SimcpHelloAccepted)
        goto CleanupExit;

    Connection->BrokerProcessId = Hello->BrokerProcessId;
    Connection->UserName = PhGetSidFullName(clientUser.User.Sid, TRUE, NULL);

    // Display-only context.

    AtpResolveLauncher(Connection, Hello);

CleanupExit:
    if (processHandle)
        NtClose(processHandle);
    if (tokenHandle)
        NtClose(tokenHandle);

    return result;
}

BOOLEAN AtpHandshake(
    _In_ PAT_CONNECTION Connection
    )
{
    NTSTATUS status;
    SIMCP_HEADER header;
    PVOID payload = NULL;
    SIMCP_HELLO hello;
    SIMCP_HELLO_ACK helloAck;
    SIMCP_HELLO_STATUS helloStatus;

    status = AtConnectionRead(Connection, &header, &payload);

    if (!NT_SUCCESS(status))
        return FALSE;

    if (header.Type != SimcpHello || !payload || header.PayloadLength < sizeof(SIMCP_HELLO))
    {
        if (payload)
            PhFree(payload);

        AtConnectionClose(Connection, SimcpCloseProtocolViolation, 0);
        return FALSE;
    }

    memcpy(&hello, payload, sizeof(SIMCP_HELLO));
    PhFree(payload);

    helloStatus = AtpAuthenticateClient(Connection, &hello);

    if (helloStatus != SimcpHelloAccepted)
    {
        AtConnectionClose(Connection, SimcpCloseRejected, helloStatus);
        return FALSE;
    }

    memset(&helloAck, 0, sizeof(SIMCP_HELLO_ACK));
    helloAck.Status = SimcpHelloAccepted;
    helloAck.ConnectionId = Connection->ConnectionId;
    PhGetBuildVersionNumbers(
        &helloAck.ServerVersion[0],
        &helloAck.ServerVersion[1],
        &helloAck.ServerVersion[2],
        &helloAck.ServerVersion[3]
        );

    status = AtConnectionSend(Connection, SimcpHelloAck, &helloAck, sizeof(SIMCP_HELLO_ACK));

    return NT_SUCCESS(status);
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS NTAPI AtpConnectionThread(
    _In_ PVOID Parameter
    )
{
    PAT_CONNECTION connection = Parameter;
    PH_AUTO_POOL autoPool;

    connection->ThreadId = NtCurrentThreadId();

    PhInitializeAutoPool(&autoPool);

    if (AtpRegisterConnection(connection) && AtpHandshake(connection))
    {
        connection->Authenticated = TRUE;

        AtConsentRequestConnection(connection);

        while (!AtConnectionIsClosing(connection))
        {
            NTSTATUS status;
            SIMCP_HEADER header;
            PVOID payload;

            status = AtConnectionRead(connection, &header, &payload);

            if (!NT_SUCCESS(status))
            {
                if (status == STATUS_INVALID_NETWORK_RESPONSE)
                    AtConnectionClose(connection, SimcpCloseProtocolViolation, 0);
                else if (status == STATUS_CANCELLED && AtConnectionIsClosing(connection))
                    AtpSendClose(connection); // requested by another thread

                break;
            }

            if (header.Type == SimcpMcp)
            {
                if (payload)
                    AtMcpHandleMessage(connection, payload, header.PayloadLength);
            }
            else
            {
                if (payload)
                    PhFree(payload);

                AtConnectionClose(connection, SimcpCloseProtocolViolation, 0);
                break;
            }

            if (payload)
                PhFree(payload);

            PhDrainAutoPool(&autoPool);
        }

        // A close requested by another thread while a message was being handled. One this
        // thread issued itself (a protocol violation seen while pumping) already sent the frame.
        if (AtConnectionIsClosing(connection) && !connection->CloseSent)
            AtpSendClose(connection);
    }

    AtConsentReleaseConnection(connection);
    AtpUnregisterConnection(connection);

    // Disconnecting discards unread data; let the broker read the Close reason first.
    if (AtConnectionIsClosing(connection))
    {
        IO_STATUS_BLOCK isb;

        NtFlushBuffersFile(connection->PipeHandle, &isb);
    }

    PhDisconnectNamedPipe(connection->PipeHandle);

    PhDeleteAutoPool(&autoPool);
    PhDereferenceObject(connection);

    return STATUS_SUCCESS;
}

PAT_CONNECTION AtpCreateConnection(
    _In_ HANDLE PipeHandle
    )
{
    PAT_CONNECTION connection;

    connection = PhCreateObject(sizeof(AT_CONNECTION), AtConnectionType);
    memset(connection, 0, sizeof(AT_CONNECTION));
    connection->PipeHandle = PipeHandle;
    connection->ConnectionId = (ULONG)InterlockedIncrement((PLONG)&AtNextConnectionId) - 1;
    PhInitializeQueuedLock(&connection->Lock);
    InitializeListHead(&connection->DeferredRequests);
    PhQuerySystemTime(&connection->ConnectTime);

    return connection;
}

VOID AtpListenerFailed(
    _In_ NTSTATUS Status
    )
{
    PhAcquireQueuedLockExclusive(&AtServerLock);

    // A stop in progress already published Stopped; this thread is about to be joined.
    if (!ReadAcquire(&AtServerStopping))
    {
        AtServerState = AtServerFailed;
        AtServerStatus = Status;
    }

    PhReleaseQueuedLockExclusive(&AtServerLock);
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS NTAPI AtpListenerThread(
    _In_ PVOID Parameter
    )
{
    HANDLE pipeHandle = Parameter;

    while (TRUE)
    {
        NTSTATUS status;
        PAT_CONNECTION connection;

        status = PhListenNamedPipe(pipeHandle);

        if (ReadAcquire(&AtServerStopping))
        {
            NtClose(pipeHandle);
            break;
        }

        if (!NT_SUCCESS(status) && status != STATUS_PIPE_CONNECTED)
        {
            NtClose(pipeHandle);
            PhDelayExecution(250);

            if (!NT_SUCCESS(status = AtpCreatePipeInstance(FALSE, &pipeHandle)))
            {
                AtpListenerFailed(status);
                break;
            }

            continue;
        }

        connection = AtpCreateConnection(pipeHandle);

        // The thread owns the reference created above; this one covers storing the thread
        // handle after the thread has already started (it may even have finished).
        PhReferenceObject(connection);

        if (!NT_SUCCESS(PhCreateThreadEx(&connection->ThreadHandle, AtpConnectionThread, connection)))
        {
            PhDisconnectNamedPipe(pipeHandle);
            PhDereferenceObject(connection);
        }

        PhDereferenceObject(connection);

        if (!NT_SUCCESS(status = AtpCreatePipeInstance(FALSE, &pipeHandle)))
        {
            AtpListenerFailed(status);
            break;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS AtServerStart(
    VOID
    )
{
    NTSTATUS status;
    HANDLE pipeHandle;
    BOOLEAN elevated;

    PhAcquireQueuedLockExclusive(&AtServerLock);

    if (AtServerState == AtServerRunning)
    {
        PhReleaseQueuedLockExclusive(&AtServerLock);
        return STATUS_SUCCESS;
    }

    if (!AtConnectionType)
    {
        AtConnectionType = PhCreateObjectType(L"AgentToolsConnection", 0, AtpConnectionDeleteProcedure);
        InitializeListHead(&AtConnectionList);
    }

    elevated = !!PhGetOwnTokenAttributes().Elevated;

    PhClearReference(&AtPipeName);
    AtPipeName = AtpFormatPipeName(elevated);

    if (AtPipeSecurityDescriptor)
    {
        PhFree(AtPipeSecurityDescriptor);
        AtPipeSecurityDescriptor = NULL;
    }

    status = AtpCreatePipeSecurityDescriptor(
        !!PhGetIntegerSetting(SETTING_NAME_ALLOW_SANDBOXED_CLIENTS),
        &AtPipeSecurityDescriptor
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    // FILE_CREATE: a second instance of System Informer in this session, or a squatter, makes
    // this fail and the options page says so.
    status = AtpCreatePipeInstance(TRUE, &pipeHandle);

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    WriteRelease(&AtServerStopping, 0);

    status = PhCreateThreadEx(&AtListenerThreadHandle, AtpListenerThread, pipeHandle);

    if (!NT_SUCCESS(status))
    {
        NtClose(pipeHandle);
        goto CleanupExit;
    }

CleanupExit:
    AtServerStatus = status;
    AtServerElevated = elevated;

    if (NT_SUCCESS(status))
        AtServerState = AtServerRunning;
    else if (status == STATUS_OBJECT_NAME_COLLISION)
        AtServerState = AtServerFailedPipeExists;
    else
        AtServerState = AtServerFailed;

    PhReleaseQueuedLockExclusive(&AtServerLock);

    return status;
}

VOID AtpCancelAndWaitForThread(
    _In_ HANDLE ThreadHandle
    )
{
    LARGE_INTEGER timeout;

    while (TRUE)
    {
        IO_STATUS_BLOCK isb;

        NtCancelSynchronousIoFile(ThreadHandle, NULL, &isb);

        if (NtWaitForSingleObject(ThreadHandle, FALSE, PhTimeoutFromMilliseconds(&timeout, 100)) != STATUS_TIMEOUT)
            break;
    }
}

VOID AtServerStop(
    _In_ SIMCP_CLOSE_REASON Reason
    )
{
    PPH_LIST connections;
    HANDLE listenerThreadHandle;
    ULONG i;

    PhAcquireQueuedLockExclusive(&AtServerLock);

    if (AtServerState == AtServerStopped)
    {
        PhReleaseQueuedLockExclusive(&AtServerLock);
        return;
    }

    WriteRelease(&AtServerStopping, 1);

    listenerThreadHandle = AtListenerThreadHandle;
    AtListenerThreadHandle = NULL;
    AtServerState = AtServerStopped;
    AtServerStatus = STATUS_SUCCESS;

    PhReleaseQueuedLockExclusive(&AtServerLock);

    // Join the listener outside the lock: its failure path takes the lock to publish the
    // failure. Start and Stop are both driven from the main thread, so nothing restarts the
    // server before this returns.
    if (listenerThreadHandle)
    {
        AtpCancelAndWaitForThread(listenerThreadHandle);
        NtClose(listenerThreadHandle);
    }

    // Close every connection and wait for its thread; the plugin may be unloading.

    connections = AtServerSnapshotConnections();

    for (i = 0; i < connections->Count; i++)
    {
        PAT_CONNECTION connection = connections->Items[i];

        AtConnectionClose(connection, Reason, 0);
    }

    for (i = 0; i < connections->Count; i++)
    {
        PAT_CONNECTION connection = connections->Items[i];

        if (connection->ThreadHandle)
            AtpCancelAndWaitForThread(connection->ThreadHandle);

        PhDereferenceObject(connection);
    }

    PhDereferenceObject(connections);
}

AT_SERVER_STATE AtServerGetState(
    _Out_opt_ PNTSTATUS Status,
    _Out_opt_ PBOOLEAN Elevated
    )
{
    AT_SERVER_STATE state;

    PhAcquireQueuedLockExclusive(&AtServerLock);
    state = AtServerState;
    if (Status) *Status = AtServerStatus;
    if (Elevated) *Elevated = AtServerElevated;
    PhReleaseQueuedLockExclusive(&AtServerLock);

    return state;
}

PPH_LIST AtServerSnapshotConnections(
    VOID
    )
{
    PPH_LIST list;
    PLIST_ENTRY entry;

    list = PhCreateList(4);

    if (!AtConnectionType)
        return list;

    PhAcquireQueuedLockExclusive(&AtConnectionListLock);

    for (entry = AtConnectionList.Flink; entry != &AtConnectionList; entry = entry->Flink)
    {
        PAT_CONNECTION connection = CONTAINING_RECORD(entry, AT_CONNECTION, ListEntry);

        PhReferenceObject(connection);
        PhAddItemList(list, connection);
    }

    PhReleaseQueuedLockExclusive(&AtConnectionListLock);

    return list;
}

VOID AtServerDisconnect(
    _In_ ULONG ConnectionId
    )
{
    PPH_LIST connections;
    ULONG i;

    connections = AtServerSnapshotConnections();

    for (i = 0; i < connections->Count; i++)
    {
        PAT_CONNECTION connection = connections->Items[i];

        if (connection->ConnectionId == ConnectionId)
        {
            AtConnectionClose(connection, SimcpCloseUserDisconnected, 0);

            // Disconnecting also voids every grant held by this session.
            PhAcquireQueuedLockExclusive(&connection->Lock);
            memset(connection->SessionPolicy, 0, sizeof(connection->SessionPolicy));
            PhReleaseQueuedLockExclusive(&connection->Lock);
        }

        PhDereferenceObject(connection);
    }

    PhDereferenceObject(connections);
}
