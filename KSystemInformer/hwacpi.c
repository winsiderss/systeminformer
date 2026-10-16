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
#include <kphdyndata.h>
#include <kphhw.h>
#include <acpiioct.h>
#include <initguid.h>
#include <devpkey.h>

#include <trace.h>

C_ASSERT(KPH_DYN_HW_ACPI_MIN_OUTPUT == sizeof(ACPI_EVAL_OUTPUT_BUFFER));

typedef struct _KPH_HW_ACPI_EVAL
{
    LIST_ENTRY AbandonedLink;
    BOOLEAN Abandoned;
    BOOLEAN Completed;
    SINGLE_LIST_ENTRY CancelLink;
    PIRP Irp;
    PDEVICE_OBJECT Pdo;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatus;
    KSI_IO_COMPLETION Completion;
    PVOID Buffer;
} KPH_HW_ACPI_EVAL, *PKPH_HW_ACPI_EVAL;

typedef struct _KPH_HW_ACPI_EVAL_INIT
{
    PDEVICE_OBJECT Pdo;
    ULONG BufferOffset;
} KPH_HW_ACPI_EVAL_INIT, *PKPH_HW_ACPI_EVAL_INIT;

typedef struct _KPH_HW_ACPI_DEVICE
{
    LIST_ENTRY Link;
    ANSI_STRING Path;
    PDEVICE_OBJECT Pdo;
    CHAR Id[KPH_DYN_HW_ACPI_ID_LENGTH];
} KPH_HW_ACPI_DEVICE, *PKPH_HW_ACPI_DEVICE;

typedef struct _KPH_HW_ACPI_FIND_PDO
{
    PCANSI_STRING Path;
    PDEVICE_OBJECT Pdo;
} KPH_HW_ACPI_FIND_PDO, *PKPH_HW_ACPI_FIND_PDO;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
DECLARE_CONST_UNICODE_STRING(KphpHwAcpiEvalTypeName, L"KphHwAcpiEval");
DECLARE_CONST_UNICODE_STRING(KphpHwAcpiDriverName, L"\\Driver\\ACPI");
DECLARE_CONST_UNICODE_STRING(KphpHwAcpiIdPrefix, L"ACPI\\");
DECLARE_CONST_UNICODE_STRING(KphpHwAcpiVenPrefix, L"VEN_");
DECLARE_CONST_UNICODE_STRING(KphpHwAcpiDevPrefix, L"&DEV_");
static const DEVICE_REGISTRY_PROPERTY KphpHwAcpiIdProperties[] =
{
    DevicePropertyHardwareID,
    DevicePropertyCompatibleIDs,
};
KPH_PROTECTED_DATA_SECTION_RO_POP();
KPH_PROTECTED_DATA_SECTION_PUSH();
static PKPH_OBJECT_TYPE KphpHwAcpiEvalType = NULL;
KPH_PROTECTED_DATA_SECTION_POP();
static KPH_RWLOCK KphpHwAcpiLock;
static BOOLEAN KphpHwAcpiInitialized = FALSE;
static KSPIN_LOCK KphpHwAcpiAbandonLock;
static LIST_ENTRY KphpHwAcpiAbandoned;
static LIST_ENTRY KphpHwAcpiDevices;

/**
 * \brief Deletes an evaluation request.
 *
 * \details May run after unload, so it does not trace.
 *
 * \param[in,out] Object The evaluation request.
 */
_Function_class_(KPH_TYPE_DELETE_PROCEDURE)
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KSIAPI KphpHwAcpiDeleteEval(
    _Inout_ PVOID Object
    )
{
    PKPH_HW_ACPI_EVAL eval;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    eval = Object;

    NT_ASSERT(!eval->Abandoned);

    IoFreeIrp(eval->Irp);
    ObDereferenceObject(eval->Pdo);
}

/**
 * \brief Frees an evaluation request.
 *
 * \details May run after unload, so it does not trace.
 *
 * \param[in] Object The evaluation request.
 */
_Function_class_(KPH_TYPE_FREE_PROCEDURE)
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KSIAPI KphpHwAcpiFreeEval(
    _In_freesMem_ PVOID Object
    )
{
    KPH_NPAGED_CODE_DISPATCH_MAX();

    KphFree(Object, KPH_TAG_HW_ACPI_EVAL);
}

/**
 * \brief Checks for an abandoned evaluation of a device.
 *
 * \param[in] Pdo The physical device object of the device.
 *
 * \return TRUE if an abandoned evaluation of the device is still pending,
 * FALSE otherwise.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
BOOLEAN KphpHwAcpiIsDeviceBusy(
    _In_ PDEVICE_OBJECT Pdo
    )
{
    KIRQL irql;
    BOOLEAN busy;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    busy = FALSE;

    KeAcquireSpinLock(&KphpHwAcpiAbandonLock, &irql);

    for (PLIST_ENTRY link = KphpHwAcpiAbandoned.Flink;
         link != &KphpHwAcpiAbandoned;
         link = link->Flink)
    {
        PKPH_HW_ACPI_EVAL eval;

        eval = CONTAINING_RECORD(link, KPH_HW_ACPI_EVAL, AbandonedLink);

        if (eval->Pdo == Pdo)
        {
            busy = TRUE;
            break;
        }
    }

    KeReleaseSpinLock(&KphpHwAcpiAbandonLock, irql);

    return busy;
}

/**
 * \brief Leaves a timed-out evaluation to its completion.
 *
 * \details The abandoned list takes a reference, which the completion
 * releases.
 *
 * \param[in] Eval The evaluation request.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphpHwAcpiAbandonEval(
    _In_ PKPH_HW_ACPI_EVAL Eval
    )
{
    KIRQL irql;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    KeAcquireSpinLock(&KphpHwAcpiAbandonLock, &irql);

    if (!Eval->Completed)
    {
        KphReferenceObject(Eval);
        Eval->Abandoned = TRUE;
        InsertTailList(&KphpHwAcpiAbandoned, &Eval->AbandonedLink);
    }

    KeReleaseSpinLock(&KphpHwAcpiAbandonLock, irql);
}

/**
 * \brief References every abandoned evaluation to cancel at unload.
 *
 * \param[out] Head Receives the referenced evaluations, chained through
 * CancelLink.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphpHwAcpiReferenceAbandoned(
    _Out_ PSINGLE_LIST_ENTRY Head
    )
{
    KIRQL irql;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    Head->Next = NULL;

    KeAcquireSpinLock(&KphpHwAcpiAbandonLock, &irql);

    for (PLIST_ENTRY link = KphpHwAcpiAbandoned.Flink;
         link != &KphpHwAcpiAbandoned;
         link = link->Flink)
    {
        PKPH_HW_ACPI_EVAL eval;

        eval = CONTAINING_RECORD(link, KPH_HW_ACPI_EVAL, AbandonedLink);

        KphReferenceObject(eval);
        PushEntryList(Head, &eval->CancelLink);
    }

    KeReleaseSpinLock(&KphpHwAcpiAbandonLock, irql);
}

/**
 * \brief Completes an evaluation request.
 *
 * \details Releases the reference of the abandoned list, if the request is on
 * it, then the reference of the completion. The IRP is freed with the request,
 * never completed further. May run after unload, so it does not trace.
 *
 * \param[in] DeviceObject Unused.
 * \param[in] Irp The IRP of the request.
 * \param[in] Context The evaluation request.
 *
 * \return STATUS_MORE_PROCESSING_REQUIRED
 */
_Function_class_(IO_COMPLETION_ROUTINE)
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS KphpHwAcpiEvalComplete(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context
    )
{
    PKPH_HW_ACPI_EVAL eval;
    KIRQL irql;
    BOOLEAN abandoned;

    KPH_NPAGED_CODE_DISPATCH_MAX();

    UNREFERENCED_PARAMETER(DeviceObject);

    eval = Context;

    NT_ASSERT(eval && (eval->Irp == Irp));

    eval->IoStatus = Irp->IoStatus;

    KeSetEvent(&eval->Event, IO_NO_INCREMENT, FALSE);

    KeAcquireSpinLock(&KphpHwAcpiAbandonLock, &irql);

    eval->Completed = TRUE;

    abandoned = eval->Abandoned;
    if (abandoned)
    {
        RemoveEntryList(&eval->AbandonedLink);
        eval->Abandoned = FALSE;
    }

    KeReleaseSpinLock(&KphpHwAcpiAbandonLock, irql);

    if (abandoned)
    {
        KphDereferenceObject(eval);
    }

    KphDereferenceObject(eval);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

KPH_PAGED_FILE();

/**
 * \brief Allocates an evaluation request.
 *
 * \param[in] Size The size to allocate.
 *
 * \return The allocation, NULL on failure.
 */
_Function_class_(KPH_TYPE_ALLOCATE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Return_allocatesMem_size_(Size)
PVOID KSIAPI KphpHwAcpiAllocateEval(
    _In_ SIZE_T Size
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return KphAllocateNPaged(Size, KPH_TAG_HW_ACPI_EVAL);
}

/**
 * \brief Initializes an evaluation request.
 *
 * \param[in,out] Object The evaluation request.
 * \param[in] Parameter The KPH_HW_ACPI_EVAL_INIT of the request.
 *
 * \return Successful or errant status.
 */
_Function_class_(KPH_TYPE_INITIALIZE_PROCEDURE)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KSIAPI KphpHwAcpiInitializeEval(
    _Inout_ PVOID Object,
    _In_opt_ PVOID Parameter
    )
{
    PKPH_HW_ACPI_EVAL eval;
    PKPH_HW_ACPI_EVAL_INIT init;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Parameter);

    eval = Object;
    init = Parameter;

    eval->Irp = IoAllocateIrp(init->Pdo->StackSize, FALSE);
    if (!eval->Irp)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "IoAllocateIrp failed");

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ObReferenceObject(init->Pdo);
    eval->Pdo = init->Pdo;
    InitializeListHead(&eval->AbandonedLink);
    KeInitializeEvent(&eval->Event, NotificationEvent, FALSE);
    eval->Buffer = Add2Ptr(eval, init->BufferOffset);

    return STATUS_SUCCESS;
}

/**
 * \brief Initializes the ACPI infrastructure.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInitializeHardwareAcpi(
    VOID
    )
{
    KPH_OBJECT_TYPE_INFO typeInfo;

    KPH_PAGED_CODE_PASSIVE();

    KphInitializeRWLock(&KphpHwAcpiLock);
    KeInitializeSpinLock(&KphpHwAcpiAbandonLock);
    InitializeListHead(&KphpHwAcpiAbandoned);
    InitializeListHead(&KphpHwAcpiDevices);

    typeInfo.Allocate = KphpHwAcpiAllocateEval;
    typeInfo.Initialize = KphpHwAcpiInitializeEval;
    typeInfo.Delete = KphpHwAcpiDeleteEval;
    typeInfo.Free = KphpHwAcpiFreeEval;
    typeInfo.Flags = 0;

    KphCreateObjectType(&KphpHwAcpiEvalTypeName,
                        &typeInfo,
                        &KphpHwAcpiEvalType);

    KphpHwAcpiInitialized = TRUE;
}

/**
 * \brief Unlinks and frees a cache entry.
 *
 * \param[in] Device The entry.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwAcpiLock)
VOID KphpHwAcpiClose(
    _In_ PKPH_HW_ACPI_DEVICE Device
    )
{
    KPH_PAGED_CODE_PASSIVE();

    RemoveEntryList(&Device->Link);

    ObDereferenceObject(Device->Pdo);

    KphFree(Device, KPH_TAG_HW_ACPI_DEVICE);
}

/**
 * \brief Cancels abandoned evaluations and releases the cache.
 *
 * \details Called after the communication port is closed. Evaluations may
 * complete after unload; ksi.dll keeps the image mapped until they do.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphCleanupHardwareAcpi(
    VOID
    )
{
    SINGLE_LIST_ENTRY abandoned;
    PSINGLE_LIST_ENTRY link;

    KPH_PAGED_CODE_PASSIVE();

    if (!KphpHwAcpiInitialized)
    {
        return;
    }

    KphpHwAcpiReferenceAbandoned(&abandoned);

    for (;;)
    {
        PKPH_HW_ACPI_EVAL eval;
        BOOLEAN cancelled;

        link = PopEntryList(&abandoned);
        if (!link)
        {
            break;
        }

        eval = CONTAINING_RECORD(link, KPH_HW_ACPI_EVAL, CancelLink);

        cancelled = IoCancelIrp(eval->Irp);

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI evaluation cancelled at unload: %!bool!",
                      cancelled);

        KphDereferenceObject(eval);
    }

    KphAcquireRWLockExclusive(&KphpHwAcpiLock);

    while (!IsListEmpty(&KphpHwAcpiDevices))
    {
        PKPH_HW_ACPI_DEVICE device;

        device = CONTAINING_RECORD(KphpHwAcpiDevices.Flink,
                                   KPH_HW_ACPI_DEVICE,
                                   Link);

        KphpHwAcpiClose(device);
    }

    KphReleaseRWLock(&KphpHwAcpiLock);

    KphDeleteRWLock(&KphpHwAcpiLock);
}

/**
 * \brief Checks whether the ACPI driver is present.
 *
 * \return TRUE if the ACPI driver object resolves, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphHwAcpiAvailable(
    VOID
    )
{
    NTSTATUS status;
    PDRIVER_OBJECT driver;

    KPH_PAGED_CODE_PASSIVE();

    status = KphHwReferenceDriver(&KphpHwAcpiDriverName, &driver);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwReferenceDriver failed: %!STATUS!",
                      status);

        return FALSE;
    }

    ObDereferenceObject(driver);

    return TRUE;
}

/**
 * \brief Checks whether a character may appear in a namespace path segment.
 *
 * \param[in] Character The character.
 * \param[in] First TRUE for the first character of a segment.
 *
 * \return TRUE for A-Z and _, and 0-9 past the first character.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpHwAcpiPathCharValid(
    _In_ CHAR Character,
    _In_ BOOLEAN First
    )
{
    KPH_PAGED_CODE_PASSIVE();

    return (((Character >= 'A') && (Character <= 'Z')) ||
            (Character == '_') ||
            (!First && (Character >= '0') && (Character <= '9')));
}

/**
 * \brief Normalizes an absolute namespace path as BiosDeviceName prints it.
 *
 * \details Trailing underscore padding is removed, so "\_SB_.EC0_" and
 * "\_SB.EC0" are the same path. The path never grows.
 *
 * \param[in,out] Path The path, normalized in place; undefined on failure.
 *
 * \return TRUE if the path is well formed.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
_Success_(return)
BOOLEAN KphpHwAcpiNormalizePath(
    _Inout_ PANSI_STRING Path
    )
{
    PCHAR buffer;
    ULONG length;
    ULONG offset;
    ULONG out;

    KPH_PAGED_CODE_PASSIVE();

    buffer = Path->Buffer;
    length = Path->Length;

    if ((length < 2) || (buffer[0] != OBJ_NAME_PATH_SEPARATOR))
    {
        return FALSE;
    }

    offset = 1;
    out = 1;

    while (offset < length)
    {
        ULONG start;
        ULONG end;

        start = offset;

        while ((offset < length) && (buffer[offset] != '.'))
        {
            if (!KphpHwAcpiPathCharValid(buffer[offset], (offset == start)))
            {
                return FALSE;
            }

            offset++;
        }

        end = offset;

        if ((end == start) || ((end - start) > 4))
        {
            return FALSE;
        }

        while (((end - start) > 1) && (buffer[end - 1] == '_'))
        {
            end--;
        }

        RtlMoveMemory(&buffer[out], &buffer[start], end - start);
        out += (end - start);

        if (offset < length)
        {
            offset++;

            if (offset >= length)
            {
                return FALSE;
            }

            buffer[out++] = '.';
        }
    }

    NT_ASSERT(out <= length);

    Path->Length = (USHORT)out;

    return TRUE;
}

/**
 * \brief Captures and normalizes the namespace path of a request.
 *
 * \param[in] Path The path.
 * \param[out] Captured Receives the normalized path, released with
 * KphReleaseAnsiString.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status; STATUS_INVALID_PARAMETER for an empty
 * or malformed path.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwAcpiCapturePath(
    _In_ PANSI_STRING Path,
    _Outptr_ PANSI_STRING* Captured,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PANSI_STRING path;

    KPH_PAGED_CODE_PASSIVE();

    *Captured = NULL;
    path = NULL;

    status = KphCaptureAnsiString(Path, &path, AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphCaptureAnsiString failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (!KphpHwAcpiNormalizePath(path))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI path %Z is malformed",
                      path);

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    *Captured = path;
    path = NULL;
    status = STATUS_SUCCESS;

Exit:

    if (path)
    {
        KphReleaseAnsiString(path);
    }

    return status;
}

/**
 * \brief Retrieves the namespace path of an ACPI device, normalized.
 *
 * \param[in] Pdo The physical device object.
 * \param[out] Path Receives the normalized path, freed with KphFree and
 * KPH_TAG_HW_ACPI_PATH.
 *
 * \return Successful or errant status; STATUS_NOT_FOUND if the device has no
 * well-formed BiosDeviceName.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwAcpiQueryPath(
    _In_ PDEVICE_OBJECT Pdo,
    _Outptr_ PANSI_STRING* Path
    )
{
    NTSTATUS status;
    PWCHAR name;
    PANSI_STRING path;
    ULONG required;
    ULONG length;
    ULONG count;
    ULONG prefix;
    ULONG pathLength;
    ULONG allocationLength;
    DEVPROPTYPE type;

    KPH_PAGED_CODE_PASSIVE();

    *Path = NULL;

    name = NULL;
    path = NULL;
    required = 0;
    type = DEVPROP_TYPE_EMPTY;

    status = IoGetDevicePropertyData(Pdo,
                                     &DEVPKEY_Device_BiosDeviceName,
                                     LOCALE_NEUTRAL,
                                     0,
                                     0,
                                     NULL,
                                     &required,
                                     &type);
    if ((status != STATUS_BUFFER_TOO_SMALL) ||
        (type != DEVPROP_TYPE_STRING) ||
        (required < (2 * sizeof(WCHAR))) ||
        ((required % sizeof(WCHAR)) != 0))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "BiosDeviceName query: %!STATUS! type %lu length %lu",
                      status,
                      (ULONG)type,
                      required);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    count = ((required / sizeof(WCHAR)) - 1);

    if (count >= MAXUSHORT)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "BiosDeviceName is too long: %lu",
                      count);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    name = KphAllocatePaged(required, KPH_TAG_HW_ACPI_BUFFER);
    if (!name)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate ACPI path buffer");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    length = required;

    status = IoGetDevicePropertyData(Pdo,
                                     &DEVPKEY_Device_BiosDeviceName,
                                     LOCALE_NEUTRAL,
                                     0,
                                     length,
                                     name,
                                     &required,
                                     &type);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "IoGetDevicePropertyData failed: %!STATUS!",
                      status);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    if ((type != DEVPROP_TYPE_STRING) ||
        (required != length) ||
        (name[count] != UNICODE_NULL))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "BiosDeviceName changed: type %lu length %lu",
                      (ULONG)type,
                      required);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    prefix = ((name[0] == OBJ_NAME_PATH_SEPARATOR) ? 0 : 1);

    NT_VERIFY(NT_SUCCESS(RtlULongAdd(count, prefix, &pathLength)));
    NT_VERIFY(NT_SUCCESS(RtlULongAdd(sizeof(ANSI_STRING),
                                     pathLength,
                                     &allocationLength)));

    path = KphAllocatePaged(allocationLength, KPH_TAG_HW_ACPI_PATH);
    if (!path)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate ACPI path");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    path->Buffer = Add2Ptr(path, sizeof(ANSI_STRING));
    path->Length = (USHORT)pathLength;
    path->MaximumLength = (USHORT)pathLength;
    path->Buffer[0] = OBJ_NAME_PATH_SEPARATOR;

    for (ULONG i = 0; i < count; i++)
    {
        if ((name[i] == UNICODE_NULL) || (name[i] > 0x7F))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "BiosDeviceName has a non-ASCII character at %lu",
                          i);

            status = STATUS_NOT_FOUND;
            goto Exit;
        }

        path->Buffer[prefix + i] = (CHAR)name[i];
    }

    if (!KphpHwAcpiNormalizePath(path))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "BiosDeviceName %Z is malformed",
                      path);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    *Path = path;
    path = NULL;
    status = STATUS_SUCCESS;

Exit:

    if (path)
    {
        KphFree(path, KPH_TAG_HW_ACPI_PATH);
    }

    if (name)
    {
        KphFree(name, KPH_TAG_HW_ACPI_BUFFER);
    }

    return status;
}

/**
 * \brief Converts a hardware or compatible ID to an ACPI_HID identity.
 *
 * \param[in] String The ID string.
 * \param[out] Id Receives the identity, zero padded.
 *
 * \return TRUE if the string names an ID.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
_Success_(return)
BOOLEAN KphpHwAcpiNormalizeId(
    _In_ PCUNICODE_STRING String,
    _Out_writes_(KPH_DYN_HW_ACPI_ID_LENGTH) PCHAR Id
    )
{
    CHAR id[KPH_DYN_HW_ACPI_ID_LENGTH];
    UNICODE_STRING string;
    UNICODE_STRING part;
    ULONG out;

    KPH_PAGED_CODE_PASSIVE();

    string = *String;

    if (RtlPrefixUnicodeString(&KphpHwAcpiIdPrefix, &string, TRUE))
    {
        string.Buffer += (KphpHwAcpiIdPrefix.Length / sizeof(WCHAR));
        string.Length -= KphpHwAcpiIdPrefix.Length;
    }
    else if ((string.Length >= sizeof(WCHAR)) && (string.Buffer[0] == L'*'))
    {
        string.Buffer++;
        string.Length -= sizeof(WCHAR);
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ID %wZ is not an ACPI ID",
                      String);

        return FALSE;
    }

    string.MaximumLength = string.Length;

    RtlZeroMemory(id, sizeof(id));
    out = 0;

    if (RtlPrefixUnicodeString(&KphpHwAcpiVenPrefix, &string, TRUE))
    {
        string.Buffer += (KphpHwAcpiVenPrefix.Length / sizeof(WCHAR));
        string.Length -= KphpHwAcpiVenPrefix.Length;
        string.MaximumLength = string.Length;

        part = string;
        part.Length = 0;

        while ((part.Length < string.Length) &&
               (string.Buffer[part.Length / sizeof(WCHAR)] != L'&'))
        {
            part.Length += sizeof(WCHAR);
        }

        string.Buffer += (part.Length / sizeof(WCHAR));
        string.Length -= part.Length;
        string.MaximumLength = string.Length;

        if (!RtlPrefixUnicodeString(&KphpHwAcpiDevPrefix, &string, TRUE))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "ACPI ID %wZ has no &DEV_",
                          String);

            return FALSE;
        }

        string.Buffer += (KphpHwAcpiDevPrefix.Length / sizeof(WCHAR));
        string.Length -= KphpHwAcpiDevPrefix.Length;
        string.MaximumLength = string.Length;

        if ((((SIZE_T)part.Length + string.Length) / sizeof(WCHAR)) >
            ARRAYSIZE(id))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "ACPI ID %wZ is too long",
                          String);

            return FALSE;
        }

        for (ULONG i = 0; i < (part.Length / sizeof(WCHAR)); i++)
        {
            WCHAR c;

            c = RtlUpcaseUnicodeChar(part.Buffer[i]);

            if ((c == UNICODE_NULL) || (c > 0x7F))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              HARDWARE,
                              "ACPI ID %wZ is not ASCII",
                              String);

                return FALSE;
            }

            id[out++] = (CHAR)c;
        }
    }

    if ((out + (string.Length / sizeof(WCHAR))) > ARRAYSIZE(id))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI ID %wZ is too long",
                      String);

        return FALSE;
    }

    for (ULONG i = 0; i < (string.Length / sizeof(WCHAR)); i++)
    {
        WCHAR c;

        c = RtlUpcaseUnicodeChar(string.Buffer[i]);

        if ((c == UNICODE_NULL) || (c > 0x7F))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "ACPI ID %wZ is not ASCII",
                          String);

            return FALSE;
        }

        id[out++] = (CHAR)c;
    }

    if (!KphDynHwAcpiIdValid(id))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI ID %wZ is not a valid ID",
                      String);

        return FALSE;
    }

    RtlCopyMemory(Id, id, sizeof(id));

    return TRUE;
}

/**
 * \brief Finds the op table of an ACPI device by its IDs.
 *
 * \param[in] Pdo The physical device object.
 * \param[in] Section The hardware section.
 * \param[out] Id Receives the identity of the row, untouched on failure.
 * \param[out] Table Receives the op table.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwAcpiFindTable(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ PKPH_DYN_HW_SECTION Section,
    _Out_writes_(KPH_DYN_HW_ACPI_ID_LENGTH) PCHAR Id,
    _Outptr_ PKPH_DYN_HW_TABLE* Table
    )
{
    NTSTATUS status;
    PWCHAR ids;

    KPH_PAGED_CODE_PASSIVE();

    *Table = NULL;

    ids = NULL;

    for (ULONG i = 0; i < ARRAYSIZE(KphpHwAcpiIdProperties); i++)
    {
        DEVICE_REGISTRY_PROPERTY property;
        ULONG length;
        ULONG allocationLength;
        UNICODE_STRING id;
        BOOLEAN found;

        property = KphpHwAcpiIdProperties[i];
        length = 0;

        status = IoGetDeviceProperty(Pdo, property, 0, NULL, &length);
        if (status != STATUS_BUFFER_TOO_SMALL)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "IoGetDeviceProperty failed: %!STATUS!",
                          status);

            continue;
        }

        status = RtlULongAdd(length,
                             (3 * sizeof(UNICODE_NULL)),
                             &allocationLength);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "RtlULongAdd failed: %!STATUS!",
                          status);

            goto Exit;
        }

        ids = KphAllocatePaged(allocationLength, KPH_TAG_HW_ACPI_BUFFER);
        if (!ids)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "Failed to allocate ACPI ID buffer");

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        status = IoGetDeviceProperty(Pdo, property, length, ids, &length);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "IoGetDeviceProperty failed: %!STATUS!",
                          status);

            KphFree(ids, KPH_TAG_HW_ACPI_BUFFER);
            ids = NULL;
            continue;
        }

        found = FALSE;

        RtlInitUnicodeString(&id, ids);

        while (id.Length > 0)
        {
            KPH_DYN_HW_IDENTITY identity;

            RtlZeroMemory(&identity, sizeof(identity));

            if (KphpHwAcpiNormalizeId(&id, identity.AcpiHid.Id))
            {
                status = KphDynHwFindTable(Section,
                                           KPH_DYN_HW_IDENTITY_ACPI_HID,
                                           &identity,
                                           Table);
                if (NT_SUCCESS(status))
                {
                    RtlCopyMemory(Id,
                                  identity.AcpiHid.Id,
                                  KPH_DYN_HW_ACPI_ID_LENGTH);
                    found = TRUE;
                    break;
                }
            }

            RtlInitUnicodeString(&id,
                                 &id.Buffer[id.Length / sizeof(WCHAR) + 1]);
        }

        if (found)
        {
            status = STATUS_SUCCESS;
            goto Exit;
        }

        KphFree(ids, KPH_TAG_HW_ACPI_BUFFER);
        ids = NULL;
    }

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "No ID of the ACPI device is allow-listed");

    status = STATUS_NOT_FOUND;

Exit:

    if (ids)
    {
        KphFree(ids, KPH_TAG_HW_ACPI_BUFFER);
    }

    return status;
}

/**
 * \brief Matches an ACPI physical device object at a path.
 *
 * \param[in] DeviceObject A device object of the ACPI driver.
 * \param[in,out] Context The find context; receives the referenced match.
 *
 * \return TRUE when the device matched, FALSE to continue enumerating.
 */
_Function_class_(KPH_HW_ENUMERATE_DEVICES_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KSIAPI KphpHwAcpiFindPdoCallback(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PKPH_HW_ACPI_FIND_PDO context;
    PANSI_STRING path;
    BOOLEAN equal;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    context = Context;

    if (!BooleanFlagOn(DeviceObject->Flags, DO_BUS_ENUMERATED_DEVICE))
    {
        return FALSE;
    }

    status = KphpHwAcpiQueryPath(DeviceObject, &path);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwAcpiQueryPath failed: %!STATUS!",
                      status);

        return FALSE;
    }

    equal = RtlEqualString(path, context->Path, FALSE);

    KphFree(path, KPH_TAG_HW_ACPI_PATH);

    if (!equal)
    {
        return FALSE;
    }

    ObReferenceObject(DeviceObject);
    context->Pdo = DeviceObject;

    return TRUE;
}

/**
 * \brief Finds and references the ACPI physical device object at a path.
 *
 * \param[in] Path The normalized namespace path.
 * \param[out] Pdo Receives the referenced physical device object.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwAcpiFindPdo(
    _In_ PCANSI_STRING Path,
    _Outptr_ PDEVICE_OBJECT* Pdo
    )
{
    NTSTATUS status;
    PDRIVER_OBJECT driver;
    KPH_HW_ACPI_FIND_PDO context;

    KPH_PAGED_CODE_PASSIVE();

    *Pdo = NULL;

    driver = NULL;

    status = KphHwReferenceDriver(&KphpHwAcpiDriverName, &driver);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwReferenceDriver failed: %!STATUS!",
                      status);

        goto Exit;
    }

    context.Path = Path;
    context.Pdo = NULL;

    status = KphHwEnumerateDriverDevices(driver,
                                         KphpHwAcpiFindPdoCallback,
                                         &context);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwEnumerateDriverDevices failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (!context.Pdo)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "No ACPI device at %Z",
                      Path);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    *Pdo = context.Pdo;
    status = STATUS_SUCCESS;

Exit:

    if (driver)
    {
        ObDereferenceObject(driver);
    }

    return status;
}

/**
 * \brief Identifies an ACPI device by its path and IDs.
 *
 * \param[in] Pdo The physical device object.
 * \param[in] Path The normalized namespace path the device must have.
 * \param[in] Section The hardware section.
 * \param[out] Id Receives the identity of the row.
 * \param[out] Table Receives the op table.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwAcpiIdentify(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ PCANSI_STRING Path,
    _In_ PKPH_DYN_HW_SECTION Section,
    _Out_writes_(KPH_DYN_HW_ACPI_ID_LENGTH) PCHAR Id,
    _Outptr_ PKPH_DYN_HW_TABLE* Table
    )
{
    NTSTATUS status;
    PANSI_STRING path;

    KPH_PAGED_CODE_PASSIVE();

    *Table = NULL;

    path = NULL;

    status = KphpHwAcpiQueryPath(Pdo, &path);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwAcpiQueryPath failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (!RtlEqualString(path, Path, FALSE))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI device moved from %Z to %Z",
                      Path,
                      path);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    status = KphpHwAcpiFindTable(Pdo, Section, Id, Table);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwAcpiFindTable failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = STATUS_SUCCESS;

Exit:

    if (path)
    {
        KphFree(path, KPH_TAG_HW_ACPI_PATH);
    }

    return status;
}

/**
 * \brief Opens the allow-listed ACPI device at a path through the cache.
 *
 * \details A cached device is revalidated on each use. Only ExpectNoRemoval
 * devices are served, since the cache holds a PDO reference.
 *
 * \param[in] Path The normalized namespace path.
 * \param[in] Section The hardware section.
 * \param[out] Device Receives the cache entry, valid while the lock is held.
 * \param[out] Table Receives the op table.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Requires_lock_held_(KphpHwAcpiLock)
_Must_inspect_result_
NTSTATUS KphpHwAcpiOpen(
    _In_ PCANSI_STRING Path,
    _In_ PKPH_DYN_HW_SECTION Section,
    _Outptr_ PKPH_HW_ACPI_DEVICE* Device,
    _Outptr_ PKPH_DYN_HW_TABLE* Table
    )
{
    NTSTATUS status;
    PKPH_HW_ACPI_DEVICE device;
    PDEVICE_OBJECT pdo;
    CHAR id[KPH_DYN_HW_ACPI_ID_LENGTH];
    ANSI_STRING idString;
    ANSI_STRING cachedIdString;
    ULONG allocationLength;

    KPH_PAGED_CODE_PASSIVE();

    *Device = NULL;
    *Table = NULL;

    device = NULL;
    pdo = NULL;

    for (PLIST_ENTRY link = KphpHwAcpiDevices.Flink;
         link != &KphpHwAcpiDevices;
         link = link->Flink)
    {
        PKPH_HW_ACPI_DEVICE entry;

        entry = CONTAINING_RECORD(link, KPH_HW_ACPI_DEVICE, Link);

        if (RtlEqualString(&entry->Path, Path, FALSE))
        {
            device = entry;
            break;
        }
    }

    if (device)
    {
        idString.Buffer = id;
        idString.Length = sizeof(id);
        idString.MaximumLength = sizeof(id);
        cachedIdString.Buffer = device->Id;
        cachedIdString.Length = sizeof(device->Id);
        cachedIdString.MaximumLength = sizeof(device->Id);

        status = KphpHwAcpiIdentify(device->Pdo,
                                    Path,
                                    Section,
                                    id,
                                    Table);

        if (NT_SUCCESS(status) &&
            RtlEqualString(&idString, &cachedIdString, FALSE))
        {
            *Device = device;
            status = STATUS_SUCCESS;
            goto Exit;
        }

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI cache entry %Z no longer identifies: %!STATUS!",
                      &device->Path,
                      status);

        *Table = NULL;

        KphpHwAcpiClose(device);
        device = NULL;
    }

    status = KphpHwAcpiFindPdo(Path, &pdo);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwAcpiFindPdo failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphpHwAcpiIdentify(pdo, Path, Section, id, Table);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwAcpiIdentify failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (!KphHwExpectsNoRemoval(pdo))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI %Z refused: removal policy is not ExpectNoRemoval",
                      Path);

        *Table = NULL;
        status = STATUS_NOT_SUPPORTED;
        goto Exit;
    }

    NT_VERIFY(NT_SUCCESS(RtlULongAdd(sizeof(KPH_HW_ACPI_DEVICE),
                                     Path->Length,
                                     &allocationLength)));

    device = KphAllocatePaged(allocationLength, KPH_TAG_HW_ACPI_DEVICE);
    if (!device)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate ACPI cache entry");

        *Table = NULL;
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    device->Path.Buffer = Add2Ptr(device, sizeof(KPH_HW_ACPI_DEVICE));
    device->Path.Length = Path->Length;
    device->Path.MaximumLength = Path->Length;
    RtlCopyMemory(device->Path.Buffer, Path->Buffer, Path->Length);
    device->Pdo = pdo;
    pdo = NULL;
    RtlCopyMemory(device->Id, id, sizeof(device->Id));

    InsertHeadList(&KphpHwAcpiDevices, &device->Link);

    *Device = device;
    status = STATUS_SUCCESS;

Exit:

    if (pdo)
    {
        ObDereferenceObject(pdo);
    }

    return status;
}

/**
 * \brief Builds the input buffer of an evaluation.
 *
 * \param[in] Method The NameSeg.
 * \param[in] ArgCount The number of arguments.
 * \param[in] Args The composed arguments.
 * \param[out] Buffer Receives the input buffer.
 * \param[in] Length The length of the buffer.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpHwAcpiBuildInput(
    _In_ ULONG Method,
    _In_ ULONG ArgCount,
    _In_reads_(ArgCount) PULONG Args,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length
    )
{
    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(Buffer, Length);

    if (ArgCount == 0)
    {
        PACPI_EVAL_INPUT_BUFFER_V1 input;

        NT_ASSERT(Length >= sizeof(ACPI_EVAL_INPUT_BUFFER_V1));

        input = Buffer;
        input->Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
        input->MethodNameAsUlong = Method;
    }
    else if (ArgCount == 1)
    {
        PACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_V1 input;

        NT_ASSERT(Length >= sizeof(ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_V1));

        input = Buffer;
        input->Signature = ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE;
        input->MethodNameAsUlong = Method;
        input->IntegerArgument = Args[0];
    }
    else
    {
        PACPI_EVAL_INPUT_BUFFER_COMPLEX_V1 input;
        PACPI_METHOD_ARGUMENT_V1 argument;

        input = Buffer;
        input->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
        input->MethodNameAsUlong = Method;
        input->Size = Length;
        input->ArgumentCount = ArgCount;

        argument = &input->Argument[0];

        for (ULONG i = 0; i < ArgCount; i++)
        {
            argument->Type = ACPI_METHOD_ARGUMENT_INTEGER;
            argument->DataLength = sizeof(ULONG);
            argument->Argument = Args[i];

            argument = ACPI_METHOD_NEXT_ARGUMENT(argument);
        }

        NT_ASSERT((ULONG_PTR)argument == ((ULONG_PTR)Buffer + Length));
    }
}

/**
 * \brief Retrieves the length of the input buffer of an evaluation.
 *
 * \param[in] ArgCount The number of arguments.
 *
 * \return The length of the input buffer.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG KphpHwAcpiInputLength(
    _In_ ULONG ArgCount
    )
{
    ULONG headerLength;
    ULONG argumentLength;
    ULONG argumentsLength;
    ULONG inputLength;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(ArgCount <= KPH_DYN_HW_ACPI_MAX_ARGS);

    if (ArgCount == 0)
    {
        return sizeof(ACPI_EVAL_INPUT_BUFFER_V1);
    }

    if (ArgCount == 1)
    {
        return sizeof(ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_V1);
    }

    headerLength = FIELD_OFFSET(ACPI_EVAL_INPUT_BUFFER_COMPLEX_V1, Argument);
    argumentLength = (ULONG)ACPI_METHOD_ARGUMENT_LENGTH(sizeof(ULONG));

    NT_VERIFY(NT_SUCCESS(RtlULongMult(ArgCount,
                                      argumentLength,
                                      &argumentsLength)));
    NT_VERIFY(NT_SUCCESS(RtlULongAdd(headerLength,
                                     argumentsLength,
                                     &inputLength)));

    return inputLength;
}

/**
 * \brief Validates the output of an evaluation.
 *
 * \details Only the top level is checked; packages are not walked.
 *
 * \param[in] Buffer The output.
 * \param[in] Length The number of bytes the ACPI driver returned.
 * \param[in] ResultTypes The result types allowed (KPH_DYN_HW_ACPI_RESULT_*).
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwAcpiValidateOutput(
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_ UCHAR ResultTypes
    )
{
    NTSTATUS status;
    PACPI_EVAL_OUTPUT_BUFFER output;
    ULONG headerLength;
    ULONG argumentHeaderLength;
    ULONG offset;

    KPH_PAGED_CODE_PASSIVE();

    headerLength = FIELD_OFFSET(ACPI_EVAL_OUTPUT_BUFFER, Argument);
    argumentHeaderLength = FIELD_OFFSET(ACPI_METHOD_ARGUMENT_V1, Data);

    if (Length < headerLength)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI output is too short: %lu",
                      Length);

        status = STATUS_DEVICE_PROTOCOL_ERROR;
        goto Exit;
    }

    output = Buffer;

    if ((output->Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE) ||
        (output->Length < headerLength) ||
        (output->Length > Length))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI output header is malformed: 0x%08lx %lu of %lu",
                      output->Signature,
                      output->Length,
                      Length);

        status = STATUS_DEVICE_PROTOCOL_ERROR;
        goto Exit;
    }

    offset = headerLength;

    for (ULONG i = 0; i < output->Count; i++)
    {
        PACPI_METHOD_ARGUMENT_V1 argument;
        ULONG dataLength;
        ULONG argumentLength;

        if ((output->Length - offset) < argumentHeaderLength)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "ACPI output argument %lu is truncated",
                          i);

            status = STATUS_DEVICE_PROTOCOL_ERROR;
            goto Exit;
        }

        argument = Add2Ptr(output, offset);
        dataLength = max((ULONG)sizeof(ULONG), argument->DataLength);

        NT_VERIFY(NT_SUCCESS(RtlULongAdd(argumentHeaderLength,
                                         dataLength,
                                         &argumentLength)));

        if (argumentLength > (output->Length - offset))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "ACPI output argument %lu overruns: %lu",
                          i,
                          argumentLength);

            status = STATUS_DEVICE_PROTOCOL_ERROR;
            goto Exit;
        }

        if ((argument->Type > ACPI_METHOD_ARGUMENT_PACKAGE) ||
            !BooleanFlagOn(ResultTypes, 1ul << argument->Type))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "ACPI output argument %lu type %lu is not allowed",
                          i,
                          (ULONG)argument->Type);

            status = STATUS_OBJECT_TYPE_MISMATCH;
            goto Exit;
        }

        offset += argumentLength;
    }

    status = STATUS_SUCCESS;

Exit:

    return status;
}

/**
 * \brief Evaluates an allow-listed method of an ACPI device.
 *
 * \param[in] Path The namespace path of the device.
 * \param[in] OpIndex The index of the op among the device's acpi_eval ops.
 * \param[in] ArgCount The number of arguments, the op's.
 * \param[in] Args The caller values, zero past ArgCount.
 * \param[in] Length The length of the output buffer.
 * \param[out] Buffer Receives the ACPI_EVAL_OUTPUT_BUFFER bytes.
 * \param[out] ReturnLength Receives the output length, also when the buffer is
 * too small.
 * \param[in] Timeout Optional timeout for the evaluation; NULL is infinite.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwEvalAcpi(
    _In_ PANSI_STRING Path,
    _In_ ULONG OpIndex,
    _In_ ULONG ArgCount,
    _In_reads_(KPH_HW_ACPI_MAX_ARGS) PCULONG Args,
    _In_ ULONG Length,
    _Out_writes_bytes_opt_(Length) PVOID Buffer,
    _Out_opt_ PULONG ReturnLength,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PANSI_STRING path;
    ULONG values[KPH_HW_ACPI_MAX_ARGS];
    ULONG args[KPH_HW_ACPI_MAX_ARGS];
    PKPH_DYN dyn;
    BOOLEAN locked;
    PKPH_HW_ACPI_DEVICE device;
    PKPH_DYN_HW_TABLE table;
    PKPH_DYN_HW_OP op;
    PKPH_DYN_HW_ACPI_EVAL body;
    KPH_HW_ACPI_EVAL_INIT init;
    PKPH_HW_ACPI_EVAL eval;
    ULONG inputLength;
    ULONG bufferLength;
    ULONG bufferOffset;
    ULONG allocationLength;
    UCHAR resultTypes;
    USHORT maxOutput;
    PIO_STACK_LOCATION stack;
    ULONG ioControlCode;
    LARGE_INTEGER timeout;
    ULONG information;

    KPH_PAGED_CODE_PASSIVE();

    path = NULL;
    dyn = NULL;
    locked = FALSE;
    eval = NULL;

    C_ASSERT(KPH_HW_ACPI_MAX_ARGS == KPH_DYN_HW_ACPI_MAX_ARGS);

    RtlCopyMemory(values, Args, sizeof(values));

    if (ArgCount > KPH_HW_ACPI_MAX_ARGS)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI argument count %lu is out of range",
                      ArgCount);

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (!Buffer && Length)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI output buffer is NULL with length %lu",
                      Length);

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    status = KphHwAcpiCapturePath(Path, &path, AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwAcpiCapturePath failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (Timeout)
    {
        status = KphReadLargeIntegerFromMode(&timeout, Timeout, AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    for (ULONG i = ArgCount; i < ARRAYSIZE(values); i++)
    {
        if (values[i])
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "ACPI argument %lu is past the argument count %lu",
                          i,
                          ArgCount);

            status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
    }

    dyn = KphReferenceDynData();
    if (!dyn || !dyn->Hardware.Section)
    {
        status = STATUS_NOINTERFACE;
        goto Exit;
    }

    KphAcquireRWLockExclusive(&KphpHwAcpiLock);
    locked = TRUE;

    status = KphpHwAcpiOpen(path, dyn->Hardware.Section, &device, &table);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwAcpiOpen failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphDynHwFindAcpiEvalOp(table, OpIndex, &op);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphDynHwFindAcpiEvalOp failed: %!STATUS!",
                      status);

        goto Exit;
    }

    body = (PKPH_DYN_HW_ACPI_EVAL)op->Body;

    if (ArgCount != body->ArgCount)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI %Z argument count %lu, the op takes %lu",
                      path,
                      ArgCount,
                      (ULONG)body->ArgCount);

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (!KphDynHwAcpiComposeArgs(op, ArgCount, values, args))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI %Z refused: arguments not allowed by the op",
                      path);

        status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    resultTypes = body->ResultTypes;
    maxOutput = body->MaxOutput;

    inputLength = KphpHwAcpiInputLength(ArgCount);
    bufferLength = max(inputLength, (ULONG)maxOutput);
    bufferOffset = (ULONG)ALIGN_UP_BY(sizeof(KPH_HW_ACPI_EVAL),
                                      MEMORY_ALLOCATION_ALIGNMENT);

    NT_VERIFY(NT_SUCCESS(RtlULongAdd(bufferOffset,
                                     bufferLength,
                                     &allocationLength)));

    if (KphpHwAcpiIsDeviceBusy(device->Pdo))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI %Z busy: an abandoned evaluation is pending",
                      path);

        status = STATUS_DEVICE_BUSY;
        goto Exit;
    }

    init.Pdo = device->Pdo;
    init.BufferOffset = bufferOffset;

    status = KphCreateObject(KphpHwAcpiEvalType,
                             allocationLength,
                             &eval,
                             &init);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphCreateObject failed: %!STATUS!",
                      status);

        goto Exit;
    }

    KphpHwAcpiBuildInput(body->Method,
                         ArgCount,
                         args,
                         eval->Buffer,
                         inputLength);

    eval->Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    eval->Irp->IoStatus.Information = 0;
    eval->Irp->AssociatedIrp.SystemBuffer = eval->Buffer;

    ioControlCode = IOCTL_ACPI_ASYNC_EVAL_METHOD;

    stack = IoGetNextIrpStackLocation(eval->Irp);
    stack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    stack->Parameters.DeviceIoControl.IoControlCode = ioControlCode;
    stack->Parameters.DeviceIoControl.InputBufferLength = inputLength;
    stack->Parameters.DeviceIoControl.OutputBufferLength = maxOutput;

    KsiInitializeIoCompletion(&eval->Completion,
                              KphDriverObject,
                              KphpHwAcpiEvalComplete,
                              eval,
                              TRUE,
                              TRUE,
                              TRUE);

    KsiSetCompletionRoutine(eval->Irp, &eval->Completion);

    KphReferenceObject(eval);

    KphReleaseRWLock(&KphpHwAcpiLock);
    locked = FALSE;

    status = IoCallDriver(eval->Pdo, eval->Irp);

    KphTracePrint(TRACE_LEVEL_VERBOSE,
                  HARDWARE,
                  "ACPI %Z IoCallDriver: %!STATUS!",
                  path,
                  status);

    status = KeWaitForSingleObject(&eval->Event,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   (Timeout ? &timeout : NULL));
    if (status == STATUS_TIMEOUT)
    {
        BOOLEAN cancelled;
        LARGE_INTEGER zero;

        cancelled = IoCancelIrp(eval->Irp);

        zero.QuadPart = 0;

        status = KeWaitForSingleObject(&eval->Event,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       &zero);
        if (status != STATUS_SUCCESS)
        {
            KphpHwAcpiAbandonEval(eval);

            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          HARDWARE,
                          "ACPI %Z evaluation abandoned (cancelled %!bool!)",
                          path,
                          cancelled);

            status = STATUS_IO_TIMEOUT;
            goto Exit;
        }
    }

    status = eval->IoStatus.Status;

    if ((status == STATUS_BUFFER_OVERFLOW) ||
        (status == STATUS_BUFFER_TOO_SMALL))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI %Z output exceeds MaxOutput %lu: %!STATUS!",
                      path,
                      (ULONG)maxOutput,
                      status);

        status = STATUS_BUFFER_OVERFLOW;
        goto Exit;
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI %Z evaluation failed: %!STATUS!",
                      path,
                      status);

        goto Exit;
    }

    if (eval->IoStatus.Information > maxOutput)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI %Z output %I64u exceeds MaxOutput %lu",
                      path,
                      (ULONG64)eval->IoStatus.Information,
                      (ULONG)maxOutput);

        status = STATUS_BUFFER_OVERFLOW;
        goto Exit;
    }

    information = (ULONG)eval->IoStatus.Information;

    status = KphpHwAcpiValidateOutput(eval->Buffer, information, resultTypes);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwAcpiValidateOutput failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (ReturnLength)
    {
        status = KphWriteULongToMode(ReturnLength, information, AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    if (information > Length)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI output %lu exceeds the buffer %lu",
                      information,
                      Length);

        status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    }

    status = KphCopyToMode(Buffer, eval->Buffer, information, AccessMode);

Exit:

    if (eval)
    {
        KphDereferenceObject(eval);
    }

    if (locked)
    {
        KphReleaseRWLock(&KphpHwAcpiLock);
    }

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    if (path)
    {
        KphReleaseAnsiString(path);
    }

    return status;
}

/**
 * \brief Derives the physical base of an ACPI_CRS window.
 *
 * \param[in] Pdo The physical device object.
 * \param[in] Ordinal The memory descriptor ordinal (the op's BarIndex).
 * \param[in] WindowSize The window size.
 * \param[in] Mailbox TRUE for an MMIO mailbox (writable and page-aligned).
 * \param[out] Base Receives the base, untouched on failure.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwAcpiDeriveCrsBase(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ ULONG Ordinal,
    _In_ ULONG WindowSize,
    _In_ BOOLEAN Mailbox,
    _Out_ PULONG64 Base
    )
{
    NTSTATUS status;
    PCM_RESOURCE_LIST raw;
    PCM_RESOURCE_LIST translated;
    ULONG rawLength;
    ULONG translatedLength;
    ULONG ordinal;
    ULONG64 start;
    ULONG64 length;
    ULONG64 translatedStart;
    ULONG64 translatedLengthOfRange;
    USHORT flags;
    USHORT translatedFlags;
    DEVICE_REGISTRY_PROPERTY property;

    KPH_PAGED_CODE_PASSIVE();

    raw = NULL;
    translated = NULL;

    status = KphHwQueryBootConfiguration(Pdo,
                                         DevicePropertyBootConfiguration,
                                         &raw,
                                         &rawLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwQueryBootConfiguration failed: %!STATUS!",
                      status);

        goto Exit;
    }

    property = DevicePropertyBootConfigurationTranslated;

    status = KphHwQueryBootConfiguration(Pdo,
                                         property,
                                         &translated,
                                         &translatedLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwQueryBootConfiguration failed: %!STATUS!",
                      status);

        goto Exit;
    }

    ordinal = Ordinal;
    start = 0;
    flags = 0;

    if (!KphHwFindMemoryDescriptor(raw,
                                   rawLength,
                                   KPH_HW_FIND_MEMORY_BY_MEMORY_ORDINAL,
                                   &ordinal,
                                   &start,
                                   &length,
                                   &flags))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI_CRS memory descriptor %lu not found",
                      Ordinal);

        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Exit;
    }

    translatedStart = 0;
    translatedFlags = 0;

    if (!KphHwFindMemoryDescriptor(translated,
                                   translatedLength,
                                   KPH_HW_FIND_MEMORY_BY_ORDINAL,
                                   &ordinal,
                                   &translatedStart,
                                   &translatedLengthOfRange,
                                   &translatedFlags))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI_CRS translated descriptor %lu not found",
                      ordinal);

        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Exit;
    }

    if ((translatedStart != start) ||
        (translatedLengthOfRange != length) ||
        (translatedFlags != flags))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI_CRS descriptor is translated: %I64x to %I64x",
                      start,
                      translatedStart);

        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Exit;
    }

    if (!start)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI_CRS descriptor base is zero");

        status = STATUS_DEVICE_NOT_READY;
        goto Exit;
    }

    if ((length < WindowSize) || ((start % sizeof(ULONG64)) != 0))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI_CRS window %I64x+%I64x unusable for size %lu",
                      start,
                      length,
                      WindowSize);

        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Exit;
    }

    if (Mailbox &&
        (BooleanFlagOn(flags, CM_RESOURCE_MEMORY_READ_ONLY) ||
         BYTE_OFFSET(start)))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI_CRS mailbox %I64x refused: flags 0x%04lx",
                      start,
                      (ULONG)flags);

        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Exit;
    }

    *Base = start;
    status = STATUS_SUCCESS;

Exit:

    if (translated)
    {
        KphFree(translated, KPH_TAG_HW_BOOT_CONFIG);
    }

    if (raw)
    {
        KphFree(raw, KPH_TAG_HW_BOOT_CONFIG);
    }

    return status;
}

/**
 * \brief Reads a batch of allow-listed registers of an ACPI_CRS window.
 *
 * \param[in] Path The normalized namespace path of the device.
 * \param[in] Section The hardware section.
 * \param[in] OpIndex The index of the op among the device's window_read ops.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries, in system memory.
 *
 * \return STATUS_SUCCESS when the batch was processed, otherwise an errant
 * status for the whole batch.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpHwAcpiReadCrsWindow(
    _In_ PCANSI_STRING Path,
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ ULONG OpIndex,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries
    )
{
    NTSTATUS status;
    PKPH_HW_ACPI_DEVICE device;
    PKPH_DYN_HW_TABLE table;
    PKPH_DYN_HW_OP op;
    PKPH_DYN_HW_WINDOW_READ window;
    ULONG spanBegin;
    ULONG spanEnd;
    ULONG64 base;
    ULONG64 check;

    KPH_PAGED_CODE_PASSIVE();

    KphAcquireRWLockExclusive(&KphpHwAcpiLock);

    status = KphpHwAcpiOpen(Path, Section, &device, &table);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwAcpiOpen failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphDynHwFindWindowOp(table, OpIndex, &op);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphDynHwFindWindowOp failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (!KphHwWindowValidateEntries(op, Count, Entries, &spanBegin, &spanEnd))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "No window entry is allowed");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    window = (PKPH_DYN_HW_WINDOW_READ)op->Body;

    NT_ASSERT(window->BaseKind == KPH_DYN_HW_WINDOW_BASE_ACPI_CRS);

    status = KphpHwAcpiDeriveCrsBase(device->Pdo,
                                     window->BarIndex,
                                     window->WindowSize,
                                     FALSE,
                                     &base);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwAcpiDeriveCrsBase failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphHwWindowMapAndRead(op,
                                   base,
                                   spanBegin,
                                   spanEnd,
                                   Count,
                                   Entries);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwWindowMapAndRead failed: %!STATUS!",
                      status);

        goto Exit;
    }

    check = 0;

    status = KphpHwAcpiDeriveCrsBase(device->Pdo,
                                     window->BarIndex,
                                     window->WindowSize,
                                     FALSE,
                                     &check);
    if (!NT_SUCCESS(status) || (check != base))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI %Z window revalidation: %!STATUS! "
                      "moved %!bool!",
                      &device->Path,
                      status,
                      (check != base));

        for (ULONG i = 0; i < Count; i++)
        {
            if (Entries[i].Status == STATUS_SUCCESS)
            {
                Entries[i].Value = 0;
                Entries[i].Status = STATUS_DEVICE_NOT_READY;
            }
        }
    }

    status = STATUS_SUCCESS;

Exit:

    KphReleaseRWLock(&KphpHwAcpiLock);

    return status;
}

#if defined(_M_X64)

/**
 * \brief Opens the MMIO mailbox of an allow-listed ACPI device.
 *
 * \param[in] Path The normalized namespace path of the device; the session
 * refers to it, so it must outlive the session.
 * \param[in] Section The hardware section.
 * \param[in] OpIndex The index of the op among the device's mailbox_transact
 * ops.
 * \param[out] Op Receives the op, valid while the section is referenced; NULL
 * on failure.
 * \param[out] Session Receives the session, closed with KphHwAcpiCloseMailbox;
 * zeroed on failure.
 *
 * \return Successful or errant status; STATUS_NOT_FOUND also when the op's
 * gate does not hold on this processor.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwAcpiOpenMailbox(
    _In_ PCANSI_STRING Path,
    _In_ PKPH_DYN_HW_SECTION Section,
    _In_ ULONG OpIndex,
    _Outptr_ PKPH_DYN_HW_OP* Op,
    _Out_ PKPH_HW_MMIO_SESSION Session
    )
{
    NTSTATUS status;
    PKPH_HW_ACPI_DEVICE device;
    PKPH_DYN_HW_TABLE table;
    PKPH_DYN_HW_OP op;
    PKPH_DYN_HW_MAILBOX_TRANSACT mailbox;
    PKPH_DYN_HW_MAILBOX_WINDOW window;
    ULONG64 base;
    ULONG64 windowEnd;
    ULONG64 rangeEnd;
    ULONG registersEnd;
    ULONG argumentsLength;
    ULONG argumentsEnd;
    ULONG mapOffset;
    ULONG mapEnd;
    PHYSICAL_ADDRESS physical;
    PVOID mapping;

    KPH_PAGED_CODE_PASSIVE();

    C_ASSERT(KPH_DYN_HW_MAILBOX_WINDOW_PAGE == PAGE_SIZE);

    *Op = NULL;
    RtlZeroMemory(Session, sizeof(KPH_HW_MMIO_SESSION));

    KphAcquireRWLockExclusive(&KphpHwAcpiLock);

    status = KphpHwAcpiOpen(Path, Section, &device, &table);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwAcpiOpen failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphDynHwFindMailboxOp(table, OpIndex, &op);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphDynHwFindMailboxOp failed: %!STATUS!",
                      status);

        goto Exit;
    }

    mailbox = (PKPH_DYN_HW_MAILBOX_TRANSACT)op->Body;
    window = &mailbox->Address.Window;

    NT_ASSERT(mailbox->Transport == KPH_DYN_HW_MAILBOX_TRANSPORT_MMIO_WINDOW);
    NT_ASSERT(window->BaseKind == KPH_DYN_HW_WINDOW_BASE_ACPI_CRS);
    NT_ASSERT(op->Gate.Kind == KPH_DYN_HW_GATE_CPUID);

    if (!KphHwCpuidGatePasses(KphGetHardwareCpuIdentity(), &op->Gate))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Mailbox %Z refused: CPUID gate does not hold",
                      &device->Path);

        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    status = KphpHwAcpiDeriveCrsBase(device->Pdo,
                                     window->BarIndex,
                                     window->WindowSize,
                                     TRUE,
                                     &base);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwAcpiDeriveCrsBase failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = RtlULongLongAdd(base, window->WindowSize, &windowEnd);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongLongAdd failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (windowEnd > (MAXULONG64 - PAGE_SIZE))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Mailbox %Z refused: window end %I64x",
                      &device->Path,
                      windowEnd);

        status = STATUS_DEVICE_CONFIGURATION_ERROR;
        goto Exit;
    }

    rangeEnd = ROUND_TO_PAGES(windowEnd);

    status = KphHwWindowCheckRange(base, rangeEnd);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Mailbox %Z refused: %!STATUS!",
                      &device->Path,
                      status);

        goto Exit;
    }

    //
    // The parser keeps MSG, RSP and the argument registers in one page.
    //
    NT_VERIFY(NT_SUCCESS(RtlULongAdd(max(window->Msg, window->Rsp),
                                     sizeof(ULONG),
                                     &registersEnd)));
    NT_VERIFY(NT_SUCCESS(RtlULongMult(mailbox->ArgCount,
                                      sizeof(ULONG),
                                      &argumentsLength)));
    NT_VERIFY(NT_SUCCESS(RtlULongAdd(window->Arg0,
                                     argumentsLength,
                                     &argumentsEnd)));

    mapOffset = min(min(window->Msg, window->Rsp), window->Arg0);
    mapEnd = max(registersEnd, argumentsEnd);

    NT_ASSERT((mapOffset < mapEnd) && (mapEnd <= window->WindowSize));
    NT_ASSERT((mapOffset / PAGE_SIZE) == ((mapEnd - 1) / PAGE_SIZE));

    physical.QuadPart = (LONGLONG)(base + mapOffset);

    mapping = MmMapIoSpaceEx(physical,
                             mapEnd - mapOffset,
                             PAGE_READWRITE | PAGE_NOCACHE);
    if (!mapping)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, HARDWARE, "MmMapIoSpaceEx failed");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    ObReferenceObject(device->Pdo);

    Session->Path = *Path;
    Session->Pdo = device->Pdo;
    Session->Base = base;
    Session->Mapping = mapping;
    Session->MapOffset = mapOffset;
    Session->MapLength = (mapEnd - mapOffset);

    *Op = op;
    status = STATUS_SUCCESS;

Exit:

    KphReleaseRWLock(&KphpHwAcpiLock);

    return status;
}

/**
 * \brief Checks whether the window of an open MMIO mailbox moved.
 *
 * \param[in] Session The MMIO mailbox session.
 * \param[in] Op The mailbox_transact op.
 *
 * \return TRUE if the base can no longer be derived or differs from the base
 * the session mapped.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphHwAcpiMailboxMoved(
    _In_ PKPH_HW_MMIO_SESSION Session,
    _In_ PKPH_DYN_HW_OP Op
    )
{
    NTSTATUS status;
    PKPH_DYN_HW_MAILBOX_WINDOW window;
    ULONG64 base;

    KPH_PAGED_CODE_PASSIVE();
    NT_ASSERT(Session->Pdo);

    window = &((PKPH_DYN_HW_MAILBOX_TRANSACT)Op->Body)->Address.Window;

    status = KphpHwAcpiDeriveCrsBase(Session->Pdo,
                                     window->BarIndex,
                                     window->WindowSize,
                                     TRUE,
                                     &base);

    return (!NT_SUCCESS(status) || (base != Session->Base));
}

/**
 * \brief Closes an MMIO mailbox session opened by KphHwAcpiOpenMailbox.
 *
 * \param[in,out] Session The session, zeroed on return.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphHwAcpiCloseMailbox(
    _Inout_ PKPH_HW_MMIO_SESSION Session
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (Session->Mapping)
    {
        MmUnmapIoSpace(Session->Mapping, Session->MapLength);
    }

    if (Session->Pdo)
    {
        ObDereferenceObject(Session->Pdo);
    }

    RtlZeroMemory(Session, sizeof(KPH_HW_MMIO_SESSION));
}

#endif

/**
 * \brief Reads a batch of allow-listed registers of an ACPI_CRS window.
 *
 * \details The base never leaves the kernel and the mapping does not outlive
 * the request.
 *
 * \param[in] Path The namespace path of the device, not empty.
 * \param[in] OpIndex The index of the op among the window_read ops.
 * \param[in] Count The number of entries.
 * \param[in,out] Entries The batch entries.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphHwReadAcpiWindowBatch(
    _In_ PANSI_STRING Path,
    _In_ ULONG OpIndex,
    _In_ ULONG Count,
    _Inout_updates_(Count) PKPH_HW_WINDOW_BATCH_ENTRY Entries,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PANSI_STRING path;
    ULONG entriesLength;
    PKPH_HW_WINDOW_BATCH_ENTRY entries;
    PKPH_DYN dyn;

    KPH_PAGED_CODE_PASSIVE();

    path = NULL;
    entries = NULL;
    dyn = NULL;

    if (Count == 0)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "ACPI window batch is empty");

        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    status = KphHwAcpiCapturePath(Path, &path, AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphHwAcpiCapturePath failed: %!STATUS!",
                      status);

        goto Exit;
    }

    dyn = KphReferenceDynData();
    if (!dyn || !dyn->Hardware.Section)
    {
        status = STATUS_NOINTERFACE;
        goto Exit;
    }

    status = RtlULongMult(Count,
                          sizeof(KPH_HW_WINDOW_BATCH_ENTRY),
                          &entriesLength);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "RtlULongMult failed: %!STATUS!",
                      status);

        goto Exit;
    }

    entries = KphAllocatePaged(entriesLength, KPH_TAG_HW_WINDOW_BATCH);
    if (!entries)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "Failed to allocate ACPI window batch buffer");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphCopyFromMode(entries, Entries, entriesLength, AccessMode);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    status = KphpHwAcpiReadCrsWindow(path,
                                     dyn->Hardware.Section,
                                     OpIndex,
                                     Count,
                                     entries);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      HARDWARE,
                      "KphpHwAcpiReadCrsWindow failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphCopyToMode(Entries, entries, entriesLength, AccessMode);

Exit:

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    if (entries)
    {
        KphFree(entries, KPH_TAG_HW_WINDOW_BATCH);
    }

    if (path)
    {
        KphReleaseAnsiString(path);
    }

    return status;
}
