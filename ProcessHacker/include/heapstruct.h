#ifndef PH_HEAPSTRUCT_H
#define PH_HEAPSTRUCT_H

// Not the actual structure, but has the same size.
typedef struct _HEAP_ENTRY
{
    PVOID Data1;
    PVOID Data2;
} HEAP_ENTRY, *PHEAP_ENTRY;

#define HEAP_SEGMENT_SIGNATURE 0xffeeffee

// First few fields of HEAP_SEGMENT, VISTA and above
typedef struct _HEAP_SEGMENT
{
    HEAP_ENTRY HeapEntry;
    ULONG SegmentSignature;
    ULONG SegmentFlags;
    LIST_ENTRY SegmentListEntry;
    struct _HEAP *Heap;

    // ...
} HEAP_SEGMENT, *PHEAP_SEGMENT;

// First few fields of HEAP_SEGMENT, WS03 and below
typedef struct _HEAP_SEGMENT_OLD
{
    HEAP_ENTRY Entry;
    ULONG Signature;
    ULONG Flags;
    struct _HEAP *Heap;

    // ...
} HEAP_SEGMENT_OLD, *PHEAP_SEGMENT_OLD;

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
    WOW64_POINTER(struct _HEAP *) Heap;

    // ...
} HEAP_SEGMENT32, *PHEAP_SEGMENT32;

typedef struct _HEAP_SEGMENT_OLD32
{
    HEAP_ENTRY32 Entry;
    ULONG Signature;
    ULONG Flags;
    WOW64_POINTER(struct _HEAP *) Heap;

    // ...
} HEAP_SEGMENT_OLD32, *PHEAP_SEGMENT_OLD32;

#define HEAP_SEGMENT_MAX_SIZE \
    (max(sizeof(HEAP_SEGMENT), max(sizeof(HEAP_SEGMENT_OLD), \
        max(sizeof(HEAP_SEGMENT32), sizeof(HEAP_SEGMENT_OLD32)))))

#endif
