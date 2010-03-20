/*
 * Process Hacker - 
 *   handle table
 * 
 * Copyright (C) 2009-2010 wj32
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

#include <phbase.h>

PPH_FREE_LIST PhHandleTableLevelFreeList;

VOID PhHandleTableInitialization()
{
    PhHandleTableLevelFreeList = PhCreateFreeList(
        sizeof(PH_HANDLE_TABLE_ENTRY) * PH_HANDLE_TABLE_LEVEL_ENTRIES,
        32
        );
}

VOID PhInitializeHandleTable(
    __out PPH_HANDLE_TABLE HandleTable
    )
{
    HandleTable->Count = 0;
    HandleTable->Entries = PhAllocateFromFreeList(PhHandleTableLevelFreeList);

    PhInitializeQueuedLock(&HandleTable->LockedCondition);
    HandleTable->FreeValue = -1;
    HandleTable->NextValue = -1;
}

VOID PhLockHandleTableEntry(
    __inout PPH_HANDLE_TABLE HandleTable,
    __inout PPH_HANDLE_TABLE_ENTRY HandleTableEntry
    )
{
    if (_interlockedbittestandset(
        (PLONG)&HandleTableEntry->Value,
        PH_HANDLE_TABLE_ENTRY_LOCKED_SHIFT
        ))
    {
        PhWaitForCondition(&HandleTable->LockedCondition, NULL, NULL);
    }
}

VOID PhUnlockHandleTableEntry(
    __inout PPH_HANDLE_TABLE HandleTable,
    __inout PPH_HANDLE_TABLE_ENTRY HandleTableEntry
    )
{
    _interlockedbittestandreset(
        (PLONG)&HandleTableEntry->Value,
        PH_HANDLE_TABLE_ENTRY_LOCKED_SHIFT
        );
    PhPulseAllCondition(&HandleTable->LockedCondition);
}
