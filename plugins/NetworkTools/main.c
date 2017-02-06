/*
 * Process Hacker Network Tools -
 *   Main program
 *
 * Copyright (C) 2010-2011 wj32
 * Copyright (C) 2013-2016 dmex
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
#include "tracert.h"
#include <commonutil.h>

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION MainMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION NetworkMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION NetworkTreeNewInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION TreeNewMessageCallbackRegistration;
HWND NetworkTreeNewHandle = NULL;

VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    LoadGeoLiteDb();
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
        ShowPingWindow(networkItem);
        break;
    case NETWORK_ACTION_TRACEROUTE:
        ShowTracertWindow(networkItem);
        break;
    case NETWORK_ACTION_WHOIS:
        ShowWhoisWindow(networkItem);
        break;
    case MAINMENU_ACTION_PING:
        {
            PH_IP_ENDPOINT RemoteEndpoint;
            PPH_STRING selectedChoice = NULL;

            while (PhaChoiceDialog(
                menuItem->OwnerWindow,
                L"Ping",
                L"IP address:",
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
                    ShowPingWindowFromAddress(RemoteEndpoint);
                    break;
                }

                if (NT_SUCCESS(RtlIpv6StringToAddress(selectedChoice->Buffer, &terminator, &RemoteEndpoint.Address.In6Addr)))
                {
                    RemoteEndpoint.Address.Type = PH_IPV6_NETWORK_TYPE;
                    ShowPingWindowFromAddress(RemoteEndpoint);
                    break;
                }
            }
        }
        break;
    case MAINMENU_ACTION_TRACERT:
        {
            PH_IP_ENDPOINT RemoteEndpoint;
            PPH_STRING selectedChoice = NULL;

            while (PhaChoiceDialog(
                menuItem->OwnerWindow,
                L"Tracert",
                L"IP address:",
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
                    ShowTracertWindowFromAddress(RemoteEndpoint);
                    break;
                }

                if (NT_SUCCESS(RtlIpv6StringToAddress(selectedChoice->Buffer, &terminator, &RemoteEndpoint.Address.In6Addr)))
                {
                    RemoteEndpoint.Address.Type = PH_IPV6_NETWORK_TYPE;
                    ShowTracertWindowFromAddress(RemoteEndpoint);
                    break;
                }
            }
        }
        break;
    case MAINMENU_ACTION_WHOIS:
        {
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
                    ShowWhoisWindowFromAddress(RemoteEndpoint);
                    break;
                }

                if (NT_SUCCESS(RtlIpv6StringToAddress(selectedChoice->Buffer, &terminator, &RemoteEndpoint.Address.In6Addr)))
                {
                    RemoteEndpoint.Address.Type = PH_IPV6_NETWORK_TYPE;
                    ShowWhoisWindowFromAddress(RemoteEndpoint);
                    break;
                }
            }
        }
        break;
    case MAINMENU_ACTION_GEOIP_UPDATE:
        {
            if (PhGetOwnTokenAttributes().Elevated)
            {
                ShowGeoIPUpdateDialog(PhMainWndHandle);
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

    if (menuInfo->u.MainMenu.SubMenuIndex != PH_MENU_ITEM_LOCATION_TOOLS)
        return;
  
    networkToolsMenu = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"Network Tools", NULL);    
    PhInsertEMenuItem(networkToolsMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MAINMENU_ACTION_GEOIP_UPDATE, L"GeoIP database update...", NULL), -1);
    PhInsertEMenuItem(networkToolsMenu, PhPluginCreateEMenuItem(PluginInstance, PH_EMENU_SEPARATOR, 0, NULL, NULL), -1);
    PhInsertEMenuItem(networkToolsMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MAINMENU_ACTION_PING, L"Ping address...", NULL), -1);
    PhInsertEMenuItem(networkToolsMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MAINMENU_ACTION_TRACERT, L"Traceroute address...", NULL), -1);
    PhInsertEMenuItem(networkToolsMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MAINMENU_ACTION_WHOIS, L"Whois address...", NULL), -1);
    PhInsertEMenuItem(menuInfo->Menu, networkToolsMenu, -1);
}

VOID NTAPI NetworkMenuInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = (PPH_PLUGIN_MENU_INFORMATION)Parameter;
    PPH_NETWORK_ITEM networkItem;
    PPH_EMENU_ITEM whoisMenu;
    PPH_EMENU_ITEM traceMenu;
    PPH_EMENU_ITEM pingMenu;

    if (menuInfo->u.Network.NumberOfNetworkItems == 1)
        networkItem = menuInfo->u.Network.NetworkItems[0];
    else
        networkItem = NULL;

    PhInsertEMenuItem(menuInfo->Menu, PhPluginCreateEMenuItem(PluginInstance, PH_EMENU_SEPARATOR, 0, NULL, NULL), 0);
    PhInsertEMenuItem(menuInfo->Menu, whoisMenu = PhPluginCreateEMenuItem(PluginInstance, 0, NETWORK_ACTION_WHOIS, L"Whois", networkItem), 0);
    PhInsertEMenuItem(menuInfo->Menu, traceMenu = PhPluginCreateEMenuItem(PluginInstance, 0, NETWORK_ACTION_TRACEROUTE, L"Traceroute", networkItem), 0);
    PhInsertEMenuItem(menuInfo->Menu, pingMenu = PhPluginCreateEMenuItem(PluginInstance, 0, NETWORK_ACTION_PING, L"Ping", networkItem), 0);

    whoisMenu->Flags |= PH_EMENU_DISABLED;
    traceMenu->Flags |= PH_EMENU_DISABLED;
    pingMenu->Flags |= PH_EMENU_DISABLED;

    if (networkItem && !PhIsNullIpAddress(&networkItem->RemoteEndpoint.Address))
    { 
        whoisMenu->Flags &= ~PH_EMENU_DISABLED;
        traceMenu->Flags &= ~PH_EMENU_DISABLED;
        pingMenu->Flags &= ~PH_EMENU_DISABLED;     
    }
}

LONG NTAPI NetworkServiceSortFunction(
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ ULONG SubId,
    _In_ PVOID Context
    )
{
    PPH_NETWORK_NODE node1 = Node1;
    PPH_NETWORK_NODE node2 = Node2;
    PNETWORK_EXTENSION extension1 = PhPluginGetObjectExtension(PluginInstance, node1->NetworkItem, EmNetworkItemType);
    PNETWORK_EXTENSION extension2 = PhPluginGetObjectExtension(PluginInstance, node2->NetworkItem, EmNetworkItemType);

    switch (SubId)
    {
    case NETWORK_COLUMN_ID_REMOTE_COUNTRY:
        return PhCompareStringWithNull(extension1->RemoteCountryCode, extension2->RemoteCountryCode, TRUE);
    }

    return 0;
}

VOID NTAPI NetworkTreeNewInitializingCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_INFORMATION info = Parameter;
    PH_TREENEW_COLUMN column;

    *(HWND*)Context = info->TreeNewHandle;

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"Country";
    column.Width = 140;
    column.Alignment = PH_ALIGN_LEFT;
    column.CustomDraw = TRUE; // Owner-draw this column to show country flags
    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, NETWORK_COLUMN_ID_REMOTE_COUNTRY, NULL, NetworkServiceSortFunction);

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"Local Service";
    column.Width = 140;
    column.Alignment = PH_ALIGN_LEFT;
    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, NETWORK_COLUMN_ID_LOCAL_SERVICE, NULL, NetworkServiceSortFunction);

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"Remote Service";
    column.Width = 140;
    column.Alignment = PH_ALIGN_LEFT;
    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, NETWORK_COLUMN_ID_REMOTE_SERVICE, NULL, NetworkServiceSortFunction);
}

VOID NTAPI NetworkItemCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    //PPH_NETWORK_ITEM networkItem = Object;
    PNETWORK_EXTENSION extension = Extension;

    memset(extension, 0, sizeof(NETWORK_EXTENSION));
}

VOID NTAPI NetworkItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    //PPH_NETWORK_ITEM networkItem = Object;
    PNETWORK_EXTENSION extension = Extension;

    PhClearReference(&extension->RemoteCountryCode);
    PhClearReference(&extension->RemoteCountryName);

    if (extension->CountryIcon)
        DestroyIcon(extension->CountryIcon);
}

VOID NTAPI NetworkNodeCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_NETWORK_NODE networkNode = Object;
    PNETWORK_EXTENSION extension = PhPluginGetObjectExtension(PluginInstance, networkNode->NetworkItem, EmNetworkItemType);

    if (!extension->CountryValid)
    {
        PPH_STRING remoteCountryCode;
        PPH_STRING remoteCountryName;

        if (LookupCountryCode(
            networkNode->NetworkItem->RemoteEndpoint.Address, 
            &remoteCountryCode, 
            &remoteCountryName
            ))
        {
            PhSwapReference(&extension->RemoteCountryCode, remoteCountryCode);
            PhSwapReference(&extension->RemoteCountryName, remoteCountryName);
        }

        extension->CountryValid = TRUE;
    }
}

VOID UpdateNetworkNode(
    _In_ NETWORK_COLUMN_ID ColumnID,
    _In_ PPH_NETWORK_NODE Node,
    _In_ PNETWORK_EXTENSION Extension
    )
{
    switch (ColumnID)
    {
    case NETWORK_COLUMN_ID_LOCAL_SERVICE:
        {
            if (!Extension->LocalValid)
            {
                for (ULONG x = 0; x < ARRAYSIZE(ResolvedPortsTable); x++)
                {
                    if (Node->NetworkItem->LocalEndpoint.Port == ResolvedPortsTable[x].Port)
                    {
                        //PhAppendFormatStringBuilder(&stringBuilder, L"%s,", ResolvedPortsTable[x].Name);
                        PhSwapReference(&Extension->LocalServiceName, PhCreateString(ResolvedPortsTable[x].Name));
                        break;
                    }
                }

                Extension->LocalValid = TRUE;
            }
        }
        break;
    case NETWORK_COLUMN_ID_REMOTE_SERVICE:
        {
            if (!Extension->RemoteValid)
            {
                for (ULONG x = 0; x < ARRAYSIZE(ResolvedPortsTable); x++)
                {
                    if (Node->NetworkItem->RemoteEndpoint.Port == ResolvedPortsTable[x].Port)
                    {
                        //PhAppendFormatStringBuilder(&stringBuilder, L"%s,", ResolvedPortsTable[x].Name);
                        PhSwapReference(&Extension->RemoteServiceName, PhCreateString(ResolvedPortsTable[x].Name));
                        break;
                    }
                }

                Extension->RemoteValid = TRUE;
            }
        }
        break;
    }
}

VOID NTAPI TreeNewMessageCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_TREENEW_MESSAGE message = Parameter;

    switch (message->Message)
    {
    case TreeNewGetCellText:
        {
            if (message->TreeNewHandle == NetworkTreeNewHandle)
            {
                PPH_TREENEW_GET_CELL_TEXT getCellText = message->Parameter1;
                PPH_NETWORK_NODE networkNode = (PPH_NETWORK_NODE)getCellText->Node;
                PNETWORK_EXTENSION extension = PhPluginGetObjectExtension(PluginInstance, networkNode->NetworkItem, EmNetworkItemType);
                
                UpdateNetworkNode(message->SubId, networkNode, extension);

                switch (message->SubId)
                {
                case NETWORK_COLUMN_ID_REMOTE_COUNTRY:
                    getCellText->Text = PhGetStringRef(extension->RemoteCountryName);
                    break;
                case NETWORK_COLUMN_ID_LOCAL_SERVICE:
                    getCellText->Text = PhGetStringRef(extension->LocalServiceName);
                    break;
                case NETWORK_COLUMN_ID_REMOTE_SERVICE:
                    getCellText->Text = PhGetStringRef(extension->RemoteServiceName);
                    break;
                }
            }
        }
        break;
    case TreeNewCustomDraw:
        {
            PPH_TREENEW_CUSTOM_DRAW customDraw = message->Parameter1;
            PPH_NETWORK_NODE networkNode = (PPH_NETWORK_NODE)customDraw->Node;
            PNETWORK_EXTENSION extension = PhPluginGetObjectExtension(PluginInstance, networkNode->NetworkItem, EmNetworkItemType);
            HDC hdc = customDraw->Dc;
            RECT rect = customDraw->CellRect;

            // Check if this is the country column
            if (message->SubId != NETWORK_COLUMN_ID_REMOTE_COUNTRY)
                break;

            // Check if there's something to draw
            if (rect.right - rect.left <= 1)
            {
                // nothing to draw
                break;
            }
            
            // Padding
            rect.left += 5;

            // Draw the column data
            if (GeoDbLoaded && extension->RemoteCountryCode && extension->RemoteCountryName)
            {
                if (!extension->CountryIcon)
                {  
                    INT resourceCode;
                    
                    if ((resourceCode = LookupResourceCode(extension->RemoteCountryCode)) != 0)
                    {
                        HBITMAP countryBitmap;

                        if (countryBitmap = LoadImageFromResources(PluginInstance->DllBase, 16, 11, MAKEINTRESOURCE(resourceCode), TRUE))
                        {
                            extension->CountryIcon = CommonBitmapToIcon(countryBitmap, 16, 11);
                        }
                    }
                }

                if (extension->CountryIcon)
                {
                    DrawIconEx(
                        hdc,
                        rect.left,
                        rect.top + ((rect.bottom - rect.top) - 11) / 2,
                        extension->CountryIcon,
                        16,
                        11,
                        0,
                        NULL,
                        DI_NORMAL
                        );

                    rect.left += 16 + 2;
                }

                DrawText(
                    hdc, 
                    extension->RemoteCountryName->Buffer,
                    (INT)extension->RemoteCountryName->Length / 2,
                    &rect,
                    DT_LEFT | DT_VCENTER | DT_SINGLELINE
                    );
            }

            if (GeoDbExpired && !extension->CountryIcon)
            {
                DrawText(hdc, L"Geoip database expired.", -1, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            }

            if (!GeoDbLoaded)
            {
                DrawText(hdc, L"Geoip database not found.", -1, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            }
        }
        break;
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
                { StringSettingType, SETTING_NAME_TRACERT_LIST_COLUMNS, L"" },
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

           PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackNetworkTreeNewInitializing),
                NetworkTreeNewInitializingCallback,
                &NetworkTreeNewHandle,
                &NetworkTreeNewInitializingCallbackRegistration
                );
            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackTreeNewMessage),
                TreeNewMessageCallback,
                NULL,
                &TreeNewMessageCallbackRegistration
                );     

            PhPluginSetObjectExtension(
                PluginInstance, 
                EmNetworkItemType, 
                sizeof(NETWORK_EXTENSION),
                NetworkItemCreateCallback,
                NetworkItemDeleteCallback
                );
            PhPluginSetObjectExtension(
                PluginInstance,
                EmNetworkNodeType,
                sizeof(NETWORK_EXTENSION),
                NetworkNodeCreateCallback,
                NULL
                );

            PhAddSettings(settings, ARRAYSIZE(settings));
        }
        break;
    }

    return TRUE;
}