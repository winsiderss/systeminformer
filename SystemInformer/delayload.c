/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021-2022
 *
 */

#include <phapp.h>

// CRT delayload support
// The msvc delayload handler throws exceptions when
// imports are unavailable instead of returning NULL (dmex)

PH_QUEUED_LOCK PhDelayLoadImportLock = PH_QUEUED_LOCK_INIT;
ULONG PhDelayLoadOldProtection = PAGE_WRITECOPY;
ULONG PhDelayLoadLockCount = 0;

// based on \MSVC\14.31.31103\include\dloadsup.h (dmex)
VOID PhDelayLoadImportAcquire(
    _In_ PVOID ImportDirectorySectionAddress,
    _In_ SIZE_T ImportDirectorySectionSize
    )
{
    PhAcquireQueuedLockExclusive(&PhDelayLoadImportLock);
    PhDelayLoadLockCount += 1;

    if (PhDelayLoadLockCount == 1)
    {
        NTSTATUS status;

        if (!NT_SUCCESS(status = NtProtectVirtualMemory(
            NtCurrentProcess(),
            &ImportDirectorySectionAddress,
            &ImportDirectorySectionSize,
            PAGE_READWRITE,
            &PhDelayLoadOldProtection
            )))
        {
            PhRaiseStatus(status);
        }
    }

    PhReleaseQueuedLockExclusive(&PhDelayLoadImportLock);
}

VOID PhDelayLoadImportRelease(
    _In_ PVOID ImportDirectorySectionAddress,
    _In_ SIZE_T ImportDirectorySectionSize
    )
{
    PhAcquireQueuedLockExclusive(&PhDelayLoadImportLock);
    PhDelayLoadLockCount -= 1;

    if (PhDelayLoadLockCount == 0)
    {
        ULONG importSectionOldProtection;
        NtProtectVirtualMemory(
            NtCurrentProcess(),
            &ImportDirectorySectionAddress,
            &ImportDirectorySectionSize,
            PhDelayLoadOldProtection,
            &importSectionOldProtection
            );
    }

    PhReleaseQueuedLockExclusive(&PhDelayLoadImportLock);
}

_Success_(return != NULL)
PVOID WINAPI __delayLoadHelper2(
    _In_ PIMAGE_DELAYLOAD_DESCRIPTOR Entry,
    _Inout_ PVOID* ImportAddress
    )
{
    BOOLEAN importNeedsFree = FALSE;
    PSTR importDllName;
    PVOID procedureAddress;
    PVOID moduleHandle;
    PVOID* importHandle;
    PIMAGE_THUNK_DATA importEntry;
    PIMAGE_THUNK_DATA importTable;
    PIMAGE_THUNK_DATA importNameTable;
    PIMAGE_NT_HEADERS imageNtHeaders;
    SIZE_T importDirectorySectionSize;
    PVOID importDirectorySectionAddress;

    importDllName = PTR_ADD_OFFSET(PhInstanceHandle, Entry->DllNameRVA);
    importHandle = PTR_ADD_OFFSET(PhInstanceHandle, Entry->ModuleHandleRVA);
    importTable = PTR_ADD_OFFSET(PhInstanceHandle, Entry->ImportAddressTableRVA);
    importNameTable = PTR_ADD_OFFSET(PhInstanceHandle, Entry->ImportNameTableRVA);

    if (!(moduleHandle = *importHandle))
    {
        PPH_STRING importDllNameString = PhZeroExtendToUtf16(importDllName);

        if (!(moduleHandle = PhLoadLibrary(importDllNameString->Buffer)))
        {
            PhDereferenceObject(importDllNameString);
            return NULL;
        }

        PhDereferenceObject(importDllNameString);
        importNeedsFree = TRUE;
    }

    importEntry = PTR_ADD_OFFSET(importNameTable, PTR_SUB_OFFSET(ImportAddress, importTable));

    if (IMAGE_SNAP_BY_ORDINAL(importEntry->u1.Ordinal))
    {
        USHORT procedureOrdinal = IMAGE_ORDINAL(importEntry->u1.Ordinal);
        procedureAddress = PhGetDllBaseProcedureAddress(moduleHandle, NULL, procedureOrdinal);
    }
    else
    {
        PIMAGE_IMPORT_BY_NAME importByName = PTR_ADD_OFFSET(PhInstanceHandle, importEntry->u1.AddressOfData);
        procedureAddress = PhGetDllBaseProcedureAddressWithHint(moduleHandle, importByName->Name, importByName->Hint);
    }

    if (!procedureAddress)
        return NULL;

    if (!NT_SUCCESS(PhGetLoaderEntryImageNtHeaders(
        PhInstanceHandle,
        &imageNtHeaders
        )))
    {
        return NULL;
    }

    if (!NT_SUCCESS(PhGetLoaderEntryImageVaToSection(
        PhInstanceHandle,
        imageNtHeaders,
        importTable,
        &importDirectorySectionAddress,
        &importDirectorySectionSize
        )))
    {
        return NULL;
    }

    PhDelayLoadImportAcquire(importDirectorySectionAddress, importDirectorySectionSize);
    InterlockedExchangePointer(ImportAddress, procedureAddress);
    PhDelayLoadImportRelease(importDirectorySectionAddress, importDirectorySectionSize);

    if ((InterlockedExchangePointer(importHandle, moduleHandle) == moduleHandle) && importNeedsFree)
    {
        PhFreeLibrary(moduleHandle); // already updated the cache (dmex)
    }

    return procedureAddress;
}
