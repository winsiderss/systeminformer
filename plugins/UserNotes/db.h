/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2015
 *     dmex    2016-2023
 *
 */

#ifndef DB_H
#define DB_H

#define PLUGIN_NAME L"ProcessHacker.UserNotes"
#define SETTING_NAME_CUSTOM_COLOR_LIST (PLUGIN_NAME L".ColorCustomList")
#define SETTING_NAME_OPTIONS_DB_COLUMNS (PLUGIN_NAME L".DbListColumns")

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
    COLORREF BackColor;
    BOOLEAN Collapse;
    KAFFINITY AffinityMask;
    ULONG PagePriorityPlusOne;
    BOOLEAN Boost;
    BOOLEAN Efficiency;
} DB_OBJECT, *PDB_OBJECT;

VOID InitializeDb(
    VOID
    );

ULONG GetNumberOfDbObjects(
    VOID
    );

_Acquires_exclusive_lock_(ObjectDbLock)
VOID LockDb(
    VOID
    );

_Releases_exclusive_lock_(ObjectDbLock)
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

typedef BOOLEAN (NTAPI* PDB_ENUM_CALLBACK)(
    _In_ PDB_OBJECT Object,
    _In_ PVOID Context
    );

VOID EnumDb(
    _In_ PDB_ENUM_CALLBACK Callback,
    _In_ PVOID Context
    );

_Success_(return)
BOOLEAN FindIfeoObject(
    _In_ PPH_STRINGREF Name,
    _Out_opt_ PULONG CpuPriorityClass,
    _Out_opt_ PULONG IoPriorityClass,
    _Out_opt_ PULONG PagePriorityClass
    );

NTSTATUS CreateIfeoObject(
    _In_ PPH_STRINGREF Name,
    _In_ ULONG CpuPriority,
    _In_ ULONG IoPriority,
    _In_ ULONG PagePriority
    );

NTSTATUS DeleteIfeoObject(
    _In_ PPH_STRINGREF Name,
    _In_ ULONG CpuPriority,
    _In_ ULONG IoPriority,
    _In_ ULONG PagePriority
    );

#endif
