/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2013
 *
 */

#ifndef _PH_FILEPOOLP_H
#define _PH_FILEPOOLP_H

typedef struct _PH_FILE_POOL_VIEW
{
    LIST_ENTRY ByIndexListEntry;
    PH_AVL_LINKS ByBaseLinks;
    LIST_ENTRY InactiveViewsListEntry;

    ULONG RefCount;
    ULONG SegmentIndex;
    PVOID Base;
} PH_FILE_POOL_VIEW, *PPH_FILE_POOL_VIEW;

NTSTATUS PhpValidateFilePoolParameters(
    _Inout_ PPH_FILE_POOL_PARAMETERS Parameters
    );

VOID PhpSetDefaultFilePoolParameters(
    _Out_ PPH_FILE_POOL_PARAMETERS Parameters
    );

// Range mapping

NTSTATUS PhFppExtendRange(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG NewSize
    );

NTSTATUS PhFppMapRange(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG Offset,
    _In_ ULONG Size,
    _Out_ PVOID *Base
    );

NTSTATUS PhFppUnmapRange(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PVOID Base
    );

// Segments

VOID PhFppInitializeSegment(
    _Inout_ PPH_FILE_POOL Pool,
    _Out_ PPH_FP_BLOCK_HEADER BlockOfSegmentHeader,
    _In_ ULONG AdditionalBlocksUsed
    );

PPH_FP_BLOCK_HEADER PhFppAllocateSegment(
    _Inout_ PPH_FILE_POOL Pool,
    _Out_ PULONG NewSegmentIndex
    );

PPH_FP_SEGMENT_HEADER PhFppGetHeaderSegment(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PPH_FP_BLOCK_HEADER FirstBlock
    );

// Views

VOID PhFppAddViewByIndex(
    _Inout_ PPH_FILE_POOL Pool,
    _Inout_ PPH_FILE_POOL_VIEW View
    );

VOID PhFppRemoveViewByIndex(
    _Inout_ PPH_FILE_POOL Pool,
    _Inout_ PPH_FILE_POOL_VIEW View
    );

PPH_FILE_POOL_VIEW PhFppFindViewByIndex(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG SegmentIndex
    );

LONG NTAPI PhpFilePoolViewByBaseCompareFunction(
    _In_ PPH_AVL_LINKS Links1,
    _In_ PPH_AVL_LINKS Links2
    );

VOID PhFppAddViewByBase(
    _Inout_ PPH_FILE_POOL Pool,
    _Inout_ PPH_FILE_POOL_VIEW View
    );

VOID PhFppRemoveViewByBase(
    _Inout_ PPH_FILE_POOL Pool,
    _Inout_ PPH_FILE_POOL_VIEW View
    );

PPH_FILE_POOL_VIEW PhFppFindViewByBase(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PVOID Base
    );

PPH_FILE_POOL_VIEW PhFppCreateView(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG SegmentIndex
    );

VOID PhFppDestroyView(
    _Inout_ PPH_FILE_POOL Pool,
    _Inout_ PPH_FILE_POOL_VIEW View
    );

VOID PhFppActivateView(
    _Inout_ PPH_FILE_POOL Pool,
    _Inout_ PPH_FILE_POOL_VIEW View
    );

VOID PhFppDeactivateView(
    _Inout_ PPH_FILE_POOL Pool,
    _Inout_ PPH_FILE_POOL_VIEW View
    );

VOID PhFppReferenceView(
    _Inout_ PPH_FILE_POOL Pool,
    _Inout_ PPH_FILE_POOL_VIEW View
    );

VOID PhFppDereferenceView(
    _Inout_ PPH_FILE_POOL Pool,
    _Inout_ PPH_FILE_POOL_VIEW View
    );

PPH_FP_BLOCK_HEADER PhFppReferenceSegment(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG SegmentIndex
    );

VOID PhFppDereferenceSegment(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG SegmentIndex
    );

VOID PhFppReferenceSegmentByBase(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PVOID Base
    );

VOID PhFppDereferenceSegmentByBase(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PVOID Base
    );

// Bitmap allocation

PPH_FP_BLOCK_HEADER PhFppAllocateBlocks(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PPH_FP_BLOCK_HEADER FirstBlock,
    _Inout_ PPH_FP_SEGMENT_HEADER SegmentHeader,
    _In_ ULONG NumberOfBlocks
    );

VOID PhFppFreeBlocks(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PPH_FP_BLOCK_HEADER FirstBlock,
    _Inout_ PPH_FP_SEGMENT_HEADER SegmentHeader,
    _In_ PPH_FP_BLOCK_HEADER BlockHeader
    );

// Free list

ULONG PhFppComputeFreeListIndex(
    _In_ PPH_FILE_POOL Pool,
    _In_ ULONG NumberOfBlocks
    );

BOOLEAN PhFppInsertFreeList(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG FreeListIndex,
    _In_ ULONG SegmentIndex,
    _In_ PPH_FP_SEGMENT_HEADER SegmentHeader
    );

BOOLEAN PhFppRemoveFreeList(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG FreeListIndex,
    _In_ ULONG SegmentIndex,
    _In_ PPH_FP_SEGMENT_HEADER SegmentHeader
    );

// Misc.

PPH_FP_BLOCK_HEADER PhFppGetHeaderBlock(
    _In_ PPH_FILE_POOL Pool,
    _In_ PVOID Block
    );

ULONG PhFppEncodeRva(
    _In_ PPH_FILE_POOL Pool,
    _In_ ULONG SegmentIndex,
    _In_ PPH_FP_BLOCK_HEADER FirstBlock,
    _In_ PVOID Address
    );

ULONG PhFppDecodeRva(
    _In_ PPH_FILE_POOL Pool,
    _In_ ULONG Rva,
    _Out_ PULONG SegmentIndex
    );

#endif
