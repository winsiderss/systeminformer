/*
 * Process Hacker Extended Tools - 
 *   services referencing module
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

#include "exttools.h"
#include "resource.h"

typedef struct _MODULE_SERVICES_CONTEXT
{
    HANDLE ProcessId;
    PWSTR ModuleName;
} MODULE_SERVICES_CONTEXT, *PMODULE_SERVICES_CONTEXT;

INT_PTR CALLBACK EtpModuleServicesDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

VOID EtShowModuleServicesDialog(
    __in HWND ParentWindowHandle,
    __in HANDLE ProcessId,
    __in PWSTR ModuleName
    )
{
    MODULE_SERVICES_CONTEXT context;

    context.ProcessId = ProcessId;
    context.ModuleName = ModuleName;

    DialogBoxParam(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_MODSERVICES),
        ParentWindowHandle,
        EtpModuleServicesDlgProc,
        (LPARAM)&context
        );
}

INT_PTR CALLBACK EtpModuleServicesDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PMODULE_SERVICES_CONTEXT context = (PMODULE_SERVICES_CONTEXT)lParam;
            ULONG win32Result;
            SC_SERVICE_NAMES_REFERENCING_MODULE_QUERY scQuery;
            _I_QueryTagInformation I_QueryTagInformation;
            PPH_LIST serviceList;
            PPH_SERVICE_ITEM *serviceItems;
            HWND serviceListHandle;
            RECT rect;
            PPH_PROCESS_ITEM processItem;
            PPH_STRING message;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            I_QueryTagInformation = PhGetProcAddress(L"advapi32.dll", "I_QueryTagInformation");

            if (!I_QueryTagInformation)
            {
                PhShowError(hwndDlg, L"Unable to query services because the feature is not supported by the operating system.");
                EndDialog(hwndDlg, IDCANCEL);
                return FALSE;
            }

            scQuery.ProcessId = (ULONG)context->ProcessId;
            scQuery.Module = context->ModuleName;
            scQuery.Unknown = 0;
            scQuery.ServiceNames = NULL;

            win32Result = I_QueryTagInformation(NULL, ServiceNamesReferencingModuleInformation, &scQuery);

            if (win32Result == ERROR_NO_MORE_ITEMS)
                win32Result = 0;

            if (win32Result != 0)
            {
                PhShowStatus(hwndDlg, L"Unable to query services", 0, win32Result);
                EndDialog(hwndDlg, IDCANCEL);
                return FALSE;
            }

            serviceList = PhCreateList(16);

            if (scQuery.ServiceNames)
            {
                PPH_SERVICE_ITEM serviceItem;
                PWSTR serviceName;
                ULONG nameLength;

                serviceName = scQuery.ServiceNames;

                while (TRUE)
                {
                    nameLength = (ULONG)wcslen(serviceName);

                    if (nameLength == 0)
                        break;

                    if (serviceItem = PhReferenceServiceItem(serviceName))
                        PhAddItemList(serviceList, serviceItem);

                    serviceName += nameLength + 1;
                }

                LocalFree(scQuery.ServiceNames);
            }

            serviceItems = PhAllocateCopy(serviceList->Items, serviceList->Count * sizeof(PPH_SERVICE_ITEM));
            PhDereferenceObject(serviceList);
            serviceListHandle = PhCreateServiceListControl(hwndDlg, serviceItems, serviceList->Count);

            // Position the control.
            GetWindowRect(GetDlgItem(hwndDlg, IDC_SERVICES_LAYOUT), &rect);
            MapWindowPoints(NULL, hwndDlg, (POINT *)&rect, 2);
            MoveWindow(serviceListHandle, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, FALSE);

            ShowWindow(serviceListHandle, SW_SHOW);

            if (processItem = PhReferenceProcessItem(context->ProcessId))
            {
                message = PhFormatString(L"Services referencing %s in %s:", context->ModuleName, processItem->ProcessName->Buffer);
                PhDereferenceObject(processItem);
            }
            else
            {
                message = PhFormatString(L"Services referencing %s:", context->ModuleName);
            }

            SetDlgItemText(hwndDlg, IDC_MESSAGE, message->Buffer);
            PhDereferenceObject(message);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    }

    return FALSE;
}
