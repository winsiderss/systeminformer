/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *
 */

#include "exttools.h"

BOOLEAN EtUiCancelIoThread(
    _In_ HWND hWnd,
    _In_ PPH_THREAD_ITEM Thread
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
        NtClose(threadHandle);
    }

    if (status == STATUS_NOT_FOUND)
    {
        PhShowInformation(hWnd, L"%s", L"There is no synchronous I/O to cancel.");
        return FALSE;
    }
    else if (!NT_SUCCESS(status))
    {
        PhShowStatus(hWnd, L"Unable to cancel synchronous I/O", status, 0);
        return FALSE;
    }

    return TRUE;
}
