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

static HANDLE SimcpStdInput = NULL;
static HANDLE SimcpStdOutput = NULL;
static HANDLE SimcpStdError = NULL;
static PH_QUEUED_LOCK SimcpStdOutputLock = PH_QUEUED_LOCK_INIT;
static SIMCP_PENDING SimcpPending = { 0 };

// One generation of the pipe. The supervisor thread is the only reader and the only thing that
// replaces Handle, so a read can never race the close. Writers are serialised by SendLock, which
// teardown takes before clearing Handle, so a write can never be left holding a closed handle.
typedef struct _SIMCP_LINK
{
    PH_QUEUED_LOCK Lock;        // guards Handle and Generation
    HANDLE Handle;              // NULL when the link is down
    ULONG Generation;
    PH_QUEUED_LOCK SendLock;    // one whole envelope at a time; sole owner of WriteEvent
    HANDLE WriteEvent;
    HANDLE ReadEvent;           // supervisor only
    HANDLE ConnectedEvent;
    HANDLE AbortEvent;
} SIMCP_LINK, *PSIMCP_LINK;

static SIMCP_LINK SimcpLink = { 0 };

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

// Terminal reasons only; anything unrecognised is reconnect-eligible so a newer server can add reasons.
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

    return status;
}

// A connect attempt becomes a generation here; the number is never reused, so a frame from an
// attempt that died mid-handshake can never be mistaken for a live one.
VOID SimcpLinkPublish(
    _In_ HANDLE PipeHandle
    )
{
    PhAcquireQueuedLockExclusive(&SimcpLink.Lock);
    SimcpLink.Handle = PipeHandle;
    SimcpLink.Generation++;
    PhReleaseQueuedLockExclusive(&SimcpLink.Lock);
}

VOID SimcpLinkConnected(
    VOID
    )
{
    NtSetEvent(SimcpLink.ConnectedEvent, NULL);
}

VOID SimcpLinkAbort(
    VOID
    )
{
    NtSetEvent(SimcpLink.AbortEvent, NULL);
}

/**
 * Closes the current generation.
 *
 * Cancels a write parked in the pipe before taking SendLock, so teardown cannot wait on a peer
 * that has stopped reading. Handle is cleared under the lock and closed only once no writer can
 * still reach it.
 */
VOID SimcpLinkTeardown(
    VOID
    )
{
    HANDLE handle;
    IO_STATUS_BLOCK isb;

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

    // A generation's outstanding requests die with it.
    SimcpClearPending(&SimcpPending);
}

/**
 * Waits until the link can carry a frame.
 *
 * 
eturn TRUE when connected, FALSE when the broker is shutting down.
 */
BOOLEAN SimcpLinkWaitConnected(
    VOID
    )
{
    HANDLE handles[2];
    NTSTATUS status;

    handles[0] = SimcpLink.ConnectedEvent;
    handles[1] = SimcpLink.AbortEvent;

    status = NtWaitForMultipleObjects(2, handles, WaitAny, FALSE, NULL);

    return status == STATUS_WAIT_0;
}

/**
 * Writes one envelope.
 *
 * Header and payload go out under a single lock, so a short write can never be split across two
 * generations and leave System Informer reading a header-less tail.
 *
 * \param Type The message type.
 * \param Payload The payload, or NULL.
 * \param PayloadLength The payload length in bytes.
 * \param WaitForConnected TRUE to block until a generation is usable; the handshake passes FALSE
 * because it is what makes the generation usable.
 * \param Generation Receives the generation the frame was written to.
 */
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

    if (WaitForConnected && !SimcpLinkWaitConnected())
        return STATUS_PIPE_DISCONNECTED;

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

typedef enum _SIMCP_ESTABLISH_RESULT
{
    SimcpEstablishConnected,
    SimcpEstablishRetry,        // transient: back off and try again
    SimcpEstablishTerminal,     // the session cannot continue
} SIMCP_ESTABLISH_RESULT;

/**
 * Connects, handshakes and publishes a generation.
 *
 * \param PipeHandle Receives the handle of the new generation.
 * \param Message Receives why the attempt failed, when it did.
 */
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

    SimcpLog("connected to System Informer");

    *PipeHandle = pipeHandle;
    SimcpLinkConnected();

    return SimcpEstablishConnected;
}

/**
 * Relays until the generation ends.
 *
 * \param PipeHandle The generation's pipe.
 * \param Message Receives why the generation ended.
 * \return TRUE when the session cannot continue, FALSE when it may reconnect.
 */
BOOLEAN SimcpPump(
    _In_ HANDLE PipeHandle,
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
                    SimcpRemovePending(&SimcpPending, envelope.Id);

                SimcpDeleteEnvelope(&envelope);

                // Relayed byte for byte; nothing here rewrites a line yet.
                SimcpWriteLine(payload, header.PayloadLength);

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

/**
 * Owns the pipe: connect, handshake, pump, tear down, and go round again.
 *
 * Only a decision ends the session: the user disconnecting, a rejected handshake, a broken
 * protocol. An outage does not.
 */
_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS NTAPI SimcpSupervisorThread(
    _In_ PVOID Parameter
    )
{
    ULONG backoff = 0;
    BOOLEAN everConnected = FALSE;
    PCSTR message = "the connection ended";

    while (TRUE)
    {
        HANDLE pipeHandle;
        SIMCP_ESTABLISH_RESULT result;

        result = SimcpEstablish(&pipeHandle, &message);

        if (result == SimcpEstablishTerminal)
            break;

        if (result == SimcpEstablishRetry)
        {
            // The first connect still reports why it failed rather than hanging: a broker that
            // waits forever for a System Informer that was never started is worse than one that
            // says so. Reconnecting is for a link that once worked.
            if (!everConnected)
                break;

            backoff = SimcpNextBackoff(backoff);
            PhDelayExecution(backoff);
            continue;
        }

        everConnected = TRUE;
        backoff = 0;

        if (SimcpPump(pipeHandle, &message))
        {
            SimcpLinkTeardown();
            break;
        }

        SimcpLinkTeardown();
        SimcpLog("System Informer went away; waiting for it to come back");

        backoff = SimcpNextBackoff(backoff);
        PhDelayExecution(backoff);
    }

    SimcpLinkAbort();
    SimcpFail(message);
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
                SIMCP_ENVELOPE envelope;

                if (lineLength > SIMCP_MAX_PAYLOAD_LENGTH)
                    SimcpFail("request line exceeds the maximum message size");

                SimcpParseEnvelope(PTR_ADD_OFFSET(buffer, lineStart), lineLength, &envelope);

                if (envelope.Kind == SimcpEnvelopeUnparsed)
                    SimcpLog("could not read the envelope of a line from the host");

                // Relayed byte for byte; nothing here rewrites a line yet. A cancellation is
                // relayed like any other line: System Informer needs it to stop the call.
                if (!NT_SUCCESS(SimcpLinkSend(
                    SimcpMcp,
                    PTR_ADD_OFFSET(buffer, lineStart),
                    lineLength,
                    TRUE,
                    NULL
                    )))
                {
                    SimcpDeleteEnvelope(&envelope);
                    SimcpFail("System Informer closed the connection");
                }

                // Tracked only once the line is on the wire, so an unsent request is never owed
                // a response by the pipe thread.
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

int __cdecl wmain(int argc, wchar_t *argv[])
{
    NTSTATUS status;

    status = PhInitializePhLib(L"simcp");

    if (!NT_SUCCESS(status))
        return 1;

    SimcpApplyMitigations();
    SimcpInitializePending(&SimcpPending);

    SimcpStdInput = PhGetStdHandle(STD_INPUT_HANDLE);
    SimcpStdOutput = PhGetStdHandle(STD_OUTPUT_HANDLE);
    SimcpStdError = PhGetStdHandle(STD_ERROR_HANDLE);

    if (!SimcpStdInput || !SimcpStdOutput)
        return 1;

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
