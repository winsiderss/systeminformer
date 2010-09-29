/*
 * Process Hacker Network Tools - 
 *   output dialog
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

#include <phdk.h>
#include "nettools.h"
#include "resource.h"

#define NTM_DONE (WM_APP + 1)
#define NTM_RECEIVED (WM_APP + 2)

typedef struct _NETWORK_OUTPUT_CONTEXT
{
    ULONG Action;
    PH_IP_ADDRESS Address;
    HWND WindowHandle;
    PH_QUEUED_LOCK WindowHandleLock;

    HANDLE ThreadHandle;
    HANDLE PipeReadHandle;
    HANDLE ProcessHandle;

    PPH_FULL_STRING ReceivedString;
} NETWORK_OUTPUT_CONTEXT, *PNETWORK_OUTPUT_CONTEXT;

INT_PTR CALLBACK NetworkOutputDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID PerformNetworkAction(
    __in HWND hWnd,
    __in ULONG Action,
    __in PPH_IP_ADDRESS Address
    )
{
    NETWORK_OUTPUT_CONTEXT context;

    memset(&context, 0, sizeof(NETWORK_OUTPUT_CONTEXT));
    context.Action = Action;
    context.Address = *Address;
    PhInitializeQueuedLock(&context.WindowHandleLock);
    context.ReceivedString = PhCreateFullString2(PAGE_SIZE);

    DialogBoxParam(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OUTPUT),
        hWnd,
        NetworkOutputDlgProc,
        (LPARAM)&context
        );

    if (context.ThreadHandle)
    {
        // Wait for our thread to exit since it uses the context structure.
        NtWaitForSingleObject(context.ThreadHandle, FALSE, NULL);
        NtClose(context.ThreadHandle);
    }

    PhDereferenceObject(context.ReceivedString);
}

static NTSTATUS NetworkWorkerThreadStart(
    __in PVOID Parameter
    )
{
    NTSTATUS status;
    PNETWORK_OUTPUT_CONTEXT context = Parameter;
    IO_STATUS_BLOCK isb;
    UCHAR buffer[4096];

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

        PhAcquireQueuedLockExclusive(&context->WindowHandleLock);

        if (!context->WindowHandle)
        {
            PhReleaseQueuedLockExclusive(&context->WindowHandleLock);
            return STATUS_SUCCESS;
        }

        if (!NT_SUCCESS(status))
        {
            SendMessage(context->WindowHandle, NTM_DONE, 0, 0);
            PhReleaseQueuedLockExclusive(&context->WindowHandleLock);
            return STATUS_SUCCESS;
        }

        SendMessage(context->WindowHandle, NTM_RECEIVED, (WPARAM)isb.Information, (LPARAM)buffer);
        PhReleaseQueuedLockExclusive(&context->WindowHandleLock);
    }

    NtClose(context->PipeReadHandle);

    return STATUS_SUCCESS;
}

INT_PTR CALLBACK NetworkOutputDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PNETWORK_OUTPUT_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PNETWORK_OUTPUT_CONTEXT)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PNETWORK_OUTPUT_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
            RemoveProp(hwndDlg, L"Context");
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            WCHAR addressString[65];
            HANDLE pipeWriteHandle;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));
            context->WindowHandle = hwndDlg;

            if (context->Address.Type == PH_IPV4_NETWORK_TYPE)
                RtlIpv4AddressToString(&context->Address.InAddr, addressString);
            else
                RtlIpv6AddressToString(&context->Address.In6Addr, addressString);

            switch (context->Action)
            {
            case NETWORK_ACTION_PING:
            case NETWORK_ACTION_TRACEROUTE:
                if (context->Action == NETWORK_ACTION_PING)
                {
                    SetDlgItemText(hwndDlg, IDC_MESSAGE,
                        PhaFormatString(L"Pinging %s...", addressString)->Buffer);
                }
                else
                {
                    SetDlgItemText(hwndDlg, IDC_MESSAGE,
                        PhaFormatString(L"Tracing route to %s...", addressString)->Buffer);
                }

                // Doing this properly would be too complex, so we'll just 
                // execute ping.exe/traceroute.exe and display its output.

                if (CreatePipe(&context->PipeReadHandle, &pipeWriteHandle, NULL, 0))
                {
                    STARTUPINFO startupInfo = { sizeof(startupInfo) };
                    PPH_STRING command;
                    OBJECT_HANDLE_FLAG_INFORMATION flagInfo;

                    startupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
                    startupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
                    startupInfo.hStdOutput = pipeWriteHandle;
                    startupInfo.hStdError = pipeWriteHandle;
                    startupInfo.wShowWindow = SW_HIDE;

                    if (context->Action == NETWORK_ACTION_PING)
                    {
                        command = PhaFormatString(
                            L"%s\\system32\\ping.exe %s",
                            USER_SHARED_DATA->NtSystemRoot,
                            addressString
                            );
                    }
                    else
                    {
                        command = PhaFormatString(
                            L"%s\\system32\\tracert.exe %s",
                            USER_SHARED_DATA->NtSystemRoot,
                            addressString
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

                    // Essential; when the process exits, the last instance of the pipe 
                    // will be disconnected and our thread will exit.
                    NtClose(pipeWriteHandle);

                    // Create a thread which will wait for output and display it.
                    context->ThreadHandle = PhCreateThread(0, NetworkWorkerThreadStart, context);
                }

                break;
            }
        }
        break;
    case WM_DESTROY:
        {
            PhAcquireQueuedLockExclusive(&context->WindowHandleLock);
            context->WindowHandle = NULL;
            PhReleaseQueuedLockExclusive(&context->WindowHandleLock);

            if (context->ProcessHandle)
            {
                NtTerminateProcess(context->ProcessHandle, STATUS_SUCCESS);
                NtClose(context->ProcessHandle);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    case NTM_DONE:
        {
            SetDlgItemText(hwndDlg, IDC_MESSAGE, L"Finished.");
        }
        break;
    case NTM_RECEIVED:
        {
            PPH_STRING convertedString;

            if (wParam != 0)
            {
                convertedString = PhCreateStringFromAnsiEx((PSTR)lParam, (SIZE_T)wParam);
                PhAppendFullString(context->ReceivedString, convertedString);
                PhDereferenceObject(convertedString);

                // Remove leading newlines.
                if (
                    context->ReceivedString->Length >= 2 * 2 &&
                    context->ReceivedString->Buffer[0] == '\r' && context->ReceivedString->Buffer[1] == '\n'
                    )
                {
                    PhRemoveFullString(context->ReceivedString, 0, 2);
                }

                SetDlgItemText(hwndDlg, IDC_TEXT, context->ReceivedString->Buffer);
                SendMessage(
                    GetDlgItem(hwndDlg, IDC_TEXT),
                    EM_SETSEL,
                    context->ReceivedString->Length / 2 - 1,
                    context->ReceivedString->Length / 2 - 1
                    );
                SendMessage(GetDlgItem(hwndDlg, IDC_TEXT), WM_VSCROLL, SB_BOTTOM, 0);
            }
        }
        break;
    }

    return FALSE;
}
