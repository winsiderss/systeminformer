/*
 * Process Hacker Network Tools -
 *   Tracert dialog
 *
 * Copyright (C) 2013 dmex
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
    _In_ PVOID Parameter
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
            break;

        SendMessage(context->WindowHandle, NTM_RECEIVEDTRACE, (WPARAM)isb.Information, (LPARAM)buffer);
    }

    SendMessage(context->WindowHandle, NTM_RECEIVEDFINISH, 0, 0);

    return STATUS_SUCCESS;
}

NTSTATUS NetworkTracertThreadStart(
    _In_ PVOID Parameter
    )
{
    HANDLE pipeWriteHandle = INVALID_HANDLE_VALUE;
    PNETWORK_OUTPUT_CONTEXT context = (PNETWORK_OUTPUT_CONTEXT)Parameter;

    if (CreatePipe(&context->PipeReadHandle, &pipeWriteHandle, NULL, 0))
    {
        HANDLE threadHandle = NULL;
        STARTUPINFO startupInfo = { sizeof(startupInfo) };
        OBJECT_HANDLE_FLAG_INFORMATION flagInfo;
        PPH_STRING command = NULL;

        startupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        startupInfo.hStdOutput = pipeWriteHandle;
        startupInfo.hStdError = pipeWriteHandle;
        startupInfo.wShowWindow = SW_HIDE;

        switch (context->Action)
        {
        case NETWORK_ACTION_TRACEROUTE:
            {
                if (PhGetIntegerSetting(L"EnableNetworkResolve"))
                {
                    command = PhFormatString(
                        L"%s\\system32\\tracert.exe %s",
                        USER_SHARED_DATA->NtSystemRoot,
                        context->IpAddressString
                        );
                }
                else
                {
                    // Disable hostname lookup.
                    command = PhFormatString(
                        L"%s\\system32\\tracert.exe -d %s",
                        USER_SHARED_DATA->NtSystemRoot,
                        context->IpAddressString
                        );
                }
            }
            break;
        case NETWORK_ACTION_PATHPING:
            {
                if (PhGetIntegerSetting(L"EnableNetworkResolve"))
                {
                    command = PhFormatString(
                        L"%s\\system32\\pathping.exe %s",
                        USER_SHARED_DATA->NtSystemRoot,
                        context->IpAddressString
                        );
                }
                else
                {
                    // Disable hostname lookup.
                    command = PhFormatString(
                        L"%s\\system32\\pathping.exe -n %s",
                        USER_SHARED_DATA->NtSystemRoot,
                        context->IpAddressString
                        );
                }
            }
            break;
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
        if (threadHandle = PhCreateThread(0, StdOutNetworkTracertThreadStart, context))
            NtClose(threadHandle);
    }

    return STATUS_SUCCESS;
}