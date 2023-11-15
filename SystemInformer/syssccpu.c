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

#include <math.h>

#include <procprv.h>
#include <phsettings.h>

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
            PhInitFormatF(&format[0], ((DOUBLE)cpuKernel + cpuUser) * 100, PhMaxPrecisionUnit);
            PhInitFormatS(&format[1], L"% (K: ");
            PhInitFormatF(&format[2], (DOUBLE)cpuKernel * 100, PhMaxPrecisionUnit);
            PhInitFormatS(&format[3], L"%, U: ");
            PhInitFormatF(&format[4], (DOUBLE)cpuUser * 100, PhMaxPrecisionUnit);
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
            PhInitFormatF(&format[0], ((DOUBLE)PhCpuKernelUsage + PhCpuUserUsage) * 100, PhMaxPrecisionUnit);
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
    ULONG PowerInformationLength;

    PhInitializeDelta(&ContextSwitchesDelta);
    PhInitializeDelta(&InterruptsDelta);
    PhInitializeDelta(&DpcsDelta);
    PhInitializeDelta(&SystemCallsDelta);

    NumberOfProcessors = PhSystemProcessorInformation.NumberOfProcessors;
    CpusGraphHandle = PhAllocate(sizeof(HWND) * NumberOfProcessors);
    CpusGraphState = PhAllocate(sizeof(PH_GRAPH_STATE) * NumberOfProcessors);
    InterruptInformation = PhAllocate(sizeof(SYSTEM_INTERRUPT_INFORMATION) * NumberOfProcessors);
    PowerInformationLength = sizeof(PROCESSOR_POWER_INFORMATION) * NumberOfProcessors;
    PowerInformation = PhAllocate(PowerInformationLength);

    PhInitializeGraphState(&CpuGraphState);

    for (ULONG i = 0; i < NumberOfProcessors; i++)
        PhInitializeGraphState(&CpusGraphState[i]);

    CpuTicked = 0;

    if (PhSystemProcessorInformation.SingleProcessorGroup)
    {
        if (!NT_SUCCESS(NtPowerInformation(
            ProcessorInformation,
            NULL,
            0,
            PowerInformation,
            PowerInformationLength
            )))
        {
            memset(PowerInformation, 0, PowerInformationLength);
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

    CpuMaxMhz = 0;

    for (ULONG i = 0; i < NumberOfProcessors; i++)
    {
        if (CpuMaxMhz < PowerInformation[i].MaxMhz)
            CpuMaxMhz = PowerInformation[i].MaxMhz;
    }

    CurrentPerformanceDistribution = NULL;
    PreviousPerformanceDistribution = NULL;

    PhSipQueryProcessorPerformanceDistribution(&CurrentPerformanceDistribution);
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
        PhFree(CurrentPerformanceDistribution);
    if (PreviousPerformanceDistribution)
        PhFree(PreviousPerformanceDistribution);

    PhSetIntegerSetting(L"SysInfoWindowOneGraphPerCpu", OneGraphPerCpu);
}

VOID PhSipTickCpuDialog(
    VOID
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

    PhUpdateDelta(&ContextSwitchesDelta, PhPerfInformation.ContextSwitches);
    PhUpdateDelta(&InterruptsDelta, PhCpuTotals.InterruptCount);
    PhUpdateDelta(&DpcsDelta, dpcCount);
    PhUpdateDelta(&SystemCallsDelta, PhPerfInformation.SystemCalls);

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

    if (PreviousPerformanceDistribution)
        PhFree(PreviousPerformanceDistribution);

    PreviousPerformanceDistribution = CurrentPerformanceDistribution;
    CurrentPerformanceDistribution = NULL;
    PhSipQueryProcessorPerformanceDistribution(&CurrentPerformanceDistribution);
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
            PhAddLayoutItemEx(&CpuLayoutManager, CpuPanel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, panelItem->Margin);

            PhSipCreateCpuGraphs();

            if (NumberOfProcessors != 1)
            {
                OneGraphPerCpu = (BOOLEAN)PhGetIntegerSetting(L"SysInfoWindowOneGraphPerCpu");
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
    case WM_NOTIFY:
        {
            NMHDR *header = (NMHDR *)lParam;
            ULONG i;

            if (header->hwndFrom == CpuGraphHandle)
            {
                PhSipNotifyCpuGraph(ULONG_MAX, header);
            }
            else
            {
                for (i = 0; i < NumberOfProcessors; i++)
                {
                    if (header->hwndFrom == CpusGraphHandle[i])
                    {
                        PhSipNotifyCpuGraph(i, header);
                        break;
                    }
                }
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
            CpuPanelUtilizationLabel = GetDlgItem(hwndDlg, IDC_UTILIZATION);
            CpuPanelSpeedLabel = GetDlgItem(hwndDlg, IDC_SPEED);
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
    CpuGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        CpuDialog,
        NULL,
        PhInstanceHandle,
        NULL
        );
    Graph_SetTooltip(CpuGraphHandle, TRUE);

    for (ULONG i = 0; i < NumberOfProcessors; i++)
    {
        CpusGraphHandle[i] = CreateWindow(
            PH_GRAPH_CLASSNAME,
            NULL,
            WS_CHILD | WS_BORDER,
            0,
            0,
            3,
            3,
            CpuDialog,
            NULL,
            PhInstanceHandle,
            NULL
            );
        Graph_SetTooltip(CpusGraphHandle[i], TRUE);
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

    GetClientRect(CpuDialog, &clientRect);
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

VOID PhSipNotifyCpuGraph(
    _In_ ULONG Index,
    _In_ NMHDR *Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_USE_LINE_2 | (PhCsEnableGraphMaxText ? PH_GRAPH_LABEL_MAX_Y : 0);
            PhSiSetColorsGraphDrawInfo(drawInfo, PhCsColorCpuKernel, PhCsColorCpuUser, CpuSection->Parameters->WindowDpi);

            if (Index == ULONG_MAX)
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
                    &CpusGraphState[Index],
                    getDrawInfo,
                    PhCpuKernelHistory.Count
                    );

                if (!CpusGraphState[Index].Valid)
                {
                    PhCopyCircularBuffer_FLOAT(&PhCpusKernelHistory[Index], CpusGraphState[Index].Data1, drawInfo->LineDataCount);
                    PhCopyCircularBuffer_FLOAT(&PhCpusUserHistory[Index], CpusGraphState[Index].Data2, drawInfo->LineDataCount);

                    if (PhCsEnableGraphMaxScale)
                    {
                        FLOAT max = 0;

                        if (PhCsEnableAvxSupport && drawInfo->LineDataCount > 128)
                        {
                            max = PhAddPlusMaxMemorySingles(
                                CpusGraphState[Index].Data1,
                                CpusGraphState[Index].Data2,
                                drawInfo->LineDataCount
                                );
                        }
                        else
                        {
                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data = CpusGraphState[Index].Data1[i] + CpusGraphState[Index].Data2[i];

                                if (max < data)
                                    max = data;
                            }
                        }

                        if (max != 0)
                        {
                            PhDivideSinglesBySingle(CpusGraphState[Index].Data1, max, drawInfo->LineDataCount);
                            PhDivideSinglesBySingle(CpusGraphState[Index].Data2, max, drawInfo->LineDataCount);
                        }

                        drawInfo->LabelYFunction = PhSiDoubleLabelYFunction;
                        drawInfo->LabelYFunctionParameter = max;
                    }
                    else
                    {
                        drawInfo->LabelYFunction = PhSiDoubleLabelYFunction;
                        drawInfo->LabelYFunctionParameter = 1.0f;
                    }

                    CpusGraphState[Index].Valid = TRUE;
                }

                if (PhCsGraphShowText)
                {
                    HDC hdc;
                    FLOAT cpuKernel;
                    FLOAT cpuUser;
                    PH_FORMAT format[6];

                    cpuKernel = PhGetItemCircularBuffer_FLOAT(&PhCpusKernelHistory[Index], 0);
                    cpuUser = PhGetItemCircularBuffer_FLOAT(&PhCpusUserHistory[Index], 0);

                    // %.2f%% (K: %.2f%%, U: %.2f%%)
                    PhInitFormatF(&format[0], ((DOUBLE)cpuKernel + cpuUser) * 100, PhMaxPrecisionUnit);
                    PhInitFormatS(&format[1], L"% (K: ");
                    PhInitFormatF(&format[2], (DOUBLE)cpuKernel * 100, PhMaxPrecisionUnit);
                    PhInitFormatS(&format[3], L"%, U: ");
                    PhInitFormatF(&format[4], (DOUBLE)cpuUser * 100, PhMaxPrecisionUnit);
                    PhInitFormatS(&format[5], L"%)");

                    PhMoveReference(&CpusGraphState[Index].Text, PhFormat(format, RTL_NUMBER_OF(format), 64));

                    hdc = Graph_GetBufferedContext(CpusGraphHandle[Index]);
                    PhSetGraphText(
                        hdc,
                        drawInfo,
                        &CpusGraphState[Index].Text->sr,
                        &PhNormalGraphTextMargin,
                        &PhNormalGraphTextPadding,
                        PH_ALIGN_TOP | PH_ALIGN_LEFT
                        );
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
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (Index == ULONG_MAX)
                {
                    if (CpuGraphState.TooltipIndex != getTooltipText->Index)
                    {
                        FLOAT cpuKernel;
                        FLOAT cpuUser;
                        PH_FORMAT format[9];

                        cpuKernel = PhGetItemCircularBuffer_FLOAT(&PhCpuKernelHistory, getTooltipText->Index);
                        cpuUser = PhGetItemCircularBuffer_FLOAT(&PhCpuUserHistory, getTooltipText->Index);

                        // %.2f%% (K: %.2f%%, U: %.2f%%)\n%s\n%s
                        PhInitFormatF(&format[0], ((DOUBLE)cpuKernel + cpuUser) * 100, PhMaxPrecisionUnit);
                        PhInitFormatS(&format[1], L"% (K: ");
                        PhInitFormatF(&format[2], (DOUBLE)cpuKernel * 100, PhMaxPrecisionUnit);
                        PhInitFormatS(&format[3], L"%, U: ");
                        PhInitFormatF(&format[4], (DOUBLE)cpuUser * 100, PhMaxPrecisionUnit);
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
                    if (CpusGraphState[Index].TooltipIndex != getTooltipText->Index)
                    {
                        FLOAT cpuKernel;
                        FLOAT cpuUser;
                        PPH_STRINGREF cpuType;
                        PH_FORMAT format[17];

                        cpuKernel = PhGetItemCircularBuffer_FLOAT(&PhCpusKernelHistory[Index], getTooltipText->Index);
                        cpuUser = PhGetItemCircularBuffer_FLOAT(&PhCpusUserHistory[Index], getTooltipText->Index);

                        // %.2f%% (K: %.2f%%, U: %.2f%%)%s\n%s
                        PhInitFormatF(&format[0], ((DOUBLE)cpuKernel + cpuUser) * 100, PhMaxPrecisionUnit);
                        PhInitFormatS(&format[1], L"% (K: ");
                        PhInitFormatF(&format[2], (DOUBLE)cpuKernel * 100, PhMaxPrecisionUnit);
                        PhInitFormatS(&format[3], L"%, U: ");
                        PhInitFormatF(&format[4], (DOUBLE)cpuUser * 100, PhMaxPrecisionUnit);
                        PhInitFormatS(&format[5], L"%)");
                        PhInitFormatSR(&format[6], PH_AUTO_T(PH_STRING, PhSipGetMaxCpuString(getTooltipText->Index))->sr);
                        PhInitFormatS(&format[7], L"\nCPU ");

                        if (PhSystemProcessorInformation.SingleProcessorGroup)
                        {
                            PhInitFormatU(&format[8], Index);
                            PhInitFormatS(&format[9], L", Core ");
                            PhInitFormatU(&format[10], PhSipGetProcessorRelationshipIndex(RelationProcessorCore, Index));
                            PhInitFormatS(&format[11], L", Socket ");
                            PhInitFormatU(&format[12], PhSipGetProcessorRelationshipIndex(RelationProcessorPackage, Index));
                        }
                        else
                        {
                            PH_PROCESSOR_NUMBER processorNumber;
                            USHORT processorNode;

                            if (NT_SUCCESS(PhGetProcessorNumberFromIndex(Index, &processorNumber)))
                            {
                                PhInitFormatU(&format[8], processorNumber.Number);
                                PhInitFormatS(&format[9], L", Group ");
                                PhInitFormatU(&format[10], processorNumber.Group);

                                if (PhGetNumaProcessorNode(&processorNumber, &processorNode))
                                {
                                    PhInitFormatS(&format[11], L", Node ");
                                    PhInitFormatU(&format[12], processorNode);
                                }
                                else
                                {
                                    PhInitFormatS(&format[11], L", Node ");
                                    PhInitFormatU(&format[12], 0);
                                }
                            }
                            else
                            {
                                PhInitFormatU(&format[8], Index);
                                PhInitFormatS(&format[9], L", Group ");
                                PhInitFormatU(&format[10], ULONG_MAX);
                                PhInitFormatS(&format[11], L", Node ");
                                PhInitFormatU(&format[12], ULONG_MAX);
                            }
                        }

                        if (cpuType = PhGetHybridProcessorType(Index))
                        {
                            PhInitFormatS(&format[13], L", ");
                            format[14].Type = StringFormatType;
                            format[14].u.String.Length = cpuType->Length;
                            format[14].u.String.Buffer = cpuType->Buffer;
                            PhInitFormatS(&format[15], L"\n");
                            PhInitFormatSR(&format[16], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                            PhMoveReference(&CpusGraphState[Index].TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                        }
                        else
                        {
                            PhInitFormatS(&format[13], L"\n");
                            PhInitFormatSR(&format[14], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                            PhMoveReference(&CpusGraphState[Index].TooltipText, PhFormat(format, 15, 0));
                        }
                    }

                    getTooltipText->Text = CpusGraphState[Index].TooltipText->sr;
                }
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
}

VOID PhSipUpdateCpuGraphs(
    VOID
    )
{
    CpuGraphState.Valid = FALSE;
    CpuGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(CpuGraphHandle, 1);
    Graph_Draw(CpuGraphHandle);
    Graph_UpdateTooltip(CpuGraphHandle);
    InvalidateRect(CpuGraphHandle, NULL, FALSE);

    for (ULONG i = 0; i < NumberOfProcessors; i++)
    {
        CpusGraphState[i].Valid = FALSE;
        CpusGraphState[i].TooltipIndex = ULONG_MAX;
        Graph_MoveGrid(CpusGraphHandle[i], 1);
        Graph_Draw(CpusGraphHandle[i]);
        Graph_UpdateTooltip(CpusGraphHandle[i]);
        InvalidateRect(CpusGraphHandle[i], NULL, FALSE);
    }
}

VOID PhSipUpdateCpuPanel(
    VOID
    )
{
    DOUBLE cpuFrequency;
    DOUBLE cpuGhz = 0;
    BOOLEAN distributionSucceeded = FALSE;
    LARGE_INTEGER systemUptime;
    LARGE_INTEGER performanceCounterStart;
    LARGE_INTEGER performanceCounterEnd;
    LARGE_INTEGER performanceCounterTicks;
    ULONG64 timeStampCounterStart;
    ULONG64 timeStampCounterEnd;
#ifdef _ARM64_
    ULONG64 currentExceptionLevel;
#else
    INT cpubrand[4];
#endif
    PH_FORMAT format[5];
    WCHAR formatBuffer[256];
    WCHAR uptimeString[PH_TIMESPAN_STR_LEN_1] = { L"Unknown" };

    if (CurrentPerformanceDistribution && PreviousPerformanceDistribution)
    {
        if (PhSipGetCpuFrequencyFromDistribution(&cpuFrequency))
        {
            cpuGhz = cpuFrequency / 1000;
            distributionSucceeded = TRUE;
        }
    }

    if (!distributionSucceeded)
        cpuGhz = (DOUBLE)PowerInformation[0].CurrentMhz / 1000;

    // %.2f%%
    PhInitFormatF(&format[0], ((DOUBLE)PhCpuUserUsage + PhCpuKernelUsage) * 100, PhMaxPrecisionUnit);
    PhInitFormatC(&format[1], L'%');

    if (PhFormatToBuffer(format, 2, formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(CpuPanelUtilizationLabel, formatBuffer);
    else
    {
        PhSetWindowText(CpuPanelUtilizationLabel, PH_AUTO_T(PH_STRING, PhFormat(format, 2, 0))->Buffer);
    }

    PhInitFormatF(&format[0], cpuGhz, PhMaxPrecisionUnit);
    PhInitFormatS(&format[1], L" / ");
    PhInitFormatF(&format[2], (DOUBLE)CpuMaxMhz / 1000, PhMaxPrecisionUnit);
    PhInitFormatS(&format[3], L" GHz");

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
    MemoryBarrier();
    timeStampCounterStart = PhReadTimeStampCounter();
    MemoryBarrier();
#ifdef _ARM64_
    // 0b11    0b000    0b0100    0b0010    0b010    CurrentEL     Current Exception Level
    currentExceptionLevel = _ReadStatusReg(ARM64_SYSREG(3, 0, 4, 2, 2));
#else
    CpuIdEx(cpubrand, 0, 0);
#endif
    MemoryBarrier();
    timeStampCounterEnd = PhReadTimeStampCounter();
    MemoryBarrier();
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
    PhInitFormatI64UGroupDigits(&format[2], PhTotalCpuQueueLength);

    if (PhFormatToBuffer(format, 3, formatBuffer, sizeof(formatBuffer), NULL))
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
            PhInitFormatF(&format[5], (DOUBLE)maxCpuUsage * 100, PhMaxPrecisionUnit);
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
            PhInitFormatF(&format[3], (DOUBLE)maxCpuUsage * 100, PhMaxPrecisionUnit);
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
    static PH_STRINGREF whitespace = PH_STRINGREF_INIT(L" ");
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
        brand = PhConvertUtf8ToUtf16Ex((PSTR)cpubrand, brandLength);
#else
        static PH_STRINGREF processorKeyName = PH_STRINGREF_INIT(L"Hardware\\Description\\System\\CentralProcessor\\0");
        HANDLE keyHandle;

        if (NT_SUCCESS(PhOpenKey(&keyHandle, KEY_READ, PH_KEY_LOCAL_MACHINE, &processorKeyName, 0)))
        {
            brand = PhQueryRegistryStringZ(keyHandle, L"ProcessorNameString");
            NtClose(keyHandle);
        }

        if (PhIsNullOrEmptyString(brand))
            PhMoveReference(&brand, PhCreateString(L"N/A"));
#endif
    }

    PhTrimToNullTerminatorString(brand);

    // Trim empty space (#611) (dmex)
    PhMoveReference(&brand, PhCreateString3(&brand->sr, PH_TRIM_END_ONLY, &whitespace));

    return brand;
}

_Success_(return)
BOOLEAN PhSipGetCpuFrequencyFromDistribution(
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

NTSTATUS PhSipQueryProcessorPerformanceDistribution(
    _Out_ PVOID *Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG bufferSize;
    ULONG attempts;

    bufferSize = 0x100;
    buffer = PhAllocate(bufferSize);

    status = NtQuerySystemInformation(
        SystemProcessorPerformanceDistribution,
        buffer,
        bufferSize,
        &bufferSize
        );
    attempts = 0;

    while (status == STATUS_INFO_LENGTH_MISMATCH && attempts < 8)
    {
        PhFree(buffer);
        buffer = PhAllocate(bufferSize);

        status = NtQuerySystemInformation(
            SystemProcessorPerformanceDistribution,
            buffer,
            bufferSize,
            &bufferSize
            );
        attempts++;
    }

    if (NT_SUCCESS(status))
        *Buffer = buffer;
    else
        PhFree(buffer);

    return status;
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

PPH_STRINGREF PhGetHybridProcessorType(
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
            static PH_STRINGREF hybridECoreTypeSr = PH_STRINGREF_INIT(L"E-Core");
            return &hybridECoreTypeSr;
        }
    case CPUID_HYBRID_CORETYPE_PCORE:
        {
            static PH_STRINGREF hybridPCoreTypeSr = PH_STRINGREF_INIT(L"P-Core");
            return &hybridPCoreTypeSr;
        }
    }

    return NULL;
}
#else
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
        //One logical core per bit set in the core mask
        ULONG_PTR mask = processorInfo->Processor.GroupMask[0].Mask;
        while (mask)
        {
            mask &= (mask - 1); // Flip the least significant bit set to '1'
            PhAddItemList(hybridProcessorTypeList, UlongToPtr(processorInfo->Processor.EfficiencyClass));
        }
    }
    PhFree(logicalInformation);

    *HybridProcessorTypeList = hybridProcessorTypeList;
    return TRUE;
}

PPH_STRINGREF PhGetHybridProcessorType(
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
    case 0:
        {
            static PH_STRINGREF hybridECoreTypeSr = PH_STRINGREF_INIT(L"E-Core");
            return &hybridECoreTypeSr;
        }
    case 1:
        {
            static PH_STRINGREF hybridPCoreTypeSr = PH_STRINGREF_INIT(L"P-Core");
            return &hybridPCoreTypeSr;
        }
    }

    return NULL;
}
#endif
