/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2017-2026
 *
 */

#include <phapp.h>
#include <phplug.h>
#include <svcsup.h>
#include <settings.h>
#include <phsettings.h>
#include <emenu.h>

#include <actions.h>
#include <srvprv.h>
#include <mainwnd.h>

#include <proctree.h>
#include <mainwndp.h>
#include <hndlinfo.h>

typedef struct _PH_SERVICES_CONTEXT
{
    PPH_SERVICE_ITEM *Services;
    ULONG NumberOfServices;
    PH_CALLBACK_REGISTRATION ModifiedEventRegistration;

    PWSTR ListViewSettingName;

    PH_LAYOUT_MANAGER LayoutManager;
    HWND WindowHandle;
    HWND ListViewHandle;
    HFONT TreeNewFont;
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

/**
 * Callback function for service list modification events.
 *
 * \param Parameter The callback parameter.
 * \param Context The callback context.
 */
_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI PhServiceListModifiedHandler(
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

/**
 * Updates the state of the service controls.
 *
 * \param hWnd The handle to the dialog.
 * \param ServiceItem The service item.
 */
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

VOID PhpStartSelectedServices(
    _In_ HWND hWnd,
    _In_ HWND ListViewHandle
    )
{
    PPH_SERVICE_ITEM *serviceItems;
    PPH_SERVICE_ITEM *startServices;
    PPH_SERVICE_ITEM *stopServices;
    ULONG numberOfItems;
    ULONG startCount = 0;
    ULONG stopCount = 0;

    if (!PhGetSelectedListViewItemParams(ListViewHandle, &serviceItems, &numberOfItems))
        return;

    startServices = PhAllocate(sizeof(PPH_SERVICE_ITEM) * numberOfItems);
    stopServices = PhAllocate(sizeof(PPH_SERVICE_ITEM) * numberOfItems);

    for (ULONG i = 0; i < numberOfItems; i++)
    {
        switch (((PPH_SERVICE_ITEM)serviceItems[i])->State)
        {
        case SERVICE_STOPPED:
            startServices[startCount++] = serviceItems[i];
            break;
        case SERVICE_RUNNING:
        case SERVICE_PAUSED:
            stopServices[stopCount++] = serviceItems[i];
            break;
        }
    }

    if (stopCount != 0)
    {
        PhReferenceObjects(stopServices, stopCount);
        PhUiStopServices(hWnd, stopServices, stopCount);
        PhDereferenceObjects(stopServices, stopCount);
    }

    if (startCount != 0)
    {
        PhReferenceObjects(startServices, startCount);
        PhUiStartServices(hWnd, startServices, startCount);
        PhDereferenceObjects(startServices, startCount);
    }

    PhFree(startServices);
    PhFree(stopServices);
    PhFree(serviceItems);
}

VOID PhpPauseSelectedServices(
    _In_ HWND hWnd,
    _In_ HWND ListViewHandle
    )
{
    PPH_SERVICE_ITEM *serviceItems;
    PPH_SERVICE_ITEM *pauseServices;
    PPH_SERVICE_ITEM *continueServices;
    ULONG numberOfItems;
    ULONG pauseCount = 0;
    ULONG continueCount = 0;

    if (!PhGetSelectedListViewItemParams(ListViewHandle, &serviceItems, &numberOfItems))
        return;

    pauseServices = PhAllocate(sizeof(PPH_SERVICE_ITEM) * numberOfItems);
    continueServices = PhAllocate(sizeof(PPH_SERVICE_ITEM) * numberOfItems);

    for (ULONG i = 0; i < numberOfItems; i++)
    {
        switch (((PPH_SERVICE_ITEM)serviceItems[i])->State)
        {
        case SERVICE_RUNNING:
            pauseServices[pauseCount++] = serviceItems[i];
            break;
        case SERVICE_PAUSED:
            continueServices[continueCount++] = serviceItems[i];
            break;
        }
    }

    if (pauseCount != 0)
    {
        PhReferenceObjects(pauseServices, pauseCount);
        PhUiPauseServices(hWnd, pauseServices, pauseCount);
        PhDereferenceObjects(pauseServices, pauseCount);
    }

    if (continueCount != 0)
    {
        PhReferenceObjects(continueServices, continueCount);
        PhUiContinueServices(hWnd, continueServices, continueCount);
        PhDereferenceObjects(continueServices, continueCount);
    }

    PhFree(pauseServices);
    PhFree(continueServices);
    PhFree(serviceItems);
}

/**
 * Dialog procedure for the services page.
 *
 * \param hwndDlg The handle to the dialog.
 * \param uMsg The message being processed.
 * \param wParam Message-specific parameter.
 * \param lParam Message-specific parameter.
 * \return TRUE if the message was handled, otherwise FALSE.
 */
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

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 120, L"Name");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 220, L"Display name");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 220, L"File name");
            PhSetExtendedListView(context->ListViewHandle);

            if (PhTreeWindowFont)
            {
                context->TreeNewFont = PhCreateTreeWindowFont(PhGetWindowDpi(hwndDlg));
                SetWindowFont(context->ListViewHandle, context->TreeNewFont, FALSE);
            }

            for (ULONG i = 0; i < context->NumberOfServices; i++)
            {
                SC_HANDLE serviceHandle;
                PPH_SERVICE_ITEM serviceItem;
                LONG lvItemIndex;

                serviceItem = context->Services[i];
                lvItemIndex = PhAddListViewItem(context->ListViewHandle, MAXINT, serviceItem->Name->Buffer, serviceItem);
                PhSetListViewSubItem(context->ListViewHandle, lvItemIndex, 1, serviceItem->DisplayName->Buffer);

                if (NT_SUCCESS(PhOpenService(&serviceHandle, SERVICE_QUERY_CONFIG, PhGetString(serviceItem->Name))))
                {
                    PPH_STRING fileName;

                    if (NT_SUCCESS(PhGetServiceHandleFileName(serviceHandle, &serviceItem->Name->sr, &fileName)))
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

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackServiceProviderModifiedEvent),
                PhServiceListModifiedHandler,
                context,
                &context->ModifiedEventRegistration
                );
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

            if (context->TreeNewFont)
                DeleteFont(context->TreeNewFont);

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
                    // PPH_SERVICE_ITEM serviceItem = PhGetSelectedListViewItemParam(context->ListViewHandle);
                    //
                    // if (serviceItem)
                    // {
                    //     switch (serviceItem->State)
                    //     {
                    //     case SERVICE_RUNNING:
                    //         PhUiStopService(hwndDlg, serviceItem);
                    //         break;
                    //     case SERVICE_PAUSED:
                    //         PhUiStopService(hwndDlg, serviceItem);
                    //         break;
                    //     case SERVICE_STOPPED:
                    //         PhUiStartService(hwndDlg, serviceItem);
                    //         break;
                    //     }
                    // }

                    PhpStartSelectedServices(hwndDlg, context->ListViewHandle);
                }
                break;
            case IDC_PAUSE:
                {
                    // PPH_SERVICE_ITEM serviceItem = PhGetSelectedListViewItemParam(context->ListViewHandle);
                    //
                    // if (serviceItem)
                    // {
                    //     switch (serviceItem->State)
                    //     {
                    //     case SERVICE_RUNNING:
                    //         PhUiPauseService(hwndDlg, serviceItem);
                    //         break;
                    //     case SERVICE_PAUSED:
                    //         PhUiContinueService(hwndDlg, serviceItem);
                    //         break;
                    //     }
                    // }

                    PhpPauseSelectedServices(hwndDlg, context->ListViewHandle);
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
    case WM_DPICHANGED_AFTERPARENT:
        {
            if (PhTreeWindowFont)
            {
                HFONT treeNewFont;

                if (treeNewFont = PhCreateTreeWindowFont(PhGetWindowDpi(hwndDlg)))
                    PhSwapReferenceFont(&context->TreeNewFont, context->ListViewHandle, treeNewFont, TRUE);
            }

            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
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

                if (PhGetSelectedListViewItemParams(context->ListViewHandle, &listviewItems, &numberOfItems))
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"Go to service", NULL, NULL), ULONG_MAX);
                    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
                    PhServiceListInsertContextMenu(hwndDlg, menu, (PPH_SERVICE_ITEM*)listviewItems, numberOfItems);
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
                                    SetForegroundWindow(PhMwpServiceTreeNewHandle);

                                    SystemInformer_SelectTabPage(1);

                                    SetFocus(PhMwpServiceTreeNewHandle);

                                    SystemInformer_SelectServiceItem((PPH_SERVICE_ITEM)listviewItems[0]);
                                }
                                break;
                            case ID_SERVICE_GOTOPROCESS:
                                {
                                    PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)listviewItems[0];
                                    PPH_PROCESS_NODE processNode;

                                    if (serviceItem)
                                    {
                                        if (processNode = PhFindProcessNode(serviceItem->ProcessId))
                                        {
                                            SetForegroundWindow(PhMwpProcessTreeNewHandle);

                                            SystemInformer_SelectTabPage(0);

                                            SetFocus(PhMwpProcessTreeNewHandle);

                                            PhSelectAndEnsureVisibleProcessNode(processNode);
                                        }
                                        else
                                        {
                                            PhShowStatus(hwndDlg, L"The process does not exist.", STATUS_INVALID_CID, 0);
                                        }
                                    }
                                }
                                break;
                            case ID_SERVICE_START:
                                {
                                    PPH_SERVICE_ITEM* serviceItems = (PPH_SERVICE_ITEM*)listviewItems;
                                    ULONG numberOfServiceItems = numberOfItems;

                                    PhReferenceObjects(serviceItems, numberOfServiceItems);
                                    PhUiStartServices(hwndDlg, serviceItems, numberOfServiceItems);
                                    PhDereferenceObjects(serviceItems, numberOfServiceItems);
                                }
                                break;
                            case ID_SERVICE_CONTINUE:
                                {
                                    PPH_SERVICE_ITEM* serviceItems = (PPH_SERVICE_ITEM*)listviewItems;
                                    ULONG numberOfServiceItems = numberOfItems;

                                    PhReferenceObjects(serviceItems, numberOfServiceItems);
                                    PhUiContinueServices(hwndDlg, serviceItems, numberOfServiceItems);
                                    PhDereferenceObjects(serviceItems, numberOfServiceItems);
                                }
                                break;
                            case ID_SERVICE_PAUSE:
                                {
                                    PPH_SERVICE_ITEM* serviceItems = (PPH_SERVICE_ITEM*)listviewItems;
                                    ULONG numberOfServiceItems = numberOfItems;

                                    PhReferenceObjects(serviceItems, numberOfServiceItems);
                                    PhUiPauseServices(hwndDlg, serviceItems, numberOfServiceItems);
                                    PhDereferenceObjects(serviceItems, numberOfServiceItems);
                                }
                                break;
                            case ID_SERVICE_STOP:
                                {
                                    PPH_SERVICE_ITEM* serviceItems = (PPH_SERVICE_ITEM*)listviewItems;
                                    ULONG numberOfServiceItems = numberOfItems;

                                    PhReferenceObjects(serviceItems, numberOfServiceItems);
                                    PhUiStopServices(hwndDlg, serviceItems, numberOfServiceItems);
                                    PhDereferenceObjects(serviceItems, numberOfServiceItems);
                                }
                                break;
                            case ID_SERVICE_DELETE:
                                {
                                    PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)listviewItems[0];

                                    if (serviceItem)
                                    {
                                        PhReferenceObject(serviceItem);

                                        if (PhUiDeleteService(hwndDlg, serviceItem))
                                        {
                                            LONG lvItemIndex;

                                            lvItemIndex = PhFindListViewItemByFlags(context->ListViewHandle, INT_ERROR, LVNI_SELECTED);

                                            if (lvItemIndex != INT_ERROR)
                                            {
                                                PhRemoveListViewItem(context->ListViewHandle, lvItemIndex);
                                            }
                                        }

                                        PhDereferenceObject(serviceItem);
                                    }
                                }
                                break;
                            case ID_SERVICE_OPENKEY:
                                {
                                    PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)listviewItems[0];

                                    if (serviceItem)
                                    {
                                        HANDLE keyHandle;

                                        if (NT_SUCCESS(PhOpenServiceKey(
                                            &keyHandle,
                                            KEY_READ,
                                            &serviceItem->Name->sr
                                            )))
                                        {
                                            PPH_STRING hklmServiceKeyName;

                                            if (NT_SUCCESS(PhQueryObjectName(keyHandle, &hklmServiceKeyName)))
                                            {
                                                PhMoveReference(&hklmServiceKeyName, PhFormatNativeKeyName(hklmServiceKeyName));

                                                PhShellOpenKey2(hwndDlg, hklmServiceKeyName);

                                                PhDereferenceObject(hklmServiceKeyName);
                                            }
                                            else
                                            {
                                                PhShowStatus(hwndDlg, L"The service does not exist.", STATUS_OBJECT_NAME_NOT_FOUND, 0);
                                            }

                                            NtClose(keyHandle);
                                        }
                                        else
                                        {
                                            PhShowStatus(hwndDlg, L"The service does not exist.", STATUS_OBJECT_NAME_NOT_FOUND, 0);
                                        }
                                    }
                                }
                                break;
                            case ID_SERVICE_OPENFILELOCATION:
                                {
                                    PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)listviewItems[0];
                                    NTSTATUS status;
                                    SC_HANDLE serviceHandle;

                                    if (serviceItem)
                                    {
                                        status = PhOpenService(&serviceHandle, SERVICE_QUERY_CONFIG, PhGetString(serviceItem->Name));

                                        if (NT_SUCCESS(status))
                                        {
                                            PPH_STRING fileName;

                                            status = PhGetServiceHandleFileName(serviceHandle, &serviceItem->Name->sr, &fileName);

                                            if (NT_SUCCESS(status))
                                            {
                                                PhShellExecuteUserString(
                                                    hwndDlg,
                                                    SETTING_FILE_BROWSE_EXECUTABLE,
                                                    PhGetString(fileName),
                                                    FALSE,
                                                    L"Make sure the Explorer executable file is present."
                                                    );
                                                PhDereferenceObject(fileName);
                                            }
                                            else
                                            {
                                                PhShowStatus(hwndDlg, L"Unable to locate the file.", status, 0);
                                            }

                                            PhCloseServiceHandle(serviceHandle);
                                        }
                                        else
                                        {
                                            PhShowStatus(hwndDlg, L"Unable to locate the file.", status, 0);
                                        }
                                    }
                                }
                                break;
                            case ID_SERVICE_PROPERTIES:
                                {
                                    PPH_SERVICE_ITEM serviceItem = listviewItems[0];

                                    if (serviceItem)
                                    {
                                        // The object relies on the list view reference, which could
                                        // disappear if we don't reference the object here.
                                        PhReferenceObject(serviceItem);
                                        PhShowServiceProperties(hwndDlg, serviceItem);
                                        PhDereferenceObject(serviceItem);
                                    }
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
