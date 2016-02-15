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

#include "extsrv.h"

typedef struct _RESTART_SERVICE_CONTEXT
{
    PPH_SERVICE_ITEM ServiceItem;
    SC_HANDLE ServiceHandle;
    BOOLEAN Starting;
    BOOLEAN DisableTimer;
} RESTART_SERVICE_CONTEXT, *PRESTART_SERVICE_CONTEXT;

INT_PTR CALLBACK EspRestartServiceDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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
            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            // TODO: Use the progress information.
            PhSetWindowStyle(GetDlgItem(hwndDlg, IDC_PROGRESS), PBS_MARQUEE, PBS_MARQUEE);
            SendMessage(GetDlgItem(hwndDlg, IDC_PROGRESS), PBM_SETMARQUEE, TRUE, 75);

            SetDlgItemText(hwndDlg, IDC_MESSAGE, PhaFormatString(L"Attempting to stop %s...", context->ServiceItem->Name->Buffer)->Buffer);

            if (PhUiStopService(hwndDlg, context->ServiceItem))
            {
                SetTimer(hwndDlg, 1, 250, NULL);
            }
            else
            {
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
            if (wParam == 1 && !context->DisableTimer)
            {
                SERVICE_STATUS serviceStatus;

                if (QueryServiceStatus(context->ServiceHandle, &serviceStatus))
                {
                    if (!context->Starting && serviceStatus.dwCurrentState == SERVICE_STOPPED)
                    {
                        // The service is stopped, so start the service now.

                        SetDlgItemText(hwndDlg, IDC_MESSAGE,
                            PhaFormatString(L"Attempting to start %s...", context->ServiceItem->Name->Buffer)->Buffer);
                        context->DisableTimer = TRUE;

                        if (PhUiStartService(hwndDlg, context->ServiceItem))
                        {
                            context->DisableTimer = FALSE;
                            context->Starting = TRUE;
                        }
                        else
                        {
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

VOID EsRestartServiceWithProgress(
    _In_ HWND hWnd,
    _In_ PPH_SERVICE_ITEM ServiceItem,
    _In_ SC_HANDLE ServiceHandle
    )
{
    RESTART_SERVICE_CONTEXT context;

    context.ServiceItem = ServiceItem;
    context.ServiceHandle = ServiceHandle;
    context.Starting = FALSE;
    context.DisableTimer = FALSE;

    DialogBoxParam(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_SRVPROGRESS),
        hWnd,
        EspRestartServiceDlgProc,
        (LPARAM)&context
        );
}