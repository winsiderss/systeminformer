/*
 * Process Hacker Driver - 
 *   object manager
 * 
 * Copyright (C) 2009 wj32
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

#include "include/kph.h"

BOOLEAN KphpQueryProcessHandlesEnumCallback(
    __inout PHANDLE_TABLE_ENTRY HandleTableEntry,
    __in HANDLE Handle,
    __in POBP_QUERY_PROCESS_HANDLES_DATA Context
    );

BOOLEAN KphpSetHandleGrantedAccessEnumCallback(
    __inout PHANDLE_TABLE_ENTRY HandleTableEntry,
    __in HANDLE Handle,
    __in POBP_SET_HANDLE_GRANTED_ACCESS_DATA Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KphDuplicateObject)
#pragma alloc_text(PAGE, ObDuplicateObject)
#endif

/* This attribute is now stored in the GrantedAccess field. */
ULONG ObpAccessProtectCloseBit = 0x2000000;

/* KphDuplicateObject
 * 
 * Duplicates a handle from the source process to the target process.
 */
NTSTATUS KphDuplicateObject(
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
    
    if (TargetHandle && AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(TargetHandle, sizeof(HANDLE), 1);
            *TargetHandle = NULL;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return STATUS_ACCESS_VIOLATION;
        }
    }
    
    status = ObReferenceObjectByHandle(
        SourceProcessHandle,
        PROCESS_DUP_HANDLE,
        *PsProcessType,
        KernelMode,
        &sourceProcess,
        NULL
        );
    
    if (!NT_SUCCESS(status))
        return status;
    
    /* Target handle is optional. */
    if (TargetProcessHandle)
    {
        status = ObReferenceObjectByHandle(
            TargetProcessHandle,
            PROCESS_DUP_HANDLE,
            *PsProcessType,
            KernelMode,
            &targetProcess,
            NULL
            );
        
        if (!NT_SUCCESS(status))
            return status;
    }
    
    /* Fix the source handle if the source process is 
     * the system process.
     */
    if (sourceProcess == PsInitialSystemProcess)
        MakeKernelHandle(SourceHandle);
    
    /* Call the internal function. */
    status = ObDuplicateObject(
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
        __try
        {
            *TargetHandle = targetHandle;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = STATUS_ACCESS_VIOLATION;
        }
    }
    
    ObDereferenceObject(sourceProcess);
    if (targetProcess)
        ObDereferenceObject(targetProcess);
    
    return status;
}

/* KphEnumProcessHandleTable
 * 
 * Enumerates the handles in the specified process' handle table.
 */
BOOLEAN KphEnumProcessHandleTable(
    __in PEPROCESS Process,
    __in PEX_ENUM_HANDLE_CALLBACK EnumHandleProcedure,
    __inout PVOID Context,
    __out_opt PHANDLE Handle
    )
{
    BOOLEAN result = FALSE;
    PHANDLE_TABLE handleTable = NULL;
    
    handleTable = ObReferenceProcessHandleTable(Process);
    
    if (!handleTable)
        return FALSE;
    
    result = ExEnumHandleTable(
        handleTable,
        EnumHandleProcedure,
        Context,
        Handle
        );
    ObDereferenceProcessHandleTable(Process);
    
    return result;
}

/* KphGetObjectTypeNt
 * 
 * Gets the type of an object.
 */
POBJECT_TYPE KphGetObjectTypeNt(
    __in PVOID Object
    )
{
    /* XP to Vista: A pointer to the object type is 
     * stored in the object header.
     */
    if (
        WindowsVersion >= WINDOWS_XP && 
        WindowsVersion <= WINDOWS_VISTA
        )
    {
        return OBJECT_TO_OBJECT_HEADER(Object)->Type;
    }
    /* Seven and above: An index to an internal object type 
     * table is stored in the object header. Luckily we have 
     * a new exported function, ObGetObjectType, to get 
     * the object type.
     */
    else if (WindowsVersion >= WINDOWS_7)
    {
        return ObGetObjectType(Object);
    }
    else
    {
        return NULL;
    }
}

/* KphOpenDirectoryObject
 * 
 * Opens a directory object.
 */
NTSTATUS KphOpenDirectoryObject(
    __out PHANDLE DirectoryObjectHandle,
    __in ACCESS_MASK DesiredAccess,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in KPROCESSOR_MODE AccessMode
    )
{
    return KphOpenNamedObject(
        DirectoryObjectHandle,
        DesiredAccess,
        ObjectAttributes,
        *ObDirectoryObjectType,
        AccessMode
        );
}

/* KphOpenNamedObject
 * 
 * Opens a named object.
 */
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
    
    if (!ObjectAttributes)
        return STATUS_INVALID_PARAMETER;
    
    /* Probe user input. */
    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(ObjectHandle, sizeof(HANDLE), 1);
            ProbeForRead(ObjectAttributes, sizeof(OBJECT_ATTRIBUTES), 1);
            
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
        /* Copy the object attributes structure. */
        memcpy(&objectAttributes, ObjectAttributes, sizeof(OBJECT_ATTRIBUTES));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }
    
    /* Verify parameters. */
    if (!objectAttributes.ObjectName)
        return STATUS_INVALID_PARAMETER;
    
    /* Make sure the root directory handle isn't a kernel handle if 
     * we're from user-mode.
     */
    if (AccessMode != KernelMode && IsKernelHandle(objectAttributes.RootDirectory))
        return STATUS_INVALID_PARAMETER;
    
    /* Capture the ObjectName string. */
    status = KphCaptureUnicodeString(
        objectAttributes.ObjectName,
        &capturedObjectName
        );
    
    if (!NT_SUCCESS(status))
        return status;
    
    /* Set the new string in the object attributes. */
    objectAttributes.ObjectName = &capturedObjectName;
    /* Make sure the SecurityDescriptor and SecurityQualityOfService fields are NULL 
     * since we haven't probed them.
     */
    objectAttributes.SecurityDescriptor = NULL;
    objectAttributes.SecurityQualityOfService = NULL;
    
    /* Open the object. */
    status = ObOpenObjectByName(
        &objectAttributes,
        ObjectType,
        KernelMode,
        NULL,
        DesiredAccess,
        NULL,
        &objectHandle
        );
    
    /* Free the captured ObjectName. */
    KphFreeCapturedUnicodeString(&capturedObjectName);
    
    /* Pass the handle back. */
    __try
    {
        *ObjectHandle = objectHandle;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }
    
    return status;
}

/* KphOpenType
 * 
 * Opens a type object.
 */
NTSTATUS KphOpenType(
    __out PHANDLE TypeHandle,
    __in POBJECT_ATTRIBUTES ObjectAttributes,
    __in KPROCESSOR_MODE AccessMode
    )
{
    return KphOpenNamedObject(
        TypeHandle,
        0,
        ObjectAttributes,
        *ObTypeObjectType,
        AccessMode
        );
}

/* KphQueryFileObjectName
 * 
 * Queries the name of a file object.
 * 
 * Technique from YAPM.
 */
NTSTATUS KphQueryNameFileObject(
    __in PFILE_OBJECT FileObject,
    __inout_bcount(BufferLength) PUNICODE_STRING Buffer,
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
    
    /* We need at least the size of UNICODE_STRING to 
     * continue.
     */
    if (BufferLength < sizeof(UNICODE_STRING))
    {
        *ReturnLength = sizeof(UNICODE_STRING);
        
        return STATUS_BUFFER_TOO_SMALL;
    }
    
    /* Assume failure. */
    Buffer->Length = 0;
    /* We will place the object name directly after the 
     * UNICODE_STRING structure in the buffer.
     */
    Buffer->Buffer = (PWSTR)PTR_ADD_OFFSET(Buffer, sizeof(UNICODE_STRING));
    /* Retain a local pointer to the object name so we 
     * can manipulate the pointer.
     */
    objectName = (PCHAR)Buffer->Buffer;
    /* A variable that keeps track of how much space we 
     * have used.
     */
    usedLength = sizeof(UNICODE_STRING);
    
    /* Check if the file object has an associated device 
     * (e.g. "\Device\NamedPipe", "\Device\Mup"). We can 
     * use the user-supplied buffer for this since if the 
     * buffer isn't big enough, we can't proceed anyway 
     * (we are going to use the name).
     */
    if (FileObject->DeviceObject)
    {
        status = ObQueryNameString(
            FileObject->DeviceObject,
            (POBJECT_NAME_INFORMATION)Buffer,
            BufferLength,
            &returnLength
            );
        
        if (!NT_SUCCESS(status))
        {
            *ReturnLength = returnLength;
            
            return status;
        }
        
        /* The UNICODE_STRING in the buffer is now filled in. 
         * We will append to the object name later, so 
         * we need to fix the object name pointer by adding 
         * the length, in bytes, of the device name string we 
         * just got.
         */
        objectName += Buffer->Length;
        usedLength += Buffer->Length;
    }
    
    /* Check if the file object has a file name component. If not, 
     * we can't do anything else, so we just return the name we 
     * have already.
     */
    if (!FileObject->FileName.Buffer)
    {
        *ReturnLength = usedLength;
        
        return STATUS_SUCCESS;
    }
    
    /* The file object has a name. We need to walk up the file 
     * object tree and append the names of the related file 
     * objects in reverse order. This means we need to calculate 
     * the total length first.
     */
    
    relatedFileObject = FileObject;
    subNameLength = 0;
    
    do
    {
        subNameLength += relatedFileObject->FileName.Length;
        
        /* Avoid infinite loops. */
        if (relatedFileObject == relatedFileObject->RelatedFileObject)
            break;
        
        relatedFileObject = relatedFileObject->RelatedFileObject;
    }
    while (relatedFileObject);
    
    usedLength += subNameLength;
    
    /* Check if we have enough space to write the whole thing. */
    if (usedLength > BufferLength)
    {
        *ReturnLength = usedLength;
        
        return STATUS_BUFFER_TOO_SMALL;
    }
    
    /* We're ready to begin copying the names. */
    
    /* Add the name length because we're copying in reverse order. */
    objectName += subNameLength;
    
    relatedFileObject = FileObject;
    
    do
    {
        objectName -= relatedFileObject->FileName.Length;
        memcpy(objectName, relatedFileObject->FileName.Buffer, relatedFileObject->FileName.Length);
        
        /* Avoid infinite loops. */
        if (relatedFileObject == relatedFileObject->RelatedFileObject)
            break;
        
        relatedFileObject = relatedFileObject->RelatedFileObject;
    }
    while (relatedFileObject);
    
    /* Update the length. */
    Buffer->Length += (USHORT)subNameLength;
    
    /* Pass the return length back. */
    *ReturnLength = usedLength;
    
    return STATUS_SUCCESS;
}

/* KphQueryObjectName
 * 
 * Queries the name of an object.
 */
NTSTATUS KphQueryNameObject(
    __in PVOID Object,
    __inout_bcount(BufferLength) PUNICODE_STRING Buffer,
    __in ULONG BufferLength,
    __out PULONG ReturnLength
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    POBJECT_TYPE objectType;
    
    objectType = KphGetObjectTypeNt(Object);
    
    /* Check if we are going to hang when querying the object, and use 
     * the special file object query function if needed.
     */
    if (
        (objectType == *IoFileObjectType) && 
        (((PFILE_OBJECT)Object)->Busy || ((PFILE_OBJECT)Object)->Waiters)
        )
    {
        status = KphQueryNameFileObject((PFILE_OBJECT)Object, Buffer, BufferLength, ReturnLength);
    }
    else
    {
        status = ObQueryNameString(Object, (POBJECT_NAME_INFORMATION)Buffer, BufferLength, ReturnLength);
    }
    
    return status;
}

/* KphQueryProcessHandles
 * 
 * Queries a process handle table.
 */
NTSTATUS KphQueryProcessHandles(
    __in HANDLE ProcessHandle,
    __out_bcount_opt(BufferLength) PPROCESS_HANDLE_INFORMATION Buffer,
    __in_opt ULONG BufferLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    BOOLEAN result;
    PEPROCESS processObject;
    OBP_QUERY_PROCESS_HANDLES_DATA context;
    
    /* Probe buffer contents. */
    if (AccessMode != KernelMode)
    {
        __try
        {
            if (Buffer)
                ProbeForWrite(Buffer, BufferLength, 1);
            if (ReturnLength)
                ProbeForWrite(ReturnLength, sizeof(ULONG), 1);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }
    
    /* Reference the process object. */
    status = ObReferenceObjectByHandle(
        ProcessHandle,
        PROCESS_QUERY_INFORMATION,
        *PsProcessType,
        KernelMode,
        &processObject,
        NULL
        );
    
    if (!NT_SUCCESS(status))
        return status;
    
    /* Initialize the enumeration context. */
    context.Buffer = Buffer;
    context.BufferLength = BufferLength;
    context.CurrentIndex = 0;
    context.Status = STATUS_SUCCESS;
    
    /* Enumerate the handles. */
    result = KphEnumProcessHandleTable(
        processObject,
        KphpQueryProcessHandlesEnumCallback,
        &context,
        NULL
        );
    ObDereferenceObject(processObject);
    
    /* Write the number of handles (if we have a buffer). */
    if (
        Buffer && 
        BufferLength >= sizeof(ULONG)
        )
    {
        __try
        {
            Buffer->HandleCount = context.CurrentIndex;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }
    
    /* Supply the return length if the caller wanted it. */
    if (ReturnLength)
    {
        __try
        {
            /* CurrentIndex should contain the number of handles, so we simply multiply it 
               by the size of PROCESS_HANDLE. */
            *ReturnLength = sizeof(ULONG) + context.CurrentIndex * sizeof(PROCESS_HANDLE);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }
    
    return context.Status;
}

/* KphpQueryProcessHandlesEnumCallback
 * 
 * The callback for KphEnumProcessHandleTable, used by 
 * KphQueryProcessHandles.
 */
BOOLEAN KphpQueryProcessHandlesEnumCallback(
    __inout PHANDLE_TABLE_ENTRY HandleTableEntry,
    __in HANDLE Handle,
    __in POBP_QUERY_PROCESS_HANDLES_DATA Context
    )
{
    PROCESS_HANDLE handleInfo;
    PPROCESS_HANDLE_INFORMATION buffer = Context->Buffer;
    ULONG i;
    
    handleInfo.Handle = Handle;
    handleInfo.Object = (PVOID)KVOFF(ObpDecodeObject(HandleTableEntry->Object), OffOhBody);
    handleInfo.GrantedAccess = ObpDecodeGrantedAccess(HandleTableEntry->GrantedAccess);
    handleInfo.Reserved1 = 0;
    handleInfo.HandleAttributes = ObpGetHandleAttributes(HandleTableEntry);
    handleInfo.Reserved2 = 0;
    
    if (WindowsVersion >= WINDOWS_7)
        handleInfo.ObjectTypeIndex = (USHORT)*(PUCHAR)KVOFF(KphGetObjectTypeNt(handleInfo.Object), OffOtIndex);
    else
        handleInfo.ObjectTypeIndex = (USHORT)*(PULONG)KVOFF(KphGetObjectTypeNt(handleInfo.Object), OffOtIndex);
    
    /* Increment the index regardless of whether the information will be written; 
       this will allow KphQueryProcessHandles to report the correct return length. */
    i = Context->CurrentIndex++;
    
    /* Only write if we have a buffer and have not exceeded the buffer length. */
    if (
        buffer && 
        (sizeof(ULONG) + Context->CurrentIndex * sizeof(PROCESS_HANDLE)) <= Context->BufferLength
        )
    {
        __try
        {
            buffer->Handles[i] = handleInfo;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            /* Report an error. */
            if (Context->Status == STATUS_SUCCESS)
                Context->Status = GetExceptionCode();
        }
    }
    else
    {
        /* Report that the buffer is too small. */
        if (Context->Status == STATUS_SUCCESS)
            Context->Status = STATUS_BUFFER_TOO_SMALL;
    }
    
    return FALSE;
}

/* KphSetHandleGrantedAccess
 * 
 * Sets the granted access of a handle.
 */
NTSTATUS KphSetHandleGrantedAccess(
    __in PEPROCESS Process,
    __in HANDLE Handle,
    __in ACCESS_MASK GrantedAccess
    )
{
    BOOLEAN result;
    OBP_SET_HANDLE_GRANTED_ACCESS_DATA context;
    
    context.Handle = Handle;
    context.GrantedAccess = GrantedAccess;
    
    result = KphEnumProcessHandleTable(
        Process,
        KphpSetHandleGrantedAccessEnumCallback,
        &context,
        NULL
        );
    
    return result ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

/* KphpSetHandleGrantedAccessEnumCallback
 * 
 * The callback for KphEnumProcessHandleTable, used by 
 * KphSetHandleGrantedAccess.
 */
BOOLEAN KphpSetHandleGrantedAccessEnumCallback(
    __inout PHANDLE_TABLE_ENTRY HandleTableEntry,
    __in HANDLE Handle,
    __in POBP_SET_HANDLE_GRANTED_ACCESS_DATA Context
    )
{
    if (Handle != Context->Handle)
        return FALSE;
    
    HandleTableEntry->GrantedAccess = Context->GrantedAccess;
    
    return TRUE;
}

/* ObDereferenceProcessHandleTable
 * 
 * Allows the process to terminate.
 */
VOID ObDereferenceProcessHandleTable(
    __in PEPROCESS Process
    )
{
    KphReleaseProcessRundownProtection(Process);
}

/* ObDuplicateObject
 * 
 * Duplicates a handle from the source process to the target process.
 * WARNING: This does not actually duplicate a handle. It simply 
 * re-opens an object in another process.
 */
NTSTATUS ObDuplicateObject(
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
    BOOLEAN sourceAttached = FALSE;
    BOOLEAN targetAttached = FALSE;
    KAPC_STATE apcState;
    PVOID object;
    HANDLE objectHandle;
    
    /* Validate the parameters */
    if (!TargetProcess || !TargetHandle)
    {
        if (!(Options & DUPLICATE_CLOSE_SOURCE))
            return STATUS_INVALID_PARAMETER;
    }
    
    /* Closing handles in the current process from kernel-mode is *bad* */
    /* Example: the handle being closed is a handle to the file object 
     * on which this very request is being sent. Deadlock.
     *
     * If we add the current process check, the handle can't possibly 
     * be the one the request is being sent on, since system calls 
     * only operate on handles from the current process.
     */
    if (SourceProcess == PsGetCurrentProcess())
    {
        if (Options & DUPLICATE_CLOSE_SOURCE)
            return STATUS_CANT_TERMINATE_SELF;
    }
    
    /* Check if we need to attach to the source process */
    if (SourceProcess != PsGetCurrentProcess())
    {
        KeStackAttachProcess(SourceProcess, &apcState);
        sourceAttached = TRUE;
    }
    
    /* If the caller wants us to close the source handle, do it now */
    if (Options & DUPLICATE_CLOSE_SOURCE)
    {
        status = NtClose(SourceHandle);
        if (sourceAttached)
            KeUnstackDetachProcess(&apcState);
        
        return status;
    }
    
    /* Reference the object and detach from the source process */
    status = ObReferenceObjectByHandle(
        SourceHandle,
        0,
        NULL,
        KernelMode,
        &object,
        NULL
        );
    if (sourceAttached)
        KeUnstackDetachProcess(&apcState);
    
    if (!NT_SUCCESS(status))
        return status;
    
    /* Check if we need to attach to the target process */
    if (TargetProcess != PsGetCurrentProcess())
    {
        KeStackAttachProcess(TargetProcess, &apcState);
        targetAttached = TRUE;
    }
    
    /* Open the object and detach from the target process */
    {
        POBJECT_TYPE objectType = KphGetObjectTypeNt(object);
        ACCESS_STATE accessState;
        CHAR auxData[AUX_ACCESS_DATA_SIZE];
        
        if (!objectType && AccessMode != KernelMode)
        {
            status = STATUS_INVALID_HANDLE;
            goto OpenObjectEnd;
        }
        
        status = SeCreateAccessState(
            &accessState,
            (PAUX_ACCESS_DATA)auxData,
            DesiredAccess,
            (PGENERIC_MAPPING)KVOFF(objectType, OffOtiGenericMapping)
            );
        
        if (!NT_SUCCESS(status))
            goto OpenObjectEnd;
        
        accessState.PreviouslyGrantedAccess |= 0xffffffff; /* HACK, doesn't work properly */
        accessState.RemainingDesiredAccess = 0;
        
        status = ObOpenObjectByPointer(
            object,
            HandleAttributes,
            &accessState,
            DesiredAccess,
            objectType,
            KernelMode,
            &objectHandle
            );
        SeDeleteAccessState(&accessState);
    }
    
OpenObjectEnd:
    ObDereferenceObject(object);
    
    if (targetAttached)
        KeUnstackDetachProcess(&apcState);
    
    if (NT_SUCCESS(status))
        *TargetHandle = objectHandle;
    else
        *TargetHandle = NULL;
    
    return status;
}

/* ObReferenceProcessHandleTable
 * 
 * Prevents the process from terminating and returns a pointer 
 * to its handle table.
 */
PHANDLE_TABLE ObReferenceProcessHandleTable(
    __in PEPROCESS Process
    )
{
    PHANDLE_TABLE handleTable = NULL;
    
    if (KphAcquireProcessRundownProtection(Process))
    {
        handleTable = *(PHANDLE_TABLE *)KVOFF(Process, OffEpObjectTable);
        
        if (!handleTable)
            KphReleaseProcessRundownProtection(Process);
    }
    
    return handleTable;
}
