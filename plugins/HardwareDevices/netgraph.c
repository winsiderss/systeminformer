/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2016
 *     dmex    2015-2024
 *
 */

#include "devices.h"

 VOID NetworkDeviceUpdatePanel(
     _Inout_ PDV_NETADAPTER_SYSINFO_CONTEXT Context
     )
 {
     ULONG64 linkSpeedValue = 0;
     NDIS_MEDIA_CONNECT_STATE mediaState = MediaConnectStateUnknown;
     HANDLE deviceHandle = NULL;
     PH_FORMAT format[2];
     WCHAR formatBuffer[256];
 
     if (NetAdapterEnableNdis)
     {
         if (NT_SUCCESS(PhCreateFile(
             &deviceHandle,
             &Context->AdapterEntry->AdapterId.InterfacePath->sr,
             FILE_GENERIC_READ,
             FILE_ATTRIBUTE_NORMAL,
             FILE_SHARE_READ | FILE_SHARE_WRITE,
             FILE_OPEN,
             FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
             )))
         {
             if (!Context->AdapterEntry->CheckedDeviceSupport)
             {
                 if (NetworkAdapterQuerySupported(deviceHandle))
                 {
                     Context->AdapterEntry->DeviceSupported = TRUE;
                 }
 
                 Context->AdapterEntry->CheckedDeviceSupport = TRUE;
             }
 
             if (!Context->AdapterEntry->DeviceSupported)
             {
                 NtClose(deviceHandle);
                 deviceHandle = NULL;
             }
         }
     }
 
     if (deviceHandle)
     {
         NDIS_LINK_STATE interfaceState;
 
         if (NT_SUCCESS(NetworkAdapterQueryLinkState(deviceHandle, &interfaceState)))
         {
             mediaState = interfaceState.MediaConnectState;
             linkSpeedValue = interfaceState.XmitLinkSpeed;
         }
         else
         {
             NetworkAdapterQueryLinkSpeed(deviceHandle, &linkSpeedValue);
         }
 
         NtClose(deviceHandle);
     }
     else
     {
         MIB_IF_ROW2 interfaceRow;
 
         if (NetworkAdapterQueryInterfaceRow(&Context->AdapterEntry->AdapterId, MibIfEntryNormal, &interfaceRow))
         {
             mediaState = interfaceRow.MediaConnectState;
             linkSpeedValue = interfaceRow.TransmitLinkSpeed;
         }
     }
 
     PhInitFormatSize(&format[0], Context->AdapterEntry->NetworkSendDelta.Value);
 
     if (PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), NULL))
         PhSetWindowText(Context->NetAdapterPanelSentLabel, formatBuffer);
     else
         PhSetWindowText(Context->NetAdapterPanelSentLabel, PhaFormatSize(Context->AdapterEntry->NetworkSendDelta.Value, ULONG_MAX)->Buffer);
 
     PhInitFormatSize(&format[0], Context->AdapterEntry->NetworkReceiveDelta.Value);
 
     if (PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), NULL))
         PhSetWindowText(Context->NetAdapterPanelReceivedLabel, formatBuffer);
     else
         PhSetWindowText(Context->NetAdapterPanelReceivedLabel, PhaFormatSize(Context->AdapterEntry->NetworkReceiveDelta.Value, ULONG_MAX)->Buffer);
 
     PhInitFormatSize(&format[0], Context->AdapterEntry->NetworkReceiveDelta.Value + Context->AdapterEntry->NetworkSendDelta.Value);
 
     if (PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), NULL))
         PhSetWindowText(Context->NetAdapterPanelTotalLabel, formatBuffer);
     else
         PhSetWindowText(Context->NetAdapterPanelTotalLabel, PhaFormatSize(Context->AdapterEntry->NetworkReceiveDelta.Value + Context->AdapterEntry->NetworkSendDelta.Value, ULONG_MAX)->Buffer);
 
     if (mediaState == MediaConnectStateConnected)
     {
         PhSetWindowText(Context->NetAdapterPanelStateLabel, L"Connected");
 
         //PhInitFormatSR(&format[0], PH_AUTO_T(PH_STRING, NetAdapterFormatBitratePrefix(linkSpeedValue))->sr);
         PhInitFormatSize(&format[0], linkSpeedValue / BITS_IN_ONE_BYTE);
         PhInitFormatS(&format[1], L"/s");
 
         if (PhFormatToBuffer(format, 2, formatBuffer, sizeof(formatBuffer), NULL))
             PhSetWindowText(Context->NetAdapterPanelSpeedLabel, formatBuffer);
         else
         {
             PhSetWindowText(Context->NetAdapterPanelSpeedLabel, PhaConcatStrings2(
                 PhaFormatSize(linkSpeedValue / BITS_IN_ONE_BYTE, ULONG_MAX)->Buffer,
                 L"/s"
                 )->Buffer);
             //PhSetWindowText(Context->NetAdapterPanelSpeedLabel, PH_AUTO_T(PH_STRING, NetAdapterFormatBitratePrefix(linkSpeedValue))->Buffer);
         }
     }
     else
     {
         PhSetWindowText(Context->NetAdapterPanelStateLabel, L"Disconnected");
         PhSetWindowText(Context->NetAdapterPanelSpeedLabel, L"N/A");
     }
 
     //PhInitFormatSR(&format[0], PH_AUTO_T(PH_STRING, NetAdapterFormatBitratePrefix((Context->AdapterEntry->CurrentNetworkReceive + Context->AdapterEntry->CurrentNetworkSend) * BITS_IN_ONE_BYTE))->sr);
     PhInitFormatSize(&format[0], Context->AdapterEntry->CurrentNetworkReceive + Context->AdapterEntry->CurrentNetworkSend);
     PhInitFormatS(&format[1], L"/s");
 
     if (PhFormatToBuffer(format, 2, formatBuffer, sizeof(formatBuffer), NULL))
         PhSetWindowText(Context->NetAdapterPanelBytesLabel, formatBuffer);
     else
     {
         PhSetWindowText(Context->NetAdapterPanelBytesLabel, PhaFormatString(
             L"%s/s",
             PhaFormatSize(Context->AdapterEntry->CurrentNetworkReceive + Context->AdapterEntry->CurrentNetworkSend, ULONG_MAX)->Buffer)->Buffer
             );
         //PhSetWindowText(Context->NetAdapterPanelBytesLabel, PH_AUTO_T(PH_STRING, NetAdapterFormatBitratePrefix((Context->AdapterEntry->CurrentNetworkReceive + Context->AdapterEntry->CurrentNetworkSend)* BITS_IN_ONE_BYTE))->Buffer);
     }
 }

INT_PTR CALLBACK NetworkDevicePanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_NETADAPTER_SYSINFO_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PDV_NETADAPTER_SYSINFO_CONTEXT)lParam;

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->NetAdapterPanelSentLabel = GetDlgItem(hwndDlg, IDC_STAT_BSENT);
            context->NetAdapterPanelReceivedLabel = GetDlgItem(hwndDlg, IDC_STAT_BRECEIVED);
            context->NetAdapterPanelTotalLabel = GetDlgItem(hwndDlg, IDC_STAT_BTOTAL);
            context->NetAdapterPanelStateLabel = GetDlgItem(hwndDlg, IDC_LINK_STATE);
            context->NetAdapterPanelSpeedLabel = GetDlgItem(hwndDlg, IDC_LINK_SPEED);
            context->NetAdapterPanelBytesLabel = GetDlgItem(hwndDlg, IDC_STAT_BYTESDELTA);
        }
        break;
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_DETAILS:
                ShowNetAdapterDetailsDialog(context);
                break;
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

VOID NetworkDeviceInitializeDialogDpi(
    _In_ PDV_NETADAPTER_SYSINFO_CONTEXT Context
    )
{
    Context->GraphPadding = PhGetDpi(RAPL_GRAPH_PADDING, Context->SysinfoSection->Parameters->WindowDpi);
}

VOID NetworkDeviceCreateGraphs(
    _Inout_ PDV_NETADAPTER_SYSINFO_CONTEXT Context
    )
{
    PhInitializeGraphState(&Context->GraphSendState);
    PhInitializeGraphState(&Context->GraphReceiveState);

    Context->GraphSendHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        Context->WindowHandle,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(Context->GraphSendHandle, TRUE);

    Context->GraphReceiveHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        Context->WindowHandle,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(Context->GraphReceiveHandle, TRUE);

    Context->LabelSendHandle = GetDlgItem(Context->WindowHandle, IDC_UP_L);
    Context->LabelReceiveHandle = GetDlgItem(Context->WindowHandle, IDC_DOWN_L);
}

VOID NetworkDeviceUpdateGraphs(
    _Inout_ PDV_NETADAPTER_SYSINFO_CONTEXT Context
    )
{
    Context->GraphSendState.Valid = FALSE;
    Context->GraphSendState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->GraphSendHandle, 1);
    Graph_Draw(Context->GraphSendHandle);
    Graph_UpdateTooltip(Context->GraphSendHandle);
    InvalidateRect(Context->GraphSendHandle, NULL, FALSE);

    Context->GraphReceiveState.Valid = FALSE;
    Context->GraphReceiveState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->GraphReceiveHandle, 1);
    Graph_Draw(Context->GraphReceiveHandle);
    Graph_UpdateTooltip(Context->GraphReceiveHandle);
    InvalidateRect(Context->GraphReceiveHandle, NULL, FALSE);
}

VOID NetworkDeviceLayoutGraphs(
    _Inout_ PDV_NETADAPTER_SYSINFO_CONTEXT Context
    )
{
    RECT clientRect;
    RECT labelRect;
    RECT margin;
    ULONG graphWidth;
    ULONG graphHeight;
    HDWP deferHandle;
    ULONG y;

    Context->GraphSendState.Valid = FALSE;
    Context->GraphSendState.TooltipIndex = ULONG_MAX;
    Context->GraphReceiveState.Valid = FALSE;
    Context->GraphReceiveState.TooltipIndex = ULONG_MAX;

    margin = Context->GraphMargin;
    PhGetSizeDpiValue(&margin, Context->SysinfoSection->Parameters->WindowDpi, TRUE);

    GetClientRect(Context->WindowHandle, &clientRect);
    GetClientRect(Context->LabelSendHandle, &labelRect);
    graphWidth = clientRect.right - margin.left - margin.right;
    graphHeight = (clientRect.bottom - margin.top - margin.bottom - labelRect.bottom * 2 - Context->GraphPadding * 3) / 2;

    deferHandle = BeginDeferWindowPos(4);
    y = margin.top;

    deferHandle = DeferWindowPos(
        deferHandle,
        Context->LabelSendHandle,
        NULL,
        margin.left,
        y,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += labelRect.bottom + Context->GraphPadding;

    deferHandle = DeferWindowPos(
        deferHandle,
        Context->GraphSendHandle,
        NULL,
        margin.left,
        y,
        graphWidth,
        graphHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += graphHeight + Context->GraphPadding;

    deferHandle = DeferWindowPos(
        deferHandle,
        Context->LabelReceiveHandle,
        NULL,
        margin.left,
        y,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += labelRect.bottom + Context->GraphPadding;

    deferHandle = DeferWindowPos(
        deferHandle,
        Context->GraphReceiveHandle,
        NULL,
        margin.left,
        y,
        graphWidth,
        graphHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += graphHeight + Context->GraphPadding;

    EndDeferWindowPos(deferHandle);
}

VOID NetworkDeviceNotifyProcessorGraph(
    _Inout_ PDV_NETADAPTER_SYSINFO_CONTEXT Context,
    _In_ NMHDR *Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y;
            Context->SysinfoSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorIoWrite"), 0, Context->SysinfoSection->Parameters->WindowDpi);

            PhGraphStateGetDrawInfo(
                &Context->GraphSendState,
                getDrawInfo,
                Context->AdapterEntry->OutboundBuffer.Count
                );

            if (!Context->GraphSendState.Valid)
            {
                ULONG i;
                FLOAT max = 4 * 1024; // Minimum scaling 4KB

                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data1;

                    Context->GraphSendState.Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&Context->AdapterEntry->OutboundBuffer, i);

                    if (max < data1)
                        max = data1;
                }

                if (max != 0)
                {
                    // Scale the data.

                    PhDivideSinglesBySingle(
                        Context->GraphSendState.Data1,
                        max,
                        drawInfo->LineDataCount
                        );
                }

                drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                drawInfo->LabelYFunctionParameter = max;

                Context->GraphSendState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (Context->GraphSendState.TooltipIndex != getTooltipText->Index)
                {
                    ULONG64 value;
                    PH_FORMAT format[4];

                    value = PhGetItemCircularBuffer_ULONG64(
                        &Context->AdapterEntry->OutboundBuffer,
                        getTooltipText->Index
                        );

                    // R: %s\nS: %s\n%s
                    PhInitFormatS(&format[0], L"S: ");
                    PhInitFormatSize(&format[1], value);
                    PhInitFormatC(&format[2], L'\n');
                    PhInitFormatSR(&format[3], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&Context->GraphSendState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                getTooltipText->Text = PhGetStringRef(Context->GraphSendState.TooltipText);
            }
        }
        break;
    }
}

VOID NetworkDeviceNotifyPackageGraph(
    _Inout_ PDV_NETADAPTER_SYSINFO_CONTEXT Context,
    _In_ NMHDR *Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y;
            Context->SysinfoSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), 0, Context->SysinfoSection->Parameters->WindowDpi);

            PhGraphStateGetDrawInfo(
                &Context->GraphReceiveState,
                getDrawInfo,
                Context->AdapterEntry->InboundBuffer.Count
                );

            if (!Context->GraphReceiveState.Valid)
            {
                ULONG i;
                FLOAT max = 4 * 1024; // Minimum scaling 4KB

                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data1;

                    Context->GraphReceiveState.Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&Context->AdapterEntry->InboundBuffer, i);

                    if (max < data1)
                        max = data1;
                }

                if (max != 0)
                {
                    // Scale the data.

                    PhDivideSinglesBySingle(
                        Context->GraphReceiveState.Data1,
                        max,
                        drawInfo->LineDataCount
                        );
                }

                drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                drawInfo->LabelYFunctionParameter = max;

                Context->GraphReceiveState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (Context->GraphReceiveState.TooltipIndex != getTooltipText->Index)
                {
                    ULONG64 value;
                    PH_FORMAT format[4];

                    value = PhGetItemCircularBuffer_ULONG64(
                        &Context->AdapterEntry->InboundBuffer,
                        getTooltipText->Index
                        );

                    // R: %s\nS: %s\n%s
                    PhInitFormatS(&format[0], L"R: ");
                    PhInitFormatSize(&format[1], value);
                    PhInitFormatC(&format[2], L'\n');
                    PhInitFormatSR(&format[3], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&Context->GraphReceiveState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                getTooltipText->Text = PhGetStringRef(Context->GraphReceiveState.TooltipText);
            }
        }
        break;
    }
}

VOID NetworkDeviceTickDialog(
    _Inout_ PDV_NETADAPTER_SYSINFO_CONTEXT Context
    )
{
    NetworkDeviceUpdateGraphs(Context);
    NetworkDeviceUpdatePanel(Context);
}

VOID NetworkDeviceUpdateTitle(
    _In_ PDV_NETADAPTER_SYSINFO_CONTEXT Context
    )
{
    if (Context->AdapterEntry->PendingQuery)
    {
        if (Context->AdapterTextLabel)
            PhSetWindowText(Context->AdapterTextLabel, L"Pending...");
        if (Context->AdapterNameLabel)
            PhSetWindowText(Context->AdapterNameLabel, L"Pending...");
    }
    else
    {
        if (Context->AdapterTextLabel)
            PhSetWindowText(Context->AdapterTextLabel, PhGetStringOrDefault(Context->AdapterEntry->AdapterAlias, L"Unknown"));
        if (Context->AdapterNameLabel)
            PhSetWindowText(Context->AdapterNameLabel, PhGetStringOrDefault(Context->AdapterEntry->AdapterName, L"Unknown"));
    }
}

VOID NetworkDeviceUpdateAdapterNameText(
    _In_ PDV_NETADAPTER_ENTRY AdapterEntry
    )
{
    NetworkDeviceUpdateDeviceInfo(NULL, AdapterEntry);
}

NTSTATUS NetworkDeviceQueryNameWorkQueueItem(
    _In_ PDV_NETADAPTER_ENTRY AdapterEntry
    )
{
    // Update the adapter aliases, index and guids (dmex)
    NetworkDeviceUpdateAdapterNameText(AdapterEntry);

#ifdef FORCE_DELAY_LABEL_WORKQUEUE
    PhDelayExecution(4000);
#endif

    InterlockedExchange(&AdapterEntry->JustProcessed, TRUE);
    AdapterEntry->PendingQuery = FALSE;

    PhDereferenceObject(AdapterEntry);
    return STATUS_SUCCESS;
}

VOID NetworkDeviceQueueNameUpdate(
    _In_ PDV_NETADAPTER_ENTRY AdapterEntry
    )
{
    AdapterEntry->PendingQuery = TRUE;

    PhReferenceObject(AdapterEntry);
    PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), NetworkDeviceQueryNameWorkQueueItem, AdapterEntry);
}

INT_PTR CALLBACK NetworkDeviceDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_NETADAPTER_SYSINFO_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PDV_NETADAPTER_SYSINFO_CONTEXT)lParam;

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PPH_LAYOUT_ITEM graphItem;
            PPH_LAYOUT_ITEM panelItem;

            context->WindowHandle = hwndDlg;
            context->AdapterTextLabel = GetDlgItem(hwndDlg, IDC_TITLE);
            context->AdapterNameLabel = GetDlgItem(hwndDlg, IDC_DEVICENAME);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->AdapterNameLabel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            graphItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_PANEL_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            context->GraphMargin = graphItem->Margin;

            SetWindowFont(context->AdapterTextLabel, context->SysinfoSection->Parameters->LargeFont, FALSE);
            SetWindowFont(context->AdapterNameLabel, context->SysinfoSection->Parameters->MediumFont, FALSE);
            PhSetWindowText(context->AdapterNameLabel, PhGetStringOrDefault(context->AdapterEntry->AdapterName, L"Unknown"));

            context->PanelWindowHandle = PhCreateDialog(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_NETADAPTER_PANEL), hwndDlg, NetworkDevicePanelDialogProc, context);
            ShowWindow(context->PanelWindowHandle, SW_SHOW);
            PhAddLayoutItemEx(&context->LayoutManager, context->PanelWindowHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, panelItem->Margin);

            NetworkDeviceInitializeDialogDpi(context);
            NetworkDeviceUpdateTitle(context);
            NetworkDeviceCreateGraphs(context);
            NetworkDeviceUpdateGraphs(context);
            NetworkDeviceUpdatePanel(context);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);

            PhDeleteGraphState(&context->GraphSendState);
            PhDeleteGraphState(&context->GraphReceiveState);

            if (context->GraphSendHandle)
                DestroyWindow(context->GraphSendHandle);
            if (context->GraphReceiveHandle)
                DestroyWindow(context->GraphReceiveHandle);
            if (context->PanelWindowHandle)
                DestroyWindow(context->PanelWindowHandle);
        }
        break;
    case WM_NCDESTROY:
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            NetworkDeviceInitializeDialogDpi(context);

            if (context->SysinfoSection->Parameters->LargeFont)
            {
                SetWindowFont(context->AdapterTextLabel, context->SysinfoSection->Parameters->LargeFont, FALSE);
            }

            if (context->SysinfoSection->Parameters->MediumFont)
            {
                SetWindowFont(context->AdapterNameLabel, context->SysinfoSection->Parameters->MediumFont, FALSE);
            }

            PhLayoutManagerLayout(&context->LayoutManager);
            NetworkDeviceLayoutGraphs(context);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
            NetworkDeviceLayoutGraphs(context);
        }
        break;
    case WM_NOTIFY:
        {
            NMHDR* header = (NMHDR*)lParam;

            if (header->hwndFrom == context->GraphSendHandle)
            {
                NetworkDeviceNotifyProcessorGraph(context, header);
            }
            else if (header->hwndFrom == context->GraphReceiveHandle)
            {
                NetworkDeviceNotifyPackageGraph(context, header);
            }
        }
        break;
    case WM_CTLCOLORBTN:
        return HANDLE_WM_CTLCOLORBTN(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORDLG:
        return HANDLE_WM_CTLCOLORDLG(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    case WM_CTLCOLORSTATIC:
        return HANDLE_WM_CTLCOLORSTATIC(hwndDlg, wParam, lParam, PhWindowThemeControlColor);
    }

    return FALSE;
}

BOOLEAN NetworkDeviceSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2
    )
{
    PDV_NETADAPTER_SYSINFO_CONTEXT context = (PDV_NETADAPTER_SYSINFO_CONTEXT)Section->Context;

    switch (Message)
    {
    case SysInfoCreate:
        {
            NetworkDeviceQueueNameUpdate(context->AdapterEntry);
        }
        return TRUE;
    case SysInfoDestroy:
        {
            PhDereferenceObject(context->AdapterEntry);
            PhFree(context);
        }
        return TRUE;
    case SysInfoTick:
        {
            if (context->WindowHandle)
            {
                if (context->AdapterEntry->JustProcessed)
                {
                    NetworkDeviceUpdateTitle(context);
                    InterlockedExchange(&context->AdapterEntry->JustProcessed, FALSE);
                }

                NetworkDeviceTickDialog(context);
            }
        }
        return TRUE;
    case SysInfoViewChanging:
        {
            PH_SYSINFO_VIEW_TYPE view = (PH_SYSINFO_VIEW_TYPE)PtrToUlong(Parameter1);
            PPH_SYSINFO_SECTION section = (PPH_SYSINFO_SECTION)Parameter2;

            if (view == SysInfoSummaryView || section != Section)
                return TRUE;

            if (context->GraphSendHandle)
            {
                context->GraphSendState.Valid = FALSE;
                context->GraphSendState.TooltipIndex = ULONG_MAX;
                Graph_Draw(context->GraphSendHandle);
            }

            if (context->GraphReceiveHandle)
            {
                context->GraphReceiveState.Valid = FALSE;
                context->GraphReceiveState.TooltipIndex = ULONG_MAX;
                Graph_Draw(context->GraphReceiveHandle);
            }
        }
        return TRUE;
    case SysInfoCreateDialog:
        {
            PPH_SYSINFO_CREATE_DIALOG createDialog = (PPH_SYSINFO_CREATE_DIALOG)Parameter1;

            createDialog->Instance = PluginInstance->DllBase;
            createDialog->Template = MAKEINTRESOURCE(IDD_NETADAPTER_DIALOG);
            createDialog->DialogProc = NetworkDeviceDialogProc;
            createDialog->Parameter = context;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = (PPH_GRAPH_DRAW_INFO)Parameter1;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y | PH_GRAPH_USE_LINE_2;
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), PhGetIntegerSetting(L"ColorIoWrite"), Section->Parameters->WindowDpi);
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, context->AdapterEntry->InboundBuffer.Count);

            if (!Section->GraphState.Valid)
            {
                FLOAT max = 4 * 1024; // Minimum scaling 4KB

                for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data1;
                    FLOAT data2;

                    Section->GraphState.Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->AdapterEntry->InboundBuffer, i);
                    Section->GraphState.Data2[i] = data2 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->AdapterEntry->OutboundBuffer, i);

                    if (max < data1 + data2)
                        max = data1 + data2;
                }

                if (max != 0)
                {
                    // Scale the data.

                    PhDivideSinglesBySingle(
                        Section->GraphState.Data1,
                        max,
                        drawInfo->LineDataCount
                        );
                    PhDivideSinglesBySingle(
                        Section->GraphState.Data2,
                        max,
                        drawInfo->LineDataCount
                        );
                }

                drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                drawInfo->LabelYFunctionParameter = max;

                Section->GraphState.Valid = TRUE;
            }
        }
        return TRUE;
    case SysInfoGraphGetTooltipText:
        {
            PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT getTooltipText = (PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT)Parameter1;
            ULONG64 adapterInboundValue;
            ULONG64 adapterOutboundValue;
            PH_FORMAT format[6];

            adapterInboundValue = PhGetItemCircularBuffer_ULONG64(
                &context->AdapterEntry->InboundBuffer,
                getTooltipText->Index
                );

            adapterOutboundValue = PhGetItemCircularBuffer_ULONG64(
                &context->AdapterEntry->OutboundBuffer,
                getTooltipText->Index
                );

            // R: %s\nS: %s\n%s
            PhInitFormatS(&format[0], L"R: ");
            PhInitFormatSize(&format[1], adapterInboundValue);
            PhInitFormatS(&format[2], L"\nS: ");
            PhInitFormatSize(&format[3], adapterOutboundValue);
            PhInitFormatC(&format[4], L'\n');
            PhInitFormatSR(&format[5], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

            PhMoveReference(&Section->GraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
            getTooltipText->Text = PhGetStringRef(Section->GraphState.TooltipText);
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = (PPH_SYSINFO_DRAW_PANEL)Parameter1;
            PH_FORMAT format[4];

            if (context->AdapterEntry->PendingQuery)
                PhMoveReference(&drawPanel->Title, PhCreateString(L"Pending..."));
            else
            {
                if (context->AdapterEntry->AdapterAlias)
                    PhSetReference(&drawPanel->Title, context->AdapterEntry->AdapterAlias);
                else
                    PhSetReference(&drawPanel->Title, context->AdapterEntry->AdapterName);
            }

            if (!drawPanel->Title)
                drawPanel->Title = PhCreateString(L"Unknown");

            // R: %s\nS: %s
            PhInitFormatS(&format[0], L"R: ");
            PhInitFormatSize(&format[1], context->AdapterEntry->CurrentNetworkReceive);
            PhInitFormatS(&format[2], L"\nS: ");
            PhInitFormatSize(&format[3], context->AdapterEntry->CurrentNetworkSend);

            drawPanel->SubTitle = PhFormat(format, RTL_NUMBER_OF(format), 0);
        }
        return TRUE;
    }

    return FALSE;
}

VOID NetworkDeviceSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers,
    _In_ _Assume_refs_(1) PDV_NETADAPTER_ENTRY AdapterEntry
    )
{
    static PH_STRINGREF text = PH_STRINGREF_INIT(L"Unknown");
    PDV_NETADAPTER_SYSINFO_CONTEXT context;
    PH_SYSINFO_SECTION section;

    context = PhAllocateZero(sizeof(DV_NETADAPTER_SYSINFO_CONTEXT));
    context->AdapterEntry = PhReferenceObject(AdapterEntry);
    context->AdapterEntry->PendingQuery = TRUE;

    memset(&section, 0, sizeof(PH_SYSINFO_SECTION));
    section.Context = context;
    section.Callback = NetworkDeviceSectionCallback;
    section.Name = text;

    context->SysinfoSection = Pointers->CreateSection(&section);
}
