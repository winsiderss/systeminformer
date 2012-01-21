/*
 * Process Hacker -
 *   system information window
 *
 * Copyright (C) 2011-2012 wj32
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

/*
 * This file is divided into two parts:
 *
 * System Information framework. The "framework" handles creation, layout
 * and events for the top-level window itself. It manages the list of
 * sections.
 *
 * Default sections. The CPU, Memory and I/O sections are also implemented
 * in this file.
 *
 * A section is an object that provides information to the user about a
 * type of system resource. The CPU, Memory and I/O sections are added
 * automatically to every System Information window. Plugins can also add
 * sections to the window. There are two views: summary and section. In
 * summary view, rows of graphs are displayed in the window, one graph for
 * each section. The section is responsible for providing the graph data
 * and any text to draw on the left-hand side of the graph. In section view,
 * the graphs become mini-graphs on the left-hand side of the window. The
 * section displays its own embedded dialog in the remaining space. Any
 * controls contained in this dialog, including graphs, are the
 * responsibility of the section.
 *
 * Users can enter section view by:
 * * Clicking on a graph or mini-graph.
 * * Pressing a number from 1 to 9.
 * * Using the tab or arrow keys to select a graph and pressing space or
 *   enter.
 *
 * Users can return to summary view by:
 * * Clicking "Back" on the left-hand side of the window.
 * * Pressing Backspace.
 * * Using the tab or arrow keys to select "Back" and pressing space or
 *   enter.
 */

#include <phapp.h>
#include <kphuser.h>
#include <settings.h>
#include <phplug.h>
#include <windowsx.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <sysinfop.h>

static HANDLE PhSipThread = NULL;
static HWND PhSipWindow = NULL;
static PPH_LIST PhSipDialogList = NULL;
static PH_EVENT InitializedEvent = PH_EVENT_INIT;
static PH_LAYOUT_MANAGER WindowLayoutManager;
static RECT MinimumSize;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;

static PPH_LIST SectionList;
static PH_SYSINFO_PARAMETERS CurrentParameters;
static PH_SYSINFO_VIEW_TYPE CurrentView;
static PPH_SYSINFO_SECTION CurrentSection;
static HWND ContainerControl;
static HWND SeparatorControl;
static HWND RestoreSummaryControl;
static WNDPROC RestoreSummaryControlOldWndProc;
static BOOLEAN RestoreSummaryControlHot;
static BOOLEAN RestoreSummaryControlHasFocus;

static _EnableThemeDialogTexture EnableThemeDialogTexture_I;
static HTHEME ThemeData;
static BOOLEAN ThemeHasItemBackground;

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
static ULONG NumberOfProcessors;
static PSYSTEM_INTERRUPT_INFORMATION InterruptInformation;
static PPROCESSOR_POWER_INFORMATION PowerInformation;
static PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION CurrentPerformanceDistribution;
static PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION PreviousPerformanceDistribution;
static PH_UINT32_DELTA ContextSwitchesDelta;
static PH_UINT32_DELTA InterruptsDelta;
static PH_UINT64_DELTA DpcsDelta;
static PH_UINT32_DELTA SystemCallsDelta;

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

static PPH_SYSINFO_SECTION IoSection;
static HWND IoDialog;
static PH_LAYOUT_MANAGER IoLayoutManager;
static HWND IoGraphHandle;
static PH_GRAPH_STATE IoGraphState;
static HWND IoPanel;
static ULONG IoTicked;
static PH_UINT64_DELTA IoReadDelta;
static PH_UINT64_DELTA IoWriteDelta;
static PH_UINT64_DELTA IoOtherDelta;

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
    HACCEL acceleratorTable;
    BOOLEAN processed;

    PhInitializeAutoPool(&autoPool);

    PhSipWindow = CreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_SYSINFO),
        NULL,
        PhSipSysInfoDialogProc
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
    case WM_NCDESTROY:
        {
            PhSipOnNcDestroy();
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
    case WM_THEMECHANGED:
        {
            PhSipOnThemeChanged();
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

INT_PTR CALLBACK PhSipContainerDialogProc(
    __in HWND hwndDlg,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    return FALSE;
}

VOID PhSipOnInitDialog(
    VOID
    )
{
    PhInitializeLayoutManager(&WindowLayoutManager, PhSipWindow);

    PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(PhSipWindow, IDC_INSTRUCTION), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
    PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(PhSipWindow, IDC_ALWAYSONTOP), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
    PhAddLayoutItem(&WindowLayoutManager, GetDlgItem(PhSipWindow, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

    PhRegisterCallback(
        &PhProcessesUpdatedEvent,
        PhSipSysInfoUpdateHandler,
        NULL,
        &ProcessesUpdatedRegistration
        );

    ContainerControl = CreateDialog(
        PhInstanceHandle,
        MAKEINTRESOURCE(IDD_CONTAINER),
        PhSipWindow,
        PhSipContainerDialogProc
        );

    if (!EnableThemeDialogTexture_I)
        EnableThemeDialogTexture_I = PhGetProcAddress(L"uxtheme.dll", "EnableThemeDialogTexture");

    PhSetControlTheme(PhSipWindow, L"explorer");
    PhSipUpdateThemeData();
}

VOID PhSipOnDestroy(
    VOID
    )
{
    PhUnregisterCallback(
        &PhProcessesUpdatedEvent,
        &ProcessesUpdatedRegistration
        );

    if (CurrentSection)
        PhSetStringSetting2(L"SysInfoWindowSection", &CurrentSection->Name);
    else
        PhSetStringSetting(L"SysInfoWindowSection", L"");

    PhSetIntegerSetting(L"SysInfoWindowAlwaysOnTop", AlwaysOnTop);

    PhSaveWindowPlacementToSetting(L"SysInfoWindowPosition", L"SysInfoWindowSize", PhSipWindow);
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
        CloseThemeData_I(ThemeData);
        ThemeData = NULL;
    }

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
    PPH_STRING sectionName;
    PPH_SYSINFO_SECTION section;

    if (SectionList)
        return;

    SectionList = PhCreateList(8);
    PhSipInitializeParameters();
    CurrentView = SysInfoSummaryView;
    CurrentSection = NULL;

    CpuSection = PhSipCreateInternalSection(L"CPU", 0, PhSipCpuSectionCallback);
    MemorySection = PhSipCreateInternalSection(L"Memory", 0, PhSipMemorySectionCallback);
    IoSection = PhSipCreateInternalSection(L"I/O", 0, PhSipIoSectionCallback);

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
        L"STATIC",
        NULL,
        WS_CHILD | SS_OWNERDRAW,
        0,
        0,
        3,
        3,
        PhSipWindow,
        (HMENU)IDC_SEPARATOR,
        PhInstanceHandle,
        NULL
        );

    RestoreSummaryControl = CreateWindow(
        L"STATIC",
        NULL,
        WS_CHILD | WS_TABSTOP | SS_OWNERDRAW | SS_NOTIFY,
        0,
        0,
        3,
        3,
        PhSipWindow,
        (HMENU)IDC_RESET,
        PhInstanceHandle,
        NULL
        );
    RestoreSummaryControlOldWndProc = (WNDPROC)GetWindowLongPtr(RestoreSummaryControl, GWLP_WNDPROC);
    SetWindowLongPtr(RestoreSummaryControl, GWLP_WNDPROC, (LONG_PTR)PhSipPanelHookWndProc);
    RestoreSummaryControlHot = FALSE;

    if (EnableThemeDialogTexture_I)
        EnableThemeDialogTexture_I(ContainerControl, ETDT_ENABLETAB);

    GetWindowRect(GetDlgItem(PhSipWindow, IDOK), &buttonRect);
    MapWindowPoints(NULL, PhSipWindow, (POINT *)&buttonRect, 2);
    GetClientRect(PhSipWindow, &clientRect);

    MinimumSize.left = 0;
    MinimumSize.top = 0;
    MinimumSize.right = WindowsVersion >= WINDOWS_VISTA ? 430 : 370; // XP doesn't have the Memory Lists group
    MinimumSize.bottom = 290;
    MapDialogRect(PhSipWindow, &MinimumSize);

    MinimumSize.right += CurrentParameters.PanelWidth;
    MinimumSize.right += GetSystemMetrics(SM_CXFRAME) * 2;
    MinimumSize.bottom += GetSystemMetrics(SM_CYFRAME) * 2;

    if (SectionList->Count != 0)
    {
        ULONG newMinimumHeight;

        newMinimumHeight =
            GetSystemMetrics(SM_CYCAPTION) +
            PH_SYSINFO_WINDOW_PADDING +
            CurrentParameters.MinimumGraphHeight * SectionList->Count +
            CurrentParameters.MinimumGraphHeight + // Back button
            PH_SYSINFO_GRAPH_PADDING * SectionList->Count +
            PH_SYSINFO_WINDOW_PADDING +
            clientRect.bottom - buttonRect.top +
            GetSystemMetrics(SM_CYFRAME) * 2;

        if (newMinimumHeight > (ULONG)MinimumSize.bottom)
            MinimumSize.bottom = newMinimumHeight;
    }

    PhLoadWindowPlacementFromSetting(L"SysInfoWindowPosition", L"SysInfoWindowSize", PhSipWindow);

    sectionName = PhGetStringSetting(L"SysInfoWindowSection");

    if (sectionName->Length != 0 &&
        (section = PhSipFindSection(&sectionName->sr)))
    {
        PhSipEnterSectionView(section);
    }

    PhDereferenceObject(sectionName);

    AlwaysOnTop = (BOOLEAN)PhGetIntegerSetting(L"SysInfoWindowAlwaysOnTop");
    Button_SetCheck(GetDlgItem(PhSipWindow, IDC_ALWAYSONTOP), AlwaysOnTop ? BST_CHECKED : BST_UNCHECKED);
    PhSipSetAlwaysOnTop();

    PhSipOnSize();
    PhSipOnUserMessage(SI_MSG_SYSINFO_UPDATE, 0, 0);
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

VOID PhSipOnThemeChanged(
    VOID
    )
{
    PhSipUpdateThemeData();
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
    case IDC_BACK:
        {
            PhSipRestoreSummaryView();
        }
        break;
    case IDC_REFRESH:
        {
            ProcessHacker_Refresh(PhMainWndHandle);
        }
        break;
    case IDC_PAUSE:
        {
            ProcessHacker_SetUpdateAutomatically(PhMainWndHandle, !ProcessHacker_GetUpdateAutomatically(PhMainWndHandle));
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
    else if (Id == IDC_SEPARATOR)
    {
        PhSipDrawSeparator(DrawItemStruct->hDC, &DrawItemStruct->rcItem);
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
    __out PPH_GRAPH_DRAW_INFO DrawInfo,
    __in COLORREF Color1,
    __in COLORREF Color2
    )
{
    switch (PhCsGraphColorMode)
    {
    case 0: // New colors
        DrawInfo->BackColor = RGB(0xef, 0xef, 0xef);
        DrawInfo->LineColor1 = PhHalveColorBrightness(Color1);
        DrawInfo->LineBackColor1 = PhMakeColorBrighter(Color1, 125);
        DrawInfo->LineColor2 = PhHalveColorBrightness(Color2);
        DrawInfo->LineBackColor2 = PhMakeColorBrighter(Color2, 125);
        DrawInfo->GridColor = RGB(0xc7, 0xc7, 0xc7);
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
        DrawInfo->TextColor = RGB(0x00, 0xff, 0x00);
        DrawInfo->TextBoxColor = RGB(0x00, 0x22, 0x00);
        break;
    }
}

VOID PhSipRegisterDialog(
    __in HWND DialogWindowHandle
    )
{
    if (!PhSipDialogList)
        PhSipDialogList = PhCreateList(4);

    PhAddItemList(PhSipDialogList, DialogWindowHandle);
}

VOID PhSipUnregisterDialog(
    __in HWND DialogWindowHandle
    )
{
    ULONG index;

    if ((index = PhFindItemList(PhSipDialogList, DialogWindowHandle)) != -1)
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

    memset(&CurrentParameters, 0, sizeof(PH_SYSINFO_PARAMETERS));

    CurrentParameters.SysInfoWindowHandle = PhSipWindow;
    CurrentParameters.ContainerWindowHandle = ContainerControl;

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

    logFont.lfHeight -= MulDiv(3, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    CurrentParameters.MediumFont = CreateFontIndirect(&logFont);

    logFont.lfHeight -= MulDiv(3, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    CurrentParameters.LargeFont = CreateFontIndirect(&logFont);

    PhSipUpdateColorParameters();

    CurrentParameters.ColorSetupFunction = PhSiSetColorsGraphDrawInfo;

    originalFont = SelectObject(hdc, CurrentParameters.Font);
    GetTextMetrics(hdc, &textMetrics);
    CurrentParameters.FontHeight = textMetrics.tmHeight;
    CurrentParameters.FontAverageWidth = textMetrics.tmAveCharWidth;

    SelectObject(hdc, CurrentParameters.MediumFont);
    GetTextMetrics(hdc, &textMetrics);
    CurrentParameters.MediumFontHeight = textMetrics.tmHeight;
    CurrentParameters.MediumFontAverageWidth = textMetrics.tmAveCharWidth;

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

    CurrentParameters.PanelWidth = CurrentParameters.MediumFontAverageWidth * 10;

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
        WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP | GC_STYLE_FADEOUT | GC_STYLE_DRAW_PANEL,
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
    options.FadeOutBackColor = CurrentParameters.GraphBackColor;
    options.FadeOutWidth = CurrentParameters.PanelWidth + PH_SYSINFO_FADE_ADD;
    options.DefaultCursor = LoadCursor(NULL, IDC_HAND);
    Graph_SetOptions(section->GraphHandle, &options);
    Graph_SetTooltip(section->GraphHandle, TRUE);

    section->PanelId = IDDYNAMIC + SectionList->Count * 2 + 2;
    section->PanelHandle = CreateWindow(
        L"STATIC",
        NULL,
        WS_CHILD | SS_OWNERDRAW | SS_NOTIFY,
        0,
        0,
        3,
        3,
        PhSipWindow,
        (HMENU)section->PanelId,
        PhInstanceHandle,
        NULL
        );

    SetProp(section->GraphHandle, PhMakeContextAtom(), section);
    section->GraphOldWndProc = (WNDPROC)GetWindowLongPtr(section->GraphHandle, GWLP_WNDPROC);
    SetWindowLongPtr(section->GraphHandle, GWLP_WNDPROC, (LONG_PTR)PhSipGraphHookWndProc);

    SetProp(section->PanelHandle, PhMakeContextAtom(), section);
    section->PanelOldWndProc = (WNDPROC)GetWindowLongPtr(section->PanelHandle, GWLP_WNDPROC);
    SetWindowLongPtr(section->PanelHandle, GWLP_WNDPROC, (LONG_PTR)PhSipPanelHookWndProc);

    PhAddItemList(SectionList, section);

    section->Callback(section, SysInfoCreate, NULL, NULL);

    return section;
}

VOID PhSipDestroySection(
    __in PPH_SYSINFO_SECTION Section
    )
{
    Section->Callback(Section, SysInfoDestroy, NULL, NULL);

    PhDeleteGraphState(&Section->GraphState);
    PhFree(Section);
}

PPH_SYSINFO_SECTION PhSipFindSection(
    __in PPH_STRINGREF Name
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
    __in PWSTR Name,
    __in ULONG Flags,
    __in PPH_SYSINFO_SECTION_CALLBACK Callback
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
    __in HDC hdc,
    __in PRECT Rect
    )
{
    FillRect(hdc, Rect, GetSysColorBrush(COLOR_3DFACE));

    if (RestoreSummaryControlHot || RestoreSummaryControlHasFocus)
    {
        if (ThemeHasItemBackground)
        {
            DrawThemeBackground_I(
                ThemeData,
                hdc,
                TVP_TREEITEM,
                TREIS_HOT,
                Rect,
                Rect
                );
        }
        else
        {
            FillRect(hdc, Rect, GetSysColorBrush(COLOR_WINDOW));
        }
    }

    SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    SetBkMode(hdc, TRANSPARENT);

    SelectObject(hdc, CurrentParameters.MediumFont);
    DrawText(hdc, L"Back", 4, Rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

VOID PhSipDrawSeparator(
    __in HDC hdc,
    __in PRECT Rect
    )
{
    RECT rect;

    rect = *Rect;
    FillRect(hdc, &rect, GetSysColorBrush(COLOR_3DHIGHLIGHT));
    rect.left += 1;
    FillRect(hdc, &rect, GetSysColorBrush(COLOR_3DSHADOW));
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

    PhSwapReference(&sysInfoDrawPanel.Title, NULL);
    PhSwapReference(&sysInfoDrawPanel.SubTitle, NULL);
    PhSwapReference(&sysInfoDrawPanel.SubTitleOverflow, NULL);
}

VOID PhSipDefaultDrawPanel(
    __in PPH_SYSINFO_SECTION Section,
    __in PPH_SYSINFO_DRAW_PANEL DrawPanel
    )
{
    HDC hdc;
    RECT rect;
    ULONG flags;

    hdc = DrawPanel->hdc;

    if (ThemeHasItemBackground)
    {
        if (CurrentView == SysInfoSectionView)
        {
            INT stateId;

            stateId = -1;

            if (Section->GraphHot || Section->PanelHot || Section->HasFocus)
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

                DrawThemeBackground_I(
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
            DrawThemeBackground_I(
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
            HBRUSH brush;

            brush = NULL;

            if (Section->GraphHot || Section->PanelHot || Section->HasFocus)
            {
                brush = GetSysColorBrush(COLOR_WINDOW); // TODO: Use a different color
            }
            else if (Section == CurrentSection)
            {
                brush = GetSysColorBrush(COLOR_WINDOW);
            }

            if (brush)
            {
                FillRect(hdc, &DrawPanel->Rect, brush);
            }
        }
    }

    if (CurrentView == SysInfoSummaryView)
        SetTextColor(hdc, CurrentParameters.PanelForeColor);
    else
        SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));

    SetBkMode(hdc, TRANSPARENT);

    rect.left = PH_SYSINFO_SMALL_GRAPH_PADDING + PH_SYSINFO_PANEL_PADDING;
    rect.top = PH_SYSINFO_PANEL_PADDING;
    rect.right = CurrentParameters.PanelWidth;
    rect.bottom = DrawPanel->Rect.bottom;

    flags = DT_NOPREFIX;

    if (CurrentView == SysInfoSummaryView)
        rect.right = DrawPanel->Rect.right; // allow the text to overflow
    else
        flags |= DT_END_ELLIPSIS;

    if (DrawPanel->Title)
    {
        SelectObject(hdc, CurrentParameters.MediumFont);
        DrawText(hdc, DrawPanel->Title->Buffer, (ULONG)DrawPanel->Title->Length / 2, &rect, flags | DT_SINGLELINE);
    }

    if (DrawPanel->SubTitle)
    {
        RECT measureRect;

        rect.top += CurrentParameters.MediumFontHeight + PH_SYSINFO_PANEL_PADDING;
        SelectObject(hdc, CurrentParameters.Font);

        measureRect = rect;
        DrawText(hdc, DrawPanel->SubTitle->Buffer, (ULONG)DrawPanel->SubTitle->Length / 2, &measureRect, (flags & ~DT_END_ELLIPSIS) | DT_CALCRECT);

        if (measureRect.right <= rect.right || !DrawPanel->SubTitleOverflow)
        {
            // Text fits; draw normally.
            DrawText(hdc, DrawPanel->SubTitle->Buffer, (ULONG)DrawPanel->SubTitle->Length / 2, &rect, flags);
        }
        else
        {
            // Text doesn't fit; draw the alternative text.
            DrawText(hdc, DrawPanel->SubTitleOverflow->Buffer, (ULONG)DrawPanel->SubTitleOverflow->Length / 2, &rect, flags);
        }
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
    ULONG containerLeft;

    GetWindowRect(GetDlgItem(PhSipWindow, IDOK), &buttonRect);
    MapWindowPoints(NULL, PhSipWindow, (POINT *)&buttonRect, 2);
    GetClientRect(PhSipWindow, &clientRect);

    availableHeight = buttonRect.top - PH_SYSINFO_WINDOW_PADDING * 2;
    availableWidth = clientRect.right - PH_SYSINFO_WINDOW_PADDING * 2;
    graphHeight = (availableHeight - PH_SYSINFO_SMALL_GRAPH_PADDING * SectionList->Count) / (SectionList->Count + 1);

    if (graphHeight > CurrentParameters.SectionViewGraphHeight)
        graphHeight = CurrentParameters.SectionViewGraphHeight;

    deferHandle = BeginDeferWindowPos(SectionList->Count * 2 + 3);
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
            PH_SYSINFO_SMALL_GRAPH_WIDTH,
            graphHeight,
            SWP_NOACTIVATE | SWP_NOZORDER
            );

        deferHandle = DeferWindowPos(
            deferHandle,
            section->PanelHandle,
            NULL,
            PH_SYSINFO_WINDOW_PADDING + PH_SYSINFO_SMALL_GRAPH_WIDTH,
            y,
            PH_SYSINFO_SMALL_GRAPH_PADDING + CurrentParameters.PanelWidth,
            graphHeight,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
        InvalidateRect(section->PanelHandle, NULL, TRUE);

        y += graphHeight + PH_SYSINFO_SMALL_GRAPH_PADDING;

        section->GraphState.Valid = FALSE;
    }

    deferHandle = DeferWindowPos(
        deferHandle,
        RestoreSummaryControl,
        NULL,
        PH_SYSINFO_WINDOW_PADDING,
        y,
        PH_SYSINFO_SMALL_GRAPH_WIDTH + PH_SYSINFO_SMALL_GRAPH_PADDING + CurrentParameters.PanelWidth,
        graphHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );
    InvalidateRect(RestoreSummaryControl, NULL, TRUE);

    deferHandle = DeferWindowPos(
        deferHandle,
        SeparatorControl,
        NULL,
        PH_SYSINFO_WINDOW_PADDING + PH_SYSINFO_SMALL_GRAPH_WIDTH + PH_SYSINFO_SMALL_GRAPH_PADDING + CurrentParameters.PanelWidth + PH_SYSINFO_WINDOW_PADDING - 7,
        0,
        PH_SYSINFO_SEPARATOR_WIDTH,
        PH_SYSINFO_WINDOW_PADDING + availableHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    containerLeft = PH_SYSINFO_WINDOW_PADDING + PH_SYSINFO_SMALL_GRAPH_WIDTH + PH_SYSINFO_SMALL_GRAPH_PADDING + CurrentParameters.PanelWidth + PH_SYSINFO_WINDOW_PADDING - 7 + PH_SYSINFO_SEPARATOR_WIDTH;
    deferHandle = DeferWindowPos(
        deferHandle,
        ContainerControl,
        NULL,
        containerLeft,
        0,
        clientRect.right - containerLeft,
        PH_SYSINFO_WINDOW_PADDING + availableHeight,
        SWP_NOACTIVATE | SWP_NOZORDER
        );

    EndDeferWindowPos(deferHandle);

    if (CurrentSection && CurrentSection->DialogHandle)
    {
        SetWindowPos(
            CurrentSection->DialogHandle,
            NULL,
            PH_SYSINFO_WINDOW_PADDING,
            PH_SYSINFO_WINDOW_PADDING,
            clientRect.right - containerLeft - PH_SYSINFO_WINDOW_PADDING - PH_SYSINFO_WINDOW_PADDING,
            availableHeight,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
    }
}

VOID PhSipEnterSectionView(
    __in PPH_SYSINFO_SECTION NewSection
    )
{
    ULONG i;
    PPH_SYSINFO_SECTION section;
    BOOLEAN fromSummaryView;
    PPH_SYSINFO_SECTION oldSection;

    fromSummaryView = CurrentView == SysInfoSummaryView;
    CurrentView = SysInfoSectionView;
    oldSection = CurrentSection;
    CurrentSection = NewSection;

    for (i = 0; i < SectionList->Count; i++)
    {
        section = SectionList->Items[i];

        section->HasFocus = FALSE;
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

    ShowWindow(ContainerControl, SW_SHOW);
    ShowWindow(RestoreSummaryControl, SW_SHOW);
    ShowWindow(SeparatorControl, SW_SHOW);
    ShowWindow(GetDlgItem(PhSipWindow, IDC_INSTRUCTION), SW_HIDE);

    if (oldSection)
        InvalidateRect(oldSection->PanelHandle, NULL, TRUE);

    InvalidateRect(NewSection->PanelHandle, NULL, TRUE);

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

    ShowWindow(ContainerControl, SW_HIDE);
    ShowWindow(RestoreSummaryControl, SW_HIDE);
    ShowWindow(SeparatorControl, SW_HIDE);
    ShowWindow(GetDlgItem(PhSipWindow, IDC_INSTRUCTION), SW_SHOW);

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

    dialogHandle = CreateDialogIndirect(Instance, (DLGTEMPLATE *)dialogCopy, ContainerControl, DialogProc);

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

LRESULT CALLBACK PhSipGraphHookWndProc(
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    WNDPROC oldWndProc;
    PPH_SYSINFO_SECTION section;

    section = GetProp(hwnd, PhMakeContextAtom());

    if (!section)
        return 0;

    oldWndProc = section->GraphOldWndProc;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            RemoveProp(hwnd, PhMakeContextAtom());
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
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
                InvalidateRect(section->PanelHandle, NULL, TRUE);
            }
            else
            {
                section->GraphHot = TRUE;
            }
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
    __in HWND hwnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    )
{
    WNDPROC oldWndProc;
    PPH_SYSINFO_SECTION section;

    section = GetProp(hwnd, PhMakeContextAtom());

    if (section)
        oldWndProc = section->PanelOldWndProc;
    else
        oldWndProc = RestoreSummaryControlOldWndProc;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            RemoveProp(hwnd, PhMakeContextAtom());
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
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
    if (
        IsThemeActive_I &&
        OpenThemeData_I &&
        CloseThemeData_I &&
        IsThemePartDefined_I &&
        DrawThemeBackground_I
        )
    {
        if (ThemeData)
        {
            CloseThemeData_I(ThemeData);
            ThemeData = NULL;
        }

        ThemeData = OpenThemeData_I(PhSipWindow, L"TREEVIEW");

        if (ThemeData)
        {
            ThemeHasItemBackground = !!IsThemePartDefined_I(ThemeData, TVP_TREEITEM, 0);
        }
        else
        {
            ThemeHasItemBackground = FALSE;
        }
    }
    else
    {
        ThemeData = NULL;
        ThemeHasItemBackground = FALSE;
    }
}

VOID PhSipSetAlwaysOnTop(
    VOID
    )
{
    SetFocus(PhSipWindow); // HACK - SetWindowPos doesn't work properly without this
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

VOID PhSipInitializeCpuDialog(
    VOID
    )
{
    ULONG i;

    PhInitializeDelta(&ContextSwitchesDelta);
    PhInitializeDelta(&InterruptsDelta);
    PhInitializeDelta(&DpcsDelta);
    PhInitializeDelta(&SystemCallsDelta);

    NumberOfProcessors = (ULONG)PhSystemBasicInformation.NumberOfProcessors;
    CpusGraphHandle = PhAllocate(sizeof(HWND) * NumberOfProcessors);
    CpusGraphState = PhAllocate(sizeof(PH_GRAPH_STATE) * NumberOfProcessors);
    InterruptInformation = PhAllocate(sizeof(SYSTEM_INTERRUPT_INFORMATION) * NumberOfProcessors);
    PowerInformation = PhAllocate(sizeof(PROCESSOR_POWER_INFORMATION) * NumberOfProcessors);

    PhInitializeGraphState(&CpuGraphState);

    for (i = 0; i < NumberOfProcessors; i++)
        PhInitializeGraphState(&CpusGraphState[i]);

    CpuTicked = 0;

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

    CurrentPerformanceDistribution = NULL;
    PreviousPerformanceDistribution = NULL;

    if (WindowsVersion >= WINDOWS_7)
        PhSipQueryProcessorPerformanceDistribution(&CurrentPerformanceDistribution);
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

    if (NT_SUCCESS(NtQuerySystemInformation(
        SystemInterruptInformation,
        InterruptInformation,
        sizeof(SYSTEM_INTERRUPT_INFORMATION) * NumberOfProcessors,
        NULL
        )))
    {
        for (i = 0; i < NumberOfProcessors; i++)
            dpcCount += InterruptInformation[i].DpcCount;
    }

    PhUpdateDelta(&ContextSwitchesDelta, PhPerfInformation.ContextSwitches);
    PhUpdateDelta(&InterruptsDelta, PhCpuTotals.InterruptCount);
    PhUpdateDelta(&DpcsDelta, dpcCount);
    PhUpdateDelta(&SystemCallsDelta, PhPerfInformation.SystemCalls);

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

    if (WindowsVersion >= WINDOWS_7)
    {
        if (PreviousPerformanceDistribution)
            PhFree(PreviousPerformanceDistribution);

        PreviousPerformanceDistribution = CurrentPerformanceDistribution;
        CurrentPerformanceDistribution = NULL;
        PhSipQueryProcessorPerformanceDistribution(&CurrentPerformanceDistribution);
    }

    CpuTicked++;

    if (CpuTicked > 2)
        CpuTicked = 2;

    PhSipUpdateCpuGraphs();
    PhSipUpdateCpuPanel();
}

INT_PTR CALLBACK PhSipCpuDialogProc(
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
            PPH_LAYOUT_ITEM graphItem;
            PPH_LAYOUT_ITEM panelItem;
            WCHAR brandString[49];

            PhSipInitializeCpuDialog();

            CpuDialog = hwndDlg;
            PhInitializeLayoutManager(&CpuLayoutManager, hwndDlg);
            PhAddLayoutItem(&CpuLayoutManager, GetDlgItem(hwndDlg, IDC_CPUNAME), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            graphItem = PhAddLayoutItem(&CpuLayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            CpuGraphMargin = graphItem->Margin;
            panelItem = PhAddLayoutItem(&CpuLayoutManager, GetDlgItem(hwndDlg, IDC_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            SendMessage(GetDlgItem(hwndDlg, IDC_TITLE), WM_SETFONT, (WPARAM)CurrentParameters.LargeFont, FALSE);
            SendMessage(GetDlgItem(hwndDlg, IDC_CPUNAME), WM_SETFONT, (WPARAM)CurrentParameters.MediumFont, FALSE);

            PhSipGetCpuBrandString(brandString);
            SetDlgItemText(hwndDlg, IDC_CPUNAME, brandString);

            CpuPanel = CreateDialog(PhInstanceHandle, MAKEINTRESOURCE(IDD_SYSINFO_CPUPANEL), hwndDlg, PhSipCpuPanelDialogProc);
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
    case WM_SIZE:
        {
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
                PhSipNotifyCpuGraph(-1, header);
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
    }

    return FALSE;
}

INT_PTR CALLBACK PhSipCpuPanelDialogProc(
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
            SendMessage(GetDlgItem(hwndDlg, IDC_UTILIZATION), WM_SETFONT, (WPARAM)CurrentParameters.MediumFont, FALSE);
            SendMessage(GetDlgItem(hwndDlg, IDC_SPEED), WM_SETFONT, (WPARAM)CurrentParameters.MediumFont, FALSE);
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_ONEGRAPHPERCPU:
                {
                    OneGraphPerCpu = Button_GetCheck(GetDlgItem(hwndDlg, IDC_ONEGRAPHPERCPU)) == BST_CHECKED;
                    PhSipLayoutCpuGraphs();
                    PhSipSetOneGraphPerCpu();
                }
                break;
            }
        }
        break;
    }

    return FALSE;
}

VOID PhSipCreateCpuGraphs(
    VOID
    )
{
    ULONG i;

    CpuGraphHandle = CreateWindow(
        PH_GRAPH_CLASSNAME,
        NULL,
        WS_CHILD | WS_BORDER,
        0,
        0,
        3,
        3,
        CpuDialog,
        (HMENU)IDC_CPU,
        PhInstanceHandle,
        NULL
        );
    Graph_SetTooltip(CpuGraphHandle, TRUE);

    for (i = 0; i < NumberOfProcessors; i++)
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
            (HMENU)(IDC_CPU0 + i),
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
    HDWP deferHandle;

    GetClientRect(CpuDialog, &clientRect);
    deferHandle = BeginDeferWindowPos(OneGraphPerCpu ? NumberOfProcessors : 1);

    if (!OneGraphPerCpu)
    {
        deferHandle = DeferWindowPos(
            deferHandle,
            CpuGraphHandle,
            NULL,
            CpuGraphMargin.left,
            CpuGraphMargin.top,
            clientRect.right - CpuGraphMargin.left - CpuGraphMargin.right,
            clientRect.bottom - CpuGraphMargin.top - CpuGraphMargin.bottom,
            SWP_NOACTIVATE | SWP_NOZORDER
            );
    }
    else
    {
        ULONG i;
        ULONG graphWidth;
        ULONG x;

        graphWidth = (clientRect.right - CpuGraphMargin.left - CpuGraphMargin.right - PH_SYSINFO_CPU_PADDING * (NumberOfProcessors - 1)) / NumberOfProcessors;
        x = CpuGraphMargin.left;

        for (i = 0; i < NumberOfProcessors; i++)
        {
            // Give the last graph the remaining space; the width we calculated might be off by a few
            // pixels due to integer division.
            if (i == NumberOfProcessors - 1)
            {
                graphWidth = clientRect.right - CpuGraphMargin.right - x;
            }

            deferHandle = DeferWindowPos(
                deferHandle,
                CpusGraphHandle[i],
                NULL,
                x,
                CpuGraphMargin.top,
                graphWidth,
                clientRect.bottom - CpuGraphMargin.top - CpuGraphMargin.bottom,
                SWP_NOACTIVATE | SWP_NOZORDER
                );
            x += graphWidth + PH_SYSINFO_CPU_PADDING;
        }
    }

    EndDeferWindowPos(deferHandle);
}

VOID PhSipSetOneGraphPerCpu(
    VOID
    )
{
    ULONG i;

    ShowWindow(CpuGraphHandle, !OneGraphPerCpu ? SW_SHOW : SW_HIDE);

    for (i = 0; i < NumberOfProcessors; i++)
    {
        ShowWindow(CpusGraphHandle[i], OneGraphPerCpu ? SW_SHOW : SW_HIDE);
    }
}

VOID PhSipNotifyCpuGraph(
    __in ULONG Index,
    __in NMHDR *Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
            PhSiSetColorsGraphDrawInfo(drawInfo, PhCsColorCpuKernel, PhCsColorCpuUser);

            if (Index == -1)
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
                    CpusGraphState[Index].Valid = TRUE;
                }
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (Index == -1)
                {
                    if (CpuGraphState.TooltipIndex != getTooltipText->Index)
                    {
                        FLOAT cpuKernel;
                        FLOAT cpuUser;

                        cpuKernel = PhGetItemCircularBuffer_FLOAT(&PhCpuKernelHistory, getTooltipText->Index);
                        cpuUser = PhGetItemCircularBuffer_FLOAT(&PhCpuUserHistory, getTooltipText->Index);

                        PhSwapReference2(&CpuGraphState.TooltipText, PhFormatString(
                            L"%.2f%%%s\n%s",
                            (cpuKernel + cpuUser) * 100,
                            PhGetStringOrEmpty(PhSipGetMaxCpuString(getTooltipText->Index)),
                            ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                            ));
                    }

                    getTooltipText->Text = CpuGraphState.TooltipText->sr;
                }
                else
                {
                    if (CpusGraphState[Index].TooltipIndex != getTooltipText->Index)
                    {
                        FLOAT cpuKernel;
                        FLOAT cpuUser;

                        cpuKernel = PhGetItemCircularBuffer_FLOAT(&PhCpusKernelHistory[Index], getTooltipText->Index);
                        cpuUser = PhGetItemCircularBuffer_FLOAT(&PhCpusUserHistory[Index], getTooltipText->Index);

                        PhSwapReference2(&CpusGraphState[Index].TooltipText, PhFormatString(
                            L"%.2f%% (K: %.2f%%, U: %.2f%%)%s\n%s",
                            (cpuKernel + cpuUser) * 100,
                            cpuKernel * 100,
                            cpuUser * 100,
                            PhGetStringOrEmpty(PhSipGetMaxCpuString(getTooltipText->Index)),
                            ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                            ));
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
    ULONG i;

    CpuGraphState.Valid = FALSE;
    CpuGraphState.TooltipIndex = -1;
    Graph_MoveGrid(CpuGraphHandle, 1);
    Graph_Draw(CpuGraphHandle);
    Graph_UpdateTooltip(CpuGraphHandle);
    InvalidateRect(CpuGraphHandle, NULL, FALSE);

    for (i = 0; i < NumberOfProcessors; i++)
    {
        CpusGraphState[i].Valid = FALSE;
        CpusGraphState[i].TooltipIndex = -1;
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
    HWND hwnd = CpuPanel;
    DOUBLE cpuFraction;
    DOUBLE cpuGhz;
    BOOLEAN distributionSucceeded;
    SYSTEM_TIMEOFDAY_INFORMATION timeOfDayInfo;
    WCHAR uptimeString[PH_TIMESPAN_STR_LEN_1] = L"Unknown";

    SetDlgItemText(hwnd, IDC_UTILIZATION, PhaFormatString(L"%.2f%%", (PhCpuUserUsage + PhCpuKernelUsage) * 100)->Buffer);

    cpuGhz = 0;
    distributionSucceeded = FALSE;

    if (WindowsVersion >= WINDOWS_7 && CurrentPerformanceDistribution && PreviousPerformanceDistribution)
    {
        if (PhSipGetCpuFrequencyFromDistribution(&cpuFraction))
        {
            cpuGhz = (DOUBLE)PowerInformation[0].MaxMhz * cpuFraction / 1000;
            distributionSucceeded = TRUE;
        }
    }

    if (!distributionSucceeded)
        cpuGhz = (DOUBLE)PowerInformation[0].CurrentMhz / 1000;

    SetDlgItemText(hwnd, IDC_SPEED, PhaFormatString(L"%.2f / %.2f GHz", cpuGhz, (DOUBLE)PowerInformation[0].MaxMhz / 1000)->Buffer);

    SetDlgItemText(hwnd, IDC_ZPROCESSES_V, PhaFormatUInt64(PhTotalProcesses, TRUE)->Buffer);
    SetDlgItemText(hwnd, IDC_ZTHREADS_V, PhaFormatUInt64(PhTotalThreads, TRUE)->Buffer);
    SetDlgItemText(hwnd, IDC_ZHANDLES_V, PhaFormatUInt64(PhTotalHandles, TRUE)->Buffer);

    if (NT_SUCCESS(NtQuerySystemInformation(
        SystemTimeOfDayInformation,
        &timeOfDayInfo,
        sizeof(SYSTEM_TIMEOFDAY_INFORMATION),
        NULL
        )))
    {
        PhPrintTimeSpan(uptimeString, timeOfDayInfo.CurrentTime.QuadPart - timeOfDayInfo.BootTime.QuadPart, PH_TIMESPAN_DHMS);
    }

    SetDlgItemText(hwnd, IDC_ZUPTIME_V, uptimeString);

    if (CpuTicked > 1)
        SetDlgItemText(hwnd, IDC_ZCONTEXTSWITCHESDELTA_V, PhaFormatUInt64(ContextSwitchesDelta.Delta, TRUE)->Buffer);
    else
        SetDlgItemText(hwnd, IDC_ZCONTEXTSWITCHESDELTA_V, L"-");

    if (CpuTicked > 1)
        SetDlgItemText(hwnd, IDC_ZINTERRUPTSDELTA_V, PhaFormatUInt64(InterruptsDelta.Delta, TRUE)->Buffer);
    else
        SetDlgItemText(hwnd, IDC_ZINTERRUPTSDELTA_V, L"-");

    if (CpuTicked > 1)
        SetDlgItemText(hwnd, IDC_ZDPCSDELTA_V, PhaFormatUInt64(DpcsDelta.Delta, TRUE)->Buffer);
    else
        SetDlgItemText(hwnd, IDC_ZDPCSDELTA_V, L"-");

    if (CpuTicked > 1)
        SetDlgItemText(hwnd, IDC_ZSYSTEMCALLSDELTA_V, PhaFormatUInt64(SystemCallsDelta.Delta, TRUE)->Buffer);
    else
        SetDlgItemText(hwnd, IDC_ZSYSTEMCALLSDELTA_V, L"-");
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

VOID PhSipGetCpuBrandString(
    __out_ecount(49) PWSTR BrandString
    )
{
    ULONG brandString[4 * 3];

    __cpuid(&brandString[0], 0x80000002);
    __cpuid(&brandString[4], 0x80000003);
    __cpuid(&brandString[8], 0x80000004);

    PhZeroExtendToUnicode((PSTR)brandString, 48, BrandString);
    BrandString[48] = 0;
}

BOOLEAN PhSipGetCpuFrequencyFromDistribution(
    __out DOUBLE *Fraction
    )
{
    PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION differences;
    PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION stateDistribution;
    ULONG i;
    ULONG j;
    DOUBLE count;
    DOUBLE total;

    // Calculate the differences from the last performance distribution.

    if (CurrentPerformanceDistribution->ProcessorCount != NumberOfProcessors || PreviousPerformanceDistribution->ProcessorCount != NumberOfProcessors)
        return FALSE;

    differences = PhAllocate((FIELD_OFFSET(SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION, States) + sizeof(SYSTEM_PROCESSOR_PERFORMANCE_HITCOUNT) * 2) * NumberOfProcessors);

    for (i = 0; i < NumberOfProcessors; i++)
    {
        stateDistribution = (PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION)((PCHAR)CurrentPerformanceDistribution + CurrentPerformanceDistribution->Offsets[i]);

        if (stateDistribution->StateCount != 2)
        {
            PhFree(differences);
            return FALSE;
        }

        for (j = 0; j < stateDistribution->StateCount; j++)
        {
            differences[i].States[j] = stateDistribution->States[j];
        }
    }

    for (i = 0; i < NumberOfProcessors; i++)
    {
        stateDistribution = (PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION)((PCHAR)PreviousPerformanceDistribution + PreviousPerformanceDistribution->Offsets[i]);

        if (stateDistribution->StateCount != 2)
        {
            PhFree(differences);
            return FALSE;
        }

        for (j = 0; j < stateDistribution->StateCount; j++)
        {
            differences[i].States[j].Hits -= stateDistribution->States[j].Hits;
        }
    }

    // Calculate the frequency.

    count = 0;
    total = 0;

    for (i = 0; i < NumberOfProcessors; i++)
    {
        for (j = 0; j < 2; j++)
        {
            count += differences[i].States[j].Hits;
            total += differences[i].States[j].Hits * differences[i].States[j].PercentFrequency;
        }
    }

    PhFree(differences);

    if (count == 0)
        return FALSE;

    total /= count;
    total /= 100;
    *Fraction = total;

    return TRUE;
}

NTSTATUS PhSipQueryProcessorPerformanceDistribution(
    __out PVOID *Buffer
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

BOOLEAN PhSipMemorySectionCallback(
    __in PPH_SYSINFO_SECTION Section,
    __in PH_SYSINFO_SECTION_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2
    )
{
    switch (Message)
    {
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
                drawInfo->Flags = PH_GRAPH_USE_GRID;
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
                        PhxfDivideSingle2U(
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
        }
        return TRUE;
    case SysInfoGraphGetTooltipText:
        {
            PPH_SYSINFO_GRAPH_GET_TOOLTIP_TEXT getTooltipText = Parameter1;
            ULONG usedPages;

            if (PhGetIntegerSetting(L"ShowCommitInSummary"))
            {
                usedPages = PhGetItemCircularBuffer_ULONG(&PhCommitHistory, getTooltipText->Index);

                PhSwapReference2(&Section->GraphState.TooltipText, PhFormatString(
                    L"Commit Charge: %s\n%s",
                    PhaFormatSize(UInt32x32To64(usedPages, PAGE_SIZE), -1)->Buffer,
                    ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                    ));
                getTooltipText->Text = Section->GraphState.TooltipText->sr;
            }
            else
            {
                usedPages = PhGetItemCircularBuffer_ULONG(&PhPhysicalHistory, getTooltipText->Index);

                PhSwapReference2(&Section->GraphState.TooltipText, PhFormatString(
                    L"Physical Memory: %s\n%s",
                    PhaFormatSize(UInt32x32To64(usedPages, PAGE_SIZE), -1)->Buffer,
                    ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
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
        PhQueueItemGlobalWorkQueue(PhSipLoadMmAddresses, NULL);
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
            static BOOL (WINAPI *getPhysicallyInstalledSystemMemory)(PULONGLONG) = NULL;

            PPH_LAYOUT_ITEM graphItem;
            PPH_LAYOUT_ITEM panelItem;
            ULONGLONG installedMemory;

            PhSipInitializeMemoryDialog();

            MemoryDialog = hwndDlg;
            PhInitializeLayoutManager(&MemoryLayoutManager, hwndDlg);
            PhAddLayoutItem(&MemoryLayoutManager, GetDlgItem(hwndDlg, IDC_TOTALPHYSICAL), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT | PH_LAYOUT_FORCE_INVALIDATE);
            graphItem = PhAddLayoutItem(&MemoryLayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            MemoryGraphMargin = graphItem->Margin;
            panelItem = PhAddLayoutItem(&MemoryLayoutManager, GetDlgItem(hwndDlg, IDC_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            SendMessage(GetDlgItem(hwndDlg, IDC_TITLE), WM_SETFONT, (WPARAM)CurrentParameters.LargeFont, FALSE);
            SendMessage(GetDlgItem(hwndDlg, IDC_TOTALPHYSICAL), WM_SETFONT, (WPARAM)CurrentParameters.MediumFont, FALSE);

            if (!getPhysicallyInstalledSystemMemory)
                getPhysicallyInstalledSystemMemory = PhGetProcAddress(L"kernel32.dll", "GetPhysicallyInstalledSystemMemory");

            if (getPhysicallyInstalledSystemMemory && getPhysicallyInstalledSystemMemory(&installedMemory))
            {
                SetDlgItemText(hwndDlg, IDC_TOTALPHYSICAL,
                    PhaConcatStrings2(PhaFormatSize(installedMemory * 1024, -1)->Buffer, L" installed")->Buffer);
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
    graphHeight = (clientRect.bottom - MemoryGraphMargin.top - MemoryGraphMargin.bottom - labelRect.bottom * 2 - PH_SYSINFO_MEMORY_PADDING * 3) / 2;

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
    y += labelRect.bottom + PH_SYSINFO_MEMORY_PADDING;

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
    y += graphHeight + PH_SYSINFO_MEMORY_PADDING;

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
    y += labelRect.bottom + PH_SYSINFO_MEMORY_PADDING;

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
    __in NMHDR *Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;
            ULONG i;

            drawInfo->Flags = PH_GRAPH_USE_GRID;
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
                    PhxfDivideSingle2U(
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

                    PhSwapReference2(&CommitGraphState.TooltipText, PhFormatString(
                        L"Commit Charge: %s\n%s",
                        PhaFormatSize(UInt32x32To64(usedPages, PAGE_SIZE), -1)->Buffer,
                        ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                        ));
                }

                getTooltipText->Text = CommitGraphState.TooltipText->sr;
            }
        }
        break;
    }
}

VOID PhSipNotifyPhysicalGraph(
    __in NMHDR *Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;
            ULONG i;

            drawInfo->Flags = PH_GRAPH_USE_GRID;
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
                    PhxfDivideSingle2U(
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

                    PhSwapReference2(&PhysicalGraphState.TooltipText, PhFormatString(
                        L"Physical Memory: %s\n%s",
                        PhaFormatSize(UInt32x32To64(usedPages, PAGE_SIZE), -1)->Buffer,
                        ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
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

    // Commit Charge

    SetDlgItemText(MemoryPanel, IDC_ZCOMMITCURRENT_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.CommittedPages, PAGE_SIZE), -1)->Buffer);
    SetDlgItemText(MemoryPanel, IDC_ZCOMMITPEAK_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.PeakCommitment, PAGE_SIZE), -1)->Buffer);
    SetDlgItemText(MemoryPanel, IDC_ZCOMMITLIMIT_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.CommitLimit, PAGE_SIZE), -1)->Buffer);

    // Physical Memory

    SetDlgItemText(MemoryPanel, IDC_ZPHYSICALCURRENT_V,
        PhaFormatSize(UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages - PhPerfInformation.AvailablePages, PAGE_SIZE), -1)->Buffer);
    SetDlgItemText(MemoryPanel, IDC_ZPHYSICALTOTAL_V,
        PhaFormatSize(UInt32x32To64(PhSystemBasicInformation.NumberOfPhysicalPages, PAGE_SIZE), -1)->Buffer);
    SetDlgItemText(MemoryPanel, IDC_ZPHYSICALCACHEWS_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.ResidentSystemCachePage, PAGE_SIZE), -1)->Buffer);
    SetDlgItemText(MemoryPanel, IDC_ZPHYSICALKERNELWS_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.ResidentSystemCodePage, PAGE_SIZE), -1)->Buffer);
    SetDlgItemText(MemoryPanel, IDC_ZPHYSICALDRIVERWS_V,
        PhaFormatSize(UInt32x32To64(PhPerfInformation.ResidentSystemDriverPage, PAGE_SIZE), -1)->Buffer);

    // Paged Pool

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

    // Non-Paged Pool

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
        pagedLimit = PhaFormatSize(paged, -1)->Buffer;
        nonPagedLimit = PhaFormatSize(nonPaged, -1)->Buffer;
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

    // Memory Lists

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

VOID PhSipGetPoolLimits(
    __out PSIZE_T Paged,
    __out PSIZE_T NonPaged
    )
{
    SIZE_T paged = 0;
    SIZE_T nonPaged = 0;

    if (MmSizeOfPagedPoolInBytes)
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

BOOLEAN PhSipIoSectionCallback(
    __in PPH_SYSINFO_SECTION Section,
    __in PH_SYSINFO_SECTION_MESSAGE Message,
    __in_opt PVOID Parameter1,
    __in_opt PVOID Parameter2
    )
{
    switch (Message)
    {
    case SysInfoDestroy:
        {
            if (IoDialog)
            {
                PhSipUninitializeIoDialog();
                IoDialog = NULL;
            }
        }
        break;
    case SysInfoTick:
        {
            if (IoDialog)
            {
                PhSipTickIoDialog();
            }
        }
        break;
    case SysInfoCreateDialog:
        {
            PPH_SYSINFO_CREATE_DIALOG createDialog = Parameter1;

            createDialog->Instance = PhInstanceHandle;
            createDialog->Template = MAKEINTRESOURCE(IDD_SYSINFO_IO);
            createDialog->DialogProc = PhSipIoDialogProc;
        }
        return TRUE;
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

VOID PhSipInitializeIoDialog(
    VOID
    )
{
    PhInitializeDelta(&IoReadDelta);
    PhInitializeDelta(&IoWriteDelta);
    PhInitializeDelta(&IoOtherDelta);

    PhInitializeGraphState(&IoGraphState);

    IoTicked = 0;
}

VOID PhSipUninitializeIoDialog(
    VOID
    )
{
    PhDeleteGraphState(&IoGraphState);
}

VOID PhSipTickIoDialog(
    VOID
    )
{
    PhUpdateDelta(&IoReadDelta, PhPerfInformation.IoReadOperationCount);
    PhUpdateDelta(&IoWriteDelta, PhPerfInformation.IoWriteOperationCount);
    PhUpdateDelta(&IoOtherDelta, PhPerfInformation.IoOtherOperationCount);

    IoTicked++;

    if (IoTicked > 2)
        IoTicked = 2;

    PhSipUpdateIoGraph();
    PhSipUpdateIoPanel();
}

INT_PTR CALLBACK PhSipIoDialogProc(
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
            PPH_LAYOUT_ITEM graphItem;
            PPH_LAYOUT_ITEM panelItem;

            PhSipInitializeIoDialog();

            IoDialog = hwndDlg;
            PhInitializeLayoutManager(&IoLayoutManager, hwndDlg);
            graphItem = PhAddLayoutItem(&IoLayoutManager, GetDlgItem(hwndDlg, IDC_GRAPH_LAYOUT), NULL, PH_ANCHOR_ALL);
            panelItem = PhAddLayoutItem(&IoLayoutManager, GetDlgItem(hwndDlg, IDC_LAYOUT), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

            SendMessage(GetDlgItem(hwndDlg, IDC_TITLE), WM_SETFONT, (WPARAM)CurrentParameters.LargeFont, FALSE);

            IoPanel = CreateDialog(PhInstanceHandle, MAKEINTRESOURCE(IDD_SYSINFO_IOPANEL), hwndDlg, PhSipIoPanelDialogProc);
            ShowWindow(IoPanel, SW_SHOW);
            PhAddLayoutItemEx(&IoLayoutManager, IoPanel, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM, panelItem->Margin);

            IoGraphHandle = CreateWindow(
                PH_GRAPH_CLASSNAME,
                NULL,
                WS_VISIBLE | WS_CHILD | WS_BORDER,
                0,
                0,
                3,
                3,
                IoDialog,
                (HMENU)IDC_IO,
                PhInstanceHandle,
                NULL
                );
            Graph_SetTooltip(IoGraphHandle, TRUE);

            PhAddLayoutItemEx(&IoLayoutManager, IoGraphHandle, NULL, PH_ANCHOR_ALL, graphItem->Margin);

            PhSipUpdateIoGraph();
            PhSipUpdateIoPanel();
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&IoLayoutManager);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&IoLayoutManager);
        }
        break;
    case WM_NOTIFY:
        {
            NMHDR *header = (NMHDR *)lParam;

            if (header->hwndFrom == IoGraphHandle)
            {
                PhSipNotifyIoGraph(header);
            }
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK PhSipIoPanelDialogProc(
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
            NOTHING;
        }
        break;
    }

    return FALSE;
}

VOID PhSipNotifyIoGraph(
    __in NMHDR *Header
    )
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;
            ULONG i;

            drawInfo->Flags = PH_GRAPH_USE_GRID | PH_GRAPH_USE_LINE_2;
            PhSiSetColorsGraphDrawInfo(drawInfo, PhCsColorIoReadOther, PhCsColorIoWrite);

            PhGraphStateGetDrawInfo(
                &IoGraphState,
                getDrawInfo,
                PhIoReadHistory.Count
                );

            if (!IoGraphState.Valid)
            {
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

                // Minimum scaling of 1 MB.
                if (max < 1024 * 1024)
                    max = 1024 * 1024;

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

                IoGraphState.Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
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
                        PhGetStringOrEmpty(PhSipGetMaxIoString(getTooltipText->Index)),
                        ((PPH_STRING)PHA_DEREFERENCE(PhGetStatisticsTimeString(NULL, getTooltipText->Index)))->Buffer
                        ));
                }

                getTooltipText->Text = IoGraphState.TooltipText->sr;
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
                record = PhSipReferenceMaxIoRecord(mouseEvent->Index);
            }

            if (record)
            {
                PhShowProcessRecordDialog(IoDialog, record);
                PhDereferenceProcessRecord(record);
            }
        }
        break;
    }
}

VOID PhSipUpdateIoGraph(
    VOID
    )
{
    IoGraphState.Valid = FALSE;
    IoGraphState.TooltipIndex = -1;
    Graph_MoveGrid(IoGraphHandle, 1);
    Graph_Draw(IoGraphHandle);
    Graph_UpdateTooltip(IoGraphHandle);
    InvalidateRect(IoGraphHandle, NULL, FALSE);
}

VOID PhSipUpdateIoPanel(
    VOID
    )
{
    // I/O Deltas

    if (IoTicked > 1)
    {
        SetDlgItemText(IoPanel, IDC_ZREADSDELTA_V, PhaFormatUInt64(IoReadDelta.Delta, TRUE)->Buffer);
        SetDlgItemText(IoPanel, IDC_ZWRITESDELTA_V, PhaFormatUInt64(IoWriteDelta.Delta, TRUE)->Buffer);
        SetDlgItemText(IoPanel, IDC_ZOTHERDELTA_V, PhaFormatUInt64(IoOtherDelta.Delta, TRUE)->Buffer);
    }
    else
    {
        SetDlgItemText(IoPanel, IDC_ZREADSDELTA_V, L"-");
        SetDlgItemText(IoPanel, IDC_ZWRITESDELTA_V, L"-");
        SetDlgItemText(IoPanel, IDC_ZOTHERDELTA_V, L"-");
    }

    if (PhIoReadHistory.Count != 0)
    {
        SetDlgItemText(IoPanel, IDC_ZREADBYTESDELTA_V, PhaFormatSize(PhIoReadDelta.Delta, -1)->Buffer);
        SetDlgItemText(IoPanel, IDC_ZWRITEBYTESDELTA_V, PhaFormatSize(PhIoWriteDelta.Delta, -1)->Buffer);
        SetDlgItemText(IoPanel, IDC_ZOTHERBYTESDELTA_V, PhaFormatSize(PhIoOtherDelta.Delta, -1)->Buffer);
    }
    else
    {
        SetDlgItemText(IoPanel, IDC_ZREADBYTESDELTA_V, L"-");
        SetDlgItemText(IoPanel, IDC_ZWRITEBYTESDELTA_V, L"-");
        SetDlgItemText(IoPanel, IDC_ZOTHERBYTESDELTA_V, L"-");
    }

    // I/O Totals

    SetDlgItemText(IoPanel, IDC_ZREADS_V, PhaFormatUInt64(PhPerfInformation.IoReadOperationCount, TRUE)->Buffer);
    SetDlgItemText(IoPanel, IDC_ZREADBYTES_V, PhaFormatSize(PhPerfInformation.IoReadTransferCount.QuadPart, -1)->Buffer);
    SetDlgItemText(IoPanel, IDC_ZWRITES_V, PhaFormatUInt64(PhPerfInformation.IoWriteOperationCount, TRUE)->Buffer);
    SetDlgItemText(IoPanel, IDC_ZWRITEBYTES_V, PhaFormatSize(PhPerfInformation.IoWriteTransferCount.QuadPart, -1)->Buffer);
    SetDlgItemText(IoPanel, IDC_ZOTHER_V, PhaFormatUInt64(PhPerfInformation.IoOtherOperationCount, TRUE)->Buffer);
    SetDlgItemText(IoPanel, IDC_ZOTHERBYTES_V, PhaFormatSize(PhPerfInformation.IoOtherTransferCount.QuadPart, -1)->Buffer);
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
