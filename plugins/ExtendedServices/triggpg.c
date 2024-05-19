/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2015
 *     dmex    2016-2023
 *
 */

#include "extsrv.h"

typedef struct _SERVICE_TRIGGERS_CONTEXT
{
    PPH_SERVICE_ITEM ServiceItem;
    HWND TriggersLv;
    PH_LAYOUT_MANAGER LayoutManager;
    struct _ES_TRIGGER_CONTEXT *TriggerContext;
} SERVICE_TRIGGERS_CONTEXT, *PSERVICE_TRIGGERS_CONTEXT;

NTSTATUS EspLoadTriggerInfo(
    _In_ HWND WindowHandle,
    _In_ PSERVICE_TRIGGERS_CONTEXT Context
    )
{
    NTSTATUS status;
    SC_HANDLE serviceHandle;

    status = PhOpenService(
        &serviceHandle,
        SERVICE_QUERY_CONFIG,
        PhGetString(Context->ServiceItem->Name)
        );

    if (NT_SUCCESS(status))
    {
        EsLoadServiceTriggerInfo(Context->TriggerContext, serviceHandle);

        PhCloseServiceHandle(serviceHandle);
    }

    return status;
}

INT_PTR CALLBACK EspServiceTriggersDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PSERVICE_TRIGGERS_CONTEXT context;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = PhAllocate(sizeof(SERVICE_TRIGGERS_CONTEXT));
        memset(context, 0, sizeof(SERVICE_TRIGGERS_CONTEXT));

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
            NTSTATUS status;
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)propSheetPage->lParam;
            HWND triggersLv;

            context->ServiceItem = serviceItem;
            context->TriggersLv = triggersLv = GetDlgItem(WindowHandle, IDC_TRIGGERS);
            context->TriggerContext = EsCreateServiceTriggerContext(
                context->ServiceItem,
                WindowHandle,
                triggersLv
                );

            status = EspLoadTriggerInfo(WindowHandle, context);

            if (!NT_SUCCESS(status))
            {
                PPH_STRING errorMessage = PhGetNtMessage(status);

                PhShowWarning(
                    WindowHandle,
                    L"Unable to query service trigger information: %s",
                    PhGetStringOrDefault(errorMessage, L"Unknown error.")
                    );

                PhClearReference(&errorMessage);
            }

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_TRIGGERS), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_NEW), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_EDIT), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_DELETE), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);

            PhDeleteLayoutManager(&context->LayoutManager);

            EsDestroyServiceTriggerContext(context->TriggerContext);
            PhFree(context);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
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
                    SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, FALSE);
                }
                return TRUE;
            case PSN_APPLY:
                {
                    NTSTATUS status = STATUS_SUCCESS;

                    SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, PSNRET_NOERROR);

                    if (!EsSaveServiceTriggerInfo(context->TriggerContext, &status))
                    {
                        if (status == STATUS_CANCELLED)
                        {
                            SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, PSNRET_INVALID);
                        }
                        else
                        {
                            PPH_STRING errorMessage = PhGetNtMessage(status);

                            if (PhShowMessage(
                                WindowHandle,
                                MB_ICONERROR | MB_RETRYCANCEL,
                                L"Unable to change service trigger information: %s",
                                PhGetStringOrDefault(errorMessage, L"Unknown error.")
                                ) == IDRETRY)
                            {
                                SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, PSNRET_INVALID);
                            }

                            PhClearReference(&errorMessage);
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
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    }

    return FALSE;
}
