/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2011
 *     dmex    2016-2023
 *
 */

#include "exttools.h"
#include "etwsys.h"
#include "gpusys.h"
#include "npusys.h"
#include <toolstatusintf.h>

typedef enum _ETP_TRAY_ICON_ID
{
    ETP_TRAY_ICON_ID_NONE,
    ETP_TRAY_ICON_ID_GPU,
    ETP_TRAY_ICON_ID_DISK,
    ETP_TRAY_ICON_ID_NETWORK,
    ETP_TRAY_ICON_ID_GPUTEXT,
    ETP_TRAY_ICON_ID_DISKTEXT,
    ETP_TRAY_ICON_ID_NETWORKTEXT,
    ETP_TRAY_ICON_ID_GPUMEM,
    ETP_TRAY_ICON_ID_GPUMEMTEXT,
    ETP_TRAY_ICON_ID_GPUTEMP,
    ETP_TRAY_ICON_ID_GPUTEMPTEXT,
    ETP_TRAY_ICON_ID_NPU,
    ETP_TRAY_ICON_ID_MAXIMUM
} ETP_TRAY_ICON_ID;

typedef enum _ETP_TRAY_ICON_GUID
{
    ETP_TRAY_ICON_GUID_GPU,
    ETP_TRAY_ICON_GUID_DISK,
    ETP_TRAY_ICON_GUID_NETWORK,
    ETP_TRAY_ICON_GUID_GPUTEXT,
    ETP_TRAY_ICON_GUID_DISKTEXT,
    ETP_TRAY_ICON_GUID_NETWORKTEXT,
    ETP_TRAY_ICON_GUID_GPUMEM,
    ETP_TRAY_ICON_GUID_GPUMEMTEXT,
    ETP_TRAY_ICON_GUID_GPUTEMP,
    ETP_TRAY_ICON_GUID_GPUTEMPTEXT,
    ETP_TRAY_ICON_GUID_NPU,
    ETP_TRAY_ICON_GUID_MAXIMUM
} ETP_TRAY_ICON_GUID;

VOID EtpGpuIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

BOOLEAN EtpGpuIconMessageCallback(
    _In_ PPH_NF_ICON Icon,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam,
    _In_opt_ PVOID Context
    );

VOID EtpNpuIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

BOOLEAN EtpNpuIconMessageCallback(
    _In_ PPH_NF_ICON Icon,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam,
    _In_opt_ PVOID Context
    );

VOID EtpDiskIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

BOOLEAN EtpDiskIconMessageCallback(
    _In_ PPH_NF_ICON Icon,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam,
    _In_opt_ PVOID Context
    );

VOID EtpNetworkIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

BOOLEAN EtpNetworkIconMessageCallback(
    _In_ PPH_NF_ICON Icon,
    _In_ ULONG_PTR WParam,
    _In_ ULONG_PTR LParam,
    _In_opt_ PVOID Context
    );

VOID EtpGpuTextIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

VOID EtpDiskTextIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

VOID EtpNetworkTextIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
    _Out_ PVOID *NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING *NewText,
    _In_opt_ PVOID Context
    );

VOID EtpGpuMemoryIconUpdateCallback(
    _In_ struct _PH_NF_ICON* Icon,
    _Out_ PVOID* NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING* NewText,
    _In_opt_ PVOID Context
    );

VOID EtpGpuMemoryTextIconUpdateCallback(
    _In_ struct _PH_NF_ICON* Icon,
    _Out_ PVOID* NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING* NewText,
    _In_opt_ PVOID Context
    );

VOID EtpGpuTemperatureIconUpdateCallback(
    _In_ struct _PH_NF_ICON* Icon,
    _Out_ PVOID* NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING* NewText,
    _In_opt_ PVOID Context
    );

VOID EtpGpuTemperatureTextIconUpdateCallback(
    _In_ struct _PH_NF_ICON* Icon,
    _Out_ PVOID* NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING* NewText,
    _In_opt_ PVOID Context
    );

BOOLEAN EtTrayIconTransparencyEnabled = FALSE;
GUID EtpTrayIconGuids[ETP_TRAY_ICON_GUID_MAXIMUM];
PH_CALLBACK_REGISTRATION EtpMainWindowMessageEventRegistration;
TB_GRAPH_CONTEXT EtpToolbarGpuHistoryGraphContext = { 0 };
TB_GRAPH_CONTEXT EtpToolbarNpuHistoryGraphContext = { 0 };
TB_GRAPH_CONTEXT EtpToolbarDiskHistoryGraphContext = { 0 };
TB_GRAPH_CONTEXT EtpToolbarNetworkHistoryGraphContext = { 0 };

VOID EtLoadTrayIconGuids(
    VOID
    )
{
    if (PhGetIntegerSetting(L"IconTrayPersistGuidEnabled"))
    {
        PPH_STRING settingsString = NULL;
        PH_STRINGREF remaining;

        settingsString = PhGetStringSetting(SETTING_NAME_TRAYICON_GUIDS);

        if (PhIsNullOrEmptyString(settingsString))
        {
            PH_STRING_BUILDER iconListBuilder;
            PPH_STRING iconGuid;

            PhInitializeStringBuilder(&iconListBuilder, 100);

            for (ULONG i = 0; i < RTL_NUMBER_OF(EtpTrayIconGuids); i++)
            {
                PhGenerateGuid(&EtpTrayIconGuids[i]);

                if (iconGuid = PhFormatGuid(&EtpTrayIconGuids[i]))
                {
                    PhAppendFormatStringBuilder(
                        &iconListBuilder,
                        L"%s|",
                        iconGuid->Buffer
                        );
                    PhDereferenceObject(iconGuid);
                }
            }

            if (iconListBuilder.String->Length != 0)
                PhRemoveEndStringBuilder(&iconListBuilder, 1);

            PhMoveReference(&settingsString, PhFinalStringBuilderString(&iconListBuilder));
            PhSetStringSetting2(SETTING_NAME_TRAYICON_GUIDS, &settingsString->sr);
            PhDereferenceObject(settingsString);
        }
        else
        {
            remaining = PhGetStringRef(settingsString);

            for (ULONG i = 0; i < RTL_NUMBER_OF(EtpTrayIconGuids); i++)
            {
                PH_STRINGREF guidPart;
                GUID guid;

                if (remaining.Length == 0)
                    continue;

                PhSplitStringRefAtChar(&remaining, L'|', &guidPart, &remaining);

                if (guidPart.Length == 0)
                    continue;

                if (!NT_SUCCESS(PhStringToGuid(&guidPart, &guid)))
                    PhGenerateGuid(&EtpTrayIconGuids[i]);
                else
                    EtpTrayIconGuids[i] = guid;
            }

            PhDereferenceObject(settingsString);
        }
    }
}

VOID EtRegisterNotifyIcons(
    _In_ PPH_TRAY_ICON_POINTERS Pointers
    )
{
    PH_NF_ICON_REGISTRATION_DATA data;

    data.MessageCallback = NULL;

    data.UpdateCallback = EtpGpuIconUpdateCallback;
    data.MessageCallback = EtpGpuIconMessageCallback;
    Pointers->RegisterTrayIcon(
        PluginInstance,
        ETP_TRAY_ICON_ID_GPU,
        EtpTrayIconGuids[ETP_TRAY_ICON_GUID_GPU],
        NULL,
        L"&GPU history",
        EtGpuEnabled ? 0 : PH_NF_ICON_UNAVAILABLE,
        &data
        );

    data.UpdateCallback = EtpNpuIconUpdateCallback;
    data.MessageCallback = EtpNpuIconMessageCallback;
    Pointers->RegisterTrayIcon(
        PluginInstance,
        ETP_TRAY_ICON_ID_NPU,
        EtpTrayIconGuids[ETP_TRAY_ICON_GUID_NPU],
        NULL,
        L"&NPU history",
        EtNpuEnabled ? 0 : PH_NF_ICON_UNAVAILABLE,
        &data
        );

    data.UpdateCallback = EtpDiskIconUpdateCallback;
    data.MessageCallback = EtpDiskIconMessageCallback;
    Pointers->RegisterTrayIcon(
        PluginInstance,
        ETP_TRAY_ICON_ID_DISK,
        EtpTrayIconGuids[ETP_TRAY_ICON_GUID_DISK],
        NULL,
        L"&Disk history",
        EtEtwEnabled ? 0 : PH_NF_ICON_UNAVAILABLE,
        &data
        );

    data.UpdateCallback = EtpNetworkIconUpdateCallback;
    data.MessageCallback = EtpNetworkIconMessageCallback;
    Pointers->RegisterTrayIcon(
        PluginInstance,
        ETP_TRAY_ICON_ID_NETWORK,
        EtpTrayIconGuids[ETP_TRAY_ICON_GUID_NETWORK],
        NULL,
        L"&Network history",
        EtEtwEnabled ? 0 : PH_NF_ICON_UNAVAILABLE,
        &data
        );

    data.UpdateCallback = EtpGpuTextIconUpdateCallback;
    data.MessageCallback = EtpGpuIconMessageCallback;
    Pointers->RegisterTrayIcon(
        PluginInstance,
        ETP_TRAY_ICON_ID_GPUTEXT,
        EtpTrayIconGuids[ETP_TRAY_ICON_GUID_GPUTEXT],
        NULL,
        L"&GPU usage (text)",
        EtGpuEnabled ? 0 : PH_NF_ICON_UNAVAILABLE,
        &data
        );

    data.UpdateCallback = EtpDiskTextIconUpdateCallback;
    data.MessageCallback = EtpDiskIconMessageCallback;
    Pointers->RegisterTrayIcon(
        PluginInstance,
        ETP_TRAY_ICON_ID_DISKTEXT,
        EtpTrayIconGuids[ETP_TRAY_ICON_GUID_DISKTEXT],
        NULL,
        L"&Disk usage (text)",
        EtEtwEnabled ? 0 : PH_NF_ICON_UNAVAILABLE,
        &data
        );

    data.UpdateCallback = EtpNetworkTextIconUpdateCallback;
    data.MessageCallback = EtpNetworkIconMessageCallback;
    Pointers->RegisterTrayIcon(
        PluginInstance,
        ETP_TRAY_ICON_ID_NETWORKTEXT,
        EtpTrayIconGuids[ETP_TRAY_ICON_GUID_NETWORKTEXT],
        NULL,
        L"&Network usage (text)",
        EtEtwEnabled ? 0 : PH_NF_ICON_UNAVAILABLE,
        &data
        );

    data.UpdateCallback = EtpGpuMemoryIconUpdateCallback;
    data.MessageCallback = EtpGpuIconMessageCallback;
    Pointers->RegisterTrayIcon(
        PluginInstance,
        ETP_TRAY_ICON_ID_GPUMEM,
        EtpTrayIconGuids[ETP_TRAY_ICON_GUID_GPUMEM],
        NULL,
        L"&GPU memory history",
        EtGpuEnabled ? 0 : PH_NF_ICON_UNAVAILABLE,
        &data
        );

    data.UpdateCallback = EtpGpuMemoryTextIconUpdateCallback;
    data.MessageCallback = EtpGpuIconMessageCallback;
    Pointers->RegisterTrayIcon(
        PluginInstance,
        ETP_TRAY_ICON_ID_GPUMEMTEXT,
        EtpTrayIconGuids[ETP_TRAY_ICON_GUID_GPUMEMTEXT],
        NULL,
        L"&GPU memory usage (text)",
        EtGpuEnabled ? 0 : PH_NF_ICON_UNAVAILABLE,
        &data
        );

    if (EtGpuSupported)
    {
        data.UpdateCallback = EtpGpuTemperatureIconUpdateCallback;
        data.MessageCallback = EtpGpuIconMessageCallback;
        Pointers->RegisterTrayIcon(
            PluginInstance,
            ETP_TRAY_ICON_ID_GPUTEMP,
            EtpTrayIconGuids[ETP_TRAY_ICON_GUID_GPUTEMP],
            NULL,
            L"&GPU temperature history",
            EtGpuEnabled ? 0 : PH_NF_ICON_UNAVAILABLE,
            &data
            );

        data.UpdateCallback = EtpGpuTemperatureTextIconUpdateCallback;
        data.MessageCallback = EtpGpuIconMessageCallback;
        Pointers->RegisterTrayIcon(
            PluginInstance,
            ETP_TRAY_ICON_ID_GPUTEMPTEXT,
            EtpTrayIconGuids[ETP_TRAY_ICON_GUID_GPUTEMPTEXT],
            NULL,
            L"&GPU temperature (text)",
            EtGpuEnabled ? 0 : PH_NF_ICON_UNAVAILABLE,
            &data
            );
    }
}

VOID EtpGpuIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
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
    lineData1 = _malloca(maxDataCount * sizeof(FLOAT));

    if (!lineData1)
    {
        SelectBitmap(hdc, oldBitmap);
        *NewIconOrBitmap = bitmap;
        *Flags = PH_NF_UPDATE_IS_BITMAP;
        *NewText = PhReferenceEmptyString();
        return;
    }

    lineDataCount = min(maxDataCount, EtGpuNodeHistory.Count);
    PhCopyCircularBuffer_FLOAT(&EtGpuNodeHistory, lineData1, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineColor1 = PhGetIntegerSetting(L"ColorCpuKernel");
    drawInfo.LineBackColor1 = PhHalveColorBrightness(drawInfo.LineColor1);
    PhDrawGraphDirect(hdc, bits, &drawInfo);

    if (EtTrayIconTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, drawInfo.Width, drawInfo.Height);
    }

    SelectBitmap(hdc, oldBitmap);
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

    PhInitFormatS(&format[0], L"GPU usage: ");
    PhInitFormatF(&format[1], EtGpuNodeUsage * 100, EtMaxPrecisionUnit);
    PhInitFormatC(&format[2], '%');

    if (maxGpuProcessItem)
    {
        PhInitFormatC(&format[3], '\n');
        PhInitFormatSR(&format[4], maxGpuProcessItem->ProcessName->sr);
        PhInitFormatS(&format[5], L": ");
        PhInitFormatF(&format[6], EtGetProcessBlock(maxGpuProcessItem)->GpuNodeUtilization * 100, EtMaxPrecisionUnit);
        PhInitFormatC(&format[7], '%');
    }

    *NewText = PhFormat(format, maxGpuProcessItem ? 8 : 3, 128);
    if (maxGpuProcessItem) PhDereferenceObject(maxGpuProcessItem);

    _freea(lineData1);
}

BOOLEAN EtpGpuIconMessageCallback(
    _In_ PPH_NF_ICON Icon,
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

VOID EtpNpuIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
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
    HANDLE maxNpuProcessId;
    PPH_PROCESS_ITEM maxNpuProcessItem;
    PH_FORMAT format[8];

    // Icon

    Icon->Pointers->BeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);
    maxDataCount = drawInfo.Width / 2 + 1;
    lineData1 = _malloca(maxDataCount * sizeof(FLOAT));

    if (!lineData1)
    {
        SelectBitmap(hdc, oldBitmap);
        *NewIconOrBitmap = bitmap;
        *Flags = PH_NF_UPDATE_IS_BITMAP;
        *NewText = PhReferenceEmptyString();
        return;
    }

    lineDataCount = min(maxDataCount, EtNpuNodeHistory.Count);
    PhCopyCircularBuffer_FLOAT(&EtNpuNodeHistory, lineData1, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineColor1 = PhGetIntegerSetting(L"ColorCpuKernel");
    drawInfo.LineBackColor1 = PhHalveColorBrightness(drawInfo.LineColor1);
    PhDrawGraphDirect(hdc, bits, &drawInfo);

    if (EtTrayIconTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, drawInfo.Width, drawInfo.Height);
    }

    SelectBitmap(hdc, oldBitmap);
    *NewIconOrBitmap = bitmap;
    *Flags = PH_NF_UPDATE_IS_BITMAP;

    // Text

    if (EtMaxNpuNodeHistory.Count != 0)
        maxNpuProcessId = UlongToHandle(PhGetItemCircularBuffer_ULONG(&EtMaxNpuNodeHistory, 0));
    else
        maxNpuProcessId = NULL;

    if (maxNpuProcessId)
        maxNpuProcessItem = PhReferenceProcessItem(maxNpuProcessId);
    else
        maxNpuProcessItem = NULL;

    PhInitFormatS(&format[0], L"NPU usage: ");
    PhInitFormatF(&format[1], EtNpuNodeUsage * 100, EtMaxPrecisionUnit);
    PhInitFormatC(&format[2], '%');

    if (maxNpuProcessItem)
    {
        PhInitFormatC(&format[3], '\n');
        PhInitFormatSR(&format[4], maxNpuProcessItem->ProcessName->sr);
        PhInitFormatS(&format[5], L": ");
        PhInitFormatF(&format[6], EtGetProcessBlock(maxNpuProcessItem)->NpuNodeUtilization * 100, EtMaxPrecisionUnit);
        PhInitFormatC(&format[7], '%');
    }

    *NewText = PhFormat(format, maxNpuProcessItem ? 8 : 3, 128);
    if (maxNpuProcessItem) PhDereferenceObject(maxNpuProcessItem);

    _freea(lineData1);
}

BOOLEAN EtpNpuIconMessageCallback(
    _In_ PPH_NF_ICON Icon,
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

            data->SectionName = L"NPU";
        }
        return TRUE;
    }

    return FALSE;
}

VOID EtpDiskIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
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
    lineData1 = _malloca(maxDataCount * sizeof(FLOAT));
    lineData2 = _malloca(maxDataCount * sizeof(FLOAT));

    if (!(lineData1 && lineData2))
    {
        SelectBitmap(hdc, oldBitmap);
        *NewIconOrBitmap = bitmap;
        *Flags = PH_NF_UPDATE_IS_BITMAP;
        *NewText = PhReferenceEmptyString();
        return;
    }

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
    PhDrawGraphDirect(hdc, bits, &drawInfo);

    if (EtTrayIconTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, drawInfo.Width, drawInfo.Height);
    }

    SelectBitmap(hdc, oldBitmap);
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

    _freea(lineData2);
    _freea(lineData1);
}

BOOLEAN EtpDiskIconMessageCallback(
    _In_ PPH_NF_ICON Icon,
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
    _In_ PPH_NF_ICON Icon,
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
    lineData1 = _malloca(maxDataCount * sizeof(FLOAT));
    lineData2 = _malloca(maxDataCount * sizeof(FLOAT));

    if (!(lineData1 && lineData2))
    {
        SelectBitmap(hdc, oldBitmap);
        *NewIconOrBitmap = bitmap;
        *Flags = PH_NF_UPDATE_IS_BITMAP;
        *NewText = PhReferenceEmptyString();
        return;
    }

    lineDataCount = min(maxDataCount, EtNetworkReceiveHistory.Count);
    max = 1024 * 1024; // minimum scaling of 1 MB.

    if (EtEnableAvxSupport && lineDataCount > 64) // 128
    {
        FLOAT data1;
        FLOAT data2;

        PhCopyConvertCircularBufferULONG(&EtNetworkReceiveHistory, lineData1, lineDataCount);
        PhCopyConvertCircularBufferULONG(&EtNetworkSendHistory, lineData2, lineDataCount);

        data1 = PhMaxMemorySingles(lineData1, lineDataCount);
        data2 = PhMaxMemorySingles(lineData2, lineDataCount);

        if (max < data1 + data2)
            max = data1 + data2;
    }
    else
    {
        for (i = 0; i < lineDataCount; i++)
        {
            lineData1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&EtNetworkReceiveHistory, i);
            lineData2[i] = (FLOAT)PhGetItemCircularBuffer_ULONG(&EtNetworkSendHistory, i);

            if (max < lineData1[i] + lineData2[i])
                max = lineData1[i] + lineData2[i];
        }
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
    PhDrawGraphDirect(hdc, bits, &drawInfo);

    if (EtTrayIconTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, drawInfo.Width, drawInfo.Height);
    }

    SelectBitmap(hdc, oldBitmap);
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

    _freea(lineData2);
    _freea(lineData1);
}

BOOLEAN EtpNetworkIconMessageCallback(
    _In_ PPH_NF_ICON Icon,
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

// Text

VOID EtpGpuTextIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
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
        0,
        RGB(0x00, 0x00, 0x00),
        0,
        NULL,
        NULL,
        0,
        0,
        0,
        0
    };
    HBITMAP bitmap;
    PVOID bits;
    HDC hdc;
    HBITMAP oldBitmap;
    HANDLE maxGpuProcessId;
    PPH_PROCESS_ITEM maxGpuProcessItem;
    PH_FORMAT format[8];
    PPH_STRING text;

    // Icon

    Icon->Pointers->BeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);

    PhInitFormatF(&format[0], (DOUBLE)EtGpuNodeUsage * 100, 0);
    text = PhFormat(format, 1, 10);

    drawInfo.TextFont = PhNfGetTrayIconFont(0);
    drawInfo.TextColor = PhGetIntegerSetting(L"ColorCpuKernel");
    PhDrawTrayIconText(hdc, bits, &drawInfo, &text->sr);
    PhDereferenceObject(text);

    if (EtTrayIconTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, drawInfo.Width, drawInfo.Height);
    }

    SelectBitmap(hdc, oldBitmap);
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

    PhInitFormatS(&format[0], L"GPU usage: ");
    PhInitFormatF(&format[1], EtGpuNodeUsage * 100, EtMaxPrecisionUnit);
    PhInitFormatC(&format[2], '%');

    if (maxGpuProcessItem)
    {
        PhInitFormatC(&format[3], '\n');
        PhInitFormatSR(&format[4], maxGpuProcessItem->ProcessName->sr);
        PhInitFormatS(&format[5], L": ");
        PhInitFormatF(&format[6], EtGetProcessBlock(maxGpuProcessItem)->GpuNodeUtilization * 100, EtMaxPrecisionUnit);
        PhInitFormatC(&format[7], '%');
    }

    *NewText = PhFormat(format, maxGpuProcessItem ? 8 : 3, 128);
    if (maxGpuProcessItem) PhDereferenceObject(maxGpuProcessItem);
}

VOID EtpDiskTextIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
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
        0,
        RGB(0x00, 0x00, 0x00),
        0,
        NULL,
        NULL,
        0,
        0,
        0,
        0
    };
    HBITMAP bitmap;
    PVOID bits;
    HDC hdc;
    HBITMAP oldBitmap;
    HANDLE maxDiskProcessId;
    PPH_PROCESS_ITEM maxDiskProcessItem;
    PH_FORMAT format[6];
    PPH_STRING text;
    static ULONG64 maxValue = 100000 * 1024; // minimum scaling of 100 MB.

    // Icon

    Icon->Pointers->BeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);

    if (maxValue < ((ULONG64)EtDiskReadDelta.Delta + EtDiskWriteDelta.Delta))
        maxValue = ((ULONG64)EtDiskReadDelta.Delta + EtDiskWriteDelta.Delta);

    PhInitFormatF(&format[0], ((DOUBLE)EtDiskReadDelta.Delta + EtDiskWriteDelta.Delta) / maxValue * 100, 0);
    text = PhFormat(format, 1, 10);

    drawInfo.TextFont = PhNfGetTrayIconFont(0);
    drawInfo.TextColor = PhGetIntegerSetting(L"ColorIoReadOther"); // ColorIoWrite
    PhDrawTrayIconText(hdc, bits, &drawInfo, &text->sr);
    PhDereferenceObject(text);

    if (EtTrayIconTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, drawInfo.Width, drawInfo.Height);
    }

    SelectBitmap(hdc, oldBitmap);
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

VOID EtpNetworkTextIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
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
        0,
        RGB(0x00, 0x00, 0x00),
        0,
        NULL,
        NULL,
        0,
        0,
        0,
        0
    };
    HBITMAP bitmap;
    PVOID bits;
    HDC hdc;
    HBITMAP oldBitmap;
    HANDLE maxNetworkProcessId;
    PPH_PROCESS_ITEM maxNetworkProcessItem;
    PH_FORMAT format[6];
    PPH_STRING text;
    static ULONG64 maxValue = UInt32x32To64(1024, 1024); // minimum scaling of 1 MB.

    // Icon

    Icon->Pointers->BeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);

    if (maxValue < ((ULONG64)EtNetworkReceiveDelta.Delta + EtNetworkSendDelta.Delta))
        maxValue = ((ULONG64)EtNetworkReceiveDelta.Delta + EtNetworkSendDelta.Delta);

    PhInitFormatF(&format[0], ((DOUBLE)EtNetworkReceiveDelta.Delta + EtNetworkSendDelta.Delta) / maxValue * 100, 0);
    text = PhFormat(format, 1, 10);

    drawInfo.TextFont = PhNfGetTrayIconFont(0);
    drawInfo.TextColor = PhGetIntegerSetting(L"ColorIoReadOther"); // ColorIoWrite
    PhDrawTrayIconText(hdc, bits, &drawInfo, &text->sr);
    PhDereferenceObject(text);

    if (EtTrayIconTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, drawInfo.Width, drawInfo.Height);
    }

    SelectBitmap(hdc, oldBitmap);
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

// GPU Memory

VOID EtpGpuMemoryIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
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
    PH_FORMAT format[2];

    // Icon

    Icon->Pointers->BeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);
    maxDataCount = drawInfo.Width / 2 + 1;
    lineData1 = _malloca(maxDataCount * sizeof(FLOAT));

    if (!lineData1)
    {
        SelectBitmap(hdc, oldBitmap);
        *NewIconOrBitmap = bitmap;
        *Flags = PH_NF_UPDATE_IS_BITMAP;
        *NewText = PhReferenceEmptyString();
        return;
    }

    lineDataCount = min(maxDataCount, EtGpuDedicatedHistory.Count);
    for (ULONG i = 0; i < lineDataCount; i++)
    {
        lineData1[i] = (FLOAT)PhGetItemCircularBuffer_ULONG64(&EtGpuDedicatedHistory, i);
    }
    PhDivideSinglesBySingle(lineData1, (FLOAT)EtGpuDedicatedLimit, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineColor1 = PhGetIntegerSetting(L"ColorPrivate");
    drawInfo.LineBackColor1 = PhHalveColorBrightness(drawInfo.LineColor1);
    PhDrawGraphDirect(hdc, bits, &drawInfo);

    if (EtTrayIconTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, drawInfo.Width, drawInfo.Height);
    }

    SelectBitmap(hdc, oldBitmap);
    *NewIconOrBitmap = bitmap;
    *Flags = PH_NF_UPDATE_IS_BITMAP;

    // Text

    PhInitFormatS(&format[0], L"GPU memory usage: ");
    PhInitFormatSize(&format[1], EtGpuDedicatedUsage);

    *NewText = PhFormat(format, 2, 128);

    _freea(lineData1);
}

VOID EtpGpuMemoryTextIconUpdateCallback(
    _In_ PPH_NF_ICON Icon,
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
        0,
        RGB(0x00, 0x00, 0x00),
        0,
        NULL,
        NULL,
        0,
        0,
        0,
        0
    };
    HBITMAP bitmap;
    PVOID bits;
    HDC hdc;
    HBITMAP oldBitmap;
    PH_FORMAT format[2];
    PPH_STRING text;

    // Icon

    Icon->Pointers->BeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);

    PhInitFormatF(&format[0], (EtGpuDedicatedUsage * 100.0f) / EtGpuDedicatedLimit, 0);
    text = PhFormat(format, 1, 10);

    drawInfo.TextFont = PhNfGetTrayIconFont(0);
    drawInfo.TextColor = PhGetIntegerSetting(L"ColorPrivate");
    PhDrawTrayIconText(hdc, bits, &drawInfo, &text->sr);
    PhDereferenceObject(text);

    if (EtTrayIconTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, drawInfo.Width, drawInfo.Height);
    }

    SelectBitmap(hdc, oldBitmap);
    *NewIconOrBitmap = bitmap;
    *Flags = PH_NF_UPDATE_IS_BITMAP;

    // Text

    PhInitFormatS(&format[0], L"GPU memory usage: ");
    PhInitFormatSize(&format[1], EtGpuDedicatedUsage);

    *NewText = PhFormat(format, 2, 128);
}

// GPU Temperature

VOID EtpGpuTemperatureIconUpdateCallback(
    _In_ struct _PH_NF_ICON* Icon,
    _Out_ PVOID* NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING* NewText,
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
    PH_FORMAT format[3];

    // Icon

    Icon->Pointers->BeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);
    maxDataCount = drawInfo.Width / 2 + 1;
    lineData1 = _malloca(maxDataCount * sizeof(FLOAT));

    if (!lineData1)
    {
        SelectBitmap(hdc, oldBitmap);
        *NewIconOrBitmap = bitmap;
        *Flags = PH_NF_UPDATE_IS_BITMAP;
        *NewText = PhReferenceEmptyString();
        return;
    }

    lineDataCount = min(maxDataCount, EtGpuTemperatureHistory.Count);
    PhCopyCircularBuffer_FLOAT(&EtGpuTemperatureHistory, lineData1, lineDataCount);
    PhDivideSinglesBySingle(lineData1, EtGpuTemperatureLimit, lineDataCount);

    drawInfo.LineDataCount = lineDataCount;
    drawInfo.LineData1 = lineData1;
    drawInfo.LineColor1 = PhGetIntegerSetting(L"ColorTemperature");
    drawInfo.LineBackColor1 = PhHalveColorBrightness(drawInfo.LineColor1);
    PhDrawGraphDirect(hdc, bits, &drawInfo);

    if (EtTrayIconTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, drawInfo.Width, drawInfo.Height);
    }

    SelectBitmap(hdc, oldBitmap);
    *NewIconOrBitmap = bitmap;
    *Flags = PH_NF_UPDATE_IS_BITMAP;

    // Text

    PhInitFormatS(&format[0], L"GPU temperature: ");
    if (EtGpuFahrenheitEnabled)
    {
        PhInitFormatF(&format[1], (EtGpuTemperature * 1.8 + 32), 1);
        PhInitFormatS(&format[2], L"\u00b0F");
    }
    else
    {
        PhInitFormatF(&format[1], EtGpuTemperature, 1);
        PhInitFormatS(&format[2], L"\u00b0C");
    }

    *NewText = PhFormat(format, 3, 128);

    _freea(lineData1);
}

VOID EtpGpuTemperatureTextIconUpdateCallback(
    _In_ struct _PH_NF_ICON* Icon,
    _Out_ PVOID* NewIconOrBitmap,
    _Out_ PULONG Flags,
    _Out_ PPH_STRING* NewText,
    _In_opt_ PVOID Context
    )
{
    static PH_GRAPH_DRAW_INFO drawInfo =
    {
        16,
        16,
        0,
        0,
        RGB(0x00, 0x00, 0x00),
        0,
        NULL,
        NULL,
        0,
        0,
        0,
        0
    };
    HBITMAP bitmap;
    PVOID bits;
    HDC hdc;
    HBITMAP oldBitmap;
    PH_FORMAT format[3];
    PPH_STRING text;

    // Icon

    Icon->Pointers->BeginBitmap(&drawInfo.Width, &drawInfo.Height, &bitmap, &bits, &hdc, &oldBitmap);

    if (EtGpuFahrenheitEnabled)
        PhInitFormatF(&format[0], (EtGpuTemperature * 1.8 + 32), 0);
    else
        PhInitFormatF(&format[0], EtGpuTemperature, 0);
    text = PhFormat(format, 1, 10);

    drawInfo.TextFont = PhNfGetTrayIconFont(0);
    drawInfo.TextColor = PhGetIntegerSetting(L"ColorTemperature");
    PhDrawTrayIconText(hdc, bits, &drawInfo, &text->sr);
    PhDereferenceObject(text);

    if (EtTrayIconTransparencyEnabled)
    {
        PhBitmapSetAlpha(bits, drawInfo.Width, drawInfo.Height);
    }

    SelectBitmap(hdc, oldBitmap);
    *NewIconOrBitmap = bitmap;
    *Flags = PH_NF_UPDATE_IS_BITMAP;

    // Text

    PhInitFormatS(&format[0], L"GPU temperature: ");
    if (EtGpuFahrenheitEnabled)
    {
        PhInitFormatF(&format[1], (EtGpuTemperature * 1.8 + 32), 1);
        PhInitFormatS(&format[2], L"\u00b0F");
    }
    else
    {
        PhInitFormatF(&format[1], EtGpuTemperature, 1);
        PhInitFormatS(&format[2], L"\u00b0C");
    }

    *NewText = PhFormat(format, 3, 128);
}

// Toolbar graphs

TOOLSTATUS_GRAPH_MESSAGE_CALLBACK_DECLARE(EtpToolbarGpuHistoryGraphMessageCallback)
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PTB_GRAPH_CONTEXT context = (PTB_GRAPH_CONTEXT)Context;
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            if (context->GraphDpi == 0)
            {
                context->GraphDpi = PhGetWindowDpi(GraphHandle);
                context->GraphColor1 = PhGetIntegerSetting(L"ColorCpuKernel");
            }

            drawInfo->Flags = PH_GRAPH_USE_GRID_X;
            PhSiSetColorsGraphDrawInfo(drawInfo, context->GraphColor1, 0, context->GraphDpi);

            if (ProcessesUpdatedCount != 3)
                break;

            PhGraphStateGetDrawInfo(GraphState, getDrawInfo, EtGpuNodeHistory.Count);

            if (!GraphState->Valid)
            {
                PhCopyCircularBuffer_FLOAT(&EtGpuNodeHistory, GraphState->Data1, drawInfo->LineDataCount);

                GraphState->Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (GraphState->TooltipIndex != getTooltipText->Index)
                {
                    FLOAT gpuUsage;
                    PH_FORMAT format[5];

                    gpuUsage = PhGetItemCircularBuffer_FLOAT(&EtGpuNodeHistory, getTooltipText->Index);

                    // %.2f%%%s\n%s
                    PhInitFormatF(&format[0], gpuUsage * 100, EtMaxPrecisionUnit);
                    PhInitFormatC(&format[1], L'%');
                    PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, EtpGpuGetMaxNodeString(getTooltipText->Index))->sr);
                    PhInitFormatC(&format[3], L'\n');
                    PhInitFormatSR(&format[4], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&GraphState->TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 160));
                }

                getTooltipText->Text = PhGetStringRef(GraphState->TooltipText);
            }
        }
        break;
    case GCN_MOUSEEVENT:
        {
            PPH_GRAPH_MOUSEEVENT mouseEvent = (PPH_GRAPH_MOUSEEVENT)Header;
            PPH_PROCESS_RECORD record = NULL;

            if (mouseEvent->Message == WM_LBUTTONDBLCLK)
            {
                if (PhGetIntegerSetting(SETTING_NAME_SHOWSYSINFOGRAPH))
                {
                    PhShowSystemInformationDialog(L"GPU");
                }
                else
                {
                    if (mouseEvent->Index < mouseEvent->TotalCount)
                    {
                        record = EtpGpuReferenceMaxNodeRecord(mouseEvent->Index);
                    }

                    if (record)
                    {
                        PhShowProcessRecordDialog(PhMainWndHandle, record);
                        PhDereferenceProcessRecord(record);
                    }
                }
            }
        }
        break;
    }
}

TOOLSTATUS_GRAPH_MESSAGE_CALLBACK_DECLARE(EtpToolbarNpuHistoryGraphMessageCallback)
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PTB_GRAPH_CONTEXT context = (PTB_GRAPH_CONTEXT)Context;
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            if (context->GraphDpi == 0)
            {
                context->GraphDpi = PhGetWindowDpi(GraphHandle);
                context->GraphColor1 = PhGetIntegerSetting(L"ColorCpuKernel");
            }

            drawInfo->Flags = PH_GRAPH_USE_GRID_X;
            PhSiSetColorsGraphDrawInfo(drawInfo, context->GraphColor1, 0, context->GraphDpi);

            if (ProcessesUpdatedCount != 3)
                break;

            PhGraphStateGetDrawInfo(GraphState, getDrawInfo, EtNpuNodeHistory.Count);

            if (!GraphState->Valid)
            {
                PhCopyCircularBuffer_FLOAT(&EtNpuNodeHistory, GraphState->Data1, drawInfo->LineDataCount);

                GraphState->Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (GraphState->TooltipIndex != getTooltipText->Index)
                {
                    FLOAT gpuUsage;
                    PH_FORMAT format[5];

                    gpuUsage = PhGetItemCircularBuffer_FLOAT(&EtNpuNodeHistory, getTooltipText->Index);

                    // %.2f%%%s\n%s
                    PhInitFormatF(&format[0], gpuUsage * 100, EtMaxPrecisionUnit);
                    PhInitFormatC(&format[1], L'%');
                    PhInitFormatSR(&format[2], PH_AUTO_T(PH_STRING, EtpNpuGetMaxNodeString(getTooltipText->Index))->sr);
                    PhInitFormatC(&format[3], L'\n');
                    PhInitFormatSR(&format[4], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&GraphState->TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 160));
                }

                getTooltipText->Text = PhGetStringRef(GraphState->TooltipText);
            }
        }
        break;
    case GCN_MOUSEEVENT:
        {
            PPH_GRAPH_MOUSEEVENT mouseEvent = (PPH_GRAPH_MOUSEEVENT)Header;
            PPH_PROCESS_RECORD record = NULL;

            if (mouseEvent->Message == WM_LBUTTONDBLCLK)
            {
                if (PhGetIntegerSetting(SETTING_NAME_SHOWSYSINFOGRAPH))
                {
                    PhShowSystemInformationDialog(L"NPU");
                }
                else
                {
                    if (mouseEvent->Index < mouseEvent->TotalCount)
                    {
                        record = EtpNpuReferenceMaxNodeRecord(mouseEvent->Index);
                    }

                    if (record)
                    {
                        PhShowProcessRecordDialog(PhMainWndHandle, record);
                        PhDereferenceProcessRecord(record);
                    }
                }
            }
        }
        break;
    }
}

TOOLSTATUS_GRAPH_MESSAGE_CALLBACK_DECLARE(EtpToolbarDiskHistoryGraphMessageCallback)
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PTB_GRAPH_CONTEXT context = (PTB_GRAPH_CONTEXT)Context;
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            if (context->GraphDpi == 0)
            {
                context->GraphDpi = PhGetWindowDpi(GraphHandle);
                context->GraphColor1 = PhGetIntegerSetting(L"ColorIoReadOther");
                context->GraphColor2 = PhGetIntegerSetting(L"ColorIoWrite");
            }

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_LINE_2;
            PhSiSetColorsGraphDrawInfo(drawInfo, context->GraphColor1, context->GraphColor2, context->GraphDpi);

            if (ProcessesUpdatedCount != 3)
                break;

            PhGraphStateGetDrawInfo(
                GraphState,
                getDrawInfo,
                EtDiskReadHistory.Count
                );

            if (!GraphState->Valid)
            {
                ULONG i;
                FLOAT max = 1024 * 1024; // Minimum scaling of 1 MB

                if (EtEnableAvxSupport && drawInfo->LineDataCount > 64) // 128
                {
                    FLOAT data1;
                    FLOAT data2;

                    PhCopyConvertCircularBufferULONG(&EtDiskReadHistory, GraphState->Data1, drawInfo->LineDataCount);
                    PhCopyConvertCircularBufferULONG(&EtDiskWriteHistory, GraphState->Data2, drawInfo->LineDataCount);

                    data1 = PhMaxMemorySingles(GraphState->Data1, drawInfo->LineDataCount);
                    data2 = PhMaxMemorySingles(GraphState->Data1, drawInfo->LineDataCount);

                    if (max < data1 + data2)
                        max = data1 + data2;
                }
                else
                {
                    for (i = 0; i < drawInfo->LineDataCount; i++)
                    {
                        FLOAT data1;
                        FLOAT data2;

                        GraphState->Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG(&EtDiskReadHistory, i);
                        GraphState->Data2[i] = data2 = (FLOAT)PhGetItemCircularBuffer_ULONG(&EtDiskWriteHistory, i);

                        if (max < data1 + data2)
                            max = data1 + data2;
                    }
                }

                if (max != 0)
                {
                    // Scale the data.

                    PhDivideSinglesBySingle(
                        GraphState->Data1,
                        max,
                        drawInfo->LineDataCount
                        );
                    PhDivideSinglesBySingle(
                        GraphState->Data2,
                        max,
                        drawInfo->LineDataCount
                        );
                }

                GraphState->Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (GraphState->TooltipIndex != getTooltipText->Index)
                {
                    ULONG64 diskRead;
                    ULONG64 diskWrite;
                    PH_FORMAT format[7];

                    diskRead = PhGetItemCircularBuffer_ULONG(&EtDiskReadHistory, getTooltipText->Index);
                    diskWrite = PhGetItemCircularBuffer_ULONG(&EtDiskWriteHistory, getTooltipText->Index);

                    // R: %s\nW: %s%s\n%s
                    PhInitFormatS(&format[0], L"R: ");
                    PhInitFormatSize(&format[1], diskRead);
                    PhInitFormatS(&format[2], L"\nW: ");
                    PhInitFormatSize(&format[3], diskWrite);
                    PhInitFormatSR(&format[4], PH_AUTO_T(PH_STRING, EtpGetMaxDiskString(getTooltipText->Index))->sr);
                    PhInitFormatC(&format[5], L'\n');
                    PhInitFormatSR(&format[6], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&GraphState->TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 160));
                }

                getTooltipText->Text = PhGetStringRef(GraphState->TooltipText);
            }
        }
        break;
    case GCN_MOUSEEVENT:
        {
            PPH_GRAPH_MOUSEEVENT mouseEvent = (PPH_GRAPH_MOUSEEVENT)Header;
            PPH_PROCESS_RECORD record = NULL;

            if (mouseEvent->Message == WM_LBUTTONDBLCLK)
            {
                if (PhGetIntegerSetting(SETTING_NAME_SHOWSYSINFOGRAPH))
                {
                    PhShowSystemInformationDialog(L"Disk");
                }
                else
                {
                    if (mouseEvent->Index < mouseEvent->TotalCount)
                    {
                        record = EtpReferenceMaxDiskRecord(mouseEvent->Index);
                    }

                    if (record)
                    {
                        PhShowProcessRecordDialog(PhMainWndHandle, record);
                        PhDereferenceProcessRecord(record);
                    }
                }
            }
        }
        break;
    }
}

TOOLSTATUS_GRAPH_MESSAGE_CALLBACK_DECLARE(EtpToolbarNetworkHistoryGraphMessageCallback)
{
    switch (Header->code)
    {
    case GCN_GETDRAWINFO:
        {
            PTB_GRAPH_CONTEXT context = (PTB_GRAPH_CONTEXT)Context;
            PPH_GRAPH_GETDRAWINFO getDrawInfo = (PPH_GRAPH_GETDRAWINFO)Header;
            PPH_GRAPH_DRAW_INFO drawInfo = getDrawInfo->DrawInfo;

            if (context->GraphDpi == 0)
            {
                context->GraphDpi = PhGetWindowDpi(GraphHandle);
                context->GraphColor1 = PhGetIntegerSetting(L"ColorIoReadOther");
                context->GraphColor2 = PhGetIntegerSetting(L"ColorIoWrite");
            }

            drawInfo->Flags = PH_GRAPH_USE_GRID_X | PH_GRAPH_USE_LINE_2;
            PhSiSetColorsGraphDrawInfo(drawInfo, context->GraphColor1, context->GraphColor2, context->GraphDpi);

            if (ProcessesUpdatedCount != 3)
                break;

            PhGraphStateGetDrawInfo(
                GraphState,
                getDrawInfo,
                EtNetworkReceiveHistory.Count
                );

            if (!GraphState->Valid)
            {
                ULONG i;
                FLOAT max = 1024 * 1024; // Minimum scaling of 1 MB

                if (EtEnableAvxSupport && drawInfo->LineDataCount > 64) // 128
                {
                    FLOAT data1;
                    FLOAT data2;

                    PhCopyConvertCircularBufferULONG(&EtNetworkReceiveHistory, GraphState->Data1, drawInfo->LineDataCount);
                    PhCopyConvertCircularBufferULONG(&EtNetworkSendHistory, GraphState->Data2, drawInfo->LineDataCount);

                    data1 = PhMaxMemorySingles(GraphState->Data1, drawInfo->LineDataCount);
                    data2 = PhMaxMemorySingles(GraphState->Data1, drawInfo->LineDataCount);

                    if (max < data1 + data2)
                        max = data1 + data2;
                }
                else
                {
                    for (i = 0; i < drawInfo->LineDataCount; i++)
                    {
                        FLOAT data1;
                        FLOAT data2;

                        GraphState->Data1[i] = data1 = (FLOAT)PhGetItemCircularBuffer_ULONG(&EtNetworkReceiveHistory, i);
                        GraphState->Data2[i] = data2 = (FLOAT)PhGetItemCircularBuffer_ULONG(&EtNetworkSendHistory, i);

                        if (max < data1 + data2)
                            max = data1 + data2;
                    }
                }

                if (max != 0)
                {
                    // Scale the data.

                    PhDivideSinglesBySingle(
                        GraphState->Data1,
                        max,
                        drawInfo->LineDataCount
                        );
                    PhDivideSinglesBySingle(
                        GraphState->Data2,
                        max,
                        drawInfo->LineDataCount
                        );
                }

                GraphState->Valid = TRUE;
            }
        }
        break;
    case GCN_GETTOOLTIPTEXT:
        {
            PPH_GRAPH_GETTOOLTIPTEXT getTooltipText = (PPH_GRAPH_GETTOOLTIPTEXT)Header;

            if (getTooltipText->Index < getTooltipText->TotalCount)
            {
                if (GraphState->TooltipIndex != getTooltipText->Index)
                {
                    ULONG64 networkReceive;
                    ULONG64 networkSend;
                    PH_FORMAT format[7];

                    networkReceive = PhGetItemCircularBuffer_ULONG(&EtNetworkReceiveHistory, getTooltipText->Index);
                    networkSend = PhGetItemCircularBuffer_ULONG(&EtNetworkSendHistory, getTooltipText->Index);

                    // R: %s\nS: %s%s\n%s
                    PhInitFormatS(&format[0], L"R: ");
                    PhInitFormatSize(&format[1], networkReceive);
                    PhInitFormatS(&format[2], L"\nS: ");
                    PhInitFormatSize(&format[3], networkSend);
                    PhInitFormatSR(&format[4], PH_AUTO_T(PH_STRING, EtpGetMaxNetworkString(getTooltipText->Index))->sr);
                    PhInitFormatC(&format[5], L'\n');
                    PhInitFormatSR(&format[6], PH_AUTO_T(PH_STRING, PhGetStatisticsTimeString(NULL, getTooltipText->Index))->sr);

                    PhMoveReference(&GraphState->TooltipText, PhFormat(format, RTL_NUMBER_OF(format), 160));
                }

                getTooltipText->Text = PhGetStringRef(GraphState->TooltipText);
            }
        }
        break;
    case GCN_MOUSEEVENT:
        {
            PPH_GRAPH_MOUSEEVENT mouseEvent = (PPH_GRAPH_MOUSEEVENT)Header;
            PPH_PROCESS_RECORD record = NULL;

            if (mouseEvent->Message == WM_LBUTTONDBLCLK)
            {
                if (PhGetIntegerSetting(SETTING_NAME_SHOWSYSINFOGRAPH))
                {
                    PhShowSystemInformationDialog(L"Network");
                }
                else
                {
                    if (mouseEvent->Index < mouseEvent->TotalCount)
                    {
                        record = EtpReferenceMaxNetworkRecord(mouseEvent->Index);
                    }

                    if (record)
                    {
                        PhShowProcessRecordDialog(PhMainWndHandle, record);
                        PhDereferenceProcessRecord(record);
                    }
                }
            }
        }
        break;
    }
}

VOID EtToolbarGraphsInitializeDpi(
    VOID
    )
{
    memset(&EtpToolbarGpuHistoryGraphContext, 0, sizeof(TB_GRAPH_CONTEXT));
    memset(&EtpToolbarNpuHistoryGraphContext, 0, sizeof(TB_GRAPH_CONTEXT));
    memset(&EtpToolbarDiskHistoryGraphContext, 0, sizeof(TB_GRAPH_CONTEXT));
    memset(&EtpToolbarNetworkHistoryGraphContext, 0, sizeof(TB_GRAPH_CONTEXT));
}

VOID NTAPI EtMainWindowMessageEventCallback(
    _In_ PVOID Parameter,
    _In_ PVOID Context
    )
{
    PMSG message = Parameter;

    switch (message->message)
    {
    case WM_DPICHANGED:
        {
            EtToolbarGraphsInitializeDpi();
        }
        break;
    }
}

VOID EtRegisterToolbarGraphs(
    VOID
    )
{
    PTOOLSTATUS_INTERFACE ToolStatusInterface;

    EtToolbarGraphsInitializeDpi();

    if (ToolStatusInterface = PhGetPluginInterfaceZ(TOOLSTATUS_PLUGIN_NAME, TOOLSTATUS_INTERFACE_VERSION))
    {
        ToolStatusInterface->RegisterToolbarGraph(
            PluginInstance,
            5,
            L"GPU history",
            EtGpuEnabled ? 0 : TOOLSTATUS_GRAPH_UNAVAILABLE,
            &EtpToolbarGpuHistoryGraphContext,
            EtpToolbarGpuHistoryGraphMessageCallback
            );

        ToolStatusInterface->RegisterToolbarGraph(
            PluginInstance,
            6,
            L"NPU history",
            EtNpuEnabled ? 0 : TOOLSTATUS_GRAPH_UNAVAILABLE,
            &EtpToolbarNpuHistoryGraphContext,
            EtpToolbarNpuHistoryGraphMessageCallback
            );

        ToolStatusInterface->RegisterToolbarGraph(
            PluginInstance,
            7,
            L"Disk history",
            EtEtwEnabled ? 0 : TOOLSTATUS_GRAPH_UNAVAILABLE,
            &EtpToolbarDiskHistoryGraphContext,
            EtpToolbarDiskHistoryGraphMessageCallback
            );

        ToolStatusInterface->RegisterToolbarGraph(
            PluginInstance,
            8,
            L"Network history",
            EtEtwEnabled ? 0 : TOOLSTATUS_GRAPH_UNAVAILABLE,
            &EtpToolbarNetworkHistoryGraphContext,
            EtpToolbarNetworkHistoryGraphMessageCallback
            );
    }

    PhRegisterCallback(
        PhGetGeneralCallback(GeneralCallbackWindowNotifyEvent),
        EtMainWindowMessageEventCallback,
        NULL,
        &EtpMainWindowMessageEventRegistration
        );
}
