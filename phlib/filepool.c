/*
 * Process Hacker - 
 *   file-based allocator
 * 
 * Copyright (C) 2011 wj32
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
#include <filepool.h>
#include <filepoolp.h>

NTSTATUS PhCreateFilePool(
    __out PPH_FILE_POOL *Pool,
    __in HANDLE FileHandle,
    __in BOOLEAN ReadOnly,
    __in_opt PPH_FILE_POOL_PARAMETERS Parameters
    )
{
    NTSTATUS status;
    PPH_FILE_POOL pool;
    LARGE_INTEGER fileSize;
    PH_FILE_POOL_PARAMETERS localParameters;
    BOOLEAN creating;
    HANDLE sectionHandle;
    PPH_FP_BLOCK_HEADER initialBlock;
    PPH_FP_FILE_HEADER header;
    ULONG i;

    if (Parameters)
    {
        PhpValidateFilePoolParameters(Parameters);
    }
    else
    {
        PhpSetDefaultFilePoolParameters(&localParameters);
        Parameters = &localParameters;
    }

    pool = PhAllocate(sizeof(PH_FILE_POOL));
    memset(pool, 0, sizeof(PH_FILE_POOL));

    pool->FileHandle = FileHandle;
    pool->ReadOnly = ReadOnly;

    if (!NT_SUCCESS(status = PhGetFileSize(FileHandle, &fileSize)))
        goto CleanupExit;

    creating = FALSE;

    // If the file is smaller than the page size, assume we're creating a new file.
    if (fileSize.QuadPart < PAGE_SIZE)
    {
        if (ReadOnly)
        {
            status = STATUS_BAD_FILE_TYPE;
            goto CleanupExit;
        }

        fileSize.QuadPart = PAGE_SIZE;

        if (!NT_SUCCESS(status = PhSetFileSize(FileHandle, &fileSize)))
            goto CleanupExit;

        creating = TRUE;
    }

    // Create a section.
    status = NtCreateSection(
        &sectionHandle,
        SECTION_ALL_ACCESS,
        NULL,
        &fileSize,
        !ReadOnly ? PAGE_READWRITE : PAGE_READONLY,
        SEC_COMMIT,
        FileHandle
        );

    if (!NT_SUCCESS(status))
        goto CleanupExit;

    pool->SectionHandle = sectionHandle;

    // Map in the first segment, set up initial parameters, then remap the 
    // first segment.

    if (!NT_SUCCESS(status = PhFppMapRange(pool, 0, PAGE_SIZE, &initialBlock)))
        goto CleanupExit;

    header = (PPH_FP_FILE_HEADER)&initialBlock->Body;

    if (creating)
    {
        header->Magic = PH_FP_MAGIC;
        header->SegmentShift = Parameters->SegmentShift;
        header->SegmentCount = 1;

        for (i = 0; i < PH_FP_FREE_LIST_COUNT; i++)
            header->FreeLists[i] = -1;
    }
    else
    {
        if (header->Magic != PH_FP_MAGIC)
        {
            PhFppUnmapRange(pool, initialBlock);
            status = STATUS_BAD_FILE_TYPE;
            goto CleanupExit;
        }
    }

    pool->SegmentShift = header->SegmentShift;
    pool->SegmentSize = 1 << pool->SegmentShift;

    pool->BlockShift = pool->SegmentShift - PH_FP_BLOCK_COUNT_SHIFT;
    pool->BlockSize = 1 << pool->BlockShift;
    pool->FileHeaderBlockSpan = (sizeof(PH_FP_FILE_HEADER) + pool->BlockSize - 1) >> pool->BlockShift;
    pool->SegmentHeaderBlockSpan = (sizeof(PH_FP_SEGMENT_HEADER) + pool->BlockSize - 1) >> pool->BlockShift;

    // Unmap the first segment and remap with the new segment size.

    PhFppUnmapRange(pool, initialBlock);

    if (creating)
    {
        // Extend the section so it fits the entire first segment.
        if (!NT_SUCCESS(status = PhFppExtendRange(pool, pool->SegmentSize)))
            goto CleanupExit;
    }

    // Runtime structure initialization

    PhInitializeFreeList(&pool->ViewFreeList, sizeof(PH_FILE_POOL_VIEW), 32);
    pool->ByIndexSize = 32;
    pool->ByIndexBuckets = PhAllocate(sizeof(PPH_FILE_POOL_VIEW) * pool->ByIndexSize);
    memset(pool->ByIndexBuckets, 0, sizeof(PPH_FILE_POOL_VIEW) * pool->ByIndexSize);
    PhInitializeAvlTree(&pool->ByBaseSet, PhpFilePoolViewByBaseCompareFunction);

    pool->MaximumInactiveViews = Parameters->MaximumInactiveViews;
    InitializeListHead(&pool->InactiveViewsListHead);

    // File structure initialization

    pool->FirstBlockOfFirstSegment = PhFppReferenceSegment(pool, 0);
    pool->Header = (PPH_FP_FILE_HEADER)&pool->FirstBlockOfFirstSegment->Body;

    if (creating)
    {
        PPH_FP_BLOCK_HEADER segmentHeaderBlock;

        // Set up the first segment properly.

        pool->FirstBlockOfFirstSegment->Span = pool->FileHeaderBlockSpan;
        segmentHeaderBlock = (PPH_FP_BLOCK_HEADER)((PCHAR)pool->FirstBlockOfFirstSegment + (pool->FileHeaderBlockSpan << pool->BlockShift));
        PhFppInitializeSegment(pool, segmentHeaderBlock, pool->FileHeaderBlockSpan);

        pool->Header->FreeLists[1] = 0;
    }

CleanupExit:
    if (NT_SUCCESS(status))
    {
        *Pool = pool;
    }
    else
    {
        // Don't close the file handle the user passed in.
        pool->FileHandle = NULL;
        PhDestroyFilePool(pool);
    }

    return status;
}

NTSTATUS PhCreateFilePool2(
    __out PPH_FILE_POOL *Pool,
    __in PWSTR FileName,
    __in BOOLEAN ReadOnly,
    __in ULONG ShareAccess,
    __in ULONG CreateDisposition,
    __in_opt PPH_FILE_POOL_PARAMETERS Parameters
    )
{
    NTSTATUS status;
    PPH_FILE_POOL pool;
    HANDLE fileHandle;
    ULONG createStatus;

    if (!NT_SUCCESS(status = PhCreateFileWin32Ex(
        &fileHandle,
        FileName,
        !ReadOnly ? (FILE_GENERIC_READ | FILE_GENERIC_WRITE | DELETE) : FILE_GENERIC_READ,
        FILE_ATTRIBUTE_NORMAL,
        ShareAccess,
        CreateDisposition,
        FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
        &createStatus
        )))
    {
        return status;
    }

    status = PhCreateFilePool(&pool, fileHandle, ReadOnly, Parameters);

    if (NT_SUCCESS(status))
    {
        *Pool = pool;
    }
    else
    {
        if (!ReadOnly && createStatus == FILE_CREATED)
        {
            FILE_DISPOSITION_INFORMATION dispositionInfo;
            IO_STATUS_BLOCK isb;

            dispositionInfo.DeleteFile = TRUE;
            NtSetInformationFile(fileHandle, &isb, &dispositionInfo, sizeof(FILE_DISPOSITION_INFORMATION), FileDispositionInformation);
        }

        NtClose(fileHandle);
    }

    return status;
}

VOID PhDestroyFilePool(
    __in __post_invalid PPH_FILE_POOL Pool
    )
{
    ULONG i;
    PLIST_ENTRY head;
    PLIST_ENTRY entry;
    PPH_FILE_POOL_VIEW view;

    // Unmap all views.
    for (i = 0; i < Pool->ByIndexSize; i++)
    {
        if (head = Pool->ByIndexBuckets[i])
        {
            entry = head;

            do
            {
                view = CONTAINING_RECORD(entry, PH_FILE_POOL_VIEW, ByIndexListEntry);
                entry = entry->Flink;
                PhFppUnmapRange(Pool, view->Base);
                PhFreeToFreeList(&Pool->ViewFreeList, view);
            } while (entry != head);
        }
    }

    if (Pool->ByIndexBuckets)
        PhFree(Pool->ByIndexBuckets);

    PhDeleteFreeList(&Pool->ViewFreeList);

    if (Pool->SectionHandle)
        NtClose(Pool->SectionHandle);
    if (Pool->FileHandle)
        NtClose(Pool->FileHandle);

    PhFree(Pool);
}

NTSTATUS PhpValidateFilePoolParameters(
    __in PPH_FILE_POOL_PARAMETERS Parameters
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    // 16 <= SegmentShift <= 28

    if (Parameters->SegmentShift < 16)
    {
        Parameters->SegmentShift = 16;
        status = STATUS_SOME_NOT_MAPPED;
    }

    if (Parameters->SegmentShift > 28)
    {
        Parameters->SegmentShift = 28;
        status = STATUS_SOME_NOT_MAPPED;
    }

    return status;
}

VOID PhpSetDefaultFilePoolParameters(
    __out PPH_FILE_POOL_PARAMETERS Parameters
    )
{
    Parameters->SegmentShift = 18; // 256kB
    Parameters->MaximumInactiveViews = 128;
}

PVOID PhAllocateFilePool(
    __inout PPH_FILE_POOL Pool,
    __in ULONG Size,
    __out_opt PULONG Rva
    )
{
    PPH_FP_BLOCK_HEADER blockHeader;
    ULONG numberOfBlocks;
    PPH_FP_BLOCK_HEADER firstBlock;
    PPH_FP_SEGMENT_HEADER segmentHeader;
    ULONG freeListIndex;
    ULONG freeListLimit;
    ULONG segmentIndex;
    ULONG nextSegmentIndex;
    ULONG newFreeListIndex;

    // Calculate the number of blocks needed for the allocation.
    numberOfBlocks = (FIELD_OFFSET(PH_FP_BLOCK_HEADER, Body) + Size + Pool->BlockSize - 1) >> Pool->BlockShift;

    if (numberOfBlocks > PH_FP_BLOCK_COUNT - Pool->SegmentHeaderBlockSpan)
    {
        // TODO: Perform a large allocation.
        return NULL;
    }

    // Scan each applicable free list and try to allocate from those segments.

    freeListLimit = PhFppComputeFreeListIndex(Pool, numberOfBlocks);

    for (freeListIndex = 0; freeListIndex <= freeListLimit; freeListIndex++)
    {
        segmentIndex = Pool->Header->FreeLists[freeListIndex];

        while (segmentIndex != -1)
        {
            firstBlock = PhFppReferenceSegment(Pool, segmentIndex);

            if (!firstBlock)
                return NULL;

            segmentHeader = PhFppGetHeaderSegment(Pool, firstBlock);
            nextSegmentIndex = segmentHeader->FreeFlink;

            blockHeader = PhFppAllocateBlocks(Pool, firstBlock, segmentHeader, numberOfBlocks);

            if (blockHeader)
                goto BlockAllocated;

            PhFppDereferenceSegment(Pool, segmentIndex);
            segmentIndex = nextSegmentIndex;
        }
    }

    // No segments have the required number of contiguous free blocks. Allocate a new one.

    firstBlock = PhFppAllocateSegment(Pool, &segmentIndex);

    if (!firstBlock)
        return NULL;

    freeListIndex = 0;
    segmentHeader = PhFppGetHeaderSegment(Pool, firstBlock);
    blockHeader = PhFppAllocateBlocks(Pool, firstBlock, segmentHeader, numberOfBlocks);

    if (!blockHeader)
    {
        PhFppDereferenceSegment(Pool, segmentIndex);
        return NULL;
    }

BlockAllocated:

    // Compute the new free list index of the segment and move it to another free list if necessary.

    newFreeListIndex = PhFppComputeFreeListIndex(Pool, segmentHeader->FreeBlocks);

    if (newFreeListIndex != freeListIndex)
    {
        PhFppRemoveFreeList(Pool, freeListIndex, segmentIndex, segmentHeader);
        PhFppInsertFreeList(Pool, newFreeListIndex, segmentIndex, segmentHeader);
    }

    if (Rva)
    {
        *Rva = PhFppEncodeRva(Pool, segmentIndex, firstBlock, &blockHeader->Body);
    }

    return &blockHeader->Body;
}

VOID PhpFreeFilePool(
    __inout PPH_FILE_POOL Pool,
    __in ULONG SegmentIndex,
    __in PPH_FP_BLOCK_HEADER FirstBlock,
    __in PVOID Block
    )
{
    PPH_FP_SEGMENT_HEADER segmentHeader;
    ULONG oldFreeListIndex;
    ULONG newFreeListIndex;

    segmentHeader = PhFppGetHeaderSegment(Pool, FirstBlock);
    oldFreeListIndex = PhFppComputeFreeListIndex(Pool, segmentHeader->FreeBlocks);
    PhFppFreeBlocks(Pool, FirstBlock, segmentHeader, PhFppGetHeaderBlock(Pool, Block));
    newFreeListIndex = PhFppComputeFreeListIndex(Pool, segmentHeader->FreeBlocks);

    // Move the segment into another free list if needed.
    if (newFreeListIndex != oldFreeListIndex)
    {
        PhFppRemoveFreeList(Pool, oldFreeListIndex, SegmentIndex, segmentHeader);
        PhFppInsertFreeList(Pool, newFreeListIndex, SegmentIndex, segmentHeader);
    }
}

VOID PhFreeFilePool(
    __inout PPH_FILE_POOL Pool,
    __in PVOID Block
    )
{
    PPH_FILE_POOL_VIEW view;
    PPH_FP_BLOCK_HEADER firstBlock;

    view = PhFppFindViewByBase(Pool, Block);

    if (!view)
        PhRaiseStatus(STATUS_INVALID_PARAMETER_2);

    firstBlock = view->Base;
    PhpFreeFilePool(Pool, view->SegmentIndex, firstBlock, Block);
}

BOOLEAN PhFreeFilePoolByRva(
    __inout PPH_FILE_POOL Pool,
    __in ULONG Rva
    )
{
    ULONG segmentIndex;
    ULONG offset;
    PPH_FP_BLOCK_HEADER firstBlock;

    offset = PhFppDecodeRva(Pool, Rva, &segmentIndex);

    if (offset == -1)
        return FALSE;

    firstBlock = PhFppReferenceSegment(Pool, segmentIndex);

    if (!firstBlock)
        return FALSE;

    PhpFreeFilePool(Pool, segmentIndex, firstBlock, (PCHAR)firstBlock + offset);
    PhFppDereferenceSegment(Pool, segmentIndex);

    return TRUE;
}

VOID PhReferenceFilePool(
    __inout PPH_FILE_POOL Pool,
    __in PVOID Address
    )
{
    PhFppReferenceSegmentByBase(Pool, Address);
}

VOID PhDereferenceFilePool(
    __inout PPH_FILE_POOL Pool,
    __in PVOID Address
    )
{
    PhFppDereferenceSegmentByBase(Pool, Address);
}

PVOID PhReferenceFilePoolByRva(
    __inout PPH_FILE_POOL Pool,
    __in ULONG Rva
    )
{
    ULONG segmentIndex;
    ULONG offset;
    PPH_FP_BLOCK_HEADER firstBlock;

    if (Rva == 0)
        return NULL;

    offset = PhFppDecodeRva(Pool, Rva, &segmentIndex);

    if (offset == -1)
        return NULL;

    firstBlock = PhFppReferenceSegment(Pool, segmentIndex);

    if (!firstBlock)
        return NULL;

    return (PCHAR)firstBlock + offset;
}

BOOLEAN PhDereferenceFilePoolByRva(
    __inout PPH_FILE_POOL Pool,
    __in ULONG Rva
    )
{
    ULONG segmentIndex;
    ULONG offset;

    offset = PhFppDecodeRva(Pool, Rva, &segmentIndex);

    if (offset == -1)
        return FALSE;

    PhFppDereferenceSegment(Pool, segmentIndex);

    return TRUE;
}

ULONG PhEncodeRvaFilePool(
    __in PPH_FILE_POOL Pool,
    __in PVOID Address
    )
{
    PPH_FILE_POOL_VIEW view;

    if (!Address)
        return 0;

    view = PhFppFindViewByBase(Pool, Address);

    if (!view)
        PhRaiseStatus(STATUS_INVALID_PARAMETER_2);

    return PhFppEncodeRva(Pool, view->SegmentIndex, view->Base, Address);
}

VOID PhGetUserContextFilePool(
    __in PPH_FILE_POOL Pool,
    __out PULONGLONG Context
    )
{
    *Context = Pool->Header->UserContext;
}

VOID PhSetUserContextFilePool(
    __inout PPH_FILE_POOL Pool,
    __in PULONGLONG Context
    )
{
    Pool->Header->UserContext = *Context;
}

NTSTATUS PhFppExtendRange(
    __inout PPH_FILE_POOL Pool,
    __in ULONG NewSize
    )
{
    LARGE_INTEGER newSectionSize;

    newSectionSize.QuadPart = NewSize;

    return NtExtendSection(Pool->SectionHandle, &newSectionSize);
}

NTSTATUS PhFppMapRange(
    __inout PPH_FILE_POOL Pool,
    __in ULONG Offset,
    __in ULONG Size,
    __out PVOID *Base
    )
{
    NTSTATUS status;
    PVOID baseAddress;
    LARGE_INTEGER sectionOffset;
    SIZE_T viewSize;

    baseAddress = NULL;
    sectionOffset.QuadPart = Offset;
    viewSize = Size;

    status = NtMapViewOfSection(
        Pool->SectionHandle,
        NtCurrentProcess(),
        &baseAddress,
        0,
        viewSize,
        &sectionOffset,
        &viewSize,
        ViewShare,
        0,
        !Pool->ReadOnly ? PAGE_READWRITE : PAGE_READONLY
        );

    if (NT_SUCCESS(status))
        *Base = baseAddress;

    return status;
}

NTSTATUS PhFppUnmapRange(
    __inout PPH_FILE_POOL Pool,
    __in PVOID Base
    )
{
    return NtUnmapViewOfSection(NtCurrentProcess(), Base);
}

VOID PhFppInitializeSegment(
    __inout PPH_FILE_POOL Pool,
    __out PPH_FP_BLOCK_HEADER BlockOfSegmentHeader,
    __in ULONG AdditionalBlocksUsed
    )
{
    PPH_FP_SEGMENT_HEADER segmentHeader;
    RTL_BITMAP bitmap;

    BlockOfSegmentHeader->Span = Pool->SegmentHeaderBlockSpan;
    segmentHeader = (PPH_FP_SEGMENT_HEADER)&BlockOfSegmentHeader->Body;

    RtlInitializeBitMap(&bitmap, segmentHeader->Bitmap, PH_FP_BLOCK_COUNT);
    RtlSetBits(&bitmap, 0, Pool->SegmentHeaderBlockSpan + AdditionalBlocksUsed);
    segmentHeader->FreeBlocks = PH_FP_BLOCK_COUNT - (Pool->SegmentHeaderBlockSpan + AdditionalBlocksUsed);
    segmentHeader->FreeFlink = -1;
    segmentHeader->FreeBlink = -1;
}

PPH_FP_BLOCK_HEADER PhFppAllocateSegment(
    __inout PPH_FILE_POOL Pool,
    __out PULONG NewSegmentIndex
    )
{
    ULONG newSize;
    ULONG segmentIndex;
    PPH_FP_BLOCK_HEADER firstBlock;
    PPH_FP_SEGMENT_HEADER segmentHeader;

    newSize = (Pool->Header->SegmentCount + 1) << Pool->SegmentShift;

    if (!NT_SUCCESS(PhFppExtendRange(Pool, newSize)))
        return NULL;

    segmentIndex = Pool->Header->SegmentCount++;
    firstBlock = PhFppReferenceSegment(Pool, segmentIndex);
    PhFppInitializeSegment(Pool, firstBlock, 0);
    segmentHeader = (PPH_FP_SEGMENT_HEADER)&firstBlock->Body;

    PhFppInsertFreeList(Pool, 0, segmentIndex, segmentHeader);

    *NewSegmentIndex = segmentIndex;

    return firstBlock;
}

PPH_FP_SEGMENT_HEADER PhFppGetHeaderSegment(
    __inout PPH_FILE_POOL Pool,
    __in PPH_FP_BLOCK_HEADER FirstBlock
    )
{
    if (FirstBlock != Pool->FirstBlockOfFirstSegment)
    {
        return (PPH_FP_SEGMENT_HEADER)&FirstBlock->Body;
    }
    else
    {
        // In the first segment, the segment header is after the file header.
        return (PPH_FP_SEGMENT_HEADER)&((PPH_FP_BLOCK_HEADER)((PCHAR)FirstBlock + (Pool->FileHeaderBlockSpan << Pool->BlockShift)))->Body;
    }
}

VOID PhFppAddViewByIndex(
    __inout PPH_FILE_POOL Pool,
    __inout PPH_FILE_POOL_VIEW View
    )
{
    ULONG index;
    PLIST_ENTRY head;

    index = View->SegmentIndex & (Pool->ByIndexSize - 1);
    head = Pool->ByIndexBuckets[index];

    if (head)
    {
        InsertHeadList(head, &View->ByIndexListEntry);
    }
    else
    {
        InitializeListHead(&View->ByIndexListEntry);
        Pool->ByIndexBuckets[index] = &View->ByIndexListEntry;
    }
}

VOID PhFppRemoveViewByIndex(
    __inout PPH_FILE_POOL Pool,
    __inout PPH_FILE_POOL_VIEW View
    )
{
    ULONG index;
    PLIST_ENTRY head;

    index = View->SegmentIndex & (Pool->ByIndexSize - 1);
    head = Pool->ByIndexBuckets[index];
    assert(head);

    // Unlink the entry from the chain.
    RemoveEntryList(&View->ByIndexListEntry);

    if (&View->ByIndexListEntry == head)
    {
        // This entry is currently the chain head.

        // If this was the last entry in the chain, then indicate 
        // that the chain is empty.
        // Otherwise, choose a new head.

        if (IsListEmpty(head))
            Pool->ByIndexBuckets[index] = NULL;
        else
            Pool->ByIndexBuckets[index] = head->Flink;
    }
}

PPH_FILE_POOL_VIEW PhFppFindViewByIndex(
    __inout PPH_FILE_POOL Pool,
    __in ULONG SegmentIndex
    )
{
    ULONG index;
    PLIST_ENTRY head;
    PLIST_ENTRY entry;
    PPH_FILE_POOL_VIEW view;

    index = SegmentIndex & (Pool->ByIndexSize - 1);
    head = Pool->ByIndexBuckets[index];

    if (!head)
        return NULL;

    entry = head;

    do
    {
        view = CONTAINING_RECORD(entry, PH_FILE_POOL_VIEW, ByIndexListEntry);

        if (view->SegmentIndex == SegmentIndex)
            return view;

        entry = entry->Flink;
    } while (entry != head);

    return NULL;
}

LONG NTAPI PhpFilePoolViewByBaseCompareFunction(
    __in PPH_AVL_LINKS Links1,
    __in PPH_AVL_LINKS Links2
    )
{
    PPH_FILE_POOL_VIEW view1 = CONTAINING_RECORD(Links1, PH_FILE_POOL_VIEW, ByBaseLinks);
    PPH_FILE_POOL_VIEW view2 = CONTAINING_RECORD(Links2, PH_FILE_POOL_VIEW, ByBaseLinks);

    return uintptrcmp((ULONG_PTR)view1->Base, (ULONG_PTR)view2->Base);
}

VOID PhFppAddViewByBase(
    __inout PPH_FILE_POOL Pool,
    __inout PPH_FILE_POOL_VIEW View
    )
{
    PhAddElementAvlTree(&Pool->ByBaseSet, &View->ByBaseLinks);
}

VOID PhFppRemoveViewByBase(
    __inout PPH_FILE_POOL Pool,
    __inout PPH_FILE_POOL_VIEW View
    )
{
    PhRemoveElementAvlTree(&Pool->ByBaseSet, &View->ByBaseLinks);
}

PPH_FILE_POOL_VIEW PhFppFindViewByBase(
    __inout PPH_FILE_POOL Pool,
    __in PVOID Base
    )
{
    PPH_FILE_POOL_VIEW view;
    PPH_AVL_LINKS links;
    PH_FILE_POOL_VIEW lookupView;
    LONG result;

    // This is an approximate search to find the target view in which the specified address 
    // lies.

    lookupView.Base = Base;

    links = PhFindElementAvlTree2(&Pool->ByBaseSet, &lookupView.ByBaseLinks, &result);

    if (!links)
        return NULL;

    if (result == 0)
    {
        return CONTAINING_RECORD(links, PH_FILE_POOL_VIEW, ByBaseLinks);
    }
    else if (result < 0)
    {
        // The base of the returned element is larger than our base. The preceding 
        // element must be the target view, or the target view doesn't exist.

        links = PhPredecessorElementAvlTree(links);

        if (!links)
            return NULL;
    }
    else
    {
        // The base of the returned element is smaller than our base. This element 
        // must be the target view, or the target view doesn't exist.
    }

    view = CONTAINING_RECORD(links, PH_FILE_POOL_VIEW, ByBaseLinks);

    if ((ULONG_PTR)Base >= (ULONG_PTR)view->Base && (ULONG_PTR)Base < (ULONG_PTR)view->Base + Pool->SegmentSize)
        return view;

    return NULL;
}

PPH_FILE_POOL_VIEW PhFppCreateView(
    __inout PPH_FILE_POOL Pool,
    __in ULONG SegmentIndex
    )
{
    PPH_FILE_POOL_VIEW view;
    PVOID base;

    // Map in the segment.
    if (!NT_SUCCESS(PhFppMapRange(Pool, SegmentIndex << Pool->SegmentShift, Pool->SegmentSize, &base)))
        return NULL;

    // Create and add the view entry.

    view = PhAllocateFromFreeList(&Pool->ViewFreeList);
    memset(view, 0, sizeof(PH_FILE_POOL_VIEW));

    view->RefCount = 1;
    view->SegmentIndex = SegmentIndex;
    view->Base = base;

    PhFppAddViewByIndex(Pool, view);
    PhFppAddViewByBase(Pool, view);

    return view;
}

VOID PhFppDestroyView(
    __inout PPH_FILE_POOL Pool,
    __inout PPH_FILE_POOL_VIEW View
    )
{
    PhFppUnmapRange(Pool, View->Base);
    PhFppRemoveViewByIndex(Pool, View);
    PhFppRemoveViewByBase(Pool, View);

    PhFreeToFreeList(&Pool->ViewFreeList, View);
}

VOID PhFppActivateView(
    __inout PPH_FILE_POOL Pool,
    __inout PPH_FILE_POOL_VIEW View
    )
{
    RemoveEntryList(&View->InactiveViewsListEntry);
    Pool->NumberOfInactiveViews--;
}

VOID PhFppDeactivateView(
    __inout PPH_FILE_POOL Pool,
    __inout PPH_FILE_POOL_VIEW View
    )
{
    InsertHeadList(&Pool->InactiveViewsListHead, &View->InactiveViewsListEntry);
    Pool->NumberOfInactiveViews++;

    // If we have too many inactive views, destroy the least recent ones.
    while (Pool->NumberOfInactiveViews > Pool->MaximumInactiveViews)
    {
        PLIST_ENTRY lruEntry;
        PPH_FILE_POOL_VIEW lruView;

        lruEntry = RemoveTailList(&Pool->InactiveViewsListHead);
        Pool->NumberOfInactiveViews--;

        assert(lruEntry != &Pool->InactiveViewsListHead);
        lruView = CONTAINING_RECORD(lruEntry, PH_FILE_POOL_VIEW, InactiveViewsListEntry);
        PhFppDestroyView(Pool, lruView);
    }
}

VOID PhFppReferenceView(
    __inout PPH_FILE_POOL Pool,
    __inout PPH_FILE_POOL_VIEW View
    )
{
    if (View->RefCount == 0)
    {
        // The view is inactive, so make it active.
        PhFppActivateView(Pool, View);
    }

    View->RefCount++;
}

VOID PhFppDereferenceView(
    __inout PPH_FILE_POOL Pool,
    __inout PPH_FILE_POOL_VIEW View
    )
{
    if (--View->RefCount == 0)
    {
        if (View->SegmentIndex == 0)
            PhRaiseStatus(STATUS_INTERNAL_ERROR);

        PhFppDeactivateView(Pool, View);
    }
}

PPH_FP_BLOCK_HEADER PhFppReferenceSegment(
    __inout PPH_FILE_POOL Pool,
    __in ULONG SegmentIndex
    )
{
    PPH_FILE_POOL_VIEW view;

    // Validate parameters.
    if (SegmentIndex != 0 && SegmentIndex >= Pool->Header->SegmentCount)
        return NULL;

    // Try to get a cached view.

    view = PhFppFindViewByIndex(Pool, SegmentIndex);

    if (view)
    {
        PhFppReferenceView(Pool, view);

        return (PPH_FP_BLOCK_HEADER)view->Base;
    }

    // No cached view, so create one.

    view = PhFppCreateView(Pool, SegmentIndex);

    if (!view)
        return NULL;

    return (PPH_FP_BLOCK_HEADER)view->Base;
}

VOID PhFppDereferenceSegment(
    __inout PPH_FILE_POOL Pool,
    __in ULONG SegmentIndex
    )
{
    PPH_FILE_POOL_VIEW view;

    view = PhFppFindViewByIndex(Pool, SegmentIndex);

    if (!view)
        PhRaiseStatus(STATUS_INTERNAL_ERROR);

    PhFppDereferenceView(Pool, view);
}

VOID PhFppReferenceSegmentByBase(
    __inout PPH_FILE_POOL Pool,
    __in PVOID Base
    )
{
    PPH_FILE_POOL_VIEW view;

    view = PhFppFindViewByBase(Pool, Base);

    if (!view)
        PhRaiseStatus(STATUS_INTERNAL_ERROR);

    PhFppReferenceView(Pool, view);
}

VOID PhFppDereferenceSegmentByBase(
    __inout PPH_FILE_POOL Pool,
    __in PVOID Base
    )
{
    PPH_FILE_POOL_VIEW view;

    view = PhFppFindViewByBase(Pool, Base);

    if (!view)
        PhRaiseStatus(STATUS_INTERNAL_ERROR);

    PhFppDereferenceView(Pool, view);
}

PPH_FP_BLOCK_HEADER PhFppAllocateBlocks(
    __inout PPH_FILE_POOL Pool,
    __in PPH_FP_BLOCK_HEADER FirstBlock,
    __inout PPH_FP_SEGMENT_HEADER SegmentHeader,
    __in ULONG NumberOfBlocks
    )
{
    RTL_BITMAP bitmap;
    ULONG hintIndex;
    ULONG foundIndex;
    PPH_FP_BLOCK_HEADER blockHeader;

    if (FirstBlock != Pool->FirstBlockOfFirstSegment)
        hintIndex = Pool->SegmentHeaderBlockSpan;
    else
        hintIndex = Pool->SegmentHeaderBlockSpan + Pool->FileHeaderBlockSpan;

    RtlInitializeBitMap(&bitmap, SegmentHeader->Bitmap, PH_FP_BLOCK_COUNT);

    // Find a range of free blocks and mark them as in-use.
    foundIndex = RtlFindClearBitsAndSet(&bitmap, NumberOfBlocks, hintIndex);

    if (foundIndex == -1)
    {
        // No more space.
        return NULL;
    }

    SegmentHeader->FreeBlocks -= NumberOfBlocks;

    blockHeader = (PPH_FP_BLOCK_HEADER)((PCHAR)FirstBlock + (foundIndex << Pool->BlockShift));
    blockHeader->Flags = 0;
    blockHeader->Span = NumberOfBlocks;

    return blockHeader;
}

VOID PhFppFreeBlocks(
    __inout PPH_FILE_POOL Pool,
    __in PPH_FP_BLOCK_HEADER FirstBlock,
    __inout PPH_FP_SEGMENT_HEADER SegmentHeader,
    __in PPH_FP_BLOCK_HEADER BlockHeader
    )
{
    RTL_BITMAP bitmap;
    ULONG startIndex;
    ULONG blockSpan;

    RtlInitializeBitMap(&bitmap, SegmentHeader->Bitmap, PH_FP_BLOCK_COUNT);

    // Mark the blocks as free.
    startIndex = (ULONG)((PCHAR)BlockHeader - (PCHAR)FirstBlock) >> Pool->BlockShift;
    blockSpan = BlockHeader->Span;
    RtlClearBits(&bitmap, startIndex, blockSpan);
    SegmentHeader->FreeBlocks += blockSpan;
}

ULONG PhFppComputeFreeListIndex(
    __in PPH_FILE_POOL Pool,
    __in ULONG NumberOfBlocks
    )
{
    // Use a binary tree to speed up comparison.

    if (NumberOfBlocks >= PH_FP_BLOCK_COUNT / 64)
    {
        if (NumberOfBlocks >= PH_FP_BLOCK_COUNT / 2)
        {
            if (NumberOfBlocks >= PH_FP_BLOCK_COUNT - Pool->SegmentHeaderBlockSpan)
                return 0;
            else
                return 1;
        }
        else
        {
            if (NumberOfBlocks >= PH_FP_BLOCK_COUNT / 16)
                return 2;
            else
                return 3;
        }
    }
    else
    {
        if (NumberOfBlocks >= 4)
        {
            if (NumberOfBlocks >= PH_FP_BLOCK_COUNT / 256)
                return 4;
            else
                return 5;
        }
        else
        {
            if (NumberOfBlocks >= 1)
                return 6;
            else
                return 7;
        }
    }
}

BOOLEAN PhFppInsertFreeList(
    __inout PPH_FILE_POOL Pool,
    __in ULONG FreeListIndex,
    __in ULONG SegmentIndex,
    __in PPH_FP_SEGMENT_HEADER SegmentHeader
    )
{
    ULONG oldSegmentIndex;
    PPH_FP_BLOCK_HEADER oldSegmentFirstBlock;
    PPH_FP_SEGMENT_HEADER oldSegmentHeader;

    oldSegmentIndex = Pool->Header->FreeLists[FreeListIndex];

    // Try to reference the segment before we commit any changes.

    if (oldSegmentIndex != -1)
    {
        oldSegmentFirstBlock = PhFppReferenceSegment(Pool, oldSegmentIndex);

        if (!oldSegmentFirstBlock)
            return FALSE;
    }

    // Insert the segment into the list.

    SegmentHeader->FreeBlink = -1;
    SegmentHeader->FreeFlink = oldSegmentIndex;
    Pool->Header->FreeLists[FreeListIndex] = SegmentIndex;

    if (oldSegmentIndex != -1)
    {
        oldSegmentHeader = PhFppGetHeaderSegment(Pool, oldSegmentFirstBlock);
        oldSegmentHeader->FreeBlink = SegmentIndex;
        PhFppDereferenceSegment(Pool, oldSegmentIndex);
    }

    return TRUE;
}

BOOLEAN PhFppRemoveFreeList(
    __inout PPH_FILE_POOL Pool,
    __in ULONG FreeListIndex,
    __in ULONG SegmentIndex,
    __in PPH_FP_SEGMENT_HEADER SegmentHeader
    )
{
    ULONG flinkSegmentIndex;
    PPH_FP_BLOCK_HEADER flinkSegmentFirstBlock;
    PPH_FP_SEGMENT_HEADER flinkSegmentHeader;
    ULONG blinkSegmentIndex;
    PPH_FP_BLOCK_HEADER blinkSegmentFirstBlock;
    PPH_FP_SEGMENT_HEADER blinkSegmentHeader;

    flinkSegmentIndex = SegmentHeader->FreeFlink;
    blinkSegmentIndex = SegmentHeader->FreeBlink;

    // Try to reference the segments before we commit any changes.

    if (flinkSegmentIndex != -1)
    {
        flinkSegmentFirstBlock = PhFppReferenceSegment(Pool, flinkSegmentIndex);

        if (!flinkSegmentFirstBlock)
            return FALSE;
    }

    if (blinkSegmentIndex != -1)
    {
        blinkSegmentFirstBlock = PhFppReferenceSegment(Pool, blinkSegmentIndex);

        if (!blinkSegmentFirstBlock)
        {
            if (flinkSegmentIndex != -1)
                PhFppDereferenceSegment(Pool, flinkSegmentIndex);

            return FALSE;
        }
    }

    // Unlink the segment from the list.

    if (flinkSegmentIndex != -1)
    {
        flinkSegmentHeader = PhFppGetHeaderSegment(Pool, flinkSegmentFirstBlock);
        flinkSegmentHeader->FreeBlink = blinkSegmentIndex;
        PhFppDereferenceSegment(Pool, flinkSegmentIndex);
    }

    if (blinkSegmentIndex != -1)
    {
        blinkSegmentHeader = PhFppGetHeaderSegment(Pool, blinkSegmentFirstBlock);
        blinkSegmentHeader->FreeBlink = flinkSegmentIndex;
        PhFppDereferenceSegment(Pool, blinkSegmentIndex);
    }
    else
    {
        // The segment was the list head; select a new one.
        Pool->Header->FreeLists[FreeListIndex] = flinkSegmentIndex;
    }

    return TRUE;
}

PPH_FP_BLOCK_HEADER PhFppGetHeaderBlock(
    __in PPH_FILE_POOL Pool,
    __in PVOID Block
    )
{
    return CONTAINING_RECORD(Block, PH_FP_BLOCK_HEADER, Body);
}

ULONG PhFppEncodeRva(
    __in PPH_FILE_POOL Pool,
    __in ULONG SegmentIndex,
    __in PPH_FP_BLOCK_HEADER FirstBlock,
    __in PVOID Address
    )
{
    return (SegmentIndex << Pool->SegmentShift) + (ULONG)((PCHAR)Address - (PCHAR)FirstBlock);
}

ULONG PhFppDecodeRva(
    __in PPH_FILE_POOL Pool,
    __in ULONG Rva,
    __out PULONG SegmentIndex
    )
{
    ULONG segmentIndex;

    segmentIndex = Rva >> Pool->SegmentShift;

    if (segmentIndex >= Pool->Header->SegmentCount)
        return -1;

    *SegmentIndex = segmentIndex;

    return Rva & (Pool->SegmentSize - 1);
}
