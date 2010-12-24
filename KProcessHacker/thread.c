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

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KpiOpenThread)
#pragma alloc_text(PAGE, KpiOpenThreadProcess)
#pragma alloc_text(PAGE, KpiTerminateThread)
#pragma alloc_text(PAGE, KpiTerminateThreadUnsafe)
#endif

NTSTATUS KpiOpenThread(
    __out PHANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __in PCLIENT_ID ClientId,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    CLIENT_ID clientId;
    PETHREAD thread;
    HANDLE threadHandle;

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(ThreadHandle, sizeof(HANDLE), sizeof(HANDLE));
            ProbeForRead(ClientId, sizeof(CLIENT_ID), sizeof(ULONG));
            clientId = *ClientId;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }
    else
    {
        clientId = *ClientId;
    }

    // Use the process ID if it was specified.
    if (clientId.UniqueProcess)
    {
        status = PsLookupProcessThreadByCid(&clientId, NULL, &thread);
    }
    else
    {
        status = PsLookupThreadByThreadId(clientId.UniqueThread, &thread);
    }

    if (!NT_SUCCESS(status))
        return status;

    // Always open in KernelMode to skip access checks.
    status = ObOpenObjectByPointer(
        thread,
        0,
        NULL,
        DesiredAccess,
        *PsThreadType,
        KernelMode,
        &threadHandle
        );
    ObDereferenceObject(thread);

    if (NT_SUCCESS(status))
    {
        if (AccessMode != KernelMode)
        {
            __try
            {
                *ThreadHandle = threadHandle;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
            }
        }
        else
        {
            *ThreadHandle = threadHandle;
        }
    }

    return status;
}

NTSTATUS KpiOpenThreadProcess(
    __in HANDLE ThreadHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE ProcessHandle,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PETHREAD thread;
    PEPROCESS process;
    HANDLE processHandle;

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(ProcessHandle, sizeof(HANDLE), sizeof(HANDLE));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    status = ObReferenceObjectByHandle(
        ThreadHandle,
        0,
        *PsThreadType,
        AccessMode,
        &thread,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    process = IoThreadToProcess(thread);

    status = ObOpenObjectByPointer(
        process,
        0,
        NULL,
        DesiredAccess,
        *PsProcessType,
        KernelMode,
        &processHandle
        );

    ObDereferenceObject(thread);

    if (NT_SUCCESS(status))
    {
        if (AccessMode != KernelMode)
        {
            __try
            {
                *ProcessHandle = processHandle;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
            }
        }
        else
        {
            *ProcessHandle = processHandle;
        }
    }

    return status;
}

NTSTATUS KpiTerminateThread(
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus,
    __in KPROCESSOR_MODE AccessMode
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KpiTerminateThreadUnsafe(
    __in HANDLE ThreadHandle,
    __in NTSTATUS ExitStatus,
    __in KPROCESSOR_MODE AccessMode
    )
{
    return STATUS_NOT_IMPLEMENTED;
}
