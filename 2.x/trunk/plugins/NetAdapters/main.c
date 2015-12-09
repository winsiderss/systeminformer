/*
 * Process Hacker Extra Plugins -
 *   Network Adapters Plugin
 *
 * Copyright (C) 2015 dmex
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

#include "main.h"

PPH_LIST NetworkAdaptersList = NULL;
PPH_PLUGIN PluginInstance = NULL;
static PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginUnloadCallbackRegistration;
static PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
static PH_CALLBACK_REGISTRATION SystemInformationInitializingCallbackRegistration;

static VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_STRING string = NULL;

    if (WindowsVersion > WINDOWS_VISTA)
    {
        if (IphlpHandle = PhGetDllHandle(L"iphlpapi.dll"))
        {
            GetIfEntry2_I = PhGetProcedureAddress(IphlpHandle, "GetIfEntry2", 0);
            GetInterfaceDescriptionFromGuid_I = PhGetProcedureAddress(IphlpHandle, "NhGetInterfaceDescriptionFromGuid", 0);
            NotifyIpInterfaceChange_I = PhGetProcedureAddress(IphlpHandle, "NotifyUnicastIpAddressChange", 0);
            CancelMibChangeNotify2_I = PhGetProcedureAddress(IphlpHandle, "CancelMibChangeNotify2", 0);
            ConvertLengthToIpv4Mask_I = PhGetProcedureAddress(IphlpHandle, "ConvertLengthToIpv4Mask", 0);
        }
    }

    NetworkAdaptersList = PhCreateList(1);

    string = PhGetStringSetting(SETTING_NAME_INTERFACE_LIST);
    LoadAdaptersList(NetworkAdaptersList, string);
    PhDereferenceObject(string);
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
    ShowOptionsDialog((HWND)Parameter);
}

static VOID NTAPI SystemInformationInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_SYSINFO_POINTERS pluginEntry = (PPH_PLUGIN_SYSINFO_POINTERS)Parameter;

    for (ULONG i = 0; i < NetworkAdaptersList->Count; i++)
    {
        PPH_NETADAPTER_ENTRY entry = (PPH_NETADAPTER_ENTRY)NetworkAdaptersList->Items[i];

        NetAdapterSysInfoInitializing(pluginEntry, entry);
    }
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
                { StringSettingType, SETTING_NAME_INTERFACE_LIST, L"" }
            };

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Network Adapters";
            info->Author = L"dmex";
            info->Description = L"Plugin for monitoring specific network adapter throughput via the System Information window.";
            info->Url = L"http://processhacker.sf.net/forums/viewtopic.php?t=1820";
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