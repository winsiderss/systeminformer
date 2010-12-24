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
#pragma alloc_text(PAGE, KpiOpenProcess)
#pragma alloc_text(PAGE, KpiOpenProcessToken)
#pragma alloc_text(PAGE, KpiOpenProcessJob)
#pragma alloc_text(PAGE, KpiSuspendProcess)
#pragma alloc_text(PAGE, KpiResumeProcess)
#pragma alloc_text(PAGE, KpiTerminateProcess)
#endif

NTSTATUS KpiOpenProcess(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in PCLIENT_ID ClientId,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    CLIENT_ID clientId;
    PEPROCESS process;
    HANDLE processHandle;

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(ProcessHandle, sizeof(HANDLE), sizeof(HANDLE));
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

    status = PsLookupProcessByProcessId(clientId.UniqueProcess, &process);

    if (!NT_SUCCESS(status))
        return status;

    // Always open in KernelMode to skip access checks.
    status = ObOpenObjectByPointer(
        process,
        0,
        NULL,
        DesiredAccess,
        *PsProcessType,
        KernelMode,
        &processHandle
        );
    ObDereferenceObject(process);

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

NTSTATUS KpiOpenProcessToken(
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE TokenHandle,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;
    PACCESS_TOKEN primaryToken;
    HANDLE tokenHandle;

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(TokenHandle, sizeof(HANDLE), sizeof(HANDLE));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    status = ObReferenceObjectByHandle(
        ProcessHandle,
        0,
        *PsProcessType,
        AccessMode,
        &process,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    primaryToken = PsReferencePrimaryToken(process);
    ObDereferenceObject(process);

    status = ObOpenObjectByPointer(
        primaryToken,
        0,
        NULL,
        DesiredAccess,
        *SeTokenObjectType,
        KernelMode,
        &tokenHandle
        );
    PsDereferencePrimaryToken(primaryToken);

    if (NT_SUCCESS(status))
    {
        if (AccessMode != KernelMode)
        {
            __try
            {
                *TokenHandle = tokenHandle;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
            }
        }
        else
        {
            *TokenHandle = tokenHandle;
        }
    }

    return status;
}

NTSTATUS KpiOpenProcessJob(
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE JobHandle,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;
    PEJOB job;
    HANDLE jobHandle;

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(JobHandle, sizeof(HANDLE), sizeof(HANDLE));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    status = ObReferenceObjectByHandle(
        ProcessHandle,
        0,
        *PsProcessType,
        AccessMode,
        &process,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    job = PsGetProcessJob(process);

    if (job)
    {
        status = ObOpenObjectByPointer(
            job,
            0,
            NULL,
            DesiredAccess,
            *PsJobType,
            KernelMode,
            &jobHandle
            );
    }
    else
    {
        status = STATUS_NOT_FOUND;
    }

    ObDereferenceObject(process);

    if (NT_SUCCESS(status))
    {
        if (AccessMode != KernelMode)
        {
            __try
            {
                *JobHandle = jobHandle;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
            }
        }
        else
        {
            *JobHandle = jobHandle;
        }
    }

    return status;
}

NTSTATUS KpiSuspendProcess(
    __in HANDLE ProcessHandle,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;

    if (!PsSuspendProcess_I)
        return STATUS_NOT_SUPPORTED;

    status = ObReferenceObjectByHandle(
        ProcessHandle,
        PROCESS_SUSPEND_RESUME,
        *PsProcessType,
        AccessMode,
        &process,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PsSuspendProcess_I(process);
    ObDereferenceObject(process);

    return status;
}

NTSTATUS KpiResumeProcess(
    __in HANDLE ProcessHandle,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;

    if (!PsResumeProcess_I)
        return STATUS_NOT_SUPPORTED;

    status = ObReferenceObjectByHandle(
        ProcessHandle,
        PROCESS_SUSPEND_RESUME,
        *PsProcessType,
        AccessMode,
        &process,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    status = PsResumeProcess_I(process);
    ObDereferenceObject(process);

    return status;
}

NTSTATUS KpiTerminateProcess(
    __in HANDLE ProcessHandle,
    __in NTSTATUS ExitStatus,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;

    status = ObReferenceObjectByHandle(
        ProcessHandle,
        PROCESS_TERMINATE,
        *PsProcessType,
        AccessMode,
        &process,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    if (process != PsGetCurrentProcess())
    {
        if (FALSE)
        {
            // TODO: Use PsTerminateProcess.
        }
        else
        {
            HANDLE newProcessHandle;

            // Re-open the process to get a kernel handle.
            if (NT_SUCCESS(status = ObOpenObjectByPointer(
                process,
                OBJ_KERNEL_HANDLE,
                NULL,
                PROCESS_TERMINATE,
                *PsProcessType,
                KernelMode,
                &newProcessHandle
                )))
            {
                status = ZwTerminateProcess(newProcessHandle, ExitStatus);
                ZwClose(newProcessHandle);
            }
        }
    }
    else
    {
        status = STATUS_CANT_TERMINATE_SELF;
    }

    ObDereferenceObject(process);

    return status;
}

NTSTATUS KpiQueryInformationProcess(
    __in HANDLE ProcessHandle,
    __in KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    __out_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS KpiSetInformationProcess(
    __in HANDLE ProcessHandle,
    __in KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    __in_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __in KPROCESSOR_MODE AccessMode
    )
{
    return STATUS_NOT_IMPLEMENTED;
}
