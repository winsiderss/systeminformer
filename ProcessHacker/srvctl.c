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

#include <svcsup.h>
#include <settings.h>

#include <actions.h>
#include <mainwnd.h>
#include <srvprv.h>

#include <windowsx.h>

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

VOID NTAPI ServiceModifiedHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    );

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
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
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

        if (uMsg == WM_DESTROY)
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            ULONG i;

            PhRegisterCallback(
                &PhServiceModifiedEvent,
                ServiceModifiedHandler,
                context,
                &context->ModifiedEventRegistration
                );

            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            // Initialize the list.
            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 120, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 220, L"Display name");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 220, L"File name");

            PhSetExtendedListView(context->ListViewHandle);

            for (i = 0; i < context->NumberOfServices; i++)
            {
                SC_HANDLE serviceHandle;
                PPH_SERVICE_ITEM serviceItem;
                INT lvItemIndex;

                serviceItem = context->Services[i];
                lvItemIndex = PhAddListViewItem(context->ListViewHandle, MAXINT, serviceItem->Name->Buffer, serviceItem);
                PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 1, serviceItem->DisplayName->Buffer);

                if (serviceHandle = PhOpenService(serviceItem->Name->Buffer, SERVICE_QUERY_CONFIG))
                {
                    PPH_STRING fileName;

                    if (fileName = PhGetServiceRelevantFileName(&serviceItem->Name->sr, serviceHandle))
                    {
                        PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 2, PhGetStringOrEmpty(fileName));
                        PhDereferenceObject(fileName);
                    }

                    CloseServiceHandle(serviceHandle);
                }
            }

            ExtendedListView_SortItems(context->ListViewHandle);

            PhpFixProcessServicesControls(hwndDlg, NULL);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_DESCRIPTION), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_START), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_PAUSE), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
        }
        break;
    case WM_DESTROY:
        {
            ULONG i;

            for (i = 0; i < context->NumberOfServices; i++)
                PhDereferenceObject(context->Services[i]);

            PhFree(context->Services);

            PhUnregisterCallback(
                &PhServiceModifiedEvent,
                &context->ModifiedEventRegistration
                );

            if (context->ListViewSettingName)
                PhSaveListViewColumnsToSetting(context->ListViewSettingName, context->ListViewHandle);

            PhDeleteLayoutManager(&context->LayoutManager);

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

            context->ListViewSettingName = settingName;
            PhLoadListViewColumnsFromSetting(settingName, context->ListViewHandle);
        }
        break;
    }

    return FALSE;
}
