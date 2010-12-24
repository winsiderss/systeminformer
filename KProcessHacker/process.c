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
#pragma alloc_text(PAGE, KpiQueryInformationProcess)
#pragma alloc_text(PAGE, KpiSetInformationProcess)
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
    PETHREAD thread;
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
    NTSTATUS status;
    PEPROCESS process;
    HANDLE newProcessHandle;
    ULONG returnLength;

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForWrite(ProcessInformation, ProcessInformationLength, sizeof(ULONG));

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
        0,
        *PsProcessType,
        AccessMode,
        &process,
        NULL
        );

    if (!NT_SUCCESS(status))
        return status;

    switch (ProcessInformationClass)
    {
    case KphProcessProtectionInformation:
        {
            status = STATUS_NOT_IMPLEMENTED;
            returnLength = 0;
        }
        break;
    case KphProcessIoPriority:
        {
            if (NT_SUCCESS(status = ObOpenObjectByPointer(
                process,
                OBJ_KERNEL_HANDLE,
                NULL,
                PROCESS_QUERY_INFORMATION,
                *PsProcessType,
                KernelMode,
                &newProcessHandle
                )))
            {
                ULONG ioPriority;

                if (NT_SUCCESS(status = ZwQueryInformationProcess(
                    newProcessHandle,
                    ProcessIoPriority,
                    &ioPriority,
                    sizeof(ULONG),
                    NULL
                    )))
                {
                    if (ProcessInformationLength == sizeof(ULONG))
                    {
                        __try
                        {
                            *(PULONG)ProcessInformation = ioPriority;
                        }
                        __except (EXCEPTION_EXECUTE_HANDLER)
                        {
                            status = GetExceptionCode();
                        }
                    }
                    else
                    {
                        status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                }

                ZwClose(newProcessHandle);
            }

            returnLength = sizeof(ULONG);
        }
        break;
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
    HANDLE newProcessHandle;

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeForRead(ProcessInformation, ProcessInformationLength, sizeof(ULONG));
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

    switch (ProcessInformationClass)
    {
    case KphProcessExecuteFlags:
        {
            ULONG executeFlags;
            KAPC_STATE apcState;

            if (ProcessInformationLength == sizeof(ULONG))
            {
                __try
                {
                    executeFlags = *(PULONG)ProcessInformation;
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    status = GetExceptionCode();
                }
            }
            else
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
            }

            if (NT_SUCCESS(status))
            {
                // We can only set execute options on the current process.
                // So, we simply attach to the target process.
                KeStackAttachProcess(process, &apcState);
                status = ZwSetInformationProcess(
                    NtCurrentProcess(),
                    ProcessExecuteFlags,
                    &executeFlags,
                    sizeof(ULONG)
                    );
                KeUnstackDetachProcess(&apcState);
            }
        }
        break;
    case KphProcessIoPriority:
        {
            ULONG ioPriority;

            if (ProcessInformationLength == sizeof(ULONG))
            {
                __try
                {
                    ioPriority = *(PULONG)ProcessInformation;
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    status = GetExceptionCode();
                }
            }
            else
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
            }

            if (NT_SUCCESS(status))
            {
                if (NT_SUCCESS(status = ObOpenObjectByPointer(
                    process,
                    OBJ_KERNEL_HANDLE,
                    NULL,
                    PROCESS_SET_INFORMATION,
                    *PsProcessType,
                    KernelMode,
                    &newProcessHandle
                    )))
                {
                    status = ZwSetInformationProcess(
                        newProcessHandle,
                        ProcessIoPriority,
                        &ioPriority,
                        sizeof(ULONG)
                        );
                    ZwClose(newProcessHandle);
                }
            }
        }
        break;
    default:
        status = STATUS_INVALID_INFO_CLASS;
        break;
    }

    ObDereferenceObject(process);

    return status;
}
