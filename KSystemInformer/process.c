/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     jxy-s   2020 
 *
 */

#include <kph.h>
#include <dyndata.h>

#include <trace.h>

PAGED_FILE();

/**
 * \brief Retrieves process protection information.
 *
 * \details This wraps a call to PsGetProcessProtection which is not exported
 * everywhere. If the function is not exported this call fails with
 * STATUS_NOT_IMPLEMENTED.
 * 
 * \param[in] Process A process object to retrieve the information from.
 * \param[out] Protection On success this is populated with the process
 * protection.
 * \return Appropriate status:
 * STATUS_NOT_IMPLEMENTED - The call is not supported.
 * STATUS_SUCCESS - Retrieved the process protection level.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphGetProcessProtection(
    _In_ PEPROCESS Process,
    _Out_ PPS_PROTECTION Protection
    )
{
    PAGED_CODE();

    if (!KphDynPsGetProcessProtection)
    {
        return STATUS_NOT_IMPLEMENTED;
    }

    *Protection = KphDynPsGetProcessProtection(Process);
    return STATUS_SUCCESS;
}

/**
 * \brief Opens a process.
 *
 * \param[out] ProcessHandle A variable which receives the process handle.
 * \param[in] DesiredAccess The desired access to the process.
 * \param[in] ClientId The identifier of a process or thread. If \a
 * UniqueThread
 * is present, the process of the identified thread will be opened. If
 * \a UniqueProcess is present, the identified process will be opened.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphOpenProcess(
    _Out_ PHANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PCLIENT_ID ClientId,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    CLIENT_ID clientId;
    PEPROCESS process;
    HANDLE processHandle;

    PAGED_PASSIVE();

    process = NULL;

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeOutputType(ProcessHandle, HANDLE);
            ProbeForRead(ClientId,
                         sizeof(CLIENT_ID),
                         TYPE_ALIGNMENT(CLIENT_ID));
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

    //
    // Use the thread ID if it was specified.
    //
    if (clientId.UniqueThread)
    {
        PETHREAD thread;

        status = PsLookupProcessThreadByCid(&clientId, &process, &thread);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "PsLookupProcessThreadByCid failed: %!STATUS!",
                          status);

            process = NULL;
            thread = NULL;
            goto Exit;
        }

        ObDereferenceObject(thread);
    }
    else
    {
        status = PsLookupProcessByProcessId(clientId.UniqueProcess, &process);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "PsLookupProcessByProcessId failed: %!STATUS!",
                          status);

            process = NULL;
            goto Exit;
        }
    }

    if ((DesiredAccess & KPH_PROCESS_READ_ACCESS) != DesiredAccess)
    {
        status = KphDominationCheck(PsGetCurrentProcess(),
                                    process,
                                    AccessMode);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "KphDominationCheck failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    //
    // Always open in KernelMode to skip ordinary access checks.
    //
    status = ObOpenObjectByPointer(process,
                                   (AccessMode ? 0 : OBJ_KERNEL_HANDLE),
                                   NULL,
                                   DesiredAccess,
                                   *PsProcessType,
                                   KernelMode,
                                   &processHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "ObOpenObjectByPointer failed: %!STATUS!",
                      status);

        processHandle = NULL;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        __try
        {
            *ProcessHandle = processHandle;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ObCloseHandle(processHandle, UserMode);
            status = GetExceptionCode();
        }
    }
    else
    {
        *ProcessHandle = processHandle;
    }

Exit:

    if (process)
    {
        ObDereferenceObject(process);
    }

    return status;
}

/**
 * \brief Opens the token of a process.
 *
 * \param[in] ProcessHandle A handle to a process.
 * \param[in] DesiredAccess The desired access to the token.
 * \param[out] TokenHandle A variable which receives the token handle.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphOpenProcessToken(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE TokenHandle,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;
    PACCESS_TOKEN primaryToken;
    HANDLE tokenHandle;

    PAGED_PASSIVE();

    process = NULL;
    primaryToken = NULL;

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeOutputType(TokenHandle, HANDLE);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    status = ObReferenceObjectByHandle(ProcessHandle,
                                       0,
                                       *PsProcessType,
                                       AccessMode,
                                       &process,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        process = NULL;
        goto Exit;
    }

    primaryToken = PsReferencePrimaryToken(process);
    if (!primaryToken)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "PsReferencePrimaryToken returned null");

        status = STATUS_NO_TOKEN;
        goto Exit;
    }

    if ((DesiredAccess & KPH_TOKEN_READ_ACCESS) != DesiredAccess)
    {
        status = KphDominationCheck(PsGetCurrentProcess(),
                                    process,
                                    AccessMode);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "KphDominationCheck failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    //
    // Always open in KernelMode to skip ordinary access checks.
    //
    status = ObOpenObjectByPointer(primaryToken,
                                   (AccessMode ? 0 : OBJ_KERNEL_HANDLE),
                                   NULL,
                                   DesiredAccess,
                                   *SeTokenObjectType,
                                   KernelMode,
                                   &tokenHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "ObOpenObjectByPointer failed: %!STATUS!",
                      status);

        tokenHandle = NULL;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        __try
        {
            *TokenHandle = tokenHandle;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ObCloseHandle(tokenHandle, UserMode);
            status = GetExceptionCode();
        }
    }
    else
    {
        *TokenHandle = tokenHandle;
    }

Exit:

    if (primaryToken)
    {
        PsDereferencePrimaryToken(primaryToken);
    }

    if (process)
    {
        ObDereferenceObject(process);
    }

    return status;
}

/**
 * \brief Opens the job object of a process.
 *
 * \param[in] ProcessHandle A handle to a process.
 * \param[in] DesiredAccess The desired access to the job.
 * \param[out] JobHandle A variable which receives the job object handle.
 * \param[in] AccessMode The mode in which to perform access checks.
 * 
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphOpenProcessJob(
    _In_ HANDLE ProcessHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _Out_ PHANDLE JobHandle,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;
    PEJOB job;
    HANDLE jobHandle;

    PAGED_PASSIVE();

    process = NULL;

    if (AccessMode != KernelMode)
    {
        __try
        {
            ProbeOutputType(JobHandle, HANDLE);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return GetExceptionCode();
        }
    }

    status = ObReferenceObjectByHandle(ProcessHandle,
                                       0,
                                       *PsProcessType,
                                       AccessMode,
                                       &process,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        process = NULL;
        goto Exit;
    }

    job = PsGetProcessJob(process);
    if (!job)
    {
        status = STATUS_NOT_FOUND;
        goto Exit;
    }

    if ((DesiredAccess & KPH_JOB_READ_ACCESS) != DesiredAccess)
    {
        status = KphDominationCheck(PsGetCurrentProcess(),
                                    process,
                                    AccessMode);
        if (!NT_SUCCESS(status))
        {
            KphTracePrint(TRACE_LEVEL_ERROR,
                          GENERAL,
                          "KphDominationCheck failed: %!STATUS!",
                          status);

            goto Exit;
        }
    }

    //
    // Always open in KernelMode to skip ordinary access checks.
    //
    status = ObOpenObjectByPointer(job,
                                   (AccessMode ? 0 : OBJ_KERNEL_HANDLE),
                                   NULL,
                                   DesiredAccess,
                                   *PsJobType,
                                   KernelMode,
                                   &jobHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "ObOpenObjectByPointer failed: %!STATUS!",
                      status);

        jobHandle = NULL;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        __try
        {
            *JobHandle = jobHandle;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            ObCloseHandle(jobHandle, UserMode);
            status = GetExceptionCode();
        }
    }
    else
    {
        *JobHandle = jobHandle;
    }

Exit:
    if (process)
    {
        ObDereferenceObject(process);
    }

    return status;
}

/**
 * Terminates a process.
 *
 * \param[in] ProcessHandle A handle to a process.
 * \param[in] ExitStatus A status value which indicates why the process is
 * being terminated.
 * \param[in] Key An access key.
 * \param[in] AccessMode The mode in which to perform access checks.
 * 
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphTerminateProcess(
    _In_ HANDLE ProcessHandle,
    _In_ NTSTATUS ExitStatus,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS process;
    HANDLE processHandle;

    PAGED_PASSIVE();

    process = NULL;
    processHandle = NULL;

    status = ObReferenceObjectByHandle(ProcessHandle,
                                       0,
                                       *PsProcessType,
                                       AccessMode,
                                       &process,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        process = NULL;
        goto Exit;
    }

    status = KphDominationCheck(PsGetCurrentProcess(),
                                process,
                                AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphDominationCheck failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (process == PsGetCurrentProcess())
    {
        KphTracePrint(TRACE_LEVEL_ERROR, GENERAL, "Can not terminate self.");

        status = STATUS_ACCESS_DENIED;
        goto Exit;
    }

    //
    // Re-open the process to get a kernel handle.
    //
    status = ObOpenObjectByPointer(process,
                                   OBJ_KERNEL_HANDLE,
                                   NULL,
                                   PROCESS_TERMINATE,
                                   *PsProcessType,
                                   KernelMode,
                                   &processHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "ObOpenObjectByPointer failed: %!STATUS!",
                      status);

        processHandle = NULL;
        goto Exit;
    }

    status = ZwTerminateProcess(processHandle, ExitStatus);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "ZwTerminateProcess failed: %!STATUS!",
                      status);
    }

Exit:

    if (processHandle)
    {
        ObCloseHandle(processHandle, KernelMode);
    }

    if (process)
    {
        ObDereferenceObject(process);
    }

    return status;
}

/**
 * \brief Queries information about a process.
 * 
 * \param[in] ProcessHandle Handle to process to query.
 * \param[in] ProcessInformationClass Information class to query.
 * \param[out] ProcessInformation Populated with process information by class.
 * \param[in] ProcessInformationLength Length of the process information buffer.
 * \param[out] ReturnLength Number of bytes written or necessary for the
 * information.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    _Out_writes_bytes_opt_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PEPROCESS processObject;
    PKPH_PROCESS_CONTEXT process;
    ULONG returnLength;

    PAGED_PASSIVE();

    processObject = NULL;
    process = NULL;
    returnLength = 0;

    if (AccessMode != KernelMode)
    {
        __try
        {
            if (ProcessInformation)
            {
                ProbeForWrite(ProcessInformation, ProcessInformationLength, 1);
            }

            if (ReturnLength)
            {
                ProbeOutputType(ReturnLength, ULONG);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }

    status = ObReferenceObjectByHandle(ProcessHandle,
                                       0,
                                       *PsProcessType,
                                       AccessMode,
                                       &processObject,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        process = NULL;
        goto Exit;
    }

    process = KphGetProcessContext(PsGetProcessId(processObject));
    if (!process)
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphGetProcessContext return null.");

        status = STATUS_OBJECTID_NOT_FOUND;
        goto Exit;
    }

    switch (ProcessInformationClass)
    {
        case KphProcessBasicInformation:
        {
            PKPH_PROCESS_BASIC_INFORMATION info;

            if (!ProcessInformation ||
                (ProcessInformationLength < sizeof(KPH_PROCESS_BASIC_INFORMATION)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(KPH_PROCESS_BASIC_INFORMATION);
                goto Exit;
            }

            info = ProcessInformation;

            __try
            {
                KphAcquireRWLockShared(&process->ThreadListLock);
                KphAcquireRWLockShared(&process->ProtectionLock);

                info->ProcessState = KphGetProcessState(process);

                info->CreatorClientId.UniqueProcess = process->CreatorClientId.UniqueProcess;
                info->CreatorClientId.UniqueThread = process->CreatorClientId.UniqueThread;

                info->NumberOfImageLoads = process->NumberOfImageLoads;

                info->Flags = process->Flags;

                info->NumberOfThreads = process->NumberOfThreads;

                info->ProcessAllowedMask = process->ProcessAllowedMask;
                info->ThreadAllowedMask = process->ThreadAllowedMask;

                info->NumberOfMicrosoftImageLoads = process->NumberOfMicrosoftImageLoads;
                info->NumberOfAntimalwareImageLoads = process->NumberOfAntimalwareImageLoads;
                info->NumberOfVerifiedImageLoads = process->NumberOfVerifiedImageLoads;
                info->NumberOfUntrustedImageLoads = process->NumberOfUntrustedImageLoads;

                KphReleaseRWLock(&process->ProtectionLock);
                KphReleaseRWLock(&process->ThreadListLock);

                info->UserWritableReferences = 0;
                if (process->FileObject &&
                    process->FileObject->SectionObjectPointer)
                {
                    info->UserWritableReferences =
                        MmDoesFileHaveUserWritableReferences(process->FileObject->SectionObjectPointer);
                }

                status = STATUS_SUCCESS;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
                goto Exit;
            }

            break;
        }
        case KphProcessStateInformation:
        {
            PKPH_PROCESS_STATE state;

            if (!ProcessInformation ||
                (ProcessInformationLength < sizeof(KPH_PROCESS_STATE)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(KPH_PROCESS_STATE);
                goto Exit;
            }

            state = ProcessInformation;

            __try
            {
                *state = KphGetProcessState(process);
                status = STATUS_SUCCESS;
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
                status = GetExceptionCode();
                goto Exit;
            }

            break;
        }
        default:
        {
            status = STATUS_INVALID_INFO_CLASS;
            break;
        }
    }

Exit:

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

    if (process)
    {
        KphDereferenceObject(process);
    }

    if (processObject)
    {
        ObDereferenceObject(processObject);
    }

    return status;
}

/**
 * \brief Sets information about a process.
 * 
 * \param[in] ProcessHandle Handle to process to set information for.
 * \param[in] ProcessInformationClass Information class to set.
 * \param[in] ProcessInformation Information to set.
 * \param[in] ProcessInformationLength Length of the process information buffer.
 * \param[in] AccessMode The mode in which to perform access checks.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphSetInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ KPH_PROCESS_INFORMATION_CLASS ProcessInformationClass,
    _In_reads_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _In_ KPROCESSOR_MODE AccessMode
    )
{
    NTSTATUS status;
    PVOID processInformation;
    UCHAR stackBuffer[64];
    PEPROCESS process;
    HANDLE processHandle;
    PROCESSINFOCLASS processInformationClass;

    PAGED_PASSIVE();

    processInformation = NULL;
    process = NULL;
    processHandle = NULL;

    if (!ProcessInformation)
    {
        status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (AccessMode != KernelMode)
    {
        if (ProcessInformationLength <= ARRAYSIZE(stackBuffer))
        {
            processInformation = stackBuffer;
        }
        else
        {
            processInformation = KphAllocatePaged(ProcessInformationLength,
                                                  KPH_TAG_PROCESS_INFO);
            if (!processInformation)
            {
                KphTracePrint(TRACE_LEVEL_ERROR,
                              GENERAL,
                              "Failed to allocate process info buffer.");

                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
            }
        }

        __try
        {
            ProbeForRead(ProcessInformation, ProcessInformationLength, 1);
            RtlCopyMemory(processInformation,
                          ProcessInformation,
                          ProcessInformationLength);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            status = GetExceptionCode();
            goto Exit;
        }
    }
    else
    {
        processInformation = ProcessInformation;
    }

    status = ObReferenceObjectByHandle(ProcessHandle,
                                       0,
                                       *PsProcessType,
                                       AccessMode,
                                       &process,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        process = NULL;
        goto Exit;
    }

    status = KphDominationCheck(PsGetCurrentProcess(),
                                process,
                                AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "KphDominationCheck failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = ObOpenObjectByPointer(process,
                                   OBJ_KERNEL_HANDLE,
                                   NULL,
                                   PROCESS_SET_INFORMATION,
                                   *PsProcessType,
                                   KernelMode,
                                   &processHandle);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      GENERAL,
                      "ObOpenObjectByPointer failed: %!STATUS!",
                      status);

        processHandle = NULL;
        goto Exit;
    }

    switch (ProcessInformationClass)
    {
        case KphProcessQuotaLimits:
        {
            processInformationClass = ProcessQuotaLimits;
            break;
        }
        case KphProcessBasePriority:
        {
            processInformationClass = ProcessBasePriority;
            break;
        }
        case KphProcessRaisePriority:
        {
            processInformationClass = ProcessRaisePriority;
            break;
        }
        case KphProcessPriorityClass:
        {
            processInformationClass = ProcessPriorityClass;
            break;
        }
        case KphProcessAffinityMask:
        {
            processInformationClass = ProcessAffinityMask;
            break;
        }
        case KphProcessPriorityBoost:
        {
            processInformationClass = ProcessPriorityBoost;
            break;
        }
        case KphProcessIoPriority:
        {
            processInformationClass = ProcessIoPriority;
            break;
        }
        case KphProcessPagePriority:
        {
            processInformationClass = ProcessPagePriority;
            break;
        }
        case KphProcessPowerThrottlingState:
        {
            processInformationClass = ProcessPowerThrottlingState;
            break;
        }
        case KphProcessPriorityClassEx:
        {
            processInformationClass = ProcessPriorityClassEx;
            break;
        }
        default:
        {
            status = STATUS_INVALID_INFO_CLASS;
            goto Exit;
        }
    }

    status = ZwSetInformationProcess(processHandle,
                                     processInformationClass,
                                     processInformation,
                                     ProcessInformationLength);

Exit:

    if (processHandle)
    {
        ObCloseHandle(processHandle, KernelMode);
    }

    if (process)
    {
        ObDereferenceObject(process);
    }

    if (processInformation &&
        (processInformation != ProcessInformation) &&
        (processInformation != stackBuffer))
    {
        KphFree(processInformation, KPH_TAG_PROCESS_INFO);
    }

    return status;
}
