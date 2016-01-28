/*
 * Process Hacker Extended Services -
 *   other information
 *
 * Copyright (C) 2010-2015 wj32
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

    struct
    {
        ULONG Ready : 1;
        ULONG Dirty : 1;
        ULONG PreshutdownTimeoutValid : 1;
        ULONG RequiredPrivilegesValid : 1;
        ULONG SidTypeValid : 1;
        ULONG LaunchProtectedValid : 1;
    };
    HWND PrivilegesLv;
    PPH_LIST PrivilegeList;

    ULONG OriginalLaunchProtected;
} SERVICE_OTHER_CONTEXT, *PSERVICE_OTHER_CONTEXT;

#define SIP(String, Integer) { (String), (PVOID)(Integer) }

BOOLEAN EspChangeServiceConfig2(
    _In_ PWSTR ServiceName,
    _In_opt_ SC_HANDLE ServiceHandle,
    _In_ ULONG InfoLevel,
    _In_ PVOID Info
    );

static PH_KEY_VALUE_PAIR EspServiceSidTypePairs[] =
{
    SIP(L"None", SERVICE_SID_TYPE_NONE),
    SIP(L"Restricted", SERVICE_SID_TYPE_RESTRICTED),
    SIP(L"Unrestricted", SERVICE_SID_TYPE_UNRESTRICTED)
};

static PH_KEY_VALUE_PAIR EspServiceLaunchProtectedPairs[] =
{
    SIP(L"None", SERVICE_LAUNCH_PROTECTED_NONE),
    SIP(L"Full (Windows)", SERVICE_LAUNCH_PROTECTED_WINDOWS),
    SIP(L"Light (Windows)", SERVICE_LAUNCH_PROTECTED_WINDOWS_LIGHT),
    SIP(L"Light (Antimalware)", SERVICE_LAUNCH_PROTECTED_ANTIMALWARE_LIGHT)
};

WCHAR *EspServiceSidTypeStrings[3] = { L"None", L"Restricted", L"Unrestricted" };
WCHAR *EspServiceLaunchProtectedStrings[4] = { L"None", L"Full (Windows)", L"Light (Windows)", L"Light (Antimalware)" };

PWSTR EspGetServiceSidTypeString(
    _In_ ULONG SidType
    )
{
    PWSTR string;

    if (PhFindStringSiKeyValuePairs(
        EspServiceSidTypePairs,
        sizeof(EspServiceSidTypePairs),
        SidType,
        &string
        ))
        return string;
    else
        return L"Unknown";
}

ULONG EspGetServiceSidTypeInteger(
    _In_ PWSTR SidType
    )
{
    ULONG integer;

    if (PhFindIntegerSiKeyValuePairs(
        EspServiceSidTypePairs,
        sizeof(EspServiceSidTypePairs),
        SidType,
        &integer
        ))
        return integer;
    else
        return -1;
}

PWSTR EspGetServiceLaunchProtectedString(
    _In_ ULONG LaunchProtected
    )
{
    PWSTR string;

    if (PhFindStringSiKeyValuePairs(
        EspServiceLaunchProtectedPairs,
        sizeof(EspServiceLaunchProtectedPairs),
        LaunchProtected,
        &string
        ))
        return string;
    else
        return L"Unknown";
}

ULONG EspGetServiceLaunchProtectedInteger(
    _In_ PWSTR LaunchProtected
    )
{
    ULONG integer;

    if (PhFindIntegerSiKeyValuePairs(
        EspServiceLaunchProtectedPairs,
        sizeof(EspServiceLaunchProtectedPairs),
        LaunchProtected,
        &integer
        ))
        return integer;
    else
        return -1;
}

NTSTATUS EspLoadOtherInfo(
    _In_ HWND hwndDlg,
    _In_ PSERVICE_OTHER_CONTEXT Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    SC_HANDLE serviceHandle;
    ULONG returnLength;
    SERVICE_PRESHUTDOWN_INFO preshutdownInfo;
    LPSERVICE_REQUIRED_PRIVILEGES_INFO requiredPrivilegesInfo;
    SERVICE_SID_INFO sidInfo;
    SERVICE_LAUNCH_PROTECTED_INFO launchProtectedInfo;

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
                privilegeLength = (ULONG)PhCountStringZ(privilege);

                if (privilegeLength == 0)
                    break;

                privilegeString = PhCreateStringEx(privilege, privilegeLength * sizeof(WCHAR));
                PhAddItemList(Context->PrivilegeList, privilegeString);

                lvItemIndex = PhAddListViewItem(Context->PrivilegesLv, MAXINT, privilege, privilegeString);
                privilegeSr.Buffer = privilege;
                privilegeSr.Length = privilegeLength * sizeof(WCHAR);

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

    // SID type

    if (QueryServiceConfig2(serviceHandle,
        SERVICE_CONFIG_SERVICE_SID_INFO,
        (PBYTE)&sidInfo,
        sizeof(SERVICE_SID_INFO),
        &returnLength
        ))
    {
        PhSelectComboBoxString(GetDlgItem(hwndDlg, IDC_SIDTYPE),
            EspGetServiceSidTypeString(sidInfo.dwServiceSidType), FALSE);
        Context->SidTypeValid = TRUE;
    }

    // Launch protected

    if (QueryServiceConfig2(serviceHandle,
        SERVICE_CONFIG_LAUNCH_PROTECTED,
        (PBYTE)&launchProtectedInfo,
        sizeof(SERVICE_LAUNCH_PROTECTED_INFO),
        &returnLength
        ))
    {
        PhSelectComboBoxString(GetDlgItem(hwndDlg, IDC_PROTECTION),
            EspGetServiceLaunchProtectedString(launchProtectedInfo.dwLaunchProtected), FALSE);
        Context->LaunchProtectedValid = TRUE;
        Context->OriginalLaunchProtected = launchProtectedInfo.dwLaunchProtected;
    }

    CloseServiceHandle(serviceHandle);

    return status;
}

static PPH_STRING EspGetServiceSidString(
    _In_ PPH_STRINGREF ServiceName
    )
{
    PSID serviceSid = NULL;
    UNICODE_STRING serviceNameUs;
    ULONG serviceSidLength = 0;
    PPH_STRING sidString = NULL;

    if (!RtlCreateServiceSid_I)
        return NULL;

    PhStringRefToUnicodeString(ServiceName, &serviceNameUs);

    if (RtlCreateServiceSid_I(&serviceNameUs, serviceSid, &serviceSidLength) == STATUS_BUFFER_TOO_SMALL)
    {
        serviceSid = PhAllocate(serviceSidLength);

        if (NT_SUCCESS(RtlCreateServiceSid_I(&serviceNameUs, serviceSid, &serviceSidLength)))
            sidString = PhSidToStringSid(serviceSid);

        PhFree(serviceSid);
    }

    return sidString;
}

static int __cdecl PrivilegeNameCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PWSTR string1 = *(PWSTR *)elem1;
    PWSTR string2 = *(PWSTR *)elem2;

    return PhCompareStringZ(string1, string2, TRUE);
}

INT_PTR CALLBACK EspServiceOtherDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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

            PhAddComboBoxStrings(GetDlgItem(hwndDlg, IDC_SIDTYPE),
                EspServiceSidTypeStrings, sizeof(EspServiceSidTypeStrings) / sizeof(PWSTR));
            PhAddComboBoxStrings(GetDlgItem(hwndDlg, IDC_PROTECTION),
                EspServiceLaunchProtectedStrings, sizeof(EspServiceLaunchProtectedStrings) / sizeof(PWSTR));

            if (WindowsVersion < WINDOWS_8_1)
                EnableWindow(GetDlgItem(hwndDlg, IDC_PROTECTION), FALSE);

            SetDlgItemText(hwndDlg, IDC_SERVICESID,
                PhGetStringOrDefault(PhAutoDereferenceObject(EspGetServiceSidString(&serviceItem->Name->sr)), L"N/A"));

            status = EspLoadOtherInfo(hwndDlg, context);

            if (!NT_SUCCESS(status))
            {
                PhShowWarning(hwndDlg, L"Unable to query service information: %s",
                    ((PPH_STRING)PhAutoDereferenceObject(PhGetNtMessage(status)))->Buffer);
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

                        LsaFreeMemory(buffer);
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

                        PhSetReference(&privilegeString, selectedChoice);
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

                    if (lvItemIndex != -1 && PhGetListViewItemParam(context->PrivilegesLv, lvItemIndex, (PVOID *)&privilegeString))
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
            }

            switch (HIWORD(wParam))
            {
            case EN_CHANGE:
            case CBN_SELCHANGE:
                {
                    if (context->Ready)
                    {
                        context->Dirty = TRUE;

                        switch (LOWORD(wParam))
                        {
                        case IDC_PRESHUTDOWNTIMEOUT:
                            context->PreshutdownTimeoutValid = TRUE;
                            break;
                        case IDC_SIDTYPE:
                            context->SidTypeValid = TRUE;
                            break;
                        case IDC_PROTECTION:
                            context->LaunchProtectedValid = TRUE;
                            break;
                        }
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
                    SC_HANDLE serviceHandle = NULL;
                    ULONG win32Result = 0;
                    BOOLEAN connectedToPhSvc = FALSE;
                    PPH_STRING launchProtectedString;
                    ULONG launchProtected;

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

                    launchProtectedString = PhAutoDereferenceObject(PhGetWindowText(GetDlgItem(hwndDlg, IDC_PROTECTION)));
                    launchProtected = EspGetServiceLaunchProtectedInteger(launchProtectedString->Buffer);

                    if (context->LaunchProtectedValid && launchProtected != 0 && launchProtected != context->OriginalLaunchProtected)
                    {
                        if (PhShowMessage(
                            hwndDlg,
                            MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2,
                            L"Setting service protection will prevent the service from being controlled, modified, or deleted. Do you want to continue?"
                            ) == IDNO)
                        {
                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                            return TRUE;
                        }
                    }

                    if (context->Dirty)
                    {
                        SERVICE_PRESHUTDOWN_INFO preshutdownInfo;
                        SERVICE_REQUIRED_PRIVILEGES_INFO requiredPrivilegesInfo;
                        SERVICE_SID_INFO sidInfo;
                        SERVICE_LAUNCH_PROTECTED_INFO launchProtectedInfo;

                        if (!(serviceHandle = PhOpenService(context->ServiceItem->Name->Buffer, SERVICE_CHANGE_CONFIG)))
                        {
                            win32Result = GetLastError();

                            if (win32Result == ERROR_ACCESS_DENIED && !PhElevated)
                            {
                                // Elevate using phsvc.
                                if (PhUiConnectToPhSvc(hwndDlg, FALSE))
                                {
                                    win32Result = 0;
                                    connectedToPhSvc = TRUE;
                                }
                                else
                                {
                                    // User cancelled elevation.
                                    win32Result = ERROR_CANCELLED;
                                    goto Done;
                                }
                            }
                            else
                            {
                                goto Done;
                            }
                        }

                        if (context->PreshutdownTimeoutValid)
                        {
                            preshutdownInfo.dwPreshutdownTimeout = GetDlgItemInt(hwndDlg, IDC_PRESHUTDOWNTIMEOUT, NULL, FALSE);

                            if (!EspChangeServiceConfig2(context->ServiceItem->Name->Buffer, serviceHandle,
                                SERVICE_CONFIG_PRESHUTDOWN_INFO, &preshutdownInfo))
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
                                PhAppendStringBuilder(&sb, &((PPH_STRING)context->PrivilegeList->Items[i])->sr);
                                PhAppendCharStringBuilder(&sb, 0);
                            }

                            requiredPrivilegesInfo.pmszRequiredPrivileges = sb.String->Buffer;

                            if (win32Result == 0 && !EspChangeServiceConfig2(context->ServiceItem->Name->Buffer, serviceHandle,
                                SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO, &requiredPrivilegesInfo))
                            {
                                win32Result = GetLastError();
                            }

                            PhDeleteStringBuilder(&sb);
                        }

                        if (context->SidTypeValid)
                        {
                            PPH_STRING sidTypeString;

                            sidTypeString = PhAutoDereferenceObject(PhGetWindowText(GetDlgItem(hwndDlg, IDC_SIDTYPE)));
                            sidInfo.dwServiceSidType = EspGetServiceSidTypeInteger(sidTypeString->Buffer);

                            if (win32Result == 0 && !EspChangeServiceConfig2(context->ServiceItem->Name->Buffer, serviceHandle,
                                SERVICE_CONFIG_SERVICE_SID_INFO, &sidInfo))
                            {
                                win32Result = GetLastError();
                            }
                        }

                        if (context->LaunchProtectedValid)
                        {
                            launchProtectedInfo.dwLaunchProtected = launchProtected;

                            if (!EspChangeServiceConfig2(context->ServiceItem->Name->Buffer, serviceHandle,
                                SERVICE_CONFIG_LAUNCH_PROTECTED, &launchProtectedInfo))
                            {
                                // For now, ignore errors here.
                                // win32Result = GetLastError();
                            }
                        }

Done:
                        if (connectedToPhSvc)
                            PhUiDisconnectFromPhSvc();
                        if (serviceHandle)
                            CloseServiceHandle(serviceHandle);

                        if (win32Result != 0)
                        {
                            if (win32Result == ERROR_CANCELLED || PhShowMessage(
                                hwndDlg,
                                MB_ICONERROR | MB_RETRYCANCEL,
                                L"Unable to change service information: %s",
                                ((PPH_STRING)PhAutoDereferenceObject(PhGetWin32Message(win32Result)))->Buffer
                                ) == IDRETRY)
                            {
                                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                            }
                        }
                    }

                    return TRUE;
                }
                break;
            case LVN_ITEMCHANGED:
                {
                    if (header->hwndFrom == context->PrivilegesLv)
                    {
                        EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVE), ListView_GetSelectedCount(context->PrivilegesLv) == 1);
                    }
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

BOOLEAN EspChangeServiceConfig2(
    _In_ PWSTR ServiceName,
    _In_opt_ SC_HANDLE ServiceHandle,
    _In_ ULONG InfoLevel,
    _In_ PVOID Info
    )
{
    if (ServiceHandle)
    {
        return !!ChangeServiceConfig2(ServiceHandle, InfoLevel, Info);
    }
    else
    {
        NTSTATUS status;

        if (NT_SUCCESS(status = PhSvcCallChangeServiceConfig2(ServiceName, InfoLevel, Info)))
        {
            return TRUE;
        }
        else
        {
            SetLastError(PhNtStatusToDosError(status));
            return FALSE;
        }
    }
}
