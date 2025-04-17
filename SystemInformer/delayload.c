/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021-2023
 *
 */

#include <phapp.h>
#include <mapldr.h>

// CRT delayload support
// The msvc delayload handler throws exceptions when
// imports are unavailable instead of returning NULL (dmex)

#ifndef PH_NATIVE_DELAYLOAD
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

#ifdef _M_ARM64
        NtFlushInstructionCache(
            NtCurrentProcess(),
            ImportDirectorySectionAddress,
            ImportDirectorySectionSize
            );
#endif
    }

    PhReleaseQueuedLockExclusive(&PhDelayLoadImportLock);
}
#endif

PVOID WINAPI __delayLoadHelper2(
    _In_ PIMAGE_DELAYLOAD_DESCRIPTOR DelayloadDescriptor,
    _In_ PIMAGE_THUNK_DATA ThunkAddress
    )
{
#if !defined(PH_NATIVE_DELAYLOAD)
    BOOLEAN importNeedsFree = FALSE;
    PCSTR importDllName;
    PVOID procedureAddress;
    PVOID moduleHandle;
    PVOID* importHandle;
    PIMAGE_THUNK_DATA importEntry;
    PIMAGE_THUNK_DATA importTable;
    PIMAGE_THUNK_DATA importNameTable;
    PIMAGE_NT_HEADERS imageNtHeaders;
    SIZE_T importDirectorySectionSize;
    PVOID importDirectorySectionAddress;

    importDllName = PTR_ADD_OFFSET(NtCurrentImageBase(), DelayloadDescriptor->DllNameRVA);
    importHandle = PTR_ADD_OFFSET(NtCurrentImageBase(), DelayloadDescriptor->ModuleHandleRVA);
    importTable = PTR_ADD_OFFSET(NtCurrentImageBase(), DelayloadDescriptor->ImportAddressTableRVA);
    importNameTable = PTR_ADD_OFFSET(NtCurrentImageBase(), DelayloadDescriptor->ImportNameTableRVA);

    if (!(moduleHandle = InterlockedCompareExchangePointer(importHandle, NULL, NULL)))
    {
        WCHAR importDllNameBuffer[DOS_MAX_PATH_LENGTH] = L"";

        PhZeroExtendToUtf16Buffer(importDllName, strlen(importDllName), importDllNameBuffer);

        if (!(moduleHandle = PhLoadLibrary(importDllNameBuffer)))
        {
            return NULL;
        }

        importNeedsFree = TRUE;
    }

    importEntry = PTR_ADD_OFFSET(importNameTable, PTR_SUB_OFFSET(ThunkAddress, importTable));

    if (IMAGE_SNAP_BY_ORDINAL(importEntry->u1.Ordinal))
    {
        USHORT procedureOrdinal = IMAGE_ORDINAL(importEntry->u1.Ordinal);
        procedureAddress = PhGetDllBaseProcedureAddress(moduleHandle, NULL, procedureOrdinal);
    }
    else
    {
        PIMAGE_IMPORT_BY_NAME importByName = PTR_ADD_OFFSET(NtCurrentImageBase(), importEntry->u1.AddressOfData);
        procedureAddress = PhGetDllBaseProcedureAddressWithHint(moduleHandle, importByName->Name, importByName->Hint);
    }

    if (!procedureAddress)
        return NULL;

    if (!NT_SUCCESS(PhGetLoaderEntryImageNtHeaders(
        NtCurrentImageBase(),
        &imageNtHeaders
        )))
    {
        return NULL;
    }

    if (!NT_SUCCESS(PhGetLoaderEntryImageVaToSection(
        NtCurrentImageBase(),
        imageNtHeaders,
        importTable,
        &importDirectorySectionAddress,
        &importDirectorySectionSize
        )))
    {
        return NULL;
    }

    PhDelayLoadImportAcquire(importDirectorySectionAddress, importDirectorySectionSize);
    InterlockedExchangePointer((PVOID)ThunkAddress, procedureAddress);
    PhDelayLoadImportRelease(importDirectorySectionAddress, importDirectorySectionSize);

    if ((InterlockedExchangePointer(importHandle, moduleHandle) == moduleHandle) && importNeedsFree)
    {
        PhFreeLibrary(moduleHandle); // already updated the cache (dmex)
    }

    return procedureAddress;
#else
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PVOID (WINAPI* DelayLoadFailureHook)(
        _In_ PCSTR DllName,
        _In_ PCSTR ProcName
        ) = NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        DelayLoadFailureHook = PhGetModuleProcAddress(L"kernel32.dll", "DelayLoadFailureHook"); // kernelbase.dll
        PhEndInitOnce(&initOnce);
    }

    return LdrResolveDelayLoadedAPI(
        NtCurrentImageBase(),
        DelayloadDescriptor,
        NULL,
        DelayLoadFailureHook,
        ThunkAddress,
        0
        );
#endif
}
