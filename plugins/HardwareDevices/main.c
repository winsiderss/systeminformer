/*
 * Process Hacker Plugins -
 *   Hardware Devices Plugin
 *
 * Copyright (C) 2015-2016 dmex
 * Copyright (C) 2016 wj32
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

#include "devices.h"

PPH_PLUGIN PluginInstance = NULL;

PPH_OBJECT_TYPE NetAdapterEntryType = NULL;
PPH_LIST NetworkAdaptersList = NULL;
PH_QUEUED_LOCK NetworkAdaptersListLock = PH_QUEUED_LOCK_INIT;

PPH_OBJECT_TYPE DiskDriveEntryType = NULL;
PPH_LIST DiskDrivesList = NULL;
PH_QUEUED_LOCK DiskDrivesListLock = PH_QUEUED_LOCK_INIT;

PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION MainWindowShowingCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;
PH_CALLBACK_REGISTRATION SystemInformationInitializingCallbackRegistration;

VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    HdPropInitialization();

    DiskDrivesInitialize();
    NetAdaptersInitialize();

    DiskDrivesLoadList();
    NetAdaptersLoadList();
}

VOID NTAPI UnloadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

VOID NTAPI ShowOptionsCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_OPTIONS_POINTERS optionsEntry = (PPH_PLUGIN_OPTIONS_POINTERS)Parameter;

    if (!optionsEntry)
        return;

    optionsEntry->CreateSection(
        L"Disk Drives",
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_DISKDRIVE_OPTIONS),
        DiskDriveOptionsDlgProc,
        NULL
        );

    optionsEntry->CreateSection(
        L"Network Adapters",
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_NETADAPTER_OPTIONS),
        NetworkAdapterOptionsDlgProc,
        NULL
        );
}

VOID NTAPI MainWindowShowingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    AddRemoveDeviceChangeCallback();
}

VOID NTAPI ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    DiskDrivesUpdate();
    NetAdaptersUpdate();
}

VOID NTAPI SystemInformationInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_SYSINFO_POINTERS pluginEntry = (PPH_PLUGIN_SYSINFO_POINTERS)Parameter;

    if (!pluginEntry)
        return;

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
    }

    PhReleaseQueuedLockShared(&NetworkAdaptersListLock);
}

PPH_STRING TrimString(
    _In_ PPH_STRING String
    )
{
    static PH_STRINGREF whitespace = PH_STRINGREF_INIT(L" \t\r\n");
    PH_STRINGREF sr = String->sr;
    PhTrimStringRef(&sr, &whitespace, 0);
    return PhCreateString2(&sr);
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

VOID ShowDeviceMenu(
    _In_ HWND ParentWindow,
    _In_ PPH_STRING DeviceInstance
    )
{
    POINT cursorPos;
    PPH_EMENU menu;
    PPH_EMENU_ITEM selectedItem;

    GetCursorPos(&cursorPos);

    menu = PhCreateEMenu();
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 0, L"Enable", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"Disable", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 2, L"Restart", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 3, L"Properties", NULL, NULL), ULONG_MAX);

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
                HMODULE devMgrHandle;

                // https://msdn.microsoft.com/en-us/library/ff548181.aspx
                VOID (WINAPI *DeviceProperties_RunDLL_I)(
                    _In_ HWND hwndStub,
                    _In_ HINSTANCE hAppInstance,
                    _In_ PWSTR lpCmdLine,
                    _In_ INT nCmdShow
                    );

                if (devMgrHandle = LoadLibrary(L"devmgr.dll"))
                {
                    if (DeviceProperties_RunDLL_I = PhGetProcedureAddress(devMgrHandle, "DeviceProperties_RunDLLW", 0))
                    {
                        // This will sometimes re-throw an RPC error while debugging and can safely be ignored.
                        DeviceProperties_RunDLL_I(
                            GetParent(ParentWindow),
                            NULL,
                            PhaFormatString(L"/DeviceID %s", DeviceInstance->Buffer)->Buffer,
                            0
                            );
                    }

                    FreeLibrary(devMgrHandle);
                }
            }
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
            };

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Hardware Devices";
            info->Author = L"dmex, wj32";
            info->Description = L"Plugin for monitoring hardware devices like Disk drives and Network adapters via the System Information window.";
            info->Url = L"https://wj32.org/processhacker/forums/viewtopic.php?t=1820";

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackUnload),
                UnloadCallback,
                NULL,
                &PluginUnloadCallbackRegistration
                );
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

            PhAddSettings(settings, RTL_NUMBER_OF(settings));
        }
        break;
    }

    return TRUE;
}
