#ifndef _PH_FILEPOOL_H
#define _PH_FILEPOOL_H

// On-disk structures

// Each file has at least one segment.
// Each segment has a number of blocks, which are allocated
// from a bitmap. The segment header is always in the first block
// of each segment, except for the first segment. In the first segment,
// the file header is in the first few blocks, followed by the segment header.
//
// The segments are placed in a particular free list depending on how many
// blocks they have free; this allows allocators to simply skip the segments
// which don't have enough segments free, and allocate new segments if necessary.
// The free list does not however guarantee that a particular segment has
// a particular number of contiguous blocks free; low performance can still
// occur when there is fragmentation.

/** The number of 32-bit integers used for each allocation bitmap. */
#define PH_FP_BITMAP_SIZE 64
/** The power-of-two index of the bitmap size. */
#define PH_FP_BITMAP_SIZE_SHIFT 6
/** The number of blocks that are available in each segment. */
#define PH_FP_BLOCK_COUNT (PH_FP_BITMAP_SIZE * 32)
/** The power-of-two index of the block count. */
#define PH_FP_BLOCK_COUNT_SHIFT (PH_FP_BITMAP_SIZE_SHIFT + 5)
/** The number of free lists for segments. */
#define PH_FP_FREE_LIST_COUNT 8

// Block flags
/** The block is the beginning of a large allocation (one that spans several segments). */
#define PH_FP_BLOCK_LARGE_ALLOCATION 0x1

typedef struct _PH_FP_BLOCK_HEADER
{
    ULONG Flags; // PH_FP_BLOCK_*
    /** The number of blocks in the entire logical block, or the number
     * of segments in a large allocation. */
    ULONG Span;
    ULONGLONG Body;
} PH_FP_BLOCK_HEADER, *PPH_FP_BLOCK_HEADER;

typedef struct _PH_FP_SEGMENT_HEADER
{
    ULONG Bitmap[PH_FP_BITMAP_SIZE];
    ULONG FreeBlocks;
    ULONG FreeFlink;
    ULONG FreeBlink;
    ULONG Reserved[13];
} PH_FP_SEGMENT_HEADER, *PPH_FP_SEGMENT_HEADER;

#define PH_FP_MAGIC ('loPF')

typedef struct _PH_FP_FILE_HEADER
{
    ULONG Magic;
    ULONG SegmentShift;
    ULONG SegmentCount;
    ULONGLONG UserContext;
    ULONG FreeLists[PH_FP_FREE_LIST_COUNT];
} PH_FP_FILE_HEADER, *PPH_FP_FILE_HEADER;

// Runtime

typedef struct _PH_FILE_POOL_PARAMETERS
{
    // File options

    /** The base-2 logarithm of the size of each segment. This value
     * must be between 16 and 28, inclusive. */
    ULONG SegmentShift;

    // Runtime options

    /** The maximum number of inactive segments to keep mapped. */
    ULONG MaximumInactiveViews;
} PH_FILE_POOL_PARAMETERS, *PPH_FILE_POOL_PARAMETERS;

typedef struct _PH_FILE_POOL
{
    HANDLE FileHandle;
    HANDLE SectionHandle;
    BOOLEAN ReadOnly;

    PH_FREE_LIST ViewFreeList;
    PLIST_ENTRY *ByIndexBuckets;
    ULONG ByIndexSize;
    PH_AVL_TREE ByBaseSet;

    ULONG MaximumInactiveViews;
    ULONG NumberOfInactiveViews;
    LIST_ENTRY InactiveViewsListHead;

    PPH_FP_BLOCK_HEADER FirstBlockOfFirstSegment;
    PPH_FP_FILE_HEADER Header;
    ULONG SegmentShift; // The power-of-two size of each segment
    ULONG SegmentSize; // The size of each segment
    ULONG BlockShift; // The power-of-two size of each block in each segment
    ULONG BlockSize; // The size of each block in each segment
    ULONG FileHeaderBlockSpan; // The number of blocks needed to store a file header
    ULONG SegmentHeaderBlockSpan; // The number of blocks needed to store a segment header
} PH_FILE_POOL, *PPH_FILE_POOL;

NTSTATUS PhCreateFilePool(
    __out PPH_FILE_POOL *Pool,
    __in HANDLE FileHandle,
    __in BOOLEAN ReadOnly,
    __in_opt PPH_FILE_POOL_PARAMETERS Parameters
    );

NTSTATUS PhCreateFilePool2(
    __out PPH_FILE_POOL *Pool,
    __in PWSTR FileName,
    __in BOOLEAN ReadOnly,
    __in ULONG ShareAccess,
    __in ULONG CreateDisposition,
    __in_opt PPH_FILE_POOL_PARAMETERS Parameters
    );

VOID PhDestroyFilePool(
    __in __post_invalid PPH_FILE_POOL Pool
    );

PVOID PhAllocateFilePool(
    __inout PPH_FILE_POOL Pool,
    __in ULONG Size,
    __out_opt PULONG Rva
    );

VOID PhFreeFilePool(
    __inout PPH_FILE_POOL Pool,
    __in PVOID Block
    );

BOOLEAN PhFreeFilePoolByRva(
    __inout PPH_FILE_POOL Pool,
    __in ULONG Rva
    );

VOID PhReferenceFilePool(
    __inout PPH_FILE_POOL Pool,
    __in PVOID Address
    );

VOID PhDereferenceFilePool(
    __inout PPH_FILE_POOL Pool,
    __in PVOID Address
    );

PVOID PhReferenceFilePoolByRva(
    __inout PPH_FILE_POOL Pool,
    __in ULONG Rva
    );

BOOLEAN PhDereferenceFilePoolByRva(
    __inout PPH_FILE_POOL Pool,
    __in ULONG Rva
    );

ULONG PhEncodeRvaFilePool(
    __in PPH_FILE_POOL Pool,
    __in PVOID Address
    );

VOID PhGetUserContextFilePool(
    __in PPH_FILE_POOL Pool,
    __out PULONGLONG Context
    );

VOID PhSetUserContextFilePool(
    __inout PPH_FILE_POOL Pool,
    __in PULONGLONG Context
    );

#endif
