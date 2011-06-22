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

    BOOLEAN Ready;
    BOOLEAN Dirty;
    BOOLEAN PreshutdownTimeoutValid;
    BOOLEAN RequiredPrivilegesValid;
    HWND PrivilegesLv;
    PPH_LIST PrivilegeList;
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
    LPSERVICE_REQUIRED_PRIVILEGES_INFO requiredPrivilegesInfo;
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
        Context->PreshutdownTimeoutValid = TRUE;
    }

    // Required privileges

    if (requiredPrivilegesInfo = PhQueryServiceVariableSize(serviceHandle, SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO))
    {
        PWSTR privilege;
        ULONG privilegeLength;
        INT lvItemIndex;
        PH_STRINGREF privilegeSr;
        PPH_STRING privilegeString;
        PPH_STRING displayName;

        privilege = requiredPrivilegesInfo->pmszRequiredPrivileges;

        if (privilege)
        {
            while (TRUE)
            {
                privilegeLength = (ULONG)wcslen(privilege);

                if (privilegeLength == 0)
                    break;

                privilegeString = PhCreateStringEx(privilege, privilegeLength * sizeof(WCHAR));
                PhAddItemList(Context->PrivilegeList, privilegeString);

                lvItemIndex = PhAddListViewItem(Context->PrivilegesLv, MAXINT, privilege, privilegeString);
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

        PhFree(requiredPrivilegesInfo);
        Context->RequiredPrivilegesValid = TRUE;
    }

    // Triggers

    if (Context->TriggerContext)
    {
        EsLoadServiceTriggerInfo(Context->TriggerContext, serviceHandle);
    }

    CloseServiceHandle(serviceHandle);

    return status;
}

static int __cdecl PrivilegeNameCompareFunction(
    __in const void *elem1,
    __in const void *elem2
    )
{
    PWSTR string1 = *(PWSTR *)elem1;
    PWSTR string2 = *(PWSTR *)elem2;

    return wcscmp(string1, string2);
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

            context->PrivilegeList = PhCreateList(32);

            if (context->ServiceItem->Type == SERVICE_KERNEL_DRIVER || context->ServiceItem->Type == SERVICE_FILE_SYSTEM_DRIVER)
            {
                // Drivers don't support required privileges.
                EnableWindow(GetDlgItem(hwndDlg, IDC_ADD), FALSE);
            }

            EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVE), FALSE);

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

            context->Ready = TRUE;
        }
        break;
    case WM_DESTROY:
        {
            if (context->PrivilegeList)
            {
                PhDereferenceObjects(context->PrivilegeList->Items, context->PrivilegeList->Count);
                PhDereferenceObject(context->PrivilegeList);
            }

            if (context->TriggerContext)
                EsDestroyServiceTriggerContext(context->TriggerContext);

            PhFree(context);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_ADD:
                {
                    NTSTATUS status;
                    LSA_HANDLE policyHandle;
                    LSA_ENUMERATION_HANDLE enumContext;
                    PPOLICY_PRIVILEGE_DEFINITION buffer;
                    ULONG count;
                    ULONG i;
                    PPH_LIST choices;
                    PPH_STRING selectedChoice = NULL;

                    choices = PhCreateList(100);

                    if (!NT_SUCCESS(status = PhOpenLsaPolicy(&policyHandle, POLICY_VIEW_LOCAL_INFORMATION, NULL)))
                    {
                        PhShowStatus(hwndDlg, L"Unable to open LSA policy", status, 0);
                        break;
                    }

                    enumContext = 0;

                    while (TRUE)
                    {
                        status = LsaEnumeratePrivileges(
                            policyHandle,
                            &enumContext,
                            &buffer,
                            0x100,
                            &count
                            );

                        if (status == STATUS_NO_MORE_ENTRIES)
                            break;
                        if (!NT_SUCCESS(status))
                            break;

                        for (i = 0; i < count; i++)
                        {
                            PhAddItemList(choices, PhaCreateStringEx(buffer[i].Name.Buffer, buffer[i].Name.Length)->Buffer);
                        }
                    }

                    LsaClose(policyHandle);

                    qsort(choices->Items, choices->Count, sizeof(PWSTR), PrivilegeNameCompareFunction);

                    while (PhaChoiceDialog(
                        hwndDlg,
                        L"Add privilege",
                        L"Select a privilege to add:",
                        (PWSTR *)choices->Items,
                        choices->Count,
                        NULL,
                        PH_CHOICE_DIALOG_CHOICE,
                        &selectedChoice,
                        NULL,
                        NULL
                        ))
                    {
                        BOOLEAN found = FALSE;
                        PPH_STRING privilegeString;
                        INT lvItemIndex;
                        PPH_STRING displayName;

                        // Check for duplicates.
                        for (i = 0; i < context->PrivilegeList->Count; i++)
                        {
                            if (PhEqualString(context->PrivilegeList->Items[i], selectedChoice, FALSE))
                            {
                                found = TRUE;
                                break;
                            }
                        }

                        if (found)
                        {
                            if (PhShowMessage(
                                hwndDlg,
                                MB_OKCANCEL | MB_ICONERROR,
                                L"The selected privilege has already been added."
                                ) == IDOK)
                            {
                                continue;
                            }
                            else
                            {
                                break;
                            }
                        }

                        privilegeString = selectedChoice;
                        PhReferenceObject(privilegeString);

                        PhAddItemList(context->PrivilegeList, privilegeString);

                        lvItemIndex = PhAddListViewItem(context->PrivilegesLv, MAXINT, privilegeString->Buffer, privilegeString);

                        if (PhLookupPrivilegeDisplayName(&privilegeString->sr, &displayName))
                        {
                            PhSetListViewSubItem(context->PrivilegesLv, lvItemIndex, 1, displayName->Buffer);
                            PhDereferenceObject(displayName);
                        }

                        ExtendedListView_SortItems(context->PrivilegesLv);

                        context->Dirty = TRUE;
                        context->RequiredPrivilegesValid = TRUE;

                        break;
                    }

                    PhDereferenceObject(choices);
                }
                break;
            case IDC_REMOVE:
                {
                    INT lvItemIndex;
                    PPH_STRING privilegeString;
                    ULONG index;

                    lvItemIndex = ListView_GetNextItem(context->PrivilegesLv, -1, LVNI_SELECTED);

                    if (lvItemIndex != -1 && PhGetListViewItemParam(context->PrivilegesLv, lvItemIndex, (PPVOID)&privilegeString))
                    {
                        index = PhFindItemList(context->PrivilegeList, privilegeString);

                        if (index != -1)
                        {
                            PhDereferenceObject(privilegeString);
                            PhRemoveItemList(context->PrivilegeList, index);
                            PhRemoveListViewItem(context->PrivilegesLv, lvItemIndex);

                            context->Dirty = TRUE;
                            context->RequiredPrivilegesValid = TRUE;
                        }
                    }
                }
                break;
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

            switch (HIWORD(wParam))
            {
            case EN_CHANGE:
                {
                    if (context->Ready)
                    {
                        context->Dirty = TRUE;
                        context->PreshutdownTimeoutValid = TRUE;
                    }
                }
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
                    SC_HANDLE serviceHandle;
                    ULONG win32Result = 0;

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

                    if (context->Dirty)
                    {
                        if (serviceHandle = PhOpenService(context->ServiceItem->Name->Buffer, SERVICE_CHANGE_CONFIG))
                        {
                            SERVICE_PRESHUTDOWN_INFO preshutdownInfo;
                            SERVICE_REQUIRED_PRIVILEGES_INFO requiredPrivilegesInfo;

                            if (context->PreshutdownTimeoutValid)
                            {
                                preshutdownInfo.dwPreshutdownTimeout = GetDlgItemInt(hwndDlg, IDC_PRESHUTDOWNTIMEOUT, NULL, FALSE);

                                if (!ChangeServiceConfig2(serviceHandle, SERVICE_CONFIG_PRESHUTDOWN_INFO, &preshutdownInfo))
                                {
                                    win32Result = GetLastError();
                                }
                            }

                            if (context->RequiredPrivilegesValid)
                            {
                                PH_STRING_BUILDER sb;
                                ULONG i;

                                PhInitializeStringBuilder(&sb, 100);

                                for (i = 0; i < context->PrivilegeList->Count; i++)
                                {
                                    PhAppendStringBuilder(&sb, context->PrivilegeList->Items[i]);
                                    PhAppendCharStringBuilder(&sb, 0);
                                }

                                requiredPrivilegesInfo.pmszRequiredPrivileges = sb.String->Buffer;

                                if (win32Result == 0 && !ChangeServiceConfig2(serviceHandle, SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO, &requiredPrivilegesInfo))
                                {
                                    win32Result = GetLastError();
                                }

                                PhDeleteStringBuilder(&sb);
                            }

                            CloseServiceHandle(serviceHandle);
                        }
                        else
                        {
                            win32Result = GetLastError();
                        }

                        if (win32Result != 0)
                        {
                            if (PhShowMessage(
                                hwndDlg,
                                MB_ICONERROR | MB_RETRYCANCEL,
                                L"Unable to change service information: %s",
                                ((PPH_STRING)PHA_DEREFERENCE(PhGetWin32Message(win32Result)))->Buffer
                                ) == IDRETRY)
                            {
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                                goto EndOfSave;
                            }
                        }
                    }

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
                                goto EndOfSave;
                            }
                        }
                    }

EndOfSave:
                    return TRUE;
                }
                break;
            case LVN_ITEMCHANGED:
                {
                    if (header->hwndFrom == context->TriggersLv && context->TriggerContext)
                    {
                        EsHandleEventServiceTrigger(context->TriggerContext, ES_TRIGGER_EVENT_SELECTIONCHANGED);
                    }
                    else if (header->hwndFrom == context->PrivilegesLv)
                    {
                        EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVE), ListView_GetSelectedCount(context->PrivilegesLv) == 1);
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
