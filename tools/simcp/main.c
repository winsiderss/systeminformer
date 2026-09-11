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

#include <ph.h>
#include <phconsole.h>
#include <verify.h>
#include <simcp.h>
#include "envelope.h"
#include "pending.h"

#define SIMCP_JSONRPC_ERROR_TRANSPORT 1000
#define SIMCP_CONNECT_ATTEMPTS 3
#define SIMCP_CONNECT_WAIT_MS 2000
#define SIMCP_RECONNECT_BACKOFF_FIRST_MS 250
#define SIMCP_RECONNECT_BACKOFF_MAX_MS 5000
#define SIMCP_RECONNECT_GRACE_MS 5000
#define SIMCP_OPTION_NO_RECONNECT 1

static HANDLE SimcpStdInput = NULL;
static HANDLE SimcpStdOutput = NULL;
static HANDLE SimcpStdError = NULL;
static PH_QUEUED_LOCK SimcpStdOutputLock = PH_QUEUED_LOCK_INIT;
static SIMCP_PENDING SimcpPending = { 0 };

typedef struct _SIMCP_LINK
{
    PH_QUEUED_LOCK Lock;
    HANDLE Handle;
    ULONG Generation;
    PH_QUEUED_LOCK SendLock;
    HANDLE WriteEvent;
    HANDLE ReadEvent;
    HANDLE ConnectedEvent;
    HANDLE AbortEvent;
    HANDLE LauncherHandle;
    BOOLEAN EverConnected;
} SIMCP_LINK, *PSIMCP_LINK;

static SIMCP_LINK SimcpLink = { 0 };

typedef struct _SIMCP_SESSION
{
    PH_QUEUED_LOCK Lock;
    PPH_BYTES InitializeLine;
    PPH_STRING ProtocolVersion;
    BOOLEAN InitializedSeen;
    BOOLEAN Modern;
} SIMCP_SESSION, *PSIMCP_SESSION;

static SIMCP_SESSION SimcpSession = { 0 };
static BOOLEAN SimcpNoReconnect = FALSE;

VOID SimcpWriteAll(
    _In_ HANDLE FileHandle,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    ULONG offset = 0;

    while (offset < Length)
    {
        ULONG written = 0;

        if (!WriteFile(FileHandle, PTR_ADD_OFFSET(Buffer, offset), Length - offset, &written, NULL))
            return;
        if (written == 0)
            return;

        offset += written;
    }
}

VOID SimcpLog(
    _In_ PCSTR Message
    )
{
    static CONST CHAR prefix[] = "simcp: ";
    static CONST CHAR newline[] = "\n";

    if (!SimcpStdError)
        return;

    SimcpWriteAll(SimcpStdError, (PVOID)prefix, sizeof(prefix) - 1);
    SimcpWriteAll(SimcpStdError, (PVOID)Message, (ULONG)strlen(Message));
    SimcpWriteAll(SimcpStdError, (PVOID)newline, sizeof(newline) - 1);
}

VOID SimcpWriteLine(
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    static CONST CHAR newline[] = "\n";

    PhAcquireQueuedLockExclusive(&SimcpStdOutputLock);
    SimcpWriteAll(SimcpStdOutput, Buffer, Length);
    SimcpWriteAll(SimcpStdOutput, (PVOID)newline, sizeof(newline) - 1);
    PhReleaseQueuedLockExclusive(&SimcpStdOutputLock);
}

VOID SimcpEmitError(
    _In_ PCSTR Message
    )
{
    static CONST CHAR head[] = "{\"jsonrpc\":\"2.0\",\"id\":null,\"error\":{\"code\":1000,\"message\":\"System Informer: ";
    static CONST CHAR tail[] = "\"}}";
    PH_BYTES_BUILDER builder;
    PPH_BYTES line;

    static_assert(SIMCP_JSONRPC_ERROR_TRANSPORT == 1000, "error code literal must match");

    PhInitializeBytesBuilder(&builder, 256);
    PhAppendBytesBuilderEx(&builder, (PVOID)head, sizeof(head) - 1, 0, NULL);
    PhAppendBytesBuilderEx(&builder, (PVOID)Message, strlen(Message), 0, NULL);
    PhAppendBytesBuilderEx(&builder, (PVOID)tail, sizeof(tail) - 1, 0, NULL);
    line = PhFinalBytesBuilderBytes(&builder);

    SimcpWriteLine(line->Buffer, (ULONG)line->Length);
    SimcpLog(Message);

    PhDereferenceObject(line);
}

VOID SimcpEmitTransportError(
    _In_ PPH_BYTES IdJson,
    _In_ PCSTR Message
    )
{
    static CONST CHAR head[] = "{\"jsonrpc\":\"2.0\",\"id\":";
    static CONST CHAR middle[] = ",\"error\":{\"code\":1000,\"message\":\"System Informer: ";
    static CONST CHAR tail[] = "\"}}";
    PH_BYTES_BUILDER builder;
    PPH_BYTES line;

    static_assert(SIMCP_JSONRPC_ERROR_TRANSPORT == 1000, "error code literal must match");

    PhInitializeBytesBuilder(&builder, 256);
    PhAppendBytesBuilderEx(&builder, (PVOID)head, sizeof(head) - 1, 0, NULL);
    PhAppendBytesBuilderEx(&builder, IdJson->Buffer, IdJson->Length, 0, NULL);
    PhAppendBytesBuilderEx(&builder, (PVOID)middle, sizeof(middle) - 1, 0, NULL);
    PhAppendBytesBuilderEx(&builder, (PVOID)Message, strlen(Message), 0, NULL);
    PhAppendBytesBuilderEx(&builder, (PVOID)tail, sizeof(tail) - 1, 0, NULL);
    line = PhFinalBytesBuilderBytes(&builder);

    SimcpWriteLine(line->Buffer, (ULONG)line->Length);

    PhDereferenceObject(line);
}

VOID SimcpEmitPong(
    _In_ PPH_BYTES IdJson
    )
{
    static CONST CHAR head[] = "{\"jsonrpc\":\"2.0\",\"id\":";
    static CONST CHAR tail[] = ",\"result\":{}}";
    PH_BYTES_BUILDER builder;
    PPH_BYTES line;

    PhInitializeBytesBuilder(&builder, 64);
    PhAppendBytesBuilderEx(&builder, (PVOID)head, sizeof(head) - 1, 0, NULL);
    PhAppendBytesBuilderEx(&builder, IdJson->Buffer, IdJson->Length, 0, NULL);
    PhAppendBytesBuilderEx(&builder, (PVOID)tail, sizeof(tail) - 1, 0, NULL);
    line = PhFinalBytesBuilderBytes(&builder);

    SimcpWriteLine(line->Buffer, (ULONG)line->Length);

    PhDereferenceObject(line);
}

VOID SimcpEmitListChanged(
    VOID
    )
{
    static CONST CHAR tools[] = "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/tools/list_changed\"}";
    static CONST CHAR resources[] = "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/resources/list_changed\"}";
    static CONST CHAR prompts[] = "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/prompts/list_changed\"}";

    SimcpWriteLine((PVOID)tools, sizeof(tools) - 1);
    SimcpWriteLine((PVOID)resources, sizeof(resources) - 1);
    SimcpWriteLine((PVOID)prompts, sizeof(prompts) - 1);
}

DECLSPEC_NORETURN
VOID SimcpFail(
    _In_ PCSTR Message
    )
{
    SimcpEmitError(Message);
    RtlExitUserProcess(1);
}

PCSTR SimcpHelloStatusToString(
    _In_ ULONG Status
    )
{
    switch (Status)
    {
    case SimcpHelloRejectedVersion:
        return "the broker and System Informer versions do not match; reinstall System Informer";
    case SimcpHelloRejectedUser:
        return "the agent is running as a different user than System Informer";
    case SimcpHelloRejectedIntegrity:
        return "the agent integrity level is too low";
    case SimcpHelloRejectedAppContainer:
        return "sandboxed clients are disabled in the AgentTools options";
    case SimcpHelloRejectedImage:
        return "the broker must be run from the System Informer installation directory";
    case SimcpHelloRejectedSignature:
        return "the broker signature could not be verified";
    case SimcpHelloRejectedProcessId:
        return "the handshake process id did not match the pipe client";
    case SimcpHelloRejectedByUser:
        return "the user did not allow this agent to connect to System Informer";
    default:
        return "the connection was rejected";
    }
}

PCSTR SimcpCloseReasonToString(
    _In_ PSIMCP_CLOSE Close
    )
{
    switch (Close->Reason)
    {
    case SimcpCloseUserDisconnected:
        return "the user disconnected this agent";
    case SimcpCloseServerDisabled:
        return "the agent tools server was disabled";
    case SimcpCloseServerShutdown:
        return "System Informer is shutting down";
    case SimcpCloseProtocolViolation:
        return "protocol violation";
    case SimcpCloseTransportError:
        return "System Informer could not write to this connection";
    case SimcpCloseRejected:
        return SimcpHelloStatusToString(Close->Detail);
    default:
        return "the connection was closed";
    }
}

BOOLEAN SimcpCloseReasonIsTerminal(
    _In_ PSIMCP_CLOSE Close
    )
{
    switch (Close->Reason)
    {
    case SimcpCloseUserDisconnected:
    case SimcpCloseProtocolViolation:
    case SimcpCloseRejected:
        return TRUE;
    default:
        return FALSE;
    }
}

NTSTATUS SimcpPipeIo(
    _In_ HANDLE PipeHandle,
    _In_ HANDLE EventHandle,
    _In_ BOOLEAN Write,
    _Inout_updates_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _Out_ PULONG Transferred
    )
{
    NTSTATUS status;
    IO_STATUS_BLOCK isb;

    memset(&isb, 0, sizeof(IO_STATUS_BLOCK));
    *Transferred = 0;

    if (Write)
        status = NtWriteFile(PipeHandle, EventHandle, NULL, NULL, &isb, Buffer, Length, NULL, NULL);
    else
        status = NtReadFile(PipeHandle, EventHandle, NULL, NULL, &isb, Buffer, Length, NULL, NULL);

    if (status == STATUS_PENDING)
    {
        NtWaitForSingleObject(EventHandle, FALSE, NULL);
        status = isb.Status;
    }

    if (NT_SUCCESS(status))
        *Transferred = (ULONG)isb.Information;

    return status;
}

NTSTATUS SimcpCreateIoEvent(
    _Out_ PHANDLE EventHandle
    )
{
    return NtCreateEvent(EventHandle, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);
}

NTSTATUS SimcpReadExact(
    _In_ HANDLE PipeHandle,
    _In_ HANDLE EventHandle,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    NTSTATUS status;
    ULONG offset = 0;

    while (offset < Length)
    {
        ULONG bytesRead = 0;

        status = SimcpPipeIo(PipeHandle, EventHandle, FALSE, PTR_ADD_OFFSET(Buffer, offset), Length - offset, &bytesRead);

        if (!NT_SUCCESS(status))
            return status;
        if (bytesRead == 0)
            return STATUS_PIPE_BROKEN;

        offset += bytesRead;
    }

    return STATUS_SUCCESS;
}

NTSTATUS SimcpWriteAllPipe(
    _In_ HANDLE PipeHandle,
    _In_ HANDLE EventHandle,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    NTSTATUS status;
    ULONG offset = 0;

    while (offset < Length)
    {
        ULONG written = 0;

        status = SimcpPipeIo(PipeHandle, EventHandle, TRUE, PTR_ADD_OFFSET(Buffer, offset), Length - offset, &written);

        if (!NT_SUCCESS(status))
            return status;
        if (written == 0)
            return STATUS_PIPE_BROKEN;

        offset += written;
    }

    return STATUS_SUCCESS;
}

NTSTATUS SimcpLinkInitialize(
    VOID
    )
{
    NTSTATUS status;

    PhInitializeQueuedLock(&SimcpLink.Lock);
    PhInitializeQueuedLock(&SimcpLink.SendLock);

    status = SimcpCreateIoEvent(&SimcpLink.ReadEvent);

    if (NT_SUCCESS(status))
        status = SimcpCreateIoEvent(&SimcpLink.WriteEvent);
    if (NT_SUCCESS(status))
        status = NtCreateEvent(&SimcpLink.ConnectedEvent, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);
    if (NT_SUCCESS(status))
        status = NtCreateEvent(&SimcpLink.AbortEvent, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE);

    if (NT_SUCCESS(status))
    {
        PROCESS_BASIC_INFORMATION basicInfo;

        // Opened once, so the handle cannot later name a different process that reused the id.
        if (NT_SUCCESS(PhGetProcessBasicInformation(NtCurrentProcess(), &basicInfo)))
        {
            if (!NT_SUCCESS(PhOpenProcess(
                &SimcpLink.LauncherHandle,
                SYNCHRONIZE,
                (HANDLE)basicInfo.InheritedFromUniqueProcessId
                )))
            {
                SimcpLink.LauncherHandle = NULL;
            }
        }
    }

    return status;
}

VOID SimcpLinkPublish(
    _In_ HANDLE PipeHandle
    )
{
    PhAcquireQueuedLockExclusive(&SimcpLink.Lock);
    SimcpLink.Handle = PipeHandle;
    SimcpLink.Generation++;
    PhReleaseQueuedLockExclusive(&SimcpLink.Lock);
}

ULONG SimcpLinkGeneration(
    VOID
    )
{
    ULONG generation;

    PhAcquireQueuedLockShared(&SimcpLink.Lock);
    generation = SimcpLink.Generation;
    PhReleaseQueuedLockShared(&SimcpLink.Lock);

    return generation;
}

VOID SimcpLinkConnected(
    VOID
    )
{
    PhAcquireQueuedLockExclusive(&SimcpLink.Lock);
    SimcpLink.EverConnected = TRUE;
    PhReleaseQueuedLockExclusive(&SimcpLink.Lock);

    NtSetEvent(SimcpLink.ConnectedEvent, NULL);
}

BOOLEAN SimcpLinkEverConnected(
    VOID
    )
{
    BOOLEAN everConnected;

    PhAcquireQueuedLockShared(&SimcpLink.Lock);
    everConnected = SimcpLink.EverConnected;
    PhReleaseQueuedLockShared(&SimcpLink.Lock);

    return everConnected;
}

VOID SimcpLinkAbort(
    VOID
    )
{
    NtSetEvent(SimcpLink.AbortEvent, NULL);
}

VOID SimcpLinkTeardown(
    VOID
    )
{
    HANDLE handle;
    IO_STATUS_BLOCK isb;
    PPH_LIST pending;
    ULONG i;

    PhAcquireQueuedLockShared(&SimcpLink.Lock);
    handle = SimcpLink.Handle;
    PhReleaseQueuedLockShared(&SimcpLink.Lock);

    if (!handle)
        return;

    NtResetEvent(SimcpLink.ConnectedEvent, NULL);
    NtCancelIoFileEx(handle, NULL, &isb);

    PhAcquireQueuedLockExclusive(&SimcpLink.SendLock);
    PhAcquireQueuedLockExclusive(&SimcpLink.Lock);
    SimcpLink.Handle = NULL;
    PhReleaseQueuedLockExclusive(&SimcpLink.Lock);
    PhReleaseQueuedLockExclusive(&SimcpLink.SendLock);

    NtClose(handle);

    // Every request that was on the wire is owed exactly one answer, and this is the moment the
    // broker knows it will never arrive. They are never replayed: a tool call is at-most-once.
    pending = SimcpTakePending(&SimcpPending);

    for (i = 0; i < pending->Count; i++)
    {
        PPH_BYTES id = pending->Items[i];

        SimcpEmitTransportError(id, "the connection was lost before this call finished");
        PhDereferenceObject(id);
    }

    PhDereferenceObject(pending);
}

BOOLEAN SimcpLinkWaitConnected(
    _In_ ULONG TimeoutMs
    )
{
    HANDLE handles[3];
    ULONG count;
    LARGE_INTEGER timeout;
    NTSTATUS status;

    handles[0] = SimcpLink.ConnectedEvent;
    handles[1] = SimcpLink.AbortEvent;
    count = 2;

    // Nothing sets AbortEvent while the supervisor is still retrying, so a wait with no deadline
    // would otherwise never end: the thread parked here is the same one that reads stdin, so it
    // cannot notice the host going away. Waking on the launcher returns it to that read, which
    // then sees end of file and unwinds. Only the unbounded wait needs this -- a bounded one
    // already returns on its own, and a launcher that dies mid-session must not end a live link.
    if (TimeoutMs == INFINITE && SimcpLink.LauncherHandle)
        handles[count++] = SimcpLink.LauncherHandle;

    status = NtWaitForMultipleObjects(
        count,
        handles,
        WaitAny,
        FALSE,
        TimeoutMs == INFINITE ? NULL : PhTimeoutFromMilliseconds(&timeout, TimeoutMs)
        );

    return status == STATUS_WAIT_0;
}

BOOLEAN SimcpLinkAborted(
    VOID
    )
{
    LARGE_INTEGER timeout = { 0 };

    return NtWaitForSingleObject(SimcpLink.AbortEvent, FALSE, &timeout) == STATUS_WAIT_0;
}

NTSTATUS SimcpLinkSend(
    _In_ USHORT Type,
    _In_reads_bytes_opt_(PayloadLength) PVOID Payload,
    _In_ ULONG PayloadLength,
    _In_ BOOLEAN WaitForConnected,
    _Out_opt_ PULONG Generation
    )
{
    NTSTATUS status;
    SIMCP_HEADER header;
    HANDLE handle;

    // During an outage a line waits only as long as a fast restart takes. Before the first
    // connect there is no session to protect and no call in flight, so it waits for System
    // Informer to turn up rather than being refused on a deadline it cannot know about.
    if (WaitForConnected &&
        !SimcpLinkWaitConnected(SimcpLinkEverConnected() ? SIMCP_RECONNECT_GRACE_MS : INFINITE))
    {
        return STATUS_PIPE_DISCONNECTED;
    }

    memset(&header, 0, sizeof(SIMCP_HEADER));
    header.Magic = SIMCP_MAGIC;
    header.Version = SIMCP_VERSION;
    header.Type = Type;
    header.PayloadLength = PayloadLength;

    PhAcquireQueuedLockExclusive(&SimcpLink.SendLock);

    PhAcquireQueuedLockShared(&SimcpLink.Lock);
    handle = SimcpLink.Handle;

    if (Generation)
        *Generation = SimcpLink.Generation;

    PhReleaseQueuedLockShared(&SimcpLink.Lock);

    if (handle)
    {
        status = SimcpWriteAllPipe(handle, SimcpLink.WriteEvent, &header, sizeof(SIMCP_HEADER));

        if (NT_SUCCESS(status) && Payload && PayloadLength)
            status = SimcpWriteAllPipe(handle, SimcpLink.WriteEvent, Payload, PayloadLength);
    }
    else
    {
        status = STATUS_PIPE_DISCONNECTED;
    }

    PhReleaseQueuedLockExclusive(&SimcpLink.SendLock);

    return status;
}

NTSTATUS SimcpReadEnvelope(
    _In_ HANDLE PipeHandle,
    _In_ HANDLE EventHandle,
    _Out_ PSIMCP_HEADER Header,
    _Outptr_result_maybenull_ PVOID* Payload
    )
{
    NTSTATUS status;
    PVOID payload = NULL;

    status = SimcpReadExact(PipeHandle, EventHandle, Header, sizeof(SIMCP_HEADER));

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
        status = SimcpReadExact(PipeHandle, EventHandle, payload, Header->PayloadLength);

        if (!NT_SUCCESS(status))
        {
            PhFree(payload);
            return status;
        }
    }

    *Payload = payload;
    return STATUS_SUCCESS;
}

static BOOLEAN SimcpValidateServer(
    _In_ HANDLE PipeHandle,
    _Out_ PCSTR* FailureMessage
    );

NTSTATUS SimcpConnectPipe(
    _Out_ PHANDLE PipeHandle,
    _Out_ PCSTR* ValidationFailure
    )
{
    static CONST PH_STRINGREF deviceName = PH_STRINGREF_INIT(DEVICE_NAMED_PIPE);
    static CONST PH_STRINGREF protectedPrefix = PH_STRINGREF_INIT(SIMCP_PIPE_PROTECTED_PREFIX);
    static CONST PH_STRINGREF namePrefix = PH_STRINGREF_INIT(SIMCP_PIPE_NAME_PREFIX);
    NTSTATUS status = STATUS_OBJECT_NAME_NOT_FOUND;
    PPH_STRING sessionName;
    PH_FORMAT format[1];
    ULONG i;

    *ValidationFailure = NULL;

    PhInitFormatU(&format[0], NtCurrentPeb()->SessionId);
    sessionName = PhFormat(format, RTL_NUMBER_OF(format), 16);

    for (i = 0; i < 2; i++)
    {
        PPH_STRING relativeName;
        PPH_STRING fullName;
        UNICODE_STRING fullNameUs;
        OBJECT_ATTRIBUTES objectAttributes;
        IO_STATUS_BLOCK isb;
        ULONG attempt;
        SECURITY_QUALITY_OF_SERVICE securityQos =
        {
            sizeof(SECURITY_QUALITY_OF_SERVICE),
            SecurityImpersonation,
            SECURITY_STATIC_TRACKING,
            FALSE
        };

        if (i == 0)
            relativeName = PhConcatStringRef3(&protectedPrefix, &namePrefix, &sessionName->sr);
        else
            relativeName = PhConcatStringRef2(&namePrefix, &sessionName->sr);

        fullName = PhConcatStringRef2(&deviceName, &relativeName->sr);

        if (!PhStringRefToUnicodeString(&fullName->sr, &fullNameUs))
        {
            PhDereferenceObject(fullName);
            PhDereferenceObject(relativeName);
            status = STATUS_NAME_TOO_LONG;
            continue;
        }

        // The server impersonates us to read our token; the server side never keeps the
        // impersonation, so identification level would also do, but impersonation is what the
        // FSCTL_PIPE_IMPERSONATE path expects.
        InitializeObjectAttributesEx(
            &objectAttributes,
            &fullNameUs,
            OBJ_CASE_INSENSITIVE,
            NULL,
            NULL,
            &securityQos
            );

        for (attempt = 0; attempt < SIMCP_CONNECT_ATTEMPTS; attempt++)
        {
            status = NtCreateFile(
                PipeHandle,
                FILE_GENERIC_READ | FILE_GENERIC_WRITE | SYNCHRONIZE,
                &objectAttributes,
                &isb,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_NON_DIRECTORY_FILE, // asynchronous: see SimcpPipeIo
                NULL,
                0
                );

            if (status != STATUS_PIPE_NOT_AVAILABLE)
                break;

            // All instances are busy; the listener creates a new one after each accept.
            PhWaitForNamedPipe(relativeName->Buffer, SIMCP_CONNECT_WAIT_MS);
        }

        PhDereferenceObject(fullName);
        PhDereferenceObject(relativeName);

        if (NT_SUCCESS(status))
        {
            PCSTR failureMessage;

            if (SimcpValidateServer(*PipeHandle, &failureMessage))
                break;

            // Not our server: remember why, drop it, try the next candidate.
            *ValidationFailure = failureMessage;
            NtClose(*PipeHandle);
            *PipeHandle = NULL;
            status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

    PhDereferenceObject(sessionName);
    return status;
}

BOOLEAN SimcpValidateServer(
    _In_ HANDLE PipeHandle,
    _Out_ PCSTR* FailureMessage
    )
{
    static CONST PH_STRINGREF serverFileName = PH_STRINGREF_INIT(SIMCP_SERVER_FILE_NAME);
    BOOLEAN result = FALSE;
    HANDLE serverProcessId;
    HANDLE processHandle = NULL;
    PPH_STRING remoteFileName = NULL;
    PPH_STRING expectedFileName = NULL;
    PPH_STRING directory;

    *FailureMessage = "the pipe server could not be validated";

    if (!NT_SUCCESS(PhGetNamedPipeServerProcessId(PipeHandle, &serverProcessId)))
        goto CleanupExit;

    if (!NT_SUCCESS(PhOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, serverProcessId)))
        goto CleanupExit;

    if (!NT_SUCCESS(PhGetProcessImageFileNameWin32(processHandle, &remoteFileName)))
        goto CleanupExit;

    if (!(directory = PhGetApplicationDirectoryWin32()))
        goto CleanupExit;

    expectedFileName = PhConcatStringRef2(&directory->sr, &serverFileName);
    PhDereferenceObject(directory);

    if (!PhEqualString(remoteFileName, expectedFileName, TRUE))
    {
        *FailureMessage = "the pipe server is not the System Informer this broker was installed with";
        goto CleanupExit;
    }

    if (!PhVerifyFileIsSystemInformer(&remoteFileName->sr, FALSE))
    {
        *FailureMessage = "the System Informer signature could not be verified";
        goto CleanupExit;
    }

    result = TRUE;

CleanupExit:
    if (expectedFileName)
        PhDereferenceObject(expectedFileName);
    if (remoteFileName)
        PhDereferenceObject(remoteFileName);
    if (processHandle)
        NtClose(processHandle);

    return result;
}

VOID SimcpFillHello(
    _Out_ PSIMCP_HELLO Hello
    )
{
    PROCESS_BASIC_INFORMATION basicInfo;

    memset(Hello, 0, sizeof(SIMCP_HELLO));
    Hello->BrokerVersion = SIMCP_VERSION;
    Hello->BrokerProcessId = HandleToUlong(NtCurrentProcessId());

    if (NT_SUCCESS(PhGetProcessBasicInformation(NtCurrentProcess(), &basicInfo)))
    {
        HANDLE parentHandle;

        Hello->LauncherProcessId = HandleToUlong((HANDLE)basicInfo.InheritedFromUniqueProcessId);

        if (NT_SUCCESS(PhOpenProcess(
            &parentHandle,
            PROCESS_QUERY_LIMITED_INFORMATION,
            (HANDLE)basicInfo.InheritedFromUniqueProcessId
            )))
        {
            KERNEL_USER_TIMES times;

            if (NT_SUCCESS(PhGetProcessTimes(parentHandle, &times)))
            {
                Hello->LauncherStartTime = times.CreateTime;
            }

            NtClose(parentHandle);
        }
    }
}

VOID SimcpSessionObserveOutgoing(
    _In_ PSIMCP_ENVELOPE Envelope,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    PhAcquireQueuedLockExclusive(&SimcpSession.Lock);

    if (Envelope->ModernMeta)
        SimcpSession.Modern = TRUE;

    if (Envelope->Kind == SimcpEnvelopeRequest &&
        PhEqualString2(Envelope->Method, L"initialize", FALSE))
    {
        PhMoveReference(&SimcpSession.InitializeLine, PhCreateBytesEx(Buffer, Length));
    }
    else if (Envelope->Kind == SimcpEnvelopeNotification &&
        PhEqualString2(Envelope->Method, L"notifications/initialized", FALSE))
    {
        SimcpSession.InitializedSeen = TRUE;
    }

    PhReleaseQueuedLockExclusive(&SimcpSession.Lock);
}

VOID SimcpSessionObserveIncoming(
    _In_ PSIMCP_ENVELOPE Envelope
    )
{
    if (!Envelope->ProtocolVersion)
        return;

    PhAcquireQueuedLockExclusive(&SimcpSession.Lock);

    if (!SimcpSession.ProtocolVersion)
        PhSetReference(&SimcpSession.ProtocolVersion, Envelope->ProtocolVersion);

    PhReleaseQueuedLockExclusive(&SimcpSession.Lock);
}

BOOLEAN SimcpReplaySession(
    _In_ HANDLE PipeHandle,
    _In_ ULONG Generation,
    _Out_ PCSTR *Message,
    _Out_ PBOOLEAN Terminal
    )
{
    static CONST CHAR initialized[] = "{\"jsonrpc\":\"2.0\",\"method\":\"notifications/initialized\"}";
    BOOLEAN modern;
    BOOLEAN initializedSeen;
    PPH_BYTES line = NULL;
    PPH_BYTES replay;
    PPH_STRING expected = NULL;
    PH_FORMAT format[3];
    PPH_STRING idString;
    PPH_BYTES idBytes;

    *Message = NULL;
    *Terminal = FALSE;

    PhAcquireQueuedLockShared(&SimcpSession.Lock);
    modern = SimcpSession.Modern;
    initializedSeen = SimcpSession.InitializedSeen;
    if (SimcpSession.InitializeLine)
        line = PhReferenceObject(SimcpSession.InitializeLine);
    if (SimcpSession.ProtocolVersion)
        PhSetReference(&expected, SimcpSession.ProtocolVersion);
    PhReleaseQueuedLockShared(&SimcpSession.Lock);

    if (modern || !line)
    {
        if (line)
            PhDereferenceObject(line);
        if (expected)
            PhDereferenceObject(expected);

        return TRUE;
    }

    PhInitFormatS(&format[0], L"si-");
    PhInitFormatU(&format[1], Generation);
    PhInitFormatS(&format[2], L"-init");
    idString = PhFormat(format, RTL_NUMBER_OF(format), 16);
    idBytes = PhConvertUtf16ToUtf8Ex(idString->Buffer, idString->Length);
    PhDereferenceObject(idString);

    replay = SimcpRewriteEnvelopeId(line->Buffer, (ULONG)line->Length, idBytes->Buffer);
    PhDereferenceObject(line);

    if (!replay)
    {
        PhDereferenceObject(idBytes);
        if (expected)
            PhDereferenceObject(expected);

        *Message = "the cached handshake could not be replayed";
        return FALSE;
    }

    if (!NT_SUCCESS(SimcpLinkSend(SimcpMcp, replay->Buffer, (ULONG)replay->Length, FALSE, NULL)))
    {
        PhDereferenceObject(replay);
        PhDereferenceObject(idBytes);
        if (expected)
            PhDereferenceObject(expected);

        *Message = "System Informer closed the connection during the handshake";
        return FALSE;
    }

    PhDereferenceObject(replay);

    // Read until our own reply comes back. Anything else the backend volunteers first is relayed,
    // because it belongs to the host.
    while (TRUE)
    {
        NTSTATUS status;
        SIMCP_HEADER header;
        PVOID payload;
        SIMCP_ENVELOPE envelope;
        BOOLEAN mine;

        status = SimcpReadEnvelope(PipeHandle, SimcpLink.ReadEvent, &header, &payload);

        if (!NT_SUCCESS(status))
        {
            PhDereferenceObject(idBytes);
            if (expected)
                PhDereferenceObject(expected);

            *Message = "System Informer closed the connection during the handshake";
            return FALSE;
        }

        if (header.Type != SimcpMcp || !payload)
        {
            if (payload)
                PhFree(payload);

            continue;
        }

        SimcpParseEnvelope(payload, header.PayloadLength, &envelope);

        mine = envelope.Kind == SimcpEnvelopeResponse && envelope.Id &&
            envelope.Id->Length == idBytes->Length + 2 &&
            memcmp(PTR_ADD_OFFSET(envelope.Id->Buffer, 1), idBytes->Buffer, idBytes->Length) == 0;

        if (!mine)
        {
            SimcpWriteLine(payload, header.PayloadLength);
            SimcpDeleteEnvelope(&envelope);
            PhFree(payload);
            continue;
        }

        // The host already has an answer to its own initialize; this one is ours and stops here.
        if (expected && envelope.ProtocolVersion &&
            !PhEqualString(expected, envelope.ProtocolVersion, FALSE))
        {
            SimcpDeleteEnvelope(&envelope);
            PhFree(payload);
            PhDereferenceObject(idBytes);
            PhDereferenceObject(expected);

            // The host is in a session this backend cannot honour; asking again will not help.
            *Terminal = TRUE;
            // The host is in a session this backend cannot honour; asking again will not help.
            *Terminal = TRUE;
            *Message = "System Informer changed protocol version; reconnect the agent";
            return FALSE;
        }

        SimcpDeleteEnvelope(&envelope);
        PhFree(payload);
        break;
    }

    PhDereferenceObject(idBytes);
    if (expected)
        PhDereferenceObject(expected);

    if (initializedSeen)
    {
        if (!NT_SUCCESS(SimcpLinkSend(SimcpMcp, (PVOID)initialized, sizeof(initialized) - 1, FALSE, NULL)))
        {
            *Message = "System Informer closed the connection during the handshake";
            return FALSE;
        }
    }

    return TRUE;
}

typedef enum _SIMCP_ESTABLISH_RESULT
{
    SimcpEstablishConnected,
    SimcpEstablishRetry,
    SimcpEstablishTerminal,
} SIMCP_ESTABLISH_RESULT;

SIMCP_ESTABLISH_RESULT SimcpEstablish(
    _Out_ PHANDLE PipeHandle,
    _Out_ PCSTR *Message
    )
{
    NTSTATUS status;
    HANDLE pipeHandle = NULL;
    PCSTR failureMessage;
    SIMCP_HELLO hello;
    SIMCP_HEADER header;
    PVOID payload;
    SIMCP_HELLO_ACK helloAck;
    BOOLEAN replayTerminal;

    *PipeHandle = NULL;
    *Message = NULL;

    status = SimcpConnectPipe(&pipeHandle, &failureMessage);

    if (!NT_SUCCESS(status))
    {
        // Nothing here is terminal: System Informer may not have started yet, and a squatter
        // holding the name must not be able to end the session by failing validation.
        if (failureMessage)
            *Message = failureMessage;
        else if (status == STATUS_OBJECT_NAME_NOT_FOUND || status == STATUS_OBJECT_PATH_NOT_FOUND)
            *Message = "System Informer is not running in this session or the agent tools server is not enabled";
        else if (status == STATUS_ACCESS_DENIED)
            *Message = "access to the System Informer agent pipe was denied";
        else
            *Message = "unable to connect to the System Informer agent pipe";

        return SimcpEstablishRetry;
    }

    // Published before the handshake so the frames below have a generation to belong to; the
    // stdin relay still cannot use it until Connected is set.
    SimcpLinkPublish(pipeHandle);

    SimcpFillHello(&hello);

    if (!NT_SUCCESS(SimcpLinkSend(SimcpHello, &hello, sizeof(SIMCP_HELLO), FALSE, NULL)))
    {
        SimcpLinkTeardown();
        *Message = "System Informer closed the connection during the handshake";
        return SimcpEstablishRetry;
    }

    if (!NT_SUCCESS(SimcpReadEnvelope(pipeHandle, SimcpLink.ReadEvent, &header, &payload)))
    {
        SimcpLinkTeardown();
        *Message = "System Informer closed the connection during the handshake";
        return SimcpEstablishRetry;
    }

    if (header.Type == SimcpClose)
    {
        SIMCP_CLOSE close;

        memset(&close, 0, sizeof(SIMCP_CLOSE));

        if (payload && header.PayloadLength >= sizeof(SIMCP_CLOSE))
            memcpy(&close, payload, sizeof(SIMCP_CLOSE));

        if (payload)
            PhFree(payload);

        SimcpLinkTeardown();
        *Message = SimcpCloseReasonToString(&close);
        return SimcpCloseReasonIsTerminal(&close) ? SimcpEstablishTerminal : SimcpEstablishRetry;
    }

    if (header.Type != SimcpHelloAck || !payload || header.PayloadLength < sizeof(SIMCP_HELLO_ACK))
    {
        if (payload)
            PhFree(payload);

        SimcpLinkTeardown();
        *Message = "unexpected handshake reply from System Informer";
        return SimcpEstablishTerminal;
    }

    memcpy(&helloAck, payload, sizeof(SIMCP_HELLO_ACK));
    PhFree(payload);

    // A rejected handshake is a decision, not an outage: retrying would only ask again.
    if (helloAck.Status != SimcpHelloAccepted)
    {
        SimcpLinkTeardown();
        *Message = SimcpHelloStatusToString(helloAck.Status);
        return SimcpEstablishTerminal;
    }

    if (!SimcpReplaySession(pipeHandle, SimcpLinkGeneration(), Message, &replayTerminal))
    {
        SimcpLinkTeardown();
        return replayTerminal ? SimcpEstablishTerminal : SimcpEstablishRetry;
    }

    SimcpLog("connected to System Informer");

    *PipeHandle = pipeHandle;
    SimcpLinkConnected();

    if (SimcpLinkGeneration() > 1)
        SimcpEmitListChanged();

    return SimcpEstablishConnected;
}

BOOLEAN SimcpIdIsPlainInteger(
    _In_ PPH_BYTES IdJson,
    _Out_ PULONG64 Value
    )
{
    ULONG64 value = 0;
    SIZE_T i;

    *Value = 0;

    if (IdJson->Length == 0 || IdJson->Length > 20)
        return FALSE;

    for (i = 0; i < IdJson->Length; i++)
    {
        CHAR c = IdJson->Buffer[i];

        if (c < '0' || c > '9')
            return FALSE;

        value = value * 10 + (ULONG64)(c - '0');
    }

    *Value = value;
    return TRUE;
}

BOOLEAN SimcpParseTaggedId(
    _In_ PPH_BYTES IdJson,
    _Out_ PULONG Generation,
    _Out_ PULONG64 Original
    )
{
    static CONST CHAR prefix[] = "\"si-";
    ULONG64 generation = 0;
    ULONG64 original = 0;
    SIZE_T i;
    SIZE_T digits;

    *Generation = 0;
    *Original = 0;

    if (IdJson->Length < sizeof(prefix) ||
        memcmp(IdJson->Buffer, prefix, sizeof(prefix) - 1) != 0 ||
        IdJson->Buffer[IdJson->Length - 1] != '"')
    {
        return FALSE;
    }

    i = sizeof(prefix) - 1;

    for (digits = 0; i < IdJson->Length - 1 && IdJson->Buffer[i] != '-'; i++, digits++)
    {
        CHAR c = IdJson->Buffer[i];

        if (c < '0' || c > '9' || digits > 10)
            return FALSE;

        generation = generation * 10 + (ULONG64)(c - '0');
    }

    if (!digits || i >= IdJson->Length - 1 || IdJson->Buffer[i] != '-')
        return FALSE;

    i++;

    for (digits = 0; i < IdJson->Length - 1; i++, digits++)
    {
        CHAR c = IdJson->Buffer[i];

        // "si-<gen>-init" is the handshake replay's own id; it is ours but has no number to
        // restore, so it is dropped rather than forwarded.
        if (c < '0' || c > '9' || digits > 19)
            return FALSE;

        original = original * 10 + (ULONG64)(c - '0');
    }

    if (!digits)
        return FALSE;

    *Generation = (ULONG)generation;
    *Original = original;
    return TRUE;
}

VOID SimcpWriteTaggedLine(
    _In_ PSIMCP_ENVELOPE Envelope,
    _In_ ULONG Generation,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    ULONG64 original;
    PH_FORMAT format[4];
    PPH_STRING idString;
    PPH_BYTES idBytes;
    PPH_BYTES tagged;

    // Only a numeric id can be restored exactly, so only a numeric id is tagged.
    if (Envelope->Kind != SimcpEnvelopeRequest || !Envelope->Id ||
        !SimcpIdIsPlainInteger(Envelope->Id, &original))
    {
        SimcpWriteLine(Buffer, Length);
        return;
    }

    PhInitFormatS(&format[0], L"si-");
    PhInitFormatU(&format[1], Generation);
    PhInitFormatS(&format[2], L"-");
    PhInitFormatI64U(&format[3], original);
    idString = PhFormat(format, RTL_NUMBER_OF(format), 24);
    idBytes = PhConvertUtf16ToUtf8Ex(idString->Buffer, idString->Length);
    PhDereferenceObject(idString);

    tagged = SimcpRewriteEnvelopeId(Buffer, Length, idBytes->Buffer);
    PhDereferenceObject(idBytes);

    if (!tagged)
    {
        SimcpWriteLine(Buffer, Length);
        return;
    }

    SimcpWriteLine(tagged->Buffer, (ULONG)tagged->Length);
    PhDereferenceObject(tagged);
}

BOOLEAN SimcpPump(
    _In_ HANDLE PipeHandle,
    _In_ ULONG Generation,
    _Out_ PCSTR *Message
    )
{
    while (TRUE)
    {
        NTSTATUS status;
        SIMCP_HEADER header;
        PVOID payload;
        BOOLEAN terminal;

        status = SimcpReadEnvelope(PipeHandle, SimcpLink.ReadEvent, &header, &payload);

        if (!NT_SUCCESS(status))
        {
            // A broken pipe is System Informer going away; a malformed frame is not.
            if (status == STATUS_INVALID_NETWORK_RESPONSE)
            {
                *Message = "malformed message from System Informer";
                return TRUE;
            }

            *Message = "System Informer closed the connection";
            return FALSE;
        }

        if (header.Type == SimcpMcp)
        {
            if (payload)
            {
                SIMCP_ENVELOPE envelope;

                SimcpParseEnvelope(payload, header.PayloadLength, &envelope);

                if (envelope.Kind == SimcpEnvelopeUnparsed)
                    SimcpLog("could not read the envelope of a line from System Informer");
                else if (envelope.Kind == SimcpEnvelopeResponse)
                {
                    SimcpRemovePending(&SimcpPending, envelope.Id);
                    SimcpSessionObserveIncoming(&envelope);
                }

                SimcpWriteTaggedLine(&envelope, Generation, payload, header.PayloadLength);

                SimcpDeleteEnvelope(&envelope);
                PhFree(payload);
            }

            continue;
        }

        if (header.Type == SimcpClose)
        {
            SIMCP_CLOSE close;

            memset(&close, 0, sizeof(SIMCP_CLOSE));

            if (payload && header.PayloadLength >= sizeof(SIMCP_CLOSE))
                memcpy(&close, payload, sizeof(SIMCP_CLOSE));

            *Message = SimcpCloseReasonToString(&close);
            terminal = SimcpCloseReasonIsTerminal(&close);
        }
        else
        {
            *Message = "unexpected message from System Informer";
            terminal = TRUE;
        }

        if (payload)
            PhFree(payload);

        return terminal;
    }
}

ULONG SimcpNextBackoff(
    _In_ ULONG Current
    )
{
    ULONG next;

    if (Current == 0)
        return SIMCP_RECONNECT_BACKOFF_FIRST_MS;

    if (!NT_SUCCESS(RtlULongMult(Current, 2, &next)) || next > SIMCP_RECONNECT_BACKOFF_MAX_MS)
        return SIMCP_RECONNECT_BACKOFF_MAX_MS;

    return next;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS NTAPI SimcpSupervisorThread(
    _In_ PVOID Parameter
    )
{
    ULONG backoff = 0;
    PCSTR message = "the connection ended";
    PCSTR reported = NULL;

    while (TRUE)
    {
        HANDLE pipeHandle;
        SIMCP_ESTABLISH_RESULT result;

        result = SimcpEstablish(&pipeHandle, &message);

        if (result == SimcpEstablishTerminal)
            break;

        if (result == SimcpEstablishRetry)
        {
            if (SimcpNoReconnect)
                break;

            // Say why once per distinct reason, so a broker that is waiting -- for System
            // Informer to be started, or because something else is holding the pipe name -- is
            // not silent about it. The messages are literals, so comparing pointers is enough
            // to spot a new one.
            if (message != reported)
            {
                SimcpLog(message);
                reported = message;
            }

            backoff = SimcpNextBackoff(backoff);
            PhDelayExecution(backoff);
            continue;
        }

        backoff = 0;
        reported = NULL;

        if (SimcpPump(pipeHandle, SimcpLinkGeneration(), &message))
        {
            SimcpLinkTeardown();
            break;
        }

        SimcpLinkTeardown();

        if (SimcpNoReconnect)
            break;

        SimcpLog("System Informer went away; waiting for it to come back");

        backoff = SimcpNextBackoff(backoff);
        PhDelayExecution(backoff);
    }

    SimcpLinkAbort();
    SimcpFail(message);
}

VOID SimcpRelayLine(
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    SIMCP_ENVELOPE envelope;

    SimcpParseEnvelope(Buffer, Length, &envelope);

    if (envelope.Kind == SimcpEnvelopeUnparsed)
        SimcpLog("could not read the envelope of a line from the host");

    SimcpSessionObserveOutgoing(&envelope, Buffer, Length);

    // A liveness probe is about the broker, and holding it for the grace window would answer the
    // wrong question.
    if (envelope.Kind == SimcpEnvelopeRequest &&
        PhEqualString2(envelope.Method, L"ping", FALSE) &&
        !SimcpLinkWaitConnected(0))
    {
        SimcpEmitPong(envelope.Id);
        SimcpDeleteEnvelope(&envelope);
        return;
    }

    // A reply to a request System Informer made comes back with the tag the broker put on it.
    if (envelope.Kind == SimcpEnvelopeResponse && envelope.Id)
    {
        ULONG generation;
        ULONG64 original;

        if (SimcpParseTaggedId(envelope.Id, &generation, &original))
        {
            PPH_BYTES restored;

            if (generation != SimcpLinkGeneration())
            {
                // The request this answers died with its generation; delivering it now would
                // hand a stale answer to whatever System Informer is asking today.
                SimcpLog("dropped an answer to a request from an earlier connection");
                SimcpDeleteEnvelope(&envelope);
                return;
            }

            restored = SimcpRewriteEnvelopeIdInteger(Buffer, Length, (LONG64)original);

            if (!restored)
            {
                SimcpDeleteEnvelope(&envelope);
                return;
            }

            if (!NT_SUCCESS(SimcpLinkSend(SimcpMcp, restored->Buffer, (ULONG)restored->Length, TRUE, NULL)))
            {
                if (SimcpLinkAborted())
                {
                    PhDereferenceObject(restored);
                    SimcpDeleteEnvelope(&envelope);
                    SimcpFail("System Informer closed the connection");
                }
            }

            PhDereferenceObject(restored);
            SimcpDeleteEnvelope(&envelope);
            return;
        }
    }

    // Relayed byte for byte otherwise. A cancellation is relayed like any other line: System
    // Informer needs it to stop the call.
    if (!NT_SUCCESS(SimcpLinkSend(SimcpMcp, Buffer, Length, TRUE, NULL)))
    {
        if (SimcpLinkAborted())
        {
            SimcpDeleteEnvelope(&envelope);
            SimcpFail("System Informer closed the connection");
        }

        if (envelope.Kind == SimcpEnvelopeRequest)
            SimcpEmitTransportError(envelope.Id, "System Informer is not available");

        // A notification is owed nothing, so it is dropped -- after the session state above.
        SimcpDeleteEnvelope(&envelope);
        return;
    }

    // Tracked only once the line is on the wire, so an unsent request is never owed a response by
    // the pipe thread.
    if (envelope.Kind == SimcpEnvelopeRequest)
    {
        if (!SimcpAddPending(&SimcpPending, envelope.Id))
            SimcpLog("could not track another outstanding request");
    }
    else if (envelope.CancelId)
    {
        SimcpRemovePending(&SimcpPending, envelope.CancelId);
    }

    SimcpDeleteEnvelope(&envelope);
}

VOID SimcpRelayStandardInput(
    VOID
    )
{
    PVOID buffer;
    ULONG bufferLength;
    ULONG dataLength = 0;

    bufferLength = PAGE_SIZE * 16;
    buffer = PhAllocate(bufferLength);

    while (TRUE)
    {
        ULONG bytesRead = 0;
        ULONG lineStart;
        ULONG i;

        if (dataLength == bufferLength)
        {
            if (bufferLength >= SIMCP_MAX_PAYLOAD_LENGTH)
                SimcpFail("request line exceeds the maximum message size");

            bufferLength *= 2;
            buffer = PhReAllocate(buffer, bufferLength);
        }

        if (!ReadFile(SimcpStdInput, PTR_ADD_OFFSET(buffer, dataLength), bufferLength - dataLength, &bytesRead, NULL))
            break;
        if (bytesRead == 0)
            break;

        lineStart = 0;

        for (i = dataLength; i < dataLength + bytesRead; i++)
        {
            ULONG lineLength;

            if (((PCHAR)buffer)[i] != '\n')
                continue;

            lineLength = i - lineStart;

            if (lineLength && ((PCHAR)buffer)[i - 1] == '\r')
                lineLength--;

            if (lineLength)
            {
                if (lineLength > SIMCP_MAX_PAYLOAD_LENGTH)
                    SimcpFail("request line exceeds the maximum message size");

                SimcpRelayLine(PTR_ADD_OFFSET(buffer, lineStart), lineLength);
            }

            lineStart = i + 1;
        }

        dataLength += bytesRead;

        if (lineStart)
        {
            memmove(buffer, PTR_ADD_OFFSET(buffer, lineStart), dataLength - lineStart);
            dataLength -= lineStart;
        }
    }

    PhFree(buffer);
}

VOID SimcpApplyMitigations(
    VOID
    )
{
    PROCESS_MITIGATION_POLICY_INFORMATION policyInfo;

    memset(&policyInfo, 0, sizeof(PROCESS_MITIGATION_POLICY_INFORMATION));
    policyInfo.Policy = ProcessExtensionPointDisablePolicy;
    policyInfo.ExtensionPointDisablePolicy.DisableExtensionPoints = TRUE;
    NtSetInformationProcess(NtCurrentProcess(), ProcessMitigationPolicy, &policyInfo, sizeof(PROCESS_MITIGATION_POLICY_INFORMATION));

    memset(&policyInfo, 0, sizeof(PROCESS_MITIGATION_POLICY_INFORMATION));
    policyInfo.Policy = ProcessImageLoadPolicy;
    policyInfo.ImageLoadPolicy.NoRemoteImages = TRUE;
    policyInfo.ImageLoadPolicy.NoLowMandatoryLabelImages = TRUE;
    policyInfo.ImageLoadPolicy.PreferSystem32Images = TRUE;
    NtSetInformationProcess(NtCurrentProcess(), ProcessMitigationPolicy, &policyInfo, sizeof(PROCESS_MITIGATION_POLICY_INFORMATION));

    memset(&policyInfo, 0, sizeof(PROCESS_MITIGATION_POLICY_INFORMATION));
    policyInfo.Policy = ProcessChildProcessPolicy;
    policyInfo.ChildProcessPolicy.NoChildProcessCreation = TRUE;
    NtSetInformationProcess(NtCurrentProcess(), ProcessMitigationPolicy, &policyInfo, sizeof(PROCESS_MITIGATION_POLICY_INFORMATION));

    memset(&policyInfo, 0, sizeof(PROCESS_MITIGATION_POLICY_INFORMATION));
    policyInfo.Policy = ProcessDynamicCodePolicy;
    policyInfo.DynamicCodePolicy.ProhibitDynamicCode = TRUE;
    NtSetInformationProcess(NtCurrentProcess(), ProcessMitigationPolicy, &policyInfo, sizeof(PROCESS_MITIGATION_POLICY_INFORMATION));

    memset(&policyInfo, 0, sizeof(PROCESS_MITIGATION_POLICY_INFORMATION));
    policyInfo.Policy = ProcessSignaturePolicy;
    policyInfo.SignaturePolicy.MicrosoftSignedOnly = TRUE;
    NtSetInformationProcess(NtCurrentProcess(), ProcessMitigationPolicy, &policyInfo, sizeof(PROCESS_MITIGATION_POLICY_INFORMATION));
}

_Function_class_(PH_COMMAND_LINE_CALLBACK)
BOOLEAN NTAPI SimcpCommandLineCallback(
    _In_opt_ PCPH_COMMAND_LINE_OPTION Option,
    _In_opt_ PPH_STRING Value,
    _In_opt_ PVOID Context
    )
{
    if (Option && Option->Id == SIMCP_OPTION_NO_RECONNECT)
    {
        SimcpNoReconnect = TRUE;
        SimcpLog("reconnect disabled by --no-reconnect");
    }
    return TRUE;
}

VOID SimcpParseArguments(
    VOID
    )
{
    // PhParseCommandLine matches a single leading dash, so the second row is what makes the
    // double-dash spelling an MCP host config would normally use work too.
    static CONST PH_COMMAND_LINE_OPTION options[] =
    {
        { SIMCP_OPTION_NO_RECONNECT, L"no-reconnect", NoArgumentType },
        { SIMCP_OPTION_NO_RECONNECT, L"-no-reconnect", NoArgumentType },
    };
    PH_STRINGREF commandLine;

    if (!NT_SUCCESS(PhGetProcessCommandLineStringRef(&commandLine)))
        return;

    PhParseCommandLine(
        &commandLine,
        options,
        RTL_NUMBER_OF(options),
        PH_COMMAND_LINE_IGNORE_UNKNOWN_OPTIONS | PH_COMMAND_LINE_IGNORE_FIRST_PART,
        SimcpCommandLineCallback,
        NULL
        );
}

int __cdecl wmain(int argc, wchar_t *argv[])
{
    NTSTATUS status;

    status = PhInitializePhLib(L"simcp");

    if (!NT_SUCCESS(status))
        return 1;

    SimcpApplyMitigations();
    SimcpInitializePending(&SimcpPending);
    PhInitializeQueuedLock(&SimcpSession.Lock);

    SimcpStdInput = PhGetStdHandle(STD_INPUT_HANDLE);
    SimcpStdOutput = PhGetStdHandle(STD_OUTPUT_HANDLE);
    SimcpStdError = PhGetStdHandle(STD_ERROR_HANDLE);

    if (!SimcpStdInput || !SimcpStdOutput)
        return 1;

    // After the standard handles, so anything it reports can actually be seen.
    SimcpParseArguments();

    if (!NT_SUCCESS(SimcpLinkInitialize()))
        SimcpFail("unable to allocate I/O resources");

    // The supervisor owns the pipe and standard output; this thread owns standard input and
    // writes through SimcpLinkSend, which serialises against the supervisor's own frames.
    status = PhCreateThread2(SimcpSupervisorThread, NULL);

    if (!NT_SUCCESS(status))
        SimcpFail("unable to start the relay thread");

    SimcpRelayStandardInput();

    // The host closed our input: the session is over. Give replies already in flight a moment to
    // reach the host, then exit; closing the pipe is what the server observes as a disconnect.
    PhDelayExecution(500);
    RtlExitUserProcess(STATUS_SUCCESS);
}
