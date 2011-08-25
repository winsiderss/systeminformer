/*
 * KProcessHacker
 *
 * Copyright (C) 2010-2011 wj32
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

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KphGetSystemRoutineAddress)
#pragma alloc_text(PAGE, KphDynamicImport)
#endif

_ObGetObjectType ObGetObjectType_I;
_PsAcquireProcessExitSynchronization PsAcquireProcessExitSynchronization_I;
_PsIsProtectedProcess PsIsProtectedProcess_I;
_PsReleaseProcessExitSynchronization PsReleaseProcessExitSynchronization_I;
_PsResumeProcess PsResumeProcess_I;
_PsSuspendProcess PsSuspendProcess_I;

/**
 * Dynamically imports routines.
 */
VOID KphDynamicImport(
    VOID
    )
{
    PAGED_CODE();

    ObGetObjectType_I = KphGetSystemRoutineAddress(L"ObGetObjectType");
    PsAcquireProcessExitSynchronization_I = KphGetSystemRoutineAddress(L"PsAcquireProcessExitSynchronization");
    PsIsProtectedProcess_I = KphGetSystemRoutineAddress(L"PsIsProtectedProcess");
    PsReleaseProcessExitSynchronization_I = KphGetSystemRoutineAddress(L"PsReleaseProcessExitSynchronization");
    PsResumeProcess_I = KphGetSystemRoutineAddress(L"PsResumeProcess");
    PsSuspendProcess_I = KphGetSystemRoutineAddress(L"PsSuspendProcess");

    dprintf("ObGetObjectType: 0x%Ix\n", ObGetObjectType_I);
    dprintf("PsAcquireProcessExitSynchronization: 0x%Ix\n", PsAcquireProcessExitSynchronization_I);
    dprintf("PsIsProtectedProcess: 0x%Ix\n", PsIsProtectedProcess_I);
    dprintf("PsReleaseProcessExitSynchronization: 0x%Ix\n", PsReleaseProcessExitSynchronization_I);
    dprintf("PsResumeProcess: 0x%Ix\n", PsResumeProcess_I);
    dprintf("PsSuspendProcess: 0x%Ix\n", PsSuspendProcess_I);
}

/**
 * Retrieves the address of a function exported by NTOS or HAL.
 *
 * \param SystemRoutineName The name of the function.
 *
 * \return The address of the function, or NULL if the function could
 * not be found.
 */
PVOID KphGetSystemRoutineAddress(
    __in PWSTR SystemRoutineName
    )
{
    UNICODE_STRING systemRoutineName;

    PAGED_CODE();

    RtlInitUnicodeString(&systemRoutineName, SystemRoutineName);

    return MmGetSystemRoutineAddress(&systemRoutineName);
}
