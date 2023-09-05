/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2017-2023
 *
 */

#include <phapp.h>
#include <phplug.h>
#include <svcsup.h>
#include <settings.h>
#include <emenu.h>

#include <actions.h>
#include <srvprv.h>
#include <mainwnd.h>

typedef struct _PH_SERVICES_CONTEXT
{
    PPH_SERVICE_ITEM *Services;
    ULONG NumberOfServices;
    PH_CALLBACK_REGISTRATION ModifiedEventRegistration;

    PWSTR ListViewSettingName;

    PH_LAYOUT_MANAGER LayoutManager;
    HWND WindowHandle;
    HWND ListViewHandle;
} PH_SERVICES_CONTEXT, *PPH_SERVICES_CONTEXT;

#define WM_PH_SERVICE_PAGE_MODIFIED (WM_APP + 106)

INT_PTR CALLBACK PhpServicesPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    );

/**
 * Creates a service list property page.
 *
 * \param ParentWindowHandle The parent of the service list.
 * \param Services An array of service items. Each
 * service item must have a reference that is transferred
 * to this function. The array must be allocated using
 * PhAllocate() and must not be freed by the caller; it
 * will be freed automatically when no longer needed.
 * \param NumberOfServices The number of service items
 * in \a Services.
 */
HWND PhCreateServiceListControl(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_SERVICE_ITEM *Services,
    _In_ ULONG NumberOfServices
    )
{
    HWND windowHandle;
    PPH_SERVICES_CONTEXT servicesContext;

    servicesContext = PhAllocateZero(sizeof(PH_SERVICES_CONTEXT));
    servicesContext->Services = Services;
    servicesContext->NumberOfServices = NumberOfServices;

    windowHandle = PhCreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_SRVLIST),
        ParentWindowHandle,
        PhpServicesPageProc,
        servicesContext
        );

    if (!windowHandle)
    {
        PhFree(servicesContext);
        return windowHandle;
    }

    return windowHandle;
}

VOID NTAPI ServiceModifiedHandler(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_SERVICE_MODIFIED_DATA serviceModifiedData = (PPH_SERVICE_MODIFIED_DATA)Parameter;
    PPH_SERVICES_CONTEXT servicesContext = (PPH_SERVICES_CONTEXT)Context;
    PPH_SERVICE_MODIFIED_DATA copy;

    copy = PhAllocateCopy(serviceModifiedData, sizeof(PH_SERVICE_MODIFIED_DATA));

    PostMessage(servicesContext->WindowHandle, WM_PH_SERVICE_PAGE_MODIFIED, 0, (LPARAM)copy);
}

VOID PhpFixProcessServicesControls(
    _In_ HWND hWnd,
    _In_opt_ PPH_SERVICE_ITEM ServiceItem
    )
{
    HWND startButton;
    HWND pauseButton;
    HWND descriptionLabel;

    startButton = GetDlgItem(hWnd, IDC_START);
    pauseButton = GetDlgItem(hWnd, IDC_PAUSE);
    descriptionLabel = GetDlgItem(hWnd, IDC_DESCRIPTION);

    if (ServiceItem)
    {
        SC_HANDLE serviceHandle;
        PPH_STRING description;

        switch (ServiceItem->State)
        {
        case SERVICE_RUNNING:
            {
                PhSetWindowText(startButton, L"S&top");
                PhSetWindowText(pauseButton, L"&Pause");
                EnableWindow(startButton, ServiceItem->ControlsAccepted & SERVICE_ACCEPT_STOP);
                EnableWindow(pauseButton, ServiceItem->ControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE);
            }
            break;
        case SERVICE_PAUSED:
            {
                PhSetWindowText(startButton, L"S&top");
                PhSetWindowText(pauseButton, L"C&ontinue");
                EnableWindow(startButton, ServiceItem->ControlsAccepted & SERVICE_ACCEPT_STOP);
                EnableWindow(pauseButton, ServiceItem->ControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE);
            }
            break;
        case SERVICE_STOPPED:
            {
                PhSetWindowText(startButton, L"&Start");
                PhSetWindowText(pauseButton, L"&Pause");
                EnableWindow(startButton, TRUE);
                EnableWindow(pauseButton, FALSE);
            }
            break;
        case SERVICE_START_PENDING:
        case SERVICE_CONTINUE_PENDING:
        case SERVICE_PAUSE_PENDING:
        case SERVICE_STOP_PENDING:
            {
                PhSetWindowText(startButton, L"&Start");
                PhSetWindowText(pauseButton, L"&Pause");
                EnableWindow(startButton, FALSE);
                EnableWindow(pauseButton, FALSE);
            }
            break;
        }

        if (NT_SUCCESS(PhOpenService(&serviceHandle, SERVICE_QUERY_CONFIG, PhGetString(ServiceItem->Name))))
        {
            if (description = PhGetServiceDescription(serviceHandle))
            {
                PhSetWindowText(descriptionLabel, description->Buffer);
                PhDereferenceObject(description);
            }

            PhCloseServiceHandle(serviceHandle);
        }
    }
    else
    {
        PhSetWindowText(startButton, L"&Start");
        PhSetWindowText(pauseButton, L"&Pause");
        EnableWindow(startButton, FALSE);
        EnableWindow(pauseButton, FALSE);
        PhSetWindowText(descriptionLabel, L"");
    }
}

INT_PTR CALLBACK PhpServicesPageProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_SERVICES_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_SERVICES_CONTEXT)lParam;
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackServiceProviderModifiedEvent),
                ServiceModifiedHandler,
                context,
                &context->ModifiedEventRegistration
                );

            // Initialize the list.
            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 120, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 220, L"Display name");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 220, L"File name");

            PhSetExtendedListView(context->ListViewHandle);

            for (ULONG i = 0; i < context->NumberOfServices; i++)
            {
                SC_HANDLE serviceHandle;
                PPH_SERVICE_ITEM serviceItem;
                INT lvItemIndex;

                serviceItem = context->Services[i];
                lvItemIndex = PhAddListViewItem(context->ListViewHandle, MAXINT, serviceItem->Name->Buffer, serviceItem);
                PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 1, serviceItem->DisplayName->Buffer);

                if (NT_SUCCESS(PhOpenService(&serviceHandle, SERVICE_QUERY_CONFIG, PhGetString(serviceItem->Name))))
                {
                    PPH_STRING fileName;

                    if (fileName = PhGetServiceHandleFileName(serviceHandle, &serviceItem->Name->sr))
                    {
                        PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 2, PhGetStringOrEmpty(fileName));
                        PhDereferenceObject(fileName);
                    }

                    PhCloseServiceHandle(serviceHandle);
                }
            }

            ExtendedListView_SortItems(context->ListViewHandle);

            if (context->NumberOfServices > 0)
            {
                SetFocus(context->ListViewHandle);
                ListView_SetItemState(context->ListViewHandle, 0, LVNI_SELECTED, LVNI_SELECTED);
                ListView_EnsureVisible(context->ListViewHandle, 0, FALSE);
                PhpFixProcessServicesControls(hwndDlg, context->Services[0]);
            }
            else
            {
                PhpFixProcessServicesControls(hwndDlg, NULL);
            }

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_DESCRIPTION), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_START), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_PAUSE), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            PhUnregisterCallback(
                PhGetGeneralCallback(GeneralCallbackServiceProviderModifiedEvent),
                &context->ModifiedEventRegistration
                );

            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->ListViewSettingName)
                PhSaveListViewColumnsToSetting(context->ListViewSettingName, context->ListViewHandle);

            for (ULONG i = 0; i < context->NumberOfServices; i++)
                PhDereferenceObject(context->Services[i]);

            PhFree(context->Services);

            PhFree(context);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_START:
                {
                    PPH_SERVICE_ITEM serviceItem = PhGetSelectedListViewItemParam(context->ListViewHandle);

                    if (serviceItem)
                    {
                        switch (serviceItem->State)
                        {
                        case SERVICE_RUNNING:
                            PhUiStopService(hwndDlg, serviceItem);
                            break;
                        case SERVICE_PAUSED:
                            PhUiStopService(hwndDlg, serviceItem);
                            break;
                        case SERVICE_STOPPED:
                            PhUiStartService(hwndDlg, serviceItem);
                            break;
                        }
                    }
                }
                break;
            case IDC_PAUSE:
                {
                    PPH_SERVICE_ITEM serviceItem = PhGetSelectedListViewItemParam(context->ListViewHandle);

                    if (serviceItem)
                    {
                        switch (serviceItem->State)
                        {
                        case SERVICE_RUNNING:
                            PhUiPauseService(hwndDlg, serviceItem);
                            break;
                        case SERVICE_PAUSED:
                            PhUiContinueService(hwndDlg, serviceItem);
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

            PhHandleListViewNotifyBehaviors(lParam, context->ListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);

            switch (header->code)
            {
            case NM_DBLCLK:
                {
                    if (header->hwndFrom == context->ListViewHandle)
                    {
                        PPH_SERVICE_ITEM serviceItem = PhGetSelectedListViewItemParam(context->ListViewHandle);

                        if (serviceItem)
                        {
                            PhShowServiceProperties(hwndDlg, serviceItem);
                        }
                    }
                }
                break;
            case LVN_ITEMCHANGED:
                {
                    if (header->hwndFrom == context->ListViewHandle)
                    {
                        //LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)header;
                        PPH_SERVICE_ITEM serviceItem = NULL;

                        if (ListView_GetSelectedCount(context->ListViewHandle) == 1)
                            serviceItem = PhGetSelectedListViewItemParam(context->ListViewHandle);

                        PhpFixProcessServicesControls(hwndDlg, serviceItem);
                    }
                }
                break;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_PH_SERVICE_PAGE_MODIFIED:
        {
            PPH_SERVICE_MODIFIED_DATA serviceModifiedData = (PPH_SERVICE_MODIFIED_DATA)lParam;
            PPH_SERVICE_ITEM serviceItem = NULL;

            if (ListView_GetSelectedCount(context->ListViewHandle) == 1)
                serviceItem = PhGetSelectedListViewItemParam(context->ListViewHandle);

            if (serviceModifiedData->ServiceItem == serviceItem)
            {
                PhpFixProcessServicesControls(hwndDlg, serviceItem);
            }

            PhFree(serviceModifiedData);
        }
        break;
    case WM_PH_SET_LIST_VIEW_SETTINGS:
        {
            PWSTR settingName = (PWSTR)lParam;

            context->ListViewSettingName = settingName;
            PhLoadListViewColumnsFromSetting(settingName, context->ListViewHandle);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->ListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID* listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->ListViewHandle, &point);

                PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"Go to service", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, IDC_COPY, context->ListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        BOOLEAN handled = FALSE;

                        handled = PhHandleCopyListViewEMenuItem(item);

                        //if (!handled && PhPluginsEnabled)
                        //    handled = PhPluginTriggerEMenuItem(&menuInfo, item);

                        if (!handled)
                        {
                            switch (item->Id)
                            {
                            case 1:
                                {
                                    ProcessHacker_SelectTabPage(1);
                                    ProcessHacker_SelectServiceItem((PPH_SERVICE_ITEM)listviewItems[0]);
                                }
                                break;
                            case IDC_COPY:
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
                                break;
                            }
                        }
                    }

                    PhDestroyEMenu(menu);
                }

                PhFree(listviewItems);
            }
        }
        break;
    }

    return FALSE;
}
