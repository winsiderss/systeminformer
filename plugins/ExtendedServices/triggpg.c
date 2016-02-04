/*
 * Process Hacker Extended Services -
 *   triggers page
 *
 * Copyright (C) 2015 wj32
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

typedef struct _SERVICE_TRIGGERS_CONTEXT
{
    PPH_SERVICE_ITEM ServiceItem;
    HWND TriggersLv;
    struct _ES_TRIGGER_CONTEXT *TriggerContext;
} SERVICE_TRIGGERS_CONTEXT, *PSERVICE_TRIGGERS_CONTEXT;

NTSTATUS EspLoadTriggerInfo(
    _In_ HWND hwndDlg,
    _In_ PSERVICE_TRIGGERS_CONTEXT Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SC_HANDLE serviceHandle;

    if (!(serviceHandle = PhOpenService(Context->ServiceItem->Name->Buffer, SERVICE_QUERY_CONFIG)))
        return NTSTATUS_FROM_WIN32(GetLastError());

    EsLoadServiceTriggerInfo(Context->TriggerContext, serviceHandle);
    CloseServiceHandle(serviceHandle);

    return status;
}

INT_PTR CALLBACK EspServiceTriggersDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PSERVICE_TRIGGERS_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocate(sizeof(SERVICE_TRIGGERS_CONTEXT));
        memset(context, 0, sizeof(SERVICE_TRIGGERS_CONTEXT));

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PSERVICE_TRIGGERS_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
            RemoveProp(hwndDlg, L"Context");
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            NTSTATUS status;
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)propSheetPage->lParam;
            HWND triggersLv;

            context->ServiceItem = serviceItem;
            context->TriggersLv = triggersLv = GetDlgItem(hwndDlg, IDC_TRIGGERS);
            context->TriggerContext = EsCreateServiceTriggerContext(
                context->ServiceItem,
                hwndDlg,
                triggersLv
                );

            status = EspLoadTriggerInfo(hwndDlg, context);

            if (!NT_SUCCESS(status))
            {
                PhShowWarning(hwndDlg, L"Unable to query service trigger information: %s",
                    ((PPH_STRING)PhAutoDereferenceObject(PhGetNtMessage(status)))->Buffer);
            }
        }
        break;
    case WM_DESTROY:
        {
            EsDestroyServiceTriggerContext(context->TriggerContext);
            PhFree(context);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_NEW:
                if (context->TriggerContext)
                    EsHandleEventServiceTrigger(context->TriggerContext, ES_TRIGGER_EVENT_NEW);
                break;
            case IDC_EDIT:
                if (context->TriggerContext)
                    EsHandleEventServiceTrigger(context->TriggerContext, ES_TRIGGER_EVENT_EDIT);
                break;
            case IDC_DELETE:
                if (context->TriggerContext)
                    EsHandleEventServiceTrigger(context->TriggerContext, ES_TRIGGER_EVENT_DELETE);
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_KILLACTIVE:
                {
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
                }
                return TRUE;
            case PSN_APPLY:
                {
                    ULONG win32Result = 0;

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

                    if (!EsSaveServiceTriggerInfo(context->TriggerContext, &win32Result))
                    {
                        if (win32Result == ERROR_CANCELLED || (PhShowMessage(
                            hwndDlg,
                            MB_ICONERROR | MB_RETRYCANCEL,
                            L"Unable to change service trigger information: %s",
                            ((PPH_STRING)PhAutoDereferenceObject(PhGetWin32Message(win32Result)))->Buffer
                            ) == IDRETRY))
                        {
                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                        }
                    }

                    return TRUE;
                }
                break;
            case LVN_ITEMCHANGED:
                {
                    if (header->hwndFrom == context->TriggersLv && context->TriggerContext)
                    {
                        EsHandleEventServiceTrigger(context->TriggerContext, ES_TRIGGER_EVENT_SELECTIONCHANGED);
                    }
                }
                break;
            case NM_DBLCLK:
                {
                    if (header->hwndFrom == context->TriggersLv && context->TriggerContext)
                    {
                        EsHandleEventServiceTrigger(context->TriggerContext, ES_TRIGGER_EVENT_EDIT);
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}
