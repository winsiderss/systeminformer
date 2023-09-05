/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2020-2023
 *
 */

#include "extsrv.h"

typedef struct _SERVICE_LIST_CONTEXT
{
    HWND ServiceListHandle;
    PH_LAYOUT_MANAGER LayoutManager;
} SERVICE_LIST_CONTEXT, *PSERVICE_LIST_CONTEXT;

VOID EspLayoutServiceListControl(
    _In_ HWND WindowHandle,
    _In_ HWND ServiceListHandle
    )
{
    RECT rect;

    GetWindowRect(GetDlgItem(WindowHandle, IDC_SERVICES_LAYOUT), &rect);
    MapWindowPoints(NULL, WindowHandle, (POINT *)&rect, 2);

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
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PSERVICE_LIST_CONTEXT context;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = PhAllocate(sizeof(SERVICE_LIST_CONTEXT));
        memset(context, 0, sizeof(SERVICE_LIST_CONTEXT));

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
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)propSheetPage->lParam;
            HWND serviceListHandle;
            SC_HANDLE serviceHandle;
            NTSTATUS status;

            PhSetDialogItemText(WindowHandle, IDC_MESSAGE, L"This service depends on the following services:");

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_SERVICES_LAYOUT), NULL, PH_ANCHOR_ALL);

            if (NT_SUCCESS(status = PhOpenService(&serviceHandle, SERVICE_QUERY_CONFIG, PhGetString(serviceItem->Name))))
            {
                LPQUERY_SERVICE_CONFIG serviceConfig;

                status = PhGetServiceConfig(serviceHandle, &serviceConfig);

                if (NT_SUCCESS(status))
                {
                    PWSTR dependency;
                    PPH_LIST serviceList;

                    serviceList = PH_AUTO(PhCreateList(8));

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

                        serviceListHandle = PhCreateServiceListControl(WindowHandle, services, serviceList->Count);
                        context->ServiceListHandle = serviceListHandle;
                        EspLayoutServiceListControl(WindowHandle, serviceListHandle);
                        ShowWindow(serviceListHandle, SW_SHOW);
                    }

                    PhFree(serviceConfig);
                }

                PhCloseServiceHandle(serviceHandle);
            }

            if (!NT_SUCCESS(status))
            {
                PPH_STRING errorMessage = PhGetNtMessage(status);

                PhSetDialogItemText(WindowHandle, IDC_SERVICES_LAYOUT, PhaConcatStrings2(
                    L"Unable to enumerate dependencies: ",
                    PhGetStringOrDefault(errorMessage, L"Unknown error.")
                    )->Buffer);

                ShowWindow(GetDlgItem(WindowHandle, IDC_SERVICES_LAYOUT), SW_SHOW);
                PhClearReference(&errorMessage);
            }

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhFree(context);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);

            if (context->ServiceListHandle)
                EspLayoutServiceListControl(WindowHandle, context->ServiceListHandle);
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK EspServiceDependentsDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PSERVICE_LIST_CONTEXT context;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = PhAllocate(sizeof(SERVICE_LIST_CONTEXT));
        memset(context, 0, sizeof(SERVICE_LIST_CONTEXT));

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
            LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
            PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)propSheetPage->lParam;
            HWND serviceListHandle;
            SC_HANDLE serviceHandle;
            NTSTATUS status;

            PhSetDialogItemText(WindowHandle, IDC_MESSAGE, L"The following services depend on this service:");

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_SERVICES_LAYOUT), NULL, PH_ANCHOR_ALL);

            status = PhOpenService(
                &serviceHandle,
                SERVICE_ENUMERATE_DEPENDENTS,
                PhGetString(serviceItem->Name)
                );

            if (NT_SUCCESS(status))
            {
                ULONG numberOfDependentServices;
                LPENUM_SERVICE_STATUS dependentServices;

                status = PhEnumDependentServices(
                    serviceHandle,
                    &dependentServices,
                    &numberOfDependentServices
                    );

                if (NT_SUCCESS(status))
                {
                    PPH_SERVICE_ITEM dependentService;
                    PPH_LIST serviceList;

                    serviceList = PH_AUTO(PhCreateList(8));

                    for (ULONG i = 0; i < numberOfDependentServices; i++)
                    {
                        if (dependentService = PhReferenceServiceItem(dependentServices[i].lpServiceName))
                            PhAddItemList(serviceList, dependentService);
                    }

                    if (serviceList->Count)
                    {
                        PPH_SERVICE_ITEM* services;

                        services = PhAllocateCopy(serviceList->Items, sizeof(PPH_SERVICE_ITEM) * serviceList->Count);

                        serviceListHandle = PhCreateServiceListControl(WindowHandle, services, serviceList->Count);
                        context->ServiceListHandle = serviceListHandle;
                        EspLayoutServiceListControl(WindowHandle, serviceListHandle);
                        ShowWindow(serviceListHandle, SW_SHOW);
                    }

                    PhFree(dependentServices);
                }

                PhCloseServiceHandle(serviceHandle);
            }

            if (!NT_SUCCESS(status))
            {
                PPH_STRING errorMessage = PhGetNtMessage(status);

                PhSetDialogItemText(WindowHandle, IDC_SERVICES_LAYOUT, PhaConcatStrings2(
                    L"Unable to enumerate dependents: ",
                    PhGetStringOrDefault(errorMessage, L"Unknown error.")
                    )->Buffer);

                ShowWindow(GetDlgItem(WindowHandle, IDC_SERVICES_LAYOUT), SW_SHOW);
                PhClearReference(&errorMessage);
            }

            PhInitializeWindowTheme(WindowHandle, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);

            PhDeleteLayoutManager(&context->LayoutManager);

            PhFree(context);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);

            if (context->ServiceListHandle)
                EspLayoutServiceListControl(WindowHandle, context->ServiceListHandle);
        }
        break;
    }

    return FALSE;
}
