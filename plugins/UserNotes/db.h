/*
 * Process Hacker User Notes -
 *   database functions
 *
 * Copyright (C) 2011-2015 wj32
 * Copyright (C) 2016 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DB_H
#define DB_H

#define PLUGIN_NAME L"ProcessHacker.UserNotes"
#define SETTING_NAME_DATABASE_PATH (PLUGIN_NAME L".DatabasePath")
#define SETTING_NAME_CUSTOM_COLOR_LIST (PLUGIN_NAME L".ColorCustomList")

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
    ULONG AffinityMask;
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
