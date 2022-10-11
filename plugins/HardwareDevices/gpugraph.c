/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2022
 *
 */

#include "devices.h"

PPH_OBJECT_TYPE GraphicsSysinfoEntryType = NULL;

PPH_STRING GraphicsDeviceGetAdapterDescription(
    _In_ PPH_STRING DevicePath
    )
{
    ULONG deviceIndex;

    if (GraphicsQueryDeviceInterfaceAdapterIndex(
        PhGetString(DevicePath),
        &deviceIndex
        ))
    {
        SIZE_T returnLength;
        PH_FORMAT format[2];
        WCHAR formatBuffer[512];

        PhInitFormatS(&format[0], L"GPU ");
        PhInitFormatU(&format[1], deviceIndex);

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), &returnLength))
        {
            PH_STRINGREF text;

            text.Buffer = formatBuffer;
            text.Length = returnLength - sizeof(UNICODE_NULL);

            return PhCreateString2(&text);
        }
        else
        {
            return PhFormat(format, RTL_NUMBER_OF(format), 0);
        }
    }
    else
    {
        return PhCreateString(L"GPU");
    }
}

VOID GraphicsDeviceSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers,
    _In_ PDV_GPU_ENTRY DeviceEntry
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    PDV_GPU_SYSINFO_CONTEXT context;
    PH_SYSINFO_SECTION section;

    if (PhBeginInitOnce(&initOnce))
    {
        GraphicsSysinfoEntryType = PhCreateObjectType(L"GpuSysInfoContext", 0, NULL);
        PhEndInitOnce(&initOnce);
    }

    context = PhCreateObjectZero(sizeof(DV_GPU_SYSINFO_CONTEXT), GraphicsSysinfoEntryType);
    context->DeviceEntry = PhReferenceObject(DeviceEntry);

    memset(&section, 0, sizeof(PH_SYSINFO_SECTION));
    section.Context = context;
    section.Callback = GraphicsDeviceSectionCallback;
    section.Name = PhGetStringRef(context->DeviceEntry->Id.DevicePath);

    context->SysinfoSection = Pointers->CreateSection(&section);
}

VOID GraphicsDeviceInitializeDialog(
    _In_ PDV_GPU_SYSINFO_CONTEXT Context
    )
{
    PhInitializeGraphState(&Context->GpuGraphState);
    PhInitializeGraphState(&Context->DedicatedGraphState);
    PhInitializeGraphState(&Context->SharedGraphState);
    PhInitializeGraphState(&Context->PowerUsageGraphState);
    PhInitializeGraphState(&Context->TemperatureGraphState);
    PhInitializeGraphState(&Context->FanRpmGraphState);
}

VOID GraphicsDeviceUninitializeDialog(
    _In_ PDV_GPU_SYSINFO_CONTEXT Context
    )
{
    PhDeleteGraphState(&Context->GpuGraphState);
    PhDeleteGraphState(&Context->DedicatedGraphState);
    PhDeleteGraphState(&Context->SharedGraphState);
    PhDeleteGraphState(&Context->PowerUsageGraphState);
    PhDeleteGraphState(&Context->TemperatureGraphState);
    PhDeleteGraphState(&Context->FanRpmGraphState);
}

VOID GraphicsDeviceTickDialog(
    _In_ PDV_GPU_SYSINFO_CONTEXT Context
    )
{
    GraphicsDeviceUpdateGraphs(Context);
    GraphicsDeviceUpdatePanel(Context);
}

VOID GraphicsDeviceCreateGraphs(
    _In_ PDV_GPU_SYSINFO_CONTEXT Context
    )
{
    Context->GpuGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        Context->GpuDialog,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(Context->GpuGraphHandle, TRUE);

    Context->DedicatedGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        Context->GpuDialog,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(Context->DedicatedGraphHandle, TRUE);

    Context->SharedGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        Context->GpuDialog,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(Context->SharedGraphHandle, TRUE);

    //if (Context->EtGpuSupported)
    {
        Context->PowerUsageGraphHandle = CreateWindow(
            PH_GRAPH_CLASSNAME,
            NULL,
            WS_VISIBLE | WS_CHILD | WS_BORDER,
            0,
            0,
            3,
            3,
            Context->GpuDialog,
            NULL,
            NULL,
            NULL
            );
        Graph_SetTooltip(Context->PowerUsageGraphHandle, TRUE);

        Context->TemperatureGraphHandle = CreateWindow(
            PH_GRAPH_CLASSNAME,
            NULL,
            WS_VISIBLE | WS_CHILD | WS_BORDER,
            0,
            0,
            3,
            3,
            Context->GpuDialog,
            NULL,
            NULL,
            NULL
            );
        Graph_SetTooltip(Context->TemperatureGraphHandle, TRUE);

        Context->FanRpmGraphHandle = CreateWindow(
            PH_GRAPH_CLASSNAME,
            NULL,
            WS_VISIBLE | WS_CHILD | WS_BORDER,
            0,
            0,
            3,
            3,
            Context->GpuDialog,
            NULL,
            NULL,
            NULL
            );
        Graph_SetTooltip(Context->FanRpmGraphHandle, TRUE);
    }
}

VOID GraphicsDeviceLayoutGraphs(
    _In_ PDV_GPU_SYSINFO_CONTEXT Context
    )
{
    RECT clientRect;
    RECT labelRect;
    ULONG graphWidth;
    ULONG graphHeight;
    HDWP deferHandle;
    ULONG y;

    Context->GpuGraphState.Valid = FALSE;
    Context->GpuGraphState.TooltipIndex = ULONG_MAX;
    Context->DedicatedGraphState.Valid = FALSE;
    Context->DedicatedGraphState.TooltipIndex = ULONG_MAX;
    Context->SharedGraphState.Valid = FALSE;
    Context->SharedGraphState.TooltipIndex = ULONG_MAX;

    //if (EtGpuSupported)
    {
        Context->PowerUsageGraphState.Valid = FALSE;
        Context->PowerUsageGraphState.TooltipIndex = ULONG_MAX;
        Context->TemperatureGraphState.Valid = FALSE;
        Context->TemperatureGraphState.TooltipIndex = ULONG_MAX;
        Context->FanRpmGraphState.Valid = FALSE;
        Context->FanRpmGraphState.TooltipIndex = ULONG_MAX;
    }

    GetClientRect(Context->GpuDialog, &clientRect);
    GetClientRect(GetDlgItem(Context->GpuDialog, IDC_GPU_L), &labelRect);
    graphWidth = clientRect.right - Context->GpuGraphMargin.left - Context->GpuGraphMargin.right;

    //if (EtGpuSupported)
        graphHeight = (clientRect.bottom - Context->GpuGraphMargin.top - Context->GpuGraphMargin.bottom - labelRect.bottom * 6 - GPU_GRAPH_PADDING * 8) / 6;
    //else
    //    graphHeight = (clientRect.bottom - Context->GpuGraphMargin.top - Context->GpuGraphMargin.bottom - labelRect.bottom * 3 - ET_GPU_PADDING * 5) / 3;


    deferHandle = BeginDeferWindowPos(12);
    y = Context->GpuGraphMargin.top;

    deferHandle = DeferWindowPos(
        deferHandle,
        GetDlgItem(Context->GpuDialog, IDC_GPU_L),
        NULL,
        Context->GpuGraphMargin.left,
        y,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += labelRect.bottom + GPU_GRAPH_PADDING;

    deferHandle = DeferWindowPos(
        deferHandle,
        Context->GpuGraphHandle,
        NULL,
        Context->GpuGraphMargin.left,
        y,
        graphWidth,
        graphHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += graphHeight + GPU_GRAPH_PADDING;

    deferHandle = DeferWindowPos(
        deferHandle,
        GetDlgItem(Context->GpuDialog, IDC_DEDICATED_L),
        NULL,
        Context->GpuGraphMargin.left,
        y,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += labelRect.bottom + GPU_GRAPH_PADDING;

    deferHandle = DeferWindowPos(
        deferHandle,
        Context->DedicatedGraphHandle,
        NULL,
        Context->GpuGraphMargin.left,
        y,
        graphWidth,
        graphHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += graphHeight + GPU_GRAPH_PADDING;

    deferHandle = DeferWindowPos(
        deferHandle,
        GetDlgItem(Context->GpuDialog, IDC_SHARED_L),
        NULL,
        Context->GpuGraphMargin.left,
        y,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += labelRect.bottom + GPU_GRAPH_PADDING;

    deferHandle = DeferWindowPos(
        deferHandle,
        Context->SharedGraphHandle,
        NULL,
        Context->GpuGraphMargin.left,
        y,
        graphWidth,
        graphHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += graphHeight + GPU_GRAPH_PADDING;

    //if (EtGpuSupported)
    {
        deferHandle = DeferWindowPos(
            deferHandle,
            GetDlgItem(Context->GpuDialog, IDC_POWER_USAGE_L),
            NULL,
            Context->GpuGraphMargin.left,
            y,
            0,
            0,
            SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
            );
        y += labelRect.bottom + GPU_GRAPH_PADDING;

        deferHandle = DeferWindowPos(
            deferHandle,
            Context->PowerUsageGraphHandle,
            NULL,
            Context->GpuGraphMargin.left,
            y,
            graphWidth,
            graphHeight,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
        y += graphHeight + GPU_GRAPH_PADDING;

        deferHandle = DeferWindowPos(
            deferHandle,
            GetDlgItem(Context->GpuDialog, IDC_TEMPERATURE_L),
            NULL,
            Context->GpuGraphMargin.left,
            y,
            0,
            0,
            SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
            );
        y += labelRect.bottom + GPU_GRAPH_PADDING;

        deferHandle = DeferWindowPos(
            deferHandle,
            Context->TemperatureGraphHandle,
            NULL,
            Context->GpuGraphMargin.left,
            y,
            graphWidth,
            graphHeight,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
        y += graphHeight + GPU_GRAPH_PADDING;

        deferHandle = DeferWindowPos(
            deferHandle,
            GetDlgItem(Context->GpuDialog, IDC_FAN_RPM_L),
            NULL,
            Context->GpuGraphMargin.left,
            y,
            0,
            0,
            SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
            );
        y += labelRect.bottom + GPU_GRAPH_PADDING;

        deferHandle = DeferWindowPos(
            deferHandle,
            Context->FanRpmGraphHandle,
            NULL,
            Context->GpuGraphMargin.left,
            y,
            graphWidth,
            clientRect.bottom - Context->GpuGraphMargin.bottom - y,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
    }

    EndDeferWindowPos(deferHandle);
}

VOID GraphicsDeviceNotifyGpuGraph(
    _In_ PDV_GPU_SYSINFO_CONTEXT Context,
    _In_ NMHDR *Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;
            LONG dpiValue;
            
            dpiValue = PhGetWindowDpi(Context->GpuGraphHandle);
                        
            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (GraphicsEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
            Context->SysinfoSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), 0, dpiValue);

            PhGraphStateGetDrawInfo(
                &Context->GpuGraphState,
                getDrawInfo,
                Context->DeviceEntry->GpuUsageHistory.Count
                );

            if (!Context->GpuGraphState.Valid)
            {
                PhCopyCircularBuffer_FLOAT(&Context->DeviceEntry->GpuUsageHistory, Context->GpuGraphState.Data1, drawInfo->LineDataCount);

                if (GraphicsEnableScaleGraph)
                {
                    FLOAT max = 0;

                    for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                    {
                        FLOAT data = Context->GpuGraphState.Data1[i];

                        if (max < data)
                            max = data;
                    }

                    if (max != 0)
                    {
                        PhDivideSinglesBySingle(Context->GpuGraphState.Data1, max, drawInfo->LineDataCount);
                    }

                    if (GraphicsEnableScaleText)
                    {
                        drawInfo->LabelYFunction = PhSiDoubleLabelYFunction;
                        drawInfo->LabelYFunctionParameter = max;
                    }
                }
                else
                {
                    if (GraphicsEnableScaleText)
                    {
                        drawInfo->LabelYFunction = PhSiDoubleLabelYFunction;
                        drawInfo->LabelYFunctionParameter = 1.0f;
                    }
                }

                Context->GpuGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (Context->GpuGraphState.TooltipIndex != getTooltipText->Index)
                {
                    FLOAT gpu;
                    PH_FORMAT format[3];

                    gpu = PhGetItemCircularBuffer_FLOAT(&Context->DeviceEntry->GpuUsageHistory, getTooltipText->Index);

                    // %.2f%%%s\n%s
                    PhInitFormatF(&format[0], gpu * 100, 2);
                    PhInitFormatS(&format[1], L"%\n");
                    PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&Context->GpuGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                getTooltipText->Text = PhGetStringRef(Context->GpuGraphState.TooltipText);
            }
        }
        break;
    }
}

VOID GraphicsDeviceNotifyDedicatedGraph(
    _In_ PDV_GPU_SYSINFO_CONTEXT Context,
    _In_ NMHDR *Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;
            ULONG i;
            LONG dpiValue;
            
            dpiValue = PhGetWindowDpi(Context->DedicatedGraphHandle);
            
            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (GraphicsEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
            Context->SysinfoSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorPrivate"), 0, dpiValue);

            PhGraphStateGetDrawInfo(
                &Context->DedicatedGraphState,
                getDrawInfo,
                Context->DeviceEntry->DedicatedHistory.Count
                );

            if (!Context->DedicatedGraphState.Valid)
            {
                FLOAT max = 0;

                if (Context->DeviceEntry->DedicatedLimit != 0 && !GraphicsEnableScaleGraph)
                {
                    max = (FLOAT)Context->DeviceEntry->DedicatedLimit;
                }

                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data = Context->DedicatedGraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG64(&Context->DeviceEntry->DedicatedHistory, i);

                    if (max < data)
                        max = data;
                }

                if (max != 0)
                {
                    PhDivideSinglesBySingle(Context->DedicatedGraphState.Data1, max, drawInfo->LineDataCount);
                }

                if (GraphicsEnableScaleText)
                {
                    drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                    drawInfo->LabelYFunctionParameter = max;
                }

                Context->DedicatedGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (Context->DedicatedGraphState.TooltipIndex != getTooltipText->Index)
                {
                    ULONG64 usedPages;
                    PH_FORMAT format[3];

                    usedPages = PhGetItemCircularBuffer_ULONG64(&Context->DeviceEntry->DedicatedHistory, getTooltipText->Index);

                    // Dedicated Memory: %s\n%s
                    PhInitFormatSize(&format[0], usedPages);
                    PhInitFormatC(&format[1], L'\n');
                    PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&Context->DedicatedGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                getTooltipText->Text = PhGetStringRef(Context->DedicatedGraphState.TooltipText);
            }
        }
        break;
    }
}

VOID GraphicsDeviceNotifySharedGraph(
    _In_ PDV_GPU_SYSINFO_CONTEXT Context,
    _In_ NMHDR *Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;
            ULONG i;
            LONG dpiValue;
            
            dpiValue = PhGetWindowDpi(Context->SharedGraphHandle);

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_USE_LINE_2 | (GraphicsEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
            Context->SysinfoSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorPhysical"), PhGetIntegerSetting(L"ColorCpuKernel"), dpiValue);

            PhGraphStateGetDrawInfo(
                &Context->SharedGraphState,
                getDrawInfo,
                Context->DeviceEntry->CommitHistory.Count
                );

            if (!Context->SharedGraphState.Valid)
            {
                FLOAT max = 1024 * 1024;

                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data1 = Context->SharedGraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG64(&Context->DeviceEntry->SharedHistory, i);
                    FLOAT data2 = Context->SharedGraphState.Data2[i] = (FLOAT)PhGetItemCircularBuffer_ULONG64(&Context->DeviceEntry->CommitHistory, i);

                    if (max < data1)
                        max = data1;
                    if (max < data2)
                        max = data2;
                }

                max = max * 2;

                if (max != 0)
                {
                    PhDivideSinglesBySingle(Context->SharedGraphState.Data1, max, drawInfo->LineDataCount);
                    PhDivideSinglesBySingle(Context->SharedGraphState.Data2, max, drawInfo->LineDataCount);
                }

                if (GraphicsEnableScaleText)
                {
                    drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                    drawInfo->LabelYFunctionParameter = max;
                }

                Context->SharedGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (Context->SharedGraphState.TooltipIndex != getTooltipText->Index)
                {
                    ULONG64 shared;
                    ULONG64 commit;
                    PH_FORMAT format[8];

                    shared = PhGetItemCircularBuffer_ULONG64(&Context->DeviceEntry->SharedHistory, getTooltipText->Index);
                    commit = PhGetItemCircularBuffer_ULONG64(&Context->DeviceEntry->CommitHistory, getTooltipText->Index);

                    // Shared Memory: %s\n%s
                    PhInitFormatS(&format[0], L"Shared: ");
                    PhInitFormatSize(&format[1], shared);
                    PhInitFormatS(&format[2], L"\nCommit: ");
                    PhInitFormatSize(&format[3], commit);
                    PhInitFormatS(&format[4], L"\nTotal: ");
                    PhInitFormatSize(&format[5], shared + commit);
                    PhInitFormatC(&format[6], L'\n');
                    PhInitFormatSR(&format[7], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&Context->SharedGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                getTooltipText->Text = PhGetStringRef(Context->SharedGraphState.TooltipText);
            }
        }
        break;
    }
}

PPH_STRING GraphicsDevicePowerLabelYFunction(
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ ULONG DataIndex,
    _In_ FLOAT Value,
    _In_ FLOAT Parameter
    )
{
    PH_FORMAT format[2];

    PhInitFormatF(&format[0], Value * Parameter, 1);
    PhInitFormatC(&format[1], L'%');

    return PhFormat(format, RTL_NUMBER_OF(format), 0);
}

VOID GraphicsDeviceNotifyPowerUsageGraph(
    _In_ PDV_GPU_SYSINFO_CONTEXT Context,
    _In_ NMHDR* Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;
            ULONG i;
            LONG dpiValue;
            
            dpiValue = PhGetWindowDpi(Context->PowerUsageGraphHandle);
            
            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (GraphicsEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
            Context->SysinfoSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorPowerUsage"), 0, dpiValue);

            PhGraphStateGetDrawInfo(
                &Context->PowerUsageGraphState,
                getDrawInfo,
                Context->DeviceEntry->PowerHistory.Count
                );

            if (!Context->PowerUsageGraphState.Valid)
            {
                FLOAT max = 100.0f;

                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data = Context->PowerUsageGraphState.Data1[i] = PhGetItemCircularBuffer_FLOAT(&Context->DeviceEntry->PowerHistory, i);

                    if (max < data)
                        max = data;
                }

                if (max != 0)
                {
                    PhDivideSinglesBySingle(Context->PowerUsageGraphState.Data1, max, drawInfo->LineDataCount);
                }

                if (GraphicsEnableScaleText)
                {
                    drawInfo->LabelYFunction = GraphicsDevicePowerLabelYFunction;
                    drawInfo->LabelYFunctionParameter = max;
                }

                Context->PowerUsageGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (Context->PowerUsageGraphState.TooltipIndex != getTooltipText->Index)
                {
                    FLOAT powerUsage;
                    PH_FORMAT format[4];

                    powerUsage = PhGetItemCircularBuffer_FLOAT(&Context->DeviceEntry->PowerHistory, getTooltipText->Index);

                    PhInitFormatF(&format[0], powerUsage, 1);
                    PhInitFormatC(&format[1], L'%');
                    PhInitFormatC(&format[2], L'\n');
                    PhInitFormatSR(&format[3], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&Context->PowerUsageGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                getTooltipText->Text = PhGetStringRef(Context->PowerUsageGraphState.TooltipText);
            }
        }
        break;
    }
}

PPH_STRING GraphicsDeviceTemperatureLabelYFunction(
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ ULONG DataIndex,
    _In_ FLOAT Value,
    _In_ FLOAT Parameter
    )
{
    PH_FORMAT format[2];

    PhInitFormatF(&format[0], Value * Parameter, 1);
    PhInitFormatS(&format[1], L"\u00b0C\n");

    return PhFormat(format, RTL_NUMBER_OF(format), 0);
}

VOID GraphicsDeviceNotifyTemperatureGraph(
    _In_ PDV_GPU_SYSINFO_CONTEXT Context,
    _In_ NMHDR* Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;
            ULONG i;
            LONG dpiValue;
            
            dpiValue = PhGetWindowDpi(Context->TemperatureGraphHandle);
            
            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (GraphicsEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
            Context->SysinfoSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorTemperature"), 0, dpiValue);

            PhGraphStateGetDrawInfo(
                &Context->TemperatureGraphState,
                getDrawInfo,
                Context->DeviceEntry->TemperatureHistory.Count
                );

            if (!Context->TemperatureGraphState.Valid)
            {
                FLOAT max = 100.0f;

                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data = Context->TemperatureGraphState.Data1[i] = PhGetItemCircularBuffer_FLOAT(&Context->DeviceEntry->TemperatureHistory, i);

                    if (max < data)
                        max = data;
                }

                if (max != 0)
                {
                    PhDivideSinglesBySingle(Context->TemperatureGraphState.Data1, max, drawInfo->LineDataCount);
                }

                if (GraphicsEnableScaleText)
                {
                    drawInfo->LabelYFunction = GraphicsDeviceTemperatureLabelYFunction;
                    drawInfo->LabelYFunctionParameter = max;
                }

                Context->TemperatureGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (Context->TemperatureGraphState.TooltipIndex != getTooltipText->Index)
                {
                    FLOAT temp;
                    PH_FORMAT format[3];

                    temp = PhGetItemCircularBuffer_FLOAT(&Context->DeviceEntry->TemperatureHistory, getTooltipText->Index);

                    //if (PhGetIntegerSetting(SETTING_NAME_ENABLE_FAHRENHEIT))
                    //{
                    //    PhInitFormatF(&format[0], (temp * 1.8 + 32), 1);
                    //    PhInitFormatS(&format[1], L"\u00b0F\n");
                    //    PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);
                    //}
                    //else
                    {
                        PhInitFormatF(&format[0], temp, 1);
                        PhInitFormatS(&format[1], L"\u00b0C\n");
                        PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);
                    }

                    PhMoveReference(&Context->TemperatureGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                getTooltipText->Text = PhGetStringRef(Context->TemperatureGraphState.TooltipText);
            }
        }
        break;
    }
}

PPH_STRING GraphicsDeviceFanLabelYFunction(
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ ULONG DataIndex,
    _In_ FLOAT Value,
    _In_ FLOAT Parameter
    )
{
    PH_FORMAT format[2];

    PhInitFormatU(&format[0], (ULONG)(Value * Parameter));
    PhInitFormatS(&format[1], L" RPM\n");

    return PhFormat(format, RTL_NUMBER_OF(format), 0);
}

VOID GraphicsDeviceNotifyFanRpmGraph(
    _In_ PDV_GPU_SYSINFO_CONTEXT Context,
    _In_ NMHDR* Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;
            ULONG i;
            LONG dpiValue;
            
            dpiValue = PhGetWindowDpi(Context->FanRpmGraphHandle);
            
            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (GraphicsEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
            Context->SysinfoSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorFanRpm"), 0, dpiValue);

            PhGraphStateGetDrawInfo(
                &Context->FanRpmGraphState,
                getDrawInfo,
                Context->DeviceEntry->FanHistory.Count
                );

            if (!Context->FanRpmGraphState.Valid)
            {
                FLOAT max = 1000.0f;

                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data = Context->FanRpmGraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&Context->DeviceEntry->FanHistory, i);

                    if (max < data)
                        max = data;
                }

                if (max != 0)
                {
                    PhDivideSinglesBySingle(Context->FanRpmGraphState.Data1, max, drawInfo->LineDataCount);
                }

                if (GraphicsEnableScaleText)
                {
                    drawInfo->LabelYFunction = GraphicsDeviceFanLabelYFunction;
                    drawInfo->LabelYFunctionParameter = max;
                }

                Context->FanRpmGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (Context->FanRpmGraphState.TooltipIndex != getTooltipText->Index)
                {
                    ULONG rpm;
                    PH_FORMAT format[3];

                    rpm = PhGetItemCircularBuffer_ULONG(&Context->DeviceEntry->FanHistory, getTooltipText->Index);

                    PhInitFormatU(&format[0], rpm);
                    PhInitFormatS(&format[1], L" RPM\n");
                    PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&Context->FanRpmGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                getTooltipText->Text = PhGetStringRef(Context->FanRpmGraphState.TooltipText);
            }
        }
        break;
    }
}

VOID GraphicsDeviceUpdateGraphs(
    _In_ PDV_GPU_SYSINFO_CONTEXT Context
    )
{
    Context->GpuGraphState.Valid = FALSE;
    Context->GpuGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->GpuGraphHandle, 1);
    Graph_Draw(Context->GpuGraphHandle);
    Graph_UpdateTooltip(Context->GpuGraphHandle);
    InvalidateRect(Context->GpuGraphHandle, NULL, FALSE);

    Context->DedicatedGraphState.Valid = FALSE;
    Context->DedicatedGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->DedicatedGraphHandle, 1);
    Graph_Draw(Context->DedicatedGraphHandle);
    Graph_UpdateTooltip(Context->DedicatedGraphHandle);
    InvalidateRect(Context->DedicatedGraphHandle, NULL, FALSE);

    Context->SharedGraphState.Valid = FALSE;
    Context->SharedGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->SharedGraphHandle, 1);
    Graph_Draw(Context->SharedGraphHandle);
    Graph_UpdateTooltip(Context->SharedGraphHandle);
    InvalidateRect(Context->SharedGraphHandle, NULL, FALSE);

    //if (EtGpuSupported)
    {
        Context->PowerUsageGraphState.Valid = FALSE;
        Context->PowerUsageGraphState.TooltipIndex = ULONG_MAX;
        Graph_MoveGrid(Context->PowerUsageGraphHandle, 1);
        Graph_Draw(Context->PowerUsageGraphHandle);
        Graph_UpdateTooltip(Context->PowerUsageGraphHandle);
        InvalidateRect(Context->PowerUsageGraphHandle, NULL, FALSE);

        Context->TemperatureGraphState.Valid = FALSE;
        Context->TemperatureGraphState.TooltipIndex = ULONG_MAX;
        Graph_MoveGrid(Context->TemperatureGraphHandle, 1);
        Graph_Draw(Context->TemperatureGraphHandle);
        Graph_UpdateTooltip(Context->TemperatureGraphHandle);
        InvalidateRect(Context->TemperatureGraphHandle, NULL, FALSE);

        Context->FanRpmGraphState.Valid = FALSE;
        Context->FanRpmGraphState.TooltipIndex = ULONG_MAX;
        Graph_MoveGrid(Context->FanRpmGraphHandle, 1);
        Graph_Draw(Context->FanRpmGraphHandle);
        Graph_UpdateTooltip(Context->FanRpmGraphHandle);
        InvalidateRect(Context->FanRpmGraphHandle, NULL, FALSE);
    }
}

VOID GraphicsDeviceUpdatePanel(
    _In_ PDV_GPU_SYSINFO_CONTEXT Context
    )
{
    PH_FORMAT format[1];
    WCHAR formatBuffer[512];

    PhInitFormatSize(&format[0], Context->DeviceEntry->CurrentDedicatedUsage);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->GpuPanelDedicatedUsageLabel, formatBuffer);
    else
        PhSetWindowText(Context->GpuPanelDedicatedUsageLabel, PhaFormatSize(Context->DeviceEntry->CurrentDedicatedUsage, ULONG_MAX)->Buffer);

    PhInitFormatSize(&format[0], Context->DeviceEntry->DedicatedLimit);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->GpuPanelDedicatedLimitLabel, formatBuffer);
    else
        PhSetWindowText(Context->GpuPanelDedicatedLimitLabel, PhaFormatSize(Context->DeviceEntry->DedicatedLimit, ULONG_MAX)->Buffer);

    PhInitFormatSize(&format[0], Context->DeviceEntry->CurrentSharedUsage);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->GpuPanelSharedUsageLabel, formatBuffer);
    else
        PhSetWindowText(Context->GpuPanelSharedUsageLabel, PhaFormatSize(Context->DeviceEntry->CurrentSharedUsage, ULONG_MAX)->Buffer);

    PhInitFormatSize(&format[0], Context->DeviceEntry->SharedLimit);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->GpuPanelSharedLimitLabel, formatBuffer);
    else
        PhSetWindowText(Context->GpuPanelSharedLimitLabel, PhaFormatSize(Context->DeviceEntry->SharedLimit, ULONG_MAX)->Buffer);
}

BOOLEAN GraphicsDeviceSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2
    )
{
    PDV_GPU_SYSINFO_CONTEXT context = (PDV_GPU_SYSINFO_CONTEXT)Section->Context;

    switch (Message)
    {
    case SysInfoDestroy:
        {
            if (context->Description)
            {
                PhDereferenceObject(context->Description);
                context->Description = NULL;
            }

            if (context->GpuDialog)
            {
                GraphicsDeviceUninitializeDialog(context);
                context->GpuDialog = NULL;
            }

            if (context->DeviceEntry)
            {
                PhDereferenceObject(context->DeviceEntry);
                context->DeviceEntry = NULL;
            }

            PhDereferenceObject(context);
        }
        return TRUE;
    case SysInfoTick:
        {
            if (context->GpuDialog)
            {
                GraphicsDeviceTickDialog(context);
            }
        }
        return TRUE;
    case SysInfoViewChanging:
        {
            PH_SYSINFO_VIEW_TYPE view = (PH_SYSINFO_VIEW_TYPE)Parameter1;
            PPH_SYSINFO_SECTION section = (PPH_SYSINFO_SECTION)Parameter2;

            if (view == SysInfoSummaryView || section != Section)
                return TRUE;

            if (context->GpuGraphHandle)
            {
                context->GpuGraphState.Valid = FALSE;
                context->GpuGraphState.TooltipIndex = ULONG_MAX;
                Graph_Draw(context->GpuGraphHandle);
            }

            if (context->DedicatedGraphHandle)
            {
                context->DedicatedGraphState.Valid = FALSE;
                context->DedicatedGraphState.TooltipIndex = ULONG_MAX;
                Graph_Draw(context->DedicatedGraphHandle);
            }

            if (context->SharedGraphHandle)
            {
                context->SharedGraphState.Valid = FALSE;
                context->SharedGraphState.TooltipIndex = ULONG_MAX;
                Graph_Draw(context->SharedGraphHandle);
            }

            //if (EtGpuSupported)
            {
                if (context->PowerUsageGraphHandle)
                {
                    context->PowerUsageGraphState.Valid = FALSE;
                    context->PowerUsageGraphState.TooltipIndex = ULONG_MAX;
                    Graph_Draw(context->PowerUsageGraphHandle);
                }

                if (context->TemperatureGraphHandle)
                {
                    context->TemperatureGraphState.Valid = FALSE;
                    context->TemperatureGraphState.TooltipIndex = ULONG_MAX;
                    Graph_Draw(context->TemperatureGraphHandle);
                }

                if (context->FanRpmGraphHandle)
                {
                    context->FanRpmGraphState.Valid = FALSE;
                    context->FanRpmGraphState.TooltipIndex = ULONG_MAX;
                    Graph_Draw(context->FanRpmGraphHandle);
                }
            }
        }
        return TRUE;
    case SysInfoCreateDialog:
        {
            PPH_SYSINFO_CREATE_DIALOG createDialog = Parameter1;

            createDialog->Instance = PluginInstance->DllBase;
            createDialog->Template = MAKEINTRESOURCE(IDD_GPUDEVICE_DIALOG);
            createDialog->DialogProc = GraphicsDeviceDialogProc;
            createDialog->Parameter = context;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = Parameter1;
            LONG dpiValue;
            
            dpiValue = PhGetWindowDpi(Section->GraphHandle);
            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (GraphicsEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), 0, dpiValue);
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, context->DeviceEntry->GpuUsageHistory.Count);

            if (!Section->GraphState.Valid)
            {
                PhCopyCircularBuffer_FLOAT(&context->DeviceEntry->GpuUsageHistory, Section->GraphState.Data1, drawInfo->LineDataCount);

                if (GraphicsEnableScaleGraph)
                {
                    FLOAT max = 0;

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

                    if (GraphicsEnableScaleText)
                    {
                        drawInfo->LabelYFunction = PhSiDoubleLabelYFunction;
                        drawInfo->LabelYFunctionParameter = max;
                    }
                }
                else
                {
                    if (GraphicsEnableScaleText)
                    {
                        drawInfo->LabelYFunction = PhSiDoubleLabelYFunction;
                        drawInfo->LabelYFunctionParameter = 1.0f;
                    }
                }

                Section->GraphState.Valid = TRUE;
            }
        }
        return TRUE;
    case SysInfoGraphGetTooltipText:
        {
            PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT getTooltipText = Parameter1;
            FLOAT gpu;
            PH_FORMAT format[3];

            gpu = PhGetItemCircularBuffer_FLOAT(&context->DeviceEntry->GpuUsageHistory, getTooltipText->Index);

            // %.2f%%%s\n%s
            PhInitFormatF(&format[0], gpu * 100, 2);
            PhInitFormatS(&format[1], L"%\n");
            PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

            PhMoveReference(&Section->GraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));

            getTooltipText->Text = PhGetStringRef(Section->GraphState.TooltipText);
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = Parameter1;

            if (PhIsNullOrEmptyString(context->Description))
            {
                PhMoveReference(&context->Description, GraphicsDeviceGetAdapterDescription(context->DeviceEntry->Id.DevicePath));
            }

            if (!PhIsNullOrEmptyString(context->Description))
            {
                drawPanel->Title = PhReferenceObject(context->Description);
            }

            // Note: Intel IGP devices don't have dedicated usage/limit values. (dmex)
            if (context->DeviceEntry->CurrentDedicatedUsage && context->DeviceEntry->DedicatedLimit)
            {
                PH_FORMAT format[5];

                // %.2f%%\n%s / %s
                PhInitFormatF(&format[0], context->DeviceEntry->CurrentGpuUsage * 100, 2);
                PhInitFormatS(&format[1], L"%\n");
                PhInitFormatSize(&format[2], context->DeviceEntry->CurrentDedicatedUsage);
                PhInitFormatS(&format[3], L" / ");
                PhInitFormatSize(&format[4], context->DeviceEntry->DedicatedLimit);

                drawPanel->SubTitle = PhFormat(format, 5, 64);

                // %.2f%%\n%s
                PhInitFormatF(&format[0], context->DeviceEntry->CurrentGpuUsage * 100, 2);
                PhInitFormatS(&format[1], L"%\n");
                PhInitFormatSize(&format[2], context->DeviceEntry->CurrentDedicatedUsage);

                drawPanel->SubTitleOverflow = PhFormat(format, 3, 64);
            }
            else if (context->DeviceEntry->CurrentSharedUsage && context->DeviceEntry->SharedLimit)
            {
                PH_FORMAT format[5];

                // %.2f%%\n%s / %s
                PhInitFormatF(&format[0], context->DeviceEntry->CurrentGpuUsage * 100, 2);
                PhInitFormatS(&format[1], L"%\n");
                PhInitFormatSize(&format[2], context->DeviceEntry->CurrentSharedUsage);
                PhInitFormatS(&format[3], L" / ");
                PhInitFormatSize(&format[4], context->DeviceEntry->SharedLimit);

                drawPanel->SubTitle = PhFormat(format, 5, 64);

                // %.2f%%\n%s
                PhInitFormatF(&format[0], context->DeviceEntry->CurrentGpuUsage * 100, 2);
                PhInitFormatS(&format[1], L"%\n");
                PhInitFormatSize(&format[2], context->DeviceEntry->CurrentSharedUsage);

                drawPanel->SubTitleOverflow = PhFormat(format, 3, 64);
            }
            else
            {
                PH_FORMAT format[2];

                // %.2f%%\n
                PhInitFormatF(&format[0], context->DeviceEntry->CurrentGpuUsage * 100, 2);
                PhInitFormatS(&format[1], L"%\n");

                drawPanel->SubTitle = PhFormat(format, RTL_NUMBER_OF(format), 0);
            }
        }
        return TRUE;
    }

    return FALSE;
}

INT_PTR CALLBACK GraphicsDeviceDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_GPU_SYSINFO_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PDV_GPU_SYSINFO_CONTEXT)lParam;

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
            PPH_STRING description;

            context->GpuDialog = hwndDlg;

            PhInitializeLayoutManager(&context->GpuLayoutManager, hwndDlg);
            PhAddLayoutItem(&context->GpuLayoutManager, GetDlgItem(hwndDlg, IDC_GPUNAME), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            graphItem = PhAddLayoutItem(&context->GpuLayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&context->GpuLayoutManager, GetDlgItem(hwndDlg, IDC_PANEL_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            context->GpuGraphMargin = graphItem->Margin;

            GraphicsDeviceInitializeDialog(context);

            SetWindowFont(GetDlgItem(hwndDlg, IDC_TITLE), context->SysinfoSection->Parameters->LargeFont, FALSE);
            SetWindowFont(GetDlgItem(hwndDlg, IDC_GPUNAME), context->SysinfoSection->Parameters->MediumFont, FALSE);

            if (GraphicsQueryDeviceProperties(PhGetString(context->DeviceEntry->Id.DevicePath), &description, NULL, NULL, NULL, NULL, NULL))
                PhSetDialogItemText(hwndDlg, IDC_GPUNAME, PhGetString(PH_AUTO_T(PH_STRING, description)));
            else
                PhSetDialogItemText(hwndDlg, IDC_GPUNAME, L"GPU");

            context->GpuPanel = PhCreateDialog(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_GPUDEVICE_PANEL), hwndDlg, GraphicsDevicePanelDialogProc, context);
            ShowWindow(context->GpuPanel, SW_SHOW);
            PhAddLayoutItemEx(&context->GpuLayoutManager, context->GpuPanel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, panelItem->Margin);

            GraphicsDeviceCreateGraphs(context);
            GraphicsDeviceUpdateGraphs(context);
            GraphicsDeviceUpdatePanel(context);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&context->GpuLayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->GpuLayoutManager);

            GraphicsDeviceLayoutGraphs(context);
        }
        break;
    case WM_NOTIFY:
        {
            NMHDR *header = (NMHDR *)lParam;

            if (header->hwndFrom == context->GpuGraphHandle)
            {
                GraphicsDeviceNotifyGpuGraph(context, header);
            }
            else if (header->hwndFrom == context->DedicatedGraphHandle)
            {
                GraphicsDeviceNotifyDedicatedGraph(context, header);
            }
            else if (header->hwndFrom == context->SharedGraphHandle)
            {
                GraphicsDeviceNotifySharedGraph(context, header);
            }
            else if (header->hwndFrom == context->PowerUsageGraphHandle)
            {
                GraphicsDeviceNotifyPowerUsageGraph(context, header);
            }
            else if (header->hwndFrom == context->TemperatureGraphHandle)
            {
                GraphicsDeviceNotifyTemperatureGraph(context, header);
            }
            else if (header->hwndFrom == context->FanRpmGraphHandle)
            {
                GraphicsDeviceNotifyFanRpmGraph(context, header);
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

INT_PTR CALLBACK GraphicsDevicePanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PDV_GPU_SYSINFO_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PDV_GPU_SYSINFO_CONTEXT)lParam;

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
            context->GpuPanelDedicatedUsageLabel = GetDlgItem(hwndDlg, IDC_ZDEDICATEDCURRENT_V);
            context->GpuPanelDedicatedLimitLabel = GetDlgItem(hwndDlg, IDC_ZDEDICATEDLIMIT_V);
            context->GpuPanelSharedUsageLabel = GetDlgItem(hwndDlg, IDC_ZSHAREDCURRENT_V);
            context->GpuPanelSharedLimitLabel = GetDlgItem(hwndDlg, IDC_ZSHAREDLIMIT_V);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_NODES:
                GraphicsDeviceShowNodesDialog(context, hwndDlg);
                break;
            case IDC_DETAILS:
                GraphicsDeviceShowDetailsDialog(context, hwndDlg);
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
