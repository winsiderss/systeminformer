/*
 * Process Hacker Window Explorer -
 *   utility functions
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

#include "wndexp.h"

PVOID WeGetProcedureAddress(
    __in PSTR Name
    )
{
    static PVOID imageBase = NULL;

    if (!imageBase)
        imageBase = GetModuleHandle(L"ProcessHacker.exe");

    return (PVOID)GetProcAddress(imageBase, Name);
}

VOID WeFormatLocalObjectName(
    __in PWSTR OriginalName,
    __inout_ecount(256) PWCHAR Buffer,
    __out PUNICODE_STRING ObjectName
    )
{
    SIZE_T length;
    SIZE_T originalNameLength;

    // Sessions other than session 0 require SeCreateGlobalPrivilege.
    if (NtCurrentPeb()->SessionId != 0)
    {
        memcpy(Buffer, L"\\Sessions\\", 10 * sizeof(WCHAR));
        _ultow(NtCurrentPeb()->SessionId, Buffer + 10, 10);
        length = wcslen(Buffer);
        originalNameLength = wcslen(OriginalName);
        memcpy(Buffer + length, OriginalName, (originalNameLength + 1) * sizeof(WCHAR));
        length += originalNameLength;

        ObjectName->Buffer = Buffer;
        ObjectName->MaximumLength = ObjectName->Length = (USHORT)(length * sizeof(WCHAR));
    }
    else
    {
        RtlInitUnicodeString(ObjectName, OriginalName);
    }
}

VOID WeInvertWindowBorder(
    __in HWND hWnd
    )
{
    RECT rect;
    HDC hdc;

    GetWindowRect(hWnd, &rect);
    hdc = GetWindowDC(hWnd);

    if (hdc)
    {
        ULONG penWidth = GetSystemMetrics(SM_CXBORDER) * 3;
        INT oldDc;
        HPEN pen;
        HBRUSH brush;

        oldDc = SaveDC(hdc);

        // Get an inversion effect.
        SetROP2(hdc, R2_NOT);

        pen = CreatePen(PS_INSIDEFRAME, penWidth, RGB(0x00, 0x00, 0x00));
        SelectObject(hdc, pen);

        brush = GetStockObject(NULL_BRUSH);
        SelectObject(hdc, brush);

        // Draw the rectangle.
        Rectangle(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top);

        // Cleanup.
        DeleteObject(pen);

        RestoreDC(hdc, oldDc);
        ReleaseDC(hWnd, hdc);
    }
}
