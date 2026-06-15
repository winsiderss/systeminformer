/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     OpenAI    2026
 *
 */

#include "exttools.h"
#include <float.h>
#include <roapi.h>
#include <windows.devices.power.h>

#define ET_WM_POWERGRID_UPDATE (WM_APP + 1)

DEFINE_GUID(IID_IPowerGridData, 0xc360fb17, 0xfc92, 0x5f6e, 0x99, 0x9d, 0x16, 0xa4, 0xcf, 0x9d, 0x6c, 0x40);
DEFINE_GUID(IID_IPowerGridForecast, 0x077e4de9, 0xed60, 0x58bb, 0xa8, 0x50, 0x00, 0x3c, 0x6a, 0x13, 0x86, 0x85);
DEFINE_GUID(IID_IPowerGridForecastStatics, 0x5b78c806, 0x2e4e, 0x5bcc, 0xbb, 0x34, 0xcb, 0x81, 0xc6, 0x0f, 0x9e, 0x12);

typedef struct _POWER_GRID_WINDOW_CONTEXT
{
    HWND WindowHandle;
    HWND ListViewHandle;
    HWND HeaderHandle;
    HWND SummaryHandle;
    HWND ParentWindowHandle;
    HFONT WindowFont;
    PH_LAYOUT_MANAGER LayoutManager;
} POWER_GRID_WINDOW_CONTEXT, *PPOWER_GRID_WINDOW_CONTEXT;

typedef struct _ET_POWER_GRID_SUMMARY
{
    __x_ABI_CWindows_CFoundation_CDateTime ForecastStart;
    __x_ABI_CWindows_CFoundation_CTimeSpan BlockDuration;
    __x_ABI_CWindows_CFoundation_CDateTime BestLowStart;
    __x_ABI_CWindows_CFoundation_CDateTime LowestStart;
    __x_ABI_CWindows_CFoundation_CDateTime HighestStart;
    ULONG TotalBlocks;
    ULONG LowImpactCount;
    DOUBLE AverageSeverity;
    DOUBLE BestLowSeverity;
    DOUBLE LowestSeverity;
    DOUBLE HighestSeverity;
} ET_POWER_GRID_SUMMARY, *PET_POWER_GRID_SUMMARY;

typedef struct _ET_POWER_GRID_UPDATE
{
    PPH_LIST Entries;
    ET_POWER_GRID_SUMMARY Summary;
} ET_POWER_GRID_UPDATE, *PET_POWER_GRID_UPDATE;

typedef struct _ET_POWER_GRID_ENTRY
{
    PPH_STRING Severity;
    PPH_STRING LowImpact;
    PPH_STRING StartTime;
    PPH_STRING BlockDuration;
    PPH_STRING TimeUntilStart;
    DOUBLE SeverityValue;
    boolean LowImpactFlag;
    boolean ActiveFlag;
    __x_ABI_CWindows_CFoundation_CDateTime BlockStartTime;
    __x_ABI_CWindows_CFoundation_CDateTime BlockEndTime;
} ET_POWER_GRID_ENTRY, *PET_POWER_GRID_ENTRY;

VOID EtpFreePowerGridEntry(
    _In_ PET_POWER_GRID_ENTRY Entry
    )
{
    if (!Entry)
        return;

    PhClearReference(&Entry->Severity);
    PhClearReference(&Entry->LowImpact);
    PhClearReference(&Entry->StartTime);
    PhClearReference(&Entry->BlockDuration);
    PhClearReference(&Entry->TimeUntilStart);
    PhFree(Entry);
}

VOID EtpFreePowerGridList(
    _In_opt_ PPH_LIST List
    )
{
    if (!List)
        return;

    for (ULONG i = 0; i < List->Count; i++)
    {
        EtpFreePowerGridEntry(List->Items[i]);
    }

    PhDereferenceObject(List);
}

VOID EtpClearPowerGridListView(
    _In_ HWND ListViewHandle
    )
{
    ListView_DeleteAllItems(ListViewHandle);
}

__x_ABI_CWindows_CFoundation_CDateTime EtpAddTicks(
    __x_ABI_CWindows_CFoundation_CDateTime DateTime,
    INT64 Ticks
    )
{
    __x_ABI_CWindows_CFoundation_CDateTime Result = DateTime;

    Result.UniversalTime += Ticks;
    return Result;
}

VOID EtpFormatPowerGridDateTime(
    _In_ __x_ABI_CWindows_CFoundation_CDateTime DateTime,
    _Out_writes_(PH_DATETIME_STR_LEN_1) PWSTR Buffer,
    _Out_opt_ PSIZE_T ReturnLength
    )
{
    LARGE_INTEGER largeInteger;
    SYSTEMTIME systemTime;
    SYSTEMTIME localSystemTime;

    if (!DateTime.UniversalTime)
    {
        wcscpy_s(Buffer, PH_DATETIME_STR_LEN_1, L"N/A");
        return;
    }

    largeInteger.QuadPart = DateTime.UniversalTime;
    PhLargeIntegerToSystemTime(&systemTime, &largeInteger);

    if (PhSystemTimeToTzSpecificLocalTime(&systemTime, &localSystemTime) &&
        PhFormatDateTimeToBuffer(&localSystemTime, Buffer, PH_DATETIME_STR_LEN_1 * sizeof(WCHAR), ReturnLength))
    {
        return;
    }

    if (PhFormatDateTimeToBuffer(&systemTime, Buffer, PH_DATETIME_STR_LEN_1 * sizeof(WCHAR), ReturnLength))
        return;

    wcscpy_s(Buffer, PH_DATETIME_STR_LEN_1, L"N/A");
}

VOID EtpFormatPowerGridDuration(
    _In_ __x_ABI_CWindows_CFoundation_CTimeSpan Duration,
    _Out_writes_(PH_TIMESPAN_STR_LEN_1) PWSTR Buffer
    )
{
    if (Duration.Duration <= 0)
    {
        wcscpy_s(Buffer, PH_TIMESPAN_STR_LEN_1, L"N/A");
        return;
    }

    PhPrintTimeSpan(Buffer, (ULONG64)Duration.Duration, PH_TIMESPAN_DHMS);
}

VOID EtAddSummaryWindowText(
    _In_ HWND SummaryHandle,
    _In_ const ET_POWER_GRID_SUMMARY* Summary
    )
{
    PH_STRING_BUILDER sb;
    WCHAR startBuffer[PH_DATETIME_STR_LEN_1];
    WCHAR endBuffer[PH_DATETIME_STR_LEN_1];
    __x_ABI_CWindows_CFoundation_CDateTime forecastEnd;
    __x_ABI_CWindows_CFoundation_CDateTime bestLowEnd;
    DOUBLE blockMinutes;
    DOUBLE totalMinutes;
    DOUBLE lowImpactRatio;

    if (!Summary || Summary->TotalBlocks == 0)
    {
        SetWindowText(SummaryHandle, L"No forecast data available.");
        return;
    }

    forecastEnd = EtpAddTicks(Summary->ForecastStart, (INT64)Summary->TotalBlocks * Summary->BlockDuration.Duration);
    blockMinutes = (DOUBLE)Summary->BlockDuration.Duration / (DOUBLE)PH_TICKS_PER_MIN;
    totalMinutes = blockMinutes * Summary->TotalBlocks;
    lowImpactRatio = (DOUBLE)Summary->LowImpactCount * 100.0 / (DOUBLE)Summary->TotalBlocks;

    EtpFormatPowerGridDateTime(Summary->ForecastStart, startBuffer, NULL);
    EtpFormatPowerGridDateTime(forecastEnd, endBuffer, NULL);

    PhInitializeStringBuilder(&sb, 512);
    PhAppendFormatStringBuilder(&sb, L"Forecast period: %s to %s\r\n", startBuffer, endBuffer);
    PhAppendFormatStringBuilder(&sb, L"Total blocks: %u (each ~%.0f minutes, total ~%.0f minutes)\r\n", Summary->TotalBlocks, blockMinutes, totalMinutes);
    PhAppendFormatStringBuilder(&sb, L"Average severity: %.4f\r\n", Summary->AverageSeverity);
    PhAppendFormatStringBuilder(&sb, L"Low-impact windows: %u of %u (%.1f%%)\r\n", Summary->LowImpactCount, Summary->TotalBlocks, lowImpactRatio);

    if (Summary->BestLowSeverity < DBL_MAX)
    {
        WCHAR rangeFrom[PH_DATETIME_STR_LEN_1];
        WCHAR rangeTo[PH_DATETIME_STR_LEN_1];

        bestLowEnd = EtpAddTicks(Summary->BestLowStart, Summary->BlockDuration.Duration);
        EtpFormatPowerGridDateTime(Summary->BestLowStart, rangeFrom, NULL);
        EtpFormatPowerGridDateTime(bestLowEnd, rangeTo, NULL);
        PhAppendFormatStringBuilder(&sb, L"Forecast low-impact time: severity %.4f from %s to %s\r\n", Summary->BestLowSeverity, rangeFrom, rangeTo);
    }
    else
    {
        PhAppendStringBuilder2(&sb, L"Forecast low-impact time: N/A\r\n");
    }

    PhSetWindowText(SummaryHandle, PhFinalStringBuilderString(&sb)->Buffer);
    PhDeleteStringBuilder(&sb);
}

PPH_STRING EtpFormatRelativeTimeString(
    _In_ __x_ABI_CWindows_CFoundation_CDateTime Target,
    _In_ __x_ABI_CWindows_CFoundation_CDateTime Reference
    )
{
    if (!Target.UniversalTime)
        return PhCreateString(L"N/A");

    INT64 delta = Target.UniversalTime - Reference.UniversalTime;
    BOOLEAN future = delta >= 0;

    if (!future)
        delta = -delta;

    ULONG totalSeconds = (ULONG)(delta / (INT64)PH_TICKS_PER_SEC);
    ULONG hours = totalSeconds / 3600;
    ULONG minutes = (totalSeconds % 3600) / 60;
    ULONG seconds = totalSeconds % 60;

    if (hours == 0 && minutes == 0 && seconds == 0)
    {
        return PhCreateString(future ? L"in moments" : L"just now");
    }

    if (hours > 0)
    {
        if (minutes > 0 && seconds > 0)
            return PhFormatString(future ? L"in %u h %u m %u s" : L"%u h %u m %u s ago", hours, minutes, seconds);
        if (minutes > 0)
            return PhFormatString(future ? L"in %u h %u m" : L"%u h %u m ago", hours, minutes);
        if (seconds > 0)
            return PhFormatString(future ? L"in %u h %u s" : L"%u h %u s ago", hours, seconds);

        return PhFormatString(future ? L"in %u h" : L"%u h ago", hours);
    }

    if (minutes > 0)
    {
        if (seconds > 0)
            return PhFormatString(future ? L"in %u m %u s" : L"%u m %u s ago", minutes, seconds);

        return PhFormatString(future ? L"in %u m" : L"%u m ago", minutes);
    }

    return PhFormatString(future ? L"in %u s" : L"%u s ago", seconds);
}

BOOLEAN EtpIsPowerGridEntryActive(
    _In_ __x_ABI_CWindows_CFoundation_CDateTime Start,
    _In_ __x_ABI_CWindows_CFoundation_CDateTime End,
    _In_ __x_ABI_CWindows_CFoundation_CDateTime Reference
    )
{
    if (!Start.UniversalTime || !End.UniversalTime || !Reference.UniversalTime)
        return FALSE;

    return (Reference.UniversalTime >= Start.UniversalTime) && (Reference.UniversalTime < End.UniversalTime);
}

VOID EtpPublishPowerGridList(
    _In_ HWND ListViewHandle,
    _In_ HWND SummaryHandle,
    _In_opt_ PPH_LIST List,
    _In_opt_ const ET_POWER_GRID_SUMMARY* Summary
    )
{
    ExtendedListView_SetRedraw(ListViewHandle, FALSE);
    EtpClearPowerGridListView(ListViewHandle);

    if (List)
    {
        for (ULONG i = 0; i < List->Count; i++)
        {
            PET_POWER_GRID_ENTRY entry = List->Items[i];
            LONG lvItemIndex;

            lvItemIndex = PhAddListViewItem(
                ListViewHandle,
                MAXINT,
                PhGetString(entry->Severity),
                entry
                );

            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 1, PhGetString(entry->LowImpact));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 2, PhGetString(entry->StartTime));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 3, PhGetString(entry->BlockDuration));
            PhSetListViewSubItem(ListViewHandle, lvItemIndex, 4, PhGetString(entry->TimeUntilStart));
        }
    }

    EtAddSummaryWindowText(SummaryHandle, Summary);

    ExtendedListView_SetRedraw(ListViewHandle, TRUE);
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS NTAPI EtpEnumeratePowerGridForecast(
    _In_ PVOID ThreadParameter
    )
{
    static PH_INITONCE initOnce = PH_INITONCE_INIT;
    static typeof(&RoInitialize) RoInitialize_I = NULL;
    static typeof(&RoUninitialize) RoUninitialize_I = NULL;
    static typeof(&CoUninitialize) CoUninitialize_I = NULL;
    HWND parentWindow = (HWND)ThreadParameter;
    HRESULT status;
    __FIVectorView_1_Windows__CDevices__CPower__CPowerGridData* powerGridDataVector = NULL;
    __x_ABI_CWindows_CDevices_CPower_CIPowerGridForecastStatics* powerGridForecastStatics = NULL;
    __x_ABI_CWindows_CDevices_CPower_CIPowerGridForecast* powerGridForecast = NULL;
    __x_ABI_CWindows_CFoundation_CDateTime startTime = { 0 };
    __x_ABI_CWindows_CFoundation_CTimeSpan blockDuration = { 0 };
    PPH_LIST powerGridList = NULL;
    UINT32 count = 0;
    WCHAR blockDurationBuffer[PH_TIMESPAN_STR_LEN_1];
    ET_POWER_GRID_SUMMARY summary = { 0 };
    HSTRING runtimeClassString = NULL;
    __x_ABI_CWindows_CFoundation_CDateTime nowTime = { 0 };
    LARGE_INTEGER nowSystemTime = { 0 };
    LARGE_INTEGER nowLarge = { 0 };

    if (PhBeginInitOnce(&initOnce))
    {
        PVOID baseAddress;

        if (baseAddress = PhLoadLibrary(L"combase.dll"))
        {
            RoInitialize_I = PhGetProcedureAddress(baseAddress, "RoInitialize", 0);
            RoUninitialize_I = PhGetProcedureAddress(baseAddress, "RoUninitialize", 0);
            CoUninitialize_I = PhGetProcedureAddress(baseAddress, "CoUninitialize", 0);
        }

        PhEndInitOnce(&initOnce);
    }

    if (!(RoInitialize_I && RoUninitialize_I && CoUninitialize_I))
        return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);

    {
        CoUninitialize_I();

        status = RoInitialize_I(RO_INIT_MULTITHREADED);

        if (HR_FAILED(status))
            return status;
    }

    status = PhGetActivationFactory(
        L"Windows.Energy.dll",
        RuntimeClass_Windows_Devices_Power_PowerGridForecast,
        &IID_IPowerGridForecastStatics,
        &powerGridForecastStatics
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    status = __x_ABI_CWindows_CDevices_CPower_CIPowerGridForecastStatics_GetForecast(
        powerGridForecastStatics,
        &powerGridForecast
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    status = __x_ABI_CWindows_CDevices_CPower_CIPowerGridForecast_get_StartTime(powerGridForecast, &startTime);

    if (HR_FAILED(status))
        goto CleanupExit;

    status = __x_ABI_CWindows_CDevices_CPower_CIPowerGridForecast_get_BlockDuration(powerGridForecast, &blockDuration);

    if (HR_FAILED(status))
        goto CleanupExit;

    status = __x_ABI_CWindows_CDevices_CPower_CIPowerGridForecast_get_Forecast(
        powerGridForecast,
        &powerGridDataVector
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    status = __FIVectorView_1_Windows__CDevices__CPower__CPowerGridData_get_Size(
        powerGridDataVector,
        &count
        );

    if (HR_FAILED(status))
        goto CleanupExit;

    powerGridList = PhCreateList(count);
    EtpFormatPowerGridDuration(blockDuration, blockDurationBuffer);

    summary.ForecastStart = startTime;
    summary.BlockDuration = blockDuration;
    summary.TotalBlocks = count;
    summary.BestLowSeverity = DBL_MAX;
    summary.LowestSeverity = DBL_MAX;
    summary.HighestSeverity = -DBL_MAX;

    PhQuerySystemTime(&nowSystemTime);
    nowTime.UniversalTime = nowSystemTime.QuadPart;

    for (UINT32 i = 0; i < count; i++)
    {
        __x_ABI_CWindows_CDevices_CPower_CIPowerGridData* powerGridData = NULL;
        __x_ABI_CWindows_CFoundation_CDateTime blockStart = startTime;
        PET_POWER_GRID_ENTRY entry;
        DOUBLE severity = 0;
        boolean isLowUserExperienceImpact = FALSE;
        WCHAR blockStartBuffer[PH_DATETIME_STR_LEN_1];

        status = __FIVectorView_1_Windows__CDevices__CPower__CPowerGridData_GetAt(
            powerGridDataVector,
            i,
            &powerGridData
            );

        if (HR_FAILED(status))
            continue;

        if (HR_FAILED(__x_ABI_CWindows_CDevices_CPower_CIPowerGridData_get_Severity(powerGridData, &severity)))
        {
            severity = 0;
        }

        if (HR_FAILED(__x_ABI_CWindows_CDevices_CPower_CIPowerGridData_get_IsLowUserExperienceImpact(powerGridData, &isLowUserExperienceImpact)))
        {
            isLowUserExperienceImpact = FALSE;
        }

        blockStart.UniversalTime += (INT64)i * blockDuration.Duration;

        entry = PhAllocateZero(sizeof(ET_POWER_GRID_ENTRY));
        entry->Severity = PhFormatString(L"%.2f", severity);
        entry->LowImpact = PhCreateString(isLowUserExperienceImpact ? L"Yes" : L"No");
        EtpFormatPowerGridDateTime(blockStart, blockStartBuffer, NULL);
        entry->StartTime = PhCreateString(blockStartBuffer);
        entry->BlockDuration = PhCreateString(blockDurationBuffer);
        entry->TimeUntilStart = EtpFormatRelativeTimeString(blockStart, nowTime);
        entry->SeverityValue = severity;
        entry->LowImpactFlag = isLowUserExperienceImpact;
        entry->BlockStartTime = blockStart;
        entry->BlockEndTime = EtpAddTicks(blockStart, blockDuration.Duration);
        entry->ActiveFlag = EtpIsPowerGridEntryActive(entry->BlockStartTime, entry->BlockEndTime, nowTime);
        PhAddItemList(powerGridList, entry);

        if (isLowUserExperienceImpact)
        {
            summary.LowImpactCount++;
        }

        if (severity < summary.BestLowSeverity && isLowUserExperienceImpact)
        {
            summary.BestLowSeverity = severity;
            summary.BestLowStart = blockStart;
        }

        if (severity < summary.LowestSeverity)
        {
            summary.LowestSeverity = severity;
            summary.LowestStart = blockStart;
        }

        if (severity > summary.HighestSeverity)
        {
            summary.HighestSeverity = severity;
            summary.HighestStart = blockStart;
        }

        summary.AverageSeverity += severity;

        __x_ABI_CWindows_CDevices_CPower_CIPowerGridData_Release(powerGridData);
    }

CleanupExit:

    if (powerGridDataVector)
        __FIVectorView_1_Windows__CDevices__CPower__CPowerGridData_Release(powerGridDataVector);
    if (powerGridForecast)
        __x_ABI_CWindows_CDevices_CPower_CIPowerGridForecast_Release(powerGridForecast);
    if (powerGridForecastStatics)
        __x_ABI_CWindows_CDevices_CPower_CIPowerGridForecastStatics_Release(powerGridForecastStatics);

    summary.AverageSeverity = summary.TotalBlocks ? summary.AverageSeverity / summary.TotalBlocks : 0.0;

    {
        PET_POWER_GRID_UPDATE update;

        update = PhAllocateZero(sizeof(ET_POWER_GRID_UPDATE));
        update->Entries = powerGridList;
        update->Summary = summary;

        if (!PostMessage(parentWindow, ET_WM_POWERGRID_UPDATE, status, (LPARAM)update))
        {
            EtpFreePowerGridList(powerGridList);
            PhFree(update);
        }
    }

    RoUninitialize_I();

    return STATUS_SUCCESS;
}

INT_PTR CALLBACK EtPowerGridDlgProc(
    _In_ HWND WindowHandle,
    _In_ UINT WindowMessage,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PPOWER_GRID_WINDOW_CONTEXT context;

    if (WindowMessage == WM_INITDIALOG)
    {
        context = (PPOWER_GRID_WINDOW_CONTEXT)lParam;
        PhSetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT, context);
    }
    else
    {
        context = PhGetWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);
    }

    if (context == NULL)
        return FALSE;

    switch (WindowMessage)
    {
    case WM_INITDIALOG:
        {
            context->WindowHandle = WindowHandle;
            context->ListViewHandle = GetDlgItem(WindowHandle, IDC_POWER_GRID_LIST);
            context->SummaryHandle = GetDlgItem(WindowHandle, IDC_POWER_GRID_SUMMARY);
            context->HeaderHandle = ListView_GetHeader(context->ListViewHandle);
            context->WindowFont = PhCreateApplicationFont(PhGetWindowDpi(WindowHandle));

            PhSetApplicationWindowIcon(WindowHandle);

            PhSetListViewStyle(context->ListViewHandle, TRUE, TRUE);
            PhSetControlTheme(context->ListViewHandle, L"explorer");
            PhSetExtendedListView(context->ListViewHandle);
            PhAddListViewColumn(context->ListViewHandle, 0, 0, 0, LVCFMT_LEFT, 200, L"Severity");
            PhAddListViewColumn(context->ListViewHandle, 1, 1, 1, LVCFMT_LEFT, 100, L"Low impact");
            PhAddListViewColumn(context->ListViewHandle, 2, 2, 2, LVCFMT_LEFT, 165, L"Start time");
            PhAddListViewColumn(context->ListViewHandle, 3, 3, 3, LVCFMT_LEFT, 110, L"Block duration");
            PhAddListViewColumn(context->ListViewHandle, 4, 4, 4, LVCFMT_LEFT, 140, L"Starts in");

            PhInitializeLayoutManager(&context->LayoutManager, WindowHandle);
            PhAddLayoutItem(&context->LayoutManager, context->ListViewHandle, NULL, PH_ANCHOR_ALL);
            PhAddLayoutItem(&context->LayoutManager, context->SummaryHandle, NULL, PH_ANCHOR_LEFT | PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDOK), NULL, PH_ANCHOR_RIGHT | PH_ANCHOR_BOTTOM);
            PhAddLayoutItem(&context->LayoutManager, GetDlgItem(WindowHandle, IDC_REFRESH), NULL, PH_ANCHOR_LEFT | PH_ANCHOR_BOTTOM);

            SetWindowFont(context->ListViewHandle, context->WindowFont, FALSE);
            SetWindowFont(context->SummaryHandle, context->WindowFont, FALSE);

            if (PhValidWindowPlacementFromSetting(SETTING_NAME_POWER_GRID_WINDOW_POSITION))
                PhLoadWindowPlacementFromSetting(SETTING_NAME_POWER_GRID_WINDOW_POSITION, SETTING_NAME_POWER_GRID_WINDOW_SIZE, WindowHandle);
            else
                PhCenterWindow(WindowHandle, context->ParentWindowHandle);

            PhLoadListViewColumnsFromSetting(SETTING_NAME_POWER_GRID_LISTVIEW_COLUMNS, context->ListViewHandle);

            PhCreateThread2(EtpEnumeratePowerGridForecast, WindowHandle);

            PhSetTimer(WindowHandle, PH_WINDOW_TIMER_DEFAULT, 1000, NULL);
        }
        break;
    case WM_DESTROY:
        {
            PhRemoveWindowContext(WindowHandle, PH_WINDOW_CONTEXT_DEFAULT);

            PhSaveWindowPlacementToSetting(SETTING_NAME_POWER_GRID_WINDOW_POSITION, SETTING_NAME_POWER_GRID_WINDOW_SIZE, WindowHandle);
            PhSaveListViewColumnsToSetting(SETTING_NAME_POWER_GRID_LISTVIEW_COLUMNS, context->ListViewHandle);

            PhDeleteLayoutManager(&context->LayoutManager);

            if (context->WindowFont) DeleteFont(context->WindowFont);

            PhFree(context);

            PostQuitMessage(0);
        }
        break;
    case WM_SIZE:
        {
            PhLayoutManagerLayout(&context->LayoutManager);

            InvalidateRect(context->ListViewHandle, NULL, FALSE);
        }
        break;
    case WM_DPICHANGED:
        {
            HFONT windowFont;

            if (windowFont = PhCreateApplicationFont(PhGetWindowDpi(WindowHandle)))
            {
                PhSwapReferenceFont(&context->WindowFont, context->ListViewHandle, windowFont, TRUE);
                SetWindowFont(context->SummaryHandle, context->WindowFont, TRUE);
            }

            PhLayoutManagerUpdate(&context->LayoutManager, LOWORD(wParam));
            PhLayoutManagerLayout(&context->LayoutManager);
        }
        break;
    case WM_TIMER:
        {
            switch (wParam)
            {
            case PH_WINDOW_TIMER_DEFAULT:
                {
                    LONG index = INT_ERROR;

                    while ((index = PhFindListViewItemByFlags(
                        context->ListViewHandle,
                        index,
                        LVNI_ALL
                        )) != INT_ERROR)
                    {
                        PET_POWER_GRID_ENTRY param;

                        if (PhGetListViewItemParam(context->ListViewHandle, index, &param))
                        {
                            __x_ABI_CWindows_CFoundation_CDateTime nowTime = { 0 };
                            LARGE_INTEGER nowSystemTime = { 0 };

                            PhQuerySystemTime(&nowSystemTime);
                            nowTime.UniversalTime = nowSystemTime.QuadPart;

                            param->ActiveFlag = EtpIsPowerGridEntryActive(param->BlockStartTime, param->BlockEndTime, nowTime);

                            PhMoveReference(&param->TimeUntilStart, EtpFormatRelativeTimeString(param->BlockStartTime, nowTime));

                            if (param->ActiveFlag)
                            {
                                PhSetListViewSubItem(context->ListViewHandle, index, 4, L"Active");
                            }
                            else
                            {
                                PhSetListViewSubItem(context->ListViewHandle, index, 4, PhGetString(param->TimeUntilStart));
                            }
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDCANCEL:
            case IDOK:
                DestroyWindow(WindowHandle);
                break;
            case IDC_REFRESH:
                {
                    EnableWindow(GET_WM_COMMAND_HWND(wParam, lParam), FALSE);

                    PhCreateThread2(EtpEnumeratePowerGridForecast, WindowHandle);
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            if (header->hwndFrom == context->ListViewHandle)
            {
                switch (header->code)
                {
                case NM_CUSTOMDRAW:
                    {
                        LPNMLVCUSTOMDRAW customDraw = (LPNMLVCUSTOMDRAW)lParam;

                        switch (customDraw->nmcd.dwDrawStage)
                        {
                        case CDDS_PREPAINT:
                            {
                                SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
                                return CDRF_NOTIFYITEMDRAW;
                            }
                            break;
                        case CDDS_ITEMPREPAINT:
                            {
                                PET_POWER_GRID_ENTRY item;
                                RECT rowRect;
                                RECT clientRect;
                                RECT progressRect;
                                double severity;
                                double effectiveSeverity;
                                BYTE red;
                                BYTE green;
                                COLORREF fillColor;
                                HBRUSH fillBrush;

                                if (customDraw->iSubItem > 0)
                                    return CDRF_DODEFAULT;

                                item = (PET_POWER_GRID_ENTRY)customDraw->nmcd.lItemlParam;
                                if (!item)
                                    return CDRF_DODEFAULT;

                                severity = item->SeverityValue;
                                if (severity < 0.0) severity = 0.0;
                                else if (severity > 1.0) severity = 1.0;

                                effectiveSeverity = severity;
                                if (item->LowImpactFlag)
                                    effectiveSeverity *= 0.65;

                                red = (BYTE)((0.15 + effectiveSeverity * 0.6) * 255.0);
                                green = (BYTE)((0.35 + (1.0 - effectiveSeverity) * 0.65) * 255.0);

                                if (item->LowImpactFlag)
                                    green = min(255, green + 20);

                                //RECT clientRect = { 0 };
                                // Set to LVIR_BOUNDS for the whole column, LVIR_LABEL for just text
                                // Get rect for item 0, LVIR_BOUNDS returns last column.
               
                                //if (item->ActiveFlag)
                                //{
                                //    // Use the full visible row width (entire list view client area).
                                //    GetClientRect(context->ListViewHandle, &clientRect);
                                //}
                                //else
                                //{
                                   ListView_GetSubItemRect(context->ListViewHandle, 0, 0, LVIR_LABEL, &clientRect);
                                //}

                                rowRect = customDraw->nmcd.rc;
                                rowRect.left = clientRect.left;
                                rowRect.right = clientRect.right;

                                progressRect = rowRect;
                                progressRect.right = progressRect.left + (LONG)((progressRect.right - progressRect.left) * severity);

                                if (progressRect.right > progressRect.left)
                                {
                                    fillColor = RGB(red, green, 0);
                                    fillBrush = CreateSolidBrush(fillColor);
                                    FillRect(customDraw->nmcd.hdc, &progressRect, fillBrush);
                                    DeleteBrush(fillBrush);
                                }

                                //if (item->ActiveFlag)
                                //{
                                //    ListView_GetSubItemRect(context->ListViewHandle, 0, 0, LVIR_BOUNDS, &clientRect);
                                //    rowRect = customDraw->nmcd.rc;
                                //    rowRect.left = clientRect.left;
                                //    rowRect.right = clientRect.right;
                                //    FillRect(customDraw->nmcd.hdc, &rowRect, GetSysColorBrush(COLOR_ACTIVEBORDER));
                                //}

                                customDraw->clrText = GetSysColor(COLOR_WINDOWTEXT);
                                customDraw->clrTextBk = CLR_NONE;

                                SetWindowLongPtr(WindowHandle, DWLP_MSGRESULT, CDRF_NEWFONT);
                                return CDRF_NEWFONT;
                            }
                        }
                    }
                    break;
                case LVN_KEYDOWN:
                    {
                        LPNMLVKEYDOWN keyDown = (LPNMLVKEYDOWN)lParam;

                        switch (keyDown->wVKey)
                        {
                        case 'C':
                            {
                                if (PhGetKeyState(VK_CONTROL))
                                {
                                    PhCopyListView(context->ListViewHandle);
                                }
                            }
                            break;
                        }
                    }
                    break;
                }
            }
            else if (header && header->hwndFrom == context->HeaderHandle)
            {
                // Redraw row progress backgrounds while/after column resize.

                switch (header->code)
                {
                case HDN_TRACKW:
                case HDN_TRACKA:
                case HDN_ENDTRACKW:
                case HDN_ENDTRACKA:
                case HDN_ITEMCHANGEDW:
                case HDN_ITEMCHANGEDA:
                    InvalidateRect(context->ListViewHandle, NULL, FALSE);
                    break;
                }
            }

            REFLECT_MESSAGE_DLG(WindowHandle, context->ListViewHandle, WindowMessage, wParam, lParam);
        }
        break;

    case ET_WM_POWERGRID_UPDATE:
        {
            PET_POWER_GRID_UPDATE update = (PET_POWER_GRID_UPDATE)lParam;

            EnableWindow(GetDlgItem(WindowHandle, IDC_REFRESH), TRUE);

            SendMessage(context->ListViewHandle, WM_SETREDRAW, FALSE, 0);

            EtpClearPowerGridListView(context->ListViewHandle);

            EtpPublishPowerGridList(context->ListViewHandle, context->SummaryHandle, update->Entries, &update->Summary);

            //EtpFreePowerGridList(update->Entries);

            SendMessage(context->ListViewHandle, WM_SETREDRAW, TRUE, 0);

            PhFree(update);
        }
        break;
    }

    return FALSE;
}

_Function_class_(USER_THREAD_START_ROUTINE)
NTSTATUS EtPowerGridDialogThreadStart(
    _In_ PVOID Parameter
    )
{
    PPOWER_GRID_WINDOW_CONTEXT context = Parameter;
    BOOL result;
    MSG message;
    HWND windowHandle;
    PH_AUTO_POOL autoPool;

    PhInitializeAutoPool(&autoPool);

    windowHandle = PhCreateDialog(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_POWER_GRID),
        !!PhGetIntegerSetting(SETTING_FORCE_NO_PARENT) ? NULL : context->ParentWindowHandle,
        EtPowerGridDlgProc,
        context
        );

    ShowWindow(windowHandle, SW_SHOW);
    SetForegroundWindow(windowHandle);

    while (result = GetMessage(&message, NULL, 0, 0))
    {
        if (result == INT_ERROR)
            break;

        if (!IsDialogMessage(windowHandle, &message))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        PhDrainAutoPool(&autoPool);
    }

    PhDeleteAutoPool(&autoPool);

    return STATUS_SUCCESS;
}

VOID EtShowPowerGridDialog(
    _In_ HWND ParentWindowHandle
    )
{
    PPOWER_GRID_WINDOW_CONTEXT context;

    context = PhAllocateZero(sizeof(POWER_GRID_WINDOW_CONTEXT));
    context->ParentWindowHandle = ParentWindowHandle;

    PhCreateThread2(EtPowerGridDialogThreadStart, context);
}
