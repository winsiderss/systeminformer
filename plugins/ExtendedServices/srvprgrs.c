/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2016-2023
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
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PRESTART_SERVICE_CONTEXT context;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = (PRESTART_SERVICE_CONTEXT)lParam;
        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            PhCenterWindow(WindowHandle, GetParent(WindowHandle));

            // TODO: Use the progress information.
            PhSetWindowStyle(GetDlgItem(WindowHandle, IDC_PROGRESS), PBS_MARQUEE, PBS_MARQUEE);
            SendMessage(GetDlgItem(WindowHandle, IDC_PROGRESS), PBM_SETMARQUEE, TRUE, 75);

            PhSetDialogItemText(WindowHandle, IDC_MESSAGE, PhaFormatString(L"Attempting to stop %s...", context->ServiceItem->Name->Buffer)->Buffer);

            if (PhUiStopService(WindowHandle, context->ServiceItem))
            {
                PhSetTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT, 250, NULL);
            }
            else
            {
                EndDialog(WindowHandle, IDCANCEL);
            }

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);

            PhKillTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
                {
                    PhKillTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT);

                    EndDialog(WindowHandle, IDCANCEL);
                }
                break;
            }
        }
        break;
    case WM_TIMER:
        {
            if (wParam == PH_WINDOW_TIMER_DEFAULT && !context->DisableTimer)
            {
                SERVICE_STATUS_PROCESS serviceStatus;

                if (NT_SUCCESS(PhQueryServiceStatus(context->ServiceHandle, &serviceStatus)))
                {
                    if (!context->Starting && serviceStatus.dwCurrentState == SERVICE_STOPPED)
                    {
                        // The service is stopped, so start the service now.

                        PhSetDialogItemText(WindowHandle, IDC_MESSAGE,
                            PhaFormatString(L"Attempting to start %s...", context->ServiceItem->Name->Buffer)->Buffer);
                        context->DisableTimer = TRUE;

                        if (PhUiStartService(WindowHandle, context->ServiceItem))
                        {
                            context->DisableTimer = FALSE;
                            context->Starting = TRUE;
                        }
                        else
                        {
                            EndDialog(WindowHandle, IDCANCEL);
                        }
                    }
                    else if (context->Starting && serviceStatus.dwCurrentState == SERVICE_RUNNING)
                    {
                        EndDialog(WindowHandle, IDOK);
                    }
                }
            }
        }
        break;
    }

    return FALSE;
}

VOID EsRestartServiceWithProgress(
    _In_ HWND ParentWindowHandle,
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
        ParentWindowHandle,
        EspRestartServiceDlgProc,
        &context
        );
}
