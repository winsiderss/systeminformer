/*
 * Process Hacker Plugins -
 *   Network Adapters Plugin
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

#include "netadapters.h"

PPH_PLUGIN PluginInstance = NULL;

PPH_OBJECT_TYPE NetAdapterEntryType = NULL;
PPH_LIST NetworkAdaptersList = NULL;
PH_QUEUED_LOCK NetworkAdaptersListLock = PH_QUEUED_LOCK_INIT;

PPH_OBJECT_TYPE DiskDriveEntryType = NULL;
PPH_LIST DiskDrivesList = NULL;
PH_QUEUED_LOCK DiskDrivesListLock = PH_QUEUED_LOCK_INIT;

static PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;
static PH_CALLBACK_REGISTRATION SystemInformationInitializingCallbackRegistration;

static VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (WindowsVersion >= WINDOWS_VISTA)
    {
        if (IphlpHandle = LoadLibrary(L"iphlpapi.dll"))
        {
            GetIfEntry2_I = PhGetProcedureAddress(IphlpHandle, "GetIfEntry2", 0);
            GetInterfaceDescriptionFromGuid_I = PhGetProcedureAddress(IphlpHandle, "NhGetInterfaceDescriptionFromGuid", 0);
            NotifyIpInterfaceChange_I = PhGetProcedureAddress(IphlpHandle, "NotifyUnicastIpAddressChange", 0);
            CancelMibChangeNotify2_I = PhGetProcedureAddress(IphlpHandle, "CancelMibChangeNotify2", 0);
            ConvertLengthToIpv4Mask_I = PhGetProcedureAddress(IphlpHandle, "ConvertLengthToIpv4Mask", 0);
        }
    }

    DiskInitialize();
    DiskDriveLoadList();

    NetAdaptersInitialize();
    NetAdaptersLoadList();
}

static VOID NTAPI UnloadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    NOTHING;
}

static VOID NTAPI ShowOptionsCallback(
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
    propSheetHeader.pszCaption = L"Devices Plugin";
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

static VOID NTAPI ProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    DiskDrivesUpdate();
    NetAdaptersUpdate();
}

static VOID NTAPI SystemInformationInitializingCallback(
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

        DiskDriveSysInfoInitializing(pluginEntry, entry);
    }

    PhReleaseQueuedLockShared(&DiskDrivesListLock);

    // Network Adapters

    PhAcquireQueuedLockShared(&NetworkAdaptersListLock);

    for (ULONG i = 0; i < NetworkAdaptersList->Count; i++)
    {
        PDV_NETADAPTER_ENTRY entry = PhReferenceObjectSafe(NetworkAdaptersList->Items[i]);

        if (!entry)
            continue;

        NetAdapterSysInfoInitializing(pluginEntry, entry);
    }

    PhReleaseQueuedLockShared(&NetworkAdaptersListLock);
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
                { IntegerSettingType, SETTING_NAME_ENABLE_HIDDEN_ADAPTERS, L"0" },
                { StringSettingType, SETTING_NAME_INTERFACE_LIST, L"" },
                { StringSettingType, SETTING_NAME_DISK_LIST, L"" }
            };

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Network Adapters";
            info->Author = L"dmex, wj32";
            info->Description = L"Plugin for monitoring specific network adapter throughput via the System Information window.";
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