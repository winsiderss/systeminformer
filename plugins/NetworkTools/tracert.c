/*
 * Process Hacker Network Tools -
 *   Tracert dialog
 *
 * Copyright (C) 2010-2013 wj32
 * Copyright (C) 2012-2013 dmex
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

#include "nettools.h"

static NTSTATUS StdOutNetworkTracertThreadStart(
    __in PVOID Parameter
    )
{
    NTSTATUS status;
    PNETWORK_OUTPUT_CONTEXT context = (PNETWORK_OUTPUT_CONTEXT)Parameter;
    IO_STATUS_BLOCK isb;
    UCHAR buffer[PAGE_SIZE];

    while (TRUE)
    {
        status = NtReadFile(
            context->PipeReadHandle,
            NULL,
            NULL,
            NULL,
            &isb,
            buffer,
            sizeof(buffer),
            NULL,
            NULL
            );

        if (!NT_SUCCESS(status))
        {
            SendMessage(context->WindowHandle, NTM_DONE, 0, 0);
            goto ExitCleanup;
        }

        SendMessage(context->WindowHandle, NTM_RECEIVED, (WPARAM)isb.Information, (LPARAM)buffer);
    }

ExitCleanup:
    NtClose(context->PipeReadHandle);

    return STATUS_SUCCESS;
}

NTSTATUS NetworkTracertThreadStart(
    __in PVOID Parameter
    )
{
    HANDLE pipeWriteHandle = INVALID_HANDLE_VALUE;
    PNETWORK_OUTPUT_CONTEXT context = (PNETWORK_OUTPUT_CONTEXT)Parameter;
        
    Static_SetText(context->WindowHandle,   
        PhFormatString(L"Tracing route to %s...", context->addressString)->Buffer);

    if (CreatePipe(&context->PipeReadHandle, &pipeWriteHandle, NULL, 0))
    {
        STARTUPINFO startupInfo = { sizeof(startupInfo) };
        OBJECT_HANDLE_FLAG_INFORMATION flagInfo;
        PPH_STRING command = NULL;

        startupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        startupInfo.hStdOutput = pipeWriteHandle;
        startupInfo.hStdError = pipeWriteHandle;
        startupInfo.wShowWindow = SW_HIDE;

        if (PhGetIntegerSetting(L"ProcessHacker.NetTools.EnableHostnameLookup"))
        {
            command = PhFormatString(
                L"%s\\system32\\tracert.exe %s",
                USER_SHARED_DATA->NtSystemRoot,
                context->addressString
                );
        }
        else
        {
            // Disable hostname lookup.
            command = PhFormatString(
                L"%s\\system32\\tracert.exe -d %s",
                USER_SHARED_DATA->NtSystemRoot,
                context->addressString
                );
        }

        // Allow the write handle to be inherited.
        flagInfo.Inherit = TRUE;
        flagInfo.ProtectFromClose = FALSE;

        NtSetInformationObject(
            pipeWriteHandle,
            ObjectHandleFlagInformation,
            &flagInfo,
            sizeof(OBJECT_HANDLE_FLAG_INFORMATION)
            );

        PhCreateProcessWin32Ex(
            NULL,
            command->Buffer,
            NULL,
            NULL,
            &startupInfo,
            PH_CREATE_PROCESS_INHERIT_HANDLES,
            NULL,
            NULL,
            &context->ProcessHandle,
            NULL
            );

        // Essential; when the process exits, the last instance of the pipe will be disconnected and our thread will exit.
        NtClose(pipeWriteHandle);

        // Create a thread which will wait for output and display it.
        context->ThreadHandle = PhCreateThread(0, (PUSER_THREAD_START_ROUTINE)StdOutNetworkTracertThreadStart, context);
    }

    return STATUS_SUCCESS;
}