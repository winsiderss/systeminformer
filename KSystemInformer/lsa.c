/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2024-2025
 *
 */

#include <kph.h>

#include <trace.h>

KPH_PROTECTED_DATA_SECTION_RO_PUSH();
static const UNICODE_STRING KphpLsaPortName = RTL_CONSTANT_STRING(L"\\SeLsaCommandPort");
KPH_PROTECTED_DATA_SECTION_RO_POP();
static HANDLE KphpLsassProcessId = NULL;

KPH_PAGED_FILE();

/**
 * \brief Retrieves the process ID of lsass.
 *
 * \param[out] ProcessId Set to the process ID of lsass.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS KphpGetLsassProcessId(
    _Out_ PHANDLE ProcessId
    )
{
    NTSTATUS status;
    KIRQL currentIrql;
    PKPH_DYN dyn;
    HANDLE portHandle;
    KAPC_STATE apcState;
    KPH_ALPC_COMMUNICATION_INFORMATION info;

    KPH_PAGED_CODE();

    //
    // We cache the process ID of lsass since opening and closing the connection
    // port frequently can cause significant performance degradation. This cache
    // is invalidated when:
    //
    // - A process exits and the process ID is what was cached.
    // - The process does not have SeCreateTokenPrivilege.
    //
    // N.B. The privilege check is always performed when checking if a process
    // is lsass, even when the cached process ID is used.
    //

    *ProcessId = InterlockedCompareExchangePointer(&KphpLsassProcessId,
                                                   NULL,
                                                   NULL);
    if (*ProcessId)
    {
        return STATUS_SUCCESS;
    }

    currentIrql = KeGetCurrentIrql();
    if (currentIrql > PASSIVE_LEVEL)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphpGetLsassProcessId called at %!irql!",
                      currentIrql);

        return STATUS_INVALID_LEVEL;
    }

    //
    // N.B. This is an optimization. In order to query the process ID of lsass
    // through the LSA port, we need the dynamic data. Rather than doing the
    // work to attach to the system process and open the port, if we know we
    // do not have the dynamic data, exit early.
    //
    dyn = KphReferenceDynData();
    if (!dyn)
    {
        return STATUS_NOINTERFACE;
    }

    //
    // Attach to system to ensure we get a kernel handle from the following
    // ZwAlpcConnectPort call. Opening the handle here does not ask for the
    // object attributes. To keep our imports simple we choose to use this
    // over the Ex version (might change in the future). Pattern is adopted
    // from msrpc.sys.
    //

    KeStackAttachProcess(PsInitialSystemProcess, &apcState);

    status = ZwAlpcConnectPort(&portHandle,
                               (PUNICODE_STRING)&KphpLsaPortName,
                               NULL,
                               NULL,
                               0,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "ZwAlpcConnectPort failed: %!STATUS!",
                      status);

        portHandle = NULL;
        goto Exit;
    }

    status = KphAlpcQueryInformation(ZwCurrentProcess(),
                                     portHandle,
                                     KphAlpcCommunicationInformation,
                                     &info,
                                     sizeof(KPH_ALPC_COMMUNICATION_INFORMATION),
                                     NULL,
                                     KernelMode);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "KphAlpcQueryInformation failed: %!STATUS!",
                      status);
        goto Exit;
    }

    *ProcessId = info.ConnectionPort.OwnerProcessId;

Exit:

    if (portHandle)
    {
        ObCloseHandle(portHandle, KernelMode);
    }

    KeUnstackDetachProcess(&apcState);

    KphDereferenceObject(dyn);

    return status;
}

/**
 * \brief Caches the lsass process ID if appropriate.
 *
 * \param[in] Process Optional lsass process object.
 * \param[in] ProcessId The lsass process ID to cache.
 */
_IRQL_requires_max_(APC_LEVEL)
VOID KphpCacheLsassProcessId(
    _In_opt_ PEPROCESS Process,
    _In_ HANDLE ProcessId
    )
{
    PEPROCESS process;

    KPH_PAGED_CODE();

    if (ReadPointerAcquire(&KphpLsassProcessId) == ProcessId)
    {
        return;
    }

    if (Process)
    {
        process = Process;
    }
    else if (!NT_SUCCESS(PsLookupProcessByProcessId(ProcessId, &process)))
    {
        return;
    }

    NT_ASSERT(PsGetProcessId(process) == ProcessId);

    if (NT_SUCCESS(PsAcquireProcessExitSynchronization(process)))
    {
        InterlockedExchangePointer(&KphpLsassProcessId, ProcessId);
        PsReleaseProcessExitSynchronization(process);
    }

    if (process != Process)
    {
        ObDereferenceObject(process);
    }
}

/**
 * \brief Checks if a given process is lsass.
 *
 * \param[in] Process The process to check.
 * \param[out] IsLsass TRUE if the process is lsass, FALSE otherwise.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(APC_LEVEL)
_Must_inspect_result_
NTSTATUS KphProcessIsLsass(
    _In_ PEPROCESS Process,
    _Out_ PBOOLEAN IsLsass
    )
{
    NTSTATUS status;
    PS_PROTECTION processProtection;
    HANDLE processId;
    SECURITY_SUBJECT_CONTEXT subjectContext;
    BOOLEAN result;

    KPH_PAGED_CODE();

    *IsLsass = FALSE;

    processProtection = PsGetProcessProtection(Process);

    if ((processProtection.Type != PsProtectedTypeNone) &&
        (processProtection.Signer == PsProtectedSignerLsa))
    {
        processId = PsGetProcessId(Process);

        KphpCacheLsassProcessId(Process, processId);
    }
    else
    {
        status = KphpGetLsassProcessId(&processId);
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        KphpCacheLsassProcessId(NULL, processId);

        if (processId != PsGetProcessId(Process))
        {
            return STATUS_SUCCESS;
        }
    }

    SeCaptureSubjectContextEx(NULL, Process, &subjectContext);
    result = KphSinglePrivilegeCheckEx(SeCreateTokenPrivilege,
                                       &subjectContext,
                                       UserMode);
    SeReleaseSubjectContext(&subjectContext);
    if (result)
    {
        *IsLsass = TRUE;
    }
    else
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      GENERAL,
                      "PID %lu does not have SeCreateTokenPrivilege",
                      HandleToULong(processId));

        InterlockedExchangePointer(&KphpLsassProcessId, NULL);
    }

    return STATUS_SUCCESS;
}

/**
 * \brief Invalidates the cached lsass process ID if it matches.
 *
 * \details This should be called whenever a process exits.
 *
 * \param[in] ProcessId The process ID to invalidate.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphInvalidateLsass(
    _In_ HANDLE ProcessId
    )
{
    KPH_PAGED_CODE_PASSIVE();

    InterlockedCompareExchangePointer(&KphpLsassProcessId, NULL, ProcessId);
}
