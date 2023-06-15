/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011
 *     dmex    2017-2018
 *
 */

#include "wndexp.h"

// WARNING: No functions from ProcessHacker.exe should be used in this file!

HWND WeGetMainWindowHandle(
    VOID
    )
{
    static HWND (WINAPI* PhGetMainWindowHandle_I)(VOID) = NULL;

    if (!PhGetMainWindowHandle_I)
        PhGetMainWindowHandle_I = WeGetProcedureAddress("PhGetMainWindowHandle");
    if (!PhGetMainWindowHandle_I)
        return NULL;

    return PhGetMainWindowHandle_I();
}

PVOID WeGetProcedureAddress(
    _In_ PCSTR Name
    )
{
    return GetProcAddress(NtCurrentPeb()->ImageBaseAddress, Name);
}

VOID WeFormatLocalObjectName(
    _In_ PWSTR OriginalName,
    _Inout_updates_(256) PWCHAR Buffer,
    _Out_ PUNICODE_STRING ObjectName
    )
{
    SIZE_T length;
    SIZE_T originalNameLength;

    // Sessions other than session 0 require SeCreateGlobalPrivilege.
    if (NtCurrentPeb()->SessionId != 0)
    {
        memcpy(Buffer, L"\\Sessions\\", 10 * sizeof(WCHAR));
        _ultow(NtCurrentPeb()->SessionId, Buffer + 10, 10);
        length = PhCountStringZ(Buffer);
        originalNameLength = PhCountStringZ(OriginalName);
        memcpy(Buffer + length, OriginalName, (originalNameLength + 1) * sizeof(WCHAR));
        length += originalNameLength;

        ObjectName->Buffer = Buffer;
        ObjectName->MaximumLength = (ObjectName->Length = (USHORT)(length * sizeof(WCHAR))) + sizeof(WCHAR);
    }
    else
    {
        RtlInitUnicodeString(ObjectName, OriginalName);
    }
}

VOID WeInvertWindowBorder(
    _In_ HWND hWnd
    )
{
    RECT rect;
    HDC hdc;
    LONG dpiValue;

    GetWindowRect(hWnd, &rect);

    dpiValue = PhGetWindowDpi(hWnd);

    if (hdc = GetWindowDC(hWnd))
    {
        ULONG penWidth = PhGetSystemMetrics(SM_CXBORDER, dpiValue) * 3;
        INT oldDc;
        HPEN pen;
        HBRUSH brush;

        oldDc = SaveDC(hdc);

        // Get an inversion effect.
        SetROP2(hdc, R2_NOT);

        pen = CreatePen(PS_INSIDEFRAME, penWidth, RGB(0x00, 0x00, 0x00));
        SelectPen(hdc, pen);

        brush = GetStockBrush(NULL_BRUSH);
        SelectBrush(hdc, brush);

        // Draw the rectangle.
        Rectangle(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top);

        // Cleanup.
        DeletePen(pen);
        RestoreDC(hdc, oldDc);
        ReleaseDC(hWnd, hdc);
    }
}
