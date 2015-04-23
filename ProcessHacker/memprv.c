/*
 * Process Hacker -
 *   memory provider
 *
 * Copyright (C) 2010-2015 wj32
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

typedef struct _PHP_KNOWN_MEMORY_REGION
{
    LIST_ENTRY ListEntry;
    PH_AVL_LINKS Links;
    ULONG_PTR Address;
    SIZE_T Size;
    PPH_STRING Name;
} PHP_KNOWN_MEMORY_REGION, *PPHP_KNOWN_MEMORY_REGION;

VOID PhpMemoryItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

PPH_OBJECT_TYPE PhMemoryItemType;

BOOLEAN PhMemoryProviderInitialization(
    VOID
    )
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
    _Out_ PPH_MEMORY_PROVIDER Provider,
    _In_ HANDLE ProcessId,
    _In_ PPH_MEMORY_PROVIDER_CALLBACK Callback,
    _In_opt_ PVOID Context
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
    _Inout_ PPH_MEMORY_PROVIDER Provider
    )
{
    if (Provider->ProcessHandle)
        NtClose(Provider->ProcessHandle);
}

PPH_MEMORY_ITEM PhCreateMemoryItem(
    VOID
    )
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
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_MEMORY_ITEM memoryItem = (PPH_MEMORY_ITEM)Object;

    PhSwapReference(&memoryItem->Name, NULL);
}

VOID PhGetMemoryProtectionString(
    _In_ ULONG Protection,
    _Out_writes_(17) PWSTR String
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
    _In_ ULONG State
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
    _In_ ULONG Type
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

static LONG NTAPI PhpCompareKnownMemoryRegionCompareFunction(
    _In_ PPH_AVL_LINKS Links1,
    _In_ PPH_AVL_LINKS Links2
    )
{
    PPHP_KNOWN_MEMORY_REGION knownMemoryRegion1 = CONTAINING_RECORD(Links1, PHP_KNOWN_MEMORY_REGION, Links);
    PPHP_KNOWN_MEMORY_REGION knownMemoryRegion2 = CONTAINING_RECORD(Links2, PHP_KNOWN_MEMORY_REGION, Links);

    return uintptrcmp(knownMemoryRegion1->Address, knownMemoryRegion2->Address);
}

VOID PhpAddKnownMemoryRegion(
    _Inout_ PPH_AVL_TREE KnownMemoryRegionsSet,
    _Inout_ PLIST_ENTRY KnownMemoryRegionsListHead,
    _In_ ULONG_PTR Address,
    _In_ SIZE_T Size,
    _In_ _Assume_refs_(1) PPH_STRING Name
    )
{
    PPHP_KNOWN_MEMORY_REGION knownMemoryRegion;

    knownMemoryRegion = PhAllocate(sizeof(PHP_KNOWN_MEMORY_REGION));
    knownMemoryRegion->Address = Address;
    knownMemoryRegion->Size = Size;
    knownMemoryRegion->Name = Name;

    if (PhAddElementAvlTree(KnownMemoryRegionsSet, &knownMemoryRegion->Links))
    {
        // Duplicate address.
        PhDereferenceObject(Name);
        PhFree(knownMemoryRegion);
        return;
    }

    InsertTailList(KnownMemoryRegionsListHead, &knownMemoryRegion->ListEntry);
}

VOID PhpCreateKnownMemoryRegions(
    _In_ HANDLE ProcessId,
    _In_ HANDLE ProcessHandle,
    _Out_ PPH_AVL_TREE KnownMemoryRegionsSet,
    _Out_ PLIST_ENTRY KnownMemoryRegionsListHead
    )
{
    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;
    ULONG i;
    BOOLEAN isWow64 = FALSE;

    PhInitializeAvlTree(KnownMemoryRegionsSet, PhpCompareKnownMemoryRegionCompareFunction);
    InitializeListHead(KnownMemoryRegionsListHead);

    if (!NT_SUCCESS(PhEnumProcessesEx(&processes, SystemExtendedProcessInformation)))
        return;

    process = PhFindProcessInformation(processes, ProcessId);

    if (!process)
    {
        PhFree(processes);
        return;
    }

    // PEB
    {
        PROCESS_BASIC_INFORMATION basicInfo;
        PVOID peb32;

        if (NT_SUCCESS(PhGetProcessBasicInformation(ProcessHandle, &basicInfo)))
        {
            PhpAddKnownMemoryRegion(KnownMemoryRegionsSet, KnownMemoryRegionsListHead,
                (ULONG_PTR)basicInfo.PebBaseAddress, PAGE_SIZE,
                PhCreateString(L"Process PEB")
                );
        }

        if (NT_SUCCESS(PhGetProcessPeb32(ProcessHandle, &peb32)) && peb32 != 0)
        {
            isWow64 = TRUE;
            PhpAddKnownMemoryRegion(KnownMemoryRegionsSet, KnownMemoryRegionsListHead,
                (ULONG_PTR)peb32, PAGE_SIZE,
                PhCreateString(L"Process 32-bit PEB")
                );
        }
    }

    // TEB, stacks
    for (i = 0; i < process->NumberOfThreads; i++)
    {
        PSYSTEM_EXTENDED_THREAD_INFORMATION thread = (PSYSTEM_EXTENDED_THREAD_INFORMATION)process->Threads + i;

        if (thread->TebBase)
        {
            NT_TIB ntTib;
            SIZE_T bytesRead;

            PhpAddKnownMemoryRegion(KnownMemoryRegionsSet, KnownMemoryRegionsListHead,
                (ULONG_PTR)thread->TebBase, PAGE_SIZE,
                PhFormatString(L"Thread %u TEB", (ULONG)thread->ThreadInfo.ClientId.UniqueThread)
                );

            if (NT_SUCCESS(PhReadVirtualMemory(ProcessHandle, thread->TebBase, &ntTib, sizeof(NT_TIB), &bytesRead)) &&
                bytesRead == sizeof(NT_TIB))
            {
                if ((ULONG_PTR)ntTib.StackLimit < (ULONG_PTR)ntTib.StackBase)
                {
                    PhpAddKnownMemoryRegion(KnownMemoryRegionsSet, KnownMemoryRegionsListHead,
                        (ULONG_PTR)ntTib.StackLimit, (ULONG_PTR)ntTib.StackBase - (ULONG_PTR)ntTib.StackLimit,
                        PhFormatString(L"Thread %u Stack", (ULONG)thread->ThreadInfo.ClientId.UniqueThread)
                        );
                }

                if (isWow64 && ntTib.ExceptionList)
                {
                    ULONG teb32 = (ULONG)ntTib.ExceptionList;
                    NT_TIB32 ntTib32;

                    // This is actually pointless because the 64-bit and 32-bit TEBs usually share the same memory region.
                    PhpAddKnownMemoryRegion(KnownMemoryRegionsSet, KnownMemoryRegionsListHead,
                        teb32, PAGE_SIZE,
                        PhFormatString(L"Thread %u 32-bit TEB", (ULONG)thread->ThreadInfo.ClientId.UniqueThread)
                        );

                    if (NT_SUCCESS(PhReadVirtualMemory(ProcessHandle, (PVOID)teb32, &ntTib32, sizeof(NT_TIB32), &bytesRead)) &&
                        bytesRead == sizeof(NT_TIB32))
                    {
                        if (ntTib32.StackLimit < ntTib32.StackBase)
                        {
                            PhpAddKnownMemoryRegion(KnownMemoryRegionsSet, KnownMemoryRegionsListHead,
                                ntTib32.StackLimit, ntTib32.StackBase - ntTib32.StackLimit,
                                PhFormatString(L"Thread %u 32-bit Stack", (ULONG)thread->ThreadInfo.ClientId.UniqueThread)
                                );
                        }
                    }
                }
            }
        }
    }

    PhFree(processes);
}

VOID PhpFreeKnownMemoryRegions(
    _In_ PLIST_ENTRY KnownMemoryRegionsListHead
    )
{
    PLIST_ENTRY listEntry;
    PPHP_KNOWN_MEMORY_REGION knownMemoryRegion;

    listEntry = KnownMemoryRegionsListHead->Flink;

    while (listEntry != KnownMemoryRegionsListHead)
    {
        knownMemoryRegion = CONTAINING_RECORD(listEntry, PHP_KNOWN_MEMORY_REGION, ListEntry);
        listEntry = listEntry->Flink;

        PhSwapReference(&knownMemoryRegion->Name, NULL);
        PhFree(knownMemoryRegion);
    }
}

PPHP_KNOWN_MEMORY_REGION PhpFindKnownMemoryRegion(
    _In_ PPH_AVL_TREE KnownMemoryRegionsSet,
    _In_ ULONG_PTR Address
    )
{
    PHP_KNOWN_MEMORY_REGION lookupKnownMemoryRegion;
    PPH_AVL_LINKS links;
    PPHP_KNOWN_MEMORY_REGION knownMemoryRegion;
    LONG result;

    // Do an approximate search on the set to locate the known memory region with the largest
    // base address that is still smaller than the given address.
    lookupKnownMemoryRegion.Address = Address;
    links = PhFindElementAvlTree2(KnownMemoryRegionsSet, &lookupKnownMemoryRegion.Links, &result);
    knownMemoryRegion = NULL;

    if (links)
    {
        if (result == 0)
        {
            // Exact match.
        }
        else if (result < 0)
        {
            // The base of the closest known memory region is larger than our address. Assume the
            // preceding element (which is going to be smaller than our address) is the
            // one we're looking for.

            links = PhPredecessorElementAvlTree(links);
        }
        else
        {
            // The base of the closest known memory region is smaller than our address. Assume this
            // is the element we're looking for.
        }

        if (links)
        {
            knownMemoryRegion = CONTAINING_RECORD(links, PHP_KNOWN_MEMORY_REGION, Links);
        }
    }
    else
    {
        // No modules loaded.
    }

    if (knownMemoryRegion && Address < knownMemoryRegion->Address + knownMemoryRegion->Size)
        return knownMemoryRegion;
    else
        return NULL;
}

VOID PhMemoryProviderUpdate(
    _In_ PPH_MEMORY_PROVIDER Provider
    )
{
    PH_AVL_TREE knownMemoryRegionsSet;
    LIST_ENTRY knownMemoryRegionsListHead;
    PVOID baseAddress;
    MEMORY_BASIC_INFORMATION basicInfo;

    if (!Provider->ProcessHandle)
        return;

    PhpCreateKnownMemoryRegions(Provider->ProcessId, Provider->ProcessHandle, &knownMemoryRegionsSet, &knownMemoryRegionsListHead);

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

            if (NT_SUCCESS(PhGetProcessMappedFileName(Provider->ProcessHandle, memoryItem->BaseAddress, &fileName)))
            {
                memoryItem->Name = PhGetBaseName(fileName);
                PhDereferenceObject(fileName);
            }
        }

        // Get a known memory region name.
        if (!memoryItem->Name && basicInfo.BaseAddress)
        {
            PPHP_KNOWN_MEMORY_REGION knownMemoryRegion;

            knownMemoryRegion = PhpFindKnownMemoryRegion(&knownMemoryRegionsSet, (ULONG_PTR)basicInfo.BaseAddress);

            if (knownMemoryRegion)
            {
                PhReferenceObject(knownMemoryRegion->Name);
                memoryItem->Name = knownMemoryRegion->Name;
            }
        }

        cont = Provider->Callback(Provider, memoryItem);

        if (!cont)
            break;

ContinueLoop:
        baseAddress = PTR_ADD_OFFSET(baseAddress, basicInfo.RegionSize);
    }

    PhpFreeKnownMemoryRegions(&knownMemoryRegionsListHead);
}
