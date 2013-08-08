/*
 * Process Hacker Network Tools -
 *   main program
 *
 * Copyright (C) 2010-2011 wj32
 * Copyright (C) 2013 dmex
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

#include "nettools.h"

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION NetworkMenuInitializingCallbackRegistration;

VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    DialogBox(
        (HINSTANCE)PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS),
        (HWND)Parameter,
        OptionsDlgProc
        );
}

VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = (PPH_PLUGIN_MENU_ITEM)Parameter;
    PPH_NETWORK_ITEM networkItem;

    switch (menuItem->Id)
    {
    case ID_TOOLS_PING:
        networkItem = menuItem->Context;
        PerformNetworkAction(NETWORK_ACTION_PING, networkItem);
        break;
    case ID_TOOLS_TRACEROUTE:
        networkItem = menuItem->Context;
        PerformNetworkAction(NETWORK_ACTION_TRACEROUTE, networkItem);
        break;
    case ID_TOOLS_WHOIS:
        networkItem = menuItem->Context;
        PerformNetworkAction(NETWORK_ACTION_WHOIS, networkItem);
        break;
    }
}

VOID NTAPI NetworkMenuInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = (PPH_PLUGIN_MENU_INFORMATION)Parameter;
    PPH_NETWORK_ITEM networkItem;
    PPH_EMENU_ITEM toolsMenu;

    if (menuInfo->u.Network.NumberOfNetworkItems == 1)
        networkItem = menuInfo->u.Network.NetworkItems[0];
    else
        networkItem = NULL;

    // Create the Tools menu.
    toolsMenu = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"Tools", NULL);
    PhInsertEMenuItem(toolsMenu, PhPluginCreateEMenuItem(PluginInstance, 0, ID_TOOLS_PING, L"Ping", networkItem), -1);
    PhInsertEMenuItem(toolsMenu, PhPluginCreateEMenuItem(PluginInstance, 0, ID_TOOLS_TRACEROUTE, L"Traceroute", networkItem), -1);
    PhInsertEMenuItem(toolsMenu, PhPluginCreateEMenuItem(PluginInstance, 0, ID_TOOLS_WHOIS, L"Whois", networkItem), -1);

    // Insert the Tools menu into the network menu.
    PhInsertEMenuItem(menuInfo->Menu, toolsMenu, 1);

    toolsMenu->Flags |= PH_EMENU_DISABLED;

    if (networkItem)
    {
        if (!PhIsNullIpAddress(&networkItem->RemoteEndpoint.Address))
        {
            toolsMenu->Flags &= ~PH_EMENU_DISABLED;
        }
    }
}

LOGICAL DllMain(
    __in HINSTANCE Instance,
    __in ULONG Reason,
    __reserved PVOID Reserved
    )
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        {
            PPH_PLUGIN_INFORMATION info;        
            PH_SETTING_CREATE settings[] =
            {
                { IntegerPairSettingType, L"ProcessHacker.NetTools.NetToolsWindowPosition", L"0,0" },
                { IntegerPairSettingType, L"ProcessHacker.NetTools.NetToolsWindowSize", L"600,365" },
                { IntegerSettingType, L"ProcessHacker.NetTools.MaxPingCount", L"4" },
                { IntegerSettingType, L"ProcessHacker.NetTools.MaxPingTimeout", L"5" },
                { IntegerSettingType, L"ProcessHacker.NetTools.EnableHostnameLookup", L"1" },
            };

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.NetworkTools", Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Network Tools";
            info->Author = L"dmex & wj32";
            info->Description = L"Provides ping, traceroute and whois for network connections.";
            info->HasOptions = TRUE;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackShowOptions),
                ShowOptionsCallback,
                NULL,
                &PluginShowOptionsCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackMenuItem),
                MenuItemCallback,
                NULL,
                &PluginMenuItemCallbackRegistration
                );

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackNetworkMenuInitializing),
                NetworkMenuInitializingCallback,
                NULL,
                &NetworkMenuInitializingCallbackRegistration
                );

            PhAddSettings(settings, _countof(settings));
        }
        break;
    }

    return TRUE;
}



