/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2015-2024
 *
 */

#include "devices.h"

VOID DiskDeviceUpdatePanel(
    _Inout_ PDV_DISK_SYSINFO_CONTEXT Context
    )
{
    PH_FORMAT format[3];
    WCHAR formatBuffer[256];

    PhInitFormatSize(&format[0], Context->DiskEntry->BytesReadDelta.Value);

    if (PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->DiskDevicePanelReadLabel, formatBuffer);
    else
        PhSetWindowText(Context->DiskDevicePanelReadLabel, PhaFormatSize(Context->DiskEntry->BytesReadDelta.Value, ULONG_MAX)->Buffer);

    PhInitFormatSize(&format[0], Context->DiskEntry->BytesWrittenDelta.Value);

    if (PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->DiskDevicePanelWriteLabel, formatBuffer);
    else
        PhSetWindowText(Context->DiskDevicePanelWriteLabel, PhaFormatSize(Context->DiskEntry->BytesWrittenDelta.Value, ULONG_MAX)->Buffer);

    PhInitFormatSize(&format[0], Context->DiskEntry->BytesReadDelta.Value + Context->DiskEntry->BytesWrittenDelta.Value);

    if (PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->DiskDevicePanelTotalLabel, formatBuffer);
    else
        PhSetWindowText(Context->DiskDevicePanelTotalLabel, PhaFormatSize(Context->DiskEntry->BytesReadDelta.Value + Context->DiskEntry->BytesWrittenDelta.Value, ULONG_MAX)->Buffer);

    PhInitFormatF(&format[0], Context->DiskEntry->ActiveTime, 0);
    PhInitFormatC(&format[1], L'%');

    if (PhFormatToBuffer(format, 2, formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->DiskDevicePanelActiveLabel, formatBuffer);
    else
        PhSetWindowText(Context->DiskDevicePanelActiveLabel, PhaFormatString(L"%.0f%%", Context->DiskEntry->ActiveTime)->Buffer);

    PhInitFormatF(&format[0], Context->DiskEntry->ResponseTime / PH_TICKS_PER_MS, 1);
    PhInitFormatS(&format[1], L" ms");

    if (PhFormatToBuffer(format, 2, formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->DiskDevicePanelTimeLabel, formatBuffer);
    else
        PhSetWindowText(Context->DiskDevicePanelTimeLabel, PhaFormatString(L"%.1f ms", Context->DiskEntry->ResponseTime / PH_TICKS_PER_MS)->Buffer);

    PhInitFormatSize(&format[0], Context->DiskEntry->BytesReadDelta.Delta + Context->DiskEntry->BytesWrittenDelta.Delta);
    PhInitFormatS(&format[1], L"/s");

    if (PhFormatToBuffer(format, 2, formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->DiskDevicePanelBytesLabel, formatBuffer);
    else
        PhSetWindowText(Context->DiskDevicePanelBytesLabel, PhaFormatString(L"%s/s", PhaFormatSize(Context->DiskEntry->BytesReadDelta.Delta + Context->DiskEntry->BytesWrittenDelta.Delta, ULONG_MAX)->Buffer)->Buffer);

    PhInitFormatI64UGroupDigits(&format[0], Context->DiskEntry->QueueDepth);
    PhInitFormatS(&format[1], L" | ");
    PhInitFormatI64UGroupDigits(&format[2], Context->DiskEntry->ReadCountDelta.Delta + Context->DiskEntry->WriteCountDelta.Delta);

    if (PhFormatToBuffer(format, 3, formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(GetDlgItem(Context->PanelWindowHandle, IDC_STAT_QUEUELENGTH), formatBuffer);
    else
        PhSetWindowText(GetDlgItem(Context->PanelWindowHandle, IDC_STAT_QUEUELENGTH), PhaFormatString(L"%lu | %lu", Context->DiskEntry->QueueDepth, Context->DiskEntry->ReadCountDelta.Delta + Context->DiskEntry->WriteCountDelta.Delta)->Buffer);

    PhInitFormatI64UGroupDigits(&format[0], Context->DiskEntry->SplitCount);

    if (PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(GetDlgItem(Context->PanelWindowHandle, IDC_STAT_SPLITCOUNT), formatBuffer);
    else
        PhSetWindowText(GetDlgItem(Context->PanelWindowHandle, IDC_STAT_SPLITCOUNT), PhaFormatString(L"%lu", Context->DiskEntry->SplitCount)->Buffer);
}

VOID DiskDeviceUpdateTitle(
    _In_ PDV_DISK_SYSINFO_CONTEXT Context
    )
{
    if (Context->DiskEntry->PendingQuery)
    {
        if (Context->DiskPathLabel)
            PhSetWindowText(Context->DiskPathLabel, L"Pending...");
        if (Context->DiskNameLabel)
            PhSetWindowText(Context->DiskNameLabel, L"Pending...");
    }
    else
    {
        if (Context->DiskPathLabel)
            PhSetWindowText(Context->DiskPathLabel, PhGetStringOrDefault(Context->DiskEntry->DiskIndexName, L"Unknown"));
        if (Context->DiskNameLabel)
            PhSetWindowText(Context->DiskNameLabel, PhGetStringOrDefault(Context->DiskEntry->DiskName, L"Unknown"));
    }
}

VOID DiskDeviceUpdateDeviceMountPoints(
    _In_ PDV_DISK_ENTRY DiskEntry
    )
{
    DiskDeviceUpdateDeviceInfo(NULL, DiskEntry);

    if (DiskEntry->DiskIndex != ULONG_MAX)
    {
        // Query the disk DosDevices mount points.
        PPH_STRING diskMountPoints = PH_AUTO_T(PH_STRING, DiskDriveQueryDosMountPoints(DiskEntry->DiskIndex));

        if (!PhIsNullOrEmptyString(diskMountPoints))
        {
            PH_FORMAT format[5];

            // Disk %lu (%s)
            PhInitFormatS(&format[0], L"Disk ");
            PhInitFormatU(&format[1], DiskEntry->DiskIndex);
            PhInitFormatS(&format[2], L" (");
            PhInitFormatSR(&format[3], diskMountPoints->sr);
            PhInitFormatC(&format[4], L')');

            PhMoveReference(&DiskEntry->DiskIndexName, PhFormat(format, RTL_NUMBER_OF(format), 0));
        }
        else
        {
            PH_FORMAT format[2];

            // Disk %lu
            PhInitFormatS(&format[0], L"Disk ");
            PhInitFormatU(&format[1], DiskEntry->DiskIndex);

            PhMoveReference(&DiskEntry->DiskIndexName, PhFormat(format, RTL_NUMBER_OF(format), 0));
        }
    }
}

INT_PTR CALLBACK DiskDevicePanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_DISK_SYSINFO_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PDV_DISK_SYSINFO_CONTEXT)lParam;

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
            context->DiskDevicePanelReadLabel = GetDlgItem(hwndDlg, IDC_STAT_BREAD);
            context->DiskDevicePanelWriteLabel = GetDlgItem(hwndDlg, IDC_STAT_BWRITE);
            context->DiskDevicePanelTotalLabel = GetDlgItem(hwndDlg, IDC_STAT_BTOTAL);
            context->DiskDevicePanelActiveLabel = GetDlgItem(hwndDlg, IDC_STAT_ACTIVE);
            context->DiskDevicePanelTimeLabel = GetDlgItem(hwndDlg, IDC_STAT_RESPONSETIME);
            context->DiskDevicePanelBytesLabel = GetDlgItem(hwndDlg, IDC_STAT_BYTESDELTA);
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
                ShowDiskDeviceDetailsDialog(context);
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

VOID DiskDeviceUpdateDialogDpi(
    _In_ PDV_DISK_SYSINFO_CONTEXT Context
    )
{
    Context->GraphPadding = PhGetDpi(RAPL_GRAPH_PADDING, Context->SysinfoSection->Parameters->WindowDpi);
}

VOID DiskDeviceCreateGraphs(
    _Inout_ PDV_DISK_SYSINFO_CONTEXT Context
    )
{
    PhInitializeGraphState(&Context->GraphReadState);
    PhInitializeGraphState(&Context->GraphWriteState);

    Context->GraphReadHandle = CreateWindow(
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
    Graph_SetTooltip(Context->GraphReadHandle, TRUE);

    Context->GraphWriteHandle = CreateWindow(
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
    Graph_SetTooltip(Context->GraphWriteHandle, TRUE);

    Context->LabelReadHandle = GetDlgItem(Context->WindowHandle, IDC_UP_L);
    Context->LabelWriteHandle = GetDlgItem(Context->WindowHandle, IDC_DOWN_L);
}

VOID DiskDeviceUpdateGraphs(
    _Inout_ PDV_DISK_SYSINFO_CONTEXT Context
    )
{
    Context->GraphReadState.Valid = FALSE;
    Context->GraphReadState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->GraphWriteHandle, 1);
    Graph_Draw(Context->GraphWriteHandle);
    Graph_UpdateTooltip(Context->GraphWriteHandle);
    InvalidateRect(Context->GraphWriteHandle, NULL, FALSE);

    Context->GraphWriteState.Valid = FALSE;
    Context->GraphWriteState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->GraphReadHandle, 1);
    Graph_Draw(Context->GraphReadHandle);
    Graph_UpdateTooltip(Context->GraphReadHandle);
    InvalidateRect(Context->GraphReadHandle, NULL, FALSE);
}

VOID DiskDeviceLayoutGraphs(
    _Inout_ PDV_DISK_SYSINFO_CONTEXT Context
    )
{
    RECT clientRect;
    RECT labelRect;
    RECT margin;
    ULONG graphWidth;
    ULONG graphHeight;
    HDWP deferHandle;
    ULONG y;

    Context->GraphReadState.Valid = FALSE;
    Context->GraphReadState.TooltipIndex = ULONG_MAX;
    Context->GraphWriteState.Valid = FALSE;
    Context->GraphWriteState.TooltipIndex = ULONG_MAX;

    margin = Context->GraphMargin;
    PhGetSizeDpiValue(&margin, Context->SysinfoSection->Parameters->WindowDpi, TRUE);

    GetClientRect(Context->WindowHandle, &clientRect);
    GetClientRect(Context->LabelWriteHandle, &labelRect);
    graphWidth = clientRect.right - margin.left - margin.right;
    graphHeight = (clientRect.bottom - margin.top - margin.bottom - labelRect.bottom * 2 - Context->GraphPadding * 3) / 2;

    deferHandle = BeginDeferWindowPos(4);
    y = margin.top;

    deferHandle = DeferWindowPos(
        deferHandle,
        Context->LabelReadHandle,
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
        Context->GraphReadHandle,
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
        Context->LabelWriteHandle,
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
        Context->GraphWriteHandle,
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

VOID DiskDeviceNotifyReadGraph(
    _Inout_ PDV_DISK_SYSINFO_CONTEXT Context,
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
                &Context->GraphWriteState,
                getDrawInfo,
                Context->DiskEntry->ReadBuffer.Count
                );

            if (!Context->GraphWriteState.Valid)
            {
                ULONG i;
                FLOAT max = 4 * 1024; // Minimum scaling 4KB

                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data1;

                    Context->GraphWriteState.Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&Context->DiskEntry->ReadBuffer, i);

                    if (max < data1)
                        max = data1;
                }

                if (max != 0)
                {
                    // Scale the data.

                    PhDivideSinglesBySingle(
                        Context->GraphWriteState.Data1,
                        max,
                        drawInfo->LineDataCount
                        );
                }

                drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                drawInfo->LabelYFunctionParameter = max;

                Context->GraphWriteState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (Context->GraphWriteState.TooltipIndex != getTooltipText->Index)
                {
                    ULONG64 value;
                    PH_FORMAT format[4];

                    value = PhGetItemCircularBuffer_ULONG64(
                        &Context->DiskEntry->ReadBuffer,
                        getTooltipText->Index
                        );

                    // R: %s\nW: %s\n%s
                    PhInitFormatS(&format[0], L"R: ");
                    PhInitFormatSize(&format[1], value);
                    PhInitFormatC(&format[2], L'\n');
                    PhInitFormatSR(&format[3], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&Context->GraphWriteState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                getTooltipText->Text = PhGetStringRef(Context->GraphWriteState.TooltipText);
            }
        }
        break;
    }
}

VOID DiskDeviceNotifyWriteGraph(
    _Inout_ PDV_DISK_SYSINFO_CONTEXT Context,
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
                &Context->GraphReadState,
                getDrawInfo,
                Context->DiskEntry->WriteBuffer.Count
                );

            if (!Context->GraphReadState.Valid)
            {
                ULONG i;
                FLOAT max = 4 * 1024; // Minimum scaling 4KB

                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data1;

                    Context->GraphReadState.Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&Context->DiskEntry->WriteBuffer, i);

                    if (max < data1)
                        max = data1;
                }

                if (max != 0)
                {
                    // Scale the data.

                    PhDivideSinglesBySingle(
                        Context->GraphReadState.Data1,
                        max,
                        drawInfo->LineDataCount
                        );
                }

                drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                drawInfo->LabelYFunctionParameter = max;

                Context->GraphReadState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (Context->GraphReadState.TooltipIndex != getTooltipText->Index)
                {
                    ULONG64 value;
                    PH_FORMAT format[4];

                    value = PhGetItemCircularBuffer_ULONG64(
                        &Context->DiskEntry->WriteBuffer,
                        getTooltipText->Index
                        );

                    // R: %s\nW: %s\n%s
                    PhInitFormatS(&format[0], L"W: ");
                    PhInitFormatSize(&format[1], value);
                    PhInitFormatC(&format[2], L'\n');
                    PhInitFormatSR(&format[3], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&Context->GraphReadState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                getTooltipText->Text = PhGetStringRef(Context->GraphReadState.TooltipText);
            }
        }
        break;
    }
}

VOID DiskDeviceTickDialog(
    _Inout_ PDV_DISK_SYSINFO_CONTEXT Context
    )
{
    DiskDeviceUpdateGraphs(Context);
    DiskDeviceUpdatePanel(Context);
}

INT_PTR CALLBACK DiskDeviceDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_DISK_SYSINFO_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PDV_DISK_SYSINFO_CONTEXT)lParam;

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
            context->DiskPathLabel = GetDlgItem(hwndDlg, IDC_TITLE);
            context->DiskNameLabel = GetDlgItem(hwndDlg, IDC_DEVICENAME);

            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, context->DiskPathLabel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            PhAddLayoutItem(&context->LayoutManager, context->DiskNameLabel, NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_TOP | PH_LAYOUT_FORCE_INVALIDATE);
            graphItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_PANEL_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            context->GraphMargin = graphItem->Margin;

            SetWindowFont(context->DiskPathLabel, context->SysinfoSection->Parameters->LargeFont, FALSE);
            SetWindowFont(context->DiskNameLabel, context->SysinfoSection->Parameters->MediumFont, FALSE);
            PhSetWindowText(context->DiskPathLabel, PhGetStringOrDefault(context->DiskEntry->DiskIndexName, L"Unknown"));
            PhSetWindowText(context->DiskNameLabel, PhGetStringOrDefault(context->DiskEntry->DiskName, L"Unknown"));

            context->PanelWindowHandle = PhCreateDialog(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_DISKDRIVE_PANEL), hwndDlg, DiskDevicePanelDialogProc, context);
            ShowWindow(context->PanelWindowHandle, SW_SHOW);
            PhAddLayoutItemEx(&context->LayoutManager, context->PanelWindowHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, panelItem->Margin);

            DiskDeviceUpdateDialogDpi(context);
            DiskDeviceUpdateTitle(context);
            DiskDeviceCreateGraphs(context);
            DiskDeviceUpdateGraphs(context);
            DiskDeviceUpdatePanel(context);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);

            PhDeleteGraphState(&context->GraphWriteState);
            PhDeleteGraphState(&context->GraphReadState);

            if (context->GraphReadHandle)
                DestroyWindow(context->GraphReadHandle);
            if (context->GraphWriteHandle)
                DestroyWindow(context->GraphWriteHandle);
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
            DiskDeviceUpdateDialogDpi(context);

            if (context->SysinfoSection->Parameters->LargeFont)
            {
                SetWindowFont(context->DiskPathLabel, context->SysinfoSection->Parameters->LargeFont, FALSE);
            }

            if (context->SysinfoSection->Parameters->MediumFont)
            {
                SetWindowFont(context->DiskNameLabel, context->SysinfoSection->Parameters->MediumFont, FALSE);
            }

            PhLayoutManagerLayout(&context->LayoutManager);
            DiskDeviceLayoutGraphs(context);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
            DiskDeviceLayoutGraphs(context);
        }
        break;
    case WM_NOTIFY:
        {
            NMHDR* header = (NMHDR*)lParam;

            if (header->hwndFrom == context->GraphReadHandle)
            {
                DiskDeviceNotifyReadGraph(context, header);
            }
            else if (header->hwndFrom == context->GraphWriteHandle)
            {
                DiskDeviceNotifyWriteGraph(context, header);
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

NTSTATUS DiskDeviceQueryNameWorkQueueItem(
    _In_ PDV_DISK_ENTRY DiskEntry
    )
{
    // Update the device index and mount points (dmex)
    DiskDeviceUpdateDeviceMountPoints(DiskEntry);

#ifdef FORCE_DELAY_LABEL_WORKQUEUE
    PhDelayExecution(4000);
#endif

    InterlockedExchange(&DiskEntry->JustProcessed, TRUE);
    DiskEntry->PendingQuery = FALSE;

    PhDereferenceObject(DiskEntry);
    return STATUS_SUCCESS;
}

VOID DiskDeviceQueueNameUpdate(
    _In_ PDV_DISK_ENTRY DiskEntry
    )
{
    DiskEntry->PendingQuery = TRUE;

    PhReferenceObject(DiskEntry);
    PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), DiskDeviceQueryNameWorkQueueItem, DiskEntry);
}

BOOLEAN DiskDeviceSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2
    )
{
    PDV_DISK_SYSINFO_CONTEXT context = (PDV_DISK_SYSINFO_CONTEXT)Section->Context;

    switch (Message)
    {
    case SysInfoCreate:
        {
            DiskDeviceQueueNameUpdate(context->DiskEntry);
        }
        return TRUE;
    case SysInfoDestroy:
        {
            PhDereferenceObject(context->DiskEntry);
            PhFree(context);
        }
        return TRUE;
    case SysInfoTick:
        {
            if (context->WindowHandle)
            {
                if (context->DiskEntry->JustProcessed)
                {
                    DiskDeviceUpdateTitle(context);
                    InterlockedExchange(&context->DiskEntry->JustProcessed, FALSE);
                }

                DiskDeviceTickDialog(context);
            }
        }
        return TRUE;
    case SysInfoViewChanging:
        {
            PH_SYSINFO_VIEW_TYPE view = (PH_SYSINFO_VIEW_TYPE)PtrToUlong(Parameter1);
            PPH_SYSINFO_SECTION section = (PPH_SYSINFO_SECTION)Parameter2;

            if (view == SysInfoSummaryView || section != Section)
                return TRUE;

            if (context->GraphReadHandle)
            {
                context->GraphReadState.Valid = FALSE;
                context->GraphReadState.TooltipIndex = ULONG_MAX;
                Graph_Draw(context->GraphReadHandle);
            }

            if (context->GraphWriteHandle)
            {
                context->GraphWriteState.Valid = FALSE;
                context->GraphWriteState.TooltipIndex = ULONG_MAX;
                Graph_Draw(context->GraphWriteHandle);
            }
        }
        return TRUE;
    case SysInfoCreateDialog:
        {
            PPH_SYSINFO_CREATE_DIALOG createDialog = (PPH_SYSINFO_CREATE_DIALOG)Parameter1;

            createDialog->Instance = PluginInstance->DllBase;
            createDialog->Template = MAKEINTRESOURCE(IDD_DISKDRIVE_DIALOG);
            createDialog->DialogProc = DiskDeviceDialogProc;
            createDialog->Parameter = context;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = (PPH_GRAPH_DRAW_INFO)Parameter1;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y | PH_GRAPH_USE_LINE_2;
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), PhGetIntegerSetting(L"ColorIoWrite"), context->SysinfoSection->Parameters->WindowDpi);
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, context->DiskEntry->ReadBuffer.Count);

            if (!Section->GraphState.Valid)
            {
                FLOAT max = 1024 * 1024; // minimum scaling of 1 MB.

                for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data1;
                    FLOAT data2;

                    Section->GraphState.Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->DiskEntry->ReadBuffer, i);
                    Section->GraphState.Data2[i] = data2 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->DiskEntry->WriteBuffer, i);

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

                    // Scale the data.
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
            ULONG64 diskReadValue;
            ULONG64 diskWriteValue;
            PH_FORMAT format[6];

            diskReadValue = PhGetItemCircularBuffer_ULONG64(
                &context->DiskEntry->ReadBuffer,
                getTooltipText->Index
                );

            diskWriteValue = PhGetItemCircularBuffer_ULONG64(
                &context->DiskEntry->WriteBuffer,
                getTooltipText->Index
                );

            // R: %s\nW: %s\n%s
            PhInitFormatS(&format[0], L"R: ");
            PhInitFormatSize(&format[1], diskReadValue);
            PhInitFormatS(&format[2], L"\nW: ");
            PhInitFormatSize(&format[3], diskWriteValue);
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

            if (context->DiskEntry->PendingQuery)
                PhMoveReference(&drawPanel->Title, PhCreateString(L"Pending..."));
            else
                PhSetReference(&drawPanel->Title, context->DiskEntry->DiskIndexName);

            if (!drawPanel->Title)
                drawPanel->Title = PhCreateString(L"Unknown");

            // R: %s\nW: %s
            PhInitFormatS(&format[0], L"R: ");
            PhInitFormatSize(&format[1], context->DiskEntry->BytesReadDelta.Delta);
            PhInitFormatS(&format[2], L"\nW: ");
            PhInitFormatSize(&format[3], context->DiskEntry->BytesWrittenDelta.Delta);

            drawPanel->SubTitle = PhFormat(format, RTL_NUMBER_OF(format), 0);
        }
        return TRUE;
    }

    return FALSE;
}

VOID DiskDeviceSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers,
    _In_ _Assume_refs_(1) PDV_DISK_ENTRY DiskEntry
    )
{
    static PH_STRINGREF text = PH_STRINGREF_INIT(L"Unknown");
    PDV_DISK_SYSINFO_CONTEXT context;
    PH_SYSINFO_SECTION section;

    context = PhAllocateZero(sizeof(DV_DISK_SYSINFO_CONTEXT));
    context->DiskEntry = PhReferenceObject(DiskEntry);
    context->DiskEntry->PendingQuery = TRUE;

    memset(&section, 0, sizeof(PH_SYSINFO_SECTION));
    section.Context = context;
    section.Callback = DiskDeviceSectionCallback;
    section.Name = text;

    context->SysinfoSection = Pointers->CreateSection(&section);
}
