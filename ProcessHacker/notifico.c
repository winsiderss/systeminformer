#include <phapp.h>
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

    wcscpy_s(notifyIcon.szTip, sizeof(notifyIcon.szTip) / sizeof(WCHAR), PhApplicationName);

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
        wcscpy_s(notifyIcon.szTip, sizeof(notifyIcon.szTip) / sizeof(WCHAR), Text);

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
    wcscpy_s(notifyIcon.szInfoTitle, sizeof(notifyIcon.szInfoTitle) / sizeof(WCHAR), Title);
    wcscpy_s(notifyIcon.szInfo, sizeof(notifyIcon.szInfo) / sizeof(WCHAR), Text);
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
    PPH_STRING maxCpuText = NULL;
    PPH_STRING text;

    // Icon

    lineDataCount = min(9, PhCpuKernelHistory.Count);
    PhCopyCircularBuffer_FLOAT(&PhCpuKernelHistory, lineData1, lineDataCount);
    PhCopyCircularBuffer_FLOAT(&PhCpuUserHistory, lineData2, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineData2 = lineData2;
    drawInfo.LineColor1 = RGB(0x00, 0xff, 0x00);
    drawInfo.LineColor2 = RGB(0xff, 0x00, 0x00);
    drawInfo.LineBackColor1 = RGB(0x00, 0x77, 0x00);
    drawInfo.LineBackColor2 = RGB(0x77, 0x00, 0x00);

    PhpBeginBitmap(16, 16, &bitmap, &hdc, &oldBitmap);
    PhDrawGraph(hdc, &drawInfo);

    SelectObject(hdc, oldBitmap);
    icon = PhBitmapToIcon(bitmap);

    // Text

    maxCpuProcessId = (HANDLE)PhCircularBufferGet_ULONG(&PhMaxCpuHistory, 0);

    if (maxCpuProcessId != NULL)
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
    PPH_STRING readString;
    PPH_STRING writeString;
    PPH_STRING otherString;
    PPH_STRING maxIoText = NULL;
    PPH_STRING text;

    // Icon

    lineDataCount = min(9, PhIoReadHistory.Count);
    max = 1024 * 1024; // minimum scaling of 1 MB.

    for (i = 0; i < lineDataCount; i++)
    {
        lineData1[i] =
            (FLOAT)PhCircularBufferGet_ULONG64(&PhIoReadHistory, i) +
            (FLOAT)PhCircularBufferGet_ULONG64(&PhIoOtherHistory, i);
        lineData2[i] =
            (FLOAT)PhCircularBufferGet_ULONG64(&PhIoWriteHistory, i);

        if (max < lineData1[i] + lineData2[i])
            max = lineData1[i] + lineData2[i];
    }

    PhxfDivideSingle2U(lineData1, max, lineDataCount);
    PhxfDivideSingle2U(lineData2, max, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineData2 = lineData2;
    drawInfo.LineColor1 = RGB(0xff, 0xff, 0x00);
    drawInfo.LineColor2 = RGB(0x77, 0x00, 0xff);
    drawInfo.LineBackColor1 = RGB(0x77, 0x77, 0x00);
    drawInfo.LineBackColor2 = RGB(0x33, 0x00, 0x77);

    PhpBeginBitmap(16, 16, &bitmap, &hdc, &oldBitmap);
    PhDrawGraph(hdc, &drawInfo);

    SelectObject(hdc, oldBitmap);
    icon = PhBitmapToIcon(bitmap);

    // Text

    maxIoProcessId = (HANDLE)PhCircularBufferGet_ULONG(&PhMaxIoHistory, 0);

    if (maxIoProcessId != NULL)
    {
        if (maxIoProcessItem = PhReferenceProcessItem(maxIoProcessId))
        {
            maxIoText = PhConcatStrings2(L"\n", maxIoProcessItem->ProcessName->Buffer);
            PhDereferenceObject(maxIoProcessItem);
        }
    }

    readString = PhFormatSize(PhIoReadDelta.Delta, -1);
    writeString = PhFormatSize(PhIoWriteDelta.Delta, -1);
    otherString = PhFormatSize(PhIoOtherDelta.Delta, -1);
    text = PhFormatString(
        L"R: %s\nW: %s\nO: %s%s",
        readString->Buffer,
        writeString->Buffer,
        otherString->Buffer,
        PhGetStringOrEmpty(maxIoText)
        );
    PhDereferenceObject(readString);
    PhDereferenceObject(writeString);
    PhDereferenceObject(otherString);
    if (maxIoText) PhDereferenceObject(maxIoText);

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
    PPH_STRING commitString;
    FLOAT commitFraction;
    PPH_STRING text;

    // Icon

    lineDataCount = min(9, PhCommitHistory.Count);

    for (i = 0; i < lineDataCount; i++)
        lineData1[i] = (FLOAT)PhCircularBufferGet_ULONG(&PhCommitHistory, i);

    PhxfDivideSingle2U(lineData1, (FLOAT)PhPerfInformation.CommitLimit, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineColor1 = RGB(0xff, 0x77, 0x00);
    drawInfo.LineBackColor1 = RGB(0x77, 0x33, 0x00);

    PhpBeginBitmap(16, 16, &bitmap, &hdc, &oldBitmap);
    PhDrawGraph(hdc, &drawInfo);

    SelectObject(hdc, oldBitmap);
    icon = PhBitmapToIcon(bitmap);

    // Text

    commitString = PhFormatSize(UInt32x32To64(PhPerfInformation.CommittedPages, PAGE_SIZE), -1);
    commitFraction = (FLOAT)PhPerfInformation.CommittedPages / PhPerfInformation.CommitLimit;
    text = PhFormatString(L"Commit: %s (%.2f%%)", commitString->Buffer, commitFraction * 100);
    PhDereferenceObject(commitString);

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
    PPH_STRING physicalString;
    FLOAT physicalFraction;
    PPH_STRING text;

    // Icon

    lineDataCount = min(9, PhCommitHistory.Count);

    for (i = 0; i < lineDataCount; i++)
        lineData1[i] = (FLOAT)PhCircularBufferGet_ULONG(&PhPhysicalHistory, i);

    PhxfDivideSingle2U(lineData1, (FLOAT)PhSystemBasicInformation.NumberOfPhysicalPages, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineColor1 = RGB(0x00, 0xff, 0xff);
    drawInfo.LineBackColor1 = RGB(0x00, 0x77, 0x77);

    PhpBeginBitmap(16, 16, &bitmap, &hdc, &oldBitmap);
    PhDrawGraph(hdc, &drawInfo);

    SelectObject(hdc, oldBitmap);
    icon = PhBitmapToIcon(bitmap);

    // Text

    physicalUsage = PhSystemBasicInformation.NumberOfPhysicalPages - PhPerfInformation.AvailablePages;
    physicalString = PhFormatSize(UInt32x32To64(physicalUsage, PAGE_SIZE), -1);
    physicalFraction = (FLOAT)physicalUsage / PhSystemBasicInformation.NumberOfPhysicalPages;
    text = PhFormatString(L"Physical Memory: %s (%.2f%%)", physicalString->Buffer, physicalFraction * 100);
    PhDereferenceObject(physicalString);

    PhModifyNotifyIcon(PH_ICON_PHYSICAL_HISTORY, NIF_TIP | NIF_ICON, text->Buffer, icon);

    DestroyIcon(icon);
    PhDereferenceObject(text);
}

VOID PhUpdateIconCpuUsage()
{

}
