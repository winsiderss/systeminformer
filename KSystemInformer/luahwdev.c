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
    PVOID Buffer;
} KPH_LUA_HW_REQUEST, *PKPH_LUA_HW_REQUEST;

typedef struct _KPH_LUA_HW_REQUEST_INIT
{
    PDEVICE_OBJECT Device;
    PFILE_OBJECT FileObject;
    ULONG BufferOffset;
} KPH_LUA_HW_REQUEST_INIT, *PKPH_LUA_HW_REQUEST_INIT;

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
    request->Buffer = Add2Ptr(request, init->BufferOffset);

    return STATUS_SUCCESS;
}

/**
 * \brief Releases a request of KphLuaHwSendRequest.
 *
 * \param[in] Context The request.
 */
_Function_class_(KPH_SCRIPT_CLEANUP_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KSIAPI KphLuaHwReleaseRequest(
    _In_opt_ PVOID Context
    )
{
    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    KphDereferenceObject(Context);
}

/**
 * \brief Sends a device control request to a device and waits for it.
 *
 * \details Does not raise. A request that does not complete in time is
 * cancelled and, when it still does not complete, abandoned: it keeps itself,
 * its buffer and the device alive until it completes, and the device is busy
 * until then. When the call's limit ends the wait the caller releases what it
 * holds and charges the call.
 *
 * \param[in] State The Lua state.
 * \param[in] Device The device to send the request to.
 * \param[in] FileObject Optional file object of the request.
 * \param[in] IoControlCode The control code, a buffered one.
 * \param[in] Internal TRUE for an internal device control request.
 * \param[in] Input The input bytes.
 * \param[in] InputLength The number of input bytes.
 * \param[in] OutputLength The number of output bytes asked for.
 * \param[in] Milliseconds How long to wait, ULONG_MAX for no limit of its own.
 * \param[out] Request Receives the completed request, released with
 * KphLuaHwReleaseRequest; NULL on failure.
 * \param[out] Output Receives the output bytes of the request.
 * \param[out] Information Receives the number of output bytes, at most
 * OutputLength.
 *
 * \return Successful or errant status; STATUS_BUFFER_OVERFLOW returns the
 * request.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphLuaHwSendRequest(
    _In_ lua_State* State,
    _In_ PDEVICE_OBJECT Device,
    _In_opt_ PFILE_OBJECT FileObject,
    _In_ ULONG IoControlCode,
    _In_ BOOLEAN Internal,
    _In_reads_bytes_(InputLength) PCVOID Input,
    _In_ ULONG InputLength,
    _In_ ULONG OutputLength,
    _In_ ULONG Milliseconds,
    _Outptr_result_maybenull_ PVOID* Request,
    _Outptr_result_maybenull_ PVOID* Output,
    _Out_ PULONG Information
    )
{
    NTSTATUS status;
    KPH_LUA_HW_REQUEST_INIT init;
    PKPH_LUA_HW_REQUEST request;
    ULONG bufferLength;
    ULONG bufferOffset;
    ULONG allocationLength;
    PIO_STACK_LOCATION stack;
    LARGE_INTEGER zero;
    BOOLEAN cancelled;
    NTSTATUS waitStatus;

    KPH_PAGED_CODE_PASSIVE();

    *Request = NULL;
    *Output = NULL;
    *Information = 0;

    request = NULL;

    bufferLength = max(InputLength, OutputLength);
    bufferOffset = (ULONG)ALIGN_UP_BY(sizeof(KPH_LUA_HW_REQUEST),
                                      MEMORY_ALLOCATION_ALIGNMENT);

    status = RtlULongAdd(bufferOffset, bufferLength, &allocationLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongAdd failed: %!STATUS!",
                      status);

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    if (KphpLuaHwIsDeviceBusy(Device))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Device busy: an abandoned request is pending");

        status = STATUS_DEVICE_BUSY;
        goto Exit;
    }

    init.Device = Device;
    init.FileObject = FileObject;
    init.BufferOffset = bufferOffset;

    status = KphCreateObject(KphpLuaHwRequestType,
                             allocationLength,
                             &request,
                             &init);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphCreateObject failed: %!STATUS!",
                      status);

        goto Exit;
    }

    RtlCopyMemory(request->Buffer, Input, InputLength);

    request->Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    request->Irp->IoStatus.Information = 0;
    request->Irp->AssociatedIrp.SystemBuffer = request->Buffer;

    stack = IoGetNextIrpStackLocation(request->Irp);
    stack->MajorFunction = (Internal ? IRP_MJ_INTERNAL_DEVICE_CONTROL :
                                       IRP_MJ_DEVICE_CONTROL);
    stack->FileObject = FileObject;
    stack->Parameters.DeviceIoControl.IoControlCode = IoControlCode;
    stack->Parameters.DeviceIoControl.InputBufferLength = InputLength;
    stack->Parameters.DeviceIoControl.OutputBufferLength = OutputLength;

    KsiInitializeIoCompletion(&request->Completion,
                              KphDriverObject,
                              KphpLuaHwRequestComplete,
                              request,
                              TRUE,
                              TRUE,
                              TRUE);

    KsiSetCompletionRoutine(request->Irp, &request->Completion);

    KphReferenceObject(request);

    status = IoCallDriver(Device, request->Irp);

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "Device control 0x%08lx IoCallDriver: %!STATUS!",
                  IoControlCode,
                  status);

    status = KphScriptWait(State, &request->Event, Milliseconds);
    if (status != STATUS_SUCCESS)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphScriptWait returned: %!STATUS!",
                      status);

        cancelled = IoCancelIrp(request->Irp);

        zero.QuadPart = 0;

        waitStatus = KeWaitForSingleObject(&request->Event,
                                           Executive,
                                           KernelMode,
                                           FALSE,
                                           &zero);
        if (waitStatus != STATUS_SUCCESS)
        {
            KphpLuaHwAbandonRequest(request);

            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Device control 0x%08lx abandoned "
                          "(cancelled %!bool!)",
                          IoControlCode,
                          cancelled);

            if (status != STATUS_SI_SCRIPT_LIMIT)
            {
                status = STATUS_IO_TIMEOUT;
            }

            goto Exit;
        }

        if (status == STATUS_SI_SCRIPT_LIMIT)
        {
            goto Exit;
        }
    }

    status = request->IoStatus.Status;
    if (!NT_SUCCESS(status) && (status != STATUS_BUFFER_OVERFLOW))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Device control 0x%08lx failed: %!STATUS!",
                      IoControlCode,
                      status);

        goto Exit;
    }

    if (status != STATUS_BUFFER_OVERFLOW)
    {
        status = STATUS_SUCCESS;
    }

    *Information = (ULONG)min(request->IoStatus.Information, OutputLength);
    *Output = request->Buffer;
    *Request = request;
    request = NULL;

Exit:

    if (request)
    {
        KphDereferenceObject(request);
    }

    return status;
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

    device = NULL;

    name.Length = (USHORT)(length * sizeof(WCHAR));
    name.MaximumLength = name.Length;
    name.Buffer = KphAllocatePaged(name.Length, KPH_TAG_LUA_HARDWARE_NAME);
    if (!name.Buffer)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate a device name");

        status = STATUS_INSUFFICIENT_RESOURCES;
        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    for (ULONG i = 0; i < (name.Length / sizeof(WCHAR)); i++)
    {
        name.Buffer[i] = (WCHAR)(UCHAR)text[i];
    }

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

    status = IoGetDeviceObjectPointer(&name,
                                      (ACCESS_MASK)access,
                                      &fileObject,
                                      &deviceObject);
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

    KphFree(name.Buffer, KPH_TAG_LUA_HARDWARE_NAME);
    name.Buffer = NULL;

    KphScriptPushObject(State, KphpLuaHwReleaseNamedDevice, device);
    device = NULL;

    results = 1;

Exit:

    if (device)
    {
        KphFreeToPagedLookaside(&KphpLuaHwNamedDeviceLookaside, device);
    }

    if (name.Buffer)
    {
        KphFree(name.Buffer, KPH_TAG_LUA_HARDWARE_NAME);
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

    KphFree(list, KPH_TAG_LUA_HARDWARE_DEVICES);
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

    name.Length = (USHORT)(length * sizeof(WCHAR));
    name.MaximumLength = name.Length;
    name.Buffer = KphAllocatePaged(name.Length, KPH_TAG_LUA_HARDWARE_NAME);
    if (!name.Buffer)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate a driver name");

        status = STATUS_INSUFFICIENT_RESOURCES;
        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    for (ULONG i = 0; i < (name.Length / sizeof(WCHAR)); i++)
    {
        name.Buffer[i] = (WCHAR)(UCHAR)text[i];
    }

    status = ObReferenceObjectByName(&name,
                                     OBJ_DONT_REPARSE,
                                     NULL,
                                     0,
                                     *IoDriverObjectType,
                                     KernelMode,
                                     NULL,
                                     (PVOID*)&driver);
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
                            KPH_TAG_LUA_HARDWARE_DEVICES);
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

    KphFree(name.Buffer, KPH_TAG_LUA_HARDWARE_NAME);
    name.Buffer = NULL;

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

    if (name.Buffer)
    {
        KphFree(name.Buffer, KPH_TAG_LUA_HARDWARE_NAME);
    }

    return results;
}

/**
 * \brief Sends a device control request (ksi.hw.device_control).
 *
 * \details May raise a Lua error. A named device takes the request at the top
 * of its stack, with its file object. A device or a PCI device object takes it
 * itself: the request goes to the bus driver, and no other driver of the stack
 * sees it.
 *
 * \param[in] State The Lua state.
 *
 * \return The number of results: the output bytes, or nil and a status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
int KphpLuaHwDeviceControl(
    _In_ lua_State* State
    )
{
    int results;
    NTSTATUS status;
    PKPH_LUA_HW_NAMED_DEVICE named;
    PDEVICE_OBJECT pdo;
    PDEVICE_OBJECT pci;
    lua_Integer code;
    PCSTR input;
    SIZE_T inputLength;
    lua_Integer outputLength;
    lua_Integer milliseconds;
    BOOLEAN internal;
    PDEVICE_OBJECT target;
    PFILE_OBJECT fileObject;
    PVOID request;
    PVOID output;
    ULONG information;
    int cleanup;

    KPH_PAGED_CODE_PASSIVE();

    named = KphScriptTestCleanup(State, 1, KphpLuaHwReleaseNamedDevice);
    pdo = KphLuaHwTestDevice(State, 1);
    pci = KphLuaHwPciTestDevice(State, 1);

    luaL_argcheck(State,
                  named || pdo || pci,
                  1,
                  "a named, device or PCI device object");

    code = luaL_checkinteger(State, 2);
    input = luaL_checklstring(State, 3, &inputLength);
    outputLength = luaL_checkinteger(State, 4);
    milliseconds = luaL_checkinteger(State, 5);
    internal = !!lua_toboolean(State, 6);

    luaL_argcheck(State,
                  (code >= 0) &&
                  (code <= ULONG_MAX) &&
                  (METHOD_FROM_CTL_CODE((ULONG)code) == METHOD_BUFFERED),
                  2,
                  "a buffered control code");
    luaL_argcheck(State, inputLength <= ULONG_MAX, 3, "too long");
    luaL_argcheck(State,
                  (outputLength >= 0) && (outputLength <= ULONG_MAX),
                  4,
                  "out of range");
    luaL_argcheck(State,
                  (milliseconds >= 1) && (milliseconds < ULONG_MAX),
                  5,
                  "out of range");

    fileObject = NULL;

    if (named)
    {
        fileObject = named->FileObject;
        target = IoGetRelatedDeviceObject(fileObject);
        ObReferenceObject(target);
    }
    else if (pdo)
    {
        target = pdo;
        ObReferenceObject(target);
    }
    else
    {
        NT_ASSERT(pci);

        target = pci;
        ObReferenceObject(target);
    }

    status = KphLuaHwSendRequest(State,
                                 target,
                                 fileObject,
                                 (ULONG)code,
                                 internal,
                                 input,
                                 (ULONG)inputLength,
                                 (ULONG)outputLength,
                                 (ULONG)milliseconds,
                                 &request,
                                 &output,
                                 &information);

    ObDereferenceObject(target);

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        KphLuaHwReleaseRequest(request);
    }

    if (status == STATUS_SI_SCRIPT_LIMIT)
    {
        KphScriptCharge(State, 0);
    }

    if (status != STATUS_SUCCESS)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphLuaHwSendRequest failed: %!STATUS!",
                      status);

        results = KphLuaNilErrorStatus(State, status);
        goto Exit;
    }

    KphScriptPushCleanup(State, KphLuaHwReleaseRequest, request);
    cleanup = lua_gettop(State);

    lua_pushlstring(State, output, information);

    KphScriptReleaseCleanup(State, cleanup);
    lua_remove(State, cleanup);

    results = 1;

Exit:

    return results;
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

    pdo = KphpLuaHwCheckPdo(State, 1);

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

    required = 0;
    type = DEVPROP_TYPE_EMPTY;

    status = IoGetDevicePropertyData(pdo,
                                     &key,
                                     LOCALE_NEUTRAL,
                                     0,
                                     0,
                                     NULL,
                                     &required,
                                     &type);
    if (NT_SUCCESS(status))
    {
        lua_pushliteral(State, "");
        lua_pushinteger(State, (lua_Integer)type);
        return 2;
    }

    if (status != STATUS_BUFFER_TOO_SMALL)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoGetDevicePropertyData failed: %!STATUS!",
                      status);

        return KphLuaNilErrorStatus(State, status);
    }

    length = required;
    data = luaL_buffinitsize(State, &buffer, length);

    status = IoGetDevicePropertyData(pdo,
                                     &key,
                                     LOCALE_NEUTRAL,
                                     0,
                                     length,
                                     data,
                                     &required,
                                     &type);
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

    status = IoGetDeviceProperty(pdo, property, 0, NULL, &required);
    if (NT_SUCCESS(status))
    {
        lua_pushliteral(State, "");
        return 1;
    }

    if (status != STATUS_BUFFER_TOO_SMALL)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoGetDeviceProperty failed: %!STATUS!",
                      status);

        return KphLuaNilErrorStatus(State, status);
    }

    length = required;
    data = luaL_buffinitsize(State, &buffer, length);

    status = IoGetDeviceProperty(pdo, property, length, data, &required);
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
    { "device_open", KphpLuaHwDeviceOpen },
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
                                KPH_TAG_LUA_HARDWARE_DEVICE);

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
