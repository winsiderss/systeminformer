/*
 * Process Hacker -
 *   server
 *
 * Copyright (C) 2011 wj32
 * Copyright (C) 2018 dmex
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
