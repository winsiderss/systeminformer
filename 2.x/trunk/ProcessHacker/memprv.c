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

#include <phapp.h>
#include <heapstruct.h>

#define MAX_HEAPS 1000
#define WS_REQUEST_COUNT (PAGE_SIZE / sizeof(MEMORY_WORKING_SET_EX_INFORMATION))

VOID PhpMemoryItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

PPH_OBJECT_TYPE PhMemoryItemType;

BOOLEAN PhMemoryProviderInitialization(
    VOID
    )
{
    PhMemoryItemType = PhCreateObjectType(L"MemoryItem", 0, PhpMemoryItemDeleteProcedure);

    return TRUE;
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
        return L"Reserved";
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

PPH_MEMORY_ITEM PhCreateMemoryItem(
    VOID
    )
{
    PPH_MEMORY_ITEM memoryItem;

    memoryItem = PhCreateObject(sizeof(PH_MEMORY_ITEM), PhMemoryItemType);
    memset(memoryItem, 0, sizeof(PH_MEMORY_ITEM));

    return memoryItem;
}

VOID PhpMemoryItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PPH_MEMORY_ITEM memoryItem = Object;

    switch (memoryItem->RegionType)
    {
    case CustomRegion:
        PhClearReference(&memoryItem->u.Custom.Text);
        break;
    case MappedFileRegion:
        PhClearReference(&memoryItem->u.MappedFile.FileName);
        break;
    }
}

static LONG NTAPI PhpMemoryItemCompareFunction(
    _In_ PPH_AVL_LINKS Links1,
    _In_ PPH_AVL_LINKS Links2
    )
{
    PPH_MEMORY_ITEM memoryItem1 = CONTAINING_RECORD(Links1, PH_MEMORY_ITEM, Links);
    PPH_MEMORY_ITEM memoryItem2 = CONTAINING_RECORD(Links2, PH_MEMORY_ITEM, Links);

    return uintptrcmp((ULONG_PTR)memoryItem1->BaseAddress, (ULONG_PTR)memoryItem2->BaseAddress);
}

VOID PhDeleteMemoryItemList(
    _In_ PPH_MEMORY_ITEM_LIST List
    )
{
    PLIST_ENTRY listEntry;
    PPH_MEMORY_ITEM memoryItem;

    listEntry = List->ListHead.Flink;

    while (listEntry != &List->ListHead)
    {
        memoryItem = CONTAINING_RECORD(listEntry, PH_MEMORY_ITEM, ListEntry);
        listEntry = listEntry->Flink;

        PhDereferenceObject(memoryItem);
    }
}

PPH_MEMORY_ITEM PhLookupMemoryItemList(
    _In_ PPH_MEMORY_ITEM_LIST List,
    _In_ PVOID Address
    )
{
    PH_MEMORY_ITEM lookupMemoryItem;
    PPH_AVL_LINKS links;
    PPH_MEMORY_ITEM memoryItem;
    LONG result;

    // Do an approximate search on the set to locate the memory item with the largest
    // base address that is still smaller than the given address.
    lookupMemoryItem.BaseAddress = Address;
    links = PhFindElementAvlTree2(&List->Set, &lookupMemoryItem.Links, &result);
    memoryItem = NULL;

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
            memoryItem = CONTAINING_RECORD(links, PH_MEMORY_ITEM, Links);
        }
    }
    else
    {
        // No modules loaded.
    }

    if (memoryItem && (ULONG_PTR)Address < (ULONG_PTR)memoryItem->BaseAddress + memoryItem->RegionSize)
        return memoryItem;
    else
        return NULL;
}

PPH_MEMORY_ITEM PhpSetMemoryRegionType(
    _In_ PPH_MEMORY_ITEM_LIST List,
    _In_ PVOID Address,
    _In_ BOOLEAN GoToAllocationBase,
    _In_ PH_MEMORY_REGION_TYPE RegionType
    )
{
    PPH_MEMORY_ITEM memoryItem;

    memoryItem = PhLookupMemoryItemList(List, Address);

    if (!memoryItem)
        return NULL;

    if (GoToAllocationBase && memoryItem->AllocationBaseItem)
        memoryItem = memoryItem->AllocationBaseItem;

    if (memoryItem->RegionType != UnknownRegion)
        return NULL;

    memoryItem->RegionType = RegionType;

    return memoryItem;
}

NTSTATUS PhpUpdateMemoryRegionTypes(
    _In_ PPH_MEMORY_ITEM_LIST List,
    _In_ HANDLE ProcessHandle
    )
{
    NTSTATUS status;
    PVOID processes;
    PSYSTEM_PROCESS_INFORMATION process;
    ULONG i;
#ifdef _WIN64
    BOOLEAN isWow64 = FALSE;
#endif
    PPH_MEMORY_ITEM memoryItem;
    PLIST_ENTRY listEntry;

    if (!NT_SUCCESS(status = PhEnumProcessesEx(&processes, SystemExtendedProcessInformation)))
        return status;

    process = PhFindProcessInformation(processes, List->ProcessId);

    if (!process)
    {
        PhFree(processes);
        return STATUS_NOT_FOUND;
    }

    // USER_SHARED_DATA
    PhpSetMemoryRegionType(List, USER_SHARED_DATA, TRUE, UserSharedDataRegion);

    // PEB, heap
    {
        PROCESS_BASIC_INFORMATION basicInfo;
        ULONG numberOfHeaps;
        PVOID processHeapsPtr;
        PVOID *processHeaps;
        ULONG i;
#ifdef _WIN64
        PVOID peb32;
        ULONG processHeapsPtr32;
        ULONG *processHeaps32;
#endif

        if (NT_SUCCESS(PhGetProcessBasicInformation(ProcessHandle, &basicInfo)))
        {
            PhpSetMemoryRegionType(List, basicInfo.PebBaseAddress, TRUE, PebRegion);

            if (NT_SUCCESS(PhReadVirtualMemory(ProcessHandle,
                PTR_ADD_OFFSET(basicInfo.PebBaseAddress, FIELD_OFFSET(PEB, NumberOfHeaps)),
                &numberOfHeaps, sizeof(ULONG), NULL)) && numberOfHeaps < MAX_HEAPS)
            {
                processHeaps = PhAllocate(numberOfHeaps * sizeof(PVOID));

                if (NT_SUCCESS(PhReadVirtualMemory(ProcessHandle,
                    PTR_ADD_OFFSET(basicInfo.PebBaseAddress, FIELD_OFFSET(PEB, ProcessHeaps)),
                    &processHeapsPtr, sizeof(PVOID), NULL)) &&
                    NT_SUCCESS(PhReadVirtualMemory(ProcessHandle,
                    processHeapsPtr,
                    processHeaps, numberOfHeaps * sizeof(PVOID), NULL)))
                {
                    for (i = 0; i < numberOfHeaps; i++)
                    {
                        if (memoryItem = PhpSetMemoryRegionType(List, processHeaps[i], TRUE, HeapRegion))
                            memoryItem->u.Heap.Index = i;
                    }
                }

                PhFree(processHeaps);
            }
        }
#ifdef _WIN64

        if (NT_SUCCESS(PhGetProcessPeb32(ProcessHandle, &peb32)) && peb32 != 0)
        {
            isWow64 = TRUE;
            PhpSetMemoryRegionType(List, peb32, TRUE, Peb32Region);

            if (NT_SUCCESS(PhReadVirtualMemory(ProcessHandle,
                PTR_ADD_OFFSET(peb32, FIELD_OFFSET(PEB32, NumberOfHeaps)),
                &numberOfHeaps, sizeof(ULONG), NULL)) && numberOfHeaps < MAX_HEAPS)
            {
                processHeaps32 = PhAllocate(numberOfHeaps * sizeof(ULONG));

                if (NT_SUCCESS(PhReadVirtualMemory(ProcessHandle,
                    PTR_ADD_OFFSET(peb32, FIELD_OFFSET(PEB32, ProcessHeaps)),
                    &processHeapsPtr32, sizeof(ULONG), NULL)) &&
                    NT_SUCCESS(PhReadVirtualMemory(ProcessHandle,
                    UlongToPtr(processHeapsPtr32),
                    processHeaps32, numberOfHeaps * sizeof(ULONG), NULL)))
                {
                    for (i = 0; i < numberOfHeaps; i++)
                    {
                        if (memoryItem = PhpSetMemoryRegionType(List, UlongToPtr(processHeaps32[i]), TRUE, Heap32Region))
                            memoryItem->u.Heap.Index = i;
                    }
                }

                PhFree(processHeaps32);
            }
        }
#endif
    }

    // TEB, stack
    for (i = 0; i < process->NumberOfThreads; i++)
    {
        PSYSTEM_EXTENDED_THREAD_INFORMATION thread = (PSYSTEM_EXTENDED_THREAD_INFORMATION)process->Threads + i;

        if (WindowsVersion < WINDOWS_VISTA)
        {
            HANDLE threadHandle;
            THREAD_BASIC_INFORMATION basicInfo;

            if (NT_SUCCESS(PhOpenThread(&threadHandle, ThreadQueryAccess, thread->ThreadInfo.ClientId.UniqueThread)))
            {
                if (NT_SUCCESS(PhGetThreadBasicInformation(threadHandle, &basicInfo)))
                    thread->TebBase = basicInfo.TebBaseAddress;

                NtClose(threadHandle);
            }
        }

        if (thread->TebBase)
        {
            NT_TIB ntTib;
            SIZE_T bytesRead;

            if (memoryItem = PhpSetMemoryRegionType(List, thread->TebBase, TRUE, TebRegion))
                memoryItem->u.Teb.ThreadId = thread->ThreadInfo.ClientId.UniqueThread;

            if (NT_SUCCESS(PhReadVirtualMemory(ProcessHandle, thread->TebBase, &ntTib, sizeof(NT_TIB), &bytesRead)) &&
                bytesRead == sizeof(NT_TIB))
            {
                if ((ULONG_PTR)ntTib.StackLimit < (ULONG_PTR)ntTib.StackBase)
                {
                    if (memoryItem = PhpSetMemoryRegionType(List, ntTib.StackLimit, TRUE, StackRegion))
                        memoryItem->u.Stack.ThreadId = thread->ThreadInfo.ClientId.UniqueThread;
                }
#ifdef _WIN64

                if (isWow64 && ntTib.ExceptionList)
                {
                    ULONG teb32 = PtrToUlong(ntTib.ExceptionList);
                    NT_TIB32 ntTib32;

                    // 64-bit and 32-bit TEBs usually share the same memory region, so don't do anything for the 32-bit
                    // TEB.

                    if (NT_SUCCESS(PhReadVirtualMemory(ProcessHandle, UlongToPtr(teb32), &ntTib32, sizeof(NT_TIB32), &bytesRead)) &&
                        bytesRead == sizeof(NT_TIB32))
                    {
                        if (ntTib32.StackLimit < ntTib32.StackBase)
                        {
                            if (memoryItem = PhpSetMemoryRegionType(List, UlongToPtr(ntTib32.StackLimit), TRUE, Stack32Region))
                                memoryItem->u.Stack.ThreadId = thread->ThreadInfo.ClientId.UniqueThread;
                        }
                    }
                }
#endif
            }
        }
    }

    // Mapped file, heap segment, unusable
    for (listEntry = List->ListHead.Flink; listEntry != &List->ListHead; listEntry = listEntry->Flink)
    {
        memoryItem = CONTAINING_RECORD(listEntry, PH_MEMORY_ITEM, ListEntry);

        if (memoryItem->RegionType != UnknownRegion)
            continue;

        if ((memoryItem->Type & (MEM_MAPPED | MEM_IMAGE)) && memoryItem->AllocationBaseItem == memoryItem)
        {
            PPH_STRING fileName;

            if (NT_SUCCESS(PhGetProcessMappedFileName(ProcessHandle, memoryItem->BaseAddress, &fileName)))
            {
                PPH_STRING newFileName = PhResolveDevicePrefix(fileName);

                if (newFileName)
                    PhMoveReference(&fileName, newFileName);

                memoryItem->RegionType = MappedFileRegion;
                memoryItem->u.MappedFile.FileName = fileName;
                continue;
            }
        }

        if (memoryItem->State & MEM_COMMIT)
        {
            UCHAR buffer[HEAP_SEGMENT_MAX_SIZE];

            if (NT_SUCCESS(PhReadVirtualMemory(ProcessHandle, memoryItem->BaseAddress,
                buffer, sizeof(buffer), NULL)))
            {
                PVOID candidateHeap = NULL;
                ULONG candidateHeap32 = 0;
                PPH_MEMORY_ITEM heapMemoryItem;

                if (WindowsVersion >= WINDOWS_VISTA)
                {
                    PHEAP_SEGMENT heapSegment = (PHEAP_SEGMENT)buffer;
                    PHEAP_SEGMENT32 heapSegment32 = (PHEAP_SEGMENT32)buffer;

                    if (heapSegment->SegmentSignature == HEAP_SEGMENT_SIGNATURE)
                        candidateHeap = heapSegment->Heap;
                    if (heapSegment32->SegmentSignature == HEAP_SEGMENT_SIGNATURE)
                        candidateHeap32 = heapSegment32->Heap;
                }
                else
                {
                    PHEAP_SEGMENT_OLD heapSegment = (PHEAP_SEGMENT_OLD)buffer;
                    PHEAP_SEGMENT_OLD32 heapSegment32 = (PHEAP_SEGMENT_OLD32)buffer;

                    if (heapSegment->Signature == HEAP_SEGMENT_SIGNATURE)
                        candidateHeap = heapSegment->Heap;
                    if (heapSegment32->Signature == HEAP_SEGMENT_SIGNATURE)
                        candidateHeap32 = heapSegment32->Heap;
                }

                if (candidateHeap)
                {
                    heapMemoryItem = PhLookupMemoryItemList(List, candidateHeap);

                    if (heapMemoryItem && heapMemoryItem->BaseAddress == candidateHeap &&
                        heapMemoryItem->RegionType == HeapRegion)
                    {
                        memoryItem->RegionType = HeapSegmentRegion;
                        memoryItem->u.HeapSegment.HeapItem = heapMemoryItem;
                        continue;
                    }
                }
                else if (candidateHeap32)
                {
                    heapMemoryItem = PhLookupMemoryItemList(List, UlongToPtr(candidateHeap32));

                    if (heapMemoryItem && heapMemoryItem->BaseAddress == UlongToPtr(candidateHeap32) &&
                        heapMemoryItem->RegionType == Heap32Region)
                    {
                        memoryItem->RegionType = HeapSegment32Region;
                        memoryItem->u.HeapSegment.HeapItem = heapMemoryItem;
                        continue;
                    }
                }
            }
        }
    }

    PhFree(processes);

    return STATUS_SUCCESS;
}

NTSTATUS PhpUpdateMemoryWsCounters(
    _In_ PPH_MEMORY_ITEM_LIST List,
    _In_ HANDLE ProcessHandle
    )
{
    PLIST_ENTRY listEntry;
    PMEMORY_WORKING_SET_EX_INFORMATION info;

    info = PhAllocatePage(WS_REQUEST_COUNT * sizeof(MEMORY_WORKING_SET_EX_INFORMATION), NULL);

    if (!info)
        return STATUS_NO_MEMORY;

    for (listEntry = List->ListHead.Flink; listEntry != &List->ListHead; listEntry = listEntry->Flink)
    {
        PPH_MEMORY_ITEM memoryItem = CONTAINING_RECORD(listEntry, PH_MEMORY_ITEM, ListEntry);
        ULONG_PTR virtualAddress;
        SIZE_T remainingPages;
        SIZE_T requestPages;
        SIZE_T i;

        if (!(memoryItem->State & MEM_COMMIT))
            continue;

        virtualAddress = (ULONG_PTR)memoryItem->BaseAddress;
        remainingPages = memoryItem->RegionSize / PAGE_SIZE;

        while (remainingPages != 0)
        {
            requestPages = min(remainingPages, WS_REQUEST_COUNT);

            for (i = 0; i < requestPages; i++)
            {
                info[i].VirtualAddress = (PVOID)virtualAddress;
                virtualAddress += PAGE_SIZE;
            }

            if (NT_SUCCESS(NtQueryVirtualMemory(
                ProcessHandle,
                NULL,
                MemoryWorkingSetExInformation,
                info,
                requestPages * sizeof(MEMORY_WORKING_SET_EX_INFORMATION),
                NULL
                )))
            {
                for (i = 0; i < requestPages; i++)
                {
                    PMEMORY_WORKING_SET_EX_BLOCK block = &info[i].u1.VirtualAttributes;

                    if (block->Valid)
                    {
                        memoryItem->TotalWorkingSetPages++;

                        if (block->ShareCount > 1)
                            memoryItem->SharedWorkingSetPages++;
                        if (block->ShareCount == 0)
                            memoryItem->PrivateWorkingSetPages++;
                        if (block->Shared)
                            memoryItem->ShareableWorkingSetPages++;
                        if (block->Locked)
                            memoryItem->LockedWorkingSetPages++;
                    }
                }
            }

            remainingPages -= requestPages;
        }
    }

    PhFreePage(info);

    return STATUS_SUCCESS;
}

NTSTATUS PhpUpdateMemoryWsCountersOld(
    _In_ PPH_MEMORY_ITEM_LIST List,
    _In_ HANDLE ProcessHandle
    )
{
    NTSTATUS status;
    PMEMORY_WORKING_SET_INFORMATION info;
    PPH_MEMORY_ITEM memoryItem = NULL;
    ULONG_PTR i;

    if (!NT_SUCCESS(status = PhGetProcessWorkingSetInformation(ProcessHandle, &info)))
        return status;

    for (i = 0; i < info->NumberOfEntries; i++)
    {
        PMEMORY_WORKING_SET_BLOCK block = &info->WorkingSetInfo[i];
        ULONG_PTR virtualAddress = block->VirtualPage * PAGE_SIZE;

        if (!memoryItem || virtualAddress < (ULONG_PTR)memoryItem->BaseAddress ||
            virtualAddress >= (ULONG_PTR)memoryItem->BaseAddress + memoryItem->RegionSize)
        {
            memoryItem = PhLookupMemoryItemList(List, (PVOID)virtualAddress);
        }

        if (memoryItem)
        {
            memoryItem->TotalWorkingSetPages++;

            if (block->ShareCount > 1)
                memoryItem->SharedWorkingSetPages++;
            if (block->ShareCount == 0)
                memoryItem->PrivateWorkingSetPages++;
            if (block->Shared)
                memoryItem->ShareableWorkingSetPages++;
        }
    }

    PhFree(info);

    return STATUS_SUCCESS;
}

NTSTATUS PhQueryMemoryItemList(
    _In_ HANDLE ProcessId,
    _In_ ULONG Flags,
    _Out_ PPH_MEMORY_ITEM_LIST List
    )
{
    NTSTATUS status;
    HANDLE processHandle;
    ULONG_PTR allocationGranularity;
    PVOID baseAddress = (PVOID)0;
    MEMORY_BASIC_INFORMATION basicInfo;
    PPH_MEMORY_ITEM allocationBaseItem = NULL;
    PPH_MEMORY_ITEM previousMemoryItem = NULL;

    if (!NT_SUCCESS(status = PhOpenProcess(
        &processHandle,
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        ProcessId
        )))
    {
        if (!NT_SUCCESS(status = PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_INFORMATION,
            ProcessId
            )))
        {
            return status;
        }
    }

    List->ProcessId = ProcessId;
    PhInitializeAvlTree(&List->Set, PhpMemoryItemCompareFunction);
    InitializeListHead(&List->ListHead);

    allocationGranularity = PhSystemBasicInformation.AllocationGranularity;

    while (NT_SUCCESS(NtQueryVirtualMemory(
        processHandle,
        baseAddress,
        MemoryBasicInformation,
        &basicInfo,
        sizeof(MEMORY_BASIC_INFORMATION),
        NULL
        )))
    {
        PPH_MEMORY_ITEM memoryItem;

        if (basicInfo.State & MEM_FREE)
        {
            if (Flags & PH_QUERY_MEMORY_IGNORE_FREE)
                goto ContinueLoop;

            basicInfo.AllocationBase = basicInfo.BaseAddress;
        }

        memoryItem = PhCreateMemoryItem();
        memoryItem->BasicInfo = basicInfo;

        if (basicInfo.AllocationBase == basicInfo.BaseAddress)
            allocationBaseItem = memoryItem;
        if (allocationBaseItem && basicInfo.AllocationBase == allocationBaseItem->BaseAddress)
            memoryItem->AllocationBaseItem = allocationBaseItem;

        if (basicInfo.State & MEM_COMMIT)
        {
            memoryItem->CommittedSize = memoryItem->RegionSize;

            if (basicInfo.Type & MEM_PRIVATE)
                memoryItem->PrivateSize = memoryItem->RegionSize;
        }

        PhAddElementAvlTree(&List->Set, &memoryItem->Links);
        InsertTailList(&List->ListHead, &memoryItem->ListEntry);

        if (basicInfo.State & MEM_FREE)
        {
            if ((ULONG_PTR)basicInfo.BaseAddress & (allocationGranularity - 1))
            {
                ULONG_PTR nextAllocationBase;
                ULONG_PTR potentialUnusableSize;

                // Split this free region into an unusable and a (possibly empty) usable region.

                nextAllocationBase = ALIGN_UP_BY(basicInfo.BaseAddress, allocationGranularity);
                potentialUnusableSize = nextAllocationBase - (ULONG_PTR)basicInfo.BaseAddress;

                memoryItem->RegionType = UnusableRegion;

                // VMMap does this, but is it correct?
                //if (previousMemoryItem && (previousMemoryItem->State & MEM_COMMIT))
                //    memoryItem->CommittedSize = min(potentialUnusableSize, basicInfo.RegionSize);

                if (nextAllocationBase < (ULONG_PTR)basicInfo.BaseAddress + basicInfo.RegionSize)
                {
                    PPH_MEMORY_ITEM otherMemoryItem;

                    memoryItem->RegionSize = potentialUnusableSize;

                    otherMemoryItem = PhCreateMemoryItem();
                    otherMemoryItem->BasicInfo = basicInfo;
                    otherMemoryItem->BaseAddress = (PVOID)nextAllocationBase;
                    otherMemoryItem->AllocationBase = otherMemoryItem->BaseAddress;
                    otherMemoryItem->RegionSize = basicInfo.RegionSize - potentialUnusableSize;
                    otherMemoryItem->AllocationBaseItem = otherMemoryItem;

                    PhAddElementAvlTree(&List->Set, &otherMemoryItem->Links);
                    InsertTailList(&List->ListHead, &otherMemoryItem->ListEntry);

                    previousMemoryItem = otherMemoryItem;
                    goto ContinueLoop;
                }
            }
        }

        previousMemoryItem = memoryItem;

ContinueLoop:
        baseAddress = PTR_ADD_OFFSET(baseAddress, basicInfo.RegionSize);
    }

    if (Flags & PH_QUERY_MEMORY_REGION_TYPE)
        PhpUpdateMemoryRegionTypes(List, processHandle);

    if (Flags & PH_QUERY_MEMORY_WS_COUNTERS)
    {
        if (WindowsVersion >= WINDOWS_SERVER_2003)
            PhpUpdateMemoryWsCounters(List, processHandle);
        else
            PhpUpdateMemoryWsCountersOld(List, processHandle);
    }

    NtClose(processHandle);

    return STATUS_SUCCESS;
}
