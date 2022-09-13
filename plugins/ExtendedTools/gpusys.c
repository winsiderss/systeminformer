/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011
 *     dmex    2015-2022
 *
 */

#include "exttools.h"
#include "gpusys.h"

static PPH_SYSINFO_SECTION GpuSection;
static HWND GpuDialog;
static PH_LAYOUT_MANAGER GpuLayoutManager;
static RECT GpuGraphMargin;
static HWND GpuGraphHandle;
static PH_GRAPH_STATE GpuGraphState;
static HWND DedicatedGraphHandle;
static PH_GRAPH_STATE DedicatedGraphState;
static HWND SharedGraphHandle;
static PH_GRAPH_STATE SharedGraphState;
static HWND PowerUsageGraphHandle;
static PH_GRAPH_STATE PowerUsageGraphState;
static HWND TemperatureGraphHandle;
static PH_GRAPH_STATE TemperatureGraphState;
static HWND FanRpmGraphHandle;
static PH_GRAPH_STATE FanRpmGraphState;
static HWND GpuPanel;
static HWND GpuPanelDedicatedUsageLabel;
static HWND GpuPanelDedicatedLimitLabel;
static HWND GpuPanelSharedUsageLabel;
static HWND GpuPanelSharedLimitLabel;

VOID EtGpuSystemInformationInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers
    )
{
    PH_SYSINFO_SECTION section;

    memset(&section, 0, sizeof(PH_SYSINFO_SECTION));
    PhInitializeStringRef(&section.Name, L"GPU");
    section.Flags = 0;
    section.Callback = EtpGpuSysInfoSectionCallback;

    GpuSection = Pointers->CreateSection(&section);
}

BOOLEAN EtpGpuSysInfoSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    switch (Message)
    {
    case SysInfoDestroy:
        {
            if (GpuDialog)
            {
                EtpUninitializeGpuDialog();
                GpuDialog = NULL;
            }
        }
        return TRUE;
    case SysInfoTick:
        {
            if (GpuDialog)
            {
                EtpTickGpuDialog();
            }
        }
        return TRUE;
    case SysInfoViewChanging:
        {
            PH_SYSINFO_VIEW_TYPE view = (PH_SYSINFO_VIEW_TYPE)Parameter1;
            PPH_SYSINFO_SECTION section = (PPH_SYSINFO_SECTION)Parameter2;

            if (view == SysInfoSummaryView || section != Section)
                return TRUE;

            if (GpuGraphHandle)
            {
                GpuGraphState.Valid = FALSE;
                GpuGraphState.TooltipIndex = ULONG_MAX;
                Graph_Draw(GpuGraphHandle);
            }

            if (DedicatedGraphHandle)
            {
                DedicatedGraphState.Valid = FALSE;
                DedicatedGraphState.TooltipIndex = ULONG_MAX;
                Graph_Draw(DedicatedGraphHandle);
            }

            if (SharedGraphHandle)
            {
                SharedGraphState.Valid = FALSE;
                SharedGraphState.TooltipIndex = ULONG_MAX;
                Graph_Draw(SharedGraphHandle);
            }

            if (EtGpuSupported)
            {
                if (PowerUsageGraphHandle)
                {
                    PowerUsageGraphState.Valid = FALSE;
                    PowerUsageGraphState.TooltipIndex = ULONG_MAX;
                    Graph_Draw(PowerUsageGraphHandle);
                }

                if (TemperatureGraphHandle)
                {
                    TemperatureGraphState.Valid = FALSE;
                    TemperatureGraphState.TooltipIndex = ULONG_MAX;
                    Graph_Draw(TemperatureGraphHandle);
                }

                if (FanRpmGraphHandle)
                {
                    FanRpmGraphState.Valid = FALSE;
                    FanRpmGraphState.TooltipIndex = ULONG_MAX;
                    Graph_Draw(FanRpmGraphHandle);
                }
            }
        }
        return TRUE;
    case SysInfoCreateDialog:
        {
            PPH_SYSINFO_CREATE_DIALOG createDialog = Parameter1;

            if (!createDialog)
                break;

            createDialog->Instance = PluginInstance->DllBase;
            createDialog->Template = MAKEINTRESOURCE(IDD_SYSINFO_GPU);
            createDialog->DialogProc = EtpGpuDialogProc;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = Parameter1;

            if (!drawInfo)
                break;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (EtEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), 0);
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, EtGpuNodeHistory.Count);

            if (!Section->GraphState.Valid)
            {
                PhCopyCircularBuffer_FLOAT(&EtGpuNodeHistory, Section->GraphState.Data1, drawInfo->LineDataCount);

                if (EtEnableScaleGraph)
                {
                    FLOAT max = 0;

                    for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                    {
                        FLOAT data = Section->GraphState.Data1[i]; // HACK

                        if (max < data)
                            max = data;
                    }

                    if (max != 0)
                    {
                        PhDivideSinglesBySingle(Section->GraphState.Data1, max, drawInfo->LineDataCount);
                    }

                    if (EtEnableScaleText)
                    {
                        drawInfo->LabelYFunction = PhSiDoubleLabelYFunction;
                        drawInfo->LabelYFunctionParameter = max;
                    }
                }
                else
                {
                    if (EtEnableScaleText)
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
            PH_FORMAT format[5];

            if (!getTooltipText)
                break;

            gpu = PhGetItemCircularBuffer_FLOAT(&EtGpuNodeHistory, getTooltipText->Index);

            // %.2f%%%s\n%s
            PhInitFormatF(&format[0], (DOUBLE)gpu * 100, 2);
            PhInitFormatC(&format[1], L'%');
            PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, EtpGetMaxNodeString(getTooltipText->Index))->sr);
            PhInitFormatC(&format[3], L'\n');
            PhInitFormatSR(&format[4], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

            PhMoveReference(&Section->GraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));

            getTooltipText->Text = PhGetStringRef(Section->GraphState.TooltipText);
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = Parameter1;

            if (!drawPanel)
                break;

            drawPanel->Title = PhCreateString(L"GPU");

            // Note: Intel IGP devices don't have dedicated usage/limit values. (dmex)
            if (EtGpuDedicatedUsage && EtGpuDedicatedLimit)
            {
                PH_FORMAT format[5];

                // %.2f%%\n%s / %s
                PhInitFormatF(&format[0], EtGpuNodeUsage * 100, 2);
                PhInitFormatS(&format[1], L"%\n");
                PhInitFormatSize(&format[2], EtGpuDedicatedUsage);
                PhInitFormatS(&format[3], L" / ");
                PhInitFormatSize(&format[4], EtGpuDedicatedLimit);

                drawPanel->SubTitle = PhFormat(format, 5, 64);

                // %.2f%%\n%s
                PhInitFormatF(&format[0], EtGpuNodeUsage * 100, 2);
                PhInitFormatS(&format[1], L"%\n");
                PhInitFormatSize(&format[2], EtGpuDedicatedUsage);

                drawPanel->SubTitleOverflow = PhFormat(format, 3, 64);
            }
            else if (EtGpuSharedUsage && EtGpuSharedLimit)
            {
                PH_FORMAT format[5];

                // %.2f%%\n%s / %s
                PhInitFormatF(&format[0], EtGpuNodeUsage * 100, 2);
                PhInitFormatS(&format[1], L"%\n");
                PhInitFormatSize(&format[2], EtGpuSharedUsage);
                PhInitFormatS(&format[3], L" / ");
                PhInitFormatSize(&format[4], EtGpuSharedLimit);

                drawPanel->SubTitle = PhFormat(format, 5, 64);

                // %.2f%%\n%s
                PhInitFormatF(&format[0], EtGpuNodeUsage * 100, 2);
                PhInitFormatS(&format[1], L"%\n");
                PhInitFormatSize(&format[2], EtGpuSharedUsage);

                drawPanel->SubTitleOverflow = PhFormat(format, 3, 64);
            }
            else
            {
                PH_FORMAT format[2];

                // %.2f%%\n
                PhInitFormatF(&format[0], EtGpuNodeUsage * 100, 2);
                PhInitFormatS(&format[1], L"%\n");

                drawPanel->SubTitle = PhFormat(format, RTL_NUMBER_OF(format), 0);
            }
        }
        return TRUE;
    }

    return FALSE;
}

VOID EtpInitializeGpuDialog(
    VOID
    )
{
    PhInitializeGraphState(&GpuGraphState);
    PhInitializeGraphState(&DedicatedGraphState);
    PhInitializeGraphState(&SharedGraphState);

    if (EtGpuSupported)
    {
        PhInitializeGraphState(&PowerUsageGraphState);
        PhInitializeGraphState(&TemperatureGraphState);
        PhInitializeGraphState(&FanRpmGraphState);
    }
}

VOID EtpUninitializeGpuDialog(
    VOID
    )
{
    PhDeleteGraphState(&GpuGraphState);
    PhDeleteGraphState(&DedicatedGraphState);
    PhDeleteGraphState(&SharedGraphState);

    if (EtGpuSupported)
    {
        PhDeleteGraphState(&PowerUsageGraphState);
        PhDeleteGraphState(&TemperatureGraphState);
        PhDeleteGraphState(&FanRpmGraphState);
    }

    // Note: Required for SysInfoViewChanging (dmex)
    GpuGraphHandle = NULL;
    DedicatedGraphHandle = NULL;
    SharedGraphHandle = NULL;
    PowerUsageGraphHandle = NULL;
    TemperatureGraphHandle = NULL;
    FanRpmGraphHandle = NULL;
}

VOID EtpTickGpuDialog(
    VOID
    )
{
    EtpUpdateGpuGraphs();
    EtpUpdateGpuPanel();
}

INT_PTR CALLBACK EtpGpuDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PPH_LAYOUT_ITEM graphItem;
            PPH_LAYOUT_ITEM panelItem;

            EtpInitializeGpuDialog();

            GpuDialog = hwndDlg;
            PhInitializeLayoutManager(&GpuLayoutManager, hwndDlg);
            PhAddLayoutItem(&GpuLayoutManager, GetDlgItem(hwndDlg, IDC_GPUNAME), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            graphItem = PhAddLayoutItem(&GpuLayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&GpuLayoutManager, GetDlgItem(hwndDlg, IDC_PANEL_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            GpuGraphMargin = graphItem->Margin;

            SetWindowFont(GetDlgItem(hwndDlg, IDC_TITLE), GpuSection->Parameters->LargeFont, FALSE);
            SetWindowFont(GetDlgItem(hwndDlg, IDC_GPUNAME), GpuSection->Parameters->MediumFont, FALSE);

            PhSetDialogItemText(hwndDlg, IDC_GPUNAME, PH_AUTO_T(PH_STRING, EtpGetGpuNameString())->Buffer);

            GpuPanel = PhCreateDialog(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_SYSINFO_GPUPANEL), hwndDlg, EtpGpuPanelDialogProc, NULL);
            ShowWindow(GpuPanel, SW_SHOW);
            PhAddLayoutItemEx(&GpuLayoutManager, GpuPanel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, panelItem->Margin);

            EtpCreateGpuGraphs();
            EtpUpdateGpuGraphs();
            EtpUpdateGpuPanel();

            if (!EtGpuSupported)
            {
                ShowWindow(GetDlgItem(hwndDlg, IDC_POWER_USAGE_L), SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_TEMPERATURE_L), SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_FAN_RPM_L), SW_HIDE);
            }
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&GpuLayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&GpuLayoutManager);
            EtpLayoutGpuGraphs();
        }
        break;
    case WM_NOTIFY:
        {
            NMHDR *header = (NMHDR *)lParam;

            if (header->hwndFrom == GpuGraphHandle)
            {
                EtpNotifyGpuGraph(header);
            }
            else if (header->hwndFrom == DedicatedGraphHandle)
            {
                EtpNotifyDedicatedGraph(header);
            }
            else if (header->hwndFrom == SharedGraphHandle)
            {
                EtpNotifySharedGraph(header);
            }
            else if (header->hwndFrom == PowerUsageGraphHandle)
            {
                EtpNotifyPowerUsageGraph(header);
            }
            else if (header->hwndFrom == TemperatureGraphHandle)
            {
                EtpNotifyTemperatureGraph(header);
            }
            else if (header->hwndFrom == FanRpmGraphHandle)
            {
                EtpNotifyFanRpmGraph(header);
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

INT_PTR CALLBACK EtpGpuPanelDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            GpuPanelDedicatedUsageLabel = GetDlgItem(hwndDlg, IDC_ZDEDICATEDCURRENT_V);
            GpuPanelDedicatedLimitLabel = GetDlgItem(hwndDlg, IDC_ZDEDICATEDLIMIT_V);
            GpuPanelSharedUsageLabel = GetDlgItem(hwndDlg, IDC_ZSHAREDCURRENT_V);
            GpuPanelSharedLimitLabel = GetDlgItem(hwndDlg, IDC_ZSHAREDLIMIT_V);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_NODES:
                EtShowGpuNodesDialog(GpuDialog);
                break;
            case IDC_DETAILS:
                EtShowGpuDetailsDialog(GpuDialog);
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

VOID EtpCreateGpuGraphs(
    VOID
    )
{
    GpuGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        GpuDialog,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(GpuGraphHandle, TRUE);

    DedicatedGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        GpuDialog,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(DedicatedGraphHandle, TRUE);

    SharedGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        GpuDialog,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(SharedGraphHandle, TRUE);

    if (EtGpuSupported)
    {
        PowerUsageGraphHandle = CreateWindow(
            PH_GRAPH_CLASSNAME,
            NULL,
            WS_VISIBLE | WS_CHILD | WS_BORDER,
            0,
            0,
            3,
            3,
            GpuDialog,
            NULL,
            NULL,
            NULL
            );
        Graph_SetTooltip(PowerUsageGraphHandle, TRUE);

        TemperatureGraphHandle = CreateWindow(
            PH_GRAPH_CLASSNAME,
            NULL,
            WS_VISIBLE | WS_CHILD | WS_BORDER,
            0,
            0,
            3,
            3,
            GpuDialog,
            NULL,
            NULL,
            NULL
            );
        Graph_SetTooltip(TemperatureGraphHandle, TRUE);

        FanRpmGraphHandle = CreateWindow(
            PH_GRAPH_CLASSNAME,
            NULL,
            WS_VISIBLE | WS_CHILD | WS_BORDER,
            0,
            0,
            3,
            3,
            GpuDialog,
            NULL,
            NULL,
            NULL
            );
        Graph_SetTooltip(FanRpmGraphHandle, TRUE);
    }
}

VOID EtpLayoutGpuGraphs(
    VOID
    )
{
    RECT clientRect;
    RECT labelRect;
    ULONG graphWidth;
    ULONG graphHeight;
    HDWP deferHandle;
    ULONG y;

    GpuGraphState.Valid = FALSE;
    GpuGraphState.TooltipIndex = ULONG_MAX;
    DedicatedGraphState.Valid = FALSE;
    DedicatedGraphState.TooltipIndex = ULONG_MAX;
    SharedGraphState.Valid = FALSE;
    SharedGraphState.TooltipIndex = ULONG_MAX;

    if (EtGpuSupported)
    {
        PowerUsageGraphState.Valid = FALSE;
        PowerUsageGraphState.TooltipIndex = ULONG_MAX;
        TemperatureGraphState.Valid = FALSE;
        TemperatureGraphState.TooltipIndex = ULONG_MAX;
        FanRpmGraphState.Valid = FALSE;
        FanRpmGraphState.TooltipIndex = ULONG_MAX;
    }

    GetClientRect(GpuDialog, &clientRect);
    GetClientRect(GetDlgItem(GpuDialog, IDC_GPU_L), &labelRect);
    graphWidth = clientRect.right - GpuGraphMargin.left - GpuGraphMargin.right;

    if (EtGpuSupported)
        graphHeight = (clientRect.bottom - GpuGraphMargin.top - GpuGraphMargin.bottom - labelRect.bottom * 6 - ET_GPU_PADDING * 8) / 6;
    else
        graphHeight = (clientRect.bottom - GpuGraphMargin.top - GpuGraphMargin.bottom - labelRect.bottom * 3 - ET_GPU_PADDING * 5) / 3;

    deferHandle = BeginDeferWindowPos(12);
    y = GpuGraphMargin.top;

    deferHandle = DeferWindowPos(
        deferHandle,
        GetDlgItem(GpuDialog, IDC_GPU_L),
        NULL,
        GpuGraphMargin.left,
        y,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += labelRect.bottom + ET_GPU_PADDING;

    deferHandle = DeferWindowPos(
        deferHandle,
        GpuGraphHandle,
        NULL,
        GpuGraphMargin.left,
        y,
        graphWidth,
        graphHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += graphHeight + ET_GPU_PADDING;

    deferHandle = DeferWindowPos(
        deferHandle,
        GetDlgItem(GpuDialog, IDC_DEDICATED_L),
        NULL,
        GpuGraphMargin.left,
        y,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += labelRect.bottom + ET_GPU_PADDING;

    deferHandle = DeferWindowPos(
        deferHandle,
        DedicatedGraphHandle,
        NULL,
        GpuGraphMargin.left,
        y,
        graphWidth,
        graphHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += graphHeight + ET_GPU_PADDING;

    deferHandle = DeferWindowPos(
        deferHandle,
        GetDlgItem(GpuDialog, IDC_SHARED_L),
        NULL,
        GpuGraphMargin.left,
        y,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += labelRect.bottom + ET_GPU_PADDING;

    deferHandle = DeferWindowPos(
        deferHandle,
        SharedGraphHandle,
        NULL,
        GpuGraphMargin.left,
        y,
        graphWidth,
        graphHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += graphHeight + ET_GPU_PADDING;

    if (EtGpuSupported)
    {
        deferHandle = DeferWindowPos(
            deferHandle,
            GetDlgItem(GpuDialog, IDC_POWER_USAGE_L),
            NULL,
            GpuGraphMargin.left,
            y,
            0,
            0,
            SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
            );
        y += labelRect.bottom + ET_GPU_PADDING;

        deferHandle = DeferWindowPos(
            deferHandle,
            PowerUsageGraphHandle,
            NULL,
            GpuGraphMargin.left,
            y,
            graphWidth,
            graphHeight,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
        y += graphHeight + ET_GPU_PADDING;

        deferHandle = DeferWindowPos(
            deferHandle,
            GetDlgItem(GpuDialog, IDC_TEMPERATURE_L),
            NULL,
            GpuGraphMargin.left,
            y,
            0,
            0,
            SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
            );
        y += labelRect.bottom + ET_GPU_PADDING;

        deferHandle = DeferWindowPos(
            deferHandle,
            TemperatureGraphHandle,
            NULL,
            GpuGraphMargin.left,
            y,
            graphWidth,
            graphHeight,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
        y += graphHeight + ET_GPU_PADDING;

        deferHandle = DeferWindowPos(
            deferHandle,
            GetDlgItem(GpuDialog, IDC_FAN_RPM_L),
            NULL,
            GpuGraphMargin.left,
            y,
            0,
            0,
            SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
            );
        y += labelRect.bottom + ET_GPU_PADDING;

        deferHandle = DeferWindowPos(
            deferHandle,
            FanRpmGraphHandle,
            NULL,
            GpuGraphMargin.left,
            y,
            graphWidth,
            clientRect.bottom - GpuGraphMargin.bottom - y,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
    }

    EndDeferWindowPos(deferHandle);
}

PPH_STRING EtpPowerUsageGraphLabelYFunction(
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

PPH_STRING EtpTemperatureGraphLabelYFunction(
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ ULONG DataIndex,
    _In_ FLOAT Value,
    _In_ FLOAT Parameter
    )
{
    PH_FORMAT format[2];

    PhInitFormatF(&format[0], (ULONG)(Value * Parameter), 1);
    PhInitFormatS(&format[1], L"\u00b0C\n");

    return PhFormat(format, RTL_NUMBER_OF(format), 0);
}

PPH_STRING EtpFanRpmGraphLabelYFunction(
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

VOID EtpNotifyGpuGraph(
    _In_ NMHDR *Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (EtEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
            GpuSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), 0);

            PhGraphStateGetDrawInfo(
                &GpuGraphState,
                getDrawInfo,
                EtGpuNodeHistory.Count
                );

            if (!GpuGraphState.Valid)
            {
                PhCopyCircularBuffer_FLOAT(&EtGpuNodeHistory, GpuGraphState.Data1, drawInfo->LineDataCount);

                if (EtEnableScaleGraph)
                {
                    FLOAT max = 0;

                    for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                    {
                        FLOAT data = GpuGraphState.Data1[i]; // HACK

                        if (max < data)
                            max = data;
                    }

                    if (max != 0)
                    {
                        PhDivideSinglesBySingle(GpuGraphState.Data1, max, drawInfo->LineDataCount);
                    }

                    if (EtEnableScaleText)
                    {
                        drawInfo->LabelYFunction = PhSiDoubleLabelYFunction;
                        drawInfo->LabelYFunctionParameter = max;
                    }
                }
                else
                {
                    if (EtEnableScaleText)
                    {
                        drawInfo->LabelYFunction = PhSiDoubleLabelYFunction;
                        drawInfo->LabelYFunctionParameter = 1.0f;
                    }
                }

                GpuGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (GpuGraphState.TooltipIndex != getTooltipText->Index)
                {
                    FLOAT gpu;
                    PH_FORMAT format[5];

                    gpu = PhGetItemCircularBuffer_FLOAT(&EtGpuNodeHistory, getTooltipText->Index);

                    // %.2f%%%s\n%s
                    PhInitFormatF(&format[0], gpu * 100, 2);
                    PhInitFormatC(&format[1], L'%');
                    PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, EtpGetMaxNodeString(getTooltipText->Index))->sr);
                    PhInitFormatC(&format[3], L'\n');
                    PhInitFormatSR(&format[4], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&GpuGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                getTooltipText->Text = PhGetStringRef(GpuGraphState.TooltipText);
            }
        }
        break;
    case GCN_MOUSEEVENT:
        {
            PPH_GRAPH_MOUSEEVENT mouseEvent = (PPH_GRAPH_MOUSEEVENT)Header;
            PPH_PROCESS_RECORD record;

            record = NULL;

            if (mouseEvent->Message == WM_LBUTTONDBLCLK && mouseEvent->Index < mouseEvent->TotalCount)
            {
                record = EtpReferenceMaxNodeRecord(mouseEvent->Index);
            }

            if (record)
            {
                PhShowProcessRecordDialog(GpuDialog, record);
                PhDereferenceProcessRecord(record);
            }
        }
        break;
    }
}

VOID EtpNotifyDedicatedGraph(
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

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (EtEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
            GpuSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorPrivate"), 0);

            PhGraphStateGetDrawInfo(
                &DedicatedGraphState,
                getDrawInfo,
                EtGpuDedicatedHistory.Count
                );

            if (!DedicatedGraphState.Valid)
            {
                FLOAT max = 0;

                if (EtGpuDedicatedLimit != 0 && !EtEnableScaleGraph)
                {
                    max = (FLOAT)EtGpuDedicatedLimit;
                }

                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data = DedicatedGraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG64(&EtGpuDedicatedHistory, i);

                    if (max < data)
                        max = data;
                }

                if (max != 0)
                {
                    PhDivideSinglesBySingle(DedicatedGraphState.Data1, max, drawInfo->LineDataCount);
                }

                if (EtEnableScaleText)
                {
                    drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                    drawInfo->LabelYFunctionParameter = max;
                }

                DedicatedGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (DedicatedGraphState.TooltipIndex != getTooltipText->Index)
                {
                    ULONG64 usedPages;
                    PH_FORMAT format[3];

                    usedPages = PhGetItemCircularBuffer_ULONG64(&EtGpuDedicatedHistory, getTooltipText->Index);

                    // Dedicated Memory: %s\n%s
                    PhInitFormatSize(&format[0], usedPages);
                    PhInitFormatC(&format[1], L'\n');
                    PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&DedicatedGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                getTooltipText->Text = PhGetStringRef(DedicatedGraphState.TooltipText);
            }
        }
        break;
    }
}

VOID EtpNotifySharedGraph(
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

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (EtEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
            GpuSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorPhysical"), 0);

            PhGraphStateGetDrawInfo(
                &SharedGraphState,
                getDrawInfo,
                EtGpuSharedHistory.Count
                );

            if (!SharedGraphState.Valid)
            {
                FLOAT max = 0;

                //if (EtGpuSharedLimit != 0 && !EtEnableScaleGraph)
                //{
                //    max = (FLOAT)EtGpuSharedLimit;
                //}

                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data = SharedGraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG64(&EtGpuSharedHistory, i);

                    if (max < data)
                        max = data;
                }

                if (max != 0)
                {
                    PhDivideSinglesBySingle(SharedGraphState.Data1, max, drawInfo->LineDataCount);
                }

                if (EtEnableScaleText)
                {
                    drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                    drawInfo->LabelYFunctionParameter = max;
                }

                SharedGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (SharedGraphState.TooltipIndex != getTooltipText->Index)
                {
                    ULONG64 usedPages;
                    PH_FORMAT format[3];

                    usedPages = PhGetItemCircularBuffer_ULONG64(&EtGpuSharedHistory, getTooltipText->Index);

                    // Shared Memory: %s\n%s
                    PhInitFormatSize(&format[0], usedPages);
                    PhInitFormatC(&format[1], L'\n');
                    PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&SharedGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                getTooltipText->Text = PhGetStringRef(SharedGraphState.TooltipText);
            }
        }
        break;
    }
}

VOID EtpNotifyPowerUsageGraph(
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

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (EtEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
            GpuSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorPowerUsage"), 0);

            PhGraphStateGetDrawInfo(
                &PowerUsageGraphState,
                getDrawInfo,
                EtGpuPowerUsageHistory.Count
                );

            if (!PowerUsageGraphState.Valid)
            {
                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    PowerUsageGraphState.Data1[i] = PhGetItemCircularBuffer_FLOAT(&EtGpuPowerUsageHistory, i);
                }

                PhDivideSinglesBySingle(
                    PowerUsageGraphState.Data1,
                    EtGpuPowerUsageLimit,
                    drawInfo->LineDataCount
                    );

                if (EtEnableScaleText)
                {
                    drawInfo->LabelYFunction = EtpPowerUsageGraphLabelYFunction;
                    drawInfo->LabelYFunctionParameter = EtGpuPowerUsageLimit;
                }

                PowerUsageGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (PowerUsageGraphState.TooltipIndex != getTooltipText->Index)
                {
                    FLOAT powerUsage;
                    PH_FORMAT format[4];

                    powerUsage = PhGetItemCircularBuffer_FLOAT(&EtGpuPowerUsageHistory, getTooltipText->Index);

                    PhInitFormatF(&format[0], powerUsage, 1);
                    PhInitFormatC(&format[1], L'%');
                    PhInitFormatC(&format[2], L'\n');
                    PhInitFormatSR(&format[3], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&PowerUsageGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                getTooltipText->Text = PhGetStringRef(PowerUsageGraphState.TooltipText);
            }
        }
        break;
    }
}

VOID EtpNotifyTemperatureGraph(
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

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (EtEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
            GpuSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorTemperature"), 0);

            PhGraphStateGetDrawInfo(
                &TemperatureGraphState,
                getDrawInfo,
                EtGpuTemperatureHistory.Count
                );

            if (!TemperatureGraphState.Valid)
            {
                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    TemperatureGraphState.Data1[i] = PhGetItemCircularBuffer_FLOAT(&EtGpuTemperatureHistory, i);
                }

                PhDivideSinglesBySingle(
                    TemperatureGraphState.Data1,
                    EtGpuTemperatureLimit,
                    drawInfo->LineDataCount
                    );

                if (EtEnableScaleText)
                {
                    drawInfo->LabelYFunction = EtpTemperatureGraphLabelYFunction;
                    drawInfo->LabelYFunctionParameter = EtGpuTemperatureLimit;
                }

                TemperatureGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (TemperatureGraphState.TooltipIndex != getTooltipText->Index)
                {
                    FLOAT temp;
                    PH_FORMAT format[3];

                    temp = PhGetItemCircularBuffer_FLOAT(&EtGpuTemperatureHistory, getTooltipText->Index);

                    if (PhGetIntegerSetting(SETTING_NAME_ENABLE_FAHRENHEIT))
                    {
                        PhInitFormatF(&format[0], (temp * 1.8 + 32), 1);
                        PhInitFormatS(&format[1], L"\u00b0F\n");
                        PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);
                    }
                    else
                    {
                        PhInitFormatF(&format[0], temp, 1);
                        PhInitFormatS(&format[1], L"\u00b0C\n");
                        PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);
                    }

                    PhMoveReference(&TemperatureGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                getTooltipText->Text = PhGetStringRef(TemperatureGraphState.TooltipText);
            }
        }
        break;
    }
}

VOID EtpNotifyFanRpmGraph(
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

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (EtEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
            GpuSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorFanRpm"), 0);

            PhGraphStateGetDrawInfo(
                &FanRpmGraphState,
                getDrawInfo,
                EtGpuFanRpmHistory.Count
                );

            if (!FanRpmGraphState.Valid)
            {
                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FanRpmGraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG64(&EtGpuFanRpmHistory, i);
                }

                PhDivideSinglesBySingle(
                    FanRpmGraphState.Data1,
                    (FLOAT)EtGpuFanRpmLimit,
                    drawInfo->LineDataCount
                    );

                if (EtEnableScaleText)
                {
                    drawInfo->LabelYFunction = EtpFanRpmGraphLabelYFunction;
                    drawInfo->LabelYFunctionParameter = (FLOAT)EtGpuFanRpmLimit;
                }

                FanRpmGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (FanRpmGraphState.TooltipIndex != getTooltipText->Index)
                {
                    ULONG64 rpm;
                    PH_FORMAT format[3];

                    rpm = PhGetItemCircularBuffer_ULONG64(&EtGpuFanRpmHistory, getTooltipText->Index);

                    PhInitFormatI64U(&format[0], rpm);
                    PhInitFormatS(&format[1], L" RPM\n");
                    PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&FanRpmGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                getTooltipText->Text = PhGetStringRef(FanRpmGraphState.TooltipText);
            }
        }
        break;
    }

}

VOID EtpUpdateGpuGraphs(
    VOID
    )
{
    GpuGraphState.Valid = FALSE;
    GpuGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(GpuGraphHandle, 1);
    Graph_Draw(GpuGraphHandle);
    Graph_UpdateTooltip(GpuGraphHandle);
    InvalidateRect(GpuGraphHandle, NULL, FALSE);

    DedicatedGraphState.Valid = FALSE;
    DedicatedGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(DedicatedGraphHandle, 1);
    Graph_Draw(DedicatedGraphHandle);
    Graph_UpdateTooltip(DedicatedGraphHandle);
    InvalidateRect(DedicatedGraphHandle, NULL, FALSE);

    SharedGraphState.Valid = FALSE;
    SharedGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(SharedGraphHandle, 1);
    Graph_Draw(SharedGraphHandle);
    Graph_UpdateTooltip(SharedGraphHandle);
    InvalidateRect(SharedGraphHandle, NULL, FALSE);

    if (EtGpuSupported)
    {
        PowerUsageGraphState.Valid = FALSE;
        PowerUsageGraphState.TooltipIndex = ULONG_MAX;
        Graph_MoveGrid(PowerUsageGraphHandle, 1);
        Graph_Draw(PowerUsageGraphHandle);
        Graph_UpdateTooltip(PowerUsageGraphHandle);
        InvalidateRect(PowerUsageGraphHandle, NULL, FALSE);

        TemperatureGraphState.Valid = FALSE;
        TemperatureGraphState.TooltipIndex = ULONG_MAX;
        Graph_MoveGrid(TemperatureGraphHandle, 1);
        Graph_Draw(TemperatureGraphHandle);
        Graph_UpdateTooltip(TemperatureGraphHandle);
        InvalidateRect(TemperatureGraphHandle, NULL, FALSE);

        FanRpmGraphState.Valid = FALSE;
        FanRpmGraphState.TooltipIndex = ULONG_MAX;
        Graph_MoveGrid(FanRpmGraphHandle, 1);
        Graph_Draw(FanRpmGraphHandle);
        Graph_UpdateTooltip(FanRpmGraphHandle);
        InvalidateRect(FanRpmGraphHandle, NULL, FALSE);
    }
}

VOID EtpUpdateGpuPanel(
    VOID
    )
{
    PH_FORMAT format[1];
    WCHAR formatBuffer[512];

    PhInitFormatSize(&format[0], EtGpuDedicatedUsage);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(GpuPanelDedicatedUsageLabel, formatBuffer);
    else
        PhSetWindowText(GpuPanelDedicatedUsageLabel, PhaFormatSize(EtGpuDedicatedUsage, ULONG_MAX)->Buffer);

    PhInitFormatSize(&format[0], EtGpuDedicatedLimit);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(GpuPanelDedicatedLimitLabel, formatBuffer);
    else
        PhSetWindowText(GpuPanelDedicatedLimitLabel, PhaFormatSize(EtGpuDedicatedLimit, ULONG_MAX)->Buffer);

    PhInitFormatSize(&format[0], EtGpuSharedUsage);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(GpuPanelSharedUsageLabel, formatBuffer);
    else
        PhSetWindowText(GpuPanelSharedUsageLabel, PhaFormatSize(EtGpuSharedUsage, ULONG_MAX)->Buffer);

    PhInitFormatSize(&format[0], EtGpuSharedLimit);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(GpuPanelSharedLimitLabel, formatBuffer);
    else
        PhSetWindowText(GpuPanelSharedLimitLabel, PhaFormatSize(EtGpuSharedLimit, ULONG_MAX)->Buffer);
}

PPH_PROCESS_RECORD EtpReferenceMaxNodeRecord(
    _In_ LONG Index
    )
{
    LARGE_INTEGER time;
    ULONG maxProcessId;

    maxProcessId = PhGetItemCircularBuffer_ULONG(&EtMaxGpuNodeHistory, Index);

    if (!maxProcessId)
        return NULL;

    PhGetStatisticsTime(NULL, Index, &time);
    time.QuadPart += PH_TICKS_PER_SEC - 1;

    return PhFindProcessRecord(UlongToHandle(maxProcessId), &time);
}

PPH_STRING EtpGetMaxNodeString(
    _In_ LONG Index
    )
{
    PPH_PROCESS_RECORD maxProcessRecord;

    if (maxProcessRecord = EtpReferenceMaxNodeRecord(Index))
    {
        PPH_STRING maxUsageString;
        FLOAT maxGpuUsage;
        PH_FORMAT format[7];

        maxGpuUsage = PhGetItemCircularBuffer_FLOAT(&EtMaxGpuNodeUsageHistory, Index);

        // \n%s (%lu): %.2f%%
        PhInitFormatC(&format[0], L'\n');
        PhInitFormatSR(&format[1], maxProcessRecord->ProcessName->sr);
        PhInitFormatS(&format[2],L" (");
        PhInitFormatU(&format[3], HandleToUlong(maxProcessRecord->ProcessId));
        PhInitFormatS(&format[4], L"): ");
        PhInitFormatF(&format[5], maxGpuUsage * 100, 2);
        PhInitFormatC(&format[6], L'%');

        maxUsageString = PhFormat(format, RTL_NUMBER_OF(format), 0);

        PhDereferenceProcessRecord(maxProcessRecord);

        return maxUsageString;
    }

    return PhReferenceEmptyString();
}

PPH_STRING EtpGetGpuNameString(
    VOID
    )
{
    ULONG i;
    ULONG count;
    PH_STRING_BUILDER sb;

    count = EtGetGpuAdapterCount();
    PhInitializeStringBuilder(&sb, 100);

    for (i = 0; i < count; i++)
    {
        PPH_STRING description;

        description = EtGetGpuAdapterDescription(i);

        if (!PhIsNullOrEmptyString(description))
        {
            // Ignore "Microsoft Basic Render Driver" unless we don't have any other adapters.
            // This does not take into account localization.
            if (count == 1 || !PhEqualString2(description, L"Microsoft Basic Render Driver", TRUE))
            {
                PhAppendStringBuilder(&sb, &description->sr);
                PhAppendStringBuilder2(&sb, L", ");
            }
        }

        if (description)
            PhDereferenceObject(description);
    }

    if (sb.String->Length != 0)
        PhRemoveEndStringBuilder(&sb, 2);

    return PhFinalStringBuilderString(&sb);
}
