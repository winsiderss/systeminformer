/*
 * Process Hacker - 
 *   object type security information
 * 
 * Copyright (C) 2010 wj32
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

#include <phgui.h>

PH_ACCESS_ENTRY PhStandardAccessEntries[] =
{
    { L"Synchronize", SYNCHRONIZE, FALSE, TRUE },
    { L"Delete", DELETE, FALSE, TRUE },
    { L"Read permissions", READ_CONTROL, FALSE, TRUE },
    { L"Change permissions", WRITE_DAC, FALSE, TRUE },
    { L"Take ownership", WRITE_OWNER, FALSE, TRUE }
};

BOOLEAN PhGetAccessEntries(
    __in PWSTR Type,
    __out PPH_ACCESS_ENTRY *AccessEntries,
    __out PULONG NumberOfAccessEntries
    )
{
    PPH_ACCESS_ENTRY accessEntries;

    accessEntries = PhAllocate(sizeof(PhStandardAccessEntries));
    memcpy(accessEntries, PhStandardAccessEntries, sizeof(PhStandardAccessEntries));

    *AccessEntries = accessEntries;
    *NumberOfAccessEntries = sizeof(PhStandardAccessEntries) / sizeof(PH_ACCESS_ENTRY);

    return TRUE;
}
