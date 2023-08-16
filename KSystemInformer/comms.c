/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022
 *
 */

#include <kph.h>
#include <comms.h>
#include <informer.h>
#include <dyndata.h>
#include <kphmsgdyn.h>

#include <trace.h>

#define KPH_COMMS_MIN_TIMEOUT_MS 300
#define KPH_COMMS_MAX_TIMEOUT_MS (60 * 1000)
#define KPH_COMMS_MIN_QUEUE_THREADS 2
#define KPH_COMMS_MAX_QUEUE_THREADS 64

static KPH_RUNDOWN KphpCommsRundown;
static PFLT_PORT KphpFltServerPort = NULL;
static PAGED_LOOKASIDE_LIST KphpMessageLookaside;
static NPAGED_LOOKASIDE_LIST KphpNPagedMessageLookaside;
static KPH_RWLOCK KphpConnectedClientLock;
static LIST_ENTRY KphpConnectedClientList;
static NPAGED_LOOKASIDE_LIST KphpMessageQueueItemLookaside;
static KQUEUE KphpMessageQueue;
static PKTHREAD* KphpMessageQueueThreads = NULL;
static ULONG KphpMessageQueueThreadsCount = 0;
static UNICODE_STRING KphpClientObjectName = RTL_CONSTANT_STRING(L"KphClient");
static PKPH_OBJECT_TYPE KphpClientObjectType = NULL;

typedef struct _KPHM_QUEUE_ITEM
{
    LIST_ENTRY Entry;
    BOOLEAN NonPaged;
    PEPROCESS TargetClientProcess;
    PKPH_MESSAGE Message;
} KPHM_QUEUE_ITEM, *PKPHM_QUEUE_ITEM;

typedef struct _KPH_CLIENT
{
    LIST_ENTRY Entry;
    PKPH_PROCESS_CONTEXT Process;
    PFLT_PORT Port;
} KPH_CLIENT, *PKPH_CLIENT;

typedef const KPH_CLIENT* PCKPH_CLIENT;

/**
 * \brief Allocates a message queue item.
 *
 * \return Message queue item, null on allocation failure.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Return_allocatesMem_
PKPHM_QUEUE_ITEM KphpAllocateMessageQueueItem()
{
    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    return KphAllocateFromNPagedLookaside(&KphpMessageQueueItemLookaside);
}

/**
 * \brief Frees a message queue item.
 *
 * \param[in] Item Message queue item to free.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphpFreeMessageQueueItem(_In_freesMem_ PKPHM_QUEUE_ITEM Item)
{
    NT_ASSERT(Item);
    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    if (Item->NonPaged)
    {
        KphFreeNPagedMessage(Item->Message);
    }
    else
    {
        KphFreeMessage(Item->Message);
    }

    KphFreeToNPagedLookaside(&KphpMessageQueueItemLookaside, Item);
}

/**
 * \brief Allocates a non-paged message.
 *
 * \return Non-paged message, null on allocation failure.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Return_allocatesMem_
PKPH_MESSAGE KphAllocateNPagedMessage(
    VOID
    )
{
    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    return KphAllocateFromNPagedLookaside(&KphpNPagedMessageLookaside);
}

/**
 * \brief Frees a non-paged message.
 *
 * \param[in] Message Message to free.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphFreeNPagedMessage(
    _In_freesMem_ PKPH_MESSAGE Message
    )
{
    NT_ASSERT(Message);
    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    KphFreeToNPagedLookaside(&KphpNPagedMessageLookaside, Message);
}

/**
 * \brief Sends a non-paged message asynchronously.
 *
 * \param[in] Message Message to send asynchronously. The call assumes ownership
 * over the message. The caller should *not* free the message after it is
 * passed to this function.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphCommsSendNPagedMessageAsync(
    _In_aliasesMem_ PKPH_MESSAGE Message
    )
{
    PKPHM_QUEUE_ITEM item;

    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    if (!KphAcquireRundown(&KphpCommsRundown))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      COMMS,
                      "Failed to acquire rundown, dropping message (%lu - %!TIME!)",
                      (ULONG)Message->Header.MessageId,
                      Message->Header.TimeStamp.QuadPart);

        KphFreeNPagedMessage(Message);
        return;
    }

    item = KphpAllocateMessageQueueItem();
    if (!item)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      COMMS,
                      "Failed to allocate queue item, dropping message (%lu - %!TIME!)",
                      (ULONG)Message->Header.MessageId,
                      Message->Header.TimeStamp.QuadPart);

        KphFreeNPagedMessage(Message);
        KphReleaseRundown(&KphpCommsRundown);
        return;
    }

    item->Message = Message;
    item->NonPaged = TRUE;
    item->TargetClientProcess = NULL;

    KeInsertHeadQueue(&KphpMessageQueue, &item->Entry);

    KphReleaseRundown(&KphpCommsRundown);
}


PAGED_FILE();

/**
 * \brief Allocates a client object.
 *
 * \param[in] Size The size requested from the object infrastructure.
 *
 * \return Allocated client object, null on allocation failure.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Return_allocatesMem_size_(Size)
PVOID KSIAPI KphpAllocateClientObject(
    _In_ SIZE_T Size
    )
{
    PAGED_PASSIVE();

    return KphAllocatePaged(Size, KPH_TAG_CLIENT);
}

/**
 * \brief Initializes a client object.
 *
 * \param[in] Object The client object to initialize.
 * \param[in] Parameter The process context associated with the client.
 *
 * \return STATUS_SUCCESS
 */
_Function_class_(KPH_TYPE_INITIALIZE_PROCEDURE)
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpInitializeClientObject(
    _Inout_ PVOID Object,
    _In_opt_ PVOID Parameter
    )
{
    PKPH_CLIENT client;

    PAGED_CODE();

    client = Object;

    client->Process = Parameter;
    KphReferenceObject(client->Process);

    return STATUS_SUCCESS;
}

/**
 * \brief Deletes a client object.
 *
 * \param[in] Object The client object to delete.
 */
_Function_class_(KPH_TYPE_DELETE_PROCEDURE)
_IRQL_requires_max_(APC_LEVEL)
VOID KSIAPI KphpDeleteClientObject(
    _Inout_ PVOID Object
    )
{
    PKPH_CLIENT client;

    PAGED_CODE();

    client = Object;

    KphDereferenceObject(client->Process);

    if (client->Port)
    {
        FltCloseClientPort(KphFltFilter, &client->Port);
    }
}

/**
 * \brief Frees a client object.
 *
 * \param[in] Object The client object to free.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_IRQL_requires_max_(APC_LEVEL)
VOID KSIAPI KphpFreeClientObject(
    _In_freesMem_ PVOID Object
    )
{
    PAGED_CODE();

    KphFree(Object, KPH_TAG_CLIENT);
}

/**
 * \brief Communication port connect notify callback.
 *
 * \param[in] ClientPort Client port for this client.
 * \param[in] ServerPortCookie Unused
 * \param[in] ConnectionContext Context from the connecting client.
 * \param[in] SizeOfContext Size of the connection context from the client.
 * \param[out] ConnectionPortCookie Returns the client object on success.
 *
 * \return Successful status or an errant one.
 */
_Function_class_(PFLT_CONNECT_NOTIFY)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS FLTAPI KphpCommsConnectNotifyCallback(
    _In_ PFLT_PORT ClientPort,
    _In_ PVOID ServerPortCookie,
    _In_ PVOID ConnectionContext,
    _In_ ULONG SizeOfContext,
    _Out_ PVOID* ConnectionPortCookie
    )
{
    NTSTATUS status;
    PKPH_PROCESS_CONTEXT process;
    KPH_PROCESS_STATE processState;
    PKPH_CLIENT client;

    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(ServerPortCookie);
    UNREFERENCED_PARAMETER(ConnectionContext);
    UNREFERENCED_PARAMETER(SizeOfContext);

    *ConnectionPortCookie = NULL;

    process = NULL;
    client = NULL;

    if (!KphSinglePrivilegeCheck(SeExports->SeDebugPrivilege, UserMode))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      COMMS,
                      "KphSinglePrivilegeCheck failed %lu",
                      HandleToULong(PsGetCurrentProcessId()));

        status = STATUS_PRIVILEGE_NOT_HELD;
        goto Exit;
    }

    process = KphGetCurrentProcessContext();
    if (!process)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      COMMS,
                      "Untracked process %lu",
                      HandleToULong(PsGetCurrentProcessId()));

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    processState = KphGetProcessState(process);
    if ((processState & KPH_PROCESS_STATE_LOW) != KPH_PROCESS_STATE_LOW)
    {
        KphTracePrint(TRACE_LEVEL_CRITICAL,
                      COMMS,
                      "Untrusted client %lu (0x%08x)",
                      HandleToULong(process->ProcessId),
                      processState);

        status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    status = KphCreateObject(KphpClientObjectType,
                             sizeof(KPH_CLIENT),
                             &client,
                             process);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      COMMS,
                      "Failed to allocate client object");

        client = NULL;
        goto Exit;
    }

    client->Port = ClientPort;

    KphReferenceObject(client);
    KphAcquireRWLockExclusive(&KphpConnectedClientLock);
    InsertTailList(&KphpConnectedClientList, &client->Entry);
    KphReleaseRWLock(&KphpConnectedClientLock);

    KphTracePrint(TRACE_LEVEL_INFORMATION,
                  COMMS,
                  "Client connected: %lu (0x%08x)",
                  HandleToULong(client->Process->ProcessId),
                  processState);

    *ConnectionPortCookie = client;
    status = STATUS_SUCCESS;

Exit:

    if (client)
    {
        KphDereferenceObject(client);
    }

    if (process)
    {
        KphDereferenceObject(process);
    }

    return status;
}

/**
 * \brief Communication port disconnect notification callback.
 *
 * \param[in] ConnectionCookie Client object
 */
_Function_class_(PFLT_DISCONNECT_NOTIFY)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID FLTAPI KphpCommsDisconnectNotifyCallback(
    _In_ PVOID ConnectionCookie
    )
{
    PKPH_CLIENT client;

    PAGED_PASSIVE();

    NT_ASSERT(ConnectionCookie);

    client = (PKPH_CLIENT)ConnectionCookie;

    KphAcquireRWLockExclusive(&KphpConnectedClientLock);
    RemoveEntryList(&client->Entry);
    KphReleaseRWLock(&KphpConnectedClientLock);

    KphTracePrint(TRACE_LEVEL_INFORMATION,
                  COMMS,
                  "Client disconnected: %lu",
                  HandleToULong(client->Process->ProcessId));

    KphDereferenceObject(client);
}

/**
 * \brief Signals clients when a required state failure occurs on inbound requests. 
 *
 * \param[in] MessageId The message ID that failed.
 * \param[in] ClientState The client state that was checked.
 * \param[in] RequiredState The state that was required.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpSendRequiredStateFailure(
    _In_ KPH_MESSAGE_ID MessageId,
    _In_ KPH_PROCESS_STATE ClientState,
    _In_ KPH_PROCESS_STATE RequiredState
    )
{
    PKPH_MESSAGE msg;

    PAGED_PASSIVE();

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "Failed to allocate message");
        return;
    }

    KphMsgInit(msg, KphMsgRequiredStateFailure);
    msg->Kernel.RequiredStateFailure.ClientId.UniqueProcess = PsGetCurrentProcessId();
    msg->Kernel.RequiredStateFailure.ClientId.UniqueThread = PsGetCurrentThreadId();
    msg->Kernel.RequiredStateFailure.MessageId = MessageId;
    msg->Kernel.RequiredStateFailure.ClientState = ClientState;
    msg->Kernel.RequiredStateFailure.RequiredState = RequiredState;

    if (KphInformerSettings.EnableStackTraces)
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);
}

/**
 * \brief Connection port message notification callback.
 *
 * \param[in] PortCookie Client object
 * \param[in] InputBuffer Input buffer from client.
 * \param[in] InputBufferLength Length of the input buffer.
 * \param[out] OutputBuffer Output buffer from client.
 * \param[in] OutputBufferLength Length of the output buffer.
 * \param[out] ReturnOutputBufferLength Set to the number of bytes written to
 * the output buffer.
 *
 * \return Successful or errant status.
 */
_Function_class_(PFLT_MESSAGE_NOTIFY)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS FLTAPI KphpCommsMessageNotifyCallback(
    _In_ PVOID PortCookie,
    _In_opt_ PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_opt_ PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _Out_ PULONG ReturnOutputBufferLength
    )
{
    NTSTATUS status;
    PKPH_CLIENT client;
    PKPH_MESSAGE msg;
    const KPH_MESSAGE_HANDLER* handler;
    KPH_PROCESS_STATE processState;
    KPH_PROCESS_STATE requiredState;

    PAGED_PASSIVE();

    client = (PKPH_CLIENT)PortCookie;

    msg = NULL;

    if ((PsGetCurrentProcess() != client->Process->EProcess) ||
        (ExGetPreviousMode() != UserMode))
    {
        KphTracePrint(TRACE_LEVEL_CRITICAL,
                      COMMS,
                      "Unexpected caller process!");

        status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    *ReturnOutputBufferLength = 0;

    if (!InputBuffer ||
        (InputBufferLength < KPH_MESSAGE_MIN_SIZE) ||
        (InputBufferLength > sizeof(KPH_MESSAGE)) ||
        OutputBuffer ||
        (OutputBufferLength > 0))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      COMMS,
                      "Client message input invalid");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_ERROR, COMMS, "Failed to allocate message");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    __try
    {
        ProbeForRead(InputBuffer, KPH_MESSAGE_MIN_SIZE, 1);
        RtlCopyMemory(msg, InputBuffer, KPH_MESSAGE_MIN_SIZE);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
        goto Exit;
    }

    status = KphMsgValidate(msg);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      COMMS,
                      "KphMsgValidate failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (msg->Header.Size > KPH_MESSAGE_MIN_SIZE)
    {
        __try
        {
            ProbeForRead(Add2Ptr(InputBuffer, KPH_MESSAGE_MIN_SIZE),
                         (msg->Header.Size - KPH_MESSAGE_MIN_SIZE),
                         1);
            RtlCopyMemory(&msg->_Dyn.Buffer[0],
                          Add2Ptr(InputBuffer, KPH_MESSAGE_MIN_SIZE),
                          (msg->Header.Size - KPH_MESSAGE_MIN_SIZE));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }

    if ((msg->Header.MessageId <= InvalidKphMsg) ||
        (msg->Header.MessageId >= MaxKphMsgClient) ||
        ((ULONG)msg->Header.MessageId >= KphCommsMessageHandlerCount))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      COMMS,
                      "Client message ID invalid: %lu",
                      (ULONG)msg->Header.MessageId);

        status = STATUS_MESSAGE_NOT_FOUND;
        goto Exit;
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  COMMS,
                  "Received input (%lu - %!TIME!) from client: %lu",
                  (ULONG)msg->Header.MessageId,
                  msg->Header.TimeStamp.QuadPart,
                  HandleToULong(client->Process->ProcessId));

    handler = &KphCommsMessageHandlers[msg->Header.MessageId];

    NT_ASSERT(handler->MessageId == msg->Header.MessageId);
    NT_ASSERT(handler->Handler);
    NT_ASSERT(handler->RequiredState);

    processState = KphGetProcessState(client->Process);
    requiredState = handler->RequiredState(msg);

    if ((processState & requiredState) != requiredState)
    {
        KphTracePrint(TRACE_LEVEL_CRITICAL,
                      COMMS,
                      "Untrusted client %lu (0x%08x, 0x%08x)",
                      HandleToULong(client->Process->ProcessId),
                      processState,
                      requiredState);

        KphpSendRequiredStateFailure(handler->MessageId,
                                     processState,
                                     requiredState);

        status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    status = handler->Handler(msg);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    NT_ASSERT(NT_SUCCESS(KphMsgValidate(msg)));

    //
    // We use the input buffer as the input and output. This is on purpose
    // to minimize the allocations and copies we have to do.
    //
    if (InputBufferLength < msg->Header.Size)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    }

    __try
    {
        ProbeForWrite(InputBuffer, msg->Header.Size, 1);
        RtlCopyMemory(InputBuffer, msg, msg->Header.Size);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
        goto Exit;
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  COMMS,
                  "Sending output (%lu - %!TIME!) to client: %lu",
                  (ULONG)msg->Header.MessageId,
                  msg->Header.TimeStamp.QuadPart,
                  HandleToULong(client->Process->ProcessId));

    status = STATUS_SUCCESS;

Exit:

    if (msg)
    {
        KphFreeMessage(msg);
    }

    return status;
}

/**
 * \brief Frees the communication port security descriptor.
 *
 * \param[in] SecurityDescriptor Security descriptor to free.
 */
_IRQL_always_function_max_(PASSIVE_LEVEL)
VOID KphpFreeCommsSecurityDescriptor(
    _In_freesMem_ PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    PAGED_PASSIVE();

    FltFreeSecurityDescriptor(SecurityDescriptor);
}

/**
 * \brief Builds the communication port security descriptor.
 *
 * \param[out] SecurityDescriptor On success set to the security descriptor for
 * the communication port. Null on failure. The caller should free this pointer
 * with KphpFreeCommsSecurityDescriptor.
 *
 * \return Successful or errant status.
 */
_IRQL_always_function_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpBuildCommsSecurityDescriptor(
    _Outptr_allocatesMem_ PSECURITY_DESCRIPTOR* SecurityDescriptor
    )
{
    NTSTATUS status;

    PAGED_PASSIVE();

    status = FltBuildDefaultSecurityDescriptor(SecurityDescriptor,
                                               FLT_PORT_ALL_ACCESS);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      COMMS,
                      "FltBuildDefaultSecurityDescriptor failed: %!STATUS!",
                      status);

        *SecurityDescriptor = NULL;
        goto Exit;
    }

Exit:

    return status;
}

/**
 * \brief Wrapper for the communication port message send API.
 *
 * \param[in] ClientPort Client port to send message to.
 * \param[in] SendBuffer Buffer to send.
 * \param[in] SendBufferLength Length of send buffer.
 * \param[out] ReplyBuffer Reply buffer.
 * \param[in,out] ReplyBufferLength Length of reply buffer on input, set to
 * number of bytes written to reply buffer on output.
 * \param[in] Timeout Time allotted for message to be received. If a reply is
 * expected this waits for a a reply to be received too. If not provided the
 * call waits indefinitely.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphpFltSendMessage(
    _In_ PFLT_PORT* ClientPort,
    _In_reads_bytes_(SendBufferLength) PVOID SendBuffer,
    _In_ ULONG SendBufferLength,
    _Out_writes_bytes_opt_(*ReplyBufferLength) PVOID ReplyBuffer,
    _Inout_opt_ PULONG ReplyBufferLength,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    NTSTATUS status;

    PAGED_CODE();

    NT_ASSERT(KphFltFilter);

    status = FltObjectReference(KphFltFilter);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      COMMS,
                      "FltObjectReference failed: %!STATUS!",
                      status);

        return status;
    }

    status = FltSendMessage(KphFltFilter,
                            ClientPort,
                            SendBuffer,
                            SendBufferLength,
                            ReplyBuffer,
                            ReplyBufferLength,
                            Timeout);
    if (status == STATUS_TIMEOUT)
    {
        //
        // return an error status instead
        //
        status = STATUS_IO_TIMEOUT;
    }

    FltObjectDereference(KphFltFilter);

    return status;
}

/**
 * \brief Sends a message asynchronously to a single target client process.
 *
 * \param[in] Message The message to send asynchronously. This function assumes
 * ownership over the message. The caller should *not* free the message after
 * it is passed to this function.
 * \param[in] TargetClientProcess Optional target client process to send the
 * message to. If provided the message will only be sent to the target client
 * from the queue processing. Otherwise, the message will be sent to all
 * clients.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphpCommsSendMessageAsync(
    _In_aliasesMem_ PKPH_MESSAGE Message,
    _In_opt_ PEPROCESS TargetClientProcess
    )
{
    PKPHM_QUEUE_ITEM item;

    PAGED_CODE();

    if (!KphAcquireRundown(&KphpCommsRundown))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      COMMS,
                      "Failed to acquire rundown, dropping message (%lu - %!TIME!)",
                      (ULONG)Message->Header.MessageId,
                      Message->Header.TimeStamp.QuadPart);

        KphFreeMessage(Message);
        return;
    }

    item = KphpAllocateMessageQueueItem();
    if (!item)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      COMMS,
                      "Failed to allocate queue item, dropping message (%lu - %!TIME!)",
                      (ULONG)Message->Header.MessageId,
                      Message->Header.TimeStamp.QuadPart);

        KphFreeMessage(Message);
        KphReleaseRundown(&KphpCommsRundown);
        return;
    }

    item->Message = Message;
    item->NonPaged = FALSE;
    item->TargetClientProcess = TargetClientProcess;

    KeInsertHeadQueue(&KphpMessageQueue, &item->Entry);

    KphReleaseRundown(&KphpCommsRundown);
}

/**
 * \brief Sends a message to all connected clients. The last client to connect
 * is given authority for any reply.
 *
 * \param[in] Message Message to send.
 * \param[out] Reply Reply from last client.
 * \param[in] Timeout Time in milliseconds for each client to receive the
 * message, and if appropriate, for the last client to reply.
 * \param[in] TargetClientProcess Optional target client process to send the
 * message to. If provided the message will only be sent to the target client
 * from the queue processing. Otherwise, the message will be sent to all
 * clients. If TargetClientProcess is provided Reply must be null.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphpCommsSendMessage(
    _In_ PKPH_MESSAGE Message,
    _Out_opt_ PKPH_MESSAGE Reply,
    _In_ ULONG TimeoutMs,
    _In_opt_ PEPROCESS TargetClientProcess
    )
{
    NTSTATUS status;
    LARGE_INTEGER timeout;

    PAGED_CODE();

    NT_ASSERT(!TargetClientProcess || !Reply);

    NT_ASSERT(NT_SUCCESS(KphMsgValidate(Message)));

    if (TimeoutMs < KPH_COMMS_MIN_TIMEOUT_MS)
    {
        timeout.QuadPart = (-10000ll * KPH_COMMS_MIN_TIMEOUT_MS);
    }
    else if (TimeoutMs > KPH_COMMS_MAX_TIMEOUT_MS)
    {
        timeout.QuadPart = (-10000ll * KPH_COMMS_MAX_TIMEOUT_MS);
    }
    else
    {
        timeout.QuadPart = (-10000ll * TimeoutMs);
    }

    KphAcquireRWLockShared(&KphpConnectedClientLock);

    if (IsListEmpty(&KphpConnectedClientList))
    {
        KphReleaseRWLock(&KphpConnectedClientLock);

        return STATUS_CONNECTION_DISCONNECTED;
    }

    status = (Reply ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS);
    for (PLIST_ENTRY entry = KphpConnectedClientList.Flink;
         entry != &KphpConnectedClientList;
         entry = entry->Flink)
    {
        PKPH_MESSAGE reply;
        ULONG replyLength;
        NTSTATUS status2;
        PKPH_CLIENT client;
        KPH_PROCESS_STATE processState;

        client = CONTAINING_RECORD(entry, KPH_CLIENT, Entry);

        if (TargetClientProcess &&
            (TargetClientProcess != client->Process->EProcess))
        {
            continue;
        }

        //
        // Since we support multiple clients and only one client may be the
        // authoritative reply. We choose to honor the reply of the last
        // client to connect.
        //
        if (entry == KphpConnectedClientList.Blink)
        {
            reply = Reply;
            replyLength = (Reply ? sizeof(*Reply) : 0);
        }
        else
        {
            reply = NULL;
            replyLength = 0;
        }

        if (reply && (PsGetCurrentProcess() == client->Process->EProcess))
        {
            PKPH_MESSAGE async;

            NT_ASSERT(!TargetClientProcess);

            //
            // This is a precaution to prevent a bottleneck. In this case the
            // kernel will block for the client to fully reply. If the client
            // generates more activity that too ends up fully synchronous it
            // could exhaust the client thread pool. This may introduce a
            // bottleneck into the system. To avoid this we force this one
            // message to be sent to the target client asynchronously. The
            // client will not be permitted to reply but will still have
            // visibility.
            //
            KphTracePrint(TRACE_LEVEL_INFORMATION,
                          COMMS,
                          "Bottleneck protection, forcing asynchronous send "
                          "(%lu - %!TIME!) to client: %lu",
                          (ULONG)Message->Header.MessageId,
                          Message->Header.TimeStamp.QuadPart,
                          HandleToULong(client->Process->ProcessId));

            //
            // We must make a copy of this message to send asynchronously.
            //
            async = KphAllocateMessage();
            if (!async)
            {
                KphTracePrint(TRACE_LEVEL_ERROR,
                              COMMS,
                              "Failed to allocate message");

                continue;
            }

            RtlCopyMemory(async, Message, Message->Header.Size);

            KphpCommsSendMessageAsync(async, client->Process->EProcess);

            continue;
        }

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      COMMS,
                      "Sending message (%lu - %!TIME!) to client: %lu",
                      (ULONG)Message->Header.MessageId,
                      Message->Header.TimeStamp.QuadPart,
                      HandleToULong(client->Process->ProcessId));

        status2 = KphpFltSendMessage(&client->Port,
                                     Message,
                                     Message->Header.Size,
                                     reply,
                                     (reply ? &replyLength : NULL),
                                     &timeout);

        if (!reply || !NT_SUCCESS(status2))
        {
            continue;
        }

        processState = KphGetProcessState(client->Process);
        if ((processState & KPH_PROCESS_STATE_MAXIMUM) != KPH_PROCESS_STATE_MAXIMUM)
        {
            KphTracePrint(TRACE_LEVEL_CRITICAL,
                          COMMS,
                          "Untrusted client %lu (0x%08x)",
                          HandleToULong(client->Process->ProcessId),
                          processState);

            status2 = STATUS_REPLY_MESSAGE_MISMATCH;
        }
        else
        {
            status2 = KphMsgValidate(reply);
            if (!NT_SUCCESS(status2))
            {
                KphTracePrint(TRACE_LEVEL_ERROR,
                              COMMS,
                              "Received invalid reply from client: %lu",
                              HandleToULong(client->Process->ProcessId));
            }
            else
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              COMMS,
                              "Received reply (%lu - %!TIME!) from client: %lu",
                              (ULONG)reply->Header.MessageId,
                              reply->Header.TimeStamp.QuadPart,
                              HandleToULong(client->Process->ProcessId));
            }
        }

        status = status2;
    }

    KphReleaseRWLock(&KphpConnectedClientLock);

    return status;
}

/**
 * \brief Async message queue thread.
 *
 * \param[in] StartContext Unused
 */
_Function_class_(KSTART_ROUTINE)
VOID KphpMessageQueueThread (
    _In_ PVOID StartContext
    )
{
    PAGED_PASSIVE();

    UNREFERENCED_PARAMETER(StartContext);

    for (;;)
    {
        NTSTATUS status;
        PLIST_ENTRY entry;
        PKPHM_QUEUE_ITEM item;

        entry = KeRemoveQueue(&KphpMessageQueue, KernelMode, NULL);

        status = (NTSTATUS)(LONG_PTR)entry;

        if ((status == STATUS_TIMEOUT) ||
            (status == STATUS_USER_APC))
        {
            KphTracePrint(TRACE_LEVEL_INFORMATION,
                          COMMS,
                          "Unexpected queue status: %!STATUS!",
                          status);

            continue;
        }

        if (status == STATUS_ABANDONED)
        {
            //
            // The rundown logic is responsible for draining/servicing the
            // remaining queue items.
            //
            KphTracePrint(TRACE_LEVEL_INFORMATION,
                          COMMS,
                          "Message queue running down");

            break;
        }

        item = CONTAINING_RECORD(entry, KPHM_QUEUE_ITEM, Entry);

        status = KphpCommsSendMessage(item->Message,
                                      NULL,
                                      KPH_COMMS_DEFAULT_TIMEOUT,
                                      item->TargetClientProcess);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          COMMS,
                          "Failed to send message (%lu - %!TIME!): %!STATUS!",
                          (ULONG)item->Message->Header.MessageId,
                          item->Message->Header.TimeStamp.QuadPart,
                          status);
        }

        KphpFreeMessageQueueItem(item);
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}

/**
 * \brief Starts the communications infrastructure.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCommsStart(
    VOID
    )
{
    NTSTATUS status;
    KPH_OBJECT_TYPE_INFO typeInfo;
    OBJECT_ATTRIBUTES objectAttributes;
    PSECURITY_DESCRIPTOR securityDescriptor;
    ULONG threadCount;

    PAGED_PASSIVE();
    NT_ASSERT(KphFltFilter);
    NT_ASSERT(!KphpFltServerPort);
    NT_ASSERT(KphDynPortName);

    securityDescriptor = NULL;

    KphInitializeRundown(&KphpCommsRundown);

    threadCount = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);

    typeInfo.Allocate = KphpAllocateClientObject;
    typeInfo.Initialize = KphpInitializeClientObject;
    typeInfo.Delete = KphpDeleteClientObject;
    typeInfo.Free = KphpFreeClientObject;

    KphCreateObjectType(&KphpClientObjectName,
                        &typeInfo,
                        &KphpClientObjectType);

    KphInitializeRWLock(&KphpConnectedClientLock);
    InitializeListHead(&KphpConnectedClientList);

    KphInitializePagedLookaside(&KphpMessageLookaside,
                                sizeof(KPH_MESSAGE),
                                KPH_TAG_MESSAGE);

    KphInitializeNPagedLookaside(&KphpNPagedMessageLookaside,
                                 sizeof(KPH_MESSAGE),
                                 KPH_TAG_NPAGED_MESSAGE);

    KphInitializeNPagedLookaside(&KphpMessageQueueItemLookaside,
                                 sizeof(KPHM_QUEUE_ITEM),
                                 KPH_TAG_QUEUE_ITEM);

    if (threadCount < KPH_COMMS_MIN_QUEUE_THREADS)
    {
        threadCount = KPH_COMMS_MIN_QUEUE_THREADS;
    }
    else if (threadCount > KPH_COMMS_MAX_QUEUE_THREADS)
    {
        threadCount = KPH_COMMS_MAX_QUEUE_THREADS;
    }

    KeInitializeQueue(&KphpMessageQueue, threadCount);

    KphpMessageQueueThreads = KphAllocatePaged((sizeof(PKTHREAD) * threadCount),
                                               KPH_TAG_THREAD_POOL);
    if (!KphpMessageQueueThreads)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      COMMS,
                      "Failed to allocate queue threads array");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    NT_ASSERT(KphpMessageQueueThreadsCount == 0);
    for (ULONG i = 0; i < threadCount; i++)
    {
        HANDLE threadHandle;

        InitializeObjectAttributes(&objectAttributes,
                                   NULL,
                                   OBJ_KERNEL_HANDLE,
                                   NULL,
                                   NULL);

        status = PsCreateSystemThread(&threadHandle,
                                      THREAD_ALL_ACCESS,
                                      &objectAttributes,
                                      NULL,
                                      NULL,
                                      KphpMessageQueueThread,
                                      NULL);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          COMMS,
                          "PsCreateSystemThread failed: %!STATUS!",
                          status);

            goto Exit;
        }

        status = ObReferenceObjectByHandle(threadHandle,
                                           THREAD_ALL_ACCESS,
                                           *PsThreadType,
                                           KernelMode,
                                           &KphpMessageQueueThreads[i],
                                           NULL);
        NT_ASSERT(NT_SUCCESS(status));
        ObCloseHandle(threadHandle, KernelMode);

        ++KphpMessageQueueThreadsCount;
    }

    status = KphpBuildCommsSecurityDescriptor(&securityDescriptor);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      COMMS,
                      "KphpBuildCommsSecurityDescriptor failed: %!STATUS!",
                      status);

        goto Exit;
    }

    InitializeObjectAttributes(&objectAttributes,
                               KphDynPortName,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               securityDescriptor);

    status = FltCreateCommunicationPort(KphFltFilter,
                                        &KphpFltServerPort,
                                        &objectAttributes,
                                        NULL,
                                        KphpCommsConnectNotifyCallback,
                                        KphpCommsDisconnectNotifyCallback,
                                        KphpCommsMessageNotifyCallback,
                                        LONG_MAX);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      COMMS,
                      "FltCreateCommunicationPort failed: %!STATUS!",
                      status);

        KphpFltServerPort = NULL;
        goto Exit;
    }

    status = STATUS_SUCCESS;

Exit:

    if (!NT_SUCCESS(status))
    {
        NT_ASSERT(!KphpFltServerPort);

        KphDeleteNPagedLookaside(&KphpMessageQueueItemLookaside);
        KphDeleteNPagedLookaside(&KphpNPagedMessageLookaside);
        KphDeletePagedLookaside(&KphpMessageLookaside);

        KphDeleteRWLock(&KphpConnectedClientLock);

        NT_VERIFY(KeRundownQueue(&KphpMessageQueue) == NULL);

        if (KphpMessageQueueThreads)
        {
            for (ULONG i = 0; i < KphpMessageQueueThreadsCount; i++)
            {
                KeWaitForSingleObject(KphpMessageQueueThreads[i],
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);
                ObDereferenceObject(KphpMessageQueueThreads[i]);
            }
            KphFree(KphpMessageQueueThreads, KPH_TAG_THREAD_POOL);
            KphpMessageQueueThreads = NULL;
            KphpMessageQueueThreadsCount = 0;
        }
    }

    if (securityDescriptor)
    {
        KphpFreeCommsSecurityDescriptor(securityDescriptor);
    }

    return status;
}

/**
 * \brief Stops the communications infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCommsStop(
    VOID
    )
{
    PLIST_ENTRY entry;

    PAGED_PASSIVE();

    if (!KphpFltServerPort)
    {
        return;
    }

    KphWaitForRundown(&KphpCommsRundown);

    entry = KeRundownQueue(&KphpMessageQueue);

    for (ULONG i = 0; i < KphpMessageQueueThreadsCount; i++)
    {
        KeWaitForSingleObject(KphpMessageQueueThreads[i],
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
        ObDereferenceObject(KphpMessageQueueThreads[i]);
    }

    KphFree(KphpMessageQueueThreads, KPH_TAG_THREAD_POOL);
    KphpMessageQueueThreads = NULL;
    KphpMessageQueueThreadsCount = 0;

    //
    // Rundown any remaining items in the queue, if there are any.
    //
    if (entry != NULL)
    {
        NTSTATUS status;
        PLIST_ENTRY first;

        first = entry;

        do
        {
            PKPHM_QUEUE_ITEM item;

            item = CONTAINING_RECORD(entry, KPHM_QUEUE_ITEM, Entry);

            entry = entry->Flink;

            status = KphpCommsSendMessage(item->Message,
                                          NULL,
                                          KPH_COMMS_DEFAULT_TIMEOUT,
                                          item->TargetClientProcess);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              COMMS,
                              "Failed to send message (%lu - %!TIME!): %!STATUS!",
                              (ULONG)item->Message->Header.MessageId,
                              item->Message->Header.TimeStamp.QuadPart,
                              status);
            }

            KphpFreeMessageQueueItem(item);

        } while (entry != first);
    }

    FltCloseCommunicationPort(KphpFltServerPort);

    KphDeleteNPagedLookaside(&KphpMessageQueueItemLookaside);
    KphDeleteNPagedLookaside(&KphpNPagedMessageLookaside);
    KphDeletePagedLookaside(&KphpMessageLookaside);

    KphDeleteRWLock(&KphpConnectedClientLock);

    KphpFltServerPort = NULL;
}

/**
 * \brief Allocates a message.
 *
 * \return Allocated message, null on allocation failure.
 */
_IRQL_requires_max_(APC_LEVEL)
_Return_allocatesMem_
PKPH_MESSAGE KphAllocateMessage(
    VOID
    )
{
    PAGED_CODE();

    return KphAllocateFromPagedLookaside(&KphpMessageLookaside);
}

/**
 * \brief Frees a message.
 *
 * \param[in] Message The message to free.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID
KphFreeMessage(
    _In_freesMem_ PKPH_MESSAGE Message
    )
{
    PAGED_CODE();

    NT_ASSERT(Message);

    KphFreeToPagedLookaside(&KphpMessageLookaside, Message);
}

/**
 * \brief Sends a message asynchronously.
 *
 * \param[in] Message The message to send asynchronously. This function assumes
 * ownership over the message. The caller should *not* free the message after
 * it is passed to this function.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphCommsSendMessageAsync(
    _In_aliasesMem_ PKPH_MESSAGE Message
    )
{
    PAGED_CODE();

    KphpCommsSendMessageAsync(Message, NULL);
}

/**
 * \brief Sends a message to all connected clients. The last client to connect
 * is given authority for any reply.
 *
 * \param[in] Message The message to send.
 * \param[out] Reply Optional reply from last client.
 * \param[in] Timeout Time in milliseconds for each client to receive the
 * message, and if appropriate, for the last client to reply.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphCommsSendMessage(
    _In_ PKPH_MESSAGE Message,
    _Out_opt_ PKPH_MESSAGE Reply,
    _In_ ULONG TimeoutMs
    )
{
    NTSTATUS status;

    PAGED_CODE();

    if (!KphAcquireRundown(&KphpCommsRundown))
    {
        return STATUS_TOO_LATE;
    }

    status = KphpCommsSendMessage(Message, Reply, TimeoutMs, NULL);

    KphReleaseRundown(&KphpCommsRundown);

    return status;
}

/**
 * \brief Captures the current stack and adds it to the message.
 *
 * \param[in,out] Message The message to populate with the current stack.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphCaptureStackInMessage(
    _Inout_ PKPH_MESSAGE Message
    )
{
    NTSTATUS status;
    PVOID frames[150];
    KPH_STACK_TRACE stack;

    PAGED_CODE();

    stack.Count = (USHORT)KphCaptureStack(frames, ARRAYSIZE(frames));
    if (stack.Count == 0)
    {
        return;
    }

    stack.Frames = frames;

    status = KphMsgDynAddStackTrace(Message, KphMsgFieldStackTrace, &stack);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_WARNING,
                      COMMS,
                      "KphMsgDynAddStackTrace failed: %!STATUS!",
                      status);
    }
}

