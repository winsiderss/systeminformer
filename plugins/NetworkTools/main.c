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
PH_CALLBACK_REGISTRATION MainMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION NetworkMenuInitializingCallbackRegistration;

VOID NTAPI LoadCallback(
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
    ShowOptionsDialog((HWND)Parameter);
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
        ShowTracertWindow(networkItem);
        break;
    case NETWORK_ACTION_WHOIS:
        PerformNetworkAction(NETWORK_ACTION_WHOIS, networkItem);
        break;
    case NETWORK_ACTION_PATHPING:
        PerformNetworkAction(NETWORK_ACTION_PATHPING, networkItem);
        break;
    case MAINMENU_ACTION_PING:
        {

        }
        break;
    case MAINMENU_ACTION_TRACERT:
        {
            BOOLEAN success = FALSE;
            PH_IP_ENDPOINT RemoteEndpoint;
            PPH_STRING selectedChoice = NULL;

            while (PhaChoiceDialog(
                menuItem->OwnerWindow,
                L"Tracert",
                L"IP address for trace:",
                NULL,
                0,
                NULL,
                PH_CHOICE_DIALOG_USER_CHOICE,
                &selectedChoice,
                NULL,
                SETTING_NAME_TRACERT_HISTORY
                ))
            {
                PWSTR terminator = NULL;

                if (NT_SUCCESS(RtlIpv4StringToAddress(selectedChoice->Buffer, TRUE, &terminator, &RemoteEndpoint.Address.InAddr)))
                {
                    RemoteEndpoint.Address.Type = PH_IPV4_NETWORK_TYPE;
                    success = TRUE;
                    break;
                }

                if (NT_SUCCESS(RtlIpv6StringToAddress(selectedChoice->Buffer, &terminator, &RemoteEndpoint.Address.In6Addr)))
                {
                    RemoteEndpoint.Address.Type = PH_IPV6_NETWORK_TYPE;
                    success = TRUE;
                    break;
                }
            }

            if (success)
            {
                ShowTracertWindowFromAddress(RemoteEndpoint);
            }
        }
        break;
    case MAINMENU_ACTION_WHOIS:
        {
            BOOLEAN success = FALSE;
            PH_IP_ENDPOINT RemoteEndpoint;
            PPH_STRING selectedChoice = NULL;

            while (PhaChoiceDialog(
                menuItem->OwnerWindow,
                L"Whois",
                L"IP address for Whois:",
                NULL,
                0,
                NULL,
                PH_CHOICE_DIALOG_USER_CHOICE,
                &selectedChoice,
                NULL,
                SETTING_NAME_TRACERT_HISTORY
                ))
            {
                PWSTR terminator = NULL;

                if (NT_SUCCESS(RtlIpv4StringToAddress(selectedChoice->Buffer, TRUE, &terminator, &RemoteEndpoint.Address.InAddr)))
                {
                    RemoteEndpoint.Address.Type = PH_IPV4_NETWORK_TYPE;
                    success = TRUE;
                    break;
                }

                if (NT_SUCCESS(RtlIpv6StringToAddress(selectedChoice->Buffer, &terminator, &RemoteEndpoint.Address.In6Addr)))
                {
                    RemoteEndpoint.Address.Type = PH_IPV6_NETWORK_TYPE;
                    success = TRUE;
                    break;
                }
            }

            if (success)
            {
                PerformTracertAction(NETWORK_ACTION_WHOIS, RemoteEndpoint);
            }
        }
        break;
    }
}

VOID NTAPI MainMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_EMENU_ITEM networkToolsMenu;

    if (!menuInfo || menuInfo->u.MainMenu.SubMenuIndex != PH_MENU_ITEM_LOCATION_TOOLS)
        return;

    networkToolsMenu = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"Network Tools", NULL);
    PhInsertEMenuItem(networkToolsMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MAINMENU_ACTION_PING, L"Ping IP address...", NULL), -1);
    PhInsertEMenuItem(networkToolsMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MAINMENU_ACTION_TRACERT, L"Traceroute IP address...", NULL), -1);
    PhInsertEMenuItem(networkToolsMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MAINMENU_ACTION_WHOIS, L"Whois IP address...", NULL), -1);

    PhInsertEMenuItem(menuInfo->Menu, PhCreateEMenuItem(PH_EMENU_SEPARATOR, 0, NULL, NULL, NULL), -1);
    PhInsertEMenuItem(menuInfo->Menu, networkToolsMenu, -1);
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
                { IntegerPairSettingType, SETTING_NAME_PING_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_PING_WINDOW_SIZE, L"@96|420,250" },
                { IntegerSettingType, SETTING_NAME_PING_MINIMUM_SCALING, L"64" }, // 100ms minimum scaling
                { IntegerSettingType, SETTING_NAME_PING_SIZE, L"20" }, // 32 byte packet
                { IntegerPairSettingType, SETTING_NAME_TRACERT_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_TRACERT_WINDOW_SIZE, L"@96|600,365" },
                { StringSettingType, SETTING_NAME_TRACERT_COLUMNS, L"" },
                { StringSettingType, SETTING_NAME_TRACERT_HISTORY, L"" },
                { IntegerSettingType, SETTING_NAME_TRACERT_MAX_HOPS, L"30" },
                { IntegerPairSettingType, SETTING_NAME_OUTPUT_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_OUTPUT_WINDOW_SIZE, L"@96|600,365" },
            };

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Network Tools";
            info->Author = L"dmex, wj32";
            info->Description = L"Provides ping, traceroute and whois for network connections.";
            info->Url = L"https://wj32.org/processhacker/forums/viewtopic.php?t=1117";
            info->HasOptions = TRUE;
  
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );
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
                PhGetGeneralCallback(GeneralCallbackMainMenuInitializing),
                MainMenuInitializingCallback,
                NULL,
                &MainMenuInitializingCallbackRegistration
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