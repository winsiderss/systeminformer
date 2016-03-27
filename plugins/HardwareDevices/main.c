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
    DiskDrivesInitialize();
    NetAdaptersInitialize();

    DiskDrivesLoadList();
    NetAdaptersLoadList();

#ifdef _NV_GPU_BUILD
    NvApiInitialize();
#endif
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
    PROPSHEETHEADER propSheetHeader = { sizeof(propSheetHeader) };
    PROPSHEETPAGE propSheetPage;
    HPROPSHEETPAGE pages[2];

    propSheetHeader.dwFlags =
        PSH_NOAPPLYNOW |
        PSH_NOCONTEXTHELP;
    propSheetHeader.hwndParent = (HWND)Parameter;
    propSheetHeader.pszCaption = L"Hardware Devices Plugin";
    propSheetHeader.nPages = 0;
    propSheetHeader.nStartPage = 0;
    propSheetHeader.phpage = pages;

    // Disk Drives
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.hInstance = PluginInstance->DllBase;
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_DISKDRIVE_OPTIONS);
    propSheetPage.pfnDlgProc = DiskDriveOptionsDlgProc;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    // Network Adapters
    memset(&propSheetPage, 0, sizeof(PROPSHEETPAGE));
    propSheetPage.dwSize = sizeof(PROPSHEETPAGE);
    propSheetPage.hInstance = PluginInstance->DllBase;
    propSheetPage.pszTemplate = MAKEINTRESOURCE(IDD_NETADAPTER_OPTIONS);
    propSheetPage.pfnDlgProc = NetworkAdapterOptionsDlgProc;
    pages[propSheetHeader.nPages++] = CreatePropertySheetPage(&propSheetPage);

    PhModalPropertySheet(&propSheetHeader);
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

#ifdef _NV_GPU_BUILD
    NvGpuUpdate();
#endif
}

VOID NTAPI SystemInformationInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_SYSINFO_POINTERS pluginEntry = (PPH_PLUGIN_SYSINFO_POINTERS)Parameter;

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

    // Graphics cards
#ifdef _NV_GPU_BUILD
    NvGpuSysInfoInitializing(pluginEntry);
#endif
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

INT AddListViewGroup(
    _In_ HWND ListViewHandle,
    _In_ INT GroupId,
    _In_ PWSTR Text
    )
{
    LVGROUP group;

    group.cbSize = LVGROUP_V5_SIZE;
    group.mask = LVGF_HEADER;
    group.pszHeader = Text;

    if (WindowsVersion >= WINDOWS_VISTA)
    {
        group.cbSize = sizeof(LVGROUP);
        group.mask |= LVGF_ALIGN | LVGF_STATE | LVGF_GROUPID;
        group.uAlign = LVGA_HEADER_LEFT;
        group.state = LVGS_COLLAPSIBLE;
        group.iGroupId = GroupId;
    }

    return (INT)ListView_InsertGroup(ListViewHandle, MAXINT, &group);
}

INT AddListViewItemGroupId(
    _In_ HWND ListViewHandle,
    _In_ INT GroupId,
    _In_ INT Index,
    _In_ PWSTR Text,
    _In_opt_ PVOID Param
    )
{
    LVITEM item;

    item.mask = LVIF_TEXT;
    item.iItem = Index;
    item.iSubItem = 0;
    item.pszText = Text;

    if (WindowsVersion >= WINDOWS_VISTA)
    {
        item.mask |= LVIF_GROUPID;
        item.iGroupId = GroupId;
    }

    if (Param)
    {
        item.mask |= LVIF_PARAM;
        item.lParam = (LPARAM)Param;
    }

    return ListView_InsertItem(ListViewHandle, &item);
}

ULONG64 RegQueryUlong64(
    _In_ HANDLE KeyHandle,
    _In_ PWSTR ValueName
    )
{
    ULONG64 value = 0;
    PH_STRINGREF valueName;
    PKEY_VALUE_PARTIAL_INFORMATION buffer;

    PhInitializeStringRef(&valueName, ValueName);

    if (NT_SUCCESS(PhQueryValueKey(KeyHandle, &valueName, KeyValuePartialInformation, &buffer)))
    {
        if (buffer->Type == REG_DWORD || buffer->Type == REG_QWORD)
        {
            value = *(ULONG64*)buffer->Data;
        }

        PhFree(buffer);
    }

    return value;
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
    //PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 0, L"Enable", NULL, NULL), -1);
    //PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"Disable", NULL, NULL), -1);
    //PhInsertEMenuItem(menu, PhCreateEMenuItem(PH_EMENU_SEPARATOR, 0, NULL, NULL, NULL), -1);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, 1, L"Properties", NULL, NULL), -1);

    selectedItem = PhShowEMenu(
        menu,
        PhMainWndHandle,
        PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_TOP,
        cursorPos.x,
        cursorPos.y
        );

    if (selectedItem && selectedItem->Id != -1)
    {
        switch (selectedItem->Id)
        {
        case 1:
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
                    if (DeviceProperties_RunDLL_I = (PVOID)GetProcAddress(devMgrHandle, "DeviceProperties_RunDLLW"))
                    {
                        // This will sometimes re-throw an RPC error during debugging and can be safely ignored.
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
                { StringSettingType, SETTING_NAME_DISK_LIST, L"" },
#ifdef _NV_GPU_BUILD
                { IntegerSettingType, SETTING_NAME_ENABLE_GPU, L"1" },
                { IntegerSettingType, SETTING_NAME_ENABLE_FAHRENHEIT, L"0" }  
#endif
            };

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Hardware Devices";
            info->Author = L"dmex, wj32";
            info->Description = L"Plugin for monitoring hardware devices like Disk drives and Network adapters via the System Information window.";
            info->Url = L"https://wj32.org/processhacker/forums/viewtopic.php?t=1820";
            info->HasOptions = TRUE;

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
                PhGetPluginCallback(PluginInstance, PluginCallbackShowOptions),
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
                &PhProcessesUpdatedEvent,
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

            PhAddSettings(settings, ARRAYSIZE(settings));
        }
        break;
    }

    return TRUE;
}