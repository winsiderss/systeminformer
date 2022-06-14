/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *
 */

#include <kph.h>
#include <dyndata.h>

#ifdef _X86_
#define KERNEL_HANDLE_BIT (0x80000000)
#else
#define KERNEL_HANDLE_BIT (0xffffffff80000000)
#endif

#define IsKernelHandle(Handle) ((LONG_PTR)(Handle) < 0)
#define MakeKernelHandle(Handle) ((HANDLE)((ULONG_PTR)(Handle) | KERNEL_HANDLE_BIT))

typedef struct _KPHP_ENUMERATE_PROCESS_HANDLES_CONTEXT
{
    PVOID Buffer;
    PVOID BufferLimit;
    PVOID CurrentEntry;
    ULONG Count;
    NTSTATUS Status;
} KPHP_ENUMERATE_PROCESS_HANDLES_CONTEXT, *PKPHP_ENUMERATE_PROCESS_HANDLES_CONTEXT;

BOOLEAN KphpEnumerateProcessHandlesEnumCallback61(
    _Inout_ PHANDLE_TABLE_ENTRY HandleTableEntry,
    _In_ HANDLE Handle,
    _In_ PVOID Context
    );

BOOLEAN KphpEnumerateProcessHandlesEnumCallback(
    _In_ PHANDLE_TABLE HandleTable,
    _Inout_ PHANDLE_TABLE_ENTRY HandleTableEntry,
    _In_ HANDLE Handle,
    _In_ PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KphReferenceProcessHandleTable)
#pragma alloc_text(PAGE, KphDereferenceProcessHandleTable)
#pragma alloc_text(PAGE, KphUnlockHandleTableEntry)
#pragma alloc_text(PAGE, KphpEnumerateProcessHandlesEnumCallback61)
#pragma alloc_text(PAGE, KphpEnumerateProcessHandlesEnumCallback)
#pragma alloc_text(PAGE, KpiEnumerateProcessHandles)
#pragma alloc_text(PAGE, KphQueryNameObject)
#pragma alloc_text(PAGE, KphQueryNameFileObject)
#pragma alloc_text(PAGE, KpiQueryInformationObject)
#pragma alloc_text(PAGE, KpiSetInformationObject)
#pragma alloc_text(PAGE, KphOpenNamedObject)
#endif

/**
 * Gets a pointer to the handle table of a process.
 *
 * \param Process A process object.
 *
 * \return A pointer to the handle table, or NULL if the process is terminating or the request is
 * not supported. You must call KphDereferenceProcessHandleTable() when the handle table is no
 * longer needed.
 */
PHANDLE_TABLE KphReferenceProcessHandleTable(
    _In_ PEPROCESS Process
    )
{
    PHANDLE_TABLE handleTable = NULL;

    PAGED_CODE();

    // Fail if we don't have an offset.
    if (KphDynEpObjectTable == ULONG_MAX)
        return NULL;

    // Prevent the process from terminating and get its handle table.
    if (NT_SUCCESS(PsAcquireProcessExitSynchronization(Process)))
    {
        handleTable = *(PHANDLE_TABLE *)PTR_ADD_OFFSET(Process, KphDynEpObjectTable);

        if (!handleTable)
            PsReleaseProcessExitSynchronization(Process);
    }

    return handleTable;
}

/**
 * Dereferences the handle table of a process.
 *
 * \param Process A process object.
 */
VOID KphDereferenceProcessHandleTable(
    _In_ PEPROCESS Process
    )
{
    PAGED_CODE();

    PsReleaseProcessExitSynchronization(Process);
}

VOID KphUnlockHandleTableEntry(
    _In_ PHANDLE_TABLE HandleTable,
    _In_ PHANDLE_TABLE_ENTRY HandleTableEntry
    )
{
    PEX_PUSH_LOCK handleContentionEvent;

    PAGED_CODE();

    // Set the unlocked bit.

#if (defined _M_X64) || (defined _M_ARM64)
    InterlockedExchangeAdd64(&HandleTableEntry->Value, 1);
#else
    InterlockedExchangeAdd(&HandleTableEntry->Value, 1);
#endif

    // Allow waiters to wake up.

    handleContentionEvent = (PEX_PUSH_LOCK)PTR_ADD_OFFSET(HandleTable, KphDynHtHandleContentionEvent);

    if (*(PULONG_PTR)handleContentionEvent != 0)
        ExfUnblockPushLock(handleContentionEvent, NULL);
}

BOOLEAN KphpEnumerateProcessHandlesEnumCallback61(
    _Inout_ PHANDLE_TABLE_ENTRY HandleTableEntry,
    _In_ HANDLE Handle,
    _In_ PVOID Context
    )
{
    PKPHP_ENUMERATE_PROCESS_HANDLES_CONTEXT context = Context;
    KPH_PROCESS_HANDLE handleInfo;
    POBJECT_HEADER objectHeader;
    POBJECT_TYPE objectType;
    PKPH_PROCESS_HANDLE entryInBuffer;

    PAGED_CODE();

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

        if (objectType && KphDynOtIndex != ULONG_MAX)
            handleInfo.ObjectTypeIndex = (USHORT)*(PUCHAR)PTR_ADD_OFFSET(objectType, KphDynOtIndex);
    }

    // Advance the current entry pointer regardless of whether the information will be written; this
    // will allow the parent function to report the correct return length.
    entryInBuffer = context->CurrentEntry;
    context->CurrentEntry = PTR_ADD_OFFSET(context->CurrentEntry, sizeof(KPH_PROCESS_HANDLE));
    context->Count++;

    // Only write if we have not exceeded the buffer length. Also check for a potential overflow (if
    // the process has an extremely large number of handles, the buffer pointer may wrap).
    if (
        (ULONG_PTR)entryInBuffer >= (ULONG_PTR)context->Buffer &&
        (ULONG_PTR)entryInBuffer + sizeof(KPH_PROCESS_HANDLE) <= (ULONG_PTR)context->BufferLimit
        )
    {
        __try
        {
            *entryInBuffer = handleInfo;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            // Report an error.
            if (context->Status == STATUS_SUCCESS)
                context->Status = GetExceptionCode();
        }
    }
    else
    {
        // Report that the buffer is too small.
        if (context->Status == STATUS_SUCCESS)
            context->Status = STATUS_BUFFER_TOO_SMALL;
    }

    return FALSE;
}

BOOLEAN KphpEnumerateProcessHandlesEnumCallback(
    _In_ PHANDLE_TABLE HandleTable,
    _Inout_ PHANDLE_TABLE_ENTRY HandleTableEntry,
    _In_ HANDLE Handle,
    _In_ PVOID Context
    )
{
    BOOLEAN result;

    PAGED_CODE();

    result = KphpEnumerateProcessHandlesEnumCallback61(HandleTableEntry, Handle, Context);
    KphUnlockHandleTableEntry(HandleTable, HandleTableEntry);

    return result;
}

/**
 * Enumerates the handles of a process.
 *
 * \param ProcessHandle A handle to a process.
 * \param Buffer The buffer in which the handle information will be stored.
 * \param BufferLength The number of bytes available in \a Buffer.
 * \param ReturnLength A variable which receives the number of bytes required to be available in
 * \a Buffer.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiEnumerateProcessHandles(
    _In_ HANDLE ProcessHandle,
    _Out_writes_bytes_(BufferLength) PVOID Buffer,
    _In_opt_ ULONG BufferLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    BOOLEAN result;
    PEPROCESS process;
    PHANDLE_TABLE handleTable;
    KPHP_ENUMERATE_PROCESS_HANDLES_CONTEXT context;

    PAGED_CODE();

    if (KphDynNtVersion >= PHNT_WIN8 && KphDynHtHandleContentionEvent == ULONG_MAX)
    {
        return STATUS_NOT_SUPPORTED;
    }

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(Buffer, BufferLength, sizeof(ULONG));

            if (ReturnLength)
                ProbeForWrite(ReturnLength, sizeof(ULONG), sizeof(ULONG));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    // Reference the process object.
    status = ObReferenceObjectByHandle(
        ProcessHandle,
        0,
        *PsProcessType,
        AccessMode,
        &process,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    // Get its handle table.
    handleTable = KphReferenceProcessHandleTable(process);

    if (!handleTable)
    {
        ObDereferenceObject(process);
        return STATUS_UNSUCCESSFUL;
    }

    // Initialize the enumeration context.
    context.Buffer = Buffer;
    context.BufferLimit = PTR_ADD_OFFSET(Buffer, BufferLength);
    context.CurrentEntry = ((PKPH_PROCESS_HANDLE_INFORMATION)Buffer)->Handles;
    context.Count = 0;
    context.Status = STATUS_SUCCESS;

    // Enumerate the handles.

    if (KphDynNtVersion >= PHNT_WIN8)
    {
        result = ExEnumHandleTable(
            handleTable,
            KphpEnumerateProcessHandlesEnumCallback,
            &context,
            NULL
            );
    }
    else
    {
        result = ExEnumHandleTable(
            handleTable,
            (PEX_ENUM_HANDLE_CALLBACK)KphpEnumerateProcessHandlesEnumCallback61,
            &context,
            NULL
            );
    }

    KphDereferenceProcessHandleTable(process);
    ObDereferenceObject(process);

    // Write the number of handles if we can.
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

    // Supply the return length if the caller wanted it.
    if (ReturnLength)
    {
        ULONG returnLength;

        // Note: if the CurrentEntry pointer wrapped, this will give the wrong return length.
        returnLength = (ULONG)((ULONG_PTR)context.CurrentEntry - (ULONG_PTR)Buffer);

        if (AccessMode != KernelMode)
        {
            __try
            {
                *ReturnLength = returnLength;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return GetExceptionCode();
            }
        }
        else
        {
            *ReturnLength = returnLength;
        }
    }

    return context.Status;
}

/**
 * Queries the name of an object.
 *
 * \param Object A pointer to an object.
 * \param Buffer The buffer in which the object name will be stored.
 * \param BufferLength The number of bytes available in \a Buffer.
 * \param ReturnLength A variable which receives the number of bytes required to be available in
 * \a Buffer.
 */
NTSTATUS KphQueryNameObject(
    _In_ PVOID Object,
    _Out_writes_bytes_(BufferLength) POBJECT_NAME_INFORMATION Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength
    )
{
    NTSTATUS status;
    POBJECT_TYPE objectType;

    PAGED_CODE();

    objectType = ObGetObjectType(Object);

    // Check if we are going to hang when querying the object, and use
    // the special file object query function if needed.
    if (objectType == *IoFileObjectType &&
        (((PFILE_OBJECT)Object)->Busy || ((PFILE_OBJECT)Object)->Waiters))
    {
        status = KphQueryNameFileObject(Object, Buffer, BufferLength, ReturnLength);
        dprintf("KphQueryNameFileObject: status 0x%x\n", status);
    }
    else
    {
        status = ObQueryNameString(Object, Buffer, BufferLength, ReturnLength);
        dprintf("ObQueryNameString: status 0x%x\n", status);
    }

    // Make the error returns consistent.
    if (status == STATUS_BUFFER_OVERFLOW) // returned by I/O subsystem
        status = STATUS_BUFFER_TOO_SMALL;
    if (status == STATUS_INFO_LENGTH_MISMATCH) // returned by ObQueryNameString
        status = STATUS_BUFFER_TOO_SMALL;

    if (NT_SUCCESS(status))
        dprintf("KphQueryNameObject: %.*S\n", Buffer->Name.Length / (USHORT)sizeof(WCHAR), Buffer->Name.Buffer);
    else
        dprintf("KphQueryNameObject: status 0x%x\n", status);

    return status;
}

/**
 * Queries the name of a file object.
 *
 * \param FileObject A pointer to a file object.
 * \param Buffer The buffer in which the object name will be stored.
 * \param BufferLength The number of bytes available in \a Buffer.
 * \param ReturnLength A variable which receives the number of bytes required to be available in
 * \a Buffer.
 */
NTSTATUS KphQueryNameFileObject(
    _In_ PFILE_OBJECT FileObject,
    _Out_writes_bytes_(BufferLength) POBJECT_NAME_INFORMATION Buffer,
    _In_ ULONG BufferLength,
    _Out_ PULONG ReturnLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG returnLength;
    PCHAR objectName;
    ULONG usedLength;
    ULONG subNameLength;
    PFILE_OBJECT relatedFileObject;

    PAGED_CODE();

    // We need at least the size of OBJECT_NAME_INFORMATION to continue.
    if (BufferLength < sizeof(OBJECT_NAME_INFORMATION))
    {
        *ReturnLength = sizeof(OBJECT_NAME_INFORMATION);

        return STATUS_BUFFER_TOO_SMALL;
    }

    // Assume failure.
    Buffer->Name.Length = 0;
    // We will place the object name directly after the UNICODE_STRING structure in the buffer.
    Buffer->Name.Buffer = (PWSTR)PTR_ADD_OFFSET(Buffer, sizeof(OBJECT_NAME_INFORMATION));
    // Retain a local pointer to the object name so we can manipulate the pointer.
    objectName = (PCHAR)Buffer->Name.Buffer;
    // A variable that keeps track of how much space we have used.
    usedLength = sizeof(OBJECT_NAME_INFORMATION);

    // Check if the file object has an associated device (e.g. "\Device\NamedPipe", "\Device\Mup").
    // We can use the user-supplied buffer for this since if the buffer isn't big enough, we can't
    // proceed anyway (we are going to use the name).
    if (FileObject->DeviceObject)
    {
        status = ObQueryNameString(
            FileObject->DeviceObject,
            Buffer,
            BufferLength,
            &returnLength
            );

        if (!NT_SUCCESS(status))
        {
            if (status == STATUS_INFO_LENGTH_MISMATCH)
                status = STATUS_BUFFER_TOO_SMALL;

            *ReturnLength = returnLength;

            return status;
        }

        // The UNICODE_STRING in the buffer is now filled in. We will append to the object name
        // later, so we need to fix the object name pointer by adding the length, in bytes, of the
        // device name string we just got.
        objectName += Buffer->Name.Length;
        usedLength += Buffer->Name.Length;
    }

    // Check if the file object has a file name component. If not, we can't do anything else, so we
    // just return the name we have already.
    if (!FileObject->FileName.Buffer)
    {
        *ReturnLength = usedLength;

        return STATUS_SUCCESS;
    }

    // The file object has a name. We need to walk up the file object chain and append the names of
    // the related file objects in reverse order. This means we need to calculate the total length
    // first.

    relatedFileObject = FileObject;
    subNameLength = 0;

    do
    {
        subNameLength += relatedFileObject->FileName.Length;

        // Avoid infinite loops.
        if (relatedFileObject == relatedFileObject->RelatedFileObject)
            break;

        relatedFileObject = relatedFileObject->RelatedFileObject;
    } while (relatedFileObject);

    usedLength += subNameLength;

    // Check if we have enough space to write the whole thing.
    if (usedLength > BufferLength)
    {
        *ReturnLength = usedLength;

        return STATUS_BUFFER_TOO_SMALL;
    }

    // We're ready to begin copying the names.

    // Add the name length because we're copying in reverse order.
    objectName += subNameLength;

    relatedFileObject = FileObject;

    do
    {
        objectName -= relatedFileObject->FileName.Length;
        memcpy(objectName, relatedFileObject->FileName.Buffer, relatedFileObject->FileName.Length);

        // Avoid infinite loops.
        if (relatedFileObject == relatedFileObject->RelatedFileObject)
            break;

        relatedFileObject = relatedFileObject->RelatedFileObject;
    } while (relatedFileObject);

    // Update the length.
    Buffer->Name.Length += (USHORT)subNameLength;

    // Pass the return length back.
    *ReturnLength = usedLength;

    return STATUS_SUCCESS;
}

/**
 * Queries object information.
 *
 * \param ProcessHandle A handle to a process.
 * \param Handle A handle which is present in the process referenced by \a ProcessHandle.
 * \param ObjectInformationClass The type of information to retrieve.
 * \param ObjectInformation The buffer in which the information will be stored.
 * \param ObjectInformationLength The number of bytes available in \a ObjectInformation.
 * \param ReturnLength A variable which receives the number of bytes required to be available in
 * \a ObjectInformation.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiQueryInformationObject(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE Handle,
    _In_ KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass,
    _Out_writes_bytes_(ObjectInformationLength) PVOID ObjectInformation,
    _In_ ULONG ObjectInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;
    KPROCESSOR_MODE referenceMode;
    KAPC_STATE apcState;
    ULONG returnLength;

    PAGED_CODE();

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(ObjectInformation, ObjectInformationLength, sizeof(ULONG));

            if (ReturnLength)
                ProbeForWrite(ReturnLength, sizeof(ULONG), sizeof(ULONG));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    status = ObReferenceObjectByHandle(
        ProcessHandle,
        0,
        *PsProcessType,
        AccessMode,
        &process,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (process == PsInitialSystemProcess)
    {
        // A check was added in Windows 7 - if we're attached to the System process, the handle must
        // be a kernel handle.
        Handle = MakeKernelHandle(Handle);
        referenceMode = KernelMode;
    }
    else
    {
        // Make sure the handle isn't a kernel handle if we're not attached to the System process.
        // This means we can avoid referencing then opening the objects later when calling
        // ZwQueryObject, etc.
        if (IsKernelHandle(Handle))
        {
            ObDereferenceObject(process);
            return STATUS_INVALID_HANDLE;
        }

        referenceMode = AccessMode;
    }

    switch (ObjectInformationClass)
    {
    case KphObjectBasicInformation:
        {
            OBJECT_BASIC_INFORMATION basicInfo;

            KeStackAttachProcess(process, &apcState);
            status = ZwQueryObject(
                Handle,
                ObjectBasicInformation,
                &basicInfo,
                sizeof(OBJECT_BASIC_INFORMATION),
                NULL
                );
            KeUnstackDetachProcess(&apcState);

            if (NT_SUCCESS(status))
            {
                if (ObjectInformationLength == sizeof(OBJECT_BASIC_INFORMATION))
                {
                    __try
                    {
                        *(POBJECT_BASIC_INFORMATION)ObjectInformation = basicInfo;
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        status = GetExceptionCode();
                    }
                }
                else
                {
                    status = STATUS_INFO_LENGTH_MISMATCH;
                }
            }

            returnLength = sizeof(OBJECT_BASIC_INFORMATION);
        }
        break;
    case KphObjectNameInformation:
        {
            PVOID object;
            ULONG allocateSize;
            POBJECT_NAME_INFORMATION nameInfo;

            returnLength = sizeof(OBJECT_NAME_INFORMATION);

            // Attach to the process a get a pointer to the object.
            KeStackAttachProcess(process, &apcState);
            status = ObReferenceObjectByHandle(
                Handle,
                0,
                NULL,
                referenceMode,
                &object,
                NULL
                );
            KeUnstackDetachProcess(&apcState);

            if (NT_SUCCESS(status))
            {
                allocateSize = ObjectInformationLength;

                if (allocateSize < sizeof(OBJECT_NAME_INFORMATION)) // make sure we never try to allocate 0 bytes
                    allocateSize = sizeof(OBJECT_NAME_INFORMATION);

                nameInfo = ExAllocatePoolQuotaZero(PagedPool, allocateSize, 'QhpK');

                if (nameInfo)
                {
                    status = KphQueryNameObject(
                        object,
                        nameInfo,
                        ObjectInformationLength,
                        &returnLength
                        );

                    dprintf("KpiQueryInformationObject: called KphQueryNameObject: Handle: 0x%Ix, ObjectInformationLength: %u, returnLength: %u\n",
                        (ULONG_PTR)Handle, ObjectInformationLength, returnLength);

                    if (NT_SUCCESS(status))
                    {
                        // Fix up the buffer pointer.
                        if (nameInfo->Name.Buffer)
                            nameInfo->Name.Buffer = (PVOID)((ULONG_PTR)nameInfo->Name.Buffer - (ULONG_PTR)nameInfo + (ULONG_PTR)ObjectInformation);

                        __try
                        {
                            memcpy(ObjectInformation, nameInfo, ObjectInformationLength);
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            status = GetExceptionCode();
                        }
                    }

                    ExFreePoolWithTag(nameInfo, 'QhpK');
                }
                else
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }

                ObDereferenceObject(object);
            }
        }
        break;
    case KphObjectTypeInformation:
        {
            ULONG allocateSize;
            POBJECT_TYPE_INFORMATION typeInfo;

            returnLength = sizeof(OBJECT_TYPE_INFORMATION);
            allocateSize = ObjectInformationLength;

            if (allocateSize < sizeof(OBJECT_TYPE_INFORMATION))
                allocateSize = sizeof(OBJECT_TYPE_INFORMATION);

            // ObQueryTypeInfo uses ObjectType->Name.MaximumLength instead of
            // ObjectType->Name.Length + sizeof(WCHAR) to calculate the required buffer size. In
            // Windows 8, certain object types (e.g. TmTx) do NOT include the null terminator in
            // MaximumLength, which causes ObQueryTypeInfo to overrun the given buffer. To work
            // around this bug, we add some (generous) padding to our allocation.
            allocateSize += sizeof(ULONGLONG);

            typeInfo = ExAllocatePoolQuotaZero(PagedPool, allocateSize, 'QhpK');

            if (typeInfo)
            {
                KeStackAttachProcess(process, &apcState);
                status = ZwQueryObject(
                    Handle,
                    ObjectTypeInformation,
                    typeInfo,
                    ObjectInformationLength,
                    &returnLength
                    );
                KeUnstackDetachProcess(&apcState);

                if (NT_SUCCESS(status))
                {
                    // Fix up the buffer pointer.
                    if (typeInfo->TypeName.Buffer)
                        typeInfo->TypeName.Buffer = (PVOID)((ULONG_PTR)typeInfo->TypeName.Buffer - (ULONG_PTR)typeInfo + (ULONG_PTR)ObjectInformation);

                    __try
                    {
                        memcpy(ObjectInformation, typeInfo, ObjectInformationLength);
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        status = GetExceptionCode();
                    }
                }

                ExFreePoolWithTag(typeInfo, 'QhpK');
            }
            else
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        break;
    case KphObjectHandleFlagInformation:
        {
            OBJECT_HANDLE_FLAG_INFORMATION handleFlagInfo;

            KeStackAttachProcess(process, &apcState);
            status = ZwQueryObject(
                Handle,
                ObjectHandleFlagInformation,
                &handleFlagInfo,
                sizeof(OBJECT_HANDLE_FLAG_INFORMATION),
                NULL
                );
            KeUnstackDetachProcess(&apcState);

            if (NT_SUCCESS(status))
            {
                if (ObjectInformationLength == sizeof(OBJECT_HANDLE_FLAG_INFORMATION))
                {
                    __try
                    {
                        *(POBJECT_HANDLE_FLAG_INFORMATION)ObjectInformation = handleFlagInfo;
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        status = GetExceptionCode();
                    }
                }
                else
                {
                    status = STATUS_INFO_LENGTH_MISMATCH;
                }
            }

            returnLength = sizeof(OBJECT_HANDLE_FLAG_INFORMATION);
        }
        break;
    case KphObjectProcessBasicInformation:
        {
            PROCESS_BASIC_INFORMATION basicInfo;

            KeStackAttachProcess(process, &apcState);
            status = ZwQueryInformationProcess(
                Handle,
                ProcessBasicInformation,
                &basicInfo,
                sizeof(PROCESS_BASIC_INFORMATION),
                NULL
                );
            KeUnstackDetachProcess(&apcState);

            if (NT_SUCCESS(status))
            {
                if (ObjectInformationLength == sizeof(PROCESS_BASIC_INFORMATION))
                {
                    __try
                    {
                        *(PPROCESS_BASIC_INFORMATION)ObjectInformation = basicInfo;
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        status = GetExceptionCode();
                    }
                }
                else
                {
                    status = STATUS_INFO_LENGTH_MISMATCH;
                }
            }

            returnLength = sizeof(PROCESS_BASIC_INFORMATION);
        }
        break;
    case KphObjectThreadBasicInformation:
        {
            THREAD_BASIC_INFORMATION basicInfo;

            KeStackAttachProcess(process, &apcState);
            status = ZwQueryInformationThread(
                Handle,
                ThreadBasicInformation,
                &basicInfo,
                sizeof(THREAD_BASIC_INFORMATION),
                NULL
                );
            KeUnstackDetachProcess(&apcState);

            if (NT_SUCCESS(status))
            {
                if (ObjectInformationLength == sizeof(THREAD_BASIC_INFORMATION))
                {
                    __try
                    {
                        *(PTHREAD_BASIC_INFORMATION)ObjectInformation = basicInfo;
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        status = GetExceptionCode();
                    }
                }
                else
                {
                    status = STATUS_INFO_LENGTH_MISMATCH;
                }
            }

            returnLength = sizeof(THREAD_BASIC_INFORMATION);
        }
        break;
    case KphObjectEtwRegBasicInformation:
        {
            PVOID etwReg;
            PVOID objectType;
            PUNICODE_STRING objectTypeName;
            UNICODE_STRING etwRegistrationName;
            PVOID guidEntry;
            ETWREG_BASIC_INFORMATION basicInfo;

            // Check dynamic data requirements.
            if (KphDynEgeGuid != ULONG_MAX &&
                KphDynEreGuidEntry != ULONG_MAX &&
                KphDynOtName != ULONG_MAX)
            {
                // Attach to the process and get a pointer to the object. We don't have a pointer to
                // the EtwRegistration object type, so we'll just have to check the type name.

                KeStackAttachProcess(process, &apcState);
                status = ObReferenceObjectByHandle(
                    Handle,
                    0,
                    NULL,
                    referenceMode,
                    &etwReg,
                    NULL
                    );
                KeUnstackDetachProcess(&apcState);

                if (NT_SUCCESS(status))
                {
                    // Check the type name.

                    objectType = ObGetObjectType(etwReg);

                    if (objectType)
                    {
                        objectTypeName = (PUNICODE_STRING)PTR_ADD_OFFSET(objectType, KphDynOtName);
                        RtlInitUnicodeString(&etwRegistrationName, L"EtwRegistration");

                        if (!RtlEqualUnicodeString(objectTypeName, &etwRegistrationName, FALSE))
                        {
                            status = STATUS_OBJECT_TYPE_MISMATCH;
                        }
                    }
                    else
                    {
                        status = STATUS_NOT_SUPPORTED;
                    }

                    if (NT_SUCCESS(status))
                    {
                        guidEntry = *(PVOID *)PTR_ADD_OFFSET(etwReg, KphDynEreGuidEntry);

                        if (guidEntry)
                            basicInfo.Guid = *(GUID *)PTR_ADD_OFFSET(guidEntry, KphDynEgeGuid);
                        else
                            memset(&basicInfo.Guid, 0, sizeof(GUID));

                        basicInfo.SessionId = 0; // not implemented

                        if (ObjectInformationLength == sizeof(ETWREG_BASIC_INFORMATION))
                        {
                            __try
                            {
                                *(PETWREG_BASIC_INFORMATION)ObjectInformation = basicInfo;
                            }
                            __except (EXCEPTION_EXECUTE_HANDLER)
                            {
                                status = GetExceptionCode();
                            }
                        }
                        else
                        {
                            status = STATUS_INFO_LENGTH_MISMATCH;
                        }
                    }

                    ObDereferenceObject(etwReg);
                }
            }
            else
            {
                status = STATUS_NOT_SUPPORTED;
            }

            returnLength = sizeof(ETWREG_BASIC_INFORMATION);
        }
        break;
    case KphObjectFileObjectInformation:
        {
            PFILE_OBJECT fileObject;
            KPH_FILE_OBJECT_INFORMATION objectInfo;

            KeStackAttachProcess(process, &apcState);
            status = ObReferenceObjectByHandle(
                Handle,
                0,
                *IoFileObjectType,
                referenceMode,
                &fileObject,
                NULL
                );
            KeUnstackDetachProcess(&apcState);

            if (NT_SUCCESS(status))
            {
                objectInfo.LockOperation = fileObject->LockOperation;
                objectInfo.DeletePending = fileObject->DeletePending;
                objectInfo.ReadAccess = fileObject->ReadAccess;
                objectInfo.WriteAccess = fileObject->WriteAccess;
                objectInfo.DeleteAccess = fileObject->DeleteAccess;
                objectInfo.SharedRead = fileObject->SharedRead;
                objectInfo.SharedWrite = fileObject->SharedWrite;
                objectInfo.SharedDelete = fileObject->SharedDelete;
                objectInfo.CurrentByteOffset = fileObject->CurrentByteOffset;
                objectInfo.Flags = fileObject->Flags;

                if (ObjectInformationLength == sizeof(KPH_FILE_OBJECT_INFORMATION))
                {
                    __try
                    {
                        *(PKPH_FILE_OBJECT_INFORMATION)ObjectInformation = objectInfo;
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                        status = GetExceptionCode();
                    }
                }
                else
                {
                    status = STATUS_INFO_LENGTH_MISMATCH;
                }

                ObDereferenceObject(fileObject);
            }

            returnLength = sizeof(KPH_FILE_OBJECT_INFORMATION);
        }
        break;
    case KphObjectFileObjectDriver:
        {
            PFILE_OBJECT fileObject;
            HANDLE driverHandle;

            if (ObjectInformationLength == sizeof(KPH_FILE_OBJECT_DRIVER))
            {
                KeStackAttachProcess(process, &apcState);
                status = ObReferenceObjectByHandle(
                    Handle,
                    0,
                    *IoFileObjectType,
                    referenceMode,
                    &fileObject,
                    NULL
                    );
                KeUnstackDetachProcess(&apcState);

                if (NT_SUCCESS(status))
                {
                    if (fileObject->DeviceObject && fileObject->DeviceObject->DriverObject)
                    {
                        status = ObOpenObjectByPointer(
                            fileObject->DeviceObject->DriverObject,
                            0,
                            NULL,
                            SYNCHRONIZE,
                            *IoDriverObjectType,
                            AccessMode,
                            &driverHandle
                            );
                    }
                    else
                    {
                        driverHandle = NULL;
                    }

                    if (NT_SUCCESS(status))
                    {
                        __try
                        {
                            ((PKPH_FILE_OBJECT_DRIVER)ObjectInformation)->DriverHandle = driverHandle;
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            status = GetExceptionCode();
                        }
                    }

                    ObDereferenceObject(fileObject);
                }
            }
            else
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
            }

            returnLength = sizeof(KPH_FILE_OBJECT_DRIVER);
        }
        break;
    default:
        status = STATUS_INVALID_INFO_CLASS;
        returnLength = 0;
        break;
    }

    ObDereferenceObject(process);

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
 * Sets object information.
 *
 * \param ProcessHandle A handle to a process.
 * \param Handle A handle which is present in the process referenced by \a ProcessHandle.
 * \param ObjectInformationClass The type of information to set.
 * \param ObjectInformation A buffer which contains the information to set.
 * \param ObjectInformationLength The number of bytes present in \a ObjectInformation.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiSetInformationObject(
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

    PAGED_CODE();

    if (AccessMode != KernelMode)
    {
        ULONG alignment;

        switch (ObjectInformationClass)
        {
        case KphObjectHandleFlagInformation:
            alignment = sizeof(BOOLEAN);
            break;
        default:
            alignment = sizeof(ULONG);
            break;
        }

        __try
        {
            ProbeForRead(ObjectInformation, ObjectInformationLength, alignment);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    status = ObReferenceObjectByHandle(
        ProcessHandle,
        0,
        *PsProcessType,
        AccessMode,
        &process,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (process == PsInitialSystemProcess)
    {
        Handle = MakeKernelHandle(Handle);
    }
    else
    {
        if (IsKernelHandle(Handle))
        {
            ObDereferenceObject(process);
            return STATUS_INVALID_HANDLE;
        }
    }

    switch (ObjectInformationClass)
    {
    case KphObjectHandleFlagInformation:
        {
            OBJECT_HANDLE_FLAG_INFORMATION handleFlagInfo;

            if (ObjectInformationLength == sizeof(OBJECT_HANDLE_FLAG_INFORMATION))
            {
                __try
                {
                    handleFlagInfo = *(POBJECT_HANDLE_FLAG_INFORMATION)ObjectInformation;
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    status = GetExceptionCode();
                }
            }
            else
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
            }

            if (NT_SUCCESS(status))
            {
                KeStackAttachProcess(process, &apcState);
                status = ObSetHandleAttributes(Handle, &handleFlagInfo, KernelMode);
                KeUnstackDetachProcess(&apcState);
            }
        }
        break;
    default:
        status = STATUS_INVALID_INFO_CLASS;
        break;
    }

    ObDereferenceObject(process);

    return status;
}

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

    PAGED_CODE();

    // Open the object.
    status = ObOpenObjectByName(
        ObjectAttributes,
        ObjectType,
        AccessMode,
        NULL,
        DesiredAccess,
        NULL,
        &objectHandle
        );

    // Pass the handle back.
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

    return status;
}
