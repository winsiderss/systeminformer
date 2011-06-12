/*
 * Process Hacker Extended Services - 
 *   other information
 * 
 * Copyright (C) 2010-2011 wj32
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

typedef struct _SERVICE_OTHER_CONTEXT
{
    PPH_SERVICE_ITEM ServiceItem;

    HWND PrivilegesLv;
    HWND TriggersLv;
    struct _ES_TRIGGER_CONTEXT *TriggerContext;
} SERVICE_OTHER_CONTEXT, *PSERVICE_OTHER_CONTEXT;

NTSTATUS EspLoadOtherInfo(
    __in HWND hwndDlg,
    __in PSERVICE_OTHER_CONTEXT Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SC_HANDLE serviceHandle;
    SERVICE_PRESHUTDOWN_INFO preshutdownInfo;
    LPSERVICE_REQUIRED_PRIVILEGES_INFO requiredPrivileges;
    ULONG returnLength;

    if (!(serviceHandle = PhOpenService(Context->ServiceItem->Name->Buffer, SERVICE_QUERY_CONFIG)))
        return NTSTATUS_FROM_WIN32(GetLastError());

    // Preshutdown timeout

    if (QueryServiceConfig2(serviceHandle,
        SERVICE_CONFIG_PRESHUTDOWN_INFO,
        (PBYTE)&preshutdownInfo,
        sizeof(SERVICE_PRESHUTDOWN_INFO),
        &returnLength
        ))
    {
        SetDlgItemInt(hwndDlg, IDC_PRESHUTDOWNTIMEOUT, preshutdownInfo.dwPreshutdownTimeout, FALSE);
    }

    // Required privileges

    if (requiredPrivileges = PhQueryServiceVariableSize(serviceHandle, SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO))
    {
        PWSTR privilege;
        ULONG privilegeLength;
        INT lvItemIndex;
        PH_STRINGREF privilegeSr;
        PPH_STRING displayName;

        privilege = requiredPrivileges->pmszRequiredPrivileges;

        if (privilege)
        {
            while (TRUE)
            {
                privilegeLength = (ULONG)wcslen(privilege);

                if (privilegeLength == 0)
                    break;

                lvItemIndex = PhAddListViewItem(Context->PrivilegesLv, MAXINT, privilege, NULL);
                privilegeSr.Buffer = privilege;
                privilegeSr.Length = (USHORT)(privilegeLength * sizeof(WCHAR));

                if (PhLookupPrivilegeDisplayName(&privilegeSr, &displayName))
                {
                    PhSetListViewSubItem(Context->PrivilegesLv, lvItemIndex, 1, displayName->Buffer);
                    PhDereferenceObject(displayName);
                }

                privilege += privilegeLength + 1;
            }
        }

        ExtendedListView_SortItems(Context->PrivilegesLv);

        PhFree(requiredPrivileges);
    }

    // Triggers

    if (Context->TriggerContext)
    {
        EsLoadServiceTriggerInfo(Context->TriggerContext, serviceHandle);
    }

    CloseServiceHandle(serviceHandle);

    return status;
}

INT_PTR CALLBACK EspServiceOtherDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PSERVICE_OTHER_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocate(sizeof(SERVICE_OTHER_CONTEXT));
        memset(context, 0, sizeof(SERVICE_OTHER_CONTEXT));

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PSERVICE_OTHER_CONTEXT)GetProp(hwndDlg, L"Context");

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
            HWND privilegesLv;
            HWND triggersLv;

            context->ServiceItem = serviceItem;

            context->PrivilegesLv = privilegesLv = GetDlgItem(hwndDlg, IDC_PRIVILEGES);
            PhSetListViewStyle(privilegesLv, FALSE, TRUE);
            PhSetControlTheme(privilegesLv, L"explorer");
            PhAddListViewColumn(privilegesLv, 0, 0, 0, LVCFMT_LEFT, 140, L"Name");
            PhAddListViewColumn(privilegesLv, 1, 1, 1, LVCFMT_LEFT, 220, L"Display Name");
            PhSetExtendedListView(privilegesLv);

            context->TriggersLv = triggersLv = GetDlgItem(hwndDlg, IDC_TRIGGERS);

            if (WindowsVersion >= WINDOWS_7)
            {
                context->TriggerContext = EsCreateServiceTriggerContext(
                    context->ServiceItem,
                    hwndDlg,
                    triggersLv
                    );
            }
            else
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_TRIGGERS_LABEL), FALSE);
                EnableWindow(triggersLv, FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_NEW), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_EDIT), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, IDC_DELETE), FALSE);
            }

            status = EspLoadOtherInfo(hwndDlg, context);

            if (!NT_SUCCESS(status))
            {
                PhShowWarning(hwndDlg, L"Unable to query service information: %s",
                    ((PPH_STRING)PHA_DEREFERENCE(PhGetNtMessage(status)))->Buffer);
            }
        }
        break;
    case WM_DESTROY:
        {
            if (context->TriggerContext)
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
                    ULONG win32Result;

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

                    if (context->TriggerContext)
                    {
                        if (!EsSaveServiceTriggerInfo(context->TriggerContext, &win32Result))
                        {
                            if (PhShowMessage(
                                hwndDlg,
                                MB_ICONERROR | MB_RETRYCANCEL,
                                L"Unable to change service trigger information: %s",
                                ((PPH_STRING)PHA_DEREFERENCE(PhGetWin32Message(win32Result)))->Buffer
                                ) == IDRETRY)
                            {
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                            }
                        }
                    }
                }
                return TRUE;
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
