/*
 * Process Hacker Driver - 
 *   security
 * 
 * Copyright (C) 2009 wj32
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

#include "include/kph.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KphOpenProcessTokenEx)
#endif

/* KphOpenProcessTokenEx
 * 
 * Opens the primary token of the specified process.
 */
NTSTATUS KphOpenProcessTokenEx(
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in ULONG ObjectAttributes,
    __out PHANDLE TokenHandle,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PEPROCESS processObject;
    PACCESS_TOKEN tokenObject;
    HANDLE tokenHandle;
    ACCESS_STATE accessState;
    CHAR auxData[AUX_ACCESS_DATA_SIZE];
    
    status = SeCreateAccessState(
        &accessState,
        (PAUX_ACCESS_DATA)auxData,
        DesiredAccess,
        (PGENERIC_MAPPING)KVOFF(*SeTokenObjectType, OffOtiGenericMapping)
        );
    
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    
    if (accessState.RemainingDesiredAccess & MAXIMUM_ALLOWED)
        accessState.PreviouslyGrantedAccess |= TOKEN_ALL_ACCESS;
    else
        accessState.PreviouslyGrantedAccess |= accessState.RemainingDesiredAccess;
    
    accessState.RemainingDesiredAccess = 0;
    
    status = ObReferenceObjectByHandle(
        ProcessHandle,
        0,
        *PsProcessType,
        KernelMode,
        &processObject,
        NULL
        );
    
    if (!NT_SUCCESS(status))
    {
        SeDeleteAccessState(&accessState);
        return status;
    }
    
    tokenObject = PsReferencePrimaryToken(processObject);
    ObDereferenceObject(processObject);
    
    status = ObOpenObjectByPointer(
        tokenObject,
        ObjectAttributes,
        &accessState,
        0,
        *SeTokenObjectType,
        AccessMode,
        &tokenHandle
        );
    SeDeleteAccessState(&accessState);
    ObDereferenceObject(tokenObject);
    
    if (NT_SUCCESS(status))
        *TokenHandle = tokenHandle;
    
    return status;
}
