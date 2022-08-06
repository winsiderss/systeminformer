/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2017-2022
 *
 */

#include <phapp.h>
#include <sysinfo.h>
#include <sysinfop.h>

#include <kphuser.h>
#include <symprv.h>
#include <workqueue.h>
#include <settings.h>

#include <procprv.h>
#include <phsettings.h>

static PPH_SYSINFO_SECTION MemorySection;
static HWND MemoryDialog;
static PH_LAYOUT_MANAGER MemoryLayoutManager;
static RECT MemoryGraphMargin;
static HWND CommitGraphHandle;
static PH_GRAPH_STATE CommitGraphState;
static HWND PhysicalGraphHandle;
static PH_GRAPH_STATE PhysicalGraphState;
static HWND MemoryPanel;
static ULONG MemoryTicked;
static PH_UINT32_DELTA PagedAllocsDelta;
static PH_UINT32_DELTA PagedFreesDelta;
static PH_UINT32_DELTA NonPagedAllocsDelta;
static PH_UINT32_DELTA NonPagedFreesDelta;
static PH_UINT32_DELTA PageFaultsDelta;
static PH_UINT32_DELTA PageReadsDelta;
static PH_UINT32_DELTA PagefileWritesDelta;
static PH_UINT32_DELTA MappedWritesDelta;
static PH_UINT64_DELTA MappedIoReadDelta;
static PH_UINT64_DELTA MappedIoWritesDelta;
static BOOLEAN MmAddressesInitialized;
static PSIZE_T MmSizeOfPagedPoolInBytes;
static PSIZE_T MmMaximumNonPagedPoolInBytes;
static ULONGLONG InstalledMemory;
static ULONGLONG ReservedMemory;

BOOLEAN PhSipMemorySectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    switch (Message)
    {
    case SysInfoCreate:
        {
            MemorySection = Section;
        }
        return TRUE;
    case SysInfoDestroy:
        {
            if (MemoryDialog)
            {
                PhSipUninitializeMemoryDialog();
                MemoryDialog = NULL;
            }
        }
        break;
    case SysInfoTick:
        {
            if (MemoryDialog)
            {
                PhSipTickMemoryDialog();
            }
        }
        return TRUE;
    case SysInfoViewChanging:
        {
            PH_SYSINFO_VIEW_TYPE view = (PH_SYSINFO_VIEW_TYPE)Parameter1;
            PPH_SYSINFO_SECTION section = (PPH_SYSINFO_SECTION)Parameter2;

            if (view == SysInfoSummaryView || section != Section)
                return TRUE;

            if (CommitGraphHandle)
            {
                CommitGraphState.Valid = FALSE;
                CommitGraphState.TooltipIndex = ULONG_MAX;
                Graph_Draw(CommitGraphHandle);
            }

            if (PhysicalGraphHandle)
            {
                PhysicalGraphState.Valid = FALSE;
                PhysicalGraphState.TooltipIndex = ULONG_MAX;
                Graph_Draw(PhysicalGraphHandle);
            }
        }
        return TRUE;
    case SysInfoCreateDialog:
        {
            PPH_SYSINFO_CREATE_DIALOG createDialog = Parameter1;

            if (!createDialog)
                break;

            createDialog->Instance = PhInstanceHandle;
            createDialog->Template = MAKEINTRESOURCE(IDD_SYSINFO_MEM);
            createDialog->DialogProc = PhSipMemoryDialogProc;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = Parameter1;
            ULONG i;

            if (!drawInfo)
                break;

            if (PhGetIntegerSetting(L"ShowCommitInSummary"))
            {
                drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (PhCsEnableGraphMaxText ? PH_GRAPH_LABEL_MAX_Y : 0);
                Section->Parameters->ColorSetupFunction(drawInfo, PhCsColorPrivate, 0);
                PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, PhCommitHistory.Count);

                if (!Section->GraphState.Valid)
                {
                    for (i = 0; i < drawInfo->LineDataCount; i++)
                    {
                        Section->GraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&PhCommitHistory, i);
                    }

                    if (PhPerfInformation.CommitLimit != 0)
                    {
                        // Scale the data.
                        PhDivideSinglesBySingle(
                            Section->GraphState.Data1,
                            (FLOAT)PhPerfInformation.CommitLimit,
                            drawInfo->LineDataCount
                            );
                    }

                    if (PhCsEnableGraphMaxText)
                    {
                        drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                        drawInfo->LabelYFunctionParameter = (FLOAT)PhPerfInformation.CommitLimit * PAGE_SIZE;
                    }

                    Section->GraphState.Valid = TRUE;
                }
            }
            else
            {
                drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (PhCsEnableGraphMaxText ? PH_GRAPH_LABEL_MAX_Y : 0);
                Section->Parameters->ColorSetupFunction(drawInfo, PhCsColorPhysical, 0);
                PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, PhPhysicalHistory.Count);

                if (!Section->GraphState.Valid)
                {
                    for (i = 0; i < drawInfo->LineDataCount; i++)
                    {
                        Section->GraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&PhPhysicalHistory, i);
                    }

                    if (PhSystemBasicInformation.NumberOfPhysicalPages != 0)
                    {
                        // Scale the data.
                        PhDivideSinglesBySingle(
                            Section->GraphState.Data1,
                            (FLOAT)PhSystemBasicInformation.NumberOfPhysicalPages,
                            drawInfo->LineDataCount
                            );
                    }

                    if (PhCsEnableGraphMaxText)
                    {
                        drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                        drawInfo->LabelYFunctionParameter = (FLOAT)PhSystemBasicInformation.NumberOfPhysicalPages * PAGE_SIZE;
                    }

                    Section->GraphState.Valid = TRUE;
                }
            }
        }
        return TRUE;
    case SysInfoGraphGetTooltipText:
        {
            PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT getTooltipText = Parameter1;
            ULONG usedPages;
            PH_FORMAT format[3];

            if (!getTooltipText)
                break;

            if (PhGetIntegerSetting(L"ShowCommitInSummary"))
            {
                usedPages = PhGetItemCircularBuffer_ULONG(&PhCommitHistory, getTooltipText->Index);

                // Commit charge: %s\n%s
                PhInitFormatSize(&format[0], UInt32x32To64(usedPages, PAGE_SIZE));
                PhInitFormatC(&format[1], L'\n');
                PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                PhMoveReference(&Section->GraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 128));
                getTooltipText->Text = Section->GraphState.TooltipText->sr;
            }
            else
            {
                usedPages = PhGetItemCircularBuffer_ULONG(&PhPhysicalHistory, getTooltipText->Index);

                // Physical memory: %s\n%s
                PhInitFormatSize(&format[0], UInt32x32To64(usedPages, PAGE_SIZE));
                PhInitFormatC(&format[1], L'\n');
                PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                PhMoveReference(&Section->GraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 128));
                getTooltipText->Text = Section->GraphState.TooltipText->sr;
            }
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = Parameter1;
            ULONG totalPages;
            ULONG usedPages;
            PH_FORMAT format[5];

            if (!drawPanel)
                break;

            if (PhGetIntegerSetting(L"ShowCommitInSummary"))
            {
                totalPages = PhPerfInformation.CommitLimit;
                usedPages = PhPerfInformation.CommittedPages;
            }
            else
            {
                totalPages = PhSystemBasicInformation.NumberOfPhysicalPages;
                usedPages = totalPages - PhPerfInformation.AvailablePages;
            }

            drawPanel->Title = PhCreateString(L"Memory");

            // %.0f%%\n%s / %s
            PhInitFormatF(&format[0], (DOUBLE)usedPages * 100 / totalPages, 0);
            PhInitFormatS(&format[1], L"%\n");
            PhInitFormatSizeWithPrecision(&format[2], UInt32x32To64(usedPages, PAGE_SIZE), 1);
            PhInitFormatS(&format[3], L" / ");
            PhInitFormatSizeWithPrecision(&format[4], UInt32x32To64(totalPages, PAGE_SIZE), 1);

            drawPanel->SubTitle = PhFormat(format, 5, 64);

            // %.0f%%\n%s
            PhInitFormatF(&format[0], (DOUBLE)usedPages * 100 / totalPages, 0);
            PhInitFormatS(&format[1], L"%\n");
            PhInitFormatSizeWithPrecision(&format[2], UInt32x32To64(usedPages, PAGE_SIZE), 1);

            drawPanel->SubTitleOverflow = PhFormat(format, 3, 0);
        }
        return TRUE;
    }

    return FALSE;
}

VOID PhSipInitializeMemoryDialog(
    VOID
    )
{
    PhInitializeDelta(&PagedAllocsDelta);
    PhInitializeDelta(&PagedFreesDelta);
    PhInitializeDelta(&NonPagedAllocsDelta);
    PhInitializeDelta(&NonPagedFreesDelta);
    PhInitializeDelta(&PageFaultsDelta);
    PhInitializeDelta(&PageReadsDelta);
    PhInitializeDelta(&PagefileWritesDelta);
    PhInitializeDelta(&MappedWritesDelta);
    PhInitializeDelta(&MappedIoReadDelta);
    PhInitializeDelta(&MappedIoWritesDelta);

    PhInitializeGraphState(&CommitGraphState);
    PhInitializeGraphState(&PhysicalGraphState);

    MemoryTicked = 0;

    if (!MmAddressesInitialized && KphIsConnected())
    {
        PhQueueItemWorkQueue(PhGetGlobalWorkQueue(), PhSipLoadMmAddresses, NULL);
        MmAddressesInitialized = TRUE;
    }
}

VOID PhSipUninitializeMemoryDialog(
    VOID
    )
{
    PhDeleteGraphState(&CommitGraphState);
    PhDeleteGraphState(&PhysicalGraphState);

    // Note: Required for SysInfoViewChanging (dmex)
    CommitGraphHandle = NULL;
    PhysicalGraphHandle = NULL;
}

VOID PhSipTickMemoryDialog(
    VOID
    )
{
    PhUpdateDelta(&PagedAllocsDelta, PhPerfInformation.PagedPoolAllocs);
    PhUpdateDelta(&PagedFreesDelta, PhPerfInformation.PagedPoolFrees);
    PhUpdateDelta(&NonPagedAllocsDelta, PhPerfInformation.NonPagedPoolAllocs);
    PhUpdateDelta(&NonPagedFreesDelta, PhPerfInformation.NonPagedPoolFrees);
    PhUpdateDelta(&PageFaultsDelta, PhPerfInformation.PageFaultCount);
    PhUpdateDelta(&PageReadsDelta, PhPerfInformation.PageReadCount);
    PhUpdateDelta(&PagefileWritesDelta, PhPerfInformation.DirtyPagesWriteCount);
    PhUpdateDelta(&MappedWritesDelta, PhPerfInformation.MappedPagesWriteCount);
    PhUpdateDelta(&MappedIoReadDelta, UInt32x32To64(PhPerfInformation.PageReadCount, PAGE_SIZE));
    PhUpdateDelta(&MappedIoWritesDelta, ((ULONG64)PhPerfInformation.MappedPagesWriteCount + PhPerfInformation.DirtyPagesWriteCount + PhPerfInformation.CcLazyWritePages) * PAGE_SIZE);

    if (MemoryTicked < 2)
        MemoryTicked++;

    PhSipUpdateMemoryGraphs();
    PhSipUpdateMemoryPanel();
}

INT_PTR CALLBACK PhSipMemoryDialogProc(
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
            HWND totalPhysicalLabel;

            PhSipInitializeMemoryDialog();

            MemoryDialog = hwndDlg;
            totalPhysicalLabel = GetDlgItem(hwndDlg, IDC_TOTALPHYSICAL);

            PhInitializeLayoutManager(&MemoryLayoutManager, hwndDlg);
            PhAddLayoutItem(&MemoryLayoutManager, totalPhysicalLabel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            graphItem = PhAddLayoutItem(&MemoryLayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&MemoryLayoutManager, GetDlgItem(hwndDlg, IDC_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            MemoryGraphMargin = graphItem->Margin;

            SetWindowFont(GetDlgItem(hwndDlg, IDC_TITLE), MemorySection->Parameters->LargeFont, FALSE);
            SetWindowFont(totalPhysicalLabel, MemorySection->Parameters->MediumFont, FALSE);

            if (PhGetPhysicallyInstalledSystemMemory(&InstalledMemory, &ReservedMemory))
            {
                PhSetWindowText(totalPhysicalLabel, PhaConcatStrings2(
                    PhaFormatSize(InstalledMemory, ULONG_MAX)->Buffer, L" installed")->Buffer);
            }
            else
            {
                PhSetWindowText(totalPhysicalLabel, PhaConcatStrings2(
                    PhaFormatSize(UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages, PAGE_SIZE), ULONG_MAX)->Buffer, L" total")->Buffer);
            }

            MemoryPanel = PhCreateDialog(PhInstanceHandle, MAKEINTRESOURCE(IDD_SYSINFO_MEMPANEL), hwndDlg, PhSipMemoryPanelDialogProc, NULL);
            ShowWindow(MemoryPanel, SW_SHOW);
            PhAddLayoutItemEx(&MemoryLayoutManager, MemoryPanel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, panelItem->Margin);

            PhSipCreateMemoryGraphs();
            PhSipUpdateMemoryGraphs();
            PhSipUpdateMemoryPanel();
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&MemoryLayoutManager);
        }
        break;
    case WM_SIZE:
        {
            CommitGraphState.Valid = FALSE;
            CommitGraphState.TooltipIndex = ULONG_MAX;
            PhysicalGraphState.Valid = FALSE;
            PhysicalGraphState.TooltipIndex = ULONG_MAX;

            PhLayoutManagerLayout(&MemoryLayoutManager);
            PhSipLayoutMemoryGraphs();
        }
        break;
    case WM_NOTIFY:
        {
            NMHDR *header = (NMHDR *)lParam;

            if (header->hwndFrom == CommitGraphHandle)
            {
                PhSipNotifyCommitGraph(header);
            }
            else if (header->hwndFrom == PhysicalGraphHandle)
            {
                PhSipNotifyPhysicalGraph(header);
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

INT_PTR CALLBACK PhSipMemoryPanelDialogProc(
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
            NOTHING;
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_MORE:
                {
                    PhShowMemoryListsDialog(PhSipWindow, PhSipRegisterDialog, PhSipUnregisterDialog);
                }
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

VOID PhSipCreateMemoryGraphs(
    VOID
    )
{
    CommitGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        MemoryDialog,
        NULL,
        PhInstanceHandle,
        NULL
        );
    Graph_SetTooltip(CommitGraphHandle, TRUE);

    PhysicalGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        MemoryDialog,
        NULL,
        PhInstanceHandle,
        NULL
        );
    Graph_SetTooltip(PhysicalGraphHandle, TRUE);
}

VOID PhSipLayoutMemoryGraphs(
    VOID
    )
{
    RECT clientRect;
    RECT labelRect;
    ULONG graphWidth;
    ULONG graphHeight;
    HDWP deferHandle;
    ULONG y;

    GetClientRect(MemoryDialog, &clientRect);
    GetClientRect(GetDlgItem(MemoryDialog, IDC_COMMIT_L), &labelRect);
    graphWidth = clientRect.right - MemoryGraphMargin.left - MemoryGraphMargin.right;
    graphHeight = (clientRect.bottom - MemoryGraphMargin.top - MemoryGraphMargin.bottom - labelRect.bottom * 2 - MemorySection->Parameters->MemoryPadding * 3) / 2;

    deferHandle = BeginDeferWindowPos(4);
    y = MemoryGraphMargin.top;

    deferHandle = DeferWindowPos(
        deferHandle,
        GetDlgItem(MemoryDialog, IDC_COMMIT_L),
        NULL,
        MemoryGraphMargin.left,
        y,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += labelRect.bottom + MemorySection->Parameters->MemoryPadding;

    deferHandle = DeferWindowPos(
        deferHandle,
        CommitGraphHandle,
        NULL,
        MemoryGraphMargin.left,
        y,
        graphWidth,
        graphHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += graphHeight + MemorySection->Parameters->MemoryPadding;

    deferHandle = DeferWindowPos(
        deferHandle,
        GetDlgItem(MemoryDialog, IDC_PHYSICAL_L),
        NULL,
        MemoryGraphMargin.left,
        y,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER
        );
    y += labelRect.bottom + MemorySection->Parameters->MemoryPadding;

    deferHandle = DeferWindowPos(
        deferHandle,
        PhysicalGraphHandle,
        NULL,
        MemoryGraphMargin.left,
        y,
        graphWidth,
        clientRect.bottom - MemoryGraphMargin.bottom - y,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    EndDeferWindowPos(deferHandle);
}

VOID PhSipNotifyCommitGraph(
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

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (PhCsEnableGraphMaxText ? PH_GRAPH_LABEL_MAX_Y : 0);
            PhSiSetColorsGraphDrawInfo(drawInfo, PhCsColorPrivate, 0);

            PhGraphStateGetDrawInfo(
                &CommitGraphState,
                getDrawInfo,
                PhCommitHistory.Count
                );

            if (!CommitGraphState.Valid)
            {
                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    CommitGraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&PhCommitHistory, i);
                }

                if (PhPerfInformation.CommitLimit != 0)
                {
                    // Scale the data.
                    PhDivideSinglesBySingle(
                        CommitGraphState.Data1,
                        (FLOAT)PhPerfInformation.CommitLimit,
                        drawInfo->LineDataCount
                        );
                }

                if (PhCsEnableGraphMaxText)
                {
                    drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                    drawInfo->LabelYFunctionParameter = (FLOAT)PhPerfInformation.CommitLimit * PAGE_SIZE;
                }

                CommitGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (CommitGraphState.TooltipIndex != getTooltipText->Index)
                {
                    ULONG usedPages;
                    PH_FORMAT format[3];

                    usedPages = PhGetItemCircularBuffer_ULONG(&PhCommitHistory, getTooltipText->Index);

                    // Commit charge: %s\n%s
                    //PhInitFormatS(&format[0], L"Commit charge: ");
                    PhInitFormatSize(&format[0], UInt32x32To64(usedPages, PAGE_SIZE));
                    PhInitFormatC(&format[1], L'\n');
                    PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&CommitGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 128));
                }

                getTooltipText->Text = CommitGraphState.TooltipText->sr;
            }
        }
        break;
    }
}

VOID PhSipNotifyPhysicalGraph(
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

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | (PhCsEnableGraphMaxText ? PH_GRAPH_LABEL_MAX_Y : 0);
            PhSiSetColorsGraphDrawInfo(drawInfo, PhCsColorPhysical, 0);

            PhGraphStateGetDrawInfo(
                &PhysicalGraphState,
                getDrawInfo,
                PhPhysicalHistory.Count
                );

            if (!PhysicalGraphState.Valid)
            {
                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    PhysicalGraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&PhPhysicalHistory, i);
                }

                if (PhSystemBasicInformation.NumberOfPhysicalPages != 0)
                {
                    // Scale the data.
                    PhDivideSinglesBySingle(
                        PhysicalGraphState.Data1,
                        (FLOAT)PhSystemBasicInformation.NumberOfPhysicalPages,
                        drawInfo->LineDataCount
                        );
                }

                if (PhCsEnableGraphMaxText)
                {
                    drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                    drawInfo->LabelYFunctionParameter = (FLOAT)PhSystemBasicInformation.NumberOfPhysicalPages * PAGE_SIZE;
                }

                PhysicalGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (PhysicalGraphState.TooltipIndex != getTooltipText->Index)
                {
                    ULONG usedPages;
                    DOUBLE currentCompressedMemory;
                    DOUBLE totalCompressedMemory;
                    DOUBLE totalSavedMemory;

                    usedPages = PhGetItemCircularBuffer_ULONG(&PhPhysicalHistory, getTooltipText->Index);

                    if (PhSipGetMemoryCompressionLimits(&currentCompressedMemory, &totalCompressedMemory, &totalSavedMemory))
                    {
                        PH_FORMAT format[13];

                        PhInitFormatS(&format[0], L"Physical memory: ");
                        PhInitFormatSize(&format[1], UInt32x32To64(usedPages, PAGE_SIZE));
                        PhInitFormatC(&format[2], L'\n');
                        PhInitFormatS(&format[3], L"Compressed memory: ");
                        PhInitFormatSize(&format[4], (ULONG64)currentCompressedMemory);
                        PhInitFormatC(&format[5], L'\n');
                        PhInitFormatS(&format[6], L"Total compressed: ");
                        PhInitFormatSize(&format[7], (ULONG64)totalCompressedMemory);
                        PhInitFormatC(&format[8], L'\n');
                        PhInitFormatS(&format[9], L"Total memory saved: ");
                        PhInitFormatSize(&format[10], (ULONG64)totalSavedMemory);
                        PhInitFormatC(&format[11], L'\n');
                        PhInitFormatSR(&format[12], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                        PhMoveReference(&PhysicalGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                    }
                    else
                    {
                        PH_FORMAT format[3];

                        // Physical memory: %s\n%s
                        PhInitFormatSize(&format[0], UInt32x32To64(usedPages, PAGE_SIZE));
                        PhInitFormatC(&format[1], L'\n');
                        PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                        PhMoveReference(&PhysicalGraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                    }
                }

                getTooltipText->Text = PhysicalGraphState.TooltipText->sr;
            }
        }
        break;
    }
}

VOID PhSipUpdateMemoryGraphs(
    VOID
    )
{
    CommitGraphState.Valid = FALSE;
    CommitGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(CommitGraphHandle, 1);
    Graph_Draw(CommitGraphHandle);
    Graph_UpdateTooltip(CommitGraphHandle);
    InvalidateRect(CommitGraphHandle, NULL, FALSE);

    PhysicalGraphState.Valid = FALSE;
    PhysicalGraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(PhysicalGraphHandle, 1);
    Graph_Draw(PhysicalGraphHandle);
    Graph_UpdateTooltip(PhysicalGraphHandle);
    InvalidateRect(PhysicalGraphHandle, NULL, FALSE);
}

VOID PhSipUpdateMemoryPanel(
    VOID
    )
{
    PWSTR pagedLimit;
    PWSTR nonPagedLimit;
    SYSTEM_MEMORY_LIST_INFORMATION memoryListInfo;

    // Commit charge

    PhSetDialogItemText(MemoryPanel, IDC_ZCOMMITCURRENT_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.CommittedPages, PAGE_SIZE), ULONG_MAX)->Buffer);
    PhSetDialogItemText(MemoryPanel, IDC_ZCOMMITPEAK_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.PeakCommitment, PAGE_SIZE), ULONG_MAX)->Buffer);
    PhSetDialogItemText(MemoryPanel, IDC_ZCOMMITLIMIT_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.CommitLimit, PAGE_SIZE), ULONG_MAX)->Buffer);

    // Physical memory

    PhSetDialogItemText(MemoryPanel, IDC_ZPHYSICALCURRENT_V,
        PhaFormatSize(UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages - PhPerfInformation.AvailablePages, PAGE_SIZE), ULONG_MAX)->Buffer);
    PhSetDialogItemText(MemoryPanel, IDC_ZPHYSICALTOTAL_V,
        PhaFormatSize(UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages, PAGE_SIZE), ULONG_MAX)->Buffer);

    if (ReservedMemory != 0)
        PhSetDialogItemText(MemoryPanel, IDC_ZPHYSICALRESERVED_V, PhaFormatSize(ReservedMemory, ULONG_MAX)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZPHYSICALRESERVED_V, L"-");

    PhSetDialogItemText(MemoryPanel, IDC_ZPHYSICALCACHEWS_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.ResidentSystemCachePage, PAGE_SIZE), ULONG_MAX)->Buffer);
    PhSetDialogItemText(MemoryPanel, IDC_ZPHYSICALKERNELWS_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.ResidentSystemCodePage, PAGE_SIZE), ULONG_MAX)->Buffer);
    PhSetDialogItemText(MemoryPanel, IDC_ZPHYSICALDRIVERWS_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.ResidentSystemDriverPage, PAGE_SIZE), ULONG_MAX)->Buffer);

    // Paged pool

    PhSetDialogItemText(MemoryPanel, IDC_ZPAGEDWORKINGSET_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.ResidentPagedPoolPage, PAGE_SIZE), ULONG_MAX)->Buffer);
    PhSetDialogItemText(MemoryPanel, IDC_ZPAGEDVIRTUALSIZE_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.PagedPoolPages, PAGE_SIZE), ULONG_MAX)->Buffer);

    if (MemoryTicked > 1)
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGEDALLOCSDELTA_V, PhaFormatUInt64(PagedAllocsDelta.Delta, TRUE)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGEDALLOCSDELTA_V, L"-");

    if (MemoryTicked > 1)
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGEDFREESDELTA_V, PhaFormatUInt64(PagedFreesDelta.Delta, TRUE)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGEDFREESDELTA_V, L"-");

    // Non-paged pool

    PhSetDialogItemText(MemoryPanel, IDC_ZNONPAGEDUSAGE_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.NonPagedPoolPages, PAGE_SIZE), ULONG_MAX)->Buffer);

    if (MemoryTicked > 1)
        PhSetDialogItemText(MemoryPanel, IDC_ZNONPAGEDALLOCSDELTA_V, PhaFormatUInt64(PagedAllocsDelta.Delta, TRUE)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZNONPAGEDALLOCSDELTA_V, L"-");

    if (MemoryTicked > 1)
        PhSetDialogItemText(MemoryPanel, IDC_ZNONPAGEDFREESDELTA_V, PhaFormatUInt64(NonPagedFreesDelta.Delta, TRUE)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZNONPAGEDFREESDELTA_V, L"-");

    // Pools (KPH)

    if (MmAddressesInitialized && (MmSizeOfPagedPoolInBytes || MmMaximumNonPagedPoolInBytes))
    {
        SIZE_T paged;
        SIZE_T nonPaged;

        PhSipGetPoolLimits(&paged, &nonPaged);

        if (paged != -1)
            pagedLimit = PhaFormatSize(paged, ULONG_MAX)->Buffer;
        else
            pagedLimit = L"N/A";

        if (nonPaged != -1)
            nonPagedLimit = PhaFormatSize(nonPaged, ULONG_MAX)->Buffer;
        else
            nonPagedLimit = L"N/A";
    }
    else
    {
        if (!KphIsConnected())
        {
            pagedLimit = nonPagedLimit = L"no driver";
        }
        else
        {
            pagedLimit = nonPagedLimit = L"no symbols";
        }
    }

    PhSetDialogItemText(MemoryPanel, IDC_ZPAGEDLIMIT_V, pagedLimit);
    PhSetDialogItemText(MemoryPanel, IDC_ZNONPAGEDLIMIT_V, nonPagedLimit);

    // Paging

    if (MemoryTicked > 1)
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGINGPAGEFAULTSDELTA_V, PhaFormatUInt64(PageFaultsDelta.Delta, TRUE)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGINGPAGEFAULTSDELTA_V, L"-");

    if (MemoryTicked > 1)
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGINGPAGEREADSDELTA_V, PhaFormatUInt64(PageReadsDelta.Delta, TRUE)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGINGPAGEREADSDELTA_V, L"-");

    if (MemoryTicked > 1)
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGINGPAGEFILEWRITESDELTA_V, PhaFormatUInt64(PagefileWritesDelta.Delta, TRUE)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGINGPAGEFILEWRITESDELTA_V, L"-");

    if (MemoryTicked > 1)
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGINGMAPPEDWRITESDELTA_V, PhaFormatUInt64(MappedWritesDelta.Delta, TRUE)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZPAGINGMAPPEDWRITESDELTA_V, L"-");

    // Mapped

    if (MemoryTicked > 1)
        PhSetDialogItemText(MemoryPanel, IDC_ZMAPPEDREADIO, PhaFormatSize(MappedIoReadDelta.Delta, ULONG_MAX)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZMAPPEDREADIO, L"-");

    if (MemoryTicked > 1)
        PhSetDialogItemText(MemoryPanel, IDC_ZMAPPEDWRITEIO, PhaFormatSize(MappedIoWritesDelta.Delta, ULONG_MAX)->Buffer);
    else
        PhSetDialogItemText(MemoryPanel, IDC_ZMAPPEDWRITEIO, L"-");

    // Memory lists

    if (NT_SUCCESS(NtQuerySystemInformation(
        SystemMemoryListInformation,
        &memoryListInfo,
        sizeof(SYSTEM_MEMORY_LIST_INFORMATION),
        NULL
        )))
    {
        ULONG_PTR standbyPageCount;
        ULONG_PTR repurposedPageCount;
        ULONG i;

        standbyPageCount = 0;
        repurposedPageCount = 0;

        for (i = 0; i < 8; i++)
        {
            standbyPageCount += memoryListInfo.PageCountByPriority[i];
            repurposedPageCount += memoryListInfo.RepurposedPagesByPriority[i];
        }

        PhSetDialogItemText(MemoryPanel, IDC_ZLISTZEROED_V, PhaFormatSize((ULONG64)memoryListInfo.ZeroPageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTFREE_V, PhaFormatSize((ULONG64)memoryListInfo.FreePageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTMODIFIED_V, PhaFormatSize((ULONG64)memoryListInfo.ModifiedPageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTMODIFIEDNOWRITE_V, PhaFormatSize((ULONG64)memoryListInfo.ModifiedNoWritePageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY_V, PhaFormatSize((ULONG64)standbyPageCount * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY0_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[0] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY1_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[1] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY2_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[2] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY3_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[3] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY4_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[4] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY5_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[5] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY6_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[6] * PAGE_SIZE, ULONG_MAX)->Buffer);
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY7_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[7] * PAGE_SIZE, ULONG_MAX)->Buffer);

        if (WindowsVersion >= WINDOWS_8)
            PhSetDialogItemText(MemoryPanel, IDC_ZLISTMODIFIEDPAGEFILE_V, PhaFormatSize((ULONG64)memoryListInfo.ModifiedPageCountPageFile * PAGE_SIZE, ULONG_MAX)->Buffer);
        else
            PhSetDialogItemText(MemoryPanel, IDC_ZLISTMODIFIEDPAGEFILE_V, L"N/A");
    }
    else
    {
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTZEROED_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTFREE_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTMODIFIED_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTMODIFIEDNOWRITE_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTMODIFIEDPAGEFILE_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY0_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY1_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY2_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY3_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY4_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY5_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY6_V, L"N/A");
        PhSetDialogItemText(MemoryPanel, IDC_ZLISTSTANDBY7_V, L"N/A");
    }
}

NTSTATUS PhSipLoadMmAddresses(
    _In_ PVOID Parameter
    )
{
    PRTL_PROCESS_MODULES kernelModules;
    PPH_SYMBOL_PROVIDER symbolProvider;
    PPH_STRING kernelFileName;
    PH_SYMBOL_INFORMATION symbolInfo;

    if (NT_SUCCESS(PhEnumKernelModules(&kernelModules)))
    {
        if (kernelModules->NumberOfModules >= 1)
        {
            symbolProvider = PhCreateSymbolProvider(NULL);
            PhLoadSymbolProviderOptions(symbolProvider);

            kernelFileName = PH_AUTO(PhConvertMultiByteToUtf16(kernelModules->Modules[0].FullPathName));

            PhLoadModuleSymbolProvider(
                symbolProvider,
                kernelFileName,
                (ULONG64)kernelModules->Modules[0].ImageBase,
                kernelModules->Modules[0].ImageSize
                );

            if (PhGetSymbolFromName(
                symbolProvider,
                L"MmSizeOfPagedPoolInBytes",
                &symbolInfo
                ))
            {
                MmSizeOfPagedPoolInBytes = (PSIZE_T)symbolInfo.Address;
            }

            if (PhGetSymbolFromName(
                symbolProvider,
                L"MmMaximumNonPagedPoolInBytes",
                &symbolInfo
                ))
            {
                MmMaximumNonPagedPoolInBytes = (PSIZE_T)symbolInfo.Address;
            }

            PhDereferenceObject(symbolProvider);
        }

        PhFree(kernelModules);
    }

    return STATUS_SUCCESS;
}

VOID PhSipGetPoolLimits(
    _Out_ PSIZE_T Paged,
    _Out_ PSIZE_T NonPaged
    )
{
    SIZE_T paged = -1;
    SIZE_T nonPaged = -1;

    if (MmSizeOfPagedPoolInBytes && WindowsVersion < WINDOWS_8)
    {
        KphReadVirtualMemoryUnsafe(
            NtCurrentProcess(),
            MmSizeOfPagedPoolInBytes,
            &paged,
            sizeof(SIZE_T),
            NULL
            );
    }

    if (MmMaximumNonPagedPoolInBytes)
    {
        KphReadVirtualMemoryUnsafe(
            NtCurrentProcess(),
            MmMaximumNonPagedPoolInBytes,
            &nonPaged,
            sizeof(SIZE_T),
            NULL
            );
    }

    *Paged = paged;
    *NonPaged = nonPaged;
}

_Success_(return)
BOOLEAN PhSipGetMemoryCompressionLimits(
    _Out_ DOUBLE *CurrentCompressedMemory,
    _Out_ DOUBLE *TotalCompressedMemory,
    _Out_ DOUBLE *TotalSavedMemory
    )
{
    PH_SYSTEM_STORE_COMPRESSION_INFORMATION compressionInfo;
    DOUBLE current;
    DOUBLE total;
    DOUBLE saved;

    if (!NT_SUCCESS(PhGetSystemCompressionStoreInformation(&compressionInfo)))
        return FALSE;
    if (!(compressionInfo.TotalDataCompressed && compressionInfo.TotalCompressedSize))
        return FALSE;

    current = (DOUBLE)compressionInfo.WorkingSetSize / (DOUBLE)(1024 * 1024);
    total = current * (DOUBLE)compressionInfo.TotalDataCompressed / (DOUBLE)compressionInfo.TotalCompressedSize;
    saved = total - current;

    *CurrentCompressedMemory = current * 1024 * 1024;
    *TotalCompressedMemory = total * 1024 * 1024;
    *TotalSavedMemory = saved * 1024 * 1024;
    return TRUE;
}
