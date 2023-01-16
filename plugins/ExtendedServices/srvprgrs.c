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
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_DESTROY)
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
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

            PhSetDialogItemText(hwndDlg, IDC_MESSAGE, PhaFormatString(L"Attempting to stop %s...", context->ServiceItem->Name->Buffer)->Buffer);

            if (PhUiStopService(hwndDlg, context->ServiceItem))
            {
                SetTimer(hwndDlg, 1, 250, NULL);
            }
            else
            {
                EndDialog(hwndDlg, IDCANCEL);
            }

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                {
                    KillTimer(hwndDlg, 1);

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

                        PhSetDialogItemText(hwndDlg, IDC_MESSAGE,
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

    PhDialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_SRVPROGRESS),
        hWnd,
        EspRestartServiceDlgProc,
        &context
        );
}