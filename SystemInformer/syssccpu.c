/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2016
 *     dmex    2017-2023
 *
 */

#include <phapp.h>
#include <settings.h>
#include <sysinfo.h>
#include <sysinfop.h>
#include <procprv.h>
#include <phsettings.h>
#include <phfirmware.h>
#include <devguid.h>

static PPH_SYSINFO_SECTION CpuSection;
static HWND CpuDialog;
static PH_LAYOUT_MANAGER CpuLayoutManager;
static RECT CpuGraphMargin;
static HWND CpuGraphHandle;
static PH_GRAPH_STATE CpuGraphState;
static HWND *CpusGraphHandle;
static PPH_GRAPH_STATE CpusGraphState;
static BOOLEAN OneGraphPerCpu;
static HWND CpuPanel;
static ULONG CpuTicked;
static ULONG CpuMaxMhz;
static ULONG NumberOfProcessors;
static FLOAT CpuTimerResolution;
static PFLOAT CpuFrequencyInformation;
static FLOAT CpuCurrentFrequency;
static PSYSTEM_INTERRUPT_INFORMATION InterruptInformation;
static PPROCESSOR_POWER_INFORMATION PowerInformation;
static PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION CurrentPerformanceDistribution;
static PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION PreviousPerformanceDistribution;
static PH_LOGICAL_PROCESSOR_INFORMATION LogicalProcessorInformation;
static PH_UINT32_DELTA ContextSwitchesDelta;
static PH_UINT32_DELTA InterruptsDelta;
static PH_UINT64_DELTA DpcsDelta;
static PH_UINT32_DELTA SystemCallsDelta;
static HWND CpuPanelUtilizationLabel;
static HWND CpuPanelSpeedLabel;
static HWND CpuVirtualizationLabel;
static HWND CpuPanelProcessesLabel;
static HWND CpuPanelThreadsLabel;
static HWND CpuPanelHandlesLabel;
static HWND CpuPanelUptimeLabel;
static HWND CpuPanelContextSwitchesLabel;
static HWND CpuPanelInterruptDeltaLabel;
static HWND CpuPanelDpcDeltaLabel;
static HWND CpuPanelSystemCallsDeltaLabel;
static HWND CpuPanelCoresLabel;
static HWND CpuPanelSocketsLabel;
static HWND CpuPanelLogicalLabel;
static HWND CpuPanelLatencyLabel;
static ULONG64 CpuL1CacheSize;
static ULONG64 CpuL2CacheSize;
static ULONG64 CpuL3CacheSize;

_Function_class_(PH_SYSINFO_SECTION_CALLBACK)
BOOLEAN PhSipCpuSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2
    )
{
    switch (Message)
    {
    case SysInfoCreate:
        {
            CpuSection = Section;
        }
        return TRUE;
    case SysInfoDestroy:
        {
            if (CpuDialog)
            {
                PhSipUninitializeCpuDialog();
                CpuDialog = NULL;
            }
        }
        return TRUE;
    case SysInfoTick:
        {
            if (CpuDialog)
            {
                PhSipTickCpuDialog();
            }
        }
        return TRUE;
    case SysInfoViewChanging:
        {
            PH_SYSINFO_VIEW_TYPE view = (PH_SYSINFO_VIEW_TYPE)PtrToUlong(Parameter1);
            PPH_SYSINFO_SECTION section = (PPH_SYSINFO_SECTION)Parameter2;

            if (view == SysInfoSummaryView || section != Section)
                return TRUE;

            if (OneGraphPerCpu)
            {
                for (ULONG i = 0; i < NumberOfProcessors; i++)
                {
                    if (CpusGraphHandle && CpusGraphHandle[i] && CpusGraphState)
                    {
                        CpusGraphState[i].Valid = FALSE;
                        CpusGraphState[i].TooltipIndex = ULONG_MAX;
                        Graph_Draw(CpusGraphHandle[i]);
                    }
                }
            }
            else
            {
                if (CpuGraphHandle)
                {
                    CpuGraphState.Valid = FALSE;
                    CpuGraphState.TooltipIndex = ULONG_MAX;
                    Graph_Draw(CpuGraphHandle);
                }
            }
        }
        return TRUE;
    case SysInfoCreateDialog:
        {
            PPH_SYSINFO_CREATE_DIALOG createDialog = Parameter1;

            createDialog->Instance = PhInstanceHandle;
            createDialog->Template = MAKEINTRESOURCE(IDD_SYSINFO_CPU);
            createDialog->DialogProc = PhSipCpuDialogProc;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = Parameter1;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_USE_LINE_2 | PH_GRAPH_LABEL_MAX_Y;
            Section->Parameters->ColorSetupFunction(drawInfo, PhCsColorCpuKernel, PhCsColorCpuUser, Section->Parameters->WindowDpi);
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, PhCpuKernelHistory.Count);

            if (!Section->GraphState.Valid)
            {
                PhCopyCircularBuffer_FLOAT(&PhCpuKernelHistory, Section->GraphState.Data1, drawInfo->LineDataCount);
                PhCopyCircularBuffer_FLOAT(&PhCpuUserHistory, Section->GraphState.Data2, drawInfo->LineDataCount);

                if (PhCsEnableGraphMaxScale)
                {
                    FLOAT max = 0;

                    if (PhCsEnableAvxSupport && drawInfo->LineDataCount > 128)
                    {
                        max = PhAddPlusMaxMemorySingles(
                            Section->GraphState.Data1,
                            Section->GraphState.Data2,
                            drawInfo->LineDataCount
                            );
                    }
                    else
                    {
                        for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                        {
                            FLOAT data = Section->GraphState.Data1[i] + Section->GraphState.Data2[i];

                            if (max < data)
                                max = data;
                        }
                    }

                    if (max != 0)
                    {
                        PhDivideSinglesBySingle(Section->GraphState.Data1, max, drawInfo->LineDataCount);
                        PhDivideSinglesBySingle(Section->GraphState.Data2, max, drawInfo->LineDataCount);
                    }

                    drawInfo->LabelYFunction = PhSiDoubleLabelYFunction;
                    drawInfo->LabelYFunctionParameter = max;
                }
                else
                {
                    drawInfo->LabelYFunction = PhSiDoubleLabelYFunction;
                    drawInfo->LabelYFunctionParameter = 1.0f;
                }

                Section->GraphState.Valid = TRUE;
            }
        }
        return TRUE;
    case SysInfoGraphGetTooltipText:
        {
            PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT getTooltipText = Parameter1;
            FLOAT cpuKernel;
            FLOAT cpuUser;
            PH_FORMAT format[9];

            cpuKernel = PhGetItemCircularBuffer_FLOAT(&PhCpuKernelHistory, getTooltipText->Index);
            cpuUser = PhGetItemCircularBuffer_FLOAT(&PhCpuUserHistory, getTooltipText->Index);

            // %.2f%% (K: %.2f%%, U: %.2f%%)\n%s\n%s
            PhInitFormatF(&format[0], (cpuKernel + cpuUser) * 100, PhMaxPrecisionUnit);
            PhInitFormatS(&format[1], L"% (K: ");
            PhInitFormatF(&format[2], cpuKernel * 100, PhMaxPrecisionUnit);
            PhInitFormatS(&format[3], L"%, U: ");
            PhInitFormatF(&format[4], cpuUser * 100, PhMaxPrecisionUnit);
            PhInitFormatS(&format[5], L"%)");
            PhInitFormatSR(&format[6], PH_AUTO_T(PH_STRING, PhSipGetMaxCpuString(getTooltipText->Index))->sr);
            PhInitFormatC(&format[7], L'\n');
            PhInitFormatSR(&format[8], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

            PhMoveReference(&Section->GraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 160));
            getTooltipText->Text = Section->GraphState.TooltipText->sr;
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = Parameter1;
            PH_FORMAT format[2];

            // %.2f%%
            PhInitFormatF(&format[0], (PhCpuKernelUsage + PhCpuUserUsage) * 100, PhMaxPrecisionUnit);
            PhInitFormatC(&format[1], L'%');

            drawPanel->Title = PhCreateString(L"CPU");
            drawPanel->SubTitle = PhFormat(format, RTL_NUMBER_OF(format), 16);
        }
        return TRUE;
    }

    return FALSE;
}

VOID PhSipInitializeCpuDialog(
    VOID
    )
{
    PhInitializeDelta(&ContextSwitchesDelta);
    PhInitializeDelta(&InterruptsDelta);
    PhInitializeDelta(&DpcsDelta);
    PhInitializeDelta(&SystemCallsDelta);

    NumberOfProcessors = PhSystemProcessorInformation.NumberOfProcessors;
    CpusGraphHandle = PhAllocate(sizeof(HWND) * NumberOfProcessors);
    CpusGraphState = PhAllocate(sizeof(PH_GRAPH_STATE) * NumberOfProcessors);
    InterruptInformation = PhAllocate(sizeof(SYSTEM_INTERRUPT_INFORMATION) * NumberOfProcessors);
    PowerInformation = PhAllocate(sizeof(PROCESSOR_POWER_INFORMATION) * NumberOfProcessors);

    if (PhGetIntegerSetting(SETTING_SYSINFO_SHOW_CPU_SPEED_PER_CPU))
    {
        CpuFrequencyInformation = PhAllocateZero(sizeof(FLOAT) * NumberOfProcessors);
    }

    PhInitializeGraphState(&CpuGraphState);

    for (ULONG i = 0; i < NumberOfProcessors; i++)
        PhInitializeGraphState(&CpusGraphState[i]);

    CpuTicked = 0;
    CpuMaxMhz = 0;

    PhSipUpdateTimerResolution();
    PhSipUpdateProcessorInformation();
    PhSipUpdateProcessorFrequency();

    CurrentPerformanceDistribution = NULL;
    PhGetSystemProcessorPerformanceDistribution(&CurrentPerformanceDistribution);

    PhGetSystemLogicalProcessorRelationInformation(&LogicalProcessorInformation);
}

VOID PhSipUninitializeCpuDialog(
    VOID
    )
{
    ULONG i;

    PhDeleteGraphState(&CpuGraphState);

    for (i = 0; i < NumberOfProcessors; i++)
        PhDeleteGraphState(&CpusGraphState[i]);

    PhFree(CpusGraphHandle);
    PhFree(CpusGraphState);
    PhFree(InterruptInformation);
    PhFree(PowerInformation);

    // Note: Required for SysInfoViewChanging (dmex)
    NumberOfProcessors = 0;
    CpuGraphHandle = NULL;
    CpusGraphHandle = NULL;
    CpusGraphState = NULL;
    InterruptInformation = NULL;
    PowerInformation = NULL;

    if (CurrentPerformanceDistribution)
    {
        PhFree(CurrentPerformanceDistribution);
        CurrentPerformanceDistribution = NULL;
    }
    if (PreviousPerformanceDistribution)
    {
        PhFree(PreviousPerformanceDistribution);
        PreviousPerformanceDistribution = NULL;
    }

    PhSetIntegerSetting(SETTING_SYSINFO_WINDOW_ONE_GRAPH_PER_CPU, OneGraphPerCpu);
}

VOID PhSipTickCpuDialog(
    VOID
    )
{
    ULONG64 dpcCount;

    PhSipUpdateInterruptInformation(&dpcCount);

    PhUpdateDelta(&ContextSwitchesDelta, PhPerfInformation.ContextSwitches);
    PhUpdateDelta(&InterruptsDelta, PhCpuTotals.InterruptCount);
    PhUpdateDelta(&DpcsDelta, dpcCount);
    PhUpdateDelta(&SystemCallsDelta, PhPerfInformation.SystemCalls);

    PhSipUpdateTimerResolution();
    PhSipUpdateProcessorInformation();
    PhSipUpdateProcessorPerformanceDistribution();
    //PhGetSystemLogicalProcessorRelationInformation(&LogicalProcessorInformation);

    if (CpuTicked < 2)
        CpuTicked++;

    PhSipUpdateCpuGraphs();
    PhSipUpdateCpuPanel();
}

INT_PTR CALLBACK PhSipCpuDialogProc(
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
            PPH_STRING brandString;
            HWND labelHandle;

            PhSipInitializeCpuDialog();

            CpuDialog = hwndDlg;
            labelHandle = GetDlgItem(hwndDlg, IDC_CPUNAME);

            PhInitializeLayoutManager(&CpuLayoutManager, hwndDlg);
            PhAddLayoutItem(&CpuLayoutManager, labelHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            graphItem = PhAddLayoutItem(&CpuLayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&CpuLayoutManager, GetDlgItem(hwndDlg, IDC_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            CpuGraphMargin = graphItem->Margin;

            SetWindowFont(GetDlgItem(hwndDlg, IDC_TITLE), CpuSection->Parameters->LargeFont, FALSE);
            SetWindowFont(labelHandle, CpuSection->Parameters->MediumFont, FALSE);

            brandString = PhSipGetCpuBrandString();
            PhSetWindowText(labelHandle, PhGetStringOrEmpty(brandString));
            PhClearReference(&brandString);

            CpuPanel = PhCreateDialog(PhInstanceHandle, MAKEINTRESOURCE(IDD_SYSINFO_CPUPANEL), hwndDlg, PhSipCpuPanelDialogProc, NULL);
            ShowWindow(CpuPanel, SW_SHOW);
            PhAddLayoutItemEx(&CpuLayoutManager, CpuPanel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, &panelItem->Margin);

            PhSipCreateCpuGraphs();

            if (NumberOfProcessors != 1)
            {
                OneGraphPerCpu = (BOOLEAN)PhGetIntegerSetting(SETTING_SYSINFO_WINDOW_ONE_GRAPH_PER_CPU);
                Button_SetCheck(GetDlgItem(CpuPanel, IDC_ONEGRAPHPERCPU), OneGraphPerCpu ? BST_CHECKED : BST_UNCHECKED);
                PhSipSetOneGraphPerCpu();
            }
            else
            {
                OneGraphPerCpu = FALSE;
                EnableWindow(GetDlgItem(CpuPanel, IDC_ONEGRAPHPERCPU), FALSE);
                PhSipSetOneGraphPerCpu();
            }

            PhSipUpdateCpuGraphs();
            PhSipUpdateCpuPanel();
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&CpuLayoutManager);
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            if (CpuSection->Parameters->LargeFont)
            {
                SetWindowFont(GetDlgItem(hwndDlg, IDC_TITLE), CpuSection->Parameters->LargeFont, FALSE);
            }

            if (CpuSection->Parameters->MediumFont)
            {
                SetWindowFont(GetDlgItem(hwndDlg, IDC_CPUNAME), CpuSection->Parameters->MediumFont, FALSE);
            }

            if (OneGraphPerCpu)
            {
                for (ULONG i = 0; i < NumberOfProcessors; i++)
                {
                    CpusGraphState[i].Valid = FALSE;
                    CpusGraphState[i].TooltipIndex = ULONG_MAX;
                }
            }
            else
            {
                CpuGraphState.Valid = FALSE;
                CpuGraphState.TooltipIndex = ULONG_MAX;
            }

            PhLayoutManagerUpdate(&CpuLayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&CpuLayoutManager);
            PhSipLayoutCpuGraphs();
        }
        break;
    case WM_SIZE:
        {
            if (OneGraphPerCpu)
            {
                for (ULONG i = 0; i < NumberOfProcessors; i++)
                {
                    CpusGraphState[i].Valid = FALSE;
                    CpusGraphState[i].TooltipIndex = ULONG_MAX;
                }
            }
            else
            {
                CpuGraphState.Valid = FALSE;
                CpuGraphState.TooltipIndex = ULONG_MAX;
            }

            PhLayoutManagerLayout(&CpuLayoutManager);
            PhSipLayoutCpuGraphs();
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

INT_PTR CALLBACK PhSipCpuPanelDialogProc(
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
            CpuL1CacheSize = 0;
            CpuL2CacheSize = 0;
            CpuL3CacheSize = 0;

            CpuPanelUtilizationLabel = GetDlgItem(hwndDlg, IDC_UTILIZATION);
            CpuPanelSpeedLabel = GetDlgItem(hwndDlg, IDC_SPEED);
            CpuVirtualizationLabel = GetDlgItem(hwndDlg, IDC_VIRTUALIZATION);
            CpuPanelProcessesLabel = GetDlgItem(hwndDlg, IDC_ZPROCESSES_V);
            CpuPanelThreadsLabel = GetDlgItem(hwndDlg, IDC_ZTHREADS_V);
            CpuPanelHandlesLabel = GetDlgItem(hwndDlg, IDC_ZHANDLES_V);
            CpuPanelUptimeLabel = GetDlgItem(hwndDlg, IDC_ZUPTIME_V);
            CpuPanelContextSwitchesLabel = GetDlgItem(hwndDlg, IDC_ZCONTEXTSWITCHESDELTA_V);
            CpuPanelInterruptDeltaLabel = GetDlgItem(hwndDlg, IDC_ZINTERRUPTSDELTA_V);
            CpuPanelDpcDeltaLabel = GetDlgItem(hwndDlg, IDC_ZDPCSDELTA_V);
            CpuPanelSystemCallsDeltaLabel = GetDlgItem(hwndDlg, IDC_ZSYSTEMCALLSDELTA_V);
            CpuPanelCoresLabel = GetDlgItem(hwndDlg, IDC_ZCORES);
            CpuPanelSocketsLabel = GetDlgItem(hwndDlg, IDC_ZSOCKETS);
            CpuPanelLogicalLabel = GetDlgItem(hwndDlg, IDC_ZLOGICAL);
            CpuPanelLatencyLabel = GetDlgItem(hwndDlg, IDC_ZLATENCY);

            SetWindowFont(CpuPanelUtilizationLabel, CpuSection->Parameters->MediumFont, FALSE);
            SetWindowFont(CpuPanelSpeedLabel, CpuSection->Parameters->MediumFont, FALSE);
            SetWindowFont(CpuVirtualizationLabel, CpuSection->Parameters->MediumFont, FALSE);

            PhQueueUserWorkItem(PhSipCpuSMBIOSWorkRoutine, NULL);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_ONEGRAPHPERCPU:
                {
                    OneGraphPerCpu = Button_GetCheck(GET_WM_COMMAND_HWND(wParam, lParam)) == BST_CHECKED;
                    PhSipLayoutCpuGraphs();
                    PhSipSetOneGraphPerCpu();
                }
                break;
            }
        }
        break;
    case WM_DPICHANGED_AFTERPARENT:
        {
            if (CpuSection->Parameters->MediumFont)
            {
                SetWindowFont(CpuPanelUtilizationLabel, CpuSection->Parameters->MediumFont, FALSE);
            }

            if (CpuSection->Parameters->MediumFont)
            {
                SetWindowFont(CpuPanelSpeedLabel, CpuSection->Parameters->MediumFont, FALSE);
            }

            PhSipLayoutCpuGraphs();
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

VOID PhSipCreateCpuGraphs(
    VOID
    )
{
    PH_GRAPH_CREATEPARAMS graphCreateParams;

    memset(&graphCreateParams, 0, sizeof(PH_GRAPH_CREATEPARAMS));
    graphCreateParams.Size = sizeof(PH_GRAPH_CREATEPARAMS);
    graphCreateParams.Callback = PhSipCpuGraphCallback;
    graphCreateParams.Context = UlongToPtr(ULONG_MAX);

    CpuGraphHandle = PhCreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_CHILD | WS_BORDER,
        0,
        0,
        0,
        0,
        CpuDialog,
        NULL,
        NULL,
        &graphCreateParams
        );

    if (PhEnableTooltipSupport)
    {
        Graph_SetTooltip(CpuGraphHandle, TRUE);
    }

    for (ULONG i = 0; i < NumberOfProcessors; i++)
    {
        graphCreateParams.Context = UlongToPtr(i);

        CpusGraphHandle[i] = PhCreateWindow(
            PH_GRAPH_CLASSNAME,
            NULL,
            WS_CHILD | WS_BORDER,
            0,
            0,
            0,
            0,
            CpuDialog,
            NULL,
            NULL,
            &graphCreateParams
            );

        if (PhEnableTooltipSupport)
        {
            Graph_SetTooltip(CpusGraphHandle[i], TRUE);
        }
    }
}

VOID PhSipLayoutCpuGraphs(
    VOID
    )
{
    RECT clientRect;
    RECT rect;
    HDWP deferHandle;

    rect = CpuGraphMargin;
    PhGetSizeDpiValue(&rect, CpuSection->Parameters->WindowDpi, TRUE);

    if (!PhGetClientRect(CpuDialog, &clientRect))
        return;

    deferHandle = BeginDeferWindowPos(OneGraphPerCpu ? NumberOfProcessors : 1);

    if (!OneGraphPerCpu)
    {
        deferHandle = DeferWindowPos(
            deferHandle,
            CpuGraphHandle,
            NULL,
            rect.left,
            rect.top,
            clientRect.right - rect.left - rect.right,
            clientRect.bottom - rect.top - rect.bottom,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
    }
    else
    {
        ULONG numberOfRows = 1;
        ULONG numberOfColumns = NumberOfProcessors;

        for (ULONG rows = 2; rows <= NumberOfProcessors / rows; rows++)
        {
            if (NumberOfProcessors % rows != 0)
                continue;

            numberOfRows = rows;
            numberOfColumns = NumberOfProcessors / rows;
        }

        if (numberOfRows == 1)
        {
            numberOfRows = (ULONG)sqrt(NumberOfProcessors);
            numberOfColumns = (NumberOfProcessors + numberOfRows - 1) / numberOfRows;
        }

        ULONG numberOfYPaddings = numberOfRows - 1;
        ULONG numberOfXPaddings = numberOfColumns - 1;

        ULONG cellHeight = (clientRect.bottom - rect.top - rect.bottom - CpuSection->Parameters->CpuPadding * numberOfYPaddings) / numberOfRows;
        ULONG y = rect.top;
        ULONG cellWidth;
        ULONG x;
        ULONG i = 0;

        for (ULONG row = 0; row < numberOfRows; row++)
        {
            // Give the last row the remaining space; the height we calculated might be off by a few
            // pixels due to integer division.
            if (row == numberOfRows - 1)
                cellHeight = clientRect.bottom - rect.bottom - y;

            cellWidth = (clientRect.right - rect.left - rect.right - CpuSection->Parameters->CpuPadding * numberOfXPaddings) / numberOfColumns;
            x = rect.left;

            for (ULONG column = 0; column < numberOfColumns; column++)
            {
                // Give the last cell the remaining space; the width we calculated might be off by a few
                // pixels due to integer division.
                if (column == numberOfColumns - 1)
                    cellWidth = clientRect.right - rect.right - x;

                if (i < NumberOfProcessors)
                {
                    deferHandle = DeferWindowPos(
                        deferHandle,
                        CpusGraphHandle[i],
                        NULL,
                        x,
                        y,
                        cellWidth,
                        cellHeight,
                        SWP_NOACTIVATE | SWP_NOZORDER
                        );
                    i++;
                }

                x += cellWidth + CpuSection->Parameters->CpuPadding;
            }

            y += cellHeight + CpuSection->Parameters->CpuPadding;
        }
    }

    EndDeferWindowPos(deferHandle);
}

VOID PhSipSetOneGraphPerCpu(
    VOID
    )
{
    ShowWindow(CpuGraphHandle, !OneGraphPerCpu ? SW_SHOW : SW_HIDE);

    for (ULONG i = 0; i < NumberOfProcessors; i++)
    {
        ShowWindow(CpusGraphHandle[i], OneGraphPerCpu ? SW_SHOW : SW_HIDE);
    }
}

_Function_class_(PH_GRAPH_MESSAGE_CALLBACK)
BOOLEAN NTAPI PhSipCpuGraphCallback(
    _In_ HWND GraphHandle,
    _In_ ULONG GraphMessage,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Context
    )
{
    switch (GraphMessage)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Parameter1;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;
            ULONG index = PtrToUlong(Context);

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_USE_LINE_2 | (PhCsEnableGraphMaxText ? PH_GRAPH_LABEL_MAX_Y : 0);
            PhSiSetColorsGraphDrawInfo(drawInfo, PhCsColorCpuKernel, PhCsColorCpuUser, CpuSection->Parameters->WindowDpi);

            if (index == ULONG_MAX)
            {
                PhGraphStateGetDrawInfo(
                    &CpuGraphState,
                    getDrawInfo,
                    PhCpuKernelHistory.Count
                    );

                if (!CpuGraphState.Valid)
                {
                    PhCopyCircularBuffer_FLOAT(&PhCpuKernelHistory, CpuGraphState.Data1, drawInfo->LineDataCount);
                    PhCopyCircularBuffer_FLOAT(&PhCpuUserHistory, CpuGraphState.Data2, drawInfo->LineDataCount);

                    if (PhCsEnableGraphMaxScale)
                    {
                        FLOAT max = 0;

                        if (PhCsEnableAvxSupport && drawInfo->LineDataCount > 128)
                        {
                            max = PhAddPlusMaxMemorySingles(
                                CpuGraphState.Data1,
                                CpuGraphState.Data2,
                                drawInfo->LineDataCount
                                );
                        }
                        else
                        {
                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data = CpuGraphState.Data1[i] + CpuGraphState.Data2[i];

                                if (max < data)
                                    max = data;
                            }
                        }

                        if (max != 0)
                        {
                            PhDivideSinglesBySingle(CpuGraphState.Data1, max, drawInfo->LineDataCount);
                            PhDivideSinglesBySingle(CpuGraphState.Data2, max, drawInfo->LineDataCount);
                        }

                        drawInfo->LabelYFunction = PhSiDoubleLabelYFunction;
                        drawInfo->LabelYFunctionParameter = max;
                    }
                    else
                    {
                        drawInfo->LabelYFunction = PhSiDoubleLabelYFunction;
                        drawInfo->LabelYFunctionParameter = 1.0f;
                    }

                    CpuGraphState.Valid = TRUE;
                }
            }
            else
            {
                PhGraphStateGetDrawInfo(
                    &CpusGraphState[index],
                    getDrawInfo,
                    PhCpuKernelHistory.Count
                    );

                if (!CpusGraphState[index].Valid)
                {
                    PhCopyCircularBuffer_FLOAT(&PhCpusKernelHistory[index], CpusGraphState[index].Data1, drawInfo->LineDataCount);
                    PhCopyCircularBuffer_FLOAT(&PhCpusUserHistory[index], CpusGraphState[index].Data2, drawInfo->LineDataCount);

                    if (PhCsEnableGraphMaxScale)
                    {
                        FLOAT max = 0;

                        if (PhCsEnableAvxSupport && drawInfo->LineDataCount > 128)
                        {
                            max = PhAddPlusMaxMemorySingles(
                                CpusGraphState[index].Data1,
                                CpusGraphState[index].Data2,
                                drawInfo->LineDataCount
                                );
                        }
                        else
                        {
                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data = CpusGraphState[index].Data1[i] + CpusGraphState[index].Data2[i];

                                if (max < data)
                                    max = data;
                            }
                        }

                        if (max != 0)
                        {
                            PhDivideSinglesBySingle(CpusGraphState[index].Data1, max, drawInfo->LineDataCount);
                            PhDivideSinglesBySingle(CpusGraphState[index].Data2, max, drawInfo->LineDataCount);
                        }

                        drawInfo->LabelYFunction = PhSiDoubleLabelYFunction;
                        drawInfo->LabelYFunctionParameter = max;
                    }
                    else
                    {
                        drawInfo->LabelYFunction = PhSiDoubleLabelYFunction;
                        drawInfo->LabelYFunctionParameter = 1.0f;
                    }

                    CpusGraphState[index].Valid = TRUE;
                }

                if (PhCsGraphShowText)
                {
                    HDC hdc;
                    FLOAT cpuKernel;
                    FLOAT cpuUser;
                    PH_FORMAT format[11];

                    cpuKernel = PhGetItemCircularBuffer_FLOAT(&PhCpusKernelHistory[index], 0);
                    cpuUser = PhGetItemCircularBuffer_FLOAT(&PhCpusUserHistory[index], 0);

                    // %.2f%% (K: %.2f%%, U: %.2f%%)
                    PhInitFormatF(&format[0], (cpuKernel + cpuUser) * 100, PhMaxPrecisionUnit);
                    PhInitFormatS(&format[1], L"% (K: ");
                    PhInitFormatF(&format[2], cpuKernel * 100, PhMaxPrecisionUnit);
                    PhInitFormatS(&format[3], L"%, U: ");
                    PhInitFormatF(&format[4], cpuUser * 100, PhMaxPrecisionUnit);

                    if (CpuFrequencyInformation)
                    {
                        PhInitFormatS(&format[5], L"%)\r\n");
                        PhInitFormatF(&format[6], CpuFrequencyInformation[index] / 1000, PhMaxPrecisionUnit);
                        PhInitFormatS(&format[7], L" GHz");
                        PhInitFormatS(&format[8], L" (");
                        PhInitFormatFD(&format[9], 1000 / CpuFrequencyInformation[index], 3); // ns per cycle
                        PhInitFormatS(&format[10], L" \u00B5s)");
                        PhMoveReference(&CpusGraphState[index].Text, PhFormat(format, RTL_NUMBER_OF(format), 0));

                        hdc = Graph_GetBufferedContext(CpusGraphHandle[index]);
                        PhSetGraphText2(
                            hdc,
                            drawInfo,
                            &CpusGraphState[index].Text->sr,
                            &PhNormalGraphTextMargin,
                            &PhNormalGraphTextPadding,
                            PH_ALIGN_TOP | PH_ALIGN_LEFT
                            );
                    }
                    else
                    {
                        PhInitFormatS(&format[5], L"%) ");
                        PhMoveReference(&CpusGraphState[index].Text, PhFormat(format, 6, 0));

                        hdc = Graph_GetBufferedContext(CpusGraphHandle[index]);
                        PhSetGraphText(
                            hdc,
                            drawInfo,
                            &CpusGraphState[index].Text->sr,
                            &PhNormalGraphTextMargin,
                            &PhNormalGraphTextPadding,
                            PH_ALIGN_TOP | PH_ALIGN_LEFT
                            );
                    }
                }
                else
                {
                    drawInfo->Text.Buffer = NULL;
                }
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Parameter1;
            ULONG index = PtrToUlong(Context);

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (index == ULONG_MAX)
                {
                    if (CpuGraphState.TooltipIndex != getTooltipText->Index)
                    {
                        FLOAT cpuKernel;
                        FLOAT cpuUser;
                        PH_FORMAT format[9];

                        cpuKernel = PhGetItemCircularBuffer_FLOAT(&PhCpuKernelHistory, getTooltipText->Index);
                        cpuUser = PhGetItemCircularBuffer_FLOAT(&PhCpuUserHistory, getTooltipText->Index);

                        // %.2f%% (K: %.2f%%, U: %.2f%%)\n%s\n%s
                        PhInitFormatF(&format[0], (cpuKernel + cpuUser) * 100, PhMaxPrecisionUnit);
                        PhInitFormatS(&format[1], L"% (K: ");
                        PhInitFormatF(&format[2], cpuKernel * 100, PhMaxPrecisionUnit);
                        PhInitFormatS(&format[3], L"%, U: ");
                        PhInitFormatF(&format[4], cpuUser * 100, PhMaxPrecisionUnit);
                        PhInitFormatS(&format[5], L"%)");
                        PhInitFormatSR(&format[6], PH_AUTO_T(PH_STRING, PhSipGetMaxCpuString(getTooltipText->Index))->sr);
                        PhInitFormatC(&format[7], L'\n');
                        PhInitFormatSR(&format[8], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                        PhMoveReference(&CpuGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 160));
                    }

                    getTooltipText->Text = CpuGraphState.TooltipText->sr;
                }
                else
                {
                    if (CpusGraphState[index].TooltipIndex != getTooltipText->Index)
                    {
                        FLOAT cpuKernel;
                        FLOAT cpuUser;
                        PCPH_STRINGREF cpuType;
                        PH_FORMAT format[20];
                        ULONG count = 0;

                        cpuKernel = PhGetItemCircularBuffer_FLOAT(&PhCpusKernelHistory[index], getTooltipText->Index);
                        cpuUser = PhGetItemCircularBuffer_FLOAT(&PhCpusUserHistory[index], getTooltipText->Index);

                        // %.2f%% (K: %.2f%%, U: %.2f%%)%s\n%s
                        PhInitFormatF(&format[count++], (cpuKernel + cpuUser) * 100, PhMaxPrecisionUnit);
                        PhInitFormatS(&format[count++], L"% (K: ");
                        PhInitFormatF(&format[count++], cpuKernel * 100, PhMaxPrecisionUnit);
                        PhInitFormatS(&format[count++], L"%, U: ");
                        PhInitFormatF(&format[count++], cpuUser * 100, PhMaxPrecisionUnit);
                        PhInitFormatS(&format[count++], L"%)");
                        PhInitFormatSR(&format[count++], PH_AUTO_T(PH_STRING, PhSipGetMaxCpuString(getTooltipText->Index))->sr);
                        PhInitFormatS(&format[count++], L"\nCPU ");

                        if (PhSystemProcessorInformation.SingleProcessorGroup)
                        {
                            PhInitFormatU(&format[count++], index);
                            PhInitFormatS(&format[count++], L", Core ");
                            PhInitFormatU(&format[count++], PhSipGetProcessorRelationshipIndex(RelationProcessorCore, index));
                            PhInitFormatS(&format[count++], L", Socket ");
                            PhInitFormatU(&format[count++], PhSipGetProcessorRelationshipIndex(RelationProcessorPackage, index));
                        }
                        else
                        {
                            PH_PROCESSOR_NUMBER processorNumber;
                            USHORT processorNode;

                            if (NT_SUCCESS(PhGetProcessorNumberFromIndex(index, &processorNumber)))
                            {
                                PhInitFormatU(&format[count++], processorNumber.Number);
                                PhInitFormatS(&format[count++], L", Group ");
                                PhInitFormatU(&format[count++], processorNumber.Group);

                                if (PhGetNumaProcessorNode(&processorNumber, &processorNode))
                                {
                                    PhInitFormatS(&format[count++], L", Node ");
                                    PhInitFormatU(&format[count++], processorNode);
                                }
                                else
                                {
                                    PhInitFormatS(&format[count++], L", Node ");
                                    PhInitFormatU(&format[count++], 0);
                                }
                            }
                            else
                            {
                                PhInitFormatU(&format[count++], index);
                                PhInitFormatS(&format[count++], L", Group ");
                                PhInitFormatU(&format[count++], ULONG_MAX);
                                PhInitFormatS(&format[count++], L", Node ");
                                PhInitFormatU(&format[count++], ULONG_MAX);
                            }
                        }

                        if (cpuType = PhGetHybridProcessorType(index))
                        {
                            PhInitFormatS(&format[count++], L", ");
                            PhInitFormatSR(&format[count++], *cpuType);
                            PhInitFormatS(&format[count++], L"\n");

                            if (PhIsCoreParked(index))
                                PhInitFormatS(&format[count++], L"Parked\n");
                        }
                        else
                        {
                            PhInitFormatS(&format[count++], L"\n");

                            if (PhIsCoreParked(index))
                                PhInitFormatS(&format[count++], L"Parked\n");
                        }

                        PhInitFormatSR(&format[count++], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);
                        PhMoveReference(&CpusGraphState[index].TooltipText, PhFormat(format, count, 0));
                    }

                    getTooltipText->Text = CpusGraphState[index].TooltipText->sr;
                }
            }
        }
        break;
    case GCN_MOUSEEVENT:
        {
            PPH_GRAPH_MOUSEEVENT mouseEvent = (PPH_GRAPH_MOUSEEVENT)Parameter1;
            PPH_PROCESS_RECORD record;

            record = NULL;

            if (mouseEvent->Message == WM_LBUTTONDBLCLK && mouseEvent->Index < mouseEvent->TotalCount)
            {
                record = PhSipReferenceMaxCpuRecord(mouseEvent->Index);
            }

            if (record)
            {
                PhShowProcessRecordDialog(CpuDialog, record);
                PhDereferenceProcessRecord(record);
            }
        }
        break;
    }

    return TRUE;
}

VOID PhSipUpdateCpuGraphs(
    VOID
    )
{
    CpuGraphState.Valid = FALSE;
    CpuGraphState.TooltipIndex = ULONG_MAX;
    Graph_Update(CpuGraphHandle);

    for (ULONG i = 0; i < NumberOfProcessors; i++)
    {
        CpusGraphState[i].Valid = FALSE;
        CpusGraphState[i].TooltipIndex = ULONG_MAX;
        Graph_Update(CpusGraphHandle[i]);
    }
}

VOID PhSipUpdateCpuPanel(
    VOID
    )
{
    LARGE_INTEGER systemUptime;
    LARGE_INTEGER performanceCounterStart;
    LARGE_INTEGER performanceCounterEnd;
    LARGE_INTEGER performanceCounterTicks;
    ULONG64 timeStampCounterStart;
    ULONG64 timeStampCounterEnd;
#ifdef _ARM64_
    ULONG64 currentExceptionLevel;
#else
    LONG cpubrand[4];
#endif
    PH_FORMAT format[6];
    WCHAR formatBuffer[256];
    WCHAR uptimeString[PH_TIMESPAN_STR_LEN_1] = { L"Unknown" };

    // Hardware

    if (CpuTicked == 0)
    {
        switch (PhGetVirtualStatus())
        {
        case PhVirtualStatusVirtualMachine:
            PhSetWindowText(CpuVirtualizationLabel, L"Virtual machine");
            break;
        case PhVirtualStatusEnabledHyperV:
        case PhVirtualStatusEnabledFirmware:
            PhSetWindowText(CpuVirtualizationLabel, L"Enabled");
            break;
        case PhVirtualStatusDisabledWithHyperV:
            PhSetWindowText(CpuVirtualizationLabel, L"Disabled / Hyper-V");
            break;
        case PhVirtualStatusDisabled:
            PhSetWindowText(CpuVirtualizationLabel, L"Disabled");
            break;
        case PhVirtualStatusNotCapable:
        default:
            PhSetWindowText(CpuVirtualizationLabel, L"Not capable");
            break;
        }

        PhSetDialogItemText(CpuPanel, IDC_ZL1CACHE_V, CpuL1CacheSize ? PhaFormatSize(CpuL1CacheSize, ULONG_MAX)->Buffer : L"N/A");
        PhSetDialogItemText(CpuPanel, IDC_ZL2CACHE_V, CpuL2CacheSize ? PhaFormatSize(CpuL2CacheSize, ULONG_MAX)->Buffer : L"N/A");
        PhSetDialogItemText(CpuPanel, IDC_ZL3CACHE_V, CpuL3CacheSize ? PhaFormatSize(CpuL3CacheSize, ULONG_MAX)->Buffer : L"N/A");
    }

    // %.2f%%
    PhInitFormatF(&format[0], (PhCpuUserUsage + PhCpuKernelUsage) * 100, PhMaxPrecisionUnit);
    PhInitFormatC(&format[1], L'%');

    if (PhFormatToBuffer(format, 2, formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(CpuPanelUtilizationLabel, formatBuffer);
    else
    {
        PhSetWindowText(CpuPanelUtilizationLabel, PH_AUTO_T(PH_STRING, PhFormat(format, 2, 0))->Buffer);
    }

    if (PhGetIntegerSetting(SETTING_SYSINFO_SHOW_CPU_SPEED_MHZ))
    {
        PhInitFormatF(&format[0], CpuCurrentFrequency, 0);
        PhInitFormatS(&format[1], L" / ");
        PhInitFormatF(&format[2], (FLOAT)CpuMaxMhz, 0);
        PhInitFormatS(&format[3], L" MHz");
    }
    else
    {
        PhInitFormatF(&format[0], CpuCurrentFrequency / 1000, PhMaxPrecisionUnit);
        PhInitFormatS(&format[1], L" / ");
        PhInitFormatF(&format[2], (FLOAT)CpuMaxMhz / 1000, PhMaxPrecisionUnit);
        PhInitFormatS(&format[3], L" GHz");
    }

    // %.2f / %.2f GHz
    if (PhFormatToBuffer(format, 4, formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(CpuPanelSpeedLabel, formatBuffer);
    else
    {
        PhSetWindowText(CpuPanelSpeedLabel, PH_AUTO_T(PH_STRING, PhFormat(format, 4, 0))->Buffer);
    }

    PhInitFormatI64UGroupDigits(&format[0], PhTotalProcesses);

    if (PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(CpuPanelProcessesLabel, formatBuffer);
    else
        PhSetWindowText(CpuPanelProcessesLabel, PhaFormatUInt64(PhTotalProcesses, TRUE)->Buffer);

    PhInitFormatI64UGroupDigits(&format[0], PhTotalThreads);

    if (PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(CpuPanelThreadsLabel, formatBuffer);
    else
        PhSetWindowText(CpuPanelThreadsLabel, PhaFormatUInt64(PhTotalThreads, TRUE)->Buffer);

    PhInitFormatI64UGroupDigits(&format[0], PhTotalHandles);

    if (PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(CpuPanelHandlesLabel, formatBuffer);
    else
        PhSetWindowText(CpuPanelHandlesLabel, PhaFormatUInt64(PhTotalHandles, TRUE)->Buffer);

    if (NT_SUCCESS(PhGetSystemUptime(&systemUptime)))
    {
        PhPrintTimeSpan(uptimeString, systemUptime.QuadPart, PH_TIMESPAN_DHMS);
    }

    PhSetWindowText(CpuPanelUptimeLabel, uptimeString);

    if (CpuTicked > 1)
    {
        PhInitFormatI64UGroupDigits(&format[0], ContextSwitchesDelta.Delta);

        if (PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), NULL))
            PhSetWindowText(CpuPanelContextSwitchesLabel, formatBuffer);
        else
            PhSetWindowText(CpuPanelContextSwitchesLabel, PhaFormatUInt64(ContextSwitchesDelta.Delta, TRUE)->Buffer);
    }
    else
    {
        PhSetWindowText(CpuPanelContextSwitchesLabel, L"-");
    }

    if (CpuTicked > 1)
    {
        PhInitFormatI64UGroupDigits(&format[0], InterruptsDelta.Delta);

        if (PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), NULL))
            PhSetWindowText(CpuPanelInterruptDeltaLabel, formatBuffer);
        else
            PhSetWindowText(CpuPanelInterruptDeltaLabel, PhaFormatUInt64(InterruptsDelta.Delta, TRUE)->Buffer);
    }
    else
    {
        PhSetWindowText(CpuPanelInterruptDeltaLabel, L"-");
    }

    if (CpuTicked > 1)
    {
        PhInitFormatI64UGroupDigits(&format[0], DpcsDelta.Delta);

        if (PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), NULL))
            PhSetWindowText(CpuPanelDpcDeltaLabel, formatBuffer);
        else
            PhSetWindowText(CpuPanelDpcDeltaLabel, PhaFormatUInt64(DpcsDelta.Delta, TRUE)->Buffer);
    }
    else
    {
        PhSetWindowText(CpuPanelDpcDeltaLabel, L"-");
    }

    if (CpuTicked > 1)
    {
        PhInitFormatI64UGroupDigits(&format[0], SystemCallsDelta.Delta);

        if (PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), NULL))
            PhSetWindowText(CpuPanelSystemCallsDeltaLabel, formatBuffer);
        else
            PhSetWindowText(CpuPanelSystemCallsDeltaLabel, PhaFormatUInt64(SystemCallsDelta.Delta, TRUE)->Buffer);
    }
    else
    {
        PhSetWindowText(CpuPanelSystemCallsDeltaLabel, L"-");
    }

    {
        PhInitFormatI64UGroupDigits(&format[0], LogicalProcessorInformation.ProcessorCoreCount);

        if (PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), NULL))
            PhSetWindowText(CpuPanelCoresLabel, formatBuffer);
        else
            PhSetWindowText(CpuPanelCoresLabel, PhaFormatUInt64(LogicalProcessorInformation.ProcessorCoreCount, TRUE)->Buffer);

        PhInitFormatI64UGroupDigits(&format[0], LogicalProcessorInformation.ProcessorPackageCount);

        if (PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), NULL))
            PhSetWindowText(CpuPanelSocketsLabel, formatBuffer);
        else
            PhSetWindowText(CpuPanelSocketsLabel, PhaFormatUInt64(LogicalProcessorInformation.ProcessorPackageCount, TRUE)->Buffer);

        PhInitFormatI64UGroupDigits(&format[0], LogicalProcessorInformation.ProcessorLogicalCount);

        if (PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), NULL))
            PhSetWindowText(CpuPanelLogicalLabel, formatBuffer);
        else
            PhSetWindowText(CpuPanelLogicalLabel, PhaFormatUInt64(LogicalProcessorInformation.ProcessorLogicalCount, TRUE)->Buffer);
    }

    // Do not optimize (dmex)
    PhQueryPerformanceCounter(&performanceCounterStart);
    timeStampCounterStart = PhReadTimeStampCounter();
#ifdef _ARM64_
    // 0b11    0b000    0b0100    0b0010    0b010    CurrentEL     Current Exception Level
    currentExceptionLevel = _ReadStatusReg(ARM64_SYSREG(3, 0, 4, 2, 2));
#else
    CpuIdEx(cpubrand, 0, 0);
#endif
    MemoryBarrier();
    timeStampCounterEnd = PhReadTimeStampCounter();
    PhQueryPerformanceCounter(&performanceCounterEnd);
    performanceCounterTicks.QuadPart = performanceCounterEnd.QuadPart - performanceCounterStart.QuadPart;

    if (timeStampCounterStart == 0 && timeStampCounterEnd == 0 &&
#ifdef _ARM64_
        currentExceptionLevel == MAXULONG64
#else
        cpubrand[0] == 0 && cpubrand[3] == 0
#endif
        )
    {
        performanceCounterTicks.QuadPart = 0;
    }

    PhInitFormatI64UGroupDigits(&format[0], performanceCounterTicks.QuadPart);
    PhInitFormatS(&format[1], L" | ");
    PhInitFormatF(&format[2], CpuTimerResolution, 0);
    PhInitFormatS(&format[3], L" | ");
    PhInitFormatI64UGroupDigits(&format[4], PhTotalCpuQueueLength);

    if (PhFormatToBuffer(format, 5, formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(CpuPanelLatencyLabel, formatBuffer);
    else
        PhSetWindowText(CpuPanelLatencyLabel, PhaFormatUInt64(performanceCounterTicks.QuadPart, TRUE)->Buffer);
}

PPH_PROCESS_RECORD PhSipReferenceMaxCpuRecord(
    _In_ LONG Index
    )
{
    LARGE_INTEGER time;
    LONG maxProcessIdLong;
    HANDLE maxProcessId;

    // Find the process record for the max. CPU process for the particular time.

    maxProcessIdLong = PhGetItemCircularBuffer_ULONG(&PhMaxCpuHistory, Index);

    if (!maxProcessIdLong)
        return NULL;

    // This must be treated as a signed integer to handle Interrupts correctly.
    maxProcessId = LongToHandle(maxProcessIdLong);

    // Note that the time we get has its components beyond seconds cleared.
    // For example:
    // * At 2.5 seconds a process is started.
    // * At 2.75 seconds our process provider is fired, and the process is determined
    //   to have 75% CPU usage, which happens to be the maximum CPU usage.
    // * However the 2.75 seconds is recorded as 2 seconds due to
    //   RtlTimeToSecondsSince1980.
    // * If we call PhFindProcessRecord, it cannot find the process because it was
    //   started at 2.5 seconds, not 2 seconds or older.
    //
    // This means we must add one second minus one tick (100ns) to the time, giving us
    // 2.9999999 seconds. This will then make sure we find the process.
    PhGetStatisticsTime(NULL, Index, &time);
    time.QuadPart += PH_TICKS_PER_SEC - 1;

    return PhFindProcessRecord(maxProcessId, &time);
}

PPH_STRING PhSipGetMaxCpuString(
    _In_ LONG Index
    )
{
    PPH_PROCESS_RECORD maxProcessRecord;
#ifdef PH_RECORD_MAX_USAGE
    FLOAT maxCpuUsage;
#endif

    if (maxProcessRecord = PhSipReferenceMaxCpuRecord(Index))
    {
        PPH_STRING maxUsageString;

        // We found the process record, so now we construct the max. usage string.
#ifdef PH_RECORD_MAX_USAGE
        maxCpuUsage = PhGetItemCircularBuffer_FLOAT(&PhMaxCpuUsageHistory, Index);

        // Make sure we don't try to display the PID of DPCs or Interrupts.
        if (!PH_IS_FAKE_PROCESS_ID(maxProcessRecord->ProcessId))
        {
            PH_FORMAT format[7];

            // \n%s (%lu): %.2f%%
            PhInitFormatC(&format[0], L'\n');
            PhInitFormatSR(&format[1], maxProcessRecord->ProcessName->sr);
            PhInitFormatS(&format[2], L" (");
            PhInitFormatU(&format[3], HandleToUlong(maxProcessRecord->ProcessId));
            PhInitFormatS(&format[4], L"): ");
            PhInitFormatF(&format[5], maxCpuUsage * 100, PhMaxPrecisionUnit);
            PhInitFormatC(&format[6], L'%');

            maxUsageString = PhFormat(format, RTL_NUMBER_OF(format), 128);
        }
        else
        {
            PH_FORMAT format[5];

            // \n%s: %.2f%%
            PhInitFormatC(&format[0], L'\n');
            PhInitFormatSR(&format[1], maxProcessRecord->ProcessName->sr);
            PhInitFormatS(&format[2], L": ");
            PhInitFormatF(&format[3], maxCpuUsage * 100, PhMaxPrecisionUnit);
            PhInitFormatC(&format[4], L'%');

            maxUsageString = PhFormat(format, RTL_NUMBER_OF(format), 128);
        }
#else
        PH_FORMAT format[2];

        PhInitFormatC(&format[0], L'\n');
        PhInitFormatSR(&format[1], maxProcessRecord->ProcessName->sr);

        maxUsageString = PhFormat(format, RTL_NUMBER_OF(format), 128);
#endif
        PhDereferenceProcessRecord(maxProcessRecord);

        return maxUsageString;
    }

    return PhReferenceEmptyString();
}

PPH_STRING PhSipGetCpuBrandString(
    VOID
    )
{
    static CONST PH_STRINGREF whitespace = PH_STRINGREF_INIT(L" ");
    PPH_STRING brand = NULL;
    ULONG brandLength;
    CHAR brandString[49];

    if (NT_SUCCESS(NtQuerySystemInformation(
        SystemProcessorBrandString,
        brandString,
        sizeof(brandString),
        NULL
        )))
    {
        brandLength = sizeof(brandString) - sizeof(ANSI_NULL);
        brand = PhConvertUtf8ToUtf16Ex(brandString, brandLength);
    }
    else
    {
#ifndef _ARM64_
        ULONG cpubrand[4 * 3];

        __cpuid(&cpubrand[0], 0x80000002);
        __cpuid(&cpubrand[4], 0x80000003);
        __cpuid(&cpubrand[8], 0x80000004);

        brandLength = sizeof(brandString) - sizeof(ANSI_NULL);
        brand = PhZeroExtendToUtf16Ex((PCSTR)cpubrand, brandLength);
#else
        ULONG deviceCount = 0;
        PDEV_OBJECT deviceObjects = NULL;

        const DEVPROPCOMPKEY deviceProperties[] =
        {
            { DEVPKEY_NAME, DEVPROP_STORE_SYSTEM, NULL },
            { DEVPKEY_Device_FriendlyName, DEVPROP_STORE_SYSTEM, NULL },
        };

        const DEVPROP_FILTER_EXPRESSION deviceFilter[] =
        {
            { DEVPROP_OPERATOR_EQUALS,
            { { DEVPKEY_Device_ClassGuid, DEVPROP_STORE_SYSTEM, NULL },
                DEVPROP_TYPE_GUID, sizeof(GUID), (PVOID)&GUID_DEVCLASS_PROCESSOR
            } }
        };

        if (SUCCEEDED(PhDevGetObjects(
            DevObjectTypeDevice,
            DevQueryFlagNone,
            RTL_NUMBER_OF(deviceProperties),
            deviceProperties,
            RTL_NUMBER_OF(deviceFilter),
            deviceFilter,
            &deviceCount,
            &deviceObjects
            )))
        {
            if (deviceCount > 0)
            {
                DEV_OBJECT device = deviceObjects[0];
                PH_STRINGREF string;

                string.Buffer = device.pProperties[0].Buffer;
                string.Length = device.pProperties[0].BufferSize ?
                    device.pProperties[0].BufferSize - sizeof(UNICODE_NULL) : 0;

                if (string.Length > 0)
                {
                    brand = PhCreateString2(&string);
                }
                else if (device.pProperties[1].BufferSize > 0)
                {
                    string.Buffer = device.pProperties[1].Buffer;
                    string.Length = device.pProperties[1].BufferSize - sizeof(UNICODE_NULL);
                    brand = PhCreateString2(&string);
                }
            }

            PhDevFreeObjects(deviceCount, deviceObjects);
        }

        //static CONST PH_STRINGREF processorKeyName = PH_STRINGREF_INIT(L"Hardware\\Description\\System\\CentralProcessor\\0");
        //HANDLE keyHandle;
        //
        //if (NT_SUCCESS(PhOpenKey(&keyHandle, KEY_READ, PH_KEY_LOCAL_MACHINE, &processorKeyName, 0)))
        //{
        //    brand = PhQueryRegistryStringZ(keyHandle, L"ProcessorNameString");
        //    NtClose(keyHandle);
        //}

        if (PhIsNullOrEmptyString(brand))
            PhMoveReference(&brand, PhCreateString(L"N/A"));
#endif
    }

    PhTrimToNullTerminatorString(brand);

    // Trim empty space (#611) (dmex)
    PhMoveReference(&brand, PhCreateString3(&brand->sr, PH_TRIM_END_ONLY, &whitespace));

    return brand;
}

VOID PhSipUpdateProcessorPerformanceDistribution(
    VOID
    )
{
    ULONG i, j;
    PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION current = NULL;
    PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION previous = NULL;
    ULONGLONG totalCount = 0;
    ULONGLONG totalValue = 0;

    if (PreviousPerformanceDistribution)
    {
        PhFree(PreviousPerformanceDistribution);
        PreviousPerformanceDistribution = NULL;
    }

    PreviousPerformanceDistribution = CurrentPerformanceDistribution;
    CurrentPerformanceDistribution = NULL;

    PhGetSystemProcessorPerformanceDistribution(&CurrentPerformanceDistribution);

    if (!CurrentPerformanceDistribution || !PreviousPerformanceDistribution)
        goto CleanupExit;
    if (CurrentPerformanceDistribution->ProcessorCount != PreviousPerformanceDistribution->ProcessorCount)
        goto CleanupExit;
    if (CurrentPerformanceDistribution->ProcessorCount != NumberOfProcessors)
        goto CleanupExit;

    for (i = 0; i < NumberOfProcessors; i++)
    {
        ULONGLONG count = 0;
        ULONGLONG value = 0;

        current = PTR_ADD_OFFSET(CurrentPerformanceDistribution, CurrentPerformanceDistribution->Offsets[i]);
        previous = PTR_ADD_OFFSET(PreviousPerformanceDistribution, PreviousPerformanceDistribution->Offsets[i]);

        if (current->StateCount != previous->StateCount)
            goto CleanupExit;

        if (WindowsVersion >= WINDOWS_8_1)
        {
            for (j = 0; j < current->StateCount; j++)
            {
                PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT hitcountCurrent = PTR_ADD_OFFSET(current->States, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT) * j);
                PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT hitcountPrevious = PTR_ADD_OFFSET(previous->States, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT) * j);
                ULONGLONG delta = hitcountCurrent->Hits - hitcountPrevious->Hits;

                if (delta)
                {
                    count += delta;
                    value += delta * hitcountCurrent->PercentFrequency * CpuMaxMhz;
                }
            }
        }
        else
        {
            for (j = 0; j < current->StateCount; j++)
            {
                PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8 hitcountOldCurrent = PTR_ADD_OFFSET(current->States, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8) * j);
                PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8 hitcountOldPrevious = PTR_ADD_OFFSET(previous->States, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8) * j);
                ULONGLONG delta = hitcountOldCurrent->Hits - hitcountOldPrevious->Hits;

                if (delta)
                {
                    count += delta;
                    value += delta * hitcountOldCurrent->PercentFrequency * CpuMaxMhz;
                }
            }
        }

        if (CpuFrequencyInformation)
        {
            CpuFrequencyInformation[i] = count ? (FLOAT)((DOUBLE)value / (DOUBLE)count / 100.0) : 0.0f;
        }

        totalCount += count;
        totalValue += value;
    }

    if (PreviousPerformanceDistribution)
    {
        PhFree(PreviousPerformanceDistribution);
        PreviousPerformanceDistribution = NULL;
    }

    if (totalCount == 0)
        CpuCurrentFrequency = (FLOAT)PowerInformation[0].CurrentMhz;
    else
        CpuCurrentFrequency = (FLOAT)((DOUBLE)totalValue / (DOUBLE)totalCount / 100.0);

    return;

CleanupExit:
    if (CurrentPerformanceDistribution)
    {
        PhFree(CurrentPerformanceDistribution);
        CurrentPerformanceDistribution = NULL;
    }
    if (PreviousPerformanceDistribution)
    {
        PhFree(PreviousPerformanceDistribution);
        PreviousPerformanceDistribution = NULL;
    }

    if (CpuFrequencyInformation)
    {
        memset(CpuFrequencyInformation, 0, sizeof(FLOAT) * NumberOfProcessors);
    }

    CpuCurrentFrequency = (FLOAT)PowerInformation[0].CurrentMhz;
}

_Success_(return)
BOOLEAN PhSipGetCpuFrequencyFromDistributionLegacy(
    _Out_ DOUBLE *Frequency
    )
{
    ULONG stateSize;
    PVOID differences;
    PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION stateDistribution;
    PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION stateDifference;
    PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8 hitcountOld;
    ULONG i;
    ULONG j;
    DOUBLE count;
    DOUBLE total;

    // Calculate the differences from the last performance distribution.

    if (CurrentPerformanceDistribution->ProcessorCount != NumberOfProcessors || PreviousPerformanceDistribution->ProcessorCount != NumberOfProcessors)
        return FALSE;

    stateSize = FIELD_OFFSET(SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION, States) + sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT) * 2;
    differences = PhAllocate(UInt32x32To64(stateSize, NumberOfProcessors));

    for (i = 0; i < NumberOfProcessors; i++)
    {
        stateDistribution = PTR_ADD_OFFSET(CurrentPerformanceDistribution, CurrentPerformanceDistribution->Offsets[i]);
        stateDifference = PTR_ADD_OFFSET(differences, UInt32x32To64(stateSize, i));

        if (stateDistribution->StateCount != 2)
        {
            PhFree(differences);
        return FALSE;
    }

        for (j = 0; j < stateDistribution->StateCount; j++)
        {
            if (WindowsVersion >= WINDOWS_8_1)
            {
                stateDifference->States[j] = stateDistribution->States[j];
            }
            else
            {
                hitcountOld = PTR_ADD_OFFSET(stateDistribution->States, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8) * j);
                stateDifference->States[j].Hits = hitcountOld->Hits;
                stateDifference->States[j].PercentFrequency = hitcountOld->PercentFrequency;
            }
        }
    }

    for (i = 0; i < NumberOfProcessors; i++)
    {
        stateDistribution = PTR_ADD_OFFSET(PreviousPerformanceDistribution, PreviousPerformanceDistribution->Offsets[i]);
        stateDifference = PTR_ADD_OFFSET(differences, UInt32x32To64(stateSize, i));

        if (stateDistribution->StateCount != 2)
        {
            PhFree(differences);
            return FALSE;
        }

        for (j = 0; j < stateDistribution->StateCount; j++)
        {
            if (WindowsVersion >= WINDOWS_8_1)
            {
                stateDifference->States[j].Hits -= stateDistribution->States[j].Hits;
            }
            else
            {
                hitcountOld = PTR_ADD_OFFSET(stateDistribution->States, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8) * j);
                stateDifference->States[j].Hits -= hitcountOld->Hits;
            }
        }
    }

    // Calculate the frequency.

    count = 0;
    total = 0;

    for (i = 0; i < NumberOfProcessors; i++)
    {
        stateDifference = PTR_ADD_OFFSET(differences, UInt32x32To64(stateSize, i));

        for (j = 0; j < 2; j++)
        {
            count += stateDifference->States[j].Hits;
            total += stateDifference->States[j].Hits * stateDifference->States[j].PercentFrequency * PowerInformation[i].MaxMhz;
        }
    }

    PhFree(differences);

    if (count == 0)
        return FALSE;

    total /= count;
    total /= 100;
    *Frequency = total;

    return TRUE;
}

_Success_(return)
BOOLEAN PhSipGetCpuFrequencyFromDistribution(
    _Out_ DOUBLE *Frequency
    )
{
    ULONG i, j;
    PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION current = NULL;
    PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION previous = NULL;
    ULONGLONG count = 0;
    ULONGLONG total = 0;
    PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION tempPrev = NULL;

    // Free any previous distribution and clear pointer
    if (PreviousPerformanceDistribution)
    {
        PhFree(PreviousPerformanceDistribution);
        PreviousPerformanceDistribution = NULL;
    }

    // Move current to previous, then get new current
    PreviousPerformanceDistribution = CurrentPerformanceDistribution;
    CurrentPerformanceDistribution = NULL;

    PhGetSystemProcessorPerformanceDistribution(&CurrentPerformanceDistribution);

    if (!CurrentPerformanceDistribution || !PreviousPerformanceDistribution)
        goto CleanupExit;
    if (CurrentPerformanceDistribution->ProcessorCount != PreviousPerformanceDistribution->ProcessorCount)
        goto CleanupExit;
    if (CurrentPerformanceDistribution->ProcessorCount != NumberOfProcessors)
        goto CleanupExit;

    for (i = 0; i < NumberOfProcessors; i++)
    {
        current = PTR_ADD_OFFSET(CurrentPerformanceDistribution, CurrentPerformanceDistribution->Offsets[i]);
        previous = PTR_ADD_OFFSET(PreviousPerformanceDistribution, PreviousPerformanceDistribution->Offsets[i]);

        if (current->StateCount != previous->StateCount)
            goto CleanupExit;

        if (WindowsVersion >= WINDOWS_8_1)
        {
            PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT hitcountCurrent;
            PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT hitcountPrevious;
            ULONGLONG delta;
            UCHAR percent;

            for (j = 0; j < current->StateCount; j++)
            {
                hitcountCurrent = PTR_ADD_OFFSET(current->States, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT) * j);
                hitcountPrevious = PTR_ADD_OFFSET(previous->States, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT) * j);
                delta = hitcountCurrent->Hits - hitcountPrevious->Hits;
                percent = hitcountCurrent->PercentFrequency;

                if (delta)
                {
                    count += delta;
                    total += delta * percent * CpuMaxMhz;
                }
            }
        }
        else
        {
            PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8 hitcountOldCurrent;
            PSYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8 hitcountOldPrevious;
            ULONGLONG delta;
            UCHAR percent;

            for (j = 0; j < current->StateCount; j++)
            {
                hitcountOldCurrent = PTR_ADD_OFFSET(current->States, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8) * j);
                hitcountOldPrevious = PTR_ADD_OFFSET(previous->States, sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT_WIN8) * j);

                delta = hitcountOldCurrent->Hits - hitcountOldPrevious->Hits;
                percent = hitcountOldCurrent->PercentFrequency;

                if (delta)
                {
                    count += delta;
                    total += delta * percent * CpuMaxMhz;
                }
            }
        }
    }

    // Cleanup previous, but keep current for next call
    if (PreviousPerformanceDistribution)
    {
        PhFree(PreviousPerformanceDistribution);
        PreviousPerformanceDistribution = NULL;
    }

    if (count == 0)
        return FALSE;

    // Total contains sum(delta * percent * maxMhz).
    // Convert to average frequency (MHz).
    *Frequency = (DOUBLE)(total / (DOUBLE)count / 100.0);

    return TRUE;

CleanupExit:
    if (CurrentPerformanceDistribution)
    {
        PhFree(CurrentPerformanceDistribution);
        CurrentPerformanceDistribution = NULL;
    }
    if (PreviousPerformanceDistribution)
    {
        PhFree(PreviousPerformanceDistribution);
        PreviousPerformanceDistribution = NULL;
    }
    return FALSE;
}

ULONG PhSipGetProcessorRelationshipIndex(
    _In_ LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType,
    _In_ ULONG Index
    )
{
    ULONG index;
    ULONG bufferLength;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX logicalInformation;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX processorInfo;

    if (!NT_SUCCESS(PhGetSystemLogicalProcessorInformation(RelationshipType, &logicalInformation, &bufferLength)))
        return ULONG_MAX;

    index = 0;

    for (
        processorInfo = logicalInformation;
        (ULONG_PTR)processorInfo < (ULONG_PTR)PTR_ADD_OFFSET(logicalInformation, bufferLength);
        processorInfo = PTR_ADD_OFFSET(processorInfo, processorInfo->Size)
        )
    {
        PROCESSOR_RELATIONSHIP processor = processorInfo->Processor;
        //BOOLEAN hyperThreaded = processor.Flags & LTP_PC_SMT;

        if (processor.GroupMask[0].Mask & AFFINITY_MASK(Index))
        {
            PhFree(logicalInformation);
            return index;
        }

        //for (USHORT j = 0; j < processor.GroupCount; j++)
        //{
        //    if (processor.GroupMask[j].Mask & ((KAFFINITY)1 << (Index % PhGetActiveProcessorCount(j))))
        //    {
        //        PhFree(logicalInformation);
        //        return index;
        //    }
        //}

        index++;
    }

    PhFree(logicalInformation);
    return ULONG_MAX;
}

#ifndef _ARM64_
// Hybrid CPU detection (dmex)

#define CPUID_EXTENDED_FEATURES_FUNCTION 0x07
#define CPUID_HYBRID_INFORMATION_FUNCTION 0x1A
#define CPUID_HYBRID_CORETYPE_ECORE 0x20
#define CPUID_HYBRID_CORETYPE_PCORE 0x40

typedef union _CPUID_HYBRID_INFORMATION
{
    struct
    {
        INT32 Eax;
        INT32 Ebx;
        INT32 Ecx;
        INT32 Edx;
    };
    INT32 AsINT32[4];
    struct
    {
        INT32 ModelId : 24;
        INT32 CoreType : 8;
    };
} CPUID_HYBRID_INFORMATION;

_Success_(return)
BOOLEAN PhInitializeHybridProcessorTypeCache(
    _Out_ PPH_LIST *HybridProcessorTypeList
    )
{
    PPH_LIST hybridProcessorTypeList = NULL;
    GROUP_AFFINITY previousThreadGroupAffinity;
    INT32 CPUIDInformation[4] = { 0 };
    INT32 CPUIDFunctionMax;
    //INT32 CPUIDExtendedFunctionMax;
    INT32 CPUIDExtendedFeatureFlags;

    memset(&CPUIDInformation, 0, sizeof(CPUIDInformation));
    CpuIdEx(CPUIDInformation, 0, 0);
    CPUIDFunctionMax = CPUIDInformation[0];
    if (!(CPUIDFunctionMax >= CPUID_HYBRID_INFORMATION_FUNCTION))
        return FALSE;

    //memset(&CPUIDInformation, 0, sizeof(CPUIDInformation));
    //CpuIdEx(CPUIDInformation, 0x80000000, 0);
    //CPUIDExtendedFunctionMax = CPUIDInformation[0] & ~0x80000000;
    //if (CPUIDExtendedFunctionMax < CPUID_EXTENDED_FEATURES_FUNCTION)
    //    return FALSE;

    memset(&CPUIDInformation, 0, sizeof(CPUIDInformation));
    CpuIdEx(CPUIDInformation, CPUID_EXTENDED_FEATURES_FUNCTION, 0);
    CPUIDExtendedFeatureFlags = CPUIDInformation[3];
    if (!_bittest(&CPUIDExtendedFeatureFlags, 15)) // hybrid
        return FALSE;

    if (!NT_SUCCESS(PhGetThreadGroupAffinity(NtCurrentThread(), &previousThreadGroupAffinity)))
        return FALSE;

    hybridProcessorTypeList = PhCreateList(PhSystemProcessorInformation.NumberOfProcessors);

    for (USHORT processorGroup = 0; processorGroup < PhSystemProcessorInformation.NumberOfProcessorGroups; processorGroup++)
    {
        USHORT processorCount = PhGetActiveProcessorCount(processorGroup);

        for (USHORT i = 0; i < processorCount; i++)
        {
            GROUP_AFFINITY threadGroup = { 0 };
            CPUID_HYBRID_INFORMATION hybridInfo = { 0 };

            threadGroup.Mask = AFFINITY_MASK(i);
            threadGroup.Group = processorGroup;

            if (NT_SUCCESS(PhSetThreadGroupAffinity(NtCurrentThread(), threadGroup)))
            {
                CpuIdEx(hybridInfo.AsINT32, CPUID_HYBRID_INFORMATION_FUNCTION, 0);
            }

            switch (hybridInfo.CoreType)
            {
            case CPUID_HYBRID_CORETYPE_ECORE:
                PhAddItemList(hybridProcessorTypeList, UlongToPtr(CPUID_HYBRID_CORETYPE_ECORE));
                break;
            case CPUID_HYBRID_CORETYPE_PCORE:
                PhAddItemList(hybridProcessorTypeList, UlongToPtr(CPUID_HYBRID_CORETYPE_PCORE));
                break;
            default:
                PhAddItemList(hybridProcessorTypeList, UlongToPtr(0));
                break;
            }
        }
    }

    PhSetThreadGroupAffinity(NtCurrentThread(), previousThreadGroupAffinity);

    *HybridProcessorTypeList = hybridProcessorTypeList;
    return TRUE;
}

PCPH_STRINGREF PhGetHybridProcessorType(
    _In_ ULONG ProcessorIndex
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PPH_LIST hybridProcessorTypeList = NULL;

    // Hybrid processors are not supported on earlier versions (dmex)
    if (WindowsVersion < WINDOWS_10)
        return NULL;
    // Hybrid processors are not included with multiple groups (for now) (dmex)
    if (PhSystemProcessorInformation.NumberOfProcessors >= MAXIMUM_PROC_PER_GROUP)
        return NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PhInitializeHybridProcessorTypeCache(&hybridProcessorTypeList);
        PhEndInitOnce(&initOnce);
    }

    if (!hybridProcessorTypeList || ProcessorIndex > hybridProcessorTypeList->Count)
        return NULL;

    switch (PtrToUlong(hybridProcessorTypeList->Items[ProcessorIndex]))
    {
    case CPUID_HYBRID_CORETYPE_ECORE:
        {
            static CONST PH_STRINGREF hybridECoreTypeSr = PH_STRINGREF_INIT(L"E-Core");
            return &hybridECoreTypeSr;
        }
    case CPUID_HYBRID_CORETYPE_PCORE:
        {
            static CONST PH_STRINGREF hybridPCoreTypeSr = PH_STRINGREF_INIT(L"P-Core");
            return &hybridPCoreTypeSr;
        }
    }

    return NULL;
}
#else

#define ARM_CORETYPE_LITTLE 0
#define ARM_CORETYPE_BIG 1

_Success_(return)
BOOLEAN PhInitializeHybridProcessorTypeCache(
    _Out_ PPH_LIST *HybridProcessorTypeList
    )
{
    ULONG bufferLength;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX logicalInformation;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX processorInfo;
    PPH_LIST hybridProcessorTypeList = NULL;

    if (!NT_SUCCESS(PhGetSystemLogicalProcessorInformation(RelationProcessorCore, &logicalInformation, &bufferLength)))
        return FALSE;

    hybridProcessorTypeList = PhCreateList(PhSystemProcessorInformation.NumberOfProcessors);

    for (
        processorInfo = logicalInformation;
        (ULONG_PTR)processorInfo < (ULONG_PTR)PTR_ADD_OFFSET(logicalInformation, bufferLength);
        processorInfo = PTR_ADD_OFFSET(processorInfo, processorInfo->Size)
        )
    {
        for (USHORT j = 0; j < processorInfo->Processor.GroupCount; j++)
        {
            //One logical processor per bit set in the core mask
            ULONG logicalProcessorsInGroup = PhCountBitsUlongPtr(processorInfo->Processor.GroupMask[j].Mask);
            while (logicalProcessorsInGroup--)
            {
                PhAddItemList(hybridProcessorTypeList, UlongToPtr(processorInfo->Processor.EfficiencyClass));
            }
        }
    }
    PhFree(logicalInformation);

    *HybridProcessorTypeList = hybridProcessorTypeList;
    return TRUE;
}

PCPH_STRINGREF PhGetHybridProcessorType(
    _In_ ULONG ProcessorIndex
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static PPH_LIST hybridProcessorTypeList = NULL;

    // Hybrid processors are not supported on earlier versions (dmex)
    if (WindowsVersion < WINDOWS_10)
        return NULL;
    // Hybrid processors are not included with multiple groups (for now) (dmex)
    if (PhSystemProcessorInformation.NumberOfProcessors >= MAXIMUM_PROC_PER_GROUP)
        return NULL;

    if (PhBeginInitOnce(&initOnce))
    {
        PhInitializeHybridProcessorTypeCache(&hybridProcessorTypeList);
        PhEndInitOnce(&initOnce);
    }

    if (!hybridProcessorTypeList || ProcessorIndex > hybridProcessorTypeList->Count)
        return NULL;

    switch (PtrToUlong(hybridProcessorTypeList->Items[ProcessorIndex]))
    {
    case ARM_CORETYPE_LITTLE:
        {
            static CONST PH_STRINGREF hybridECoreTypeSr = PH_STRINGREF_INIT(L"E-Core");
            return &hybridECoreTypeSr;
        }
    case ARM_CORETYPE_BIG:
        {
            static CONST PH_STRINGREF hybridPCoreTypeSr = PH_STRINGREF_INIT(L"P-Core");
            return &hybridPCoreTypeSr;
        }
    }

    return NULL;
}
#endif

/**
 * \brief Checks if a specific logical processor (core) in the system is parked.
 *
 * This function determines whether a specific logical processor in the system
 * is parked, meaning it is temporarily unavailable for the thread scheduler.
 * It utilizes CPU sets functionality available starting with Windows 10.
 *
 * \param[in] ProcessorIndex The index of the logical processor to check.
 *
 * \return TRUE if the specified logical processor is parked, FALSE otherwise or
 * in case of failure.
 */
BOOLEAN PhIsCoreParked(
    _In_ ULONG ProcessorIndex
    )
{
    static ULONG initialBufferSize = 0;
    static HANDLE processHandle = NULL;
    NTSTATUS status;
    ULONG returnLength;
    BOOLEAN isParked;
    PSYSTEM_CPU_SET_INFORMATION cpuSetInfo;

    if (WindowsVersion < WINDOWS_10)
        return FALSE;

    //
    // MSDN: https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-system_cpu_set_information
    // This is a variable-sized structure designed for future expansion. When iterating over
    // this structure, use the size field to determine the offset to the next structure.
    //
    // N.B. The kernel returns the size even in the last element, we over-allocate by the
    // Size offset and check it to minimize instructions (jxy-s).
    //

    if (initialBufferSize)
    {
        returnLength = initialBufferSize;
        cpuSetInfo = PhAllocateStack(returnLength);
        if (!cpuSetInfo) return FALSE;
        RtlZeroMemory(cpuSetInfo, returnLength);
    }
    else
    {
        returnLength = 0;
        cpuSetInfo = NULL;
    }

    status = NtQuerySystemInformationEx(
        SystemCpuSetInformation,
        &processHandle,
        sizeof(HANDLE),
        cpuSetInfo,
        returnLength,
        &returnLength
        );

    if (status == STATUS_BUFFER_TOO_SMALL)
    {
        returnLength += RTL_SIZEOF_THROUGH_FIELD(SYSTEM_CPU_SET_INFORMATION, Size);

        PhFreeStack(cpuSetInfo);
        cpuSetInfo = PhAllocateStack(returnLength);
        if (!cpuSetInfo) return FALSE;
        RtlZeroMemory(cpuSetInfo, returnLength);

        status = NtQuerySystemInformationEx(
            SystemCpuSetInformation,
            &processHandle,
            sizeof(HANDLE),
            cpuSetInfo,
            returnLength,
            NULL
            );

        if (NT_SUCCESS(status) && initialBufferSize <= 0x100000)
            initialBufferSize = returnLength;
    }

    if (!cpuSetInfo)
        return FALSE;

    isParked = FALSE;

    if (NT_SUCCESS(status))
    {
        for (PSYSTEM_CPU_SET_INFORMATION info = cpuSetInfo;
             RTL_CONTAINS_FIELD(info, info->Size, CpuSet);
             info = PTR_ADD_OFFSET(info, info->Size))
        {
            if (info->CpuSet.LogicalProcessorIndex == ProcessorIndex)
            {
                isParked = info->CpuSet.Parked;
                break;
            }
        }
    }

    PhFreeStack(cpuSetInfo);

    return isParked;
}

VOID PhSipUpdateProcessorInformation(
    VOID
    )
{
    if (PhSystemProcessorInformation.SingleProcessorGroup)
    {
        if (!NT_SUCCESS(NtPowerInformation(
            ProcessorInformation,
            NULL,
            0,
            PowerInformation,
            sizeof(PROCESSOR_POWER_INFORMATION) * NumberOfProcessors
            )))
        {
            memset(PowerInformation, 0, sizeof(PROCESSOR_POWER_INFORMATION) * NumberOfProcessors);
        }
    }
    else
    {
        USHORT processorCount = 0;

        for (USHORT processorGroup = 0; processorGroup < PhSystemProcessorInformation.NumberOfProcessorGroups; processorGroup++)
        {
            USHORT activeProcessorCount = PhGetActiveProcessorCount(processorGroup);

            if (!NT_SUCCESS(NtPowerInformation(
                ProcessorInformationEx,
                &processorGroup,
                sizeof(USHORT),
                PTR_ADD_OFFSET(PowerInformation, sizeof(PROCESSOR_POWER_INFORMATION) * processorCount),
                sizeof(PROCESSOR_POWER_INFORMATION) * activeProcessorCount
                )))
            {
                memset(
                    PTR_ADD_OFFSET(PowerInformation, sizeof(PROCESSOR_POWER_INFORMATION) * processorCount),
                    0,
                    sizeof(PROCESSOR_POWER_INFORMATION) * activeProcessorCount
                    );
            }

            processorCount += activeProcessorCount;
        }
    }
}

VOID PhSipUpdateInterruptInformation(
    _Out_ PULONG64 DpcCount
    )
{
    ULONG64 dpcCount;
    ULONG i;

    dpcCount = 0;

    if (PhSystemProcessorInformation.SingleProcessorGroup)
    {
        if (!NT_SUCCESS(NtQuerySystemInformation(
            SystemInterruptInformation,
            InterruptInformation,
            sizeof(SYSTEM_INTERRUPT_INFORMATION) * NumberOfProcessors,
            NULL
            )))
        {
            memset(InterruptInformation, 0, sizeof(SYSTEM_INTERRUPT_INFORMATION) * NumberOfProcessors);
        }

        for (i = 0; i < NumberOfProcessors; i++)
            dpcCount += InterruptInformation[i].DpcCount;
    }
    else
    {
        USHORT processorCount = 0;

        for (USHORT processorGroup = 0; processorGroup < PhSystemProcessorInformation.NumberOfProcessorGroups; processorGroup++)
        {
            USHORT activeProcessorCount = PhGetActiveProcessorCount(processorGroup);

            if (!NT_SUCCESS(NtQuerySystemInformationEx(
                SystemInterruptInformation,
                &processorGroup,
                sizeof(USHORT),
                PTR_ADD_OFFSET(InterruptInformation, sizeof(SYSTEM_INTERRUPT_INFORMATION) * processorCount),
                sizeof(SYSTEM_INTERRUPT_INFORMATION) * activeProcessorCount,
                NULL
                )))
            {
                memset(
                    PTR_ADD_OFFSET(InterruptInformation, sizeof(SYSTEM_INTERRUPT_INFORMATION) * processorCount),
                    0,
                    sizeof(SYSTEM_INTERRUPT_INFORMATION) * activeProcessorCount
                    );
            }

            processorCount += activeProcessorCount;
        }

        for (i = 0; i < NumberOfProcessors; i++)
            dpcCount += InterruptInformation[i].DpcCount;
    }

    *DpcCount = dpcCount;
}

VOID PhSipUpdateProcessorFrequency(
    VOID
    )
{
    POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT input;
    POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT output;

    memset(&input, 0, sizeof(POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_INPUT));
    input.InternalType = PowerInternalProcessorBrandedFrequency;
    input.ProcessorNumber.Group = USHRT_MAX;
    input.ProcessorNumber.Number = UCHAR_MAX;
    input.ProcessorNumber.Reserved = UCHAR_MAX;

    memset(&output, 0, sizeof(POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_OUTPUT));
    output.Version = POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_VERSION;

    if (NT_SUCCESS(NtPowerInformation(
        PowerInformationInternal,
        &input,
        sizeof(input),
        &output,
        sizeof(output)
        )))
    {
        if (output.Version == POWER_INTERNAL_PROCESSOR_BRANDED_FREQUENCY_VERSION)
        {
            CpuMaxMhz = output.NominalFrequency;
            return;
        }
    }

    for (ULONG i = 0; i < NumberOfProcessors; i++)
    {
        if (CpuMaxMhz < PowerInformation[i].MaxMhz)
            CpuMaxMhz = PowerInformation[i].MaxMhz;
    }
}

VOID PhSipUpdateTimerResolution(
    VOID
    )
{
    ULONG maximumTime;
    ULONG minimumTime;
    ULONG currentTime;

    if (NT_SUCCESS(NtQueryTimerResolution(&maximumTime, &minimumTime, &currentTime)))
    {
        CpuTimerResolution = (FLOAT)((DOUBLE)currentTime / 10000.0);
    }
    else
    {
        CpuTimerResolution = 0.0f;
    }
}

_Function_class_(PH_ENUM_SMBIOS_CALLBACK)
BOOLEAN NTAPI PhpSipCpuSMBIOSCallback(
    _In_ ULONG_PTR EnumHandle,
    _In_ UCHAR MajorVersion,
    _In_ UCHAR MinorVersion,
    _In_ PPH_SMBIOS_ENTRY Entry,
    _In_opt_ PVOID Context
    )
{
    ULONG64 size;

    if (Entry->Header.Type != SMBIOS_CACHE_INFORMATION_TYPE)
        return FALSE;

    if (!PH_SMBIOS_CONTAINS_FIELD(Entry, Cache, Configuration) ||
        !Entry->Cache.Configuration.Enabled)
    {
        return FALSE;
    }

    size = 0;

    if (PH_SMBIOS_CONTAINS_FIELD(Entry, Cache, InstalledSize))
    {
        if (Entry->Cache.InstalledSize.Value == MAXUSHORT &&
            PH_SMBIOS_CONTAINS_FIELD(Entry, Cache, InstalledSize2))
        {
            if (Entry->Cache.InstalledSize2.Granularity)
                size = (ULONG64)Entry->Cache.InstalledSize2.Size * 0x10000;
            else
                size = (ULONG64)Entry->Cache.InstalledSize2.Size * 0x400;
        }
        else
        {
            if (Entry->Cache.InstalledSize.Granularity)
                size = (ULONG64)Entry->Cache.InstalledSize.Size * 0x10000;
            else
                size = (ULONG64)Entry->Cache.InstalledSize.Size * 0x400;
        }
    }

    switch (Entry->Cache.Configuration.Level)
    {
    case 0:
        CpuL1CacheSize += size;
        break;
    case 1:
        CpuL2CacheSize += size;
        break;
    case 2:
        CpuL3CacheSize += size;
        break;
    }

    return FALSE;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS NTAPI PhSipCpuSMBIOSWorkRoutine(
    _In_ PVOID ThreadParameter
    )
{
    PhEnumSMBIOS(PhpSipCpuSMBIOSCallback, NULL);
    return STATUS_SUCCESS;
}
