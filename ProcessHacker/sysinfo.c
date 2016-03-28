/*
 * Process Hacker -
 *   System Information window
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
#include <procprv.h>
#include <settings.h>
#include <sysinfo.h>
#include <phplug.h>
#include <windowsx.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <sysinfop.h>

static HANDLE PhSipThread = NULL;
HWND PhSipWindow = NULL;
static PPH_LIST PhSipDialogList = NULL;
static PH_EVENT InitializedEvent = PH_EVENT_INIT;
static PWSTR InitialSectionName;
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

static HTHEME ThemeData;
static BOOLEAN ThemeHasItemBackground;

static BOOLEAN AlwaysOnTop;

VOID PhShowSystemInformationDialog(
    _In_opt_ PWSTR SectionName
    )
{
    InitialSectionName = SectionName;

    if (!PhSipWindow)
    {
        if (!(PhSipThread = PhCreateThread(0, PhSipSysInfoThreadStart, NULL)))
        {
            PhShowStatus(PhMainWndHandle, L"Unable to create the system information window", 0, GetLastError());
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
    case WM_SHOWWINDOW:
        {
            PhSipOnShowWindow(!!wParam, (ULONG)lParam);
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
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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
        CloseThemeData(ThemeData);
        ThemeData = NULL;
    }

    PhDeleteLayoutManager(&WindowLayoutManager);
    PostQuitMessage(0);
}

VOID PhSipOnShowWindow(
    _In_ BOOLEAN Showing,
    _In_ ULONG State
    )
{
    RECT buttonRect;
    RECT clientRect;
    PH_STRINGREF sectionName;
    PPH_SYSINFO_SECTION section;

    if (SectionList)
        return;

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

    EnableThemeDialogTexture(ContainerControl, ETDT_ENABLETAB);

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
            CurrentParameters.WindowPadding +
            CurrentParameters.MinimumGraphHeight * SectionList->Count +
            CurrentParameters.MinimumGraphHeight + // Back button
            CurrentParameters.GraphPadding * SectionList->Count +
            CurrentParameters.WindowPadding +
            clientRect.bottom - buttonRect.top +
            GetSystemMetrics(SM_CYFRAME) * 2;

        if (newMinimumHeight > (ULONG)MinimumSize.bottom)
            MinimumSize.bottom = newMinimumHeight;
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

    AlwaysOnTop = (BOOLEAN)PhGetIntegerSetting(L"SysInfoWindowAlwaysOnTop");
    Button_SetCheck(GetDlgItem(PhSipWindow, IDC_ALWAYSONTOP), AlwaysOnTop ? BST_CHECKED : BST_UNCHECKED);
    PhSipSetAlwaysOnTop();

    PhSipOnSize();
    PhSipOnUserMessage(SI_MSG_SYSINFO_UPDATE, 0, 0);
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
    _In_ ULONG Id,
    _In_ ULONG Code
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
                            drawInfo->LabelMaxYIndexLimit = -1;
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
    _In_ DRAWITEMSTRUCT *DrawItemStruct
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

            if (SectionList)
            {
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
    _In_ COLORREF Color2
    )
{
    static PH_QUEUED_LOCK lock = PH_QUEUED_LOCK_INIT;
    static ULONG lastDpi = -1;
    static HFONT iconTitleFont;

    // Get the appropriate fonts.

    if (DrawInfo->Flags & PH_GRAPH_LABEL_MAX_Y)
    {
        PhAcquireQueuedLockExclusive(&lock);

        if (lastDpi != PhGlobalDpi)
        {
            LOGFONT logFont;

            if (SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &logFont, 0))
            {
                logFont.lfHeight += PhMultiplyDivide(1, PhGlobalDpi, 72);
                iconTitleFont = CreateFontIndirect(&logFont);
            }

            if (!iconTitleFont)
                iconTitleFont = PhApplicationFont;

            lastDpi = PhGlobalDpi;
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

    size = (ULONG64)(Value * Parameter);

    if (size != 0)
    {
        PH_FORMAT format;

        format.Type = SizeFormatType | FormatUsePrecision | FormatUseRadix;
        format.Precision = 0;
        format.Radix = -1;
        format.u.Size = size;

        return PhFormat(&format, 1, 0);
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

    logFont.lfHeight -= PhMultiplyDivide(3, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    CurrentParameters.MediumFont = CreateFontIndirect(&logFont);

    logFont.lfHeight -= PhMultiplyDivide(3, GetDeviceCaps(hdc, LOGPIXELSY), 72);
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

    // Internal padding and other values
    CurrentParameters.PanelPadding = PH_SCALE_DPI(PH_SYSINFO_PANEL_PADDING);
    CurrentParameters.WindowPadding = PH_SCALE_DPI(PH_SYSINFO_WINDOW_PADDING);
    CurrentParameters.GraphPadding = PH_SCALE_DPI(PH_SYSINFO_GRAPH_PADDING);
    CurrentParameters.SmallGraphWidth = PH_SCALE_DPI(PH_SYSINFO_SMALL_GRAPH_WIDTH);
    CurrentParameters.SmallGraphPadding = PH_SCALE_DPI(PH_SYSINFO_SMALL_GRAPH_PADDING);
    CurrentParameters.SeparatorWidth = PH_SCALE_DPI(PH_SYSINFO_SEPARATOR_WIDTH);
    CurrentParameters.CpuPadding = PH_SCALE_DPI(PH_SYSINFO_CPU_PADDING);
    CurrentParameters.MemoryPadding = PH_SCALE_DPI(PH_SYSINFO_MEMORY_PADDING);

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
    _In_ PPH_SYSINFO_SECTION Template
    )
{
    PPH_SYSINFO_SECTION section;
    PH_GRAPH_OPTIONS options;

    section = PhAllocate(sizeof(PH_SYSINFO_SECTION));
    memset(section, 0, sizeof(PH_SYSINFO_SECTION));

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
        (HMENU)(ULONG_PTR)section->PanelId,
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
    _In_ HDC hdc,
    _In_ PRECT Rect
    )
{
    FillRect(hdc, Rect, GetSysColorBrush(COLOR_3DFACE));

    if (RestoreSummaryControlHot || RestoreSummaryControlHasFocus)
    {
        if (ThemeHasItemBackground)
        {
            DrawThemeBackground(
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
    _In_ HDC hdc,
    _In_ PRECT Rect
    )
{
    RECT rect;

    rect = *Rect;
    FillRect(hdc, &rect, GetSysColorBrush(COLOR_3DHIGHLIGHT));
    rect.left += 1;
    FillRect(hdc, &rect, GetSysColorBrush(COLOR_3DSHADOW));
}

VOID PhSipDrawPanel(
    _In_ PPH_SYSINFO_SECTION Section,
    _In_ HDC hdc,
    _In_ PRECT Rect
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

                DrawThemeBackground(
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
            DrawThemeBackground(
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
        SelectObject(hdc, CurrentParameters.MediumFont);
        DrawText(hdc, DrawPanel->Title->Buffer, (ULONG)DrawPanel->Title->Length / 2, &rect, flags | DT_SINGLELINE);
    }

    if (DrawPanel->SubTitle)
    {
        RECT measureRect;

        rect.top += CurrentParameters.MediumFontHeight + CurrentParameters.PanelPadding;
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

    availableHeight = buttonRect.top - CurrentParameters.WindowPadding * 2;
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

    availableHeight = buttonRect.top - CurrentParameters.WindowPadding * 2;
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

    deferHandle = BeginDeferWindowPos(SectionList->Count + 4);
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
    deferHandle = DeferWindowPos(deferHandle, GetDlgItem(PhSipWindow, IDC_INSTRUCTION), NULL, 0, 0, 0, 0, SWP_HIDEWINDOW_ONLY);

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
    ShowWindow(GetDlgItem(PhSipWindow, IDC_INSTRUCTION), SW_SHOW);

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
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
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
    if (ThemeData)
    {
        CloseThemeData(ThemeData);
        ThemeData = NULL;
    }

    ThemeData = OpenThemeData(PhSipWindow, L"TREEVIEW");

    if (ThemeData)
    {
        ThemeHasItemBackground = !!IsThemePartDefined(ThemeData, TVP_TREEITEM, 0);
    }
    else
    {
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

PPH_STRING PhSipFormatSizeWithPrecision(
    _In_ ULONG64 Size,
    _In_ USHORT Precision
    )
{
    PH_FORMAT format;

    format.Type = SizeFormatType | FormatUsePrecision;
    format.Precision = Precision;
    format.u.Size = Size;

    return PH_AUTO(PhFormat(&format, 1, 0));
}
