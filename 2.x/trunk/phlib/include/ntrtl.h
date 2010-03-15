#ifndef NTRTL_H
#define NTRTL_H

#include <ntldr.h>

// Defines for forwarded ntdll functions

#define RtlExitUserProcess ExitProcess
#define RtlExitUserThread ExitThread

// Linked lists

FORCEINLINE VOID InitializeListHead(
    __in PLIST_ENTRY ListHead
    )
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

#define IsListEmpty(ListHead) ((ListHead)->Flink == (ListHead))

FORCEINLINE BOOLEAN RemoveEntryList(
    __in PLIST_ENTRY Entry
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

FORCEINLINE PLIST_ENTRY RemoveHeadList(
    __in PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Flink;
    Flink = Entry->Flink;
    ListHead->Flink = Flink;
    Flink->Blink = ListHead;

    return Entry;
}

FORCEINLINE PLIST_ENTRY RemoveTailList(
    __in PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Blink;
    Blink = Entry->Blink;
    ListHead->Blink = Blink;
    Blink->Flink = ListHead;

    return Entry;
}

FORCEINLINE VOID InsertTailList(
    __in PLIST_ENTRY ListHead,
    __in PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Blink;

    Blink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = Blink;
    Blink->Flink = Entry;
    ListHead->Blink = Entry;
}

FORCEINLINE VOID InsertHeadList(
    __in PLIST_ENTRY ListHead,
    __in PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Flink;

    Flink = ListHead->Flink;
    Entry->Flink = Flink;
    Entry->Blink = ListHead;
    Flink->Blink = Entry;
    ListHead->Flink = Entry;
}

#define PopEntryList(ListHead) \
    (ListHead)->Next; \
    { \
        PSINGLE_LIST_ENTRY FirstEntry; \
        FirstEntry = (ListHead)->Next; \
        if (FirstEntry != NULL) \
        { \
            (ListHead)->Next = FirstEntry->Next; \
        } \
    }

#define PushEntryList(ListHead,Entry) \
    (Entry)->Next = (ListHead)->Next; \
    (ListHead)->Next = (Entry)

// Synchronization

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeCriticalSection(
    __out PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeCriticalSectionAndSpinCount(
    __inout PRTL_CRITICAL_SECTION CriticalSection,
    __in ULONG SpinCount
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteCriticalSection(
    __inout PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlEnterCriticalSection(
    __inout PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlLeaveCriticalSection(
    __inout PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
LOGICAL
NTAPI
RtlTryEnterCriticalSection(
    __inout PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
LOGICAL
NTAPI
RtlIsCriticalSectionLocked(
    __in PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
LOGICAL
NTAPI
RtlIsCriticalSectionLockedByThread(
    __in PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
ULONG
NTAPI
RtlGetCriticalSectionRecursionCount(
    __in PRTL_CRITICAL_SECTION CriticalSection
    );

NTSYSAPI
ULONG
NTAPI
RtlSetCriticalSectionSpinCount(
    __inout PRTL_CRITICAL_SECTION CriticalSection,
    __in ULONG SpinCount
    );

NTSYSAPI
ULONG
NTAPI
RtlGetCurrentProcessorNumber();

// Environment

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateEnvironment(
    __in BOOLEAN CloneCurrentEnvironment,
    __out PVOID *Environment
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDestroyEnvironment(
    __in PVOID Environment
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetCurrentEnvironment(
    __in PVOID Environment,
    __out_opt PVOID *PreviousEnvironment
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetEnvironmentVariable(
    __in_opt PVOID *Environment,
    __in PUNICODE_STRING Name,
    __in PUNICODE_STRING Value
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryEnvironmentVariable_U(
    __in PVOID Environment,
    __in PUNICODE_STRING Name,
    __out PUNICODE_STRING Value
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlExpandEnvironmentStrings_U(
    __in_opt PVOID Environment,
    __in PUNICODE_STRING Source,
    __out PUNICODE_STRING Destination,
    __out_opt PULONG ReturnedLength
    );

// Processes

#define DOS_MAX_COMPONENT_LENGTH 255
#define DOS_MAX_PATH_LENGTH (DOS_MAX_COMPONENT_LENGTH + 5)

typedef struct _CURDIR
{
    UNICODE_STRING DosPath;
    HANDLE Handle;
} CURDIR, *PCURDIR;

#define RTL_USER_PROC_CURDIR_CLOSE 0x00000002
#define RTL_USER_PROC_CURDIR_INHERIT 0x00000003

typedef struct _RTL_DRIVE_LETTER_CURDIR
{
    USHORT Flags;
    USHORT Length;
    ULONG TimeStamp;
    STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

#define RTL_MAX_DRIVE_LETTERS 32
#define RTL_DRIVE_LETTER_VALID (USHORT)0x0001

typedef struct _RTL_USER_PROCESS_PARAMETERS
{
    ULONG MaximumLength;
    ULONG Length;

    ULONG Flags;
    ULONG DebugFlags;

    HANDLE ConsoleHandle;
    ULONG  ConsoleFlags;
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

    ULONG EnvironmentSize;
    ULONG EnvironmentVersion;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

// Threads

typedef NTSTATUS (NTAPI *PUSER_THREAD_START_ROUTINE)(
    __in PVOID ThreadParameter
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
RtlCreateUserThread(
    __in HANDLE Process,
    __in_opt PSECURITY_DESCRIPTOR ThreadSecurityDescriptor,
    __in BOOLEAN CreateSuspended,
    __in_opt ULONG ZeroBits,
    __in_opt SIZE_T MaximumStackSize,
    __in_opt SIZE_T CommittedStackSize,
    __in PUSER_THREAD_START_ROUTINE StartAddress,
    __in_opt PVOID Parameter,
    __out_opt PHANDLE Thread,
    __out_opt PCLIENT_ID ClientId
    );

#ifdef _M_X64
NTSYSCALLAPI
NTSTATUS
NTAPI
RtlWow64GetThreadContext(
    __in HANDLE ThreadHandle,
    __inout PWOW64_CONTEXT ThreadContext
    );
#endif

#ifdef _M_X64
NTSYSCALLAPI
NTSTATUS
NTAPI
RtlWow64SetThreadContext(
    __in HANDLE ThreadHandle,
    __in PWOW64_CONTEXT ThreadContext
    );
#endif

// Heaps

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
#define RTL_HEAP_SETTABLE_FLAGS (USHORT)0x00E0
#define RTL_HEAP_UNCOMMITTED_RANGE (USHORT)0x0100
#define RTL_HEAP_PROTECTED_ENTRY (USHORT)0x0200

typedef struct _RTL_HEAP_TAG
{
    ULONG NumberOfAllocations;
    ULONG NumberOfFrees;
    SIZE_T BytesAllocated;
    USHORT TagIndex;
    USHORT CreatorBackTraceIndex;
    WCHAR TagName[24];
} RTL_HEAP_TAG, *PRTL_HEAP_TAG;

typedef struct _RTL_HEAP_INFORMATION
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
} RTL_HEAP_INFORMATION, *PRTL_HEAP_INFORMATION;

typedef struct _RTL_PROCESS_HEAPS
{
    ULONG NumberOfHeaps;
    RTL_HEAP_INFORMATION Heaps[1];
} RTL_PROCESS_HEAPS, *PRTL_PROCESS_HEAPS;

typedef NTSTATUS (NTAPI *PRTL_HEAP_COMMIT_ROUTINE)(
    __in PVOID Base,
    __inout PVOID *CommitAddress,
    __inout PSIZE_T CommitSize
    );

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

NTSYSAPI
PVOID
NTAPI
RtlCreateHeap(
    __in ULONG Flags,
    __in_opt PVOID HeapBase,
    __in_opt SIZE_T ReserveSize,
    __in_opt SIZE_T CommitSize,
    __in_opt PVOID Lock,
    __in_opt PRTL_HEAP_PARAMETERS Parameters
    );

NTSYSAPI
PVOID
NTAPI
RtlDestroyHeap(
    __in __post_invalid PVOID HeapHandle
    );

NTSYSAPI
PVOID
NTAPI
RtlAllocateHeap(
    __in PVOID HeapHandle,
    __in_opt ULONG Flags,
    __in SIZE_T Size
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlFreeHeap(
    __in PVOID HeapHandle,
    __in_opt ULONG Flags,
    __in __post_invalid PVOID BaseAddress
    );

NTSYSAPI
SIZE_T
NTAPI
RtlSizeHeap(
    __in PVOID HeapHandle,
    __in ULONG Flags,
    __in PVOID BaseAddress
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlZeroHeap(
    __in PVOID HeapHandle,
    __in ULONG Flags
    );

#define RtlProcessHeap() (NtCurrentPeb()->ProcessHeap)

NTSYSAPI
BOOLEAN
NTAPI
RtlLockHeap(
    __in PVOID HeapHandle
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlUnlockHeap(
    __in PVOID HeapHandle
    );

NTSYSAPI
PVOID
NTAPI
RtlReAllocateHeap(
    __in PVOID HeapHandle,
    __in ULONG Flags,
    __in PVOID BaseAddress,
    __in SIZE_T Size
    );

NTSYSAPI
SIZE_T
NTAPI
RtlCompactHeap(
    __in PVOID HeapHandle,
    __in ULONG Flags
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlValidateHeap(
    __in PVOID HeapHandle,
    __in ULONG Flags,
    __in PVOID BaseAddress
    );

// Transactions

#if (PHNT_VERSION >= PHNT_VISTA)
/* * */
NTSYSAPI
HANDLE
NTAPI
RtlGetCurrentTransaction();
#endif

#if (PHNT_VERSION >= PHNT_VISTA)
/* * */
NTSYSAPI
BOOLEAN
NTAPI
RtlSetCurrentTransaction(
    __in HANDLE TransactionHandle
    );
#endif

// LUIDs

FORCEINLINE LUID RtlConvertLongToLuid(
    __in LONG Long
    )
{
    LUID tempLuid;
    LARGE_INTEGER tempLi;

    tempLi.QuadPart = Long;
    tempLuid.LowPart = tempLi.LowPart;
    tempLuid.HighPart = tempLi.HighPart;

    return tempLuid;
}

// Debugging

typedef struct RTL_PROCESS_BACKTRACES *PRTL_PROCESS_BACKTRACES;
typedef struct RTL_PROCESS_LOCKS *PRTL_PROCESS_LOCKS;

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
    PRTL_PROCESS_MODULES Modules;
    PRTL_PROCESS_BACKTRACES BackTraces;
    PRTL_PROCESS_HEAPS Heaps;
    PRTL_PROCESS_LOCKS Locks;
    PVOID SpecificHeap;
    HANDLE TargetProcessHandle;
    PVOID Reserved[6];
} RTL_DEBUG_INFORMATION, *PRTL_DEBUG_INFORMATION;

NTSYSCALLAPI
PRTL_DEBUG_INFORMATION
NTAPI
RtlCreateQueryDebugBuffer(
    __in_opt ULONG MaximumCommit,
    __in BOOLEAN UseEventPair
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
RtlDestroyQueryDebugBuffer(
    __in PRTL_DEBUG_INFORMATION Buffer
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
RtlQueryProcessDebugInformation(
    __in HANDLE UniqueProcessId,
    __in ULONG Flags,
    __inout PRTL_DEBUG_INFORMATION Buffer
    );

#define RTL_QUERY_PROCESS_MODULES 0x00000001
#define RTL_QUERY_PROCESS_BACKTRACES 0x00000002
#define RTL_QUERY_PROCESS_HEAP_SUMMARY 0x00000004
#define RTL_QUERY_PROCESS_HEAP_TAGS 0x00000008
#define RTL_QUERY_PROCESS_HEAP_ENTRIES 0x00000010
#define RTL_QUERY_PROCESS_LOCKS 0x00000020
#define RTL_QUERY_PROCESS_MODULES32 0x00000040
#define RTL_QUERY_PROCESS_NONINVASIVE 0x80000000

// Messages

NTSYSCALLAPI
NTSTATUS
NTAPI
RtlFindMessage(
    __in PVOID DllHandle,
    __in ULONG MessageTableId,
    __in ULONG MessageLanguageId,
    __in ULONG MessageId,
    __out PMESSAGE_RESOURCE_ENTRY *MessageEntry
    );

// Errors

NTSYSCALLAPI
ULONG
NTAPI
RtlNtStatusToDosError(
    __in NTSTATUS Status
    );

// Strings

FORCEINLINE VOID RtlInitUnicodeString(
    __out PUNICODE_STRING DestinationString,
    __in PWSTR SourceString
    )
{
    DestinationString->MaximumLength = DestinationString->Length = (USHORT)(wcslen(SourceString) * sizeof(WCHAR));
    DestinationString->Buffer = SourceString;
}

NTSYSAPI
NTSTATUS
NTAPI
RtlAnsiStringToUnicodeString(
    __inout PUNICODE_STRING DestinationString,
    __in PANSI_STRING SourceString,
    __in BOOLEAN AllocateDestinationString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlUnicodeStringToAnsiString(
    __inout PANSI_STRING DestinationString,
    __in PUNICODE_STRING SourceString,
    __in BOOLEAN AllocateDestinationString
    );

NTSYSAPI
LONG
NTAPI
RtlCompareUnicodeString(
    __in PUNICODE_STRING String1,
    __in PUNICODE_STRING String2,
    __in BOOLEAN CaseInSensitive
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlEqualUnicodeString(
    __in PUNICODE_STRING String1,
    __in PUNICODE_STRING String2,
    __in BOOLEAN CaseInSensitive
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlPrefixUnicodeString(
    __in PUNICODE_STRING String1,
    __in PUNICODE_STRING String2,
    __in BOOLEAN CaseInSensitive
    );

#define RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END 0x00000001
#define RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET 0x00000002
#define RTL_FIND_CHAR_IN_UNICODE_STRING_CASE_INSENSITIVE 0x00000004

NTSYSAPI
NTSTATUS
NTAPI
RtlFindCharInUnicodeString(
    __in ULONG Flags,
    __in PUNICODE_STRING StringToSearch,
    __in PUNICODE_STRING CharSet,
    __out PUSHORT NonInclusivePrefixLength
    );

NTSYSAPI
VOID
NTAPI
RtlFreeUnicodeString(
    __in PUNICODE_STRING UnicodeString
    );

NTSYSAPI
VOID
NTAPI
RtlFreeAnsiString(
    __in PANSI_STRING AnsiString
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
RtlMultiByteToUnicodeN(
    __out PWSTR UnicodeString,
    __in ULONG MaxBytesInUnicodeString,
    __out_opt PULONG BytesInUnicodeString,
    __in PSTR MultiByteString,
    __in ULONG BytesInMultiByteString
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
RtlMultiByteToUnicodeSize(
    __out PULONG BytesInUnicodeString,
    __in PSTR MultiByteString,
    __in ULONG BytesInMultiByteString
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
RtlUnicodeToMultiByteN(
    __out PSTR MultiByteString,
    __in ULONG MaxBytesInMultiByteString,
    __out_opt PULONG BytesInMultiByteString,
    __in PWSTR UnicodeString,
    __in ULONG BytesInUnicodeString
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
RtlUnicodeToMultiByteSize(
    __out PULONG BytesInMultiByteString,
    __in PWSTR UnicodeString,
    __in ULONG BytesInUnicodeString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlStringFromGUID(
    __in PGUID Guid,
    __out PUNICODE_STRING GuidString
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGUIDFromString(
    __in PUNICODE_STRING GuidString,
    __out PGUID Guid
    );

// Bitmaps

typedef struct _RTL_BITMAP
{
    ULONG SizeOfBitMap;
    PULONG Buffer;
} RTL_BITMAP, *PRTL_BITMAP;

NTSYSAPI
VOID
NTAPI
RtlInitializeBitMap(
    __out PRTL_BITMAP BitMapHeader,
    __in PULONG BitMapBuffer,
    __in ULONG SizeOfBitMap
    );

NTSYSAPI
VOID
NTAPI
RtlClearBit(
    __in PRTL_BITMAP BitMapHeader,
    __in_range(<, BitMapHeader->SizeOfBitMap) ULONG BitNumber
    );

NTSYSAPI
VOID
NTAPI
RtlSetBit(
    __in PRTL_BITMAP BitMapHeader,
    __in_range(<, BitMapHeader->SizeOfBitMap) ULONG BitNumber
    );

__checkReturn
NTSYSAPI
BOOLEAN
NTAPI
RtlTestBit(
    __in PRTL_BITMAP BitMapHeader,
    __in_range(<, BitMapHeader->SizeOfBitMap) ULONG BitNumber
    );

NTSYSAPI
VOID
NTAPI
RtlClearAllBits(
    __in PRTL_BITMAP BitMapHeader
    );

NTSYSAPI
VOID
NTAPI
RtlSetAllBits(
    __in PRTL_BITMAP BitMapHeader
    );

__success(return != -1)
__checkReturn
NTSYSAPI
ULONG
NTAPI
RtlFindClearBits(
    __in PRTL_BITMAP BitMapHeader,
    __in ULONG NumberToFind,
    __in ULONG HintIndex
    );

__success(return != -1)
__checkReturn
NTSYSAPI
ULONG
NTAPI
RtlFindSetBits(
    __in PRTL_BITMAP BitMapHeader,
    __in ULONG NumberToFind,
    __in ULONG HintIndex
    );

__success(return != -1)
NTSYSAPI
ULONG
NTAPI
RtlFindClearBitsAndSet(
    __in PRTL_BITMAP BitMapHeader,
    __in ULONG NumberToFind,
    __in ULONG HintIndex
    );

__success(return != -1)
NTSYSAPI
ULONG
NTAPI
RtlFindSetBitsAndClear(
    __in PRTL_BITMAP BitMapHeader,
    __in ULONG NumberToFind,
    __in ULONG HintIndex
    );

NTSYSAPI
VOID
NTAPI
RtlClearBits(
    __in PRTL_BITMAP BitMapHeader,
    __in_range(0, BitMapHeader->SizeOfBitMap - NumberToClear) ULONG StartingIndex,
    __in_range(0, BitMapHeader->SizeOfBitMap - StartingIndex) ULONG NumberToClear
    );

NTSYSAPI
VOID
NTAPI
RtlSetBits(
    __in PRTL_BITMAP BitMapHeader,
    __in_range(0, BitMapHeader->SizeOfBitMap - NumberToSet) ULONG StartingIndex,
    __in_range(0, BitMapHeader->SizeOfBitMap - StartingIndex) ULONG NumberToSet
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
    __in PRTL_BITMAP BitMapHeader,
    __out_ecount_part(SizeOfRunArray, return) PRTL_BITMAP_RUN RunArray,
    __in_range(>, 0) ULONG SizeOfRunArray,
    __in BOOLEAN LocateLongestRuns
    );

NTSYSAPI
ULONG
NTAPI
RtlFindLongestRunClear(
    __in PRTL_BITMAP BitMapHeader,
    __out PULONG StartingIndex
    );

NTSYSAPI
ULONG
NTAPI
RtlFindFirstRunClear(
    __in PRTL_BITMAP BitMapHeader,
    __out PULONG StartingIndex
    );

__checkReturn
FORCEINLINE
BOOLEAN
RtlCheckBit(
    __in PRTL_BITMAP BitMapHeader,
    __in_range(<, BitMapHeader->SizeOfBitMap) ULONG BitPosition
    )
{
#ifdef _M_IX86
    return (((PLONG)BitMapHeader->Buffer)[BitPosition / 32] >> (BitPosition % 32)) & 0x1;
#else
    return BitTest64((LONG64 const *)BitMapHeader->Buffer, (LONG64)BitPosition);
#endif
}

NTSYSAPI
ULONG
NTAPI
RtlNumberOfClearBits(
    __in PRTL_BITMAP BitMapHeader
    );

NTSYSAPI
ULONG
NTAPI
RtlNumberOfSetBits(
    __in PRTL_BITMAP BitMapHeader
    );

__checkReturn
NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsClear(
    __in PRTL_BITMAP BitMapHeader,
    __in ULONG StartingIndex,
    __in ULONG Length
    );

__checkReturn
NTSYSAPI
BOOLEAN
NTAPI
RtlAreBitsSet(
    __in PRTL_BITMAP BitMapHeader,
    __in ULONG StartingIndex,
    __in ULONG Length
    );

NTSYSAPI
ULONG
NTAPI
RtlFindNextForwardRunClear(
    __in PRTL_BITMAP BitMapHeader,
    __in ULONG FromIndex,
    __out PULONG StartingRunIndex
    );

NTSYSAPI
ULONG
NTAPI
RtlFindLastBackwardRunClear(
    __in PRTL_BITMAP BitMapHeader,
    __in ULONG FromIndex,
    __out PULONG StartingRunIndex
    );

// SIDs

__checkReturn
NTSYSAPI
BOOLEAN
NTAPI
RtlValidSid(
    __in PSID Sid
    );

__checkReturn
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualSid(
    __in PSID Sid1,
    __in PSID Sid2
    );

NTSYSAPI
ULONG
NTAPI
RtlLengthRequiredSid(
    __in ULONG SubAuthorityCount
    );

NTSYSAPI
PVOID
NTAPI
RtlFreeSid(
    __in __post_invalid PSID Sid
    );

__checkReturn
NTSYSAPI
NTSTATUS
NTAPI
RtlAllocateAndInitializeSid(
    __in PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    __in UCHAR SubAuthorityCount,
    __in ULONG SubAuthority0,
    __in ULONG SubAuthority1,
    __in ULONG SubAuthority2,
    __in ULONG SubAuthority3,
    __in ULONG SubAuthority4,
    __in ULONG SubAuthority5,
    __in ULONG SubAuthority6,
    __in ULONG SubAuthority7,
    __deref_out PSID *Sid
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlInitializeSid(
    __out PSID Sid,
    __in PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
    __in UCHAR SubAuthorityCount
    );

NTSYSAPI
PSID_IDENTIFIER_AUTHORITY
NTAPI
RtlIdentifierAuthoritySid(
    __in PSID Sid
    );

NTSYSAPI
PULONG
NTAPI
RtlSubAuthoritySid(
    __in PSID Sid,
    __in ULONG SubAuthority
    );

NTSYSAPI
PUCHAR
NTAPI
RtlSubAuthorityCountSid(
    __in PSID Sid
    );

NTSYSAPI
ULONG
NTAPI
RtlLengthSid(
    __in PSID Sid
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCopySid(
    __in ULONG DestinationSidLength,
    __in_bcount(DestinationSidLength) PSID DestinationSid,
    __in PSID SourceSid
    );

#if (PHNT_VERSION >= PHNT_VISTA)
NTSYSAPI
NTSTATUS
NTAPI
RtlCreateServiceSid(
    __in PUNICODE_STRING ServiceName,
    __out_bcount(*ServiceSidLength) PSID ServiceSid,
    __inout PULONG ServiceSidLength
    );
#endif

#define MAX_UNICODE_STACK_BUFFER_LENGTH 256

NTSYSAPI
NTSTATUS
NTAPI
RtlConvertSidToUnicodeString(
    __inout PUNICODE_STRING UnicodeString,
    __in PSID Sid,
    __in BOOLEAN AllocateDestinationString
    );

// LUIDs

#define RtlEqualLuid(L1, L2) \
    (((L1)->LowPart == (L2)->LowPart) && \
    ((L1)->HighPart == (L2)->HighPart))

#define RtlIsZeroLuid(L1) ((BOOLEAN)(((L1)->LowPart | (L1)->HighPart) == 0))

NTSYSAPI
VOID
NTAPI
RtlCopyLuid(
    __out PLUID DestinationLuid,
    __in PLUID SourceLuid
    );

// Security Descriptors

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateSecurityDescriptor(
    __out PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in ULONG Revision
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateSecurityDescriptorRelative(
    __out PISECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
    __in ULONG Revision
    );

__checkReturn
NTSYSAPI
BOOLEAN
NTAPI
RtlValidSecurityDescriptor(
    __in PSECURITY_DESCRIPTOR SecurityDescriptor
    );

NTSYSAPI
ULONG
NTAPI
RtlLengthSecurityDescriptor(
    __in PSECURITY_DESCRIPTOR SecurityDescriptor
    );

__checkReturn
NTSYSAPI
BOOLEAN
NTAPI
RtlValidRelativeSecurityDescriptor(
    __in_bcount(SecurityDescriptorLength) PSECURITY_DESCRIPTOR SecurityDescriptorInput,
    __in ULONG SecurityDescriptorLength,
    __in SECURITY_INFORMATION RequiredInformation
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetControlSecurityDescriptor(
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __out PSECURITY_DESCRIPTOR_CONTROL Control,
    __out PULONG Revision
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetControlSecurityDescriptor(
     __inout PSECURITY_DESCRIPTOR SecurityDescriptor,
     __in SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
     __in SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet
     );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetAttributesSecurityDescriptor(
    __inout PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in SECURITY_DESCRIPTOR_CONTROL Control,
    __out PULONG Revision
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlGetSecurityDescriptorRMControl(
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __out PUCHAR RMControl
    );

NTSYSAPI
VOID
NTAPI
RtlSetSecurityDescriptorRMControl(
    __inout PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in_opt PUCHAR RMControl
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetDaclSecurityDescriptor(
    __inout PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in BOOLEAN DaclPresent,
    __in_opt PACL Dacl,
    __in_opt BOOLEAN DaclDefaulted
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetDaclSecurityDescriptor(
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __out PBOOLEAN DaclPresent,
    __out PACL *Dacl,
    __out PBOOLEAN DaclDefaulted
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetSaclSecurityDescriptor(
    __inout PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in BOOLEAN SaclPresent,
    __in_opt PACL Sacl,
    __in_opt BOOLEAN SaclDefaulted
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetSaclSecurityDescriptor(
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __out PBOOLEAN SaclPresent,
    __out PACL *Sacl,
    __out PBOOLEAN SaclDefaulted
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetSaclSecurityDescriptor(
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __out PBOOLEAN SaclPresent,
    __out PACL *Sacl,
    __out PBOOLEAN SaclDefaulted
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetOwnerSecurityDescriptor(
    __inout PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in_opt PSID Owner,
    __in_opt BOOLEAN OwnerDefaulted
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetOwnerSecurityDescriptor(
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __out PSID *Owner,
    __out PBOOLEAN OwnerDefaulted
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetGroupSecurityDescriptor(
    __inout PSECURITY_DESCRIPTOR SecurityDescriptor,
    __in_opt PSID Group,
    __in_opt BOOLEAN GroupDefaulted
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetGroupSecurityDescriptor(
    __in PSECURITY_DESCRIPTOR SecurityDescriptor,
    __out PSID *Group,
    __out PBOOLEAN GroupDefaulted
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlMakeSelfRelativeSD(
    __in PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
    __out_bcount(*BufferLength) PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    __inout PULONG BufferLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAbsoluteToSelfRelativeSD(
    __in PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
    __out_bcount_part_opt(*BufferLength, *BufferLength) PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    __inout PULONG BufferLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSelfRelativeToAbsoluteSD(
    __in PSECURITY_DESCRIPTOR SelfRelativeSecurityDescriptor,
    __out_bcount_part_opt(*AbsoluteSecurityDescriptorSize, *AbsoluteSecurityDescriptorSize) PSECURITY_DESCRIPTOR AbsoluteSecurityDescriptor,
    __inout PULONG AbsoluteSecurityDescriptorSize,
    __out_bcount_part_opt(*DaclSize, *DaclSize) PACL Dacl,
    __inout PULONG DaclSize,
    __out_bcount_part_opt(*SaclSize, *SaclSize) PACL Sacl,
    __inout PULONG SaclSize,
    __out_bcount_part_opt(*OwnerSize, *OwnerSize) PSID Owner,
    __inout PULONG OwnerSize,
    __out_bcount_part_opt(*PrimaryGroupSize, *PrimaryGroupSize) PSID PrimaryGroup,
    __inout PULONG PrimaryGroupSize
    );

// ACLs

NTSYSAPI
NTSTATUS
NTAPI
RtlCreateAcl(
    __out_bcount(AclLength) PACL Acl,
    __in ULONG AclLength,
    __in ULONG AclRevision
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlValidAcl(
    __in PACL Acl
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryInformationAcl(
    __in PACL Acl,
    __out_bcount(AclInformationLength) PVOID AclInformation,
    __in ULONG AclInformationLength,
    __in ACL_INFORMATION_CLASS AclInformationClass
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetInformationAcl(
    __inout PACL Acl,
    __in_bcount(AclInformationLength) PVOID AclInformation,
    __in ULONG AclInformationLength,
    __in ACL_INFORMATION_CLASS AclInformationClass
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAce(
    __inout PACL Acl,
    __in ULONG AceRevision,
    __in ULONG StartingAceIndex,
    __in_bcount(AceListLength) PVOID AceList,
    __in ULONG AceListLength
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlDeleteAce(
    __inout PACL Acl,
    __in ULONG AceIndex
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlGetAce(
    __in PACL Acl,
    __in ULONG AceIndex,
    __deref_out PVOID *Ace
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessAllowedAce(
    __inout PACL Acl,
    __in ULONG AceRevision,
    __in ACCESS_MASK AccessMask,
    __in PSID Sid
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessAllowedAceEx(
    __inout PACL Acl,
    __in ULONG AceRevision,
    __in ULONG AceFlags,
    __in ACCESS_MASK AccessMask,
    __in PSID Sid
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessDeniedAce(
    __inout PACL Acl,
    __in ULONG AceRevision,
    __in ACCESS_MASK AccessMask,
    __in PSID Sid
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessDeniedAceEx(
    __inout PACL Acl,
    __in ULONG AceRevision,
    __in ULONG AceFlags,
    __in ACCESS_MASK AccessMask,
    __in PSID Sid
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAuditAccessAce(
    __inout PACL Acl,
    __in ULONG AceRevision,
    __in ACCESS_MASK AccessMask,
    __in PSID Sid,
    __in BOOLEAN AuditSuccess,
    __in BOOLEAN AuditFailure
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAuditAccessAceEx(
    __inout PACL Acl,
    __in ULONG AceRevision,
    __in ULONG AceFlags,
    __in ACCESS_MASK AccessMask,
    __in PSID Sid,
    __in BOOLEAN AuditSuccess,
    __in BOOLEAN AuditFailure
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessAllowedObjectAce(
    __inout PACL Acl,
    __in ULONG AceRevision,
    __in ULONG AceFlags,
    __in ACCESS_MASK AccessMask,
    __in_opt PGUID ObjectTypeGuid,
    __in_opt PGUID InheritedObjectTypeGuid,
    __in PSID Sid
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAccessDeniedObjectAce(
    __inout PACL Acl,
    __in ULONG AceRevision,
    __in ULONG AceFlags,
    __in ACCESS_MASK AccessMask,
    __in_opt PGUID ObjectTypeGuid,
    __in_opt PGUID InheritedObjectTypeGuid,
    __in PSID Sid
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddAuditAccessObjectAce(
    __inout PACL Acl,
    __in ULONG AceRevision,
    __in ULONG AceFlags,
    __in ACCESS_MASK AccessMask,
    __in_opt PGUID ObjectTypeGuid,
    __in_opt PGUID InheritedObjectTypeGuid,
    __in PSID Sid,
    __in BOOLEAN AuditSuccess,
    __in BOOLEAN AuditFailure
    );

NTSYSAPI
BOOLEAN
NTAPI
RtlFirstFreeAce(
    __in PACL Acl,
    __out PVOID *FirstFree
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlAddCompoundAce(
    __inout PACL Acl,
    __in ULONG AceRevision,
    __in UCHAR AceType,
    __in ACCESS_MASK AccessMask,
    __in PSID ServerSid,
    __in PSID ClientSid
    );

// Security objects

NTSYSCALLAPI
NTSTATUS
NTAPI
RtlNewSecurityObject(
    __in_opt PSECURITY_DESCRIPTOR ParentDescriptor,
    __in_opt PSECURITY_DESCRIPTOR CreatorDescriptor,
    __out PSECURITY_DESCRIPTOR *NewDescriptor,
    __in BOOLEAN IsDirectoryObject,
    __in_opt HANDLE Token,
    __in PGENERIC_MAPPING GenericMapping
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
RtlDeleteSecurityObject(
    __inout PSECURITY_DESCRIPTOR *ObjectDescriptor
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
RtlCopySecurityDescriptor(
    __in PSECURITY_DESCRIPTOR InputSecurityDescriptor,
    __out PSECURITY_DESCRIPTOR *OutputSecurityDescriptor
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
RtlQuerySecurityObject(
     __in PSECURITY_DESCRIPTOR ObjectDescriptor,
     __in SECURITY_INFORMATION SecurityInformation,
     __out_opt PSECURITY_DESCRIPTOR ResultantDescriptor,
     __in ULONG DescriptorLength,
     __out PULONG ReturnLength
     );

NTSYSCALLAPI
NTSTATUS
NTAPI
RtlSetSecurityObject(
    __in SECURITY_INFORMATION SecurityInformation,
    __in PSECURITY_DESCRIPTOR ModificationDescriptor,
    __inout PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    __in PGENERIC_MAPPING GenericMapping,
    __in_opt HANDLE Token
    );

#endif
