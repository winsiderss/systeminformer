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
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    DialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS),
        (HWND)Parameter,
        OptionsDlgProc
        );
}

VOID NTAPI MenuItemCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = (PPH_PLUGIN_MENU_ITEM)Parameter;
    PPH_NETWORK_ITEM networkItem = (PPH_NETWORK_ITEM)menuItem->Context;

    switch (menuItem->Id)
    {
    case NETWORK_ACTION_PING:
        PerformNetworkAction(NETWORK_ACTION_PING, networkItem);
        break;
    case NETWORK_ACTION_TRACEROUTE:
        PerformNetworkAction(NETWORK_ACTION_TRACEROUTE, networkItem);
        break;
    case NETWORK_ACTION_WHOIS:
        PerformNetworkAction(NETWORK_ACTION_WHOIS, networkItem);
        break;
    case NETWORK_ACTION_PATHPING:
        PerformNetworkAction(NETWORK_ACTION_PATHPING, networkItem);
        break;
    }
}

VOID NTAPI NetworkMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = (PPH_PLUGIN_MENU_INFORMATION)Parameter;
    PPH_NETWORK_ITEM networkItem;
    PPH_EMENU_ITEM toolsMenu;
    PPH_EMENU_ITEM closeMenuItem;

    if (menuInfo->u.Network.NumberOfNetworkItems == 1)
        networkItem = menuInfo->u.Network.NetworkItems[0];
    else
        networkItem = NULL;

    // Create the Tools menu.
    toolsMenu = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"Tools", NULL);
    PhInsertEMenuItem(toolsMenu, PhPluginCreateEMenuItem(PluginInstance, 0, NETWORK_ACTION_PING, L"Ping", networkItem), -1);
    PhInsertEMenuItem(toolsMenu, PhPluginCreateEMenuItem(PluginInstance, 0, NETWORK_ACTION_TRACEROUTE, L"Traceroute", networkItem), -1);
    PhInsertEMenuItem(toolsMenu, PhPluginCreateEMenuItem(PluginInstance, 0, NETWORK_ACTION_WHOIS, L"Whois", networkItem), -1);
    PhInsertEMenuItem(toolsMenu, PhPluginCreateEMenuItem(PluginInstance, 0, NETWORK_ACTION_PATHPING, L"PathPing", networkItem), -1);

    // Insert the Tools menu into the network menu.
    closeMenuItem = PhFindEMenuItem(menuInfo->Menu, 0, L"Close", 0);
    PhInsertEMenuItem(menuInfo->Menu, toolsMenu, closeMenuItem ? PhIndexOfEMenuItem(menuInfo->Menu, closeMenuItem) : 1);

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
                { IntegerPairSettingType, SETTING_NAME_TRACERT_WINDOW_POSITION, L"0,0" },
                { IntegerPairSettingType, SETTING_NAME_TRACERT_WINDOW_SIZE, L"600,365" },
                { IntegerPairSettingType, SETTING_NAME_PING_WINDOW_POSITION, L"0,0" },
                { IntegerPairSettingType, SETTING_NAME_PING_WINDOW_SIZE, L"420,250" },
                { IntegerSettingType, SETTING_NAME_PING_TIMEOUT, L"3e8" } // 1000 timeout.
            };

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Network Tools";
            info->Author = L"dmex, wj32";
            info->Description = L"Provides ping, traceroute and whois for network connections.";
            info->Url = L"http://processhacker.sf.net/forums/viewtopic.php?t=1117";
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

            PhAddSettings(settings, ARRAYSIZE(settings));
        }
        break;
    }

    return TRUE;
}