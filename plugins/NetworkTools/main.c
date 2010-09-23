/*
 * Process Hacker Network Tools - 
 *   main program
 * 
 * Copyright (C) 2010 wj32
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

#include <phdk.h>
#include "resource.h"

VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID NTAPI NetworkMenuInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION NetworkMenuInitializingCallbackRegistration;

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
            PH_PLUGIN_INFORMATION info;

            info.DisplayName = L"Network Tools";
            info.Author = L"wj32";
            info.Description = L"Provides ping, traceroute and whois for network connections.";
            info.HasOptions = FALSE;

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.NetworkTools", Instance, &info);

            if (!PluginInstance)
                return FALSE;

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
                PhGetGeneralCallback(GeneralCallbackNetworkMenuInitializing),
                NetworkMenuInitializingCallback,
                NULL,
                &NetworkMenuInitializingCallbackRegistration
                );
        }
        break;
    }

    return TRUE;
}

VOID NTAPI LoadCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    // Nothing
}

VOID NTAPI ShowOptionsCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    // Nothing
}

VOID NTAPI MenuItemCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = Parameter;
    PPH_NETWORK_ITEM networkItem;

    switch (menuItem->Id)
    {
    case ID_TOOLS_PING:
        networkItem = menuItem->Context;
        PhShowInformation(PhMainWndHandle, L"You chose Ping for %s", networkItem->RemoteAddressString);
        break;
    case ID_TOOLS_TRACEROUTE:
        networkItem = menuItem->Context;
        PhShowInformation(PhMainWndHandle, L"You chose Traceroute for %s", networkItem->RemoteAddressString);
        break;
    case ID_TOOLS_WHOIS:
        networkItem = menuItem->Context;
        PhShowInformation(PhMainWndHandle, L"You chose Whois for %s", networkItem->RemoteAddressString);
        break;
    }
}

VOID NTAPI NetworkMenuInitializingCallback(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
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
