/*
 * Process Hacker -
 *   System Information memory section
 *
 * Copyright (C) 2011-2016 wj32
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

#include <phapp.h>
#include <workqueue.h>
#include <kphuser.h>
#include <symprv.h>
#include <procprv.h>
#include <settings.h>
#include <sysinfo.h>
#include <sysinfop.h>

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
static BOOLEAN MmAddressesInitialized;
static PSIZE_T MmSizeOfPagedPoolInBytes;
static PSIZE_T MmMaximumNonPagedPoolInBytes;
static ULONGLONG InstalledMemory;

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
    case SysInfoCreateDialog:
        {
            PPH_SYSINFO_CREATE_DIALOG createDialog = Parameter1;

            createDialog->Instance = PhInstanceHandle;
            createDialog->Template = MAKEINTRESOURCE(IDD_SYSINFO_MEM);
            createDialog->DialogProc = PhSipMemoryDialogProc;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = Parameter1;
            ULONG i;

            if (PhGetIntegerSetting(L"ShowCommitInSummary"))
            {
                drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y;
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

                    Section->GraphState.Valid = TRUE;
                }
            }
            else
            {
                drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y;
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

                    Section->GraphState.Valid = TRUE;
                }
            }
        }
        return TRUE;
    case SysInfoGraphGetTooltipText:
        {
            PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT getTooltipText = Parameter1;
            ULONG usedPages;

            if (PhGetIntegerSetting(L"ShowCommitInSummary"))
            {
                usedPages = PhGetItemCircularBuffer_ULONG(&PhCommitHistory, getTooltipText->Index);

                PhMoveReference(&Section->GraphState.TooltipText, PhFormatString(
                    L"Commit charge: %s\n%s",
                    PhaFormatSize(UInt32x32To64(usedPages, PAGE_SIZE), -1)->Buffer,
                    PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->Buffer
                    ));
                getTooltipText->Text = Section->GraphState.TooltipText->sr;
            }
            else
            {
                usedPages = PhGetItemCircularBuffer_ULONG(&PhPhysicalHistory, getTooltipText->Index);

                PhMoveReference(&Section->GraphState.TooltipText, PhFormatString(
                    L"Physical memory: %s\n%s",
                    PhaFormatSize(UInt32x32To64(usedPages, PAGE_SIZE), -1)->Buffer,
                    PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->Buffer
                    ));
                getTooltipText->Text = Section->GraphState.TooltipText->sr;
            }
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = Parameter1;
            ULONG totalPages;
            ULONG usedPages;

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
            drawPanel->SubTitle = PhFormatString(
                L"%.0f%%\n%s / %s",
                (FLOAT)usedPages * 100 / totalPages,
                PhSipFormatSizeWithPrecision(UInt32x32To64(usedPages, PAGE_SIZE), 1)->Buffer,
                PhSipFormatSizeWithPrecision(UInt32x32To64(totalPages, PAGE_SIZE), 1)->Buffer
                );
            drawPanel->SubTitleOverflow = PhFormatString(
                L"%.0f%%\n%s",
                (FLOAT)usedPages * 100 / totalPages,
                PhSipFormatSizeWithPrecision(UInt32x32To64(usedPages, PAGE_SIZE), 1)->Buffer
                );
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

    MemoryTicked++;

    if (MemoryTicked > 2)
        MemoryTicked = 2;

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
            static BOOL (WINAPI *getPhysicallyInstalledSystemMemory)(PULONGLONG) = NULL;

            PPH_LAYOUT_ITEM graphItem;
            PPH_LAYOUT_ITEM panelItem;

            PhSipInitializeMemoryDialog();

            MemoryDialog = hwndDlg;
            PhInitializeLayoutManager(&MemoryLayoutManager, hwndDlg);
            PhAddLayoutItem(&MemoryLayoutManager, GetDlgItem(hwndDlg, IDC_TOTALPHYSICAL), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            graphItem = PhAddLayoutItem(&MemoryLayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            MemoryGraphMargin = graphItem->Margin;
            panelItem = PhAddLayoutItem(&MemoryLayoutManager, GetDlgItem(hwndDlg, IDC_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            SendMessage(GetDlgItem(hwndDlg, IDC_TITLE), WM_SETFONT, (WPARAM)MemorySection->Parameters->LargeFont, FALSE);
            SendMessage(GetDlgItem(hwndDlg, IDC_TOTALPHYSICAL), WM_SETFONT, (WPARAM)MemorySection->Parameters->MediumFont, FALSE);

            if (!getPhysicallyInstalledSystemMemory)
                getPhysicallyInstalledSystemMemory = PhGetModuleProcAddress(L"kernel32.dll", "GetPhysicallyInstalledSystemMemory");

            InstalledMemory = 0;

            if (getPhysicallyInstalledSystemMemory && getPhysicallyInstalledSystemMemory(&InstalledMemory))
            {
                SetDlgItemText(hwndDlg, IDC_TOTALPHYSICAL,
                    PhaConcatStrings2(PhaFormatSize(InstalledMemory * 1024, -1)->Buffer, L" installed")->Buffer);
            }
            else
            {
                SetDlgItemText(hwndDlg, IDC_TOTALPHYSICAL,
                    PhaConcatStrings2(PhaFormatSize(UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages, PAGE_SIZE), -1)->Buffer, L" total")->Buffer);
            }

            MemoryPanel = CreateDialog(
                PhInstanceHandle,
                WindowsVersion >= WINDOWS_VISTA ? MAKEINTRESOURCE(IDD_SYSINFO_MEMPANEL) : MAKEINTRESOURCE(IDD_SYSINFO_MEMPANELXP),
                hwndDlg,
                PhSipMemoryPanelDialogProc
                );
            ShowWindow(MemoryPanel, SW_SHOW);
            PhAddLayoutItemEx(&MemoryLayoutManager, MemoryPanel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, panelItem->Margin);

            CommitGraphHandle = CreateWindow(
                PH_GRAPH_CLASSNAME,
                NULL,
                WS_VISIBLE | WS_CHILD | WS_BORDER,
                0,
                0,
                3,
                3,
                MemoryDialog,
                (HMENU)IDC_COMMIT,
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
                (HMENU)IDC_PHYSICAL,
                PhInstanceHandle,
                NULL
                );
            Graph_SetTooltip(PhysicalGraphHandle, TRUE);

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
            switch (LOWORD(wParam))
            {
            case IDC_MORE:
                {
                    PhShowMemoryListsDialog(PhSipWindow, PhSipRegisterDialog, PhSipUnregisterDialog);
                }
                break;
            }
        }
        break;
    }

    return FALSE;
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

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y;
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

                    usedPages = PhGetItemCircularBuffer_ULONG(&PhCommitHistory, getTooltipText->Index);

                    PhMoveReference(&CommitGraphState.TooltipText, PhFormatString(
                        L"Commit charge: %s\n%s",
                        PhaFormatSize(UInt32x32To64(usedPages, PAGE_SIZE), -1)->Buffer,
                        PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->Buffer
                        ));
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

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y;
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

                    usedPages = PhGetItemCircularBuffer_ULONG(&PhPhysicalHistory, getTooltipText->Index);

                    PhMoveReference(&PhysicalGraphState.TooltipText, PhFormatString(
                        L"Physical memory: %s\n%s",
                        PhaFormatSize(UInt32x32To64(usedPages, PAGE_SIZE), -1)->Buffer,
                        PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->Buffer
                        ));
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
    CommitGraphState.TooltipIndex = -1;
    Graph_MoveGrid(CommitGraphHandle, 1);
    Graph_Draw(CommitGraphHandle);
    Graph_UpdateTooltip(CommitGraphHandle);
    InvalidateRect(CommitGraphHandle, NULL, FALSE);

    PhysicalGraphState.Valid = FALSE;
    PhysicalGraphState.TooltipIndex = -1;
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

    SetDlgItemText(MemoryPanel, IDC_ZCOMMITCURRENT_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.CommittedPages, PAGE_SIZE), -1)->Buffer);
    SetDlgItemText(MemoryPanel, IDC_ZCOMMITPEAK_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.PeakCommitment, PAGE_SIZE), -1)->Buffer);
    SetDlgItemText(MemoryPanel, IDC_ZCOMMITLIMIT_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.CommitLimit, PAGE_SIZE), -1)->Buffer);

    // Physical memory

    SetDlgItemText(MemoryPanel, IDC_ZPHYSICALCURRENT_V,
        PhaFormatSize(UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages - PhPerfInformation.AvailablePages, PAGE_SIZE), -1)->Buffer);
    SetDlgItemText(MemoryPanel, IDC_ZPHYSICALTOTAL_V,
        PhaFormatSize(UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages, PAGE_SIZE), -1)->Buffer);

    if (InstalledMemory != 0)
    {
        SetDlgItemText(MemoryPanel, IDC_ZPHYSICALRESERVED_V, 
            PhaFormatSize(InstalledMemory * 1024 - UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages, PAGE_SIZE), -1)->Buffer);
    }
    else
    {
        SetDlgItemText(MemoryPanel, IDC_ZPHYSICALRESERVED_V, L"-");
    }

    SetDlgItemText(MemoryPanel, IDC_ZPHYSICALCACHEWS_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.ResidentSystemCachePage, PAGE_SIZE), -1)->Buffer);
    SetDlgItemText(MemoryPanel, IDC_ZPHYSICALKERNELWS_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.ResidentSystemCodePage, PAGE_SIZE), -1)->Buffer);
    SetDlgItemText(MemoryPanel, IDC_ZPHYSICALDRIVERWS_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.ResidentSystemDriverPage, PAGE_SIZE), -1)->Buffer);

    // Paged pool

    SetDlgItemText(MemoryPanel, IDC_ZPAGEDWORKINGSET_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.ResidentPagedPoolPage, PAGE_SIZE), -1)->Buffer);
    SetDlgItemText(MemoryPanel, IDC_ZPAGEDVIRTUALSIZE_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.PagedPoolPages, PAGE_SIZE), -1)->Buffer);

    if (MemoryTicked > 1)
        SetDlgItemText(MemoryPanel, IDC_ZPAGEDALLOCSDELTA_V, PhaFormatUInt64(PagedAllocsDelta.Delta, TRUE)->Buffer);
    else
        SetDlgItemText(MemoryPanel, IDC_ZPAGEDALLOCSDELTA_V, L"-");

    if (MemoryTicked > 1)
        SetDlgItemText(MemoryPanel, IDC_ZPAGEDFREESDELTA_V, PhaFormatUInt64(PagedFreesDelta.Delta, TRUE)->Buffer);
    else
        SetDlgItemText(MemoryPanel, IDC_ZPAGEDFREESDELTA_V, L"-");

    // Non-paged pool

    SetDlgItemText(MemoryPanel, IDC_ZNONPAGEDUSAGE_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.NonPagedPoolPages, PAGE_SIZE), -1)->Buffer);

    if (MemoryTicked > 1)
        SetDlgItemText(MemoryPanel, IDC_ZNONPAGEDALLOCSDELTA_V, PhaFormatUInt64(PagedAllocsDelta.Delta, TRUE)->Buffer);
    else
        SetDlgItemText(MemoryPanel, IDC_ZNONPAGEDALLOCSDELTA_V, L"-");

    if (MemoryTicked > 1)
        SetDlgItemText(MemoryPanel, IDC_ZNONPAGEDFREESDELTA_V, PhaFormatUInt64(NonPagedFreesDelta.Delta, TRUE)->Buffer);
    else
        SetDlgItemText(MemoryPanel, IDC_ZNONPAGEDFREESDELTA_V, L"-");

    // Pools (KPH)

    if (MmAddressesInitialized && (MmSizeOfPagedPoolInBytes || MmMaximumNonPagedPoolInBytes))
    {
        SIZE_T paged;
        SIZE_T nonPaged;

        PhSipGetPoolLimits(&paged, &nonPaged);
        
        if (paged != -1)
            pagedLimit = PhaFormatSize(paged, -1)->Buffer;
        else
            pagedLimit = L"N/A";

        if (nonPaged != -1)
            nonPagedLimit = PhaFormatSize(nonPaged, -1)->Buffer;
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

    SetDlgItemText(MemoryPanel, IDC_ZPAGEDLIMIT_V, pagedLimit);
    SetDlgItemText(MemoryPanel, IDC_ZNONPAGEDLIMIT_V, nonPagedLimit);

    // Paging

    if (MemoryTicked > 1)
        SetDlgItemText(MemoryPanel, IDC_ZPAGINGPAGEFAULTSDELTA_V, PhaFormatUInt64(PageFaultsDelta.Delta, TRUE)->Buffer);
    else
        SetDlgItemText(MemoryPanel, IDC_ZPAGINGPAGEFAULTSDELTA_V, L"-");

    if (MemoryTicked > 1)
        SetDlgItemText(MemoryPanel, IDC_ZPAGINGPAGEREADSDELTA_V, PhaFormatUInt64(PageReadsDelta.Delta, TRUE)->Buffer);
    else
        SetDlgItemText(MemoryPanel, IDC_ZPAGINGPAGEREADSDELTA_V, L"-");

    if (MemoryTicked > 1)
        SetDlgItemText(MemoryPanel, IDC_ZPAGINGPAGEFILEWRITESDELTA_V, PhaFormatUInt64(PagefileWritesDelta.Delta, TRUE)->Buffer);
    else
        SetDlgItemText(MemoryPanel, IDC_ZPAGINGPAGEFILEWRITESDELTA_V, L"-");

    if (MemoryTicked > 1)
        SetDlgItemText(MemoryPanel, IDC_ZPAGINGMAPPEDWRITESDELTA_V, PhaFormatUInt64(MappedWritesDelta.Delta, TRUE)->Buffer);
    else
        SetDlgItemText(MemoryPanel, IDC_ZPAGINGMAPPEDWRITESDELTA_V, L"-");

    // Memory lists

    if (WindowsVersion >= WINDOWS_VISTA)
    {
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

            SetDlgItemText(MemoryPanel, IDC_ZLISTZEROED_V, PhaFormatSize((ULONG64)memoryListInfo.ZeroPageCount * PAGE_SIZE, -1)->Buffer);
            SetDlgItemText(MemoryPanel, IDC_ZLISTFREE_V, PhaFormatSize((ULONG64)memoryListInfo.FreePageCount * PAGE_SIZE, -1)->Buffer);
            SetDlgItemText(MemoryPanel, IDC_ZLISTMODIFIED_V, PhaFormatSize((ULONG64)memoryListInfo.ModifiedPageCount * PAGE_SIZE, -1)->Buffer);
            SetDlgItemText(MemoryPanel, IDC_ZLISTMODIFIEDNOWRITE_V, PhaFormatSize((ULONG64)memoryListInfo.ModifiedNoWritePageCount * PAGE_SIZE, -1)->Buffer);
            SetDlgItemText(MemoryPanel, IDC_ZLISTSTANDBY_V, PhaFormatSize((ULONG64)standbyPageCount * PAGE_SIZE, -1)->Buffer);
            SetDlgItemText(MemoryPanel, IDC_ZLISTSTANDBY0_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[0] * PAGE_SIZE, -1)->Buffer);
            SetDlgItemText(MemoryPanel, IDC_ZLISTSTANDBY1_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[1] * PAGE_SIZE, -1)->Buffer);
            SetDlgItemText(MemoryPanel, IDC_ZLISTSTANDBY2_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[2] * PAGE_SIZE, -1)->Buffer);
            SetDlgItemText(MemoryPanel, IDC_ZLISTSTANDBY3_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[3] * PAGE_SIZE, -1)->Buffer);
            SetDlgItemText(MemoryPanel, IDC_ZLISTSTANDBY4_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[4] * PAGE_SIZE, -1)->Buffer);
            SetDlgItemText(MemoryPanel, IDC_ZLISTSTANDBY5_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[5] * PAGE_SIZE, -1)->Buffer);
            SetDlgItemText(MemoryPanel, IDC_ZLISTSTANDBY6_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[6] * PAGE_SIZE, -1)->Buffer);
            SetDlgItemText(MemoryPanel, IDC_ZLISTSTANDBY7_V, PhaFormatSize((ULONG64)memoryListInfo.PageCountByPriority[7] * PAGE_SIZE, -1)->Buffer);

            if (WindowsVersion >= WINDOWS_8)
                SetDlgItemText(MemoryPanel, IDC_ZLISTMODIFIEDPAGEFILE_V, PhaFormatSize((ULONG64)memoryListInfo.ModifiedPageCountPageFile * PAGE_SIZE, -1)->Buffer);
            else
                SetDlgItemText(MemoryPanel, IDC_ZLISTMODIFIEDPAGEFILE_V, L"N/A");
        }
        else
        {
            SetDlgItemText(MemoryPanel, IDC_ZLISTZEROED_V, L"N/A");
            SetDlgItemText(MemoryPanel, IDC_ZLISTFREE_V, L"N/A");
            SetDlgItemText(MemoryPanel, IDC_ZLISTMODIFIED_V, L"N/A");
            SetDlgItemText(MemoryPanel, IDC_ZLISTMODIFIEDNOWRITE_V, L"N/A");
            SetDlgItemText(MemoryPanel, IDC_ZLISTMODIFIEDPAGEFILE_V, L"N/A");
            SetDlgItemText(MemoryPanel, IDC_ZLISTSTANDBY_V, L"N/A");
            SetDlgItemText(MemoryPanel, IDC_ZLISTSTANDBY0_V, L"N/A");
            SetDlgItemText(MemoryPanel, IDC_ZLISTSTANDBY1_V, L"N/A");
            SetDlgItemText(MemoryPanel, IDC_ZLISTSTANDBY2_V, L"N/A");
            SetDlgItemText(MemoryPanel, IDC_ZLISTSTANDBY3_V, L"N/A");
            SetDlgItemText(MemoryPanel, IDC_ZLISTSTANDBY4_V, L"N/A");
            SetDlgItemText(MemoryPanel, IDC_ZLISTSTANDBY5_V, L"N/A");
            SetDlgItemText(MemoryPanel, IDC_ZLISTSTANDBY6_V, L"N/A");
            SetDlgItemText(MemoryPanel, IDC_ZLISTSTANDBY7_V, L"N/A");
        }
    }
}

NTSTATUS PhSipLoadMmAddresses(
    _In_ PVOID Parameter
    )
{
    PRTL_PROCESS_MODULES kernelModules;
    PPH_SYMBOL_PROVIDER symbolProvider;
    PPH_STRING kernelFileName;
    PPH_STRING newFileName;
    PH_SYMBOL_INFORMATION symbolInfo;

    if (NT_SUCCESS(PhEnumKernelModules(&kernelModules)))
    {
        if (kernelModules->NumberOfModules >= 1)
        {
            symbolProvider = PhCreateSymbolProvider(NULL);
            PhLoadSymbolProviderOptions(symbolProvider);

            kernelFileName = PH_AUTO(PhConvertMultiByteToUtf16(kernelModules->Modules[0].FullPathName));
            newFileName = PH_AUTO(PhGetFileName(kernelFileName));

            PhLoadModuleSymbolProvider(
                symbolProvider,
                newFileName->Buffer,
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
