/*
 * Process Hacker - 
 *   service list control
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

#include <phapp.h>

typedef struct _PH_SERVICES_CONTEXT
{
    PPH_SERVICE_ITEM *Services;
    ULONG NumberOfServices;
    PH_CALLBACK_REGISTRATION ModifiedEventRegistration;

    PWSTR ListViewSettingName;

    PH_LAYOUT_MANAGER LayoutManager;
    HWND WindowHandle;
} PH_SERVICES_CONTEXT, *PPH_SERVICES_CONTEXT;

VOID NTAPI ServiceModifiedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

INT_PTR CALLBACK PhpServicesPageProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
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
    __in HWND ParentWindowHandle,
    __in PPH_SERVICE_ITEM *Services,
    __in ULONG NumberOfServices
    )
{
    HWND windowHandle;
    PPH_SERVICES_CONTEXT servicesContext;

    servicesContext = PhAllocate(sizeof(PH_SERVICES_CONTEXT));

    memset(servicesContext, 0, sizeof(PH_SERVICES_CONTEXT));
    servicesContext->Services = Services;
    servicesContext->NumberOfServices = NumberOfServices;

    windowHandle = CreateDialogParam(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_SRVLIST),
        ParentWindowHandle,
        PhpServicesPageProc,
        (LPARAM)servicesContext
        );

    if (!windowHandle)
    {
        PhFree(servicesContext);
        return windowHandle;
    }

    return windowHandle;
}

static VOID NTAPI ServiceModifiedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_SERVICE_MODIFIED_DATA serviceModifiedData = (PPH_SERVICE_MODIFIED_DATA)Parameter;
    PPH_SERVICES_CONTEXT servicesContext = (PPH_SERVICES_CONTEXT)Context;
    PPH_SERVICE_MODIFIED_DATA copy;

    copy = PhAllocateCopy(serviceModifiedData, sizeof(PH_SERVICE_MODIFIED_DATA));

    PostMessage(servicesContext->WindowHandle, WM_PH_SERVICE_MODIFIED, 0, (LPARAM)copy);
}

VOID PhpFixProcessServicesControls(
    __in HWND hWnd,
    __in_opt PPH_SERVICE_ITEM ServiceItem
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
                SetWindowText(startButton, L"S&top");
                SetWindowText(pauseButton, L"&Pause");
                EnableWindow(startButton, ServiceItem->ControlsAccepted & SERVICE_ACCEPT_STOP);
                EnableWindow(pauseButton, ServiceItem->ControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE);
            }
            break;
        case SERVICE_PAUSED:
            {
                SetWindowText(startButton, L"S&top");
                SetWindowText(pauseButton, L"C&ontinue");
                EnableWindow(startButton, ServiceItem->ControlsAccepted & SERVICE_ACCEPT_STOP);
                EnableWindow(pauseButton, ServiceItem->ControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE);
            }
            break;
        case SERVICE_STOPPED:
            {
                SetWindowText(startButton, L"&Start");
                SetWindowText(pauseButton, L"&Pause");
                EnableWindow(startButton, TRUE);
                EnableWindow(pauseButton, FALSE);
            }
            break;
        case SERVICE_START_PENDING:
        case SERVICE_CONTINUE_PENDING:
        case SERVICE_PAUSE_PENDING:
        case SERVICE_STOP_PENDING:
            {
                SetWindowText(startButton, L"&Start");
                SetWindowText(pauseButton, L"&Pause");
                EnableWindow(startButton, FALSE);
                EnableWindow(pauseButton, FALSE);
            }
            break;
        }

        if (serviceHandle = PhOpenService(
            ServiceItem->Name->Buffer,
            SERVICE_QUERY_CONFIG
            ))
        {
            if (description = PhGetServiceDescription(serviceHandle))
            {
                SetWindowText(descriptionLabel, description->Buffer);
                PhDereferenceObject(description);
            }

            CloseServiceHandle(serviceHandle);
        }
    }
    else
    {
        SetWindowText(startButton, L"&Start");
        SetWindowText(pauseButton, L"&Pause");
        EnableWindow(startButton, FALSE);
        EnableWindow(pauseButton, FALSE);
        SetWindowText(descriptionLabel, L"");
    }
}

INT_PTR CALLBACK PhpServicesPageProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    PPH_SERVICES_CONTEXT servicesContext;
    HWND lvHandle;

    if (uMsg == WM_INITDIALOG)
    {
        servicesContext = (PPH_SERVICES_CONTEXT)lParam;
        SetProp(hwndDlg, PhMakeContextAtom(), (HANDLE)servicesContext);
    }
    else
    {
        servicesContext = (PPH_SERVICES_CONTEXT)GetProp(hwndDlg, PhMakeContextAtom());

        if (uMsg == WM_DESTROY)
        {
            RemoveProp(hwndDlg, PhMakeContextAtom());
        }
    }

    if (!servicesContext)
        return FALSE;

    lvHandle = GetDlgItem(hwndDlg, IDC_LIST);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            ULONG i;

            PhRegisterCallback(
                &PhServiceModifiedEvent,
                ServiceModifiedHandler,
                servicesContext,
                &servicesContext->ModifiedEventRegistration
                );

            servicesContext->WindowHandle = hwndDlg;

            // Initialize the list.
            PhSetListViewStyle(lvHandle, TRUE, TRUE);
            PhSetControlTheme(lvHandle, L"explorer");
            PhAddListViewColumn(lvHandle, 0, 0, 0, LVCFMT_LEFT, 120, L"Name");
            PhAddListViewColumn(lvHandle, 1, 1, 1, LVCFMT_LEFT, 220, L"Display Name");

            PhSetExtendedListView(lvHandle);

            for (i = 0; i < servicesContext->NumberOfServices; i++)
            {
                PPH_SERVICE_ITEM serviceItem;
                INT lvItemIndex;

                serviceItem = servicesContext->Services[i];
                lvItemIndex = PhAddListViewItem(lvHandle, MAXINT, serviceItem->Name->Buffer, serviceItem);
                PhSetListViewSubItem(lvHandle, lvItemIndex, 1, serviceItem->DisplayName->Buffer);
            }

            ExtendedListView_SortItems(lvHandle);

            PhpFixProcessServicesControls(hwndDlg, NULL);

            PhInitializeLayoutManager(&servicesContext->LayoutManager, hwndDlg);
            PhAddLayoutItem(&servicesContext->LayoutManager, GetDlgItem(hwndDlg, IDC_LIST),
                NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&servicesContext->LayoutManager, GetDlgItem(hwndDlg, IDC_DESCRIPTION),
                NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&servicesContext->LayoutManager, GetDlgItem(hwndDlg, IDC_START),
                NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&servicesContext->LayoutManager, GetDlgItem(hwndDlg, IDC_PAUSE),
                NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
        }
        break;
    case WM_DESTROY:
        {
            ULONG i;

            for (i = 0; i < servicesContext->NumberOfServices; i++)
                PhDereferenceObject(servicesContext->Services[i]);

            PhFree(servicesContext->Services);

            PhUnregisterCallback(
                &PhServiceModifiedEvent,
                &servicesContext->ModifiedEventRegistration
                );

            if (servicesContext->ListViewSettingName)
                PhSaveListViewColumnsToSetting(servicesContext->ListViewSettingName, lvHandle);

            PhDeleteLayoutManager(&servicesContext->LayoutManager);

            PhFree(servicesContext);
        }
        break;
    case WM_COMMAND:
        {
            INT id = LOWORD(wParam);

            switch (id)
            {
            case IDC_START:
                {
                    PPH_SERVICE_ITEM serviceItem = PhGetSelectedListViewItemParam(lvHandle);

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
                    PPH_SERVICE_ITEM serviceItem = PhGetSelectedListViewItemParam(lvHandle);

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

            PhHandleListViewNotifyForCopy(lParam, lvHandle);

            switch (header->code)
            {
            case NM_DBLCLK:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        PPH_SERVICE_ITEM serviceItem = PhGetSelectedListViewItemParam(lvHandle);

                        if (serviceItem)
                        {
                            PhShowServiceProperties(hwndDlg, serviceItem);
                        }
                    }
                }
                break;
            case LVN_ITEMCHANGED:
                {
                    if (header->hwndFrom == lvHandle)
                    {
                        //LPNMITEMACTIVATE itemActivate = (LPNMITEMACTIVATE)header;
                        PPH_SERVICE_ITEM serviceItem = NULL;

                        if (ListView_GetSelectedCount(lvHandle) == 1)
                            serviceItem = PhGetSelectedListViewItemParam(lvHandle);

                        PhpFixProcessServicesControls(hwndDlg, serviceItem);
                    }
                }
                break;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&servicesContext->LayoutManager);
        }
        break;
    case WM_PH_SERVICE_MODIFIED:
        {
            PPH_SERVICE_MODIFIED_DATA serviceModifiedData = (PPH_SERVICE_MODIFIED_DATA)lParam;
            PPH_SERVICE_ITEM serviceItem = NULL;

            if (ListView_GetSelectedCount(lvHandle) == 1)
                serviceItem = PhGetSelectedListViewItemParam(lvHandle);

            if (serviceModifiedData->Service == serviceItem)
            {
                PhpFixProcessServicesControls(hwndDlg, serviceItem);
            }

            PhFree(serviceModifiedData);
        }
        break;
    case WM_PH_SET_LIST_VIEW_SETTINGS:
        {
            PWSTR settingName = (PWSTR)lParam;

            servicesContext->ListViewSettingName = settingName;
            PhLoadListViewColumnsFromSetting(settingName, lvHandle);
        }
        break;
    }

    return FALSE;
}
