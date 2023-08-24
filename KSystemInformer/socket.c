#include <kph.h>
#include <wsk.h>

#include <trace.h>

static WSK_REGISTRATION KphpWskRegistration;
static WSK_PROVIDER_NPI KphpWskProvider;
static WSK_CLIENT_DISPATCH KphpWskDispatch = { MAKE_WSK_VERSION(1, 0), 0, NULL };
static BOOLEAN KphpWskRegistered = FALSE;
static BOOLEAN KphpWskProviderCaptured = FALSE;

typedef struct _KPH_WSK_IO
{
    PIRP Irp;
    KEVENT Event;
} KPH_WSK_IO, *PKPH_WSK_IO;

typedef struct _KPH_SOCKET
{
    PWSK_SOCKET WskSocket;
    PWSK_PROVIDER_CONNECTION_DISPATCH WskDispatch;
    KPH_WSK_IO Io;
} KPH_SOCKET, *PKPH_SOCKET;

/**
 * \brief WSK I/O completion routine.
 *
 * \param[in] DeviceObject unused
 * \param[in] Irp unused
 * \param[in] Context Points to an event to signal. 
 *
 * \return STATUS_MORE_PROCESSING_REQUIRED
 */
_Function_class_(IO_COMPLETION_ROUTINE)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS KphpWskIoCompletionRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_opt_ PVOID Context
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    NPAGED_CODE_DISPATCH_MAX();
    NT_ASSERT(Context);

    KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

/**
 * \brief Initialize an WSK I/O object.
 *
 * \param[out] Io The WSK I/O object to initialize. Once initialized the data 
 * must be deleted with KphpWskIoDelete.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS KphpWskIoCreate(
    _Out_ PKPH_WSK_IO Io
    )
{
    NPAGED_CODE_DISPATCH_MAX();

    KeInitializeEvent(&Io->Event, NotificationEvent, FALSE);

    Io->Irp = IoAllocateIrp(1, FALSE);
    if (!Io->Irp)
    {
        KphTracePrint(TRACE_LEVEL_ERROR, SOCKET, "IoAllocateIrp failed");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoSetCompletionRoutine(Io->Irp,
                           &KphpWskIoCompletionRoutine,
                           &Io->Event,
                           TRUE,
                           TRUE,
                           TRUE);

    return STATUS_SUCCESS;
}

/**
 * \brief Resets an WSK I/O, preparing it to be reused for another request.
 *
 * \param[in,out] Io The WSK I/O object to reset.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphpWskIoReset(
    _Inout_ PKPH_WSK_IO Io
    )
{
    NPAGED_CODE_DISPATCH_MAX();

    KeResetEvent(&Io->Event);
    IoReuseIrp(Io->Irp, STATUS_UNSUCCESSFUL);
    IoSetCompletionRoutine(Io->Irp,
                           &KphpWskIoCompletionRoutine,
                           &Io->Event,
                           TRUE,
                           TRUE,
                           TRUE);
}

/**
 * \brief Deletes an WSK I/O object.
 *
 * \param[in] Io The WSK I/O object to delete.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphpWskIoDelete(
    _In_ PKPH_WSK_IO Io
    )
{
    NPAGED_CODE_DISPATCH_MAX();

    if (Io->Irp)
    {
        IoFreeIrp(Io->Irp);
    }
}

/**
 * \brief Waits for WSK I/O to complete.
 *
 * \param[in] Io The WSK I/O object to wait for.
 * \param[in] RequestStatus The status from the originating request.
 * \param[in] Timeout Optional timeout for the request to complete.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS KphpWskIoWaitForCompletion(
    _In_ PKPH_WSK_IO Io,
    _In_ NTSTATUS RequestStatus,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    NTSTATUS status;

    NPAGED_CODE_DISPATCH_MAX();

    if (RequestStatus != STATUS_PENDING)
    {
        return RequestStatus;
    }

    status = KeWaitForSingleObject(&Io->Event,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   Timeout);
    if (status == STATUS_TIMEOUT)
    {
        IoCancelIrp(Io->Irp);

        KeWaitForSingleObject(&Io->Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }
    else
    {
        NT_ASSERT(status == STATUS_SUCCESS);
    }

    return Io->Irp->IoStatus.Status;
}

/**
 * \brief Closes a socket object.
 *
 * \param[in] Socket A handle to the socket object to close.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphSocketClose(
    _In_freesMem_ KPH_SOCKET_HANDLE Socket
    )
{
    NTSTATUS status;
    PKPH_SOCKET socket;

    NPAGED_CODE_DISPATCH_MAX();

    socket = (PKPH_SOCKET)Socket;

    KphpWskIoReset(&socket->Io);

    status = socket->WskDispatch->WskCloseSocket(socket->WskSocket,
                                                 socket->Io.Irp);
    status = KphpWskIoWaitForCompletion(&socket->Io, status, NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "WskCloseSocket failed: %!STATUS!",
                      status);
    }

    KphpWskIoDelete(&socket->Io);
    KphFree(socket, KPH_TAG_SOCKET);
}

/**
 * \brief Creates a connection-oriented socket, binds it to a local address,
 * and connects it to a remote address.
 *
 * \param[in] SocktType The type of socket to create (e.g. SOCK_STREAM).
 * \param[in] Protocol The protocol to use (e.g. IPPROTO_TCP).
 * \param[in] LocalAddress The local address to bind to.
 * \param[in] RemoteAddress The remote address to connect to.
 * \param[in] Timeout Optional timeout for the connection to complete.
 * \param[out] Socket Set to a handle to the socket object on success.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS KphSocketConnect(
    _In_ USHORT SocketType,
    _In_ ULONG Protocol,
    _In_ PSOCKADDR LocalAddress,
    _In_ PSOCKADDR RemoteAddress,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Outptr_allocatesMem_ PKPH_SOCKET_HANDLE Socket
    )
{
    NTSTATUS status;
    PKPH_SOCKET socket;

    NPAGED_CODE_DISPATCH_MAX();

    *Socket = NULL;

    socket = KphAllocateNPaged(sizeof(KPH_SOCKET), KPH_TAG_SOCKET);
    if (!socket)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphpWskIoCreate(&socket->Io);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "KphpWskIoCreate failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpWskProvider.Dispatch->WskSocketConnect(KphpWskProvider.Client,
                                                        SocketType,
                                                        Protocol,
                                                        LocalAddress,
                                                        RemoteAddress,
                                                        WSK_FLAG_CONNECTION_SOCKET,
                                                        NULL,
                                                        NULL,
                                                        PsInitialSystemProcess,
                                                        NULL,
                                                        NULL,
                                                        socket->Io.Irp);
    status = KphpWskIoWaitForCompletion(&socket->Io, status, Timeout);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "WskSocketConnect failed: %!STATUS!",
                      status);

        goto Exit;
    }

    socket->WskSocket = (PWSK_SOCKET)socket->Io.Irp->IoStatus.Information;
    socket->WskDispatch = (PWSK_PROVIDER_CONNECTION_DISPATCH)socket->WskSocket->Dispatch;
    *Socket = (KPH_SOCKET_HANDLE)socket;
    socket = NULL;

Exit:

    if (socket)
    {
        KphpWskIoDelete(&socket->Io);
        KphFree(socket, KPH_TAG_SOCKET);
    }

    return status;
}

/**
 * \brief Sends data over a connection-oriented socket.
 *
 * \param[in] Socket A handle to the socket object to send data over.
 * \param[in] Timeout Optional timeout for the send to complete.
 * \param[in] Buffer The buffer containing the data to send.
 * \param[in] Length The length of the data to send.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS KphSocketSend(
    _In_ KPH_SOCKET_HANDLE Socket,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    NTSTATUS status;
    PKPH_SOCKET socket;
    WSK_BUF wskBuffer;
    BOOLEAN unlockPages;

    NPAGED_CODE_DISPATCH_MAX();

    socket = (PKPH_SOCKET)Socket;
    unlockPages = FALSE;

    wskBuffer.Offset = 0;
    wskBuffer.Length = Length;
    wskBuffer.Mdl = IoAllocateMdl(Buffer, Length, FALSE, FALSE, NULL);
    if (!wskBuffer.Mdl)
    {
        KphTracePrint(TRACE_LEVEL_ERROR, SOCKET, "IoAllocateMdl failed");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    __try
    {
        MmProbeAndLockPages(wskBuffer.Mdl, KernelMode, IoReadAccess);
        unlockPages = TRUE;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
        goto Exit;
    }

    KphpWskIoReset(&socket->Io);

    status = socket->WskDispatch->WskSend(socket->WskSocket,
                                          &wskBuffer,
                                          0,
                                          socket->Io.Irp);
    status = KphpWskIoWaitForCompletion(&socket->Io, status, Timeout);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "WskSend failed: %!STATUS!",
                      status);
    }

Exit:

    if (wskBuffer.Mdl)
    {
        if (unlockPages)
        {
            MmUnlockPages(wskBuffer.Mdl);
        }

        IoFreeMdl(wskBuffer.Mdl);
    }

    return status;
}

/**
 * \brief Receives data from a connection-oriented socket.
 *
 * \param[in] Socket A handle to the socket object to receive data from.
 * \param[in] Timeout Optional timeout for the receive to complete.
 * \param[out] Buffer The buffer to receive the data into.
 * \param[in,out] Length On input, the length of the buffer. On output, the
 * number of bytes written to the buffer.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
NTSTATUS KphSocketRecv(
    _In_ KPH_SOCKET_HANDLE Socket,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Out_writes_bytes_to_(*Length, *Length) PVOID Buffer,
    _Inout_ PULONG Length
    )
{
    NTSTATUS status;
    PKPH_SOCKET socket;
    WSK_BUF wskBuffer;
    BOOLEAN unlockPages;

    NPAGED_CODE_DISPATCH_MAX();

    socket = (PKPH_SOCKET)Socket;
    unlockPages = FALSE;

    wskBuffer.Offset = 0;
    wskBuffer.Length = *Length;
#pragma prefast(suppress: 6001)
    wskBuffer.Mdl = IoAllocateMdl(Buffer, *Length, FALSE, FALSE, NULL);
    if (!wskBuffer.Mdl)
    {
        KphTracePrint(TRACE_LEVEL_ERROR, SOCKET, "IoAllocateMdl failed");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    __try
    {
        MmProbeAndLockPages(wskBuffer.Mdl, KernelMode, IoWriteAccess);
        unlockPages = TRUE;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
        goto Exit;
    }

    KphpWskIoReset(&socket->Io);

    status = socket->WskDispatch->WskReceive(socket->WskSocket,
                                             &wskBuffer,
                                             0,
                                             socket->Io.Irp);
    status = KphpWskIoWaitForCompletion(&socket->Io, status, Timeout);
    if (NT_SUCCESS(status))
    {
        *Length = (ULONG)socket->Io.Irp->IoStatus.Information;
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "WskReceive failed: %!STATUS!",
                      status);
    }

Exit:

    if (wskBuffer.Mdl)
    {
        if (unlockPages)
        {
            MmUnlockPages(wskBuffer.Mdl);
        }

        IoFreeMdl(wskBuffer.Mdl);
    }

    return status;
}


PAGED_FILE();

/**
 * \brief Initializes the socket infrastructure.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphInitializeSocket(
    VOID
    )
{
    NTSTATUS status;
    WSK_CLIENT_NPI wskClient;

    PAGED_CODE_PASSIVE();

    wskClient.ClientContext = NULL;
    wskClient.Dispatch = &KphpWskDispatch;

    status = WskRegister(&wskClient, &KphpWskRegistration);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "WskRegister failed: %!STATUS!",
                      status);

        goto Exit;
    }

    KphpWskRegistered = TRUE;

    status = WskCaptureProviderNPI(&KphpWskRegistration,
                                   WSK_INFINITE_WAIT,
                                   &KphpWskProvider);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "WskCaptureProviderNPI failed: %!STATUS!",
                      status);

        goto Exit;
    }

    KphpWskProviderCaptured = TRUE;

Exit:

    return status;
}

/**
 * \brief Cleans up the socket infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupSocket(
    VOID
    )
{
    PAGED_CODE_PASSIVE();

    if (KphpWskProviderCaptured)
    {
        WskReleaseProviderNPI(&KphpWskRegistration);
    }

    if (KphpWskRegistered)
    {
        WskDeregister(&KphpWskRegistration);
    }
}

/**
 * \brief Performs protocol-independent name resolution.
 *
 * \param[in] NodeName Host (node) name or a numeric host address string.
 * \param[in] ServiceName Optional service name or port number string.
 * \param[in] Hints Optional hints about the type of socket desired.
 * \param[in] Timeout Optional timeout for the resolution to complete.
 * \param[out] AddressInfo Receives a linked list of resolved items. This must 
 * be freed using KphFreeAddressInfo.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetAddressInfo(
    _In_ PUNICODE_STRING NodeName,
    _In_opt_ PUNICODE_STRING ServiceName,
    _In_opt_ PADDRINFOEXW Hints,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Outptr_allocatesMem_ PADDRINFOEXW* AddressInfo
    )
{
    NTSTATUS status;
    KPH_WSK_IO io;

    PAGED_CODE_PASSIVE();

    *AddressInfo = NULL;

    status = KphpWskIoCreate(&io);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "KphpWskIoCreate failed: %!STATUS!",
                      status);

        return status;
    }

    status = KphpWskProvider.Dispatch->WskGetAddressInfo(KphpWskProvider.Client,
                                                         NodeName,
                                                         ServiceName,
                                                         0,
                                                         NULL,
                                                         Hints,
                                                         AddressInfo,
                                                         NULL,
                                                         NULL,
                                                         io.Irp);
    status = KphpWskIoWaitForCompletion(&io, status, Timeout);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "WskGetAddressInfo failed: %!STATUS!",
                      status);

        *AddressInfo = NULL;
    }

    KphpWskIoDelete(&io);

    return status;
}

/**
 * \brief Frees the address information returned by KphGetAddressInfo.
 *
 * \param[in] AddressInfo The address information to free.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphFreeAddressInfo(
    _In_freesMem_ PADDRINFOEXW AddressInfo
    )
{
    PAGED_CODE_PASSIVE();

    KphpWskProvider.Dispatch->WskFreeAddressInfo(KphpWskProvider.Client,
                                                 AddressInfo);
}
