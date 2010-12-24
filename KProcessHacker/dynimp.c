/*
 * KProcessHacker
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

#include <kph.h>

PVOID KphpGetSystemRoutineAddress(
    __in PWSTR SystemRoutineName
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KphpGetSystemRoutineAddress)
#pragma alloc_text(PAGE, KphDynamicImport)
#endif

_PsSuspendProcess PsSuspendProcess_I;
_PsResumeProcess PsResumeProcess_I;

PVOID KphpGetSystemRoutineAddress(
    __in PWSTR SystemRoutineName
    )
{
    UNICODE_STRING systemRoutineName;

    RtlInitUnicodeString(&systemRoutineName, SystemRoutineName);

    return MmGetSystemRoutineAddress(&systemRoutineName);
}

VOID KphDynamicImport(
    VOID
    )
{
    PsSuspendProcess_I = KphpGetSystemRoutineAddress(L"PsSuspendProcess");
    PsResumeProcess_I = KphpGetSystemRoutineAddress(L"PsResumeProcess");
}
