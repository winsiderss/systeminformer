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

#include <kph.h>
#include <script.h>
#include <trace.h>

#define KPH_LUA_HW_DEVICE_NAME_MAX 32767
#define KPH_LUA_HW_GUARD_INPUT_MAX 256

typedef struct _KPH_LUA_HW_REQUEST
{
    LIST_ENTRY AbandonedLink;
    BOOLEAN Abandoned;
    BOOLEAN Completed;
    SINGLE_LIST_ENTRY CancelLink;
    PIRP Irp;
    PDEVICE_OBJECT Device;
    PFILE_OBJECT FileObject;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatus;
    KSI_IO_COMPLETION Completion;
    PMDL Mdl;
    PVOID Input;
    PVOID Data;
    PVOID Buffers;
} KPH_LUA_HW_REQUEST, *PKPH_LUA_HW_REQUEST;

typedef struct _KPH_LUA_HW_REQUEST_INIT
{
    PDEVICE_OBJECT Device;
    PFILE_OBJECT FileObject;
    ULONG InputOffset;
    ULONG DataOffset;
    ULONG BuffersOffset;
} KPH_LUA_HW_REQUEST_INIT, *PKPH_LUA_HW_REQUEST_INIT;

typedef struct _KPH_LUA_HW_BUFFERS
{
    int Index;
    ULONG Count;
    ULONG Length;
} KPH_LUA_HW_BUFFERS, *PKPH_LUA_HW_BUFFERS;

typedef struct _KPH_LUA_HW_REQUEST_SPEC
{
    UCHAR MajorFunction;
    ULONG IoControlCode;
    LONGLONG Offset;
    PCVOID Input;
    ULONG InputLength;
    PCVOID Data;
    ULONG DataLength;
    PKPH_LUA_HW_BUFFERS Buffers;
    BOOLEAN InputArgument;
} KPH_LUA_HW_REQUEST_SPEC, *PKPH_LUA_HW_REQUEST_SPEC;

typedef const KPH_LUA_HW_REQUEST_SPEC* PCKPH_LUA_HW_REQUEST_SPEC;

typedef struct _KPH_LUA_HW_NAMED_DEVICE
{
    PFILE_OBJECT FileObject;
} KPH_LUA_HW_NAMED_DEVICE, *PKPH_LUA_HW_NAMED_DEVICE;

typedef struct _KPH_LUA_HW_DEVICE
{
    PDEVICE_OBJECT Pdo;
} KPH_LUA_HW_DEVICE, *PKPH_LUA_HW_DEVICE;

typedef struct _KPH_LUA_HW_DEVICE_LIST
{
    PDEVICE_OBJECT* Devices;
    ULONG Count;
} KPH_LUA_HW_DEVICE_LIST, *PKPH_LUA_HW_DEVICE_LIST;

typedef struct _KPH_LUA_HW_GUARD
{
    PKPH_LUA_HW_REQUEST Request;
    ULONG IoControlCode;
    BOOLEAN Claimed;
} KPH_LUA_HW_GUARD, *PKPH_LUA_HW_GUARD;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
DECLARE_CONST_UNICODE_STRING(KphpLuaHwRequestTypeName, L"KphLuaHwRequest");
KPH_PROTECTED_DATA_SECTION_RO_POP();
KPH_PROTECTED_DATA_SECTION_PUSH();
static PKPH_OBJECT_TYPE KphpLuaHwRequestType = NULL;
KPH_PROTECTED_DATA_SECTION_POP();
static KSPIN_LOCK KphpLuaHwAbandonLock;
static LIST_ENTRY KphpLuaHwAbandoned;
static PAGED_LOOKASIDE_LIST KphpLuaHwNamedDeviceLookaside = { 0 };
static PAGED_LOOKASIDE_LIST KphpLuaHwDeviceLookaside = { 0 };

/**
 * \brief Deletes a request.
 *
 * \details May run after unload, so it does not trace.
 *
 * \param[in,out] Object The request.
 */
_Function_class_(KPH_TYPE_DELETE_PROCEDURE)
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KSIAPI KphpLuaHwDeleteRequest(
    _Inout_ PVOID Object
    )
{
    PKPH_LUA_HW_REQUEST request;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    request = Object;

    NT_ASSERT(!request->Abandoned);

    if (request->Mdl)
    {
        request->Irp->MdlAddress = NULL;
        IoFreeMdl(request->Mdl);
    }

    IoFreeIrp(request->Irp);
    ObDereferenceObject(request->Device);

    if (request->FileObject)
    {
        ObDereferenceObject(request->FileObject);
    }
}

/**
 * \brief Frees a request.
 *
 * \details May run after unload, so it does not trace.
 *
 * \param[in] Object The request.
 */
_Function_class_(KPH_TYPE_FREE_PROCEDURE)
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KSIAPI KphpLuaHwFreeRequest(
    _In_freesMem_ PVOID Object
    )
{
    KPH_NPAGED_CODE_DISPATCH_MAX();

    KphFree(Object, KPH_TAG_LUA_HARDWARE_REQUEST);
}

/**
 * \brief Checks for an abandoned request to a device.
 *
 * \param[in] Device The device the request was sent to.
 *
 * \return TRUE if an abandoned request to the device is still pending,
 * FALSE otherwise.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
BOOLEAN KphpLuaHwIsDeviceBusy(
    _In_ PDEVICE_OBJECT Device
    )
{
    KLOCK_QUEUE_HANDLE lockHandle;
    BOOLEAN busy;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    busy = FALSE;

    KeAcquireInStackQueuedSpinLock(&KphpLuaHwAbandonLock, &lockHandle);

    for (PLIST_ENTRY link = KphpLuaHwAbandoned.Flink;
         link != &KphpLuaHwAbandoned;
         link = link->Flink)
    {
        PKPH_LUA_HW_REQUEST request;

        request = CONTAINING_RECORD(link, KPH_LUA_HW_REQUEST, AbandonedLink);

        if (request->Device == Device)
        {
            busy = TRUE;
            break;
        }
    }

    KeReleaseInStackQueuedSpinLock(&lockHandle);

    return busy;
}

/**
 * \brief Leaves a request that did not complete in time to its completion.
 *
 * \details The abandoned list takes a reference, which the completion
 * releases.
 *
 * \param[in] Request The request.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphpLuaHwAbandonRequest(
    _In_ PKPH_LUA_HW_REQUEST Request
    )
{
    KLOCK_QUEUE_HANDLE lockHandle;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    KeAcquireInStackQueuedSpinLock(&KphpLuaHwAbandonLock, &lockHandle);

    if (!Request->Completed)
    {
        KphReferenceObject(Request);
        Request->Abandoned = TRUE;
        InsertTailList(&KphpLuaHwAbandoned, &Request->AbandonedLink);
    }

    KeReleaseInStackQueuedSpinLock(&lockHandle);
}

/**
 * \brief References every abandoned request to cancel at unload.
 *
 * \param[out] Head Receives the referenced requests, chained through
 * CancelLink.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphpLuaHwReferenceAbandoned(
    _Out_ PSINGLE_LIST_ENTRY Head
    )
{
    KLOCK_QUEUE_HANDLE lockHandle;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    Head->Next = NULL;

    KeAcquireInStackQueuedSpinLock(&KphpLuaHwAbandonLock, &lockHandle);

    for (PLIST_ENTRY link = KphpLuaHwAbandoned.Flink;
         link != &KphpLuaHwAbandoned;
         link = link->Flink)
    {
        PKPH_LUA_HW_REQUEST request;

        request = CONTAINING_RECORD(link, KPH_LUA_HW_REQUEST, AbandonedLink);

        KphReferenceObject(request);
        PushEntryList(Head, &request->CancelLink);
    }

    KeReleaseInStackQueuedSpinLock(&lockHandle);
}

/**
 * \brief Completes a request.
 *
 * \details Releases the reference of the abandoned list, if the request is on
 * it, then the reference of the completion. The IRP is freed with the request,
 * never completed further. May run after unload, so it does not trace.
 *
 * \param[in] DeviceObject Unused.
 * \param[in] Irp The IRP of the request.
 * \param[in] Context The request.
 *
 * \return STATUS_MORE_PROCESSING_REQUIRED
 */
_Function_class_(IO_COMPLETION_ROUTINE)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS KphpLuaHwRequestComplete(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context
    )
{
    PKPH_LUA_HW_REQUEST request;
    KLOCK_QUEUE_HANDLE lockHandle;
    BOOLEAN abandoned;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    UNREFERENCED_PARAMETER(DeviceObject);

    request = Context;

    NT_ASSERT(request && (request->Irp == Irp));

    request->IoStatus = Irp->IoStatus;

    KeSetEvent(&request->Event, IO_NO_INCREMENT, FALSE);

    KeAcquireInStackQueuedSpinLock(&KphpLuaHwAbandonLock, &lockHandle);

    request->Completed = TRUE;

    abandoned = request->Abandoned;
    if (abandoned)
    {
        RemoveEntryList(&request->AbandonedLink);
        request->Abandoned = FALSE;
    }

    KeReleaseInStackQueuedSpinLock(&lockHandle);

    if (abandoned)
    {
        KphDereferenceObject(request);
    }

    KphDereferenceObject(request);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

KPH_PAGED_FILE();

/**
 * \brief Allocates a request.
 *
 * \param[in] Size The size to allocate.
 *
 * \return The allocation, NULL on failure.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Return_allocatesMem_size_(Size)
PVOID KSIAPI KphpLuaHwAllocateRequest(
    _In_ SIZE_T Size
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphAllocateNPaged(Size, KPH_TAG_LUA_HARDWARE_REQUEST);
}

/**
 * \brief Initializes a request.
 *
 * \param[in,out] Object The request.
 * \param[in] Parameter The KPH_LUA_HW_REQUEST_INIT of the request.
 *
 * \return Successful or errant status.
 */
_Function_class_(KPH_TYPE_INITIALIZE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpLuaHwInitializeRequest(
    _Inout_ PVOID Object,
    _In_opt_ PVOID Parameter
    )
{
    PKPH_LUA_HW_REQUEST request;
    PKPH_LUA_HW_REQUEST_INIT init;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Parameter);

    request = Object;
    init = Parameter;

    request->Irp = IoAllocateIrp(init->Device->StackSize, FALSE);
    if (!request->Irp)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "IoAllocateIrp failed");

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ObReferenceObject(init->Device);
    request->Device = init->Device;

    if (init->FileObject)
    {
        ObReferenceObject(init->FileObject);
        request->FileObject = init->FileObject;
    }

    InitializeListHead(&request->AbandonedLink);
    KeInitializeEvent(&request->Event, NotificationEvent, FALSE);
    request->Input = Add2Ptr(request, init->InputOffset);
    request->Data = Add2Ptr(request, init->DataOffset);
    request->Buffers = Add2Ptr(request, init->BuffersOffset);

    return STATUS_SUCCESS;
}

/**
 * \brief Releases a request.
 *
 * \param[in] Context The request.
 */
_Function_class_(KPH_SCRIPT_CLEANUP_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpLuaHwReleaseRequest(
    _In_opt_ PVOID Context
    )
{
    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    KphDereferenceObject(Context);
}

/**
 * \brief Gives the transfer method of a request.
 *
 * \param[in] Device The device the request goes to.
 * \param[in] MajorFunction The major function of the request.
 * \param[in] IoControlCode The control code of a device control request.
 *
 * \return The METHOD_ of the control code, or of the device's flags for a read
 * or write.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG KphpLuaHwTransferMethod(
    _In_ PDEVICE_OBJECT Device,
    _In_ UCHAR MajorFunction,
    _In_ ULONG IoControlCode
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if ((MajorFunction == IRP_MJ_DEVICE_CONTROL) ||
        (MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL))
    {
        return METHOD_FROM_CTL_CODE(IoControlCode);
    }

    if (BooleanFlagOn(Device->Flags, DO_BUFFERED_IO))
    {
        return METHOD_BUFFERED;
    }

    if (BooleanFlagOn(Device->Flags, DO_DIRECT_IO))
    {
        return METHOD_OUT_DIRECT;
    }

    return METHOD_NEITHER;
}

/**
 * \brief Creates a request with its buffers.
 *
 * \param[in] Device The device to send the request to.
 * \param[in] FileObject Optional file object of the request.
 * \param[in] Method The transfer method of the request.
 * \param[in] InputLength The number of input bytes.
 * \param[in] DataLength The number of data bytes.
 * \param[in] BuffersLength The number of bytes of the referred buffers.
 * \param[out] Request Receives the request, its buffers zeroed.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpLuaHwCreateRequest(
    _In_ PDEVICE_OBJECT Device,
    _In_opt_ PFILE_OBJECT FileObject,
    _In_ ULONG Method,
    _In_ ULONG InputLength,
    _In_ ULONG DataLength,
    _In_ ULONG BuffersLength,
    _Outptr_result_nullonfailure_ PKPH_LUA_HW_REQUEST* Request
    )
{
    NTSTATUS status;
    KPH_LUA_HW_REQUEST_INIT init;
    ULONG inputSpan;
    ULONG dataSpan;
    ULONG dataEnd;
    ULONG allocationLength;

    KPH_PAGED_CODE_PASSIVE();

    *Request = NULL;

    if (KphpLuaHwIsDeviceBusy(Device))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Device busy: an abandoned request is pending");

        return STATUS_DEVICE_BUSY;
    }

    init.Device = Device;
    init.FileObject = FileObject;
    init.InputOffset = (ULONG)ALIGN_UP_BY(sizeof(KPH_LUA_HW_REQUEST),
                                          MEMORY_ALLOCATION_ALIGNMENT);

    if (Method == METHOD_BUFFERED)
    {
        inputSpan = 0;
        dataSpan = max(InputLength, DataLength);
    }
    else
    {
        status = RtlULongAdd(InputLength,
                             MEMORY_ALLOCATION_ALIGNMENT - 1,
                             &inputSpan);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "RtlULongAdd failed: %!STATUS!",
                          status);

            return status;
        }

        inputSpan = ALIGN_DOWN_BY(inputSpan, MEMORY_ALLOCATION_ALIGNMENT);
        dataSpan = DataLength;
    }

    status = RtlULongAdd(init.InputOffset, inputSpan, &init.DataOffset);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        return status;
    }

    status = RtlULongAdd(init.DataOffset, dataSpan, &dataEnd);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        return status;
    }

    status = RtlULongAdd(dataEnd,
                         MEMORY_ALLOCATION_ALIGNMENT - 1,
                         &init.BuffersOffset);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        return status;
    }

    init.BuffersOffset = ALIGN_DOWN_BY(init.BuffersOffset,
                                       MEMORY_ALLOCATION_ALIGNMENT);

    status = RtlULongAdd(init.BuffersOffset,
                         BuffersLength,
                         &allocationLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        return status;
    }

    status = KphCreateObject(KphpLuaHwRequestType,
                             allocationLength,
                             Request,
                             &init);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphCreateObject failed: %!STATUS!",
                      status);
    }

    return status;
}

/**
 * \brief Sets up the request packet of a request.
 *
 * \param[in,out] Request The request.
 * \param[in] MajorFunction The major function of the request.
 * \param[in] IoControlCode The control code of a device control request.
 * \param[in] Offset The byte offset of a read or write.
 * \param[in] InputLength The number of input bytes.
 * \param[in] DataLength The number of data bytes.
 * \param[in] InputArgument Whether Argument1 is the input's address.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpLuaHwPrepareRequest(
    _Inout_ PKPH_LUA_HW_REQUEST Request,
    _In_ UCHAR MajorFunction,
    _In_ ULONG IoControlCode,
    _In_ LONGLONG Offset,
    _In_ ULONG InputLength,
    _In_ ULONG DataLength,
    _In_ BOOLEAN InputArgument
    )
{
    PIRP irp;
    PIO_STACK_LOCATION stack;
    ULONG method;

    KPH_PAGED_CODE_PASSIVE();

    irp = Request->Irp;
    method = KphpLuaHwTransferMethod(Request->Device,
                                     MajorFunction,
                                     IoControlCode);

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;

    stack = IoGetNextIrpStackLocation(irp);
    stack->MajorFunction = MajorFunction;
    stack->FileObject = Request->FileObject;

    if (method == METHOD_BUFFERED)
    {
        irp->AssociatedIrp.SystemBuffer = Request->Input;
    }
    else if (method == METHOD_NEITHER)
    {
        irp->UserBuffer = (DataLength ? Request->Data : NULL);
    }
    else
    {
        irp->AssociatedIrp.SystemBuffer = (InputLength ? Request->Input : NULL);

        if (DataLength)
        {
            Request->Mdl = IoAllocateMdl(Request->Data,
                                         DataLength,
                                         FALSE,
                                         FALSE,
                                         NULL);
            if (!Request->Mdl)
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              HARDWARE,
                              "IoAllocateMdl failed");

                return STATUS_INSUFFICIENT_RESOURCES;
            }

            MmBuildMdlForNonPagedPool(Request->Mdl);
            irp->MdlAddress = Request->Mdl;
        }
    }

    if (MajorFunction == IRP_MJ_READ)
    {
        stack->Parameters.Read.Length = DataLength;
        stack->Parameters.Read.ByteOffset.QuadPart = Offset;
    }
    else if (MajorFunction == IRP_MJ_WRITE)
    {
        stack->Parameters.Write.Length = DataLength;
        stack->Parameters.Write.ByteOffset.QuadPart = Offset;
    }
    else
    {
        stack->Parameters.DeviceIoControl.IoControlCode = IoControlCode;
        stack->Parameters.DeviceIoControl.InputBufferLength = InputLength;
        stack->Parameters.DeviceIoControl.OutputBufferLength = DataLength;

        if ((method == METHOD_NEITHER) && InputLength)
        {
            stack->Parameters.DeviceIoControl.Type3InputBuffer = Request->Input;
        }
        else
        {
            stack->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
        }

        if (InputArgument)
        {
            stack->Parameters.Others.Argument1 = Request->Input;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Sends a prepared request; its completion signals the request's event.
 *
 * \param[in,out] Request The request; the completion holds a reference until
 * it runs.
 * \param[out] MajorFunction Receives the major function of the request.
 * \param[out] IoControlCode Receives the control code, 0 for a read or write.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpLuaHwStartRequest(
    _Inout_ PKPH_LUA_HW_REQUEST Request,
    _Out_ PUCHAR MajorFunction,
    _Out_ PULONG IoControlCode
    )
{
    NTSTATUS status;
    PIO_STACK_LOCATION stack;

    KPH_PAGED_CODE_PASSIVE();

    stack = IoGetNextIrpStackLocation(Request->Irp);
    *MajorFunction = stack->MajorFunction;

    if ((*MajorFunction == IRP_MJ_DEVICE_CONTROL) ||
        (*MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL))
    {
        *IoControlCode = stack->Parameters.DeviceIoControl.IoControlCode;
    }
    else
    {
        *IoControlCode = 0;
    }

    KsiInitializeIoCompletion(&Request->Completion,
                              KphDriverObject,
                              KphpLuaHwRequestComplete,
                              Request,
                              TRUE,
                              TRUE,
                              TRUE);

    KsiSetCompletionRoutine(Request->Irp, &Request->Completion);

    KphReferenceObject(Request);

    status = IoCallDriver(Request->Device, Request->Irp);

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "Request %lu 0x%08lx IoCallDriver: %!STATUS!",
                  (ULONG)*MajorFunction,
                  *IoControlCode,
                  status);
}

/**
 * \brief Sends a prepared request and waits for it.
 *
 * \details A request that does not complete in time is cancelled, then
 * abandoned or, with Abandon FALSE, waited for.
 *
 * \param[in] State The Lua state.
 * \param[in] Request The request; the caller keeps its reference.
 * \param[in] Milliseconds How long to wait, ULONG_MAX for no limit of its own.
 * \param[in] Abandon FALSE to wait for the request to complete.
 * \param[out] LimitReached Optionally receives TRUE when the call's limit was
 * reached while a request that may not be abandoned was waited for.
 * \param[out] Completed Receives TRUE when the request completed and its status
 * is returned, FALSE when the wait's status is returned.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpLuaHwCallRequest(
    _In_ lua_State* State,
    _In_ PKPH_LUA_HW_REQUEST Request,
    _In_ ULONG Milliseconds,
    _In_ BOOLEAN Abandon,
    _Out_opt_ PBOOLEAN LimitReached,
    _Out_ PBOOLEAN Completed
    )
{
    NTSTATUS status;
    UCHAR majorFunction;
    ULONG ioControlCode;
    LARGE_INTEGER zero;
    BOOLEAN cancelled;
    NTSTATUS waitStatus;

    KPH_PAGED_CODE_PASSIVE();

    *Completed = FALSE;

    if (LimitReached)
    {
        *LimitReached = FALSE;
    }

    KphpLuaHwStartRequest(Request, &majorFunction, &ioControlCode);

    status = KphScriptWait(State, &Request->Event, Milliseconds);
    if (status == STATUS_SUCCESS)
    {
        goto Completed;
    }

    if (LimitReached)
    {
        *LimitReached = (status == STATUS_SI_SCRIPT_LIMIT);
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "KphScriptWait returned: %!STATUS!",
                  status);

    cancelled = IoCancelIrp(Request->Irp);

    if (!Abandon)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Request %lu 0x%08lx waited for (cancelled %!bool!)",
                      (ULONG)majorFunction,
                      ioControlCode,
                      cancelled);

        waitStatus = KeWaitForSingleObject(&Request->Event,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           NULL);
        NT_ASSERT(waitStatus == STATUS_SUCCESS);

        goto Completed;
    }

    zero.QuadPart = 0;

    waitStatus = KeWaitForSingleObject(&Request->Event,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       &zero);
    if (waitStatus != STATUS_SUCCESS)
    {
        KphpLuaHwAbandonRequest(Request);

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Request %lu 0x%08lx abandoned (cancelled %!bool!)",
                      (ULONG)majorFunction,
                      ioControlCode,
                      cancelled);

        return status;
    }

    if (status == STATUS_SI_SCRIPT_LIMIT)
    {
        return status;
    }

Completed:

    *Completed = TRUE;

    status = Request->IoStatus.Status;
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Request %lu 0x%08lx failed: %!STATUS!",
                      (ULONG)majorFunction,
                      ioControlCode,
                      status);
    }

    return status;
}

/**
 * \brief Gives the first stack slot of a referred buffer.
 *
 * \param[in] Buffers The buffers.
 * \param[in] Number The number of the buffer, from 1.
 *
 * \return The stack index.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwBufferSlot(
    _In_ PKPH_LUA_HW_BUFFERS Buffers,
    _In_ ULONG Number
    )
{
    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT((Number >= 1) && (Number <= Buffers->Count));

    return Buffers->Index + (int)((Number - 1) * 5);
}

/**
 * \brief Gives the length of a referred buffer.
 *
 * \param[in] State The Lua state.
 * \param[in] Buffers The buffers.
 * \param[in] Number The number of the buffer, from 1.
 *
 * \return The length of the buffer.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG KphpLuaHwBufferLength(
    _In_ lua_State* State,
    _In_ PKPH_LUA_HW_BUFFERS Buffers,
    _In_ ULONG Number
    )
{
    int slot;

    KPH_PAGED_CODE_PASSIVE();

    slot = KphpLuaHwBufferSlot(Buffers, Number);

    if (lua_type(State, slot) == LUA_TSTRING)
    {
        return (ULONG)lua_rawlen(State, slot);
    }

    return (ULONG)lua_tointeger(State, slot);
}

/**
 * \brief Gives the address of a referred buffer.
 *
 * \param[in] State The Lua state.
 * \param[in] Request The request.
 * \param[in] Buffers The buffers.
 * \param[in] Number The number of the buffer, from 1; 0 for the input.
 *
 * \return The address in the request.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PVOID KphpLuaHwBufferAddress(
    _In_ lua_State* State,
    _In_ PKPH_LUA_HW_REQUEST Request,
    _In_ PKPH_LUA_HW_BUFFERS Buffers,
    _In_ ULONG Number
    )
{
    int slot;

    KPH_PAGED_CODE_PASSIVE();

    if (Number == 0)
    {
        return Request->Input;
    }

    slot = KphpLuaHwBufferSlot(Buffers, Number);

    return Add2Ptr(Request->Buffers, (ULONG)lua_tointeger(State, slot + 3));
}

/**
 * \brief Checks the buffers a device control request's input refers to.
 *
 * \details May raise a Lua error. Each is { bytes | length, at, [into],
 * [length_at] }; its values are left on the stack, out of the script's reach.
 *
 * \param[in] State The Lua state.
 * \param[in] Table The index of the list of buffers.
 * \param[in] InputLength The number of input bytes.
 * \param[out] Buffers Receives the buffers.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpLuaHwCheckBuffers(
    _In_ lua_State* State,
    _In_ int Table,
    _In_ ULONG InputLength,
    _Out_ PKPH_LUA_HW_BUFFERS Buffers
    )
{
    NTSTATUS status;
    lua_Integer count;
    ULONG length;

    KPH_PAGED_CODE_PASSIVE();

    Buffers->Index = 0;
    Buffers->Count = 0;
    Buffers->Length = 0;

    count = (lua_Integer)lua_rawlen(State, Table);

    luaL_argcheck(State, count <= ((INT_MAX - 16) / 5), 6, "too many buffers");
    luaL_checkstack(State, ((int)count * 5) + 16, "too many buffers");

    Buffers->Index = lua_gettop(State) + 1;
    length = 0;

    for (lua_Integer i = 1; i <= count; i++)
    {
        int entry;
        BOOLEAN bytes;
        ULONG span;
        lua_Integer at;
        lua_Integer into;
        lua_Integer lengthAt;
        int source;

        lua_rawgeti(State, Table, i);
        entry = lua_gettop(State);

        luaL_argcheck(State,
                      lua_type(State, entry) == LUA_TTABLE,
                      6,
                      "a list of buffer tables");

        lua_getfield(State, entry, "bytes");
        lua_getfield(State, entry, "length");
        lua_getfield(State, entry, "at");
        lua_getfield(State, entry, "into");
        lua_getfield(State, entry, "length_at");

        bytes = (lua_type(State, entry + 1) != LUA_TNIL);

        luaL_argcheck(State,
                      bytes != (lua_type(State, entry + 2) != LUA_TNIL),
                      6,
                      "a buffer with one of bytes or length");

        if (bytes)
        {
            SIZE_T size;

            luaL_argcheck(State,
                          lua_type(State, entry + 1) == LUA_TSTRING,
                          6,
                          "bytes of a string");

            lua_tolstring(State, entry + 1, &size);

            luaL_argcheck(State, size <= ULONG_MAX, 6, "bytes too long");

            span = (ULONG)size;
        }
        else
        {
            lua_Integer value;

            luaL_argcheck(State,
                          lua_isinteger(State, entry + 2),
                          6,
                          "a length of an integer");

            value = lua_tointeger(State, entry + 2);

            luaL_argcheck(State,
                          (value >= 0) && (value <= ULONG_MAX),
                          6,
                          "a length out of range");

            span = (ULONG)value;
        }

        luaL_argcheck(State,
                      lua_isinteger(State, entry + 3),
                      6,
                      "an at of an integer");

        at = lua_tointeger(State, entry + 3);

        luaL_argcheck(State,
                      (at >= 0) &&
                      (at <= ULONG_MAX) &&
                      ((at % sizeof(PVOID)) == 0),
                      6,
                      "an at out of range or not pointer-aligned");

        if (lua_isnil(State, entry + 4))
        {
            into = 0;
        }
        else
        {
            luaL_argcheck(State,
                          lua_isinteger(State, entry + 4),
                          6,
                          "an into of an integer");

            into = lua_tointeger(State, entry + 4);

            luaL_argcheck(State,
                          (into >= 1) && (into <= count),
                          6,
                          "an into of no buffer");
        }

        if (lua_isnil(State, entry + 5))
        {
            lengthAt = -1;
        }
        else
        {
            luaL_argcheck(State,
                          lua_isinteger(State, entry + 5),
                          6,
                          "a length_at of an integer");

            lengthAt = lua_tointeger(State, entry + 5);

            luaL_argcheck(State,
                          (lengthAt >= 0) &&
                          (lengthAt <= ULONG_MAX) &&
                          ((lengthAt % sizeof(ULONG)) == 0),
                          6,
                          "a length_at out of range or not ULONG-aligned");
            luaL_argcheck(State,
                          ((lengthAt + (lua_Integer)sizeof(ULONG)) <= at) ||
                          (lengthAt >= (at + (lua_Integer)sizeof(PVOID))),
                          6,
                          "a length_at over its at");
        }

        if (bytes)
        {
            source = entry + 1;
        }
        else
        {
            source = entry + 2;
        }

        lua_pushvalue(State, source);
        lua_pushinteger(State, at);
        lua_pushinteger(State, into);
        lua_pushinteger(State, length);
        lua_pushinteger(State, lengthAt);

        status = RtlULongAdd(span, MEMORY_ALLOCATION_ALIGNMENT - 1, &span);
        if (NT_SUCCESS(status))
        {
            span = ALIGN_DOWN_BY(span, MEMORY_ALLOCATION_ALIGNMENT);

            status = RtlULongAdd(length, span, &length);
        }

        luaL_argcheck(State, NT_SUCCESS(status), 6, "buffers too long");

        //
        // The five values take the place of the entry and its fields.
        //
        lua_rotate(State, entry, 5);
        lua_settop(State, entry + 4);
    }

    Buffers->Count = (ULONG)count;

    for (ULONG i = 0; i < Buffers->Count; i++)
    {
        int slot;
        lua_Integer at;
        ULONG into;
        ULONG targetLength;
        lua_Integer lengthAt;

        slot = KphpLuaHwBufferSlot(Buffers, i + 1);
        at = lua_tointeger(State, slot + 1);
        into = (ULONG)lua_tointeger(State, slot + 2);

        if (into == 0)
        {
            targetLength = InputLength;
        }
        else
        {
            targetLength = KphpLuaHwBufferLength(State, Buffers, into);
        }

        luaL_argcheck(State,
                      (at + (lua_Integer)sizeof(PVOID)) <= targetLength,
                      6,
                      "an at past the end of its target");

        lengthAt = lua_tointeger(State, slot + 4);

        luaL_argcheck(State,
                      (lengthAt < 0) ||
                      ((lengthAt + (lua_Integer)sizeof(ULONG)) <= targetLength),
                      6,
                      "a length_at past the end of its target");
    }

    Buffers->Length = length;
}

/**
 * \brief Writes the referred buffers' bytes, addresses and lengths.
 *
 * \param[in] State The Lua state.
 * \param[in] Request The request.
 * \param[in] Buffers The buffers.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpLuaHwLayBuffers(
    _In_ lua_State* State,
    _In_ PKPH_LUA_HW_REQUEST Request,
    _In_ PKPH_LUA_HW_BUFFERS Buffers
    )
{
    KPH_PAGED_CODE_PASSIVE();

    //
    // Every buffer's bytes first: an address may go into a later buffer.
    //
    for (ULONG i = 0; i < Buffers->Count; i++)
    {
        int slot;

        slot = KphpLuaHwBufferSlot(Buffers, i + 1);

        if (lua_type(State, slot) == LUA_TSTRING)
        {
            PCSTR bytes;
            SIZE_T length;

            bytes = lua_tolstring(State, slot, &length);

            RtlCopyMemory(
                KphpLuaHwBufferAddress(State, Request, Buffers, i + 1),
                bytes,
                length
                );
        }
    }

    for (ULONG i = 0; i < Buffers->Count; i++)
    {
        int slot;
        PVOID address;
        PVOID target;

        slot = KphpLuaHwBufferSlot(Buffers, i + 1);
        address = KphpLuaHwBufferAddress(State, Request, Buffers, i + 1);
        target = KphpLuaHwBufferAddress(State,
                                        Request,
                                        Buffers,
                                        (ULONG)lua_tointeger(State, slot + 2));

        RtlCopyMemory(Add2Ptr(target, (ULONG)lua_tointeger(State, slot + 1)),
                      &address,
                      sizeof(PVOID));

        if (lua_tointeger(State, slot + 4) >= 0)
        {
            ULONG length;

            length = KphpLuaHwBufferLength(State, Buffers, i + 1);

            RtlCopyMemory(
                Add2Ptr(target, (ULONG)lua_tointeger(State, slot + 4)),
                &length,
                sizeof(ULONG)
                );
        }
    }
}

/**
 * \brief Creates and prepares a request.
 *
 * \param[in] State The Lua state.
 * \param[in] Device The device to send the request to.
 * \param[in] FileObject Optional file object of the request.
 * \param[in] Spec What to send.
 * \param[out] Request Receives the request for the caller to release.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpLuaHwBuildRequest(
    _In_ lua_State* State,
    _In_ PDEVICE_OBJECT Device,
    _In_opt_ PFILE_OBJECT FileObject,
    _In_ PCKPH_LUA_HW_REQUEST_SPEC Spec,
    _Outptr_result_nullonfailure_ PKPH_LUA_HW_REQUEST* Request
    )
{
    NTSTATUS status;
    ULONG method;
    ULONG buffersLength;
    PKPH_LUA_HW_REQUEST request;

    KPH_PAGED_CODE_PASSIVE();

    *Request = NULL;

    method = KphpLuaHwTransferMethod(Device,
                                     Spec->MajorFunction,
                                     Spec->IoControlCode);

    if (Spec->Buffers)
    {
        buffersLength = Spec->Buffers->Length;
    }
    else
    {
        buffersLength = 0;
    }

    status = KphpLuaHwCreateRequest(Device,
                                    FileObject,
                                    method,
                                    Spec->InputLength,
                                    Spec->DataLength,
                                    buffersLength,
                                    &request);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwCreateRequest failed: %!STATUS!",
                      status);

        return status;
    }

    if (Spec->Input)
    {
        RtlCopyMemory(request->Input, Spec->Input, Spec->InputLength);
    }

    if (Spec->Data && (method != METHOD_BUFFERED))
    {
        RtlCopyMemory(request->Data, Spec->Data, Spec->DataLength);
    }

    if (Spec->Buffers)
    {
        KphpLuaHwLayBuffers(State, request, Spec->Buffers);
    }

    status = KphpLuaHwPrepareRequest(request,
                                     Spec->MajorFunction,
                                     Spec->IoControlCode,
                                     Spec->Offset,
                                     Spec->InputLength,
                                     Spec->DataLength,
                                     Spec->InputArgument);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwPrepareRequest failed: %!STATUS!",
                      status);

        KphDereferenceObject(request);
        return status;
    }

    *Request = request;

    return STATUS_SUCCESS;
}

/**
 * \brief Pushes a list of the referred buffers' bytes.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 * \param[in] Request The completed request.
 * \param[in] Buffers The buffers.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpLuaHwPushBuffers(
    _In_ lua_State* State,
    _In_ PKPH_LUA_HW_REQUEST Request,
    _In_ PKPH_LUA_HW_BUFFERS Buffers
    )
{
    KPH_PAGED_CODE_PASSIVE();

    lua_createtable(State, (int)Buffers->Count, 0);

    for (ULONG i = 0; i < Buffers->Count; i++)
    {
        lua_pushlstring(State,
                        KphpLuaHwBufferAddress(State, Request, Buffers, i + 1),
                        KphpLuaHwBufferLength(State, Buffers, i + 1));
        lua_rawseti(State, -2, (lua_Integer)i + 1);
    }
}

/**
 * \brief Pushes the result of a request.
 *
 * \details May raise a Lua error. Releases the request either way.
 *
 * \param[in] State The Lua state.
 * \param[in] Status The status of the request.
 * \param[in] Completed TRUE when the request completed.
 * \param[in] Request Optional request.
 * \param[in] DataLength The number of data bytes.
 * \param[in] Buffers Optional referred buffers.
 * \param[in] Count TRUE to push the reported count rather than the data.
 *
 * \return The number of results.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwPushRequest(
    _In_ lua_State* State,
    _In_ NTSTATUS Status,
    _In_ BOOLEAN Completed,
    _In_opt_ PKPH_LUA_HW_REQUEST Request,
    _In_ ULONG DataLength,
    _In_opt_ PKPH_LUA_HW_BUFFERS Buffers,
    _In_ BOOLEAN Count
    )
{
    int results;
    int cleanup;
    ULONG information;

    KPH_PAGED_CODE_PASSIVE();

    if (!Request)
    {
        return KphLuaNilErrorStatus(State, Status);
    }

    KphScriptPushCleanup(State, KphpLuaHwReleaseRequest, Request);
    cleanup = lua_gettop(State);

    if (!Completed && (Status == STATUS_SI_SCRIPT_LIMIT))
    {
        KphScriptCharge(State, 0);
    }

    if (!Completed)
    {
        results = KphLuaNilErrorStatus(State, Status);
    }
    else if (Status == STATUS_BUFFER_OVERFLOW)
    {
        results = KphLuaNilErrorStatus(State, Status);

        //
        // The whole buffer: the ACPI driver writes its header and counts none.
        //
        lua_pushlstring(State, Request->Data, DataLength);
        results++;

        if (Buffers)
        {
            KphpLuaHwPushBuffers(State, Request, Buffers);
            results++;
        }
    }
    else if (!NT_SUCCESS(Status))
    {
        results = KphLuaNilErrorStatus(State, Status);
    }
    else
    {
        information = (ULONG)min(Request->IoStatus.Information, DataLength);

        if (Count)
        {
            lua_pushinteger(State, (lua_Integer)Request->IoStatus.Information);
        }
        else
        {
            lua_pushlstring(State, Request->Data, information);
        }

        results = 1;

        if (Buffers)
        {
            KphpLuaHwPushBuffers(State, Request, Buffers);
            results++;
        }
    }

    KphScriptReleaseCleanup(State, cleanup);
    lua_remove(State, cleanup);

    return results;
}

/**
 * \brief Releases a device object of ksi.hw.device_open.
 *
 * \param[in] Context The device.
 */
_Function_class_(KPH_SCRIPT_CLEANUP_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpLuaHwReleaseNamedDevice(
    _In_opt_ PVOID Context
    )
{
    PKPH_LUA_HW_NAMED_DEVICE device;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    device = Context;

    ObDereferenceObject(device->FileObject);

    KphFreeToPagedLookaside(&KphpLuaHwNamedDeviceLookaside, device);
}

/**
 * \brief Opens a named device (ksi.hw.device_open).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the device object, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwDeviceOpen(
    _In_ lua_State* State
    )
{
    int results;
    NTSTATUS status;
    PCSTR text;
    SIZE_T length;
    lua_Integer access;
    ANSI_STRING ansi;
    UNICODE_STRING name;
    PKPH_LUA_HW_NAMED_DEVICE device;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;

    KPH_PAGED_CODE_PASSIVE();

    text = luaL_checklstring(State, 1, &length);
    access = luaL_optinteger(State, 2, FILE_READ_DATA);

    luaL_argcheck(State,
                  (length >= 1) && (length <= KPH_LUA_HW_DEVICE_NAME_MAX),
                  1,
                  "1 to 32767 characters");
    luaL_argcheck(State,
                  (access >= 0) && (access <= ULONG_MAX),
                  2,
                  "out of range");

    device = KphAllocateFromPagedLookaside(&KphpLuaHwNamedDeviceLookaside);
    if (!device)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate a device object");

        status = STATUS_INSUFFICIENT_RESOURCES;
        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    NT_ASSERT(sizeof(KPH_LUA_HW_NAMED_DEVICE) <=
              KphpLuaHwNamedDeviceLookaside.L.Size);

    ansi.Buffer = (PCHAR)text;
    ansi.Length = (USHORT)length;
    ansi.MaximumLength = (USHORT)length;

    RtlZeroMemory(&name, sizeof(name));

    status = RtlAnsiStringToUnicodeString(&name, &ansi, TRUE);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlAnsiStringToUnicodeString failed: %!STATUS!",
                      status);

        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    status = IoGetDeviceObjectPointer(&name,
                                      (ACCESS_MASK)access,
                                      &fileObject,
                                      &deviceObject);

    RtlFreeUnicodeString(&name);

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoGetDeviceObjectPointer failed: %!STATUS!",
                      status);

        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    device->FileObject = fileObject;

    KphScriptPushObject(State, KphpLuaHwReleaseNamedDevice, device);
    device = NULL;

    results = 1;

Exit:

    if (device)
    {
        KphFreeToPagedLookaside(&KphpLuaHwNamedDeviceLookaside, device);
    }

    return results;
}

/**
 * \brief Releases a device object of ksi.hw.devices.
 *
 * \param[in] Context The device.
 */
_Function_class_(KPH_SCRIPT_CLEANUP_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpLuaHwReleaseDevice(
    _In_opt_ PVOID Context
    )
{
    PKPH_LUA_HW_DEVICE device;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    device = Context;

    ObDereferenceObject(device->Pdo);

    KphFreeToPagedLookaside(&KphpLuaHwDeviceLookaside, device);
}

/**
 * \brief Releases the device objects ksi.hw.devices did not hand out.
 *
 * \param[in] Context The list.
 */
_Function_class_(KPH_SCRIPT_CLEANUP_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpLuaHwReleaseDeviceList(
    _In_opt_ PVOID Context
    )
{
    PKPH_LUA_HW_DEVICE_LIST list;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    list = Context;

    for (ULONG i = 0; i < list->Count; i++)
    {
        if (list->Devices[i])
        {
            ObDereferenceObject(list->Devices[i]);
        }
    }

    if (list->Devices)
    {
        KphFree(list->Devices, KPH_TAG_LUA_HARDWARE_DEVICES);
    }

    KphFree(list, KPH_TAG_LUA_HARDWARE_DEVICE_LIST);
}

/**
 * \brief Tells whether a value is a device object of ksi.hw.devices.
 *
 * \param[in] State The Lua state.
 * \param[in] Index The index of the value.
 *
 * \return The physical device object, valid while the object is, or NULL.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PDEVICE_OBJECT KphLuaHwTestDevice(
    _In_ lua_State* State,
    _In_ int Index
    )
{
    PKPH_LUA_HW_DEVICE device;

    KPH_PAGED_CODE_PASSIVE();

    device = KphScriptTestCleanup(State, Index, KphpLuaHwReleaseDevice);
    if (!device)
    {
        return NULL;
    }

    return device->Pdo;
}

/**
 * \brief Lists the bus-enumerated device objects of a driver
 * (ksi.hw.devices).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the array of device objects, or nil and a
 * status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwDevices(
    _In_ lua_State* State
    )
{
    int results;
    NTSTATUS status;
    PCSTR text;
    SIZE_T length;
    ANSI_STRING ansi;
    UNICODE_STRING name;
    PDRIVER_OBJECT driver;
    PKPH_LUA_HW_DEVICE_LIST list;
    PKPH_LUA_HW_DEVICE device;
    lua_Integer index;
    int cleanup;
    int size;

    KPH_PAGED_CODE_PASSIVE();

    text = luaL_checklstring(State, 1, &length);

    luaL_argcheck(State,
                  (length >= 1) && (length <= KPH_LUA_HW_DEVICE_NAME_MAX),
                  1,
                  "1 to 32767 characters");

    driver = NULL;
    list = NULL;

    ansi.Buffer = (PCHAR)text;
    ansi.Length = (USHORT)length;
    ansi.MaximumLength = (USHORT)length;

    RtlZeroMemory(&name, sizeof(name));

    status = RtlAnsiStringToUnicodeString(&name, &ansi, TRUE);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlAnsiStringToUnicodeString failed: %!STATUS!",
                      status);

        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    status = ObReferenceObjectByName(&name,
                                     OBJ_DONT_REPARSE,
                                     NULL,
                                     0,
                                     *IoDriverObjectType,
                                     KernelMode,
                                     NULL,
                                     (PVOID*)&driver);

    RtlFreeUnicodeString(&name);

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ObReferenceObjectByName failed: %!STATUS!",
                      status);

        driver = NULL;
        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    list = KphAllocatePaged(sizeof(KPH_LUA_HW_DEVICE_LIST),
                            KPH_TAG_LUA_HARDWARE_DEVICE_LIST);
    if (!list)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate a device list");

        status = STATUS_INSUFFICIENT_RESOURCES;
        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    status = KphLuaHwReferenceDriverDevices(driver,
                                            &list->Devices,
                                            &list->Count);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphLuaHwReferenceDriverDevices failed: %!STATUS!",
                      status);

        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    ObDereferenceObject(driver);
    driver = NULL;

    if (!list->Devices)
    {
        KphpLuaHwReleaseDeviceList(list);
        list = NULL;

        lua_newtable(State);

        results = 1;
        goto Exit;
    }

    KphScriptPushCleanup(State, KphpLuaHwReleaseDeviceList, list);
    cleanup = lua_gettop(State);

    KphScriptCharge(State, list->Count);

    NT_VERIFY(NT_SUCCESS(RtlULongToInt(list->Count, &size)));

    lua_createtable(State, size, 0);

    index = 0;

    for (ULONG i = 0; i < list->Count; i++)
    {
        if (!BooleanFlagOn(list->Devices[i]->Flags, DO_BUS_ENUMERATED_DEVICE))
        {
            continue;
        }

        device = KphAllocateFromPagedLookaside(&KphpLuaHwDeviceLookaside);
        if (!device)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Failed to allocate a device object");

            KphScriptReleaseCleanup(State, cleanup);
            list = NULL;

            status = STATUS_INSUFFICIENT_RESOURCES;
            results = KphLuaNilErrorStatus(State, status);
            goto Exit;
        }

        NT_ASSERT(sizeof(KPH_LUA_HW_DEVICE) <=
                  KphpLuaHwDeviceLookaside.L.Size);

        device->Pdo = list->Devices[i];
        list->Devices[i] = NULL;

        KphScriptPushObject(State, KphpLuaHwReleaseDevice, device);

        lua_rawseti(State, -2, ++index);
    }

    KphScriptReleaseCleanup(State, cleanup);
    lua_remove(State, cleanup);
    list = NULL;

    results = 1;

Exit:

    if (driver)
    {
        ObDereferenceObject(driver);
    }

    if (list)
    {
        KphpLuaHwReleaseDeviceList(list);
    }

    return results;
}

/**
 * \brief Checks a device argument of a request: a named, device or PCI device
 * object.
 *
 * \details May raise a Lua error. A named device takes a request at the top of
 * its stack; a device or PCI device object takes it itself, safe only while
 * the script holds an open of it or its driver copes with a removed device.
 *
 * \param[in] State The Lua state.
 * \param[in] Index The index of the argument.
 * \param[out] FileObject Receives the file object of a named device, NULL
 * otherwise.
 *
 * \return The device to send the request to, valid while the argument is.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PDEVICE_OBJECT KphpLuaHwCheckTarget(
    _In_ lua_State* State,
    _In_ int Index,
    _Out_ PFILE_OBJECT* FileObject
    )
{
    PKPH_LUA_HW_NAMED_DEVICE named;
    PDEVICE_OBJECT pdo;
    PDEVICE_OBJECT pci;

    KPH_PAGED_CODE_PASSIVE();

    *FileObject = NULL;

    named = KphScriptTestCleanup(State, Index, KphpLuaHwReleaseNamedDevice);
    pdo = KphLuaHwTestDevice(State, Index);
    pci = KphLuaHwPciTestDevice(State, Index);

    luaL_argcheck(State,
                  named || pdo || pci,
                  Index,
                  "a named, device or PCI device object");

    if (named)
    {
        *FileObject = named->FileObject;
        return IoGetRelatedDeviceObject(named->FileObject);
    }

    return (pdo ? pdo : pci);
}

/**
 * \brief Checks a milliseconds argument of a request.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 * \param[in] Index The index of the argument.
 *
 * \return The milliseconds.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG KphpLuaHwCheckMilliseconds(
    _In_ lua_State* State,
    _In_ int Index
    )
{
    lua_Integer milliseconds;

    KPH_PAGED_CODE_PASSIVE();

    milliseconds = luaL_checkinteger(State, Index);

    luaL_argcheck(State,
                  (milliseconds >= 1) && (milliseconds < ULONG_MAX),
                  Index,
                  "out of range");

    return (ULONG)milliseconds;
}

/**
 * \brief Sends the give-back of a claimed guard and frees the guard.
 *
 * \param[in] Context The guard.
 */
_Function_class_(KPH_SCRIPT_CLEANUP_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphpLuaHwReleaseGuard(
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PKPH_LUA_HW_GUARD guard;
    UCHAR majorFunction;
    ULONG ioControlCode;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    guard = Context;

    if (guard->Claimed)
    {
        KphpLuaHwStartRequest(guard->Request, &majorFunction, &ioControlCode);

        status = KeWaitForSingleObject(&guard->Request->Event,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);
        NT_ASSERT(status == STATUS_SUCCESS);

        status = guard->Request->IoStatus.Status;
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Guard 0x%08lx give-back failed: %!STATUS!",
                          guard->IoControlCode,
                          status);
        }
    }

    KphDereferenceObject(guard->Request);

    KphFree(guard, KPH_TAG_LUA_HARDWARE_GUARD);
}

/**
 * \brief Checks a request table of ksi.hw.device_guard: its code and input.
 *
 * \details May raise a Lua error. Leaves the fields it reads on the stack.
 *
 * \param[in] State The Lua state.
 * \param[in] Index The index of the table.
 * \param[in] InputMax The most input bytes.
 * \param[out] IoControlCode Receives the control code.
 * \param[out] Input Receives the input.
 * \param[out] InputLength Receives the number of input bytes.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpLuaHwCheckGuardRequest(
    _In_ lua_State* State,
    _In_ int Index,
    _In_ ULONG InputMax,
    _Out_ PULONG IoControlCode,
    _Out_ PCSTR* Input,
    _Out_ PULONG InputLength
    )
{
    lua_Integer code;
    SIZE_T inputLength;

    KPH_PAGED_CODE_PASSIVE();

    *IoControlCode = 0;
    *Input = NULL;
    *InputLength = 0;

    luaL_checktype(State, Index, LUA_TTABLE);

    lua_getfield(State, Index, "code");
    luaL_argcheck(State,
                  lua_isinteger(State, -1),
                  Index,
                  "a code of an integer");
    code = lua_tointeger(State, -1);
    luaL_argcheck(State,
                  (code >= 0) && (code <= ULONG_MAX),
                  Index,
                  "a code out of range");

    lua_getfield(State, Index, "input");
    luaL_argcheck(State,
                  lua_type(State, -1) == LUA_TSTRING,
                  Index,
                  "an input of a string");
    *Input = lua_tolstring(State, -1, &inputLength);
    luaL_argcheck(State, inputLength <= InputMax, Index, "an input too long");

    *IoControlCode = (ULONG)code;
    *InputLength = (ULONG)inputLength;
}

/**
 * \brief Takes ownership through a device control request and returns the
 * guard that gives it back (ksi.hw.device_guard).
 *
 * \details May raise a Lua error. Takes the device, the take request { code,
 * input, output }, the give-back request { code, input, from, to, length } and
 * the milliseconds; the take's output [from, from + length) is copied into the
 * give-back input at to.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the guard, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwDeviceGuard(
    _In_ lua_State* State
    )
{
    NTSTATUS status;
    PDEVICE_OBJECT target;
    PFILE_OBJECT fileObject;
    ULONG takeCode;
    PCSTR takeInput;
    ULONG takeInputLength;
    lua_Integer outputLength;
    ULONG giveCode;
    PCSTR giveInput;
    ULONG giveInputLength;
    lua_Integer from;
    lua_Integer to;
    lua_Integer length;
    ULONG milliseconds;
    KPH_LUA_HW_REQUEST_SPEC spec;
    PKPH_LUA_HW_REQUEST give;
    PKPH_LUA_HW_GUARD guard;
    int guardIndex;
    BOOLEAN limitReached;
    BOOLEAN completed;
    PKPH_LUA_HW_REQUEST request;

    KPH_PAGED_CODE_PASSIVE();

    milliseconds = KphpLuaHwCheckMilliseconds(State, 4);

    KphpLuaHwCheckGuardRequest(State,
                               2,
                               ULONG_MAX,
                               &takeCode,
                               &takeInput,
                               &takeInputLength);

    lua_getfield(State, 2, "output");

    outputLength = 0;

    if (!lua_isnil(State, -1))
    {
        luaL_argcheck(State,
                      lua_isinteger(State, -1),
                      2,
                      "an output of an integer");
        outputLength = lua_tointeger(State, -1);
        luaL_argcheck(State,
                      (outputLength >= 0) && (outputLength <= ULONG_MAX),
                      2,
                      "an output out of range");
    }

    KphpLuaHwCheckGuardRequest(State,
                               3,
                               KPH_LUA_HW_GUARD_INPUT_MAX,
                               &giveCode,
                               &giveInput,
                               &giveInputLength);

    lua_getfield(State, 3, "from");
    lua_getfield(State, 3, "to");
    lua_getfield(State, 3, "length");

    from = 0;
    to = 0;
    length = 0;

    if (!lua_isnil(State, -3) || !lua_isnil(State, -2) || !lua_isnil(State, -1))
    {
        luaL_argcheck(State,
                      lua_isinteger(State, -3) &&
                      lua_isinteger(State, -2) &&
                      lua_isinteger(State, -1),
                      3,
                      "a splice of integers");

        from = lua_tointeger(State, -3);
        to = lua_tointeger(State, -2);
        length = lua_tointeger(State, -1);

        luaL_argcheck(State,
                      (from >= 0) && (to >= 0) && (length >= 1) &&
                      (from <= outputLength) &&
                      (length <= outputLength - from) &&
                      (to <= (lua_Integer)giveInputLength) &&
                      (length <= (lua_Integer)giveInputLength - to),
                      3,
                      "a splice out of range");
    }

    RtlZeroMemory(&spec, sizeof(spec));
    spec.MajorFunction = IRP_MJ_DEVICE_CONTROL;
    spec.IoControlCode = giveCode;
    spec.Input = giveInput;
    spec.InputLength = giveInputLength;

    target = KphpLuaHwCheckTarget(State, 1, &fileObject);

    status = KphpLuaHwBuildRequest(State, target, fileObject, &spec, &give);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwBuildRequest failed: %!STATUS!",
                      status);

        return KphLuaNilErrorStatus(State, status);
    }

    guard = KphAllocatePaged(sizeof(KPH_LUA_HW_GUARD),
                             KPH_TAG_LUA_HARDWARE_GUARD);
    if (!guard)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate a guard");

        KphDereferenceObject(give);
        return KphLuaNilErrorStatus(State, STATUS_INSUFFICIENT_RESOURCES);
    }

    guard->Request = give;
    guard->IoControlCode = giveCode;
    guard->Claimed = FALSE;

    KphScriptPushCleanup(State, KphpLuaHwReleaseGuard, guard);
    guardIndex = lua_gettop(State);

    RtlZeroMemory(&spec, sizeof(spec));
    spec.MajorFunction = IRP_MJ_DEVICE_CONTROL;
    spec.IoControlCode = takeCode;
    spec.Input = takeInput;
    spec.InputLength = takeInputLength;
    spec.DataLength = (ULONG)outputLength;

    limitReached = FALSE;
    completed = FALSE;

    ObReferenceObject(target);

    status = KphpLuaHwBuildRequest(State, target, fileObject, &spec, &request);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwBuildRequest failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpLuaHwCallRequest(State,
                                  request,
                                  milliseconds,
                                  FALSE,
                                  &limitReached,
                                  &completed);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwCallRequest failed: %!STATUS!",
                      status);

        goto Exit;
    }

Exit:

    ObDereferenceObject(target);

    if (completed && NT_SUCCESS(status))
    {
        NT_ASSERT(request);

        if (length &&
            (((ULONG_PTR)from > request->IoStatus.Information) ||
             ((ULONG_PTR)length >
              request->IoStatus.Information - (ULONG_PTR)from)))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Guard 0x%08lx not claimed: the output is short",
                          giveCode);

            NT_ASSERT(FALSE);
            status = STATUS_DEVICE_PROTOCOL_ERROR;
        }
        else
        {
            if (length)
            {
                RtlCopyMemory(Add2Ptr(give->Input, (ULONG)to),
                              Add2Ptr(request->Data, (ULONG)from),
                              (ULONG)length);
            }

            guard->Claimed = TRUE;
        }
    }

    if (request)
    {
        KphpLuaHwReleaseRequest(request);
    }

    if (limitReached)
    {
        KphScriptCharge(State, 0);
    }

    if (!guard->Claimed)
    {
        KphScriptReleaseCleanup(State, guardIndex);
        return KphLuaNilErrorStatus(State, status);
    }

    lua_pushvalue(State, guardIndex);

    return 1;
}

/**
 * \brief Sends a device control request (ksi.hw.device_control).
 *
 * \details May raise a Lua error. The output is a length, or the initial bytes
 * for a direct or neither code; the options are { internal, argument,
 * buffers }, argument putting the input's address in Argument1 of an internal
 * request.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the output or nil, a status and, on
 * STATUS_BUFFER_OVERFLOW, the output; then the buffers' bytes, if any.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwDeviceControl(
    _In_ lua_State* State
    )
{
    NTSTATUS status;
    PDEVICE_OBJECT target;
    PFILE_OBJECT fileObject;
    lua_Integer code;
    PCSTR input;
    SIZE_T inputLength;
    PCSTR data;
    SIZE_T dataLength;
    lua_Integer outputLength;
    ULONG milliseconds;
    BOOLEAN internal;
    BOOLEAN inputArgument;
    UCHAR majorFunction;
    KPH_LUA_HW_BUFFERS bufferList;
    PKPH_LUA_HW_BUFFERS buffers;
    KPH_LUA_HW_REQUEST_SPEC spec;
    BOOLEAN completed;
    PKPH_LUA_HW_REQUEST request;

    KPH_PAGED_CODE_PASSIVE();

    code = luaL_checkinteger(State, 2);
    input = luaL_checklstring(State, 3, &inputLength);
    milliseconds = KphpLuaHwCheckMilliseconds(State, 5);

    luaL_argcheck(State, (code >= 0) && (code <= ULONG_MAX), 2, "out of range");
    luaL_argcheck(State, inputLength <= ULONG_MAX, 3, "too long");

    if (lua_type(State, 4) == LUA_TSTRING)
    {
        data = lua_tolstring(State, 4, &dataLength);

        luaL_argcheck(State,
                      METHOD_FROM_CTL_CODE((ULONG)code) != METHOD_BUFFERED,
                      4,
                      "a length for a buffered control code");
        luaL_argcheck(State, dataLength <= ULONG_MAX, 4, "too long");
    }
    else
    {
        outputLength = luaL_checkinteger(State, 4);

        luaL_argcheck(State,
                      (outputLength >= 0) && (outputLength <= ULONG_MAX),
                      4,
                      "out of range");

        data = NULL;
        dataLength = (SIZE_T)outputLength;
    }

    internal = FALSE;
    inputArgument = FALSE;
    buffers = NULL;

    if (!lua_isnoneornil(State, 6))
    {
        luaL_checktype(State, 6, LUA_TTABLE);

        lua_getfield(State, 6, "internal");
        internal = !!lua_toboolean(State, -1);

        lua_getfield(State, 6, "argument");
        inputArgument = !!lua_toboolean(State, -1);

        luaL_argcheck(State,
                      internal || !inputArgument,
                      6,
                      "an argument of an internal request only");

        lua_getfield(State, 6, "buffers");

        if (!lua_isnil(State, -1))
        {
            luaL_argcheck(State,
                          lua_type(State, -1) == LUA_TTABLE,
                          6,
                          "buffers of a list");

            KphpLuaHwCheckBuffers(State,
                                  lua_gettop(State),
                                  (ULONG)inputLength,
                                  &bufferList);

            buffers = &bufferList;
        }
    }

    if (internal)
    {
        majorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    }
    else
    {
        majorFunction = IRP_MJ_DEVICE_CONTROL;
    }

    RtlZeroMemory(&spec, sizeof(spec));
    spec.MajorFunction = majorFunction;
    spec.IoControlCode = (ULONG)code;
    spec.Input = input;
    spec.InputLength = (ULONG)inputLength;
    spec.Data = data;
    spec.DataLength = (ULONG)dataLength;
    spec.Buffers = buffers;
    spec.InputArgument = inputArgument;

    target = KphpLuaHwCheckTarget(State, 1, &fileObject);

    ObReferenceObject(target);

    completed = FALSE;

    status = KphpLuaHwBuildRequest(State, target, fileObject, &spec, &request);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwBuildRequest failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpLuaHwCallRequest(State,
                                  request,
                                  milliseconds,
                                  TRUE,
                                  NULL,
                                  &completed);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwCallRequest failed: %!STATUS!",
                      status);

        goto Exit;
    }

Exit:

    ObDereferenceObject(target);

    return KphpLuaHwPushRequest(State,
                                status,
                                completed,
                                request,
                                (ULONG)dataLength,
                                buffers,
                                FALSE);
}

/**
 * \brief Sends a read request (ksi.hw.device_read).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the bytes read, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwDeviceRead(
    _In_ lua_State* State
    )
{
    NTSTATUS status;
    PDEVICE_OBJECT target;
    PFILE_OBJECT fileObject;
    lua_Integer length;
    ULONG milliseconds;
    lua_Integer offset;
    KPH_LUA_HW_REQUEST_SPEC spec;
    BOOLEAN completed;
    PKPH_LUA_HW_REQUEST request;

    KPH_PAGED_CODE_PASSIVE();

    target = KphpLuaHwCheckTarget(State, 1, &fileObject);
    length = luaL_checkinteger(State, 2);
    milliseconds = KphpLuaHwCheckMilliseconds(State, 3);
    offset = luaL_optinteger(State, 4, 0);

    luaL_argcheck(State,
                  (length >= 0) && (length <= ULONG_MAX),
                  2,
                  "out of range");

    RtlZeroMemory(&spec, sizeof(spec));
    spec.MajorFunction = IRP_MJ_READ;
    spec.Offset = (LONGLONG)offset;
    spec.DataLength = (ULONG)length;

    ObReferenceObject(target);

    completed = FALSE;

    status = KphpLuaHwBuildRequest(State, target, fileObject, &spec, &request);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwBuildRequest failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpLuaHwCallRequest(State,
                                  request,
                                  milliseconds,
                                  TRUE,
                                  NULL,
                                  &completed);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwCallRequest failed: %!STATUS!",
                      status);

        goto Exit;
    }

Exit:

    ObDereferenceObject(target);

    return KphpLuaHwPushRequest(State,
                                status,
                                completed,
                                request,
                                (ULONG)length,
                                NULL,
                                FALSE);
}

/**
 * \brief Sends a write request (ksi.hw.device_write).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the count written, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwDeviceWrite(
    _In_ lua_State* State
    )
{
    NTSTATUS status;
    PDEVICE_OBJECT target;
    PFILE_OBJECT fileObject;
    PCSTR data;
    SIZE_T length;
    ULONG milliseconds;
    lua_Integer offset;
    ULONG inputLength;
    KPH_LUA_HW_REQUEST_SPEC spec;
    BOOLEAN completed;
    PKPH_LUA_HW_REQUEST request;

    KPH_PAGED_CODE_PASSIVE();

    target = KphpLuaHwCheckTarget(State, 1, &fileObject);
    data = luaL_checklstring(State, 2, &length);
    milliseconds = KphpLuaHwCheckMilliseconds(State, 3);
    offset = luaL_optinteger(State, 4, 0);

    luaL_argcheck(State, length <= ULONG_MAX, 2, "too long");

    //
    // Buffered I/O has the data in its one buffer, which starts with the input.
    //
    if (KphpLuaHwTransferMethod(target, IRP_MJ_WRITE, 0) == METHOD_BUFFERED)
    {
        inputLength = (ULONG)length;
    }
    else
    {
        inputLength = 0;
    }

    RtlZeroMemory(&spec, sizeof(spec));
    spec.MajorFunction = IRP_MJ_WRITE;
    spec.Offset = (LONGLONG)offset;
    spec.Input = data;
    spec.InputLength = inputLength;
    spec.Data = data;
    spec.DataLength = (ULONG)length;

    ObReferenceObject(target);

    completed = FALSE;

    status = KphpLuaHwBuildRequest(State, target, fileObject, &spec, &request);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwBuildRequest failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpLuaHwCallRequest(State,
                                  request,
                                  milliseconds,
                                  TRUE,
                                  NULL,
                                  &completed);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpLuaHwCallRequest failed: %!STATUS!",
                      status);

        goto Exit;
    }

Exit:

    ObDereferenceObject(target);

    return KphpLuaHwPushRequest(State,
                                status,
                                completed,
                                request,
                                (ULONG)length,
                                NULL,
                                TRUE);
}

/**
 * \brief Checks a device object argument: a device or a PCI device object.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 * \param[in] Index The index of the argument.
 *
 * \return The physical device object, valid while the object is.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
PDEVICE_OBJECT KphpLuaHwCheckPdo(
    _In_ lua_State* State,
    _In_ int Index
    )
{
    PDEVICE_OBJECT pdo;

    KPH_PAGED_CODE_PASSIVE();

    pdo = KphLuaHwTestDevice(State, Index);
    if (!pdo)
    {
        pdo = KphLuaHwPciTestDevice(State, Index);
    }

    luaL_argcheck(State, pdo, Index, "a device or PCI device object");

    return pdo;
}

/**
 * \brief Retrieves a device property by key (ksi.hw.device_property_data).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the bytes and the type, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwDevicePropertyData(
    _In_ lua_State* State
    )
{
    NTSTATUS status;
    PDEVICE_OBJECT pdo;
    DEVPROPKEY key;
    int fieldType;
    PCSTR guid;
    SIZE_T guidLength;
    lua_Integer pid;
    int isInteger;
    ULONG length;
    ULONG required;
    DEVPROPTYPE type;
    PVOID data;
    luaL_Buffer buffer;

    KPH_PAGED_CODE_PASSIVE();

    luaL_checktype(State, 2, LUA_TTABLE);

    fieldType = lua_getfield(State, 2, "guid");
    luaL_argcheck(State,
                  fieldType == LUA_TSTRING,
                  2,
                  "guid not a 16-byte string");
    guid = lua_tolstring(State, -1, &guidLength);
    luaL_argcheck(State,
                  guidLength == sizeof(key.fmtid),
                  2,
                  "guid not a 16-byte string");
    RtlCopyMemory(&key.fmtid, guid, sizeof(key.fmtid));

    lua_getfield(State, 2, "pid");
    pid = lua_tointegerx(State, -1, &isInteger);
    luaL_argcheck(State,
                  isInteger && (pid >= 0) && (pid <= ULONG_MAX),
                  2,
                  "pid out of range");
    key.pid = (DEVPROPID)pid;

    lua_pop(State, 2);

    pdo = KphpLuaHwCheckPdo(State, 1);

    required = 0;
    type = DEVPROP_TYPE_EMPTY;

    //
    // The buffer's own space first; the size is asked only of a larger one.
    //
    luaL_buffinit(State, &buffer);
    length = LUAL_BUFFERSIZE;
    data = luaL_prepbuffsize(&buffer, length);

    status = IoGetDevicePropertyData(pdo,
                                     &key,
                                     LOCALE_NEUTRAL,
                                     0,
                                     length,
                                     data,
                                     &required,
                                     &type);
    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        length = required;
        data = luaL_prepbuffsize(&buffer, length);

        status = IoGetDevicePropertyData(pdo,
                                         &key,
                                         LOCALE_NEUTRAL,
                                         0,
                                         length,
                                         data,
                                         &required,
                                         &type);
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoGetDevicePropertyData failed: %!STATUS!",
                      status);

        return KphLuaNilErrorStatus(State, status);
    }

    luaL_pushresultsize(&buffer, min(required, length));
    lua_pushinteger(State, (lua_Integer)type);

    return 2;
}

/**
 * \brief Retrieves a device property by number (ksi.hw.device_property).
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the bytes, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwDeviceProperty(
    _In_ lua_State* State
    )
{
    NTSTATUS status;
    PDEVICE_OBJECT pdo;
    lua_Integer id;
    DEVICE_REGISTRY_PROPERTY property;
    ULONG length;
    ULONG required;
    PVOID data;
    luaL_Buffer buffer;

    KPH_PAGED_CODE_PASSIVE();

    pdo = KphpLuaHwCheckPdo(State, 1);
    id = luaL_checkinteger(State, 2);

    luaL_argcheck(State, (id >= 0) && (id <= ULONG_MAX), 2, "out of range");

    property = (DEVICE_REGISTRY_PROPERTY)(ULONG)id;
    required = 0;

    //
    // The buffer's own space first; the size is asked only of a larger one.
    //
    luaL_buffinit(State, &buffer);
    length = LUAL_BUFFERSIZE;
    data = luaL_prepbuffsize(&buffer, length);

    status = IoGetDeviceProperty(pdo, property, length, data, &required);
    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        length = required;
        data = luaL_prepbuffsize(&buffer, length);

        status = IoGetDeviceProperty(pdo, property, length, data, &required);
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoGetDeviceProperty failed: %!STATUS!",
                      status);

        return KphLuaNilErrorStatus(State, status);
    }

    luaL_pushresultsize(&buffer, min(required, length));

    return 1;
}

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const luaL_Reg KphpLuaHwDeviceLibrary[] =
{
    { "device_control", KphpLuaHwDeviceControl },
    { "device_guard", KphpLuaHwDeviceGuard },
    { "device_open", KphpLuaHwDeviceOpen },
    { "device_read", KphpLuaHwDeviceRead },
    { "device_write", KphpLuaHwDeviceWrite },
    { "devices", KphpLuaHwDevices },
    { "device_property", KphpLuaHwDeviceProperty },
    { "device_property_data", KphpLuaHwDevicePropertyData },
    { NULL, NULL }
};
KPH_PROTECTED_DATA_SECTION_RO_POP();

/**
 * \brief Initializes the device request functions of the hardware library.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaInitializeHardwareDevice(
    VOID
    )
{
    KPH_OBJECT_TYPE_INFO typeInfo;

    KPH_PAGED_CODE_PASSIVE();

    KeInitializeSpinLock(&KphpLuaHwAbandonLock);
    InitializeListHead(&KphpLuaHwAbandoned);

    KphInitializePagedLookaside(&KphpLuaHwDeviceLookaside,
                                sizeof(KPH_LUA_HW_DEVICE),
                                KPH_TAG_LUA_HARDWARE_DEVICE);

    KphInitializePagedLookaside(&KphpLuaHwNamedDeviceLookaside,
                                sizeof(KPH_LUA_HW_NAMED_DEVICE),
                                KPH_TAG_LUA_HARDWARE_NAMED_DEVICE);

    typeInfo.Allocate = KphpLuaHwAllocateRequest;
    typeInfo.Initialize = KphpLuaHwInitializeRequest;
    typeInfo.Delete = KphpLuaHwDeleteRequest;
    typeInfo.Free = KphpLuaHwFreeRequest;
    typeInfo.Flags = 0;

    KphCreateObjectType(&KphpLuaHwRequestTypeName,
                        &typeInfo,
                        &KphpLuaHwRequestType);
}

/**
 * \brief Cleans up the device request functions of the hardware library.
 *
 * \details Abandoned requests are cancelled; they may complete after unload,
 * and the completion keeps the image mapped until they do.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaCleanupHardwareDevice(
    VOID
    )
{
    SINGLE_LIST_ENTRY abandoned;
    PSINGLE_LIST_ENTRY link;

    KPH_PAGED_CODE_PASSIVE();

    KphpLuaHwReferenceAbandoned(&abandoned);

    for (;;)
    {
        PKPH_LUA_HW_REQUEST request;
        BOOLEAN cancelled;

        link = PopEntryList(&abandoned);
        if (!link)
        {
            break;
        }

        request = CONTAINING_RECORD(link, KPH_LUA_HW_REQUEST, CancelLink);

        cancelled = IoCancelIrp(request->Irp);

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Device control cancelled at unload: %!bool!",
                      cancelled);

        KphDereferenceObject(request);
    }

    KphDeletePagedLookaside(&KphpLuaHwNamedDeviceLookaside);
    KphDeletePagedLookaside(&KphpLuaHwDeviceLookaside);
}

/**
 * \brief Adds the device request functions to the hardware library.
 *
 * \details May raise a Lua error.
 *
 * \param[in] State The Lua state, with the library table on top.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphLuaOpenHardwareDevice(
    _In_ lua_State* State
    )
{
    KPH_PAGED_CODE_PASSIVE();

    luaL_setfuncs(State, KphpLuaHwDeviceLibrary, 0);
}
