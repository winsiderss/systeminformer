#ifndef DB_H
#define DB_H

#define FILE_TAG 1
#define SERVICE_TAG 2
#define COMMAND_LINE_TAG 3

typedef struct _DB_OBJECT
{
    ULONG Tag;
    PH_STRINGREF Key;

    PPH_STRING Name;
    PPH_STRING Comment;
    ULONG PriorityClass;
    ULONG IoPriorityPlusOne;
} DB_OBJECT, *PDB_OBJECT;

VOID InitializeDb(
    VOID
    );

ULONG GetNumberOfDbObjects(
    VOID
    );

VOID LockDb(
    VOID
    );

VOID UnlockDb(
    VOID
    );

PDB_OBJECT FindDbObject(
    _In_ ULONG Tag,
    _In_ PPH_STRINGREF Name
    );

PDB_OBJECT CreateDbObject(
    _In_ ULONG Tag,
    _In_ PPH_STRINGREF Name,
    _In_opt_ PPH_STRING Comment
    );

VOID DeleteDbObject(
    _In_ PDB_OBJECT Object
    );

VOID SetDbPath(
    _In_ PPH_STRING Path
    );

NTSTATUS LoadDb(
    VOID
    );

NTSTATUS SaveDb(
    VOID
    );

#endif
