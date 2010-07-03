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
    HDC screenHdc;
    HDC hdc;
    HBITMAP bitmap;

    screenHdc = GetDC(NULL);
    hdc = CreateCompatibleDC(screenHdc);
    bitmap = CreateCompatibleBitmap(screenHdc, Width, Height);
    ReleaseDC(NULL, screenHdc);

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
    DeleteDC(hdc);

    icon = PhBitmapToIcon(bitmap);
    DeleteObject(bitmap);

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

    PhModifyNotifyIcon(PH_ICON_CPU_HISTORY, NIF_TIP | NIF_ICON, text->Buffer, icon);

    DestroyIcon(icon);
    PhDereferenceObject(text);
    if (maxCpuText) PhDereferenceObject(maxCpuText);
}

VOID PhUpdateIconIoHistory()
{

}

VOID PhUpdateIconCommitHistory()
{

}

VOID PhUpdateIconPhysicalHistory()
{

}

VOID PhUpdateIconCpuUsage()
{

}
