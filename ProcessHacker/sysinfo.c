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

static HANDLE PhSipThread = NULL;
static HWND PhSipWindow = NULL;
static PH_EVENT InitializedEvent = PH_EVENT_INIT;
static PH_LAYOUT_MANAGER WindowLayoutManager;
static RECT MinimumSize;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

static PPH_LIST SectionList;
static PH_SYSINFO_PARAMETERS CurrentParameters;
static PH_SYSINFO_VIEW_TYPE CurrentView;
static PPH_SYSINFO_SECTION CurrentSection;
static HWND RestoreSummaryControl;

static PPH_SYSINFO_SECTION CpuSection;
static PPH_SYSINFO_SECTION MemorySection;
static PPH_SYSINFO_SECTION IoSection;

static BOOLEAN AlwaysOnTop;

VOID PhShowSystemInformationDialog(
    __in_opt PWSTR SectionName
    )
{
    if (!PhSipWindow)
    {
        if (!(PhSipThread = PhCreateThread(0, PhSipSysInfoThreadStart, NULL)))
        {
            PhShowStatus(PhMainWndHandle, L"Unable to create the system information window", 0, GetLastError());
            return;
        }

        PhWaitForEvent(&InitializedEvent, NULL);
    }

    SendMessage(PhSipWindow, SI_MSG_SYSINFO_ACTIVATE, 0, 0);
}

NTSTATUS PhSipSysInfoThreadStart(
    __in PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    BOOL result;
    MSG message;

    PhInitializeAutoPool(&autoPool);

    PhSipWindow = CreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_SYSINFO),
        NULL,
        PhSipSysInfoDialogProc
        );

    PhSetEvent(&InitializedEvent);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        if (!IsDialogMessage(PhSipWindow, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);
    PhResetEvent(&InitializedEvent);
    NtClose(PhSipThread);

    PhSipWindow = NULL;
    PhSipThread = NULL;

    return STATUS_SUCCESS;
}

INT_PTR CALLBACK PhSipSysInfoDialogProc(
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
            PhSipWindow = hwndDlg;
            PhSipOnInitDialog();
        }
        break;
    case WM_DESTROY:
        {
            PhSipOnDestroy();
        }
        break;
    case WM_SHOWWINDOW:
        {
            PhSipOnShowWindow(!!wParam, (ULONG)lParam);
        }
        break;
    case WM_SIZE:
        {
            PhSipOnSize();
        }
        break;
    case WM_SIZING:
        {
            PhSipOnSizing((ULONG)wParam, (PRECT)lParam);
        }
        break;
    case WM_COMMAND:
        {
            PhSipOnCommand(LOWORD(wParam), HIWORD(wParam));
        }
        break;
    case WM_NOTIFY:
        {
            LRESULT result;

            if (PhSipOnNotify((NMHDR *)lParam, &result))
            {
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, result);
                return TRUE;
            }
        }
        break;
    case WM_DRAWITEM:
        {
            if (PhSipOnDrawItem(wParam, (DRAWITEMSTRUCT *)lParam))
                return TRUE;
        }
        break;
    }

    if (uMsg >= SI_MSG_SYSINFO_FIRST && uMsg <= SI_MSG_SYSINFO_LAST)
    {
        PhSipOnUserMessage(uMsg, wParam, lParam);
    }

    return FALSE;
}

VOID PhSipOnInitDialog(
    VOID
    )
{
    PhSetWindowStyle(PhSipWindow, WS_CLIPCHILDREN, WS_CLIPCHILDREN);

    PhInitializeLayoutManager(&WindowLayoutManager, PhSipWindow);

    PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(PhSipWindow, IDC_ALWAYSONTOP), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
    PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(PhSipWindow, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

    PhRegisterCallback(
        &PhProcessesUpdatedEvent,
        PhSipSysInfoUpdateHandler,
        NULL,
        &ProcessesUpdatedRegistration
        );

    PhLoadWindowPlacementFromSetting(L"SysInfoWindowPosition", L"SysInfoWindowSize", PhSipWindow);
}

VOID PhSipOnDestroy(
    VOID
    )
{
    ULONG i;
    PPH_SYSINFO_SECTION section;

    PhUnregisterCallback(
        &PhProcessesUpdatedEvent,
        &ProcessesUpdatedRegistration
        );

    PhSetIntegerSetting(L"SysInfoWindowAlwaysOnTop", AlwaysOnTop);

    PhSaveWindowPlacementToSetting(L"SysInfoWindowPosition", L"SysInfoWindowSize", PhSipWindow);

    for (i = 0; i < SectionList->Count; i++)
    {
        section = SectionList->Items[i];
        PhSipDestroySection(section);
    }

    PhDereferenceObject(SectionList);
    SectionList = NULL;
    PhSipDeleteParameters();

    PhDeleteLayoutManager(&WindowLayoutManager);
    PostQuitMessage(0);
}

VOID PhSipOnShowWindow(
    __in BOOLEAN Showing,
    __in ULONG State
    )
{
    RECT buttonRect;
    RECT clientRect;

    SectionList = PhCreateList(8);
    PhSipInitializeParameters();
    CurrentView = SysInfoSummaryView;

    CpuSection = PhSipCreateInternalSection(L"CPU", 0, PhSipCpuSectionCallback);
    MemorySection = PhSipCreateInternalSection(L"Memory", 0, PhSipMemorySectionCallback);
    IoSection = PhSipCreateInternalSection(L"I/O", 0, PhSipIoSectionCallback);

    RestoreSummaryControl = CreateWindow(
        L"STATIC",
        NULL,
        WS_CHILD | WS_CLIPSIBLINGS | SS_OWNERDRAW | SS_NOTIFY,
        0,
        0,
        3,
        3,
        PhSipWindow,
        (HMENU)IDC_RESET,
        PhInstanceHandle,
        NULL
        );

    GetWindowRect(GetDlgItem(PhSipWindow, IDOK), &buttonRect);
    MapWindowPoints(NULL, PhSipWindow, (POINT *)&buttonRect, 2);
    GetClientRect(PhSipWindow, &clientRect);

    MinimumSize.left = 0;
    MinimumSize.top = 0;
    MinimumSize.right = 400;
    MinimumSize.bottom = 200;

    if (SectionList->Count != 0)
    {
        MinimumSize.bottom =
            GetSystemMetrics(SM_CYCAPTION) +
            PH_SYSINFO_WINDOW_PADDING +
            CurrentParameters.SectionViewGraphHeight * (SectionList->Count + 1) + // includes Back button
            PH_SYSINFO_GRAPH_PADDING * SectionList->Count +
            PH_SYSINFO_WINDOW_PADDING +
            clientRect.bottom - buttonRect.top +
            GetSystemMetrics(SM_CYSIZEFRAME) * 2;
    }

    AlwaysOnTop = (BOOLEAN)PhGetIntegerSetting(L"SysInfoWindowAlwaysOnTop");
    Button_SetCheck(GetDlgItem(PhSipWindow, IDC_ALWAYSONTOP), AlwaysOnTop ? BST_CHECKED : BST_UNCHECKED);
    PhSipSetAlwaysOnTop();

    SendMessage(PhSipWindow, WM_SIZE, 0, 0);
    SendMessage(PhSipWindow, SI_MSG_SYSINFO_UPDATE, 0, 0);
}

VOID PhSipOnSize(
    VOID
    )
{
    PhLayoutManagerLayout(&WindowLayoutManager);

    if (SectionList && SectionList->Count != 0)
    {
        if (CurrentView == SysInfoSummaryView)
            PhSipLayoutSummaryView();
        else if (CurrentView == SysInfoSectionView)
            PhSipLayoutSectionView();
    }
}

VOID PhSipOnSizing(
    __in ULONG Edge,
    __in PRECT DragRectangle
    )
{
    PhResizingMinimumSize(DragRectangle, Edge, MinimumSize.right, MinimumSize.bottom);
}

VOID PhSipOnCommand(
    __in ULONG Id,
    __in ULONG Code
    )
{
    switch (Id)
    {
    case IDCANCEL:
    case IDOK:
        DestroyWindow(PhSipWindow);
        break;
    case IDC_ALWAYSONTOP:
        {
            AlwaysOnTop = Button_GetCheck(GetDlgItem(PhSipWindow, IDC_ALWAYSONTOP)) == BST_CHECKED;
            PhSipSetAlwaysOnTop();
        }
        break;
    case IDC_RESET:
        {
            if (Code == STN_CLICKED)
            {
                PhSipRestoreSummaryView();
            }
        }
        break;
    }
}

BOOLEAN PhSipOnNotify(
    __in NMHDR *Header,
    __out LRESULT *Result
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;
            ULONG i;
            PPH_SYSINFO_SECTION section;

            for (i = 0; i < SectionList->Count; i++)
            {
                section = SectionList->Items[i];

                if (getDrawInfo->Header.hwndFrom == section->GraphHandle)
                {
                    section->Callback(section, SysInfoGraphGetDrawInfo, drawInfo, 0);

                    if (CurrentView == SysInfoSectionView)
                        drawInfo->Flags &= ~PH_GRAPH_USE_GRID;

                    break;
                }
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;
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

                        section->Callback(section, SysInfoGraphGetTooltipText, &graphGetTooltipText, 0);

                        getTooltipText->Text = graphGetTooltipText.Text;

                        break;
                    }
                }
            }
        }
        break;
    case GCN_DRAWPANEL:
        {
            PPH_GRAPH_DRAWPANEL drawPanel = (PPH_GRAPH_DRAWPANEL)Header;
            ULONG i;
            PPH_SYSINFO_SECTION section;

            if (CurrentView == SysInfoSummaryView)
            {
                for (i = 0; i < SectionList->Count; i++)
                {
                    section = SectionList->Items[i];

                    if (drawPanel->Header.hwndFrom == section->GraphHandle)
                    {
                        PhSipDrawPanel(section, drawPanel->hdc, &drawPanel->Rect);
                        break;
                    }
                }
            }
        }
        break;
    case GCN_MOUSEEVENT:
        {
            PPH_GRAPH_MOUSEEVENT mouseEvent = (PPH_GRAPH_MOUSEEVENT)Header;
            ULONG i;
            PPH_SYSINFO_SECTION section;

            for (i = 0; i < SectionList->Count; i++)
            {
                section = SectionList->Items[i];

                if (mouseEvent->Header.hwndFrom == section->GraphHandle)
                {
                    if (mouseEvent->Message == WM_LBUTTONDOWN)
                    {
                        PhSipEnterSectionView(section);
                    }

                    break;
                }
            }
        }
        break;
    }

    return FALSE;
}

BOOLEAN PhSipOnDrawItem(
    __in ULONG_PTR Id,
    __in DRAWITEMSTRUCT *DrawItemStruct
    )
{
    ULONG i;
    PPH_SYSINFO_SECTION section;

    if (Id == IDC_RESET)
    {
        PhSipDrawRestoreSummaryPanel(DrawItemStruct->hDC, &DrawItemStruct->rcItem);
        return TRUE;
    }

    for (i = 0; i < SectionList->Count; i++)
    {
        section = SectionList->Items[i];

        if (Id == section->PanelId)
        {
            PhSipDrawPanel(section, DrawItemStruct->hDC, &DrawItemStruct->rcItem);
            return TRUE;
        }
    }

    return FALSE;
}

VOID PhSipOnUserMessage(
    __in ULONG Message,
    __in ULONG_PTR WParam,
    __in ULONG_PTR LParam
    )
{
    switch (Message)
    {
    case SI_MSG_SYSINFO_ACTIVATE:
        {
            if (IsIconic(PhSipWindow))
                ShowWindow(PhSipWindow, SW_RESTORE);
            else
                ShowWindow(PhSipWindow, SW_SHOW);

            SetForegroundWindow(PhSipWindow);
        }
        break;
    case SI_MSG_SYSINFO_UPDATE:
        {
            ULONG i;
            PPH_SYSINFO_SECTION section;

            for (i = 0; i < SectionList->Count; i++)
            {
                section = SectionList->Items[i];

                section->Callback(section, SysInfoTick, NULL, NULL);

                section->GraphState.Valid = FALSE;
                section->GraphState.TooltipIndex = -1;
                Graph_MoveGrid(section->GraphHandle, 1);
                Graph_Draw(section->GraphHandle);
                Graph_UpdateTooltip(section->GraphHandle);
                InvalidateRect(section->GraphHandle, NULL, FALSE);

                InvalidateRect(section->PanelHandle, NULL, FALSE);
            }
        }
        break;
    }
}

VOID PhSiSetColorsGraphDrawInfo(
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

VOID PhSipInitializeParameters(
    VOID
    )
{
    LOGFONT logFont;
    HDC hdc;
    TEXTMETRIC textMetrics;
    HFONT originalFont;

    memset(&CurrentParameters, 0, sizeof(PH_SYSINFO_PARAMETERS));

    CurrentParameters.SysInfoWindowHandle = PhSipWindow;

    if (SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &logFont, 0))
    {
        CurrentParameters.Font = CreateFontIndirect(&logFont);
    }
    else
    {
        CurrentParameters.Font = PhApplicationFont;
        GetObject(PhApplicationFont, sizeof(LOGFONT), &logFont);
    }

    hdc = GetDC(PhSipWindow);

    logFont.lfHeight -= MulDiv(4, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    CurrentParameters.MediumFont = CreateFontIndirect(&logFont);

    logFont.lfHeight -= MulDiv(4, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    CurrentParameters.LargeFont = CreateFontIndirect(&logFont);

    CurrentParameters.GraphBackColor = RGB(0xef, 0xef, 0xef);
    CurrentParameters.PanelForeColor = RGB(0x00, 0x00, 0x00);

    CurrentParameters.ColorSetupFunction = PhSiSetColorsGraphDrawInfo;

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

    ReleaseDC(PhSipWindow, hdc);
}

VOID PhSipDeleteParameters(
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

PPH_SYSINFO_SECTION PhSipCreateSection(
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
        PhSipWindow,
        (HMENU)(IDDYNAMIC + SectionList->Count * 2 + 1),
        PhInstanceHandle,
        NULL
        );
    PhInitializeGraphState(&section->GraphState);
    section->Parameters = &CurrentParameters;

    Graph_GetOptions(section->GraphHandle, &options);
    options.FadeOutWidth = PH_SYSINFO_FADE_WIDTH;
    //options.DefaultCursor = LoadCursor(NULL, IDC_HAND);
    Graph_SetOptions(section->GraphHandle, &options);
    Graph_SetTooltip(section->GraphHandle, TRUE);

    section->PanelId = IDDYNAMIC + SectionList->Count * 2 + 2;
    section->PanelHandle = CreateWindow(
        L"STATIC",
        NULL,
        WS_CHILD | WS_CLIPSIBLINGS | SS_OWNERDRAW,
        0,
        0,
        3,
        3,
        PhSipWindow,
        (HMENU)section->PanelId,
        PhInstanceHandle,
        NULL
        );

    PhAddItemList(SectionList, section);

    return section;
}

VOID PhSipDestroySection(
    __in PPH_SYSINFO_SECTION Section
    )
{
    PhDeleteGraphState(&Section->GraphState);
    PhFree(Section);
}

PPH_SYSINFO_SECTION PhSipCreateInternalSection(
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

    return PhSipCreateSection(&section);
}

VOID PhSipDrawRestoreSummaryPanel(
    __in HDC hdc,
    __in PRECT Rect
    )
{
    FillRect(hdc, Rect, GetSysColorBrush(COLOR_3DFACE));
    SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    SetBkMode(hdc, TRANSPARENT);

    SelectObject(hdc, CurrentParameters.MediumFont);
    DrawText(hdc, L"Back", 4, Rect, DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_SINGLELINE);
}

VOID PhSipDrawPanel(
    __in PPH_SYSINFO_SECTION Section,
    __in HDC hdc,
    __in PRECT Rect
    )
{
    PH_SYSINFO_DRAW_PANEL sysInfoDrawPanel;

    if (CurrentView == SysInfoSectionView)
        FillRect(hdc, Rect, GetSysColorBrush(COLOR_3DFACE));

    sysInfoDrawPanel.hdc = hdc;
    sysInfoDrawPanel.Rect = *Rect;
    sysInfoDrawPanel.Rect.right = PH_SYSINFO_FADE_WIDTH;
    sysInfoDrawPanel.CustomDraw = FALSE;

    sysInfoDrawPanel.Title = NULL;
    sysInfoDrawPanel.SubTitle = NULL;

    Section->Callback(Section, SysInfoGraphDrawPanel, &sysInfoDrawPanel, NULL);

    if (!sysInfoDrawPanel.CustomDraw)
        PhSipDefaultDrawPanel(Section, &sysInfoDrawPanel);

    PhSwapReference(&sysInfoDrawPanel.Title, NULL);
    PhSwapReference(&sysInfoDrawPanel.SubTitle, NULL);
}

VOID PhSipDefaultDrawPanel(
    __in PPH_SYSINFO_SECTION Section,
    __in PPH_SYSINFO_DRAW_PANEL DrawPanel
    )
{
    HDC hdc;
    RECT rect;

    hdc = DrawPanel->hdc;

    if (CurrentView == SysInfoSummaryView)
        SetTextColor(hdc, CurrentParameters.PanelForeColor);
    else
        SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));

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

VOID PhSipLayoutSummaryView(
    VOID
    )
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

    GetWindowRect(GetDlgItem(PhSipWindow, IDOK), &buttonRect);
    MapWindowPoints(NULL, PhSipWindow, (POINT *)&buttonRect, 2);
    GetClientRect(PhSipWindow, &clientRect);

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

VOID PhSipLayoutSectionView(
    VOID
    )
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

    GetWindowRect(GetDlgItem(PhSipWindow, IDOK), &buttonRect);
    MapWindowPoints(NULL, PhSipWindow, (POINT *)&buttonRect, 2);
    GetClientRect(PhSipWindow, &clientRect);

    availableHeight = buttonRect.top - PH_SYSINFO_WINDOW_PADDING * 2;
    availableWidth = clientRect.right - PH_SYSINFO_WINDOW_PADDING * 2;
    graphHeight = (availableHeight - PH_SYSINFO_GRAPH_PADDING * (SectionList->Count - 1)) / SectionList->Count;

    deferHandle = BeginDeferWindowPos(SectionList->Count * 2 + 1);
    y = PH_SYSINFO_WINDOW_PADDING;
    // TODO: Progressively make each section graph smaller as the window gets smaller
    for (i = 0; i < SectionList->Count; i++)
    {
        section = SectionList->Items[i];

        deferHandle = DeferWindowPos(
            deferHandle,
            section->GraphHandle,
            NULL,
            PH_SYSINFO_WINDOW_PADDING,
            y,
            PH_SYSINFO_SMALL_GRAPH_WIDTH,
            CurrentParameters.SectionViewGraphHeight,
            SWP_NOACTIVATE | SWP_NOZORDER
            );

        deferHandle = DeferWindowPos(
            deferHandle,
            section->PanelHandle,
            NULL,
            PH_SYSINFO_WINDOW_PADDING + PH_SYSINFO_SMALL_GRAPH_WIDTH + PH_SYSINFO_SMALL_GRAPH_PADDING,
            y,
            PH_SYSINFO_FADE_WIDTH - 40,
            CurrentParameters.SectionViewGraphHeight,
            SWP_NOACTIVATE | SWP_NOZORDER
            );

        y += CurrentParameters.SectionViewGraphHeight + PH_SYSINFO_SMALL_GRAPH_PADDING;

        section->GraphState.Valid = FALSE;
    }

    deferHandle = DeferWindowPos(
        deferHandle,
        RestoreSummaryControl,
        NULL,
        PH_SYSINFO_WINDOW_PADDING,
        y,
        PH_SYSINFO_SMALL_GRAPH_WIDTH + PH_SYSINFO_SMALL_GRAPH_PADDING + PH_SYSINFO_FADE_WIDTH - 40,
        CurrentParameters.SectionViewGraphHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    EndDeferWindowPos(deferHandle);
}

VOID PhSipEnterSectionView(
    __in PPH_SYSINFO_SECTION NewSection
    )
{
    ULONG i;
    PPH_SYSINFO_SECTION section;
    BOOLEAN fromSummaryView;

    fromSummaryView = CurrentView == SysInfoSummaryView;
    CurrentView = SysInfoSectionView;
    CurrentSection = NewSection;

    for (i = 0; i < SectionList->Count; i++)
    {
        section = SectionList->Items[i];

        section->Callback(section, SysInfoViewChanging, (PVOID)CurrentView, CurrentSection);

        if (fromSummaryView)
        {
            PhSetWindowStyle(section->GraphHandle, GC_STYLE_FADEOUT | GC_STYLE_DRAW_PANEL, 0);
            ShowWindow(section->PanelHandle, SW_SHOW);
        }

        if (section == CurrentSection && !section->DialogHandle)
            PhSipCreateSectionDialog(section);

        if (section->DialogHandle)
        {
            if (section == CurrentSection)
                ShowWindow(section->DialogHandle, SW_SHOW);
            else
                ShowWindow(section->DialogHandle, SW_HIDE);
        }
    }

    ShowWindow(RestoreSummaryControl, SW_SHOW);

    PhSipLayoutSectionView();
}

VOID PhSipRestoreSummaryView(
    VOID
    )
{
    ULONG i;
    PPH_SYSINFO_SECTION section;

    if (CurrentView == SysInfoSummaryView)
        return;

    CurrentView = SysInfoSummaryView;
    CurrentSection = NULL;

    for (i = 0; i < SectionList->Count; i++)
    {
        section = SectionList->Items[i];

        section->Callback(section, SysInfoViewChanging, (PVOID)CurrentView, NULL);

        PhSetWindowStyle(section->GraphHandle, GC_STYLE_FADEOUT | GC_STYLE_DRAW_PANEL, GC_STYLE_FADEOUT | GC_STYLE_DRAW_PANEL);
        ShowWindow(section->PanelHandle, SW_HIDE);

        if (section->DialogHandle)
            ShowWindow(section->DialogHandle, SW_HIDE);
    }

    ShowWindow(RestoreSummaryControl, SW_HIDE);

    PhSipLayoutSummaryView();
}

HWND PhSipDefaultCreateDialog(
    __in PVOID Instance,
    __in PWSTR Template,
    __in DLGPROC DialogProc
    )
{
    HRSRC resourceInfo;
    ULONG resourceSize;
    HGLOBAL resourceHandle;
    PDLGTEMPLATEEX dialog;
    PDLGTEMPLATEEX dialogCopy;
    HWND dialogHandle;

    resourceInfo = FindResource(Instance, Template, MAKEINTRESOURCE(RT_DIALOG));

    if (!resourceInfo)
        return NULL;

    resourceSize = SizeofResource(Instance, resourceInfo);

    if (resourceSize == 0)
        return NULL;

    resourceHandle = LoadResource(Instance, resourceInfo);

    if (!resourceHandle)
        return NULL;

    dialog = LockResource(resourceHandle);

    if (!dialog)
        return NULL;

    dialogCopy = PhAllocateCopy(dialog, resourceSize);

    if (dialogCopy->signature == 0xffff)
    {
        dialogCopy->style = DS_SETFONT | DS_FIXEDSYS | DS_CONTROL | WS_CHILD;
    }
    else
    {
        ((DLGTEMPLATE *)dialogCopy)->style = DS_SETFONT | DS_FIXEDSYS | DS_CONTROL | WS_CHILD;
    }

    dialogHandle = CreateDialogIndirect(Instance, (DLGTEMPLATE *)dialogCopy, PhSipWindow, DialogProc);

    PhFree(dialogCopy);

    return dialogHandle;
}

VOID PhSipCreateSectionDialog(
    __in PPH_SYSINFO_SECTION Section
    )
{
    PH_SYSINFO_CREATE_DIALOG createDialog;

    memset(&createDialog, 0, sizeof(PH_SYSINFO_CREATE_DIALOG));

    if (Section->Callback(Section, SysInfoCreateDialog, &createDialog, NULL))
    {
        if (!createDialog.CustomCreate)
        {
            Section->DialogHandle = PhSipDefaultCreateDialog(createDialog.Instance, createDialog.Template, createDialog.DialogProc);
        }
    }
}

VOID PhSipSetAlwaysOnTop(
    VOID
    )
{
    SetWindowPos(PhSipWindow, AlwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

VOID NTAPI PhSipSysInfoUpdateHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    PostMessage(PhSipWindow, SI_MSG_SYSINFO_UPDATE, 0, 0);
}

PPH_STRING PhSipFormatSizeWithPrecision(
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

BOOLEAN PhSipCpuSectionCallback(
    __in PPH_SYSINFO_SECTION Section,
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
        return TRUE;
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
                PhGetStringOrEmpty(PhSipGetMaxCpuString(getTooltipText->Index)),
                ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                ));
            getTooltipText->Text = Section->GraphState.TooltipText->sr;
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = Parameter1;

            drawPanel->Title = PhCreateString(L"CPU");
            drawPanel->SubTitle = PhFormatString(L"%.2f%%", (PhCpuKernelUsage + PhCpuUserUsage) * 100);
        }
        return TRUE;
    }

    return FALSE;
}

PPH_PROCESS_RECORD PhSipReferenceMaxCpuRecord(
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

PPH_STRING PhSipGetMaxCpuString(
    __in LONG Index
    )
{
    PPH_PROCESS_RECORD maxProcessRecord;
#ifdef PH_RECORD_MAX_USAGE
    FLOAT maxCpuUsage;
#endif
    PPH_STRING maxUsageString = NULL;

    if (maxProcessRecord = PhSipReferenceMaxCpuRecord(Index))
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

BOOLEAN PhSipMemorySectionCallback(
    __in PPH_SYSINFO_SECTION Section,
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
        return TRUE;
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
        return TRUE;
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
                PhSipFormatSizeWithPrecision(UInt32x32To64(usedPages, PAGE_SIZE), 1)->Buffer,
                PhSipFormatSizeWithPrecision(UInt32x32To64(totalPages, PAGE_SIZE), 1)->Buffer
                );
        }
        return TRUE;
    }

    return FALSE;
}

BOOLEAN PhSipIoSectionCallback(
    __in PPH_SYSINFO_SECTION Section,
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
        return TRUE;
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
                PhGetStringOrEmpty(PhSipGetMaxIoString(getTooltipText->Index)),
                ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                ));
            getTooltipText->Text = Section->GraphState.TooltipText->sr;
        }
        return TRUE;
    case SysInfoGraphDrawPanel:
        {
            PPH_SYSINFO_DRAW_PANEL drawPanel = Parameter1;

            drawPanel->Title = PhCreateString(L"I/O");
            drawPanel->SubTitle = PhFormatString(
                L"R+O: %s\nW: %s",
                PhSipFormatSizeWithPrecision(PhIoReadDelta.Delta + PhIoOtherDelta.Delta, 1)->Buffer,
                PhSipFormatSizeWithPrecision(PhIoWriteDelta.Delta, 1)->Buffer
                );
        }
        return TRUE;
    }

    return FALSE;
}

PPH_PROCESS_RECORD PhSipReferenceMaxIoRecord(
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

PPH_STRING PhSipGetMaxIoString(
    __in LONG Index
    )
{
    PPH_PROCESS_RECORD maxProcessRecord;
#ifdef PH_RECORD_MAX_USAGE
    ULONG64 maxIoReadOther;
    ULONG64 maxIoWrite;
#endif
    PPH_STRING maxUsageString = NULL;

    if (maxProcessRecord = PhSipReferenceMaxIoRecord(Index))
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
