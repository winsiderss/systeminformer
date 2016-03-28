/*
 * KProcessHacker
 *
 * Copyright (C) 2016 wj32
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

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KphFreeCapturedUnicodeString)
#pragma alloc_text(PAGE, KphCaptureUnicodeString)
#pragma alloc_text(PAGE, KphEnumerateSystemModules)
#pragma alloc_text(PAGE, KphValidateAddressForSystemModules)
#pragma alloc_text(PAGE, KphGetProcessMappedFileName)
#endif

VOID KphFreeCapturedUnicodeString(
    __in PUNICODE_STRING CapturedUnicodeString
    )
{
    PAGED_CODE();

    if (CapturedUnicodeString->Buffer)
        ExFreePoolWithTag(CapturedUnicodeString->Buffer, 'UhpK');
}

NTSTATUS KphCaptureUnicodeString(
    __in PUNICODE_STRING UnicodeString,
    __out PUNICODE_STRING CapturedUnicodeString
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
        unicodeString.Buffer = ExAllocatePoolWithTag(
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
    __out PRTL_PROCESS_MODULES *Modules
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
        buffer = ExAllocatePoolWithTag(PagedPool, bufferSize, 'ThpK');

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
    __in PVOID Address,
    __in SIZE_T Length
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
    __in HANDLE ProcessHandle,
    __in PVOID BaseAddress,
    __out PUNICODE_STRING *FileName
    )
{
    NTSTATUS status;
    PVOID buffer;
    SIZE_T bufferSize;
    SIZE_T returnLength;

    PAGED_CODE();

    bufferSize = 0x100;
    buffer = ExAllocatePoolWithTag(PagedPool, bufferSize, 'ThpK');

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
        buffer = ExAllocatePoolWithTag(PagedPool, bufferSize, 'ThpK');

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
