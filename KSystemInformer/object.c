/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     jxy-s   2021-2022
 *
 */

#include <kph.h>
#include <dyndata.h>

#include <trace.h>

PAGED_FILE();

static UNICODE_STRING KphpEtwRegistrationName = RTL_CONSTANT_STRING(L"EtwRegistration");

typedef struct _KPHP_ENUMERATE_PROCESS_HANDLES_CONTEXT
{
    PVOID Buffer;
    PVOID BufferLimit;
    PVOID CurrentEntry;
    ULONG Count;
    NTSTATUS Status;
} KPHP_ENUMERATE_PROCESS_HANDLES_CONTEXT, *PKPHP_ENUMERATE_PROCESS_HANDLES_CONTEXT;

/**
 * \brief Gets a pointer to the handle table of a process. On success, acquires
 * process exit synchronization, the process should be released by calling
 * KphDereferenceProcessHandleTable.
 *
 * \param[in] Process A process object.
 * \param[out] HandleTable On success set to the process handle table.
 *
 * \return Successful or errant status.
 */
_Acquires_lock_(Process)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphReferenceProcessHandleTable(
    _In_ PEPROCESS Process,
    _Outptr_result_nullonfailure_ PHANDLE_TABLE* HandleTable
    )
{
    PHANDLE_TABLE handleTable;
    NTSTATUS status;

    PAGED_PASSIVE();

    *HandleTable = NULL;

    //
    // Fail if we don't have an offset.
    //
    if (KphDynEpObjectTable == ULONG_MAX)
    {
        return STATUS_NOINTERFACE;
    }

    //
    // Prevent the process from terminating and get its handle table.
    //
    status = PsAcquireProcessExitSynchronization(Process);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "PsAcquireProcessExitSynchronization failed: %!STATUS!",
                      status);

        return STATUS_TOO_LATE;
    }

    handleTable = *(PHANDLE_TABLE *)Add2Ptr(Process, KphDynEpObjectTable);

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
    PAGED_PASSIVE();

    PsReleaseProcessExitSynchronization(Process);
}

/**
 * \brief Unlocks a handle table entry.
 *
 * \param[in] HandleTable Handle table to unlock.
 * \param[in] HandleTableEntry Handle table entry to unlock.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpUnlockHandleTableEntry(
    _In_ PHANDLE_TABLE HandleTable,
    _In_ PHANDLE_TABLE_ENTRY HandleTableEntry
    )
{
    PEX_PUSH_LOCK handleContentionEvent;

    PAGED_PASSIVE();

    NT_ASSERT(KphDynHtHandleContentionEvent != ULONG_MAX);

    //
    // Set the unlocked bit.
    //
    InterlockedExchangeAddULongPtr(&HandleTableEntry->Value, 1);

    //
    // Allow waiters to wake up.
    //
    handleContentionEvent = (PEX_PUSH_LOCK)Add2Ptr(HandleTable, KphDynHtHandleContentionEvent);
    if (*(PULONG_PTR)handleContentionEvent != 0)
    {
        ExfUnblockPushLock(handleContentionEvent, NULL);
    }
}

typedef struct _KPH_ENUM_PROC_HANDLE_EX_CONTEXT
{
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

    PAGED_PASSIVE();

    NT_ASSERT(Context);

    context = Context;

    result = context->Callback(HandleTableEntry, Handle, context->Parameter);

    KphpUnlockHandleTableEntry(HandleTable, HandleTableEntry);

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
    KPH_ENUM_PROC_HANDLE_EX_CONTEXT context;
    PHANDLE_TABLE handleTable;

    PAGED_PASSIVE();

    if ((KphDynHtHandleContentionEvent == ULONG_MAX) ||
        (KphDynEpObjectTable == ULONG_MAX))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphEnumerateProcessHandlesEx not supported");

        return STATUS_NOINTERFACE;
    }

    status = KphReferenceProcessHandleTable(Process, &handleTable);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphReferenceProcessHandleTable failed: %!STATUS!",
                      status);

        return status;
    }

    context.Callback = Callback;
    context.Parameter = Parameter;

    ExEnumHandleTable(handleTable,
                      KphEnumerateProcessHandlesExCallback,
                      &context,
                      NULL);

    KphDereferenceProcessHandleTable(Process);

    return STATUS_SUCCESS;
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
_Function_class_(PKPH_ENUM_PROCESS_HANDLES_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOLEAN KphpEnumerateProcessHandlesCallbck(
    _Inout_ PHANDLE_TABLE_ENTRY HandleTableEntry,
    _In_ HANDLE Handle,
    _In_ PVOID Context
    )
{
    PKPHP_ENUMERATE_PROCESS_HANDLES_CONTEXT context;
    KPH_PROCESS_HANDLE handleInfo;
    POBJECT_HEADER objectHeader;
    POBJECT_TYPE objectType;
    PKPH_PROCESS_HANDLE entryInBuffer;

    PAGED_PASSIVE();

    context = Context;

    objectHeader = ObpDecodeObject(HandleTableEntry->Object);
    handleInfo.Handle = Handle;
    handleInfo.Object = objectHeader ? &objectHeader->Body : NULL;
    handleInfo.GrantedAccess = ObpDecodeGrantedAccess(HandleTableEntry->GrantedAccess);
    handleInfo.ObjectTypeIndex = USHORT_MAX;
    handleInfo.Reserved1 = 0;
    handleInfo.HandleAttributes = ObpGetHandleAttributes(HandleTableEntry);
    handleInfo.Reserved2 = 0;

    if (handleInfo.Object)
    {
        objectType = ObGetObjectType(handleInfo.Object);

        if (objectType && (KphDynOtIndex != ULONG_MAX))
        {
            handleInfo.ObjectTypeIndex = (USHORT)(*(PUCHAR)Add2Ptr(objectType, KphDynOtIndex));
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
    _In_opt_ ULONG BufferLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;
    KPHP_ENUMERATE_PROCESS_HANDLES_CONTEXT context;

    PAGED_PASSIVE();

    if (!Buffer)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(Buffer, BufferLength, 1);

            if (ReturnLength)
            {
                ProbeOutputType(ReturnLength, ULONG);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
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
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        return status;
    }

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
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphEnumerateProcessHandlesEx failed: %!STATUS!",
                      status);

        ObDereferenceObject(process);
        return status;
    }

    ObDereferenceObject(process);

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
                return GetExceptionCode();
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
            return STATUS_INTEGER_OVERFLOW;
        }

        if (AccessMode != KernelMode)
        {
            __try
            {
                *ReturnLength = (ULONG)returnLength;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return GetExceptionCode();
            }
        }
        else
        {
            *ReturnLength = (ULONG)returnLength;
        }
    }

    return context.Status;
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
    _Out_writes_bytes_(BufferLength) POBJECT_NAME_INFORMATION Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    POBJECT_TYPE objectType;

    PAGED_PASSIVE();

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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphQueryNameObject: %wZ",
                      &Buffer->Name);
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
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
    _Out_writes_bytes_(BufferLength) POBJECT_NAME_INFORMATION Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength
    )
{
    ULONG returnLength;
    PCHAR objectName;
    ULONG usedLength;
    ULONG subNameLength;
    PFILE_OBJECT relatedFileObject;

    PAGED_PASSIVE();

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

    //
    // Check if the file object has an associated device (e.g.
    // "\Device\NamedPipe", "\Device\Mup"). We can use the user-supplied buffer
    // for this since if the buffer isn't big enough, we can't proceed anyway
    // (we are going to use the name).
    //
    if (FileObject->DeviceObject)
    {
        NTSTATUS status;

        status = ObQueryNameString(FileObject->DeviceObject,
                                   Buffer,
                                   BufferLength,
                                   &returnLength);

        if (!NT_SUCCESS(status))
        {
            if (status == STATUS_INFO_LENGTH_MISMATCH)
            {
                status = STATUS_BUFFER_TOO_SMALL;
            }

            *ReturnLength = returnLength;

            return status;
        }

        //
        // The UNICODE_STRING in the buffer is now filled in. We will append
        // to the object name later, so we need to fix the object name pointer
        // by adding the length, in bytes, of the device name string we just got.
        //
        objectName += Buffer->Name.Length;
        usedLength += Buffer->Name.Length;
    }

    //
    // Check if the file object has a file name component. If not, we can't do
    // anything else, so we just return the name we have already.
    //
    if (!FileObject->FileName.Buffer)
    {
        *ReturnLength = usedLength;

        return STATUS_SUCCESS;
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
        subNameLength += relatedFileObject->FileName.Length;

        //
        // Avoid infinite loops.
        //
        if (relatedFileObject == relatedFileObject->RelatedFileObject)
        {
            break;
        }

        relatedFileObject = relatedFileObject->RelatedFileObject;

    } while (relatedFileObject);

    usedLength += subNameLength;

    //
    // Check if we have enough space to write the whole thing.
    //
    if (usedLength > BufferLength)
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
        objectName -= relatedFileObject->FileName.Length;
        RtlCopyMemory(objectName,
                      relatedFileObject->FileName.Buffer,
                      relatedFileObject->FileName.Length);

        //
        // Avoid infinite loops.
        //
        if (relatedFileObject == relatedFileObject->RelatedFileObject)
        {
            break;
        }

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
    _Out_writes_bytes_(BufferLength) POBJECT_NAME_INFORMATION Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    PFLT_FILE_NAME_INFORMATION fileNameInfo;

    PAGED_PASSIVE();

    fileNameInfo = NULL;

    if (IoGetTopLevelIrp() || KeAreAllApcsDisabled())
    {
        status = STATUS_POSSIBLE_DEADLOCK;
    }
    else
    {
        status = FltGetFileNameInformationUnsafe(FileObject,
                                                 NULL,
                                                 (FLT_FILE_NAME_NORMALIZED |
                                                  FLT_FILE_NAME_QUERY_DEFAULT),
                                                 &fileNameInfo);
        if (!NT_SUCCESS(status))
        {
            fileNameInfo = NULL;
        }
        else
        {
            *ReturnLength = sizeof(OBJECT_NAME_INFORMATION);
            *ReturnLength += fileNameInfo->Name.Length;
            if (BufferLength < *ReturnLength)
            {
                status = STATUS_BUFFER_TOO_SMALL;
                goto Exit;
            }

            Buffer->Name.Length = 0;
            Buffer->Name.MaximumLength = (USHORT)(BufferLength - sizeof(OBJECT_NAME_INFORMATION));
            Buffer->Name.Buffer = (PWSTR)Add2Ptr(Buffer, sizeof(OBJECT_NAME_INFORMATION));

            RtlCopyUnicodeString(&Buffer->Name, &fileNameInfo->Name);

            status = STATUS_SUCCESS;
            goto Exit;
        }
    }

    if (!NT_SUCCESS(status) &&
        !BooleanFlagOn(FileObject->Flags, FO_CLEANUP_COMPLETE))
    {
        status = KphpExtractNameFileObject(FileObject,
                                           Buffer,
                                           BufferLength,
                                           ReturnLength);
    }

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
    PEPROCESS process;
    KAPC_STATE apcState;
    PVOID object;
    ULONG returnLength;
    PVOID buffer;
    BYTE stackBuffer[64];
    KPROCESSOR_MODE accessMode;
    PKPH_PROCESS_CONTEXT processContext;

    PAGED_PASSIVE();

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
                ProbeForWrite(ObjectInformation, ObjectInformationLength, 1);
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
        KphTracePrint(TRACE_LEVEL_ERROR,
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
                (ObjectInformationLength != sizeof(basicInfo)))
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
                KphTracePrint(TRACE_LEVEL_ERROR,
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

            if (allocateSize <= ARRAYSIZE(stackBuffer))
            {
                buffer = stackBuffer;
            }
            else
            {
                buffer = KphAllocatePaged(allocateSize, KPH_TAG_OBJECT_QUERY);
                if (!buffer)
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Exit;
                }
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

                if (nameInfo->Name.Buffer)
                {
                    RebaseUnicodeString(&nameInfo->Name,
                                        nameInfo,
                                        ObjectInformation);
                }

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
            allocateSize += sizeof(ULONGLONG);

            if (allocateSize <= ARRAYSIZE(stackBuffer))
            {
                buffer = stackBuffer;
            }
            else
            {
                buffer = KphAllocatePaged(allocateSize, KPH_TAG_OBJECT_QUERY);
                if (!buffer)
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Exit;
                }
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

                if (typeInfo->TypeName.Buffer)
                {
                    RebaseUnicodeString(&typeInfo->TypeName,
                                        typeInfo,
                                        ObjectInformation);
                }

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
                (ObjectInformationLength != sizeof(handleFlagInfo)))
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
                (ObjectInformationLength != sizeof(basicInfo)))
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
                (ObjectInformationLength != sizeof(basicInfo)))
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
            ETWREG_BASIC_INFORMATION basicInfo;

            if ((KphDynEgeGuid == ULONG_MAX) ||
                (KphDynEreGuidEntry == ULONG_MAX) ||
                (KphDynOtName == ULONG_MAX))
            {
                status = STATUS_NOINTERFACE;
                goto Exit;
            }

            if (!ObjectInformation ||
                (ObjectInformationLength != sizeof(basicInfo)))
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
                KphTracePrint(TRACE_LEVEL_ERROR,
                              GENERAL,
                              "ObReferenceObjectByHandle failed: %!STATUS!",
                              status);

                object = NULL;
                goto Exit;
            }

            objectType = ObGetObjectType(object);
            if (!objectType)
            {
                KphTracePrint(TRACE_LEVEL_ERROR,
                              GENERAL,
                              "ObGetObjectType returned null");

                status = STATUS_NOT_SUPPORTED;
                goto Exit;
            }

            objectTypeName = (PUNICODE_STRING)Add2Ptr(objectType, KphDynOtName);
            if (!RtlEqualUnicodeString(objectTypeName,
                                       &KphpEtwRegistrationName,
                                       FALSE))
            {
                status = STATUS_OBJECT_TYPE_MISMATCH;
                goto Exit;
            }

            guidEntry = *(PVOID*)Add2Ptr(object, KphDynEreGuidEntry);
            if (guidEntry)
            {
                RtlCopyMemory(&basicInfo.Guid,
                              Add2Ptr(guidEntry, KphDynEgeGuid),
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
                (ObjectInformationLength != sizeof(fileInfo)))
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
                KphTracePrint(TRACE_LEVEL_ERROR,
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
                (ObjectInformationLength != sizeof(KPH_FILE_OBJECT_DRIVER)))
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
                KphTracePrint(TRACE_LEVEL_ERROR,
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
                (ObjectInformationLength != sizeof(times)))
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
                (ObjectInformationLength != sizeof(times)))
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
                KphTracePrint(TRACE_LEVEL_ERROR,
                              GENERAL,
                              "ObReferenceObjectByHandle failed: %!STATUS!",
                              status);

                goto Exit;
            }

            processContext = KphGetProcessContext(PsGetProcessId(targetProcess));
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
            allocateSize += sizeof(ULONGLONG);

            if (allocateSize <= ARRAYSIZE(stackBuffer))
            {
                buffer = stackBuffer;
            }
            else
            {
                buffer = KphAllocatePaged(allocateSize, KPH_TAG_OBJECT_QUERY);
                if (!buffer)
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Exit;
                }
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

                if (nameInfo->ThreadName.Buffer)
                {
                    RebaseUnicodeString(&nameInfo->ThreadName,
                                        nameInfo,
                                        ObjectInformation);
                }

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
                (ObjectInformationLength != sizeof(threadIsTerminated)))
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
        {
            SECTION_BASIC_INFORMATION sectionInfo;

            if (!ObjectInformation ||
                (ObjectInformationLength != sizeof(sectionInfo)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(sectionInfo);
                goto Exit;
            }

            KeStackAttachProcess(process, &apcState);
            status = ZwQuerySection(Handle,
                                    SectionBasicInformation,
                                    &sectionInfo,
                                    sizeof(sectionInfo),
                                    NULL);
            KeUnstackDetachProcess(&apcState);
            if (NT_SUCCESS(status))
            {
                __try
                {
                    RtlCopyMemory(ObjectInformation,
                                  &sectionInfo,
                                  sizeof(sectionInfo));
                    returnLength = sizeof(sectionInfo);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    status = GetExceptionCode();
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
            allocateSize += sizeof(ULONGLONG);

            if (allocateSize <= ARRAYSIZE(stackBuffer))
            {
                buffer = stackBuffer;
            }
            else
            {
                buffer = KphAllocatePaged(allocateSize, KPH_TAG_OBJECT_QUERY);
                if (!buffer)
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto Exit;
                }
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
                KphTracePrint(TRACE_LEVEL_ERROR,
                              GENERAL,
                              "ObDuplicateObject failed: %!STATUS!",
                              status);

                goto Exit;
            }

            KeStackAttachProcess(PsInitialSystemProcess, &apcState);

            baseAddress = NULL;
            viewSize = 0;
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

            ObCloseHandle(sectionHandle, KernelMode);

            KeUnstackDetachProcess(&apcState);

            if (NT_SUCCESS(status))
            {
                if (!ObjectInformation)
                {
                    status = STATUS_BUFFER_TOO_SMALL;
                    goto Exit;
                }

                if (sectionFileName->Buffer)
                {
                    RebaseUnicodeString(sectionFileName,
                                        sectionFileName,
                                        ObjectInformation);
                }

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

    if (buffer && (buffer != stackBuffer))
    {
        KphFree(buffer, KPH_TAG_OBJECT_QUERY);
    }

    if (object)
    {
        ObDereferenceObject(object);
    }

    if (process)
    {
        ObDereferenceObject(process);
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
    PEPROCESS process;
    KAPC_STATE apcState;

    PAGED_PASSIVE();

    process = NULL;

    if (!ObjectInformation)
    {
        status = STATUS_INVALID_PARAMETER_4;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForRead(ObjectInformation, ObjectInformationLength, 1);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }

    status = ObReferenceObjectByHandle(ProcessHandle,
                                       PROCESS_SET_INFORMATION,
                                       *PsProcessType,
                                       AccessMode,
                                       &process,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
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
            OBJECT_HANDLE_FLAG_INFORMATION handleFlagInfo;

            if (ObjectInformationLength != sizeof(handleFlagInfo))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                goto Exit;
            }

            __try
            {
                RtlCopyMemory(&handleFlagInfo,
                              ObjectInformation,
                              sizeof(handleFlagInfo));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
                goto Exit;
            }

            KeStackAttachProcess(process, &apcState);
            status = ObSetHandleAttributes(Handle, &handleFlagInfo, KernelMode);
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

    PAGED_PASSIVE();

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
        KphTracePrint(TRACE_LEVEL_ERROR,
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

    PAGED_PASSIVE();

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
        KphTracePrint(TRACE_LEVEL_ERROR,
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
        KphTracePrint(TRACE_LEVEL_ERROR,
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
