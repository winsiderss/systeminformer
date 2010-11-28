/*
 * Process Hacker Extended Services - 
 *   other information
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

typedef struct _SERVICE_OTHER_CONTEXT
{
    PPH_SERVICE_ITEM ServiceItem;

    HWND PrivilegesLv;
    HWND TriggersLv;
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
    PSERVICE_TRIGGER_INFO triggerInfo;
    ULONG returnLength;
    ULONG i;

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

                if (NT_SUCCESS(PhLookupPrivilegeDisplayName(&privilegeSr, &displayName)))
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

    if (WindowsVersion >= WINDOWS_7 && (triggerInfo = PhQueryServiceVariableSize(serviceHandle, SERVICE_CONFIG_TRIGGER_INFO)))
    {
        for (i = 0; i < triggerInfo->cTriggers; i++)
        {
            static GUID networkManagerFirstIpAddressArrivalGuid = { 0x4f27f2de, 0x14e2, 0x430b, { 0xa5, 0x49, 0x7c, 0xd4, 0x8c, 0xbc, 0x82, 0x45 } };
            static GUID networkManagerLastIpAddressRemovalGuid = { 0xcc4ba62a, 0x162e, 0x4648, { 0x84, 0x7a, 0xb6, 0xbd, 0xf9, 0x93, 0xe3, 0x35 } };
            static GUID domainJoinGuid = { 0x1ce20aba, 0x9851, 0x4421, { 0x94, 0x30, 0x1d, 0xde, 0xb7, 0x66, 0xe8, 0x09 } };
            static GUID domainLeaveGuid = { 0xddaf516e, 0x58c2, 0x4866, { 0x95, 0x74, 0xc3, 0xb6, 0x15, 0xd4, 0x2e, 0xa1 } };
            static GUID firewallPortOpenGuid = { 0xb7569e07, 0x8421, 0x4ee0, { 0xad, 0x10, 0x86, 0x91, 0x5a, 0xfd, 0xad, 0x09 } };
            static GUID firewallPortCloseGuid = { 0xa144ed38, 0x8e12, 0x4de4, { 0x9d, 0x96, 0xe6, 0x47, 0x40, 0xb1, 0xa5, 0x24 } };
            static GUID machinePolicyPresentGuid = { 0x659fcae6, 0x5bdb, 0x4da9, { 0xb1, 0xff, 0xca, 0x2a, 0x17, 0x8d, 0x46, 0xe0 } };
            static GUID userPolicyPresentGuid = { 0x54fb46c8, 0xf089, 0x464c, { 0xb1, 0xfd, 0x59, 0xd1, 0xb6, 0x2c, 0x3b, 0x50 } };

            PSERVICE_TRIGGER trigger = &triggerInfo->pTriggers[i];
            PPH_STRING stringUsed = NULL;
            PWSTR triggerString;
            PWSTR actionString;
            INT lvItemIndex;

            switch (trigger->dwTriggerType)
            {
            case SERVICE_TRIGGER_TYPE_DEVICE_INTERFACE_ARRIVAL:
                triggerString = L"Device interface arrival";
                break;
            case SERVICE_TRIGGER_TYPE_IP_ADDRESS_AVAILABILITY:
                if (!trigger->pTriggerSubtype)
                    triggerString = L"IP address";
                else if (memcmp(trigger->pTriggerSubtype, &networkManagerFirstIpAddressArrivalGuid, sizeof(GUID)) == 0)
                    triggerString = L"IP address: First arrival";
                else if (memcmp(trigger->pTriggerSubtype, &networkManagerLastIpAddressRemovalGuid, sizeof(GUID)) == 0)
                    triggerString = L"IP address: Last removal";
                else
                    triggerString = L"IP address: Unknown";
                break;
            case SERVICE_TRIGGER_TYPE_DOMAIN_JOIN:
                if (!trigger->pTriggerSubtype)
                    triggerString = L"Domain";
                else if (memcmp(trigger->pTriggerSubtype, &domainJoinGuid, sizeof(GUID)) == 0)
                    triggerString = L"Domain: Join";
                else if (memcmp(trigger->pTriggerSubtype, &domainLeaveGuid, sizeof(GUID)) == 0)
                    triggerString = L"Domain: Leave";
                else
                    triggerString = L"Domain: Unknown";
                break;
            case SERVICE_TRIGGER_TYPE_FIREWALL_PORT_EVENT:
                if (!trigger->pTriggerSubtype)
                    triggerString = L"Firewall port";
                else if (memcmp(trigger->pTriggerSubtype, &firewallPortOpenGuid, sizeof(GUID)) == 0)
                    triggerString = L"Firewall port: Open";
                else if (memcmp(trigger->pTriggerSubtype, &firewallPortCloseGuid, sizeof(GUID)) == 0)
                    triggerString = L"Firewall port: Close";
                else
                    triggerString = L"Firewall port: Unknown";
                break;
            case SERVICE_TRIGGER_TYPE_GROUP_POLICY:
                if (!trigger->pTriggerSubtype)
                    triggerString = L"Group policy change";
                else if (memcmp(trigger->pTriggerSubtype, &machinePolicyPresentGuid, sizeof(GUID)) == 0)
                    triggerString = L"Group policy change: Machine";
                else if (memcmp(trigger->pTriggerSubtype, &userPolicyPresentGuid, sizeof(GUID)) == 0)
                    triggerString = L"Group policy change: User";
                else
                    triggerString = L"Group policy change: Unknown";
                break;
            case SERVICE_TRIGGER_TYPE_CUSTOM:
                {
                    PPH_STRING guidString;
                    PPH_STRING keyName;
                    HKEY keyHandle;
                    PPH_STRING publisherName = NULL;

                    if (trigger->pTriggerSubtype)
                    {
                        // Try to lookup the publisher name from the GUID.
                        // Copied from ProcessHacker\hndlinfo.c.

                        guidString = PhFormatGuid(trigger->pTriggerSubtype);

                        keyName = PhConcatStrings2(
                            L"Software\\Microsoft\\Windows\\CurrentVersion\\WINEVT\\Publishers\\",
                            guidString->Buffer
                            );

                        if (RegOpenKeyEx(
                            HKEY_LOCAL_MACHINE,
                            keyName->Buffer,
                            0,
                            KEY_READ,
                            &keyHandle
                            ) == ERROR_SUCCESS)
                        {
                            publisherName = PhQueryRegistryString(keyHandle, NULL);

                            if (publisherName && publisherName->Length == 0)
                            {
                                PhDereferenceObject(publisherName);
                                publisherName = NULL;
                            }

                            RegCloseKey(keyHandle);
                        }

                        PhDereferenceObject(keyName);

                        if (publisherName)
                        {
                            stringUsed = PhFormatString(L"Custom: %s", publisherName->Buffer);
                            PhDereferenceObject(publisherName);
                        }
                        else
                        {
                            stringUsed = PhFormatString(L"Custom: %s", guidString->Buffer);
                        }

                        PhDereferenceObject(guidString);

                        triggerString = stringUsed->Buffer;
                    }
                    else
                    {
                        triggerString = L"Custom";
                    }
                }
                break;
            default:
                triggerString = L"Unknown";
                break;
            }

            switch (trigger->dwAction)
            {
            case SERVICE_TRIGGER_ACTION_SERVICE_START:
                actionString = L"Start";
                break;
            case SERVICE_TRIGGER_ACTION_SERVICE_STOP:
                actionString = L"Stop";
                break;
            default:
                actionString = L"Unknown";
                break;
            }

            lvItemIndex = PhAddListViewItem(Context->TriggersLv, MAXINT, triggerString, NULL);
            PhSetListViewSubItem(Context->TriggersLv, lvItemIndex, 1, actionString);

            if (stringUsed)
                PhDereferenceObject(stringUsed);
        }

        ExtendedListView_SortItems(Context->TriggersLv);

        PhFree(triggerInfo);
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
            PhSetListViewStyle(triggersLv, FALSE, TRUE);
            PhSetControlTheme(triggersLv, L"explorer");
            PhAddListViewColumn(triggersLv, 0, 0, 0, LVCFMT_LEFT, 300, L"Trigger");
            PhAddListViewColumn(triggersLv, 1, 1, 1, LVCFMT_LEFT, 60, L"Action");
            PhSetExtendedListView(triggersLv);

            if (WindowsVersion < WINDOWS_7)
            {
                EnableWindow(GetDlgItem(hwndDlg, IDC_TRIGGERS_LABEL), FALSE);
                EnableWindow(triggersLv, FALSE);
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
            PhFree(context);
        }
        break;
    }

    return FALSE;
}
