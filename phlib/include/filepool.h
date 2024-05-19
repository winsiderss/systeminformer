/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2016
 *
 */

#ifndef _PH_FILEPOOL_H
#define _PH_FILEPOOL_H

#ifdef __cplusplus
extern "C" {
#endif

// On-disk structures

// Each file has at least one segment. Each segment has a number of blocks, which are allocated from
// a bitmap. The segment header is always in the first block of each segment, except for the first
// segment. In the first segment, the file header is in the first few blocks, followed by the
// segment header.
//
// The segments are placed in a particular free list depending on how many blocks they have free;
// this allows allocators to simply skip the segments which don't have enough segments free, and
// allocate new segments if necessary. The free list does not however guarantee that a particular
// segment has a particular number of contiguous blocks free; low performance can still occur when
// there is fragmentation.

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

    /**
     * The base-2 logarithm of the size of each segment. This value must be between 16 and 28,
     * inclusive.
     */
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

PHLIBAPI
NTSTATUS PhCreateFilePool(
    _Out_ PPH_FILE_POOL *Pool,
    _In_ HANDLE FileHandle,
    _In_ BOOLEAN ReadOnly,
    _In_opt_ PPH_FILE_POOL_PARAMETERS Parameters
    );

PHLIBAPI
NTSTATUS PhCreateFilePool2(
    _Out_ PPH_FILE_POOL *Pool,
    _In_ PWSTR FileName,
    _In_ BOOLEAN ReadOnly,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_opt_ PPH_FILE_POOL_PARAMETERS Parameters
    );

PHLIBAPI
VOID PhDestroyFilePool(
    _In_ _Post_invalid_ PPH_FILE_POOL Pool
    );

PHLIBAPI
PVOID PhAllocateFilePool(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG Size,
    _Out_opt_ PULONG Rva
    );

PHLIBAPI
VOID PhFreeFilePool(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PVOID Block
    );

PHLIBAPI
BOOLEAN PhFreeFilePoolByRva(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG Rva
    );

PHLIBAPI
VOID PhReferenceFilePool(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PVOID Address
    );

PHLIBAPI
VOID PhDereferenceFilePool(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PVOID Address
    );

PHLIBAPI
PVOID PhReferenceFilePoolByRva(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG Rva
    );

PHLIBAPI
BOOLEAN PhDereferenceFilePoolByRva(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ ULONG Rva
    );

PHLIBAPI
ULONG PhEncodeRvaFilePool(
    _In_ PPH_FILE_POOL Pool,
    _In_ PVOID Address
    );

PHLIBAPI
VOID PhGetUserContextFilePool(
    _In_ PPH_FILE_POOL Pool,
    _Out_ PULONGLONG Context
    );

PHLIBAPI
VOID PhSetUserContextFilePool(
    _Inout_ PPH_FILE_POOL Pool,
    _In_ PULONGLONG Context
    );

#ifdef __cplusplus
}
#endif

#endif
