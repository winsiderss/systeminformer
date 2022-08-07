#include <kphcomms.h>
#include <settings.h>
#include <fltUser.h>
#include <threadpoolapiset.h>

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
PH_RUNDOWN_PROTECT KphpCommsRundown;

#define KPH_COMMS_MIN_THREADS   2
#define KPH_COMMS_MESSAGE_SCALE 2
#define KPH_COMMS_THREAD_SCALE  2
#define KPH_COMMS_MAX_MESSAGES  1024

typedef struct _FILTER_PORT_EA
{
    PUNICODE_STRING PortName;
    PVOID Unknown;
    USHORT SizeOfContext;
    BYTE Padding[6];
    BYTE ConnectionContext[ANYSIZE_ARRAY];

} FILTER_PORT_EA, *PFILTER_PORT_EA;

#define KPI_FILTER_EA_NAME "FLTPORT"

#define FLT_CTL_SEND_MESSAGE CTL_CODE(FILE_DEVICE_DISK_FILE_SYSTEM, 6, METHOD_NEITHER, FILE_WRITE_ACCESS)
#define FLT_CTL_GET_MESSAGE CTL_CODE(FILE_DEVICE_DISK_FILE_SYSTEM, 7, METHOD_NEITHER, FILE_READ_ACCESS)
#define FLT_CTL_REPLY_MESSAGE CTL_CODE(FILE_DEVICE_DISK_FILE_SYSTEM, 8, METHOD_NEITHER, FILE_WRITE_ACCESS)

/**
 * \brief Wrapper which is essentially FilterpDeviceIoControl.
 * 
 * \param[in] Handle Filter port handle.
 * \param[in] IoControlCode Filter I/O control code
 * \param[in] InBuffer Input buffer.
 * \param[in] InBufferSize Input buffer size.
 * \param[out] OutputBuffer Output Buffer.
 * \param[in] OutputBufferSize Output buffer size.
 * \param[out] BytesReturned Optionally set to the number of bytes returned.
 * \param[in,out] Overlapped Optional overlapped structure.
 * 
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphpFilterDeviceIoControl(
    _In_ HANDLE Handle,
    _In_ ULONG IoControlCode,
    _In_reads_bytes_(InBufferSize) PVOID InBuffer,
    _In_ ULONG InBufferSize,
    _Out_writes_bytes_to_opt_(OutBufferSize, *BytesReturned) PVOID OutBuffer,
    _In_ ULONG OutBufferSize,
    _Out_opt_ PULONG BytesReturned,
    _Inout_opt_ LPOVERLAPPED Overlapped
    )
{
    NTSTATUS status;

    if (BytesReturned)
    {
        *BytesReturned = 0;
    }

    if (Overlapped)
    {
        Overlapped->Internal = STATUS_PENDING;

        if (DEVICE_TYPE_FROM_CTL_CODE(IoControlCode) == FILE_DEVICE_FILE_SYSTEM)
        {
            status = NtFsControlFile(Handle,
                                     Overlapped->hEvent,
                                     NULL,
                                     Overlapped,
                                     (PIO_STATUS_BLOCK)Overlapped,
                                     IoControlCode,
                                     InBuffer,
                                     InBufferSize,
                                     OutBuffer,
                                     OutBufferSize);
        }
        else
        {
            status = NtDeviceIoControlFile(Handle,
                                           Overlapped->hEvent,
                                           NULL,
                                           Overlapped,
                                           (PIO_STATUS_BLOCK)Overlapped,
                                           IoControlCode,
                                           InBuffer,
                                           InBufferSize,
                                           OutBuffer,
                                           OutBufferSize);
        }

        if (NT_INFORMATION(status) && BytesReturned)
        {
            *BytesReturned = (ULONG)Overlapped->InternalHigh;
        }
    }
    else
    {
        IO_STATUS_BLOCK ioStatusBlock;

        if (DEVICE_TYPE_FROM_CTL_CODE(IoControlCode) == FILE_DEVICE_FILE_SYSTEM)
        {
            status = NtFsControlFile(Handle,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &ioStatusBlock,
                                     IoControlCode,
                                     InBuffer,
                                     InBufferSize,
                                     OutBuffer,
                                     OutBufferSize);
        }
        else
        {
            status = NtDeviceIoControlFile(Handle,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &ioStatusBlock,
                                           IoControlCode,
                                           InBuffer,
                                           InBufferSize,
                                           OutBuffer,
                                           OutBufferSize);
        }

        if (status == STATUS_PENDING)
        {
            status = NtWaitForSingleObject(Handle, FALSE, NULL);
            if (NT_SUCCESS(status))
            {
                status = ioStatusBlock.Status;
            }
        }

        if (BytesReturned)
        {
            *BytesReturned = (ULONG)ioStatusBlock.Information;
        }
    }

    return status;
}

/**
 * \brief Wrapper which is essentially FilterSendMessage.
 * 
 * \param[in] Port Filter port handle.
 * \param[in] InBuffer Input buffer.
 * \param[in] InBufferSize Input buffer size.
 * \param[out] OutputBuffer Output Buffer.
 * \param[in] OutputBufferSize Output buffer size.
 * \param[out] BytesReturned Set to the number of bytes returned.
 * 
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphpFilterSendMessage(
    _In_ HANDLE Port,
    _In_reads_bytes_(InBufferSize) PVOID InBuffer,
    _In_ ULONG InBufferSize,
    _Out_writes_bytes_to_opt_(OutBufferSize,*BytesReturned) PVOID OutBuffer,
    _In_ ULONG OutBufferSize,
    _Out_ PULONG BytesReturned
    )
{
    return KphpFilterDeviceIoControl(Port,
                                     FLT_CTL_SEND_MESSAGE,
                                     InBuffer,
                                     InBufferSize,
                                     OutBuffer,
                                     OutBufferSize,
                                     BytesReturned,
                                     NULL);
}

/**
 * \brief Wrapper which is essentially FilterGetMessage.
 * 
 * \param[in] Port Filter port handle.
 * \param[out] MessageBuffer Message buffer.
 * \param[in] MessageBufferSize Message buffer size.
 * \param[in] Overlapped Th overlapped structure.
 * 
 * \return Successful or errant status. 
 */
_Must_inspect_result_
NTSTATUS KphpFilterGetMessage(
    _In_ HANDLE Port,
    _Out_writes_bytes_(MessageBufferSize) PFILTER_MESSAGE_HEADER MessageBuffer,
    _In_ ULONG MessageBufferSize,
    _Inout_ LPOVERLAPPED Overlapped
    )
{
    return KphpFilterDeviceIoControl(Port,
                                     FLT_CTL_GET_MESSAGE,
                                     NULL,
                                     0,
                                     MessageBuffer,
                                     MessageBufferSize,
                                     NULL,
                                     Overlapped);
}

/**
 * \brief Wrapper which is essentially FilterReplyMessage.
 * 
 * \param[in] Port Filter port handle.
 * \param[in] ReplyBuffer Reply buffer.
 * \param[in] ReplyBufferSize Reply buffer size.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphpFilterReplyMessage(
    _In_ HANDLE Port,
    _In_reads_bytes_(ReplyBufferSize) PFILTER_REPLY_HEADER ReplyBuffer,
    _In_ ULONG ReplyBufferSize
    )
{
    return KphpFilterDeviceIoControl(Port,
                                     FLT_CTL_REPLY_MESSAGE,
                                     ReplyBuffer,
                                     ReplyBufferSize,
                                     NULL,
                                     0,
                                     NULL,
                                     NULL);
}

/**
 * \brief Wrapper which is essentially FilterConnectCommunicationPort.
 * 
 * \param[in] PortName Name of filter port to connect to.
 * \param[in] Options Filter port options.
 * \param[in] ConnectionContext Connection context.
 * \param[in] SizeOfContext Size of connection context.
 * \param[in] SecurityAttributes Security attributes for handle.
 * \param[out] Port Set to filter port handle on success, null on failure.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphpFilterConnectCommunicationPort(
    _In_ LPCWSTR PortName,
    _In_ ULONG Options,
    _In_reads_bytes_opt_(SizeOfContext) LPCVOID ConnectionContext,
    _In_ USHORT SizeOfContext,
    _In_opt_ LPSECURITY_ATTRIBUTES SecurityAttributes,
    _Outptr_ HANDLE *Port
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING fltMgr;
    UNICODE_STRING portName;
    ULONG eaLen;
    PFILE_FULL_EA_INFORMATION ea;
    PFILTER_PORT_EA eaValue;
    IO_STATUS_BLOCK iosb;
    ULONG createOptions;

    *Port = NULL;

    if (((SizeOfContext > 0) && !ConnectionContext) ||
        ((SizeOfContext == 0) && ConnectionContext))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Build the filter EA, this contains the port name and the context.
    //

    eaLen = (sizeof(FILE_FULL_EA_INFORMATION)
             + sizeof(FILTER_PORT_EA)
             + ARRAYSIZE(KPI_FILTER_EA_NAME)
             + SizeOfContext);
    ea = (PFILE_FULL_EA_INFORMATION)PhAllocateZeroSafe(eaLen);
    if (!ea)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ea->NextEntryOffset = 0;
    ea->Flags = 0;
    ea->EaNameLength = (ARRAYSIZE(KPI_FILTER_EA_NAME) - 1);
    RtlCopyMemory(ea->EaName, KPI_FILTER_EA_NAME, ARRAYSIZE(KPI_FILTER_EA_NAME));
    ea->EaValueLength = (sizeof(FILTER_PORT_EA) + SizeOfContext);
    eaValue = (PFILTER_PORT_EA)PTR_ADD_OFFSET(ea->EaName, ea->EaNameLength + 1);
    RtlInitUnicodeString(&portName, PortName);
    eaValue->PortName = &portName;
    eaValue->Unknown = NULL;
    eaValue->SizeOfContext = SizeOfContext;
    if (SizeOfContext > 0)
    {
        RtlCopyMemory(eaValue->ConnectionContext,
                      ConnectionContext,
                      SizeOfContext);
    }

    RtlInitUnicodeString(&fltMgr, L"\\Global??\\FltMgrMsg");
    InitializeObjectAttributes(&oa,
                               &fltMgr,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    if (SecurityAttributes)
    {
        if (SecurityAttributes->bInheritHandle)
        {
            oa.Attributes |= OBJ_INHERIT;
        }
        oa.SecurityDescriptor = SecurityAttributes->lpSecurityDescriptor;
    }

    RtlZeroMemory(&iosb, sizeof(iosb));

    createOptions = 0;
    if (Options & FLT_PORT_FLAG_SYNC_HANDLE)
    {
        createOptions = FILE_SYNCHRONOUS_IO_NONALERT;
    }

    status = NtCreateFile(Port,
                          FILE_READ_ACCESS | FILE_WRITE_ACCESS | SYNCHRONIZE,
                          &oa,
                          &iosb,
                          NULL,
                          0,
                          0,
                          FILE_OPEN_IF,
                          createOptions, 
                          ea,
                          eaLen);

    PhFree(ea);

    return status;
}

/**
 * \brief Retrieves the message port name.
 * 
 * @return Message port name string. 
 */
PPH_STRING KphCommGetMessagePortName(
    VOID
    )
{
    PPH_STRING portName;

    portName = PhGetStringSetting(L"KphPortName");
    if (PhIsNullOrEmptyString(portName))
    {
        if (portName)
        {
            PhDereferenceObject(portName);
        }

        portName = PhCreateString(L"\\KSystemInformer");
    }

    return portName;
}

/**
 * \brief I/O thread pool callback for filter port.
 * 
 * \param[in,out] Instance Unused 
 * \param[in,out] Context Unused
 * \param[in,out] Overlapped Pointer to overlapped I/O that was completed.
 * \param[in] IoResult Result of the asynchronous I/O operation.
 * \param[in] NumberOfBytesTransferred Number of bytes transferred from the
 * I/O completion.
 * \param[in,out] Io Unused
 */
VOID WINAPI KphpCommsIoCallback(
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Context,
    _Inout_opt_ PVOID Overlapped,
    _In_ ULONG IoResult,
    _In_ ULONG_PTR NumberOfBytesTransferred,
    _Inout_ PTP_IO Io
    )
{
    NTSTATUS status;
    PKPH_UMESSAGE msg;

    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Io);

    if (!PhAcquireRundownProtection(&KphpCommsRundown))
    {
        return;
    }

    msg = CONTAINING_RECORD(Overlapped, KPH_UMESSAGE, Overlapped);

    if (IoResult != ERROR_SUCCESS)
    {
        goto Exit;
    }

    if (NumberOfBytesTransferred < KPH_MESSAGE_MIN_SIZE)
    {
        assert(FALSE);
        goto Exit;
    }

    status = KphMsgValidate(&msg->Message);
    if (!NT_SUCCESS(status))
    {
        assert(FALSE);
        goto Exit;
    }

    if (KphpCommsRegisteredCallback)
    {
        KphpCommsRegisteredCallback((ULONG_PTR)&msg->MessageHeader,
                                    &msg->Message);
    }

Exit:

    RtlZeroMemory(&msg->Overlapped, FIELD_OFFSET(OVERLAPPED, hEvent));

    StartThreadpoolIo(KphpCommsThreadPoolIo);

    status = KphpFilterGetMessage(KphpCommsFltPortHandle,
                                  &msg->MessageHeader,
                                  FIELD_OFFSET(KPH_UMESSAGE, Overlapped),
                                  &msg->Overlapped);

    if (status != STATUS_PENDING)
    {
        if (status == STATUS_PORT_DISCONNECTED)
        {
            //
            // Mark the port disconnected so KphCommsIsConnected returns false.
            // This can happen if the driver goes away before the client.
            //
            KphpCommsPortDisconnected = TRUE;
        }
        else
        {
            assert(FALSE);
        }

        CancelThreadpoolIo(KphpCommsThreadPoolIo);
    }

    PhReleaseRundownProtection(&KphpCommsRundown);
}

/**
 * \brief Starts the communications infrastructure.
 * 
 * \param[in] PortName Communication port name.
 * \param[in] Callback Communication callback for receiving (and replying to)
 * messages from the driver.
 *
 * \return Successful or errant status.
 */
_Must_inspect_result_
NTSTATUS KphCommsStart(
    _In_ PPH_STRING PortName,
    _In_opt_ PKPH_COMMS_CALLBACK Callback
    )
{
    NTSTATUS status;
    ULONG numberOfThreads;
    ULONG numberOfMessages;
    SYSTEM_BASIC_INFORMATION sysInfo;

    if (KphpCommsFltPortHandle)
    {
        status = STATUS_ALREADY_INITIALIZED;
        goto Exit;
    }

    status = KphpFilterConnectCommunicationPort(PhGetString(PortName),
                                                0,
                                                NULL,
                                                0,
                                                NULL,
                                                &KphpCommsFltPortHandle);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    KphpCommsPortDisconnected = FALSE;
    KphpCommsRegisteredCallback = Callback;

    PhInitializeRundownProtection(&KphpCommsRundown);

    status = ZwQuerySystemInformation(SystemBasicInformation,
                                      &sysInfo,
                                      sizeof(sysInfo),
                                      NULL);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    if (sysInfo.NumberOfProcessors < KPH_COMMS_MIN_THREADS)
    {
        numberOfThreads = KPH_COMMS_MIN_THREADS;
    }
    else
    {
        numberOfThreads = (sysInfo.NumberOfProcessors * KPH_COMMS_THREAD_SCALE);
    }

    numberOfMessages = numberOfThreads * KPH_COMMS_MESSAGE_SCALE;
    if (numberOfMessages > KPH_COMMS_MAX_MESSAGES)
    {
        numberOfMessages = KPH_COMMS_MAX_MESSAGES;
    }

    KphpCommsThreadPool = CreateThreadpool(NULL);
    if (!KphpCommsThreadPool)
    {
        status = PhGetLastWin32ErrorAsNtStatus();
        goto Exit;
    }

    InitializeThreadpoolEnvironment(&KphpCommsThreadPoolEnv);

    SetThreadpoolThreadMinimum(KphpCommsThreadPool, KPH_COMMS_MIN_THREADS);
    SetThreadpoolThreadMaximum(KphpCommsThreadPool, numberOfThreads);

    SetThreadpoolCallbackPool(&KphpCommsThreadPoolEnv, KphpCommsThreadPool);
    //SetThreadpoolCallbackRunsLong(&PiCommsThreadPoolEnv);

    SetFileCompletionNotificationModes(KphpCommsFltPortHandle,
                                       FILE_SKIP_SET_EVENT_ON_HANDLE);

    KphpCommsThreadPoolIo = CreateThreadpoolIo(KphpCommsFltPortHandle,
                                               KphpCommsIoCallback,
                                               NULL,
                                               &KphpCommsThreadPoolEnv);
    if (!KphpCommsThreadPoolIo)
    {
        status = PhGetLastWin32ErrorAsNtStatus();
        goto Exit;
    }

    KphpCommsMessages = PhAllocateExSafe(sizeof(KPH_UMESSAGE) * numberOfMessages,
                                         HEAP_ZERO_MEMORY);
    if (!KphpCommsMessages)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    for (ULONG i = 0; i < numberOfMessages; i++)
    {
        status = NtCreateEvent(&KphpCommsMessages[i].Overlapped.hEvent,
                               EVENT_ALL_ACCESS,
                               NULL,
                               NotificationEvent,
                               FALSE);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }

        RtlZeroMemory(&KphpCommsMessages[i].Overlapped,
                      FIELD_OFFSET(OVERLAPPED, hEvent));

        StartThreadpoolIo(KphpCommsThreadPoolIo);

        status = KphpFilterGetMessage(KphpCommsFltPortHandle,
                                      &KphpCommsMessages[i].MessageHeader,
                                      FIELD_OFFSET(KPH_UMESSAGE, Overlapped),
                                      &KphpCommsMessages[i].Overlapped);
        if (status == STATUS_PENDING)
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_FLT_INTERNAL_ERROR;
            goto Exit;
        }
    }

    status = STATUS_SUCCESS;

Exit:

    if (!NT_SUCCESS(status))
    {
        KphCommsStop();
    }

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
    {
        return;
    }

    PhWaitForRundownProtection(&KphpCommsRundown);

    if (KphpCommsThreadPoolIo)
    {
        WaitForThreadpoolIoCallbacks(KphpCommsThreadPoolIo, TRUE);
        CloseThreadpoolIo(KphpCommsThreadPoolIo);
        KphpCommsThreadPoolIo = NULL;
    }

    DestroyThreadpoolEnvironment(&KphpCommsThreadPoolEnv);

    if (KphpCommsThreadPool)
    {
        CloseThreadpool(KphpCommsThreadPool);
        KphpCommsThreadPool = NULL;
    }

    KphpCommsRegisteredCallback = NULL;
    KphpCommsPortDisconnected = TRUE;

    if (KphpCommsFltPortHandle)
    {
        NtClose(KphpCommsFltPortHandle);
        KphpCommsFltPortHandle = NULL;
    }

    if (KphpCommsMessages)
    {
        PhFree(KphpCommsMessages);
        KphpCommsMessages = NULL;
    }
}

/**
 * \brief Checks if communications is connected to the driver.
 * 
 * @return TRUE if connected, FALSE otherwise. 
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
    header = (PFILTER_MESSAGE_HEADER)(ReplyToken);

    if (!KphpCommsFltPortHandle)
    {
        status = STATUS_FLT_NOT_INITIALIZED;
        goto Exit;
    }

    if (!header || (header->ReplyLength == 0))
    {
        //
        // Kernel is not expecting a reply.
        //
        status = STATUS_INVALID_PARAMETER_1;
        goto Exit;
    }

    status = KphMsgValidate(Message);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    reply = PhAllocateSafe(sizeof(KPH_UREPLY));
    if (!reply)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    RtlCopyMemory(&reply->ReplyHeader, header, sizeof(reply->ReplyHeader));
    RtlCopyMemory(&reply->Message, Message, Message->Header.Size);

    status = KphpFilterReplyMessage(
                           KphpCommsFltPortHandle,
                           &reply->ReplyHeader,
                           sizeof(reply->ReplyHeader) + Message->Header.Size);

    if (status == STATUS_PORT_DISCONNECTED)
    {
        //
        // Mark the port disconnected so KphCommsIsConnected returns false.
        // This can happen if the driver goes away before the client.
        //
        KphpCommsPortDisconnected = TRUE;
    }

Exit:

    if (reply)
    {
        PhFree(reply);
    }

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
    {
        return STATUS_FLT_NOT_INITIALIZED;
    }

    status = KphMsgValidate(Message);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = KphpFilterSendMessage(KphpCommsFltPortHandle,
                                   Message,
                                   sizeof(KPH_MESSAGE),
                                   NULL,
                                   0,
                                   &bytesReturned);

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
