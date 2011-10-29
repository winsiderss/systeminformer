#ifndef DB_H
#define DB_H

#define FILE_TAG 1
#define SERVICE_TAG 2

typedef struct _DB_OBJECT
{
    ULONG Tag;
    PH_STRINGREF Key;

    PPH_STRING Name;
    PPH_STRING Comment;
} DB_OBJECT, *PDB_OBJECT;

VOID InitializeDb(
    VOID
    );

VOID LockDb(
    VOID
    );

VOID UnlockDb(
    VOID
    );

PDB_OBJECT FindDbObject(
    __in ULONG Tag,
    __in PPH_STRINGREF Name
    );

PDB_OBJECT CreateDbObject(
    __in ULONG Tag,
    __in PPH_STRINGREF Name,
    __in PPH_STRING Comment
    );

VOID DeleteDbObject(
    __in PDB_OBJECT Object
    );

VOID SetDbPath(
    __in PPH_STRING Path
    );

NTSTATUS LoadDb(
    VOID
    );

NTSTATUS SaveDb(
    VOID
    );

#endif
