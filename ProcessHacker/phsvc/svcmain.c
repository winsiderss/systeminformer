/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011
 *     dmex    2018
 *
 */

#include <phapp.h>
#include <phsvc.h>

HANDLE PhSvcTimeoutStandbyEventHandle = NULL;
HANDLE PhSvcTimeoutCancelEventHandle = NULL;

NTSTATUS PhSvcMain(
    _In_opt_ PPH_STRING PortName,
    _Inout_opt_ PPHSVC_STOP Stop
    )
{
    NTSTATUS status;
    UNICODE_STRING portName;
    LARGE_INTEGER timeout;

    if (PortName)
    {
        PhStringRefToUnicodeString(&PortName->sr, &portName);
    }
    else
    {
        if (PhIsExecutingInWow64())
            RtlInitUnicodeString(&portName, PHSVC_WOW64_PORT_NAME);
        else
            RtlInitUnicodeString(&portName, PHSVC_PORT_NAME);
    }

    if (!NT_SUCCESS(status = PhSvcApiPortInitialization(&portName)))
        return status;

    if (!NT_SUCCESS(status = NtCreateEvent(&PhSvcTimeoutStandbyEventHandle, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, TRUE)))
        return status;

    if (!NT_SUCCESS(status = NtCreateEvent(&PhSvcTimeoutCancelEventHandle, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE)))
    {
        NtClose(PhSvcTimeoutStandbyEventHandle);
        return status;
    }

    if (Stop)
    {
        Stop->Event1 = PhSvcTimeoutStandbyEventHandle;
        Stop->Event2 = PhSvcTimeoutCancelEventHandle;
        MemoryBarrier();

        if (Stop->Stop)
            return STATUS_SUCCESS;
    }

    while (TRUE)
    {
        NtWaitForSingleObject(PhSvcTimeoutStandbyEventHandle, FALSE, NULL);

        if (Stop && Stop->Stop)
            break;

        status = NtWaitForSingleObject(PhSvcTimeoutCancelEventHandle, FALSE, PhTimeoutFromMilliseconds(&timeout, 10 * 1000));

        if (Stop && Stop->Stop)
            break;
        if (status == STATUS_TIMEOUT)
            break;

        // A client connected, so we wait on the standby event again.
    }

    return status;
}

VOID PhSvcStop(
    _Inout_ PPHSVC_STOP Stop
    )
{
    Stop->Stop = TRUE;

    if (Stop->Event1)
        NtSetEvent(Stop->Event1, NULL);
    if (Stop->Event2)
        NtSetEvent(Stop->Event2, NULL);
}
