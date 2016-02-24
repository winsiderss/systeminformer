#ifndef _PH_HANDLE_H
#define _PH_HANDLE_H

struct _PH_HANDLE_TABLE;
typedef struct _PH_HANDLE_TABLE *PPH_HANDLE_TABLE;

typedef struct _PH_HANDLE_TABLE_ENTRY
{
    union
    {
        PVOID Object;
        ULONG_PTR Value;
        struct
        {
            /** The type of the entry; 1 if the entry is free, otherwise 0 if the entry is in use. */
            ULONG_PTR Type : 1;
            /**
             * Whether the entry is not locked; 1 if the entry is not locked, otherwise 0 if the
             * entry is locked.
             */
            ULONG_PTR Locked : 1;
            ULONG_PTR Value : sizeof(ULONG_PTR) * 8 - 2;
        } TypeAndValue;
    };
    union
    {
        ACCESS_MASK GrantedAccess;
        ULONG NextFreeValue;
        ULONG_PTR Value2;
    };
} PH_HANDLE_TABLE_ENTRY, *PPH_HANDLE_TABLE_ENTRY;

#define PH_HANDLE_TABLE_SAFE
#define PH_HANDLE_TABLE_FREE_COUNT 64

#define PH_HANDLE_TABLE_STRICT_FIFO 0x1
#define PH_HANDLE_TABLE_VALID_FLAGS 0x1

PPH_HANDLE_TABLE
NTAPI
PhCreateHandleTable(
    VOID
    );

VOID
NTAPI
PhDestroyHandleTable(
    _In_ _Post_invalid_ PPH_HANDLE_TABLE HandleTable
    );

BOOLEAN
NTAPI
PhLockHandleTableEntry(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _Inout_ PPH_HANDLE_TABLE_ENTRY HandleTableEntry
    );

VOID
NTAPI
PhUnlockHandleTableEntry(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _Inout_ PPH_HANDLE_TABLE_ENTRY HandleTableEntry
    );

HANDLE
NTAPI
PhCreateHandle(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _In_ PPH_HANDLE_TABLE_ENTRY HandleTableEntry
    );

BOOLEAN
NTAPI
PhDestroyHandle(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _In_ HANDLE Handle,
    _In_opt_ PPH_HANDLE_TABLE_ENTRY HandleTableEntry
    );

PPH_HANDLE_TABLE_ENTRY
NTAPI
PhLookupHandleTableEntry(
    _In_ PPH_HANDLE_TABLE HandleTable,
    _In_ HANDLE Handle
    );

typedef BOOLEAN (NTAPI *PPH_ENUM_HANDLE_TABLE_CALLBACK)(
    _In_ PPH_HANDLE_TABLE HandleTable,
    _In_ HANDLE Handle,
    _In_ PPH_HANDLE_TABLE_ENTRY HandleTableEntry,
    _In_opt_ PVOID Context
    );

VOID
NTAPI
PhEnumHandleTable(
    _In_ PPH_HANDLE_TABLE HandleTable,
    _In_ PPH_ENUM_HANDLE_TABLE_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

VOID
NTAPI
PhSweepHandleTable(
    _In_ PPH_HANDLE_TABLE HandleTable,
    _In_ PPH_ENUM_HANDLE_TABLE_CALLBACK Callback,
    _In_opt_ PVOID Context
    );

typedef enum _PH_HANDLE_TABLE_INFORMATION_CLASS
{
    HandleTableBasicInformation,
    HandleTableFlagsInformation,
    MaxHandleTableInfoClass
} PH_HANDLE_TABLE_INFORMATION_CLASS;

typedef struct _PH_HANDLE_TABLE_BASIC_INFORMATION
{
    ULONG Count;
    ULONG Flags;
    ULONG TableLevel;
} PH_HANDLE_TABLE_BASIC_INFORMATION, *PPH_HANDLE_TABLE_BASIC_INFORMATION;

typedef struct _PH_HANDLE_TABLE_FLAGS_INFORMATION
{
    ULONG Flags;
} PH_HANDLE_TABLE_FLAGS_INFORMATION, *PPH_HANDLE_TABLE_FLAGS_INFORMATION;

NTSTATUS
NTAPI
PhQueryInformationHandleTable(
    _In_ PPH_HANDLE_TABLE HandleTable,
    _In_ PH_HANDLE_TABLE_INFORMATION_CLASS InformationClass,
    _Out_writes_bytes_opt_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength,
    _Out_opt_ PULONG ReturnLength
    );

NTSTATUS
NTAPI
PhSetInformationHandleTable(
    _Inout_ PPH_HANDLE_TABLE HandleTable,
    _In_ PH_HANDLE_TABLE_INFORMATION_CLASS InformationClass,
    _In_reads_bytes_(BufferLength) PVOID Buffer,
    _In_ ULONG BufferLength
    );

#endif
