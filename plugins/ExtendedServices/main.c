#include <phdk.h>
#include <windowsx.h>
#include "resource.h"

typedef struct _SERVICE_LIST_CONTEXT
{
    HWND ServiceListHandle;
    PH_LAYOUT_MANAGER LayoutManager;
} SERVICE_LIST_CONTEXT, *PSERVICE_LIST_CONTEXT;

VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ServicePropertiesInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION ServicePropertiesInitializingCallbackRegistration;

LOGICAL DllMain(
    __in HINSTANCE Instance,
    __in ULONG Reason,
    __reserved PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PH_PLUGIN_INFORMATION info;

            info.DisplayName = L"Extended Services";
            info.Author = L"wj32";
            info.Description = L"Adds more tabs to service properties.";
            info.HasOptions = FALSE;

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.ExtendedServices", Instance, &info);
            
            if (!PluginInstance)
                return FALSE;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackServicePropertiesInitializaing),
                ServicePropertiesInitializingCallback,
                NULL,
                &ServicePropertiesInitializingCallbackRegistration
                );
        }
        break;
    }

    return TRUE;
}

VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    // Nothing
}

static VOID PhpLayoutServiceListControl(
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

INT_PTR CALLBACK PhpServiceDependenciesDlgProc(      
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

                            if (dependencyService = PhReferenceServiceItem(dependency))
                                PhAddItemList(serviceList, dependencyService);

                            dependency += dependencyLength + 1;
                        }
                    }

                    services = PhAllocateCopy(serviceList->Items, sizeof(PPH_SERVICE_ITEM) * serviceList->Count);

                    serviceListHandle = PhCreateServiceListControl(hwndDlg, services, serviceList->Count);
                    context->ServiceListHandle = serviceListHandle;
                    PhpLayoutServiceListControl(hwndDlg, serviceListHandle);
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
                PhpLayoutServiceListControl(hwndDlg, context->ServiceListHandle);
        }
        break;
    }

    return FALSE;
}

LPENUM_SERVICE_STATUS PhEnumDependentServices(
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

INT_PTR CALLBACK PhpServiceDependentsDlgProc(      
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

                if (dependentServices = PhEnumDependentServices(serviceHandle, 0, &numberOfDependentServices))
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
                    PhpLayoutServiceListControl(hwndDlg, serviceListHandle);
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
                PhpLayoutServiceListControl(hwndDlg, context->ServiceListHandle);
        }
        break;
    }

    return FALSE;
}

VOID NTAPI ServicePropertiesInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_OBJECT_PROPERTIES objectProperties = Parameter;
    PROPSHEETPAGE propSheetPage;
    PPH_SERVICE_ITEM serviceItem;

    serviceItem = objectProperties->Parameter;

    if (objectProperties->NumberOfPages < objectProperties->MaximumNumberOfPages)
    {
        // Dependencies

        memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
        propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
        propSheetPage.dwFlags = PSP_USETITLE;
        propSheetPage.hInstance = PluginInstance->DllBase;
        propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_SRVLIST);
        propSheetPage.pszTitle = L"Dependencies";
        propSheetPage.pfnDlgProc = PhpServiceDependenciesDlgProc;
        propSheetPage.lParam = (LPARAM)serviceItem;
        objectProperties->Pages[objectProperties->NumberOfPages++] = CreatePropertySheetPage(&propSheetPage);

        // Dependents

        memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
        propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
        propSheetPage.dwFlags = PSP_USETITLE;
        propSheetPage.hInstance = PluginInstance->DllBase;
        propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_SRVLIST);
        propSheetPage.pszTitle = L"Dependents";
        propSheetPage.pfnDlgProc = PhpServiceDependentsDlgProc;
        propSheetPage.lParam = (LPARAM)serviceItem;
        objectProperties->Pages[objectProperties->NumberOfPages++] = CreatePropertySheetPage(&propSheetPage);
    }
}
