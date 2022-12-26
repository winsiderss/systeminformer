/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011-2016
 *     dmex    2017-2022
 *
 */

/*
 * This file contains the System Information framework. The "framework" handles creation, layout and
 * events for the top-level window itself. It manages the list of sections.
 *
 * A section is an object that provides information to the user about a type of system resource. The
 * CPU, Memory and I/O sections are added automatically to every System Information window. Plugins
 * can also add sections to the window. There are two views: summary and section. In summary view,
 * rows of graphs are displayed in the window, one graph for each section. The section is
 * responsible for providing the graph data and any text to draw on the left-hand side of the graph.
 * In section view, the graphs become mini-graphs on the left-hand side of the window. The section
 * displays its own embedded dialog in the remaining space. Any controls contained in this dialog,
 * including graphs, are the responsibility of the section.
 *
 * Users can enter section view by:
 * * Clicking on a graph or mini-graph.
 * * Pressing a number from 1 to 9.
 * * Using the tab or arrow keys to select a graph and pressing space or enter.
 *
 * Users can return to summary view by:
 * * Clicking "Back" on the left-hand side of the window.
 * * Pressing Backspace.
 * * Using the tab or arrow keys to select "Back" and pressing space or enter.
 */

#include <phapp.h>
#include <settings.h>
#include <sysinfo.h>
#include <sysinfop.h>

#include <vssym32.h>

#include <mainwnd.h>
#include <guisup.h>
#include <phplug.h>
#include <phsettings.h>

static HANDLE PhSipThread = NULL;
HWND PhSipWindow = NULL;
static PPH_LIST PhSipDialogList = NULL;
static PH_EVENT InitializedEvent = PH_EVENT_INIT;
static PWSTR InitialSectionName;
static RECT MinimumSize;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

static PPH_LIST SectionList;
static PH_SYSINFO_PARAMETERS CurrentParameters = {0};
static PH_SYSINFO_VIEW_TYPE CurrentView;
static PPH_SYSINFO_SECTION CurrentSection;
static HWND ContainerControl;
static HWND SeparatorControl;
static HWND RestoreSummaryControl;
static WNDPROC RestoreSummaryControlOldWndProc;
static BOOLEAN RestoreSummaryControlHot;
static BOOLEAN RestoreSummaryControlHasFocus;
static HTHEME ThemeData;
static BOOLEAN ThemeHasItemBackground;

VOID PhShowSystemInformationDialog(
    _In_opt_ PWSTR SectionName
    )
{
    InitialSectionName = SectionName;

    if (!PhSipThread)
    {
        if (!NT_SUCCESS(PhCreateThreadEx(&PhSipThread, PhSipSysInfoThreadStart, NULL)))
        {
            PhShowError(PhMainWndHandle, L"%s", L"Unable to create the window.");
            return;
        }

        PhWaitForEvent(&InitializedEvent, NULL);
    }

    SendMessage(PhSipWindow, SI_MSG_SYSINFO_ACTIVATE, (WPARAM)SectionName, 0);
}

NTSTATUS PhSipSysInfoThreadStart(
    _In_ PVOID Parameter
    )
{
    PH_AUTO_POOL autoPool;
    BOOL result;
    MSG message;
    HACCEL acceleratorTable;
    BOOLEAN processed;

    PhInitializeAutoPool(&autoPool);

    PhSipWindow = PhCreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_SYSINFO),
        NULL,
        PhSipSysInfoDialogProc,
        NULL
        );

    PhSetEvent(&InitializedEvent);

    acceleratorTable = LoadAccelerators(PhInstanceHandle, MAKEINTRESOURCE(IDR_SYSINFO_ACCEL));

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == -1)
            break;

        processed = FALSE;

        if (
            message.hwnd == PhSipWindow ||
            IsChild(PhSipWindow, message.hwnd)
            )
        {
            if (TranslateAccelerator(PhSipWindow, acceleratorTable, &message))
                processed = TRUE;
        }

        if (!processed && PhSipDialogList)
        {
            ULONG i;

            for (i = 0; i < PhSipDialogList->Count; i++)
            {
                if (IsDialogMessage((HWND)PhSipDialogList->Items[i], &message))
                {
                    processed = TRUE;
                    break;
                }
            }
        }

        if (!processed)
        {
            if (!IsDialogMessage(PhSipWindow, &message))
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
            }
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);
    PhResetEvent(&InitializedEvent);
    NtClose(PhSipThread);

    PhSipWindow = NULL;
    PhSipThread = NULL;

    if (PhSipDialogList)
    {
        PhDereferenceObject(PhSipDialogList);
        PhSipDialogList = NULL;
    }

    return STATUS_SUCCESS;
}

INT_PTR CALLBACK PhSipSysInfoDialogProc(
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
            PhSipWindow = hwndDlg;
            PhSipOnInitDialog();
        }
        break;
    case WM_DESTROY:
        {
            PhSipOnDestroy();
        }
        break;
    case WM_NCDESTROY:
        {
            PhSipOnNcDestroy();
        }
        break;
    case WM_SYSCOMMAND:
        {
            if (PhSipOnSysCommand((ULONG)wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
                return 0;
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
    case WM_THEMECHANGED:
        {
            PhSipOnThemeChanged();
        }
        break;
    case WM_COMMAND:
        {
            PhSipOnCommand((HWND)lParam, LOWORD(wParam), HIWORD(wParam));
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
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);

            if (PhEnableThemeSupport)
            {
                switch (PhCsGraphColorMode)
                {
                case 0: // New colors
                    SetTextColor((HDC)wParam, RGB(0x0, 0x0, 0x0));
                    SetDCBrushColor((HDC)wParam, RGB(0xef, 0xef, 0xef)); // GetSysColor(COLOR_WINDOW)
                    break;
                case 1: // Old colors
                    SetTextColor((HDC)wParam, RGB(0xff, 0xff, 0xff));
                    SetDCBrushColor((HDC)wParam, PhThemeWindowBackgroundColor); // RGB(30, 30, 30));
                    break;
                }
            }
            else
            {
                SetTextColor((HDC)wParam, GetSysColor(COLOR_WINDOWTEXT));
                SetDCBrushColor((HDC)wParam, GetSysColor(COLOR_WINDOW));
            }

            return (INT_PTR)GetStockBrush(DC_BRUSH);
        }
        break;
    case WM_DPICHANGED:
        {
            PhSipInitializeParameters();

            PhDpiChangedForwardChildWindows(hwndDlg);

            InvalidateRect(hwndDlg, NULL, TRUE);
        }
        break;
    }

    if (uMsg >= SI_MSG_SYSINFO_FIRST && uMsg <= SI_MSG_SYSINFO_LAST)
    {
        PhSipOnUserMessage(uMsg, wParam, lParam);
    }

    return FALSE;
}

INT_PTR CALLBACK PhSipContainerDialogProc(
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
            //if (WindowsVersion >= WINDOWS_8)
            //{
            //    // TODO: The container background is drawn before child controls
            //    // causing slight flickering when switching between sysinfo panels.
            //    // We need to somehow exclude the container background drawing,
            //    // setting the container window as composited works well but has
            //    // slower drawing and should be considered a temporary workaround. (dmex)
            //    PhSetWindowExStyle(hwndDlg, WS_EX_COMPOSITED, WS_EX_COMPOSITED);
            //}
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        {
            SetBkMode((HDC)wParam, TRANSPARENT);

            if (PhEnableThemeSupport)
            {
                switch (PhCsGraphColorMode)
                {
                case 0: // New colors
                    SetTextColor((HDC)wParam, RGB(0x0, 0x0, 0x0));
                    SetDCBrushColor((HDC)wParam, GetSysColor(COLOR_WINDOW));
                    break;
                case 1: // Old colors
                    SetTextColor((HDC)wParam, RGB(0xff, 0xff, 0xff));
                    SetDCBrushColor((HDC)wParam, PhThemeWindowBackgroundColor);
                    break;
                }
            }
            else
            {
                SetTextColor((HDC)wParam, GetSysColor(COLOR_WINDOWTEXT));
                SetDCBrushColor((HDC)wParam, GetSysColor(COLOR_WINDOW));
            }

            return (INT_PTR)GetStockBrush(DC_BRUSH);
        }
        break;
    }

    return FALSE;
}

VOID PhSipOnInitDialog(
    VOID
    )
{
    RECT clientRect;
    PH_STRINGREF sectionName;
    PPH_SYSINFO_SECTION section;
    LONG dpiValue;

    PhSetApplicationWindowIcon(PhSipWindow);

    dpiValue = PhGetWindowDpi(PhSipWindow);

    PhSetControlTheme(PhSipWindow, L"explorer");

    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
        PhSipSysInfoUpdateHandler,
        NULL,
        &ProcessesUpdatedRegistration
        );

    ContainerControl = PhCreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_CONTAINER),
        PhSipWindow,
        PhSipContainerDialogProc,
        NULL
        );

    PhSipUpdateThemeData();

    SectionList = PhCreateList(8);
    PhSipInitializeParameters();
    CurrentView = SysInfoSummaryView;
    CurrentSection = NULL;

    PhSipCreateInternalSection(L"CPU", 0, PhSipCpuSectionCallback);
    PhSipCreateInternalSection(L"Memory", 0, PhSipMemorySectionCallback);
    PhSipCreateInternalSection(L"I/O", 0, PhSipIoSectionCallback);

    if (PhPluginsEnabled)
    {
        PH_PLUGIN_SYSINFO_POINTERS pointers;

        pointers.WindowHandle = PhSipWindow;
        pointers.CreateSection = PhSipCreateSection;
        pointers.FindSection = PhSipFindSection;
        pointers.EnterSectionView = PhSipEnterSectionView;
        pointers.RestoreSummaryView = PhSipRestoreSummaryView;
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackSystemInformationInitializing), &pointers);
    }

    SeparatorControl = CreateWindow(
        WC_STATIC,
        NULL,
        WS_CHILD | SS_OWNERDRAW,
        0,
        0,
        3,
        3,
        PhSipWindow,
        NULL,
        PhInstanceHandle,
        NULL
        );
    RestoreSummaryControl = CreateWindow(
        WC_STATIC,
        NULL,
        WS_CHILD | WS_TABSTOP | SS_OWNERDRAW | SS_NOTIFY,
        0,
        0,
        3,
        3,
        PhSipWindow,
        NULL,
        PhInstanceHandle,
        NULL
        );

    RestoreSummaryControlOldWndProc = (WNDPROC)GetWindowLongPtr(RestoreSummaryControl, GWLP_WNDPROC);
    SetWindowLongPtr(RestoreSummaryControl, GWLP_WNDPROC, (LONG_PTR)PhSipPanelHookWndProc);
    RestoreSummaryControlHot = FALSE;

    GetClientRect(PhSipWindow, &clientRect);
    MinimumSize.left = 0;
    MinimumSize.top = 0;
    MinimumSize.right = 430;
    MinimumSize.bottom = 290;
    MapDialogRect(PhSipWindow, &MinimumSize);

    MinimumSize.right += CurrentParameters.PanelWidth;
    MinimumSize.right += PhGetSystemMetrics(SM_CXFRAME, dpiValue) * 2;
    MinimumSize.bottom += PhGetSystemMetrics(SM_CYFRAME, dpiValue) * 2;

    if (SectionList->Count != 0)
    {
        //ULONG newMinimumHeight;
        //
        //newMinimumHeight =
        //    GetSystemMetrics(SM_CYCAPTION) +
        //    CurrentParameters.WindowPadding +
        //    CurrentParameters.MinimumGraphHeight * SectionList->Count +
        //    CurrentParameters.MinimumGraphHeight + // Back button
        //    CurrentParameters.GraphPadding * SectionList->Count +
        //    CurrentParameters.WindowPadding +
        //    clientRect.bottom - buttonRect.top +
        //    GetSystemMetrics(SM_CYFRAME) * 2;
        //
        //if (newMinimumHeight > (ULONG)MinimumSize.bottom)
        //    MinimumSize.bottom = newMinimumHeight;
    }

    PhLoadWindowPlacementFromSetting(L"SysInfoWindowPosition", L"SysInfoWindowSize", PhSipWindow);

    if (PhGetIntegerSetting(L"SysInfoWindowState") == SW_MAXIMIZE)
        ShowWindow(PhSipWindow, SW_MAXIMIZE);

    if (InitialSectionName)
        PhInitializeStringRefLongHint(&sectionName, InitialSectionName);
    else
        sectionName = PhaGetStringSetting(L"SysInfoWindowSection")->sr;

    if (sectionName.Length != 0 && (section = PhSipFindSection(&sectionName)))
    {
        PhSipEnterSectionView(section);
    }

    PhRegisterWindowCallback(PhSipWindow, PH_PLUGIN_WINDOW_EVENT_TYPE_TOPMOST, NULL);

    //PhInitializeWindowTheme(PhSipWindow, PhEnableThemeSupport);
    PhInitializeThemeWindowFrame(PhSipWindow);

    PhSipOnSize();
    PhSipOnUserMessage(SI_MSG_SYSINFO_UPDATE, 0, 0);
}

VOID PhSipOnDestroy(
    VOID
    )
{
    PhUnregisterWindowCallback(PhSipWindow);
    PhUnregisterCallback(PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent), &ProcessesUpdatedRegistration);

    if (CurrentSection)
        PhSetStringSetting2(L"SysInfoWindowSection", &CurrentSection->Name);
    else
        PhSetStringSetting(L"SysInfoWindowSection", L"");

    PhSaveWindowPlacementToSetting(L"SysInfoWindowPosition", L"SysInfoWindowSize", PhSipWindow);
    PhSipSaveWindowState();
}

VOID PhSipOnNcDestroy(
    VOID
    )
{
    ULONG i;
    PPH_SYSINFO_SECTION section;

    for (i = 0; i < SectionList->Count; i++)
    {
        section = SectionList->Items[i];
        PhSipDestroySection(section);
    }

    PhDereferenceObject(SectionList);
    SectionList = NULL;
    PhSipDeleteParameters();

    if (ThemeData)
    {
        PhCloseThemeData(ThemeData);
        ThemeData = NULL;
    }

    PostQuitMessage(0);
}

BOOLEAN PhSipOnSysCommand(
    _In_ ULONG Type,
    _In_ LONG CursorScreenX,
    _In_ LONG CursorScreenY
    )
{
    switch (Type)
    {
    case SC_MINIMIZE:
        {
            // Save the current window state because we may not have a chance to later.
            PhSipSaveWindowState();
        }
        break;
    }

    return FALSE;
}

VOID PhSipOnSize(
    VOID
    )
{
    if (SectionList && SectionList->Count != 0)
    {
        if (CurrentView == SysInfoSummaryView)
            PhSipLayoutSummaryView();
        else if (CurrentView == SysInfoSectionView)
            PhSipLayoutSectionView();
    }
}

VOID PhSipOnSizing(
    _In_ ULONG Edge,
    _In_ PRECT DragRectangle
    )
{
    PhResizingMinimumSize(DragRectangle, Edge, MinimumSize.right, MinimumSize.bottom);
}

VOID PhSipOnThemeChanged(
    VOID
    )
{
    PhSipUpdateThemeData();
}

VOID PhSipOnCommand(
    _In_ HWND HwndControl,
    _In_ ULONG Id,
    _In_ ULONG Code
    )
{
    switch (Id)
    {
    case IDCANCEL:
        DestroyWindow(PhSipWindow);
        break;
    case IDC_BACK:
        {
            PhSipRestoreSummaryView();
        }
        break;
    case IDC_REFRESH:
        {
            ProcessHacker_Refresh();
        }
        break;
    case IDC_PAUSE:
        {
            ProcessHacker_SetUpdateAutomatically(!ProcessHacker_GetUpdateAutomatically());
        }
        break;
    case IDC_MAXSCREEN:
        {
            static WINDOWPLACEMENT windowLayout = { sizeof(WINDOWPLACEMENT) };
            ULONG windowStyle = (ULONG)GetWindowLongPtr(PhSipWindow, GWL_STYLE);

            if (windowStyle & WS_OVERLAPPEDWINDOW)
            {
                MONITORINFO info = { sizeof(MONITORINFO) };

                if (
                    GetWindowPlacement(PhSipWindow, &windowLayout) &&
                    GetMonitorInfo(MonitorFromWindow(PhSipWindow, MONITOR_DEFAULTTOPRIMARY), &info)
                    )
                {
                    PhSetWindowStyle(PhSipWindow, WS_OVERLAPPEDWINDOW, 0);
                    SetWindowPos(
                        PhSipWindow,
                        HWND_TOPMOST,
                        info.rcMonitor.left,
                        info.rcMonitor.top,
                        (info.rcMonitor.right - info.rcMonitor.left),
                        (info.rcMonitor.bottom - info.rcMonitor.top),
                        SWP_NOOWNERZORDER | SWP_FRAMECHANGED
                        );
                }
            }
            else
            {
                PhSetWindowStyle(PhSipWindow, WS_OVERLAPPEDWINDOW, WS_OVERLAPPEDWINDOW);
                SetWindowPlacement(PhSipWindow, &windowLayout);
                SetWindowPos(PhSipWindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            }
        }
        break;
    }

    if (SectionList && Id >= ID_DIGIT1 && Id <= ID_DIGIT9)
    {
        ULONG index;

        index = Id - ID_DIGIT1;

        if (index < SectionList->Count)
            PhSipEnterSectionView(SectionList->Items[index]);
    }

    if (SectionList && Code == STN_CLICKED)
    {
        ULONG i;
        PPH_SYSINFO_SECTION section;

        for (i = 0; i < SectionList->Count; i++)
        {
            section = SectionList->Items[i];

            if (Id == section->PanelId)
            {
                PhSipEnterSectionView(section);
                break;
            }
        }
    }

    if (HwndControl == RestoreSummaryControl)
    {
        if (Code == STN_CLICKED)
        {
            PhSipRestoreSummaryView();
        }
    }
}

_Success_(return)
BOOLEAN PhSipOnNotify(
    _In_ NMHDR *Header,
    _Out_ LRESULT *Result
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
                    {
                        drawInfo->Flags &= ~(PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_GRID_Y | PH_GRAPH_LABEL_MAX_Y);
                    }
                    else
                    {
                        ULONG badWidth = CurrentParameters.PanelWidth;

                        // Try not to draw max data point labels that will get covered by the
                        // fade-out part of the graph.
                        if (badWidth < drawInfo->Width)
                            drawInfo->LabelMaxYIndexLimit = (drawInfo->Width - badWidth) / 2;
                        else
                            drawInfo->LabelMaxYIndexLimit = ULONG_MAX;
                    }

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
    _In_ ULONG_PTR Id,
    _In_ PDRAWITEMSTRUCT DrawItemStruct
    )
{
    ULONG i;
    PPH_SYSINFO_SECTION section;

    if (DrawItemStruct->hwndItem == RestoreSummaryControl)
    {
        PhSipDrawRestoreSummaryPanel(DrawItemStruct);
        return TRUE;
    }
    else if (DrawItemStruct->hwndItem == SeparatorControl)
    {
        PhSipDrawSeparator(DrawItemStruct);
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
    _In_ ULONG Message,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam
    )
{
    switch (Message)
    {
    case SI_MSG_SYSINFO_ACTIVATE:
        {
            PWSTR sectionName = (PWSTR)WParam;

            if (SectionList && sectionName)
            {
                PH_STRINGREF sectionNameSr;
                PPH_SYSINFO_SECTION section;

                PhInitializeStringRefLongHint(&sectionNameSr, sectionName);
                section = PhSipFindSection(&sectionNameSr);

                if (section)
                    PhSipEnterSectionView(section);
                else
                    PhSipRestoreSummaryView();
            }

            if (IsMinimized(PhSipWindow))
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

            if (SectionList)
            {
                for (i = 0; i < SectionList->Count; i++)
                {
                    section = SectionList->Items[i];

                    section->Callback(section, SysInfoTick, NULL, NULL);

                    section->GraphState.Valid = FALSE;
                    section->GraphState.TooltipIndex = ULONG_MAX;
                    Graph_MoveGrid(section->GraphHandle, 1);
                    Graph_Draw(section->GraphHandle);
                    Graph_UpdateTooltip(section->GraphHandle);
                    InvalidateRect(section->GraphHandle, NULL, FALSE);

                    InvalidateRect(section->PanelHandle, NULL, FALSE);
                }
            }
        }
        break;
    case SI_MSG_SYSINFO_CHANGE_SETTINGS:
        {
            ULONG i;
            PPH_SYSINFO_SECTION section;
            PH_GRAPH_OPTIONS options;

            if (SectionList)
            {
                for (i = 0; i < SectionList->Count; i++)
                {
                    section = SectionList->Items[i];
                    Graph_GetOptions(section->GraphHandle, &options);
                    options.FadeOutBackColor = CurrentParameters.GraphBackColor;
                    Graph_SetOptions(section->GraphHandle, &options);
                }
            }

            InvalidateRect(PhSipWindow, NULL, TRUE);
        }
        break;
    }
}

VOID PhSiNotifyChangeSettings(
    VOID
    )
{
    HWND window;

    PhSipUpdateColorParameters();

    window = PhSipWindow;

    if (window)
        PostMessage(window, SI_MSG_SYSINFO_CHANGE_SETTINGS, 0, 0);
}

VOID PhSiSetColorsGraphDrawInfo(
    _Out_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ COLORREF Color1,
    _In_ COLORREF Color2,
    _In_ LONG dpiValue
    )
{
    static PH_QUEUED_LOCK lock = PH_QUEUED_LOCK_INIT;
    static ULONG lastDpi = ULONG_MAX;
    static HFONT iconTitleFont;

    // Get the appropriate fonts.

    if (DrawInfo->Flags & PH_GRAPH_LABEL_MAX_Y)
    {
        PhAcquireQueuedLockExclusive(&lock);

        if (lastDpi != dpiValue)
        {
            LOGFONT logFont;

            if (PhGetSystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &logFont, dpiValue))
            {
                logFont.lfHeight += PhMultiplyDivide(1, dpiValue, 72);
                iconTitleFont = CreateFontIndirect(&logFont);
            }

            if (!iconTitleFont)
                iconTitleFont = PhApplicationFont;

            lastDpi = dpiValue;
        }

        DrawInfo->LabelYFont = iconTitleFont;

        PhReleaseQueuedLockExclusive(&lock);
    }

    DrawInfo->TextFont = PhApplicationFont;

    // Set up the colors.

    switch (PhCsGraphColorMode)
    {
    case 0: // New colors
        DrawInfo->BackColor = RGB(0xef, 0xef, 0xef);
        DrawInfo->LineColor1 = PhHalveColorBrightness(Color1);
        DrawInfo->LineBackColor1 = PhMakeColorBrighter(Color1, 125);
        DrawInfo->LineColor2 = PhHalveColorBrightness(Color2);
        DrawInfo->LineBackColor2 = PhMakeColorBrighter(Color2, 125);
        DrawInfo->GridColor = RGB(0xc7, 0xc7, 0xc7);
        DrawInfo->LabelYColor = RGB(0xa0, 0x60, 0x20);
        DrawInfo->TextColor = RGB(0x00, 0x00, 0x00);
        DrawInfo->TextBoxColor = RGB(0xe7, 0xe7, 0xe7);
        break;
    case 1: // Old colors
        DrawInfo->BackColor = RGB(0x00, 0x00, 0x00);
        DrawInfo->LineColor1 = Color1;
        DrawInfo->LineBackColor1 = PhHalveColorBrightness(Color1);
        DrawInfo->LineColor2 = Color2;
        DrawInfo->LineBackColor2 = PhHalveColorBrightness(Color2);
        DrawInfo->GridColor = RGB(0x00, 0x57, 0x00);
        DrawInfo->LabelYColor = RGB(0xd0, 0xa0, 0x70);
        DrawInfo->TextColor = RGB(0x00, 0xff, 0x00);
        DrawInfo->TextBoxColor = RGB(0x00, 0x22, 0x00);
        break;
    }
}

PPH_STRING PhSiSizeLabelYFunction(
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ ULONG DataIndex,
    _In_ FLOAT Value,
    _In_ FLOAT Parameter
    )
{
    ULONG64 size;

    size = (ULONG64)((DOUBLE)Value * Parameter);

    if (size != 0)
    {
        PH_FORMAT format;

        format.Type = SizeFormatType | FormatUsePrecision | FormatUseRadix;
        format.Precision = 0;
        format.Radix = (UCHAR)PhMaxSizeUnit;
        format.u.Size = size;

        return PhFormat(&format, 1, 0);
    }
    else
    {
        return PhReferenceEmptyString();
    }
}

PPH_STRING PhSiDoubleLabelYFunction(
    _In_ PPH_GRAPH_DRAW_INFO DrawInfo,
    _In_ ULONG DataIndex,
    _In_ FLOAT Value,
    _In_ FLOAT Parameter
    )
{
    DOUBLE value;

    value = (DOUBLE)((DOUBLE)Value * Parameter);

    if (value != 0)
    {
        PH_FORMAT format[2];

        PhInitFormatF(&format[0], value * 100, PhMaxPrecisionUnit);
        PhInitFormatC(&format[1], L'%');

        return PhFormat(format, RTL_NUMBER_OF(format), 0);
    }
    else
    {
        return PhReferenceEmptyString();
    }
}

VOID PhSipRegisterDialog(
    _In_ HWND DialogWindowHandle
    )
{
    if (!PhSipDialogList)
        PhSipDialogList = PhCreateList(4);

    PhAddItemList(PhSipDialogList, DialogWindowHandle);
}

VOID PhSipUnregisterDialog(
    _In_ HWND DialogWindowHandle
    )
{
    ULONG index;

    if ((index = PhFindItemList(PhSipDialogList, DialogWindowHandle)) != ULONG_MAX)
        PhRemoveItemList(PhSipDialogList, index);
}

VOID PhSipInitializeParameters(
    VOID
    )
{
    LOGFONT logFont;
    HDC hdc;
    TEXTMETRIC textMetrics;
    HFONT originalFont;
    LONG dpiValue;

    PhSipDeleteParameters();

    memset(&CurrentParameters, 0, sizeof(PH_SYSINFO_PARAMETERS));

    dpiValue = PhGetWindowDpi(PhSipWindow);

    CurrentParameters.SysInfoWindowHandle = PhSipWindow;
    CurrentParameters.ContainerWindowHandle = ContainerControl;

    if (PhGetSystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &logFont, dpiValue))
    {
        CurrentParameters.Font = CreateFontIndirect(&logFont);
    }
    else
    {
        CurrentParameters.Font = PhApplicationFont;
        GetObject(PhApplicationFont, sizeof(LOGFONT), &logFont);
    }

    hdc = GetDC(PhSipWindow);

    logFont.lfHeight -= PhMultiplyDivide(3, dpiValue, 72);
    CurrentParameters.MediumFont = CreateFontIndirect(&logFont);

    logFont.lfHeight -= PhMultiplyDivide(3, dpiValue, 72);
    CurrentParameters.LargeFont = CreateFontIndirect(&logFont);

    PhSipUpdateColorParameters();

    CurrentParameters.ColorSetupFunction = PhSiSetColorsGraphDrawInfo;

    originalFont = SelectFont(hdc, CurrentParameters.Font);
    GetTextMetrics(hdc, &textMetrics);
    CurrentParameters.FontHeight = textMetrics.tmHeight;
    CurrentParameters.FontAverageWidth = textMetrics.tmAveCharWidth;

    SelectFont(hdc, CurrentParameters.MediumFont);
    GetTextMetrics(hdc, &textMetrics);
    CurrentParameters.MediumFontHeight = textMetrics.tmHeight;
    CurrentParameters.MediumFontAverageWidth = textMetrics.tmAveCharWidth;

    SelectFont(hdc, originalFont);

    // Internal padding and other values
    CurrentParameters.PanelPadding = PhGetDpi(PH_SYSINFO_PANEL_PADDING, dpiValue);
    CurrentParameters.WindowPadding = PhGetDpi(PH_SYSINFO_WINDOW_PADDING, dpiValue);
    CurrentParameters.GraphPadding = PhGetDpi(PH_SYSINFO_GRAPH_PADDING, dpiValue);
    CurrentParameters.SmallGraphWidth = PhGetDpi(PH_SYSINFO_SMALL_GRAPH_WIDTH, dpiValue);
    CurrentParameters.SmallGraphPadding = PhGetDpi(PH_SYSINFO_SMALL_GRAPH_PADDING, dpiValue);
    CurrentParameters.SeparatorWidth = PhGetDpi(PH_SYSINFO_SEPARATOR_WIDTH, dpiValue);
    CurrentParameters.CpuPadding = PhGetDpi(PH_SYSINFO_CPU_PADDING, dpiValue);
    CurrentParameters.MemoryPadding = PhGetDpi(PH_SYSINFO_MEMORY_PADDING, dpiValue);

    CurrentParameters.MinimumGraphHeight =
        CurrentParameters.PanelPadding +
        CurrentParameters.MediumFontHeight +
        CurrentParameters.PanelPadding;

    CurrentParameters.SectionViewGraphHeight =
        CurrentParameters.PanelPadding +
        CurrentParameters.MediumFontHeight +
        CurrentParameters.PanelPadding +
        CurrentParameters.FontHeight +
        2 +
        CurrentParameters.FontHeight +
        CurrentParameters.PanelPadding +
        2;

    CurrentParameters.PanelWidth = CurrentParameters.MediumFontAverageWidth * 10;

    ReleaseDC(PhSipWindow, hdc);
}

VOID PhSipDeleteParameters(
    VOID
    )
{
    if (CurrentParameters.Font)
        DeleteFont(CurrentParameters.Font);
    if (CurrentParameters.MediumFont)
        DeleteFont(CurrentParameters.MediumFont);
    if (CurrentParameters.LargeFont)
        DeleteFont(CurrentParameters.LargeFont);
}

VOID PhSipUpdateColorParameters(
    VOID
    )
{
    switch (PhCsGraphColorMode)
    {
    case 0: // New colors
        CurrentParameters.GraphBackColor = RGB(0xef, 0xef, 0xef);
        CurrentParameters.PanelForeColor = RGB(0x00, 0x00, 0x00);
        break;
    case 1: // Old colors
        CurrentParameters.GraphBackColor = RGB(0x00, 0x00, 0x00);
        CurrentParameters.PanelForeColor = RGB(0xff, 0xff, 0xff);
        break;
    }
}

PPH_SYSINFO_SECTION PhSipCreateSection(
    _In_ PPH_SYSINFO_SECTION Template
    )
{
    PPH_SYSINFO_SECTION section;
    PH_GRAPH_OPTIONS options;

    section = PhAllocateZero(sizeof(PH_SYSINFO_SECTION));
    section->Name = Template->Name;
    section->Flags = Template->Flags;
    section->Callback = Template->Callback;
    section->Context = Template->Context;

    section->GraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | GC_STYLE_FADEOUT | GC_STYLE_DRAW_PANEL,
        0,
        0,
        3,
        3,
        PhSipWindow,
        NULL,
        PhInstanceHandle,
        NULL
        );
    PhInitializeGraphState(&section->GraphState);
    section->Parameters = &CurrentParameters;

    Graph_GetOptions(section->GraphHandle, &options);
    options.FadeOutBackColor = CurrentParameters.GraphBackColor;
    options.FadeOutWidth = CurrentParameters.PanelWidth + PH_SYSINFO_FADE_ADD;
    options.DefaultCursor = LoadCursor(NULL, IDC_HAND);
    Graph_SetOptions(section->GraphHandle, &options);
    if (PhEnableTooltipSupport) Graph_SetTooltip(section->GraphHandle, TRUE);

    section->PanelId = IDDYNAMIC + SectionList->Count * 2 + 2;
    section->PanelHandle = CreateWindow(
        WC_STATIC,
        NULL,
        WS_CHILD | SS_OWNERDRAW | SS_NOTIFY,
        0,
        0,
        3,
        3,
        PhSipWindow,
        UlongToHandle(section->PanelId),
        PhInstanceHandle,
        NULL
        );

    section->GraphWindowProc = (WNDPROC)GetWindowLongPtr(section->GraphHandle, GWLP_WNDPROC);
    section->PanelWindowProc = (WNDPROC)GetWindowLongPtr(section->PanelHandle, GWLP_WNDPROC);

    PhSetWindowContext(section->GraphHandle, 0xF, section);
    PhSetWindowContext(section->PanelHandle, 0xF, section);

    SetWindowLongPtr(section->GraphHandle, GWLP_WNDPROC, (LONG_PTR)PhSipGraphHookWndProc);
    SetWindowLongPtr(section->PanelHandle, GWLP_WNDPROC, (LONG_PTR)PhSipPanelHookWndProc);

    PhAddItemList(SectionList, section);

    section->Callback(section, SysInfoCreate, NULL, NULL);

    return section;
}

VOID PhSipDestroySection(
    _In_ PPH_SYSINFO_SECTION Section
    )
{
    Section->Callback(Section, SysInfoDestroy, NULL, NULL);

    PhDeleteGraphState(&Section->GraphState);
    PhFree(Section);
}

PPH_SYSINFO_SECTION PhSipFindSection(
    _In_ PPH_STRINGREF Name
    )
{
    ULONG i;
    PPH_SYSINFO_SECTION section;

    for (i = 0; i < SectionList->Count; i++)
    {
        section = SectionList->Items[i];

        if (PhEqualStringRef(&section->Name, Name, TRUE))
            return section;
    }

    return NULL;
}

PPH_SYSINFO_SECTION PhSipCreateInternalSection(
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_ PPH_SYSINFO_SECTION_CALLBACK Callback
    )
{
    PH_SYSINFO_SECTION section;

    memset(&section, 0, sizeof(PH_SYSINFO_SECTION));
    PhInitializeStringRef(&section.Name, Name);
    section.Flags = Flags;
    section.Callback = Callback;

    return PhSipCreateSection(&section);
}

VOID PhSipDrawRestoreSummaryPanel(
    _In_ PDRAWITEMSTRUCT DrawItemStruct
    )
{
    HDC hdc;
    HDC bufferDc;
    HBITMAP bufferBitmap;
    HBITMAP oldBufferBitmap;
    RECT bufferRect =
    {
        0, 0,
        DrawItemStruct->rcItem.right - DrawItemStruct->rcItem.left,
        DrawItemStruct->rcItem.bottom - DrawItemStruct->rcItem.top
    };

    if (!(hdc = GetDC(DrawItemStruct->hwndItem)))
        return;

    bufferDc = CreateCompatibleDC(hdc);
    bufferBitmap = CreateCompatibleBitmap(hdc, bufferRect.right, bufferRect.bottom);
    oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);

    SetBkMode(bufferDc, TRANSPARENT);

    if (PhEnableThemeSupport)
    {
        switch (PhCsGraphColorMode)
        {
        case 0: // New colors
            SetTextColor(bufferDc, RGB(0x00, 0x00, 0x00));
            SetDCBrushColor(bufferDc, RGB(0xff, 0xff, 0xff));
            FillRect(bufferDc, &bufferRect, GetStockBrush(DC_BRUSH));
            break;
        case 1: // Old colors
            SetTextColor(bufferDc, PhThemeWindowTextColor);
            SetDCBrushColor(bufferDc, PhThemeWindowBackgroundColor);// RGB(43, 43, 43));
            FillRect(bufferDc, &bufferRect, GetStockBrush(DC_BRUSH));
            break;
        }
    }
    else
    {
        SetTextColor(bufferDc, GetSysColor(COLOR_WINDOWTEXT));
        FillRect(bufferDc, &bufferRect, GetSysColorBrush(COLOR_WINDOW));
    }

    if (RestoreSummaryControlHot || RestoreSummaryControlHasFocus)
    {
        if (ThemeHasItemBackground)
        {
            PhDrawThemeBackground(
                ThemeData,
                bufferDc,
                TVP_TREEITEM,
                TREIS_HOT,
                &bufferRect,
                &bufferRect
                );
        }
        else
        {
            FillRect(bufferDc, &bufferRect, GetSysColorBrush(COLOR_WINDOW));
        }
    }

    SelectFont(bufferDc, CurrentParameters.MediumFont);
    DrawText(bufferDc, L"Back", 4, &bufferRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    BitBlt(
        hdc,
        DrawItemStruct->rcItem.left,
        DrawItemStruct->rcItem.top,
        DrawItemStruct->rcItem.right,
        DrawItemStruct->rcItem.bottom,
        bufferDc,
        0,
        0,
        SRCCOPY
        );

    SelectBitmap(bufferDc, oldBufferBitmap);
    DeleteBitmap(bufferBitmap);
    DeleteDC(bufferDc);

    ReleaseDC(DrawItemStruct->hwndItem, hdc);
}

VOID PhSipDrawSeparator(
    _In_ PDRAWITEMSTRUCT DrawItemStruct
    )
{
    HDC hdc;
    HDC bufferDc;
    HBITMAP bufferBitmap;
    HBITMAP oldBufferBitmap;
    RECT bufferRect =
    {
        0, 0,
        DrawItemStruct->rcItem.right - DrawItemStruct->rcItem.left,
        DrawItemStruct->rcItem.bottom - DrawItemStruct->rcItem.top
    };

    if (!(hdc = GetDC(DrawItemStruct->hwndItem)))
        return;

    bufferDc = CreateCompatibleDC(hdc);
    bufferBitmap = CreateCompatibleBitmap(hdc, bufferRect.right, bufferRect.bottom);
    oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);

    SetBkMode(bufferDc, TRANSPARENT);

    if (PhEnableThemeSupport)
    {
        switch (PhCsGraphColorMode)
        {
        case 0: // New colors
            {
                //FillRect(bufferDc, &bufferRect, GetSysColorBrush(COLOR_3DFACE));
                //bufferRect.left += 1;
                //FillRect(bufferDc, &bufferRect, GetSysColorBrush(COLOR_3DSHADOW));
                //bufferRect.left -= 1;
            }
            break;
        case 1: // Old colors
            {
                SetDCBrushColor(bufferDc, RGB(43, 43, 43));
                FillRect(bufferDc, &bufferRect, GetStockBrush(DC_BRUSH));
            }
            break;
        }
    }
    else
    {
        //FillRect(bufferDc, &bufferRect, GetSysColorBrush(COLOR_3DHIGHLIGHT));
        //bufferRect.left += 1;
        //FillRect(bufferDc, &bufferRect, GetSysColorBrush(COLOR_3DSHADOW));
        //bufferRect.left -= 1;

        SetDCBrushColor(bufferDc, RGB(0xff, 0xff, 0xff));
        FillRect(bufferDc, &bufferRect, GetStockBrush(DC_BRUSH));
    }

    BitBlt(
        hdc,
        DrawItemStruct->rcItem.left,
        DrawItemStruct->rcItem.top,
        DrawItemStruct->rcItem.right,
        DrawItemStruct->rcItem.bottom,
        bufferDc,
        0,
        0,
        SRCCOPY
        );

    SelectBitmap(bufferDc, oldBufferBitmap);
    DeleteBitmap(bufferBitmap);
    DeleteDC(bufferDc);

    ReleaseDC(DrawItemStruct->hwndItem, hdc);
}

VOID PhSipDrawPanel(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ HDC hdc,
    _In_ PRECT Rect
    )
{
    PH_SYSINFO_DRAW_PANEL sysInfoDrawPanel;

    if (CurrentView == SysInfoSectionView)
    {
        if (PhEnableThemeSupport)
        {
            switch (PhCsGraphColorMode)
            {
            case 0: // New colors
                SetDCBrushColor(hdc, RGB(0xff, 0xff, 0xff));
                FillRect(hdc, Rect, GetStockBrush(DC_BRUSH));
                break;
            case 1: // Old colors
                SetDCBrushColor(hdc, PhThemeWindowBackgroundColor);
                FillRect(hdc, Rect, GetStockBrush(DC_BRUSH));
                break;
            }
        }
        else
        {
            SetDCBrushColor(hdc, RGB(255, 255, 255));
            FillRect(hdc, Rect, GetStockBrush(DC_BRUSH));
        }

        //if (PhEnableThemeSupport)
        //{
        //    switch (PhCsGraphColorMode)
        //    {
        //    case 0: // New colors
        //        FillRect(hdc, Rect, GetSysColorBrush(COLOR_3DFACE));
        //        break;
        //    case 1: // Old colors
        //        SetDCBrushColor(hdc, RGB(30, 30, 30));
        //        FillRect(hdc, Rect, GetStockBrush(DC_BRUSH));
        //        break;
        //    }
        //}
        //else
        //{
        //    FillRect(hdc, Rect, GetSysColorBrush(COLOR_3DFACE));
        //}
    }

    sysInfoDrawPanel.hdc = hdc;
    sysInfoDrawPanel.Rect = *Rect;
    sysInfoDrawPanel.Rect.right = CurrentParameters.PanelWidth;
    sysInfoDrawPanel.CustomDraw = FALSE;

    sysInfoDrawPanel.Title = NULL;
    sysInfoDrawPanel.SubTitle = NULL;
    sysInfoDrawPanel.SubTitleOverflow = NULL;

    Section->Callback(Section, SysInfoGraphDrawPanel, &sysInfoDrawPanel, NULL);

    if (!sysInfoDrawPanel.CustomDraw)
    {
        sysInfoDrawPanel.Rect.right = Rect->right;
        PhSipDefaultDrawPanel(Section, &sysInfoDrawPanel);
    }

    PhClearReference(&sysInfoDrawPanel.Title);
    PhClearReference(&sysInfoDrawPanel.SubTitle);
    PhClearReference(&sysInfoDrawPanel.SubTitleOverflow);
}

VOID PhSipDefaultDrawPanel(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ PPH_SYSINFO_DRAW_PANEL DrawPanel
    )
{
    HDC hdc;
    RECT rect;
    ULONG flags;

    hdc = DrawPanel->hdc;

    if (ThemeHasItemBackground)
    {
        if (CurrentView == SysInfoSummaryView)
        {
            //if (Section->GraphHot)
            //{
            //    PhDrawThemeBackground(
            //        ThemeData,
            //        hdc,
            //        TVP_TREEITEM,
            //        TREIS_HOT,
            //        &DrawPanel->Rect,
            //        &DrawPanel->Rect
            //        );
            //}
        }
        else if (CurrentView == SysInfoSectionView)
        {
            INT stateId;

            stateId = -1;

            if (Section->GraphHot || Section->PanelHot || (Section->HasFocus && !Section->HideFocus))
            {
                if (Section == CurrentSection)
                    stateId = TREIS_HOTSELECTED;
                else
                    stateId = TREIS_HOT;
            }
            else if (Section == CurrentSection)
            {
                stateId = TREIS_SELECTED;
            }

            if (stateId != -1)
            {
                RECT themeRect;

                themeRect = DrawPanel->Rect;
                themeRect.left -= 2; // remove left edge

                PhDrawThemeBackground(
                    ThemeData,
                    hdc,
                    TVP_TREEITEM,
                    stateId,
                    &themeRect,
                    &DrawPanel->Rect
                    );
            }
        }
        else if (Section->HasFocus)
        {
            PhDrawThemeBackground(
                ThemeData,
                hdc,
                TVP_TREEITEM,
                TREIS_HOT,
                &DrawPanel->Rect,
                &DrawPanel->Rect
                );
        }
    }
    else
    {
        if (CurrentView == SysInfoSectionView)
        {
            //HBRUSH brush = NULL;

            if (Section->GraphHot || Section->PanelHot || Section->HasFocus)
            {
                //brush = GetSysColorBrush(COLOR_WINDOW); // TODO: Use a different color
            }
            else if (Section == CurrentSection)
            {
                //brush = GetSysColorBrush(COLOR_WINDOW);
            }

            //if (brush)
            //{
            //    FillRect(hdc, &DrawPanel->Rect, brush);
            //}
        }
    }

    SetBkMode(hdc, TRANSPARENT);

    if (PhEnableThemeSupport)
    {
        switch (PhCsGraphColorMode)
        {
        case 0: // New colors
            SetTextColor(hdc, RGB(0x00, 0x00, 0x00));
            //SetDCBrushColor(hdc, RGB(0xff, 0xff, 0xff));
            //FillRect(hdc, Rect, GetStockBrush(DC_BRUSH));
            break;
        case 1: // Old colors
            SetTextColor(hdc, RGB(0xff, 0xff, 0xff));
            //SetDCBrushColor(hdc, RGB(0xff, 0xff, 0x00));
            //FillRect(hdc, Rect, GetStockBrush(DC_BRUSH));
            break;
        }

        //SetTextColor(hdc, CurrentParameters.PanelForeColor);
    }
    else
    {
        //SetTextColor(hdc, RGB(0x00, 0x00, 0x00));

        if (CurrentView == SysInfoSummaryView)
        {
            SetTextColor(hdc, CurrentParameters.PanelForeColor);
        }
        else
        {
            SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
        }
    }

    rect.left = CurrentParameters.SmallGraphPadding + CurrentParameters.PanelPadding;
    rect.top = CurrentParameters.PanelPadding;
    rect.right = CurrentParameters.PanelWidth;
    rect.bottom = DrawPanel->Rect.bottom;

    flags = DT_NOPREFIX;

    if (CurrentView == SysInfoSummaryView)
        rect.right = DrawPanel->Rect.right; // allow the text to overflow
    else
        flags |= DT_END_ELLIPSIS;

    if (DrawPanel->Title)
    {
        SelectFont(hdc, CurrentParameters.MediumFont);
        DrawText(hdc, DrawPanel->Title->Buffer, (ULONG)DrawPanel->Title->Length / sizeof(WCHAR), &rect, flags | DT_SINGLELINE);
    }

    if (DrawPanel->SubTitle)
    {
        RECT measureRect;
        SIZE textSize;
        LONG lineHeight;
        LONG lineWidth;
        LONG rectHeight;
        LONG rectWidth;
        LONG textHeight;
        LONG textWidth;

        rect.top += CurrentParameters.MediumFontHeight + CurrentParameters.PanelPadding;
        measureRect = rect;

        SelectFont(hdc, CurrentParameters.Font);
        GetTextExtentPoint32(hdc, DrawPanel->SubTitle->Buffer, (ULONG)DrawPanel->SubTitle->Length / sizeof(WCHAR), &textSize);
        DrawText(hdc, DrawPanel->SubTitle->Buffer, (ULONG)DrawPanel->SubTitle->Length / sizeof(WCHAR), &measureRect, flags | DT_CALCRECT);

        lineHeight = textSize.cy;
        lineWidth = textSize.cx;
        rectHeight = rect.bottom - rect.top;
        rectWidth = rect.right - rect.left;
        textHeight = measureRect.bottom - measureRect.top;
        textWidth = measureRect.right - measureRect.left;
        //dprintf(
        //    "[rectHeight: %u, rectwidth: %u] [lineHeight: %u, lineWidth: %u] [textHeight: %u, textWidth: %u]\n",
        //    rectHeight, rectWidth,
        //    lineHeight, lineWidth,
        //    textHeight, textWidth
        //    );

        if (rectHeight > lineHeight)
        {
            if (rectHeight > textHeight)
            {
                if (lineWidth < rectWidth || !DrawPanel->SubTitleOverflow)
                {
                    // Text fits; draw normally.
                    DrawText(hdc, DrawPanel->SubTitle->Buffer, (ULONG)DrawPanel->SubTitle->Length / sizeof(WCHAR), &rect, flags);
                }
                else
                {
                    // Text doesn't fit; draw the alternative text.
                    DrawText(hdc, DrawPanel->SubTitleOverflow->Buffer, (ULONG)DrawPanel->SubTitleOverflow->Length / sizeof(WCHAR), &rect, flags);
                }
            }
            else
            {
                PH_STRINGREF titlePart;
                PH_STRINGREF remainingPart;

                // dmex: Multiline text doesn't fit; split the string and draw the first line.
                if (DrawPanel->SubTitleOverflow)
                {
                    if (PhSplitStringRefAtChar(&DrawPanel->SubTitleOverflow->sr, L'\n', &titlePart, &remainingPart))
                        DrawText(hdc, titlePart.Buffer, (ULONG)titlePart.Length / sizeof(WCHAR), &rect, flags);
                    else
                        DrawText(hdc, DrawPanel->SubTitleOverflow->Buffer, (ULONG)DrawPanel->SubTitleOverflow->Length / sizeof(WCHAR), &rect, flags);
                }
                else
                {
                    if (PhSplitStringRefAtChar(&DrawPanel->SubTitle->sr, L'\n', &titlePart, &remainingPart))
                        DrawText(hdc, titlePart.Buffer, (ULONG)titlePart.Length / sizeof(WCHAR), &rect, flags);
                    else
                        DrawText(hdc, DrawPanel->SubTitle->Buffer, (ULONG)DrawPanel->SubTitle->Length / sizeof(WCHAR), &rect, flags);
                }
            }
        }
    }
}

VOID PhSipLayoutSummaryView(
    VOID
    )
{
    RECT clientRect;
    ULONG availableHeight;
    ULONG availableWidth;
    ULONG graphHeight;
    ULONG i;
    PPH_SYSINFO_SECTION section;
    HDWP deferHandle;
    ULONG y;

    GetClientRect(PhSipWindow, &clientRect);

    availableHeight = clientRect.bottom - CurrentParameters.WindowPadding * 2;
    availableWidth = clientRect.right - CurrentParameters.WindowPadding * 2;
    graphHeight = (availableHeight - CurrentParameters.GraphPadding * (SectionList->Count - 1)) / SectionList->Count;

    deferHandle = BeginDeferWindowPos(SectionList->Count);
    y = CurrentParameters.WindowPadding;

    for (i = 0; i < SectionList->Count; i++)
    {
        section = SectionList->Items[i];

        deferHandle = DeferWindowPos(
            deferHandle,
            section->GraphHandle,
            NULL,
            CurrentParameters.WindowPadding,
            y,
            availableWidth,
            graphHeight,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
        y += graphHeight + CurrentParameters.GraphPadding;

        section->GraphState.Valid = FALSE;
    }

    EndDeferWindowPos(deferHandle);
}

VOID PhSipLayoutSectionView(
    VOID
    )
{
    RECT clientRect;
    ULONG availableHeight;
    ULONG availableWidth;
    ULONG graphHeight;
    ULONG i;
    PPH_SYSINFO_SECTION section;
    HDWP deferHandle;
    ULONG y;
    ULONG containerLeft;

    GetClientRect(PhSipWindow, &clientRect);

    availableHeight = clientRect.bottom - CurrentParameters.WindowPadding * 2;
    availableWidth = clientRect.right - CurrentParameters.WindowPadding * 2;
    graphHeight = (availableHeight - CurrentParameters.SmallGraphPadding * SectionList->Count) / (SectionList->Count + 1);

    if (graphHeight > CurrentParameters.SectionViewGraphHeight)
        graphHeight = CurrentParameters.SectionViewGraphHeight;

    deferHandle = BeginDeferWindowPos(SectionList->Count * 2 + 3);
    y = CurrentParameters.WindowPadding;

    for (i = 0; i < SectionList->Count; i++)
    {
        section = SectionList->Items[i];

        deferHandle = DeferWindowPos(
            deferHandle,
            section->GraphHandle,
            NULL,
            CurrentParameters.WindowPadding,
            y,
            CurrentParameters.SmallGraphWidth,
            graphHeight,
            SWP_NOACTIVATE | SWP_NOZORDER
            );

        deferHandle = DeferWindowPos(
            deferHandle,
            section->PanelHandle,
            NULL,
            CurrentParameters.WindowPadding + CurrentParameters.SmallGraphWidth,
            y,
            CurrentParameters.SmallGraphPadding + CurrentParameters.PanelWidth,
            graphHeight,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
        InvalidateRect(section->PanelHandle, NULL, TRUE);

        y += graphHeight + CurrentParameters.SmallGraphPadding;

        section->GraphState.Valid = FALSE;
    }

    deferHandle = DeferWindowPos(
        deferHandle,
        RestoreSummaryControl,
        NULL,
        CurrentParameters.WindowPadding,
        y,
        CurrentParameters.SmallGraphWidth + CurrentParameters.SmallGraphPadding + CurrentParameters.PanelWidth,
        graphHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );
    InvalidateRect(RestoreSummaryControl, NULL, TRUE);

    deferHandle = DeferWindowPos(
        deferHandle,
        SeparatorControl,
        NULL,
        CurrentParameters.WindowPadding + CurrentParameters.SmallGraphWidth + CurrentParameters.SmallGraphPadding + CurrentParameters.PanelWidth + CurrentParameters.WindowPadding - 7,
        0,
        CurrentParameters.SeparatorWidth,
        CurrentParameters.WindowPadding + availableHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    containerLeft = CurrentParameters.WindowPadding + CurrentParameters.SmallGraphWidth + CurrentParameters.SmallGraphPadding + CurrentParameters.PanelWidth + CurrentParameters.WindowPadding - 7 + CurrentParameters.SeparatorWidth;
    deferHandle = DeferWindowPos(
        deferHandle,
        ContainerControl,
        NULL,
        containerLeft,
        0,
        clientRect.right - containerLeft,
        CurrentParameters.WindowPadding + availableHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    EndDeferWindowPos(deferHandle);

    if (CurrentSection && CurrentSection->DialogHandle)
    {
        SetWindowPos(
            CurrentSection->DialogHandle,
            NULL,
            CurrentParameters.WindowPadding,
            CurrentParameters.WindowPadding,
            clientRect.right - containerLeft - CurrentParameters.WindowPadding - CurrentParameters.WindowPadding,
            availableHeight,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
    }
}

VOID PhSipEnterSectionView(
    _In_ PPH_SYSINFO_SECTION NewSection
    )
{
    ULONG i;
    PPH_SYSINFO_SECTION section;
    BOOLEAN fromSummaryView;
    PPH_SYSINFO_SECTION oldSection;
    HDWP deferHandle;
    HDWP containerDeferHandle;

    if (CurrentSection == NewSection)
        return;

    fromSummaryView = CurrentView == SysInfoSummaryView;
    CurrentView = SysInfoSectionView;
    oldSection = CurrentSection;
    CurrentSection = NewSection;

    deferHandle = BeginDeferWindowPos(SectionList->Count + 3);
    containerDeferHandle = BeginDeferWindowPos(SectionList->Count);

    PhSipEnterSectionViewInner(NewSection, fromSummaryView, &deferHandle, &containerDeferHandle);
    PhSipLayoutSectionView();

    for (i = 0; i < SectionList->Count; i++)
    {
        section = SectionList->Items[i];

        if (section != NewSection)
            PhSipEnterSectionViewInner(section, fromSummaryView, &deferHandle, &containerDeferHandle);
    }

    deferHandle = DeferWindowPos(deferHandle, ContainerControl, NULL, 0, 0, 0, 0, SWP_SHOWWINDOW_ONLY);
    deferHandle = DeferWindowPos(deferHandle, RestoreSummaryControl, NULL, 0, 0, 0, 0, SWP_SHOWWINDOW_ONLY);
    deferHandle = DeferWindowPos(deferHandle, SeparatorControl, NULL, 0, 0, 0, 0, SWP_SHOWWINDOW_ONLY);

    EndDeferWindowPos(deferHandle);
    EndDeferWindowPos(containerDeferHandle);

    if (oldSection)
        InvalidateRect(oldSection->PanelHandle, NULL, TRUE);

    InvalidateRect(NewSection->PanelHandle, NULL, TRUE);

    if (NewSection->DialogHandle)
        RedrawWindow(NewSection->DialogHandle, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

VOID PhSipEnterSectionViewInner(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ BOOLEAN FromSummaryView,
    _Inout_ HDWP *DeferHandle,
    _Inout_ HDWP *ContainerDeferHandle
    )
{
    Section->HasFocus = FALSE;
    Section->Callback(Section, SysInfoViewChanging, (PVOID)CurrentView, CurrentSection);

    if (FromSummaryView)
    {
        PhSetWindowStyle(Section->GraphHandle, GC_STYLE_FADEOUT | GC_STYLE_DRAW_PANEL, 0);
        *DeferHandle = DeferWindowPos(*DeferHandle, Section->PanelHandle, NULL, 0, 0, 0, 0, SWP_SHOWWINDOW_ONLY);
    }

    if (Section == CurrentSection && !Section->DialogHandle)
        PhSipCreateSectionDialog(Section);

    if (Section->DialogHandle)
    {
        if (Section == CurrentSection)
            *ContainerDeferHandle = DeferWindowPos(*ContainerDeferHandle, Section->DialogHandle, NULL, 0, 0, 0, 0, SWP_SHOWWINDOW_ONLY | SWP_NOREDRAW);
        else
            *ContainerDeferHandle = DeferWindowPos(*ContainerDeferHandle, Section->DialogHandle, NULL, 0, 0, 0, 0, SWP_HIDEWINDOW_ONLY | SWP_NOREDRAW);
    }
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

    ShowWindow(ContainerControl, SW_HIDE);
    ShowWindow(RestoreSummaryControl, SW_HIDE);
    ShowWindow(SeparatorControl, SW_HIDE);

    PhSipLayoutSummaryView();
}

VOID PhSipCreateSectionDialog(
    _In_ PPH_SYSINFO_SECTION Section
    )
{
    PH_SYSINFO_CREATE_DIALOG createDialog;

    memset(&createDialog, 0, sizeof(PH_SYSINFO_CREATE_DIALOG));

    if (Section->Callback(Section, SysInfoCreateDialog, &createDialog, NULL))
    {
        if (!createDialog.CustomCreate)
        {
            Section->DialogHandle = PhCreateDialogFromTemplate(
                ContainerControl,
                DS_SETFONT | DS_FIXEDSYS | DS_CONTROL | WS_CHILD,
                createDialog.Instance,
                createDialog.Template,
                createDialog.DialogProc,
                createDialog.Parameter
                );

            if (PhEnableThemeSupport)
            {
                PhInitializeWindowTheme(Section->DialogHandle, PhEnableThemeSupport);
            }
        }
    }
}

LRESULT CALLBACK PhSipGraphHookWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_SYSINFO_SECTION section;
    WNDPROC oldWndProc;

    if (!(section = PhGetWindowContext(hwnd, 0xF)))
        return 0;

    oldWndProc = section->GraphWindowProc;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(hwnd, 0xF);
        }
        break;
    case WM_SETFOCUS:
        section->HasFocus = TRUE;

        if (CurrentView == SysInfoSummaryView)
        {
            Graph_Draw(section->GraphHandle);
            InvalidateRect(section->GraphHandle, NULL, FALSE);
        }
        else
        {
            InvalidateRect(section->PanelHandle, NULL, TRUE);
        }

        break;
    case WM_KILLFOCUS:
        section->HasFocus = FALSE;

        if (CurrentView == SysInfoSummaryView)
        {
            Graph_Draw(section->GraphHandle);
            InvalidateRect(section->GraphHandle, NULL, FALSE);
        }
        else
        {
            InvalidateRect(section->PanelHandle, NULL, TRUE);
        }

        break;
    case WM_GETDLGCODE:
        if (wParam == VK_SPACE || wParam == VK_RETURN ||
            wParam == VK_UP || wParam == VK_DOWN ||
            wParam == VK_LEFT || wParam == VK_RIGHT)
            return DLGC_WANTALLKEYS;
        break;
    case WM_KEYDOWN:
        {
            if (wParam == VK_SPACE || wParam == VK_RETURN)
            {
                PhSipEnterSectionView(section);
            }
            else if (wParam == VK_UP || wParam == VK_LEFT ||
                wParam == VK_DOWN || wParam == VK_RIGHT)
            {
                ULONG i;

                for (i = 0; i < SectionList->Count; i++)
                {
                    if (SectionList->Items[i] == section)
                    {
                        if ((wParam == VK_UP || wParam == VK_LEFT) && i > 0)
                        {
                            SetFocus(((PPH_SYSINFO_SECTION)SectionList->Items[i - 1])->GraphHandle);
                        }
                        else if (wParam == VK_DOWN || wParam == VK_RIGHT)
                        {
                            if (i < SectionList->Count - 1)
                                SetFocus(((PPH_SYSINFO_SECTION)SectionList->Items[i + 1])->GraphHandle);
                            else
                                SetFocus(RestoreSummaryControl);
                        }

                        break;
                    }
                }
            }
        }
        break;
    case WM_MOUSEMOVE:
        {
            TRACKMOUSEEVENT trackMouseEvent;

            trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
            trackMouseEvent.dwFlags = TME_LEAVE;
            trackMouseEvent.hwndTrack = hwnd;
            trackMouseEvent.dwHoverTime = 0;
            TrackMouseEvent(&trackMouseEvent);

            if (!(section->GraphHot || section->PanelHot))
            {
                section->GraphHot = TRUE;
            }
            else
            {
                section->GraphHot = TRUE;
            }

            Graph_Draw(section->GraphHandle);
            InvalidateRect(section->GraphHandle, NULL, TRUE);
            InvalidateRect(section->PanelHandle, NULL, TRUE);
        }
        break;
    case WM_MOUSELEAVE:
        {
            if (!section->PanelHot)
            {
                section->GraphHot = FALSE;
                InvalidateRect(section->PanelHandle, NULL, TRUE);
            }
            else
            {
                section->GraphHot = FALSE;
            }

            section->HasFocus = FALSE;

            if (CurrentView == SysInfoSummaryView)
            {
                Graph_Draw(section->GraphHandle);
                InvalidateRect(section->GraphHandle, NULL, FALSE);
            }
        }
        break;
    case WM_NCLBUTTONDOWN:
        {
            PhSipEnterSectionView(section);
        }
        break;
    case WM_UPDATEUISTATE:
        {
            switch (LOWORD(wParam))
            {
            case UIS_SET:
                if (HIWORD(wParam) & UISF_HIDEFOCUS)
                    section->HideFocus = TRUE;
                break;
            case UIS_CLEAR:
                if (HIWORD(wParam) & UISF_HIDEFOCUS)
                    section->HideFocus = FALSE;
                break;
            case UIS_INITIALIZE:
                section->HideFocus = !!(HIWORD(wParam) & UISF_HIDEFOCUS);
                break;
            }
        }
        break;
    }

    return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK PhSipPanelHookWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_SYSINFO_SECTION section;
    WNDPROC oldWndProc;

    section = PhGetWindowContext(hwnd, 0xF);

    if (section)
        oldWndProc = section->PanelWindowProc;
    else
        oldWndProc = RestoreSummaryControlOldWndProc;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(hwnd, 0xF);
        }
        break;
    case WM_SETFOCUS:
        {
            if (!section)
            {
                RestoreSummaryControlHasFocus = TRUE;
                InvalidateRect(hwnd, NULL, TRUE);
            }
        }
        break;
    case WM_KILLFOCUS:
        {
            if (!section)
            {
                RestoreSummaryControlHasFocus = FALSE;
                InvalidateRect(hwnd, NULL, TRUE);
            }
        }
        break;
    case WM_SETCURSOR:
        {
            SetCursor(LoadCursor(NULL, IDC_HAND));
        }
        return TRUE;
    case WM_GETDLGCODE:
        if (wParam == VK_SPACE || wParam == VK_RETURN ||
            wParam == VK_UP || wParam == VK_DOWN ||
            wParam == VK_LEFT || wParam == VK_RIGHT)
            return DLGC_WANTALLKEYS;
        break;
    case WM_KEYDOWN:
        {
            if (wParam == VK_SPACE || wParam == VK_RETURN)
            {
                if (section)
                    PhSipEnterSectionView(section);
                else
                    PhSipRestoreSummaryView();
            }
            else if (wParam == VK_UP || wParam == VK_LEFT)
            {
                if (!section && SectionList->Count != 0)
                {
                    SetFocus(((PPH_SYSINFO_SECTION)SectionList->Items[SectionList->Count - 1])->GraphHandle);
                }
            }
        }
        break;
    case WM_MOUSEMOVE:
        {
            TRACKMOUSEEVENT trackMouseEvent;

            trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
            trackMouseEvent.dwFlags = TME_LEAVE;
            trackMouseEvent.hwndTrack = hwnd;
            trackMouseEvent.dwHoverTime = 0;
            TrackMouseEvent(&trackMouseEvent);

            if (section)
            {
                if (!(section->GraphHot || section->PanelHot))
                {
                    section->PanelHot = TRUE;
                    InvalidateRect(section->PanelHandle, NULL, TRUE);
                }
                else
                {
                    section->PanelHot = TRUE;
                }
            }
            else
            {
                RestoreSummaryControlHot = TRUE;
                InvalidateRect(RestoreSummaryControl, NULL, TRUE);
            }
        }
        break;
    case WM_MOUSELEAVE:
        {
            if (section)
            {
                section->HasFocus = FALSE;

                if (!section->GraphHot)
                {
                    section->PanelHot = FALSE;
                    InvalidateRect(section->PanelHandle, NULL, TRUE);
                }
                else
                {
                    section->PanelHot = FALSE;
                }
            }
            else
            {
                RestoreSummaryControlHasFocus = FALSE;
                RestoreSummaryControlHot = FALSE;
                InvalidateRect(RestoreSummaryControl, NULL, TRUE);
            }
        }
        break;
    }

    return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
}

VOID PhSipUpdateThemeData(
    VOID
    )
{
    LONG dpi = PhGetWindowDpi(PhSipWindow);

    if (ThemeData)
    {
        PhCloseThemeData(ThemeData);
        ThemeData = NULL;
    }

    ThemeData = PhOpenThemeData(PhSipWindow, VSCLASS_TREEVIEW, dpi);

    if (ThemeData)
    {
        ThemeHasItemBackground = PhIsThemePartDefined(ThemeData, TVP_TREEITEM, 0);
    }
    else
    {
        ThemeHasItemBackground = FALSE;
    }
}

VOID PhSipSaveWindowState(
    VOID
    )
{
    WINDOWPLACEMENT placement = { sizeof(placement) };

    GetWindowPlacement(PhSipWindow, &placement);

    if (placement.showCmd == SW_NORMAL)
        PhSetIntegerSetting(L"SysInfoWindowState", SW_NORMAL);
    else if (placement.showCmd == SW_MAXIMIZE)
        PhSetIntegerSetting(L"SysInfoWindowState", SW_MAXIMIZE);
}

VOID NTAPI PhSipSysInfoUpdateHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PostMessage(PhSipWindow, SI_MSG_SYSINFO_UPDATE, 0, 0);
}
