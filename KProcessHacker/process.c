/*
 * KProcessHacker
 *
 * Copyright (C) 2010-2013 wj32
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
#pragma alloc_text(PAGE, KpiSuspendProcess)
#pragma alloc_text(PAGE, KpiResumeProcess)
#pragma alloc_text(PAGE, KphTerminateProcessInternal)
#pragma alloc_text(PAGE, KpiTerminateProcess)
#pragma alloc_text(PAGE, KpiQueryInformationProcess)
#pragma alloc_text(PAGE, KpiSetInformationProcess)
#endif

/**
 * Opens a process.
 *
 * \param ProcessHandle A variable which receives the process handle.
 * \param DesiredAccess The desired access to the process.
 * \param ClientId The identifier of a process or thread. If \a UniqueThread
 * is present, the process of the identified thread will be opened. If
 * \a UniqueProcess is present, the identified process will be opened.
 * \param AccessMode The mode in which to perform access checks.
 */
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

/**
 * Opens the token of a process.
 *
 * \param ProcessHandle A handle to a process.
 * \param DesiredAccess The desired access to the token.
 * \param TokenHandle A variable which receives the token handle.
 * \param AccessMode The mode in which to perform access checks.
 */
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

/**
 * Opens the job object of a process.
 *
 * \param ProcessHandle A handle to a process.
 * \param DesiredAccess The desired access to the token.
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

/**
 * Suspends a process.
 *
 * \param ProcessHandle A handle to a process.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiSuspendProcess(
    __in HANDLE ProcessHandle,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;

    PAGED_CODE();

    if (!PsSuspendProcess_I)
        return STATUS_NOT_SUPPORTED;

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

    status = PsSuspendProcess_I(process);
    ObDereferenceObject(process);

    return status;
}

/**
 * Resumes a process.
 *
 * \param ProcessHandle A handle to a process.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiResumeProcess(
    __in HANDLE ProcessHandle,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;

    PAGED_CODE();

    if (!PsResumeProcess_I)
        return STATUS_NOT_SUPPORTED;

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

    status = PsResumeProcess_I(process);
    ObDereferenceObject(process);

    return status;
}

/**
 * Terminates a process using PsTerminateProcess.
 *
 * \param Process A process object.
 * \param ExitStatus A status value which indicates why the process
 * is being terminated.
 */
NTSTATUS KphTerminateProcessInternal(
    __in PEPROCESS Process,
    __in NTSTATUS ExitStatus
    )
{
    NTSTATUS status;
    _PsTerminateProcess PsTerminateProcess_I;

    PAGED_CODE();

    if (KphParameters.DisableDynamicProcedureScan)
        return STATUS_NOT_SUPPORTED;

    PsTerminateProcess_I = KphGetDynamicProcedureScan(&KphDynPsTerminateProcessScan);

    if (!PsTerminateProcess_I)
    {
        dprintf("Unable to find PsTerminateProcess\n");
        return STATUS_NOT_SUPPORTED;
    }

#ifdef _X86_

    if (
        KphDynNtVersion == PHNT_WINXP ||
        KphDynNtVersion == PHNT_WS03 ||
        KphDynNtVersion == PHNT_WIN8
        )
    {
        dprintf("Calling XP/03/8-style PsTerminateProcess\n");

        // PspTerminateProcess on XP and Server 2003 is normal.
        // PsTerminateProcess on 8 is also normal.
        status = PsTerminateProcess_I(Process, ExitStatus);
    }
    else if (
        KphDynNtVersion == PHNT_VISTA ||
        KphDynNtVersion == PHNT_WIN7
        )
    {
        dprintf("Calling Vista/7-style PsTerminateProcess\n");

        // PsTerminateProcess on Vista and 7 has its first argument
        // in ecx.
        __asm
        {
            push    [ExitStatus]
            mov     ecx, [Process]
            call    [PsTerminateProcess_I]
            mov     [status], eax
        }
    }
    else if (KphDynNtVersion == PHNT_WINBLUE)
    {
        dprintf("Calling 8.1-style PsTerminateProcess\n");

        // PsTerminateProcess on 8.1 is fastcall.
        status = ((_PsTerminateProcess63)PsTerminateProcess_I)(Process, ExitStatus);
    }
    else
    {
        return STATUS_NOT_SUPPORTED;
    }

#else

    status = PsTerminateProcess_I(Process, ExitStatus);

#endif

    return status;
}

/**
 * Terminates a process using PsTerminateProcess.
 *
 * \param ProcessHandle A handle to a process.
 * \param ExitStatus A status value which indicates why the process
 * is being terminated.
 * \param AccessMode The mode in which to perform access checks.
 */
NTSTATUS KpiTerminateProcess(
    __in HANDLE ProcessHandle,
    __in NTSTATUS ExitStatus,
    __in KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;

    PAGED_CODE();

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
        dprintf("Calling KphTerminateProcessInternal from KpiTerminateProcess\n");
        status = KphTerminateProcessInternal(process, ExitStatus);

        if (status == STATUS_NOT_SUPPORTED)
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

/**
 * Queries process information.
 *
 * \param ProcessHandle A handle to a process.
 * \param ProcessInformationClass The type of information to query.
 * \param ProcessInformation The buffer in which the information will be stored.
 * \param ProcessInformationLength The number of bytes available in
 * \a ProcessInformation.
 * \param ReturnLength A variable which receives the number of bytes
 * required to be available in \a ProcessInformation.
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
        case KphProcessProtectionInformation:
            alignment = sizeof(KPH_PROCESS_PROTECTION_INFORMATION);
            break;
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
            BOOLEAN protectedProcess = FALSE;

            if (PsIsProtectedProcess_I)
                protectedProcess = PsIsProtectedProcess_I(process);
            else
                status = STATUS_NOT_SUPPORTED;

            if (NT_SUCCESS(status))
            {
                if (ProcessInformationLength == sizeof(KPH_PROCESS_PROTECTION_INFORMATION))
                {
                    __try
                    {
                        ((PKPH_PROCESS_PROTECTION_INFORMATION)ProcessInformation)->IsProtectedProcess = protectedProcess;
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

            returnLength = sizeof(KPH_PROCESS_PROTECTION_INFORMATION);
        }
        break;
    case KphProcessExecuteFlags:
        {
            KAPC_STATE apcState;
            ULONG executeFlags;

            KeStackAttachProcess(process, &apcState);
            status = ZwQueryInformationProcess(
                NtCurrentProcess(),
                ProcessExecuteFlags,
                &executeFlags,
                sizeof(ULONG),
                NULL
                );
            KeUnstackDetachProcess(&apcState);

            if (NT_SUCCESS(status))
            {
                if (ProcessInformationLength == sizeof(ULONG))
                {
                    __try
                    {
                        *(PULONG)ProcessInformation = executeFlags;
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

            returnLength = sizeof(ULONG);
        }
        break;
    case KphProcessIoPriority:
        {
            HANDLE newProcessHandle;
            ULONG ioPriority;

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

/**
 * Sets process information.
 *
 * \param ProcessHandle A handle to a process.
 * \param ProcessInformationClass The type of information to set.
 * \param ProcessInformation A buffer which contains the information to set.
 * \param ProcessInformationLength The number of bytes present in
 * \a ProcessInformation.
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
                // Make sure the process isn't terminating, otherwise the call
                // may hang due to a recursive acquire of the working set mutex.
                if (KphAcquireProcessRundownProtection(process))
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

                    KphReleaseProcessRundownProtection(process);
                }
                else
                {
                    status = STATUS_PROCESS_IS_TERMINATING;
                }
            }
        }
        break;
    case KphProcessIoPriority:
        {
            ULONG ioPriority;
            HANDLE newProcessHandle;

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

/**
 * Prevents a process from terminating.
 *
 * \param Process A process object.
 *
 * \return TRUE if the function succeeded, FALSE if the process is
 * currently terminating or the request is not supported.
 */
BOOLEAN KphAcquireProcessRundownProtection(
    __in PEPROCESS Process
    )
{
    // Use the exported function if it is available.
    // Note that we make sure the corresponding release function is also available.
    if (PsAcquireProcessExitSynchronization_I && PsReleaseProcessExitSynchronization_I)
    {
        return NT_SUCCESS(PsAcquireProcessExitSynchronization_I(Process));
    }

    // Fail if we don't have an offset.
    if (KphDynEpRundownProtect == -1)
        return FALSE;

    return ExAcquireRundownProtection((PEX_RUNDOWN_REF)((ULONG_PTR)Process + KphDynEpRundownProtect));
}

/**
 * Allows a process to terminate.
 *
 * \param Process A process object.
 */
VOID KphReleaseProcessRundownProtection(
    __in PEPROCESS Process
    )
{
    if (PsReleaseProcessExitSynchronization_I)
    {
        PsReleaseProcessExitSynchronization_I(Process);
        return;
    }

    if (KphDynEpRundownProtect == -1)
        return;

    ExReleaseRundownProtection((PEX_RUNDOWN_REF)((ULONG_PTR)Process + KphDynEpRundownProtect));
}
