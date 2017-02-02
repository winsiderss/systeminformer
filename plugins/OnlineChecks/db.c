/*
 * Process Hacker Online Checks -
 *   database functions
 *
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

#include "onlnchk.h"

PPH_HASHTABLE ProcessObjectDb;
PH_QUEUED_LOCK ProcessObjectDbLock = PH_QUEUED_LOCK_INIT;

BOOLEAN NTAPI ProcessObjectDbEqualFunction(
    _In_ PVOID Entry1,
    _In_ PVOID Entry2
    )
{
    PPROCESS_DB_OBJECT object1 = *(PPROCESS_DB_OBJECT *)Entry1;
    PPROCESS_DB_OBJECT object2 = *(PPROCESS_DB_OBJECT *)Entry2;

    return PhEqualStringRef(&object1->FileName, &object2->FileName, TRUE);
}

ULONG NTAPI ProcessObjectDbHashFunction(
    _In_ PVOID Entry
    )
{
    PPROCESS_DB_OBJECT object = *(PPROCESS_DB_OBJECT *)Entry;

    return PhHashStringRef(&object->FileName, TRUE);
}

VOID InitializeProcessDb(
    VOID
    )
{
    ProcessObjectDb = PhCreateHashtable(
        sizeof(PPROCESS_DB_OBJECT),
        ProcessObjectDbEqualFunction,
        ProcessObjectDbHashFunction,
        64
        );
}

VOID CleanupProcessDb(
    VOID
    )
{
    PhDereferenceObject(ProcessObjectDb);
}

ULONG GetNumberOfProcessDbObjects(
    VOID
    )
{
    return ProcessObjectDb->Count;
}

VOID LockProcessDb(
    VOID
    )
{
    PhAcquireQueuedLockExclusive(&ProcessObjectDbLock);
}

VOID UnlockProcessDb(
    VOID
    )
{
    PhReleaseQueuedLockExclusive(&ProcessObjectDbLock);
}

PPROCESS_DB_OBJECT FindProcessDbObject(
    _In_ PPH_STRINGREF FileName
    )
{
    PROCESS_DB_OBJECT lookupObject;
    PPROCESS_DB_OBJECT lookupObjectPtr;
    PPROCESS_DB_OBJECT *objectPtr;

    lookupObject.FileName = *FileName;
    lookupObjectPtr = &lookupObject;

    objectPtr = PhFindEntryHashtable(ProcessObjectDb, &lookupObjectPtr);

    if (objectPtr)
        return *objectPtr;
    else
        return NULL;
}

PPROCESS_DB_OBJECT CreateProcessDbObject(
    _In_ PPH_STRING FileName,
    _In_opt_ INT64 Positives,
    _In_opt_ PPH_STRING Hash,
    _In_opt_ PPH_STRING Result
    )
{
    PPROCESS_DB_OBJECT object;
    BOOLEAN added;
    PPROCESS_DB_OBJECT *realObject;

    object = PhAllocate(sizeof(PROCESS_DB_OBJECT));
    memset(object, 0, sizeof(PROCESS_DB_OBJECT));

    PhInitializeStringRefLongHint(&object->FileName, FileName->Buffer);

    object->Positives = Positives;
    object->Hash = PhDuplicateString(FileName);
    object->Result = PhDuplicateString(Result);

    realObject = PhAddEntryHashtableEx(ProcessObjectDb, &object, &added);

    if (added)   
        return object;
    else
        return NULL;
}