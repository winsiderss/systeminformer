/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011
 *     dmex    2017-2026
 *
 */

#include "wndexp.h"
#include <propkey.h>
#include <propsys.h>
#include <shellapi.h>

#define WE_HIGHLIGHT_OVERLAY_CLASS L"SystemInformerWindowExplorerHighlightOverlay"
#define WE_HIGHLIGHT_OVERLAY_COLOR RGB(0xff, 0x00, 0xff)

/**
 * Converts a physical screen point into the logical coordinate space of a window.
 *
 * We run as per-monitor DPI aware, so screen coordinates are physical. Windows still virtualizes
 * coordinates for DPI unaware windows, which lay out in a logical space that is scaled up to
 * physical. This converts between the two so coordinates handed to (or shared with) another
 * process line up.
 *
 * \param[in] WindowHandle The target window.
 * \param[in,out] Point On input a physical screen point; on success the equivalent logical screen
 * point in the DPI context of \a WindowHandle. Left unmodified on failure.
 * \return TRUE if the point was converted, FALSE if it was left unmodified.
 */
static BOOLEAN WepPhysicalToLogicalPoint(
    _In_ HWND WindowHandle,
    _Inout_ PPOINT Point
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOL (WINAPI* PhysicalToLogicalPointForPerMonitorDPI_I)(_In_opt_ HWND WindowHandle, _Inout_ LPPOINT Point) = NULL;
    POINT point;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"user32.dll"))
        {
            PhysicalToLogicalPointForPerMonitorDPI_I = PhGetProcedureAddress(baseAddress, "PhysicalToLogicalPointForPerMonitorDPI", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    // Windows 8.1+
    if (!PhysicalToLogicalPointForPerMonitorDPI_I)
        return FALSE;

    point = *Point;

    if (!PhysicalToLogicalPointForPerMonitorDPI_I(WindowHandle, &point))
        return FALSE;

    *Point = point;

    return TRUE;
}

static LRESULT CALLBACK WeWindowHighlightOverlayWndProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    switch (WindowMessage)
    {
    case WM_NCHITTEST:
        return HTTRANSPARENT;
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    case WM_ERASEBKGND:
        return TRUE;
    case WM_PAINT:
        {
            PAINTSTRUCT paintStruct;
            HDC hdc;
            RECT rect;
            HBRUSH brush;
            HPEN pen;
            HGDIOBJ oldBrush;
            HGDIOBJ oldPen;

            hdc = BeginPaint(WindowHandle, &paintStruct);
            GetClientRect(WindowHandle, &rect);
            brush = CreateSolidBrush(WE_HIGHLIGHT_OVERLAY_COLOR);
            pen = CreatePen(PS_INSIDEFRAME, max(2, PhGetSystemMetrics(SM_CXBORDER, PhGetWindowDpi(WindowHandle)) * 3), RGB(0x00, 0x78, 0xd7));
            oldBrush = SelectObject(hdc, brush);
            oldPen = SelectObject(hdc, pen);
            FillRect(hdc, &paintStruct.rcPaint, brush);
            Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBrush);
            DeletePen(pen);
            DeleteBrush(brush);
            EndPaint(WindowHandle, &paintStruct);
        }
        return 0;
    }

    return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
}

RTL_ATOM WeInitializeWindowHighlightOverlayClass(
    VOID
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static RTL_ATOM highlightWindowAtom = RTL_ATOM_INVALID_ATOM;


    if (PhBeginInitOnce(&initOnce))
    {
        WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

        wcex.lpfnWndProc = WeWindowHighlightOverlayWndProc;
        wcex.hInstance = PluginInstance->DllBase;
        wcex.hCursor = PhLoadCursor(NULL, IDC_ARROW);
        wcex.lpszClassName = WE_HIGHLIGHT_OVERLAY_CLASS;

        highlightWindowAtom = RegisterClassEx(&wcex);

        PhEndInitOnce(&initOnce);
    }

    return highlightWindowAtom;
}

VOID WeShowWindowHighlightOverlay(
    _Inout_ HWND *OverlayWindow,
    _In_ HWND TargetWindow
    )
{
    RECT rect;

    if (!IsWindow(TargetWindow) || !PhGetWindowRect(TargetWindow, &rect))
    {
        WeHideWindowHighlightOverlay(OverlayWindow);
        return;
    }

    if (!*OverlayWindow)
    {
        RTL_ATOM highlightWindowAtom;
        
        highlightWindowAtom = WeInitializeWindowHighlightOverlayClass();

        *OverlayWindow = PhCreateWindowEx(
            MAKEINTATOM(highlightWindowAtom),
            NULL,
            WS_POPUP | WS_DISABLED,
            WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
            rect.left,
            rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top,
            NULL,
            NULL,
            PluginInstance->DllBase,
            NULL
            );

        if (!*OverlayWindow)
            return;

        SetLayeredWindowAttributes(*OverlayWindow, WE_HIGHLIGHT_OVERLAY_COLOR, 0, LWA_COLORKEY);
    }

    SetWindowPos(
        *OverlayWindow,
        HWND_TOPMOST,
        rect.left,
        rect.top,
        rect.right - rect.left,
        rect.bottom - rect.top,
        SWP_NOACTIVATE | SWP_SHOWWINDOW
        );
    InvalidateRect(*OverlayWindow, NULL, FALSE);
    UpdateWindow(*OverlayWindow);
}

VOID WeHideWindowHighlightOverlay(
    _Inout_ HWND *OverlayWindow
    )
{
    if (*OverlayWindow)
    {
        DestroyWindow(*OverlayWindow);
        *OverlayWindow = NULL;
    }
}

VOID WeStopWindowHighlight(
    _Inout_ HWND *OverlayWindow,
    _Inout_ HWND *HighlightingWindow,
    _Inout_ PULONG HighlightingWindowCount,
    _Inout_ PBOOLEAN HighlightingOverlay
    )
{
    RECT rect;

    WeHideWindowHighlightOverlay(OverlayWindow);

    if (
        !*HighlightingOverlay &&
        (*HighlightingWindowCount & 1) &&
        IsWindow(*HighlightingWindow) &&
        PhGetWindowRect(*HighlightingWindow, &rect)
        )
    {
        WeInvertWindowBorder(*HighlightingWindow);
    }

    *HighlightingWindow = NULL;
    *HighlightingWindowCount = 0;
    *HighlightingOverlay = FALSE;
}

VOID WeStartWindowHighlight(
    _Inout_ HWND *OverlayWindow,
    _Inout_ HWND *HighlightingWindow,
    _Inout_ PULONG HighlightingWindowCount,
    _Inout_ PBOOLEAN HighlightingOverlay,
    _In_ HWND TargetWindow
    )
{
    RECT rect;

    WeStopWindowHighlight(
        OverlayWindow,
        HighlightingWindow,
        HighlightingWindowCount,
        HighlightingOverlay
        );

    if (!IsWindow(TargetWindow) || !PhGetWindowRect(TargetWindow, &rect))
        return;

    *HighlightingWindow = TargetWindow;
    *HighlightingWindowCount = 10;
    *HighlightingOverlay = !!PhGetIntegerSetting(SETTING_NAME_WINDOW_HIGHLIGHT_LAYERED);

    if (*HighlightingOverlay)
        WeShowWindowHighlightOverlay(OverlayWindow, TargetWindow);
}

BOOLEAN WeUpdateWindowHighlight(
    _Inout_ HWND *OverlayWindow,
    _Inout_ HWND *HighlightingWindow,
    _Inout_ PULONG HighlightingWindowCount,
    _Inout_ PBOOLEAN HighlightingOverlay
    )
{
    RECT rect;

    if (*HighlightingWindowCount == 0)
        return FALSE;

    if (
        !IsWindow(*HighlightingWindow) ||
        !PhGetWindowRect(*HighlightingWindow, &rect)
        )
    {
        WeStopWindowHighlight(
            OverlayWindow,
            HighlightingWindow,
            HighlightingWindowCount,
            HighlightingOverlay
            );
        return FALSE;
    }

    if (*HighlightingOverlay)
    {
        if (*HighlightingWindowCount & 1)
            WeShowWindowHighlightOverlay(OverlayWindow, *HighlightingWindow);
        else
            WeHideWindowHighlightOverlay(OverlayWindow);
    }
    else
    {
        WeInvertWindowBorder(*HighlightingWindow);
    }

    if (--(*HighlightingWindowCount) == 0)
    {
        WeHideWindowHighlightOverlay(OverlayWindow);
        *HighlightingWindow = NULL;
        *HighlightingOverlay = FALSE;
        return FALSE;
    }

    return TRUE;
}

// {602D4995-B13A-429B-A66E-1935E44F4317}, 101
static const PROPERTYKEY PKEY_ITaskbarList2 = { { 0x602D4995, 0xB13A, 0x429B, { 0xA6, 0x6E, 0x19, 0x35, 0xE4, 0x4F, 0x43, 0x17 } }, 101 };
// {9F4C2855-9F79-4B39-A8D0-E1D42DE1D5F3}, 4
static const PROPERTYKEY PKEY_AppUserModel_RelaunchDisplayNameResource_Private = { { 0x9F4C2855, 0x9F79, 0x4B39, { 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3 } }, 4 };

NTSTATUS WeWindowEndTask(
    _In_ HWND WindowHandle,
    _In_ BOOL Force
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOL (WINAPI* EndTask_I)(
        _In_ HWND WindowHandle,
        _In_ BOOL ShutDown,
        _In_ BOOL Force
        );

    if (PhBeginInitOnce(&initOnce))
    {
        HANDLE moduleHandle;

        if (moduleHandle = PhLoadLibrary(L"user32.dll"))
        {
            EndTask_I = PhGetProcedureAddress(moduleHandle, "EndTask", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!EndTask_I)
        return STATUS_PROCEDURE_NOT_FOUND;

    if (EndTask_I(WindowHandle, FALSE, Force))
        return STATUS_SUCCESS;

    return PhGetLastWin32ErrorAsNtStatus();
}

VOID WeSetWindowToDpiForTesting(
    _In_ HWND WindowHandle,
    _In_ LONG WindowDPI
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOL (WINAPI *NtUserForceWindowToDpiForTest_I)(
        _In_ HWND WindowHandle,
        _In_ UINT32 dpi
        );

    if (PhBeginInitOnce(&initOnce))
    {
        HANDLE baseAddress;

        if (baseAddress = PhLoadLibrary(L"win32u.dll"))
            NtUserForceWindowToDpiForTest_I = PhGetProcedureAddress(baseAddress, "NtUserForceWindowToDpiForTest", 0);

        PhEndInitOnce(&initOnce);
    }

    if (NtUserForceWindowToDpiForTest_I)
        NtUserForceWindowToDpiForTest_I(WindowHandle, WindowDPI);
}

NTSTATUS WeDestroyRemoteWindow(
    _In_ HWND TargetWindow,
    _In_ HANDLE ProcessId
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    HANDLE processHandle = NULL;

    // Windows 8 requires ALL_ACCESS for PLM execution requests. (dmex)
    if (PhWindowsVersion >= WINDOWS_8 && PhWindowsVersion <= WINDOWS_8_1)
    {
        status = PhOpenProcess(
            &processHandle,
            PROCESS_ALL_ACCESS,
            ProcessId
            );
    }

    // Windows 10 and above require SET_LIMITED for PLM execution requests. (dmex)
    if (!NT_SUCCESS(status))
    {
        status = PhOpenProcess(
            &processHandle,
            PROCESS_QUERY_INFORMATION | PROCESS_SET_LIMITED_INFORMATION |
            PROCESS_VM_READ | SYNCHRONIZE,
            ProcessId
            );
    }

    if (NT_SUCCESS(status))
    {
        status = PhDestroyWindowRemote(processHandle, TargetWindow);
        NtClose(processHandle);
    }

    return status;
}

BOOL WeIsTopLevelWindow(
    _In_ HWND WindowHandle
    )
{
    static BOOL (WINAPI* IsTopLevelWindow_I)(_In_ HWND WindowHandle) = NULL;

    if (!IsTopLevelWindow_I)
        IsTopLevelWindow_I = PhGetModuleProcAddress(L"user32.dll", "IsTopLevelWindow");

    if (!IsTopLevelWindow_I)
        return FALSE;

    return IsTopLevelWindow_I(WindowHandle);
}

BOOL WeIsInDesktopWindowBand(
    _In_ HWND WindowHandle
    )
{
    static BOOL (WINAPI* IsInDesktopWindowBand_I)(_In_ HWND WindowHandle) = NULL;

    if (!IsInDesktopWindowBand_I)
        IsInDesktopWindowBand_I = PhGetModuleProcAddress(L"user32.dll", "IsInDesktopWindowBand");

    if (!IsInDesktopWindowBand_I)
        return FALSE;

    return !!IsInDesktopWindowBand_I(WindowHandle);
}

HICON WeGetInternalWindowIcon(
    _In_ HWND WindowHandle,
    _In_ UINT IconType
    )
{
    static HICON (WINAPI *InternalGetWindowIcon_I)(_In_ HWND WindowHandle, _In_ UINT iconType) = NULL;

    if (!InternalGetWindowIcon_I)
        InternalGetWindowIcon_I = PhGetModuleProcAddress(L"user32.dll", "InternalGetWindowIcon");

    if (!InternalGetWindowIcon_I)
        return NULL;

    return InternalGetWindowIcon_I(WindowHandle, IconType);
}

HICON WeGetWindowIcon(
    _In_ HWND WindowHandle
    )
{
    ULONG_PTR windowIcon = 0;
    NTSTATUS status;

    status = PhSendMessageTimeout(WindowHandle, WM_GETICON, ICON_SMALL2, 0, 100, &windowIcon);

    if (!NT_SUCCESS(status) || windowIcon == 0)
    {
        status = PhSendMessageTimeout(WindowHandle, WM_GETICON, 0, 0, 100, &windowIcon);
    }

    if (!NT_SUCCESS(status) || windowIcon == 0)
    {
        windowIcon = GetClassLongPtr(WindowHandle, GCLP_HICONSM);

        if (windowIcon == 0)
            windowIcon = GetClassLongPtr(WindowHandle, GCLP_HICON);
    }

    return (HICON)windowIcon;
}

PPH_STRING WeHashWindowExtraBytes(
    _In_ HWND WindowHandle
    )
{
    PPH_STRING string;
    ULONG extraBytes;
    PBYTE extraData;

    extraBytes = (ULONG)GetClassLongPtr(WindowHandle, GCL_CBWNDEXTRA);

    if (extraBytes <= 0)
        return PhReferenceEmptyString();

    extraData = PhAllocate(extraBytes);

    if (extraData)
    {
        // Copy the extra bytes
        //
        for (ULONG offset = 0; offset < extraBytes; offset += sizeof(ULONG_PTR))
        {
            ULONG_PTR value = GetWindowLongPtr(WindowHandle, offset);
            ULONG copy = min(extraBytes - offset, (ULONG)sizeof(ULONG_PTR));

            memcpy(extraData + offset, &value, copy);
        }
    }

    if (extraBytes <= 0)
    {
        PhFree(extraData);
        return PhReferenceEmptyString();
    }

    string = PhBufferToHexString(extraData, extraBytes);
    PhFree(extraData);

    return string;
}

BOOLEAN WeWindowHasAutomationProvider(
    _In_ HWND WindowHandle
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOL (WINAPI *UiaHasServerSideProvider_I)(_In_ HWND WindowHandle);

    if (PhBeginInitOnce(&initOnce))
    {
        HANDLE baseAddress;

        if (baseAddress = PhLoadLibrary(L"uiautomationcore.dll"))
            UiaHasServerSideProvider_I = PhGetProcedureAddress(baseAddress, "UiaHasServerSideProvider", 0);

        PhEndInitOnce(&initOnce);
    }

    if (!UiaHasServerSideProvider_I)
        return FALSE;

    return !!UiaHasServerSideProvider_I(WindowHandle);
}

BOOLEAN WeIsWindowCloaked(
    _In_ HWND WindowHandle
    )
{
#define DWMWA_CLOAKED 14
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static HRESULT (WINAPI *DwmGetWindowAttribute_I)(
        _In_ HWND WindowHandle,
        _In_ ULONG dwAttribute,
        _Out_ PVOID pvAttribute,
        _In_ ULONG cbAttribute
        );
    BOOL windowCloaked = FALSE;

    if (PhBeginInitOnce(&initOnce))
    {
        HANDLE baseAddress;

        if (baseAddress = PhLoadLibrary(L"dwmapi.dll"))
            DwmGetWindowAttribute_I = PhGetProcedureAddress(baseAddress, "DwmGetWindowAttribute", 0);

        PhEndInitOnce(&initOnce);
    }

    if (DwmGetWindowAttribute_I)
        DwmGetWindowAttribute_I(WindowHandle, DWMWA_CLOAKED, &windowCloaked, sizeof(BOOL));

#undef DWMWA_CLOAKED

    return !!windowCloaked;
}

ULONG WeGetWindowBand(
    _In_ HWND WindowHandle
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOL (WINAPI *GetWindowBand_I)(_In_ HWND WindowHandle, _Out_ PULONG pdwBand);
    ULONG windowBand = ULONG_MAX;

    if (PhBeginInitOnce(&initOnce))
    {
        HANDLE baseAddress;

        if (baseAddress = PhLoadLibrary(L"user32.dll"))
            GetWindowBand_I = PhGetProcedureAddress(baseAddress, "GetWindowBand", 0);

        PhEndInitOnce(&initOnce);
    }

    // "Identity and Access Management (IAM)" (dmex)
    if (GetWindowBand_I)
        GetWindowBand_I(WindowHandle, &windowBand);

    return windowBand;
}

BOOLEAN WeIsWindowEffectivelyVisible(
    _In_ HWND WindowHandle
    )
{
    while (WindowHandle && WindowHandle != GetDesktopWindow())
    {
        if (!IsWindowVisible(WindowHandle))
            return FALSE;

        WindowHandle = GetParent(WindowHandle);
    }

    return TRUE;
}

VOID WeDrawWindowBorderForTargeting(
    _In_ HWND WindowHandle
    )
{
    RECT rect;
    HDC hdc;

    if (!PhGetWindowRect(WindowHandle, &rect))
        return;

    hdc = GetDC(NULL);

    if (hdc)
    {
        INT penWidth = 3;
        INT oldDc = SaveDC(hdc);
        HPEN pen = CreatePen(PS_INSIDEFRAME, penWidth, RGB(0x00, 0x00, 0x00));
        HBRUSH brush = GetStockBrush(NULL_BRUSH);

        SetROP2(hdc, R2_NOT);
        SelectPen(hdc, pen);
        SelectBrush(hdc, brush);
        Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

        RestoreDC(hdc, oldDc);
        DeletePen(pen);
        ReleaseDC(NULL, hdc);
    }
}

/**
 * Inverts the border of a window.
 *
 * \param[in] hWnd The window handle.
 */
VOID WeInvertWindowBorder(
    _In_ HWND hWnd
    )
{
    RECT rect;
    HDC hdc;
    LONG dpiValue;

    if (!PhGetWindowRect(hWnd, &rect))
        return;

    hdc = GetWindowDC(hWnd);

    if (hdc)
    {
        INT penWidth;
        INT oldDc;
        HPEN pen;
        HPEN oldPen;
        HBRUSH brush;
        HBRUSH oldBrush;
        POINT topLeft;
        POINT bottomRight;

        dpiValue = PhGetWindowDpi(hWnd);
        penWidth = PhGetSystemMetrics(SM_CXBORDER, dpiValue) * 3;

        // The window DC uses the logical coordinate space of the target window, but the rectangle
        // above is physical. Convert the corners so the border matches DPI virtualized windows.

        topLeft.x = rect.left;
        topLeft.y = rect.top;
        bottomRight.x = rect.right;
        bottomRight.y = rect.bottom;

        WepPhysicalToLogicalPoint(hWnd, &topLeft);
        WepPhysicalToLogicalPoint(hWnd, &bottomRight);

        oldDc = SaveDC(hdc);

        // Get an inversion effect.
        SetROP2(hdc, R2_NOT);

        pen = CreatePen(PS_INSIDEFRAME, penWidth, RGB(0x00, 0x00, 0x00));
        oldPen = SelectPen(hdc, pen);

        brush = PhGetStockBrush(NULL_BRUSH);
        oldBrush = SelectBrush(hdc, brush);

        // Draw the rectangle.
        Rectangle(hdc, 0, 0, bottomRight.x - topLeft.x, bottomRight.y - topLeft.y);

        // Cleanup.
        SelectBrush(hdc, oldBrush);
        SelectPen(hdc, oldPen);

        DeletePen(pen);

        RestoreDC(hdc, oldDc);
        ReleaseDC(hWnd, hdc);
    }
}

/**
 * Determines if the caller identity is a hosted window.
 *
 * \param[in] WindowHandle The window handle.
 * \param[out] IsHosted Set to 1 if hosted, 0 otherwise.
 * \return STATUS_SUCCESS.
 */
NTSTATUS CallerIdentityIsHostedWindow(
    _In_ HWND WindowHandle,
    _Out_ PULONG IsHosted
    )
{
    HWND windowHandle;
    WCHAR className[260];

    *IsHosted = 0;

    for (windowHandle = WindowHandle; windowHandle; windowHandle = GetWindow(windowHandle, GW_HWNDNEXT))
    {
        if (
            NT_SUCCESS(PhGetClassName(windowHandle, className, RTL_NUMBER_OF(className), NULL)) &&
            PhEqualStringZ(className, L"ApplicationHostBridgeWindow", TRUE)
            )
        {
            *IsHosted = 1;
            break;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * Enumerates window tabs for a given window.
 *
 * \param[in] WindowHandle The window handle.
 * \param[out] WindowList A list of window handles.
 * \return STATUS_SUCCESS on success.
 */
NTSTATUS WeEnumerateWindowTabs(
    _In_ HWND WindowHandle,
    _Out_ PPH_LIST *WindowList
    )
{
    IPropertyStore *propStore;
    PROPVARIANT propVar;
    PPH_LIST windowList;
    NTSTATUS status = STATUS_NOT_FOUND;

    *WindowList = NULL;

    if (!SUCCEEDED(SHGetPropertyStoreForWindow(WindowHandle, &IID_IPropertyStore, &propStore)))
        return STATUS_UNSUCCESSFUL;

    PropVariantInit(&propVar);

    if (SUCCEEDED(IPropertyStore_GetValue(propStore, &PKEY_ITaskbarList2, &propVar)))
    {
        if (propVar.vt == (VT_VECTOR | VT_UI4))
        {
            windowList = PhCreateList(propVar.caui.cElems);

            for (ULONG i = 0; i < propVar.caui.cElems; i++)
            {
                HWND tabWindowHandle = (HWND)(ULONG_PTR)propVar.caui.pElems[i];

                if (GetProp(tabWindowHandle, (LPCWSTR)0xA91B) == WindowHandle)
                {
                    PhAddItemList(windowList, tabWindowHandle);
                }
            }

            *WindowList = windowList;
            status = STATUS_SUCCESS;
        }
        else
        {
            PropVariantClear(&propVar);
            IPropertyStore_Release(propStore);
            return STATUS_NOT_FOUND;
        }

        PropVariantClear(&propVar);
    }

    IPropertyStore_Release(propStore);

    return status;
}

/**
 * Determines the subgroup of a window.
 *
 * \param[in] WindowHandle The window handle.
 * \param[out] ResolvedWindow The resolved window handle.
 * \return The window subgroup type.
 */
WE_WINDOW_SUBGROUP WeDetermineWindowSubgroup(
    _In_ HWND WindowHandle,
    _Out_ HWND *ResolvedWindow
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static BOOL (WINAPI *GetWindowBand_I)(HWND, PULONG) = NULL;
    static BOOL (WINAPI *IsShellManagedWindow_I)(HWND) = NULL;
    static BOOL (WINAPI *IsShellFrameWindow_I)(HWND) = NULL;
    static BOOL (WINAPI *GetModernAppWindow_I)(HWND, HWND*) = NULL;

    ULONG windowBand = 0;
    BOOL isShellManagedWindow = FALSE;
    WE_WINDOW_SUBGROUP subgroupType = WeWindowSubgroupNone;
    ULONG isHosted = 0;

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID shell32 = PhLoadLibrary(L"shell32.dll");
        if (shell32)
        {
            GetWindowBand_I = PhGetProcedureAddress(shell32, "GetWindowBand", 0);
            IsShellManagedWindow_I = PhGetProcedureAddress(shell32, "IsShellManagedWindow", 0);
            IsShellFrameWindow_I = PhGetProcedureAddress(shell32, "IsShellFrameWindow", 0);
            GetModernAppWindow_I = PhGetProcedureAddress(shell32, "GetModernAppWindow", 0);
        }
        PhEndInitOnce(&initOnce);
    }

    *ResolvedWindow = NULL;

    if (GetWindowBand_I)
    {
        if (!GetWindowBand_I(WindowHandle, &windowBand))
            return WeWindowSubgroupUnknown;
    }

    if (IsShellManagedWindow_I)
        isShellManagedWindow = IsShellManagedWindow_I(WindowHandle);

    if (windowBand == 2 || windowBand == 16) // ZBID_UIACCESS, ZBID_SYSTEM_TOOLS
        return WeWindowSubgroupSystem;

    if (windowBand == 13) // ZBID_IMMERSIVE_SEARCH
    {
        if (!isShellManagedWindow)
            return WeWindowSubgroupNone;

        return WeWindowSubgroupShell;
    }

    if ((windowBand != 1 && windowBand != 8) || !isShellManagedWindow) // ZBID_DESKTOP, ZBID_IMMERSIVE_INACTIVEMOBODY
        return WeWindowSubgroupNone;

    *ResolvedWindow = WindowHandle;
    subgroupType = WeWindowSubgroupShell;

    if (PhGetWindowStyle(WindowHandle) & WS_VISIBLE)
    {
        subgroupType = WeWindowSubgroupStandard;

        if (!GetWindow(WindowHandle, GW_OWNER))
        {
            if (!NT_SUCCESS(CallerIdentityIsHostedWindow(WindowHandle, &isHosted)) || !isHosted)
            {
                ULONG isCloaked = 0;
                WINDOWCOMPOSITIONATTRIBDATA data;

                data.Attribute = WCA_EVER_UNCLOAKED;
                data.Data = &isCloaked;
                data.Length = sizeof(ULONG);

                if (!NT_SUCCESS(PhGetWindowCompositionAttribute(WindowHandle, &data)) || isCloaked)
                {
                    if (IsShellFrameWindow_I && IsShellFrameWindow_I(WindowHandle))
                    {
                        HWND modernAppWindow = NULL;
                        if (GetModernAppWindow_I && GetModernAppWindow_I(WindowHandle, &modernAppWindow))
                            *ResolvedWindow = modernAppWindow ? modernAppWindow : WindowHandle;
                    }
                }
            }
        }
    }

    return subgroupType;
}

/**
 * Determines the dllhost process name from a window.
 *
 * \param[in] WindowHandle The window handle.
 * \param[in,out] AppInfo The application info structure.
 * \return STATUS_SUCCESS on success.
 */
NTSTATUS WeDetermineDllhostProcessNameFromWindow(
    _In_ HWND WindowHandle,
    _Inout_ PWE_APPLICATION_INFO AppInfo
    )
{
    IPropertyStore *propStore;
    PROPVARIANT propVar;

    if (!SUCCEEDED(SHGetPropertyStoreForWindow(WindowHandle, &IID_IPropertyStore, &propStore)))
        return STATUS_UNSUCCESSFUL;

    PropVariantInit(&propVar);

    if (SUCCEEDED(IPropertyStore_GetValue(propStore, &PKEY_AppUserModel_RelaunchDisplayNameResource, &propVar)))
    {
        if (propVar.vt == VT_LPWSTR)
        {
            PH_STRINGREF sourceRef;

            PhInitializeStringRefLongHint(&sourceRef, propVar.pwszVal);

            if (sourceRef.Length > 0 && sourceRef.Buffer[0] == L'@')
            {
                PPH_STRING resolvedString = PhLoadIndirectString(&sourceRef);

                if (resolvedString)
                {
                    PhSwapReference(&AppInfo->RelaunchDisplayName, resolvedString);
                }
                else
                {
                    PhSwapReference(&AppInfo->RelaunchDisplayName, PhCreateString2(&sourceRef));
                }
            }
            else
            {
                PhSwapReference(&AppInfo->RelaunchDisplayName, PhCreateString2(&sourceRef));
            }
        }

        PropVariantClear(&propVar);
    }

    IPropertyStore_Release(propStore);

    return STATUS_SUCCESS;
}

#ifdef WE_ENABLE_SNAPSHOT_SELECTION

static BOOLEAN WeIntersectRect(
    _Out_ PRECT Result,
    _In_ PRECT Rect1,
    _In_ PRECT Rect2
    )
{
    Result->left = Rect1->left > Rect2->left ? Rect1->left : Rect2->left;
    Result->top = Rect1->top > Rect2->top ? Rect1->top : Rect2->top;
    Result->right = Rect1->right < Rect2->right ? Rect1->right : Rect2->right;
    Result->bottom = Rect1->bottom < Rect2->bottom ? Rect1->bottom : Rect2->bottom;

    return Result->right > Result->left && Result->bottom > Result->top;
}

typedef struct _WE_WINDOW_FROM_POINT_CONTEXT
{
    HWND SnapshotWindow;
    HWND WindowHandle;
    POINT Point;
} WE_WINDOW_FROM_POINT_CONTEXT, *PWE_WINDOW_FROM_POINT_CONTEXT;

static BOOL CALLBACK WeWindowFromPointEnumProc(
    _In_ HWND WindowHandle,
    _In_ LPARAM lParam
    )
{
    PWE_WINDOW_FROM_POINT_CONTEXT context;
    RECT windowRect;

    context = (PWE_WINDOW_FROM_POINT_CONTEXT)lParam;

    if (WindowHandle == context->SnapshotWindow)
        return TRUE;

    if (!IsWindowVisible(WindowHandle))
        return TRUE;

    if (!PhGetWindowRect(WindowHandle, &windowRect))
        return TRUE;

    if (!PtInRect(&windowRect, context->Point))
        return TRUE;

    context->WindowHandle = WindowHandle;

    return FALSE;
}

static HWND WeWindowFromPointBelowSnapshot(
    _In_ HWND SnapshotWindow,
    _In_ POINT Point
    )
{
    WE_WINDOW_FROM_POINT_CONTEXT context;
    HWND windowHandle;

    context.SnapshotWindow = SnapshotWindow;
    context.WindowHandle = NULL;
    context.Point = Point;

    EnumWindows(WeWindowFromPointEnumProc, (LPARAM)&context);

    windowHandle = context.WindowHandle;

    while (windowHandle)
    {
        POINT logicalPoint;
        POINT clientOrigin;
        POINT clientPoint;
        HWND childWindowHandle;

        // The point is physical, but ChildWindowFromPointEx walks child rectangles stored in the
        // logical space of the target window. Convert both the point and the client origin into
        // that space so the difference below is a valid logical client offset. Using ScreenToClient
        // here would subtract a physical origin from a logical point.

        logicalPoint = Point;
        WepPhysicalToLogicalPoint(windowHandle, &logicalPoint);

        clientOrigin.x = 0;
        clientOrigin.y = 0;
        ClientToScreen(windowHandle, &clientOrigin);
        WepPhysicalToLogicalPoint(windowHandle, &clientOrigin);

        clientPoint.x = logicalPoint.x - clientOrigin.x;
        clientPoint.y = logicalPoint.y - clientOrigin.y;

        childWindowHandle = ChildWindowFromPointEx(
            windowHandle,
            clientPoint,
            CWP_SKIPINVISIBLE | CWP_SKIPDISABLED
            );

        if (!childWindowHandle || childWindowHandle == windowHandle)
            break;

        windowHandle = childWindowHandle;
    }

    return windowHandle;
}

static BOOLEAN WeCreateBlurredSnapshotBitmap(
    _Inout_ PWE_SNAPSHOT_CONTEXT Context,
    _In_ HDC ScreenDC
    )
{
    LONG screenWidth;
    LONG screenHeight;
    LONG smallWidth;
    LONG smallHeight;
    HDC smallDC;
    HBITMAP smallBitmap;
    HBITMAP oldSmallBitmap;
    INT oldStretchMode;

    screenWidth = Context->ScreenRect.right - Context->ScreenRect.left;
    screenHeight = Context->ScreenRect.bottom - Context->ScreenRect.top;

    if (screenWidth <= 0 || screenHeight <= 0)
        return FALSE;

    Context->BlurredDC = CreateCompatibleDC(ScreenDC);
    if (!Context->BlurredDC)
        return FALSE;

    Context->BlurredBitmap = PhCreateDIBSection(ScreenDC, PHBF_TOPDOWNDIB, screenWidth, screenHeight, NULL);
    if (!Context->BlurredBitmap)
    {
        DeleteDC(Context->BlurredDC);
        Context->BlurredDC = NULL;
        return FALSE;
    }

    Context->OldBlurredBitmap = SelectBitmap(Context->BlurredDC, Context->BlurredBitmap);

    smallWidth = screenWidth / 24;
    smallHeight = screenHeight / 24;

    if (smallWidth < 1)
        smallWidth = 1;

    if (smallHeight < 1)
        smallHeight = 1;

    smallDC = CreateCompatibleDC(ScreenDC);
    if (!smallDC)
    {
        SelectBitmap(Context->BlurredDC, Context->OldBlurredBitmap);
        DeleteBitmap(Context->BlurredBitmap);
        DeleteDC(Context->BlurredDC);
        Context->BlurredBitmap = NULL;
        Context->BlurredDC = NULL;
        return FALSE;
    }

    smallBitmap = PhCreateDIBSection(ScreenDC, PHBF_TOPDOWNDIB, smallWidth, smallHeight, NULL);
    if (!smallBitmap)
    {
        DeleteDC(smallDC);
        SelectBitmap(Context->BlurredDC, Context->OldBlurredBitmap);
        DeleteBitmap(Context->BlurredBitmap);
        DeleteDC(Context->BlurredDC);
        Context->BlurredBitmap = NULL;
        Context->BlurredDC = NULL;
        return FALSE;
    }

    oldSmallBitmap = SelectBitmap(smallDC, smallBitmap);

    oldStretchMode = SetStretchBltMode(smallDC, HALFTONE);
    SetBrushOrgEx(smallDC, 0, 0, NULL);

    StretchBlt(
        smallDC,
        0,
        0,
        smallWidth,
        smallHeight,
        Context->MemoryDC,
        0,
        0,
        screenWidth,
        screenHeight,
        SRCCOPY
        );

    if (oldStretchMode)
        SetStretchBltMode(smallDC, oldStretchMode);

    oldStretchMode = SetStretchBltMode(Context->BlurredDC, HALFTONE);
    SetBrushOrgEx(Context->BlurredDC, 0, 0, NULL);

    StretchBlt(
        Context->BlurredDC,
        0,
        0,
        screenWidth,
        screenHeight,
        smallDC,
        0,
        0,
        smallWidth,
        smallHeight,
        SRCCOPY
        );

    if (oldStretchMode)
        SetStretchBltMode(Context->BlurredDC, oldStretchMode);

    SelectBitmap(smallDC, oldSmallBitmap);
    DeleteBitmap(smallBitmap);
    DeleteDC(smallDC);

    return TRUE;
}

/**
 * Window procedure for the snapshot selection window.
 *
 * \param[in] WindowHandle The window handle.
 * \param[in] WindowMessage The window message.
 * \param[in] wParam The message WPARAM.
 * \param[in] lParam The message LPARAM.
 * \return LRESULT.
 */
static LRESULT CALLBACK WeSnapshotWindowProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PWE_SNAPSHOT_CONTEXT context;

    if (WindowMessage == WM_CREATE)
    {
        context = ((CREATESTRUCT*)lParam)->lpCreateParams;
        PhSetWindowContext(WindowHandle, 0xFF, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, 0xFF);
    }

    if (!context)
        return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);

    switch (WindowMessage)
    {
    case WM_DESTROY:
        {
            context->IsActive = FALSE;
            PhRemoveWindowContext(WindowHandle, 0xFF);
        }
        break;
    case WM_SHOWWINDOW:
        {
            if (wParam) // Being shown
            {
                InvalidateRect(WindowHandle, NULL, TRUE);
                UpdateWindow(WindowHandle);
            }
        }
        break;
    case WM_ACTIVATE:
        {
            if (LOWORD(wParam) != WA_INACTIVE)
            {
                InvalidateRect(WindowHandle, NULL, FALSE);
            }
        }
        break;
    case WM_ERASEBKGND:
        return TRUE; // Prevent flicker
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(WindowHandle, &ps);

            if (hdc)
            {
                RECT paintRect = ps.rcPaint;
                LONG paintWidth = paintRect.right - paintRect.left;
                LONG paintHeight = paintRect.bottom - paintRect.top;

                // Draw the blurred snapshot first, then restore the hovered window region.
                // Only copy the invalid region, the snapshot is 1:1 with the client area.
                if (context->BlurredDC && context->BlurredBitmap)
                {
                    BitBlt(hdc, paintRect.left, paintRect.top, paintWidth, paintHeight, context->BlurredDC, paintRect.left, paintRect.top, SRCCOPY);
                }
                else if (context->MemoryDC && context->ScreenBitmap)
                {
                    BitBlt(hdc, paintRect.left, paintRect.top, paintWidth, paintHeight, context->MemoryDC, paintRect.left, paintRect.top, SRCCOPY);
                }
                else
                {
                    // Fallback: just fill with black if bitmap failed
                    FillRect(hdc, &paintRect, (HBRUSH)PhGetStockBrush(BLACK_BRUSH));
                }

                // Draw highlight if we have a window
                if (
                    context->HighlightRect.right > context->HighlightRect.left &&
                    context->HighlightRect.bottom > context->HighlightRect.top
                    )
                {
                    RECT highlightRect;
                    HPEN pen;
                    HPEN oldPen;
                    HBRUSH oldBrush;

                    // Convert screen coordinates to client coordinates
                    highlightRect.left = context->HighlightRect.left - context->ScreenRect.left;
                    highlightRect.top = context->HighlightRect.top - context->ScreenRect.top;
                    highlightRect.right = context->HighlightRect.right - context->ScreenRect.left;
                    highlightRect.bottom = context->HighlightRect.bottom - context->ScreenRect.top;

                    if (context->MemoryDC && context->ScreenBitmap)
                    {
                        BitBlt(
                            hdc,
                            highlightRect.left,
                            highlightRect.top,
                            highlightRect.right - highlightRect.left,
                            highlightRect.bottom - highlightRect.top,
                            context->MemoryDC,
                            highlightRect.left,
                            highlightRect.top,
                            SRCCOPY
                            );
                    }

                    // Draw a bright border
                    pen = CreatePen(PS_SOLID, 4, RGB(0, 120, 215));
                    oldPen = SelectPen(hdc, pen);
                    oldBrush = SelectBrush(hdc, GetStockBrush(NULL_BRUSH));

                    Rectangle(
                        hdc,
                        highlightRect.left,
                        highlightRect.top,
                        highlightRect.right,
                        highlightRect.bottom
                        );

                    SelectBrush(hdc, oldBrush);
                    SelectPen(hdc, oldPen);
                    DeletePen(pen);
                }
            }

            EndPaint(WindowHandle, &ps);
        }
        return 0;
    case WM_MOUSEMOVE:
        {
            POINT pt;
            HWND windowUnderCursor;
            RECT windowRect;
            RECT clippedRect;

            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            ClientToScreen(WindowHandle, &pt);
            windowUnderCursor = WeWindowFromPointBelowSnapshot(WindowHandle, pt);

            // Get window rectangle
            if (
                windowUnderCursor &&
                PhGetWindowRect(windowUnderCursor, &windowRect) &&
                WeIntersectRect(&clippedRect, &windowRect, &context->ScreenRect)
                )
            {
                // Only update if window changed
                if (
                    context->TargetWindow != windowUnderCursor ||
                    !EqualRect(&context->HighlightRect, &clippedRect)
                    )
                {
                    context->TargetWindow = windowUnderCursor;
                    context->HighlightRect = clippedRect;

                    // Invalidate and force immediate repaint for smooth updates
                    InvalidateRect(WindowHandle, NULL, FALSE);
                    UpdateWindow(WindowHandle);
                }
            }
            else
            {
                // Clear highlight if no window under cursor
                if (context->TargetWindow != NULL)
                {
                    context->TargetWindow = NULL;
                    RtlZeroMemory(&context->HighlightRect, sizeof(RECT));
                    InvalidateRect(WindowHandle, NULL, FALSE);
                    UpdateWindow(WindowHandle);
                }
            }
        }
        return 0;
    case WM_LBUTTONDOWN:
        {
            POINT pt;

            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            ClientToScreen(WindowHandle, &pt);
            context->TargetWindow = WeWindowFromPointBelowSnapshot(WindowHandle, pt);

            context->IsActive = FALSE;
            DestroyWindow(WindowHandle);
            return 0;
        }
    case WM_KEYDOWN:
        {
            if (wParam == VK_ESCAPE)
            {
                context->TargetWindow = NULL;
                context->IsActive = FALSE;
                DestroyWindow(WindowHandle);
                return 0;
            }
        }
        break;
    case WM_SETCURSOR:
        {
            SetCursor(LoadCursor(NULL, IDC_CROSS));
        }
        return TRUE;
    }

    return DefWindowProc(WindowHandle, WindowMessage, wParam, lParam);
}

/**
 * Creates a snapshot selection.
 *
 * \param[out] Context The snapshot context.
 * \return TRUE on success, FALSE on failure.
 */
BOOLEAN WeCreateSnapshotSelection(
    _Out_ PWE_SNAPSHOT_CONTEXT Context
    )
{
    RECT screenRect;
    HDC screenDC;
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };

    RtlZeroMemory(Context, sizeof(WE_SNAPSHOT_CONTEXT));

    // Get screen dimensions
    screenRect.left = PhGetSystemMetrics(SM_XVIRTUALSCREEN, 0);
    screenRect.top = PhGetSystemMetrics(SM_YVIRTUALSCREEN, 0);
    screenRect.right = screenRect.left + PhGetSystemMetrics(SM_CXVIRTUALSCREEN, 0);
    screenRect.bottom = screenRect.top + PhGetSystemMetrics(SM_CYVIRTUALSCREEN, 0);
    Context->ScreenRect = screenRect;

    // Capture the screen
    screenDC = GetDC(NULL);
    if (!screenDC)
        return FALSE;

    Context->MemoryDC = CreateCompatibleDC(screenDC);
    if (!Context->MemoryDC)
    {
        ReleaseDC(NULL, screenDC);
        return FALSE;
    }

    Context->ScreenBitmap = PhCreateDIBSection(
        screenDC,
        PHBF_TOPDOWNDIB,
        screenRect.right - screenRect.left,
        screenRect.bottom - screenRect.top,
        NULL
        );

    if (!Context->ScreenBitmap)
    {
        WeDestroySnapshotSelection(Context);
        ReleaseDC(NULL, screenDC);
        return FALSE;
    }

    Context->OldScreenBitmap = SelectBitmap(Context->MemoryDC, Context->ScreenBitmap);

    // Copy screen to bitmap
    BitBlt(Context->MemoryDC, 0, 0,
        screenRect.right - screenRect.left,
        screenRect.bottom - screenRect.top,
        screenDC, screenRect.left, screenRect.top, SRCCOPY);

    if (!WeCreateBlurredSnapshotBitmap(Context, screenDC))
    {
        ReleaseDC(NULL, screenDC);
        WeDestroySnapshotSelection(Context);
        return FALSE;
    }

    ReleaseDC(NULL, screenDC);

    // Register window class
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WeSnapshotWindowProc;
    wcex.hInstance = PluginInstance->DllBase;
    wcex.hCursor = LoadCursor(NULL, IDC_CROSS);
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszClassName = L"WeSnapshotWindow";

    if (!RegisterClassEx(&wcex) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        WeDestroySnapshotSelection(Context);
        return FALSE;
    }

    // Create fullscreen overlay window
    Context->SnapshotWindow = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"WeSnapshotWindow",
        NULL,
        WS_POPUP | WS_VISIBLE,
        screenRect.left, screenRect.top,
        screenRect.right - screenRect.left,
        screenRect.bottom - screenRect.top,
        NULL, NULL,
        PluginInstance->DllBase,
        Context
        );

    if (!Context->SnapshotWindow)
    {
        WeDestroySnapshotSelection(Context);
        return FALSE;
    }

    Context->IsActive = TRUE;

    // Force window to show and paint
    ShowWindow(Context->SnapshotWindow, SW_SHOWNA);
    SetWindowPos(Context->SnapshotWindow, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    UpdateWindow(Context->SnapshotWindow);
    SetForegroundWindow(Context->SnapshotWindow);
    SetFocus(Context->SnapshotWindow);

    return TRUE;
}

/**
 * Destroys a snapshot selection.
 *
 * \param[in,out] Context The snapshot context.
 */
VOID WeDestroySnapshotSelection(
    _Inout_ PWE_SNAPSHOT_CONTEXT Context
    )
{
    if (Context->SnapshotWindow)
    {
        DestroyWindow(Context->SnapshotWindow);
        Context->SnapshotWindow = NULL;
    }

    if (Context->ScreenBitmap)
    {
        if (Context->MemoryDC && Context->OldScreenBitmap)
        {
            SelectBitmap(Context->MemoryDC, Context->OldScreenBitmap);
            Context->OldScreenBitmap = NULL;
        }

        DeleteBitmap(Context->ScreenBitmap);
        Context->ScreenBitmap = NULL;
    }

    if (Context->BlurredBitmap)
    {
        if (Context->BlurredDC && Context->OldBlurredBitmap)
        {
            SelectBitmap(Context->BlurredDC, Context->OldBlurredBitmap);
            Context->OldBlurredBitmap = NULL;
        }

        DeleteBitmap(Context->BlurredBitmap);
        Context->BlurredBitmap = NULL;
    }

    if (Context->MemoryDC)
    {
        DeleteDC(Context->MemoryDC);
        Context->MemoryDC = NULL;
    }

    if (Context->BlurredDC)
    {
        DeleteDC(Context->BlurredDC);
        Context->BlurredDC = NULL;
    }

    Context->IsActive = FALSE;
}

/**
 * Gets the window handle from a snapshot selection.
 *
 * \param[in] Context The snapshot context.
 * \return The window handle.
 */
HWND WeGetWindowFromSnapshot(
    _In_ PWE_SNAPSHOT_CONTEXT Context
    )
{
    return Context->TargetWindow;
}

#endif
