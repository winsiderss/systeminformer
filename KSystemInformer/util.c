/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     jxy-s   2022
 *
 */

#include <kph.h>
#include <dyndata.h>

#include <trace.h>

#define KPH_MODULES_MAX_COUNT 1024

/**
 * \brief Captures the current stack, both kernel and user if possible.
 *
 * \param[out] Frames Populated with the stack frames.
 * \param[in] Count Number of pointers in the frames buffer.
 *
 * \return Number of captured frames.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
ULONG KphCaptureStack(
    _Out_ PVOID* Frames,
    _In_ ULONG Count
    )
{
    ULONG frames;

    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    frames = RtlWalkFrameChain(Frames, Count, 0);

    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
    {
        if (frames >= Count)
        {
            return frames;
        }

        frames += RtlWalkFrameChain(&Frames[frames],
                                    (Count - frames),
                                    RTL_WALK_USER_MODE_STACK);
    }

    return frames;
}

/**
 * \brief Acquires rundown. On successful return the caller should release
 * the rundown using KphReleaseRundown.
 *
 * \param[in,out] Rundown The rundown object to acquire.
 *
 * \return TRUE if rundown is acquired, FALSE if object is already ran down.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
_Must_inspect_result_
BOOLEAN KphAcquireRundown(
    _Inout_ PKPH_RUNDOWN Rundown
    )
{
    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    return ExAcquireRundownProtection(Rundown);
}

/**
 * \brief Releases rundown previously accquired by KphAcquireRundown.
 *
 * \param[in,out] Rundown The rundown object to release.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID KphReleaseRundown(
    _Inout_ PKPH_RUNDOWN Rundown
    )
{
    NT_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    ExReleaseRundownProtection(Rundown);
}


PAGED_FILE();

static UNICODE_STRING KphpLsaPortName = RTL_CONSTANT_STRING(L"\\SeLsaCommandPort");

/**
 * \brief Initializes rundown object.
 *
 * \param[out] Rundown The rundown object to initialize.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphInitializeRundown(
    _Out_ PKPH_RUNDOWN Rundown
    )
{
    PAGED_CODE();

    ExInitializeRundownProtection(Rundown);
}

/**
 * \brief Marks rundown active and waits for rundown release. Subsequent
 * attempts to acquire rundown will fail.
 *
 * \param[in,out] Rundown The rundown object to activate and wait for.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphWaitForRundown(
    _Inout_ PKPH_RUNDOWN Rundown
    )
{
    PAGED_CODE();

    ExWaitForRundownProtectionRelease(Rundown);
}

/**
 * \brief Initializes a readers-writer lock.
 *
 * \param[in] Lock The readers-writer lock to initialize.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphInitializeRWLock(
    _Out_ PKPH_RWLOCK Lock
    )
{
    PAGED_CODE();

    FltInitializePushLock(Lock);
}

/**
 * \brief Deletes a readers-writer lock.
 *
 * \param[in] Lock The readers-writer lock to delete.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphDeleteRWLock(
    _In_ PKPH_RWLOCK Lock
    )
{
    PAGED_CODE();

    FltDeletePushLock(Lock);
}

/**
 * \brief Acquires a readers-writer lock exclusive.
 *
 * \param[in,out] Lock The readers-writer lock to acquire exclusively.
 */
_IRQL_requires_max_(APC_LEVEL)
_Acquires_lock_(_Global_critical_region_)
VOID KphAcquireRWLockExclusive(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_) PKPH_RWLOCK Lock
    )
{
    PAGED_CODE();

    FltAcquirePushLockExclusive(Lock);
}

/**
 * \brief Acquires readers-writer lock shared.
 *
 * \param[in,out] Lock The readers-writer lock to acquire shared.
 */
_IRQL_requires_max_(APC_LEVEL)
_Acquires_lock_(_Global_critical_region_)
VOID KphAcquireRWLockShared(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_) PKPH_RWLOCK Lock
    )
{
    PAGED_CODE();

    FltAcquirePushLockShared(Lock);
}

/**
 * \brief Releases a readers-writer lock.
 *
 * \param[in,out] Lock The readers-writer lock to release.
 */
_IRQL_requires_max_(APC_LEVEL)
_Releases_lock_(_Global_critical_region_)
VOID KphReleaseRWLock(
    _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_) PKPH_RWLOCK Lock
    )
{
    PAGED_CODE();

    FltReleasePushLock(Lock);
}

/**
 * \brief Checks if an address range lies within a kernel module.
 *
 * \param[in] Address The beginning of the address range.
 * \param[in] Length The number of bytes in the address range.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphValidateAddressForSystemModules(
    _In_ PVOID Address,
    _In_ SIZE_T Length
    )
{
    BOOLEAN valid;
    PVOID endAddress;

    PAGED_CODE();

    if (Add2Ptr(Address, Length) < Address)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    endAddress = Add2Ptr(Address, Length);

    KeEnterCriticalRegion();
    if (!ExAcquireResourceSharedLite(PsLoadedModuleResource, TRUE))
    {
        KeLeaveCriticalRegion();

        KphTracePrint(TRACE_LEVEL_ERROR,
                      UTIL,
                      "Failed to acquire PsLoadedModuleResource");

        return STATUS_RESOURCE_NOT_OWNED;
    }

    valid = FALSE;

    for (PLIST_ENTRY link = PsLoadedModuleList->Flink;
         link != PsLoadedModuleList;
         link = link->Flink)
    {
        PKLDR_DATA_TABLE_ENTRY entry;
        PVOID endOfImage;

        entry = CONTAINING_RECORD(link, KLDR_DATA_TABLE_ENTRY, InLoadOrderLinks);

        endOfImage = Add2Ptr(entry->DllBase, entry->SizeOfImage);

        if ((Address >= entry->DllBase) && (endAddress <= endOfImage))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          UTIL,
                          "Validated address in %wZ",
                          &entry->FullDllName);
            valid = TRUE;
            break;
        }
    }

    ExReleaseResourceLite(PsLoadedModuleResource);
    KeLeaveCriticalRegion();

    return (valid ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION);
}

/**
 * \brief Queries the registry key for a string value.
 *
 * \param[in] KeyHandle Handle to key to query.
 * \param[in] ValueName Name of value to query.
 * \param[out] String Populated with the queried registry string. The string is
 * guaranteed to be null terminated and the MaximumLength will reflect this
 * when compared to the Length. This string should be freed using
 * KphFreeRegistryString.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryRegistryString(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _Outptr_allocatesMem_ PUNICODE_STRING* String
    )
{
    NTSTATUS status;
    ULONG resultLength;
    PKEY_VALUE_PARTIAL_INFORMATION info;
    PUNICODE_STRING string;
    ULONG length;

    PAGED_PASSIVE();

    *String = NULL;
    info = NULL;

    status = ZwQueryValueKey(KeyHandle,
                             ValueName,
                             KeyValuePartialInformation,
                             NULL,
                             0,
                             &resultLength);
    if ((status != STATUS_BUFFER_OVERFLOW) &&
        (status != STATUS_BUFFER_TOO_SMALL))
    {
        if (NT_SUCCESS(status))
        {
            status = STATUS_UNSUCCESSFUL;
        }

        goto Exit;
    }

    info = KphAllocatePaged(resultLength, KPH_TAG_REG_STRING);
    if (!info)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = ZwQueryValueKey(KeyHandle,
                             ValueName,
                             KeyValuePartialInformation,
                             info,
                             resultLength,
                             &resultLength);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    status = RtlULongAdd(info->DataLength, sizeof(WCHAR), &length);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    if ((info->Type != REG_SZ) ||
        (length > UNICODE_STRING_MAX_BYTES) ||
        ((info->DataLength % 2) != 0))
    {
        status = STATUS_OBJECT_TYPE_MISMATCH;
        goto Exit;
    }

    string = KphAllocatePaged((sizeof(UNICODE_STRING) + length),
                              KPH_TAG_REG_STRING);
    if (!string)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    string->Buffer = Add2Ptr(string, sizeof(UNICODE_STRING));
    RtlZeroMemory(string->Buffer, length);

    NT_ASSERT(info->DataLength < UNICODE_STRING_MAX_BYTES);
    NT_ASSERT(length < UNICODE_STRING_MAX_BYTES);

    string->Length = (USHORT)info->DataLength;
    string->MaximumLength = (USHORT)length;

    if (string->Length)
    {
        PWCHAR sz;

        sz = (PWCHAR)info->Data;

        NT_ASSERT(info->DataLength >= sizeof(WCHAR));

        if (sz[(info->DataLength / sizeof(WCHAR)) - 1] == L'\0')
        {
            string->Length -= sizeof(WCHAR);
        }

        RtlCopyMemory(string->Buffer, info->Data, string->Length);
    }

    *String = string;
    status = STATUS_SUCCESS;

Exit:

    if (info)
    {
        KphFree(info, KPH_TAG_REG_STRING);
    }

    return status;
}

/**
 * \brief Frees a string retrieved by KphQueryRegistryString.
 *
 * \param[in] String The string to free.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphFreeRegistryString(
    _In_freesMem_ PUNICODE_STRING String
    )
{
    PAGED_CODE();

    KphFree(String, KPH_TAG_REG_STRING);
}

/**
 * \brief Queries the registry key for a string value.
 *
 * \param[in] KeyHandle Handle to key to query.
 * \param[in] ValueName Name of value to query.
 * \param[out] Buffer Registry binary buffer on success, should be freed using
 * KphFreeRegistryBinary.
 * \param[out] Length Set to the length of the buffer.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryRegistryBinary(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _Outptr_allocatesMem_ PBYTE* Buffer,
    _Out_ PULONG Length
    )
{
    NTSTATUS status;
    PBYTE buffer;
    ULONG resultLength;
    PKEY_VALUE_PARTIAL_INFORMATION info;

    PAGED_PASSIVE();

    *Buffer = NULL;
    *Length = 0;
    buffer = NULL;

    status = ZwQueryValueKey(KeyHandle,
                             ValueName,
                             KeyValuePartialInformation,
                             NULL,
                             0,
                             &resultLength);
    if ((status != STATUS_BUFFER_OVERFLOW) &&
        (status != STATUS_BUFFER_TOO_SMALL))
    {
        if (NT_SUCCESS(status))
        {
            status = STATUS_UNSUCCESSFUL;
        }

        goto Exit;
    }

    buffer = KphAllocatePaged(resultLength, KPH_TAG_DYNDATA);
    if (!buffer)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = ZwQueryValueKey(KeyHandle,
                             ValueName,
                             KeyValuePartialInformation,
                             buffer,
                             resultLength,
                             &resultLength);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
    }

    info = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;

    if (info->Type != REG_BINARY)
    {
        status = STATUS_OBJECT_TYPE_MISMATCH;
        goto Exit;
    }

    *Length = info->DataLength;
    *Buffer = info->Data;
    buffer = NULL;
    status = STATUS_SUCCESS;

Exit:

    if (buffer)
    {
        KphFree(buffer, KPH_TAG_REG_BINARY);
    }

    return status;
}

/**
 * \brief Frees a buffer retrieved by KphQueryRegistryBinary.
 *
 * \param[in] String String to free.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphFreeRegistryBinary(
    _In_freesMem_ PBYTE Buffer
    )
{
    PAGED_CODE();

    KphFree(CONTAINING_RECORD(Buffer, KEY_VALUE_PARTIAL_INFORMATION, Data),
            KPH_TAG_REG_BINARY);
}

/**
 * \brief Queries the registry key for a string value.
 *
 * \param[in] KeyHandle Handle to key to query.
 * \param[in] ValueName Name of value to query.
 * \param[out] Value Registry unsigned long.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryRegistryULong(
    _In_ HANDLE KeyHandle,
    _In_ PUNICODE_STRING ValueName,
    _Out_ PULONG Value
    )
{
    NTSTATUS status;
    BYTE buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONGLONG)];
    ULONG resultLength;
    PKEY_VALUE_PARTIAL_INFORMATION info;

    PAGED_PASSIVE();

    *Value = 0;

    status = ZwQueryValueKey(KeyHandle,
                             ValueName,
                             KeyValuePartialInformation,
                             buffer,
                             ARRAYSIZE(buffer),
                             &resultLength);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    info = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;

    if (info->Type != REG_DWORD)
    {
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    if (info->DataLength != sizeof(ULONG))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    *Value = *(PULONG)info->Data;

    return STATUS_SUCCESS;
}

/**
 * \brief Maps view into the system address space. The caller should unmap the
 * view by calling KphUnmapViewInSystem.
 *
 * \param[in] FileHandle Handle to file to map.
 * \param[in] Flags Options for the mapping (see: KPH_MAP..).
 * \param[out] MappedBase Base address of the mapping.
 * \param[out] ViewSize Size of the mapped view. If not an image mapping and
 * if the view exceeds the file size, it is clamped to the file size.
 *
 * \return Successful or errant status.
 */
_IRQL_always_function_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphMapViewInSystem(
    _In_ HANDLE FileHandle,
    _In_ ULONG Flags,
    _Outptr_result_bytebuffer_(*ViewSize) PVOID *MappedBase,
    _Inout_ PSIZE_T ViewSize
    )
{
    NTSTATUS status;
    HANDLE sectionHandle;
    PVOID sectionObject;
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG allocationAttributes;
    SIZE_T fileSize;
    LARGE_INTEGER sectionOffset;

    PAGED_PASSIVE();

    sectionHandle = NULL;
    sectionObject = NULL;
    *MappedBase = NULL;

    if (BooleanFlagOn(Flags, KPH_MAP_IMAGE))
    {
        allocationAttributes = SEC_IMAGE_NO_EXECUTE;
        fileSize = SIZE_T_MAX;
    }
    else
    {
        IO_STATUS_BLOCK ioStatusBlock;
        FILE_STANDARD_INFORMATION fileInfo;

        allocationAttributes = SEC_COMMIT;

        status = ZwQueryInformationFile(FileHandle,
                                        &ioStatusBlock,
                                        &fileInfo,
                                        sizeof(fileInfo),
                                        FileStandardInformation);
        if (!NT_SUCCESS(status))
        {
            goto Exit;
        }

        if (fileInfo.EndOfFile.QuadPart < 0)
        {
            status = STATUS_FILE_TOO_LARGE;
            goto Exit;
        }

#ifdef _WIN64
        fileSize = (SIZE_T)fileInfo.EndOfFile.QuadPart;
#else
        if (fileInfo.EndOfFile.HighPart != 0)
        {
            status = STATUS_FILE_TOO_LARGE;
            goto Exit;
        }
        fileSize = fileInfo.EndOfFile.LowPart;
#endif
    }

    InitializeObjectAttributes(&objectAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    status = ZwCreateSection(&sectionHandle,
                             SECTION_MAP_READ,
                             &objectAttributes,
                             NULL,
                             PAGE_READONLY,
                             allocationAttributes,
                             FileHandle);
    if (!NT_SUCCESS(status))
    {
        sectionHandle = NULL;
        goto Exit;
    }

    status = ObReferenceObjectByHandle(sectionHandle,
                                       SECTION_MAP_READ,
                                       *MmSectionObjectType,
                                       KernelMode,
                                       &sectionObject,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        sectionObject = NULL;
        goto Exit;
    }

    sectionOffset.QuadPart = 0;
    status = MmMapViewInSystemSpaceEx(sectionObject,
                                      MappedBase,
                                      ViewSize,
                                      &sectionOffset,
                                      MM_SYSTEM_VIEW_EXCEPTIONS_FOR_INPAGE_ERRORS);
    if (!NT_SUCCESS(status))
    {
        *MappedBase = NULL;
        *ViewSize = 0;
        goto Exit;
    }

    if (*ViewSize > fileSize)
    {
        *ViewSize = fileSize;
    }

    status = STATUS_SUCCESS;

Exit:

    if (sectionObject)
    {
        ObDereferenceObject(sectionObject);
    }

    if (sectionHandle)
    {
        ObCloseHandle(sectionHandle, KernelMode);
    }

    return status;
}

/**
 * \brief Unmaps view from the system process address space.
 *
 * \param[in] MappedBase Base address of the mapping.
 */
_IRQL_always_function_max_(PASSIVE_LEVEL)
VOID KphUnmapViewInSystem(
    _In_ PVOID MappedBase
    )
{
    PAGED_PASSIVE();

    MmUnmapViewInSystemSpace(MappedBase);
}

/**
 * \brief Gets the file name from a file object.
 *
 * \param[in] FileObject The file object to get the name from.
 * \param[out] FileName Set to a point to the file name on success, caller
 * should free this using KphFreeFileNameObject.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetNameFileObject(
    _In_ PFILE_OBJECT FileObject,
    _Outptr_allocatesMem_ PUNICODE_STRING* FileName
    )
{
    NTSTATUS status;
    ULONG returnLength;
    POBJECT_NAME_INFORMATION nameInfo;

    PAGED_PASSIVE();

    *FileName = NULL;

    returnLength = (sizeof(OBJECT_NAME_INFORMATION) + MAX_PATH);
    nameInfo = KphAllocatePaged(returnLength, KPH_TAG_FILE_OBJECT_NAME);
    if (!nameInfo)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      UTIL,
                      "Failed to allocate for file object name.");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    status = KphQueryNameFileObject(FileObject,
                                    nameInfo,
                                    returnLength,
                                    &returnLength);
    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        KphFree(nameInfo, KPH_TAG_FILE_OBJECT_NAME);
        nameInfo = KphAllocatePaged(returnLength, KPH_TAG_FILE_OBJECT_NAME);
        if (!nameInfo)
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          UTIL,
                          "Failed to allocate for file object name.");

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        status = KphQueryNameFileObject(FileObject,
                                        nameInfo,
                                        returnLength,
                                        &returnLength);
    }

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      UTIL,
                      "KphQueryNameFileObject failed: %!STATUS!",
                      status);

        goto Exit;
    }

    *FileName = &nameInfo->Name;
    nameInfo = NULL;
    status = STATUS_SUCCESS;

Exit:

    if (nameInfo)
    {
        KphFree(nameInfo, KPH_TAG_FILE_OBJECT_NAME);
    }

    return status;
}

/**
 * \brief Frees a file name previously retrieved by KphGetFileNameObject.
 *
 * \param[out] FileName The file name to free.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphFreeNameFileObject(
    _In_freesMem_ PUNICODE_STRING FileName
    )
{
    PAGED_CODE();

    KphFree(CONTAINING_RECORD(FileName, OBJECT_NAME_INFORMATION, Name),
            KPH_TAG_FILE_OBJECT_NAME);
}

/**
 * \brief Preforms a single privilege check on the supplied subject context.
 *
 * \param[in] PrivilegeValue The privilege value to check.
 * \param[in] SubjectSecurityContext The subject context to check.
 * \param[in] AccessMode The access mode used for the access check.
 *
 * \return TRUE if the subject has the desired privilege, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KphSinglePrivilegeCheckEx(
    _In_ LUID PrivilegeValue,
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    PRIVILEGE_SET requiredPrivileges;

    PAGED_PASSIVE();

    requiredPrivileges.PrivilegeCount = 1;
    requiredPrivileges.Control = PRIVILEGE_SET_ALL_NECESSARY;
    requiredPrivileges.Privilege[0].Luid = PrivilegeValue;
    requiredPrivileges.Privilege[0].Attributes = 0;

    return SePrivilegeCheck(&requiredPrivileges,
                            SubjectSecurityContext,
                            AccessMode);
}

/**
 * \brief Preforms a single privilege check on the current subject context.
 *
 * \param[in] PrivilegeValue The privilege value to check.
 * \param[in] AccessMode The access mode used for the access check.
 *
 * \return TRUE if the subject has the desired privilege, FALSE otherwise.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KphSinglePrivilegeCheck(
    _In_ LUID PrivilegeValue,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    BOOLEAN accessGranted;
    SECURITY_SUBJECT_CONTEXT subjectContext;

    PAGED_PASSIVE();

    SeCaptureSubjectContext(&subjectContext);

    accessGranted = KphSinglePrivilegeCheckEx(PrivilegeValue,
                                              &subjectContext,
                                              AccessMode);

    SeReleaseSubjectContext(&subjectContext);

    return accessGranted;
}

/**
 * \brief Retrieves the process ID of lsass.
 *
 * \param[out] ProcessId Set to the process ID of lsass.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS KphpGetLsassProcessId(
    _Out_ PHANDLE ProcessId
    )
{
    NTSTATUS status;
    HANDLE portHandle;
    KAPC_STATE apcState;
    KPH_ALPC_COMMUNICATION_INFORMATION info;

    PAGED_PASSIVE();

    *ProcessId = NULL;

    //
    // Attach to system to ensure we get a kernel handle from the following
    // ZwAlpcConnectPort call. Opening the handle here does not ask for the
    // object attributes. To keep our imports simple we choose to use this
    // over the Ex version (might change in the future). Pattern is adopted
    // from msrpc.sys.
    //

    KeStackAttachProcess(PsInitialSystemProcess, &apcState);

    status = ZwAlpcConnectPort(&portHandle,
                               &KphpLsaPortName,
                               NULL,
                               NULL,
                               0,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      UTIL,
                      "ZwAlpcConnectPort failed: %!STATUS!",
                      status);

        portHandle = NULL;
        goto Exit;
    }

    status = KphAlpcQueryInformation(ZwCurrentProcess(),
                                     portHandle,
                                     KphAlpcCommunicationInformation,
                                     &info,
                                     sizeof(info),
                                     NULL,
                                     KernelMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      UTIL,
                      "ZwAlpcQueryInformation failed: %!STATUS!",
                      status);
        goto Exit;
    }

    *ProcessId = info.ConnectionPort.OwnerProcessId;

Exit:

    if (portHandle)
    {
        ObCloseHandle(portHandle, KernelMode);
    }

    KeUnstackDetachProcess(&apcState);

    return status;
}

/**
 * \brief Checks if a given process is lsass.
 *
 * \param[in] Process The process to check.
 *
 * \return TRUE if the process is lsass.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN KphProcessIsLsass(
    _In_ PEPROCESS Process
    )
{
    NTSTATUS status;
    HANDLE processId;
    SECURITY_SUBJECT_CONTEXT subjectContext;
    BOOLEAN result;

    PAGED_PASSIVE();

    status = KphpGetLsassProcessId(&processId);
    if (!NT_SUCCESS(status))
    {
        return FALSE;
    }

    if (processId != PsGetProcessId(Process))
    {
        return FALSE;
    }

    SeCaptureSubjectContextEx(NULL, Process, &subjectContext);

    result = KphSinglePrivilegeCheckEx(SeExports->SeCreateTokenPrivilege,
                                       &subjectContext,
                                       UserMode);

    SeReleaseSubjectContext(&subjectContext);

    return result;
}

/**
 * \brief Retrieves the kernel file name.
 *
 * \param[out] FileName On success set to the file name for the kernel. Must be
 * freed using RtlFreeUnicodeString.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphpGetKernelFileName(
    _Out_ _At_(FileName->Buffer, __drv_allocatesMem(Mem)) 
    PUNICODE_STRING FileName
    )
{
    NTSTATUS status;
    SYSTEM_SINGLE_MODULE_INFORMATION info;
    ANSI_STRING fullPathName;

    PAGED_PASSIVE();

    RtlZeroMemory(FileName, sizeof(UNICODE_STRING));

    RtlZeroMemory(&info, sizeof(info));

    info.TargetModuleAddress = KphGetSystemRoutineAddress(L"ObCloseHandle");
    if (!info.TargetModuleAddress)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      UTIL,
                      L"KphGetSystemRoutineAddress failed");

        return STATUS_NOT_FOUND;
    }

    status = ZwQuerySystemInformation(SystemSingleModuleInformation,
                                      &info,
                                      sizeof(info),
                                      NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      UTIL,
                      L"ZwQuerySystemInformation failed: %!STATUS!",
                      status);

        return status;
    }

    status = RtlInitAnsiStringEx(&fullPathName,
                                 (PCSZ)info.ExInfo.BaseInfo.FullPathName);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return RtlAnsiStringToUnicodeString(FileName, &fullPathName, TRUE);
}

/**
 * \brief Retrieves the version of the kernel.
 *
 * \param[out] Version Set to the kernel build version.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetKernelVersion(
    _Out_ PKPH_FILE_VERSION Version 
    )
{
    NTSTATUS status;
    UNICODE_STRING kernelFileName;

    PAGED_PASSIVE();

    status = KphpGetKernelFileName(&kernelFileName);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      UTIL,
                      "KphGetKernelFileName failed: %!STATUS!",
                      status);

        RtlZeroMemory(Version, sizeof(KPH_FILE_VERSION));
        goto Exit;
    }

    KphTracePrint(TRACE_LEVEL_INFORMATION,
                  UTIL,
                  "Kernel file name: \"%wZ\"",
                  &kernelFileName);

    status = KphGetFileVersion(&kernelFileName, Version);

Exit:

    RtlFreeUnicodeString(&kernelFileName);

    return status;
}

/**
 * \brief Retrieves the file version from a file.
 *
 * \param[in] FileName The name of the file to get the version from.
 * \param[out] Version Set to the file version.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetFileVersion(
    _In_ PUNICODE_STRING FileName,
    _Out_ PKPH_FILE_VERSION Version
    )
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE fileHandle;
    IO_STATUS_BLOCK ioStatusBlock;
    PVOID imageBase;
    SIZE_T imageSize;
    PVOID imageEnd;
    LDR_RESOURCE_INFO resourceInfo;
    PIMAGE_RESOURCE_DATA_ENTRY resourceData;
    PVOID resourceBuffer;
    ULONG resourceLength;
    PVS_VERSION_INFO_STRUCT versionInfo;
    UNICODE_STRING keyName;
    PVS_FIXEDFILEINFO fileInfo;

    PAGED_PASSIVE();

    RtlZeroMemory(Version, sizeof(KPH_FILE_VERSION));

    imageBase = NULL;
    fileHandle = NULL;

    InitializeObjectAttributes(&objectAttributes,
                               FileName,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    status = KphCreateFile(&fileHandle,
                           FILE_READ_ACCESS | SYNCHRONIZE,
                           &objectAttributes,
                           &ioStatusBlock,
                           NULL,
                           FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN,
                           FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                           NULL,
                           0,
                           IO_IGNORE_SHARE_ACCESS_CHECK,
                           KernelMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      UTIL,
                      "KphCreateFile failed: %!STATUS!",
                      status);

        fileHandle = NULL;
        goto Exit;
    }

    imageSize = 0;
    status = KphMapViewInSystem(fileHandle,
                                KPH_MAP_IMAGE,
                                &imageBase,
                                &imageSize);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      UTIL,
                      "KphMapViewInSystem failed: %!STATUS!",
                      status);

        imageBase = NULL;
        goto Exit;
    }

    imageEnd = Add2Ptr(imageBase, imageSize);

    resourceInfo.Type = (ULONG_PTR)VS_FILE_INFO;
    resourceInfo.Name = (ULONG_PTR)MAKEINTRESOURCEW(VS_VERSION_INFO);
    resourceInfo.Language = MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL);

    __try
    {
        status = LdrFindResource_U(imageBase,
                                   &resourceInfo,
                                   RESOURCE_DATA_LEVEL,
                                   &resourceData);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          UTIL,
                          "LdrFindResource_U failed: %!STATUS!",
                          status);

            goto Exit;
        }

        status = LdrAccessResource(imageBase,
                                   resourceData,
                                   &resourceBuffer,
                                   &resourceLength);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          UTIL,
                          "LdrAccessResource failed: %!STATUS!",
                          status);

            goto Exit;
        }

        if (Add2Ptr(resourceBuffer, resourceLength) >= imageEnd)
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          UTIL,
                          "Resource buffer overflows mapping");

            status = STATUS_BUFFER_OVERFLOW;
            goto Exit;
        }

        if (resourceLength < sizeof(VS_VERSION_INFO_STRUCT))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          UTIL,
                          "Resource length insufficient");

            status = STATUS_BUFFER_TOO_SMALL;
            goto Exit;
        }

        versionInfo = resourceBuffer;

        if (Add2Ptr(resourceBuffer, versionInfo->Length) >= imageEnd)
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          UTIL,
                          "Version info overflows mapping");

            status = STATUS_BUFFER_OVERFLOW;
            goto Exit;
        }

        if (versionInfo->ValueLength < sizeof(VS_FIXEDFILEINFO))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          UTIL,
                          "Value length insufficient");

            status = STATUS_BUFFER_TOO_SMALL;
            goto Exit;
        }

        status = RtlInitUnicodeStringEx(&keyName, versionInfo->Key);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          UTIL,
                          "RtlInitUnicodeStringEx failed: %!STATUS!",
                          status);

            goto Exit;
        }

        fileInfo = Add2Ptr(versionInfo, RTL_SIZEOF_THROUGH_FIELD(VS_VERSION_INFO_STRUCT, Type));
        fileInfo = (PVS_FIXEDFILEINFO)ALIGN_UP(Add2Ptr(fileInfo, keyName.MaximumLength), ULONG);

        if (Add2Ptr(fileInfo, sizeof(VS_FIXEDFILEINFO)) >= imageEnd)
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          UTIL,
                          "File version info overflows mapping");

            status = STATUS_BUFFER_TOO_SMALL;
            goto Exit;
        }

        if (fileInfo->dwSignature != VS_FFI_SIGNATURE)
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          UTIL,
                          "Invalid file version information signature (0x%08x)",
                          fileInfo->dwSignature);

            status = STATUS_INVALID_SIGNATURE;
            goto Exit;
        }

        if (fileInfo->dwStrucVersion != 0x10000)
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          UTIL,
                          "Unknown file version information structure (0x%08x)",
                          fileInfo->dwStrucVersion);

            status = STATUS_REVISION_MISMATCH;
            goto Exit;
        }

        Version->MajorVersion = HIWORD(fileInfo->dwFileVersionMS);
        Version->MinorVersion = LOWORD(fileInfo->dwFileVersionMS);
        Version->BuildNumber = HIWORD(fileInfo->dwFileVersionLS);
        Version->Revision = LOWORD(fileInfo->dwFileVersionLS);
        status = STATUS_SUCCESS;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }

Exit:

    if (imageBase)
    {
        KphUnmapViewInSystem(imageBase);
    }

    if (fileHandle)
    {
        ObCloseHandle(fileHandle, KernelMode);
    }

    return status;
}

/**
 * \brief Grants a call target in a process permission to be called by setting
 * CFG flags for that page.
 *
 * \details The process must already have the relevant mitigations enabled for
 * the CFG flags, else this call will fail. See RtlpGuardGrantSuppressedCallAccess.
 * 
 * \param ProcessHandle Handle to the process to configure CFG for.
 * \param VirtualAddress Virtual address of the call target.
 * \param Flags CFG flags to configure.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGuardGrantSuppressedCallAccess(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID VirtualAddress,
    _In_ ULONG Flags
    )
{
    MEMORY_RANGE_ENTRY memoryRange;
    CFG_CALL_TARGET_INFO targetInfo;
    CFG_CALL_TARGET_LIST_INFORMATION targetListInfo;
    ULONG numberOfEntriesProcessed;

    PAGED_PASSIVE();

    memoryRange.VirtualAddress = (PVOID)((ULONG_PTR)VirtualAddress & ~(PAGE_SIZE - 1));
    memoryRange.NumberOfBytes = PAGE_SIZE;

    targetInfo.Offset = ((ULONG_PTR)VirtualAddress & (PAGE_SIZE - 1));
    targetInfo.Flags = Flags;

    numberOfEntriesProcessed = 0;

    RtlZeroMemory(&targetListInfo, sizeof(targetListInfo));
    targetListInfo.NumberOfEntries = 1;
    targetListInfo.NumberOfEntriesProcessed = &numberOfEntriesProcessed;
    targetListInfo.CallTargetInfo = &targetInfo;

    return ZwSetInformationVirtualMemory(ProcessHandle,
                                         VmCfgCallTargetInformation,
                                         1,
                                         &memoryRange,
                                         &targetListInfo,
                                         sizeof(targetListInfo));
}

/**
 * \brief Gets the final component of a file name.
 *
 * \details The final component is the last part of the file name, e.g. for
 * "\SystemRoot\System32\ntoskrnl.exe" it would be "ntoskrnl.exe". Returns an
 * error if the final component is not found.
 *
 * \param[in] FileName File name to get the final component of.
 * \param[out] FinalComponent Final component of the file name, references
 * input string, this string should not be freed and the input string must
 * remain valid as long as this is in use.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetFileNameFinalComponent(
    _In_ PUNICODE_STRING FileName,
    _Out_ PUNICODE_STRING FinalComponent
    )
{
    PAGED_CODE();

    for (USHORT i = (FileName->Length / sizeof(WCHAR)); i > 0; i--)
    {
        if (FileName->Buffer[i - 1] != L'\\')
        {
            continue;
        }

        FinalComponent->Buffer = &FileName->Buffer[i];
        FinalComponent->Length = FileName->Length - (i * sizeof(WCHAR));
        FinalComponent->MaximumLength = FinalComponent->Length;

        return STATUS_SUCCESS;
    }

    RtlZeroMemory(FinalComponent, sizeof(*FinalComponent));

    return STATUS_NOT_FOUND;
}

/**
 * \brief Gets the image name from a process.
 *
 * \param[in] Process The process to get the image name from.
 * \param[out] ImageName Populated with the image name of the process, this 
 * string should be freed using KphFreeProcessImageName.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetProcessImageName(
    _In_ PEPROCESS Process,
    _Out_allocatesMem_ PUNICODE_STRING ImageName
    )
{
    NTSTATUS status;
    PUCHAR fileName;
    SIZE_T len;

    PAGED_PASSIVE();

    fileName = PsGetProcessImageFileName(Process);

    status = RtlStringCbLengthA((STRSAFE_PCNZCH)fileName, 15, &len);
    if (NT_SUCCESS(status))
    {
        ANSI_STRING string;

        string.Buffer = (PCHAR)fileName;
        string.Length = (USHORT)len;
        string.MaximumLength = string.Length;

        status = RtlAnsiStringToUnicodeString(ImageName, &string, TRUE);
        if (NT_SUCCESS(status))
        {
            return status;
        }
    }

    RtlZeroMemory(ImageName, sizeof(*ImageName));

    return status;
}

/**
 * \brief Frees a process image file name from KphGetProcessImageName.
 *
 * \param[in] ImageName The image name to free.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphFreeProcessImageName(
    _In_freesMem_ PUNICODE_STRING ImageName
    )
{
    PAGED_CODE();

    RtlFreeUnicodeString(ImageName);
}
