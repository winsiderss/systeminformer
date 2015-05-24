/*
 * Process Hacker Extra Plugins -
 *   Service Extras Plugin
 *
 * Copyright (C) 2015 dmex
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

#include "main.h"

static PWSTR PhServiceProtectedTypeStrings[4] =
{
    L"None",
    L"Full (Windows)",
    L"Light (Windows)",
    L"Light (Antimalware)"
};

static PWSTR PhServiceSidTypeStrings[3] =
{
    L"None",
    L"Restricted",
    L"Unrestricted"
};

// Windows 8.1 and above
static PPH_STRING PhGetServiceProtectionType(
    _In_ SC_HANDLE ServiceHandle
    )
{
    SERVICE_LAUNCH_PROTECTED_INFO launchProtectedInfo;
    ULONG returnLength;

    if (QueryServiceConfig2(
        ServiceHandle,
        SERVICE_CONFIG_LAUNCH_PROTECTED,
        (PBYTE)&launchProtectedInfo,
        sizeof(SERVICE_LAUNCH_PROTECTED_INFO),
        &returnLength
        ))
    {
        switch (launchProtectedInfo.dwLaunchProtected)
        {
        case SERVICE_LAUNCH_PROTECTED_NONE:
            return PhCreateString(PhServiceProtectedTypeStrings[SERVICE_LAUNCH_PROTECTED_NONE]);
        case SERVICE_LAUNCH_PROTECTED_WINDOWS:
            return PhCreateString(PhServiceProtectedTypeStrings[SERVICE_LAUNCH_PROTECTED_WINDOWS]);
        case SERVICE_LAUNCH_PROTECTED_WINDOWS_LIGHT:
            return PhCreateString(PhServiceProtectedTypeStrings[SERVICE_LAUNCH_PROTECTED_WINDOWS_LIGHT]);
        case SERVICE_LAUNCH_PROTECTED_ANTIMALWARE_LIGHT:
            return PhCreateString(PhServiceProtectedTypeStrings[SERVICE_LAUNCH_PROTECTED_ANTIMALWARE_LIGHT]);
        }
    }

    return NULL;
}

static BOOLEAN PhSetServiceProtectionType(
    _In_ SC_HANDLE ServiceHandle,
    _In_ ULONG ProtectionType
    )
{
    SERVICE_LAUNCH_PROTECTED_INFO launchProtectedInfo;

    launchProtectedInfo.dwLaunchProtected = ProtectionType;

    return !!ChangeServiceConfig2(
        ServiceHandle,
        SERVICE_CONFIG_LAUNCH_PROTECTED,
        &launchProtectedInfo
        );
}

// Windows Vista and above
static PPH_STRING PhGetServiceSidInfo(
    _In_ SC_HANDLE ServiceHandle
    )
{
    SERVICE_SID_INFO serviceSidInfo;
    ULONG returnLength;

    if (QueryServiceConfig2(
        ServiceHandle,
        SERVICE_CONFIG_SERVICE_SID_INFO,
        (PBYTE)&serviceSidInfo,
        sizeof(SERVICE_SID_INFO),
        &returnLength
        ))
    {
        switch (serviceSidInfo.dwServiceSidType)
        {
        case SERVICE_SID_TYPE_NONE:
            return PhCreateString(PhServiceSidTypeStrings[0]);
        case SERVICE_SID_TYPE_RESTRICTED:
            return PhCreateString(PhServiceSidTypeStrings[1]);
        case SERVICE_SID_TYPE_UNRESTRICTED:
            return PhCreateString(PhServiceSidTypeStrings[2]);
        }
    }

    return NULL;
}

static BOOLEAN PhSetServiceServiceSidInfo(
    _In_ SC_HANDLE ServiceHandle,
    _In_ ULONG ServiceSidType
    )
{
    SERVICE_SID_INFO serviceSidInfo;

    serviceSidInfo.dwServiceSidType = ServiceSidType;

    return !!ChangeServiceConfig2(
        ServiceHandle,
        SERVICE_CONFIG_SERVICE_SID_INFO,
        &serviceSidInfo
        );
}

static PPH_STRING PhGetServiceSidString(
    _In_ PPH_STRING ServiceName
    )
{
    ULONG serviceSidLength = 0;
    UNICODE_STRING serviceNameString;
    PSID serviceSid = NULL;
    PPH_STRING sidString = NULL;

    RtlInitUnicodeString(&serviceNameString, ServiceName->Buffer);

    if (RtlCreateServiceSid_I && RtlCreateServiceSid_I(&serviceNameString, serviceSid, &serviceSidLength) == STATUS_BUFFER_TOO_SMALL)
    {
        serviceSid = PhAllocate(serviceSidLength);

        if (NT_SUCCESS(RtlCreateServiceSid_I(&serviceNameString, serviceSid, &serviceSidLength)))
        {
            sidString = PhSidToStringSid(serviceSid);
        }

        PhFree(serviceSid);
    }

    return sidString;
}

VOID UpdateServiceSid(
    _In_ PPH_SERVICE_NODE Node,
    _In_ PSERVICE_EXTENSION Extension
    )
{
    if (!Extension->Valid)
    {
        PhSwapReference(&Extension->ServiceSid, PhGetServiceSidString(Node->ServiceItem->Name));
        Extension->Valid = TRUE;
    }
}

INT_PTR CALLBACK ServiceExtraDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PSERVICE_EXTRA_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocate(sizeof(SERVICE_EXTRA_CONTEXT));
        memset(context, 0, sizeof(SERVICE_EXTRA_CONTEXT));

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PSERVICE_EXTRA_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
            RemoveProp(hwndDlg, L"Context");
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            SC_HANDLE serviceHandle;
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)propSheetPage->lParam;

            context->ServiceProtectCombo = GetDlgItem(hwndDlg, IDC_PROTECTCOMBO);
            context->ServiceSidCombo = GetDlgItem(hwndDlg, IDC_SIDTYPECOMBO);
            context->ServiceApplyButton = GetDlgItem(hwndDlg, IDC_APPLY);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);

            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_SRVPROTECTGRPBOX), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->ServiceProtectCombo, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_SRVSIDTYPEGRPBOX), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->ServiceSidCombo, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
            PhAddLayoutItem(&context->LayoutManager, context->ServiceApplyButton, NULL, PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);

            PhAddComboBoxStrings(context->ServiceProtectCombo, PhServiceProtectedTypeStrings, _countof(PhServiceProtectedTypeStrings));
            PhAddComboBoxStrings(context->ServiceSidCombo, PhServiceSidTypeStrings, _countof(PhServiceSidTypeStrings));

            PhSelectComboBoxString(context->ServiceProtectCombo, L"None", FALSE);
            PhSelectComboBoxString(context->ServiceSidCombo, L"None", FALSE);

            if (serviceHandle = PhOpenService(serviceItem->Name->Buffer, SERVICE_QUERY_CONFIG))
            {
                PPH_STRING serviceProtectType = NULL;
                PPH_STRING serviceSidType = NULL;

                serviceProtectType = PhGetServiceProtectionType(serviceHandle);
                serviceSidType = PhGetServiceSidInfo(serviceHandle);

                if (serviceProtectType)
                {
                    PhSelectComboBoxString(context->ServiceProtectCombo, serviceProtectType->Buffer, FALSE);
                    PhDereferenceObject(serviceProtectType);
                }
                else
                {
                    EnableWindow(context->ServiceProtectCombo, FALSE);
                }

                if (serviceSidType)
                {
                    PhSelectComboBoxString(context->ServiceSidCombo, serviceSidType->Buffer, FALSE);
                    PhDereferenceObject(serviceSidType);
                }
                else
                {
                    EnableWindow(context->ServiceSidCombo, FALSE);
                }

                CloseServiceHandle(serviceHandle);
            }
            else
            {
                EnableWindow(context->ServiceProtectCombo, FALSE);
                EnableWindow(context->ServiceSidCombo, FALSE);
            }
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);
            PhFree(context);
        }
        break;
    case WM_SIZE:
        PhLayoutManagerLayout(&context->LayoutManager);
        break;
    }

    return FALSE;
}