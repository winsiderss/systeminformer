#include <phapp.h>
#include <settings.h>
#include <graph.h>

static HICON BlackIcon = NULL;
static HBITMAP BlackIconMask = NULL;

HICON PhpGetBlackIcon()
{
    if (!BlackIcon)
    {
        ICONINFO iconInfo;

        BlackIcon = (HICON)LoadImage(PhInstanceHandle, MAKEINTRESOURCE(IDI_BLACK), IMAGE_ICON, 16, 16, 0);

        GetIconInfo(BlackIcon, &iconInfo);

        BlackIconMask = iconInfo.hbmMask;
        DeleteObject(iconInfo.hbmColor);
    }

    return BlackIcon;
}

VOID PhAddNotifyIcon(
    __in ULONG Id
    )
{
    NOTIFYICONDATA notifyIcon = { NOTIFYICONDATA_V3_SIZE };

    notifyIcon.hWnd = PhMainWndHandle;
    notifyIcon.uID = Id;
    notifyIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    notifyIcon.uCallbackMessage = WM_PH_NOTIFY_ICON_MESSAGE;

    wcsncpy_s(notifyIcon.szTip, sizeof(notifyIcon.szTip) / sizeof(WCHAR), PhApplicationName, _TRUNCATE);

    notifyIcon.hIcon = PhpGetBlackIcon();

    Shell_NotifyIcon(NIM_ADD, &notifyIcon);
}

VOID PhRemoveNotifyIcon(
    __in ULONG Id
    )
{
    NOTIFYICONDATA notifyIcon = { NOTIFYICONDATA_V3_SIZE };

    notifyIcon.hWnd = PhMainWndHandle;
    notifyIcon.uID = Id;

    Shell_NotifyIcon(NIM_DELETE, &notifyIcon);
}

VOID PhModifyNotifyIcon(
    __in ULONG Id,
    __in ULONG Flags,
    __in_opt PWSTR Text,
    __in_opt HICON Icon
    )
{
    NOTIFYICONDATA notifyIcon = { NOTIFYICONDATA_V3_SIZE };

    notifyIcon.hWnd = PhMainWndHandle;
    notifyIcon.uID = Id;
    notifyIcon.uFlags = Flags;

    if (Text)
        wcsncpy_s(notifyIcon.szTip, sizeof(notifyIcon.szTip) / sizeof(WCHAR), Text, _TRUNCATE);

    notifyIcon.hIcon = Icon;

    if (!Shell_NotifyIcon(NIM_MODIFY, &notifyIcon))
    {
        // Explorer probably died and we lost our icon. Try to add the icon, and try again.
        PhAddNotifyIcon(Id);
        Shell_NotifyIcon(NIM_MODIFY, &notifyIcon);
    }
}

VOID PhShowBalloonTipNotifyIcon(
    __in ULONG Id,
    __in PWSTR Title,
    __in PWSTR Text,
    __in ULONG Timeout,
    __in ULONG Flags
    )
{
    NOTIFYICONDATA notifyIcon = { NOTIFYICONDATA_V3_SIZE };

    notifyIcon.hWnd = PhMainWndHandle;
    notifyIcon.uID = Id;
    notifyIcon.uFlags = NIF_INFO;
    wcsncpy_s(notifyIcon.szInfoTitle, sizeof(notifyIcon.szInfoTitle) / sizeof(WCHAR), Title, _TRUNCATE);
    wcsncpy_s(notifyIcon.szInfo, sizeof(notifyIcon.szInfo) / sizeof(WCHAR), Text, _TRUNCATE);
    notifyIcon.uTimeout = Timeout;
    notifyIcon.dwInfoFlags = Flags;

    Shell_NotifyIcon(NIM_MODIFY, &notifyIcon);
}

HICON PhBitmapToIcon(
    __in HBITMAP Bitmap
    )
{
    ICONINFO iconInfo;

    PhpGetBlackIcon();

    iconInfo.fIcon = TRUE;
    iconInfo.xHotspot = 0;
    iconInfo.yHotspot = 0;
    iconInfo.hbmMask = BlackIconMask;
    iconInfo.hbmColor = Bitmap;

    return CreateIconIndirect(&iconInfo);
}

static VOID PhpBeginBitmap(
    __in ULONG Width,
    __in ULONG Height,
    __out HBITMAP *Bitmap,
    __out HDC *Hdc,
    __out HBITMAP *OldBitmap
    )
{
    static BOOLEAN initialized = FALSE;
    static HDC hdc;
    static HBITMAP bitmap;

    if (!initialized)
    {
        HDC screenHdc;

        screenHdc = GetDC(NULL);
        hdc = CreateCompatibleDC(screenHdc);
        bitmap = CreateCompatibleBitmap(screenHdc, Width, Height);
        ReleaseDC(NULL, screenHdc);

        initialized = TRUE;
    }

    *Bitmap = bitmap;
    *Hdc = hdc;
    *OldBitmap = SelectObject(hdc, bitmap);
}

VOID PhUpdateIconCpuHistory()
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
    ULONG lineDataCount;
    FLOAT lineData1[9];
    FLOAT lineData2[9];
    HBITMAP bitmap;
    HDC hdc;
    HBITMAP oldBitmap;
    HICON icon;
    HANDLE maxCpuProcessId;
    PPH_PROCESS_ITEM maxCpuProcessItem;
    PH_FORMAT format[8];
    PPH_STRING text;

    // Icon

    lineDataCount = min(9, PhCpuKernelHistory.Count);
    PhCopyCircularBuffer_FLOAT(&PhCpuKernelHistory, lineData1, lineDataCount);
    PhCopyCircularBuffer_FLOAT(&PhCpuUserHistory, lineData2, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineData2 = lineData2;
    drawInfo.LineColor1 = PhCsColorCpuKernel;
    drawInfo.LineColor2 = PhCsColorCpuUser;
    drawInfo.LineBackColor1 = PhHalveColorBrightness(PhCsColorCpuKernel);
    drawInfo.LineBackColor2 = PhHalveColorBrightness(PhCsColorCpuUser);

    PhpBeginBitmap(16, 16, &bitmap, &hdc, &oldBitmap);
    PhDrawGraph(hdc, &drawInfo);

    SelectObject(hdc, oldBitmap);
    icon = PhBitmapToIcon(bitmap);

    // Text

    maxCpuProcessId = (HANDLE)PhGetItemCircularBuffer_ULONG(&PhMaxCpuHistory, 0);

    if (maxCpuProcessId)
        maxCpuProcessItem = PhReferenceProcessItem(maxCpuProcessId);
    else
        maxCpuProcessItem = NULL;

    format[0].Type = StringFormatType;
    PhInitializeStringRef(&format[0].u.String, L"CPU usage: ");
    format[1].Type = DoubleFormatType | FormatUsePrecision;
    format[1].Precision = 2;
    format[1].u.Double = (PhCpuKernelUsage + PhCpuUserUsage) * 100;
    format[2].Type = CharFormatType;
    format[2].u.Char = '%';

    if (maxCpuProcessItem)
    {
        format[3].Type = CharFormatType;
        format[3].u.Char = '\n';
        format[4].Type = StringFormatType;
        format[4].u.String = maxCpuProcessItem->ProcessName->sr;
        format[5].Type = StringFormatType;
        PhInitializeStringRef(&format[5].u.String, L": ");
        format[6].Type = DoubleFormatType | FormatUsePrecision;
        format[6].Precision = 2;
        format[6].u.Double = maxCpuProcessItem->CpuUsage * 100;
        format[7].Type = CharFormatType;
        format[7].u.Char = '%';
    }

    text = PhFormat(format, maxCpuProcessItem ? 8 : 3, 128);
    if (maxCpuProcessItem) PhDereferenceObject(maxCpuProcessItem);

    PhModifyNotifyIcon(PH_ICON_CPU_HISTORY, NIF_TIP | NIF_ICON, text->Buffer, icon);

    DestroyIcon(icon);
    PhDereferenceObject(text);
}

VOID PhUpdateIconIoHistory()
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
    ULONG lineDataCount;
    FLOAT lineData1[9];
    FLOAT lineData2[9];
    FLOAT max;
    ULONG i;
    HBITMAP bitmap;
    HDC hdc;
    HBITMAP oldBitmap;
    HICON icon;
    HANDLE maxIoProcessId;
    PPH_PROCESS_ITEM maxIoProcessItem;
    PH_FORMAT format[8];
    PPH_STRING text;

    // Icon

    lineDataCount = min(9, PhIoReadHistory.Count);
    max = 1024 * 1024; // minimum scaling of 1 MB.

    for (i = 0; i < lineDataCount; i++)
    {
        lineData1[i] =
            (FLOAT)PhGetItemCircularBuffer_ULONG64(&PhIoReadHistory, i) +
            (FLOAT)PhGetItemCircularBuffer_ULONG64(&PhIoOtherHistory, i);
        lineData2[i] =
            (FLOAT)PhGetItemCircularBuffer_ULONG64(&PhIoWriteHistory, i);

        if (max < lineData1[i] + lineData2[i])
            max = lineData1[i] + lineData2[i];
    }

    PhxfDivideSingle2U(lineData1, max, lineDataCount);
    PhxfDivideSingle2U(lineData2, max, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineData2 = lineData2;
    drawInfo.LineColor1 = PhCsColorIoReadOther;
    drawInfo.LineColor2 = PhCsColorIoWrite;
    drawInfo.LineBackColor1 = PhHalveColorBrightness(PhCsColorIoReadOther);
    drawInfo.LineBackColor2 = PhHalveColorBrightness(PhCsColorIoWrite);

    PhpBeginBitmap(16, 16, &bitmap, &hdc, &oldBitmap);
    PhDrawGraph(hdc, &drawInfo);

    SelectObject(hdc, oldBitmap);
    icon = PhBitmapToIcon(bitmap);

    // Text

    maxIoProcessId = (HANDLE)PhGetItemCircularBuffer_ULONG(&PhMaxIoHistory, 0);

    if (maxIoProcessId)
        maxIoProcessItem = PhReferenceProcessItem(maxIoProcessId);
    else
        maxIoProcessItem = NULL;

    format[0].Type = StringFormatType;
    PhInitializeStringRef(&format[0].u.String, L"R: ");
    format[1].Type = SizeFormatType;
    format[1].u.Size = PhIoReadDelta.Delta;
    format[2].Type = StringFormatType;
    PhInitializeStringRef(&format[2].u.String, L"\nW: ");
    format[3].Type = SizeFormatType;
    format[3].u.Size = PhIoWriteDelta.Delta;
    format[4].Type = StringFormatType;
    PhInitializeStringRef(&format[4].u.String, L"\nO: ");
    format[5].Type = SizeFormatType;
    format[5].u.Size = PhIoOtherDelta.Delta;

    if (maxIoProcessItem)
    {
        format[6].Type = CharFormatType;
        format[6].u.Char = '\n';
        format[7].Type = StringFormatType;
        format[7].u.String = maxIoProcessItem->ProcessName->sr;
    }

    text = PhFormat(format, maxIoProcessItem ? 8 : 6, 128);
    if (maxIoProcessItem) PhDereferenceObject(maxIoProcessItem);

    PhModifyNotifyIcon(PH_ICON_IO_HISTORY, NIF_TIP | NIF_ICON, text->Buffer, icon);

    DestroyIcon(icon);
    PhDereferenceObject(text);
}

VOID PhUpdateIconCommitHistory()
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
    ULONG lineDataCount;
    FLOAT lineData1[9];
    ULONG i;
    HBITMAP bitmap;
    HDC hdc;
    HBITMAP oldBitmap;
    HICON icon;
    DOUBLE commitFraction;
    PH_FORMAT format[5];
    PPH_STRING text;

    // Icon

    lineDataCount = min(9, PhCommitHistory.Count);

    for (i = 0; i < lineDataCount; i++)
        lineData1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&PhCommitHistory, i);

    PhxfDivideSingle2U(lineData1, (FLOAT)PhPerfInformation.CommitLimit, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineColor1 = PhCsColorPrivate;
    drawInfo.LineBackColor1 = PhHalveColorBrightness(PhCsColorPrivate);

    PhpBeginBitmap(16, 16, &bitmap, &hdc, &oldBitmap);
    PhDrawGraph(hdc, &drawInfo);

    SelectObject(hdc, oldBitmap);
    icon = PhBitmapToIcon(bitmap);

    // Text

    commitFraction = (DOUBLE)PhPerfInformation.CommittedPages / PhPerfInformation.CommitLimit;

    format[0].Type = StringFormatType;
    PhInitializeStringRef(&format[0].u.String, L"Commit: ");
    format[1].Type = SizeFormatType;
    format[1].u.Size = UInt32x32To64(PhPerfInformation.CommittedPages, PAGE_SIZE);
    format[2].Type = StringFormatType;
    PhInitializeStringRef(&format[2].u.String, L" (");
    format[3].Type = DoubleFormatType | FormatUsePrecision;
    format[3].Precision = 2;
    format[3].u.Double = commitFraction * 100;
    format[4].Type = StringFormatType;
    PhInitializeStringRef(&format[4].u.String, L"%)");

    text = PhFormat(format, sizeof(format) / sizeof(PH_FORMAT), 64);

    PhModifyNotifyIcon(PH_ICON_COMMIT_HISTORY, NIF_TIP | NIF_ICON, text->Buffer, icon);

    DestroyIcon(icon);
    PhDereferenceObject(text);
}

VOID PhUpdateIconPhysicalHistory()
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
    ULONG lineDataCount;
    FLOAT lineData1[9];
    ULONG i;
    HBITMAP bitmap;
    HDC hdc;
    HBITMAP oldBitmap;
    HICON icon;
    ULONG physicalUsage;
    FLOAT physicalFraction;
    PH_FORMAT format[5];
    PPH_STRING text;

    // Icon

    lineDataCount = min(9, PhCommitHistory.Count);

    for (i = 0; i < lineDataCount; i++)
        lineData1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&PhPhysicalHistory, i);

    PhxfDivideSingle2U(lineData1, (FLOAT)PhSystemBasicInformation.NumberOfPhysicalPages, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineColor1 = PhCsColorPhysical;
    drawInfo.LineBackColor1 = PhHalveColorBrightness(PhCsColorPhysical);

    PhpBeginBitmap(16, 16, &bitmap, &hdc, &oldBitmap);
    PhDrawGraph(hdc, &drawInfo);

    SelectObject(hdc, oldBitmap);
    icon = PhBitmapToIcon(bitmap);

    // Text

    physicalUsage = PhSystemBasicInformation.NumberOfPhysicalPages - PhPerfInformation.AvailablePages;
    physicalFraction = (FLOAT)physicalUsage / PhSystemBasicInformation.NumberOfPhysicalPages;

    format[0].Type = StringFormatType;
    PhInitializeStringRef(&format[0].u.String, L"Physical Memory: ");
    format[1].Type = SizeFormatType;
    format[1].u.Size = UInt32x32To64(physicalUsage, PAGE_SIZE);
    format[2].Type = StringFormatType;
    PhInitializeStringRef(&format[2].u.String, L" (");
    format[3].Type = DoubleFormatType | FormatUsePrecision;
    format[3].Precision = 2;
    format[3].u.Double = physicalFraction * 100;
    format[4].Type = StringFormatType;
    PhInitializeStringRef(&format[4].u.String, L"%)");

    text = PhFormat(format, sizeof(format) / sizeof(PH_FORMAT), 64);

    PhModifyNotifyIcon(PH_ICON_PHYSICAL_HISTORY, NIF_TIP | NIF_ICON, text->Buffer, icon);

    DestroyIcon(icon);
    PhDereferenceObject(text);
}

VOID PhUpdateIconCpuUsage()
{
    HBITMAP bitmap;
    HDC hdc;
    HBITMAP oldBitmap;
    HICON icon;
    HANDLE maxCpuProcessId;
    PPH_PROCESS_ITEM maxCpuProcessItem;
    PPH_STRING maxCpuText = NULL;
    PPH_STRING text;

    // Icon

    PhpBeginBitmap(16, 16, &bitmap, &hdc, &oldBitmap);

    // This stuff is copied from CpuUsageIcon.cs (PH 1.x).
    {
        COLORREF kColor = PhCsColorCpuKernel;
        COLORREF uColor = PhCsColorCpuUser;
        COLORREF kbColor = PhHalveColorBrightness(PhCsColorCpuKernel);
        COLORREF ubColor = PhHalveColorBrightness(PhCsColorCpuUser);
        FLOAT k = PhCpuKernelUsage;
        FLOAT u = PhCpuUserUsage;
        LONG kl = (LONG)(k * 16);
        LONG ul = (LONG)(u * 16);
        RECT rect;
        HBRUSH dcBrush;
        HBRUSH dcPen;
        POINT points[2];

        dcBrush = GetStockObject(DC_BRUSH);
        dcPen = GetStockObject(DC_PEN);
        rect.left = 0;
        rect.top = 0;
        rect.right = 16;
        rect.bottom = 16;
        SetDCBrushColor(hdc, RGB(0x00, 0x00, 0x00));
        FillRect(hdc, &rect, dcBrush);

        // Draw the base line.
        if (kl + ul == 0)
        {
            SelectObject(hdc, dcPen);
            SetDCPenColor(hdc, uColor);
            points[0].x = 0;
            points[0].y = 15;
            points[1].x = 16;
            points[1].y = 15;
            Polyline(hdc, points, 2);
        }
        else
        {
            rect.left = 0;
            rect.top = 16 - ul - kl;
            rect.right = 16;
            rect.bottom = 16 - kl;
            SetDCBrushColor(hdc, ubColor);
            FillRect(hdc, &rect, dcBrush);

            points[0].x = 0;
            points[0].y = 15 - ul - kl;
            if (points[0].y < 0) points[0].y = 0;
            points[1].x = 16;
            points[1].y = points[0].y;
            SelectObject(hdc, dcPen);
            SetDCPenColor(hdc, uColor);
            Polyline(hdc, points, 2);

            if (kl != 0)
            {
                rect.left = 0;
                rect.top = 16 - kl;
                rect.right = 16;
                rect.bottom = 16;
                SetDCBrushColor(hdc, kbColor);
                FillRect(hdc, &rect, dcBrush);

                points[0].x = 0;
                points[0].y = 15 - kl;
                if (points[0].y < 0) points[0].y = 0;
                points[1].x = 16;
                points[1].y = points[0].y;
                SelectObject(hdc, dcPen);
                SetDCPenColor(hdc, kColor);
                Polyline(hdc, points, 2);
            }
        }
    }

    SelectObject(hdc, oldBitmap);
    icon = PhBitmapToIcon(bitmap);

    // Text

    maxCpuProcessId = (HANDLE)PhGetItemCircularBuffer_ULONG(&PhMaxCpuHistory, 0);

    if (maxCpuProcessId)
    {
        if (maxCpuProcessItem = PhReferenceProcessItem(maxCpuProcessId))
        {
            maxCpuText = PhFormatString(
                L"\n%s: %.2f%%",
                maxCpuProcessItem->ProcessName->Buffer,
                maxCpuProcessItem->CpuUsage * 100
                );
            PhDereferenceObject(maxCpuProcessItem);
        }
    }

    text = PhFormatString(L"CPU usage: %.2f%%%s", (PhCpuKernelUsage + PhCpuUserUsage) * 100, PhGetStringOrEmpty(maxCpuText));
    if (maxCpuText) PhDereferenceObject(maxCpuText);

    PhModifyNotifyIcon(PH_ICON_CPU_USAGE, NIF_TIP | NIF_ICON, text->Buffer, icon);

    DestroyIcon(icon);
    PhDereferenceObject(text);
}
