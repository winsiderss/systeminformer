/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2015-2016
 *     dmex    2017-2022
 *
 */

#ifndef PH_HEAPSTRUCT_H
#define PH_HEAPSTRUCT_H

// Not the actual structure, but has the same size.
typedef struct _HEAP_ENTRY
{
    PVOID Data1;
    PVOID Data2;
} HEAP_ENTRY, *PHEAP_ENTRY;

#define HEAP_SEGMENT_SIGNATURE 0xffeeffee

// Windows 7 and above
typedef struct _HEAP_SEGMENT
{
    HEAP_ENTRY Entry;
    ULONG SegmentSignature;
    ULONG SegmentFlags;
    LIST_ENTRY SegmentListEntry;
    struct _HEAP *Heap;
    PVOID BaseAddress;
    ULONG NumberOfPages;
    PHEAP_ENTRY FirstEntry;
    PHEAP_ENTRY LastValidEntry;
    ULONG NumberOfUnCommittedPages;
    ULONG NumberOfUnCommittedRanges;
    USHORT SegmentAllocatorBackTraceIndex;
    USHORT Reserved;
    LIST_ENTRY UCRSegmentList;
} HEAP_SEGMENT, *PHEAP_SEGMENT;

#define HEAP_SIGNATURE 0xeeffeeff

// Windows 7
typedef struct _HEAP_OLD
{
    HEAP_SEGMENT Segment;
    ULONG Flags;
    ULONG ForceFlags;
    ULONG CompatibilityFlags;
    ULONG EncodeFlagMask;
    HEAP_ENTRY Encoding;
    ULONG_PTR PointerKey; // Windows 7 only
    ULONG Interceptor;
    ULONG VirtualMemoryThreshold;
    ULONG Signature;
    // ...
} HEAP_OLD, *PHEAP_OLD;

// Windows 8 and above
typedef struct _HEAP_WIN8
{
    HEAP_SEGMENT Segment;
    ULONG Flags;
    ULONG ForceFlags;
    ULONG CompatibilityFlags;
    ULONG EncodeFlagMask;
    HEAP_ENTRY Encoding;
    ULONG Interceptor;
    ULONG VirtualMemoryThreshold;
    ULONG Signature;
    // ...
} HEAP_WIN8, *PHEAP_WIN8;

//
// Windows 10 and above
//

typedef struct _RTLP_HEAP_COMMIT_LIMIT_DATA
{
    SIZE_T CommitLimitBytes;
    SIZE_T CommitLimitFailureCode;
} RTLP_HEAP_COMMIT_LIMIT_DATA, *PRTLP_HEAP_COMMIT_LIMIT_DATA;

typedef struct _HEAP_PSEUDO_TAG_ENTRY
{
    SIZE_T CommitLimitBytes;
    SIZE_T CommitLimitFailureCode;
} HEAP_PSEUDO_TAG_ENTRY, *PHEAP_PSEUDO_TAG_ENTRY;

typedef struct _HEAP_TUNING_PARAMETERS
{
    ULONG CommittThresholdShift;
    SIZE_T MaxPreCommittThreshold;
} HEAP_TUNING_PARAMETERS, *PHEAP_TUNING_PARAMETERS;

typedef struct _HEAP_COUNTERS
{
    SIZE_T TotalMemoryReserved;
    SIZE_T TotalMemoryCommitted;
    SIZE_T TotalMemoryLargeUCR;
    SIZE_T TotalSizeInVirtualBlocks;
    ULONG TotalSegments;
    ULONG TotalUCRs;
    ULONG CommittOps;
    ULONG DeCommitOps;
    ULONG LockAcquires;
    ULONG LockCollisions;
    ULONG CommitRate;
    ULONG DecommittRate;
    ULONG CommitFailures;
    ULONG InBlockCommitFailures;
    ULONG PollIntervalCounter;
    ULONG DecommitsSinceLastCheck;
    ULONG HeapPollInterval;
    ULONG AllocAndFreeOps;
    ULONG AllocationIndicesActive;
    ULONG InBlockDeccommits;
    SIZE_T InBlockDeccomitSize;
    SIZE_T HighWatermarkSize;
    SIZE_T LastPolledSize;
} HEAP_COUNTERS, *PHEAP_COUNTERS;

typedef struct _HEAP
{
    HEAP_SEGMENT Segment;
    HEAP_ENTRY Entry;
    ULONG SegmentSignature;
    ULONG SegmentFlags;
    LIST_ENTRY SegmentListEntry;
    struct _HEAP* Heap;
    PVOID BaseAddress;
    ULONG NumberOfPages;
    PHEAP_ENTRY FirstEntry;
    PHEAP_ENTRY LastValidEntry;
    ULONG NumberOfUnCommittedPages;
    ULONG NumberOfUnCommittedRanges;
    USHORT SegmentAllocatorBackTraceIndex;
    USHORT Reserved;
    LIST_ENTRY UCRSegmentList;
    ULONG Flags;
    ULONG ForceFlags;
    ULONG CompatibilityFlags;
    ULONG EncodeFlagMask;
    HEAP_ENTRY Encoding;
    ULONG Interceptor;
    ULONG VirtualMemoryThreshold;
    ULONG Signature;
    SIZE_T SegmentReserve;
    SIZE_T SegmentCommit;
    SIZE_T DeCommitFreeBlockThreshold;
    SIZE_T DeCommitTotalFreeThreshold;
    SIZE_T TotalFreeSize;
    SIZE_T MaximumAllocationSize;
    USHORT ProcessHeapsListIndex;
    USHORT HeaderValidateLength;
    PVOID HeaderValidateCopy;
    USHORT NextAvailableTagIndex;
    USHORT MaximumTagIndex;
    struct _HEAP_TAG_ENTRY* TagEntries;
    LIST_ENTRY UCRList;
    SIZE_T AlignRound;
    SIZE_T AlignMask;
    LIST_ENTRY VirtualAllocdBlocks;
    LIST_ENTRY SegmentList;
    USHORT AllocatorBackTraceIndex;
    ULONG NonDedicatedListLength;
    PVOID BlocksIndex;
    PVOID UCRIndex;
    PHEAP_PSEUDO_TAG_ENTRY PseudoTagEntries;
    LIST_ENTRY FreeLists;
    struct _HEAP_LOCK* LockVariable;
    long (*CommitRoutine)(void);
    RTL_RUN_ONCE StackTraceInitVar;
    RTLP_HEAP_COMMIT_LIMIT_DATA CommitLimitData;
    PVOID UserContext;
    ULONG_PTR Spare;
    PVOID FrontEndHeap;
    USHORT FrontHeapLockCount;
    UCHAR FrontEndHeapType;
    UCHAR RequestedFrontEndHeapType;
    PUSHORT FrontEndHeapUsageData;
    USHORT FrontEndHeapMaximumIndex;
    UCHAR FrontEndHeapStatusBitmap[129];
    UCHAR ReadOnly : 1;
    UCHAR InternalFlags;
    HEAP_COUNTERS Counters;
    HEAP_TUNING_PARAMETERS TuningParameters;
} HEAP;


#define SEGMENT_HEAP_SIGNATURE 0xddeeddee

// Windows 8.1 and above
typedef struct _SEGMENT_HEAP_WIN8
{
    ULONG_PTR Padding[2];
    ULONG Signature;
    ULONG GlobalFlags;
    // ...
} SEGMENT_HEAP_WIN8, PSEGMENT_HEAP_WIN8;

//
// Windows 10 and above
//

typedef struct _RTL_HP_ENV_HANDLE
{
    ULONG_PTR h[2];
} RTL_HP_ENV_HANDLE, PRTL_HP_ENV_HANDLE;

typedef struct _HEAP_OPPORTUNISTIC_LARGE_PAGE_STATS
{
    SIZE_T SmallPagesInUseWithinLarge;
    SIZE_T OpportunisticLargePageCount;
} HEAP_OPPORTUNISTIC_LARGE_PAGE_STATS, PHEAP_OPPORTUNISTIC_LARGE_PAGE_STATS;

typedef struct _RTL_HP_SEG_ALLOC_POLICY
{
    SIZE_T SmallPagesInUseWithinLarge;
    SIZE_T OpportunisticLargePageCount;
} RTL_HP_SEG_ALLOC_POLICY, PRTL_HP_SEG_ALLOC_POLICY;

typedef struct _HEAP_RUNTIME_MEMORY_STATS
{
    SIZE_T TotalReservedPages;
    SIZE_T TotalCommittedPages;
    SIZE_T FreeCommittedPages;
    SIZE_T LfhFreeCommittedPages;
    SIZE_T VsFreeCommittedPages;
    HEAP_OPPORTUNISTIC_LARGE_PAGE_STATS LargePageStats[2];
    RTL_HP_SEG_ALLOC_POLICY LargePageUtilizationPolicy;
} HEAP_RUNTIME_MEMORY_STATS, PHEAP_RUNTIME_MEMORY_STATS;

typedef struct _SEGMENT_HEAP
{
    RTL_HP_ENV_HANDLE EnvHandle;
    ULONG Signature;
    ULONG GlobalFlags;
    ULONG Interceptor;
    USHORT ProcessHeapListIndex;
    struct
    {
        USHORT AllocatedFromMetadata : 1;
        USHORT ReadOnly : 1;
        USHORT InternalFlags : 14;
    };
    RTLP_HEAP_COMMIT_LIMIT_DATA CommitLimitData;
    SIZE_T ReservedMustBeZero;
    PVOID UserContext;
    SIZE_T LargeMetadataLock;
    RTL_RB_TREE LargeAllocMetadata;
    SIZE_T LargeReservedPages;
    SIZE_T LargeCommittedPages;
    SIZE_T Tag;
    RTL_RUN_ONCE StackTraceInitVar;
    HEAP_RUNTIME_MEMORY_STATS MemStats;
    ULONG GlobalLockOwner;
    SIZE_T ContextExtendLock;
    PBYTE AllocatedBase;
    PBYTE UncommittedBase;
    PBYTE ReservedLimit;
    PBYTE ReservedRegionEnd;
    //RTL_HP_HEAP_VA_CALLBACKS_ENCODED CallbacksEncoded;
    //HEAP_SEG_CONTEXT SegContexts[2];
    //HEAP_VS_CONTEXT VsContext;
    //HEAP_LFH_CONTEXT LfhContext;
} SEGMENT_HEAP, PSEGMENT_HEAP;

//
// 32-bit versions
//

typedef struct _HEAP_ENTRY32
{
    WOW64_POINTER(PVOID) Data1;
    WOW64_POINTER(PVOID) Data2;
} HEAP_ENTRY32, *PHEAP_ENTRY32;

typedef struct _HEAP_SEGMENT32
{
    HEAP_ENTRY32 HeapEntry;
    ULONG SegmentSignature;
    ULONG SegmentFlags;
    LIST_ENTRY32 SegmentListEntry;
    WOW64_POINTER(struct _HEAP32 *) Heap;
    WOW64_POINTER(PVOID) BaseAddress;
    ULONG NumberOfPages;
    WOW64_POINTER(PHEAP_ENTRY32) FirstEntry;
    WOW64_POINTER(PHEAP_ENTRY32) LastValidEntry;
    ULONG NumberOfUnCommittedPages;
    ULONG NumberOfUnCommittedRanges;
    USHORT SegmentAllocatorBackTraceIndex;
    USHORT Reserved;
    LIST_ENTRY32 UCRSegmentList;
} HEAP_SEGMENT32, *PHEAP_SEGMENT32;

typedef struct _HEAP_OLD32
{
    HEAP_SEGMENT32 Segment;
    ULONG Flags;
    ULONG ForceFlags;
    ULONG CompatibilityFlags;
    ULONG EncodeFlagMask;
    HEAP_ENTRY32 Encoding;
    WOW64_POINTER(ULONG_PTR) PointerKey;
    ULONG Interceptor;
    ULONG VirtualMemoryThreshold;
    ULONG Signature;
    // ...
} HEAP_OLD32, *PHEAP_OLD32;

typedef struct _HEAP32
{
    HEAP_SEGMENT32 Segment;
    ULONG Flags;
    ULONG ForceFlags;
    ULONG CompatibilityFlags;
    ULONG EncodeFlagMask;
    HEAP_ENTRY32 Encoding;
    ULONG Interceptor;
    ULONG VirtualMemoryThreshold;
    ULONG Signature;
    // ...
} HEAP32, *PHEAP32;

typedef struct _SEGMENT_HEAP32
{
    WOW64_POINTER(ULONG_PTR) Padding[2];
    ULONG Signature;
    ULONG GlobalFlags;
    // ...
} SEGMENT_HEAP32, PSEGMENT_HEAP32;

typedef union _PH_ANY_HEAP
{
    HEAP_OLD HeapOld;
    HEAP_OLD32 HeapOld32;
    HEAP Heap;
    HEAP32 Heap32;
    SEGMENT_HEAP SegmentHeap;
    SEGMENT_HEAP32 SegmentHeap32;
} PH_ANY_HEAP;

#define HEAP_SEGMENT_MAX_SIZE \
    (max(sizeof(HEAP_SEGMENT), sizeof(HEAP_SEGMENT32)))

#endif
