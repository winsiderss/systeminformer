/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2022-2025
 *     dmex    2022-2025
 *
 */

#include <ph.h>
#include <kphcomms.h>
#include <kphuser.h>
#include <kphringbuff.h>
#include <mapldr.h>

typedef struct _KPH_UMESSAGE
{
    FILTER_MESSAGE_HEADER MessageHeader;
    KPH_MESSAGE Message;
    OVERLAPPED Overlapped;
} KPH_UMESSAGE, *PKPH_UMESSAGE;

typedef struct _KPH_UREPLY
{
    FILTER_REPLY_HEADER ReplyHeader;
    KPH_MESSAGE Message;
} KPH_UREPLY, *PKPH_UREPLY;

PKPH_COMMS_CALLBACK KphpCommsRegisteredCallback = NULL;
HANDLE KphpCommsFltPortHandle = NULL;
BOOLEAN KphpCommsPortDisconnected = TRUE;
PTP_POOL KphpCommsThreadPool = NULL;
TP_CALLBACK_ENVIRON KphpCommsThreadPoolEnv;
PTP_IO KphpCommsThreadPoolIo = NULL;
PKPH_UMESSAGE KphpCommsMessages = NULL;
ULONG KphpCommsMessageCount = 0;
PH_RUNDOWN_PROTECT KphpCommsRundown;
PH_FREE_LIST KphpCommsReplyFreeList;
HANDLE KphpCommsRingBufferThread = NULL;
KPH_RING_BUFFER_CONNECT KphpCommsRingBuffer = { 0 };
ULONG KphpCommsTlsSlot = TLS_OUT_OF_INDEXES;

#define KPH_COMMS_MIN_THREADS           2
#define KPH_COMMS_MESSAGE_SCALE         2
#define KPH_COMMS_THREAD_SCALE          2
#define KPH_COMMS_MAX_MESSAGES          1024
#define KPH_COMMS_THREAD_PROPERTIES_SET UlongToPtr(1)

VOID KphpCommsSetThreadProperties(
    _In_z_ PCWSTR ThreadName,
    _In_ KPRIORITY Priority
    )
{
    if (PhTlsGetValue(KphpCommsTlsSlot) != KPH_COMMS_THREAD_PROPERTIES_SET)
    {
        PhSetThreadName(NtCurrentThread(), ThreadName);
        PhSetThreadBasePriority(NtCurrentThread(), Priority);
        PhTlsSetValue(KphpCommsTlsSlot, KPH_COMMS_THREAD_PROPERTIES_SET);
    }
}

/**
 * \brief Unhandled communications callback.
 *
 * \details Synchronous messages expecting a reply must always be handled, this
 * no-operation default callback will handle them as necessary. Used when no
 * callback is provided when connecting. Or when the callback does not handle
 * the message.
 *
 * \param[in] ReplyToken - Token used to reply, when possible.
 * \param[in] Message - Message from KPH to handle.
 */
VOID KphpCommsCallbackUnhandled(
    _In_ ULONG_PTR ReplyToken,
    _In_ PCKPH_MESSAGE Message
    )
{
    PPH_FREE_LIST freelist;
    PKPH_MESSAGE msg;

    if (!ReplyToken)
        return;

    freelist = KphGetMessageFreeList();

    msg = PhAllocateFromFreeList(freelist);
    KphMsgInit(msg, KphMsgUnhandled);
    KphCommsReplyMessage(ReplyToken, msg);

    PhFreeToFreeList(freelist, msg);
}

/**
 * \brief I/O thread pool callback for filter port.
 *
 * \param[in,out] Instance Unused
 * \param[in,out] Context Unused
 * \param[in,out] ApcContext Pointer to overlapped I/O that was completed.
 * \param[in] IoSB Result of the asynchronous I/O operation.
 * \param[in,out] Io Unused
 */
_Function_class_(TP_IO_CALLBACK)
VOID WINAPI KphpCommsIoCallback(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Context,
    _In_ PVOID ApcContext,
    _In_ PIO_STATUS_BLOCK IoSB,
    _In_ PTP_IO Io
    )
{
    NTSTATUS status;
    PKPH_UMESSAGE msg;
    SIZE_T length;

    if (!PhAcquireRundownProtection(&KphpCommsRundown))
        return;

    KphpCommsSetThreadProperties(L"Message Processor", THREAD_PRIORITY_HIGHEST);

    msg = CONTAINING_RECORD(ApcContext, KPH_UMESSAGE, Overlapped);

    if (IoSB->Status != STATUS_SUCCESS)
        goto QueueIoOperation;

    assert(IoSB->Information >= UFIELD_OFFSET(KPH_UMESSAGE, Message));

    if (IoSB->Information < UFIELD_OFFSET(KPH_UMESSAGE, Message))
        goto QueueIoOperation;

    length = IoSB->Information - UFIELD_OFFSET(KPH_UMESSAGE, Message);

    assert(length >= KPH_MESSAGE_MIN_SIZE);

    if (length < KPH_MESSAGE_MIN_SIZE)
        goto QueueIoOperation;

    if (!NT_SUCCESS(status = KphMsgValidate(&msg->Message)))
    {
        assert(NT_SUCCESS(status));
        goto QueueIoOperation;
    }

    if (msg->MessageHeader.ReplyLength)
    {
        BOOLEAN handled;
        ULONG_PTR replyToken;

        replyToken = (ULONG_PTR)&msg->MessageHeader;

        assert(length == msg->Message.Header.Size);

        if (KphpCommsRegisteredCallback)
            handled = KphpCommsRegisteredCallback(replyToken, &msg->Message);
        else
            handled = FALSE;

        if (!handled)
            KphpCommsCallbackUnhandled(replyToken, &msg->Message);
    }
    else if (KphpCommsRegisteredCallback)
    {
        for (ULONG offset = 0; offset < length; NOTHING)
        {
            PKPH_MESSAGE message;

            assert(offset < sizeof(KPH_MESSAGE));

            message = PTR_ADD_OFFSET(&msg->Message, offset);

            KphpCommsRegisteredCallback(0, message);

            offset += message->Header.Size;
        }
    }

QueueIoOperation:

    RtlZeroMemory(&msg->Overlapped, sizeof(OVERLAPPED));

    assert(Io == KphpCommsThreadPoolIo);

    TpStartAsyncIoOperation(KphpCommsThreadPoolIo);

    status = PhFilterGetMessage(
        KphpCommsFltPortHandle,
        &msg->MessageHeader,
        FIELD_OFFSET(KPH_UMESSAGE, Overlapped),
        &msg->Overlapped
        );

    if (status != STATUS_PENDING)
    {
        assert(status == STATUS_PORT_DISCONNECTED);

        if (status == STATUS_PORT_DISCONNECTED)
        {
            //
            // Mark the port disconnected so KphCommsIsConnected returns false.
            // This can happen if the driver goes away before the client.
            //
            KphpCommsPortDisconnected = TRUE;
        }

        TpCancelAsyncIoOperation(KphpCommsThreadPoolIo);
    }

    PhReleaseRundownProtection(&KphpCommsRundown);
}

_Function_class_(KPH_RING_CALLBACK)
BOOLEAN NTAPI KphpRingBufferCallback(
    _In_opt_ PVOID Context,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    NTSTATUS status;
    PKPH_MESSAGE msg;

    if (!PhAcquireRundownProtection(&KphpCommsRundown))
    {
        return TRUE;
    }

    if (!Length)
    {
        if (KphpCommsRingBuffer.EventHandle)
            PhWaitForSingleObject(KphpCommsRingBuffer.EventHandle, 1000);
        else
            PhDelayExecution(300);
        goto Exit;
    }

    if (!KphpCommsRegisteredCallback || Length < KPH_MESSAGE_MIN_SIZE)
    {
        goto Exit;
    }

    msg = Buffer;

    status = KphMsgValidate(msg);
    if (!NT_SUCCESS(status))
    {
        assert(FALSE);
        goto Exit;
    }

    KphpCommsRegisteredCallback(0, msg);

Exit:

    PhReleaseRundownProtection(&KphpCommsRundown);

    return FALSE;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS NTAPI KphpRingBufferProcessor(
    _In_ PVOID Context
    )
{
    KphpCommsSetThreadProperties(L"Message Ring Processor", THREAD_PRIORITY_ABOVE_NORMAL);
    KphProcessRingBuffer(&KphpCommsRingBuffer.Ring, KphpRingBufferCallback, NULL);
    return STATUS_SUCCESS;
}

static VOID KphpTpSetPoolThreadBasePriority(
    _Inout_ PTP_POOL Pool,
    _In_ ULONG BasePriority
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static NTSTATUS (NTAPI* TpSetPoolThreadBasePriority_I)(
        _Inout_ PTP_POOL Pool,
        _In_ ULONG BasePriority
        ) = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhGetLoaderEntryDllBaseZ(RtlNtdllName))
        {
            TpSetPoolThreadBasePriority_I = PhGetDllBaseProcedureAddress(baseAddress, "TpSetPoolThreadBasePriority", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (TpSetPoolThreadBasePriority_I)
    {
        TpSetPoolThreadBasePriority_I(Pool, BasePriority);
    }
}

/**
 * \brief Starts the communications infrastructure.
 *
 * \param[in] PortName Communication port name.
 * \param[in] Callback Communication callback for receiving (and replying to)
 * messages from the driver.
 * \param[in] RingBufferLength Size of the ring buffer to use for messages.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphCommsStart(
    _In_ PCPH_STRINGREF PortName,
    _In_opt_ PKPH_COMMS_CALLBACK Callback,
    _In_ ULONG RingBufferLength
    )
{
    NTSTATUS status;
    ULONG numberOfThreads;
    PVOID connectionContext;
    PVOID connectionContextPointer;
    USHORT connectionContextSize;

    if (KphpCommsFltPortHandle)
    {
        status = STATUS_ALREADY_INITIALIZED;
        goto CleanupExit;
    }

    if (RingBufferLength)
    {
        //NtCreateEvent(&KphpCommsRingBuffer.EventHandle, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE);
        KphpCommsRingBuffer.Length = RingBufferLength;
        connectionContext = &KphpCommsRingBuffer;
        connectionContextPointer = &connectionContext;
        connectionContextSize = sizeof(PVOID);
    }
    else
    {
        connectionContextPointer = NULL;
        connectionContextSize = 0;
    }

    if (!NT_SUCCESS(status = PhFilterConnectCommunicationPort(
        PortName,
        0,
        connectionContextPointer,
        connectionContextSize,
        NULL,
        &KphpCommsFltPortHandle
        )))
        goto CleanupExit;

    KphpCommsPortDisconnected = FALSE;
    KphpCommsRegisteredCallback = Callback;

    PhInitializeRundownProtection(&KphpCommsRundown);
    PhInitializeFreeList(&KphpCommsReplyFreeList, sizeof(KPH_UREPLY), 16);

    KphpCommsTlsSlot = PhTlsAlloc();
    if (KphpCommsTlsSlot == TLS_OUT_OF_INDEXES)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto CleanupExit;
    }

    if (RingBufferLength)
    {
        if (!NT_SUCCESS(status = PhCreateThreadEx(&KphpCommsRingBufferThread, KphpRingBufferProcessor, NULL)))
            goto CleanupExit;
    }

    if (PhSystemProcessorInformation.NumberOfProcessors >= KPH_COMMS_MIN_THREADS)
        numberOfThreads = PhSystemProcessorInformation.NumberOfProcessors * KPH_COMMS_THREAD_SCALE;
    else
        numberOfThreads = KPH_COMMS_MIN_THREADS;

    KphpCommsMessageCount = numberOfThreads * KPH_COMMS_MESSAGE_SCALE;
    if (KphpCommsMessageCount > KPH_COMMS_MAX_MESSAGES)
        KphpCommsMessageCount = KPH_COMMS_MAX_MESSAGES;

    if (!NT_SUCCESS(status = TpAllocPool(&KphpCommsThreadPool, NULL)))
        goto CleanupExit;

    TpSetPoolMinThreads(KphpCommsThreadPool, KPH_COMMS_MIN_THREADS);
    TpSetPoolMaxThreads(KphpCommsThreadPool, numberOfThreads);
    KphpTpSetPoolThreadBasePriority(KphpCommsThreadPool, THREAD_PRIORITY_HIGHEST);

    TpInitializeCallbackEnviron(&KphpCommsThreadPoolEnv);
    TpSetCallbackNoActivationContext(&KphpCommsThreadPoolEnv);
    TpSetCallbackPriority(&KphpCommsThreadPoolEnv, TP_CALLBACK_PRIORITY_HIGH);
    TpSetCallbackThreadpool(&KphpCommsThreadPoolEnv, KphpCommsThreadPool);
    //TpSetCallbackLongFunction(&KphpCommsThreadPoolEnv);

    PhSetFileCompletionNotificationMode(KphpCommsFltPortHandle, FILE_SKIP_SET_EVENT_ON_HANDLE);

    if (!NT_SUCCESS(status = TpAllocIoCompletion(
        &KphpCommsThreadPoolIo,
        KphpCommsFltPortHandle,
        KphpCommsIoCallback,
        NULL,
        &KphpCommsThreadPoolEnv
        )))
        goto CleanupExit;

    KphpCommsMessages = PhAllocateZeroSafe(sizeof(KPH_UMESSAGE) * KphpCommsMessageCount);
    if (!KphpCommsMessages)
    {
        status = STATUS_NO_MEMORY;
        goto CleanupExit;
    }

    for (ULONG i = 0; i < KphpCommsMessageCount; i++)
    {
        RtlZeroMemory(&KphpCommsMessages[i].Overlapped, sizeof(OVERLAPPED));

        TpStartAsyncIoOperation(KphpCommsThreadPoolIo);

        status = PhFilterGetMessage(
            KphpCommsFltPortHandle,
            &KphpCommsMessages[i].MessageHeader,
            FIELD_OFFSET(KPH_UMESSAGE, Overlapped),
            &KphpCommsMessages[i].Overlapped
            );
        if (status == STATUS_PENDING)
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_FLT_INTERNAL_ERROR;
            goto CleanupExit;
        }
    }

    status = STATUS_SUCCESS;

CleanupExit:

    if (!NT_SUCCESS(status))
        KphCommsStop();

    return status;
}

/**
 * \brief Stops the communications infrastructure.
 */
VOID KphCommsStop(
    VOID
    )
{
    if (!KphpCommsFltPortHandle)
        return;

    PhWaitForRundownProtection(&KphpCommsRundown);

    if (KphpCommsThreadPoolIo)
    {
        TpWaitForIoCompletion(KphpCommsThreadPoolIo, TRUE);
        TpReleaseIoCompletion(KphpCommsThreadPoolIo);
        KphpCommsThreadPoolIo = NULL;
    }

    TpDestroyCallbackEnviron(&KphpCommsThreadPoolEnv);

    if (KphpCommsThreadPool)
    {
        TpReleasePool(KphpCommsThreadPool);
        KphpCommsThreadPool = NULL;
    }

    if (KphpCommsRingBufferThread)
    {
        if (KphpCommsRingBuffer.EventHandle)
            NtSetEvent(KphpCommsRingBuffer.EventHandle, NULL);

        NtWaitForSingleObject(KphpCommsRingBufferThread, FALSE, NULL);
        NtClose(KphpCommsRingBufferThread);
        KphpCommsRingBufferThread = NULL;

        if (KphpCommsRingBuffer.Ring.Producer)
        {
            NtUnmapViewOfSection(NtCurrentProcess(), KphpCommsRingBuffer.Ring.Producer);
            KphpCommsRingBuffer.Ring.Producer = NULL;
        }

        if (KphpCommsRingBuffer.Ring.Consumer)
        {
            NtUnmapViewOfSection(NtCurrentProcess(), KphpCommsRingBuffer.Ring.Consumer);
            KphpCommsRingBuffer.Ring.Consumer = NULL;
        }

        if (KphpCommsRingBuffer.EventHandle)
        {
            NtClose(KphpCommsRingBuffer.EventHandle);
            KphpCommsRingBuffer.EventHandle = NULL;
        }
    }

    KphpCommsRegisteredCallback = NULL;
    KphpCommsPortDisconnected = TRUE;

    NtClose(KphpCommsFltPortHandle);
    KphpCommsFltPortHandle = NULL;

    if (KphpCommsMessages)
    {
        KphpCommsMessageCount = 0;

        PhFree(KphpCommsMessages);
        KphpCommsMessages = NULL;
    }

    if (KphpCommsTlsSlot != TLS_OUT_OF_INDEXES)
    {
        PhTlsFree(KphpCommsTlsSlot);
        KphpCommsTlsSlot = TLS_OUT_OF_INDEXES;
    }

    PhDeleteFreeList(&KphpCommsReplyFreeList);
}

/**
 * \brief Checks if communications is connected to the driver.
 *
 * \return TRUE if connected, FALSE otherwise.
 */
BOOLEAN KphCommsIsConnected(
    VOID
    )
{
    return (KphpCommsFltPortHandle && !KphpCommsPortDisconnected);
}

/**
 * \brief Replies to a message send to the communications callback.
 *
 * \param[in] ReplyToken Reply token from the communications callback.
 * \param[in] Message The message to reply with.
 *
 * \return Successful or errant status.
 */
NTSTATUS KphCommsReplyMessage(
    _In_ ULONG_PTR ReplyToken,
    _In_ PKPH_MESSAGE Message
    )
{
    NTSTATUS status;
    PKPH_UREPLY reply;
    PFILTER_MESSAGE_HEADER header;

    reply = NULL;
    header = (PFILTER_MESSAGE_HEADER)ReplyToken;

    if (!KphpCommsFltPortHandle)
    {
        status = STATUS_FLT_NOT_INITIALIZED;
        goto CleanupExit;
    }

    if (!header || !header->ReplyLength)
    {
        //
        // Kernel is not expecting a reply.
        //
        status = STATUS_INVALID_PARAMETER_1;
        goto CleanupExit;
    }

    if (!NT_SUCCESS(status = KphMsgValidate(Message)))
        goto CleanupExit;

    if (!(reply = PhAllocateFromFreeList(&KphpCommsReplyFreeList)))
    {
        status = STATUS_NO_MEMORY;
        goto CleanupExit;
    }

    RtlZeroMemory(reply, sizeof(KPH_UREPLY));
    RtlCopyMemory(&reply->ReplyHeader, header, sizeof(reply->ReplyHeader));
    RtlCopyMemory(&reply->Message, Message, Message->Header.Size);

    status = PhFilterReplyMessage(
        KphpCommsFltPortHandle,
        &reply->ReplyHeader,
        sizeof(reply->ReplyHeader) + Message->Header.Size
        );

    if (status == STATUS_PORT_DISCONNECTED)
    {
        //
        // Mark the port disconnected so KphCommsIsConnected returns false.
        // This can happen if the driver goes away before the client.
        //
        KphpCommsPortDisconnected = TRUE;
    }

CleanupExit:

    if (reply)
        PhFreeToFreeList(&KphpCommsReplyFreeList, reply);

    return status;
}

/**
 * \brief Sends a message to the driver (and may receive output if applicable).
 *
 * \param[in,out] Message The message to send to driver, on success this message
 * contains the output from the driver.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphCommsSendMessage(
    _Inout_ PKPH_MESSAGE Message
    )
{
    NTSTATUS status;
    ULONG bytesReturned;

    if (!KphpCommsFltPortHandle)
        return STATUS_FLT_NOT_INITIALIZED;

    if (!NT_SUCCESS(status = KphMsgValidate(Message)))
        return status;

    status = PhFilterSendMessage(
        KphpCommsFltPortHandle,
        Message,
        sizeof(KPH_MESSAGE),
        NULL,
        0,
        &bytesReturned
        );

    if (status == STATUS_PORT_DISCONNECTED)
    {
        //
        // Mark the port disconnected so KphCommsIsConnected returns false.
        // This can happen if the driver goes away before the client.
        //
        KphpCommsPortDisconnected = TRUE;
    }

    return status;
}
