/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011
 *     dmex    2015-2023
 *     jxy-s   2024
 *
 */

#include "exttools.h"
#include "npusys.h"

static PPH_SYSINFO_SECTION NpuSection;
static HWND NpuDialog;
static LONG NpuDialogWindowDpi;
static PH_LAYOUT_MANAGER NpuLayoutManager;
static RECT NpuGraphMargin;
static HWND NpuGraphHandle;
static PH_GRAPH_STATE NpuGraphState;
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
static HWND NpuPanel;
static HWND NpuPanelDedicatedUsageLabel;
static HWND NpuPanelDedicatedLimitLabel;
static HWND NpuPanelSharedUsageLabel;
static HWND NpuPanelSharedLimitLabel;

VOID EtNpuSystemInformationInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers
    )
{
    PH_SYSINFO_SECTION section;

    memset(&section, 0, sizeof(PH_SYSINFO_SECTION));
    PhInitializeStringRef(&section.Name, L"NPU");
    section.Flags = 0;
    section.Callback = EtpNpuSysInfoSectionCallback;

    NpuSection = Pointers->CreateSection(&section);
}

BOOLEAN EtpNpuSysInfoSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2
    )
{
    switch (Message)
    {
    case SysInfoDestroy:
        {
            if (NpuDialog)
            {
                EtpUninitializeNpuDialog();
                NpuDialog = NULL;
            }
        }
        return TRUE;
    case SysInfoTick:
        {
            if (NpuDialog)
            {
                EtpTickNpuDialog();
            }
        }
        return TRUE;
    case SysInfoViewChanging:
        {
            PH_SYSINFO_VIEW_TYPE view = (PH_SYSINFO_VIEW_TYPE)PtrToUlong(Parameter1);
            PPH_SYSINFO_SECTION section = (PPH_SYSINFO_SECTION)Parameter2;

            if (view == SysInfoSummaryView || section != Section)
                return TRUE;

            if (NpuGraphHandle)
            {
                NpuGraphState.Valid = FALSE;
                NpuGraphState.TooltipIndex = ULONG_MAX;
                Graph_Draw(NpuGraphHandle);
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

            if (EtNpuSupported)
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

            createDialog->Instance = PluginInstance->DllBase;
            createDialog->Template = MAKEINTRESOURCE(IDD_SYSINFO_NPU);
            createDialog->DialogProc = EtpNpuDialogProc;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = Parameter1;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (EtEnableScaleText ? PH_GRAPH_LABEL_MAX_Y : 0);
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), 0, Section->Parameters->WindowDpi);
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, EtNpuNodeHistory.Count);

            if (!Section->GraphState.Valid)
            {
                PhCopyCircularBuffer_FLOAT(&EtNpuNodeHistory, Section->GraphState.Data1, drawInfo->LineDataCount);

                if (EtEnableScaleGraph)
                {
                    FLOAT max = 0;

                    if (EtEnableAvxSupport && drawInfo->LineDataCount > 128)
                    {
                        max = PhMaxMemorySingles(Section->GraphState.Data1, drawInfo->LineDataCount);
                    }
                    else
                    {
                        for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                        {
                            FLOAT data = Section->GraphState.Data1[i];

                            if (max < data)
                                max = data;
                        }
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

            gpu = PhGetItemCircularBuffer_FLOAT(&EtNpuNodeHistory, getTooltipText->Index);

            // %.2f%%%s\n%s
            PhInitFormatF(&format[0], gpu * 100, EtMaxPrecisionUnit);
            PhInitFormatC(&format[1], L'%');
            PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, EtpNpuGetMaxNodeString(getTooltipText->Index))->sr);
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

            drawPanel->Title = PhCreateString(L"NPU");

            // Note: Intel IGP devices don't have dedicated usage/limit values. (dmex)
            if (EtNpuDedicatedUsage && EtNpuDedicatedLimit)
            {
                PH_FORMAT format[5];

                // %.2f%%\n%s / %s
                PhInitFormatF(&format[0], EtNpuNodeUsage * 100, EtMaxPrecisionUnit);
                PhInitFormatS(&format[1], L"%\n");
                PhInitFormatSize(&format[2], EtNpuDedicatedUsage);
                PhInitFormatS(&format[3], L" / ");
                PhInitFormatSize(&format[4], EtNpuDedicatedLimit);

                drawPanel->SubTitle = PhFormat(format, 5, 64);

                // %.2f%%\n%s
                PhInitFormatF(&format[0], EtNpuNodeUsage * 100, EtMaxPrecisionUnit);
                PhInitFormatS(&format[1], L"%\n");
                PhInitFormatSize(&format[2], EtNpuDedicatedUsage);

                drawPanel->SubTitleOverflow = PhFormat(format, 3, 64);
            }
            else if (EtNpuSharedUsage && EtNpuSharedLimit)
            {
                PH_FORMAT format[5];

                // %.2f%%\n%s / %s
                PhInitFormatF(&format[0], EtNpuNodeUsage * 100, EtMaxPrecisionUnit);
                PhInitFormatS(&format[1], L"%\n");
                PhInitFormatSize(&format[2], EtNpuSharedUsage);
                PhInitFormatS(&format[3], L" / ");
                PhInitFormatSize(&format[4], EtNpuSharedLimit);

                drawPanel->SubTitle = PhFormat(format, 5, 64);

                // %.2f%%\n%s
                PhInitFormatF(&format[0], EtNpuNodeUsage * 100, 2);
                PhInitFormatS(&format[1], L"%\n");
                PhInitFormatSize(&format[2], EtNpuSharedUsage);

                drawPanel->SubTitleOverflow = PhFormat(format, 3, 64);
            }
            else
            {
                PH_FORMAT format[2];

                // %.2f%%\n
                PhInitFormatF(&format[0], EtNpuNodeUsage * 100, 2);
                PhInitFormatS(&format[1], L"%\n");

                drawPanel->SubTitle = PhFormat(format, RTL_NUMBER_OF(format), 0);
            }
        }
        return TRUE;
    }

    return FALSE;
}

VOID EtpInitializeNpuDialog(
    VOID
    )
{
    PhInitializeGraphState(&NpuGraphState);
    PhInitializeGraphState(&DedicatedGraphState);
    PhInitializeGraphState(&SharedGraphState);

    if (EtNpuSupported)
    {
        PhInitializeGraphState(&PowerUsageGraphState);
        PhInitializeGraphState(&TemperatureGraphState);
        PhInitializeGraphState(&FanRpmGraphState);
    }
}

VOID EtpUninitializeNpuDialog(
    VOID
    )
{
    PhDeleteGraphState(&NpuGraphState);
    PhDeleteGraphState(&DedicatedGraphState);
    PhDeleteGraphState(&SharedGraphState);

    if (EtNpuSupported)
    {
        PhDeleteGraphState(&PowerUsageGraphState);
        PhDeleteGraphState(&TemperatureGraphState);
        PhDeleteGraphState(&FanRpmGraphState);
    }

    // Note: Required for SysInfoViewChanging (dmex)
    NpuGraphHandle = NULL;
    DedicatedGraphHandle = NULL;
    SharedGraphHandle = NULL;
    PowerUsageGraphHandle = NULL;
    TemperatureGraphHandle = NULL;
    FanRpmGraphHandle = NULL;
}

VOID EtpTickNpuDialog(
    VOID
    )
{
    EtpUpdateNpuGraphs();
    EtpUpdateNpuPanel();
}

INT_PTR CALLBACK EtpNpuDialogProc(
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

            EtpInitializeNpuDialog();

            NpuDialog = hwndDlg;
            NpuDialogWindowDpi = PhGetWindowDpi(NpuDialog);

            PhInitializeLayoutManager(&NpuLayoutManager, hwndDlg);
            PhAddLayoutItem(&NpuLayoutManager, GetDlgItem(hwndDlg, IDC_NPUNAME), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            graphItem = PhAddLayoutItem(&NpuLayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&NpuLayoutManager, GetDlgItem(hwndDlg, IDC_PANEL_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            NpuGraphMargin = graphItem->Margin;

            SetWindowFont(GetDlgItem(hwndDlg, IDC_TITLE), NpuSection->Parameters->LargeFont, FALSE);
            SetWindowFont(GetDlgItem(hwndDlg, IDC_NPUNAME), NpuSection->Parameters->MediumFont, FALSE);

            PhSetDialogItemText(hwndDlg, IDC_NPUNAME, PH_AUTO_T(PH_STRING, EtpNpuGetNameString())->Buffer);

            NpuPanel = PhCreateDialog(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_SYSINFO_NPUPANEL), hwndDlg, EtpNpuPanelDialogProc, NULL);
            ShowWindow(NpuPanel, SW_SHOW);
            PhAddLayoutItemEx(&NpuLayoutManager, NpuPanel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, panelItem->Margin);

            EtpCreateNpuGraphs();
            EtpUpdateNpuGraphs();
            EtpUpdateNpuPanel();

            if (!EtNpuSupported)
            {
                ShowWindow(GetDlgItem(hwndDlg, IDC_POWER_USAGE_L), SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_TEMPERATURE_L), SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_FAN_RPM_L), SW_HIDE);
            }
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&NpuLayoutManager);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            NpuDialogWindowDpi = PhGetWindowDpi(NpuDialog);

            if (NpuSection->Parameters->LargeFont)
            {
                SetWindowFont(GetDlgItem(hwndDlg, IDC_TITLE), NpuSection->Parameters->LargeFont, FALSE);
            }

            if (NpuSection->Parameters->MediumFont)
            {
                SetWindowFont(GetDlgItem(hwndDlg, IDC_NPUNAME), NpuSection->Parameters->MediumFont, FALSE);
            }

            EtpLayoutNpuGraphs(hwndDlg);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&NpuLayoutManager);
            EtpLayoutNpuGraphs(hwndDlg);
        }
        break;
    case WM_NOTIFY:
        {
            NMHDR *header = (NMHDR *)lParam;

            if (header->hwndFrom == NpuGraphHandle)
            {
                EtpNotifyNpuGraph(header);
            }
            else if (header->hwndFrom == DedicatedGraphHandle)
            {
                EtpNotifyDedicatedNpuGraph(header);
            }
            else if (header->hwndFrom == SharedGraphHandle)
            {
                EtpNotifySharedNpuGraph(header);
            }
            else if (header->hwndFrom == PowerUsageGraphHandle)
            {
                EtpNotifyPowerUsageNpuGraph(header);
            }
            else if (header->hwndFrom == TemperatureGraphHandle)
            {
                EtpNotifyTemperatureNpuGraph(header);
            }
            else if (header->hwndFrom == FanRpmGraphHandle)
            {
                EtpNotifyFanRpmNpuGraph(header);
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

INT_PTR CALLBACK EtpNpuPanelDialogProc(
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
            NpuPanelDedicatedUsageLabel = GetDlgItem(hwndDlg, IDC_ZDEDICATEDCURRENT_V);
            NpuPanelDedicatedLimitLabel = GetDlgItem(hwndDlg, IDC_ZDEDICATEDLIMIT_V);
            NpuPanelSharedUsageLabel = GetDlgItem(hwndDlg, IDC_ZSHAREDCURRENT_V);
            NpuPanelSharedLimitLabel = GetDlgItem(hwndDlg, IDC_ZSHAREDLIMIT_V);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_NODES:
                EtShowNpuNodesDialog(NpuDialog);
                break;
            case IDC_DETAILS:
                EtShowNpuDetailsDialog(NpuDialog);
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

VOID EtpCreateNpuGraphs(
    VOID
    )
{
    NpuGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        NpuDialog,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(NpuGraphHandle, TRUE);

    DedicatedGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        NpuDialog,
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
        NpuDialog,
        NULL,
        NULL,
        NULL
        );
    Graph_SetTooltip(SharedGraphHandle, TRUE);

    if (EtNpuSupported)
    {
        PowerUsageGraphHandle = CreateWindow(
            PH_GRAPH_CLASSNAME,
            NULL,
            WS_VISIBLE | WS_CHILD | WS_BORDER,
            0,
            0,
            3,
            3,
            NpuDialog,
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
            NpuDialog,
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
            NpuDialog,
            NULL,
            NULL,
            NULL
            );
        Graph_SetTooltip(FanRpmGraphHandle, TRUE);
    }
}

VOID EtpLayoutNpuGraphs(
    _In_ HWND hwnd
    )
{
    RECT clientRect;
    RECT labelRect;
    RECT marginRect;
    ULONG graphWidth;
    ULONG graphHeight;
    HDWP deferHandle;
    ULONG y;
    LONG graphPadding;

    NpuGraphState.Valid = FALSE;
    NpuGraphState.TooltipIndex = ULONG_MAX;
    DedicatedGraphState.Valid = FALSE;
    DedicatedGraphState.TooltipIndex = ULONG_MAX;
    SharedGraphState.Valid = FALSE;
    SharedGraphState.TooltipIndex = ULONG_MAX;

    if (EtNpuSupported)
    {
        PowerUsageGraphState.Valid = FALSE;
        PowerUsageGraphState.TooltipIndex = ULONG_MAX;
        TemperatureGraphState.Valid = FALSE;
        TemperatureGraphState.TooltipIndex = ULONG_MAX;
        FanRpmGraphState.Valid = FALSE;
        FanRpmGraphState.TooltipIndex = ULONG_MAX;
    }

    marginRect = NpuGraphMargin;
    PhGetSizeDpiValue(&marginRect, NpuDialogWindowDpi, TRUE);
    graphPadding = PhGetDpi(ET_NPU_PADDING, NpuDialogWindowDpi);

    GetClientRect(NpuDialog, &clientRect);
    GetClientRect(GetDlgItem(NpuDialog, IDC_NPU_L), &labelRect);
    graphWidth = clientRect.right - marginRect.left - marginRect.right;

    if (EtNpuSupported)
        graphHeight = (clientRect.bottom - marginRect.top - marginRect.bottom - labelRect.bottom * 6 - graphPadding * 8) / 6;
    else
        graphHeight = (clientRect.bottom - marginRect.top - marginRect.bottom - labelRect.bottom * 3 - graphPadding * 5) / 3;

    deferHandle = BeginDeferWindowPos(12);
    y = marginRect.top;

    deferHandle = DeferWindowPos(
        deferHandle,
        GetDlgItem(NpuDialog, IDC_NPU_L),
        NULL,
        marginRect.left,
        y,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += labelRect.bottom + graphPadding;

    deferHandle = DeferWindowPos(
        deferHandle,
        NpuGraphHandle,
        NULL,
        marginRect.left,
        y,
        graphWidth,
        graphHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += graphHeight + graphPadding;

    deferHandle = DeferWindowPos(
        deferHandle,
        GetDlgItem(NpuDialog, IDC_DEDICATED_L),
        NULL,
        marginRect.left,
        y,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += labelRect.bottom + graphPadding;

    deferHandle = DeferWindowPos(
        deferHandle,
        DedicatedGraphHandle,
        NULL,
        marginRect.left,
        y,
        graphWidth,
        graphHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += graphHeight + graphPadding;

    deferHandle = DeferWindowPos(
        deferHandle,
        GetDlgItem(NpuDialog, IDC_SHARED_L),
        NULL,
        marginRect.left,
        y,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += labelRect.bottom + graphPadding;

    deferHandle = DeferWindowPos(
        deferHandle,
        SharedGraphHandle,
        NULL,
        marginRect.left,
        y,
        graphWidth,
        graphHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += graphHeight + graphPadding;

    if (EtNpuSupported)
    {
        deferHandle = DeferWindowPos(
            deferHandle,
            GetDlgItem(NpuDialog, IDC_POWER_USAGE_L),
            NULL,
            marginRect.left,
            y,
            0,
            0,
            SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
            );
        y += labelRect.bottom + graphPadding;

        deferHandle = DeferWindowPos(
            deferHandle,
            PowerUsageGraphHandle,
            NULL,
            marginRect.left,
            y,
            graphWidth,
            graphHeight,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
        y += graphHeight + graphPadding;

        deferHandle = DeferWindowPos(
            deferHandle,
            GetDlgItem(NpuDialog, IDC_TEMPERATURE_L),
            NULL,
            marginRect.left,
            y,
            0,
            0,
            SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
            );
        y += labelRect.bottom + graphPadding;

        deferHandle = DeferWindowPos(
            deferHandle,
            TemperatureGraphHandle,
            NULL,
            marginRect.left,
            y,
            graphWidth,
            graphHeight,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
        y += graphHeight + graphPadding;

        deferHandle = DeferWindowPos(
            deferHandle,
            GetDlgItem(NpuDialog, IDC_FAN_RPM_L),
            NULL,
            marginRect.left,
            y,
            0,
            0,
            SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
            );
        y += labelRect.bottom + graphPadding;

        deferHandle = DeferWindowPos(
            deferHandle,
            FanRpmGraphHandle,
            NULL,
            marginRect.left,
            y,
            graphWidth,
            clientRect.bottom - marginRect.bottom - y,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
    }

    EndDeferWindowPos(deferHandle);
}

PPH_STRING EtpNpuPowerUsageGraphLabelYFunction(
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

PPH_STRING EtpNpuTemperatureGraphLabelYFunction(
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ ULONG DataIndex,
    _In_ FLOAT Value,
    _In_ FLOAT Parameter
    )
{
    PH_FORMAT format[2];

    if (EtNpuFahrenheitEnabled)
    {
        PhInitFormatF(&format[0], (ULONG)(Value * Parameter) * 1.8 + 32, 1);
        PhInitFormatS(&format[1], L"\u00b0F");
    }
    else
    {
        PhInitFormatF(&format[0], (ULONG)(Value * Parameter), 1);
        PhInitFormatS(&format[1], L"\u00b0C\n");
    }

    return PhFormat(format, RTL_NUMBER_OF(format), 0);
}

PPH_STRING EtpNpuFanRpmGraphLabelYFunction(
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

VOID EtpNotifyNpuGraph(
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
            NpuSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), 0, NpuSection->Parameters->WindowDpi);

            PhGraphStateGetDrawInfo(
                &NpuGraphState,
                getDrawInfo,
                EtNpuNodeHistory.Count
                );

            if (!NpuGraphState.Valid)
            {
                PhCopyCircularBuffer_FLOAT(&EtNpuNodeHistory, NpuGraphState.Data1, drawInfo->LineDataCount);

                if (EtEnableScaleGraph)
                {
                    FLOAT max = 0;

                    for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                    {
                        FLOAT data = NpuGraphState.Data1[i]; // HACK

                        if (max < data)
                            max = data;
                    }

                    if (max != 0)
                    {
                        PhDivideSinglesBySingle(NpuGraphState.Data1, max, drawInfo->LineDataCount);
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

                NpuGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (NpuGraphState.TooltipIndex != getTooltipText->Index)
                {
                    FLOAT gpu;
                    PH_FORMAT format[5];

                    gpu = PhGetItemCircularBuffer_FLOAT(&EtNpuNodeHistory, getTooltipText->Index);

                    // %.2f%%%s\n%s
                    PhInitFormatF(&format[0], gpu * 100, EtMaxPrecisionUnit);
                    PhInitFormatC(&format[1], L'%');
                    PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, EtpNpuGetMaxNodeString(getTooltipText->Index))->sr);
                    PhInitFormatC(&format[3], L'\n');
                    PhInitFormatSR(&format[4], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&NpuGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                }

                getTooltipText->Text = PhGetStringRef(NpuGraphState.TooltipText);
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
                record = EtpNpuReferenceMaxNodeRecord(mouseEvent->Index);
            }

            if (record)
            {
                PhShowProcessRecordDialog(NpuDialog, record);
                PhDereferenceProcessRecord(record);
            }
        }
        break;
    }
}

VOID EtpNotifyDedicatedNpuGraph(
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
            NpuSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorPrivate"), 0, NpuSection->Parameters->WindowDpi);

            PhGraphStateGetDrawInfo(
                &DedicatedGraphState,
                getDrawInfo,
                EtNpuDedicatedHistory.Count
                );

            if (!DedicatedGraphState.Valid)
            {
                FLOAT max = 0;

                if (EtNpuDedicatedLimit != 0 && !EtEnableScaleGraph)
                {
                    max = (FLOAT)EtNpuDedicatedLimit;
                }

                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data = DedicatedGraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG64(&EtNpuDedicatedHistory, i);

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

                    usedPages = PhGetItemCircularBuffer_ULONG64(&EtNpuDedicatedHistory, getTooltipText->Index);

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

VOID EtpNotifySharedNpuGraph(
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
            NpuSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorPhysical"), 0, NpuSection->Parameters->WindowDpi);

            PhGraphStateGetDrawInfo(
                &SharedGraphState,
                getDrawInfo,
                EtNpuSharedHistory.Count
                );

            if (!SharedGraphState.Valid)
            {
                FLOAT max = 0;

                //if (EtNpuSharedLimit != 0 && !EtEnableScaleGraph)
                //{
                //    max = (FLOAT)EtNpuSharedLimit;
                //}

                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data = SharedGraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG64(&EtNpuSharedHistory, i);

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

                    usedPages = PhGetItemCircularBuffer_ULONG64(&EtNpuSharedHistory, getTooltipText->Index);

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

VOID EtpNotifyPowerUsageNpuGraph(
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
            NpuSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorPowerUsage"), 0, NpuSection->Parameters->WindowDpi);

            PhGraphStateGetDrawInfo(
                &PowerUsageGraphState,
                getDrawInfo,
                EtNpuPowerUsageHistory.Count
                );

            if (!PowerUsageGraphState.Valid)
            {
                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    PowerUsageGraphState.Data1[i] = PhGetItemCircularBuffer_FLOAT(&EtNpuPowerUsageHistory, i);
                }

                PhDivideSinglesBySingle(
                    PowerUsageGraphState.Data1,
                    EtNpuPowerUsageLimit,
                    drawInfo->LineDataCount
                    );

                if (EtEnableScaleText)
                {
                    drawInfo->LabelYFunction = EtpNpuPowerUsageGraphLabelYFunction;
                    drawInfo->LabelYFunctionParameter = EtNpuPowerUsageLimit;
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

                    powerUsage = PhGetItemCircularBuffer_FLOAT(&EtNpuPowerUsageHistory, getTooltipText->Index);

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

VOID EtpNotifyTemperatureNpuGraph(
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
            NpuSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorTemperature"), 0, NpuSection->Parameters->WindowDpi);

            PhGraphStateGetDrawInfo(
                &TemperatureGraphState,
                getDrawInfo,
                EtNpuTemperatureHistory.Count
                );

            if (!TemperatureGraphState.Valid)
            {
                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    TemperatureGraphState.Data1[i] = PhGetItemCircularBuffer_FLOAT(&EtNpuTemperatureHistory, i);
                }

                PhDivideSinglesBySingle(
                    TemperatureGraphState.Data1,
                    EtNpuTemperatureLimit,
                    drawInfo->LineDataCount
                    );

                if (EtEnableScaleText)
                {
                    drawInfo->LabelYFunction = EtpNpuTemperatureGraphLabelYFunction;
                    drawInfo->LabelYFunctionParameter = EtNpuTemperatureLimit;
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

                    temp = PhGetItemCircularBuffer_FLOAT(&EtNpuTemperatureHistory, getTooltipText->Index);

                    if (EtNpuFahrenheitEnabled)
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

VOID EtpNotifyFanRpmNpuGraph(
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
            NpuSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorFanRpm"), 0, NpuSection->Parameters->WindowDpi);

            PhGraphStateGetDrawInfo(
                &FanRpmGraphState,
                getDrawInfo,
                EtNpuFanRpmHistory.Count
                );

            if (!FanRpmGraphState.Valid)
            {
                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FanRpmGraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG64(&EtNpuFanRpmHistory, i);
                }

                PhDivideSinglesBySingle(
                    FanRpmGraphState.Data1,
                    (FLOAT)EtNpuFanRpmLimit,
                    drawInfo->LineDataCount
                    );

                if (EtEnableScaleText)
                {
                    drawInfo->LabelYFunction = EtpNpuFanRpmGraphLabelYFunction;
                    drawInfo->LabelYFunctionParameter = (FLOAT)EtNpuFanRpmLimit;
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

                    rpm = PhGetItemCircularBuffer_ULONG64(&EtNpuFanRpmHistory, getTooltipText->Index);

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

VOID EtpUpdateNpuGraphs(
    VOID
    )
{
    NpuGraphState.Valid = FALSE;
    NpuGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(NpuGraphHandle, 1);
    Graph_Draw(NpuGraphHandle);
    Graph_UpdateTooltip(NpuGraphHandle);
    InvalidateRect(NpuGraphHandle, NULL, FALSE);

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

    if (EtNpuSupported)
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

VOID EtpUpdateNpuPanel(
    VOID
    )
{
    PH_FORMAT format[1];
    WCHAR formatBuffer[512];

    PhInitFormatSize(&format[0], EtNpuDedicatedUsage);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(NpuPanelDedicatedUsageLabel, formatBuffer);
    else
        PhSetWindowText(NpuPanelDedicatedUsageLabel, PhaFormatSize(EtNpuDedicatedUsage, ULONG_MAX)->Buffer);

    PhInitFormatSize(&format[0], EtNpuDedicatedLimit);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(NpuPanelDedicatedLimitLabel, formatBuffer);
    else
        PhSetWindowText(NpuPanelDedicatedLimitLabel, PhaFormatSize(EtNpuDedicatedLimit, ULONG_MAX)->Buffer);

    PhInitFormatSize(&format[0], EtNpuSharedUsage);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(NpuPanelSharedUsageLabel, formatBuffer);
    else
        PhSetWindowText(NpuPanelSharedUsageLabel, PhaFormatSize(EtNpuSharedUsage, ULONG_MAX)->Buffer);

    PhInitFormatSize(&format[0], EtNpuSharedLimit);

    if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(NpuPanelSharedLimitLabel, formatBuffer);
    else
        PhSetWindowText(NpuPanelSharedLimitLabel, PhaFormatSize(EtNpuSharedLimit, ULONG_MAX)->Buffer);
}

PPH_PROCESS_RECORD EtpNpuReferenceMaxNodeRecord(
    _In_ LONG Index
    )
{
    LARGE_INTEGER time;
    ULONG maxProcessId;

    maxProcessId = PhGetItemCircularBuffer_ULONG(&EtMaxNpuNodeHistory, Index);

    if (!maxProcessId)
        return NULL;

    PhGetStatisticsTime(NULL, Index, &time);
    time.QuadPart += PH_TICKS_PER_SEC - 1;

    return PhFindProcessRecord(UlongToHandle(maxProcessId), &time);
}

PPH_STRING EtpNpuGetMaxNodeString(
    _In_ LONG Index
    )
{
    PPH_PROCESS_RECORD maxProcessRecord;

    if (maxProcessRecord = EtpNpuReferenceMaxNodeRecord(Index))
    {
        PPH_STRING maxUsageString;
        FLOAT maxNpuUsage;
        PH_FORMAT format[7];

        maxNpuUsage = PhGetItemCircularBuffer_FLOAT(&EtMaxNpuNodeUsageHistory, Index);

        // \n%s (%lu): %.2f%%
        PhInitFormatC(&format[0], L'\n');
        PhInitFormatSR(&format[1], maxProcessRecord->ProcessName->sr);
        PhInitFormatS(&format[2],L" (");
        PhInitFormatU(&format[3], HandleToUlong(maxProcessRecord->ProcessId));
        PhInitFormatS(&format[4], L"): ");
        PhInitFormatF(&format[5], maxNpuUsage * 100, EtMaxPrecisionUnit);
        PhInitFormatC(&format[6], L'%');

        maxUsageString = PhFormat(format, RTL_NUMBER_OF(format), 0);

        PhDereferenceProcessRecord(maxProcessRecord);

        return maxUsageString;
    }

    return PhReferenceEmptyString();
}

PPH_STRING EtpNpuGetNameString(
    VOID
    )
{
    ULONG i;
    ULONG count;
    PH_STRING_BUILDER sb;

    count = EtGetNpuAdapterCount();
    PhInitializeStringBuilder(&sb, 100);

    for (i = 0; i < count; i++)
    {
        PPH_STRING description;

        description = EtGetNpuAdapterDescription(i);

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
