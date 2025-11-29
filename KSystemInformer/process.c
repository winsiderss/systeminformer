/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     jxy-s   2020-2025
 *
 */

#include <kph.h>

#include <trace.h>

KPH_PAGED_FILE();

/**
 * \brief Opens a process.
 *
 * \param[out] ProcessHandle A variable which receives the process handle.
 * \param[in] DesiredAccess The desired access to the process.
 * \param[in] ClientId The identifier of a client. UniqueThread is optional.
 * If UniqueThread is present, the thread of the referenced process will be
 * checked against this identifier.
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

    KPH_PAGED_CODE_PASSIVE();

    process = NULL;

    status = KphCopyFromMode(&clientId,
                             ClientId,
                             sizeof(CLIENT_ID),
                             AccessMode);
    if (!NT_SUCCESS(status))
    {
        goto Exit;
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
            KphTracePrint(TRACE_LEVEL_VERBOSE,
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
            KphTracePrint(TRACE_LEVEL_VERBOSE,
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
            KphTracePrint(TRACE_LEVEL_VERBOSE,
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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObOpenObjectByPointer failed: %!STATUS!",
                      status);

        processHandle = NULL;
        goto Exit;
    }

    status = KphWriteHandleToMode(ProcessHandle, processHandle, AccessMode);

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

    KPH_PAGED_CODE_PASSIVE();

    primaryToken = NULL;

    status = ObReferenceObjectByHandle(ProcessHandle,
                                       0,
                                       *PsProcessType,
                                       AccessMode,
                                       &process,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        process = NULL;
        goto Exit;
    }

    primaryToken = PsReferencePrimaryToken(process);
    if (!primaryToken)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
            KphTracePrint(TRACE_LEVEL_VERBOSE,
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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObOpenObjectByPointer failed: %!STATUS!",
                      status);

        tokenHandle = NULL;
        goto Exit;
    }

    status = KphWriteHandleToMode(TokenHandle, tokenHandle, AccessMode);

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

    KPH_PAGED_CODE_PASSIVE();

    status = ObReferenceObjectByHandle(ProcessHandle,
                                       0,
                                       *PsProcessType,
                                       AccessMode,
                                       &process,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
            KphTracePrint(TRACE_LEVEL_VERBOSE,
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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObOpenObjectByPointer failed: %!STATUS!",
                      status);

        jobHandle = NULL;
        goto Exit;
    }

    status = KphWriteHandleToMode(JobHandle, jobHandle, AccessMode);

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
 * \param[in] ExitStatus A status value initiating why the process terminated.
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

    KPH_PAGED_CODE_PASSIVE();

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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        process = NULL;
        goto Exit;
    }

    status = KphDominationAndPrivilegeCheck(KPH_TOKEN_PRIVILEGE_TERMINATE,
                                            PsGetCurrentThread(),
                                            process,
                                            AccessMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphDominationAndPrivilegeCheck failed: %!STATUS!",
                      status);

        goto Exit;
    }

    if (process == PsGetCurrentProcess())
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE, GENERAL, "Can not terminate self.");

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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObOpenObjectByPointer failed: %!STATUS!",
                      status);

        processHandle = NULL;
        goto Exit;
    }

    status = ZwTerminateProcess(processHandle, ExitStatus);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
 * \param[out] ReturnLength Receives the number of bytes written or required.
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
    PKPH_DYN dyn;
    PEPROCESS processObject;
    PKPH_PROCESS_CONTEXT process;
    ULONG returnLength;

    KPH_PAGED_CODE_PASSIVE();

    dyn = NULL;
    process = NULL;
    returnLength = 0;

    status = ObReferenceObjectByHandle(ProcessHandle,
                                       0,
                                       *PsProcessType,
                                       AccessMode,
                                       &processObject,
                                       NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        processObject = NULL;
        goto Exit;
    }

    process = KphGetEProcessContext(processObject);
    if (!process)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphGetEProcessContext return null.");

        status = STATUS_OBJECTID_NOT_FOUND;
        goto Exit;
    }

    switch (ProcessInformationClass)
    {
        case KphProcessBasicInformation:
        {
            KPH_PROCESS_BASIC_INFORMATION info;

            if (!ProcessInformation ||
                (ProcessInformationLength < sizeof(KPH_PROCESS_BASIC_INFORMATION)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(KPH_PROCESS_BASIC_INFORMATION);
                goto Exit;
            }

            KphAcquireRWLockShared(&process->ThreadListLock);
            KphAcquireRWLockShared(&process->ProtectionLock);

            info.ProcessState = KphGetProcessState(process);

            info.ProcessStartKey = KphGetProcessStartKey(processObject);
            info.CreatorClientId.UniqueProcess = process->CreatorClientId.UniqueProcess;
            info.CreatorClientId.UniqueThread = process->CreatorClientId.UniqueThread;

            info.NumberOfImageLoads = ReadSizeTNoFence(&process->NumberOfImageLoads);

            info.Flags = process->Flags;

            info.NumberOfThreads = process->NumberOfThreads;

            info.ProcessAllowedMask = process->ProcessAllowedMask;
            info.ThreadAllowedMask = process->ThreadAllowedMask;

            info.NumberOfMicrosoftImageLoads = ReadSizeTNoFence(&process->NumberOfMicrosoftImageLoads);
            info.NumberOfAntimalwareImageLoads = ReadSizeTNoFence(&process->NumberOfAntimalwareImageLoads);
            info.NumberOfVerifiedImageLoads = ReadSizeTNoFence(&process->NumberOfVerifiedImageLoads);
            info.NumberOfUntrustedImageLoads = ReadSizeTNoFence(&process->NumberOfUntrustedImageLoads);

            info.UserWritableReferences = 0;
            if (process->FileObject &&
                process->FileObject->SectionObjectPointer)
            {
                info.UserWritableReferences =
                    MmDoesFileHaveUserWritableReferences(process->FileObject->SectionObjectPointer);
            }

            KphReleaseRWLock(&process->ProtectionLock);
            KphReleaseRWLock(&process->ThreadListLock);

            status = KphCopyToMode(ProcessInformation,
                                   &info,
                                   sizeof(KPH_PROCESS_BASIC_INFORMATION),
                                   AccessMode);
            if (NT_SUCCESS(status))
            {
                returnLength = sizeof(KPH_PROCESS_BASIC_INFORMATION);
            }

            break;
        }
        case KphProcessStateInformation:
        {
            KPH_PROCESS_STATE processState;

            if (!ProcessInformation ||
                (ProcessInformationLength < sizeof(KPH_PROCESS_STATE)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(KPH_PROCESS_STATE);
                goto Exit;
            }

            processState = KphGetProcessState(process);

            status = KphWriteULongToMode(ProcessInformation,
                                         processState,
                                         AccessMode);
            if (NT_SUCCESS(status))
            {
                returnLength = sizeof(KPH_PROCESS_STATE);
            }

            break;
        }
        case KphProcessWSLProcessId:
        {
            ULONG processId;

            if (process->SubsystemType != SubsystemInformationTypeWSL)
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "Invalid subsystem for WSL process ID query.");

                status = STATUS_INVALID_HANDLE;
                goto Exit;
            }

            if (!ProcessInformation ||
                (ProcessInformationLength < sizeof(ULONG)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(ULONG);
                goto Exit;
            }

            status = KphQueryInformationProcessContext(process,
                                                       KphProcessContextWSLProcessId,
                                                       &processId,
                                                       sizeof(ULONG),
                                                       NULL);
            if (!NT_SUCCESS(status))
            {
                goto Exit;
            }

            status = KphWriteULongToMode(ProcessInformation,
                                         processId,
                                         AccessMode);
            if (NT_SUCCESS(status))
            {
                returnLength = sizeof(ULONG);
            }

            break;
        }
        case KphProcessSequenceNumber:
        {
            ULONG64 sequenceNumber;

            if (!ProcessInformation ||
                (ProcessInformationLength < sizeof(ULONG64)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(ULONG64);
                goto Exit;
            }

            sequenceNumber = KphGetProcessSequenceNumber(processObject);

            status = KphWriteULong64ToMode(ProcessInformation,
                                           sequenceNumber,
                                           AccessMode);
            if (NT_SUCCESS(status))
            {
                returnLength = sizeof(ULONG64);
            }

            break;
        }
        case KphProcessStartKey:
        {
            ULONG64 startKey;

            if (!ProcessInformation ||
                (ProcessInformationLength < sizeof(ULONG64)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(ULONG64);
                goto Exit;
            }

            startKey = KphGetProcessStartKey(processObject);

            status = KphWriteULong64ToMode(ProcessInformation,
                                           startKey,
                                           AccessMode);
            if (NT_SUCCESS(status))
            {
                returnLength = sizeof(ULONG64);
            }

            break;
        }
        case KphProcessImageSection:
        {
            PVOID processSectionObject;
            HANDLE processSectionHandle;

            dyn = KphReferenceDynData();
            if (!dyn || (dyn->EpSectionObject == ULONG_MAX))
            {
                status = STATUS_NOINTERFACE;
                goto Exit;
            }

            if (!ProcessInformation ||
                (ProcessInformationLength < sizeof(HANDLE)))
            {
                status = STATUS_INFO_LENGTH_MISMATCH;
                returnLength = sizeof(HANDLE);
                goto Exit;
            }

            status = PsAcquireProcessExitSynchronization(processObject);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "PsAcquireProcessExitSynchronization failed: %!STATUS!",
                              status);

                goto Exit;
            }

            processSectionObject = *(PVOID*)Add2Ptr(processObject,
                                                    dyn->EpSectionObject);

            if (processSectionObject)
            {
                ObReferenceObject(processSectionObject);
            }

            PsReleaseProcessExitSynchronization(processObject);

            if (!processSectionObject)
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "Process section object pointer is null.");

                status = STATUS_INVALID_PARAMETER;
                goto Exit;
            }

            status = ObOpenObjectByPointer(processSectionObject,
                                           (AccessMode ? 0 : OBJ_KERNEL_HANDLE),
                                           NULL,
                                           SECTION_QUERY | SECTION_MAP_READ,
                                           *MmSectionObjectType,
                                           KernelMode,
                                           &processSectionHandle);

            ObDereferenceObject(processSectionObject);

            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "ObOpenObjectByPointer failed: %!STATUS!",
                              status);

                goto Exit;
            }

            status = KphWriteHandleToMode(ProcessInformation,
                                          processSectionHandle,
                                          AccessMode);
            if (NT_SUCCESS(status))
            {
                returnLength = sizeof(HANDLE);
            }

            break;
        }
        case KphProcessImageFileName:
        {
            if (!process->ImageFileName)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto Exit;
            }

            status = KphCopyUnicodeStringToMode(ProcessInformation,
                                                ProcessInformationLength,
                                                process->ImageFileName,
                                                &returnLength,
                                                AccessMode);

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
        KphWriteULongToMode(ReturnLength, returnLength, AccessMode);
    }

    if (process)
    {
        KphDereferenceObject(process);
    }

    if (processObject)
    {
        ObDereferenceObject(processObject);
    }

    if (dyn)
    {
        KphDereferenceObject(dyn);
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
    BYTE stackBuffer[64];
    PEPROCESS process;
    HANDLE processHandle;
    PROCESSINFOCLASS processInformationClass;

    KPH_PAGED_CODE_PASSIVE();

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
        processInformation = KphAllocatePagedA(ProcessInformationLength,
                                               KPH_TAG_PROCESS_INFO,
                                               stackBuffer);
        if (!processInformation)
        {
            KphTracePrint(TRACE_LEVEL_VERBOSE,
                          GENERAL,
                          "Failed to allocate process info buffer.");

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit;
        }

        __try
        {
            CopyFromUser(processInformation,
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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ObReferenceObjectByHandle failed: %!STATUS!",
                      status);

        process = NULL;
        goto Exit;
    }

    switch (ProcessInformationClass)
    {
        case KphProcessEmptyWorkingSet:
        {
            //
            // Permitted across PPL boundaries.
            //
            break;
        }
        case KphProcessQuotaLimits:
        case KphProcessBasePriority:
        case KphProcessRaisePriority:
        case KphProcessPriorityClass:
        case KphProcessAffinityMask:
        case KphProcessPriorityBoost:
        case KphProcessIoPriority:
        case KphProcessPagePriority:
        case KphProcessPowerThrottlingState:
        case KphProcessPriorityClassEx:
        default:
        {
            status = KphDominationCheck(PsGetCurrentProcess(),
                                        process,
                                        AccessMode);
            if (!NT_SUCCESS(status))
            {
                KphTracePrint(TRACE_LEVEL_VERBOSE,
                              GENERAL,
                              "KphDominationCheck failed: %!STATUS!",
                              status);

                goto Exit;
            }
            break;
        }
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
        KphTracePrint(TRACE_LEVEL_VERBOSE,
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
        case KphProcessEmptyWorkingSet:
        {
            QUOTA_LIMITS_EX quotaLimits;

            RtlZeroMemory(&quotaLimits, sizeof(QUOTA_LIMITS_EX));
            quotaLimits.MinimumWorkingSetSize = SIZE_T_MAX;
            quotaLimits.MaximumWorkingSetSize = SIZE_T_MAX;

            status = ZwSetInformationProcess(processHandle,
                                             ProcessQuotaLimits,
                                             &quotaLimits,
                                             sizeof(QUOTA_LIMITS_EX));
            //
            // Bypass generic call to exit immediately.
            //
            goto Exit;
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

    if (processInformation && (processInformation != ProcessInformation))
    {
        KphFreeA(processInformation, KPH_TAG_PROCESS_INFO, stackBuffer);
    }

    return status;
}
