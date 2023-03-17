/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2022
 *
 */

#include "extsrv.h"

#include <cfgmgr32.h>
#include <hndlinfo.h>
#include <devquery.h>

typedef struct _PNP_SERVICE_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    PPH_SERVICE_ITEM ServiceItem;
    HIMAGELIST ImageList;
} PNP_SERVICE_CONTEXT, *PPNP_SERVICE_CONTEXT;

BOOLEAN HardwareDeviceEnableDisable(
    _In_ HWND ParentWindow,
    _In_ PPH_STRING DeviceInstance,
    _In_ BOOLEAN Enable
    )
{
    CONFIGRET result;
    DEVINST deviceInstanceHandle;

    result = CM_Locate_DevNode(
        &deviceInstanceHandle,
        DeviceInstance->Buffer,
        CM_LOCATE_DEVNODE_PHANTOM
        );

    if (result != CR_SUCCESS)
    {
        PhShowStatus(ParentWindow, L"Failed to change the device state.", 0, CM_MapCrToWin32Err(result, ERROR_INVALID_HANDLE_STATE));
        return FALSE;
    }

    if (Enable)
        result = CM_Enable_DevInst(deviceInstanceHandle, 0); // CM_DISABLE_PERSIST
    else
        result = CM_Disable_DevInst(deviceInstanceHandle, 0); // CM_DISABLE_PERSIST

    if (result != CR_SUCCESS)
    {
        PhShowStatus(ParentWindow, L"Failed to change the device state.", 0, CM_MapCrToWin32Err(result, ERROR_INVALID_HANDLE_STATE));
        return FALSE;
    }

    return TRUE;
}

BOOLEAN HardwareDeviceRestart(
    _In_ HWND ParentWindow,
    _In_ PPH_STRING DeviceInstance
    )
{
    CONFIGRET result;
    DEVINST deviceInstanceHandle;

    result = CM_Locate_DevNode(
        &deviceInstanceHandle,
        DeviceInstance->Buffer,
        CM_LOCATE_DEVNODE_PHANTOM
        );

    if (result != CR_SUCCESS)
    {
        PhShowStatus(ParentWindow, L"Failed to restart the device.", 0, CM_MapCrToWin32Err(result, ERROR_UNKNOWN_PROPERTY));
        return FALSE;
    }

    result = CM_Query_And_Remove_SubTree(
        deviceInstanceHandle,
        NULL,
        NULL,
        0,
        CM_REMOVE_NO_RESTART
        );

    if (result != CR_SUCCESS)
    {
        PhShowStatus(ParentWindow, L"Failed to restart the device.", 0, CM_MapCrToWin32Err(result, ERROR_UNKNOWN_PROPERTY));
        return FALSE;
    }

    result = CM_Setup_DevInst(
        deviceInstanceHandle,
        CM_SETUP_DEVNODE_READY
        );

    if (result != CR_SUCCESS)
    {
        PhShowStatus(ParentWindow, L"Failed to restart the device.", 0, CM_MapCrToWin32Err(result, ERROR_UNKNOWN_PROPERTY));
        return FALSE;
    }

    return TRUE;
}

BOOLEAN HardwareDeviceUninstall(
    _In_ HWND ParentWindow,
    _In_ PPH_STRING DeviceInstance
    )
{
    CONFIGRET result;
    DEVINST deviceInstanceHandle;

    result = CM_Locate_DevNode(
        &deviceInstanceHandle,
        DeviceInstance->Buffer,
        CM_LOCATE_DEVNODE_PHANTOM
        );

    if (result != CR_SUCCESS)
    {
        PhShowStatus(ParentWindow, L"Failed to uninstall the device.", 0, CM_MapCrToWin32Err(result, ERROR_UNKNOWN_PROPERTY));
        return FALSE;
    }

    result = CM_Uninstall_DevInst(deviceInstanceHandle, 0);

    if (result != CR_SUCCESS)
    {
        PhShowStatus(ParentWindow, L"Failed to uninstall the device.", 0, CM_MapCrToWin32Err(result, ERROR_UNKNOWN_PROPERTY));
        return FALSE;
    }

    return TRUE;
}

BOOLEAN HardwareDeviceShowProperties(
    _In_ HWND WindowHandle,
    _In_ PPH_STRING DeviceInstance
    )
{
    HMODULE devMgrHandle;

    // https://msdn.microsoft.com/en-us/library/ff548181.aspx
    VOID (WINAPI* DeviceProperties_RunDLL_I)(
        _In_ HWND hwndStub,
        _In_ HINSTANCE hAppInstance,
        _In_ PWSTR lpCmdLine,
        _In_ INT nCmdShow
        );

    //ULONG (WINAPI *DeviceAdvancedPropertiesW_I)(
    //    _In_opt_ HWND hWndParent,
    //    _In_opt_ PWSTR MachineName,
    //    _In_ PWSTR DeviceID);

    if (devMgrHandle = PhLoadLibrary(L"devmgr.dll"))
    {
        if (DeviceProperties_RunDLL_I = PhGetProcedureAddress(devMgrHandle, "DeviceProperties_RunDLLW", 0))
        {
            PH_FORMAT format[2];
            WCHAR formatBuffer[512];

            // /DeviceID %s
            PhInitFormatS(&format[0], L"/DeviceID ");
            PhInitFormatSR(&format[1], DeviceInstance->sr);

            if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
            {
                // This will sometimes re-throw an RPC error while debugging and can safely be ignored. (dmex)
                DeviceProperties_RunDLL_I(
                    GetParent(WindowHandle),
                    NULL,
                    formatBuffer,
                    0
                    );
            }
            else
            {
                // This will sometimes re-throw an RPC error while debugging and can safely be ignored. (dmex)
                DeviceProperties_RunDLL_I(
                    GetParent(WindowHandle),
                    NULL,
                    PhaFormatString(L"/DeviceID %s", DeviceInstance->Buffer)->Buffer,
                    0
                    );
            }
        }

        PhFreeLibrary(devMgrHandle);
    }

    return FALSE;
}

BOOLEAN HardwareDeviceOpenKey(
    _In_ HWND ParentWindow,
    _In_ PPH_STRING DeviceInstance,
    _In_ ULONG KeyIndex
    )
{
    CONFIGRET result;
    DEVINST deviceInstanceHandle;
    ULONG keyIndex;
    HKEY keyHandle;

    result = CM_Locate_DevNode(
        &deviceInstanceHandle,
        DeviceInstance->Buffer,
        CM_LOCATE_DEVNODE_PHANTOM
        );

    if (result != CR_SUCCESS)
    {
        PhShowStatus(ParentWindow, L"Failed to locate the device.", 0, CM_MapCrToWin32Err(result, ERROR_UNKNOWN_PROPERTY));
        return FALSE;
    }

    switch (KeyIndex)
    {
    case 4:
    default:
        keyIndex = CM_REGISTRY_HARDWARE;
        break;
    case 5:
        keyIndex = CM_REGISTRY_SOFTWARE;
        break;
    case 6:
        keyIndex = CM_REGISTRY_USER;
        break;
    case 7:
        keyIndex = CM_REGISTRY_CONFIG;
        break;
    }

    if (CM_Open_DevInst_Key(
        deviceInstanceHandle,
        KEY_READ,
        0,
        RegDisposition_OpenExisting,
        &keyHandle,
        keyIndex
        ) == CR_SUCCESS)
    {
        PPH_STRING bestObjectName = NULL;

        PhGetHandleInformation(
            NtCurrentProcess(),
            keyHandle,
            ULONG_MAX,
            NULL,
            NULL,
            NULL,
            &bestObjectName
            );

        if (bestObjectName)
        {
            // HKLM\SYSTEM\ControlSet\Control\Class\ += DEVPKEY_Device_Driver
            PhShellOpenKey2(ParentWindow, bestObjectName);
            PhDereferenceObject(bestObjectName);
        }

        NtClose(keyHandle);
    }

    return TRUE;
}

VOID EspShowDeviceInstanceMenu(
    _In_ HWND ParentWindow,
    _In_ PPH_STRING DeviceInstance
    )
{
    POINT cursorPos;
    PPH_EMENU menu;
    PPH_EMENU subMenu;
    PPH_EMENU_ITEM selectedItem;

    GetCursorPos(&cursorPos);

    menu = PhCreateEMenu();
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 0, L"Enable", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"Disable", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 2, L"Restart", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 3, L"Uninstall", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    subMenu = PhCreateEMenuItem(0, 0, L"Open key", NULL, NULL);
    PhInsertEMenuItem(subMenu, PhCreateEMenuItem(0, 4, L"Hardware", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(subMenu, PhCreateEMenuItem(0, 5, L"Software", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(subMenu, PhCreateEMenuItem(0, 6, L"User", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(subMenu, PhCreateEMenuItem(0, 7, L"Config", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, subMenu, ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 10, L"Properties", NULL, NULL), ULONG_MAX);

    selectedItem = PhShowEMenu(
        menu,
        ParentWindow,
        PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_TOP,
        cursorPos.x,
        cursorPos.y
        );

    if (selectedItem && selectedItem->Id != ULONG_MAX)
    {
        switch (selectedItem->Id)
        {
        case 0:
        case 1:
            HardwareDeviceEnableDisable(ParentWindow, DeviceInstance, selectedItem->Id == 0);
            break;
        case 2:
            HardwareDeviceRestart(ParentWindow, DeviceInstance);
            break;
        case 3:
            {
                if (HardwareDeviceUninstall(ParentWindow, DeviceInstance))
                {
                    NOTHING;
                }
            }
            break;
        case 4:
        case 5:
        case 6:
        case 7:
            HardwareDeviceOpenKey(ParentWindow, DeviceInstance, selectedItem->Id);
            break;
        case 10:
            HardwareDeviceShowProperties(ParentWindow, DeviceInstance);
            break;
        }
    }

    PhDestroyEMenu(menu);
}

VOID EspLoadDeviceInstanceImage(
    _In_ PPNP_SERVICE_CONTEXT Context,
    _In_ GUID DeviceClass,
    _In_ INT ItemIndex
    )
{
    HICON smallIcon;
    CONFIGRET result;
    ULONG deviceIconPathLength;
    DEVPROPTYPE deviceIconPathPropertyType;
    PPH_STRING deviceIconPath;
    LONG dpiValue;

    deviceIconPathLength = 0x40;
    deviceIconPath = PhCreateStringEx(NULL, deviceIconPathLength);

    if ((result = CM_Get_Class_Property(
        &DeviceClass,
        &DEVPKEY_DeviceClass_IconPath,
        &deviceIconPathPropertyType,
        (PBYTE)deviceIconPath->Buffer,
        &deviceIconPathLength,
        0
        )) != CR_SUCCESS)
    {
        PhDereferenceObject(deviceIconPath);
        deviceIconPath = PhCreateStringEx(NULL, deviceIconPathLength);

        result = CM_Get_Class_Property(
            &DeviceClass,
            &DEVPKEY_DeviceClass_IconPath,
            &deviceIconPathPropertyType,
            (PBYTE)deviceIconPath->Buffer,
            &deviceIconPathLength,
            0
            );
    }

    if (result != CR_SUCCESS)
    {
        PhDereferenceObject(deviceIconPath);
        return;
    }

    PhTrimToNullTerminatorString(deviceIconPath);

    {
        PPH_STRING dllIconPath;
        PH_STRINGREF dllPartSr;
        PH_STRINGREF indexPartSr;
        LONG64 index = 0;

        if (
            PhSplitStringRefAtChar(&deviceIconPath->sr, L',', &dllPartSr, &indexPartSr) &&
            PhStringToInteger64(&indexPartSr, 10, &index)
            )
        {
            if (dllIconPath = PhExpandEnvironmentStrings(&dllPartSr))
            {
                dpiValue = PhGetWindowDpi(Context->WindowHandle);

                if (PhExtractIconEx(dllIconPath, FALSE, (INT)index, &smallIcon, NULL, dpiValue))
                {
                    INT imageIndex = PhImageListAddIcon(Context->ImageList, smallIcon);
                    PhSetListViewItemImageIndex(Context->ListViewHandle, ItemIndex, imageIndex);
                    DestroyIcon(smallIcon);
                }

                PhDereferenceObject(dllIconPath);
            }
        }
    }

    PhDereferenceObject(deviceIconPath);
}

_Success_(return)
BOOLEAN EspQueryDeviceInstanceInformation(
    _In_ PWSTR DeviceInstanceId,
    _Out_ DEVINST* DeviceInstanceHandle,
    _Out_ PPH_STRING* DeviceDescription
    )
{
    CONFIGRET result;
    ULONG bufferSize;
    PPH_STRING deviceDescription;
    DEVPROPTYPE devicePropertyType;
    DEVINST deviceInstanceHandle;

    if (CM_Locate_DevNode(
        &deviceInstanceHandle,
        DeviceInstanceId,
        CM_LOCATE_DEVNODE_PHANTOM
        ) != CR_SUCCESS)
    {
        return FALSE;
    }

    bufferSize = 0x40;
    deviceDescription = PhCreateStringEx(NULL, bufferSize);

    if ((result = CM_Get_DevNode_Property(
        deviceInstanceHandle,
        &DEVPKEY_NAME,
        &devicePropertyType,
        (PBYTE)deviceDescription->Buffer,
        &bufferSize,
        0
        )) != CR_SUCCESS)
    {
        PhDereferenceObject(deviceDescription);
        deviceDescription = PhCreateStringEx(NULL, bufferSize);

        result = CM_Get_DevNode_Property(
            deviceInstanceHandle,
            &DEVPKEY_NAME,
            &devicePropertyType,
            (PBYTE)deviceDescription->Buffer,
            &bufferSize,
            0
            );
    }

    if (result != CR_SUCCESS)
    {
        PhDereferenceObject(deviceDescription);
        return FALSE;
    }

    PhTrimToNullTerminatorString(deviceDescription);

    *DeviceInstanceHandle = deviceInstanceHandle;
    *DeviceDescription = deviceDescription;

    return TRUE;
}

BOOLEAN EspEnumerateDriverPnpDevicesAlt(
    _In_ PPNP_SERVICE_CONTEXT Context
    )
{
    CONFIGRET status;
    PWSTR deviceIdentifierList;
    ULONG deviceIdentifierListLength = 0;
    PWSTR deviceIdentifier;

    status = CM_Get_Device_ID_List_Size(
        &deviceIdentifierListLength,
        PhGetString(Context->ServiceItem->Name),
        CM_GETIDLIST_FILTER_SERVICE | CM_GETIDLIST_DONOTGENERATE
        );

    if (status != CR_SUCCESS)
        return FALSE;
    if (deviceIdentifierListLength <= sizeof(UNICODE_NULL))
        return FALSE;

    deviceIdentifierList = PhAllocate(deviceIdentifierListLength * sizeof(WCHAR));
    memset(deviceIdentifierList, 0, deviceIdentifierListLength * sizeof(WCHAR));

    status = CM_Get_Device_ID_List(
        PhGetString(Context->ServiceItem->Name),
        deviceIdentifierList,
        deviceIdentifierListLength,
        CM_GETIDLIST_FILTER_SERVICE | CM_GETIDLIST_DONOTGENERATE
        );

    if (status != CR_SUCCESS)
    {
        PhFree(deviceIdentifierList);
        return FALSE;
    }

    for (deviceIdentifier = deviceIdentifierList; *deviceIdentifier; deviceIdentifier += PhCountStringZ(deviceIdentifier) + 1)
    {
        DEVINST deviceInstanceHandle;
        PPH_STRING deviceDescription;
        DEVPROPTYPE devicePropertyType;
        DEVPROP_BOOLEAN devicePresent = DEVPROP_FALSE;
        GUID classGuid = { 0 };
        ULONG bufferSize;

        if (!EspQueryDeviceInstanceInformation(deviceIdentifier, &deviceInstanceHandle, &deviceDescription))
            continue;

        bufferSize = sizeof(DEVPROP_BOOLEAN);
        CM_Get_DevNode_Property(
            deviceInstanceHandle,
            &DEVPKEY_Device_IsPresent,
            &devicePropertyType,
            (PBYTE)&devicePresent,
            &bufferSize,
            0
            );

        bufferSize = sizeof(GUID);
        if (CM_Get_DevNode_Property(
            deviceInstanceHandle,
            &DEVPKEY_Device_ClassGuid,
            &devicePropertyType,
            (PBYTE)&classGuid,
            &bufferSize,
            0
            ) == CR_SUCCESS)
        {
            INT lvItemIndex = PhAddListViewGroupItem(
                Context->ListViewHandle,
                devicePresent ? 0 : 1,
                MAXINT,
                PhGetString(deviceDescription),
                PhCreateString(deviceIdentifier)
                );

            EspLoadDeviceInstanceImage(Context, classGuid, lvItemIndex);
        }
    }

    PhFree(deviceIdentifierList);
    return TRUE;
}

BOOLEAN EspEnumerateDriverPnpDevices(
    _In_ PPNP_SERVICE_CONTEXT Context
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HRESULT (WINAPI* DevGetObjects_I)(
        _In_ DEV_OBJECT_TYPE ObjectType,
        _In_ ULONG QueryFlags,
        _In_ ULONG cRequestedProperties,
        _In_reads_opt_(cRequestedProperties) const DEVPROPCOMPKEY *pRequestedProperties,
        _In_ ULONG cFilterExpressionCount,
        _In_reads_opt_(cFilterExpressionCount) const DEVPROP_FILTER_EXPRESSION *pFilter,
        _Out_ PULONG pcObjectCount,
        _Outptr_result_buffer_maybenull_(*pcObjectCount) const DEV_OBJECT **ppObjects) = NULL;
    static HRESULT (WINAPI* DevFreeObjects_I)(
        _In_ ULONG cObjectCount,
        _In_reads_(cObjectCount) const DEV_OBJECT *pObjects) = NULL;
    PPH_STRING serviceName = Context->ServiceItem->Name;
    ULONG deviceCount = 0;
    PDEV_OBJECT deviceObjects = NULL;
    DEVPROPCOMPKEY deviceProperties[5];
    DEVPROP_FILTER_EXPRESSION deviceFilter[1];
    DEVPROPERTY deviceFilterProperty;
    DEVPROPCOMPKEY deviceFilterCompoundProp;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID cfgmgr32;

        if (cfgmgr32 = PhLoadLibrary(L"cfgmgr32.dll"))
        {
            DevGetObjects_I = PhGetProcedureAddress(cfgmgr32, "DevGetObjects", 0);
            DevFreeObjects_I = PhGetProcedureAddress(cfgmgr32, "DevFreeObjects", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!(DevGetObjects_I && DevFreeObjects_I))
        return FALSE;

    memset(deviceProperties, 0, sizeof(deviceProperties));
    deviceProperties[0].Key = DEVPKEY_Device_InstanceId;
    deviceProperties[0].Store = DEVPROP_STORE_SYSTEM;
    deviceProperties[1].Key = DEVPKEY_NAME; // DEVPKEY_Device_FriendlyName
    deviceProperties[1].Store = DEVPROP_STORE_SYSTEM;
    deviceProperties[2].Key = DEVPKEY_Device_PDOName;
    deviceProperties[2].Store = DEVPROP_STORE_SYSTEM;
    deviceProperties[3].Key = DEVPKEY_Device_ClassGuid;
    deviceProperties[3].Store = DEVPROP_STORE_SYSTEM;
    deviceProperties[4].Key = DEVPKEY_Device_IsPresent;
    deviceProperties[4].Store = DEVPROP_STORE_SYSTEM;

    memset(&deviceFilterCompoundProp, 0, sizeof(deviceFilterCompoundProp));
    deviceFilterCompoundProp.Key = DEVPKEY_Device_Service;
    deviceFilterCompoundProp.Store = DEVPROP_STORE_SYSTEM;

    memset(&deviceFilterProperty, 0, sizeof(deviceFilterProperty));
    deviceFilterProperty.CompKey = deviceFilterCompoundProp;
    deviceFilterProperty.Type = DEVPROP_TYPE_STRING;
    deviceFilterProperty.BufferSize = (ULONG)serviceName->Length + sizeof(UNICODE_NULL);
    deviceFilterProperty.Buffer = serviceName->Buffer;

    memset(deviceFilter, 0, sizeof(deviceFilter));
    deviceFilter[0].Operator = DEVPROP_OPERATOR_EQUALS_IGNORE_CASE;
    deviceFilter[0].Property = deviceFilterProperty;

    if (SUCCEEDED(DevGetObjects_I(
        DevObjectTypeDevice,
        DevQueryFlagNone,
        RTL_NUMBER_OF(deviceProperties),
        deviceProperties,
        RTL_NUMBER_OF(deviceFilter),
        deviceFilter,
        &deviceCount,
        &deviceObjects
        )))
    {
        for (ULONG i = 0; i < deviceCount; i++)
        {
            DEV_OBJECT device = deviceObjects[i];
            INT lvItemIndex;
            PPH_STRING deviceName;
            PH_STRINGREF deviceInterface;
            PH_STRINGREF displayName;
            PH_STRINGREF deviceObjectName;
            PH_STRINGREF firstPart;
            PH_STRINGREF secondPart;
            DEVPROP_BOOLEAN present;
            GUID classGuid = { 0 };

            assert(device.cPropertyCount == RTL_NUMBER_OF(deviceProperties));
            deviceInterface.Length = device.pProperties[0].BufferSize;
            deviceInterface.Buffer = device.pProperties[0].Buffer;
            displayName.Length = device.pProperties[1].BufferSize;
            displayName.Buffer = device.pProperties[1].Buffer;
            deviceObjectName.Length = device.pProperties[2].BufferSize;
            deviceObjectName.Buffer = device.pProperties[2].Buffer;
            memcpy_s(&classGuid, sizeof(GUID), device.pProperties[3].Buffer, device.pProperties[3].BufferSize);
            present = *(PDEVPROP_BOOLEAN)device.pProperties[4].Buffer == DEVPROP_TRUE;

            // TODO: USBXHCI service: %1 USB %2 eXtensible Host Controller - %3 (Microsoft);(ASMedia,3.0,1.0)
            if (PhSplitStringRefAtLastChar(&displayName, L';', &firstPart, &secondPart))
                deviceName = PhCreateString2(&secondPart);
            else
                deviceName = PhCreateString2(&displayName);

            if (deviceName->Length >= sizeof(UNICODE_NULL) && deviceName->Buffer[deviceName->Length / sizeof(WCHAR)] == UNICODE_NULL)
                deviceName->Length -= sizeof(UNICODE_NULL); // PhTrimToNullTerminatorString(deviceName);
            if (deviceObjectName.Length >= sizeof(UNICODE_NULL) && deviceObjectName.Buffer[deviceObjectName.Length / sizeof(WCHAR)] == UNICODE_NULL)
                deviceObjectName.Length -= sizeof(UNICODE_NULL); // PhTrimToNullTerminatorString(deviceObjectName);

            if (deviceName && deviceObjectName.Length)
            {
                PH_FORMAT format[4];

                PhInitFormatSR(&format[0], deviceName->sr);
                PhInitFormatS(&format[1], L" (PDO: ");
                PhInitFormatSR(&format[2], deviceObjectName);
                PhInitFormatC(&format[3], ')');

                PhMoveReference(&deviceName, PhFormat(format, RTL_NUMBER_OF(format), 0));
            }

            lvItemIndex = PhAddListViewGroupItem(
                Context->ListViewHandle,
                present ? 0 : 1,
                MAXINT,
                PhGetString(deviceName),
                PhCreateString2(&deviceInterface)
                );
            EspLoadDeviceInstanceImage(Context, classGuid, lvItemIndex);

            if (deviceName) PhDereferenceObject(deviceName);
        }

        DevFreeObjects_I(deviceCount, deviceObjects);
    }

    return TRUE;
}

VOID EspFreeListViewDiskDriveEntries(
    _In_ PPNP_SERVICE_CONTEXT Context
    )
{
    INT index = INT_ERROR;

    while ((index = PhFindListViewItemByFlags(Context->ListViewHandle, index, LVNI_ALL)) != INT_ERROR)
    {
        PPH_STRING param;

        if (PhGetListViewItemParam(Context->ListViewHandle, index, &param))
        {
            PhDereferenceObject(param);
        }
    }
}

INT_PTR CALLBACK EspPnPServiceDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPNP_SERVICE_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE propSheetPage = (LPPROPSHEETPAGE)lParam;
        PPH_SERVICE_ITEM serviceItem = (PPH_SERVICE_ITEM)propSheetPage->lParam;

        context = PhAllocateZero(sizeof(PNP_SERVICE_CONTEXT));
        context->ServiceItem = serviceItem;

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
            LONG dpiValue;

            context->WindowHandle = hwndDlg;
            context->ListViewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhSetListViewStyle(context->ListViewHandle, FALSE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 350, L"PnP Devices");
            PhSetExtendedListView(context->ListViewHandle);
            ExtendedListView_SetColumnWidth(context->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
            if (PhWindowsVersion > WINDOWS_7)
                ListView_EnableGroupView(context->ListViewHandle, TRUE);
            PhAddListViewGroup(context->ListViewHandle, 0, L"Connected");
            PhAddListViewGroup(context->ListViewHandle, 1, L"Disconnected");

            dpiValue = PhGetWindowDpi(hwndDlg);
            context->ImageList = PhImageListCreate(
                PhGetDpi(24, dpiValue), // PhGetSystemMetrics(SM_CXSMICON, dpiValue)
                PhGetDpi(24, dpiValue), // PhGetSystemMetrics(SM_CYSMICON, dpiValue)
                ILC_MASK | ILC_COLOR32,
                1, 1
                );
            ListView_SetImageList(context->ListViewHandle, context->ImageList, LVSIL_SMALL);

            if (context->ServiceItem->Type & SERVICE_DRIVER)
            {
                PhSetDialogItemText(hwndDlg, IDC_MESSAGE, L"This service has registered the following PnP devices:");

                if (!EspEnumerateDriverPnpDevices(context))
                {
                    EspEnumerateDriverPnpDevicesAlt(context);
                }
            }
            else
            {
                PhSetDialogItemText(hwndDlg, IDC_MESSAGE, L"This service type doesn't support PnP devices.");
                ShowWindow(context->ListViewHandle, SW_HIDE);
            }

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

            EspFreeListViewDiskDriveEntries(context);
            PhFree(context);
        }
        break;
    case WM_SIZE:
        {
            ExtendedListView_SetColumnWidth(context->ListViewHandle, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)context->ListViewHandle);
                return TRUE;
            case NM_RCLICK:
                {
                    PPH_STRING deviceInstance;

                    if (deviceInstance = PhGetSelectedListViewItemParam(context->ListViewHandle))
                    {
                        EspShowDeviceInstanceMenu(hwndDlg, deviceInstance);

                        EspFreeListViewDiskDriveEntries(context);
                        ListView_DeleteAllItems(context->ListViewHandle);

                        if (!EspEnumerateDriverPnpDevices(context))
                        {
                            EspEnumerateDriverPnpDevicesAlt(context);
                        }
                    }
                }
                break;
            case NM_DBLCLK:
                {
                    PPH_STRING deviceInstance;

                    if (deviceInstance = PhGetSelectedListViewItemParam(context->ListViewHandle))
                    {
                        HardwareDeviceShowProperties(hwndDlg, deviceInstance);
                    }
                }
                break;
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}
