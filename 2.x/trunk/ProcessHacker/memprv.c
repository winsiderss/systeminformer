/*
 * Process Hacker - 
 *   memory provider
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

#define PH_MEMPRV_PRIVATE
#include <phapp.h>

VOID PhpMemoryItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    );

PPH_OBJECT_TYPE PhMemoryItemType;

BOOLEAN PhMemoryProviderInitialization()
{
    if (!NT_SUCCESS(PhCreateObjectType(
        &PhMemoryItemType,
        L"MemoryItem",
        0,
        PhpMemoryItemDeleteProcedure
        )))
        return FALSE;

    return TRUE;
}

VOID PhInitializeMemoryProvider(
    __out PPH_MEMORY_PROVIDER Provider,
    __in HANDLE ProcessId,
    __in PPH_MEMORY_PROVIDER_CALLBACK Callback,
    __in_opt PVOID Context
    )
{
    Provider->Callback = Callback;
    Provider->Context = Context;
    Provider->ProcessId = ProcessId;
    Provider->ProcessHandle = NULL;
    Provider->IgnoreFreeRegions = FALSE;

    if (!NT_SUCCESS(PhOpenProcess(
        &Provider->ProcessHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        ProcessId
        )))
    {
        PhOpenProcess(
            &Provider->ProcessHandle,
            PROCESS_QUERY_INFORMATION,
            ProcessId
            );
    }
}

VOID PhDeleteMemoryProvider(
    __inout PPH_MEMORY_PROVIDER Provider
    )
{
    if (Provider->ProcessHandle)
        NtClose(Provider->ProcessHandle);
}

PPH_MEMORY_ITEM PhCreateMemoryItem()
{
    PPH_MEMORY_ITEM memoryItem;

    if (!NT_SUCCESS(PhCreateObject(
        &memoryItem,
        sizeof(PH_MEMORY_ITEM),
        0,
        PhMemoryItemType
        )))
        return NULL;

    memset(memoryItem, 0, sizeof(PH_MEMORY_ITEM));

    return memoryItem;
}

VOID PhpMemoryItemDeleteProcedure(
    __in PVOID Object,
    __in ULONG Flags
    )
{
    PPH_MEMORY_ITEM memoryItem = (PPH_MEMORY_ITEM)Object;

    if (memoryItem->Name)
        PhDereferenceObject(memoryItem->Name);
}

VOID PhGetMemoryProtectionString(
    __in ULONG Protection,
    __out_ecount(17) PWSTR String
    )
{
    PWSTR string;
    PH_STRINGREF base;

    if (!Protection)
    {
        String[0] = 0;
        return;
    }

    if (Protection & PAGE_NOACCESS)
        PhInitializeStringRef(&base, L"NA");
    else if (Protection & PAGE_READONLY)
        PhInitializeStringRef(&base, L"R");
    else if (Protection & PAGE_READWRITE)
        PhInitializeStringRef(&base, L"RW");
    else if (Protection & PAGE_WRITECOPY)
        PhInitializeStringRef(&base, L"WC");
    else if (Protection & PAGE_EXECUTE)
        PhInitializeStringRef(&base, L"X");
    else if (Protection & PAGE_EXECUTE_READ)
        PhInitializeStringRef(&base, L"RX");
    else if (Protection & PAGE_EXECUTE_READWRITE)
        PhInitializeStringRef(&base, L"RWX");
    else if (Protection & PAGE_EXECUTE_WRITECOPY)
        PhInitializeStringRef(&base, L"WCX");
    else
        PhInitializeStringRef(&base, L"?");

    string = String;

    memcpy(string, base.Buffer, base.Length);
    string += base.Length / sizeof(WCHAR);

    if (Protection & PAGE_GUARD)
    {
        memcpy(string, L"+G", 2 * 2);
        string += 2;
    }

    if (Protection & PAGE_NOCACHE)
    {
        memcpy(string, L"+NC", 3 * 2);
        string += 3;
    }

    if (Protection & PAGE_WRITECOMBINE)
    {
        memcpy(string, L"+WCM", 4 * 2);
        string += 4;
    }

    *string = 0;
}

PWSTR PhGetMemoryStateString(
    __in ULONG State
    )
{
    if (State & MEM_COMMIT)
        return L"Commit";
    else if (State & MEM_RESERVE)
        return L"Reserve";
    else if (State & MEM_FREE)
        return L"Free";
    else
        return L"Unknown";
}

PWSTR PhGetMemoryTypeString(
    __in ULONG Type
    )
{
    if (Type & MEM_PRIVATE)
        return L"Private";
    else if (Type & MEM_MAPPED)
        return L"Mapped";
    else if (Type & MEM_IMAGE)
        return L"Image";
    else
        return L"Unknown";
}

VOID PhMemoryProviderUpdate(
    __in PPH_MEMORY_PROVIDER Provider
    )
{
    PVOID baseAddress;
    MEMORY_BASIC_INFORMATION basicInfo;

    if (!Provider->ProcessHandle)
        return;

    baseAddress = (PVOID)0;

    while (NT_SUCCESS(NtQueryVirtualMemory(
        Provider->ProcessHandle,
        baseAddress,
        MemoryBasicInformation,
        &basicInfo,
        sizeof(MEMORY_BASIC_INFORMATION),
        NULL
        )))
    {
        PPH_MEMORY_ITEM memoryItem;
        BOOLEAN cont;

        if (Provider->IgnoreFreeRegions && basicInfo.Type == MEM_FREE)
            goto ContinueLoop;

        memoryItem = PhCreateMemoryItem();

        memoryItem->BaseAddress = basicInfo.BaseAddress;
        PhPrintPointer(memoryItem->BaseAddressString, memoryItem->BaseAddress);
        memoryItem->Size = basicInfo.RegionSize;
        memoryItem->Flags = basicInfo.State | basicInfo.Type;
        memoryItem->Protection = basicInfo.Protect;

        // Get the mapped file name.
        if (memoryItem->Flags & (MEM_MAPPED | MEM_IMAGE))
        {
            PPH_STRING fileName;

            if (NT_SUCCESS(PhGetProcessMappedFileName(
                Provider->ProcessHandle,
                memoryItem->BaseAddress,
                &fileName
                )))
            {
                memoryItem->Name = PhGetBaseName(fileName);
                PhDereferenceObject(fileName);
            }
        }

        cont = Provider->Callback(Provider, memoryItem);

        if (!cont)
            break;

ContinueLoop:
        baseAddress = PTR_ADD_OFFSET(baseAddress, basicInfo.RegionSize);
    }
}
