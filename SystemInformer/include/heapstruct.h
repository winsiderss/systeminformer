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
typedef struct _HEAP
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
} HEAP, *PHEAP;

#define SEGMENT_HEAP_SIGNATURE 0xddeeddee

// Windows 8.1 and above
typedef struct _SEGMENT_HEAP
{
    ULONG_PTR Padding[2];
    ULONG Signature;
    ULONG GlobalFlags;
    // ...
} SEGMENT_HEAP, PSEGMENT_HEAP;

// 32-bit versions

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
