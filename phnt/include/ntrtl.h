/*
 * RTL support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTRTL_H
#define _NTRTL_H

#define RtlOffsetToPointer(Base, Offset) ((PUCHAR)(((PUCHAR)(Base)) + ((ULONG_PTR)(Offset))))
#define RtlPointerToOffset(Base, Pointer) ((ULONG)(((PUCHAR)(Pointer)) - ((PUCHAR)(Base))))

#define RTL_PTR_ADD(Pointer, Value) ((PVOID)((ULONG_PTR)(Pointer) + (ULONG_PTR)(Value)))
#define RTL_PTR_SUBTRACT(Pointer, Value) ((PVOID)((ULONG_PTR)(Pointer) - (ULONG_PTR)(Value)))

#define RTL_NANOSEC_PER_SEC              1000000000ull
#define RTL_NANOSEC_PER_MILLISEC            1000000ull
#define RTL_100NANOSEC_PER_SEC             10000000ull
#define RTL_100NANOSEC_PER_MILLISEC           10000ull
#define RTL_MILLISEC_PER_SEC                   1000ull

#define RTL_MILLISEC_TO_100NANOSEC(m) ((m) * RTL_100NANOSEC_PER_MILLISEC)
#define RTL_SEC_TO_100NANOSEC(s) ((s) * RTL_100NANOSEC_PER_SEC)
#define RTL_SEC_TO_MILLISEC(s) ((s) * RTL_MILLISEC_PER_SEC)

#define RTL_MEG (1024UL * 1024UL)
#define RTL_IMAGE_MAX_DOS_HEADER (256UL * RTL_MEG)

#define RTL_NANOSEC_PER_TICK        100
#define RTL_TICKS_PER_MICROSEC      10
#define RTL_TICKS_PER_MILLISEC      (RTL_TICKS_PER_MICROSEC * 1000)  // 10,000
#define RTL_TICKS_PER_SEC           (RTL_TICKS_PER_MILLISEC * 1000)  // 10,000,000
#define RTL_TICKS_PER_MIN           (RTL_TICKS_PER_SEC * 60)         // 600,000,000
#define RTL_TICKS_PER_HOUR          (RTL_TICKS_PER_MIN * 60)         // 36,000,000,000
#define RTL_TICKS_PER_DAY           (RTL_TICKS_PER_HOUR * 24)        // 864,000,000,000

//
// Errors
//

/**
 * The RtlFailFast routine brings down the caller immediately in the event that critical corruption has been detected. No exception handlers are invoked.
 * The routine may be used in libraries shared with user mode and kernel mode. In user mode, the process is terminated, whereas in kernel mode, a KERNEL_SECURITY_CHECK_FAILURE bug check is raised.
 *
 * @param Code A FAST_FAIL_<description> symbolic constant from winnt.h or wdm.h that indicates the reason for process termination.
 * @return None. There is no return from this routine.
 */
DECLSPEC_NORETURN
FORCEINLINE
VOID
NTAPI_INLINE
RtlFailFast(
    _In_ ULONG Code
    )
{
    __fastfail(Code);
}

DECLSPEC_NORETURN
FORCEINLINE
VOID
NTAPI_INLINE
RtlFatalListEntryError(
    _In_ PVOID p1,
    _In_ PVOID p2,
    _In_ PVOID p3
    )
{
    //++
    //    This routine reports a fatal list entry error.  It is implemented here as a
    //    wrapper around RtlFailFast so that alternative reporting mechanisms (such
    //    as simply logging and trying to continue) can be easily switched in.
    //--
    
    UNREFERENCED_PARAMETER(p1);
    UNREFERENCED_PARAMETER(p2);
    UNREFERENCED_PARAMETER(p3);

    RtlFailFast(FAST_FAIL_CORRUPT_LIST_ENTRY);
}

//
// Linked lists
//

typedef struct _LIST_ENTRY LIST_ENTRY, *PLIST_ENTRY;

#define RTL_STATIC_LIST_HEAD(x) \
    LIST_ENTRY (x) = { &(x), &(x) }

#define RTL_LIST_FOREACH(Entry, ListHead) \
    for ((Entry) = &(ListHead); (Entry) != &(ListHead); (Entry) = (Entry)->Flink)

// #ifndef NO_LIST_ENTRY_CHECKS
// #define NO_LIST_ENTRY_CHECKS
// #endif

FORCEINLINE
VOID
NTAPI_INLINE
RtlCheckListEntry(
    _In_ PLIST_ENTRY Entry
    )
{
    if ((((Entry->Flink)->Blink) != Entry) || (((Entry->Blink)->Flink) != Entry)) 
    {
        RtlFatalListEntryError(
            (PVOID)(Entry), 
            (PVOID)((Entry->Flink)->Blink), 
            (PVOID)((Entry->Blink)->Flink)
            );
    }
}

FORCEINLINE
VOID
NTAPI_INLINE
InitializeListHead(
    _Out_ PLIST_ENTRY ListHead
    )
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

FORCEINLINE
VOID
NTAPI_INLINE
InitializeListHead32(
    _Out_ PLIST_ENTRY32 ListHead
    )
{
    ListHead->Flink = ListHead->Blink = PtrToUlong(ListHead);
}

_Must_inspect_result_
FORCEINLINE
BOOLEAN
NTAPI_INLINE
IsListEmpty(
    _In_ PLIST_ENTRY ListHead
    )
{
    return ListHead->Flink == ListHead;
}

FORCEINLINE
BOOLEAN
NTAPI_INLINE
RemoveEntryListUnsafe(
    _In_ PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Flink;

    Flink = Entry->Flink;
    Blink = Entry->Blink;
    Blink->Flink = Flink;
    Flink->Blink = Blink;
    return (BOOLEAN)(Flink == Blink);
}

FORCEINLINE
BOOLEAN
NTAPI_INLINE
RemoveEntryList(
    _In_ PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY PrevEntry;
    PLIST_ENTRY NextEntry;

    NextEntry = Entry->Flink;
    PrevEntry = Entry->Blink;

#if !defined(NO_LIST_ENTRY_CHECKS)
    if ((NextEntry->Blink != Entry) || (PrevEntry->Flink != Entry))
    {
        RtlFatalListEntryError((PVOID)PrevEntry, (PVOID)Entry, (PVOID)NextEntry);
    }
#endif

    PrevEntry->Flink = NextEntry;
    NextEntry->Blink = PrevEntry;

    return NextEntry == PrevEntry;
}

FORCEINLINE
PLIST_ENTRY
NTAPI_INLINE
RemoveHeadList(
    _Inout_ PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Entry;
    PLIST_ENTRY NextEntry;

    Entry = ListHead->Flink;
    NextEntry = Entry->Flink;

#if !defined(NO_LIST_ENTRY_CHECKS)
    if ((Entry->Blink != ListHead) || (NextEntry->Blink != Entry)) 
    {
        RtlFatalListEntryError((PVOID)ListHead, (PVOID)Entry, (PVOID)NextEntry);
    }
#endif

    ListHead->Flink = NextEntry;
    NextEntry->Blink = ListHead;

    return Entry;
}

FORCEINLINE
PLIST_ENTRY
NTAPI_INLINE
RemoveTailList(
    _Inout_ PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Entry;
    PLIST_ENTRY PrevEntry;

    Entry = ListHead->Blink;
    PrevEntry = Entry->Blink;

    if ((Entry->Flink != ListHead) || (PrevEntry->Flink != Entry)) 
    {
        RtlFatalListEntryError((PVOID)PrevEntry, (PVOID)Entry, (PVOID)ListHead);
    }

    ListHead->Blink = PrevEntry;
    PrevEntry->Flink = ListHead;

    return Entry;
}

FORCEINLINE
VOID
NTAPI_INLINE
InsertTailList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ __drv_aliasesMem PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY PrevEntry;

    PrevEntry = ListHead->Blink;

    if (PrevEntry->Flink != ListHead) 
    {
        RtlFatalListEntryError((PVOID)PrevEntry, (PVOID)ListHead, (PVOID)PrevEntry->Flink);
    }

    Entry->Flink = ListHead;
    Entry->Blink = PrevEntry;
    PrevEntry->Flink = Entry;
    ListHead->Blink = Entry;
}

FORCEINLINE
VOID
NTAPI_INLINE
InsertHeadList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ __drv_aliasesMem PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY NextEntry;

    NextEntry = ListHead->Flink;

    RtlCheckListEntry(ListHead);

    if (NextEntry->Blink != ListHead) 
    {
        RtlFatalListEntryError((PVOID)ListHead, (PVOID)NextEntry, (PVOID)NextEntry->Blink);
    }

    Entry->Flink = NextEntry;
    Entry->Blink = ListHead;
    NextEntry->Blink = Entry;
    ListHead->Flink = Entry;
}

FORCEINLINE
VOID
NTAPI_INLINE
AppendTailList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ PLIST_ENTRY ListToAppend
    )
{
    PLIST_ENTRY ListEnd = ListHead->Blink;

    RtlCheckListEntry(ListHead);
    RtlCheckListEntry(ListToAppend);

    ListHead->Blink->Flink = ListToAppend;
    ListHead->Blink = ListToAppend->Blink;
    ListToAppend->Blink->Flink = ListHead;
    ListToAppend->Blink = ListEnd;
}

FORCEINLINE
PSINGLE_LIST_ENTRY
NTAPI_INLINE
PopEntryList(
    _Inout_ PSINGLE_LIST_ENTRY ListHead
    )
{
    PSINGLE_LIST_ENTRY FirstEntry;

    FirstEntry = ListHead->Next;

    if (FirstEntry)
        ListHead->Next = FirstEntry->Next;

    return FirstEntry;
}

FORCEINLINE
VOID
NTAPI_INLINE
PushEntryList(
    _Inout_ PSINGLE_LIST_ENTRY ListHead,
    _Inout_ __drv_aliasesMem PSINGLE_LIST_ENTRY Entry
    )
{
    Entry->Next = ListHead->Next;
    ListHead->Next = Entry;
}

//
// AVL and splay trees
//

typedef enum _TABLE_SEARCH_RESULT
{
    TableEmptyTree,
    TableFoundNode,
    TableInsertAsLeft,
    TableInsertAsRight
} TABLE_SEARCH_RESULT;

typedef enum _RTL_GENERIC_COMPARE_RESULTS
{
    GenericLessThan,
    GenericGreaterThan,
    GenericEqual
} RTL_GENERIC_COMPARE_RESULTS;

typedef struct _RTL_AVL_TABLE RTL_AVL_TABLE, *PRTL_AVL_TABLE;

typedef _Function_class_(RTL_AVL_COMPARE_ROUTINE)
RTL_GENERIC_COMPARE_RESULTS NTAPI RTL_AVL_COMPARE_ROUTINE(
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID FirstStruct,
    _In_ PVOID SecondStruct
    );
typedef RTL_AVL_COMPARE_ROUTINE* PRTL_AVL_COMPARE_ROUTINE;

typedef _Function_class_(RTL_AVL_ALLOCATE_ROUTINE)
PVOID NTAPI RTL_AVL_ALLOCATE_ROUTINE(
    _In_ PRTL_AVL_TABLE Table,
    _In_ CLONG ByteSize
    );
typedef RTL_AVL_ALLOCATE_ROUTINE* PRTL_AVL_ALLOCATE_ROUTINE;

typedef _Function_class_(RTL_AVL_FREE_ROUTINE)
VOID NTAPI RTL_AVL_FREE_ROUTINE(
    _In_ PRTL_AVL_TABLE Table,
    _In_ _Post_invalid_ PVOID Buffer
    );
typedef RTL_AVL_FREE_ROUTINE* PRTL_AVL_FREE_ROUTINE;

typedef _Function_class_(RTL_AVL_MATCH_FUNCTION)
NTSTATUS NTAPI RTL_AVL_MATCH_FUNCTION(
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID UserData,
    _In_ PVOID MatchData
    );
typedef RTL_AVL_MATCH_FUNCTION* PRTL_AVL_MATCH_FUNCTION;

typedef struct _RTL_BALANCED_LINKS
{
    struct _RTL_BALANCED_LINKS *Parent;
    struct _RTL_BALANCED_LINKS *LeftChild;
    struct _RTL_BALANCED_LINKS *RightChild;
    CHAR Balance;
    UCHAR Reserved[3];
} RTL_BALANCED_LINKS, *PRTL_BALANCED_LINKS;

typedef struct _RTL_AVL_TABLE
{
    RTL_BALANCED_LINKS BalancedRoot;
    PVOID OrderedPointer;
    ULONG WhichOrderedElement;
    ULONG NumberGenericTableElements;
    ULONG DepthOfTree;
    PRTL_BALANCED_LINKS RestartKey;
    ULONG DeleteCount;
    PRTL_AVL_COMPARE_ROUTINE CompareRoutine;
    PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine;
    PRTL_AVL_FREE_ROUTINE FreeRoutine;
    PVOID TableContext;
} RTL_AVL_TABLE, *PRTL_AVL_TABLE;

NTSYSAPI
VOID
NTAPI
RtlInitializeGenericTableAvl(
    _Out_ PRTL_AVL_TABLE Table,
    _In_ PRTL_AVL_COMPARE_ROUTINE CompareRoutine,
    _In_ PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine,
    _In_ PRTL_AVL_FREE_ROUTINE FreeRoutine,
    _In_opt_ PVOID TableContext
    );

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ CLONG BufferSize,
    _Out_opt_ PBOOLEAN NewElement
    );

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableFullAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ CLONG BufferSize,
    _Out_opt_ PBOOLEAN NewElement,
    _In_ PVOID NodeOrParent,
    _In_ TABLE_SEARCH_RESULT SearchResult
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlDeleteElementGenericTableAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID Buffer
    );

_Check_return_
NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID Buffer
    );

NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableFullAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID Buffer,
    _Out_ PVOID *NodeOrParent,
    _Out_ TABLE_SEARCH_RESULT *SearchResult
    );

_Check_return_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_ BOOLEAN Restart
    );

_Check_return_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplayingAvl(
    _In_ PRTL_AVL_TABLE Table,
    _Inout_ PVOID *RestartKey
    );

_Check_return_
NTSYSAPI
PVOID
NTAPI
RtlLookupFirstMatchingElementGenericTableAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID Buffer,
    _Out_ PVOID *RestartKey
    );

_Check_return_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableLikeADirectory(
    _In_ PRTL_AVL_TABLE Table,
    _In_opt_ PRTL_AVL_MATCH_FUNCTION MatchFunction,
    _In_opt_ PVOID MatchData,
    _In_ ULONG NextFlag,
    _Inout_ PVOID *RestartKey,
    _Inout_ PULONG DeleteCount,
    _In_ PVOID Buffer
    );

_Check_return_
NTSYSAPI
PVOID
NTAPI
RtlGetElementGenericTableAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_ ULONG I
    );

NTSYSAPI
ULONG
NTAPI
RtlNumberGenericTableElementsAvl(
    _In_ PRTL_AVL_TABLE Table
    );

_Check_return_
NTSYSAPI
BOOLEAN
NTAPI
RtlIsGenericTableEmptyAvl(
    _In_ PRTL_AVL_TABLE Table
    );

typedef struct _RTL_SPLAY_LINKS
{
    struct _RTL_SPLAY_LINKS *Parent;
    struct _RTL_SPLAY_LINKS *LeftChild;
    struct _RTL_SPLAY_LINKS *RightChild;
} RTL_SPLAY_LINKS, *PRTL_SPLAY_LINKS;

#define RtlInitializeSplayLinks(Links) \
{ \
    PRTL_SPLAY_LINKS _SplayLinks; \
    _SplayLinks = (PRTL_SPLAY_LINKS)(Links); \
    _SplayLinks->Parent = _SplayLinks; \
    _SplayLinks->LeftChild = NULL; \
    _SplayLinks->RightChild = NULL; \
}

#define RtlParent(Links) ((PRTL_SPLAY_LINKS)(Links)->Parent)
#define RtlLeftChild(Links) ((PRTL_SPLAY_LINKS)(Links)->LeftChild)
#define RtlRightChild(Links) ((PRTL_SPLAY_LINKS)(Links)->RightChild)
#define RtlIsRoot(Links) ((RtlParent(Links) == (PRTL_SPLAY_LINKS)(Links)))
#define RtlIsLeftChild(Links) ((RtlLeftChild(RtlParent(Links)) == (PRTL_SPLAY_LINKS)(Links)))
#define RtlIsRightChild(Links) ((RtlRightChild(RtlParent(Links)) == (PRTL_SPLAY_LINKS)(Links)))

#define RtlInsertAsLeftChild(ParentLinks, ChildLinks) \
{ \
    PRTL_SPLAY_LINKS _SplayParent; \
    PRTL_SPLAY_LINKS _SplayChild; \
    _SplayParent = (PRTL_SPLAY_LINKS)(ParentLinks); \
    _SplayChild = (PRTL_SPLAY_LINKS)(ChildLinks); \
    _SplayParent->LeftChild = _SplayChild; \
    _SplayChild->Parent = _SplayParent; \
}

#define RtlInsertAsRightChild(ParentLinks, ChildLinks) \
{ \
    PRTL_SPLAY_LINKS _SplayParent; \
    PRTL_SPLAY_LINKS _SplayChild; \
    _SplayParent = (PRTL_SPLAY_LINKS)(ParentLinks); \
    _SplayChild = (PRTL_SPLAY_LINKS)(ChildLinks); \
    _SplayParent->RightChild = _SplayChild; \
    _SplayChild->Parent = _SplayParent; \
}

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSplay(
    _Inout_ PRTL_SPLAY_LINKS Links
    );

NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlDelete(
    _In_ PRTL_SPLAY_LINKS Links
    );

NTSYSAPI
VOID
NTAPI
RtlDeleteNoSplay(
    _In_ PRTL_SPLAY_LINKS Links,
    _Inout_ PRTL_SPLAY_LINKS *Root
    );

_Check_return_
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSubtreeSuccessor(
    _In_ PRTL_SPLAY_LINKS Links
    );

_Check_return_
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSubtreePredecessor(
    _In_ PRTL_SPLAY_LINKS Links
    );

_Check_return_
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlRealSuccessor(
    _In_ PRTL_SPLAY_LINKS Links
    );

_Check_return_
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlRealPredecessor(
    _In_ PRTL_SPLAY_LINKS Links
    );

struct _RTL_GENERIC_TABLE;

typedef RTL_GENERIC_COMPARE_RESULTS (NTAPI *PRTL_GENERIC_COMPARE_ROUTINE)(
    _In_ struct _RTL_GENERIC_TABLE *Table,
    _In_ PVOID FirstStruct,
    _In_ PVOID SecondStruct
    );

typedef PVOID (NTAPI *PRTL_GENERIC_ALLOCATE_ROUTINE)(
    _In_ struct _RTL_GENERIC_TABLE *Table,
    _In_ CLONG ByteSize
    );

typedef VOID (NTAPI *PRTL_GENERIC_FREE_ROUTINE)(
    _In_ struct _RTL_GENERIC_TABLE *Table,
    _In_ _Post_invalid_ PVOID Buffer
    );

typedef struct _RTL_GENERIC_TABLE
{
    PRTL_SPLAY_LINKS TableRoot;
    LIST_ENTRY InsertOrderList;
    PLIST_ENTRY OrderedPointer;
    ULONG WhichOrderedElement;
    ULONG NumberGenericTableElements;
    PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine;
    PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine;
    PRTL_GENERIC_FREE_ROUTINE FreeRoutine;
    PVOID TableContext;
} RTL_GENERIC_TABLE, *PRTL_GENERIC_TABLE;

NTSYSAPI
VOID
NTAPI
RtlInitializeGenericTable(
    _Out_ PRTL_GENERIC_TABLE Table,
    _In_ PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine,
    _In_ PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine,
    _In_ PRTL_GENERIC_FREE_ROUTINE FreeRoutine,
    _In_opt_ PVOID TableContext
    );

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTable(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ CLONG BufferSize,
    _Out_opt_ PBOOLEAN NewElement
    );

NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableFull(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ CLONG BufferSize,
    _Out_opt_ PBOOLEAN NewElement,
    _In_ PVOID NodeOrParent,
    _In_ TABLE_SEARCH_RESULT SearchResult
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlDeleteElementGenericTable(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ PVOID Buffer
    );

_Check_return_
NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTable(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ PVOID Buffer
    );

NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableFull(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ PVOID Buffer,
    _Out_ PVOID *NodeOrParent,
    _Out_ TABLE_SEARCH_RESULT *SearchResult
    );

_Check_return_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTable(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ BOOLEAN Restart
    );

_Check_return_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplaying(
    _In_ PRTL_GENERIC_TABLE Table,
    _Inout_ PVOID *RestartKey
    );

_Check_return_
NTSYSAPI
PVOID
NTAPI
RtlGetElementGenericTable(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ ULONG I
    );

NTSYSAPI
ULONG
NTAPI
RtlNumberGenericTableElements(
    _In_ PRTL_GENERIC_TABLE Table
    );

_Check_return_
NTSYSAPI
BOOLEAN
NTAPI
RtlIsGenericTableEmpty(
    _In_ PRTL_GENERIC_TABLE Table
    );

//
// RB trees
//

typedef struct _RTL_RB_TREE
{
    PRTL_BALANCED_NODE Root;
    PRTL_BALANCED_NODE Min;
} RTL_RB_TREE, *PRTL_RB_TREE;

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// rev
NTSYSAPI
BOOLEAN
NTAPI
RtlRbInsertNodeEx(
    _In_ PRTL_RB_TREE Tree,
    _In_opt_ PRTL_BALANCED_NODE Parent,
    _In_ BOOLEAN Right,
    _Out_ PRTL_BALANCED_NODE Node
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
RtlRbRemoveNode(
    _In_ PRTL_RB_TREE Tree,
    _In_ PRTL_BALANCED_NODE Node
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlCompareExchangePointerMapping(
    _In_ PRTL_BALANCED_NODE Node1,
    _In_ PRTL_BALANCED_NODE Node2,
    _Out_ PRTL_BALANCED_NODE *Node3,
    _Out_ PRTL_BALANCED_NODE *Node4
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryPointerMapping(
    _In_ PRTL_RB_TREE Tree,
    _Inout_ PRTL_BALANCED_NODE Children
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlRemovePointerMapping(
    _In_ PRTL_RB_TREE Tree,
    _Inout_ PRTL_BALANCED_NODE Children
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11

//
// Hash tables
//

// begin_ntddk

#define RTL_HASH_ALLOCATED_HEADER 0x00000001
#define RTL_HASH_RESERVED_SIGNATURE 0

typedef struct _RTL_DYNAMIC_HASH_TABLE_ENTRY
{
    LIST_ENTRY Linkage;
    ULONG_PTR Signature;
} RTL_DYNAMIC_HASH_TABLE_ENTRY, *PRTL_DYNAMIC_HASH_TABLE_ENTRY;

#define HASH_ENTRY_KEY(x) ((x)->Signature)

typedef struct _RTL_DYNAMIC_HASH_TABLE_CONTEXT
{
    PLIST_ENTRY ChainHead;
    PLIST_ENTRY PrevLinkage;
    ULONG_PTR Signature;
} RTL_DYNAMIC_HASH_TABLE_CONTEXT, *PRTL_DYNAMIC_HASH_TABLE_CONTEXT;

typedef struct _RTL_DYNAMIC_HASH_TABLE_ENUMERATOR
{
    RTL_DYNAMIC_HASH_TABLE_ENTRY HashEntry;
    PLIST_ENTRY ChainHead;
    ULONG BucketIndex;
} RTL_DYNAMIC_HASH_TABLE_ENUMERATOR, *PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR;

typedef struct _RTL_DYNAMIC_HASH_TABLE
{
    // Entries initialized at creation.
    ULONG Flags;
    ULONG Shift;

    // Entries used in bucket computation.
    ULONG TableSize;
    ULONG Pivot;
    ULONG DivisorMask;

    // Counters.
    ULONG NumEntries;
    ULONG NonEmptyBuckets;
    ULONG NumEnumerators;

    // The directory. This field is for internal use only.
    PVOID Directory;
} RTL_DYNAMIC_HASH_TABLE, *PRTL_DYNAMIC_HASH_TABLE;

FORCEINLINE
VOID
NTAPI_INLINE
RtlInitHashTableContext(
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context
    )
{
    Context->ChainHead = NULL;
    Context->PrevLinkage = NULL;
}

FORCEINLINE
VOID
NTAPI_INLINE
RtlInitHashTableContextFromEnumerator(
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context,
    _In_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    )
{
    Context->ChainHead = Enumerator->ChainHead;
    Context->PrevLinkage = Enumerator->HashEntry.Linkage.Blink;
}

FORCEINLINE
VOID
NTAPI_INLINE
RtlReleaseHashTableContext(
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context
    )
{
    UNREFERENCED_PARAMETER(Context);
    return;
}

FORCEINLINE
ULONG
NTAPI_INLINE
RtlTotalBucketsHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    )
{
    return HashTable->TableSize;
}

FORCEINLINE
ULONG
NTAPI_INLINE
RtlNonEmptyBucketsHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    )
{
    return HashTable->NonEmptyBuckets;
}

FORCEINLINE
ULONG
NTAPI_INLINE
RtlEmptyBucketsHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    )
{
    return HashTable->TableSize - HashTable->NonEmptyBuckets;
}

FORCEINLINE
ULONG
NTAPI_INLINE
RtlTotalEntriesHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    )
{
    return HashTable->NumEntries;
}

FORCEINLINE
ULONG
NTAPI_INLINE
RtlActiveEnumeratorsHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    )
{
    return HashTable->NumEnumerators;
}

_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlCreateHashTable(
    _Inout_ _When_(*HashTable == NULL, __drv_allocatesMem(Mem)) PRTL_DYNAMIC_HASH_TABLE *HashTable,
    _In_ ULONG Shift,
    _In_ _Reserved_ ULONG Flags
    );

_Must_inspect_result_
_Success_(return != 0)
NTSYSAPI
BOOLEAN
NTAPI
RtlCreateHashTableEx(
    _Inout_ _When_(NULL == *HashTable, _At_(*HashTable, __drv_allocatesMem(Mem))) PRTL_DYNAMIC_HASH_TABLE *HashTable,
    _In_ ULONG InitialSize,
    _In_ ULONG Shift,
    _Reserved_ ULONG Flags
    );

NTSYSAPI
LOGICAL
NTAPI
RtlDeleteHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlInsertEntryHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _In_ PRTL_DYNAMIC_HASH_TABLE_ENTRY Entry,
    _In_ ULONG_PTR Signature,
    _Inout_opt_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlRemoveEntryHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _In_ PRTL_DYNAMIC_HASH_TABLE_ENTRY Entry,
    _Inout_opt_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context
    );

_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlLookupEntryHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _In_ ULONG_PTR Signature,
    _Out_opt_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context
    );

_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlGetNextEntryHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _In_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlInitEnumerationHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Out_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );

_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlEnumerateEntryHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );

NTSYSAPI
VOID
NTAPI
RtlEndEnumerationHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlInitWeakEnumerationHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Out_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );

_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlWeaklyEnumerateEntryHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );

NTSYSAPI
VOID
NTAPI
RtlEndWeakEnumerationHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlExpandHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlContractHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10)

NTSYSAPI
BOOLEAN
NTAPI
RtlInitStrongEnumerationHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Out_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );

_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlStronglyEnumerateEntryHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );

NTSYSAPI
VOID
NTAPI
RtlEndStrongEnumerationHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_10

// end_ntddk

//
// Critical sections
//

// These flags define the upper byte of the critical section SpinCount field
#define RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO         0x01000000
#define RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN          0x02000000
#define RTL_CRITICAL_SECTION_FLAG_STATIC_INIT           0x04000000
#define RTL_CRITICAL_SECTION_FLAG_RESOURCE_TYPE         0x08000000
#define RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO      0x10000000
#define RTL_CRITICAL_SECTION_ALL_FLAG_BITS              0xFF000000
#define RTL_CRITICAL_SECTION_FLAG_RESERVED              (RTL_CRITICAL_SECTION_ALL_FLAG_BITS & (~(RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO | RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN | RTL_CRITICAL_SECTION_FLAG_STATIC_INIT | RTL_CRITICAL_SECTION_FLAG_RESOURCE_TYPE | RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO)))
// These flags define possible values stored in the Flags field of a critsec debuginfo.
#define RTL_CRITICAL_SECTION_DEBUG_FLAG_STATIC_INIT 0x00000001

// typedef struct _RTL_CRITICAL_SECTION_DEBUG
// {
//     USHORT Type;
//     USHORT CreatorBackTraceIndex;
//     struct _RTL_CRITICAL_SECTION *CriticalSection;
//     LIST_ENTRY ProcessLocksList;
//     ULONG EntryCount;
//     ULONG ContentionCount;
//     ULONG Flags;
//     USHORT CreatorBackTraceIndexHigh;
//     USHORT Identifier;
// } RTL_CRITICAL_SECTION_DEBUG, *PRTL_CRITICAL_SECTION_DEBUG, RTL_RESOURCE_DEBUG, *PRTL_RESOURCE_DEBUG;
//
// #pragma pack(push, 8)
// typedef struct _RTL_CRITICAL_SECTION
// {
//     PRTL_CRITICAL_SECTION_DEBUG DebugInfo;
//     LONG LockCount;
//     LONG RecursionCount;
//     HANDLE OwningThread;
//     HANDLE LockSemaphore;
//     SIZE_T SpinCount;
// } RTL_CRITICAL_SECTION, *PRTL_CRITICAL_SECTION;
// #pragma pack(pop)

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeCriticalSection(
    _Out_ PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeCriticalSectionAndSpinCount(
    _Out_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_ ULONG SpinCount
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeCriticalSectionEx(
    _Out_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_ ULONG SpinCount,
    _In_ ULONG Flags
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );

_Acquires_exclusive_lock_(*CriticalSection)
NTSYSAPI
NTSTATUS
NTAPI
RtlEnterCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );

_Releases_exclusive_lock_(*CriticalSection)
NTSYSAPI
NTSTATUS
NTAPI
RtlLeaveCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );

_When_(return != 0, _Acquires_exclusive_lock_(*CriticalSection))
NTSYSAPI
LOGICAL
NTAPI
RtlTryEnterCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
LOGICAL
NTAPI
RtlIsCriticalSectionLocked(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
LOGICAL
NTAPI
RtlIsCriticalSectionLockedByThread(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
ULONG
NTAPI
RtlGetCriticalSectionRecursionCount(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
ULONG
NTAPI
RtlSetCriticalSectionSpinCount(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_ ULONG SpinCount
    );

NTSYSAPI
HANDLE
NTAPI
RtlQueryCriticalSectionOwner(
    _In_ HANDLE EventHandle,
    _In_ BOOLEAN ExactMatchOwnerAddress
    );

NTSYSAPI
VOID
NTAPI
RtlCheckForOrphanedCriticalSections(
    _In_ HANDLE ThreadHandle
    );

/**
 * Enables the creation of early critical section events.
 *
 * This function allows the system to create critical section events early in the process
 * initialization. It is typically used to ensure that critical sections are properly
 * initialized and can be used safely during the early stages of process startup.
 * @remarks This function sets the FLG_CRITSEC_EVENT_CREATION flag in the PEB flags field.
 * @return A pointer to the Process Environment Block (PEB).
 */
NTSYSAPI
PPEB
NTAPI
RtlEnableEarlyCriticalSectionEventCreation(
    VOID
    );

//
// Resources
//

typedef struct _RTL_RESOURCE
{
    RTL_CRITICAL_SECTION CriticalSection;

    HANDLE SharedSemaphore;
    volatile ULONG NumberOfWaitingShared;
    HANDLE ExclusiveSemaphore;
    volatile ULONG NumberOfWaitingExclusive;

    volatile LONG NumberOfActive; // negative: exclusive acquire; zero: not acquired; positive: shared acquire(s)
    HANDLE ExclusiveOwnerThread;

    ULONG Flags; // RTL_RESOURCE_FLAG_*

    PRTL_RESOURCE_DEBUG DebugInfo;
} RTL_RESOURCE, *PRTL_RESOURCE;

#define RTL_RESOURCE_FLAG_LONG_TERM ((ULONG)0x00000001)

NTSYSAPI
VOID
NTAPI
RtlInitializeResource(
    _Out_ PRTL_RESOURCE Resource
    );

NTSYSAPI
VOID
NTAPI
RtlDeleteResource(
    _Inout_ PRTL_RESOURCE Resource
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlAcquireResourceShared(
    _Inout_ PRTL_RESOURCE Resource,
    _In_ BOOLEAN Wait
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlAcquireResourceExclusive(
    _Inout_ PRTL_RESOURCE Resource,
    _In_ BOOLEAN Wait
    );

NTSYSAPI
VOID
NTAPI
RtlReleaseResource(
    _Inout_ PRTL_RESOURCE Resource
    );

NTSYSAPI
VOID
NTAPI
RtlConvertSharedToExclusive(
    _Inout_ PRTL_RESOURCE Resource
    );

NTSYSAPI
VOID
NTAPI
RtlConvertExclusiveToShared(
    _Inout_ PRTL_RESOURCE Resource
    );

NTSYSAPI
ULONG
NTAPI
RtlDumpResource(
    _Inout_ PRTL_RESOURCE Resource
    );

//
// Slim reader-writer locks, condition variables, and barriers
//

#ifndef RTL_SRWLOCK_INIT
#define RTL_SRWLOCK_INIT {0}
#endif

// winbase:InitializeSRWLock
/**
 * The RtlInitializeSRWLock routine initializes a slim reader/writer (SRW) lock.
 * 
 * \param SRWLock A pointer to the SRW lock.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializesrwlock
 */
NTSYSAPI
VOID
NTAPI
RtlInitializeSRWLock(
    _Out_ PRTL_SRWLOCK SRWLock
    );

// winbase:AcquireSRWLockExclusive
_Acquires_exclusive_lock_(*SRWLock)
NTSYSAPI
VOID
NTAPI
RtlAcquireSRWLockExclusive(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

// winbase:AcquireSRWLockShared
_Acquires_shared_lock_(*SRWLock)
NTSYSAPI
VOID
NTAPI
RtlAcquireSRWLockShared(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

// winbase:ReleaseSRWLockExclusive
_Releases_exclusive_lock_(*SRWLock)
NTSYSAPI
VOID
NTAPI
RtlReleaseSRWLockExclusive(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

// winbase:ReleaseSRWLockShared
_Releases_shared_lock_(*SRWLock)
NTSYSAPI
VOID
NTAPI
RtlReleaseSRWLockShared(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

// winbase:TryAcquireSRWLockExclusive
_When_(return != 0, _Acquires_exclusive_lock_(*SRWLock))
NTSYSAPI
BOOLEAN
NTAPI
RtlTryAcquireSRWLockExclusive(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

// winbase:TryAcquireSRWLockShared
_When_(return != 0, _Acquires_shared_lock_(*SRWLock))
NTSYSAPI
BOOLEAN
NTAPI
RtlTryAcquireSRWLockShared(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

// rev
NTSYSAPI
VOID
NTAPI
RtlAcquireReleaseSRWLockExclusive(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
// rev
NTSYSAPI
BOOLEAN
NTAPI
RtlConvertSRWLockExclusiveToShared(
    _Inout_ PRTL_SRWLOCK SRWLock
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10

#if (PHNT_VERSION >= PHNT_WINDOWS_11)

//
// Read-Copy-Update (RCU).
//
// RCU synchronization allows concurrent access to shared data structures,
// such as linked lists, trees, or hash tables, without using traditional locking methods
// in scenarios where read operations are frequent and need to be fast.
// It is particularly useful in multi-threaded environments where multiple threads
// may read from the same data structure while one or more threads may modify it.
// @remarks RCU synchronization is not for general-purpose synchronization.
// Teb->Rcu is used to store the RCU state.

NTSYSAPI
PVOID
NTAPI
RtlRcuAllocate(
    _In_ SIZE_T Size
    );

NTSYSAPI
LOGICAL
NTAPI
RtlRcuFree(
    _In_ PULONG Rcu
    );

NTSYSAPI
VOID
NTAPI
RtlRcuReadLock(
    _Inout_ PRTL_SRWLOCK SRWLock,
    _Out_ PULONG Rcu
    );

NTSYSAPI
VOID
NTAPI
RtlRcuReadUnlock(
    _Inout_ PRTL_SRWLOCK SRWLock,
    _Inout_ PULONG* Rcu
    );

NTSYSAPI
LONG
NTAPI
RtlRcuSynchronize(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_11

#define RTL_CONDITION_VARIABLE_INIT {0}
#define RTL_CONDITION_VARIABLE_LOCKMODE_SHARED 0x1

// winbase:InitializeConditionVariable
NTSYSAPI
VOID
NTAPI
RtlInitializeConditionVariable(
    _Out_ PRTL_CONDITION_VARIABLE ConditionVariable
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlSleepConditionVariableCS(
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable,
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_opt_ PLARGE_INTEGER Timeout
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlSleepConditionVariableSRW(
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable,
    _Inout_ PRTL_SRWLOCK SRWLock,
    _In_opt_ PLARGE_INTEGER Timeout,
    _In_ ULONG Flags
    );

// winbase:WakeConditionVariable
NTSYSAPI
VOID
NTAPI
RtlWakeConditionVariable(
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable
    );

// winbase:WakeAllConditionVariable
NTSYSAPI
VOID
NTAPI
RtlWakeAllConditionVariable(
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable
    );

// begin_rev
#define RTL_BARRIER_FLAGS_SPIN_ONLY 0x00000001 // never block on event - always spin
#define RTL_BARRIER_FLAGS_BLOCK_ONLY 0x00000002 // always block on event - never spin
#define RTL_BARRIER_FLAGS_NO_DELETE 0x00000004 // use if barrier will never be deleted
// end_rev

// begin_private

NTSYSAPI
NTSTATUS
NTAPI
RtlInitBarrier(
    _Out_ PRTL_BARRIER Barrier,
    _In_ ULONG TotalThreads,
    _In_ ULONG SpinCount
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteBarrier(
    _In_ PRTL_BARRIER Barrier
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlBarrier(
    _Inout_ PRTL_BARRIER Barrier,
    _In_ ULONG Flags
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlBarrierForDelete(
    _Inout_ PRTL_BARRIER Barrier,
    _In_ ULONG Flags
    );

// end_private

//
// Wait on address
//

// begin_rev

#if (PHNT_VERSION >= PHNT_WINDOWS_8)

NTSYSAPI
NTSTATUS
NTAPI
RtlWaitOnAddress(
    _In_reads_bytes_(AddressSize) volatile VOID *Address,
    _In_reads_bytes_(AddressSize) PVOID CompareAddress,
    _In_ SIZE_T AddressSize,
    _In_opt_ PLARGE_INTEGER Timeout
    );

NTSYSAPI
VOID
NTAPI
RtlWakeAddressAll(
    _In_ PVOID Address
    );

NTSYSAPI
VOID
NTAPI
RtlWakeAddressAllNoFence(
    _In_ PVOID Address
    );

NTSYSAPI
VOID
NTAPI
RtlWakeAddressSingle(
    _In_ PVOID Address
    );

NTSYSAPI
VOID
NTAPI
RtlWakeAddressSingleNoFence(
    _In_ PVOID Address
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_8

// end_rev

//
// Strings
//

_At_(AnsiString->Buffer, _Post_equal_to_(Buffer))
_At_(AnsiString->Length, _Post_equal_to_(0))
_At_(AnsiString->MaximumLength, _Post_equal_to_(MaximumLength))
FORCEINLINE
VOID
NTAPI_INLINE
RtlInitEmptyAnsiString(
    _Out_ PANSI_STRING AnsiString,
    _Pre_maybenull_ _Pre_readable_size_(MaximumLength) __drv_aliasesMem PCHAR Buffer,
    _In_ USHORT MaximumLength
    )
{
    memset(AnsiString, 0, sizeof(ANSI_STRING));
    AnsiString->MaximumLength = MaximumLength;
    AnsiString->Buffer = Buffer;
}

#ifndef PHNT_NO_INLINE_INIT_STRING
FORCEINLINE
VOID
NTAPI_INLINE
RtlInitString(
    _Out_ PSTRING DestinationString,
    _In_opt_z_ PCSTR SourceString
    )
{
    if (SourceString)
        DestinationString->MaximumLength = (DestinationString->Length = (USHORT)strlen(SourceString)) + sizeof(ANSI_NULL);
    else
        DestinationString->MaximumLength = DestinationString->Length = 0;

    DestinationString->Buffer = (PCHAR)SourceString;
}
#else
NTSYSAPI
VOID
NTAPI
RtlInitString(
    _Out_ PSTRING DestinationString,
    _In_opt_z_ PCSTR SourceString
    );
#endif // PHNT_NO_INLINE_INIT_STRING

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
NTSYSAPI
NTSTATUS
NTAPI
RtlInitStringEx(
    _Out_ PSTRING DestinationString,
    _In_opt_z_ __drv_aliasesMem PCSZ SourceString
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10

#ifndef PHNT_NO_INLINE_INIT_STRING
FORCEINLINE
VOID
NTAPI_INLINE
RtlInitAnsiString(
    _Out_ PANSI_STRING DestinationString,
    _In_opt_z_ PCSTR SourceString
    )
{
    if (SourceString)
        DestinationString->MaximumLength = (DestinationString->Length = (USHORT)strlen(SourceString)) + sizeof(ANSI_NULL);
    else
        DestinationString->MaximumLength = DestinationString->Length = 0;

    DestinationString->Buffer = (PCHAR)SourceString;
}
#else
NTSYSAPI
VOID
NTAPI
RtlInitAnsiString(
    _Out_ PANSI_STRING DestinationString,
    _In_opt_z_ PCSTR SourceString
    );
#endif // PHNT_NO_INLINE_INIT_STRING

NTSYSAPI
NTSTATUS
NTAPI
RtlInitAnsiStringEx(
    _Out_ PANSI_STRING DestinationString,
    _In_opt_z_ __drv_aliasesMem PCSZ SourceString
    );

NTSYSAPI
VOID
NTAPI
RtlFreeAnsiString(
    _Inout_ _At_(AnsiString->Buffer, _Frees_ptr_opt_) PANSI_STRING AnsiString
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)
NTSYSAPI
VOID
NTAPI
RtlInitUTF8String(
    _Out_ PUTF8_STRING DestinationString,
    _In_opt_z_ PCSZ SourceString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlInitUTF8StringEx(
    _Out_ PUTF8_STRING DestinationString,
    _In_opt_z_ __drv_aliasesMem PCSZ SourceString
    );

NTSYSAPI
VOID
NTAPI
RtlFreeUTF8String(
    _Inout_ _At_(Utf8String->Buffer, _Frees_ptr_opt_) PUTF8_STRING Utf8String
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_20H1

NTSYSAPI
VOID
NTAPI
RtlFreeOemString(
    _Inout_ POEM_STRING OemString
    );

NTSYSAPI
VOID
NTAPI
RtlCopyString(
    _In_ PSTRING DestinationString,
    _In_opt_ PSTRING SourceString
    );

NTSYSAPI
CHAR
NTAPI
RtlUpperChar(
    _In_ CHAR Character
    );

_Must_inspect_result_
NTSYSAPI
LONG
NTAPI
RtlCompareString(
    _In_ PSTRING String1,
    _In_ PSTRING String2,
    _In_ BOOLEAN CaseInSensitive
    );

_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualString(
    _In_ PSTRING String1,
    _In_ PSTRING String2,
    _In_ BOOLEAN CaseInSensitive
    );

_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlPrefixString(
    _In_ PSTRING String1,
    _In_ PSTRING String2,
    _In_ BOOLEAN CaseInSensitive
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAppendStringToString(
    _Inout_ PSTRING Destination,
    _In_ PSTRING Source
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAppendAsciizToString(
    _Inout_ PSTRING Destination,
    _In_opt_z_ PCSTR Source
    );

NTSYSAPI
VOID
NTAPI
RtlUpperString(
    _Inout_ PSTRING DestinationString,
    _In_ const STRING* SourceString
    );

FORCEINLINE
BOOLEAN
NTAPI_INLINE
RtlIsNullOrEmptyUnicodeString(
    _In_opt_ PCUNICODE_STRING String
    )
{
    return !String || String->Length == 0;
}

_At_(DestinationString->Buffer, _Post_equal_to_(Buffer))
_At_(DestinationString->Length, _Post_equal_to_(0))
_At_(DestinationString->MaximumLength, _Post_equal_to_(MaximumLength))
FORCEINLINE
VOID
NTAPI_INLINE
RtlInitEmptyUnicodeString(
    _Out_ PUNICODE_STRING DestinationString,
    _Writable_bytes_(MaximumLength) _When_(MaximumLength != 0, _Notnull_) __drv_aliasesMem PWCHAR Buffer,
    _In_ USHORT MaximumLength
    )
{
    memset(DestinationString, 0, sizeof(UNICODE_STRING));
    DestinationString->MaximumLength = MaximumLength;
    DestinationString->Buffer = Buffer;
}

#ifndef PHNT_NO_INLINE_INIT_STRING
FORCEINLINE
VOID
NTAPI_INLINE
RtlInitUnicodeString(
    _Out_ PUNICODE_STRING DestinationString,
    _In_opt_z_ PCWSTR SourceString
    )
{
    if (SourceString)
        DestinationString->MaximumLength = (DestinationString->Length = (USHORT)(wcslen(SourceString) * sizeof(WCHAR))) + sizeof(UNICODE_NULL);
    else
        DestinationString->MaximumLength = DestinationString->Length = 0;

    DestinationString->Buffer = (PWCH)SourceString;
}
#else
NTSYSAPI
VOID
NTAPI
RtlInitUnicodeString(
    _Out_ PUNICODE_STRING DestinationString,
    _In_opt_z_ PCWSTR SourceString
    );
#endif // PHNT_NO_INLINE_INIT_STRING

#ifndef PHNT_NO_INLINE_INIT_STRING
FORCEINLINE
NTSTATUS
NTAPI_INLINE
RtlInitUnicodeStringEx(
    _Out_ PUNICODE_STRING DestinationString,
    _In_opt_z_ PCWSTR SourceString
    )
{
    size_t stringLength;

    DestinationString->Length = 0;
    DestinationString->Buffer = (PWCH)SourceString;

    if (!SourceString)
        return STATUS_SUCCESS;

    stringLength = wcslen(SourceString);

    if (stringLength <= UNICODE_STRING_MAX_CHARS - 1)
    {
        DestinationString->Length = (USHORT)stringLength * sizeof(WCHAR);
        DestinationString->MaximumLength = DestinationString->Length + sizeof(UNICODE_NULL);
        return STATUS_SUCCESS;
    }

    return STATUS_NAME_TOO_LONG;
}
#else
NTSYSAPI
NTSTATUS
NTAPI
RtlInitUnicodeStringEx(
    _Out_ PUNICODE_STRING DestinationString,
    _In_opt_z_ PCWSTR SourceString
    );
#endif // PHNT_NO_INLINE_INIT_STRING

/**
 * The RtlCreateUnicodeString routine creates a new counted Unicode string.
 *
 * \param DestinationString Pointer to the newly allocated and initialized Unicode string.
 * \param SourceString Pointer to a null-terminated Unicode string with which to initialize the new string.
 * \return TRUE if the Unicode string was successfully created, FALSE otherwise.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlcreateunicodestring
 */
_Success_(return != 0)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlCreateUnicodeString(
    _Out_ PUNICODE_STRING DestinationString,
    _In_z_ PCWSTR SourceString
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlCreateUnicodeStringFromAsciiz(
    _Out_ PUNICODE_STRING DestinationString,
    _In_z_ PCSTR SourceString
    );

/**
 * The RtlFreeUnicodeString routine releases storage that was allocated by RtlAnsiStringToUnicodeString or RtlUpcaseUnicodeString.
 *
 * \param UnicodeString A pointer to the string buffer.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlfreeunicodestring
 */
NTSYSAPI
VOID
NTAPI
RtlFreeUnicodeString(
    _Inout_ _At_(UnicodeString->Buffer, _Frees_ptr_opt_) PUNICODE_STRING UnicodeString
    );

#define RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE (0x00000001)
#define RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING (0x00000002)

NTSYSAPI
NTSTATUS
NTAPI
RtlDuplicateUnicodeString(
    _In_ ULONG Flags,
    _In_ PCUNICODE_STRING StringIn,
    _Out_ PUNICODE_STRING StringOut
    );

/**
 * The RtlCopyUnicodeString routine copies a source string to a destination string.
 *
 * \param[in] DestinationString A pointer to the destination string buffer.
 * \param[in] SourceString A pointer to the source string buffer.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlcopyunicodestring
 */
NTSYSAPI
VOID
NTAPI
RtlCopyUnicodeString(
    _In_ PCUNICODE_STRING DestinationString,
    _In_opt_ PCUNICODE_STRING SourceString
    );

/**
 * The RtlUpcaseUnicodeChar routine converts the specified Unicode character to uppercase.
 *
 * \param[in] SourceCharacter Specifies the character to convert.
 * \return The uppercase version of the specified Unicode character.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlupcaseunicodechar
 */
NTSYSAPI
WCHAR
NTAPI
RtlUpcaseUnicodeChar(
    _In_ WCHAR SourceCharacter
    );

/**
 * The RtlDowncaseUnicodeChar routine converts the specified Unicode character to lowercase.
 *
 * \param[in] SourceCharacter Specifies the character to convert.
 * \return The lowercase version of the specified Unicode character.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtldowncaseunicodechar
 */
NTSYSAPI
WCHAR
NTAPI
RtlDowncaseUnicodeChar(
    _In_ WCHAR SourceCharacter
    );

/**
 * The RtlCompareUnicodeString routine compares two Unicode strings.
 * 
 * \param[in] String1 Pointer to the first string.
 * \param[in] String2 Pointer to the second string.
 * \param[in] CaseInSensitive If TRUE, case should be ignored when doing the comparison.
 * \return A signed value that gives the results of the comparison.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlcompareunicodestring
 */
_Must_inspect_result_
NTSYSAPI
LONG
NTAPI
RtlCompareUnicodeString(
    _In_ PCUNICODE_STRING String1,
    _In_ PCUNICODE_STRING String2,
    _In_ BOOLEAN CaseInSensitive
    );

/**
 * The RtlCompareUnicodeStrings routine compares two Unicode strings.
 * 
 * \param[in] String1 Pointer to the first string.
 * \param[in] String1Length The length, in bytes, of the first string.
 * \param[in] String2 Pointer to the second string.
 * \param[in] String2Length The length, in bytes, of the second string.
 * \param[in] CaseInSensitive If TRUE, case should be ignored when doing the comparison.
 * \return A signed value that gives the results of the comparison.
 */
_Must_inspect_result_
NTSYSAPI
LONG
NTAPI
RtlCompareUnicodeStrings(
    _In_reads_(String1Length) PCWCH String1,
    _In_ SIZE_T String1Length,
    _In_reads_(String2Length) PCWCH String2,
    _In_ SIZE_T String2Length,
    _In_ BOOLEAN CaseInSensitive
    );

/**
 * The RtlEqualUnicodeString routine compares two Unicode strings to determine whether they are equal.
 *
 * \param[in] String1 Pointer to the first Unicode string.
 * \param[in] String2 Pointer to the second Unicode string.
 * \param[in] CaseInSensitive If TRUE, case should be ignored when doing the comparison.
 * \return TRUE if the two Unicode strings are equal; otherwise, it returns FALSE.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlequalunicodestring
 */
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualUnicodeString(
    _In_ PCUNICODE_STRING String1,
    _In_ PCUNICODE_STRING String2,
    _In_ BOOLEAN CaseInSensitive
    );

#define HASH_STRING_ALGORITHM_DEFAULT 0
#define HASH_STRING_ALGORITHM_X65599 1
#define HASH_STRING_ALGORITHM_INVALID 0xffffffff

/**
 * The RtlHashUnicodeString routine creates a hash value from a given Unicode string and hash algorithm.
 *
 * \param[in] String A pointer to a UNICODE_STRING structure that contains the Unicode string to be converted to a hash value.
 * \param[in] CaseInSensitive Specifies whether to treat the Unicode string as case sensitive when computing the hash value. If CaseInSensitive is TRUE, a lowercase and uppercase string hash to the same value.
 * \param[in] HashAlgorithm The hash algorithm to use.
 * \param[out] HashValue A pointer to a ULONG variable that receives the hash value.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlhashunicodestring
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlHashUnicodeString(
    _In_ PCUNICODE_STRING String,
    _In_ BOOLEAN CaseInSensitive,
    _In_ ULONG HashAlgorithm,
    _Out_ PULONG HashValue
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlValidateUnicodeString(
    _In_ ULONG Flags,
    _In_ PCUNICODE_STRING String
    );

/**
 * The RtlPrefixUnicodeString routine compares two Unicode strings to determine whether one string is a prefix of the other.
 *
 * \param[in] String1 Pointer to the first string, which might be a prefix of the buffered Unicode string at String2.
 * \param[in] String2 Pointer to the second string.
 * \param[in] CaseInSensitive TRUE, case should be ignored when doing the comparison.
 * \return TRUE if String1 is a prefix of String2.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlprefixunicodestring
 */
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlPrefixUnicodeString(
    _In_ PCUNICODE_STRING String1,
    _In_ PCUNICODE_STRING String2,
    _In_ BOOLEAN CaseInSensitive
    );

#if (PHNT_MODE == PHNT_MODE_KERNEL && PHNT_VERSION >= PHNT_WINDOWS_10)
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlSuffixUnicodeString(
    _In_ PCUNICODE_STRING String1,
    _In_ PCUNICODE_STRING String2,
    _In_ BOOLEAN CaseInSensitive
    );
#endif // PHNT_MODE == PHNT_MODE_KERNEL && PHNT_VERSION >= PHNT_WINDOWS_10

#pragma prefast(push)
#pragma prefast(disable : 6101, "Out parameter is not written fully or at all.")
FORCEINLINE
VOID
NTAPI_INLINE
RtlSanitizeUnicodeStringPadding(
    _Out_ PUNICODE_STRING String
    )
{
#if defined(_WIN64)
    ULONG PaddingSize;
    ULONG PaddingStart;

    PaddingStart = FIELD_OFFSET(UNICODE_STRING, MaximumLength) + sizeof(String->MaximumLength);
    PaddingSize = FIELD_OFFSET(UNICODE_STRING, Buffer) - PaddingStart;

    memset((PCH)String + PaddingStart, 0, PaddingSize);
#else
    UNREFERENCED_PARAMETER(String);
#endif
}
#pragma prefast(pop)

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
_Must_inspect_result_
NTSYSAPI
PWCHAR
NTAPI
RtlFindUnicodeSubstring(
    _In_ PCUNICODE_STRING FullString,
    _In_ PCUNICODE_STRING SearchString,
    _In_ BOOLEAN CaseInSensitive
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10

#define RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END 0x00000001
#define RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET 0x00000002
#define RTL_FIND_CHAR_IN_UNICODE_STRING_CASE_INSENSITIVE 0x00000004

NTSYSAPI
NTSTATUS
NTAPI
RtlFindCharInUnicodeString(
    _In_ ULONG Flags,
    _In_ PCUNICODE_STRING StringToSearch,
    _In_ PCUNICODE_STRING CharSet,
    _Out_ PUSHORT NonInclusivePrefixLength
    );

typedef struct _RTL_UNICODE_STRING_BUFFER RTL_UNICODE_STRING_BUFFER, *PRTL_UNICODE_STRING_BUFFER;

NTSYSAPI
NTSTATUS
NTAPI
RtlMultiAppendUnicodeStringBuffer(
    _Inout_ PRTL_UNICODE_STRING_BUFFER Buffer,
    _In_ ULONG BufferCount,
    _In_ PCUNICODE_STRING Source
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAppendPathElement(
    _In_ ULONG Flags,
    _Inout_ PRTL_UNICODE_STRING_BUFFER Buffer,
    _In_ PCUNICODE_STRING Source
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAppendUnicodeStringToString(
    _Inout_ PUNICODE_STRING Destination,
    _In_ PCUNICODE_STRING Source
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAppendUnicodeToString(
    _Inout_ PUNICODE_STRING Destination,
    _In_opt_z_ PCWSTR Source
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeString(
    _Inout_ PUNICODE_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDowncaseUnicodeString(
    _Inout_ PUNICODE_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

NTSYSAPI
VOID
NTAPI
RtlEraseUnicodeString(
    _Inout_ PUNICODE_STRING String
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAnsiStringToUnicodeString(
    _Inout_ PUNICODE_STRING DestinationString,
    _In_ PCANSI_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

NTSYSAPI
ULONG
NTAPI
RtlxAnsiStringToUnicodeSize(
    _In_ PCANSI_STRING AnsiString
    );

// NTSYSAPI
// ULONG
// NTAPI
// RtlAnsiStringToUnicodeSize(
//     _In_ PCANSI_STRING AnsiString
//     );

#define RtlAnsiStringToUnicodeSize(STRING) \
    RtlxAnsiStringToUnicodeSize(STRING)

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToAnsiString(
    _Inout_ PANSI_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

// rev
NTSYSAPI
ULONG
NTAPI
RtlUnicodeStringToAnsiSize(
    _In_ PCUNICODE_STRING SourceString
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToUTF8String(
    _Inout_ PUTF8_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlUTF8StringToUnicodeString(
    _Inout_ PUNICODE_STRING DestinationString,
    _In_ PCUTF8_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_20H1

NTSYSAPI
WCHAR
NTAPI
RtlAnsiCharToUnicodeChar(
    _Inout_ PUCHAR *SourceCharacter
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToAnsiString(
    _Inout_ PANSI_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlOemStringToUnicodeString(
    _Inout_ PUNICODE_STRING DestinationString,
    _In_ POEM_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToOemString(
    _Inout_ POEM_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToOemString(
    _Inout_ POEM_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlOemStringToCountedUnicodeString(
    _Inout_ PUNICODE_STRING DestinationString,
    _In_ PCOEM_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToCountedOemString(
    _Inout_ POEM_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToCountedOemString(
    _Inout_ POEM_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

/**
 * The RtlMultiByteToUnicodeN routine translates the specified source string into a Unicode string, using the current system ANSI code page (ACP).
 * The source string is not necessarily from a multibyte character set.
 *
 * \param UnicodeString Pointer to a caller-allocated buffer that receives the translated string. UnicodeString buffer must not overlap with MultiByteString buffer.
 * \param MaxBytesInUnicodeString Maximum number of bytes to be written at UnicodeString. If this value causes the translated string to be truncated, RtlMultiByteToUnicodeN does not return an error status.
 * \param BytesInUnicodeString Pointer to a caller-allocated variable that receives the length, in bytes, of the translated string. This parameter can be NULL.
 * \param MultiByteString Pointer to the string to be translated.
 * \param BytesInMultiByteString Size, in bytes, of the string at MultiByteString.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlmultibytetounicoden
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlMultiByteToUnicodeN(
    _Out_writes_bytes_to_(MaxBytesInUnicodeString, *BytesInUnicodeString) PWCH UnicodeString,
    _In_ ULONG MaxBytesInUnicodeString,
    _Out_opt_ PULONG BytesInUnicodeString,
    _In_reads_bytes_(BytesInMultiByteString) PCSTR MultiByteString,
    _In_ ULONG BytesInMultiByteString
    );

/**
 * The RtlMultiByteToUnicodeSize routine determines the number of bytes that are required to store the Unicode translation for the specified source string.
 * The translation is assumed to use the current system ANSI code page (ACP). The source string is not necessarily from a multibyte character set.
 *
 * \param BytesInUnicodeString Pointer to a caller-allocated variable that receives the number of bytes that are required to store the translated string.
 * \param MultiByteString Pointer to the source string for which the Unicode length is to be calculated.
 * \param BytesInMultiByteString Length, in bytes, of the source string.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlmultibytetounicodesize
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlMultiByteToUnicodeSize(
    _Out_ PULONG BytesInUnicodeString,
    _In_reads_bytes_(BytesInMultiByteString) PCSTR MultiByteString,
    _In_ ULONG BytesInMultiByteString
    );

/**
 * The RtlUnicodeToMultiByteN routine translates the specified Unicode string into a new character string, using the current system ANSI code page (ACP).
 * The source string is not necessarily from a multibyte character set.
 *
 * \param MultiByteString Pointer to a caller-allocated buffer to receive the translated string. MultiByteString buffer must not overlap with UnicodeString buffer.
 * \param MaxBytesInMultiByteString Maximum number of bytes to be written to MultiByteString. If this value causes the translated string to be truncated, RtlUnicodeToMultiByteN does not return an error status.
 * \param BytesInMultiByteString Pointer to a caller-allocated variable that receives the length, in bytes, of the translated string. This parameter is optional and can be NULL.
 * \param UnicodeString Pointer to the Unicode source string to be translated.
 * \param BytesInUnicodeString Size, in bytes, of the string at UnicodeString.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlunicodetomultibyten
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToMultiByteN(
    _Out_writes_bytes_to_(MaxBytesInMultiByteString, *BytesInMultiByteString) PCHAR MultiByteString,
    _In_ ULONG MaxBytesInMultiByteString,
    _Out_opt_ PULONG BytesInMultiByteString,
    _In_reads_bytes_(BytesInUnicodeString) PCWCH UnicodeString,
    _In_ ULONG BytesInUnicodeString
    );

/**
 * The RtlUnicodeToMultiByteSize routine determines the number of bytes that are required to store the multibyte translation for the specified Unicode string.
 * The translation is assumed to use the current system ANSI code page (ACP). The source string is not necessarily from a multibyte character set.
 *
 * \param BytesInMultiByteString Pointer to a caller-allocated variable that receives the number of bytes required to store the translated string.
 * \param UnicodeString Pointer to the Unicode string for which the multibyte length is to be calculated.
 * \param BytesInUnicodeString Length, in bytes, of the source string.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlunicodetomultibytesize
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToMultiByteSize(
    _Out_ PULONG BytesInMultiByteString,
    _In_reads_bytes_(BytesInUnicodeString) PCWCH UnicodeString,
    _In_ ULONG BytesInUnicodeString
    );

/**
 * The RtlUpcaseUnicodeToMultiByteN routine translates the specified Unicode string into a new uppercase character string, using the current system ANSI code page (ACP).
 * The translated string is not necessarily from a multibyte character set.
 *
 * \param MultiByteString Pointer to a caller-allocated buffer to receive the translated string.
 * \param MaxBytesInMultiByteString Maximum number of bytes to be written at MultiByteString. If this value causes the translated string to be truncated, RtlUpcaseUnicodeToMultiByteN does not return an error status.
 * \param BytesInMultiByteString Pointer to a caller-allocated variable that receives the length, in bytes, of the translated string. This parameter can be NULL.
 * \param UnicodeString Pointer to the Unicode source string to be translated.
 * \param BytesInUnicodeString Size, in bytes, of the string at UnicodeString.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlupcaseunicodetomultibyten
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeToMultiByteN(
    _Out_writes_bytes_to_(MaxBytesInMultiByteString, *BytesInMultiByteString) PCHAR MultiByteString,
    _In_ ULONG MaxBytesInMultiByteString,
    _Out_opt_ PULONG BytesInMultiByteString,
    _In_reads_bytes_(BytesInUnicodeString) PCWCH UnicodeString,
    _In_ ULONG BytesInUnicodeString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlOemToUnicodeN(
    _Out_writes_bytes_to_(MaxBytesInUnicodeString, *BytesInUnicodeString) PWSTR UnicodeString,
    _In_ ULONG MaxBytesInUnicodeString,
    _Out_opt_ PULONG BytesInUnicodeString,
    _In_reads_bytes_(BytesInOemString) PCCH OemString,
    _In_ ULONG BytesInOemString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToOemN(
    _Out_writes_bytes_to_(MaxBytesInOemString, *BytesInOemString) PCHAR OemString,
    _In_ ULONG MaxBytesInOemString,
    _Out_opt_ PULONG BytesInOemString,
    _In_reads_bytes_(BytesInUnicodeString) PCWCH UnicodeString,
    _In_ ULONG BytesInUnicodeString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeToOemN(
    _Out_writes_bytes_to_(MaxBytesInOemString, *BytesInOemString) PCHAR OemString,
    _In_ ULONG MaxBytesInOemString,
    _Out_opt_ PULONG BytesInOemString,
    _In_reads_bytes_(BytesInUnicodeString) PCWCH UnicodeString,
    _In_ ULONG BytesInUnicodeString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlConsoleMultiByteToUnicodeN(
    _Out_writes_bytes_to_(MaxBytesInUnicodeString, *BytesInUnicodeString) PWCH UnicodeString,
    _In_ ULONG MaxBytesInUnicodeString,
    _Out_opt_ PULONG BytesInUnicodeString,
    _In_reads_bytes_(BytesInMultiByteString) PCCH MultiByteString,
    _In_ ULONG BytesInMultiByteString,
    _Out_ PULONG pdwSpecialChar
    );

/**
 * The RtlUTF8ToUnicodeN routine translates the specified source string into a Unicode string, using the 8-bit Unicode Transformation Format (UTF-8) code page.
 *
 * \param UnicodeStringDestination Pointer to a caller-allocated buffer to receive the translated string.
 * \param UnicodeStringMaxByteCount Maximum number of bytes to be written at MultiByteString. If this value causes the translated string to be truncated, RtlUpcaseUnicodeToMultiByteN does not return an error status.
 * \param UnicodeStringActualByteCount Pointer to a caller-allocated variable that receives the length, in bytes, of the translated string. This parameter can be NULL.
 * \param UTF8StringSource Pointer to the Unicode source string to be translated.
 * \param UTF8StringByteCount Size, in bytes, of the string at UnicodeString.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/devnotes/rtlutf8tounicoden
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlUTF8ToUnicodeN(
    _Out_writes_bytes_to_(UnicodeStringMaxByteCount, *UnicodeStringActualByteCount) PWSTR UnicodeStringDestination,
    _In_ ULONG UnicodeStringMaxByteCount,
    _Out_opt_ PULONG UnicodeStringActualByteCount,
    _In_reads_bytes_(UTF8StringByteCount) PCCH UTF8StringSource,
    _In_ ULONG UTF8StringByteCount
    );

/**
 * The RtlUnicodeToUTF8N routine translates the specified Unicode string into a new character string, using the 8-bit Unicode Transformation Format (UTF-8) code page.
 *
 * \param UTF8StringDestination Pointer to a caller-allocated buffer to receive the translated string.
 * \param UTF8StringMaxByteCount Maximum number of bytes to be written to UTF8StringDestination. If this value causes the translated string to be truncated, RtlUnicodeToUTF8N returns an error status.
 * \param UTF8StringActualByteCount A pointer to a caller-allocated variable that receives the length, in bytes, of the translated string. This parameter is optional and can be NULL. If the string is truncated then the returned number counts the actual truncated string count.
 * \param UnicodeStringSource A pointer to the Unicode source string to be translated.
 * \param UnicodeStringByteCount Specifies the number of bytes in the Unicode source string that the UnicodeStringSource parameter points to.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/devnotes/rtlunicodetoutf8n
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToUTF8N(
    _Out_writes_bytes_to_(UTF8StringMaxByteCount, *UTF8StringActualByteCount) PCHAR UTF8StringDestination,
    _In_ ULONG UTF8StringMaxByteCount,
    _Out_opt_ PULONG UTF8StringActualByteCount,
    _In_reads_bytes_(UnicodeStringByteCount) PCWCH UnicodeStringSource,
    _In_ ULONG UnicodeStringByteCount
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCustomCPToUnicodeN(
    _In_ PCPTABLEINFO CustomCP,
    _Out_writes_bytes_to_(MaxBytesInUnicodeString, *BytesInUnicodeString) PWCH UnicodeString,
    _In_ ULONG MaxBytesInUnicodeString,
    _Out_opt_ PULONG BytesInUnicodeString,
    _In_reads_bytes_(BytesInCustomCPString) PCH CustomCPString,
    _In_ ULONG BytesInCustomCPString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeToCustomCPN(
    _In_ PCPTABLEINFO CustomCP,
    _Out_writes_bytes_to_(MaxBytesInCustomCPString, *BytesInCustomCPString) PCH CustomCPString,
    _In_ ULONG MaxBytesInCustomCPString,
    _Out_opt_ PULONG BytesInCustomCPString,
    _In_reads_bytes_(BytesInUnicodeString) PWCH UnicodeString,
    _In_ ULONG BytesInUnicodeString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeToCustomCPN(
    _In_ PCPTABLEINFO CustomCP,
    _Out_writes_bytes_to_(MaxBytesInCustomCPString, *BytesInCustomCPString) PCH CustomCPString,
    _In_ ULONG MaxBytesInCustomCPString,
    _Out_opt_ PULONG BytesInCustomCPString,
    _In_reads_bytes_(BytesInUnicodeString) PWCH UnicodeString,
    _In_ ULONG BytesInUnicodeString
    );

NTSYSAPI
VOID
NTAPI
RtlInitCodePageTable(
    _In_reads_opt_(2) PUSHORT TableBase,
    _Inout_ PCPTABLEINFO CodePageTable
    );

#if (PHNT_VERSION < PHNT_WINDOWS_11)
NTSYSAPI
VOID
NTAPI
RtlInitNlsTables(
    _In_ PUSHORT AnsiNlsBase,
    _In_ PUSHORT OemNlsBase,
    _In_ PUSHORT LanguageNlsBase,
    _Out_ PNLSTABLEINFO TableInfo // PCPTABLEINFO?
    );
#endif

NTSYSAPI
VOID
NTAPI
RtlResetRtlTranslations(
    _In_ PNLSTABLEINFO TableInfo
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlIsTextUnicode(
    _In_ PVOID Buffer,
    _In_ ULONG Size,
    _Inout_opt_ PULONG Result
    );

typedef enum _RTL_NORM_FORM
{
    NormOther = 0x0,
    NormC = 0x1,
    NormD = 0x2,
    NormKC = 0x5,
    NormKD = 0x6,
    NormIdna = 0xd,
    DisallowUnassigned = 0x100,
    NormCDisallowUnassigned = 0x101,
    NormDDisallowUnassigned = 0x102,
    NormKCDisallowUnassigned = 0x105,
    NormKDDisallowUnassigned = 0x106,
    NormIdnaDisallowUnassigned = 0x10d
} RTL_NORM_FORM;

NTSYSAPI
NTSTATUS
NTAPI
RtlNormalizeString(
    _In_ ULONG NormForm, // RTL_NORM_FORM
    _In_ PCWSTR SourceString,
    _In_ LONG SourceStringLength,
    _Out_writes_to_(*DestinationStringLength, *DestinationStringLength) PWSTR DestinationString,
    _Inout_ PLONG DestinationStringLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlIsNormalizedString(
    _In_ ULONG NormForm, // RTL_NORM_FORM
    _In_ PCWSTR SourceString,
    _In_ LONG SourceStringLength,
    _Out_ PBOOLEAN Normalized
    );

// ntifs:FsRtlIsNameInExpression
NTSYSAPI
BOOLEAN
NTAPI
RtlIsNameInExpression(
    _In_ PCUNICODE_STRING Expression,
    _In_ PCUNICODE_STRING Name,
    _In_ BOOLEAN IgnoreCase,
    _In_opt_ PWCH UpcaseTable
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS4)
// rev
NTSYSAPI
BOOLEAN
NTAPI
RtlIsNameInUnUpcasedExpression(
    _In_ PCUNICODE_STRING Expression,
    _In_ PCUNICODE_STRING Name,
    _In_ BOOLEAN IgnoreCase,
    _In_opt_ PWCH UpcaseTable
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS4

#if (PHNT_VERSION >= PHNT_WINDOWS_10_19H1)
NTSYSAPI
BOOLEAN
NTAPI
RtlDoesNameContainWildCards(
    _In_ PCUNICODE_STRING Expression
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_19H1

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualDomainName(
    _In_ PCUNICODE_STRING String1,
    _In_ PCUNICODE_STRING String2
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualComputerName(
    _In_ PCUNICODE_STRING String1,
    _In_ PCUNICODE_STRING String2
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDnsHostNameToComputerName(
    _Out_ PUNICODE_STRING ComputerNameString,
    _In_ PCUNICODE_STRING DnsHostNameString,
    _In_ BOOLEAN AllocateComputerNameString
    );

/**
 * The RtlStringFromGUID routine converts a given GUID from binary format into a Unicode string.
 *
 * \param[in] Guid Specifies the binary-format GUID to convert.
 * \param[out] GuidString Pointer to a caller-supplied variable in which a pointer to the converted GUID string is returned and must free by calling RtlFreeUnicodeString.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlstringfromguid
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlStringFromGUID(
    _In_ PGUID Guid,
    _Out_ PUNICODE_STRING GuidString
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8_1)

#define RTL_GUID_STRING_SIZE 38
#define MAX_UNICODE_GUID_STRING_LENGTH (36 + sizeof(UNICODE_NULL))

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlStringFromGUIDEx(
    _In_ PGUID Guid,
    _Inout_ PUNICODE_STRING GuidString,
    _In_ BOOLEAN AllocateGuidString
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_8_1

NTSYSAPI
NTSTATUS
NTAPI
RtlGUIDFromString(
    _In_ PCUNICODE_STRING GuidString,
    _Out_ PGUID Guid
    );

NTSYSAPI
LONG
NTAPI
RtlCompareAltitudes(
    _In_ PCUNICODE_STRING Altitude1,
    _In_ PCUNICODE_STRING Altitude2
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlIdnToAscii(
    _In_ ULONG Flags,
    _In_ PCWSTR SourceString,
    _In_ LONG SourceStringLength,
    _Out_writes_to_(*DestinationStringLength, *DestinationStringLength) PWSTR DestinationString,
    _Inout_ PLONG DestinationStringLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlIdnToUnicode(
    _In_ ULONG Flags,
    _In_ PCWSTR SourceString,
    _In_ LONG SourceStringLength,
    _Out_writes_to_(*DestinationStringLength, *DestinationStringLength) PWSTR DestinationString,
    _Inout_ PLONG DestinationStringLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlIdnToNameprepUnicode(
    _In_ ULONG Flags,
    _In_ PCWSTR SourceString,
    _In_ LONG SourceStringLength,
    _Out_writes_to_(*DestinationStringLength, *DestinationStringLength) PWSTR DestinationString,
    _Inout_ PLONG DestinationStringLength
    );

//
// Prefix
//

typedef struct _PREFIX_TABLE_ENTRY
{
    CSHORT NodeTypeCode;
    CSHORT NameLength;
    struct _PREFIX_TABLE_ENTRY *NextPrefixTree;
    RTL_SPLAY_LINKS Links;
    PSTRING Prefix;
} PREFIX_TABLE_ENTRY, *PPREFIX_TABLE_ENTRY;

typedef struct _PREFIX_TABLE
{
    CSHORT NodeTypeCode;
    CSHORT NameLength;
    PPREFIX_TABLE_ENTRY NextPrefixTree;
} PREFIX_TABLE, *PPREFIX_TABLE;

NTSYSAPI
VOID
NTAPI
PfxInitialize(
    _Out_ PPREFIX_TABLE PrefixTable
    );

NTSYSAPI
BOOLEAN
NTAPI
PfxInsertPrefix(
    _In_ PPREFIX_TABLE PrefixTable,
    _In_ __drv_aliasesMem PSTRING Prefix,
    _Out_ PPREFIX_TABLE_ENTRY PrefixTableEntry
    );

NTSYSAPI
VOID
NTAPI
PfxRemovePrefix(
    _In_ PPREFIX_TABLE PrefixTable,
    _In_ PPREFIX_TABLE_ENTRY PrefixTableEntry
    );

NTSYSAPI
PPREFIX_TABLE_ENTRY
NTAPI
PfxFindPrefix(
    _In_ PPREFIX_TABLE PrefixTable,
    _In_ PSTRING FullName
    );

typedef struct _UNICODE_PREFIX_TABLE_ENTRY
{
    CSHORT NodeTypeCode;
    CSHORT NameLength;
    struct _UNICODE_PREFIX_TABLE_ENTRY *NextPrefixTree;
    struct _UNICODE_PREFIX_TABLE_ENTRY *CaseMatch;
    RTL_SPLAY_LINKS Links;
    PUNICODE_STRING Prefix;
} UNICODE_PREFIX_TABLE_ENTRY, *PUNICODE_PREFIX_TABLE_ENTRY;

typedef struct _UNICODE_PREFIX_TABLE
{
    CSHORT NodeTypeCode;
    CSHORT NameLength;
    PUNICODE_PREFIX_TABLE_ENTRY NextPrefixTree;
    PUNICODE_PREFIX_TABLE_ENTRY LastNextEntry;
} UNICODE_PREFIX_TABLE, *PUNICODE_PREFIX_TABLE;

NTSYSAPI
VOID
NTAPI
RtlInitializeUnicodePrefix(
    _Out_ PUNICODE_PREFIX_TABLE PrefixTable
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlInsertUnicodePrefix(
    _In_ PUNICODE_PREFIX_TABLE PrefixTable,
    _In_ __drv_aliasesMem PCUNICODE_STRING Prefix,
    _Out_ PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry
    );

NTSYSAPI
VOID
NTAPI
RtlRemoveUnicodePrefix(
    _In_ PUNICODE_PREFIX_TABLE PrefixTable,
    _In_ PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry
    );

NTSYSAPI
PUNICODE_PREFIX_TABLE_ENTRY
NTAPI
RtlFindUnicodePrefix(
    _In_ PUNICODE_PREFIX_TABLE PrefixTable,
    _In_ PCUNICODE_STRING FullName,
    _In_ ULONG CaseInsensitiveIndex
    );

NTSYSAPI
PUNICODE_PREFIX_TABLE_ENTRY
NTAPI
RtlNextUnicodePrefix(
    _In_ PUNICODE_PREFIX_TABLE PrefixTable,
    _In_ BOOLEAN Restart
    );

//
// Compression
//

#define COMPRESSION_FORMAT_NONE          (0x0000)
#define COMPRESSION_FORMAT_DEFAULT       (0x0001)
#define COMPRESSION_FORMAT_LZNT1         (0x0002)
#define COMPRESSION_FORMAT_XPRESS        (0x0003)
#define COMPRESSION_FORMAT_XPRESS_HUFF   (0x0004)
#define COMPRESSION_FORMAT_XP10          (0x0005)
#define COMPRESSION_FORMAT_LZ4           (0x0006)
#define COMPRESSION_FORMAT_DEFLATE       (0x0007)
#define COMPRESSION_FORMAT_ZLIB          (0x0008)
#define COMPRESSION_FORMAT_MAX           (0x0008)

#define COMPRESSION_ENGINE_STANDARD      (0x0000)
#define COMPRESSION_ENGINE_MAXIMUM       (0x0100)
#define COMPRESSION_ENGINE_HIBER         (0x0200)
#define COMPRESSION_ENGINE_MAX           (0x0200)

#define COMPRESSION_FORMAT_MASK          (0x00FF)
#define COMPRESSION_ENGINE_MASK          (0xFF00)
#define COMPRESSION_FORMAT_ENGINE_MASK   (COMPRESSION_FORMAT_MASK | COMPRESSION_ENGINE_MASK)

typedef struct _COMPRESSED_DATA_INFO
{
    //
    //  Code for the compression format (and engine) as
    //  defined in ntrtl.h.  Note that COMPRESSION_FORMAT_NONE
    //  and COMPRESSION_FORMAT_DEFAULT are invalid if
    //  any of the described chunks are compressed.
    //

    USHORT CompressionFormatAndEngine;

    //
    //  Since chunks and compression units are expected to be
    //  powers of 2 in size, we express then log2.  So, for
    //  example (1 << ChunkShift) == ChunkSizeInBytes.  The
    //  ClusterShift indicates how much space must be saved
    //  to successfully compress a compression unit - each
    //  successfully compressed compression unit must occupy
    //  at least one cluster less in bytes than an uncompressed
    //  compression unit.
    //

    UCHAR CompressionUnitShift;
    UCHAR ChunkShift;
    UCHAR ClusterShift;
    UCHAR Reserved;

    //
    //  This is the number of entries in the CompressedChunkSizes
    //  array.
    //

    USHORT NumberOfChunks;

    //
    //  This is an array of the sizes of all chunks resident
    //  in the compressed data buffer.  There must be one entry
    //  in this array for each chunk possible in the uncompressed
    //  buffer size.  A size of FSRTL_CHUNK_SIZE indicates the
    //  corresponding chunk is uncompressed and occupies exactly
    //  that size.  A size of 0 indicates that the corresponding
    //  chunk contains nothing but binary 0's, and occupies no
    //  space in the compressed data.  All other sizes must be
    //  less than FSRTL_CHUNK_SIZE, and indicate the exact size
    //  of the compressed data in bytes.
    //

    ULONG CompressedChunkSizes[ANYSIZE_ARRAY];
} COMPRESSED_DATA_INFO, *PCOMPRESSED_DATA_INFO;

NTSYSAPI
NTSTATUS
NTAPI
RtlGetCompressionWorkSpaceSize(
    _In_ USHORT CompressionFormatAndEngine,
    _Out_ PULONG CompressBufferWorkSpaceSize,
    _Out_ PULONG CompressFragmentWorkSpaceSize
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCompressBuffer(
    _In_ USHORT CompressionFormatAndEngine,
    _In_reads_bytes_(UncompressedBufferSize) PUCHAR UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _Out_writes_bytes_to_(CompressedBufferSize, *FinalCompressedSize) PUCHAR CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _In_ ULONG UncompressedChunkSize,
    _Out_ PULONG FinalCompressedSize,
    _In_ PVOID WorkSpace
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDecompressBuffer(
    _In_ USHORT CompressionFormat,
    _Out_writes_bytes_to_(UncompressedBufferSize, *FinalUncompressedSize) PUCHAR UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _In_reads_bytes_(CompressedBufferSize) PUCHAR CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _Out_ PULONG FinalUncompressedSize
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
NTSYSAPI
NTSTATUS
NTAPI
RtlDecompressBufferEx(
    _In_ USHORT CompressionFormat,
    _Out_writes_bytes_to_(UncompressedBufferSize, *FinalUncompressedSize) PUCHAR UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _In_reads_bytes_(CompressedBufferSize) PUCHAR CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _Out_ PULONG FinalUncompressedSize,
    _In_opt_ PVOID WorkSpace
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8

#if (PHNT_VERSION >= PHNT_WINDOWS_8_1)
NTSYSAPI
NTSTATUS
NTAPI
RtlDecompressBufferEx2(
    _In_ USHORT CompressionFormat,
    _Out_writes_bytes_to_(UncompressedBufferSize, *FinalUncompressedSize) PUCHAR UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _In_reads_bytes_(CompressedBufferSize) PUCHAR CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _In_ ULONG UncompressedChunkSize,
    _Out_ PULONG FinalUncompressedSize,
    _In_opt_ PVOID WorkSpace
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8_1

NTSYSAPI
NTSTATUS
NTAPI
RtlDecompressFragment(
    _In_ USHORT CompressionFormat,
    _Out_writes_bytes_to_(UncompressedFragmentSize, *FinalUncompressedSize) PUCHAR UncompressedFragment,
    _In_ ULONG UncompressedFragmentSize,
    _In_reads_bytes_(CompressedBufferSize) PUCHAR CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _In_range_(<, CompressedBufferSize) ULONG FragmentOffset,
    _Out_ PULONG FinalUncompressedSize,
    _In_ PVOID WorkSpace
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8_1)
NTSYSAPI
NTSTATUS
NTAPI
RtlDecompressFragmentEx(
    _In_ USHORT CompressionFormat,
    _Out_writes_bytes_to_(UncompressedFragmentSize, *FinalUncompressedSize) PUCHAR UncompressedFragment,
    _In_ ULONG UncompressedFragmentSize,
    _In_reads_bytes_(CompressedBufferSize) PUCHAR CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _In_range_(<, CompressedBufferSize) ULONG FragmentOffset,
    _In_ ULONG UncompressedChunkSize,
    _Out_ PULONG FinalUncompressedSize,
    _In_ PVOID WorkSpace
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8_1

NTSYSAPI
NTSTATUS
NTAPI
RtlDescribeChunk(
    _In_ USHORT CompressionFormat,
    _Inout_ PUCHAR *CompressedBuffer,
    _In_ PUCHAR EndOfCompressedBufferPlus1,
    _Out_ PUCHAR *ChunkBuffer,
    _Out_ PULONG ChunkSize
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlReserveChunk(
    _In_ USHORT CompressionFormat,
    _Inout_ PUCHAR *CompressedBuffer,
    _In_ PUCHAR EndOfCompressedBufferPlus1,
    _Out_ PUCHAR *ChunkBuffer,
    _In_ ULONG ChunkSize
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDecompressChunks(
    _Out_writes_bytes_(UncompressedBufferSize) PUCHAR UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _In_reads_bytes_(CompressedBufferSize) PUCHAR CompressedBuffer,
    _In_ ULONG CompressedBufferSize,
    _In_reads_bytes_(CompressedTailSize) PUCHAR CompressedTail,
    _In_ ULONG CompressedTailSize,
    _In_ PCOMPRESSED_DATA_INFO CompressedDataInfo
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCompressChunks(
    _In_reads_bytes_(UncompressedBufferSize) PUCHAR UncompressedBuffer,
    _In_ ULONG UncompressedBufferSize,
    _Out_writes_bytes_(CompressedBufferSize) PUCHAR CompressedBuffer,
    _In_range_(>=, (UncompressedBufferSize - (UncompressedBufferSize / 16))) ULONG CompressedBufferSize,
    _Inout_updates_bytes_(CompressedDataInfoLength) PCOMPRESSED_DATA_INFO CompressedDataInfo,
    _In_range_(>, sizeof(COMPRESSED_DATA_INFO)) ULONG CompressedDataInfoLength,
    _In_ PVOID WorkSpace
    );

//
// Locale
//

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlConvertLCIDToString(
    _In_ LCID LcidValue,
    _In_ ULONG Base,
    _In_ ULONG Padding, // string is padded to this width
    _Out_writes_(Size) PWSTR pResultBuf,
    _In_ ULONG Size
    );

// private
NTSYSAPI
BOOLEAN
NTAPI
RtlIsValidLocaleName(
    _In_ PCWSTR LocaleName,
    _In_ ULONG Flags
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlGetParentLocaleName(
    _In_ PCWSTR LocaleName,
    _Inout_ PUNICODE_STRING ParentLocaleName,
    _In_ ULONG Flags,
    _In_ BOOLEAN AllocateDestinationString
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlLcidToLocaleName(
    _In_ LCID lcid, // sic
    _Inout_ PUNICODE_STRING LocaleName,
    _In_ ULONG Flags,
    _In_ BOOLEAN AllocateDestinationString
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlLocaleNameToLcid(
    _In_ PCWSTR LocaleName,
    _Out_ PLCID lcid,
    _In_ ULONG Flags
    );

// private
NTSYSAPI
BOOLEAN
NTAPI
RtlLCIDToCultureName(
    _In_ LCID Lcid,
    _Inout_ PUNICODE_STRING String
    );

// private
NTSYSAPI
BOOLEAN
NTAPI
RtlCultureNameToLCID(
    _In_ PCUNICODE_STRING String,
    _Out_ PLCID Lcid
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
RtlpConvertLCIDsToCultureNames(
    _In_ PCWSTR Lcids, // array
    _Out_ PCWSTR* CultureNames
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
RtlpConvertCultureNamesToLCIDs(
    _In_ PCWSTR CultureNames, // array
    _Out_ PCWSTR* Lcids
    );

// private
NTSYSAPI
VOID
NTAPI
RtlCleanUpTEBLangLists(
    VOID
    );

// rev from GetThreadPreferredUILanguages
NTSYSAPI
NTSTATUS
NTAPI
RtlGetThreadPreferredUILanguages(
    _In_ ULONG Flags, // MUI_LANGUAGE_NAME
    _Out_ PULONG NumberOfLanguages,
    _Out_writes_opt_(*ReturnLength) PZZWSTR Languages,
    _Inout_ PULONG ReturnLength
    );

// rev from GetProcessPreferredUILanguages
NTSYSAPI
NTSTATUS
NTAPI
RtlGetProcessPreferredUILanguages(
    _In_ ULONG Flags, // MUI_LANGUAGE_NAME
    _Out_ PULONG NumberOfLanguages,
    _Out_writes_opt_(*ReturnLength) PZZWSTR Languages,
    _Inout_ PULONG ReturnLength
    );

// rev from GetSystemPreferredUILanguages
NTSYSAPI
NTSTATUS
NTAPI
RtlGetSystemPreferredUILanguages(
    _In_ ULONG Flags, // MUI_LANGUAGE_NAME
    _In_opt_ PCWSTR LocaleName,
    _Out_ PULONG NumberOfLanguages,
    _Out_writes_opt_(*ReturnLength) PZZWSTR Languages,
    _Inout_ PULONG ReturnLength
    );

// rev from GetSystemDefaultUILanguage
NTSYSAPI
NTSTATUS
NTAPI
RtlpGetSystemDefaultUILanguage(
    _Out_ LANGID DefaultUILanguageId,
    _Inout_ PLCID Lcid
    );

// rev from GetUserPreferredUILanguages
NTSYSAPI
NTSTATUS
NTAPI
RtlGetUserPreferredUILanguages(
    _In_ ULONG Flags, // MUI_LANGUAGE_NAME
    _In_opt_ PCWSTR LocaleName,
    _Out_ PULONG NumberOfLanguages,
    _Out_writes_opt_(*ReturnLength) PZZWSTR Languages,
    _Inout_ PULONG ReturnLength
    );

// rev from GetUILanguageInfo
NTSYSAPI
NTSTATUS
NTAPI
RtlGetUILanguageInfo(
    _In_ ULONG Flags,
    _In_ PCZZWSTR Languages,
    _Out_writes_opt_(*NumberOfFallbackLanguages) PZZWSTR FallbackLanguages,
    _Inout_opt_ PULONG NumberOfFallbackLanguages,
    _Out_ PULONG Attributes
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlGetLocaleFileMappingAddress(
    _Out_ PVOID *BaseAddress,
    _Out_ PLCID DefaultLocaleId,
    _Out_ PLARGE_INTEGER DefaultCasingTableSize,
    _Out_opt_ PULONG CurrentNLSVersion
    );

//
// PEB
//

NTSYSAPI
PPEB
NTAPI
RtlGetCurrentPeb(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAcquirePebLock(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlReleasePebLock(
    VOID
    );

// private
NTSYSAPI
LOGICAL
NTAPI
RtlTryAcquirePebLock(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAllocateFromPeb(
    _In_ ULONG Size,
    _Out_ PVOID *Block
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlFreeToPeb(
    _In_ PVOID Block,
    _In_ ULONG Size
    );

//
// Processes
//

// CURDIR Handle | Flags
#define RTL_USER_PROC_CURDIR_CLOSE 0x00000002
#define RTL_USER_PROC_CURDIR_INHERIT 0x00000003

typedef struct _CURDIR
{
    UNICODE_STRING DosPath;
    HANDLE Handle;
} CURDIR, *PCURDIR;

// RTL_DRIVE_LETTER_CURDIR Flags
#define RTL_MAX_DRIVE_LETTERS 32
#define RTL_DRIVE_LETTER_VALID (USHORT)0x0001

typedef struct _RTL_DRIVE_LETTER_CURDIR
{
    USHORT Flags;
    USHORT Length;
    ULONG TimeStamp;
    STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

#define RTL_USER_PROC_DETACHED_PROCESS ((HANDLE)(LONG_PTR)-1)
#define RTL_USER_PROC_CREATE_NEW_CONSOLE ((HANDLE)(LONG_PTR)-2)
#define RTL_USER_PROC_CREATE_NO_WINDOW ((HANDLE)(LONG_PTR)-3)

typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    ULONG MaximumLength;
    ULONG Length;

    ULONG Flags;
    ULONG DebugFlags;

    HANDLE ConsoleHandle;
    ULONG ConsoleFlags;
    HANDLE StandardInput;
    HANDLE StandardOutput;
    HANDLE StandardError;

    CURDIR CurrentDirectory;
    UNICODE_STRING DllPath;
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
    PVOID Environment;

    ULONG StartingX;
    ULONG StartingY;
    ULONG CountX;
    ULONG CountY;
    ULONG CountCharsX;
    ULONG CountCharsY;
    ULONG FillAttribute;

    ULONG WindowFlags;
    ULONG ShowWindowFlags;
    UNICODE_STRING WindowTitle;
    UNICODE_STRING DesktopInfo;
    UNICODE_STRING ShellInfo;
    UNICODE_STRING RuntimeData;
    RTL_DRIVE_LETTER_CURDIR CurrentDirectories[RTL_MAX_DRIVE_LETTERS];

    ULONG_PTR EnvironmentSize;
    ULONG_PTR EnvironmentVersion;

    PVOID PackageDependencyData;
    ULONG ProcessGroupId;
    ULONG LoaderThreads;
    UNICODE_STRING RedirectionDllName; // REDSTONE4
    UNICODE_STRING HeapPartitionName; // 19H1
    PULONGLONG DefaultThreadpoolCpuSetMasks;
    ULONG DefaultThreadpoolCpuSetMaskCount;
    ULONG DefaultThreadpoolThreadMaximum;
    ULONG HeapMemoryTypeMask; // WIN11
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

// RTL_USER_PROCESS_PARAMETERS Flags
#define RTL_USER_PROC_PARAMS_NORMALIZED                 0x00000001
#define RTL_USER_PROC_PROFILE_USER                      0x00000002
#define RTL_USER_PROC_PROFILE_KERNEL                    0x00000004
#define RTL_USER_PROC_PROFILE_SERVER                    0x00000008
#define RTL_USER_PROC_RESERVE_1MB                       0x00000020
#define RTL_USER_PROC_RESERVE_16MB                      0x00000040
#define RTL_USER_PROC_CASE_SENSITIVE                    0x00000080
#define RTL_USER_PROC_DISABLE_HEAP_DECOMMIT             0x00000100
#define RTL_USER_PROC_DLL_REDIRECTION_LOCAL             0x00001000
#define RTL_USER_PROC_APP_MANIFEST_PRESENT              0x00002000
#define RTL_USER_PROC_IMAGE_KEY_MISSING                 0x00004000
#define RTL_USER_PROC_DEV_OVERRIDE_ENABLED              0x00008000
#define RTL_USER_PROC_OPTIN_PROCESS                     0x00020000
#define RTL_USER_PROC_OPTIN_PROCESS                     0x00020000
#define RTL_USER_PROC_SESSION_OWNER                     0x00040000
#define RTL_USER_PROC_HANDLE_USER_CALLBACK_EXCEPTIONS   0x00080000
#define RTL_USER_PROC_PROTECTED_PROCESS                 0x00400000
#define RTL_USER_PROC_SECURE_PROCESS                    0x80000000

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateProcessParameters(
    _Out_ PRTL_USER_PROCESS_PARAMETERS *ProcessParameters,
    _In_ PCUNICODE_STRING ImagePathName,
    _In_opt_ PCUNICODE_STRING DllPath,
    _In_opt_ PCUNICODE_STRING CurrentDirectory,
    _In_opt_ PCUNICODE_STRING CommandLine,
    _In_opt_ PVOID Environment,
    _In_opt_ PCUNICODE_STRING WindowTitle,
    _In_opt_ PCUNICODE_STRING DesktopInfo,
    _In_opt_ PCUNICODE_STRING ShellInfo,
    _In_opt_ PCUNICODE_STRING RuntimeData
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateProcessParametersEx(
    _Out_ PRTL_USER_PROCESS_PARAMETERS *ProcessParameters,
    _In_ PCUNICODE_STRING ImagePathName,
    _In_opt_ PCUNICODE_STRING DllPath,
    _In_opt_ PCUNICODE_STRING CurrentDirectory,
    _In_opt_ PCUNICODE_STRING CommandLine,
    _In_opt_ PVOID Environment,
    _In_opt_ PCUNICODE_STRING WindowTitle,
    _In_opt_ PCUNICODE_STRING DesktopInfo,
    _In_opt_ PCUNICODE_STRING ShellInfo,
    _In_opt_ PCUNICODE_STRING RuntimeData,
    _In_ ULONG Flags // pass RTL_USER_PROC_PARAMS_NORMALIZED to keep parameters normalized
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS4)
// private
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateProcessParametersWithTemplate(
    _Out_ PRTL_USER_PROCESS_PARAMETERS *ProcessParameters,
    _In_ PCUNICODE_STRING ImagePathName,
    _In_opt_ PCUNICODE_STRING DllPath,
    _In_opt_ PCUNICODE_STRING CurrentDirectory,
    _In_opt_ PCUNICODE_STRING CommandLine,
    _In_opt_ PVOID Environment,
    _In_opt_ PCUNICODE_STRING WindowTitle,
    _In_opt_ PCUNICODE_STRING DesktopInfo,
    _In_opt_ PCUNICODE_STRING ShellInfo,
    _In_opt_ PCUNICODE_STRING RuntimeData,
    _In_opt_ PCUNICODE_STRING RedirectionDllName,
    _In_ ULONG Flags // pass RTL_USER_PROC_PARAMS_NORMALIZED to keep parameters normalized
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS4

NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyProcessParameters(
    _In_ _Post_invalid_ PRTL_USER_PROCESS_PARAMETERS ProcessParameters
    );

NTSYSAPI
PRTL_USER_PROCESS_PARAMETERS
NTAPI
RtlNormalizeProcessParams(
    _Inout_ PRTL_USER_PROCESS_PARAMETERS ProcessParameters
    );

NTSYSAPI
PRTL_USER_PROCESS_PARAMETERS
NTAPI
RtlDeNormalizeProcessParams(
    _Inout_ PRTL_USER_PROCESS_PARAMETERS ProcessParameters
    );

typedef struct _RTL_USER_PROCESS_INFORMATION
{
    ULONG Length;
    HANDLE ProcessHandle;
    HANDLE ThreadHandle;
    CLIENT_ID ClientId;
    SECTION_IMAGE_INFORMATION ImageInformation;
} RTL_USER_PROCESS_INFORMATION, *PRTL_USER_PROCESS_INFORMATION;

// private
/**
 * Creates a new process and its primary thread. The new process runs in the security context of the calling process.
 *
 * @param NtImagePathName The path of the image to be executed.
 * @param ExtendedParameters Reserved
 * @param ProcessParameters The process parameter information.
 * @param ProcessSecurityDescriptor The security descriptor for the new process. If NULL, the process gets a default security descriptor.
 * @param ThreadSecurityDescriptor The security descriptor for the initial thread. If NULL, the thread gets a default security descriptor.
 * @param ParentProcess The handle of a process to use (instead of the calling process) as the parent for the process being created.
 * @param InheritHandles If this parameter is TRUE, each inheritable handle in the calling process is inherited by the new process.
 * @param DebugPort The handle of an ALPC port for debug messages. If NULL, the process gets a default port. (WindowsErrorReportingServicePort)
 * @param TokenHandle The handle of a Token to use as the security context.
 * @param ProcessInformation The user process information.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateUserProcess(
    _In_ PCUNICODE_STRING NtImagePathName,
    _In_ ULONG ExtendedParameters, // HIWORD(NumaNodeNumber), LOWORD(Reserved)
    _In_ PRTL_USER_PROCESS_PARAMETERS ProcessParameters,
    _In_opt_ PSECURITY_DESCRIPTOR ProcessSecurityDescriptor,
    _In_opt_ PSECURITY_DESCRIPTOR ThreadSecurityDescriptor,
    _In_opt_ HANDLE ParentProcess,
    _In_ BOOLEAN InheritHandles,
    _In_opt_ HANDLE DebugPort,
    _In_opt_ HANDLE TokenHandle, // used to be ExceptionPort
    _Out_ PRTL_USER_PROCESS_INFORMATION ProcessInformation
    );

#define RTL_USER_PROCESS_EXTENDED_PARAMETERS_VERSION 1

// private
typedef struct _RTL_USER_PROCESS_EXTENDED_PARAMETERS
{
    USHORT Version;
    USHORT NodeNumber;
    PSECURITY_DESCRIPTOR ProcessSecurityDescriptor;
    PSECURITY_DESCRIPTOR ThreadSecurityDescriptor;
    HANDLE ParentProcess;
    HANDLE DebugPort;
    HANDLE TokenHandle;
    HANDLE JobHandle;
} RTL_USER_PROCESS_EXTENDED_PARAMETERS, *PRTL_USER_PROCESS_EXTENDED_PARAMETERS;

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS2)
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateUserProcessEx(
    _In_ PCUNICODE_STRING NtImagePathName,
    _In_ PRTL_USER_PROCESS_PARAMETERS ProcessParameters,
    _In_ BOOLEAN InheritHandles,
    _In_opt_ PRTL_USER_PROCESS_EXTENDED_PARAMETERS ProcessExtendedParameters,
    _Out_ PRTL_USER_PROCESS_INFORMATION ProcessInformation
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS2

/**
 * Ends the calling process and all its threads.
 *
 * \param ExitStatus The exit status for the process and all threads.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-exitprocess
 * \remarks This function does not return to the caller. It terminates the process and all threads immediately.
 */
_Analysis_noreturn_
DECLSPEC_NORETURN
NTSYSAPI
VOID
NTAPI
RtlExitUserProcess(
    _In_ NTSTATUS ExitStatus
    );

// begin_rev
#define RTL_CLONE_PROCESS_FLAGS_CREATE_SUSPENDED 0x00000001
#define RTL_CLONE_PROCESS_FLAGS_INHERIT_HANDLES 0x00000002
#define RTL_CLONE_PROCESS_FLAGS_NO_SYNCHRONIZE 0x00000004 // don't update synchronization objects
// end_rev

// private
/**
 * Creates a new process from the current process.
 *
 * @param ProcessFlags The path of the image to be executed.
 * @param ProcessSecurityDescriptor The security descriptor for the new process. If NULL, the process gets a default security descriptor.
 * @param ThreadSecurityDescriptor The security descriptor for the initial thread. If NULL, the thread gets a default security descriptor.
 * @param DebugPort The handle of an ALPC port for debug messages. If NULL, the process gets a default port. (WindowsErrorReportingServicePort)
 * @param ProcessInformation The new process information.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCloneUserProcess(
    _In_ ULONG ProcessFlags,
    _In_opt_ PSECURITY_DESCRIPTOR ProcessSecurityDescriptor,
    _In_opt_ PSECURITY_DESCRIPTOR ThreadSecurityDescriptor,
    _In_opt_ HANDLE DebugPort,
    _Out_ PRTL_USER_PROCESS_INFORMATION ProcessInformation
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlPrepareForProcessCloning(
    VOID
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlCompleteProcessCloning(
    _In_ LOGICAL Completed
    );

// private
NTSYSAPI
VOID
NTAPI
RtlUpdateClonedCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );

// private
NTSYSAPI
VOID
NTAPI
RtlUpdateClonedSRWLock(
    _Inout_ PRTL_SRWLOCK SRWLock,
    _In_ LOGICAL Shared // TRUE to set to shared acquire
    );

// rev RtlCloneUserProcess Flags
#define RTL_PROCESS_REFLECTION_FLAGS_CREATE_SUSPENDED 0x00000001
#define RTL_PROCESS_REFLECTION_FLAGS_INHERIT_HANDLES  0x00000002
#define RTL_PROCESS_REFLECTION_FLAGS_NO_SUSPEND       0x00000004
#define RTL_PROCESS_REFLECTION_FLAGS_NO_SYNCHRONIZE   0x00000008
#define RTL_PROCESS_REFLECTION_FLAGS_NO_CLOSE_EVENT   0x00000010

// private
typedef struct _RTLP_PROCESS_REFLECTION_REFLECTION_INFORMATION
{
    HANDLE ReflectionProcessHandle;
    HANDLE ReflectionThreadHandle;
    CLIENT_ID ReflectionClientId;
} RTLP_PROCESS_REFLECTION_REFLECTION_INFORMATION, *PRTLP_PROCESS_REFLECTION_REFLECTION_INFORMATION;

typedef RTLP_PROCESS_REFLECTION_REFLECTION_INFORMATION PROCESS_REFLECTION_INFORMATION, *PPROCESS_REFLECTION_INFORMATION;

// rev
/**
 * The RtlCreateProcessReflection function creates a lightweight copy of a process for debugging or snapshot purposes.
 *
 * \param ProcessHandle Handle to the process to reflect.
 * \param Flags Flags that control the behavior of the reflection. See RTL_PROCESS_REFLECTION_FLAGS_*.
 * \param StartRoutine Optional pointer to a routine to execute in the reflected process.
 * \param StartContext Optional pointer to context to pass to the start routine.
 * \param EventHandle Optional handle to an event to signal when the reflection is complete.
 * \param ReflectionInformation Optional pointer to a structure that receives information about the reflected process.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateProcessReflection(
    _In_ HANDLE ProcessHandle,
    _In_ ULONG Flags, // RTL_PROCESS_REFLECTION_FLAGS_*
    _In_opt_ PVOID StartRoutine,
    _In_opt_ PVOID StartContext,
    _In_opt_ HANDLE EventHandle,
    _Out_opt_ PRTLP_PROCESS_REFLECTION_REFLECTION_INFORMATION ReflectionInformation
    );

/**
 * The RtlSetProcessIsCritical function sets or clears the critical status of the current process.
 *
 * \param NewValue TRUE to mark the process as critical, FALSE to clear.
 * \param OldValue Optional pointer to receive the previous critical status.
 * \param CheckFlag If TRUE, checks for certain conditions before setting.
 * \return NTSTATUS Successful or errant status.
 * \remarks A critical process will cause a system bugcheck if terminated.
 */
NTSYSAPI
NTSTATUS
STDAPIVCALLTYPE
RtlSetProcessIsCritical(
    _In_ BOOLEAN NewValue,
    _Out_opt_ PBOOLEAN OldValue,
    _In_ BOOLEAN CheckFlag
    );

/**
 * The RtlSetThreadIsCritical function sets or clears the critical status of the current thread.
 *
 * \param NewValue TRUE to mark the thread as critical, FALSE to clear.
 * \param OldValue Optional pointer to receive the previous critical status.
 * \param CheckFlag If TRUE, checks for certain conditions before setting.
 * \return NTSTATUS Successful or errant status.
 * \remarks A critical thread will cause a system bugcheck if terminated.
 */
NTSYSAPI
NTSTATUS
STDAPIVCALLTYPE
RtlSetThreadIsCritical(
    _In_ BOOLEAN NewValue,
    _Out_opt_ PBOOLEAN OldValue,
    _In_ BOOLEAN CheckFlag
    );

// rev
/**
 * The RtlSetThreadSubProcessTag function sets the sub-process tag for the current thread.
 *
 * @param SubProcessTag Pointer to the tag value to set.
 * @return The previous sub-process tag value.
 */
NTSYSAPI
PVOID
NTAPI
RtlSetThreadSubProcessTag(
    _In_ PVOID SubProcessTag
    );

// rev
/**
 * Validates the process protection level.
 *
 * @param ProcessProtection Pointer to a PS_PROTECTION structure describing the protection.
 * @return TRUE if the protection level is valid, FALSE otherwise.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlValidProcessProtection(
    _In_ PS_PROTECTION ProcessProtection
    );

// rev
/**
 * Tests whether a source protection level can access a target protection level.
 *
 * @param Source Pointer to a PS_PROTECTION structure for the source.
 * @param Target Pointer to a PS_PROTECTION structure for the target.
 * @return TRUE if access is allowed, FALSE otherwise.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlTestProtectedAccess(
    _In_ PS_PROTECTION Source,
    _In_ PS_PROTECTION Target
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS3)
// rev
NTSYSAPI
BOOLEAN
NTAPI
RtlIsCurrentProcess( // NtCompareObjects(NtCurrentProcess(), ProcessHandle)
    _In_ HANDLE ProcessHandle
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
RtlIsCurrentThread( // NtCompareObjects(NtCurrentThread(), ThreadHandle)
    _In_ HANDLE ThreadHandle
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS3

//
// Threads
//

typedef _Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS NTAPI USER_THREAD_START_ROUTINE(
    _In_ PVOID ThreadParameter
    );
typedef USER_THREAD_START_ROUTINE* PUSER_THREAD_START_ROUTINE;

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateUserThread(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PSECURITY_DESCRIPTOR ThreadSecurityDescriptor,
    _In_ BOOLEAN CreateSuspended,
    _In_opt_ ULONG ZeroBits,
    _In_opt_ SIZE_T MaximumStackSize,
    _In_opt_ SIZE_T CommittedStackSize,
    _In_ PUSER_THREAD_START_ROUTINE StartAddress,
    _In_opt_ PVOID Parameter,
    _Out_opt_ PHANDLE ThreadHandle,
    _Out_opt_ PCLIENT_ID ClientId
    );

_Analysis_noreturn_
DECLSPEC_NORETURN
NTSYSAPI
VOID
NTAPI
RtlExitUserThread(
    _In_ NTSTATUS ExitStatus
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
RtlIsCurrentThreadAttachExempt(
    VOID
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateUserStack(
    _In_opt_ SIZE_T CommittedStackSize,
    _In_opt_ SIZE_T MaximumStackSize,
    _In_opt_ ULONG_PTR ZeroBits,
    _In_ SIZE_T PageSize,
    _In_ ULONG_PTR ReserveAlignment,
    _Out_ PINITIAL_TEB InitialTeb
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlFreeUserStack(
    _In_ PVOID AllocationBase
    );

//
// Extended thread context
//

typedef struct _CONTEXT_CHUNK
{
    LONG Offset; // Offset may be negative.
    ULONG Length;
} CONTEXT_CHUNK, *PCONTEXT_CHUNK;

typedef struct _CONTEXT_EX
{
    CONTEXT_CHUNK All;
    CONTEXT_CHUNK Legacy;
    CONTEXT_CHUNK XState;
    CONTEXT_CHUNK KernelCet;
} CONTEXT_EX, *PCONTEXT_EX;

#if defined(_AMD64_) || defined(_ARM64_) || defined(_ARM64EC_)
#define CONTEXT_ALIGN 0x10
#else
#define CONTEXT_ALIGN 0x8
#endif // _AMD64_ || _ARM64_ || _ARM64EC_

#if defined(_AMD64_)
#define CONTEXT_FRAME_LENGTH 0x4D0
#define CONTEXT_EX_PADDING   0x10
#elif defined(_ARM64_) || defined(_ARM64EC_)
#define CONTEXT_FRAME_LENGTH 0x390
#define CONTEXT_EX_PADDING   0x10
#elif defined(_M_ARM)
#define CONTEXT_FRAME_LENGTH 0x1a0
#define CONTEXT_EX_PADDING   0x8
#else
#define CONTEXT_FRAME_LENGTH 0x2CC
#define CONTEXT_EX_PADDING   0x4
#endif // _AMD64_

#define CONTEXT_ALIGNMENT(Size, Align) \
    (((ULONG_PTR)(Size) + (Align) - 1) & ~((Align) - 1))

#define CONTEXT_EX_LENGTH \
    CONTEXT_ALIGNMENT(sizeof(CONTEXT_EX), CONTEXT_ALIGN)

static_assert(CONTEXT_FRAME_LENGTH == sizeof(CONTEXT));
static_assert(CONTEXT_EX_LENGTH == 0x20);

#define RTL_CONTEXT_EX_OFFSET(ContextEx, Chunk) ((ContextEx)->Chunk.Offset)
#define RTL_CONTEXT_EX_LENGTH(ContextEx, Chunk) ((ContextEx)->Chunk.Length)
#define RTL_CONTEXT_EX_CHUNK(Base, Layout, Chunk) ((PVOID)((PUCHAR)(Base) + RTL_CONTEXT_EX_OFFSET(Layout, Chunk)))
#define RTL_CONTEXT_OFFSET(Context, Chunk) RTL_CONTEXT_EX_OFFSET((PCONTEXT_EX)((Context) + 1), Chunk)
#define RTL_CONTEXT_LENGTH(Context, Chunk) RTL_CONTEXT_EX_LENGTH((PCONTEXT_EX)((Context) + 1), Chunk)
#define RTL_CONTEXT_CHUNK(Context, Chunk) RTL_CONTEXT_EX_CHUNK((PCONTEXT_EX)((Context) + 1), (PCONTEXT_EX)((Context) + 1), Chunk)

#if defined(_M_AMD64)
// returns constant 0xf0e0d0c0a0908070 (dmex)
NTSYSAPI
ULONG64
NTAPI
RtlInitializeContext(
    _Reserved_ HANDLE Reserved,
    _Out_ PCONTEXT Context,
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID InitialPc,
    _In_opt_ PVOID InitialSp
    );
#else
// returns status of NtWriteVirtualMemory (dmex)
NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeContext(
    _In_ HANDLE ProcessHandle,
    _Out_ PCONTEXT Context,
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID InitialPc,
    _In_opt_ PVOID InitialSp
    );
#endif // _M_AMD64

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeExtendedContext(
    _Out_ PCONTEXT Context,
    _In_ ULONG ContextFlags,
    _Out_ PCONTEXT_EX* ContextEx
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeExtendedContext2(
    _Out_ PCONTEXT Context,
    _In_ ULONG ContextFlags,
    _Out_ PCONTEXT_EX* ContextEx,
    _In_ ULONG64 EnabledExtendedFeatures // RtlGetEnabledExtendedFeatures(-1)
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCopyContext(
    _Inout_ PCONTEXT Context,
    _In_ ULONG ContextFlags,
    _Out_ PCONTEXT Source
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCopyExtendedContext(
    _Out_ PCONTEXT_EX Destination,
    _In_ ULONG ContextFlags,
    _In_ PCONTEXT_EX Source
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetExtendedContextLength(
    _In_ ULONG ContextFlags,
    _Out_ PULONG ContextLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetExtendedContextLength2(
    _In_ ULONG ContextFlags,
    _Out_ PULONG ContextLength,
    _In_ ULONG64 EnabledExtendedFeatures // RtlGetEnabledExtendedFeatures(-1)
    );

NTSYSAPI
ULONG64
NTAPI
RtlGetExtendedFeaturesMask(
    _In_ PCONTEXT_EX ContextEx
    );

NTSYSAPI
PVOID
NTAPI
RtlLocateExtendedFeature(
    _In_ PCONTEXT_EX ContextEx,
    _In_ ULONG FeatureId,
    _Out_opt_ PULONG Length
    );

NTSYSAPI
PCONTEXT
NTAPI
RtlLocateLegacyContext(
    _In_ PCONTEXT_EX ContextEx,
    _Out_opt_ PULONG Length
    );

NTSYSAPI
VOID
NTAPI
RtlSetExtendedFeaturesMask(
    _In_ PCONTEXT_EX ContextEx,
    _In_ ULONG64 FeatureMask
    );

#if defined(_WIN64)
#if defined(_PHLIB_)
FORCEINLINE
NTSTATUS
NTAPI_INLINE
RtlWow64GetThreadContext(
    _In_ HANDLE ThreadHandle,
    _Inout_ PWOW64_CONTEXT ThreadContext
    )
{
    return NtQueryInformationThread(
        ThreadHandle,
        ThreadWow64Context,
        ThreadContext,
        sizeof(WOW64_CONTEXT),
        NULL
        );
}
#else
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlWow64GetThreadContext(
    _In_ HANDLE ThreadHandle,
    _Inout_ PWOW64_CONTEXT ThreadContext
    );
#endif // _PHLIB_
#endif // _WIN64

#if defined(_WIN64)
#if defined(_PHLIB_)
FORCEINLINE
NTSTATUS
NTAPI_INLINE
RtlWow64SetThreadContext(
    _In_ HANDLE ThreadHandle,
    _In_ PWOW64_CONTEXT ThreadContext
    )
{
    return NtSetInformationThread(
        ThreadHandle,
        ThreadWow64Context,
        ThreadContext,
        sizeof(WOW64_CONTEXT)
        );
}
#else
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlWow64SetThreadContext(
    _In_ HANDLE ThreadHandle,
    _In_ PWOW64_CONTEXT ThreadContext
    );
#endif // _PHLIB_
#endif // _WIN64

NTSYSAPI
NTSTATUS
NTAPI
RtlRemoteCall(
    _In_ HANDLE ProcessHandle,
    _In_ HANDLE ThreadHandle,
    _In_ PVOID CallSite,
    _In_ ULONG ArgumentCount,
    _In_opt_ PULONG_PTR Arguments,
    _In_ BOOLEAN PassContext,
    _In_ BOOLEAN AlreadySuspended
    );

//
// Vectored Exception Handlers
//

/**
 * Registers a vectored exception handler.
 *
 * @param First If this parameter is TRUE, the handler is the first handler in the list.
 * @param Handler A pointer to the vectored exception handler to be called.
 * @return A handle to the vectored exception handler.
 * @see https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-addvectoredexceptionhandler
 */
NTSYSAPI
PVOID
NTAPI
RtlAddVectoredExceptionHandler(
    _In_ ULONG First,
    _In_ PVECTORED_EXCEPTION_HANDLER Handler
    );

/**
 * Removes a vectored exception handler.
 *
 * @param Handle A handle to the vectored exception handler to remove.
 * @return The function returns 0 if the handler is removed, or -1 if the handler is not found.
 * @see https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-removevectoredexceptionhandler
 */
NTSYSAPI
ULONG
NTAPI
RtlRemoveVectoredExceptionHandler(
    _In_ PVOID Handle
    );

/**
 * Registers a vectored continue handler.
 *
 * @param First If this parameter is TRUE, the handler is the first handler in the list.
 * @param Handler A pointer to the vectored exception handler to be called.
 * @return A handle to the vectored continue handler.
 * @see https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-addvectoredcontinuehandler
 */
NTSYSAPI
PVOID
NTAPI
RtlAddVectoredContinueHandler(
    _In_ ULONG First,
    _In_ PVECTORED_EXCEPTION_HANDLER Handler
    );

/**
 * Removes a vectored continue handler.
 *
 * @param Handle A handle to the vectored continue handler to remove.
 * @return The function returns 0 if the handler is removed, or -1 if the handler is not found.
 * @see https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-removevectoredcontinuehandler
 */
NTSYSAPI
ULONG
NTAPI
RtlRemoveVectoredContinueHandler(
    _In_ PVOID Handle
    );

//
// Runtime exception handling
//

typedef _Function_class_(RTLP_UNHANDLED_EXCEPTION_FILTER)
ULONG NTAPI RTLP_UNHANDLED_EXCEPTION_FILTER(
    _In_ PEXCEPTION_POINTERS ExceptionInfo
    );
typedef RTLP_UNHANDLED_EXCEPTION_FILTER* PRTLP_UNHANDLED_EXCEPTION_FILTER;

NTSYSAPI
VOID
NTAPI
RtlSetUnhandledExceptionFilter(
    _In_ PRTLP_UNHANDLED_EXCEPTION_FILTER UnhandledExceptionFilter
    );

// rev
NTSYSAPI
LONG
NTAPI
RtlUnhandledExceptionFilter(
    _In_ PEXCEPTION_POINTERS ExceptionPointers
    );

// rev
NTSYSAPI
LONG
NTAPI
RtlUnhandledExceptionFilter2(
    _In_ PEXCEPTION_POINTERS ExceptionPointers,
    _In_ ULONG Flags
    );

// rev
NTSYSAPI
LONG
NTAPI
RtlKnownExceptionFilter(
    _In_ PEXCEPTION_POINTERS ExceptionPointers
    );

#ifdef _WIN64

// private
typedef enum _FUNCTION_TABLE_TYPE
{
    RF_SORTED,
    RF_UNSORTED,
    RF_CALLBACK,
    RF_KERNEL_DYNAMIC
} FUNCTION_TABLE_TYPE;

// private
typedef struct _DYNAMIC_FUNCTION_TABLE
{
    LIST_ENTRY ListEntry;
    PRUNTIME_FUNCTION FunctionTable;
    LARGE_INTEGER TimeStamp;
    ULONG64 MinimumAddress;
    ULONG64 MaximumAddress;
    ULONG64 BaseAddress;
    PGET_RUNTIME_FUNCTION_CALLBACK Callback;
    PVOID Context;
    PWSTR OutOfProcessCallbackDll;
    FUNCTION_TABLE_TYPE Type;
    ULONG EntryCount;
    RTL_BALANCED_NODE TreeNodeMin;
    RTL_BALANCED_NODE TreeNodeMax;
} DYNAMIC_FUNCTION_TABLE, *PDYNAMIC_FUNCTION_TABLE;

// rev
NTSYSAPI
PLIST_ENTRY
NTAPI
RtlGetFunctionTableListHead(
    VOID
    );

#endif // _WIN64

//
// Linked lists
//

NTSYSAPI
VOID
NTAPI
RtlInitializeSListHead(
    _Out_ PSLIST_HEADER ListHead
    );

_Must_inspect_result_
NTSYSAPI
PSLIST_ENTRY
NTAPI
RtlFirstEntrySList(
    _In_ const SLIST_HEADER *ListHead
    );

NTSYSAPI
PSLIST_ENTRY
NTAPI
RtlInterlockedPopEntrySList(
    _Inout_ PSLIST_HEADER ListHead
    );

NTSYSAPI
PSLIST_ENTRY
NTAPI
RtlInterlockedPushEntrySList(
    _Inout_ PSLIST_HEADER ListHead,
    _Inout_ __drv_aliasesMem PSLIST_ENTRY ListEntry
    );

NTSYSAPI
PSLIST_ENTRY
NTAPI
RtlInterlockedPushListSListEx(
    _Inout_ PSLIST_HEADER ListHead,
    _Inout_ __drv_aliasesMem PSLIST_ENTRY List,
    _Inout_ PSLIST_ENTRY ListEnd,
    _In_ ULONG Count
    );

NTSYSAPI
PSLIST_ENTRY
NTAPI
RtlInterlockedFlushSList(
    _Inout_ PSLIST_HEADER ListHead
    );

NTSYSAPI
USHORT
NTAPI
RtlQueryDepthSList(
    _In_ PSLIST_HEADER ListHead
    );

//
// Activation Contexts
//

#define INVALID_ACTIVATION_CONTEXT ((HANDLE)(LONG_PTR)-1)
#define ACTCTX_PROCESS_DEFAULT ((HANDLE)(LONG_PTR)0)
#define ACTCTX_EMPTY ((HANDLE)(LONG_PTR)-3)
#define ACTCTX_SYSTEM_DEFAULT ((HANDLE)(LONG_PTR)-4)
#define IS_SPECIAL_ACTCTX(x) (((((LONG_PTR)(x)) - 1) | 7) == -1)

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlGetActiveActivationContext(
    _Out_ PACTIVATION_CONTEXT ActivationContext
    );

// private
NTSYSAPI
VOID
NTAPI
RtlAddRefActivationContext(
    _In_ PACTIVATION_CONTEXT ActivationContext
    );

// private
NTSYSAPI
VOID
NTAPI
RtlReleaseActivationContext(
    _In_ PACTIVATION_CONTEXT ActivationContext
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlZombifyActivationContext(
    _In_ PACTIVATION_CONTEXT ActivationContext
    );

// private
NTSYSAPI
BOOLEAN
NTAPI
RtlIsActivationContextActive(
    _In_ PACTIVATION_CONTEXT ActivationContext
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlActivateActivationContext(
    _Reserved_ ULONG Flags,
    _In_ PACTIVATION_CONTEXT ActivationContext,
    _Out_ PULONG_PTR Cookie
    );

#define RTL_ACTIVATE_ACTIVATION_CONTEXT_EX_FLAG_RELEASE_ON_STACK_DEALLOCATION 0x00000001

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlActivateActivationContextEx(
    _In_ ULONG Flags,
    _In_ PTEB Teb,
    _In_ PACTIVATION_CONTEXT ActivationContext,
    _Out_ PULONG_PTR Cookie
    );

#define RTL_DEACTIVATE_ACTIVATION_CONTEXT_FLAG_FORCE_EARLY_DEACTIVATION 0x00000001

// private
NTSYSAPI
VOID
NTAPI
RtlDeactivateActivationContext(
    _In_ ULONG Flags,
    _In_ ULONG_PTR Cookie
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateActivationContext(
    _Reserved_ ULONG Flags,
    _In_ PACTIVATION_CONTEXT_DATA ActivationContextData,
    _In_opt_ ULONG ExtraBytes,
    _In_opt_ PACTIVATION_CONTEXT_NOTIFY_ROUTINE NotificationRoutine,
    _In_opt_ PVOID NotificationContext,
    _Out_ PACTIVATION_CONTEXT *ActivationContext
    );

#define FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_ACTIVATION_CONTEXT 0x00000001
#define FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_FLAGS 0x00000002
#define FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_ASSEMBLY_METADATA 0x00000004

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlFindActivationContextSectionString(
    _In_ ULONG Flags,
    _In_opt_ PGUID ExtensionGuid,
    _In_ ULONG SectionId, // ACTIVATION_CONTEXT_SECTION_*
    _In_ PCUNICODE_STRING StringToFind,
    _Inout_ PACTCTX_SECTION_KEYED_DATA ReturnedData
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlFindActivationContextSectionGuid(
    _In_ ULONG Flags,
    _In_opt_ PGUID ExtensionGuid,
    _In_ ULONG SectionId, // ACTIVATION_CONTEXT_SECTION_*
    _In_ PGUID GuidToFind,
    _Inout_ PACTCTX_SECTION_KEYED_DATA ReturnedData
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryActivationContextApplicationSettings(
    _Reserved_ ULONG Flags,
    _In_ PACTIVATION_CONTEXT ActivationContext,
    _In_ PCWSTR SettingsNameSpace,
    _In_ PCWSTR SettingName,
    _Out_writes_bytes_(BufferLength) PWSTR Buffer,
    _In_ SIZE_T BufferLength,
    _Out_opt_ PSIZE_T RequiredLength
    );

// ACTIVATION_CONTEXT_INFO_CLASS
//   ActivationContextBasicInformation                      // q: ACTIVATION_CONTEXT_BASIC_INFORMATION
//   ActivationContextDetailedInformation                   // q: ACTIVATION_CONTEXT_DETAILED_INFORMATION
//   AssemblyDetailedInformationInActivationContext         // q: ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION
//   FileInformationInAssemblyOfAssemblyInActivationContext // q: ASSEMBLY_FILE_DETAILED_INFORMATION
//   RunlevelInformationInActivationContext                 // q: ACTIVATION_CONTEXT_RUN_LEVEL_INFORMATION
//   CompatibilityInformationInActivationContext            // q: ACTIVATION_CONTEXT_COMPATIBILITY_INFORMATION[_LEGACY]
//   ActivationContextManifestResourceName                  // q: ULONG

#define RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_USE_ACTIVE_ACTIVATION_CONTEXT 0x00000001
#define RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_ACTIVATION_CONTEXT_IS_MODULE 0x00000002
#define RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_ACTIVATION_CONTEXT_IS_ADDRESS 0x00000004
#define RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_NO_ADDREF 0x80000000

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryInformationActivationContext(
    _In_ ULONG Flags,
    _In_opt_ PACTIVATION_CONTEXT ActivationContext,
    _In_opt_ PACTIVATION_CONTEXT_QUERY_INDEX SubInstanceIndex,
    _In_ ACTIVATION_CONTEXT_INFO_CLASS ActivationContextInformationClass,
    _Out_writes_bytes_(ActivationContextInformationLength) PVOID ActivationContextInformation,
    _In_ SIZE_T ActivationContextInformationLength,
    _Out_opt_ PSIZE_T ReturnLength
    );

#if defined(_PHLIB_)
// private
FORCEINLINE
NTSTATUS
NTAPI_INLINE
RtlQueryInformationActiveActivationContext(
    _In_ ACTIVATION_CONTEXT_INFO_CLASS ActivationContextInformationClass,
    _Out_writes_bytes_(ActivationContextInformationLength) PVOID ActivationContextInformation,
    _In_ SIZE_T ActivationContextInformationLength,
    _Out_opt_ PSIZE_T ReturnLength
    )
{
    return RtlQueryInformationActivationContext(
        RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_USE_ACTIVE_ACTIVATION_CONTEXT,
        NULL,
        NULL,
        ActivationContextInformationClass,
        ActivationContextInformation,
        ActivationContextInformationLength,
        ReturnLength
        );
}
#else
// private
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryInformationActiveActivationContext(
    _In_ ACTIVATION_CONTEXT_INFO_CLASS ActivationContextInformationClass,
    _Out_writes_bytes_(ActivationContextInformationLength) PVOID ActivationContextInformation,
    _In_ SIZE_T ActivationContextInformationLength,
    _Out_opt_ PSIZE_T ReturnLength
    );
#endif // _PHLIB_

//
// Images
//

NTSYSAPI
PIMAGE_NT_HEADERS
NTAPI
RtlImageNtHeader(
    _In_ PVOID BaseOfImage
    );

#define RTL_IMAGE_NT_HEADER_EX_FLAG_NO_RANGE_CHECK 0x00000001

NTSYSAPI
NTSTATUS
NTAPI
RtlImageNtHeaderEx(
    _In_ ULONG Flags,
    _In_ PVOID BaseOfImage,
    _In_ ULONG64 Size,
    _Out_ PIMAGE_NT_HEADERS *OutHeaders
    );

NTSYSAPI
PVOID
NTAPI
RtlAddressInSectionTable(
    _In_ PIMAGE_NT_HEADERS NtHeaders,
    _In_ PVOID BaseOfImage,
    _In_ ULONG VirtualAddress
    );

NTSYSAPI
PIMAGE_SECTION_HEADER
NTAPI
RtlSectionTableFromVirtualAddress(
    _In_ PIMAGE_NT_HEADERS NtHeaders,
    _In_ PVOID BaseOfImage,
    _In_ ULONG VirtualAddress
    );

NTSYSAPI
PVOID
NTAPI
RtlImageDirectoryEntryToData(
    _In_ PVOID BaseOfImage,
    _In_ BOOLEAN MappedAsImage,
    _In_ USHORT DirectoryEntry,
    _Out_ PULONG Size
    );

NTSYSAPI
PIMAGE_SECTION_HEADER
NTAPI
RtlImageRvaToSection(
    _In_ PIMAGE_NT_HEADERS NtHeaders,
    _In_ PVOID BaseOfImage,
    _In_ ULONG Rva
    );

NTSYSAPI
PVOID
NTAPI
RtlImageRvaToVa(
    _In_ PIMAGE_NT_HEADERS NtHeaders,
    _In_ PVOID BaseOfImage,
    _In_ ULONG Rva,
    _Out_opt_ PIMAGE_SECTION_HEADER *LastRvaSection
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)

// rev
NTSYSAPI
PVOID
NTAPI
RtlFindExportedRoutineByName(
    _In_ PVOID BaseOfImage,
    _In_z_ PCSTR RoutineName
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlGuardCheckLongJumpTarget(
    _In_ PVOID PcValue,
    _In_ BOOL IsFastFail,
    _Out_ PBOOL IsLongJumpTarget
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS1

#if (PHNT_VERSION >= PHNT_WINDOWS_11_22H2)
NTSYSAPI
VOID
NTAPI
RtlValidateUserCallTarget(
    _In_ PVOID Address,
    _Out_ PULONG Flags
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11_22H2

//
// Memory
//

_Check_return_
NTSYSAPI
SIZE_T
NTAPI
RtlCompareMemory(
    _In_ const VOID* Source1,
    _In_ const VOID* Source2,
    _In_ SIZE_T Length
    );

_Must_inspect_result_
NTSYSAPI
SIZE_T
NTAPI
RtlCompareMemoryUlong(
    _In_reads_bytes_(Length) PVOID Source,
    _In_ SIZE_T Length,
    _In_ ULONG Pattern
    );

#if defined(_M_AMD64)
FORCEINLINE
VOID
NTAPI_INLINE
RtlFillMemoryUlong(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_ SIZE_T Length,
    _In_ ULONG Pattern
    )
{
    PULONG Address = (PULONG)Destination;

    //
    // If the number of DWORDs is not zero, then fill the specified buffer
    // with the specified pattern.
    //

    if ((Length /= 4) != 0) {

        //
        // If the destination is not quadword aligned (ignoring low bits),
        // then align the destination by storing one DWORD.
        //

        if (((ULONG64)Address & 4) != 0) {
            *Address = Pattern;
            if ((Length -= 1) == 0) {
                return;
            }

            Address += 1;
        }

        //
        // If the number of QWORDs is not zero, then fill the destination
        // buffer a QWORD at a time.
        //

         __stosq((PULONG64)(Address),
                 Pattern | ((ULONG64)Pattern << 32),
                 Length / 2);

        if ((Length & 1) != 0) {
            Address[Length - 1] = Pattern;
        }
    }

    return;
}
#else
NTSYSAPI
VOID
NTAPI
RtlFillMemoryUlong(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_ SIZE_T Length,
    _In_ ULONG Pattern
    );
#endif // _M_AMD64

#if defined(_M_AMD64)

#define RtlFillMemoryUlonglong(Destination, Length, Pattern) \
    __stosq((PULONG64)(Destination), Pattern, (Length) / 8)

#else
NTSYSAPI
VOID
NTAPI
RtlFillMemoryUlonglong(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_ SIZE_T Length,
    _In_ ULONGLONG Pattern
    );
#endif // _M_AMD64

#if (PHNT_VERSION >= PHNT_WINDOWS_10_19H2)
NTSYSAPI
BOOLEAN
NTAPI
RtlIsZeroMemory(
    _In_ PVOID Buffer,
    _In_ SIZE_T Length
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_19H2

NTSYSAPI
ULONG
NTAPI
RtlCrc32(
    _In_reads_bytes_(Size) const void *Buffer,
    _In_ size_t Size,
    _In_ ULONG InitialCrc
    );

NTSYSAPI
ULONGLONG
NTAPI
RtlCrc64(
    _In_reads_bytes_(Size) const void *Buffer,
    _In_ size_t Size,
    _In_ ULONGLONG InitialCrc
    );

// RTL_SYSTEM_GLOBAL_DATA_ID
#define GlobalDataIdUnknown 0
#define GlobalDataIdRngSeedVersion 1                // KUSER_SHARED_DATA->RngSeedVersion
#define GlobalDataIdInterruptTime 2                 // KUSER_SHARED_DATA->InterruptTime
#define GlobalDataIdTimeZoneBias 3                  // KUSER_SHARED_DATA->TimeZoneBias
#define GlobalDataIdImageNumberLow 4                // KUSER_SHARED_DATA->ImageNumberLow
#define GlobalDataIdImageNumberHigh 5               // KUSER_SHARED_DATA->ImageNumberHigh
#define GlobalDataIdTimeZoneId 6                    // KUSER_SHARED_DATA->TimeZoneId
#define GlobalDataIdNtMajorVersion 7                // KUSER_SHARED_DATA->NtMajorVersion
#define GlobalDataIdNtMinorVersion 8                // KUSER_SHARED_DATA->NtMinorVersion
#define GlobalDataIdSystemExpirationDate 9          // KUSER_SHARED_DATA->SystemExpirationDate
#define GlobalDataIdKdDebuggerEnabled 10            // KUSER_SHARED_DATA->KdDebuggerEnabled
#define GlobalDataIdCyclesPerYield 11               // KUSER_SHARED_DATA->CyclesPerYield
#define GlobalDataIdSafeBootMode 12                 // KUSER_SHARED_DATA->SafeBootMode
#define GlobalDataIdLastSystemRITEventTickCount 13  // KUSER_SHARED_DATA->LastSystemRITEventTickCount
#define GlobalDataIdConsoleSharedDataFlags 14       // KUSER_SHARED_DATA->ConsoleSharedDataFlags
#define GlobalDataIdNtSystemRootDrive 15            // KUSER_SHARED_DATA->NtSystemRoot // RtlGetNtSystemRoot 
#define GlobalDataIdQpcBypassEnabled 16             // KUSER_SHARED_DATA->QpcBypassEnabled
#define GlobalDataIdQpcData 17                      // KUSER_SHARED_DATA->QpcData
#define GlobalDataIdQpcBias 18                      // KUSER_SHARED_DATA->QpcBias

#if !defined(NTDDI_WIN10_FE) || (NTDDI_VERSION < NTDDI_WIN10_FE)
typedef ULONG RTL_SYSTEM_GLOBAL_DATA_ID;
#endif // !defined(NTDDI_WIN10_FE) || (NTDDI_VERSION < NTDDI_WIN10_FE)

NTSYSAPI
ULONG
NTAPI
RtlGetSystemGlobalData(
    _In_ RTL_SYSTEM_GLOBAL_DATA_ID DataId,
    _Inout_ PVOID Buffer,
    _In_ ULONG Size
    );

NTSYSAPI
ULONG
NTAPI
RtlSetSystemGlobalData(
    _In_ RTL_SYSTEM_GLOBAL_DATA_ID DataId,
    _In_ PVOID Buffer,
    _In_ ULONG Size
    );

//
// Environment
//

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateEnvironment(
    _In_ BOOLEAN CloneCurrentEnvironment,
    _Out_ PVOID *Environment
    );

// begin_rev
#define RTL_CREATE_ENVIRONMENT_TRANSLATE 0x1 // translate from multi-byte to Unicode
#define RTL_CREATE_ENVIRONMENT_TRANSLATE_FROM_OEM 0x2 // translate from OEM to Unicode (Translate flag must also be set)
#define RTL_CREATE_ENVIRONMENT_EMPTY 0x4 // create empty environment block
// end_rev

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateEnvironmentEx(
    _In_opt_ PVOID SourceEnvironment,
    _Out_ PVOID *Environment,
    _In_ ULONG Flags
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyEnvironment(
    _In_ _Post_invalid_ PVOID Environment
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetCurrentEnvironment(
    _In_ PVOID Environment,
    _Out_opt_ PVOID *PreviousEnvironment
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlSetEnvironmentVar(
    _Inout_opt_ PVOID *Environment,
    _In_reads_(NameLength) PCWSTR Name,
    _In_ SIZE_T NameLength,
    _In_reads_(ValueLength) PCWSTR Value,
    _In_opt_ SIZE_T ValueLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetEnvironmentVariable(
    _Inout_opt_ PVOID *Environment,
    _In_ PCUNICODE_STRING Name,
    _In_opt_ PCUNICODE_STRING Value
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryEnvironmentVariable(
    _In_opt_ PVOID Environment,
    _In_reads_(NameLength) PCWSTR Name,
    _In_ SIZE_T NameLength,
    _Out_writes_opt_(ValueLength) PWSTR Value,
    _In_opt_ SIZE_T ValueLength,
    _Out_ PSIZE_T ReturnLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryEnvironmentVariable_U(
    _In_opt_ PVOID Environment,
    _In_ PCUNICODE_STRING Name,
    _Inout_ PUNICODE_STRING Value
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlExpandEnvironmentStrings(
    _In_opt_ PVOID Environment,
    _In_reads_(SourceLength) PCWSTR Source,
    _In_ SIZE_T SourceLength,
    _Out_writes_(DestinationLength) PWSTR Destination,
    _In_ SIZE_T DestinationLength,
    _Out_opt_ PSIZE_T ReturnLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlExpandEnvironmentStrings_U(
    _In_opt_ PVOID Environment,
    _In_ PCUNICODE_STRING Source,
    _Inout_ PUNICODE_STRING Destination,
    _Out_opt_ PULONG ReturnedLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetEnvironmentStrings(
    _In_ PCWSTR NewEnvironment,
    _In_ SIZE_T NewEnvironmentSize
    );

//
// Directory and path support
//

typedef struct _RTLP_CURDIR_REF
{
    LONG ReferenceCount;
    HANDLE DirectoryHandle;
} RTLP_CURDIR_REF, *PRTLP_CURDIR_REF;

typedef struct _RTL_RELATIVE_NAME_U
{
    UNICODE_STRING RelativeName;
    HANDLE ContainingDirectory;
    PRTLP_CURDIR_REF CurDirRef;
} RTL_RELATIVE_NAME_U, *PRTL_RELATIVE_NAME_U;

typedef enum _RTL_PATH_TYPE
{
    RtlPathTypeUnknown,
    RtlPathTypeUncAbsolute,     // "\\\\server\\share\\folder\\file.txt
    RtlPathTypeDriveAbsolute,   // "C:\\folder\\file.txt"
    RtlPathTypeDriveRelative,   // "C:folder\\file.txt"
    RtlPathTypeRooted,          // "\\folder\\file.txt"
    RtlPathTypeRelative,        // "folder\\file.txt"
    RtlPathTypeLocalDevice,     // "\\\\.\\PhysicalDrive0"
    RtlPathTypeRootLocalDevice  // "\\\\?\\C:\\folder\\file.txt"
} RTL_PATH_TYPE;

#if !defined(PHNT_INLINE_SEPERATOR_STRINGS)

#define RtlNtdllName L"ntdll.dll"
#define RtlDosPathSeperatorsString ((CONST UNICODE_STRING)RTL_CONSTANT_STRING(L"\\/"))
#define RtlAlternateDosPathSeperatorString ((CONST UNICODE_STRING)RTL_CONSTANT_STRING(L"/"))
#define RtlNtPathSeperatorString ((CONST UNICODE_STRING)RTL_CONSTANT_STRING(L"\\"))

#define RtlDosDevicesPrefix ((CONST UNICODE_STRING)RTL_CONSTANT_STRING(L"\\??\\"))
#define RtlDosDevicesUncPrefix ((CONST UNICODE_STRING)RTL_CONSTANT_STRING(L"\\??\\UNC\\"))
#define RtlSlashSlashDot ((CONST UNICODE_STRING)RTL_CONSTANT_STRING(L"\\\\.\\"))
#define RtlNullString ((CONST UNICODE_STRING)RTL_CONSTANT_STRING(L""))
#define RtlWin32NtRootSlash ((CONST UNICODE_STRING)RTL_CONSTANT_STRING(L"\\\\?\\"))
#define RtlWin32NtRoot ((CONST UNICODE_STRING)RTL_CONSTANT_STRING(L"\\\\?"))
#define RtlWin32NtUncRoot ((CONST UNICODE_STRING)RTL_CONSTANT_STRING(L"\\\\?\\UNC"))
#define RtlWin32NtUncRootSlash ((CONST UNICODE_STRING)RTL_CONSTANT_STRING(L"\\\\?\\UNC\\"))
#define RtlDefaultExtension ((CONST UNICODE_STRING)RTL_CONSTANT_STRING(L".DLL"))

#else

// Data exports (ntdll.lib/ntdllp.lib)

NTSYSAPI PCWSTR RtlNtdllName;
NTSYSAPI UNICODE_STRING RtlDosPathSeperatorsString;
NTSYSAPI UNICODE_STRING RtlAlternateDosPathSeperatorString;
NTSYSAPI UNICODE_STRING RtlNtPathSeperatorString;

#endif // PHNT_INLINE_SEPERATOR_STRINGS

//
// Path functions
//

/**
 * The RtlDetermineDosPathNameType_U routine determines the type of Dos or Win32 path type for the specified filename.
 *
 * \param DosFileName A pointer to the buffer that contains the Dos or Win32 filename.
 * \return The return value specifies the path type for the specified file.
 */
NTSYSAPI
RTL_PATH_TYPE
NTAPI
RtlDetermineDosPathNameType_U(
    _In_ PCWSTR DosFileName
    );

/**
 * The RtlIsDosDeviceName_U routine examines the Dos format file name and determines if it is a Dos device name.
 *
 * \param DosFileName A pointer to the buffer that contains the DOS or Win32 filename.
 * \return A nonzero value when the Dos file name is the name of a Dos device. The high order 16 bits is the offset
 * in the input buffer where the dos device name beings and the low order 16 bits is the length of the device name (excluding any optional trailing colon).
 * Otherwise, A zero value when the Dos file name is not the name of a Dos device.
 * \sa https://learn.microsoft.com/en-us/windows/win32/devnotes/rtlisdosdevicename_u
 */
NTSYSAPI
ULONG
NTAPI
RtlIsDosDeviceName_U(
    _In_ PCWSTR DosFileName
    );

/**
 * The RtlGetFullPathName_U routine retrieves the full path and file name of the specified file.
 *
 * \param FileName A pointer to the buffer that contains the relative filename.
 * \param BufferLength The length of the buffer for the file path string, in WCHARs. The buffer length must include room for a terminating null character.
 * \param Buffer A pointer to the buffer that receives the file path string.
 * \param FilePart A pointer to a buffer that receives the address (within Buffer) of the final file name component in the path.
 * \return If the function succeeds, the return value specifies the number of characters that are written to the buffer, not including the terminating null character.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfullpathnamea
 */
NTSYSAPI
ULONG
NTAPI
RtlGetFullPathName_U(
    _In_ PCWSTR FileName,
    _In_ ULONG BufferLength,
    _Out_writes_bytes_(BufferLength) PWSTR Buffer,
    _Out_opt_ PWSTR *FilePart
    );

// rev
/**
 * The RtlGetFullPathName_UEx routine retrieves the full path and file name of the specified file.
 *
 * \param FileName A pointer to the buffer that contains the relative filename.
 * \param BufferLength The length of the buffer for the file path string, in WCHARs. The buffer length must include room for a terminating null character.
 * \param Buffer A pointer to the buffer that receives the file path string.
 * \param FilePart A pointer to a buffer that receives the address (within Buffer) of the final file name component in the path.
 * \param BytesRequired If the function succeeds, the return value specifies the number of characters that are written to the buffer, not including the terminating null character.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getfullpathnamea
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetFullPathName_UEx(
    _In_ PCWSTR FileName,
    _In_ ULONG BufferLength,
    _Out_writes_bytes_(BufferLength) PWSTR Buffer,
    _Out_opt_ PWSTR *FilePart,
    _Out_opt_ ULONG *BytesRequired
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetFullPathName_UstrEx(
    _In_ PCUNICODE_STRING FileName,
    _Inout_ PUNICODE_STRING StaticString,
    _Out_opt_ PUNICODE_STRING DynamicString,
    _Out_opt_ PUNICODE_STRING *StringUsed,
    _Out_opt_ SIZE_T *FilePartPrefixCch,
    _Out_opt_ PBOOLEAN NameInvalid,
    _Out_ RTL_PATH_TYPE *InputPathType,
    _Out_opt_ SIZE_T *BytesRequired
    );

/**
 * The RtlGetCurrentDirectory_U routine retrieves the current directory for the current process.
 *
 * \param BufferLength The length of the buffer for the current directory string, in WCHARs. The buffer length must include room for a terminating null character.
 * \param Buffer A pointer to the buffer that receives the current directory string.
 * \return If the function succeeds, the return value specifies the number of characters that are written to the buffer, not including the terminating null character.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getcurrentdirectory
 */
NTSYSAPI
ULONG
NTAPI
RtlGetCurrentDirectory_U(
    _In_ ULONG BufferLength,
    _Out_writes_bytes_(BufferLength) PWSTR Buffer
    );

/**
 * The RtlSetCurrentDirectory_U routine changes the current directory for the current process.
 *
 * \param PathName The path to the new current directory.
 * This parameter may specify a relative path or a full path. In either case, the full path of the specified directory is calculated and stored as the current directory.
 * \return If the function succeeds, the return value specifies the number of characters that are written to the buffer, not including the terminating null character.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getcurrentdirectory
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetCurrentDirectory_U(
    _In_ PCUNICODE_STRING PathName
    );

NTSYSAPI
ULONG
NTAPI
RtlGetLongestNtPathLength(
    VOID
    );

// rev
typedef struct _RTL_BUFFER
{
    PUCHAR Buffer;
    PUCHAR StaticBuffer;
    SIZE_T Size;
    SIZE_T StaticSize;
} RTL_BUFFER, *PRTL_BUFFER;

//FORCEINLINE
//VOID
//RtlInitBuffer(
//    _Inout_ PRTL_BUFFER Buffer,
//    _In_ PUCHAR Data,
//    _In_ ULONG DataSize
//    )
//{
//    Buffer->Buffer = Buffer->StaticBuffer = Data;
//    Buffer->Size = Buffer->StaticSize = DataSize;
//}
//
//FORCEINLINE
//VOID
//RtlFreeBuffer(
//    _Inout_ PRTL_BUFFER Buffer
//    )
//{
//    if (Buffer->Buffer != Buffer->StaticBuffer && Buffer->Buffer)
//        RtlFreeHeap(RtlProcessHeap(), 0, Buffer->Buffer);
//    Buffer->Buffer = Buffer->StaticBuffer;
//    Buffer->Size = Buffer->StaticSize;
//}

// rev
typedef struct _RTL_UNICODE_STRING_BUFFER
{
    UNICODE_STRING String;
    RTL_BUFFER ByteBuffer;
    UCHAR MinimumStaticBufferForTerminalNul[2];
} RTL_UNICODE_STRING_BUFFER, *PRTL_UNICODE_STRING_BUFFER;

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlNtPathNameToDosPathName(
    _Reserved_ ULONG Flags,
    _Inout_ PRTL_UNICODE_STRING_BUFFER Path,
    _Out_opt_ PULONG Disposition, // RtlDetermineDosPathNameType_U
    _Out_opt_ PWSTR* FilePart
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlDosPathNameToNtPathName_U(
    _In_ PCWSTR DosFileName,
    _Out_ PUNICODE_STRING NtFileName,
    _Out_opt_ PWSTR *FilePart,
    _Out_opt_ PRTL_RELATIVE_NAME_U RelativeName
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDosPathNameToNtPathName_U_WithStatus(
    _In_ PCWSTR DosFileName,
    _Out_ PUNICODE_STRING NtFileName,
    _Out_opt_ PWSTR *FilePart,
    _Out_opt_ PRTL_RELATIVE_NAME_U RelativeName
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS3)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlDosLongPathNameToNtPathName_U_WithStatus(
    _In_ PCWSTR DosFileName,
    _Out_ PUNICODE_STRING NtFileName,
    _Out_opt_ PWSTR *FilePart,
    _Out_opt_ PRTL_RELATIVE_NAME_U RelativeName
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS3

NTSYSAPI
BOOLEAN
NTAPI
RtlDosPathNameToRelativeNtPathName_U(
    _In_ PCWSTR DosFileName,
    _Out_ PUNICODE_STRING NtFileName,
    _Out_opt_ PWSTR *FilePart,
    _Out_opt_ PRTL_RELATIVE_NAME_U RelativeName
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDosPathNameToRelativeNtPathName_U_WithStatus(
    _In_ PCWSTR DosFileName,
    _Out_ PUNICODE_STRING NtFileName,
    _Out_opt_ PWSTR *FilePart,
    _Out_opt_ PRTL_RELATIVE_NAME_U RelativeName
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS3)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlDosLongPathNameToRelativeNtPathName_U_WithStatus(
    _In_ PCWSTR DosFileName,
    _Out_ PUNICODE_STRING NtFileName,
    _Out_opt_ PWSTR *FilePart,
    _Out_opt_ PRTL_RELATIVE_NAME_U RelativeName
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS3

NTSYSAPI
VOID
NTAPI
RtlReleaseRelativeName(
    _Inout_ PRTL_RELATIVE_NAME_U RelativeName
    );

NTSYSAPI
ULONG
NTAPI
RtlDosSearchPath_U(
    _In_ PCWSTR Path,
    _In_ PCWSTR FileName,
    _In_opt_ PCWSTR Extension,
    _In_ ULONG BufferLength,
    _Out_writes_bytes_(BufferLength) PWSTR Buffer,
    _Out_opt_ PWSTR *FilePart
    );

#define RTL_DOS_SEARCH_PATH_FLAG_APPLY_ISOLATION_REDIRECTION 0x00000001
#define RTL_DOS_SEARCH_PATH_FLAG_DISALLOW_DOT_RELATIVE_PATH_SEARCH 0x00000002
#define RTL_DOS_SEARCH_PATH_FLAG_APPLY_DEFAULT_EXTENSION_WHEN_NOT_RELATIVE_PATH_EVEN_IF_FILE_HAS_EXTENSION 0x00000004

NTSYSAPI
NTSTATUS
NTAPI
RtlDosSearchPath_Ustr(
    _In_ ULONG Flags,
    _In_ PCUNICODE_STRING Path,
    _In_ PCUNICODE_STRING FileName,
    _In_opt_ PCUNICODE_STRING DefaultExtension,
    _Out_opt_ PUNICODE_STRING StaticString,
    _Out_opt_ PUNICODE_STRING DynamicString,
    _Out_opt_ PCUNICODE_STRING *FullFileNameOut,
    _Out_opt_ SIZE_T *FilePartPrefixCch,
    _Out_opt_ SIZE_T *BytesRequired
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlDoesFileExists_U(
    _In_ PCWSTR FileName
    );

// ros
NTSYSAPI
NTSTATUS
NTAPI
RtlDosApplyFileIsolationRedirection_Ustr(
    _In_ ULONG Flags,
    _In_ PCUNICODE_STRING OriginalName,
    _In_ PCUNICODE_STRING Extension,
    _In_opt_ PCUNICODE_STRING StaticString,
    _In_opt_ PCUNICODE_STRING DynamicString,
    _In_opt_ PCUNICODE_STRING* NewName,
    _In_ PULONG NewFlags,
    _In_ PSIZE_T FileNameSize,
    _In_ PSIZE_T RequiredLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetLengthWithoutLastFullDosOrNtPathElement(
    _Reserved_ ULONG Flags,
    _In_ PCUNICODE_STRING PathString,
    _Out_ PULONG Length
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetLengthWithoutTrailingPathSeperators(
    _Reserved_ ULONG Flags,
    _In_ PCUNICODE_STRING PathString,
    _Out_ PULONG Length
    );

typedef struct _GENERATE_NAME_CONTEXT
{
    USHORT Checksum;
    BOOLEAN CheckSumInserted;
    UCHAR NameLength;
    WCHAR NameBuffer[8];
    ULONG ExtensionLength;
    WCHAR ExtensionBuffer[4];
    ULONG LastIndexValue;
} GENERATE_NAME_CONTEXT, *PGENERATE_NAME_CONTEXT;

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlGenerate8dot3Name(
    _In_ PCUNICODE_STRING Name,
    _In_ BOOLEAN AllowExtendedCharacters,
    _Inout_ PGENERATE_NAME_CONTEXT Context,
    _Inout_ PUNICODE_STRING Name8dot3
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlComputePrivatizedDllName_U(
    _In_ PCUNICODE_STRING DllName,
    _Out_ PUNICODE_STRING RealName,
    _Out_ PUNICODE_STRING LocalName
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlGetSearchPath(
    _Out_ PCWSTR* Path // RtlReleasePath
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlSetSearchPathMode(
    _In_ ULONG Flags
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlGetExePath(
    _In_ PCWSTR DosPathName,
    _Out_ PCWSTR* Path
    );

// rev
NTSYSAPI
VOID
NTAPI
RtlReleasePath(
    _In_ PCWSTR Path
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_8

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
// rev
NTSYSAPI
ULONG
NTAPI
RtlReplaceSystemDirectoryInPath(
    _Inout_ PUNICODE_STRING Destination,
    _In_ USHORT Machine, // IMAGE_FILE_MACHINE_I386
    _In_ USHORT TargetMachine, // IMAGE_FILE_MACHINE_TARGET_HOST
    _In_ BOOLEAN IncludePathSeperator
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS1

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
// rev from Wow64DetermineEnvironment
NTSYSAPI
USHORT
NTAPI
RtlWow64GetCurrentMachine(
    VOID
    );

// rev from Wow64DetermineEnvironment
NTSYSAPI
NTSTATUS
NTAPI
RtlWow64IsWowGuestMachineSupported(
    _In_ USHORT NativeMachine,
    _Out_ PBOOLEAN IsWowGuestMachineSupported
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS1

#if (PHNT_VERSION >= PHNT_WINDOWS_10_21H2)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlWow64GetProcessMachines(
    _In_ HANDLE ProcessHandle,
    _Out_ PUSHORT ProcessMachine,
    _Out_opt_ PUSHORT NativeMachine
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_21H2

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
// rev
#define IMAGE_FILE_NATIVE_MACHINE_I386  0x1
#define IMAGE_FILE_NATIVE_MACHINE_AMD64 0x2
#define IMAGE_FILE_NATIVE_MACHINE_ARMNT 0x4
#define IMAGE_FILE_NATIVE_MACHINE_ARM64 0x8

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlGetImageFileMachines(
    _In_ PCWSTR FileName,
    _Out_ PUSHORT FileMachines
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS2)
// rev
NTSYSAPI
PWSTR
NTAPI
RtlGetNtSystemRoot(
    VOID
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
RtlAreLongPathsEnabled(
    VOID
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS2

NTSYSAPI
BOOLEAN
NTAPI
RtlIsThreadWithinLoaderCallout(
    VOID
    );

/**
 * Gets a value indicating whether the process is currently in the shutdown phase.
 *
 * @return TRUE if a shutdown of the current dll process is in progress; otherwise, FALSE.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlDllShutdownInProgress(
    VOID
    );

//
// Heaps
//

typedef struct _RTL_HEAP_ENTRY
{
    SIZE_T Size;
    USHORT Flags;
    USHORT AllocatorBackTraceIndex;
    union
    {
        struct
        {
            SIZE_T Settable;
            ULONG Tag;
        } s1;
        struct
        {
            SIZE_T CommittedSize;
            PVOID FirstBlock;
        } s2;
    } u;
} RTL_HEAP_ENTRY, *PRTL_HEAP_ENTRY;

#define RTL_HEAP_BUSY (USHORT)0x0001
#define RTL_HEAP_SEGMENT (USHORT)0x0002
#define RTL_HEAP_SETTABLE_VALUE (USHORT)0x0010
#define RTL_HEAP_SETTABLE_FLAG1 (USHORT)0x0020
#define RTL_HEAP_SETTABLE_FLAG2 (USHORT)0x0040
#define RTL_HEAP_SETTABLE_FLAG3 (USHORT)0x0080
#define RTL_HEAP_SETTABLE_FLAGS (USHORT)0x00e0
#define RTL_HEAP_UNCOMMITTED_RANGE (USHORT)0x1000
#define RTL_HEAP_PROTECTED_ENTRY (USHORT)0x2000
#define RTL_HEAP_LARGE_ALLOC (USHORT)0x4000
#define RTL_HEAP_LFH_ALLOC (USHORT)0x8000

typedef struct _RTL_HEAP_TAG
{
    ULONG NumberOfAllocations;
    ULONG NumberOfFrees;
    SIZE_T BytesAllocated;
    USHORT TagIndex;
    USHORT CreatorBackTraceIndex;
    WCHAR TagName[24];
} RTL_HEAP_TAG, *PRTL_HEAP_TAG;

// Windows 7/8/10
typedef struct _RTL_HEAP_INFORMATION_V1
{
    PVOID BaseAddress;
    ULONG Flags;
    USHORT EntryOverhead;
    USHORT CreatorBackTraceIndex;
    SIZE_T BytesAllocated;
    SIZE_T BytesCommitted;
    ULONG NumberOfTags;
    ULONG NumberOfEntries;
    ULONG NumberOfPseudoTags;
    ULONG PseudoTagGranularity;
    ULONG Reserved[5];
    PRTL_HEAP_TAG Tags;
    PRTL_HEAP_ENTRY Entries;
} RTL_HEAP_INFORMATION_V1, *PRTL_HEAP_INFORMATION_V1;

// Windows 11 > 22000
typedef struct _RTL_HEAP_INFORMATION_V2
{
    PVOID BaseAddress;
    ULONG Flags;
    USHORT EntryOverhead;
    USHORT CreatorBackTraceIndex;
    SIZE_T BytesAllocated;
    SIZE_T BytesCommitted;
    ULONG NumberOfTags;
    ULONG NumberOfEntries;
    ULONG NumberOfPseudoTags;
    ULONG PseudoTagGranularity;
    ULONG Reserved[5];
    PRTL_HEAP_TAG Tags;
    PRTL_HEAP_ENTRY Entries;
    ULONG64 HeapTag;
} RTL_HEAP_INFORMATION_V2, *PRTL_HEAP_INFORMATION_V2;

#define RTL_HEAP_SIGNATURE 0xFFEEFFEEUL
#define RTL_HEAP_SEGMENT_SIGNATURE 0xDDEEDDEEUL

typedef struct _RTL_PROCESS_HEAPS_V1
{
    ULONG NumberOfHeaps;
    _Field_size_(NumberOfHeaps) RTL_HEAP_INFORMATION_V1 Heaps[1];
} RTL_PROCESS_HEAPS_V1, *PRTL_PROCESS_HEAPS_V1;

typedef struct _RTL_PROCESS_HEAPS_V2
{
    ULONG NumberOfHeaps;
    _Field_size_(NumberOfHeaps) RTL_HEAP_INFORMATION_V2 Heaps[1];
} RTL_PROCESS_HEAPS_V2, *PRTL_PROCESS_HEAPS_V2;

//
// Segment heap parameters.
//

typedef enum _RTL_MEMORY_TYPE
{
    MemoryTypePaged,
    MemoryTypeNonPaged,
    MemoryType64KPage,
    MemoryTypeLargePage,
    MemoryTypeHugePage,
    MemoryTypeCustom,
    MemoryTypeMax
} RTL_MEMORY_TYPE, *PRTL_MEMORY_TYPE;

typedef enum _HEAP_MEMORY_INFO_CLASS
{
    HeapMemoryBasicInformation
} HEAP_MEMORY_INFO_CLASS;

typedef _Function_class_(ALLOCATE_VIRTUAL_MEMORY_EX_CALLBACK)
NTSTATUS NTAPI ALLOCATE_VIRTUAL_MEMORY_EX_CALLBACK(
    _Inout_ HANDLE CallbackContext,
    _In_ HANDLE ProcessHandle,
    _Inout_ _At_ (*BaseAddress, _Readable_bytes_ (*RegionSize) _Writable_bytes_ (*RegionSize) _Post_readable_byte_size_ (*RegionSize)) PVOID* BaseAddress,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG AllocationType,
    _In_ ULONG PageProtection,
    _Inout_updates_opt_(ExtendedParameterCount) PMEM_EXTENDED_PARAMETER ExtendedParameters,
    _In_ ULONG ExtendedParameterCount
    );
typedef ALLOCATE_VIRTUAL_MEMORY_EX_CALLBACK *PALLOCATE_VIRTUAL_MEMORY_EX_CALLBACK;

typedef _Function_class_(FREE_VIRTUAL_MEMORY_EX_CALLBACK)
NTSTATUS NTAPI FREE_VIRTUAL_MEMORY_EX_CALLBACK(
    _Inout_ HANDLE CallbackContext,
    _In_ HANDLE ProcessHandle,
    _Inout_ __drv_freesMem(Mem) PVOID *BaseAddress,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG FreeType
    );
typedef FREE_VIRTUAL_MEMORY_EX_CALLBACK *PFREE_VIRTUAL_MEMORY_EX_CALLBACK;

typedef _Function_class_(QUERY_VIRTUAL_MEMORY_CALLBACK)
NTSTATUS NTAPI QUERY_VIRTUAL_MEMORY_CALLBACK(
    _Inout_ HANDLE CallbackContext,
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID BaseAddress,
    _In_ HEAP_MEMORY_INFO_CLASS MemoryInformationClass,
    _Out_writes_bytes_(MemoryInformationLength) PVOID MemoryInformation,
    _In_ SIZE_T MemoryInformationLength,
    _Out_opt_ PSIZE_T ReturnLength
    );
typedef QUERY_VIRTUAL_MEMORY_CALLBACK *PQUERY_VIRTUAL_MEMORY_CALLBACK;

typedef struct _RTL_SEGMENT_HEAP_VA_CALLBACKS
{
    HANDLE CallbackContext;
    PALLOCATE_VIRTUAL_MEMORY_EX_CALLBACK AllocateVirtualMemory;
    PFREE_VIRTUAL_MEMORY_EX_CALLBACK FreeVirtualMemory;
    PQUERY_VIRTUAL_MEMORY_CALLBACK QueryVirtualMemory;
} RTL_SEGMENT_HEAP_VA_CALLBACKS, *PRTL_SEGMENT_HEAP_VA_CALLBACKS;

#define RTL_SEGHEAP_MEM_SOURCE_ANY_NODE ((ULONG)-1)

typedef struct _RTL_SEGMENT_HEAP_MEMORY_SOURCE
{
    ULONG Flags;
    ULONG MemoryTypeMask; // Mask of RTL_MEMORY_TYPE members.
    ULONG NumaNode;
    union
    {
        HANDLE PartitionHandle;
        RTL_SEGMENT_HEAP_VA_CALLBACKS *Callbacks;
    };
    SIZE_T Reserved[2];
} RTL_SEGMENT_HEAP_MEMORY_SOURCE, *PRTL_SEGMENT_HEAP_MEMORY_SOURCE;

#define SEGMENT_HEAP_PARAMETERS_VERSION         3
#define SEGMENT_HEAP_FLG_USE_PAGE_HEAP          0x1
#define SEGMENT_HEAP_FLG_NO_LFH                 0x2
#define SEGMENT_HEAP_PARAMS_VALID_FLAGS         0x3

typedef struct _RTL_SEGMENT_HEAP_PARAMETERS
{
    USHORT Version;
    USHORT Size;
    ULONG Flags;
    RTL_SEGMENT_HEAP_MEMORY_SOURCE MemorySource;
    SIZE_T Reserved[4];
} RTL_SEGMENT_HEAP_PARAMETERS, *PRTL_SEGMENT_HEAP_PARAMETERS;

//
// Heap parameters.
//

typedef _Function_class_(RTL_HEAP_COMMIT_ROUTINE)
NTSTATUS NTAPI RTL_HEAP_COMMIT_ROUTINE(
    _In_ PVOID Base,
    _Inout_ PVOID* CommitAddress,
    _Inout_ PSIZE_T CommitSize
    );
typedef RTL_HEAP_COMMIT_ROUTINE* PRTL_HEAP_COMMIT_ROUTINE;

typedef struct _RTL_HEAP_PARAMETERS
{
    ULONG Length;
    SIZE_T SegmentReserve;
    SIZE_T SegmentCommit;
    SIZE_T DeCommitFreeBlockThreshold;
    SIZE_T DeCommitTotalFreeThreshold;
    SIZE_T MaximumAllocationSize;
    SIZE_T VirtualMemoryThreshold;
    SIZE_T InitialCommit;
    SIZE_T InitialReserve;
    PRTL_HEAP_COMMIT_ROUTINE CommitRoutine;
    SIZE_T Reserved[2];
} RTL_HEAP_PARAMETERS, *PRTL_HEAP_PARAMETERS;

#define HEAP_SETTABLE_USER_VALUE 0x00000100
#define HEAP_SETTABLE_USER_FLAG1 0x00000200
#define HEAP_SETTABLE_USER_FLAG2 0x00000400
#define HEAP_SETTABLE_USER_FLAG3 0x00000800
#define HEAP_SETTABLE_USER_FLAGS 0x00000e00

#define HEAP_CLASS_0 0x00000000 // Process heap
#define HEAP_CLASS_1 0x00001000 // Private heap
#define HEAP_CLASS_2 0x00002000 // Kernel heap
#define HEAP_CLASS_3 0x00003000 // GDI heap
#define HEAP_CLASS_4 0x00004000 // User heap
#define HEAP_CLASS_5 0x00005000 // Console heap
#define HEAP_CLASS_6 0x00006000 // User desktop heap
#define HEAP_CLASS_7 0x00007000 // CSR shared heap
#define HEAP_CLASS_8 0x00008000 // CSR port heap
#define HEAP_CLASS_MASK 0x0000f000

#define HEAP_MAXIMUM_TAG 0x0FFF
#define HEAP_GLOBAL_TAG 0x0800
#define HEAP_PSEUDO_TAG_FLAG 0x8000
#define HEAP_TAG_SHIFT 18
#define HEAP_TAG_MASK (HEAP_MAXIMUM_TAG << HEAP_TAG_SHIFT)

#define HEAP_CREATE_SEGMENT_HEAP 0x00000100
//
// Only applies to segment heap. Applies pointer obfuscation which is
// generally excessive and unnecessary but is necessary for certain insecure
// heaps in win32k.
//
// Specifying HEAP_CREATE_HARDENED prevents the heap from using locks as
// pointers would potentially be exposed in heap metadata lock variables.
// Callers are therefore responsible for synchronizing access to hardened heaps.
//
#define HEAP_CREATE_HARDENED 0x00000200

/**
 * The RtlCreateHeap routine creates a heap object that can be used by the calling process. This routine reserves
 * space in the virtual address space of the process and allocates physical storage for a specified initial portion of this block.
 *
 * @param Flags Flags specifying optional attributes of the heap.
 * @param HeapBase If HeapBase is a non-NULL value, it specifies the base address for a block of caller-allocated memory to use for the heap.
 * @param ReserveSize If ReserveSize is a nonzero value, it specifies the initial amount of memory, in bytes, to reserve for the heap.
 * @param CommitSize If CommitSize is a nonzero value, it specifies the initial amount of memory, in bytes, to commit for the heap.
 * @param Lock Pointer to an opaque structure to be used as the heap lock.
 * @param Parameters Pointer to a RTL_HEAP_PARAMETERS structure that contains parameters to be applied when creating the heap.
 * @return RtlCreateHeap returns a handle to be used in accessing the created heap.
 * @remarks https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlcreateheap
 */
_Success_(return != 0)
_Must_inspect_result_
_Ret_maybenull_
NTSYSAPI
HANDLE
NTAPI
RtlCreateHeap(
    _In_ ULONG Flags,
    _In_opt_ PVOID HeapBase,
    _In_opt_ SIZE_T ReserveSize,
    _In_opt_ SIZE_T CommitSize,
    _In_opt_ PVOID Lock,
    _When_((Flags & HEAP_CREATE_SEGMENT_HEAP) != 0, _In_reads_bytes_opt_(sizeof(RTL_SEGMENT_HEAP_PARAMETERS)))
    _When_((Flags & HEAP_CREATE_SEGMENT_HEAP) == 0, _In_reads_bytes_opt_(sizeof(RTL_HEAP_PARAMETERS)))
    _In_opt_ PVOID Parameters
    );

/**
 * The RtlDestroyHeap routine destroys the specified heap object. RtlDestroyHeap decommits and releases all the pages of a private heap object,
 * and it invalidates the handle to the heap.
 *
 * @param HeapHandle Handle for the heap to be destroyed. This parameter is a heap handle returned by RtlCreateHeap.
 * @return If the call to RtlDestroyHeap succeeds, the return value is a NULL pointer. If the call to RtlDestroyHeap fails, the return value is a handle for the heap.
 * @remarks https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtldestroyheap
 */
_Success_(return == 0)
NTSYSAPI
PVOID
NTAPI
RtlDestroyHeap(
    _In_ _Post_invalid_ HANDLE HeapHandle
    );

/**
 * The RtlAllocateHeap routine allocates a block of memory from a heap.
 *
 * @param HeapHandle Handle for a private heap from which the memory will be allocated.
 * @param Flags Controllable aspects of heap allocation. Specifying any flags will override the corresponding value specified when the heap was created with RtlCreateHeap.
 * @param Size Number of bytes to be allocated. If the heap, specified by the HeapHandle parameter, is a nongrowable heap, Size must be less than or equal to the heap's virtual memory threshold.
 * @return If the call to RtlAllocateHeap succeeds, the return value is a pointer to the newly-allocated block. The return value is NULL if the allocation failed.
 * @remarks https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlallocateheap
 */
_Success_(return != 0)
_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
__drv_allocatesMem(Mem)
NTSYSAPI
DECLSPEC_ALLOCATOR
DECLSPEC_NOALIAS
DECLSPEC_RESTRICT
PVOID
NTAPI
RtlAllocateHeap(
    _In_ HANDLE HeapHandle,
    _In_opt_ ULONG Flags,
    _In_ SIZE_T Size
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
_Success_(return != 0)
NTSYSAPI
LOGICAL
NTAPI
RtlFreeHeap(
    _In_ HANDLE HeapHandle,
    _In_opt_ ULONG Flags,
    _Frees_ptr_opt_ _Post_invalid_ PVOID BaseAddress
    );
#else
_Success_(return)
NTSYSAPI
BOOLEAN
NTAPI
RtlFreeHeap(
    _In_ HANDLE HeapHandle,
    _In_opt_ ULONG Flags,
    _Frees_ptr_opt_ PVOID BaseAddress
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8

NTSYSAPI
SIZE_T
NTAPI
RtlSizeHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _In_ PCVOID BaseAddress
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlZeroHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags
    );

NTSYSAPI
VOID
NTAPI
RtlProtectHeap(
    _In_ HANDLE HeapHandle,
    _In_ BOOLEAN MakeReadOnly
    );

#define RtlProcessHeap() (NtCurrentPeb()->ProcessHeap)

NTSYSAPI
BOOLEAN
NTAPI
RtlLockHeap(
    _In_ HANDLE HeapHandle
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlUnlockHeap(
    _In_ HANDLE HeapHandle
    );

_Success_(return != 0)
_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
_When_(Size > 0, __drv_allocatesMem(Mem))
NTSYSAPI
DECLSPEC_ALLOCATOR
DECLSPEC_NOALIAS
DECLSPEC_RESTRICT
PVOID
NTAPI
RtlReAllocateHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _Frees_ptr_opt_ PVOID BaseAddress,
    _In_ SIZE_T Size
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlGetUserInfoHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _In_ PVOID BaseAddress,
    _Out_opt_ PVOID *UserValue,
    _Out_opt_ PULONG UserFlags
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlSetUserValueHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _In_ PVOID BaseAddress,
    _In_ PVOID UserValue
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlSetUserFlagsHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _In_ PVOID BaseAddress,
    _In_ ULONG UserFlagsReset,
    _In_ ULONG UserFlagsSet
    );

typedef struct _RTL_HEAP_TAG_INFO
{
    ULONG NumberOfAllocations;
    ULONG NumberOfFrees;
    SIZE_T BytesAllocated;
} RTL_HEAP_TAG_INFO, *PRTL_HEAP_TAG_INFO;

#define RTL_HEAP_MAKE_TAG HEAP_MAKE_TAG_FLAGS

NTSYSAPI
ULONG
NTAPI
RtlCreateTagHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _In_opt_ PCWSTR TagPrefix,
    _In_ PCWSTR TagNames
    );

NTSYSAPI
PWSTR
NTAPI
RtlQueryTagHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _In_ USHORT TagIndex,
    _In_ BOOLEAN ResetCounters,
    _Out_opt_ PRTL_HEAP_TAG_INFO TagInfo
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlExtendHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _In_ PVOID Base,
    _In_ SIZE_T Size
    );

NTSYSAPI
SIZE_T
NTAPI
RtlCompactHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlValidateHeap(
    _In_opt_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _In_opt_ PVOID BaseAddress
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlValidateProcessHeaps(
    VOID
    );

NTSYSAPI
ULONG
NTAPI
RtlGetProcessHeaps(
    _In_ ULONG NumberOfHeaps,
    _Out_ PVOID *ProcessHeaps
    );

typedef _Function_class_(RTL_ENUM_HEAPS_ROUTINE)
NTSTATUS NTAPI RTL_ENUM_HEAPS_ROUTINE(
    _In_ HANDLE HeapHandle,
    _In_ PVOID Parameter
    );
typedef RTL_ENUM_HEAPS_ROUTINE *PRTL_ENUM_HEAPS_ROUTINE;

NTSYSAPI
NTSTATUS
NTAPI
RtlEnumProcessHeaps(
    _In_ PRTL_ENUM_HEAPS_ROUTINE EnumRoutine,
    _In_ PVOID Parameter
    );

typedef struct _RTL_HEAP_USAGE_ENTRY
{
    struct _RTL_HEAP_USAGE_ENTRY *Next;
    PVOID Address;
    SIZE_T Size;
    USHORT AllocatorBackTraceIndex;
    USHORT TagIndex;
} RTL_HEAP_USAGE_ENTRY, *PRTL_HEAP_USAGE_ENTRY;

typedef struct _RTL_HEAP_USAGE
{
    ULONG Length;
    SIZE_T BytesAllocated;
    SIZE_T BytesCommitted;
    SIZE_T BytesReserved;
    SIZE_T BytesReservedMaximum;
    PRTL_HEAP_USAGE_ENTRY Entries;
    PRTL_HEAP_USAGE_ENTRY AddedEntries;
    PRTL_HEAP_USAGE_ENTRY RemovedEntries;
    ULONG_PTR Reserved[8];
} RTL_HEAP_USAGE, *PRTL_HEAP_USAGE;

#define HEAP_USAGE_ALLOCATED_BLOCKS HEAP_REALLOC_IN_PLACE_ONLY
#define HEAP_USAGE_FREE_BUFFER HEAP_ZERO_MEMORY

NTSYSAPI
NTSTATUS
NTAPI
RtlUsageHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _Inout_ PRTL_HEAP_USAGE Usage
    );

typedef struct _RTL_HEAP_WALK_ENTRY
{
    PVOID DataAddress;
    SIZE_T DataSize;
    UCHAR OverheadBytes;
    UCHAR SegmentIndex;
    USHORT Flags;
    union
    {
        struct
        {
            SIZE_T Settable;
            USHORT TagIndex;
            USHORT AllocatorBackTraceIndex;
            ULONG Reserved[2];
        } Block;
        struct
        {
            ULONG CommittedSize;
            ULONG UnCommittedSize;
            PVOID FirstEntry;
            PVOID LastEntry;
        } Segment;
    };
} RTL_HEAP_WALK_ENTRY, *PRTL_HEAP_WALK_ENTRY;

NTSYSAPI
NTSTATUS
NTAPI
RtlWalkHeap(
    _In_ HANDLE HeapHandle,
    _Inout_ PRTL_HEAP_WALK_ENTRY Entry
    );

// HEAP_INFORMATION_CLASS
#define HeapCompatibilityInformation 0x0 // q; s: ULONG
#define HeapEnableTerminationOnCorruption 0x1 // q; s: NULL
#define HeapExtendedInformation 0x2 // q; s: HEAP_EXTENDED_INFORMATION
#define HeapOptimizeResources 0x3 // q; s: HEAP_OPTIMIZE_RESOURCES_INFORMATION
#define HeapTaggingInformation 0x4 // q: RTLP_HEAP_TAGGING_INFO
#define HeapStackDatabase 0x5 // q: RTL_HEAP_STACK_QUERY; s: RTL_HEAP_STACK_CONTROL
#define HeapMemoryLimit 0x6 // since 19H2
#define HeapTag 0x7 // since 20H1
#define HeapDetailedFailureInformation 0x80000001
#define HeapSetDebuggingInformation 0x80000002 // q; s: HEAP_DEBUGGING_INFORMATION

typedef enum _HEAP_COMPATIBILITY_MODE
{
    HEAP_COMPATIBILITY_MODE_STANDARD = 0UL,
    HEAP_COMPATIBILITY_MODE_LAL = 1UL, // Lookaside list heap (LAL) compatibility mode.
    HEAP_COMPATIBILITY_MODE_LFH = 2UL, // Low-fragmentation heap (LFH) compatibility mode.
} HEAP_COMPATIBILITY_MODE;

typedef struct _RTLP_TAG_INFO
{
    GUID Id;
    SIZE_T CurrentAllocatedBytes;
} RTLP_TAG_INFO, *PRTLP_TAG_INFO;

#define RTLP_HEAP_TAGGING_INFO_VERSION 0x1

typedef struct _RTLP_HEAP_TAGGING_INFO
{
    USHORT Version;
    USHORT Flags; // 1: Multiple Tags, 2: Single Tag + Hash
    HANDLE ProcessHandle;
    SIZE_T EntriesCount;
    RTLP_TAG_INFO Entries[1];
} RTLP_HEAP_TAGGING_INFO, *PRTLP_HEAP_TAGGING_INFO;

typedef struct _PROCESS_HEAP_INFORMATION
{
    SIZE_T ReserveSize;
    SIZE_T CommitSize;
    ULONG NumberOfHeaps;
    ULONG_PTR FirstHeapInformationOffset;
} PROCESS_HEAP_INFORMATION, *PPROCESS_HEAP_INFORMATION;

typedef struct _HEAP_REGION_INFORMATION
{
    PVOID Address;
    SIZE_T ReserveSize;
    SIZE_T CommitSize;
    ULONG_PTR FirstRangeInformationOffset;
    ULONG_PTR NextRegionInformationOffset;
} HEAP_REGION_INFORMATION, *PHEAP_REGION_INFORMATION;

typedef struct _HEAP_RANGE_INFORMATION
{
    PVOID Address;
    SIZE_T Size;
    ULONG Type;
    ULONG Protection;
    ULONG_PTR FirstBlockInformationOffset;
    ULONG_PTR NextRangeInformationOffset;
} HEAP_RANGE_INFORMATION, *PHEAP_RANGE_INFORMATION;

typedef struct _HEAP_BLOCK_INFORMATION
{
    PVOID Address;
    ULONG Flags;
    SIZE_T DataSize;
    ULONG_PTR OverheadSize;
    ULONG_PTR NextBlockInformationOffset;
} HEAP_BLOCK_INFORMATION, *PHEAP_BLOCK_INFORMATION;

typedef struct _HEAP_INFORMATION
{
    PVOID Address;
    ULONG Mode;
    SIZE_T ReserveSize;
    SIZE_T CommitSize;
    ULONG_PTR FirstRegionInformationOffset;
    ULONG_PTR NextHeapInformationOffset;
} HEAP_INFORMATION, *PHEAP_INFORMATION;

typedef struct _SEGMENT_HEAP_PERFORMANCE_COUNTER_INFORMATION
{
    SIZE_T SegmentReserveSize;
    SIZE_T SegmentCommitSize;
    SIZE_T SegmentCount;
    SIZE_T AllocatedSize;
    SIZE_T LargeAllocReserveSize;
    SIZE_T LargeAllocCommitSize;
} SEGMENT_HEAP_PERFORMANCE_COUNTER_INFORMATION, *PSEGMENT_HEAP_PERFORMANCE_COUNTER_INFORMATION;

#define HeapPerformanceCountersInformationStandardHeapVersion 0x1
#define HeapPerformanceCountersInformationSegmentHeapVersion 0x2

typedef struct _HEAP_PERFORMANCE_COUNTERS_INFORMATION
{
    ULONG Size;
    ULONG Version;
    ULONG HeapIndex;
    ULONG LastHeapIndex;
    PVOID BaseAddress;
    SIZE_T ReserveSize;
    SIZE_T CommitSize;
    ULONG SegmentCount;
    SIZE_T LargeUCRMemory;
    ULONG UCRLength;
    SIZE_T AllocatedSpace;
    SIZE_T FreeSpace;
    ULONG FreeListLength;
    ULONG Contention;
    ULONG VirtualBlocks;
    ULONG CommitRate;
    ULONG DecommitRate;
    SEGMENT_HEAP_PERFORMANCE_COUNTER_INFORMATION SegmentHeapPerfInformation; // since WIN8
} HEAP_PERFORMANCE_COUNTERS_INFORMATION, *PHEAP_PERFORMANCE_COUNTERS_INFORMATION;

typedef struct _HEAP_INFORMATION_ITEM
{
    ULONG Level;
    SIZE_T Size;
    union
    {
        PROCESS_HEAP_INFORMATION ProcessHeapInformation;
        HEAP_INFORMATION HeapInformation;
        HEAP_REGION_INFORMATION HeapRegionInformation;
        HEAP_RANGE_INFORMATION HeapRangeInformation;
        HEAP_BLOCK_INFORMATION HeapBlockInformation;
        HEAP_PERFORMANCE_COUNTERS_INFORMATION HeapPerfInformation;
        ULONG_PTR DynamicStart;
    };
} HEAP_INFORMATION_ITEM, *PHEAP_INFORMATION_ITEM;

typedef _Function_class_(RTL_HEAP_EXTENDED_ENUMERATION_ROUTINE)
NTSTATUS NTAPI RTL_HEAP_EXTENDED_ENUMERATION_ROUTINE(
    _In_ PHEAP_INFORMATION_ITEM Information,
    _In_opt_ PVOID Context
    );
typedef RTL_HEAP_EXTENDED_ENUMERATION_ROUTINE* PRTL_HEAP_EXTENDED_ENUMERATION_ROUTINE;

// HEAP_EXTENDED_INFORMATION Level
#define HeapExtendedProcessHeapInformationLevel 0x1
#define HeapExtendedHeapInformationLevel 0x2
#define HeapExtendedHeapRegionInformationLevel 0x3
#define HeapExtendedHeapRangeInformationLevel 0x4
#define HeapExtendedHeapBlockInformationLevel 0x5
#define HeapExtendedHeapHeapPerfInformationLevel 0x80000000

typedef struct _HEAP_EXTENDED_INFORMATION
{
    HANDLE ProcessHandle;
    HANDLE HeapHandle;
    ULONG Level;
    PRTL_HEAP_EXTENDED_ENUMERATION_ROUTINE CallbackRoutine;
    PVOID CallbackContext;
    union
    {
        PROCESS_HEAP_INFORMATION ProcessHeapInformation;
        HEAP_INFORMATION HeapInformation;
    };
} HEAP_EXTENDED_INFORMATION, *PHEAP_EXTENDED_INFORMATION;

// rev
typedef _Function_class_(RTL_HEAP_STACK_WRITE_ROUTINE)
NTSTATUS NTAPI RTL_HEAP_STACK_WRITE_ROUTINE(
    _In_ PVOID Information, // TODO: 3 missing structures (dmex)
    _In_ ULONG Size,
    _In_opt_ PVOID Context
    );
typedef RTL_HEAP_STACK_WRITE_ROUTINE* PRTL_HEAP_STACK_WRITE_ROUTINE;

// rev
typedef struct _RTLP_HEAP_STACK_TRACE_SERIALIZATION_INIT
{
    ULONG Count;
    ULONG Total;
    ULONG Flags;
} RTLP_HEAP_STACK_TRACE_SERIALIZATION_INIT, *PRTLP_HEAP_STACK_TRACE_SERIALIZATION_INIT;

// rev
typedef struct _RTLP_HEAP_STACK_TRACE_SERIALIZATION_HEADER
{
    USHORT Version;
    USHORT PointerSize;
    PVOID Heap;
    SIZE_T TotalCommit;
    SIZE_T TotalReserve;
} RTLP_HEAP_STACK_TRACE_SERIALIZATION_HEADER, *PRTLP_HEAP_STACK_TRACE_SERIALIZATION_HEADER;

// rev
typedef struct _RTLP_HEAP_STACK_TRACE_SERIALIZATION_ALLOCATION
{
    PVOID Address;
    ULONG Flags;
    SIZE_T DataSize;
} RTLP_HEAP_STACK_TRACE_SERIALIZATION_ALLOCATION, *PRTLP_HEAP_STACK_TRACE_SERIALIZATION_ALLOCATION;

// rev
typedef struct _RTLP_HEAP_STACK_TRACE_SERIALIZATION_STACKFRAME
{
    PVOID StackFrame[8];
} RTLP_HEAP_STACK_TRACE_SERIALIZATION_STACKFRAME, *PRTLP_HEAP_STACK_TRACE_SERIALIZATION_STACKFRAME;

#define HEAP_STACK_QUERY_VERSION 0x2

typedef struct _RTL_HEAP_STACK_QUERY
{
    ULONG Version;
    HANDLE ProcessHandle;
    PRTL_HEAP_STACK_WRITE_ROUTINE WriteRoutine;
    PVOID SerializationContext;
    UCHAR QueryLevel;
    UCHAR Flags;
} RTL_HEAP_STACK_QUERY, *PRTL_HEAP_STACK_QUERY;

#define HEAP_STACK_CONTROL_VERSION 0x1
#define HEAP_STACK_CONTROL_FLAGS_STACKTRACE_ENABLE 0x1
#define HEAP_STACK_CONTROL_FLAGS_STACKTRACE_DISABLE 0x2

typedef struct _RTL_HEAP_STACK_CONTROL
{
    USHORT Version;
    USHORT Flags;
    HANDLE ProcessHandle;
} RTL_HEAP_STACK_CONTROL, *PRTL_HEAP_STACK_CONTROL;

// rev
typedef _Function_class_(RTL_HEAP_DEBUGGING_INTERCEPTOR_ROUTINE)
NTSTATUS NTAPI RTL_HEAP_DEBUGGING_INTERCEPTOR_ROUTINE(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Action,
    _In_ ULONG StackFramesToCapture,
    _In_ PVOID *StackTrace
    );
typedef RTL_HEAP_DEBUGGING_INTERCEPTOR_ROUTINE* PRTL_HEAP_DEBUGGING_INTERCEPTOR_ROUTINE;

// rev
typedef _Function_class_(RTL_HEAP_LEAK_ENUMERATION_ROUTINE)
NTSTATUS NTAPI RTL_HEAP_LEAK_ENUMERATION_ROUTINE(
    _In_ LONG Reserved,
    _In_ HANDLE HeapHandle,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T BlockSize,
    _In_ ULONG StackTraceDepth,
    _In_ PVOID *StackTrace
    );
typedef RTL_HEAP_LEAK_ENUMERATION_ROUTINE* PRTL_HEAP_LEAK_ENUMERATION_ROUTINE;

// symbols
typedef struct _HEAP_DEBUGGING_INFORMATION
{
    PRTL_HEAP_DEBUGGING_INTERCEPTOR_ROUTINE InterceptorFunction;
    USHORT InterceptorValue;
    ULONG ExtendedOptions;
    ULONG StackTraceDepth;
    SIZE_T MinTotalBlockSize;
    SIZE_T MaxTotalBlockSize;
    PRTL_HEAP_LEAK_ENUMERATION_ROUTINE HeapLeakEnumerationRoutine;
} HEAP_DEBUGGING_INFORMATION, *PHEAP_DEBUGGING_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryHeapInformation(
    _In_opt_ HANDLE HeapHandle,
    _In_ HEAP_INFORMATION_CLASS HeapInformationClass,
    _Out_opt_ PVOID HeapInformation,
    _In_opt_ SIZE_T HeapInformationLength,
    _Out_opt_ PSIZE_T ReturnLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetHeapInformation(
    _In_opt_ HANDLE HeapHandle,
    _In_ HEAP_INFORMATION_CLASS HeapInformationClass,
    _In_opt_ PCVOID HeapInformation,
    _In_opt_ SIZE_T HeapInformationLength
    );

NTSYSAPI
ULONG
NTAPI
RtlMultipleAllocateHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _In_ SIZE_T Size,
    _In_ ULONG Count,
    _Out_ PVOID *Array
    );

NTSYSAPI
ULONG
NTAPI
RtlMultipleFreeHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _In_ ULONG Count,
    _In_ PVOID *Array
    );

NTSYSAPI
VOID
NTAPI
RtlDetectHeapLeaks(
    VOID
    );

NTSYSAPI
VOID
NTAPI
RtlFlushHeaps(
    VOID
    );

//
// Memory zones
//

// begin_private

typedef struct _RTL_MEMORY_ZONE_SEGMENT
{
    struct _RTL_MEMORY_ZONE_SEGMENT *NextSegment;
    SIZE_T Size;
    PVOID Next;
    PVOID Limit;
} RTL_MEMORY_ZONE_SEGMENT, *PRTL_MEMORY_ZONE_SEGMENT;

typedef struct _RTL_MEMORY_ZONE
{
    RTL_MEMORY_ZONE_SEGMENT Segment;
    RTL_SRWLOCK Lock;
    ULONG LockCount;
    PRTL_MEMORY_ZONE_SEGMENT FirstSegment;
} RTL_MEMORY_ZONE, *PRTL_MEMORY_ZONE;

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateMemoryZone(
    _Out_ PVOID *MemoryZone,
    _In_ SIZE_T InitialSize,
    _Reserved_ ULONG Flags
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyMemoryZone(
    _In_ _Post_invalid_ PVOID MemoryZone
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAllocateMemoryZone(
    _In_ PVOID MemoryZone,
    _In_ SIZE_T BlockSize,
    _Out_ PVOID *Block
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlResetMemoryZone(
    _In_ PVOID MemoryZone
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlLockMemoryZone(
    _In_ PVOID MemoryZone
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlUnlockMemoryZone(
    _In_ PVOID MemoryZone
    );

//
// Memory block lookaside lists
//

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateMemoryBlockLookaside(
    _Out_ PVOID *MemoryBlockLookaside,
    _Reserved_ ULONG Flags,
    _In_ ULONG InitialSize,
    _In_ ULONG MinimumBlockSize,
    _In_ ULONG MaximumBlockSize
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyMemoryBlockLookaside(
    _In_ PVOID MemoryBlockLookaside
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAllocateMemoryBlockLookaside(
    _In_ PVOID MemoryBlockLookaside,
    _In_ ULONG BlockSize,
    _Out_ PVOID *Block
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlFreeMemoryBlockLookaside(
    _In_ PVOID MemoryBlockLookaside,
    _In_ PVOID Block
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlExtendMemoryBlockLookaside(
    _In_ PVOID MemoryBlockLookaside,
    _In_ ULONG Increment
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlResetMemoryBlockLookaside(
    _In_ PVOID MemoryBlockLookaside
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlLockMemoryBlockLookaside(
    _In_ PVOID MemoryBlockLookaside
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlUnlockMemoryBlockLookaside(
    _In_ PVOID MemoryBlockLookaside
    );

// end_private

//
// Transactions
//

// private
NTSYSAPI
HANDLE
NTAPI
RtlGetCurrentTransaction(
    _In_opt_ PCWSTR ExistingFileName,
    _In_opt_ PCWSTR NewFileName
    );

// private
NTSYSAPI
LOGICAL
NTAPI
RtlSetCurrentTransaction(
    _In_opt_ HANDLE TransactionHandle
    );

//
// LUIDs
//

FORCEINLINE
BOOLEAN
NTAPI_INLINE
RtlIsEqualLuid( // RtlEqualLuid
    _In_ PLUID L1,
    _In_ PLUID L2
    )
{
    return L1->LowPart == L2->LowPart &&
        L1->HighPart == L2->HighPart;
}

FORCEINLINE
BOOLEAN
NTAPI_INLINE
RtlIsZeroLuid(
    _In_ PLUID L1
    )
{
    return (L1->LowPart | L1->HighPart) == 0;
}

FORCEINLINE
LUID
NTAPI_INLINE
RtlConvertLongToLuid(
    _In_ LONG Long
    )
{
    LUID tempLuid;

    tempLuid.LowPart = Long;
    tempLuid.HighPart = 0;

    return tempLuid;
}

FORCEINLINE
LUID
NTAPI_INLINE
RtlConvertUlongToLuid(
    _In_ ULONG Ulong
    )
{
    LUID tempLuid;

    tempLuid.LowPart = Ulong;
    tempLuid.HighPart = 0;

    return tempLuid;
}

FORCEINLINE
LONGLONG
NTAPI_INLINE
RtlConvertLuidToLonglong(
    _In_ LUID Luid
    )
{
    LARGE_INTEGER tempLi;

    tempLi.LowPart = Luid.LowPart;
    tempLi.HighPart = Luid.HighPart;

    return tempLi.QuadPart;
}

FORCEINLINE
ULONGLONG
NTAPI_INLINE
RtlConvertLuidToUlonglong(
    _In_ LUID Luid
    )
{
    ULARGE_INTEGER tempLi;

    tempLi.LowPart = Luid.LowPart;
    tempLi.HighPart = Luid.HighPart;

    return tempLi.QuadPart;
}

NTSYSAPI
VOID
NTAPI
RtlCopyLuid(
    _Out_ PLUID DestinationLuid,
    _In_ PLUID SourceLuid
    );

// ros
NTSYSAPI
VOID
NTAPI
RtlCopyLuidAndAttributesArray(
    _In_ ULONG Count,
    _In_ PLUID_AND_ATTRIBUTES Src,
    _In_ PLUID_AND_ATTRIBUTES Dest
    );

//
// Byte swap routines.
//

#ifndef PHNT_RTL_BYTESWAP
#define RtlUshortByteSwap(_x) _byteswap_ushort((USHORT)(_x))
#define RtlUlongByteSwap(_x) _byteswap_ulong((_x))
#define RtlUlonglongByteSwap(_x) _byteswap_uint64((_x))
#else
NTSYSAPI
USHORT
FASTCALL
RtlUshortByteSwap(
    _In_ USHORT Source
    );

NTSYSAPI
ULONG
FASTCALL
RtlUlongByteSwap(
    _In_ ULONG Source
    );

NTSYSAPI
ULONGLONG
FASTCALL
RtlUlonglongByteSwap(
    _In_ ULONGLONG Source
    );
#endif // PHNT_RTL_BYTESWAP

DECLSPEC_DEPRECATED
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlConvertUlongToLargeInteger(
    _In_ ULONG UnsignedInteger
    );

DECLSPEC_DEPRECATED
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlConvertLongToLargeInteger(
    _In_ LONG SignedInteger
    );

DECLSPEC_DEPRECATED
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlEnlargedIntegerMultiply(
    _In_ LONG Multiplicand,
    _In_ LONG Multiplier
    );

DECLSPEC_DEPRECATED
NTSYSAPI
LARGE_INTEGER
NTAPI_INLINE
RtlEnlargedUnsignedMultiply(
    _In_ ULONG Multiplicand,
    _In_ ULONG Multiplier
    );

//
// Debugging
//

// private
typedef struct _RTL_PROCESS_MODULES *PRTL_PROCESS_MODULES;
typedef struct _RTL_PROCESS_MODULE_INFORMATION_EX *PRTL_PROCESS_MODULE_INFORMATION_EX;
typedef struct _RTL_PROCESS_BACKTRACES *PRTL_PROCESS_BACKTRACES;
typedef struct _RTL_PROCESS_LOCKS *PRTL_PROCESS_LOCKS;

typedef struct _RTL_PROCESS_VERIFIER_OPTIONS
{
    ULONG SizeStruct;
    ULONG Option;
    UCHAR OptionData[1];
} RTL_PROCESS_VERIFIER_OPTIONS, *PRTL_PROCESS_VERIFIER_OPTIONS;

// private
typedef struct _RTL_DEBUG_INFORMATION
{
    HANDLE SectionHandleClient;
    PVOID ViewBaseClient;
    PVOID ViewBaseTarget;
    ULONG_PTR ViewBaseDelta;
    HANDLE EventPairClient;
    HANDLE EventPairTarget;
    HANDLE TargetProcessId;
    HANDLE TargetThreadHandle;
    ULONG Flags;
    SIZE_T OffsetFree;
    SIZE_T CommitSize;
    SIZE_T ViewSize;
    union
    {
        PRTL_PROCESS_MODULES Modules;
        PRTL_PROCESS_MODULE_INFORMATION_EX ModulesEx;
    };
    PRTL_PROCESS_BACKTRACES BackTraces;
    PVOID Heaps;
    PRTL_PROCESS_LOCKS Locks;
    PVOID SpecificHeap;
    HANDLE TargetProcessHandle;
    PRTL_PROCESS_VERIFIER_OPTIONS VerifierOptions;
    PVOID ProcessHeap;
    HANDLE CriticalSectionHandle;
    HANDLE CriticalSectionOwnerThread;
    PVOID Reserved[4];
} RTL_DEBUG_INFORMATION, *PRTL_DEBUG_INFORMATION;

NTSYSAPI
PRTL_DEBUG_INFORMATION
NTAPI
RtlCreateQueryDebugBuffer(
    _In_opt_ ULONG MaximumCommit,
    _In_ BOOLEAN UseEventPair
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyQueryDebugBuffer(
    _In_ PRTL_DEBUG_INFORMATION Buffer
    );

// private
NTSYSAPI
PVOID
NTAPI
RtlCommitDebugInfo(
    _Inout_ PRTL_DEBUG_INFORMATION Buffer,
    _In_ SIZE_T Size
    );

// private
NTSYSAPI
VOID
NTAPI
RtlDeCommitDebugInfo(
    _Inout_ PRTL_DEBUG_INFORMATION Buffer,
    _In_ PVOID p,
    _In_ SIZE_T Size
    );

#define RTL_QUERY_PROCESS_MODULES 0x00000001
#define RTL_QUERY_PROCESS_BACKTRACES 0x00000002
#define RTL_QUERY_PROCESS_HEAP_SUMMARY 0x00000004
#define RTL_QUERY_PROCESS_HEAP_TAGS 0x00000008
#define RTL_QUERY_PROCESS_HEAP_ENTRIES 0x00000010
#define RTL_QUERY_PROCESS_LOCKS 0x00000020
#define RTL_QUERY_PROCESS_MODULES32 0x00000040
#define RTL_QUERY_PROCESS_VERIFIER_OPTIONS 0x00000080 // rev
#define RTL_QUERY_PROCESS_MODULESEX 0x00000100 // rev
#define RTL_QUERY_PROCESS_HEAP_SEGMENTS 0x00000200
#define RTL_QUERY_PROCESS_CS_OWNER 0x00000400 // rev
#define RTL_QUERY_PROCESS_NONINVASIVE 0x80000000
#define RTL_QUERY_PROCESS_NONINVASIVE_CS_OWNER 0x80000800 // WIN11

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProcessDebugInformation(
    _In_ HANDLE UniqueProcessId,
    _In_ ULONG Flags,
    _Inout_ PRTL_DEBUG_INFORMATION Buffer
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlSetProcessDebugInformation(
    _In_ HANDLE UniqueProcessId,
    _In_ ULONG Flags,
    _Inout_ PRTL_DEBUG_INFORMATION Buffer
    );

// rev
FORCEINLINE
BOOLEAN
NTAPI_INLINE
RtlIsAnyDebuggerPresent(
    VOID
    )
{
    BOOLEAN result;

    result = NtCurrentPeb()->BeingDebugged;

    if (!result)
        return USER_SHARED_DATA->KdDebuggerEnabled;

    return result;
}

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlDebugPrintTimes(
    VOID
    );

//
// Messages
//

NTSYSAPI
NTSTATUS
NTAPI
RtlFindMessage(
    _In_ PVOID DllHandle,
    _In_ ULONG MessageTableId,
    _In_ ULONG MessageLanguageId,
    _In_ ULONG MessageId,
    _Out_ PMESSAGE_RESOURCE_ENTRY *MessageEntry
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlFormatMessage(
    _In_ PCWSTR MessageFormat,
    _In_ ULONG MaximumWidth,
    _In_ BOOLEAN IgnoreInserts,
    _In_ BOOLEAN ArgumentsAreAnsi,
    _In_ BOOLEAN ArgumentsAreAnArray,
    _In_ va_list *Arguments,
    _Out_writes_bytes_to_(Length, *ReturnLength) PWSTR Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG ReturnLength
    );

typedef struct _PARSE_MESSAGE_CONTEXT
{
    ULONG fFlags;
    ULONG cwSavColumn;
    SIZE_T iwSrc;
    SIZE_T iwDst;
    SIZE_T iwDstSpace;
    va_list lpvArgStart;
} PARSE_MESSAGE_CONTEXT, *PPARSE_MESSAGE_CONTEXT;

#define INIT_PARSE_MESSAGE_CONTEXT(ctx) { (ctx)->fFlags = 0; }
#define TEST_PARSE_MESSAGE_CONTEXT_FLAG(ctx, flag) ((ctx)->fFlags & (flag))
#define SET_PARSE_MESSAGE_CONTEXT_FLAG(ctx, flag) ((ctx)->fFlags |= (flag))
#define CLEAR_PARSE_MESSAGE_CONTEXT_FLAG(ctx, flag) ((ctx)->fFlags &= ~(flag))

NTSYSAPI
NTSTATUS
NTAPI
RtlFormatMessageEx(
    _In_ PCWSTR MessageFormat,
    _In_ ULONG MaximumWidth,
    _In_ BOOLEAN IgnoreInserts,
    _In_ BOOLEAN ArgumentsAreAnsi,
    _In_ BOOLEAN ArgumentsAreAnArray,
    _In_ va_list *Arguments,
    _Out_writes_bytes_to_(Length, *ReturnLength) PWSTR Buffer,
    _In_ ULONG Length,
    _Out_opt_ PULONG ReturnLength,
    _Out_opt_ PPARSE_MESSAGE_CONTEXT ParseContext
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetFileMUIPath(
    _In_ ULONG Flags,
    _In_ PCWSTR FilePath,
    _Inout_opt_ PCWSTR Language,
    _Inout_ PULONG LanguageLength,
    _Out_opt_ PWSTR FileMUIPath,
    _Inout_ PULONG FileMUIPathLength,
    _Inout_ PULONGLONG Enumerator
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlLoadString(
    _In_ PVOID DllHandle,
    _In_ ULONG StringId,
    _In_opt_ PCWSTR StringLanguage,
    _In_ ULONG Flags,
    _Out_ PCWSTR *ReturnString,
    _Out_opt_ PUSHORT ReturnStringLen,
    _Out_writes_(ReturnLanguageLen) PWSTR ReturnLanguageName,
    _Inout_opt_ PULONG ReturnLanguageLen
    );

//
// Errors
//

_When_(Status < 0, _Out_range_(>, 0))
_When_(Status >= 0, _Out_range_(==, 0))
NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosError(
    _In_ NTSTATUS Status
    );

_When_(Status < 0, _Out_range_(>, 0))
_When_(Status >= 0, _Out_range_(==, 0))
NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosErrorNoTeb(
    _In_ NTSTATUS Status
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetLastNtStatus(
    VOID
    );

NTSYSAPI
LONG
NTAPI
RtlGetLastWin32Error(
    VOID
    );

NTSYSAPI
VOID
NTAPI
RtlSetLastWin32ErrorAndNtStatusFromNtStatus(
    _In_ NTSTATUS Status
    );

NTSYSAPI
VOID
NTAPI
RtlSetLastWin32Error(
    _In_ LONG Win32Error
    );

NTSYSAPI
VOID
NTAPI
RtlRestoreLastWin32Error(
    _In_ LONG Win32Error
    );

#define RTL_ERRORMODE_FAILCRITICALERRORS 0x0010
#define RTL_ERRORMODE_NOGPFAULTERRORBOX 0x0020
#define RTL_ERRORMODE_NOOPENFILEERRORBOX 0x0040

NTSYSAPI
ULONG
NTAPI
RtlGetThreadErrorMode(
    VOID
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetThreadErrorMode(
    _In_ ULONG NewMode,
    _Out_opt_ PULONG OldMode
    );

//
// Windows Error Reporting
//

NTSYSAPI
NTSTATUS
NTAPI
RtlReportException(
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PCONTEXT ContextRecord,
    _In_ ULONG Flags
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlReportExceptionEx(
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PCONTEXT ContextRecord,
    _In_ ULONG Flags,
    _In_ PLARGE_INTEGER Timeout
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS1

NTSYSAPI
NTSTATUS
NTAPI
RtlWerpReportException(
    _In_ ULONG ProcessId,
    _In_ HANDLE CrashReportSharedMem,
    _In_ ULONG Flags,
    _Out_ PHANDLE CrashVerticalProcessHandle
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlReportSilentProcessExit(
    _In_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus
    );

//
// Random
//

NTSYSAPI
ULONG
NTAPI
RtlUniform(
    _Inout_ PULONG Seed
    );

_Ret_range_(<=, MAXLONG)
NTSYSAPI
ULONG
NTAPI
RtlRandom(
    _Inout_ PULONG Seed
    );

_Ret_range_(<=, MAXLONG)
NTSYSAPI
ULONG
NTAPI
RtlRandomEx(
    _Inout_ PULONG Seed
    );

#define RTL_IMPORT_TABLE_HASH_REVISION 1

NTSYSAPI
NTSTATUS
NTAPI
RtlComputeImportTableHash(
    _In_ HANDLE FileHandle,
    _Out_writes_bytes_(16) PUCHAR Hash,
    _In_ ULONG ImportTableHashRevision // must be 1
    );

//
// Integer conversion
//

NTSYSAPI
NTSTATUS
NTAPI
RtlIntegerToChar(
    _In_ ULONG Value,
    _In_opt_ ULONG Base,
    _In_ LONG OutputLength, // negative to pad to width
    _Out_ PSTR String
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCharToInteger(
    _In_z_ PCSTR String,
    _In_opt_ ULONG Base,
    _Out_ PULONG Value
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlLargeIntegerToChar(
    _In_ PLARGE_INTEGER Value,
    _In_opt_ ULONG Base,
    _In_ LONG OutputLength,
    _Out_ PSTR String
    );

#define RtlLargeIntegerGreaterThan(X,Y) ((((X).HighPart == (Y).HighPart) && ((X).LowPart > (Y).LowPart)) || ((X).HighPart > (Y).HighPart))
#define RtlLargeIntegerGreaterThanOrEqualTo(X,Y) ((((X).HighPart == (Y).HighPart) && ((X).LowPart >= (Y).LowPart)) || ((X).HighPart > (Y).HighPart)))
#define RtlLargeIntegerEqualTo(X,Y) (!(((X).LowPart ^ (Y).LowPart) | ((X).HighPart ^ (Y).HighPart)))
#define RtlLargeIntegerNotEqualTo(X,Y) ((((X).LowPart ^ (Y).LowPart) | ((X).HighPart ^ (Y).HighPart)))
#define RtlLargeIntegerLessThan(X,Y) ((((X).HighPart == (Y).HighPart) && ((X).LowPart < (Y).LowPart)) || ((X).HighPart < (Y).HighPart))
#define RtlLargeIntegerLessThanOrEqualTo(X,Y) ((((X).HighPart == (Y).HighPart) && ((X).LowPart <= (Y).LowPart)) || ((X).HighPart < (Y).HighPart))
#define RtlLargeIntegerGreaterThanZero(X) ((((X).HighPart == 0) && ((X).LowPart > 0)) || ((X).HighPart > 0 ))
#define RtlLargeIntegerGreaterOrEqualToZero(X) ((X).HighPart >= 0)
#define RtlLargeIntegerEqualToZero(X) (!((X).LowPart | (X).HighPart))
#define RtlLargeIntegerNotEqualToZero(X) (((X).LowPart | (X).HighPart))
#define RtlLargeIntegerLessThanZero(X) (((X).HighPart < 0))
#define RtlLargeIntegerLessOrEqualToZero(X) (((X).HighPart < 0) || !((X).LowPart | (X).HighPart))

NTSYSAPI
NTSTATUS
NTAPI
RtlIntegerToUnicodeString(
    _In_ ULONG Value,
    _In_opt_ ULONG Base,
    _Inout_ PUNICODE_STRING String
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlInt64ToUnicodeString(
    _In_ ULONGLONG Value,
    _In_opt_ ULONG Base,
    _Inout_ PUNICODE_STRING String
    );

#ifdef _WIN64
#define RtlIntPtrToUnicodeString(Value, Base, String) RtlInt64ToUnicodeString(Value, Base, String)
#else
#define RtlIntPtrToUnicodeString(Value, Base, String) RtlIntegerToUnicodeString(Value, Base, String)
#endif // _WIN64

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToInteger(
    _In_ PCUNICODE_STRING String,
    _In_opt_ ULONG Base,
    _Out_ PULONG Value
    );

//
// IPv4/6 conversion
//

#ifndef s_addr
//
// IPv4 Internet address
// This is an 'on-wire' format structure.
//
typedef struct in_addr
{
    union
    {
        struct { UCHAR s_b1, s_b2, s_b3, s_b4; } S_un_b;
        struct { USHORT s_w1, s_w2; } S_un_w;
        ULONG S_addr;
    } S_un;
#define s_addr  S_un.S_addr /* can be used for most tcp & ip code */
#define s_host  S_un.S_un_b.s_b2    // host on imp
#define s_net   S_un.S_un_b.s_b1    // network
#define s_imp   S_un.S_un_w.s_w2    // imp
#define s_impno S_un.S_un_b.s_b4    // imp #
#define s_lh    S_un.S_un_b.s_b3    // logical host
} IN_ADDR, * PIN_ADDR, FAR* LPIN_ADDR;
#endif // s_addr

#ifndef s6_addr
//
// IPv6 Internet address (RFC 2553)
// This is an 'on-wire' format structure.
//
typedef struct in6_addr
{
    union
    {
        UCHAR Byte[16];
        USHORT Word[8];
    } u;
#define in_addr6 in6_addr
#define _S6_un   u
#define _S6_u8   Byte
#define s6_addr  _S6_un._S6_u8
#define s6_bytes u.Byte
#define s6_words u.Word
} IN6_ADDR, *PIN6_ADDR, FAR *LPIN6_ADDR;
#endif // s6_addr

typedef struct in_addr IN_ADDR, *PIN_ADDR;
typedef struct in6_addr IN6_ADDR, *PIN6_ADDR;
typedef IN_ADDR const *PCIN_ADDR;
typedef IN6_ADDR const *PCIN6_ADDR;

NTSYSAPI
PWSTR
NTAPI
RtlIpv4AddressToStringW(
    _In_ PCIN_ADDR Address,
    _Out_writes_(16) PWSTR AddressString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlIpv4AddressToStringExW(
    _In_ PCIN_ADDR Address,
    _In_ USHORT Port,
    _Out_writes_to_(*AddressStringLength, *AddressStringLength) PWSTR AddressString,
    _Inout_ PULONG AddressStringLength
    );

NTSYSAPI
PWSTR
NTAPI
RtlIpv6AddressToStringW(
    _In_ PCIN6_ADDR Address,
    _Out_writes_(46) PWSTR AddressString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlIpv6AddressToStringExW(
    _In_ PCIN6_ADDR Address,
    _In_ ULONG ScopeId,
    _In_ USHORT Port,
    _Out_writes_to_(*AddressStringLength, *AddressStringLength) PWSTR AddressString,
    _Inout_ PULONG AddressStringLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlIpv4StringToAddressW(
    _In_ PCWSTR AddressString,
    _In_ BOOLEAN Strict,
    _Out_ LPCWSTR *Terminator,
    _Out_ PIN_ADDR Address
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlIpv4StringToAddressExW(
    _In_ PCWSTR AddressString,
    _In_ BOOLEAN Strict,
    _Out_ PIN_ADDR Address,
    _Out_ PUSHORT Port
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlIpv6StringToAddressW(
    _In_ PCWSTR AddressString,
    _Out_ PCWSTR *Terminator,
    _Out_ PIN6_ADDR Address
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlIpv6StringToAddressExW(
    _In_ PCWSTR AddressString,
    _Out_ PIN6_ADDR Address,
    _Out_ PULONG ScopeId,
    _Out_ PUSHORT Port
    );

#define RtlIpv4AddressToString RtlIpv4AddressToStringW
#define RtlIpv4AddressToStringEx RtlIpv4AddressToStringExW
#define RtlIpv6AddressToString RtlIpv6AddressToStringW
#define RtlIpv6AddressToStringEx RtlIpv6AddressToStringExW
#define RtlIpv4StringToAddress RtlIpv4StringToAddressW
#define RtlIpv4StringToAddressEx RtlIpv4StringToAddressExW
#define RtlIpv6StringToAddress RtlIpv6StringToAddressW
#define RtlIpv6StringToAddressEx RtlIpv6StringToAddressExW

//
// Time
//

typedef struct _TIME_FIELDS
{
    CSHORT Year; // 1601...
    CSHORT Month; // 1..12
    CSHORT Day; // 1..31
    CSHORT Hour; // 0..23
    CSHORT Minute; // 0..59
    CSHORT Second; // 0..59
    CSHORT Milliseconds; // 0..999
    CSHORT Weekday; // 0..6 = Sunday..Saturday
} TIME_FIELDS, *PTIME_FIELDS;

NTSYSAPI
BOOLEAN
NTAPI
RtlCutoverTimeToSystemTime(
    _In_ PTIME_FIELDS CutoverTime,
    _Out_ PLARGE_INTEGER SystemTime,
    _In_ PLARGE_INTEGER CurrentSystemTime,
    _In_ BOOLEAN ThisYear
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSystemTimeToLocalTime(
    _In_ PLARGE_INTEGER SystemTime,
    _Out_ PLARGE_INTEGER LocalTime
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlLocalTimeToSystemTime(
    _In_ PLARGE_INTEGER LocalTime,
    _Out_ PLARGE_INTEGER SystemTime
    );

NTSYSAPI
VOID
NTAPI
RtlTimeToElapsedTimeFields(
    _In_ PLARGE_INTEGER Time,
    _Out_ PTIME_FIELDS TimeFields
    );

NTSYSAPI
VOID
NTAPI
RtlTimeToTimeFields(
    _In_ PLARGE_INTEGER Time,
    _Out_ PTIME_FIELDS TimeFields
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlTimeFieldsToTime(
    _In_ PTIME_FIELDS TimeFields, // Weekday is ignored
    _Out_ PLARGE_INTEGER Time
    );

#define SecondsToStartOf1980 11960006400
#define SecondsToStartOf1970 11644473600

NTSYSAPI
BOOLEAN
NTAPI
RtlTimeToSecondsSince1980(
    _In_ PLARGE_INTEGER Time,
    _Out_ PULONG ElapsedSeconds
    );

NTSYSAPI
VOID
NTAPI
RtlSecondsSince1980ToTime(
    _In_ ULONG ElapsedSeconds,
    _Out_ PLARGE_INTEGER Time
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlTimeToSecondsSince1970(
    _In_ PLARGE_INTEGER Time,
    _Out_ PULONG ElapsedSeconds
    );

NTSYSAPI
VOID
NTAPI
RtlSecondsSince1970ToTime(
    _In_ ULONG ElapsedSeconds,
    _Out_ PLARGE_INTEGER Time
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
NTSYSAPI
ULONGLONG
NTAPI
RtlGetSystemTimePrecise(
    VOID
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8

#if (PHNT_VERSION >= PHNT_WINDOWS_10_21H2)
NTSYSAPI
KSYSTEM_TIME
NTAPI
RtlGetSystemTimeAndBias(
    _Out_ KSYSTEM_TIME TimeZoneBias,
    _Out_opt_ PLARGE_INTEGER TimeZoneBiasEffectiveStart,
    _Out_opt_ PLARGE_INTEGER TimeZoneBiasEffectiveEnd
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_21H2

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
NTSYSAPI
ULONGLONG
NTAPI
RtlGetInterruptTimePrecise(
    _Out_ PLARGE_INTEGER PerformanceCounter
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
NTSYSAPI
ULONGLONG
NTAPI
RtlQueryUnbiasedInterruptTime(
    _Out_ PLARGE_INTEGER InterruptTime
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8

#if (PHNT_VERSION >= PHNT_WINDOWS_11_24H2)
// rev
NTSYSAPI
ULONGLONG
NTAPI
RtlQueryUnbiasedInterruptTimePrecise(
    _Out_ PLARGE_INTEGER InterruptTime
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11_24H2

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
FORCEINLINE
ULONGLONG
NTAPI_INLINE
RtlBeginReadTickLock(
    _In_ PULONGLONG TimeUpdateLock // USER_SHARED_DATA->TimeUpdateLock
    )
{
    ULONGLONG result;

    for (result = *TimeUpdateLock; (*TimeUpdateLock & 1) != 0; result = *TimeUpdateLock)
    {
        YieldProcessor();
    }

    return result;
}
#endif // PHNT_VERSION >= PHNT_WINDOWS_11

//
// Time zones
//

typedef struct _RTL_TIME_ZONE_INFORMATION
{
    LONG Bias;
    WCHAR StandardName[32];
    TIME_FIELDS StandardStart;
    LONG StandardBias;
    WCHAR DaylightName[32];
    TIME_FIELDS DaylightStart;
    LONG DaylightBias;
} RTL_TIME_ZONE_INFORMATION, *PRTL_TIME_ZONE_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryTimeZoneInformation(
    _Out_ PRTL_TIME_ZONE_INFORMATION TimeZoneInformation
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetTimeZoneInformation(
    _In_ PRTL_TIME_ZONE_INFORMATION TimeZoneInformation
    );

//
// Interlocked bit manipulation interfaces
//

#define RtlInterlockedSetBits(Flags, Flag) \
    InterlockedOr((PLONG)(Flags), Flag)

#define RtlInterlockedAndBits(Flags, Flag) \
    InterlockedAnd((PLONG)(Flags), Flag)

#define RtlInterlockedClearBits(Flags, Flag) \
    RtlInterlockedAndBits(Flags, ~(Flag))

#define RtlInterlockedXorBits(Flags, Flag) \
    InterlockedXor(Flags, Flag)

#define RtlInterlockedSetBitsDiscardReturn(Flags, Flag) \
    (VOID) RtlInterlockedSetBits(Flags, Flag)

#define RtlInterlockedAndBitsDiscardReturn(Flags, Flag) \
    (VOID) RtlInterlockedAndBits(Flags, Flag)

#define RtlInterlockedClearBitsDiscardReturn(Flags, Flag) \
    RtlInterlockedAndBitsDiscardReturn(Flags, ~(Flag))

#define RtlInterlockedTestBits(Flags, Flag) \
    ((InterlockedOr((PLONG)(Flags), 0) & (Flag)) == (Flag)) // dmex

//
// Bitmaps
//

typedef struct _RTL_BITMAP
{
    ULONG SizeOfBitMap;
    PULONG Buffer;
} RTL_BITMAP, *PRTL_BITMAP;

NTSYSAPI
VOID
NTAPI
RtlInitializeBitMap(
    _Out_ PRTL_BITMAP BitMapHeader,
    _In_ PULONG BitMapBuffer,
    _In_ ULONG SizeOfBitMap
    );

#if (PHNT_MODE == PHNT_MODE_KERNEL || PHNT_VERSION >= PHNT_WINDOWS_8)
NTSYSAPI
VOID
NTAPI
RtlClearBit(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG BitNumber
    );
#endif // PHNT_MODE == PHNT_MODE_KERNEL || PHNT_VERSION >= PHNT_WINDOWS_8

#if (PHNT_MODE == PHNT_MODE_KERNEL || PHNT_VERSION >= PHNT_WINDOWS_8)
NTSYSAPI
VOID
NTAPI
RtlSetBit(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG BitNumber
    );
#endif // PHNT_MODE == PHNT_MODE_KERNEL || PHNT_VERSION >= PHNT_WINDOWS_8

_Check_return_
NTSYSAPI
BOOLEAN
NTAPI
RtlTestBit(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG BitNumber
    );

NTSYSAPI
VOID
NTAPI
RtlClearAllBits(
    _In_ PRTL_BITMAP BitMapHeader
    );

NTSYSAPI
VOID
NTAPI
RtlSetAllBits(
    _In_ PRTL_BITMAP BitMapHeader
    );

_Success_(return != -1)
_Check_return_
NTSYSAPI
ULONG
NTAPI
RtlFindClearBits(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG NumberToFind,
    _In_ ULONG HintIndex
    );

_Success_(return != -1)
_Check_return_
NTSYSAPI
ULONG
NTAPI
RtlFindSetBits(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG NumberToFind,
    _In_ ULONG HintIndex
    );

_Success_(return != -1)
NTSYSAPI
ULONG
NTAPI
RtlFindClearBitsAndSet(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG NumberToFind,
    _In_ ULONG HintIndex
    );

_Success_(return != -1)
NTSYSAPI
ULONG
NTAPI
RtlFindSetBitsAndClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG NumberToFind,
    _In_ ULONG HintIndex
    );

NTSYSAPI
VOID
NTAPI
RtlClearBits(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(0, BitMapHeader->SizeOfBitMap - NumberToClear) ULONG StartingIndex,
    _In_range_(0, BitMapHeader->SizeOfBitMap - StartingIndex) ULONG NumberToClear
    );

NTSYSAPI
VOID
NTAPI
RtlSetBits(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(0, BitMapHeader->SizeOfBitMap - NumberToSet) ULONG StartingIndex,
    _In_range_(0, BitMapHeader->SizeOfBitMap - StartingIndex) ULONG NumberToSet
    );

NTSYSAPI
CCHAR
NTAPI
RtlFindMostSignificantBit(
    _In_ ULONGLONG Set
    );

NTSYSAPI
CCHAR
NTAPI
RtlFindLeastSignificantBit(
    _In_ ULONGLONG Set
    );

typedef struct _RTL_BITMAP_RUN
{
    ULONG StartingIndex;
    ULONG NumberOfBits;
} RTL_BITMAP_RUN, *PRTL_BITMAP_RUN;

NTSYSAPI
ULONG
NTAPI
RtlFindClearRuns(
    _In_ PRTL_BITMAP BitMapHeader,
    _Out_writes_to_(SizeOfRunArray, return) PRTL_BITMAP_RUN RunArray,
    _In_range_(>, 0) ULONG SizeOfRunArray,
    _In_ BOOLEAN LocateLongestRuns
    );

NTSYSAPI
ULONG
NTAPI
RtlFindLongestRunClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _Out_ PULONG StartingIndex
    );

NTSYSAPI
ULONG
NTAPI
RtlFindFirstRunClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _Out_ PULONG StartingIndex
    );

_Check_return_
FORCEINLINE
BOOLEAN
NTAPI_INLINE
RtlCheckBit(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG BitPosition
    )
{
#ifdef _WIN64
    return BitTest64((LONG64 const *)BitMapHeader->Buffer, (LONG64)BitPosition);
#else
    return (((PLONG)BitMapHeader->Buffer)[BitPosition / 32] >> (BitPosition % 32)) & 0x1;
#endif // _WIN64
}

NTSYSAPI
ULONG
NTAPI
RtlNumberOfClearBits(
    _In_ PRTL_BITMAP BitMapHeader
    );

NTSYSAPI
ULONG
NTAPI
RtlNumberOfSetBits(
    _In_ PRTL_BITMAP BitMapHeader
    );

_Check_return_
NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG StartingIndex,
    _In_ ULONG Length
    );

_Check_return_
NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsSet(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG StartingIndex,
    _In_ ULONG Length
    );

NTSYSAPI
ULONG
NTAPI
RtlFindNextForwardRunClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG FromIndex,
    _Out_ PULONG StartingRunIndex
    );

NTSYSAPI
ULONG
NTAPI
RtlFindLastBackwardRunClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG FromIndex,
    _Out_ PULONG StartingRunIndex
    );

NTSYSAPI
ULONG
NTAPI
RtlNumberOfSetBitsUlongPtr(
    _In_ ULONG_PTR Target
    );

// rev
NTSYSAPI
VOID
NTAPI
RtlInterlockedClearBitRun(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(0, BitMapHeader->SizeOfBitMap - NumberToClear) ULONG StartingIndex,
    _In_range_(0, BitMapHeader->SizeOfBitMap - StartingIndex) ULONG NumberToClear
    );

// rev
NTSYSAPI
VOID
NTAPI
RtlInterlockedSetBitRun(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(0, BitMapHeader->SizeOfBitMap - NumberToSet) ULONG StartingIndex,
    _In_range_(0, BitMapHeader->SizeOfBitMap - StartingIndex) ULONG NumberToSet
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)

NTSYSAPI
VOID
NTAPI
RtlCopyBitMap(
    _In_ PRTL_BITMAP Source,
    _In_ PRTL_BITMAP Destination,
    _In_range_(0, Destination->SizeOfBitMap - 1) ULONG TargetBit
    );

NTSYSAPI
VOID
NTAPI
RtlExtractBitMap(
    _In_ PRTL_BITMAP Source,
    _In_ PRTL_BITMAP Destination,
    _In_range_(0, Source->SizeOfBitMap - 1) ULONG TargetBit,
    _In_range_(0, Source->SizeOfBitMap) ULONG NumberOfBits
    );

NTSYSAPI
ULONG
NTAPI
RtlNumberOfClearBitsInRange(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG StartingIndex,
    _In_ ULONG Length
    );

NTSYSAPI
ULONG
NTAPI
RtlNumberOfSetBitsInRange(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG StartingIndex,
    _In_ ULONG Length
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_8

#if (PHNT_VERSION >= PHNT_WINDOWS_10)

// private
typedef struct _RTL_BITMAP_EX
{
    ULONG64 SizeOfBitMap;
    PULONG64 Buffer;
} RTL_BITMAP_EX, *PRTL_BITMAP_EX;

// rev
NTSYSAPI
VOID
NTAPI
RtlInitializeBitMapEx(
    _Out_ PRTL_BITMAP_EX BitMapHeader,
    _In_ PULONG64 BitMapBuffer,
    _In_ ULONG64 SizeOfBitMap
    );

// rev
_Check_return_
NTSYSAPI
BOOLEAN
NTAPI
RtlTestBitEx(
    _In_ PRTL_BITMAP_EX BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG64 BitNumber
    );

// rev
NTSYSAPI
VOID
NTAPI
RtlClearAllBitsEx(
    _In_ PRTL_BITMAP_EX BitMapHeader
    );

// rev
NTSYSAPI
VOID
NTAPI
RtlClearBitEx(
    _In_ PRTL_BITMAP_EX BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG64 BitNumber
    );

// rev
NTSYSAPI
VOID
NTAPI
RtlSetBitEx(
    _In_ PRTL_BITMAP_EX BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG64 BitNumber
    );

// rev
NTSYSAPI
ULONG64
NTAPI
RtlFindSetBitsEx(
    _In_ PRTL_BITMAP_EX BitMapHeader,
    _In_ ULONG64 NumberToFind,
    _In_ ULONG64 HintIndex
    );

NTSYSAPI
ULONG64
NTAPI
RtlFindSetBitsAndClearEx(
    _In_ PRTL_BITMAP_EX BitMapHeader,
    _In_ ULONG64 NumberToFind,
    _In_ ULONG64 HintIndex
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_10

//
// Handle tables
//

typedef struct _RTL_HANDLE_TABLE_ENTRY
{
    union
    {
        ULONG Flags; // allocated entries have the low bit set
        struct _RTL_HANDLE_TABLE_ENTRY *NextFree;
    };
} RTL_HANDLE_TABLE_ENTRY, *PRTL_HANDLE_TABLE_ENTRY;

#define RTL_HANDLE_ALLOCATED (USHORT)0x0001

typedef struct _RTL_HANDLE_TABLE
{
    ULONG MaximumNumberOfHandles;
    ULONG SizeOfHandleTableEntry;
    ULONG Reserved[2];
    PRTL_HANDLE_TABLE_ENTRY FreeHandles;
    PRTL_HANDLE_TABLE_ENTRY CommittedHandles;
    PRTL_HANDLE_TABLE_ENTRY UnCommittedHandles;
    PRTL_HANDLE_TABLE_ENTRY MaxReservedHandles;
} RTL_HANDLE_TABLE, *PRTL_HANDLE_TABLE;

NTSYSAPI
VOID
NTAPI
RtlInitializeHandleTable(
    _In_ ULONG MaximumNumberOfHandles,
    _In_ ULONG SizeOfHandleTableEntry,
    _Out_ PRTL_HANDLE_TABLE HandleTable
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyHandleTable(
    _Inout_ PRTL_HANDLE_TABLE HandleTable
    );

NTSYSAPI
PRTL_HANDLE_TABLE_ENTRY
NTAPI
RtlAllocateHandle(
    _In_ PRTL_HANDLE_TABLE HandleTable,
    _Out_opt_ PULONG HandleIndex
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlFreeHandle(
    _In_ PRTL_HANDLE_TABLE HandleTable,
    _In_ PRTL_HANDLE_TABLE_ENTRY Handle
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlIsValidHandle(
    _In_ PRTL_HANDLE_TABLE HandleTable,
    _In_ PRTL_HANDLE_TABLE_ENTRY Handle
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlIsValidIndexHandle(
    _In_ PRTL_HANDLE_TABLE HandleTable,
    _In_ ULONG HandleIndex,
    _Out_ PRTL_HANDLE_TABLE_ENTRY *Handle
    );

//
// Atom tables
//

#define RTL_ATOM_MAXIMUM_INTEGER_ATOM (RTL_ATOM)0xc000
#define RTL_ATOM_INVALID_ATOM (RTL_ATOM)0x0000
#define RTL_ATOM_TABLE_DEFAULT_NUMBER_OF_BUCKETS 37
#define RTL_ATOM_MAXIMUM_NAME_LENGTH 255
#define RTL_ATOM_PINNED 0x01

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateAtomTable(
    _In_ ULONG NumberOfBuckets,
    _Out_ PVOID *AtomTableHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyAtomTable(
    _In_ _Post_invalid_ PVOID AtomTableHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlEmptyAtomTable(
    _In_ PVOID AtomTableHandle,
    _In_ BOOLEAN IncludePinnedAtoms
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAtomToAtomTable(
    _In_ PVOID AtomTableHandle,
    _In_ PCWSTR AtomName,
    _Inout_opt_ PRTL_ATOM Atom
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlLookupAtomInAtomTable(
    _In_ PVOID AtomTableHandle,
    _In_ PCWSTR AtomName,
    _Out_opt_ PRTL_ATOM Atom
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteAtomFromAtomTable(
    _In_ PVOID AtomTableHandle,
    _In_ RTL_ATOM Atom
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlPinAtomInAtomTable(
    _In_ PVOID AtomTableHandle,
    _In_ RTL_ATOM Atom
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryAtomInAtomTable(
    _In_ PVOID AtomTableHandle,
    _In_ RTL_ATOM Atom,
    _Out_opt_ PULONG AtomUsage,
    _Out_opt_ PULONG AtomFlags,
    _Inout_updates_bytes_to_opt_(*AtomNameLength, *AtomNameLength) PWSTR AtomName,
    _Inout_opt_ PULONG AtomNameLength
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
RtlGetIntegerAtom(
    _In_ PCWSTR AtomName,
    _Out_opt_ PUSHORT IntegerAtom
    );

//
// SIDs
//

_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlValidSid(
    _In_ PSID Sid
    );

_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualSid(
    _In_ PSID Sid1,
    _In_ PSID Sid2
    );

_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualPrefixSid(
    _In_ PSID Sid1,
    _In_ PSID Sid2
    );

NTSYSAPI
ULONG
NTAPI
RtlLengthRequiredSid(
    _In_ ULONG SubAuthorityCount
    );

NTSYSAPI
PVOID
NTAPI
RtlFreeSid(
    _In_ _Post_invalid_ PSID Sid
    );

_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlAllocateAndInitializeSid(
    _In_ PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    _In_ UCHAR SubAuthorityCount,
    _In_ ULONG SubAuthority0,
    _In_ ULONG SubAuthority1,
    _In_ ULONG SubAuthority2,
    _In_ ULONG SubAuthority3,
    _In_ ULONG SubAuthority4,
    _In_ ULONG SubAuthority5,
    _In_ ULONG SubAuthority6,
    _In_ ULONG SubAuthority7,
    _Outptr_ PSID *Sid
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8_1)
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlAllocateAndInitializeSidEx(
    _In_ PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    _In_ UCHAR SubAuthorityCount,
    _In_reads_(SubAuthorityCount) PULONG SubAuthorities,
    _Outptr_ PSID *Sid
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8_1

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeSid(
    _Out_ PSID Sid,
    _In_ PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    _In_ UCHAR SubAuthorityCount
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeSidEx(
    _Out_writes_bytes_(SECURITY_SID_SIZE(SubAuthorityCount)) PSID Sid,
    _In_ PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    _In_ UCHAR SubAuthorityCount,
    ...
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10

NTSYSAPI
PSID_IDENTIFIER_AUTHORITY
NTAPI
RtlIdentifierAuthoritySid(
    _In_ PSID Sid
    );

NTSYSAPI
PULONG
NTAPI
RtlSubAuthoritySid(
    _In_ PSID Sid,
    _In_ ULONG SubAuthority
    );

NTSYSAPI
PUCHAR
NTAPI
RtlSubAuthorityCountSid(
    _In_ PSID Sid
    );

NTSYSAPI
ULONG
NTAPI
RtlLengthSid(
    _In_ PSID Sid
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCopySid(
    _In_ ULONG DestinationSidLength,
    _Out_writes_bytes_(DestinationSidLength) PSID DestinationSid,
    _In_ PSID SourceSid
    );

// ros
NTSYSAPI
NTSTATUS
NTAPI
RtlCopySidAndAttributesArray(
    _In_ ULONG Count,
    _In_ PSID_AND_ATTRIBUTES Src,
    _In_ ULONG SidAreaSize,
    _In_ PSID_AND_ATTRIBUTES Dest,
    _In_ PSID SidArea,
    _Out_ PSID *RemainingSidArea,
    _Out_ PULONG RemainingSidAreaSize
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateServiceSid(
    _In_ PCUNICODE_STRING ServiceName,
    _Out_writes_bytes_opt_(*ServiceSidLength) PSID ServiceSid,
    _Inout_ PULONG ServiceSidLength
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlSidDominates(
    _In_ PSID Sid1,
    _In_ PSID Sid2,
    _Out_ PBOOLEAN Dominates
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8_1)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlSidDominatesForTrust(
    _In_ PSID Sid1,
    _In_ PSID Sid2,
    _Out_ PBOOLEAN DominatesTrust // TokenProcessTrustLevel
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8_1

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlSidEqualLevel(
    _In_ PSID Sid1,
    _In_ PSID Sid2,
    _Out_ PBOOLEAN EqualLevel
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlSidIsHigherLevel(
    _In_ PSID Sid1,
    _In_ PSID Sid2,
    _Out_ PBOOLEAN HigherLevel
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateVirtualAccountSid(
    _In_ PCUNICODE_STRING Name,
    _In_ ULONG BaseSubAuthority,
    _Out_writes_bytes_(*SidLength) PSID Sid,
    _Inout_ PULONG SidLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlReplaceSidInSd(
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ PSID OldSid,
    _In_ PSID NewSid,
    _Out_ ULONG *NumChanges
    );

#define MAX_UNICODE_STACK_BUFFER_LENGTH 256

NTSYSAPI
NTSTATUS
NTAPI
RtlLengthSidAsUnicodeString(
    _In_ PSID Sid,
    _Out_ PULONG StringLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlConvertSidToUnicodeString(
    _Inout_ PUNICODE_STRING UnicodeString,
    _In_ PSID Sid,
    _In_ BOOLEAN AllocateDestinationString
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlSidHashInitialize(
    _In_reads_(SidCount) PSID_AND_ATTRIBUTES SidAttr,
    _In_ ULONG SidCount,
    _Out_ PSID_AND_ATTRIBUTES_HASH SidAttrHash
    );

// private
NTSYSAPI
PSID_AND_ATTRIBUTES
NTAPI
RtlSidHashLookup(
    _In_ PSID_AND_ATTRIBUTES_HASH SidAttrHash,
    _In_ PSID Sid
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
RtlIsElevatedRid(
    _In_ PSID_AND_ATTRIBUTES SidAttr
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlDeriveCapabilitySidsFromName(
    _Inout_ PUNICODE_STRING UnicodeString,
    _Out_ PSID CapabilityGroupSid,
    _Out_ PSID CapabilitySid
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10

//
// Security Descriptors
//

/**
 * The RtlCreateSecurityDescriptor routine initializes a new absolute-format security descriptor.
 * On return, the security descriptor is initialized with no system ACL, no discretionary ACL, no owner, no primary group, and all control flags set to zero.
 *
 * @param SecurityDescriptor Pointer to the buffer for the \ref SECURITY_DESCRIPTOR to be initialized.
 * @param Revision Specifies the revision level to assign to the security descriptor. Set this parameter to SECURITY_DESCRIPTOR_REVISION.
 * @return NTSTATUS Successful or errant status.
 * @see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlcreatesecuritydescriptor
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateSecurityDescriptor(
    _Out_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ ULONG Revision
    );

/**
 * The RtlValidSecurityDescriptor routine checks a given security descriptor's validity.
 *
 * @param SecurityDescriptor Pointer to the \ref SECURITY_DESCRIPTOR to be checked.
 * @return Returns TRUE if the security descriptor is valid, or FALSE otherwise.
 * @remarks The routine checks the validity of an absolute-format security descriptor. To check the validity of a self-relative security descriptor, use the \ref RtlValidRelativeSecurityDescriptor routine instead.
 * @see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlvalidsecuritydescriptor
 */
_Check_return_
NTSYSAPI
BOOLEAN
NTAPI
RtlValidSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

/**
 * The RtlLengthSecurityDescriptor routine returns the size of a given security descriptor.
 *
 * @param SecurityDescriptor A pointer to a \ref SECURITY_DESCRIPTOR structure whose length the function retrieves.
 * @return Returns the length, in bytes, of the SECURITY_DESCRIPTOR structure.
 * @see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtllengthsecuritydescriptor
 */
NTSYSAPI
ULONG
NTAPI
RtlLengthSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor
    );

/**
 * The RtlValidRelativeSecurityDescriptor routine checks the validity of a self-relative security descriptor.
 *
 * @param SecurityDescriptorInput A pointer to the buffer that contains the security descriptor in self-relative format.
 * The buffer must begin with a SECURITY_DESCRIPTOR structure, which is followed by the rest of the security descriptor data.
 * @param SecurityDescriptorLength The size of the SecurityDescriptorInput structure.
 * @param RequiredInformation A SECURITY_INFORMATION value that specifies the information that is required to be contained in the security descriptor.
 * @return RtlValidRelativeSecurityDescriptor returns TRUE if the security descriptor is valid and includes the information that the RequiredInformation parameter specifies. Otherwise, this routine returns FALSE.
 * @see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlvalidrelativesecuritydescriptor
 */
_Check_return_
NTSYSAPI
BOOLEAN
NTAPI
RtlValidRelativeSecurityDescriptor(
    _In_reads_bytes_(SecurityDescriptorLength) PSECURITY_DESCRIPTOR SecurityDescriptorInput,
    _In_ ULONG SecurityDescriptorLength,
    _In_ SECURITY_INFORMATION RequiredInformation
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetControlSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ PSECURITY_DESCRIPTOR_CONTROL Control,
    _Out_ PULONG Revision
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetControlSecurityDescriptor(
     _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
     _In_ SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
     _In_ SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet
     );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetAttributesSecurityDescriptor(
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ SECURITY_DESCRIPTOR_CONTROL Control,
    _Out_ PULONG Revision
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlGetSecurityDescriptorRMControl(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ PUCHAR RMControl
    );

NTSYSAPI
VOID
NTAPI
RtlSetSecurityDescriptorRMControl(
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PUCHAR RMControl
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetDaclSecurityDescriptor(
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ BOOLEAN DaclPresent,
    _In_opt_ PACL Dacl,
    _In_ BOOLEAN DaclDefaulted
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetDaclSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ PBOOLEAN DaclPresent,
    _Outptr_result_maybenull_ PACL *Dacl,
    _Out_ PBOOLEAN DaclDefaulted
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetSaclSecurityDescriptor(
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ BOOLEAN SaclPresent,
    _In_opt_ PACL Sacl,
    _In_ BOOLEAN SaclDefaulted
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetSaclSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ PBOOLEAN SaclPresent,
    _Out_ PACL *Sacl,
    _Out_ PBOOLEAN SaclDefaulted
    );

/**
 * The RtlSetOwnerSecurityDescriptor routine sets the owner information of an absolute-format security descriptor. It replaces any owner information that is already present in the security descriptor.
 *
 * @param SecurityDescriptor Pointer to the SECURITY_DESCRIPTOR structure whose owner is to be set. RtlSetOwnerSecurityDescriptor replaces any existing owner with the new owner.
 * @param Owner Pointer to a security identifier (SID) structure for the security descriptor's new primary owner.
 * @li \c This pointer, not the SID structure itself, is copied into the security descriptor.
 * @li \c If this parameter is NULL, RtlSetOwnerSecurityDescriptor clears the security descriptor's owner information. This marks the security descriptor as having no owner.
 * @param OwnerDefaulted Set to TRUE if the owner information is derived from a default mechanism.
 * @li \c If this value is TRUE, it is default information. RtlSetOwnerSecurityDescriptor sets the SE_OWNER_DEFAULTED flag in the security descriptor's SECURITY_DESCRIPTOR_CONTROL field.
 * @li \c If this parameter is FALSE, the SE_OWNER_DEFAULTED flag is cleared.
 * @return NTSTATUS Successful or errant status.
 * @see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlsetownersecuritydescriptor
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetOwnerSecurityDescriptor(
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID Owner,
    _In_ BOOLEAN OwnerDefaulted
    );

/**
 * The RtlGetOwnerSecurityDescriptor routine returns the owner information for a given security descriptor.
 *
 * @param SecurityDescriptor Pointer to the SECURITY_DESCRIPTOR structure.
 * @param Owner Pointer to an address to receive a pointer to the owner security identifier (SID). If the security descriptor does not currently contain an owner SID, Owner receives NULL.
 * @param OwnerDefaulted Pointer to a Boolean variable that receives TRUE if the owner information is derived from a default mechanism, FALSE otherwise. Valid only if Owner receives a non-NULL value.
 * @return NTSTATUS Successful or errant status.
 * @see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlgetownersecuritydescriptor
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetOwnerSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Outptr_result_maybenull_ PSID *Owner,
    _Out_ PBOOLEAN OwnerDefaulted
    );

/**
 * The RtlSetGroupSecurityDescriptor routine sets the primary group information of an absolute-format security descriptor. It replaces any primary group information that is already present in the security descriptor.
 *
 * @param SecurityDescriptor Pointer to the SECURITY_DESCRIPTOR structure whose primary group is to be set. RtlSetGroupSecurityDescriptor replaces any existing primary group with the new primary group.
 * @param Group Pointer to a security identifier (SID) structure for the security descriptor's new primary owner.
 * @li \c This pointer, not the SID structure itself, is copied into the security descriptor.
 * @li \c If Group is NULL, RtlSetGroupSecurityDescriptor clears the security descriptor's primary group information. This marks the security descriptor as having no primary group.
 * @param GroupDefaulted Set this Boolean variable to TRUE if the primary group information is derived from a default mechanism.
 * @li \c If this parameter is TRUE, RtlSetGroupSecurityDescriptor sets the SE_GROUP_DEFAULTED flag in the security descriptor's SECURITY_DESCRIPTOR_CONTROL field.
 * @li \c If this parameter is FALSE, RtlSetGroupSecurityDescriptor clears the SE_GROUP_DEFAULTED flag.
 * @return NTSTATUS Successful or errant status.
 * @see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlsetgroupsecuritydescriptor
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetGroupSecurityDescriptor(
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID Group,
    _In_ BOOLEAN GroupDefaulted
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetGroupSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Outptr_result_maybenull_ PSID *Group,
    _Out_ PBOOLEAN GroupDefaulted
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlMakeSelfRelativeSD(
    _In_ PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
    _Out_writes_bytes_(*BufferLength) PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    _Inout_ PULONG BufferLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAbsoluteToSelfRelativeSD(
    _In_ PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
    _Out_writes_bytes_to_opt_(*BufferLength, *BufferLength) PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    _Inout_ PULONG BufferLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSelfRelativeToAbsoluteSD(
    _In_ PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    _Out_writes_bytes_to_opt_(*AbsoluteSecurityDescriptorSize, *AbsoluteSecurityDescriptorSize) PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
    _Inout_ PULONG AbsoluteSecurityDescriptorSize,
    _Out_writes_bytes_to_opt_(*DaclSize, *DaclSize) PACL Dacl,
    _Inout_ PULONG DaclSize,
    _Out_writes_bytes_to_opt_(*SaclSize, *SaclSize) PACL Sacl,
    _Inout_ PULONG SaclSize,
    _Out_writes_bytes_to_opt_(*OwnerSize, *OwnerSize) PSID Owner,
    _Inout_ PULONG OwnerSize,
    _Out_writes_bytes_to_opt_(*PrimaryGroupSize, *PrimaryGroupSize) PSID PrimaryGroup,
    _Inout_ PULONG PrimaryGroupSize
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlSelfRelativeToAbsoluteSD2(
    _Inout_ PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    _Inout_ PULONG BufferSize
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_19H2)
__drv_maxIRQL(APC_LEVEL)
NTSYSAPI
BOOLEAN
NTAPI
RtlNormalizeSecurityDescriptor(
    _Inout_ PSECURITY_DESCRIPTOR *SecurityDescriptor,
    _In_ ULONG SecurityDescriptorLength,
    _Out_opt_ PSECURITY_DESCRIPTOR *NewSecurityDescriptor,
    _Out_opt_ PULONG NewSecurityDescriptorLength,
    _In_ BOOLEAN CheckOnly
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_19H2

//
// Access masks
//

#ifndef PHNT_NO_INLINE_ACCESSES_GRANTED
/**
 * Checks if all desired accesses are granted.
 *
 * This function determines whether all the accesses specified in the DesiredAccess
 * mask are granted by the GrantedAccess mask.
 *
 * @param GrantedAccess The access mask that specifies the granted accesses.
 * @param DesiredAccess The access mask that specifies the desired accesses.
 * @return Returns TRUE if all desired accesses are granted, otherwise FALSE.
 */
FORCEINLINE
BOOLEAN
NTAPI_INLINE
RtlAreAllAccessesGranted(
    _In_ ACCESS_MASK GrantedAccess,
    _In_ ACCESS_MASK DesiredAccess
    )
{
    return (~GrantedAccess & DesiredAccess) == 0;
}

/**
 * Checks if any of the desired accesses are granted.
 *
 * This function determines if any of the access rights specified in the DesiredAccess
 * mask are present in the GrantedAccess mask.
 *
 * @param GrantedAccess The access mask that specifies the granted access rights.
 * @param DesiredAccess The access mask that specifies the desired access rights.
 * @return Returns TRUE if any of the desired access rights are granted, otherwise FALSE.
 */
FORCEINLINE
BOOLEAN
NTAPI_INLINE
RtlAreAnyAccessesGranted(
    _In_ ACCESS_MASK GrantedAccess,
    _In_ ACCESS_MASK DesiredAccess
    )
{
    return (GrantedAccess & DesiredAccess) != 0;
}
#else
/**
 * Checks if all desired accesses are granted.
 *
 * This function determines whether all the accesses specified in the DesiredAccess
 * mask are granted by the GrantedAccess mask.
 *
 * @param GrantedAccess The access mask that specifies the granted accesses.
 * @param DesiredAccess The access mask that specifies the desired accesses.
 * @return Returns TRUE if all desired accesses are granted, otherwise FALSE.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlAreAllAccessesGranted(
    _In_ ACCESS_MASK GrantedAccess,
    _In_ ACCESS_MASK DesiredAccess
    );

/**
 * Checks if any of the desired accesses are granted.
 *
 * This function determines if any of the access rights specified in the DesiredAccess
 * mask are present in the GrantedAccess mask.
 *
 * @param GrantedAccess The access mask that specifies the granted access rights.
 * @param DesiredAccess The access mask that specifies the desired access rights.
 * @return Returns TRUE if any of the desired access rights are granted, otherwise FALSE.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlAreAnyAccessesGranted(
    _In_ ACCESS_MASK GrantedAccess,
    _In_ ACCESS_MASK DesiredAccess
    );
#endif // PHNT_NO_INLINE_ACCESSES_GRANTED

NTSYSAPI
VOID
NTAPI
RtlMapGenericMask(
    _Inout_ PACCESS_MASK AccessMask,
    _In_ PGENERIC_MAPPING GenericMapping
    );

//
// ACLs
//

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateAcl(
    _Out_writes_bytes_(AclLength) PACL Acl,
    _In_ ULONG AclLength,
    _In_ ULONG AclRevision
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlValidAcl(
    _In_ PACL Acl
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryInformationAcl(
    _In_ PACL Acl,
    _Out_writes_bytes_(AclInformationLength) PVOID AclInformation,
    _In_ ULONG AclInformationLength,
    _In_ ACL_INFORMATION_CLASS AclInformationClass
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetInformationAcl(
    _Inout_ PACL Acl,
    _In_reads_bytes_(AclInformationLength) PVOID AclInformation,
    _In_ ULONG AclInformationLength,
    _In_ ACL_INFORMATION_CLASS AclInformationClass
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ULONG StartingAceIndex,
    _In_reads_bytes_(AceListLength) PVOID AceList,
    _In_ ULONG AceListLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceIndex
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetAce(
    _In_ PACL Acl,
    _In_ ULONG AceIndex,
    _Outptr_ PVOID *Ace
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11_24H2)
NTSYSAPI
NTSTATUS
NTAPI
RtlGetAcesBufferSize(
    _In_ PACL Acl,
    _Out_ PULONG AcesBufferSize
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11_24H2

NTSYSAPI
BOOLEAN
NTAPI
RtlFirstFreeAce(
    _In_ PACL Acl,
    _Out_ PVOID *FirstFree
    );

// private
NTSYSAPI
PVOID
NTAPI
RtlFindAceByType(
    _In_ PACL Acl,
    _In_ UCHAR AceType,
    _Out_opt_ PULONG Index
    );

// private
NTSYSAPI
BOOLEAN
NTAPI
RtlOwnerAcesPresent(
    _In_ PACL pAcl
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessAllowedAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ACCESS_MASK AccessMask,
    _In_ PSID Sid
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessAllowedAceEx(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ULONG AceFlags,
    _In_ ACCESS_MASK AccessMask,
    _In_ PSID Sid
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessDeniedAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ACCESS_MASK AccessMask,
    _In_ PSID Sid
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessDeniedAceEx(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ULONG AceFlags,
    _In_ ACCESS_MASK AccessMask,
    _In_ PSID Sid
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAuditAccessAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ACCESS_MASK AccessMask,
    _In_ PSID Sid,
    _In_ BOOLEAN AuditSuccess,
    _In_ BOOLEAN AuditFailure
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAuditAccessAceEx(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ULONG AceFlags,
    _In_ ACCESS_MASK AccessMask,
    _In_ PSID Sid,
    _In_ BOOLEAN AuditSuccess,
    _In_ BOOLEAN AuditFailure
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessAllowedObjectAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ULONG AceFlags,
    _In_ ACCESS_MASK AccessMask,
    _In_opt_ PGUID ObjectTypeGuid,
    _In_opt_ PGUID InheritedObjectTypeGuid,
    _In_ PSID Sid
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessDeniedObjectAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ULONG AceFlags,
    _In_ ACCESS_MASK AccessMask,
    _In_opt_ PGUID ObjectTypeGuid,
    _In_opt_ PGUID InheritedObjectTypeGuid,
    _In_ PSID Sid
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAuditAccessObjectAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ULONG AceFlags,
    _In_ ACCESS_MASK AccessMask,
    _In_opt_ PGUID ObjectTypeGuid,
    _In_opt_ PGUID InheritedObjectTypeGuid,
    _In_ PSID Sid,
    _In_ BOOLEAN AuditSuccess,
    _In_ BOOLEAN AuditFailure
    );

// private
#define COMPOUND_ACE_IMPERSONATION 1

// private
typedef struct _COMPOUND_ACCESS_ALLOWED_ACE
{
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    USHORT CompoundAceType; // COMPOUND_ACE_*
    USHORT Reserved;
    ULONG SidStart; // Server SID
    // Client SID follows
} COMPOUND_ACCESS_ALLOWED_ACE, *PCOMPOUND_ACCESS_ALLOWED_ACE;

NTSYSAPI
NTSTATUS
NTAPI
RtlAddCompoundAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ UCHAR AceType, // COMPOUND_ACE_*
    _In_ ACCESS_MASK AccessMask,
    _In_ PSID ServerSid,
    _In_ PSID ClientSid
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlAddMandatoryAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ULONG AceFlags,
    _In_ PSID Sid,
    _In_ UCHAR AceType,
    _In_ ACCESS_MASK AccessMask
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
NTSYSAPI
NTSTATUS
NTAPI
RtlAddResourceAttributeAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ULONG AceFlags,
    _In_ ULONG AccessMask,
    _In_ PSID Sid,
    _In_ PCLAIM_SECURITY_ATTRIBUTES_INFORMATION AttributeInfo,
    _Out_ PULONG ReturnLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddScopedPolicyIDAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ULONG AceFlags,
    _In_ ULONG AccessMask,
    _In_ PSID Sid
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlAddProcessTrustLabelAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ULONG AceFlags,
    _In_ PSID ProcessTrustLabelSid,
    _In_ UCHAR AceType, // SYSTEM_PROCESS_TRUST_LABEL_ACE_TYPE
    _In_ ACCESS_MASK AccessMask
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8

//
// Named pipes
//

NTSYSAPI
NTSTATUS
NTAPI
RtlDefaultNpAcl(
    _Out_ PACL *Acl
    );

//
// Security objects
//

NTSYSAPI
NTSTATUS
NTAPI
RtlNewSecurityObject(
    _In_opt_ PSECURITY_DESCRIPTOR ParentDescriptor,
    _In_opt_ PSECURITY_DESCRIPTOR CreatorDescriptor,
    _Out_ PSECURITY_DESCRIPTOR *NewDescriptor,
    _In_ BOOLEAN IsDirectoryObject,
    _In_opt_ HANDLE Token,
    _In_ PGENERIC_MAPPING GenericMapping
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlNewSecurityObjectEx(
    _In_opt_ PSECURITY_DESCRIPTOR ParentDescriptor,
    _In_opt_ PSECURITY_DESCRIPTOR CreatorDescriptor,
    _Out_ PSECURITY_DESCRIPTOR *NewDescriptor,
    _In_opt_ GUID *ObjectType,
    _In_ BOOLEAN IsDirectoryObject,
    _In_ ULONG AutoInheritFlags, // SEF_*
    _In_opt_ HANDLE Token,
    _In_ PGENERIC_MAPPING GenericMapping
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlNewSecurityObjectWithMultipleInheritance(
    _In_opt_ PSECURITY_DESCRIPTOR ParentDescriptor,
    _In_opt_ PSECURITY_DESCRIPTOR CreatorDescriptor,
    _Out_ PSECURITY_DESCRIPTOR *NewDescriptor,
    _In_opt_ GUID **ObjectType,
    _In_ ULONG GuidCount,
    _In_ BOOLEAN IsDirectoryObject,
    _In_ ULONG AutoInheritFlags, // SEF_*
    _In_opt_ HANDLE Token,
    _In_ PGENERIC_MAPPING GenericMapping
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteSecurityObject(
    _Inout_ PSECURITY_DESCRIPTOR *ObjectDescriptor
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlQuerySecurityObject(
     _In_ PSECURITY_DESCRIPTOR ObjectDescriptor,
     _In_ SECURITY_INFORMATION SecurityInformation,
     _Out_opt_ PSECURITY_DESCRIPTOR ResultantDescriptor,
     _In_ ULONG DescriptorLength,
     _Out_ PULONG ReturnLength
     );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetSecurityObject(
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR ModificationDescriptor,
    _Inout_ PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_opt_ HANDLE TokenHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetSecurityObjectEx(
    _In_ SECURITY_INFORMATION SecurityInformation,
    _In_ PSECURITY_DESCRIPTOR ModificationDescriptor,
    _Inout_ PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    _In_ ULONG AutoInheritFlags, // SEF_*
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_opt_ HANDLE TokenHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlConvertToAutoInheritSecurityObject(
    _In_opt_ PSECURITY_DESCRIPTOR ParentDescriptor,
    _In_ PSECURITY_DESCRIPTOR CurrentSecurityDescriptor,
    _Out_ PSECURITY_DESCRIPTOR *NewSecurityDescriptor,
    _In_opt_ GUID *ObjectType,
    _In_ BOOLEAN IsDirectoryObject,
    _In_ PGENERIC_MAPPING GenericMapping
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlNewInstanceSecurityObject(
    _In_ BOOLEAN ParentDescriptorChanged,
    _In_ BOOLEAN CreatorDescriptorChanged,
    _In_ PLUID OldClientTokenModifiedId,
    _Out_ PLUID NewClientTokenModifiedId,
    _In_opt_ PSECURITY_DESCRIPTOR ParentDescriptor,
    _In_opt_ PSECURITY_DESCRIPTOR CreatorDescriptor,
    _Out_ PSECURITY_DESCRIPTOR *NewDescriptor,
    _In_ BOOLEAN IsDirectoryObject,
    _In_ HANDLE TokenHandle,
    _In_ PGENERIC_MAPPING GenericMapping
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCopySecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR InputSecurityDescriptor,
    _Out_ PSECURITY_DESCRIPTOR *OutputSecurityDescriptor
    );

// private
typedef struct _RTL_ACE_DATA
{
    UCHAR AceType;
    UCHAR InheritFlags;
    UCHAR AceFlags;
    ACCESS_MASK AccessMask;
    PSID* Sid;
} RTL_ACE_DATA, *PRTL_ACE_DATA;

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateUserSecurityObject(
    _In_ PRTL_ACE_DATA AceData,
    _In_ ULONG AceCount,
    _In_ PSID OwnerSid,
    _In_ PSID GroupSid,
    _In_ BOOLEAN IsDirectoryObject,
    _In_ PGENERIC_MAPPING GenericMapping,
    _Out_ PSECURITY_DESCRIPTOR* NewSecurityDescriptor
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateAndSetSD(
    _In_ PRTL_ACE_DATA AceData,
    _In_ ULONG AceCount,
    _In_opt_ PSID OwnerSid,
    _In_opt_ PSID GroupSid,
    _Out_ PSECURITY_DESCRIPTOR* NewSecurityDescriptor
    );

// Misc. security

NTSYSAPI
VOID
NTAPI
RtlRunEncodeUnicodeString(
    _Inout_ PUCHAR Seed,
    _Inout_ PUNICODE_STRING String
    );

NTSYSAPI
VOID
NTAPI
RtlRunDecodeUnicodeString(
    _In_ UCHAR Seed,
    _Inout_ PUNICODE_STRING String
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlImpersonateSelf(
    _In_ SECURITY_IMPERSONATION_LEVEL ImpersonationLevel
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlImpersonateSelfEx(
    _In_ SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    _In_opt_ ACCESS_MASK AdditionalAccess,
    _Out_opt_ PHANDLE ThreadToken
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAdjustPrivilege(
    _In_ ULONG Privilege,
    _In_ BOOLEAN Enable,
    _In_ BOOLEAN Client,
    _Out_ PBOOLEAN WasEnabled
    );

#define RTL_ACQUIRE_PRIVILEGE_REVERT 0x00000001
#define RTL_ACQUIRE_PRIVILEGE_PROCESS 0x00000002

NTSYSAPI
NTSTATUS
NTAPI
RtlAcquirePrivilege(
    _In_ PULONG Privilege,
    _In_ ULONG NumPriv,
    _In_ ULONG Flags,
    _Out_ PVOID *ReturnedState
    );

NTSYSAPI
VOID
NTAPI
RtlReleasePrivilege(
    _In_ PVOID StatePointer
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlRemovePrivileges(
    _In_ HANDLE TokenHandle,
    _In_ PULONG PrivilegesToKeep,
    _In_ ULONG PrivilegeCount
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlIsUntrustedObject(
    _In_opt_ HANDLE Handle,
    _In_opt_ PVOID Object,
    _Out_ PBOOLEAN IsUntrustedObject
    );

NTSYSAPI
ULONG
NTAPI
RtlQueryValidationRunlevel(
    _In_opt_ PCUNICODE_STRING ComponentName
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_8

//
// Private namespaces
//

// rev
#define BOUNDARY_DESCRIPTOR_ADD_APPCONTAINER_SID 0x0001

_Ret_maybenull_
_Success_(return != NULL)
NTSYSAPI
POBJECT_BOUNDARY_DESCRIPTOR
NTAPI
RtlCreateBoundaryDescriptor(
    _In_ PCUNICODE_STRING Name,
    _In_ ULONG Flags
    );

NTSYSAPI
VOID
NTAPI
RtlDeleteBoundaryDescriptor(
    _In_ _Post_invalid_ POBJECT_BOUNDARY_DESCRIPTOR BoundaryDescriptor
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddSIDToBoundaryDescriptor(
    _Inout_ POBJECT_BOUNDARY_DESCRIPTOR *BoundaryDescriptor,
    _In_ PSID RequiredSid
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlAddIntegrityLabelToBoundaryDescriptor(
    _Inout_ POBJECT_BOUNDARY_DESCRIPTOR *BoundaryDescriptor,
    _In_ PSID IntegrityLabel
    );

//
// Version
//

// rev
typedef struct _RTL_OSVERSIONINFO
{
    ULONG OSVersionInfoSize;
    ULONG MajorVersion;
    ULONG MinorVersion;
    ULONG BuildNumber;
    ULONG PlatformId;
    WCHAR CSDVersion[128];
} RTL_OSVERSIONINFO, *PRTL_OSVERSIONINFO;

// rev
typedef struct _RTL_OSVERSIONINFOEX
{
    ULONG OSVersionInfoSize;
    ULONG MajorVersion;
    ULONG MinorVersion;
    ULONG BuildNumber;
    ULONG PlatformId;
    WCHAR CSDVersion[128];
    USHORT ServicePackMajor;
    USHORT ServicePackMinor;
    USHORT SuiteMask;
    UCHAR ProductType;
    UCHAR Reserved;
} RTL_OSVERSIONINFOEX, *PRTL_OSVERSIONINFOEX;

// rev
typedef struct _RTL_OSVERSIONINFOEX2
{
    ULONG OSVersionInfoSize;
    ULONG MajorVersion;
    ULONG MinorVersion;
    ULONG BuildNumber;
    ULONG PlatformId;
    WCHAR CSDVersion[128];
    USHORT ServicePackMajor;
    USHORT ServicePackMinor;
    USHORT SuiteMask;
    UCHAR ProductType;
    UCHAR Reserved;
    ULONG SuiteMaskEx;
    ULONG Reserved2;
} RTL_OSVERSIONINFOEX2, *PRTL_OSVERSIONINFOEX2;

// rev
typedef struct _RTL_OSVERSIONINFOEX3
{
    ULONG OSVersionInfoSize;
    ULONG MajorVersion;
    ULONG MinorVersion;
    ULONG BuildNumber;
    union
    {
        ULONG PlatformId;
        ULONG QfeNumber;
    };
    union
    {
        WCHAR CSDVersion[128];
        WCHAR LayerAttrib[128];
    };
    USHORT ServicePackMajor;
    USHORT ServicePackMinor;
    USHORT SuiteMask;
    UCHAR ProductType;
    UCHAR Reserved;
    ULONG SuiteMaskEx;
    ULONG Reserved2;
    union
    {
        USHORT RawInput16;
        USHORT LayerNumber : 12;
        USHORT AttribSelector : 4;
    } Input;
    USHORT LayerCount;
    ULONG LayerFlags;
} RTL_OSVERSIONINFOEX3, *PRTL_OSVERSIONINFOEX3;

NTSYSAPI
NTSTATUS
NTAPI
RtlGetVersion(
    _Out_ PVOID VersionInformation
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlVerifyVersionInfo(
    _In_ PVOID VersionInformation,
    _In_ ULONG TypeMask,
    _In_ ULONGLONG ConditionMask
    );

// rev
NTSYSAPI
VOID
NTAPI
RtlGetNtVersionNumbers(
    _Out_opt_ PULONG NtMajorVersion,
    _Out_opt_ PULONG NtMinorVersion,
    _Out_opt_ PULONG NtBuildNumber
    );

//
// System information
//

// rev
NTSYSAPI
ULONG
NTAPI
RtlGetNtGlobalFlags(
    VOID
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
RtlGetNtProductType(
    _Out_ PNT_PRODUCT_TYPE NtProductType
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
// private
NTSYSAPI
ULONG
NTAPI
RtlGetSuiteMask(
    VOID
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS1

//
// Thread pool (old)
//

typedef _Function_class_(WAIT_CALLBACK_ROUTINE)
VOID NTAPI WAIT_CALLBACK_ROUTINE(
    _In_ PVOID Parameter,
    _In_ BOOLEAN TimerOrWaitFired
    );
typedef WAIT_CALLBACK_ROUTINE* PWAIT_CALLBACK_ROUTINE;

#define WT_EXECUTEDEFAULT               0x00000000
#define WT_EXECUTEINIOTHREAD            0x00000001
#define WT_EXECUTEINUITHREAD            0x00000002
#define WT_EXECUTEINWAITTHREAD          0x00000004
#define WT_EXECUTEONLYONCE              0x00000008
#define WT_EXECUTELONGFUNCTION          0x00000010
#define WT_EXECUTEINTIMERTHREAD         0x00000020
#define WT_EXECUTEINPERSISTENTIOTHREAD  0x00000040
#define WT_EXECUTEINPERSISTENTTHREAD    0x00000080
#define WT_TRANSFER_IMPERSONATION       0x00000100

NTSYSAPI
NTSTATUS
NTAPI
RtlRegisterWait(
    _Out_ PHANDLE WaitHandle,
    _In_ HANDLE Handle,
    _In_ PWAIT_CALLBACK_ROUTINE Function,
    _In_opt_ PVOID Context,
    _In_ ULONG Milliseconds,
    _In_ ULONG Flags
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDeregisterWait(
    _In_ HANDLE WaitHandle
    );

#define RTL_WAITER_DEREGISTER_WAIT_FOR_COMPLETION ((HANDLE)(LONG_PTR)-1)

/**
 * Releases all resources used by a wait object.
 *
 * @param WaitHandle The access mask that specifies the granted access rights.
 * @param CompletionEvent Optional completion event for wait callback completion.
 * @remarks RTL_WAITER_DEREGISTER_WAIT_FOR_COMPLETION: blocking wait for wait callback completion.
 * NULL: non-blocking wait for wait callback completion.
 * EventHandle: caller wait for wait callback completion.
 * @return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDeregisterWaitEx(
    _In_ HANDLE WaitHandle,
    _In_opt_ HANDLE CompletionEvent // optional: RTL_WAITER_DEREGISTER_WAIT_FOR_COMPLETION
    );

typedef _Function_class_(RTL_WORK_CALLBACK)
VOID NTAPI RTL_WORK_CALLBACK(
    _In_ PVOID ThreadParameter
    );
typedef RTL_WORK_CALLBACK* PRTL_WORK_CALLBACK;

NTSYSAPI
NTSTATUS
NTAPI
RtlQueueWorkItem(
    _In_ PRTL_WORK_CALLBACK Function,
    _In_opt_ PVOID Context,
    _In_ ULONG Flags
    );

typedef _Function_class_(RTL_OVERLAPPED_COMPLETION_ROUTINE)
VOID NTAPI RTL_OVERLAPPED_COMPLETION_ROUTINE(
    _In_ NTSTATUS StatusCode,
    _In_ PVOID Context1,
    _In_ PVOID Context2
    );
typedef RTL_OVERLAPPED_COMPLETION_ROUTINE* PRTL_OVERLAPPED_COMPLETION_ROUTINE;

NTSYSAPI
NTSTATUS
NTAPI
RtlSetIoCompletionCallback(
    _In_ HANDLE FileHandle,
    _In_ PRTL_OVERLAPPED_COMPLETION_ROUTINE Function,
    _In_ ULONG Flags
    );

typedef _Function_class_(RTL_START_POOL_THREAD)
NTSTATUS NTAPI RTL_START_POOL_THREAD(
    _In_ PUSER_THREAD_START_ROUTINE Function,
    _In_ PVOID Parameter,
    _Out_ PHANDLE ThreadHandle
    );
typedef RTL_START_POOL_THREAD *PRTL_START_POOL_THREAD;

typedef _Function_class_(RTL_EXIT_POOL_THREAD)
NTSTATUS NTAPI RTL_EXIT_POOL_THREAD(
    _In_ NTSTATUS ExitStatus
    );
typedef RTL_EXIT_POOL_THREAD *PRTL_EXIT_POOL_THREAD;

NTSYSAPI
NTSTATUS
NTAPI
RtlSetThreadPoolStartFunc(
    _In_ PRTL_START_POOL_THREAD StartPoolThread,
    _In_ PRTL_EXIT_POOL_THREAD ExitPoolThread
    );

NTSYSAPI
VOID
NTAPI
RtlUserThreadStart(
    _In_ PUSER_THREAD_START_ROUTINE Function,
    _In_ PVOID Parameter
    );

NTSYSAPI
VOID
NTAPI
LdrInitializeThunk(
    _In_ PCONTEXT ContextRecord,
    _In_ PVOID Parameter
    );

//
// Thread execution
//

NTSYSAPI
NTSTATUS
NTAPI
RtlDelayExecution(
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER DelayInterval
    );

//
// Timer support
//

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateTimerQueue(
    _Out_ PHANDLE TimerQueueHandle
    );

typedef _Function_class_(RTL_TIMER_CALLBACK)
VOID NTAPI RTL_TIMER_CALLBACK(
    _In_ PVOID Parameter,
    _In_ BOOLEAN TimerOrWaitFired
    );
typedef RTL_TIMER_CALLBACK *PRTL_TIMER_CALLBACK;

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateTimer(
    _In_ HANDLE TimerQueueHandle,
    _Out_ PHANDLE Handle,
    _In_ PRTL_TIMER_CALLBACK Function,
    _In_opt_ PVOID Context,
    _In_ ULONG DueTime,
    _In_ ULONG Period,
    _In_ ULONG Flags
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetTimer(
    _In_ HANDLE TimerQueueHandle,
    _Out_ PHANDLE Handle,
    _In_ PRTL_TIMER_CALLBACK Function,
    _In_opt_ PVOID Context,
    _In_ ULONG DueTime,
    _In_ ULONG Period,
    _In_ ULONG Flags
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlUpdateTimer(
    _In_ HANDLE TimerQueueHandle,
    _In_ HANDLE TimerHandle,
    _In_ ULONG DueTime,
    _In_ ULONG Period
    );

#define RTL_TIMER_DELETE_WAIT_FOR_COMPLETION ((HANDLE)(LONG_PTR)-1)

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteTimer(
    _In_ HANDLE TimerQueueHandle,
    _In_ HANDLE TimerToCancel,
    _In_opt_ HANDLE Event // optional: RTL_TIMER_DELETE_WAIT_FOR_COMPLETION
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteTimerQueue(
    _In_ HANDLE TimerQueueHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteTimerQueueEx(
    _In_ HANDLE TimerQueueHandle,
    _In_opt_ HANDLE Event
    );

//
// Registry access
//

NTSYSAPI
NTSTATUS
NTAPI
RtlFormatCurrentUserKeyPath(
    _Out_ PUNICODE_STRING CurrentUserKeyPath
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlOpenCurrentUser(
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE CurrentUserKey
    );

#define RTL_REGISTRY_ABSOLUTE 0
#define RTL_REGISTRY_SERVICES 1 // \Registry\Machine\System\CurrentControlSet\Services
#define RTL_REGISTRY_CONTROL 2 // \Registry\Machine\System\CurrentControlSet\Control
#define RTL_REGISTRY_WINDOWS_NT 3 // \Registry\Machine\Software\Microsoft\Windows NT\CurrentVersion
#define RTL_REGISTRY_DEVICEMAP 4 // \Registry\Machine\Hardware\DeviceMap
#define RTL_REGISTRY_USER 5 // \Registry\User\CurrentUser
#define RTL_REGISTRY_MAXIMUM 6
#define RTL_REGISTRY_HANDLE 0x40000000
#define RTL_REGISTRY_OPTIONAL 0x80000000

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateRegistryKey(
    _In_ ULONG RelativeTo,
    _In_ PCWSTR Path
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCheckRegistryKey(
    _In_ ULONG RelativeTo,
    _In_ PCWSTR Path
    );

typedef _Function_class_(RTL_QUERY_REGISTRY_ROUTINE) 
NTSTATUS NTAPI RTL_QUERY_REGISTRY_ROUTINE(
    _In_z_ PCWSTR ValueName,
    _In_ ULONG ValueType,
    _In_ PVOID ValueData,
    _In_ ULONG ValueLength,
    _In_opt_ PVOID Context,
    _In_opt_ PVOID EntryContext
    );
typedef RTL_QUERY_REGISTRY_ROUTINE *PRTL_QUERY_REGISTRY_ROUTINE;

typedef struct _RTL_QUERY_REGISTRY_TABLE
{
    PRTL_QUERY_REGISTRY_ROUTINE QueryRoutine;
    ULONG Flags;
    PWSTR Name;
    PVOID EntryContext;
    ULONG DefaultType;
    PVOID DefaultData;
    ULONG DefaultLength;
} RTL_QUERY_REGISTRY_TABLE, *PRTL_QUERY_REGISTRY_TABLE;

#define RTL_QUERY_REGISTRY_SUBKEY 0x00000001
#define RTL_QUERY_REGISTRY_TOPKEY 0x00000002
#define RTL_QUERY_REGISTRY_REQUIRED 0x00000004
#define RTL_QUERY_REGISTRY_NOVALUE 0x00000008
#define RTL_QUERY_REGISTRY_NOEXPAND 0x00000010
#define RTL_QUERY_REGISTRY_DIRECT 0x00000020
#define RTL_QUERY_REGISTRY_DELETE 0x00000040
#define RTL_QUERY_REGISTRY_NOSTRING 0x00000080 // deprecated
#define RTL_QUERY_REGISTRY_TYPECHECK 0x00000100

#define RTL_QUERY_REGISTRY_TYPECHECK_SHIFT 24
#define RTL_QUERY_REGISTRY_TYPECHECK_MASK (0xff << RTL_QUERY_REGISTRY_TYPECHECK_SHIFT)

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryRegistryValues(
    _In_ ULONG RelativeTo,
    _In_ PCWSTR Path,
    _Inout_ _At_(*(*QueryTable).EntryContext, _Pre_unknown_) PRTL_QUERY_REGISTRY_TABLE QueryTable,
    _In_opt_ PVOID Context,
    _In_opt_ PVOID Environment
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryRegistryValuesEx(
    _In_ ULONG RelativeTo,
    _In_ PCWSTR Path,
    _Inout_ _At_(*(*QueryTable).EntryContext, _Pre_unknown_) PRTL_QUERY_REGISTRY_TABLE QueryTable,
    _In_opt_ PVOID Context,
    _In_opt_ PVOID Environment
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS4)
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryRegistryValueWithFallback(
    _In_opt_ HANDLE PrimaryHandle,
    _In_opt_ HANDLE FallbackHandle,
    _In_ PCUNICODE_STRING ValueName,
    _In_ ULONG ValueLength,
    _Out_opt_ PULONG ValueType,
    _Out_writes_bytes_to_(ValueLength, *ResultLength) PVOID ValueData,
    _Out_range_(<= , ValueLength) PULONG ResultLength
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS4

NTSYSAPI
NTSTATUS
NTAPI
RtlWriteRegistryValue(
    _In_ ULONG RelativeTo,
    _In_ PCWSTR Path,
    _In_z_ PCWSTR ValueName,
    _In_ ULONG ValueType,
    _In_ PVOID ValueData,
    _In_ ULONG ValueLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteRegistryValue(
    _In_ ULONG RelativeTo,
    _In_ PCWSTR Path,
    _In_z_ PCWSTR ValueName
    );

//
// Thread profiling
//

// rev
/**
 * Enables thread profiling on the specified thread.
 *
 * @param ThreadHandle The handle to the thread on which you want to enable profiling. This must be the current thread.
 * @param Flags To receive thread profiling data such as context switch count, set this parameter to THREAD_PROFILING_FLAG_DISPATCH; otherwise, set to 0.
 * @param HardwareCounters To receive hardware performance counter data, set this parameter to a bitmask that identifies the hardware counters to collect.
 * @param PerformanceDataHandle An opaque handle that you use when calling the RtlReadThreadProfilingData and RtlDisableThreadProfiling functions.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-enablethreadprofiling
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlEnableThreadProfiling(
    _In_ HANDLE ThreadHandle,
    _In_ ULONG Flags,
    _In_ ULONG64 HardwareCounters,
    _Out_ PVOID *PerformanceDataHandle
    );

// rev
/**
 * Disables thread profiling.
 *
 * @param PerformanceDataHandle The handle that the RtlEnableThreadProfiling function returned.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-querythreadprofiling
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDisableThreadProfiling(
    _In_ PVOID PerformanceDataHandle
    );

// rev
/**
 * Determines whether thread profiling is enabled for the specified thread.
 *
 * @param ThreadHandle The handle to the thread on which you want to enable profiling. This must be the current thread.
 * @param Enabled Is TRUE if thread profiling is enabled for the specified thread; otherwise, FALSE.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-querythreadprofiling
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryThreadProfiling(
    _In_ HANDLE ThreadHandle,
    _Out_ PBOOLEAN Enabled
    );

// rev
/**
 * Reads the specified profiling data associated with the thread.
 *
 * @param PerformanceDataHandle The handle that the RtlEnableThreadProfiling function returned.
 * @param Flags One or more flags set when you called the RtlEnableThreadProfiling function that specify the counter data to read.
 * @param PerformanceData A PERFORMANCE_DATA structure that contains the thread profiling and hardware counter data.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-readthreadprofilingdata
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlReadThreadProfilingData(
    _In_ HANDLE PerformanceDataHandle,
    _In_ ULONG Flags,
    _Out_ PPERFORMANCE_DATA PerformanceData
    );

//
// WOW64
//

NTSYSAPI
NTSTATUS
NTAPI
RtlGetNativeSystemInformation(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _In_ PVOID NativeSystemInformation,
    _In_ ULONG InformationLength,
    _Out_opt_ PULONG ReturnLength
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
NtWow64GetNativeSystemInformation(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _In_ PVOID NativeSystemInformation,
    _In_ ULONG InformationLength,
    _Out_opt_ PULONG ReturnLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlQueueApcWow64Thread(
    _In_ HANDLE ThreadHandle,
    _In_ PPS_APC_ROUTINE ApcRoutine,
    _In_opt_ PVOID ApcArgument1,
    _In_opt_ PVOID ApcArgument2,
    _In_opt_ PVOID ApcArgument3
    );

/**
 * Enables or disables file system redirection for the calling thread.
 *
 * @param Wow64FsEnableRedirection If TRUE, requests redirection be enabled; if FALSE, requests redirection be disabled.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/wow64apiset/nf-wow64apiset-wow64enablewow64fsredirection
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlWow64EnableFsRedirection(
    _In_ BOOLEAN Wow64FsEnableRedirection
    );

/**
 * Enables or disables file system redirection for the calling thread.
 *
 * @param Wow64FsEnableRedirection If TRUE, requests redirection be enabled; if FALSE, requests redirection be disabled.
 * @param OldFsRedirectionLevel The WOW64 file system redirection value. The system uses this parameter to store information necessary to revert (re-enable) file system redirection.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/wow64apiset/nf-wow64apiset-wow64disablewow64fsredirection
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlWow64EnableFsRedirectionEx(
    _In_ PVOID Wow64FsEnableRedirection,
    _Out_ PVOID *OldFsRedirectionLevel
    );

//
// Misc.
//

NTSYSAPI
ULONG32
NTAPI
RtlComputeCrc32(
    _In_ ULONG32 PartialCrc,
    _In_ PVOID Buffer,
    _In_ ULONG Length
    );

/**
 * Encodes the specified pointer. Encoded pointers can be used to provide another layer of protection for pointer values.
 *
 * @param Ptr The system pointer to be encoded.
 * @return The function returns the encoded pointer.
 * @sa https://learn.microsoft.com/en-us/previous-versions/bb432254(v=vs.85)
 */
_Ret_maybenull_
NTSYSAPI
PVOID
NTAPI
RtlEncodePointer(
    _In_opt_ PVOID Ptr
    );

/**
 * Decodes a pointer that was previously encoded with RtlEncodePointer.
 *
 * @param Ptr The system pointer to be decoded.
 * @return The function returns the decoded pointer.
 * @sa https://learn.microsoft.com/en-us/previous-versions/bb432242(v=vs.85)
 */
_Ret_maybenull_
NTSYSAPI
PVOID
NTAPI
RtlDecodePointer(
    _In_opt_ PVOID Ptr
    );

/**
 * Encodes the specified pointer with a system-specific value. Encoded pointers can be used to provide another layer of protection for pointer values.
 *
 * @param Ptr The system pointer to be encoded.
 * @return The function returns the encoded pointer.
 * @sa https://learn.microsoft.com/en-us/previous-versions/bb432255(v=vs.85)
 */
_Ret_maybenull_
NTSYSAPI
PVOID
NTAPI
RtlEncodeSystemPointer(
    _In_opt_ PVOID Ptr
    );

/**
 * Decodes a pointer that was previously encoded with RtlEncodeSystemPointer.
 *
 * @param Ptr The pointer to be decoded.
 * @return The function returns the decoded pointer.
 * @sa https://learn.microsoft.com/en-us/previous-versions/bb432243(v=vs.85)
 */
_Ret_maybenull_
NTSYSAPI
PVOID
NTAPI
RtlDecodeSystemPointer(
    _In_opt_ PVOID Ptr
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
// rev
/**
 * Encodes the specified pointer of the specified process. Encoded pointers can be used to provide another layer of protection for pointer values.
 *
 * @param ProcessHandle Handle to the remote process that owns the pointer.
 * @param Pointer The pointer to be encoded.
 * @param EncodedPointer The encoded pointer.
 * @return HRESULT Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/previous-versions/dn877135(v=vs.85)
 */
NTSYSAPI
HRESULT
NTAPI
RtlEncodeRemotePointer(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID Pointer,
    _Out_ PVOID *EncodedPointer
    );

// rev
/**
 * Decodes a pointer in a specified process that was previously encoded with RtlEncodePointer or RtlEncodeRemotePointer.
 *
 * @param ProcessHandle Handle to the remote process that owns the pointer.
 * @param Pointer The pointer to be decoded.
 * @param DecodedPointer The decoded pointer.
 * @return HRESULT Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/previous-versions/dn877133(v=vs.85)
 */
NTSYSAPI
HRESULT
NTAPI
RtlDecodeRemotePointer(
    _In_ HANDLE ProcessHandle,
    _In_opt_ PVOID Pointer,
    _Out_ PVOID *DecodedPointer
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
// rev
/**
 * Determines whether the specified processor feature is supported by the current computer.
 *
 * @param ProcessorFeature The processor feature to be tested.
 * @return If the feature is supported, the return value is a nonzero value.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-isprocessorfeaturepresent
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsProcessorFeaturePresent(
    _In_ ULONG ProcessorFeature
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10

// rev
/**
 * Retrieves the number of the processor the current thread was running on during the call to this function.
 *
 * @return The function returns the current processor number.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getcurrentprocessornumber
 */
NTSYSAPI
ULONG
NTAPI
RtlGetCurrentProcessorNumber(
    VOID
    );

// rev
/**
 * Retrieves the processor group and number of the logical processor in which the calling thread is running.
 *
 * @param ProcessorNumber A pointer to a PROCESSOR_NUMBER structure that receives the processor group and number of the logical processor the calling thread is running.
 * @return This function does not return a value.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getcurrentprocessornumberex
 */
NTSYSAPI
VOID
NTAPI
RtlGetCurrentProcessorNumberEx(
    _Out_ PPROCESSOR_NUMBER ProcessorNumber
    );

//
// Stack support
//

NTSYSAPI
VOID
NTAPI
RtlPushFrame(
    _In_ PTEB_ACTIVE_FRAME Frame
    );

NTSYSAPI
VOID
NTAPI
RtlPopFrame(
    _In_ PTEB_ACTIVE_FRAME Frame
    );

NTSYSAPI
PTEB_ACTIVE_FRAME
NTAPI
RtlGetFrame(
    VOID
    );

#define RTL_WALK_USER_MODE_STACK 0x00000001
#define RTL_WALK_KERNEL_STACK 0x00000002
#define RTL_WALK_USER_KERNEL_STACK 0x00000003
#define RTL_WALK_VALID_FLAGS 0x00000006
#define RTL_STACK_WALKING_MODE_FRAMES_TO_SKIP_SHIFT 0x00000008

// private
NTSYSAPI
ULONG
NTAPI
RtlWalkFrameChain(
    _Out_writes_(Count - (Flags >> RTL_STACK_WALKING_MODE_FRAMES_TO_SKIP_SHIFT)) PVOID *Callers,
    _In_ ULONG Count,
    _In_ ULONG Flags
    );

// rev
DECLSPEC_DEPRECATED
NTSYSAPI
VOID
NTAPI
RtlGetCallersAddress( // Use the intrinsic _ReturnAddress instead.
    _Out_ PVOID *CallersAddress,
    _Out_ PVOID *CallersCaller
    );

/**
 * The RtlGetEnabledExtendedFeatures routine returns a mask of extended processor features that are enabled by the system.
 *
 * @param FeatureMask A 64-bit feature mask. This parameter indicates a set of extended processor features for which the caller requests information about whether the features are enabled.
 * @return A 64-bitmask of enabled extended processor features. The routine calculates this mask as the intersection (bitwise AND) between all enabled features and the value of the FeatureMask parameter.
 * @sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlgetenabledextendedfeatures
 */
NTSYSAPI
ULONG64
NTAPI
RtlGetEnabledExtendedFeatures(
    _In_ ULONG64 FeatureMask
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS4)

// msdn
NTSYSAPI
ULONG64
NTAPI
RtlGetEnabledExtendedAndSupervisorFeatures(
    _In_ ULONG64 FeatureMask
    );

// msdn
_Ret_maybenull_
_Success_(return != NULL)
NTSYSAPI
PVOID
NTAPI
RtlLocateSupervisorFeature(
    _In_ PXSAVE_AREA_HEADER XStateHeader,
    _In_range_(XSTATE_AVX, MAXIMUM_XSTATE_FEATURES - 1) ULONG FeatureId,
    _Out_opt_ PULONG Length
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS4

#define ELEVATION_FLAG_TOKEN_CHECKS 0x00000001
#define ELEVATION_FLAG_VIRTUALIZATION 0x00000002
#define ELEVATION_FLAG_SHORTCUT_REDIR 0x00000004
#define ELEVATION_FLAG_NO_SIGNATURE_CHECK 0x00000008

// private
typedef union _RTL_ELEVATION_FLAGS
{
    ULONG Flags;
    struct
    {
        ULONG ElevationEnabled : 1;
        ULONG VirtualizationEnabled : 1;
        ULONG InstallerDetectEnabled : 1;
        ULONG AdminApprovalModeType : 2;
        ULONG ReservedBits : 27;
    };
} RTL_ELEVATION_FLAGS, *PRTL_ELEVATION_FLAGS;

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryElevationFlags(
    _Out_ PRTL_ELEVATION_FLAGS Flags
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlRegisterThreadWithCsrss(
    VOID
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlLockCurrentThread(
    VOID
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlUnlockCurrentThread(
    VOID
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlLockModuleSection(
    _In_ PVOID Address
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlUnlockModuleSection(
    _In_ PVOID Address
    );

#define RTL_UNLOAD_EVENT_TRACE_NUMBER 64

// private
/**
 * The RTL_UNLOAD_EVENT_TRACE structure contains the unloaded module event information.
 *
 * @sa https://learn.microsoft.com/en-us/windows/win32/devnotes/rtlgetunloadeventtrace
 */
typedef struct _RTL_UNLOAD_EVENT_TRACE
{
    PVOID BaseAddress;   // Base address of dll
    SIZE_T SizeOfImage;  // Size of image
    ULONG Sequence;      // Sequence number for this event
    ULONG TimeDateStamp; // Time and date of image
    ULONG CheckSum;      // Image checksum
    WCHAR ImageName[32]; // Image name
    ULONG Version[2];
} RTL_UNLOAD_EVENT_TRACE, *PRTL_UNLOAD_EVENT_TRACE;

typedef struct _RTL_UNLOAD_EVENT_TRACE32
{
    ULONG BaseAddress;   // Base address of dll
    ULONG SizeOfImage;   // Size of image
    ULONG Sequence;      // Sequence number for this event
    ULONG TimeDateStamp; // Time and date of image
    ULONG CheckSum;      // Image checksum
    WCHAR ImageName[32]; // Image name
    ULONG Version[2];
} RTL_UNLOAD_EVENT_TRACE32, *PRTL_UNLOAD_EVENT_TRACE32;

/**
 * Enables the dump code to get the unloaded module information from Ntdll.dll for storage in the minidump.
 *
 * @return A pointer to an array of unload events.
 * @sa https://learn.microsoft.com/en-us/windows/win32/devnotes/rtlgetunloadeventtrace
 */
NTSYSAPI
PRTL_UNLOAD_EVENT_TRACE
NTAPI
RtlGetUnloadEventTrace(
    VOID
    );

/**
 * Retrieves the size and location of the dynamically unloaded module list for the current process.
 *
 * @param ElementSize A pointer to a variable that contains the size of an element in the list.
 * @param ElementCount A pointer to a variable that contains the number of elements in the list.
 * @param EventTrace A pointer to an array of RTL_UNLOAD_EVENT_TRACE structures.
 * @return A pointer to an array of unload events.
 * @sa https://learn.microsoft.com/en-us/windows/win32/devnotes/rtlgetunloadeventtraceex
 */
NTSYSAPI
PRTL_UNLOAD_EVENT_TRACE
NTAPI
RtlGetUnloadEventTraceEx(
    _Out_ PULONG *ElementSize,
    _Out_ PULONG *ElementCount,
    _Out_ PVOID *EventTrace // works across all processes
    );

/**
 * The RtlCaptureStackBackTrace routine captures a stack trace by walking the stack and recording the information for each frame.
 *
 * @param FramesToSkip Number of frames to skip from the start (current call point) of the back trace.
 * @param FramesToCapture Number of frames to be captured.
 * @param BackTrace Caller-allocated array in which pointers to the return addresses captured from the current stack trace are returned.
 * @param BackTraceHash Optional value that can be used to organize hash tables. This hash value is calculated based on the values of the pointers returned in the BackTrace array. Two identical stack traces will generate identical hash values.
 * @return The number of captured frames.
 * @sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlcapturestackbacktrace
 */
_Success_(return != 0)
NTSYSAPI
USHORT
NTAPI
RtlCaptureStackBackTrace(
    _In_ ULONG FramesToSkip,
    _In_ ULONG FramesToCapture,
    _Out_writes_to_(FramesToCapture,return) PVOID* BackTrace,
    _Out_opt_ PULONG BackTraceHash
    );

/**
 * The RtlCaptureContext function retrieves a context record in the context of the caller.
 *
 * @param ContextRecord A pointer to a CONTEXT structure.
 * @return This function does not return a value.
 * @sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlcapturecontext
 */
NTSYSAPI
VOID
NTAPI
RtlCaptureContext(
    _Out_ PCONTEXT ContextRecord
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)
NTSYSAPI
VOID
NTAPI
RtlCaptureContext2(
    _Inout_ PCONTEXT ContextRecord
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_20H1

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
NTSYSAPI
VOID
STDAPIVCALLTYPE
RtlRestoreContext(
    _In_ PCONTEXT ContextRecord,
    _In_opt_ struct _EXCEPTION_RECORD* ExceptionRecord
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11

NTSYSAPI
VOID
NTAPI
RtlUnwind(
    _In_opt_ PVOID TargetFrame,
    _In_opt_ PVOID TargetIp,
    _In_opt_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PVOID ReturnValue
    );

#if defined(_M_AMD64) && defined(_M_ARM64EC)
NTSYSAPI
BOOLEAN
STDAPIVCALLTYPE
RtlAddFunctionTable(
    _In_reads_(EntryCount) PRUNTIME_FUNCTION FunctionTable,
    _In_ ULONG EntryCount,
    _In_ ULONG64 BaseAddress
    );

NTSYSAPI
BOOLEAN
STDAPIVCALLTYPE
RtlDeleteFunctionTable(
    _In_ PRUNTIME_FUNCTION FunctionTable
    );

NTSYSAPI
BOOLEAN
STDAPIVCALLTYPE
RtlInstallFunctionTableCallback(
    _In_ ULONG64 TableIdentifier,
    _In_ ULONG64 BaseAddress,
    _In_ ULONG Length,
    _In_ PGET_RUNTIME_FUNCTION_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _In_opt_ PCWSTR OutOfProcessCallbackDll
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
NTSYSAPI
ULONG
NTAPI
RtlAddGrowableFunctionTable(
    _Out_ PVOID* DynamicTable,
    _In_reads_(MaximumEntryCount) PRUNTIME_FUNCTION FunctionTable,
    _In_ ULONG EntryCount,
    _In_ ULONG MaximumEntryCount,
    _In_ ULONG_PTR RangeBase,
    _In_ ULONG_PTR RangeEnd
    );

NTSYSAPI
VOID
NTAPI
RtlGrowFunctionTable(
    _Inout_ PVOID DynamicTable,
    _In_ ULONG NewEntryCount
    );

NTSYSAPI
VOID
NTAPI
RtlDeleteGrowableFunctionTable(
    _In_ PVOID DynamicTable
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8
#endif // _M_AMD64 && _M_ARM64EC

#if defined(_M_ARM64EC)
NTSYSAPI
BOOLEAN
NTAPI
RtlIsEcCode(
    _In_ ULONG64 CodePointer
    );
#endif // _M_ARM64EC

/**
 * Retrieves the base address of the image that contains the specified PC value.
 *
 * @param PcValue The PC value. The function searches all modules mapped into the address space of the calling process for a module that contains this value.
 * @param BaseOfImage The base address of the image containing the PC value. This value must be added to any relative addresses in the headers to locate the image.
 * @return If the PC value is found, returns the base address of the image that contains the PC value. If no image contains the PC value, the function returns NULL.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/nf-winnt-rtlpctofileheader
 */
NTSYSAPI
PVOID
NTAPI
RtlPcToFileHeader(
    _In_ PVOID PcValue,
    _Out_ PVOID* BaseOfImage
    );

// rev
/**
 * Retrieves the current value of the performance counter, which is a high resolution (<1us) time stamp that can be used for time-interval measurements.
 *
 * @param PerformanceCounter A pointer to a variable that receives the current performance-counter value, in counts.
 * @return Returns TRUE if the function succeeds, otherwise FALSE. On systems that run Windows XP or later, the function will always succeed and will thus never return zero.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancecounter
 */
NTSYSAPI
LOGICAL
NTAPI
RtlQueryPerformanceCounter(
    _Out_ PLARGE_INTEGER PerformanceCounter
    );

// rev
/**
 * Retrieves the frequency of the performance counter. The frequency of the performance counter is fixed at system boot and is consistent across all processors.
 * Therefore, the frequency need only be queried upon application initialization, and the result can be cached.
 *
 * @param PerformanceFrequency A pointer to a variable that receives the current performance-counter frequency, in counts per second. 
 * @return Returns TRUE if the function succeeds, otherwise FALSE. On systems that run Windows XP or later, the function will always succeed and will thus never return zero.
 * @sa https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancefrequency
 */
NTSYSAPI
LOGICAL
NTAPI
RtlQueryPerformanceFrequency(
    _Out_ PLARGE_INTEGER PerformanceFrequency
    );

//
// Image Mitigation
//

// rev
typedef enum _IMAGE_MITIGATION_POLICY
{
    ImageDepPolicy, // RTL_IMAGE_MITIGATION_DEP_POLICY
    ImageAslrPolicy, // RTL_IMAGE_MITIGATION_ASLR_POLICY
    ImageDynamicCodePolicy, // RTL_IMAGE_MITIGATION_DYNAMIC_CODE_POLICY
    ImageStrictHandleCheckPolicy, // RTL_IMAGE_MITIGATION_STRICT_HANDLE_CHECK_POLICY
    ImageSystemCallDisablePolicy, // RTL_IMAGE_MITIGATION_SYSTEM_CALL_DISABLE_POLICY
    ImageMitigationOptionsMask,
    ImageExtensionPointDisablePolicy, // RTL_IMAGE_MITIGATION_EXTENSION_POINT_DISABLE_POLICY
    ImageControlFlowGuardPolicy, // RTL_IMAGE_MITIGATION_CONTROL_FLOW_GUARD_POLICY
    ImageSignaturePolicy, // RTL_IMAGE_MITIGATION_BINARY_SIGNATURE_POLICY
    ImageFontDisablePolicy, // RTL_IMAGE_MITIGATION_FONT_DISABLE_POLICY
    ImageImageLoadPolicy, // RTL_IMAGE_MITIGATION_IMAGE_LOAD_POLICY
    ImagePayloadRestrictionPolicy, // RTL_IMAGE_MITIGATION_PAYLOAD_RESTRICTION_POLICY
    ImageChildProcessPolicy, // RTL_IMAGE_MITIGATION_CHILD_PROCESS_POLICY
    ImageSehopPolicy, // RTL_IMAGE_MITIGATION_SEHOP_POLICY
    ImageHeapPolicy, // RTL_IMAGE_MITIGATION_HEAP_POLICY
    ImageUserShadowStackPolicy, // RTL_IMAGE_MITIGATION_USER_SHADOW_STACK_POLICY
    ImageRedirectionTrustPolicy, // RTL_IMAGE_MITIGATION_REDIRECTION_TRUST_POLICY
    ImageUserPointerAuthPolicy, // RTL_IMAGE_MITIGATION_USER_POINTER_AUTH_POLICY
    MaxImageMitigationPolicy
} IMAGE_MITIGATION_POLICY;

// rev
typedef union _RTL_IMAGE_MITIGATION_POLICY
{
    struct
    {
        ULONG64 AuditState : 2;
        ULONG64 AuditFlag : 1;
        ULONG64 EnableAdditionalAuditingOption : 1;
        ULONG64 Reserved : 60;
    };
    struct
    {
        ULONG64 PolicyState : 2;
        ULONG64 AlwaysInherit : 1;
        ULONG64 EnableAdditionalPolicyOption : 1;
        ULONG64 AuditReserved : 60;
    };
} RTL_IMAGE_MITIGATION_POLICY, *PRTL_IMAGE_MITIGATION_POLICY;

// rev
typedef struct _RTL_IMAGE_MITIGATION_DEP_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY Dep;
} RTL_IMAGE_MITIGATION_DEP_POLICY, *PRTL_IMAGE_MITIGATION_DEP_POLICY;

// rev
typedef struct _RTL_IMAGE_MITIGATION_ASLR_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY ForceRelocateImages;
    RTL_IMAGE_MITIGATION_POLICY BottomUpRandomization;
    RTL_IMAGE_MITIGATION_POLICY HighEntropyRandomization;
} RTL_IMAGE_MITIGATION_ASLR_POLICY, *PRTL_IMAGE_MITIGATION_ASLR_POLICY;

// rev
typedef struct _RTL_IMAGE_MITIGATION_DYNAMIC_CODE_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY BlockDynamicCode;
} RTL_IMAGE_MITIGATION_DYNAMIC_CODE_POLICY, *PRTL_IMAGE_MITIGATION_DYNAMIC_CODE_POLICY;

// rev
typedef struct _RTL_IMAGE_MITIGATION_STRICT_HANDLE_CHECK_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY StrictHandleChecks;
} RTL_IMAGE_MITIGATION_STRICT_HANDLE_CHECK_POLICY, *PRTL_IMAGE_MITIGATION_STRICT_HANDLE_CHECK_POLICY;

// rev
typedef struct _RTL_IMAGE_MITIGATION_SYSTEM_CALL_DISABLE_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY BlockWin32kSystemCalls;
} RTL_IMAGE_MITIGATION_SYSTEM_CALL_DISABLE_POLICY, *PRTL_IMAGE_MITIGATION_SYSTEM_CALL_DISABLE_POLICY;

// rev
typedef struct _RTL_IMAGE_MITIGATION_EXTENSION_POINT_DISABLE_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY DisableExtensionPoints;
} RTL_IMAGE_MITIGATION_EXTENSION_POINT_DISABLE_POLICY, *PRTL_IMAGE_MITIGATION_EXTENSION_POINT_DISABLE_POLICY;

// rev
typedef struct _RTL_IMAGE_MITIGATION_CONTROL_FLOW_GUARD_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY ControlFlowGuard;
    RTL_IMAGE_MITIGATION_POLICY StrictControlFlowGuard;
} RTL_IMAGE_MITIGATION_CONTROL_FLOW_GUARD_POLICY, *PRTL_IMAGE_MITIGATION_CONTROL_FLOW_GUARD_POLICY;

// rev
typedef struct _RTL_IMAGE_MITIGATION_BINARY_SIGNATURE_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY BlockNonMicrosoftSignedBinaries;
    RTL_IMAGE_MITIGATION_POLICY EnforceSigningOnModuleDependencies;
} RTL_IMAGE_MITIGATION_BINARY_SIGNATURE_POLICY, *PRTL_IMAGE_MITIGATION_BINARY_SIGNATURE_POLICY;

// rev
typedef struct _RTL_IMAGE_MITIGATION_FONT_DISABLE_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY DisableNonSystemFonts;
} RTL_IMAGE_MITIGATION_FONT_DISABLE_POLICY, *PRTL_IMAGE_MITIGATION_FONT_DISABLE_POLICY;

// rev
typedef struct _RTL_IMAGE_MITIGATION_IMAGE_LOAD_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY BlockRemoteImageLoads;
    RTL_IMAGE_MITIGATION_POLICY BlockLowLabelImageLoads;
    RTL_IMAGE_MITIGATION_POLICY PreferSystem32;
} RTL_IMAGE_MITIGATION_IMAGE_LOAD_POLICY, *PRTL_IMAGE_MITIGATION_IMAGE_LOAD_POLICY;

// rev
typedef struct _RTL_IMAGE_MITIGATION_PAYLOAD_RESTRICTION_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY EnableExportAddressFilter;
    RTL_IMAGE_MITIGATION_POLICY EnableExportAddressFilterPlus;
    RTL_IMAGE_MITIGATION_POLICY EnableImportAddressFilter;
    RTL_IMAGE_MITIGATION_POLICY EnableRopStackPivot;
    RTL_IMAGE_MITIGATION_POLICY EnableRopCallerCheck;
    RTL_IMAGE_MITIGATION_POLICY EnableRopSimExec;
    WCHAR EafPlusModuleList[512]; // 19H1
} RTL_IMAGE_MITIGATION_PAYLOAD_RESTRICTION_POLICY, *PRTL_IMAGE_MITIGATION_PAYLOAD_RESTRICTION_POLICY;

// rev
typedef struct _RTL_IMAGE_MITIGATION_CHILD_PROCESS_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY DisallowChildProcessCreation;
} RTL_IMAGE_MITIGATION_CHILD_PROCESS_POLICY, *PRTL_IMAGE_MITIGATION_CHILD_PROCESS_POLICY;

// rev
typedef struct _RTL_IMAGE_MITIGATION_SEHOP_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY Sehop;
} RTL_IMAGE_MITIGATION_SEHOP_POLICY, *PRTL_IMAGE_MITIGATION_SEHOP_POLICY;

// rev
typedef struct _RTL_IMAGE_MITIGATION_HEAP_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY TerminateOnHeapErrors;
} RTL_IMAGE_MITIGATION_HEAP_POLICY, *PRTL_IMAGE_MITIGATION_HEAP_POLICY;

// rev
typedef struct _RTL_IMAGE_MITIGATION_USER_SHADOW_STACK_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY UserShadowStack;
    RTL_IMAGE_MITIGATION_POLICY SetContextIpValidation;
    RTL_IMAGE_MITIGATION_POLICY BlockNonCetBinaries;
} RTL_IMAGE_MITIGATION_USER_SHADOW_STACK_POLICY, *PRTL_IMAGE_MITIGATION_USER_SHADOW_STACK_POLICY;

// rev
typedef struct _RTL_IMAGE_MITIGATION_REDIRECTION_TRUST_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY BlockUntrustedRedirections;
} RTL_IMAGE_MITIGATION_REDIRECTION_TRUST_POLICY, *PRTL_IMAGE_MITIGATION_REDIRECTION_TRUST_POLICY;

// rev
typedef struct _RTL_IMAGE_MITIGATION_USER_POINTER_AUTH_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY PointerAuthUserIp;
} RTL_IMAGE_MITIGATION_USER_POINTER_AUTH_POLICY, *PRTL_IMAGE_MITIGATION_USER_POINTER_AUTH_POLICY;

// rev
typedef enum _RTL_IMAGE_MITIGATION_OPTION_STATE
{
    RtlMitigationOptionStateNotConfigured,
    RtlMitigationOptionStateOn,
    RtlMitigationOptionStateOff,
    RtlMitigationOptionStateForce,
    RtlMitigationOptionStateOption
} RTL_IMAGE_MITIGATION_OPTION_STATE;

#define RTL_IMAGE_MITIGATION_OPTION_STATEMASK 3UL
#define RTL_IMAGE_MITIGATION_OPTION_FORCEMASK 4UL
#define RTL_IMAGE_MITIGATION_OPTION_OPTIONMASK 8UL

// rev from PROCESS_MITIGATION_FLAGS
#define RTL_IMAGE_MITIGATION_FLAG_RESET 0x1
#define RTL_IMAGE_MITIGATION_FLAG_REMOVE 0x2
#define RTL_IMAGE_MITIGATION_FLAG_OSDEFAULT 0x4
#define RTL_IMAGE_MITIGATION_FLAG_AUDIT 0x8

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS3)

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryImageMitigationPolicy(
    _In_opt_ PCWSTR ImagePath, // NULL for system-wide defaults
    _In_ IMAGE_MITIGATION_POLICY Policy,
    _In_ ULONG Flags,
    _Inout_ PVOID Buffer,
    _In_ ULONG BufferSize
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlSetImageMitigationPolicy(
    _In_opt_ PCWSTR ImagePath, // NULL for system-wide defaults
    _In_ IMAGE_MITIGATION_POLICY Policy,
    _In_ ULONG Flags,
    _Inout_ PVOID Buffer,
    _In_ ULONG BufferSize
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS3

//
// Session
//

// rev
NTSYSAPI
ULONG
NTAPI
RtlGetCurrentServiceSessionId(
    VOID
    );

// private
NTSYSAPI
ULONG
NTAPI
RtlGetActiveConsoleId(
    VOID
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
// private
NTSYSAPI
LONGLONG
NTAPI
RtlGetConsoleSessionForegroundProcessId(
    VOID
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS1

//
// Appcontainer
//

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS2)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlGetTokenNamedObjectPath(
    _In_ HANDLE TokenHandle,
    _In_opt_ PSID Sid,
    _Out_ PUNICODE_STRING ObjectPath // RtlFreeUnicodeString
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS2

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlGetAppContainerNamedObjectPath(
    _In_opt_ HANDLE TokenHandle,
    _In_opt_ PSID AppContainerSid,
    _In_ BOOLEAN RelativePath,
    _Out_ PUNICODE_STRING ObjectPath // RtlFreeUnicodeString
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8

#if (PHNT_VERSION >= PHNT_WINDOWS_8_1)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlGetAppContainerParent(
    _In_ PSID AppContainerSid,
    _Out_ PSID* AppContainerSidParent // RtlFreeSid
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8_1

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlCheckSandboxedToken(
    _In_opt_ HANDLE TokenHandle,
    _Out_ PBOOLEAN IsSandboxed
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlCheckTokenCapability(
    _In_opt_ HANDLE TokenHandle,
    _In_ PSID CapabilitySidToCheck,
    _Out_ PBOOLEAN HasCapability
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlCapabilityCheck(
    _In_opt_ HANDLE TokenHandle,
    _In_ PCUNICODE_STRING CapabilityName,
    _Out_ PBOOLEAN HasCapability
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlCheckTokenMembership(
    _In_opt_ HANDLE TokenHandle,
    _In_ PSID SidToCheck,
    _Out_ PBOOLEAN IsMember
    );

// RtlCheckTokenMembershipEx Flags
#define CTMF_INCLUDE_APPCONTAINER 0x00000001UL
#define CTMF_INCLUDE_LPAC 0x00000002UL
#define CTMF_VALID_FLAGS (CTMF_INCLUDE_APPCONTAINER | CTMF_INCLUDE_LPAC)

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlCheckTokenMembershipEx(
    _In_opt_ HANDLE TokenHandle,
    _In_ PSID SidToCheck,
    _In_ ULONG Flags, // CTMF_VALID_FLAGS
    _Out_ PBOOLEAN IsMember
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS4)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryTokenHostIdAsUlong64(
    _In_ HANDLE TokenHandle,
    _Out_ PULONG64 HostId // (WIN://PKGHOSTID)
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS4

#if (PHNT_VERSION >= PHNT_WINDOWS_8_1)
// rev
NTSYSAPI
BOOLEAN
NTAPI
RtlIsParentOfChildAppContainer(
    _In_ PSID ParentAppContainerSid,
    _In_ PSID ChildAppContainerSid
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8_1

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlIsApiSetImplemented(
    _In_z_ PCSTR ApiSetName
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// rev
NTSYSAPI
BOOLEAN
NTAPI
RtlIsCapabilitySid(
    _In_ PSID Sid
    );

// rev
NTSYSAPI
BOOLEAN
NTAPI
RtlIsPackageSid(
    _In_ PSID Sid
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8

#if (PHNT_VERSION >= PHNT_WINDOWS_8_1)
// rev
NTSYSAPI
BOOLEAN
NTAPI
RtlIsValidProcessTrustLabelSid(
    _In_ PSID Sid
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8_1

typedef enum _APPCONTAINER_SID_TYPE
{
    NotAppContainerSidType,
    ChildAppContainerSidType,
    ParentAppContainerSidType,
    InvalidAppContainerSidType,
    MaxAppContainerSidType
} APPCONTAINER_SID_TYPE, *PAPPCONTAINER_SID_TYPE;

#if (PHNT_VERSION >= PHNT_WINDOWS_8_1)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlGetAppContainerSidType(
    _In_ PSID AppContainerSid,
    _Out_ PAPPCONTAINER_SID_TYPE AppContainerSidType
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8_1

NTSYSAPI
NTSTATUS
NTAPI
RtlFlsAlloc(
    _In_opt_ PFLS_CALLBACK_FUNCTION Callback,
    _Out_ PULONG FlsIndex
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlFlsAllocEx(
    _In_opt_ PFLS_CALLBACK_FUNCTION Callback,
    _Out_ PULONG,
    _Out_ PULONG FlsIndex
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlFlsFree(
    _In_ ULONG FlsIndex
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlFlsGetValue(
    _In_ ULONG FlsIndex,
    _Out_ PVOID* FlsData
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)
NTSYSAPI
PVOID
NTAPI
RtlFlsGetValue2(
    _In_ ULONG FlsIndex
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_20H1

NTSYSAPI
NTSTATUS
NTAPI
RtlFlsSetValue(
    _In_ ULONG FlsIndex,
    _In_ PVOID FlsData
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)
NTSYSAPI
NTSTATUS
NTAPI
RtlProcessFlsData(
    _In_ HANDLE ProcessHandle,
    _Out_ PVOID* FlsData
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_20H1

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlTlsAlloc(
    _Out_ PULONG TlsIndex
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlTlsFree(
    _In_ ULONG TlsIndex
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlTlsSetValue(
    _In_ ULONG TlsIndex,
    _In_ PVOID TlsData
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11

//
// State isolation
//

typedef enum _STATE_LOCATION_TYPE
{
    LocationTypeRegistry,
    LocationTypeFileSystem,
    LocationTypeMaximum
} STATE_LOCATION_TYPE;

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS3)
// private
NTSYSAPI
BOOLEAN
NTAPI
RtlIsStateSeparationEnabled(
    VOID
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlGetPersistedStateLocation(
    _In_ PCWSTR SourceID,
    _In_opt_ PCWSTR CustomValue,
    _In_opt_ PCWSTR DefaultPath,
    _In_ STATE_LOCATION_TYPE StateLocationType,
    _Out_writes_bytes_to_opt_(BufferLengthIn, *BufferLengthOut) PWCHAR TargetPath,
    _In_ ULONG BufferLengthIn,
    _Out_opt_ PULONG BufferLengthOut
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS3

//
// Cloud Filters
//

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS3)
// msdn
NTSYSAPI
BOOLEAN
NTAPI
RtlIsCloudFilesPlaceholder(
    _In_ ULONG FileAttributes,
    _In_ ULONG ReparseTag
    );

// msdn
NTSYSAPI
BOOLEAN
NTAPI
RtlIsPartialPlaceholder(
    _In_ ULONG FileAttributes,
    _In_ ULONG ReparseTag
    );

// msdn
NTSYSAPI
NTSTATUS
NTAPI
RtlIsPartialPlaceholderFileHandle(
    _In_ HANDLE FileHandle,
    _Out_ PBOOLEAN IsPartialPlaceholder
    );

// msdn
NTSYSAPI
NTSTATUS
NTAPI
RtlIsPartialPlaceholderFileInfo(
    _In_ PVOID InfoBuffer,
    _In_ FILE_INFORMATION_CLASS InfoClass,
    _Out_ PBOOLEAN IsPartialPlaceholder
    );

#undef PHCM_MAX
#define PHCM_APPLICATION_DEFAULT ((CHAR)0)
#define PHCM_DISGUISE_PLACEHOLDERS ((CHAR)1)
#define PHCM_EXPOSE_PLACEHOLDERS ((CHAR)2)
#define PHCM_MAX ((CHAR)2)

#define PHCM_ERROR_INVALID_PARAMETER ((CHAR)-1)
#define PHCM_ERROR_NO_TEB ((CHAR)-2)

NTSYSAPI
CHAR
NTAPI
RtlQueryThreadPlaceholderCompatibilityMode(
    VOID
    );

NTSYSAPI
CHAR
NTAPI
RtlSetThreadPlaceholderCompatibilityMode(
    _In_ CHAR Mode
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS3

#undef PHCM_MAX
#define PHCM_DISGUISE_FULL_PLACEHOLDERS ((CHAR)3)
#define PHCM_MAX ((CHAR)3)
#define PHCM_ERROR_NO_PEB ((CHAR)-3)

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS4)

NTSYSAPI
CHAR
NTAPI
RtlQueryProcessPlaceholderCompatibilityMode(
    VOID
    );

NTSYSAPI
CHAR
NTAPI
RtlSetProcessPlaceholderCompatibilityMode(
    _In_ CHAR Mode
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS4

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS2)
// rev
NTSYSAPI
BOOLEAN
NTAPI
RtlIsNonEmptyDirectoryReparsePointAllowed(
    _In_ ULONG ReparseTag
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS2

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlAppxIsFileOwnedByTrustedInstaller(
    _In_ HANDLE FileHandle,
    _Out_ PBOOLEAN IsFileOwnedByTrustedInstaller
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8

// Windows Internals book
#define PSM_ACTIVATION_TOKEN_PACKAGED_APPLICATION       0x00000001UL // AppX package format
#define PSM_ACTIVATION_TOKEN_SHARED_ENTITY              0x00000002UL // Shared token, multiple binaries in the same package
#define PSM_ACTIVATION_TOKEN_FULL_TRUST                 0x00000004UL // Trusted (Centennial), converted Win32 application
#define PSM_ACTIVATION_TOKEN_NATIVE_SERVICE             0x00000008UL // Packaged service created by SCM
//#define PSM_ACTIVATION_TOKEN_DEVELOPMENT_APP          0x00000010UL
#define PSM_ACTIVATION_TOKEN_MULTIPLE_INSTANCES_ALLOWED 0x00000010UL
#define PSM_ACTIVATION_TOKEN_BREAKAWAY_INHIBITED        0x00000020UL // Cannot create non-packaged child processes
#define PSM_ACTIVATION_TOKEN_RUNTIME_BROKER             0x00000040UL // rev
#define PSM_ACTIVATION_TOKEN_UNIVERSAL_CONSOLE          0x00000200UL // rev
#define PSM_ACTIVATION_TOKEN_WIN32ALACARTE_PROCESS      0x00010000UL // rev

// PackageOrigin appmodel.h
//#define PackageOrigin_Unknown           0
//#define PackageOrigin_Unsigned          1
//#define PackageOrigin_Inbox             2
//#define PackageOrigin_Store             3
//#define PackageOrigin_DeveloperUnsigned 4
//#define PackageOrigin_DeveloperSigned   5
//#define PackageOrigin_LineOfBusiness    6

#define PSMP_MINIMUM_SYSAPP_CLAIM_VALUES 2
#define PSMP_MAXIMUM_SYSAPP_CLAIM_VALUES 4

// private
typedef struct _PS_PKG_CLAIM
{
    ULONG Flags;  // PSM_ACTIVATION_TOKEN_*
    ULONG Origin; // PackageOrigin
} PS_PKG_CLAIM, *PPS_PKG_CLAIM;

// private // WIN://BGKD
typedef enum _PSM_ACTIVATE_BACKGROUND_TYPE
{
  PsmActNotBackground = 0,
  PsmActMixedHost = 1,
  PsmActPureHost = 2,
  PsmActSystemHost = 3,
  PsmActInvalidType = 4,
} PSM_ACTIVATE_BACKGROUND_TYPE;

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryPackageClaims(
    _In_ HANDLE TokenHandle,
    _Out_writes_bytes_to_opt_(*PackageSize, *PackageSize) PWSTR PackageFullName,
    _Inout_opt_ PSIZE_T PackageSize,
    _Out_writes_bytes_to_opt_(*AppIdSize, *AppIdSize) PWSTR AppId,
    _Inout_opt_ PSIZE_T AppIdSize,
    _Out_opt_ PGUID DynamicId,
    _Out_opt_ PPS_PKG_CLAIM PkgClaim,
    _Out_opt_ PULONG64 AttributesPresent
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryPackageIdentity(
    _In_ HANDLE TokenHandle,
    _Out_writes_bytes_to_(*PackageSize, *PackageSize) PWSTR PackageFullName,
    _Inout_ PSIZE_T PackageSize,
    _Out_writes_bytes_to_opt_(*AppIdSize, *AppIdSize) PWSTR AppId,
    _Inout_opt_ PSIZE_T AppIdSize,
    _Out_opt_ PBOOLEAN Packaged
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8

#if (PHNT_VERSION >= PHNT_WINDOWS_8_1)
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryPackageIdentityEx(
    _In_ HANDLE TokenHandle,
    _Out_writes_bytes_to_(*PackageSize, *PackageSize) PWSTR PackageFullName,
    _Inout_ PSIZE_T PackageSize,
    _Out_writes_bytes_to_opt_(*AppIdSize, *AppIdSize) PWSTR AppId,
    _Inout_opt_ PSIZE_T AppIdSize,
    _Out_opt_ PGUID DynamicId,
    _Out_opt_ PULONG64 Flags
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8_1

//
// Protected policies
//

#if (PHNT_VERSION >= PHNT_WINDOWS_8_1)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProtectedPolicy(
    _In_ PGUID PolicyGuid,
    _Out_ PULONG_PTR PolicyValue
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlSetProtectedPolicy(
    _In_ PGUID PolicyGuid,
    _In_ ULONG_PTR PolicyValue,
    _Out_ PULONG_PTR OldPolicyValue
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8_1

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
// rev
NTSYSAPI
BOOLEAN
NTAPI
RtlIsEnclaveFeaturePresent(
    _In_ ULONG FeatureMask
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS1

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
// private
NTSYSAPI
BOOLEAN
NTAPI
RtlIsMultiSessionSku(
    VOID
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
// private
NTSYSAPI
BOOLEAN
NTAPI
RtlIsMultiUsersInSessionSku(
    VOID
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS1

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlGetSessionProperties(
    _In_ ULONG SessionId,
    _Out_ PULONG SharedUserSessionId
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11

// private
typedef enum _RTL_BSD_ITEM_TYPE
{
    RtlBsdItemVersionNumber, // q; s: ULONG
    RtlBsdItemProductType, // q; s: NT_PRODUCT_TYPE (ULONG)
    RtlBsdItemAabEnabled, // q: s: BOOLEAN // AutoAdvancedBoot
    RtlBsdItemAabTimeout, // q: s: UCHAR // AdvancedBootMenuTimeout
    RtlBsdItemBootGood, // q: s: BOOLEAN // LastBootSucceeded
    RtlBsdItemBootShutdown, // q: s: BOOLEAN // LastBootShutdown
    RtlBsdSleepInProgress, // q: s: BOOLEAN // SleepInProgress
    RtlBsdPowerTransition, // q: s: RTL_BSD_DATA_POWER_TRANSITION
    RtlBsdItemBootAttemptCount, // q: s: UCHAR // BootAttemptCount
    RtlBsdItemBootCheckpoint, // q: s: UCHAR // LastBootCheckpoint
    RtlBsdItemBootId, // q; s: ULONG (USER_SHARED_DATA->BootId)
    RtlBsdItemShutdownBootId, // q; s: ULONG
    RtlBsdItemReportedAbnormalShutdownBootId, // q; s: ULONG
    RtlBsdItemErrorInfo, // RTL_BSD_DATA_ERROR_INFO
    RtlBsdItemPowerButtonPressInfo, // RTL_BSD_POWER_BUTTON_PRESS_INFO
    RtlBsdItemChecksum, // q: s: UCHAR
    RtlBsdPowerTransitionExtension,
    RtlBsdItemFeatureConfigurationState, // q; s: ULONG
    RtlBsdItemRevocationListInfo, // 24H2
    RtlBsdItemMax
} RTL_BSD_ITEM_TYPE;

// ros
typedef struct _RTL_BSD_DATA_POWER_TRANSITION
{
    LARGE_INTEGER PowerButtonTimestamp;
    struct
    {
        BOOLEAN SystemRunning : 1;
        BOOLEAN ConnectedStandbyInProgress : 1;
        BOOLEAN UserShutdownInProgress : 1;
        BOOLEAN SystemShutdownInProgress : 1;
        BOOLEAN SleepInProgress : 4;
    } Flags;
    UCHAR ConnectedStandbyScenarioInstanceId;
    UCHAR ConnectedStandbyEntryReason;
    UCHAR ConnectedStandbyExitReason;
    USHORT SystemSleepTransitionCount;
    LARGE_INTEGER LastReferenceTime;
    ULONG LastReferenceTimeChecksum;
    ULONG LastUpdateBootId;
} RTL_BSD_DATA_POWER_TRANSITION, *PRTL_BSD_DATA_POWER_TRANSITION;

// ros
typedef struct _RTL_BSD_DATA_ERROR_INFO
{
    ULONG BootId;
    ULONG RepeatCount;
    ULONG OtherErrorCount;
    ULONG Code;
    ULONG OtherErrorCount2;
} RTL_BSD_DATA_ERROR_INFO, *PRTL_BSD_DATA_ERROR_INFO;

// ros
typedef struct _RTL_BSD_POWER_BUTTON_PRESS_INFO
{
    LARGE_INTEGER LastPressTime;
    ULONG CumulativePressCount;
    USHORT LastPressBootId;
    UCHAR LastPowerWatchdogStage;
    struct
    {
        UCHAR WatchdogArmed : 1;
        UCHAR ShutdownInProgress : 1;
    } Flags;
    LARGE_INTEGER LastReleaseTime;
    ULONG CumulativeReleaseCount;
    USHORT LastReleaseBootId;
    USHORT ErrorCount;
    UCHAR CurrentConnectedStandbyPhase;
    ULONG TransitionLatestCheckpointId;
    ULONG TransitionLatestCheckpointType;
    ULONG TransitionLatestCheckpointSequenceNumber;
} RTL_BSD_POWER_BUTTON_PRESS_INFO, *PRTL_BSD_POWER_BUTTON_PRESS_INFO;

// private
typedef struct _RTL_BSD_ITEM
{
    RTL_BSD_ITEM_TYPE Type;
    PVOID DataBuffer;
    ULONG DataLength;
} RTL_BSD_ITEM, *PRTL_BSD_ITEM;

// ros
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateBootStatusDataFile(
    VOID
    );

// ros
NTSYSAPI
NTSTATUS
NTAPI
RtlLockBootStatusData(
    _Out_ PHANDLE FileHandle
    );

// ros
NTSYSAPI
NTSTATUS
NTAPI
RtlUnlockBootStatusData(
    _In_ HANDLE FileHandle
    );

// ros
NTSYSAPI
NTSTATUS
NTAPI
RtlGetSetBootStatusData(
    _In_ HANDLE FileHandle,
    _In_ BOOLEAN Read,
    _In_ RTL_BSD_ITEM_TYPE DataClass,
    _In_ PVOID Buffer,
    _In_ ULONG BufferSize,
    _Out_opt_ PULONG ReturnLength
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlCheckBootStatusIntegrity(
    _In_ HANDLE FileHandle,
    _Out_ PBOOLEAN Verified
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlRestoreBootStatusDefaults(
    _In_ HANDLE FileHandle
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS1

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS3)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlRestoreSystemBootStatusDefaults(
    VOID
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlGetSystemBootStatus(
    _In_ RTL_BSD_ITEM_TYPE BootStatusInformationClass,
    _Out_ PVOID DataBuffer,
    _In_ ULONG DataLength,
    _Out_opt_ PULONG ReturnLength
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlSetSystemBootStatus(
    _In_ RTL_BSD_ITEM_TYPE BootStatusInformationClass,
    _In_ PVOID DataBuffer,
    _In_ ULONG DataLength,
    _Out_opt_ PULONG ReturnLength
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS3

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlCheckPortableOperatingSystem(
    _Out_ PBOOLEAN IsPortable // VOID
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlSetPortableOperatingSystem(
    _In_ BOOLEAN IsPortable
    );

// rev
NTSYSAPI
ULONG
NTAPI
RtlSetProxiedProcessId(
    _In_ ULONG ProxiedProcessId
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8

NTSYSAPI
NTSTATUS
NTAPI
RtlFindClosestEncodableLength(
    _In_ ULONGLONG SourceLength,
    _Out_ PULONGLONG TargetLength
    );

//
// Memory cache
//

typedef _Function_class_(RTL_SECURE_MEMORY_CACHE_CALLBACK)
NTSTATUS NTAPI RTL_SECURE_MEMORY_CACHE_CALLBACK(
    _In_ PVOID Address,
    _In_ SIZE_T Length
    );
typedef RTL_SECURE_MEMORY_CACHE_CALLBACK *PRTL_SECURE_MEMORY_CACHE_CALLBACK;

// ros
NTSYSAPI
NTSTATUS
NTAPI
RtlRegisterSecureMemoryCacheCallback(
    _In_ PRTL_SECURE_MEMORY_CACHE_CALLBACK Callback
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDeregisterSecureMemoryCacheCallback(
    _In_ PRTL_SECURE_MEMORY_CACHE_CALLBACK Callback
    );

// ros
NTSYSAPI
BOOLEAN
NTAPI
RtlFlushSecureMemoryCache(
    _In_ PVOID MemoryCache,
    _In_opt_ SIZE_T MemoryLength
    );

//
// Feature configuration
//

// private
typedef ULONG RTL_FEATURE_ID;
typedef ULONGLONG RTL_FEATURE_CHANGE_STAMP, *PRTL_FEATURE_CHANGE_STAMP;
typedef UCHAR RTL_FEATURE_VARIANT;
typedef ULONG RTL_FEATURE_VARIANT_PAYLOAD;
typedef PVOID RTL_FEATURE_CONFIGURATION_CHANGE_REGISTRATION, *PRTL_FEATURE_CONFIGURATION_CHANGE_REGISTRATION;

// private
typedef struct _RTL_FEATURE_USAGE_REPORT
{
    ULONG FeatureId;
    USHORT ReportingKind;
    USHORT ReportingOptions;
} RTL_FEATURE_USAGE_REPORT, *PRTL_FEATURE_USAGE_REPORT;

// private
typedef enum _RTL_FEATURE_CONFIGURATION_TYPE
{
    RtlFeatureConfigurationBoot,
    RtlFeatureConfigurationRuntime,
    RtlFeatureConfigurationCount
} RTL_FEATURE_CONFIGURATION_TYPE;

// private
typedef struct _RTL_FEATURE_CONFIGURATION
{
    RTL_FEATURE_ID FeatureId;
    union
    {
        ULONG Flags;
        struct
        {
            ULONG Priority : 4;
            ULONG EnabledState : 2;
            ULONG IsWexpConfiguration : 1;
            ULONG HasSubscriptions : 1;
            ULONG Variant : 6;
            ULONG VariantPayloadKind : 2;
            ULONG Reserved : 16;
        };
    };
    RTL_FEATURE_VARIANT_PAYLOAD VariantPayload;
} RTL_FEATURE_CONFIGURATION, *PRTL_FEATURE_CONFIGURATION;

// private
typedef struct _RTL_FEATURE_CONFIGURATION_TABLE
{
    ULONG FeatureCount;
    _Field_size_(FeatureCount) RTL_FEATURE_CONFIGURATION Features[ANYSIZE_ARRAY];
} RTL_FEATURE_CONFIGURATION_TABLE, *PRTL_FEATURE_CONFIGURATION_TABLE;

// private
typedef enum _RTL_FEATURE_CONFIGURATION_PRIORITY
{
    FeatureConfigurationPriorityImageDefault   = 0,
    FeatureConfigurationPriorityEKB            = 1,
    FeatureConfigurationPrioritySafeguard      = 2,
    FeatureConfigurationPriorityPersistent     = FeatureConfigurationPrioritySafeguard,
    FeatureConfigurationPriorityReserved3      = 3,
    FeatureConfigurationPriorityService        = 4,
    FeatureConfigurationPriorityReserved5      = 5,
    FeatureConfigurationPriorityDynamic        = 6,
    FeatureConfigurationPriorityReserved7      = 7,
    FeatureConfigurationPriorityUser           = 8,
    FeatureConfigurationPrioritySecurity       = 9,
    FeatureConfigurationPriorityUserPolicy     = 10,
    FeatureConfigurationPriorityReserved11     = 11,
    FeatureConfigurationPriorityTest           = 12,
    FeatureConfigurationPriorityReserved13     = 13,
    FeatureConfigurationPriorityReserved14     = 14,
    FeatureConfigurationPriorityImageOverride  = 15,
    FeatureConfigurationPriorityMax            = FeatureConfigurationPriorityImageOverride
} RTL_FEATURE_CONFIGURATION_PRIORITY, *PRTL_FEATURE_CONFIGURATION_PRIORITY;

// private
typedef enum _RTL_FEATURE_ENABLED_STATE
{
    FeatureEnabledStateDefault,
    FeatureEnabledStateDisabled,
    FeatureEnabledStateEnabled
} RTL_FEATURE_ENABLED_STATE;

// private
typedef enum _RTL_FEATURE_ENABLED_STATE_OPTIONS
{
    FeatureEnabledStateOptionsNone,
    FeatureEnabledStateOptionsWexpConfig
} RTL_FEATURE_ENABLED_STATE_OPTIONS, *PRTL_FEATURE_ENABLED_STATE_OPTIONS;

// private
typedef enum _RTL_FEATURE_VARIANT_PAYLOAD_KIND
{
    FeatureVariantPayloadKindNone,
    FeatureVariantPayloadKindResident,
    FeatureVariantPayloadKindExternal
} RTL_FEATURE_VARIANT_PAYLOAD_KIND, *PRTL_FEATURE_VARIANT_PAYLOAD_KIND;

// private
typedef enum _RTL_FEATURE_CONFIGURATION_OPERATION
{
    FeatureConfigurationOperationNone         = 0,
    FeatureConfigurationOperationFeatureState = 1,
    FeatureConfigurationOperationVariantState = 2,
    FeatureConfigurationOperationResetState   = 4
} RTL_FEATURE_CONFIGURATION_OPERATION, *PRTL_FEATURE_CONFIGURATION_OPERATION;

// private
typedef struct _RTL_FEATURE_CONFIGURATION_UPDATE
{
    RTL_FEATURE_ID FeatureId;
    RTL_FEATURE_CONFIGURATION_PRIORITY Priority;
    RTL_FEATURE_ENABLED_STATE EnabledState;
    RTL_FEATURE_ENABLED_STATE_OPTIONS EnabledStateOptions;
    RTL_FEATURE_VARIANT Variant;
    UCHAR Reserved[3];
    RTL_FEATURE_VARIANT_PAYLOAD_KIND VariantPayloadKind;
    RTL_FEATURE_VARIANT_PAYLOAD VariantPayload;
    RTL_FEATURE_CONFIGURATION_OPERATION Operation;
} RTL_FEATURE_CONFIGURATION_UPDATE, *PRTL_FEATURE_CONFIGURATION_UPDATE;

// private
typedef struct _RTL_FEATURE_USAGE_SUBSCRIPTION_TARGET
{
    ULONG Data[2];
} RTL_FEATURE_USAGE_SUBSCRIPTION_TARGET, *PRTL_FEATURE_USAGE_SUBSCRIPTION_TARGET;

// private
typedef struct _RTL_FEATURE_USAGE_DATA
{
    RTL_FEATURE_ID FeatureId;
    USHORT ReportingKind;
    USHORT Reserved;
} RTL_FEATURE_USAGE_DATA, *PRTL_FEATURE_USAGE_DATA;

// private
typedef struct _RTL_FEATURE_USAGE_SUBSCRIPTION_DETAILS
{
    RTL_FEATURE_ID FeatureId;
    USHORT ReportingKind;
    USHORT ReportingOptions;
    RTL_FEATURE_USAGE_SUBSCRIPTION_TARGET ReportingTarget;
} RTL_FEATURE_USAGE_SUBSCRIPTION_DETAILS, *PRTL_FEATURE_USAGE_SUBSCRIPTION_DETAILS;

// private
typedef struct _RTL_FEATURE_USAGE_SUBSCRIPTION_TABLE
{
    ULONG SubscriptionCount;
    _Field_size_(SubscriptionCount) RTL_FEATURE_USAGE_SUBSCRIPTION_DETAILS Subscriptions[ANYSIZE_ARRAY];
} RTL_FEATURE_USAGE_SUBSCRIPTION_TABLE, *PRTL_FEATURE_USAGE_SUBSCRIPTION_TABLE;

// private
typedef _Function_class_(RTL_FEATURE_CONFIGURATION_CHANGE_CALLBACK)
VOID NTAPI RTL_FEATURE_CONFIGURATION_CHANGE_CALLBACK(
    _In_opt_ PVOID Context
    );
typedef RTL_FEATURE_CONFIGURATION_CHANGE_CALLBACK *PRTL_FEATURE_CONFIGURATION_CHANGE_CALLBACK;

// private
typedef struct _SYSTEM_FEATURE_CONFIGURATION_QUERY
{
    RTL_FEATURE_CONFIGURATION_TYPE ConfigurationType;
    RTL_FEATURE_ID FeatureId;
} SYSTEM_FEATURE_CONFIGURATION_QUERY, *PSYSTEM_FEATURE_CONFIGURATION_QUERY;

// private
typedef struct _SYSTEM_FEATURE_CONFIGURATION_INFORMATION
{
    RTL_FEATURE_CHANGE_STAMP ChangeStamp;
    RTL_FEATURE_CONFIGURATION Configuration;
} SYSTEM_FEATURE_CONFIGURATION_INFORMATION, *PSYSTEM_FEATURE_CONFIGURATION_INFORMATION;

// private
typedef enum _SYSTEM_FEATURE_CONFIGURATION_UPDATE_TYPE
{
  SystemFeatureConfigurationUpdateTypeUpdate = 0,
  SystemFeatureConfigurationUpdateTypeOverwrite = 1,
  SystemFeatureConfigurationUpdateTypeCount = 2,
} SYSTEM_FEATURE_CONFIGURATION_UPDATE_TYPE, *PSYSTEM_FEATURE_CONFIGURATION_UPDATE_TYPE;

// private
typedef struct _SYSTEM_FEATURE_CONFIGURATION_UPDATE
{
    SYSTEM_FEATURE_CONFIGURATION_UPDATE_TYPE UpdateType;
    union
    {
        struct
        {
            RTL_FEATURE_CHANGE_STAMP PreviousChangeStamp;
            RTL_FEATURE_CONFIGURATION_TYPE ConfigurationType;
            ULONG UpdateCount;
            _Field_size_(UpdateCount) RTL_FEATURE_CONFIGURATION_UPDATE Updates[ANYSIZE_ARRAY];
        } Update;

        struct
        {
            RTL_FEATURE_CHANGE_STAMP PreviousChangeStamp;
            RTL_FEATURE_CONFIGURATION_TYPE ConfigurationType;
            SIZE_T BufferSize;
            PVOID Buffer;
        } Overwrite;
    };
} SYSTEM_FEATURE_CONFIGURATION_UPDATE, *PSYSTEM_FEATURE_CONFIGURATION_UPDATE;

// private
typedef struct _SYSTEM_FEATURE_CONFIGURATION_SECTIONS_INFORMATION_ENTRY
{
    RTL_FEATURE_CHANGE_STAMP ChangeStamp;
    PVOID Section;
    SIZE_T Size;
} SYSTEM_FEATURE_CONFIGURATION_SECTIONS_INFORMATION_ENTRY, *PSYSTEM_FEATURE_CONFIGURATION_SECTIONS_INFORMATION_ENTRY;

// private
typedef enum _SYSTEM_FEATURE_CONFIGURATION_SECTION_TYPE
{
  SystemFeatureConfigurationSectionTypeBoot = 0,
  SystemFeatureConfigurationSectionTypeRuntime = 1,
  SystemFeatureConfigurationSectionTypeUsageTriggers = 2,
  SystemFeatureConfigurationSectionTypeCount = 3,
} SYSTEM_FEATURE_CONFIGURATION_SECTION_TYPE;

// private
typedef struct _SYSTEM_FEATURE_CONFIGURATION_SECTIONS_REQUEST
{
    RTL_FEATURE_CHANGE_STAMP PreviousChangeStamps[SystemFeatureConfigurationSectionTypeCount];
} SYSTEM_FEATURE_CONFIGURATION_SECTIONS_REQUEST, *PSYSTEM_FEATURE_CONFIGURATION_SECTIONS_REQUEST;

// private
typedef struct _SYSTEM_FEATURE_CONFIGURATION_SECTIONS_INFORMATION
{
    RTL_FEATURE_CHANGE_STAMP OverallChangeStamp;
    SYSTEM_FEATURE_CONFIGURATION_SECTIONS_INFORMATION_ENTRY Descriptors[SystemFeatureConfigurationSectionTypeCount];
} SYSTEM_FEATURE_CONFIGURATION_SECTIONS_INFORMATION, *PSYSTEM_FEATURE_CONFIGURATION_SECTIONS_INFORMATION;

// private
typedef struct _SYSTEM_FEATURE_USAGE_SUBSCRIPTION_DETAILS
{
    RTL_FEATURE_ID FeatureId;
    USHORT ReportingKind;
    USHORT ReportingOptions;
    RTL_FEATURE_USAGE_SUBSCRIPTION_TARGET ReportingTarget;
} SYSTEM_FEATURE_USAGE_SUBSCRIPTION_DETAILS, *PSYSTEM_FEATURE_USAGE_SUBSCRIPTION_DETAILS;

typedef struct _SYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE_ENTRY
{
    ULONG Remove;
    RTL_FEATURE_USAGE_SUBSCRIPTION_DETAILS Details;
} SYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE_ENTRY, *PSYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE_ENTRY;

typedef struct _SYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE
{
    ULONG UpdateCount;
    _Field_size_(UpdateCount) SYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE_ENTRY Updates[ANYSIZE_ARRAY];
} SYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE, *PSYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE;

#if (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlNotifyFeatureUsage(
    _In_ PRTL_FEATURE_USAGE_REPORT FeatureUsageReport
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryFeatureConfiguration(
    _In_ RTL_FEATURE_ID FeatureId,
    _In_ RTL_FEATURE_CONFIGURATION_TYPE ConfigurationType,
    _Out_ PRTL_FEATURE_CHANGE_STAMP ChangeStamp,
    _Out_ PRTL_FEATURE_CONFIGURATION FeatureConfiguration
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlSetFeatureConfigurations(
    _In_opt_ PRTL_FEATURE_CHANGE_STAMP PreviousChangeStamp,
    _In_ RTL_FEATURE_CONFIGURATION_TYPE ConfigurationType,
    _In_reads_(ConfigurationUpdateCount) PRTL_FEATURE_CONFIGURATION_UPDATE ConfigurationUpdates,
    _In_ SIZE_T ConfigurationUpdateCount
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryAllFeatureConfigurations(
    _In_ RTL_FEATURE_CONFIGURATION_TYPE ConfigurationType,
    _Out_opt_ PRTL_FEATURE_CHANGE_STAMP ChangeStamp,
    _Out_writes_(*ConfigurationCount) PRTL_FEATURE_CONFIGURATION Configurations,
    _Inout_ PSIZE_T ConfigurationCount
    );

// private
NTSYSAPI
RTL_FEATURE_CHANGE_STAMP
NTAPI
RtlQueryFeatureConfigurationChangeStamp(
    VOID
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryFeatureUsageNotificationSubscriptions(
    _Out_writes_(*SubscriptionCount) PRTL_FEATURE_USAGE_SUBSCRIPTION_DETAILS Subscriptions,
    _Inout_ PSIZE_T SubscriptionCount
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlRegisterFeatureConfigurationChangeNotification(
    _In_ PRTL_FEATURE_CONFIGURATION_CHANGE_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _In_opt_ PRTL_FEATURE_CHANGE_STAMP ObservedChangeStamp,
    _Out_ PRTL_FEATURE_CONFIGURATION_CHANGE_REGISTRATION RegistrationHandle
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlUnregisterFeatureConfigurationChangeNotification(
    _In_ RTL_FEATURE_CONFIGURATION_CHANGE_REGISTRATION RegistrationHandle
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlSubscribeForFeatureUsageNotification(
    _In_reads_(SubscriptionCount) PRTL_FEATURE_USAGE_SUBSCRIPTION_DETAILS SubscriptionDetails,
    _In_ SIZE_T SubscriptionCount
    );

// private
NTSYSAPI
NTSTATUS
NTAPI
RtlUnsubscribeFromFeatureUsageNotifications(
    _In_reads_(SubscriptionCount) PRTL_FEATURE_USAGE_SUBSCRIPTION_DETAILS SubscriptionDetails,
    _In_ SIZE_T SubscriptionCount
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_20H1

// private
#if (PHNT_VERSION >= PHNT_WINDOWS_11)
NTSYSAPI
NTSTATUS
NTAPI
RtlOverwriteFeatureConfigurationBuffer(
    _In_opt_ PRTL_FEATURE_CHANGE_STAMP PreviousChangeStamp,
    _In_ RTL_FEATURE_CONFIGURATION_TYPE ConfigurationType,
    _In_reads_bytes_opt_(ConfigurationBufferSize) PVOID ConfigurationBuffer,
    _In_ ULONG ConfigurationBufferSize
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlNotifyFeatureToggleUsage(
    _In_ PRTL_FEATURE_USAGE_REPORT FeatureUsageReport,
    _In_ RTL_FEATURE_ID FeatureId,
    _In_ ULONG Flags
    );

//
// Run Once
//

#ifndef _RTL_RUN_ONCE_DEF
#define _RTL_RUN_ONCE_DEF
//
// Run once initializer
//
#define RTL_RUN_ONCE_INIT {0}
//
// Run once flags
//
#define RTL_RUN_ONCE_CHECK_ONLY     0x00000001UL
#define RTL_RUN_ONCE_ASYNC          0x00000002UL
#define RTL_RUN_ONCE_INIT_FAILED    0x00000004UL
//
// The context stored in the run once structure must
// leave the following number of low order bits unused.
//
#define RTL_RUN_ONCE_CTX_RESERVED_BITS 2

typedef union _RTL_RUN_ONCE
{
    PVOID Ptr;
} RTL_RUN_ONCE, *PRTL_RUN_ONCE;
#endif // _RTL_RUN_ONCE_DEF

NTSYSAPI
VOID
NTAPI
RtlRunOnceInitialize(
    _Out_ PRTL_RUN_ONCE RunOnce
    );

typedef _Function_class_(RTL_RUN_ONCE_INIT_FN)
LOGICAL NTAPI RTL_RUN_ONCE_INIT_FN(
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _Inout_opt_ PVOID Parameter,
    _Inout_opt_ PVOID *Context
    );
typedef RTL_RUN_ONCE_INIT_FN *PRTL_RUN_ONCE_INIT_FN;

_Maybe_raises_SEH_exception_
NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceExecuteOnce(
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ __callback PRTL_RUN_ONCE_INIT_FN InitFn,
    _Inout_opt_ PVOID Parameter,
    _Outptr_opt_result_maybenull_ PVOID *Context
    );

_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceBeginInitialize(
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ ULONG Flags,
    _Outptr_opt_result_maybenull_ PVOID *Context
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceComplete(
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ ULONG Flags,
    _In_opt_ PVOID Context
    );

//
// WNF (Windows Notification Facility)
//

#if (PHNT_VERSION >= PHNT_WINDOWS_10)

#define WNF_STATE_KEY 0x41C64E6DA3BC0074

_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualWnfChangeStamps(
    _In_ WNF_CHANGE_STAMP ChangeStamp1,
    _In_ WNF_CHANGE_STAMP ChangeStamp2
    );

_Always_(_Post_satisfies_(return == STATUS_NO_MEMORY || return == STATUS_RETRY || return == STATUS_SUCCESS))
typedef _Function_class_(WNF_USER_CALLBACK)
NTSTATUS NTAPI WNF_USER_CALLBACK(
    _In_ WNF_STATE_NAME StateName,
    _In_ WNF_CHANGE_STAMP ChangeStamp,
    _In_opt_ PWNF_TYPE_ID TypeId,
    _In_opt_ PVOID CallbackContext,
    _In_reads_bytes_opt_(Length) const VOID* Buffer,
    _In_ ULONG Length
    );
typedef WNF_USER_CALLBACK *PWNF_USER_CALLBACK;

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryWnfStateData(
    _Out_ PWNF_CHANGE_STAMP ChangeStamp,
    _In_ WNF_STATE_NAME StateName,
    _In_ PWNF_USER_CALLBACK Callback,
    _In_opt_ PVOID CallbackContext,
    _In_opt_ PWNF_TYPE_ID TypeId
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlPublishWnfStateData(
    _In_ WNF_STATE_NAME StateName,
    _In_opt_ PCWNF_TYPE_ID TypeId,
    _In_reads_bytes_opt_(Length) const VOID* Buffer,
    _In_opt_ ULONG Length,
    _In_opt_ const VOID* ExplicitScope
    );

typedef struct WNF_USER_SUBSCRIPTION *PWNF_USER_SUBSCRIPTION;

#define WNF_CREATE_SERIALIZATION_GROUP_FLAG 0x00000001L

NTSYSAPI
NTSTATUS
NTAPI
RtlSubscribeWnfStateChangeNotification(
    _Out_ PWNF_USER_SUBSCRIPTION* SubscriptionHandle,
    _In_ WNF_STATE_NAME StateName,
    _In_ WNF_CHANGE_STAMP ChangeStamp,
    _In_ PWNF_USER_CALLBACK Callback,
    _In_opt_ PVOID CallbackContext,
    _In_opt_ PCWNF_TYPE_ID TypeId,
    _In_opt_ ULONG SerializationGroup,
    _In_ ULONG Flags
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlUnsubscribeWnfStateChangeNotification(
    _In_ PWNF_USER_CALLBACK Callback
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlWnfDllUnloadCallback(
    _In_ PVOID DllBase
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_10

#define COPY_FILE_CHUNK_DUPLICATE_EXTENTS 0x00000001L // 24H2
#define VALID_COPY_FILE_CHUNK_FLAGS (COPY_FILE_CHUNK_DUPLICATE_EXTENTS)

//
// Property Store
//

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryPropertyStore(
    _In_ ULONG_PTR Key,
    _Out_ PULONG_PTR Context
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlRemovePropertyStore(
    _In_ ULONG_PTR Key,
    _Out_ PULONG_PTR Context
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlCompareExchangePropertyStore(
    _In_ ULONG_PTR Key,
    _In_ PULONG_PTR Comperand,
    _In_opt_ PULONG_PTR Exchange,
    _Out_ PULONG_PTR Context
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlWow64ChangeProcessState(
    _In_ HANDLE ProcessStateChangeHandle,
    _In_ HANDLE ProcessHandle,
    _In_ PROCESS_STATE_CHANGE_TYPE StateChangeType
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlWow64ChangeThreadState(
    _In_ HANDLE ThreadStateChangeHandle,
    _In_ HANDLE ThreadHandle,
    _In_ THREAD_STATE_CHANGE_TYPE StateChangeType
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
// rev
NTSYSAPI
USHORT
NTAPI
RtlGetCurrentThreadPrimaryGroup(
    VOID
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS1

#if (PHNT_VERSION >= PHNT_WINDOWS_11_24H2)
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProcessAvailableCpus(
    _In_ HANDLE ProcessHandle,
    _In_ PKAFFINITY_EX Affinity,
    _In_ ULONG64 ObservedSequenceNumber,
    _Out_opt_ PULONG64 SequenceNumber
    );

// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProcessAvailableCpusCount(
    _In_ HANDLE ProcessHandle,
    _Out_ PULONG AvailableCpusCount,
    _Out_opt_ PULONG64 SequenceNumber
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11_24H2

#endif // _NTRTL_H

/*
 * RTL forward symbol typedefs
 *
 * This file is part of System Informer.
 */
#ifndef _NTRTL_FWD_H
#define _NTRTL_FWD_H

// Note: ntdll symbols and exports define these forwarders:

// begin_forwarders
#ifndef PHNT_INLINE_NAME_FORWARDERS
#define RtlGetNativeSystemInformation NtQuerySystemInformation
#define RtlGetTickCount NtGetTickCount
#define RtlGuardRestoreContext RtlRestoreContext
#define RtlRandom RtlRandomEx
#define RtlOpenImageFileOptionsKey LdrOpenImageFileOptionsKey
#define RtlQueryImageFileExecutionOptions LdrQueryImageFileExecutionOptionsEx
#define RtlQueryImageFileKeyOption LdrQueryImageFileKeyOption
#define RtlSetTimer RtlCreateTimer
#define RtlRestoreLastWin32Error RtlSetLastWin32Error
#endif // PHNT_INLINE_NAME_FORWARDERS
// end_forwarders

#endif // _NTRTL_FWD_H
