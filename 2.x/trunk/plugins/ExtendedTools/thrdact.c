/*
 * Process Hacker Extended Tools - 
 *   thread actions extensions
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

#include "exttools.h"
#include "resource.h"

BOOLEAN EtUiCancelIoThread(
    __in HWND hWnd,
    __in PPH_THREAD_ITEM Thread
    )
{
    NTSTATUS status;
    BOOLEAN cont = FALSE;
    HANDLE threadHandle;
    IO_STATUS_BLOCK isb;

    if (!PhGetIntegerSetting(L"EnableWarnings") || PhShowConfirmMessage(
        hWnd,
        L"end",
        L"I/O for the selected thread",
        NULL,
        FALSE
        ))
        cont = TRUE;

    if (!cont)
        return FALSE;

    if (NT_SUCCESS(status = PhOpenThread(&threadHandle, THREAD_TERMINATE, Thread->ThreadId)))
    {
        status = NtCancelSynchronousIoFile(threadHandle, NULL, &isb);
    }

    if (status == STATUS_NOT_FOUND)
    {
        PhShowInformation(hWnd, L"There is no synchronous I/O to cancel.");
        return FALSE;
    }
    else if (!NT_SUCCESS(status))
    {
        PhShowStatus(hWnd, L"Unable to cancel synchronous I/O", status, 0);
        return FALSE;
    }

    return TRUE;
}
