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
    __inout PPH_FILE_POOL_PARAMETERS Parameters
    );

VOID PhpSetDefaultFilePoolParameters(
    __out PPH_FILE_POOL_PARAMETERS Parameters
    );

// Range mapping

NTSTATUS PhFppExtendRange(
    __inout PPH_FILE_POOL Pool,
    __in ULONG NewSize
    );

NTSTATUS PhFppMapRange(
    __inout PPH_FILE_POOL Pool,
    __in ULONG Offset,
    __in ULONG Size,
    __out PVOID *Base
    );

NTSTATUS PhFppUnmapRange(
    __inout PPH_FILE_POOL Pool,
    __in PVOID Base
    );

// Segments

VOID PhFppInitializeSegment(
    __inout PPH_FILE_POOL Pool,
    __out PPH_FP_BLOCK_HEADER BlockOfSegmentHeader,
    __in ULONG AdditionalBlocksUsed
    );

PPH_FP_BLOCK_HEADER PhFppAllocateSegment(
    __inout PPH_FILE_POOL Pool,
    __out PULONG NewSegmentIndex
    );

PPH_FP_SEGMENT_HEADER PhFppGetHeaderSegment(
    __inout PPH_FILE_POOL Pool,
    __in PPH_FP_BLOCK_HEADER FirstBlock
    );

// Views

VOID PhFppAddViewByIndex(
    __inout PPH_FILE_POOL Pool,
    __inout PPH_FILE_POOL_VIEW View
    );

VOID PhFppRemoveViewByIndex(
    __inout PPH_FILE_POOL Pool,
    __inout PPH_FILE_POOL_VIEW View
    );

PPH_FILE_POOL_VIEW PhFppFindViewByIndex(
    __inout PPH_FILE_POOL Pool,
    __in ULONG SegmentIndex
    );

LONG NTAPI PhpFilePoolViewByBaseCompareFunction(
    __in PPH_AVL_LINKS Links1,
    __in PPH_AVL_LINKS Links2
    );

VOID PhFppAddViewByBase(
    __inout PPH_FILE_POOL Pool,
    __inout PPH_FILE_POOL_VIEW View
    );

VOID PhFppRemoveViewByBase(
    __inout PPH_FILE_POOL Pool,
    __inout PPH_FILE_POOL_VIEW View
    );

PPH_FILE_POOL_VIEW PhFppFindViewByBase(
    __inout PPH_FILE_POOL Pool,
    __in PVOID Base
    );

PPH_FILE_POOL_VIEW PhFppCreateView(
    __inout PPH_FILE_POOL Pool,
    __in ULONG SegmentIndex
    );

VOID PhFppDestroyView(
    __inout PPH_FILE_POOL Pool,
    __inout PPH_FILE_POOL_VIEW View
    );

VOID PhFppActivateView(
    __inout PPH_FILE_POOL Pool,
    __inout PPH_FILE_POOL_VIEW View
    );

VOID PhFppDeactivateView(
    __inout PPH_FILE_POOL Pool,
    __inout PPH_FILE_POOL_VIEW View
    );

VOID PhFppReferenceView(
    __inout PPH_FILE_POOL Pool,
    __inout PPH_FILE_POOL_VIEW View
    );

VOID PhFppDereferenceView(
    __inout PPH_FILE_POOL Pool,
    __inout PPH_FILE_POOL_VIEW View
    );

PPH_FP_BLOCK_HEADER PhFppReferenceSegment(
    __inout PPH_FILE_POOL Pool,
    __in ULONG SegmentIndex
    );

VOID PhFppDereferenceSegment(
    __inout PPH_FILE_POOL Pool,
    __in ULONG SegmentIndex
    );

VOID PhFppReferenceSegmentByBase(
    __inout PPH_FILE_POOL Pool,
    __in PVOID Base
    );

VOID PhFppDereferenceSegmentByBase(
    __inout PPH_FILE_POOL Pool,
    __in PVOID Base
    );

// Bitmap allocation

PPH_FP_BLOCK_HEADER PhFppAllocateBlocks(
    __inout PPH_FILE_POOL Pool,
    __in PPH_FP_BLOCK_HEADER FirstBlock,
    __inout PPH_FP_SEGMENT_HEADER SegmentHeader,
    __in ULONG NumberOfBlocks
    );

VOID PhFppFreeBlocks(
    __inout PPH_FILE_POOL Pool,
    __in PPH_FP_BLOCK_HEADER FirstBlock,
    __inout PPH_FP_SEGMENT_HEADER SegmentHeader,
    __in PPH_FP_BLOCK_HEADER BlockHeader
    );

// Free list

ULONG PhFppComputeFreeListIndex(
    __in PPH_FILE_POOL Pool,
    __in ULONG NumberOfBlocks
    );

BOOLEAN PhFppInsertFreeList(
    __inout PPH_FILE_POOL Pool,
    __in ULONG FreeListIndex,
    __in ULONG SegmentIndex,
    __in PPH_FP_SEGMENT_HEADER SegmentHeader
    );

BOOLEAN PhFppRemoveFreeList(
    __inout PPH_FILE_POOL Pool,
    __in ULONG FreeListIndex,
    __in ULONG SegmentIndex,
    __in PPH_FP_SEGMENT_HEADER SegmentHeader
    );

// Misc.

PPH_FP_BLOCK_HEADER PhFppGetHeaderBlock(
    __in PPH_FILE_POOL Pool,
    __in PVOID Block
    );

ULONG PhFppEncodeRva(
    __in PPH_FILE_POOL Pool,
    __in ULONG SegmentIndex,
    __in PPH_FP_BLOCK_HEADER FirstBlock,
    __in PVOID Address
    );

ULONG PhFppDecodeRva(
    __in PPH_FILE_POOL Pool,
    __in ULONG Rva,
    __out PULONG SegmentIndex
    );

#endif
