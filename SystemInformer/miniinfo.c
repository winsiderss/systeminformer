/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2015-2016
 *     dmex    2017-2023
 *
 */

#include <phapp.h>
#include <miniinfo.h>
#include <miniinfop.h>

#include <actions.h>
#include <mainwnd.h>
#include <notifico.h>
#include <phplug.h>
#include <phsettings.h>
#include <procprv.h>
#include <proctree.h>

#include <emenu.h>
#include <settings.h>

static HWND PhMipContainerWindow = NULL;
static POINT PhMipSourcePoint;
static LONG PhMipPinCounts[MaxMiniInfoPinType];
static LONG PhMipMaxPinCounts[] =
{
    1, // MiniInfoManualPinType
    1, // MiniInfoIconPinType
    1, // MiniInfoActivePinType
    1, // MiniInfoHoverPinType
    1, // MiniInfoChildControlPinType
};
C_ASSERT(sizeof(PhMipMaxPinCounts) / sizeof(LONG) == MaxMiniInfoPinType);
static LONG PhMipDelayedPinAdjustments[MaxMiniInfoPinType];
static PPH_MESSAGE_LOOP_FILTER_ENTRY PhMipMessageLoopFilterEntry;
static HWND PhMipLastTrackedWindow;
static HWND PhMipLastNcTrackedWindow;
static ULONG PhMipRefreshAutomatically;
static BOOLEAN PhMipPinned;

static HWND PhMipWindow = NULL;
static PH_LAYOUT_MANAGER PhMipLayoutManager;
static RECT MinimumSize;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
static PH_STRINGREF DownArrowPrefix = PH_STRINGREF_INIT(L"\u25be ");

static PPH_LIST SectionList;
static PH_MINIINFO_PARAMETERS CurrentParameters;
static PPH_MINIINFO_SECTION CurrentSection;

VOID PhPinMiniInformation(
    _In_ PH_MINIINFO_PIN_TYPE PinType,
    _In_ LONG PinCount,
    _In_opt_ ULONG PinDelayMs,
    _In_ ULONG Flags,
    _In_opt_ PWSTR SectionName,
    _In_opt_ PPOINT SourcePoint
    )
{
    PH_MIP_ADJUST_PIN_RESULT adjustPinResult;

    if (PinDelayMs && PinCount < 0)
    {
        PhMipDelayedPinAdjustments[PinType] = PinCount;

        if (PhMipContainerWindow)
        {
            PhSetTimer(PhMipContainerWindow, (UINT_PTR)MIP_TIMER_PIN_FIRST + PinType, PinDelayMs, NULL);
        }
        return;
    }
    else
    {
        PhMipDelayedPinAdjustments[PinType] = 0;

        if (PhMipContainerWindow)
        {
            PhKillTimer(PhMipContainerWindow, (UINT_PTR)MIP_TIMER_PIN_FIRST + PinType);
        }
    }

    adjustPinResult = PhMipAdjustPin(PinType, PinCount);

    if (adjustPinResult == ShowAdjustPinResult)
    {
        PH_RECTANGLE windowRectangle;
        ULONG opacity;

        if (SourcePoint)
            PhMipSourcePoint = *SourcePoint;

        if (!PhMipContainerWindow)
        {
            WNDCLASSEX wcex;
            RTL_ATOM windowAtom;
            PPH_STRING className;

            memset(&wcex, 0, sizeof(WNDCLASSEX));
            wcex.cbSize = sizeof(WNDCLASSEX);
            wcex.lpfnWndProc = PhMipContainerWndProc;
            wcex.hInstance = PhInstanceHandle;
            className = PhaGetStringSetting(L"MiniInfoWindowClassName");
            wcex.lpszClassName = PhGetStringOrDefault(className, L"MiniInfoWindowClassName");
            wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
            windowAtom = RegisterClassEx(&wcex);

            PhMipContainerWindow = CreateWindow(
                MAKEINTATOM(windowAtom),
                PhApplicationName,
                WS_BORDER | WS_THICKFRAME | WS_POPUP,
                0,
                0,
                400,
                400,
                NULL,
                NULL,
                NULL,
                NULL
                );
            PhSetWindowExStyle(PhMipContainerWindow, WS_EX_TOOLWINDOW, WS_EX_TOOLWINDOW);
            PhMipWindow = PhCreateDialog(
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_MINIINFO),
                PhMipContainerWindow,
                PhMipMiniInfoDialogProc,
                NULL
                );
            ShowWindow(PhMipWindow, SW_SHOW);

            if (PhGetIntegerSetting(L"MiniInfoWindowPinned"))
                PhMipSetPinned(TRUE);

            PhMipRefreshAutomatically = PhGetIntegerSetting(L"MiniInfoWindowRefreshAutomatically");

            opacity = PhGetIntegerSetting(L"MiniInfoWindowOpacity");

            if (opacity != 0)
                PhSetWindowOpacity(PhMipContainerWindow, opacity);

            MinimumSize.left = 0;
            MinimumSize.top = 0;
            MinimumSize.right = 210;
            MinimumSize.bottom = 60;
            MapDialogRect(PhMipWindow, &MinimumSize);
        }

        if (!(Flags & PH_MINIINFO_LOAD_POSITION))
        {
            PhMipCalculateWindowRectangle(&PhMipSourcePoint, &windowRectangle);
            SetWindowPos(
                PhMipContainerWindow,
                HWND_TOPMOST,
                windowRectangle.Left,
                windowRectangle.Top,
                windowRectangle.Width,
                windowRectangle.Height,
                SWP_NOACTIVATE
                );
        }
        else
        {
            PhLoadWindowPlacementFromSetting(L"MiniInfoWindowPosition", L"MiniInfoWindowSize", PhMipContainerWindow);
            SetWindowPos(PhMipContainerWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
        }

        PhInitializeWindowTheme(PhMipContainerWindow, PhEnableThemeSupport); // HACK

        ShowWindow(PhMipContainerWindow, (Flags & PH_MINIINFO_ACTIVATE_WINDOW) ? SW_SHOW : SW_SHOWNOACTIVATE);
    }
    else if (adjustPinResult == HideAdjustPinResult)
    {
        if (PhMipContainerWindow)
            ShowWindow(PhMipContainerWindow, SW_HIDE);
    }
    else
    {
        if ((Flags & PH_MINIINFO_ACTIVATE_WINDOW) && PhMipContainerWindow && IsWindowVisible(PhMipContainerWindow))
            SetActiveWindow(PhMipContainerWindow);
    }

    if ((Flags & PH_MINIINFO_ACTIVATE_WINDOW) && PhMipContainerWindow)
        SetForegroundWindow(PhMipContainerWindow);

    if (SectionName && (!PhMipPinned || !(Flags & PH_MINIINFO_DONT_CHANGE_SECTION_IF_PINNED)))
    {
        PH_STRINGREF sectionName;
        PPH_MINIINFO_SECTION section;

        PhInitializeStringRefLongHint(&sectionName, SectionName);

        if (section = PhMipFindSection(&sectionName))
            PhMipChangeSection(section);
    }
}

LRESULT CALLBACK PhMipContainerWndProc(
    _In_ HWND hWnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_SHOWWINDOW:
        {
            PhMipContainerOnShowWindow(!!wParam, (ULONG)lParam);
        }
        break;
    case WM_ACTIVATE:
        {
            PhMipContainerOnActivate(GET_WM_COMMAND_ID(wParam, lParam), !!HIWORD(wParam));
        }
        break;
    case WM_SIZE:
        {
            PhMipContainerOnSize();
        }
        break;
    case WM_SIZING:
        {
            PhMipContainerOnSizing((ULONG)wParam, (PRECT)lParam);
        }
        break;
    case WM_EXITSIZEMOVE:
        {
            PhMipContainerOnExitSizeMove();
        }
        break;
    case WM_CLOSE:
        {
            // Hide, don't close.
            ShowWindow(hWnd, SW_HIDE);
            SetWindowLongPtr(hWnd, DWLP_MSGRESULT, 0);
        }
        return TRUE;
    case WM_ERASEBKGND:
        {
            if (PhMipContainerOnEraseBkgnd((HDC)wParam))
                return TRUE;
        }
        break;
    case WM_TIMER:
        {
            PhMipContainerOnTimer((ULONG)wParam);
        }
        break;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

INT_PTR CALLBACK PhMipMiniInfoDialogProc(
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
            PhMipWindow = hwndDlg;
            PhMipOnInitDialog();
        }
        break;
    case WM_SHOWWINDOW:
        {
            PhMipOnShowWindow(!!wParam, (ULONG)lParam);
        }
        break;
    case WM_COMMAND:
        {
            PhMipOnCommand(GET_WM_COMMAND_ID(wParam, lParam), HIWORD(wParam));
        }
        break;
    case WM_NOTIFY:
        {
            LRESULT result;

            if (PhMipOnNotify((NMHDR *)lParam, &result))
            {
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, result);
                return TRUE;
            }
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        {
            HBRUSH brush;

            if (PhMipOnCtlColorXxx(uMsg, (HWND)lParam, (HDC)wParam, &brush))
                return (INT_PTR)brush;
        }
        break;
    case WM_DRAWITEM:
        {
            if (PhMipOnDrawItem(wParam, (DRAWITEMSTRUCT *)lParam))
                return TRUE;
        }
        break;
    }

    if (uMsg >= MIP_MSG_FIRST && uMsg <= MIP_MSG_LAST)
    {
        PhMipOnUserMessage(uMsg, wParam, lParam);
    }

    return FALSE;
}

VOID PhMipContainerOnShowWindow(
    _In_ BOOLEAN Showing,
    _In_ ULONG State
    )
{
    ULONG i;
    PPH_MINIINFO_SECTION section;

    if (Showing)
    {
        PostMessage(PhMipWindow, MIP_MSG_UPDATE, 0, 0);

        PhMipMessageLoopFilterEntry = PhRegisterMessageLoopFilter(PhMipMessageLoopFilter, NULL);

        PhRegisterCallback(
            PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
            PhMipUpdateHandler,
            NULL,
            &ProcessesUpdatedRegistration
            );

        PhMipContainerOnSize();
    }
    else
    {
        ULONG i;

        for (i = 0; i < MaxMiniInfoPinType; i++)
            PhMipPinCounts[i] = 0;

        Button_SetCheck(GetDlgItem(PhMipWindow, IDC_PINWINDOW), BST_UNCHECKED);
        PhMipSetPinned(FALSE);
        PhSetIntegerSetting(L"MiniInfoWindowPinned", FALSE);

        PhUnregisterCallback(
            PhGetGeneralCallback(GeneralCallbackProcessProviderUpdatedEvent),
            &ProcessesUpdatedRegistration
            );

        if (PhMipMessageLoopFilterEntry)
        {
            PhUnregisterMessageLoopFilter(PhMipMessageLoopFilterEntry);
            PhMipMessageLoopFilterEntry = NULL;
        }

        PhSaveWindowPlacementToSetting(L"MiniInfoWindowPosition", L"MiniInfoWindowSize", PhMipContainerWindow);
    }

    if (SectionList)
    {
        for (i = 0; i < SectionList->Count; i++)
        {
            section = SectionList->Items[i];
            section->Callback(section, MiniInfoShowing, (PVOID)Showing, NULL);
        }
    }
}

VOID PhMipContainerOnActivate(
    _In_ ULONG Type,
    _In_ BOOLEAN Minimized
    )
{
    if (Type == WA_ACTIVE || Type == WA_CLICKACTIVE)
    {
        PhPinMiniInformation(MiniInfoActivePinType, 1, 0, 0, NULL, NULL);
    }
    else if (Type == WA_INACTIVE)
    {
        PhPinMiniInformation(MiniInfoActivePinType, -1, 0, 0, NULL, NULL);
    }
}

VOID PhMipContainerOnSize(
    VOID
    )
{
    if (PhMipWindow)
    {
        InvalidateRect(PhMipContainerWindow, NULL, FALSE);
        PhMipLayout();
    }
}

VOID PhMipContainerOnSizing(
    _In_ ULONG Edge,
    _In_ PRECT DragRectangle
    )
{
    PhResizingMinimumSize(DragRectangle, Edge, MinimumSize.right, MinimumSize.bottom);
}

VOID PhMipContainerOnExitSizeMove(
    VOID
    )
{
    PhSaveWindowPlacementToSetting(L"MiniInfoWindowPosition", L"MiniInfoWindowSize", PhMipContainerWindow);
}

BOOLEAN PhMipContainerOnEraseBkgnd(
    _In_ HDC hdc
    )
{
    return FALSE;
}

VOID PhMipContainerOnTimer(
    _In_ ULONG Id
    )
{
    if (Id >= MIP_TIMER_PIN_FIRST && Id <= MIP_TIMER_PIN_LAST)
    {
        PH_MINIINFO_PIN_TYPE pinType = Id - MIP_TIMER_PIN_FIRST;

        // PhPinMiniInformation kills the timer for us.
        PhPinMiniInformation(pinType, PhMipDelayedPinAdjustments[pinType], 0, 0, NULL, NULL);
    }
}

VOID PhMipOnInitDialog(
    VOID
    )
{
    HICON cog;
    HICON pin;
    WNDPROC oldWndProc;
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(PhMipWindow);

    cog = PH_LOAD_SHARED_ICON_SMALL(PhInstanceHandle, MAKEINTRESOURCE(IDI_COG), dpiValue);
    SET_BUTTON_ICON(PhMipWindow, IDC_OPTIONS, cog);

    pin = PH_LOAD_SHARED_ICON_SMALL(PhInstanceHandle, MAKEINTRESOURCE(IDI_PIN), dpiValue);
    SET_BUTTON_ICON(PhMipWindow, IDC_PINWINDOW, pin);

    PhInitializeLayoutManager(&PhMipLayoutManager, PhMipWindow);
    PhAddLayoutItem(&PhMipLayoutManager, GetDlgItem(PhMipWindow, IDC_LAYOUT), NULL, PH_ANCHOR_ALL);
    PhAddLayoutItem(&PhMipLayoutManager, GetDlgItem(PhMipWindow, IDC_SECTION), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM | PH_LAYOUT_FORCE_INVALIDATE);
    PhAddLayoutItem(&PhMipLayoutManager, GetDlgItem(PhMipWindow, IDC_OPTIONS), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
    PhAddLayoutItem(&PhMipLayoutManager, GetDlgItem(PhMipWindow, IDC_PINWINDOW), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);

    Button_SetCheck(GetDlgItem(PhMipWindow, IDC_PINWINDOW), !!PhGetIntegerSetting(L"MiniInfoWindowPinned"));

    // Subclass the window procedure.
    oldWndProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(PhMipWindow, IDC_SECTION), GWLP_WNDPROC);
    PhSetWindowContext(GetDlgItem(PhMipWindow, IDC_SECTION), 0xF, oldWndProc);
    SetWindowLongPtr(GetDlgItem(PhMipWindow, IDC_SECTION), GWLP_WNDPROC, (LONG_PTR)PhMipSectionControlHookWndProc);
}

VOID PhMipOnShowWindow(
    _In_ BOOLEAN Showing,
    _In_ ULONG State
    )
{
    if (SectionList)
        return;

    SectionList = PhCreateList(8);
    PhMipInitializeParameters();

    SetWindowFont(GetDlgItem(PhMipWindow, IDC_SECTION), CurrentParameters.MediumFont, FALSE);

    PhMipCreateInternalListSection(L"CPU", 0, PhMipCpuListSectionCallback);
    PhMipCreateInternalListSection(L"Commit charge", 0, PhMipCommitListSectionCallback);
    PhMipCreateInternalListSection(L"Physical memory", 0, PhMipPhysicalListSectionCallback);
    PhMipCreateInternalListSection(L"I/O", 0, PhMipIoListSectionCallback);

    if (PhPluginsEnabled)
    {
        PH_PLUGIN_MINIINFO_POINTERS pointers;

        pointers.WindowHandle = PhMipContainerWindow;
        pointers.CreateSection = PhMipCreateSection;
        pointers.FindSection = PhMipFindSection;
        pointers.CreateListSection = PhMipCreateListSection;
        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMiniInformationInitializing), &pointers);
    }

    PhMipChangeSection(SectionList->Items[0]);
}

VOID PhMipOnCommand(
    _In_ ULONG Id,
    _In_ ULONG Code
    )
{
    switch (Id)
    {
    case IDC_SECTION:
        switch (Code)
        {
        case STN_CLICKED:
            PhMipShowSectionMenu();
            break;
        }
        break;
    case IDC_OPTIONS:
        PhMipShowOptionsMenu();
        break;
    case IDC_PINWINDOW:
        {
            BOOLEAN pinned;

            pinned = Button_GetCheck(GetDlgItem(PhMipWindow, IDC_PINWINDOW)) == BST_CHECKED;
            PhPinMiniInformation(MiniInfoManualPinType, pinned ? 1 : -1, 0, 0, NULL, NULL);
            PhMipSetPinned(pinned);
            PhSetIntegerSetting(L"MiniInfoWindowPinned", pinned);
        }
        break;
    }
}

_Success_(return)
BOOLEAN PhMipOnNotify(
    _In_ NMHDR *Header,
    _Out_ LRESULT *Result
    )
{
    return FALSE;
}

_Success_(return)
BOOLEAN PhMipOnCtlColorXxx(
    _In_ ULONG Message,
    _In_ HWND hwnd,
    _In_ HDC hdc,
    _Out_ HBRUSH *Brush
    )
{
    return FALSE;
}

BOOLEAN PhMipOnDrawItem(
    _In_ ULONG_PTR Id,
    _In_ DRAWITEMSTRUCT *DrawItemStruct
    )
{
    return FALSE;
}

VOID PhMipOnUserMessage(
    _In_ ULONG Message,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam
    )
{
    switch (Message)
    {
    case MIP_MSG_UPDATE:
        {
            ULONG i;
            PPH_MINIINFO_SECTION section;

            if (SectionList)
            {
                for (i = 0; i < SectionList->Count; i++)
                {
                    section = SectionList->Items[i];
                    section->Callback(section, MiniInfoTick, NULL, NULL);
                }
            }
        }
        break;
    }
}

BOOLEAN PhMipMessageLoopFilter(
    _In_ PMSG Message,
    _In_ PVOID Context
    )
{
    if (Message->hwnd == PhMipContainerWindow || IsChild(PhMipContainerWindow, Message->hwnd))
    {
        if (Message->message == WM_MOUSEMOVE || Message->message == WM_NCMOUSEMOVE)
        {
            TRACKMOUSEEVENT trackMouseEvent;

            trackMouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
            trackMouseEvent.dwFlags = TME_LEAVE | (Message->message == WM_NCMOUSEMOVE ? TME_NONCLIENT : 0);
            trackMouseEvent.hwndTrack = Message->hwnd;
            trackMouseEvent.dwHoverTime = 0;
            TrackMouseEvent(&trackMouseEvent);

            if (Message->message == WM_MOUSEMOVE)
                PhMipLastTrackedWindow = Message->hwnd;
            else
                PhMipLastNcTrackedWindow = Message->hwnd;

            PhPinMiniInformation(MiniInfoHoverPinType, 1, 0, 0, NULL, NULL);
        }
        else if (Message->message == WM_MOUSELEAVE && Message->hwnd == PhMipLastTrackedWindow)
        {
            PhPinMiniInformation(MiniInfoHoverPinType, -1, MIP_UNPIN_HOVER_DELAY, 0, NULL, NULL);
        }
        else if (Message->message == WM_NCMOUSELEAVE && Message->hwnd == PhMipLastNcTrackedWindow)
        {
            PhPinMiniInformation(MiniInfoHoverPinType, -1, MIP_UNPIN_HOVER_DELAY, 0, NULL, NULL);
        }
        else if (Message->message == WM_KEYDOWN)
        {
            switch (Message->wParam)
            {
            case VK_F5:
                PhMipRefresh();
                break;
            case VK_F6:
            case VK_PAUSE:
                PhMipToggleRefreshAutomatically();
                break;
            }
        }
    }

    return FALSE;
}

VOID NTAPI PhMipUpdateHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    if (PhMipRefreshAutomatically & MIP_REFRESH_AUTOMATICALLY_FLAG(PhMipPinned))
        PostMessage(PhMipWindow, MIP_MSG_UPDATE, 0, 0);
}

PH_MIP_ADJUST_PIN_RESULT PhMipAdjustPin(
    _In_ PH_MINIINFO_PIN_TYPE PinType,
    _In_ LONG PinCount
    )
{
    LONG oldTotalPinCount;
    LONG oldPinCount;
    LONG newPinCount;
    ULONG i;

    oldTotalPinCount = 0;

    for (i = 0; i < MaxMiniInfoPinType; i++)
        oldTotalPinCount += PhMipPinCounts[i];

    oldPinCount = PhMipPinCounts[PinType];
    newPinCount = max(oldPinCount + PinCount, 0);
    newPinCount = min(newPinCount, PhMipMaxPinCounts[PinType]);
    PhMipPinCounts[PinType] = newPinCount;

    if (oldTotalPinCount == 0 && newPinCount > oldPinCount)
        return ShowAdjustPinResult;
    else if (oldTotalPinCount > 0 && oldTotalPinCount - oldPinCount + newPinCount == 0)
        return HideAdjustPinResult;
    else
        return NoAdjustPinResult;
}

VOID PhMipCalculateWindowRectangle(
    _In_ PPOINT SourcePoint,
    _Out_ PPH_RECTANGLE WindowRectangle
    )
{
    RECT windowRect;
    PH_RECTANGLE windowRectangle;
    PH_RECTANGLE point;
    MONITORINFO monitorInfo = { sizeof(monitorInfo) };

    PhLoadWindowPlacementFromSetting(NULL, L"MiniInfoWindowSize", PhMipContainerWindow);
    GetWindowRect(PhMipContainerWindow, &windowRect);
    SendMessage(PhMipContainerWindow, WM_SIZING, WMSZ_BOTTOMRIGHT, (LPARAM)&windowRect); // Adjust for the minimum size.
    windowRectangle = PhRectToRectangle(windowRect);

    point.Left = SourcePoint->x;
    point.Top = SourcePoint->y;
    point.Width = 0;
    point.Height = 0;
    PhCenterRectangle(&windowRectangle, &point);

    if (GetMonitorInfo(
        MonitorFromPoint(*SourcePoint, MONITOR_DEFAULTTOPRIMARY),
        &monitorInfo
        ))
    {
        PH_RECTANGLE bounds;

        if (RtlEqualMemory(&monitorInfo.rcWork, &monitorInfo.rcMonitor, sizeof(RECT)))
        {
            HWND trayWindow;
            RECT taskbarRect;

            // The taskbar probably has auto-hide enabled. We need to adjust for that.

            if ((trayWindow = FindWindow(L"Shell_TrayWnd", NULL)) &&
                GetMonitorInfo(MonitorFromWindow(trayWindow, MONITOR_DEFAULTTOPRIMARY), &monitorInfo) && // Just in case
                GetWindowRect(trayWindow, &taskbarRect))
            {
                LONG monitorMidX = (monitorInfo.rcMonitor.left + monitorInfo.rcMonitor.right) / 2;
                LONG monitorMidY = (monitorInfo.rcMonitor.top + monitorInfo.rcMonitor.bottom) / 2;

                if (taskbarRect.right < monitorMidX)
                {
                    // Left
                    monitorInfo.rcWork.left += taskbarRect.right - taskbarRect.left;
                }
                else if (taskbarRect.bottom < monitorMidY)
                {
                    // Top
                    monitorInfo.rcWork.top += taskbarRect.bottom - taskbarRect.top;
                }
                else if (taskbarRect.left > monitorMidX)
                {
                    // Right
                    monitorInfo.rcWork.right -= taskbarRect.right - taskbarRect.left;
                }
                else if (taskbarRect.top > monitorMidY)
                {
                    // Bottom
                    monitorInfo.rcWork.bottom -= taskbarRect.bottom - taskbarRect.top;
                }
            }
        }

        bounds = PhRectToRectangle(monitorInfo.rcWork);

        PhAdjustRectangleToBounds(&windowRectangle, &bounds);
    }

    *WindowRectangle = windowRectangle;
}

VOID PhMipInitializeParameters(
    VOID
    )
{
    LOGFONT logFont;
    HDC hdc;
    TEXTMETRIC textMetrics;
    HFONT originalFont;
    LONG dpiValue;

    memset(&CurrentParameters, 0, sizeof(PH_MINIINFO_PARAMETERS));

    CurrentParameters.ContainerWindowHandle = PhMipContainerWindow;
    CurrentParameters.MiniInfoWindowHandle = PhMipWindow;

    dpiValue = PhGetWindowDpi(PhMipWindow);

    if (PhGetSystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &logFont, dpiValue))
    {
        CurrentParameters.Font = CreateFontIndirect(&logFont);
    }
    else
    {
        CurrentParameters.Font = PhApplicationFont;
        GetObject(PhApplicationFont, sizeof(LOGFONT), &logFont);
    }

    hdc = GetDC(PhMipWindow);

    logFont.lfHeight -= PhMultiplyDivide(2, dpiValue, 72);
    CurrentParameters.MediumFont = CreateFontIndirect(&logFont);

    originalFont = SelectFont(hdc, CurrentParameters.Font);
    GetTextMetrics(hdc, &textMetrics);
    CurrentParameters.FontHeight = textMetrics.tmHeight;
    CurrentParameters.FontAverageWidth = textMetrics.tmAveCharWidth;

    SelectFont(hdc, CurrentParameters.MediumFont);
    GetTextMetrics(hdc, &textMetrics);
    CurrentParameters.MediumFontHeight = textMetrics.tmHeight;
    CurrentParameters.MediumFontAverageWidth = textMetrics.tmAveCharWidth;

    CurrentParameters.SetSectionText = PhMipSetSectionText;

    SelectFont(hdc, originalFont);
    ReleaseDC(PhMipWindow, hdc);
}

PPH_MINIINFO_SECTION PhMipCreateSection(
    _In_ PPH_MINIINFO_SECTION Template
    )
{
    PPH_MINIINFO_SECTION section;

    section = PhAllocateZero(sizeof(PH_MINIINFO_SECTION));
    section->Name = Template->Name;
    section->Flags = Template->Flags;
    section->Callback = Template->Callback;
    section->Context = Template->Context;
    section->Parameters = &CurrentParameters;

    PhAddItemList(SectionList, section);

    section->Callback(section, MiniInfoCreate, NULL, NULL);

    return section;
}

VOID PhMipDestroySection(
    _In_ PPH_MINIINFO_SECTION Section
    )
{
    Section->Callback(Section, MiniInfoDestroy, NULL, NULL);

    PhClearReference(&Section->Text);
    PhFree(Section);
}

PPH_MINIINFO_SECTION PhMipFindSection(
    _In_ PPH_STRINGREF Name
    )
{
    ULONG i;
    PPH_MINIINFO_SECTION section;

    for (i = 0; i < SectionList->Count; i++)
    {
        section = SectionList->Items[i];

        if (PhEqualStringRef(&section->Name, Name, TRUE))
            return section;
    }

    return NULL;
}

PPH_MINIINFO_SECTION PhMipCreateInternalSection(
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_ PPH_MINIINFO_SECTION_CALLBACK Callback
    )
{
    PH_MINIINFO_SECTION section;

    memset(&section, 0, sizeof(PH_MINIINFO_SECTION));
    PhInitializeStringRefLongHint(&section.Name, Name);
    section.Flags = Flags;
    section.Callback = Callback;

    return PhMipCreateSection(&section);
}

VOID PhMipCreateSectionDialog(
    _In_ PPH_MINIINFO_SECTION Section
    )
{
    PH_MINIINFO_CREATE_DIALOG createDialog;

    memset(&createDialog, 0, sizeof(PH_MINIINFO_CREATE_DIALOG));

    if (Section->Callback(Section, MiniInfoCreateDialog, &createDialog, NULL))
    {
        if (!createDialog.CustomCreate)
        {
            Section->DialogHandle = PhCreateDialogFromTemplate(
                PhMipWindow,
                DS_SETFONT | DS_FIXEDSYS | DS_CONTROL | WS_CHILD,
                createDialog.Instance,
                createDialog.Template,
                createDialog.DialogProc,
                createDialog.Parameter
                );

            PhInitializeWindowTheme(Section->DialogHandle, PhEnableThemeSupport);
        }
    }
}

VOID PhMipChangeSection(
    _In_ PPH_MINIINFO_SECTION NewSection
    )
{
    PPH_MINIINFO_SECTION oldSection;

    if (NewSection == CurrentSection)
        return;

    oldSection = CurrentSection;
    CurrentSection = NewSection;

    if (oldSection)
    {
        oldSection->Callback(oldSection, MiniInfoSectionChanging, CurrentSection, NULL);

        if (oldSection->DialogHandle)
            ShowWindow(oldSection->DialogHandle, SW_HIDE);
    }

    if (!NewSection->DialogHandle)
        PhMipCreateSectionDialog(NewSection);
    if (NewSection->DialogHandle)
        ShowWindow(NewSection->DialogHandle, SW_SHOW);

    PhMipUpdateSectionText(NewSection);
    PhMipLayout();

    NewSection->Callback(NewSection, MiniInfoTick, NULL, NULL);
}

VOID PhMipSetSectionText(
    _In_ struct _PH_MINIINFO_SECTION *Section,
    _In_opt_ PPH_STRING Text
    )
{
    PhSwapReference(&Section->Text, Text);

    if (Section == CurrentSection)
        PhMipUpdateSectionText(Section);
}

VOID PhMipUpdateSectionText(
    _In_ PPH_MINIINFO_SECTION Section
    )
{
    if (Section->Text)
    {
        PhSetDialogItemText(PhMipWindow, IDC_SECTION,
            PH_AUTO_T(PH_STRING, PhConcatStringRef2(&DownArrowPrefix, &Section->Text->sr))->Buffer);
    }
    else
    {
        PhSetDialogItemText(PhMipWindow, IDC_SECTION,
            PH_AUTO_T(PH_STRING, PhConcatStringRef2(&DownArrowPrefix, &Section->Name))->Buffer);
    }
}

VOID PhMipLayout(
    VOID
    )
{
    RECT clientRect;
    RECT rect;

    GetClientRect(PhMipContainerWindow, &clientRect);
    MoveWindow(
        PhMipWindow,
        clientRect.left, clientRect.top,
        clientRect.right - clientRect.left, clientRect.bottom - clientRect.top,
        FALSE
        );

    PhLayoutManagerLayout(&PhMipLayoutManager);

    GetWindowRect(GetDlgItem(PhMipWindow, IDC_LAYOUT), &rect);
    MapWindowPoints(NULL, PhMipWindow, (POINT *)&rect, 2);

    if (CurrentSection && CurrentSection->DialogHandle)
    {
        if (CurrentSection->Flags & PH_MINIINFO_SECTION_NO_UPPER_MARGINS)
        {
            rect.left = 0;
            rect.top = 0;
            rect.right = clientRect.right;
        }
        else
        {
            LONG leftDistance = rect.left - clientRect.left;
            LONG rightDistance = clientRect.right - rect.right;
            LONG minDistance;

            if (leftDistance != rightDistance)
            {
                // HACK: Enforce symmetry. Sometimes these are off by a pixel.
                minDistance = min(leftDistance, rightDistance);
                rect.left = clientRect.left + minDistance;
                rect.right = clientRect.right - minDistance;
            }
        }

        MoveWindow(
            CurrentSection->DialogHandle,
            rect.left, rect.top,
            rect.right - rect.left, rect.bottom - rect.top,
            TRUE
            );
    }

    GetWindowRect(GetDlgItem(PhMipWindow, IDC_PINWINDOW), &rect);
    MapWindowPoints(NULL, PhMipWindow, (POINT *)&rect, 2);
}

VOID PhMipBeginChildControlPin(
    VOID
    )
{
    PhPinMiniInformation(MiniInfoChildControlPinType, 1, 0, 0, NULL, NULL);
}

VOID PhMipEndChildControlPin(
    VOID
    )
{
    PhPinMiniInformation(MiniInfoChildControlPinType, -1, MIP_UNPIN_CHILD_CONTROL_DELAY, 0, NULL, NULL);
    PostMessage(PhMipWindow, WM_MOUSEMOVE, 0, 0); // Re-evaluate hover pin
}

VOID PhMipRefresh(
    VOID
    )
{
    if (PhMipPinned)
        ProcessHacker_Refresh();

    PostMessage(PhMipWindow, MIP_MSG_UPDATE, 0, 0);
}

VOID PhMipToggleRefreshAutomatically(
    VOID
    )
{
    PhMipRefreshAutomatically ^= MIP_REFRESH_AUTOMATICALLY_FLAG(PhMipPinned);
    PhSetIntegerSetting(L"MiniInfoWindowRefreshAutomatically", PhMipRefreshAutomatically);
}

VOID PhMipSetPinned(
    _In_ BOOLEAN Pinned
    )
{
    PhSetWindowStyle(PhMipContainerWindow, WS_DLGFRAME | WS_SYSMENU, Pinned ? (WS_DLGFRAME | WS_SYSMENU) : 0);
    SetWindowPos(PhMipContainerWindow, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
    PhMipPinned = Pinned;

    PhNfNotifyMiniInfoPinned(Pinned);
}

VOID PhMipShowSectionMenu(
    VOID
    )
{
    PPH_EMENU menu;
    ULONG i;
    PPH_MINIINFO_SECTION section;
    PPH_EMENU_ITEM menuItem;
    POINT point;

    PhMipBeginChildControlPin();
    menu = PhCreateEMenu();

    for (i = 0; i < SectionList->Count; i++)
    {
        section = SectionList->Items[i];
        menuItem = PhCreateEMenuItem(
            (section == CurrentSection ? (PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK) : 0),
            0,
            PH_AUTO_T(PH_STRING, PhCreateString2(&section->Name))->Buffer,
            NULL,
            section
            );
        PhInsertEMenuItem(menu, menuItem, ULONG_MAX);
    }

    GetCursorPos(&point);
    menuItem = PhShowEMenu(menu, PhMipWindow, PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_TOP, point.x, point.y);

    if (menuItem)
    {
        PhMipChangeSection(menuItem->Context);
    }

    PhDestroyEMenu(menu);
    PhMipEndChildControlPin();
}

PPH_EMENU PhpMipCreateMenu(
    VOID
    )
{
    PPH_EMENU menu;
    PPH_EMENU_ITEM menuItem;

    menu = PhCreateEMenu();
    menuItem = PhCreateEMenuItem(0, 0, L"&Opacity", NULL, NULL);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_10, L"10%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_20, L"20%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_30, L"30%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_40, L"40%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_50, L"50%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_60, L"60%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_70, L"70%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_80, L"80%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_90, L"90%", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menuItem, PhCreateEMenuItem(0, ID_OPACITY_OPAQUE, L"Opaque", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, menuItem, ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_MINIINFO_REFRESH, L"&Refresh\bF5", NULL, NULL), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_MINIINFO_REFRESHAUTOMATICALLY, L"Refresh a&utomatically\bF6", NULL, NULL), ULONG_MAX);

    return menu;
}

VOID PhMipShowOptionsMenu(
    VOID
    )
{
    PPH_EMENU menu;
    PPH_EMENU_ITEM menuItem;
    ULONG id;
    RECT rect;

    PhMipBeginChildControlPin();

    // Menu

    menu = PhpMipCreateMenu();

    // Opacity

    id = PH_OPACITY_TO_ID(PhGetIntegerSetting(L"MiniInfoWindowOpacity"));

    if (menuItem = PhFindEMenuItem(menu, PH_EMENU_FIND_DESCEND, NULL, id))
        menuItem->Flags |= PH_EMENU_CHECKED | PH_EMENU_RADIOCHECK;

    // Refresh Automatically

    if (PhMipRefreshAutomatically & MIP_REFRESH_AUTOMATICALLY_FLAG(PhMipPinned))
        PhSetFlagsEMenuItem(menu, ID_MINIINFO_REFRESHAUTOMATICALLY, PH_EMENU_CHECKED, PH_EMENU_CHECKED);

    // Show the menu.

    GetWindowRect(GetDlgItem(PhMipWindow, IDC_OPTIONS), &rect);
    menuItem = PhShowEMenu(menu, PhMipWindow, PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_BOTTOM, rect.left, rect.top);

    if (menuItem)
    {
        switch (menuItem->Id)
        {
        case ID_OPACITY_10:
        case ID_OPACITY_20:
        case ID_OPACITY_30:
        case ID_OPACITY_40:
        case ID_OPACITY_50:
        case ID_OPACITY_60:
        case ID_OPACITY_70:
        case ID_OPACITY_80:
        case ID_OPACITY_90:
        case ID_OPACITY_OPAQUE:
            {
                ULONG opacity;

                opacity = PH_ID_TO_OPACITY(menuItem->Id);
                PhSetIntegerSetting(L"MiniInfoWindowOpacity", opacity);
                PhSetWindowOpacity(PhMipContainerWindow, opacity);
            }
            break;
        case ID_MINIINFO_REFRESH:
            PhMipRefresh();
            break;
        case ID_MINIINFO_REFRESHAUTOMATICALLY:
            PhMipToggleRefreshAutomatically();
            break;
        }
    }

    PhDestroyEMenu(menu);
    PhMipEndChildControlPin();
}

LRESULT CALLBACK PhMipSectionControlHookWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    WNDPROC oldWndProc;

    if (!(oldWndProc = PhGetWindowContext(hwnd, 0xF)))
        return 0;

    switch (uMsg)
    {
    case WM_DESTROY:
        {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)oldWndProc);
            PhRemoveWindowContext(hwnd, 0xF);
        }
        break;
    case WM_SETCURSOR:
        {
            SetCursor(LoadCursor(NULL, IDC_HAND));
        }
        return TRUE;
    }

    return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
}

PPH_MINIINFO_LIST_SECTION PhMipCreateListSection(
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_ PPH_MINIINFO_LIST_SECTION Template
    )
{
    PPH_MINIINFO_LIST_SECTION listSection;
    PH_MINIINFO_SECTION section;

    listSection = PhAllocateZero(sizeof(PH_MINIINFO_LIST_SECTION));
    listSection->Context = Template->Context;
    listSection->Callback = Template->Callback;

    memset(&section, 0, sizeof(PH_MINIINFO_SECTION));
    PhInitializeStringRefLongHint(&section.Name, Name);
    section.Flags = PH_MINIINFO_SECTION_NO_UPPER_MARGINS;
    section.Callback = PhMipListSectionCallback;
    section.Context = listSection;
    listSection->Section = PhMipCreateSection(&section);

    return listSection;
}

PPH_MINIINFO_LIST_SECTION PhMipCreateInternalListSection(
    _In_ PWSTR Name,
    _In_ ULONG Flags,
    _In_ PPH_MINIINFO_LIST_SECTION_CALLBACK Callback
    )
{
    PH_MINIINFO_LIST_SECTION listSection;

    memset(&listSection, 0, sizeof(PH_MINIINFO_LIST_SECTION));
    listSection.Callback = Callback;

    return PhMipCreateListSection(Name, Flags, &listSection);
}

BOOLEAN PhMipListSectionCallback(
    _In_ PPH_MINIINFO_SECTION Section,
    _In_ PH_MINIINFO_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    PPH_MINIINFO_LIST_SECTION listSection = Section->Context;

    switch (Message)
    {
    case MiniInfoCreate:
        {
            listSection->NodeList = PhCreateList(2);
            listSection->Callback(listSection, MiListSectionCreate, NULL, NULL);
        }
        break;
    case MiniInfoDestroy:
        {
            listSection->Callback(listSection, MiListSectionDestroy, NULL, NULL);

            PhMipClearListSection(listSection);
            PhDereferenceObject(listSection->NodeList);
            PhFree(listSection);
        }
        break;
    case MiniInfoTick:
        if (listSection->SuspendUpdate == 0)
            PhMipTickListSection(listSection);
        break;
    case MiniInfoShowing:
        {
            listSection->Callback(listSection, MiListSectionShowing, Parameter1, Parameter2);

            if (!Parameter1) // Showing
            {
                // We don't want to hold process item references while the mini info window
                // is hidden.
                PhMipClearListSection(listSection);
                TreeNew_NodesStructured(listSection->TreeNewHandle);
            }
        }
        break;
    case MiniInfoCreateDialog:
        {
            PPH_MINIINFO_CREATE_DIALOG createDialog = Parameter1;

            if (!createDialog)
                break;

            createDialog->Instance = PhInstanceHandle;
            createDialog->Template = MAKEINTRESOURCE(IDD_MINIINFO_LIST);
            createDialog->DialogProc = PhMipListSectionDialogProc;
            createDialog->Parameter = listSection;
        }
        return TRUE;
    }

    return FALSE;
}

INT_PTR CALLBACK PhMipListSectionDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPH_MINIINFO_LIST_SECTION listSection;

    if (uMsg == WM_INITDIALOG)
    {
        listSection = (PPH_MINIINFO_LIST_SECTION)lParam;
        PhSetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT, listSection);
    }
    else
    {
        listSection = PhGetWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (!listSection)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PPH_LAYOUT_ITEM layoutItem;

            listSection->DialogHandle = hwndDlg;
            listSection->TreeNewHandle = GetDlgItem(hwndDlg, IDC_LIST);

            PhInitializeLayoutManager(&listSection->LayoutManager, hwndDlg);
            layoutItem = PhAddLayoutItem(&listSection->LayoutManager, listSection->TreeNewHandle, NULL, PH_ANCHOR_ALL);

            // Use negative margins to maximize our use of the window area.
            layoutItem->Margin.left = -1;
            layoutItem->Margin.top = -1;
            layoutItem->Margin.right = -1;

            PhSetControlTheme(listSection->TreeNewHandle, L"explorer");
            TreeNew_SetCallback(listSection->TreeNewHandle, PhMipListSectionTreeNewCallback, listSection);
            TreeNew_SetRowHeight(listSection->TreeNewHandle, PhMipCalculateRowHeight(hwndDlg));
            TreeNew_SetRedraw(listSection->TreeNewHandle, FALSE);
            PhAddTreeNewColumnEx2(listSection->TreeNewHandle, MIP_SINGLE_COLUMN_ID, TRUE, L"Process", 1,
                PH_ALIGN_LEFT, 0, 0, TN_COLUMN_FLAG_CUSTOMDRAW);
            TreeNew_SetRedraw(listSection->TreeNewHandle, TRUE);

            listSection->Callback(listSection, MiListSectionDialogCreated, hwndDlg, NULL);
            PhMipTickListSection(listSection);
        }
        break;
    case WM_DESTROY:
        {
            PhDeleteLayoutManager(&listSection->LayoutManager);
            PhRemoveWindowContext(hwndDlg, PH_WINDOW_CONTEXT_DEFAULT);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&listSection->LayoutManager);
            TreeNew_AutoSizeColumn(listSection->TreeNewHandle, MIP_SINGLE_COLUMN_ID, TN_AUTOSIZE_REMAINING_SPACE);
        }
        break;
    }

    return FALSE;
}

VOID PhMipListSectionSortFunction(
    _In_ PPH_LIST List,
    _In_opt_ PVOID Context
    )
{
    PPH_MINIINFO_LIST_SECTION listSection = Context;
    PH_MINIINFO_LIST_SECTION_SORT_LIST sortList;

    if (!listSection)
        return;

    sortList.List = List;
    listSection->Callback(listSection, MiListSectionSortProcessList, &sortList, NULL);
}

VOID PhMipTickListSection(
    _In_ PPH_MINIINFO_LIST_SECTION ListSection
    )
{
    ULONG i;
    PPH_MIP_GROUP_NODE node;
    PH_MINIINFO_LIST_SECTION_ASSIGN_SORT_DATA assignSortData;

    PhMipClearListSection(ListSection);

    ListSection->ProcessGroupList = PhCreateProcessGroupList(
        PhMipListSectionSortFunction,
        ListSection,
        MIP_MAX_PROCESS_GROUPS,
        0
        );

    if (!ListSection->ProcessGroupList)
        return;

    for (i = 0; i < ListSection->ProcessGroupList->Count; i++)
    {
        node = PhMipAddGroupNode(ListSection, ListSection->ProcessGroupList->Items[i]);

        if (node->RepresentativeProcessId == ListSection->SelectedRepresentativeProcessId &&
            node->RepresentativeCreateTime.QuadPart == ListSection->SelectedRepresentativeCreateTime.QuadPart)
        {
            node->Node.Selected = TRUE;
        }

        assignSortData.ProcessGroup = node->ProcessGroup;
        assignSortData.SortData = &node->SortData;
        ListSection->Callback(ListSection, MiListSectionAssignSortData, &assignSortData, NULL);
    }

    TreeNew_NodesStructured(ListSection->TreeNewHandle);
    TreeNew_AutoSizeColumn(ListSection->TreeNewHandle, MIP_SINGLE_COLUMN_ID, TN_AUTOSIZE_REMAINING_SPACE);

    ListSection->Callback(ListSection, MiListSectionTick, NULL, NULL);
}

VOID PhMipClearListSection(
    _In_ PPH_MINIINFO_LIST_SECTION ListSection
    )
{
    if (ListSection->ProcessGroupList)
    {
        PhFreeProcessGroupList(ListSection->ProcessGroupList);
        ListSection->ProcessGroupList = NULL;
    }

    for (ULONG i = 0; i < ListSection->NodeList->Count; i++)
        PhMipDestroyGroupNode(ListSection->NodeList->Items[i]);

    PhClearList(ListSection->NodeList);
}

LONG PhMipCalculateRowHeight(
    _In_ HWND hwnd
    )
{
    LONG iconHeight;
    LONG titleAndSubtitleHeight;
    LONG dpiValue;

    dpiValue = PhGetWindowDpi(hwnd);

    iconHeight = PhGetDpi(MIP_ICON_PADDING + MIP_CELL_PADDING, dpiValue) + PhGetSystemMetrics(SM_CXICON, dpiValue);
    titleAndSubtitleHeight =
        PhGetDpi(MIP_CELL_PADDING, dpiValue) * 2 + CurrentParameters.FontHeight + PhGetDpi(MIP_INNER_PADDING, dpiValue) + CurrentParameters.FontHeight;

    return max(iconHeight, titleAndSubtitleHeight);
}

PPH_MIP_GROUP_NODE PhMipAddGroupNode(
    _In_ PPH_MINIINFO_LIST_SECTION ListSection,
    _In_ PPH_PROCESS_GROUP ProcessGroup
    )
{
    PPH_MIP_GROUP_NODE node;

    node = PhAllocate(sizeof(PH_MIP_GROUP_NODE));
    memset(node, 0, sizeof(PH_MIP_GROUP_NODE));

    PhInitializeTreeNewNode(&node->Node);
    node->ProcessGroup = ProcessGroup;
    node->RepresentativeProcessId = ProcessGroup->Representative->ProcessId;
    node->RepresentativeCreateTime = ProcessGroup->Representative->CreateTime;
    node->RepresentativeIsHung = ProcessGroup->WindowHandle && IsHungAppWindow(ProcessGroup->WindowHandle);

    if (node->RepresentativeIsHung)
    {
        // Make sure this is a real hung window, not a ghost window.
        if (PhHungWindowFromGhostWindow(ProcessGroup->WindowHandle))
            node->RepresentativeIsHung = FALSE;
    }

    PhAddItemList(ListSection->NodeList, node);

    return node;
}

VOID PhMipDestroyGroupNode(
    _In_ PPH_MIP_GROUP_NODE Node
    )
{
    PhClearReference(&Node->TooltipText);
    PhFree(Node);
}

BOOLEAN PhMipListSectionTreeNewCallback(
    _In_ HWND hwnd,
    _In_ PH_TREENEW_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2,
    _In_opt_ PVOID Context
    )
{
    PPH_MINIINFO_LIST_SECTION listSection = Context;

    switch (Message)
    {
    case TreeNewGetChildren:
        {
            PPH_TREENEW_GET_CHILDREN getChildren = Parameter1;
            PH_MINIINFO_LIST_SECTION_SORT_LIST sortList;

            if (!getChildren->Node)
            {
                getChildren->Children = (PPH_TREENEW_NODE *)listSection->NodeList->Items;
                getChildren->NumberOfChildren = listSection->NodeList->Count;

                sortList.List = listSection->NodeList;
                listSection->Callback(listSection, MiListSectionSortGroupList, &sortList, NULL);
            }
        }
        return TRUE;
    case TreeNewIsLeaf:
        {
            PPH_TREENEW_IS_LEAF isLeaf = Parameter1;
            isLeaf->IsLeaf = TRUE;
        }
        return TRUE;
    case TreeNewCustomDraw:
        {
            PPH_TREENEW_CUSTOM_DRAW customDraw = Parameter1;
            PPH_MIP_GROUP_NODE node = (PPH_MIP_GROUP_NODE)customDraw->Node;
            PPH_PROCESS_ITEM processItem = node->ProcessGroup->Representative;
            HDC hdc = customDraw->Dc;
            RECT rect = customDraw->CellRect;
            ULONG baseTextFlags = DT_NOPREFIX | DT_VCENTER | DT_SINGLELINE;
            HICON icon;
            COLORREF originalTextColor;
            RECT topRect;
            RECT bottomRect;
            PH_MINIINFO_LIST_SECTION_GET_USAGE_TEXT getUsageText;
            ULONG usageTextTopWidth = 0;
            ULONG usageTextBottomWidth = 0;
            PH_MINIINFO_LIST_SECTION_GET_TITLE_TEXT getTitleText;
            LONG dpiValue;
            LONG width;
            LONG height;
            LONG iconPadding;
            LONG cellPadding;

            dpiValue = PhGetWindowDpi(hwnd);

            width = PhGetSystemMetrics(SM_CXICON, dpiValue);
            height = PhGetSystemMetrics(SM_CYICON, dpiValue);

            iconPadding = PhGetDpi(MIP_ICON_PADDING, dpiValue);
            cellPadding = PhGetDpi(MIP_CELL_PADDING, dpiValue);

            rect.left += iconPadding;
            rect.top += iconPadding;
            rect.right -= cellPadding;
            rect.bottom -= cellPadding;

            icon = PhGetImageListIcon(processItem->LargeIconIndex, TRUE);
            DrawIconEx(hdc, rect.left, rect.top, icon, width, height, 0, NULL, DI_NORMAL);
            DestroyIcon(icon);

            rect.left += (cellPadding - iconPadding) + width + cellPadding;
            rect.top += cellPadding - iconPadding;
            SelectFont(hdc, CurrentParameters.Font);

            // This color changes depending on whether the node is selected, etc.
            originalTextColor = GetTextColor(hdc);

            // Usage text

            topRect = rect;
            topRect.bottom = topRect.top + CurrentParameters.FontHeight;
            bottomRect = rect;
            bottomRect.top = bottomRect.bottom - CurrentParameters.FontHeight;

            getUsageText.ProcessGroup = node->ProcessGroup;
            getUsageText.SortData = &node->SortData;
            getUsageText.Line1 = NULL;
            getUsageText.Line2 = NULL;
            getUsageText.Line1Color = originalTextColor;
            getUsageText.Line2Color = originalTextColor;

            if (listSection->Callback(listSection, MiListSectionGetUsageText, &getUsageText, NULL))
            {
                PH_STRINGREF text;
                RECT textRect;
                SIZE textSize;

                // Top
                text = PhGetStringRef(getUsageText.Line1);
                GetTextExtentPoint32(hdc, text.Buffer, (ULONG)text.Length / sizeof(WCHAR), &textSize);
                usageTextTopWidth = textSize.cx;
                textRect = topRect;
                textRect.left = textRect.right - textSize.cx;
                SetTextColor(hdc, getUsageText.Line1Color);
                DrawText(hdc, text.Buffer, (ULONG)text.Length / sizeof(WCHAR), &textRect, baseTextFlags | DT_RIGHT);
                PhClearReference(&getUsageText.Line1);

                // Bottom
                text = PhGetStringRef(getUsageText.Line2);
                GetTextExtentPoint32(hdc, text.Buffer, (ULONG)text.Length / sizeof(WCHAR), &textSize);
                usageTextBottomWidth = textSize.cx;
                textRect = bottomRect;
                textRect.left = textRect.right - textSize.cx;
                SetTextColor(hdc, getUsageText.Line2Color);
                DrawText(hdc, text.Buffer, (ULONG)text.Length / sizeof(WCHAR), &textRect, baseTextFlags | DT_RIGHT);
                PhClearReference(&getUsageText.Line2);
            }

            // Title, subtitle

            getTitleText.ProcessGroup = node->ProcessGroup;
            getTitleText.SortData = &node->SortData;

            if (!PhIsNullOrEmptyString(processItem->VersionInfo.FileDescription))
                PhSetReference(&getTitleText.Title, processItem->VersionInfo.FileDescription);
            else
                PhSetReference(&getTitleText.Title, processItem->ProcessName);

            if (node->ProcessGroup->Processes->Count == 1)
            {
                PhSetReference(&getTitleText.Subtitle, processItem->ProcessName);
            }
            else
            {
                getTitleText.Subtitle = PhFormatString(
                    L"%s (%u processes)",
                    processItem->ProcessName->Buffer,
                    node->ProcessGroup->Processes->Count
                    );
            }

            getTitleText.TitleColor = originalTextColor;
            getTitleText.SubtitleColor = GetSysColor(COLOR_GRAYTEXT);

            // Special text for hung windows
            if (node->RepresentativeIsHung)
            {
                static PH_STRINGREF hungPrefix = PH_STRINGREF_INIT(L"(Not responding) ");

                PhMoveReference(&getTitleText.Title, PhConcatStringRef2(&hungPrefix, &getTitleText.Title->sr));
                getTitleText.TitleColor = RGB(0xff, 0x00, 0x00);
            }

            listSection->Callback(listSection, MiListSectionGetTitleText, &getTitleText, NULL);

            if (!PhIsNullOrEmptyString(getTitleText.Title))
            {
                RECT textRect;

                textRect = topRect;
                textRect.right -= usageTextTopWidth + MIP_INNER_PADDING;
                SetTextColor(hdc, getTitleText.TitleColor);
                DrawText(
                    hdc,
                    getTitleText.Title->Buffer,
                    (ULONG)getTitleText.Title->Length / sizeof(WCHAR),
                    &textRect,
                    baseTextFlags | DT_END_ELLIPSIS
                    );
            }

            if (!PhIsNullOrEmptyString(getTitleText.Subtitle))
            {
                RECT textRect;

                textRect = bottomRect;
                textRect.right -= usageTextBottomWidth + MIP_INNER_PADDING;
                SetTextColor(hdc, getTitleText.SubtitleColor);
                DrawText(
                    hdc,
                    getTitleText.Subtitle->Buffer,
                    (ULONG)getTitleText.Subtitle->Length / sizeof(WCHAR),
                    &textRect,
                    baseTextFlags | DT_END_ELLIPSIS
                    );
            }

            PhClearReference(&getTitleText.Title);
            PhClearReference(&getTitleText.Subtitle);
        }
        return TRUE;
    case TreeNewGetCellTooltip:
        {
            PPH_TREENEW_GET_CELL_TOOLTIP getCellTooltip = Parameter1;
            PPH_MIP_GROUP_NODE node = (PPH_MIP_GROUP_NODE)getCellTooltip->Node;

            // This is useless most of the time because the tooltip doesn't display unless the window is active.
            // TODO: Find a way to make the tooltip display all the time.

            if (!node->TooltipText)
                node->TooltipText = PhMipGetGroupNodeTooltip(listSection, node);

            if (!PhIsNullOrEmptyString(node->TooltipText))
            {
                getCellTooltip->Text = node->TooltipText->sr;
                getCellTooltip->Unfolding = FALSE;
                getCellTooltip->MaximumWidth = ULONG_MAX;
            }
            else
            {
                return FALSE;
            }
        }
        return TRUE;
    case TreeNewSelectionChanged:
        {
            ULONG i;
            PPH_MIP_GROUP_NODE node;

            listSection->SelectedRepresentativeProcessId = NULL;
            listSection->SelectedRepresentativeCreateTime.QuadPart = 0;

            for (i = 0; i < listSection->NodeList->Count; i++)
            {
                node = listSection->NodeList->Items[i];

                if (node->Node.Selected)
                {
                    listSection->SelectedRepresentativeProcessId = node->RepresentativeProcessId;
                    listSection->SelectedRepresentativeCreateTime = node->RepresentativeCreateTime;
                    break;
                }
            }
        }
        break;
    case TreeNewKeyDown:
        {
            PPH_TREENEW_KEY_EVENT keyEvent = Parameter1;
            PPH_MIP_GROUP_NODE node;

            listSection->SuspendUpdate++;

            switch (keyEvent->VirtualKey)
            {
            case VK_DELETE:
                if (node = PhMipGetSelectedGroupNode(listSection))
                    PhUiTerminateProcesses(listSection->DialogHandle, &node->ProcessGroup->Representative, 1);
                break;
            case VK_RETURN:
                if (node = PhMipGetSelectedGroupNode(listSection))
                {
                    if (GetKeyState(VK_CONTROL) >= 0)
                    {
                        PhMipHandleListSectionCommand(listSection, node->ProcessGroup, ID_PROCESS_GOTOPROCESS);
                    }
                    else
                    {
                        if (node->ProcessGroup->Representative->FileName)
                        {
                            PhShellExecuteUserString(
                                listSection->DialogHandle,
                                L"FileBrowseExecutable",
                                node->ProcessGroup->Representative->FileName->Buffer,
                                FALSE,
                                L"Make sure the Explorer executable file is present."
                                );
                        }
                    }
                }
                break;
            }

            listSection->SuspendUpdate--;
        }
        return TRUE;
    case TreeNewLeftDoubleClick:
        {
            PPH_TREENEW_MOUSE_EVENT mouseEvent = Parameter1;
            PPH_MIP_GROUP_NODE node = (PPH_MIP_GROUP_NODE)mouseEvent->Node;

            if (node)
            {
                listSection->SuspendUpdate++;
                PhMipHandleListSectionCommand(listSection, node->ProcessGroup, ID_PROCESS_GOTOPROCESS);
                listSection->SuspendUpdate--;
            }
        }
        break;
    case TreeNewContextMenu:
        {
            PPH_TREENEW_CONTEXT_MENU contextMenu = Parameter1;

            // Prevent the node list from being updated (otherwise any nodes we're using might be destroyed while we're
            // in a modal message loop).
            listSection->SuspendUpdate++;
            PhMipBeginChildControlPin();
            PhMipShowListSectionContextMenu(listSection, contextMenu);
            PhMipEndChildControlPin();
            listSection->SuspendUpdate--;
        }
        break;
    }

    return FALSE;
}

PPH_STRING PhMipGetGroupNodeTooltip(
    _In_ PPH_MINIINFO_LIST_SECTION ListSection,
    _In_ PPH_MIP_GROUP_NODE Node
    )
{
    PH_STRING_BUILDER sb;

    PhInitializeStringBuilder(&sb, 100);

    // TODO

    return PhFinalStringBuilderString(&sb);
}

PPH_MIP_GROUP_NODE PhMipGetSelectedGroupNode(
    _In_ PPH_MINIINFO_LIST_SECTION ListSection
    )
{
    ULONG i;
    PPH_MIP_GROUP_NODE node;

    for (i = 0; i < ListSection->NodeList->Count; i++)
    {
        node = ListSection->NodeList->Items[i];

        if (node->Node.Selected)
            return node;
    }

    return NULL;
}

VOID PhMipShowListSectionContextMenu(
    _In_ PPH_MINIINFO_LIST_SECTION ListSection,
    _In_ PPH_TREENEW_CONTEXT_MENU ContextMenu
    )
{
    PPH_MIP_GROUP_NODE selectedNode;
    PPH_EMENU menu;
    PPH_EMENU_ITEM item;
    PH_MINIINFO_LIST_SECTION_MENU_INFORMATION menuInfo;
    PH_PLUGIN_MENU_INFORMATION pluginMenuInfo;

    selectedNode = PhMipGetSelectedGroupNode(ListSection);

    if (!selectedNode)
        return;

    menu = PhCreateEMenu();
    // TODO: If there are multiple processes, then create submenus for each process.
    PhAddMiniProcessMenuItems(menu, ListSection->SelectedRepresentativeProcessId);
    PhInsertEMenuItem(menu, PhCreateEMenuSeparator(), ULONG_MAX);
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, ID_PROCESS_GOTOPROCESS, L"&Go to process", NULL, NULL), ULONG_MAX);
    PhSetFlagsEMenuItem(menu, ID_PROCESS_GOTOPROCESS, PH_EMENU_DEFAULT, PH_EMENU_DEFAULT);

    if (selectedNode->ProcessGroup->Processes->Count != 1)
    {
        if (item = PhFindEMenuItem(menu, 0, NULL, ID_PROCESS_GOTOPROCESS))
            PhModifyEMenuItem(item, PH_EMENU_MODIFY_TEXT, 0, L"&Go to processes", NULL);
    }

    memset(&menuInfo, 0, sizeof(PH_MINIINFO_LIST_SECTION_MENU_INFORMATION));
    menuInfo.ProcessGroup = selectedNode->ProcessGroup;
    menuInfo.SortData = &selectedNode->SortData;
    menuInfo.ContextMenu = ContextMenu;
    ListSection->Callback(ListSection, MiListSectionInitializeContextMenu, &menuInfo, NULL);

    if (PhPluginsEnabled)
    {
        PhPluginInitializeMenuInfo(&pluginMenuInfo, menu, ListSection->DialogHandle, 0);
        pluginMenuInfo.Menu = menu;
        pluginMenuInfo.OwnerWindow = PhMipWindow;
        pluginMenuInfo.u.MiListSection.SectionName = &ListSection->Section->Name;
        pluginMenuInfo.u.MiListSection.ProcessGroup = selectedNode->ProcessGroup;

        PhInvokeCallback(PhGetGeneralCallback(GeneralCallbackMiListSectionMenuInitializing), &pluginMenuInfo);
    }

    item = PhShowEMenu(
        menu,
        PhMipWindow,
        PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_TOP,
        ContextMenu->Location.x,
        ContextMenu->Location.y
        );

    if (item)
    {
        BOOLEAN handled = FALSE;

        if (!handled && PhPluginsEnabled)
            handled = PhPluginTriggerEMenuItem(&pluginMenuInfo, item);

        if (!handled)
        {
            menuInfo.SelectedItem = item;
            handled = ListSection->Callback(ListSection, MiListSectionHandleContextMenu, &menuInfo, NULL);
        }

        if (!handled)
            PhHandleMiniProcessMenuItem(item);

        if (!handled)
            PhMipHandleListSectionCommand(ListSection, selectedNode->ProcessGroup, item->Id);
    }

    PhDestroyEMenu(menu);
}

VOID PhMipHandleListSectionCommand(
    _In_ PPH_MINIINFO_LIST_SECTION ListSection,
    _In_ PPH_PROCESS_GROUP ProcessGroup,
    _In_ ULONG Id
    )
{
    switch (Id)
    {
    case ID_PROCESS_GOTOPROCESS:
        {
            PPH_LIST nodes;
            ULONG i;

            nodes = PhCreateList(ProcessGroup->Processes->Count);

            for (i = 0; i < ProcessGroup->Processes->Count; i++)
            {
                PPH_PROCESS_NODE node;

                if (node = PhFindProcessNode(((PPH_PROCESS_ITEM)ProcessGroup->Processes->Items[i])->ProcessId))
                    PhAddItemList(nodes, node);
            }

            PhPinMiniInformation(MiniInfoIconPinType, -1, 0, 0, NULL, NULL);
            PhPinMiniInformation(MiniInfoActivePinType, -1, 0, 0, NULL, NULL);
            PhPinMiniInformation(MiniInfoHoverPinType, -1, 0, 0, NULL, NULL);

            ProcessHacker_ToggleVisible(TRUE);
            ProcessHacker_SelectTabPage(0);
            PhSelectAndEnsureVisibleProcessNodes((PPH_PROCESS_NODE*)nodes->Items, nodes->Count);
            PhDereferenceObject(nodes);
        }
        break;
    }
}

BOOLEAN PhMipCpuListSectionCallback(
    _In_ struct _PH_MINIINFO_LIST_SECTION *ListSection,
    _In_ PH_MINIINFO_LIST_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    switch (Message)
    {
    case MiListSectionTick:
        {
            PH_FORMAT format[3];

            // CPU    %.2f%%
            PhInitFormatS(&format[0], L"CPU    ");
            PhInitFormatF(&format[1], ((DOUBLE)PhCpuUserUsage + PhCpuKernelUsage) * 100, PhMaxPrecisionUnit);
            PhInitFormatC(&format[2], L'%');

            ListSection->Section->Parameters->SetSectionText(ListSection->Section,
                PH_AUTO(PhFormat(format, RTL_NUMBER_OF(format), 16)));
        }
        break;
    case MiListSectionSortProcessList:
        {
            PPH_MINIINFO_LIST_SECTION_SORT_LIST sortList = Parameter1;

            if (!sortList)
                break;

            qsort(sortList->List->Items, sortList->List->Count,
                sizeof(PPH_PROCESS_NODE), PhMipCpuListSectionProcessCompareFunction);
        }
        return TRUE;
    case MiListSectionAssignSortData:
        {
            PPH_MINIINFO_LIST_SECTION_ASSIGN_SORT_DATA assignSortData = Parameter1;
            PPH_LIST processes;
            FLOAT cpuUsage;
            ULONG i;

            if (!assignSortData)
                break;

            processes = assignSortData->ProcessGroup->Processes;
            cpuUsage = 0;

            for (i = 0; i < processes->Count; i++)
                cpuUsage += ((PPH_PROCESS_ITEM)processes->Items[i])->CpuUsage;

            *(PFLOAT)assignSortData->SortData->UserData = cpuUsage;
        }
        return TRUE;
    case MiListSectionSortGroupList:
        {
            PPH_MINIINFO_LIST_SECTION_SORT_LIST sortList = Parameter1;

            if (!sortList)
                break;

            qsort(sortList->List->Items, sortList->List->Count,
                sizeof(PPH_MINIINFO_LIST_SECTION_SORT_DATA), PhMipCpuListSectionNodeCompareFunction);
        }
        return TRUE;
    case MiListSectionGetUsageText:
        {
            PPH_MINIINFO_LIST_SECTION_GET_USAGE_TEXT getUsageText = Parameter1;
            PPH_LIST processes;
            FLOAT cpuUsage;
            PPH_STRING cpuUsageText;

            if (!getUsageText)
                break;

            processes = getUsageText->ProcessGroup->Processes;
            cpuUsage = *(PFLOAT)getUsageText->SortData->UserData * 100;

            if (cpuUsage >= 0.01)
            {
                PH_FORMAT format[2];

                // %.2f%%
                PhInitFormatF(&format[0], cpuUsage, PhMaxPrecisionUnit);
                PhInitFormatC(&format[1], L'%');

                cpuUsageText = PhFormat(format, RTL_NUMBER_OF(format), 16);
            }
            else if (cpuUsage != 0)
                cpuUsageText = PhCreateString(L"< 0.01%");
            else
                cpuUsageText = NULL;

            PhMoveReference(&getUsageText->Line1, cpuUsageText);
        }
        return TRUE;
    }

    return FALSE;
}

int __cdecl PhMipCpuListSectionProcessCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    int result;
    PPH_PROCESS_NODE node1 = *(PPH_PROCESS_NODE *)elem1;
    PPH_PROCESS_NODE node2 = *(PPH_PROCESS_NODE *)elem2;

    result = singlecmp(node2->ProcessItem->CpuUsage, node1->ProcessItem->CpuUsage);

    if (result == 0)
        result = uint64cmp(node2->ProcessItem->UserTime.QuadPart, node1->ProcessItem->UserTime.QuadPart);

    return result;
}

int __cdecl PhMipCpuListSectionNodeCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PPH_MINIINFO_LIST_SECTION_SORT_DATA data1 = *(PPH_MINIINFO_LIST_SECTION_SORT_DATA *)elem1;
    PPH_MINIINFO_LIST_SECTION_SORT_DATA data2 = *(PPH_MINIINFO_LIST_SECTION_SORT_DATA *)elem2;

    return singlecmp(*(PFLOAT)data2->UserData, *(PFLOAT)data1->UserData);
}

BOOLEAN PhMipCommitListSectionCallback(
    _In_ struct _PH_MINIINFO_LIST_SECTION *ListSection,
    _In_ PH_MINIINFO_LIST_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    switch (Message)
    {
    case MiListSectionTick:
        {
            DOUBLE commitFraction = (DOUBLE)PhPerfInformation.CommittedPages / PhPerfInformation.CommitLimit;
            PH_FORMAT format[5];

            PhInitFormatS(&format[0], L"Commit    ");
            PhInitFormatSize(&format[1], UInt32x32To64(PhPerfInformation.CommittedPages, PAGE_SIZE));
            PhInitFormatS(&format[2], L" (");
            PhInitFormatF(&format[3], commitFraction * 100, PhMaxPrecisionUnit);
            PhInitFormatS(&format[4], L"%)");

            ListSection->Section->Parameters->SetSectionText(ListSection->Section,
                PH_AUTO(PhFormat(format, RTL_NUMBER_OF(format), 96)));
        }
        break;
    case MiListSectionSortProcessList:
        {
            PPH_MINIINFO_LIST_SECTION_SORT_LIST sortList = Parameter1;

            if (!sortList)
                break;

            qsort(sortList->List->Items, sortList->List->Count,
                sizeof(PPH_PROCESS_NODE), PhMipCommitListSectionProcessCompareFunction);
        }
        return TRUE;
    case MiListSectionAssignSortData:
        {
            PPH_MINIINFO_LIST_SECTION_ASSIGN_SORT_DATA assignSortData = Parameter1;
            PPH_LIST processes;
            ULONG64 privateBytes;
            ULONG i;

            if (!assignSortData)
                break;

            processes = assignSortData->ProcessGroup->Processes;
            privateBytes = 0;

            for (i = 0; i < processes->Count; i++)
                privateBytes += ((PPH_PROCESS_ITEM)processes->Items[i])->VmCounters.PagefileUsage;

            *(PULONG64)assignSortData->SortData->UserData = privateBytes;
        }
        return TRUE;
    case MiListSectionSortGroupList:
        {
            PPH_MINIINFO_LIST_SECTION_SORT_LIST sortList = Parameter1;

            if (!sortList)
                break;

            qsort(sortList->List->Items, sortList->List->Count,
                sizeof(PPH_MINIINFO_LIST_SECTION_SORT_DATA), PhMipCommitListSectionNodeCompareFunction);
        }
        return TRUE;
    case MiListSectionGetUsageText:
        {
            PPH_MINIINFO_LIST_SECTION_GET_USAGE_TEXT getUsageText = Parameter1;
            PPH_LIST processes;
            ULONG64 privateBytes;

            if (!getUsageText)
                break;

            processes = getUsageText->ProcessGroup->Processes;
            privateBytes = *(PULONG64)getUsageText->SortData->UserData;

            PhMoveReference(&getUsageText->Line1, PhFormatSize(privateBytes, ULONG_MAX));
            PhMoveReference(&getUsageText->Line2, PhCreateString(L"Private bytes"));
            getUsageText->Line2Color = GetSysColor(COLOR_GRAYTEXT);
        }
        return TRUE;
    }

    return FALSE;
}

int __cdecl PhMipCommitListSectionProcessCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PPH_PROCESS_NODE node1 = *(PPH_PROCESS_NODE *)elem1;
    PPH_PROCESS_NODE node2 = *(PPH_PROCESS_NODE *)elem2;

    return uintptrcmp(node2->ProcessItem->VmCounters.PagefileUsage, node1->ProcessItem->VmCounters.PagefileUsage);
}

int __cdecl PhMipCommitListSectionNodeCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PPH_MINIINFO_LIST_SECTION_SORT_DATA data1 = *(PPH_MINIINFO_LIST_SECTION_SORT_DATA *)elem1;
    PPH_MINIINFO_LIST_SECTION_SORT_DATA data2 = *(PPH_MINIINFO_LIST_SECTION_SORT_DATA *)elem2;

    return uint64cmp(*(PULONG64)data2->UserData, *(PULONG64)data1->UserData);
}

BOOLEAN PhMipPhysicalListSectionCallback(
    _In_ struct _PH_MINIINFO_LIST_SECTION *ListSection,
    _In_ PH_MINIINFO_LIST_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    switch (Message)
    {
    case MiListSectionTick:
        {
            ULONG physicalUsage = PhSystemBasicInformation.NumberOfPhysicalPages - PhPerfInformation.AvailablePages;
            FLOAT physicalFraction = (FLOAT)physicalUsage / PhSystemBasicInformation.NumberOfPhysicalPages;
            FLOAT physicalPercent = physicalFraction * 100;
            PH_FORMAT format[5];

            PhInitFormatS(&format[0], L"Physical    ");
            PhInitFormatSize(&format[1], UInt32x32To64(physicalUsage, PAGE_SIZE));
            PhInitFormatS(&format[2], L" (");
            PhInitFormatF(&format[3], physicalPercent, PhMaxPrecisionUnit);
            PhInitFormatS(&format[4], L"%)");

            ListSection->Section->Parameters->SetSectionText(ListSection->Section,
                PH_AUTO(PhFormat(format, RTL_NUMBER_OF(format), 96)));
        }
        break;
    case MiListSectionSortProcessList:
        {
            PPH_MINIINFO_LIST_SECTION_SORT_LIST sortList = Parameter1;

            if (!sortList)
                break;

            qsort(sortList->List->Items, sortList->List->Count,
                sizeof(PPH_PROCESS_NODE), PhMipPhysicalListSectionProcessCompareFunction);
        }
        return TRUE;
    case MiListSectionAssignSortData:
        {
            PPH_MINIINFO_LIST_SECTION_ASSIGN_SORT_DATA assignSortData = Parameter1;
            PPH_LIST processes;
            ULONG64 workingSet;
            ULONG i;

            if (!assignSortData)
                break;

            processes = assignSortData->ProcessGroup->Processes;
            workingSet = 0;

            for (i = 0; i < processes->Count; i++)
                workingSet += ((PPH_PROCESS_ITEM)processes->Items[i])->VmCounters.WorkingSetSize;

            *(PULONG64)assignSortData->SortData->UserData = workingSet;
        }
        return TRUE;
    case MiListSectionSortGroupList:
        {
            PPH_MINIINFO_LIST_SECTION_SORT_LIST sortList = Parameter1;

            if (!sortList)
                break;

            qsort(sortList->List->Items, sortList->List->Count,
                sizeof(PPH_MINIINFO_LIST_SECTION_SORT_DATA), PhMipPhysicalListSectionNodeCompareFunction);
        }
        return TRUE;
    case MiListSectionGetUsageText:
        {
            PPH_MINIINFO_LIST_SECTION_GET_USAGE_TEXT getUsageText = Parameter1;
            PPH_LIST processes;
            ULONG64 privateBytes;

            if (!getUsageText)
                break;

            processes = getUsageText->ProcessGroup->Processes;
            privateBytes = *(PULONG64)getUsageText->SortData->UserData;

            PhMoveReference(&getUsageText->Line1, PhFormatSize(privateBytes, ULONG_MAX));
            PhMoveReference(&getUsageText->Line2, PhCreateString(L"Working set"));
            getUsageText->Line2Color = GetSysColor(COLOR_GRAYTEXT);
        }
        return TRUE;
    }

    return FALSE;
}

int __cdecl PhMipPhysicalListSectionProcessCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PPH_PROCESS_NODE node1 = *(PPH_PROCESS_NODE *)elem1;
    PPH_PROCESS_NODE node2 = *(PPH_PROCESS_NODE *)elem2;

    return uintptrcmp(node2->ProcessItem->VmCounters.WorkingSetSize, node1->ProcessItem->VmCounters.WorkingSetSize);
}

int __cdecl PhMipPhysicalListSectionNodeCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PPH_MINIINFO_LIST_SECTION_SORT_DATA data1 = *(PPH_MINIINFO_LIST_SECTION_SORT_DATA *)elem1;
    PPH_MINIINFO_LIST_SECTION_SORT_DATA data2 = *(PPH_MINIINFO_LIST_SECTION_SORT_DATA *)elem2;

    return uint64cmp(*(PULONG64)data2->UserData, *(PULONG64)data1->UserData);
}

BOOLEAN PhMipIoListSectionCallback(
    _In_ struct _PH_MINIINFO_LIST_SECTION *ListSection,
    _In_ PH_MINIINFO_LIST_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    switch (Message)
    {
    case MiListSectionTick:
        {
            PH_FORMAT format[6];

            PhInitFormatS(&format[0], L"I/O    R: ");
            PhInitFormatSizeWithPrecision(&format[1], PhIoReadDelta.Delta, 0);
            PhInitFormatS(&format[2], L"  W: ");
            PhInitFormatSizeWithPrecision(&format[3], PhIoWriteDelta.Delta, 0);
            PhInitFormatS(&format[4], L"  O: ");
            PhInitFormatSizeWithPrecision(&format[5], PhIoOtherDelta.Delta, 0);

            ListSection->Section->Parameters->SetSectionText(ListSection->Section,
                PH_AUTO(PhFormat(format, RTL_NUMBER_OF(format), 80)));
        }
        break;
    case MiListSectionSortProcessList:
        {
            PPH_MINIINFO_LIST_SECTION_SORT_LIST sortList = Parameter1;

            if (!sortList)
                break;

            qsort(sortList->List->Items, sortList->List->Count,
                sizeof(PPH_PROCESS_NODE), PhMipIoListSectionProcessCompareFunction);
        }
        return TRUE;
    case MiListSectionAssignSortData:
        {
            PPH_MINIINFO_LIST_SECTION_ASSIGN_SORT_DATA assignSortData = Parameter1;
            PPH_LIST processes;
            ULONG64 ioReadOtherDelta;
            ULONG64 ioWriteDelta;
            ULONG i;

            if (!assignSortData)
                break;

            processes = assignSortData->ProcessGroup->Processes;
            ioReadOtherDelta = 0;
            ioWriteDelta = 0;

            for (i = 0; i < processes->Count; i++)
            {
                PPH_PROCESS_ITEM processItem = processes->Items[i];
                ioReadOtherDelta += processItem->IoReadDelta.Delta;
                ioWriteDelta += processItem->IoWriteDelta.Delta;
                ioReadOtherDelta += processItem->IoOtherDelta.Delta;
            }

            assignSortData->SortData->UserData[0] = ioReadOtherDelta;
            assignSortData->SortData->UserData[1] = ioWriteDelta;
        }
        return TRUE;
    case MiListSectionSortGroupList:
        {
            PPH_MINIINFO_LIST_SECTION_SORT_LIST sortList = Parameter1;

            if (!sortList)
                break;

            qsort(sortList->List->Items, sortList->List->Count,
                sizeof(PPH_MINIINFO_LIST_SECTION_SORT_DATA), PhMipIoListSectionNodeCompareFunction);
        }
        return TRUE;
    case MiListSectionGetUsageText:
        {
            PPH_MINIINFO_LIST_SECTION_GET_USAGE_TEXT getUsageText = Parameter1;
            PPH_LIST processes;
            ULONG64 ioReadOtherDelta;
            ULONG64 ioWriteDelta;
            PH_FORMAT format[1];

            if (!getUsageText)
                break;

            processes = getUsageText->ProcessGroup->Processes;
            ioReadOtherDelta = getUsageText->SortData->UserData[0];
            ioWriteDelta = getUsageText->SortData->UserData[1];

            PhInitFormatSize(&format[0], ioReadOtherDelta + ioWriteDelta);
            PhMoveReference(&getUsageText->Line1, PhFormat(format, 1, 16));
        }
        return TRUE;
    }

    return FALSE;
}

int __cdecl PhMipIoListSectionProcessCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    int result;
    PPH_PROCESS_NODE node1 = *(PPH_PROCESS_NODE *)elem1;
    PPH_PROCESS_NODE node2 = *(PPH_PROCESS_NODE *)elem2;
    ULONG64 delta1 = node1->ProcessItem->IoReadDelta.Delta + node1->ProcessItem->IoWriteDelta.Delta + node1->ProcessItem->IoOtherDelta.Delta;
    ULONG64 delta2 = node2->ProcessItem->IoReadDelta.Delta + node2->ProcessItem->IoWriteDelta.Delta + node2->ProcessItem->IoOtherDelta.Delta;
    ULONG64 value1 = node1->ProcessItem->IoReadDelta.Value + node1->ProcessItem->IoWriteDelta.Value + node1->ProcessItem->IoOtherDelta.Value;
    ULONG64 value2 = node2->ProcessItem->IoReadDelta.Value + node2->ProcessItem->IoWriteDelta.Value + node2->ProcessItem->IoOtherDelta.Value;

    result = uint64cmp(delta2, delta1);

    if (result == 0)
        result = uint64cmp(value2, value1);

    return result;
}

int __cdecl PhMipIoListSectionNodeCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PPH_MINIINFO_LIST_SECTION_SORT_DATA data1 = *(PPH_MINIINFO_LIST_SECTION_SORT_DATA *)elem1;
    PPH_MINIINFO_LIST_SECTION_SORT_DATA data2 = *(PPH_MINIINFO_LIST_SECTION_SORT_DATA *)elem2;

    return uint64cmp(data2->UserData[0] + data2->UserData[1], data1->UserData[0] + data1->UserData[1]);
}
