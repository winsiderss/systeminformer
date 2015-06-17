/*
 * Process Hacker Extended Tools -
 *   notification icon extensions
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

#include "exttools.h"

#define GPU_ICON_ID 1
#define DISK_ICON_ID 2
#define NETWORK_ICON_ID 3

VOID EtpGpuIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

BOOLEAN EtpGpuIconMessageCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam,
    _In_opt_ PVOID Context
    );

VOID EtpDiskIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

BOOLEAN EtpDiskIconMessageCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam,
    _In_opt_ PVOID Context
    );

VOID EtpNetworkIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

BOOLEAN EtpNetworkIconMessageCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam,
    _In_opt_ PVOID Context
    );

VOID EtRegisterNotifyIcons(
    VOID
    )
{
    PH_NF_ICON_REGISTRATION_DATA data;

    data.MessageCallback = NULL;

    data.UpdateCallback = EtpGpuIconUpdateCallback;
    data.MessageCallback = EtpGpuIconMessageCallback;
    PhPluginRegisterIcon(
        PluginInstance,
        GPU_ICON_ID,
        NULL,
        L"GPU History",
        PH_NF_ICON_SHOW_MINIINFO | (EtGpuEnabled ? 0 : PH_NF_ICON_UNAVAILABLE),
        &data
        );

    data.UpdateCallback = EtpDiskIconUpdateCallback;
    data.MessageCallback = EtpDiskIconMessageCallback;
    PhPluginRegisterIcon(
        PluginInstance,
        DISK_ICON_ID,
        NULL,
        L"Disk History",
        PH_NF_ICON_SHOW_MINIINFO | (EtEtwEnabled ? 0 : PH_NF_ICON_UNAVAILABLE),
        &data
        );

    data.UpdateCallback = EtpNetworkIconUpdateCallback;
    data.MessageCallback = EtpNetworkIconMessageCallback;
    PhPluginRegisterIcon(
        PluginInstance,
        NETWORK_ICON_ID,
        NULL,
        L"Network History",
        PH_NF_ICON_SHOW_MINIINFO | (EtEtwEnabled ? 0 : PH_NF_ICON_UNAVAILABLE),
        &data
        );
}

VOID EtpGpuIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
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
    HBITMAP bitmap;
    PVOID bits;
    HDC hdc;
    HBITMAP oldBitmap;
    HANDLE maxGpuProcessId;
    PPH_PROCESS_ITEM maxGpuProcessItem;
    PH_FORMAT format[8];

    // Icon

    Icon->Pointers->BeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);
    maxDataCount = drawInfo.Width / 2 + 1;
    lineData1 = _alloca(maxDataCount * sizeof(FLOAT));

    lineDataCount = min(maxDataCount, EtGpuNodeHistory.Count);
    PhCopyCircularBuffer_FLOAT(&EtGpuNodeHistory, lineData1, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineColor1 = PhGetIntegerSetting(L"ColorCpuKernel");
    drawInfo.LineBackColor1 = PhHalveColorBrightness(drawInfo.LineColor1);

    if (bits)
        PhDrawGraphDirect(hdc, bits, &drawInfo);

    SelectObject(hdc, oldBitmap);
    *NewIconOrBitmap = bitmap;
    *Flags = PH_NF_UPDATE_IS_BITMAP;

    // Text

    if (EtMaxGpuNodeHistory.Count != 0)
        maxGpuProcessId = UlongToHandle(PhGetItemCircularBuffer_ULONG(&EtMaxGpuNodeHistory, 0));
    else
        maxGpuProcessId = NULL;

    if (maxGpuProcessId)
        maxGpuProcessItem = PhReferenceProcessItem(maxGpuProcessId);
    else
        maxGpuProcessItem = NULL;

    PhInitFormatS(&format[0], L"GPU Usage: ");
    PhInitFormatF(&format[1], EtGpuNodeUsage * 100, 2);
    PhInitFormatC(&format[2], '%');

    if (maxGpuProcessItem)
    {
        PhInitFormatC(&format[3], '\n');
        PhInitFormatSR(&format[4], maxGpuProcessItem->ProcessName->sr);
        PhInitFormatS(&format[5], L": ");
        PhInitFormatF(&format[6], EtGetProcessBlock(maxGpuProcessItem)->GpuNodeUsage * 100, 2);
        PhInitFormatC(&format[7], '%');
    }

    *NewText = PhFormat(format, maxGpuProcessItem ? 8 : 3, 128);
    if (maxGpuProcessItem) PhDereferenceObject(maxGpuProcessItem);
}

BOOLEAN EtpGpuIconMessageCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam,
    _In_opt_ PVOID Context
    )
{
    switch (LOWORD(LParam))
    {
    case PH_NF_MSG_SHOWMINIINFOSECTION:
        {
            PPH_NF_MSG_SHOWMINIINFOSECTION_DATA data = (PVOID)WParam;

            data->SectionName = L"GPU";
        }
        return TRUE;
    }

    return FALSE;
}

VOID EtpDiskIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
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
    HANDLE maxDiskProcessId;
    PPH_PROCESS_ITEM maxDiskProcessItem;
    PH_FORMAT format[6];

    // Icon

    Icon->Pointers->BeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);
    maxDataCount = drawInfo.Width / 2 + 1;
    lineData1 = _alloca(maxDataCount * sizeof(FLOAT));
    lineData2 = _alloca(maxDataCount * sizeof(FLOAT));

    lineDataCount = min(maxDataCount, EtDiskReadHistory.Count);
    max = 1024 * 1024; // minimum scaling of 1 MB.

    for (i = 0; i < lineDataCount; i++)
    {
        lineData1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&EtDiskReadHistory, i);
        lineData2[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&EtDiskWriteHistory, i);

        if (max < lineData1[i] + lineData2[i])
            max = lineData1[i] + lineData2[i];
    }

    PhDivideSinglesBySingle(lineData1, max, lineDataCount);
    PhDivideSinglesBySingle(lineData2, max, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineData2 = lineData2;
    drawInfo.LineColor1 = PhGetIntegerSetting(L"ColorIoReadOther");
    drawInfo.LineColor2 = PhGetIntegerSetting(L"ColorIoWrite");
    drawInfo.LineBackColor1 = PhHalveColorBrightness(drawInfo.LineColor1);
    drawInfo.LineBackColor2 = PhHalveColorBrightness(drawInfo.LineColor2);

    if (bits)
        PhDrawGraphDirect(hdc, bits, &drawInfo);

    SelectObject(hdc, oldBitmap);
    *NewIconOrBitmap = bitmap;
    *Flags = PH_NF_UPDATE_IS_BITMAP;

    // Text

    if (EtMaxDiskHistory.Count != 0)
        maxDiskProcessId = UlongToHandle(PhGetItemCircularBuffer_ULONG(&EtMaxDiskHistory, 0));
    else
        maxDiskProcessId = NULL;

    if (maxDiskProcessId)
        maxDiskProcessItem = PhReferenceProcessItem(maxDiskProcessId);
    else
        maxDiskProcessItem = NULL;

    PhInitFormatS(&format[0], L"Disk\nR: ");
    PhInitFormatSize(&format[1], EtDiskReadDelta.Delta);
    PhInitFormatS(&format[2], L"\nW: ");
    PhInitFormatSize(&format[3], EtDiskWriteDelta.Delta);

    if (maxDiskProcessItem)
    {
        PhInitFormatC(&format[4], '\n');
        PhInitFormatSR(&format[5], maxDiskProcessItem->ProcessName->sr);
    }

    *NewText = PhFormat(format, maxDiskProcessItem ? 6 : 4, 128);
    if (maxDiskProcessItem) PhDereferenceObject(maxDiskProcessItem);
}

BOOLEAN EtpDiskIconMessageCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam,
    _In_opt_ PVOID Context
    )
{
    switch (LOWORD(LParam))
    {
    case PH_NF_MSG_SHOWMINIINFOSECTION:
        {
            PPH_NF_MSG_SHOWMINIINFOSECTION_DATA data = (PVOID)WParam;

            data->SectionName = L"Disk";
        }
        return TRUE;
    }

    return FALSE;
}

VOID EtpNetworkIconUpdateCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
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
    HANDLE maxNetworkProcessId;
    PPH_PROCESS_ITEM maxNetworkProcessItem;
    PH_FORMAT format[6];

    // Icon

    Icon->Pointers->BeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);
    maxDataCount = drawInfo.Width / 2 + 1;
    lineData1 = _alloca(maxDataCount * sizeof(FLOAT));
    lineData2 = _alloca(maxDataCount * sizeof(FLOAT));

    lineDataCount = min(maxDataCount, EtNetworkReceiveHistory.Count);
    max = 1024 * 1024; // minimum scaling of 1 MB.

    for (i = 0; i < lineDataCount; i++)
    {
        lineData1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&EtNetworkReceiveHistory, i);
        lineData2[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&EtNetworkSendHistory, i);

        if (max < lineData1[i] + lineData2[i])
            max = lineData1[i] + lineData2[i];
    }

    PhDivideSinglesBySingle(lineData1, max, lineDataCount);
    PhDivideSinglesBySingle(lineData2, max, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineData2 = lineData2;
    drawInfo.LineColor1 = PhGetIntegerSetting(L"ColorIoReadOther");
    drawInfo.LineColor2 = PhGetIntegerSetting(L"ColorIoWrite");
    drawInfo.LineBackColor1 = PhHalveColorBrightness(drawInfo.LineColor1);
    drawInfo.LineBackColor2 = PhHalveColorBrightness(drawInfo.LineColor2);

    if (bits)
        PhDrawGraphDirect(hdc, bits, &drawInfo);

    SelectObject(hdc, oldBitmap);
    *NewIconOrBitmap = bitmap;
    *Flags = PH_NF_UPDATE_IS_BITMAP;

    // Text

    if (EtMaxNetworkHistory.Count != 0)
        maxNetworkProcessId = UlongToHandle(PhGetItemCircularBuffer_ULONG(&EtMaxNetworkHistory, 0));
    else
        maxNetworkProcessId = NULL;

    if (maxNetworkProcessId)
        maxNetworkProcessItem = PhReferenceProcessItem(maxNetworkProcessId);
    else
        maxNetworkProcessItem = NULL;

    PhInitFormatS(&format[0], L"Network\nR: ");
    PhInitFormatSize(&format[1], EtNetworkReceiveDelta.Delta);
    PhInitFormatS(&format[2], L"\nS: ");
    PhInitFormatSize(&format[3], EtNetworkSendDelta.Delta);

    if (maxNetworkProcessItem)
    {
        PhInitFormatC(&format[4], '\n');
        PhInitFormatSR(&format[5], maxNetworkProcessItem->ProcessName->sr);
    }

    *NewText = PhFormat(format, maxNetworkProcessItem ? 6 : 4, 128);
    if (maxNetworkProcessItem) PhDereferenceObject(maxNetworkProcessItem);
}

BOOLEAN EtpNetworkIconMessageCallback(
    _In_ struct _PH_NF_ICON *Icon,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam,
    _In_opt_ PVOID Context
    )
{
    switch (LOWORD(LParam))
    {
    case PH_NF_MSG_SHOWMINIINFOSECTION:
        {
            PPH_NF_MSG_SHOWMINIINFOSECTION_DATA data = (PVOID)WParam;

            data->SectionName = L"Network";
        }
        return TRUE;
    }

    return FALSE;
}
