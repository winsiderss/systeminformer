/*
 * Process Hacker Network Tools -
 *   main program
 *
 * Copyright (C) 2010-2011 wj32
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
#define MAIN_PRIVATE
#include "nettools.h"
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
            PPH_PLUGIN_INFORMATION info;

            PluginInstance = PhRegisterPlugin(L"ProcessHacker.NetworkTools", Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Network Tools";
            info->Author = L"wj32";
            info->Description = L"Provides ping, traceroute and whois for network connections.";
            info->HasOptions = FALSE;

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
        PerformNetworkAction(PhMainWndHandle, NETWORK_ACTION_PING, &networkItem->RemoteEndpoint.Address);
        break;
    case ID_TOOLS_TRACEROUTE:
        networkItem = menuItem->Context;
        PerformNetworkAction(PhMainWndHandle, NETWORK_ACTION_TRACEROUTE, &networkItem->RemoteEndpoint.Address);
        break;
    case ID_TOOLS_WHOIS:
        networkItem = menuItem->Context;
        // TODO: Integrate WHOIS with the GUI.
        PhShellExecute(
            PhMainWndHandle,
            PhaConcatStrings2(L"http://wq.apnic.net/apnic-bin/whois.pl?searchtext=", networkItem->RemoteAddressString)->Buffer,
            NULL
            );
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
