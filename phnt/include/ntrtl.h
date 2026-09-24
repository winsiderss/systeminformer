/*
 * RTL support functions
 *
 * This file is part of System Informer.
 */

#ifndef _NTRTL_H
#define _NTRTL_H

/**
 * Forward declaration of the CPTABLEINFO (code page table information) structure.
 */
typedef struct _CPTABLEINFO CPTABLEINFO, *PCPTABLEINFO;
/**
 * Forward declaration of the FILE_INFORMATION_CLASS enumeration.
 */
typedef enum _FILE_INFORMATION_CLASS FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;
/**
 * Forward declaration of the RTL_AVL_TREE structure.
 */
typedef struct _RTL_AVL_TREE RTL_AVL_TREE, *PRTL_AVL_TREE;
/**
 * Forward declaration of the RTL_TRACE_DATABASE structure.
 */
typedef struct _RTL_TRACE_DATABASE RTL_TRACE_DATABASE, *PRTL_TRACE_DATABASE;
/**
 * Forward declaration of the RTL_DEBUG_INFORMATION structure.
 */
typedef struct _RTL_DEBUG_INFORMATION RTL_DEBUG_INFORMATION, *PRTL_DEBUG_INFORMATION;
/**
 * Forward declaration of the RTL_BUFFER structure.
 */
typedef struct _RTL_BUFFER RTL_BUFFER, *PRTL_BUFFER;
/**
 * Forward declaration of the RTL_RXACT_CONTEXT (registry transaction context) structure.
 */
typedef struct _RTL_RXACT_CONTEXT RTL_RXACT_CONTEXT, *PRTL_RXACT_CONTEXT;
/**
 * Forward declaration of the RTL_MUI_REGISTRY_INFO structure.
 */
typedef struct _RTL_MUI_REGISTRY_INFO RTL_MUI_REGISTRY_INFO, *PRTL_MUI_REGISTRY_INFO;
/**
 * Forward declaration of the CM_PARTIAL_RESOURCE_DESCRIPTOR structure.
 */
typedef struct _CM_PARTIAL_RESOURCE_DESCRIPTOR CM_PARTIAL_RESOURCE_DESCRIPTOR, *PCM_PARTIAL_RESOURCE_DESCRIPTOR;
/**
 * Forward declaration of the IO_RESOURCE_DESCRIPTOR structure.
 */
typedef struct _IO_RESOURCE_DESCRIPTOR IO_RESOURCE_DESCRIPTOR, *PIO_RESOURCE_DESCRIPTOR;


/**
 * Opaque pointer placeholder for an RTL length function.
 */
typedef PVOID PRTL_LENGTH_FUNCTION;
/**
 * Represents a 64-bit event tracing (ETW) registration handle.
 */
typedef ULONGLONG REGHANDLE, *PREGHANDLE;
/**
 * Opaque pointer to dynamic time zone information.
 */
typedef PVOID PRTL_DYNAMIC_TIME_ZONE_INFORMATION;

//
// Pointer arithmetic macros (type safe)
//

/**
 * Convert between a base pointer and a byte offset.
 */
#define RtlOffsetToPointer(Base, Offset) ((PUCHAR)(((PUCHAR)(Base)) + ((ULONG_PTR)(Offset))))
#define RtlPointerToOffset(Base, Pointer) ((ULONG)(((PUCHAR)(Pointer)) - ((PUCHAR)(Base))))

#if defined(__cplusplus)

EXTERN_C_END

/**
 * The RTL_PTR_ADD routine advances a typed pointer by a byte offset.
 *
 * \param Pointer The base pointer.
 * \param Value The number of bytes to add to the pointer.
 * \return T* A pointer of the same type advanced by the specified number of bytes.
 */
template <typename T>
FORCEINLINE
T*
RTL_PTR_ADD(T* Pointer, ULONG_PTR Value) noexcept {
    return reinterpret_cast<T*>(reinterpret_cast<PUCHAR>(Pointer) + Value);
}

/**
 * The RTL_PTR_SUBTRACT routine rewinds a typed pointer by a byte offset.
 *
 * \param Pointer The base pointer.
 * \param Value The number of bytes to subtract from the pointer.
 * \return T* A pointer of the same type rewound by the specified number of bytes.
 */
template <typename T>
FORCEINLINE
T*
RTL_PTR_SUBTRACT(T* Pointer, ULONG_PTR Value) noexcept {
    return reinterpret_cast<T*>(reinterpret_cast<PUCHAR>(Pointer) - Value);
}

EXTERN_C_START

#else

#ifndef RTL_PTR_ADD
/**
 * Adds a byte offset to a pointer.
 */
#define RTL_PTR_ADD(Pointer, Value) ((PVOID)(((PUCHAR)(Pointer)) + ((ULONG_PTR)(Value))))
#endif

#ifndef RTL_PTR_SUBTRACT
/**
 * Subtracts a byte offset from a pointer.
 */
#define RTL_PTR_SUBTRACT(Pointer, Value) ((PVOID)(((PUCHAR)(Pointer)) - ((ULONG_PTR)(Value))))
#endif

#endif

#ifndef RTL_IS_POWER_OF_TWO
/**
 * Evaluates to nonzero when a value is a power of two.
 */
#define RTL_IS_POWER_OF_TWO(Value) ((Value != 0) && !((Value) & ((Value) - 1)))
#endif

#ifndef RTL_IS_CLEAR_OR_SINGLE_FLAG
/**
 * Evaluates to nonzero when at most one flag bit of a mask is set.
 */
#define RTL_IS_CLEAR_OR_SINGLE_FLAG(Flags, Mask) (((Flags) & (Mask)) == 0 || !(((Flags) & (Mask)) & (((Flags) & (Mask)) - 1)))
#endif

#ifndef RTL_NUM_ALIGN_DOWN
/**
 * Rounds a number down to the nearest multiple of an alignment.
 */
#define RTL_NUM_ALIGN_DOWN(Number, Alignment) ((Number) - ((Number) & ((Alignment) - 1)))
#endif

#ifndef RTL_NUM_ALIGN_UP
/**
 * Rounds a number up to the nearest multiple of an alignment.
 */
#define RTL_NUM_ALIGN_UP(Number, Alignment) RTL_NUM_ALIGN_DOWN((Number) + (Alignment) - 1, (Alignment))
#endif

//
// Time unit constants (ordered by magnitude)
//

/**
 * System time-unit conversion constants expressed in 100-nanosecond ticks.
 */
#define RTL_NANOSEC_PER_TICK        ULONG64_C(100)
#define RTL_TICKS_PER_MICROSEC      ULONG64_C(10)
#define RTL_TICKS_PER_MILLISEC      (RTL_TICKS_PER_MICROSEC * ULONG64_C(1000))  // 10,000
#define RTL_TICKS_PER_SEC           (RTL_TICKS_PER_MILLISEC * ULONG64_C(1000))  // 10,000,000
#define RTL_TICKS_PER_MIN           (RTL_TICKS_PER_SEC      * ULONG64_C(60))    // 600,000,000
#define RTL_TICKS_PER_HOUR          (RTL_TICKS_PER_MIN      * ULONG64_C(60))    // 36,000,000,000
#define RTL_TICKS_PER_DAY           (RTL_TICKS_PER_HOUR     * ULONG64_C(24))    // 864,000,000,000
#define RTL_TICKS_PER_WEEK          (RTL_TICKS_PER_DAY      * ULONG64_C(7))     // 6,048,000,000,000
#define RTL_TICKS_PER_MONTH         (RTL_TICKS_PER_DAY      * ULONG64_C(30))    // 25,920,000,000,000
#define RTL_TICKS_PER_YEAR          (RTL_TICKS_PER_DAY      * ULONG64_C(365))   // 31,536,000,000,000
#define RTL_TICKS_PER_LEAP_YEAR     (RTL_TICKS_PER_DAY      * ULONG64_C(366))   // 31,622,400,000,000

/**
 * Constants expressing sub-second time units in nanoseconds and 100-nanosecond units.
 */
#define RTL_NANOSEC_PER_SEC              ULONG64_C(1000000000)
#define RTL_NANOSEC_PER_MILLISEC            ULONG64_C(1000000)
#define RTL_100NANOSEC_PER_SEC             ULONG64_C(10000000)
#define RTL_100NANOSEC_PER_MILLISEC           ULONG64_C(10000)
#define RTL_MILLISEC_PER_SEC                   ULONG64_C(1000)

/**
 * Constants expressing common time spans in whole seconds.
 */
#define RTL_SEC_PER_HOUR                       ULONG64_C(3600) // 1 hour  // 3,600 seconds
#define RTL_SEC_PER_DAY                       ULONG64_C(86400) // 1 day   // 86,400 seconds
#define RTL_SEC_PER_WEEK                     ULONG64_C(604800) // 1 week  // 604,800 seconds
#define RTL_SEC_PER_MONTH                   ULONG64_C(2592000) // 1 month // 2,592,000 seconds (30 days)
#define RTL_SEC_PER_YEAR                   ULONG64_C(31536000) // 1 year  // 31,536,000 seconds (365 days)

//
// Time conversion macros (ordered by unit)
//

/**
 * Macros converting time values to and from nanoseconds.
 */
#define RTL_SEC_TO_NANOSEC(s)          ((s) * RTL_NANOSEC_PER_SEC)
#define RTL_NANOSEC_TO_SEC(ns)         ((ns) / RTL_NANOSEC_PER_SEC)
#define RTL_MILLISEC_TO_NANOSEC(m)     ((m) * RTL_NANOSEC_PER_MILLISEC)
#define RTL_NANOSEC_TO_MILLISEC(ns)    ((ns) / RTL_NANOSEC_PER_MILLISEC)
#define RTL_NANOSEC_TO_100NANOSEC(ns)  ((ns) / ULONG64_C(100))
#define RTL_100NANOSEC_TO_NANOSEC(ns)  ((ns) * ULONG64_C(100))

/**
 * Macros converting time values to and from 100-nanosecond units.
 */
#define RTL_SEC_TO_100NANOSEC(s)       ((s) * RTL_100NANOSEC_PER_SEC)
#define RTL_100NANOSEC_TO_SEC(ns)      ((ns) / RTL_100NANOSEC_PER_SEC)
#define RTL_MILLISEC_TO_100NANOSEC(m)  ((m) * RTL_100NANOSEC_PER_MILLISEC)
#define RTL_100NANOSEC_TO_MILLISEC(ns) ((ns) / RTL_100NANOSEC_PER_MILLISEC)

/**
 * Macros converting time values to and from milliseconds.
 */
#define RTL_SEC_TO_MILLISEC(s)         ((s) * RTL_MILLISEC_PER_SEC)
#define RTL_MILLISEC_TO_SEC(m)         ((m) / RTL_MILLISEC_PER_SEC)

/**
 * The maximum value of the e_lfanew field in the IMAGE_DOS_HEADER structure for validation.
 */
#define RTL_IMAGE_MAX_DOS_HEADER (ULONG_C(256) * (ULONG_C(1024) * ULONG_C(1024))) // 256 MB

/**
 * Meta characters for wildcard processing.
 * \remarks NtQueryDirectoryFile(Ex), RtlDoesNameContainWildCards and file system drivers (FAT, NTFS, REFS).
 */
#define ANSI_DOS_STAR ((CHAR)'<')
#define ANSI_DOS_STAR_W ((WCHAR)L'<')
#define ANSI_DOS_QM ((CHAR)'>')
#define ANSI_DOS_QM_W ((WCHAR)L'>')
#define ANSI_DOS_DOT ((CHAR)'"')
#define ANSI_DOS_DOT_W ((WCHAR)L'"')

/**
 * Record sizes used when querying loaded module information.
 */
#define RTL_QUERY_MODULE_INFORMATION_RECORD_SIZE_IMAGE_BASE 0x8
#define RTL_QUERY_MODULE_INFORMATION_RECORD_SIZE_MODULE     0x110

/**
 * Identifies the class of system resource governed by a resource policy.
 */
typedef enum _RTL_RESOURCE_POLICY_CLASS
{
    RtlResourcePolicyPhysicalMemory = 0,
    RtlResourcePolicyDiskSpace = 1,
    RtlResourcePolicyDiskSpeed = 2,
    RtlResourcePolicyDiskWriteConstraint = 3
} RTL_RESOURCE_POLICY_CLASS, *PRTL_RESOURCE_POLICY_CLASS;

//
// Errors
//

/**
 * The RtlFailFast routine brings down the caller immediately in the event that critical corruption has been detected. No exception handlers are invoked.
 *
 * \param Code A FAST_FAIL_* symbolic constant from winnt.h or wdm.h that indicates the reason for process termination.
 * \return None. There is no return from this routine.
 * \remarks The routine is shared with user mode and kernel mode. In user mode, the process is terminated, whereas in kernel mode, a KERNEL_SECURITY_CHECK_FAILURE bug check is raised.
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

/**
 * The RtlFatalListEntryError routine reports a fatal list entry error.
 *
 * \param p1 The first parameter passed to `RtlFailFast`.
 * \param p2 The second parameter passed to `RtlFailFast`.
 * \param p3 The third parameter passed to `RtlFailFast`.
 * \remarks This routine is a wrapper around `RtlFailFast` that can be used to provide alternative reporting mechanisms, such as logging and trying to continue.
 */
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

/**
 * Forward declaration of the LIST_ENTRY structure.
 */
typedef struct _LIST_ENTRY LIST_ENTRY, *PLIST_ENTRY;

/**
 * Declares and statically initializes a doubly linked list head.
 */
#define RTL_STATIC_LIST_HEAD(x) \
    LIST_ENTRY (x) = { &(x), &(x) }

/**
 * Iterates over each entry of a doubly linked list.
 */
#define RTL_LIST_FOREACH(Entry, ListHead) \
    for ((Entry) = (&(ListHead))->Flink; (Entry) != &(ListHead); (Entry) = (Entry)->Flink)

// #ifndef NO_LIST_ENTRY_CHECKS
// #define NO_LIST_ENTRY_CHECKS
// #endif

/**
 * The RtlCheckListEntry routine checks the integrity of a doubly linked list entry.
 *
 * \param Entry A pointer to the list entry to check.
 * \remarks This function calls `RtlFatalListEntryError` if the list entry is corrupted.
 */
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

/**
 * The InitializeListHead routine initializes a doubly linked list head.
 *
 * \param ListHead A pointer to the `LIST_ENTRY` structure to be initialized as a list head.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-initializelisthead
 */
FORCEINLINE
VOID
NTAPI_INLINE
InitializeListHead(
    _Out_ PLIST_ENTRY ListHead
    )
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

/**
 * The InitializeListHead32 routine initializes a 32-bit doubly linked list head.
 *
 * \param ListHead A pointer to the `LIST_ENTRY32` structure to be initialized as a list head.
 */
FORCEINLINE
VOID
NTAPI_INLINE
InitializeListHead32(
    _Out_ PLIST_ENTRY32 ListHead
    )
{
    ListHead->Flink = ListHead->Blink = PtrToUlong(ListHead);
}

/**
 * The IsListEmpty routine determines whether a doubly linked list is empty.
 *
 * \param ListHead A pointer to the list head.
 *
 * \return `TRUE` if the list is empty, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-islistempty
 */
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

/**
 * The RemoveEntryListUnsafe routine removes an entry from a doubly linked list without checking for integrity.
 *
 * \param Entry A pointer to the list entry to be removed.
 * \return `TRUE` if the list becomes empty after the entry is removed, otherwise `FALSE`.
 */
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

/**
 * The RemoveEntryList routine removes an entry from a doubly linked list.
 *
 * \param Entry A pointer to the list entry to be removed.
 * \return `TRUE` if the list becomes empty after the entry is removed, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-removeentrylist
 */
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

/**
 * The RemoveHeadList routine removes the entry from the head of a doubly linked list.
 *
 * \param ListHead A pointer to the list head.
 * \return A pointer to the entry removed from the head of the list.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-removeheadlist
 */
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

/**
 * The RemoveTailList routine removes the entry from the tail of a doubly linked list.
 *
 * \param ListHead A pointer to the list head.
 * \return A pointer to the entry removed from the tail of the list.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-removetaillist
 */
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

#if !defined(NO_LIST_ENTRY_CHECKS)
    if ((Entry->Flink != ListHead) || (PrevEntry->Flink != Entry))
    {
        RtlFatalListEntryError((PVOID)PrevEntry, (PVOID)Entry, (PVOID)ListHead);
    }
#endif

    ListHead->Blink = PrevEntry;
    PrevEntry->Flink = ListHead;

    return Entry;
}

/**
 * The InsertTailList routine inserts an entry at the tail of a doubly linked list.
 *
 * \param ListHead A pointer to the list head.
 * \param Entry A pointer to the list entry to be inserted.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-inserttaillist
 */
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

#if !defined(NO_LIST_ENTRY_CHECKS)
    if (PrevEntry->Flink != ListHead)
    {
        RtlFatalListEntryError((PVOID)PrevEntry, (PVOID)ListHead, (PVOID)PrevEntry->Flink);
    }
#endif

    Entry->Flink = ListHead;
    Entry->Blink = PrevEntry;
    PrevEntry->Flink = Entry;
    ListHead->Blink = Entry;
}

/**
 * The InsertHeadList routine inserts an entry at the head of a doubly linked list.
 *
 * \param ListHead A pointer to the list head.
 * \param Entry A pointer to the list entry to be inserted.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-insertheadlist
 */
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

#if !defined(NO_LIST_ENTRY_CHECKS)
    RtlCheckListEntry(ListHead);

    if (NextEntry->Blink != ListHead)
    {
        RtlFatalListEntryError((PVOID)ListHead, (PVOID)NextEntry, (PVOID)NextEntry->Blink);
    }
#endif

    Entry->Flink = NextEntry;
    Entry->Blink = ListHead;
    NextEntry->Blink = Entry;
    ListHead->Flink = Entry;
}

/**
 * The AppendTailList routine appends a doubly linked list to the tail of another doubly linked list.
 *
 * \param ListHead A pointer to the head of the list to which to append.
 * \param ListToAppend A pointer to the list to be appended.
 */
FORCEINLINE
VOID
NTAPI_INLINE
AppendTailList(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ PLIST_ENTRY ListToAppend
    )
{
    PLIST_ENTRY ListEnd = ListHead->Blink;

#if !defined(NO_LIST_ENTRY_CHECKS)
    RtlCheckListEntry(ListHead);
    RtlCheckListEntry(ListToAppend);
#endif

    ListHead->Blink->Flink = ListToAppend;
    ListHead->Blink = ListToAppend->Blink;
    ListToAppend->Blink->Flink = ListHead;
    ListToAppend->Blink = ListEnd;
}

/**
 * The IsSingleListEmpty routine indicates whether a singly linked list is empty.
 *
 * \param ListHead A pointer to the SINGLE_LIST_ENTRY that serves as the list header.
 * \return Returns `TRUE` if the list is empty, otherwise `FALSE`.
 */
_Must_inspect_result_
FORCEINLINE
BOOLEAN
IsSingleListEmpty (
    _Inout_ PSINGLE_LIST_ENTRY ListHead
    )
{
    return ListHead->Next == NULL;
}

/**
 * The PopEntryList routine removes the first entry from a singly linked list.
 *
 * \param ListHead A pointer to the list head.
 * \return A pointer to the entry removed from the head of the list, or `NULL` if the list is empty.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-popentrylist
 */
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

/**
 * The PushEntryList routine inserts an entry at the head of a singly linked list.
 *
 * \param ListHead A pointer to the list head.
 * \param Entry A pointer to the list entry to be inserted.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-pushentrylist
 */
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
// Single list volatile accessors
//

/**
 * The IsSingleListEmptyNoFence routine indicates whether a singly linked list is empty using a no-fence (non-serializing) read.
 *
 * \param ListHead A pointer to the SINGLE_LIST_ENTRY that serves as the list header.
 * \return Returns `TRUE` if the list is empty, otherwise `FALSE`.
 */
_Must_inspect_result_
FORCEINLINE
BOOLEAN
IsSingleListEmptyNoFence (
    _Inout_ PSINGLE_LIST_ENTRY ListHead
    )
{
    return ReadPointerNoFence((PVOID*)&ListHead->Next) == NULL;
}

/**
 * The PopEntryListNoFence routine removes the first entry from a singly linked list using a no-fence (non-serializing) read.
 *
 * \param ListHead A pointer to the SINGLE_LIST_ENTRY that serves as the list header.
 * \return PSINGLE_LIST_ENTRY A pointer to the entry removed from the list, or `NULL` if the list was empty.
 */
FORCEINLINE
PSINGLE_LIST_ENTRY
PopEntryListNoFence (
    _Inout_ PSINGLE_LIST_ENTRY ListHead
    )
{
    PSINGLE_LIST_ENTRY FirstEntry;

    FirstEntry = ListHead->Next;
    if (FirstEntry != NULL)
    {
        WritePointerNoFence((PVOID*)&ListHead->Next, FirstEntry->Next);
    }

    return FirstEntry;
}

/**
 * The PushEntryListNoFence routine inserts an entry at the front of a singly linked list using a no-fence (non-serializing) write.
 *
 * \param ListHead A pointer to the SINGLE_LIST_ENTRY that serves as the list header.
 * \param Entry A pointer to the SINGLE_LIST_ENTRY to insert at the front of the list.
 */
FORCEINLINE
VOID
PushEntryListNoFence (
    _Inout_ PSINGLE_LIST_ENTRY ListHead,
    _Inout_ __drv_aliasesMem PSINGLE_LIST_ENTRY Entry
    )
{
    Entry->Next = ListHead->Next;
    WritePointerNoFence((PVOID*)&ListHead->Next, Entry);
    return;
}

//
// List volatile accessors
//

/**
 * The RemoveEntryListNoFence routine removes an entry from a doubly linked list using no-fence (non-serializing) writes.
 *
 * \param Entry A pointer to the LIST_ENTRY that represents the entry to remove.
 * \return Returns `TRUE` if the list is empty after removing the entry, otherwise `FALSE`.
 */
FORCEINLINE
BOOLEAN
RemoveEntryListNoFence(
    _In_ PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY PrevEntry;
    PLIST_ENTRY NextEntry;

    NextEntry = (PLIST_ENTRY)ReadPointerNoFence((volatile const PVOID*)&Entry->Flink);
    PrevEntry = (PLIST_ENTRY)ReadPointerNoFence((volatile const PVOID*)&Entry->Blink);

    if ((ReadPointerNoFence((volatile const PVOID*)&NextEntry->Blink) != Entry) ||
        (ReadPointerNoFence((volatile const PVOID*)&PrevEntry->Flink) != Entry))
    {
        RtlFatalListEntryError((PVOID)PrevEntry,
                               (PVOID)Entry,
                               (PVOID)NextEntry);
    }

    WritePointerNoFence((volatile PVOID*)&PrevEntry->Flink, NextEntry);
    WritePointerNoFence((volatile PVOID*)&NextEntry->Blink, PrevEntry);
    return (BOOLEAN)(PrevEntry == NextEntry);
}

/**
 * The RemoveHeadListNoFence routine removes the entry at the head of a doubly linked list using no-fence (non-serializing) accesses.
 *
 * \param ListHead A pointer to the LIST_ENTRY that serves as the list header.
 * \return PLIST_ENTRY A pointer to the entry removed from the head of the list.
 */
FORCEINLINE
PLIST_ENTRY
RemoveHeadListNoFence(
    _Inout_ PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Entry;
    PLIST_ENTRY NextEntry;

    Entry = (PLIST_ENTRY)ReadPointerNoFence((volatile const PVOID*)&ListHead->Flink);

#if DBG

    RtlCheckListEntry(ListHead);

#endif

    NextEntry = (PLIST_ENTRY)ReadPointerNoFence((volatile const PVOID*)&Entry->Flink);

    if ((ReadPointerNoFence((volatile const PVOID*)&Entry->Blink) != ListHead) ||
        (ReadPointerNoFence((volatile const PVOID*)&NextEntry->Blink) != Entry))
    {
        RtlFatalListEntryError((PVOID)ListHead,
                               (PVOID)Entry,
                               (PVOID)NextEntry);
    }

    WritePointerNoFence((volatile PVOID*)&ListHead->Flink, NextEntry);
    WritePointerNoFence((volatile PVOID*)&NextEntry->Blink, ListHead);

    return Entry;
}

/**
 * The RemoveTailListNoFence routine removes the entry at the tail of a doubly linked list using no-fence (non-serializing) accesses.
 *
 * \param ListHead A pointer to the LIST_ENTRY that serves as the list header.
 * \return PLIST_ENTRY A pointer to the entry removed from the tail of the list.
 */
FORCEINLINE
PLIST_ENTRY
RemoveTailListNoFence(
    _Inout_ PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Entry;
    PLIST_ENTRY PrevEntry;

    Entry = (PLIST_ENTRY)ReadPointerNoFence((volatile const PVOID*)&ListHead->Blink);

#if DEBUG

    RtlCheckListEntry(ListHead);

#endif

    PrevEntry = (PLIST_ENTRY)ReadPointerNoFence((volatile const PVOID*)&Entry->Blink);

    if ((ReadPointerNoFence((volatile const PVOID*)&Entry->Flink) != ListHead) ||
        (ReadPointerNoFence((volatile const PVOID*)&PrevEntry->Flink) != Entry))
    {
        RtlFatalListEntryError((PVOID)PrevEntry,
                               (PVOID)Entry,
                               (PVOID)ListHead);
    }

    WritePointerNoFence((volatile PVOID*)&ListHead->Blink, PrevEntry);
    WritePointerNoFence((volatile PVOID*)&PrevEntry->Flink, ListHead);
    return Entry;
}

/**
 * The InsertTailListNoFence routine inserts an entry at the tail of a doubly linked list using a no-fence (non-serializing) write.
 *
 * \param ListHead A pointer to the LIST_ENTRY that serves as the list header.
 * \param Entry A pointer to the LIST_ENTRY to insert at the tail of the list.
 */
FORCEINLINE
VOID
InsertTailListNoFence(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ __drv_aliasesMem PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY PrevEntry;

#if DEBUG

    RtlCheckListEntry(ListHead);

#endif

    PrevEntry = (PLIST_ENTRY)ReadPointerNoFence((volatile const PVOID*)&ListHead->Blink);

    if (ReadPointerNoFence((volatile const PVOID*)&PrevEntry->Flink) != ListHead)
    {
        RtlFatalListEntryError((PVOID)PrevEntry,
                               (PVOID)ListHead,
                               (PVOID)PrevEntry->Flink);
    }

    WritePointerNoFence((volatile PVOID*)&Entry->Flink, ListHead);
    WritePointerNoFence((volatile PVOID*)&Entry->Blink, PrevEntry);
    WritePointerNoFence((volatile PVOID*)&PrevEntry->Flink, Entry);
    WritePointerNoFence((volatile PVOID*)&ListHead->Blink, Entry);
    return;
}

/**
 * The InsertHeadListNoFence routine inserts an entry at the head of a doubly linked list using a no-fence (non-serializing) write.
 *
 * \param ListHead A pointer to the LIST_ENTRY that serves as the list header.
 * \param Entry A pointer to the LIST_ENTRY to insert at the head of the list.
 */
FORCEINLINE
VOID
InsertHeadListNoFence(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ __drv_aliasesMem PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY NextEntry;

#if DEBUG

    RtlCheckListEntry(ListHead);

#endif

    NextEntry = (PLIST_ENTRY)ReadPointerNoFence((volatile const PVOID*)&ListHead->Flink);

    if (ReadPointerNoFence((volatile const PVOID*)&NextEntry->Blink) != ListHead)
    {
        RtlFatalListEntryError((PVOID)ListHead,
                               (PVOID)NextEntry,
                               (PVOID)NextEntry->Blink);
    }

    WritePointerNoFence((volatile PVOID*)&Entry->Flink, NextEntry);
    WritePointerNoFence((volatile PVOID*)&Entry->Blink, ListHead);
    WritePointerNoFence((volatile PVOID*)&NextEntry->Blink, Entry);
    WritePointerNoFence((volatile PVOID*)&ListHead->Flink, Entry);
    return;
}

/**
 * The AppendTailListNoFence routine appends a doubly linked list to the tail of another doubly linked list using a no-fence (non-serializing) write.
 *
 * \param ListHead A pointer to the LIST_ENTRY that serves as the header of the destination list.
 * \param ListToAppend A pointer to the LIST_ENTRY that serves as the header of the list to append.
 */
FORCEINLINE
VOID
AppendTailListNoFence(
    _Inout_ PLIST_ENTRY ListHead,
    _Inout_ PLIST_ENTRY ListToAppend
    )
{
    PLIST_ENTRY ListEnd = (PLIST_ENTRY)ReadPointerNoFence((volatile const PVOID*)&ListHead->Blink);

    RtlCheckListEntry(ListHead);
    RtlCheckListEntry(ListToAppend);

    WritePointerNoFence((volatile PVOID*)&ListHead->Blink->Flink, ListToAppend);
    WritePointerNoFence((volatile PVOID*)&ListHead->Blink, ListToAppend->Blink);
    WritePointerNoFence((volatile PVOID*)&ListToAppend->Blink->Flink, ListHead);
    WritePointerNoFence((volatile PVOID*)&ListToAppend->Blink, ListEnd);
    return;
}

// Rtl-prefixed aliases for list helpers.
/**
 * Rtl-prefixed aliases for the standard doubly linked list manipulation macros.
 */
#define RtlInitializeListHead InitializeListHead
#define RtlInitializeListHead32 InitializeListHead32
#define RtlIsListEmpty IsListEmpty
#define RtlRemoveEntryListUnsafe RemoveEntryListUnsafe
#define RtlRemoveEntryList RemoveEntryList
#define RtlRemoveHeadList RemoveHeadList
#define RtlRemoveTailList RemoveTailList
#define RtlInsertTailList InsertTailList
#define RtlInsertHeadList InsertHeadList
#define RtlAppendTailList AppendTailList
#define RtlPopEntryList PopEntryList
#define RtlPushEntryList PushEntryList

//
// AVL and splay trees
//

/**
 * Describes the outcome of searching a generic or AVL table for an element.
 */
typedef enum _TABLE_SEARCH_RESULT
{
    TableEmptyTree,
    TableFoundNode,
    TableInsertAsLeft,
    TableInsertAsRight
} TABLE_SEARCH_RESULT;

/**
 * Describes the ordering relationship between two elements compared in a generic table.
 */
typedef enum _RTL_GENERIC_COMPARE_RESULTS
{
    GenericLessThan,
    GenericGreaterThan,
    GenericEqual
} RTL_GENERIC_COMPARE_RESULTS;

/**
 * Forward declaration of the RTL_AVL_TABLE structure.
 */
typedef struct _RTL_AVL_TABLE RTL_AVL_TABLE, *PRTL_AVL_TABLE;

typedef _Function_class_(RTL_AVL_COMPARE_ROUTINE)
RTL_GENERIC_COMPARE_RESULTS NTAPI RTL_AVL_COMPARE_ROUTINE(
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID FirstStruct,
    _In_ PVOID SecondStruct
    );
/**
 * Pointer to an RTL_AVL_COMPARE_ROUTINE callback.
 */
typedef RTL_AVL_COMPARE_ROUTINE* PRTL_AVL_COMPARE_ROUTINE;

typedef _Function_class_(RTL_AVL_ALLOCATE_ROUTINE)
PVOID NTAPI RTL_AVL_ALLOCATE_ROUTINE(
    _In_ PRTL_AVL_TABLE Table,
    _In_ CLONG ByteSize
    );
/**
 * Pointer to an RTL_AVL_ALLOCATE_ROUTINE callback.
 */
typedef RTL_AVL_ALLOCATE_ROUTINE* PRTL_AVL_ALLOCATE_ROUTINE;

typedef _Function_class_(RTL_AVL_FREE_ROUTINE)
VOID NTAPI RTL_AVL_FREE_ROUTINE(
    _In_ PRTL_AVL_TABLE Table,
    _In_ _Post_invalid_ PVOID Buffer
    );
/**
 * Pointer to an RTL_AVL_FREE_ROUTINE callback.
 */
typedef RTL_AVL_FREE_ROUTINE* PRTL_AVL_FREE_ROUTINE;

typedef _Function_class_(RTL_AVL_MATCH_FUNCTION)
NTSTATUS NTAPI RTL_AVL_MATCH_FUNCTION(
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID UserData,
    _In_ PVOID MatchData
    );
/**
 * Pointer to an RTL_AVL_MATCH_FUNCTION callback.
 */
typedef RTL_AVL_MATCH_FUNCTION* PRTL_AVL_MATCH_FUNCTION;

/**
 * Represents a node in a balanced (AVL) binary tree used by the generic table package.
 */
typedef struct _RTL_BALANCED_LINKS
{
    struct _RTL_BALANCED_LINKS *Parent;
    struct _RTL_BALANCED_LINKS *LeftChild;
    struct _RTL_BALANCED_LINKS *RightChild;
    CHAR Balance;
    UCHAR Reserved[3];
} RTL_BALANCED_LINKS, *PRTL_BALANCED_LINKS;

/**
 * Represents a generic table implemented as a balanced (AVL) binary tree.
 */
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

/**
 * The RtlInitializeGenericTableAvl routine initializes a generic AVL table.
 *
 * \param Table A pointer to the `RTL_AVL_TABLE` structure to be initialized.
 * \param CompareRoutine A comparison routine to be used for comparing elements.
 * \param AllocateRoutine An allocation routine to be used for allocating memory for the table.
 * \param FreeRoutine A free routine to be used for freeing memory allocated for the table.
 * \param TableContext A context to be passed to the comparison, allocation, and free routines.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlinitializegenerictableavl
 */
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

/**
 * The RtlInsertElementGenericTableAvl routine inserts an element into a generic AVL table.
 *
 * \param Table A pointer to the generic table.
 * \param Buffer A pointer to the buffer containing the element to be inserted.
 * \param BufferSize The size of the buffer.
 * \param NewElement A pointer to a boolean that receives `TRUE` if the element was newly inserted, or `FALSE` if the element already existed.
 * \return A pointer to the newly inserted or existing element.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlinsertelementgenerictableavl
 */
NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTableAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ CLONG BufferSize,
    _Out_opt_ PBOOLEAN NewElement
    );

/**
 * The RtlInsertElementGenericTableFullAvl routine inserts an element into a generic AVL table.
 *
 * \param Table A pointer to the generic table.
 * \param Buffer A pointer to the buffer containing the element to be inserted.
 * \param BufferSize The size of the buffer.
 * \param NewElement A pointer to a boolean that receives `TRUE` if the element was newly inserted, or `FALSE` if the element already existed.
 * \param NodeOrParent A pointer to the node or parent for the insertion.
 * \param SearchResult The result of the search for the insertion point.
 * \return A pointer to the newly inserted or existing element.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlinsertelementgenerictablefullavl
 */
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

/**
 * The RtlDeleteElementGenericTableAvl routine deletes an element from a generic AVL table.
 *
 * \param Table A pointer to the generic table.
 * \param Buffer A pointer to the buffer containing the element to be deleted.
 * \return `TRUE` if the element was deleted, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtldeleteelementgenerictableavl
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlDeleteElementGenericTableAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID Buffer
    );

/**
 * The RtlLookupElementGenericTableAvl routine looks up an element in a generic AVL table.
 *
 * \param Table A pointer to the generic table.
 * \param Buffer A pointer to the buffer containing the element to look up.
 * \return A pointer to the found element, or `NULL` if the element was not found.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtllookupelementgenerictableavl
 */
_Check_return_
NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID Buffer
    );

/**
 * The RtlLookupElementGenericTableFullAvl routine looks up an element in a generic AVL table.
 *
 * \param Table A pointer to the generic table.
 * \param Buffer A pointer to the buffer containing the element to look up.
 * \param NodeOrParent A pointer that receives the node or parent of the element.
 * \param SearchResult A pointer that receives the result of the search.
 * \return A pointer to the found element, or `NULL` if the element was not found.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtllookupelementgenerictablefullavl
 */
NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableFullAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID Buffer,
    _Out_ PVOID *NodeOrParent,
    _Out_ TABLE_SEARCH_RESULT *SearchResult
    );

/**
 * The RtlEnumerateGenericTableAvl routine enumerates the elements in a generic AVL table.
 *
 * \param Table A pointer to the generic table.
 * \param Restart `TRUE` to restart the enumeration, `FALSE` to continue.
 * \return A pointer to the next element in the table, or `NULL` if there are no more elements.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlenumerategenerictableavl
 */
_Check_return_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_ BOOLEAN Restart
    );

/**
 * The RtlEnumerateGenericTableWithoutSplayingAvl routine enumerates the elements in a generic AVL table without splaying.
 *
 * \param Table A pointer to the generic table.
 * \param RestartKey A pointer to a restart key.
 * \return A pointer to the next element in the table, or `NULL` if there are no more elements.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlenumerategenerictablewithoutsplayingavl
 */
_Check_return_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplayingAvl(
    _In_ PRTL_AVL_TABLE Table,
    _Inout_ PVOID *RestartKey
    );

/**
 * The RtlLookupFirstMatchingElementGenericTableAvl routine looks up the first matching element in a generic AVL table.
 *
 * \param Table A pointer to the generic table.
 * \param Buffer A pointer to the buffer containing the element to look up.
 * \param RestartKey A pointer to a restart key.
 * \return A pointer to the first matching element, or `NULL` if no matching element was found.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtllookupfirstmatchingelementgenerictableavl
 */
_Check_return_
NTSYSAPI
PVOID
NTAPI
RtlLookupFirstMatchingElementGenericTableAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_ PVOID Buffer,
    _Out_ PVOID *RestartKey
    );

/**
 * The RtlEnumerateGenericTableLikeADirectory routine enumerates the elements in a generic AVL table like a directory.
 *
 * \param Table A pointer to the generic table.
 * \param MatchFunction A match function to be used for comparing elements.
 * \param MatchData A context to be passed to the match function.
 * \param NextFlag A flag indicating whether to move to the next element.
 * \param RestartKey A pointer to a restart key.
 * \param DeleteCount A pointer to a counter for deleted elements.
 * \param Buffer A pointer to the buffer containing the element to look up.
 * \return A pointer to the next element in the table, or `NULL` if there are no more elements.
 */
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

/**
 * The RtlGetElementGenericTableAvl routine gets an element from a generic AVL table by its index.
 *
 * \param Table A pointer to the generic table.
 * \param I The index of the element to get.
 * \return A pointer to the element, or `NULL` if the index is out of range.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlgetelementgenerictableavl
 */
_Check_return_
NTSYSAPI
PVOID
NTAPI
RtlGetElementGenericTableAvl(
    _In_ PRTL_AVL_TABLE Table,
    _In_ ULONG I
    );

/**
 * The RtlNumberGenericTableElementsAvl routine gets the number of elements in a generic AVL table.
 *
 * \param Table A pointer to the generic table.
 * \return The number of elements in the table.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlnumbergenerictableelementsavl
 */
NTSYSAPI
ULONG
NTAPI
RtlNumberGenericTableElementsAvl(
    _In_ PRTL_AVL_TABLE Table
    );

/**
 * The RtlIsGenericTableEmptyAvl routine determines whether a generic AVL table is empty.
 *
 * \param Table A pointer to the generic table.
 * \return `TRUE` if the table is empty, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlisgenerictableemptyavl
 */
_Check_return_
NTSYSAPI
BOOLEAN
NTAPI
RtlIsGenericTableEmptyAvl(
    _In_ PRTL_AVL_TABLE Table
    );

/**
 * Represents the links of a node in a splay tree.
 */
typedef struct _RTL_SPLAY_LINKS
{
    struct _RTL_SPLAY_LINKS *Parent;
    struct _RTL_SPLAY_LINKS *LeftChild;
    struct _RTL_SPLAY_LINKS *RightChild;
} RTL_SPLAY_LINKS, *PRTL_SPLAY_LINKS;

/**
 * Initializes the links of a splay tree node.
 */
#define RtlInitializeSplayLinks(Links) \
{ \
    PRTL_SPLAY_LINKS _SplayLinks; \
    _SplayLinks = (PRTL_SPLAY_LINKS)(Links); \
    _SplayLinks->Parent = _SplayLinks; \
    _SplayLinks->LeftChild = NULL; \
    _SplayLinks->RightChild = NULL; \
}

/**
 * Accessors for the parent and child links of a splay tree node.
 */
#define RtlParent(Links) ((PRTL_SPLAY_LINKS)(Links)->Parent)
#define RtlLeftChild(Links) ((PRTL_SPLAY_LINKS)(Links)->LeftChild)
#define RtlRightChild(Links) ((PRTL_SPLAY_LINKS)(Links)->RightChild)
#define RtlIsRoot(Links) ((RtlParent(Links) == (PRTL_SPLAY_LINKS)(Links)))
#define RtlIsLeftChild(Links) ((RtlLeftChild(RtlParent(Links)) == (PRTL_SPLAY_LINKS)(Links)))
#define RtlIsRightChild(Links) ((RtlRightChild(RtlParent(Links)) == (PRTL_SPLAY_LINKS)(Links)))

/**
 * Inserts a node as the left child of a splay tree node.
 */
#define RtlInsertAsLeftChild(ParentLinks, ChildLinks) \
{ \
    PRTL_SPLAY_LINKS _SplayParent; \
    PRTL_SPLAY_LINKS _SplayChild; \
    _SplayParent = (PRTL_SPLAY_LINKS)(ParentLinks); \
    _SplayChild = (PRTL_SPLAY_LINKS)(ChildLinks); \
    _SplayParent->LeftChild = _SplayChild; \
    _SplayChild->Parent = _SplayParent; \
}

/**
 * Inserts a node as the right child of a splay tree node.
 */
#define RtlInsertAsRightChild(ParentLinks, ChildLinks) \
{ \
    PRTL_SPLAY_LINKS _SplayParent; \
    PRTL_SPLAY_LINKS _SplayChild; \
    _SplayParent = (PRTL_SPLAY_LINKS)(ParentLinks); \
    _SplayChild = (PRTL_SPLAY_LINKS)(ChildLinks); \
    _SplayParent->RightChild = _SplayChild; \
    _SplayChild->Parent = _SplayParent; \
}

/**
 * The RtlSplay routine performs a splay operation on a splay tree.
 *
 * \param Links A pointer to the splay links of the node to splay.
 * \return A pointer to the new root of the splay tree.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlsplay
 */
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSplay(
    _Inout_ PRTL_SPLAY_LINKS Links
    );

/**
 * The RtlDelete routine deletes a node from a splay tree.
 *
 * \param Links A pointer to the splay links of the node to delete.
 * \return A pointer to the new root of the splay tree.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtldelete
 */
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlDelete(
    _In_ PRTL_SPLAY_LINKS Links
    );

/**
 * The RtlDeleteNoSplay routine deletes a node from a splay tree without splaying.
 *
 * \param Links A pointer to the splay links of the node to delete.
 * \param Root A pointer to the root of the splay tree.
 */
NTSYSAPI
VOID
NTAPI
RtlDeleteNoSplay(
    _In_ PRTL_SPLAY_LINKS Links,
    _Inout_ PRTL_SPLAY_LINKS *Root
    );

/**
 * The RtlSubtreeSuccessor routine finds the successor of a node in a splay tree.
 *
 * \param Links A pointer to the splay links of the node.
 * \return A pointer to the splay links of the successor node.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlsubtreesuccessor
 */
_Check_return_
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSubtreeSuccessor(
    _In_ PRTL_SPLAY_LINKS Links
    );

/**
 * The RtlSubtreePredecessor routine finds the predecessor of a node in a splay tree.
 *
 * \param Links A pointer to the splay links of the node.
 * \return A pointer to the splay links of the predecessor node.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlsubtreepredecessor
 */
_Check_return_
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlSubtreePredecessor(
    _In_ PRTL_SPLAY_LINKS Links
    );

/**
 * The RtlRealSuccessor routine finds the real successor of a node in a splay tree.
 *
 * \param Links A pointer to the splay links of the node.
 * \return A pointer to the splay links of the real successor node.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlrealsuccessor
 */
_Check_return_
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlRealSuccessor(
    _In_ PRTL_SPLAY_LINKS Links
    );

/**
 * The RtlRealPredecessor routine finds the real predecessor of a node in a splay tree.
 *
 * \param Links A pointer to the splay links of the node.
 * \return A pointer to the splay links of the real predecessor node.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlrealpredecessor
 */
_Check_return_
NTSYSAPI
PRTL_SPLAY_LINKS
NTAPI
RtlRealPredecessor(
    _In_ PRTL_SPLAY_LINKS Links
    );

/**
 * Forward declaration of the RTL_GENERIC_TABLE structure.
 */
typedef struct _RTL_GENERIC_TABLE RTL_GENERIC_TABLE, *PRTL_GENERIC_TABLE;

typedef _Function_class_(RTL_GENERIC_COMPARE_ROUTINE)
RTL_GENERIC_COMPARE_RESULTS NTAPI RTL_GENERIC_COMPARE_ROUTINE(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ PVOID FirstStruct,
    _In_ PVOID SecondStruct
    );
/**
 * Pointer to an RTL_GENERIC_COMPARE_ROUTINE callback.
 */
typedef RTL_GENERIC_COMPARE_ROUTINE* PRTL_GENERIC_COMPARE_ROUTINE;

typedef _Function_class_(RTL_GENERIC_FREE_ROUTINE)
VOID NTAPI RTL_GENERIC_FREE_ROUTINE(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ _Post_invalid_ PVOID Buffer
    );
/**
 * Pointer to an RTL_GENERIC_FREE_ROUTINE callback.
 */
typedef RTL_GENERIC_FREE_ROUTINE* PRTL_GENERIC_FREE_ROUTINE;

typedef _Function_class_(RTL_GENERIC_ALLOCATE_ROUTINE)
PVOID NTAPI RTL_GENERIC_ALLOCATE_ROUTINE(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ CLONG ByteSize
    );
/**
 * Pointer to an RTL_GENERIC_ALLOCATE_ROUTINE callback.
 */
typedef RTL_GENERIC_ALLOCATE_ROUTINE* PRTL_GENERIC_ALLOCATE_ROUTINE;

/**
 * Represents a generic table implemented as a splay tree.
 */
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

/**
 * The RtlInitializeGenericTable routine initializes a generic table.
 *
 * \param Table A pointer to the `RTL_GENERIC_TABLE` structure to be initialized.
 * \param CompareRoutine A comparison routine to be used for comparing elements.
 * \param AllocateRoutine An allocation routine to be used for allocating memory for the table.
 * \param FreeRoutine A free routine to be used for freeing memory allocated for the table.
 * \param TableContext A context to be passed to the comparison, allocation, and free routines.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlinitializegenerictable
 */
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

/**
 * The RtlInsertElementGenericTable routine inserts an element into a generic table.
 *
 * \param Table A pointer to the generic table.
 * \param Buffer A pointer to the buffer containing the element to be inserted.
 * \param BufferSize The size of the buffer.
 * \param NewElement A pointer to a boolean that receives `TRUE` if the element was newly inserted, or `FALSE` if the element already existed.
 * \return A pointer to the newly inserted or existing element.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlinsertelementgenerictable
 */
NTSYSAPI
PVOID
NTAPI
RtlInsertElementGenericTable(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_reads_bytes_(BufferSize) PVOID Buffer,
    _In_ CLONG BufferSize,
    _Out_opt_ PBOOLEAN NewElement
    );

/**
 * The RtlInsertElementGenericTableFull routine inserts an element into a generic table.
 *
 * \param Table A pointer to the generic table.
 * \param Buffer A pointer to the buffer containing the element to be inserted.
 * \param BufferSize The size of the buffer.
 * \param NewElement A pointer to a boolean that receives `TRUE` if the element was newly inserted, or `FALSE` if the element already existed.
 * \param NodeOrParent A pointer to the node or parent for the insertion.
 * \param SearchResult The result of the search for the insertion point.
 * \return A pointer to the newly inserted or existing element.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlinsertelementgenerictablefull
 */
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

/**
 * The RtlDeleteElementGenericTable routine deletes the element matching the supplied key from a generic table.
 *
 * \param Table The generic table to modify.
 * \param Buffer A buffer containing the key that identifies the element to delete.
 * \return TRUE if the element was found and deleted; otherwise, FALSE.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlDeleteElementGenericTable(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ PVOID Buffer
    );

/**
 * The RtlLookupElementGenericTable routine finds the element matching the supplied key in a generic table.
 *
 * \param Table The generic table to search.
 * \param Buffer A buffer containing the key that identifies the element to find.
 * \return A pointer to the user data of the matching element, or NULL if not found.
 */
_Check_return_
NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTable(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ PVOID Buffer
    );

/**
 * The RtlLookupElementGenericTableFull routine finds the element matching the supplied key in a generic table and returns its node position.
 *
 * \param Table The generic table to search.
 * \param Buffer A buffer containing the key that identifies the element to find.
 * \param NodeOrParent Receives the matching node, or its prospective parent if not found.
 * \param SearchResult Receives the result of the search describing NodeOrParent.
 * \return A pointer to the user data of the matching element, or NULL if not found.
 */
NTSYSAPI
PVOID
NTAPI
RtlLookupElementGenericTableFull(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ PVOID Buffer,
    _Out_ PVOID *NodeOrParent,
    _Out_ TABLE_SEARCH_RESULT *SearchResult
    );

/**
 * The RtlEnumerateGenericTable routine returns the elements of a generic table in order, one per call.
 *
 * \param Table The generic table to enumerate.
 * \param Restart TRUE to restart the enumeration from the first element; FALSE to continue.
 * \return A pointer to the user data of the next element, or NULL when enumeration is complete.
 */
_Check_return_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTable(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ BOOLEAN Restart
    );

/**
 * The RtlEnumerateGenericTableWithoutSplaying routine returns the elements of a generic table in order without splaying the underlying tree.
 *
 * \param Table The generic table to enumerate.
 * \param RestartKey On input, NULL restarts the enumeration; receives a key used to continue enumeration.
 * \return A pointer to the user data of the next element, or NULL when enumeration is complete.
 */
_Check_return_
NTSYSAPI
PVOID
NTAPI
RtlEnumerateGenericTableWithoutSplaying(
    _In_ PRTL_GENERIC_TABLE Table,
    _Inout_ PVOID *RestartKey
    );

/**
 * The RtlGetElementGenericTable routine returns the element at the specified ordinal position in a generic table.
 *
 * \param Table The generic table to query.
 * \param I The zero-based index of the element to retrieve.
 * \return A pointer to the user data of the element, or NULL if the index is out of range.
 */
_Check_return_
NTSYSAPI
PVOID
NTAPI
RtlGetElementGenericTable(
    _In_ PRTL_GENERIC_TABLE Table,
    _In_ ULONG I
    );

/**
 * The RtlNumberGenericTableElements routine returns the number of elements currently in a generic table.
 *
 * \param Table A pointer to the generic table to query.
 * \return The number of elements in the generic table.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlnumbergenerictableelements
 */
NTSYSAPI
ULONG
NTAPI
RtlNumberGenericTableElements(
    _In_ PRTL_GENERIC_TABLE Table
    );

/**
 * The RtlIsGenericTableEmpty routine indicates whether a generic table currently contains any elements.
 *
 * \param Table A pointer to the generic table to query.
 * \return `TRUE` if the generic table is empty, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlisgenerictableempty
 */
_Check_return_
NTSYSAPI
BOOLEAN
NTAPI
RtlIsGenericTableEmpty(
    _In_ PRTL_GENERIC_TABLE Table
    );

//
// AVL Trees
//

/**
 * The RtlAvlRemoveNode routine removes a node from an Adelson-Velsky/Landis (AVL) balanced binary tree.
 *
 * \param Root A pointer to the root pointer of the AVL tree.
 * \param Node A pointer to the balanced node to remove from the tree.
 */
NTSYSAPI
VOID
NTAPI
RtlAvlRemoveNode(
    _Inout_ PRTL_BALANCED_NODE *Root,
    _In_ PRTL_BALANCED_NODE Node
    );

//
// RB trees
//

/**
 * Represents the root of a red-black tree.
 */
typedef struct _RTL_RB_TREE
{
    PRTL_BALANCED_NODE Root;
    union
    {
        UCHAR Encoded : 1;
        PRTL_BALANCED_NODE Min;
    };
} RTL_RB_TREE, *PRTL_RB_TREE;

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// rev
/**
 * The RtlRbInsertNodeEx routine inserts a node into a red-black balanced binary tree at a specified position.
 *
 * \param Tree A pointer to the red-black tree.
 * \param Parent An optional pointer to the parent node under which the new node is inserted.
 * \param Right If `TRUE`, the node is inserted as the right child of the parent; otherwise as the left child.
 * \param Node A pointer to the balanced node to insert into the tree.
 * \return `TRUE` if the node was inserted successfully, otherwise `FALSE`.
 */
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
/**
 * The RtlRbRemoveNode routine removes a node from a red-black balanced binary tree.
 *
 * \param Tree A pointer to the red-black tree.
 * \param Node A pointer to the balanced node to remove from the tree.
 */
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
/**
 * The RtlCompareExchangePointerMapping routine atomically exchanges a pair of pointer-mapping nodes in a red-black tree.
 *
 * \param Node1 A pointer to the first balanced node.
 * \param Node2 A pointer to the second balanced node.
 * \param Node3 A pointer to a variable that receives a balanced node.
 * \param Node4 A pointer to a variable that receives a balanced node.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlQueryPointerMapping routine queries the pointer-mapping entries associated with a red-black tree.
 *
 * \param Tree A pointer to the red-black tree.
 * \param Children A pointer to a balanced node that receives the queried mapping.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryPointerMapping(
    _In_ PRTL_RB_TREE Tree,
    _Inout_ PRTL_BALANCED_NODE Children
    );

// rev
/**
 * The RtlRemovePointerMapping routine removes the pointer-mapping entries associated with a red-black tree.
 *
 * \param Tree A pointer to the red-black tree.
 * \param Children A pointer to a balanced node describing the mapping to remove.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Flags and reserved values for dynamic hash table entries.
 */
#define RTL_HASH_ALLOCATED_HEADER 0x00000001
#define RTL_HASH_RESERVED_SIGNATURE 0

/**
 * Represents an entry stored in a dynamically sizing hash table.
 */
typedef struct _RTL_DYNAMIC_HASH_TABLE_ENTRY
{
    LIST_ENTRY Linkage;
    ULONG_PTR Signature;
} RTL_DYNAMIC_HASH_TABLE_ENTRY, *PRTL_DYNAMIC_HASH_TABLE_ENTRY;

/**
 * Retrieves the key (signature) of a dynamic hash table entry.
 */
#define HASH_ENTRY_KEY(x) ((x)->Signature)

/**
 * Caches a lookup position within a dynamic hash table to accelerate subsequent operations.
 */
typedef struct _RTL_DYNAMIC_HASH_TABLE_CONTEXT
{
    PLIST_ENTRY ChainHead;
    PLIST_ENTRY PrevLinkage;
    ULONG_PTR Signature;
} RTL_DYNAMIC_HASH_TABLE_CONTEXT, *PRTL_DYNAMIC_HASH_TABLE_CONTEXT;

/**
 * Maintains the state required to enumerate the entries of a dynamic hash table.
 */
typedef struct _RTL_DYNAMIC_HASH_TABLE_ENUMERATOR
{
    union
    {
        RTL_DYNAMIC_HASH_TABLE_ENTRY HashEntry;
        PLIST_ENTRY CurEntry;
    };
    PLIST_ENTRY ChainHead;
    ULONG BucketIndex;
} RTL_DYNAMIC_HASH_TABLE_ENUMERATOR, *PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR;

/**
 * Represents a dynamically sizing (expanding and contracting) hash table.
 */
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

/**
 * The RtlInitHashTableContext routine initializes a dynamic hash table context for use in subsequent lookup operations.
 *
 * \param Context A pointer to the dynamic hash table context to initialize.
 */
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

/**
 * The RtlInitHashTableContextFromEnumerator routine initializes a dynamic hash table context from an active enumerator.
 *
 * \param Context A pointer to the dynamic hash table context to initialize.
 * \param Enumerator A pointer to the enumerator whose position initializes the context.
 */
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

/**
 * The RtlReleaseHashTableContext routine releases any resources associated with a dynamic hash table context.
 *
 * \param Context A pointer to the dynamic hash table context to release.
 */
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

/**
 * The RtlTotalBucketsHashTable routine returns the total number of buckets in a dynamic hash table.
 *
 * \param HashTable A pointer to the dynamic hash table to query.
 * \return ULONG The total number of buckets in the hash table.
 */
FORCEINLINE
ULONG
NTAPI_INLINE
RtlTotalBucketsHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    )
{
    return HashTable->TableSize;
}

/**
 * The RtlNonEmptyBucketsHashTable routine returns the number of non-empty buckets in a dynamic hash table.
 *
 * \param HashTable A pointer to the dynamic hash table to query.
 * \return ULONG The number of non-empty buckets in the hash table.
 */
FORCEINLINE
ULONG
NTAPI_INLINE
RtlNonEmptyBucketsHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    )
{
    return HashTable->NonEmptyBuckets;
}

/**
 * The RtlEmptyBucketsHashTable routine returns the number of empty buckets in a dynamic hash table.
 *
 * \param HashTable A pointer to the dynamic hash table to query.
 * \return ULONG The number of empty buckets in the hash table.
 */
FORCEINLINE
ULONG
NTAPI_INLINE
RtlEmptyBucketsHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    )
{
    return HashTable->TableSize - HashTable->NonEmptyBuckets;
}

/**
 * The RtlTotalEntriesHashTable routine returns the total number of entries in a dynamic hash table.
 *
 * \param HashTable A pointer to the dynamic hash table to query.
 * \return ULONG The total number of entries in the hash table.
 */
FORCEINLINE
ULONG
NTAPI_INLINE
RtlTotalEntriesHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    )
{
    return HashTable->NumEntries;
}

/**
 * The RtlActiveEnumeratorsHashTable routine returns the number of active enumerators on a dynamic hash table.
 *
 * \param HashTable A pointer to the dynamic hash table to query.
 * \return ULONG The number of active enumerators on the hash table.
 */
FORCEINLINE
ULONG
NTAPI_INLINE
RtlActiveEnumeratorsHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    )
{
    return HashTable->NumEnumerators;
}

/**
 * The RtlCreateHashTable routine creates and initializes a dynamic hash table.
 *
 * \param HashTable A pointer to a variable that receives the created hash table. If the variable is `NULL`, the routine allocates the hash table.
 * \param Shift The number of reserved low-order bits of the hash value, used to control the number of buckets.
 * \param Flags Reserved. Must be zero.
 * \return `TRUE` if the hash table was created successfully, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlcreatehashtable
 */
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlCreateHashTable(
    _Inout_ _When_(*HashTable == NULL, __drv_allocatesMem(Mem)) PRTL_DYNAMIC_HASH_TABLE *HashTable,
    _In_ ULONG Shift,
    _In_ _Reserved_ ULONG Flags
    );

/**
 * The RtlCreateHashTableEx routine creates and initializes a dynamic hash table with an initial size.
 *
 * \param HashTable A pointer to a variable that receives the created hash table. If the variable is `NULL`, the routine allocates the hash table.
 * \param InitialSize The initial number of buckets to allocate for the hash table.
 * \param Shift The number of reserved low-order bits of the hash value, used to control the number of buckets.
 * \param Flags Reserved. Must be zero.
 * \return `TRUE` if the hash table was created successfully, otherwise `FALSE`.
 */
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

/**
 * The RtlDeleteHashTable routine deletes a dynamic hash table and frees the resources allocated for it.
 *
 * \param HashTable A pointer to the dynamic hash table to delete.
 * \return `TRUE` if the hash table was deleted successfully, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtldeletehashtable
 */
NTSYSAPI
LOGICAL
NTAPI
RtlDeleteHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    );

/**
 * The RtlInsertEntryHashTable routine inserts an entry into a dynamic hash table.
 *
 * \param HashTable A pointer to the dynamic hash table.
 * \param Entry A pointer to the entry to insert.
 * \param Signature The signature (hash value) for the entry.
 * \param Context An optional pointer to a context structure that can be used to optimize the insertion.
 * \return `TRUE` if the entry was inserted successfully, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlinsertentryhashtable
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlInsertEntryHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _In_ PRTL_DYNAMIC_HASH_TABLE_ENTRY Entry,
    _In_ ULONG_PTR Signature,
    _Inout_opt_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context
    );

/**
 * The RtlRemoveEntryHashTable routine removes an entry from a dynamic hash table.
 *
 * \param HashTable A pointer to the dynamic hash table.
 * \param Entry A pointer to the entry to remove.
 * \param Context An optional pointer to a context structure that can be used to optimize the removal.
 * \return `TRUE` if the entry was removed successfully, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlremoveentryhashtable
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlRemoveEntryHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _In_ PRTL_DYNAMIC_HASH_TABLE_ENTRY Entry,
    _Inout_opt_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context
    );

/**
 * The RtlLookupEntryHashTable routine looks up an entry in a dynamic hash table.
 *
 * \param HashTable A pointer to the dynamic hash table.
 * \param Signature The signature (hash value) of the entry to look up.
 * \param Context An optional pointer to a context structure that receives information used to enumerate matching entries.
 * \return A pointer to the first matching entry, or `NULL` if no matching entry was found.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtllookupentryhashtable
 */
_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlLookupEntryHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _In_ ULONG_PTR Signature,
    _Out_opt_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context
    );

/**
 * The RtlGetNextEntryHashTable routine retrieves the next entry that matches a previous lookup in a dynamic hash table.
 *
 * \param HashTable A pointer to the dynamic hash table.
 * \param Context A pointer to a context structure initialized by a previous call to RtlLookupEntryHashTable.
 * \return A pointer to the next matching entry, or `NULL` if there are no more matching entries.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlgetnextentryhashtable
 */
_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlGetNextEntryHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _In_ PRTL_DYNAMIC_HASH_TABLE_CONTEXT Context
    );

/**
 * The RtlInitEnumerationHashTable routine initializes an enumerator for a strong enumeration of a dynamic hash table.
 *
 * \param HashTable A pointer to the dynamic hash table.
 * \param Enumerator A pointer to the enumerator structure to initialize.
 * \return `TRUE` if the enumerator was initialized successfully, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlinitenumerationhashtable
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlInitEnumerationHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Out_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );

/**
 * The RtlEnumerateEntryHashTable routine retrieves the next entry in a dynamic hash table enumeration.
 *
 * \param HashTable A pointer to the dynamic hash table.
 * \param Enumerator A pointer to the enumerator initialized by RtlInitEnumerationHashTable.
 * \return A pointer to the next entry in the enumeration, or `NULL` if the enumeration is complete.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlenumerateentryhashtable
 */
_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlEnumerateEntryHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );

/**
 * The RtlEndEnumerationHashTable routine terminates an enumeration of a dynamic hash table and releases the resources associated with the enumerator.
 *
 * \param HashTable A pointer to the dynamic hash table.
 * \param Enumerator A pointer to the enumerator to terminate.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlendenumerationhashtable
 */
NTSYSAPI
VOID
NTAPI
RtlEndEnumerationHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );

/**
 * The RtlInitWeakEnumerationHashTable routine initializes an enumerator for a weak enumeration of a dynamic hash table.
 *
 * \param HashTable A pointer to the dynamic hash table.
 * \param Enumerator A pointer to the enumerator structure to initialize.
 * \return `TRUE` if the enumerator was initialized successfully, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlinitweakenumerationhashtable
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlInitWeakEnumerationHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Out_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );

/**
 * The RtlWeaklyEnumerateEntryHashTable routine retrieves the next entry in a weak enumeration of a dynamic hash table.
 *
 * \param HashTable A pointer to the dynamic hash table.
 * \param Enumerator A pointer to the enumerator initialized by RtlInitWeakEnumerationHashTable.
 * \return A pointer to the next entry in the enumeration, or `NULL` if the enumeration is complete.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlweaklyenumerateentryhashtable
 */
_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlWeaklyEnumerateEntryHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );

/**
 * The RtlEndWeakEnumerationHashTable routine terminates a weak enumeration of a dynamic hash table and releases the resources associated with the enumerator.
 *
 * \param HashTable A pointer to the dynamic hash table.
 * \param Enumerator A pointer to the enumerator to terminate.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlendweakenumerationhashtable
 */
NTSYSAPI
VOID
NTAPI
RtlEndWeakEnumerationHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );

/**
 * The RtlExpandHashTable routine expands a dynamic hash table by increasing the number of buckets.
 *
 * \param HashTable A pointer to the dynamic hash table.
 * \return `TRUE` if the hash table was expanded, otherwise `FALSE` if no expansion was performed.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlexpandhashtable
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlExpandHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    );

/**
 * The RtlContractHashTable routine contracts a dynamic hash table by reducing the number of buckets.
 *
 * \param HashTable A pointer to the dynamic hash table.
 * \return `TRUE` if the hash table was contracted, otherwise `FALSE` if no contraction was performed.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlcontracthashtable
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlContractHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10)

/**
 * The RtlInitStrongEnumerationHashTable routine initializes an enumerator for a strong enumeration of a dynamic hash table.
 *
 * \param HashTable A pointer to the dynamic hash table.
 * \param Enumerator A pointer to the enumerator structure to initialize.
 * \return `TRUE` if the enumerator was initialized successfully, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlinitstrongenumerationhashtable
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlInitStrongEnumerationHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Out_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );

/**
 * The RtlStronglyEnumerateEntryHashTable routine retrieves the next entry in a strong enumeration of a dynamic hash table.
 *
 * \param HashTable A pointer to the dynamic hash table.
 * \param Enumerator A pointer to the enumerator initialized by RtlInitStrongEnumerationHashTable.
 * \return A pointer to the next entry in the enumeration, or `NULL` if the enumeration is complete.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlstronglyenumerateentryhashtable
 */
_Must_inspect_result_
NTSYSAPI
PRTL_DYNAMIC_HASH_TABLE_ENTRY
NTAPI
RtlStronglyEnumerateEntryHashTable(
    _In_ PRTL_DYNAMIC_HASH_TABLE HashTable,
    _Inout_ PRTL_DYNAMIC_HASH_TABLE_ENUMERATOR Enumerator
    );

/**
 * The RtlEndStrongEnumerationHashTable routine terminates a strong enumeration of a dynamic hash table and releases the resources associated with the enumerator.
 *
 * \param HashTable A pointer to the dynamic hash table.
 * \param Enumerator A pointer to the enumerator to terminate.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlendstrongenumerationhashtable
 */
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
/**
 * Flags controlling critical section initialization behavior.
 */
#define RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO         0x01000000
#define RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN          0x02000000
#define RTL_CRITICAL_SECTION_FLAG_STATIC_INIT           0x04000000
#define RTL_CRITICAL_SECTION_FLAG_RESOURCE_TYPE         0x08000000
#define RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO      0x10000000
#define RTL_CRITICAL_SECTION_ALL_FLAG_BITS              0xFF000000
#define RTL_CRITICAL_SECTION_FLAG_RESERVED              (RTL_CRITICAL_SECTION_ALL_FLAG_BITS & (~(RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO | RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN | RTL_CRITICAL_SECTION_FLAG_STATIC_INIT | RTL_CRITICAL_SECTION_FLAG_RESOURCE_TYPE | RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO)))
// These flags define possible values stored in the Flags field of a critsec debuginfo.
/**
 * Flag marking a statically initialized critical section debug information block.
 */
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

/**
 * The RtlInitializeCriticalSection routine initializes a critical section object.
 *
 * \param CriticalSection A pointer to the critical section object.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializecriticalsection
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeCriticalSection(
    _Out_ PRTL_CRITICAL_SECTION CriticalSection
    );

/**
 * The RtlInitializeCriticalSectionAndSpinCount routine initializes a critical section object and sets the spin count for the critical section.
 *
 * \param CriticalSection A pointer to the critical section object.
 * \param SpinCount The spin count for the critical section object. On single-processor systems, the spin count is ignored.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializecriticalsectionandspincount
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeCriticalSectionAndSpinCount(
    _Out_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_ ULONG SpinCount
    );

/**
 * The RtlInitializeCriticalSectionEx routine initializes a critical section object and sets the spin count for the critical section with flags.
 *
 * \param CriticalSection A pointer to the critical section object.
 * \param SpinCount The spin count for the critical section object. On single-processor systems, the spin count is ignored.
 * \param Flags This parameter can be 0 or the CRITICAL_SECTION_NO_DEBUG_INFO flag.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializecriticalsectionex
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeCriticalSectionEx(
    _Out_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_ ULONG SpinCount,
    _In_ ULONG Flags
    );

/**
 * The RtlDeleteCriticalSection routine releases all resources used by a critical section object that is no longer needed.
 *
 * \param CriticalSection A pointer to the critical section object to delete.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );

/**
 * The RtlEnterCriticalSection routine waits for ownership of the specified critical section object, blocking the calling thread until the object is available.
 *
 * \param CriticalSection A pointer to the critical section object.
 * \return NTSTATUS Successful or errant status.
 */
_Acquires_exclusive_lock_(*CriticalSection)
NTSYSAPI
NTSTATUS
NTAPI
RtlEnterCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );

/**
 * The RtlLeaveCriticalSection routine releases ownership of the specified critical section object.
 *
 * \param CriticalSection A pointer to the critical section object.
 * \return NTSTATUS Successful or errant status.
 */
_Releases_exclusive_lock_(*CriticalSection)
NTSYSAPI
NTSTATUS
NTAPI
RtlLeaveCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );

/**
 * The RtlTryEnterCriticalSection routine attempts to enter a critical section without blocking the calling thread.
 *
 * \param CriticalSection A pointer to the critical section object.
 * \return A nonzero value if the critical section was successfully entered, otherwise zero.
 */
_When_(return != 0, _Acquires_exclusive_lock_(*CriticalSection))
NTSYSAPI
LOGICAL
NTAPI
RtlTryEnterCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );

/**
 * The RtlIsCriticalSectionLocked routine determines whether a critical section is currently owned by any thread.
 *
 * \param CriticalSection A pointer to the critical section object.
 * \return A nonzero value if the critical section is locked, otherwise zero.
 */
NTSYSAPI
LOGICAL
NTAPI
RtlIsCriticalSectionLocked(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
    );

/**
 * The RtlIsCriticalSectionLockedByThread routine determines whether a critical section is currently owned by the calling thread.
 *
 * \param CriticalSection A pointer to the critical section object.
 * \return A nonzero value if the critical section is owned by the calling thread, otherwise zero.
 */
NTSYSAPI
LOGICAL
NTAPI
RtlIsCriticalSectionLockedByThread(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
    );

/**
 * The RtlGetCriticalSectionRecursionCount routine returns the recursion (re-entry) count of a critical section owned by the calling thread.
 *
 * \param CriticalSection A pointer to the critical section object.
 * \return The recursion count of the critical section.
 */
NTSYSAPI
ULONG
NTAPI
RtlGetCriticalSectionRecursionCount(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
    );

/**
 * The RtlSetCriticalSectionSpinCount routine sets the spin count for a critical section object.
 *
 * \param CriticalSection A pointer to the critical section object.
 * \param SpinCount The spin count for the critical section object.
 * \return The previous spin count for the critical section object.
 */
NTSYSAPI
ULONG
NTAPI
RtlSetCriticalSectionSpinCount(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_ ULONG SpinCount
    );

/**
 * The RtlQueryCriticalSectionOwner routine returns the thread that owns the critical section associated with the specified event handle.
 *
 * \param EventHandle A handle to the event used by the critical section's lock semaphore.
 * \param ExactMatchOwnerAddress If `TRUE`, requires an exact match of the owner address.
 * \return A handle to the owning thread, or `NULL` if the critical section is not owned.
 */
NTSYSAPI
HANDLE
NTAPI
RtlQueryCriticalSectionOwner(
    _In_ HANDLE EventHandle,
    _In_ BOOLEAN ExactMatchOwnerAddress
    );

/**
 * The RtlCheckForOrphanedCriticalSections routine checks whether the specified thread owns any critical sections that would be orphaned if the thread terminated.
 *
 * \param ThreadHandle A handle to the thread to check.
 */
NTSYSAPI
VOID
NTAPI
RtlCheckForOrphanedCriticalSections(
    _In_ HANDLE ThreadHandle
    );

/**
 * The RtlEnableEarlyCriticalSectionEventCreation routine enables the creation of early critical section events.
 *
 * This function allows the system to create critical section events early in the process
 * initialization. It is typically used to ensure that critical sections are properly
 * initialized and can be used safely during the early stages of process startup.
 * \remarks This function sets the FLG_CRITSEC_EVENT_CREATION flag in the PEB flags field.
 * \return A pointer to the Process Environment Block (PEB).
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

/**
 * Represents a reader/writer resource synchronization object.
 */
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

/**
 * Flag indicating that an RTL_RESOURCE is held for a long term.
 */
#define RTL_RESOURCE_FLAG_LONG_TERM ((ULONG)0x00000001)

/**
 * The RtlInitializeResource routine initializes a resource variable.
 *
 * \param Resource A pointer to the resource variable to initialize.
 */
NTSYSAPI
VOID
NTAPI
RtlInitializeResource(
    _Out_ PRTL_RESOURCE Resource
    );

/**
 * The RtlDeleteResource routine releases all resources used by a resource variable that is no longer needed.
 *
 * \param Resource A pointer to the resource variable to delete.
 */
NTSYSAPI
VOID
NTAPI
RtlDeleteResource(
    _Inout_ PRTL_RESOURCE Resource
    );

/**
 * The RtlAcquireResourceShared routine acquires the specified resource variable for shared access.
 *
 * \param Resource A pointer to the resource variable.
 * \param Wait If `TRUE`, the calling thread waits until the resource can be acquired; if `FALSE`, the routine returns immediately.
 * \return `TRUE` if the resource was acquired, otherwise `FALSE`.
 */
_When_(return != 0, _Acquires_shared_lock_(*Resource))
NTSYSAPI
BOOLEAN
NTAPI
RtlAcquireResourceShared(
    _Inout_ PRTL_RESOURCE Resource,
    _In_ BOOLEAN Wait
    );

/**
 * The RtlAcquireResourceExclusive routine acquires the specified resource variable for exclusive access.
 *
 * \param Resource A pointer to the resource variable.
 * \param Wait If `TRUE`, the calling thread waits until the resource can be acquired; if `FALSE`, the routine returns immediately.
 * \return `TRUE` if the resource was acquired, otherwise `FALSE`.
 */
_When_(return != 0, _Acquires_exclusive_lock_(*Resource))
NTSYSAPI
BOOLEAN
NTAPI
RtlAcquireResourceExclusive(
    _Inout_ PRTL_RESOURCE Resource,
    _In_ BOOLEAN Wait
    );

/**
 * The RtlReleaseResource routine releases a resource variable that was acquired for shared or exclusive access.
 *
 * \param Resource A pointer to the resource variable.
 */
_Releases_lock_(*Resource)
NTSYSAPI
VOID
NTAPI
RtlReleaseResource(
    _Inout_ PRTL_RESOURCE Resource
    );

/**
 * The RtlConvertSharedToExclusive routine converts shared access of a resource variable to exclusive access.
 *
 * \param Resource A pointer to the resource variable.
 */
NTSYSAPI
VOID
NTAPI
RtlConvertSharedToExclusive(
    _Inout_ PRTL_RESOURCE Resource
    );

/**
 * The RtlConvertExclusiveToShared routine converts exclusive access of a resource variable to shared access.
 *
 * \param Resource A pointer to the resource variable.
 */
NTSYSAPI
VOID
NTAPI
RtlConvertExclusiveToShared(
    _Inout_ PRTL_RESOURCE Resource
    );

/**
 * The RtlDumpResource routine dumps diagnostic information about the specified resource variable.
 *
 * \param Resource A pointer to the resource variable.
 * \return The number of threads currently waiting on the resource.
 */
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
/**
 * Static initializer for a slim reader/writer (SRW) lock.
 */
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
/**
 * The RtlAcquireSRWLockExclusive routine acquires a slim reader/writer (SRW) lock in exclusive mode.
 *
 * \param SRWLock A pointer to the SRW lock.
 */
_Acquires_exclusive_lock_(*SRWLock)
NTSYSAPI
VOID
NTAPI
RtlAcquireSRWLockExclusive(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

// winbase:AcquireSRWLockShared
/**
 * The RtlAcquireSRWLockShared routine acquires a slim reader/writer (SRW) lock in shared mode.
 *
 * \param SRWLock A pointer to the SRW lock.
 */
_Acquires_shared_lock_(*SRWLock)
NTSYSAPI
VOID
NTAPI
RtlAcquireSRWLockShared(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

// winbase:ReleaseSRWLockExclusive
/**
 * The RtlReleaseSRWLockExclusive routine releases a slim reader/writer (SRW) lock that was acquired in exclusive mode.
 *
 * \param SRWLock A pointer to the SRW lock.
 */
_Releases_exclusive_lock_(*SRWLock)
NTSYSAPI
VOID
NTAPI
RtlReleaseSRWLockExclusive(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

// winbase:ReleaseSRWLockShared
/**
 * The RtlReleaseSRWLockShared routine releases a slim reader/writer (SRW) lock that was acquired in shared mode.
 *
 * \param SRWLock A pointer to the SRW lock.
 */
_Releases_shared_lock_(*SRWLock)
NTSYSAPI
VOID
NTAPI
RtlReleaseSRWLockShared(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

// winbase:TryAcquireSRWLockExclusive
/**
 * The RtlTryAcquireSRWLockExclusive routine attempts to acquire a slim reader/writer (SRW) lock in exclusive mode without blocking.
 *
 * \param SRWLock A pointer to the SRW lock.
 * \return `TRUE` if the lock was acquired, otherwise `FALSE`.
 */
_When_(return != 0, _Acquires_exclusive_lock_(*SRWLock))
NTSYSAPI
BOOLEAN
NTAPI
RtlTryAcquireSRWLockExclusive(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

// winbase:TryAcquireSRWLockShared
/**
 * The RtlTryAcquireSRWLockShared routine attempts to acquire a slim reader/writer (SRW) lock in shared mode without blocking.
 *
 * \param SRWLock A pointer to the SRW lock.
 * \return `TRUE` if the lock was acquired, otherwise `FALSE`.
 */
_When_(return != 0, _Acquires_shared_lock_(*SRWLock))
NTSYSAPI
BOOLEAN
NTAPI
RtlTryAcquireSRWLockShared(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

// rev
/**
 * The RtlAcquireReleaseSRWLockExclusive routine acquires and immediately releases a slim reader/writer (SRW) lock in exclusive mode, which can be used to drain pending exclusive waiters.
 *
 * \param SRWLock A pointer to the SRW lock.
 */
_Requires_lock_not_held_(*SRWLock)
NTSYSAPI
VOID
NTAPI
RtlAcquireReleaseSRWLockExclusive(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
// rev
/**
 * The RtlConvertSRWLockExclusiveToShared routine converts a slim reader/writer (SRW) lock held in exclusive mode to shared mode.
 *
 * \param SRWLock A pointer to the SRW lock.
 * \return `TRUE` if the conversion succeeded, otherwise `FALSE`.
 */
_Requires_exclusive_lock_held_(*SRWLock)
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

// rev
/**
 * Represents a per-thread registration entry within a read-copy-update (RCU) domain.
 */
typedef struct _RTL_RCU_THREAD_ENTRY 
{ 
    volatile long long RefCount;
    ULONG SessionId;
    ULONG ThreadId;
    volatile long long ObservedEpoch;
    struct _RTL_RCU_THREAD_ENTRY* Next;
} RTL_RCU_THREAD_ENTRY, *PRTL_RCU_THREAD_ENTRY;

// rev
/**
 * Represents a hash bucket array of registered RCU thread entries.
 */
typedef struct _RTL_RCU_BUCKET_ARRAY 
{ 
    ULONG Count;
    ULONG Reserved;
    PRTL_RCU_THREAD_ENTRY Slots[ANYSIZE_ARRAY];
    //struct _RTL_RCU_BUCKET_ARRAY* Next; // after Slots
} RTL_RCU_BUCKET_ARRAY, *PRTL_RCU_BUCKET_ARRAY;

// rev
/**
 * Represents the state of a read-copy-update (RCU) synchronization domain.
 */
typedef struct _RTL_RCU_STATE
{ 
    struct _RTL_RCU_STATE* Flink;
    struct _RTL_RCU_STATE* Blink;
    volatile long long Epoch;
    RTL_RCU_BUCKET_ARRAY* Buckets;
    PRTL_RCU_THREAD_ENTRY ThreadListHead;
    PRTL_RCU_THREAD_ENTRY BucketCache[10];
    RTL_SRWLOCK Lock;
    ULONG Options;
    ULONG ReservedTail;
} RTL_RCU_STATE, *PRTL_RCU_STATE;

/**
 * The RtlRcuAllocate routine allocates and initializes a read-copy-update (RCU) synchronization state.
 *
 * \param Options Options that control the created RCU state.
 * \return A pointer to the allocated RCU state, or `NULL` on failure.
 */
NTSYSAPI
PRTL_RCU_STATE
NTAPI
RtlRcuAllocate(
    _In_ ULONG Options
    );

/**
 * The RtlRcuFree routine frees a read-copy-update (RCU) synchronization state previously allocated by RtlRcuAllocate.
 *
 * \param State A pointer to the RCU state to free.
 */
NTSYSAPI
VOID
NTAPI
RtlRcuFree(
    _In_ PRTL_RCU_STATE State
    );

// Note: ThreadData can be NULL when it falls back to SRW share-lock.
/**
 * The RtlRcuReadLock routine enters a read-copy-update (RCU) read-side critical section.
 *
 * \param State A pointer to the RCU state.
 * \param ThreadData A pointer to a variable that receives the per-thread RCU entry associated with the read lock.
 */
_Acquires_lock_(*State)
NTSYSAPI
VOID
FASTCALL
RtlRcuReadLock(
    _Inout_ PRTL_RCU_STATE State,
    _Outptr_result_maybenull_ PRTL_RCU_THREAD_ENTRY* ThreadData
    );

/**
 * The RtlRcuReadUnlock routine leaves a read-copy-update (RCU) read-side critical section.
 *
 * \param State A pointer to the RCU state.
 * \param ThreadData A pointer to the per-thread RCU entry returned by RtlRcuReadLock.
 */
_Releases_lock_(*State)
NTSYSAPI
VOID
FASTCALL
RtlRcuReadUnlock(
    _Inout_ PRTL_RCU_STATE State,
    _In_ PRTL_RCU_THREAD_ENTRY* ThreadData
    );

/**
 * The RtlRcuSynchronize routine waits until all pre-existing read-copy-update (RCU) read-side critical sections have completed.
 *
 * \param State A pointer to the RCU state.
 */
_Requires_lock_not_held_(*State)
NTSYSAPI
VOID
NTAPI
RtlRcuSynchronize(
    _Inout_ PRTL_RCU_STATE State
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_11

/**
 * Static initializer and flags for a condition variable.
 */
#define RTL_CONDITION_VARIABLE_INIT {0}
#define RTL_CONDITION_VARIABLE_LOCKMODE_SHARED 0x1

// winbase:InitializeConditionVariable
/**
 * The RtlInitializeConditionVariable routine initializes a condition variable.
 *
 * \param ConditionVariable A pointer to the condition variable to initialize.
 */
NTSYSAPI
VOID
NTAPI
RtlInitializeConditionVariable(
    _Out_ PRTL_CONDITION_VARIABLE ConditionVariable
    );

// private
/**
 * The RtlSleepConditionVariableCS routine sleeps on a condition variable and releases the specified critical section as an atomic operation.
 *
 * \param ConditionVariable A pointer to the condition variable.
 * \param CriticalSection A pointer to the critical section associated with the condition variable.
 * \param Timeout An optional pointer to the time-out interval. If `NULL`, the routine waits indefinitely.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSleepConditionVariableCS(
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable,
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_opt_ PLARGE_INTEGER Timeout
    );

// private
/**
 * The RtlSleepConditionVariableSRW routine sleeps on a condition variable and releases the specified slim reader/writer (SRW) lock as an atomic operation.
 *
 * \param ConditionVariable A pointer to the condition variable.
 * \param SRWLock A pointer to the SRW lock associated with the condition variable.
 * \param Timeout An optional pointer to the time-out interval. If `NULL`, the routine waits indefinitely.
 * \param Flags If RTL_CONDITION_VARIABLE_LOCKMODE_SHARED is set, the SRW lock is held in shared mode; otherwise it is held in exclusive mode.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlWakeConditionVariable routine wakes a single thread waiting on the specified condition variable.
 *
 * \param ConditionVariable A pointer to the condition variable.
 */
NTSYSAPI
VOID
NTAPI
RtlWakeConditionVariable(
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable
    );

// winbase:WakeAllConditionVariable
/**
 * The RtlWakeAllConditionVariable routine wakes all threads waiting on the specified condition variable.
 *
 * \param ConditionVariable A pointer to the condition variable.
 */
NTSYSAPI
VOID
NTAPI
RtlWakeAllConditionVariable(
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable
    );

// begin_rev
/**
 * Flags controlling barrier wait behavior.
 */
#define RTL_BARRIER_FLAGS_SPIN_ONLY 0x00000001 // never block on event - always spin
#define RTL_BARRIER_FLAGS_BLOCK_ONLY 0x00000002 // always block on event - never spin
#define RTL_BARRIER_FLAGS_NO_DELETE 0x00000004 // use if barrier will never be deleted
// end_rev

// begin_private

/**
 * The RtlInitBarrier routine initializes a synchronization barrier for a specified number of threads.
 *
 * \param Barrier A pointer to the barrier to initialize.
 * \param TotalThreads The maximum number of threads that participate in the barrier.
 * \param SpinCount The number of times a thread spins while waiting for other threads to reach the barrier.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlInitBarrier(
    _Out_ PRTL_BARRIER Barrier,
    _In_ ULONG TotalThreads,
    _In_ ULONG SpinCount
    );

/**
 * The RtlDeleteBarrier routine releases all resources used by a synchronization barrier that is no longer needed.
 *
 * \param Barrier A pointer to the barrier to delete.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteBarrier(
    _In_ PRTL_BARRIER Barrier
    );

/**
 * The RtlBarrier routine causes the calling thread to wait at a synchronization barrier until the required number of threads have reached it.
 *
 * \param Barrier A pointer to the barrier.
 * \param Flags Flags that control the barrier behavior.
 * \return `TRUE` for the last thread to reach the barrier, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlBarrier(
    _Inout_ PRTL_BARRIER Barrier,
    _In_ ULONG Flags
    );

/**
 * The RtlBarrierForDelete routine waits at a synchronization barrier in preparation for deleting it.
 *
 * \param Barrier A pointer to the barrier.
 * \param Flags Flags that control the barrier behavior.
 * \return `TRUE` for the last thread to reach the barrier, otherwise `FALSE`.
 */
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

/**
 * The RtlWaitOnAddress routine waits for the value at the specified address to change.
 *
 * \param Address The address on which to wait.
 * \param CompareAddress A pointer to the location of the previously observed value at Address.
 * \param AddressSize The size of the value, in bytes. This parameter can be 1, 2, 4, or 8.
 * \param Timeout A pointer to the time-out value, in units of 100 nanoseconds. If this parameter is NULL, the thread waits indefinitely.
 * - A negative value specifies an interval relative to the current time.
 * - A positive value specifies an absolute time, measured in 100-nanosecond intervals since January 1, 1601 (UTC).
 * \remarks WaitOnAddress is guaranteed to return when the address is signaled, but it is also allowed to return for other reasons.
 * For this reason, the caller should compare the new value with the original undesired value to confirm that the value has actually changed.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlWaitOnAddress(
    _In_reads_bytes_(AddressSize) volatile VOID *Address,
    _In_reads_bytes_(AddressSize) PVOID CompareAddress,
    _In_ SIZE_T AddressSize,
    _In_opt_ PLARGE_INTEGER Timeout
    );

/**
 * The RtlWakeAddressAll routine wakes all threads that are waiting for the value of an address to change.
 *
 * \param Address The address to signal. If any threads have previously called RtlWaitOnAddress for this address, the system wakes all of the waiting threads.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-wakebyaddressall
 */
NTSYSAPI
VOID
NTAPI
RtlWakeAddressAll(
    _In_ PVOID Address
    );

/**
 * The RtlWakeAddressAllNoFence routine wakes all threads that are waiting for the value of an address to change.
 *
 * \param Address The address to signal. If any threads have previously called RtlWaitOnAddress for this address, the system wakes all of the waiting threads.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-wakebyaddressall
 */
NTSYSAPI
VOID
NTAPI
RtlWakeAddressAllNoFence(
    _In_ PVOID Address
    );

/**
 * The RtlWakeAddressSingle routine wakes one thread that is waiting for the value of an address to change.
 *
 * \param Address The address to signal.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-wakebyaddresssingle
 */
NTSYSAPI
VOID
NTAPI
RtlWakeAddressSingle(
    _In_ PVOID Address
    );

/**
 * The RtlWakeAddressSingleNoFence routine wakes one thread that is waiting for the value of an address to change.
 *
 * \param Address The address to signal.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-wakebyaddresssingle
 */
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

/**
 * The RtlInitEmptyAnsiString routine initializes an ANSI_STRING with a caller-supplied buffer and zero length.
 *
 * \param AnsiString A pointer to the ANSI_STRING structure to initialize.
 * \param Buffer A pointer to the caller-allocated buffer that backs the string.
 * \param MaximumLength The size, in bytes, of the buffer.
 */
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

/**
 * The RtlInitString routine initializes a counted ANSI string.
 *
 * \param DestinationString A pointer to the STRING structure to initialize.
 * \param SourceString An optional pointer to a null-terminated string used to initialize the counted string.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlinitstring
 */
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
/**
 * The RtlInitStringEx routine initializes a counted ANSI string and validates the length of the source string.
 *
 * \param DestinationString A pointer to the STRING structure to initialize.
 * \param SourceString An optional pointer to a null-terminated string used to initialize the counted string.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlInitStringEx(
    _Out_ PSTRING DestinationString,
    _In_opt_z_ __drv_aliasesMem PCSZ SourceString
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10

/**
 * The RtlInitAnsiString routine initializes a counted ANSI string.
 *
 * \param DestinationString A pointer to the ANSI_STRING structure to initialize.
 * \param SourceString An optional pointer to a null-terminated string used to initialize the counted string.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlinitansistring
 */
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

/**
 * The RtlInitAnsiStringEx routine initializes a counted ANSI string and validates the length of the source string.
 *
 * \param DestinationString A pointer to the ANSI_STRING structure to initialize.
 * \param SourceString An optional pointer to a null-terminated string used to initialize the counted string.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlinitansistringex
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlInitAnsiStringEx(
    _Out_ PANSI_STRING DestinationString,
    _In_opt_z_ __drv_aliasesMem PCSZ SourceString
    );

/**
 * The RtlFreeAnsiString routine releases storage that was allocated by RtlUnicodeStringToAnsiString.
 *
 * \param AnsiString A pointer to the ANSI string whose buffer is freed.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlfreeansistring
 */
NTSYSAPI
VOID
NTAPI
RtlFreeAnsiString(
    _Inout_ _At_(AnsiString->Buffer, _Frees_ptr_opt_) PANSI_STRING AnsiString
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)
/**
 * The RtlInitUTF8String routine initializes a counted UTF-8 string.
 *
 * \param DestinationString A pointer to the UTF8_STRING structure to initialize.
 * \param SourceString An optional pointer to a null-terminated string used to initialize the counted string.
 */
NTSYSAPI
VOID
NTAPI
RtlInitUTF8String(
    _Out_ PUTF8_STRING DestinationString,
    _In_opt_z_ PCSZ SourceString
    );

/**
 * The RtlInitUTF8StringEx routine initializes a counted UTF-8 string and validates the length of the source string.
 *
 * \param DestinationString A pointer to the UTF8_STRING structure to initialize.
 * \param SourceString An optional pointer to a null-terminated string used to initialize the counted string.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlInitUTF8StringEx(
    _Out_ PUTF8_STRING DestinationString,
    _In_opt_z_ __drv_aliasesMem PCSZ SourceString
    );

/**
 * The RtlFreeUTF8String routine releases storage that was allocated by RtlUnicodeStringToUTF8String.
 *
 * \param Utf8String A pointer to the UTF-8 string whose buffer is freed.
 */
NTSYSAPI
VOID
NTAPI
RtlFreeUTF8String(
    _Inout_ _At_(Utf8String->Buffer, _Frees_ptr_opt_) PUTF8_STRING Utf8String
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_20H1

/**
 * The RtlFreeOemString routine releases storage that was allocated by RtlUnicodeStringToOemString.
 *
 * \param OemString A pointer to the OEM string whose buffer is freed.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlfreeoemstring
 */
NTSYSAPI
VOID
NTAPI
RtlFreeOemString(
    _Inout_ POEM_STRING OemString
    );

/**
 * The RtlCopyString routine copies a source string to a destination string.
 *
 * \param DestinationString A pointer to the destination counted string.
 * \param SourceString An optional pointer to the source counted string. If `NULL`, the destination length is set to zero.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlcopystring
 */
NTSYSAPI
VOID
NTAPI
RtlCopyString(
    _In_ PSTRING DestinationString,
    _In_opt_ PSTRING SourceString
    );

/**
 * The RtlUpperChar routine converts the specified character to uppercase.
 *
 * \param Character The character to convert.
 * \return The uppercase equivalent of the specified character.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlupperchar
 */
NTSYSAPI
CHAR
NTAPI
RtlUpperChar(
    _In_ CHAR Character
    );

/**
 * The RtlCompareString routine compares two counted strings.
 *
 * \param String1 A pointer to the first counted string.
 * \param String2 A pointer to the second counted string.
 * \param CaseInSensitive If `TRUE`, case is ignored when comparing the strings.
 * \return A signed value that is negative, zero, or positive if String1 is less than, equal to, or greater than String2.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlcomparestring
 */
_Must_inspect_result_
NTSYSAPI
LONG
NTAPI
RtlCompareString(
    _In_ PSTRING String1,
    _In_ PSTRING String2,
    _In_ BOOLEAN CaseInSensitive
    );

/**
 * The RtlEqualString routine compares two counted strings to determine whether they are equal.
 *
 * \param String1 A pointer to the first counted string.
 * \param String2 A pointer to the second counted string.
 * \param CaseInSensitive If `TRUE`, case is ignored when comparing the strings.
 * \return `TRUE` if the strings are equal, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlequalstring
 */
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualString(
    _In_ PSTRING String1,
    _In_ PSTRING String2,
    _In_ BOOLEAN CaseInSensitive
    );

/**
 * The RtlPrefixString routine determines whether one counted string is a prefix of another.
 *
 * \param String1 A pointer to the counted string that is the potential prefix.
 * \param String2 A pointer to the counted string to search.
 * \param CaseInSensitive If `TRUE`, case is ignored when comparing the strings.
 * \return `TRUE` if String1 is a prefix of String2, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlprefixstring
 */
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlPrefixString(
    _In_ PSTRING String1,
    _In_ PSTRING String2,
    _In_ BOOLEAN CaseInSensitive
    );

/**
 * The RtlAppendStringToString routine concatenates two counted strings.
 *
 * \param Destination A pointer to the destination counted string.
 * \param Source A pointer to the source counted string to append.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlappendstringtostring
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAppendStringToString(
    _Inout_ PSTRING Destination,
    _In_ PSTRING Source
    );

/**
 * The RtlAppendAsciizToString routine concatenates a null-terminated string to a counted string.
 *
 * \param Destination A pointer to the destination counted string.
 * \param Source An optional pointer to the null-terminated source string to append.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlappendasciiztostring
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAppendAsciizToString(
    _Inout_ PSTRING Destination,
    _In_opt_z_ PCSTR Source
    );

/**
 * The RtlUpperString routine copies a source string to a destination string, converting each character to uppercase.
 *
 * \param DestinationString A pointer to the destination counted string.
 * \param SourceString A pointer to the source counted string.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlupperstring
 */
NTSYSAPI
VOID
NTAPI
RtlUpperString(
    _Inout_ PSTRING DestinationString,
    _In_ const STRING* SourceString
    );

/**
 * The RtlIsNullOrEmptyUnicodeString routine determines whether a UNICODE_STRING pointer is NULL or refers to a zero-length string.
 *
 * \param String An optional pointer to the UNICODE_STRING to test.
 * \return Returns `TRUE` if the string is NULL or empty, otherwise `FALSE`.
 */
FORCEINLINE
BOOLEAN
NTAPI_INLINE
RtlIsNullOrEmptyUnicodeString(
    _In_opt_ PCUNICODE_STRING String
    )
{
    return !String || String->Length == 0;
}

/**
 * The RtlInitEmptyUnicodeString routine initializes a UNICODE_STRING with a caller-supplied buffer and zero length.
 *
 * \param DestinationString A pointer to the UNICODE_STRING structure to initialize.
 * \param Buffer A pointer to the caller-allocated buffer that backs the string.
 * \param MaximumLength The size, in bytes, of the buffer.
 */
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

/**
 * The RtlInitUnicodeString routine initializes a counted Unicode string.
 *
 * \param DestinationString A pointer to the UNICODE_STRING structure to initialize.
 * \param SourceString An optional pointer to a null-terminated Unicode string used to initialize the counted string.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlinitunicodestring
 */
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

/**
 * The RtlInitUnicodeStringEx routine initializes a counted Unicode string and validates the length of the source string.
 *
 * \param DestinationString A pointer to the UNICODE_STRING structure to initialize.
 * \param SourceString An optional pointer to a null-terminated Unicode string used to initialize the counted string.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlinitunicodestringex
 */
#ifndef PHNT_NO_INLINE_INIT_STRING
FORCEINLINE
NTSTATUS
NTAPI_INLINE
RtlInitUnicodeStringEx(
    _Out_ PUNICODE_STRING DestinationString,
    _In_opt_z_ PCWSTR SourceString
    )
{
    SIZE_T stringLength;

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

/**
 * The RtlCreateUnicodeStringFromAsciiz routine allocates and initializes a counted Unicode string from a null-terminated ANSI string.
 *
 * \param DestinationString A pointer to the UNICODE_STRING structure that receives the newly allocated string.
 * \param SourceString A pointer to the null-terminated ANSI source string.
 * \return `TRUE` if the Unicode string was successfully created, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlcreateunicodestringfromasciiz
 */
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

/**
 * Flags for RtlDuplicateUnicodeString.
 */
#define RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE (0x00000001)
#define RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING (0x00000002)

/**
 * The RtlDuplicateUnicodeString routine creates a copy of a counted Unicode string.
 *
 * \param Flags Flags that control how the string is duplicated.
 * \param StringIn A pointer to the source Unicode string.
 * \param StringOut A pointer to the UNICODE_STRING structure that receives the duplicated string.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlduplicateunicodestring
 */
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

/**
 * String hashing algorithm identifiers for RtlHashUnicodeString.
 */
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

/**
 * The RtlValidateUnicodeString routine validates that a counted Unicode string is well formed.
 *
 * \param Flags Reserved. Must be zero.
 * \param String A pointer to the Unicode string to validate.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlSuffixUnicodeString routine determines whether one counted Unicode string is a suffix of another.
 *
 * \param String1 A pointer to the Unicode string that is the potential suffix.
 * \param String2 A pointer to the Unicode string to search.
 * \param CaseInSensitive If `TRUE`, case is ignored when comparing the strings.
 * \return `TRUE` if String1 is a suffix of String2, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlsuffixunicodestring
 */
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

/**
 * The RtlSanitizeUnicodeStringPadding routine zeroes the structure padding of a UNICODE_STRING to avoid leaking uninitialized memory.
 *
 * \param String A pointer to the UNICODE_STRING whose padding is cleared.
 */
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
/**
 * The RtlFindUnicodeSubstring routine searches for the first occurrence of a substring within a counted Unicode string.
 *
 * \param FullString A pointer to the Unicode string to search.
 * \param SearchString A pointer to the Unicode substring to find.
 * \param CaseInSensitive If `TRUE`, case is ignored when comparing the strings.
 * \return A pointer to the first occurrence of the substring, or `NULL` if it was not found.
 */
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

/**
 * Flags for RtlFindCharInUnicodeString.
 */
#define RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END 0x00000001
#define RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET 0x00000002
#define RTL_FIND_CHAR_IN_UNICODE_STRING_CASE_INSENSITIVE 0x00000004

/**
 * The RtlFindCharInUnicodeString routine searches for the first occurrence of any character from a character set within a counted Unicode string.
 *
 * \param Flags Flags that control how the search is performed.
 * \param StringToSearch A pointer to the Unicode string to search.
 * \param CharSet A pointer to a Unicode string that contains the set of characters to find.
 * \param NonInclusivePrefixLength A pointer to a variable that receives the length, in bytes, of the prefix that precedes the found character.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlfindcharinunicodestring
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlFindCharInUnicodeString(
    _In_ ULONG Flags,
    _In_ PCUNICODE_STRING StringToSearch,
    _In_ PCUNICODE_STRING CharSet,
    _Out_ PUSHORT NonInclusivePrefixLength
    );

/**
 * Forward declaration of the RTL_UNICODE_STRING_BUFFER structure.
 */
typedef struct _RTL_UNICODE_STRING_BUFFER RTL_UNICODE_STRING_BUFFER, *PRTL_UNICODE_STRING_BUFFER;

/**
 * The RtlMultiAppendUnicodeStringBuffer routine appends multiple counted Unicode strings to a Unicode string buffer.
 *
 * \param Buffer A pointer to the Unicode string buffer.
 * \param BufferCount The number of source strings in the array.
 * \param Source A pointer to an array of Unicode strings to append.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlMultiAppendUnicodeStringBuffer(
    _Inout_ PRTL_UNICODE_STRING_BUFFER Buffer,
    _In_ ULONG BufferCount,
    _In_ PCUNICODE_STRING Source
    );

/**
 * The RtlAppendPathElement routine appends a path element to a Unicode string buffer, inserting a path separator as needed.
 *
 * \param Flags Flags that control how the path element is appended.
 * \param Buffer A pointer to the Unicode string buffer.
 * \param Source A pointer to the path element to append.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAppendPathElement(
    _In_ ULONG Flags,
    _Inout_ PRTL_UNICODE_STRING_BUFFER Buffer,
    _In_ PCUNICODE_STRING Source
    );

/**
 * The RtlAppendUnicodeStringToString routine concatenates two counted Unicode strings.
 *
 * \param Destination A pointer to the destination Unicode string.
 * \param Source A pointer to the source Unicode string to append.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlappendunicodestringtostring
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAppendUnicodeStringToString(
    _Inout_ PUNICODE_STRING Destination,
    _In_ PCUNICODE_STRING Source
    );

/**
 * The RtlAppendUnicodeToString routine concatenates a null-terminated Unicode string to a counted Unicode string.
 *
 * \param Destination A pointer to the destination Unicode string.
 * \param Source An optional pointer to the null-terminated Unicode source string to append.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlappendunicodetostring
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAppendUnicodeToString(
    _Inout_ PUNICODE_STRING Destination,
    _In_opt_z_ PCWSTR Source
    );

/**
 * The RtlUpcaseUnicodeString routine converts a counted Unicode string to uppercase.
 *
 * \param DestinationString A pointer to the destination Unicode string. If AllocateDestinationString is `TRUE`, the buffer is allocated by the routine.
 * \param SourceString A pointer to the source Unicode string.
 * \param AllocateDestinationString If `TRUE`, the routine allocates the destination string buffer.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlupcaseunicodestring
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeString(
    _When_(AllocateDestinationString, _Out_) _When_(!AllocateDestinationString, _In_) PUNICODE_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

/**
 * The RtlDowncaseUnicodeString routine converts a counted Unicode string to lowercase.
 *
 * \param DestinationString A pointer to the destination Unicode string. If AllocateDestinationString is `TRUE`, the buffer is allocated by the routine.
 * \param SourceString A pointer to the source Unicode string.
 * \param AllocateDestinationString If `TRUE`, the routine allocates the destination string buffer.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtldowncaseunicodestring
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDowncaseUnicodeString(
    _When_(AllocateDestinationString, _Out_) _When_(!AllocateDestinationString, _In_) PUNICODE_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

/**
 * The RtlEraseUnicodeString routine securely zeroes the buffer of a counted Unicode string.
 *
 * \param String A pointer to the Unicode string to erase.
 */
NTSYSAPI
VOID
NTAPI
RtlEraseUnicodeString(
    _Inout_ PUNICODE_STRING String
    );

/**
 * The RtlAnsiStringToUnicodeString routine converts a counted ANSI string to a counted Unicode string.
 *
 * \param DestinationString A pointer to the destination Unicode string. If AllocateDestinationString is `TRUE`, the buffer is allocated by the routine.
 * \param SourceString A pointer to the source ANSI string.
 * \param AllocateDestinationString If `TRUE`, the routine allocates the destination string buffer.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlansistringtounicodestring
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAnsiStringToUnicodeString(
    _When_(AllocateDestinationString, _Out_) _When_(!AllocateDestinationString, _In_) PUNICODE_STRING DestinationString,
    _In_ PCANSI_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

/**
 * The RtlxAnsiStringToUnicodeSize routine computes the number of bytes required to hold the Unicode translation of a counted ANSI string.
 *
 * \param AnsiString A pointer to the ANSI string.
 * \return The size, in bytes, required for the Unicode translation, including the terminating null character.
 */
NTSYSAPI
ULONG
NTAPI
RtlxAnsiStringToUnicodeSize(
    _In_ PCANSI_STRING AnsiString
    );

/**
 * The RtlxUnicodeStringToAnsiSize routine routine returns the number of bytes required for a null-terminated ANSI string that is equivalent to a specified Unicode string.
 *
 * \param UnicodeString Pointer to the Unicode string for which to compute the number of bytes required for an equivalent null-terminated ANSI string.
 * \return If the Unicode string can be translated into an ANSI string using the current system locale information,
 * RtlxUnicodeStringToAnsiSize returns the number of bytes required for an equivalent null-terminated ANSI string. Otherwise, it returns zero.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlxunicodestringtoansisize
 */
NTSYSAPI
ULONG
NTAPI
RtlxUnicodeStringToAnsiSize(
    _In_ PCUNICODE_STRING UnicodeString
    );

/**
 * The RtlxUnicodeStringToOemSize routine is reserved for system use - use RtlUnicodeStringToOemSize instead.
 *
 * \param UnicodeString Reserved.
 * \return Reserved.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlxunicodestringtooemsize
 */
NTSYSAPI
ULONG
NTAPI
RtlxUnicodeStringToOemSize(
    _In_ PCUNICODE_STRING UnicodeString
    );

/**
 * The RtlxOemStringToUnicodeSize routine is reserved for system use - use RtlOemStringToUnicodeSize instead.
 *
 * \param UnicodeString Reserved.
 * \return Reserved.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlxoemstringtounicodesize
 */
NTSYSAPI
ULONG
NTAPI
RtlxOemStringToUnicodeSize(
    _In_ PCUNICODE_STRING UnicodeString
    );

// NTSYSAPI
// ULONG
// NTAPI
// RtlAnsiStringToUnicodeSize(
//     _In_ PCANSI_STRING AnsiString
//     );

/**
 * Computes the size, in bytes, required to hold the Unicode form of an ANSI string.
 */
#define RtlAnsiStringToUnicodeSize(STRING) \
    RtlxAnsiStringToUnicodeSize(STRING)

/**
 * The RtlUnicodeStringToAnsiString routine converts a counted Unicode string to a counted ANSI string.
 *
 * \param DestinationString A pointer to the destination ANSI string. If AllocateDestinationString is `TRUE`, the buffer is allocated by the routine.
 * \param SourceString A pointer to the source Unicode string.
 * \param AllocateDestinationString If `TRUE`, the routine allocates the destination string buffer.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlunicodestringtoansistring
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToAnsiString(
    _When_(AllocateDestinationString, _Out_) _When_(!AllocateDestinationString, _In_) PANSI_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

// rev
/**
 * The RtlUnicodeStringToAnsiSize routine computes the number of bytes required to hold the ANSI translation of a counted Unicode string.
 *
 * \param SourceString A pointer to the Unicode string.
 * \return The size, in bytes, required for the ANSI translation, including the terminating null character.
 */
NTSYSAPI
ULONG
NTAPI
RtlUnicodeStringToAnsiSize(
    _In_ PCUNICODE_STRING SourceString
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)
/**
 * The RtlUnicodeStringToUTF8String routine converts a counted Unicode string to a counted UTF-8 string.
 *
 * \param DestinationString A pointer to the destination UTF-8 string. If AllocateDestinationString is `TRUE`, the buffer is allocated by the routine.
 * \param SourceString A pointer to the source Unicode string.
 * \param AllocateDestinationString If `TRUE`, the routine allocates the destination string buffer.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToUTF8String(
    _When_(AllocateDestinationString, _Out_) _When_(!AllocateDestinationString, _In_) PUTF8_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

/**
 * The RtlUTF8StringToUnicodeString routine converts a counted UTF-8 string to a counted Unicode string.
 *
 * \param DestinationString A pointer to the destination Unicode string. If AllocateDestinationString is `TRUE`, the buffer is allocated by the routine.
 * \param SourceString A pointer to the source UTF-8 string.
 * \param AllocateDestinationString If `TRUE`, the routine allocates the destination string buffer.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlUTF8StringToUnicodeString(
    _When_(AllocateDestinationString, _Out_) _When_(!AllocateDestinationString, _In_) PUNICODE_STRING DestinationString,
    _In_ PCUTF8_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_20H1

/**
 * The RtlAnsiCharToUnicodeChar routine converts the next ANSI (multibyte) character to a Unicode character and advances the source pointer.
 *
 * \param SourceCharacter A pointer to a variable that points to the ANSI character to convert. On return, the variable is advanced past the converted character.
 * \return The Unicode equivalent of the source character.
 */
NTSYSAPI
WCHAR
NTAPI
RtlAnsiCharToUnicodeChar(
    _Inout_ PUCHAR *SourceCharacter
    );

/**
 * The RtlUpcaseUnicodeStringToAnsiString routine converts a counted Unicode string to an uppercase counted ANSI string.
 *
 * \param DestinationString A pointer to the destination ANSI string. If AllocateDestinationString is `TRUE`, the buffer is allocated by the routine.
 * \param SourceString A pointer to the source Unicode string.
 * \param AllocateDestinationString If `TRUE`, the routine allocates the destination string buffer.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlupcaseunicodestringtoansistring
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToAnsiString(
    _When_(AllocateDestinationString, _Out_) _When_(!AllocateDestinationString, _In_) PANSI_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

/**
 * The RtlOemStringToUnicodeString routine converts a counted OEM string to a counted Unicode string.
 *
 * \param DestinationString A pointer to the destination Unicode string. If AllocateDestinationString is `TRUE`, the buffer is allocated by the routine.
 * \param SourceString A pointer to the source OEM string.
 * \param AllocateDestinationString If `TRUE`, the routine allocates the destination string buffer.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtloemstringtounicodestring
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlOemStringToUnicodeString(
    _When_(AllocateDestinationString, _Out_) _When_(!AllocateDestinationString, _In_) PUNICODE_STRING DestinationString,
    _In_ POEM_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

/**
 * The RtlUnicodeStringToOemString routine converts a counted Unicode string to a counted OEM string.
 *
 * \param DestinationString A pointer to the destination OEM string. If AllocateDestinationString is `TRUE`, the buffer is allocated by the routine.
 * \param SourceString A pointer to the source Unicode string.
 * \param AllocateDestinationString If `TRUE`, the routine allocates the destination string buffer.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlunicodestringtooemstring
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToOemString(
    _When_(AllocateDestinationString, _Out_) _When_(!AllocateDestinationString, _In_) POEM_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

/**
 * The RtlUpcaseUnicodeStringToOemString routine converts a counted Unicode string to an uppercase counted OEM string.
 *
 * \param DestinationString A pointer to the destination OEM string. If AllocateDestinationString is `TRUE`, the buffer is allocated by the routine.
 * \param SourceString A pointer to the source Unicode string.
 * \param AllocateDestinationString If `TRUE`, the routine allocates the destination string buffer.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlupcaseunicodestringtooemstring
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToOemString(
    _When_(AllocateDestinationString, _Out_) _When_(!AllocateDestinationString, _In_) POEM_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

/**
 * The RtlOemStringToCountedUnicodeString routine converts a counted OEM string to a counted Unicode string that is not null-terminated.
 *
 * \param DestinationString A pointer to the destination Unicode string. If AllocateDestinationString is `TRUE`, the buffer is allocated by the routine.
 * \param SourceString A pointer to the source OEM string.
 * \param AllocateDestinationString If `TRUE`, the routine allocates the destination string buffer.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtloemstringtocountedunicodestring
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlOemStringToCountedUnicodeString(
    _When_(AllocateDestinationString, _Out_) _When_(!AllocateDestinationString, _In_) PUNICODE_STRING DestinationString,
    _In_ PCOEM_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

/**
 * The RtlUnicodeStringToCountedOemString routine converts a counted Unicode string to a counted OEM string that is not null-terminated.
 *
 * \param DestinationString A pointer to the destination OEM string. If AllocateDestinationString is `TRUE`, the buffer is allocated by the routine.
 * \param SourceString A pointer to the source Unicode string.
 * \param AllocateDestinationString If `TRUE`, the routine allocates the destination string buffer.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlunicodestringtocountedoemstring
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToCountedOemString(
    _When_(AllocateDestinationString, _Out_) _When_(!AllocateDestinationString, _In_) POEM_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

/**
 * The RtlUpcaseUnicodeStringToCountedOemString routine converts a counted Unicode string to an uppercase counted OEM string that is not null-terminated.
 *
 * \param DestinationString A pointer to the destination OEM string. If AllocateDestinationString is `TRUE`, the buffer is allocated by the routine.
 * \param SourceString A pointer to the source Unicode string.
 * \param AllocateDestinationString If `TRUE`, the routine allocates the destination string buffer.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlupcaseunicodestringtocountedoemstring
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlUpcaseUnicodeStringToCountedOemString(
    _When_(AllocateDestinationString, _Out_) _When_(!AllocateDestinationString, _In_) POEM_STRING DestinationString,
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

/**
 * The RtlOemToUnicodeN routine converts an OEM string to a Unicode string.
 *
 * \param UnicodeString A pointer to a buffer that receives the translated Unicode string.
 * \param MaxBytesInUnicodeString The maximum number of bytes to write to UnicodeString.
 * \param BytesInUnicodeString An optional pointer to a variable that receives the number of bytes written to UnicodeString.
 * \param OemString A pointer to the OEM source string.
 * \param BytesInOemString The number of bytes in the OEM source string.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtloemtounicoden
 */
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

/**
 * The RtlUnicodeToOemN routine converts a Unicode string to an OEM string.
 *
 * \param OemString A pointer to a buffer that receives the translated OEM string.
 * \param MaxBytesInOemString The maximum number of bytes to write to OemString.
 * \param BytesInOemString An optional pointer to a variable that receives the number of bytes written to OemString.
 * \param UnicodeString A pointer to the Unicode source string.
 * \param BytesInUnicodeString The number of bytes in the Unicode source string.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlunicodetooemn
 */
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

/**
 * The RtlUpcaseUnicodeToOemN routine converts a Unicode string to an uppercase OEM string.
 *
 * \param OemString A pointer to a buffer that receives the translated OEM string.
 * \param MaxBytesInOemString The maximum number of bytes to write to OemString.
 * \param BytesInOemString An optional pointer to a variable that receives the number of bytes written to OemString.
 * \param UnicodeString A pointer to the Unicode source string.
 * \param BytesInUnicodeString The number of bytes in the Unicode source string.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlupcaseunicodetooemn
 */
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

/**
 * The RtlConsoleMultiByteToUnicodeN routine converts a console multibyte string to a Unicode string, reporting whether any special characters were encountered.
 *
 * \param UnicodeString A pointer to a buffer that receives the translated Unicode string.
 * \param MaxBytesInUnicodeString The maximum number of bytes to write to UnicodeString.
 * \param BytesInUnicodeString An optional pointer to a variable that receives the number of bytes written to UnicodeString.
 * \param MultiByteString A pointer to the multibyte source string.
 * \param BytesInMultiByteString The number of bytes in the multibyte source string.
 * \param pdwSpecialChar A pointer to a variable that receives a value indicating whether a special character was encountered.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlCustomCPToUnicodeN routine converts a string in a custom code page to a Unicode string.
 *
 * \param CustomCP A pointer to the code page table that describes the custom code page.
 * \param UnicodeString A pointer to a buffer that receives the translated Unicode string.
 * \param MaxBytesInUnicodeString The maximum number of bytes to write to UnicodeString.
 * \param BytesInUnicodeString An optional pointer to a variable that receives the number of bytes written to UnicodeString.
 * \param CustomCPString A pointer to the custom code page source string.
 * \param BytesInCustomCPString The number of bytes in the custom code page source string.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlUnicodeToCustomCPN routine converts a Unicode string to a string in a custom code page.
 *
 * \param CustomCP A pointer to the code page table that describes the custom code page.
 * \param CustomCPString A pointer to a buffer that receives the translated custom code page string.
 * \param MaxBytesInCustomCPString The maximum number of bytes to write to CustomCPString.
 * \param BytesInCustomCPString An optional pointer to a variable that receives the number of bytes written to CustomCPString.
 * \param UnicodeString A pointer to the Unicode source string.
 * \param BytesInUnicodeString The number of bytes in the Unicode source string.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlUpcaseUnicodeToCustomCPN routine converts a Unicode string to an uppercase string in a custom code page.
 *
 * \param CustomCP A pointer to the code page table that describes the custom code page.
 * \param CustomCPString A pointer to a buffer that receives the translated custom code page string.
 * \param MaxBytesInCustomCPString The maximum number of bytes to write to CustomCPString.
 * \param BytesInCustomCPString An optional pointer to a variable that receives the number of bytes written to CustomCPString.
 * \param UnicodeString A pointer to the Unicode source string.
 * \param BytesInUnicodeString The number of bytes in the Unicode source string.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlInitCodePageTable routine initializes a code page table structure from a raw code page table image.
 *
 * \param TableBase A pointer to the raw code page table data.
 * \param CodePageTable A pointer to the CPTABLEINFO structure that receives the initialized code page table.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlinitcodepagetable
 */
NTSYSAPI
VOID
NTAPI
RtlInitCodePageTable(
    _In_reads_opt_(2) PUSHORT TableBase,
    _Inout_ PCPTABLEINFO CodePageTable
    );

#if (PHNT_VERSION < PHNT_WINDOWS_11)
/**
 * The RtlInitNlsTables routine initializes an NLS (National Language Support) table information structure from the specified ANSI, OEM, and language code page tables.
 *
 * \param AnsiNlsBase A pointer to the ANSI code page NLS data.
 * \param OemNlsBase A pointer to the OEM code page NLS data.
 * \param LanguageNlsBase A pointer to the language (case-mapping) NLS data.
 * \param TableInfo A pointer to the NLSTABLEINFO structure that receives the initialized table information.
 */
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

/**
 * The RtlResetRtlTranslations routine resets the code page translation tables used by the run-time library to those described by the specified NLS table information.
 *
 * \param TableInfo A pointer to the NLSTABLEINFO structure that describes the translation tables.
 */
NTSYSAPI
VOID
NTAPI
RtlResetRtlTranslations(
    _In_ PNLSTABLEINFO TableInfo
    );

/**
 * The RtlIsTextUnicode routine applies heuristics to determine whether a buffer likely contains Unicode (UTF-16) text.
 *
 * \param Buffer The buffer to examine.
 * \param Size The size, in bytes, of the buffer.
 * \param Result On input specifies which tests to apply (or NULL for all); on output receives the tests that passed.
 * \return TRUE if the buffer is likely Unicode text; otherwise, FALSE.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsTextUnicode(
    _In_ PVOID Buffer,
    _In_ ULONG Size,
    _Inout_opt_ PULONG Result
    );

/**
 * Identifies the Unicode normalization form applied by string normalization routines.
 */
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

/**
 * The RtlNormalizeString routine normalizes a Unicode string according to the specified Unicode normalization form.
 *
 * \param NormForm The normalization form to apply (RTL_NORM_FORM).
 * \param SourceString A pointer to the source Unicode string.
 * \param SourceStringLength The length, in characters, of the source string, or -1 if the string is null-terminated.
 * \param DestinationString A buffer that receives the normalized string.
 * \param DestinationStringLength On input, the size of the destination buffer in characters; on output, the number of characters written or required.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlnormalizestring
 */
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

/**
 * The RtlIsNormalizedString routine determines whether a Unicode string is already in the specified normalization form.
 *
 * \param NormForm The normalization form to test against (RTL_NORM_FORM).
 * \param SourceString A pointer to the source Unicode string.
 * \param SourceStringLength The length, in characters, of the source string, or -1 if the string is null-terminated.
 * \param Normalized A pointer to a variable that receives `TRUE` if the string is normalized, otherwise `FALSE`.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlisnormalizedstring
 */
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
/**
 * The RtlIsNameInExpression routine determines whether a Unicode string matches the specified pattern.
 *
 * \param Expression A pointer to the pattern string. This string can contain wildcard characters. If the IgnoreCase parameter is TRUE, the string must contain only uppercase characters.
 * \param Name Maximum number of bytes to be written to UTF8StringDestination. If this value causes the translated string to be truncated, RtlUnicodeToUTF8N returns an error status.
 * \param IgnoreCase TRUE for case-insensitive matching, or FALSE for case-sensitive matching.
 * \param UpcaseTable An optional pointer to an uppercase character table to use for case-insensitive matching. If this parameter is NULL, the default system uppercase character table is used.
 * \return TRUE if the string matches the pattern. If the string does not match the pattern, this function returns FALSE.
 * \sa https://learn.microsoft.com/en-us/windows/win32/devnotes/rtlisnameinexpression
 */
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
/**
 * The RtlIsNameInUnUpcasedExpression routine determines whether a name matches a wildcard expression, using a caller-supplied upcase table rather than upcasing the expression.
 *
 * \param Expression A pointer to the wildcard expression, which must already be uppercase when IgnoreCase is `TRUE`.
 * \param Name A pointer to the name to test against the expression.
 * \param IgnoreCase If `TRUE`, the comparison is case-insensitive.
 * \param UpcaseTable An optional pointer to an upcase translation table used to upcase the name.
 * \return `TRUE` if the name matches the expression, otherwise `FALSE`.
 */
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
/**
 * The RtlDoesNameContainWildCards routine determines whether a Unicode string contains wildcard characters.
 *
 * \param Name A pointer to the string to be checked.
 * \return TRUE if one or more wildcard characters were found, FALSE otherwise.
 * \remarks The following are wildcard characters: *, ?, ANSI_DOS_STAR, ANSI_DOS_DOT, and ANSI_DOS_QM.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-_fsrtl_advanced_fcb_header-fsrtldoesnamecontainwildcards
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlDoesNameContainWildCards(
    _In_ PCUNICODE_STRING Expression
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_10_19H1

/**
 * The RtlEqualDomainName routine compares two domain names for equality.
 *
 * \param String1 A pointer to the first domain name.
 * \param String2 A pointer to the second domain name.
 * \return `TRUE` if the domain names are equal, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlequaldomainname
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualDomainName(
    _In_ PCUNICODE_STRING String1,
    _In_ PCUNICODE_STRING String2
    );

/**
 * The RtlEqualComputerName routine compares two computer names for equality.
 *
 * \param String1 A pointer to the first computer name.
 * \param String2 A pointer to the second computer name.
 * \return `TRUE` if the computer names are equal, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlequalcomputername
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualComputerName(
    _In_ PCUNICODE_STRING String1,
    _In_ PCUNICODE_STRING String2
    );

/**
 * The RtlDnsHostNameToComputerName routine converts a DNS host name to a NetBIOS computer name.
 *
 * \param ComputerNameString A pointer to the string that receives the computer name.
 * \param DnsHostNameString A pointer to the DNS host name to convert.
 * \param AllocateComputerNameString If `TRUE`, the routine allocates the computer name string buffer.
 * \return NTSTATUS Successful or errant status.
 */
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
    _In_ PCGUID Guid,
    _Out_ PUNICODE_STRING GuidString
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8_1)

/**
 * Constants describing the length of a GUID string.
 */
#define RTL_GUID_STRING_SIZE 38
#define MAX_UNICODE_GUID_STRING_LENGTH (36 + sizeof(UNICODE_NULL))

// rev
/**
 * The RtlStringFromGUIDEx routine converts a GUID into its Unicode string representation, optionally allocating the destination string.
 *
 * \param Guid A pointer to the GUID to convert.
 * \param GuidString A pointer to the Unicode string that receives the string representation. If AllocateGuidString is `TRUE`, the buffer is allocated by the routine.
 * \param AllocateGuidString If `TRUE`, the routine allocates the destination string buffer.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlStringFromGUIDEx(
    _In_ PCGUID Guid,
    _Inout_ PUNICODE_STRING GuidString,
    _In_ BOOLEAN AllocateGuidString
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_8_1

/**
 * The RtlGUIDFromString routine converts the Unicode string representation of a GUID into a GUID structure.
 *
 * \param GuidString A pointer to the Unicode string that contains the GUID.
 * \param Guid A pointer to a variable that receives the converted GUID.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlguidfromstring
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGUIDFromString(
    _In_ PCUNICODE_STRING GuidString,
    _Out_ PGUID Guid
    );

/**
 * The RtlCompareAltitudes routine compares two filter driver altitude strings.
 *
 * \param Altitude1 A pointer to the first altitude string.
 * \param Altitude2 A pointer to the second altitude string.
 * \return A signed value that is negative, zero, or positive if Altitude1 is less than, equal to, or greater than Altitude2.
 */
NTSYSAPI
LONG
NTAPI
RtlCompareAltitudes(
    _In_ PCUNICODE_STRING Altitude1,
    _In_ PCUNICODE_STRING Altitude2
    );

/**
 * The RtlIdnToAscii routine converts an internationalized domain name (IDN) to its ASCII (Punycode) representation.
 *
 * \param Flags Flags that control the conversion.
 * \param SourceString A pointer to the source Unicode string.
 * \param SourceStringLength The length, in characters, of the source string, or -1 if the string is null-terminated.
 * \param DestinationString A buffer that receives the converted string.
 * \param DestinationStringLength On input, the size of the destination buffer in characters; on output, the number of characters written or required.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlidntoascii
 */
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

/**
 * The RtlIdnToUnicode routine converts an ASCII (Punycode) internationalized domain name to its Unicode representation.
 *
 * \param Flags Flags that control the conversion.
 * \param SourceString A pointer to the source string.
 * \param SourceStringLength The length, in characters, of the source string, or -1 if the string is null-terminated.
 * \param DestinationString A buffer that receives the converted string.
 * \param DestinationStringLength On input, the size of the destination buffer in characters; on output, the number of characters written or required.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlidntounicode
 */
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

/**
 * The RtlIdnToNameprepUnicode routine applies the nameprep algorithm to an internationalized domain name and returns the result in Unicode.
 *
 * \param Flags Flags that control the conversion.
 * \param SourceString A pointer to the source Unicode string.
 * \param SourceStringLength The length, in characters, of the source string, or -1 if the string is null-terminated.
 * \param DestinationString A buffer that receives the converted string.
 * \param DestinationStringLength On input, the size of the destination buffer in characters; on output, the number of characters written or required.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlidntonameprepunicode
 */
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

/**
 * Represents an entry in an ANSI prefix table.
 */
typedef struct _PREFIX_TABLE_ENTRY
{
    CSHORT NodeTypeCode;
    CSHORT NameLength;
    struct _PREFIX_TABLE_ENTRY *NextPrefixTree;
    RTL_SPLAY_LINKS Links;
    PSTRING Prefix;
} PREFIX_TABLE_ENTRY, *PPREFIX_TABLE_ENTRY;

/**
 * Represents an ANSI prefix table used for longest-prefix name lookups.
 */
typedef struct _PREFIX_TABLE
{
    CSHORT NodeTypeCode;
    CSHORT NameLength;
    PPREFIX_TABLE_ENTRY NextPrefixTree;
} PREFIX_TABLE, *PPREFIX_TABLE;

/**
 * The PfxInitialize routine initializes a prefix table used to hold ANSI string prefixes.
 *
 * \param PrefixTable A pointer to the prefix table to initialize.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-pfxinitialize
 */
NTSYSAPI
VOID
NTAPI
PfxInitialize(
    _Out_ PPREFIX_TABLE PrefixTable
    );

/**
 * The PfxInsertPrefix routine inserts a prefix into a prefix table.
 *
 * \param PrefixTable A pointer to the prefix table.
 * \param Prefix A pointer to the counted string that is the prefix to insert.
 * \param PrefixTableEntry A pointer to the prefix table entry that describes the inserted prefix.
 * \return `TRUE` if the prefix was inserted, otherwise `FALSE` if a matching prefix already exists.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-pfxinsertprefix
 */
NTSYSAPI
BOOLEAN
NTAPI
PfxInsertPrefix(
    _In_ PPREFIX_TABLE PrefixTable,
    _In_ __drv_aliasesMem PSTRING Prefix,
    _Out_ PPREFIX_TABLE_ENTRY PrefixTableEntry
    );

/**
 * The PfxRemovePrefix routine removes a prefix from a prefix table.
 *
 * \param PrefixTable A pointer to the prefix table.
 * \param PrefixTableEntry A pointer to the prefix table entry to remove.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-pfxremoveprefix
 */
NTSYSAPI
VOID
NTAPI
PfxRemovePrefix(
    _In_ PPREFIX_TABLE PrefixTable,
    _In_ PPREFIX_TABLE_ENTRY PrefixTableEntry
    );

/**
 * The PfxFindPrefix routine searches a prefix table for the longest prefix of the specified name.
 *
 * \param PrefixTable A pointer to the prefix table.
 * \param FullName A pointer to the counted string to search for a prefix.
 * \return A pointer to the matching prefix table entry, or `NULL` if no prefix was found.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-pfxfindprefix
 */
NTSYSAPI
PPREFIX_TABLE_ENTRY
NTAPI
PfxFindPrefix(
    _In_ PPREFIX_TABLE PrefixTable,
    _In_ PSTRING FullName
    );

/**
 * Represents an entry in a Unicode prefix table.
 */
typedef struct _UNICODE_PREFIX_TABLE_ENTRY
{
    CSHORT NodeTypeCode;
    CSHORT NameLength;
    struct _UNICODE_PREFIX_TABLE_ENTRY *NextPrefixTree;
    struct _UNICODE_PREFIX_TABLE_ENTRY *CaseMatch;
    RTL_SPLAY_LINKS Links;
    PUNICODE_STRING Prefix;
} UNICODE_PREFIX_TABLE_ENTRY, *PUNICODE_PREFIX_TABLE_ENTRY;

/**
 * Represents a Unicode prefix table used for longest-prefix name lookups.
 */
typedef struct _UNICODE_PREFIX_TABLE
{
    CSHORT NodeTypeCode;
    CSHORT NameLength;
    PUNICODE_PREFIX_TABLE_ENTRY NextPrefixTree;
    PUNICODE_PREFIX_TABLE_ENTRY LastNextEntry;
} UNICODE_PREFIX_TABLE, *PUNICODE_PREFIX_TABLE;

/**
 * The RtlInitializeUnicodePrefix routine initializes a Unicode prefix table.
 *
 * \param PrefixTable A pointer to the Unicode prefix table to initialize.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlinitializeunicodeprefix
 */
NTSYSAPI
VOID
NTAPI
RtlInitializeUnicodePrefix(
    _Out_ PUNICODE_PREFIX_TABLE PrefixTable
    );

/**
 * The RtlInsertUnicodePrefix routine inserts a prefix into a Unicode prefix table.
 *
 * \param PrefixTable A pointer to the Unicode prefix table.
 * \param Prefix A pointer to the Unicode string that is the prefix to insert.
 * \param PrefixTableEntry A pointer to the prefix table entry that describes the inserted prefix.
 * \return `TRUE` if the prefix was inserted, otherwise `FALSE` if a matching prefix already exists.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlinsertunicodeprefix
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlInsertUnicodePrefix(
    _In_ PUNICODE_PREFIX_TABLE PrefixTable,
    _In_ __drv_aliasesMem PCUNICODE_STRING Prefix,
    _Out_ PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry
    );

/**
 * The RtlRemoveUnicodePrefix routine removes a prefix from a Unicode prefix table.
 *
 * \param PrefixTable A pointer to the Unicode prefix table.
 * \param PrefixTableEntry A pointer to the prefix table entry to remove.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlremoveunicodeprefix
 */
NTSYSAPI
VOID
NTAPI
RtlRemoveUnicodePrefix(
    _In_ PUNICODE_PREFIX_TABLE PrefixTable,
    _In_ PUNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry
    );

/**
 * The RtlFindUnicodePrefix routine searches a Unicode prefix table for the longest prefix of the specified name.
 *
 * \param PrefixTable A pointer to the Unicode prefix table.
 * \param FullName A pointer to the Unicode string to search for a prefix.
 * \param CaseInsensitiveIndex The number of leading characters to compare case-insensitively.
 * \return A pointer to the matching prefix table entry, or `NULL` if no prefix was found.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlfindunicodeprefix
 */
NTSYSAPI
PUNICODE_PREFIX_TABLE_ENTRY
NTAPI
RtlFindUnicodePrefix(
    _In_ PUNICODE_PREFIX_TABLE PrefixTable,
    _In_ PCUNICODE_STRING FullName,
    _In_ ULONG CaseInsensitiveIndex
    );

/**
 * The RtlNextUnicodePrefix routine enumerates the entries in a Unicode prefix table.
 *
 * \param PrefixTable A pointer to the Unicode prefix table.
 * \param Restart If `TRUE`, the enumeration restarts from the first entry.
 * \return A pointer to the next prefix table entry, or `NULL` when the enumeration is complete.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlnextunicodeprefix
 */
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

/**
 * Compression format identifiers used by the RTL compression routines.
 */
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

/**
 * Compression engine identifiers used by the RTL compression routines.
 */
#define COMPRESSION_ENGINE_STANDARD      (0x0000)
#define COMPRESSION_ENGINE_MAXIMUM       (0x0100)
#define COMPRESSION_ENGINE_HIBER         (0x0200)
#define COMPRESSION_ENGINE_MAX           (0x0200)

/**
 * Masks for extracting the compression format and engine from a combined value.
 */
#define COMPRESSION_FORMAT_MASK          (0x00FF)
#define COMPRESSION_ENGINE_MASK          (0xFF00)
#define COMPRESSION_FORMAT_ENGINE_MASK   (COMPRESSION_FORMAT_MASK | COMPRESSION_ENGINE_MASK)

/**
 * Describes the format and parameters of a compressed data buffer.
 */
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

/**
 * The RtlGetCompressionWorkSpaceSize routine determines the size of the work space required by the compression and decompression routines.
 *
 * \param CompressionFormatAndEngine The compression format and engine (for example, COMPRESSION_FORMAT_LZNT1 combined with an engine value).
 * \param CompressBufferWorkSpaceSize A pointer to a variable that receives the work space size required to compress a buffer.
 * \param CompressFragmentWorkSpaceSize A pointer to a variable that receives the work space size required to compress a fragment.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlgetcompressionworkspacesize
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetCompressionWorkSpaceSize(
    _In_ USHORT CompressionFormatAndEngine,
    _Out_ PULONG CompressBufferWorkSpaceSize,
    _Out_ PULONG CompressFragmentWorkSpaceSize
    );

/**
 * The RtlCompressBuffer routine compresses a buffer using the specified compression format and engine.
 *
 * \param CompressionFormatAndEngine The compression format (COMPRESSION_FORMAT_*) combined with the engine (COMPRESSION_ENGINE_*).
 * \param UncompressedBuffer The data to compress.
 * \param UncompressedBufferSize The size, in bytes, of UncompressedBuffer.
 * \param CompressedBuffer A buffer that receives the compressed data.
 * \param CompressedBufferSize The size, in bytes, of CompressedBuffer.
 * \param UncompressedChunkSize The chunk size used during compression (typically 4096).
 * \param FinalCompressedSize Receives the size, in bytes, of the compressed data.
 * \param WorkSpace A workspace buffer sized per RtlGetCompressionWorkSpaceSize.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlDecompressBuffer routine decompresses an entire compressed buffer.
 *
 * \param CompressionFormat The compression format used to compress the buffer.
 * \param UncompressedBuffer A buffer that receives the decompressed data.
 * \param UncompressedBufferSize The size, in bytes, of the uncompressed buffer.
 * \param CompressedBuffer A pointer to the compressed data.
 * \param CompressedBufferSize The size, in bytes, of the compressed buffer.
 * \param FinalUncompressedSize A pointer to a variable that receives the number of bytes written to the uncompressed buffer.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtldecompressbuffer
 */
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
/**
 * The RtlDecompressBufferEx routine decompresses a buffer using the specified compression format, with an optional caller-supplied workspace.
 *
 * \param CompressionFormat The compression format (COMPRESSION_FORMAT_*) of the compressed data.
 * \param UncompressedBuffer A buffer that receives the decompressed data.
 * \param UncompressedBufferSize The size, in bytes, of UncompressedBuffer.
 * \param CompressedBuffer The compressed data to decompress.
 * \param CompressedBufferSize The size, in bytes, of CompressedBuffer.
 * \param FinalUncompressedSize Receives the size, in bytes, of the decompressed data.
 * \param WorkSpace An optional workspace buffer sized per RtlGetCompressionWorkSpaceSize.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlDecompressBufferEx2 routine decompresses a buffer using the specified compression format and chunk size, with an optional workspace.
 *
 * \param CompressionFormat The compression format (COMPRESSION_FORMAT_*) of the compressed data.
 * \param UncompressedBuffer A buffer that receives the decompressed data.
 * \param UncompressedBufferSize The size, in bytes, of UncompressedBuffer.
 * \param CompressedBuffer The compressed data to decompress.
 * \param CompressedBufferSize The size, in bytes, of CompressedBuffer.
 * \param UncompressedChunkSize The chunk size used during compression (typically 4096).
 * \param FinalUncompressedSize Receives the size, in bytes, of the decompressed data.
 * \param WorkSpace An optional workspace buffer sized per RtlGetCompressionWorkSpaceSize.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlDecompressFragment routine decompresses a fragment of a compressed buffer beginning at the specified offset.
 *
 * \param CompressionFormat The compression format (COMPRESSION_FORMAT_*) of the compressed data.
 * \param UncompressedFragment A buffer that receives the decompressed fragment.
 * \param UncompressedFragmentSize The size, in bytes, of UncompressedFragment.
 * \param CompressedBuffer The compressed data to decompress.
 * \param CompressedBufferSize The size, in bytes, of CompressedBuffer.
 * \param FragmentOffset The offset, in bytes, into the uncompressed data at which the fragment begins.
 * \param FinalUncompressedSize Receives the size, in bytes, of the decompressed fragment.
 * \param WorkSpace A workspace buffer sized per RtlGetCompressionWorkSpaceSize.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlDecompressFragmentEx routine decompresses a fragment of a compressed buffer using the specified chunk size.
 *
 * \param CompressionFormat The compression format (COMPRESSION_FORMAT_*) of the compressed data.
 * \param UncompressedFragment A buffer that receives the decompressed fragment.
 * \param UncompressedFragmentSize The size, in bytes, of UncompressedFragment.
 * \param CompressedBuffer The compressed data to decompress.
 * \param CompressedBufferSize The size, in bytes, of CompressedBuffer.
 * \param FragmentOffset The offset, in bytes, into the uncompressed data at which the fragment begins.
 * \param UncompressedChunkSize The chunk size used during compression (typically 4096).
 * \param FinalUncompressedSize Receives the size, in bytes, of the decompressed fragment.
 * \param WorkSpace A workspace buffer sized per RtlGetCompressionWorkSpaceSize.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlDescribeChunk routine describes the next chunk within a compressed buffer and advances the buffer pointer past it.
 *
 * \param CompressionFormat The compression format used to compress the buffer.
 * \param CompressedBuffer A pointer to a variable that points to the current position in the compressed buffer. On return, the variable is advanced past the described chunk.
 * \param EndOfCompressedBufferPlus1 A pointer to the first byte beyond the end of the compressed buffer.
 * \param ChunkBuffer A pointer to a variable that receives a pointer to the chunk data.
 * \param ChunkSize A pointer to a variable that receives the size, in bytes, of the chunk.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlReserveChunk routine reserves space for a chunk within a compressed buffer.
 *
 * \param CompressionFormat The compression format used to compress the buffer.
 * \param CompressedBuffer A pointer to a variable that points to the current position in the compressed buffer. On return, the variable is advanced past the reserved chunk.
 * \param EndOfCompressedBufferPlus1 A pointer to the first byte beyond the end of the compressed buffer.
 * \param ChunkBuffer A pointer to a variable that receives a pointer to the reserved chunk data.
 * \param ChunkSize The size, in bytes, of the chunk to reserve.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlDecompressChunks routine decompresses a set of compressed chunks described by a compressed data information structure.
 *
 * \param UncompressedBuffer A buffer that receives the decompressed data.
 * \param UncompressedBufferSize The size, in bytes, of the uncompressed buffer.
 * \param CompressedBuffer A pointer to the compressed data.
 * \param CompressedBufferSize The size, in bytes, of the compressed buffer.
 * \param CompressedTail A pointer to the compressed tail data.
 * \param CompressedTailSize The size, in bytes, of the compressed tail.
 * \param CompressedDataInfo A pointer to the structure that describes the compressed chunks.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlCompressChunks routine compresses a buffer as a series of chunks, recording per-chunk metadata.
 *
 * \param UncompressedBuffer The data to compress.
 * \param UncompressedBufferSize The size, in bytes, of UncompressedBuffer.
 * \param CompressedBuffer A buffer that receives the compressed data.
 * \param CompressedBufferSize The size, in bytes, of CompressedBuffer.
 * \param CompressedDataInfo Receives per-chunk compressed data information.
 * \param CompressedDataInfoLength The size, in bytes, of CompressedDataInfo.
 * \param WorkSpace A workspace buffer sized per RtlGetCompressionWorkSpaceSize.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlConvertLCIDToString routine converts a locale identifier (LCID) to its string representation.
 *
 * \param LcidValue The locale identifier to convert.
 * \param Base The numeric base used for the conversion.
 * \param Padding The minimum width, in characters, to which the resulting string is padded.
 * \param pResultBuf A buffer that receives the resulting string.
 * \param Size The size, in characters, of the result buffer.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlIsValidLocaleName routine determines whether the specified locale name is valid.
 *
 * \param LocaleName A pointer to the locale name to validate.
 * \param Flags Flags that control the validation.
 * \return `TRUE` if the locale name is valid, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsValidLocaleName(
    _In_ PCWSTR LocaleName,
    _In_ ULONG Flags
    );

// private
/**
 * The RtlGetParentLocaleName routine returns the parent locale name of the specified locale.
 *
 * \param LocaleName A pointer to the locale name to query.
 * \param ParentLocaleName A pointer to the string that receives the parent locale name. If AllocateDestinationString is `TRUE`, the buffer is allocated by the routine.
 * \param Flags Flags that control the operation.
 * \param AllocateDestinationString If `TRUE`, the routine allocates the destination string buffer.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetParentLocaleName(
    _In_ PCWSTR LocaleName,
    _When_(AllocateDestinationString, _Out_) _When_(!AllocateDestinationString, _In_) PUNICODE_STRING ParentLocaleName,
    _In_ ULONG Flags,
    _In_ BOOLEAN AllocateDestinationString
    );

// private
/**
 * The RtlLcidToLocaleName routine converts a locale identifier (LCID) to a locale name.
 *
 * \param lcid The locale identifier to convert.
 * \param LocaleName A pointer to the string that receives the locale name. If AllocateDestinationString is `TRUE`, the buffer is allocated by the routine.
 * \param Flags Flags that control the operation.
 * \param AllocateDestinationString If `TRUE`, the routine allocates the destination string buffer.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlLcidToLocaleName(
    _In_ LCID lcid, // sic
    _When_(AllocateDestinationString, _Out_) _When_(!AllocateDestinationString, _In_) PUNICODE_STRING LocaleName,
    _In_ ULONG Flags,
    _In_ BOOLEAN AllocateDestinationString
    );

// private
/**
 * The RtlLocaleNameToLcid routine converts a locale name to a locale identifier (LCID).
 *
 * \param LocaleName A pointer to the locale name to convert.
 * \param lcid A pointer to a variable that receives the locale identifier.
 * \param Flags Flags that control the operation.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlLocaleNameToLcid(
    _In_ PCWSTR LocaleName,
    _Out_ PLCID lcid,
    _In_ ULONG Flags
    );

// private
/**
 * The RtlLCIDToCultureName routine converts a locale identifier (LCID) to its culture name.
 *
 * \param Lcid The locale identifier to convert.
 * \param String A pointer to the string that receives the culture name.
 * \return `TRUE` if the conversion succeeded, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlLCIDToCultureName(
    _In_ LCID Lcid,
    _Inout_ PUNICODE_STRING String
    );

// private
/**
 * The RtlCultureNameToLCID routine converts a culture name to a locale identifier (LCID).
 *
 * \param String A pointer to the culture name to convert.
 * \param Lcid A pointer to a variable that receives the locale identifier.
 * \return `TRUE` if the conversion succeeded, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlCultureNameToLCID(
    _In_ PCUNICODE_STRING String,
    _Out_ PLCID Lcid
    );

// rev
/**
 * The RtlpConvertLCIDsToCultureNames routine converts an array of locale identifiers (LCIDs) to an array of culture names.
 *
 * \param Lcids A pointer to the array of locale identifiers, in string form.
 * \param CultureNames A pointer to a variable that receives the array of culture names.
 * \return `TRUE` if the conversion succeeded, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlpConvertLCIDsToCultureNames(
    _In_ PCWSTR Lcids, // array
    _Out_ PCWSTR* CultureNames
    );

// rev
/**
 * The RtlpConvertCultureNamesToLCIDs routine converts an array of culture names to an array of locale identifiers (LCIDs).
 *
 * \param CultureNames A pointer to the array of culture names.
 * \param Lcids A pointer to a variable that receives the array of locale identifiers, in string form.
 * \return `TRUE` if the conversion succeeded, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlpConvertCultureNamesToLCIDs(
    _In_ PCWSTR CultureNames, // array
    _Out_ PCWSTR* Lcids
    );

// private
/**
 * The RtlCleanUpTEBLangLists routine cleans up the preferred UI language lists stored in the thread environment block (TEB) of the calling thread.
 */
NTSYSAPI
VOID
NTAPI
RtlCleanUpTEBLangLists(
    VOID
    );

// rev from GetThreadPreferredUILanguages
/**
 * The RtlGetThreadPreferredUILanguages routine retrieves the preferred UI languages for the calling thread.
 *
 * \param Flags Flags that control the format of the returned languages (for example, MUI_LANGUAGE_NAME).
 * \param NumberOfLanguages A pointer to a variable that receives the number of languages returned.
 * \param Languages An optional buffer that receives the double-null-terminated list of languages.
 * \param ReturnLength On input, the size of the buffer in characters; on output, the number of characters returned or required.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlGetProcessPreferredUILanguages routine retrieves the preferred UI languages for the current process.
 *
 * \param Flags Flags that control the format of the returned languages (for example, MUI_LANGUAGE_NAME).
 * \param NumberOfLanguages A pointer to a variable that receives the number of languages returned.
 * \param Languages An optional buffer that receives the double-null-terminated list of languages.
 * \param ReturnLength On input, the size of the buffer in characters; on output, the number of characters returned or required.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlGetSystemPreferredUILanguages routine retrieves the preferred UI languages for the system.
 *
 * \param Flags Flags that control the format of the returned languages (for example, MUI_LANGUAGE_NAME).
 * \param LocaleName An optional pointer to a locale name that scopes the query.
 * \param NumberOfLanguages A pointer to a variable that receives the number of languages returned.
 * \param Languages An optional buffer that receives the double-null-terminated list of languages.
 * \param ReturnLength On input, the size of the buffer in characters; on output, the number of characters returned or required.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlpGetSystemDefaultUILanguage routine retrieves the system default UI language.
 *
 * \param DefaultUILanguageId The default UI language identifier.
 * \param Lcid A pointer to a variable that receives the corresponding locale identifier.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpGetSystemDefaultUILanguage(
    _Out_ LANGID DefaultUILanguageId,
    _Inout_ PLCID Lcid
    );

// rev from GetUserPreferredUILanguages
/**
 * The RtlGetUserPreferredUILanguages routine retrieves the preferred UI languages for the current user.
 *
 * \param Flags Flags that control the format of the returned languages (for example, MUI_LANGUAGE_NAME).
 * \param LocaleName An optional pointer to a locale name that scopes the query.
 * \param NumberOfLanguages A pointer to a variable that receives the number of languages returned.
 * \param Languages An optional buffer that receives the double-null-terminated list of languages.
 * \param ReturnLength On input, the size of the buffer in characters; on output, the number of characters returned or required.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlGetUILanguageInfo routine retrieves fallback and attribute information for a set of UI languages.
 *
 * \param Flags Flags that control the operation.
 * \param Languages A pointer to a double-null-terminated list of languages to query.
 * \param FallbackLanguages An optional buffer that receives the double-null-terminated list of fallback languages.
 * \param NumberOfFallbackLanguages On input, the size of the fallback buffer in characters; on output, the number of characters returned or required.
 * \param Attributes A pointer to a variable that receives the language attributes.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Retrieves the base address of the memory-mapped NLS locale data file, mapping and caching it on first use.
 *
 * \param BaseAddress A pointer to a variable that receives the base address of the mapped locale data file.
 * \param DefaultLocaleId A pointer to a variable that receives the default system locale identifier.
 * \param DefaultCasingTableSize An optional pointer to a variable that receives the size, in bytes, of the default casing table.
 * \param CurrentNLSVersion An optional pointer to a variable that receives the current NLS version.
 * \return NTSTATUS Successful or errant status.
 */
// rev
/**
 * The RtlGetLocaleFileMappingAddress routine returns the base address of the mapped locale data file and related default locale information.
 *
 * \param BaseAddress A pointer to a variable that receives the base address of the mapped locale data.
 * \param DefaultLocaleId A pointer to a variable that receives the default locale identifier.
 * \param DefaultCasingTableSize An optional pointer to a variable that receives the size of the default casing table.
 * \param CurrentNLSVersion An optional pointer to a variable that receives the current NLS version.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetLocaleFileMappingAddress(
    _Out_ PVOID *BaseAddress,
    _Out_ PLCID DefaultLocaleId,
    _Out_opt_ PLARGE_INTEGER DefaultCasingTableSize,
    _Out_opt_ PULONG CurrentNLSVersion
    );

//
// MUI / Languages
//

/**
 * Restores the thread preferred UI language state previously captured by RtlSetThreadPreferredUILanguages2 and releases the saved-state block.
 *
 * \param State A pointer to the opaque saved-state block returned through the SavedState parameter of RtlSetThreadPreferredUILanguages2.
 * \return BOOLEAN TRUE if the saved state was restored and freed; otherwise the routine raises a critical failure when the state does not belong to the current thread.
 * \remarks Prototype reconstructed from disassembly. The saved-state block records the previous PreferredLanguages, MergedPrefLanguages and UserPrefLanguages lists together with the owning thread id.
 */
// rev
/**
 * The RtlRestoreThreadPreferredUILanguages routine restores the thread's preferred UI languages from a previously saved state.
 *
 * \param State A pointer to the saved state returned by a previous call to RtlSetThreadPreferredUILanguages2.
 * \return `TRUE` if the languages were restored, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlRestoreThreadPreferredUILanguages(
    _In_ PVOID State
    );

// rev
/**
 * The RtlSetProcessPreferredUILanguages routine sets the preferred UI languages for the current process.
 *
 * \param Flags Flags that control the operation.
 * \param LanguagesBuffer An optional pointer to the double-null-terminated list of languages to set.
 * \param NumberOfLanguages An optional pointer to a variable that receives the number of languages set.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetProcessPreferredUILanguages(
    _In_ ULONG Flags,
    _In_opt_ PUSHORT LanguagesBuffer,
    _Out_opt_ PULONG NumberOfLanguages
    );

/**
 * Sets the preferred UI languages for the current thread.
 *
 * \param Flags Flags controlling the language format and matching behavior (MUI_LANGUAGE_ID, MUI_LANGUAGE_NAME, etc.).
 * \param LanguagesBuffer An optional null-null terminated multi-string of preferred UI languages. When NULL the thread language list is cleared.
 * \param NumberOfLanguages An optional pointer to a variable that receives the number of languages that were set.
 * \return NTSTATUS Successful or errant status.
 * \remarks Prototype reconstructed from disassembly; the trailing Reserved parameter present in earlier headers does not exist.
 */
// rev
/**
 * The RtlSetThreadPreferredUILanguages routine sets the preferred UI languages for the calling thread.
 *
 * \param Flags Flags that control the operation.
 * \param LanguagesBuffer An optional pointer to the double-null-terminated list of languages to set.
 * \param NumberOfLanguages An optional pointer to a variable that receives the number of languages set.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetThreadPreferredUILanguages(
    _In_ ULONG Flags,
    _In_opt_ PCZZWSTR LanguagesBuffer,
    _Out_opt_ PULONG NumberOfLanguages
    );

/**
 * Sets the preferred UI languages for the current thread and optionally captures the previous state for later restoration.
 *
 * \param Flags Flags controlling the language format and matching behavior (MUI_LANGUAGE_ID, MUI_LANGUAGE_NAME, etc.).
 * \param LanguagesBuffer An optional null-null terminated multi-string of preferred UI languages.
 * \param NumberOfLanguages An optional pointer to a variable that receives the number of languages that were set.
 * \param SavedState An optional pointer to a variable that receives an opaque saved-state block to be passed to RtlRestoreThreadPreferredUILanguages.
 * \return NTSTATUS Successful or errant status.
 * \remarks Prototype reconstructed from disassembly; SavedState receives a pointer (not a scalar cookie).
 */
// rev
/**
 * The RtlSetThreadPreferredUILanguages2 routine sets the preferred UI languages for the calling thread and returns a state that can be used to restore the previous setting.
 *
 * \param Flags Flags that control the operation.
 * \param LanguagesBuffer An optional pointer to the double-null-terminated list of languages to set.
 * \param NumberOfLanguages An optional pointer to a variable that receives the number of languages set.
 * \param SavedState An optional pointer to a variable that receives the saved state, which can be passed to RtlRestoreThreadPreferredUILanguages.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetThreadPreferredUILanguages2(
    _In_ ULONG Flags,
    _In_opt_ PCZZWSTR LanguagesBuffer,
    _Out_opt_ PULONG NumberOfLanguages,
    _Out_opt_ PVOID *SavedState
    );

/**
 * Resolves the locale identifier associated with a language-info node using the MUI registry information string pool.
 *
 * \param RegistryInfo The MUI registry information block that owns the string pool referenced by the node.
 * \param LangInfoNode A pointer to the language-info node to resolve.
 * \param Lcid A pointer to a variable that receives the resolved locale identifier.
 * \return NTSTATUS Successful or errant status.
 */
// rev
/**
 * The RtlpGetLCIDFromLangInfoNode routine retrieves the locale identifier associated with a language information node in the MUI registry information.
 *
 * \param RegistryInfo A pointer to the MUI registry information.
 * \param LangInfoNode A pointer to the language information node.
 * \param Lcid A pointer to a variable that receives the locale identifier.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpGetLCIDFromLangInfoNode(
    _In_ PRTL_MUI_REGISTRY_INFO RegistryInfo,
    _In_ PVOID LangInfoNode,
    _Out_ PUSHORT Lcid
    );

// rev
/**
 * The RtlpGetUserOrMachineUILanguage4NLS routine retrieves the user or machine UI languages for use by NLS.
 *
 * \param UserOrMachine A value that selects whether the user or machine languages are retrieved.
 * \param LanguagesMultiSz An optional buffer that receives the double-null-terminated list of languages.
 * \param LanguageCount On input, the size of the buffer; on output, the number of languages returned or required.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpGetUserOrMachineUILanguage4NLS(
    _In_ ULONG UserOrMachine,
    _Out_writes_opt_(*LanguageCount) PWSTR LanguagesMultiSz,
    _Inout_ PULONGLONG LanguageCount
    );

/**
 * Determines whether a language node qualifies against the installed and parent language set.
 *
 * \param RegistryInfo Context describing the installed language set to validate against.
 * \param LangNode A pointer to the language node (culture identifier) to test.
 * \param CheckInstallLanguage When TRUE the install language is included in the qualification check.
 * \return NTSTATUS STATUS_SUCCESS (with the qualified result encoded by the routine) or an errant status.
 */
// rev
/**
 * The RtlpIsQualifiedLanguage routine determines whether a language node identifies a qualified (supported) language.
 *
 * \param RegistryInfo A pointer to the MUI registry information.
 * \param LangNode A pointer to the language node to test.
 * \param CheckInstallLanguage If `TRUE`, the install language is also considered.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpIsQualifiedLanguage(
    _In_ PVOID RegistryInfo,
    _In_ PSHORT LangNode,
    _In_ BOOLEAN CheckInstallLanguage
    );

/**
 * Frees every sub-allocation owned by an MUI registry information block and releases the block itself.
 *
 * \param RegistryInfo The MUI registry information block to free.
 * \return NTSTATUS Successful or errant status.
 * \remarks Prototype reconstructed from disassembly; this is a full-teardown wrapper around RtlpMuiRegFreeRegistryInfo(RegistryInfo, 0xFFF) followed by RtlFreeHeap.
 */
// rev
/**
 * The RtlpMuiFreeLangRegistryInfo routine frees the language portion of an MUI registry information structure.
 *
 * \param RegistryInfo A pointer to the MUI registry information to free.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpMuiFreeLangRegistryInfo(
    _In_ PRTL_MUI_REGISTRY_INFO RegistryInfo
    );

/**
 * Allocates and zero-initializes an empty MUI registry information block.
 *
 * \return A pointer to the newly allocated MUI registry information block, or NULL on allocation failure.
 */
// rev
/**
 * The RtlpMuiRegCreateRegistryInfo routine allocates and initializes an MUI registry information structure.
 *
 * \return A pointer to the allocated MUI registry information, or `NULL` on failure.
 */
NTSYSAPI
PRTL_MUI_REGISTRY_INFO
NTAPI
RtlpMuiRegCreateRegistryInfo(
    VOID
    );

/**
 * Selectively frees the sub-allocations owned by an MUI registry information block according to a free mask.
 *
 * \param RegistryInfo The MUI registry information block to operate on.
 * \param FreeMask A bitmask selecting which owned members to release (0xFFF frees all standard categories).
 * \return NTSTATUS Successful or errant status.
 * \remarks Prototype reconstructed from disassembly; the block itself is not freed, only its selected members.
 */
// rev
/**
 * The RtlpMuiRegFreeRegistryInfo routine frees the resources associated with an MUI registry information structure.
 *
 * \param RegistryInfo A pointer to the MUI registry information to free.
 * \param FreeMask A mask that specifies which portions of the structure to free.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpMuiRegFreeRegistryInfo(
    _Inout_ PRTL_MUI_REGISTRY_INFO RegistryInfo,
    _In_ ULONG FreeMask
    );

/**
 * Loads the requested categories of MUI registry information into an existing block.
 *
 * \param RegistryInfo The MUI registry information block to populate.
 * \param LoadMask A bitmask selecting which categories to load (installed languages, license information, fallback and language-configuration lists).
 * \return NTSTATUS Successful or errant status.
 * \remarks Prototype reconstructed from disassembly.
 */
// rev
/**
 * The RtlpMuiRegLoadRegistryInfo routine loads MUI language information from the registry into an MUI registry information structure.
 *
 * \param RegistryInfo A pointer to the MUI registry information to populate.
 * \param LoadMask A mask that specifies which portions of the structure to load.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpMuiRegLoadRegistryInfo(
    _Inout_ PRTL_MUI_REGISTRY_INFO RegistryInfo,
    _In_ SHORT LoadMask
    );

// rev
/**
 * The RtlpQueryDefaultUILanguage routine queries the default UI language.
 *
 * \param DefaultLanguage A pointer to a variable that receives the default UI language identifier.
 * \param ForceMachinePolicy If `TRUE`, the machine policy is used to determine the default language.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpQueryDefaultUILanguage(
    _Out_ PUSHORT DefaultLanguage,
    _In_ BOOLEAN ForceMachinePolicy
    );

// rev
/**
 * The RtlpRefreshCachedUILanguage routine refreshes the cached UI language value.
 *
 * \param SourceString A pointer to the language string used to refresh the cache.
 * \param CommitImmediately If `TRUE`, the refreshed value is committed immediately.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpRefreshCachedUILanguage(
    _In_ PCWSTR SourceString,
    _In_ BOOLEAN CommitImmediately
    );

// rev
/**
 * The RtlpSetInstallLanguage routine sets the install language.
 *
 * \param Flags Flags that control the operation.
 * \param Language A pointer to the null-terminated install language string.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpSetInstallLanguage(
    _In_ CHAR Flags,
    _In_z_ PCWSTR Language
    );

/**
 * Applies a preferred UI language multi-string to the current process or thread scope selected by Flags.
 *
 * \param Flags Flags controlling the target scope and language format.
 * \param LanguagesMultiSz An optional null-null terminated multi-string of preferred UI languages.
 * \param LanguagesCount An optional pointer to a variable that receives the number of languages that were applied.
 * \param Reserved Reserved; must be NULL.
 * \return NTSTATUS Successful or errant status.
 */
// rev
/**
 * The RtlpSetPreferredUILanguages routine sets the preferred UI languages using the specified multi-string language list.
 *
 * \param Flags Flags that control the operation.
 * \param LanguagesMultiSz An optional pointer to the double-null-terminated list of languages to set.
 * \param LanguagesCount An optional pointer to a variable that receives the number of languages set.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpSetPreferredUILanguages(
    _In_ ULONG Flags,
    _In_opt_z_ PCWSTR LanguagesMultiSz,
    _Out_opt_ PULONG LanguagesCount
    );

// rev
/**
 * The RtlpSetUserPreferredUILanguages routine sets the preferred UI languages for the current user using the specified multi-string language list.
 *
 * \param Flags Flags that control the operation.
 * \param LanguagesMultiSz An optional pointer to the double-null-terminated list of languages to set.
 * \param LanguagesCount An optional pointer to a variable that receives the number of languages set.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpSetUserPreferredUILanguages(
    _In_ ULONG Flags,
    _In_opt_z_ PWSTR LanguagesMultiSz,
    _Out_opt_ PULONG LanguagesCount
    );

// rev
/**
 * The RtlpVerifyAndCommitUILanguageSettings routine verifies and commits the pending UI language settings.
 *
 * \param ShutdownOnFailure If `TRUE`, the process is shut down if the settings cannot be committed.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpVerifyAndCommitUILanguageSettings(
    _In_ BOOLEAN ShutdownOnFailure
    );

//
// PEB
//

/**
 * The RtlGetCurrentPeb routine returns a pointer to the process environment block (PEB) of the current process.
 *
 * \return A pointer to the PEB of the current process.
 */
NTSYSAPI
PPEB
NTAPI
RtlGetCurrentPeb(
    VOID
    );

/**
 * The RtlAcquirePebLock routine acquires the lock that protects the process environment block (PEB).
 *
 * \return NTSTATUS Successful or errant status.
 */
_Acquires_exclusive_lock_(*NtCurrentPeb()->FastPebLock)
NTSYSAPI
NTSTATUS
NTAPI
RtlAcquirePebLock(
    VOID
    );

/**
 * The RtlReleasePebLock routine releases the lock that protects the process environment block (PEB).
 *
 * \return NTSTATUS Successful or errant status.
 */
_Releases_exclusive_lock_(*NtCurrentPeb()->FastPebLock)
NTSYSAPI
NTSTATUS
NTAPI
RtlReleasePebLock(
    VOID
    );

// private
/**
 * The RtlTryAcquirePebLock routine attempts to acquire the process environment block (PEB) lock without blocking.
 *
 * \return A nonzero value if the lock was acquired, otherwise zero.
 */
_When_(return != 0, _Acquires_exclusive_lock_(*NtCurrentPeb()->FastPebLock))
NTSYSAPI
LOGICAL
NTAPI
RtlTryAcquirePebLock(
    VOID
    );

/**
 * The RtlAllocateFromPeb routine allocates a block of memory from the process environment block (PEB) heap.
 *
 * \param Size The number of bytes to allocate.
 * \param Block Receives a pointer to the allocated block.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAllocateFromPeb(
    _In_ ULONG Size,
    _Out_ PVOID *Block
    );

/**
 * The RtlFreeToPeb routine frees a block of memory previously allocated with RtlAllocateFromPeb.
 *
 * \param Block The block to free.
 * \param Size The size, in bytes, of the block.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * Flags describing how the current directory handle is inherited by a new process.
 */
#define RTL_USER_PROC_CURDIR_CLOSE 0x00000002
#define RTL_USER_PROC_CURDIR_INHERIT 0x00000003

/**
 * Represents the current directory of a process.
 */
typedef struct _CURDIR
{
    UNICODE_STRING DosPath;
    HANDLE Handle;
} CURDIR, *PCURDIR;

// RTL_DRIVE_LETTER_CURDIR Flags
/**
 * Constants describing the per-drive current directory table.
 */
#define RTL_MAX_DRIVE_LETTERS 32
#define RTL_DRIVE_LETTER_VALID (USHORT)0x0001

/**
 * Represents a saved per-drive current directory entry.
 */
typedef struct _RTL_DRIVE_LETTER_CURDIR
{
    USHORT Flags;
    USHORT Length;
    ULONG TimeStamp;
    STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

/**
 * Console and window creation flags for a new user process.
 */
#define RTL_USER_PROC_DETACHED_PROCESS ((HANDLE)(LONG_PTR)-1)
#define RTL_USER_PROC_CREATE_NEW_CONSOLE ((HANDLE)(LONG_PTR)-2)
#define RTL_USER_PROC_CREATE_NO_WINDOW ((HANDLE)(LONG_PTR)-3)

/**
 * Contains the parameters used to create and initialize a user-mode process environment.
 */
typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    ULONG MaximumLength;
    ULONG Length;

    ULONG Flags; // RTL_USER_PROC_FLAGS
    ULONG DebugFlags; // RTL_USER_DEBUG_FLAGS

    HANDLE ConsoleHandle;
    ULONG ConsoleFlags; // RTL_USER_PROC_CONSOLE_FLAGS
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

    ULONG WindowFlags; // RTL_USER_PROC_WINDOW_FLAGS
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
    ULONG LoaderThreads; // THRESHOLD
    UNICODE_STRING RedirectionDllName; // REDSTONE5
    UNICODE_STRING HeapPartitionName; // 19H1
    PULONGLONG DefaultThreadpoolCpuSetMasks;
    ULONG DefaultThreadpoolCpuSetMaskCount;
    ULONG DefaultThreadpoolThreadMaximum; // 20H1
    ULONG HeapMemoryTypeMask; // WIN11 22H2
    PVOID AttributeList;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

// RTL_USER_PROCESS_PARAMETERS Flags
/**
 * Flags describing the state of an RTL_USER_PROCESS_PARAMETERS block.
 */
#define RTL_USER_PROC_PARAMS_NORMALIZED                 0x00000001 // Pointer representation: 1=absolute pointers, 0=relative offsets; set by RtlNormalizeProcessParams, cleared by RtlDeNormalizeProcessParams
#define RTL_USER_PROC_PROFILE_USER                      0x00000002 // User-mode profiling enabled
#define RTL_USER_PROC_PROFILE_KERNEL                    0x00000004 // Kernel-mode profiling enabled
#define RTL_USER_PROC_PROFILE_SERVER                    0x00000008 // Server-mode profiling enabled
//#define RTL_USER_PROC_RESERVE_64K                     0x00000010 // Unused/reserved
/**
 * Reserved address-space and flag values for process parameter creation.
 */
#define RTL_USER_PROC_RESERVE_1MB                       0x00000020 // Reserve 1MB virtual memory (mutually exclusive group)
#define RTL_USER_PROC_RESERVE_16MB                      0x00000040 // Reserve 16MB virtual memory (mutually exclusive group)
#define RTL_USER_PROC_CASE_SENSITIVE                    0x00000080 // Enable case-sensitive filename matching (NTFS)
#define RTL_USER_PROC_DISABLE_HEAP_DECOMMIT             0x00000100 // Disable heap decommitment
#define RTL_USER_PROC_DLL_REDIRECTION_LOCAL             0x00001000 // Enable local DLL redirection (.local manifest behavior)
#define RTL_USER_PROC_APP_MANIFEST_PRESENT              0x00002000 // Application manifest is present
#define RTL_USER_PROC_IMAGE_KEY_MISSING                 0x00004000 // Process registry image key not found (masked during kernel flag validation)
#define RTL_USER_PROC_DEV_OVERRIDE_ENABLED              0x00008000 // Developer override configuration enabled
#define RTL_USER_PROC_OPTIN_PROCESS                     0x00020000 // Process opted into mitigations (Windows 8+)
#define RTL_USER_PROC_SESSION_OWNER                     0x00040000 // Process is session owner
#define RTL_USER_PROC_HANDLE_USER_CALLBACK_EXCEPTIONS   0x00080000 // Handle user callback exceptions
#define RTL_USER_PROC_PROTECTED_PROCESS                 0x00400000 // Process is protected (Windows Vista+)
#define RTL_USER_PROC_RESERVE_PLACEHOLDER               0x01000000 // Reserved user-mapping placeholder mode; grouped with RTL_USER_PROC_RESERVE* flags // PspSetupReservedUserMappings
#define RTL_USER_PROC_SECURE_PROCESS                    0x80000000 // Process is secure (Windows 11+); rejected by current kernel capture validation path

/**
 * The RtlCreateProcessParameters routine creates and initializes a process parameters block for a new process.
 *
 * \param ProcessParameters Receives the newly allocated process parameters block.
 * \param ImagePathName The image path name of the process.
 * \param DllPath An optional DLL search path.
 * \param CurrentDirectory An optional current directory.
 * \param CommandLine An optional command line.
 * \param Environment An optional environment block for the process.
 * \param WindowTitle An optional window title.
 * \param DesktopInfo An optional desktop information string.
 * \param ShellInfo An optional shell information string.
 * \param RuntimeData An optional runtime data string.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlCreateProcessParametersEx routine creates and initializes a process parameters block for a new process, with control over normalization.
 *
 * \param ProcessParameters Receives the newly allocated process parameters block.
 * \param ImagePathName The image path name of the process.
 * \param DllPath An optional DLL search path.
 * \param CurrentDirectory An optional current directory.
 * \param CommandLine An optional command line.
 * \param Environment An optional environment block for the process.
 * \param WindowTitle An optional window title.
 * \param DesktopInfo An optional desktop information string.
 * \param ShellInfo An optional shell information string.
 * \param RuntimeData An optional runtime data string.
 * \param Flags Pass RTL_USER_PROC_PARAMS_NORMALIZED to keep the parameters normalized.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlCreateProcessParametersWithTemplate routine creates a process parameters block for a new process using a redirection DLL template.
 *
 * \param ProcessParameters Receives the newly allocated process parameters block.
 * \param ImagePathName The image path name of the process.
 * \param DllPath An optional DLL search path.
 * \param CurrentDirectory An optional current directory.
 * \param CommandLine An optional command line.
 * \param ProcessParameters Receives a pointer to the created process parameters block.
 * \param Template Pointer to an existing process parameters block to use as a template.
 * \param Flags Pass RTL_USER_PROC_PARAMS_NORMALIZED to keep parameters normalized.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateProcessParametersWithTemplate(
    _Out_ PRTL_USER_PROCESS_PARAMETERS *ProcessParameters,
    _In_ PRTL_USER_PROCESS_PARAMETERS Template,
    _In_ ULONG Flags // pass RTL_USER_PROC_PARAMS_NORMALIZED to keep parameters normalized
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS4

/**
 * The RtlDestroyProcessParameters routine releases the memory used by a process parameters block created by RtlCreateProcessParameters.
 *
 * \param ProcessParameters A pointer to the process parameters block to destroy.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyProcessParameters(
    _In_ _Post_invalid_ PRTL_USER_PROCESS_PARAMETERS ProcessParameters
    );

/**
 * The RtlNormalizeProcessParams routine converts the embedded pointers in a process parameters block from relative offsets to absolute addresses.
 *
 * \param ProcessParameters A pointer to the process parameters block to normalize.
 * \return A pointer to the normalized process parameters block.
 */
NTSYSAPI
PRTL_USER_PROCESS_PARAMETERS
NTAPI
RtlNormalizeProcessParams(
    _Inout_ PRTL_USER_PROCESS_PARAMETERS ProcessParameters
    );

/**
 * The RtlDeNormalizeProcessParams routine converts the embedded pointers in a process parameters block from absolute addresses back to relative offsets.
 *
 * \param ProcessParameters A pointer to the process parameters block to de-normalize.
 * \return A pointer to the de-normalized process parameters block.
 */
NTSYSAPI
PRTL_USER_PROCESS_PARAMETERS
NTAPI
RtlDeNormalizeProcessParams(
    _Inout_ PRTL_USER_PROCESS_PARAMETERS ProcessParameters
    );

/**
 * Receives information about a process created by RtlCreateUserProcess.
 */
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
 * The RtlCreateUserProcess routine creates a new process and its primary thread. The new process runs in the security context of the calling process.
 *
 * \param NtImagePathName The path of the image to be executed.
 * \param ExtendedParameters Reserved
 * \param ProcessParameters The process parameter information.
 * \param ProcessSecurityDescriptor The security descriptor for the new process. If NULL, the process gets a default security descriptor.
 * \param ThreadSecurityDescriptor The security descriptor for the initial thread. If NULL, the thread gets a default security descriptor.
 * \param ParentProcess The handle of a process to use (instead of the calling process) as the parent for the process being created.
 * \param InheritHandles If this parameter is TRUE, each inheritable handle in the calling process is inherited by the new process.
 * \param DebugPort The handle of an ALPC port for debug messages. If NULL, the process gets a default port. (WindowsErrorReportingServicePort)
 * \param TokenHandle The handle of a Token to use as the security context.
 * \param ProcessInformation The user process information.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw
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

/**
 * Version number of the RTL_USER_PROCESS_EXTENDED_PARAMETERS structure.
 */
#define RTL_USER_PROCESS_EXTENDED_PARAMETERS_VERSION 1

// private
/**
 * Specifies extended parameters for creating a user-mode process.
 */
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
/**
 * The RtlCreateUserProcessEx routine creates a new process and its primary thread, with extended parameters.
 *
 * \param NtImagePathName Pointer to a UNICODE_STRING that specifies the path of the image to be executed.
 * \param ProcessParameters Pointer to a RTL_USER_PROCESS_PARAMETERS structure that contains process parameter information.
 * \param InheritHandles If TRUE, each inheritable handle in the calling process is inherited by the new process.
 * \param ProcessExtendedParameters Optional pointer to a RTL_USER_PROCESS_EXTENDED_PARAMETERS structure for additional process creation options. Can be NULL.
 * \param ProcessInformation Pointer to a RTL_USER_PROCESS_INFORMATION structure that receives information about the new process and its primary thread.
 * \return NTSTATUS Successful or errant status.
 * \remarks This function is available on Windows 10 RS2 and later. It allows for more advanced process creation scenarios than RtlCreateUserProcess.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw
 */
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
 * The RtlExitUserProcess routine ends the calling process and all its threads.
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
/**
 * Flags for RtlCloneUserProcess.
 */
#define RTL_CLONE_PROCESS_FLAGS_CREATE_SUSPENDED 0x00000001
#define RTL_CLONE_PROCESS_FLAGS_INHERIT_HANDLES 0x00000002
#define RTL_CLONE_PROCESS_FLAGS_NO_SYNCHRONIZE 0x00000004 // don't update synchronization objects
// end_rev

// private
/**
 * The RtlCloneUserProcess routine creates a new process from the current process.
 *
 * \param ProcessFlags The path of the image to be executed.
 * \param ProcessSecurityDescriptor The security descriptor for the new process. If NULL, the process gets a default security descriptor.
 * \param ThreadSecurityDescriptor The security descriptor for the initial thread. If NULL, the thread gets a default security descriptor.
 * \param DebugPort The handle of an ALPC port for debug messages. If NULL, the process gets a default port. (WindowsErrorReportingServicePort)
 * \param ProcessInformation The new process information.
 * \return NTSTATUS Successful or errant status.
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
/**
 * The RtlPrepareForProcessCloning routine prepares the current process for cloning by acquiring the locks required to produce a consistent clone.
 *
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlPrepareForProcessCloning(
    VOID
    );

// rev
/**
 * The RtlCompleteProcessCloning routine completes a process cloning operation started by RtlPrepareForProcessCloning, releasing the locks acquired for cloning.
 *
 * \param Completed A value indicating whether the clone completed successfully.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCompleteProcessCloning(
    _In_ LOGICAL Completed
    );

// private
/**
 * The RtlUpdateClonedCriticalSection routine updates a critical section in a cloned process so that it is usable in the clone.
 *
 * \param CriticalSection A pointer to the critical section to update.
 */
NTSYSAPI
VOID
NTAPI
RtlUpdateClonedCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );

// private
/**
 * The RtlUpdateClonedSRWLock routine updates a slim reader/writer (SRW) lock in a cloned process so that it is usable in the clone.
 *
 * \param SRWLock A pointer to the SRW lock to update.
 * \param Shared A value indicating whether the lock is set to a shared acquire (`TRUE`) or an exclusive acquire (`FALSE`).
 */
NTSYSAPI
VOID
NTAPI
RtlUpdateClonedSRWLock(
    _Inout_ PRTL_SRWLOCK SRWLock,
    _In_ LOGICAL Shared // TRUE to set to shared acquire
    );

// rev RtlCloneUserProcess Flags
/**
 * Flags for RtlCreateProcessReflection.
 */
#define RTL_PROCESS_REFLECTION_FLAGS_CREATE_SUSPENDED 0x00000001
#define RTL_PROCESS_REFLECTION_FLAGS_INHERIT_HANDLES  0x00000002
#define RTL_PROCESS_REFLECTION_FLAGS_NO_SUSPEND       0x00000004
#define RTL_PROCESS_REFLECTION_FLAGS_NO_SYNCHRONIZE   0x00000008
#define RTL_PROCESS_REFLECTION_FLAGS_NO_CLOSE_EVENT   0x00000010

// private
/**
 * Receives information about a process created by reflection (cloning).
 */
typedef struct _RTLP_PROCESS_REFLECTION_REFLECTION_INFORMATION
{
    HANDLE ReflectionProcessHandle;
    HANDLE ReflectionThreadHandle;
    CLIENT_ID ReflectionClientId;
} RTLP_PROCESS_REFLECTION_REFLECTION_INFORMATION, *PRTLP_PROCESS_REFLECTION_REFLECTION_INFORMATION;

/**
 * Public alias for the process reflection information structure.
 */
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
    _Out_opt_ PPROCESS_REFLECTION_INFORMATION ReflectionInformation
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
 * \param SubProcessTag Pointer to the tag value to set.
 * \return The previous sub-process tag value.
 */
NTSYSAPI
PVOID
NTAPI
RtlSetThreadSubProcessTag(
    _In_ PVOID SubProcessTag
    );

// rev
/**
 * The RtlValidProcessProtection function validates the process protection level.
 *
 * \param ProcessProtection Pointer to a PS_PROTECTION structure describing the protection.
 * \return TRUE if the protection level is valid, FALSE otherwise.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlValidProcessProtection(
    _In_ PS_PROTECTION ProcessProtection
    );

// rev
/**
 * The RtlTestProtectedAccess function tests whether a source protection level can access a target protection level.
 *
 * \param Source Pointer to a PS_PROTECTION structure for the source.
 * \param Target Pointer to a PS_PROTECTION structure for the target.
 * \return TRUE if access is allowed, FALSE otherwise.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlTestProtectedAccess(
    _In_ PS_PROTECTION Source,
    _In_ PS_PROTECTION Target
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS3)
/**
 * The RtlIsCurrentProcess function determines whether the specified process handle refers to the current process.
 *
 * \param ProcessHandle Handle to the process to compare with the current process.
 * \return TRUE if the handle refers to the current process; otherwise, FALSE.
 * \remarks Internally compares the specified handle with the current process handle using NtCompareObjects.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsCurrentProcess( // NtCompareObjects(NtCurrentProcess(), ProcessHandle)
    _In_ HANDLE ProcessHandle
    );

/**
 * The RtlIsCurrentThread function determines whether the specified thread handle refers to the current thread.
 *
 * \param ThreadHandle Handle to the thread to compare with the current thread.
 * \return TRUE if the handle refers to the current thread; otherwise, FALSE.
 * \remarks Internally compares the specified handle with the current thread handle using NtCompareObjects.
 */
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
/**
 * Pointer to a USER_THREAD_START_ROUTINE callback.
 */
typedef USER_THREAD_START_ROUTINE* PUSER_THREAD_START_ROUTINE;

/**
 * The RtlCreateUserThread routine creates a thread in the specified process.
 *
 * \param ProcessHandle Handle to the process in which the thread is to be created.
 * \param ThreadSecurityDescriptor Optional pointer to a security descriptor for the new thread. If NULL, the thread gets a default security descriptor.
 * \param CreateSuspended If TRUE, the thread is created in a suspended state and must be resumed explicitly. If FALSE, the thread starts running immediately.
 * \param ZeroBits Optional number of high-order address bits that must be zero in the stack's base address. Usually set to 0.
 * \param MaximumStackSize Optional maximum size, in bytes, of the stack for the new thread. If 0, the default size is used.
 * \param CommittedStackSize Optional initial size, in bytes, of committed stack for the new thread. If 0, the default size is used.
 * \param StartAddress Pointer to the application-defined function to be executed by the thread.
 * \param Parameter Optional pointer to a variable to be passed to the thread function.
 * \param ThreadHandle Optional pointer to a variable that receives the handle of the new thread.
 * \param ClientId Optional pointer to a CLIENT_ID structure that receives the thread and process identifiers.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlExitUserThread routine ends the calling thread and returns the specified exit status.
 *
 * \param ExitStatus The exit status for the thread.
 * \remarks This function does not return to the caller. It terminates the thread immediately.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-exitthread
 */
_Analysis_noreturn_
DECLSPEC_NORETURN
NTSYSAPI
VOID
NTAPI
RtlExitUserThread(
    _In_ NTSTATUS ExitStatus
    );

// rev
/**
 * The RtlIsCurrentThreadAttachExempt routine determines whether the current thread is exempt from attach notifications.
 *
 * \return TRUE if the current thread is attach-exempt; otherwise, FALSE.
 * \remarks Attach-exempt threads do not receive DLL_THREAD_ATTACH and DLL_THREAD_DETACH notifications.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsCurrentThreadAttachExempt(
    VOID
    );

/**
 * The RtlCreateUserStack routine allocates and initializes a user-mode stack for a new thread.
 *
 * \param CommittedStackSize The initial size, in bytes, of committed stack. If 0, the default is used.
 * \param MaximumStackSize The maximum size, in bytes, of the stack. If 0, the default is used.
 * \param ZeroBits The number of high-order address bits that must be zero in the stack's base address. Usually set to 0.
 * \param PageSize The system page size, in bytes.
 * \param ReserveAlignment The alignment for the reserved stack region.
 * \param InitialTeb Pointer to an INITIAL_TEB structure that receives the stack information.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlFreeUserStack routine frees a user-mode stack previously allocated for a thread.
 *
 * \param AllocationBase The base address of the stack allocation to free.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlFreeUserStack(
    _In_ PVOID AllocationBase
    );

//
// Extended thread context
//

/**
 * Describes the location and size of a single chunk within an extended CONTEXT structure.
 */
typedef struct _CONTEXT_CHUNK
{
    LONG Offset; // Offset may be negative.
    ULONG Length;
} CONTEXT_CHUNK, *PCONTEXT_CHUNK;

/**
 * Extends the CONTEXT structure with optional, variable-length processor-state chunks.
 */
typedef struct _CONTEXT_EX
{
    CONTEXT_CHUNK All;
    CONTEXT_CHUNK Legacy;
    CONTEXT_CHUNK XState;
    CONTEXT_CHUNK KernelCet;
} CONTEXT_EX, *PCONTEXT_EX;

#if defined(_AMD64_) || defined(_ARM64_) || defined(_ARM64EC_)
/**
 * Alignment of the CONTEXT structure for the current architecture.
 */
#define CONTEXT_ALIGN 0x10
#else
#define CONTEXT_ALIGN 0x8
#endif // _AMD64_ || _ARM64_ || _ARM64EC_

#if defined(_AMD64_)
/**
 * Size and length constants of the CONTEXT structure for the current architecture.
 */
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

/**
 * Alignment, in bytes, of a CONTEXT structure.
 */
#define CONTEXT_ALIGNMENT(Size, Align) \
    (((ULONG_PTR)(Size) + (Align) - 1) & ~((Align) - 1))

/**
 * Size, in bytes, of the CONTEXT_EX header.
 */
#define CONTEXT_EX_LENGTH \
    CONTEXT_ALIGNMENT(sizeof(CONTEXT_EX), CONTEXT_ALIGN)

static_assert(CONTEXT_FRAME_LENGTH == sizeof(CONTEXT));
static_assert(CONTEXT_EX_LENGTH == 0x20);

/**
 * Accessor macros for locating chunks within a CONTEXT_EX structure.
 */
#define RTL_CONTEXT_EX_OFFSET(ContextEx, Chunk) ((ContextEx)->Chunk.Offset)
#define RTL_CONTEXT_EX_LENGTH(ContextEx, Chunk) ((ContextEx)->Chunk.Length)
#define RTL_CONTEXT_EX_CHUNK(Base, Layout, Chunk) ((PVOID)((PUCHAR)(Base) + RTL_CONTEXT_EX_OFFSET(Layout, Chunk)))
#define RTL_CONTEXT_OFFSET(Context, Chunk) RTL_CONTEXT_EX_OFFSET((PCONTEXT_EX)((Context) + 1), Chunk)
#define RTL_CONTEXT_LENGTH(Context, Chunk) RTL_CONTEXT_EX_LENGTH((PCONTEXT_EX)((Context) + 1), Chunk)
#define RTL_CONTEXT_CHUNK(Context, Chunk) RTL_CONTEXT_EX_CHUNK((PCONTEXT_EX)((Context) + 1), (PCONTEXT_EX)((Context) + 1), Chunk)

/**
 * The RtlInitializeContext function initializes a CONTEXT structure.
 *
 * \param ProcessHandle Handle to the process to write the CONTEXT. (32bit only)
 * \param Context A pointer to a buffer within which to initialize a CONTEXT structure.
 * \param Parameter Optional parameter passed to the thread start routine.
 * \param InitialPc Initial instruction pointer (thread start routine).
 * \param InitialSp Initial stack pointer.
 * \return On 32bit, returns the status of NtWriteVirtualMemory. On 64bit, returns a constant value (0xf0e0d0c0a0908070).
 * \remarks The return value on 64bit systems is not an NTSTATUS; callers should ignore it.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-initializecontext
 */
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

/**
 * The RtlInitializeExtendedContext routine initializes an extended context structure within a caller-supplied buffer.
 *
 * \param Context A pointer to the buffer that receives the initialized context.
 * \param ContextFlags Flags that specify which portions of the context are initialized.
 * \param ContextEx A pointer to a variable that receives a pointer to the extended context.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeExtendedContext(
    _Out_ PCONTEXT Context,
    _In_ ULONG ContextFlags,
    _Out_ PCONTEXT_EX* ContextEx
    );

/**
 * The RtlInitializeExtendedContext2 routine initializes an extended context structure within a caller-supplied buffer, using the specified set of enabled extended features.
 *
 * \param Context A pointer to the buffer that receives the initialized context.
 * \param ContextFlags Flags that specify which portions of the context are initialized.
 * \param ContextEx A pointer to a variable that receives a pointer to the extended context.
 * \param EnabledExtendedFeatures A mask of the enabled extended processor features.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeExtendedContext2(
    _Out_ PCONTEXT Context,
    _In_ ULONG ContextFlags,
    _Out_ PCONTEXT_EX* ContextEx,
    _In_ ULONG64 EnabledExtendedFeatures // RtlGetEnabledExtendedFeatures(-1)
    );

/**
 * The RtlCopyContext routine copies the specified portions of a context structure.
 *
 * \param Context A pointer to the destination context.
 * \param ContextFlags Flags that specify which portions of the context are copied.
 * \param Source A pointer to the source context.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCopyContext(
    _Inout_ PCONTEXT Context,
    _In_ ULONG ContextFlags,
    _Out_ PCONTEXT Source
    );

/**
 * The RtlCopyExtendedContext routine copies the specified portions of an extended context structure.
 *
 * \param Destination A pointer to the destination extended context.
 * \param ContextFlags Flags that specify which portions of the context are copied.
 * \param Source A pointer to the source extended context.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCopyExtendedContext(
    _Out_ PCONTEXT_EX Destination,
    _In_ ULONG ContextFlags,
    _In_ PCONTEXT_EX Source
    );

/**
 * The RtlGetExtendedContextLength routine computes the length required for an extended context structure.
 *
 * \param ContextFlags Flags that specify which portions of the context are included.
 * \param ContextLength A pointer to a variable that receives the required length, in bytes.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetExtendedContextLength(
    _In_ ULONG ContextFlags,
    _Out_ PULONG ContextLength
    );

/**
 * The RtlGetExtendedContextLength2 routine computes the length required for an extended context structure, using the specified set of enabled extended features.
 *
 * \param ContextFlags Flags that specify which portions of the context are included.
 * \param ContextLength A pointer to a variable that receives the required length, in bytes.
 * \param EnabledExtendedFeatures A mask of the enabled extended processor features.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetExtendedContextLength2(
    _In_ ULONG ContextFlags,
    _Out_ PULONG ContextLength,
    _In_ ULONG64 EnabledExtendedFeatures // RtlGetEnabledExtendedFeatures(-1)
    );

/**
 * The RtlGetExtendedFeaturesMask routine returns the mask of extended processor features present in an extended context.
 *
 * \param ContextEx A pointer to the extended context.
 * \return The mask of extended processor features.
 */
NTSYSAPI
ULONG64
NTAPI
RtlGetExtendedFeaturesMask(
    _In_ PCONTEXT_EX ContextEx
    );

/**
 * The RtlLocateExtendedFeature routine locates the save area of an extended processor feature within an extended context.
 *
 * \param ContextEx The extended context to search.
 * \param FeatureId The XSTATE feature identifier to locate.
 * \param Length An optional pointer that receives the length, in bytes, of the feature save area.
 * \return A pointer to the feature save area, or NULL if the feature is not present.
 */
NTSYSAPI
PVOID
NTAPI
RtlLocateExtendedFeature(
    _In_ PCONTEXT_EX ContextEx,
    _In_ ULONG FeatureId,
    _Out_opt_ PULONG Length
    );

/**
 * The RtlLocateExtendedFeature2 routine locates the save area of an extended processor feature within an extended context, using an explicit XSTATE configuration.
 *
 * \param ContextEx The extended context to search.
 * \param FeatureId The XSTATE feature identifier to locate.
 * \param XState The XSTATE configuration describing feature layout.
 * \param Length An optional pointer that receives the length, in bytes, of the feature save area.
 * \return A pointer to the feature save area, or NULL if the feature is not present.
 */
NTSYSAPI
PVOID
NTAPI
RtlLocateExtendedFeature2(
    _In_ PCONTEXT_EX ContextEx,
    _In_ ULONG FeatureId,
    _In_ XSTATE_CONFIGURATION XState,
    _Out_opt_ PULONG Length
    );

/**
 * The RtlLocateLegacyContext routine locates the legacy CONTEXT structure within an extended context.
 *
 * \param ContextEx A pointer to the extended context.
 * \param Length An optional pointer to a variable that receives the length of the legacy context.
 * \return A pointer to the legacy CONTEXT structure.
 */
NTSYSAPI
PCONTEXT
NTAPI
RtlLocateLegacyContext(
    _In_ PCONTEXT_EX ContextEx,
    _Out_opt_ PULONG Length
    );

/**
 * The RtlSetExtendedFeaturesMask routine sets the mask of extended processor features in an extended context.
 *
 * \param ContextEx A pointer to the extended context.
 * \param FeatureMask The mask of extended processor features to set.
 */
NTSYSAPI
VOID
NTAPI
RtlSetExtendedFeaturesMask(
    _In_ PCONTEXT_EX ContextEx,
    _In_ ULONG64 FeatureMask
    );

/**
 * The RtlWow64GetThreadContext routine retrieves the WOW64 (32-bit) context of the specified thread.
 *
 * \param ThreadHandle A handle to the thread whose context is retrieved.
 * \param ThreadContext A pointer to a WOW64_CONTEXT structure that receives the thread context.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlWow64SetThreadContext routine sets the WOW64 (32-bit) context of the specified thread.
 *
 * \param ThreadHandle A handle to the thread whose context is set.
 * \param ThreadContext A pointer to a WOW64_CONTEXT structure that contains the thread context.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlRemoteCall routine calls a function in the context of a specified thread in a remote process.
 *
 * \param ProcessHandle Handle to the process in which the thread resides.
 * \param ThreadHandle Handle to the thread in which the function is to be called.
 * \param CallSite Address of the function to call in the remote process.
 * \param ArgumentCount Number of arguments to pass to the function.
 * \param Arguments Pointer to an array of arguments to pass to the function. Can be NULL if no arguments are needed.
 * \param PassContext If TRUE, the thread context is passed to the function.
 * \param AlreadySuspended If TRUE, the thread is already suspended and does not need to be suspended by this routine.
 * \return NTSTATUS Successful or errant status.
 */
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
 * The RtlAddVectoredExceptionHandler routine registers a vectored exception handler.
 *
 * \param First If this parameter is TRUE, the handler is the first handler in the list.
 * \param Handler A pointer to the vectored exception handler to be called.
 * \return A handle to the vectored exception handler.
 * \see https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-addvectoredexceptionhandler
 */
NTSYSAPI
PVOID
NTAPI
RtlAddVectoredExceptionHandler(
    _In_ ULONG First,
    _In_ PVECTORED_EXCEPTION_HANDLER Handler
    );

/**
 * The RtlRemoveVectoredExceptionHandler routine removes a vectored exception handler.
 *
 * \param Handle A handle to the vectored exception handler to remove.
 * \return The function returns 0 if the handler is removed, or -1 if the handler is not found.
 * \see https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-removevectoredexceptionhandler
 */
NTSYSAPI
ULONG
NTAPI
RtlRemoveVectoredExceptionHandler(
    _In_ PVOID Handle
    );

/**
 * The RtlAddVectoredContinueHandler routine registers a vectored continue handler.
 *
 * \param First If this parameter is TRUE, the handler is the first handler in the list.
 * \param Handler A pointer to the vectored exception handler to be called.
 * \return A handle to the vectored continue handler.
 * \see https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-addvectoredcontinuehandler
 */
NTSYSAPI
PVOID
NTAPI
RtlAddVectoredContinueHandler(
    _In_ ULONG First,
    _In_ PVECTORED_EXCEPTION_HANDLER Handler
    );

/**
 * The RtlRemoveVectoredContinueHandler routine removes a vectored continue handler.
 *
 * \param Handle A handle to the vectored continue handler to remove.
 * \return The function returns 0 if the handler is removed, or -1 if the handler is not found.
 * \see https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-removevectoredcontinuehandler
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
LONG NTAPI RTLP_UNHANDLED_EXCEPTION_FILTER(
    _In_ PEXCEPTION_POINTERS ExceptionInfo
    );
/**
 * Pointer to an RTLP_UNHANDLED_EXCEPTION_FILTER callback.
 */
typedef RTLP_UNHANDLED_EXCEPTION_FILTER* PRTLP_UNHANDLED_EXCEPTION_FILTER;

/**
 * The RtlSetUnhandledExceptionFilter routine registers a top-level unhandled exception filter for the process.
 *
 * \param UnhandledExceptionFilter An optional pointer to the unhandled exception filter callback.
 */
NTSYSAPI
VOID
NTAPI
RtlSetUnhandledExceptionFilter(
    _In_opt_ PRTLP_UNHANDLED_EXCEPTION_FILTER UnhandledExceptionFilter
    );

// rev
/**
 * The RtlUnhandledExceptionFilter routine is the default filter that handles exceptions not handled by any other filter.
 *
 * \param ExceptionPointers A pointer to the EXCEPTION_POINTERS structure that describes the exception.
 * \return An exception disposition value, such as EXCEPTION_CONTINUE_SEARCH or EXCEPTION_EXECUTE_HANDLER.
 */
NTSYSAPI
LONG
NTAPI
RtlUnhandledExceptionFilter(
    _In_ PEXCEPTION_POINTERS ExceptionPointers
    );

// rev
/**
 * The RtlUnhandledExceptionFilter2 routine is the default filter that handles exceptions not handled by any other filter, with additional control flags.
 *
 * \param ExceptionPointers A pointer to the EXCEPTION_POINTERS structure that describes the exception.
 * \param Flags Flags that control the filtering behavior.
 * \return An exception disposition value, such as EXCEPTION_CONTINUE_SEARCH or EXCEPTION_EXECUTE_HANDLER.
 */
NTSYSAPI
LONG
NTAPI
RtlUnhandledExceptionFilter2(
    _In_ PEXCEPTION_POINTERS ExceptionPointers,
    _In_ ULONG Flags
    );

// rev
/**
 * The RtlKnownExceptionFilter routine examines an exception against the set of known exceptions and returns the appropriate disposition.
 *
 * \param ExceptionPointers A pointer to the EXCEPTION_POINTERS structure that describes the exception.
 * \return An exception disposition value, such as EXCEPTION_CONTINUE_SEARCH or EXCEPTION_EXECUTE_HANDLER.
 */
NTSYSAPI
LONG
NTAPI
RtlKnownExceptionFilter(
    _In_ PEXCEPTION_POINTERS ExceptionPointers
    );

#ifdef _WIN64

// private
/**
 * Identifies how a dynamic function table describes its function entries.
 */
typedef enum _FUNCTION_TABLE_TYPE
{
    RF_SORTED,
    RF_UNSORTED,
    RF_CALLBACK,
    RF_KERNEL_DYNAMIC
} FUNCTION_TABLE_TYPE;

// private
/**
 * Describes a dynamically registered exception and unwind function table.
 */
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
/**
 * The RtlGetFunctionTableListHead routine returns the head of the list of dynamic function tables for the current process.
 *
 * \return A pointer to the list head of the process dynamic function tables.
 */
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

/**
 * The RtlInitializeSListHead routine initializes the head of a singly linked list.
 *
 * \param ListHead A pointer to the SLIST_HEADER that represents the list head.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/nf-winnt-rtlinitializeslisthead
 */
NTSYSAPI
VOID
NTAPI
RtlInitializeSListHead(
    _Out_ PSLIST_HEADER ListHead
    );

/**
 * The RtlFirstEntrySList routine retrieves the first entry in a singly linked list.
 *
 * \param ListHead A pointer to the initialized singly linked-list header.
 * \return A pointer to the first entry in the list, or NULL if the list is empty.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/nf-winnt-rtlfirstentryslist
 */
_Must_inspect_result_
NTSYSAPI
PSLIST_ENTRY
NTAPI
RtlFirstEntrySList(
    _In_ const SLIST_HEADER *ListHead
    );

/**
 * The RtlInterlockedPopEntrySList routine removes an entry from the front of a singly linked list.
 *
 * \param ListHead A pointer to the singly linked-list header.
 * \return The removed entry, or NULL if the list was empty.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/nf-winnt-rtlinterlockedpopentryslist
 */
NTSYSAPI
PSLIST_ENTRY
NTAPI
RtlInterlockedPopEntrySList(
    _Inout_ PSLIST_HEADER ListHead
    );

/**
 * The RtlInterlockedPushEntrySList routine inserts an entry at the front of a singly linked list.
 *
 * \param ListHead A pointer to the singly linked-list header.
 * \param ListEntry A pointer to the entry to insert.
 * \return The previous first entry in the list, or NULL if the list was empty.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/nf-winnt-rtlinterlockedpushentryslist
 */
NTSYSAPI
PSLIST_ENTRY
NTAPI
RtlInterlockedPushEntrySList(
    _Inout_ PSLIST_HEADER ListHead,
    _Inout_ __drv_aliasesMem PSLIST_ENTRY ListEntry
    );

/**
 * The RtlInterlockedPushListSListEx routine inserts an entire singly linked list at the front of another singly linked list.
 *
 * \param ListHead A pointer to the destination list head.
 * \param List A pointer to the first entry in the list being inserted.
 * \param ListEnd A pointer to the last entry in the list being inserted.
 * \param Count The number of entries in the list being inserted.
 * \return The previous first entry in the destination list, or NULL if the
 * destination list was empty.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/interlockedapi/nf-interlockedapi-interlockedpushlistslistex
 */
NTSYSAPI
PSLIST_ENTRY
NTAPI
RtlInterlockedPushListSListEx(
    _Inout_ PSLIST_HEADER ListHead,
    _Inout_ __drv_aliasesMem PSLIST_ENTRY List,
    _Inout_ PSLIST_ENTRY ListEnd,
    _In_ ULONG Count
    );

/**
 * The RtlInterlockedFlushSList routine removes all entries from a singly linked list.
 *
 * \param ListHead A pointer to the singly linked-list header.
 * \return The previous first entry in the list, or NULL if the list was empty.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/nf-winnt-rtlinterlockedflushslist
 */
NTSYSAPI
PSLIST_ENTRY
NTAPI
RtlInterlockedFlushSList(
    _Inout_ PSLIST_HEADER ListHead
    );

/**
 * The RtlQueryDepthSList routine retrieves the number of entries in a singly linked list.
 *
 * \param ListHead A pointer to the singly linked-list header.
 * \return The number of entries currently present in the list.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/nf-winnt-rtlquerydepthslist
 */
NTSYSAPI
USHORT
NTAPI
RtlQueryDepthSList(
    _In_ PSLIST_HEADER ListHead
    );

//
// Activation Contexts
//

/**
 * Sentinel handle values and flags for activation contexts.
 */
#define INVALID_ACTIVATION_CONTEXT ((HANDLE)(LONG_PTR)-1)
#define ACTCTX_PROCESS_DEFAULT ((HANDLE)(LONG_PTR)0)
#define ACTCTX_EMPTY ((HANDLE)(LONG_PTR)-3)
#define ACTCTX_SYSTEM_DEFAULT ((HANDLE)(LONG_PTR)-4)
#define IS_SPECIAL_ACTCTX(x) (((((LONG_PTR)(x)) - 1) | 7) == -1)

// private
/**
 * The RtlGetActiveActivationContext routine retrieves the activation context that is currently active on the calling thread.
 *
 * \param ActivationContext A pointer to a variable that receives the active activation context.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetActiveActivationContext(
    _Out_ PACTIVATION_CONTEXT ActivationContext
    );

// private
/**
 * The RtlAddRefActivationContext routine increments the reference count of an activation context.
 *
 * \param ActivationContext A pointer to the activation context.
 */
NTSYSAPI
VOID
NTAPI
RtlAddRefActivationContext(
    _In_ PACTIVATION_CONTEXT ActivationContext
    );

// private
/**
 * The RtlReleaseActivationContext routine decrements the reference count of an activation context, releasing it when the count reaches zero.
 *
 * \param ActivationContext A pointer to the activation context.
 */
NTSYSAPI
VOID
NTAPI
RtlReleaseActivationContext(
    _In_ PACTIVATION_CONTEXT ActivationContext
    );

// private
/**
 * The RtlZombifyActivationContext routine marks an activation context as a zombie, releasing its resources while keeping the object alive.
 *
 * \param ActivationContext A pointer to the activation context.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlZombifyActivationContext(
    _In_ PACTIVATION_CONTEXT ActivationContext
    );

// private
/**
 * The RtlIsActivationContextActive routine determines whether the specified activation context is currently active.
 *
 * \param ActivationContext A pointer to the activation context.
 * \return `TRUE` if the activation context is active, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsActivationContextActive(
    _In_ PACTIVATION_CONTEXT ActivationContext
    );

// private
/**
 * The RtlActivateActivationContext routine activates an activation context on the calling thread.
 *
 * \param Flags Reserved. Must be zero.
 * \param ActivationContext A pointer to the activation context to activate.
 * \param Cookie A pointer to a variable that receives a cookie used to deactivate the context.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlActivateActivationContext(
    _Reserved_ ULONG Flags,
    _In_ PACTIVATION_CONTEXT ActivationContext,
    _Out_ PULONG_PTR Cookie
    );

/**
 * Flag for RtlActivateActivationContextEx.
 */
#define RTL_ACTIVATE_ACTIVATION_CONTEXT_EX_FLAG_RELEASE_ON_STACK_DEALLOCATION 0x00000001

// private
/**
 * The RtlActivateActivationContextEx routine activates an activation context on the specified thread.
 *
 * \param Flags Flags that control the activation.
 * \param Teb A pointer to the thread environment block (TEB) of the thread on which to activate the context.
 * \param ActivationContext A pointer to the activation context to activate.
 * \param Cookie A pointer to a variable that receives a cookie used to deactivate the context.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlActivateActivationContextEx(
    _In_ ULONG Flags,
    _In_ PTEB Teb,
    _In_ PACTIVATION_CONTEXT ActivationContext,
    _Out_ PULONG_PTR Cookie
    );

/**
 * Flag for RtlDeactivateActivationContext.
 */
#define RTL_DEACTIVATE_ACTIVATION_CONTEXT_FLAG_FORCE_EARLY_DEACTIVATION 0x00000001

// private
/**
 * The RtlDeactivateActivationContext routine deactivates an activation context previously activated on the calling thread.
 *
 * \param Flags Flags that control the deactivation.
 * \param Cookie The cookie returned when the activation context was activated.
 */
NTSYSAPI
VOID
NTAPI
RtlDeactivateActivationContext(
    _In_ ULONG Flags,
    _In_ ULONG_PTR Cookie
    );

// private
/**
 * The RtlCreateActivationContext routine creates an activation context from the supplied activation context data.
 *
 * \param Flags Reserved; must be zero.
 * \param ActivationContextData The activation context data used to build the context.
 * \param ExtraBytes An optional number of extra bytes to allocate with the context.
 * \param NotificationRoutine An optional routine that receives activation context notifications.
 * \param NotificationContext An optional context value passed to the notification routine.
 * \param ActivationContext Receives the newly created activation context.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Flags for the activation context section search routines.
 */
#define FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_ACTIVATION_CONTEXT 0x00000001
#define FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_FLAGS 0x00000002
#define FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_ASSEMBLY_METADATA 0x00000004

// private
/**
 * The RtlFindActivationContextSectionString routine searches an activation context for a string entry in the specified section.
 *
 * \param Flags Flags that control the search.
 * \param ExtensionGuid An optional pointer to the extension GUID that identifies a custom section.
 * \param SectionId The identifier of the section to search (ACTIVATION_CONTEXT_SECTION_*).
 * \param StringToFind A pointer to the string to find.
 * \param ReturnedData A pointer to a structure that receives the located section data.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlFindActivationContextSectionString(
    _In_ ULONG Flags,
    _In_opt_ PCGUID ExtensionGuid,
    _In_ ULONG SectionId, // ACTIVATION_CONTEXT_SECTION_*
    _In_ PCUNICODE_STRING StringToFind,
    _Inout_ PACTCTX_SECTION_KEYED_DATA ReturnedData
    );

// private
/**
 * The RtlFindActivationContextSectionGuid routine searches an activation context for a GUID entry in the specified section.
 *
 * \param Flags Flags that control the search.
 * \param ExtensionGuid An optional pointer to the extension GUID that identifies a custom section.
 * \param SectionId The identifier of the section to search (ACTIVATION_CONTEXT_SECTION_*).
 * \param GuidToFind A pointer to the GUID to find.
 * \param ReturnedData A pointer to a structure that receives the located section data.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlFindActivationContextSectionGuid(
    _In_ ULONG Flags,
    _In_opt_ PCGUID ExtensionGuid,
    _In_ ULONG SectionId, // ACTIVATION_CONTEXT_SECTION_*
    _In_ PCGUID GuidToFind,
    _Inout_ PACTCTX_SECTION_KEYED_DATA ReturnedData
    );

// rev
/**
 * The RtlQueryActivationContextApplicationSettings routine queries an application setting from the specified activation context.
 *
 * \param Flags Reserved. Must be zero.
 * \param ActivationContext A pointer to the activation context to query.
 * \param SettingsNameSpace An optional pointer to the settings namespace URI.
 * \param SettingName A pointer to the name of the setting to query.
 * \param Buffer A buffer that receives the setting value.
 * \param BufferLength The size, in bytes, of the buffer.
 * \param RequiredLength An optional pointer to a variable that receives the required buffer size.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Flags for RtlQueryInformationActivationContext.
 */
#define RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_USE_ACTIVE_ACTIVATION_CONTEXT 0x00000001
#define RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_ACTIVATION_CONTEXT_IS_MODULE 0x00000002
#define RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_ACTIVATION_CONTEXT_IS_ADDRESS 0x00000004
#define RTL_QUERY_INFORMATION_ACTIVATION_CONTEXT_FLAG_NO_ADDREF 0x80000000

// private
/**
 * The RtlQueryInformationActivationContext routine retrieves information about an activation context.
 *
 * \param Flags Flags controlling the query (for example, whether ActivationContext is a handle).
 * \param ActivationContext An optional activation context to query; NULL queries the active context.
 * \param SubInstanceIndex An optional index selecting a sub-instance within the context.
 * \param ActivationContextInformationClass The class of information to retrieve.
 * \param ActivationContextInformation A buffer that receives the requested information.
 * \param ActivationContextInformationLength The size, in bytes, of the buffer.
 * \param ReturnLength An optional pointer that receives the number of bytes returned.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlQueryInformationActiveActivationContext routine retrieves information about the currently active activation context.
 *
 * \param ActivationContextInformationClass The class of information to retrieve.
 * \param ActivationContextInformation A buffer that receives the requested information.
 * \param ActivationContextInformationLength The size, in bytes, of the buffer.
 * \param ReturnLength An optional pointer that receives the number of bytes returned.
 * \return NTSTATUS Successful or errant status.
 */
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
// Loader (Ldr)
//

/**
 * The LdrHotPatchNotify routine notifies the loader that a hot-patch image has been applied so that dependent loader state can be updated.
 *
 * \param ImageBase The base address of the image receiving the hot patch.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
LdrHotPatchNotify(
    _In_ PVOID ImageBase
    );

/**
 * The LdrInitShimEngineDynamic routine dynamically initializes the application-compatibility shim engine for the specified image.
 *
 * \param ImageBase The base address of the shim engine image.
 * \param ShimData Opaque shim data describing the shims to apply.
 * \return BOOL TRUE if successful, FALSE otherwise.
 */
// rev
NTSYSAPI
BOOL
NTAPI
LdrInitShimEngineDynamic(
    _In_ PVOID ImageBase,
    _In_opt_ PVOID ShimData
    );

/**
 * The LdrRscIsTypeExist routine determines whether a resource of the specified type exists within a resource enumeration context.
 *
 * \param RscContext A pointer to the resource enumeration context.
 * \param Type The resource type name to test for.
 * \param Reserved Reserved; must be NULL.
 * \param Flags On input specifies matching flags; on output receives result flags.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
LdrRscIsTypeExist(
    _Inout_ PULONG RscContext,
    _In_z_ PCWSTR Type,
    _Reserved_ PVOID Reserved,
    _Inout_ PULONG Flags
    );

/**
 * The LdrSetAppCompatDllRedirectionCallback routine registers a callback used to redirect application-compatibility DLL loads.
 *
 * \param Callback The redirection callback routine to register.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrSetAppCompatDllRedirectionCallback(
    _In_ PVOID Callback
    );

// rev
/**
 * The LdrSetMUICacheType routine sets the MUI (Multilingual User Interface) resource cache type for the current process.
 *
 * \param MuiCacheType The MUI cache type to set.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrSetMUICacheType(
    _In_ ULONG MuiCacheType
    );

//
// Images
//

/**
 * The RtlImageNtHeader routine returns a pointer to the IMAGE_NT_HEADERS of a mapped image.
 *
 * \param BaseOfImage The base address of the mapped image.
 * \return A pointer to the image NT headers, or NULL if the image is invalid.
 */
NTSYSAPI
PIMAGE_NT_HEADERS
NTAPI
RtlImageNtHeader(
    _In_ PVOID BaseOfImage
    );

/**
 * Flag to disable range checking in RtlImageNtHeaderEx.
 */
#define RTL_IMAGE_NT_HEADER_EX_FLAG_NO_RANGE_CHECK 0x00000001

/**
 * The RtlImageNtHeaderEx routine returns a pointer to the IMAGE_NT_HEADERS of a mapped image, with validation options.
 *
 * \param Flags Flags controlling validation (for example, RTL_IMAGE_NT_HEADER_EX_FLAG_NO_RANGE_CHECK).
 * \param BaseOfImage The base address of the mapped image.
 * \param Size The size, in bytes, of the mapped image.
 * \param OutHeaders Receives a pointer to the image NT headers.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlImageNtHeaderEx(
    _In_ ULONG Flags,
    _In_ PVOID BaseOfImage,
    _In_ ULONG64 Size,
    _Out_ PIMAGE_NT_HEADERS *OutHeaders
    );

/**
 * The RtlAddressInSectionTable routine returns the address corresponding to a relative virtual address (RVA) using the image section table.
 *
 * \param NtHeaders The NT headers of the image.
 * \param BaseOfImage The base address of the mapped image.
 * \param VirtualAddress The relative virtual address to translate.
 * \return A pointer to the data at the RVA, or NULL if the RVA is not within any section.
 */
NTSYSAPI
PVOID
NTAPI
RtlAddressInSectionTable(
    _In_ PIMAGE_NT_HEADERS NtHeaders,
    _In_ PVOID BaseOfImage,
    _In_ ULONG VirtualAddress
    );

/**
 * The RtlSectionTableFromVirtualAddress routine returns the section header that contains the specified relative virtual address (RVA).
 *
 * \param NtHeaders The NT headers of the image.
 * \param BaseOfImage The base address of the mapped image.
 * \param VirtualAddress The relative virtual address to locate.
 * \return A pointer to the containing section header, or NULL if none contains the RVA.
 */
NTSYSAPI
PIMAGE_SECTION_HEADER
NTAPI
RtlSectionTableFromVirtualAddress(
    _In_ PIMAGE_NT_HEADERS NtHeaders,
    _In_ PVOID BaseOfImage,
    _In_ ULONG VirtualAddress
    );

/**
 * The RtlImageDirectoryEntryToData routine returns a pointer to the data of the specified image data directory.
 *
 * \param BaseOfImage The base address of the image.
 * \param MappedAsImage TRUE if the image is mapped as an image; FALSE if mapped as a flat file.
 * \param DirectoryEntry The data directory entry index (IMAGE_DIRECTORY_ENTRY_*).
 * \param Size Receives the size, in bytes, of the directory data.
 * \return A pointer to the directory data, or NULL if the directory is absent.
 */
NTSYSAPI
PVOID
NTAPI
RtlImageDirectoryEntryToData(
    _In_ PVOID BaseOfImage,
    _In_ BOOLEAN MappedAsImage,
    _In_ USHORT DirectoryEntry,
    _Out_ PULONG Size
    );

/**
 * The RtlImageRvaToSection routine returns the section header that contains the specified relative virtual address (RVA).
 *
 * \param NtHeaders The NT headers of the image.
 * \param BaseOfImage The base address of the mapped image.
 * \param Rva The relative virtual address to locate.
 * \return A pointer to the containing section header, or NULL if none contains the RVA.
 */
NTSYSAPI
PIMAGE_SECTION_HEADER
NTAPI
RtlImageRvaToSection(
    _In_ PIMAGE_NT_HEADERS NtHeaders,
    _In_ PVOID BaseOfImage,
    _In_ ULONG Rva
    );

/**
 * The RtlImageRvaToVa routine translates a relative virtual address (RVA) to a virtual address within a mapped image.
 *
 * \param NtHeaders The NT headers of the image.
 * \param BaseOfImage The base address of the mapped image.
 * \param Rva The relative virtual address to translate.
 * \param LastRvaSection An optional pointer to a cached section header used and updated across calls.
 * \return A pointer to the data at the RVA, or NULL if the RVA is not within any section.
 */
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

/**
 * The RtlFindExportedRoutineByName routine locates an exported routine within a mapped image by its export name.
 *
 * \param BaseOfImage The base address of the mapped image to search.
 * \param RoutineName The name of the exported routine to find.
 * \return A pointer to the exported routine, or NULL if the export was not found.
 */
// rev
NTSYSAPI
PVOID
NTAPI
RtlFindExportedRoutineByName(
    _In_ PVOID BaseOfImage,
    _In_z_ PCSTR RoutineName
    );

/**
 * The RtlGuardCheckLongJumpTarget routine determines whether the specified address is a valid Control Flow Guard long-jump target.
 *
 * \param PcValue The target instruction address to validate.
 * \param IsFastFail Specifies whether a failed validation should fast-fail.
 * \param IsLongJumpTarget Receives TRUE if the address is a valid long-jump target.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlValidateUserCallTarget routine validates whether an address is a valid Control Flow Guard indirect call target.
 *
 * \param Address The target address to validate.
 * \param Flags Receives flags describing the validation result.
 */
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

/**
 * The RtlCompareMemory routine compares two blocks of memory and returns the number of leading bytes that are equal.
 *
 * \param Source1 A pointer to the first block of memory to compare.
 * \param Source2 A pointer to the second block of memory to compare.
 * \param Length The number of bytes to compare.
 * \return SIZE_T The number of bytes that are equal in the two memory blocks.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlcomparememory
 */
_Check_return_
NTSYSAPI
SIZE_T
NTAPI
RtlCompareMemory(
    _In_ const VOID* Source1,
    _In_ const VOID* Source2,
    _In_ SIZE_T Length
    );

/**
 * The RtlCompareMemoryUlong routine compares a block of memory against a repeating ULONG pattern.
 *
 * \param Source The memory to compare.
 * \param Length The number of bytes to compare.
 * \param Pattern The ULONG pattern to compare against.
 * \return The number of leading bytes that match the pattern.
 */
_Must_inspect_result_
NTSYSAPI
SIZE_T
NTAPI
RtlCompareMemoryUlong(
    _In_reads_bytes_(Length) PVOID Source,
    _In_ SIZE_T Length,
    _In_ ULONG Pattern
    );

/**
 * The RtlCopyMappedMemory routine copies memory, tolerating faults that may occur when reading mapped memory.
 *
 * \param Destination The destination buffer.
 * \param Source The source memory to copy from.
 * \param Length The number of bytes to copy.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCopyMappedMemory(
    _Out_writes_bytes_all_(Length) PVOID Destination,
    _In_reads_bytes_(Length) PVOID Source,
    _In_ SIZE_T Length
    );

#if defined(_M_AMD64) || defined(_M_ARM64)
/**
 * The RtlCopyMemoryNonTemporal routine copies the contents of one memory block to another using non-temporal store operations that bypass the CPU cache.
 *
 * \param Destination A pointer to the destination memory block.
 * \param Source A pointer to the source memory block.
 * \param Length The number of bytes to copy.
 */
NTSYSAPI
VOID
NTAPI
RtlCopyMemoryNonTemporal(
   _Out_writes_bytes_all_(Length) VOID UNALIGNED *Destination,
   _In_reads_bytes_(Length) CONST VOID UNALIGNED *Source,
   _In_ SIZE_T Length
   );

/**
 * The RtlFillMemoryNonTemporal routine fills a block of memory with the specified byte value using non-temporal store operations that bypass the CPU cache.
 *
 * \param Destination A pointer to the memory block to fill.
 * \param Length The number of bytes to fill.
 * \param Value The byte value to write to each byte of the memory block.
 */
NTSYSAPI
VOID
NTAPI
RtlFillMemoryNonTemporal(
   _Out_writes_bytes_all_(Length) VOID UNALIGNED *Destination,
   _In_ SIZE_T Length,
   _In_ CONST UCHAR Value
   );
#else
#define RtlCopyMemoryNonTemporal RtlCopyMemory
#define RtlFillMemoryNonTemporal RtlFillMemory
#endif

/**
 * The RtlFillMemoryUlong routine fills a block of memory with a repeating ULONG pattern.
 *
 * \param Destination The buffer to fill.
 * \param Length The number of bytes to fill.
 * \param Pattern The ULONG pattern written across the buffer.
 */
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

/**
 * The RtlFillMemoryUlonglong routine fills a block of memory with a repeating ULONGLONG pattern.
 *
 * \param Destination The buffer to fill.
 * \param Length The number of bytes to fill.
 * \param Pattern The ULONGLONG pattern written across the buffer.
 */
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

/**
 * The RtlIsZeroMemory routine determines whether a block of memory contains only zero bytes.
 *
 * \param Buffer The buffer to examine.
 * \param Length The number of bytes to examine.
 * \return TRUE if all examined bytes are zero; otherwise, FALSE.
 */
#if (PHNT_VERSION >= PHNT_WINDOWS_10_19H2)
NTSYSAPI
BOOLEAN
NTAPI
RtlIsZeroMemory(
    _In_ PVOID Buffer,
    _In_ SIZE_T Length
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_19H2

FORCEINLINE
BOOLEAN 
NTAPI 
RtlIsZeroMemory(
    _In_ PVOID Buffer,
    _In_ SIZE_T Length
    )
{
    PCHAR buffer = (PCHAR)Buffer;

    while (((ULONG_PTR)buffer & 7) != 0 && Length != 0)
    {
        if (*buffer != 0)
            return FALSE;

        buffer++;
        Length--;
    }

    while (Length >= sizeof(ULONG64))
    {
        if (*(PULONG64)buffer != 0)
            return FALSE;

        buffer += sizeof(ULONG64);
        Length -= sizeof(ULONG64);
    }

    while (Length != 0)
    {
        if (*buffer != 0)
            return FALSE;

        buffer++;
        Length--;
    }

    return TRUE;
}

/**
 * The RtlCrc32 routine computes the CRC-32 checksum of a block of memory.
 *
 * \param Buffer A pointer to the buffer whose checksum is computed.
 * \param Size The size, in bytes, of the buffer.
 * \param InitialCrc The initial CRC value used to seed the computation.
 * \return ULONG The computed CRC-32 checksum.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlcrc32
 */
NTSYSAPI
ULONG
NTAPI
RtlCrc32(
    _In_reads_bytes_(Size) const VOID* Buffer,
    _In_ size_t Size,
    _In_ ULONG InitialCrc
    );

/**
 * The RtlCrc64 routine computes the CRC-64 checksum of a block of memory.
 *
 * \param Buffer A pointer to the buffer whose checksum is computed.
 * \param Size The size, in bytes, of the buffer.
 * \param InitialCrc The initial CRC value used to seed the computation.
 * \return ULONGLONG The computed CRC-64 checksum.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlcrc64
 */
NTSYSAPI
ULONGLONG
NTAPI
RtlCrc64(
    _In_reads_bytes_(Size) const VOID *Buffer,
    _In_ size_t Size,
    _In_ ULONGLONG InitialCrc
    );

// RTL_SYSTEM_GLOBAL_DATA_ID
/**
 * Identifiers for fields retrievable via RtlGetSystemGlobalData (RTL_SYSTEM_GLOBAL_DATA_ID).
 */
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
/**
 * Identifies a field in the shared user data region retrieved by RtlGetSystemGlobalData.
 */
typedef ULONG RTL_SYSTEM_GLOBAL_DATA_ID;
#endif // !defined(NTDDI_WIN10_FE) || (NTDDI_VERSION < NTDDI_WIN10_FE)

/**
 * The RtlGetSystemGlobalData routine retrieves a system global data value.
 *
 * \param DataId The identifier of the system global data to retrieve.
 * \param Buffer A buffer that receives the data.
 * \param Size The size, in bytes, of the buffer.
 * \return The number of bytes returned, or zero on failure.
 */
NTSYSAPI
ULONG
NTAPI
RtlGetSystemGlobalData(
    _In_ RTL_SYSTEM_GLOBAL_DATA_ID DataId,
    _Inout_ PVOID Buffer,
    _In_ ULONG Size
    );

/**
 * The RtlSetSystemGlobalData routine sets a system global data value.
 *
 * \param DataId The identifier of the system global data to set.
 * \param Buffer A buffer containing the data to set.
 * \param Size The size, in bytes, of the buffer.
 * \return The number of bytes written, or zero on failure.
 */
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

/**
 * The RtlCreateEnvironment routine creates a new environment block, optionally cloning the current process environment.
 *
 * \param CloneCurrentEnvironment TRUE to clone the current environment; FALSE to create an empty one.
 * \param Environment Receives the newly created environment block.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateEnvironment(
    _In_ BOOLEAN CloneCurrentEnvironment,
    _Out_ PVOID *Environment
    );

// begin_rev
/**
 * Flags for RtlCreateEnvironmentEx.
 */
#define RTL_CREATE_ENVIRONMENT_TRANSLATE 0x1 // translate from multi-byte to Unicode
#define RTL_CREATE_ENVIRONMENT_TRANSLATE_FROM_OEM 0x2 // translate from OEM to Unicode (Translate flag must also be set)
#define RTL_CREATE_ENVIRONMENT_EMPTY 0x4 // create empty environment block
// end_rev

// private
/**
 * The RtlCreateEnvironmentEx routine creates a new environment block from an optional source environment.
 *
 * \param SourceEnvironment An optional source environment block to copy from.
 * \param Environment Receives the newly created environment block.
 * \param Flags Flags controlling creation.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateEnvironmentEx(
    _In_opt_ PVOID SourceEnvironment,
    _Out_ PVOID *Environment,
    _In_ ULONG Flags
    );

/**
 * The RtlDestroyEnvironment routine destroys an environment block created by RtlCreateEnvironment or RtlCreateEnvironmentEx.
 *
 * \param Environment The environment block to destroy.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyEnvironment(
    _In_ _Post_invalid_ PVOID Environment
    );

/**
 * The RtlSetCurrentEnvironment routine sets the environment block for the current process.
 *
 * \param Environment The environment block to make current.
 * \param PreviousEnvironment An optional pointer that receives the previous environment block.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetCurrentEnvironment(
    _In_ PVOID Environment,
    _Out_opt_ PVOID *PreviousEnvironment
    );

// private
/**
 * The RtlSetEnvironmentVar routine sets or removes an environment variable within an environment block, using counted strings.
 *
 * \param Environment An optional pointer to the environment block to modify; NULL uses the current environment.
 * \param Name The name of the environment variable.
 * \param NameLength The length, in characters, of Name.
 * \param Value The value to set, or NULL to remove the variable.
 * \param ValueLength The length, in characters, of Value.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlSetEnvironmentVariable routine sets or removes an environment variable within an environment block.
 *
 * \param Environment An optional pointer to the environment block to modify; NULL uses the current environment.
 * \param Name The name of the environment variable.
 * \param Value The value to set, or NULL to remove the variable.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetEnvironmentVariable(
    _Inout_opt_ PVOID *Environment,
    _In_ PCUNICODE_STRING Name,
    _In_opt_ PCUNICODE_STRING Value
    );

// private
/**
 * The RtlQueryEnvironmentVariable routine retrieves the value of an environment variable, using counted strings.
 *
 * \param Environment An optional environment block to query; NULL uses the current environment.
 * \param Name The name of the environment variable.
 * \param NameLength The length, in characters, of Name.
 * \param Value An optional buffer that receives the value.
 * \param ValueLength The length, in characters, of the Value buffer.
 * \param ReturnLength Receives the length, in characters, of the value.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlQueryEnvironmentVariable_U routine retrieves the value of an environment variable.
 *
 * \param Environment An optional environment block to query; NULL uses the current environment.
 * \param Name The name of the environment variable.
 * \param Value Receives the value of the environment variable.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryEnvironmentVariable_U(
    _In_opt_ PVOID Environment,
    _In_ PCUNICODE_STRING Name,
    _Inout_ PUNICODE_STRING Value
    );

// private
/**
 * The RtlExpandEnvironmentStrings routine expands environment-variable references within a string, using counted strings.
 *
 * \param Environment An optional environment block used for expansion; NULL uses the current environment.
 * \param Source The source string containing references to expand.
 * \param SourceLength The length, in characters, of Source.
 * \param Destination A buffer that receives the expanded string.
 * \param DestinationLength The length, in characters, of the Destination buffer.
 * \param ReturnLength An optional pointer that receives the length, in characters, of the expanded string.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlExpandEnvironmentStrings_U routine expands environment-variable references within a string.
 *
 * \param Environment An optional environment block used for expansion; NULL uses the current environment.
 * \param Source The source string containing references to expand.
 * \param Destination Receives the expanded string.
 * \param ReturnedLength An optional pointer that receives the length, in bytes, of the expanded string.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlExpandEnvironmentStrings_U(
    _In_opt_ PVOID Environment,
    _In_ PCUNICODE_STRING Source,
    _Inout_ PUNICODE_STRING Destination,
    _Out_opt_ PULONG ReturnedLength
    );

/**
 * The RtlSetEnvironmentStrings routine replaces the environment block of the current process with the specified environment strings.
 *
 * \param NewEnvironment A pointer to the new environment block, consisting of null-terminated name=value strings terminated by an additional null character.
 * \param NewEnvironmentSize The size, in bytes, of the new environment block.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Represents a reference-counted current directory handle.
 */
typedef struct _RTLP_CURDIR_REF
{
    LONG ReferenceCount;
    HANDLE DirectoryHandle;
} RTLP_CURDIR_REF, *PRTLP_CURDIR_REF;

/**
 * Describes a path expressed relative to a containing directory, as produced by path parsing routines.
 */
typedef struct _RTL_RELATIVE_NAME_U
{
    UNICODE_STRING RelativeName;
    HANDLE ContainingDirectory;
    PRTLP_CURDIR_REF CurDirRef;
} RTL_RELATIVE_NAME_U, *PRTL_RELATIVE_NAME_U;

/**
 * Identifies the syntactic type of a DOS path.
 */
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

/**
 * Well-known module name and path-separator string constants.
 */
#define RtlNtdllName L"ntdll.dll"
#define RtlDosPathSeperatorsString ((CONST UNICODE_STRING)RTL_CONSTANT_STRING(L"\\/"))
#define RtlAlternateDosPathSeperatorString ((CONST UNICODE_STRING)RTL_CONSTANT_STRING(L"/"))
#define RtlNtPathSeperatorString ((CONST UNICODE_STRING)RTL_CONSTANT_STRING(L"\\"))

/**
 * Well-known DOS device and NT namespace path prefix string constants.
 */
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

/**
 * The RtlGetFullPathName_UstrEx routine retrieves the full path and file name of the specified file as a counted Unicode string.
 *
 * \param FileName A pointer to the file name to resolve.
 * \param StaticString A caller-supplied buffer that receives the full path if it is large enough.
 * \param DynamicString An optional buffer that receives an allocated string when the static buffer is too small.
 * \param StringUsed An optional pointer to a variable that receives which of the static or dynamic strings holds the result.
 * \param FilePartPrefixCch An optional pointer to a variable that receives the length, in characters, of the path preceding the file name.
 * \param NameInvalid An optional pointer to a variable that receives whether the name is invalid.
 * \param InputPathType A pointer to a variable that receives the RTL_PATH_TYPE of the input path.
 * \param BytesRequired An optional pointer to a variable that receives the number of bytes required for the full path.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlGetLongestNtPathLength routine returns the length, in characters, of the longest possible NT path name.
 *
 * \return ULONG The maximum NT path length, in characters.
 */
NTSYSAPI
ULONG
NTAPI
RtlGetLongestNtPathLength(
    VOID
    );

// rev
/**
 * Represents a growable memory buffer with a fixed-size static portion.
 */
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
/**
 * Represents a Unicode string backed by a growable RTL_BUFFER.
 */
typedef struct _RTL_UNICODE_STRING_BUFFER
{
    UNICODE_STRING String;
    RTL_BUFFER ByteBuffer;
    UCHAR MinimumStaticBufferForTerminalNul[2];
} RTL_UNICODE_STRING_BUFFER, *PRTL_UNICODE_STRING_BUFFER;

// rev
/**
 * The RtlNtPathNameToDosPathName routine converts an NT path name to its equivalent DOS path name.
 *
 * \param Flags Reserved. This parameter must be zero.
 * \param Path A pointer to a path buffer that contains the NT path on input and receives the DOS path on output.
 * \param Disposition An optional pointer to a variable that receives the DOS path type (as returned by RtlDetermineDosPathNameType_U).
 * \param FilePart An optional pointer to a variable that receives a pointer to the file-name portion of the resulting path.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlNtPathNameToDosPathName(
    _Reserved_ ULONG Flags,
    _Inout_ PRTL_UNICODE_STRING_BUFFER Path,
    _Out_opt_ PULONG Disposition, // RtlDetermineDosPathNameType_U
    _Inout_opt_ PWSTR* FilePart
    );

/**
 * The RtlDosPathNameToNtPathName_U routine converts a DOS path name to its equivalent NT path name.
 *
 * \param DosFileName A pointer to the DOS path name to convert.
 * \param NtFileName A pointer to a UNICODE_STRING that receives the equivalent NT path name.
 * \param FilePart An optional pointer to a variable that receives a pointer to the file-name portion of the NT path.
 * \param RelativeName An optional pointer to a structure that receives relative-name information.
 * \return `TRUE` if the conversion succeeded, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlDosPathNameToNtPathName_U(
    _In_ PCWSTR DosFileName,
    _Out_ PUNICODE_STRING NtFileName,
    _Out_opt_ PWSTR *FilePart,
    _Out_opt_ PRTL_RELATIVE_NAME_U RelativeName
    );

/**
 * The RtlDosPathNameToNtPathName_U_WithStatus routine converts a DOS path name to its equivalent NT path name and returns a status code.
 *
 * \param DosFileName A pointer to the DOS path name to convert.
 * \param NtFileName A pointer to a UNICODE_STRING that receives the equivalent NT path name.
 * \param FilePart An optional pointer to a variable that receives a pointer to the file-name portion of the NT path.
 * \param RelativeName An optional pointer to a structure that receives relative-name information.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlDosLongPathNameToNtPathName_U_WithStatus routine converts a DOS long path name to its equivalent NT path name and returns a status code.
 *
 * \param DosFileName A pointer to the DOS long path name to convert.
 * \param NtFileName A pointer to a UNICODE_STRING that receives the equivalent NT path name.
 * \param FilePart An optional pointer to a variable that receives a pointer to the file-name portion of the NT path.
 * \param RelativeName An optional pointer to a structure that receives relative-name information.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlDosPathNameToRelativeNtPathName_U routine converts a DOS path name to its equivalent NT path name, including relative-name information.
 *
 * \param DosFileName A pointer to the DOS path name to convert.
 * \param NtFileName A pointer to a UNICODE_STRING that receives the equivalent NT path name.
 * \param FilePart An optional pointer to a variable that receives a pointer to the file-name portion of the NT path.
 * \param RelativeName An optional pointer to a structure that receives relative-name information.
 * \return `TRUE` if the conversion succeeded, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlDosPathNameToRelativeNtPathName_U(
    _In_ PCWSTR DosFileName,
    _Out_ PUNICODE_STRING NtFileName,
    _Out_opt_ PWSTR *FilePart,
    _Out_opt_ PRTL_RELATIVE_NAME_U RelativeName
    );

/**
 * The RtlDosPathNameToRelativeNtPathName_U_WithStatus routine converts a DOS path name to its equivalent NT path name, including relative-name information, and returns a status code.
 *
 * \param DosFileName A pointer to the DOS path name to convert.
 * \param NtFileName A pointer to a UNICODE_STRING that receives the equivalent NT path name.
 * \param FilePart An optional pointer to a variable that receives a pointer to the file-name portion of the NT path.
 * \param RelativeName An optional pointer to a structure that receives relative-name information.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlDosLongPathNameToRelativeNtPathName_U_WithStatus routine converts a DOS long path name to its equivalent NT path name, including relative-name information, and returns a status code.
 *
 * \param DosFileName A pointer to the DOS long path name to convert.
 * \param NtFileName A pointer to a UNICODE_STRING that receives the equivalent NT path name.
 * \param FilePart An optional pointer to a variable that receives a pointer to the file-name portion of the NT path.
 * \param RelativeName An optional pointer to a structure that receives relative-name information.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlReleaseRelativeName routine releases the resources associated with a relative-name structure previously returned by one of the RtlDosPathNameToRelativeNtPathName routines.
 *
 * \param RelativeName A pointer to the relative-name structure to release.
 */
NTSYSAPI
VOID
NTAPI
RtlReleaseRelativeName(
    _Inout_ PRTL_RELATIVE_NAME_U RelativeName
    );

/**
 * The RtlDosSearchPath_U routine searches a set of directories for the specified file.
 *
 * \param Path A pointer to the search path, consisting of one or more directories separated by semicolons.
 * \param FileName A pointer to the name of the file to locate.
 * \param Extension An optional default extension to append when the file name has none.
 * \param BufferLength The size, in bytes, of the output buffer.
 * \param Buffer A buffer that receives the full path of the located file.
 * \param FilePart An optional pointer to a variable that receives a pointer to the file-name portion of the result.
 * \return ULONG The length, in bytes, of the string copied to Buffer, or the required size if the buffer is too small.
 */
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

/**
 * Flags for the RTL DOS path search routines.
 */
#define RTL_DOS_SEARCH_PATH_FLAG_APPLY_ISOLATION_REDIRECTION 0x00000001
#define RTL_DOS_SEARCH_PATH_FLAG_DISALLOW_DOT_RELATIVE_PATH_SEARCH 0x00000002
#define RTL_DOS_SEARCH_PATH_FLAG_APPLY_DEFAULT_EXTENSION_WHEN_NOT_RELATIVE_PATH_EVEN_IF_FILE_HAS_EXTENSION 0x00000004

/**
 * The RtlDosSearchPath_Ustr routine searches a set of directories for the specified file and returns the result as a counted Unicode string.
 *
 * \param Flags Flags that control the search behavior.
 * \param Path A pointer to the search path, consisting of one or more directories separated by semicolons.
 * \param FileName A pointer to the name of the file to locate.
 * \param DefaultExtension An optional default extension to append when the file name has none.
 * \param StaticString A caller-supplied buffer that receives the result if it is large enough.
 * \param DynamicString An optional buffer that receives an allocated string when the static buffer is too small.
 * \param FullFileNameOut An optional pointer to a variable that receives a pointer to the full file name.
 * \param FilePartPrefixCch An optional pointer to a variable that receives the length, in characters, of the path preceding the file name.
 * \param BytesRequired An optional pointer to a variable that receives the number of bytes required for the full path.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlDoesFileExists_U routine determines whether the specified file exists.
 *
 * \param FileName A pointer to the name of the file to test.
 * \return `TRUE` if the file exists, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlDoesFileExists_U(
    _In_ PCWSTR FileName
    );

// ros
/**
 * The RtlDosApplyFileIsolationRedirection_Ustr routine applies side-by-side (SxS) isolation redirection to the specified file name.
 *
 * \param Flags Flags that control the redirection behavior.
 * \param OriginalName A pointer to the original file name to redirect.
 * \param Extension A pointer to the default extension to apply when the name has none.
 * \param StaticString An optional caller-supplied buffer that receives the redirected name if it is large enough.
 * \param DynamicString An optional buffer that receives an allocated string when the static buffer is too small.
 * \param NewName An optional pointer to a variable that receives a pointer to the redirected name.
 * \param NewFlags A pointer to a variable that receives flags describing the redirection result.
 * \param FileNameSize A pointer to a variable that receives the size, in bytes, of the redirected name.
 * \param RequiredLength A pointer to a variable that receives the number of bytes required for the redirected name.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlGetLengthWithoutLastFullDosOrNtPathElement routine computes the length of a path string excluding its last full DOS or NT path element.
 *
 * \param Flags Reserved. This parameter must be zero.
 * \param PathString A pointer to the path string to examine.
 * \param Length A pointer to a variable that receives the length, in characters, of the path without its last element.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetLengthWithoutLastFullDosOrNtPathElement(
    _Reserved_ ULONG Flags,
    _In_ PCUNICODE_STRING PathString,
    _Out_ PULONG Length
    );

/**
 * The RtlGetLengthWithoutTrailingPathSeperators routine computes the length of a path string excluding any trailing path separators.
 *
 * \param Flags Reserved. This parameter must be zero.
 * \param PathString A pointer to the path string to examine.
 * \param Length A pointer to a variable that receives the length, in characters, of the path without trailing separators.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetLengthWithoutTrailingPathSeperators(
    _Reserved_ ULONG Flags,
    _In_ PCUNICODE_STRING PathString,
    _Out_ PULONG Length
    );

/**
 * Maintains state used when generating short (8.3) file names from long names.
 */
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
/**
 * The RtlGenerate8dot3Name routine generates a short (8.3) file name from the specified long file name.
 *
 * \param Name A pointer to the long file name from which the short name is generated.
 * \param AllowExtendedCharacters A boolean that specifies whether extended characters are permitted in the generated name.
 * \param Context A pointer to a context structure that maintains state across successive generation attempts.
 * \param Name8dot3 A pointer to a UNICODE_STRING that receives the generated 8.3 name.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlgenerate8dot3name
 */
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
/**
 * The RtlComputePrivatizedDllName_U routine computes the privatized (side-by-side) DLL file names for the specified DLL.
 *
 * \param DllName A pointer to the name of the DLL to privatize.
 * \param RealName A pointer to a UNICODE_STRING that receives the fully qualified real DLL name.
 * \param LocalName A pointer to a UNICODE_STRING that receives the local (privatized) DLL name.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlComputePrivatizedDllName_U(
    _In_ PCUNICODE_STRING DllName,
    _Out_ PUNICODE_STRING RealName,
    _Out_ PUNICODE_STRING LocalName
    );

// rev
/**
 * The RtlGetSearchPath routine retrieves the search path used to locate images and files for the current process.
 *
 * \param Path A pointer to a variable that receives an allocated null-terminated search path string. The caller frees the string with RtlReleasePath.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetSearchPath(
    _Out_ PCWSTR* Path // RtlReleasePath
    );

// rev
/**
 * The RtlSetSearchPathMode routine sets the search path mode used when locating images and files for the current process.
 *
 * \param Flags Flags that specify the search path mode.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetSearchPathMode(
    _In_ ULONG Flags
    );

// rev
/**
 * The RtlGetExePath routine retrieves the executable search path for the specified application.
 *
 * \param DosPathName A pointer to the DOS path name of the application.
 * \param Path A pointer to a variable that receives an allocated null-terminated search path string. The caller frees the string with RtlReleasePath.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetExePath(
    _In_ PCWSTR DosPathName,
    _Out_ PCWSTR* Path
    );

// rev
/**
 * The RtlReleasePath routine frees a path buffer previously allocated by the RtlGetSearchPath or RtlGetExePath routines.
 *
 * \param Path A pointer to the path buffer to free.
 */
NTSYSAPI
VOID
NTAPI
RtlReleasePath(
    _In_ PCWSTR Path
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_8

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
// rev
/**
 * The RtlReplaceSystemDirectoryInPath routine replaces the system directory in a path with the directory appropriate for the specified machine architecture.
 *
 * \param Destination A pointer to a UNICODE_STRING that contains the path on input and receives the modified path on output.
 * \param Machine The source machine architecture (an IMAGE_FILE_MACHINE_* value).
 * \param TargetMachine The target machine architecture (an IMAGE_FILE_MACHINE_* value).
 * \param IncludePathSeperator A boolean that specifies whether a trailing path separator is included.
 * \return ULONG The length, in bytes, of the resulting path.
 */
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
/**
 * The RtlWow64GetCurrentMachine routine returns the machine architecture of the current process image.
 *
 * \return USHORT An IMAGE_FILE_MACHINE_* value identifying the current machine.
 */
NTSYSAPI
USHORT
NTAPI
RtlWow64GetCurrentMachine(
    VOID
    );

// rev from Wow64DetermineEnvironment
/**
 * The RtlWow64IsWowGuestMachineSupported routine determines whether the specified guest machine architecture is supported for WOW64 emulation on the current host.
 *
 * \param NativeMachine The native (host) machine architecture (an IMAGE_FILE_MACHINE_* value).
 * \param IsWowGuestMachineSupported A pointer to a variable that receives whether the guest machine is supported.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlWow64GetProcessMachines routine retrieves the process and native machine architectures for the specified process.
 *
 * \param ProcessHandle A handle to the process to query.
 * \param ProcessMachine A pointer to a variable that receives the process machine architecture (an IMAGE_FILE_MACHINE_* value).
 * \param NativeMachine An optional pointer to a variable that receives the native machine architecture.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * Native machine architecture identifiers for the running platform.
 */
#define IMAGE_FILE_NATIVE_MACHINE_I386  0x1
#define IMAGE_FILE_NATIVE_MACHINE_AMD64 0x2
#define IMAGE_FILE_NATIVE_MACHINE_ARMNT 0x4
#define IMAGE_FILE_NATIVE_MACHINE_ARM64 0x8
#define IMAGE_FILE_NATIVE_MACHINE_ARM64EC 0x10

#if !defined(NTDDI_WIN11_BR) || (NTDDI_VERSION < NTDDI_WIN11_BR)
// private
/**
 * Represents, as a bitmask, the set of image machine architectures supported by a file.
 */
typedef struct _IMAGE_FILE_MACHINES
{
    union
    {
        ULONG Value;
        struct
        {
            ULONG MachineX86 : 1;
            ULONG MachineAmd64 : 1;
            ULONG MachineArm : 1;
            ULONG MachineArm64 : 1;
            ULONG MachineArm64EC : 1;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} IMAGE_FILE_MACHINES;

// rev
/**
 * The RtlGetImageFileMachines routine retrieves the set of machine architectures supported by the specified image file.
 *
 * \param FileName A pointer to the path of the image file to query.
 * \param MachineTypeFlags A pointer to a variable that receives the supported machine type flags.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetImageFileMachines(
    _In_ PCWSTR FileName,
    _Out_ IMAGE_FILE_MACHINES *MachineTypeFlags
    );
#endif // #if !defined(NTDDI_WIN11_BR) || (NTDDI_VERSION < NTDDI_WIN11_BR)
#endif // PHNT_VERSION >= PHNT_WINDOWS_11

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS2)
// rev
/**
 * The RtlGetNtSystemRoot routine returns the path of the Windows system root directory.
 *
 * \return PWSTR A pointer to a null-terminated string containing the system root path.
 */
NTSYSAPI
PWSTR
NTAPI
RtlGetNtSystemRoot(
    VOID
    );

// rev
/**
 * The RtlAreLongPathsEnabled routine determines whether long path support is enabled for the current process.
 *
 * \return `TRUE` if long paths are enabled, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlAreLongPathsEnabled(
    VOID
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS2

/**
 * The RtlIsThreadWithinLoaderCallout routine determines whether the current thread is executing within a loader callout.
 *
 * \return `TRUE` if the current thread is within a loader callout, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsThreadWithinLoaderCallout(
    VOID
    );

/**
 * The RtlDllShutdownInProgress routine gets a value indicating whether the process is currently in the shutdown phase.
 *
 * \return TRUE if a shutdown of the current dll process is in progress; otherwise, FALSE.
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

/**
 * Describes a single heap block returned when walking a heap.
 */
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

/**
 * Flags describing the state of a heap entry returned by RtlWalkHeap.
 */
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

/**
 * Describes a heap allocation tag and its associated statistics.
 */
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
/**
 * Describes a single heap (version 1) in a process heap information snapshot.
 */
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
/**
 * Describes a single heap (version 2) in a process heap information snapshot.
 */
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

/**
 * Signature values identifying heap information structures.
 */
#define RTL_HEAP_SIGNATURE 0xFFEEFFEEUL
#define RTL_HEAP_SEGMENT_SIGNATURE 0xDDEEDDEEUL

/**
 * Contains version 1 information about all heaps in a process.
 */
typedef struct _RTL_PROCESS_HEAPS_V1
{
    ULONG NumberOfHeaps;
    _Field_size_(NumberOfHeaps) RTL_HEAP_INFORMATION_V1 Heaps[1];
} RTL_PROCESS_HEAPS_V1, *PRTL_PROCESS_HEAPS_V1;

/**
 * Contains version 2 information about all heaps in a process.
 */
typedef struct _RTL_PROCESS_HEAPS_V2
{
    ULONG NumberOfHeaps;
    _Field_size_(NumberOfHeaps) RTL_HEAP_INFORMATION_V2 Heaps[1];
} RTL_PROCESS_HEAPS_V2, *PRTL_PROCESS_HEAPS_V2;

//
// Segment heap parameters.
//

/**
 * Identifies the type of memory backing a heap allocation.
 */
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

/**
 * Identifies the class of heap memory information being queried.
 */
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
/**
 * Pointer to an ALLOCATE_VIRTUAL_MEMORY_EX_CALLBACK callback.
 */
typedef ALLOCATE_VIRTUAL_MEMORY_EX_CALLBACK *PALLOCATE_VIRTUAL_MEMORY_EX_CALLBACK;

typedef _Function_class_(FREE_VIRTUAL_MEMORY_EX_CALLBACK)
NTSTATUS NTAPI FREE_VIRTUAL_MEMORY_EX_CALLBACK(
    _Inout_ HANDLE CallbackContext,
    _In_ HANDLE ProcessHandle,
    _Inout_ __drv_freesMem(Mem) PVOID *BaseAddress,
    _Inout_ PSIZE_T RegionSize,
    _In_ ULONG FreeType
    );
/**
 * Pointer to a FREE_VIRTUAL_MEMORY_EX_CALLBACK callback.
 */
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
/**
 * Pointer to a QUERY_VIRTUAL_MEMORY_CALLBACK callback.
 */
typedef QUERY_VIRTUAL_MEMORY_CALLBACK *PQUERY_VIRTUAL_MEMORY_CALLBACK;

/**
 * Specifies callbacks used by a segment heap to manage its virtual address space.
 */
typedef struct _RTL_SEGMENT_HEAP_VA_CALLBACKS
{
    HANDLE CallbackContext;
    PALLOCATE_VIRTUAL_MEMORY_EX_CALLBACK AllocateVirtualMemory;
    PFREE_VIRTUAL_MEMORY_EX_CALLBACK FreeVirtualMemory;
    PQUERY_VIRTUAL_MEMORY_CALLBACK QueryVirtualMemory;
} RTL_SEGMENT_HEAP_VA_CALLBACKS, *PRTL_SEGMENT_HEAP_VA_CALLBACKS;

/**
 * Value selecting any NUMA node for a segment heap memory source.
 */
#define RTL_SEGHEAP_MEM_SOURCE_ANY_NODE ((ULONG)-1)

/**
 * Describes the memory source used to back a segment heap.
 */
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

/**
 * Version and flag values for segment heap parameters.
 */
#define SEGMENT_HEAP_PARAMETERS_VERSION         3
#define SEGMENT_HEAP_FLG_USE_PAGE_HEAP          0x1
#define SEGMENT_HEAP_FLG_NO_LFH                 0x2
#define SEGMENT_HEAP_PARAMS_VALID_FLAGS         0x3

/**
 * Specifies configuration parameters for a segment heap.
 */
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
/**
 * Pointer to an RTL_HEAP_COMMIT_ROUTINE callback.
 */
typedef RTL_HEAP_COMMIT_ROUTINE* PRTL_HEAP_COMMIT_ROUTINE;

/**
 * Specifies configuration parameters for a heap created by RtlCreateHeap.
 */
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

/**
 * Flags controlling heap allocation behavior.
 */
#define HEAP_SETTABLE_USER_VALUE 0x00000100
#define HEAP_SETTABLE_USER_FLAG1 0x00000200
#define HEAP_SETTABLE_USER_FLAG2 0x00000400
#define HEAP_SETTABLE_USER_FLAG3 0x00000800
#define HEAP_SETTABLE_USER_FLAGS 0x00000e00

/**
 * Heap class identifiers encoded in the heap creation flags.
 */
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

/**
 * Constants describing heap tag limits and layout.
 */
#define HEAP_MAXIMUM_TAG 0x0FFF
#define HEAP_GLOBAL_TAG 0x0800
#define HEAP_PSEUDO_TAG_FLAG 0x8000
#define HEAP_TAG_SHIFT 18
#define HEAP_TAG_MASK (HEAP_MAXIMUM_TAG << HEAP_TAG_SHIFT)

/**
 * Flag requesting creation of a segment heap.
 */
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
/**
 * Flag requesting creation of a hardened heap.
 */
#define HEAP_CREATE_HARDENED 0x00000200

/**
 * The RtlCreateHeap routine creates a heap object that can be used by the calling process. This routine reserves
 * space in the virtual address space of the process and allocates physical storage for a specified initial portion of this block.
 *
 * \param Flags Flags specifying optional attributes of the heap.
 * \param HeapBase If HeapBase is a non-NULL value, it specifies the base address for a block of caller-allocated memory to use for the heap.
 * \param ReserveSize If ReserveSize is a nonzero value, it specifies the initial amount of memory, in bytes, to reserve for the heap.
 * \param CommitSize If CommitSize is a nonzero value, it specifies the initial amount of memory, in bytes, to commit for the heap.
 * \param Lock Pointer to an opaque structure to be used as the heap lock.
 * \param Parameters Pointer to a RTL_HEAP_PARAMETERS structure that contains parameters to be applied when creating the heap.
 * \return RtlCreateHeap returns a handle to be used in accessing the created heap.
 * \remarks https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlcreateheap
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
 * \param HeapHandle Handle for the heap to be destroyed. This parameter is a heap handle returned by RtlCreateHeap.
 * \return If the call to RtlDestroyHeap succeeds, the return value is a NULL pointer. If the call to RtlDestroyHeap fails, the return value is a handle for the heap.
 * \remarks https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtldestroyheap
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
 * \param HeapHandle Handle for a private heap from which the memory will be allocated.
 * \param Flags Controllable aspects of heap allocation. Specifying any flags will override the corresponding value specified when the heap was created with RtlCreateHeap.
 * \param Size Number of bytes to be allocated. If the heap, specified by the HeapHandle parameter, is a nongrowable heap, Size must be less than or equal to the heap's virtual memory threshold.
 * \return If the call to RtlAllocateHeap succeeds, the return value is a pointer to the newly-allocated block. The return value is NULL if the allocation failed.
 * \remarks https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlallocateheap
 */
_Success_(return != 0)
_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
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

/**
 * The RtlFreeHeap routine frees a memory block allocated from a heap.
 *
 * \param HeapHandle A handle to the heap from which the block was allocated.
 * \param Flags Heap free flags (for example, HEAP_NO_SERIALIZE).
 * \param BaseAddress The block to free.
 * \return TRUE if the block was freed; otherwise, FALSE.
 */
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

/**
 * The RtlSizeHeap routine returns the size, in bytes, of a memory block allocated from a heap.
 *
 * \param HeapHandle A handle to the heap from which the block was allocated.
 * \param Flags Heap access flags (for example, HEAP_NO_SERIALIZE).
 * \param BaseAddress A pointer to the memory block whose size is queried.
 * \return SIZE_T The size, in bytes, of the memory block, or (SIZE_T)-1 if the block is invalid.
 */
NTSYSAPI
SIZE_T
NTAPI
RtlSizeHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _In_ PCVOID BaseAddress
    );

/**
 * The RtlZeroHeap routine zeroes the memory of the specified heap.
 *
 * \param HeapHandle A handle to the heap to zero.
 * \param Flags Heap access flags (for example, HEAP_NO_SERIALIZE).
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlZeroHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags
    );

/**
 * The RtlProtectHeap routine makes the specified heap read-only or read-write.
 *
 * \param HeapHandle A handle to the heap to protect.
 * \param MakeReadOnly A boolean that specifies whether the heap is made read-only (`TRUE`) or read-write (`FALSE`).
 */
NTSYSAPI
VOID
NTAPI
RtlProtectHeap(
    _In_ HANDLE HeapHandle,
    _In_ BOOLEAN MakeReadOnly
    );

/**
 * Retrieves the process default heap from the PEB.
 */
#define RtlProcessHeap() (NtCurrentPeb()->ProcessHeap)

/**
 * The RtlLockHeap routine acquires exclusive access to the specified heap, serializing access from other threads.
 *
 * \param HeapHandle A handle to the heap to lock.
 * \return `TRUE` if the heap was locked, otherwise `FALSE`.
 */
_When_(return != 0, _Acquires_lock_(HeapHandle))
NTSYSAPI
BOOLEAN
NTAPI
RtlLockHeap(
    _In_ HANDLE HeapHandle
    );

/**
 * The RtlUnlockHeap routine releases exclusive access to the specified heap previously acquired with RtlLockHeap.
 *
 * \param HeapHandle A handle to the heap to unlock.
 * \return `TRUE` if the heap was unlocked, otherwise `FALSE`.
 */
_When_(return != 0, _Releases_lock_(HeapHandle))
NTSYSAPI
BOOLEAN
NTAPI
RtlUnlockHeap(
    _In_ HANDLE HeapHandle
    );

/**
 * The RtlReAllocateHeap routine reallocates a memory block from a heap, changing its size.
 *
 * \param HeapHandle A handle to the heap from which the block was allocated.
 * \param Flags Heap allocation flags (for example, HEAP_ZERO_MEMORY).
 * \param BaseAddress The block to reallocate.
 * \param Size The new size, in bytes, of the block.
 * \return A pointer to the reallocated block, or NULL on failure.
 */
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

/**
 * The RtlGetUserInfoHeap routine retrieves the user value and user flags associated with a heap block.
 *
 * \param HeapHandle A handle to the heap.
 * \param Flags Heap flags.
 * \param BaseAddress The heap block to query.
 * \param UserValue An optional pointer that receives the user value.
 * \param UserFlags An optional pointer that receives the user flags.
 * \return TRUE if the information was retrieved; otherwise, FALSE.
 */
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

/**
 * The RtlSetUserValueHeap routine associates a user value with a heap block.
 *
 * \param HeapHandle A handle to the heap.
 * \param Flags Heap flags.
 * \param BaseAddress The heap block to modify.
 * \param UserValue The user value to associate with the block.
 * \return TRUE if the value was set; otherwise, FALSE.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlSetUserValueHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _In_ PVOID BaseAddress,
    _In_ PVOID UserValue
    );

/**
 * The RtlSetUserFlagsHeap routine modifies the user flags associated with a heap block.
 *
 * \param HeapHandle A handle to the heap.
 * \param Flags Heap flags.
 * \param BaseAddress The heap block to modify.
 * \param UserFlagsReset The user flags to clear.
 * \param UserFlagsSet The user flags to set.
 * \return TRUE if the flags were modified; otherwise, FALSE.
 */
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

/**
 * Receives allocation statistics for a heap tag.
 */
typedef struct _RTL_HEAP_TAG_INFO
{
    ULONG NumberOfAllocations;
    ULONG NumberOfFrees;
    SIZE_T BytesAllocated;
} RTL_HEAP_TAG_INFO, *PRTL_HEAP_TAG_INFO;

/**
 * Constructs a heap tag value from a tag base and index.
 */
#define RTL_HEAP_MAKE_TAG HEAP_MAKE_TAG_FLAGS

/**
 * The RtlCreateTagHeap routine creates a set of allocation tags used to track allocations within the specified heap.
 *
 * \param HeapHandle A handle to the heap for which tags are created.
 * \param Flags Flags that control tag creation.
 * \param TagPrefix An optional prefix applied to each created tag name.
 * \param TagNames A pointer to a multi-string containing the tag names to create.
 * \return ULONG The base tag index for the created tags.
 */
NTSYSAPI
ULONG
NTAPI
RtlCreateTagHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _In_opt_ PCWSTR TagPrefix,
    _In_ PCWSTR TagNames
    );

/**
 * The RtlQueryTagHeap routine retrieves usage statistics and the name for a heap allocation tag.
 *
 * \param HeapHandle A handle to the heap to query.
 * \param Flags Flags that control the query.
 * \param TagIndex The index of the tag to query.
 * \param ResetCounters A boolean that specifies whether the tag usage counters are reset after the query.
 * \param TagInfo An optional pointer to a structure that receives the tag usage information.
 * \return PWSTR A pointer to the tag name string.
 */
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

/**
 * The RtlExtendHeap routine extends a heap by adding a caller-supplied memory region.
 *
 * \param HeapHandle A handle to the heap to extend.
 * \param Flags Heap flags.
 * \param Base The base address of the memory region to add.
 * \param Size The size, in bytes, of the memory region.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlExtendHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _In_ PVOID Base,
    _In_ SIZE_T Size
    );

/**
 * The RtlCompactHeap routine coalesces adjacent free blocks in the specified heap and returns the size of the largest committed free block.
 *
 * \param HeapHandle A handle to the heap to compact.
 * \param Flags Heap access flags (for example, HEAP_NO_SERIALIZE).
 * \return SIZE_T The size, in bytes, of the largest committed free block.
 */
NTSYSAPI
SIZE_T
NTAPI
RtlCompactHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags
    );

/**
 * The RtlValidateHeap routine validates the internal consistency of a heap or a single heap allocation.
 *
 * \param HeapHandle An optional handle to the heap; NULL validates all process heaps.
 * \param Flags Heap flags.
 * \param BaseAddress An optional specific allocation to validate; NULL validates the whole heap.
 * \return TRUE if the heap (or allocation) is valid; otherwise, FALSE.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlValidateHeap(
    _In_opt_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _In_opt_ PVOID BaseAddress
    );

/**
 * The RtlValidateProcessHeaps routine validates the integrity of all heaps in the current process.
 *
 * \return `TRUE` if all heaps are valid, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlValidateProcessHeaps(
    VOID
    );

/**
 * The RtlGetProcessHeaps routine retrieves handles to the heaps currently active in the process.
 *
 * \param NumberOfHeaps The capacity, in entries, of the ProcessHeaps array.
 * \param ProcessHeaps A buffer that receives the heap handles.
 * \return The number of heaps active in the process.
 */
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
/**
 * Pointer to an RTL_ENUM_HEAPS_ROUTINE callback.
 */
typedef RTL_ENUM_HEAPS_ROUTINE *PRTL_ENUM_HEAPS_ROUTINE;

/**
 * The RtlEnumProcessHeaps routine enumerates the heaps of the process, invoking a callback for each.
 *
 * \param EnumRoutine The callback invoked for each process heap.
 * \param Parameter A caller-defined value passed to the callback.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlEnumProcessHeaps(
    _In_ PRTL_ENUM_HEAPS_ROUTINE EnumRoutine,
    _In_ PVOID Parameter
    );

/**
 * Describes the usage of a single allocated heap block.
 */
typedef struct _RTL_HEAP_USAGE_ENTRY
{
    struct _RTL_HEAP_USAGE_ENTRY *Next;
    PVOID Address;
    SIZE_T Size;
    USHORT AllocatorBackTraceIndex;
    USHORT TagIndex;
} RTL_HEAP_USAGE_ENTRY, *PRTL_HEAP_USAGE_ENTRY;

/**
 * Describes the overall usage of a heap, including a list of allocated blocks.
 */
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

/**
 * Flags for RtlUsageHeap.
 */
#define HEAP_USAGE_ALLOCATED_BLOCKS HEAP_REALLOC_IN_PLACE_ONLY
#define HEAP_USAGE_FREE_BUFFER HEAP_ZERO_MEMORY

/**
 * The RtlUsageHeap routine retrieves usage information for the specified heap.
 *
 * \param HeapHandle A handle to the heap to query.
 * \param Flags Flags that control the query.
 * \param Usage A pointer to a structure that receives the heap usage information.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlUsageHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _Inout_ PRTL_HEAP_USAGE Usage
    );

/**
 * Describes a single entry returned when walking a heap with RtlWalkHeap.
 */
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

/**
 * The RtlWalkHeap routine enumerates the allocated and free blocks of the specified heap.
 *
 * \param HeapHandle A handle to the heap to enumerate.
 * \param Entry A pointer to a structure that receives the next heap block and maintains enumeration state.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlWalkHeap(
    _In_ HANDLE HeapHandle,
    _Inout_ PRTL_HEAP_WALK_ENTRY Entry
    );

// HEAP_INFORMATION_CLASS
/**
 * Heap information class identifiers (HEAP_INFORMATION_CLASS).
 */
#define HeapCompatibilityInformation 0x0            // q; s: ULONG
#define HeapEnableTerminationOnCorruption 0x1       // q; s: NULL
#define HeapExtendedInformation 0x2                 // q; s: HEAP_EXTENDED_INFORMATION
#define HeapOptimizeResources 0x3                   // q; s: HEAP_OPTIMIZE_RESOURCES_INFORMATION
#define HeapTaggingInformation 0x4                  // q: RTLP_HEAP_TAGGING_INFO
#define HeapStackDatabase 0x5                       // q: RTL_HEAP_STACK_QUERY; s: RTL_HEAP_STACK_CONTROL
#define HeapMemoryLimit 0x6                         // q: since 19H2
#define HeapTag 0x7                                 // q: since 20H1
#define HeapMemoryUsageInformation 0x8              // q: HEAP_MEMORY_USAGE_INFORMATION // since 26H1
#define HeapDetailedFailureInformation 0x80000001
#define HeapSetDebuggingInformation 0x80000002      // q; s: HEAP_DEBUGGING_INFORMATION

/**
 * Identifies the front-end compatibility mode of a heap.
 */
typedef enum _HEAP_COMPATIBILITY_MODE
{
    HEAP_COMPATIBILITY_MODE_STANDARD = 0UL,
    HEAP_COMPATIBILITY_MODE_LAL = 1UL, // Lookaside list heap (LAL) compatibility mode.
    HEAP_COMPATIBILITY_MODE_LFH = 2UL, // Low-fragmentation heap (LFH) compatibility mode.
} HEAP_COMPATIBILITY_MODE;

/**
 * Describes a heap tagging information entry.
 */
typedef struct _RTLP_TAG_INFO
{
    GUID Id;
    SIZE_T CurrentAllocatedBytes;
} RTLP_TAG_INFO, *PRTLP_TAG_INFO;

/**
 * Version number of the heap tagging information structure.
 */
#define RTLP_HEAP_TAGGING_INFO_VERSION 0x1

/**
 * Contains heap tagging information for a heap.
 */
typedef struct _RTLP_HEAP_TAGGING_INFO
{
    USHORT Version;
    USHORT Flags; // 1: Multiple Tags, 2: Single Tag + Hash
    HANDLE ProcessHandle;
    SIZE_T EntriesCount;
    RTLP_TAG_INFO Entries[1];
} RTLP_HEAP_TAGGING_INFO, *PRTLP_HEAP_TAGGING_INFO;

/**
 * Contains summary heap information for a process.
 */
typedef struct _PROCESS_HEAP_INFORMATION
{
    SIZE_T ReserveSize;
    SIZE_T CommitSize;
    ULONG NumberOfHeaps;
    ULONG_PTR FirstHeapInformationOffset;
} PROCESS_HEAP_INFORMATION, *PPROCESS_HEAP_INFORMATION;

/**
 * Describes a memory region belonging to a heap.
 */
typedef struct _HEAP_REGION_INFORMATION
{
    PVOID Address;
    SIZE_T ReserveSize;
    SIZE_T CommitSize;
    ULONG_PTR FirstRangeInformationOffset;
    ULONG_PTR NextRegionInformationOffset;
} HEAP_REGION_INFORMATION, *PHEAP_REGION_INFORMATION;

/**
 * Describes a range of memory within a heap region.
 */
typedef struct _HEAP_RANGE_INFORMATION
{
    PVOID Address;
    SIZE_T Size;
    ULONG Type;
    ULONG Protection;
    ULONG_PTR FirstBlockInformationOffset;
    ULONG_PTR NextRangeInformationOffset;
} HEAP_RANGE_INFORMATION, *PHEAP_RANGE_INFORMATION;

/**
 * Describes a single block within a heap range.
 */
typedef struct _HEAP_BLOCK_INFORMATION
{
    PVOID Address;
    ULONG Flags;
    SIZE_T DataSize;
    ULONG_PTR OverheadSize;
    ULONG_PTR NextBlockInformationOffset;
} HEAP_BLOCK_INFORMATION, *PHEAP_BLOCK_INFORMATION;

/**
 * Describes a heap and its associated regions, ranges, and blocks.
 */
typedef struct _HEAP_INFORMATION
{
    PVOID Address;
    ULONG Mode;
    SIZE_T ReserveSize;
    SIZE_T CommitSize;
    ULONG_PTR FirstRegionInformationOffset;
    ULONG_PTR NextHeapInformationOffset;
} HEAP_INFORMATION, *PHEAP_INFORMATION;

/**
 * Contains performance counters for a segment heap.
 */
typedef struct _SEGMENT_HEAP_PERFORMANCE_COUNTER_INFORMATION
{
    SIZE_T SegmentReserveSize;
    SIZE_T SegmentCommitSize;
    SIZE_T SegmentCount;
    SIZE_T AllocatedSize;
    SIZE_T LargeAllocReserveSize;
    SIZE_T LargeAllocCommitSize;
} SEGMENT_HEAP_PERFORMANCE_COUNTER_INFORMATION, *PSEGMENT_HEAP_PERFORMANCE_COUNTER_INFORMATION;

/**
 * Version numbers of the heap performance counter information structures.
 */
#define HeapPerformanceCountersInformationStandardHeapVersion 0x1
#define HeapPerformanceCountersInformationSegmentHeapVersion 0x2

/**
 * Contains performance counters describing heap activity.
 */
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

/**
 * Describes a single item in an extended heap information query.
 */
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
    } DUMMYUNIONNAME;
} HEAP_INFORMATION_ITEM, *PHEAP_INFORMATION_ITEM;

typedef _Function_class_(RTL_HEAP_EXTENDED_ENUMERATION_ROUTINE)
NTSTATUS NTAPI RTL_HEAP_EXTENDED_ENUMERATION_ROUTINE(
    _In_ PHEAP_INFORMATION_ITEM Information,
    _In_opt_ PVOID Context
    );
/**
 * Pointer to an RTL_HEAP_EXTENDED_ENUMERATION_ROUTINE callback.
 */
typedef RTL_HEAP_EXTENDED_ENUMERATION_ROUTINE* PRTL_HEAP_EXTENDED_ENUMERATION_ROUTINE;

// HEAP_EXTENDED_INFORMATION Level
/**
 * Information levels for extended heap information queries.
 */
#define HeapExtendedProcessHeapInformationLevel 0x1
#define HeapExtendedHeapInformationLevel 0x2
#define HeapExtendedHeapRegionInformationLevel 0x3
#define HeapExtendedHeapRangeInformationLevel 0x4
#define HeapExtendedHeapBlockInformationLevel 0x5
#define HeapExtendedHeapHeapPerfInformationLevel 0x80000000

/**
 * Contains extended information about a heap returned by a heap information query.
 */
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
// Information points to one of: RTLP_HEAP_STACK_TRACE_SERIALIZATION_INIT,
// RTLP_HEAP_STACK_TRACE_SERIALIZATION_HEADER, RTLP_HEAP_STACK_TRACE_SERIALIZATION_ALLOCATION.
// A null Information/Size signals end-of-stream.
typedef _Function_class_(RTL_HEAP_STACK_WRITE_ROUTINE)
NTSTATUS NTAPI RTL_HEAP_STACK_WRITE_ROUTINE(
    _In_ PVOID Information,
    _In_ ULONG Size,
    _In_opt_ PVOID Context
    );
/**
 * Pointer to an RTL_HEAP_STACK_WRITE_ROUTINE callback.
 */
typedef RTL_HEAP_STACK_WRITE_ROUTINE* PRTL_HEAP_STACK_WRITE_ROUTINE;

// rev - written first; Flags == 0x80001
/**
 * Describes the initialization record of a serialized heap stack-trace database.
 */
typedef struct _RTLP_HEAP_STACK_TRACE_SERIALIZATION_INIT
{
    ULONG Count;
    ULONG Total;
    ULONG Flags;
} RTLP_HEAP_STACK_TRACE_SERIALIZATION_INIT, *PRTLP_HEAP_STACK_TRACE_SERIALIZATION_INIT;

// rev - written per-heap; Version == 2, Flags/Version field == 0x80002
/**
 * Header of a serialized heap stack-trace database.
 */
typedef struct _RTLP_HEAP_STACK_TRACE_SERIALIZATION_HEADER
{
    USHORT Version;
    USHORT PointerSize;
    PVOID Heap;
    SIZE_T TotalCommit;
    SIZE_T TotalReserve;
} RTLP_HEAP_STACK_TRACE_SERIALIZATION_HEADER, *PRTLP_HEAP_STACK_TRACE_SERIALIZATION_HEADER;

// rev - written per live allocation
/**
 * Describes a serialized heap allocation record within a stack-trace database.
 */
typedef struct _RTLP_HEAP_STACK_TRACE_SERIALIZATION_ALLOCATION
{
    PVOID Address;
    ULONG Flags;
    SIZE_T DataSize;
} RTLP_HEAP_STACK_TRACE_SERIALIZATION_ALLOCATION, *PRTLP_HEAP_STACK_TRACE_SERIALIZATION_ALLOCATION;

// rev - written as end-of-heap sentinel; Address == 0x1234CDEF, DataSize == -1
/**
 * Marks the end of a serialized heap stack-trace database.
 */
typedef struct _RTLP_HEAP_STACK_TRACE_SERIALIZATION_TERMINATOR
{
    PVOID Address; // 0x1234CDEF
    ULONG Flags;
    SIZE_T DataSize; // -1
} RTLP_HEAP_STACK_TRACE_SERIALIZATION_TERMINATOR, *PRTLP_HEAP_STACK_TRACE_SERIALIZATION_TERMINATOR;

// rev - variable-length block; Max depth is 0xC0 (192).
// Determine frame count from Size / sizeof(PVOID).
/**
 * Describes a single stack frame in a serialized heap stack-trace record.
 */
typedef struct _RTLP_HEAP_STACK_TRACE_SERIALIZATION_STACKFRAME
{
    PVOID StackFrame[ANYSIZE_ARRAY]; // actual count: Size / sizeof(PVOID)
} RTLP_HEAP_STACK_TRACE_SERIALIZATION_STACKFRAME, *PRTLP_HEAP_STACK_TRACE_SERIALIZATION_STACKFRAME;

/**
 * Version number of the heap stack query structure.
 */
#define HEAP_STACK_QUERY_VERSION 0x2

/**
 * Specifies parameters for querying heap stack-trace information.
 */
typedef struct _RTL_HEAP_STACK_QUERY
{
    ULONG Version;
    HANDLE ProcessHandle;
    PRTL_HEAP_STACK_WRITE_ROUTINE WriteRoutine;
    PVOID SerializationContext;
    UCHAR QueryLevel;
    UCHAR Flags;
} RTL_HEAP_STACK_QUERY, *PRTL_HEAP_STACK_QUERY;

/**
 * Version and flag values for heap stack-trace control.
 */
#define HEAP_STACK_CONTROL_VERSION 0x1
#define HEAP_STACK_CONTROL_FLAGS_STACKTRACE_ENABLE 0x1
#define HEAP_STACK_CONTROL_FLAGS_STACKTRACE_DISABLE 0x2

/**
 * Specifies parameters that control heap stack-trace collection.
 */
typedef struct _RTL_HEAP_STACK_CONTROL
{
    USHORT Version;
    USHORT Flags;
    HANDLE ProcessHandle;
} RTL_HEAP_STACK_CONTROL, *PRTL_HEAP_STACK_CONTROL;

//#define HEAP_MEMORY_USAGE_INFO_CURRENT_VERSION 0x1
//
//typedef struct _HEAP_MEMORY_USAGE_ENTRY
//{
//    PVOID HeapHandle;
//    SIZE_T TotalCommittedBytes;
//    SIZE_T TotalReservedBytes;
//} HEAP_MEMORY_USAGE_ENTRY, *PHEAP_MEMORY_USAGE_ENTRY;
//
//typedef struct _HEAP_MEMORY_USAGE_INFORMATION
//{
//    USHORT Version;
//    SIZE_T EntryCount;
//    HEAP_MEMORY_USAGE_ENTRY Entries[ANYSIZE_ARRAY];
//} HEAP_MEMORY_USAGE_INFORMATION, *PHEAP_MEMORY_USAGE_INFORMATION;

// rev
typedef _Function_class_(RTL_HEAP_DEBUGGING_INTERCEPTOR_ROUTINE)
NTSTATUS NTAPI RTL_HEAP_DEBUGGING_INTERCEPTOR_ROUTINE(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Action,
    _In_ ULONG StackFramesToCapture,
    _In_ PVOID *StackTrace
    );
/**
 * Pointer to an RTL_HEAP_DEBUGGING_INTERCEPTOR_ROUTINE callback.
 */
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
/**
 * Pointer to an RTL_HEAP_LEAK_ENUMERATION_ROUTINE callback.
 */
typedef RTL_HEAP_LEAK_ENUMERATION_ROUTINE* PRTL_HEAP_LEAK_ENUMERATION_ROUTINE;

// rev
// ExtendedOptions valid values are 0..3 (low 2 bits).
// RtlpSetHeapDebuggingInformation writes (ExtendedOptions << 1) into LFH bucket flags (mask 0x6),
// and app-compat metadata references "HeapPaddingAndLFHSubsegmentCommitSwitch" semantics.
/**
 * Extended debugging option flags for a heap.
 */
#define HEAP_DEBUG_EXTENDED_OPTION_NONE                                0x0
#define HEAP_DEBUG_EXTENDED_OPTION_LFH_SUBSEGMENT_COMMIT               0x1
#define HEAP_DEBUG_EXTENDED_OPTION_PAD_ALLOCATIONS_WITH_HEADER_BLOCK   0x2
#define HEAP_DEBUG_EXTENDED_OPTION_VALID_MASK                          0x3

// symbols
/**
 * Contains debugging information and callbacks for a heap.
 */
typedef struct _HEAP_DEBUGGING_INFORMATION
{
    PRTL_HEAP_DEBUGGING_INTERCEPTOR_ROUTINE InterceptorFunction;
    USHORT InterceptorValue;
    ULONG ExtendedOptions; // HEAP_DEBUG_EXTENDED_OPTION_*
    ULONG StackTraceDepth;
    SIZE_T MinTotalBlockSize;
    SIZE_T MaxTotalBlockSize;
    PRTL_HEAP_LEAK_ENUMERATION_ROUTINE HeapLeakEnumerationRoutine;
} HEAP_DEBUGGING_INFORMATION, *PHEAP_DEBUGGING_INFORMATION;

/**
 * The RtlQueryHeapInformation routine retrieves information about a heap or process heaps.
 *
 * \param HeapHandle Handle to the heap to query. For classes that operate on process-wide data,
 * this parameter may be NULL or ignored depending on HeapInformationClass.
 * \param HeapInformationClass The information class to query (for example, compatibility mode,
 * extended heap information, stack database information).
 * \param HeapInformation A caller-supplied buffer that receives the queried information.
 * \param HeapInformationLength Size, in bytes, of the HeapInformation buffer.
 * \param ReturnLength Receives the required or returned size, in bytes.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/heapapi/nf-heapapi-heapqueryinformation
 */
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

/**
 * The RtlSetHeapInformation routine sets information for a heap or process heap policy.
 *
 * \param HeapHandle Handle to the heap to configure. Some information classes allow NULL to apply
 * process-wide policy.
 * \param HeapInformationClass The information class to set.
 * \param HeapInformation Pointer to the class-specific input data.
 * \param HeapInformationLength Size, in bytes, of HeapInformation.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/heapapi/nf-heapapi-heapsetinformation
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetHeapInformation(
    _In_opt_ HANDLE HeapHandle,
    _In_ HEAP_INFORMATION_CLASS HeapInformationClass,
    _In_opt_ PCVOID HeapInformation,
    _In_opt_ SIZE_T HeapInformationLength
    );

/**
 * The RtlMultipleAllocateHeap routine allocates multiple fixed-size blocks from a heap.
 *
 * \param HeapHandle Handle for the heap from which memory is allocated.
 * \param Flags Controllable aspects of heap allocation. Specifying any flags overrides the corresponding heap defaults.
 * \param Size Number of bytes to allocate for each element.
 * \param Count Number of elements to allocate.
 * \param Array Caller-supplied array that receives pointers to allocated blocks.
 * \return The number of successfully allocated elements.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/heapapi/nf-heapapi-heapalloc
 */
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

/**
 * The RtlMultipleFreeHeap routine frees multiple heap blocks.
 *
 * \param HeapHandle Handle for the heap that owns the blocks.
 * \param Flags Controllable aspects of heap free behavior.
 * \param Count Number of elements in Array.
 * \param Array Array of pointers to previously allocated heap blocks.
 * \return The number of successfully freed elements.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/heapapi/nf-heapapi-heapfree
 */
NTSYSAPI
ULONG
NTAPI
RtlMultipleFreeHeap(
    _In_ HANDLE HeapHandle,
    _In_ ULONG Flags,
    _In_ ULONG Count,
    _In_ PVOID *Array
    );

/**
 * The RtlDetectHeapLeaks routine performs heap leak detection across all heaps
 * in the current process and invokes any registered callbacks during enumeration.
 */
NTSYSAPI
VOID
NTAPI
RtlDetectHeapLeaks(
    VOID
    );

/**
 * The RtlFlushHeaps routine flushes heap state for process heaps.
 *
 * \return This routine does not return a value.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/heapapi/nf-heapapi-heapcompact
 */
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

/**
 * Describes a single segment within a memory zone.
 */
typedef struct _RTL_MEMORY_ZONE_SEGMENT
{
    struct _RTL_MEMORY_ZONE_SEGMENT *NextSegment;
    SIZE_T Size;
    PVOID Next;
    PVOID Limit;
} RTL_MEMORY_ZONE_SEGMENT, *PRTL_MEMORY_ZONE_SEGMENT;

/**
 * Represents a memory zone, a fast suballocator carved from larger reserved segments.
 */
typedef struct _RTL_MEMORY_ZONE
{
    RTL_MEMORY_ZONE_SEGMENT Segment;
    RTL_SRWLOCK Lock;
    ULONG LockCount;
    PRTL_MEMORY_ZONE_SEGMENT FirstSegment;
} RTL_MEMORY_ZONE, *PRTL_MEMORY_ZONE;

/**
 * The RtlCreateMemoryZone routine creates a memory zone, a fast lock-free suballocator backed by committed memory.
 *
 * \param MemoryZone Receives the newly created memory zone.
 * \param InitialSize The initial number of bytes to reserve for the zone.
 * \param Flags Reserved; must be zero.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateMemoryZone(
    _Out_ PRTL_MEMORY_ZONE *MemoryZone,
    _In_ SIZE_T InitialSize,
    _Reserved_ ULONG Flags
    );

/**
 * The RtlDestroyMemoryZone routine destroys a memory zone and releases its backing memory.
 *
 * \param MemoryZone The memory zone to destroy.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyMemoryZone(
    _In_ _Post_invalid_ PRTL_MEMORY_ZONE MemoryZone
    );

/**
 * The RtlAllocateMemoryZone routine allocates a block from a memory zone.
 *
 * \param MemoryZone The memory zone to allocate from.
 * \param BlockSize The number of bytes to allocate.
 * \param Block Receives a pointer to the allocated block.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAllocateMemoryZone(
    _In_ PRTL_MEMORY_ZONE MemoryZone,
    _In_ SIZE_T BlockSize,
    _Out_ PVOID *Block
    );

/**
 * The RtlResetMemoryZone routine resets a memory zone, freeing all allocations while retaining its backing memory.
 *
 * \param MemoryZone The memory zone to reset.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlResetMemoryZone(
    _In_ PRTL_MEMORY_ZONE MemoryZone
    );

/**
 * The RtlLockMemoryZone routine locks the memory backing a memory zone into the working set.
 *
 * \param MemoryZone The memory zone to lock.
 * \return NTSTATUS Successful or errant status.
 */
_Acquires_lock_(MemoryZone)
NTSYSAPI
NTSTATUS
NTAPI
RtlLockMemoryZone(
    _In_ PRTL_MEMORY_ZONE MemoryZone
    );

/**
 * The RtlUnlockMemoryZone routine unlocks the memory backing a memory zone previously locked with RtlLockMemoryZone.
 *
 * \param MemoryZone The memory zone to unlock.
 * \return NTSTATUS Successful or errant status.
 */
_Releases_lock_(MemoryZone)
NTSYSAPI
NTSTATUS
NTAPI
RtlUnlockMemoryZone(
    _In_ PRTL_MEMORY_ZONE MemoryZone
    );

//
// Memory block lookaside lists
//

/**
 * The RtlCreateMemoryBlockLookaside routine creates a memory block lookaside list for fast fixed-range block allocation.
 *
 * \param MemoryBlockLookaside Receives the newly created lookaside.
 * \param Flags Reserved; must be zero.
 * \param InitialSize The initial number of bytes to reserve.
 * \param MinimumBlockSize The minimum block size served by the lookaside.
 * \param MaximumBlockSize The maximum block size served by the lookaside.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlDestroyMemoryBlockLookaside routine destroys a memory block lookaside and releases its backing memory.
 *
 * \param MemoryBlockLookaside The lookaside to destroy.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyMemoryBlockLookaside(
    _In_ PVOID MemoryBlockLookaside
    );

/**
 * The RtlAllocateMemoryBlockLookaside routine allocates a block from a memory block lookaside.
 *
 * \param MemoryBlockLookaside The lookaside to allocate from.
 * \param BlockSize The number of bytes to allocate.
 * \param Block Receives a pointer to the allocated block.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAllocateMemoryBlockLookaside(
    _In_ PVOID MemoryBlockLookaside,
    _In_ ULONG BlockSize,
    _Out_ PVOID *Block
    );

/**
 * The RtlFreeMemoryBlockLookaside routine returns a block to a memory block lookaside.
 *
 * \param MemoryBlockLookaside The lookaside that owns the block.
 * \param Block The block to free.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlFreeMemoryBlockLookaside(
    _In_ PVOID MemoryBlockLookaside,
    _In_ PVOID Block
    );

/**
 * The RtlExtendMemoryBlockLookaside routine extends the capacity of a memory block lookaside.
 *
 * \param MemoryBlockLookaside The lookaside to extend.
 * \param Increment The number of additional blocks to make available.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlExtendMemoryBlockLookaside(
    _In_ PVOID MemoryBlockLookaside,
    _In_ ULONG Increment
    );

/**
 * The RtlResetMemoryBlockLookaside routine resets a memory block lookaside, returning all outstanding blocks to the free list.
 *
 * \param MemoryBlockLookaside The lookaside to reset.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlResetMemoryBlockLookaside(
    _In_ PVOID MemoryBlockLookaside
    );

/**
 * The RtlLockMemoryBlockLookaside routine locks the memory backing a memory block lookaside into the working set.
 *
 * \param MemoryBlockLookaside The lookaside to lock.
 * \return NTSTATUS Successful or errant status.
 */
_Acquires_lock_(MemoryBlockLookaside)
NTSYSAPI
NTSTATUS
NTAPI
RtlLockMemoryBlockLookaside(
    _In_ PVOID MemoryBlockLookaside
    );

/**
 * The RtlUnlockMemoryBlockLookaside routine unlocks the memory backing a memory block lookaside.
 *
 * \param MemoryBlockLookaside The lookaside to unlock.
 * \return NTSTATUS Successful or errant status.
 */
_Releases_lock_(MemoryBlockLookaside)
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
/**
 * The RtlGetCurrentTransaction routine returns the transaction handle associated with the calling thread.
 *
 * \return HANDLE A handle to the current transaction, or NULL if no transaction is active.
 */
NTSYSAPI
HANDLE
NTAPI
RtlGetCurrentTransaction(
    VOID
    );

// private
/**
 * The RtlSetCurrentTransaction routine associates the specified transaction handle with the calling thread.
 *
 * \param TransactionHandle An optional handle to the transaction to associate with the current thread, or NULL to clear the current transaction.
 * \return `TRUE` if the transaction was set, otherwise `FALSE`.
 */
NTSYSAPI
LOGICAL
NTAPI
RtlSetCurrentTransaction(
    _In_opt_ HANDLE TransactionHandle
    );

//
// LUIDs
//

/**
 * The RtlIsEqualLuid routine compares two LUID values for equality.
 *
 * \param L1 A pointer to the first LUID.
 * \param L2 A pointer to the second LUID.
 * \return Returns `TRUE` if the two LUIDs are equal, otherwise `FALSE`.
 */
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

/**
 * The RtlIsZeroLuid routine determines whether a LUID is zero.
 *
 * \param L1 A pointer to the LUID to test.
 * \return Returns `TRUE` if the LUID is zero, otherwise `FALSE`.
 */
FORCEINLINE
BOOLEAN
NTAPI_INLINE
RtlIsZeroLuid(
    _In_ PLUID L1
    )
{
    return (L1->LowPart | L1->HighPart) == 0;
}

/**
 * The RtlConvertLongToLuid routine converts a signed 32-bit value to a LUID.
 *
 * \param Long The signed 32-bit value to convert.
 * \return LUID The LUID whose low part is the specified value and whose high part is zero.
 */
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

/**
 * The RtlConvertUlongToLuid routine converts an unsigned 32-bit value to a LUID.
 *
 * \param Ulong The unsigned 32-bit value to convert.
 * \return LUID The LUID whose low part is the specified value and whose high part is zero.
 */
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

/**
 * The RtlConvertLuidToLonglong routine converts a LUID to a signed 64-bit value.
 *
 * \param Luid The LUID to convert.
 * \return LONGLONG The 64-bit signed value formed from the LUID.
 */
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

/**
 * The RtlConvertLuidToUlonglong routine converts a LUID to an unsigned 64-bit value.
 *
 * \param Luid The LUID to convert.
 * \return ULONGLONG The 64-bit unsigned value formed from the LUID.
 */
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

/**
 * The RtlCopyLuid routine copies a locally unique identifier (LUID) from a source to a destination.
 *
 * \param DestinationLuid A pointer to a variable that receives the copied LUID.
 * \param SourceLuid A pointer to the LUID to copy.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlcopyluid
 */
NTSYSAPI
VOID
NTAPI
RtlCopyLuid(
    _Out_ PLUID DestinationLuid,
    _In_ PLUID SourceLuid
    );

// ros
/**
 * The RtlCopyLuidAndAttributesArray routine copies an array of LUID_AND_ATTRIBUTES structures from a source to a destination.
 *
 * \param Count The number of elements in the array.
 * \param Src A pointer to the source array of LUID_AND_ATTRIBUTES structures.
 * \param Dest A pointer to the destination array that receives the copied structures.
 */
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
/**
 * Byte-swap macros for 16-, 32-, and 64-bit integers.
 */
#define RtlUshortByteSwap(_x) _byteswap_ushort((USHORT)(_x))
#define RtlUlongByteSwap(_x) _byteswap_ulong((_x))
#define RtlUlonglongByteSwap(_x) _byteswap_uint64((_x))
#else
/**
 * The RtlUshortByteSwap routine reverses the byte order of a 16-bit unsigned integer.
 *
 * \param Source The 16-bit value whose bytes are swapped.
 * \return USHORT The byte-swapped 16-bit value.
 */
NTSYSAPI
USHORT
FASTCALL
RtlUshortByteSwap(
    _In_ USHORT Source
    );

/**
 * The RtlUlongByteSwap routine reverses the byte order of a 32-bit unsigned integer.
 *
 * \param Source The 32-bit value whose bytes are swapped.
 * \return ULONG The byte-swapped 32-bit value.
 */
NTSYSAPI
ULONG
FASTCALL
RtlUlongByteSwap(
    _In_ ULONG Source
    );

/**
 * The RtlUlonglongByteSwap routine reverses the byte order of a 64-bit unsigned integer.
 *
 * \param Source The 64-bit value whose bytes are swapped.
 * \return ULONGLONG The byte-swapped 64-bit value.
 */
NTSYSAPI
ULONGLONG
FASTCALL
RtlUlonglongByteSwap(
    _In_ ULONGLONG Source
    );
#endif // PHNT_RTL_BYTESWAP

DECLSPEC_DEPRECATED
/**
 * The RtlConvertUlongToLargeInteger routine converts an unsigned 32-bit integer to a signed large integer.
 *
 * \param UnsignedInteger The unsigned 32-bit value to convert.
 * \return LARGE_INTEGER The converted large integer.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlconvertulongtolargeinteger
 */
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlConvertUlongToLargeInteger(
    _In_ ULONG UnsignedInteger
    );

DECLSPEC_DEPRECATED
/**
 * The RtlConvertLongToLargeInteger routine converts a signed 32-bit integer to a signed large integer.
 *
 * \param SignedInteger The signed 32-bit value to convert.
 * \return LARGE_INTEGER The converted large integer.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlconvertlongtolargeinteger
 */
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlConvertLongToLargeInteger(
    _In_ LONG SignedInteger
    );

DECLSPEC_DEPRECATED
/**
 * The RtlEnlargedIntegerMultiply routine multiplies two signed 32-bit integers and returns a signed 64-bit result.
 *
 * \param Multiplicand The signed 32-bit multiplicand.
 * \param Multiplier The signed 32-bit multiplier.
 * \return LARGE_INTEGER The 64-bit product of the two operands.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlenlargedintegermultiply
 */
NTSYSAPI
LARGE_INTEGER
NTAPI
RtlEnlargedIntegerMultiply(
    _In_ LONG Multiplicand,
    _In_ LONG Multiplier
    );

DECLSPEC_DEPRECATED
/**
 * The RtlEnlargedUnsignedMultiply routine multiplies two unsigned 32-bit integers and returns an unsigned 64-bit result.
 *
 * \param Multiplicand The unsigned 32-bit multiplicand.
 * \param Multiplier The unsigned 32-bit multiplier.
 * \return LARGE_INTEGER The 64-bit product of the two operands.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlenlargedunsignedmultiply
 */
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
/**
 * Pointer to an RTL_PROCESS_MODULES structure.
 */
typedef struct _RTL_PROCESS_MODULES *PRTL_PROCESS_MODULES;
/**
 * Pointer to an RTL_PROCESS_MODULE_INFORMATION_EX structure.
 */
typedef struct _RTL_PROCESS_MODULE_INFORMATION_EX *PRTL_PROCESS_MODULE_INFORMATION_EX;
/**
 * Pointer to an RTL_PROCESS_BACKTRACES structure.
 */
typedef struct _RTL_PROCESS_BACKTRACES *PRTL_PROCESS_BACKTRACES;
/**
 * Pointer to an RTL_PROCESS_LOCKS structure.
 */
typedef struct _RTL_PROCESS_LOCKS *PRTL_PROCESS_LOCKS;

/**
 * Specifies Application Verifier options for a process.
 */
typedef struct _RTL_PROCESS_VERIFIER_OPTIONS
{
    ULONG SizeStruct;
    ULONG Option;
    UCHAR OptionData[1];
} RTL_PROCESS_VERIFIER_OPTIONS, *PRTL_PROCESS_VERIFIER_OPTIONS;

// private
/**
 * Contains the state used to capture and hold process debug information snapshots.
 */
typedef struct _RTL_DEBUG_INFORMATION
{
    HANDLE SectionHandleClient;                         // Debug buffer section handle (client view)
    PVOID ViewBaseClient;                               // Debug buffer view base (client process)
    PVOID ViewBaseTarget;                               // Debug buffer view base (target process)
    ULONG_PTR ViewBaseDelta;                            // Offset between client and target view bases
    HANDLE EventPairClient;                             // Event pair for synchronization (client)
    HANDLE EventPairTarget;                             // Event pair for synchronization (target)
    HANDLE TargetProcessId;                             // Target process ID or current process (if RTL_QUERY_PROCESS_USE_CURRENT_PROCESS set)
    HANDLE TargetThreadHandle;                          // Target thread handle
    ULONG Flags;                                        // Query flags (RTL_QUERY_PROCESS_* flags)
    SIZE_T OffsetFree;                                  // Offset of free space in debug buffer
    SIZE_T CommitSize;                                  // Committed size of debug buffer
    SIZE_T ViewSize;                                    // Total view size of debug buffer
    union
    {
        PRTL_PROCESS_MODULES Modules;                   // Module list // RtlQueryProcessModuleInformation // RTL_QUERY_PROCESS_MODULES // RTL_QUERY_PROCESS_MODULES32 // RTL_QUERY_PROCESS_MODULESEX
        PRTL_PROCESS_MODULE_INFORMATION_EX ModulesEx;   // Extended module list // RtlQueryProcessModuleInformation // RTL_QUERY_PROCESS_MODULES // RTL_QUERY_PROCESS_MODULES32 // RTL_QUERY_PROCESS_MODULESEX
    };
    PRTL_PROCESS_BACKTRACES BackTraces;                 // Stack backtraces // RtlQueryProcessBackTraceInformation // RTL_QUERY_PROCESS_BACKTRACES
    PVOID Heaps;                                        // Heap information // RtlQueryProcessHeapInformation // RTL_QUERY_PROCESS_HEAP_SUMMARY // RTL_QUERY_PROCESS_HEAP_TAGS // RTL_QUERY_PROCESS_HEAP_ENTRIES // RTL_QUERY_PROCESS_HEAP_SEGMENTS
    PRTL_PROCESS_LOCKS Locks;                           // Lock information // RtlQueryProcessLockInformation // RTL_QUERY_PROCESS_LOCKS
    PVOID SpecificHeap;                                 // Target heap to query
    HANDLE TargetProcessHandle;                         // Target process to query
    PRTL_PROCESS_VERIFIER_OPTIONS VerifierOptions;      // Verifier options // AVrfpQueryProcessVerifierOptions // RTL_QUERY_PROCESS_VERIFIER_OPTIONS
    PVOID ProcessHeap;                                  // Process heap reference
    HANDLE CriticalSectionHandle;                       // Critical section handle // RtlQueryCriticalSectionOwner // RTL_QUERY_PROCESS_CS_OWNER // RTL_QUERY_PROCESS_NONINVASIVE_CS_OWNER
    HANDLE CriticalSectionOwnerThread;                  // Critical section owner thread // RtlQueryCriticalSectionOwner
    PVOID Reserved[4];
} RTL_DEBUG_INFORMATION, *PRTL_DEBUG_INFORMATION;

/**
 * Application-defined hash function used to index stack-trace entries in a trace database.
 *
 * \param Count The number of entries in the Trace array.
 * \param Trace An array of Count captured return addresses.
 * \return The hash value for the supplied stack trace.
 */
typedef _Function_class_(RTL_TRACE_HASH_FUNCTION)
ULONG
NTAPI
RTL_TRACE_HASH_FUNCTION(
    _In_ ULONG Count,
    _In_reads_(Count) PVOID* Trace
    );
/**
 * Pointer to an RTL_TRACE_HASH_FUNCTION callback.
 */
typedef RTL_TRACE_HASH_FUNCTION *PRTL_TRACE_HASH_FUNCTION;

/**
 * The RtlCreateQueryDebugBuffer routine allocates and initializes a debug information buffer for use with the RtlQueryProcessDebugInformation routine.
 *
 * \param MaximumCommit The maximum number of bytes to commit for the buffer, or zero for the default.
 * \param UseEventPair A boolean that specifies whether an event pair is used to synchronize with the target process.
 * \return PRTL_DEBUG_INFORMATION A pointer to the allocated debug buffer, or NULL on failure.
 */
NTSYSAPI
PRTL_DEBUG_INFORMATION
NTAPI
RtlCreateQueryDebugBuffer(
    _In_opt_ ULONG MaximumCommit,
    _In_ BOOLEAN UseEventPair
    );

/**
 * The RtlDestroyQueryDebugBuffer routine frees a debug information buffer previously allocated by RtlCreateQueryDebugBuffer.
 *
 * \param Buffer A pointer to the debug buffer to free.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyQueryDebugBuffer(
    _In_ PRTL_DEBUG_INFORMATION Buffer
    );

// private
/**
 * The RtlCommitDebugInfo routine allocates a block within an RTL_DEBUG_INFORMATION buffer.
 *
 * \param Buffer The debug information buffer to allocate from.
 * \param Size The number of bytes to allocate.
 * \return A pointer to the allocated block, or NULL on failure.
 */
NTSYSAPI
PVOID
NTAPI
RtlCommitDebugInfo(
    _Inout_ PRTL_DEBUG_INFORMATION Buffer,
    _In_ SIZE_T Size
    );

// private
/**
 * The RtlDeCommitDebugInfo routine frees a block previously allocated within an RTL_DEBUG_INFORMATION buffer.
 *
 * \param Buffer The debug information buffer that owns the block.
 * \param p The block to free.
 * \param Size The size, in bytes, of the block.
 */
NTSYSAPI
VOID
NTAPI
RtlDeCommitDebugInfo(
    _Inout_ PRTL_DEBUG_INFORMATION Buffer,
    _In_ PVOID p,
    _In_ SIZE_T Size
    );

/**
 * Flags for RtlQueryProcessDebugInformation and related debug information queries.
 */
#define RTL_QUERY_PROCESS_MODULES 0x00000001 // RtlQueryProcessModuleInformation
#define RTL_QUERY_PROCESS_BACKTRACES 0x00000002 // RtlQueryProcessBackTraceInformation
#define RTL_QUERY_PROCESS_HEAP_SUMMARY 0x00000004 // RtlQueryProcessHeapInformation
#define RTL_QUERY_PROCESS_HEAP_TAGS 0x00000008 // RtlQueryProcessHeapInformation
#define RTL_QUERY_PROCESS_HEAP_ENTRIES 0x00000010 // RtlQueryProcessHeapInformation
#define RTL_QUERY_PROCESS_LOCKS 0x00000020 // RtlQueryProcessLockInformation
#define RTL_QUERY_PROCESS_MODULES32 0x00000040 // RtlQueryProcessModuleInformation (32-bit)
#define RTL_QUERY_PROCESS_VERIFIER_OPTIONS 0x00000080 // AVrfpQueryProcessVerifierOptions; rev
#define RTL_QUERY_PROCESS_MODULESEX 0x00000100 // RtlQueryProcessModuleInformation (extended); rev
#define RTL_QUERY_PROCESS_HEAP_SEGMENTS 0x00000200 // RtlQueryProcessHeapInformation (segments)
#define RTL_QUERY_PROCESS_CS_OWNER 0x00000400 // RtlQueryCriticalSectionOwner; rev
#define RTL_QUERY_PROCESS_USE_CURRENT_PROCESS 0x40000000 // Control flag for current process path; rev
#define RTL_QUERY_PROCESS_NONINVASIVE 0x80000000 // Non-invasive query flag
#define RTL_QUERY_PROCESS_NONINVASIVE_CS_OWNER 0x80000800 // RtlQueryCriticalSectionOwner (non-invasive); WIN11

/**
 * The RtlQueryProcessDebugInformation routine retrieves debug information, such as heaps, modules, and locks, for the specified process.
 *
 * \param UniqueProcessId The unique process identifier of the process to query.
 * \param Flags Flags that specify which classes of debug information are collected.
 * \param Buffer A pointer to a debug buffer, allocated by RtlCreateQueryDebugBuffer, that receives the information.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProcessDebugInformation(
    _In_ HANDLE UniqueProcessId,
    _In_ ULONG Flags,
    _Inout_ PRTL_DEBUG_INFORMATION Buffer
    );

// rev
/**
 * The RtlSetProcessDebugInformation routine sets debug information for the specified process.
 *
 * \param UniqueProcessId The unique process identifier of the target process.
 * \param Flags Flags that specify which classes of debug information are set.
 * \param Buffer A pointer to a debug buffer, allocated by RtlCreateQueryDebugBuffer, that contains the information.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetProcessDebugInformation(
    _In_ HANDLE UniqueProcessId,
    _In_ ULONG Flags,
    _Inout_ PRTL_DEBUG_INFORMATION Buffer
    );

// rev
/**
 * The RtlIsAnyDebuggerPresent routine determines whether a user-mode or kernel-mode debugger is present.
 *
 * \return Returns `TRUE` if a user-mode or kernel-mode debugger is present, otherwise `FALSE`.
 */
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
/**
 * The RtlDebugPrintTimes routine prints the accumulated timing statistics collected by the run-time library to the debugger.
 *
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDebugPrintTimes(
    VOID
    );

//
// Trace Database
//

/**
 * The RtlTraceDatabaseAdd routine adds a stack trace to a trace database.
 *
 * \param Database The trace database to modify.
 * \param Count The number of entries in the Trace array.
 * \param Trace An optional array of captured return addresses.
 * \param TraceBlock An optional pointer that receives the trace block that stores the trace.
 * \return TRUE if the trace was added; otherwise, FALSE.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlTraceDatabaseAdd(
    _In_ PRTL_TRACE_DATABASE Database,
    _In_ ULONG Count,
    _In_opt_ PVOID Trace,
    _Out_opt_ PVOID *TraceBlock
    );

/**
 * The RtlTraceDatabaseCreate routine creates a trace database used to record stack traces and other trace entries.
 *
 * \param Buckets The number of hash buckets in the trace database.
 * \param MaximumSize An optional maximum size, in bytes, for the trace database.
 * \param Flags Flags that control the trace database behavior.
 * \param Tag A tag value associated with the trace database.
 * \param HashFunction An optional hash function used to index trace entries.
 * \return PRTL_TRACE_DATABASE A pointer to the created trace database, or NULL on failure.
 */
NTSYSAPI
PRTL_TRACE_DATABASE
NTAPI
RtlTraceDatabaseCreate(
    _In_ ULONG Buckets,
    _In_opt_ SIZE_T MaximumSize,
    _In_ ULONG Flags,
    _In_ ULONG Tag,
    _In_opt_ PRTL_TRACE_HASH_FUNCTION HashFunction
    );

/**
 * The RtlTraceDatabaseDestroy routine destroys a trace database previously created by RtlTraceDatabaseCreate.
 *
 * \param Database A pointer to the trace database to destroy.
 * \return `TRUE` if the trace database was destroyed, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlTraceDatabaseDestroy(
    _In_ _Post_invalid_ PRTL_TRACE_DATABASE Database
    );

/**
 * The RtlTraceDatabaseEnumerate routine enumerates the stack traces recorded in a trace database.
 *
 * \param Database The trace database to enumerate.
 * \param Enumerator An enumeration cursor advanced across calls.
 * \param TraceBlock An optional pointer that receives the current trace block.
 * \return TRUE if a trace was returned; FALSE when enumeration is complete.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlTraceDatabaseEnumerate(
    _In_ PRTL_TRACE_DATABASE Database,
    _Inout_ PVOID Enumerator,
    _Out_opt_ PULONGLONG TraceBlock
    );

/**
 * The RtlTraceDatabaseFind routine finds a stack trace within a trace database.
 *
 * \param Database The trace database to search.
 * \param Count The number of entries in the Trace array.
 * \param Trace An optional array of captured return addresses to match.
 * \param TraceBlock An optional pointer that receives the matching trace block.
 * \return TRUE if the trace was found; otherwise, FALSE.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlTraceDatabaseFind(
    _In_ PRTL_TRACE_DATABASE Database,
    _In_ ULONG Count,
    _In_opt_ PVOID Trace,
    _Out_opt_ PVOID *TraceBlock
    );

/**
 * The RtlTraceDatabaseLock routine acquires the lock that serializes access to the specified trace database.
 *
 * \param Database A pointer to the trace database to lock.
 * \return NTSTATUS Successful or errant status.
 */
_Acquires_lock_(Database)
NTSYSAPI
NTSTATUS
NTAPI
RtlTraceDatabaseLock(
    _In_ PRTL_TRACE_DATABASE Database
    );

/**
 * The RtlTraceDatabaseUnlock routine releases the lock on the specified trace database previously acquired with RtlTraceDatabaseLock.
 *
 * \param Database A pointer to the trace database to unlock.
 * \return NTSTATUS Successful or errant status.
 */
_Releases_lock_(Database)
NTSYSAPI
NTSTATUS
NTAPI
RtlTraceDatabaseUnlock(
    _In_ PRTL_TRACE_DATABASE Database
    );

/**
 * The RtlTraceDatabaseValidate routine validates the integrity of the specified trace database.
 *
 * \param Database A pointer to the trace database to validate.
 * \return `TRUE` if the trace database is valid, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlTraceDatabaseValidate(
    _In_ PRTL_TRACE_DATABASE Database
    );

//
// Messages
//

/**
 * The RtlFindMessage routine locates a message resource entry within a module's message table.
 *
 * \param DllHandle The base address of the module containing the message table.
 * \param MessageTableId The identifier of the message table resource.
 * \param MessageLanguageId The language identifier of the message.
 * \param MessageId The identifier of the message to locate.
 * \param MessageEntry Receives a pointer to the message resource entry.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlFormatMessage routine formats a message string, substituting the supplied insert arguments.
 *
 * \param MessageFormat A pointer to the message format string.
 * \param MaximumWidth The maximum line width, in characters, or zero for no limit.
 * \param IgnoreInserts A boolean that specifies whether insert sequences are left unexpanded.
 * \param ArgumentsAreAnsi A boolean that specifies whether the arguments are ANSI strings.
 * \param ArgumentsAreAnArray A boolean that specifies whether the arguments are supplied as an array rather than a va_list.
 * \param Arguments A pointer to the insert arguments.
 * \param Buffer A buffer that receives the formatted message.
 * \param Length The size, in bytes, of the output buffer.
 * \param ReturnLength An optional pointer to a variable that receives the length, in bytes, of the formatted message.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Maintains state used while parsing a message with RtlFormatMessage-style routines.
 */
typedef struct _PARSE_MESSAGE_CONTEXT
{
    ULONG fFlags;
    ULONG cwSavColumn;
    SIZE_T iwSrc;
    SIZE_T iwDst;
    SIZE_T iwDstSpace;
    va_list lpvArgStart;
} PARSE_MESSAGE_CONTEXT, *PPARSE_MESSAGE_CONTEXT;

/**
 * Helper macros for initializing and testing a message parse context.
 */
#define INIT_PARSE_MESSAGE_CONTEXT(ctx) { (ctx)->fFlags = 0; }
#define TEST_PARSE_MESSAGE_CONTEXT_FLAG(ctx, flag) ((ctx)->fFlags & (flag))
#define SET_PARSE_MESSAGE_CONTEXT_FLAG(ctx, flag) ((ctx)->fFlags |= (flag))
#define CLEAR_PARSE_MESSAGE_CONTEXT_FLAG(ctx, flag) ((ctx)->fFlags &= ~(flag))

/**
 * The RtlFormatMessageEx routine formats a message string, substituting the supplied insert arguments, with an extended parse context.
 *
 * \param MessageFormat A pointer to the message format string.
 * \param MaximumWidth The maximum line width, in characters, or zero for no limit.
 * \param IgnoreInserts A boolean that specifies whether insert sequences are left unexpanded.
 * \param ArgumentsAreAnsi A boolean that specifies whether the arguments are ANSI strings.
 * \param ArgumentsAreAnArray A boolean that specifies whether the arguments are supplied as an array rather than a va_list.
 * \param Arguments A pointer to the insert arguments.
 * \param Buffer A buffer that receives the formatted message.
 * \param Length The size, in bytes, of the output buffer.
 * \param ReturnLength An optional pointer to a variable that receives the length, in bytes, of the formatted message.
 * \param ParseContext An optional pointer to a parse-message context that receives extended parse information.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlGetFileMUIPath routine retrieves the path of the MUI (Multilingual User Interface) resource file associated with the specified file.
 *
 * \param Flags Flags that control the MUI path lookup.
 * \param FilePath A pointer to the path of the language-neutral (LN) file.
 * \param Language A pointer to a buffer that contains, on input, the desired language and receives, on output, the resolved language.
 * \param LanguageLength A pointer to a variable that specifies and receives the length, in characters, of the language buffer.
 * \param FileMUIPath An optional buffer that receives the resolved MUI file path.
 * \param FileMUIPathLength A pointer to a variable that specifies and receives the length, in characters, of the MUI path buffer.
 * \param Enumerator A pointer to a variable that maintains enumeration state across successive calls.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlLoadString routine loads a string resource from a module.
 *
 * \param DllHandle The base address of the module containing the string resource.
 * \param StringId The identifier of the string to load.
 * \param StringLanguage An optional language name selecting the string; NULL uses the default.
 * \param Flags Flags controlling the load.
 * \param ReturnString Receives a pointer to the loaded string.
 * \param ReturnStringLen An optional pointer that receives the length, in characters, of the string.
 * \param ReturnLanguageName A buffer that receives the language name of the loaded string.
 * \param ReturnLanguageLen On input specifies the buffer length; on output receives the language name length.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlNtStatusToDosError routine converts the specified NTSTATUS code to the corresponding Windows (Win32) error code.
 *
 * \param Status The NTSTATUS code to convert.
 * \return ULONG The corresponding Windows error code.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlntstatustodoserror
 */
_When_(Status < 0, _Out_range_(>, 0))
_When_(Status >= 0, _Out_range_(==, 0))
NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosError(
    _In_ NTSTATUS Status
    );

/**
 * The RtlNtStatusToDosErrorNoTeb routine converts the specified NTSTATUS code to the corresponding Windows (Win32) error code without updating the thread environment block (TEB).
 *
 * \param Status The NTSTATUS code to convert.
 * \return ULONG The corresponding Windows error code.
 */
_When_(Status < 0, _Out_range_(>, 0))
_When_(Status >= 0, _Out_range_(==, 0))
NTSYSAPI
ULONG
NTAPI
RtlNtStatusToDosErrorNoTeb(
    _In_ NTSTATUS Status
    );

/**
 * The RtlGetLastNtStatus routine returns the NTSTATUS code of the last error recorded for the calling thread.
 *
 * \return NTSTATUS The last NTSTATUS code recorded for the calling thread.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetLastNtStatus(
    VOID
    );

/**
 * The RtlGetLastWin32Error routine returns the Windows (Win32) error code of the last error recorded for the calling thread.
 *
 * \return LONG The last Windows error code recorded for the calling thread.
 */
_Check_return_
_Post_equals_last_error_
NTSYSAPI
LONG
NTAPI
RtlGetLastWin32Error(
    VOID
    );

/**
 * The RtlSetLastWin32ErrorAndNtStatusFromNtStatus routine sets the calling thread's last error, recording both the specified NTSTATUS code and its equivalent Windows (Win32) error code.
 *
 * \param Status The NTSTATUS code to record as the last error.
 */
NTSYSAPI
VOID
NTAPI
RtlSetLastWin32ErrorAndNtStatusFromNtStatus(
    _In_ NTSTATUS Status
    );

/**
 * The RtlSetLastWin32Error routine sets the Windows (Win32) error code of the last error for the calling thread.
 *
 * \param Win32Error The Windows error code to record as the last error.
 */
NTSYSAPI
VOID
NTAPI
RtlSetLastWin32Error(
    _In_ LONG Win32Error
    );

/**
 * The RtlRestoreLastWin32Error routine restores the calling thread's last Windows (Win32) error code without side effects.
 *
 * \param Win32Error The Windows error code to restore as the last error.
 */
NTSYSAPI
VOID
NTAPI
RtlRestoreLastWin32Error(
    _In_ LONG Win32Error
    );

/**
 * Process error mode flags used by RtlGetThreadErrorMode and RtlSetThreadErrorMode.
 */
#define RTL_ERRORMODE_FAILCRITICALERRORS 0x0010
#define RTL_ERRORMODE_NOGPFAULTERRORBOX 0x0020
#define RTL_ERRORMODE_NOOPENFILEERRORBOX 0x0040

/**
 * The RtlGetThreadErrorMode routine returns the error mode for the calling thread.
 *
 * \return ULONG The current thread error mode flags.
 */
NTSYSAPI
ULONG
NTAPI
RtlGetThreadErrorMode(
    VOID
    );

/**
 * The RtlSetThreadErrorMode routine sets the error mode for the calling thread.
 *
 * \param NewMode The new thread error mode flags to set.
 * \param OldMode An optional pointer to a variable that receives the previous thread error mode.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlReportException routine reports an exception to Windows Error Reporting (WER).
 *
 * \param ExceptionRecord A pointer to the exception record describing the exception.
 * \param ContextRecord A pointer to the context record captured at the time of the exception.
 * \param Flags Flags that control how the exception is reported.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlReportExceptionEx routine reports an exception to Windows Error Reporting (WER) with an optional timeout.
 *
 * \param ExceptionRecord A pointer to the exception record describing the exception.
 * \param ContextRecord A pointer to the context record captured at the time of the exception.
 * \param Flags Flags that control how the exception is reported.
 * \param Timeout A pointer to the maximum time to wait for the report to complete.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlReportExceptionEx(
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PCONTEXT ContextRecord,
    _In_ ULONG Flags,
    _In_opt_ HANDLE ProcessHandle,
    _In_opt_ HANDLE ThreadHandle
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS1

/**
 * The RtlWerpReportException routine submits an exception report for the specified process to Windows Error Reporting (WER).
 *
 * \param ProcessId The identifier of the process for which the exception is reported.
 * \param CrashReportSharedMem A handle to the shared memory section containing the crash report data.
 * \param Entries An optional array of crash report entry pointers.
 * \param EntryCount The number of entries in the Entries array.
 * \param Flags Flags that control how the exception is reported.
 * \param CrashVerticalProcessHandle A pointer to a variable that receives a handle to the WER vertical process.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlWerpReportException(
    _In_ ULONG ProcessId,
    _In_ HANDLE CrashReportSharedMem,
    _In_reads_opt_(EntryCount) const ULONG_PTR *Entries,
    _In_ ULONG EntryCount,
    _In_ ULONG Flags,
    _Out_ PHANDLE CrashVerticalProcessHandle
    );

// rev
/**
 * The RtlReportSilentProcessExit routine reports a silent process exit for the specified process.
 *
 * \param ProcessHandle A handle to the process that is exiting.
 * \param ExitStatus The exit status of the process.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlUniform routine generates a uniformly distributed pseudo-random number from the specified seed.
 *
 * \param Seed A pointer to the seed value, which is updated on each call.
 * \return ULONG The generated pseudo-random number.
 */
NTSYSAPI
ULONG
NTAPI
RtlUniform(
    _Inout_ PULONG Seed
    );

/**
 * The RtlRandom routine generates a pseudo-random number from the specified seed.
 *
 * \param Seed A pointer to the seed value, which is updated on each call.
 * \return ULONG The generated pseudo-random number.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlrandom
 */
_Ret_range_(<=, MAXLONG)
NTSYSAPI
ULONG
NTAPI
RtlRandom(
    _Inout_ PULONG Seed
    );

/**
 * The RtlRandomEx routine generates a pseudo-random number from the specified seed using an improved algorithm.
 *
 * \param Seed A pointer to the seed value, which is updated on each call.
 * \return ULONG The generated pseudo-random number.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlrandomex
 */
_Ret_range_(<=, MAXLONG)
NTSYSAPI
ULONG
NTAPI
RtlRandomEx(
    _Inout_ PULONG Seed
    );

/**
 * Revision number of the import table hashing scheme.
 */
#define RTL_IMPORT_TABLE_HASH_REVISION 1

/**
 * The RtlComputeImportTableHash routine computes a hash of the import table of the image referenced by the specified file handle.
 *
 * \param FileHandle A handle to the image file whose import table is hashed.
 * \param Hash A buffer that receives the 16-byte computed hash.
 * \param ImportTableHashRevision The import table hash revision. This value must be 1.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlIntegerToChar routine converts an unsigned integer to its character-string representation in the specified base.
 *
 * \param Value The unsigned integer to convert.
 * \param Base The numeric base for the conversion, or zero for base 10.
 * \param OutputLength The size, in characters, of the output buffer. A negative value pads the result to the given width.
 * \param String A buffer that receives the converted character string.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlIntegerToChar(
    _In_ ULONG Value,
    _In_opt_ ULONG Base,
    _In_ LONG OutputLength, // negative to pad to width
    _Out_ PSTR String
    );

/**
 * The RtlCharToInteger routine converts the character-string representation of an integer to its integer value.
 *
 * \param String A pointer to the null-terminated character string to convert.
 * \param Base The numeric base of the string, or zero to infer the base from the string prefix.
 * \param Value A pointer to a variable that receives the converted integer value.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlchartointeger
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCharToInteger(
    _In_z_ PCSTR String,
    _In_opt_ ULONG Base,
    _Out_ PULONG Value
    );

/**
 * The RtlLargeIntegerToChar routine converts a large integer to its character-string representation in the specified base.
 *
 * \param Value A pointer to the large integer to convert.
 * \param Base The numeric base for the conversion, or zero for base 10.
 * \param OutputLength The size, in characters, of the output buffer. A negative value pads the result to the given width.
 * \param String A buffer that receives the converted character string.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlLargeIntegerToChar(
    _In_ PLARGE_INTEGER Value,
    _In_opt_ ULONG Base,
    _In_ LONG OutputLength,
    _Out_ PSTR String
    );

/**
 * Compatibility macros implementing LARGE_INTEGER comparison and arithmetic.
 */
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

/**
 * The RtlIntegerToUnicodeString routine converts an unsigned integer to its Unicode-string representation in the specified base.
 *
 * \param Value The unsigned integer to convert.
 * \param Base The numeric base for the conversion, or zero for base 10.
 * \param String A pointer to a UNICODE_STRING that receives the converted string.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlintegertounicodestring
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlIntegerToUnicodeString(
    _In_ ULONG Value,
    _In_opt_ ULONG Base,
    _Inout_ PUNICODE_STRING String
    );

/**
 * The RtlInt64ToUnicodeString routine converts an unsigned 64-bit integer to its Unicode-string representation in the specified base.
 *
 * \param Value The unsigned 64-bit integer to convert.
 * \param Base The numeric base for the conversion, or zero for base 10.
 * \param String A pointer to a UNICODE_STRING that receives the converted string.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlint64tounicodestring
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlInt64ToUnicodeString(
    _In_ ULONGLONG Value,
    _In_opt_ ULONG Base,
    _Inout_ PUNICODE_STRING String
    );

#ifdef _WIN64
/**
 * Maps RtlIntPtrToUnicodeString to the appropriate 32- or 64-bit conversion routine.
 */
#define RtlIntPtrToUnicodeString(Value, Base, String) RtlInt64ToUnicodeString(Value, Base, String)
#else
#define RtlIntPtrToUnicodeString(Value, Base, String) RtlIntegerToUnicodeString(Value, Base, String)
#endif // _WIN64

/**
 * The RtlUnicodeStringToInteger routine converts the Unicode-string representation of an integer to its integer value.
 *
 * \param String A pointer to the UNICODE_STRING to convert.
 * \param Base The numeric base of the string, or zero to infer the base from the string prefix.
 * \param Value A pointer to a variable that receives the converted integer value.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlunicodestringtointeger
 */
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
/**
 * Represents an IPv4 address.
 */
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
/**
 * Represents an IPv6 address.
 */
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
typedef IN_ADDR CONST *PCIN_ADDR;
typedef IN6_ADDR CONST *PCIN6_ADDR;

/**
 * The RtlIpv4AddressToStringA routine converts an IPv4 address to a null-terminated ANSI string in standard
 * dotted-decimal notation.
 *
 * \param Address The IPv4 address in network byte order.
 * \param AddressString A caller-supplied buffer that receives the string form.
 * The buffer should be able to hold at least 16 characters.
 * \return A pointer to the terminating null character written to AddressString.
 * \remarks This is a convenience routine that does not require Winsock to be loaded.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ip2string/nf-ip2string-rtlipv4addresstostringa
 */
NTSYSAPI
PSTR
NTAPI
RtlIpv4AddressToStringA(
    _In_ PCIN_ADDR Address,
    _Out_writes_(16) PSTR AddressString
    );

/**
 * The RtlIpv4AddressToStringW routine converts an IPv4 address to a null-terminated Unicode string in standard dotted-decimal notation.
 *
 * \param Address The IPv4 address in network byte order.
 * \param AddressString A caller-supplied buffer that receives the string form.
 * The buffer should be able to hold at least 16 wide characters.
 * \return A pointer to the terminating null character written to AddressString.
 * \remarks This is a convenience routine that does not require Winsock to be loaded.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ip2string/nf-ip2string-rtlipv4addresstostringw
 */
NTSYSAPI
PWSTR
NTAPI
RtlIpv4AddressToStringW(
    _In_ PCIN_ADDR Address,
    _Out_writes_(16) PWSTR AddressString
    );

/**
 * The RtlIpv4AddressToStringExA routine converts an IPv4 address and port to a null-terminated ANSI string.
 *
 * \param Address The IPv4 address in network byte order.
 * \param Port The port number in network byte order.
 * \param AddressString A caller-supplied output buffer.
 * \param AddressStringLength On input, the size of AddressString. On output,
 * receives the required size if the buffer is too small.
 * \return STATUS_SUCCESS on success, or an error status such as
 * STATUS_INVALID_PARAMETER if the buffer is too small.
 * \remarks The resulting string uses dotted-decimal notation followed by a
 * colon and port number.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ip2string/nf-ip2string-rtlipv4addresstostringexa
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlIpv4AddressToStringExA(
    _In_ PCIN_ADDR Address,
    _In_ USHORT Port,
    _Out_writes_to_(*AddressStringLength, *AddressStringLength) PSTR AddressString,
    _Inout_ PULONG AddressStringLength
    );

/**
 * The RtlIpv4AddressToStringExW routine converts an IPv4 address and port to a null-terminated Unicode string.
 *
 * \param Address The IPv4 address in network byte order.
 * \param Port The port number in network byte order.
 * \param AddressString A caller-supplied output buffer.
 * \param AddressStringLength On input, the size of AddressString. On output,
 * receives the required size if the buffer is too small.
 * \return STATUS_SUCCESS on success, or an error status such as
 * STATUS_INVALID_PARAMETER if the buffer is too small.
 * \remarks The resulting string uses dotted-decimal notation followed by a
 * colon and port number.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ip2string/nf-ip2string-rtlipv4addresstostringexw
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlIpv4AddressToStringExW(
    _In_ PCIN_ADDR Address,
    _In_ USHORT Port,
    _Out_writes_to_(*AddressStringLength, *AddressStringLength) PWSTR AddressString,
    _Inout_ PULONG AddressStringLength
    );

/**
 * The RtlIpv6AddressToStringA routine converts an IPv6 address to a null-terminated ANSI string in standard IPv6 text format.
 *
 * \param Address The IPv6 address in network byte order.
 * \param AddressString A caller-supplied buffer that receives the string form.
 * The buffer should be able to hold at least 46 characters.
 * \return A pointer to the terminating null character written to AddressString.
 * \remarks This is a convenience routine that does not require Winsock to be loaded.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ip2string/nf-ip2string-rtlipv6addresstostringa
 */
NTSYSAPI
PSTR
NTAPI
RtlIpv6AddressToStringA(
    _In_ PCIN6_ADDR Address,
    _Out_writes_(46) PSTR AddressString
    );

/**
 * The RtlIpv6AddressToStringW routine converts an IPv6 address to a null-terminated Unicode string in standard IPv6 text format.
 *
 * \param Address The IPv6 address in network byte order.
 * \param AddressString A caller-supplied buffer that receives the string form.
 * The buffer should be able to hold at least 46 wide characters.
 * \return A pointer to the terminating null character written to AddressString.
 * \remarks This is a convenience routine that does not require Winsock to be loaded.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ip2string/nf-ip2string-rtlipv6addresstostringw
 */
NTSYSAPI
PWSTR
NTAPI
RtlIpv6AddressToStringW(
    _In_ PCIN6_ADDR Address,
    _Out_writes_(46) PWSTR AddressString
    );

/**
 * The RtlIpv6AddressToStringExA routine converts an IPv6 address, scope ID, and port to a null-terminated ANSI string.
 *
 * \param Address The IPv6 address in network byte order.
 * \param ScopeId The IPv6 scope identifier.
 * \param Port The port number in network byte order.
 * \param AddressString A caller-supplied output buffer.
 * \param AddressStringLength On input, the size of AddressString. On output,
 * receives the required size if the buffer is too small.
 * \return STATUS_SUCCESS on success or an error status on failure.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ip2string/nf-ip2string-rtlipv6addresstostringexa
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlIpv6AddressToStringExA(
    _In_ PCIN6_ADDR Address,
    _In_ ULONG ScopeId,
    _In_ USHORT Port,
    _Out_writes_to_(*AddressStringLength, *AddressStringLength) PSTR AddressString,
    _Inout_ PULONG AddressStringLength
    );

/**
 * The RtlIpv6AddressToStringExW routine converts an IPv6 address, scope ID, and port to a null-terminated Unicode string.
 *
 * \param Address The IPv6 address in network byte order.
 * \param ScopeId The IPv6 scope identifier.
 * \param Port The port number in network byte order.
 * \param AddressString A caller-supplied output buffer.
 * \param AddressStringLength On input, the size of AddressString. On output,
 * receives the required size if the buffer is too small.
 * \return STATUS_SUCCESS on success or an error status on failure.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ip2string/nf-ip2string-rtlipv6addresstostringexw
 */
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

/**
 * The RtlIpv4StringToAddressA routine parses an ANSI IPv4 address string into a binary IPv4 address.
 *
 * \param AddressString The null-terminated IPv4 address string to parse.
 * \param Strict If TRUE, requires strict four-part dotted-decimal notation.
 * If FALSE, additional legacy forms are accepted.
 * \param Terminator On success, receives a pointer to the character that
 * terminated the parsed address.
 * \param Address Receives the parsed IPv4 address in network byte order.
 * \return STATUS_SUCCESS on success or STATUS_INVALID_PARAMETER if parsing fails.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ip2string/nf-ip2string-rtlipv4stringtoaddressa
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlIpv4StringToAddressA(
    _In_ PCSTR AddressString,
    _In_ BOOLEAN Strict,
    _Out_ PCSTR *Terminator,
    _Out_ PIN_ADDR Address
    );

/**
 * The RtlIpv4StringToAddressW routine parses a Unicode IPv4 address string into a binary IPv4 address.
 *
 * \param AddressString The null-terminated IPv4 address string to parse.
 * \param Strict If TRUE, requires strict four-part dotted-decimal notation.
 * If FALSE, additional legacy forms are accepted.
 * \param Terminator On success, receives a pointer to the character that
 * terminated the parsed address.
 * \param Address Receives the parsed IPv4 address in network byte order.
 * \return STATUS_SUCCESS on success or STATUS_INVALID_PARAMETER if parsing fails.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ip2string/nf-ip2string-rtlipv4stringtoaddressw
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlIpv4StringToAddressW(
    _In_ PCWSTR AddressString,
    _In_ BOOLEAN Strict,
    _Out_ PCWSTR *Terminator,
    _Out_ PIN_ADDR Address
    );

/**
 * The RtlIpv4StringToAddressExA routine parses an ANSI IPv4 address string and optional port into binary values.
 *
 * \param AddressString The null-terminated IPv4 address string, optionally
 * followed by a colon and port number.
 * \param Strict If TRUE, requires strict four-part dotted-decimal notation.
 * If FALSE, additional legacy forms are accepted.
 * \param Address Receives the parsed IPv4 address in network byte order.
 * \param Port Receives the parsed port in network byte order, or zero if not present.
 * \return STATUS_SUCCESS on success or STATUS_INVALID_PARAMETER on failure.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ip2string/nf-ip2string-rtlipv4stringtoaddressexa
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlIpv4StringToAddressExA(
    _In_ PCSTR AddressString,
    _In_ BOOLEAN Strict,
    _Out_ PIN_ADDR Address,
    _Out_ PUSHORT Port
    );

/**
 * The RtlIpv4StringToAddressExW routine parses a Unicode IPv4 address string and optional port into binary values.
 *
 * \param AddressString The null-terminated IPv4 address string, optionally
 * followed by a colon and port number.
 * \param Strict If TRUE, requires strict four-part dotted-decimal notation.
 * If FALSE, additional legacy forms are accepted.
 * \param Address Receives the parsed IPv4 address in network byte order.
 * \param Port Receives the parsed port in network byte order, or zero if not present.
 * \return STATUS_SUCCESS on success or STATUS_INVALID_PARAMETER on failure.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ip2string/nf-ip2string-rtlipv4stringtoaddressexw
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlIpv4StringToAddressExW(
    _In_ PCWSTR AddressString,
    _In_ BOOLEAN Strict,
    _Out_ PIN_ADDR Address,
    _Out_ PUSHORT Port
    );

/**
 * The RtlIpv6StringToAddressA routine parses an ANSI IPv6 address string into a binary IPv6 address.
 *
 * \param AddressString The null-terminated IPv6 address string to parse.
 * \param Terminator On success, receives a pointer to the character that
 * terminated the parsed address.
 * \param Address Receives the parsed IPv6 address in network byte order.
 * \return STATUS_SUCCESS on success or STATUS_INVALID_PARAMETER on failure.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ip2string/nf-ip2string-rtlipv6stringtoaddressa
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlIpv6StringToAddressA(
    _In_ PCSTR AddressString,
    _Out_ PCSTR *Terminator,
    _Out_ PIN6_ADDR Address
    );

/**
 * The RtlIpv6StringToAddressW routine parses a Unicode IPv6 address string into a binary IPv6 address.
 *
 * \param AddressString The null-terminated IPv6 address string to parse.
 * \param Terminator On success, receives a pointer to the character that
 * terminated the parsed address.
 * \param Address Receives the parsed IPv6 address in network byte order.
 * \return STATUS_SUCCESS on success or STATUS_INVALID_PARAMETER on failure.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ip2string/nf-ip2string-rtlipv6stringtoaddressw
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlIpv6StringToAddressW(
    _In_ PCWSTR AddressString,
    _Out_ PCWSTR *Terminator,
    _Out_ PIN6_ADDR Address
    );

/**
 * The RtlIpv6StringToAddressExA routine parses an ANSI IPv6 address string with optional scope ID and port.
 *
 * \param AddressString The null-terminated IPv6 address string to parse.
 * \param Address Receives the parsed IPv6 address in network byte order.
 * \param ScopeId Receives the parsed scope identifier.
 * \param Port Receives the parsed port in network byte order.
 * \return STATUS_SUCCESS on success or STATUS_INVALID_PARAMETER on failure.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ip2string/nf-ip2string-rtlipv6stringtoaddressexa
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlIpv6StringToAddressExA(
    _In_ PCSTR AddressString,
    _Out_ PIN6_ADDR Address,
    _Out_ PULONG ScopeId,
    _Out_ PUSHORT Port
    );

/**
 * The RtlIpv6StringToAddressExW routine parses a Unicode IPv6 address string with optional scope ID and port.
 *
 * \param AddressString The null-terminated IPv6 address string to parse.
 * \param Address Receives the parsed IPv6 address in network byte order.
 * \param ScopeId Receives the parsed scope identifier.
 * \param Port Receives the parsed port in network byte order.
 * \return STATUS_SUCCESS on success or STATUS_INVALID_PARAMETER on failure.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/ip2string/nf-ip2string-rtlipv6stringtoaddressexw
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlIpv6StringToAddressExW(
    _In_ PCWSTR AddressString,
    _Out_ PIN6_ADDR Address,
    _Out_ PULONG ScopeId,
    _Out_ PUSHORT Port
    );

/**
 * Aliases mapping the generic IP address string routines to their ANSI or Unicode forms.
 */
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

/**
 * Represents a time value broken into its individual calendar fields.
 */
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

/**
 * The RtlCutoverTimeToSystemTime routine converts a daylight-saving cutover time to an absolute system time.
 *
 * \param CutoverTime A pointer to the TIME_FIELDS structure describing the cutover time.
 * \param SystemTime A pointer to a variable that receives the resulting absolute system time.
 * \param CurrentSystemTime A pointer to the current system time used to resolve the cutover.
 * \param ThisYear A boolean that specifies whether the cutover is resolved for the current year.
 * \return `TRUE` if the conversion succeeded, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlcutovertimetosystemtime
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlCutoverTimeToSystemTime(
    _In_ PTIME_FIELDS CutoverTime,
    _Out_ PLARGE_INTEGER SystemTime,
    _In_ PLARGE_INTEGER CurrentSystemTime,
    _In_ BOOLEAN ThisYear
    );

/**
 * The RtlSystemTimeToLocalTime routine converts a system (UTC) time to local time.
 *
 * \param SystemTime A pointer to the system time to convert.
 * \param LocalTime A pointer to a variable that receives the resulting local time.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlsystemtimetolocaltime
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSystemTimeToLocalTime(
    _In_ PLARGE_INTEGER SystemTime,
    _Out_ PLARGE_INTEGER LocalTime
    );

/**
 * The RtlLocalTimeToSystemTime routine converts a local time to system (UTC) time.
 *
 * \param LocalTime A pointer to the local time to convert.
 * \param SystemTime A pointer to a variable that receives the resulting system time.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtllocaltimetosystemtime
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlLocalTimeToSystemTime(
    _In_ PLARGE_INTEGER LocalTime,
    _Out_ PLARGE_INTEGER SystemTime
    );

/**
 * The RtlTimeToElapsedTimeFields routine converts an elapsed-time interval to a TIME_FIELDS structure.
 *
 * \param Time A pointer to the elapsed-time interval, in 100-nanosecond units.
 * \param TimeFields A pointer to a TIME_FIELDS structure that receives the converted elapsed time.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtltimetoelapsedtimefields
 */
NTSYSAPI
VOID
NTAPI
RtlTimeToElapsedTimeFields(
    _In_ PLARGE_INTEGER Time,
    _Out_ PTIME_FIELDS TimeFields
    );

/**
 * The RtlTimeToTimeFields routine converts a system time to a TIME_FIELDS structure.
 *
 * \param Time A pointer to the system time, in 100-nanosecond units, to convert.
 * \param TimeFields A pointer to a TIME_FIELDS structure that receives the converted time.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtltimetotimefields
 */
NTSYSAPI
VOID
NTAPI
RtlTimeToTimeFields(
    _In_ PLARGE_INTEGER Time,
    _Out_ PTIME_FIELDS TimeFields
    );

/**
 * The RtlTimeFieldsToTime routine converts a TIME_FIELDS structure into a 64-bit system time value.
 *
 * \param TimeFields A pointer to the TIME_FIELDS structure to convert. The Weekday member is ignored.
 * \param Time A pointer to a variable that receives the converted system time.
 * \return Returns `TRUE` if the time fields are valid and the conversion succeeds, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtltimefieldstotime
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlTimeFieldsToTime(
    _In_ PTIME_FIELDS TimeFields, // Weekday is ignored
    _Out_ PLARGE_INTEGER Time
    );

/**
 * Constants giving the number of seconds from 1970 to the start of 1980.
 */
#define SecondsToStartOf1980 LONGLONG_C(11960006400)
#define SecondsToStartOf1970 LONGLONG_C(11644473600)

/**
 * The RtlTimeToSecondsSince1980 routine converts a 64-bit system time value into the number of seconds elapsed since January 1, 1980.
 *
 * \param Time A pointer to the system time value to convert.
 * \param ElapsedSeconds A pointer to a variable that receives the number of elapsed seconds since 1980.
 * \return Returns `TRUE` if the conversion succeeds, otherwise `FALSE` if the time is out of range.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtltimetosecondssince1980
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlTimeToSecondsSince1980(
    _In_ PLARGE_INTEGER Time,
    _Out_ PULONG ElapsedSeconds
    );

/**
 * The RtlSecondsSince1980ToTime routine converts a number of seconds elapsed since January 1, 1980 into a 64-bit system time value.
 *
 * \param ElapsedSeconds The number of elapsed seconds since 1980 to convert.
 * \param Time A pointer to a variable that receives the converted system time.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlsecondssince1980totime
 */
NTSYSAPI
VOID
NTAPI
RtlSecondsSince1980ToTime(
    _In_ ULONG ElapsedSeconds,
    _Out_ PLARGE_INTEGER Time
    );

/**
 * The RtlTimeToSecondsSince1970 routine converts a 64-bit system time value into the number of seconds elapsed since January 1, 1970.
 *
 * \param Time A pointer to the system time value to convert.
 * \param ElapsedSeconds A pointer to a variable that receives the number of elapsed seconds since 1970.
 * \return Returns `TRUE` if the conversion succeeds, otherwise `FALSE` if the time is out of range.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlTimeToSecondsSince1970(
    _In_ PLARGE_INTEGER Time,
    _Out_ PULONG ElapsedSeconds
    );

/**
 * The RtlSecondsSince1970ToTime routine converts a number of seconds elapsed since January 1, 1970 into a 64-bit system time value.
 *
 * \param ElapsedSeconds The number of elapsed seconds since 1970 to convert.
 * \param Time A pointer to a variable that receives the converted system time.
 */
NTSYSAPI
VOID
NTAPI
RtlSecondsSince1970ToTime(
    _In_ ULONG ElapsedSeconds,
    _Out_ PLARGE_INTEGER Time
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
/**
 * The RtlGetSystemTimePrecise routine returns the current system time with the highest available precision.
 *
 * \return ULONGLONG The current system time, in 100-nanosecond intervals since January 1, 1601 (UTC).
 */
NTSYSAPI
ULONGLONG
NTAPI
RtlGetSystemTimePrecise(
    VOID
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8

#if (PHNT_VERSION >= PHNT_WINDOWS_10_21H2)
/**
 * The RtlGetSystemTimeAndBias routine returns the current system time together with the current time zone bias and the effective range of that bias.
 *
 * \param TimeZoneBias A pointer to a variable that receives the current time zone bias, in 100-nanosecond intervals.
 * \param TimeZoneBiasEffectiveStart An optional pointer to a variable that receives the time at which the current bias became effective.
 * \param TimeZoneBiasEffectiveEnd An optional pointer to a variable that receives the time at which the current bias ceases to be effective.
 * \return ULONGLONG The current system time, in 100-nanosecond intervals since January 1, 1601 (UTC).
 */
NTSYSAPI
ULONGLONG
NTAPI
RtlGetSystemTimeAndBias(
    _Out_ PLARGE_INTEGER TimeZoneBias,
    _Out_opt_ PLARGE_INTEGER TimeZoneBiasEffectiveStart,
    _Out_opt_ PLARGE_INTEGER TimeZoneBiasEffectiveEnd
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_21H2

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
/**
 * The RtlGetInterruptTimePrecise routine returns the current interrupt time with the highest available precision, together with the correlated performance counter.
 *
 * \param PerformanceCounter A pointer to a variable that receives the performance counter value correlated with the returned interrupt time.
 * \return ULONGLONG The current interrupt time, in 100-nanosecond intervals.
 */
NTSYSAPI
ULONGLONG
NTAPI
RtlGetInterruptTimePrecise(
    _Out_ PLARGE_INTEGER PerformanceCounter
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
/**
 * The RtlQueryUnbiasedInterruptTime routine returns the current unbiased interrupt time, which excludes time spent in suspend or connected-standby states.
 *
 * \param InterruptTime A pointer to a variable that receives the current unbiased interrupt time, in 100-nanosecond intervals.
 * \return Returns `TRUE` if the unbiased interrupt time was retrieved successfully, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlQueryUnbiasedInterruptTime(
    _Out_ PLARGE_INTEGER InterruptTime
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The RtlQueryUnbiasedInterruptTimePrecise routine returns the current unbiased interrupt time with the highest available precision.
 *
 * \param InterruptTime A pointer to a variable that receives the current unbiased interrupt time, in 100-nanosecond intervals.
 * \return ULONGLONG The current biased interrupt time, in 100-nanosecond intervals, used to correlate the unbiased value.
 */
NTSYSAPI
ULONGLONG
NTAPI
RtlQueryUnbiasedInterruptTimePrecise(
    _Out_ PLARGE_INTEGER InterruptTime
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10

#if (PHNT_VERSION >= PHNT_WINDOWS_11_24H2)
// RtlGetMultiTimePrecise RequestedMask/ProvidedMask bits
/**
 * Request flags for RtlGetMultiTimePrecise.
 */
#define RTL_GET_MULTI_TIME_PRECISE_PERF_COUNTER        0x00000001UL
#define RTL_GET_MULTI_TIME_PRECISE_HV_CORRELATED_TIME  0x00000002UL
#define RTL_GET_MULTI_TIME_PRECISE_SHAREDUSER_TIME     0x00000004UL
#define RTL_GET_MULTI_TIME_PRECISE_SUPPORTED_MASK      0x00000007UL

/**
 * Contains multiple precise system time values captured together.
 */
typedef struct _RTL_MULTI_TIME_PRECISE
{
    ULONGLONG PerformanceCounter;
    ULONGLONG HypervisorCorrelatedTime;
    ULONGLONG SharedUserTime;
} RTL_MULTI_TIME_PRECISE, *PRTL_MULTI_TIME_PRECISE;

// Bit 0x1: writes PerformanceCounter; sets ProvidedMask bit 0x1.
// Bit 0x2: writes HypervisorCorrelatedTime when available/stable; sets bit 0x2 on success.
// Bit 0x4: writes SharedUserTime using SharedUserData calibration fields; sets bit 0x4.
// RequestedMask == 0 returns success with *ProvidedMask = 0.
/**
 * The RtlGetMultiTimePrecise routine retrieves several correlated high-precision time values in a single call, according to a caller-supplied request mask.
 *
 * \param TimesOut A pointer to a RTL_MULTI_TIME_PRECISE structure that receives the requested time values.
 * \param RequestedMask A bit mask selecting which time values to retrieve (for example, performance counter, hypervisor-correlated time, and shared user time).
 * \param ProvidedMask A pointer to a variable that receives the mask of time values that were actually provided.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetMultiTimePrecise(
    _Out_ PRTL_MULTI_TIME_PRECISE TimesOut,
    _In_ ULONG RequestedMask,
    _Out_ PULONG ProvidedMask
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11_24H2

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The RtlBeginReadTickLock routine begins a lock-free read of the tick count by spinning until the time update lock is not being written.
 *
 * \param TimeUpdateLock A pointer to the time update lock (USER_SHARED_DATA->TimeUpdateLock).
 * \return ULONGLONG The value of the time update lock captured at the start of a stable read.
 */
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

/**
 * Describes a time zone, including its bias and daylight-saving transitions.
 */
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

/**
 * The RtlQueryTimeZoneInformation routine retrieves the current time zone information for the system.
 *
 * \param TimeZoneInformation A pointer to a RTL_TIME_ZONE_INFORMATION structure that receives the current time zone settings.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryTimeZoneInformation(
    _Out_ PRTL_TIME_ZONE_INFORMATION TimeZoneInformation
    );

/**
 * The RtlSetTimeZoneInformation routine sets the current time zone information for the system.
 *
 * \param TimeZoneInformation A pointer to a RTL_TIME_ZONE_INFORMATION structure that specifies the time zone settings to apply.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetTimeZoneInformation(
    _In_ PRTL_TIME_ZONE_INFORMATION TimeZoneInformation
    );

//
// Interlocked bit manipulation interfaces
//

/**
 * Atomically sets the specified bits of a ULONG and returns the previous value.
 */
#define RtlInterlockedSetBits(Flags, Flag) \
    InterlockedOr((PLONG)(Flags), Flag)

/**
 * Atomically ANDs the specified bits into a ULONG and returns the previous value.
 */
#define RtlInterlockedAndBits(Flags, Flag) \
    InterlockedAnd((PLONG)(Flags), Flag)

/**
 * Atomically clears the specified bits of a ULONG and returns the previous value.
 */
#define RtlInterlockedClearBits(Flags, Flag) \
    RtlInterlockedAndBits(Flags, ~(Flag))

/**
 * Atomically XORs the specified bits of a ULONG and returns the previous value.
 */
#define RtlInterlockedXorBits(Flags, Flag) \
    InterlockedXor(Flags, Flag)

/**
 * Atomically sets the specified bits of a ULONG, discarding the previous value.
 */
#define RtlInterlockedSetBitsDiscardReturn(Flags, Flag) \
    (VOID) RtlInterlockedSetBits(Flags, Flag)

/**
 * Atomically ANDs the specified bits into a ULONG, discarding the previous value.
 */
#define RtlInterlockedAndBitsDiscardReturn(Flags, Flag) \
    (VOID) RtlInterlockedAndBits(Flags, Flag)

/**
 * Atomically clears the specified bits of a ULONG, discarding the previous value.
 */
#define RtlInterlockedClearBitsDiscardReturn(Flags, Flag) \
    RtlInterlockedAndBitsDiscardReturn(Flags, ~(Flag))

/**
 * Atomically tests whether the specified bits of a ULONG are set.
 */
#define RtlInterlockedTestBits(Flags, Flag) \
    ((InterlockedOr((PLONG)(Flags), 0) & (Flag)) == (Flag)) // dmex

//
// Bitmaps
//

/**
 * Represents a bitmap of a specified number of bits.
 */
typedef struct _RTL_BITMAP
{
    ULONG SizeOfBitMap;
    PULONG Buffer;
} RTL_BITMAP, *PRTL_BITMAP;

/**
 * The RtlInitializeBitMap routine initializes the header of a bitmap variable.
 *
 * \param BitMapHeader Pointer to an empty RTL_BITMAP structure.
 * \param BitMapBuffer Pointer to caller-allocated memory for the bitmap itself. The base address of this buffer must be ULONG-aligned. The size of the allocated buffer must be an integer multiple of sizeof(ULONG) bytes.
 * \param SizeOfBitMap Specifies the number of bits in the bitmap. This value can be any number of bits that will fit in the buffer allocated for the bitmap.
 * \remarks RtlInitializeBitMap must be called before any other RtlXxx routine that operates on a bitmap variable. The BitMapHeader pointer is an input parameter in all subsequent RtlXxx calls that operate on the caller's bitmap variable at BitMapBuffer. The caller is responsible for synchronizing access to the bitmap variable.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlinitializebitmap
 */
#ifndef PHNT_NO_INLINE_RTL_BITMAP
FORCEINLINE
VOID
RtlInitializeBitMap(
    _Out_ PRTL_BITMAP BitMapHeader,
    _In_ PULONG BitMapBuffer,
    _In_ ULONG SizeOfBitMap
    )
{
    BitMapHeader->SizeOfBitMap = SizeOfBitMap;
    BitMapHeader->Buffer = BitMapBuffer;
}
#else
NTSYSAPI
VOID
NTAPI
RtlInitializeBitMap(
    _Out_ PRTL_BITMAP BitMapHeader,
    _In_ PULONG BitMapBuffer,
    _In_ ULONG SizeOfBitMap
    );
#endif

/**
 * The RtlClearBit routine sets the specified bit in a bitmap to zero.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP structure that describes the bitmap. This structure must have been initialized by the RtlInitializeBitMap routine.
 * \param BitNumber Specifies the zero-based index of the bit within the bitmap. The routine sets this bit to zero.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlclearbit
 */
#if (PHNT_MODE == PHNT_MODE_KERNEL || PHNT_VERSION >= PHNT_WINDOWS_8)
NTSYSAPI
VOID
NTAPI
RtlClearBit(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG BitNumber
    );
#endif // PHNT_MODE == PHNT_MODE_KERNEL || PHNT_VERSION >= PHNT_WINDOWS_8

/**
 * The RtlSetBit routine sets the specified bit in a bitmap to one.
 *
 * \param BitMapHeader Pointer to the RTL_BITMAP structure that describes the bitmap. This structure must have been initialized by the RtlInitializeBitMap routine.
 * \param BitNumber Specifies the zero-based index of the bit within the bitmap. The routine sets this bit to one.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlsetbit
 */
#if (PHNT_MODE == PHNT_MODE_KERNEL || PHNT_VERSION >= PHNT_WINDOWS_8)
#ifndef PHNT_NO_INLINE_RTL_BITMAP
FORCEINLINE
VOID
RtlSetBit(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG BitNumber
    )
{
    ((PUCHAR)BitMapHeader->Buffer)[BitNumber >> 3] |= (UCHAR)(1u << (BitNumber & 7));
}
#else
NTSYSAPI
VOID
NTAPI
RtlSetBit(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG BitNumber
    );
#endif
#endif // PHNT_MODE == PHNT_MODE_KERNEL || PHNT_VERSION >= PHNT_WINDOWS_8

/**
 * The RtlTestBit routine returns the value of a bit in a bitmap.
 *
 * \param BitMapHeader Pointer to the RTL_BITMAP structure that describes the bitmap. This structure must have been initialized by the RtlInitializeBitMap routine.
 * \param BitNumber Specifies the zero-based index of the bit within the bitmap. The routine returns the value of this bit.
 * \return RtlTestBit returns the value of the bit that the BitNumber parameter points to.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtltestbit
 */
#ifndef PHNT_NO_INLINE_RTL_BITMAP
FORCEINLINE
BOOLEAN
RtlTestBit(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG BitNumber
    )
{
    return (BOOLEAN)((((const UCHAR*)BitMapHeader->Buffer)[BitNumber >> 3] >> (BitNumber & 7)) & 0x1);
}
#else
_Check_return_
NTSYSAPI
BOOLEAN
NTAPI
RtlTestBit(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG BitNumber
    );
#endif

/**
 * The RtlClearAllBits routine sets all bits in a given bitmap variable to zero.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP structure that describes the bitmap. This structure must have been initialized by the RtlInitializeBitMap routine.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlclearallbits
 */
NTSYSAPI
VOID
NTAPI
RtlClearAllBits(
    _In_ PRTL_BITMAP BitMapHeader
    );

/**
 * The RtlSetAllBits routine sets all bits in a given bitmap variable to one.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP structure that describes the bitmap. This structure must have been initialized by the RtlInitializeBitMap routine.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlsetallbits
 */
NTSYSAPI
VOID
NTAPI
RtlSetAllBits(
    _In_ PRTL_BITMAP BitMapHeader
    );

/**
 * The RtlFindClearBits routine searches for a range of clear bits of a requested size within a bitmap.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP structure that describes the bitmap. This structure must have been initialized by the RtlInitializeBitMap routine.
 * \param NumberToFind Specifies how many contiguous clear bits will satisfy this request.
 * \param HintIndex Specifies a zero-based bit position from which to start looking for a clear bit range of the given size.
 * \return RtlFindClearBits either returns the zero-based starting bit index for a clear bit range of at least the requested size, or it returns 0xFFFFFFFF if it cannot find such a range within the given bitmap.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlfindclearbits
 */
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

/**
 * The RtlFindSetBits routine searches for a range of set bits of a requested size within a bitmap.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP structure that describes the bitmap. This structure must have been initialized by the RtlInitializeBitMap routine.
 * \param NumberToFind Specifies how many contiguous set bits will satisfy this request.
 * \param HintIndex Specifies a zero-based bit position around which to start looking for a set bit range of the given size.
 * \return RtlFindSetBits either returns the zero-based starting bit index for a set bit range of the requested size, or it returns 0xFFFFFFFF if it cannot find such a range within the given bitmap variable.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlfindsetbits
 */
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

/**
 * The RtlFindClearBitsAndSet routine searches for a range of clear bits of a requested size within a bitmap and sets all bits in the range when it has been located.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP structure that describes the bitmap. This structure must have been initialized by the RtlInitializeBitMap routine.
 * \param NumberToFind Specifies how many contiguous clear bits will satisfy this request.
 * \param HintIndex Specifies a zero-based bit position from which to start looking for a clear bit range of the given size.
 * \return RtlFindClearBitsAndSet either returns the zero-based starting bit index for a clear bit range of the requested size that it set, or it returns 0xFFFFFFFF if it cannot find such a range within the given bitmap variable.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlfindclearbitsandset
 */
_Success_(return != -1)
NTSYSAPI
ULONG
NTAPI
RtlFindClearBitsAndSet(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG NumberToFind,
    _In_ ULONG HintIndex
    );

/**
 * The RtlFindSetBitsAndClear routine searches for a range of set bits of a requested size within a bitmap and clears all bits in the range when it has been located.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP structure that describes the bitmap. This structure must have been initialized by the RtlInitializeBitMap routine.
 * \param NumberToFind Specifies how many contiguous set bits will satisfy this request.
 * \param HintIndex Specifies a zero-based bit position around which to start looking for a set bit range of the given size.
 * \return RtlFindSetBitsAndClear either returns the zero-based starting bit index for a set bit range of the requested size that it cleared, or it returns 0xFFFFFFFF if it cannot find such a range within the given bitmap variable.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlfindsetbitsandclear
 */
_Success_(return != -1)
NTSYSAPI
ULONG
NTAPI
RtlFindSetBitsAndClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG NumberToFind,
    _In_ ULONG HintIndex
    );

/**
 * The RtlClearBits routine sets all bits in the specified range of bits in the bitmap to zero.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP structure that describes the bitmap. This structure must have been initialized by the RtlInitializeBitMap routine.
 * \param StartingIndex The index of the first bit in the bit range that is to be cleared. If the bitmap contains N bits, the bits are numbered from 0 to N-1.
 * \param NumberToClear Specifies how many bits to clear. If the bitmap contains N bits, this parameter can be a value in the range 1 to (N - StartingIndex).
 * \remarks If the NumberToClear parameter is zero, RtlClearBits simply returns control without clearing any bits. The sum (StartingIndex + NumberToClear) must not exceed the SizeOfBitMap parameter value specified in the RtlInitializeBitMap call that initialized the bitmap.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlclearbits
 */
NTSYSAPI
VOID
NTAPI
RtlClearBits(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(0, BitMapHeader->SizeOfBitMap - NumberToClear) ULONG StartingIndex,
    _In_range_(0, BitMapHeader->SizeOfBitMap - StartingIndex) ULONG NumberToClear
    );

/**
 * The RtlSetBits routine sets all bits in a given range of a given bitmap variable.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP structure that describes the bitmap. This structure must have been initialized by the RtlInitializeBitMap routine.
 * \param StartingIndex Specifies the start of the bit range to be set. This is a zero-based value indicating the position of the first bit in the range.
 * \param NumberToSet Specifies how many bits to set.
 * \remarks RtlSetBits simply returns control if the input NumberToSet is zero. StartingIndex plus NumberToSet must be less than or equal to BitMapHeader->SizeOfBitMap.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlsetbits
 */
NTSYSAPI
VOID
NTAPI
RtlSetBits(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(0, BitMapHeader->SizeOfBitMap - NumberToSet) ULONG StartingIndex,
    _In_range_(0, BitMapHeader->SizeOfBitMap - StartingIndex) ULONG NumberToSet
    );

/**
 * The RtlFindMostSignificantBit routine returns the zero-based position of the most significant nonzero bit in its parameter.
 *
 * \param Set The 64-bit value to be searched for its most significant nonzero bit.
 * \return The zero-based bit position of the most significant nonzero bit, or -1 if every bit is zero.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlfindmostsignificantbit
 */
NTSYSAPI
CCHAR
NTAPI
RtlFindMostSignificantBit(
    _In_ ULONGLONG Set
    );

/**
 * The RtlFindLeastSignificantBit routine returns the zero-based position of the least significant nonzero bit in its parameter.
 *
 * \param Set The 64-bit value to be searched for its least significant nonzero bit.
 * \return The zero-based bit position of the least significant nonzero bit, or -1 if every bit is zero.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlfindleastsignificantbit
 */
NTSYSAPI
CCHAR
NTAPI
RtlFindLeastSignificantBit(
    _In_ ULONGLONG Set
    );

/**
 * Describes a run of contiguous bits within a bitmap.
 */
typedef struct _RTL_BITMAP_RUN
{
    ULONG StartingIndex;
    ULONG NumberOfBits;
} RTL_BITMAP_RUN, *PRTL_BITMAP_RUN;

/**
 * The RtlFindClearRuns routine finds the specified number of runs of clear bits within a given bitmap.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP structure that describes the bitmap. This structure must have been initialized by the RtlInitializeBitMap routine.
 * \param RunArray Pointer to the first element in a caller-allocated array for the bit position and length of each clear run found in the given bitmap variable.
 * \param SizeOfRunArray Specifies the maximum number of clear runs to satisfy this request.
 * \param LocateLongestRuns If TRUE, specifies that the routine is to search the entire bitmap for the longest clear runs it can find. Otherwise, the routine stops searching when it has found the number of clear runs specified by SizeOfRunArray.
 * \return RtlFindClearRuns returns the number of clear runs found.
 * \remarks If LocateLongestRuns is TRUE, the clear runs indicated at RunArray are sorted from longest to shortest. A clear run can consist of a single bit.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlfindclearruns
 */
NTSYSAPI
ULONG
NTAPI
RtlFindClearRuns(
    _In_ PRTL_BITMAP BitMapHeader,
    _Out_writes_to_(SizeOfRunArray, return) PRTL_BITMAP_RUN RunArray,
    _In_range_(>, 0) ULONG SizeOfRunArray,
    _In_ BOOLEAN LocateLongestRuns
    );

/**
 * The RtlFindLongestRunClear routine searches for the largest contiguous range of clear bits within a given bitmap.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP structure that describes the bitmap. This structure must have been initialized by the RtlInitializeBitMap routine.
 * \param StartingIndex Pointer to a variable in which the starting index of the longest clear run in the bitmap is returned. This is a zero-based value indicating the bit position of the first clear bit in the returned range.
 * \return RtlFindLongestRunClear returns either the number of bits in the run beginning at StartingIndex, or zero if it cannot find a run of clear bits within the bitmap.
 * \remarks A returned run can have a single clear bit.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlfindlongestrunclear
 */
NTSYSAPI
ULONG
NTAPI
RtlFindLongestRunClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _Out_ PULONG StartingIndex
    );

/**
 * The RtlFindFirstRunClear routine searches for the initial contiguous range of clear bits within a given bitmap.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP structure that describes the bitmap. This structure must have been initialized by the RtlInitializeBitMap routine.
 * \param StartingIndex Pointer to a variable in which the starting index of the first clear run in the bitmap is returned. This is a zero-based value indicating the bit position of the first clear bit in the returned range.
 * \return RtlFindFirstRunClear returns either the number of bits in the run beginning at StartingIndex, or zero if it cannot find a run of clear bits within the bitmap.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlfindfirstrunclear
 */
NTSYSAPI
ULONG
NTAPI
RtlFindFirstRunClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _Out_ PULONG StartingIndex
    );

/**
 * The RtlCheckBit routine determines whether a particular bit in a given bitmap variable is clear or set.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP structure that describes the bitmap. This structure must have been initialized by the RtlInitializeBitMap routine.
 * \param BitPosition Specifies which bit to check. This is a zero-based value indicating the position of the bit to be tested.
 * \return RtlCheckBit returns zero if the given bit is clear, or one if the given bit is set.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlcheckbit
 */
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

/**
 * The RtlNumberOfClearBits routine returns a count of the clear bits in a given bitmap variable.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP structure that describes the bitmap. This structure must have been initialized by the RtlInitializeBitMap routine.
 * \return RtlNumberOfClearBits returns the number of bits that are currently clear.
 * \remarks Callers of RtlNumberOfClearBits must be running at IRQL <= APC_LEVEL if the memory that contains the bitmap variable is pageable or the memory at BitMapHeader is pageable. Otherwise, RtlNumberOfClearBits can be called at any IRQL.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlnumberofclearbits
 */
NTSYSAPI
ULONG
NTAPI
RtlNumberOfClearBits(
    _In_ PRTL_BITMAP BitMapHeader
    );

/**
 * The RtlNumberOfSetBits routine returns a count of the set bits in a given bitmap variable.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP structure that describes the bitmap. This structure must have been initialized by the RtlInitializeBitMap routine.
 * \return RtlNumberOfSetBits returns a count of the bits that are currently set.
 * \remarks Callers of RtlNumberOfSetBits must be running at IRQL <= APC_LEVEL if the memory that contains the bitmap variable is pageable or the memory at BitMapHeader is pageable. Otherwise, RtlNumberOfSetBits can be called at any IRQL.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlnumberofsetbits
 */
NTSYSAPI
ULONG
NTAPI
RtlNumberOfSetBits(
    _In_ PRTL_BITMAP BitMapHeader
    );

/**
 * The RtlAreBitsClear routine determines whether a given range of bits within a bitmap variable is clear.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP structure that describes the bitmap. This structure must have been initialized by the RtlInitializeBitMap routine.
 * \param StartingIndex Specifies the start of the bit range to be examined. This is a zero-based value indicating the position of the first bit in the range.
 * \param Length Specifies how many bits to check.
 * \return RtlAreBitsClear returns TRUE if Length contiguous bits starting at StartingIndex are clear (that is, all the bits from StartingIndex to (StartingIndex + Length) -1). It returns FALSE if any bit in the given range is set, if the given range is not a proper subset of the bitmap, or if Length is zero.
 * \remarks Callers of RtlAreBitsClear must be running at IRQL <= APC_LEVEL if the memory that contains the bitmap variable is pageable or the memory at BitMapHeader is pageable. Otherwise, RtlAreBitsClear can be called at any IRQL.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlarebitsclear
 */
_Check_return_
NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG StartingIndex,
    _In_ ULONG Length
    );

/**
 * The RtlAreBitsSet routine determines whether a given range of bits within a bitmap variable is set.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP structure that describes the bitmap. This structure must have been initialized by the RtlInitializeBitMap routine.
 * \param StartingIndex Specifies the start of the bit range to be tested. This is a zero-based value indicating the position of the first bit in the range.
 * \param Length Specifies how many bits to test.
 * \return RtlAreBitsSet returns TRUE if Length consecutive bits beginning at StartingIndex are set (that is, all the bits from StartingIndex to (StartingIndex + Length)). It returns FALSE if any bit in the given range is clear, if the given range is not a proper subset of the bitmap, or if the given Length is zero.
 * \remarks Callers of RtlAreBitsSet must be running at IRQL <= APC_LEVEL if the memory that contains the bitmap variable is pageable or the memory at BitMapHeader is pageable. Otherwise, RtlAreBitsSet can be called at any IRQL.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlarebitsset
 */
_Check_return_
NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsSet(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG StartingIndex,
    _In_ ULONG Length
    );

/**
 * The RtlFindNextForwardRunClear routine searches a given bitmap variable for the next clear run of bits, starting from the specified index position.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP structure that describes the bitmap. This structure must have been initialized by the RtlInitializeBitMap routine.
 * \param FromIndex Specifies a zero-based bit position at which to start looking for a clear run of bits.
 * \param StartingRunIndex Pointer to a variable in which the starting index of the clear run found in the bitmap is returned. This is a zero-based value indicating the bit position of the first clear bit in the run. Its value is meaningless if RtlFindNextForwardRunClear cannot find a run of clear bits.
 * \return RtlFindNextForwardRunClear returns either the number of bits in the run beginning at StartingRunIndex, or zero if it cannot find a run of clear bits following FromIndex in the bitmap.
 * \remarks Callers of RtlFindNextForwardRunClear must be running at IRQL <= APC_LEVEL if the memory that contains the bitmap variable is pageable or the memory at BitMapHeader is pageable. Otherwise, RtlFindNextForwardRunClear can be called at any IRQL.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlfindnextforwardrunclear
 */
NTSYSAPI
ULONG
NTAPI
RtlFindNextForwardRunClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG FromIndex,
    _Out_ PULONG StartingRunIndex
    );

/**
 * The RtlFindLastBackwardRunClear routine searches a given bitmap for the preceding clear run of bits, starting from the specified index position.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP structure that describes the bitmap. This structure must have been initialized by the RtlInitializeBitMap routine.
 * \param FromIndex Specifies a zero-based bit position at which to start looking for a clear run of bits.
 * \param StartingRunIndex Pointer to a variable in which the starting index of the clear run found in the bitmap is returned. This is a zero-based value indicating the bit position of the first clear bit in the run preceding the given FromIndex. Its value is meaningless if RtlFindLastBackwardRunClear cannot find a run of clear bits.
 * \return RtlFindLastBackwardRunClear returns the number of bits in the run beginning at StartingRunIndex, or zero if it cannot find a run of clear bits preceding FromIndex in the bitmap.
 * \remarks Callers of RtlFindLastBackwardRunClear must be running at IRQL <= APC_LEVEL if the memory that contains the bitmap variable is pageable or the memory at BitMapHeader is pageable. Otherwise, RtlFindLastBackwardRunClear can be called at any IRQL.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlfindlastbackwardrunclear
 */
NTSYSAPI
ULONG
NTAPI
RtlFindLastBackwardRunClear(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG FromIndex,
    _Out_ PULONG StartingRunIndex
    );

/**
 * The RtlNumberOfSetBitsUlongPtr routine returns the number of bits set to one in the specified pointer-sized value.
 *
 * \param Target The value whose set bits are counted.
 * \return ULONG The number of bits set to one in Target.
 */
NTSYSAPI
ULONG
NTAPI
RtlNumberOfSetBitsUlongPtr(
    _In_ ULONG_PTR Target
    );

// rev
/**
 * The RtlInterlockedClearBitRun routine atomically clears a run of consecutive bits in a bitmap.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP that describes the bitmap.
 * \param StartingIndex The zero-based index of the first bit to clear.
 * \param NumberToClear The number of consecutive bits to clear.
 */
NTSYSAPI
VOID
NTAPI
RtlInterlockedClearBitRun(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(0, BitMapHeader->SizeOfBitMap - NumberToClear) ULONG StartingIndex,
    _In_range_(0, BitMapHeader->SizeOfBitMap - StartingIndex) ULONG NumberToClear
    );

// rev
/**
 * The RtlInterlockedSetBitRun routine atomically sets a run of consecutive bits in a bitmap.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP that describes the bitmap.
 * \param StartingIndex The zero-based index of the first bit to set.
 * \param NumberToSet The number of consecutive bits to set.
 */
NTSYSAPI
VOID
NTAPI
RtlInterlockedSetBitRun(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_range_(0, BitMapHeader->SizeOfBitMap - NumberToSet) ULONG StartingIndex,
    _In_range_(0, BitMapHeader->SizeOfBitMap - StartingIndex) ULONG NumberToSet
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)

/**
 * The RtlCopyBitMap routine copies the bits of one bitmap into another bitmap starting at a specified target bit.
 *
 * \param Source A pointer to the RTL_BITMAP to copy from.
 * \param Destination A pointer to the RTL_BITMAP to copy into.
 * \param TargetBit The zero-based index in the destination bitmap at which copying begins.
 */
NTSYSAPI
VOID
NTAPI
RtlCopyBitMap(
    _In_ PRTL_BITMAP Source,
    _In_ PRTL_BITMAP Destination,
    _In_range_(0, Destination->SizeOfBitMap - 1) ULONG TargetBit
    );

/**
 * The RtlExtractBitMap routine extracts a range of bits from a source bitmap into a destination bitmap.
 *
 * \param Source A pointer to the RTL_BITMAP to extract from.
 * \param Destination A pointer to the RTL_BITMAP that receives the extracted bits.
 * \param TargetBit The zero-based index of the first bit to extract from the source bitmap.
 * \param NumberOfBits The number of bits to extract.
 */
NTSYSAPI
VOID
NTAPI
RtlExtractBitMap(
    _In_ PRTL_BITMAP Source,
    _In_ PRTL_BITMAP Destination,
    _In_range_(0, Source->SizeOfBitMap - 1) ULONG TargetBit,
    _In_range_(0, Source->SizeOfBitMap) ULONG NumberOfBits
    );

/**
 * The RtlNumberOfClearBitsInRange routine returns the number of clear bits within a specified range of a bitmap.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP that describes the bitmap.
 * \param StartingIndex The zero-based index of the first bit in the range.
 * \param Length The number of bits in the range to examine.
 * \return ULONG The number of clear bits in the specified range.
 */
NTSYSAPI
ULONG
NTAPI
RtlNumberOfClearBitsInRange(
    _In_ PRTL_BITMAP BitMapHeader,
    _In_ ULONG StartingIndex,
    _In_ ULONG Length
    );

/**
 * The RtlNumberOfSetBitsInRange routine returns the number of set bits within a specified range of a bitmap.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP that describes the bitmap.
 * \param StartingIndex The zero-based index of the first bit in the range.
 * \param Length The number of bits in the range to examine.
 * \return ULONG The number of set bits in the specified range.
 */
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
/**
 * Represents a bitmap addressed with 64-bit indices.
 */
typedef struct _RTL_BITMAP_EX
{
    ULONG64 SizeOfBitMap;
    PULONG64 Buffer;
} RTL_BITMAP_EX, *PRTL_BITMAP_EX;

// rev
/**
 * The RtlInitializeBitMapEx routine initializes a 64-bit-capable extended bitmap header over a caller-supplied buffer.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP_EX to initialize.
 * \param BitMapBuffer A pointer to the caller-allocated buffer that stores the bitmap bits.
 * \param SizeOfBitMap The size of the bitmap, in bits.
 */
NTSYSAPI
VOID
NTAPI
RtlInitializeBitMapEx(
    _Out_ PRTL_BITMAP_EX BitMapHeader,
    _In_ PULONG64 BitMapBuffer,
    _In_ ULONG64 SizeOfBitMap
    );

// rev
/**
 * The RtlTestBitEx routine tests whether a specified bit is set in an extended bitmap.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP_EX that describes the bitmap.
 * \param BitNumber The zero-based index of the bit to test.
 * \return Returns `TRUE` if the specified bit is set, otherwise `FALSE`.
 */
_Check_return_
NTSYSAPI
BOOLEAN
NTAPI
RtlTestBitEx(
    _In_ PRTL_BITMAP_EX BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG64 BitNumber
    );

// rev
/**
 * The RtlClearAllBitsEx routine clears every bit in an extended bitmap.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP_EX that describes the bitmap.
 */
NTSYSAPI
VOID
NTAPI
RtlClearAllBitsEx(
    _In_ PRTL_BITMAP_EX BitMapHeader
    );

// rev
/**
 * The RtlClearBitEx routine clears a single bit in an extended bitmap.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP_EX that describes the bitmap.
 * \param BitNumber The zero-based index of the bit to clear.
 */
NTSYSAPI
VOID
NTAPI
RtlClearBitEx(
    _In_ PRTL_BITMAP_EX BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG64 BitNumber
    );

// rev
/**
 * The RtlSetBitEx routine sets a single bit in an extended bitmap.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP_EX that describes the bitmap.
 * \param BitNumber The zero-based index of the bit to set.
 */
NTSYSAPI
VOID
NTAPI
RtlSetBitEx(
    _In_ PRTL_BITMAP_EX BitMapHeader,
    _In_range_(<, BitMapHeader->SizeOfBitMap) ULONG64 BitNumber
    );

/**
 * The RtlSetBitsEx routine sets a run of consecutive bits in an extended bitmap.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP_EX that describes the bitmap.
 * \param StartingIndex The zero-based index of the first bit to set.
 * \param NumberToSet The number of consecutive bits to set.
 */
NTSYSAPI
VOID
NTAPI
RtlSetBitsEx(
    _In_ PRTL_BITMAP_EX BitMapHeader,
    _In_ ULONGLONG StartingIndex,
    _In_ ULONGLONG NumberToSet
    );

// rev
/**
 * The RtlSetAllBitsEx routine sets every bit in an extended bitmap.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP_EX that describes the bitmap.
 */
NTSYSAPI
VOID
NTAPI
RtlSetAllBitsEx(
    _In_ PRTL_BITMAP_EX BitMapHeader
    );

// rev
/**
 * The RtlFindSetBitsEx routine searches an extended bitmap for a run of the specified number of set bits.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP_EX that describes the bitmap.
 * \param NumberToFind The number of consecutive set bits to locate.
 * \param HintIndex The zero-based index at which to begin the search.
 * \return ULONG64 The starting index of the located run, or -1 if no such run exists.
 */
NTSYSAPI
ULONG64
NTAPI
RtlFindSetBitsEx(
    _In_ PRTL_BITMAP_EX BitMapHeader,
    _In_ ULONG64 NumberToFind,
    _In_ ULONG64 HintIndex
    );

/**
 * The RtlFindSetBitsAndClearEx routine searches an extended bitmap for a run of set bits and clears them.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP_EX that describes the bitmap.
 * \param NumberToFind The number of consecutive set bits to locate and clear.
 * \param HintIndex The zero-based index at which to begin the search.
 * \return ULONG64 The starting index of the located run, or -1 if no such run exists.
 */
NTSYSAPI
ULONG64
NTAPI
RtlFindSetBitsAndClearEx(
    _In_ PRTL_BITMAP_EX BitMapHeader,
    _In_ ULONG64 NumberToFind,
    _In_ ULONG64 HintIndex
    );

/**
 * The RtlNumberOfClearBitsEx routine returns the number of clear bits in an extended bitmap.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP_EX that describes the bitmap.
 * \return ULONGLONG The number of clear bits in the bitmap.
 */
NTSYSAPI
ULONGLONG
NTAPI
RtlNumberOfClearBitsEx(
    _In_ PRTL_BITMAP_EX BitMapHeader
    );

/**
 * The RtlFindClearBitsAndSetEx routine searches an extended bitmap for a run of clear bits and sets them.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP_EX that describes the bitmap.
 * \param NumberToFind The number of consecutive clear bits to locate and set.
 * \param HintIndex The zero-based index at which to begin the search.
 * \return ULONGLONG The starting index of the located run, or -1 if no such run exists.
 */
NTSYSAPI
ULONGLONG
NTAPI
RtlFindClearBitsAndSetEx(
    _In_ PRTL_BITMAP_EX BitMapHeader,
    _In_ ULONGLONG NumberToFind,
    _In_ ULONGLONG HintIndex
    );

/**
 * The RtlFindClearBitsEx routine searches an extended bitmap for a run of the specified number of clear bits.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP_EX that describes the bitmap.
 * \param NumberToFind The number of consecutive clear bits to locate.
 * \param HintIndex The zero-based index at which to begin the search.
 * \return ULONGLONG The starting index of the located run, or -1 if no such run exists.
 */
NTSYSAPI
ULONGLONG
NTAPI
RtlFindClearBitsEx(
    _In_ PRTL_BITMAP_EX BitMapHeader,
    _In_ ULONGLONG NumberToFind,
    _In_ ULONGLONG HintIndex
    );

/**
 * The RtlClearBitsEx routine clears a run of consecutive bits in an extended bitmap.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP_EX that describes the bitmap.
 * \param StartingIndex The zero-based index of the first bit to clear.
 * \param NumberToClear The number of consecutive bits to clear.
 */
NTSYSAPI
VOID
NTAPI
RtlClearBitsEx(
    _In_ PRTL_BITMAP_EX BitMapHeader,
    _In_ ULONGLONG StartingIndex,
    _In_ ULONGLONG NumberToClear
    );

/**
 * The RtlNumberOfSetBitsEx routine returns the number of set bits in an extended bitmap.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP_EX that describes the bitmap.
 * \return ULONGLONG The number of set bits in the bitmap.
 */
NTSYSAPI
ULONGLONG
NTAPI
RtlNumberOfSetBitsEx(
    _In_ PRTL_BITMAP_EX BitMapHeader
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_10

// rev
/**
 * The RtlInterlockedClearBitRunEx routine atomically clears a run of consecutive bits in an extended bitmap.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP_EX that describes the bitmap.
 * \param StartingIndex The zero-based index of the first bit to clear.
 * \param NumberToClear The number of consecutive bits to clear.
 */
NTSYSAPI
VOID
NTAPI
RtlInterlockedClearBitRunEx(
    _In_ PRTL_BITMAP_EX BitMapHeader,
    _In_range_(0, BitMapHeader->SizeOfBitMap - NumberToClear) ULONG64 StartingIndex,
    _In_range_(0, BitMapHeader->SizeOfBitMap - StartingIndex) ULONG64 NumberToClear
    );

// rev
/**
 * The RtlLengthCurrentClearRunBackwardEx routine computes the length of a run of clear (zero) bits in an extended bitmap starting from a given index and searching backward.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP_EX that describes the bitmap.
 * \param StartingIndex The zero-based index from which to begin searching backward.
 * \param MaximumLength The maximum number of bits to check.
 * \return The number of contiguous clear bits found backward from StartingIndex, up to MaximumLength.
 */
NTSYSAPI
ULONGLONG
NTAPI
RtlLengthCurrentClearRunBackwardEx(
    _In_ PRTL_BITMAP_EX BitMapHeader,
    _In_ ULONGLONG StartingIndex,
    _In_ ULONGLONG MaximumLength
    );

// rev
/**
 * The RtlLengthCurrentClearRunForwardEx routine computes the length of a run of clear (zero) bits in an extended bitmap starting from a given index and searching forward.
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP_EX that describes the bitmap.
 * \param StartingIndex The zero-based index from which to begin searching forward.
 * \param MaximumLength The maximum number of bits to check.
 * \return The number of contiguous clear bits found forward from StartingIndex, up to MaximumLength.
 */
NTSYSAPI
ULONGLONG
NTAPI
RtlLengthCurrentClearRunForwardEx(
    _In_ PRTL_BITMAP_EX BitMapHeader,
    _In_ ULONGLONG StartingIndex,
    _In_ ULONGLONG MaximumLength
    );

// rev
/**
 * The RtlAreBitsClearEx routine determines whether all bits within a specified range of an extended bitmap are clear (zero).
 *
 * \param BitMapHeader A pointer to the RTL_BITMAP_EX that describes the bitmap.
 * \param StartingIndex The zero-based index of the first bit to examine.
 * \param Length The number of bits to check.
 * \return TRUE if all bits in the specified range are clear; otherwise, FALSE.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsClearEx(
    _In_ PRTL_BITMAP_EX BitMapHeader,
    _In_ ULONGLONG StartingIndex,
    _In_ ULONGLONG Length
    );

//
// Handle tables
//

/**
 * Represents a single entry in an RTL handle table.
 */
typedef struct _RTL_HANDLE_TABLE_ENTRY
{
    union
    {
        ULONG Flags; // allocated entries have the low bit set
        struct _RTL_HANDLE_TABLE_ENTRY *NextFree;
    };
} RTL_HANDLE_TABLE_ENTRY, *PRTL_HANDLE_TABLE_ENTRY;

/**
 * Flag marking an RTL handle table entry as allocated.
 */
#define RTL_HANDLE_ALLOCATED (USHORT)0x0001

/**
 * Represents a user-mode handle table managed by the RTL handle package.
 */
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

/**
 * The RtlInitializeHandleTable routine initializes a handle table for allocating and tracking fixed-size handle entries.
 *
 * \param MaximumNumberOfHandles The maximum number of handles the table can contain.
 * \param SizeOfHandleTableEntry The size, in bytes, of each handle table entry.
 * \param HandleTable A pointer to the RTL_HANDLE_TABLE to initialize.
 */
NTSYSAPI
VOID
NTAPI
RtlInitializeHandleTable(
    _In_ ULONG MaximumNumberOfHandles,
    _In_ ULONG SizeOfHandleTableEntry,
    _Out_ PRTL_HANDLE_TABLE HandleTable
    );

/**
 * The RtlDestroyHandleTable routine destroys a handle table and releases the memory it allocated.
 *
 * \param HandleTable A pointer to the RTL_HANDLE_TABLE to destroy.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyHandleTable(
    _Inout_ PRTL_HANDLE_TABLE HandleTable
    );

/**
 * The RtlAllocateHandle routine allocates a handle from a handle table.
 *
 * \param HandleTable A pointer to the RTL_HANDLE_TABLE from which to allocate.
 * \param HandleIndex An optional pointer to a variable that receives the index of the allocated handle.
 * \return PRTL_HANDLE_TABLE_ENTRY A pointer to the allocated handle table entry, or `NULL` on failure.
 */
NTSYSAPI
PRTL_HANDLE_TABLE_ENTRY
NTAPI
RtlAllocateHandle(
    _In_ PRTL_HANDLE_TABLE HandleTable,
    _Out_opt_ PULONG HandleIndex
    );

/**
 * The RtlFreeHandle routine returns a previously allocated handle to a handle table.
 *
 * \param HandleTable A pointer to the RTL_HANDLE_TABLE that owns the handle.
 * \param Handle A pointer to the handle table entry to free.
 * \return Returns `TRUE` if the handle was freed successfully, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlFreeHandle(
    _In_ PRTL_HANDLE_TABLE HandleTable,
    _In_ PRTL_HANDLE_TABLE_ENTRY Handle
    );

/**
 * The RtlIsValidHandle routine determines whether a handle table entry is currently valid and allocated.
 *
 * \param HandleTable A pointer to the RTL_HANDLE_TABLE that owns the handle.
 * \param Handle A pointer to the handle table entry to validate.
 * \return Returns `TRUE` if the handle is valid, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsValidHandle(
    _In_ PRTL_HANDLE_TABLE HandleTable,
    _In_ PRTL_HANDLE_TABLE_ENTRY Handle
    );

/**
 * The RtlIsValidIndexHandle routine determines whether a handle identified by index is valid and returns the corresponding entry.
 *
 * \param HandleTable A pointer to the RTL_HANDLE_TABLE that owns the handle.
 * \param HandleIndex The zero-based index of the handle to validate.
 * \param Handle A pointer to a variable that receives the handle table entry for the index.
 * \return Returns `TRUE` if the indexed handle is valid, otherwise `FALSE`.
 */
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

/**
 * Constants describing atom table limits and reserved values.
 */
#define RTL_ATOM_MAXIMUM_INTEGER_ATOM (RTL_ATOM)0xc000
#define RTL_ATOM_INVALID_ATOM (RTL_ATOM)0x0000
#define RTL_ATOM_TABLE_DEFAULT_NUMBER_OF_BUCKETS 37
#define RTL_ATOM_MAXIMUM_NAME_LENGTH 255
#define RTL_ATOM_PINNED 0x01

/**
 * The RtlCreateAtomTable routine creates an atom table.
 *
 * \param NumberOfBuckets The number of hash buckets; zero selects a default.
 * \param AtomTableHandle Receives the newly created atom table.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateAtomTable(
    _In_ ULONG NumberOfBuckets,
    _Inout_ PVOID *AtomTableHandle
    );

/**
 * The RtlDestroyAtomTable routine destroys an atom table.
 *
 * \param AtomTableHandle The atom table to destroy.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyAtomTable(
    _In_ _Post_invalid_ PVOID AtomTableHandle
    );

/**
 * The RtlEmptyAtomTable routine removes atoms from an atom table.
 *
 * \param AtomTableHandle The atom table to empty.
 * \param IncludePinnedAtoms TRUE to also remove pinned atoms; FALSE to retain them.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlEmptyAtomTable(
    _In_ PVOID AtomTableHandle,
    _In_ BOOLEAN IncludePinnedAtoms
    );

/**
 * The RtlAddAtomToAtomTable routine adds a named atom to an atom table.
 *
 * \param AtomTableHandle The atom table to modify.
 * \param AtomName The name of the atom to add.
 * \param Atom On input may specify an integer atom; receives the resulting atom.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAddAtomToAtomTable(
    _In_ PVOID AtomTableHandle,
    _In_ PCWSTR AtomName,
    _Inout_opt_ PRTL_ATOM Atom
    );

/**
 * The RtlLookupAtomInAtomTable routine looks up a named atom in an atom table.
 *
 * \param AtomTableHandle The atom table to search.
 * \param AtomName The name of the atom to find.
 * \param Atom An optional pointer that receives the atom.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlLookupAtomInAtomTable(
    _In_ PVOID AtomTableHandle,
    _In_ PCWSTR AtomName,
    _Out_opt_ PRTL_ATOM Atom
    );

/**
 * The RtlDeleteAtomFromAtomTable routine removes an atom from an atom table.
 *
 * \param AtomTableHandle The atom table to modify.
 * \param Atom The atom to remove.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteAtomFromAtomTable(
    _In_ PVOID AtomTableHandle,
    _In_ RTL_ATOM Atom
    );

/**
 * The RtlPinAtomInAtomTable routine pins an atom in an atom table so that it cannot be deleted by reference counting.
 *
 * \param AtomTableHandle The atom table containing the atom.
 * \param Atom The atom to pin.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlPinAtomInAtomTable(
    _In_ PVOID AtomTableHandle,
    _In_ RTL_ATOM Atom
    );

/**
 * The RtlQueryAtomInAtomTable routine retrieves information about an atom in an atom table.
 *
 * \param AtomTableHandle The atom table to query.
 * \param Atom The atom to query.
 * \param AtomUsage An optional pointer that receives the atom reference count.
 * \param AtomFlags An optional pointer that receives the atom flags.
 * \param AtomName An optional buffer that receives the atom name.
 * \param AtomNameLength On input specifies the buffer size; on output receives the name length.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlGetIntegerAtom routine converts an atom name string into its integer atom value.
 *
 * \param AtomName A pointer to the null-terminated atom name to convert.
 * \param IntegerAtom An optional pointer to a variable that receives the integer atom value.
 * \return Returns `TRUE` if the name represents a valid integer atom, otherwise `FALSE`.
 */
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

/**
 * The RtlValidSid routine validates a security identifier (SID) by verifying that its revision level and subauthority count are within valid ranges.
 *
 * \param Sid A pointer to the SID to validate.
 * \return Returns `TRUE` if the SID is structurally valid, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlvalidsid
 */
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlValidSid(
    _In_ PSID Sid
    );

/**
 * The RtlEqualSid routine determines whether two security identifiers (SIDs) are equal.
 *
 * \param Sid1 A pointer to the first SID to compare.
 * \param Sid2 A pointer to the second SID to compare.
 * \return Returns `TRUE` if the two SIDs are equal, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlequalsid
 */
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualSid(
    _In_ PSID Sid1,
    _In_ PSID Sid2
    );

/**
 * The RtlEqualPrefixSid routine determines whether two security identifiers (SIDs) share the same prefix, ignoring the final subauthority (relative identifier).
 *
 * \param Sid1 A pointer to the first SID to compare.
 * \param Sid2 A pointer to the second SID to compare.
 * \return Returns `TRUE` if the SID prefixes are equal, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlequalprefixsid
 */
_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualPrefixSid(
    _In_ PSID Sid1,
    _In_ PSID Sid2
    );

/**
 * The RtlLengthRequiredSid routine returns the length, in bytes, required to store a security identifier (SID) with the specified number of subauthorities.
 *
 * \param SubAuthorityCount The number of subauthorities the SID will contain.
 * \return ULONG The number of bytes required to hold the SID.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtllengthrequiredsid
 */
NTSYSAPI
ULONG
NTAPI
RtlLengthRequiredSid(
    _In_ ULONG SubAuthorityCount
    );

/**
 * The RtlFreeSid routine frees a SID previously allocated with RtlAllocateAndInitializeSid.
 *
 * \param Sid The SID to free.
 * \return NULL.
 */
NTSYSAPI
PVOID
NTAPI
RtlFreeSid(
    _In_ _Post_invalid_ PSID Sid
    );

/**
 * The RtlAllocateAndInitializeSid routine allocates and initializes a security identifier (SID) with up to eight subauthorities.
 *
 * \param IdentifierAuthority A pointer to the SID_IDENTIFIER_AUTHORITY for the new SID.
 * \param SubAuthorityCount The number of subauthorities to place in the SID.
 * \param SubAuthority0 The first subauthority value.
 * \param SubAuthority1 The second subauthority value.
 * \param SubAuthority2 The third subauthority value.
 * \param SubAuthority3 The fourth subauthority value.
 * \param SubAuthority4 The fifth subauthority value.
 * \param SubAuthority5 The sixth subauthority value.
 * \param SubAuthority6 The seventh subauthority value.
 * \param SubAuthority7 The eighth subauthority value.
 * \param Sid A pointer to a variable that receives the allocated SID.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlallocateandinitializesid
 */
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
/**
 * The RtlAllocateAndInitializeSidEx routine allocates and initializes a security identifier (SID) from an array of subauthorities.
 *
 * \param IdentifierAuthority A pointer to the SID_IDENTIFIER_AUTHORITY for the new SID.
 * \param SubAuthorityCount The number of subauthorities to place in the SID.
 * \param SubAuthorities A pointer to an array of subauthority values.
 * \param Sid A pointer to a variable that receives the allocated SID.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlallocateandinitializesidex
 */
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

/**
 * The RtlInitializeSid routine initializes a caller-allocated security identifier (SID) structure.
 *
 * \param Sid A pointer to the SID buffer to initialize.
 * \param IdentifierAuthority A pointer to the SID_IDENTIFIER_AUTHORITY for the SID.
 * \param SubAuthorityCount The number of subauthorities the SID will contain.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlinitializesid
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeSid(
    _Out_ PSID Sid,
    _In_ PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    _In_ UCHAR SubAuthorityCount
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
/**
 * The RtlInitializeSidEx routine initializes a caller-allocated security identifier (SID) using a variable number of subauthority arguments.
 *
 * \param Sid A pointer to the SID buffer to initialize.
 * \param IdentifierAuthority A pointer to the SID_IDENTIFIER_AUTHORITY for the SID.
 * \param SubAuthorityCount The number of subauthority arguments that follow.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlinitializesidex
 */
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

/**
 * The RtlIdentifierAuthoritySid routine returns a pointer to the identifier authority of a security identifier (SID).
 *
 * \param Sid A pointer to the SID to query.
 * \return PSID_IDENTIFIER_AUTHORITY A pointer to the SID_IDENTIFIER_AUTHORITY within the SID.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlidentifierauthoritysid
 */
NTSYSAPI
PSID_IDENTIFIER_AUTHORITY
NTAPI
RtlIdentifierAuthoritySid(
    _In_ PSID Sid
    );

/**
 * The RtlSubAuthoritySid routine returns a pointer to a specified subauthority of a security identifier (SID).
 *
 * \param Sid A pointer to the SID to query.
 * \param SubAuthority The zero-based index of the subauthority to retrieve.
 * \return PULONG A pointer to the requested subauthority value within the SID.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlsubauthoritysid
 */
NTSYSAPI
PULONG
NTAPI
RtlSubAuthoritySid(
    _In_ PSID Sid,
    _In_ ULONG SubAuthority
    );

/**
 * The RtlSubAuthorityCountSid routine returns a pointer to the subauthority count field of a security identifier (SID).
 *
 * \param Sid A pointer to the SID to query.
 * \return PUCHAR A pointer to the subauthority count field within the SID.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlsubauthoritycountsid
 */
NTSYSAPI
PUCHAR
NTAPI
RtlSubAuthorityCountSid(
    _In_ PSID Sid
    );

/**
 * The RtlLengthSid routine returns the length, in bytes, of a security identifier (SID).
 *
 * \param Sid A pointer to the SID to measure.
 * \return ULONG The length of the SID, in bytes.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtllengthsid
 */
NTSYSAPI
ULONG
NTAPI
RtlLengthSid(
    _In_ PSID Sid
    );

/**
 * The RtlCopySid routine copies a security identifier (SID) into a caller-supplied buffer.
 *
 * \param DestinationSidLength The length, in bytes, of the destination buffer.
 * \param DestinationSid A pointer to the buffer that receives the copied SID.
 * \param SourceSid A pointer to the SID to copy.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlcopysid
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCopySid(
    _In_ ULONG DestinationSidLength,
    _Out_writes_bytes_(DestinationSidLength) PSID DestinationSid,
    _In_ PSID SourceSid
    );

// ros
/**
 * The RtlCopySidAndAttributesArray routine copies an array of SID_AND_ATTRIBUTES entries, placing the copied SIDs into a separate SID area.
 *
 * \param Count The number of SID_AND_ATTRIBUTES entries to copy.
 * \param Src A pointer to the source array of SID_AND_ATTRIBUTES entries.
 * \param SidAreaSize The size, in bytes, of the buffer that receives the copied SIDs.
 * \param Dest A pointer to the destination array of SID_AND_ATTRIBUTES entries.
 * \param SidArea A pointer to the buffer that receives the copied SIDs.
 * \param RemainingSidArea A pointer to a variable that receives a pointer to the unused portion of the SID area.
 * \param RemainingSidAreaSize A pointer to a variable that receives the size, in bytes, of the unused portion of the SID area.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlCreateServiceSid routine creates a service security identifier (SID) derived from a service name.
 *
 * \param ServiceName A pointer to the Unicode string that specifies the service name.
 * \param ServiceSid A pointer to a caller-allocated buffer that receives the service SID.
 * \param ServiceSidLength A pointer to a variable that on input specifies the buffer size and on output receives the required or written length, in bytes.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlcreateservicesid
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateServiceSid(
    _In_ PCUNICODE_STRING ServiceName,
    _Out_writes_bytes_opt_(*ServiceSidLength) PSID ServiceSid,
    _Inout_ PULONG ServiceSidLength
    );

// private
/**
 * The RtlSidDominates routine determines whether one integrity-level security identifier (SID) dominates another.
 *
 * \param Sid1 A pointer to the first SID.
 * \param Sid2 A pointer to the second SID.
 * \param Dominates A pointer to a variable that receives `TRUE` if Sid1 dominates Sid2, otherwise `FALSE`.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlSidDominatesForTrust routine determines whether a security identifier (SID) dominates a process trust level SID.
 *
 * \param Sid1 A pointer to the first SID.
 * \param Sid2 A pointer to the second SID.
 * \param DominatesTrust A pointer to a variable that receives `TRUE` if Sid1 dominates the trust SID, otherwise `FALSE`.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlSidEqualLevel routine determines whether two integrity-level security identifiers (SIDs) represent the same level.
 *
 * \param Sid1 A pointer to the first SID.
 * \param Sid2 A pointer to the second SID.
 * \param EqualLevel A pointer to a variable that receives `TRUE` if the SIDs are at the same level, otherwise `FALSE`.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSidEqualLevel(
    _In_ PSID Sid1,
    _In_ PSID Sid2,
    _Out_ PBOOLEAN EqualLevel
    );

// private
/**
 * The RtlSidIsHigherLevel routine determines whether one integrity-level security identifier (SID) represents a higher level than another.
 *
 * \param Sid1 A pointer to the first SID.
 * \param Sid2 A pointer to the second SID.
 * \param HigherLevel A pointer to a variable that receives `TRUE` if Sid1 is at a higher level than Sid2, otherwise `FALSE`.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSidIsHigherLevel(
    _In_ PSID Sid1,
    _In_ PSID Sid2,
    _Out_ PBOOLEAN HigherLevel
    );

/**
 * The RtlCreateVirtualAccountSid routine creates a virtual account security identifier (SID) from a name and base subauthority.
 *
 * \param Name A pointer to the Unicode string that specifies the virtual account name.
 * \param BaseSubAuthority The base subauthority value that identifies the virtual account domain.
 * \param Sid A pointer to a caller-allocated buffer that receives the virtual account SID.
 * \param SidLength A pointer to a variable that on input specifies the buffer size and on output receives the required or written length, in bytes.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateVirtualAccountSid(
    _In_ PCUNICODE_STRING Name,
    _In_ ULONG BaseSubAuthority,
    _Out_writes_bytes_(*SidLength) PSID Sid,
    _Inout_ PULONG SidLength
    );

/**
 * The RtlReplaceSidInSd routine replaces every occurrence of a security identifier (SID) in a security descriptor with a new SID.
 *
 * \param SecurityDescriptor A pointer to the security descriptor to modify.
 * \param OldSid A pointer to the SID to be replaced.
 * \param NewSid A pointer to the replacement SID.
 * \param NumChanges A pointer to a variable that receives the number of replacements performed.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlreplacesidinsd
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlReplaceSidInSd(
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ PSID OldSid,
    _In_ PSID NewSid,
    _Out_ ULONG *NumChanges
    );

/**
 * Maximum length of a Unicode string held in a stack buffer.
 */
#define MAX_UNICODE_STACK_BUFFER_LENGTH 256

/**
 * The RtlLengthSidAsUnicodeString routine returns the length, in bytes, of the Unicode string representation of a security identifier (SID).
 *
 * \param Sid A pointer to the SID to measure.
 * \param StringLength A pointer to a variable that receives the required string length, in bytes.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlLengthSidAsUnicodeString(
    _In_ PSID Sid,
    _Out_ PULONG StringLength
    );

/**
 * The RtlConvertSidToUnicodeString routine converts a security identifier (SID) into its Unicode string representation.
 *
 * \param UnicodeString A pointer to a UNICODE_STRING that receives the string representation. If AllocateDestinationString is `TRUE`, the routine allocates the buffer; otherwise the caller supplies it.
 * \param Sid A pointer to the SID to convert.
 * \param AllocateDestinationString If `TRUE`, the routine allocates the destination string buffer; if `FALSE`, it uses the caller-supplied buffer.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlconvertsidtounicodestring
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlConvertSidToUnicodeString(
    _When_(AllocateDestinationString, _Out_) _When_(!AllocateDestinationString, _In_) PUNICODE_STRING UnicodeString,
    _In_ PSID Sid,
    _In_ BOOLEAN AllocateDestinationString
    );

// private
/**
 * The RtlSidHashInitialize routine initializes a SID_AND_ATTRIBUTES_HASH structure for fast lookup of an array of SIDs.
 *
 * \param SidAttr A pointer to the array of SID_AND_ATTRIBUTES entries to hash.
 * \param SidCount The number of entries in the SidAttr array.
 * \param SidAttrHash A pointer to the SID_AND_ATTRIBUTES_HASH structure to initialize.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSidHashInitialize(
    _In_reads_(SidCount) PSID_AND_ATTRIBUTES SidAttr,
    _In_ ULONG SidCount,
    _Out_ PSID_AND_ATTRIBUTES_HASH SidAttrHash
    );

// private
/**
 * The RtlSidHashLookup routine looks up a security identifier (SID) in a previously initialized SID hash.
 *
 * \param SidAttrHash A pointer to the SID_AND_ATTRIBUTES_HASH to search.
 * \param Sid A pointer to the SID to find.
 * \return PSID_AND_ATTRIBUTES A pointer to the matching SID_AND_ATTRIBUTES entry, or `NULL` if not found.
 */
NTSYSAPI
PSID_AND_ATTRIBUTES
NTAPI
RtlSidHashLookup(
    _In_ PSID_AND_ATTRIBUTES_HASH SidAttrHash,
    _In_ PSID Sid
    );

// rev
/**
 * The RtlIsElevatedRid routine determines whether a SID_AND_ATTRIBUTES entry represents the elevated relative identifier (RID).
 *
 * \param SidAttr A pointer to the SID_AND_ATTRIBUTES entry to test.
 * \return Returns `TRUE` if the entry represents an elevated RID, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsElevatedRid(
    _In_ PSID_AND_ATTRIBUTES SidAttr
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
// rev
/**
 * The RtlDeriveCapabilitySidsFromName routine derives the capability group SID and capability SID that correspond to a capability name.
 *
 * \param UnicodeString A pointer to the Unicode string that specifies the capability name.
 * \param CapabilityGroupSid A pointer to a caller-allocated buffer that receives the capability group SID.
 * \param CapabilitySid A pointer to a caller-allocated buffer that receives the capability SID.
 * \return NTSTATUS Successful or errant status.
 */
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
 * \param SecurityDescriptor Pointer to the buffer for the \ref SECURITY_DESCRIPTOR to be initialized.
 * \param Revision Specifies the revision level to assign to the security descriptor. Set this parameter to SECURITY_DESCRIPTOR_REVISION.
 * \return NTSTATUS Successful or errant status.
 * \see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlcreatesecuritydescriptor
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
 * \param SecurityDescriptor Pointer to the \ref SECURITY_DESCRIPTOR to be checked.
 * \return Returns TRUE if the security descriptor is valid, or FALSE otherwise.
 * \remarks The routine checks the validity of an absolute-format security descriptor. To check the validity of a self-relative security descriptor, use the \ref RtlValidRelativeSecurityDescriptor routine instead.
 * \see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlvalidsecuritydescriptor
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
 * \param SecurityDescriptor A pointer to a \ref SECURITY_DESCRIPTOR structure whose length the function retrieves.
 * \return Returns the length, in bytes, of the SECURITY_DESCRIPTOR structure.
 * \see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtllengthsecuritydescriptor
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
 * \param SecurityDescriptorInput A pointer to the buffer that contains the security descriptor in self-relative format.
 * The buffer must begin with a SECURITY_DESCRIPTOR structure, which is followed by the rest of the security descriptor data.
 * \param SecurityDescriptorLength The size of the SecurityDescriptorInput structure.
 * \param RequiredInformation A SECURITY_INFORMATION value that specifies the information that is required to be contained in the security descriptor.
 * \return RtlValidRelativeSecurityDescriptor returns TRUE if the security descriptor is valid and includes the information that the RequiredInformation parameter specifies. Otherwise, this routine returns FALSE.
 * \see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlvalidrelativesecuritydescriptor
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

/**
 * The RtlGetControlSecurityDescriptor routine retrieves the control information and revision of a security descriptor.
 *
 * \param SecurityDescriptor A pointer to the security descriptor to query.
 * \param Control A pointer to a variable that receives the SECURITY_DESCRIPTOR_CONTROL flags.
 * \param Revision A pointer to a variable that receives the revision of the security descriptor.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlgetcontrolsecuritydescriptor
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetControlSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ PSECURITY_DESCRIPTOR_CONTROL Control,
    _Out_ PULONG Revision
    );

/**
 * The RtlSetControlSecurityDescriptor routine sets selected control bits of a security descriptor.
 *
 * \param SecurityDescriptor A pointer to the security descriptor to modify.
 * \param ControlBitsOfInterest A mask identifying the control bits to modify.
 * \param ControlBitsToSet The values to assign to the control bits identified by ControlBitsOfInterest.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlsetcontrolsecuritydescriptor
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetControlSecurityDescriptor(
     _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
     _In_ SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
     _In_ SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet
     );

/**
 * The RtlSetAttributesSecurityDescriptor routine sets the control attributes of a security descriptor and returns its revision.
 *
 * \param SecurityDescriptor A pointer to the security descriptor to modify.
 * \param Control The SECURITY_DESCRIPTOR_CONTROL attributes to set.
 * \param Revision A pointer to a variable that receives the revision of the security descriptor.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlsetattributessecuritydescriptor
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetAttributesSecurityDescriptor(
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ SECURITY_DESCRIPTOR_CONTROL Control,
    _Out_ PULONG Revision
    );

/**
 * The RtlGetSecurityDescriptorRMControl routine retrieves the resource manager control byte from a security descriptor.
 *
 * \param SecurityDescriptor A pointer to the security descriptor to query.
 * \param RMControl A pointer to a variable that receives the resource manager control byte.
 * \return Returns `TRUE` if the security descriptor contains a resource manager control byte, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlGetSecurityDescriptorRMControl(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ PUCHAR RMControl
    );

/**
 * The RtlSetSecurityDescriptorRMControl routine sets or clears the resource manager control byte of a security descriptor.
 *
 * \param SecurityDescriptor A pointer to the security descriptor to modify.
 * \param RMControl An optional pointer to the resource manager control byte to set. If `NULL`, the resource manager control is cleared.
 */
NTSYSAPI
VOID
NTAPI
RtlSetSecurityDescriptorRMControl(
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PUCHAR RMControl
    );

/**
 * The RtlSetDaclSecurityDescriptor routine sets the discretionary access control list (DACL) of an absolute-format security descriptor.
 *
 * \param SecurityDescriptor A pointer to the security descriptor to modify.
 * \param DaclPresent If `TRUE`, the security descriptor is marked as containing a DACL.
 * \param Dacl An optional pointer to the DACL to assign to the security descriptor.
 * \param DaclDefaulted If `TRUE`, the DACL was obtained by a default mechanism.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlsetdaclsecuritydescriptor
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetDaclSecurityDescriptor(
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ BOOLEAN DaclPresent,
    _In_opt_ PACL Dacl,
    _In_ BOOLEAN DaclDefaulted
    );

/**
 * The RtlGetDaclSecurityDescriptor routine retrieves the discretionary access control list (DACL) of a security descriptor.
 *
 * \param SecurityDescriptor A pointer to the security descriptor to query.
 * \param DaclPresent A pointer to a variable that receives `TRUE` if the security descriptor contains a DACL.
 * \param Dacl A pointer to a variable that receives a pointer to the DACL, or `NULL` if none is present.
 * \param DaclDefaulted A pointer to a variable that receives `TRUE` if the DACL was obtained by a default mechanism.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlgetdaclsecuritydescriptor
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetDaclSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Out_ PBOOLEAN DaclPresent,
    _Outptr_result_maybenull_ PACL *Dacl,
    _Out_ PBOOLEAN DaclDefaulted
    );

/**
 * The RtlSetSaclSecurityDescriptor routine sets the system access control list (SACL) of an absolute-format security descriptor.
 *
 * \param SecurityDescriptor A pointer to the security descriptor to modify.
 * \param SaclPresent If `TRUE`, the security descriptor is marked as containing a SACL.
 * \param Sacl An optional pointer to the SACL to assign to the security descriptor.
 * \param SaclDefaulted If `TRUE`, the SACL was obtained by a default mechanism.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlsetsaclsecuritydescriptor
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetSaclSecurityDescriptor(
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ BOOLEAN SaclPresent,
    _In_opt_ PACL Sacl,
    _In_ BOOLEAN SaclDefaulted
    );

/**
 * The RtlGetSaclSecurityDescriptor routine retrieves the system access control list (SACL) of a security descriptor.
 *
 * \param SecurityDescriptor A pointer to the security descriptor to query.
 * \param SaclPresent A pointer to a variable that receives `TRUE` if the security descriptor contains a SACL.
 * \param Sacl A pointer to a variable that receives a pointer to the SACL, or `NULL` if none is present.
 * \param SaclDefaulted A pointer to a variable that receives `TRUE` if the SACL was obtained by a default mechanism.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlgetsaclsecuritydescriptor
 */
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
 * \param SecurityDescriptor Pointer to the SECURITY_DESCRIPTOR structure whose owner is to be set. RtlSetOwnerSecurityDescriptor replaces any existing owner with the new owner.
 * \param Owner Pointer to a security identifier (SID) structure for the security descriptor's new primary owner.
 * \li \c This pointer, not the SID structure itself, is copied into the security descriptor.
 * \li \c If this parameter is NULL, RtlSetOwnerSecurityDescriptor clears the security descriptor's owner information. This marks the security descriptor as having no owner.
 * \param OwnerDefaulted Set to TRUE if the owner information is derived from a default mechanism.
 * \li \c If this value is TRUE, it is default information. RtlSetOwnerSecurityDescriptor sets the SE_OWNER_DEFAULTED flag in the security descriptor's SECURITY_DESCRIPTOR_CONTROL field.
 * \li \c If this parameter is FALSE, the SE_OWNER_DEFAULTED flag is cleared.
 * \return NTSTATUS Successful or errant status.
 * \see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlsetownersecuritydescriptor
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
 * \param SecurityDescriptor Pointer to the SECURITY_DESCRIPTOR structure.
 * \param Owner Pointer to an address to receive a pointer to the owner security identifier (SID). If the security descriptor does not currently contain an owner SID, Owner receives NULL.
 * \param OwnerDefaulted Pointer to a Boolean variable that receives TRUE if the owner information is derived from a default mechanism, FALSE otherwise. Valid only if Owner receives a non-NULL value.
 * \return NTSTATUS Successful or errant status.
 * \see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlgetownersecuritydescriptor
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
 * \param SecurityDescriptor Pointer to the SECURITY_DESCRIPTOR structure whose primary group is to be set. RtlSetGroupSecurityDescriptor replaces any existing primary group with the new primary group.
 * \param Group Pointer to a security identifier (SID) structure for the security descriptor's new primary owner.
 * \li \c This pointer, not the SID structure itself, is copied into the security descriptor.
 * \li \c If Group is NULL, RtlSetGroupSecurityDescriptor clears the security descriptor's primary group information. This marks the security descriptor as having no primary group.
 * \param GroupDefaulted Set this Boolean variable to TRUE if the primary group information is derived from a default mechanism.
 * \li \c If this parameter is TRUE, RtlSetGroupSecurityDescriptor sets the SE_GROUP_DEFAULTED flag in the security descriptor's SECURITY_DESCRIPTOR_CONTROL field.
 * \li \c If this parameter is FALSE, RtlSetGroupSecurityDescriptor clears the SE_GROUP_DEFAULTED flag.
 * \return NTSTATUS Successful or errant status.
 * \see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlsetgroupsecuritydescriptor
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetGroupSecurityDescriptor(
    _Inout_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID Group,
    _In_ BOOLEAN GroupDefaulted
    );

/**
 * The RtlGetGroupSecurityDescriptor routine returns the primary group information for a given security descriptor.
 *
 * \param SecurityDescriptor Pointer to the security descriptor whose primary group information is to be returned.
 * \param Group Pointer to a variable that receives a pointer to the security identifier (SID) for the primary group.
 * \param GroupDefaulted Pointer to a Boolean variable that receives the value of the SE_GROUP_DEFAULTED flag.
 * \return NTSTATUS Successful or errant status.
 * \see https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlgetgroupsecuritydescriptor
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetGroupSecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _Outptr_result_maybenull_ PSID *Group,
    _Out_ PBOOLEAN GroupDefaulted
    );

/**
 * The RtlMakeSelfRelativeSD routine creates a self-relative security descriptor from an absolute-format security descriptor.
 *
 * \param AbsoluteSecurityDescriptor A pointer to the absolute-format security descriptor to convert.
 * \param SelfRelativeSecurityDescriptor A pointer to a caller-allocated buffer that receives the self-relative security descriptor.
 * \param BufferLength A pointer to a variable that on input specifies the buffer size and on output receives the required length, in bytes.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlmakeselfrelativesd
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlMakeSelfRelativeSD(
    _In_ PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
    _Out_writes_bytes_(*BufferLength) PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    _Inout_ PULONG BufferLength
    );

/**
 * The RtlAbsoluteToSelfRelativeSD routine converts an absolute-format security descriptor into a self-relative security descriptor.
 *
 * \param AbsoluteSecurityDescriptor A pointer to the absolute-format security descriptor to convert.
 * \param SelfRelativeSecurityDescriptor A pointer to a caller-allocated buffer that receives the self-relative security descriptor.
 * \param BufferLength A pointer to a variable that on input specifies the buffer size and on output receives the required length, in bytes.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlabsolutetoselfrelativesd
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAbsoluteToSelfRelativeSD(
    _In_ PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
    _Out_writes_bytes_to_opt_(*BufferLength, *BufferLength) PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    _Inout_ PULONG BufferLength
    );

/**
 * The RtlSelfRelativeToAbsoluteSD routine converts a self-relative security descriptor into an absolute-format security descriptor and its associated components.
 *
 * \param SelfRelativeSecurityDescriptor A pointer to the self-relative security descriptor to convert.
 * \param AbsoluteSecurityDescriptor A pointer to a buffer that receives the absolute-format security descriptor.
 * \param AbsoluteSecurityDescriptorSize A pointer to a variable specifying and receiving the size, in bytes, of the absolute security descriptor.
 * \param Dacl A pointer to a buffer that receives the discretionary access control list (DACL).
 * \param DaclSize A pointer to a variable specifying and receiving the size, in bytes, of the DACL.
 * \param Sacl A pointer to a buffer that receives the system access control list (SACL).
 * \param SaclSize A pointer to a variable specifying and receiving the size, in bytes, of the SACL.
 * \param Owner A pointer to a buffer that receives the owner SID.
 * \param OwnerSize A pointer to a variable specifying and receiving the size, in bytes, of the owner SID.
 * \param PrimaryGroup A pointer to a buffer that receives the primary group SID.
 * \param PrimaryGroupSize A pointer to a variable specifying and receiving the size, in bytes, of the primary group SID.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlselfrelativetoabsolutesd
 */
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
/**
 * The RtlSelfRelativeToAbsoluteSD2 routine converts a self-relative security descriptor into an absolute-format security descriptor in place.
 *
 * \param SelfRelativeSecurityDescriptor A pointer to the self-relative security descriptor to convert in place.
 * \param BufferSize A pointer to a variable that on input specifies the buffer size and on output receives the size, in bytes, of the resulting descriptor.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSelfRelativeToAbsoluteSD2(
    _Inout_ PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    _Inout_ PULONG BufferSize
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_19H2)
/**
 * The RtlNormalizeSecurityDescriptor routine produces a normalized copy of a security descriptor with its components arranged in canonical order.
 *
 * \param SecurityDescriptor A pointer to a variable that references the security descriptor to normalize.
 * \param SecurityDescriptorLength The length, in bytes, of the security descriptor.
 * \param NewSecurityDescriptor An optional pointer to a variable that receives the normalized security descriptor.
 * \param NewSecurityDescriptorLength An optional pointer to a variable that receives the length, in bytes, of the normalized security descriptor.
 * \param CheckOnly If `TRUE`, the routine only checks whether normalization is required without producing a new descriptor.
 * \return Returns `TRUE` if the security descriptor was already normalized or was normalized successfully, otherwise `FALSE`.
 */
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
 * \param GrantedAccess The access mask that specifies the granted accesses.
 * \param DesiredAccess The access mask that specifies the desired accesses.
 * \return Returns TRUE if all desired accesses are granted, otherwise FALSE.
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
 * \param GrantedAccess The access mask that specifies the granted access rights.
 * \param DesiredAccess The access mask that specifies the desired access rights.
 * \return Returns TRUE if any of the desired access rights are granted, otherwise FALSE.
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
 * The RtlAreAllAccessesGranted routine checks if all desired accesses are granted.
 *
 * This function determines whether all the accesses specified in the DesiredAccess
 * mask are granted by the GrantedAccess mask.
 *
 * \param GrantedAccess The access mask that specifies the granted accesses.
 * \param DesiredAccess The access mask that specifies the desired accesses.
 * \return Returns TRUE if all desired accesses are granted, otherwise FALSE.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlAreAllAccessesGranted(
    _In_ ACCESS_MASK GrantedAccess,
    _In_ ACCESS_MASK DesiredAccess
    );

/**
 * The RtlAreAnyAccessesGranted routine checks if any of the desired accesses are granted.
 *
 * This function determines if any of the access rights specified in the DesiredAccess
 * mask are present in the GrantedAccess mask.
 *
 * \param GrantedAccess The access mask that specifies the granted access rights.
 * \param DesiredAccess The access mask that specifies the desired access rights.
 * \return Returns TRUE if any of the desired access rights are granted, otherwise FALSE.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlAreAnyAccessesGranted(
    _In_ ACCESS_MASK GrantedAccess,
    _In_ ACCESS_MASK DesiredAccess
    );
#endif // PHNT_NO_INLINE_ACCESSES_GRANTED

/**
 * The RtlMapGenericMask routine maps the generic access rights in an access mask to their corresponding specific and standard rights.
 *
 * \param AccessMask A pointer to the access mask whose generic rights are mapped in place.
 * \param GenericMapping A pointer to the GENERIC_MAPPING that defines the mapping of generic rights.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlmapgenericmask
 */
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

/**
 * The RtlCreateAcl routine creates and initializes an access control list (ACL).
 *
 * \param Acl A pointer to a caller-allocated buffer that receives the initialized ACL.
 * \param AclLength The length, in bytes, of the buffer pointed to by Acl.
 * \param AclRevision The revision level of the ACL to create.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlcreateacl
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateAcl(
    _Out_writes_bytes_(AclLength) PACL Acl,
    _In_ ULONG AclLength,
    _In_ ULONG AclRevision
    );

/**
 * The RtlValidAcl routine validates an access control list (ACL) by verifying its structure and contents.
 *
 * \param Acl A pointer to the ACL to validate.
 * \return Returns `TRUE` if the ACL is structurally valid, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlValidAcl(
    _In_ PACL Acl
    );

/**
 * The RtlQueryInformationAcl routine retrieves information about an access control list (ACL).
 *
 * \param Acl The ACL to query.
 * \param AclInformation A buffer that receives the requested information.
 * \param AclInformationLength The size, in bytes, of the buffer.
 * \param AclInformationClass The class of information to retrieve.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryInformationAcl(
    _In_ PACL Acl,
    _Out_writes_bytes_(AclInformationLength) PVOID AclInformation,
    _In_ ULONG AclInformationLength,
    _In_ ACL_INFORMATION_CLASS AclInformationClass
    );

/**
 * The RtlSetInformationAcl routine sets information on an access control list (ACL).
 *
 * \param Acl The ACL to modify.
 * \param AclInformation A buffer containing the information to set.
 * \param AclInformationLength The size, in bytes, of the buffer.
 * \param AclInformationClass The class of information to set.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetInformationAcl(
    _Inout_ PACL Acl,
    _In_reads_bytes_(AclInformationLength) PVOID AclInformation,
    _In_ ULONG AclInformationLength,
    _In_ ACL_INFORMATION_CLASS AclInformationClass
    );

/**
 * The RtlAddAce routine adds one or more ACEs to an access control list (ACL).
 *
 * \param Acl The ACL to modify.
 * \param AceRevision The revision of the ACL.
 * \param StartingAceIndex The zero-based index at which to insert the ACEs.
 * \param AceList A buffer containing the ACEs to add.
 * \param AceListLength The size, in bytes, of AceList.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlDeleteAce routine deletes an access control entry (ACE) at a specified index from an access control list (ACL).
 *
 * \param Acl A pointer to the ACL from which to delete the ACE.
 * \param AceIndex The zero-based index of the ACE to delete.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtldeleteace
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceIndex
    );

/**
 * The RtlGetAce routine retrieves a pointer to an ACE at the specified index in an access control list (ACL).
 *
 * \param Acl The ACL to query.
 * \param AceIndex The zero-based index of the ACE to retrieve.
 * \param Ace Receives a pointer to the ACE.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetAce(
    _In_ PACL Acl,
    _In_ ULONG AceIndex,
    _Outptr_ PVOID *Ace
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11_24H2)
/**
 * The RtlGetAcesBufferSize routine returns the total size, in bytes, of the access control entries (ACEs) contained in an access control list (ACL).
 *
 * \param Acl A pointer to the ACL to examine.
 * \param AcesBufferSize A pointer to a variable that receives the size, in bytes, of the ACEs.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetAcesBufferSize(
    _In_ PACL Acl,
    _Out_ PULONG AcesBufferSize
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11_24H2

/**
 * The RtlFirstFreeAce routine locates the first unused position in an access control list (ACL).
 *
 * \param Acl The ACL to examine.
 * \param FirstFree Receives a pointer to the first free position within the ACL.
 * \return TRUE if the ACL is well-formed; otherwise, FALSE.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlFirstFreeAce(
    _In_ PACL Acl,
    _Out_ PVOID *FirstFree
    );

/**
 * The RtlFindAceByType routine finds the first ACE of the specified type in an access control list (ACL).
 *
 * \param Acl The ACL to search.
 * \param AceType The ACE type to find.
 * \param Index An optional pointer that receives the index of the matching ACE.
 * \return A pointer to the matching ACE, or NULL if none was found.
 */
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
/**
 * The RtlOwnerAcesPresent routine determines whether an access control list (ACL) contains any access control entries (ACEs) that carry the owner flag.
 *
 * \param pAcl A pointer to the ACL to examine.
 * \return Returns `TRUE` if the ACL contains owner ACEs, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlOwnerAcesPresent(
    _In_ PACL pAcl
    );

/**
 * The RtlAddAccessAllowedAce routine adds an access-allowed access control entry (ACE) to an access control list (ACL).
 *
 * \param Acl A pointer to the ACL to modify.
 * \param AceRevision The revision level of the ACE to add.
 * \param AccessMask The access mask granted by the ACE.
 * \param Sid A pointer to the SID to which access is granted.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtladdaccessallowedace
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessAllowedAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ACCESS_MASK AccessMask,
    _In_ PSID Sid
    );

/**
 * The RtlAddAccessAllowedAceEx routine adds an access-allowed access control entry (ACE), including inheritance flags, to an access control list (ACL).
 *
 * \param Acl A pointer to the ACL to modify.
 * \param AceRevision The revision level of the ACE to add.
 * \param AceFlags The inheritance and audit flags for the ACE.
 * \param AccessMask The access mask granted by the ACE.
 * \param Sid A pointer to the SID to which access is granted.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtladdaccessallowedaceex
 */
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

/**
 * The RtlAddAccessDeniedAce routine adds an access-denied access control entry (ACE) to an access control list (ACL).
 *
 * \param Acl A pointer to the ACL to modify.
 * \param AceRevision The revision level of the ACE to add.
 * \param AccessMask The access mask denied by the ACE.
 * \param Sid A pointer to the SID to which access is denied.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtladdaccessdeniedace
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessDeniedAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ACCESS_MASK AccessMask,
    _In_ PSID Sid
    );

/**
 * The RtlAddAccessDeniedAceEx routine adds an access-denied access control entry (ACE), including inheritance flags, to an access control list (ACL).
 *
 * \param Acl A pointer to the ACL to modify.
 * \param AceRevision The revision level of the ACE to add.
 * \param AceFlags The inheritance and audit flags for the ACE.
 * \param AccessMask The access mask denied by the ACE.
 * \param Sid A pointer to the SID to which access is denied.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtladdaccessdeniedaceex
 */
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

/**
 * The RtlAddAuditAccessAce routine adds a system-audit access control entry (ACE) to an access control list (ACL).
 *
 * \param Acl A pointer to the ACL to modify.
 * \param AceRevision The revision level of the ACE to add.
 * \param AccessMask The access mask that triggers auditing.
 * \param Sid A pointer to the SID for which access is audited.
 * \param AuditSuccess If `TRUE`, successful access attempts are audited.
 * \param AuditFailure If `TRUE`, failed access attempts are audited.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtladdauditaccessace
 */
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

/**
 * The RtlAddAuditAccessAceEx routine adds a system-audit access control entry (ACE), including inheritance flags, to an access control list (ACL).
 *
 * \param Acl A pointer to the ACL to modify.
 * \param AceRevision The revision level of the ACE to add.
 * \param AceFlags The inheritance and audit flags for the ACE.
 * \param AccessMask The access mask that triggers auditing.
 * \param Sid A pointer to the SID for which access is audited.
 * \param AuditSuccess If `TRUE`, successful access attempts are audited.
 * \param AuditFailure If `TRUE`, failed access attempts are audited.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtladdauditaccessaceex
 */
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

/**
 * The RtlAddAccessAllowedObjectAce routine adds an object-specific access-allowed access control entry (ACE) to an access control list (ACL).
 *
 * \param Acl A pointer to the ACL to modify.
 * \param AceRevision The revision level of the ACE to add.
 * \param AceFlags The inheritance and audit flags for the ACE.
 * \param AccessMask The access mask granted by the ACE.
 * \param ObjectTypeGuid An optional pointer to the GUID of the object type protected by the ACE.
 * \param InheritedObjectTypeGuid An optional pointer to the GUID of the object type that inherits the ACE.
 * \param Sid A pointer to the SID to which access is granted.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtladdaccessallowedobjectace
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessAllowedObjectAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ULONG AceFlags,
    _In_ ACCESS_MASK AccessMask,
    _In_opt_ PCGUID ObjectTypeGuid,
    _In_opt_ PCGUID InheritedObjectTypeGuid,
    _In_ PSID Sid
    );

/**
 * The RtlAddAccessDeniedObjectAce routine adds an object-specific access-denied access control entry (ACE) to an access control list (ACL).
 *
 * \param Acl A pointer to the ACL to modify.
 * \param AceRevision The revision level of the ACE to add.
 * \param AceFlags The inheritance and audit flags for the ACE.
 * \param AccessMask The access mask denied by the ACE.
 * \param ObjectTypeGuid An optional pointer to the GUID of the object type protected by the ACE.
 * \param InheritedObjectTypeGuid An optional pointer to the GUID of the object type that inherits the ACE.
 * \param Sid A pointer to the SID to which access is denied.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtladdaccessdeniedobjectace
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessDeniedObjectAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ULONG AceFlags,
    _In_ ACCESS_MASK AccessMask,
    _In_opt_ PCGUID ObjectTypeGuid,
    _In_opt_ PCGUID InheritedObjectTypeGuid,
    _In_ PSID Sid
    );

/**
 * The RtlAddAuditAccessObjectAce routine adds an object-specific system-audit access control entry (ACE) to an access control list (ACL).
 *
 * \param Acl A pointer to the ACL to modify.
 * \param AceRevision The revision level of the ACE to add.
 * \param AceFlags The inheritance and audit flags for the ACE.
 * \param AccessMask The access mask that triggers auditing.
 * \param ObjectTypeGuid An optional pointer to the GUID of the object type protected by the ACE.
 * \param InheritedObjectTypeGuid An optional pointer to the GUID of the object type that inherits the ACE.
 * \param Sid A pointer to the SID for which access is audited.
 * \param AuditSuccess If `TRUE`, successful access attempts are audited.
 * \param AuditFailure If `TRUE`, failed access attempts are audited.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtladdauditaccessobjectace
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAddAuditAccessObjectAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ULONG AceFlags,
    _In_ ACCESS_MASK AccessMask,
    _In_opt_ PCGUID ObjectTypeGuid,
    _In_opt_ PCGUID InheritedObjectTypeGuid,
    _In_ PSID Sid,
    _In_ BOOLEAN AuditSuccess,
    _In_ BOOLEAN AuditFailure
    );

// private
/**
 * Compound ACE type value indicating impersonation.
 */
#define COMPOUND_ACE_IMPERSONATION 1

// private
/**
 * Represents a compound access-allowed access control entry (ACE).
 */
typedef struct _COMPOUND_ACCESS_ALLOWED_ACE
{
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    USHORT CompoundAceType; // COMPOUND_ACE_*
    USHORT Reserved;
    ULONG SidStart; // Server SID
    // Client SID follows
} COMPOUND_ACCESS_ALLOWED_ACE, *PCOMPOUND_ACCESS_ALLOWED_ACE;

/**
 * The RtlAddCompoundAce routine adds a compound access control entry (ACE) that associates a server SID with a client SID to an access control list (ACL).
 *
 * \param Acl A pointer to the ACL to modify.
 * \param AceRevision The revision level of the ACE to add.
 * \param AceType The compound ACE type.
 * \param AccessMask The access mask granted by the ACE.
 * \param ServerSid A pointer to the server SID.
 * \param ClientSid A pointer to the client SID.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlAddMandatoryAce routine adds a mandatory integrity label access control entry (ACE) to an access control list (ACL).
 *
 * \param Acl A pointer to the ACL to modify.
 * \param AceRevision The revision level of the ACE to add.
 * \param AceFlags The inheritance and audit flags for the ACE.
 * \param Sid A pointer to the SID that specifies the mandatory integrity level.
 * \param AceType The mandatory ACE type.
 * \param AccessMask The mandatory policy access mask enforced by the ACE.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlAddResourceAttributeAce routine adds a resource attribute access control entry (ACE) to an access control list (ACL).
 *
 * \param Acl A pointer to the ACL to modify.
 * \param AceRevision The revision level of the ACE to add.
 * \param AceFlags The inheritance and audit flags for the ACE.
 * \param AccessMask The access mask for the ACE.
 * \param Sid A pointer to the SID associated with the ACE.
 * \param AttributeInfo A pointer to the CLAIM_SECURITY_ATTRIBUTES_INFORMATION that describes the resource attribute.
 * \param ReturnLength A pointer to a variable that receives the size, in bytes, of the added ACE.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtladdresourceattributeace
 */
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

/**
 * The RtlAddScopedPolicyIDAce routine adds a scoped policy identifier access control entry (ACE) to an access control list (ACL).
 *
 * \param Acl A pointer to the ACL to modify.
 * \param AceRevision The revision level of the ACE to add.
 * \param AceFlags The inheritance and audit flags for the ACE.
 * \param AccessMask The access mask for the ACE.
 * \param Sid A pointer to the SID that identifies the central access policy.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlAddProcessTrustLabelAce routine adds a process trust label access control entry (ACE) to an access control list (ACL).
 *
 * \param Acl A pointer to the ACL to modify.
 * \param AceRevision The revision level of the ACE to add.
 * \param AceFlags The inheritance and audit flags for the ACE.
 * \param ProcessTrustLabelSid A pointer to the SID that specifies the process trust label.
 * \param AceType The process trust label ACE type.
 * \param AccessMask The access mask enforced by the ACE.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlAddAccessFilterAce routine adds an access-filter ACE to the specified access control list.
 *
 * \param Acl The ACL to modify.
 * \param AceRevision The revision of the ACL.
 * \param AceFlags The ACE flags (for example, TRUST_PROTECTED_FILTER_ACE_FLAG).
 * \param AccessFilterSid The SID identifying the access filter.
 * \param AceType The system filtering ACE type (SYSTEM_FILTERING_ACE_TYPE).
 * \param AccessMask The access mask for the ACE.
 * \param Buffer An optional buffer describing the SYSTEM_ACCESS_FILTER_ACE payload.
 * \param BufferLength The length, in bytes, of Buffer.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessFilterAce(
    _Inout_ PACL Acl,
    _In_ ULONG AceRevision,
    _In_ ULONG AceFlags, // TRUST_PROTECTED_FILTER_ACE_FLAG
    _In_ PSID AccessFilterSid,
    _In_ UCHAR AceType, // SYSTEM_FILTERING_ACE_TYPE
    _In_ ACCESS_MASK AccessMask,
    _In_ PVOID Buffer, // SYSTEM_ACCESS_FILTER_ACE
    _In_ USHORT BufferLength
    );

//
// Named pipes
//

/**
 * The RtlDefaultNpAcl routine creates the default discretionary access control list (DACL) used for named pipes.
 *
 * \param Acl A pointer to a variable that receives the allocated default named-pipe ACL.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDefaultNpAcl(
    _Out_ PACL *Acl
    );

//
// Security objects
//

/**
 * The RtlNewSecurityObject routine allocates and initializes a self-relative security descriptor for a new object, combining the parent and creator descriptors.
 *
 * \param ParentDescriptor An optional pointer to the security descriptor of the parent object.
 * \param CreatorDescriptor An optional pointer to the security descriptor supplied by the creator.
 * \param NewDescriptor A pointer to a variable that receives the newly allocated security descriptor.
 * \param IsDirectoryObject If `TRUE`, the new object is a container that can contain other objects.
 * \param Token An optional handle to the token representing the client that is creating the object.
 * \param GenericMapping A pointer to the GENERIC_MAPPING for the object type.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlNewSecurityObjectEx routine allocates and initializes a self-relative security descriptor for a new object, with support for object types and auto-inheritance.
 *
 * \param ParentDescriptor An optional pointer to the security descriptor of the parent object.
 * \param CreatorDescriptor An optional pointer to the security descriptor supplied by the creator.
 * \param NewDescriptor A pointer to a variable that receives the newly allocated security descriptor.
 * \param ObjectType An optional pointer to the GUID that identifies the object type.
 * \param IsDirectoryObject If `TRUE`, the new object is a container that can contain other objects.
 * \param AutoInheritFlags A combination of SEF_ flags controlling automatic inheritance.
 * \param Token An optional handle to the token representing the client that is creating the object.
 * \param GenericMapping A pointer to the GENERIC_MAPPING for the object type.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlNewSecurityObjectWithMultipleInheritance routine allocates and initializes a self-relative security descriptor for a new object, supporting inheritance from multiple object types.
 *
 * \param ParentDescriptor An optional pointer to the security descriptor of the parent object.
 * \param CreatorDescriptor An optional pointer to the security descriptor supplied by the creator.
 * \param NewDescriptor A pointer to a variable that receives the newly allocated security descriptor.
 * \param ObjectType An optional pointer to an array of GUID pointers that identify the object types.
 * \param GuidCount The number of object type GUIDs in the ObjectType array.
 * \param IsDirectoryObject If `TRUE`, the new object is a container that can contain other objects.
 * \param AutoInheritFlags A combination of SEF_ flags controlling automatic inheritance.
 * \param Token An optional handle to the token representing the client that is creating the object.
 * \param GenericMapping A pointer to the GENERIC_MAPPING for the object type.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlDeleteSecurityObject routine frees a security descriptor previously created by one of the RtlNewSecurityObject routines.
 *
 * \param ObjectDescriptor A pointer to a variable that references the security descriptor to delete.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteSecurityObject(
    _Inout_ PSECURITY_DESCRIPTOR *ObjectDescriptor
    );

/**
 * The RtlQuerySecurityObject routine retrieves selected security information from a self-relative security descriptor.
 *
 * \param ObjectDescriptor A pointer to the security descriptor to query.
 * \param SecurityInformation A SECURITY_INFORMATION value specifying which components to retrieve.
 * \param ResultantDescriptor An optional pointer to a buffer that receives the resulting security descriptor.
 * \param DescriptorLength The length, in bytes, of the ResultantDescriptor buffer.
 * \param ReturnLength A pointer to a variable that receives the required or written length, in bytes.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlSetSecurityObject routine applies a modification to an object's security descriptor.
 *
 * \param SecurityInformation A SECURITY_INFORMATION value specifying which components to modify.
 * \param ModificationDescriptor A pointer to the security descriptor that supplies the new information.
 * \param ObjectsSecurityDescriptor A pointer to a variable that references the object's security descriptor, updated on success.
 * \param GenericMapping A pointer to the GENERIC_MAPPING for the object type.
 * \param TokenHandle An optional handle to the token representing the client making the modification.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlSetSecurityObjectEx routine applies a modification to an object's security descriptor, with control over auto-inheritance.
 *
 * \param SecurityInformation A SECURITY_INFORMATION value specifying which components to modify.
 * \param ModificationDescriptor A pointer to the security descriptor that supplies the new information.
 * \param ObjectsSecurityDescriptor A pointer to a variable that references the object's security descriptor, updated on success.
 * \param AutoInheritFlags A combination of SEF_ flags controlling automatic inheritance.
 * \param GenericMapping A pointer to the GENERIC_MAPPING for the object type.
 * \param TokenHandle An optional handle to the token representing the client making the modification.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlConvertToAutoInheritSecurityObject routine converts a security descriptor so that it uses automatic inheritance.
 *
 * \param ParentDescriptor An optional pointer to the security descriptor of the parent object.
 * \param CurrentSecurityDescriptor A pointer to the security descriptor to convert.
 * \param NewSecurityDescriptor A pointer to a variable that receives the converted security descriptor.
 * \param ObjectType An optional pointer to the GUID that identifies the object type.
 * \param IsDirectoryObject If `TRUE`, the object is a container that can contain other objects.
 * \param GenericMapping A pointer to the GENERIC_MAPPING for the object type.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlNewInstanceSecurityObject routine creates a new security descriptor for an object instance, reusing prior state when the parent and creator descriptors are unchanged.
 *
 * \param ParentDescriptorChanged If `TRUE`, the parent descriptor has changed since the last call.
 * \param CreatorDescriptorChanged If `TRUE`, the creator descriptor has changed since the last call.
 * \param OldClientTokenModifiedId A pointer to the LUID recording the client token's previous modification identifier.
 * \param NewClientTokenModifiedId A pointer to a variable that receives the client token's current modification identifier.
 * \param ParentDescriptor An optional pointer to the security descriptor of the parent object.
 * \param CreatorDescriptor An optional pointer to the security descriptor supplied by the creator.
 * \param NewDescriptor A pointer to a variable that receives the newly allocated security descriptor.
 * \param IsDirectoryObject If `TRUE`, the new object is a container that can contain other objects.
 * \param TokenHandle A handle to the token representing the client that is creating the object.
 * \param GenericMapping A pointer to the GENERIC_MAPPING for the object type.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlCopySecurityDescriptor routine creates a copy of a security descriptor.
 *
 * \param InputSecurityDescriptor A pointer to the security descriptor to copy.
 * \param OutputSecurityDescriptor A pointer to a variable that receives the newly allocated copy.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCopySecurityDescriptor(
    _In_ PSECURITY_DESCRIPTOR InputSecurityDescriptor,
    _Out_ PSECURITY_DESCRIPTOR *OutputSecurityDescriptor
    );

// private
/**
 * Describes an access control entry used to build an access control list template.
 */
typedef struct _RTL_ACE_DATA
{
    UCHAR AceType;
    UCHAR InheritFlags;
    UCHAR AceFlags;
    ACCESS_MASK AccessMask;
    PSID* Sid;
} RTL_ACE_DATA, *PRTL_ACE_DATA;

/**
 * The RtlCreateUserSecurityObject routine creates a security descriptor for an object from an array of ACE data, owner, and group.
 *
 * \param AceData A pointer to an array of RTL_ACE_DATA entries describing the ACEs to place in the DACL.
 * \param AceCount The number of entries in the AceData array.
 * \param OwnerSid A pointer to the SID to assign as the object owner.
 * \param GroupSid A pointer to the SID to assign as the object primary group.
 * \param IsDirectoryObject If `TRUE`, the new object is a container that can contain other objects.
 * \param GenericMapping A pointer to the GENERIC_MAPPING for the object type.
 * \param NewSecurityDescriptor A pointer to a variable that receives the newly allocated security descriptor.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlCreateAndSetSD routine creates a self-relative security descriptor from an array of ACE data, owner, and group.
 *
 * \param AceData A pointer to an array of RTL_ACE_DATA entries describing the ACEs to place in the DACL.
 * \param AceCount The number of entries in the AceData array.
 * \param OwnerSid An optional pointer to the SID to assign as the object owner.
 * \param GroupSid An optional pointer to the SID to assign as the object primary group.
 * \param NewSecurityDescriptor A pointer to a variable that receives the newly allocated security descriptor.
 * \return NTSTATUS Successful or errant status.
 */
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

//
// Misc. security
//

/**
 * The RtlRunEncodeUnicodeString routine obfuscates a Unicode string in place using a simple run-based encoding seeded by the specified value.
 *
 * \param Seed A pointer to the seed byte; the routine updates it with the seed actually used.
 * \param String A pointer to the UNICODE_STRING to encode in place.
 */
NTSYSAPI
VOID
NTAPI
RtlRunEncodeUnicodeString(
    _Inout_ PUCHAR Seed,
    _Inout_ PUNICODE_STRING String
    );

/**
 * The RtlRunDecodeUnicodeString routine reverses the encoding performed by RtlRunEncodeUnicodeString, restoring a Unicode string in place.
 *
 * \param Seed The seed byte that was used to encode the string.
 * \param String A pointer to the UNICODE_STRING to decode in place.
 */
NTSYSAPI
VOID
NTAPI
RtlRunDecodeUnicodeString(
    _In_ UCHAR Seed,
    _Inout_ PUNICODE_STRING String
    );

/**
 * The RtlImpersonateSelf routine begins impersonation of the security context of the calling process on the current thread.
 *
 * \param ImpersonationLevel The SECURITY_IMPERSONATION_LEVEL at which to impersonate.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlImpersonateSelf(
    _In_ SECURITY_IMPERSONATION_LEVEL ImpersonationLevel
    );

// private
/**
 * The RtlImpersonateSelfEx routine begins impersonation of the calling process's security context on the current thread, with additional access and an optional returned token.
 *
 * \param ImpersonationLevel The SECURITY_IMPERSONATION_LEVEL at which to impersonate.
 * \param AdditionalAccess An optional additional access mask to request on the impersonation token.
 * \param ThreadToken An optional pointer to a variable that receives a handle to the impersonation token.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlImpersonateSelfEx(
    _In_ SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    _In_opt_ ACCESS_MASK AdditionalAccess,
    _Out_opt_ PHANDLE ThreadToken
    );

/**
 * The RtlAdjustPrivilege routine enables or disables a privilege in the token of the current thread or process.
 *
 * \param Privilege The identifier of the privilege to adjust.
 * \param Enable If `TRUE`, the privilege is enabled; if `FALSE`, it is disabled.
 * \param Client If `TRUE`, the privilege is adjusted in the current thread's impersonation token; otherwise in the process token.
 * \param WasEnabled A pointer to a variable that receives the previous enabled state of the privilege.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAdjustPrivilege(
    _In_ ULONG Privilege,
    _In_ BOOLEAN Enable,
    _In_ BOOLEAN Client,
    _Out_ PBOOLEAN WasEnabled
    );

/**
 * Flags for RtlAcquirePrivilege.
 */
#define RTL_ACQUIRE_PRIVILEGE_REVERT 0x00000001
#define RTL_ACQUIRE_PRIVILEGE_PROCESS 0x00000002

/**
 * The RtlAcquirePrivilege routine temporarily enables one or more privileges on the current thread.
 *
 * \param Privilege An array of privilege identifiers to enable.
 * \param NumPriv The number of privileges in the array.
 * \param Flags Flags controlling how the privileges are acquired.
 * \param ReturnedState Receives an opaque state pointer to pass to RtlReleasePrivilege.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAcquirePrivilege(
    _In_ PULONG Privilege,
    _In_ ULONG NumPriv,
    _In_ ULONG Flags,
    _Out_ PVOID *ReturnedState
    );

/**
 * The RtlReleasePrivilege routine restores privileges previously enabled with RtlAcquirePrivilege.
 *
 * \param StatePointer The state pointer returned by RtlAcquirePrivilege.
 */
NTSYSAPI
VOID
NTAPI
RtlReleasePrivilege(
    _In_ PVOID StatePointer
    );

// private
/**
 * The RtlRemovePrivileges routine removes from a token all privileges except those in a caller-supplied keep list.
 *
 * \param TokenHandle A handle to the token from which privileges are removed.
 * \param PrivilegesToKeep A pointer to an array of privilege identifiers to retain.
 * \param PrivilegeCount The number of entries in the PrivilegesToKeep array.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlRemovePrivileges(
    _In_ HANDLE TokenHandle,
    _In_ PULONG PrivilegesToKeep,
    _In_ ULONG PrivilegeCount
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_8)

/**
 * The RtlIsUntrustedObject routine determines whether the specified object originates from an untrusted (lower-integrity) source.
 *
 * \param Handle An optional handle to the object to inspect.
 * \param Object An optional pointer to the object to inspect.
 * \param IsUntrustedObject Receives TRUE if the object is untrusted.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlIsUntrustedObject(
    _In_opt_ HANDLE Handle,
    _In_opt_ PVOID Object,
    _Out_ PBOOLEAN IsUntrustedObject
    );

/**
 * The RtlQueryValidationRunlevel routine returns the validation run level associated with an optional component name.
 *
 * \param ComponentName An optional pointer to the Unicode string that names the component to query.
 * \return ULONG The validation run level for the specified component.
 */
NTSYSAPI
ULONG
NTAPI
RtlQueryValidationRunlevel(
    _In_opt_ PCUNICODE_STRING ComponentName
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_8

/**
 * The RtlNewSecurityGrantedAccess routine determines the access granted to a client for a desired access mask, taking privileges into account.
 *
 * \param DesiredAccess The access mask requested by the client.
 * \param NewPrivileges A pointer to a PRIVILEGE_SET that receives the privileges used to grant access.
 * \param Length A pointer to a variable that on input specifies and on output receives the size, in bytes, of NewPrivileges.
 * \param TokenHandle An optional handle to the client token.
 * \param GenericMapping A pointer to the GENERIC_MAPPING for the object type.
 * \param RemainingDesiredAccess A pointer to a variable that receives the access still requiring a discretionary check.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlNewSecurityGrantedAccess(
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PPRIVILEGE_SET NewPrivileges,
    _Inout_ PULONG Length,
    _In_opt_ HANDLE TokenHandle,
    _In_ PGENERIC_MAPPING GenericMapping,
    _Out_ PACCESS_MASK RemainingDesiredAccess
    );

//
// Private namespaces
//

// rev
/**
 * Flags for boundary descriptors.
 */
#define BOUNDARY_DESCRIPTOR_FLAG_NONE 0x0
#define BOUNDARY_DESCRIPTOR_ADD_APPCONTAINER_SID 0x0001

/**
 * The RtlCreateBoundaryDescriptor routine creates a boundary descriptor used to define an isolation boundary for a private object namespace.
 *
 * \param Name A pointer to the Unicode string that names the boundary.
 * \param Flags Flags that control the creation of the boundary descriptor.
 * \return POBJECT_BOUNDARY_DESCRIPTOR A pointer to the created boundary descriptor, or `NULL` on failure.
 */
_Ret_maybenull_
_Success_(return != NULL)
NTSYSAPI
POBJECT_BOUNDARY_DESCRIPTOR
NTAPI
RtlCreateBoundaryDescriptor(
    _In_ PCUNICODE_STRING Name,
    _In_ ULONG Flags
    );

/**
 * The RtlDeleteBoundaryDescriptor routine deletes a boundary descriptor and frees its resources.
 *
 * \param BoundaryDescriptor A pointer to the boundary descriptor to delete. The pointer is invalid after this call.
 */
NTSYSAPI
VOID
NTAPI
RtlDeleteBoundaryDescriptor(
    _In_ _Post_invalid_ POBJECT_BOUNDARY_DESCRIPTOR BoundaryDescriptor
    );

/**
 * The RtlAddSIDToBoundaryDescriptor routine adds a required security identifier (SID) to a boundary descriptor.
 *
 * \param BoundaryDescriptor A pointer to a variable that references the boundary descriptor to modify.
 * \param RequiredSid A pointer to the SID to add to the boundary.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAddSIDToBoundaryDescriptor(
    _Inout_ POBJECT_BOUNDARY_DESCRIPTOR *BoundaryDescriptor,
    _In_ PCSID RequiredSid
    );

// rev
/**
 * The RtlAddIntegrityLabelToBoundaryDescriptor routine adds a mandatory integrity label to a boundary descriptor.
 *
 * \param BoundaryDescriptor A pointer to a variable that references the boundary descriptor to modify.
 * \param IntegrityLabel A pointer to the SID that specifies the integrity label to add.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAddIntegrityLabelToBoundaryDescriptor(
    _Inout_ POBJECT_BOUNDARY_DESCRIPTOR *BoundaryDescriptor,
    _In_ PCSID IntegrityLabel
    );

//
// Version
//

/**
 * \brief Basic operating system version information returned by RtlGetVersion.
 *
 * \details This is the RTL equivalent of OSVERSIONINFOW. Callers must set
 * OSVersionInfoSize to sizeof(RTL_OSVERSIONINFO) before calling RtlGetVersion.
 */
// rev
/**
 * Contains operating system version information.
 */
typedef struct _RTL_OSVERSIONINFO
{
    ULONG OSVersionInfoSize;
    ULONG MajorVersion;
    ULONG MinorVersion;
    ULONG BuildNumber;
    ULONG PlatformId;
    WCHAR CSDVersion[128];
} RTL_OSVERSIONINFO, *PRTL_OSVERSIONINFO;

/**
 * \brief Extended operating system version information returned by RtlGetVersion.
 *
 * \details This is the RTL equivalent of OSVERSIONINFOEXW. In addition to the
 * basic version fields, it includes service-pack, suite-mask, and product-type
 * information. Callers must set OSVersionInfoSize to sizeof(RTL_OSVERSIONINFOEX)
 * before calling RtlGetVersion.
 */
// rev
/**
 * Contains extended operating system version information.
 */
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
/**
 * Further-extended operating system version information used by newer
 * Windows builds.
 */
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
//
// Input:
// - OSVersionInfoSize must be set to sizeof(RTL_OSVERSIONINFOEX3).
// - Input.LayerNumber selects which build layer to query.
// - Input.AttribSelector selects which attribute to return for that layer.
//
// Output:
// - MajorVersion/MinorVersion/BuildNumber identify the selected layer.
// - LayerAttrib contains the string for the selected attribute.
// - LayerCount returns the number of available build layers.
// - LayerFlags contains per-layer flags; bit 0 is top-level and bit 1 is checked.

/**
 * Attribute flags describing operating system version information layers.
 */
#define RTL_OSVERSIONINFO_ATTRIB_LAYER_NAME    0
#define RTL_OSVERSIONINFO_ATTRIB_BUILD_STAMP   1
#define RTL_OSVERSIONINFO_ATTRIB_BUILD_BRANCH  2 // HKLM\Software\Microsoft\Windows NT\CurrentVersion\BuildBranch
#define RTL_OSVERSIONINFO_ATTRIB_BUILD_ARCH    3
#define RTL_OSVERSIONINFO_ATTRIB_BUILD_LAB     4 // HKLM\Software\Microsoft\Windows NT\CurrentVersion\BuildLab
#define RTL_OSVERSIONINFO_ATTRIB_BUILD_LAB_EX  5 // HKLM\Software\Microsoft\Windows NT\CurrentVersion\BuildLabEx

// rev
/**
 * Further-extended operating system version information used by newer
 * Windows builds.
 */
typedef struct _RTL_OSVERSIONINFOEX3
{
    //
    // Input: Set to sizeof(RTL_OSVERSIONINFOEX3) before calling RtlGetVersion.
    //
    ULONG OSVersionInfoSize;

    //
    // Output: Version numbers for the selected build layer.
    //
    ULONG MajorVersion;
    ULONG MinorVersion;
    ULONG BuildNumber;

    //
    // Output: A QFE/build-layer numeric field.
    //
    union
    {
        ULONG PlatformId;
        ULONG QfeNumber;
    };

    //
    // Output: Contains the string for the selected attribute.
    //
    union
    {
        WCHAR CSDVersion[128];
        WCHAR LayerAttrib[128];
    };

    //
    // Output: Operating system version information
    //
    USHORT ServicePackMajor;
    USHORT ServicePackMinor;
    USHORT SuiteMask;
    UCHAR ProductType;
    UCHAR Reserved;
    ULONG SuiteMaskEx;
    ULONG Reserved2;

    //
    // Input LayerNumber:
    //   Which build layer to query, in the range [0, LayerCount).
    //
    // Input AttribSelector:
    //   Which value to retrieve for that layer:
    //     0 = layer display name
    //     1 = BuildStamp
    //     2 = BuildBranch
    //     3 = BuildArch
    //     4 = BuildLab
    //     5 = BuildLabEx
    //
    union
    {
        USHORT RawInput16;
        struct
        {
            USHORT LayerNumber : 12;
            USHORT AttribSelector : 4;
        };
    } Input;

    //
    // Output: total number of available build layers.
    //
    USHORT LayerCount;

    //
    // Output: flags for the selected layer.
    //
    union
    {
        ULONG LayerFlags;
        struct
        {
            ULONG IsTopLevel : 1;
            ULONG IsChecked : 1;
            ULONG Spare : 30;
        };
    };
} RTL_OSVERSIONINFOEX3, * PRTL_OSVERSIONINFOEX3;

/**
 * The RtlGetVersion routine gets version information about the currently running operating system.
 *
 * \param VersionInformation A pointer to an RTL_OSVERSIONINFO- or
 * RTL_OSVERSIONINFOEX-compatible structure that receives the current operating
 * system version information.
 * \return STATUS_SUCCESS on success.
 * \remarks RtlGetVersion is the native equivalent of GetVersionEx. When using
 * this routine to test for a required Windows version, compare version numbers
 * as greater-than-or-equal rather than exact equality so later versions also
 * satisfy the check. Because features can be delivered outside the base OS,
 * major and minor version numbers alone are not a reliable feature test; use
 * RtlVerifyVersionInfo when checking for specific system features.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlgetversion
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetVersion(
    _Out_ PVOID VersionInformation
    );

/**
 * The RtlVerifyVersionInfo routine compares specified operating system version requirements against the
 * currently running operating system.
 *
 * \param VersionInformation A pointer to an RTL_OSVERSIONINFOEX-compatible
 * structure that describes the required operating system attributes.
 * \param TypeMask A bitwise OR of VER_* flags that selects which members of
 * VersionInformation participate in the comparison.
 * \param ConditionMask A comparison mask built with VER_SET_CONDITION that
 * specifies how each selected member is compared.
 * \return STATUS_SUCCESS if the current operating system satisfies the
 * specified requirements, STATUS_INVALID_PARAMETER for invalid input, or
 * STATUS_REVISION_MISMATCH if the version check fails.
 * \remarks This routine is the native equivalent of VerifyVersionInfo. It is
 * intended for version and feature gating, and is more reliable than comparing
 * major/minor version numbers alone. Version comparisons for major version,
 * minor version, and service pack fields are evaluated sequentially, so a
 * higher major version satisfies the check without testing lower-order fields.
 * To verify a version range, call RtlVerifyVersionInfo separately for the lower
 * and upper bounds.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlverifyversioninfo
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlVerifyVersionInfo(
    _In_ PVOID VersionInformation,
    _In_ ULONG TypeMask,
    _In_ ULONGLONG ConditionMask
    );

// rev
/**
 * The RtlGetNtVersionNumbers routine retrieves the major version, minor version, and build number of the running operating system.
 *
 * \param NtMajorVersion An optional pointer to a variable that receives the major version number.
 * \param NtMinorVersion An optional pointer to a variable that receives the minor version number.
 * \param NtBuildNumber An optional pointer to a variable that receives the build number.
 */
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
/**
 * The RtlGetNtGlobalFlags routine returns the NT global flags (NtGlobalFlag) for the current process.
 *
 * \return ULONG The value of the NT global flags.
 */
NTSYSAPI
ULONG
NTAPI
RtlGetNtGlobalFlags(
    VOID
    );

// rev
/**
 * The RtlGetNtProductType routine retrieves the product type of the running operating system.
 *
 * \param NtProductType A pointer to a variable that receives the NT_PRODUCT_TYPE value.
 * \return Returns `TRUE` if the product type was retrieved successfully, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlGetNtProductType(
    _Out_ PNT_PRODUCT_TYPE NtProductType
    );

// private
/**
 * The RtlGetProductInfo routine retrieves the product type for the specified operating system and service pack versions.
 *
 * \param OSMajorVersion The major version number of the operating system.
 * \param OSMinorVersion The minor version number of the operating system.
 * \param SpMajorVersion The major version number of the service pack.
 * \param SpMinorVersion The minor version number of the service pack.
 * \param ReturnedProductType A pointer to a variable that receives the product type.
 * \return Returns `TRUE` if the product information was retrieved successfully, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlGetProductInfo(
    _In_ ULONG OSMajorVersion,
    _In_ ULONG OSMinorVersion,
    _In_ ULONG SpMajorVersion,
    _In_ ULONG SpMinorVersion,
    _Out_ PULONG ReturnedProductType
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
// private
/**
 * The RtlGetSuiteMask routine returns the suite mask that identifies the product suites available on the running system.
 *
 * \return ULONG The suite mask of the running system.
 */
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
/**
 * Pointer to a WAIT_CALLBACK_ROUTINE callback.
 */
typedef WAIT_CALLBACK_ROUTINE* PWAIT_CALLBACK_ROUTINE;

/**
 * Worker thread flags for RtlQueueWorkItem and related thread-pool routines.
 */
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

/**
 * The RtlRegisterWait routine directs a wait thread in the thread pool to wait on the object.
 *
 * \param WaitHandle A pointer to a variable that receives a wait handle on return.
 * Note that a wait handle cannot be used in functions that require an object handle.
 * \param Handle A handle to the object. If this handle is closed while the wait is
 * still pending, the function's behavior is undefined. The handle must have SYNCHRONIZE access.
 * \param Function Optional completion event for wait callback completion.
 * \param Context Optional value that is passed to the callback function.
 * \param Milliseconds The time-out interval, in milliseconds.
 * \param Flags Flags that control the behavior of the wait handle.
 * \return NTSTATUS Successful or errant status.
 * \remarks The wait thread queues the specified callback function to the thread pool
 * when the specified object is in the signaled state or the time-out interval elapses.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-registerwaitforsingleobject
 */
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

/**
 * The RtlDeregisterWait routine cancels a registered wait operation issued by the RtlRegisterWait function.
 *
 * \param WaitHandle The wait handle
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-unregisterwait
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDeregisterWait(
    _In_ HANDLE WaitHandle
    );

//
// RtlDeregisterWaitEx waits for all callback functions to complete before returning
// when the RTL_WAITER_DEREGISTER_WAIT_FOR_COMPLETION flag is passed to CompletionEvent.
//
/**
 * Flag requesting that wait deregistration block until pending callbacks complete.
 */
#define RTL_WAITER_DEREGISTER_WAIT_FOR_COMPLETION ((HANDLE)(LONG_PTR)-1)

/**
 * The RtlDeregisterWaitEx routine releases all resources used by a wait object.
 *
 * \param WaitHandle The wait handle.
 * \param CompletionEvent A handle to the event object to be signaled when the wait operation
 * has been unregistered. This parameter can be NULL.
 * \remarks If this parameter is RTL_WAITER_DEREGISTER_WAIT_FOR_COMPLETION, the function waits
 * for all callback functions to complete before returning.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/sync/unregisterwaitex
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
/**
 * Pointer to an RTL_WORK_CALLBACK callback.
 */
typedef RTL_WORK_CALLBACK* PRTL_WORK_CALLBACK;

/**
 * The RtlQueueWorkItem routine queues a work item for execution by a thread-pool worker thread.
 *
 * \param Function The callback to execute.
 * \param Context An optional context value passed to the callback.
 * \param Flags Flags controlling execution (for example, WT_EXECUTELONGFUNCTION).
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * Pointer to an RTL_OVERLAPPED_COMPLETION_ROUTINE callback.
 */
typedef RTL_OVERLAPPED_COMPLETION_ROUTINE* PRTL_OVERLAPPED_COMPLETION_ROUTINE;

/**
 * The RtlSetIoCompletionCallback routine associates the I/O completion port owned by the thread pool with the specified file handle.
 * On completion of an I/O request involving this file, a non-I/O worker thread will execute the specified callback function.
 *
 * \param FileHandle A handle to the file or device for which to set the I/O completion callback.
 * \param Function A pointer to the callback function to be executed when an I/O operation completes.
 * \param Flags Reserved; must be zero.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-bindiocompletioncallback
 */
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
/**
 * Pointer to an RTL_START_POOL_THREAD callback.
 */
typedef RTL_START_POOL_THREAD *PRTL_START_POOL_THREAD;

typedef _Function_class_(RTL_EXIT_POOL_THREAD)
NTSTATUS NTAPI RTL_EXIT_POOL_THREAD(
    _In_ NTSTATUS ExitStatus
    );
/**
 * Pointer to an RTL_EXIT_POOL_THREAD callback.
 */
typedef RTL_EXIT_POOL_THREAD *PRTL_EXIT_POOL_THREAD;

/**
 * The RtlSetThreadPoolStartFunc routine registers the callbacks used to start and exit worker threads in the RTL thread pool.
 *
 * \param StartPoolThread A pointer to the routine that starts a pool thread.
 * \param ExitPoolThread A pointer to the routine that exits a pool thread.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetThreadPoolStartFunc(
    _In_ PRTL_START_POOL_THREAD StartPoolThread,
    _In_ PRTL_EXIT_POOL_THREAD ExitPoolThread
    );

/**
 * The RtlUserThreadStart routine is the default entry-point wrapper for user-mode threads created by the loader.
 *
 * \param Function The thread start routine to invoke.
 * \param Parameter The parameter passed to the thread start routine.
 */
NTSYSAPI
VOID
NTAPI
RtlUserThreadStart(
    _In_ PUSER_THREAD_START_ROUTINE Function,
    _In_ PVOID Parameter
    );

/**
 * The LdrInitializeThunk routine is the loader initialization routine invoked when a new thread begins execution.
 *
 * \param ContextRecord The initial thread context to resume after loader initialization.
 * \param Parameter The system startup argument for the thread.
 */
NTSYSAPI
VOID
NTAPI
LdrInitializeThunk(
    _In_ PCONTEXT ContextRecord,
    _In_ PVOID Parameter
    );

/**
 * The LdrProcessInitializationComplete routine signals that process initialization performed by the loader is complete.
 *
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
LdrProcessInitializationComplete(
    VOID
    );

//
// Thread execution
//

/**
 * The RtlDelayExecution routine suspends the current thread for the specified interval.
 *
 * \param Alertable If `TRUE`, the delay can be interrupted by the delivery of an alert to the thread.
 * \param DelayInterval An optional pointer to the interval to wait, in 100-nanosecond units.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlCreateTimerQueue routine creates a queue for timers.
 *
 * \param TimerQueueHandle A pointer to a variable that receives the handle to the newly created timer queue.
 * \return NTSTATUS Successful or errant status.
 * \remarks Timer-queue timers are lightweight objects that enable you to specify a callback function to be called at a specified time.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoollegacyapiset/nf-threadpoollegacyapiset-createtimerqueue
 */
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
/**
 * Pointer to an RTL_TIMER_CALLBACK callback.
 */
typedef RTL_TIMER_CALLBACK *PRTL_TIMER_CALLBACK;

/**
 * The RtlCreateTimer routine creates a timer-queue timer.
 *
 * \param TimerQueueHandle A handle to the timer queue. This handle is returned by a previous call to RtlCreateTimerQueue.
 * \param Handle A pointer to a variable that receives the handle to the newly created timer-queue timer.
 * \param Function A pointer to the callback function to be executed when the timer expires.
 * \param Context A pointer to a variable to be passed to the callback function.
 * \param DueTime The amount of time in milliseconds relative to the current time that must elapse before the timer is signaled for the first time.
 * \param Period The period of the timer in milliseconds. If this value is zero, the timer is signaled once; otherwise, it is signaled periodically.
 * \param Flags The flags that control the behavior of the timer. This parameter can be zero or one of the following values:
 * WT_EXECUTEDEFAULT, WT_EXECUTEONLYONCE, WT_EXECUTELONGFUNCTION, WT_EXECUTEINTIMERTHREAD, WT_EXECUTEINPERSISTENTTHREAD, WT_TRANSFER_IMPERSONATION.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/threadpoollegacyapiset/nf-threadpoollegacyapiset-createtimerqueuetimer
 */
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

/**
 * The RtlCancelTimer routine cancels a timer previously created in a timer queue.
 *
 * \param TimerQueueHandle A handle to the timer queue that owns the timer.
 * \param Handle A handle to the timer to cancel.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCancelTimer(
    _In_ HANDLE TimerQueueHandle,
    _In_ HANDLE Handle
    );

/**
 * The RtlSetTimer routine creates a timer within a timer queue.
 *
 * \param TimerQueueHandle A handle to the timer queue.
 * \param Handle Receives a handle to the newly created timer.
 * \param Function The callback invoked when the timer expires.
 * \param Context An optional context value passed to the callback.
 * \param DueTime The time, in milliseconds, before the timer first expires.
 * \param Period The period, in milliseconds, for recurring expiration; zero for one-shot.
 * \param Flags Flags controlling timer behavior.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlUpdateTimer routine changes the due time and period of an existing timer.
 *
 * \param TimerQueueHandle A handle to the timer queue that owns the timer.
 * \param TimerHandle A handle to the timer to update.
 * \param DueTime The new time, in milliseconds, before the timer first fires.
 * \param Period The new period, in milliseconds, between subsequent firings, or zero for a one-shot timer.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlUpdateTimer(
    _In_ HANDLE TimerQueueHandle,
    _In_ HANDLE TimerHandle,
    _In_ ULONG DueTime,
    _In_ ULONG Period
    );

/**
 * Flag requesting that timer deletion block until pending callbacks complete.
 */
#define RTL_TIMER_DELETE_WAIT_FOR_COMPLETION ((HANDLE)(LONG_PTR)-1)

/**
 * The RtlDeleteTimer routine deletes a timer from a timer queue, optionally waiting for pending callbacks to complete.
 *
 * \param TimerQueueHandle A handle to the timer queue that owns the timer.
 * \param TimerToCancel A handle to the timer to delete.
 * \param Event An optional handle to an event signaled when the timer is deleted, or RTL_TIMER_DELETE_WAIT_FOR_COMPLETION to wait for completion.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteTimer(
    _In_ HANDLE TimerQueueHandle,
    _In_ HANDLE TimerToCancel,
    _In_opt_ HANDLE Event // optional: RTL_TIMER_DELETE_WAIT_FOR_COMPLETION
    );

/**
 * The RtlDeleteTimerQueue routine deletes a timer queue and all of the timers it contains.
 *
 * \param TimerQueueHandle A handle to the timer queue to delete.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteTimerQueue(
    _In_ HANDLE TimerQueueHandle
    );

/**
 * The RtlDeleteTimerQueueEx routine deletes a timer queue and all of its timers, optionally waiting for pending callbacks to complete.
 *
 * \param TimerQueueHandle A handle to the timer queue to delete.
 * \param Event An optional handle to an event signaled when the queue is deleted, or a sentinel value to wait for completion.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlFormatCurrentUserKeyPath routine builds the registry path of the current user's key under HKEY_USERS.
 *
 * \param CurrentUserKeyPath A pointer to a UNICODE_STRING that receives the allocated current-user key path.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlFormatCurrentUserKeyPath(
    _Out_ PUNICODE_STRING CurrentUserKeyPath
    );

/**
 * The RtlOpenCurrentUser routine opens the registry key for the current user (equivalent to HKEY_CURRENT_USER).
 *
 * \param DesiredAccess The access mask requested on the key.
 * \param CurrentUserKey A pointer to a variable that receives the handle to the opened key.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlOpenCurrentUser(
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE CurrentUserKey
    );

/**
 * Registry path base and flag values for RtlQueryRegistryValues and related routines.
 */
#define RTL_REGISTRY_ABSOLUTE 0
#define RTL_REGISTRY_SERVICES 1 // \Registry\Machine\System\CurrentControlSet\Services
#define RTL_REGISTRY_CONTROL 2 // \Registry\Machine\System\CurrentControlSet\Control
#define RTL_REGISTRY_WINDOWS_NT 3 // \Registry\Machine\Software\Microsoft\Windows NT\CurrentVersion
#define RTL_REGISTRY_DEVICEMAP 4 // \Registry\Machine\Hardware\DeviceMap
#define RTL_REGISTRY_USER 5 // \Registry\User\CurrentUser
#define RTL_REGISTRY_MAXIMUM 6
#define RTL_REGISTRY_HANDLE 0x40000000
#define RTL_REGISTRY_OPTIONAL 0x80000000

/**
 * The RtlCreateRegistryKey routine creates a registry key relative to one of the predefined RTL_REGISTRY_ locations.
 *
 * \param RelativeTo A flag that specifies the base location the Path is relative to.
 * \param Path A pointer to the null-terminated path of the key to create.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlcreateregistrykey
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateRegistryKey(
    _In_ ULONG RelativeTo,
    _In_ PCWSTR Path
    );

/**
 * The RtlCheckRegistryKey routine determines whether a registry key exists relative to one of the predefined RTL_REGISTRY_ locations.
 *
 * \param RelativeTo A flag that specifies the base location the Path is relative to.
 * \param Path A pointer to the null-terminated path of the key to check.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtlcheckregistrykey
 */
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
/**
 * Pointer to an RTL_QUERY_REGISTRY_ROUTINE callback.
 */
typedef RTL_QUERY_REGISTRY_ROUTINE *PRTL_QUERY_REGISTRY_ROUTINE;

/**
 * Describes a registry value to be queried or enumerated by RtlQueryRegistryValues.
 */
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

/**
 * Flags controlling RtlQueryRegistryValues behavior.
 */
#define RTL_QUERY_REGISTRY_SUBKEY 0x00000001
#define RTL_QUERY_REGISTRY_TOPKEY 0x00000002
#define RTL_QUERY_REGISTRY_REQUIRED 0x00000004
#define RTL_QUERY_REGISTRY_NOVALUE 0x00000008
#define RTL_QUERY_REGISTRY_NOEXPAND 0x00000010
#define RTL_QUERY_REGISTRY_DIRECT 0x00000020
#define RTL_QUERY_REGISTRY_DELETE 0x00000040
#define RTL_QUERY_REGISTRY_NOSTRING 0x00000080 // deprecated
#define RTL_QUERY_REGISTRY_TYPECHECK 0x00000100

/**
 * Constants for the registry value type-check field.
 */
#define RTL_QUERY_REGISTRY_TYPECHECK_SHIFT 24
#define RTL_QUERY_REGISTRY_TYPECHECK_MASK (0xff << RTL_QUERY_REGISTRY_TYPECHECK_SHIFT)

/**
 * The RtlQueryRegistryValues routine queries multiple registry values in a single call using a query table.
 *
 * \param RelativeTo Specifies how Path is interpreted (RTL_REGISTRY_*).
 * \param Path The registry path to query, relative to RelativeTo.
 * \param QueryTable A table describing the values to query and their handlers.
 * \param Context An optional context value passed to the query-table callbacks.
 * \param Environment An optional environment block used to expand REG_EXPAND_SZ values.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlQueryRegistryValuesEx routine queries multiple registry values using a query table, with extended validation.
 *
 * \param RelativeTo Specifies how Path is interpreted (RTL_REGISTRY_*).
 * \param Path The registry path to query, relative to RelativeTo.
 * \param QueryTable A table describing the values to query and their handlers.
 * \param Context An optional context value passed to the query-table callbacks.
 * \param Environment An optional environment block used to expand REG_EXPAND_SZ values.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlQueryRegistryValueWithFallback routine queries a registry value from a primary key, falling back to a secondary key if absent.
 *
 * \param PrimaryHandle An optional handle to the primary key to query.
 * \param FallbackHandle An optional handle to the fallback key.
 * \param ValueName The name of the value to query.
 * \param ValueLength The size, in bytes, of the ValueData buffer.
 * \param ValueType An optional pointer that receives the value type (REG_*).
 * \param ValueData A buffer that receives the value data.
 * \param ResultLength Receives the size, in bytes, of the value data.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlWriteRegistryValue routine writes a single registry value.
 *
 * \param RelativeTo Specifies how Path is interpreted (RTL_REGISTRY_*).
 * \param Path The registry path to write to, relative to RelativeTo.
 * \param ValueName The name of the value to write.
 * \param ValueType The type of the value (REG_*).
 * \param ValueData A buffer containing the value data to write.
 * \param ValueLength The size, in bytes, of ValueData.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlDeleteRegistryValue routine deletes a value from a registry key relative to one of the predefined RTL_REGISTRY_ locations.
 *
 * \param RelativeTo A flag that specifies the base location the Path is relative to.
 * \param Path A pointer to the null-terminated path of the key that contains the value.
 * \param ValueName A pointer to the null-terminated name of the value to delete.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-rtldeleteregistryvalue
 */
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
 * The RtlEnableThreadProfiling routine enables thread profiling on the specified thread.
 *
 * \param ThreadHandle The handle to the thread on which you want to enable profiling. This must be the current thread.
 * \param Flags To receive thread profiling data such as context switch count, set this parameter to THREAD_PROFILING_FLAG_DISPATCH; otherwise, set to 0.
 * \param HardwareCounters To receive hardware performance counter data, set this parameter to a bitmask that identifies the hardware counters to collect.
 * \param PerformanceDataHandle An opaque handle that you use when calling the RtlReadThreadProfilingData and RtlDisableThreadProfiling functions.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-enablethreadprofiling
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
 * The RtlDisableThreadProfiling routine disables thread profiling.
 *
 * \param PerformanceDataHandle The handle that the RtlEnableThreadProfiling function returned.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-querythreadprofiling
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDisableThreadProfiling(
    _In_ PVOID PerformanceDataHandle
    );

// rev
/**
 * The RtlQueryThreadProfiling routine determines whether thread profiling is enabled for the specified thread.
 *
 * \param ThreadHandle The handle to the thread on which you want to enable profiling. This must be the current thread.
 * \param Enabled Is TRUE if thread profiling is enabled for the specified thread; otherwise, FALSE.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-querythreadprofiling
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
 * The RtlReadThreadProfilingData routine reads the specified profiling data associated with the thread.
 *
 * \param PerformanceDataHandle The handle that the RtlEnableThreadProfiling function returned.
 * \param Flags One or more flags set when you called the RtlEnableThreadProfiling function that specify the counter data to read.
 * \param PerformanceData A PERFORMANCE_DATA structure that contains the thread profiling and hardware counter data.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-readthreadprofilingdata
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlReadThreadProfilingData(
    _In_ HANDLE PerformanceDataHandle,
    _In_ ULONG Flags,
    _Out_ PPERFORMANCE_DATA PerformanceData
    );

/**
 * The RtlGetNativeSystemInformation routine retrieves native system information, returning 64-bit data from a WOW64 process where applicable.
 *
 * \param SystemInformationClass The system information class to query.
 * \param NativeSystemInformation A buffer that receives the requested information.
 * \param InformationLength The size, in bytes, of the buffer.
 * \param ReturnLength An optional pointer that receives the number of bytes returned.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The NtWow64GetNativeSystemInformation routine retrieves native (64-bit) system information from a WOW64 (32-bit) process.
 *
 * \param SystemInformationClass The system information class to query.
 * \param NativeSystemInformation A buffer that receives the requested native system information.
 * \param InformationLength The size, in bytes, of the buffer.
 * \param ReturnLength An optional pointer to a variable that receives the number of bytes returned.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlQueueApcWow64Thread routine queues a user-mode APC to a WOW64 (32-bit) thread from a native (64-bit) process.
 *
 * \param ThreadHandle A handle to the target WOW64 thread.
 * \param ApcRoutine The APC routine to execute in the target thread.
 * \param ApcArgument1 An optional first argument passed to the APC routine.
 * \param ApcArgument2 An optional second argument passed to the APC routine.
 * \param ApcArgument3 An optional third argument passed to the APC routine.
 * \return NTSTATUS Successful or errant status.
 */
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
 * The RtlWow64EnableFsRedirection routine enables or disables file system redirection for the calling thread.
 *
 * \param Wow64FsEnableRedirection If TRUE, requests redirection be enabled; if FALSE, requests redirection be disabled.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/wow64apiset/nf-wow64apiset-wow64enablewow64fsredirection
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlWow64EnableFsRedirection(
    _In_ BOOLEAN Wow64FsEnableRedirection
    );

/**
 * The RtlWow64EnableFsRedirectionEx routine enables or disables file system redirection for the calling thread.
 *
 * \param Wow64FsEnableRedirection If TRUE, requests redirection be enabled; if FALSE, requests redirection be disabled.
 * \param OldFsRedirectionLevel The WOW64 file system redirection value. The system uses this parameter to store information
 *  necessary to revert (re-enable) file system redirection.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/wow64apiset/nf-wow64apiset-wow64disablewow64fsredirection
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlWow64EnableFsRedirectionEx(
    _In_ PVOID Wow64FsEnableRedirection,
    _Out_ PVOID *OldFsRedirectionLevel
    );

//
// WOW64
//

/**
 * The RtlWow64GetCpuAreaEnabledFeatures routine returns the extended processor features enabled in the WoW64 CPU area.
 *
 * \param Features A pointer to a variable that on input specifies and on output receives the feature flags.
 * \return ULONGLONG The mask of enabled extended features in the CPU area.
 */
NTSYSAPI
ULONGLONG
NTAPI
RtlWow64GetCpuAreaEnabledFeatures(
    _Inout_ PULONG Features
    );

// rev
//NTSYSAPI
//NTSTATUS
//NTAPI
//RtlWow64GetCpuAreaInfo(
//    _In_ PWOW64_CPU_AREA_HEADER CpuArea,
//    _In_ USHORT MachineType,
//    _Out_ PWOW64_CPU_AREA_INFO CpuAreaInfo
//    );

// rev
/**
 * The RtlWow64GetCurrentCpuArea routine retrieves information about the CPU area for the current WoW64 thread.
 *
 * \param MachineType An optional pointer to a variable that receives the machine architecture of the CPU area.
 * \param ContextRecordAddress An optional pointer to a variable that receives the address of the context record.
 * \param SharedInfoAddress An optional pointer to a variable that receives the address of the shared CPU area information.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlWow64GetCurrentCpuArea(
    _Out_opt_ PUSHORT MachineType,
    _Out_opt_ PULONGLONG ContextRecordAddress,
    _Out_opt_ PULONGLONG SharedInfoAddress
    );

// rev
/**
 * The RtlWow64GetEquivalentMachineCHPE routine returns the compiled-hybrid (CHPE) machine type equivalent to a given machine type.
 *
 * \param MachineType The source machine architecture.
 * \return SHORT The equivalent CHPE machine architecture.
 */
NTSYSAPI
SHORT
NTAPI
RtlWow64GetEquivalentMachineCHPE(
    _In_ SHORT MachineType
    );

// rev
//NTSYSAPI
//NTSTATUS
//NTAPI
//RtlWow64GetSharedInfoProcess(
//    _In_ HANDLE ProcessHandle,
//    _Out_ PUCHAR IsWow64,
//    _Out_writes_bytes_(0x28) PWOW64_PROCESS_SHARED_INFO SharedInfo
//    );
//
//typedef struct _THREAD_DESCRIPTOR_INFORMATION
//{
//    _In_ ULONG Selector;
//    _Out_ LDT_ENTRY Entry;
//} THREAD_DESCRIPTOR_INFORMATION, *PTHREAD_DESCRIPTOR_INFORMATION;
//
//
//NTSYSAPI
//NTSTATUS
//NTAPI
//RtlWow64GetThreadSelectorEntry(
//    _In_ HANDLE ThreadHandle,
//    _Inout_ PTHREAD_DESCRIPTOR_INFORMATION SelectorEntry,
//    _In_ ULONG SelectorEntryLength,
//    _Out_opt_ PULONG ReturnLength
//    );

/**
 * The RtlWow64LogMessageInEventLogger routine writes a WOW64 subsystem message to the system event logger.
 *
 * \param StringCount The number of strings in the Strings array.
 * \param Strings An array of pointers to unicode strings to log.
 * \param EventId The event identifier to log.
 */
// rev
NTSYSAPI
VOID
NTAPI
RtlWow64LogMessageInEventLogger(
    _In_ USHORT StringCount,
    _In_reads_(StringCount) PCWSTR *Strings,
    _In_ ULONG EventId
    );

//
// 64-bit packed atomic list head for the WOW64 cross-process work queue.
// Manipulated exclusively via lock cmpxchg8b / _InterlockedCompareExchange64.
//
/**
 * Header of a WOW64 cross-process work list.
 */
typedef union _WOW64_CROSS_PROCESS_WORK_HDR 
{
    ULONG64 Value;
    struct 
    {
        ULONG FirstEntry : 31;   // byte offset from header base to first entry (0 == list empty)
        ULONG Flag       : 1;    // status/reset flag (surfaced via ResetFlag)
        ULONG Counter;           // ABA sequence counter, incremented on each CAS
    } s;
} WOW64_CROSS_PROCESS_WORK_HDR;

//
// A single work item. Entries live in a shared 0x4000-byte (4-page) window
// anchored at the page-aligned base of the list header. Total size 0x28.
//
/**
 * Represents a single entry in a WOW64 cross-process work list.
 */
typedef struct _WOW64_CROSS_PROCESS_WORK_ENTRY 
{
    ULONG NextEntry;            // byte offset to next entry(0 == end of chain)
    ULONG Command;              // work command/type (== 8 in the coalesce path of the pusher)
    ULONG64 BaseAddress;        // target base address
    ULONG64 RegionSize;         // region size
    ULONG64 Reserved0;
    ULONG64 Reserved1;
} WOW64_CROSS_PROCESS_WORK_ENTRY, *PWOW64_CROSS_PROCESS_WORK_ENTRY;

// rev
/**
 * The RtlWow64PopAllCrossProcessWorkFromWorkList routine atomically detaches all pending work
 * items from the shared WOW64 cross-process work list and returns them as a self-contained,
 * base-relative singly-linked chain.
 *
 * The 64-bit header at *WorkList is a bitfield:
 *   [63]    Busy flag (set by a concurrent producer)
 *   [62:32] Epoch counter (incremented on each successful pop)
 *   [30:0]  Offset from WorkList to the first list entry (0 = empty)
 *
 * \param WorkList Pointer to the shared 64-bit work-list header.
 * \param BusyFlag Receives the busy flag from the original header snapshot.
 * \return Head of the detached work chain, or NULL if the list was empty or busy.
 *         Each entry's first ULONG is a relative offset to the next entry.
 */
NTSYSAPI
PWOW64_CROSS_PROCESS_WORK_ENTRY
NTAPI
RtlWow64PopAllCrossProcessWorkFromWorkList(
    _In_ volatile WOW64_CROSS_PROCESS_WORK_HDR* WorkList,
    _Out_ BOOLEAN *BusyFlag
    );

/**
 * The RtlWow64PushCrossProcessWorkOntoFreeList routine atomically push a work entry onto the head of a free list.
 *
 * \details Links \p Entry at the head of \p FreeList, writing \c Entry->NextEntry
 * to the prior head offset and CAS-installing \p Entry's relative offset
 * as the new head. The \c Flag bit and \c Counter are preserved/bumped.
 * \p Entry is range-checked against the shared window before use.
 * \param[in,out] FreeList  Pointer to the atomic free-list head.
 * \param[in,out] Entry Entry to release onto the free list; its \c NextEntry field is overwritten to link it into the chain.
 * \return \c TRUE on success.
 * \exception STATUS_INVALID_PARAMETER (0xC000000D) if \p Entry lies outside the shared region.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlWow64PushCrossProcessWorkOntoFreeList(
    _Inout_ volatile WOW64_CROSS_PROCESS_WORK_HDR *FreeList,
    _Inout_ WOW64_CROSS_PROCESS_WORK_ENTRY *Entry
    );

/**
 * The RtlWow64PushCrossProcessWorkOntoWorkList routine atomically push a work entry onto the head of the active work list, coalescing with an adjacent existing entry where possible.
 * \details If the current head entry describes a region immediately adjacent to
 * \p Entry (matching \c Command == 8 and contiguous
 * \c BaseAddress / \c RegionSize), the two are merged in place instead
 * of linking a new node. The pre-merge state is snapshotted so the CAS
 * can be safely retried on contention.
 * \param[in,out] WorkList Pointer to the atomic work-list head.
 * \param[in] Entry Address of the entry to enqueue. Passed by value as a \c ULONGLONG in the raw ABI (RDX).
 * \param[out] ReclaimEntry Receives the prior head/merge target address, or 0.
 * \return \c TRUE on success.
 * \exception STATUS_INVALID_PARAMETER (0xC000000D) on range-check failure.  // rev
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlWow64PushCrossProcessWorkOntoWorkList(
    _Inout_  volatile WOW64_CROSS_PROCESS_WORK_HDR *WorkList,
    _In_ WOW64_CROSS_PROCESS_WORK_ENTRY *Entry,
    _Out_ WOW64_CROSS_PROCESS_WORK_ENTRY **ReclaimEntry
    );

// rev
/**
 * The RtlWow64PopCrossProcessWorkFromFreeList routine atomically pop a single entry from the head of a free list.
 *
 * \details CAS-replaces the head with \c head->NextEntry (preserving \c Flag,
 * bumping \c Counter). The popped entry's \c NextEntry field is zeroed before return.
 * \param[in,out] FreeList Pointer to the atomic free-list head.
 * \return Pointer to the detached entry, or \c NULL if the list was empty.
 * \exception STATUS_INVALID_PARAMETER (0xC000000D) if the head entry is out of range.
 */
_Ret_maybenull_
NTSYSAPI
PWOW64_CROSS_PROCESS_WORK_ENTRY
NTAPI
RtlWow64PopCrossProcessWorkFromFreeList(
    _Inout_ volatile WOW64_CROSS_PROCESS_WORK_HDR *FreeList
    );

// rev
/**
 * The RtlWow64RequestCrossProcessHeavyFlush routine requests a cross-process "heavy" flush by setting the work-list flag bit.
 *
 * \details Atomically sets bit 31 (@c Flag) of the head word and bumps
 * \c Counter, without dequeuing anything. The flag is later consumed by
 * \ref RtlWow64PopAllCrossProcessWorkFromWorkList via its \c ResetFlag output.
 * \param[in,out] WorkList Pointer to the atomic work-list head.
 * \return TRUE on success.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlWow64RequestCrossProcessHeavyFlush(
    _Inout_ volatile WOW64_CROSS_PROCESS_WORK_HDR *WorkList
    );

// rev
/**
 * Describes the owner of a critical section in a 32-bit process being debugged.
 */
typedef struct _RTL_QUERY_PROCESS_DEBUG_CS_OWNER32
{
    ULONG CriticalSectionHandle;      // in  -> RTL_DEBUG_INFORMATION.CriticalSectionHandle
    ULONG CriticalSectionOwnerThread; // out <- RTL_DEBUG_INFORMATION.CriticalSectionOwnerThread
} RTL_QUERY_PROCESS_DEBUG_CS_OWNER32, *PRTL_QUERY_PROCESS_DEBUG_CS_OWNER32;

// rev
/**
 * Receives debug information for a 32-bit (WOW64) process.
 */
typedef struct _RTL_QUERY_PROCESS_DEBUG_INFO_WOW64
{
    ULONG UniqueProcessId; // Target PID
    ULONG Padding;
    ULONG32 DebugInformation; // 32-bit Wow64 pointer to RTL_QUERY_PROCESS_DEBUG_CS_OWNER32
} RTL_QUERY_PROCESS_DEBUG_INFO_WOW64, *PRTL_QUERY_PROCESS_DEBUG_INFO_WOW64;

// rev
/**
 * The RtlpQueryProcessDebugInformationFromWow64 routine is a WOW64 thunk that queries critical-section owner debug information for a
 * target process on behalf of a 32-bit caller.
 *
 * \details Creates a debug buffer, forwards to RtlQueryProcessDebugInformation
 * for the requested process, and marshals the resulting CriticalSectionHandle
 * and CriticalSectionOwnerThread back into the WOW64 request block.
 * \param[in] Flags Query flags; must be RTL_QUERY_PROCESS_CS_OWNER (0x400)
 * (0x800 is also accepted).
 * \param[in,out] Wow64Info WOW64 request block carrying the target process id
 * and a pointer to the caller's result sink.
 * \return
 * - STATUS_SUCCESS on success (and the underlying query status otherwise).
 * - STATUS_INVALID_PARAMETER (0xC000000D) if \p Flags is not 0x400/0x800.
 * - STATUS_NO_MEMORY (0xC0000017) if the debug buffer cannot be allocated.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpQueryProcessDebugInformationFromWow64(
    _In_ ULONG Flags,
    _Inout_ PRTL_QUERY_PROCESS_DEBUG_INFO_WOW64 Wow64Info
    );

// rev
/**
 * The RtlpWow64CtxFromAmd64 routine down-converts a native AMD64 CONTEXT into a WOW64 (x86) context.
 *
 * \details Populates \p Wow64Context from \p Context, copying each register
 * group (control, integer, segments, floating-point, debug, extended/legacy
 * XMM, and XSTATE) only when the group is both requested in \p ContextFlags and
 * present in the source context's ContextFlags. WOW64 segment selectors are
 * applied as constants. Bit 0x40000000 reconciles the destination ContextFlags
 * against the source.
 * \param[in] ContextFlags x86 (CONTEXT_i386) context flags selecting which
 * register groups to convert.
 * \param[in] Context The source native AMD64 CONTEXT.
 * \param[in,out] Wow64Context The destination WOW64_CONTEXT to populate.
 * \return STATUS_SUCCESS, or a failure status propagated from the XSTATE copy.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpWow64CtxFromAmd64(
    _In_ ULONG ContextFlags,
    _In_ PCONTEXT Context,
    _Inout_ PWOW64_CONTEXT Wow64Context
    );

//
// Misc.
//

/**
 * The RtlComputeCrc32 routine computes the CRC32 checksum for a buffer, allowing for incremental computation by providing a partial CRC value.
 *
 * \param PartialCrc The initial CRC32 value. Use 0 for a new computation, or the result of a previous call to continue CRC calculation over additional data.
 * \param Buffer Pointer to the buffer containing the data to compute the CRC32 for.
 * \param Length The length, in bytes, of the buffer.
 * \return The computed CRC32 value.
 */
NTSYSAPI
ULONG32
NTAPI
RtlComputeCrc32(
    _In_ ULONG32 PartialCrc,
    _In_ PVOID Buffer,
    _In_ ULONG Length
    );

/**
 * The RtlEncodePointer routine encodes the specified pointer. Encoded pointers can be used to provide another layer of protection for pointer values.
 *
 * \param Ptr The system pointer to be encoded.
 * \return The function returns the encoded pointer.
 * \sa https://learn.microsoft.com/en-us/previous-versions/bb432254(v=vs.85)
 */
_Ret_maybenull_
NTSYSAPI
PVOID
NTAPI
RtlEncodePointer(
    _In_opt_ PVOID Ptr
    );

/**
 * The RtlDecodePointer routine decodes a pointer that was previously encoded with RtlEncodePointer.
 *
 * \param Ptr The system pointer to be decoded.
 * \return The function returns the decoded pointer.
 * \sa https://learn.microsoft.com/en-us/previous-versions/bb432242(v=vs.85)
 */
_Ret_maybenull_
NTSYSAPI
PVOID
NTAPI
RtlDecodePointer(
    _In_opt_ PVOID Ptr
    );

/**
 * The RtlEncodeSystemPointer routine encodes the specified pointer with a system-specific value.
 * Encoded pointers can be used to provide another layer of protection for pointer values.
 *
 * \param Ptr The system pointer to be encoded.
 * \return The function returns the encoded pointer.
 * \sa https://learn.microsoft.com/en-us/previous-versions/bb432255(v=vs.85)
 */
_Ret_maybenull_
NTSYSAPI
PVOID
NTAPI
RtlEncodeSystemPointer(
    _In_opt_ PVOID Ptr
    );

/**
 * The RtlDecodeSystemPointer routine decodes a pointer that was previously encoded with RtlEncodeSystemPointer.
 *
 * \param Ptr The pointer to be decoded.
 * \return The function returns the decoded pointer.
 * \sa https://learn.microsoft.com/en-us/previous-versions/bb432243(v=vs.85)
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
 * The RtlEncodeRemotePointer routine encodes the specified pointer of the specified process.
 * Encoded pointers can be used to provide another layer of protection for pointer values.
 *
 * \param ProcessHandle Handle to the remote process that owns the pointer.
 * \param Pointer The pointer to be encoded.
 * \param EncodedPointer The encoded pointer.
 * \return HRESULT Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/previous-versions/dn877135(v=vs.85)
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
 * The RtlDecodeRemotePointer routine decodes a pointer in a specified process that was previously
 * encoded with RtlEncodePointer or RtlEncodeRemotePointer.
 *
 * \param ProcessHandle Handle to the remote process that owns the pointer.
 * \param Pointer The pointer to be decoded.
 * \param DecodedPointer The decoded pointer.
 * \return HRESULT Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/previous-versions/dn877133(v=vs.85)
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
 * The RtlIsProcessorFeaturePresent routine determines whether the specified processor feature is supported by the current computer.
 *
 * \param ProcessorFeature The processor feature to be tested.
 * \return If the feature is supported, the return value is a nonzero value.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-isprocessorfeaturepresent
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
 * The RtlGetCurrentProcessorNumber routine retrieves the number of the processor the current thread was running
 * on during the call to this function.
 *
 * \return The function returns the current processor number.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getcurrentprocessornumber
 */
NTSYSAPI
ULONG
NTAPI
RtlGetCurrentProcessorNumber(
    VOID
    );

// rev
/**
 * The RtlGetCurrentProcessorNumberEx routine retrieves the processor group and number of the logical processor
 * in which the calling thread is running.
 *
 * \param ProcessorNumber A pointer to a PROCESSOR_NUMBER structure that receives the processor group and number
 * of the logical processor the calling thread is running.
 * \return This function does not return a value.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getcurrentprocessornumberex
 */
NTSYSAPI
VOID
NTAPI
RtlGetCurrentProcessorNumberEx(
    _Out_ PPROCESSOR_NUMBER ProcessorNumber
    );

//
// Private (Rtlp)
//

// rev
/**
 * The RtlpApplyLengthFunction routine invokes a length function to populate a UNICODE_STRING, then applies
 * the resulting character count as the string's byte Length.
 *
 * \details Calls \p LengthFunction to fill \p Destination and report a character
 * count, converts that count to a byte Length (count * sizeof(WCHAR)), and
 * stores it in \p Destination->Length. If \p Type is 56 the buffer is
 * additionally NUL-terminated at Buffer[count]. A count exceeding 0x7FFF
 * characters (0xFFFE bytes) overflows the UNICODE_STRING length field and is
 * rejected.
 * \param[in] Reserved Reserved; must be 0.
 * \param[in] Type Mode selector. Must be 16 (no termination) or 56
 * (NUL-terminate \p Destination->Buffer).
 * \param[in,out] Destination The target UNICODE_STRING; its Buffer is filled by
 * \p LengthFunction and its Length is set on success.
 * \param[in] LengthFunction Callback that fills \p Destination and returns the
 * character count.
 * \return
 * - STATUS_SUCCESS on success.
 * - STATUS_INVALID_PARAMETER (0xC000000D) if \p Reserved != 0,
 *   \p Destination or \p LengthFunction is NULL, or \p Type is not 16 or 56.
 * - STATUS_NAME_TOO_LONG (0xC0000106) if the character count exceeds 0x7FFF.
 * - Any negative NTSTATUS propagated from \p LengthFunction.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpApplyLengthFunction(
    _In_ ULONG_PTR Reserved,
    _In_ ULONG Type,
    _Inout_ PUNICODE_STRING Destination,
    _In_ PRTL_LENGTH_FUNCTION LengthFunction
    );

// rev
/**
 * The RtlpCheckDynamicTimeZoneInformation routine re-evaluates dynamic time-zone data for a given year and updates the
 * caller's structure in place if the transition rules have changed.
 *
 * \details Opens the registry handle for the zone named in
 * \p TimeZoneInformation->TimeZoneKeyName (via RtlpGetDynamicTimeZoneInfoHandle),
 * loads the REG_TZI record applicable to \p Year (RtlpFindRegTziForCurrentYear),
 * converts it to TIME_ZONE_INFORMATION form (RtlpRegTziFormatToTzi), and
 * memcmp's the first 0xAC bytes against \p TimeZoneInformation. If they differ,
 * the bias/date fields are overwritten while the descriptive name strings are preserved.
 * \param[in,out] TimeZoneInformation The dynamic time-zone info to validate and,
 * if stale, refresh in place.
 * \param[in] Year The (dynamic) year whose transition rules should be applied.
 * \return TRUE if \p TimeZoneInformation was updated; FALSE if it was already
 * current or the registry lookup failed.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlpCheckDynamicTimeZoneInformation(
    _Inout_ PRTL_DYNAMIC_TIME_ZONE_INFORMATION TimeZoneInformation,
    _In_ USHORT Year
    );

// rev
/**
 * The RtlpCleanupRegistryKeys routine removes stale MUI UI-language subkeys for the process, retaining only
 * the current/installed UI language, and invalidates cached MUI registry state.
 *
 * \details Enumerates the subkeys of
 * \\Registry\\Machine\\System\\CurrentControlSet\\Control\\MUI\\UILanguages and
 * collects those that are neither a currently installed language nor match the
 * system default UI language's culture name. Because keys cannot be deleted
 * during enumeration, the matching subkey handles are gathered into a growable
 * array and deleted in a second pass (NtDeleteKey + NtClose). If any key was
 * deleted, the process's cached MUI registry information (g_RegInfo) and TEB
 * language lists are released under RegistryInfoCritSect.
 * \return STATUS_SUCCESS on success; otherwise the first failing NTSTATUS
 * (e.g. STATUS_NO_MEMORY (0xC0000017), STATUS_UNSUCCESSFUL (0xC0000001)).
 * STATUS_NO_MORE_ENTRIES from the enumeration terminator is normalized to
 * STATUS_SUCCESS.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpCleanupRegistryKeys(
    VOID
    );

// rev
/**
 * Converts a self-relative token security attribute (V1) into its absolute (pointer-based) form.
 *
 * \details Validates a TOKEN_SECURITY_ATTRIBUTE_RELATIVE_V1 whose Name and
 * per-value data are stored as byte offsets from the structure base, then emits
 * an equivalent TOKEN_SECURITY_ATTRIBUTE_V1 with real pointers into
 * \p AbsoluteAttribute. Handling is dispatched on ValueType (INT64=1, UINT64=2,
 * STRING=3, FQBN=5/6-array path, SID=6, OCTET_STRING=16). All offsets and sizes
 * are overflow- and bounds-checked against \p RelativeAttributeLength.
 * \param[in] RelativeAttribute The source relative-format attribute.
 * \param[in] RelativeAttributeLength Size, in bytes, of \p RelativeAttribute.
 * \param[out] AbsoluteAttribute Receives the absolute-format attribute. May be
 * NULL only when querying the required size.
 * \param[in,out] AbsoluteAttributeLength On input, the byte capacity of
 * \p AbsoluteAttribute; on output, the required/consumed size.
 * \return 
 * - STATUS_SUCCESS on success.
 * - STATUS_INVALID_PARAMETER (0xC000000D) if \p RelativeAttribute or
 *   \p AbsoluteAttributeLength is NULL, or output requested with NULL buffer.
 * - STATUS_INVALID_BUFFER_SIZE (0xC0000077) if the input is truncated 
 *   inconsistent with its declared offsets.
 * - STATUS_INTEGER_OVERFLOW (0xC0000095) on size/offset arithmetic overflow.
 * - STATUS_BUFFER_TOO_SMALL (0xC0000023) if \p AbsoluteAttribute is too small;
 *   the required size is returned in \p AbsoluteAttributeLength.
 */
//NTSYSAPI
//NTSTATUS
//NTAPI
//RtlpConvertRelativeToAbsoluteSecurityAttribute(
//    _In_ PTOKEN_SECURITY_ATTRIBUTE_RELATIVE_V1 RelativeAttribute,
//    _In_ ULONG RelativeAttributeLength,
//    _Out_writes_bytes_opt_(*AbsoluteAttributeLength) PTOKEN_SECURITY_ATTRIBUTE_V1 AbsoluteAttribute,
//    _Inout_ PULONG AbsoluteAttributeLength
//    );

// rev
//
// Internal process MUI/NLS registry-information cache (ntdll!g_RegInfo).
// Allocated (0xA8 bytes) by RtlpMuiRegCreateRegistryInfo; released selectively
// by RtlpMuiRegFreeRegistryInfo. Flags is a per-member ownership/validity mask;
// each set bit designates an owned sub-allocation with its own destructor.
//
/**
 * Holds the cached MUI (multilingual user interface) registry state used for language resolution.
 */
typedef struct _RTL_MUI_REGISTRY_INFO
{
    ULONG Flags;                                  // +0x00 ownership mask (0x400 = self-owned)
    ULONG Reserved0;                              // +0x04
    PVOID Reserved1;                              // +0x08
    PVOID Reserved2;                              // +0x10
    PVOID SerializedData;                         // +0x18 (0x001) RtlFreeHeap
    PVOID StringPool;                             // +0x20 (0x002) RtlpMuiRegFreeStringPool
    PVOID UserLanguageConfigList;                 // +0x28 (0x004) RtlpMuiRegFreeLanguageConfigList
    PVOID MachineLanguageConfigList;              // +0x30 (0x008) RtlpMuiRegFreeLanguageConfigList
    PVOID LanguageList10;                         // +0x38 (0x010) RtlpMuiRegFreeLanguageList
    PVOID LanguageList20;                         // +0x40 (0x020) RtlpMuiRegFreeLanguageList
    PVOID Reserved3;                              // +0x48
    PVOID LanguageList80;                         // +0x50 (0x080) RtlpMuiRegFreeLanguageList
    PVOID LanguageList40;                         // +0x58 (0x040) RtlpMuiRegFreeLanguageList
    PVOID LanguageList200;                        // +0x60 (0x200) RtlpMuiRegFreeLanguageList
    struct _RTL_MUI_REGISTRY_INFO *Alternate;     // +0x68 nested (recursive free + RtlFreeHeap)
    PVOID Reserved4;                              // +0x70
    PVOID Reserved5;                              // +0x78
    PVOID Reserved6;                              // +0x80
    PVOID FallbackData;                           // +0x88 (0x800) RtlFreeHeap
    PVOID Reserved7;                              // +0x90
    PVOID Reserved8;                              // +0x98
    PVOID Reserved9;                              // +0xA0
} RTL_MUI_REGISTRY_INFO, *PRTL_MUI_REGISTRY_INFO;
    
// rev
/**
 * The RtlpCreateProcessRegistryInfo routine lazily creates and loads the process-wide MUI registry information cache, returning a pointer to it.
 *
 * \details On first use, initializes \c g_RegInfo via \c RtlpMuiRegCreateAndLoadRegistryInfo under \c RegistryInfoCritSect
 * using double-checked locking. Subsequent calls return the cached pointer without locking.
 * \param[out] RegistryInfo Optional. Receives the cached MUI registry-info pointer on success, or NULL on failure.
 * \return STATUS_SUCCESS on success; otherwise the status from the underlying load (e.g. STATUS_NO_MEMORY, 0xC0000017).
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpCreateProcessRegistryInfo(
    _Out_opt_ PRTL_MUI_REGISTRY_INFO *RegistryInfo
    );

// rev
/**
 * The RtlpEnsureBufferSize routine ensures an RTL_BUFFER can hold at least RequiredSize bytes, growing it via the atom-table allocator if necessary.
 *
 * \details No-op if the buffer is already large enough. If the request still
 * fits the embedded static buffer, only the size field is updated.
 * Otherwise a new block is allocated with \c RtlpAllocateAtom, the
 * existing contents are copied unless \p Flags requests otherwise, any
 * prior dynamic allocation is released with \c RtlpSysVolFree, and the
 * buffer is repointed. The buffer is never shrunk.
 * \param[in] Flags Bit 0 skips copying the existing contents on reallocation (RTL_SKIP_BUFFER_COPY); all other bits must be zero.
 * \param[in,out] Buffer The RTL_BUFFER to enlarge.
 * \param[in] RequiredSize The minimum required capacity, in bytes.
 * \return STATUS_SUCCESS on success.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpEnsureBufferSize(
    _In_ ULONG Flags,
    _Inout_ PRTL_BUFFER BufferState,
    _In_ SIZE_T RequiredSize
    );

// rev
/**
 * Accumulated system freeze/suspend bias, in 100-nanosecond units.
 *
 * \details Total time the system has spent in a frozen/suspended
 * (connected-standby) state. Subtracted from the raw interrupt time to yield
 * *unbiased* interrupt time:
 * \code
 * UnbiasedInterruptTime = SharedUserData->InterruptTime      // KUSER_SHARED_DATA+0x08
 *                       - RtlpFreezeTimeBias                 // this value
 *                       - SharedUserData->QpcBias;           // KUSER_SHARED_DATA+0x3B0
 * \endcode
 * Consumed by RtlQueryUnbiasedInterruptTime and the threadpool/WNF timer paths,
 * which read it in a torn-read guard loop since it is a 64-bit value updated by
 * the kernel.
 */
NTSYSAPI LONGLONG RtlpFreezeTimeBias;

// rev
/**
 * The RtlpGetDeviceFamilyInfoEnum routine retrieves device family and form factor information for the current system.
 *
 * \param UapInfo An optional pointer that receives the Universal Action Platform (UAP) version information.
 * \param DeviceFamily An optional pointer that receives the device family enumeration value (DEVICEFAMILYINFOENUM).
 * \param DeviceForm An optional pointer that receives the device form factor enumeration value (DEVICEFAMILYDEVICEFORM).
 */
NTSYSAPI
VOID
NTAPI
RtlpGetDeviceFamilyInfoEnum(
    _Out_opt_ PULONGLONG UapInfo,
    _Out_opt_ PULONG DeviceFamily,
    _Out_opt_ PULONG DeviceForm
    );

/**
 * The RtlpGetNameFromLangInfoNode routine retrieves the language name associated with a language-info node from the MUI registry information string pool.
 *
 * \param RegistryInfo The MUI registry information block that owns the string pool referenced by the node.
 * \param LangInfoNode A pointer to the language-info node to resolve.
 * \param Name Receives the resolved language name.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlpGetNameFromLangInfoNode(
    _In_ PRTL_MUI_REGISTRY_INFO RegistryInfo,
    _In_ PVOID LangInfoNode,
    _Inout_ PUNICODE_STRING Name
    );

/**
 * The RtlpInitializeLangRegistryInfo routine ensures the process MUI registry information cache is created and loaded, initializing it on first use.
 *
 * \param RegistryInfo On input points to the cache pointer; when the pointed-to value is NULL the cache is created and loaded.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlpInitializeLangRegistryInfo(
    _Inout_ PRTL_MUI_REGISTRY_INFO *RegistryInfo
    );

/**
 * The RtlpLoadMachineUIByPolicy routine loads the machine UI language list dictated by the MUI Group Policy settings.
 *
 * \param PolicyRootKey An optional handle to the policy root key; when NULL the default MUI policy key is opened.
 * \param RegistryInfo The MUI registry information block used to build the language list.
 * \param LanguageList On input points to an existing language list (or NULL); receives the populated language list.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlpLoadMachineUIByPolicy(
    _In_opt_ HANDLE PolicyRootKey,
    _In_ PRTL_MUI_REGISTRY_INFO RegistryInfo,
    _Inout_ PVOID *LanguageList
    );

/**
 * The RtlpLoadUserUIByPolicy routine loads the per-user UI language list dictated by the MUI Group Policy settings.
 *
 * \param UserRootKey An optional handle to the user policy root key; when NULL the default MUI policy key is opened.
 * \param RegistryInfo The MUI registry information block used to build the language list.
 * \param LanguageList On input points to an existing language list (or NULL); receives the populated language list.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlpLoadUserUIByPolicy(
    _In_opt_ HANDLE UserRootKey,
    _In_ PRTL_MUI_REGISTRY_INFO RegistryInfo,
    _Inout_ PVOID *LanguageList
    );

/**
 * The RtlpMergeSecurityAttributeInformation routine merges the resource-attribute ACEs from two system access control lists (SACLs) into a newly allocated ACL.
 *
 * \param SourceSacl An optional source ACL containing SYSTEM_RESOURCE_ATTRIBUTE_ACE entries.
 * \param AdditionalSacl An optional additional ACL whose attribute ACEs are merged with the source.
 * \param MergedSacl Receives a newly allocated ACL containing the merged attribute ACEs.
 * \param MergeMode Controls how overlapping attribute ACEs from the two ACLs are combined.
 * \return NTSTATUS Successful or errant status.
 * \remarks Prototype reconstructed from disassembly; the routine walks ACL structures (not raw security descriptors).
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlpMergeSecurityAttributeInformation(
    _In_opt_ PACL SourceSacl,
    _In_opt_ PACL AdditionalSacl,
    _Outptr_ PACL *MergedSacl,
    _In_ CHAR MergeMode
    );

// rev
/**
 * The RtlpNotOwnerCriticalSection routine handles the case in which a thread that does not own a critical section attempts to release it.
 *
 * \param CriticalSection A pointer to the critical section involved.
 */
NTSYSAPI
VOID
NTAPI
RtlpNotOwnerCriticalSection(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
    );

// rev
/**
 * The RtlpNtCreateKey routine is an internal helper that creates or opens a registry key using simplified parameters.
 *
 * \param KeyHandle A pointer to a variable that receives the handle to the created or opened key.
 * \param DesiredAccess The access mask requested on the key.
 * \param ObjectAttributes An optional pointer to the OBJECT_ATTRIBUTES that specify the key name and attributes.
 * \param CreateOptions Optional flags that control the creation of the key.
 * \param ValueType An optional value type associated with the key.
 * \param Disposition An optional pointer to a variable that receives whether the key was created or opened.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpNtCreateKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Inout_opt_ PCOBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ ULONG CreateOptions,
    _In_opt_ ULONG ValueType,
    _Out_opt_ PULONG Disposition
    );

// rev
/**
 * The RtlpNtEnumerateSubKey routine is an internal helper that enumerates the subkeys of a registry key by index.
 *
 * \param KeyHandle A handle to the key whose subkeys are enumerated.
 * \param SubKeyName A pointer to a UNICODE_STRING that receives the name of the subkey.
 * \param Index The zero-based index of the subkey to retrieve.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpNtEnumerateSubKey(
    _In_ HANDLE KeyHandle,
    _Inout_ PCUNICODE_STRING SubKeyName,
    _In_ ULONG Index
    );

// rev
/**
 * The RtlpNtMakeTemporaryKey routine is an internal helper that marks a registry key as temporary so that it is deleted when its last handle closes.
 *
 * \param KeyHandle A handle to the key to mark temporary.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpNtMakeTemporaryKey(
    _In_ HANDLE KeyHandle
    );

// rev
/**
 * The RtlpNtOpenKey routine is an internal helper that opens a registry key using simplified parameters.
 *
 * \param KeyHandle A pointer to a variable that receives the handle to the opened key.
 * \param DesiredAccess The access mask requested on the key.
 * \param ObjectAttributes An optional pointer to the OBJECT_ATTRIBUTES that specify the key name and attributes.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpNtOpenKey(
    _Out_ PHANDLE KeyHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Inout_opt_ PCOBJECT_ATTRIBUTES ObjectAttributes
    );

/**
 * The RtlpNtQueryValueKey routine queries the type and data of a registry key value (internal helper for the Rtl registry APIs).
 *
 * \param KeyHandle A handle to the registry key.
 * \param Type An optional pointer to a variable that receives the value type (REG_*).
 * \param Data An optional buffer that receives the value data.
 * \param DataLength On input specifies the buffer size; on output receives the data size, in bytes.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlpNtQueryValueKey(
    _In_ HANDLE KeyHandle,
    _Out_opt_ PULONG Type,
    _Out_writes_bytes_opt_(*DataLength) PVOID Data,
    _Inout_opt_ PINT DataLength
    );

/**
 * The RtlpNtSetValueKey routine sets the type and data of a registry key value (internal helper for the Rtl registry APIs).
 *
 * \param KeyHandle A handle to the registry key.
 * \param Type The value type (REG_*).
 * \param Data An optional buffer containing the value data.
 * \param DataLength The size, in bytes, of Data.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlpNtSetValueKey(
    _In_ HANDLE KeyHandle,
    _In_ ULONG Type,
    _In_reads_bytes_opt_(DataLength) PVOID Data,
    _In_ ULONG DataLength
    );

// rev
/**
 * The RtlpQueryProcessDebugInformationRemote routine is the internal worker thread that collects debug information from a target process.
 *
 * \param DebugInfo A pointer to the RTL_DEBUG_INFORMATION buffer that receives the collected information.
 */
NTSYSAPI
VOID
NTAPI
RtlpQueryProcessDebugInformationRemote(
    _Inout_ PRTL_DEBUG_INFORMATION DebugInfo
    );

// rev
/**
 * The RtlpTimeFieldsToTime routine converts a TIME_FIELDS structure into a system time value, accounting for an optional leap-second context.
 *
 * \param TimeFields A pointer to the TIME_FIELDS structure to convert.
 * \param Time A pointer to a variable that receives the converted system time.
 * \param LeapSecondContext An optional pointer to the leap-second context used during conversion.
 * \return Returns `TRUE` if the conversion succeeds, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlpTimeFieldsToTime(
    _In_ PTIME_FIELDS TimeFields,
    _Out_ PLARGE_INTEGER Time,
    _In_opt_ PLARGE_INTEGER LeapSecondContext
    );

// rev
/**
 * The RtlpTimeToTimeFields routine converts a system time value into a TIME_FIELDS structure, accounting for an optional leap-second context.
 *
 * \param Time A pointer to the system time value to convert.
 * \param TimeFields A pointer to the TIME_FIELDS structure that receives the converted time.
 * \param LeapSecondContext An optional pointer to the leap-second context used during conversion.
 * \return SHORT The number of leap seconds applied during the conversion.
 */
NTSYSAPI
SHORT
NTAPI
RtlpTimeToTimeFields(
    _In_ PLARGE_INTEGER Time,
    _Out_ PTIME_FIELDS TimeFields,
    _In_opt_ PLARGE_INTEGER LeapSecondContext
    );

// rev
/**
 * The RtlpUnWaitCriticalSection routine is an internal helper that wakes a thread waiting on a critical section.
 *
 * \param CriticalSection A pointer to the critical section whose waiter is released.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlpUnWaitCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );

/**
 * The RtlAbortRXact routine aborts a registry transaction (RXact), discarding its pending changes.
 *
 * \param RxactContext The RXact context to abort.
 * \return NTSTATUS Successful or errant status.
 */
//
// General (Rtl)
//

NTSYSAPI
NTSTATUS
NTAPI
RtlAbortRXact(
    _Inout_ PRTL_RXACT_CONTEXT RxactContext
    );

/**
 * The RtlActivateActivationContextUnsafeFast routine activates an activation context on the current thread using the fast, lock-free path.
 *
 * \param CallerFrame A 0x48-byte caller-allocated RTL_CALLER_ALLOCATED_ACTIVATION_CONTEXT_STACK_FRAME_EXTENDED that receives the frame state.
 * \param ActivationContext An optional activation context to activate.
 */
// rev
NTSYSAPI
VOID
FASTCALL
RtlActivateActivationContextUnsafeFast(
    _Out_writes_bytes_(0x48) PVOID CallerFrame,
    _In_opt_ PVOID ActivationContext
    );

// rev
/**
 * The RtlAddActionToRXact routine adds a registry action to a registry transaction (RXACT) context.
 *
 * \param RxactContext A pointer to the RTL_RXACT_CONTEXT to modify.
 * \param ActionType The type of action to add.
 * \param Name A pointer to the Unicode string that names the registry key affected by the action.
 * \param Operation The operation code for the action.
 * \param Data An optional pointer to the data associated with the action.
 * \param DataSize The size, in bytes, of the data.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAddActionToRXact(
    _Inout_ PRTL_RXACT_CONTEXT RxactContext,
    _In_ ULONG ActionType,
    _In_ PCUNICODE_STRING Name,
    _In_ ULONG Operation,
    _In_reads_bytes_opt_(DataSize) const VOID *Data,
    _In_ SIZE_T DataSize
    );

// rev
/**
 * The RtlAddAttributeActionToRXact routine adds a registry value attribute action to a registry transaction (RXACT) context.
 *
 * \param RxactContext A pointer to the RTL_RXACT_CONTEXT to modify.
 * \param ActionType The type of action to add.
 * \param KeyName A pointer to the Unicode string that names the registry key.
 * \param AttributeIndex The index of the attribute affected by the action.
 * \param ValueName A pointer to the Unicode string that names the registry value.
 * \param ValueType The type of the registry value.
 * \param Data An optional pointer to the data associated with the action.
 * \param DataSize The size, in bytes, of the data.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAddAttributeActionToRXact(
    _Inout_ PRTL_RXACT_CONTEXT RxactContext,
    _In_ ULONG ActionType,
    _In_ PCUNICODE_STRING KeyName,
    _In_ LONGLONG AttributeIndex,
    _In_ PCUNICODE_STRING ValueName,
    _In_ ULONG ValueType,
    _In_reads_bytes_opt_(DataSize) const VOID *Data,
    _In_ SIZE_T DataSize
    );

// rev
/**
 * The RtlAllocateActivationContextStack routine allocates an activation context stack for a thread.
 *
 * \param ActivationContextStack A pointer to a variable that receives the allocated activation context stack.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAllocateActivationContextStack(
    _Inout_ PACTIVATION_CONTEXT_STACK* ActivationContextStack
    );

/**
 * The RtlApplicationVerifierStop routine reports an Application Verifier stop, breaking into the debugger with the supplied diagnostic parameters.
 *
 * \param Param1 An optional first diagnostic value.
 * \param Desc1 An optional description of the first value.
 * \param Param2 An optional second diagnostic value.
 * \param Desc2 An optional description of the second value.
 * \param Param3 An optional third diagnostic value.
 * \param Desc3 An optional description of the third value.
 * \param Param4 An optional fourth diagnostic value.
 * \param Desc4 An optional description of the fourth value.
 * \param Param5 An optional fifth diagnostic value.
 * \param Desc5 An optional description of the fifth value.
 * \return Implementation-defined; typically NULL.
 */
// rev
NTSYSAPI
PVOID
NTAPI
RtlApplicationVerifierStop(
    _In_opt_ const VOID *Param1,
    _In_opt_z_ const CHAR *Desc1,
    _In_opt_ const VOID *Param2,
    _In_opt_z_ const CHAR *Desc2,
    _In_opt_ const VOID *Param3,
    _In_opt_z_ const CHAR *Desc3,
    _In_opt_ const VOID *Param4,
    _In_opt_z_ const CHAR *Desc4,
    _In_opt_ const VOID *Param5,
    _In_opt_z_ const CHAR *Desc5
    );

// rev
/**
 * The RtlApplyRXact routine commits the actions accumulated in a registry transaction (RXACT) context and flushes the affected keys.
 *
 * \param RxactContext A pointer to the RTL_RXACT_CONTEXT to commit.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlApplyRXact(
    _Inout_ PRTL_RXACT_CONTEXT RxactContext
    );

// rev
/**
 * The RtlApplyRXactNoFlush routine commits the actions accumulated in a registry transaction (RXACT) context without flushing the affected keys.
 *
 * \param RxactContext A pointer to the RTL_RXACT_CONTEXT to commit.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlApplyRXactNoFlush(
    _Inout_ PRTL_RXACT_CONTEXT RxactContext
    );

// rev
/**
 * The RtlAvlInsertNodeEx routine inserts a node into an AVL tree at a specified parent and side, rebalancing as required.
 *
 * \param Root A pointer to a variable that references the root of the AVL tree.
 * \param Parent An optional pointer to the parent node under which the new node is inserted.
 * \param Right If `TRUE`, the node is inserted as the right child of the parent; otherwise as the left child.
 * \param Node A pointer to the RTL_BALANCED_NODE to insert.
 * \return CHAR The balance adjustment applied to the tree as a result of the insertion.
 */
NTSYSAPI
char
NTAPI
RtlAvlInsertNodeEx(
    _Inout_ PRTL_BALANCED_NODE *Root,
    _In_opt_ PRTL_BALANCED_NODE Parent,
    _In_ BOOLEAN Right,
    _In_ PRTL_BALANCED_NODE Node
    );

/**
 * The RtlCallEnclave routine calls a routine inside a VBS enclave.
 *
 * \param Routine The address of the routine to call within the enclave.
 * \param Reserved Reserved; must be NULL.
 * \param Flags Call flags controlling the enclave transition.
 * \param ReturnValue Receives the value returned by the enclave routine.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlCallEnclave(
    _In_ PVOID Routine,
    _In_opt_ PVOID Reserved,
    _In_ ULONG Flags,
    _Out_ PVOID *ReturnValue
    );

// rev
/**
 * The RtlCallEnclaveReturn routine returns control from an enclave call back to the host.
 *
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCallEnclaveReturn(
    VOID
    );

// rev
/**
 * The RtlCanonicalizeDomainName routine produces the canonical form of a domain name.
 *
 * \param DestinationName A pointer to a UNICODE_STRING that receives the canonicalized name.
 * \param SourceName A pointer to the Unicode string that specifies the domain name to canonicalize.
 * \param AllowInvalidLabels If `TRUE`, labels that are otherwise invalid are permitted.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCanonicalizeDomainName(
    _Out_ PUNICODE_STRING DestinationName,
    _In_ PCUNICODE_STRING SourceName,
    _In_ BOOLEAN AllowInvalidLabels
    );

/**
 * The RtlCapabilityCheckForSingleSessionSku routine determines whether a token holds the specified capability on a single-session SKU.
 *
 * \param TokenHandle A handle to the access token to inspect.
 * \param CapabilityName The name of the capability to check.
 * \param IsCapable Receives TRUE if the token has the capability.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlCapabilityCheckForSingleSessionSku(
    _In_ PVOID TokenHandle,
    _In_ PCUNICODE_STRING CapabilityName,
    _Out_ PBOOLEAN IsCapable
    );

/**
 * The RtlCheckSystemBootStatusIntegrity routine validates the integrity of the boot status data (BSD) context.
 *
 * \param BootStatusContext The boot status data context to validate.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlCheckSystemBootStatusIntegrity(
    _In_ PVOID BootStatusContext
    );

// rev
/**
 * The RtlClearThreadWorkOnBehalfTicket routine clears the work-on-behalf-of ticket associated with the current thread.
 *
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlClearThreadWorkOnBehalfTicket(
    VOID
    );

// rev
/**
 * The RtlCmDecodeMemIoResource routine decodes a memory or I/O resource descriptor and returns its length and translated start address.
 *
 * \param ResourceDescriptor A pointer to the CM_PARTIAL_RESOURCE_DESCRIPTOR to decode.
 * \param TranslatedAddress An optional pointer to a variable that receives the start address of the resource.
 * \return ULONGLONG The length, in bytes, of the decoded resource.
 */
NTSYSAPI
ULONGLONG
NTAPI
RtlCmDecodeMemIoResource(
    _In_ const VOID *ResourceDescriptor,
    _Out_opt_ PULONGLONG TranslatedAddress
    );

/**
 * The RtlCmEncodeMemIoResource routine encodes a memory or I/O resource descriptor for the configuration manager.
 *
 * \param Descriptor Receives the encoded partial resource descriptor.
 * \param Type The resource type.
 * \param Length The length of the resource.
 * \param Start The start address of the resource.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlCmEncodeMemIoResource(
    _Out_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    _In_ UCHAR Type,
    _In_ ULONGLONG Length,
    _In_ ULONGLONG Start
    );

// rev
/**
 * The RtlConstructCrossVmEventPath routine constructs the object path for a cross-VM event identified by two GUIDs.
 *
 * \param ObjectPath A pointer to a UNICODE_STRING that receives the constructed event path.
 * \param Guid1 A pointer to the first GUID that identifies the cross-VM channel.
 * \param Guid2 A pointer to the second GUID that identifies the cross-VM channel.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlConstructCrossVmEventPath(
    _In_ PCUNICODE_STRING ObjectPath,
    _In_ const GUID *Guid1,
    _In_ const GUID *Guid2
    );

// rev
/**
 * The RtlConstructCrossVmMutexPath routine constructs the object path for a cross-VM mutex identified by two GUIDs.
 *
 * \param ObjectPath A pointer to a UNICODE_STRING that receives the constructed mutex path.
 * \param Guid1 A pointer to the first GUID that identifies the cross-VM channel.
 * \param Guid2 A pointer to the second GUID that identifies the cross-VM channel.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlConstructCrossVmMutexPath(
    _In_ PCUNICODE_STRING ObjectPath,
    _In_ const GUID *Guid1,
    _In_ const GUID *Guid2
    );

//NTSYSAPI
//ULONG
//NTAPI
//RtlConvertDeviceFamilyInfoToString(
//    _Inout_ PULONG DeviceFamilyBufferSize,
//    _Inout_ PULONG DeviceFormBufferSize,
//    _Out_writes_bytes_opt_(*DeviceFamilyBufferSize) PWSTR DeviceFamily,
//    _Out_writes_bytes_opt_(*DeviceFormBufferSize) PWSTR DeviceForm
//    );
//
//NTSYSAPI
//VOID
//NTAPI
//RtlGetDeviceFamilyInfoEnum(
//    _Out_opt_ PULONGLONG UapInfo,
//    _Out_opt_ PULONG DeviceFamily,
//    _Out_opt_ PULONG DeviceForm
//    );

/**
 * The RtlDeactivateActivationContextUnsafeFast routine deactivates the activation context previously activated on the current thread using the fast path.
 *
 * \param CallerFrame The 0x48-byte caller frame previously initialized by RtlActivateActivationContextUnsafeFast.
 */
// rev
NTSYSAPI
VOID
FASTCALL
RtlDeactivateActivationContextUnsafeFast(
    _Inout_updates_bytes_(0x48) PVOID CallerFrame
    );

// rev
/**
 * The RtlConvertHostPerfCounterToPerfCounter routine converts a host performance counter value into the guest performance counter timebase.
 *
 * \param HostCounter The host performance counter value to convert.
 * \param MaxDelta The maximum allowed delta used to bound the conversion.
 * \param PerfCounterOut A pointer to a variable that receives the converted performance counter value.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlConvertHostPerfCounterToPerfCounter(
    _In_ ULONGLONG HostCounter,
    _In_ ULONGLONG MaxDelta,
    _Out_ ULONGLONG *PerfCounterOut
    );

// rev
/**
 * The RtlCreateSystemVolumeInformationFolder routine creates the System Volume Information folder on the volume at the specified root path.
 *
 * \param RootPath A pointer to the Unicode string that specifies the volume root path.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateSystemVolumeInformationFolder(
    _In_ PCUNICODE_STRING RootPath
    );

/**
 * The RtlCreateUserFiberShadowStack routine creates a shadow stack for a user-mode fiber (CET shadow-stack support).
 *
 * \param ShadowStackInfo Describes the shadow stack to create.
 * \param ReserveSize The number of bytes to reserve for the shadow stack.
 * \param ShadowStackOut Receives the base of the newly created shadow stack.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateUserFiberShadowStack(
    _In_ PVOID ShadowStackInfo,
    _In_ ULONGLONG ReserveSize,
    _Out_ PVOID *ShadowStackOut
    );

/**
 * The RtlDeleteElementGenericTableAvlEx routine deletes the element matching the supplied key from an AVL generic table.
 *
 * \param Table The AVL generic table to modify.
 * \param Buffer A buffer containing the key that identifies the element to delete.
 * \return A pointer to the restart key of the parent, or NULL if the element was not found.
 */
// rev
NTSYSAPI
PVOID
NTAPI
RtlDeleteElementGenericTableAvlEx(
    _Inout_ PRTL_AVL_TREE Table,
    _In_ PVOID Buffer
    );

// rev
/**
 * The RtlDisownModuleHeapAllocation routine relinquishes ownership of a module's heap allocation so that it is not freed when the module unloads.
 *
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlDisownModuleHeapAllocation(
    VOID
    );

// rev
//NTSYSAPI
//ULONG
//NTAPI
//RtlDrainNonVolatileFlush(
//    _In_ PVOID NvToken
//    );

/**
 * The RtlEnclaveCallDispatchReturn routine returns control from an enclave dispatch call back to the host.
 *
 * \param EnclaveTarget The enclave target address associated with the dispatch.
 * \param LeafRoutine An optional leaf routine to invoke on return.
 * \param LeafNumber The enclave leaf number.
 * \param DispatchContext On input the current dispatch context; receives the updated context.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlEnclaveCallDispatchReturn(
    _In_ PVOID EnclaveTarget,
    _In_opt_ PVOID LeafRoutine,
    _In_ ULONG LeafNumber,
    _Inout_opt_ PVOID *DispatchContext
    );

//
//NTSYSAPI
//PSTR
//NTAPI
//RtlEthernetAddressToStringA(
//    _In_reads_(6) const UCHAR *Addr,
//    _Out_writes_(18) PSTR S
//    );
//
//NTSYSAPI
//PWSTR
//NTAPI
//RtlEthernetAddressToStringW(
//    _In_reads_(6) const UCHAR *Addr,
//    _Out_writes_(18) PWSTR S
//    );
//
//NTSYSAPI
//LONG
//NTAPI
//RtlEthernetStringToAddressA(
//    _In_z_ PCSTR S,
//    _Outptr_ PCSTR *Terminator,
//    _Out_writes_(6) UCHAR *Addr
//    );
//
//NTSYSAPI
//LONG
//NTAPI
//RtlEthernetStringToAddressW(
//    _In_z_ PCWSTR S,
//    _Outptr_ PCWSTR *Terminator,
//    _Out_writes_(6) UCHAR *Addr
//    );
//
//NTSYSAPI
//ULONG
//NTAPI
//RtlExtendCorrelationVector(
//    _Inout_ PVOID CorrelationVector
//    );

/**
 * The RtlExtendMemoryZone routine extends a memory zone by committing an additional segment.
 *
 * \param MemoryZone The memory zone to extend.
 * \param RequestedSize The number of additional bytes to make available.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlExtendMemoryZone(
    _Inout_ PRTL_MEMORY_ZONE MemoryZone,
    _In_ SIZE_T RequestedSize
    );

//
//NTSYSAPI
//ULONG
//NTAPI
//RtlFillNonVolatileMemory(
//    _In_ PVOID NvToken,
//    _Out_writes_bytes_(Size) PVOID NvDestination,
//    _In_ SIZE_T Size,
//    _In_ BYTE Value,
//    _In_ ULONG Flags
//    );

// rev
/**
 * The RtlFreeActivationContextStack routine frees an activation context stack.
 *
 * \param ActivationContextStack An optional pointer to the activation context stack to free.
 */
NTSYSAPI
VOID
NTAPI
RtlFreeActivationContextStack(
    _Inout_opt_ PACTIVATION_CONTEXT_STACK ActivationContextStack
    );

//NTSYSAPI
//NTSTATUS
//NTAPI
//RtlFlushNonVolatileMemory(
//    _In_ UCHAR NvToken,
//    _In_ PVOID BaseAddress,
//    _In_ SIZE_T Length,
//    _In_ UCHAR Flags
//    );
//
//NTSYSAPI
//NTSTATUS
//NTAPI
//RtlFlushNonVolatileMemoryRanges(
//    _In_ UCHAR NvToken,
//    _In_reads_(PairCount) const ULONGLONG *AddressLengthPairs,
//    _In_ SIZE_T PairCount,
//    _In_ UCHAR Flags
//    );
//
// rev
// NTSYSAPI
// NTSTATUS
// NTAPI
// RtlFreeNonVolatileToken(
//     _In_ PVOID NvToken
//     );
//
// rev
//NTSYSAPI
//ULONG
//NTAPI
//RtlGetNonVolatileToken(
//    _In_ PVOID NvBuffer,
//    _In_ SIZE_T Size,
//    _Outptr_ PVOID *NvToken
//    );
//

// rev
/**
 * The RtlFreeThreadActivationContextStack routine frees the activation context stack associated with the current thread.
 */
NTSYSAPI
VOID
NTAPI
RtlFreeThreadActivationContextStack(
    VOID
    );

// rev
/**
 * The RtlFreeUserFiberShadowStack routine frees a user-mode fiber shadow stack allocation.
 *
 * \param AllocationBase The base address of the fiber shadow stack allocation to free.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlFreeUserFiberShadowStack( // NtSetInformationProcess(ProcessFreeFiberShadowStackAllocation)
    _In_ PVOID AllocationBase
    );

// rev
/**
 * The RtlGetFeatureToggleConfiguration routine returns the configuration value for a feature toggle.
 *
 * \param FeatureId The identifier of the feature toggle to query.
 * \param ConfigurationType The type of configuration value to retrieve.
 * \return ULONGLONG The configuration value for the feature toggle.
 */
NTSYSAPI
ULONGLONG
NTAPI
RtlGetFeatureToggleConfiguration(
    _In_ ULONG FeatureId,
    _In_ ULONGLONG ConfigurationType
    );

// rev
/**
 * The RtlGetThreadLangIdByIndex routine retrieves the language identifier for a thread by index.
 *
 * \param Reserved Reserved; must be zero.
 * \param Index The index of the language in the thread's preferred-language list.
 * \param LanguageId Pointer to a variable that receives the language identifier.
 * \param Count Optional pointer to a variable that receives the number of preferred languages.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
LONG
NTAPI
RtlGetThreadLangIdByIndex(
    _Reserved_ ULONG Reserved,
    _In_ ULONG Index,
    _Out_ PULONG LanguageId,
    _Out_opt_ PULONG Count
    );

// rev
/**
 * The RtlGetThreadWorkOnBehalfTicket routine retrieves the work-on-behalf-of ticket associated with the current thread.
 *
 * \param Ticket A pointer to a variable that receives the work-on-behalf-of ticket.
 * \param Flags Flags that control how the ticket is retrieved.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetThreadWorkOnBehalfTicket(
    _Out_ PULONGLONG Ticket,
    _In_ ULONG Flags
    );

// rev
/**
 * The RtlGetSystemBootStatusEx routine retrieves extended boot status data (BSD) information.
 *
 * \param Buffer A buffer that receives the boot status data.
 * \param BufferLength The size, in bytes, of the buffer.
 * \param ReturnLength An optional pointer to a variable that receives the number of bytes returned.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetSystemBootStatusEx(
    _Out_writes_bytes_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_opt_ PULONG ReturnLength
    );

// rev
/**
 * The RtlHeapTrkInitialize routine initializes heap allocation tracking using a shared section.
 *
 * \param SectionHandle A handle to the section used to record heap tracking information.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlHeapTrkInitialize(
    _In_ HANDLE SectionHandle
    );

// rev
//NTSYSAPI
//ULONG
//NTAPI
//RtlIncrementCorrelationVector(
//    _Inout_ PVOID CorrelationVector
//    );
//
//NTSYSAPI
//ULONG
//NTAPI
//RtlInitializeCorrelationVector(
//    _Inout_ PVOID CorrelationVector,
//    _In_ ULONG Version,
//    _In_ const GUID *Guid
//    );

// rev
/**
 * The RtlInitializeRXact routine initializes a registry transaction (RXACT) context rooted at a registry key.
 *
 * \param RootKeyHandle A handle to the registry key that serves as the transaction root.
 * \param CommitIfNecessary If TRUE, any pending transaction log found under the root key is committed during initialization.
 * \param RxactContext A pointer to a variable that receives the initialized transaction context.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeRXact(
    _In_ HANDLE RootKeyHandle,
    _In_ BOOLEAN CommitIfNecessary,
    _Out_ PRTL_RXACT_CONTEXT *RxactContext
    );

// rev
/**
 * The RtlInitializeAtomPackage routine initializes the process-wide atom package.
 *
 * \param Callback An optional callback invoked by the atom package.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeAtomPackage(
    _In_opt_ PVOID Callback
    );

/**
 * The RtlInitializeNtUserPfn routine registers the win32u/NtUser private function (PFN) dispatch tables with ntdll.
 *
 * \param NtUserPfnTable The primary NtUser PFN dispatch table.
 * \param NtUserPfnTableSize The size, in bytes, of the primary table.
 * \param NtUserPfnTable2 An optional secondary NtUser PFN dispatch table.
 * \param NtUserPfnTable2Size The size, in bytes, of the secondary table.
 * \param NtUserPfnTable3 An optional tertiary NtUser PFN dispatch table.
 * \param NtUserPfnTable3Size The size, in bytes, of the tertiary table.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeNtUserPfn(
    _In_ PVOID NtUserPfnTable,
    _In_ SIZE_T NtUserPfnTableSize,
    _In_opt_ PVOID NtUserPfnTable2,
    _In_ SIZE_T NtUserPfnTable2Size,
    _In_opt_ PVOID NtUserPfnTable3,
    _In_ SIZE_T NtUserPfnTable3Size
    );

/**
 * The RtlInterlockedPushListSList routine atomically pushes a list of entries onto a singly linked interlocked list (SLIST).
 *
 * \param Header The SLIST header to push onto.
 * \param List A pointer to the first entry in the list being inserted.
 * \param ListEnd A pointer to the last entry in the list being inserted.
 * \param Count The number of entries in the list being inserted.
 * \return The previous first entry of the list.
 */
// rev
NTSYSAPI
PSLIST_ENTRY
FASTCALL
RtlInterlockedPushListSList(
    _Inout_ PSLIST_HEADER Header,
    _Inout_ PSLIST_ENTRY List,
    _Inout_ PSLIST_ENTRY ListEnd,
    _In_ ULONG Count
    );

/**
 * The RtlIoDecodeMemIoResource routine decodes a memory or I/O resource descriptor into its address and length components.
 *
 * \param ResourceDescriptor The partial resource descriptor to decode.
 * \param TranslatedAddress An optional pointer that receives the translated address.
 * \param StartAddress An optional pointer that receives the start address.
 * \param Length An optional pointer that receives the resource length.
 * \return The decoded resource length.
 */
// rev
NTSYSAPI
ULONGLONG
NTAPI
RtlIoDecodeMemIoResource(
    _In_ PVOID ResourceDescriptor,
    _Out_opt_ PULONGLONG TranslatedAddress,
    _Out_opt_ PULONGLONG StartAddress,
    _Out_opt_ PULONGLONG Length
    );

/**
 * The RtlIoEncodeMemIoResource routine encodes address and length information into a memory or I/O resource descriptor.
 *
 * \param Descriptor The IO resource descriptor to populate.
 * \param Type The resource type.
 * \param Length The resource length.
 * \param Alignment The alignment requirement.
 * \param MinimumAddress The minimum acceptable address.
 * \param MaximumAddress The maximum acceptable address.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlIoEncodeMemIoResource(
    _Out_ PIO_RESOURCE_DESCRIPTOR Descriptor,
    _In_ UCHAR Type,
    _In_ ULONGLONG Length,
    _In_ ULONGLONG Alignment,
    _In_ ULONGLONG MinimumAddress,
    _In_ ULONGLONG MaximumAddress
    );

// rev
/**
 * The RtlIsFeatureEnabledForEnterprise routine determines whether a feature is enabled for the enterprise configuration.
 *
 * \param FeatureId The identifier of the feature to test.
 * \return Returns `TRUE` if the feature is enabled for the enterprise, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsFeatureEnabledForEnterprise(
    _In_ ULONG FeatureId
    );

// rev
/**
 * The RtlIsNameLegalDOS8Dot3 routine determines whether a Unicode name is a legal MS-DOS 8.3-format file name.
 *
 * \param Name A pointer to the Unicode string that specifies the name to test.
 * \param OemName An optional pointer to an OEM string that receives the name in the OEM code page.
 * \param NameContainsSpaces An optional pointer to a variable that receives whether the name contains spaces.
 * \return Returns `TRUE` if the name is a legal 8.3-format name, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsNameLegalDOS8Dot3(
    _In_ PCUNICODE_STRING Name,
    _Out_opt_ POEM_STRING OemName,
    _Out_opt_ PBOOLEAN NameContainsSpaces
    );

// rev
/**
 * The RtlLogStackBackTrace routine captures the current stack back trace and records it in the process stack trace database.
 *
 * \return ULONG An index that identifies the logged stack back trace, or zero on failure.
 */
NTSYSAPI
ULONG
NTAPI
RtlLogStackBackTrace(
    VOID
    );

// rev
/**
 * The RtlLogUnexpectedCodepath routine records that an unexpected code path was reached for diagnostic purposes.
 *
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlLogUnexpectedCodepath(
    VOID
    );

// rev
//NTSYSAPI
//PRUNTIME_FUNCTION
//NTAPI
//RtlLookupFunctionTable(
//    _In_ ULONGLONG ControlPc,
//    _Out_ ULONGLONG *ImageBase,
//    _Out_ ULONG *Length,
//    _In_ ULONGLONG HistoryTable
//    );

/**
 * The RtlpConvertAbsoluteToRelativeSecurityAttribute routine converts a security attribute from absolute to self-relative form.
 *
 * \param AbsoluteSa The absolute-form security attribute information.
 * \param RelativeSa A buffer that receives the self-relative security attribute information.
 * \param RelativeSaLength On input specifies the buffer size; on output receives the required or written length, in bytes.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlpConvertAbsoluteToRelativeSecurityAttribute(
    _In_ PVOID AbsoluteSa,
    _Out_ PVOID RelativeSa,
    _Inout_ ULONG *RelativeSaLength
    );

// rev
/**
 * The RtlMapSecurityErrorToNtStatus routine maps an SSPI security status code to the corresponding NTSTATUS value.
 *
 * \param SecurityStatus The security status code to map.
 * \return NTSTATUS The NTSTATUS value that corresponds to the security status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlMapSecurityErrorToNtStatus(
    _In_ LONG SecurityStatus
    );

// NTSYSAPI
// PVOID
// NTAPI
// RtlMoveMemory(
//     _Out_writes_bytes_all_(Length) PVOID Destination,
//     _In_reads_bytes_(Length) const VOID *Source,
//     _In_ SIZE_T Length
//     );

/**
 * The RtlOpenCrossProcessEmulatorWorkConnection routine opens a shared-memory connection used to exchange cross-process emulator work items.
 *
 * \param ProcessHandle A handle to the target process.
 * \param SectionHandle Receives a handle to the shared section.
 * \param ViewBase Receives the base address of the mapped view.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlOpenCrossProcessEmulatorWorkConnection(
    _In_ HANDLE ProcessHandle,
    _Out_ PHANDLE SectionHandle,
    _Outptr_ PVOID *ViewBase
    );

// NTSYSAPI
// ULONG
// NTAPI
// RtlOsDeploymentState(
//     _In_ ULONG Flags
//     );

/**
 * The RtlQueryDynamicTimeZoneInformation routine retrieves the current dynamic time zone information for the system.
 *
 * \param DynamicTimeZoneInformation Receives the dynamic time zone information.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryDynamicTimeZoneInformation(
    _Out_ PRTL_DYNAMIC_TIME_ZONE_INFORMATION DynamicTimeZoneInformation
    );

/**
 * The RtlQueryInternalFeatureConfiguration routine queries the configuration of an internal (velocity/feature-staging) feature.
 *
 * \param FeatureId The identifier of the feature to query.
 * \param QueryFlags Flags controlling the query.
 * \param ChangeStamp An optional pointer that receives the configuration change stamp.
 * \param FeatureConfiguration Receives the feature configuration.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryInternalFeatureConfiguration(
    _In_ ULONG FeatureId,
    _In_ ULONG QueryFlags,
    _Out_opt_ PULONGLONG ChangeStamp,
    _Out_ PVOID FeatureConfiguration
    );

/**
 * The RtlQueryModuleInformation routine retrieves information about the modules loaded in the system.
 *
 * \param BufferSize On input specifies the buffer size; on output receives the required or written size, in bytes.
 * \param UnitSize The size, in bytes, of each module information record (RTL_QUERY_MODULE_INFORMATION_RECORD_SIZE_*).
 * \param ModuleInformation An optional buffer that receives the module information records.
 * \return NTSTATUS Successful or errant status.
 */
// RtlQueryModuleInformation only accepts record sizes 0x8 and 0x110.
// UnitSize is treated as an unsigned selector, and the ModuleInformation
// buffer layout depends on the selected record size.
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryModuleInformation(
    _Inout_ PULONG BufferSize,
    _In_ ULONG UnitSize, // RTL_QUERY_MODULE_INFORMATION_RECORD_SIZE_*
    _Out_writes_bytes_opt_(*BufferSize) PVOID ModuleInformation
    );

// rev
// Reserved must be 0 and ValueSize must be sizeof(ULONG).
/**
 * The RtlQueryResourcePolicy routine retrieves the value of a system resource policy.
 *
 * \param PolicyClass The RTL_RESOURCE_POLICY_CLASS that identifies the policy to query.
 * \param Reserved Reserved; must be zero.
 * \param PolicyValue A pointer to a variable that receives the policy value.
 * \param ValueSize The size, in bytes, of the value buffer; must be sizeof(ULONG).
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryResourcePolicy(
    _In_ RTL_RESOURCE_POLICY_CLASS PolicyClass,
    _Reserved_ ULONG Reserved,
    _Out_ PULONG PolicyValue,
    _In_ ULONG ValueSize
    );

// rev
//NTSYSAPI
//ULONG
//NTAPI
//RtlRaiseCustomSystemEventTrigger(
//    _In_ PVOID TriggerConfig
//    );

/**
 * The RtlRegisterForWnfMetaNotification routine registers a callback to receive WNF meta-notifications for state-name changes.
 *
 * \param Subscription Receives the subscription pointer.
 * \param StateName The WNF state name to monitor.
 * \param DeliveryFlags Flags controlling notification delivery.
 * \param Callback A callback invoked when a meta-notification is delivered.
 * \param CallbackContext An optional context value passed to the callback.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlRegisterForWnfMetaNotification(
    _Out_ PVOID *Subscription,
    _In_ ULONGLONG StateName,
    _In_ ULONG DeliveryFlags,
    _In_ PVOID Callback,
    _In_opt_ PVOID CallbackContext
    );

/**
 * The RtlReportSqmEscalation routine reports a Software Quality Metrics (SQM) escalation event.
 *
 * \param Callback A callback that supplies the escalation data.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlReportSqmEscalation(
    _In_ PVOID Callback
    );

/**
 * The RtlResetNtUserPfn routine resets (unregisters) the win32u/NtUser private function (PFN) dispatch tables.
 *
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlResetNtUserPfn(
    VOID
    );

// rev
/**
 * The RtlRetrieveNtUserPfn routine retrieves the win32k user-mode callback function-pointer tables.
 *
 * \param NtUserPfnTable A pointer to a variable that receives the address of the first user PFN table.
 * \param NtUserPfnTable2 A pointer to a variable that receives the address of the second user PFN table.
 * \param NtUserPfnTable3 A pointer to a variable that receives the address of the third user PFN table.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlRetrieveNtUserPfn(
    _Out_ PULONGLONG NtUserPfnTable,
    _Out_ PULONGLONG NtUserPfnTable2,
    _Out_ PULONGLONG NtUserPfnTable3
    );

// rev
/**
 * The RtlSetDynamicTimeZoneInformation routine sets the system dynamic time zone information.
 *
 * \param DynamicTimeZoneInformation A pointer to the DYNAMIC_TIME_ZONE_INFORMATION to apply.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetDynamicTimeZoneInformation(
    _In_ PDYNAMIC_TIME_ZONE_INFORMATION DynamicTimeZoneInformation
    );

/**
 * The RtlSetSystemBootStatusEx routine sets extended boot status data (BSD) information.
 *
 * \param Buffer A buffer containing the boot status data to set.
 * \param BufferLength The size, in bytes, of the buffer.
 * \param Reserved Reserved; must be NULL.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlSetSystemBootStatusEx(
    _In_ PVOID Buffer,
    _In_ ULONG BufferLength,
    _In_opt_ PVOID Reserved
    );

// rev
/**
 * The RtlSetThreadWorkOnBehalfTicket routine sets the work-on-behalf-of ticket associated with the current thread.
 *
 * \param Ticket A pointer to the work-on-behalf-of ticket to set.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetThreadWorkOnBehalfTicket(
    _In_ PULONGLONG Ticket
    );

/**
 * The RtlStartRXact routine begins a registry transaction (RXact), allocating and initializing its context.
 *
 * \param RxactContext The RXact context whose transaction buffer is allocated and initialized.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlStartRXact(
    _Inout_ PRTL_RXACT_CONTEXT RxactContext
    );

// rev
//NTSYSAPI
//ULONG
//NTAPI
//RtlSwitchedVVI(
//    _In_ PRTL_OSVERSIONINFOEXW VersionInfo,
//    _In_ ULONG TypeMask,
//    _In_ ULONGLONG ConditionMask
//    );

/**
 * The RtlTestAndPublishWnfStateData routine publishes new WNF state data only if the current change stamp matches the expected value.
 *
 * \param StateName The WNF state name to publish to.
 * \param TypeId An optional type identifier describing the data.
 * \param Buffer An optional buffer containing the state data to publish.
 * \param BufferSize The size, in bytes, of Buffer.
 * \param ExplicitScope An optional explicit scope for the state name.
 * \param MatchingChangeStamp The change stamp the current state must match for the publish to occur.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlTestAndPublishWnfStateData(
    _In_ ULONGLONG StateName,
    _In_opt_ PVOID TypeId,
    _In_reads_bytes_opt_(BufferSize) const VOID *Buffer,
    _In_ ULONG BufferSize,
    _In_opt_ PVOID ExplicitScope,
    _In_ ULONG MatchingChangeStamp
    );

// rev
/**
 * The RtlTryConvertSRWLockSharedToExclusiveOrRelease routine attempts to upgrade a slim reader/writer (SRW) lock from shared to exclusive mode, releasing the shared lock if the upgrade cannot be performed.
 *
 * \param SRWLock A pointer to the SRW lock to convert.
 * \return Returns `TRUE` if the lock was upgraded to exclusive mode, otherwise `FALSE`.
 */
_Requires_shared_lock_held_(*SRWLock)
NTSYSAPI
BOOLEAN
NTAPI
RtlTryConvertSRWLockSharedToExclusiveOrRelease(
    _Inout_ volatile RTL_SRWLOCK *SRWLock
    );

// rev
/**
 * The RtlUdiv128 routine divides an unsigned 128-bit value by an unsigned 64-bit divisor.
 *
 * \param DividendHigh The high 64 bits of the 128-bit dividend.
 * \param DividendLow The low 64 bits of the 128-bit dividend.
 * \param Divisor The 64-bit divisor.
 * \param Remainder An optional pointer to a variable that receives the remainder.
 * \return ULONGLONG The 64-bit quotient of the division.
 */
NTSYSAPI
ULONGLONG
NTAPI
RtlUdiv128(
    _In_ ULONGLONG DividendHigh,
    _In_ ULONGLONG DividendLow,
    _In_ ULONGLONG Divisor,
    _Out_opt_ PLONGLONG Remainder
    );

// rev
/**
 * The RtlUmsThreadYield routine yields execution of the current user-mode scheduling (UMS) thread back to the UMS scheduler.
 *
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlUmsThreadYield(
    _In_ PVOID SchedulerParam
    );

//
//NTSYSAPI
//VOID
//NTAPI
//RtlUnwindEx(
//    VOID
//    );

// rev
/**
 * The RtlUserFiberStart routine is the internal entry point that begins execution of a user-mode fiber.
 */
DECLSPEC_NORETURN
NTSYSAPI
VOID
NTAPI
RtlUserFiberStart(
    VOID
    );

//
//NTSYSAPI
//ULONG
//NTAPI
//RtlValidateCorrelationVector(
//    _In_ PVOID CorrelationVector
//    );
//
//NTSYSAPI
//PEXCEPTION_ROUTINE
//NTAPI
//RtlVirtualUnwind(
//    _In_ ULONG HandlerType,
//    _In_ ULONGLONG ImageBase,
//    _In_ ULONGLONG ControlPc,
//    _In_ PRUNTIME_FUNCTION FunctionEntry,
//    _Inout_ PCONTEXT ContextRecord,
//    _Outptr_ PVOID *HandlerData,
//    _Out_ PULONGLONG EstablisherFrame,
//    _Inout_opt_ PVOID ContextPointers
//    );
//
//NTSYSAPI
//NTSTATUS
//NTAPI
//RtlVirtualUnwind2(
//    _In_ ULONG HandlerType,
//    _In_ ULONGLONG ImageBase,
//    _In_ char *ControlPc,
//    _In_ PULONG FunctionEntry,
//    _Inout_ PULONG ContextRecord,
//    _Inout_opt_ PUCHAR UnwindHistoryTable,
//    _Inout_opt_ PULONGLONG HandlerData,
//    _Inout_opt_ char ***EstablisherFrame,
//    _In_opt_ PVOID ContextPointers,
//    _In_opt_ PVOID Reserved1,
//    _In_opt_ PVOID Reserved2,
//    _Inout_opt_ PULONGLONG MachineFrameUnwound,
//    _In_ ULONG Flags
//    );

/**
 * The RtlWaitForWnfMetaNotification routine waits for a WNF meta-notification on the specified state name.
 *
 * \param StateName The WNF state name to wait on.
 * \param WaitFlags Flags controlling the wait.
 * \param TimeoutMs The wait timeout, in milliseconds.
 * \param Reserved Reserved; must be NULL.
 * \param ResultFlags Receives flags describing why the wait completed.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlWaitForWnfMetaNotification(
    _In_ ULONGLONG StateName,
    _In_ ULONG WaitFlags,
    _In_ ULONG TimeoutMs,
    _In_opt_ PVOID Reserved,
    _Out_ PINT ResultFlags
    );

//
//NTSYSAPI
//ULONG
//NTAPI
//RtlWriteNonVolatileMemory(
//    _In_ PVOID NvToken,
//    _Out_writes_bytes_(Size) PVOID NvDestination,
//    _In_reads_bytes_(Size) const VOID *Source,
//    _In_ SIZE_T Size,
//    _In_ ULONG Flags
//    );

/**
 * The RtlXRestore routine restores extended processor state (XSAVE area) for the specified feature mask.
 *
 * \param XStateContext The extended state (XSAVE) area to restore from.
 * \param FeatureMask A bitmask of processor state components to restore.
 * \return The processor XState restore result.
 */
// rev
NTSYSAPI
ULONGLONG
NTAPI
RtlXRestore(
    _In_ PVOID XStateContext,
    _In_ ULONGLONG FeatureMask
    );

// rev
/**
 * The RtlXSave routine saves the specified extended processor state features into an extended state context.
 *
 * \param XStateContext A pointer to the extended state context buffer that receives the saved state.
 * \param FeatureMask A mask of the extended state features to save.
 * \return ULONGLONG The mask of features actually saved.
 */
NTSYSAPI
ULONGLONG
NTAPI
RtlXSave(
    _Inout_ PULONG XStateContext,
    _In_ ULONGLONG FeatureMask
    );

//
// Stack support
//

/**
 * The RtlPushFrame routine pushes an active frame onto the current thread's active frame stack.
 *
 * \param Frame A pointer to the TEB_ACTIVE_FRAME to push.
 */
NTSYSAPI
VOID
NTAPI
RtlPushFrame(
    _In_ PTEB_ACTIVE_FRAME Frame
    );

/**
 * The RtlPopFrame routine pops an active frame from the current thread's active frame stack.
 *
 * \param Frame A pointer to the TEB_ACTIVE_FRAME to pop.
 */
NTSYSAPI
VOID
NTAPI
RtlPopFrame(
    _In_ PTEB_ACTIVE_FRAME Frame
    );

/**
 * The RtlGetFrame routine returns the topmost active frame on the current thread's active frame stack.
 *
 * \return PTEB_ACTIVE_FRAME A pointer to the current active frame, or `NULL` if the stack is empty.
 */
NTSYSAPI
PTEB_ACTIVE_FRAME
NTAPI
RtlGetFrame(
    VOID
    );

/**
 * Flags controlling RtlWalkFrameChain stack capture.
 */
#define RTL_WALK_USER_MODE_STACK 0x00000001
#define RTL_WALK_KERNEL_STACK 0x00000002
#define RTL_WALK_USER_KERNEL_STACK 0x00000003
#define RTL_WALK_VALID_FLAGS 0x00000006
#define RTL_STACK_WALKING_MODE_FRAMES_TO_SKIP_SHIFT 0x00000008

/**
 * The RtlWalkFrameChain routine captures the current call stack as an array of return addresses.
 *
 * \param Callers A buffer that receives the captured return addresses.
 * \param Count The maximum number of frames to capture.
 * \param Flags Flags controlling the walk, including the number of leading frames to skip.
 * \return The number of frames captured.
 */
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
/**
 * The RtlGetCallersAddress routine retrieves the return addresses of the caller and caller's caller.
 *
 * \param CallersAddress A pointer that receives the caller's return address.
 * \param CallersCaller A pointer that receives the return address of the caller's caller.
 * \remarks This routine is deprecated. Callers should prefer the intrinsic _ReturnAddress.
 */
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
 * \param FeatureMask A 64-bit feature mask. This parameter indicates a set of extended processor features for which the caller
 * requests information about whether the features are enabled.
 * \return A 64-bitmask of enabled extended processor features. The routine calculates this mask as the intersection (bitwise AND)
 * between all enabled features and the value of the FeatureMask parameter.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlgetenabledextendedfeatures
 */
NTSYSAPI
ULONG64
NTAPI
RtlGetEnabledExtendedFeatures(
    _In_ ULONG64 FeatureMask
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS4)

// msdn
/**
 * The RtlGetEnabledExtendedAndSupervisorFeatures routine returns the enabled extended and supervisor processor state features that intersect a caller-supplied mask.
 *
 * \param FeatureMask A mask of the features to test.
 * \return ULONG64 The subset of FeatureMask corresponding to enabled extended and supervisor features.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlgetenabledextendedandsupervisorfeatures
 */
NTSYSAPI
ULONG64
NTAPI
RtlGetEnabledExtendedAndSupervisorFeatures(
    _In_ ULONG64 FeatureMask
    );

/**
 * The RtlLocateSupervisorFeature routine locates the save area of a supervisor extended processor feature within an XSAVE header.
 *
 * \param XStateHeader The XSAVE area header to search.
 * \param FeatureId The XSTATE supervisor feature identifier to locate.
 * \param Length An optional pointer that receives the length, in bytes, of the feature save area.
 * \return A pointer to the feature save area, or NULL if the feature is not present.
 */
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

/**
 * Token elevation state flags.
 */
#define ELEVATION_FLAG_TOKEN_CHECKS 0x00000001
#define ELEVATION_FLAG_VIRTUALIZATION 0x00000002
#define ELEVATION_FLAG_SHORTCUT_REDIR 0x00000004
#define ELEVATION_FLAG_NO_SIGNATURE_CHECK 0x00000008

// private
/**
 * Describes the User Account Control (UAC) elevation state of the current token.
 */
typedef struct _RTL_ELEVATION_FLAGS
{
    union
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
    };
} RTL_ELEVATION_FLAGS, *PRTL_ELEVATION_FLAGS;

// private
/**
 * The RtlQueryElevationFlags routine retrieves the User Account Control (UAC) elevation flags for the system.
 *
 * \param Flags A pointer to a variable that receives the RTL_ELEVATION_FLAGS.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryElevationFlags(
    _Out_ PRTL_ELEVATION_FLAGS Flags
    );

// private
/**
 * Identifies a client/server runtime (CSR) subsystem.
 */
typedef enum _CSR_SUBSYSTEM_ID
{
    CsrSubsystemIdUnknown = 0,
    CsrSubsystemIdWindows = 1,
    CsrSubsystemIdPosix = 2,
    CsrSubsystemIdOs2 = 3
} CSR_SUBSYSTEM_ID;

// rev
/**
 * Header describing per-subsystem data shared with the CSR server.
 */
typedef struct _CSR_SUBSYSTEM_DATA_HEADER
{
    ULONG SubsystemId;
    ULONG DataSize; // version/reserved or padding
} CSR_SUBSYSTEM_DATA_HEADER, *PCSR_SUBSYSTEM_DATA_HEADER;

// rev
/**
 * The RtlGetPerSubsystemServerData routine retrieves a pointer to the server-side data for a specific subsystem.
 *
 * \param SubsystemId The ID of the subsystem (e.g., CsrSubsystemIdWindows).
 * \return A pointer to the data block following the header, or NULL if not found.
 */
FORCEINLINE
PVOID
NTAPI
RtlGetPerSubsystemServerData(
    _In_ ULONG SubsystemId
    )
{

    PVOID* StaticServerData;

    StaticServerData = NtCurrentPeb()->ReadOnlyStaticServerData;

    if (StaticServerData)
    {
        while (*StaticServerData)
        {
            PCSR_SUBSYSTEM_DATA_HEADER SubsystemData = (PCSR_SUBSYSTEM_DATA_HEADER)*StaticServerData;

            if (SubsystemData->SubsystemId == SubsystemId)
            {
                return RTL_PTR_ADD(SubsystemData, sizeof(CSR_SUBSYSTEM_DATA_HEADER));
            }

            StaticServerData++;
       }
   }
   return NULL;
}

// private
/**
 * The RtlRegisterThreadWithCsrss routine registers the current thread with the client/server runtime subsystem (CSRSS).
 *
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlRegisterThreadWithCsrss(
    VOID
    );

// private
/**
 * The RtlLockCurrentThread routine locks the current thread to prevent it from being suspended or terminated during a critical operation.
 *
 * \return NTSTATUS Successful or errant status.
 */
_Acquires_lock_(NtCurrentThread())
NTSYSAPI
NTSTATUS
NTAPI
RtlLockCurrentThread(
    VOID
    );

// private
/**
 * The RtlUnlockCurrentThread routine releases a lock previously acquired by RtlLockCurrentThread on the current thread.
 *
 * \return NTSTATUS Successful or errant status.
 */
_Releases_lock_(NtCurrentThread())
NTSYSAPI
NTSTATUS
NTAPI
RtlUnlockCurrentThread(
    VOID
    );

/**
 * The RtlLockModuleSection routine locks the image section containing the specified address into the working set.
 *
 * \param Address An address within the module section to lock.
 * \return NTSTATUS Successful or errant status.
 */
// private
_Acquires_lock_(Address)
NTSYSAPI
NTSTATUS
NTAPI
RtlLockModuleSection(
    _In_ PVOID Address
    );

/**
 * The RtlUnlockModuleSection routine unlocks the image section containing the specified address.
 *
 * \param Address An address within the module section to unlock.
 * \return NTSTATUS Successful or errant status.
 */
// private
_Releases_lock_(Address)
NTSYSAPI
NTSTATUS
NTAPI
RtlUnlockModuleSection(
    _In_ PVOID Address
    );

/**
 * Maximum number of module unload event trace records retained.
 */
#define RTL_UNLOAD_EVENT_TRACE_NUMBER 64

// private
/**
 * The RTL_UNLOAD_EVENT_TRACE structure contains information about modules unloaded by the current process.
 *
 * \sa https://learn.microsoft.com/en-us/windows/win32/devnotes/rtlgetunloadeventtrace
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

/**
 * 32-bit form of a module unload event trace record.
 */
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
 * The RtlGetUnloadEventTrace routine enables the dump code to get the unloaded module information from Ntdll.dll for storage in the minidump.
 *
 * \return A pointer to an array of unload events.
 * \sa https://learn.microsoft.com/en-us/windows/win32/devnotes/rtlgetunloadeventtrace
 */
NTSYSAPI
PRTL_UNLOAD_EVENT_TRACE
NTAPI
RtlGetUnloadEventTrace(
    VOID
    );

/**
 * The RtlGetUnloadEventTraceEx routine retrieves the size and location of the dynamically unloaded module list for the current process.
 *
 * \param ElementSize A pointer to a variable that contains the size of an element in the list.
 * \param ElementCount A pointer to a variable that contains the number of elements in the list.
 * \param EventTrace A pointer to an array of RTL_UNLOAD_EVENT_TRACE structures.
 * \return A pointer to an array of unload events.
 * \sa https://learn.microsoft.com/en-us/windows/win32/devnotes/rtlgetunloadeventtraceex
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
 * \param FramesToSkip Number of frames to skip from the start (current call point) of the back trace.
 * \param FramesToCapture Number of frames to be captured.
 * \param BackTrace Caller-allocated array in which pointers to the return addresses captured from the current stack trace are returned.
 * \param BackTraceHash Optional value that can be used to organize hash tables. This hash value is calculated based on the values of the pointers returned in the BackTrace array. Two identical stack traces will generate identical hash values.
 * \return The number of captured frames.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlcapturestackbacktrace
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
 * The RtlCaptureContext routine retrieves a context record in the context of the caller.
 *
 * \param ContextRecord A pointer to a CONTEXT structure.
 * \return This function does not return a value.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlcapturecontext
 */
NTSYSAPI
VOID
NTAPI
RtlCaptureContext(
    _Out_ PCONTEXT ContextRecord
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)
/**
 * The RtlCaptureContext2 routine retrieves an extended context record for the calling thread, updating the supplied context in place.
 *
 * \param ContextRecord A pointer to a CONTEXT structure that receives the captured thread context.
 */
NTSYSAPI
VOID
NTAPI
RtlCaptureContext2(
    _Inout_ PCONTEXT ContextRecord
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_20H1

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
/**
 * The RtlRestoreContext routine restores the context of the calling thread from the specified context record, optionally applying a longjump or unwind exception.
 *
 * \param ContextRecord A pointer to the CONTEXT structure to restore.
 * \param ExceptionRecord An optional pointer to an EXCEPTION_RECORD that describes a longjump or unwind operation.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/nf-winnt-rtlrestorecontext
 */
NTSYSAPI
VOID
STDAPIVCALLTYPE
RtlRestoreContext(
    _In_ PCONTEXT ContextRecord,
    _In_opt_ struct _EXCEPTION_RECORD* ExceptionRecord
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11

/**
 * The RtlUnwind routine initiates an unwind of procedure call frames up to a target frame.
 *
 * \param TargetFrame An optional target frame at which the unwind terminates.
 * \param TargetIp An optional continuation address to transfer control to after unwinding.
 * \param ExceptionRecord An optional exception record describing the reason for the unwind.
 * \param ReturnValue A value placed in the integer return register on completion.
 */
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
/**
 * The RtlAddFunctionTable routine adds a dynamic function table to the dynamic function table list.
 *
 * \param FunctionTable A pointer to an array of function entries that describe the dynamically generated code.
 * \param EntryCount The number of entries in the FunctionTable array.
 * \param BaseAddress The base address used when computing full virtual addresses from the relative virtual addresses of the entries.
 * \return Returns `TRUE` if the function table was added successfully, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/nf-winnt-rtladdfunctiontable
 */
NTSYSAPI
BOOLEAN
STDAPIVCALLTYPE
RtlAddFunctionTable(
    _In_reads_(EntryCount) PRUNTIME_FUNCTION FunctionTable,
    _In_ ULONG EntryCount,
    _In_ ULONG64 BaseAddress
    );

/**
 * The RtlDeleteFunctionTable routine removes a dynamic function table from the dynamic function table list.
 *
 * \param FunctionTable A pointer to the function-entry array previously passed to RtlAddFunctionTable, or an identifier previously passed to RtlInstallFunctionTableCallback.
 * \return Returns `TRUE` if the function table was removed successfully, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/nf-winnt-rtldeletefunctiontable
 */
NTSYSAPI
BOOLEAN
STDAPIVCALLTYPE
RtlDeleteFunctionTable(
    _In_ PRUNTIME_FUNCTION FunctionTable
    );

/**
 * The RtlInstallFunctionTableCallback routine adds a dynamic function table whose entries are produced on demand by a callback routine.
 *
 * \param TableIdentifier The identifier for the dynamic function table; the two low-order bits must be set.
 * \param BaseAddress The base address of the region of memory managed by the callback.
 * \param Length The size, in bytes, of the region of memory managed by the callback.
 * \param Callback A pointer to the routine that returns the function-table entries for addresses in the region.
 * \param Context An optional caller-defined value passed to the callback routine.
 * \param OutOfProcessCallbackDll An optional path to a DLL that provides function-table entries for out-of-process access, such as by a debugger.
 * \return Returns `TRUE` if the callback was installed successfully, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/nf-winnt-rtlinstallfunctiontablecallback
 */
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
/**
 * The RtlAddGrowableFunctionTable routine registers a growable dynamic function table for exception handling of dynamically generated code.
 *
 * \param DynamicTable Receives an opaque handle identifying the registered table.
 * \param FunctionTable An array of runtime function entries describing the code.
 * \param EntryCount The number of currently valid entries in FunctionTable.
 * \param MaximumEntryCount The maximum number of entries the table can grow to.
 * \param RangeBase The base address of the code range covered by the table.
 * \param RangeEnd The end address of the code range covered by the table.
 * \return An NTSTATUS-style status code returned as ULONG.
 */
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

/**
 * The RtlGrowFunctionTable routine increases the number of valid entries in a growable dynamic function table.
 *
 * \param DynamicTable The handle returned by RtlAddGrowableFunctionTable.
 * \param NewEntryCount The new number of valid entries in the table.
 */
NTSYSAPI
VOID
NTAPI
RtlGrowFunctionTable(
    _Inout_ PVOID DynamicTable,
    _In_ ULONG NewEntryCount
    );

/**
 * The RtlDeleteGrowableFunctionTable routine unregisters a growable dynamic function table.
 *
 * \param DynamicTable The handle returned by RtlAddGrowableFunctionTable.
 */
NTSYSAPI
VOID
NTAPI
RtlDeleteGrowableFunctionTable(
    _In_ PVOID DynamicTable
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8
#endif // _M_AMD64 && _M_ARM64EC

#if defined(_M_ARM64EC)
/**
 * The RtlIsEcCode routine determines whether the specified code pointer refers to ARM64EC (emulation-compatible) code.
 *
 * \param CodePointer The code address to test.
 * \return Returns `TRUE` if the address refers to ARM64EC code, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsEcCode(
    _In_ ULONG64 CodePointer
    );
#endif // _M_ARM64EC

/**
 * The RtlLookupFunctionEntry routine searches the active function tables for an entry that corresponds to the specified PC value.
 *
 * \param ControlPc The virtual address of an instruction bundle within the function.
 * \param ImageBase The base address of module to which the function belongs.
 * \return The entry in the function table for the specified PC.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/nf-winnt-rtllookupfunctionentry
 */
// NTSYSAPI
// PRUNTIME_FUNCTION
// NTAPI
// RtlLookupFunctionEntry(
//     _In_ ULONG_PTR ControlPc,
//     _Out_ PULONG_PTR ImageBase,
//     _Inout_opt_ PUNWIND_HISTORY_TABLE HistoryTable
//     );

/**
 * The RtlPcToFileHeader routine retrieves the base address of the image that contains the specified PC value.
 *
 * \param PcValue The PC value. The function searches all modules mapped into the address space of the calling process for a module that contains this value.
 * \param BaseOfImage The base address of the image containing the PC value. This value must be added to any relative addresses in the headers to locate the image.
 * \return If the PC value is found, returns the base address of the image that contains the PC value. If no image contains the PC value, the function returns NULL.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/winnt/nf-winnt-rtlpctofileheader
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
 * The RtlQueryPerformanceCounter routine retrieves the current value of the performance counter, which is a high resolution (<1us) time stamp that can be used for time-interval measurements.
 *
 * \param PerformanceCounter A pointer to a variable that receives the current performance-counter value, in counts.
 * \return Returns TRUE if the function succeeds, otherwise FALSE. On systems that run Windows XP or later, the function will always succeed and will thus never return zero.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancecounter
 */
NTSYSAPI
LOGICAL
NTAPI
RtlQueryPerformanceCounter(
    _Out_ PLARGE_INTEGER PerformanceCounter
    );

// rev
/**
 * The RtlQueryPerformanceFrequency routine retrieves the frequency of the performance counter. The frequency of the performance counter is fixed at system boot and is consistent across all processors.
 * Therefore, the frequency need only be queried upon application initialization, and the result can be cached.
 *
 * \param PerformanceFrequency A pointer to a variable that receives the current performance-counter frequency, in counts per second.
 * \return Returns TRUE if the function succeeds, otherwise FALSE. On systems that run Windows XP or later, the function will always succeed and will thus never return zero.
 * \sa https://learn.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancefrequency
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
/**
 * Identifies an image mitigation policy category.
 */
typedef enum _IMAGE_MITIGATION_POLICY
{
    ImageDepPolicy,                     // RTL_IMAGE_MITIGATION_DEP_POLICY
    ImageAslrPolicy,                    // RTL_IMAGE_MITIGATION_ASLR_POLICY
    ImageDynamicCodePolicy,             // RTL_IMAGE_MITIGATION_DYNAMIC_CODE_POLICY
    ImageStrictHandleCheckPolicy,       // RTL_IMAGE_MITIGATION_STRICT_HANDLE_CHECK_POLICY
    ImageSystemCallDisablePolicy,       // RTL_IMAGE_MITIGATION_SYSTEM_CALL_DISABLE_POLICY
    ImageMitigationOptionsMask,
    ImageExtensionPointDisablePolicy,   // RTL_IMAGE_MITIGATION_EXTENSION_POINT_DISABLE_POLICY
    ImageControlFlowGuardPolicy,        // RTL_IMAGE_MITIGATION_CONTROL_FLOW_GUARD_POLICY
    ImageSignaturePolicy,               // RTL_IMAGE_MITIGATION_BINARY_SIGNATURE_POLICY
    ImageFontDisablePolicy,             // RTL_IMAGE_MITIGATION_FONT_DISABLE_POLICY
    ImageImageLoadPolicy,               // RTL_IMAGE_MITIGATION_IMAGE_LOAD_POLICY
    ImagePayloadRestrictionPolicy,      // RTL_IMAGE_MITIGATION_PAYLOAD_RESTRICTION_POLICY
    ImageChildProcessPolicy,            // RTL_IMAGE_MITIGATION_CHILD_PROCESS_POLICY
    ImageSehopPolicy,                   // RTL_IMAGE_MITIGATION_SEHOP_POLICY
    ImageHeapPolicy,                    // RTL_IMAGE_MITIGATION_HEAP_POLICY
    ImageUserShadowStackPolicy,         // RTL_IMAGE_MITIGATION_USER_SHADOW_STACK_POLICY
    ImageRedirectionTrustPolicy,        // RTL_IMAGE_MITIGATION_REDIRECTION_TRUST_POLICY
    ImageUserPointerAuthPolicy,         // RTL_IMAGE_MITIGATION_USER_POINTER_AUTH_POLICY
    MaxImageMitigationPolicy
} IMAGE_MITIGATION_POLICY;

// rev
/**
 * Describes the state of an image mitigation policy option.
 */
typedef union _RTL_IMAGE_MITIGATION_POLICY
{
    struct
    {
        ULONG64 AuditState : 2;
        ULONG64 AuditFlag : 1;
        ULONG64 EnableAdditionalAuditingOption : 1;
        ULONG64 Reserved : 60;
    } DUMMYSTRUCTNAME;
    struct
    {
        ULONG64 PolicyState : 2;
        ULONG64 AlwaysInherit : 1;
        ULONG64 EnableAdditionalPolicyOption : 1;
        ULONG64 AuditReserved : 60;
    } DUMMYSTRUCTNAME2;
} RTL_IMAGE_MITIGATION_POLICY, *PRTL_IMAGE_MITIGATION_POLICY;

// rev
/**
 * Describes the Data Execution Prevention (DEP) mitigation policy for an image.
 */
typedef struct _RTL_IMAGE_MITIGATION_DEP_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY Dep;
} RTL_IMAGE_MITIGATION_DEP_POLICY, *PRTL_IMAGE_MITIGATION_DEP_POLICY;

// rev
/**
 * Describes the Address Space Layout Randomization (ASLR) mitigation policy for an image.
 */
typedef struct _RTL_IMAGE_MITIGATION_ASLR_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY ForceRelocateImages;
    RTL_IMAGE_MITIGATION_POLICY BottomUpRandomization;
    RTL_IMAGE_MITIGATION_POLICY HighEntropyRandomization;
} RTL_IMAGE_MITIGATION_ASLR_POLICY, *PRTL_IMAGE_MITIGATION_ASLR_POLICY;

// rev
/**
 * Describes the dynamic code mitigation policy for an image.
 */
typedef struct _RTL_IMAGE_MITIGATION_DYNAMIC_CODE_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY BlockDynamicCode;
} RTL_IMAGE_MITIGATION_DYNAMIC_CODE_POLICY, *PRTL_IMAGE_MITIGATION_DYNAMIC_CODE_POLICY;

// rev
/**
 * Describes the strict handle-check mitigation policy for an image.
 */
typedef struct _RTL_IMAGE_MITIGATION_STRICT_HANDLE_CHECK_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY StrictHandleChecks;
} RTL_IMAGE_MITIGATION_STRICT_HANDLE_CHECK_POLICY, *PRTL_IMAGE_MITIGATION_STRICT_HANDLE_CHECK_POLICY;

// rev
/**
 * Describes the system-call-disable mitigation policy for an image.
 */
typedef struct _RTL_IMAGE_MITIGATION_SYSTEM_CALL_DISABLE_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY BlockWin32kSystemCalls;
} RTL_IMAGE_MITIGATION_SYSTEM_CALL_DISABLE_POLICY, *PRTL_IMAGE_MITIGATION_SYSTEM_CALL_DISABLE_POLICY;

// rev
/**
 * Describes the extension-point-disable mitigation policy for an image.
 */
typedef struct _RTL_IMAGE_MITIGATION_EXTENSION_POINT_DISABLE_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY DisableExtensionPoints;
} RTL_IMAGE_MITIGATION_EXTENSION_POINT_DISABLE_POLICY, *PRTL_IMAGE_MITIGATION_EXTENSION_POINT_DISABLE_POLICY;

// rev
/**
 * Describes the Control Flow Guard (CFG) mitigation policy for an image.
 */
typedef struct _RTL_IMAGE_MITIGATION_CONTROL_FLOW_GUARD_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY ControlFlowGuard;
    RTL_IMAGE_MITIGATION_POLICY StrictControlFlowGuard;
} RTL_IMAGE_MITIGATION_CONTROL_FLOW_GUARD_POLICY, *PRTL_IMAGE_MITIGATION_CONTROL_FLOW_GUARD_POLICY;

// rev
/**
 * Describes the binary signature (code integrity) mitigation policy for an image.
 */
typedef struct _RTL_IMAGE_MITIGATION_BINARY_SIGNATURE_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY BlockNonMicrosoftSignedBinaries;
    RTL_IMAGE_MITIGATION_POLICY EnforceSigningOnModuleDependencies;
} RTL_IMAGE_MITIGATION_BINARY_SIGNATURE_POLICY, *PRTL_IMAGE_MITIGATION_BINARY_SIGNATURE_POLICY;

// rev
/**
 * Describes the font-disable mitigation policy for an image.
 */
typedef struct _RTL_IMAGE_MITIGATION_FONT_DISABLE_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY DisableNonSystemFonts;
} RTL_IMAGE_MITIGATION_FONT_DISABLE_POLICY, *PRTL_IMAGE_MITIGATION_FONT_DISABLE_POLICY;

// rev
/**
 * Describes the image-load mitigation policy for an image.
 */
typedef struct _RTL_IMAGE_MITIGATION_IMAGE_LOAD_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY BlockRemoteImageLoads;
    RTL_IMAGE_MITIGATION_POLICY BlockLowLabelImageLoads;
    RTL_IMAGE_MITIGATION_POLICY PreferSystem32;
} RTL_IMAGE_MITIGATION_IMAGE_LOAD_POLICY, *PRTL_IMAGE_MITIGATION_IMAGE_LOAD_POLICY;

// rev
/**
 * Describes the payload restriction mitigation policy for an image.
 */
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
/**
 * Describes the child-process creation mitigation policy for an image.
 */
typedef struct _RTL_IMAGE_MITIGATION_CHILD_PROCESS_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY DisallowChildProcessCreation;
} RTL_IMAGE_MITIGATION_CHILD_PROCESS_POLICY, *PRTL_IMAGE_MITIGATION_CHILD_PROCESS_POLICY;

// rev
/**
 * Describes the Structured Exception Handling Overwrite Protection (SEHOP) mitigation policy for an image.
 */
typedef struct _RTL_IMAGE_MITIGATION_SEHOP_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY Sehop;
} RTL_IMAGE_MITIGATION_SEHOP_POLICY, *PRTL_IMAGE_MITIGATION_SEHOP_POLICY;

// rev
/**
 * Describes the heap mitigation policy for an image.
 */
typedef struct _RTL_IMAGE_MITIGATION_HEAP_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY TerminateOnHeapErrors;
} RTL_IMAGE_MITIGATION_HEAP_POLICY, *PRTL_IMAGE_MITIGATION_HEAP_POLICY;

// rev
/**
 * Describes the user-mode shadow stack (CET) mitigation policy for an image.
 */
typedef struct _RTL_IMAGE_MITIGATION_USER_SHADOW_STACK_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY UserShadowStack;
    RTL_IMAGE_MITIGATION_POLICY SetContextIpValidation;
    RTL_IMAGE_MITIGATION_POLICY BlockNonCetBinaries;
} RTL_IMAGE_MITIGATION_USER_SHADOW_STACK_POLICY, *PRTL_IMAGE_MITIGATION_USER_SHADOW_STACK_POLICY;

// rev
/**
 * Describes the redirection trust mitigation policy for an image.
 */
typedef struct _RTL_IMAGE_MITIGATION_REDIRECTION_TRUST_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY BlockUntrustedRedirections;
} RTL_IMAGE_MITIGATION_REDIRECTION_TRUST_POLICY, *PRTL_IMAGE_MITIGATION_REDIRECTION_TRUST_POLICY;

// rev
/**
 * Describes the user-mode pointer authentication mitigation policy for an image.
 */
typedef struct _RTL_IMAGE_MITIGATION_USER_POINTER_AUTH_POLICY
{
    RTL_IMAGE_MITIGATION_POLICY PointerAuthUserIp;
} RTL_IMAGE_MITIGATION_USER_POINTER_AUTH_POLICY, *PRTL_IMAGE_MITIGATION_USER_POINTER_AUTH_POLICY;

// rev
/**
 * Identifies the configured state of an image mitigation option.
 */
typedef enum _RTL_IMAGE_MITIGATION_OPTION_STATE
{
    RtlMitigationOptionStateNotConfigured,
    RtlMitigationOptionStateOn,
    RtlMitigationOptionStateOff,
    RtlMitigationOptionStateForce,
    RtlMitigationOptionStateOption
} RTL_IMAGE_MITIGATION_OPTION_STATE;

/**
 * Mask and shift values for image mitigation option states.
 */
#define RTL_IMAGE_MITIGATION_OPTION_STATEMASK 3UL
#define RTL_IMAGE_MITIGATION_OPTION_FORCEMASK 4UL
#define RTL_IMAGE_MITIGATION_OPTION_OPTIONMASK 8UL

// rev from PROCESS_MITIGATION_FLAGS
/**
 * Flags controlling how image mitigation policies are queried or applied.
 */
#define RTL_IMAGE_MITIGATION_FLAG_RESET 0x1
#define RTL_IMAGE_MITIGATION_FLAG_REMOVE 0x2
#define RTL_IMAGE_MITIGATION_FLAG_OSDEFAULT 0x4
#define RTL_IMAGE_MITIGATION_FLAG_AUDIT 0x8

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS3)

/**
 * The RtlQueryImageMitigationPolicy routine queries an image mitigation policy for an executable image or the system-wide defaults.
 *
 * \param ImagePath The path of the image to query, or NULL for the system-wide defaults.
 * \param Policy The mitigation policy to query.
 * \param Flags Flags controlling the query.
 * \param Buffer A buffer that receives the policy data.
 * \param BufferSize The size, in bytes, of Buffer.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlSetImageMitigationPolicy routine sets an image mitigation policy for an executable image or the system-wide defaults.
 *
 * \param ImagePath The path of the image to configure, or NULL for the system-wide defaults.
 * \param Policy The mitigation policy to set.
 * \param Flags Flags controlling the operation.
 * \param Buffer A buffer containing the policy data to apply.
 * \param BufferSize The size, in bytes, of Buffer.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlGetCurrentServiceSessionId routine returns the service session identifier associated with the current process.
 *
 * \return ULONG The current service session identifier, or zero if none is assigned.
 */
NTSYSAPI
ULONG
NTAPI
RtlGetCurrentServiceSessionId(
    VOID
    );

// private
/**
 * The RtlGetActiveConsoleId routine returns the session identifier of the currently active console session.
 *
 * \return ULONG The identifier of the active console session.
 */
NTSYSAPI
ULONG
NTAPI
RtlGetActiveConsoleId(
    VOID
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
// private
/**
 * The RtlGetConsoleSessionForegroundProcessId routine returns the process identifier of the foreground process in the active console session.
 *
 * \return LONGLONG The process identifier of the console session's foreground process.
 */
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
/**
 * The RtlGetTokenNamedObjectPath routine retrieves the named object path associated with a token.
 *
 * \param TokenHandle A handle to the token to query.
 * \param Sid An optional pointer to a SID used to compute the object path.
 * \param ObjectPath A pointer to a UNICODE_STRING that receives the object path. The caller frees it with RtlFreeUnicodeString.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlGetAppContainerNamedObjectPath routine retrieves the named object path for an app container.
 *
 * \param TokenHandle An optional handle to the app container token.
 * \param AppContainerSid An optional pointer to the app container SID.
 * \param RelativePath If `TRUE`, the returned path is relative to the app container root; otherwise it is absolute.
 * \param ObjectPath A pointer to a UNICODE_STRING that receives the object path. The caller frees it with RtlFreeUnicodeString.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlGetAppContainerParent routine retrieves the parent app container SID of the specified app container SID.
 *
 * \param AppContainerSid A pointer to the child app container SID.
 * \param AppContainerSidParent A pointer to a variable that receives the parent app container SID. The caller frees it with RtlFreeSid.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlCheckSandboxedToken routine determines whether the specified token is sandboxed.
 *
 * \param TokenHandle An optional handle to the token to test; if NULL, the token of the current thread or process is used.
 * \param IsSandboxed A pointer to a variable that receives whether the token is sandboxed.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlCheckTokenCapability routine determines whether a token has the specified capability, identified by its SID.
 *
 * \param TokenHandle An optional handle to the token to test; if NULL, the token of the current thread or process is used.
 * \param CapabilitySidToCheck A pointer to the capability SID to check.
 * \param HasCapability A pointer to a variable that receives whether the token has the capability.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlCapabilityCheck routine determines whether a token has the capability identified by name.
 *
 * \param TokenHandle An optional handle to the token to test; if NULL, the token of the current thread or process is used.
 * \param CapabilityName A pointer to the Unicode string that names the capability to check.
 * \param HasCapability A pointer to a variable that receives whether the token has the capability.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlCheckTokenMembership routine determines whether a specified SID is enabled in the given token.
 *
 * \param TokenHandle An optional handle to an impersonation token; if NULL, the token of the current thread is used.
 * \param SidToCheck A pointer to the SID whose membership is checked.
 * \param IsMember A pointer to a variable that receives whether the SID is enabled in the token.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCheckTokenMembership(
    _In_opt_ HANDLE TokenHandle,
    _In_ PSID SidToCheck,
    _Out_ PBOOLEAN IsMember
    );

// RtlCheckTokenMembershipEx Flags
/**
 * Flags for RtlCheckTokenMembershipEx.
 */
#define CTMF_INCLUDE_APPCONTAINER 0x00000001UL
#define CTMF_INCLUDE_LPAC 0x00000002UL
#define CTMF_VALID_FLAGS (CTMF_INCLUDE_APPCONTAINER | CTMF_INCLUDE_LPAC)

// rev
/**
 * The RtlCheckTokenMembershipEx routine determines whether a specified SID is enabled in the given token, with control over the evaluation flags.
 *
 * \param TokenHandle An optional handle to the token to test; if NULL, the token of the current thread is used.
 * \param SidToCheck A pointer to the SID whose membership is checked.
 * \param Flags A combination of CTMF_ flags that control how membership is evaluated.
 * \param IsMember A pointer to a variable that receives whether the SID is enabled in the token.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlQueryTokenHostIdAsUlong64 routine retrieves the package host identifier associated with a token as a 64-bit value.
 *
 * \param TokenHandle A handle to the token to query.
 * \param HostId A pointer to a variable that receives the package host identifier (WIN://PKGHOSTID).
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlIsParentOfChildAppContainer routine determines whether one app container SID is the parent of another.
 *
 * \param ParentAppContainerSid A pointer to the candidate parent app container SID.
 * \param ChildAppContainerSid A pointer to the candidate child app container SID.
 * \return Returns `TRUE` if the first SID is the parent of the second, otherwise `FALSE`.
 */
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
/**
 * The RtlIsApiSetImplemented routine determines whether the named API set is implemented on the current system.
 *
 * \param ApiSetName A pointer to the null-terminated name of the API set to query.
 * \return NTSTATUS Successful status if the API set is implemented, otherwise an errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlIsApiSetImplemented(
    _In_z_ PCSTR ApiSetName
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// rev
/**
 * The RtlIsCapabilitySid routine determines whether the specified SID is a capability SID.
 *
 * \param Sid A pointer to the SID to test.
 * \return Returns `TRUE` if the SID is a capability SID, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsCapabilitySid(
    _In_ PSID Sid
    );

// rev
/**
 * The RtlIsPackageSid routine determines whether the specified SID is a package (app container) SID.
 *
 * \param Sid A pointer to the SID to test.
 * \return Returns `TRUE` if the SID is a package SID, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsPackageSid(
    _In_ PSID Sid
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8

#if (PHNT_VERSION >= PHNT_WINDOWS_8_1)
// rev
/**
 * The RtlIsValidProcessTrustLabelSid routine determines whether the specified SID is a valid process trust label SID.
 *
 * \param Sid A pointer to the SID to test.
 * \return Returns `TRUE` if the SID is a valid process trust label SID, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsValidProcessTrustLabelSid(
    _In_ PSID Sid
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8_1

/**
 * Identifies the type of an AppContainer security identifier.
 */
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
/**
 * The RtlGetAppContainerSidType routine returns the type classification of an app container SID.
 *
 * \param AppContainerSid A pointer to the app container SID to classify.
 * \param AppContainerSidType A pointer to a variable that receives the APPCONTAINER_SID_TYPE value.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetAppContainerSidType(
    _In_ PSID AppContainerSid,
    _Out_ PAPPCONTAINER_SID_TYPE AppContainerSidType
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8_1

/**
 * The RtlFlsAlloc routine allocates a fiber local storage (FLS) index, optionally associating a cleanup callback.
 *
 * \param Callback An optional pointer to a callback invoked when an FLS slot is freed or a fiber is deleted.
 * \param FlsIndex A pointer to a variable that receives the allocated FLS index.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlFlsAlloc(
    _In_opt_ PFLS_CALLBACK_FUNCTION Callback,
    _Out_ PULONG FlsIndex
    );

// rev
/**
 * The RtlFlsAllocEx routine allocates a fiber local storage (FLS) index with extended output, optionally associating a cleanup callback.
 *
 * \param Callback An optional pointer to a callback invoked when an FLS slot is freed or a fiber is deleted.
 * \param Unused A pointer to a variable that receives an additional allocation output value.
 * \param FlsIndex A pointer to a variable that receives the allocated FLS index.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlFlsAllocEx(
    _In_opt_ PFLS_CALLBACK_FUNCTION Callback,
    _Out_ PULONG,
    _Out_ PULONG FlsIndex
    );

/**
 * The RtlFlsFree routine frees a fiber local storage (FLS) index previously allocated by RtlFlsAlloc.
 *
 * \param FlsIndex The FLS index to free.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlFlsFree(
    _In_ ULONG FlsIndex
    );

/**
 * The RtlFlsGetValue routine retrieves the value stored in a fiber local storage (FLS) slot of the current fiber.
 *
 * \param FlsIndex The FLS slot index.
 * \param FlsData Receives the value stored in the slot.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlFlsGetValue(
    _In_ ULONG FlsIndex,
    _Out_ PVOID* FlsData
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)
/**
 * The RtlFlsGetValue2 routine retrieves the value stored in a fiber local storage (FLS) slot of the current fiber.
 *
 * \param FlsIndex The FLS slot index.
 * \return The value stored in the slot.
 */
NTSYSAPI
PVOID
NTAPI
RtlFlsGetValue2(
    _In_ ULONG FlsIndex
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_20H1

/**
 * The RtlFlsSetValue routine stores a value in a fiber local storage (FLS) slot of the current fiber.
 *
 * \param FlsIndex The FLS slot index.
 * \param FlsData An optional value to store in the slot.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlFlsSetValue(
    _In_ ULONG FlsIndex,
    _In_opt_ PVOID FlsData
    );

/**
 * Flags for fiber-local storage (FLS) data cleanup.
 */
#define RTL_FLS_DATA_CLEANUP_PER_SLOT 1
#define RTL_FLS_DATA_CLEANUP_DEALLOCATE 2

/**
 * The RtlProcessFlsData routine processes the fiber local storage (FLS) data of a fiber, invoking FLS callbacks.
 *
 * \param FlsData The FLS data block to process.
 * \param Flags Flags controlling how the FLS data is processed.
 */
NTSYSAPI
VOID
NTAPI
RtlProcessFlsData(
    _In_ PVOID FlsData,
    _In_ ULONG Flags
    );

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
// rev
/**
 * The RtlTlsAlloc routine allocates a thread local storage (TLS) index.
 *
 * \param TlsIndex A pointer to a variable that receives the allocated TLS index.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlTlsAlloc(
    _Out_ PULONG TlsIndex
    );

// rev
/**
 * The RtlTlsFree routine frees a thread local storage (TLS) index previously allocated by RtlTlsAlloc.
 *
 * \param TlsIndex The TLS index to free.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlTlsFree(
    _In_ ULONG TlsIndex
    );

/**
 * The RtlTlsSetValue routine stores a value in the specified thread local storage (TLS) slot of the current thread.
 *
 * \param TlsIndex The TLS slot index.
 * \param TlsData An optional value to store in the slot.
 * \return NTSTATUS Successful or errant status.
 */
// rev
NTSYSAPI
NTSTATUS
NTAPI
RtlTlsSetValue(
    _In_ ULONG TlsIndex,
    _In_opt_ PVOID TlsData
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11

//
// State isolation
//

/**
 * Identifies whether a state location resides in the registry or the file system.
 */
typedef enum _STATE_LOCATION_TYPE
{
    LocationTypeRegistry,
    LocationTypeFileSystem,
    LocationTypeMaximum
} STATE_LOCATION_TYPE;

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS3)
// private
/**
 * The RtlIsStateSeparationEnabled routine determines whether OS state separation is enabled on the current system.
 *
 * \return Returns `TRUE` if state separation is enabled, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsStateSeparationEnabled(
    VOID
    );

// private
/**
 * The RtlGetPersistedStateLocation routine resolves the persisted state storage location for a source identifier.
 *
 * \param SourceID A pointer to the null-terminated source identifier.
 * \param CustomValue An optional pointer to a custom value that refines the location.
 * \param DefaultPath An optional pointer to a default path used when no persisted location is configured.
 * \param StateLocationType The STATE_LOCATION_TYPE that selects the class of location to resolve.
 * \param TargetPath An optional buffer that receives the resolved location path.
 * \param BufferLengthIn The size, in bytes, of the TargetPath buffer.
 * \param BufferLengthOut An optional pointer to a variable that receives the required or written length, in bytes.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlIsCloudFilesPlaceholder routine determines whether a file or directory is a Cloud Files placeholder, based on its attributes and reparse tag.
 *
 * \param FileAttributes The file attributes of the file or directory.
 * \param ReparseTag The reparse tag (or EaSize) of the file or directory.
 * \return Returns `TRUE` if the file or directory is a Cloud Files placeholder, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtliscloudfilesplaceholder
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsCloudFilesPlaceholder(
    _In_ ULONG FileAttributes,
    _In_ ULONG ReparseTag
    );

// msdn
/**
 * The RtlIsPartialPlaceholder routine determines whether a file or directory is a partial Cloud Files placeholder, based on its attributes and reparse tag.
 *
 * \param FileAttributes The file attributes of the file or directory.
 * \param ReparseTag The reparse tag (or EaSize) of the file or directory.
 * \return Returns `TRUE` if the file or directory is a partial placeholder, otherwise `FALSE`.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlispartialplaceholder
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsPartialPlaceholder(
    _In_ ULONG FileAttributes,
    _In_ ULONG ReparseTag
    );

// msdn
/**
 * The RtlIsPartialPlaceholderFileHandle routine determines whether the file referenced by a handle is a partial placeholder.
 *
 * \param FileHandle A handle to the file to test. The handle must have at least FILE_READ_ATTRIBUTES access.
 * \param IsPartialPlaceholder A pointer to a variable that receives whether the file is a partial placeholder.
 * \return NTSTATUS Successful or errant status.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlispartialplaceholderfilehandle
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlIsPartialPlaceholderFileHandle(
    _In_ HANDLE FileHandle,
    _Out_ PBOOLEAN IsPartialPlaceholder
    );

/**
 * The RtlIsPartialPlaceholderFileInfo routine determines whether file information describes a partial cloud-files placeholder.
 *
 * \param InfoBuffer A buffer containing file information of the specified class.
 * \param InfoClass The file information class of the data in InfoBuffer.
 * \param IsPartialPlaceholder Receives TRUE if the file is a partial placeholder.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * Process placeholder compatibility mode values.
 */
#define PHCM_APPLICATION_DEFAULT ((CHAR)0)
#define PHCM_DISGUISE_PLACEHOLDERS ((CHAR)1)
#define PHCM_EXPOSE_PLACEHOLDERS ((CHAR)2)
#define PHCM_MAX ((CHAR)2)

/**
 * Error values returned by the placeholder compatibility mode routines.
 */
#define PHCM_ERROR_INVALID_PARAMETER ((CHAR)-1)
#define PHCM_ERROR_NO_TEB ((CHAR)-2)

/**
 * The RtlQueryThreadPlaceholderCompatibilityMode routine returns the placeholder compatibility mode for the current thread.
 *
 * \return CHAR The current thread placeholder compatibility mode.
 */
NTSYSAPI
CHAR
NTAPI
RtlQueryThreadPlaceholderCompatibilityMode(
    VOID
    );

/**
 * The RtlSetThreadPlaceholderCompatibilityMode routine sets the placeholder compatibility mode for the current thread.
 *
 * \param Mode The placeholder compatibility mode to set.
 * \return CHAR The previous thread placeholder compatibility mode.
 */
NTSYSAPI
CHAR
NTAPI
RtlSetThreadPlaceholderCompatibilityMode(
    _In_ CHAR Mode
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS3

#undef PHCM_MAX
/**
 * Extended (thread-level) placeholder compatibility mode values.
 */
#define PHCM_DISGUISE_FULL_PLACEHOLDERS ((CHAR)3)
#define PHCM_MAX ((CHAR)3)
#define PHCM_ERROR_NO_PEB ((CHAR)-3)

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS4)
/**
 * The RtlQueryProcessPlaceholderCompatibilityMode routine returns the placeholder compatibility mode for the current process.
 *
 * \return CHAR The current process placeholder compatibility mode.
 */
NTSYSAPI
CHAR
NTAPI
RtlQueryProcessPlaceholderCompatibilityMode(
    VOID
    );

/**
 * The RtlSetProcessPlaceholderCompatibilityMode routine sets the placeholder compatibility mode for the current process.
 *
 * \param Mode The placeholder compatibility mode to set.
 * \return CHAR The previous process placeholder compatibility mode.
 */
NTSYSAPI
CHAR
NTAPI
RtlSetProcessPlaceholderCompatibilityMode(
    _In_ CHAR Mode
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS4

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS2)
// rev
/**
 * The RtlIsNonEmptyDirectoryReparsePointAllowed routine determines whether a reparse point identified by the specified tag is allowed on a non-empty directory.
 *
 * \param ReparseTag The reparse tag to test.
 * \return Returns `TRUE` if the reparse tag is allowed on a non-empty directory, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsNonEmptyDirectoryReparsePointAllowed(
    _In_ ULONG ReparseTag
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS2

#if (PHNT_VERSION >= PHNT_WINDOWS_8)
// rev
/**
 * The RtlAppxIsFileOwnedByTrustedInstaller routine determines whether the file referenced by a handle is owned by the TrustedInstaller.
 *
 * \param FileHandle A handle to the file to test.
 * \param IsFileOwnedByTrustedInstaller A pointer to a variable that receives whether the file is owned by the TrustedInstaller.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlAppxIsFileOwnedByTrustedInstaller(
    _In_ HANDLE FileHandle,
    _Out_ PBOOLEAN IsFileOwnedByTrustedInstaller
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8

// Windows Internals book
/**
 * Package claim flags describing a packaged application activation token.
 */
#define PSM_ACTIVATION_TOKEN_PACKAGED_APPLICATION       0x00000001UL // AppX package format
#define PSM_ACTIVATION_TOKEN_SHARED_ENTITY              0x00000002UL // Shared token, multiple binaries in the same package
#define PSM_ACTIVATION_TOKEN_FULL_TRUST                 0x00000004UL // Trusted (Centennial), converted Win32 application
#define PSM_ACTIVATION_TOKEN_NATIVE_SERVICE             0x00000008UL // Packaged service created by SCM
//#define PSM_ACTIVATION_TOKEN_DEVELOPMENT_APP          0x00000010UL
/**
 * Additional package claim and activation token flags.
 */
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

/**
 * Constants describing minimum system application claim values.
 */
#define PSMP_MINIMUM_SYSAPP_CLAIM_VALUES 2
#define PSMP_MAXIMUM_SYSAPP_CLAIM_VALUES 4

// private
/**
 * Describes the package claim (origin and flags) associated with a process.
 */
typedef struct _PS_PKG_CLAIM
{
    ULONG Flags;  // PSM_ACTIVATION_TOKEN_*
    ULONG Origin; // PackageOrigin
} PS_PKG_CLAIM, *PPS_PKG_CLAIM;

// private // WIN://BGKD
/**
 * Identifies the background activation type of a packaged application.
 */
typedef enum _PSM_ACTIVATE_BACKGROUND_TYPE
{
  PsmActNotBackground = 0,
  PsmActMixedHost = 1,
  PsmActPureHost = 2,
  PsmActSystemHost = 3,
  PsmActInvalidType = 4,
} PSM_ACTIVATE_BACKGROUND_TYPE;

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
/**
 * The RtlQueryPackageClaims routine retrieves the package claims associated with a token, including the package full name, application identifier, and attributes.
 *
 * \param TokenHandle A handle to the token to query.
 * \param PackageFullName An optional buffer that receives the package full name.
 * \param PackageSize An optional pointer to the size, in bytes, of the PackageFullName buffer; updated with the required or written size.
 * \param AppId An optional buffer that receives the application identifier.
 * \param AppIdSize An optional pointer to the size, in bytes, of the AppId buffer; updated with the required or written size.
 * \param DynamicId An optional pointer to a variable that receives the dynamic package identifier.
 * \param PkgClaim An optional pointer to a PS_PKG_CLAIM structure that receives the package claim.
 * \param AttributesPresent An optional pointer to a variable that receives the mask of attributes present.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlQueryPackageIdentity routine retrieves the package identity associated with a token, including the package full name and application identifier.
 *
 * \param TokenHandle A handle to the token to query.
 * \param PackageFullName A buffer that receives the package full name.
 * \param PackageSize A pointer to the size, in bytes, of the PackageFullName buffer; updated with the required or written size.
 * \param AppId An optional buffer that receives the application identifier.
 * \param AppIdSize An optional pointer to the size, in bytes, of the AppId buffer; updated with the required or written size.
 * \param Packaged An optional pointer to a variable that receives whether the token belongs to a packaged application.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlQueryPackageIdentityEx routine retrieves the package identity associated with a token, with additional dynamic identifier and flag outputs.
 *
 * \param TokenHandle A handle to the token to query.
 * \param PackageFullName A buffer that receives the package full name.
 * \param PackageSize A pointer to the size, in bytes, of the PackageFullName buffer; updated with the required or written size.
 * \param AppId An optional buffer that receives the application identifier.
 * \param AppIdSize An optional pointer to the size, in bytes, of the AppId buffer; updated with the required or written size.
 * \param DynamicId An optional pointer to a variable that receives the dynamic package identifier.
 * \param Flags An optional pointer to a variable that receives package identity flags.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlQueryProtectedPolicy routine retrieves the value of a protected policy identified by a GUID.
 *
 * \param PolicyGuid A pointer to the GUID that identifies the protected policy.
 * \param PolicyValue A pointer to a variable that receives the policy value.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryProtectedPolicy(
    _In_ PCGUID PolicyGuid,
    _Out_ PULONG_PTR PolicyValue
    );

// rev
/**
 * The RtlSetProtectedPolicy routine sets the value of a protected policy identified by a GUID.
 *
 * \param PolicyGuid A pointer to the GUID that identifies the protected policy.
 * \param PolicyValue The new value to assign to the protected policy.
 * \param OldPolicyValue A pointer to a variable that receives the previous policy value.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetProtectedPolicy(
    _In_ PCGUID PolicyGuid,
    _In_ ULONG_PTR PolicyValue,
    _Out_ PULONG_PTR OldPolicyValue
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8_1

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
// rev
/**
 * The RtlIsEnclaveFeaturePresent routine determines whether the specified enclave features are present on the current system.
 *
 * \param FeatureMask A mask of the enclave features to test.
 * \return Returns `TRUE` if the specified enclave features are present, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsEnclaveFeaturePresent(
    _In_ ULONG FeatureMask
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS1

#if (PHNT_VERSION >= PHNT_WINDOWS_10)
// private
/**
 * The RtlIsMultiSessionSku routine determines whether the current operating system SKU supports multiple sessions.
 *
 * \return Returns `TRUE` if the SKU supports multiple sessions, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsMultiSessionSku(
    VOID
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
// private
/**
 * The RtlIsMultiUsersInSessionSku routine determines whether the current operating system SKU supports multiple users within a single session.
 *
 * \return Returns `TRUE` if the SKU supports multiple users in a session, otherwise `FALSE`.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlIsMultiUsersInSessionSku(
    VOID
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS1

#if (PHNT_VERSION >= PHNT_WINDOWS_11)

/**
 * Contains properties describing a session.
 */
typedef struct _RTL_SESSION_PROPERTIES
{
    ULONG IsCurrentSessionId;
} RTL_SESSION_PROPERTIES, *PRTL_SESSION_PROPERTIES;

// rev
/**
 * The RtlGetSessionProperties routine retrieves the properties of the specified session.
 *
 * \param SessionId The identifier of the session to query.
 * \param SessionProperties A pointer to an RTL_SESSION_PROPERTIES structure that receives the session properties.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlGetSessionProperties(
    _In_ ULONG SessionId,
    _Out_ PRTL_SESSION_PROPERTIES SessionProperties
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11

// private
/**
 * Identifies an item stored in the Boot Status Data (BSD).
 */
typedef enum _RTL_BSD_ITEM_TYPE
{
    RtlBsdItemVersionNumber,                    // qs: ULONG
    RtlBsdItemProductType,                      // qs: NT_PRODUCT_TYPE (ULONG)
    RtlBsdItemAabEnabled,                       // qs: BOOLEAN // AutoAdvancedBoot
    RtlBsdItemAabTimeout,                       // qs: UCHAR // AdvancedBootMenuTimeout
    RtlBsdItemBootGood,                         // qs: BOOLEAN // LastBootSucceeded
    RtlBsdItemBootShutdown,                     // qs: BOOLEAN // LastBootShutdown
    RtlBsdSleepInProgress,                      // qs: BOOLEAN // SleepInProgress
    RtlBsdPowerTransition,                      // qs: RTL_BSD_DATA_POWER_TRANSITION
    RtlBsdItemBootAttemptCount,                 // qs: UCHAR // BootAttemptCount
    RtlBsdItemBootCheckpoint,                   // qs: UCHAR // LastBootCheckpoint
    RtlBsdItemBootId,                           // qs: ULONG (USER_SHARED_DATA->BootId) // 10
    RtlBsdItemShutdownBootId,                   // qs: ULONG
    RtlBsdItemReportedAbnormalShutdownBootId,   // qs: ULONG
    RtlBsdItemErrorInfo,                        // qs: RTL_BSD_DATA_ERROR_INFO
    RtlBsdItemPowerButtonPressInfo,             // qs: RTL_BSD_POWER_BUTTON_PRESS_INFO
    RtlBsdItemChecksum,                         // q: UCHAR
    RtlBsdPowerTransitionExtension,             // qs: RTL_BSD_DATA_POWER_TRANSITION_EXTENSION
    RtlBsdItemFeatureConfigurationState,        // qs: ULONG
    RtlBsdItemRevocationListInfo,               // qs: RTL_BSD_ITEM_REVOCATION_LIST // 24H2
    RtlBsdItemMax
} RTL_BSD_ITEM_TYPE;

/**
 * Describes a power transition record stored in the Boot Status Data.
 */
typedef struct _RTL_BSD_DATA_POWER_TRANSITION
{
    UCHAR PowerButton : 1;
    UCHAR SleepButton : 1;
    UCHAR LidClose : 1;
    UCHAR SystemIdle : 1;
    UCHAR UserPresent : 1; // Power setting "Keep Alive"
    UCHAR ApmBattery : 1;
    UCHAR Reserved : 2;
} RTL_BSD_DATA_POWER_TRANSITION, *PRTL_BSD_DATA_POWER_TRANSITION;

/**
 * Describes boot error information stored in the Boot Status Data.
 */
typedef struct _RTL_BSD_DATA_ERROR_INFO
{
    ULONG BootId;           // The Boot ID where the error occurred
    ULONG RepeatCount;      // How many times this specific error happened
    ULONG OtherErrorCount;  // Count of other errors
} RTL_BSD_DATA_ERROR_INFO, *PRTL_BSD_DATA_ERROR_INFO;

/**
 * Describes power button press information stored in the Boot Status Data.
 */
typedef struct _RTL_BSD_POWER_BUTTON_PRESS_INFO
{
    ULONG LastPressBootId;
    ULONG LastPressTime;         // Time in seconds since boot
    ULONG LastReleaseTime;
    ULONG ButtonPressCount;
    ULONG CoalescedPressTime;    // Total time pressed across recent boots
    ULONG CoalescedPressCount;
} RTL_BSD_POWER_BUTTON_PRESS_INFO, *PRTL_BSD_POWER_BUTTON_PRESS_INFO;

/**
 * Extends a Boot Status Data power transition record with additional fields.
 */
typedef struct _RTL_BSD_DATA_POWER_TRANSITION_EXTENSION
{
    UCHAR SystemIdleTransition : 1;
    UCHAR FanError : 1;
    UCHAR ThermalShutdown : 1;
    UCHAR Reserved : 5;
} RTL_BSD_DATA_POWER_TRANSITION_EXTENSION, *PRTL_BSD_DATA_POWER_TRANSITION_EXTENSION;

// private
/**
 * Represents an item read from or written to the Boot Status Data.
 */
typedef struct _RTL_BSD_ITEM
{
    RTL_BSD_ITEM_TYPE Type;
    PVOID DataBuffer;
    ULONG DataLength;
} RTL_BSD_ITEM, *PRTL_BSD_ITEM;

// ros
/**
 * The RtlCreateBootStatusDataFile routine creates the boot status data file used to track boot progress and recovery state.
 *
 * \param BootStatusFileName An optional path to the boot status data file.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateBootStatusDataFile(
    _In_opt_ PCWSTR BootStatusFileName
    );

// ros
/**
 * The RtlLockBootStatusData routine opens and locks the boot status data file for access.
 *
 * \param FileHandle A pointer to a variable that receives a handle to the locked boot status data file. The caller releases it with RtlUnlockBootStatusData.
 * \return NTSTATUS Successful or errant status.
 */
_Acquires_lock_(*FileHandle)
NTSYSAPI
NTSTATUS
NTAPI
RtlLockBootStatusData(
    _Out_ PHANDLE FileHandle
    );

// ros
/**
 * The RtlUnlockBootStatusData routine unlocks and closes the boot status data file previously locked by RtlLockBootStatusData.
 *
 * \param FileHandle A handle to the boot status data file returned by RtlLockBootStatusData.
 * \return NTSTATUS Successful or errant status.
 */
_Releases_lock_(FileHandle)
NTSYSAPI
NTSTATUS
NTAPI
RtlUnlockBootStatusData(
    _In_ HANDLE FileHandle
    );

/**
 * The RtlGetSetBootStatusData routine reads or writes an item of boot status data (BSD) using an open BSD file handle.
 *
 * \param FileHandle A handle to the boot status data file.
 * \param Read TRUE to read the item; FALSE to write it.
 * \param DataClass The boot status item to read or write.
 * \param Buffer A buffer that receives or supplies the item data.
 * \param BufferSize The size, in bytes, of Buffer.
 * \param ReturnLength An optional pointer that receives the number of bytes transferred.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlCheckBootStatusIntegrity routine verifies the integrity of the boot status data file.
 *
 * \param FileHandle A handle to the boot status data file returned by RtlLockBootStatusData.
 * \param Verified A pointer to a variable that receives whether the boot status data integrity was verified.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCheckBootStatusIntegrity(
    _In_ HANDLE FileHandle,
    _Out_ PBOOLEAN Verified
    );

// rev
/**
 * The RtlRestoreBootStatusDefaults routine restores the boot status data file to its default values.
 *
 * \param FileHandle A handle to the boot status data file returned by RtlLockBootStatusData.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlRestoreBootStatusDefaults(
    _In_ HANDLE FileHandle
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS1

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS3)
// rev
/**
 * The RtlRestoreSystemBootStatusDefaults routine restores the system boot status data to its default values.
 *
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlRestoreSystemBootStatusDefaults(
    VOID
    );

/**
 * The RtlGetSystemBootStatus routine retrieves an item of boot status data (BSD).
 *
 * \param BootStatusInformationClass The boot status item to retrieve.
 * \param DataBuffer A buffer that receives the item data.
 * \param DataLength The size, in bytes, of the buffer.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlSetSystemBootStatus routine sets an item of boot status data (BSD).
 *
 * \param BootStatusInformationClass The boot status item to set.
 * \param DataBuffer A buffer containing the item data to set.
 * \param DataLength The size, in bytes, of the buffer.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlCheckPortableOperatingSystem routine determines whether the operating system is running as a portable (Windows To Go) installation.
 *
 * \param IsPortable A pointer to a variable that receives whether the operating system is portable.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlCheckPortableOperatingSystem(
    _Out_ PBOOLEAN IsPortable // VOID
    );

// rev
/**
 * The RtlSetPortableOperatingSystem routine sets whether the operating system is marked as a portable (Windows To Go) installation.
 *
 * \param IsPortable Set to `TRUE` to mark the operating system as portable, or `FALSE` otherwise.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSetPortableOperatingSystem(
    _In_ BOOLEAN IsPortable
    );

// rev
/**
 * The RtlSetProxiedProcessId routine sets the proxied process identifier for the current process.
 *
 * \param ProxiedProcessId The proxied process identifier to set.
 * \return ULONG The previous proxied process identifier.
 */
NTSYSAPI
ULONG
NTAPI
RtlSetProxiedProcessId(
    _In_ ULONG ProxiedProcessId
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_8

/**
 * The RtlFindClosestEncodableLength routine finds the closest length that can be encoded for the specified source length.
 *
 * \param SourceLength The source length to encode.
 * \param TargetLength A pointer to a variable that receives the closest encodable length.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * Pointer to an RTL_SECURE_MEMORY_CACHE_CALLBACK callback.
 */
typedef RTL_SECURE_MEMORY_CACHE_CALLBACK *PRTL_SECURE_MEMORY_CACHE_CALLBACK;

// ros
/**
 * The RtlRegisterSecureMemoryCacheCallback routine registers a callback that is invoked when a secured memory range is freed or its protections are changed.
 *
 * \param Callback A pointer to the secure memory cache callback routine to register.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlRegisterSecureMemoryCacheCallback(
    _In_ PRTL_SECURE_MEMORY_CACHE_CALLBACK Callback
    );

/**
 * The RtlDeregisterSecureMemoryCacheCallback routine removes a secure memory cache callback previously registered by RtlRegisterSecureMemoryCacheCallback.
 *
 * \param Callback A pointer to the secure memory cache callback routine to deregister.
 * \return BOOLEAN TRUE if the callback was deregistered, FALSE otherwise.
 */
NTSYSAPI
BOOLEAN
NTAPI
RtlDeregisterSecureMemoryCacheCallback(
    _In_ PRTL_SECURE_MEMORY_CACHE_CALLBACK Callback
    );

/**
 * The RtlFlushSecureMemoryCache routine flushes a region from the secure memory cache.
 *
 * \param MemoryCache The base address of the secure memory region to flush.
 * \param MemoryLength An optional length, in bytes, of the region to flush.
 * \return TRUE if the cache was flushed; otherwise, FALSE.
 */
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
/**
 * Identifies a feature in the feature configuration store.
 */
typedef ULONG RTL_FEATURE_ID;
/**
 * Represents a stamp that changes whenever the feature configuration is modified.
 */
typedef ULONGLONG RTL_FEATURE_CHANGE_STAMP, *PRTL_FEATURE_CHANGE_STAMP;
/**
 * Represents the variant selected for a feature.
 */
typedef UCHAR RTL_FEATURE_VARIANT;
/**
 * Represents the payload value associated with a feature variant.
 */
typedef ULONG RTL_FEATURE_VARIANT_PAYLOAD;
/**
 * Represents a registration for feature configuration change notifications.
 */
typedef PVOID RTL_FEATURE_CONFIGURATION_CHANGE_REGISTRATION, *PRTL_FEATURE_CONFIGURATION_CHANGE_REGISTRATION;

// private
/**
 * Describes a feature usage report submitted to the feature configuration store.
 */
typedef struct _RTL_FEATURE_USAGE_REPORT
{
    ULONG FeatureId;
    USHORT ReportingKind;
    USHORT ReportingOptions;
} RTL_FEATURE_USAGE_REPORT, *PRTL_FEATURE_USAGE_REPORT;

// private
/**
 * Identifies the type of a feature configuration.
 */
typedef enum _RTL_FEATURE_CONFIGURATION_TYPE
{
    RtlFeatureConfigurationBoot,
    RtlFeatureConfigurationRuntime,
    RtlFeatureConfigurationCount
} RTL_FEATURE_CONFIGURATION_TYPE;

// private
/**
 * Describes the configuration of a single feature.
 */
typedef struct _RTL_FEATURE_CONFIGURATION
{
    ULONG FeatureId;
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
    ULONG VariantPayload;
} RTL_FEATURE_CONFIGURATION, *PRTL_FEATURE_CONFIGURATION;

// private
/**
 * Internal representation of a feature configuration entry.
 */
typedef struct _RTL_FEATURE_CONFIGURATION_INTERNAL
{
    ULONG FeatureId;
    union
    {
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
        ULONG Flags;
    };
    ULONG VariantPayload;
    union
    {
        struct
        {
            ULONG ChangeTimeUpgrade : 1;
            ULONG HasGroupBypass : 1;
            ULONG Reserved2 : 30;
        };
        ULONG Flags2;
    };
} RTL_FEATURE_CONFIGURATION_INTERNAL, *PRTL_FEATURE_CONFIGURATION_INTERNAL;

// private
/**
 * Describes a single feature configuration section entry.
 */
typedef struct _SYSTEM_FEATURE_CONFIGURATION_SECTIONS_INFORMATION_ENTRY
{
    RTL_FEATURE_CHANGE_STAMP ChangeStamp;
    HANDLE SectionHandle;
    SIZE_T Size;
} SYSTEM_FEATURE_CONFIGURATION_SECTIONS_INFORMATION_ENTRY, *PSYSTEM_FEATURE_CONFIGURATION_SECTIONS_INFORMATION_ENTRY;

// private
/**
 * Identifies the type of a feature configuration section.
 */
typedef enum _SYSTEM_FEATURE_CONFIGURATION_SECTION_TYPE
{
    SystemFeatureConfigurationSectionTypeBoot = 0,
    SystemFeatureConfigurationSectionTypeRuntime = 1,
    SystemFeatureConfigurationSectionTypeUsageTriggers = 2,
    SystemFeatureConfigurationSectionTypeGoverned = 3,
    SystemFeatureConfigurationSectionTypeCount
} SYSTEM_FEATURE_CONFIGURATION_SECTION_TYPE;

// private
/**
 * Specifies a request for feature configuration section information.
 */
typedef struct _SYSTEM_FEATURE_CONFIGURATION_SECTIONS_REQUEST
{
    RTL_FEATURE_CHANGE_STAMP PreviousChangeStamps[SystemFeatureConfigurationSectionTypeCount];
} SYSTEM_FEATURE_CONFIGURATION_SECTIONS_REQUEST, *PSYSTEM_FEATURE_CONFIGURATION_SECTIONS_REQUEST;

// private
/**
 * Contains information about feature configuration sections.
 */
typedef struct _SYSTEM_FEATURE_CONFIGURATION_SECTIONS_INFORMATION
{
    RTL_FEATURE_CHANGE_STAMP OverallChangeStamp;
    SYSTEM_FEATURE_CONFIGURATION_SECTIONS_INFORMATION_ENTRY Descriptors[SystemFeatureConfigurationSectionTypeCount];
} SYSTEM_FEATURE_CONFIGURATION_SECTIONS_INFORMATION, *PSYSTEM_FEATURE_CONFIGURATION_SECTIONS_INFORMATION;

//typedef struct _SYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE
//{
//    ULONG UpdateCount;
//    _Field_size_(UpdateCount) SYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE_ENTRY Updates[ANYSIZE_ARRAY];
//} SYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE, *PSYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE;

// private
/**
 * Represents a table of feature configuration entries.
 */
typedef struct _RTL_FEATURE_CONFIGURATION_TABLE
{
    ULONG FeatureCount;
    _Field_size_(FeatureCount) RTL_FEATURE_CONFIGURATION_INTERNAL Features[ANYSIZE_ARRAY];
} RTL_FEATURE_CONFIGURATION_TABLE, *PRTL_FEATURE_CONFIGURATION_TABLE;

// private
/**
 * Identifies the priority level of a feature configuration.
 */
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
/**
 * Identifies whether a feature is enabled, disabled, or set to its default state.
 */
typedef enum _RTL_FEATURE_ENABLED_STATE
{
    FeatureEnabledStateDefault,
    FeatureEnabledStateDisabled,
    FeatureEnabledStateEnabled
} RTL_FEATURE_ENABLED_STATE;

// private
/**
 * Specifies options controlling how a feature enabled state is evaluated.
 */
typedef enum _RTL_FEATURE_ENABLED_STATE_OPTIONS
{
    FeatureEnabledStateOptionsNone,
    FeatureEnabledStateOptionsWexpConfig
} RTL_FEATURE_ENABLED_STATE_OPTIONS, *PRTL_FEATURE_ENABLED_STATE_OPTIONS;

// private
/**
 * Identifies the kind of payload carried by a feature variant.
 */
typedef enum _RTL_FEATURE_VARIANT_PAYLOAD_KIND
{
    FeatureVariantPayloadKindNone,
    FeatureVariantPayloadKindResident,
    FeatureVariantPayloadKindExternal
} RTL_FEATURE_VARIANT_PAYLOAD_KIND, *PRTL_FEATURE_VARIANT_PAYLOAD_KIND;

// private
/**
 * Identifies an operation performed on a feature configuration.
 */
typedef enum _RTL_FEATURE_CONFIGURATION_OPERATION
{
    FeatureConfigurationOperationNone         = 0,
    FeatureConfigurationOperationFeatureState = 1,
    FeatureConfigurationOperationVariantState = 2,
    FeatureConfigurationOperationResetState   = 4
} RTL_FEATURE_CONFIGURATION_OPERATION, *PRTL_FEATURE_CONFIGURATION_OPERATION;

/**
 * Masks and shifts for encoding a feature variant and its payload.
 */
#define RTL_FEATURE_VARIANT_MASK              0x000000FF
#define RTL_FEATURE_CHANGE_TIME_UPGRADE       0x00000100
#define RTL_FEATURE_HAS_GROUP_BYPASS          0x00000200

// private
/**
 * Describes an update to be applied to a feature configuration.
 */
typedef struct _RTL_FEATURE_CONFIGURATION_UPDATE
{
    RTL_FEATURE_ID FeatureId;
    RTL_FEATURE_CONFIGURATION_PRIORITY Priority;
    RTL_FEATURE_ENABLED_STATE EnabledState;
    RTL_FEATURE_ENABLED_STATE_OPTIONS EnabledStateOptions;

    union
    {
        ULONG VariantFlags;
        struct
        {
            UCHAR Variant;
            UCHAR Flags;
            USHORT ReservedFlags;
        } DUMMYSTRUCTNAME;
        struct
        {
            ULONG Variant_ : 8;
            ULONG ChangeTimeUpgrade : 1;
            ULONG HasGroupBypass : 1;
            ULONG Reserved_ : 22;
        } DUMMYSTRUCTNAME2;
    } DUMMYUNIONNAME;

    RTL_FEATURE_VARIANT_PAYLOAD_KIND VariantPayloadKind;
    UCHAR Reserved[3];
    RTL_FEATURE_VARIANT_PAYLOAD VariantPayload;
    RTL_FEATURE_CONFIGURATION_OPERATION Operation;
} RTL_FEATURE_CONFIGURATION_UPDATE, *PRTL_FEATURE_CONFIGURATION_UPDATE;

// private
/**
 * Describes the target of a feature usage subscription.
 */
typedef struct _RTL_FEATURE_USAGE_SUBSCRIPTION_TARGET
{
    ULONG Data[2];
} RTL_FEATURE_USAGE_SUBSCRIPTION_TARGET, *PRTL_FEATURE_USAGE_SUBSCRIPTION_TARGET;

// private
/**
 * Describes the details of a feature usage subscription.
 */
typedef struct _SYSTEM_FEATURE_USAGE_SUBSCRIPTION_DETAILS
{
    RTL_FEATURE_ID FeatureId;
    USHORT ReportingKind;
    USHORT ReportingOptions;
    RTL_FEATURE_USAGE_SUBSCRIPTION_TARGET ReportingTarget;
} SYSTEM_FEATURE_USAGE_SUBSCRIPTION_DETAILS, *PSYSTEM_FEATURE_USAGE_SUBSCRIPTION_DETAILS;

// private
/**
 * Contains usage data collected for a feature.
 */
typedef struct _RTL_FEATURE_USAGE_DATA
{
    RTL_FEATURE_ID FeatureId;
    USHORT ReportingKind;
    USHORT UsageCount;
} RTL_FEATURE_USAGE_DATA, *PRTL_FEATURE_USAGE_DATA;

// private
/**
 * Describes a feature usage subscription.
 */
typedef struct _RTL_FEATURE_USAGE_SUBSCRIPTION_DETAILS
{
    RTL_FEATURE_ID FeatureId;
    USHORT ReportingKind;
    USHORT ReportingOptions;
    RTL_FEATURE_USAGE_SUBSCRIPTION_TARGET ReportingTarget;
} RTL_FEATURE_USAGE_SUBSCRIPTION_DETAILS, *PRTL_FEATURE_USAGE_SUBSCRIPTION_DETAILS;

// private
/**
 * Represents a table of feature usage subscriptions.
 */
typedef struct _RTL_FEATURE_USAGE_SUBSCRIPTION_TABLE
{
    ULONG SubscriptionCount;
    _Field_size_(SubscriptionCount) RTL_FEATURE_USAGE_SUBSCRIPTION_DETAILS Subscriptions[ANYSIZE_ARRAY];
} RTL_FEATURE_USAGE_SUBSCRIPTION_TABLE, *PRTL_FEATURE_USAGE_SUBSCRIPTION_TABLE;

// private
/**
 * Describes a single feature usage subscription update entry.
 */
typedef struct _SYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE_ENTRY
{
    ULONG Remove;
    RTL_FEATURE_USAGE_SUBSCRIPTION_DETAILS Details;
} SYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE_ENTRY, *PSYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE_ENTRY;

// private
typedef _Function_class_(RTL_FEATURE_CONFIGURATION_CHANGE_CALLBACK)
VOID NTAPI RTL_FEATURE_CONFIGURATION_CHANGE_CALLBACK(
    _In_opt_ PVOID Context
    );
/**
 * Pointer to an RTL_FEATURE_CONFIGURATION_CHANGE_CALLBACK callback.
 */
typedef RTL_FEATURE_CONFIGURATION_CHANGE_CALLBACK *PRTL_FEATURE_CONFIGURATION_CHANGE_CALLBACK;

// private
/**
 * Specifies a query for feature configuration information.
 */
typedef struct _SYSTEM_FEATURE_CONFIGURATION_QUERY
{
    RTL_FEATURE_CONFIGURATION_TYPE ConfigurationType;
    RTL_FEATURE_ID FeatureId;
} SYSTEM_FEATURE_CONFIGURATION_QUERY, *PSYSTEM_FEATURE_CONFIGURATION_QUERY;

// private
/**
 * Contains feature configuration information returned by a query.
 */
typedef struct _SYSTEM_FEATURE_CONFIGURATION_INFORMATION
{
    RTL_FEATURE_CHANGE_STAMP ChangeStamp;
    RTL_FEATURE_CONFIGURATION Configuration;
} SYSTEM_FEATURE_CONFIGURATION_INFORMATION, *PSYSTEM_FEATURE_CONFIGURATION_INFORMATION;

// private
/**
 * Identifies the type of a feature configuration update.
 */
typedef enum _SYSTEM_FEATURE_CONFIGURATION_UPDATE_TYPE
{
    SystemFeatureConfigurationUpdateTypeUpdate = 0,
    SystemFeatureConfigurationUpdateTypeOverwrite = 1,
    SystemFeatureConfigurationUpdateTypeCount = 2,
} SYSTEM_FEATURE_CONFIGURATION_UPDATE_TYPE, *PSYSTEM_FEATURE_CONFIGURATION_UPDATE_TYPE;

// private
/**
 * Describes a feature configuration update request.
 */
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
    } UNION;
} SYSTEM_FEATURE_CONFIGURATION_UPDATE, *PSYSTEM_FEATURE_CONFIGURATION_UPDATE;

// private
//typedef struct _SYSTEM_FEATURE_USAGE_SUBSCRIPTION_DETAILS
//{
//    RTL_FEATURE_ID FeatureId;
//    USHORT ReportingKind;
//    USHORT ReportingOptions;
//    RTL_FEATURE_USAGE_SUBSCRIPTION_TARGET ReportingTarget;
//} SYSTEM_FEATURE_USAGE_SUBSCRIPTION_DETAILS, *PSYSTEM_FEATURE_USAGE_SUBSCRIPTION_DETAILS;

//typedef struct _SYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE_ENTRY
//{
//    ULONG Remove;
//    RTL_FEATURE_USAGE_SUBSCRIPTION_DETAILS Details;
//} SYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE_ENTRY, *PSYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE_ENTRY;
//
//typedef struct _SYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE
//{
//    ULONG UpdateCount;
//    _Field_size_(UpdateCount) SYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE_ENTRY Updates[ANYSIZE_ARRAY];
//} SYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE, *PSYSTEM_FEATURE_USAGE_SUBSCRIPTION_UPDATE;

#if (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)

// private
/**
 * The RtlNotifyFeatureUsage routine reports usage of a staged feature to the feature configuration subsystem.
 *
 * \param FeatureUsageReport A pointer to an RTL_FEATURE_USAGE_REPORT structure that describes the feature usage.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlNotifyFeatureUsage(
    _In_ PRTL_FEATURE_USAGE_REPORT FeatureUsageReport
    );

// private
/**
 * The RtlQueryFeatureConfiguration routine retrieves the configuration of a staged feature.
 *
 * \param FeatureId The identifier of the feature to query.
 * \param ConfigurationType The RTL_FEATURE_CONFIGURATION_TYPE that selects the configuration store to query.
 * \param ChangeStamp A pointer to a variable that receives the change stamp of the returned configuration.
 * \param FeatureConfiguration A pointer to an RTL_FEATURE_CONFIGURATION structure that receives the feature configuration.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlSetFeatureConfigurations routine applies a set of feature configuration updates.
 *
 * \param PreviousChangeStamp An optional pointer to the expected previous change stamp; the update is applied only if it matches.
 * \param ConfigurationType The RTL_FEATURE_CONFIGURATION_TYPE that selects the configuration store to update.
 * \param ConfigurationUpdates A pointer to an array of RTL_FEATURE_CONFIGURATION_UPDATE entries to apply.
 * \param ConfigurationUpdateCount The number of entries in the ConfigurationUpdates array.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlQueryAllFeatureConfigurations routine retrieves all feature configurations from the specified configuration store.
 *
 * \param ConfigurationType The RTL_FEATURE_CONFIGURATION_TYPE that selects the configuration store to query.
 * \param ChangeStamp An optional pointer to a variable that receives the change stamp of the returned configurations.
 * \param Configurations A buffer that receives the array of RTL_FEATURE_CONFIGURATION entries.
 * \param ConfigurationCount A pointer to the number of entries the buffer can hold; updated with the number of entries available or written.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlQueryAllInternalFeatureConfigurations routine retrieves all internal feature configurations from the specified configuration store.
 *
 * \param ConfigurationType The RTL_FEATURE_CONFIGURATION_TYPE that selects the configuration store to query.
 * \param ChangeStamp An optional pointer to a variable that receives the change stamp of the returned configurations.
 * \param Configurations A buffer that receives the array of RTL_FEATURE_CONFIGURATION entries.
 * \param ConfigurationCount A pointer to the number of entries the buffer can hold; updated with the number of entries available or written.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryAllInternalFeatureConfigurations(
    _In_ RTL_FEATURE_CONFIGURATION_TYPE ConfigurationType,
    _Out_opt_ PRTL_FEATURE_CHANGE_STAMP ChangeStamp,
    _Out_writes_(*ConfigurationCount) PRTL_FEATURE_CONFIGURATION Configurations,
    _Inout_ PSIZE_T ConfigurationCount
    );

// private
/**
 * The RtlQueryAllInternalRuntimeFeatureConfigurations routine retrieves all internal runtime feature configurations related to the specified feature.
 *
 * \param FeatureId The identifier of the feature whose runtime configurations are queried.
 * \param ConfigurationType The RTL_FEATURE_CONFIGURATION_TYPE that selects the configuration store to query.
 * \param ChangeStamp An optional pointer to a variable that receives the change stamp of the returned configurations.
 * \param Configurations A buffer that receives the array of RTL_FEATURE_CONFIGURATION entries.
 * \param ConfigurationCount A pointer to the number of entries the buffer can hold; updated with the number of entries available or written.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryAllInternalRuntimeFeatureConfigurations(
    _In_ RTL_FEATURE_ID FeatureId,
    _In_ RTL_FEATURE_CONFIGURATION_TYPE ConfigurationType,
    _Out_opt_ PRTL_FEATURE_CHANGE_STAMP ChangeStamp,
    _Out_writes_(*ConfigurationCount) PRTL_FEATURE_CONFIGURATION Configurations,
    _Inout_ PSIZE_T ConfigurationCount
    );

// private
/**
 * The RtlQueryFeatureConfigurationChangeStamp routine returns the current global feature configuration change stamp.
 *
 * \return RTL_FEATURE_CHANGE_STAMP The current feature configuration change stamp.
 */
NTSYSAPI
RTL_FEATURE_CHANGE_STAMP
NTAPI
RtlQueryFeatureConfigurationChangeStamp(
    VOID
    );

// private
/**
 * The RtlQueryFeatureUsageNotificationSubscriptions routine retrieves the current feature usage notification subscriptions.
 *
 * \param Subscriptions A buffer that receives the array of RTL_FEATURE_USAGE_SUBSCRIPTION_DETAILS entries.
 * \param SubscriptionCount A pointer to the number of entries the buffer can hold; updated with the number of entries available or written.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryFeatureUsageNotificationSubscriptions(
    _Out_writes_(*SubscriptionCount) PRTL_FEATURE_USAGE_SUBSCRIPTION_DETAILS Subscriptions,
    _Inout_ PSIZE_T SubscriptionCount
    );

/**
 * The RtlRegisterFeatureConfigurationChangeNotification routine registers a callback invoked when the feature configuration state changes.
 *
 * \param Callback The callback invoked on feature configuration changes.
 * \param Context An optional context value passed to the callback.
 * \param ObservedChangeStamp An optional change stamp last observed by the caller.
 * \param RegistrationHandle Receives the registration handle.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlUnregisterFeatureConfigurationChangeNotification routine removes a feature configuration change notification registration.
 *
 * \param RegistrationHandle The registration handle returned when the change notification was registered.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlUnregisterFeatureConfigurationChangeNotification(
    _In_ RTL_FEATURE_CONFIGURATION_CHANGE_REGISTRATION RegistrationHandle
    );

// private
/**
 * The RtlSubscribeForFeatureUsageNotification routine subscribes for notifications about usage of the specified features.
 *
 * \param SubscriptionDetails A pointer to an array of RTL_FEATURE_USAGE_SUBSCRIPTION_DETAILS entries describing the subscriptions.
 * \param SubscriptionCount The number of entries in the SubscriptionDetails array.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlSubscribeForFeatureUsageNotification(
    _In_reads_(SubscriptionCount) PRTL_FEATURE_USAGE_SUBSCRIPTION_DETAILS SubscriptionDetails,
    _In_ SIZE_T SubscriptionCount
    );

// private
/**
 * The RtlUnsubscribeFromFeatureUsageNotifications routine removes feature usage notification subscriptions previously created by RtlSubscribeForFeatureUsageNotification.
 *
 * \param SubscriptionDetails A pointer to an array of RTL_FEATURE_USAGE_SUBSCRIPTION_DETAILS entries describing the subscriptions to remove.
 * \param SubscriptionCount The number of entries in the SubscriptionDetails array.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlOverwriteFeatureConfigurationBuffer routine replaces the current feature configuration buffer.
 *
 * \param PreviousChangeStamp An optional change stamp that must match the current one for the update to occur.
 * \param ConfigurationType The type of feature configuration to overwrite.
 * \param ConfigurationBuffer An optional buffer containing the new feature configuration.
 * \param ConfigurationBufferSize The size, in bytes, of ConfigurationBuffer.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlNotifyFeatureToggleUsage routine reports usage of a feature toggle to the feature configuration subsystem.
 *
 * \param FeatureId The identifier of the feature toggle.
 * \param Configuration The feature configuration.
 * \param UsageKind The kind of feature usage being reported.
 * \return ULONG Status or result code.
 */
NTSYSAPI
ULONG
NTAPI
RtlNotifyFeatureToggleUsage(
    _In_ ULONG FeatureId,
    _In_ ULONGLONG Configuration,
    _In_ ULONG UsageKind
    );

// rev
/**
 * The RtlGetFeatureTogglesChangeToken routine returns a token that changes whenever the feature toggle state changes.
 *
 * \return ULONG The current feature toggles change token.
 */
NTSYSAPI
ULONG
NTAPI
RtlGetFeatureTogglesChangeToken(
    VOID
    );

//
// Run Once
//

#ifndef _RTL_RUN_ONCE_DEF
#define _RTL_RUN_ONCE_DEF

//
// Run once initializer
//
/**
 * Static initializer for a run-once synchronization object.
 */
#define RTL_RUN_ONCE_INIT {0}

//
// Run once flags
//
/**
 * Flags for the run-once initialization routines.
 */
#define RTL_RUN_ONCE_CHECK_ONLY     0x00000001UL
#define RTL_RUN_ONCE_ASYNC          0x00000002UL
#define RTL_RUN_ONCE_INIT_FAILED    0x00000004UL
//
// The context stored in the run once structure must
// leave the following number of low order bits unused.
//
/**
 * Number of low-order context bits reserved by the run-once routines.
 */
#define RTL_RUN_ONCE_CTX_RESERVED_BITS 2

/**
 * Represents a one-time initialization (run-once) synchronization object.
 */
typedef union _RTL_RUN_ONCE
{
    PVOID Ptr;
} RTL_RUN_ONCE, *PRTL_RUN_ONCE;
#endif // _RTL_RUN_ONCE_DEF

/**
 * The RtlRunOnceInitialize routine initializes a one-time initialization (RTL_RUN_ONCE) structure.
 *
 * \param RunOnce A pointer to the RTL_RUN_ONCE structure to initialize.
 * \sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-rtlrunonceinitialize
 */
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
/**
 * Pointer to an RTL_RUN_ONCE_INIT_FN callback.
 */
typedef RTL_RUN_ONCE_INIT_FN *PRTL_RUN_ONCE_INIT_FN;

/**
 * The RtlRunOnceExecuteOnce routine executes a one-time initialization routine, ensuring it runs exactly once across threads.
 *
 * \param RunOnce The run-once structure that tracks initialization state.
 * \param InitFn The initialization routine to execute.
 * \param Parameter An optional parameter passed to the initialization routine.
 * \param Context An optional pointer that receives the context produced by initialization.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlRunOnceBeginInitialize routine begins a one-time initialization, optionally in asynchronous mode.
 *
 * \param RunOnce The run-once structure that tracks initialization state.
 * \param Flags Flags controlling the initialization (for example, RTL_RUN_ONCE_CHECK_ONLY).
 * \param Context An optional pointer that receives the context associated with the completed initialization.
 * \return NTSTATUS Successful or errant status.
 */
_Must_inspect_result_
NTSYSAPI
NTSTATUS
NTAPI
RtlRunOnceBeginInitialize(
    _Inout_ PRTL_RUN_ONCE RunOnce,
    _In_ ULONG Flags,
    _Outptr_opt_result_maybenull_ PVOID *Context
    );

/**
 * The RtlRunOnceComplete routine completes a one-time initialization begun with RtlRunOnceBeginInitialize.
 *
 * \param RunOnce The run-once structure that tracks initialization state.
 * \param Flags Flags controlling completion (for example, RTL_RUN_ONCE_INIT_FAILED).
 * \param Context An optional context value to associate with the completed initialization.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Constant used to derive a WNF state name key.
 */
#define WNF_STATE_KEY 0x41C64E6DA3BC0074

/**
 * The RtlEqualWnfChangeStamps routine determines whether two WNF change stamps are equal.
 *
 * \param ChangeStamp1 The first WNF change stamp to compare.
 * \param ChangeStamp2 The second WNF change stamp to compare.
 * \return Returns `TRUE` if the change stamps are equal, otherwise `FALSE`.
 */
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
/**
 * Pointer to a WNF_USER_CALLBACK callback.
 */
typedef WNF_USER_CALLBACK *PWNF_USER_CALLBACK;

/**
 * The RtlQueryWnfStateData routine retrieves the current data for a WNF state name via a user callback.
 *
 * \param ChangeStamp Receives the change stamp of the current state data.
 * \param StateName The WNF state name to query.
 * \param Callback The callback invoked with the current state data.
 * \param CallbackContext An optional context value passed to the callback.
 * \param TypeId An optional type identifier describing the state data.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlPublishWnfStateData routine publishes new state data for a WNF state name.
 *
 * \param StateName The WNF state name to publish to.
 * \param TypeId An optional pointer to the type identifier that describes the data format.
 * \param Buffer An optional pointer to the buffer that contains the state data to publish.
 * \param Length The size, in bytes, of the data in Buffer.
 * \param ExplicitScope An optional pointer to the explicit scope in which to publish the data.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * Pointer to an opaque WNF_USER_SUBSCRIPTION structure.
 */
typedef struct WNF_USER_SUBSCRIPTION *PWNF_USER_SUBSCRIPTION;

/**
 * Flag controlling WNF serialization group creation.
 */
#define WNF_CREATE_SERIALIZATION_GROUP_FLAG 0x00000001L

/**
 * The RtlSubscribeWnfStateChangeNotification routine subscribes to change notifications for a WNF state name.
 *
 * \param SubscriptionHandle Receives the subscription handle.
 * \param StateName The WNF state name to subscribe to.
 * \param ChangeStamp The change stamp from which to begin receiving notifications.
 * \param Callback The callback invoked when the state changes.
 * \param CallbackContext An optional context value passed to the callback.
 * \param TypeId An optional type identifier describing the state data.
 * \param SerializationGroup An optional serialization group for ordering callbacks.
 * \param Flags Flags controlling the subscription.
 * \return NTSTATUS Successful or errant status.
 */
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

/**
 * The RtlUnsubscribeWnfStateChangeNotification routine removes a WNF state change notification subscription.
 *
 * \param SubscriptionHandle A pointer to the user subscription to remove.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlUnsubscribeWnfStateChangeNotification(
    _In_ PWNF_USER_SUBSCRIPTION SubscriptionHandle
    );

/**
 * The RtlWnfDllUnloadCallback routine cancels any WNF subscriptions owned by a module as it is unloaded.
 *
 * \param DllBase The base address of the module being unloaded.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlWnfDllUnloadCallback(
    _In_ PVOID DllBase
    );

#endif // PHNT_VERSION >= PHNT_WINDOWS_10

#if (PHNT_VERSION >= PHNT_WINDOWS_10_20H1)
/**
 * The RtlGetReturnAddressHijackTarget routine returns the target address used for return address hijacking protection.
 *
 * \return ULONG_PTR The return address hijack target, or zero if none is configured.
 */
NTSYSAPI
ULONG_PTR
NTAPI
RtlGetReturnAddressHijackTarget(
    VOID
    );
#endif

/**
 * Flags for RtlCopyFileChunk.
 */
#define COPY_FILE_CHUNK_DUPLICATE_EXTENTS 0x00000001L // 24H2
#define VALID_COPY_FILE_CHUNK_FLAGS (COPY_FILE_CHUNK_DUPLICATE_EXTENTS)

//
// WNF
//

/**
 * The RtlAllocateWnfSerializationGroup routine allocates a WNF serialization group used to order state change notifications.
 *
 * \return ULONG The identifier of the allocated WNF serialization group.
 */
NTSYSAPI
ULONG
NTAPI
RtlAllocateWnfSerializationGroup(
    VOID
    );

/**
 * The RtlQueryWnfMetaNotification routine queries meta-notification information for a WNF state name.
 *
 * \param Result A pointer to a variable that receives the query result.
 * \param NameInfoClass The WNF_STATE_NAME_INFORMATION class that selects the information to query.
 * \param StateName The WNF state name to query.
 * \param ExplicitScope An optional pointer to the explicit scope SID for the query.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryWnfMetaNotification(
    _Out_ PULONG Result,
    _In_ WNF_STATE_NAME_INFORMATION NameInfoClass,
    _In_ WNF_STATE_NAME StateName,
    _In_opt_ PCSID ExplicitScope
    );

/**
 * The RtlQueryWnfStateDataWithExplicitScope routine retrieves the state data for a WNF state name within an explicit scope, decoding it with a caller-supplied type decoder.
 *
 * \param ChangeStamp A pointer to a variable that receives the change stamp of the returned data.
 * \param StateName The WNF state name to query.
 * \param ExplicitScope An optional pointer to the explicit scope for the query.
 * \param TypeDecoder A pointer to a callback that decodes the raw state data into the caller's buffer.
 * \param CallbackContext A caller-defined value passed to the type decoder callback.
 * \param TypeId An optional pointer to the type identifier that describes the data format.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryWnfStateDataWithExplicitScope(
    _Out_ PWNF_CHANGE_STAMP ChangeStamp,
    _In_ WNF_STATE_NAME StateName,
    _In_opt_ const VOID *ExplicitScope,
    _In_ PWNF_USER_CALLBACK TypeDecoder,
    _In_opt_ PVOID CallbackContext,
    _In_opt_ PCWNF_TYPE_ID TypeId
    );

/**
 * The RtlUnsubscribeWnfNotificationWaitForCompletion routine removes a WNF notification subscription and waits for any in-progress callbacks to complete.
 *
 * \param SubscriptionHandle A handle to the WNF notification subscription to remove.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlUnsubscribeWnfNotificationWaitForCompletion(
    _In_ HANDLE SubscriptionHandle
    );

/**
 * The RtlUnsubscribeWnfNotificationWithCompletionCallback routine cancels a WNF subscription and invokes a completion callback once cancellation finishes.
 *
 * \param SubscriptionHandle The subscription handle to cancel.
 * \param CompletionCallback An optional callback invoked when cancellation completes.
 * \param CompletionContext An optional context value passed to the completion callback.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlUnsubscribeWnfNotificationWithCompletionCallback(
    _In_ HANDLE SubscriptionHandle,
    _In_opt_ PVOID CompletionCallback,
    _In_opt_ PVOID CompletionContext
    );

//
// Property Store
//

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
// rev
/**
 * The RtlQueryPropertyStore routine retrieves the value associated with a key in the process property store.
 *
 * \param Key The key whose value is retrieved.
 * \param Context A pointer to a variable that receives the value associated with the key.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlQueryPropertyStore(
    _In_ ULONG_PTR Key,
    _Out_ PULONG_PTR Context
    );

// rev
/**
 * The RtlRemovePropertyStore routine removes a key and its value from the process property store.
 *
 * \param Key The key to remove.
 * \param Context A pointer to a variable that receives the value that was associated with the key.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlRemovePropertyStore(
    _In_ ULONG_PTR Key,
    _Out_ PULONG_PTR Context
    );

// rev
/**
 * The RtlCompareExchangePropertyStore routine atomically compares and exchanges the value associated with a key in the process property store.
 *
 * \param Key The key whose value is exchanged.
 * \param Comperand A pointer to the value that the current value is compared against.
 * \param Exchange An optional pointer to the value to store if the comparison succeeds.
 * \param Context A pointer to a variable that receives the previous value associated with the key.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlWow64ChangeProcessState routine applies a process state change to a target process through a process state change handle.
 *
 * \param ProcessStateChangeHandle A handle to the process state change object.
 * \param ProcessHandle A handle to the process whose state is changed.
 * \param StateChangeType The PROCESS_STATE_CHANGE_TYPE that specifies the state change to apply.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlWow64ChangeProcessState(
    _In_ HANDLE ProcessStateChangeHandle,
    _In_ HANDLE ProcessHandle,
    _In_ PROCESS_STATE_CHANGE_TYPE StateChangeType
    );

// rev
/**
 * The RtlWow64ChangeThreadState routine applies a thread state change to a target thread through a thread state change handle.
 *
 * \param ThreadStateChangeHandle A handle to the thread state change object.
 * \param ThreadHandle A handle to the thread whose state is changed.
 * \param StateChangeType The THREAD_STATE_CHANGE_TYPE that specifies the state change to apply.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlWow64ChangeThreadState(
    _In_ HANDLE ThreadStateChangeHandle,
    _In_ HANDLE ThreadHandle,
    _In_ THREAD_STATE_CHANGE_TYPE StateChangeType
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11

#if (PHNT_VERSION >= PHNT_WINDOWS_11)
// rev
/**
 * The RtlWow64SuspendProcess routine suspends all threads in the specified process.
 *
 * \param ProcessHandle A handle to the process to suspend.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlWow64SuspendProcess(
    _In_ HANDLE ProcessHandle
    );

// rev
/**
 * The RtlWow64SuspendThread routine suspends the specified thread.
 *
 * \param ThreadHandle A handle to the thread to suspend.
 * \param SuspendCount An optional pointer to a variable that receives the thread's previous suspend count.
 * \return NTSTATUS Successful or errant status.
 */
NTSYSAPI
NTSTATUS
NTAPI
RtlWow64SuspendThread(
    _In_ HANDLE ThreadHandle,
    _Out_opt_ PULONG SuspendCount
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_11

#if (PHNT_VERSION >= PHNT_WINDOWS_10_RS1)
// rev
/**
 * The RtlGetCurrentThreadPrimaryGroup routine returns the processor group number that is primary for the current thread.
 *
 * \return USHORT The primary processor group number of the current thread.
 */
NTSYSAPI
USHORT
NTAPI
RtlGetCurrentThreadPrimaryGroup(
    VOID
    );
#endif // PHNT_VERSION >= PHNT_WINDOWS_10_RS1

#if (PHNT_VERSION >= PHNT_WINDOWS_11_24H2)
// rev
/**
 * The RtlQueryProcessAvailableCpus routine retrieves the set of processors available to the specified process.
 *
 * \param ProcessHandle A handle to the process to query.
 * \param Affinity A pointer to a KAFFINITY_EX structure that receives the available processor affinity.
 * \param ObservedSequenceNumber The sequence number previously observed by the caller.
 * \param SequenceNumber An optional pointer to a variable that receives the current sequence number.
 * \return NTSTATUS Successful or errant status.
 */
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
/**
 * The RtlQueryProcessAvailableCpusCount routine retrieves the number of processors available to the specified process.
 *
 * \param ProcessHandle A handle to the process to query.
 * \param AvailableCpusCount A pointer to a variable that receives the number of available processors.
 * \param SequenceNumber An optional pointer to a variable that receives the current sequence number.
 * \return NTSTATUS Successful or errant status.
 */
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
