/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2011
 *     dmex    2013-2024
 *
 */

#include "nettools.h"
#include <networktoolsintf.h>

#include <trace.h>

PPH_PLUGIN PluginInstance;
PH_CALLBACK_REGISTRATION PluginLoadCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginShowOptionsCallbackRegistration;
PH_CALLBACK_REGISTRATION PluginMenuItemCallbackRegistration;
PH_CALLBACK_REGISTRATION MainMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION NetworkMenuInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION NetworkTreeNewInitializingCallbackRegistration;
PH_CALLBACK_REGISTRATION TreeNewMessageCallbackRegistration;
PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;

HWND NetworkTreeNewHandle = NULL;
BOOLEAN NetworkExtensionEnabled = FALSE;
RTL_STATIC_LIST_HEAD(NetworkExtensionListHead);
PH_QUEUED_LOCK NetworkExtensionListLock = PH_QUEUED_LOCK_INIT;

NETWORKTOOLS_INTERFACE PluginInterface =
{
    NETWORKTOOLS_INTERFACE_VERSION,
    LookupCountryCode,
    LookupCountryIcon,
    LookupPortServiceName,
    DrawCountryIcon,
    ShowPingWindowFromAddress,
    ShowTracertWindowFromAddress,
    ShowWhoisWindowFromAddress
};

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI LoadCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    GeoLiteDatabaseType = PhGetIntegerSetting(SETTING_NAME_GEOLITE_DB_TYPE);
    NetworkExtensionEnabled = !!PhGetIntegerSetting(SETTING_NAME_EXTENDED_TCP_STATS);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI ShowOptionsCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PPH_PLUGIN_OPTIONS_POINTERS optionsEntry = (PPH_PLUGIN_OPTIONS_POINTERS)Parameter;

    optionsEntry->CreateSection(
        L"NetworkTools",
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_OPTIONS),
        OptionsDlgProc,
        NULL
        );
}

_Success_(return)
static BOOLEAN ParseNetworkAddress(
    _In_ PCWSTR AddressString,
    _Out_ PPH_IP_ENDPOINT RemoteEndpoint
    )
{
    NET_ADDRESS_INFO addressInfo;

    memset(&addressInfo, 0, sizeof(NET_ADDRESS_INFO));

    if (NT_SUCCESS(PhIpv4StringToAddress(
        AddressString,
        FALSE,
        &addressInfo.Ipv4Address.sin_addr,
        &addressInfo.Ipv4Address.sin_port
        )))
    {
        // ParseNetworkString doesn't support legacy formats of IPv4 addressing, so we'll first
        // try parsing the address with RtlIpv4StringToAddressEx since it does support these formats (dmex)
        RemoteEndpoint->Address.InAddr.s_addr = addressInfo.Ipv4Address.sin_addr.s_addr;
        RemoteEndpoint->Port = _byteswap_ushort(addressInfo.Ipv4Address.sin_port);
        RemoteEndpoint->Address.Type = PH_NETWORK_TYPE_IPV4;
        return TRUE;
    }

    if (ParseNetworkString(
        AddressString,
        NET_STRING_ANY_ADDRESS | NET_STRING_ANY_SERVICE,
        &addressInfo,
        NULL,
        NULL
        ) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    if (addressInfo.Format == NET_ADDRESS_DNS_NAME)
    {
        BOOLEAN success = FALSE;
        PADDRINFOT result;
        WSADATA wsaData;

        if (WSAStartup(WINSOCK_VERSION, &wsaData))
            return FALSE;

        if (GetAddrInfo(addressInfo.NamedAddress.Address, addressInfo.NamedAddress.Port, NULL, &result) == ERROR_SUCCESS)
        {
            for (PADDRINFOT i = result; i; i = i->ai_next)
            {
                if (i->ai_family == AF_INET)
                {
                    RemoteEndpoint->Address.InAddr.s_addr = ((PSOCKADDR_IN)i->ai_addr)->sin_addr.s_addr;
                    RemoteEndpoint->Port = _byteswap_ushort(((PSOCKADDR_IN)i->ai_addr)->sin_port);
                    RemoteEndpoint->Address.Type = PH_NETWORK_TYPE_IPV4;
                    success = TRUE;
                    break;
                }
                else if (i->ai_family == AF_INET6)
                {
                    memcpy_s(
                        RemoteEndpoint->Address.In6Addr.s6_addr,
                        sizeof(RemoteEndpoint->Address.In6Addr.s6_addr),
                        ((PSOCKADDR_IN6)i->ai_addr)->sin6_addr.s6_addr,
                        sizeof(((PSOCKADDR_IN6)i->ai_addr)->sin6_addr.s6_addr)
                        );
                    RemoteEndpoint->Port = _byteswap_ushort(((PSOCKADDR_IN6)i->ai_addr)->sin6_port);
                    RemoteEndpoint->Address.Type = PH_NETWORK_TYPE_IPV6;
                    success = TRUE;
                    break;
                }
            }

            FreeAddrInfo(result);
        }

        WSACleanup();

        if (success)
            return TRUE;
    }

    if (addressInfo.Format == NET_ADDRESS_IPV4)
    {
        RemoteEndpoint->Address.InAddr.s_addr = addressInfo.Ipv4Address.sin_addr.s_addr;
        RemoteEndpoint->Port = _byteswap_ushort(addressInfo.Ipv4Address.sin_port);
        RemoteEndpoint->Address.Type = PH_NETWORK_TYPE_IPV4;
        return TRUE;
    }

    if (addressInfo.Format == NET_ADDRESS_IPV6)
    {
        memcpy_s(
            RemoteEndpoint->Address.In6Addr.s6_addr,
            sizeof(RemoteEndpoint->Address.In6Addr.s6_addr),
            addressInfo.Ipv6Address.sin6_addr.s6_addr,
            sizeof(addressInfo.Ipv6Address.sin6_addr.s6_addr)
            );
        RemoteEndpoint->Port = _byteswap_ushort(addressInfo.Ipv6Address.sin6_port);
        RemoteEndpoint->Address.Type = PH_NETWORK_TYPE_IPV6;
        return TRUE;
    }

    return FALSE;
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI MenuItemCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_ITEM menuItem = (PPH_PLUGIN_MENU_ITEM)Parameter;

    switch (menuItem->Id)
    {
    case NETWORK_ACTION_PING:
        ShowPingWindow(menuItem->OwnerWindow, (PPH_NETWORK_ITEM)menuItem->Context);
        break;
    case NETWORK_ACTION_TRACEROUTE:
        ShowTracertWindow(menuItem->OwnerWindow, (PPH_NETWORK_ITEM)menuItem->Context);
        break;
    case NETWORK_ACTION_WHOIS:
        ShowWhoisWindow(menuItem->OwnerWindow, (PPH_NETWORK_ITEM)menuItem->Context);
        break;
    case MAINMENU_ACTION_PING:
        {
            PPH_STRING selectedChoice = NULL;
            PH_IP_ENDPOINT remoteEndpoint = { 0 };

            while (PhChoiceDialog(
                menuItem->OwnerWindow,
                L"Ping the Hostname or IP Address",
                L"You can use an IPv4 or IPv6 address as the target or a fully qualified domain name (FQDN) or a website URL as the target.",
                NULL,
                0,
                NULL,
                PH_CHOICE_DIALOG_USER_CHOICE,
                &selectedChoice,
                NULL,
                SETTING_NAME_ADDRESS_HISTORY
                ))
            {
                if (ParseNetworkAddress(selectedChoice->Buffer, &remoteEndpoint))
                {
                    ShowPingWindowFromAddress(menuItem->OwnerWindow, remoteEndpoint);
                    break;
                }
            }
        }
        break;
    case MAINMENU_ACTION_TRACERT:
        {
            PPH_STRING selectedChoice = NULL;
            PH_IP_ENDPOINT remoteEndpoint = { 0 };

            while (PhChoiceDialog(
                menuItem->OwnerWindow,
                L"Tracert the Hostname or IP Address",
                L"You can use an IPv4 or IPv6 address as the target or a fully qualified domain name (FQDN) or a website URL as the target.",
                NULL,
                0,
                NULL,
                PH_CHOICE_DIALOG_USER_CHOICE,
                &selectedChoice,
                NULL,
                SETTING_NAME_ADDRESS_HISTORY
                ))
            {
                if (ParseNetworkAddress(selectedChoice->Buffer, &remoteEndpoint))
                {
                    ShowTracertWindowFromAddress(menuItem->OwnerWindow, remoteEndpoint);
                    break;
                }
            }
        }
        break;
    case MAINMENU_ACTION_WHOIS:
        {
            PPH_STRING selectedChoice = NULL;
            PH_IP_ENDPOINT remoteEndpoint = { 0 };

            while (PhChoiceDialog(
                menuItem->OwnerWindow,
                L"Whois the Hostname or IP Address",
                L"You can use an IPv4 or IPv6 address as the target or a fully qualified domain name (FQDN) or a website URL as the target.",
                NULL,
                0,
                NULL,
                PH_CHOICE_DIALOG_USER_CHOICE,
                &selectedChoice,
                NULL,
                SETTING_NAME_ADDRESS_HISTORY
                ))
            {
                if (ParseNetworkAddress(selectedChoice->Buffer, &remoteEndpoint))
                {
                    ShowWhoisWindowFromAddress(menuItem->OwnerWindow, remoteEndpoint);
                    break;
                }
            }
        }
        break;
    case MAINMENU_ACTION_GEOIP_UPDATE:
        ShowGeoLiteUpdateDialog(menuItem->OwnerWindow);
        break;
    }
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI MainMenuInitializingCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPH_PLUGIN_MENU_INFORMATION menuInfo = Parameter;
    PPH_EMENU_ITEM networkToolsMenu;

    if (menuInfo->u.MainMenu.SubMenuIndex != PH_MENU_ITEM_LOCATION_TOOLS)
        return;

    networkToolsMenu = PhPluginCreateEMenuItem(PluginInstance, 0, 0, L"&Network Tools", NULL);
    PhInsertEMenuItem(networkToolsMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MAINMENU_ACTION_GEOIP_UPDATE, L"&GeoLite database update...", NULL), ULONG_MAX);
    PhInsertEMenuItem(networkToolsMenu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(networkToolsMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MAINMENU_ACTION_PING, L"&Ping address...", NULL), ULONG_MAX);
    PhInsertEMenuItem(networkToolsMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MAINMENU_ACTION_TRACERT, L"&Traceroute address...", NULL), ULONG_MAX);
    PhInsertEMenuItem(networkToolsMenu, PhPluginCreateEMenuItem(PluginInstance, 0, MAINMENU_ACTION_WHOIS, L"&Whois address...", NULL), ULONG_MAX);
    PhInsertEMenuItem(menuInfo->Menu, networkToolsMenu, ULONG_MAX);
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID NTAPI NetworkMenuInitializingCallback(
    _In_ PVOID Parameter,
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

    PhInsertEMenuItem(menuInfo->Menu, pingMenu = PhPluginCreateEMenuItem(PluginInstance, 0, NETWORK_ACTION_PING, L"&Ping", networkItem), 0);
    PhInsertEMenuItem(menuInfo->Menu, traceMenu = PhPluginCreateEMenuItem(PluginInstance, 0, NETWORK_ACTION_TRACEROUTE, L"&Traceroute", networkItem), 1);
    PhInsertEMenuItem(menuInfo->Menu, whoisMenu = PhPluginCreateEMenuItem(PluginInstance, 0, NETWORK_ACTION_WHOIS, L"&Whois", networkItem), 2);
    PhInsertEMenuItem(menuInfo->Menu, PhCreateEMenuSeparator(), 3);

    if (networkItem)
    {
        if (PhIsNullIpAddress(&networkItem->RemoteEndpoint.Address))
        {
            PhSetDisabledEMenuItem(whoisMenu);
            PhSetDisabledEMenuItem(traceMenu);
            PhSetDisabledEMenuItem(pingMenu);
        }
        else if (networkItem->RemoteEndpoint.Address.Type == PH_NETWORK_TYPE_IPV4)
        {
            if (
                IN4_IS_ADDR_UNSPECIFIED(&networkItem->RemoteEndpoint.Address.InAddr) ||
                IN4_IS_ADDR_LOOPBACK(&networkItem->RemoteEndpoint.Address.InAddr)
                )
            {
                PhSetDisabledEMenuItem(whoisMenu);
                PhSetDisabledEMenuItem(traceMenu);
                PhSetDisabledEMenuItem(pingMenu);
            }

            if (IN4_IS_ADDR_RFC1918(&networkItem->RemoteEndpoint.Address.InAddr))
            {
                PhSetDisabledEMenuItem(whoisMenu);
            }
        }
        else  if (networkItem->RemoteEndpoint.Address.Type == PH_NETWORK_TYPE_IPV6)
        {
            if (
                IN6_IS_ADDR_UNSPECIFIED(&networkItem->RemoteEndpoint.Address.In6Addr) ||
                IN6_IS_ADDR_LOOPBACK(&networkItem->RemoteEndpoint.Address.In6Addr)
                )
            {
                PhSetDisabledEMenuItem(whoisMenu);
                PhSetDisabledEMenuItem(traceMenu);
                PhSetDisabledEMenuItem(pingMenu);
            }
        }
    }
    else
    {
        PhSetDisabledEMenuItem(whoisMenu);
        PhSetDisabledEMenuItem(traceMenu);
        PhSetDisabledEMenuItem(pingMenu);
    }
}

PNETWORK_EXTENSION GetNetworkItemExtension(
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    return PhPluginGetObjectExtension(PluginInstance, NetworkItem, EmNetworkItemType);
}

LONG NTAPI NetworkServiceSortFunction(
    _In_ PVOID Node1,
    _In_ PVOID Node2,
    _In_ ULONG SubId,
    _In_ PH_SORT_ORDER SortOrder,
    _In_ PVOID Context
    )
{
    PPH_NETWORK_NODE node1 = Node1;
    PPH_NETWORK_NODE node2 = Node2;
    PNETWORK_EXTENSION extension1 = GetNetworkItemExtension(node1->NetworkItem);
    PNETWORK_EXTENSION extension2 = GetNetworkItemExtension(node2->NetworkItem);

    switch (SubId)
    {
    case NETWORK_COLUMN_ID_REMOTE_COUNTRY:
        return PhCompareStringWithNullSortOrder(extension1->RemoteCountryName, extension2->RemoteCountryName, SortOrder, TRUE);
    case NETWORK_COLUMN_ID_LOCAL_SERVICE:
        return PhCompareStringRefWithNullSortOrder(extension1->LocalServiceName, extension2->LocalServiceName, SortOrder, TRUE);
    case NETWORK_COLUMN_ID_REMOTE_SERVICE:
        return PhCompareStringRefWithNullSortOrder(extension1->RemoteServiceName, extension2->RemoteServiceName, SortOrder, TRUE);
    case NETWORK_COLUMN_ID_BYTES_IN:
        return uint64cmp(extension1->NumberOfBytesIn, extension2->NumberOfBytesIn);
    case NETWORK_COLUMN_ID_BYTES_OUT:
        return uint64cmp(extension1->NumberOfBytesOut, extension2->NumberOfBytesOut);
    case NETWORK_COLUMN_ID_PACKETLOSS:
        return uint64cmp(extension1->NumberOfLostPackets, extension2->NumberOfLostPackets);
    case NETWORK_COLUMN_ID_JITTER:
        return uintcmp(extension1->VarianceRtt, extension2->VarianceRtt);
    case NETWORK_COLUMN_ID_LATENCY:
        return uintcmp(extension1->SampleRtt, extension2->SampleRtt);
    }

    return 0;
}

VOID NTAPI NetworkTreeNewInitializingCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
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
    column.Text = L"Local service";
    column.Width = 140;
    column.Alignment = PH_ALIGN_LEFT;
    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, NETWORK_COLUMN_ID_LOCAL_SERVICE, NULL, NetworkServiceSortFunction);

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"Remote service";
    column.Width = 140;
    column.Alignment = PH_ALIGN_LEFT;
    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, NETWORK_COLUMN_ID_REMOTE_SERVICE, NULL, NetworkServiceSortFunction);

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"Total bytes in";
    column.Width = 80;
    column.Alignment = PH_ALIGN_LEFT;
    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, NETWORK_COLUMN_ID_BYTES_IN, NULL, NetworkServiceSortFunction);

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"Total bytes out";
    column.Width = 80;
    column.Alignment = PH_ALIGN_LEFT;
    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, NETWORK_COLUMN_ID_BYTES_OUT, NULL, NetworkServiceSortFunction);

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"Packet loss";
    column.Width = 80;
    column.Alignment = PH_ALIGN_LEFT;
    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, NETWORK_COLUMN_ID_PACKETLOSS, NULL, NetworkServiceSortFunction);

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"Jitter (ms)";
    column.Width = 80;
    column.Alignment = PH_ALIGN_LEFT;
    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, NETWORK_COLUMN_ID_JITTER, NULL, NetworkServiceSortFunction);

    memset(&column, 0, sizeof(PH_TREENEW_COLUMN));
    column.Text = L"Latency (ms)";
    column.Width = 80;
    column.Alignment = PH_ALIGN_LEFT;
    PhPluginAddTreeNewColumn(PluginInstance, info->CmData, &column, NETWORK_COLUMN_ID_LATENCY, NULL, NetworkServiceSortFunction);
}

VOID NTAPI NetworkItemCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_NETWORK_ITEM networkItem = Object;
    PNETWORK_EXTENSION extension = Extension;

    memset(extension, 0, sizeof(NETWORK_EXTENSION));
    extension->NetworkItem = networkItem;
    extension->CountryIconIndex = INT_ERROR;

    if (NetworkExtensionEnabled)
    {
        PhAcquireQueuedLockExclusive(&NetworkExtensionListLock);
        InsertTailList(&NetworkExtensionListHead, &extension->ListEntry);
        PhReleaseQueuedLockExclusive(&NetworkExtensionListLock);
    }
}

VOID NTAPI NetworkItemDeleteCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    //PPH_NETWORK_ITEM networkItem = Object;
    PNETWORK_EXTENSION extension = Extension;

    if (NetworkExtensionEnabled)
    {
        PhAcquireQueuedLockExclusive(&NetworkExtensionListLock);
        RemoveEntryList(&extension->ListEntry);
        PhReleaseQueuedLockExclusive(&NetworkExtensionListLock);
    }

    if (extension->RemoteCountryName)
        PhDereferenceObject(extension->RemoteCountryName);
    if (extension->BytesIn)
        PhDereferenceObject(extension->BytesIn);
    if (extension->BytesOut)
        PhDereferenceObject(extension->BytesOut);
    if (extension->LossText)
        PhDereferenceObject(extension->LossText);
    if (extension->JitterText)
        PhDereferenceObject(extension->JitterText);
    if (extension->LatencyText)
        PhDereferenceObject(extension->LatencyText);
}

VOID PhpNetworkItemToRow(
    _Out_ PMIB_TCPROW Row,
     _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    Row->State = NetworkItem->State;
    Row->dwLocalAddr = NetworkItem->LocalEndpoint.Address.InAddr.s_addr;
    Row->dwLocalPort = _byteswap_ushort((USHORT)NetworkItem->LocalEndpoint.Port);
    Row->dwRemoteAddr = NetworkItem->RemoteEndpoint.Address.InAddr.s_addr;
    Row->dwRemotePort = _byteswap_ushort((USHORT)NetworkItem->RemoteEndpoint.Port);
}

VOID PhpNetworkItemToRow6(
    _Out_ PMIB_TCP6ROW Row,
    _In_ PPH_NETWORK_ITEM NetworkItem
    )
{
    Row->State = NetworkItem->State;
    memcpy_s(Row->LocalAddr.s6_addr, sizeof(Row->LocalAddr.s6_addr), NetworkItem->LocalEndpoint.Address.Ipv6, sizeof(NetworkItem->LocalEndpoint.Address.Ipv6));
    Row->dwLocalScopeId = NetworkItem->LocalScopeId;
    Row->dwLocalPort = _byteswap_ushort((USHORT)NetworkItem->LocalEndpoint.Port);
    memcpy_s(Row->RemoteAddr.s6_addr, sizeof(Row->RemoteAddr.s6_addr), NetworkItem->RemoteEndpoint.Address.Ipv6, sizeof(NetworkItem->RemoteEndpoint.Address.Ipv6));
    Row->dwRemoteScopeId = NetworkItem->RemoteScopeId;
    Row->dwRemotePort = _byteswap_ushort((USHORT)NetworkItem->RemoteEndpoint.Port);
}

VOID NTAPI NetworkNodeCreateCallback(
    _In_ PVOID Object,
    _In_ PH_EM_OBJECT_TYPE ObjectType,
    _In_ PVOID Extension
    )
{
    PPH_NETWORK_NODE networkNode = Object;
    PNETWORK_EXTENSION extension = GetNetworkItemExtension(networkNode->NetworkItem);

    if (!extension->CountryValid)
    {
        ULONG remoteCountryCode;
        PPH_STRING remoteCountryName;

        if (LookupCountryCode(
            networkNode->NetworkItem->RemoteEndpoint.Address,
            &remoteCountryCode,
            &remoteCountryName
            ))
        {
            PhMoveReference(&extension->RemoteCountryName, remoteCountryName);
            extension->CountryIconIndex = LookupCountryIcon(remoteCountryCode);
        }

        extension->CountryValid = TRUE;
    }

    if (!extension->StatsEnabled)
    {
        if (NetworkExtensionEnabled)
        {
            if (networkNode->NetworkItem->ProtocolType == PH_NETWORK_PROTOCOL_TCP4)
            {
                MIB_TCPROW tcpRow;
                TCP_ESTATS_DATA_RW_v0 dataRw;
                TCP_ESTATS_PATH_RW_v0 pathRw;

                dataRw.EnableCollection = TRUE;
                pathRw.EnableCollection = TRUE;

                PhpNetworkItemToRow(&tcpRow, networkNode->NetworkItem);

                SetPerTcpConnectionEStats(&tcpRow, TcpConnectionEstatsData, (PUCHAR)&dataRw, 0, sizeof(TCP_ESTATS_DATA_RW_v0), 0);
                SetPerTcpConnectionEStats(&tcpRow, TcpConnectionEstatsPath, (PUCHAR)&pathRw, 0, sizeof(TCP_ESTATS_PATH_RW_v0), 0);
            }
            else if (networkNode->NetworkItem->ProtocolType == PH_NETWORK_PROTOCOL_TCP6)
            {
                MIB_TCP6ROW tcp6Row;
                TCP_ESTATS_DATA_RW_v0 dataRw;
                TCP_ESTATS_PATH_RW_v0 pathRw;

                dataRw.EnableCollection = TRUE;
                pathRw.EnableCollection = TRUE;

                PhpNetworkItemToRow6(&tcp6Row, networkNode->NetworkItem);

                SetPerTcp6ConnectionEStats(&tcp6Row, TcpConnectionEstatsData, (PUCHAR)&dataRw, 0, sizeof(TCP_ESTATS_DATA_RW_v0), 0);
                SetPerTcp6ConnectionEStats(&tcp6Row, TcpConnectionEstatsPath, (PUCHAR)&pathRw, 0, sizeof(TCP_ESTATS_PATH_RW_v0), 0);
            }
        }

        extension->StatsEnabled = TRUE;
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
                PPH_STRINGREF localServiceName;

                if (LookupPortServiceName(Node->NetworkItem->LocalEndpoint.Port, &localServiceName))
                {
                    Extension->LocalServiceName = localServiceName;
                }

                Extension->LocalValid = TRUE;
            }
        }
        break;
    case NETWORK_COLUMN_ID_REMOTE_SERVICE:
        {
            if (!Extension->RemoteValid)
            {
                PPH_STRINGREF remoteServiceName;

                if (LookupPortServiceName(Node->NetworkItem->RemoteEndpoint.Port, &remoteServiceName))
                {
                    Extension->RemoteServiceName = remoteServiceName;
                }

                Extension->RemoteValid = TRUE;
            }
        }
        break;
    case NETWORK_COLUMN_ID_BYTES_IN:
        {
            if (Extension->NumberOfBytesIn)
                PhMoveReference(&Extension->BytesIn, PhFormatSize(Extension->NumberOfBytesIn, ULONG_MAX));

            if (!NetworkExtensionEnabled && !Extension->BytesIn && PhGetOwnTokenAttributes().Elevated)
                PhMoveReference(&Extension->BytesIn, PhCreateString(L"Extended TCP statistics are disabled"));
        }
        break;
    case NETWORK_COLUMN_ID_BYTES_OUT:
        {
            if (Extension->NumberOfBytesOut)
                PhMoveReference(&Extension->BytesOut, PhFormatSize(Extension->NumberOfBytesOut, ULONG_MAX));

            if (!NetworkExtensionEnabled && !Extension->BytesOut && PhGetOwnTokenAttributes().Elevated)
                PhMoveReference(&Extension->BytesOut, PhCreateString(L"Extended TCP statistics are disabled"));
        }
        break;
    case NETWORK_COLUMN_ID_PACKETLOSS:
        {
            if (Extension->NumberOfLostPackets)
                PhMoveReference(&Extension->LossText, PhFormatUInt64(Extension->NumberOfLostPackets, TRUE));

            if (!NetworkExtensionEnabled && !Extension->LossText && PhGetOwnTokenAttributes().Elevated)
                PhMoveReference(&Extension->LossText, PhCreateString(L"Extended TCP statistics are disabled"));
        }
        break;
    case NETWORK_COLUMN_ID_JITTER:
        {
            if (Extension->VarianceRtt)
                PhMoveReference(&Extension->JitterText, PhFormatUInt64(Extension->VarianceRtt, TRUE));

            if (!NetworkExtensionEnabled && !Extension->JitterText && PhGetOwnTokenAttributes().Elevated)
                PhMoveReference(&Extension->JitterText, PhCreateString(L"Extended TCP statistics are disabled"));
        }
        break;
    case NETWORK_COLUMN_ID_LATENCY:
        {
            if (Extension->SampleRtt)
                PhMoveReference(&Extension->LatencyText, PhFormatUInt64(Extension->SampleRtt, TRUE));

            if (!NetworkExtensionEnabled && !Extension->LatencyText && PhGetOwnTokenAttributes().Elevated)
                PhMoveReference(&Extension->LatencyText, PhCreateString(L"Extended TCP statistics are disabled"));
        }
        break;
    }
}

VOID NTAPI TreeNewMessageCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
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
                PNETWORK_EXTENSION extension = GetNetworkItemExtension(networkNode->NetworkItem);

                UpdateNetworkNode(message->SubId, networkNode, extension);

                switch (message->SubId)
                {
                case NETWORK_COLUMN_ID_REMOTE_COUNTRY:
                    getCellText->Text = PhGetStringRef(extension->RemoteCountryName);
                    break;
                case NETWORK_COLUMN_ID_LOCAL_SERVICE:
                    {
                        if (extension->LocalServiceName && extension->LocalServiceName->Length)
                        {
                            getCellText->Text.Buffer = extension->LocalServiceName->Buffer;
                            getCellText->Text.Length = extension->LocalServiceName->Length;
                        }
                        else
                        {
                            PhInitializeEmptyStringRef(&getCellText->Text);
                        }
                    }
                    break;
                case NETWORK_COLUMN_ID_REMOTE_SERVICE:
                    {
                        if (extension->RemoteServiceName && extension->RemoteServiceName->Length)
                        {
                            getCellText->Text.Buffer = extension->RemoteServiceName->Buffer;
                            getCellText->Text.Length = extension->RemoteServiceName->Length;
                        }
                        else
                        {
                            PhInitializeEmptyStringRef(&getCellText->Text);
                        }
                    }
                    break;
                case NETWORK_COLUMN_ID_BYTES_IN:
                    getCellText->Text = PhGetStringRef(extension->BytesIn);
                    break;
                case NETWORK_COLUMN_ID_BYTES_OUT:
                    getCellText->Text = PhGetStringRef(extension->BytesOut);
                    break;
                case NETWORK_COLUMN_ID_PACKETLOSS:
                    getCellText->Text = PhGetStringRef(extension->LossText);
                    break;
                case NETWORK_COLUMN_ID_JITTER:
                    getCellText->Text = PhGetStringRef(extension->JitterText);
                    break;
                case NETWORK_COLUMN_ID_LATENCY:
                    getCellText->Text = PhGetStringRef(extension->LatencyText);
                    break;
                }

                getCellText->Flags = TN_CACHE;
            }
        }
        break;
    case TreeNewCustomDraw:
        {
            PPH_TREENEW_CUSTOM_DRAW customDraw = message->Parameter1;
            PPH_NETWORK_NODE networkNode = (PPH_NETWORK_NODE)customDraw->Node;
            PNETWORK_EXTENSION extension = GetNetworkItemExtension(networkNode->NetworkItem);
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
            if (extension->RemoteCountryName)
            {
                if (extension->CountryIconIndex != INT_ERROR)
                {
                    DrawCountryIcon(hdc, rect, extension->CountryIconIndex);
                    rect.left += 16 + 2;
                }

                DrawText(
                    hdc,
                    extension->RemoteCountryName->Buffer,
                    (INT)extension->RemoteCountryName->Length / sizeof(WCHAR),
                    &rect,
                    DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS | DT_SINGLELINE
                    );
            }
            else if (!GeoDbInitialized)
            {
                DrawText(hdc, L"Geoip database not found.", -1, &rect, DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS | DT_SINGLELINE);
            }
        }
        break;
    }
}

BOOLEAN NetworkToolsGeoIpFlushCache(
    VOID
    )
{
    static ULONG64 lastTickCount = 0;
    ULONG64 tickCount = NtGetTickCount64();

    if (lastTickCount == 0)
        lastTickCount = tickCount;

    if (tickCount - lastTickCount >= 600 * 1000)
    {
        lastTickCount = tickCount;
        NetworkToolsGeoDbFlushCache();
        return TRUE;
    }

    return FALSE;
}

_Function_class_(PH_CALLBACK_FUNCTION)
VOID ProcessesUpdatedCallback(
    _In_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PLIST_ENTRY listEntry;

    if (!NetworkExtensionEnabled)
        return;

    if (PtrToUlong(Parameter) < 3)
        return;

    NetworkToolsGeoIpFlushCache();

    for (
        listEntry = NetworkExtensionListHead.Flink;
        listEntry != &NetworkExtensionListHead;
        listEntry = listEntry->Flink
        )
    {
        PNETWORK_EXTENSION extension = CONTAINING_RECORD(listEntry, NETWORK_EXTENSION, ListEntry);

        if (!extension || !extension->StatsEnabled)
            continue;

        if (extension->NetworkItem->ProtocolType == PH_NETWORK_PROTOCOL_TCP4)
        {
            MIB_TCPROW tcpRow;
            TCP_ESTATS_DATA_RW_v0 dataRw;
            TCP_ESTATS_PATH_RW_v0 pathRw;
            TCP_ESTATS_DATA_ROD_v0 dataRod;
            TCP_ESTATS_PATH_ROD_v0 pathRod;

            PhpNetworkItemToRow(&tcpRow, extension->NetworkItem);

            if (GetPerTcpConnectionEStats(
                &tcpRow,
                TcpConnectionEstatsData,
                (PUCHAR)&dataRw,
                0,
                sizeof(TCP_ESTATS_DATA_RW_v0),
                NULL,
                0,
                0,
                (PUCHAR)&dataRod,
                0,
                sizeof(TCP_ESTATS_DATA_ROD_v0)
                ) == ERROR_SUCCESS)
            {
                if (dataRw.EnableCollection)
                {
                    extension->NumberOfBytesIn = dataRod.DataBytesIn;
                    extension->NumberOfBytesOut = dataRod.DataBytesOut;
                }
            }

            if (GetPerTcpConnectionEStats(
                &tcpRow,
                TcpConnectionEstatsPath,
                (PUCHAR)&pathRw,
                0,
                sizeof(TCP_ESTATS_PATH_RW_v0),
                NULL,
                0,
                0,
                (PUCHAR)&pathRod,
                0,
                sizeof(TCP_ESTATS_PATH_ROD_v0)
                ) == ERROR_SUCCESS)
            {
                if (pathRw.EnableCollection)
                {
                    extension->NumberOfLostPackets = UInt32Add32To64(pathRod.FastRetran, pathRod.PktsRetrans);
                    extension->SampleRtt = pathRod.SampleRtt;
                    extension->VarianceRtt = pathRod.RttVar;

                    if (extension->SampleRtt == ULONG_MAX)
                        extension->SampleRtt = 0;
                    if (extension->VarianceRtt == ULONG_MAX)
                        extension->VarianceRtt = 0;
                }
            }
        }
        else if (extension->NetworkItem->ProtocolType == PH_NETWORK_PROTOCOL_TCP6)
        {
            MIB_TCP6ROW tcp6Row;
            TCP_ESTATS_DATA_RW_v0 dataRw;
            TCP_ESTATS_PATH_RW_v0 pathRw;
            TCP_ESTATS_DATA_ROD_v0 dataRod;
            TCP_ESTATS_PATH_ROD_v0 pathRod;

            PhpNetworkItemToRow6(&tcp6Row, extension->NetworkItem);

            if (GetPerTcp6ConnectionEStats(
                &tcp6Row,
                TcpConnectionEstatsData,
                (PUCHAR)&dataRw,
                0,
                sizeof(TCP_ESTATS_DATA_RW_v0),
                NULL,
                0,
                0,
                (PUCHAR)&dataRod,
                0,
                sizeof(TCP_ESTATS_DATA_ROD_v0)
                ) == ERROR_SUCCESS)
            {
                if (dataRw.EnableCollection)
                {
                    extension->NumberOfBytesIn = dataRod.DataBytesIn;
                    extension->NumberOfBytesOut = dataRod.DataBytesOut;
                }
            }

            if (GetPerTcp6ConnectionEStats(
                &tcp6Row,
                TcpConnectionEstatsPath,
                (PUCHAR)&pathRw,
                0,
                sizeof(TCP_ESTATS_PATH_RW_v0),
                NULL,
                0,
                0,
                (PUCHAR)&pathRod,
                0,
                sizeof(TCP_ESTATS_PATH_ROD_v0)
                ) == ERROR_SUCCESS)
            {
                if (pathRw.EnableCollection)
                {
                    extension->NumberOfLostPackets = UInt32Add32To64(pathRod.FastRetran, pathRod.PktsRetrans);
                    extension->SampleRtt = pathRod.SampleRtt;
                    extension->VarianceRtt = pathRod.RttVar;

                    if (extension->SampleRtt == ULONG_MAX)
                        extension->SampleRtt = 0;
                    if (extension->VarianceRtt == ULONG_MAX)
                        extension->VarianceRtt = 0;
                }
            }
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
                { StringSettingType, SETTING_NAME_ADDRESS_HISTORY, L"" },
                { IntegerPairSettingType, SETTING_NAME_PING_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_PING_WINDOW_SIZE, L"@96|420,250" },
                { IntegerSettingType, SETTING_NAME_PING_MINIMUM_SCALING, L"c8" }, // 200ms minimum scaling
                { IntegerSettingType, SETTING_NAME_PING_TIMEOUT, L"3e8" }, // 1000 timeout
                { IntegerSettingType, SETTING_NAME_PING_SIZE, L"20" }, // 32 byte packet
                { IntegerPairSettingType, SETTING_NAME_TRACERT_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_TRACERT_WINDOW_SIZE, L"@96|850,490" },
                { StringSettingType, SETTING_NAME_TRACERT_TREE_LIST_COLUMNS, L"" },
                { IntegerPairSettingType, SETTING_NAME_TRACERT_TREE_LIST_SORT, L"0,1" },
                { IntegerSettingType, SETTING_NAME_TRACERT_MAX_HOPS, L"14" },
                { IntegerPairSettingType, SETTING_NAME_WHOIS_WINDOW_POSITION, L"0,0" },
                { ScalableIntegerPairSettingType, SETTING_NAME_WHOIS_WINDOW_SIZE, L"@96|600,365" },
                { IntegerSettingType, SETTING_NAME_WHOIS_IPV6_SUPPORT, L"0" },
                { IntegerSettingType, SETTING_NAME_EXTENDED_TCP_STATS, L"0" },
                { StringSettingType, SETTING_NAME_GEOLITE_API_KEY, L"" },
                { StringSettingType, SETTING_NAME_GEOLITE_API_ID, L"" },
                { IntegerSettingType, SETTING_NAME_GEOLITE_DB_TYPE, L"0" },
            };

            WPP_INIT_TRACING(PLUGIN_NAME);

            PluginInstance = PhRegisterPlugin(PLUGIN_NAME, Instance, &info);

            if (!PluginInstance)
                return FALSE;

            info->DisplayName = L"Network Tools";
            info->Description = L"Provides ping, traceroute and whois for network connections.";
            info->Interface = &PluginInterface;

            PhRegisterCallback(
                PhGetPluginCallback(PluginInstance, PluginCallbackLoad),
                LoadCallback,
                NULL,
                &PluginLoadCallbackRegistration
                );
            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackOptionsWindowInitializing),
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

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessesUpdated),
                ProcessesUpdatedCallback,
                NULL,
                &ProcessesUpdatedCallbackRegistration
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
