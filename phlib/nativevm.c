/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *     dmex    2017-2023
 *
 */

#include <ph.h>
#include <apiimport.h>

// rev from PrefetchVirtualMemory (dmex)
/**
 * Provides an efficient mechanism to bring into memory potentially discontiguous virtual address ranges in a process address space.
 *
 * \param ProcessHandle A handle to the process whose virtual address ranges are to be prefetched.
 * \param NumberOfEntries Number of entries in the array pointed to by the VirtualAddresses parameter.
 * \param VirtualAddresses A pointer to an array of MEMORY_RANGE_ENTRY structures which each specify a virtual address range
 * to be prefetched. The virtual address ranges may cover any part of the process address space accessible by the target process.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhPrefetchVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG_PTR NumberOfEntries,
    _In_ PMEMORY_RANGE_ENTRY VirtualAddresses
    )
{
    NTSTATUS status;
    MEMORY_PREFETCH_INFORMATION prefetchInformationFlags;

    if (!NtSetInformationVirtualMemory_Import())
        return STATUS_PROCEDURE_NOT_FOUND;

    memset(&prefetchInformationFlags, 0, sizeof(prefetchInformationFlags));

    status = NtSetInformationVirtualMemory_Import()(
        ProcessHandle,
        VmPrefetchInformation,
        NumberOfEntries,
        VirtualAddresses,
        &prefetchInformationFlags,
        sizeof(prefetchInformationFlags)
        );

    return status;
}

// rev from OfferVirtualMemory (dmex)
NTSTATUS PhOfferVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID VirtualAddress,
    _In_ SIZE_T NumberOfBytes,
    _In_ OFFER_PRIORITY Priority
    )
{
    NTSTATUS status;
    MEMORY_RANGE_ENTRY virtualMemoryRange;
    ULONG virtualMemoryFlags;

    if (!NtSetInformationVirtualMemory_Import())
        return STATUS_PROCEDURE_NOT_FOUND;

    // TODO: NtQueryVirtualMemory (dmex)

    memset(&virtualMemoryRange, 0, sizeof(MEMORY_RANGE_ENTRY));
    virtualMemoryRange.VirtualAddress = VirtualAddress;
    virtualMemoryRange.NumberOfBytes = NumberOfBytes;

    memset(&virtualMemoryFlags, 0, sizeof(virtualMemoryFlags));
    virtualMemoryFlags = Priority;

    status = NtSetInformationVirtualMemory_Import()(
        ProcessHandle,
        VmPagePriorityInformation,
        1,
        &virtualMemoryRange,
        &virtualMemoryFlags,
        sizeof(virtualMemoryFlags)
        );

    return status;
}

// rev from DiscardVirtualMemory (dmex)
NTSTATUS PhDiscardVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID VirtualAddress,
    _In_ SIZE_T NumberOfBytes
    )
{
    NTSTATUS status;
    MEMORY_RANGE_ENTRY virtualMemoryRange;
    ULONG virtualMemoryFlags;

    if (!NtSetInformationVirtualMemory_Import())
        return STATUS_PROCEDURE_NOT_FOUND;

    memset(&virtualMemoryRange, 0, sizeof(MEMORY_RANGE_ENTRY));
    virtualMemoryRange.VirtualAddress = VirtualAddress;
    virtualMemoryRange.NumberOfBytes = NumberOfBytes;

    memset(&virtualMemoryFlags, 0, sizeof(virtualMemoryFlags));

    status = NtSetInformationVirtualMemory_Import()(
        ProcessHandle,
        VmPagePriorityInformation,
        1,
        &virtualMemoryRange,
        &virtualMemoryFlags,
        sizeof(virtualMemoryFlags)
        );

    return status;
}

// rev from SetProcessValidCallTargets (dmex)
NTSTATUS PhSetProcessValidCallTarget(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID VirtualAddress
    )
{
    NTSTATUS status;
    MEMORY_BASIC_INFORMATION basicInfo;
    MEMORY_RANGE_ENTRY cfgCallTargetRangeInfo;
    CFG_CALL_TARGET_INFO cfgCallTargetInfo;
    CFG_CALL_TARGET_LIST_INFORMATION cfgCallTargetListInfo;
    ULONG numberOfEntriesProcessed = 0;

    if (!NtSetInformationVirtualMemory_Import())
        return STATUS_PROCEDURE_NOT_FOUND;

    status = NtQueryVirtualMemory(
        ProcessHandle,
        VirtualAddress,
        MemoryBasicInformation,
        &basicInfo,
        sizeof(MEMORY_BASIC_INFORMATION),
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    memset(&cfgCallTargetInfo, 0, sizeof(CFG_CALL_TARGET_INFO));
    cfgCallTargetInfo.Offset = (ULONG_PTR)VirtualAddress - (ULONG_PTR)basicInfo.AllocationBase;
    cfgCallTargetInfo.Flags = CFG_CALL_TARGET_VALID;

    memset(&cfgCallTargetRangeInfo, 0, sizeof(MEMORY_RANGE_ENTRY));
    cfgCallTargetRangeInfo.VirtualAddress = basicInfo.AllocationBase;
    cfgCallTargetRangeInfo.NumberOfBytes = basicInfo.RegionSize;

    memset(&cfgCallTargetListInfo, 0, sizeof(CFG_CALL_TARGET_LIST_INFORMATION));
    cfgCallTargetListInfo.NumberOfEntries = 1;
    cfgCallTargetListInfo.Reserved = 0;
    cfgCallTargetListInfo.NumberOfEntriesProcessed = &numberOfEntriesProcessed;
    cfgCallTargetListInfo.CallTargetInfo = &cfgCallTargetInfo;

    status = NtSetInformationVirtualMemory_Import()(
        ProcessHandle,
        VmCfgCallTargetInformation,
        1,
        &cfgCallTargetRangeInfo,
        &cfgCallTargetListInfo,
        sizeof(CFG_CALL_TARGET_LIST_INFORMATION)
        );

    if (status == STATUS_INVALID_PAGE_PROTECTION)
        status = STATUS_SUCCESS;

    return status;
}

// rev from RtlGuardGrantSuppressedCallAccess (dmex)
NTSTATUS PhGuardGrantSuppressedCallAccess(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID VirtualAddress
    )
{
    NTSTATUS status;
    MEMORY_RANGE_ENTRY cfgCallTargetRangeInfo;
    CFG_CALL_TARGET_INFO cfgCallTargetInfo;
    CFG_CALL_TARGET_LIST_INFORMATION cfgCallTargetListInfo;
    ULONG numberOfEntriesProcessed = 0;

    if (!NtSetInformationVirtualMemory_Import())
        return STATUS_PROCEDURE_NOT_FOUND;

    memset(&cfgCallTargetRangeInfo, 0, sizeof(MEMORY_RANGE_ENTRY));
    cfgCallTargetRangeInfo.VirtualAddress = PAGE_ALIGN(VirtualAddress);
    cfgCallTargetRangeInfo.NumberOfBytes = PAGE_SIZE;

    memset(&cfgCallTargetInfo, 0, sizeof(CFG_CALL_TARGET_INFO));
    cfgCallTargetInfo.Offset = BYTE_OFFSET(VirtualAddress);
    cfgCallTargetInfo.Flags = CFG_CALL_TARGET_VALID;

    memset(&cfgCallTargetListInfo, 0, sizeof(CFG_CALL_TARGET_LIST_INFORMATION));
    cfgCallTargetListInfo.NumberOfEntries = 1;
    cfgCallTargetListInfo.Reserved = 0;
    cfgCallTargetListInfo.NumberOfEntriesProcessed = &numberOfEntriesProcessed;
    cfgCallTargetListInfo.CallTargetInfo = &cfgCallTargetInfo;

    status = NtSetInformationVirtualMemory_Import()(
        ProcessHandle,
        VmCfgCallTargetInformation,
        1,
        &cfgCallTargetRangeInfo,
        &cfgCallTargetListInfo,
        sizeof(CFG_CALL_TARGET_LIST_INFORMATION)
        );

    if (status == STATUS_INVALID_PAGE_PROTECTION)
        status = STATUS_SUCCESS;

    return status;
}

// rev from RtlDisableXfgOnTarget (dmex)
NTSTATUS PhDisableXfgOnTarget(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID VirtualAddress
    )
{
    NTSTATUS status;
    MEMORY_RANGE_ENTRY cfgCallTargetRangeInfo;
    CFG_CALL_TARGET_INFO cfgCallTargetInfo;
    CFG_CALL_TARGET_LIST_INFORMATION cfgCallTargetListInfo;
    ULONG numberOfEntriesProcessed = 0;

    if (!NtSetInformationVirtualMemory_Import())
        return STATUS_PROCEDURE_NOT_FOUND;

    memset(&cfgCallTargetRangeInfo, 0, sizeof(MEMORY_RANGE_ENTRY));
    cfgCallTargetRangeInfo.VirtualAddress = PAGE_ALIGN(VirtualAddress);
    cfgCallTargetRangeInfo.NumberOfBytes = PAGE_SIZE;

    memset(&cfgCallTargetInfo, 0, sizeof(CFG_CALL_TARGET_INFO));
    cfgCallTargetInfo.Offset = BYTE_OFFSET(VirtualAddress);
    cfgCallTargetInfo.Flags = CFG_CALL_TARGET_CONVERT_XFG_TO_CFG;

    memset(&cfgCallTargetListInfo, 0, sizeof(CFG_CALL_TARGET_LIST_INFORMATION));
    cfgCallTargetListInfo.NumberOfEntries = 1;
    cfgCallTargetListInfo.Reserved = 0;
    cfgCallTargetListInfo.NumberOfEntriesProcessed = &numberOfEntriesProcessed;
    cfgCallTargetListInfo.CallTargetInfo = &cfgCallTargetInfo;

    status = NtSetInformationVirtualMemory_Import()(
        ProcessHandle,
        VmCfgCallTargetInformation,
        1,
        &cfgCallTargetRangeInfo,
        &cfgCallTargetListInfo,
        sizeof(CFG_CALL_TARGET_LIST_INFORMATION)
        );

    if (status == STATUS_INVALID_PAGE_PROTECTION)
        status = STATUS_SUCCESS;

    return status;
}

/**
 * Retrieves information about a range of pages within the virtual address space of a specified process.
 *
 * \param ProcessHandle A handle to a process.
 * \param Callback A callback function which is executed for each memory region.
 * \param Context A user-defined value to pass to the callback function.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhEnumVirtualMemory(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_MEMORY_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID baseAddress;
    MEMORY_BASIC_INFORMATION basicInfo;

    if (!NT_SUCCESS(status))
        return status;

    baseAddress = (PVOID)0;

    while (TRUE)
    {
        status = NtQueryVirtualMemory(
            ProcessHandle,
            baseAddress,
            MemoryBasicInformation,
            &basicInfo,
            sizeof(MEMORY_BASIC_INFORMATION),
            NULL
            );

        if (!NT_SUCCESS(status))
            break;

        if (FlagOn(basicInfo.State, MEM_FREE))
        {
            basicInfo.AllocationBase = basicInfo.BaseAddress;
            basicInfo.AllocationProtect = basicInfo.Protect;
        }

        status = Callback(ProcessHandle, &basicInfo, Context);

        if (!NT_SUCCESS(status))
            break;

        baseAddress = PTR_ADD_OFFSET(baseAddress, basicInfo.RegionSize);

        if ((ULONG_PTR)baseAddress >= PhSystemBasicInformation.MaximumUserModeAddress)
            break;
    }

    return status;
}

/**
 * Retrieves information about a range of pages within the virtual address space of a specified process in batches for improved performance.
 *
 * \param ProcessHandle A handle to a process.
 * \param BaseAddress The base address at which to begin retrieving information.
 * \param BulkQuery A boolean indicating the mode of bulk query (accuracy vs reliability).
 * \param Callback A callback function which is executed for each memory region.
 * \param Context A user-defined value to pass to the callback function.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhEnumVirtualMemoryBulk(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_ BOOLEAN BulkQuery,
    _In_ PPH_ENUM_MEMORY_BULK_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;

    if (WindowsVersion < WINDOWS_10_20H1)
        return STATUS_PROCEDURE_NOT_FOUND;

    // BulkQuery... TRUE:
    // * Faster.
    // * More accurate snapshots.
    // * Copies the entire VA space into local memory.
    // * Wastes large amounts of heap memory due to buffer doubling.
    // * Unsuitable for low-memory situations and fails with insufficient system resources.
    // * ...
    //
    // BulkQuery... FALSE:
    // * Slightly slower.
    // * Slightly less accurate snapshots.
    // * Does not copy the VA space.
    // * Does not waste heap memory.
    // * Suitable for low-memory situations and doesn't fail with insufficient system resources.
    // * ...

    if (BulkQuery)
    {
        SIZE_T bufferLength;
        PNTPSS_MEMORY_BULK_INFORMATION buffer;
        PMEMORY_BASIC_INFORMATION information;

        bufferLength = sizeof(NTPSS_MEMORY_BULK_INFORMATION) + sizeof(MEMORY_BASIC_INFORMATION[20]);
        buffer = PhAllocate(bufferLength);
        buffer->QueryFlags = MEMORY_BULK_INFORMATION_FLAG_BASIC;

        // Allocate a large buffer and copy all entries.

        while ((status = NtPssCaptureVaSpaceBulk_Import()(
            ProcessHandle,
            BaseAddress,
            buffer,
            bufferLength,
            NULL
            )) == STATUS_MORE_ENTRIES)
        {
            PhFree(buffer);
            bufferLength *= 2;

            if (bufferLength > PH_LARGE_BUFFER_SIZE)
                return STATUS_INSUFFICIENT_RESOURCES;

            buffer = PhAllocate(bufferLength);
            buffer->QueryFlags = MEMORY_BULK_INFORMATION_FLAG_BASIC;
        }

        if (NT_SUCCESS(status))
        {
            // Skip the enumeration header.

            information = PTR_ADD_OFFSET(buffer, RTL_SIZEOF_THROUGH_FIELD(NTPSS_MEMORY_BULK_INFORMATION, NextValidAddress));

            // Execute the callback.

            Callback(ProcessHandle, information, buffer->NumberOfEntries, Context);
        }

        PhFree(buffer);
    }
    else
    {
        UCHAR stackBuffer[sizeof(NTPSS_MEMORY_BULK_INFORMATION) + sizeof(MEMORY_BASIC_INFORMATION[20])];
        SIZE_T bufferLength;
        PNTPSS_MEMORY_BULK_INFORMATION buffer;
        PMEMORY_BASIC_INFORMATION information;

        bufferLength = sizeof(stackBuffer);
        buffer = (PNTPSS_MEMORY_BULK_INFORMATION)stackBuffer;
        buffer->QueryFlags = MEMORY_BULK_INFORMATION_FLAG_BASIC;
        buffer->NextValidAddress = BaseAddress;

        while (TRUE)
        {
            // Get a batch of entries.

            status = NtPssCaptureVaSpaceBulk_Import()(
                ProcessHandle,
                buffer->NextValidAddress,
                buffer,
                bufferLength,
                NULL
                );

            if (!NT_SUCCESS(status))
                break;

            // Skip the enumeration header.

            information = PTR_ADD_OFFSET(buffer, RTL_SIZEOF_THROUGH_FIELD(NTPSS_MEMORY_BULK_INFORMATION, NextValidAddress));

            // Execute the callback.

            if (!NT_SUCCESS(Callback(ProcessHandle, information, buffer->NumberOfEntries, Context)))
                break;

            // Get the next batch.

            if (status != STATUS_MORE_ENTRIES)
                break;
        }
    }

    return status;
}

/**
 * Retrieves information about the pages currently added to the working set of the specified process.
 *
 * \param ProcessHandle A handle to a process.
 * \param Callback A callback function which is executed for each memory page.
 * \param Context A user-defined value to pass to the callback function.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhEnumVirtualMemoryPages(
    _In_ HANDLE ProcessHandle,
    _In_ PPH_ENUM_MEMORY_PAGE_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PMEMORY_WORKING_SET_INFORMATION pageInfo;

    status = PhGetProcessWorkingSetInformation(
        ProcessHandle,
        &pageInfo
        );

    if (NT_SUCCESS(status))
    {
        status = Callback(
            ProcessHandle,
            pageInfo->NumberOfEntries,
            pageInfo->WorkingSetInfo,
            Context
            );

        //for (ULONG_PTR i = 0; i < pageInfo->NumberOfEntries; i++)
        //{
        //    PMEMORY_WORKING_SET_BLOCK workingSetBlock = &pageInfo->WorkingSetInfo[i];
        //    PVOID virtualAddress = (PVOID)(workingSetBlock->VirtualPage << PAGE_SHIFT);
        //}

        PhFree(pageInfo);
    }

    return status;
}

/**
 * Retrieves extended information about the pages currently added to the working set at specific virtual addresses in the address space of the specified process.
 *
 * \param ProcessHandle A handle to a process.
 * \param BaseAddress The base address at which to begin retrieving information.
 * \param Size The total number of pages to query from the base address.
 * \param Callback A callback function which is executed for each memory page.
 * \param Context A user-defined value to pass to the callback function.
 *
 * \return Successful or errant status.
 */
NTSTATUS PhEnumVirtualMemoryAttributes(
    _In_ HANDLE ProcessHandle,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T Size,
    _In_ PPH_ENUM_MEMORY_ATTRIBUTE_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SIZE_T numberOfPages;
    ULONG_PTR virtualAddress;
    PMEMORY_WORKING_SET_EX_INFORMATION info;
    SIZE_T i;

    numberOfPages = ADDRESS_AND_SIZE_TO_SPAN_PAGES(BaseAddress, Size);
    virtualAddress = (ULONG_PTR)PAGE_ALIGN(BaseAddress);

    if (!numberOfPages)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    info = PhAllocatePage(numberOfPages * sizeof(MEMORY_WORKING_SET_EX_INFORMATION), NULL);

    if (!info)
    {
        status = STATUS_UNSUCCESSFUL;
        goto CleanupExit;
    }

    for (i = 0; i < numberOfPages; i++)
    {
        info[i].VirtualAddress = (PVOID)virtualAddress;
        virtualAddress += PAGE_SIZE;
    }

    status = NtQueryVirtualMemory(
        ProcessHandle,
        NULL,
        MemoryWorkingSetExInformation,
        info,
        numberOfPages * sizeof(MEMORY_WORKING_SET_EX_INFORMATION),
        NULL
        );

    if (NT_SUCCESS(status))
    {
        status = Callback(
            ProcessHandle,
            BaseAddress,
            Size,
            numberOfPages,
            info,
            Context
            );
    }

    PhFreePage(info);

CleanupExit:
    return status;
}
