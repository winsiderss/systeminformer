/*
 * Process Hacker Extended Services - 
 *   progress dialog
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
#include <windowsx.h>
#include "extsrv.h"
#include "resource.h"

typedef struct _RESTART_SERVICE_CONTEXT
{
    PWSTR ServiceName;
    SC_HANDLE ServiceHandle;
    BOOLEAN Starting;
} RESTART_SERVICE_CONTEXT, *PRESTART_SERVICE_CONTEXT;

INT_PTR CALLBACK EspRestartServiceDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID EsRestartServiceWithProgress(
    __in HWND hWnd,
    __in PWSTR ServiceName,
    __in SC_HANDLE ServiceHandle
    )
{
    RESTART_SERVICE_CONTEXT context;

    context.ServiceName = ServiceName;
    context.ServiceHandle = ServiceHandle;
    context.Starting = FALSE;

    DialogBoxParam(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_SRVPROGRESS),
        hWnd,
        EspRestartServiceDlgProc,
        (LPARAM)&context
        );
}

INT_PTR CALLBACK EspRestartServiceDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PRESTART_SERVICE_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PRESTART_SERVICE_CONTEXT)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PRESTART_SERVICE_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
            RemoveProp(hwndDlg, L"Context");
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            SERVICE_STATUS serviceStatus;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            // TODO: Use the progress information.
            PhSetWindowStyle(GetDlgItem(hwndDlg, IDC_PROGRESS), PBS_MARQUEE, PBS_MARQUEE);
            SendMessage(GetDlgItem(hwndDlg, IDC_PROGRESS), PBM_SETMARQUEE, TRUE, 75);

            SetDlgItemText(hwndDlg, IDC_MESSAGE, PhaFormatString(L"Attempting to stop %s...", context->ServiceName)->Buffer);

            if (ControlService(context->ServiceHandle, SERVICE_CONTROL_STOP, &serviceStatus))
            {
                SetTimer(hwndDlg, 1, 250, NULL);
            }
            else
            {
                PhShowStatus(
                    hwndDlg,
                    PhaFormatString(L"Unable to stop %s", context->ServiceName)->Buffer,
                    0,
                    GetLastError()
                    );
                EndDialog(hwndDlg, IDCANCEL);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                {
                    EndDialog(hwndDlg, IDCANCEL);
                }
                break;
            }
        }
        break;
    case WM_TIMER:
        {
            if (wParam == 1)
            {
                SERVICE_STATUS serviceStatus;

                if (QueryServiceStatus(context->ServiceHandle, &serviceStatus))
                {
                    if (!context->Starting && serviceStatus.dwCurrentState == SERVICE_STOPPED)
                    {
                        // The service is stopped, so start the service now.

                        SetDlgItemText(hwndDlg, IDC_MESSAGE,
                            PhaFormatString(L"Attempting to start %s...", context->ServiceName)->Buffer);

                        if (StartService(context->ServiceHandle, 0, NULL))
                        {
                            context->Starting = TRUE;
                        }
                        else
                        {
                            PhShowStatus(
                                hwndDlg,
                                PhaFormatString(L"Unable to start %s", context->ServiceName)->Buffer,
                                0,
                                GetLastError()
                                );
                            EndDialog(hwndDlg, IDCANCEL);
                        }
                    }
                    else if (context->Starting && serviceStatus.dwCurrentState == SERVICE_RUNNING)
                    {
                        EndDialog(hwndDlg, IDOK);
                    }
                }
            }
        }
        break;
    }

    return FALSE;
}
