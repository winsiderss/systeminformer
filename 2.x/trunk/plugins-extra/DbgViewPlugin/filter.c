/*
 * Process Hacker Extra Plugins -
 *   Debug View Plugin
 *
 * Copyright (C) 2015 dmex
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

#include "main.h"

VOID AddFilterType(
    _Inout_ PPH_DBGEVENTS_CONTEXT Context,
    _In_ FILTER_BY_TYPE Type,
    _In_ HANDLE ProcessID,
    _In_ PPH_STRING ProcessName
    )
{
    PDBG_FILTER_TYPE newFilterEntry;

    newFilterEntry = PhAllocate(sizeof(DBG_FILTER_TYPE));
    newFilterEntry->Type = Type;
    newFilterEntry->ProcessId = ProcessID;
    newFilterEntry->ProcessName = ProcessName;

    PhAddItemList(Context->ExcludeList, newFilterEntry);

    // Remove any existing entries...
    for (ULONG i = 0; i < Context->LogMessageList->Count; i++)
    {
        PDEBUG_LOG_ENTRY listEntry = Context->LogMessageList->Items[i];

        if (Type == FilterByName)
        {
            if (PhEqualString(listEntry->ProcessName, newFilterEntry->ProcessName, TRUE))
            {
                PhRemoveItemList(Context->LogMessageList, i);
                i--;
            }
        }
        else if (Type == FilterByPid)
        {
            if (listEntry->ProcessId == ProcessID)
            {
                PhRemoveItemList(Context->LogMessageList, i);
                i--;
            }
        }
    }
}

VOID ResetFilters(
    _Inout_ PPH_DBGEVENTS_CONTEXT Context
    )
{
    for (ULONG i = 0; i < Context->ExcludeList->Count; i++)
    {
        PDBG_FILTER_TYPE filterEntry = Context->ExcludeList->Items[i];

        if (filterEntry->ProcessName)
            PhDereferenceObject(filterEntry->ProcessName);

        PhFree(filterEntry);

        PhRemoveItemList(Context->ExcludeList, i);
        i--;
    }
}