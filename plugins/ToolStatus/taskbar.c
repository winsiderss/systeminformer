/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2020-2023
 *
 */

#include "toolstatus.h"

PH_TASKBAR_ICON TaskbarListIconType = TASKBAR_ICON_NONE;
BOOLEAN TaskbarIsDirty = FALSE;
BOOLEAN TaskbarMainWndExiting = FALSE;
BOOLEAN TaskbarTransparencyEnabled = FALSE;
static HANDLE TaskbarListClass = NULL;
static HANDLE TaskbarThreadHandle = NULL;
static HANDLE TaskbarEventHandle = NULL;

VOID NTAPI TaskbarInitialize(
    VOID
    )
{
    TaskbarTransparencyEnabled = !!PhGetIntegerSetting(SETTING_ICON_TRANSPARENCY_ENABLED);

    if (TaskbarListIconType != TASKBAR_ICON_NONE)
    {
        static PH_INITONCE initOnce = PH_INITONCE_INIT;

        if (PhBeginInitOnce(&initOnce))
        {
            if (NT_SUCCESS(PhCreateEvent(
                &TaskbarEventHandle,
                EVENT_ALL_ACCESS,
                SynchronizationEvent,
                FALSE
                )))
            {
                // Use a separate thread so we don't block the main GUI or
                // the provider threads when explorer is not responding. (dmex)
                PhCreateThreadEx(&TaskbarThreadHandle, TaskbarIconUpdateThread, NULL);
            }

            PhEndInitOnce(&initOnce);
        }
    }
}

VOID NTAPI TaskbarUpdateEvents(
    VOID
    )
{
    if (TaskbarEventHandle)
    {
        NtSetEvent(TaskbarEventHandle, NULL);
    }
}

VOID NTAPI TaskbarUpdateGraphs(
    VOID
    )
{
    if (TaskbarListIconType == TASKBAR_ICON_NONE)
    {
        if (TaskbarIsDirty)
        {
            if (TaskbarListClass)
            {
                PhTaskbarListSetOverlayIcon(TaskbarListClass, MainWindowHandle, NULL, NULL);
                PhTaskbarListDestroy(TaskbarListClass);
                TaskbarListClass = NULL;
            }

            TaskbarIsDirty = FALSE;
        }
    }
    else
    {
        HICON overlayIcon = NULL;

        if (!TaskbarListClass)
        {
            if (!HR_SUCCESS(PhTaskbarListCreate(&TaskbarListClass)))
            {
                TaskbarListClass = NULL;
            }
        }

        if (TaskbarIsDirty)
        {
            if (TaskbarListClass)
                PhTaskbarListSetOverlayIcon(TaskbarListClass, MainWindowHandle, NULL, NULL);
            TaskbarIsDirty = FALSE;
        }

        switch (TaskbarListIconType)
        {
        case TASKBAR_ICON_CPU_HISTORY:
            overlayIcon = PhUpdateIconCpuHistory(&SystemStatistics);
            break;
        case TASKBAR_ICON_IO_HISTORY:
            overlayIcon = PhUpdateIconIoHistory(&SystemStatistics);
            break;
        case TASKBAR_ICON_COMMIT_HISTORY:
            overlayIcon = PhUpdateIconCommitHistory(&SystemStatistics);
            break;
        case TASKBAR_ICON_PHYSICAL_HISTORY:
            overlayIcon = PhUpdateIconPhysicalHistory(&SystemStatistics);
            break;
        case TASKBAR_ICON_CPU_USAGE:
            overlayIcon = PhUpdateIconCpuUsage(&SystemStatistics);
            break;
        }

        if (overlayIcon)
        {
            if (TaskbarListClass)
                PhTaskbarListSetOverlayIcon(TaskbarListClass, MainWindowHandle, overlayIcon, NULL);
            DestroyIcon(overlayIcon);
        }
    }
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS TaskbarIconUpdateThread(
    _In_opt_ PVOID Context
    )
{
    PhSetThreadName(NtCurrentThread(), L"TaskbarIconUpdateThread");

    while (TRUE)
    {
        if (TaskbarMainWndExiting)
            break;

        if (!TaskbarEventHandle)
        {
            PhDelayExecution(1000);
            continue;
        }

        if (NT_SUCCESS(NtWaitForSingleObject(TaskbarEventHandle, FALSE, NULL)))
        {
            TaskbarUpdateGraphs();
        }
    }

    return STATUS_SUCCESS;
}

typedef struct _PH_NF_BITMAP
{
    HDC Hdc;
    HBITMAP Bitmap;
    PVOID Bits;
    LONG Width;
    LONG Height;
    LONG TaskbarDpi;
} PH_NF_BITMAP, *PPH_NF_BITMAP;

static PH_NF_BITMAP PhDefaultBitmapContext = { 0 };
static PH_NF_BITMAP PhBlackBitmapContext = { 0 };
static HBITMAP PhBlackBitmap = NULL;
static HICON PhBlackIcon = NULL;

static VOID PhBeginBitmap2(
    _Inout_ PPH_NF_BITMAP Context,
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ HBITMAP *Bitmap,
    _Out_opt_ PVOID *Bits,
    _Out_ HDC *Hdc,
    _Out_ HBITMAP *OldBitmap
    )
{
    LONG dpiValue = PhGetTaskbarDpi();

    // Initialize and cache the current system metrics. (dmex)

    if (Context->TaskbarDpi == 0 || Context->TaskbarDpi != dpiValue)
    {
        Context->Width = PhGetSystemMetrics(SM_CXSMICON, dpiValue);
        Context->Height = PhGetSystemMetrics(SM_CXSMICON, dpiValue);
    }

    // Cleanup existing resources when the DPI changes so we're able to re-initialize with updated DPI. (dmex)

    if (Context->TaskbarDpi != 0 && Context->TaskbarDpi != dpiValue)
    {
        if (PhBlackIcon)
        {
            DestroyIcon(PhBlackIcon);
            PhBlackIcon = NULL;
        }

        if (Context->Hdc)
        {
            DeleteDC(Context->Hdc);
            Context->Hdc = NULL;
        }

        if (Context->Bitmap)
        {
            DeleteBitmap(Context->Bitmap);
            Context->Bitmap = NULL;
        }
    }

    if (!Context->Bitmap)
    {
        Context->Hdc = CreateCompatibleDC(NULL);
        Context->Bitmap = PhCreateDIBSection(Context->Hdc, PHBF_DIB, Context->Width, Context->Height, &Context->Bits);
        Context->TaskbarDpi = dpiValue;
    }

    *Width = Context->Width;
    *Height = Context->Height;
    *Bitmap = Context->Bitmap;
    *Bits = Context->Bits;
    *Hdc = Context->Hdc;
    *OldBitmap = SelectBitmap(Context->Hdc, Context->Bitmap);
}

static VOID PhBeginBitmap(
    _Out_ PULONG Width,
    _Out_ PULONG Height,
    _Out_ HBITMAP *Bitmap,
    _Out_opt_ PVOID *Bits,
    _Out_ HDC *Hdc,
    _Out_ HBITMAP *OldBitmap
    )
{
    PhBeginBitmap2(&PhDefaultBitmapContext, Width, Height, Bitmap, Bits, Hdc, OldBitmap);
}

HICON PhGetBlackIcon(
    VOID
    )
{
    if (!PhBlackIcon)
    {
        ULONG width;
        ULONG height;
        PVOID bits;
        HDC hdc;
        HBITMAP mask;
        HBITMAP oldBitmap;
        ICONINFO iconInfo;

        PhBeginBitmap2(&PhBlackBitmapContext, &width, &height, &PhBlackBitmap, &bits, &hdc, &oldBitmap);
        memset(bits, TaskbarTransparencyEnabled ? 1 : 0, width * height * sizeof(RGBQUAD));

        if (!(mask = CreateBitmap(width, height, 1, 1, NULL)))
            return NULL;

        iconInfo.fIcon = TRUE;
        iconInfo.xHotspot = 0;
        iconInfo.yHotspot = 0;
        iconInfo.hbmMask = mask;
        iconInfo.hbmColor = PhBlackBitmap;
        PhBlackIcon = CreateIconIndirect(&iconInfo);

        SelectBitmap(hdc, oldBitmap);
        DeleteBitmap(mask);
    }

    return PhBlackIcon;
}

HICON PhBitmapToIcon(
    _In_ HBITMAP Bitmap
    )
{
    ICONINFO iconInfo;

    PhGetBlackIcon();

    iconInfo.fIcon = TRUE;
    iconInfo.xHotspot = 0;
    iconInfo.yHotspot = 0;
    iconInfo.hbmMask = PhBlackBitmap;
    iconInfo.hbmColor = Bitmap;

    return CreateIconIndirect(&iconInfo);
}

HICON PhUpdateIconCpuHistory(
    _In_ PPH_PLUGIN_SYSTEM_STATISTICS Statistics
    )
{
    static PH_GRAPH_DRAW_INFO drawInfo =
    {
        16,
        16,
        PH_GRAPH_USE_LINE_2,
        2,
        RGB(0x00, 0x00, 0x00),
        16,
        NULL,
        NULL,
        0,
        0,
        0,
        0
    };
    ULONG maxDataCount;
    ULONG lineDataCount;
    PFLOAT lineData1;
    PFLOAT lineData2;
    HBITMAP bitmap;
    PVOID bits;
    HDC hdc;
    HBITMAP oldBitmap;
    HICON icon;

    // Icon
    PhBeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);
    maxDataCount = drawInfo.Width / 2 + 1;

    if (!(lineData1 = _malloca(maxDataCount * sizeof(FLOAT))))
        return NULL;
    if (!(lineData2 = _malloca(maxDataCount * sizeof(FLOAT))))
        return NULL;

    lineDataCount = min(maxDataCount, Statistics->CpuKernelHistory->Count);
    PhCopyCircularBuffer_FLOAT(Statistics->CpuKernelHistory, lineData1, lineDataCount);
    PhCopyCircularBuffer_FLOAT(Statistics->CpuUserHistory, lineData2, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineData2 = lineData2;
    drawInfo.LineColor1 = PhGetIntegerSetting(SETTING_COLOR_CPU_KERNEL);
    drawInfo.LineColor2 = PhGetIntegerSetting(SETTING_COLOR_CPU_USER);
    drawInfo.LineBackColor1 = PhHalveColorBrightness(drawInfo.LineColor1);
    drawInfo.LineBackColor2 = PhHalveColorBrightness(drawInfo.LineColor2);
    PhDrawGraphDirect(hdc, bits, &drawInfo);

    SelectBitmap(hdc, oldBitmap);
    icon = PhBitmapToIcon(bitmap);

    _freea(lineData2);
    _freea(lineData1);

    return icon;
}

HICON PhUpdateIconIoHistory(
    _In_ PPH_PLUGIN_SYSTEM_STATISTICS Statistics
    )
{
    static PH_GRAPH_DRAW_INFO drawInfo =
    {
        16,
        16,
        PH_GRAPH_USE_LINE_2,
        2,
        RGB(0x00, 0x00, 0x00),
        16,
        NULL,
        NULL,
        0,
        0,
        0,
        0
    };
    ULONG maxDataCount;
    ULONG lineDataCount;
    PFLOAT lineData1;
    PFLOAT lineData2;
    FLOAT max;
    ULONG i;
    HBITMAP bitmap;
    PVOID bits;
    HDC hdc;
    HBITMAP oldBitmap;
    HICON icon;

    // Icon
    PhBeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);
    maxDataCount = drawInfo.Width / 2 + 1;

    if (!(lineData1 = _malloca(maxDataCount * sizeof(FLOAT))))
        return NULL;
    if (!(lineData2 = _malloca(maxDataCount * sizeof(FLOAT))))
        return NULL;

    lineDataCount = min(maxDataCount, Statistics->IoReadHistory->Count);
    max = 1024 * 1024; // minimum scaling of 1 MB.

    for (i = 0; i < lineDataCount; i++)
    {
        lineData1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG64(Statistics->IoReadHistory, i)
                     + (FLOAT)PhGetItemCircularBuffer_ULONG64(Statistics->IoOtherHistory, i);
        lineData2[i] = (FLOAT)PhGetItemCircularBuffer_ULONG64(Statistics->IoWriteHistory, i);

        if (max < lineData1[i] + lineData2[i])
            max = lineData1[i] + lineData2[i];
    }

    PhDivideSinglesBySingle(lineData1, max, lineDataCount);
    PhDivideSinglesBySingle(lineData2, max, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineData2 = lineData2;
    drawInfo.LineColor1 = PhGetIntegerSetting(SETTING_COLOR_IO_READ_OTHER);
    drawInfo.LineColor2 = PhGetIntegerSetting(SETTING_COLOR_IO_WRITE);
    drawInfo.LineBackColor1 = PhHalveColorBrightness(drawInfo.LineColor1);
    drawInfo.LineBackColor2 = PhHalveColorBrightness(drawInfo.LineColor2);
    PhDrawGraphDirect(hdc, bits, &drawInfo);

    if (TaskbarTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, drawInfo.Width, drawInfo.Height);
    }

    SelectBitmap(hdc, oldBitmap);
    icon = PhBitmapToIcon(bitmap);

    _freea(lineData2);
    _freea(lineData1);

    return icon;
}

HICON PhUpdateIconCommitHistory(
    _In_ PPH_PLUGIN_SYSTEM_STATISTICS Statistics
    )
{
    static PH_GRAPH_DRAW_INFO drawInfo =
    {
        16,
        16,
        0,
        2,
        RGB(0x00, 0x00, 0x00),
        16,
        NULL,
        NULL,
        0,
        0,
        0,
        0
    };
    ULONG maxDataCount;
    ULONG lineDataCount;
    PFLOAT lineData1;
    ULONG i;
    HBITMAP bitmap;
    PVOID bits;
    HDC hdc;
    HBITMAP oldBitmap;
    HICON icon;

    // Icon
    PhBeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);
    maxDataCount = drawInfo.Width / 2 + 1;

    if (!(lineData1 = _malloca(maxDataCount * sizeof(FLOAT))))
        return NULL;

    lineDataCount = min(maxDataCount, Statistics->CommitHistory->Count);

    for (i = 0; i < lineDataCount; i++)
        lineData1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(Statistics->CommitHistory, i);

    PhDivideSinglesBySingle(lineData1, (FLOAT)Statistics->Performance->CommitLimit, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineColor1 = PhGetIntegerSetting(SETTING_COLOR_PRIVATE);
    drawInfo.LineBackColor1 = PhHalveColorBrightness(drawInfo.LineColor1);
    PhDrawGraphDirect(hdc, bits, &drawInfo);

    if (TaskbarTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, drawInfo.Width, drawInfo.Height);
    }

    SelectBitmap(hdc, oldBitmap);
    icon = PhBitmapToIcon(bitmap);

    _freea(lineData1);

    return icon;
}

HICON PhUpdateIconPhysicalHistory(
    _In_ PPH_PLUGIN_SYSTEM_STATISTICS Statistics
    )
{
    static PH_GRAPH_DRAW_INFO drawInfo =
    {
        16,
        16,
        0,
        2,
        RGB(0x00, 0x00, 0x00),
        16,
        NULL,
        NULL,
        0,
        0,
        0,
        0
    };
    ULONG maxDataCount;
    ULONG lineDataCount;
    PFLOAT lineData1;
    ULONG i;
    HBITMAP bitmap;
    PVOID bits;
    HDC hdc;
    HBITMAP oldBitmap;
    HICON icon;

    // Icon
    PhBeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);
    maxDataCount = drawInfo.Width / 2 + 1;

    if (!(lineData1 = _malloca(maxDataCount * sizeof(FLOAT))))
        return NULL;

    lineDataCount = min(maxDataCount, Statistics->CommitHistory->Count);

    for (i = 0; i < lineDataCount; i++)
        lineData1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(Statistics->PhysicalHistory, i);

    PhDivideSinglesBySingle(lineData1, (FLOAT)PhSystemBasicInformation.NumberOfPhysicalPages, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineColor1 = PhGetIntegerSetting(SETTING_COLOR_PHYSICAL);
    drawInfo.LineBackColor1 = PhHalveColorBrightness(drawInfo.LineColor1);
    PhDrawGraphDirect(hdc, bits, &drawInfo);

    SelectBitmap(hdc, oldBitmap);
    icon = PhBitmapToIcon(bitmap);

    _freea(lineData1);

    return icon;
    return 0;
}

HICON PhUpdateIconCpuUsage(
    _In_ PPH_PLUGIN_SYSTEM_STATISTICS Statistics
    )
{
    ULONG width;
    ULONG height;
    HBITMAP bitmap;
    PVOID bits;
    HDC hdc;
    HBITMAP oldBitmap;
    HICON icon;

    // Icon
    PhBeginBitmap(&width, &height, &bitmap, &bits, &hdc, &oldBitmap);

    // This stuff is copied from CpuUsageIcon.cs (PH 1.x).
    {
        COLORREF kColor = PhGetIntegerSetting(SETTING_COLOR_CPU_KERNEL);
        COLORREF uColor = PhGetIntegerSetting(SETTING_COLOR_CPU_USER);
        COLORREF kbColor = PhHalveColorBrightness(kColor);
        COLORREF ubColor = PhHalveColorBrightness(uColor);
        FLOAT k = Statistics->CpuKernelUsage;
        FLOAT u = Statistics->CpuUserUsage;
        LONG kl = (LONG)(k * height);
        LONG ul = (LONG)(u * height);
        RECT rect;
        HBRUSH dcBrush;
        HPEN dcPen;
        POINT points[2];

        dcBrush = PhGetStockBrush(DC_BRUSH);
        dcPen = PhGetStockPen(DC_PEN);
        rect.left = 0;
        rect.top = 0;
        rect.right = width;
        rect.bottom = height;
        SelectBrush(hdc, dcBrush);
        SetDCBrushColor(hdc, RGB(0x00, 0x00, 0x00));
        FillRect(hdc, &rect, dcBrush);

        // Draw the base line.
        if (kl + ul == 0)
        {
            SelectPen(hdc, dcPen);
            SetDCPenColor(hdc, uColor);
            points[0].x = 0;
            points[0].y = height - 1;
            points[1].x = width;
            points[1].y = height - 1;
            Polyline(hdc, points, 2);
        }
        else
        {
            rect.left = 0;
            rect.top = height - ul - kl;
            rect.right = width;
            rect.bottom = height - kl;
            SelectBrush(hdc, dcBrush);
            SetDCBrushColor(hdc, ubColor);
            FillRect(hdc, &rect, dcBrush);

            points[0].x = 0;
            points[0].y = height - 1 - ul - kl;
            if (points[0].y < 0) points[0].y = 0;
            points[1].x = width;
            points[1].y = points[0].y;
            SelectPen(hdc, dcPen);
            SetDCPenColor(hdc, uColor);
            Polyline(hdc, points, 2);

            if (kl != 0)
            {
                rect.left = 0;
                rect.top = height - kl;
                rect.right = width;
                rect.bottom = height;
                SelectBrush(hdc, dcBrush);
                SetDCBrushColor(hdc, kbColor);
                FillRect(hdc, &rect, dcBrush);

                points[0].x = 0;
                points[0].y = height - 1 - kl;
                if (points[0].y < 0) points[0].y = 0;
                points[1].x = width;
                points[1].y = points[0].y;
                SelectPen(hdc, dcPen);
                SetDCPenColor(hdc, kColor);
                Polyline(hdc, points, 2);
            }
        }
    }

    SelectBitmap(hdc, oldBitmap);
    icon = PhBitmapToIcon(bitmap);

    return icon;
}
