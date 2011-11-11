/*
 * Process Hacker -
 *   system information window
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

#include <phapp.h>
#include <kphuser.h>
#include <settings.h>
#include <windowsx.h>
#include <sysinfop.h>

#define WM_PH_SYSINFO_ACTIVATE (WM_APP + 150)
#define WM_PH_SYSINFO_UPDATE (WM_APP + 151)

NTSTATUS PhpSysInfoThreadStart(
    __in PVOID Parameter
    );

INT_PTR CALLBACK PhpSysInfoDlgProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

BOOLEAN PhpCpuSectionCallback(
    __in PPH_SYSINFO_SECTION Section,
    __in HWND DialogWindowHandle,
    __in PH_SYSINFO_SECTION_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2
    );

BOOLEAN PhpMemorySectionCallback(
    __in PPH_SYSINFO_SECTION Section,
    __in HWND DialogWindowHandle,
    __in PH_SYSINFO_SECTION_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2
    );

BOOLEAN PhpIoSectionCallback(
    __in PPH_SYSINFO_SECTION Section,
    __in HWND DialogWindowHandle,
    __in PH_SYSINFO_SECTION_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2
    );

HANDLE PhSysInfoThreadHandle = NULL;
HWND PhSysInfoWindowHandle = NULL;
static PH_EVENT InitializedEvent = PH_EVENT_INIT;
static PH_LAYOUT_MANAGER WindowLayoutManager;
static RECT MinimumSize;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

static PH_SYSINFO_VIEW_TYPE CurrentView;
static PH_SYSINFO_PARAMETERS CurrentParameters;
static PPH_LIST SectionList;

static PPH_SYSINFO_SECTION CpuSection;
static PPH_SYSINFO_SECTION MemorySection;
static PPH_SYSINFO_SECTION IoSection;

static BOOLEAN AlwaysOnTop;

VOID PhShowSystemInformationDialog(
    __in_opt PWSTR SectionName
    )
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

static NTSTATUS PhpSysInfoThreadStart(
    __in PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    BOOL result;
    MSG message;

    PhInitializeAutoPool(&autoPool);

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
    PhSysInfoThreadHandle = NULL;

    return STATUS_SUCCESS;
}

static VOID PhpSetAlwaysOnTop(
    VOID
    )
{
    SetWindowPos(PhSysInfoWindowHandle, AlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

static VOID NTAPI SysInfoUpdateHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PostMessage(PhSysInfoWindowHandle, WM_PH_SYSINFO_UPDATE, 0, 0);
}

static VOID PhpColorSetupFunction(
    __out PPH_GRAPH_DRAW_INFO DrawInfo,
    __in COLORREF Color1,
    __in COLORREF Color2
    )
{
    DrawInfo->LineColor1 = PhHalveColorBrightness(Color1);
    DrawInfo->LineBackColor1 = PhMakeColorBrighter(Color1, 125);
    DrawInfo->LineColor2 = PhHalveColorBrightness(Color2);
    DrawInfo->LineBackColor2 = PhMakeColorBrighter(Color2, 125);
}

static VOID PhpInitializeParameters(
    __in HWND WindowHandle
    )
{
    LOGFONT logFont;
    HDC hdc;
    TEXTMETRIC textMetrics;
    HFONT originalFont;

    memset(&CurrentParameters, 0, sizeof(PH_SYSINFO_PARAMETERS));

    if (SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &logFont, 0))
    {
        CurrentParameters.Font = CreateFontIndirect(&logFont);
    }
    else
    {
        CurrentParameters.Font = PhApplicationFont;
        GetObject(PhApplicationFont, sizeof(LOGFONT), &logFont);
    }

    hdc = GetDC(WindowHandle);

    logFont.lfHeight -= MulDiv(4, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    CurrentParameters.MediumFont = CreateFontIndirect(&logFont);

    logFont.lfHeight -= MulDiv(4, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    CurrentParameters.LargeFont = CreateFontIndirect(&logFont);

    CurrentParameters.GraphBackColor = RGB(0xef, 0xef, 0xef);
    CurrentParameters.PanelForeColor = RGB(0x00, 0x00, 0x00);

    CurrentParameters.ColorSetupFunction = PhpColorSetupFunction;

    originalFont = SelectObject(hdc, CurrentParameters.Font);
    GetTextMetrics(hdc, &textMetrics);
    CurrentParameters.FontHeight = textMetrics.tmHeight;

    SelectObject(hdc, CurrentParameters.MediumFont);
    GetTextMetrics(hdc, &textMetrics);
    CurrentParameters.MediumFontHeight = textMetrics.tmHeight;

    SelectObject(hdc, originalFont);

    CurrentParameters.MinimumGraphHeight =
        PH_SYSINFO_PANEL_PADDING +
        CurrentParameters.MediumFontHeight +
        PH_SYSINFO_PANEL_PADDING;

    CurrentParameters.SectionViewGraphHeight =
        PH_SYSINFO_PANEL_PADDING +
        CurrentParameters.MediumFontHeight +
        PH_SYSINFO_PANEL_PADDING +
        CurrentParameters.FontHeight +
        2 +
        CurrentParameters.FontHeight +
        PH_SYSINFO_PANEL_PADDING +
        2;

    ReleaseDC(WindowHandle, hdc);
}

static VOID PhpDeleteParameters(
    VOID
    )
{
    if (CurrentParameters.Font)
        DeleteObject(CurrentParameters.Font);
    if (CurrentParameters.MediumFont)
        DeleteObject(CurrentParameters.MediumFont);
    if (CurrentParameters.LargeFont)
        DeleteObject(CurrentParameters.LargeFont);
}

static PPH_SYSINFO_SECTION PhpCreateSection(
    __in PPH_SYSINFO_SECTION Template
    )
{
    PPH_SYSINFO_SECTION section;
    PH_GRAPH_OPTIONS options;

    section = PhAllocate(sizeof(PH_SYSINFO_SECTION));
    memset(section, 0, sizeof(PH_SYSINFO_SECTION));

    section->Name = Template->Name;
    section->Flags = Template->Flags;
    section->Callback = Template->Callback;

    section->GraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_BORDER | GC_STYLE_FADEOUT | GC_STYLE_DRAW_PANEL,
        0,
        0,
        3,
        3,
        PhSysInfoWindowHandle,
        (HMENU)(IDDYNAMIC + 1 + SectionList->Count),
        PhInstanceHandle,
        NULL
        );
    PhInitializeGraphState(&section->GraphState);
    section->Parameters = &CurrentParameters;

    Graph_GetOptions(section->GraphHandle, &options);
    options.FadeOutWidth = PH_SYSINFO_FADE_WIDTH;
    options.DefaultCursor = LoadCursor(NULL, IDC_HAND);
    Graph_SetOptions(section->GraphHandle, &options);
    Graph_SetTooltip(section->GraphHandle, TRUE);

    PhAddItemList(SectionList, section);

    return section;
}

static VOID PhpDestroySection(
    __in PPH_SYSINFO_SECTION Section
    )
{
    PhDeleteGraphState(&Section->GraphState);
    PhFree(Section);
}

static PPH_SYSINFO_SECTION PhpCreateInternalSection(
    __in PWSTR Name,
    __in ULONG Flags,
    __in PPH_SYSINFO_SECTION_CALLBACK Callback
    )
{
    PH_SYSINFO_SECTION section;

    memset(&section, 0, sizeof(PH_SYSINFO_SECTION));
    section.Name = Name;
    section.Flags = Flags;
    section.Callback = Callback;

    return PhpCreateSection(&section);
}

static VOID PhpDefaultDrawPanel(
    __in PPH_SYSINFO_SECTION Section,
    __in PPH_SYSINFO_DRAW_PANEL DrawPanel
    )
{
    HDC hdc;
    RECT rect;

    hdc = DrawPanel->hdc;

    SetTextColor(hdc, CurrentParameters.PanelForeColor);
    SetBkMode(hdc, TRANSPARENT);

    rect.left = PH_SYSINFO_PANEL_PADDING;
    rect.top = PH_SYSINFO_PANEL_PADDING;
    rect.right = PH_SYSINFO_FADE_WIDTH;
    rect.bottom = DrawPanel->Rect.bottom;

    if (DrawPanel->Title)
    {
        SelectObject(hdc, CurrentParameters.MediumFont);
        DrawText(hdc, DrawPanel->Title->Buffer, DrawPanel->Title->Length / 2, &rect, DT_NOPREFIX | DT_SINGLELINE);
    }

    if (DrawPanel->SubTitle)
    {
        rect.top += CurrentParameters.MediumFontHeight + PH_SYSINFO_PANEL_PADDING;
        SelectObject(hdc, CurrentParameters.Font);
        DrawText(hdc, DrawPanel->SubTitle->Buffer, DrawPanel->SubTitle->Length / 2, &rect, DT_NOPREFIX);
    }
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
            PhSetWindowStyle(hwndDlg, WS_CLIPCHILDREN, WS_CLIPCHILDREN);

            PhInitializeLayoutManager(&WindowLayoutManager, hwndDlg);

            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDC_ALWAYSONTOP), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(hwndDlg, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            PhRegisterCallback(
                &PhProcessesUpdatedEvent,
                SysInfoUpdateHandler,
                NULL,
                &ProcessesUpdatedRegistration
                );

            PhLoadWindowPlacementFromSetting(L"SysInfoWindowPosition", L"SysInfoWindowSize", hwndDlg);
        }
        break;
    case WM_DESTROY:
        {
            ULONG i;
            PPH_SYSINFO_SECTION section;

            PhUnregisterCallback(
                &PhProcessesUpdatedEvent,
                &ProcessesUpdatedRegistration
                );

            PhSetIntegerSetting(L"SysInfoWindowAlwaysOnTop", AlwaysOnTop);

            PhSaveWindowPlacementToSetting(L"SysInfoWindowPosition", L"SysInfoWindowSize", hwndDlg);

            for (i = 0; i < SectionList->Count; i++)
            {
                section = SectionList->Items[i];
                PhpDestroySection(section);
            }

            PhDereferenceObject(SectionList);
            SectionList = NULL;
            PhpDeleteParameters();

            PhDeleteLayoutManager(&WindowLayoutManager);
            PostQuitMessage(0);
        }
        break;
    case WM_SHOWWINDOW:
        {
            RECT buttonRect;
            RECT clientRect;

            SectionList = PhCreateList(8);
            PhpInitializeParameters(hwndDlg);

            CpuSection = PhpCreateInternalSection(L"CPU", 0, PhpCpuSectionCallback);
            MemorySection = PhpCreateInternalSection(L"Memory", 0, PhpMemorySectionCallback);
            IoSection = PhpCreateInternalSection(L"I/O", 0, PhpIoSectionCallback);

            GetWindowRect(GetDlgItem(hwndDlg, IDOK), &buttonRect);
            MapWindowPoints(NULL, hwndDlg, (POINT *)&buttonRect, 2);
            GetClientRect(hwndDlg, &clientRect);

            MinimumSize.left = 0;
            MinimumSize.top = 0;
            MinimumSize.right = 400;
            MinimumSize.bottom = 200;

            if (SectionList->Count != 0)
            {
                MinimumSize.bottom =
                    GetSystemMetrics(SM_CYCAPTION) +
                    PH_SYSINFO_WINDOW_PADDING +
                    CurrentParameters.SectionViewGraphHeight * SectionList->Count +
                    PH_SYSINFO_GRAPH_PADDING * (SectionList->Count - 1) +
                    PH_SYSINFO_WINDOW_PADDING +
                    clientRect.bottom - buttonRect.top +
                    GetSystemMetrics(SM_CYSIZEFRAME) * 2;
            }

            AlwaysOnTop = (BOOLEAN)PhGetIntegerSetting(L"SysInfoWindowAlwaysOnTop");
            Button_SetCheck(GetDlgItem(hwndDlg, IDC_ALWAYSONTOP), AlwaysOnTop ? BST_CHECKED : BST_UNCHECKED);
            PhpSetAlwaysOnTop();

            SendMessage(hwndDlg, WM_SIZE, 0, 0);
            SendMessage(hwndDlg, WM_PH_SYSINFO_UPDATE, 0, 0);
        }
        break;
    case WM_SIZE:
        {
            RECT buttonRect;
            RECT clientRect;
            ULONG availableHeight;
            ULONG availableWidth;
            ULONG graphHeight;
            ULONG i;
            PPH_SYSINFO_SECTION section;
            HDWP deferHandle;
            ULONG y;

            PhLayoutManagerLayout(&WindowLayoutManager);

            if (SectionList && SectionList->Count != 0)
            {
                GetWindowRect(GetDlgItem(hwndDlg, IDOK), &buttonRect);
                MapWindowPoints(NULL, hwndDlg, (POINT *)&buttonRect, 2);
                GetClientRect(hwndDlg, &clientRect);

                availableHeight = buttonRect.top - PH_SYSINFO_WINDOW_PADDING * 2;
                availableWidth = clientRect.right - PH_SYSINFO_WINDOW_PADDING * 2;
                graphHeight = (availableHeight - PH_SYSINFO_GRAPH_PADDING * (SectionList->Count - 1)) / SectionList->Count;

                deferHandle = BeginDeferWindowPos(SectionList->Count);
                y = PH_SYSINFO_WINDOW_PADDING;

                for (i = 0; i < SectionList->Count; i++)
                {
                    section = SectionList->Items[i];

                    deferHandle = DeferWindowPos(
                        deferHandle,
                        section->GraphHandle,
                        NULL,
                        PH_SYSINFO_WINDOW_PADDING,
                        y,
                        availableWidth,
                        graphHeight,
                        SWP_NOACTIVATE | SWP_NOZORDER
                        );
                    y += graphHeight + PH_SYSINFO_GRAPH_PADDING;

                    section->GraphState.Valid = FALSE;
                }

                EndDeferWindowPos(deferHandle);
            }
        }
        break;
    case WM_SIZING:
        {
            PhResizingMinimumSize((PRECT)lParam, wParam, MinimumSize.right, MinimumSize.bottom);
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
                    PPH_SYSINFO_SECTION section;

                    for (i = 0; i < SectionList->Count; i++)
                    {
                        section = SectionList->Items[i];

                        if (getDrawInfo->Header.hwndFrom == section->GraphHandle)
                        {
                            section->Callback(section, hwndDlg, SysInfoGraphGetDrawInfo, drawInfo, 0);

                            if (CurrentView == SysInfoSectionView)
                                drawInfo->Flags &= ~PH_GRAPH_USE_GRID;

                            break;
                        }
                    }
                }
                break;
            case GCN_GETTOOLTIPTEXT:
                {
                    PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)lParam;
                    ULONG i;
                    PPH_SYSINFO_SECTION section;

                    if (getTooltipText->Index < getTooltipText->TotalCount)
                    {
                        for (i = 0; i < SectionList->Count; i++)
                        {
                            section = SectionList->Items[i];

                            if (getTooltipText->Header.hwndFrom == section->GraphHandle)
                            {
                                PH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT graphGetTooltipText;

                                graphGetTooltipText.Index = getTooltipText->Index;
                                PhInitializeEmptyStringRef(&graphGetTooltipText.Text);

                                section->Callback(section, hwndDlg, SysInfoGraphGetTooltipText, &graphGetTooltipText, 0);

                                getTooltipText->Text = graphGetTooltipText.Text;

                                break;
                            }
                        }
                    }
                }
                break;
            case GCN_DRAWPANEL:
                {
                    PPH_GRAPH_DRAWPANEL drawPanel = (PPH_GRAPH_DRAWPANEL)lParam;
                    ULONG i;
                    PPH_SYSINFO_SECTION section;

                    for (i = 0; i < SectionList->Count; i++)
                    {
                        section = SectionList->Items[i];

                        if (drawPanel->Header.hwndFrom == section->GraphHandle)
                        {
                            PH_SYSINFO_DRAW_PANEL sysInfoDrawPanel;

                            sysInfoDrawPanel.hdc = drawPanel->hdc;
                            sysInfoDrawPanel.Rect = drawPanel->Rect;
                            sysInfoDrawPanel.Rect.right = PH_SYSINFO_FADE_WIDTH;
                            sysInfoDrawPanel.CustomDraw = FALSE;

                            sysInfoDrawPanel.Title = NULL;
                            sysInfoDrawPanel.SubTitle = NULL;

                            section->Callback(section, hwndDlg, SysInfoGraphDrawPanel, &sysInfoDrawPanel, NULL);

                            if (!sysInfoDrawPanel.CustomDraw)
                                PhpDefaultDrawPanel(section, &sysInfoDrawPanel);

                            PhSwapReference(&sysInfoDrawPanel.Title, NULL);
                            PhSwapReference(&sysInfoDrawPanel.SubTitle, NULL);

                            break;
                        }
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
            PPH_SYSINFO_SECTION section;

            for (i = 0; i < SectionList->Count; i++)
            {
                section = SectionList->Items[i];

                section->Callback(section, hwndDlg, SysInfoTick, NULL, NULL);

                section->GraphState.Valid = FALSE;
                section->GraphState.TooltipIndex = -1;
                Graph_MoveGrid(section->GraphHandle, 1);
                Graph_Draw(section->GraphHandle);
                Graph_UpdateTooltip(section->GraphHandle);
                InvalidateRect(section->GraphHandle, NULL, FALSE);
            }
        }
        break;
    }

    return FALSE;
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

    return PhFindProcessRecord(LongToHandle(maxProcessId), &time);
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

        // Make sure we don't try to display the PID of DPCs or Interrupts.
        if (!PH_IS_FAKE_PROCESS_ID(maxProcessRecord->ProcessId))
        {
            maxUsageString = PhaFormatString(
                L"\n%s (%u): %.2f%%",
                maxProcessRecord->ProcessName->Buffer,
                (ULONG)maxProcessRecord->ProcessId,
                maxCpuUsage * 100
                );
        }
        else
        {
            maxUsageString = PhaFormatString(
                L"\n%s: %.2f%%",
                maxProcessRecord->ProcessName->Buffer,
                maxCpuUsage * 100
                );
        }
#else
        maxUsageString = PhaConcatStrings2(L"\n", maxProcessRecord->ProcessName->Buffer);
#endif

        PhDereferenceProcessRecord(maxProcessRecord);
    }

    return maxUsageString;
}

static BOOLEAN PhpCpuSectionCallback(
    __in PPH_SYSINFO_SECTION Section,
    __in HWND DialogWindowHandle,
    __in PH_SYSINFO_SECTION_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2
    )
{
    switch (Message)
    {
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = Parameter1;

            drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
            Section->Parameters->ColorSetupFunction(drawInfo, PhCsColorCpuKernel, PhCsColorCpuUser);
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, PhCpuKernelHistory.Count);

            if (!Section->GraphState.Valid)
            {
                PhCopyCircularBuffer_FLOAT(&PhCpuKernelHistory, Section->GraphState.Data1, drawInfo->LineDataCount);
                PhCopyCircularBuffer_FLOAT(&PhCpuUserHistory, Section->GraphState.Data2, drawInfo->LineDataCount);
                Section->GraphState.Valid = TRUE;
            }
        }
        break;
    case SysInfoGraphGetTooltipText:
        {
            PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT getTooltipText = Parameter1;
            FLOAT cpuKernel;
            FLOAT cpuUser;

            cpuKernel = PhGetItemCircularBuffer_FLOAT(&PhCpuKernelHistory, getTooltipText->Index);
            cpuUser = PhGetItemCircularBuffer_FLOAT(&PhCpuUserHistory, getTooltipText->Index);

            PhSwapReference2(&Section->GraphState.TooltipText, PhFormatString(
                L"%.2f%%%s\n%s",
                (cpuKernel + cpuUser) * 100,
                PhGetStringOrEmpty(PhapGetMaxCpuString(getTooltipText->Index)),
                ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                ));
            getTooltipText->Text = Section->GraphState.TooltipText->sr;
        }
        break;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = Parameter1;

            drawPanel->Title = PhCreateString(L"CPU");
            drawPanel->SubTitle = PhFormatString(L"%.2f%%", (PhCpuKernelUsage + PhCpuUserUsage) * 100);
        }
        break;
    }

    return FALSE;
}

static PPH_STRING PhapFormatSizeWithPrecision(
    __in ULONG64 Size,
    __in USHORT Precision
    )
{
    PH_FORMAT format;

    format.Type = SizeFormatType | FormatUsePrecision;
    format.Precision = Precision;
    format.u.Size = Size;

    return PHA_DEREFERENCE(PhFormat(&format, 1, 0));
}

static BOOLEAN PhpMemorySectionCallback(
    __in PPH_SYSINFO_SECTION Section,
    __in HWND DialogWindowHandle,
    __in PH_SYSINFO_SECTION_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2
    )
{
    switch (Message)
    {
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = Parameter1;
            ULONG i;

            drawInfo->Flags = PH_GRAPH_USE_GRID;
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
                    PhxfDivideSingle2U(
                        Section->GraphState.Data1,
                        (FLOAT)PhSystemBasicInformation.NumberOfPhysicalPages,
                        drawInfo->LineDataCount
                        );
                }

                Section->GraphState.Valid = TRUE;
            }
        }
        break;
    case SysInfoGraphGetTooltipText:
        {
            PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT getTooltipText = Parameter1;
            ULONG usedPages;

            usedPages = PhGetItemCircularBuffer_ULONG(&PhPhysicalHistory, getTooltipText->Index);

            PhSwapReference2(&Section->GraphState.TooltipText, PhFormatString(
                L"Physical Memory: %s\n%s",
                PhaFormatSize(UInt32x32To64(usedPages, PAGE_SIZE), -1)->Buffer,
                ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                ));
            getTooltipText->Text = Section->GraphState.TooltipText->sr;
        }
        break;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = Parameter1;
            ULONG totalPages;
            ULONG usedPages;

            totalPages = PhSystemBasicInformation.NumberOfPhysicalPages;
            usedPages = totalPages - PhPerfInformation.AvailablePages;

            drawPanel->Title = PhCreateString(L"Memory");
            drawPanel->SubTitle = PhFormatString(
                L"%.0f%%\n%s / %s",
                (FLOAT)usedPages * 100 / totalPages,
                PhapFormatSizeWithPrecision(UInt32x32To64(usedPages, PAGE_SIZE), 1)->Buffer,
                PhapFormatSizeWithPrecision(UInt32x32To64(totalPages, PAGE_SIZE), 1)->Buffer
                );
        }
        break;
    }

    return FALSE;
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

        if (!PH_IS_FAKE_PROCESS_ID(maxProcessRecord->ProcessId))
        {
            maxUsageString = PhaFormatString(
                L"\n%s (%u): R+O: %s, W: %s",
                maxProcessRecord->ProcessName->Buffer,
                (ULONG)maxProcessRecord->ProcessId,
                PhaFormatSize(maxIoReadOther, -1)->Buffer,
                PhaFormatSize(maxIoWrite, -1)->Buffer
                );
        }
        else
        {
            maxUsageString = PhaFormatString(
                L"\n%s: R+O: %s, W: %s",
                maxProcessRecord->ProcessName->Buffer,
                PhaFormatSize(maxIoReadOther, -1)->Buffer,
                PhaFormatSize(maxIoWrite, -1)->Buffer
                );
        }
#else
        maxUsageString = PhaConcatStrings2(L"\n", maxProcessRecord->ProcessName->Buffer);
#endif

        PhDereferenceProcessRecord(maxProcessRecord);
    }

    return maxUsageString;
}

static BOOLEAN PhpIoSectionCallback(
    __in PPH_SYSINFO_SECTION Section,
    __in HWND DialogWindowHandle,
    __in PH_SYSINFO_SECTION_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2
    )
{
    switch (Message)
    {
    case SysInfoGraphGetDrawInfo:
        {
            PPH_GRAPH_DRAW_INFO drawInfo = Parameter1;
            ULONG i;
            FLOAT max;

            drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
            Section->Parameters->ColorSetupFunction(drawInfo, PhCsColorIoReadOther, PhCsColorIoWrite);
            PhGetDrawInfoGraphBuffers(&Section->GraphState.Buffers, drawInfo, PhIoReadHistory.Count);

            if (!Section->GraphState.Valid)
            {
                max = 0;

                for (i = 0; i < drawInfo->LineDataCount; i++)
                {
                    FLOAT data1;
                    FLOAT data2;

                    Section->GraphState.Data1[i] = data1 =
                        (FLOAT)PhGetItemCircularBuffer_ULONG64(&PhIoReadHistory, i) +
                        (FLOAT)PhGetItemCircularBuffer_ULONG64(&PhIoOtherHistory, i);
                    Section->GraphState.Data2[i] = data2 =
                        (FLOAT)PhGetItemCircularBuffer_ULONG64(&PhIoWriteHistory, i);

                    if (max < data1 + data2)
                        max = data1 + data2;
                }

                // Minimum scaling of 1 MB.
                if (max < 1024 * 1024)
                    max = 1024 * 1024;

                // Scale the data.

                PhxfDivideSingle2U(
                    Section->GraphState.Data1,
                    max,
                    drawInfo->LineDataCount
                    );
                PhxfDivideSingle2U(
                    Section->GraphState.Data2,
                    max,
                    drawInfo->LineDataCount
                    );

                Section->GraphState.Valid = TRUE;
            }
        }
        break;
    case SysInfoGraphGetTooltipText:
        {
            PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT getTooltipText = Parameter1;
            ULONG64 ioRead;
            ULONG64 ioWrite;
            ULONG64 ioOther;

            ioRead = PhGetItemCircularBuffer_ULONG64(&PhIoReadHistory, getTooltipText->Index);
            ioWrite = PhGetItemCircularBuffer_ULONG64(&PhIoWriteHistory, getTooltipText->Index);
            ioOther = PhGetItemCircularBuffer_ULONG64(&PhIoOtherHistory, getTooltipText->Index);

            PhSwapReference2(&Section->GraphState.TooltipText, PhFormatString(
                L"R: %s\nW: %s\nO: %s%s\n%s",
                PhaFormatSize(ioRead, -1)->Buffer,
                PhaFormatSize(ioWrite, -1)->Buffer,
                PhaFormatSize(ioOther, -1)->Buffer,
                PhGetStringOrEmpty(PhapGetMaxIoString(getTooltipText->Index)),
                ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                ));
            getTooltipText->Text = Section->GraphState.TooltipText->sr;
        }
        break;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = Parameter1;

            drawPanel->Title = PhCreateString(L"I/O");
            drawPanel->SubTitle = PhFormatString(
                L"R+O: %s\nW: %s",
                PhapFormatSizeWithPrecision(PhIoReadDelta.Delta + PhIoOtherDelta.Delta, 1)->Buffer,
                PhapFormatSizeWithPrecision(PhIoWriteDelta.Delta, 1)->Buffer
                );
        }
        break;
    }

    return FALSE;
}
