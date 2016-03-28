/*
 * Process Hacker -
 *   Process properties: Services page
 *
 * Copyright (C) 2009-2016 wj32
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
#include <procprv.h>
#include <procprpp.h>

static VOID PhpLayoutServiceListControl(
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

INT_PTR CALLBACK PhpProcessServicesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;

    if (!PhpPropPageDlgProcHeader(hwndDlg, uMsg, lParam,
        &propSheetPage, &propPageContext, &processItem))
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PPH_SERVICE_ITEM *services;
            ULONG numberOfServices;
            ULONG i;
            HWND serviceListHandle;

            // Get a copy of the process' service list.

            PhAcquireQueuedLockShared(&processItem->ServiceListLock);

            numberOfServices = processItem->ServiceList->Count;
            services = PhAllocate(numberOfServices * sizeof(PPH_SERVICE_ITEM));

            {
                ULONG enumerationKey = 0;
                PPH_SERVICE_ITEM serviceItem;

                i = 0;

                while (PhEnumPointerList(processItem->ServiceList, &enumerationKey, &serviceItem))
                {
                    PhReferenceObject(serviceItem);
                    services[i++] = serviceItem;
                }
            }

            PhReleaseQueuedLockShared(&processItem->ServiceListLock);

            serviceListHandle = PhCreateServiceListControl(
                hwndDlg,
                services,
                numberOfServices
                );
            SendMessage(serviceListHandle, WM_PH_SET_LIST_VIEW_SETTINGS, 0, (LPARAM)L"ProcessServiceListViewColumns");
            ShowWindow(serviceListHandle, SW_SHOW);

            propPageContext->Context = serviceListHandle;
        }
        break;
    case WM_DESTROY:
        {
            PhpPropPageDlgProcDestroy(hwndDlg);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PhAddPropPageLayoutItem(hwndDlg, hwndDlg,
                    PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_SERVICES_LAYOUT),
                    dialogItem, PH_ANCHOR_ALL);

                PhDoPropPageLayout(hwndDlg);
                PhpLayoutServiceListControl(hwndDlg, (HWND)propPageContext->Context);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_SIZE:
        {
            PhpLayoutServiceListControl(hwndDlg, (HWND)propPageContext->Context);
        }
        break;
    }

    return FALSE;
}
