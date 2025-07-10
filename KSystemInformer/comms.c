/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2025
 *
 */

#include <kph.h>
#include <comms.h>
#include <informer.h>
#include <kphmsgdyn.h>

#include <trace.h>

#define KPH_COMMS_MIN_QUEUE_THREADS 2
#define KPH_COMMS_MAX_QUEUE_THREADS 64
#define KPH_COMMS_MAX_CLIENTS       32

typedef struct _KPHM_QUEUE_ITEM
{
    LIST_ENTRY Entry;
    BOOLEAN NonPaged;
    PKPH_MESSAGE Message;
    ULONG TargetClientCount;
    PKPH_CLIENT TargetClients[KPH_COMMS_MAX_CLIENTS];
} KPHM_QUEUE_ITEM, *PKPHM_QUEUE_ITEM;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const UNICODE_STRING KphpClientObjectName = RTL_CONSTANT_STRING(L"KphClient");
static const LARGE_INTEGER KphpMessageMinTimeout = KPH_TIMEOUT(300);
static const KPH_MESSAGE_TIMEOUTS KphpDefaultMessageTimeouts =
{
    .AsyncTimeout = KPH_TIMEOUT(3000),
    .DefaultTimeout = KPH_TIMEOUT(3000),
    .ProcessCreateTimeout = KPH_TIMEOUT(3000),
    .FilePreCreateTimeout = KPH_TIMEOUT(3000),
    .FilePostCreateTimeout = KPH_TIMEOUT(3000),
};
static const UNICODE_STRING KphpMessageQueueThreadName = RTL_CONSTANT_STRING(L"System Informer Message Queue");
KPH_PROTECTED_DATA_SECTION_RO_POP();
KPH_PROTECTED_DATA_SECTION_PUSH();
static PFLT_PORT KphpFltServerPort = NULL;
static PKPH_OBJECT_TYPE KphpClientObjectType = NULL;
KPH_PROTECTED_DATA_SECTION_POP();
static KPH_RUNDOWN KphpCommsRundown;
static PAGED_LOOKASIDE_LIST KphpMessageLookaside;
static NPAGED_LOOKASIDE_LIST KphpNPagedMessageLookaside;
static NPAGED_LOOKASIDE_LIST KphpMessageQueueItemLookaside;
static ALIGNED_EX_SPINLOCK KphpConnectedClientsLock = 0;
static ULONG KphpConnectedClientsCount = 0;
static PKPH_CLIENT KphpConnectedClients[KPH_COMMS_MAX_CLIENTS] = { NULL };
static KQUEUE KphpMessageQueue;
static PETHREAD* KphpMessageQueueThreads = NULL;
static ULONG KphpMessageQueueThreadsCount = 0;

/**
 * \brief Gets the number of connected clients.
 *
 * \return Number of connected clients.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
ULONG KphGetConnectedClientCount(
    VOID
    )
{
    ULONG count;
    KIRQL oldIrql;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    oldIrql = ExAcquireSpinLockShared(&KphpConnectedClientsLock);

    count = KphpConnectedClientsCount;

    ExReleaseSpinLockShared(&KphpConnectedClientsLock, oldIrql);

    return count;
}

/**
 * \brief Retrieves the connected clients. The caller is responsible for
 * dereferencing the clients after use.
 *
 * \param[out] Clients Receives the connected clients.
 *
 * \return Number of connected clients.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
ULONG KphpGetConnectedClients(
    _Out_writes_to_(KPH_COMMS_MAX_CLIENTS, return) PKPH_CLIENT* Clients
    )
{
    ULONG count;
    KIRQL oldIrql;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    oldIrql = ExAcquireSpinLockShared(&KphpConnectedClientsLock);

    count = KphpConnectedClientsCount;

    for (ULONG i = 0; i < KphpConnectedClientsCount; i++)
    {
        KphReferenceObject(KphpConnectedClients[i]);
        Clients[i] = KphpConnectedClients[i];
    }

    ExReleaseSpinLockShared(&KphpConnectedClientsLock, oldIrql);

    return count;
}

/**
 * \brief Adds a client to the connected clients list.
 *
 * \param[in] Client The client to add.
 *
 * \return TRUE if the client was added successfully, FALSE otherwise.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
BOOLEAN KphpAddConnectedClient(
    _In_ PKPH_CLIENT Client
    )
{
    BOOLEAN result;
    KIRQL oldIrql;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    result = FALSE;

    oldIrql = ExAcquireSpinLockExclusive(&KphpConnectedClientsLock);

    if (KphpConnectedClientsCount < KPH_COMMS_MAX_CLIENTS)
    {
        KphpConnectedClients[KphpConnectedClientsCount++] = Client;
        KphReferenceObject(Client);
        result = TRUE;
    }

    ExReleaseSpinLockExclusive(&KphpConnectedClientsLock, oldIrql);

    return result;
}

/**
 * \brief Removes a client from the connected clients list. The caller is
 * responsible for dereferencing any return client.
 *
 * \param[in] Client The client to remove.
 *
 * \return The removed client if it was found, NULL otherwise.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
PKPH_CLIENT KphpRemoveConnectedClient(
    _In_ PKPH_CLIENT Client
    )
{
    PKPH_CLIENT client;
    KIRQL oldIrql;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    client = NULL;

    oldIrql = ExAcquireSpinLockExclusive(&KphpConnectedClientsLock);

    for (ULONG i = 0; i < KphpConnectedClientsCount; i++)
    {
        if (KphpConnectedClients[i] != Client)
        {
            continue;
        }

        client = KphpConnectedClients[i];
        KphpConnectedClientsCount--;

        RtlMoveMemory(&KphpConnectedClients[i],
                      &KphpConnectedClients[i + 1],
                      sizeof(PKPH_CLIENT) * ((KPH_COMMS_MAX_CLIENTS - i) - 1));

        KphpConnectedClients[KPH_COMMS_MAX_CLIENTS - 1] = NULL;

        break;
    }

    ExReleaseSpinLockExclusive(&KphpConnectedClientsLock, oldIrql);

    return client;
}

/**
 * \brief Checks if the informer is enabled for a given client.
 *
 * \param[in] Client The client to check.
 * \param[in] Settings The settings to check.
 *
 * \return TRUE if the informer is enabled, FALSE otherwise.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
BOOLEAN KphpCommsInformerEnabled(
    _In_ PKPH_CLIENT Client,
    _In_ PCKPH_INFORMER_SETTINGS Settings
    )
{
    KPH_NPAGED_CODE_DISPATCH_MAX();

    return KphCheckInformerSettings(&Client->InformerSettings, Settings);
}

/**
 * \brief Checks if the informer is enabled for client communications.
 *
 * \details Checks if any of the connected clients have any of the passed
 * informer settings enabled. This is usually used as a upstream check to
 * avoid downstream work.
 *
 * \param[in] Settings The settings to check.
 *
 * \return TRUE if the informer is enabled, FALSE otherwise.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
BOOLEAN KphCommsInformerEnabled(
    _In_ PCKPH_INFORMER_SETTINGS Settings
    )
{
    BOOLEAN enabled;
    ULONG clientsCount;
    PKPH_CLIENT clients[KPH_COMMS_MAX_CLIENTS];

    KPH_NPAGED_CODE_DISPATCH_MAX();

    enabled = FALSE;

    clientsCount = KphpGetConnectedClients(clients);

    for (ULONG i = 0; i < clientsCount; i++)
    {
        if (!enabled)
        {
            enabled = KphpCommsInformerEnabled(clients[i], Settings);
        }

        KphDereferenceObjectDeferDelete(clients[i]);
    }

    return enabled;
}

/**
 * \brief Allocates a message queue item.
 *
 * \return Message queue item, null on allocation failure.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Return_allocatesMem_
PKPHM_QUEUE_ITEM KphpAllocateMessageQueueItem()
{
    KPH_NPAGED_CODE_DISPATCH_MAX();

    return KphAllocateFromNPagedLookaside(&KphpMessageQueueItemLookaside);
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
    KPH_NPAGED_CODE_DISPATCH_MAX();

    return KphAllocateFromNPagedLookaside(&KphpNPagedMessageLookaside);
}

/**
 * \brief Frees a non-paged message.
 *
 * \param[in] Message The message to free.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphFreeNPagedMessage(
    _In_freesMem_ PKPH_MESSAGE Message
    )
{
    NT_ASSERT(Message);
    KPH_NPAGED_CODE_DISPATCH_MAX();

    KphFreeToNPagedLookaside(&KphpNPagedMessageLookaside, Message);
}

/**
 * \brief Copies a message to a client ring buffer.
 *
 * \param[in] Client The client to copy the message to.
 * \param[in] Message The message to copy to the ring buffer.
 *
 * \return TRUE if the message was copied successfully, FALSE otherwise.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN KphpCommsCopyMessageToRingBuffer(
    _In_ PKPH_CLIENT Client,
    _In_ PKPH_MESSAGE Message
    )
{
    PVOID buffer;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    if (!Client->RingBuffer)
    {
        return FALSE;
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  COMMS,
                  "Sending message (%lu - %!TIME!) to client: %wZ (%lu)",
                  (ULONG)Message->Header.MessageId,
                  Message->Header.TimeStamp.QuadPart,
                  &Client->Process->ImageName,
                  HandleToULong(Client->Process->ProcessId));

    buffer = KphReserveRingBuffer(Client->RingBuffer,
                                  Message->Header.Size);
    if (!buffer)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      COMMS,
                      "Ring buffer exhausted (%lu - %!TIME!) %wZ (%lu)",
                      (ULONG)Message->Header.MessageId,
                      Message->Header.TimeStamp.QuadPart,
                      &Client->Process->ImageName,
                      HandleToULong(Client->Process->ProcessId));

        return FALSE;
    }

    RtlCopyMemory(buffer, Message, Message->Header.Size);
    KphCommitRingBuffer(Client->RingBuffer, buffer);

    return TRUE;
}

/**
 * \brief Sends a message to all connected client ring buffers. If a client can
 * not receive the message in its ring buffer, it is added to the output. The
 * caller is responsible for dereferencing the clients that could not receive
 * the message.
 *
 * \param[out] Clients Receives the clients that could not receive the message.
 * \param[in] Message The message to send to the clients.
 *
 * \return Number of clients that could not receive the message.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
ULONG KphpCommsSendMessageToRingBuffers(
    _Out_writes_to_(KPH_COMMS_MAX_CLIENTS, return) PKPH_CLIENT* Clients,
    _In_ PKPH_MESSAGE Message
    )
{
    ULONG count;
    ULONG clientsCount;
    PKPH_CLIENT clients[KPH_COMMS_MAX_CLIENTS];

    KPH_NPAGED_CODE_DISPATCH_MAX();

    count = 0;

    clientsCount = KphpGetConnectedClients(clients);

    for (ULONG i = 0; i < clientsCount; i++)
    {
        if (KphpCommsCopyMessageToRingBuffer(clients[i], Message))
        {
            KphDereferenceObjectDeferDelete(clients[i]);
            continue;
        }

        Clients[count++] = clients[i];
    }

    return count;
}

/**
 * \brief Sends a non-paged message asynchronously.
 *
 * \param[in] Message The message to send asynchronously. The call assumes
 * ownership over the message. The caller should *not* free the message after
 * it is passed to this function.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphCommsSendNPagedMessageAsync(
    _In_aliasesMem_ PKPH_MESSAGE Message
    )
{
    PKPHM_QUEUE_ITEM item;
    ULONG targetClientCount;
    PKPH_CLIENT targetClients[KPH_COMMS_MAX_CLIENTS];

    KPH_NPAGED_CODE_DISPATCH_MAX();

    if (!KphAcquireRundown(&KphpCommsRundown))
    {
        KphTracePrint(TRACE_LEVEL_WARNING,
                      COMMS,
                      "Failed to acquire rundown, dropping message "
                      "(%lu - %!TIME!)",
                      (ULONG)Message->Header.MessageId,
                      Message->Header.TimeStamp.QuadPart);

        KphFreeNPagedMessage(Message);
        return;
    }

    targetClientCount = KphpCommsSendMessageToRingBuffers(targetClients,
                                                          Message);
    if (!targetClientCount)
    {
        goto Exit;
    }

    item = KphpAllocateMessageQueueItem();
    if (!item)
    {
        KphTracePrint(TRACE_LEVEL_WARNING,
                      COMMS,
                      "Failed to allocate queue item, dropping message "
                      "(%lu - %!TIME!)",
                      (ULONG)Message->Header.MessageId,
                      Message->Header.TimeStamp.QuadPart);

        goto Exit;
    }

    item->Message = Message;
    item->NonPaged = TRUE;
    Message = NULL;

    item->TargetClientCount = targetClientCount;
    RtlCopyMemory(item->TargetClients,
                  targetClients,
                  sizeof(PKPH_CLIENT) * targetClientCount);
    targetClientCount = 0;

    KeInsertQueue(&KphpMessageQueue, &item->Entry);

Exit:

    for (ULONG i = 0; i < targetClientCount; i++)
    {
        KphDereferenceObjectDeferDelete(targetClients[i]);
    }

    if (Message)
    {
        KphFreeNPagedMessage(Message);
    }

    KphReleaseRundown(&KphpCommsRundown);
}

/**
 * \brief Captures the current stack and adds it to the message.
 *
 * \param[in,out] Message The message to populate with the current stack.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphCaptureStackInMessage(
    _Inout_ PKPH_MESSAGE Message
    )
{
    NTSTATUS status;
    PVOID frames[150];
    KPHM_STACK_TRACE stack;
    ULONG flags;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    flags = (KPH_STACK_BACK_TRACE_USER_MODE | KPH_STACK_BACK_TRACE_SKIP_KPH);

    stack.Count = (USHORT)KphCaptureStackBackTrace(0,
                                                   ARRAYSIZE(frames),
                                                   frames,
                                                   NULL,
                                                   flags);
    if (stack.Count == 0)
    {
        return;
    }

    stack.Frames = frames;

    status = KphMsgDynAddStackTrace(Message, KphMsgFieldStackTrace, &stack);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      COMMS,
                      "KphMsgDynAddStackTrace failed: %!STATUS!",
                      status);
    }
}

KPH_PAGED_FILE();

/**
 * \brief Frees a message queue item.
 *
 * \param[in] Item Message queue item to free.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphpFreeMessageQueueItem(_In_freesMem_ PKPHM_QUEUE_ITEM Item)
{
    KPH_PAGED_CODE();

    for (ULONG i = 0; i < Item->TargetClientCount; i++)
    {
        KphDereferenceObject(Item->TargetClients[i]);
    }

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
    KPH_PAGED_CODE_PASSIVE();

    return KphAllocateNPaged(Size, KPH_TAG_CLIENT);
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

    KPH_PAGED_CODE();

    NT_ASSERT(Parameter);

    client = Object;

    client->Process = Parameter;
    KphReferenceObject(client->Process);

    RtlCopyMemory(&client->MessageTimeouts,
                  &KphpDefaultMessageTimeouts,
                  sizeof(KPH_MESSAGE_TIMEOUTS));

    return STATUS_SUCCESS;
}

/**
 * \brief Deletes a client object.
 *
 * \param[in] Object The client object to delete.
 */
_Function_class_(KPH_TYPE_DELETE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpDeleteClientObject(
    _Inout_ PVOID Object
    )
{
    NTSTATUS status;
    PKPH_CLIENT client;

    KPH_PAGED_CODE_PASSIVE();

    client = Object;

    if (client->DriverUnloadProtectionRef.Count)
    {
        //
        // The client is being destroyed while it has acquired driver unload
        // protection. The communication handlers only acquire or release the
        // protection once for each client, so we only need to call this once
        // here.
        //
        status = KphReleaseDriverUnloadProtection(NULL);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          COMMS,
                          "KphReleaseDriverUnloadProtection failed: %!STATUS!",
                          status);
        }
    }

    KphDereferenceObject(client->Process);

    if (client->Port)
    {
        FltCloseClientPort(KphFltFilter, &client->Port);
    }

    if (client->RingBuffer)
    {
        KphDereferenceObject(client->RingBuffer);
    }
}

/**
 * \brief Frees a client object.
 *
 * \param[in] Object The client object to free.
 */
_Function_class_(KPH_TYPE_FREE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpFreeClientObject(
    _In_freesMem_ PVOID Object
    )
{
    KPH_PAGED_CODE_PASSIVE();

    KphFree(Object, KPH_TAG_CLIENT);
}

/**
 * \brief Initializes the client ring buffer.
 *
 * \param[in] Client The client to initialize the ring buffer for.
 * \param[in,out] Connection Pointer to the ring buffer connection context.
 *
 * \return Successful status or an errant one.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS KphpCommsInitializeRingBuffer(
    _In_ PKPH_CLIENT Client,
    _Inout_ PKPH_RING_BUFFER_CONNECT Connection
    )
{
    NTSTATUS status;
    PKPH_RING_BUFFER ring;
    KPH_RING_BUFFER_CONNECT connection;
    PKEVENT event;

    KPH_PAGED_CODE_PASSIVE();

    ring = NULL;
    event = NULL;

    __try
    {
        RtlCopyFromUser(&connection,
                        Connection,
                        sizeof(KPH_RING_BUFFER_CONNECT));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
        goto Exit;
    }

    if (connection.EventHandle)
    {
        status = ObReferenceObjectByHandle(connection.EventHandle,
                                           EVENT_MODIFY_STATE,
                                           *ExEventObjectType,
                                           UserMode,
                                           &event,
                                           NULL);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          COMMS,
                          "ObReferenceObjectByHandle failed: %!STATUS!",
                          status);

            event = NULL;
            goto Exit;
        }
    }

    status = KphCreateRingBuffer(&ring,
                                 &Connection->Ring,
                                 connection.Length,
                                 event,
                                 UserMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      COMMS,
                      "KphCreateRingBuffer failed: %!STATUS!",
                      status);

        ring = NULL;
        goto Exit;
    }

    Client->RingBuffer = ring;
    KphReferenceObject(ring);

Exit:

    if (ring)
    {
        KphDereferenceObject(ring);
    }

    if (event)
    {
        ObDereferenceObject(event);
    }

    return status;
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

    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(ServerPortCookie);

    *ConnectionPortCookie = NULL;

    process = NULL;
    client = NULL;

    if (!KphSinglePrivilegeCheck(SeDebugPrivilege, UserMode))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      COMMS,
                      "KphSinglePrivilegeCheck failed %lu",
                      HandleToULong(PsGetCurrentProcessId()));

        status = STATUS_PRIVILEGE_NOT_HELD;
        goto Exit;
    }

    process = KphGetCurrentProcessContext();
    if (!process)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      COMMS,
                      "Untracked process %lu",
                      HandleToULong(PsGetCurrentProcessId()));

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    processState = KphGetProcessState(process);
    if (!KphTestProcessState(processState, KPH_PROCESS_STATE_LOW))
    {
        KphTracePrint(TRACE_LEVEL_CRITICAL,
                      COMMS,
                      "Untrusted client %wZ (%lu) (0x%08x)",
                      &process->ImageName,
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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      COMMS,
                      "Failed to allocate client object");

        client = NULL;
        goto Exit;
    }

    if (ConnectionContext && (SizeOfContext >= sizeof(PVOID)))
    {
        PKPH_RING_BUFFER_CONNECT connection;

        NT_ASSERT(ConnectionContext > MmHighestUserAddress);

        connection = *(PVOID*)ConnectionContext;

        status = KphpCommsInitializeRingBuffer(client, connection);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          COMMS,
                          "KphpCommsInitializeRingBuffer failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    client->Port = ClientPort;

    if (!KphpAddConnectedClient(client))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      COMMS,
                      "Clients at maximum: %wZ (%lu) (0x%08x)",
                      &client->Process->ImageName,
                      HandleToULong(client->Process->ProcessId),
                      processState);

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    KphTracePrint(TRACE_LEVEL_INFORMATION,
                  COMMS,
                  "Client connected: %wZ (%lu) (0x%08x)",
                  &client->Process->ImageName,
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

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(ConnectionCookie);

    client = KphpRemoveConnectedClient(ConnectionCookie);
    if (client)
    {
        KphTracePrint(TRACE_LEVEL_INFORMATION,
                      COMMS,
                      "Client disconnected: %wZ (%lu)",
                      &client->Process->ImageName,
                      HandleToULong(client->Process->ProcessId));

        KphDereferenceObject(client);
    }
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

    KPH_PAGED_CODE_PASSIVE();

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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

    if (KphInformerEnabled(EnableStackTraces, NULL))
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
 * \param[out] ReturnOutputBufferLength Receives the number of bytes written.
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

    KPH_PAGED_CODE_PASSIVE();

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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      COMMS,
                      "Client message input invalid");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      COMMS,
                      "Failed to allocate message");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    __try
    {
        RtlCopyFromUser(msg, InputBuffer, KPH_MESSAGE_MIN_SIZE);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
        goto Exit;
    }

    status = KphMsgValidate(msg);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      COMMS,
                      "KphMsgValidate failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (msg->Header.Size > KPH_MESSAGE_MIN_SIZE)
    {
        __try
        {
            RtlCopyFromUser(msg->_Dyn.Buffer,
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
        KphTracePrint(TRACE_LEVEL_WARNING,
                      COMMS,
                      "Client message ID invalid: %lu",
                      (ULONG)msg->Header.MessageId);

        status = STATUS_MESSAGE_NOT_FOUND;
        goto Exit;
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  COMMS,
                  "Received input (%lu - %!TIME!) from client: %wZ (%lu)",
                  (ULONG)msg->Header.MessageId,
                  msg->Header.TimeStamp.QuadPart,
                  &client->Process->ImageName,
                  HandleToULong(client->Process->ProcessId));

    handler = &KphCommsMessageHandlers[msg->Header.MessageId];

    NT_ASSERT(handler->MessageId == msg->Header.MessageId);
    NT_ASSERT(handler->Handler);
    NT_ASSERT(handler->RequiredState);

    processState = KphGetProcessState(client->Process);
    requiredState = handler->RequiredState(client, msg);

    if (!KphTestProcessState(processState, requiredState))
    {
        KphTracePrint(TRACE_LEVEL_CRITICAL,
                      COMMS,
                      "Untrusted client %wZ (%lu) (0x%08x, 0x%08x)",
                      &client->Process->ImageName,
                      HandleToULong(client->Process->ProcessId),
                      processState,
                      requiredState);

        KphpSendRequiredStateFailure(handler->MessageId,
                                     processState,
                                     requiredState);

        status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    status = handler->Handler(client, msg);
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
        RtlCopyToUser(InputBuffer, msg, msg->Header.Size);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
        goto Exit;
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  COMMS,
                  "Sending output (%lu - %!TIME!) to client: %wZ (%lu)",
                  (ULONG)msg->Header.MessageId,
                  msg->Header.TimeStamp.QuadPart,
                  &client->Process->ImageName,
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
    KPH_PAGED_CODE_PASSIVE();

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

    KPH_PAGED_CODE_PASSIVE();

    status = FltBuildDefaultSecurityDescriptor(SecurityDescriptor,
                                               FLT_PORT_ALL_ACCESS);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
 * \brief Gets the timeouts for messages.
 *
 * \param[in] Client The client to get the timeouts from.
 * \param[out] Timeouts Receives the timeouts for messages.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphGetMessageTimeouts(
    _In_ PKPH_CLIENT Client,
    _Out_ PKPH_MESSAGE_TIMEOUTS Timeouts
    )
{
    KPH_PAGED_CODE();

#define KPH_GET_MESSAGE_TIMEOUT(t) \
    Timeouts->t.QuadPart = Client->MessageTimeouts.t.QuadPart

    KPH_GET_MESSAGE_TIMEOUT(AsyncTimeout);
    KPH_GET_MESSAGE_TIMEOUT(DefaultTimeout);
    KPH_GET_MESSAGE_TIMEOUT(ProcessCreateTimeout);
    KPH_GET_MESSAGE_TIMEOUT(FilePreCreateTimeout);
    KPH_GET_MESSAGE_TIMEOUT(FilePostCreateTimeout);
}

/**
 * \brief Sets the timeouts for messages.
 *
 * \param[in] Client The client to set the timeouts for.
 * \param[in] Timeouts The timeouts to apply.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphSetMessageTimeouts(
    _In_ PKPH_CLIENT Client,
    _In_ PKPH_MESSAGE_TIMEOUTS Timeouts
    )
{
    KPH_PAGED_CODE();

    //
    // Timeouts must be relative. Thus the timeout must be _less_ than or equal
    // to the minimum timeout.
    //
#define KPH_VALIDATE_MESSAGE_TIMEOUT(t) \
    (Timeouts->t.QuadPart <= KphpMessageMinTimeout.QuadPart)

    if (!KPH_VALIDATE_MESSAGE_TIMEOUT(AsyncTimeout) ||
        !KPH_VALIDATE_MESSAGE_TIMEOUT(DefaultTimeout) ||
        !KPH_VALIDATE_MESSAGE_TIMEOUT(ProcessCreateTimeout) ||
        !KPH_VALIDATE_MESSAGE_TIMEOUT(FilePreCreateTimeout) ||
        !KPH_VALIDATE_MESSAGE_TIMEOUT(FilePostCreateTimeout))
    {
        return STATUS_INVALID_PARAMETER;
    }

#define KPH_SET_MESSAGE_TIMEOUT(t) \
    Client->MessageTimeouts.t.QuadPart = Timeouts->t.QuadPart

    KPH_SET_MESSAGE_TIMEOUT(AsyncTimeout);
    KPH_SET_MESSAGE_TIMEOUT(DefaultTimeout);
    KPH_SET_MESSAGE_TIMEOUT(ProcessCreateTimeout);
    KPH_SET_MESSAGE_TIMEOUT(FilePreCreateTimeout);
    KPH_SET_MESSAGE_TIMEOUT(FilePostCreateTimeout);

    return STATUS_SUCCESS;
}

/**
 * \brief Retrieves the timeout for a message.
 *
 * \param[in] Client The client to get the timeout for.
 * \param[in] Message The message to retrieve the timeout for.
 * \param[in] AsyncTimeout If TRUE the asynchronous timeout is returned, else
 * the timeout appropriate for the message is returned.
 *
 * \return Timeout for the message.
 */
_IRQL_requires_max_(APC_LEVEL)
LARGE_INTEGER KphpGetTimeoutForMessage(
    _In_ PKPH_CLIENT Client,
    _In_ PKPH_MESSAGE Message,
    _In_ BOOLEAN AsyncTimeout
    )
{
    KPH_PAGED_CODE();

    if (AsyncTimeout)
    {
        return Client->MessageTimeouts.AsyncTimeout;
    }

    switch (Message->Header.MessageId)
    {
        case KphMsgProcessCreate:
        {
            return Client->MessageTimeouts.ProcessCreateTimeout;
        }
        case KphMsgFilePreCreate:
        {
            return Client->MessageTimeouts.FilePreCreateTimeout;
        }
        case KphMsgFilePostCreate:
        {
            return Client->MessageTimeouts.FilePostCreateTimeout;
        }
        default:
        {
            return Client->MessageTimeouts.DefaultTimeout;
        }
    }
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

    KPH_PAGED_CODE();

    NT_ASSERT(KphFltFilter);

    status = FltObjectReference(KphFltFilter);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
 * \param[in] TargetClient Optional target client to send the message to. If
 * provided the message will only be sent to the target client from the queue
 * processing. Otherwise, the message will be sent to all clients.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphpCommsSendMessageAsync(
    _In_aliasesMem_ PKPH_MESSAGE Message,
    _In_opt_ PKPH_CLIENT TargetClient
    )
{
    PKPHM_QUEUE_ITEM item;
    ULONG targetClientCount;
    PKPH_CLIENT targetClients[KPH_COMMS_MAX_CLIENTS];

    KPH_PAGED_CODE();

    if (!KphAcquireRundown(&KphpCommsRundown))
    {
        KphTracePrint(TRACE_LEVEL_WARNING,
                      COMMS,
                      "Failed to acquire rundown, dropping message "
                      "(%lu - %!TIME!)",
                      (ULONG)Message->Header.MessageId,
                      Message->Header.TimeStamp.QuadPart);

        KphFreeMessage(Message);
        return;
    }

    if (TargetClient)
    {
        targetClientCount = 1;
        targetClients[0] = TargetClient;
        KphReferenceObject(TargetClient);
    }
    else
    {
        targetClientCount = KphpCommsSendMessageToRingBuffers(targetClients,
                                                              Message);
        if (!targetClientCount)
        {
            goto Exit;
        }
    }

    item = KphpAllocateMessageQueueItem();
    if (!item)
    {
        KphTracePrint(TRACE_LEVEL_WARNING,
                      COMMS,
                      "Failed to allocate queue item, dropping message "
                      "(%lu - %!TIME!)",
                      (ULONG)Message->Header.MessageId,
                      Message->Header.TimeStamp.QuadPart);

        goto Exit;
    }

    item->Message = Message;
    item->NonPaged = FALSE;
    Message = NULL;

    item->TargetClientCount = targetClientCount;
    RtlCopyMemory(item->TargetClients,
                  targetClients,
                  sizeof(PKPH_CLIENT) * targetClientCount);
    targetClientCount = 0;

    KeInsertQueue(&KphpMessageQueue, &item->Entry);

Exit:

    for (ULONG i = 0; i < targetClientCount; i++)
    {
        KphDereferenceObject(targetClients[i]);
    }

    if (Message)
    {
        KphFreeMessage(Message);
    }

    KphReleaseRundown(&KphpCommsRundown);
}

/**
 * \brief Sends a message to a specific client.
 *
 * \param[in] Client The client to send the message to.
 * \param[in] Message The message to send. May contain multiple messages if
 * the send is originating from the asynchronous message queue and the
 * asynchronous message queue has coalesced multiple messages into one.
 * \param[in] Length The length of the message to send. May constitute multiple
 * messages if the send is originating from the asynchronous message queue and
 * the asynchronous message queue has coalesced multiple messages into one.
 * \param[out] Reply The reply from last client.
 * \param[in] FromAsyncQueue If TRUE the message is being sent from the
 * asynchronous message queue, otherwise FALSE.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphpCommsSendMessage(
    _In_ PKPH_CLIENT Client,
    _In_ PKPH_MESSAGE Message,
    _In_ ULONG Length,
    _Inout_opt_ PKPH_MESSAGE Reply,
    _In_ BOOLEAN FromAsyncQueue
    )
{
    NTSTATUS status;
    PKPH_MESSAGE reply;
    ULONG replyLength;
    KPH_PROCESS_STATE processState;
    LARGE_INTEGER timeout;

    KPH_PAGED_CODE();

    //
    // Since we support multiple clients and only one client may be the
    // authoritative reply. We choose to honor the first client to reply.
    //
    if (Reply && (Reply->Header.MessageId == KphMsgUnhandled))
    {
        reply = Reply;
        replyLength = sizeof(KPH_MESSAGE);
    }
    else
    {
        reply = NULL;
        replyLength = 0;
    }

    if (reply && (PsGetCurrentProcess() == Client->Process->EProcess))
    {
        PKPH_MESSAGE async;

        NT_ASSERT(!FromAsyncQueue);
        NT_ASSERT(Message->Header.Size == Length);

        //
        // This is a precaution to prevent a bottleneck. In this case the kernel
        // will block for the client to fully reply. If the client generates
        // more activity that too ends up fully synchronous it could exhaust
        // the client thread pool. This may introduce a bottleneck into the
        // system. To avoid this we force this one message to be sent to the
        // target client asynchronously. The client will not be permitted to
        // reply but will still have visibility.
        //
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      COMMS,
                      "Bottleneck protection, forcing asynchronous send "
                      "(%lu - %!TIME!) to client: %wZ (%lu)",
                      (ULONG)Message->Header.MessageId,
                      Message->Header.TimeStamp.QuadPart,
                      &Client->Process->ImageName,
                      HandleToULong(Client->Process->ProcessId));

        //
        // We must make a copy of this message to send asynchronously.
        //
        async = KphAllocateMessage();
        if (!async)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          COMMS,
                          "Failed to allocate message");

            return;
        }

        RtlCopyMemory(async, Message, Message->Header.Size);

        KphpCommsSendMessageAsync(async, Client);

        return;
    }

    for (ULONG offset = 0; offset < Length; NOTHING)
    {
        PKPH_MESSAGE message;

        NT_ASSERT(offset < sizeof(KPH_MESSAGE));

        message = Add2Ptr(Message, offset);

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      COMMS,
                      "Sending message (%lu - %!TIME!) to client: %wZ (%lu)",
                      (ULONG)message->Header.MessageId,
                      message->Header.TimeStamp.QuadPart,
                      &Client->Process->ImageName,
                      HandleToULong(Client->Process->ProcessId));

        offset += message->Header.Size;
    }

    timeout = KphpGetTimeoutForMessage(Client, Message, FromAsyncQueue);

    status = KphpFltSendMessage(&Client->Port,
                                Message,
                                Length,
                                reply,
                                (reply ? &replyLength : NULL),
                                &timeout);
    if (!reply)
    {
        NT_ANALYSIS_ASSUME(NT_SUCCESS(status));
        return;
    }

    if (NT_SUCCESS(status))
    {
        processState = KphGetProcessState(Client->Process);
        if (!KphTestProcessState(processState, KPH_PROCESS_STATE_MAXIMUM))
        {
            KphTracePrint(TRACE_LEVEL_CRITICAL,
                          COMMS,
                          "Untrusted client %wZ (%lu) (0x%08x)",
                          &Client->Process->ImageName,
                          HandleToULong(Client->Process->ProcessId),
                          processState);

            status = STATUS_REPLY_MESSAGE_MISMATCH;
        }
        else
        {
            status = KphMsgValidate(reply);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_WARNING,
                              COMMS,
                              "Received invalid reply from client: %wZ (%lu)",
                              &Client->Process->ImageName,
                              HandleToULong(Client->Process->ProcessId));
            }
            else
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              COMMS,
                              "Received reply (%lu - %!TIME!) from client: %wZ (%lu)",
                              (ULONG)reply->Header.MessageId,
                              reply->Header.TimeStamp.QuadPart,
                              &Client->Process->ImageName,
                              HandleToULong(Client->Process->ProcessId));
            }
        }
    }

    if (!NT_SUCCESS(status))
    {
        KphMsgInit(reply, KphMsgUnhandled);
    }
}

/**
 * \brief Attempts to coalesce consecutive items in a message queue to minimize
 * the number of individual message transactions.
 *
 * \details This function optimizes message transmission by combining compatible
 * items from the queue into a single message. If coalescing is not possible,
 * it simply returns the next item to be sent as-is.
 *
 * \param[in] Items The items to process.
 * \param[in] Count The number of items to process.
 * \param[in,out] Index The index of the first item to consider for coalescing.
 * Incremented for each coalesced item.
 * \param[out] Length Receives the length (in bytes) of the message to send,
 * including any coalesced items.
 * \param[out] Rundown Set to TRUE if rundown is active, FALSE otherwise.
 *
 * \return The next item to send, NULL otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
PKPHM_QUEUE_ITEM KphpMessageQueueCoalesceItems(
    _In_reads_(Count) PLIST_ENTRY* Items,
    _In_ ULONG Count,
    _Inout_ PULONG Index,
    _Out_ PULONG Length,
    _Out_ PBOOLEAN Rundown
    )
{
    NTSTATUS status;
    PKPHM_QUEUE_ITEM first;
    ULONG coalescedCount;
    ULONG remainingLength;
    PVOID messagePointer;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(*Index < Count);

    *Rundown = FALSE;

    first = NULL;
    remainingLength = sizeof(KPH_MESSAGE);
    coalescedCount = 0;

    status = (NTSTATUS)(LONG_PTR)Items[*Index];

    if ((status == STATUS_TIMEOUT) ||
        (status == STATUS_USER_APC))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      COMMS,
                      "Unexpected queue status: %!STATUS!",
                      status);

        goto Exit;
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

        *Rundown = TRUE;
        goto Exit;
    }

    first = CONTAINING_RECORD(Items[*Index], KPHM_QUEUE_ITEM, Entry);

    NT_ASSERT(first);

    messagePointer = Add2Ptr(first->Message, first->Message->Header.Size);
    remainingLength = (sizeof(KPH_MESSAGE) - first->Message->Header.Size);

    for (ULONG i = (*Index + 1); i < Count; i++)
    {
        PKPHM_QUEUE_ITEM item;
        BOOLEAN clientsMatch;

        status = (NTSTATUS)(LONG_PTR)Items[i];

        if ((status == STATUS_TIMEOUT) ||
            (status == STATUS_USER_APC))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
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

            *Rundown = TRUE;
            goto Exit;
        }

        item = CONTAINING_RECORD(Items[i], KPHM_QUEUE_ITEM, Entry);

        NT_ASSERT(item);

        //
        // Verify that the client list matches and there is sufficient room to
        // coalesce this message into the first.
        //

        if (first->TargetClientCount != item->TargetClientCount)
        {
            break;
        }

        clientsMatch = TRUE;

        for (ULONG j = 0; j < first->TargetClientCount; j++)
        {
            if (first->TargetClients[j] != item->TargetClients[j])
            {
                clientsMatch = FALSE;
                break;
            }
        }

        if (!clientsMatch)
        {
            break;
        }

        if (remainingLength < item->Message->Header.Size)
        {
            break;
        }

        RtlCopyMemory(messagePointer,
                      item->Message,
                      item->Message->Header.Size);

        messagePointer = Add2Ptr(messagePointer, item->Message->Header.Size);
        remainingLength -= item->Message->Header.Size;

        (*Index)++;
        coalescedCount++;

        KphpFreeMessageQueueItem(item);
    }

Exit:

    *Length = (sizeof(KPH_MESSAGE) - remainingLength);

    if (coalescedCount)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      COMMS,
                      "Coalesced %lu messages (%lu bytes)",
                      coalescedCount + 1,
                      *Length);
    }

    return first;
}

/**
 * \brief Processes message queue items.
 *
 * \param[in] Items The items to process.
 * \param[in] Count The number of items to process.
 *
 * \return TRUE if rundown is active, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KphpMessageQueueProcessItems(
    _In_reads_(Count) PLIST_ENTRY* Items,
    _In_ ULONG Count
    )
{
    BOOLEAN rundown;

    KPH_PAGED_CODE_PASSIVE();

    rundown = FALSE;

    for (ULONG i = 0; i < Count; i++)
    {
        PKPHM_QUEUE_ITEM item;
        ULONG length;

        item = KphpMessageQueueCoalesceItems(Items,
                                             Count,
                                             &i,
                                             &length,
                                             &rundown);
        if (item)
        {
            for (ULONG j = 0; j < item->TargetClientCount; j++)
            {
                KphpCommsSendMessage(item->TargetClients[j],
                                     item->Message,
                                     length,
                                     NULL,
                                     TRUE);
            }

            KphpFreeMessageQueueItem(item);
        }

        if (rundown)
        {
            break;
        }
    }

    return rundown;
}

/**
 * \brief Asynchronous message queue thread.
 *
 * \param[in] Parameter Unused
 */
_Function_class_(KPH_THREAD_START_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
NTSTATUS KphpMessageQueueThread(
    _In_opt_ PVOID Parameter
    )
{
    PLIST_ENTRY items[32];
    ULONG count;

    KPH_PAGED_CODE_PASSIVE();

    UNREFERENCED_PARAMETER(Parameter);

    count = ARRAYSIZE(items);

    for (;;)
    {
        RtlZeroMemory(items, count * sizeof(PLIST_ENTRY));

        count = KeRemoveQueueEx(&KphpMessageQueue,
                                KernelMode,
                                FALSE,
                                NULL,
                                items,
                                ARRAYSIZE(items));

        if (KphpMessageQueueProcessItems(items, count))
        {
            break;
        }
    }

    return STATUS_SUCCESS;
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

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(KphFltFilter);
    NT_ASSERT(!KphpFltServerPort);
    NT_ASSERT(KphPortName);

    securityDescriptor = NULL;

    KphInitializeRundown(&KphpCommsRundown);

    threadCount = KeQueryActiveProcessorCountEx(ALL_PROCESSOR_GROUPS);

    typeInfo.Allocate = KphpAllocateClientObject;
    typeInfo.Initialize = KphpInitializeClientObject;
    typeInfo.Delete = KphpDeleteClientObject;
    typeInfo.Free = KphpFreeClientObject;
    typeInfo.Flags = 0;

    KphCreateObjectType(&KphpClientObjectName,
                        &typeInfo,
                        &KphpClientObjectType);

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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      COMMS,
                      "Failed to allocate queue threads array");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    NT_ASSERT(KphpMessageQueueThreadsCount == 0);
    for (ULONG i = 0; i < threadCount; i++)
    {
        status = KphCreateSystemThread(NULL,
                                       &KphpMessageQueueThreads[i],
                                       KphpMessageQueueThread,
                                       NULL,
                                       &KphpMessageQueueThreadName,
                                       KPH_CREATE_SYSTEM_THREAD_IN_KSI_PROCESS);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          COMMS,
                          "KphCreateSystemThread failed: %!STATUS!",
                          status);

            KphpMessageQueueThreads[i] = NULL;
            goto Exit;
        }

        ++KphpMessageQueueThreadsCount;
    }

    status = KphpBuildCommsSecurityDescriptor(&securityDescriptor);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      COMMS,
                      "KphpBuildCommsSecurityDescriptor failed: %!STATUS!",
                      status);

        goto Exit;
    }

    InitializeObjectAttributes(&objectAttributes,
                               KphPortName,
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
                                        KPH_COMMS_MAX_CLIENTS);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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

    KPH_PAGED_CODE_PASSIVE();

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
        PLIST_ENTRY first;

        first = entry;

        do
        {
            PLIST_ENTRY item;

            item = entry;

            entry = entry->Flink;

            KphpMessageQueueProcessItems(&item, 1);

        } while (entry != first);
    }

    FltCloseCommunicationPort(KphpFltServerPort);

    KphDeleteNPagedLookaside(&KphpMessageQueueItemLookaside);
    KphDeleteNPagedLookaside(&KphpNPagedMessageLookaside);
    KphDeletePagedLookaside(&KphpMessageLookaside);
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
    KPH_PAGED_CODE();

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
    KPH_PAGED_CODE();

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
    KPH_PAGED_CODE();

    KphpCommsSendMessageAsync(Message, NULL);
}

/**
 * \brief Sends a message to all connected clients. The first client to respond
 * is given authority for any reply.
 *
 * \details Callers expecting a specific reply should check for the reply
 * message identifier even on success. This function will return success with
 * KphMsgUnhandled when no client handles the message.
 *
 * \param[in] Message The message to send.
 * \param[out] Reply Optional reply from last client.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphCommsSendMessage(
    _In_ PKPH_MESSAGE Message,
    _Out_opt_ PKPH_MESSAGE Reply
    )
{
    NTSTATUS status;
    ULONG clientCount;
    PKPH_CLIENT clients[KPH_COMMS_MAX_CLIENTS];

    KPH_PAGED_CODE();

    NT_ASSERT(NT_SUCCESS(KphMsgValidate(Message)));

    if (!KphAcquireRundown(&KphpCommsRundown))
    {
        return STATUS_TOO_LATE;
    }

    if (Reply)
    {
        KphMsgInit(Reply, KphMsgUnhandled);
    }

    status = STATUS_PORT_DISCONNECTED;

    clientCount = KphpGetConnectedClients(clients);

    for (ULONG i = 0; i < clientCount; i++)
    {
        KphpCommsSendMessage(clients[i],
                             Message,
                             Message->Header.Size,
                             Reply,
                             FALSE);

        KphDereferenceObject(clients[i]);

        status = STATUS_SUCCESS;
    }

    KphReleaseRundown(&KphpCommsRundown);

    return status;
}
