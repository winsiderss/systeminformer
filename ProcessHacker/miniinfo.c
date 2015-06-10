/*
 * Process Hacker -
 *   mini information window
 *
 * Copyright (C) 2015 wj32
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
#include <settings.h>
#include <emenu.h>
#include <windowsx.h>
#include <uxtheme.h>
#include <miniinfop.h>

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

static HWND PhMipWindow = NULL;
static PH_LAYOUT_MANAGER PhMipLayoutManager;
static RECT MinimumSize;
static PH_CALLBACK_REGISTRATION ProcessesUpdatedRegistration;
static PH_STRINGREF DownArrowPrefix = PH_STRINGREF_INIT(L"\u25be ");
static HWND SeparatorControl;
static WNDPROC SectionControlOldWndProc;

static PPH_LIST SectionList;
static PH_MINIINFO_PARAMETERS CurrentParameters;
static PPH_MINIINFO_SECTION CurrentSection;

static HWND CpuDialog;
static PH_LAYOUT_MANAGER CpuLayoutManager;

VOID PhPinMiniInformation(
    _In_ PH_MINIINFO_PIN_TYPE PinType,
    _In_ LONG PinCount,
    _In_opt_ ULONG PinDelayMs,
    _In_ ULONG Flags,
    _In_opt_ PWSTR SectionName,
    _In_opt_ PPOINT SourcePoint
    )
{
    MIP_ADJUST_PIN_RESULT adjustPinResult;

    if (PinDelayMs && PinCount < 0)
    {
        PhMipDelayedPinAdjustments[PinType] = PinCount;
        SetTimer(PhMipContainerWindow, MIP_TIMER_PIN_FIRST + PinType, PinDelayMs, NULL);
        return;
    }
    else
    {
        PhMipDelayedPinAdjustments[PinType] = 0;
        KillTimer(PhMipContainerWindow, MIP_TIMER_PIN_FIRST + PinType);
    }

    adjustPinResult = PhMipAdjustPin(PinType, PinCount);

    if (adjustPinResult == ShowAdjustPinResult)
    {
        PH_RECTANGLE windowRectangle;

        if (SourcePoint)
            PhMipSourcePoint = *SourcePoint;

        if (!PhMipContainerWindow)
        {
            WNDCLASSEX wcex;

            memset(&wcex, 0, sizeof(WNDCLASSEX));
            wcex.cbSize = sizeof(WNDCLASSEX);
            wcex.style = 0;
            wcex.lpfnWndProc = PhMipContainerWndProc;
            wcex.cbClsExtra = 0;
            wcex.cbWndExtra = 0;
            wcex.hInstance = PhInstanceHandle;
            wcex.hIcon = LoadIcon(PhInstanceHandle, MAKEINTRESOURCE(IDI_PROCESSHACKER));
            wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
            wcex.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
            wcex.lpszClassName = MIP_CONTAINER_CLASSNAME;
            wcex.hIconSm = (HICON)LoadImage(PhInstanceHandle, MAKEINTRESOURCE(IDI_PROCESSHACKER), IMAGE_ICON, 16, 16, 0);
            RegisterClassEx(&wcex);

            PhMipContainerWindow = CreateWindow(
                MIP_CONTAINER_CLASSNAME,
                L"Process Hacker",
                WS_BORDER | WS_THICKFRAME | WS_POPUP,
                0,
                0,
                400,
                400,
                NULL,
                NULL,
                PhInstanceHandle,
                NULL
                );
            PhSetWindowExStyle(PhMipContainerWindow, WS_EX_TOOLWINDOW, WS_EX_TOOLWINDOW);
            PhMipWindow = CreateDialog(
                PhInstanceHandle,
                MAKEINTRESOURCE(IDD_MINIINFO),
                PhMipContainerWindow,
                PhMipMiniInfoDialogProc
                );
            ShowWindow(PhMipWindow, SW_SHOW);

            if (PhGetIntegerSetting(L"MiniInfoWindowPinned"))
                PhMipSetPinned(TRUE);

            MinimumSize.left = 0;
            MinimumSize.top = 0;
            MinimumSize.right = 210;
            MinimumSize.bottom = 140;
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

        ShowWindow(PhMipContainerWindow, (Flags & PH_MINIINFO_ACTIVATE_WINDOW) ? SW_SHOW : SW_SHOWNOACTIVATE);
    }
    else if (adjustPinResult == HideAdjustPinResult)
    {
        if (PhMipContainerWindow)
            ShowWindow(PhMipContainerWindow, SW_HIDE);
    }
    else
    {
        if ((Flags & PH_MINIINFO_ACTIVATE_WINDOW) && IsWindowVisible(PhMipContainerWindow))
            SetActiveWindow(PhMipContainerWindow);
    }

    if (SectionName)
    {
        PH_STRINGREF sectionName;
        PPH_MINIINFO_SECTION section;

        PhInitializeStringRef(&sectionName, SectionName);

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
            PhMipContainerOnActivate(LOWORD(wParam), !!HIWORD(wParam));
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
            PhMipOnCommand(LOWORD(wParam), HIWORD(wParam));
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
    if (Showing)
    {
        PostMessage(PhMipWindow, MIP_MSG_UPDATE, 0, 0);

        PhMipMessageLoopFilterEntry = PhRegisterMessageLoopFilter(PhMipMessageLoopFilter, NULL);

        PhRegisterCallback(
            &PhProcessesUpdatedEvent,
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

        Button_SetCheck(GetDlgItem(PhMipWindow, IDC_PIN), BST_UNCHECKED);
        PhMipSetPinned(FALSE);
        PhSetIntegerSetting(L"MiniInfoWindowPinned", FALSE);

        PhUnregisterCallback(
            &PhProcessesUpdatedEvent,
            &ProcessesUpdatedRegistration
            );

        if (PhMipMessageLoopFilterEntry)
        {
            PhUnregisterMessageLoopFilter(PhMipMessageLoopFilterEntry);
            PhMipMessageLoopFilterEntry = NULL;
        }

        PhSaveWindowPlacementToSetting(L"MiniInfoWindowPosition", L"MiniInfoWindowSize", PhMipContainerWindow);
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
    HBITMAP cog;
    HBITMAP pin;

    cog = PH_LOAD_SHARED_IMAGE(MAKEINTRESOURCE(IDB_COG), IMAGE_BITMAP);
    SET_BUTTON_BITMAP(PhMipWindow, IDC_OPTIONS, cog);

    pin = PH_LOAD_SHARED_IMAGE(MAKEINTRESOURCE(IDB_PIN), IMAGE_BITMAP);
    SET_BUTTON_BITMAP(PhMipWindow, IDC_PIN, pin);

    PhInitializeLayoutManager(&PhMipLayoutManager, PhMipWindow);
    PhAddLayoutItem(&PhMipLayoutManager, GetDlgItem(PhMipWindow, IDC_LAYOUT), NULL,
        PH_ANCHOR_ALL);
    PhAddLayoutItem(&PhMipLayoutManager, GetDlgItem(PhMipWindow, IDC_SECTION), NULL,
        PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_TOP | PH_LAYOUT_FORCE_INVALIDATE);
    PhAddLayoutItem(&PhMipLayoutManager, GetDlgItem(PhMipWindow, IDC_OPEN), NULL,
        PH_ANCHOR_RIGHT | PH_ANCHOR_TOP);
    PhAddLayoutItem(&PhMipLayoutManager, GetDlgItem(PhMipWindow, IDC_OPTIONS), NULL,
        PH_ANCHOR_RIGHT | PH_ANCHOR_TOP);
    PhAddLayoutItem(&PhMipLayoutManager, GetDlgItem(PhMipWindow, IDC_PIN), NULL,
        PH_ANCHOR_RIGHT | PH_ANCHOR_TOP);

    SeparatorControl = CreateWindow(
        L"STATIC",
        NULL,
        WS_CHILD | SS_OWNERDRAW,
        0,
        0,
        3,
        3,
        PhMipWindow,
        (HMENU)IDC_SEPARATOR,
        PhInstanceHandle,
        NULL
        );
    ShowWindow(SeparatorControl, SW_SHOW);

    SectionControlOldWndProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(PhMipWindow, IDC_SECTION), GWLP_WNDPROC);
    SetWindowLongPtr(GetDlgItem(PhMipWindow, IDC_SECTION), GWLP_WNDPROC, (LONG_PTR)PhMipSectionControlHookWndProc);

    Button_SetCheck(GetDlgItem(PhMipWindow, IDC_PIN), !!PhGetIntegerSetting(L"MiniInfoWindowPinned"));
}

VOID PhMipOnShowWindow(
    _In_ BOOLEAN Showing,
    _In_ ULONG State
    )
{
    PPH_MINIINFO_SECTION section;

    if (SectionList)
        return;

    SectionList = PhCreateList(8);
    PhMipInitializeParameters();

    SendMessage(GetDlgItem(PhMipWindow, IDC_SECTION), WM_SETFONT, (WPARAM)CurrentParameters.MediumFont, FALSE);
    SendMessage(GetDlgItem(PhMipWindow, IDC_OPEN), WM_SETFONT, (WPARAM)CurrentParameters.MediumFont, FALSE);

    section = PhMipCreateInternalSection(L"CPU", 0, PhMipCpuSectionCallback);
    PhMipCreateInternalSection(L"Memory", 0, PhMipCpuSectionCallback);
    PhMipCreateInternalSection(L"I/O", 0, PhMipCpuSectionCallback);

    PhMipChangeSection(section);
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
                        0,
                        0,
                        ((PPH_STRING)PhAutoDereferenceObject(PhCreateString2(&section->Name)))->Buffer,
                        NULL,
                        section
                        );
                    PhInsertEMenuItem(menu, menuItem, -1);
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
            break;
        }
        break;
    case IDC_OPTIONS:
        {
            PPH_EMENU menu;
            PPH_EMENU_ITEM menuItem;
            RECT rect;

            PhMipBeginChildControlPin();
            menu = PhCreateEMenu();
            PhLoadResourceEMenuItem(menu, PhInstanceHandle, MAKEINTRESOURCE(IDR_MINIINFO), 0);

            GetWindowRect(GetDlgItem(PhMipWindow, IDC_OPTIONS), &rect);
            menuItem = PhShowEMenu(menu, PhMipWindow, PH_EMENU_SHOW_LEFTRIGHT,
                PH_ALIGN_LEFT | PH_ALIGN_TOP, rect.left, rect.bottom);

            if (menuItem)
            {
                // TODO
            }

            PhDestroyEMenu(menu);
            PhMipEndChildControlPin();
        }
        break;
    case IDC_PIN:
        {
            BOOLEAN pinned;

            pinned = Button_GetCheck(GetDlgItem(PhMipWindow, IDC_PIN)) == BST_CHECKED;
            PhPinMiniInformation(MiniInfoManualPinType, pinned ? 1 : -1, 0, 0, NULL, NULL);
            PhMipSetPinned(pinned);
            PhSetIntegerSetting(L"MiniInfoWindowPinned", pinned);
        }
        break;
    }
}

BOOLEAN PhMipOnNotify(
    _In_ NMHDR *Header,
    _Out_ LRESULT *Result
    )
{
    switch (Header->code)
    {
    case NM_CLICK:
        {
            switch (Header->idFrom)
            {
            case IDC_OPEN:
                // Clear most pin types.
                PhPinMiniInformation(MiniInfoIconPinType, -1, 0, 0, NULL, NULL);
                PhPinMiniInformation(MiniInfoHoverPinType, -1, 0, 0, NULL, NULL);
                ProcessHacker_ToggleVisible(PhMainWndHandle, TRUE);
                break;
            }
        }
        break;
    }

    return FALSE;
}

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
    if (Id == IDC_SEPARATOR)
    {
        RECT rect;

        rect = DrawItemStruct->rcItem;
        FillRect(DrawItemStruct->hDC, &rect, GetSysColorBrush(COLOR_3DHIGHLIGHT));
        rect.top += 1;
        FillRect(DrawItemStruct->hDC, &rect, GetSysColorBrush(COLOR_3DSHADOW));

        return TRUE;
    }

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
            {
                PhMipLastTrackedWindow = Message->hwnd;
                PhMipLastNcTrackedWindow = NULL;
            }
            else
            {
                PhMipLastTrackedWindow = NULL;
                PhMipLastNcTrackedWindow = Message->hwnd;
            }

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
    }

    return FALSE;
}

VOID NTAPI PhMipUpdateHandler(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PostMessage(PhMipWindow, MIP_MSG_UPDATE, 0, 0);
}

MIP_ADJUST_PIN_RESULT PhMipAdjustPin(
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

    memset(&CurrentParameters, 0, sizeof(PH_MINIINFO_PARAMETERS));

    CurrentParameters.ContainerWindowHandle = PhMipContainerWindow;
    CurrentParameters.MiniInfoWindowHandle = PhMipWindow;

    if (SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &logFont, 0))
    {
        CurrentParameters.Font = CreateFontIndirect(&logFont);
    }
    else
    {
        CurrentParameters.Font = PhApplicationFont;
        GetObject(PhApplicationFont, sizeof(LOGFONT), &logFont);
    }

    hdc = GetDC(PhMipWindow);

    logFont.lfHeight -= MulDiv(2, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    CurrentParameters.MediumFont = CreateFontIndirect(&logFont);

    originalFont = SelectObject(hdc, CurrentParameters.Font);
    GetTextMetrics(hdc, &textMetrics);
    CurrentParameters.FontHeight = textMetrics.tmHeight;
    CurrentParameters.FontAverageWidth = textMetrics.tmAveCharWidth;

    SelectObject(hdc, CurrentParameters.MediumFont);
    GetTextMetrics(hdc, &textMetrics);
    CurrentParameters.MediumFontHeight = textMetrics.tmHeight;
    CurrentParameters.MediumFontAverageWidth = textMetrics.tmAveCharWidth;

    SelectObject(hdc, originalFont);
    ReleaseDC(PhMipWindow, hdc);
}

PPH_MINIINFO_SECTION PhMipCreateSection(
    _In_ PPH_MINIINFO_SECTION Template
    )
{
    PPH_MINIINFO_SECTION section;

    section = PhAllocate(sizeof(PH_MINIINFO_SECTION));
    memset(section, 0, sizeof(PH_MINIINFO_SECTION));

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
    PhInitializeStringRef(&section.Name, Name);
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
        }
    }
}

VOID PhMipChangeSection(
    _In_ PPH_MINIINFO_SECTION NewSection
    )
{
    PPH_MINIINFO_SECTION oldSection;

    oldSection = CurrentSection;
    CurrentSection = NewSection;

    if (oldSection)
    {
        oldSection->Callback(oldSection, MiniInfoViewChanging, CurrentSection, NULL);

        if (oldSection->DialogHandle)
            ShowWindow(oldSection->DialogHandle, SW_HIDE);
    }

    if (!NewSection->DialogHandle)
        PhMipCreateSectionDialog(NewSection);
    if (NewSection->DialogHandle)
        ShowWindow(NewSection->DialogHandle, SW_SHOW);

    SetDlgItemText(PhMipWindow, IDC_SECTION,
        ((PPH_STRING)PhAutoDereferenceObject(PhConcatStringRef2(&DownArrowPrefix, &NewSection->Name)))->Buffer);

    PhMipLayout();
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

        MoveWindow(
            CurrentSection->DialogHandle,
            rect.left, rect.top,
            rect.right - rect.left, rect.bottom - rect.top,
            TRUE
            );
    }

    GetWindowRect(GetDlgItem(PhMipWindow, IDC_PIN), &rect);
    MapWindowPoints(NULL, PhMipWindow, (POINT *)&rect, 2);

    MoveWindow(SeparatorControl, 0, rect.bottom + MIP_PADDING_SIZE, clientRect.right, MIP_SEPARATOR_HEIGHT, TRUE);
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

VOID PhMipSetPinned(
    _In_ BOOLEAN Pinned
    )
{
    PhSetWindowStyle(PhMipContainerWindow, WS_DLGFRAME | WS_SYSMENU, Pinned ? (WS_DLGFRAME | WS_SYSMENU) : 0);
    SetWindowPos(PhMipContainerWindow, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
}

LRESULT CALLBACK PhMipSectionControlHookWndProc(
    _In_ HWND hwnd,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WM_SETCURSOR:
        {
            SetCursor(LoadCursor(NULL, IDC_HAND));
        }
        return TRUE;
    }

    return CallWindowProc(SectionControlOldWndProc, hwnd, uMsg, wParam, lParam);
}

BOOLEAN PhMipCpuSectionCallback(
    _In_ PPH_MINIINFO_SECTION Section,
    _In_ PH_MINIINFO_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    switch (Message)
    {
    case MiniInfoTick:
        {
            if (CpuDialog)
            {
                PhMipTickCpuDialog();
            }
        }
        break;
    case MiniInfoCreateDialog:
        {
            PPH_MINIINFO_CREATE_DIALOG createDialog = Parameter1;

            createDialog->Instance = PhInstanceHandle;
            createDialog->Template = MAKEINTRESOURCE(IDD_MINIINFO_CPU);
            createDialog->DialogProc = PhMipCpuDialogProc;
        }
        return TRUE;
    }

    return FALSE;
}

VOID PhMipTickCpuDialog(
    VOID
    )
{
    SetDlgItemText(CpuDialog, IDC_UTILIZATION, PhaFormatString(L"%.2f%%", (PhCpuUserUsage + PhCpuKernelUsage) * 100)->Buffer);
}

INT_PTR CALLBACK PhMipCpuDialogProc(
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
            CpuDialog = hwndDlg;
            PhInitializeLayoutManager(&CpuLayoutManager, hwndDlg);
            PhAddLayoutItem(&CpuLayoutManager, GetDlgItem(hwndDlg, IDC_LIST), NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&CpuLayoutManager, GetDlgItem(hwndDlg, IDC_UTILIZATION_L), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&CpuLayoutManager, GetDlgItem(hwndDlg, IDC_UTILIZATION), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);

            SendMessage(GetDlgItem(hwndDlg, IDC_UTILIZATION), WM_SETFONT, (WPARAM)CurrentParameters.MediumFont, FALSE);

            PhMipTickCpuDialog();
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
        }
        break;
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        {
            HDC hdc = (HDC)wParam;

            SetBkMode(hdc, TRANSPARENT);

            return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
        }
        break;
    }

    return FALSE;
}
