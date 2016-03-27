/*
 * KProcessHacker
 *
 * Copyright (C) 2010-2016 wj32
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
#include <dyndata.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KpiOpenProcess)
#pragma alloc_text(PAGE, KpiOpenProcessToken)
#pragma alloc_text(PAGE, KpiOpenProcessJob)
#pragma alloc_text(PAGE, KpiTerminateProcess)
#pragma alloc_text(PAGE, KpiQueryInformationProcess)
#pragma alloc_text(PAGE, KpiSetInformationProcess)
#endif

/**
 * Opens a process.
 *
 * \param ProcessHandle A variable which receives the process handle.
 * \param DesiredAccess The desired access to the process.
 * \param ClientId The identifier of a process or thread. If \a UniqueThread is present, the process
 * of the identified thread will be opened. If \a UniqueProcess is present, the identified process
 * will be opened.
 * \param Key An access key.
 * \li If a L2 key is provided, no access checks are performed.
 * \li If a L1 key is provided, only read access is permitted but no additional access checks are
 * performed.
 * \li If no valid key is provided, the function fails.
 * \param Client The client that initiated the request.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiOpenProcess(
    __out PHANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __in PCLIENT_ID ClientId,
    __in_opt KPH_KEY Key,
    __in PKPH_CLIENT Client,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    CLIENT_ID clientId;
    PEPROCESS process;
    PETHREAD thread;
    KPH_KEY_LEVEL requiredKeyLevel;
    HANDLE processHandle;

    PAGED_CODE();

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

    // Use the thread ID if it was specified.
    if (clientId.UniqueThread)
    {
        status = PsLookupProcessThreadByCid(&clientId, &process, &thread);

        if (NT_SUCCESS(status))
        {
            // We don't actually need the thread.
            ObDereferenceObject(thread);
        }
    }
    else
    {
        status = PsLookupProcessByProcessId(clientId.UniqueProcess, &process);
    }

    if (!NT_SUCCESS(status))
        return status;

    requiredKeyLevel = KphKeyLevel1;

    if ((DesiredAccess & KPH_PROCESS_READ_ACCESS) != DesiredAccess)
        requiredKeyLevel = KphKeyLevel2;

    if (NT_SUCCESS(status = KphValidateKey(requiredKeyLevel, Key, Client, AccessMode)))
    {
        // Always open in KernelMode to skip ordinary access checks.
        status = ObOpenObjectByPointer(
            process,
            0,
            NULL,
            DesiredAccess,
            *PsProcessType,
            KernelMode,
            &processHandle
            );
    }

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

/**
 * Opens the token of a process.
 *
 * \param ProcessHandle A handle to a process.
 * \param DesiredAccess The desired access to the token.
 * \param TokenHandle A variable which receives the token handle.
 * \param Key An access key.
 * \li If a L2 key is provided, no access checks are performed.
 * \li If a L1 key is provided, only read access is permitted but no additional access checks are
 * performed.
 * \li If no valid key is provided, the function fails.
 * \param Client The client that initiated the request.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiOpenProcessToken(
    __in HANDLE ProcessHandle,
    __in ACCESS_MASK DesiredAccess,
    __out PHANDLE TokenHandle,
    __in_opt KPH_KEY Key,
    __in PKPH_CLIENT Client,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;
    PACCESS_TOKEN primaryToken;
    KPH_KEY_LEVEL requiredKeyLevel;
    HANDLE tokenHandle;

    PAGED_CODE();

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

    if (primaryToken = PsReferencePrimaryToken(process))
    {
        requiredKeyLevel = KphKeyLevel1;

        if ((DesiredAccess & KPH_TOKEN_READ_ACCESS) != DesiredAccess)
            requiredKeyLevel = KphKeyLevel2;

        if (NT_SUCCESS(status = KphValidateKey(requiredKeyLevel, Key, Client, AccessMode)))
        {
            status = ObOpenObjectByPointer(
                primaryToken,
                0,
                NULL,
                DesiredAccess,
                *SeTokenObjectType,
                KernelMode,
                &tokenHandle
                );
        }

        PsDereferencePrimaryToken(primaryToken);
    }
    else
    {
        status = STATUS_NO_TOKEN;
    }

    ObDereferenceObject(process);

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

/**
 * Opens the job object of a process.
 *
 * \param ProcessHandle A handle to a process.
 * \param DesiredAccess The desired access to the job.
 * \param JobHandle A variable which receives the job object handle.
 * \param AccessMode The mode in which to perform access checks.
 */
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
    HANDLE jobHandle = NULL;

    PAGED_CODE();

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
            AccessMode,
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

/**
 * Terminates a process.
 *
 * \param ProcessHandle A handle to a process.
 * \param ExitStatus A status value which indicates why the process is being terminated.
 * \param Key An access key.
 * \li If a L2 key is provided, no access checks are performed.
 * \li If no valid L2 key is provided, the function fails.
 * \param Client The client that initiated the request.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiTerminateProcess(
    __in HANDLE ProcessHandle,
    __in NTSTATUS ExitStatus,
    __in_opt KPH_KEY Key,
    __in PKPH_CLIENT Client,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;

    PAGED_CODE();

    if (!NT_SUCCESS(status = KphValidateKey(KphKeyLevel2, Key, Client, AccessMode)))
        return status;

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

    if (process != PsGetCurrentProcess())
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
    else
    {
        status = STATUS_CANT_TERMINATE_SELF;
    }

    ObDereferenceObject(process);

    return status;
}

/**
 * Queries process information.
 *
 * \param ProcessHandle A handle to a process.
 * \param ProcessInformationClass The type of information to query.
 * \param ProcessInformation The buffer in which the information will be stored.
 * \param ProcessInformationLength The number of bytes available in \a ProcessInformation.
 * \param ReturnLength A variable which receives the number of bytes required to be available in
 * \a ProcessInformation.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiQueryInformationProcess(
    __in HANDLE ProcessHandle,
    __in KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    __out_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __out_opt PULONG ReturnLength,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;
    ULONG returnLength;

    PAGED_CODE();

    if (AccessMode != KernelMode)
    {
        ULONG alignment;

        switch (ProcessInformationClass)
        {
        default:
            alignment = sizeof(ULONG);
            break;
        }

        __try
        {
            ProbeForWrite(ProcessInformation, ProcessInformationLength, alignment);

            if (ReturnLength)
                ProbeForWrite(ReturnLength, sizeof(ULONG), sizeof(ULONG));
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    status = ObReferenceObjectByHandle(
        ProcessHandle,
        PROCESS_QUERY_INFORMATION,
        *PsProcessType,
        AccessMode,
        &process,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    switch (ProcessInformationClass)
    {
    default:
        status = STATUS_INVALID_INFO_CLASS;
        returnLength = 0;
        break;
    }

    ObDereferenceObject(process);

    if (ReturnLength)
    {
        if (AccessMode != KernelMode)
        {
            __try
            {
                *ReturnLength = returnLength;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                NOTHING;
            }
        }
        else
        {
            *ReturnLength = returnLength;
        }
    }

    return status;
}

/**
 * Sets process information.
 *
 * \param ProcessHandle A handle to a process.
 * \param ProcessInformationClass The type of information to set.
 * \param ProcessInformation A buffer which contains the information to set.
 * \param ProcessInformationLength The number of bytes present in \a ProcessInformation.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiSetInformationProcess(
    __in HANDLE ProcessHandle,
    __in KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    __in_bcount(ProcessInformationLength) PVOID ProcessInformation,
    __in ULONG ProcessInformationLength,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;

    PAGED_CODE();

    if (AccessMode != KernelMode)
    {
        ULONG alignment;

        switch (ProcessInformationClass)
        {
        default:
            alignment = sizeof(ULONG);
            break;
        }

        __try
        {
            ProbeForRead(ProcessInformation, ProcessInformationLength, alignment);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    status = ObReferenceObjectByHandle(
        ProcessHandle,
        PROCESS_SET_INFORMATION,
        *PsProcessType,
        AccessMode,
        &process,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    switch (ProcessInformationClass)
    {
    default:
        status = STATUS_INVALID_INFO_CLASS;
        break;
    }

    ObDereferenceObject(process);

    return status;
}
