/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2021-2023
 *
 */

#include "devices.h"

INT_PTR CALLBACK RaplDevicePanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_RAPL_SYSINFO_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PDV_RAPL_SYSINFO_CONTEXT)lParam;

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_NCDESTROY)
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context->RaplDeviceProcessorUsageLabel = GetDlgItem(hwndDlg, IDC_PACKAGE_L);
            context->RaplDeviceCoreUsageLabel = GetDlgItem(hwndDlg, IDC_CORE_L);
            context->RaplDeviceDimmUsageLabel = GetDlgItem(hwndDlg, IDC_DIMM_L);
            context->RaplDeviceGpuLimitLabel = GetDlgItem(hwndDlg, IDC_GPUDISCRETE_L);
            context->RaplDeviceComponentUsageLabel = GetDlgItem(hwndDlg, IDC_CPUCOMP_L);
            context->RaplDeviceTotalUsageLabel = GetDlgItem(hwndDlg, IDC_TOTALPOWER_L);
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

VOID RaplDeviceInitializeDialogDpi(
    _In_ PDV_RAPL_SYSINFO_CONTEXT Context
    )
{
    Context->GraphPadding = PhGetDpi(RAPL_GRAPH_PADDING, Context->SysinfoSection->Parameters->WindowDpi);
}

VOID RaplDeviceInitializeGraphStates(
    _Inout_ PDV_RAPL_SYSINFO_CONTEXT Context
    )
{
    PhInitializeGraphState(&Context->ProcessorGraphState);
    PhInitializeGraphState(&Context->CoreGraphState);
    PhInitializeGraphState(&Context->DimmGraphState);
    PhInitializeGraphState(&Context->TotalGraphState);
}

VOID RaplDeviceCreateGraphs(
    _Inout_ PDV_RAPL_SYSINFO_CONTEXT Context
    )
{
    Context->ProcessorGraphHandle = CreateWindow(
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
    Graph_SetTooltip(Context->ProcessorGraphHandle, TRUE);

    Context->CoreGraphHandle = CreateWindow(
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
    Graph_SetTooltip(Context->CoreGraphHandle, TRUE);

    Context->DimmGraphHandle = CreateWindow(
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
    Graph_SetTooltip(Context->DimmGraphHandle, TRUE);

    Context->TotalGraphHandle = CreateWindow(
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
    Graph_SetTooltip(Context->TotalGraphHandle, TRUE);

    Context->PackageGraphLabelHandle = GetDlgItem(Context->WindowHandle, IDC_PACKAGE_L);
    Context->CoreGraphLabelHandle = GetDlgItem(Context->WindowHandle, IDC_CORE_L);
    Context->DimmGraphLabelHandle = GetDlgItem(Context->WindowHandle, IDC_DIMM_L);
    Context->TotalGraphLabelHandle = GetDlgItem(Context->WindowHandle, IDC_TOTAL_L);
}

VOID RaplDeviceUpdateGraphs(
    _Inout_ PDV_RAPL_SYSINFO_CONTEXT Context
    )
{
    Context->ProcessorGraphState.Valid = FALSE;
    Context->ProcessorGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->ProcessorGraphHandle, 1);
    Graph_Draw(Context->ProcessorGraphHandle);
    Graph_UpdateTooltip(Context->ProcessorGraphHandle);
    InvalidateRect(Context->ProcessorGraphHandle, NULL, FALSE);

    Context->CoreGraphState.Valid = FALSE;
    Context->CoreGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->CoreGraphHandle, 1);
    Graph_Draw(Context->CoreGraphHandle);
    Graph_UpdateTooltip(Context->CoreGraphHandle);
    InvalidateRect(Context->CoreGraphHandle, NULL, FALSE);

    Context->DimmGraphState.Valid = FALSE;
    Context->DimmGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->DimmGraphHandle, 1);
    Graph_Draw(Context->DimmGraphHandle);
    Graph_UpdateTooltip(Context->DimmGraphHandle);
    InvalidateRect(Context->DimmGraphHandle, NULL, FALSE);

    Context->TotalGraphState.Valid = FALSE;
    Context->TotalGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->TotalGraphHandle, 1);
    Graph_Draw(Context->TotalGraphHandle);
    Graph_UpdateTooltip(Context->TotalGraphHandle);
    InvalidateRect(Context->TotalGraphHandle, NULL, FALSE);
}

VOID RaplDeviceUpdatePanel(
    _Inout_ PDV_RAPL_SYSINFO_CONTEXT Context
    )
{
    PH_FORMAT format[2];
    WCHAR formatBuffer[512];

    PhInitFormatF(&format[0], Context->DeviceEntry->CurrentProcessorPower, 2);
    PhInitFormatS(&format[1], L" W");

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->RaplDeviceProcessorUsageLabel, formatBuffer);
    else
        PhSetWindowText(Context->RaplDeviceProcessorUsageLabel, L"N/A");

    PhInitFormatF(&format[0], Context->DeviceEntry->CurrentCorePower, 2);
    PhInitFormatS(&format[1], L" W");

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->RaplDeviceCoreUsageLabel, formatBuffer);
    else
        PhSetWindowText(Context->RaplDeviceCoreUsageLabel, L"N/A");

    PhInitFormatF(&format[0], Context->DeviceEntry->CurrentDramPower, 2);
    PhInitFormatS(&format[1], L" W");

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->RaplDeviceDimmUsageLabel, formatBuffer);
    else
        PhSetWindowText(Context->RaplDeviceDimmUsageLabel, L"N/A");

    PhInitFormatF(&format[0], Context->DeviceEntry->CurrentDiscreteGpuPower, 2);
    PhInitFormatS(&format[1], L" W");

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->RaplDeviceGpuLimitLabel, formatBuffer);
    else
        PhSetWindowText(Context->RaplDeviceGpuLimitLabel, L"N/A");

    PhInitFormatF(&format[0], Context->DeviceEntry->CurrentComponentPower, 2);
    PhInitFormatS(&format[1], L" W");

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->RaplDeviceComponentUsageLabel, formatBuffer);
    else
        PhSetWindowText(Context->RaplDeviceComponentUsageLabel, L"N/A");

    PhInitFormatF(&format[0], Context->DeviceEntry->CurrentTotalPower, 2);
    PhInitFormatS(&format[1], L" W");

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->RaplDeviceTotalUsageLabel, formatBuffer);
    else
        PhSetWindowText(Context->RaplDeviceTotalUsageLabel, L"N/A");
}

VOID RaplDeviceLayoutGraphs(
    _Inout_ PDV_RAPL_SYSINFO_CONTEXT Context
    )
{
    RECT clientRect;
    RECT labelRect;
    RECT margin;
    ULONG graphWidth;
    ULONG graphHeight;
    HDWP deferHandle;
    ULONG y;

    Context->ProcessorGraphState.Valid = FALSE;
    Context->ProcessorGraphState.TooltipIndex = ULONG_MAX;
    Context->CoreGraphState.Valid = FALSE;
    Context->CoreGraphState.TooltipIndex = ULONG_MAX;
    Context->DimmGraphState.Valid = FALSE;
    Context->DimmGraphState.TooltipIndex = ULONG_MAX;
    Context->TotalGraphState.Valid = FALSE;
    Context->TotalGraphState.TooltipIndex = ULONG_MAX;

    margin = Context->GraphMargin;
    PhGetSizeDpiValue(&margin, Context->SysinfoSection->Parameters->WindowDpi, TRUE);

    GetClientRect(Context->WindowHandle, &clientRect);
    GetClientRect(Context->PackageGraphLabelHandle, &labelRect);
    graphWidth = clientRect.right - margin.left - margin.right;
    graphHeight = (clientRect.bottom - margin.top - margin.bottom - labelRect.bottom * 4 - Context->GraphPadding * 5) / 4;

    deferHandle = BeginDeferWindowPos(8);
    y = margin.top;

    deferHandle = DeferWindowPos(
        deferHandle,
        Context->PackageGraphLabelHandle,
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
        Context->ProcessorGraphHandle,
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
        Context->CoreGraphLabelHandle,
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
        Context->CoreGraphHandle,
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
        Context->DimmGraphLabelHandle,
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
        Context->DimmGraphHandle,
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
        Context->TotalGraphLabelHandle,
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
        Context->TotalGraphHandle,
        NULL,
        margin.left,
        y,
        graphWidth,
        clientRect.bottom - margin.bottom - y,
        SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += graphHeight + Context->GraphPadding;

    EndDeferWindowPos(deferHandle);
}

PPH_STRING RaplGraphSingleLabelYFunction(
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ ULONG DataIndex,
    _In_ FLOAT Value,
    _In_ FLOAT Parameter
    )
{
    DOUBLE value = (DOUBLE)Value * (DOUBLE)Parameter;

    if (value != 0)
    {
        PH_FORMAT format[2];

        PhInitFormatF(&format[0], value, 2);
        PhInitFormatS(&format[1], L" W");

        return PhFormat(format, RTL_NUMBER_OF(format), 0);
    }
    else
    {
        return PhReferenceEmptyString();
    }
}

VOID RaplDeviceNotifyProcessorGraph(
    _Inout_ PDV_RAPL_SYSINFO_CONTEXT Context,
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
            Context->SysinfoSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), 0, Context->SysinfoSection->Parameters->WindowDpi);
            PhGraphStateGetDrawInfo(&Context->ProcessorGraphState, getDrawInfo, Context->DeviceEntry->PackageBuffer.Count);

            if (!Context->ProcessorGraphState.Valid)
            {
                FLOAT max = 0;

                PhCopyCircularBuffer_FLOAT(&Context->DeviceEntry->PackageBuffer, Context->ProcessorGraphState.Data1, drawInfo->LineDataCount);

                for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data = Context->ProcessorGraphState.Data1[i];

                    if (max < data)
                        max = data;
                }

                if (max != 0)
                {
                    PhDivideSinglesBySingle(Context->ProcessorGraphState.Data1, max, drawInfo->LineDataCount);
                }

                drawInfo->LabelYFunction = RaplGraphSingleLabelYFunction;
                drawInfo->LabelYFunctionParameter = max;

                Context->ProcessorGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (Context->ProcessorGraphState.TooltipIndex != getTooltipText->Index)
                {
                    FLOAT value;
                    PH_FORMAT format[3];

                    value = PhGetItemCircularBuffer_FLOAT(&Context->DeviceEntry->PackageBuffer, getTooltipText->Index);

                    // %.2f W\n%s
                    PhInitFormatF(&format[0], value, 2);
                    PhInitFormatS(&format[1], L" W\n");
                    PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&Context->ProcessorGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                getTooltipText->Text = PhGetStringRef(Context->ProcessorGraphState.TooltipText);
            }
        }
        break;
    }
}

VOID RaplDeviceNotifyPackageGraph(
    _Inout_ PDV_RAPL_SYSINFO_CONTEXT Context,
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
            Context->SysinfoSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorPhysical"), 0, Context->SysinfoSection->Parameters->WindowDpi);
            PhGraphStateGetDrawInfo(&Context->CoreGraphState, getDrawInfo, Context->DeviceEntry->CoreBuffer.Count);

            if (!Context->CoreGraphState.Valid)
            {
                FLOAT max = 0;

                PhCopyCircularBuffer_FLOAT(&Context->DeviceEntry->CoreBuffer, Context->CoreGraphState.Data1, drawInfo->LineDataCount);

                for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data = Context->CoreGraphState.Data1[i];

                    if (max < data)
                        max = data;
                }

                if (max != 0)
                {
                    PhDivideSinglesBySingle(Context->CoreGraphState.Data1, max, drawInfo->LineDataCount);
                }

                drawInfo->LabelYFunction = RaplGraphSingleLabelYFunction;
                drawInfo->LabelYFunctionParameter = max;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (Context->CoreGraphState.TooltipIndex != getTooltipText->Index)
                {
                    FLOAT value;
                    PH_FORMAT format[3];

                    value = PhGetItemCircularBuffer_FLOAT(&Context->DeviceEntry->CoreBuffer, getTooltipText->Index);

                    // %.2f W\%s
                    PhInitFormatF(&format[0], value, 2);
                    PhInitFormatS(&format[1], L" W\n");
                    PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&Context->CoreGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                getTooltipText->Text = PhGetStringRef(Context->CoreGraphState.TooltipText);
            }
        }
        break;
    }
}

VOID RaplDeviceNotifyDimmGraph(
    _Inout_ PDV_RAPL_SYSINFO_CONTEXT Context,
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
            PhGraphStateGetDrawInfo(&Context->DimmGraphState, getDrawInfo, Context->DeviceEntry->DimmBuffer.Count);

            if (!Context->DimmGraphState.Valid)
            {
                FLOAT max = 0;

                PhCopyCircularBuffer_FLOAT(&Context->DeviceEntry->DimmBuffer, Context->DimmGraphState.Data1, drawInfo->LineDataCount);

                for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data = Context->DimmGraphState.Data1[i];

                    if (max < data)
                        max = data;
                }

                if (max != 0)
                {
                    PhDivideSinglesBySingle(Context->DimmGraphState.Data1, max, drawInfo->LineDataCount);
                }

                drawInfo->LabelYFunction = RaplGraphSingleLabelYFunction;
                drawInfo->LabelYFunctionParameter = max;

                Context->DimmGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (Context->DimmGraphState.TooltipIndex != getTooltipText->Index)
                {
                    FLOAT value;
                    PH_FORMAT format[3];

                    value = PhGetItemCircularBuffer_FLOAT(&Context->DeviceEntry->DimmBuffer, getTooltipText->Index);

                    // %.2f W\%s
                    PhInitFormatF(&format[0], value, 2);
                    PhInitFormatS(&format[1], L" W\n");
                    PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&Context->DimmGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                getTooltipText->Text = PhGetStringRef(Context->DimmGraphState.TooltipText);
            }
        }
        break;
    }
}

VOID RaplDeviceNotifyTotalGraph(
    _Inout_ PDV_RAPL_SYSINFO_CONTEXT Context,
    _In_ NMHDR* Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y;
            Context->SysinfoSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorPrivate"), 0, Context->SysinfoSection->Parameters->WindowDpi);
            PhGraphStateGetDrawInfo(&Context->TotalGraphState, getDrawInfo, Context->DeviceEntry->TotalBuffer.Count);

            if (!Context->TotalGraphState.Valid)
            {
                FLOAT max = 0;

                PhCopyCircularBuffer_FLOAT(&Context->DeviceEntry->TotalBuffer, Context->TotalGraphState.Data1, drawInfo->LineDataCount);

                for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data = Context->TotalGraphState.Data1[i];

                    if (max < data)
                        max = data;
                }

                if (max != 0)
                {
                    PhDivideSinglesBySingle(Context->TotalGraphState.Data1, max, drawInfo->LineDataCount);
                }

                drawInfo->LabelYFunction = RaplGraphSingleLabelYFunction;
                drawInfo->LabelYFunctionParameter = max;

                Context->TotalGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (Context->TotalGraphState.TooltipIndex != getTooltipText->Index)
                {
                    FLOAT value;
                    PH_FORMAT format[3];

                    value = PhGetItemCircularBuffer_FLOAT(&Context->DeviceEntry->TotalBuffer, getTooltipText->Index);

                    // %.2f W\%s
                    PhInitFormatF(&format[0], value, 2);
                    PhInitFormatS(&format[1], L" W\n");
                    PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&Context->TotalGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                getTooltipText->Text = PhGetStringRef(Context->TotalGraphState.TooltipText);
            }
        }
        break;
    }
}

VOID RaplDeviceTickDialog(
    _Inout_ PDV_RAPL_SYSINFO_CONTEXT Context
    )
{
    RaplDeviceUpdateGraphs(Context);
    RaplDeviceUpdatePanel(Context);
}

INT_PTR CALLBACK RaplDeviceDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_RAPL_SYSINFO_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PDV_RAPL_SYSINFO_CONTEXT)lParam;

        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);

        if (uMsg == WM_NCDESTROY)
        {
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
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
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_DEVICENAME), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            graphItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_PANEL_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            context->GraphMargin = graphItem->Margin;

            RaplDeviceInitializeDialogDpi(context);

            SetWindowFont(GetDlgItem(hwndDlg, IDC_TITLE), context->SysinfoSection->Parameters->LargeFont, FALSE);
            SetWindowFont(GetDlgItem(hwndDlg, IDC_DEVICENAME), context->SysinfoSection->Parameters->MediumFont, FALSE);
            PhSetDialogItemText(hwndDlg, IDC_DEVICENAME, PhGetStringOrDefault(context->DeviceEntry->DeviceName, L"Unknown"));

            context->RaplDevicePanel = PhCreateDialog(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_RAPLDEVICE_PANEL), hwndDlg, RaplDevicePanelDialogProc, context);
            ShowWindow(context->RaplDevicePanel, SW_SHOW);
            PhAddLayoutItemEx(&context->LayoutManager, context->RaplDevicePanel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, panelItem->Margin);

            RaplDeviceInitializeGraphStates(context);
            RaplDeviceCreateGraphs(context);
            RaplDeviceUpdateGraphs(context);
            RaplDeviceUpdatePanel(context);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->LayoutManager);

            PhDeleteGraphState(&context->ProcessorGraphState);
            PhDeleteGraphState(&context->CoreGraphState);
            PhDeleteGraphState(&context->DimmGraphState);
            PhDeleteGraphState(&context->TotalGraphState);

            if (context->ProcessorGraphHandle)
                DestroyWindow(context->ProcessorGraphHandle);
            if (context->CoreGraphHandle)
                DestroyWindow(context->CoreGraphHandle);
            if (context->DimmGraphHandle)
                DestroyWindow(context->DimmGraphHandle);
            if (context->TotalGraphHandle)
                DestroyWindow(context->TotalGraphHandle);
            if (context->RaplDevicePanel)
                DestroyWindow(context->RaplDevicePanel);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            RaplDeviceInitializeDialogDpi(context);

            if (context->SysinfoSection->Parameters->LargeFont)
            {
                SetWindowFont(GetDlgItem(hwndDlg, IDC_TITLE), context->SysinfoSection->Parameters->LargeFont, FALSE);
            }

            if (context->SysinfoSection->Parameters->MediumFont)
            {
                SetWindowFont(GetDlgItem(hwndDlg, IDC_DEVICENAME), context->SysinfoSection->Parameters->MediumFont, FALSE);
            }

            PhLayoutManagerLayout(&context->LayoutManager);
            RaplDeviceLayoutGraphs(context);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);
            RaplDeviceLayoutGraphs(context);
        }
        break;
    case WM_NOTIFY:
        {
            NMHDR* header = (NMHDR*)lParam;

            if (header->hwndFrom == context->ProcessorGraphHandle)
            {
                RaplDeviceNotifyProcessorGraph(context, header);
            }
            else if (header->hwndFrom == context->CoreGraphHandle)
            {
                RaplDeviceNotifyPackageGraph(context, header);
            }
            else if (header->hwndFrom == context->DimmGraphHandle)
            {
                RaplDeviceNotifyDimmGraph(context, header);
            }
            else if (header->hwndFrom == context->TotalGraphHandle)
            {
                RaplDeviceNotifyTotalGraph(context, header);
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

BOOLEAN RaplDeviceSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2
    )
{
    PDV_RAPL_SYSINFO_CONTEXT context = (PDV_RAPL_SYSINFO_CONTEXT)Section->Context;

    switch (Message)
    {
    case SysInfoCreate:
        {
            if (PhIsNullOrEmptyString(context->DeviceEntry->DeviceName))
            {
                PPH_STRING deviceName;

                if (QueryRaplDeviceInterfaceDescription(PhGetString(context->DeviceEntry->Id.DevicePath), &deviceName))
                {
                    PhMoveReference(&context->DeviceEntry->DeviceName, deviceName);
                }
            }
        }
        return TRUE;
    case SysInfoDestroy:
        {
            PhDereferenceObject(context->DeviceEntry);
            PhFree(context);
        }
        return TRUE;
    case SysInfoTick:
        {
            if (context->WindowHandle)
            {
                RaplDeviceTickDialog(context);
            }
        }
        return TRUE;
    case SysInfoViewChanging:
        {
            PH_SYSINFO_VIEW_TYPE view = (PH_SYSINFO_VIEW_TYPE)Parameter1;
            PPH_SYSINFO_SECTION section = (PPH_SYSINFO_SECTION)Parameter2;

            if (view == SysInfoSummaryView || section != Section)
                return TRUE;

            if (context->ProcessorGraphHandle)
            {
                context->ProcessorGraphState.Valid = FALSE;
                context->ProcessorGraphState.TooltipIndex = ULONG_MAX;
                Graph_Draw(context->ProcessorGraphHandle);
            }

            if (context->CoreGraphHandle)
            {
                context->CoreGraphState.Valid = FALSE;
                context->CoreGraphState.TooltipIndex = ULONG_MAX;
                Graph_Draw(context->CoreGraphHandle);
            }

            if (context->DimmGraphHandle)
            {
                context->DimmGraphState.Valid = FALSE;
                context->DimmGraphState.TooltipIndex = ULONG_MAX;
                Graph_Draw(context->DimmGraphHandle);
            }

            if (context->TotalGraphHandle)
            {
                context->TotalGraphState.Valid = FALSE;
                context->TotalGraphState.TooltipIndex = ULONG_MAX;
                Graph_Draw(context->TotalGraphHandle);
            }
        }
        return TRUE;
    case SysInfoCreateDialog:
        {
            PPH_SYSINFO_CREATE_DIALOG createDialog = (PPH_SYSINFO_CREATE_DIALOG)Parameter1;

            createDialog->Instance = PluginInstance->DllBase;
            createDialog->Template = MAKEINTRESOURCE(IDD_RAPLDEVICE_DIALOG);
            createDialog->DialogProc = RaplDeviceDialogProc;
            createDialog->Parameter = context;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = (PPH_GRAPH_DRAW_INFO)Parameter1;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y;
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorPrivate"), 0, context->SysinfoSection->Parameters->WindowDpi);
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, context->DeviceEntry->TotalBuffer.Count);

            if (!Section->GraphState.Valid)
            {
                FLOAT max = 0;

                PhCopyCircularBuffer_FLOAT(&context->DeviceEntry->TotalBuffer, Section->GraphState.Data1, drawInfo->LineDataCount);

                for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data = Section->GraphState.Data1[i];

                    if (max < data)
                        max = data;
                }

                if (max != 0)
                {
                    PhDivideSinglesBySingle(Section->GraphState.Data1, max, drawInfo->LineDataCount);
                }

                drawInfo->LabelYFunction = RaplGraphSingleLabelYFunction;
                drawInfo->LabelYFunctionParameter = max;

                Section->GraphState.Valid = TRUE;
            }
        }
        return TRUE;
    case SysInfoGraphGetTooltipText:
        {
            PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT getTooltipText = (PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT)Parameter1;
            FLOAT value;
            PH_FORMAT format[3];

            value = PhGetItemCircularBuffer_FLOAT(&context->DeviceEntry->TotalBuffer, getTooltipText->Index);

            // %.2f W\%s
            PhInitFormatF(&format[0], value, 2);
            PhInitFormatS(&format[1], L" W\n");
            PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

            PhMoveReference(&Section->GraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
            getTooltipText->Text = PhGetStringRef(Section->GraphState.TooltipText);
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = (PPH_SYSINFO_DRAW_PANEL)Parameter1;
            PH_FORMAT format[2];

            drawPanel->Title = PhCreateString(L"RAPL");

            // %.2f W
            PhInitFormatF(&format[0], context->DeviceEntry->CurrentTotalPower, 2);
            PhInitFormatS(&format[1], L" W");

            drawPanel->SubTitle = PhFormat(format, RTL_NUMBER_OF(format), 0);
        }
        return TRUE;
    }

    return FALSE;
}

VOID RaplDeviceSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers,
    _In_ _Assume_refs_(1) PDV_RAPL_ENTRY DeviceEntry
    )
{
    static PH_STRINGREF text = PH_STRINGREF_INIT(L"RAPL");
    PDV_RAPL_SYSINFO_CONTEXT context;
    PH_SYSINFO_SECTION section;

    context = PhAllocateZero(sizeof(DV_RAPL_SYSINFO_CONTEXT));
    context->DeviceEntry = PhReferenceObject(DeviceEntry);

    memset(&section, 0, sizeof(PH_SYSINFO_SECTION));
    section.Context = context;
    section.Callback = RaplDeviceSectionCallback;
    section.Name = text;

    context->SysinfoSection = Pointers->CreateSection(&section);
}
