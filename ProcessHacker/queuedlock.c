/*
 * Process Hacker - 
 *   queued lock
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

#include <phbase.h>

HANDLE PhQueuedLockKeyedEvent;
ULONG PhQueuedLockSpinCount;

BOOLEAN PhQueuedLockInitialization()
{
    if (!NT_SUCCESS(NtCreateKeyedEvent(
        &PhQueuedLockKeyedEvent,
        KEYEDEVENT_ALL_ACCESS,
        NULL,
        0
        )))
        return FALSE;

    if ((ULONG)PhSystemBasicInformation.NumberOfProcessors > 1)
        PhQueuedLockSpinCount = 4000;
    else
        PhQueuedLockSpinCount = 0;

    return TRUE;
}

VOID FASTCALL PhfAcquireQueuedLockExclusive(
    __inout PPH_QUEUED_LOCK QueuedLock
    )
{

}

VOID FASTCALL PhfAcquireQueuedLockShared(
    __inout PPH_QUEUED_LOCK QueuedLock
    )
{

}

VOID FASTCALL PhfReleaseQueuedLockExclusive(
    __inout PPH_QUEUED_LOCK QueuedLock
    )
{

}

VOID FASTCALL PhfReleaseQueuedLockShared(
    __inout PPH_QUEUED_LOCK QueuedLock
    )
{

}
