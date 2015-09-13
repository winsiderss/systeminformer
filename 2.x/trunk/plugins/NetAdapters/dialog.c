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

static PH_LAYOUT_MANAGER LayoutManager;

static INT PhAddListViewItemGroupId(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _In_ INT SubIndex,
    _In_ INT GroupId,
    _In_ PWSTR Text
    )
{
    LVITEM item;

    item.mask = LVIF_TEXT | LVIF_GROUPID;
    item.iItem = Index;
    item.iSubItem = SubIndex;
    item.pszText = Text;
    item.iGroupId = GroupId;

    return ListView_InsertItem(ListViewHandle, &item);
}

static INT PhAddListViewGroup(
    _In_ HWND ListViewHandle,
    _In_ INT Index,
    _In_ PWSTR Text
    )
{
    LVGROUP group;

    group.cbSize = sizeof(LVGROUP);
    group.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_ALIGN | LVGF_STATE;
    group.uAlign = LVGA_HEADER_LEFT;
    group.state = LVGS_COLLAPSIBLE;
    group.iGroupId = Index;
    group.pszHeader = Text;

    return (INT)ListView_InsertGroup(ListViewHandle, INT_MAX, &group);
}

static NDIS_STATISTICS_INFO NetAdapterRowToNdisStatistics(
    _In_ MIB_IF_ROW2 Row
    )
{
    NDIS_STATISTICS_INFO interfaceStats;

    interfaceStats.ifInDiscards = Row.InDiscards;
    interfaceStats.ifInErrors = Row.InErrors;
    interfaceStats.ifHCInOctets = Row.InOctets;
    interfaceStats.ifHCInUcastPkts = Row.InUcastPkts;
    //interfaceStats.ifHCInMulticastPkts;
    //interfaceStats.ifHCInBroadcastPkts;
    interfaceStats.ifHCOutOctets = Row.OutOctets;
    interfaceStats.ifHCOutUcastPkts = Row.OutUcastPkts;
    //interfaceStats.ifHCOutMulticastPkts;
    //interfaceStats.ifHCOutBroadcastPkts;
    interfaceStats.ifOutErrors = Row.OutErrors;
    interfaceStats.ifOutDiscards = Row.OutDiscards;
    interfaceStats.ifHCInUcastOctets = Row.InUcastOctets;
    interfaceStats.ifHCInMulticastOctets = Row.InMulticastOctets;
    interfaceStats.ifHCInBroadcastOctets = Row.InBroadcastOctets;
    interfaceStats.ifHCOutUcastOctets = Row.OutUcastOctets;
    interfaceStats.ifHCOutMulticastOctets = Row.OutMulticastOctets;
    interfaceStats.ifHCOutBroadcastOctets = Row.OutBroadcastOctets;
    //interfaceRow.InNUcastPkts;
    //interfaceRow.InUnknownProtos;
    //interfaceRow.OutNUcastPkts;
    //interfaceRow.OutQLen;

    return interfaceStats;
}

static VOID NetAdapterUpdateDetails(
    _Inout_ PPH_NETADAPTER_SYSINFO_CONTEXT Context
    )
{
    ULONG64 interfaceLinkSpeed = 0;
    ULONG64 interfaceRcvSpeed = 0;
    ULONG64 interfaceXmitSpeed = 0;
    ULONG64 interfaceRcvUnicastSpeed = 0;
    ULONG64 interfaceXmitUnicastSpeed = 0;
    FLOAT utilization = 0.0f;
    FLOAT utilization2 = 0.0f;
    NDIS_STATISTICS_INFO interfaceStats = { 0 };
    NDIS_MEDIA_CONNECT_STATE mediaState = MediaConnectStateUnknown;
    INT index = 0;

    //PH_NETADAPTER_CONFIG Config = { 0 };
    //BOOLEAN internet = FALSE;

    //NetworkAdapterQueryConfig(Context, &Config);

    //if (Config.IPAddress)
    //{
    //    //internet = NetworkAdapterQueryInternet(Context, Config.IPAddress);
    //}

    if (Context->DeviceHandle)
    {
        NDIS_LINK_STATE interfaceState;

        NetworkAdapterQueryStatistics(Context->DeviceHandle, &interfaceStats);

        if (!NT_SUCCESS(NetworkAdapterQueryLinkState(Context->DeviceHandle, &interfaceState)))
        {
            NetworkAdapterQueryLinkSpeed(Context->DeviceHandle, &interfaceLinkSpeed);
        }
        else
        {
            mediaState = interfaceState.MediaConnectState;
            interfaceLinkSpeed = interfaceState.XmitLinkSpeed;
        }
    }
    else
    {
        if (GetIfEntry2_I)
        {
            MIB_IF_ROW2 interfaceRow;

            interfaceRow = QueryInterfaceRowVista(Context);

            interfaceStats = NetAdapterRowToNdisStatistics(interfaceRow);

            mediaState = interfaceRow.MediaConnectState;
            interfaceLinkSpeed = interfaceRow.TransmitLinkSpeed;
        }
        else
        {
            MIB_IFROW interfaceRow;

            interfaceRow = QueryInterfaceRowXP(Context);

            interfaceStats.ifHCInOctets = interfaceRow.dwInOctets;
            interfaceStats.ifHCOutOctets = interfaceRow.dwOutOctets;
            interfaceLinkSpeed = interfaceRow.dwSpeed;

            if (interfaceRow.dwOperStatus == IF_OPER_STATUS_OPERATIONAL)
                mediaState = MediaConnectStateConnected;
            else
                mediaState = MediaConnectStateDisconnected;
        }
    }

    //interfaceRcvSpeed = interfaceStats.ifHCInOctets - Context->LastDetailsInboundValue;
    //interfaceXmitSpeed = interfaceStats.ifHCOutOctets - Context->LastDetailsIOutboundValue;
    //interfaceRcvUnicastSpeed = interfaceStats.ifHCInUcastOctets - Context->LastDetailsInboundUnicastValue;
    //interfaceXmitUnicastSpeed = interfaceStats.ifHCOutUcastOctets - Context->LastDetailsIOutboundUnicastValue;

    if (!Context->HaveFirstDetailsSample)
    {
        interfaceRcvSpeed = 0;
        interfaceXmitSpeed = 0;
        interfaceRcvUnicastSpeed = 0;
        interfaceXmitUnicastSpeed = 0;
        Context->HaveFirstDetailsSample = TRUE;
    }

    //utilization = (FLOAT)((interfaceRcvSpeed + interfaceXmitSpeed) * BITS_IN_ONE_BYTE) / interfaceLinkSpeed * 100;
    utilization = (FLOAT)(interfaceRcvSpeed + interfaceXmitSpeed) / (interfaceLinkSpeed / BITS_IN_ONE_BYTE) * 100;
    utilization2 = (FLOAT)(interfaceRcvUnicastSpeed + interfaceXmitUnicastSpeed) / (interfaceLinkSpeed / BITS_IN_ONE_BYTE) * 100;

    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, mediaState == MediaConnectStateConnected ? L"Connected" : L"Disconnected");

    //PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, internet ? L"Internet" : L"Local");
    //PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, Config.Domain ? Config.Domain->Buffer : L"");
    //PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, Config.IPAddress ? Config.IPAddress->Buffer : L"");
    //PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, Config.SubnetMask ? Config.SubnetMask->Buffer : L"");
    //PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, Config.DefaultGateway ? Config.DefaultGateway->Buffer : L"");

    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatString(L"%s/s", PhaFormatSize(interfaceLinkSpeed / BITS_IN_ONE_BYTE, -1)->Buffer)->Buffer);   
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatSize(interfaceStats.ifHCOutOctets, -1)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatSize(interfaceStats.ifHCInOctets, -1)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatSize(interfaceStats.ifHCInOctets + interfaceStats.ifHCOutOctets, -1)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, interfaceXmitSpeed != 0 ? PhaFormatString(L"%s/s", PhaFormatSize(interfaceXmitSpeed, -1)->Buffer)->Buffer : L"");
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, interfaceRcvSpeed != 0 ? PhaFormatString(L"%s/s", PhaFormatSize(interfaceRcvSpeed, -1)->Buffer)->Buffer : L"");
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatString(L"%.2f%%", utilization)->Buffer);

    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatUInt64(interfaceStats.ifHCOutUcastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatUInt64(interfaceStats.ifHCInUcastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatUInt64(interfaceStats.ifHCInUcastPkts + interfaceStats.ifHCOutUcastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatSize(interfaceStats.ifHCOutUcastOctets, -1)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatSize(interfaceStats.ifHCInUcastOctets, -1)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatSize(interfaceStats.ifHCInUcastOctets + interfaceStats.ifHCOutUcastOctets, -1)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, interfaceXmitUnicastSpeed != 0 ? PhaFormatString(L"%s/s", PhaFormatSize(interfaceXmitUnicastSpeed, -1)->Buffer)->Buffer : L"");
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, interfaceRcvUnicastSpeed != 0 ? PhaFormatString(L"%s/s", PhaFormatSize(interfaceRcvUnicastSpeed, -1)->Buffer)->Buffer : L"");
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatString(L"%.2f%%", utilization2)->Buffer);

    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatUInt64(interfaceStats.ifHCOutBroadcastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatUInt64(interfaceStats.ifHCInBroadcastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatUInt64(interfaceStats.ifHCInBroadcastPkts + interfaceStats.ifHCOutBroadcastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatSize(interfaceStats.ifHCOutBroadcastOctets, -1)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatSize(interfaceStats.ifHCInBroadcastOctets, -1)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatSize(interfaceStats.ifHCInBroadcastOctets + interfaceStats.ifHCOutBroadcastOctets, -1)->Buffer);

    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatUInt64(interfaceStats.ifHCOutMulticastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatUInt64(interfaceStats.ifHCInMulticastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatUInt64(interfaceStats.ifHCInMulticastPkts + interfaceStats.ifHCOutMulticastPkts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatSize(interfaceStats.ifHCOutMulticastOctets, -1)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatSize(interfaceStats.ifHCInMulticastOctets, -1)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatSize(interfaceStats.ifHCInMulticastOctets + interfaceStats.ifHCOutMulticastOctets, -1)->Buffer);

    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatUInt64(interfaceStats.ifOutErrors, TRUE)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatUInt64(interfaceStats.ifInErrors, TRUE)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatUInt64(interfaceStats.ifInErrors + interfaceStats.ifOutErrors, TRUE)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatUInt64(interfaceStats.ifOutDiscards, TRUE)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatUInt64(interfaceStats.ifInDiscards, TRUE)->Buffer);
    PhSetListViewSubItem(Context->DetailsLvHandle, index++, 1, PhaFormatUInt64(interfaceStats.ifInDiscards + interfaceStats.ifOutDiscards, TRUE)->Buffer);

    //Context->LastDetailsInboundValue = interfaceStats.ifHCInOctets;
    //Context->LastDetailsIOutboundValue = interfaceStats.ifHCOutOctets;
    //Context->LastDetailsInboundUnicastValue = interfaceStats.ifHCInUcastOctets;
    //Context->LastDetailsIOutboundUnicastValue = interfaceStats.ifHCOutUcastOctets;
}

static INT_PTR CALLBACK AdapterDetailsDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_NETADAPTER_SYSINFO_CONTEXT context;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PPH_NETADAPTER_SYSINFO_CONTEXT)lParam;
        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PPH_NETADAPTER_SYSINFO_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_DESTROY)
            RemoveProp(hwndDlg, L"Context");
    }

    if (!context)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->DetailsLvHandle = GetDlgItem(hwndDlg, IDC_DETAILS_LIST);
            context->DetailsHandle = hwndDlg;

            PhCenterWindow(hwndDlg, GetParent(hwndDlg));

            PhInitializeLayoutManager(&LayoutManager, hwndDlg);
            PhAddLayoutItem(&LayoutManager, context->DetailsLvHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&LayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_RIGHT);

            PhSetListViewStyle(context->DetailsLvHandle, FALSE, TRUE);
            PhSetControlTheme(context->DetailsLvHandle, L"explorer");
            PhAddListViewColumn(context->DetailsLvHandle, 0, 0, 0, LVCFMT_LEFT, 325, L"Property");
            PhAddListViewColumn(context->DetailsLvHandle, 1, 1, 1, LVCFMT_LEFT, 90, L"Value");
            //PhSetExtendedListView(context->DetailsLvHandle);

            if (context->AdapterName)
                SetWindowText(hwndDlg, context->AdapterName->Buffer);

            SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)PhGetFileShellIcon(NULL, L".exe", FALSE));
            SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)PhGetFileShellIcon(NULL, L".exe", TRUE));

            ListView_EnableGroupView(context->DetailsLvHandle, TRUE);
            PhAddListViewGroup(context->DetailsLvHandle, 0, L"Adapter");
            PhAddListViewGroup(context->DetailsLvHandle, 1, L"Unicast");
            PhAddListViewGroup(context->DetailsLvHandle, 2, L"Broadcast");
            PhAddListViewGroup(context->DetailsLvHandle, 3, L"Multicast");
            PhAddListViewGroup(context->DetailsLvHandle, 4, L"Errors");

            INT index = 0;
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 0, L"State");
            //PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 0, L"Connectivity");
            //PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 0, L"Domain");
            //PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 0, L"IP Address");
            //PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 0, L"SubnetMask");
            //PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 0, L"DefaultGateway");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 0, L"Link Speed");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 0, L"Sent");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 0, L"Received");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 0, L"Total");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 0, L"Sending");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 0, L"Receiving");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 0, L"Utilization");

            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 1, L"Sent Packets");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 1, L"Received Packets");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 1, L"Total Packets");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 1, L"Sent");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 1, L"Received");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 1, L"Total");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 1, L"Sending");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 1, L"Receiving");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 1, L"Utilization");

            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 2, L"Sent Packets");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 2, L"Received Packets");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 2, L"Total Packets");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 2, L"Sent");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 2, L"Received");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 2, L"Total");

            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 3, L"Sent Packets");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 3, L"Received Packets");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 3, L"Total Packets");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 3, L"Sent");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 3, L"Received");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 3, L"Total");

            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 4, L"Send Errors");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 4, L"Receive Errors");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 4, L"Total Errors");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 4, L"Send Discards");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 4, L"Receive Discards");
            PhAddListViewItemGroupId(context->DetailsLvHandle, index++, 0, 4, L"Total Discards");

            NetAdapterUpdateDetails(context);
        }
        break;
    case WM_DESTROY:
        {
            context->DetailsHandle = NULL;
            context->DetailsLvHandle = NULL;

            PhDeleteLayoutManager(&LayoutManager);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                EndDialog(hwndDlg, IDOK);
                break;
            }
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&LayoutManager);
        }
        break;
    case MSG_UPDATE:
        {
            NetAdapterUpdateDetails(context);
        }
        break;
    }

    return FALSE;
}

VOID ShowDetailsDialog(
    _In_opt_ PPH_NETADAPTER_SYSINFO_CONTEXT Context
    )
{
    DialogBoxParam(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_NETADAPTER_DETAILS),
        Context->WindowHandle,
        AdapterDetailsDlgProc,
        (LPARAM)Context
        );
}