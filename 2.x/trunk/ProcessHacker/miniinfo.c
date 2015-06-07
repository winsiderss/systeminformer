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
#include <windowsx.h>
#include <uxtheme.h>
#include <miniinfop.h>

static HWND PhMipContainerWindow = NULL;
static POINT PhMipSourcePoint;
static LONG PhMipPinCounts[MaxMiniInfoPinType];
static LONG PhMipMaxPinCounts[MaxMiniInfoPinType] =
{
    1, // MiniInfoManualPinType
    1, // MiniInfoIconPinType
    1, // MiniInfoHoverPinType
};
static LONG PhMipDelayedPinAdjustments[MaxMiniInfoPinType];
static PPH_MESSAGE_LOOP_FILTER_ENTRY PhMipMessageLoopFilterEntry;
static HWND PhMipLastTrackedWindow;
static HWND PhMipLastNcTrackedWindow;

static HWND PhMipWindow = NULL;
static PH_LAYOUT_MANAGER PhMipLayoutManager;
static RECT MinimumSize;

VOID PhShowMiniInformationDialog(
    _In_ PH_MINIINFO_PIN_TYPE PinType,
    _In_ LONG PinCount,
    _In_opt_ ULONG PinDelayMs,
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
            wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            wcex.lpszClassName = MIP_CONTAINER_CLASSNAME;
            wcex.hIconSm = (HICON)LoadImage(PhInstanceHandle, MAKEINTRESOURCE(IDI_PROCESSHACKER), IMAGE_ICON, 16, 16, 0);
            RegisterClassEx(&wcex);

            PhMipContainerWindow = CreateWindow(
                MIP_CONTAINER_CLASSNAME,
                L"Process Hacker",
                WS_THICKFRAME | WS_POPUP,
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

            MinimumSize.left = 0;
            MinimumSize.top = 0;
            MinimumSize.right = 100;
            MinimumSize.bottom = 50;
            MapDialogRect(PhMipWindow, &MinimumSize);
        }

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
        ShowWindow(PhMipContainerWindow, SW_SHOWNOACTIVATE);

        PhMipMessageLoopFilterEntry = PhRegisterMessageLoopFilter(PhMipMessageLoopFilter, NULL);
    }
    else if (adjustPinResult == HideAdjustPinResult)
    {
        if (PhMipMessageLoopFilterEntry)
        {
            PhUnregisterMessageLoopFilter(PhMipMessageLoopFilterEntry);
            PhMipMessageLoopFilterEntry = NULL;
        }

        if (PhMipContainerWindow)
            ShowWindow(PhMipContainerWindow, SW_HIDE);
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
    }

    return FALSE;
}

VOID PhMipContainerOnShowWindow(
    _In_ BOOLEAN Showing,
    _In_ ULONG State
    )
{
    if (Showing)
        PhMipContainerOnSize();
}

VOID PhMipContainerOnSize(
    VOID
    )
{
    if (PhMipWindow)
    {
        RECT rect;

        GetClientRect(PhMipContainerWindow, &rect);
        MoveWindow(
            PhMipWindow,
            rect.left, rect.top,
            rect.right - rect.left, rect.bottom - rect.top,
            FALSE
            );

        PhLayoutManagerLayout(&PhMipLayoutManager);
    }
}

VOID PhMipContainerOnSizing(
    _In_ ULONG Edge,
    _In_ PRECT DragRectangle
    )
{
    PhResizingMinimumSize(DragRectangle, Edge, MinimumSize.right, MinimumSize.bottom);
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

        // PhShowMiniInformationDialog kills the timer for us.
        PhShowMiniInformationDialog(pinType, PhMipDelayedPinAdjustments[pinType], 0, NULL);
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
    PhAddLayoutItem(&PhMipLayoutManager, GetDlgItem(PhMipWindow, IDC_OPEN), NULL,
        PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
    PhAddLayoutItem(&PhMipLayoutManager, GetDlgItem(PhMipWindow, IDC_OPTIONS), NULL,
        PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
    PhAddLayoutItem(&PhMipLayoutManager, GetDlgItem(PhMipWindow, IDC_PIN), NULL,
        PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
}

VOID PhMipOnShowWindow(
    _In_ BOOLEAN Showing,
    _In_ ULONG State
    )
{
    NOTHING;
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
                PhShowMiniInformationDialog(MiniInfoIconPinType, -1, 0, NULL);
                PhShowMiniInformationDialog(MiniInfoHoverPinType, -1, 0, NULL);
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
    SetBkMode(hdc, TRANSPARENT);
    *Brush = GetSysColorBrush(COLOR_WINDOW);

    return TRUE;
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

            PhShowMiniInformationDialog(MiniInfoHoverPinType, 1, 0, NULL);
        }
        else if (Message->message == WM_MOUSELEAVE && Message->hwnd == PhMipLastTrackedWindow)
        {
            PhShowMiniInformationDialog(MiniInfoHoverPinType, -1, 250, NULL);
        }
        else if (Message->message == WM_NCMOUSELEAVE && Message->hwnd == PhMipLastNcTrackedWindow)
        {
            PhShowMiniInformationDialog(MiniInfoHoverPinType, -1, 250, NULL);
        }
    }

    return FALSE;
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
    RECT clientRect;
    PH_RECTANGLE windowRectangle;
    PH_RECTANGLE point;
    MONITORINFO monitorInfo = { sizeof(monitorInfo) };

    GetWindowRect(PhMipContainerWindow, &clientRect);
    windowRectangle = PhRectToRectangle(clientRect);

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
