/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     jxy-s   2023
 *
 */

#include "devices.h"

typedef enum _DEVICE_PROPERTIES_CATEGORY
{
    DEVICE_PROPERTIES_CATEGORY_GENERAL,
    DEVICE_PROPERTIES_CATEGORY_CLASS
} DEVICE_PROPERTIES_CATEGORY;

typedef enum _DEVICE_PROPERTIES_INDEX
{
    DEVICE_PROPERTIES_INDEX_DESCRIPTION,
    DEVICE_PROPERTIES_INDEX_INSTALLED,
    DEVICE_PROPERTIES_INDEX_FIRST_INSTALLED,
    DEVICE_PROPERTIES_INDEX_LAST_ARRIVAL,
    DEVICE_PROPERTIES_INDEX_LAST_REMOVAL,
    DEVICE_PROPERTIES_INDEX_PROBLEM_CODE,
    DEVICE_PROPERTIES_INDEX_PROBLEM_DESCRIPTION,
    DEVICE_PROPERTIES_INDEX_PROBLEM_STATUS,
    DEVICE_PROPERTIES_INDEX_DRIVER,
    DEVICE_PROPERTIES_INDEX_DRIVER_PROVIDER,
    DEVICE_PROPERTIES_INDEX_DRIVER_DESCRIPTION,
    DEVICE_PROPERTIES_INDEX_DRIVER_DATE,
    DEVICE_PROPERTIES_INDEX_DRIVER_VERSION,
    DEVICE_PROPERTIES_INDEX_DRIVER_INF,
    DEVICE_PROPERTIES_INDEX_DRIVER_INF_SECTION,
    DEVICE_PROPERTIES_INDEX_SERVICE,
    DEVICE_PROPERTIES_INDEX_SECURITY_DESCRIPTOR,
    DEVICE_PROPERTIES_INDEX_ENUMERATOR,
    DEVICE_PROPERTIES_INDEX_PDO_NAME,
    DEVICE_PROPERTIES_INDEX_INSTANCEID,
    DEVICE_PROPERTIES_INDEX_PARENT_INSTANCEID,
    DEVICE_PROPERTIES_INDEX_LOCATION_INFO,
    DEVICE_PROPERTIES_INDEX_LOCATION_PATHS,
    DEVICE_PROPERTIES_INDEX_MATCHING_ID,
    DEVICE_PROPERTIES_INDEX_HARDWARE_IDS,

    DEVICE_PROPERTIES_INDEX_CLASS_NAME,
    DEVICE_PROPERTIES_INDEX_CLASS_GUID,
    DEVICE_PROPERTIES_INDEX_CLASS_DEVICE_NAME,
    DEVICE_PROPERTIES_INDEX_CLASS_CLASS_NAME,
    DEVICE_PROPERTIES_INDEX_CLASS_INSTALLER,
    DEVICE_PROPERTIES_INDEX_CLASS_DEFAULT_SERVICE,
    DEVICE_PROPERTIES_INDEX_CLASS_PROPERTY_PAGE_PROVIDER,
} DEVICE_PROPERTIES_INDEX;

typedef struct _DEVICE_PROPERTIES_CONTEXT
{
    PPH_DEVICE_ITEM DeviceItem;
    HWND ParentWindowHandle;
    HWND GeneralListViewHandle;
    HWND PropsListViewHandle;
    HICON DeviceIcon;
    PH_INTEGER_PAIR DeviceIconSize;

    union
    {
        struct
        {
            ULONG GeneralPageRefreshed : 1;
            ULONG PropsPageRefreshed : 1;
        };
        ULONG Flags;
    };
} DEVICE_PROPERTIES_CONTEXT, *PDEVICE_PROPERTIES_CONTEXT;

VOID NTAPI DevicePropertiesDeleteProcedure(
    _In_ PVOID Object,
    _In_ ULONG Flags
    )
{
    PDEVICE_PROPERTIES_CONTEXT context = Object;
    PhDereferenceObject(context->DeviceItem);
}

VOID DeviceInitializeGeneralPageGroups(
    _In_ HWND ListViewHandle
    )
{
    ListView_EnableGroupView(ListViewHandle, TRUE);
    PhAddListViewGroup(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, L"General");
    PhAddListViewGroup(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_CLASS, L"Class");

    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_DESCRIPTION, L"Description", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_INSTALLED, L"Installed", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_FIRST_INSTALLED, L"First installed", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_LAST_ARRIVAL, L"Last arrival", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_LAST_REMOVAL, L"Last removal", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_PROBLEM_CODE, L"Problem code", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_PROBLEM_DESCRIPTION, L"Problem description", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_PROBLEM_STATUS, L"Problem status", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_DRIVER, L"Driver", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_DRIVER_PROVIDER, L"Driver provider", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_DRIVER_DESCRIPTION, L"Driver description", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_DRIVER_DATE, L"Driver date", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_DRIVER_VERSION, L"Driver version", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_DRIVER_INF, L"Driver INF", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_DRIVER_INF_SECTION, L"Driver INF section", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_SERVICE, L"Service", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_SECURITY_DESCRIPTOR, L"Security descriptor", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_ENUMERATOR, L"Enumerator", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_PDO_NAME, L"PDO name", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_INSTANCEID, L"Instance ID", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_PARENT_INSTANCEID, L"Parent instance ID", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_LOCATION_INFO, L"Location info", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_LOCATION_PATHS, L"Location paths", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_MATCHING_ID, L"Matching ID", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_GENERAL, DEVICE_PROPERTIES_INDEX_HARDWARE_IDS, L"Hardware IDs", NULL);

    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_CLASS, DEVICE_PROPERTIES_INDEX_CLASS_NAME, L"Name", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_CLASS, DEVICE_PROPERTIES_INDEX_CLASS_GUID, L"GUID", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_CLASS, DEVICE_PROPERTIES_INDEX_CLASS_DEVICE_NAME, L"Device name", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_CLASS, DEVICE_PROPERTIES_INDEX_CLASS_CLASS_NAME, L"Class name", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_CLASS, DEVICE_PROPERTIES_INDEX_CLASS_INSTALLER, L"Installer", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_CLASS, DEVICE_PROPERTIES_INDEX_CLASS_DEFAULT_SERVICE, L"Default service", NULL);
    PhAddListViewGroupItem(ListViewHandle, DEVICE_PROPERTIES_CATEGORY_CLASS, DEVICE_PROPERTIES_INDEX_CLASS_PROPERTY_PAGE_PROVIDER, L"Property page provider", NULL);
}

VOID DeviceInitializeGenealPageItems(
    _In_ PPH_DEVICE_ITEM DeviceItem,
    _In_ HWND ListViewHandle
    )
{
#define DEVICE_ADD_PAGE_ITEM(Index, Prop) PhSetListViewSubItem(ListViewHandle, Index, 1, PhGetStringOrEmpty(PhGetDeviceProperty(DeviceItem, Prop)->AsString))

    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_DESCRIPTION, PhDevicePropertyDeviceDesc);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_INSTALLED, PhDevicePropertyInstallDate);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_FIRST_INSTALLED, PhDevicePropertyFirstInstallDate);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_LAST_ARRIVAL, PhDevicePropertyLastArrivalDate);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_LAST_REMOVAL, PhDevicePropertyLastRemovalDate);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_PROBLEM_CODE, PhDevicePropertyProblemCode);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_PROBLEM_DESCRIPTION, PhDevicePropertyDriverProblemDesc);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_PROBLEM_STATUS, PhDevicePropertyProblemStatus);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_DRIVER, PhDevicePropertyDriver);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_DRIVER_PROVIDER, PhDevicePropertyDriverProvider);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_DRIVER_DESCRIPTION, PhDevicePropertyDriverDesc);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_DRIVER_DATE, PhDevicePropertyDriverDate);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_DRIVER_VERSION, PhDevicePropertyDriverVersion);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_DRIVER_INF, PhDevicePropertyDriverInfPath);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_DRIVER_INF_SECTION, PhDevicePropertyDriverInfSection);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_SERVICE, PhDevicePropertyService);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_SECURITY_DESCRIPTOR, PhDevicePropertySecuritySDS);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_ENUMERATOR, PhDevicePropertyEnumeratorName);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_PDO_NAME, PhDevicePropertyPDOName);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_INSTANCEID, PhDevicePropertyInstanceId);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_PARENT_INSTANCEID, PhDevicePropertyParentInstanceId);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_LOCATION_INFO, PhDevicePropertyLocationInfo);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_LOCATION_PATHS, PhDevicePropertyLocationPaths);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_MATCHING_ID, PhDevicePropertyMatchingDeviceId);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_HARDWARE_IDS, PhDevicePropertyHardwareIds);

    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_CLASS_NAME, PhDevicePropertyClass);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_CLASS_GUID, PhDevicePropertyClassGuid);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_CLASS_DEVICE_NAME, PhDevicePropertyClassName);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_CLASS_CLASS_NAME, PhDevicePropertyClassClassName);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_CLASS_INSTALLER, PhDevicePropertyClassClassInstaller);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_CLASS_DEFAULT_SERVICE, PhDevicePropertyClassDefaultService);
    DEVICE_ADD_PAGE_ITEM(DEVICE_PROPERTIES_INDEX_CLASS_PROPERTY_PAGE_PROVIDER, PhDevicePropertyClassPropPageProvider);
}

VOID DeviceInitializeGeneralPage(
    _In_ HWND hwndDlg,
    _In_ PDEVICE_PROPERTIES_CONTEXT Context
    )
{
    LONG dpi;

    ExtendedListView_SetRedraw(Context->GeneralListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->GeneralListViewHandle);

    dpi = PhGetWindowDpi(hwndDlg);
    Context->DeviceIconSize.X = PhGetSystemMetrics(SM_CXICON, dpi);
    Context->DeviceIconSize.Y = PhGetSystemMetrics(SM_CYICON, dpi);
    Context->DeviceIcon = PhGetDeviceIcon(Context->DeviceItem, &Context->DeviceIconSize);
    if (Context->DeviceIcon)
        Static_SetIcon(GetDlgItem(hwndDlg, IDC_DEVICE_ICON), Context->DeviceIcon);

    PhSetWindowText(
        GetDlgItem(hwndDlg, IDC_DEVICE_NAME),
        PhGetStringOrDefault(PhGetDeviceProperty(Context->DeviceItem, PhDevicePropertyName)->AsString, L"Unnamed Device")
        );
    PhSetWindowText(
        GetDlgItem(hwndDlg, IDC_DEVICE_MANUFACTURER),
        PhGetStringOrEmpty(PhGetDeviceProperty(Context->DeviceItem, PhDevicePropertyManufacturer)->AsString)
        );

    DeviceInitializeGeneralPageGroups(Context->GeneralListViewHandle);
    DeviceInitializeGenealPageItems(Context->DeviceItem, Context->GeneralListViewHandle);

    ExtendedListView_SetRedraw(Context->GeneralListViewHandle, TRUE);
}

INT_PTR CALLBACK DevicePropGeneralDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDEVICE_PROPERTIES_CONTEXT context;
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    context = (PDEVICE_PROPERTIES_CONTEXT)propPageContext->Context;

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->GeneralListViewHandle = GetDlgItem(hwndDlg, IDC_DEVICE_INFO);

            PhSetApplicationWindowIcon(GetParent(hwndDlg));

            PhSetListViewStyle(context->GeneralListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->GeneralListViewHandle, L"explorer");
            PhAddListViewColumn(context->GeneralListViewHandle, 0, 0, 0, LVCFMT_LEFT, 180, L"Name");
            PhAddListViewColumn(context->GeneralListViewHandle, 1, 1, 1, LVCFMT_LEFT, 300, L"Value");
            PhSetExtendedListView(context->GeneralListViewHandle);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_DEVICE_GENERAL_COLUMNS, context->GeneralListViewHandle);

            DeviceInitializeGeneralPage(hwndDlg, context);

            if (!PhGetIntegerPairSetting(SETTING_NAME_DEVICE_PROPERTIES_POSITION).X) // HACK
            {
                PhCenterWindow(GetParent(hwndDlg), context->ParentWindowHandle);
                ExtendedListView_SetColumnWidth(context->GeneralListViewHandle, 1, ELVSCW_AUTOSIZE_REMAININGSPACE);
            }

            if (!!PhGetIntegerSetting(L"EnableThemeSupport")) // TODO: Required for compat (dmex)
                PhInitializeWindowTheme(GetParent(hwndDlg), !!PhGetIntegerSetting(L"EnableThemeSupport"));
            else
                PhInitializeWindowTheme(hwndDlg, FALSE);
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(SETTING_NAME_DEVICE_GENERAL_COLUMNS, context->GeneralListViewHandle);

            if (context->DeviceIcon)
                DestroyIcon(context->DeviceIcon);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItemEx(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL, TRUE);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_DEVICE_GROUPBOX), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_DEVICE_NAME), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PvAddPropPageLayoutItem(hwndDlg, GetDlgItem(hwndDlg, IDC_DEVICE_MANUFACTURER), dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PvAddPropPageLayoutItem(hwndDlg, context->GeneralListViewHandle, dialogItem, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyBehaviors(lParam, context->GeneralListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->GeneralListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID *listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->GeneralListViewHandle, &point);

                PhGetSelectedListViewItemParams(context->GeneralListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, PHAPP_IDC_COPY, context->GeneralListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        if (!PhHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case PHAPP_IDC_COPY:
                                {
                                    PhCopyListView(context->GeneralListViewHandle);
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
    case WM_PH_UPDATE_DIALOG:
        {
            PPH_EMENU menu;
            PPH_EMENU_ITEM enable;
            PPH_EMENU_ITEM disable;
            PPH_EMENU_ITEM restart;
            PPH_EMENU_ITEM uninstall;
            RECT rect;
            PPH_EMENU_ITEM selectedItem;

            menu = PhCreateEMenu();
            PhInsertEMenuItem(menu, enable = PhCreateEMenuItem(0, 0, L"Enable", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, disable = PhCreateEMenuItem(0, 1, L"Disable", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, restart = PhCreateEMenuItem(0, 2, L"Restart", NULL, NULL), ULONG_MAX);
            PhInsertEMenuItem(menu, uninstall = PhCreateEMenuItem(0, 3, L"Uninstall", NULL, NULL), ULONG_MAX);

            if (!PhGetOwnTokenAttributes().Elevated)
            {
                enable->Flags |= PH_EMENU_DISABLED;
                disable->Flags |= PH_EMENU_DISABLED;
                restart->Flags |= PH_EMENU_DISABLED;
                uninstall->Flags |= PH_EMENU_DISABLED;
            }

            GetWindowRect(hwndDlg, &rect);
            selectedItem = PhShowEMenu(
                menu,
                hwndDlg,
                PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_BOTTOM,
                rect.left - 4,
                rect.bottom + 10
                );

            if (selectedItem && selectedItem->Id != ULONG_MAX)
            {
                switch (selectedItem->Id)
                {
                case 0:
                case 1:
                    HardwareDeviceEnableDisable(hwndDlg, context->DeviceItem->InstanceId, selectedItem->Id == 0);
                    break;
                case 2:
                    HardwareDeviceRestart(hwndDlg, context->DeviceItem->InstanceId);
                    break;
                case 3:
                    HardwareDeviceUninstall(hwndDlg, context->DeviceItem->InstanceId);
                    break;
                }
            }

            PhDestroyEMenu(menu);
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORLISTBOX:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            SetTextColor((HDC)wParam, RGB(0, 0, 0));
            SetDCBrushColor((HDC)wParam, RGB(255, 255, 255));
            return (INT_PTR)GetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}

VOID DeviceInitializePropsPage(
    _In_ HWND hwndDlg,
    _In_ PDEVICE_PROPERTIES_CONTEXT Context
    )
{
    ExtendedListView_SetRedraw(Context->PropsListViewHandle, FALSE);
    ListView_DeleteAllItems(Context->PropsListViewHandle);

    for (ULONG i = 0; i < DeviceItemPropertyTableCount; i++)
    {
        const DEVICE_PROPERTY_TABLE_ENTRY* entry = &DeviceItemPropertyTable[i];
        INT lvItemIndex;
        PWSTR value;

        value = PhGetStringOrEmpty(PhGetDeviceProperty(Context->DeviceItem, entry->PropClass)->AsString);

        lvItemIndex = PhAddListViewItem(Context->PropsListViewHandle, MAXINT, entry->ColumnName, NULL);
        PhSetListViewSubItem(Context->PropsListViewHandle, lvItemIndex, 1, value);
    }

    ExtendedListView_SetRedraw(Context->PropsListViewHandle, TRUE);
}

INT_PTR CALLBACK DevicePropPropertiesDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDEVICE_PROPERTIES_CONTEXT context;
    LPPROPSHEETPAGE propSheetPage;
    PPV_PROPPAGECONTEXT propPageContext;

    if (!PvPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext))
        return FALSE;

    context = (PDEVICE_PROPERTIES_CONTEXT)propPageContext->Context;

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->PropsListViewHandle = GetDlgItem(hwndDlg, IDC_DEVICE_PROPERTIES_LIST);

            PhSetListViewStyle(context->PropsListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->PropsListViewHandle, L"explorer");
            PhAddListViewColumn(context->PropsListViewHandle, 0, 0, 0, LVCFMT_LEFT, 160, L"Name");
            PhAddListViewColumn(context->PropsListViewHandle, 1, 1, 1, LVCFMT_LEFT, 300, L"Value");
            PhSetExtendedListView(context->PropsListViewHandle);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_DEVICE_PROPERTIES_COLUMNS, context->PropsListViewHandle);

            DeviceInitializePropsPage(hwndDlg, context);

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhSaveListViewColumnsToSetting(SETTING_NAME_DEVICE_PROPERTIES_COLUMNS, context->PropsListViewHandle);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (!propPageContext->LayoutInitialized)
            {
                PPH_LAYOUT_ITEM dialogItem;

                dialogItem = PvAddPropPageLayoutItem(hwndDlg, hwndDlg, PH_PROP_PAGE_TAB_CONTROL_PARENT, PH_ANCHOR_ALL);
                PvAddPropPageLayoutItem(hwndDlg, context->PropsListViewHandle, dialogItem, PH_ANCHOR_ALL);
                PvDoPropPageLayout(hwndDlg);

                propPageContext->LayoutInitialized = TRUE;
            }
        }
        break;
    case WM_NOTIFY:
        {
            PhHandleListViewNotifyBehaviors(lParam, context->PropsListViewHandle, PH_LIST_VIEW_DEFAULT_1_BEHAVIORS);
        }
        break;
    case WM_CONTEXTMENU:
        {
            if ((HWND)wParam == context->PropsListViewHandle)
            {
                POINT point;
                PPH_EMENU menu;
                PPH_EMENU item;
                PVOID *listviewItems;
                ULONG numberOfItems;

                point.x = GET_X_LPARAM(lParam);
                point.y = GET_Y_LPARAM(lParam);

                if (point.x == -1 && point.y == -1)
                    PhGetListViewContextMenuPoint(context->PropsListViewHandle, &point);

                PhGetSelectedListViewItemParams(context->PropsListViewHandle, &listviewItems, &numberOfItems);

                if (numberOfItems != 0)
                {
                    menu = PhCreateEMenu();
                    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, PHAPP_IDC_COPY, L"&Copy", NULL, NULL), ULONG_MAX);
                    PhInsertCopyListViewEMenuItem(menu, PHAPP_IDC_COPY, context->PropsListViewHandle);

                    item = PhShowEMenu(
                        menu,
                        hwndDlg,
                        PH_EMENU_SHOW_SEND_COMMAND | PH_EMENU_SHOW_LEFTRIGHT,
                        PH_ALIGN_LEFT | PH_ALIGN_TOP,
                        point.x,
                        point.y
                        );

                    if (item)
                    {
                        if (!PhHandleCopyListViewEMenuItem(item))
                        {
                            switch (item->Id)
                            {
                            case PHAPP_IDC_COPY:
                                {
                                    PhCopyListView(context->PropsListViewHandle);
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

VOID DeviceAddControlButtons(
    _In_ PPV_PROPSHEETCONTEXT PropSheetContext
    )
{

}

NTSTATUS DevicePropertiesThreadStart(
    _In_ PVOID Parameter
    )
{
    PDEVICE_PROPERTIES_CONTEXT context = Parameter;
    PPV_PROPCONTEXT propContext;

    if (propContext = HdCreatePropContext(PhGetStringOrDefault(
        PhGetDeviceProperty(context->DeviceItem, PhDevicePropertyName)->AsString,
        L"Device"
        )))
    {
        PPV_PROPPAGECONTEXT newPage;

        propContext->EnableControlButtons = TRUE;

        // General
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_DEVICE_GENERAL),
            DevicePropGeneralDlgProc,
            context);
        PvAddPropPage(propContext, newPage);

        // Properties
        newPage = PvCreatePropPageContext(
            MAKEINTRESOURCE(IDD_DEVICE_PROPERTIES),
            DevicePropPropertiesDlgProc,
            context);
        PvAddPropPage(propContext, newPage);

        PhModalPropertySheet(&propContext->PropSheetHeader);
        PhDereferenceObject(propContext);
    }

    PhFree(context);

    return STATUS_SUCCESS;
}

BOOLEAN DeviceShowProperties(
    _In_ HWND ParentWindowHandle,
    _In_ PPH_DEVICE_ITEM DeviceItem
    )
{
    PDEVICE_PROPERTIES_CONTEXT context;

    context = PhAllocateZero(sizeof(DEVICE_PROPERTIES_CONTEXT));

    context->ParentWindowHandle = ParentWindowHandle;
    context->DeviceItem = PhReferenceObject(DeviceItem);

    PhCreateThread2(DevicePropertiesThreadStart, context);
    return TRUE;
}
