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

static HANDLE SimcpStdInput = NULL;
static HANDLE SimcpStdOutput = NULL;
static HANDLE SimcpStdError = NULL;
static HANDLE SimcpPipeHandle = NULL;
static HANDLE SimcpPipeReadEvent = NULL;
static HANDLE SimcpPipeWriteEvent = NULL;
static PH_QUEUED_LOCK SimcpStdOutputLock = PH_QUEUED_LOCK_INIT;
static SIMCP_PENDING SimcpPending = { 0 };

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

NTSTATUS SimcpWriteEnvelope(
    _In_ HANDLE PipeHandle,
    _In_ HANDLE EventHandle,
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

    status = SimcpWriteAllPipe(PipeHandle, EventHandle, &header, sizeof(SIMCP_HEADER));

    if (NT_SUCCESS(status) && Payload && PayloadLength)
    {
        status = SimcpWriteAllPipe(PipeHandle, EventHandle, Payload, PayloadLength);
    }

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

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS NTAPI SimcpPipeReaderThread(
    _In_ PVOID Parameter
    )
{
    while (TRUE)
    {
        NTSTATUS status;
        SIMCP_HEADER header;
        PVOID payload;

        status = SimcpReadEnvelope(SimcpPipeHandle, SimcpPipeReadEvent, &header, &payload);

        if (!NT_SUCCESS(status))
        {
            if (status == STATUS_INVALID_NETWORK_RESPONSE)
                SimcpFail("malformed message from System Informer");
            else
                SimcpFail("System Informer closed the connection");
        }

        switch (header.Type)
        {
        case SimcpMcp:
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
                }
            }
            break;
        case SimcpClose:
            {
                SIMCP_CLOSE close;

                memset(&close, 0, sizeof(SIMCP_CLOSE));

                if (payload && header.PayloadLength >= sizeof(SIMCP_CLOSE))
                    memcpy(&close, payload, sizeof(SIMCP_CLOSE));

                SimcpFail(SimcpCloseReasonToString(&close));
            }
            break;
        default:
            SimcpFail("unexpected message from System Informer");
        }

        if (payload)
            PhFree(payload);
    }
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
                if (!NT_SUCCESS(SimcpWriteEnvelope(
                    SimcpPipeHandle,
                    SimcpPipeWriteEvent,
                    SimcpMcp,
                    PTR_ADD_OFFSET(buffer, lineStart),
                    lineLength
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
    SIMCP_HELLO hello;
    SIMCP_HEADER header;
    PVOID payload;
    SIMCP_HELLO_ACK helloAck;
    PCSTR failureMessage;

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

    if (!NT_SUCCESS(SimcpCreateIoEvent(&SimcpPipeReadEvent)) ||
        !NT_SUCCESS(SimcpCreateIoEvent(&SimcpPipeWriteEvent)))
    {
        SimcpFail("unable to allocate I/O resources");
    }

    status = SimcpConnectPipe(&SimcpPipeHandle, &failureMessage);

    if (!NT_SUCCESS(status))
    {
        if (failureMessage)
            SimcpFail(failureMessage);
        else if (status == STATUS_OBJECT_NAME_NOT_FOUND || status == STATUS_OBJECT_PATH_NOT_FOUND)
            SimcpFail("System Informer is not running in this session or the agent tools server is not enabled");
        else if (status == STATUS_ACCESS_DENIED)
            SimcpFail("access to the System Informer agent pipe was denied");
        else
            SimcpFail("unable to connect to the System Informer agent pipe");
    }

    SimcpFillHello(&hello);

    if (!NT_SUCCESS(SimcpWriteEnvelope(SimcpPipeHandle, SimcpPipeWriteEvent, SimcpHello, &hello, sizeof(SIMCP_HELLO))))
        SimcpFail("System Informer closed the connection during the handshake");

    if (!NT_SUCCESS(SimcpReadEnvelope(SimcpPipeHandle, SimcpPipeReadEvent, &header, &payload)))
        SimcpFail("System Informer closed the connection during the handshake");

    if (header.Type == SimcpClose)
    {
        SIMCP_CLOSE close;

        memset(&close, 0, sizeof(SIMCP_CLOSE));

        if (payload && header.PayloadLength >= sizeof(SIMCP_CLOSE))
            memcpy(&close, payload, sizeof(SIMCP_CLOSE));

        SimcpFail(SimcpCloseReasonToString(&close));
    }

    if (header.Type != SimcpHelloAck || !payload || header.PayloadLength < sizeof(SIMCP_HELLO_ACK))
        SimcpFail("unexpected handshake reply from System Informer");

    memcpy(&helloAck, payload, sizeof(SIMCP_HELLO_ACK));
    PhFree(payload);

    if (helloAck.Status != SimcpHelloAccepted)
        SimcpFail(SimcpHelloStatusToString(helloAck.Status));

    SimcpLog("connected to System Informer");

    // One thread per direction. Neither thread interprets payloads; the pipe reader owns
    // standard output (bar the error line) and this thread owns the pipe write side.
    status = PhCreateThread2(SimcpPipeReaderThread, NULL);

    if (!NT_SUCCESS(status))
        SimcpFail("unable to start the relay thread");

    SimcpRelayStandardInput();

    // The host closed our input: the session is over. Give replies already in flight a moment to
    // reach the host, then exit; closing the pipe is what the server observes as a disconnect.
    // The broker never reconnects.
    PhDelayExecution(500);
    RtlExitUserProcess(STATUS_SUCCESS);
}
