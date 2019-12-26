/*
 * Process Hacker Extended Tools -
 *   GPU system information section
 *
 * Copyright (C) 2011 wj32
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
static HWND GpuPanel;

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

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y;
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), 0);
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, EtGpuNodeHistory.Count);

            if (!Section->GraphState.Valid)
            {
                PhCopyCircularBuffer_FLOAT(&EtGpuNodeHistory, Section->GraphState.Data1, drawInfo->LineDataCount);
                Section->GraphState.Valid = TRUE;
            }
        }
        return TRUE;
    case SysInfoGraphGetTooltipText:
        {
            PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT getTooltipText = Parameter1;
            FLOAT gpu;

            if (!getTooltipText)
                break;

            gpu = PhGetItemCircularBuffer_FLOAT(&EtGpuNodeHistory, getTooltipText->Index);

            PhMoveReference(&Section->GraphState.TooltipText, PhFormatString(
                L"%.2f%%%s\n%s",
                gpu * 100,
                PhGetStringOrEmpty(EtpGetMaxNodeString(getTooltipText->Index)),
                ((PPH_STRING)PH_AUTO(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                ));
            getTooltipText->Text = Section->GraphState.TooltipText->sr;
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = Parameter1;

            if (!drawPanel)
                break;

            drawPanel->Title = PhCreateString(L"GPU");

            if (EtGpuDedicatedUsage && EtGpuDedicatedLimit) // Required for Intel IGP devices -dmex
            {
                drawPanel->SubTitle = PhFormatString(
                    L"%.2f%%\n%s / %s",
                    EtGpuNodeUsage * 100,
                    PhaFormatSize(EtGpuDedicatedUsage, ULONG_MAX)->Buffer,
                    PhaFormatSize(EtGpuDedicatedLimit, ULONG_MAX)->Buffer
                    );
                drawPanel->SubTitleOverflow = PhFormatString(
                    L"%.2f%%\n%s",
                    EtGpuNodeUsage * 100,
                    PhaFormatSize(EtGpuDedicatedUsage, ULONG_MAX)->Buffer
                    );
            }
            else
            {
                drawPanel->SubTitle = PhFormatString(
                    L"%.2f%%\n",
                    EtGpuNodeUsage * 100
                    );
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
}

VOID EtpUninitializeGpuDialog(
    VOID
    )
{
    PhDeleteGraphState(&GpuGraphState);
    PhDeleteGraphState(&DedicatedGraphState);
    PhDeleteGraphState(&SharedGraphState);
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
            GpuGraphMargin = graphItem->Margin;
            panelItem = PhAddLayoutItem(&GpuLayoutManager, GetDlgItem(hwndDlg, IDC_PANEL_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            SetWindowFont(GetDlgItem(hwndDlg, IDC_TITLE), GpuSection->Parameters->LargeFont, FALSE);
            SetWindowFont(GetDlgItem(hwndDlg, IDC_GPUNAME), GpuSection->Parameters->MediumFont, FALSE);

            PhSetDialogItemText(hwndDlg, IDC_GPUNAME, ((PPH_STRING)PH_AUTO(EtpGetGpuNameString()))->Buffer);

            GpuPanel = CreateDialog(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_SYSINFO_GPUPANEL), hwndDlg, EtpGpuPanelDialogProc);
            ShowWindow(GpuPanel, SW_SHOW);
            PhAddLayoutItemEx(&GpuLayoutManager, GpuPanel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, panelItem->Margin);

            EtpCreateGpuGraphs();
            EtpUpdateGpuGraphs();
            EtpUpdateGpuPanel();
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
        }
        break;
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

    GetClientRect(GpuDialog, &clientRect);
    GetClientRect(GetDlgItem(GpuDialog, IDC_GPU_L), &labelRect);
    graphWidth = clientRect.right - GpuGraphMargin.left - GpuGraphMargin.right;
    graphHeight = (clientRect.bottom - GpuGraphMargin.top - GpuGraphMargin.bottom - labelRect.bottom * 3 - ET_GPU_PADDING * 5) / 3;

    deferHandle = BeginDeferWindowPos(6);
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
        clientRect.bottom - GpuGraphMargin.bottom - y,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    EndDeferWindowPos(deferHandle);
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

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y;
            GpuSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorCpuKernel"), 0);

            PhGraphStateGetDrawInfo(
                &GpuGraphState,
                getDrawInfo,
                EtGpuNodeHistory.Count
                );

            if (!GpuGraphState.Valid)
            {
                PhCopyCircularBuffer_FLOAT(&EtGpuNodeHistory, GpuGraphState.Data1, drawInfo->LineDataCount);
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

                    gpu = PhGetItemCircularBuffer_FLOAT(&EtGpuNodeHistory, getTooltipText->Index);

                    PhMoveReference(&GpuGraphState.TooltipText, PhFormatString(
                        L"%.2f%%%s\n%s",
                        gpu * 100,
                        PhGetStringOrEmpty(EtpGetMaxNodeString(getTooltipText->Index)),
                        ((PPH_STRING)PH_AUTO(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                        ));
                }

                getTooltipText->Text = GpuGraphState.TooltipText->sr;
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

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y;
            GpuSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorPrivate"), 0);

            PhGraphStateGetDrawInfo(
                &DedicatedGraphState,
                getDrawInfo,
                EtGpuDedicatedHistory.Count
                );

            if (!DedicatedGraphState.Valid)
            {
                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    DedicatedGraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG64(&EtGpuDedicatedHistory, i);
                }

                if (EtGpuDedicatedLimit != 0)
                {
                    // Scale the data.
                    PhDivideSinglesBySingle(
                        DedicatedGraphState.Data1,
                        (FLOAT)EtGpuDedicatedLimit,
                        drawInfo->LineDataCount
                        );
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

                    usedPages = PhGetItemCircularBuffer_ULONG64(&EtGpuDedicatedHistory, getTooltipText->Index);

                    PhMoveReference(&DedicatedGraphState.TooltipText, PhFormatString(
                        L"Dedicated Memory: %s\n%s",
                        PhaFormatSize(usedPages, ULONG_MAX)->Buffer,
                        ((PPH_STRING)PH_AUTO(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                        ));
                }

                getTooltipText->Text = DedicatedGraphState.TooltipText->sr;
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

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y;
            GpuSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorPhysical"), 0);

            PhGraphStateGetDrawInfo(
                &SharedGraphState,
                getDrawInfo,
                EtGpuSharedHistory.Count
                );

            if (!SharedGraphState.Valid)
            {
                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    SharedGraphState.Data1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG64(&EtGpuSharedHistory, i);
                }

                if (EtGpuSharedLimit != 0)
                {
                    // Scale the data.
                    PhDivideSinglesBySingle(
                        SharedGraphState.Data1,
                        (FLOAT)EtGpuSharedLimit,
                        drawInfo->LineDataCount
                        );
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

                    usedPages = PhGetItemCircularBuffer_ULONG64(&EtGpuSharedHistory, getTooltipText->Index);

                    PhMoveReference(&SharedGraphState.TooltipText, PhFormatString(
                        L"Shared Memory: %s\n%s",
                        PhaFormatSize(usedPages, ULONG_MAX)->Buffer,
                        ((PPH_STRING)PH_AUTO(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                        ));
                }

                getTooltipText->Text = SharedGraphState.TooltipText->sr;
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
}

VOID EtpUpdateGpuPanel(
    VOID
    )
{
    PhSetDialogItemText(GpuPanel, IDC_ZDEDICATEDCURRENT_V, PhaFormatSize(EtGpuDedicatedUsage, ULONG_MAX)->Buffer);
    PhSetDialogItemText(GpuPanel, IDC_ZDEDICATEDLIMIT_V, PhaFormatSize(EtGpuDedicatedLimit, ULONG_MAX)->Buffer);
    PhSetDialogItemText(GpuPanel, IDC_ZSHAREDCURRENT_V, PhaFormatSize(EtGpuSharedUsage, ULONG_MAX)->Buffer);
    PhSetDialogItemText(GpuPanel, IDC_ZSHAREDLIMIT_V, PhaFormatSize(EtGpuSharedLimit, ULONG_MAX)->Buffer);
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
    FLOAT maxGpuUsage;
    PPH_STRING maxUsageString = NULL;

    if (maxProcessRecord = EtpReferenceMaxNodeRecord(Index))
    {
        maxGpuUsage = PhGetItemCircularBuffer_FLOAT(&EtMaxGpuNodeUsageHistory, Index);

        maxUsageString = PhaFormatString(
            L"\n%s (%lu): %.2f%%",
            maxProcessRecord->ProcessName->Buffer,
            HandleToUlong(maxProcessRecord->ProcessId),
            maxGpuUsage * 100
            );

        PhDereferenceProcessRecord(maxProcessRecord);
    }

    return maxUsageString;
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
