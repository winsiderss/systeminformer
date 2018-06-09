/*
 * Process Hacker Online Checks -
 *   database functions
 *
 * Copyright (C) 2016-2017 dmex
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

extern PH_STRINGREF ProcessObjectDbHash;
extern PH_STRINGREF ServiceObjectDbHash;

typedef struct _PROCESS_DB_OBJECT
{
    PH_STRINGREF FileName;

    INT64 Positives;
    PPH_STRING Hash;
    PPH_STRING Result;
} PROCESS_DB_OBJECT, *PPROCESS_DB_OBJECT;

VOID InitializeProcessDb(
    VOID
    );

VOID CleanupProcessDb(
    VOID
    );

VOID LockProcessDb(
    VOID
    );

VOID UnlockProcessDb(
    VOID
    );

PPROCESS_DB_OBJECT FindProcessDbObject(
    _In_ PPH_STRINGREF Hash
    );

PPROCESS_DB_OBJECT CreateProcessDbObject(
    _In_ PPH_STRING FileName,
    _In_opt_ INT64 Positives,
    _In_opt_ INT64 Total,
    _In_opt_ PPH_STRING Hash
    );

#endif
