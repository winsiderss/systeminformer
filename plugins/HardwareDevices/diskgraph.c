/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2015-2022
 *
 */

#include "devices.h"

VOID DiskDriveUpdateGraphs(
    _Inout_ PDV_DISK_SYSINFO_CONTEXT Context
    )
{
    Context->GraphState.Valid = FALSE;
    Context->GraphState.TooltipIndex = ULONG_MAX;
    Graph_MoveGrid(Context->GraphHandle, 1);
    Graph_Draw(Context->GraphHandle);
    Graph_UpdateTooltip(Context->GraphHandle);
    InvalidateRect(Context->GraphHandle, NULL, FALSE);
}

VOID DiskDriveUpdatePanel(
    _Inout_ PDV_DISK_SYSINFO_CONTEXT Context
    )
{
    PH_FORMAT format[3];
    WCHAR formatBuffer[256];

    PhInitFormatSize(&format[0], Context->DiskEntry->BytesReadDelta.Value);

    if (PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->DiskDrivePanelReadLabel, formatBuffer);
    else
        PhSetWindowText(Context->DiskDrivePanelReadLabel, PhaFormatSize(Context->DiskEntry->BytesReadDelta.Value, ULONG_MAX)->Buffer);

    PhInitFormatSize(&format[0], Context->DiskEntry->BytesWrittenDelta.Value);

    if (PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->DiskDrivePanelWriteLabel, formatBuffer);
    else
        PhSetWindowText(Context->DiskDrivePanelWriteLabel, PhaFormatSize(Context->DiskEntry->BytesWrittenDelta.Value, ULONG_MAX)->Buffer);

    PhInitFormatSize(&format[0], Context->DiskEntry->BytesReadDelta.Value + Context->DiskEntry->BytesWrittenDelta.Value);

    if (PhFormatToBuffer(format, 1, formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->DiskDrivePanelTotalLabel, formatBuffer);
    else
        PhSetWindowText(Context->DiskDrivePanelTotalLabel, PhaFormatSize(Context->DiskEntry->BytesReadDelta.Value + Context->DiskEntry->BytesWrittenDelta.Value, ULONG_MAX)->Buffer);

    PhInitFormatF(&format[0], Context->DiskEntry->ActiveTime, 0);
    PhInitFormatC(&format[1], L'%');

    if (PhFormatToBuffer(format, 2, formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->DiskDrivePanelActiveLabel, formatBuffer);
    else
        PhSetWindowText(Context->DiskDrivePanelActiveLabel, PhaFormatString(L"%.0f%%", Context->DiskEntry->ActiveTime)->Buffer);

    PhInitFormatF(&format[0], Context->DiskEntry->ResponseTime / PH_TICKS_PER_MS, 1);
    PhInitFormatS(&format[1], L" ms");

    if (PhFormatToBuffer(format, 2, formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->DiskDrivePanelTimeLabel, formatBuffer);
    else
        PhSetWindowText(Context->DiskDrivePanelTimeLabel, PhaFormatString(L"%.1f ms", Context->DiskEntry->ResponseTime / PH_TICKS_PER_MS)->Buffer);

    PhInitFormatSize(&format[0], Context->DiskEntry->BytesReadDelta.Delta + Context->DiskEntry->BytesWrittenDelta.Delta);
    PhInitFormatS(&format[1], L"/s");

    if (PhFormatToBuffer(format, 2, formatBuffer, sizeof(formatBuffer), NULL))
        PhSetWindowText(Context->DiskDrivePanelBytesLabel, formatBuffer);
    else
        PhSetWindowText(Context->DiskDrivePanelBytesLabel, PhaFormatString(L"%s/s", PhaFormatSize(Context->DiskEntry->BytesReadDelta.Delta + Context->DiskEntry->BytesWrittenDelta.Delta, ULONG_MAX)->Buffer)->Buffer);

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

VOID DiskDriveUpdateTitle(
    _Inout_ PDV_DISK_SYSINFO_CONTEXT Context
    )
{
    // The disk letters can change so update the value.
    PhSetWindowText(Context->DiskPathLabel, PhGetStringOrDefault(Context->DiskEntry->DiskIndexName, L"Unknown disk"));
    PhSetWindowText(Context->DiskNameLabel, PhGetStringOrDefault(Context->DiskEntry->DiskName, L"Unknown disk"));  // TODO: We only need to set the name once. (dmex)
}

VOID UpdateDiskIndexText(
    _Inout_ PDV_DISK_SYSINFO_CONTEXT Context
    )
{
    // If our delayed lookup of the disk name, index and type hasn't fired then query the information now.
    DiskDriveUpdateDeviceInfo(NULL, Context->DiskEntry);

    // TODO: Move into DiskDriveUpdateDeviceInfo.
    if (Context->DiskEntry->DiskIndex != ULONG_MAX && !Context->DiskEntry->DiskIndexName)
    {
        // Query the disk DosDevices mount points.
        PPH_STRING diskMountPoints = PH_AUTO_T(PH_STRING, DiskDriveQueryDosMountPoints(Context->DiskEntry->DiskIndex));

        if (!PhIsNullOrEmptyString(diskMountPoints))
        {
            PH_FORMAT format[5];

            // Disk %lu (%s)
            PhInitFormatS(&format[0], L"Disk ");
            PhInitFormatU(&format[1], Context->DiskEntry->DiskIndex);
            PhInitFormatS(&format[2], L" (");
            PhInitFormatSR(&format[3], diskMountPoints->sr);
            PhInitFormatC(&format[4], L')');

            PhMoveReference(&Context->DiskEntry->DiskIndexName, PhFormat(format, RTL_NUMBER_OF(format), 0));
        }
        else
        {
            PH_FORMAT format[2];

            // Disk %lu
            PhInitFormatS(&format[0], L"Disk ");
            PhInitFormatU(&format[1], Context->DiskEntry->DiskIndex);

            PhMoveReference(&Context->DiskEntry->DiskIndexName, PhFormat(format, RTL_NUMBER_OF(format), 0));
        }
    }
}

INT_PTR CALLBACK DiskDrivePanelDialogProc(
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
            context->DiskDrivePanelReadLabel = GetDlgItem(hwndDlg, IDC_STAT_BREAD);
            context->DiskDrivePanelWriteLabel = GetDlgItem(hwndDlg, IDC_STAT_BWRITE);
            context->DiskDrivePanelTotalLabel = GetDlgItem(hwndDlg, IDC_STAT_BTOTAL);
            context->DiskDrivePanelActiveLabel = GetDlgItem(hwndDlg, IDC_STAT_ACTIVE);
            context->DiskDrivePanelTimeLabel = GetDlgItem(hwndDlg, IDC_STAT_RESPONSETIME);
            context->DiskDrivePanelBytesLabel = GetDlgItem(hwndDlg, IDC_STAT_BYTESDELTA);
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_DETAILS:
                ShowDiskDriveDetailsDialog(context);
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

VOID DiskDriveCreateGraph(
    _Inout_ PDV_DISK_SYSINFO_CONTEXT Context
    )
{
    Context->GraphHandle = CreateWindow(
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
    Graph_SetTooltip(Context->GraphHandle, TRUE);

    PhAddLayoutItemEx(&Context->LayoutManager, Context->GraphHandle, NULL, PH_ANCHOR_ALL, Context->GraphMargin);
}

VOID DiskDriveTickDialog(
    _Inout_ PDV_DISK_SYSINFO_CONTEXT Context
    )
{
    DiskDriveUpdateTitle(Context);
    DiskDriveUpdateGraphs(Context);
    DiskDriveUpdatePanel(Context);
}

INT_PTR CALLBACK DiskDriveDialogProc(
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

        if (uMsg == WM_DESTROY)
        {
            PhDeleteLayoutManager(&context->LayoutManager);
            PhDeleteGraphState(&context->GraphState);

            if (context->GraphHandle)
                DestroyWindow(context->GraphHandle);

            if (context->PanelWindowHandle)
                DestroyWindow(context->PanelWindowHandle);

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
            LONG dpiValue;

            dpiValue = PhGetWindowDpi(hwndDlg);

            context->WindowHandle = hwndDlg;
            context->DiskPathLabel = GetDlgItem(hwndDlg, IDC_DISKMOUNTPATH);
            context->DiskNameLabel = GetDlgItem(hwndDlg, IDC_DISKNAME);

            PhInitializeGraphState(&context->GraphState);
            PhInitializeLayoutManager(&context->LayoutManager, hwndDlg);

            PhAddLayoutItem(&context->LayoutManager, context->DiskPathLabel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            PhAddLayoutItem(&context->LayoutManager, context->DiskNameLabel, NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_TOP | PH_LAYOUT_FORCE_INVALIDATE);
            graphItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&context->LayoutManager, GetDlgItem(hwndDlg, IDC_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            context->GraphMargin = graphItem->Margin;
            PhGetSizeDpiValue(&context->GraphMargin, dpiValue, TRUE);

            SetWindowFont(context->DiskPathLabel, context->SysinfoSection->Parameters->LargeFont, FALSE);
            SetWindowFont(context->DiskNameLabel, context->SysinfoSection->Parameters->MediumFont, FALSE);

            context->PanelWindowHandle = PhCreateDialog(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_DISKDRIVE_PANEL), hwndDlg, DiskDrivePanelDialogProc, context);
            ShowWindow(context->PanelWindowHandle, SW_SHOW);
            PhAddLayoutItemEx(&context->LayoutManager, context->PanelWindowHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, panelItem->Margin);

            DiskDriveUpdateTitle(context);
            DiskDriveCreateGraph(context);
            DiskDriveUpdateGraphs(context);
            DiskDriveUpdatePanel(context);
        }
        break;
    case WM_SIZE:
        {
            context->GraphState.Valid = FALSE;
            context->GraphState.TooltipIndex = ULONG_MAX;

            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            NMHDR* header = (NMHDR*)lParam;

            if (header->hwndFrom == context->GraphHandle)
            {
                switch (header->code)
                {
                case GCN_GETDRAWINFO:
                    {
                        PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)header;
                        PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;
                        LONG dpiValue;

                        dpiValue = PhGetWindowDpi(header->hwndFrom);

                        drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y | PH_GRAPH_USE_LINE_2;
                        context->SysinfoSection->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), PhGetIntegerSetting(L"ColorIoWrite"), dpiValue);

                        PhGraphStateGetDrawInfo(
                            &context->GraphState,
                            getDrawInfo,
                            context->DiskEntry->ReadBuffer.Count
                            );

                        if (!context->GraphState.Valid)
                        {
                            FLOAT max = 1024 * 1024; // minimum scaling of 1 MB.

                            for (ULONG i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data1;
                                FLOAT data2;

                                context->GraphState.Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->DiskEntry->ReadBuffer, i);
                                context->GraphState.Data2[i] = data2 = (FLOAT)PhGetItemCircularBuffer_ULONG64(&context->DiskEntry->WriteBuffer, i);

                                if (max < data1 + data2)
                                    max = data1 + data2;
                            }

                            if (max != 0)
                            {
                                // Scale the data.
                                PhDivideSinglesBySingle(
                                    context->GraphState.Data1,
                                    max,
                                    drawInfo->LineDataCount
                                    );

                                // Scale the data.
                                PhDivideSinglesBySingle(
                                    context->GraphState.Data2,
                                    max,
                                    drawInfo->LineDataCount
                                    );
                            }

                            drawInfo->LabelYFunction = PhSiSizeLabelYFunction;
                            drawInfo->LabelYFunctionParameter = max;

                            context->GraphState.Valid = TRUE;
                        }
                    }
                    break;
                case GCN_GETTOOLTIPTEXT:
                    {
                        PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)header;

                        if (getTooltipText->Index < getTooltipText->TotalCount)
                        {
                            if (context->GraphState.TooltipIndex != getTooltipText->Index)
                            {
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

                                PhMoveReference(&context->GraphState.TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 0));
                            }

                            getTooltipText->Text = PhGetStringRef(context->GraphState.TooltipText);
                        }
                    }
                    break;
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

BOOLEAN DiskDriveSectionCallback(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PH_SYSINFO_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    PDV_DISK_SYSINFO_CONTEXT context = (PDV_DISK_SYSINFO_CONTEXT)Section->Context;

    switch (Message)
    {
    case SysInfoCreate:
        {
            UpdateDiskIndexText(context);
        }
        return TRUE;
    case SysInfoDestroy:
        {
            PhDereferenceObject(context->DiskEntry);
            PhDereferenceObject(context->SectionName);
            PhFree(context);
        }
        return TRUE;
    case SysInfoTick:
        {
            UpdateDiskIndexText(context);

            if (context->WindowHandle)
            {
                DiskDriveTickDialog(context);
            }
        }
        return TRUE;
    case SysInfoViewChanging:
        {
            PH_SYSINFO_VIEW_TYPE view = (PH_SYSINFO_VIEW_TYPE)Parameter1;
            PPH_SYSINFO_SECTION section = (PPH_SYSINFO_SECTION)Parameter2;

            if (view == SysInfoSummaryView || section != Section)
                return TRUE;

            if (context->GraphHandle)
            {
                context->GraphState.Valid = FALSE;
                context->GraphState.TooltipIndex = ULONG_MAX;
                Graph_Draw(context->GraphHandle);
            }
        }
        return TRUE;
    case SysInfoCreateDialog:
        {
            PPH_SYSINFO_CREATE_DIALOG createDialog = (PPH_SYSINFO_CREATE_DIALOG)Parameter1;

            if (!createDialog)
                break;

            createDialog->Instance = PluginInstance->DllBase;
            createDialog->Template = MAKEINTRESOURCE(IDD_DISKDRIVE_DIALOG);
            createDialog->DialogProc = DiskDriveDialogProc;
            createDialog->Parameter = context;
        }
        return TRUE;
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = (PPH_GRAPH_DRAW_INFO)Parameter1;
            LONG dpiValue;

            if (!drawInfo)
                break;

            dpiValue = PhGetWindowDpi(Section->GraphHandle);
            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y | PH_GRAPH_USE_LINE_2;
            Section->Parameters->ColorSetupFunction(drawInfo, PhGetIntegerSetting(L"ColorIoReadOther"), PhGetIntegerSetting(L"ColorIoWrite"), dpiValue);
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

            if (!getTooltipText)
                break;

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

            if (!drawPanel)
                break;

            PhSetReference(&drawPanel->Title, context->DiskEntry->DiskIndexName);
            if (!drawPanel->Title) drawPanel->Title = PhCreateString(L"Unknown disk");

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

VOID DiskDriveSysInfoInitializing(
    _In_ PPH_PLUGIN_SYSINFO_POINTERS Pointers,
    _In_ _Assume_refs_(1) PDV_DISK_ENTRY DiskEntry
    )
{
    static PH_STRINGREF text = PH_STRINGREF_INIT(L"Disk ");
    PDV_DISK_SYSINFO_CONTEXT context;
    PH_SYSINFO_SECTION section;

    context = PhAllocateZero(sizeof(DV_DISK_SYSINFO_CONTEXT));
    context->SectionName = PhConcatStringRef2(&text, &DiskEntry->Id.DevicePath->sr);
    context->DiskEntry = PhReferenceObject(DiskEntry);

    memset(&section, 0, sizeof(PH_SYSINFO_SECTION));
    section.Context = context;
    section.Callback = DiskDriveSectionCallback;
    section.Name = PhGetStringRef(context->SectionName);

    context->SysinfoSection = Pointers->CreateSection(&section);
}
