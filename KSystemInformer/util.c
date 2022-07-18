/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *
 */

#include <kph.h>
#include <dyndata.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KphFreeCapturedUnicodeString)
#pragma alloc_text(PAGE, KphCaptureUnicodeString)
#pragma alloc_text(PAGE, KphEnumerateSystemModules)
#pragma alloc_text(PAGE, KphValidateAddressForSystemModules)
#pragma alloc_text(PAGE, KphGetProcessMappedFileName)
#pragma alloc_text(PAGE, KphImageNtHeader)
#endif

VOID KphFreeCapturedUnicodeString(
    _In_ PUNICODE_STRING CapturedUnicodeString
    )
{
    PAGED_CODE();

    if (CapturedUnicodeString->Buffer)
        ExFreePoolWithTag(CapturedUnicodeString->Buffer, 'UhpK');
}

NTSTATUS KphCaptureUnicodeString(
    _In_ PUNICODE_STRING UnicodeString,
    _Out_ PUNICODE_STRING CapturedUnicodeString
    )
{
    UNICODE_STRING unicodeString;
    PWCHAR userBuffer;

    PAGED_CODE();

    __try
    {
        ProbeForRead(UnicodeString, sizeof(UNICODE_STRING), sizeof(ULONG));
        unicodeString.Length = UnicodeString->Length;
        unicodeString.MaximumLength = unicodeString.Length;
        unicodeString.Buffer = NULL;

        userBuffer = UnicodeString->Buffer;
        ProbeForRead(userBuffer, unicodeString.Length, sizeof(WCHAR));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return GetExceptionCode();
    }

    if (unicodeString.Length & 1)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (unicodeString.Length != 0)
    {
        unicodeString.Buffer = ExAllocatePoolZero(
            PagedPool,
            unicodeString.Length,
            'UhpK'
            );

        if (!unicodeString.Buffer)
            return STATUS_INSUFFICIENT_RESOURCES;

        __try
        {
            memcpy(
                unicodeString.Buffer,
                userBuffer,
                unicodeString.Length
                );
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            KphFreeCapturedUnicodeString(&unicodeString);
            return GetExceptionCode();
        }
    }

    *CapturedUnicodeString = unicodeString;

    return STATUS_SUCCESS;
}

/**
 * Enumerates the modules loaded by the kernel.
 *
 * \param Modules A variable which receives a pointer to a structure containing information about
 * the kernel modules. The structure must be freed with the tag 'ThpK'.
 */
NTSTATUS KphEnumerateSystemModules(
    _Out_ PRTL_PROCESS_MODULES *Modules
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts;

    PAGED_CODE();

    bufferSize = 2048;
    attempts = 8;

    do
    {
        buffer = ExAllocatePoolZero(PagedPool, bufferSize, 'ThpK');

        if (!buffer)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        status = ZwQuerySystemInformation(
            SystemModuleInformation,
            buffer,
            bufferSize,
            &bufferSize
            );

        if (NT_SUCCESS(status))
        {
            *Modules = buffer;

            return status;
        }

        ExFreePoolWithTag(buffer, 'ThpK');

        if (status != STATUS_INFO_LENGTH_MISMATCH)
        {
            break;
        }
    } while (--attempts);

    return status;
}

/**
 * Checks if an address range lies within a kernel module.
 *
 * \param Address The beginning of the address range.
 * \param Length The number of bytes in the address range.
 */
NTSTATUS KphValidateAddressForSystemModules(
    _In_ PVOID Address,
    _In_ SIZE_T Length
    )
{
    NTSTATUS status;
    PRTL_PROCESS_MODULES modules;
    ULONG i;
    BOOLEAN valid;

    PAGED_CODE();

    status = KphEnumerateSystemModules(&modules);

    if (!NT_SUCCESS(status))
        return status;

    valid = FALSE;

    for (i = 0; i < modules->NumberOfModules; i++)
    {
        if (
            (ULONG_PTR)Address + Length >= (ULONG_PTR)Address &&
            (ULONG_PTR)Address >= (ULONG_PTR)modules->Modules[i].ImageBase &&
            (ULONG_PTR)Address + Length <= (ULONG_PTR)modules->Modules[i].ImageBase + modules->Modules[i].ImageSize
            )
        {
            dprintf("Validated address 0x%Ix in %s\n", Address, modules->Modules[i].FullPathName);
            valid = TRUE;
            break;
        }
    }

    ExFreePoolWithTag(modules, 'ThpK');

    if (valid)
        status = STATUS_SUCCESS;
    else
        status = STATUS_ACCESS_VIOLATION;

    return status;
}

/**
 * Gets the file name of a mapped section.
 *
 * \param ProcessHandle A handle to a process. The handle must have PROCESS_QUERY_INFORMATION
 * access.
 * \param BaseAddress The base address of the section view.
 * \param Modules A variable which receives a pointer to a string containing the file name of the
 * section. The structure must be freed with the tag 'ThpK'.
 */
NTSTATUS KphGetProcessMappedFileName(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _Out_ PUNICODE_STRING *FileName
    )
{
    NTSTATUS status;
    PVOID buffer;
    SIZE_T bufferSize;
    SIZE_T returnLength;

    PAGED_CODE();

    bufferSize = 0x100;
    buffer = ExAllocatePoolZero(PagedPool, bufferSize, 'ThpK');

    if (!buffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    status = ZwQueryVirtualMemory(
        ProcessHandle,
        BaseAddress,
        MemoryMappedFilenameInformation,
        buffer,
        bufferSize,
        &returnLength
        );

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        ExFreePoolWithTag(buffer, 'ThpK');
        bufferSize = returnLength;
        buffer = ExAllocatePoolZero(PagedPool, bufferSize, 'ThpK');

        if (!buffer)
            return STATUS_INSUFFICIENT_RESOURCES;

        status = ZwQueryVirtualMemory(
            ProcessHandle,
            BaseAddress,
            MemoryMappedFilenameInformation,
            buffer,
            bufferSize,
            &returnLength
            );
    }

    if (!NT_SUCCESS(status))
    {
        ExFreePoolWithTag(buffer, 'ThpK');
        return status;
    }

    *FileName = buffer;

    return status;
}

/**
 * Retrieves the image headers from a module base address. 
 *
 * \param[in] Base - Module base address. 
 * \param[in] Size - Size of the address range to parse.
 * \param[out] OutHeaders - On success points to the image headers.
 *
 * \return Appropriate status.
*/
_IRQL_requires_min_(APC_LEVEL)
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
