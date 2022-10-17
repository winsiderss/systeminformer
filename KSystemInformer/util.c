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
 * \brief Retrieves the modules loaded by the kernel.
 *
 * \param[out] Modules A variable which receives a pointer to a structure
 * containing information about the kernel modules. The structure must be
 * freed with KphFreeSystemModules.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetSystemModules(
    _Outptr_allocatesMem_ PRTL_PROCESS_MODULES *Modules
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts;

    PAGED_CODE();

    *Modules = NULL;
    bufferSize = 2048;
    attempts = 8;

    do
    {
        buffer = KphAllocatePaged(bufferSize, KPH_TAG_MODULES);

        if (!buffer)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        status = ZwQuerySystemInformation(SystemModuleInformation,
                                          buffer,
                                          bufferSize,
                                          &bufferSize);
        if (NT_SUCCESS(status))
        {
            *Modules = buffer;

            return status;
        }

        KphFree(buffer, KPH_TAG_MODULES);

        if (status != STATUS_INFO_LENGTH_MISMATCH)
        {
            break;
        }
    } while (--attempts);

    return status;
}

/**
 * \brief Frees modules previously retrieved by KphEnumerateSystemModules.
 *
 * \param[in] Modules The modules to free.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphFreeSystemModules(
    _In_freesMem_ PRTL_PROCESS_MODULES Modules
    )
{
    PAGED_CODE();

    KphFree(Modules, KPH_TAG_MODULES);
}

/**
 * \brief Checks if an address range lies within a kernel module using the
 * system module list. Caller must guarantee that PsLoadedModuleList and
 * PsLoadedModuleResource is available prior to this call.
 *
 * \param[in] Address The beginning of the address range.
 * \param[in] Length The number of bytes in the address range.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphpValidateAddressForSystemModulesByModuleList(
    _In_ PVOID Address,
    _In_ SIZE_T Length
    )
{
    BOOLEAN valid;
    PVOID endAddress;

    PAGED_CODE();

    NT_ASSERT(KphDynPsLoadedModuleList && KphDynPsLoadedModuleResource);

    if (Add2Ptr(Address, Length) < Address)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    endAddress = Add2Ptr(Address, Length);

    if (!ExAcquireResourceSharedLite(KphDynPsLoadedModuleResource, TRUE))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      UTIL,
                      "Failed to acquire PsLoadedModuleResource");

        return STATUS_RESOURCE_NOT_OWNED;
    }

    valid = FALSE;

    for (PLIST_ENTRY link = KphDynPsLoadedModuleList->Flink;
         link != KphDynPsLoadedModuleList;
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

    ExReleaseResourceLite(KphDynPsLoadedModuleResource);

    return (valid ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION);
}

/**
 * \brief Checks if an address range lies within a kernel module by querying
 * the system module list.
 *
 * \param[in] Address The beginning of the address range.
 * \param[in] Length The number of bytes in the address range.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphpValidateAddressForSystemModulesBySystemQuery(
    _In_ PVOID Address,
    _In_ SIZE_T Length
    )
{
    NTSTATUS status;
    PRTL_PROCESS_MODULES modules;
    ULONG i;
    BOOLEAN valid;
    PVOID endAddress;

    PAGED_CODE();

    if (Add2Ptr(Address, Length) < Address)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    endAddress = Add2Ptr(Address, Length);

    status = KphGetSystemModules(&modules);

    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      UTIL,
                      "KphGetSystemModules failed: %!STATUS!",
                      status);

        return status;
    }

    valid = FALSE;

    for (i = 0; i < modules->NumberOfModules; i++)
    {
        PVOID endOfImage;

        endOfImage = Add2Ptr(modules->Modules[i].ImageBase, modules->Modules[i].ImageSize);

        if ((Address >= modules->Modules[i].ImageBase) && (endAddress <= endOfImage))
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          UTIL,
                          "Validated address in %s",
                          (PCSTR)modules->Modules[i].FullPathName);
            valid = TRUE;
            break;
        }
    }

    KphFreeSystemModules(modules);

    return (valid ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION);
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
    PAGED_CODE();

    if (KphDynPsLoadedModuleList && KphDynPsLoadedModuleResource)
    {
        return KphpValidateAddressForSystemModulesByModuleList(Address, Length);
    }

    return KphpValidateAddressForSystemModulesBySystemQuery(Address, Length);
}

/**
 * \brief Gets the file name of a mapped section.
 *
 * \param[in] ProcessHandle A handle to a process. The handle must have
 * PROCESS_QUERY_INFORMATION access.
 * \param[in] BaseAddress The base address of the section view.
 * \param[out] Modules A variable which receives a pointer to a string
 * containing the file name of the section. The structure must be freed with
 * KphFreeProcessMappedFileName.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetProcessMappedFileName(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Outptr_allocatesMem_ PUNICODE_STRING* FileName
    )
{
    NTSTATUS status;
    PVOID buffer;
    SIZE_T bufferSize;
    SIZE_T returnLength;

    PAGED_PASSIVE();

    *FileName = NULL;

    bufferSize = 0x100;
    buffer = KphAllocatePaged(bufferSize, KPH_TAG_FILE_NAME);

    if (!buffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ZwQueryVirtualMemory(ProcessHandle,
                                  BaseAddress,
                                  MemoryMappedFilenameInformation,
                                  buffer,
                                  bufferSize,
                                  &returnLength);

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        KphFree(buffer, KPH_TAG_FILE_NAME);
        bufferSize = returnLength;
        buffer = KphAllocatePaged(bufferSize, KPH_TAG_FILE_NAME);

        if (!buffer)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = ZwQueryVirtualMemory(ProcessHandle,
                                      BaseAddress,
                                      MemoryMappedFilenameInformation,
                                      buffer,
                                      bufferSize,
                                      &returnLength);
    }

    if (!NT_SUCCESS(status))
    {
        KphFree(buffer, KPH_TAG_FILE_NAME);
        return status;
    }

    *FileName = buffer;

    return status;
}

/**
 * \brief Frees a mapped file name retrieved from KphGetProcessMappedFileName.
 *
 * \param[in] FileName File name to free.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphFreeProcessMappedFileName(
    _In_freesMem_ PUNICODE_STRING FileName
    )
{
    PAGED_CODE();

    KphFree(FileName, KPH_TAG_FILE_NAME);
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
    _Outptr_allocatesMem_ PUCHAR* Buffer,
    _Out_ PULONG Length
    )
{
    NTSTATUS status;
    PUCHAR buffer;
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
    _In_freesMem_ PUCHAR Buffer
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
    UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONGLONG)];
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
 * \brief Retrieves the image headers from a module base address.
 *
 * \param[in] Base - Module base address.
 * \param[in] Size - Size of the address range to parse.
 * \param[out] OutHeaders - On success points to the image headers.
 *
 * \return Appropriate status.
*/
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS
KphImageNtHeader(
    _In_ PVOID Base,
    _In_ ULONG64 Size,
    _Out_ PIMAGE_NT_HEADERS* OutHeaders
    )
{
    PAGED_CODE();

    if (KphDynRtlImageNtHeaderEx)
    {
        return KphDynRtlImageNtHeaderEx(0, Base, Size, OutHeaders);
    }

    *OutHeaders = RtlImageNtHeader(Base);
    return (*OutHeaders ? STATUS_SUCCESS : STATUS_INVALID_IMAGE_FORMAT);
}

/**
 * \brief Adds an offset to a pointer.
 *
 * \param[in,out] Pointer Pointer to offset.
 * \param[in] Offer Offset to apply to the pointer.
 *
 * \return Successful or errant status if an overflow occurs.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphPtrAddOffset(
    _Inout_ PVOID* Pointer,
    _In_ SIZE_T Offset
    )
{
    PVOID pointer;

    PAGED_CODE();

    pointer = Add2Ptr(*Pointer, Offset);

    if (pointer < *Pointer)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    *Pointer = pointer;

    return STATUS_SUCCESS;
}

/**
 * \brief Advances a pointer. If the pointer advancement would advance at or
 * beyond the end pointer the function fails.
 *
 * \param[in,out] Pointer Pointer to advance.
 * \param[in] EndPointer End of buffer.
 * \param[in] Offset Offset to advance by.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphAdvancePointer(
    _Inout_ PVOID* Pointer,
    _In_ PVOID EndPointer,
    _In_ SIZE_T Offset
    )
{
    NTSTATUS status;
    PVOID pointer;

    PAGED_CODE();

    pointer = *Pointer;

    status = KphPtrAddOffset(&pointer, Offset);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (pointer >= EndPointer)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    *Pointer = pointer;

    return STATUS_SUCCESS;
}

/**
 * \brief Advances a buffer. If the advancement would advance at or passed the
 * remaining size of the buffer the function fails.
 *
 * \param[in,out] Pointer Pointer to advance.
 * \param[in,out] Size Remaining size of the buffer.
 * \param[in] Offset Offset to advance by.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphAdvanceBuffer(
    _Inout_ PVOID* Pointer,
    _Inout_ PSIZE_T Size,
    _In_ SIZE_T Offset
    )
{
    NTSTATUS status;
    PVOID pointer;
    SIZE_T size;

    PAGED_CODE();

    pointer = *Pointer;

    status = KphAdvancePointer(&pointer, Add2Ptr(*Pointer, *Size), Offset);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = RtlULongPtrSub(*Size, Offset, &size);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    *Pointer = pointer;
    *Size = size;

    return STATUS_SUCCESS;
}

/**
 * \brief Resolves a relative virtual address to a section.
 *
 * \param[in] SectionHeaders Section headers to look for the relative virtual
 * address in.
 * \param[in] NumberOfSection Number of section headers.
 * \param[in] Rva Relative virtual address to look for.
 * \param[out] Section Set to the section the relative virtual address is in
 * on success, null on failure.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphMappedImageRvaToSection(
    _In_ PIMAGE_SECTION_HEADER SectionHeaders,
    _In_ ULONG NumberOfSections,
    _In_ ULONG Rva,
    _Out_ PIMAGE_SECTION_HEADER* Section
    )
{
    NTSTATUS status;

    PAGED_CODE();
    NT_ASSERT((PVOID)SectionHeaders > MmHighestUserAddress);

    *Section = NULL;

    for (ULONG i = 0; i < NumberOfSections; i++)
    {
        ULONG end;

        status = RtlULongAdd(SectionHeaders[i].VirtualAddress,
                             SectionHeaders[i].SizeOfRawData,
                             &end);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        if ((Rva >= SectionHeaders[i].VirtualAddress) && (Rva < end))
        {
            *Section = &SectionHeaders[i];
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}

/**
 * \brief Converts a relative virtual address to a virtual address in the mapping.
 *
 * \param[in] MappedBase Base address of the image mapping.
 * \param[in] ViewSize Size of section mapping.
 * \param[in] SectionHeaders Image section headers.
 * \param[in] NumberOfSections Number of image section headers.
 * \param[in] Rva Relative virtual address to look for.
 * \param[out] Va Virtual address in the mapping for the relative one.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphMappedImageRvaToVa(
    _In_ PVOID MappedBase,
    _In_ SIZE_T ViewSize,
    _In_ PIMAGE_SECTION_HEADER SectionHeaders,
    _In_ ULONG NumberOfSections,
    _In_ ULONG Rva,
    _Out_ PVOID* Va
    )
{
    NTSTATUS status;
    PIMAGE_SECTION_HEADER section;
    PVOID va;
    ULONG offset;
    SIZE_T viewSize;

    PAGED_CODE();
    NT_ASSERT((PVOID)MappedBase > MmHighestUserAddress);

    *Va = NULL;

    status = KphMappedImageRvaToSection(SectionHeaders,
                                        NumberOfSections,
                                        Rva,
                                        &section);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = RtlULongSub(Rva, section->VirtualAddress, &offset);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    status = RtlULongAdd(offset, section->PointerToRawData, &offset);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    viewSize = ViewSize;
    va = MappedBase;
    status = KphAdvanceBuffer(&va, &viewSize, offset);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    *Va = va;

    return STATUS_SUCCESS;
}

/**
 * \brief Maps view into the system process address space. On success the
 * caller is attached to the system process. The call to unmap the view
 * will detach from the system process. The caller should unmap the view by
 * calling KphUnmapViewInSystemProcess.
 *
 * \param[in] FileHandle Handle to file to map.
 * \param[out] MappedBase Base address of the mapping.
 * \param[out] ViewSize Size of the mapped view. If the view exceeds the file
 * size, it is clamped to the file size.
 * \param[out] ApcState APC state to be passed to KphUnmapViewInSystemProcess.
 *
 * \return Successful or errant status.
 */
_IRQL_always_function_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphMapViewOfFileInSystemProcess(
    _In_ HANDLE FileHandle,
    _Outptr_result_bytebuffer_(*ViewSize) PVOID *MappedBase,
    _Inout_ PSIZE_T ViewSize,
    _Out_ PKAPC_STATE ApcState
    )
{
    NTSTATUS status;
    HANDLE sectionHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    FILE_STANDARD_INFORMATION fileInfo;
    PVOID mappedEnd;

    PAGED_PASSIVE();

    sectionHandle = NULL;
    *MappedBase = NULL;

    KeStackAttachProcess(PsInitialSystemProcess, ApcState);

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
#pragma warning(suppress : 4127) // conditional is constant
    if (sizeof(SIZE_T) < sizeof(fileInfo.EndOfFile.QuadPart))
    {
        if (fileInfo.EndOfFile.HighPart != 0)
        {
            status = STATUS_FILE_TOO_LARGE;
            goto Exit;
        }
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
                             SEC_COMMIT,
                             FileHandle);
    if (!NT_SUCCESS(status))
    {
        sectionHandle = NULL;
        goto Exit;
    }

    *MappedBase = NULL;
    status = ZwMapViewOfSection(sectionHandle,
                                ZwCurrentProcess(),
                                MappedBase,
                                0,
                                0,
                                NULL,
                                ViewSize,
                                ViewUnmap,
                                0,
                                PAGE_READONLY);
    if (!NT_SUCCESS(status))
    {
        *MappedBase = NULL;
        *ViewSize = 0;
        goto Exit;
    }

    if (*ViewSize > (SIZE_T)fileInfo.EndOfFile.QuadPart)
    {
        *ViewSize = fileInfo.EndOfFile.LowPart;
    }

    mappedEnd = Add2Ptr(*MappedBase, *ViewSize);
    if ((mappedEnd < *MappedBase) || (mappedEnd >= MmHighestUserAddress))
    {
        *MappedBase = NULL;
        *ViewSize = 0;
        status = STATUS_BUFFER_OVERFLOW;
        goto Exit;
    }

    status = STATUS_SUCCESS;

Exit:

    if (sectionHandle)
    {
        ObCloseHandle(sectionHandle, KernelMode);
    }

    if (!NT_SUCCESS(status))
    {
        KeUnstackDetachProcess(ApcState);
    }

    return status;
}

/**
 * \brief Unmaps view from the system process address space.
 *
 * \param[in] MappedBase Base address of the mapping.
 * \param[in] ApcState APC state from KphMapViewInSystemProcess.
 */
_IRQL_always_function_max_(PASSIVE_LEVEL)
VOID KphUnmapViewInSystemProcess(
    _In_ PVOID MappedBase,
    _In_ PKAPC_STATE ApcState
    )
{
    PAGED_PASSIVE();
    NT_ASSERT(PsGetCurrentProcess() == PsInitialSystemProcess);
    ZwUnmapViewOfSection(ZwCurrentProcess(), MappedBase);
    KeUnstackDetachProcess(ApcState);
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
 * \brief Retrieves the exit status from a thread.
 *
 * \details This exists to support Win7, we will use PsGetThreadExitStatus if
 * exported.
 *
 * \return Thread's exit status or STATUS_PENDING.
 */
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphGetThreadExitStatus(
    _In_ PETHREAD Thread
    )
{
    NTSTATUS status;
    HANDLE threadHandle;
    NTSTATUS exitStatus;
    THREAD_BASIC_INFORMATION threadInfo;

    PAGED_CODE();

    if (KphDynPsGetThreadExitStatus)
    {
        //
        // If we have this export we can fast-track it.
        //
        return KphDynPsGetThreadExitStatus(Thread);
    }

    //
    // Win7 doesn't export PsGetThreadExitStatus. Take this path instead.
    // If we fail (practically shouldn't) we will assume the thread is still
    // active.
    //

    status = ObOpenObjectByPointer(Thread,
                                   0,
                                   NULL,
                                   0,
                                   *PsThreadType,
                                   KernelMode,
                                   &threadHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      UTIL,
                      "ObOpenObjectByPointer failed: %!STATUS!",
                      status);

        threadHandle = NULL;
        exitStatus = STATUS_PENDING;
        goto Exit;
    }

    status = ZwQueryInformationThread(threadHandle,
                                      ThreadBasicInformation,
                                      &threadInfo,
                                      sizeof(threadInfo),
                                      NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      UTIL,
                      "ZwQueryInformationThread failed: %!STATUS!",
                      status);

        exitStatus = STATUS_PENDING;
        goto Exit;
    }

    exitStatus = threadInfo.ExitStatus;

Exit:

    if (threadHandle)
    {
        ObCloseHandle(threadHandle, KernelMode);
    }

    return exitStatus;
}

/**
 * \brief Compares two Unicode strings to determine whether one string is a
 * suffix of the other.
 *
 * \param[in] Suffix The string which might be a suffix of the other.
 * \param[in] String The string to check.
 * \param[in] CaseInSensitive If TRUE, case should be ignored when doing the
 * comparison.
 *
 * \return TRUE if the string suffixed by the other. 
 */
_IRQL_requires_max_(APC_LEVEL)
BOOLEAN KphSuffixUnicodeString(
    _In_ PUNICODE_STRING Suffix,
    _In_ PUNICODE_STRING String,
    _In_ BOOLEAN CaseInSensitive
    )
{
    UNICODE_STRING string;

    PAGED_CODE();

    if (String->Length < Suffix->Length)
    {
        return FALSE;
    }

    string.Length = Suffix->Length;
    string.MaximumLength = Suffix->Length;
    string.Buffer = &String->Buffer[(String->Length - Suffix->Length) / sizeof(WCHAR)];

    return RtlEqualUnicodeString(&string, Suffix, CaseInSensitive);
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

    result = KphSinglePrivilegeCheckEx(SeCreateTokenPrivilege,
                                       &subjectContext,
                                       UserMode);

    SeReleaseSubjectContext(&subjectContext);

    return result;
}
