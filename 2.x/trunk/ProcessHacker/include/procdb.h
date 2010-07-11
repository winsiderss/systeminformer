#ifndef PROCDB_H
#define PROCDB_H

#define PH_PROCDB_HASH_SIZE 16

#define PH_PROCDB_ENTRY_MATCH_HASH 0x1
#define PH_PROCDB_ENTRY_HASH_VALID 0x2
#define PH_PROCDB_ENTRY_SAFE 0x2000
#define PH_PROCDB_ENTRY_ENFORCE_PRIORITY 0x4000

typedef struct _PH_PROCDB_ENTRY
{
    LIST_ENTRY ListEntry;
    PH_AVL_LINKS Links;
    PPH_STRING FileName;
    UCHAR Hash[PH_PROCDB_HASH_SIZE];

    LONG RefCount;
    ULONG Flags;
    ULONG DesiredPriorityWin32;
} PH_PROCDB_ENTRY, *PPH_PROCDB_ENTRY;

VOID PhProcDbInitialization();

VOID PhReferenceProcDbEntry(
    __inout PPH_PROCDB_ENTRY Entry
    );

VOID PhDereferenceProcDbEntry(
    __inout PPH_PROCDB_ENTRY Entry
    );

PPH_PROCDB_ENTRY PhAddProcDbEntry(
    __in PPH_STRING FileName,
    __in BOOLEAN SaveHash,
    __in BOOLEAN LookupByHash
    );

VOID PhRemoveProcDbEntry(
    __in PPH_PROCDB_ENTRY Entry
    );

PPH_PROCDB_ENTRY PhLookupProcDbEntry(
    __in PPH_STRING FileName
    );

NTSTATUS PhLoadProcDb(
    __in PWSTR FileName
    );

NTSTATUS PhSaveProcDb(
    __in PWSTR FileName
    );

VOID PhToggleSafeProcess(
    __in PPH_PROCESS_ITEM ProcessItem
    );

#endif
