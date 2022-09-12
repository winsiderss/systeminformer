/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011
 *
 */

/*
 * File pool allows blocks of storage to be allocated from a file. Each file looks like this:
 *
 * Segment 0 __________________________________________________________
 * |              |                |              |                |
 * | Block Header |  File Header   | Block Header | Segment Header |
 * |______________|________________|______________|________________|
 * |              |                |              |                |
 * | Block Header |    User Data   | Block Header |    User Data   |
 * |______________|________________|______________|________________|
 * |              |                |              |                |
 * |      ...     |       ...      |     ...      |      ...       |
 * |______________|________________|______________|________________|
 * Segment 1 __________________________________________________________
 * |              |                |              |                |
 * | Block Header | Segment Header | Block Header |    User Data   |
 * |______________|________________|______________|________________|
 * |              |                |              |                |
 * |      ...     |       ...      |     ...      |      ...       |
 * |______________|________________|______________|________________|
 * Segment 2 __________________________________________________________
 * |              |                |              |                |
 * |      ...     |       ...      |     ...      |      ...       |
 * |______________|________________|______________|________________|
 *
 */

/*
 * A file consists of a variable number of segments, with the segment size specified as a power of
 * two. Each segment contains a fixed number of blocks, leading to a variable block size. Every
 * allocation made by the user is an allocation of a certain number of blocks, with enough space for
 * the block header. This is placed at the beginning of each allocation and contains the number of
 * blocks in the allocation (a better name for it would be the allocation header).
 *
 * Block management in each segment is handled by a bitmap which is stored in the segment header at
 * the beginning of each segment. The first segment (segment 0) is special with the file header
 * being placed immediately after an initial block header. This is because the segment size is
 * stored in the file header, and without it we cannot calculate the block size, which is used to
 * locate everything else in the file.
 *
 * To speed up allocations a number of free lists are maintained which categorize each segment based
 * on how many free blocks they have. This means we can avoid trying to allocate from every existing
 * segment before finding out we have to allocate a new segment, or trying to allocate from segments
 * without the required number of free blocks. The downside of this technique is that it doesn't
 * account for fragmentation within the allocation bitmap.
 *
 * Each segment is mapped in separately, and each view is cached. Even after a view becomes inactive
 * (has a reference count of 0) it remains mapped in until the maximum number of inactive views is
 * reached.
 */

#include <ph.h>
#include <filepool.h>

#include <filepoolp.h>

/**
 * Creates or opens a file pool.
 *
 * \param Pool A variable which receives the file pool instance.
 * \param FileHandle A handle to the file.
 * \param ReadOnly TRUE to disallow writes to the file.
 * \param Parameters Parameters for on-disk and runtime structures.
 */
NTSTATUS PhCreateFilePool(
    _Out_ PPH_FILE_POOL *Pool,
    _In_ HANDLE FileHandle,
    _In_ BOOLEAN ReadOnly,
    _In_opt_ PPH_FILE_POOL_PARAMETERS Parameters
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

    // Map in the first segment, set up initial parameters, then remap the first segment.

    if (!NT_SUCCESS(status = PhFppMapRange(pool, 0, PAGE_SIZE, &initialBlock)))
        goto CleanupExit;

    header = (PPH_FP_FILE_HEADER)&initialBlock->Body;

    if (creating)
    {
        header->Magic = PH_FP_MAGIC;
        header->SegmentShift = Parameters->SegmentShift;
        header->SegmentCount = 1;

        for (i = 0; i < PH_FP_FREE_LIST_COUNT; i++)
            header->FreeLists[i] = ULONG_MAX;
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
        segmentHeaderBlock = (PPH_FP_BLOCK_HEADER)PTR_ADD_OFFSET(pool->FirstBlockOfFirstSegment, (pool->FileHeaderBlockSpan << pool->BlockShift));
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

/**
 * Creates or opens a file pool.
 *
 * \param Pool A variable which receives the file pool instance.
 * \param FileName The file name of the file pool.
 * \param ReadOnly TRUE to disallow writes to the file.
 * \param ShareAccess The file access granted to other threads.
 * \param CreateDisposition The action to perform if the file does or does not exist. See
 * PhCreateFileWin32() for more information.
 * \param Parameters Parameters for on-disk and runtime structures.
 */
NTSTATUS PhCreateFilePool2(
    _Out_ PPH_FILE_POOL *Pool,
    _In_ PWSTR FileName,
    _In_ BOOLEAN ReadOnly,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_opt_ PPH_FILE_POOL_PARAMETERS Parameters
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
        NULL,
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

/**
 * Frees resources used by a file pool instance.
 *
 * \param Pool The file pool.
 */
VOID PhDestroyFilePool(
    _In_ _Post_invalid_ PPH_FILE_POOL Pool
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

/**
 * Validates and corrects file pool parameters.
 *
 * \param Parameters The parameters structure which is validated and modified if necessary.
 */
NTSTATUS PhpValidateFilePoolParameters(
    _Inout_ PPH_FILE_POOL_PARAMETERS Parameters
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

/**
 * Creates default file pool parameters.
 *
 * \param Parameters The parameters structure which receives the default parameter values.
 */
VOID PhpSetDefaultFilePoolParameters(
    _Out_ PPH_FILE_POOL_PARAMETERS Parameters
    )
{
    Parameters->SegmentShift = 18; // 256kB
    Parameters->MaximumInactiveViews = 128;
}

/**
 * Allocates a block from a file pool.
 *
 * \param Pool The file pool.
 * \param Size The number of bytes to allocate.
 * \param Rva A variable which receives the relative virtual address of the allocated block.
 *
 * \return A pointer to the allocated block. You must call PhDereferenceFilePool() or
 * PhDereferenceFilePoolByRva() when you no longer need a reference to the block.
 *
 * \remarks The returned pointer is not valid beyond the lifetime of the file pool instance. Use the
 * relative virtual address if you need a permanent reference to the allocated block.
 */
PVOID PhAllocateFilePool(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG Size,
    _Out_opt_ PULONG Rva
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

        while (segmentIndex != ULONG_MAX)
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

/**
 * Frees a block.
 *
 * \param Pool The file pool.
 * \param SegmentIndex The index of the segment containing the block.
 * \param FirstBlock The first block of the segment containing the block.
 * \param Block A pointer to the block.
 */
VOID PhpFreeFilePool(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG SegmentIndex,
    _In_ PPH_FP_BLOCK_HEADER FirstBlock,
    _In_ PVOID Block
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

/**
 * Frees a block allocated by PhAllocateFilePool().
 *
 * \param Pool The file pool.
 * \param Block A pointer to the block. The pointer is no longer valid after you call this function.
 * Do not use PhDereferenceFilePool() or PhDereferenceFilePoolByRva().
 */
VOID PhFreeFilePool(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PVOID Block
    )
{
    PPH_FILE_POOL_VIEW view;
    PPH_FP_BLOCK_HEADER firstBlock;

    view = PhFppFindViewByBase(Pool, Block);

    if (!view)
        PhRaiseStatus(STATUS_INVALID_PARAMETER_2);

    firstBlock = view->Base;
    PhpFreeFilePool(Pool, view->SegmentIndex, firstBlock, Block);
    PhFppDereferenceView(Pool, view);
}

/**
 * Frees a block allocated by PhAllocateFilePool().
 *
 * \param Pool The file pool.
 * \param Rva The relative virtual address of the block.
 */
BOOLEAN PhFreeFilePoolByRva(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG Rva
    )
{
    ULONG segmentIndex;
    ULONG offset;
    PPH_FP_BLOCK_HEADER firstBlock;

    offset = PhFppDecodeRva(Pool, Rva, &segmentIndex);

    if (offset == ULONG_MAX)
        return FALSE;

    firstBlock = PhFppReferenceSegment(Pool, segmentIndex);

    if (!firstBlock)
        return FALSE;

    PhpFreeFilePool(Pool, segmentIndex, firstBlock, PTR_ADD_OFFSET(firstBlock, offset));
    PhFppDereferenceSegment(Pool, segmentIndex);

    return TRUE;
}

/**
 * Increments the reference count for the specified address.
 *
 * \param Pool The file pool.
 * \param Address An address.
 */
VOID PhReferenceFilePool(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PVOID Address
    )
{
    PhFppReferenceSegmentByBase(Pool, Address);
}

/**
 * Decrements the reference count for the specified address.
 *
 * \param Pool The file pool.
 * \param Address An address.
 */
VOID PhDereferenceFilePool(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PVOID Address
    )
{
    PhFppDereferenceSegmentByBase(Pool, Address);
}

/**
 * Obtains a pointer for a relative virtual address, incrementing the reference count of the
 * address.
 *
 * \param Pool The file pool.
 * \param Rva A relative virtual address.
 */
PVOID PhReferenceFilePoolByRva(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG Rva
    )
{
    ULONG segmentIndex;
    ULONG offset;
    PPH_FP_BLOCK_HEADER firstBlock;

    if (Rva == 0)
        return NULL;

    offset = PhFppDecodeRva(Pool, Rva, &segmentIndex);

    if (offset == ULONG_MAX)
        return NULL;

    firstBlock = PhFppReferenceSegment(Pool, segmentIndex);

    if (!firstBlock)
        return NULL;

    return PTR_ADD_OFFSET(firstBlock, offset);
}

/**
 * Decrements the reference count for the specified relative virtual address.
 *
 * \param Pool The file pool.
 * \param Rva A relative virtual address.
 */
BOOLEAN PhDereferenceFilePoolByRva(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG Rva
    )
{
    ULONG segmentIndex;
    ULONG offset;

    offset = PhFppDecodeRva(Pool, Rva, &segmentIndex);

    if (offset == ULONG_MAX)
        return FALSE;

    PhFppDereferenceSegment(Pool, segmentIndex);

    return TRUE;
}

/**
 * Obtains a relative virtual address for a pointer.
 *
 * \param Pool The file pool.
 * \param Address A pointer.
 *
 * \return The relative virtual address.
 *
 * \remarks No reference counts are changed.
 */
ULONG PhEncodeRvaFilePool(
    _In_ PPH_FILE_POOL Pool,
    _In_ PVOID Address
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

/**
 * Retrieves user data.
 *
 * \param Pool The file pool.
 * \param Context A variable which receives the user data.
 */
VOID PhGetUserContextFilePool(
    _In_ PPH_FILE_POOL Pool,
    _Out_ PULONGLONG Context
    )
{
    *Context = Pool->Header->UserContext;
}

/**
 * Stores user data.
 *
 * \param Pool The file pool.
 * \param Context A variable which contains the user data.
 */
VOID PhSetUserContextFilePool(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PULONGLONG Context
    )
{
    Pool->Header->UserContext = *Context;
}

/**
 * Extends a file pool.
 *
 * \param Pool The file pool.
 * \param NewSize The new size of the file, in bytes.
 */
NTSTATUS PhFppExtendRange(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG NewSize
    )
{
    LARGE_INTEGER newSectionSize;

    newSectionSize.QuadPart = NewSize;

    return NtExtendSection(Pool->SectionHandle, &newSectionSize);
}

/**
 * Maps in a view of a file pool.
 *
 * \param Pool The file pool.
 * \param Offset The offset of the view, in bytes.
 * \param Size The size of the view, in bytes.
 * \param Base A variable which receives the base address of the view.
 */
NTSTATUS PhFppMapRange(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG Offset,
    _In_ ULONG Size,
    _Out_ PVOID *Base
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
        ViewUnmap,
        0,
        !Pool->ReadOnly ? PAGE_READWRITE : PAGE_READONLY
        );

    if (NT_SUCCESS(status))
        *Base = baseAddress;

    return status;
}

/**
 * Unmaps a view of a file pool.
 *
 * \param Pool The file pool.
 * \param Base The base address of the view.
 */
NTSTATUS PhFppUnmapRange(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PVOID Base
    )
{
    return NtUnmapViewOfSection(NtCurrentProcess(), Base);
}

/**
 * Initializes a segment.
 *
 * \param Pool The file pool.
 * \param BlockOfSegmentHeader The block header of the span containing the segment header.
 * \param AdditionalBlocksUsed The number of blocks already allocated from the segment, excluding
 * the blocks comprising the segment header.
 */
VOID PhFppInitializeSegment(
    _Inout_ PPH_FILE_POOL Pool,
    _Out_ PPH_FP_BLOCK_HEADER BlockOfSegmentHeader,
    _In_ ULONG AdditionalBlocksUsed
    )
{
    PPH_FP_SEGMENT_HEADER segmentHeader;
    RTL_BITMAP bitmap;

    BlockOfSegmentHeader->Span = Pool->SegmentHeaderBlockSpan;
    segmentHeader = (PPH_FP_SEGMENT_HEADER)&BlockOfSegmentHeader->Body;

    RtlInitializeBitMap(&bitmap, segmentHeader->Bitmap, PH_FP_BLOCK_COUNT);
    RtlSetBits(&bitmap, 0, Pool->SegmentHeaderBlockSpan + AdditionalBlocksUsed);
    segmentHeader->FreeBlocks = PH_FP_BLOCK_COUNT - (Pool->SegmentHeaderBlockSpan + AdditionalBlocksUsed);
    segmentHeader->FreeFlink = ULONG_MAX;
    segmentHeader->FreeBlink = ULONG_MAX;
}

/**
 * Allocates a segment.
 *
 * \param Pool The file pool.
 * \param NewSegmentIndex A variable which receives the index of the new segment.
 *
 * \return A pointer to the first block of the segment.
 */
PPH_FP_BLOCK_HEADER PhFppAllocateSegment(
    _Inout_ PPH_FILE_POOL Pool,
    _Out_ PULONG NewSegmentIndex
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

/**
 * Retrieves the header of a segment.
 *
 * \param Pool The file pool.
 * \param FirstBlock The first block of the segment.
 */
PPH_FP_SEGMENT_HEADER PhFppGetHeaderSegment(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PPH_FP_BLOCK_HEADER FirstBlock
    )
{
    if (FirstBlock != Pool->FirstBlockOfFirstSegment)
    {
        return (PPH_FP_SEGMENT_HEADER)&FirstBlock->Body;
    }
    else
    {
        // In the first segment, the segment header is after the file header.
        return (PPH_FP_SEGMENT_HEADER)&((PPH_FP_BLOCK_HEADER)PTR_ADD_OFFSET(FirstBlock, (Pool->FileHeaderBlockSpan << Pool->BlockShift)))->Body;
    }
}

VOID PhFppAddViewByIndex(
    _Inout_ PPH_FILE_POOL Pool,
    _Inout_ PPH_FILE_POOL_VIEW View
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
    _Inout_ PPH_FILE_POOL Pool,
    _Inout_ PPH_FILE_POOL_VIEW View
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

        // If this was the last entry in the chain, then indicate that the chain is empty.
        // Otherwise, choose a new head.

        if (IsListEmpty(head))
            Pool->ByIndexBuckets[index] = NULL;
        else
            Pool->ByIndexBuckets[index] = head->Flink;
    }
}

/**
 * Finds a view for the specified segment.
 *
 * \param Pool The file pool.
 * \param SegmentIndex The index of the segment.
 *
 * \return The view for the segment, or NULL if no view is present for the segment.
 */
PPH_FILE_POOL_VIEW PhFppFindViewByIndex(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG SegmentIndex
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
    _In_ PPH_AVL_LINKS Links1,
    _In_ PPH_AVL_LINKS Links2
    )
{
    PPH_FILE_POOL_VIEW view1 = CONTAINING_RECORD(Links1, PH_FILE_POOL_VIEW, ByBaseLinks);
    PPH_FILE_POOL_VIEW view2 = CONTAINING_RECORD(Links2, PH_FILE_POOL_VIEW, ByBaseLinks);

    return uintptrcmp((ULONG_PTR)view1->Base, (ULONG_PTR)view2->Base);
}

VOID PhFppAddViewByBase(
    _Inout_ PPH_FILE_POOL Pool,
    _Inout_ PPH_FILE_POOL_VIEW View
    )
{
    PhAddElementAvlTree(&Pool->ByBaseSet, &View->ByBaseLinks);
}

VOID PhFppRemoveViewByBase(
    _Inout_ PPH_FILE_POOL Pool,
    _Inout_ PPH_FILE_POOL_VIEW View
    )
{
    PhRemoveElementAvlTree(&Pool->ByBaseSet, &View->ByBaseLinks);
}

/**
 * Finds a view containing the specified address.
 *
 * \param Pool The file pool.
 * \param Base The address.
 *
 * \return The view containing the address, or NULL if no view is present for the address.
 */
PPH_FILE_POOL_VIEW PhFppFindViewByBase(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PVOID Base
    )
{
    PPH_FILE_POOL_VIEW view;
    PPH_AVL_LINKS links;
    PH_FILE_POOL_VIEW lookupView;

    // This is an approximate search to find the target view in which the specified address lies.

    lookupView.Base = Base;
    links = PhUpperDualBoundElementAvlTree(&Pool->ByBaseSet, &lookupView.ByBaseLinks);

    if (!links)
        return NULL;

    view = CONTAINING_RECORD(links, PH_FILE_POOL_VIEW, ByBaseLinks);

    if ((ULONG_PTR)Base < (ULONG_PTR)view->Base + Pool->SegmentSize)
        return view;

    return NULL;
}

PPH_FILE_POOL_VIEW PhFppCreateView(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG SegmentIndex
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
    _Inout_ PPH_FILE_POOL Pool,
    _Inout_ PPH_FILE_POOL_VIEW View
    )
{
    PhFppUnmapRange(Pool, View->Base);
    PhFppRemoveViewByIndex(Pool, View);
    PhFppRemoveViewByBase(Pool, View);

    PhFreeToFreeList(&Pool->ViewFreeList, View);
}

VOID PhFppActivateView(
    _Inout_ PPH_FILE_POOL Pool,
    _Inout_ PPH_FILE_POOL_VIEW View
    )
{
    RemoveEntryList(&View->InactiveViewsListEntry);
    Pool->NumberOfInactiveViews--;
}

VOID PhFppDeactivateView(
    _Inout_ PPH_FILE_POOL Pool,
    _Inout_ PPH_FILE_POOL_VIEW View
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
    _Inout_ PPH_FILE_POOL Pool,
    _Inout_ PPH_FILE_POOL_VIEW View
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
    _Inout_ PPH_FILE_POOL Pool,
    _Inout_ PPH_FILE_POOL_VIEW View
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
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG SegmentIndex
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
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG SegmentIndex
    )
{
    PPH_FILE_POOL_VIEW view;

    view = PhFppFindViewByIndex(Pool, SegmentIndex);

    if (!view)
        PhRaiseStatus(STATUS_INTERNAL_ERROR);

    PhFppDereferenceView(Pool, view);
}

VOID PhFppReferenceSegmentByBase(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PVOID Base
    )
{
    PPH_FILE_POOL_VIEW view;

    view = PhFppFindViewByBase(Pool, Base);

    if (!view)
        PhRaiseStatus(STATUS_INTERNAL_ERROR);

    PhFppReferenceView(Pool, view);
}

VOID PhFppDereferenceSegmentByBase(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PVOID Base
    )
{
    PPH_FILE_POOL_VIEW view;

    view = PhFppFindViewByBase(Pool, Base);

    if (!view)
        PhRaiseStatus(STATUS_INTERNAL_ERROR);

    PhFppDereferenceView(Pool, view);
}

/**
 * Allocates blocks from a segment.
 *
 * \param Pool The file pool.
 * \param FirstBlock The first block of the segment.
 * \param SegmentHeader The header of the segment.
 * \param NumberOfBlocks The number of blocks to allocate.
 *
 * \return The header of the allocated span, or NULL if there is an insufficient number of
 * contiguous free blocks for the allocation.
 */
PPH_FP_BLOCK_HEADER PhFppAllocateBlocks(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PPH_FP_BLOCK_HEADER FirstBlock,
    _Inout_ PPH_FP_SEGMENT_HEADER SegmentHeader,
    _In_ ULONG NumberOfBlocks
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

    if (foundIndex == ULONG_MAX)
    {
        // No more space.
        return NULL;
    }

    SegmentHeader->FreeBlocks -= NumberOfBlocks;

    blockHeader = (PPH_FP_BLOCK_HEADER)PTR_ADD_OFFSET(FirstBlock, (foundIndex << Pool->BlockShift));
    blockHeader->Flags = 0;
    blockHeader->Span = NumberOfBlocks;

    return blockHeader;
}

/**
 * Frees blocks in a segment.
 *
 * \param Pool The file pool.
 * \param FirstBlock The first block of the segment.
 * \param SegmentHeader The header of the segment.
 * \param BlockHeader The header of the allocated span.
 */
VOID PhFppFreeBlocks(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PPH_FP_BLOCK_HEADER FirstBlock,
    _Inout_ PPH_FP_SEGMENT_HEADER SegmentHeader,
    _In_ PPH_FP_BLOCK_HEADER BlockHeader
    )
{
    RTL_BITMAP bitmap;
    ULONG startIndex;
    ULONG blockSpan;

    RtlInitializeBitMap(&bitmap, SegmentHeader->Bitmap, PH_FP_BLOCK_COUNT);

    // Mark the blocks as free.
    startIndex = PtrToUlong(PTR_SUB_OFFSET(BlockHeader, FirstBlock)) >> Pool->BlockShift;
    blockSpan = BlockHeader->Span;
    RtlClearBits(&bitmap, startIndex, blockSpan);
    SegmentHeader->FreeBlocks += blockSpan;
}

/**
 * Computes the free list index (category) for a specified number of blocks.
 *
 * \param Pool The file pool.
 * \param NumberOfBlocks The number of free or required blocks.
 */
ULONG PhFppComputeFreeListIndex(
    _In_ PPH_FILE_POOL Pool,
    _In_ ULONG NumberOfBlocks
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

/**
 * Inserts a segment into a free list.
 *
 * \param Pool The file pool.
 * \param FreeListIndex The index of a free list.
 * \param SegmentIndex The index of the segment.
 * \param SegmentHeader The header of the segment.
 */
BOOLEAN PhFppInsertFreeList(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG FreeListIndex,
    _In_ ULONG SegmentIndex,
    _In_ PPH_FP_SEGMENT_HEADER SegmentHeader
    )
{
    ULONG oldSegmentIndex;
    PPH_FP_BLOCK_HEADER oldSegmentFirstBlock;
    PPH_FP_SEGMENT_HEADER oldSegmentHeader;

    oldSegmentIndex = Pool->Header->FreeLists[FreeListIndex];

    // Try to reference the segment before we commit any changes.

    if (oldSegmentIndex != ULONG_MAX)
    {
        oldSegmentFirstBlock = PhFppReferenceSegment(Pool, oldSegmentIndex);

        if (!oldSegmentFirstBlock)
            return FALSE;
    }

    // Insert the segment into the list.

    SegmentHeader->FreeBlink = ULONG_MAX;
    SegmentHeader->FreeFlink = oldSegmentIndex;
    Pool->Header->FreeLists[FreeListIndex] = SegmentIndex;

    if (oldSegmentIndex != ULONG_MAX)
    {
        oldSegmentHeader = PhFppGetHeaderSegment(Pool, oldSegmentFirstBlock);
        oldSegmentHeader->FreeBlink = SegmentIndex;
        PhFppDereferenceSegment(Pool, oldSegmentIndex);
    }

    return TRUE;
}

/**
 * Removes a segment from a free list.
 *
 * \param Pool The file pool.
 * \param FreeListIndex The index of a free list.
 * \param SegmentIndex The index of the segment.
 * \param SegmentHeader The header of the segment.
 */
BOOLEAN PhFppRemoveFreeList(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG FreeListIndex,
    _In_ ULONG SegmentIndex,
    _In_ PPH_FP_SEGMENT_HEADER SegmentHeader
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

    if (flinkSegmentIndex != ULONG_MAX)
    {
        flinkSegmentFirstBlock = PhFppReferenceSegment(Pool, flinkSegmentIndex);

        if (!flinkSegmentFirstBlock)
            return FALSE;
    }

    if (blinkSegmentIndex != ULONG_MAX)
    {
        blinkSegmentFirstBlock = PhFppReferenceSegment(Pool, blinkSegmentIndex);

        if (!blinkSegmentFirstBlock)
        {
            if (flinkSegmentIndex != ULONG_MAX)
                PhFppDereferenceSegment(Pool, flinkSegmentIndex);

            return FALSE;
        }
    }

    // Unlink the segment from the list.

    if (flinkSegmentIndex != ULONG_MAX)
    {
        flinkSegmentHeader = PhFppGetHeaderSegment(Pool, flinkSegmentFirstBlock);
        flinkSegmentHeader->FreeBlink = blinkSegmentIndex;
        PhFppDereferenceSegment(Pool, flinkSegmentIndex);
    }

    if (blinkSegmentIndex != ULONG_MAX)
    {
        blinkSegmentHeader = PhFppGetHeaderSegment(Pool, blinkSegmentFirstBlock);
        blinkSegmentHeader->FreeFlink = flinkSegmentIndex;
        PhFppDereferenceSegment(Pool, blinkSegmentIndex);
    }
    else
    {
        // The segment was the list head; select a new one.
        Pool->Header->FreeLists[FreeListIndex] = flinkSegmentIndex;
    }

    return TRUE;
}

/**
 * Retrieves the header of a block.
 *
 * \param Pool The file pool.
 * \param Block A pointer to the body of the block.
 */
PPH_FP_BLOCK_HEADER PhFppGetHeaderBlock(
    _In_ PPH_FILE_POOL Pool,
    _In_ PVOID Block
    )
{
    return CONTAINING_RECORD(Block, PH_FP_BLOCK_HEADER, Body);
}

/**
 * Creates a relative virtual address.
 *
 * \param Pool The file pool.
 * \param SegmentIndex The index of the segment containing \a Address.
 * \param FirstBlock The first block of the segment containing \a Address.
 * \param Address An address.
 */
ULONG PhFppEncodeRva(
    _In_ PPH_FILE_POOL Pool,
    _In_ ULONG SegmentIndex,
    _In_ PPH_FP_BLOCK_HEADER FirstBlock,
    _In_ PVOID Address
    )
{
    return (SegmentIndex << Pool->SegmentShift) + PtrToUlong(PTR_SUB_OFFSET(Address, FirstBlock));
}

/**
 * Decodes a relative virtual address.
 *
 * \param Pool The file pool.
 * \param Rva The relative virtual address.
 * \param SegmentIndex A variable which receives the segment index.
 *
 * \return An offset into the segment specified by \a SegmentIndex, or -1 if \a Rva is invalid.
 */
ULONG PhFppDecodeRva(
    _In_ PPH_FILE_POOL Pool,
    _In_ ULONG Rva,
    _Out_ PULONG SegmentIndex
    )
{
    ULONG segmentIndex;

    segmentIndex = Rva >> Pool->SegmentShift;

    if (segmentIndex >= Pool->Header->SegmentCount)
        return ULONG_MAX;

    *SegmentIndex = segmentIndex;

    return Rva & (Pool->SegmentSize - 1);
}
