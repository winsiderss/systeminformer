/*
 * Process Hacker - 
 *   notification icon manager
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

#include <phapp.h>
#include <settings.h>
#include <extmgri.h>
#include <phplug.h>
#include <notifico.h>
#include <windowsx.h>

HICON PhNfpGetBlackIcon(
    VOID
    );

BOOLEAN PhNfpAddNotifyIcon(
    __in ULONG Id
    );

BOOLEAN PhNfpRemoveNotifyIcon(
    __in ULONG Id
    );

BOOLEAN PhNfpModifyNotifyIcon(
    __in ULONG Id,
    __in ULONG Flags,
    __in_opt PWSTR Text,
    __in_opt HICON Icon
    );

VOID PhNfpProcessesUpdatedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    );

VOID PhNfpUpdateRegisteredIcon(
    __in PPH_NF_ICON Icon
    );

VOID PhNfpBeginBitmap(
    __out PULONG Width,
    __out PULONG Height,
    __out HBITMAP *Bitmap,
    __out_opt PVOID *Bits,
    __out HDC *Hdc,
    __out HBITMAP *OldBitmap
    );

VOID PhNfpUpdateIconCpuHistory(
    VOID
    );

VOID PhNfpUpdateIconIoHistory(
    VOID
    );

VOID PhNfpUpdateIconCommitHistory(
    VOID
    );

VOID PhNfpUpdateIconPhysicalHistory(
    VOID
    );

VOID PhNfpUpdateIconCpuUsage(
    VOID
    );

BOOLEAN PhNfTerminating = FALSE;
ULONG PhNfIconMask;
ULONG PhNfIconNotifyMask;
ULONG PhNfMaximumIconId = PH_ICON_DEFAULT_MAXIMUM;
PPH_NF_ICON PhNfRegisteredIcons[32] = { 0 };

PH_NF_POINTERS PhNfpPointers;
PH_CALLBACK_REGISTRATION PhNfpProcessesUpdatedRegistration;
HICON PhNfpBlackIcon = NULL;
HBITMAP PhNfpBlackIconMask = NULL;

VOID PhNfLoadStage1(
    VOID
    )
{
    PPH_STRING iconList;
    PH_STRINGREF part;
    PH_STRINGREF remainingPart;

    PhNfpPointers.UpdateRegisteredIcon = PhNfpUpdateRegisteredIcon;
    PhNfpPointers.BeginBitmap = PhNfpBeginBitmap;

    // Load settings for default icons.
    PhNfIconMask = PhGetIntegerSetting(L"IconMask");

    // Load settings for registered icons.

    iconList = PhGetStringSetting(L"IconMaskList");
    remainingPart = iconList->sr;

    while (remainingPart.Length != 0)
    {
        PhSplitStringRefAtChar(&remainingPart, '|', &part, &remainingPart);

        if (part.Length != 0)
        {
            PH_STRINGREF pluginName;
            ULONG subId;
            PPH_NF_ICON icon;

            if (PhEmParseCompoundId(&part, &pluginName, &subId) &&
                (icon = PhNfFindIcon(&pluginName, subId)))
            {
                PhNfIconMask |= icon->IconId;
            }
        }
    }

    PhDereferenceObject(iconList);
}

VOID PhNfLoadStage2(
    VOID
    )
{
    ULONG i;

    for (i = PH_ICON_MINIMUM; i != PhNfMaximumIconId; i <<= 1)
    {
        if (PhNfIconMask & i)
            PhNfpAddNotifyIcon(i);
    }

    PhRegisterCallback(
        &PhProcessesUpdatedEvent,
        PhNfpProcessesUpdatedHandler,
        NULL,
        &PhNfpProcessesUpdatedRegistration
        );
}

VOID PhNfSaveSettings(
    VOID
    )
{
    ULONG registeredIconMask;

    PhSetIntegerSetting(L"IconMask", PhNfIconMask & PH_ICON_DEFAULT_ALL);

    registeredIconMask = PhNfIconMask & ~PH_ICON_DEFAULT_ALL;

    if (registeredIconMask != 0)
    {
        PH_STRING_BUILDER iconListBuilder;
        ULONG i;

        PhInitializeStringBuilder(&iconListBuilder, 60);

        for (i = 0; i < sizeof(PhNfRegisteredIcons) / sizeof(PPH_NF_ICON); i++)
        {
            if (PhNfRegisteredIcons[i])
            {
                if (registeredIconMask & PhNfRegisteredIcons[i]->IconId)
                {
                    PhAppendFormatStringBuilder(
                        &iconListBuilder,
                        L"+%s+%u|",
                        PhNfRegisteredIcons[i]->Plugin->Name,
                        PhNfRegisteredIcons[i]->SubId
                        );
                }
            }
        }

        if (iconListBuilder.String->Length != 0)
            PhRemoveStringBuilder(&iconListBuilder, iconListBuilder.String->Length / 2 - 1, 1);

        PhSetStringSetting2(L"IconMaskList", &iconListBuilder.String->sr);
        PhDeleteStringBuilder(&iconListBuilder);
    }
}

VOID PhNfUninitialization(
    VOID
    )
{
    ULONG i;

    // Remove all icons to prevent them hanging around after we exit.

    PhNfTerminating = TRUE; // prevent further icon updating

    for (i = PH_ICON_MINIMUM; i != PhNfMaximumIconId; i <<= 1)
    {
        if (PhNfIconMask & i)
            PhNfpRemoveNotifyIcon(i);
    }
}

VOID PhNfForwardMessage(
    __in ULONG_PTR WParam,
    __in ULONG_PTR LParam
    )
{
    ULONG iconIndex;

    iconIndex = HIWORD(LParam);

    if (iconIndex < sizeof(PhNfRegisteredIcons) / sizeof(PPH_NF_ICON) && PhNfRegisteredIcons[iconIndex])
    {
        if (PhNfRegisteredIcons[iconIndex]->MessageCallback)
        {
            if (PhNfRegisteredIcons[iconIndex]->MessageCallback(
                PhNfRegisteredIcons[iconIndex],
                WParam,
                LParam,
                PhNfRegisteredIcons[iconIndex]->Context
                ))
            {
                return;
            }
        }
    }

    switch (LOWORD(LParam))
    {
    case WM_LBUTTONDOWN:
        {
            if (PhGetIntegerSetting(L"IconSingleClick"))
                ProcessHacker_ToggleVisible(PhMainWndHandle);
        }
        break;
    case WM_LBUTTONDBLCLK:
        {
            if (!PhGetIntegerSetting(L"IconSingleClick"))
                ProcessHacker_ToggleVisible(PhMainWndHandle);
        }
        break;
    case WM_RBUTTONUP:
        {
            POINT location;

            GetCursorPos(&location);
            PhShowIconContextMenu(location);
        }
        break;
    }
}

ULONG PhNfGetMaximumIconId(
    VOID
    )
{
    return PhNfMaximumIconId;
}

ULONG PhNfTestIconMask(
    __in ULONG Id
    )
{
    return PhNfIconMask & Id;
}

VOID PhNfSetVisibleIcon(
    __in ULONG Id,
    __in BOOLEAN Visible
    )
{
    if (Visible)
    {
        PhNfIconMask |= Id;
        PhNfpAddNotifyIcon(Id);
    }
    else
    {
        PhNfIconMask &= ~Id;
        PhNfpRemoveNotifyIcon(Id);
    }
}

BOOLEAN PhNfShowBalloonTip(
    __in_opt ULONG Id,
    __in PWSTR Title,
    __in PWSTR Text,
    __in ULONG Timeout,
    __in ULONG Flags
    )
{
    NOTIFYICONDATA notifyIcon = { NOTIFYICONDATA_V3_SIZE };

    if (Id == 0)
    {
        // Choose the first visible icon.
        Id = PhNfIconMask;
    }

    if (!_BitScanForward(&Id, Id))
        return FALSE;

    notifyIcon.hWnd = PhMainWndHandle;
    notifyIcon.uID = Id;
    notifyIcon.uFlags = NIF_INFO;
    wcsncpy_s(notifyIcon.szInfoTitle, sizeof(notifyIcon.szInfoTitle) / sizeof(WCHAR), Title, _TRUNCATE);
    wcsncpy_s(notifyIcon.szInfo, sizeof(notifyIcon.szInfo) / sizeof(WCHAR), Text, _TRUNCATE);
    notifyIcon.uTimeout = Timeout;
    notifyIcon.dwInfoFlags = Flags;

    Shell_NotifyIcon(NIM_MODIFY, &notifyIcon);

    return TRUE;
}

HICON PhNfBitmapToIcon(
    __in HBITMAP Bitmap
    )
{
    ICONINFO iconInfo;

    PhNfpGetBlackIcon();

    iconInfo.fIcon = TRUE;
    iconInfo.xHotspot = 0;
    iconInfo.yHotspot = 0;
    iconInfo.hbmMask = PhNfpBlackIconMask;
    iconInfo.hbmColor = Bitmap;

    return CreateIconIndirect(&iconInfo);
}

PPH_NF_ICON PhNfRegisterIcon(
    __in struct _PH_PLUGIN *Plugin,
    __in ULONG SubId,
    __in_opt PVOID Context,
    __in PWSTR Text,
    __reserved ULONG Flags,
    __in_opt PPH_NF_ICON_UPDATE_CALLBACK UpdateCallback,
    __in_opt PPH_NF_ICON_MESSAGE_CALLBACK MessageCallback
    )
{
    PPH_NF_ICON icon;
    ULONG iconId;
    ULONG iconIndex;

    if (PhNfMaximumIconId == PH_ICON_LIMIT)
    {
        // No room for any more icons.
        return NULL;
    }

    iconId = PhNfMaximumIconId;

    if (!_BitScanReverse(&iconIndex, iconId) ||
        iconIndex >= sizeof(PhNfRegisteredIcons) / sizeof(PPH_NF_ICON))
    {
        // Should never happen.
        return NULL;
    }

    PhNfMaximumIconId <<= 1;

    icon = PhAllocate(sizeof(PH_NF_ICON));
    icon->Plugin = Plugin;
    icon->SubId = SubId;
    icon->Context = Context;
    icon->Pointers = &PhNfpPointers;
    icon->Text = Text;
    icon->Flags = Flags;
    icon->IconId = iconId;
    icon->UpdateCallback = UpdateCallback;
    icon->MessageCallback = MessageCallback;

    PhNfRegisteredIcons[iconIndex] = icon;

    return icon;
}

PPH_NF_ICON PhNfGetIconById(
    __in ULONG Id
    )
{
    ULONG iconIndex;

    if (!_BitScanReverse(&iconIndex, Id))
        return NULL;

    return PhNfRegisteredIcons[iconIndex];
}

PPH_NF_ICON PhNfFindIcon(
    __in PPH_STRINGREF PluginName,
    __in ULONG SubId
    )
{
    ULONG i;

    for (i = 0; i < sizeof(PhNfRegisteredIcons) / sizeof(PPH_NF_ICON); i++)
    {
        if (PhNfRegisteredIcons[i])
        {
            if (PhNfRegisteredIcons[i]->SubId == SubId &&
                PhEqualStringRef(PluginName, &PhNfRegisteredIcons[i]->Plugin->AppContext.AppName, FALSE))
            {
                return PhNfRegisteredIcons[i];
            }
        }
    }

    return NULL;
}

HICON PhNfpGetBlackIcon(
    VOID
    )
{
    if (!PhNfpBlackIcon)
    {
        ICONINFO iconInfo;

        // It doesn't really matter what resolution the icon is at because when it's scaled up, it will
        // still be all black.
        PhNfpBlackIcon = (HICON)LoadImage(PhInstanceHandle, MAKEINTRESOURCE(IDI_BLACK), IMAGE_ICON, 16, 16, 0);

        GetIconInfo(PhNfpBlackIcon, &iconInfo);

        PhNfpBlackIconMask = iconInfo.hbmMask;
        DeleteObject(iconInfo.hbmColor);
    }

    return PhNfpBlackIcon;
}

BOOLEAN PhNfpAddNotifyIcon(
    __in ULONG Id
    )
{
    NOTIFYICONDATA notifyIcon = { NOTIFYICONDATA_V3_SIZE };
    PPH_NF_ICON icon;

    if ((icon = PhNfGetIconById(Id)) && (icon->Flags & PH_NF_ICON_UNAVAILABLE))
        return FALSE;

    // The IDs we pass to explorer are bit indicies, not the normal mask values.

    if (!_BitScanForward(&Id, Id))
        return FALSE;

    notifyIcon.hWnd = PhMainWndHandle;
    notifyIcon.uID = Id;
    notifyIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    notifyIcon.uCallbackMessage = WM_PH_NOTIFY_ICON_MESSAGE;

    wcsncpy_s(notifyIcon.szTip, sizeof(notifyIcon.szTip) / sizeof(WCHAR), PhApplicationName, _TRUNCATE);

    notifyIcon.hIcon = PhNfpGetBlackIcon();

    Shell_NotifyIcon(NIM_ADD, &notifyIcon);

    return TRUE;
}

BOOLEAN PhNfpRemoveNotifyIcon(
    __in ULONG Id
    )
{
    NOTIFYICONDATA notifyIcon = { NOTIFYICONDATA_V3_SIZE };
    PPH_NF_ICON icon;

    if ((icon = PhNfGetIconById(Id)) && (icon->Flags & PH_NF_ICON_UNAVAILABLE))
        return FALSE;

    if (!_BitScanForward(&Id, Id))
        return FALSE;

    notifyIcon.hWnd = PhMainWndHandle;
    notifyIcon.uID = Id;

    Shell_NotifyIcon(NIM_DELETE, &notifyIcon);

    return TRUE;
}

BOOLEAN PhNfpModifyNotifyIcon(
    __in ULONG Id,
    __in ULONG Flags,
    __in_opt PWSTR Text,
    __in_opt HICON Icon
    )
{
    NOTIFYICONDATA notifyIcon = { NOTIFYICONDATA_V3_SIZE };
    PPH_NF_ICON icon;

    if ((icon = PhNfGetIconById(Id)) && (icon->Flags & PH_NF_ICON_UNAVAILABLE))
        return FALSE;

    if (!_BitScanForward(&Id, Id))
        return FALSE;

    notifyIcon.hWnd = PhMainWndHandle;
    notifyIcon.uID = Id;
    notifyIcon.uFlags = Flags;

    if (Text)
        wcsncpy_s(notifyIcon.szTip, sizeof(notifyIcon.szTip) / sizeof(WCHAR), Text, _TRUNCATE);

    notifyIcon.hIcon = Icon;

    if (!Shell_NotifyIcon(NIM_MODIFY, &notifyIcon))
    {
        // Explorer probably died and we lost our icon. Try to add the icon, and try again.
        PhNfpAddNotifyIcon(Id);
        Shell_NotifyIcon(NIM_MODIFY, &notifyIcon);
    }

    return TRUE;
}

VOID PhNfpProcessesUpdatedHandler(
    __in_opt PVOID Parameter,
    __in_opt PVOID Context
    )
{
    ULONG registeredIconMask;

    // We do icon updating on the provider thread so we don't block the main GUI when 
    // explorer is not responding.

    if (PhNfIconMask & PH_ICON_CPU_HISTORY)
        PhNfpUpdateIconCpuHistory();
    if (PhNfIconMask & PH_ICON_IO_HISTORY)
        PhNfpUpdateIconIoHistory();
    if (PhNfIconMask & PH_ICON_COMMIT_HISTORY)
        PhNfpUpdateIconCommitHistory();
    if (PhNfIconMask & PH_ICON_PHYSICAL_HISTORY)
        PhNfpUpdateIconPhysicalHistory();
    if (PhNfIconMask & PH_ICON_CPU_USAGE)
        PhNfpUpdateIconCpuUsage();

    registeredIconMask = PhNfIconMask & ~PH_ICON_DEFAULT_ALL;

    if (registeredIconMask != 0)
    {
        ULONG i;

        for (i = 0; i < sizeof(PhNfRegisteredIcons) / sizeof(PPH_NF_ICON); i++)
        {
            if (PhNfRegisteredIcons[i] && (registeredIconMask & PhNfRegisteredIcons[i]->IconId))
            {
                PhNfpUpdateRegisteredIcon(PhNfRegisteredIcons[i]);
            }
        }
    }
}

VOID PhNfpUpdateRegisteredIcon(
    __in PPH_NF_ICON Icon
    )
{
    PVOID newIconOrBitmap;
    ULONG updateFlags;
    PPH_STRING newText;
    HICON newIcon;
    ULONG flags;

    if (!Icon->UpdateCallback)
        return;

    newIconOrBitmap = NULL;
    newText = NULL;
    newIcon = NULL;
    flags = 0;

    Icon->UpdateCallback(
        Icon,
        &newIconOrBitmap,
        &updateFlags,
        &newText,
        Icon->Context
        );

    if (newIconOrBitmap)
    {
        if (updateFlags & PH_NF_UPDATE_IS_BITMAP)
            newIcon = PhNfBitmapToIcon(newIconOrBitmap);
        else
            newIcon = newIconOrBitmap;

        flags |= NIF_ICON;
    }

    if (newText)
        flags |= NIF_TIP;

    if (flags != 0)
        PhNfpModifyNotifyIcon(Icon->IconId, flags, PhGetString(newText), newIcon);

    if (newIcon && (updateFlags & PH_NF_UPDATE_IS_BITMAP))
        DestroyIcon(newIcon);

    if (newIconOrBitmap && (updateFlags & PH_NF_UPDATE_DESTROY_RESOURCE))
    {
        if (updateFlags & PH_NF_UPDATE_IS_BITMAP)
            DeleteObject(newIconOrBitmap);
        else
            DestroyIcon(newIconOrBitmap);
    }

    if (newText)
        PhDereferenceObject(newText);
}

VOID PhNfpBeginBitmap(
    __out PULONG Width,
    __out PULONG Height,
    __out HBITMAP *Bitmap,
    __out_opt PVOID *Bits,
    __out HDC *Hdc,
    __out HBITMAP *OldBitmap
    )
{
    static BOOLEAN initialized = FALSE;
    static ULONG iconWidth;
    static ULONG iconHeight;
    static HDC hdc;
    static BITMAPINFOHEADER header;
    static HBITMAP bitmap;
    static PVOID bits;

    if (!initialized)
    {
        HDC screenHdc;

        screenHdc = GetDC(NULL);
        hdc = CreateCompatibleDC(screenHdc);

        memset(&header, 0, sizeof(BITMAPINFOHEADER));
        header.biSize = sizeof(BITMAPINFOHEADER);
        header.biWidth = PhSmallIconSize.X;
        header.biHeight = PhSmallIconSize.Y;
        header.biPlanes = 1;
        header.biBitCount = 32;
        bitmap = CreateDIBSection(screenHdc, (BITMAPINFO *)&header, DIB_RGB_COLORS, &bits, NULL, 0);

        ReleaseDC(NULL, screenHdc);

        initialized = TRUE;
    }

    *Width = PhSmallIconSize.X;
    *Height = PhSmallIconSize.Y;
    *Bitmap = bitmap;
    if (Bits) *Bits = bits;
    *Hdc = hdc;
    *OldBitmap = SelectObject(hdc, bitmap);
}

VOID PhNfpUpdateIconCpuHistory(
    VOID
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
    HANDLE maxCpuProcessId;
    PPH_PROCESS_ITEM maxCpuProcessItem;
    PH_FORMAT format[8];
    PPH_STRING text;

    // Icon

    PhNfpBeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);
    maxDataCount = drawInfo.Width / 2 + 1;
    lineData1 = _alloca(maxDataCount * sizeof(FLOAT));
    lineData2 = _alloca(maxDataCount * sizeof(FLOAT));

    lineDataCount = min(maxDataCount, PhCpuKernelHistory.Count);
    PhCopyCircularBuffer_FLOAT(&PhCpuKernelHistory, lineData1, lineDataCount);
    PhCopyCircularBuffer_FLOAT(&PhCpuUserHistory, lineData2, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineData2 = lineData2;
    drawInfo.LineColor1 = PhCsColorCpuKernel;
    drawInfo.LineColor2 = PhCsColorCpuUser;
    drawInfo.LineBackColor1 = PhHalveColorBrightness(PhCsColorCpuKernel);
    drawInfo.LineBackColor2 = PhHalveColorBrightness(PhCsColorCpuUser);

    if (bits)
        PhDrawGraphDirect(hdc, bits, &drawInfo);

    SelectObject(hdc, oldBitmap);
    icon = PhNfBitmapToIcon(bitmap);

    // Text

    if (PhMaxCpuHistory.Count != 0)
        maxCpuProcessId = (HANDLE)PhGetItemCircularBuffer_ULONG(&PhMaxCpuHistory, 0);
    else
        maxCpuProcessId = NULL;

    if (maxCpuProcessId)
        maxCpuProcessItem = PhReferenceProcessItem(maxCpuProcessId);
    else
        maxCpuProcessItem = NULL;

    PhInitFormatS(&format[0], L"CPU Usage: ");
    PhInitFormatF(&format[1], (PhCpuKernelUsage + PhCpuUserUsage) * 100, 2);
    PhInitFormatC(&format[2], '%');

    if (maxCpuProcessItem)
    {
        PhInitFormatC(&format[3], '\n');
        PhInitFormatSR(&format[4], maxCpuProcessItem->ProcessName->sr);
        PhInitFormatS(&format[5], L": ");
        PhInitFormatF(&format[6], maxCpuProcessItem->CpuUsage * 100, 2);
        PhInitFormatC(&format[7], '%');
    }

    text = PhFormat(format, maxCpuProcessItem ? 8 : 3, 128);
    if (maxCpuProcessItem) PhDereferenceObject(maxCpuProcessItem);

    PhNfpModifyNotifyIcon(PH_ICON_CPU_HISTORY, NIF_TIP | NIF_ICON, text->Buffer, icon);

    DestroyIcon(icon);
    PhDereferenceObject(text);
}

VOID PhNfpUpdateIconIoHistory(
    VOID
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
    HANDLE maxIoProcessId;
    PPH_PROCESS_ITEM maxIoProcessItem;
    PH_FORMAT format[8];
    PPH_STRING text;

    // Icon

    PhNfpBeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);
    maxDataCount = drawInfo.Width / 2 + 1;
    lineData1 = _alloca(maxDataCount * sizeof(FLOAT));
    lineData2 = _alloca(maxDataCount * sizeof(FLOAT));

    lineDataCount = min(maxDataCount, PhIoReadHistory.Count);
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

    if (bits)
        PhDrawGraphDirect(hdc, bits, &drawInfo);

    SelectObject(hdc, oldBitmap);
    icon = PhNfBitmapToIcon(bitmap);

    // Text

    if (PhMaxIoHistory.Count != 0)
        maxIoProcessId = (HANDLE)PhGetItemCircularBuffer_ULONG(&PhMaxIoHistory, 0);
    else
        maxIoProcessId = NULL;

    if (maxIoProcessId)
        maxIoProcessItem = PhReferenceProcessItem(maxIoProcessId);
    else
        maxIoProcessItem = NULL;

    PhInitFormatS(&format[0], L"I/O\nR: ");
    PhInitFormatSize(&format[1], PhIoReadDelta.Delta);
    PhInitFormatS(&format[2], L"\nW: ");
    PhInitFormatSize(&format[3], PhIoWriteDelta.Delta);
    PhInitFormatS(&format[4], L"\nO: ");
    PhInitFormatSize(&format[5], PhIoOtherDelta.Delta);

    if (maxIoProcessItem)
    {
        PhInitFormatC(&format[6], '\n');
        PhInitFormatSR(&format[7], maxIoProcessItem->ProcessName->sr);
    }

    text = PhFormat(format, maxIoProcessItem ? 8 : 6, 128);
    if (maxIoProcessItem) PhDereferenceObject(maxIoProcessItem);

    PhNfpModifyNotifyIcon(PH_ICON_IO_HISTORY, NIF_TIP | NIF_ICON, text->Buffer, icon);

    DestroyIcon(icon);
    PhDereferenceObject(text);
}

VOID PhNfpUpdateIconCommitHistory(
    VOID
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
    DOUBLE commitFraction;
    PH_FORMAT format[5];
    PPH_STRING text;

    // Icon

    PhNfpBeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);
    maxDataCount = drawInfo.Width / 2 + 1;
    lineData1 = _alloca(maxDataCount * sizeof(FLOAT));

    lineDataCount = min(maxDataCount, PhCommitHistory.Count);

    for (i = 0; i < lineDataCount; i++)
        lineData1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&PhCommitHistory, i);

    PhxfDivideSingle2U(lineData1, (FLOAT)PhPerfInformation.CommitLimit, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineColor1 = PhCsColorPrivate;
    drawInfo.LineBackColor1 = PhHalveColorBrightness(PhCsColorPrivate);

    if (bits)
        PhDrawGraphDirect(hdc, bits, &drawInfo);

    SelectObject(hdc, oldBitmap);
    icon = PhNfBitmapToIcon(bitmap);

    // Text

    commitFraction = (DOUBLE)PhPerfInformation.CommittedPages / PhPerfInformation.CommitLimit;

    PhInitFormatS(&format[0], L"Commit: ");
    PhInitFormatSize(&format[1], UInt32x32To64(PhPerfInformation.CommittedPages, PAGE_SIZE));
    PhInitFormatS(&format[2], L" (");
    PhInitFormatF(&format[3], commitFraction * 100, 2);
    PhInitFormatS(&format[4], L"%)");

    text = PhFormat(format, 5, 96);

    PhNfpModifyNotifyIcon(PH_ICON_COMMIT_HISTORY, NIF_TIP | NIF_ICON, text->Buffer, icon);

    DestroyIcon(icon);
    PhDereferenceObject(text);
}

VOID PhNfpUpdateIconPhysicalHistory(
    VOID
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
    ULONG physicalUsage;
    FLOAT physicalFraction;
    PH_FORMAT format[5];
    PPH_STRING text;

    // Icon

    PhNfpBeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);
    maxDataCount = drawInfo.Width / 2 + 1;
    lineData1 = _alloca(maxDataCount * sizeof(FLOAT));

    lineDataCount = min(maxDataCount, PhCommitHistory.Count);

    for (i = 0; i < lineDataCount; i++)
        lineData1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&PhPhysicalHistory, i);

    PhxfDivideSingle2U(lineData1, (FLOAT)PhSystemBasicInformation.NumberOfPhysicalPages, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineColor1 = PhCsColorPhysical;
    drawInfo.LineBackColor1 = PhHalveColorBrightness(PhCsColorPhysical);

    if (bits)
        PhDrawGraphDirect(hdc, bits, &drawInfo);

    SelectObject(hdc, oldBitmap);
    icon = PhNfBitmapToIcon(bitmap);

    // Text

    physicalUsage = PhSystemBasicInformation.NumberOfPhysicalPages - PhPerfInformation.AvailablePages;
    physicalFraction = (FLOAT)physicalUsage / PhSystemBasicInformation.NumberOfPhysicalPages;

    PhInitFormatS(&format[0], L"Physical Memory: ");
    PhInitFormatSize(&format[1], UInt32x32To64(physicalUsage, PAGE_SIZE));
    PhInitFormatS(&format[2], L" (");
    PhInitFormatF(&format[3], physicalFraction * 100, 2);
    PhInitFormatS(&format[4], L"%)");

    text = PhFormat(format, 5, 96);

    PhNfpModifyNotifyIcon(PH_ICON_PHYSICAL_HISTORY, NIF_TIP | NIF_ICON, text->Buffer, icon);

    DestroyIcon(icon);
    PhDereferenceObject(text);
}

VOID PhNfpUpdateIconCpuUsage(
    VOID
    )
{
    ULONG width;
    ULONG height;
    HBITMAP bitmap;
    HDC hdc;
    HBITMAP oldBitmap;
    HICON icon;
    HANDLE maxCpuProcessId;
    PPH_PROCESS_ITEM maxCpuProcessItem;
    PPH_STRING maxCpuText = NULL;
    PPH_STRING text;

    // Icon

    PhNfpBeginBitmap(&width, &height, &bitmap, NULL, &hdc, &oldBitmap);

    // This stuff is copied from CpuUsageIcon.cs (PH 1.x).
    {
        COLORREF kColor = PhCsColorCpuKernel;
        COLORREF uColor = PhCsColorCpuUser;
        COLORREF kbColor = PhHalveColorBrightness(PhCsColorCpuKernel);
        COLORREF ubColor = PhHalveColorBrightness(PhCsColorCpuUser);
        FLOAT k = PhCpuKernelUsage;
        FLOAT u = PhCpuUserUsage;
        LONG kl = (LONG)(k * height);
        LONG ul = (LONG)(u * height);
        RECT rect;
        HBRUSH dcBrush;
        HBRUSH dcPen;
        POINT points[2];

        dcBrush = GetStockObject(DC_BRUSH);
        dcPen = GetStockObject(DC_PEN);
        rect.left = 0;
        rect.top = 0;
        rect.right = width;
        rect.bottom = height;
        SetDCBrushColor(hdc, RGB(0x00, 0x00, 0x00));
        FillRect(hdc, &rect, dcBrush);

        // Draw the base line.
        if (kl + ul == 0)
        {
            SelectObject(hdc, dcPen);
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
            SetDCBrushColor(hdc, ubColor);
            FillRect(hdc, &rect, dcBrush);

            points[0].x = 0;
            points[0].y = height - 1 - ul - kl;
            if (points[0].y < 0) points[0].y = 0;
            points[1].x = width;
            points[1].y = points[0].y;
            SelectObject(hdc, dcPen);
            SetDCPenColor(hdc, uColor);
            Polyline(hdc, points, 2);

            if (kl != 0)
            {
                rect.left = 0;
                rect.top = height - kl;
                rect.right = width;
                rect.bottom = height;
                SetDCBrushColor(hdc, kbColor);
                FillRect(hdc, &rect, dcBrush);

                points[0].x = 0;
                points[0].y = height - 1 - kl;
                if (points[0].y < 0) points[0].y = 0;
                points[1].x = width;
                points[1].y = points[0].y;
                SelectObject(hdc, dcPen);
                SetDCPenColor(hdc, kColor);
                Polyline(hdc, points, 2);
            }
        }
    }

    SelectObject(hdc, oldBitmap);
    icon = PhNfBitmapToIcon(bitmap);

    // Text

    if (PhMaxCpuHistory.Count != 0)
        maxCpuProcessId = (HANDLE)PhGetItemCircularBuffer_ULONG(&PhMaxCpuHistory, 0);
    else
        maxCpuProcessId = NULL;

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

    text = PhFormatString(L"CPU Usage: %.2f%%%s", (PhCpuKernelUsage + PhCpuUserUsage) * 100, PhGetStringOrEmpty(maxCpuText));
    if (maxCpuText) PhDereferenceObject(maxCpuText);

    PhNfpModifyNotifyIcon(PH_ICON_CPU_USAGE, NIF_TIP | NIF_ICON, text->Buffer, icon);

    DestroyIcon(icon);
    PhDereferenceObject(text);
}
