#include <kph.h>
#include <sspi.h>
#include <winerror.h>

#include <trace.h>

static WSK_REGISTRATION KphpWskRegistration;
static WSK_PROVIDER_NPI KphpWskProvider;
static WSK_CLIENT_DISPATCH KphpWskDispatch = { MAKE_WSK_VERSION(1, 0), 0, NULL };
static BOOLEAN KphpWskRegistered = FALSE;
static BOOLEAN KphpWskProviderCaptured = FALSE;
static UNICODE_STRING KphpSecurityPackageName = RTL_CONSTANT_STRING(SCHANNEL_NAME_W);
static LARGE_INTEGER KphpSocketCloseTimeout = { .QuadPart = -30000000ll }; // 3 seconds
KPH_PROTECTED_DATA_SECTION_PUSH();
//
// Not all the functions we need are exported, however they should all be
// available through the dispatch table.
//
static PSecurityFunctionTableW KphpSecFnTable = NULL;
KPH_PROTECTED_DATA_SECTION_POP();

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

typedef struct _KPH_TLS_RECV
{
    ULONG Received;
    ULONG Used;
    ULONG Available;
    PVOID Decrypted;
} KPH_TLS_RECV, *PKPH_TLS_RECV;

typedef struct _KPH_TLS
{
    CredHandle CredentialsHandle;
    CtxtHandle ContextHandle;
    SecPkgContext_StreamSizes StreamSizes;
    PVOID Buffer;
    ULONG Length;
    KPH_TLS_RECV Recv;
} KPH_TLS, *PKPH_TLS;

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
    status = KphpWskIoWaitForCompletion(&socket->Io,
                                        status,
                                        &KphpSocketCloseTimeout);
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

    KphpSecFnTable = InitSecurityInterfaceW();
    if (!KphpSecFnTable)
    {
        KphTracePrint(TRACE_LEVEL_WARNING,
                      SOCKET,
                      "InitSecurityInterfaceW failed");
    }

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

/**
 * \brief Converts a SECURITY_STATUS (HRESULT) code to an NTSTATUS code.
 *
 * \param[in] SecStatus The SECURITY_STATUS code to convert.
 *
 * \return STATUS_SUCCESS if SecStatus is SEC_E_OK. Otherwise, an errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS KphpSecStatusToNtStatus(
    _In_ SECURITY_STATUS SecStatus
    )
{
    PAGED_CODE_PASSIVE();

    switch (SecStatus)
    {
        //
        // N.B. Should always return errant NTSTATUS except for SEC_E_OK.
        //
        case SEC_E_OK:
        {
            return STATUS_SUCCESS;
        }
        case SEC_E_INSUFFICIENT_MEMORY:
        case SEC_E_EXT_BUFFER_TOO_SMALL:
        case SEC_E_INSUFFICIENT_BUFFERS:
        case SEC_E_BUFFER_TOO_SMALL:
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        case SEC_E_INVALID_PARAMETER:
        {
            return STATUS_INVALID_PARAMETER;
        }
        case SEC_E_INVALID_HANDLE:
        case SEC_E_WRONG_CREDENTIAL_HANDLE:
        {
            return STATUS_INVALID_HANDLE;
        }
        case SEC_E_QOP_NOT_SUPPORTED:
        case SEC_E_UNSUPPORTED_FUNCTION:
        {
            return STATUS_NOT_SUPPORTED;
        }
        case SEC_E_TARGET_UNKNOWN:
        case SEC_E_SECPKG_NOT_FOUND:
        {
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }
        case SEC_E_INTERNAL_ERROR:
        {
            return STATUS_INTERNAL_ERROR;
        }
        case SEC_E_KDC_CERT_EXPIRED:
        case SEC_E_CERT_EXPIRED:
        {
            return STATUS_KDC_CERT_EXPIRED;
        }
        case SEC_E_KDC_CERT_REVOKED:
        {
            return STATUS_KDC_CERT_REVOKED;
        }
        case SEC_E_CERT_UNKNOWN:
        case SEC_E_UNTRUSTED_ROOT:
        case SEC_E_CERT_WRONG_USAGE:
        case SEC_E_WRONG_PRINCIPAL:
        case SEC_E_ISSUING_CA_UNTRUSTED:
        case SEC_E_ISSUING_CA_UNTRUSTED_KDC:
        case SEC_E_NO_AUTHENTICATING_AUTHORITY:
        case SEC_E_NO_KERB_KEY:
        {
            return STATUS_ISSUING_CA_UNTRUSTED;
        }
        case SEC_E_LOGON_DENIED:
        case SEC_E_NO_CREDENTIALS:
        case SEC_E_NOT_OWNER:
        {
            return STATUS_ACCESS_DENIED;
        }
        case SEC_I_CONTEXT_EXPIRED: // server closed the TLS connection
        case SEC_I_RENEGOTIATE:     // we don't support TLS renegotiation
        {
            return STATUS_PORT_DISCONNECTED;
        }
        case SEC_E_DECRYPT_FAILURE:
        {
            return STATUS_DECRYPTION_FAILED;
        }
        case SEC_E_ENCRYPT_FAILURE:
        {
            return STATUS_ENCRYPTION_FAILED;
        }
        case SEC_E_OUT_OF_SEQUENCE:
        {
            return STATUS_REQUEST_OUT_OF_SEQUENCE;
        }
        case SEC_E_INCOMPLETE_MESSAGE:
        {
            return STATUS_INVALID_NETWORK_RESPONSE;
        }
        case E_NOINTERFACE: // missing KphpSecFnTable or dispatch function
        {
            return STATUS_NOINTERFACE;
        }
        default:
        {
            //
            // All other codes are normalized to a generic error.
            //
            NT_ASSERT(FALSE);
            return STATUS_UNSUCCESSFUL;
        }
    }
}

/**
 * \brief Wrapper for AcquireCredentialsHandle.
 */
_Function_class_(ACQUIRE_CREDENTIALS_HANDLE_FN_W)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Success_(return == SEC_E_OK)
_Must_inspect_result_
SECURITY_STATUS KphpSecAcquireCredentialsHandle(
    _In_opt_ PSECURITY_STRING Principal,
    _In_ PSECURITY_STRING Package,
    _In_ ULONG CredentialUse,
    _In_opt_ PVOID LogonId,
    _In_opt_ PVOID AuthData,
    _In_opt_ SEC_GET_KEY_FN GetKeyFn,
    _In_opt_ PVOID GetKeyArgument,
    _Out_ PCredHandle Credential,
    _Out_opt_ PTimeStamp Expiry
    )
{
    PAGED_CODE_PASSIVE();

    NT_ASSERT(PsGetCurrentProcess() == PsInitialSystemProcess);

    if (!KphpSecFnTable || !KphpSecFnTable->AcquireCredentialsHandleW)
    {
        return E_NOINTERFACE;
    }

    return KphpSecFnTable->AcquireCredentialsHandleW(Principal,
                                                     Package,
                                                     CredentialUse,
                                                     LogonId,
                                                     AuthData,
                                                     GetKeyFn,
                                                     GetKeyArgument,
                                                     Credential,
                                                     Expiry);
}

/**
 * \brief Wrapper for DeleteSecurityContext.
 */
_Function_class_(DELETE_SECURITY_CONTEXT_FN)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Success_(return == SEC_E_OK)
SECURITY_STATUS KphpSecDeleteSecurityContext(
    _In_ PCtxtHandle Context
    )
{
    PAGED_CODE_PASSIVE();

    NT_ASSERT(PsGetCurrentProcess() == PsInitialSystemProcess);
    NT_ASSERT(SecIsValidHandle(Context));

    if (!KphpSecFnTable || !KphpSecFnTable->DeleteSecurityContext)
    {
        return E_NOINTERFACE;
    }

    return KphpSecFnTable->DeleteSecurityContext(Context);
}

/**
 * \brief Wrapper for FreeContextBuffer.
 */
_Function_class_(FREE_CONTEXT_BUFFER_FN)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Success_(return == SEC_E_OK)
SECURITY_STATUS KphpSecFreeContextBuffer(
    _Inout_ PVOID ContextBuffer
    )
{
    PAGED_CODE_PASSIVE();

    NT_ASSERT(PsGetCurrentProcess() == PsInitialSystemProcess);

    if (!KphpSecFnTable || !KphpSecFnTable->FreeContextBuffer)
    {
        return E_NOINTERFACE;
    }

    return KphpSecFnTable->FreeContextBuffer(ContextBuffer);
}

/**
 * \brief Wrapper for FreeCredentialsHandle.
 */
_Function_class_(FREE_CREDENTIALS_HANDLE_FN)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Success_(return == SEC_E_OK)
SECURITY_STATUS KphpSecFreeCredentialsHandle(
    _In_ PCredHandle Credential
    )
{
    PAGED_CODE_PASSIVE();

    NT_ASSERT(PsGetCurrentProcess() == PsInitialSystemProcess);
    NT_ASSERT(SecIsValidHandle(Credential));

    if (!KphpSecFnTable || !KphpSecFnTable->FreeCredentialsHandle)
    {
        return E_NOINTERFACE;
    }

    return KphpSecFnTable->FreeCredentialsHandle(Credential);
}

/**
 * \brief Wrapper for InitializeSecurityContext.
 */
_Function_class_(INITIALIZE_SECURITY_CONTEXT_FN_W)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Success_(return == SEC_E_OK)
_Must_inspect_result_
SECURITY_STATUS KphpSecInitializeSecurityContext(
    _In_opt_ PCredHandle Credential,
    _In_opt_ PCtxtHandle Context,
    _In_opt_ PSECURITY_STRING TargetName,
    _In_ ULONG ContextReq,
    _In_ ULONG Reserved1,
    _In_ ULONG TargetDataRep,
    _In_opt_ PSecBufferDesc Input,
    _In_ ULONG Reserved2,
    _Inout_opt_ PCtxtHandle NewContext,
    _Inout_opt_ PSecBufferDesc Output,
    _Out_ PULONG ContextAttr,
    _Out_opt_ PTimeStamp Expiry
    )
{
    PAGED_CODE_PASSIVE();

    NT_ASSERT(PsGetCurrentProcess() == PsInitialSystemProcess);

    if (!KphpSecFnTable || !KphpSecFnTable->InitializeSecurityContextW)
    {
        return E_NOINTERFACE;
    }

    return KphpSecFnTable->InitializeSecurityContextW(Credential,
                                                      Context,
                                                      TargetName,
                                                      ContextReq,
                                                      Reserved1,
                                                      TargetDataRep,
                                                      Input,
                                                      Reserved2,
                                                      NewContext,
                                                      Output,
                                                      ContextAttr,
                                                      Expiry);
}

/**
 * \brief Wrapper for QueryContextAttributes.
 */
_Function_class_(QUERY_CONTEXT_ATTRIBUTES_FN_W)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Success_(return == SEC_E_OK)
_Must_inspect_result_
SECURITY_STATUS KphpSecQueryContextAttributes(
    _In_ PCtxtHandle Context,
    _In_ ULONG Attribute,
    _Out_ PVOID Buffer
    )
{
    PAGED_CODE_PASSIVE();

    NT_ASSERT(PsGetCurrentProcess() == PsInitialSystemProcess);

    if (!KphpSecFnTable || !KphpSecFnTable->QueryContextAttributesW)
    {
        return E_NOINTERFACE;
    }

    return KphpSecFnTable->QueryContextAttributesW(Context, Attribute, Buffer);
}

/**
 * \brief Wrapper for EncryptMessage.
 */
_Function_class_(ENCRYPT_MESSAGE_FN)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Success_(return == SEC_E_OK)
_Must_inspect_result_
SECURITY_STATUS KphpSecEncryptMessage(
    _In_ PCtxtHandle Context,
    _In_ ULONG QOP,
    _In_ PSecBufferDesc Message,
    _In_ ULONG MessageSeqNo
    )
{
    PAGED_CODE_PASSIVE();

    NT_ASSERT(PsGetCurrentProcess() == PsInitialSystemProcess);

    if (!KphpSecFnTable || !KphpSecFnTable->EncryptMessage)
    {
        return E_NOINTERFACE;
    }

    return KphpSecFnTable->EncryptMessage(Context, QOP, Message, MessageSeqNo);
}

/**
 * \brief Wrapper for DecryptMessage.
 */
_Function_class_(DECRYPT_MESSAGE_FN)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Success_(return == SEC_E_OK)
_Must_inspect_result_
SECURITY_STATUS KphpSecDecryptMessage(
    _In_ PCtxtHandle Context,
    _In_ PSecBufferDesc Message,
    _In_ ULONG MessageSeqNo,
    _Out_opt_ PULONG QOP
    )
{
    PAGED_CODE_PASSIVE();

    NT_ASSERT(PsGetCurrentProcess() == PsInitialSystemProcess);

    if (!KphpSecFnTable || !KphpSecFnTable->DecryptMessage)
    {
        return E_NOINTERFACE;
    }

    return KphpSecFnTable->DecryptMessage(Context, Message, MessageSeqNo, QOP);
}

/**
 * \brief Wrapper for ApplyControlToken.
 */
_Function_class_(APPLY_CONTROL_TOKEN_FN)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Success_(return == SEC_E_OK)
_Must_inspect_result_
SECURITY_STATUS KphpSecApplyControlToken(
    _In_ PCtxtHandle Context,
    _In_ PSecBufferDesc Input
    )
{
    PAGED_CODE_PASSIVE();

    NT_ASSERT(PsGetCurrentProcess() == PsInitialSystemProcess);

    if (!KphpSecFnTable || !KphpSecFnTable->ApplyControlToken)
    {
        return E_NOINTERFACE;
    }

    return KphpSecFnTable->ApplyControlToken(Context, Input);
}

/**
 * \brief Closes a TLS object.
 *
 * \param[in] Tls Handle to a TLS object to close.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphSocketTlsClose(
    _In_freesMem_ KPH_TLS_HANDLE Tls
    )
{
    PKPH_TLS tls;
    KAPC_STATE apcState;

    PAGED_CODE_PASSIVE();

    KeStackAttachProcess(PsInitialSystemProcess, &apcState);

    tls = (PKPH_TLS)Tls;

    NT_ASSERT(SecIsValidHandle(&tls->CredentialsHandle));
    NT_ASSERT(!SecIsValidHandle(&tls->ContextHandle));

    KphpSecFreeCredentialsHandle(&tls->CredentialsHandle);

    if (tls->Buffer)
    {
        KphFree(tls->Buffer, KPH_TAG_TLS_BUFFER);
    }

    KphFree(tls, KPH_TAG_TLS);

    KeUnstackDetachProcess(&apcState);
}

/**
 * \brief Creates a TLS object.
 *
 * \details The TLS handle should be closed using KphSocketTlsClose.
 *
 * \param[out] Tls On success, receives a handle to the TLS object.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphSocketTlsCreate(
    _Outptr_allocatesMem_ PKPH_TLS_HANDLE Tls
    )
{
    NTSTATUS status;
    SECURITY_STATUS secStatus;
    KAPC_STATE apcState;
    PKPH_TLS tls;
    SCH_CREDENTIALS credentials;
    TLS_PARAMETERS tlsParameters[1];

    PAGED_CODE_PASSIVE();

    *Tls = NULL;

    KeStackAttachProcess(PsInitialSystemProcess, &apcState);

    tls = KphAllocatePaged(sizeof(KPH_TLS), KPH_TAG_TLS);
    if (!tls)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "Failed to allocate TLS object");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    SecInvalidateHandle(&tls->CredentialsHandle);
    SecInvalidateHandle(&tls->ContextHandle);

    RtlZeroMemory(&credentials, sizeof(credentials));
    RtlZeroMemory(&tlsParameters, sizeof(tlsParameters));

    credentials.dwVersion = SCH_CREDENTIALS_VERSION;
    credentials.dwFlags = (
        SCH_USE_STRONG_CRYPTO |
        SCH_CRED_AUTO_CRED_VALIDATION |
        SCH_CRED_NO_DEFAULT_CREDS |
        SCH_CRED_REVOCATION_CHECK_CHAIN
        );
    credentials.cTlsParameters = ARRAYSIZE(tlsParameters);
    credentials.pTlsParameters = tlsParameters;
    //
    // TODO(jxy-s) look into testing and supporting TLS 1.3
    //
    tlsParameters[0].grbitDisabledProtocols = (ULONG)~SP_PROT_TLS1_2;

    secStatus = KphpSecAcquireCredentialsHandle(NULL,
                                                &KphpSecurityPackageName,
                                                SECPKG_CRED_OUTBOUND,
                                                NULL,
                                                &credentials,
                                                NULL,
                                                NULL,
                                                &tls->CredentialsHandle,
                                                NULL);
    if (secStatus != SEC_E_OK)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "KphpSecAcquireCredentialsHandle failed: %!HRESULT!",
                      secStatus);

        status = KphpSecStatusToNtStatus(secStatus);
        NT_ASSERT(!NT_SUCCESS(status));
        goto Exit;
    }

    *Tls = tls;
    tls = NULL;
    status = STATUS_SUCCESS;

Exit:

    if (tls)
    {
        if (SecIsValidHandle(&tls->CredentialsHandle))
        {
            KphpSecFreeCredentialsHandle(&tls->CredentialsHandle);
        }

        KphFree(tls, KPH_TAG_TLS);
    }

    KeUnstackDetachProcess(&apcState);

    return status;
}

/**
 * \brief Reallocates the buffer used by a TLS object.
 *
 * \param[in,out] Tls The object to reallocate the buffer of.
 * \param[in] Length The requested new length of the buffer.
 *
 * \return Successful or error status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpSocketTlsReallocateBuffer(
    _Inout_ PKPH_TLS Tls,
    _In_ ULONG Length
    )
{
    PVOID buffer;

    PAGED_CODE_PASSIVE();

    if (Tls->Length >= Length)
    {
        return STATUS_SUCCESS;
    }

    if (Length > MAXUSHORT)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "Requested TLS buffer size (%lu) is too large",
                      Length);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    buffer = KphAllocatePaged(Length, KPH_TAG_TLS_BUFFER);
    if (!buffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Tls->Buffer)
    {
        RtlCopyMemory(buffer, Tls->Buffer, Tls->Length);
        KphFree(Tls->Buffer, KPH_TAG_TLS_BUFFER);
    }

    Tls->Buffer = buffer;
    Tls->Length = Length;

    return STATUS_SUCCESS;
}

/**
 * \brief Finalizes the TLS handshake.
 *
 * \details This must be called after the handshake completes successfully.
 *
 * \param[in] Tls The TLS object to finalize the handshake of.
 *
 * \return Successful or error status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpSocketTlsHandshakeFinalize(
    _Inout_ PKPH_TLS Tls
    )
{
    NTSTATUS status;
    SECURITY_STATUS secStatus;
    ULONG length;

    PAGED_CODE_PASSIVE();

    secStatus = KphpSecQueryContextAttributes(&Tls->ContextHandle,
                                              SECPKG_ATTR_STREAM_SIZES,
                                              &Tls->StreamSizes);
    if (secStatus != SEC_E_OK)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "KphpSecQueryContextAttributes failed: %!HRESULT!",
                      secStatus);

        status = KphpSecStatusToNtStatus(secStatus);
        NT_ASSERT(!NT_SUCCESS(status));
        goto Exit;
    }

    length = 0;

    status = RtlULongAdd(length, Tls->StreamSizes.cbHeader, &length);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = RtlULongAdd(length, Tls->StreamSizes.cbMaximumMessage, &length);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = RtlULongAdd(length, Tls->StreamSizes.cbTrailer, &length);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpSocketTlsReallocateBuffer(Tls, length);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "KphpSocketTlsReallocateBuffer failed: %!STATUS!",
                      status);

        goto Exit;
    }

Exit:

    return status;
}

/**
 * \brief Processes extra buffers during the TSL handshake.
 *
 * \details There are a few cases where this can happen:
 * 1. A connection is being renegotiated (we don't support this). Include the
 *    information to be processed immediately regardless.
 * 2. We are negotiating a connection and this extra data is part of the
 *    handshake, usually due to the initial buffer being insufficient.
 *    Downstream we will reallocate as necessary.
 * Regardless, this prepares the state during the handshake by moving the extra
 * data to the front of the buffer.
 *
 * \param[in,out] Tls The TLS object to process extra data for.
 * \param[in] Extra The extra data to process.
 * \param[in,out] Received On input, the amount of data already received during
 * the handshake. On output, the amount of extra data that needs processed.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpSocketTlsHandshakeExtra(
    _Inout_ PKPH_TLS Tls,
    _In_ PSecBuffer Extra,
    _Inout_ PULONG Received
    )
{
    NTSTATUS status;
    ULONG offset;
    ULONG end;

    PAGED_CODE_PASSIVE();

    NT_ASSERT(Extra->BufferType == SECBUFFER_EXTRA);

    status = RtlULongSub(*Received, Extra->cbBuffer, &offset);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "RtlULongSub failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (offset == 0)
    {
        //
        // No need to move data if the extra data is at the front.
        //
        *Received = Extra->cbBuffer;
        status = STATUS_SUCCESS;
        goto Exit;
    }

    status = RtlULongAdd(offset, Extra->cbBuffer, &end);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (end > Tls->Length)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "Unexpected buffer overflow.");

        status = STATUS_BUFFER_OVERFLOW;
        goto Exit;
    }

    RtlMoveMemory(Tls->Buffer,
                  Add2Ptr(Tls->Buffer, offset),
                  Extra->cbBuffer);
    *Received = Extra->cbBuffer;
    status = STATUS_SUCCESS;

Exit:

    return status;
}

/**
 * \brief Receives data during the TLS handshake.
 *
 * \details Reallocates the internal TLS object buffer if necessary.
 *
 * \param[in] Socket A handle to the socket object to receive data from.
 * \param[in] Timeout Optional timeout for the receive.
 * \param[in,out] Tls The TLS object to receive data for.
 * \param[in,out] Received On input, the amount of data already received during
 * the TLS handshake. On output, updated to reflect the addition of the newly
 * received bytes.
 *
 * \return Successful or error status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpSocketTlsHandshakeRecv(
    _In_ KPH_SOCKET_HANDLE Socket,
    _In_opt_ PLARGE_INTEGER Timeout,
    _Inout_ PKPH_TLS Tls,
    _Inout_ PULONG Received
    )
{
    NTSTATUS status;
    ULONG length;

    PAGED_CODE_PASSIVE();

    if (*Received >= Tls->Length)
    {
        status = RtlULongAdd(Tls->Length, PAGE_SIZE, &length);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          SOCKET,
                          "RtlULongAdd failed: %!STATUS!",
                          status);

            goto Exit;
        }

        status = KphpSocketTlsReallocateBuffer(Tls, length);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          SOCKET,
                          "KphpSocketTlsReallocateBuffer failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    NT_ASSERT(Tls->Length > *Received);
    length = Tls->Length - *Received;

    status = KphSocketRecv(Socket,
                           Timeout,
                           Add2Ptr(Tls->Buffer, *Received),
                           &length);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "KphSocketRecv failed: %!STATUS!",
                      status);

        goto Exit;
    }

    *Received += length;

Exit:

    return status;
}

/**
 * \brief Performs TLS handshake.
 *
 * \details After a successful TLS handshake, the caller must eventually call
 * KphSocketTlsShutdown to inform the peer the intention to shut down the TLS
 * session. It is acceptable to call TlsSocketTlsShutdown on a TLS handle even
 * if this routine fails.
 *
 * \param[in] Socket Handle to a socket to perform the handshake on.
 * \param[in] Timeout Optional timeout for the handshake. This timeout is for
 * any individual socket operation. meaning that the total time spent may
 * exceed the requested timeout. But any given socket operation will not.
 * \param[in] Tls Handle to a TLS object.
 * \param[in] TargetName The target name to use for principal verification.
 *
 * \return Successful or error status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphSocketTlsHandshake(
    _In_ KPH_SOCKET_HANDLE Socket,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ KPH_TLS_HANDLE Tls,
    _In_ PUNICODE_STRING TargetName
    )
{
    NTSTATUS status;
    SECURITY_STATUS secStatus;
    KAPC_STATE apcState;
    PKPH_TLS tls;
    CtxtHandle* context;
    SecBuffer inBuffers[2];
    SecBuffer outBuffers[1];
    SecBufferDesc inDesc;
    SecBufferDesc outDesc;
    ULONG flags;

    PAGED_CODE_PASSIVE();

    tls = (PKPH_TLS)Tls;

    NT_ASSERT(SecIsValidHandle(&tls->CredentialsHandle));
    NT_ASSERT(!SecIsValidHandle(&tls->ContextHandle));

    context = NULL;
    outBuffers[0].pvBuffer = NULL;

    inDesc.ulVersion = SECBUFFER_VERSION;
    inDesc.cBuffers = ARRAYSIZE(inBuffers);
    inDesc.pBuffers = inBuffers;

    outDesc.ulVersion = SECBUFFER_VERSION;
    outDesc.cBuffers = ARRAYSIZE(outBuffers);
    outDesc.pBuffers = outBuffers;

    KeStackAttachProcess(PsInitialSystemProcess, &apcState);

    status = KphpSocketTlsReallocateBuffer(tls, PAGE_SIZE * 2);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "KphpSocketTlsReallocateBuffer failed: %!STATUS!",
                      status);

        goto Exit;
    }

    for (ULONG received = 0;;)
    {
        inBuffers[0].BufferType = SECBUFFER_TOKEN;
        inBuffers[0].pvBuffer = tls->Buffer;
        inBuffers[0].cbBuffer = received;

        inBuffers[1].BufferType = SECBUFFER_EMPTY;
        inBuffers[1].pvBuffer = NULL;
        inBuffers[1].cbBuffer = 0;

        if (outBuffers[0].pvBuffer)
        {
            KphpSecFreeContextBuffer(outBuffers[0].pvBuffer);
        }
        outBuffers[0].BufferType = SECBUFFER_TOKEN;
        outBuffers[0].pvBuffer = NULL;
        outBuffers[0].cbBuffer = 0;

        flags = (
            ISC_REQ_ALLOCATE_MEMORY |
            ISC_REQ_USE_SUPPLIED_CREDS |
            ISC_REQ_CONFIDENTIALITY |
            ISC_REQ_REPLAY_DETECT |
            ISC_REQ_SEQUENCE_DETECT |
            ISC_REQ_STREAM
            );

        secStatus = KphpSecInitializeSecurityContext(&tls->CredentialsHandle,
                                                     context,
                                                     context ? NULL : TargetName,
                                                     flags,
                                                     0,
                                                     SECURITY_NETWORK_DREP,
                                                     context ? &inDesc : NULL,
                                                     0,
                                                     &tls->ContextHandle,
                                                     &outDesc,
                                                     &flags,
                                                     NULL);

        context = &tls->ContextHandle;

        if ((inBuffers[1].BufferType == SECBUFFER_EXTRA) &&
            (inBuffers[1].cbBuffer > 0))
        {
            status = KphpSocketTlsHandshakeExtra(tls,
                                                 &inBuffers[1],
                                                 &received);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_ERROR,
                              SOCKET,
                              "KphpSocketTlsHandshakeExtra failed: %!STATUS!",
                              status);

                goto Exit;
            }

            //
            // Force a receive. But, only if a output continuation isn't
            // necessary (there is an output buffer).
            //
            if ((secStatus == SEC_I_CONTINUE_NEEDED) &&
                (outBuffers[0].BufferType != SECBUFFER_MISSING))
            {
                secStatus = SEC_E_INCOMPLETE_MESSAGE;
            }
        }
        else if (inBuffers[1].BufferType != SECBUFFER_MISSING)
        {
            received = 0;
        }

        if (secStatus == SEC_E_OK)
        {
            //
            // The handshake completed successfully.
            //
            status = KphpSocketTlsHandshakeFinalize(Tls);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_ERROR,
                              SOCKET,
                              "KphpSocketTlsHandshakeFinalize failed: %!STATUS!",
                              status);

            }

            goto Exit;
        }

        if (secStatus == SEC_I_INCOMPLETE_CREDENTIALS)
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          SOCKET,
                          "Server requested client authentication, "
                          "client authentication is not supported.");

            status = STATUS_NOT_SUPPORTED;
            goto Exit;
        }

        if ((secStatus == SEC_I_CONTINUE_NEEDED) &&
            (outBuffers[0].BufferType != SECBUFFER_MISSING))
        {
            status = KphSocketSend(Socket,
                                   Timeout,
                                   outBuffers[0].pvBuffer,
                                   outBuffers[0].cbBuffer);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_ERROR,
                              SOCKET,
                              "KphSocketSend failed: %!STATUS!",
                              status);

                goto Exit;
            }

            continue;
        }

        if (secStatus != SEC_E_INCOMPLETE_MESSAGE)
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          SOCKET,
                          "TSL handshake failed: %!HRESULT!",
                          secStatus);

            status = KphpSecStatusToNtStatus(secStatus);
            NT_ASSERT(!NT_SUCCESS(status));
            goto Exit;
        }

        //
        // The handshake is not complete, we need to receive more data.
        //

        status = KphpSocketTlsHandshakeRecv(Socket, Timeout, tls, &received);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          SOCKET,
                          "KphpSocketTlsHandshakeRecv failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

Exit:

    if (outBuffers[0].pvBuffer)
    {
        KphpSecFreeContextBuffer(outBuffers[0].pvBuffer);
    }

    if (!NT_SUCCESS(status) && SecIsValidHandle(&tls->ContextHandle))
    {
        KphpSecDeleteSecurityContext(&tls->ContextHandle);
        SecInvalidateHandle(&tls->ContextHandle);
    }

    KeUnstackDetachProcess(&apcState);

    return status;
}

/**
 * \brief Shuts down a TLS session.
 *
 * \details It is appropriate to call this even when KphSocketTlsHandshake
 * fails. Regardless of any send or receive errors, shutdown should always be
 * called to inform the peer of the intention to shut down the TLS session.
 *
 * \param[in] Socket Handle to a socket object to shut down the TLS session on.
 * \param[in] Tls Handle to a TLS object to shut down the session of.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphSocketTlsShutdown(
    _In_ KPH_SOCKET_HANDLE Socket,
    _In_ KPH_TLS_HANDLE Tls
    )
{
    NTSTATUS status;
    SECURITY_STATUS secStatus;
    KAPC_STATE apcState;
    PKPH_TLS tls;
    ULONG shutdown;
    SecBuffer inBuffers[1];
    SecBuffer outBuffers[1];
    SecBufferDesc inDesc;
    SecBufferDesc outDesc;
    ULONG flags;

    PAGED_CODE_PASSIVE();

    tls = (PKPH_TLS)Tls;

    if (!SecIsValidHandle(&tls->ContextHandle))
    {
        return;
    }

    KeStackAttachProcess(PsInitialSystemProcess, &apcState);

    inDesc.ulVersion = SECBUFFER_VERSION;
    inDesc.cBuffers = ARRAYSIZE(inBuffers);
    inDesc.pBuffers = inBuffers;

    outDesc.ulVersion = SECBUFFER_VERSION;
    outDesc.cBuffers = ARRAYSIZE(outBuffers);
    outDesc.pBuffers = outBuffers;

    shutdown = SCHANNEL_SHUTDOWN;
    inBuffers[0].BufferType = SECBUFFER_TOKEN;
    inBuffers[0].pvBuffer = &shutdown;
    inBuffers[0].cbBuffer = sizeof(shutdown);

    outBuffers[0].pvBuffer = NULL;

    secStatus = KphpSecApplyControlToken(&tls->ContextHandle, &inDesc);
    if (secStatus != SEC_E_OK)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      SOCKET,
                      "KphpSecApplyControlToken failed %!HRESULT!",
                      secStatus);

        goto Exit;
    }

    do
    {
        inBuffers[0].BufferType = SECBUFFER_EMPTY;
        inBuffers[0].pvBuffer = NULL;
        inBuffers[0].cbBuffer = 0;

        if (outBuffers[0].pvBuffer)
        {
            KphpSecFreeContextBuffer(outBuffers[0].pvBuffer);
        }
        outBuffers[0].BufferType = SECBUFFER_TOKEN;
        outBuffers[0].pvBuffer = NULL;
        outBuffers[0].cbBuffer = 0;

        flags = (
            ISC_REQ_ALLOCATE_MEMORY |
            ISC_REQ_CONFIDENTIALITY |
            ISC_REQ_REPLAY_DETECT |
            ISC_REQ_SEQUENCE_DETECT |
            ISC_REQ_STREAM
            );

        secStatus = KphpSecInitializeSecurityContext(&tls->CredentialsHandle,
                                                     &tls->ContextHandle,
                                                     NULL,
                                                     flags,
                                                     0,
                                                     SECURITY_NETWORK_DREP,
                                                     &inDesc,
                                                     0,
                                                     &tls->ContextHandle,
                                                     &outDesc,
                                                     &flags,
                                                     NULL);
        if ((secStatus == SEC_I_CONTINUE_NEEDED) &&
            (outBuffers[0].BufferType != SECBUFFER_MISSING))
        {
            status = KphSocketSend(Socket,
                                   &KphpSocketCloseTimeout,
                                   outBuffers[0].pvBuffer,
                                   outBuffers[0].cbBuffer);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_ERROR,
                              SOCKET,
                              "KphSocketSend failed: %!STATUS!",
                              status);

                goto Exit;
            }
        }

    } while ((secStatus != SEC_E_OK) && (secStatus != SEC_I_CONTEXT_EXPIRED));

Exit:

    if (outBuffers[0].pvBuffer)
    {
        KphpSecFreeContextBuffer(outBuffers[0].pvBuffer);
    }

    KphpSecDeleteSecurityContext(&tls->ContextHandle);
    SecInvalidateHandle(&tls->ContextHandle);

    RtlZeroMemory(&tls->StreamSizes, sizeof(tls->StreamSizes));

    RtlZeroMemory(&tls->Recv, sizeof(tls->Recv));

    KeUnstackDetachProcess(&apcState);
}

/**
 * \brief Sends data over a TLS session.
 *
 * \details If the requested data to be sent exceeds the maximum size capable
 * of being sent at once, this routine will perform multiple sends.
 *
 * \param[in] Socket Handle to a socket object to send data over.
 * \param[in] Timeout Optional timeout for the handshake. This timeout is for
 * any individual socket operation. meaning that the total time spent may
 * exceed the requested timeout. But any given socket operation will not.
 * \param[in] Tls Handle to a TLS object to use for sending data.
 * \param[in] Buffer Pointer to a buffer containing the data to send.
 * \param[in] Length The length of the data to send.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphSocketTlsSend(
    _In_ KPH_SOCKET_HANDLE Socket,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ KPH_TLS_HANDLE Tls,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    NTSTATUS status;
    SECURITY_STATUS secStatus;
    KAPC_STATE apcState;
    PKPH_TLS tls;
    SecBuffer buffers[3];
    SecBufferDesc desc;

    PAGED_CODE_PASSIVE();

    tls = (PKPH_TLS)Tls;

    RtlZeroMemory(&tls->Recv, sizeof(tls->Recv));

    if (Length == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    KeStackAttachProcess(PsInitialSystemProcess, &apcState);

    desc.ulVersion = SECBUFFER_VERSION;
    desc.cBuffers = ARRAYSIZE(buffers);
    desc.pBuffers = buffers;

    for (ULONG remaining = Length; remaining > 0;)
    {
        ULONG length;

        //
        // The preallocated buffer is determined during the handshake and will
        // be sufficient for the maximum packet size.
        //
        NT_ASSERT(tls->Length >= (tls->StreamSizes.cbHeader +
                                  tls->StreamSizes.cbMaximumMessage +
                                  tls->StreamSizes.cbTrailer));

        length = min(remaining, tls->StreamSizes.cbMaximumMessage);

        NT_ASSERT(length <= Length);
        NT_ASSERT(remaining <= Length);

        buffers[0].BufferType = SECBUFFER_STREAM_HEADER;
        buffers[0].pvBuffer = tls->Buffer;
        buffers[0].cbBuffer = tls->StreamSizes.cbHeader;
        buffers[1].BufferType = SECBUFFER_DATA;
        buffers[1].pvBuffer = Add2Ptr(tls->Buffer, tls->StreamSizes.cbHeader);
        buffers[1].cbBuffer = length;
        buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;
        buffers[2].pvBuffer = Add2Ptr(tls->Buffer, (ULONG_PTR)tls->StreamSizes.cbHeader + length);
        buffers[2].cbBuffer = tls->StreamSizes.cbTrailer;

        RtlCopyMemory(buffers[1].pvBuffer,
                      Add2Ptr(Buffer, Length - remaining),
                      length);

        secStatus = KphpSecEncryptMessage(&tls->ContextHandle, 0, &desc, 0);
        if (secStatus != SEC_E_OK)
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          SOCKET,
                          "KphpSecEncryptMessage failed: %!HRESULT!",
                          secStatus);

            status = KphpSecStatusToNtStatus(secStatus);
            NT_ASSERT(!NT_SUCCESS(status));
            goto Exit;
        }

        remaining -= length;

        length += (tls->StreamSizes.cbHeader + tls->StreamSizes.cbTrailer);

        status = KphSocketSend(Socket, Timeout, tls->Buffer, length);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          SOCKET,
                          "KphSocketSend failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    status = STATUS_SUCCESS;

Exit:

    KeUnstackDetachProcess(&apcState);

    return status;
}

/**
 * \brief Receives data over a TLS session.
 *
 * \details This routine will populate as much of the supplied buffer as
 * possible and prepare more data to be received by a following call. The
 * caller should check the output Length parameter for 0 to determine if there
 * is no more data to be received.
 *
 * \param[in] Socket Handle to a socket object to receive data over.
 * \param[in] Timeout Optional timeout for the handshake. This timeout is for
 * any individual socket operation. meaning that the total time spent may
 * exceed the requested timeout. But any given socket operation will not.
 * \param[in] Tls Handle to a TLS object to use for receiving data.
 * \param[out] Buffer Pointer to a buffer to receive the data.
 * \param[in,out] Length On input, the length of the buffer. On output, the
 * length of the data received.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphSocketTlsRecv(
    _In_ KPH_SOCKET_HANDLE Socket,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ KPH_TLS_HANDLE Tls,
    _Out_writes_bytes_to_(*Length, *Length) PVOID Buffer,
    _Inout_ PULONG Length
    )
{
    NTSTATUS status;
    KAPC_STATE apcState;
    PKPH_TLS tls;
    SecBuffer buffers[4];
    SecBufferDesc desc;
    PVOID buffer;
    ULONG length;

    PAGED_CODE_PASSIVE();

    KeStackAttachProcess(PsInitialSystemProcess, &apcState);

    tls = (PKPH_TLS)Tls;
    buffer = Buffer;
    length = *Length;

    desc.ulVersion = SECBUFFER_VERSION;
    desc.cBuffers = ARRAYSIZE(buffers);
    desc.pBuffers = buffers;

    while (length > 0)
    {
        ULONG received;
        SECURITY_STATUS secStatus;

        if (tls->Recv.Available > 0)
        {
            ULONG consumed;

            NT_ASSERT(tls->Recv.Decrypted);

            consumed = min(length, tls->Recv.Available);

            RtlCopyMemory(buffer, tls->Recv.Decrypted, consumed);
            buffer = Add2Ptr(buffer, consumed);
            length -= consumed;

            if (consumed == tls->Recv.Available)
            {
                NT_ASSERT(tls->Recv.Used <= tls->Length);
                NT_ASSERT(tls->Recv.Used <= tls->Recv.Received);

                RtlMoveMemory(tls->Buffer,
                              Add2Ptr(tls->Buffer, tls->Recv.Used),
                              tls->Recv.Received - tls->Recv.Used);

                tls->Recv.Received -= tls->Recv.Used;
                tls->Recv.Used = 0;
                tls->Recv.Available = 0;
                tls->Recv.Decrypted = NULL;
            }
            else
            {
                tls->Recv.Available -= consumed;
                tls->Recv.Decrypted = Add2Ptr(tls->Recv.Decrypted, consumed);
            }

            continue;
        }

        if (tls->Recv.Received > 0)
        {
            buffers[0].BufferType = SECBUFFER_DATA;
            buffers[0].pvBuffer = tls->Buffer;
            buffers[0].cbBuffer = tls->Recv.Received;
            buffers[1].BufferType = SECBUFFER_EMPTY;
            buffers[1].pvBuffer = NULL;
            buffers[1].cbBuffer = 0;
            buffers[2].BufferType = SECBUFFER_EMPTY;
            buffers[2].pvBuffer = NULL;
            buffers[2].cbBuffer = 0;
            buffers[3].BufferType = SECBUFFER_EMPTY;
            buffers[3].pvBuffer = NULL;
            buffers[3].cbBuffer = 0;

            secStatus = KphpSecDecryptMessage(&tls->ContextHandle,
                                              &desc,
                                              0,
                                              NULL);
            if (secStatus == SEC_E_OK)
            {
                if ((buffers[0].BufferType != SECBUFFER_STREAM_HEADER) ||
                    (buffers[1].BufferType != SECBUFFER_DATA) ||
                    (buffers[2].BufferType != SECBUFFER_STREAM_TRAILER))
                {
                    KphTracePrint(TRACE_LEVEL_ERROR,
                                  SOCKET,
                                  "Unexpected buffer types: %lu %lu %lu",
                                  buffers[0].BufferType,
                                  buffers[1].BufferType,
                                  buffers[2].BufferType);

                    status = STATUS_UNEXPECTED_NETWORK_ERROR;
                    goto Exit;
                }

                tls->Recv.Decrypted = buffers[1].pvBuffer;
                tls->Recv.Available = buffers[1].cbBuffer;
                tls->Recv.Used = tls->Recv.Received;
                if (buffers[3].BufferType == SECBUFFER_EXTRA)
                {
                    status = RtlULongSub(tls->Recv.Used,
                                         buffers[3].cbBuffer,
                                         &tls->Recv.Used);
                    if (!NT_SUCCESS(status))
                    {
                        KphTracePrint(TRACE_LEVEL_ERROR,
                                      SOCKET,
                                      "RtlULongSub failed: %!STATUS!",
                                      status);

                        goto Exit;
                    }
                }

                continue;
            }

            if (secStatus == SEC_I_CONTEXT_EXPIRED)
            {
                //
                // The TLS session has been closed.
                //
                RtlZeroMemory(&tls->Recv, sizeof(tls->Recv));
                break;
            }

            if (secStatus != SEC_E_INCOMPLETE_MESSAGE)
            {
                KphTracePrint(TRACE_LEVEL_ERROR,
                              SOCKET,
                              "KphpSecDecryptMessage failed: %!HRESULT!",
                              secStatus);

                status = KphpSecStatusToNtStatus(secStatus);
                NT_ASSERT(!NT_SUCCESS(status));
                goto Exit;
            }
        }

        NT_ASSERT(tls->Recv.Received < tls->Length);
        received = tls->Length - tls->Recv.Received;

        status = KphSocketRecv(Socket,
                               Timeout,
                               Add2Ptr(tls->Buffer, tls->Recv.Received),
                               &received);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          SOCKET,
                          "KphSocketRecv failed: %!STATUS!",
                          status);

            goto Exit;
        }

        if (received == 0)
        {
            if (tls->Recv.Received == 0)
            {
                NT_ASSERT(tls->Recv.Available == 0);
                break;
            }

            KphTracePrint(TRACE_LEVEL_ERROR,
                          SOCKET,
                          "Unexpected incomplete message");

            status = STATUS_UNEXPECTED_NETWORK_ERROR;
            goto Exit;
        }

        tls->Recv.Received += received;
    }

    status = STATUS_SUCCESS;

Exit:

    *Length = (*Length - length);

    if (!NT_SUCCESS(status))
    {
        RtlZeroMemory(&tls->Recv, sizeof(tls->Recv));
    }

    KeUnstackDetachProcess(&apcState);

    return status;
}
