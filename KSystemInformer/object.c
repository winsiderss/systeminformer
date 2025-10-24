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
    if ((ULONG_PTR)entryInBuffer >= (ULONG_PTR)context->Buffer &&
        (ULONG_PTR)entryInBuffer + sizeof(KPH_PROCESS_HANDLE) <= (ULONG_PTR)context->BufferLimit)
    {
        __try
        {
            *entryInBuffer = handleInfo;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Report an error.
            //
            if (context->Status == STATUS_SUCCESS)
            {
                context->Status = GetExceptionCode();
            }
        }
    }
    else
    {
        //
        // Report that the buffer is too small.
        //
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
 * \param[in] BufferLength The number of bytes available in \a Buffer.
 * \param[out] ReturnLength A variable which receives the number of bytes
 * required to be available in \a Buffer.
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

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeOutputBytes(Buffer, BufferLength);

            if (ReturnLength)
            {
                ProbeOutputType(ReturnLength, ULONG);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
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
        if (AccessMode != KernelMode)
        {
            __try
            {
                ((PKPH_PROCESS_HANDLE_INFORMATION)Buffer)->HandleCount = context.Count;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
                goto Exit;
            }
        }
        else
        {
            ((PKPH_PROCESS_HANDLE_INFORMATION)Buffer)->HandleCount = context.Count;
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

        if (AccessMode != KernelMode)
        {
            __try
            {
                *ReturnLength = (ULONG)returnLength;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
                goto Exit;
            }
        }
        else
        {
            *ReturnLength = (ULONG)returnLength;
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
 * \param[in] BufferLength The number of bytes available in \a Buffer.
 * \param[out] ReturnLength A variable which receives the number of bytes
 * required to be available in \a Buffer.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
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

    KPH_PAGED_CODE_PASSIVE();

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
 * \param[in] BufferLength The number of bytes available in \a Buffer.
 * \param[out] ReturnLength A variable which receives the number of bytes
 * required to be available in \a Buffer.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpExtractNameFileObject(
    _In_ PFILE_OBJECT FileObject,
    _Out_writes_bytes_opt_(BufferLength) POBJECT_NAME_INFORMATION Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    PCHAR objectName;
    ULONG usedLength;
    ULONG subNameLength;
    PFILE_OBJECT relatedFileObject;

    KPH_PAGED_CODE_PASSIVE();

    if (FlagOn(FileObject->Flags, FO_CLEANUP_COMPLETE))
    {
        *ReturnLength = 0;

        return STATUS_FILE_CLOSED;
    }

    if (Buffer)
    {
        if (BufferLength < sizeof(OBJECT_NAME_INFORMATION))
        {
            *ReturnLength = sizeof(OBJECT_NAME_INFORMATION);

            return STATUS_BUFFER_TOO_SMALL;
        }

        //
        // We will place the object name directly after the UNICODE_STRING
        // structure in the buffer.
        //
        Buffer->Name.Length = 0;
        Buffer->Name.MaximumLength = (USHORT)(BufferLength - sizeof(OBJECT_NAME_INFORMATION));
        Buffer->Name.Buffer = (PWSTR)Add2Ptr(Buffer, sizeof(OBJECT_NAME_INFORMATION));

        //
        // Retain a local pointer to the object name so we can manipulate the
        // pointer.
        //
        objectName = (PCHAR)Buffer->Name.Buffer;
        usedLength = sizeof(OBJECT_NAME_INFORMATION);
    }
    else
    {
        objectName = NULL;
        usedLength = sizeof(OBJECT_NAME_INFORMATION);
    }

    //
    // Check if the file object has an associated device (e.g.
    // "\Device\NamedPipe", "\Device\Mup"). We can use the user-supplied buffer
    // for this since if the buffer isn't big enough, we can't proceed anyway
    // (we are going to use the name).
    //
    if (FileObject->DeviceObject)
    {
        ULONG returnLength;

        status = ObQueryNameString(FileObject->DeviceObject,
                                   Buffer,
                                   BufferLength,
                                   &returnLength);

        usedLength = returnLength;

        if (NT_SUCCESS(status))
        {
            NT_ASSERT(objectName && Buffer);
            //
            // The UNICODE_STRING in the buffer is now filled in. We will
            // append to the object name later, so we need to fix the
            // object name pointer // by adding the length, in bytes, of
            // the device name string we just got.
            //
            objectName += Buffer->Name.Length;
        }
        else
        {
            if (status == STATUS_INFO_LENGTH_MISMATCH)
            {
                status = STATUS_BUFFER_TOO_SMALL;
            }
        }
    }
    else
    {
        status = STATUS_SUCCESS;
    }

    //
    // Check if the file object has a file name component. If not, we can't do
    // anything else, so we just return the name we have already.
    //
    if (!FileObject->FileName.Buffer)
    {
        if (!objectName || (usedLength > BufferLength))
        {
            status = STATUS_BUFFER_TOO_SMALL;
        }

        *ReturnLength = usedLength;

        return status;
    }

    //
    // The file object has a name. We need to walk up the file object chain
    // and append the names of the related file objects in reverse order. This
    // means we need to calculate the total length first.
    //

    relatedFileObject = FileObject;
    subNameLength = 0;

    do
    {
        if (FlagOn(relatedFileObject->Flags, FO_CLEANUP_COMPLETE))
        {
            break;
        }

        subNameLength += relatedFileObject->FileName.Length;

        relatedFileObject = relatedFileObject->RelatedFileObject;

    } while (relatedFileObject);

    usedLength += subNameLength;

    //
    // Check if we have enough space to write the whole thing.
    //
    if (!objectName || (usedLength > BufferLength))
    {
        *ReturnLength = usedLength;

        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // We're ready to begin copying the names.
    // Add the name length because we're copying in reverse order.
    //

    objectName += subNameLength;

    relatedFileObject = FileObject;

    do
    {
        if (FlagOn(relatedFileObject->Flags, FO_CLEANUP_COMPLETE))
        {
            break;
        }

        objectName -= relatedFileObject->FileName.Length;
        RtlCopyMemory(objectName,
                      relatedFileObject->FileName.Buffer,
                      relatedFileObject->FileName.Length);

        relatedFileObject = relatedFileObject->RelatedFileObject;

    } while (relatedFileObject);

    Buffer->Name.Length += (USHORT)subNameLength;
    Buffer->Name.MaximumLength = (USHORT)(BufferLength - sizeof(OBJECT_NAME_INFORMATION));
    *ReturnLength = usedLength;

    return STATUS_SUCCESS;
}

/**
 * \brief Queries the name of a file object.
 *
 * \param[in] FileObject A pointer to a file object.
 * \param[out] Buffer The buffer in which the object name will be stored.
 * \param[in] BufferLength The number of bytes available in \a Buffer.
 * \param[out] ReturnLength A variable which receives the number of bytes
 * required to be available in \a Buffer.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
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

    KPH_PAGED_CODE_PASSIVE();

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
 * \param[in] Handle A handle which is present in the process referenced by
 * \a ProcessHandle.
 * \param[in] ObjectInformationClass The type of information to retrieve.
 * \param[out] ObjectInformation The buffer in which the information will be
 * stored.
 * \param[in] ObjectInformationLength The number of bytes available in \a
 * ObjectInformation.
 * \param[out] ReturnLength A variable which receives the number of bytes
 * required to be available in \a ObjectInformation.
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
    process = NULL;
    returnLength = 0;
    object = NULL;
    buffer = NULL;
    processContext = NULL;

    if (AccessMode != KernelMode)
    {
        __try
        {
            if (ObjectInformation)
            {
                ProbeOutputBytes(ObjectInformation, ObjectInformationLength);
            }

            if (ReturnLength)
            {
                ProbeOutputType(ReturnLength, ULONG);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
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
                (ObjectInformationLength < sizeof(basicInfo)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(basicInfo);
                goto Exit;
            }

            KeStackAttachProcess(process, &apcState);
            status = ZwQueryObject(Handle,
                                   ObjectBasicInformation,
                                   &basicInfo,
                                   sizeof(basicInfo),
                                   NULL);
            KeUnstackDetachProcess(&apcState);
            if (NT_SUCCESS(status))
            {
                __try
                {
                    RtlCopyMemory(ObjectInformation,
                                  &basicInfo,
                                  sizeof(basicInfo));
                    returnLength = sizeof(basicInfo);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    status = GetExceptionCode();
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

                __try
                {
                    RtlCopyMemory(ObjectInformation,
                                  nameInfo,
                                  returnLength);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    status = GetExceptionCode();
                }
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

                __try
                {
                    RtlCopyMemory(ObjectInformation,
                                  typeInfo,
                                  returnLength);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    status = GetExceptionCode();
                }
            }

            break;
        }
        case KphObjectHandleFlagInformation:
        {
            OBJECT_HANDLE_FLAG_INFORMATION handleFlagInfo;

            if (!ObjectInformation ||
                (ObjectInformationLength < sizeof(handleFlagInfo)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(handleFlagInfo);
                goto Exit;
            }

            KeStackAttachProcess(process, &apcState);
            status = ZwQueryObject(Handle,
                                   ObjectHandleFlagInformation,
                                   &handleFlagInfo,
                                   sizeof(handleFlagInfo),
                                   NULL);
            KeUnstackDetachProcess(&apcState);
            if (NT_SUCCESS(status))
            {
                __try
                {
                    RtlCopyMemory(ObjectInformation,
                                  &handleFlagInfo,
                                  sizeof(handleFlagInfo));
                    returnLength = sizeof(handleFlagInfo);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    status = GetExceptionCode();
                }
            }

            break;
        }
        case KphObjectProcessBasicInformation:
        {
            PROCESS_BASIC_INFORMATION basicInfo;

            if (!ObjectInformation ||
                (ObjectInformationLength < sizeof(basicInfo)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(basicInfo);
                goto Exit;
            }

            KeStackAttachProcess(process, &apcState);
            status = ZwQueryInformationProcess(Handle,
                                               ProcessBasicInformation,
                                               &basicInfo,
                                               sizeof(basicInfo),
                                               NULL);
            KeUnstackDetachProcess(&apcState);
            if (NT_SUCCESS(status))
            {
                __try
                {
                    RtlCopyMemory(ObjectInformation,
                                  &basicInfo,
                                  sizeof(basicInfo));
                    returnLength = sizeof(basicInfo);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    status = GetExceptionCode();
                }
            }

            break;
        }
        case KphObjectThreadBasicInformation:
        {
            THREAD_BASIC_INFORMATION basicInfo;

            if (!ObjectInformation ||
                (ObjectInformationLength < sizeof(basicInfo)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(basicInfo);
                goto Exit;
            }

            KeStackAttachProcess(process, &apcState);
            status = ZwQueryInformationThread(Handle,
                                              ThreadBasicInformation,
                                              &basicInfo,
                                              sizeof(basicInfo),
                                              NULL);
            KeUnstackDetachProcess(&apcState);
            if (NT_SUCCESS(status))
            {
                __try
                {
                    RtlCopyMemory(ObjectInformation,
                                  &basicInfo,
                                  sizeof(basicInfo));
                    returnLength = sizeof(basicInfo);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    status = GetExceptionCode();
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
                (ObjectInformationLength < sizeof(basicInfo)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(basicInfo);
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

            __try
            {
                RtlCopyMemory(ObjectInformation,
                              &basicInfo,
                              sizeof(basicInfo));
                returnLength = sizeof(basicInfo);
                status = STATUS_SUCCESS;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
            }

            break;
        }
        case KphObjectFileObjectInformation:
        {
            PFILE_OBJECT fileObject;
            KPH_FILE_OBJECT_INFORMATION fileInfo;
            PDEVICE_OBJECT attachedDevice;
            PDEVICE_OBJECT relatedDevice;
            PVPB vpb;
            KIRQL irql;

            if (!ObjectInformation ||
                (ObjectInformationLength < sizeof(fileInfo)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(fileInfo);
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
            attachedDevice = IoGetAttachedDevice(fileObject->DeviceObject);
            relatedDevice = IoGetRelatedDeviceObject(fileObject);

            RtlZeroMemory(&fileInfo, sizeof(fileInfo));

            fileInfo.LockOperation = fileObject->LockOperation;
            fileInfo.DeletePending = fileObject->DeletePending;
            fileInfo.ReadAccess = fileObject->ReadAccess;
            fileInfo.WriteAccess = fileObject->WriteAccess;
            fileInfo.DeleteAccess = fileObject->DeleteAccess;
            fileInfo.SharedRead = fileObject->SharedRead;
            fileInfo.SharedWrite = fileObject->SharedWrite;
            fileInfo.SharedDelete = fileObject->SharedDelete;
            fileInfo.CurrentByteOffset = fileObject->CurrentByteOffset;
            fileInfo.Flags = fileObject->Flags;
            if (fileObject->SectionObjectPointer)
            {
                fileInfo.UserWritableReferences =
                    MmDoesFileHaveUserWritableReferences(fileObject->SectionObjectPointer);
            }
            fileInfo.HasActiveTransaction =
                (IoGetTransactionParameterBlock(fileObject) ? TRUE : FALSE);
            fileInfo.IsIgnoringSharing = IoIsFileObjectIgnoringSharing(fileObject);
            fileInfo.Waiters = fileObject->Waiters;
            fileInfo.Busy = fileObject->Busy;

            fileInfo.Device.Type = fileObject->DeviceObject->DeviceType;
            fileInfo.Device.Characteristics = fileObject->DeviceObject->Characteristics;
            fileInfo.Device.Flags = fileObject->DeviceObject->Flags;

            fileInfo.AttachedDevice.Type = attachedDevice->DeviceType;
            fileInfo.AttachedDevice.Characteristics = attachedDevice->Characteristics;
            fileInfo.AttachedDevice.Flags = attachedDevice->Flags;

            fileInfo.RelatedDevice.Type = attachedDevice->DeviceType;
            fileInfo.RelatedDevice.Characteristics = attachedDevice->Characteristics;
            fileInfo.RelatedDevice.Flags = attachedDevice->Flags;

            C_ASSERT(ARRAYSIZE(fileInfo.Vpb.VolumeLabel) == ARRAYSIZE(vpb->VolumeLabel));

            IoAcquireVpbSpinLock(&irql);

            //
            // Accessing the VPB is reserved for "certain classes of drivers".
            //
#pragma prefast(push)
#pragma prefast(disable : 28175)
            vpb = fileObject->Vpb;
            if (vpb)
            {
                fileInfo.Vpb.Type = vpb->Type;
                fileInfo.Vpb.Size = vpb->Size;
                fileInfo.Vpb.Flags = vpb->Flags;
                fileInfo.Vpb.VolumeLabelLength = vpb->VolumeLabelLength;
                fileInfo.Vpb.SerialNumber = vpb->SerialNumber;
                fileInfo.Vpb.ReferenceCount = vpb->ReferenceCount;
                RtlCopyMemory(fileInfo.Vpb.VolumeLabel,
                              vpb->VolumeLabel,
                              ARRAYSIZE(vpb->VolumeLabel) * sizeof(WCHAR));
            }

            vpb = fileObject->DeviceObject->Vpb;
            if (vpb)
            {
                fileInfo.Device.Vpb.Type = vpb->Type;
                fileInfo.Device.Vpb.Size = vpb->Size;
                fileInfo.Device.Vpb.Flags = vpb->Flags;
                fileInfo.Device.Vpb.VolumeLabelLength = vpb->VolumeLabelLength;
                fileInfo.Device.Vpb.SerialNumber = vpb->SerialNumber;
                fileInfo.Device.Vpb.ReferenceCount = vpb->ReferenceCount;
                RtlCopyMemory(fileInfo.Device.Vpb.VolumeLabel,
                              vpb->VolumeLabel,
                              ARRAYSIZE(vpb->VolumeLabel) * sizeof(WCHAR));
            }

            vpb = attachedDevice->Vpb;
            if (vpb)
            {
                fileInfo.AttachedDevice.Vpb.Type = vpb->Type;
                fileInfo.AttachedDevice.Vpb.Size = vpb->Size;
                fileInfo.AttachedDevice.Vpb.Flags = vpb->Flags;
                fileInfo.AttachedDevice.Vpb.VolumeLabelLength = vpb->VolumeLabelLength;
                fileInfo.AttachedDevice.Vpb.SerialNumber = vpb->SerialNumber;
                fileInfo.AttachedDevice.Vpb.ReferenceCount = vpb->ReferenceCount;
                RtlCopyMemory(fileInfo.AttachedDevice.Vpb.VolumeLabel,
                              vpb->VolumeLabel,
                              ARRAYSIZE(vpb->VolumeLabel) * sizeof(WCHAR));
            }

            vpb = relatedDevice->Vpb;
            if (vpb)
            {
                fileInfo.RelatedDevice.Vpb.Type = vpb->Type;
                fileInfo.RelatedDevice.Vpb.Size = vpb->Size;
                fileInfo.RelatedDevice.Vpb.Flags = vpb->Flags;
                fileInfo.RelatedDevice.Vpb.VolumeLabelLength = vpb->VolumeLabelLength;
                fileInfo.RelatedDevice.Vpb.SerialNumber = vpb->SerialNumber;
                fileInfo.RelatedDevice.Vpb.ReferenceCount = vpb->ReferenceCount;
                RtlCopyMemory(fileInfo.RelatedDevice.Vpb.VolumeLabel,
                              vpb->VolumeLabel,
                              ARRAYSIZE(vpb->VolumeLabel) * sizeof(WCHAR));
            }
#pragma prefast(pop)

            IoReleaseVpbSpinLock(irql);

            __try
            {
                RtlCopyMemory(ObjectInformation, &fileInfo, sizeof(fileInfo));
                returnLength = sizeof(fileInfo);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
            }

            break;
        }
        case KphObjectFileObjectDriver:
        {
            PFILE_OBJECT fileObject;
            HANDLE driverHandle;

            if (!ObjectInformation ||
                (ObjectInformationLength < sizeof(KPH_FILE_OBJECT_DRIVER)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(KPH_FILE_OBJECT_DRIVER);
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
                __try
                {
                    ((PKPH_FILE_OBJECT_DRIVER)ObjectInformation)->DriverHandle = driverHandle;
                    returnLength = sizeof(KPH_FILE_OBJECT_DRIVER);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    ObCloseHandle(driverHandle, AccessMode);
                    status = GetExceptionCode();
                }
            }

            break;
        }
        case KphObjectProcessTimes:
        {
            KERNEL_USER_TIMES times;

            if (!ObjectInformation ||
                (ObjectInformationLength < sizeof(times)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(times);
                goto Exit;
            }

            KeStackAttachProcess(process, &apcState);
            status = ZwQueryInformationProcess(Handle,
                                               ProcessTimes,
                                               &times,
                                               sizeof(times),
                                               NULL);
            KeUnstackDetachProcess(&apcState);
            if (NT_SUCCESS(status))
            {
                __try
                {
                    RtlCopyMemory(ObjectInformation, &times, sizeof(times));
                    returnLength = sizeof(times);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    status = GetExceptionCode();
                }
            }

            break;
        }
        case KphObjectThreadTimes:
        {
            KERNEL_USER_TIMES times;

            if (!ObjectInformation ||
                (ObjectInformationLength < sizeof(times)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(times);
                goto Exit;
            }

            KeStackAttachProcess(process, &apcState);
            status = ZwQueryInformationThread(Handle,
                                              ThreadTimes,
                                              &times,
                                              sizeof(times),
                                              NULL);
            KeUnstackDetachProcess(&apcState);
            if (NT_SUCCESS(status))
            {
                __try
                {
                    RtlCopyMemory(ObjectInformation, &times, sizeof(times));
                    returnLength = sizeof(times);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    status = GetExceptionCode();
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
            if (!processContext || !processContext->ImageFileName)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
            }

            returnLength = sizeof(UNICODE_STRING);
            returnLength += processContext->ImageFileName->Length;

            if (ObjectInformationLength < returnLength)
            {
                status = STATUS_BUFFER_TOO_SMALL;
                goto Exit;
            }

            if (!ObjectInformation)
            {
                status = STATUS_BUFFER_TOO_SMALL;
                goto Exit;
            }

            __try
            {
                PUNICODE_STRING outputString;

                outputString = ObjectInformation;
                outputString->Length = processContext->ImageFileName->Length;
                outputString->MaximumLength = processContext->ImageFileName->Length;
                outputString->Buffer = Add2Ptr(ObjectInformation,
                                               sizeof(UNICODE_STRING));
                RtlCopyMemory(outputString->Buffer,
                              processContext->ImageFileName->Buffer,
                              processContext->ImageFileName->Length);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
            }

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

                __try
                {
                    RtlCopyMemory(ObjectInformation, nameInfo, returnLength);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    status = GetExceptionCode();
                }
            }

            break;
        }
        case KphObjectThreadIsTerminated:
        {
            ULONG threadIsTerminated;

            if (!ObjectInformation ||
                (ObjectInformationLength < sizeof(threadIsTerminated)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(threadIsTerminated);
                goto Exit;
            }

            KeStackAttachProcess(process, &apcState);
            status = ZwQueryInformationThread(Handle,
                                              ThreadIsTerminated,
                                              &threadIsTerminated,
                                              sizeof(threadIsTerminated),
                                              NULL);
            KeUnstackDetachProcess(&apcState);
            if (NT_SUCCESS(status))
            {
                __try
                {
                    *(PULONG)ObjectInformation = threadIsTerminated;
                    returnLength = sizeof(threadIsTerminated);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    status = GetExceptionCode();
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
                    __try
                    {
                        RtlCopyMemory(ObjectInformation, buffer, returnLength);
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        status = GetExceptionCode();
                    }
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

                    __try
                    {
                        RtlCopyMemory(ObjectInformation, buffer, length);
                        returnLength = (ULONG)length;
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        status = GetExceptionCode();
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

                __try
                {
                    RtlCopyMemory(ObjectInformation,
                                  sectionFileName,
                                  returnLength);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    status = GetExceptionCode();
                }
            }

            break;
        }
        case KphObjectAttributesInformation:
        {
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

            __try
            {
                PKPH_OBJECT_ATTRIBUTES_INFORMATION attributesInfo;

                attributesInfo = ObjectInformation;
                attributesInfo->Flags = OBJECT_TO_OBJECT_HEADER(object)->Flags;
                returnLength = sizeof(KPH_OBJECT_ATTRIBUTES_INFORMATION);
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
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
        if (AccessMode != KernelMode)
        {
            __try
            {
                *ReturnLength = returnLength;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                NOTHING;
            }
        }
        else
        {
            *ReturnLength = returnLength;
        }
    }

    return status;
}

/**
 * \brief Sets object information.
 *
 * \param[in] ProcessHandle A handle to a process.
 * \param[in] Handle A handle which is present in the process referenced by
 * \a ProcessHandle.
 * \param[in] ObjectInformationClass The type of information to set.
 * \param[in] ObjectInformation A buffer which contains the information to set.
 * \param[in] ObjectInformationLength The number of bytes present in \a
 * ObjectInformation.
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
            ProbeInputBytes(ObjectInformation, ObjectInformationLength);
            RtlCopyVolatileMemory(objectInformation,
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

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeOutputType(ObjectHandle, HANDLE);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }

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

    if (AccessMode != KernelMode)
    {
        __try
        {
            *ObjectHandle = objectHandle;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            ObCloseHandle(objectHandle, UserMode);
        }
    }
    else
    {
        *ObjectHandle = objectHandle;
    }

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

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeOutputType(TargetHandle, HANDLE);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
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
        if (AccessMode != KernelMode)
        {
            __try
            {
                *TargetHandle = targetHandle;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
                ObCloseHandle(targetHandle, UserMode);
            }
        }
        else
        {
            *TargetHandle = targetHandle;
        }
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
