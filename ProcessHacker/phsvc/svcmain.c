/*
 * Process Hacker - 
 *   server
 * 
 * Copyright (C) 2011 wj32
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

HANDLE PhSvcTimeoutStandbyEventHandle;
HANDLE PhSvcTimeoutCancelEventHandle;

NTSTATUS PhSvcMain()
{
    NTSTATUS status;
    LARGE_INTEGER timeout;

    if (!NT_SUCCESS(status = PhSvcClientInitialization()))
        return status;
    if (!NT_SUCCESS(status = PhSvcApiInitialization()))
        return status;
    if (!NT_SUCCESS(status = PhSvcApiPortInitialization()))
        return status;

    if (!NT_SUCCESS(status = NtCreateEvent(&PhSvcTimeoutStandbyEventHandle, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, TRUE)))
        return status;

    if (!NT_SUCCESS(status = NtCreateEvent(&PhSvcTimeoutCancelEventHandle, EVENT_ALL_ACCESS, NULL, SynchronizationEvent, FALSE)))
    {
        NtClose(PhSvcTimeoutStandbyEventHandle);
        return status;
    }

    timeout.QuadPart = -15 * PH_TIMEOUT_SEC;

    while (TRUE)
    {
        NtWaitForSingleObject(PhSvcTimeoutStandbyEventHandle, FALSE, NULL);
        status = NtWaitForSingleObject(PhSvcTimeoutCancelEventHandle, FALSE, &timeout);

        if (status == STATUS_TIMEOUT)
            break;

        // A client connected, so we wait on the standby event again.
    }

    return status;
}
