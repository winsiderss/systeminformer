#ifndef NTRTL_H
#define NTRTL_H

#include <ntldr.h>

// Defines for forwarded ntdll functions

#define RtlDestroyHeap HeapDestroy
#define RtlAllocateHeap HeapAlloc
#define RtlFreeHeap HeapFree
#define RtlReAllocateHeap HeapReAlloc
#define RtlSizeHeap HeapSize

#define RtlInitializeCriticalSection InitializeCriticalSection
#define RtlDeleteCriticalSection DeleteCriticalSection
#define RtlEnterCriticalSection EnterCriticalSection
#define RtlTryEnterCriticalSection TryEnterCriticalSection
#define RtlLeaveCriticalSection LeaveCriticalSection

#define RtlExitUserProcess ExitProcess
#define RtlExitUserThread ExitThread

#define RtlGetCurrentProcessorNumber GetCurrentProcessorNumber

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

// Strings

FORCEINLINE VOID RtlInitUnicodeString(
    __out PUNICODE_STRING DestinationString,
    __in PWSTR SourceString
    )
{
    DestinationString->MaximumLength = DestinationString->Length = (USHORT)(wcslen(SourceString) * sizeof(WCHAR));
    DestinationString->Buffer = SourceString;
}

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
