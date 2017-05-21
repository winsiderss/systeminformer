/*
 * Process Hacker Toolchain - 
 *   Image DelayLoad Helper
 * 
 * Copyright (C) 2017 dmex
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

#include <ph.h>
#include <delayimp.h>

extern const IMAGE_DOS_HEADER __ImageBase;

PVOID WINAPI __delayLoadHelper2(
    _In_ PIMAGE_DELAYLOAD_DESCRIPTOR Entry,
    _Out_ PVOID *ImportAddress
    )
{
    BOOLEAN needsFree = FALSE;
    MEMORY_BASIC_INFORMATION info;
    ULONG delayLoadJunk;
    PSTR importName;
    PVOID procedureAddress;
    PVOID moduleHandle;
    PVOID* importHandle;
    PIMAGE_THUNK_DATA entry;
    PIMAGE_THUNK_DATA importTable;
    PIMAGE_THUNK_DATA importNameTable;

    importName = PTR_ADD_OFFSET(&__ImageBase, Entry->DllNameRVA);
    importHandle = PTR_ADD_OFFSET(&__ImageBase, Entry->ModuleHandleRVA);
    importTable = PTR_ADD_OFFSET(&__ImageBase, Entry->ImportAddressTableRVA);
    importNameTable = PTR_ADD_OFFSET(&__ImageBase, Entry->ImportNameTableRVA);
   
    if (!strcmp(importName, "ProcessHacker.exe"))
    {
        moduleHandle = NtCurrentPeb()->ImageBaseAddress;
    }
    else
    {
        if (moduleHandle = LoadLibraryA(importName))
        {
            needsFree = TRUE;
        }
        else
        {
            DelayLoadInfo dli =
            {
                sizeof(DelayLoadInfo),
                (PCImgDelayDescr)Entry,
                (FARPROC*)ImportAddress,
                importName
            };

            RaiseException(
                VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND),
                0,
                1,
                (PULONG_PTR)&dli
                );

            return NULL;
        }
    }

    entry = PTR_ADD_OFFSET(importNameTable, (ULONG)((PBYTE)ImportAddress - (PBYTE)importTable));

    if (IMAGE_SNAP_BY_ORDINAL(entry->u1.Ordinal))
    {
        ULONG procedureOrdinal = IMAGE_ORDINAL(entry->u1.Ordinal);
        procedureAddress = PhGetProcedureAddress(moduleHandle, NULL, procedureOrdinal);
    }
    else
    {
        PSTR procedureName = ((PIMAGE_IMPORT_BY_NAME)PTR_ADD_OFFSET(&__ImageBase, entry->u1.AddressOfData))->Name;
        procedureAddress = PhGetProcedureAddress(moduleHandle, procedureName, 0);
    }

    if (!procedureAddress)
    {
        PSTR procedureName = ((PIMAGE_IMPORT_BY_NAME)PTR_ADD_OFFSET(&__ImageBase, entry->u1.AddressOfData))->Name;
        DelayLoadInfo dli =
        {
            sizeof(DelayLoadInfo),
            (PCImgDelayDescr)Entry,
            (FARPROC*)ImportAddress,
            procedureName
        };

        RaiseException(
            VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND),
            0,
            1,
            (PULONG_PTR)&dli
            );

        return NULL;
    }

    if (!NT_SUCCESS(NtQueryVirtualMemory(
        NtCurrentProcess(),
        ImportAddress,
        MemoryBasicInformation,
        &info,
        sizeof(MEMORY_BASIC_INFORMATION),
        NULL
        )))
    {
        return NULL;
    }

    if (!VirtualProtect(
        info.BaseAddress,
        info.RegionSize,
        PAGE_EXECUTE_READWRITE,
        &info.Protect
        ))
    {
        return NULL;
    }

    // Cache the procedure address (Fixes CRT delayload bug).
    if (InterlockedExchangePointer(ImportAddress, procedureAddress))
    {
        NOTHING;
    }

    if (!VirtualProtect(
        info.BaseAddress,
        info.RegionSize,
        info.Protect,
        &delayLoadJunk
        ))
    {
        return NULL;
    }

    // Cache the module handle in the IAT entry (required) (Fixes CRT use-after-free bug).
    if (InterlockedExchangePointer(importHandle, moduleHandle) == moduleHandle && needsFree)
    {
        FreeLibrary(moduleHandle); // A different thread has already updated the cache.
    }

    return procedureAddress;
}
