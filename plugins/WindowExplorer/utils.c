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

VOID WeInvertWindowBorder(
    _In_ HWND hWnd
    )
{
    RECT rect;
    HDC hdc;
    LONG dpiValue;

    GetWindowRect(hWnd, &rect);
    hdc = GetWindowDC(hWnd);

    if (hdc)
    {
        INT penWidth;
        INT oldDc;
        HPEN pen;
        HPEN oldPen;
        HBRUSH brush;
        HBRUSH oldBrush;

        dpiValue = PhGetWindowDpi(hWnd);
        penWidth = PhGetSystemMetrics(SM_CXBORDER, dpiValue) * 3;

        oldDc = SaveDC(hdc);

        // Get an inversion effect.
        SetROP2(hdc, R2_NOT);

        pen = CreatePen(PS_INSIDEFRAME, penWidth, RGB(0x00, 0x00, 0x00));
        oldPen = SelectPen(hdc, pen);

        brush = PhGetStockBrush(NULL_BRUSH);
        oldBrush = SelectBrush(hdc, brush);

        // Draw the rectangle.
        Rectangle(hdc, 0, 0, rect.right - rect.left, rect.bottom - rect.top);

        // Cleanup.
        SelectBrush(hdc, oldBrush);
        SelectPen(hdc, oldPen);

        DeletePen(pen);

        RestoreDC(hdc, oldDc);
        ReleaseDC(hWnd, hdc);
    }
}
