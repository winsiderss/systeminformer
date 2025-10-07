/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     jxy-s   2021-2025
 *
 */

#include <kph.h>

#include <trace.h>

typedef struct _KPH_ENUMERATE_PROCESS_HANDLES_CONTEXT
{
    PKPH_DYN Dyn;
    KPROCESSOR_MODE AccessMode;
    PVOID Buffer;
    PVOID BufferLimit;
    PVOID CurrentEntry;
    ULONG Count;
    NTSTATUS Status;
} KPH_ENUMERATE_PROCESS_HANDLES_CONTEXT, *PKPH_ENUMERATE_PROCESS_HANDLES_CONTEXT;

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const UNICODE_STRING KphpEtwRegistrationName = RTL_CONSTANT_STRING(L"EtwRegistration");
KPH_PROTECTED_DATA_SECTION_RO_POP();

_Must_inspect_result_
PVOID KphObpDecodeObject(
    _In_ PKPH_DYN Dyn,
    _In_ PHANDLE_TABLE_ENTRY HandleTableEntry
    )
{
#if defined(_M_X64) || defined(_M_ARM64)
    if (Dyn->ObDecodeShift != ULONG_MAX)
    {
        LONG_PTR object;

        object = (LONG_PTR)(HandleTableEntry->Object);

        //
        // Decode the object pointer, shift back up the lower nibble to zeros.
        // N.B. LA57 is special but the way we define HANDLE_TABLE_ENTRY and
        // use dynamic data supports it.
        //
        object >>= Dyn->ObDecodeShift;
        object <<= 4;

        return (PVOID)object;
    }
    else
    {
        return NULL;
    }
#else
    return (PVOID)((ULONG_PTR)(HandleTableEntry->Object) & ~OBJ_HANDLE_ATTRIBUTES);
#endif
}

ULONG KphObpGetHandleAttributes(
    _In_ PKPH_DYN Dyn,
    _In_ PHANDLE_TABLE_ENTRY HandleTableEntry
    )
{
#if defined(_M_X64) || defined(_M_ARM64)
    if (Dyn->ObAttributesShift != ULONG_MAX)
    {
        return (ULONG)(HandleTableEntry->Value >> Dyn->ObAttributesShift) & 0x3;
    }
    else
    {
        return 0;
    }
#else
    return (HandleTableEntry->ObAttributes & (OBJ_INHERIT | OBJ_AUDIT_OBJECT_CLOSE)) |
        ((HandleTableEntry->GrantedAccess & ObpAccessProtectCloseBit) ? OBJ_PROTECT_CLOSE : 0);
#endif
}

/**
 * \brief Retrieves information about a file object.
 *
 * \param[in] FileObject The file object to retrieve information for.
 * \param[out] Information Receives the file object information.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpGetFileObjectInformation(
    _In_ PFILE_OBJECT FileObject,
    _Out_ PKPH_FILE_OBJECT_INFORMATION Information
    )
{
    PDEVICE_OBJECT attachedDevice;
    PDEVICE_OBJECT relatedDevice;
    KIRQL irql;
    PVPB vpb;

    //
    // VPB lock will be acquired, code placed in non-paged segment on purpose.
    //
    KPH_NPAGED_CODE_PASSIVE();

    attachedDevice = IoGetAttachedDevice(FileObject->DeviceObject);
    relatedDevice = IoGetRelatedDeviceObject(FileObject);

    RtlZeroMemory(Information, sizeof(KPH_FILE_OBJECT_INFORMATION));

    Information->LockOperation = FileObject->LockOperation;
    Information->DeletePending = FileObject->DeletePending;
    Information->ReadAccess = FileObject->ReadAccess;
    Information->WriteAccess = FileObject->WriteAccess;
    Information->DeleteAccess = FileObject->DeleteAccess;
    Information->SharedRead = FileObject->SharedRead;
    Information->SharedWrite = FileObject->SharedWrite;
    Information->SharedDelete = FileObject->SharedDelete;
    Information->CurrentByteOffset = FileObject->CurrentByteOffset;
    Information->Flags = FileObject->Flags;
    if (FileObject->SectionObjectPointer)
    {
        Information->UserWritableReferences =
            MmDoesFileHaveUserWritableReferences(FileObject->SectionObjectPointer);
    }
    Information->HasActiveTransaction =
        (IoGetTransactionParameterBlock(FileObject) ? TRUE : FALSE);
    Information->IsIgnoringSharing = IoIsFileObjectIgnoringSharing(FileObject);
    Information->Waiters = FileObject->Waiters;
    Information->Busy = FileObject->Busy;

    Information->Device.Type = FileObject->DeviceObject->DeviceType;
    Information->Device.Characteristics = FileObject->DeviceObject->Characteristics;
    Information->Device.Flags = FileObject->DeviceObject->Flags;

    Information->AttachedDevice.Type = attachedDevice->DeviceType;
    Information->AttachedDevice.Characteristics = attachedDevice->Characteristics;
    Information->AttachedDevice.Flags = attachedDevice->Flags;

    Information->RelatedDevice.Type = relatedDevice->DeviceType;
    Information->RelatedDevice.Characteristics = relatedDevice->Characteristics;
    Information->RelatedDevice.Flags = relatedDevice->Flags;

    C_ASSERT(ARRAYSIZE(Information->Vpb.VolumeLabel) == ARRAYSIZE(vpb->VolumeLabel));

    IoAcquireVpbSpinLock(&irql);

    //
    // Accessing the VPB is reserved for "certain classes of drivers".
    //
#pragma prefast(push)
#pragma prefast(disable : 28175)
    vpb = FileObject->Vpb;
    if (vpb)
    {
        Information->Vpb.Type = vpb->Type;
        Information->Vpb.Size = vpb->Size;
        Information->Vpb.Flags = vpb->Flags;
        Information->Vpb.VolumeLabelLength = vpb->VolumeLabelLength;
        Information->Vpb.SerialNumber = vpb->SerialNumber;
        Information->Vpb.ReferenceCount = vpb->ReferenceCount;
        RtlCopyMemory(Information->Vpb.VolumeLabel,
                      vpb->VolumeLabel,
                      ARRAYSIZE(vpb->VolumeLabel) * sizeof(WCHAR));
    }

    vpb = FileObject->DeviceObject->Vpb;
    if (vpb)
    {
        Information->Device.Vpb.Type = vpb->Type;
        Information->Device.Vpb.Size = vpb->Size;
        Information->Device.Vpb.Flags = vpb->Flags;
        Information->Device.Vpb.VolumeLabelLength = vpb->VolumeLabelLength;
        Information->Device.Vpb.SerialNumber = vpb->SerialNumber;
        Information->Device.Vpb.ReferenceCount = vpb->ReferenceCount;
        RtlCopyMemory(Information->Device.Vpb.VolumeLabel,
                      vpb->VolumeLabel,
                      ARRAYSIZE(vpb->VolumeLabel) * sizeof(WCHAR));
    }

    vpb = attachedDevice->Vpb;
    if (vpb)
    {
        Information->AttachedDevice.Vpb.Type = vpb->Type;
        Information->AttachedDevice.Vpb.Size = vpb->Size;
        Information->AttachedDevice.Vpb.Flags = vpb->Flags;
        Information->AttachedDevice.Vpb.VolumeLabelLength = vpb->VolumeLabelLength;
        Information->AttachedDevice.Vpb.SerialNumber = vpb->SerialNumber;
        Information->AttachedDevice.Vpb.ReferenceCount = vpb->ReferenceCount;
        RtlCopyMemory(Information->AttachedDevice.Vpb.VolumeLabel,
                      vpb->VolumeLabel,
                      ARRAYSIZE(vpb->VolumeLabel) * sizeof(WCHAR));
    }

    vpb = relatedDevice->Vpb;
    if (vpb)
    {
        Information->RelatedDevice.Vpb.Type = vpb->Type;
        Information->RelatedDevice.Vpb.Size = vpb->Size;
        Information->RelatedDevice.Vpb.Flags = vpb->Flags;
        Information->RelatedDevice.Vpb.VolumeLabelLength = vpb->VolumeLabelLength;
        Information->RelatedDevice.Vpb.SerialNumber = vpb->SerialNumber;
        Information->RelatedDevice.Vpb.ReferenceCount = vpb->ReferenceCount;
        RtlCopyMemory(Information->RelatedDevice.Vpb.VolumeLabel,
                      vpb->VolumeLabel,
                      ARRAYSIZE(vpb->VolumeLabel) * sizeof(WCHAR));
    }
#pragma prefast(pop)

    IoReleaseVpbSpinLock(irql);
}

KPH_PAGED_FILE();

/**
 * \brief Gets a pointer to the handle table of a process. On success, acquires
 * process exit synchronization, the process should be released by calling
 * KphDereferenceProcessHandleTable.
 *
 * \param[in] Dyn Dynamic configuration.
 * \param[in] Process A process object.
 * \param[out] HandleTable On success set to the process handle table.
 *
 * \return Successful or errant status.
 */
_When_(NT_SUCCESS(return), _Acquires_lock_(Process))
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphReferenceProcessHandleTable(
    _In_ PKPH_DYN Dyn,
    _In_ PEPROCESS Process,
    _Outptr_result_nullonfailure_ PHANDLE_TABLE* HandleTable
    )
{
    PHANDLE_TABLE handleTable;
    NTSTATUS status;

    KPH_PAGED_CODE_PASSIVE();

    *HandleTable = NULL;

    //
    // Fail if we don't have an offset.
    //
    if (Dyn->EpObjectTable == ULONG_MAX)
    {
        return STATUS_NOINTERFACE;
    }

    //
    // Prevent the process from terminating and get its handle table.
    //
    status = PsAcquireProcessExitSynchronization(Process);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "PsAcquireProcessExitSynchronization failed: %!STATUS!",
                      status);

        return STATUS_TOO_LATE;
    }

    handleTable = *(PHANDLE_TABLE*)Add2Ptr(Process, Dyn->EpObjectTable);

    if (!handleTable)
    {
        PsReleaseProcessExitSynchronization(Process);

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "Process has no handle table.");

        return STATUS_NOT_FOUND;
    }

    *HandleTable = handleTable;

    return STATUS_SUCCESS;
}

/**
 * \brief Releases process after acquiring the process handle table.
 *
 * \param[in] Process A process object.
 */
_Releases_lock_(Process)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphDereferenceProcessHandleTable(
    _In_ PEPROCESS Process
    )
{
    KPH_PAGED_CODE_PASSIVE();

    PsReleaseProcessExitSynchronization(Process);
}

/**
 * \brief Unlocks a handle table entry.
 *
 * \param[in] Dyn Dynamic configuration.
 * \param[in] HandleTable Handle table to unlock.
 * \param[in] HandleTableEntry Handle table entry to unlock.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpUnlockHandleTableEntry(
    _In_ PKPH_DYN Dyn,
    _In_ PHANDLE_TABLE HandleTable,
    _In_ PHANDLE_TABLE_ENTRY HandleTableEntry
    )
{
    PEX_PUSH_LOCK handleContentionEvent;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Dyn->HtHandleContentionEvent != ULONG_MAX);

    //
    // Set the unlocked bit.
    //
    InterlockedExchangeAddULongPtr(&HandleTableEntry->Value, 1);

    //
    // Allow waiters to wake up.
    //
    handleContentionEvent = (PEX_PUSH_LOCK)Add2Ptr(HandleTable,
                                                   Dyn->HtHandleContentionEvent);
    if (*(PULONG_PTR)handleContentionEvent != 0)
    {
        ExfUnblockPushLock(handleContentionEvent, NULL);
    }
}

typedef struct _KPH_ENUM_PROC_HANDLE_EX_CONTEXT
{
    PKPH_DYN Dyn;
    PKPH_ENUM_PROCESS_HANDLES_CALLBACK Callback;
    PVOID Parameter;
} KPH_ENUM_PROC_HANDLE_EX_CONTEXT, *PKPH_ENUM_PROC_HANDLE_EX_CONTEXT;

/**
 * \brief Pass-through callback for handle table enumeration.
 *
 * \param[in,out] HandleTableEntry Related handle table entry.
 * \param[in] Handle The handle for this entry.
 * \param[in] Context Enumeration context.
 *
 * \return Result from wrapped callback.
 */
_Function_class_(EX_ENUM_HANDLE_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN NTAPI KphEnumerateProcessHandlesExCallback(
    _In_ PHANDLE_TABLE HandleTable,
    _Inout_ PHANDLE_TABLE_ENTRY HandleTableEntry,
    _In_ HANDLE Handle,
    _In_opt_ PVOID Context
    )
{
    PKPH_ENUM_PROC_HANDLE_EX_CONTEXT context;
    BOOLEAN result;

    KPH_PAGED_CODE_PASSIVE();

    NT_ASSERT(Context);

    context = Context;

    result = context->Callback(HandleTableEntry, Handle, context->Parameter);

    KphpUnlockHandleTableEntry(context->Dyn, HandleTable, HandleTableEntry);

    return result;
}

/**
 * \brief Enumerates the handles of a process.
 *
 * \param[in] Process The process to enumerate handles of.
 * \param[in] Callback The callback to invoke for each handle table entry.
 * \param[in] Parameter Optional parameter to pass to the callback.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS KphEnumerateProcessHandlesEx(
    _In_ PEPROCESS Process,
    _In_ PKPH_ENUM_PROCESS_HANDLES_CALLBACK Callback,
    _In_opt_ PVOID Parameter
    )
{
    NTSTATUS status;
    PKPH_DYN dyn;
    KPH_ENUM_PROC_HANDLE_EX_CONTEXT context;
    PHANDLE_TABLE handleTable;

    KPH_PAGED_CODE_PASSIVE();

    dyn = KphReferenceDynData();
    if (!dyn ||
        (dyn->HtHandleContentionEvent == ULONG_MAX) ||
        (dyn->EpObjectTable == ULONG_MAX))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphEnumerateProcessHandlesEx not supported");

        status = STATUS_NOINTERFACE;
        goto Exit;
    }

    status = KphReferenceProcessHandleTable(dyn, Process, &handleTable);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphReferenceProcessHandleTable failed: %!STATUS!",
                      status);

        goto Exit;
    }

    context.Dyn = dyn;
    context.Callback = Callback;
    context.Parameter = Parameter;

    ExEnumHandleTable(handleTable,
                      KphEnumerateProcessHandlesExCallback,
                      &context,
                      NULL);

    KphDereferenceProcessHandleTable(Process);

Exit:

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    return status;
}

/**
 * \brief Enumeration callback for handle enumeration.
 *
 * \param[in,out] HandleTableEntry Related handle table entry.
 * \param[in] Handle The handle for this entry.
 * \param[in] Context Enumeration context.
 *
 * \return FALSE
 */
_Function_class_(KPH_ENUM_PROCESS_HANDLES_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpEnumerateProcessHandlesCallbck(
    _Inout_ PHANDLE_TABLE_ENTRY HandleTableEntry,
    _In_ HANDLE Handle,
    _In_ PVOID Context
    )
{
    PKPH_ENUMERATE_PROCESS_HANDLES_CONTEXT context;
    KPH_PROCESS_HANDLE handleInfo;
    POBJECT_HEADER objectHeader;
    POBJECT_TYPE objectType;
    PKPH_PROCESS_HANDLE entryInBuffer;
    KPH_MEMORY_REGION region;

    KPH_PAGED_CODE_PASSIVE();

    context = Context;

    objectHeader = KphObpDecodeObject(context->Dyn, HandleTableEntry);
    handleInfo.Handle = Handle;
    handleInfo.Object = objectHeader ? &objectHeader->Body : NULL;
    handleInfo.GrantedAccess = ObpDecodeGrantedAccess(HandleTableEntry->GrantedAccess);
    handleInfo.ObjectTypeIndex = USHORT_MAX;
    handleInfo.Reserved1 = 0;
    handleInfo.HandleAttributes = KphObpGetHandleAttributes(context->Dyn,
                                                            HandleTableEntry);
    handleInfo.Reserved2 = 0;

    if (handleInfo.Object)
    {
        objectType = ObGetObjectType(handleInfo.Object);

        if (objectType && (context->Dyn->OtIndex != ULONG_MAX))
        {
            UCHAR typeIndex;

            typeIndex = *(PUCHAR)Add2Ptr(objectType, context->Dyn->OtIndex);

            handleInfo.ObjectTypeIndex = (USHORT)typeIndex;
        }
    }

    //
    // Advance the current entry pointer regardless of whether the information
    // will be written. This will allow the parent function to report the
    // correct return length.
    //
    entryInBuffer = context->CurrentEntry;
    context->CurrentEntry = Add2Ptr(context->CurrentEntry, sizeof(KPH_PROCESS_HANDLE));
    context->Count++;

    //
    // Only write if we have not exceeded the buffer length. Also check for a
    // potential overflow (if the process has an extremely large number of
    // handles, the buffer pointer may wrap).
    //
    region.Start = entryInBuffer;
    region.End = Add2Ptr(entryInBuffer, sizeof(KPH_PROCESS_HANDLE));
    if (KphIsValidMemoryRegion(&region, context->Buffer, context->BufferLimit))
    {
        context->Status = KphCopyToMode(entryInBuffer,
                                        &handleInfo,
                                        sizeof(KPH_PROCESS_HANDLE),
                                        context->AccessMode);
        if (!NT_SUCCESS(context->Status))
        {
            return TRUE;
        }
    }
    else
    {
        if (context->Status == STATUS_SUCCESS)
        {
            context->Status = STATUS_BUFFER_TOO_SMALL;
        }
    }

    return FALSE;
}

/**
 * \brief Enumerates the handles of a process.
 *
 * \param[in] ProcessHandle A handle to a process.
 * \param[out] Buffer The buffer in which the handle information will be stored.
 * \param[in] BufferLength The number of bytes available in Buffer.
 * \param[out] ReturnLength Receives the number of bytes written or required.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphEnumerateProcessHandles(
    _In_ HANDLE ProcessHandle,
    _Out_writes_bytes_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PKPH_DYN dyn;
    PEPROCESS process;
    KPH_ENUMERATE_PROCESS_HANDLES_CONTEXT context;

    KPH_PAGED_CODE_PASSIVE();

    dyn = NULL;
    process = NULL;

    if (!Buffer)
    {
        status = STATUS_INVALID_PARAMETER_2;
        goto Exit;
    }

    dyn = KphReferenceDynData();
    if (!dyn)
    {
        status = STATUS_NOINTERFACE;
        goto Exit;
    }

    status = ObReferenceObjectByHandle(ProcessHandle,
                                       0,
                                       *PsProcessType,
                                       AccessMode,
                                       &process,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        goto Exit;
    }

    context.Dyn = dyn;
    context.AccessMode = AccessMode;
    context.Buffer = Buffer;
    context.BufferLimit = Add2Ptr(Buffer, BufferLength);
    context.CurrentEntry = ((PKPH_PROCESS_HANDLE_INFORMATION)Buffer)->Handles;
    context.Count = 0;
    context.Status = STATUS_SUCCESS;

    status = KphEnumerateProcessHandlesEx(process,
                                          KphpEnumerateProcessHandlesCallbck,
                                          &context);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphEnumerateProcessHandlesEx failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (BufferLength >= UFIELD_OFFSET(KPH_PROCESS_HANDLE_INFORMATION, Handles))
    {
        PKPH_PROCESS_HANDLE_INFORMATION info;

        info = Buffer;

        status = KphWriteULongToMode(&info->HandleCount,
                                     context.Count,
                                     AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    if (ReturnLength)
    {
        ULONG_PTR returnLength;

        status = RtlULongPtrSub((ULONG_PTR)context.CurrentEntry,
                                (ULONG_PTR)Buffer,
                                &returnLength);
        if (!NT_SUCCESS(status) || (returnLength > ULONG_MAX))
        {
            status = STATUS_INTEGER_OVERFLOW;
            goto Exit;
        }

        status = KphWriteULongToMode(ReturnLength,
                                     (ULONG)returnLength,
                                     AccessMode);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }
    }

    status = context.Status;

Exit:

    if (process)
    {
        ObDereferenceObject(process);
    }

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    return status;
}

/**
 * \brief Queries the name of an object.
 *
 * \param[in] Object A pointer to an object.
 * \param[out] Buffer The buffer in which the object name will be stored.
 * \param[in] BufferLength The number of bytes available in Buffer.
 * \param[out] ReturnLength Receives the number of bytes written.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryNameObject(
    _In_ PVOID Object,
    _Out_writes_bytes_opt_(BufferLength) POBJECT_NAME_INFORMATION Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    POBJECT_TYPE objectType;

    KPH_PAGED_CODE();

    objectType = ObGetObjectType(Object);

    if (objectType == *IoFileObjectType)
    {
        status = KphQueryNameFileObject(Object,
                                        Buffer,
                                        BufferLength,
                                        ReturnLength);

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphQueryNameFileObject: %!STATUS!",
                      status);
    }
    else
    {
        status = ObQueryNameString(Object, Buffer, BufferLength, ReturnLength);

        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObQueryNameString: %!STATUS!",
                      status);
    }

    //
    // Make the error returns consistent.
    //
    if ((status == STATUS_BUFFER_OVERFLOW) ||    // returned by I/O subsystem
        (status == STATUS_INFO_LENGTH_MISMATCH)) // returned by ObQueryNameString
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (NT_SUCCESS(status))
    {
        NT_ASSERT(Buffer);
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphQueryNameObject: %wZ",
                      &Buffer->Name);
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphQueryNameObject: %!STATUS!",
                      status);
    }

    return status;
}

/**
 * \brief Extracts the name of a file object by retrieving the device name,
 * walking the related file objects, and the target file object.
 *
 * \param[in] FileObject A pointer to a file object.
 * \param[out] Buffer The buffer in which the object name will be stored.
 * \param[in] BufferLength The number of bytes available in Buffer.
 * \param[out] ReturnLength Receives the number of bytes written or required.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphpExtractNameFileObject(
    _In_ PFILE_OBJECT FileObject,
    _Out_writes_bytes_opt_(BufferLength) POBJECT_NAME_INFORMATION Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    ULONG returnLength;
    PFILE_OBJECT fileObject;
    PUCHAR pos;
    USHORT len;

    KPH_PAGED_CODE();

    *ReturnLength = 0;

    if (!Buffer && BufferLength)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (FlagOn(FileObject->Flags, FO_CLEANUP_COMPLETE))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, GENERAL, "File object closed");

        return STATUS_FILE_CLOSED;
    }

    if (FileObject->DeviceObject)
    {
        returnLength = 0;

        status = ObQueryNameString(FileObject->DeviceObject,
                                   Buffer,
                                   BufferLength,
                                   &returnLength);

        if (NT_SUCCESS(status))
        {
            NT_ASSERT(Buffer && BufferLength);
            NT_ASSERT(returnLength >= sizeof(OBJECT_NAME_INFORMATION));
            NT_ASSERT(Buffer->Name.Buffer == Add2Ptr(Buffer, sizeof(OBJECT_NAME_INFORMATION)));
        }
        else if (status == STATUS_INFO_LENGTH_MISMATCH)
        {
            NT_ASSERT(returnLength);
        }
        else
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "ObQueryNameString failed: %!STATUS!",
                          status);

            return status;
        }
    }
    else
    {
        if (Buffer && (BufferLength >= sizeof(OBJECT_NAME_INFORMATION)))
        {
            Buffer->Name.Length = 0;
            Buffer->Name.Buffer = Add2Ptr(Buffer, sizeof(OBJECT_NAME_INFORMATION));
        }

        returnLength = sizeof(OBJECT_NAME_INFORMATION);
    }

    //
    // Walk the file object and related objects. This is walking the name in
    // reverse order. Place the name at the end of the buffer to be moved into
    // place afterwards. If along the way we exhaust the buffer we'll continue
    // to calculate the needed length but not write to the buffer.
    //

    fileObject = FileObject;
    pos = Buffer ? Add2Ptr(Buffer, BufferLength) : NULL;
    len = 0;

    do
    {
        //
        // N.B. There are some drivers that aren't as tidy with their related
        // file objects (condrv.sys) and leave a dangling pointer. Admittedly
        // we're doing a "non-blessed" thing by extracting the name like this.
        // So we sanity check that the pointer is a valid file object to work
        // around this, it's not perfect but we get value from this routine
        // for cases where extracting the full name is otherwise impossible.
        //
        if ((ObGetObjectType(fileObject) != *IoFileObjectType) ||
            FlagOn(fileObject->Flags, FO_CLEANUP_COMPLETE))
        {
            break;
        }

        status = RtlULongAdd(returnLength,
                             fileObject->FileName.Length,
                             &returnLength);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "RtlULongAdd failed: %!STATUS!",
                          status);

            return status;
        }

        status = RtlUShortAdd(len, fileObject->FileName.Length, &len);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "RtlUShortAdd failed: %!STATUS!",
                          status);

            return status;
        }

        if (Buffer && (returnLength <= BufferLength))
        {
            pos -= fileObject->FileName.Length;
            RtlCopyMemory(pos,
                          fileObject->FileName.Buffer,
                          fileObject->FileName.Length);
        }

        fileObject = fileObject->RelatedFileObject;

    } while (fileObject);

    *ReturnLength = returnLength;

    if (!Buffer || (returnLength > BufferLength))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlMoveMemory(Add2Ptr(Buffer->Name.Buffer, Buffer->Name.Length), pos, len);

    status = RtlUShortAdd(Buffer->Name.Length, len, &Buffer->Name.Length);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "RtlUShortAdd failed: %!STATUS!",
                      status);

        return status;
    }

    if ((BufferLength - returnLength) > sizeof(UNICODE_NULL))
    {
        Buffer->Name.Buffer[Buffer->Name.Length / sizeof(WCHAR)] = UNICODE_NULL;
        returnLength += sizeof(UNICODE_NULL);
    }

    NT_ASSERT(returnLength >= sizeof(OBJECT_NAME_INFORMATION));

    returnLength -= sizeof(OBJECT_NAME_INFORMATION);

    NT_ASSERT(returnLength <= USHORT_MAX);

    Buffer->Name.MaximumLength = (USHORT)returnLength;

    return STATUS_SUCCESS;
}

/**
 * \brief Queries the name of a file object.
 *
 * \param[in] FileObject A pointer to a file object.
 * \param[out] Buffer The buffer in which the object name will be stored.
 * \param[in] BufferLength The number of bytes available in Buffer.
 * \param[out] ReturnLength Receives the number of bytes written or required.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryNameFileObject(
    _In_ PFILE_OBJECT FileObject,
    _Out_writes_bytes_opt_(BufferLength) POBJECT_NAME_INFORMATION Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    PFLT_FILE_NAME_INFORMATION fileNameInfo;
    FLT_FILE_NAME_OPTIONS nameOptions;

    KPH_PAGED_CODE();

    nameOptions = FLT_FILE_NAME_NORMALIZED;

    if (IoGetTopLevelIrp() || KeAreAllApcsDisabled())
    {
        nameOptions |= FLT_FILE_NAME_QUERY_CACHE_ONLY;
    }
    else
    {
        nameOptions |= FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP;
    }

    status = FltGetFileNameInformationUnsafe(FileObject,
                                             NULL,
                                             nameOptions,
                                             &fileNameInfo);
    if (!NT_SUCCESS(status))
    {
        fileNameInfo = NULL;

        status = KphpExtractNameFileObject(FileObject,
                                           Buffer,
                                           BufferLength,
                                           ReturnLength);
        goto Exit;
    }

    *ReturnLength = sizeof(OBJECT_NAME_INFORMATION);
    *ReturnLength += fileNameInfo->Name.Length;
    if (!Buffer || (BufferLength < *ReturnLength))
    {
        status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    }

    Buffer->Name.Length = 0;
    Buffer->Name.MaximumLength = (USHORT)(BufferLength - sizeof(OBJECT_NAME_INFORMATION));
    Buffer->Name.Buffer = (PWSTR)Add2Ptr(Buffer, sizeof(OBJECT_NAME_INFORMATION));

    RtlCopyUnicodeString(&Buffer->Name, &fileNameInfo->Name);

Exit:

    if (fileNameInfo)
    {
        FltReleaseFileNameInformation(fileNameInfo);
    }

    return status;
}

/**
 * \brief Queries object information.
 *
 * \param[in] ProcessHandle A handle to a process.
 * \param[in] Handle A handle which is present in the process.
 * \param[in] ObjectInformationClass The type of information to retrieve.
 * \param[out] ObjectInformation Information buffer to populate.
 * \param[in] ObjectInformationLength Length of the information buffer in bytes.
 * \param[out] ReturnLength Receives the number of bytes written or required.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryInformationObject(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _Out_writes_bytes_opt_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PKPH_DYN dyn;
    PEPROCESS process;
    KAPC_STATE apcState;
    PVOID object;
    ULONG returnLength;
    PVOID buffer;
    BYTE stackBuffer[64];
    KPROCESSOR_MODE accessMode;
    PKPH_PROCESS_CONTEXT processContext;

    KPH_PAGED_CODE_PASSIVE();

    dyn = NULL;
    returnLength = 0;
    object = NULL;
    buffer = NULL;
    processContext = NULL;

    status = ObReferenceObjectByHandle(ProcessHandle,
                                       0,
                                       *PsProcessType,
                                       AccessMode,
                                       &process,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed %!STATUS!",
                      status);

        goto Exit;
    }

    if (process == PsInitialSystemProcess)
    {
        Handle = MakeKernelHandle(Handle);
        accessMode = KernelMode;
    }
    else
    {
        if (IsKernelHandle(Handle) && !IsPseudoHandle(Handle))
        {
            status = STATUS_INVALID_HANDLE;
            goto Exit;
        }
        accessMode = AccessMode;
    }

    switch (ObjectInformationClass)
    {
        case KphObjectBasicInformation:
        {
            OBJECT_BASIC_INFORMATION basicInfo;

            if (!ObjectInformation ||
                (ObjectInformationLength < sizeof(OBJECT_BASIC_INFORMATION)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(OBJECT_BASIC_INFORMATION);
                goto Exit;
            }

            KeStackAttachProcess(process, &apcState);
            status = ZwQueryObject(Handle,
                                   ObjectBasicInformation,
                                   &basicInfo,
                                   sizeof(OBJECT_BASIC_INFORMATION),
                                   NULL);
            KeUnstackDetachProcess(&apcState);
            if (NT_SUCCESS(status))
            {
                status = KphCopyToMode(ObjectInformation,
                                       &basicInfo,
                                       sizeof(OBJECT_BASIC_INFORMATION),
                                       AccessMode);
                if (NT_SUCCESS(status))
                {
                    returnLength = sizeof(OBJECT_BASIC_INFORMATION);
                }
            }

            break;
        }
        case KphObjectNameInformation:
        {
            ULONG allocateSize;
            POBJECT_NAME_INFORMATION nameInfo;

            KeStackAttachProcess(process, &apcState);
            status = ObReferenceObjectByHandle(Handle,
                                               0,
                                               NULL,
                                               accessMode,
                                               &object,
                                               NULL);
            KeUnstackDetachProcess(&apcState);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "ObReferenceObjectByHandle failed: %!STATUS!",
                              status);

                object = NULL;
                goto Exit;
            }

            allocateSize = ObjectInformationLength;

            if (allocateSize < sizeof(OBJECT_NAME_INFORMATION))
            {
                allocateSize = sizeof(OBJECT_NAME_INFORMATION);
            }

            buffer = KphAllocatePagedA(allocateSize,
                                       KPH_TAG_OBJECT_QUERY,
                                       stackBuffer);
            if (!buffer)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
            }

            nameInfo = (POBJECT_NAME_INFORMATION)buffer;

            status = KphQueryNameObject(object,
                                        nameInfo,
                                        ObjectInformationLength,
                                        &returnLength);
            if (NT_SUCCESS(status))
            {
                if (!ObjectInformation)
                {
                    status = STATUS_BUFFER_TOO_SMALL;
                    goto Exit;
                }

                RebaseUnicodeString(&nameInfo->Name,
                                    nameInfo,
                                    ObjectInformation);

                status = KphCopyToMode(ObjectInformation,
                                       nameInfo,
                                       returnLength,
                                       AccessMode);
            }

            break;
        }
        case KphObjectTypeInformation:
        {
            ULONG allocateSize;
            POBJECT_TYPE_INFORMATION typeInfo;

            returnLength = sizeof(OBJECT_TYPE_INFORMATION);
            allocateSize = ObjectInformationLength;
            if (allocateSize < sizeof(OBJECT_TYPE_INFORMATION))
            {
                allocateSize = sizeof(OBJECT_TYPE_INFORMATION);
            }

            //
            // ObQueryTypeInfo uses ObjectType->Name.MaximumLength instead of
            // ObjectType->Name.Length + sizeof(WCHAR) to calculate the
            // required buffer size. In Windows 8, certain object types
            // (e.g. TmTx) do NOT include the null terminator in MaximumLength,
            // which causes ObQueryTypeInfo to overrun the given buffer. To
            // work around this bug, we add some (generous) padding to our
            // allocation.
            //
            allocateSize += sizeof(ULONG64);

            buffer = KphAllocatePagedA(allocateSize,
                                       KPH_TAG_OBJECT_QUERY,
                                       stackBuffer);
            if (!buffer)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
            }

            typeInfo = (POBJECT_TYPE_INFORMATION)buffer;

            KeStackAttachProcess(process, &apcState);
            status = ZwQueryObject(Handle,
                                   ObjectTypeInformation,
                                   typeInfo,
                                   ObjectInformationLength,
                                   &returnLength);
            KeUnstackDetachProcess(&apcState);
            if (NT_SUCCESS(status))
            {
                if (!ObjectInformation)
                {
                    status = STATUS_BUFFER_TOO_SMALL;
                    goto Exit;
                }

                RebaseUnicodeString(&typeInfo->TypeName,
                                    typeInfo,
                                    ObjectInformation);

                status = KphCopyToMode(ObjectInformation,
                                       typeInfo,
                                       returnLength,
                                       AccessMode);
            }

            break;
        }
        case KphObjectHandleFlagInformation:
        {
            OBJECT_HANDLE_FLAG_INFORMATION handleFlagInfo;

            if (!ObjectInformation ||
                (ObjectInformationLength < sizeof(OBJECT_HANDLE_FLAG_INFORMATION)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(OBJECT_HANDLE_FLAG_INFORMATION);
                goto Exit;
            }

            KeStackAttachProcess(process, &apcState);
            status = ZwQueryObject(Handle,
                                   ObjectHandleFlagInformation,
                                   &handleFlagInfo,
                                   sizeof(OBJECT_HANDLE_FLAG_INFORMATION),
                                   NULL);
            KeUnstackDetachProcess(&apcState);
            if (NT_SUCCESS(status))
            {
                status = KphCopyToMode(ObjectInformation,
                                       &handleFlagInfo,
                                       sizeof(OBJECT_HANDLE_FLAG_INFORMATION),
                                       AccessMode);
                if (NT_SUCCESS(status))
                {
                    returnLength = sizeof(OBJECT_HANDLE_FLAG_INFORMATION);
                }
            }

            break;
        }
        case KphObjectProcessBasicInformation:
        {
            PROCESS_BASIC_INFORMATION basicInfo;

            if (!ObjectInformation ||
                (ObjectInformationLength < sizeof(PROCESS_BASIC_INFORMATION)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(PROCESS_BASIC_INFORMATION);
                goto Exit;
            }

            KeStackAttachProcess(process, &apcState);
            status = ZwQueryInformationProcess(Handle,
                                               ProcessBasicInformation,
                                               &basicInfo,
                                               sizeof(PROCESS_BASIC_INFORMATION),
                                               NULL);
            KeUnstackDetachProcess(&apcState);
            if (NT_SUCCESS(status))
            {
                status = KphCopyToMode(ObjectInformation,
                                       &basicInfo,
                                       sizeof(PROCESS_BASIC_INFORMATION),
                                       AccessMode);
                if (NT_SUCCESS(status))
                {
                    returnLength = sizeof(PROCESS_BASIC_INFORMATION);
                }
            }

            break;
        }
        case KphObjectThreadBasicInformation:
        {
            THREAD_BASIC_INFORMATION basicInfo;

            if (!ObjectInformation ||
                (ObjectInformationLength < sizeof(THREAD_BASIC_INFORMATION)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(THREAD_BASIC_INFORMATION);
                goto Exit;
            }

            KeStackAttachProcess(process, &apcState);
            status = ZwQueryInformationThread(Handle,
                                              ThreadBasicInformation,
                                              &basicInfo,
                                              sizeof(THREAD_BASIC_INFORMATION),
                                              NULL);
            KeUnstackDetachProcess(&apcState);
            if (NT_SUCCESS(status))
            {
                status = KphCopyToMode(ObjectInformation,
                                       &basicInfo,
                                       sizeof(THREAD_BASIC_INFORMATION),
                                       AccessMode);
                if (NT_SUCCESS(status))
                {
                    returnLength = sizeof(THREAD_BASIC_INFORMATION);
                }
            }

            break;
        }
        case KphObjectEtwRegBasicInformation:
        {
            PVOID objectType;
            PUNICODE_STRING objectTypeName;
            PVOID guidEntry;
            KPH_ETWREG_BASIC_INFORMATION basicInfo;

            dyn = KphReferenceDynData();
            if (!dyn ||
                (dyn->EgeGuid == ULONG_MAX) ||
                (dyn->EreGuidEntry == ULONG_MAX) ||
                (dyn->OtName == ULONG_MAX))
            {
                status = STATUS_NOINTERFACE;
                goto Exit;
            }

            if (!ObjectInformation ||
                (ObjectInformationLength < sizeof(KPH_ETWREG_BASIC_INFORMATION)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(KPH_ETWREG_BASIC_INFORMATION);
                goto Exit;
            }

            KeStackAttachProcess(process, &apcState);
            status = ObReferenceObjectByHandle(Handle,
                                               0,
                                               NULL,
                                               accessMode,
                                               &object,
                                               NULL);
            KeUnstackDetachProcess(&apcState);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "ObReferenceObjectByHandle failed: %!STATUS!",
                              status);

                object = NULL;
                goto Exit;
            }

            objectType = ObGetObjectType(object);
            if (!objectType)
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "ObGetObjectType returned null");

                status = STATUS_NOT_SUPPORTED;
                goto Exit;
            }

            objectTypeName = (PUNICODE_STRING)Add2Ptr(objectType, dyn->OtName);
            if (!RtlEqualUnicodeString(objectTypeName,
                                       &KphpEtwRegistrationName,
                                       FALSE))
            {
                status = STATUS_OBJECT_TYPE_MISMATCH;
                goto Exit;
            }

            guidEntry = *(PVOID*)Add2Ptr(object, dyn->EreGuidEntry);
            if (guidEntry)
            {
                RtlCopyMemory(&basicInfo.Guid,
                              Add2Ptr(guidEntry, dyn->EgeGuid),
                              sizeof(GUID));
            }
            else
            {
                RtlZeroMemory(&basicInfo.Guid, sizeof(GUID));
            }

            //
            // Not implemented.
            //
            basicInfo.SessionId = 0;

            status = KphCopyToMode(ObjectInformation,
                                   &basicInfo,
                                   sizeof(KPH_ETWREG_BASIC_INFORMATION),
                                   AccessMode);
            if (NT_SUCCESS(status))
            {
                returnLength = sizeof(KPH_ETWREG_BASIC_INFORMATION);
            }

            break;
        }
        case KphObjectFileObjectInformation:
        {
            KPH_FILE_OBJECT_INFORMATION fileInfo;

            if (!ObjectInformation ||
                (ObjectInformationLength < sizeof(KPH_FILE_OBJECT_INFORMATION)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(KPH_FILE_OBJECT_INFORMATION);
                goto Exit;
            }

            KeStackAttachProcess(process, &apcState);
            status = ObReferenceObjectByHandle(Handle,
                                               0,
                                               *IoFileObjectType,
                                               accessMode,
                                               &object,
                                               NULL);
            KeUnstackDetachProcess(&apcState);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "ObReferenceObjectByHandle failed: %!STATUS!",
                              status);

                object = NULL;
                goto Exit;
            }

            KphpGetFileObjectInformation(object, &fileInfo);

            status = KphCopyToMode(ObjectInformation,
                                   &fileInfo,
                                   sizeof(KPH_FILE_OBJECT_INFORMATION),
                                   AccessMode);
            if (NT_SUCCESS(status))
            {
                returnLength = sizeof(KPH_FILE_OBJECT_INFORMATION);
            }

            break;
        }
        case KphObjectFileObjectDriver:
        {
            PFILE_OBJECT fileObject;
            HANDLE driverHandle;

            if (!ObjectInformation ||
                (ObjectInformationLength < sizeof(HANDLE)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(HANDLE);
                goto Exit;
            }

            KeStackAttachProcess(process, &apcState);
            status = ObReferenceObjectByHandle(Handle,
                                               0,
                                               *IoFileObjectType,
                                               accessMode,
                                               &object,
                                               NULL);
            KeUnstackDetachProcess(&apcState);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "ObReferenceObjectByHandle failed: %!STATUS!",
                              status);

                object = NULL;
                goto Exit;
            }

            fileObject = (PFILE_OBJECT)object;
            if (!fileObject->DeviceObject ||
                !fileObject->DeviceObject->DriverObject)
            {
                status = STATUS_INVALID_DEVICE_REQUEST;
                goto Exit;
            }

            status = ObOpenObjectByPointer(fileObject->DeviceObject->DriverObject,
                                           (AccessMode ? 0 : OBJ_KERNEL_HANDLE),
                                           NULL,
                                           SYNCHRONIZE,
                                           *IoDriverObjectType,
                                           AccessMode,
                                           &driverHandle);
            if (NT_SUCCESS(status))
            {
                status = KphWriteHandleToMode(ObjectInformation,
                                              driverHandle,
                                              AccessMode);
                if (NT_SUCCESS(status))
                {
                    returnLength = sizeof(HANDLE);
                }
            }

            break;
        }
        case KphObjectProcessTimes:
        {
            KERNEL_USER_TIMES times;

            if (!ObjectInformation ||
                (ObjectInformationLength < sizeof(KERNEL_USER_TIMES)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(KERNEL_USER_TIMES);
                goto Exit;
            }

            KeStackAttachProcess(process, &apcState);
            status = ZwQueryInformationProcess(Handle,
                                               ProcessTimes,
                                               &times,
                                               sizeof(KERNEL_USER_TIMES),
                                               NULL);
            KeUnstackDetachProcess(&apcState);
            if (NT_SUCCESS(status))
            {
                status = KphCopyToMode(ObjectInformation,
                                       &times,
                                       sizeof(KERNEL_USER_TIMES),
                                       AccessMode);
                if (NT_SUCCESS(status))
                {
                    returnLength = sizeof(KERNEL_USER_TIMES);
                }
            }

            break;
        }
        case KphObjectThreadTimes:
        {
            KERNEL_USER_TIMES times;

            if (!ObjectInformation ||
                (ObjectInformationLength < sizeof(KERNEL_USER_TIMES)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(KERNEL_USER_TIMES);
                goto Exit;
            }

            KeStackAttachProcess(process, &apcState);
            status = ZwQueryInformationThread(Handle,
                                              ThreadTimes,
                                              &times,
                                              sizeof(KERNEL_USER_TIMES),
                                              NULL);
            KeUnstackDetachProcess(&apcState);
            if (NT_SUCCESS(status))
            {
                status = KphCopyToMode(ObjectInformation,
                                       &times,
                                       sizeof(KERNEL_USER_TIMES),
                                       AccessMode);
                if (NT_SUCCESS(status))
                {
                    returnLength = sizeof(KERNEL_USER_TIMES);
                }
            }

            break;
        }
        case KphObjectProcessImageFileName:
        {
            PEPROCESS targetProcess;

            KeStackAttachProcess(process, &apcState);
            status = ObReferenceObjectByHandle(Handle,
                                               0,
                                               *PsProcessType,
                                               accessMode,
                                               &targetProcess,
                                               NULL);
            KeUnstackDetachProcess(&apcState);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "ObReferenceObjectByHandle failed: %!STATUS!",
                              status);

                goto Exit;
            }

            processContext = KphGetEProcessContext(targetProcess);

            ObDereferenceObject(targetProcess);

            if (!processContext || !processContext->ImageFileName)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
            }

            status = KphCopyUnicodeStringToMode(ObjectInformation,
                                                ObjectInformationLength,
                                                processContext->ImageFileName,
                                                &returnLength,
                                                AccessMode);

            break;
        }
        case KphObjectThreadNameInformation:
        {
            ULONG allocateSize;
            PTHREAD_NAME_INFORMATION nameInfo;

            returnLength = sizeof(THREAD_NAME_INFORMATION);
            allocateSize = ObjectInformationLength;
            if (allocateSize < sizeof(THREAD_NAME_INFORMATION))
            {
                allocateSize = sizeof(THREAD_NAME_INFORMATION);
            }
            allocateSize += sizeof(ULONG64);

            buffer = KphAllocatePagedA(allocateSize,
                                       KPH_TAG_OBJECT_QUERY,
                                       stackBuffer);
            if (!buffer)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
            }

            nameInfo = (PTHREAD_NAME_INFORMATION)buffer;

            KeStackAttachProcess(process, &apcState);
            status = ZwQueryInformationThread(Handle,
                                              ThreadNameInformation,
                                              nameInfo,
                                              ObjectInformationLength,
                                              &returnLength);
            KeUnstackDetachProcess(&apcState);
            if (NT_SUCCESS(status))
            {
                if (!ObjectInformation)
                {
                    status = STATUS_BUFFER_TOO_SMALL;
                    goto Exit;
                }

                RebaseUnicodeString(&nameInfo->ThreadName,
                                    nameInfo,
                                    ObjectInformation);

                status = KphCopyToMode(ObjectInformation,
                                       nameInfo,
                                       returnLength,
                                       AccessMode);
            }

            break;
        }
        case KphObjectThreadIsTerminated:
        {
            ULONG threadIsTerminated;

            if (!ObjectInformation || (ObjectInformationLength < sizeof(ULONG)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(ULONG);
                goto Exit;
            }

            KeStackAttachProcess(process, &apcState);
            status = ZwQueryInformationThread(Handle,
                                              ThreadIsTerminated,
                                              &threadIsTerminated,
                                              sizeof(ULONG),
                                              NULL);
            KeUnstackDetachProcess(&apcState);
            if (NT_SUCCESS(status))
            {
                status = KphWriteULongToMode(ObjectInformation,
                                             threadIsTerminated,
                                             AccessMode);
                if (NT_SUCCESS(status))
                {
                    returnLength = sizeof(ULONG);
                }
            }

            break;
        }
        case KphObjectSectionBasicInformation:
        case KphObjectSectionImageInformation:
        case KphObjectSectionRelocationInformation:
        case KphObjectSectionOriginalBaseInformation:
        case KphObjectSectionInternalImageInformation:
        case KphObjectSectionMappingsInformation:
        {
            if (ObjectInformation)
            {
                buffer = KphAllocatePagedA(ObjectInformationLength,
                                           KPH_TAG_OBJECT_QUERY,
                                           stackBuffer);
                if (!buffer)
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Exit;
                }
            }
            else
            {
                NT_ASSERT(!buffer);
                ObjectInformationLength = 0;
            }

            if (ObjectInformationClass == KphObjectSectionMappingsInformation)
            {
                KeStackAttachProcess(process, &apcState);
                status = KphQuerySection(Handle,
                                         KphSectionMappingsInformation,
                                         buffer,
                                         ObjectInformationLength,
                                         &returnLength,
                                         KernelMode);
                KeUnstackDetachProcess(&apcState);
                if (NT_SUCCESS(status))
                {
                    status = KphCopyToMode(ObjectInformation,
                                           buffer,
                                           returnLength,
                                           AccessMode);
                }
            }
            else
            {
                SIZE_T length;
                SECTION_INFORMATION_CLASS sectionInfoClass;

                if (ObjectInformationClass == KphObjectSectionBasicInformation)
                {
                    sectionInfoClass = SectionBasicInformation;
                }
                else if (ObjectInformationClass == KphObjectSectionImageInformation)
                {
                    sectionInfoClass = SectionImageInformation;
                }
                else if (ObjectInformationClass == KphObjectSectionRelocationInformation)
                {
                    sectionInfoClass = SectionRelocationInformation;
                }
                else if (ObjectInformationClass == KphObjectSectionOriginalBaseInformation)
                {
                    sectionInfoClass = SectionOriginalBaseInformation;
                }
                else
                {
                    NT_ASSERT(ObjectInformationClass == KphObjectSectionInternalImageInformation);
                    sectionInfoClass = SectionInternalImageInformation;
                }

                length = 0;
                KeStackAttachProcess(process, &apcState);
                status = ZwQuerySection(Handle,
                                        sectionInfoClass,
                                        buffer,
                                        ObjectInformationLength,
                                        &length);
                KeUnstackDetachProcess(&apcState);
                if (NT_SUCCESS(status))
                {
                    if (length > ULONG_MAX)
                    {
                        status = STATUS_INTEGER_OVERFLOW;
                        returnLength = 0;
                        goto Exit;
                    }

                    status = KphCopyToMode(ObjectInformation,
                                           buffer,
                                           length,
                                           AccessMode);
                    if (NT_SUCCESS(status))
                    {
                        returnLength = (ULONG)length;
                    }
                }
            }

            break;
        }
        case KphObjectSectionFileName:
        {
            PUNICODE_STRING sectionFileName;
            ULONG allocateSize;
            HANDLE sectionHandle;
            PVOID baseAddress;
            SIZE_T viewSize;

            returnLength = sizeof(UNICODE_STRING);
            allocateSize = ObjectInformationLength;
            if (allocateSize < sizeof(UNICODE_STRING))
            {
                allocateSize = sizeof(UNICODE_STRING);
            }
            allocateSize += sizeof(ULONG64);

            buffer = KphAllocatePagedA(allocateSize,
                                       KPH_TAG_OBJECT_QUERY,
                                       stackBuffer);
            if (!buffer)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
            }

            sectionFileName = (PUNICODE_STRING)buffer;

            status = ObDuplicateObject(process,
                                       Handle,
                                       PsInitialSystemProcess,
                                       &sectionHandle,
                                       SECTION_QUERY | SECTION_MAP_READ,
                                       OBJ_KERNEL_HANDLE,
                                       0,
                                       KernelMode);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "ObDuplicateObject failed: %!STATUS!",
                              status);

                goto Exit;
            }

            KeStackAttachProcess(PsInitialSystemProcess, &apcState);

            baseAddress = NULL;
            viewSize = PAGE_SIZE;
            status = ZwMapViewOfSection(sectionHandle,
                                        ZwCurrentProcess(),
                                        &baseAddress,
                                        0,
                                        0,
                                        NULL,
                                        &viewSize,
                                        ViewUnmap,
                                        0,
                                        PAGE_READONLY);
            if (NT_SUCCESS(status))
            {
                SIZE_T length;

                length = 0;
                status = ZwQueryVirtualMemory(ZwCurrentProcess(),
                                              baseAddress,
                                              MemoryMappedFilenameInformation,
                                              sectionFileName,
                                              ObjectInformationLength,
                                              &length);
                if (length > ULONG_MAX)
                {
                    status = STATUS_INTEGER_OVERFLOW;
                    returnLength = 0;
                }
                else
                {
                    returnLength = (ULONG)length;
                }

                ZwUnmapViewOfSection(ZwCurrentProcess(), baseAddress);
            }

            KeUnstackDetachProcess(&apcState);

            ObCloseHandle(sectionHandle, KernelMode);

            if (NT_SUCCESS(status))
            {
                if (!ObjectInformation)
                {
                    status = STATUS_BUFFER_TOO_SMALL;
                    goto Exit;
                }

                RebaseUnicodeString(sectionFileName,
                                    sectionFileName,
                                    ObjectInformation);

                status = KphCopyToMode(ObjectInformation,
                                       sectionFileName,
                                       returnLength,
                                       AccessMode);
            }

            break;
        }
        case KphObjectAttributesInformation:
        {
            PKPH_OBJECT_ATTRIBUTES_INFORMATION attributesInfo;

            if (!ObjectInformation ||
                (ObjectInformationLength < sizeof(KPH_OBJECT_ATTRIBUTES_INFORMATION)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(KPH_OBJECT_ATTRIBUTES_INFORMATION);
                goto Exit;
            }

            KeStackAttachProcess(process, &apcState);
            status = ObReferenceObjectByHandle(Handle,
                                               0,
                                               NULL,
                                               accessMode,
                                               &object,
                                               NULL);
            KeUnstackDetachProcess(&apcState);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "ObReferenceObjectByHandle failed: %!STATUS!",
                              status);

                object = NULL;
                goto Exit;
            }

            attributesInfo = ObjectInformation;

            C_ASSERT(sizeof(KPH_OBJECT_ATTRIBUTES_INFORMATION) == sizeof(UCHAR));

            status = KphWriteUCharToMode(&attributesInfo->Flags,
                                         OBJECT_TO_OBJECT_HEADER(object)->Flags,
                                         AccessMode);
            if (NT_SUCCESS(status))
            {
                returnLength = sizeof(KPH_OBJECT_ATTRIBUTES_INFORMATION);
            }

            break;
        }
        default:
        {
            status = STATUS_INVALID_INFO_CLASS;
            break;
        }
    }

Exit:

    if (processContext)
    {
        KphDereferenceObject(processContext);
    }

    if (buffer)
    {
        KphFreeA(buffer, KPH_TAG_OBJECT_QUERY, stackBuffer);
    }

    if (object)
    {
        ObDereferenceObject(object);
    }

    if (process)
    {
        ObDereferenceObject(process);
    }

    if (dyn)
    {
        KphDereferenceObject(dyn);
    }

    if (ReturnLength)
    {
        KphWriteULongToMode(ReturnLength, returnLength, AccessMode);
    }

    return status;
}

/**
 * \brief Sets object information.
 *
 * \param[in] ProcessHandle A handle to a process.
 * \param[in] Handle A handle which is present in the process.
 * \param[in] ObjectInformationClass The type of information to set.
 * \param[in] ObjectInformation A buffer which contains the information to set.
 * \param[in] ObjectInformationLength Length of the information buffer in bytes.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphSetInformationObject(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _In_reads_bytes_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PVOID objectInformation;
    BYTE stackBuffer[64];
    PEPROCESS process;
    KAPC_STATE apcState;

    KPH_PAGED_CODE_PASSIVE();

    objectInformation = NULL;
    process = NULL;

    if (!ObjectInformation)
    {
        status = STATUS_INVALID_PARAMETER_4;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        objectInformation = KphAllocatePagedA(ObjectInformationLength,
                                              KPH_TAG_OBJECT_INFO,
                                              stackBuffer);
        if (!objectInformation)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "Failed to allocate object info buffer.");

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        __try
        {
            CopyFromUser(objectInformation,
                         ObjectInformation,
                         ObjectInformationLength);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }
    else
    {
        objectInformation = ObjectInformation;
    }

    status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_SET_INFORMATION,
                                       *PsProcessType,
                                       AccessMode,
                                       &process,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        process = NULL;
        goto Exit;
    }

    if (process == PsInitialSystemProcess)
    {
        Handle = MakeKernelHandle(Handle);
    }
    else
    {
        if (IsKernelHandle(Handle))
        {
            status = STATUS_INVALID_HANDLE;
            goto Exit;
        }
    }

    switch (ObjectInformationClass)
    {
        case KphObjectHandleFlagInformation:
        {
            if (ObjectInformationLength < sizeof(OBJECT_HANDLE_FLAG_INFORMATION))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                goto Exit;
            }

            KeStackAttachProcess(process, &apcState);
            status = ObSetHandleAttributes(Handle, objectInformation, KernelMode);
            KeUnstackDetachProcess(&apcState);

            break;
        }
        default:
        {
            status = STATUS_INVALID_INFO_CLASS;
            break;
        }
    }

Exit:

    if (objectInformation && (objectInformation != ObjectInformation))
    {
        KphFreeA(objectInformation, KPH_TAG_OBJECT_QUERY, stackBuffer);
    }

    if (process)
    {
        ObDereferenceObject(process);
    }

    return status;
}

/**
 * \brief Opens a named object.
 *
 * \param[out] ObjectHandle Set to the opened handle.
 * \param[in] DesiredAccess Desires access to the object.
 * \param[in] ObjectAttributes Attributes to open the object.
 * \param[in] ObjectType Type of object to open.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphOpenNamedObject(
    _Out_ PHANDLE ObjectHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ POBJECT_TYPE ObjectType,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    HANDLE objectHandle;

    KPH_PAGED_CODE_PASSIVE();

    status = ObOpenObjectByName(ObjectAttributes,
                                ObjectType,
                                AccessMode,
                                NULL,
                                DesiredAccess,
                                NULL,
                                &objectHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObOpenObjectByName failed: %!STATUS!",
                      status);

        objectHandle = NULL;
        goto Exit;
    }

    status = KphWriteHandleToMode(ObjectHandle, objectHandle, AccessMode);

Exit:

    return status;
}

/**
 * \brief Duplicates an object.
 *
 * \param[in] ProcessHandle Handle to process where the source handle exists.
 * \param[in] SourceHandle Source handle in the target process.
 * \param[in] DesiredAccess Desired access for duplicated handle.
 * \param[out] TargetHandle Populated with the duplicated handle.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphDuplicateObject(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE SourceHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TargetHandle,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS sourceProcess;
    PEPROCESS targetProcess;
    HANDLE targetHandle;

    KPH_PAGED_CODE_PASSIVE();

    targetProcess = NULL;
    sourceProcess = NULL;

    if (DesiredAccess & ~(READ_CONTROL | ACCESS_SYSTEM_SECURITY | SYNCHRONIZE))
    {
        status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    if (!TargetHandle)
    {
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    status = ObReferenceObjectByHandle(ProcessHandle,
                                       0,
                                       *PsProcessType,
                                       AccessMode,
                                       &sourceProcess,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed %!STATUS!",
                      status);

        sourceProcess = NULL;
        goto Exit;
    }

    if (sourceProcess == PsInitialSystemProcess)
    {
        SourceHandle = MakeKernelHandle(SourceHandle);
    }
    else
    {
        if (IsKernelHandle(SourceHandle))
        {
            status = STATUS_INVALID_HANDLE;
            goto Exit;
        }
    }

    status = ObReferenceObjectByHandle(ZwCurrentProcess(),
                                       0,
                                       *PsProcessType,
                                       AccessMode,
                                       &targetProcess,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed %!STATUS!",
                      status);

        targetProcess = NULL;
        goto Exit;
    }

    status = ObDuplicateObject(sourceProcess,
                               SourceHandle,
                               targetProcess,
                               &targetHandle,
                               DesiredAccess,
                               (AccessMode ? 0 : OBJ_KERNEL_HANDLE),
                               0,
                               KernelMode);
    if (NT_SUCCESS(status))
    {
        status = KphWriteHandleToMode(TargetHandle, targetHandle, AccessMode);
    }

Exit:

    if (targetProcess)
    {
        ObDereferenceObject(targetProcess);
    }

    if (sourceProcess)
    {
        ObDereferenceObject(sourceProcess);
    }

    return status;
}

/**
 * \brief Checks if two object handles refer to the same object.
 *
 * \param[in] FirstObjectHandle First handle to compare with the second.
 * \param[in] SecondObjectHandle Second handle to compare with the first.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return STATUS_SUCCESS if the object handles refer to the same object,
 * STATUS_NOT_SAME_OBJECT if the object handles refer to different objects,
 * and STATUS_INVALID_HANDLE if one of the handles is invalid.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpCompareObjects(
    _In_ HANDLE FirstObjectHandle,
    _In_ HANDLE SecondObjectHandle,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PVOID firstObject;
    PVOID secondObject;

    KPH_PAGED_CODE_PASSIVE();

    status = ObReferenceObjectByHandle(FirstObjectHandle,
                                       0,
                                       NULL,
                                       AccessMode,
                                       &firstObject,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed %!STATUS!",
                      status);

        firstObject = NULL;
        secondObject = NULL;
        goto Exit;
    }

    status = ObReferenceObjectByHandle(SecondObjectHandle,
                                       0,
                                       NULL,
                                       AccessMode,
                                       &secondObject,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed %!STATUS!",
                      status);

        secondObject = NULL;
        goto Exit;
    }

    status = (firstObject == secondObject) ?
        STATUS_SUCCESS : STATUS_NOT_SAME_OBJECT;

Exit:

    if (secondObject)
    {
        ObDereferenceObject(secondObject);
    }

    if (firstObject)
    {
        ObDereferenceObject(firstObject);
    }

    return status;
}

/**
 * \brief Checks if two object handles refer to the same object, for a process.
 *
 * \param[in] ProcessHandle A handle to a process.
 * \param[in] FirstObjectHandle First handle to compare with the second.
 * \param[in] SecondObjectHandle Second handle to compare with the first.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return STATUS_SUCCESS if the object handles refer to the same object,
 * STATUS_NOT_SAME_OBJECT if the object handles refer to different objects,
 * and STATUS_INVALID_HANDLE if one of the handles is invalid.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphCompareObjects(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE FirstObjectHandle,
    _In_ HANDLE SecondObjectHandle,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;
    KPROCESSOR_MODE accessMode;
    KAPC_STATE apcState;

    KPH_PAGED_CODE_PASSIVE();

    status = ObReferenceObjectByHandle(ProcessHandle,
                                       0,
                                       *PsProcessType,
                                       AccessMode,
                                       &process,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed %!STATUS!",
                      status);

        process = NULL;
        goto Exit;
    }

    if (process == PsInitialSystemProcess)
    {
        FirstObjectHandle = MakeKernelHandle(FirstObjectHandle);
        SecondObjectHandle = MakeKernelHandle(SecondObjectHandle);
        accessMode = KernelMode;
    }
    else
    {
        accessMode = AccessMode;
    }

    KeStackAttachProcess(process, &apcState);
    status = KphpCompareObjects(FirstObjectHandle,
                                SecondObjectHandle,
                                accessMode);
    KeUnstackDetachProcess(&apcState);

Exit:

    if (process)
    {
        ObDereferenceObject(process);
    }

    return status;
}
