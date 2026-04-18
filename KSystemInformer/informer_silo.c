/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2026
 *
 */

#include <kph.h>
#include <informer.h>
#include <comms.h>
#include <kphmsgdyn.h>

#include <trace.h>

KPH_PAGED_FILE();

static PSILO_MONITOR KphpSiloMonitor = NULL;

//
// N.B. PsIsHostSilo is just a NULL check. The desired pattern by Microsoft
// is to infer the host silo based on a NULL PESILO. This is unfortunate since
// their SAL annotations ended up misleading. They state _In_ when the host
// silo is passed (a NULL pointer). This breaks reasonable expectations and
// one can easily pass a NULL pointer to a routine like ObQueryNameString. Our
// ntfill and callback code opts to use _In_opt_ to indicate what routines
// are capable of handling a NULL PESILE or not. Our callbacks also opt to use
// _In_opt_ instead of _In_ to make it obvious that the callback can be called
// with a NULL PESILO.
//

/**
 * \brief Copies silo information from a silo object.
 *
 * \param[in] Silo The silo to copy information from.
 * \param[out] SiloInfo The silo information structure to fill.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpCopySiloInformation(
    _In_opt_ PESILO Silo,
    _Out_ PKPHM_SILO_INFORMATION SiloInfo
    )
{
    KPH_PAGED_CODE_PASSIVE();

    RtlZeroMemory(SiloInfo, sizeof(KPHM_SILO_INFORMATION));

    if (KphDynPsIsHostSilo)
    {
        SiloInfo->IsHostSilo = KphDynPsIsHostSilo(Silo);
    }

    if (KphDynPsGetSiloIdentifier)
    {
        SiloInfo->SiloIdentifier = KphDynPsGetSiloIdentifier(Silo);
    }

    if (KphDynPsGetServerSiloServiceSessionId)
    {
        SiloInfo->ServiceSessionId = KphDynPsGetServerSiloServiceSessionId(Silo);
    }

    if (KphDynPsGetServerSiloActiveConsoleId)
    {
        SiloInfo->ActiveConsoleId = KphDynPsGetServerSiloActiveConsoleId(Silo);
    }

    if (KphDynPsGetSiloContainerId && Silo)
    {
        RtlCopyMemory(&SiloInfo->ContainerId,
                      KphDynPsGetSiloContainerId(Silo),
                      sizeof(GUID));
    }
}

/**
 * \brief Copies the silo name to a message structure.
 *
 * \param[in,out] Message The message to copy the silo name to.
 * \param[in] Silo The silo to copy the name from.
 * \param[in] FieldId The field ID of the message to copy the silo name to.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphpCopySiloName(
    _Inout_ PKPH_MESSAGE Message,
    _In_ PESILO Silo,
    _In_ ULONG FieldId
    )
{
    NTSTATUS status;
    POBJECT_NAME_INFORMATION info;
    ULONG length;

    KPH_PAGED_CODE_PASSIVE();

    info = NULL;

    status = ObQueryNameString(Silo, NULL, 0, &length);
    if ((status != STATUS_INFO_LENGTH_MISMATCH) &&
        (status != STATUS_BUFFER_TOO_SMALL))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "ObQueryNameString: %!STATUS!",
                      status);

        goto Exit;
    }

    info = KphAllocatePaged(length, KPH_TAG_SILO_NAME);
    if (!info)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to allocate buffer for object name");

        goto Exit;
    }

    status = ObQueryNameString(Silo, info, length, &length);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "ObQueryNameString failed: %!STATUS!",
                      status);

        goto Exit;
    }

    status = KphMsgDynAddUnicodeString(Message, FieldId, &info->Name);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "KphMsgDynAddUnicodeString failed: %!STATUS!",
                      status);
    }

Exit:

    if (info)
    {
        KphFree(info, KPH_TAG_SILO_NAME);
    }
}

/**
 * \brief Silo monitor create callback.
 *
 * \param[in] Silo The silo being created.
 *
 * \return STATUS_SUCCESS
 */
_Function_class_(SILO_MONITOR_CREATE_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS NTAPI KphpSiloMonitorCreateCallback(
    _In_opt_ PESILO Silo
    )
{
    PKPH_MESSAGE msg;

    KPH_PAGED_CODE_PASSIVE();

    KPH_INFORMER_CONTEXT_ENTER();

    if (!KphInformerEnabled(SiloCreate))
    {
        goto Exit;
    }

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to allocate message");
        goto Exit;
    }

    KphMsgInit(msg, KphMsgSiloCreate);
    KphCaptureCurrentContext(&msg->Kernel.SiloCreate.Context);

    KphpCopySiloInformation(Silo, &msg->Kernel.SiloCreate.Silo);

    if (Silo)
    {
        KphpCopySiloName(msg, Silo, KphMsgFieldObjectName);
    }

    if (KphDynPsGetEffectiveServerSilo)
    {
        PESILO serverSilo;

        serverSilo = KphDynPsGetEffectiveServerSilo(Silo);

        KphpCopySiloInformation(serverSilo, &msg->Kernel.SiloCreate.ServerSilo);

        if (serverSilo)
        {
            KphpCopySiloName(msg, serverSilo, KphMsgFieldOtherObjectName);
        }
    }

    if (KphInformerOpts().EnableStackTraces)
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);

Exit:

    KPH_INFORMER_CONTEXT_EXIT();

    return STATUS_SUCCESS;
}

/**
 * \brief Silo monitor terminate callback.
 *
 * \param[in] Silo The silo being terminated.
 */
_Function_class_(SILO_MONITOR_TERMINATE_CALLBACK)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID NTAPI KphpSiloMonitorTerminateCallback(
    _In_opt_ PESILO Silo
    )
{
    PKPH_MESSAGE msg;

    KPH_PAGED_CODE_PASSIVE();

    KPH_INFORMER_CONTEXT_ENTER();

    if (!KphInformerEnabled(SiloTerminate))
    {
        goto Exit;
    }

    msg = KphAllocateMessage();
    if (!msg)
    {
        KphTracePrint(TRACE_LEVEL_VERBOSE,
                      INFORMER,
                      "Failed to allocate message");
        goto Exit;
    }

    KphMsgInit(msg, KphMsgSiloTerminate);
    KphCaptureCurrentContext(&msg->Kernel.SiloTerminate.Context);

    KphpCopySiloInformation(Silo, &msg->Kernel.SiloTerminate.Silo);

    if (Silo)
    {
        KphpCopySiloName(msg, Silo, KphMsgFieldObjectName);
    }

    if (KphDynPsGetEffectiveServerSilo)
    {
        PESILO serverSilo;

        serverSilo = KphDynPsGetEffectiveServerSilo(Silo);

        KphpCopySiloInformation(serverSilo, &msg->Kernel.SiloTerminate.ServerSilo);

        if (serverSilo)
        {
            KphpCopySiloName(msg, serverSilo, KphMsgFieldOtherObjectName);
        }
    }

    if (KphInformerOpts().EnableStackTraces)
    {
        KphCaptureStackInMessage(msg);
    }

    KphCommsSendMessageAsync(msg);

Exit:

    KPH_INFORMER_CONTEXT_EXIT();
}

/**
 * \brief Starts the silo informer.
 *
 * \return Successful or errant status.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS KphSiloInformerStart(
    VOID
    )
{
    NTSTATUS status;
    SILO_MONITOR_REGISTRATION registration;

    KPH_PAGED_CODE_PASSIVE();

    if (!KphDynPsRegisterSiloMonitor ||
        !KphDynPsUnregisterSiloMonitor ||
        !KphDynPsStartSiloMonitor)
    {
        return STATUS_SUCCESS;
    }

    RtlZeroMemory(&registration, sizeof(SILO_MONITOR_REGISTRATION));
    registration.Version = SILO_MONITOR_REGISTRATION_VERSION;
    registration.MonitorHost = TRUE;
    registration.MonitorExistingSilos = TRUE;
#pragma prefast(suppress : 28175)
    registration.DriverObjectName = &KphDriverObject->DriverName;
    registration.CreateCallback = KphpSiloMonitorCreateCallback;
    registration.TerminateCallback = KphpSiloMonitorTerminateCallback;

    status = KphDynPsRegisterSiloMonitor(&registration, &KphpSiloMonitor);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "PsRegisterSiloMonitor failed: %!STATUS!",
                      status);

        KphpSiloMonitor = NULL;
        goto Exit;
    }

    status = KphDynPsStartSiloMonitor(KphpSiloMonitor);
    if (!NT_SUCCESS(status))
    {
        KphTracePrint(TRACE_LEVEL_ERROR,
                      INFORMER,
                      "PsStartSiloMonitor failed: %!STATUS!",
                      status);

        KphDynPsUnregisterSiloMonitor(KphpSiloMonitor);
        KphpSiloMonitor = NULL;
        goto Exit;
    }

Exit:

    //
    // Silo callbacks are not critical to driver entry.
    //
    return STATUS_SUCCESS;
}

/**
 * \brief Stops the silo informer.
 */
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID KphSiloInformerStop(
    VOID
    )
{
    KPH_PAGED_CODE_PASSIVE();

    if (KphpSiloMonitor)
    {
        NT_ASSERT(KphDynPsUnregisterSiloMonitor);

        KphDynPsUnregisterSiloMonitor(KphpSiloMonitor);
    }
}
