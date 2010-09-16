/*
 * Process Hacker Extended Services - 
 *   dependencies and dependents
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

typedef struct _SERVICE_LIST_CONTEXT
{
    HWND ServiceListHandle;
    PH_LAYOUT_MANAGER LayoutManager;
} SERVICE_LIST_CONTEXT, *PSERVICE_LIST_CONTEXT;

LPENUM_SERVICE_STATUS EsEnumDependentServices(
    __in SC_HANDLE ServiceHandle,
    __in_opt ULONG State,
    __out PULONG Count
    )
{
    LOGICAL result;
    PVOID buffer;
    ULONG bufferSize;
    ULONG returnLength;
    ULONG servicesReturned;

    if (!State)
        State = SERVICE_STATE_ALL;

    bufferSize = 0x800;
    buffer = PhAllocate(bufferSize);

    if (!(result = EnumDependentServices(
        ServiceHandle,
        State,
        buffer,
        bufferSize,
        &returnLength,
        &servicesReturned
        )))
    {
        if (GetLastError() == ERROR_MORE_DATA)
        {
            PhFree(buffer);
            bufferSize = returnLength;
            buffer = PhAllocate(bufferSize);

            result = EnumDependentServices(
                ServiceHandle,
                State,
                buffer,
                bufferSize,
                &returnLength,
                &servicesReturned
                );
        }

        if (!result)
        {
            PhFree(buffer);
            return NULL;
        }
    }

    *Count = servicesReturned;

    return buffer;
}

static VOID EspLayoutServiceListControl(
    __in HWND hwndDlg,
    __in HWND ServiceListHandle
    )
{
    RECT rect;

    GetWindowRect(GetDlgItem(hwndDlg, IDC_SERVICES_LAYOUT), &rect);
    MapWindowPoints(NULL, hwndDlg, (POINT *)&rect, 2);

    MoveWindow(
        ServiceListHandle,
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        TRUE
        );
}

INT_PTR CALLBACK EspServiceDependenciesDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PSERVICE_LIST_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocate(sizeof(SERVICE_LIST_CONTEXT));
        memset(context, 0, sizeof(SERVICE_LIST_CONTEXT));

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PSERVICE_LIST_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
            RemoveProp(hwndDlg, L"Context");
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)propSheetPage->lParam;
            HWND serviceListHandle;
            PPH_LIST serviceList;
            SC_HANDLE serviceHandle;
            ULONG win32Result = 0;
            BOOLEAN success = FALSE;
            PPH_SERVICE_ITEM *services;

            SetDlgItemText(hwndDlg, IDC_MESSAGE, L"This service depends on the following services:");

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_SERVICES_LAYOUT), NULL, PH_ANCHOR_ALL);

            if (serviceHandle = PhOpenService(serviceItem->Name->Buffer, SERVICE_QUERY_CONFIG))
            {
                LPQUERY_SERVICE_CONFIG serviceConfig;

                if (serviceConfig = PhGetServiceConfig(serviceHandle))
                {
                    PWSTR dependency;
                    PPH_SERVICE_ITEM dependencyService;

                    dependency = serviceConfig->lpDependencies;
                    serviceList = PhCreateList(8);
                    success = TRUE;

                    if (dependency)
                    {
                        ULONG dependencyLength;

                        while (TRUE)
                        {
                            dependencyLength = wcslen(dependency);

                            if (dependencyLength == 0)
                                break;

                            if (dependency[0] == SC_GROUP_IDENTIFIER)
                                goto ContinueLoop;

                            if (dependencyService = PhReferenceServiceItem(dependency))
                                PhAddItemList(serviceList, dependencyService);

ContinueLoop:
                            dependency += dependencyLength + 1;
                        }
                    }

                    services = PhAllocateCopy(serviceList->Items, sizeof(PPH_SERVICE_ITEM) * serviceList->Count);

                    serviceListHandle = PhCreateServiceListControl(hwndDlg, services, serviceList->Count);
                    context->ServiceListHandle = serviceListHandle;
                    EspLayoutServiceListControl(hwndDlg, serviceListHandle);
                    ShowWindow(serviceListHandle, SW_SHOW);

                    PhDereferenceObject(serviceList);
                    PhFree(serviceConfig);
                }
                else
                {
                    win32Result = GetLastError();
                }

                CloseServiceHandle(serviceHandle);
            }
            else
            {
                win32Result = GetLastError();
            }

            if (!success)
            {
                SetDlgItemText(hwndDlg, IDC_SERVICES_LAYOUT, PhaConcatStrings2(L"Unable to enumerate dependencies: ",
                    ((PPH_STRING)PHA_DEREFERENCE(PhGetWin32Message(win32Result)))->Buffer)->Buffer);
                ShowWindow(GetDlgItem(hwndDlg, IDC_SERVICES_LAYOUT), SW_SHOW);
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
        {
            PhLayoutManagerLayout(&context->LayoutManager);

            if (context->ServiceListHandle)
                EspLayoutServiceListControl(hwndDlg, context->ServiceListHandle);
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK EspServiceDependentsDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PSERVICE_LIST_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocate(sizeof(SERVICE_LIST_CONTEXT));
        memset(context, 0, sizeof(SERVICE_LIST_CONTEXT));

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PSERVICE_LIST_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
            RemoveProp(hwndDlg, L"Context");
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)propSheetPage->lParam;
            HWND serviceListHandle;
            PPH_LIST serviceList;
            SC_HANDLE serviceHandle;
            ULONG win32Result = 0;
            BOOLEAN success = FALSE;
            PPH_SERVICE_ITEM *services;

            SetDlgItemText(hwndDlg, IDC_MESSAGE, L"The following services depend on this service:");

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_SERVICES_LAYOUT), NULL, PH_ANCHOR_ALL);

            if (serviceHandle = PhOpenService(serviceItem->Name->Buffer, SERVICE_ENUMERATE_DEPENDENTS))
            {
                LPENUM_SERVICE_STATUS dependentServices;
                ULONG numberOfDependentServices;

                if (dependentServices = EsEnumDependentServices(serviceHandle, 0, &numberOfDependentServices))
                {
                    ULONG i;
                    PPH_SERVICE_ITEM dependentService;

                    serviceList = PhCreateList(8);
                    success = TRUE;

                    for (i = 0; i < numberOfDependentServices; i++)
                    {
                        if (dependentService = PhReferenceServiceItem(dependentServices[i].lpServiceName))
                            PhAddItemList(serviceList, dependentService);
                    }

                    services = PhAllocateCopy(serviceList->Items, sizeof(PPH_SERVICE_ITEM) * serviceList->Count);

                    serviceListHandle = PhCreateServiceListControl(hwndDlg, services, serviceList->Count);
                    context->ServiceListHandle = serviceListHandle;
                    EspLayoutServiceListControl(hwndDlg, serviceListHandle);
                    ShowWindow(serviceListHandle, SW_SHOW);

                    PhDereferenceObject(serviceList);
                    PhFree(dependentServices);
                }
                else
                {
                    win32Result = GetLastError();
                }

                CloseServiceHandle(serviceHandle);
            }
            else
            {
                win32Result = GetLastError();
            }

            if (!success)
            {
                SetDlgItemText(hwndDlg, IDC_SERVICES_LAYOUT, PhaConcatStrings2(L"Unable to enumerate dependents: ",
                    ((PPH_STRING)PHA_DEREFERENCE(PhGetWin32Message(win32Result)))->Buffer)->Buffer);
                ShowWindow(GetDlgItem(hwndDlg, IDC_SERVICES_LAYOUT), SW_SHOW);
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
        {
            PhLayoutManagerLayout(&context->LayoutManager);

            if (context->ServiceListHandle)
                EspLayoutServiceListControl(hwndDlg, context->ServiceListHandle);
        }
        break;
    }

    return FALSE;
}
