/*
 * Process Hacker - 
 *   system information
 * 
 * Copyright (C) 2010 wj32
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
#include <kph.h>
#include <settings.h>
#include <graph.h>
#include <windowsx.h>

#define WM_PH_SYSINFO_ACTIVATE (WM_APP + 150)
#define WM_PH_SYSINFO_UPDATE (WM_APP + 151)
#define WM_PH_SYSINFO_PANEL_UPDATE (WM_APP + 160)

NTSTATUS PhpSysInfoThreadStart(
    __in PVOID Parameter
    );

INT_PTR CALLBACK PhpSysInfoDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

INT_PTR CALLBACK PhpSysInfoPanelDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

HANDLE PhSysInfoThreadHandle = NULL;
HWND PhSysInfoWindowHandle = NULL;
HWND PhSysInfoPanelWindowHandle = NULL;
static PH_EVENT InitializedEvent = PH_EVENT_INIT;
static PH_LAYOUT_MANAGER WindowLayoutManager;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

static BOOLEAN OneGraphPerCpu;
static BOOLEAN AlwaysOnTop;

static HWND CpuGraphHandle;
static HWND IoGraphHandle;
static HWND PhysicalGraphHandle;
static HWND *CpusGraphHandle;

static PH_GRAPH_STATE CpuGraphState;
static PH_GRAPH_STATE IoGraphState;
static PH_GRAPH_STATE PhysicalGraphState;
static PPH_GRAPH_STATE CpusGraphState;

static BOOLEAN MmAddressesInitialized = FALSE;
static PSIZE_T MmSizeOfPagedPoolInBytes = NULL;
static PSIZE_T MmMaximumNonPagedPoolInBytes = NULL;

VOID PhShowSystemInformationDialog()
{
    if (!PhSysInfoWindowHandle)
    {
        if (!(PhSysInfoThreadHandle = PhCreateThread(0, PhpSysInfoThreadStart, NULL)))
        {
            PhShowStatus(PhMainWndHandle, L"Unable to create the system information window", 0, GetLastError());
            return;
        }

        PhWaitForEvent(&InitializedEvent, NULL);
    }

    SendMessage(PhSysInfoWindowHandle, WM_PH_SYSINFO_ACTIVATE, 0, 0);
}

static NTSTATUS PhpLoadMmAddresses(
    __in PVOID Parameter
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

            kernelFileName = PhCreateStringFromAnsi(kernelModules->Modules[0].FullPathName);
            newFileName = PhGetFileName(kernelFileName);
            PhDereferenceObject(kernelFileName);

            PhLoadModuleSymbolProvider(
                symbolProvider,
                newFileName->Buffer,
                (ULONG64)kernelModules->Modules[0].ImageBase,
                kernelModules->Modules[0].ImageSize
                );
            PhDereferenceObject(newFileName);

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

static NTSTATUS PhpSysInfoThreadStart(
    __in PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    BOOL result;
    MSG message;

    PhInitializeAutoPool(&autoPool);

    if (!MmAddressesInitialized && PhKphHandle)
    {
        PhQueueItemGlobalWorkQueue(PhpLoadMmAddresses, NULL);
        MmAddressesInitialized = TRUE;
    }

    PhSysInfoWindowHandle = CreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_SYSINFO),
        NULL,
        PhpSysInfoDlgProc
        );

    PhSetEvent(&InitializedEvent);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(PhSysInfoWindowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);
    PhResetEvent(&InitializedEvent);
    NtClose(PhSysInfoThreadHandle);

    PhSysInfoWindowHandle = NULL;
    PhSysInfoPanelWindowHandle = NULL;
    PhSysInfoThreadHandle = NULL;

    return STATUS_SUCCESS;
}

static VOID PhpSetAlwaysOnTop()
{
    SetWindowPos(PhSysInfoWindowHandle, AlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

static VOID PhpSetOneGraphPerCpu()
{
    ULONG i;

    PhLayoutManagerLayout(&WindowLayoutManager);

    ShowWindow(CpuGraphHandle, !OneGraphPerCpu ? SW_SHOW : SW_HIDE);

    for (i = 0; i < (ULONG)PhSystemBasicInformation.NumberOfProcessors; i++)
    {
        ShowWindow(CpusGraphHandle[i], OneGraphPerCpu ? SW_SHOW : SW_HIDE);
    }
}

static VOID NTAPI SysInfoUpdateHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PostMessage(PhSysInfoWindowHandle, WM_PH_SYSINFO_UPDATE, 0, 0);
}

static PPH_PROCESS_RECORD PhpReferenceMaxCpuRecord(
    __in LONG Index
    )
{
    LARGE_INTEGER time;
    ULONG maxProcessId;

    // Find the process record for the max. CPU process for the particular time.

    maxProcessId = PhGetItemCircularBuffer_ULONG(&PhMaxCpuHistory, Index);

    if (!maxProcessId)
        return NULL;

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
    // This mean we must add one second minus one tick (100ns) to the time, giving us 
    // 2.9999999 seconds. This will then make sure we find the process.
    PhGetStatisticsTime(NULL, Index, &time);
    time.QuadPart += PH_TICKS_PER_SEC - 1;

    return PhFindProcessRecord((HANDLE)maxProcessId, &time);
}

static PPH_STRING PhapGetMaxCpuString(
    __in LONG Index
    )
{
    PPH_PROCESS_RECORD maxProcessRecord;
#ifdef PH_RECORD_MAX_USAGE
    FLOAT maxCpuUsage;
#endif
    PPH_STRING maxUsageString = NULL;

    if (maxProcessRecord = PhpReferenceMaxCpuRecord(Index))
    {
        // We found the process record, so now we construct the max. usage string.
#ifdef PH_RECORD_MAX_USAGE
        maxCpuUsage = PhGetItemCircularBuffer_FLOAT(&PhMaxCpuUsageHistory, Index);
        maxUsageString = PhaFormatString(
            L"\n%s (%u): %.2f%%",
            maxProcessRecord->ProcessName->Buffer,
            (ULONG)maxProcessRecord->ProcessId,
            maxCpuUsage * 100
            );
#else
        maxUsageString = PhaConcatStrings2(L"\n", maxProcessRecord->ProcessName->Buffer);
#endif
        PhDereferenceProcessRecord(maxProcessRecord);
    }

    return maxUsageString;
}

static PPH_PROCESS_RECORD PhpReferenceMaxIoRecord(
    __in LONG Index
    )
{
    LARGE_INTEGER time;
    ULONG maxProcessId;

    // Find the process record for the max. I/O process for the particular time.

    maxProcessId = PhGetItemCircularBuffer_ULONG(&PhMaxIoHistory, Index);

    if (!maxProcessId)
        return NULL;

    // See above for the explanation.
    PhGetStatisticsTime(NULL, Index, &time);
    time.QuadPart += PH_TICKS_PER_SEC - 1;

    return PhFindProcessRecord((HANDLE)maxProcessId, &time);
}

static PPH_STRING PhapGetMaxIoString(
    __in LONG Index
    )
{
    PPH_PROCESS_RECORD maxProcessRecord;
#ifdef PH_RECORD_MAX_USAGE
    ULONG64 maxIoReadOther;
    ULONG64 maxIoWrite;
#endif
    PPH_STRING maxUsageString = NULL;

    if (maxProcessRecord = PhpReferenceMaxIoRecord(Index))
    {
        // We found the process record, so now we construct the max. usage string.
#ifdef PH_RECORD_MAX_USAGE
        maxIoReadOther = PhGetItemCircularBuffer_ULONG64(&PhMaxIoReadOtherHistory, Index);
        maxIoWrite = PhGetItemCircularBuffer_ULONG64(&PhMaxIoWriteHistory, Index);
        maxUsageString = PhaFormatString(
            L"\n%s (%u): R+O: %s, W: %s",
            maxProcessRecord->ProcessName->Buffer,
            (ULONG)maxProcessRecord->ProcessId,
            PhaFormatSize(maxIoReadOther, -1)->Buffer,
            PhaFormatSize(maxIoWrite, -1)->Buffer
            );
#else
        maxUsageString = PhaConcatStrings2(L"\n", maxProcessRecord->ProcessName->Buffer);
#endif
        PhDereferenceProcessRecord(maxProcessRecord);
    }

    return maxUsageString;
}

INT_PTR CALLBACK PhpSysInfoDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            ULONG processors;
            ULONG i;

            // We have already set the group boxes to have WS_EX_TRANSPARENT to fix 
            // the drawing issue that arises when using WS_CLIPCHILDREN. However 
            // in removing the flicker from the graphs the group boxes will now flicker. 
            // It's a good tradeoff since no one stares at the group boxes.
            PhSetWindowStyle(hwndDlg, WS_CLIPCHILDREN, WS_CLIPCHILDREN);

            PhInitializeLayoutManager(&WindowLayoutManager, hwndDlg);

            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_ONEGRAPHPERCPU), NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_ALWAYSONTOP), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            PhLoadWindowPlacementFromSetting(L"SysInfoWindowPosition", L"SysInfoWindowSize", hwndDlg);

            PhInitializeGraphState(&CpuGraphState);
            PhInitializeGraphState(&IoGraphState);
            PhInitializeGraphState(&PhysicalGraphState);

            CpuGraphHandle = PhCreateGraphControl(hwndDlg, IDC_CPU);
            Graph_SetTooltip(CpuGraphHandle, TRUE);
            BringWindowToTop(CpuGraphHandle);

            IoGraphHandle = PhCreateGraphControl(hwndDlg, IDC_IO);
            Graph_SetTooltip(IoGraphHandle, TRUE);
            ShowWindow(IoGraphHandle, SW_SHOW);
            BringWindowToTop(IoGraphHandle);

            PhysicalGraphHandle = PhCreateGraphControl(hwndDlg, IDC_PHYSICAL);
            Graph_SetTooltip(PhysicalGraphHandle, TRUE);
            ShowWindow(PhysicalGraphHandle, SW_SHOW);
            BringWindowToTop(PhysicalGraphHandle);

            processors = (ULONG)PhSystemBasicInformation.NumberOfProcessors;
            CpusGraphState = PhAllocate(sizeof(PH_GRAPH_STATE) * processors);
            CpusGraphHandle = PhAllocate(sizeof(HWND) * processors);

            for (i = 0; i < processors; i++)
            {
                PhInitializeGraphState(&CpusGraphState[i]);

                CpusGraphHandle[i] = PhCreateGraphControl(hwndDlg, IDC_PHYSICAL);
                Graph_SetTooltip(CpusGraphHandle[i], TRUE);
                BringWindowToTop(CpusGraphHandle[i]);
            }

            PhRegisterCallback(
                &PhProcessesUpdatedEvent,
                SysInfoUpdateHandler,
                NULL,
                &ProcessesUpdatedRegistration
                );
        }
        break;
    case WM_DESTROY:
        {
            ULONG i;

            PhUnregisterCallback(
                &PhProcessesUpdatedEvent,
                &ProcessesUpdatedRegistration
                );

            PhFree(CpusGraphHandle);

            for (i = 0; i < (ULONG)PhSystemBasicInformation.NumberOfProcessors; i++)
                PhDeleteGraphState(&CpusGraphState[i]);

            PhFree(CpusGraphState);

            PhDeleteGraphState(&CpuGraphState);
            PhDeleteGraphState(&IoGraphState);
            PhDeleteGraphState(&PhysicalGraphState);

            PhSetIntegerSetting(L"SysInfoWindowAlwaysOnTop", AlwaysOnTop);
            PhSetIntegerSetting(L"SysInfoWindowOneGraphPerCpu", OneGraphPerCpu);

            PhSaveWindowPlacementToSetting(L"SysInfoWindowPosition", L"SysInfoWindowSize", hwndDlg);

            PhDeleteLayoutManager(&WindowLayoutManager);
            PostQuitMessage(0);
        }
        break;
    case WM_SHOWWINDOW:
        {
            RECT margin;

            PhSysInfoPanelWindowHandle = CreateDialog(
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_SYSINFO_PANEL),
                PhSysInfoWindowHandle,
                PhpSysInfoPanelDlgProc
                );

            SetWindowPos(PhSysInfoPanelWindowHandle, NULL, 10, 0, 0, 0,
                SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOSIZE | SWP_NOZORDER);
            ShowWindow(PhSysInfoPanelWindowHandle, SW_SHOW);

            AlwaysOnTop = (BOOLEAN)PhGetIntegerSetting(L"SysInfoWindowAlwaysOnTop");
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ALWAYSONTOP), AlwaysOnTop ? BST_CHECKED : BST_UNCHECKED);
            PhpSetAlwaysOnTop();

            if (PhSystemBasicInformation.NumberOfProcessors != 1)
            {
                OneGraphPerCpu = (BOOLEAN)PhGetIntegerSetting(L"SysInfoWindowOneGraphPerCpu");
                Button_SetCheck(GetDlgItem(hwndDlg, IDC_ONEGRAPHPERCPU), OneGraphPerCpu ? BST_CHECKED : BST_UNCHECKED);
                PhpSetOneGraphPerCpu();
            }
            else
            {
                OneGraphPerCpu = FALSE;
                EnableWindow(GetDlgItem(hwndDlg, IDC_ONEGRAPHPERCPU), FALSE);
                PhpSetOneGraphPerCpu();
            }

            margin.left = 0;
            margin.top = 0;
            margin.right = 0;
            margin.bottom = 25;
            MapDialogRect(hwndDlg, &margin);

            PhAddLayoutItemEx(&WindowLayoutManager, PhSysInfoPanelWindowHandle, NULL, PH_ANCHOR_BOTTOM | PH_ANCHOR_LEFT, margin);

            SendMessage(hwndDlg, WM_SIZE, 0, 0);
            SendMessage(hwndDlg, WM_PH_SYSINFO_UPDATE, 0, 0);
        }
        break;
    case WM_SIZE:
        {                      
            HDWP deferHandle;
            HWND cpuGroupBox = GetDlgItem(hwndDlg, IDC_GROUPCPU);
            HWND ioGroupBox = GetDlgItem(hwndDlg, IDC_GROUPIO);
            HWND physicalGroupBox = GetDlgItem(hwndDlg, IDC_GROUPPHYSICAL);
            RECT clientRect;
            RECT panelRect;
            RECT margin = { 13, 13, 13, 13 };
            RECT innerMargin = { 10, 20, 10, 10 };
            LONG between = 3;
            LONG width;
            LONG height;

            PhLayoutManagerLayout(&WindowLayoutManager);

            CpuGraphState.Valid = FALSE;
            IoGraphState.Valid = FALSE;
            PhysicalGraphState.Valid = FALSE;

            GetClientRect(hwndDlg, &clientRect);
            // Limit the rectangle bottom to the top of the panel.
            GetWindowRect(PhSysInfoPanelWindowHandle, &panelRect);
            MapWindowPoints(NULL, hwndDlg, (POINT *)&panelRect, 2);
            clientRect.bottom = panelRect.top;

            width = clientRect.right - margin.left - margin.right;
            height = (clientRect.bottom - margin.top - margin.bottom - between * 2) / 3;

            deferHandle = BeginDeferWindowPos(6);

            deferHandle = DeferWindowPos(deferHandle, cpuGroupBox, NULL, margin.left, margin.top,
                width, height, SWP_NOACTIVATE | SWP_NOZORDER);

            if (!OneGraphPerCpu)
            {
                deferHandle = DeferWindowPos(
                    deferHandle,
                    CpuGraphHandle,
                    NULL,
                    margin.left + innerMargin.left,
                    margin.top + innerMargin.top,
                    width - innerMargin.left - innerMargin.right,
                    height - innerMargin.top - innerMargin.bottom,
                    SWP_NOACTIVATE | SWP_NOZORDER
                    );
            }
            else
            {
                ULONG i;
                ULONG processors;
                ULONG graphBetween = 5;
                ULONG graphWidth;
                ULONG x;

                processors = (ULONG)PhSystemBasicInformation.NumberOfProcessors;
                graphWidth = (width - innerMargin.left - innerMargin.right - graphBetween * (processors - 1)) / processors;
                x = margin.left + innerMargin.left;

                for (i = 0; i < processors; i++)
                {
                    if (i == processors - 1)
                    {
                        graphWidth = clientRect.right - margin.right - innerMargin.right - x;
                    }

                    deferHandle = DeferWindowPos(
                        deferHandle,
                        CpusGraphHandle[i],
                        NULL,
                        x,
                        margin.top + innerMargin.top,
                        graphWidth,
                        height - innerMargin.top - innerMargin.bottom,
                        SWP_NOACTIVATE | SWP_NOZORDER
                        );
                    x += graphWidth + graphBetween; 
                }
            }

            deferHandle = DeferWindowPos(deferHandle, ioGroupBox, NULL, margin.left, margin.top + height + between,
                width, height, SWP_NOACTIVATE | SWP_NOZORDER);
            deferHandle = DeferWindowPos(
                deferHandle,
                IoGraphHandle,
                NULL,
                margin.left + innerMargin.left,
                margin.top + height + between + innerMargin.top,
                width - innerMargin.left - innerMargin.right,
                height - innerMargin.top - innerMargin.bottom,
                SWP_NOACTIVATE | SWP_NOZORDER
                );

            deferHandle = DeferWindowPos(deferHandle, physicalGroupBox, NULL, margin.left, margin.top + (height + between) * 2,
                width, height, SWP_NOACTIVATE | SWP_NOZORDER);
            deferHandle = DeferWindowPos(
                deferHandle,
                PhysicalGraphHandle,
                NULL,
                margin.left + innerMargin.left,
                margin.top + (height + between) * 2 + innerMargin.top,
                width - innerMargin.left - innerMargin.right,
                height - innerMargin.top - innerMargin.bottom,
                SWP_NOACTIVATE | SWP_NOZORDER
                );

            EndDeferWindowPos(deferHandle);
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, 620, 590);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(hwndDlg);
                break;
            case IDC_ALWAYSONTOP:
                {
                    AlwaysOnTop = Button_GetCheck(GetDlgItem(hwndDlg, IDC_ALWAYSONTOP)) == BST_CHECKED;
                    PhpSetAlwaysOnTop();
                }
                break;
            case IDC_ONEGRAPHPERCPU:
                {
                    OneGraphPerCpu = Button_GetCheck(GetDlgItem(hwndDlg, IDC_ONEGRAPHPERCPU)) == BST_CHECKED;
                    PhpSetOneGraphPerCpu();
                    SendMessage(hwndDlg, WM_SIZE, 0, 0);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case GCN_GETDRAWINFO:
                {
                    PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)header;
                    PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;
                    ULONG i;

                    if (header->hwndFrom == CpuGraphHandle)
                    {
                        if (PhCsGraphShowText)
                        {
                            HDC hdc;

                            PhSwapReference2(&CpuGraphState.TooltipText,
                                PhFormatString(L"%.2f%% (K: %.2f%%, U: %.2f%%)",
                                (PhCpuKernelUsage + PhCpuUserUsage) * 100,
                                PhCpuKernelUsage * 100,
                                PhCpuUserUsage * 100
                                ));

                            hdc = Graph_GetBufferedContext(CpuGraphHandle);
                            SelectObject(hdc, PhApplicationFont);
                            PhSetGraphText(hdc, drawInfo, &CpuGraphState.TooltipText->sr,
                                &PhNormalGraphTextMargin, &PhNormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }

                        drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
                        drawInfo->LineColor1 = PhCsColorCpuKernel;
                        drawInfo->LineColor2 = PhCsColorCpuUser;
                        drawInfo->LineBackColor1 = PhHalveColorBrightness(PhCsColorCpuKernel);
                        drawInfo->LineBackColor2 = PhHalveColorBrightness(PhCsColorCpuUser);

                        PhGraphStateGetDrawInfo(
                            &CpuGraphState,
                            getDrawInfo,
                            PhCpuKernelHistory.Count
                            );

                        if (!CpuGraphState.Valid)
                        {
                            PhCopyCircularBuffer_FLOAT(&PhCpuKernelHistory,
                                CpuGraphState.Data1, drawInfo->LineDataCount);
                            PhCopyCircularBuffer_FLOAT(&PhCpuUserHistory,
                                CpuGraphState.Data2, drawInfo->LineDataCount);
                            CpuGraphState.Valid = TRUE;
                        }
                    }
                    else if (header->hwndFrom == IoGraphHandle)
                    {
                        if (PhCsGraphShowText)
                        {
                            HDC hdc;

                            PhSwapReference2(&IoGraphState.TooltipText,
                                PhFormatString(
                                L"R+O: %s, W: %s",
                                PhaFormatSize(PhIoReadDelta.Delta + PhIoOtherDelta.Delta, -1)->Buffer,
                                PhaFormatSize(PhIoWriteDelta.Delta, -1)->Buffer
                                ));

                            hdc = Graph_GetBufferedContext(IoGraphHandle);
                            SelectObject(hdc, PhApplicationFont);
                            PhSetGraphText(hdc, drawInfo, &IoGraphState.TooltipText->sr,
                                &PhNormalGraphTextMargin, &PhNormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }

                        drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
                        drawInfo->LineColor1 = PhCsColorIoReadOther;
                        drawInfo->LineColor2 = PhCsColorIoWrite;
                        drawInfo->LineBackColor1 = PhHalveColorBrightness(PhCsColorIoReadOther);
                        drawInfo->LineBackColor2 = PhHalveColorBrightness(PhCsColorIoWrite);

                        PhGraphStateGetDrawInfo(
                            &IoGraphState,
                            getDrawInfo,
                            PhIoReadHistory.Count
                            );

                        if (!IoGraphState.Valid)
                        {
                            ULONG i;
                            FLOAT max = 0;

                            for (i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                FLOAT data1;
                                FLOAT data2;

                                IoGraphState.Data1[i] = data1 =
                                    (FLOAT)PhGetItemCircularBuffer_ULONG64(&PhIoReadHistory, i) +
                                    (FLOAT)PhGetItemCircularBuffer_ULONG64(&PhIoOtherHistory, i);
                                IoGraphState.Data2[i] = data2 =
                                    (FLOAT)PhGetItemCircularBuffer_ULONG64(&PhIoWriteHistory, i);

                                if (max < data1 + data2)
                                    max = data1 + data2;
                            }

                            if (max != 0)
                            {
                                // Scale the data.

                                PhxfDivideSingle2U(
                                    IoGraphState.Data1,
                                    max,
                                    drawInfo->LineDataCount
                                    );
                                PhxfDivideSingle2U(
                                    IoGraphState.Data2,
                                    max,
                                    drawInfo->LineDataCount
                                    );
                            }

                            IoGraphState.Valid = TRUE;
                        }
                    }
                    else if (header->hwndFrom == PhysicalGraphHandle)
                    {
                        ULONG totalPages;
                        ULONG usedPages;

                        totalPages = PhSystemBasicInformation.NumberOfPhysicalPages;
                        usedPages = totalPages - PhPerfInformation.AvailablePages;

                        if (PhCsGraphShowText)
                        {
                            HDC hdc;

                            PhSwapReference2(&PhysicalGraphState.TooltipText,
                                PhConcatStrings2(
                                L"Physical Memory: ",
                                PhaFormatSize(UInt32x32To64(usedPages, PAGE_SIZE), -1)->Buffer
                                ));

                            hdc = Graph_GetBufferedContext(PhysicalGraphHandle);
                            SelectObject(hdc, PhApplicationFont);
                            PhSetGraphText(hdc, drawInfo, &PhysicalGraphState.TooltipText->sr,
                                &PhNormalGraphTextMargin, &PhNormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
                        }
                        else
                        {
                            drawInfo->Text.Buffer = NULL;
                        }

                        drawInfo->Flags = PH_GRAPH_USE_GRID;
                        drawInfo->LineColor1 = PhCsColorPhysical;
                        drawInfo->LineBackColor1 = PhHalveColorBrightness(PhCsColorPhysical);

                        PhGraphStateGetDrawInfo(
                            &PhysicalGraphState,
                            getDrawInfo,
                            PhPhysicalHistory.Count
                            );

                        if (!PhysicalGraphState.Valid)
                        {
                            ULONG i;

                            for (i = 0; i < drawInfo->LineDataCount; i++)
                            {
                                PhysicalGraphState.Data1[i] =
                                    (FLOAT)PhGetItemCircularBuffer_ULONG(&PhPhysicalHistory, i);
                            }

                            if (PhSystemBasicInformation.NumberOfPhysicalPages != 0)
                            {
                                // Scale the data.
                                PhxfDivideSingle2U(
                                    PhysicalGraphState.Data1,
                                    (FLOAT)totalPages,
                                    drawInfo->LineDataCount
                                    );
                            }

                            PhysicalGraphState.Valid = TRUE;
                        }
                    }
                    else if (CpusGraphHandle)
                    {
                        for (i = 0; i < (ULONG)PhSystemBasicInformation.NumberOfProcessors; i++)
                        {
                            if (header->hwndFrom != CpusGraphHandle[i])
                                continue;

                            if (PhCsGraphShowText)
                            {
                                HDC hdc;

                                PhSwapReference2(&CpusGraphState[i].TooltipText,
                                    PhFormatString(L"%.2f%% (K: %.2f%%, U: %.2f%%)",
                                    (PhCpusKernelUsage[i] + PhCpusUserUsage[i]) * 100,
                                    PhCpusKernelUsage[i] * 100,
                                    PhCpusUserUsage[i] * 100
                                    ));

                                hdc = Graph_GetBufferedContext(CpusGraphHandle[i]);
                                SelectObject(hdc, PhApplicationFont);
                                PhSetGraphText(hdc, drawInfo, &CpusGraphState[i].TooltipText->sr,
                                    &PhNormalGraphTextMargin, &PhNormalGraphTextPadding, PH_ALIGN_TOP | PH_ALIGN_LEFT);
                            }
                            else
                            {
                                drawInfo->Text.Buffer = NULL;
                            }

                            drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
                            drawInfo->LineColor1 = RGB(0x00, 0xff, 0x00);
                            drawInfo->LineColor2 = RGB(0xff, 0x00, 0x00);
                            drawInfo->LineBackColor1 = RGB(0x00, 0x77, 0x00);
                            drawInfo->LineBackColor2 = RGB(0x77, 0x00, 0x00);

                            PhGraphStateGetDrawInfo(
                                &CpusGraphState[i],
                                getDrawInfo,
                                PhCpusKernelHistory[i].Count
                                );

                            if (!CpusGraphState[i].Valid)
                            {
                                PhCopyCircularBuffer_FLOAT(&PhCpusKernelHistory[i],
                                    CpusGraphState[i].Data1, drawInfo->LineDataCount);
                                PhCopyCircularBuffer_FLOAT(&PhCpusUserHistory[i],
                                    CpusGraphState[i].Data2, drawInfo->LineDataCount);
                                CpusGraphState[i].Valid = TRUE;
                            }
                        }
                    }
                }
                break;
            case GCN_GETTOOLTIPTEXT:
                {
                    PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)lParam;
                    ULONG i;

                    if (
                        header->hwndFrom == CpuGraphHandle &&
                        getTooltipText->Index < getTooltipText->TotalCount
                        )
                    {
                        if (CpuGraphState.TooltipIndex != getTooltipText->Index)
                        {
                            FLOAT cpuKernel;
                            FLOAT cpuUser;

                            cpuKernel = PhGetItemCircularBuffer_FLOAT(&PhCpuKernelHistory, getTooltipText->Index);
                            cpuUser = PhGetItemCircularBuffer_FLOAT(&PhCpuUserHistory, getTooltipText->Index);

                            PhSwapReference2(&CpuGraphState.TooltipText, PhFormatString(
                                L"%.2f%% (K: %.2f%%, U: %.2f%%)%s\n%s",
                                (cpuKernel + cpuUser) * 100,
                                cpuKernel * 100,
                                cpuUser * 100,
                                PhGetStringOrEmpty(PhapGetMaxCpuString(getTooltipText->Index)),
                                ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                                ));
                        }

                        getTooltipText->Text = CpuGraphState.TooltipText->sr;
                    }
                    else if (
                        header->hwndFrom == IoGraphHandle &&
                        getTooltipText->Index < getTooltipText->TotalCount
                        )
                    {
                        if (IoGraphState.TooltipIndex != getTooltipText->Index)
                        {
                            ULONG64 ioRead;
                            ULONG64 ioWrite;
                            ULONG64 ioOther;

                            ioRead = PhGetItemCircularBuffer_ULONG64(&PhIoReadHistory, getTooltipText->Index);
                            ioWrite = PhGetItemCircularBuffer_ULONG64(&PhIoWriteHistory, getTooltipText->Index);
                            ioOther = PhGetItemCircularBuffer_ULONG64(&PhIoOtherHistory, getTooltipText->Index);

                            PhSwapReference2(&IoGraphState.TooltipText, PhFormatString(
                                L"R: %s\nW: %s\nO: %s%s\n%s",
                                PhaFormatSize(ioRead, -1)->Buffer,
                                PhaFormatSize(ioWrite, -1)->Buffer,
                                PhaFormatSize(ioOther, -1)->Buffer,
                                PhGetStringOrEmpty(PhapGetMaxIoString(getTooltipText->Index)),
                                ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                                ));
                        }

                        getTooltipText->Text = IoGraphState.TooltipText->sr;
                    }
                    else if (
                        header->hwndFrom == PhysicalGraphHandle &&
                        getTooltipText->Index < getTooltipText->TotalCount
                        )
                    {
                        if (PhysicalGraphState.TooltipIndex != getTooltipText->Index)
                        {
                            ULONG usedPages;

                            usedPages = PhGetItemCircularBuffer_ULONG(&PhPhysicalHistory, getTooltipText->Index);

                            PhSwapReference2(&PhysicalGraphState.TooltipText, PhFormatString(
                                L"Physical Memory: %s\n%s",
                                PhaFormatSize(UInt32x32To64(usedPages, PAGE_SIZE), -1)->Buffer,
                                ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                                ));
                        }

                        getTooltipText->Text = PhysicalGraphState.TooltipText->sr;
                    }
                    else if (
                        CpusGraphHandle &&
                        getTooltipText->Index < getTooltipText->TotalCount
                        )
                    {
                        for (i = 0; i < (ULONG)PhSystemBasicInformation.NumberOfProcessors; i++)
                        {
                            if (header->hwndFrom != CpusGraphHandle[i])
                                continue;

                            if (CpusGraphState[i].TooltipIndex != getTooltipText->Index)
                            {
                                FLOAT cpuKernel;
                                FLOAT cpuUser;

                                cpuKernel = PhGetItemCircularBuffer_FLOAT(&PhCpusKernelHistory[i], getTooltipText->Index);
                                cpuUser = PhGetItemCircularBuffer_FLOAT(&PhCpusUserHistory[i], getTooltipText->Index);

                                PhSwapReference2(&CpusGraphState[i].TooltipText, PhFormatString(
                                    L"%.2f%% (K: %.2f%%, U: %.2f%%)%s\n%s",
                                    (cpuKernel + cpuUser) * 100,
                                    cpuKernel * 100,
                                    cpuUser * 100,
                                    PhGetStringOrEmpty(PhapGetMaxCpuString(getTooltipText->Index)),
                                    ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                                    ));
                            }

                            getTooltipText->Text = CpusGraphState[i].TooltipText->sr;
                        }
                    }
                }
                break;
            case GCN_MOUSEEVENT:
                {
                    PPH_GRAPH_MOUSEEVENT mouseEvent = (PPH_GRAPH_MOUSEEVENT)lParam;
                    PPH_PROCESS_RECORD record;
                    ULONG i;

                    record = NULL;

                    if (mouseEvent->Message == WM_LBUTTONDBLCLK)
                    {
                        if (
                            header->hwndFrom == CpuGraphHandle &&
                            mouseEvent->Index < mouseEvent->TotalCount
                            )
                        {
                            record = PhpReferenceMaxCpuRecord(mouseEvent->Index);
                        }
                        else if (
                            header->hwndFrom == IoGraphHandle &&
                            mouseEvent->Index < mouseEvent->TotalCount
                            )
                        {
                            record = PhpReferenceMaxIoRecord(mouseEvent->Index);
                        }
                        else if (
                            CpusGraphHandle &&
                            mouseEvent->Index < mouseEvent->TotalCount
                            )
                        {
                            for (i = 0; i < (ULONG)PhSystemBasicInformation.NumberOfProcessors; i++)
                            {
                                if (header->hwndFrom != CpusGraphHandle[i])
                                    continue;

                                record = PhpReferenceMaxCpuRecord(mouseEvent->Index);
                            }
                        }
                    }

                    if (record)
                    {
                        PhShowProcessRecordDialog(PhSysInfoWindowHandle, record);
                        PhDereferenceProcessRecord(record);
                    }
                }
                break;
            }
        }
        break;
    case WM_PH_SYSINFO_ACTIVATE:
        {
            if (IsIconic(hwndDlg))
                ShowWindow(hwndDlg, SW_RESTORE);
            else
                ShowWindow(hwndDlg, SW_SHOW);

            SetForegroundWindow(hwndDlg);
        }
        break;
    case WM_PH_SYSINFO_UPDATE:
        {
            ULONG i;

            CpuGraphState.Valid = FALSE;
            CpuGraphState.TooltipIndex = -1;
            Graph_MoveGrid(CpuGraphHandle, 1);
            Graph_Draw(CpuGraphHandle);
            Graph_UpdateTooltip(CpuGraphHandle);
            InvalidateRect(CpuGraphHandle, NULL, FALSE);

            IoGraphState.Valid = FALSE;
            IoGraphState.TooltipIndex = -1;
            Graph_MoveGrid(IoGraphHandle, 1);
            Graph_Draw(IoGraphHandle);
            Graph_UpdateTooltip(IoGraphHandle);
            InvalidateRect(IoGraphHandle, NULL, FALSE);

            PhysicalGraphState.Valid = FALSE;
            PhysicalGraphState.TooltipIndex = -1;
            Graph_MoveGrid(PhysicalGraphHandle, 1);
            Graph_Draw(PhysicalGraphHandle);
            Graph_UpdateTooltip(PhysicalGraphHandle);
            InvalidateRect(PhysicalGraphHandle, NULL, FALSE);

            for (i = 0; i < (ULONG)PhSystemBasicInformation.NumberOfProcessors; i++)
            {
                CpusGraphState[i].Valid = FALSE;
                CpusGraphState[i].TooltipIndex = -1;
                Graph_MoveGrid(CpusGraphHandle[i], 1);
                Graph_Draw(CpusGraphHandle[i]);
                Graph_UpdateTooltip(CpusGraphHandle[i]);
                InvalidateRect(CpusGraphHandle[i], NULL, FALSE);
            }

            SendMessage(PhSysInfoPanelWindowHandle, WM_PH_SYSINFO_PANEL_UPDATE, 0, 0);
        }
        break;
    }

    return FALSE;
}

static VOID PhpGetPoolLimits(
    __out PSIZE_T Paged,
    __out PSIZE_T NonPaged
    )
{
    SIZE_T paged = 0;
    SIZE_T nonPaged = 0;

    if (MmSizeOfPagedPoolInBytes)
    {
        KphUnsafeReadVirtualMemory(
            PhKphHandle,
            NtCurrentProcess(),
            MmSizeOfPagedPoolInBytes,
            &paged,
            sizeof(SIZE_T),
            NULL
            );
    }

    if (MmMaximumNonPagedPoolInBytes)
    {
        KphUnsafeReadVirtualMemory(
            PhKphHandle,
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

INT_PTR CALLBACK PhpSysInfoPanelDlgProc(      
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {

        }
        break;
    case WM_DESTROY:
        {

        }
        break;
    case WM_PH_SYSINFO_PANEL_UPDATE:
        {
            WCHAR timeSpan[PH_TIMESPAN_STR_LEN_1] = L"Unknown";
            SYSTEM_TIMEOFDAY_INFORMATION timeOfDayInfo;
            SYSTEM_FILECACHE_INFORMATION fileCacheInfo;
            PWSTR pagedLimit;
            PWSTR nonPagedLimit;

            if (!NT_SUCCESS(NtQuerySystemInformation(
                SystemFileCacheInformation,
                &fileCacheInfo,
                sizeof(SYSTEM_FILECACHE_INFORMATION),
                NULL
                )))
            {
                memset(&fileCacheInfo, 0, sizeof(SYSTEM_FILECACHE_INFORMATION));
            }

            // System

            SetDlgItemText(hwndDlg, IDC_ZPROCESSES_V, PhaFormatUInt64(PhTotalProcesses, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZTHREADS_V, PhaFormatUInt64(PhTotalThreads, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZHANDLES_V, PhaFormatUInt64(PhTotalHandles, TRUE)->Buffer);

            if (NT_SUCCESS(NtQuerySystemInformation(
                SystemTimeOfDayInformation,
                &timeOfDayInfo,
                sizeof(SYSTEM_TIMEOFDAY_INFORMATION),
                NULL
                )))
            {
                PhPrintTimeSpan(
                    timeSpan,
                    timeOfDayInfo.CurrentTime.QuadPart - timeOfDayInfo.BootTime.QuadPart,
                    PH_TIMESPAN_DHMS
                    );
            }

            SetDlgItemText(hwndDlg, IDC_ZUPTIME_V, timeSpan);

            // Commit Charge

            SetDlgItemText(hwndDlg, IDC_ZCOMMITCURRENT_V,
                PhaFormatSize(UInt32x32To64(PhPerfInformation.CommittedPages, PAGE_SIZE), -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZCOMMITPEAK_V,
                PhaFormatSize(UInt32x32To64(PhPerfInformation.PeakCommitment, PAGE_SIZE), -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZCOMMITLIMIT_V,
                PhaFormatSize(UInt32x32To64(PhPerfInformation.CommitLimit, PAGE_SIZE), -1)->Buffer);

            // Physical Memory

            SetDlgItemText(hwndDlg, IDC_ZPHYSICALCURRENT_V,
                PhaFormatSize(UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages - PhPerfInformation.AvailablePages, PAGE_SIZE), -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZPHYSICALSYSTEMCACHE_V,
                PhaFormatSize(UInt32x32To64(fileCacheInfo.CurrentSizeIncludingTransitionInPages, PAGE_SIZE), -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZPHYSICALTOTAL_V,
                PhaFormatSize(UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages, PAGE_SIZE), -1)->Buffer);

            // Kernel Pools

            SetDlgItemText(hwndDlg, IDC_ZPAGEDPHYSICAL_V,
                PhaFormatSize(UInt32x32To64(PhPerfInformation.ResidentPagedPoolPage, PAGE_SIZE), -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZPAGEDVIRTUAL_V,
                PhaFormatSize(UInt32x32To64(PhPerfInformation.PagedPoolPages, PAGE_SIZE), -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZPAGEDALLOCS_V,
                PhaFormatUInt64(PhPerfInformation.PagedPoolAllocs, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZPAGEDFREES_V,
                PhaFormatUInt64(PhPerfInformation.PagedPoolFrees, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZNONPAGEDUSAGE_V,
                PhaFormatSize(UInt32x32To64(PhPerfInformation.NonPagedPoolPages, PAGE_SIZE), -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZNONPAGEDALLOCS_V,
                PhaFormatUInt64(PhPerfInformation.NonPagedPoolAllocs, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZNONPAGEDFREES_V,
                PhaFormatUInt64(PhPerfInformation.NonPagedPoolFrees, TRUE)->Buffer);

            if (MmAddressesInitialized && (MmSizeOfPagedPoolInBytes || MmMaximumNonPagedPoolInBytes))
            {
                SIZE_T paged;
                SIZE_T nonPaged;

                PhpGetPoolLimits(&paged, &nonPaged);
                pagedLimit = PhaFormatSize(paged, -1)->Buffer;
                nonPagedLimit = PhaFormatSize(nonPaged, -1)->Buffer;
            }
            else
            {
                if (!PhKphHandle)
                {
                    pagedLimit = nonPagedLimit = L"no driver";
                }
                else
                {
                    pagedLimit = nonPagedLimit = L"no symbols";
                }
            }

            SetDlgItemText(hwndDlg, IDC_ZPAGEDLIMIT_V, pagedLimit);
            SetDlgItemText(hwndDlg, IDC_ZNONPAGEDLIMIT_V, nonPagedLimit);

            // Page Faults

            SetDlgItemText(hwndDlg, IDC_ZPAGEFAULTSTOTAL_V,
                PhaFormatUInt64(PhPerfInformation.PageFaultCount, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZPAGEFAULTSCOPYONWRITE_V,
                PhaFormatUInt64(PhPerfInformation.CopyOnWriteCount, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZPAGEFAULTSTRANSITION_V,
                PhaFormatUInt64(PhPerfInformation.TransitionCount, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZPAGEFAULTSCACHE_V,
                PhaFormatUInt64(fileCacheInfo.PageFaultCount, TRUE)->Buffer);

            // I/O

            SetDlgItemText(hwndDlg, IDC_ZIOREADS_V,
                PhaFormatUInt64(PhPerfInformation.IoReadOperationCount, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZIOREADBYTES_V,
                PhaFormatSize(PhPerfInformation.IoReadTransferCount.QuadPart, -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZIOWRITES_V,
                PhaFormatUInt64(PhPerfInformation.IoWriteOperationCount, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZIOWRITEBYTES_V,
                PhaFormatSize(PhPerfInformation.IoWriteTransferCount.QuadPart, -1)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZIOOTHER_V,
                PhaFormatUInt64(PhPerfInformation.IoOtherOperationCount, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZIOOTHERBYTES_V,
                PhaFormatSize(PhPerfInformation.IoOtherTransferCount.QuadPart, -1)->Buffer);

            // CPU

            SetDlgItemText(hwndDlg, IDC_ZCONTEXTSWITCHES_V,
                PhaFormatUInt64(PhPerfInformation.ContextSwitches, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZINTERRUPTS_V,
                PhaFormatUInt64(PhCpuTotals.InterruptCount, TRUE)->Buffer);
            SetDlgItemText(hwndDlg, IDC_ZSYSTEMCALLS_V,
                PhaFormatUInt64(PhPerfInformation.SystemCalls, TRUE)->Buffer);
        }
        break;
    }

    return FALSE;
}
