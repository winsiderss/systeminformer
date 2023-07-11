/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2015-2023
 *
 */

#include "devices.h"
#include <hndlinfo.h>
#include <secedit.h>

PPH_PLUGIN PluginInstance = NULL;
BOOLEAN NetAdapterEnableNdis = FALSE;
ULONG NetWindowsVersion = WINDOWS_ANCIENT;

PPH_OBJECT_TYPE NetAdapterEntryType = NULL;
PPH_LIST NetworkAdaptersList = NULL;
PH_QUEUED_LOCK NetworkAdaptersListLock = PH_QUEUED_LOCK_INIT;

PPH_OBJECT_TYPE DiskDriveEntryType = NULL;
PPH_LIST DiskDrivesList = NULL;
PH_QUEUED_LOCK DiskDrivesListLock = PH_QUEUED_LOCK_INIT;

PPH_OBJECT_TYPE RaplDeviceEntryType = NULL;
PPH_LIST RaplDevicesList = NULL;
PH_QUEUED_LOCK RaplDevicesListLock = PH_QUEUED_LOCK_INIT;

PPH_OBJECT_TYPE GraphicsDeviceEntryType = NULL;
PPH_LIST GraphicsDevicesList = NULL;
PH_QUEUED_LOCK GraphicsDevicesListLock = PH_QUEUED_LOCK_INIT;

PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;
PH_CALLBACK_REGISTRATION SystemInformationInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION SettingsUpdatedCallbackRegistration;

VOID NTAPI LoadSettings(
    VOID
    )
{
    NetAdapterEnableNdis = !!PhGetIntegerSetting(SETTING_NAME_ENABLE_NDIS);
    NetWindowsVersion = PhWindowsVersion;
}

VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    LoadSettings();

    GraphicsDeviceInitialize();
    DiskDrivesInitialize();
    NetAdaptersInitialize();
    RaplDeviceInitialize();

    GraphicsDevicesLoadList();
    DiskDrivesLoadList();
    NetAdaptersLoadList();
    RaplDevicesLoadList();
}

VOID NTAPI UnloadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

VOID NTAPI ShowOptionsCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_OPTIONS_POINTERS optionsEntry = (PPH_PLUGIN_OPTIONS_POINTERS)Parameter;

    optionsEntry->CreateSection(
        L"Disk Devices",
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_DISKDRIVE_OPTIONS),
        DiskDriveOptionsDlgProc,
        NULL
        );

    optionsEntry->CreateSection(
        L"Graphics Devices",
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_GPUDEVICE_OPTIONS),
        GraphicsDeviceOptionsDlgProc,
        NULL
        );

    optionsEntry->CreateSection(
        L"Network Devices",
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_NETADAPTER_OPTIONS),
        NetworkAdapterOptionsDlgProc,
        NULL
        );

    optionsEntry->CreateSection(
        L"RAPL Devices",
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_RAPLDEVICE_OPTIONS),
        RaplDeviceOptionsDlgProc,
        NULL
        );
}

VOID NTAPI MainWindowShowingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    AddRemoveDeviceChangeCallback();
    if (NetWindowsVersion >= WINDOWS_10)
        InitializeDevicesTab();
}

VOID NTAPI ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    GraphicsDevicesUpdate();
    DiskDrivesUpdate();
    NetAdaptersUpdate();
    RaplDevicesUpdate();
}

VOID NTAPI SystemInformationInitializingCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_SYSINFO_POINTERS pluginEntry = (PPH_PLUGIN_SYSINFO_POINTERS)Parameter;

    // GPU Devices

    PhAcquireQueuedLockShared(&GraphicsDevicesListLock);

    for (ULONG i = 0; i < GraphicsDevicesList->Count; i++)
    {
        PDV_GPU_ENTRY entry = PhReferenceObjectSafe(GraphicsDevicesList->Items[i]);

        if (!entry)
            continue;

        if (entry->DevicePresent)
        {
            GraphicsDeviceSysInfoInitializing(pluginEntry, entry);
        }

        PhDereferenceObjectDeferDelete(entry);
    }

    PhReleaseQueuedLockShared(&GraphicsDevicesListLock);

    // Disk Drives

    PhAcquireQueuedLockShared(&DiskDrivesListLock);

    for (ULONG i = 0; i < DiskDrivesList->Count; i++)
    {
        PDV_DISK_ENTRY entry = PhReferenceObjectSafe(DiskDrivesList->Items[i]);

        if (!entry)
            continue;

        if (entry->DevicePresent)
        {
            DiskDriveSysInfoInitializing(pluginEntry, entry);
        }

        PhDereferenceObjectDeferDelete(entry);
    }

    PhReleaseQueuedLockShared(&DiskDrivesListLock);

    // Network Adapters

    PhAcquireQueuedLockShared(&NetworkAdaptersListLock);

    for (ULONG i = 0; i < NetworkAdaptersList->Count; i++)
    {
        PDV_NETADAPTER_ENTRY entry = PhReferenceObjectSafe(NetworkAdaptersList->Items[i]);

        if (!entry)
            continue;

        if (entry->DevicePresent)
        {
            NetAdapterSysInfoInitializing(pluginEntry, entry);
        }

        PhDereferenceObjectDeferDelete(entry);
    }

    PhReleaseQueuedLockShared(&NetworkAdaptersListLock);

    // RAPL Devices

    PhAcquireQueuedLockShared(&RaplDevicesListLock);

    for (ULONG i = 0; i < RaplDevicesList->Count; i++)
    {
        PDV_RAPL_ENTRY entry = PhReferenceObjectSafe(RaplDevicesList->Items[i]);

        if (!entry)
            continue;

        if (entry->DevicePresent)
        {
            RaplDeviceSysInfoInitializing(pluginEntry, entry);
        }

        PhDereferenceObjectDeferDelete(entry);
    }

    PhReleaseQueuedLockShared(&RaplDevicesListLock);
}

VOID NTAPI SettingsUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    LoadSettings();
}

PPH_STRING TrimString(
    _In_ PPH_STRING String
    )
{
    static PH_STRINGREF whitespace = PH_STRINGREF_INIT(L" \t\r\n");
    return PhCreateString3(&String->sr, 0, &whitespace);
}

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

_Success_(return)
BOOLEAN HardwareDeviceShowSecurity(
    _In_ HWND ParentWindow,
    _In_ PPH_STRING DeviceInstance
    )
{
    DEVINST deviceInstanceHandle;
    CONFIGRET result;
    PBYTE buffer;
    ULONG bufferSize;
    DEVPROPTYPE propertyType;

    if (CM_Locate_DevNode(
        &deviceInstanceHandle,
        DeviceInstance->Buffer,
        CM_LOCATE_DEVNODE_NORMAL
        ) != CR_SUCCESS)
    {
        return FALSE;
    }

    bufferSize = 0x80;
    buffer = PhAllocate(bufferSize);
    propertyType = DEVPROP_TYPE_EMPTY;

    if ((result = CM_Get_DevNode_Property(
        deviceInstanceHandle,
        &DEVPKEY_Device_Security,
        &propertyType,
        buffer,
        &bufferSize,
        0
        )) == CR_BUFFER_SMALL)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        result = CM_Get_DevNode_Property(
            deviceInstanceHandle,
            &DEVPKEY_Device_Security,
            &propertyType,
            buffer,
            &bufferSize,
            0
            );
    }

    return FALSE;
}

BOOLEAN HardwareDeviceShowProperties(
    _In_ HWND WindowHandle,
    _In_ PPH_STRING DeviceInstance
    )
{
    PVOID devMgrHandle;

    // https://msdn.microsoft.com/en-us/library/ff548181.aspx
    //VOID (WINAPI* DeviceProperties_RunDLL_I)( // No resources tab (dmex)
    //    _In_ HWND hwndStub,
    //    _In_ HINSTANCE hAppInstance,
    //    _In_ PWSTR lpCmdLine,
    //    _In_ INT nCmdShow
    //    );

    INT_PTR (WINAPI* DevicePropertiesW)( // Includes resources tab (dmex)
        _In_opt_ HWND ParentWindow,
        _In_opt_ PCWSTR MachineName,
        _In_opt_ PCWSTR DeviceID,
        _In_ BOOL ShowDeviceManager
        );

    //PhShellExecuteEx(
    //    GetParent(WindowHandle),
    //    L"DeviceProperties.exe", // auto-elevated (dmex)
    //    PhGetString(DeviceInstance),
    //    NULL,
    //    SW_SHOWDEFAULT,
    //    0,
    //    0,
    //    NULL
    //    );

    if (devMgrHandle = PhLoadLibrary(L"devmgr.dll"))
    {
        if (DevicePropertiesW = PhGetProcedureAddress(devMgrHandle, "DevicePropertiesW", 0))
        {
            DevicePropertiesW(GetParent(WindowHandle), NULL, PhGetString(DeviceInstance), FALSE);
        }

        //if (DeviceProperties_RunDLL_I = PhGetProcedureAddress(devMgrHandle, "DeviceProperties_RunDLLW", 0))
        //{
        //    PH_FORMAT format[2];
        //    WCHAR formatBuffer[512];
        //
        //    // /DeviceID %s
        //    PhInitFormatS(&format[0], L"/DeviceID ");
        //    PhInitFormatSR(&format[1], DeviceInstance->sr);
        //
        //    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        //    {
        //        // This will sometimes re-throw an RPC error while debugging and can safely be ignored. (dmex)
        //        DeviceProperties_RunDLL_I(
        //            GetParent(WindowHandle),
        //            NULL,
        //            formatBuffer,
        //            0
        //            );
        //    }
        //    else
        //    {
        //        // This will sometimes re-throw an RPC error while debugging and can safely be ignored. (dmex)
        //        DeviceProperties_RunDLL_I(
        //            GetParent(WindowHandle),
        //            NULL,
        //            PhaFormatString(L"/DeviceID %s", DeviceInstance->Buffer)->Buffer,
        //            0
        //            );
        //    }
        //}

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

VOID ShowDeviceMenu(
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
    PhInsertEMenuItem(subMenu, PhCreateEMenuItem(0, HW_KEY_INDEX_HARDWARE, L"Hardware", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(subMenu, PhCreateEMenuItem(0, HW_KEY_INDEX_SOFTWARE, L"Software", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(subMenu, PhCreateEMenuItem(0, HW_KEY_INDEX_USER, L"User", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(subMenu, PhCreateEMenuItem(0, HW_KEY_INDEX_CONFIG, L"Config", NULL, NULL), ULONG_MAX);
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
        case HW_KEY_INDEX_HARDWARE:
        case HW_KEY_INDEX_SOFTWARE:
        case HW_KEY_INDEX_USER:
        case HW_KEY_INDEX_CONFIG:
            HardwareDeviceOpenKey(ParentWindow, DeviceInstance, selectedItem->Id);
            break;
        case 10:
            HardwareDeviceShowProperties(ParentWindow, DeviceInstance);
            break;
        }
    }

    PhDestroyEMenu(menu);
}

LOGICAL DllMain(
    _In_ HINSTANCE Instance,
    _In_ ULONG Reason,
    _Reserved_ PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PPH_PLUGIN_INFORMATION info;
            PH_SETTING_CREATE settings[] =
            {
                { IntegerSettingType, SETTING_NAME_ENABLE_NDIS, L"1" },
                { StringSettingType, SETTING_NAME_INTERFACE_LIST, L"" },
                { IntegerPairSettingType, SETTING_NAME_NETWORK_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_NETWORK_SIZE, L"@96|309,265" },
                { StringSettingType, SETTING_NAME_NETWORK_COLUMNS, L"" },
                { StringSettingType, SETTING_NAME_NETWORK_SORTCOLUMN, L"" },
                { StringSettingType, SETTING_NAME_DISK_LIST, L"" },
                { IntegerPairSettingType, SETTING_NAME_DISK_POSITION, L"100,100" },
                { ScalableIntegerPairSettingType, SETTING_NAME_DISK_SIZE, L"@96|309,265" },
                { StringSettingType, SETTING_NAME_DISK_COUNTERS_COLUMNS, L"" },
                { StringSettingType, SETTING_NAME_SMART_COUNTERS_COLUMNS, L"" },
                { StringSettingType, SETTING_NAME_RAPL_LIST, L"" },
                { StringSettingType, SETTING_NAME_GRAPHICS_LIST, L"" },
                { IntegerPairSettingType, SETTING_NAME_GRAPHICS_NODES_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_GRAPHICS_NODES_WINDOW_SIZE, L"@96|850,490" },
                { IntegerSettingType, SETTING_NAME_DEVICE_TREE_AUTO_REFRESH, L"1" },
                { IntegerSettingType, SETTING_NAME_DEVICE_TREE_SHOW_DISCONNECTED, L"0" },
                { IntegerSettingType, SETTING_NAME_DEVICE_TREE_HIGHLIGHT_UPPER_FILTERED, L"0" },
                { IntegerSettingType, SETTING_NAME_DEVICE_TREE_HIGHLIGHT_LOWER_FILTERED, L"0" },
                { IntegerPairSettingType, SETTING_NAME_DEVICE_TREE_SORT, L"0,0" },
                { StringSettingType, SETTING_NAME_DEVICE_TREE_COLUMNS, L"" },
                { IntegerSettingType, SETTING_NAME_DEVICE_PROBLEM_COLOR, L"283cff" },
                { IntegerSettingType, SETTING_NAME_DEVICE_DISABLED_COLOR, L"777777" },
                { IntegerSettingType, SETTING_NAME_DEVICE_DISCONNECTED_COLOR, L"6d6d6d" },
                { IntegerSettingType, SETTING_NAME_DEVICE_HIGHLIGHT_COLOR, L"00aaff" },
                { IntegerSettingType, SETTING_NAME_DEVICE_INTERFACE_COLOR, L"ffccaa" },
                { IntegerSettingType, SETTING_NAME_DEVICE_DISABLED_INTERFACE_COLOR, L"886644" },
                { IntegerSettingType, SETTING_NAME_DEVICE_SORT_CHILDREN_BY_NAME, L"1" },
                { IntegerSettingType, SETTING_NAME_DEVICE_SHOW_ROOT, L"0" },
                { IntegerSettingType, SETTING_NAME_DEVICE_SHOW_SOFTWARE_COMPONENTS, L"1" },
                { IntegerSettingType, SETTING_NAME_DEVICE_SHOW_DEVICE_INTERFACES, L"0" },
                { IntegerSettingType, SETTING_NAME_DEVICE_SHOW_DISABLED_DEVICE_INTERFACES, L"0" },
                { IntegerPairSettingType, SETTING_NAME_DEVICE_PROPERTIES_POSITION, L"0,0" },
                { StringSettingType, SETTING_NAME_DEVICE_GENERAL_COLUMNS, L"" },
                { StringSettingType, SETTING_NAME_DEVICE_PROPERTIES_COLUMNS, L"" },
                { StringSettingType, SETTING_NAME_DEVICE_INTERFACES_COLUMNS, L"" },
            };

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Hardware Devices";
            info->Author = L"dmex, wj32, jxy-s";
            info->Description = L"Plugin for monitoring hardware devices like Disk drives and Network adapters via the System Information window.";

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );
            //PhRegisterCallback(
            //    PhGetPluginCallback(PluginInstance, PluginCallbackUnload),
            //    UnloadCallback,
            //    NULL,
            //    &PluginUnloadCallbackRegistration
            //    );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackOptionsWindowInitializing),
                ShowOptionsCallback,
                NULL,
                &PluginShowOptionsCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackMainWindowShowing),
                MainWindowShowingCallback,
                NULL,
                &MainWindowShowingCallbackRegistration
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
                ProcessesUpdatedCallback,
                NULL,
                &ProcessesUpdatedCallbackRegistration
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackSystemInformationInitializing),
                SystemInformationInitializingCallback,
                NULL,
                &SystemInformationInitializingCallbackRegistration
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackSettingsUpdated),
                SettingsUpdatedCallback,
                NULL,
                &SettingsUpdatedCallbackRegistration
                );

            PhAddSettings(settings, RTL_NUMBER_OF(settings));
        }
        break;
    }

    return TRUE;
}
