/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2009-2016
 *
 */

#include <phapp.h>
#include <phsettings.h>
#include <procprp.h>
#include <procprv.h>

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
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;

    if (!PhPropPageDlgProcHeader(hwndDlg, uMsg, lParam, NULL, &propPageContext, &processItem))
        return FALSE;

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

            PhInitializeWindowTheme(hwndDlg, PhEnableThemeSupport);
        }
        break;
    case WM_SHOWWINDOW:
        {
            PPH_LAYOUT_ITEM dialogItem;

            if (dialogItem = PhBeginPropPageLayout(hwndDlg, propPageContext))
            {
                PhAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_SERVICES_LAYOUT), dialogItem, PH_ANCHOR_ALL);
                PhEndPropPageLayout(hwndDlg, propPageContext);

                PhpLayoutServiceListControl(hwndDlg, (HWND)propPageContext->Context);
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
