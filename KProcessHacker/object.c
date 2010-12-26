/*
 * KProcessHacker
 * 
 * Copyright (C) 2010 wj32
 * 
 * This file is part of Process Hacker.
 * 
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
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
    ULONG BufferLength;
    ULONG CurrentIndex;
    NTSTATUS Status;
} KPHP_ENUMERATE_PROCESS_HANDLES_CONTEXT, *PKPHP_ENUMERATE_PROCESS_HANDLES_CONTEXT;

BOOLEAN KphpEnumerateProcessHandlesEnumCallback(
    __inout PHANDLE_TABLE_ENTRY HandleTableEntry,
    __in HANDLE Handle,
    __in PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KphGetObjectType)
#pragma alloc_text(PAGE, KphReferenceProcessHandleTable)
#pragma alloc_text(PAGE, KphDereferenceProcessHandleTable)
#pragma alloc_text(PAGE, KphpEnumerateProcessHandlesEnumCallback)
#pragma alloc_text(PAGE, KpiEnumerateProcessHandles)
#pragma alloc_text(PAGE, KphQueryNameObject)
#pragma alloc_text(PAGE, KphQueryNameFileObject)
#pragma alloc_text(PAGE, KpiQueryInformationObject)
#pragma alloc_text(PAGE, KpiSetInformationObject)
#pragma alloc_text(PAGE, KphDuplicateObject)
#pragma alloc_text(PAGE, KpiDuplicateObject)
#pragma alloc_text(PAGE, KphOpenNamedObject)
#endif

/* This attribute is now stored in the GrantedAccess field. */
ULONG ObpAccessProtectCloseBit = 0x2000000;

POBJECT_TYPE KphGetObjectType(
    __in PVOID Object
    )
{
    // XP to Vista: A pointer to the object type is 
    // stored in the object header.
    if (
        KphDynNtVersion >= PHNT_WINXP && 
        KphDynNtVersion <= PHNT_VISTA
        )
    {
        return OBJECT_TO_OBJECT_HEADER(Object)->Type;
    }
    // Seven and above: An index to an internal object type 
    // table is stored in the object header. Luckily we have 
    // a new exported function, ObGetObjectType, to get 
    // the object type.
    else if (KphDynNtVersion >= PHNT_WIN7)
    {
        if (ObGetObjectType_I)
            return ObGetObjectType_I(Object);
        else
            return NULL;
    }
    else
    {
        return NULL;
    }
}

PHANDLE_TABLE KphReferenceProcessHandleTable(
    __in PEPROCESS Process
    )
{
    PHANDLE_TABLE handleTable = NULL;

    // Fail if we don't have an offset.
    if (KphDynEpObjectTable == -1)
        return NULL;

    // Prevent the process from terminating and get its handle table.
    if (KphAcquireProcessRundownProtection(Process))
    {
        handleTable = *(PHANDLE_TABLE *)((ULONG_PTR)Process + KphDynEpObjectTable);

        if (!handleTable)
            KphReleaseProcessRundownProtection(Process);
    }

    return handleTable;
}

VOID KphDereferenceProcessHandleTable(
    __in PEPROCESS Process
    )
{
    KphReleaseProcessRundownProtection(Process);
}

BOOLEAN KphpEnumerateProcessHandlesEnumCallback(
    __inout PHANDLE_TABLE_ENTRY HandleTableEntry,
    __in HANDLE Handle,
    __in PVOID Context
    )
{
    PKPHP_ENUMERATE_PROCESS_HANDLES_CONTEXT context = Context;
    KPH_PROCESS_HANDLE handleInfo;
    POBJECT_TYPE objectType;
    PKPH_PROCESS_HANDLE_INFORMATION buffer;
    ULONG i;

    buffer = context->Buffer;

    handleInfo.Handle = Handle;
    handleInfo.Object = &((POBJECT_HEADER)ObpDecodeObject(HandleTableEntry->Object))->Body;
    handleInfo.GrantedAccess = ObpDecodeGrantedAccess(HandleTableEntry->GrantedAccess);
    handleInfo.Reserved1 = 0;
    handleInfo.HandleAttributes = ObpGetHandleAttributes(HandleTableEntry);
    handleInfo.Reserved2 = 0;

    objectType = KphGetObjectType(handleInfo.Object);

    if (objectType && KphDynOtIndex != -1)
    {
        if (KphDynNtVersion >= PHNT_WIN7)
            handleInfo.ObjectTypeIndex = (USHORT)*(PUCHAR)((ULONG_PTR)objectType + KphDynOtIndex);
        else
            handleInfo.ObjectTypeIndex = (USHORT)*(PULONG)((ULONG_PTR)objectType + KphDynOtIndex);
    }
    else
    {
        handleInfo.ObjectTypeIndex = -1;
    }

    // Increment the index regardless of whether the information will be written;
    // this will allow the parent function to report the correct return length.
    i = context->CurrentIndex++;

    /* Only write if we have not exceeded the buffer length. */
    if (sizeof(ULONG) + context->CurrentIndex * sizeof(KPH_PROCESS_HANDLE) <= context->BufferLength)
    {
        __try
        {
            buffer->Handles[i] = handleInfo;
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

NTSTATUS KpiEnumerateProcessHandles(
    __in HANDLE ProcessHandle,
    __out_bcount(BufferLength) PVOID Buffer,
    __in_opt ULONG BufferLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    BOOLEAN result;
    PEPROCESS process;
    PHANDLE_TABLE handleTable;
    KPHP_ENUMERATE_PROCESS_HANDLES_CONTEXT context;

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
    context.BufferLength = BufferLength;
    context.CurrentIndex = 0;
    context.Status = STATUS_SUCCESS;

    /* Enumerate the handles. */
    result = ExEnumHandleTable(
        handleTable,
        KphpEnumerateProcessHandlesEnumCallback,
        &context,
        NULL
        );
    KphDereferenceProcessHandleTable(process);
    ObDereferenceObject(process);

    // Write the number of handles if we can.
    if (BufferLength >= FIELD_OFFSET(KPH_PROCESS_HANDLE_INFORMATION, Handles))
    {
        if (AccessMode != KernelMode)
        {
            __try
            {
                ((PKPH_PROCESS_HANDLE_INFORMATION)Buffer)->HandleCount = context.CurrentIndex;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                return GetExceptionCode();
            }
        }
        else
        {
            ((PKPH_PROCESS_HANDLE_INFORMATION)Buffer)->HandleCount = context.CurrentIndex;
        }
    }

    // Supply the return length if the caller wanted it.
    if (ReturnLength)
    {
        ULONG returnLength;

        // CurrentIndex should contain the number of handles, so we simply multiply it
        // by the size of each entry.
        returnLength = sizeof(ULONG) + context.CurrentIndex * sizeof(KPH_PROCESS_HANDLE);

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

NTSTATUS KphQueryNameObject(
    __in PVOID Object,
    __out_bcount(BufferLength) POBJECT_NAME_INFORMATION Buffer,
    __in ULONG BufferLength,
    __out PULONG ReturnLength
    )
{
    NTSTATUS status;
    POBJECT_TYPE objectType;

    objectType = KphGetObjectType(Object);

    // Check if we are going to hang when querying the object, and use
    // the special file object query function if needed.
    if (objectType == *IoFileObjectType && 
        (((PFILE_OBJECT)Object)->Busy || ((PFILE_OBJECT)Object)->Waiters))
    {
        status = KphQueryNameFileObject(Object, Buffer, BufferLength, ReturnLength);
    }
    else
    {
        status = ObQueryNameString(Object, Buffer, BufferLength, ReturnLength);
    }

    return status;
}

NTSTATUS KphQueryNameFileObject(
    __in PFILE_OBJECT FileObject,
    __out_bcount(BufferLength) POBJECT_NAME_INFORMATION Buffer,
    __in ULONG BufferLength,
    __out PULONG ReturnLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG returnLength;
    PCHAR objectName;
    ULONG usedLength;
    ULONG subNameLength;
    PFILE_OBJECT relatedFileObject;

    // We need at least the size of OBJECT_NAME_INFORMATION to
    // continue.
    if (BufferLength < sizeof(OBJECT_NAME_INFORMATION))
    {
        *ReturnLength = sizeof(OBJECT_NAME_INFORMATION);

        return STATUS_BUFFER_TOO_SMALL;
    }

    // Assume failure.
    Buffer->Name.Length = 0;
    // We will place the object name directly after the
    // UNICODE_STRING structure in the buffer.
    Buffer->Name.Buffer = (PWSTR)((ULONG_PTR)Buffer + sizeof(OBJECT_NAME_INFORMATION));
    // Retain a local pointer to the object name so we
    // can manipulate the pointer.
    objectName = (PCHAR)Buffer->Name.Buffer;
    // A variable that keeps track of how much space we
    // have used.
    usedLength = sizeof(OBJECT_NAME_INFORMATION);

    // Check if the file object has an associated device
    // (e.g. "\Device\NamedPipe", "\Device\Mup"). We can
    // use the user-supplied buffer for this since if the
    // buffer isn't big enough, we can't proceed anyway
    // (we are going to use the name).
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
            *ReturnLength = returnLength;

            return status;
        }

        // The UNICODE_STRING in the buffer is now filled in.
        // We will append to the object name later, so
        // we need to fix the object name pointer by adding
        // the length, in bytes, of the device name string we
        // just got.
        objectName += Buffer->Name.Length;
        usedLength += Buffer->Name.Length;
    }

    // Check if the file object has a file name component. If not,
    // we can't do anything else, so we just return the name we
    // have already.
    if (!FileObject->FileName.Buffer)
    {
        *ReturnLength = usedLength;

        return STATUS_SUCCESS;
    }

    // The file object has a name. We need to walk up the file
    // object chain and append the names of the related file
    // objects in reverse order. This means we need to calculate
    // the total length first.

    relatedFileObject = FileObject;
    subNameLength = 0;

    do
    {
        subNameLength += relatedFileObject->FileName.Length;

        // Avoid infinite loops.
        if (relatedFileObject == relatedFileObject->RelatedFileObject)
            break;

        relatedFileObject = relatedFileObject->RelatedFileObject;
    }
    while (relatedFileObject);

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
    }
    while (relatedFileObject);

    // Update the length.
    Buffer->Name.Length += (USHORT)subNameLength;

    // Pass the return length back.
    *ReturnLength = usedLength;

    return STATUS_SUCCESS;
}

NTSTATUS KpiQueryInformationObject(
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass,
    __out_bcount(ObjectInformationLength) PVOID ObjectInformation,
    __in ULONG ObjectInformationLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;
    KAPC_STATE apcState;
    ULONG returnLength;

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
        // A check was added in Windows 7 - if we're attached to the System process,
        // the handle must be a kernel handle.
        Handle = MakeKernelHandle(Handle);
    }
    else
    {
        // Make sure the handle isn't a kernel handle if we're not attached to the 
        // System process.
        if (IsKernelHandle(Handle))
        {
            ObDereferenceObject(process);
            return STATUS_INVALID_HANDLE;
        }
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
            POBJECT_NAME_INFORMATION nameInfo;

            nameInfo = ExAllocatePoolWithQuotaTag(PagedPool, ObjectInformationLength, 'QhpK');

            if (nameInfo)
            {
                // Make sure we don't leak any data.
                memset(nameInfo, 0, ObjectInformationLength);

                KeStackAttachProcess(process, &apcState);
                status = KphQueryNameObject(
                    Handle,
                    nameInfo,
                    ObjectInformationLength,
                    &returnLength
                    );
                KeUnstackDetachProcess(&apcState);

                if (NT_SUCCESS(status))
                {
                    // Fix up the buffer pointer.
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
        }
        break;
    case KphObjectTypeInformation:
        {
            POBJECT_TYPE_INFORMATION typeInfo;

            typeInfo = ExAllocatePoolWithQuotaTag(PagedPool, ObjectInformationLength, 'QhpK');

            if (typeInfo)
            {
                memset(typeInfo, 0, ObjectInformationLength);

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
            if (
                KphDynEgeGuid != -1 &&
                KphDynEreGuidEntry != -1 &&
                KphDynOtName != -1
                )
            {
                // Attach to the process and get a pointer to the object.
                // We don't have a pointer to the EtwRegistration object type,
                // so we'll just have to check the type name.

                KeStackAttachProcess(process, &apcState);
                status = ObReferenceObjectByHandle(
                    Handle,
                    0,
                    NULL,
                    AccessMode,
                    &etwReg,
                    NULL
                    );
                KeUnstackDetachProcess(&apcState);

                // Check the type name.

                objectType = KphGetObjectType(etwReg);

                if (objectType)
                {
                    objectTypeName = (PUNICODE_STRING)((ULONG_PTR)objectType + KphDynOtName);
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
                    guidEntry = *(PVOID *)((ULONG_PTR)etwReg + KphDynEreGuidEntry);

                    if (guidEntry)
                        basicInfo.Guid = *(GUID *)((ULONG_PTR)guidEntry + KphDynEgeGuid);
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
            else
            {
                status = STATUS_NOT_SUPPORTED;
            }

            returnLength = sizeof(ETWREG_BASIC_INFORMATION);
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

NTSTATUS KpiSetInformationObject(
    __in HANDLE ProcessHandle,
    __in HANDLE Handle,
    __in KPH_OBJECT_INFORMATION_CLASS ObjectInformationClass,
    __in_bcount(ObjectInformationLength) PVOID ObjectInformation,
    __in ULONG ObjectInformationLength,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;
    KAPC_STATE apcState;

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForRead(ObjectInformation, ObjectInformationLength, sizeof(ULONG));
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
    }

    ObDereferenceObject(process);

    return status;
}

NTSTATUS KphDuplicateObject(
    __in PEPROCESS SourceProcess,
    __in_opt PEPROCESS TargetProcess,
    __in HANDLE SourceHandle,
    __out_opt PHANDLE TargetHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __in ULONG Options,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    KPROCESSOR_MODE referenceMode;
    BOOLEAN sourceAttached = FALSE;
    BOOLEAN targetAttached = FALSE;
    KAPC_STATE apcState;
    PVOID object;
    HANDLE objectHandle;

    // Validate the parameters.

    if (!TargetProcess || !TargetHandle)
    {
        if (!(Options & DUPLICATE_CLOSE_SOURCE))
            return STATUS_INVALID_PARAMETER_MIX;
    }

    if (AccessMode != KernelMode && (HandleAttributes & OBJ_KERNEL_HANDLE))
    {
        return STATUS_INVALID_PARAMETER_6;
    }

    // Fix the source handle if the source process is the System process.
    if (SourceProcess == PsInitialSystemProcess)
    {
        SourceHandle = MakeKernelHandle(SourceHandle);
        referenceMode = KernelMode;
    }
    else
    {
        referenceMode = UserMode;
    }

    // Closing handles in the current process from kernel-mode is *bad*.

    // Example: the handle being closed is a handle to the file object
    // on which this very request is being sent. Deadlock.
    //
    // If we add the current process check, the handle can't possibly
    // be the one the request is being sent on, since system calls
    // only operate on handles from the current process.
    if (SourceProcess == PsGetCurrentProcess())
    {
        if (Options & DUPLICATE_CLOSE_SOURCE)
            return STATUS_CANT_TERMINATE_SELF;
    }

    // Check if we need to attach to the source process.
    if (SourceProcess != PsGetCurrentProcess())
    {
        KeStackAttachProcess(SourceProcess, &apcState);
        sourceAttached = TRUE;
    }

    // If the caller wants us to close the source handle, do it now.
    if (Options & DUPLICATE_CLOSE_SOURCE)
    {
        status = ObCloseHandle(SourceHandle, referenceMode);

        if (sourceAttached)
            KeUnstackDetachProcess(&apcState);

        return status;
    }

    // Reference the object and detach from the source process.
    status = ObReferenceObjectByHandle(
        SourceHandle,
        0,
        NULL,
        referenceMode,
        &object,
        NULL
        );
    if (sourceAttached)
        KeUnstackDetachProcess(&apcState);

    if (!NT_SUCCESS(status))
        return status;

    // Check if we need to attach to the target process.
    if (TargetProcess != PsGetCurrentProcess())
    {
        KeStackAttachProcess(TargetProcess, &apcState);
        targetAttached = TRUE;
    }

    // Open the object and detach from the target process.

    status = ObOpenObjectByPointer(
        object,
        HandleAttributes,
        NULL,
        DesiredAccess,
        NULL,
        KernelMode,
        &objectHandle
        );
    ObDereferenceObject(object);

    if (targetAttached)
        KeUnstackDetachProcess(&apcState);

    if (NT_SUCCESS(status))
        *TargetHandle = objectHandle;
    else
        *TargetHandle = NULL;

    return status;
}

NTSTATUS KpiDuplicateObject(
    __in HANDLE SourceProcessHandle,
    __in HANDLE SourceHandle,
    __in_opt HANDLE TargetProcessHandle,
    __out_opt PHANDLE TargetHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG HandleAttributes,
    __in ULONG Options,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS sourceProcess = NULL;
    PEPROCESS targetProcess = NULL;
    HANDLE targetHandle;

    if (AccessMode != KernelMode)
    {
        __try
        {
            if (TargetHandle)
                ProbeForWrite(TargetHandle, sizeof(HANDLE), 1);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    status = ObReferenceObjectByHandle(
        SourceProcessHandle,
        PROCESS_DUP_HANDLE,
        *PsProcessType,
        AccessMode,
        &sourceProcess,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    // Target handle is optional.
    if (TargetProcessHandle)
    {
        status = ObReferenceObjectByHandle(
            TargetProcessHandle,
            PROCESS_DUP_HANDLE,
            *PsProcessType,
            AccessMode,
            &targetProcess,
            NULL
            );

        if (!NT_SUCCESS(status))
        {
            ObDereferenceObject(sourceProcess);
            return status;
        }
    }

    status = KphDuplicateObject(
        sourceProcess,
        targetProcess,
        SourceHandle,
        &targetHandle,
        DesiredAccess,
        HandleAttributes,
        Options,
        AccessMode
        );

    if (TargetHandle)
    {
        if (AccessMode != KernelMode)
        {
            __try
            {
                *TargetHandle = targetHandle;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = STATUS_ACCESS_VIOLATION;
            }
        }
        else
        {
            *TargetHandle = targetHandle;
        }
    }

    if (targetProcess)
        ObDereferenceObject(targetProcess);

    ObDereferenceObject(sourceProcess);

    return status;
}

NTSTATUS KphOpenNamedObject(
    __out PHANDLE ObjectHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in POBJECT_TYPE ObjectType,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE objectHandle;
    UNICODE_STRING capturedObjectName;
    OBJECT_ATTRIBUTES objectAttributes = { 0 };

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(ObjectHandle, sizeof(HANDLE), sizeof(HANDLE));
            ProbeForRead(ObjectAttributes, sizeof(OBJECT_ATTRIBUTES), sizeof(ULONG));

            if (ObjectAttributes->ObjectName)
                KphProbeForReadUnicodeString(ObjectAttributes->ObjectName);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    __try
    {
        // Copy the object attributes structure.
        memcpy(&objectAttributes, ObjectAttributes, sizeof(OBJECT_ATTRIBUTES));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    // Verify parameters.
    if (!objectAttributes.ObjectName)
        return STATUS_INVALID_PARAMETER;

    // Make sure the root directory handle isn't a kernel handle if
    // we're from user-mode.
    if (AccessMode != KernelMode && IsKernelHandle(objectAttributes.RootDirectory))
        return STATUS_INVALID_PARAMETER;

    // Capture the ObjectName string.
    status = KphCaptureUnicodeString(objectAttributes.ObjectName, &capturedObjectName);

    if (!NT_SUCCESS(status))
        return status;

    // Set the new string in the object attributes.
    objectAttributes.ObjectName = &capturedObjectName;
    // Make sure the SecurityDescriptor and SecurityQualityOfService fields are NULL
    // since we haven't probed them. They don't apply anyway because we're opening an 
    // object here.
    objectAttributes.SecurityDescriptor = NULL;
    objectAttributes.SecurityQualityOfService = NULL;

    // Open the object.
    status = ObOpenObjectByName(
        &objectAttributes,
        ObjectType,
        KernelMode,
        NULL,
        DesiredAccess,
        NULL,
        &objectHandle
        );

    // Free the captured ObjectName.
    KphFreeCapturedUnicodeString(&capturedObjectName);

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
