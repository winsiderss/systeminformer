/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2017-2023
 *
 */

#include <phapp.h>
#include <memprv.h>
#include <heapstruct.h>
#include <settings.h>

#define MAX_HEAPS 1000
#define WS_REQUEST_COUNT (PAGE_SIZE / sizeof(MEMORY_WORKING_SET_EX_INFORMATION))

VOID PhpMemoryItemDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    );

PPH_OBJECT_TYPE PhMemoryItemType = NULL;

VOID PhGetMemoryProtectionString(
    _In_ ULONG Protection,
    _Inout_z_ PWSTR String
    )
{
    PWSTR string;
    PH_STRINGREF base;

    if (!Protection)
    {
        String[0] = UNICODE_NULL;
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
        memcpy(string, L"+G", 2 * sizeof(WCHAR));
        string += 2;
    }

    if (Protection & PAGE_NOCACHE)
    {
        memcpy(string, L"+NC", 3 * sizeof(WCHAR));
        string += 3;
    }

    if (Protection & PAGE_WRITECOMBINE)
    {
        memcpy(string, L"+WCM", 4 * sizeof(WCHAR));
        string += 4;
    }

    *string = UNICODE_NULL;
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

PPH_STRINGREF PhGetSigningLevelString(
    _In_ SE_SIGNING_LEVEL SigningLevel
    )
{
    static PH_STRINGREF SigningLevelString[] =
    {
        PH_STRINGREF_INIT(L"Unchecked"),
        PH_STRINGREF_INIT(L"Unsigned"),
        PH_STRINGREF_INIT(L"Enterprise"),
        PH_STRINGREF_INIT(L"Developer"),
        PH_STRINGREF_INIT(L"Authenticode"),
        PH_STRINGREF_INIT(L"Custom"),
        PH_STRINGREF_INIT(L"StoreApp"),
        PH_STRINGREF_INIT(L"Antimalware"),
        PH_STRINGREF_INIT(L"Microsoft"),
        PH_STRINGREF_INIT(L"Custom"),
        PH_STRINGREF_INIT(L"Custom"),
        PH_STRINGREF_INIT(L"CodeGen"),
        PH_STRINGREF_INIT(L"Windows"),
        PH_STRINGREF_INIT(L"Custom"),
        PH_STRINGREF_INIT(L"WinTcb"),
        PH_STRINGREF_INIT(L"Custom"),
    };

    static_assert(ARRAYSIZE(SigningLevelString) == SE_SIGNING_LEVEL_CUSTOM_6 + 1, "SigningLevelString must equal SE_SIGNING_LEVEL_MAX");

    switch (SigningLevel)
    {
    case SE_SIGNING_LEVEL_UNCHECKED:
    case SE_SIGNING_LEVEL_UNSIGNED:
    case SE_SIGNING_LEVEL_ENTERPRISE:
    case SE_SIGNING_LEVEL_DEVELOPER:
    case SE_SIGNING_LEVEL_AUTHENTICODE:
    case SE_SIGNING_LEVEL_CUSTOM_2:
    case SE_SIGNING_LEVEL_STORE:
    case SE_SIGNING_LEVEL_ANTIMALWARE:
    case SE_SIGNING_LEVEL_MICROSOFT:
    case SE_SIGNING_LEVEL_CUSTOM_4:
    case SE_SIGNING_LEVEL_CUSTOM_5:
    case SE_SIGNING_LEVEL_DYNAMIC_CODEGEN:
    case SE_SIGNING_LEVEL_WINDOWS:
    case SE_SIGNING_LEVEL_CUSTOM_7:
    case SE_SIGNING_LEVEL_WINDOWS_TCB:
    case SE_SIGNING_LEVEL_CUSTOM_6:
        return &SigningLevelString[SigningLevel];
    }

    assert(FALSE);
    return NULL;

    //switch (SigningLevel)
    //{
    //    case SE_SIGNING_LEVEL_UNCHECKED:
    //        return L"Unchecked";
    //    case SE_SIGNING_LEVEL_UNSIGNED:
    //        return L"Unsigned";
    //    case SE_SIGNING_LEVEL_ENTERPRISE:
    //        return L"Enterprise";
    //    case SE_SIGNING_LEVEL_DEVELOPER:
    //        return L"Developer";
    //    case SE_SIGNING_LEVEL_AUTHENTICODE:
    //        return L"Authenticode";
    //    case SE_SIGNING_LEVEL_STORE:
    //        return L"StoreApp";
    //    case SE_SIGNING_LEVEL_ANTIMALWARE:
    //        return L"Antimalware";
    //    case SE_SIGNING_LEVEL_MICROSOFT:
    //        return L"Microsoft";
    //    case SE_SIGNING_LEVEL_DYNAMIC_CODEGEN:
    //        return L"CodeGen";
    //    case SE_SIGNING_LEVEL_WINDOWS:
    //        return L"Windows";
    //    case SE_SIGNING_LEVEL_WINDOWS_TCB:
    //        return L"WinTcb";
    //    case SE_SIGNING_LEVEL_CUSTOM_2:
    //    case SE_SIGNING_LEVEL_CUSTOM_4:
    //    case SE_SIGNING_LEVEL_CUSTOM_5:
    //    case SE_SIGNING_LEVEL_CUSTOM_6:
    //    case SE_SIGNING_LEVEL_CUSTOM_7:
    //        return L"Custom";
    //    default:
    //        return L"";
    //}
}

PPH_STRING PhGetMemoryRegionTypeExString(
    _In_ PPH_MEMORY_ITEM MemoryItem
    )
{
    PH_STRING_BUILDER stringBuilder;
    WCHAR pointer[PH_PTR_STR_LEN_1];

    if (!MemoryItem->RegionTypeEx)
        return NULL;

    PhInitializeStringBuilder(&stringBuilder, 0x50);

    if (MemoryItem->Private)
        PhAppendStringBuilder2(&stringBuilder, L"Private, ");
    if (MemoryItem->MappedDataFile)
        PhAppendStringBuilder2(&stringBuilder, L"MappedDataFile, ");
    if (MemoryItem->MappedImage)
        PhAppendStringBuilder2(&stringBuilder, L"MappedImage, ");
    if (MemoryItem->MappedPageFile)
        PhAppendStringBuilder2(&stringBuilder, L"MappedPageFile, ");
    if (MemoryItem->MappedPhysical)
        PhAppendStringBuilder2(&stringBuilder, L"MappedPhysical, ");
    if (MemoryItem->DirectMapped)
        PhAppendStringBuilder2(&stringBuilder, L"DirectMapped, ");
    if (MemoryItem->SoftwareEnclave)
        PhAppendStringBuilder2(&stringBuilder, L"Software enclave, ");
    if (MemoryItem->PageSize64K)
        PhAppendStringBuilder2(&stringBuilder, L"PageSize64K, ");
    if (MemoryItem->PlaceholderReservation)
        PhAppendStringBuilder2(&stringBuilder, L"Placeholder, ");
    if (MemoryItem->MappedAwe)
        PhAppendStringBuilder2(&stringBuilder, L"Mapped AWE, ");
    if (MemoryItem->MappedWriteWatch)
        PhAppendStringBuilder2(&stringBuilder, L"MappedWriteWatch, ");
    if (MemoryItem->PageSizeLarge)
        PhAppendStringBuilder2(&stringBuilder, L"PageSizeLarge, ");
    if (MemoryItem->PageSizeHuge)
        PhAppendStringBuilder2(&stringBuilder, L"PageSizeHuge, ");

    if (PhEndsWithString2(stringBuilder.String, L", ", FALSE))
        PhRemoveEndStringBuilder(&stringBuilder, 2);

    PhPrintPointer(pointer, UlongToPtr(MemoryItem->RegionTypeEx));
    PhAppendFormatStringBuilder(&stringBuilder, L" (%s)", pointer);

    return PhFinalStringBuilderString(&stringBuilder);
}

_Ret_notnull_
PPH_MEMORY_ITEM PhCreateMemoryItem(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    PPH_MEMORY_ITEM memoryItem;

    if (PhBeginInitOnce(&initOnce))
    {
        PhMemoryItemType = PhCreateObjectType(L"MemoryItem", 0, PhpMemoryItemDeleteProcedure);
        PhEndInitOnce(&initOnce);
    }

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

    // Do an approximate search on the set to locate the memory item with the largest
    // base address that is still smaller than the given address.
    lookupMemoryItem.BaseAddress = Address;
    links = PhUpperDualBoundElementAvlTree(&List->Set, &lookupMemoryItem.Links);

    if (links)
    {
        memoryItem = CONTAINING_RECORD(links, PH_MEMORY_ITEM, Links);

        if ((ULONG_PTR)Address < (ULONG_PTR)memoryItem->BaseAddress + memoryItem->RegionSize)
            return memoryItem;
    }

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

VOID PhpUpdateHeapRegions(
    _In_ PPH_MEMORY_ITEM_LIST List
    )
{
    NTSTATUS status;
    HANDLE processHandle = NULL;
    HANDLE clientProcessId = List->ProcessId;
    PROCESS_REFLECTION_INFORMATION reflectionInfo = { 0 };
    PRTL_DEBUG_INFORMATION debugBuffer = NULL;
    PPH_PROCESS_DEBUG_HEAP_INFORMATION heapDebugInfo = NULL;
    HANDLE powerRequestHandle = NULL;

    status = PhOpenProcess(
        &processHandle,
        PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_DUP_HANDLE | PROCESS_SET_LIMITED_INFORMATION,
        List->ProcessId
        );

    if (NT_SUCCESS(status))
    {
        if (WindowsVersion >= WINDOWS_10)
            PhCreateExecutionRequiredRequest(processHandle, &powerRequestHandle);

        if (PhGetIntegerSetting(L"EnableHeapReflection"))
        {
            status = PhCreateProcessReflection(
                &reflectionInfo,
                processHandle,
                List->ProcessId
                );

            if (NT_SUCCESS(status))
            {
                clientProcessId = reflectionInfo.ReflectionClientId.UniqueProcess;
            }
        }
    }

    for (ULONG i = 0x10000; ; i *= 2) // based on PhQueryProcessHeapInformation (ge0rdi)
    {
        if (!(debugBuffer = RtlCreateQueryDebugBuffer(i, FALSE)))
        {
            status = STATUS_UNSUCCESSFUL;
            break;
        }

        status = RtlQueryProcessDebugInformation(
            clientProcessId,
            RTL_QUERY_PROCESS_HEAP_SUMMARY | RTL_QUERY_PROCESS_HEAP_SEGMENTS | RTL_QUERY_PROCESS_NONINVASIVE,
            debugBuffer
            );

        if (!NT_SUCCESS(status))
        {
            RtlDestroyQueryDebugBuffer(debugBuffer);
            debugBuffer = NULL;
        }

        if (NT_SUCCESS(status) || status != STATUS_NO_MEMORY)
            break;

        if (2 * i <= i)
            break;
    }

    PhFreeProcessReflection(&reflectionInfo);

    if (processHandle)
        NtClose(processHandle);

    if (powerRequestHandle)
        PhDestroyExecutionRequiredRequest(powerRequestHandle);

    if (!NT_SUCCESS(status))
        return;

    if (!debugBuffer->Heaps)
    {
        RtlDestroyQueryDebugBuffer(debugBuffer);
        return;
    }

    if (WindowsVersion > WINDOWS_11)
    {
        heapDebugInfo = PhAllocateZero(sizeof(PH_PROCESS_DEBUG_HEAP_INFORMATION) + ((PRTL_PROCESS_HEAPS_V2)debugBuffer->Heaps)->NumberOfHeaps * sizeof(PH_PROCESS_DEBUG_HEAP_ENTRY));
        heapDebugInfo->NumberOfHeaps = ((PRTL_PROCESS_HEAPS_V2)debugBuffer->Heaps)->NumberOfHeaps;
    }
    else
    {
        heapDebugInfo = PhAllocateZero(sizeof(PH_PROCESS_DEBUG_HEAP_INFORMATION) + ((PRTL_PROCESS_HEAPS_V1)debugBuffer->Heaps)->NumberOfHeaps * sizeof(PH_PROCESS_DEBUG_HEAP_ENTRY));
        heapDebugInfo->NumberOfHeaps = ((PRTL_PROCESS_HEAPS_V1)debugBuffer->Heaps)->NumberOfHeaps;
    }

    for (ULONG i = 0; i < heapDebugInfo->NumberOfHeaps; i++)
    {
        RTL_HEAP_INFORMATION_V2 heapInfo = { 0 };
        PPH_MEMORY_ITEM heapMemoryItem;

        if (WindowsVersion > WINDOWS_11)
        {
            heapInfo = ((PRTL_PROCESS_HEAPS_V2)debugBuffer->Heaps)->Heaps[i];
        }
        else
        {
            RTL_HEAP_INFORMATION_V1 heapInfoV1 = ((PRTL_PROCESS_HEAPS_V1)debugBuffer->Heaps)->Heaps[i];
            heapInfo.NumberOfEntries = heapInfoV1.NumberOfEntries;
            heapInfo.Entries = heapInfoV1.Entries;
            heapInfo.BytesCommitted = heapInfoV1.BytesCommitted;
            heapInfo.Flags = heapInfoV1.Flags;
            heapInfo.BaseAddress = heapInfoV1.BaseAddress;
        }

        if (!heapInfo.BaseAddress)
            continue;

        if (heapMemoryItem = PhpSetMemoryRegionType(List, heapInfo.BaseAddress, TRUE, HeapRegion))
        {
            heapMemoryItem->u.Heap.Index = i;

            for (ULONG j = 0; j < heapInfo.NumberOfEntries; j++)
            {
                PRTL_HEAP_ENTRY heapEntry = &heapInfo.Entries[j];

                if (heapEntry->Flags & RTL_HEAP_SEGMENT)
                {
                    PVOID blockAddress = heapEntry->u.s2.FirstBlock;

                    PPH_MEMORY_ITEM memoryItem = PhLookupMemoryItemList(List, blockAddress);

                    if (memoryItem && memoryItem->BaseAddress == blockAddress && memoryItem->RegionType == UnknownRegion)
                    {
                        memoryItem->RegionType = HeapSegmentRegion;
                        memoryItem->u.HeapSegment.HeapItem = heapMemoryItem;
                    }
                }
            }
        }
    }

    RtlDestroyQueryDebugBuffer(debugBuffer);
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

    // HYPERVISOR_SHARED_DATA
    if (WindowsVersion >= WINDOWS_10_RS4)
    {
        static PVOID HypervisorSharedDataVa = NULL;
        static PH_INITONCE HypervisorSharedDataInitOnce = PH_INITONCE_INIT;

        if (PhBeginInitOnce(&HypervisorSharedDataInitOnce))
        {
            SYSTEM_HYPERVISOR_SHARED_PAGE_INFORMATION hypervSharedPageInfo;

            if (NT_SUCCESS(NtQuerySystemInformation(
                SystemHypervisorSharedPageInformation,
                &hypervSharedPageInfo,
                sizeof(SYSTEM_HYPERVISOR_SHARED_PAGE_INFORMATION),
                NULL
                )))
            {
                HypervisorSharedDataVa = hypervSharedPageInfo.HypervisorSharedUserVa;
            }

            PhEndInitOnce(&HypervisorSharedDataInitOnce);
        }

        if (HypervisorSharedDataVa)
        {
            PhpSetMemoryRegionType(List, HypervisorSharedDataVa, TRUE, HypervisorSharedDataRegion);
        }
    }

    // PEB, heap
    {
        ULONG numberOfHeaps;
        PVOID processPeb;
        PVOID processHeapsPtr;
        PVOID *processHeaps;
        PVOID apiSetMap;
        PVOID readOnlySharedMemory;
        PVOID codePageData;
        PVOID gdiSharedHandleTable;
        PVOID shimData;
        PVOID activationContextData;
        PVOID defaultActivationContextData;
#ifdef _WIN64
        PVOID processPeb32;
        ULONG processHeapsPtr32;
        ULONG *processHeaps32;
        ULONG apiSetMap32;
        ULONG readOnlySharedMemory32;
        ULONG codePageData32;
        ULONG gdiSharedHandleTable32;
        ULONG shimData32;
        ULONG activationContextData32;
        ULONG defaultActivationContextData32;
#endif
        if (PhGetIntegerSetting(L"EnableHeapMemoryTagging"))
        {
            PhpUpdateHeapRegions(List);
        }

        if (NT_SUCCESS(PhGetProcessPeb(ProcessHandle, &processPeb)))
        {
            // HACK: Windows 10 RS2 and above 'added TEB/PEB sub-VAD segments' and we need to tag individual sections. (dmex)
            PhpSetMemoryRegionType(List, processPeb, WindowsVersion < WINDOWS_10_RS2 ? TRUE : FALSE, PebRegion);

            if (NT_SUCCESS(NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(processPeb, UFIELD_OFFSET(PEB, NumberOfHeaps)),
                &numberOfHeaps,
                sizeof(ULONG),
                NULL
                )) && numberOfHeaps > 0 && numberOfHeaps < MAX_HEAPS)
            {
                processHeaps = PhAllocate(numberOfHeaps * sizeof(PVOID));

                if (
                    NT_SUCCESS(NtReadVirtualMemory(ProcessHandle, PTR_ADD_OFFSET(processPeb, UFIELD_OFFSET(PEB, ProcessHeaps)), &processHeapsPtr, sizeof(PVOID), NULL)) &&
                    NT_SUCCESS(NtReadVirtualMemory(ProcessHandle, processHeapsPtr, processHeaps, numberOfHeaps * sizeof(PVOID), NULL))
                    )
                {
                    for (i = 0; i < numberOfHeaps; i++)
                    {
                        if (memoryItem = PhpSetMemoryRegionType(List, processHeaps[i], TRUE, HeapRegion))
                        {
                            PH_ANY_HEAP buffer;

                            memoryItem->u.Heap.Index = i;

                            // Try to read heap class from the header
                            if (NT_SUCCESS(NtReadVirtualMemory(ProcessHandle, processHeaps[i], &buffer, sizeof(buffer), NULL)))
                            {
                                if (WindowsVersion >= WINDOWS_8)
                                {
                                    if (buffer.Heap.Signature == HEAP_SIGNATURE)
                                    {
                                        memoryItem->u.Heap.ClassValid = TRUE;
                                        memoryItem->u.Heap.Class = buffer.Heap.Flags & HEAP_CLASS_MASK;
                                    }
                                    else if (buffer.Heap32.Signature == HEAP_SIGNATURE)
                                    {
                                        memoryItem->u.Heap.ClassValid = TRUE;
                                        memoryItem->u.Heap.Class = buffer.Heap32.Flags & HEAP_CLASS_MASK;
                                    }
                                    else if (WindowsVersion >= WINDOWS_8_1)
                                    {
                                        // Windows 8.1 and above can use segment heaps
                                        if (buffer.SegmentHeap.Signature == SEGMENT_HEAP_SIGNATURE)
                                        {
                                            memoryItem->u.Heap.ClassValid = TRUE;
                                            memoryItem->u.Heap.Class = buffer.SegmentHeap.GlobalFlags & HEAP_CLASS_MASK;
                                        }
                                        else if (buffer.SegmentHeap32.Signature == SEGMENT_HEAP_SIGNATURE)
                                        {
                                            memoryItem->u.Heap.ClassValid = TRUE;
                                            memoryItem->u.Heap.Class = buffer.SegmentHeap32.GlobalFlags & HEAP_CLASS_MASK;
                                        }
                                    }
                                }
                                else
                                {
                                    if (buffer.HeapOld.Signature == HEAP_SIGNATURE)
                                    {
                                        memoryItem->u.Heap.ClassValid = TRUE;
                                        memoryItem->u.Heap.Class = buffer.HeapOld.Flags & HEAP_CLASS_MASK;
                                    }
                                    else if (buffer.HeapOld32.Signature == HEAP_SIGNATURE)
                                    {
                                        memoryItem->u.Heap.ClassValid = TRUE;
                                        memoryItem->u.Heap.Class = buffer.HeapOld32.Flags & HEAP_CLASS_MASK;
                                    }
                                }
                            }
                        }
                    }
                }

                PhFree(processHeaps);
            }

            // ApiSet schema map
            if (NT_SUCCESS(NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(processPeb, FIELD_OFFSET(PEB, ApiSetMap)),
                &apiSetMap,
                sizeof(PVOID),
                NULL
                )) && apiSetMap)
            {
                PhpSetMemoryRegionType(List, apiSetMap, TRUE, ApiSetMapRegion);
            }

            // CSR shared memory
            if (NT_SUCCESS(NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(processPeb, FIELD_OFFSET(PEB, ReadOnlySharedMemoryBase)),
                &readOnlySharedMemory,
                sizeof(PVOID),
                NULL
                )) && readOnlySharedMemory)
            {
                PhpSetMemoryRegionType(List, readOnlySharedMemory, TRUE, ReadOnlySharedMemoryRegion);
            }

            // CodePage data
            if (NT_SUCCESS(NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(processPeb, FIELD_OFFSET(PEB, AnsiCodePageData)),
                &codePageData,
                sizeof(PVOID),
                NULL
                )) && codePageData)
            {
                PhpSetMemoryRegionType(List, codePageData, TRUE, CodePageDataRegion);
            }

            // GDI shared handle table
            if (NT_SUCCESS(NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(processPeb, FIELD_OFFSET(PEB, GdiSharedHandleTable)),
                &gdiSharedHandleTable,
                sizeof(PVOID),
                NULL
                )) && gdiSharedHandleTable)
            {
                PhpSetMemoryRegionType(List, gdiSharedHandleTable, TRUE, GdiSharedHandleTableRegion);
            }

            // Shim data
            if (NT_SUCCESS(NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(processPeb, FIELD_OFFSET(PEB, pShimData)),
                &shimData,
                sizeof(PVOID),
                NULL
                )) && shimData)
            {
                PhpSetMemoryRegionType(List, shimData, TRUE, ShimDataRegion);
            }

            // Activation context data
            if (NT_SUCCESS(NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(processPeb, FIELD_OFFSET(PEB, ActivationContextData)),
                &activationContextData,
                sizeof(PVOID),
                NULL
                )) && activationContextData)
            {
                PhpSetMemoryRegionType(List, activationContextData, TRUE, ActivationContextDataRegion);
            }

            // Default activation context data
            if (NT_SUCCESS(NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(processPeb, FIELD_OFFSET(PEB, SystemDefaultActivationContextData)),
                &defaultActivationContextData,
                sizeof(PVOID),
                NULL
                )) && defaultActivationContextData)
            {
                PhpSetMemoryRegionType(List, defaultActivationContextData, TRUE, SystemDefaultActivationContextDataRegion);
            }
        }
#ifdef _WIN64

        if (NT_SUCCESS(PhGetProcessPeb32(ProcessHandle, &processPeb32)))
        {
            isWow64 = TRUE;
            PhpSetMemoryRegionType(List, processPeb32, TRUE, Peb32Region);

            if (NT_SUCCESS(NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(processPeb32, UFIELD_OFFSET(PEB32, NumberOfHeaps)),
                &numberOfHeaps,
                sizeof(ULONG),
                NULL
                )) && numberOfHeaps < MAX_HEAPS)
            {
                processHeaps32 = PhAllocate(numberOfHeaps * sizeof(ULONG));

                if (
                    NT_SUCCESS(NtReadVirtualMemory(ProcessHandle, PTR_ADD_OFFSET(processPeb32, UFIELD_OFFSET(PEB32, ProcessHeaps)), &processHeapsPtr32, sizeof(ULONG), NULL)) &&
                    NT_SUCCESS(NtReadVirtualMemory(ProcessHandle, UlongToPtr(processHeapsPtr32), processHeaps32, numberOfHeaps * sizeof(ULONG), NULL))
                    )
                {
                    for (i = 0; i < numberOfHeaps; i++)
                    {
                        if (memoryItem = PhpSetMemoryRegionType(List, UlongToPtr(processHeaps32[i]), TRUE, Heap32Region))
                            memoryItem->u.Heap.Index = i;
                    }
                }

                PhFree(processHeaps32);
            }

            // ApiSet schema map
            if (NT_SUCCESS(NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(processPeb32, UFIELD_OFFSET(PEB32, ApiSetMap)),
                &apiSetMap32,
                sizeof(ULONG),
                NULL
                )) && apiSetMap32)
            {
                PhpSetMemoryRegionType(List, UlongToPtr(apiSetMap32), TRUE, ApiSetMapRegion);
            }

            // CSR shared memory
            if (NT_SUCCESS(NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(processPeb32, UFIELD_OFFSET(PEB32, ReadOnlySharedMemoryBase)),
                &readOnlySharedMemory32,
                sizeof(ULONG),
                NULL
                )) && readOnlySharedMemory32)
            {
                PhpSetMemoryRegionType(List, UlongToPtr(readOnlySharedMemory32), TRUE, ReadOnlySharedMemoryRegion);
            }

            // CodePage data
            if (NT_SUCCESS(NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(processPeb32, UFIELD_OFFSET(PEB32, AnsiCodePageData)),
                &codePageData32,
                sizeof(ULONG),
                NULL
                )) && codePageData32)
            {
                PhpSetMemoryRegionType(List, UlongToPtr(codePageData32), TRUE, CodePageDataRegion);
            }

            // GDI shared handle table
            if (NT_SUCCESS(NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(processPeb32, UFIELD_OFFSET(PEB32, GdiSharedHandleTable)),
                &gdiSharedHandleTable32,
                sizeof(ULONG),
                NULL
                )) && gdiSharedHandleTable32)
            {
                PhpSetMemoryRegionType(List, UlongToPtr(gdiSharedHandleTable32), TRUE, GdiSharedHandleTableRegion);
            }

            // Shim data
            if (NT_SUCCESS(NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(processPeb32, UFIELD_OFFSET(PEB32, pShimData)),
                &shimData32,
                sizeof(ULONG),
                NULL
                )) && shimData32)
            {
                PhpSetMemoryRegionType(List, UlongToPtr(shimData32), TRUE, ShimDataRegion);
            }

            // Activation context data
            if (NT_SUCCESS(NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(processPeb32, UFIELD_OFFSET(PEB32, ActivationContextData)),
                &activationContextData32,
                sizeof(ULONG),
                NULL
                )) && activationContextData32)
            {
                PhpSetMemoryRegionType(List, UlongToPtr(activationContextData32), TRUE, ActivationContextDataRegion);
            }

            // Default activation context data
            if (NT_SUCCESS(NtReadVirtualMemory(
                ProcessHandle,
                PTR_ADD_OFFSET(processPeb32, UFIELD_OFFSET(PEB32, SystemDefaultActivationContextData)),
                &defaultActivationContextData32,
                sizeof(ULONG),
                NULL
                )) && defaultActivationContextData32)
            {
                PhpSetMemoryRegionType(List, UlongToPtr(defaultActivationContextData32), TRUE, SystemDefaultActivationContextDataRegion);
            }
        }
#endif
    }

    // TEB, stack
    for (i = 0; i < process->NumberOfThreads; i++)
    {
        PSYSTEM_EXTENDED_THREAD_INFORMATION thread = (PSYSTEM_EXTENDED_THREAD_INFORMATION)process->Threads + i;

        if (thread->TebBase)
        {
            NT_TIB ntTib;
            SIZE_T bytesRead;

            // HACK: Windows 10 RS2 and above 'added TEB/PEB sub-VAD segments' and we need to tag individual sections.
            if (memoryItem = PhpSetMemoryRegionType(List, thread->TebBase, WindowsVersion < WINDOWS_10_RS2 ? TRUE : FALSE, TebRegion))
                memoryItem->u.Teb.ThreadId = thread->ThreadInfo.ClientId.UniqueThread;

            if (NT_SUCCESS(NtReadVirtualMemory(ProcessHandle, thread->TebBase, &ntTib, sizeof(NT_TIB), &bytesRead)) &&
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

                    if (NT_SUCCESS(NtReadVirtualMemory(ProcessHandle, UlongToPtr(teb32), &ntTib32, sizeof(NT_TIB32), &bytesRead)) &&
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
            MEMORY_IMAGE_INFORMATION imageInfo;
            PPH_STRING fileName;

            if (NT_SUCCESS(PhGetProcessMappedImageInformation(ProcessHandle, memoryItem->BaseAddress, &imageInfo)))
            {
                memoryItem->u.MappedFile.SigningLevelValid = TRUE;
                memoryItem->u.MappedFile.SigningLevel = (SE_SIGNING_LEVEL)imageInfo.ImageSigningLevel;
            }

            if (NT_SUCCESS(PhGetProcessMappedFileName(ProcessHandle, memoryItem->BaseAddress, &fileName)))
            {
                PPH_STRING newFileName = PhResolveDevicePrefix(&fileName->sr);

                if (newFileName)
                    PhMoveReference(&fileName, newFileName);

                memoryItem->RegionType = MappedFileRegion;
                memoryItem->u.MappedFile.FileName = fileName;
                continue;
            }
        }

        if (memoryItem->State & MEM_COMMIT && memoryItem->Valid && !memoryItem->Bad)
        {
            UCHAR buffer[HEAP_SEGMENT_MAX_SIZE];

            if (NT_SUCCESS(NtReadVirtualMemory(ProcessHandle, memoryItem->BaseAddress, buffer, sizeof(buffer), NULL)))
            {
                PVOID candidateHeap = NULL;
                ULONG candidateHeap32 = 0;
                PPH_MEMORY_ITEM heapMemoryItem;
                PHEAP_SEGMENT heapSegment = (PHEAP_SEGMENT)buffer;
                PHEAP_SEGMENT32 heapSegment32 = (PHEAP_SEGMENT32)buffer;

                if (heapSegment->SegmentSignature == HEAP_SEGMENT_SIGNATURE)
                    candidateHeap = heapSegment->Heap;
                if (heapSegment32->SegmentSignature == HEAP_SEGMENT_SIGNATURE)
                    candidateHeap32 = heapSegment32->Heap;

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

#ifdef _WIN64

    PPS_SYSTEM_DLL_INIT_BLOCK ldrInitBlock;
    PPH_MEMORY_ITEM cfgBitmapMemoryItem;

    status = PhGetProcessSystemDllInitBlock(ProcessHandle, &ldrInitBlock);

    if (NT_SUCCESS(status))
    {
        PVOID cfgBitmapAddress = NULL;
        PVOID cfgBitmapWow64Address = NULL;

        if (RTL_CONTAINS_FIELD(ldrInitBlock, ldrInitBlock->Size, Wow64CfgBitMap))
        {
            cfgBitmapAddress = (PVOID)ldrInitBlock->CfgBitMap;
            cfgBitmapWow64Address = (PVOID)ldrInitBlock->Wow64CfgBitMap;
        }

        if (cfgBitmapAddress && (cfgBitmapMemoryItem = PhLookupMemoryItemList(List, cfgBitmapAddress)))
        {
            listEntry = &cfgBitmapMemoryItem->ListEntry;
            memoryItem = CONTAINING_RECORD(listEntry, PH_MEMORY_ITEM, ListEntry);

            while (memoryItem->AllocationBaseItem == cfgBitmapMemoryItem)
            {
                // lucasg: We could do a finer tagging since each MEM_COMMIT memory
                // map is the CFG bitmap of a loaded module. However that might be
                // brittle to changes made by Windows dev teams.
                memoryItem->RegionType = CfgBitmapRegion;

                listEntry = listEntry->Flink;
                memoryItem = CONTAINING_RECORD(listEntry, PH_MEMORY_ITEM, ListEntry);
            }
        }

        // Note: Wow64 processes on 64bit also have CfgBitmap regions.
        if (isWow64 && cfgBitmapWow64Address && (cfgBitmapMemoryItem = PhLookupMemoryItemList(List, cfgBitmapWow64Address)))
        {
            listEntry = &cfgBitmapMemoryItem->ListEntry;
            memoryItem = CONTAINING_RECORD(listEntry, PH_MEMORY_ITEM, ListEntry);

            while (memoryItem->AllocationBaseItem == cfgBitmapMemoryItem)
            {
                memoryItem->RegionType = CfgBitmap32Region;

                listEntry = listEntry->Flink;
                memoryItem = CONTAINING_RECORD(listEntry, PH_MEMORY_ITEM, ListEntry);
            }
        }

        PhFree(ldrInitBlock);
    }
#endif

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

                        if (memoryItem->Type & (MEM_MAPPED | MEM_IMAGE) && block->SharedOriginal)
                            memoryItem->SharedOriginalPages++;
                        if (block->Priority > memoryItem->Priority)
                            memoryItem->Priority = block->Priority;
                    }
                    else
                    {
                        if (memoryItem->Type & (MEM_MAPPED | MEM_IMAGE) && block->Invalid.SharedOriginal)
                            memoryItem->SharedOriginalPages++;

                        // VMMap does this, but is it correct? (dmex)
                        if (block->Invalid.Shared)
                            memoryItem->ShareableWorkingSetPages++;
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
            PROCESS_QUERY_LIMITED_INFORMATION,
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

        if (Flags & PH_QUERY_MEMORY_ZERO_PAD_ADDRESSES)
            PhPrintPointerPadZeros(memoryItem->BaseAddressString, memoryItem->BasicInfo.BaseAddress);
        else
            PhPrintPointer(memoryItem->BaseAddressString, memoryItem->BasicInfo.BaseAddress);

        if (basicInfo.State & MEM_COMMIT)
        {
            memoryItem->CommittedSize = memoryItem->RegionSize;

            if (basicInfo.Type & MEM_PRIVATE)
                memoryItem->PrivateSize = memoryItem->RegionSize;
        }

        if (!FlagOn(basicInfo.State, MEM_FREE))
        {
            MEMORY_WORKING_SET_EX_INFORMATION pageInfo;

            static_assert(HEAP_SEGMENT_MAX_SIZE < PAGE_SIZE, "Update query attributes for additional pages");

            // Query the attributes for the first page (dmex)

            memset(&pageInfo, 0, sizeof(MEMORY_WORKING_SET_EX_INFORMATION));
            pageInfo.VirtualAddress = baseAddress;

            if (NT_SUCCESS(NtQueryVirtualMemory(
                processHandle,
                NULL,
                MemoryWorkingSetExInformation,
                &pageInfo,
                sizeof(MEMORY_WORKING_SET_EX_INFORMATION),
                NULL
                )))
            {
                PMEMORY_WORKING_SET_EX_BLOCK block = &pageInfo.u1.VirtualAttributes;

                memoryItem->Valid = !!block->Valid;
                memoryItem->Bad = !!block->Bad;
            }

            if (WindowsVersion > WINDOWS_8)
            {
                MEMORY_REGION_INFORMATION regionInfo;

                // Query the region (dmex)

                memset(&regionInfo, 0, sizeof(MEMORY_REGION_INFORMATION));

                if (NT_SUCCESS(NtQueryVirtualMemory(
                    processHandle,
                    baseAddress,
                    MemoryRegionInformationEx,
                    &regionInfo,
                    sizeof(MEMORY_REGION_INFORMATION),
                    NULL
                    )))
                {
                    memoryItem->RegionTypeEx = regionInfo.RegionType;
                    //memoryItem->RegionSize = regionInfo.RegionSize;
                    //memoryItem->CommittedSize = regionInfo.CommitSize;
                }
            }
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

                    if (Flags & PH_QUERY_MEMORY_ZERO_PAD_ADDRESSES)
                        PhPrintPointerPadZeros(otherMemoryItem->BaseAddressString, otherMemoryItem->BasicInfo.BaseAddress);
                    else
                        PhPrintPointer(otherMemoryItem->BaseAddressString, otherMemoryItem->BasicInfo.BaseAddress);

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
        PhpUpdateMemoryWsCounters(List, processHandle);

    NtClose(processHandle);

    return STATUS_SUCCESS;
}
