/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2020
 *
 */

#include "extsrv.h"

typedef struct _SERVICE_LIST_CONTEXT
{
    HWND ServiceListHandle;
    PH_LAYOUT_MANAGER LayoutManager;
} SERVICE_LIST_CONTEXT, *PSERVICE_LIST_CONTEXT;

_Success_(return)
BOOLEAN EsEnumDependentServices(
    _In_ SC_HANDLE ServiceHandle,
    _Out_ PULONG Count,
    _Out_ LPENUM_SERVICE_STATUS* Services
    )
{
    BOOLEAN result;
    PVOID buffer;
    ULONG bufferSize;
    ULONG returnLength;
    ULONG servicesReturned;

    bufferSize = 0x800;
    buffer = PhAllocate(bufferSize);

    result = !!EnumDependentServices(
        ServiceHandle,
        SERVICE_STATE_ALL,
        buffer,
        bufferSize,
        &returnLength,
        &servicesReturned
        );

    if (!result)
    {
        if (GetLastError() == ERROR_MORE_DATA)
        {
            PhFree(buffer);
            bufferSize = returnLength;
            buffer = PhAllocate(bufferSize);

            result = !!EnumDependentServices(
                ServiceHandle,
                SERVICE_STATE_ALL,
                buffer,
                bufferSize,
                &returnLength,
                &servicesReturned
                );
        }

        if (!result)
        {
            PhFree(buffer);
            return FALSE;
        }
    }

    if (Count)
    {
        *Count = servicesReturned;
    }
    if (Services)
    {
        *Services = buffer;
    }

    return result;
}

VOID EspLayoutServiceListControl(
    _In_ HWND hwndDlg,
    _In_ HWND ServiceListHandle
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
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PSERVICE_LIST_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocate(sizeof(SERVICE_LIST_CONTEXT));
        memset(context, 0, sizeof(SERVICE_LIST_CONTEXT));

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_DESTROY)
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
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
            SC_HANDLE serviceHandle;
            ULONG win32Result = ERROR_SUCCESS;
            BOOLEAN success = FALSE;

            PhSetDialogItemText(hwndDlg, IDC_MESSAGE, L"This service depends on the following services:");

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_SERVICES_LAYOUT), NULL, PH_ANCHOR_ALL);

            if (serviceHandle = PhOpenService(serviceItem->Name->Buffer, SERVICE_QUERY_CONFIG))
            {
                LPQUERY_SERVICE_CONFIG serviceConfig;

                if (serviceConfig = PhGetServiceConfig(serviceHandle))
                {
                    PWSTR dependency;
                    PPH_LIST serviceList;

                    serviceList = PH_AUTO(PhCreateList(8));
                    success = TRUE;

                    if (serviceConfig->lpDependencies)
                    {
                        for (dependency = serviceConfig->lpDependencies; *dependency; dependency += PhCountStringZ(dependency) + 1)
                        {
                            PPH_SERVICE_ITEM dependencyService;

                            if (dependency[0] == SC_GROUP_IDENTIFIER)
                                continue;

                            if (dependencyService = PhReferenceServiceItem(dependency))
                                PhAddItemList(serviceList, dependencyService);
                        }
                    }

                    if (serviceList->Count)
                    {
                        PPH_SERVICE_ITEM* services;

                        services = PhAllocateCopy(serviceList->Items, sizeof(PPH_SERVICE_ITEM) * serviceList->Count);

                        serviceListHandle = PhCreateServiceListControl(hwndDlg, services, serviceList->Count);
                        context->ServiceListHandle = serviceListHandle;
                        EspLayoutServiceListControl(hwndDlg, serviceListHandle);
                        ShowWindow(serviceListHandle, SW_SHOW);
                    }

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
                PPH_STRING errorMessage = PhGetWin32Message(win32Result);

                PhSetDialogItemText(hwndDlg, IDC_SERVICES_LAYOUT, PhaConcatStrings2(
                    L"Unable to enumerate dependencies: ",
                    PhGetStringOrDefault(errorMessage, L"Unknown error.")
                    )->Buffer);

                ShowWindow(GetDlgItem(hwndDlg, IDC_SERVICES_LAYOUT), SW_SHOW);
                PhClearReference(&errorMessage);
            }

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
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
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PSERVICE_LIST_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = PhAllocate(sizeof(SERVICE_LIST_CONTEXT));
        memset(context, 0, sizeof(SERVICE_LIST_CONTEXT));

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_DESTROY)
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
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
            SC_HANDLE serviceHandle;
            ULONG win32Result = ERROR_SUCCESS;
            BOOLEAN success = FALSE;

            PhSetDialogItemText(hwndDlg, IDC_MESSAGE, L"The following services depend on this service:");

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_SERVICES_LAYOUT), NULL, PH_ANCHOR_ALL);

            if (serviceHandle = PhOpenService(serviceItem->Name->Buffer, SERVICE_ENUMERATE_DEPENDENTS))
            {
                ULONG numberOfDependentServices;
                LPENUM_SERVICE_STATUS dependentServices;

                if (EsEnumDependentServices(
                    serviceHandle,
                    &numberOfDependentServices,
                    &dependentServices
                    ))
                {
                    PPH_SERVICE_ITEM dependentService;
                    PPH_LIST serviceList;

                    serviceList = PH_AUTO(PhCreateList(8));
                    success = TRUE;

                    for (ULONG i = 0; i < numberOfDependentServices; i++)
                    {
                        if (dependentService = PhReferenceServiceItem(dependentServices[i].lpServiceName))
                            PhAddItemList(serviceList, dependentService);
                    }

                    if (serviceList->Count)
                    {
                        PPH_SERVICE_ITEM* services;

                        services = PhAllocateCopy(serviceList->Items, sizeof(PPH_SERVICE_ITEM) * serviceList->Count);

                        serviceListHandle = PhCreateServiceListControl(hwndDlg, services, serviceList->Count);
                        context->ServiceListHandle = serviceListHandle;
                        EspLayoutServiceListControl(hwndDlg, serviceListHandle);
                        ShowWindow(serviceListHandle, SW_SHOW);
                    }

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
                PPH_STRING errorMessage = PhGetWin32Message(win32Result);

                PhSetDialogItemText(hwndDlg, IDC_SERVICES_LAYOUT, PhaConcatStrings2(
                    L"Unable to enumerate dependents: ",
                    PhGetStringOrDefault(errorMessage, L"Unknown error.")
                    )->Buffer);

                ShowWindow(GetDlgItem(hwndDlg, IDC_SERVICES_LAYOUT), SW_SHOW);
                PhClearReference(&errorMessage);
            }

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
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
