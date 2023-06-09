/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2013
 *     dmex    2011-2022
 *
 */

#include "toolstatus.h"

HWND StatusBarHandle = NULL;
ULONG StatusBarMaxWidths[MAX_STATUSBAR_ITEMS];
PPH_LIST StatusBarItemList = NULL;
ULONG StatusBarItems[MAX_STATUSBAR_ITEMS] =
{
    // Default items (displayed)
    ID_STATUS_CPUUSAGE,
    ID_STATUS_PHYSICALMEMORY,
    ID_STATUS_FREEMEMORY,
    // Available items (hidden)
    ID_STATUS_COMMITCHARGE,
    ID_STATUS_NUMBEROFPROCESSES,
    ID_STATUS_NUMBEROFTHREADS,
    ID_STATUS_NUMBEROFHANDLES,
    ID_STATUS_NUMBEROFVISIBLEITEMS,
    ID_STATUS_NUMBEROFSELECTEDITEMS,
    ID_STATUS_INTERVALSTATUS,
    ID_STATUS_IO_RO,
    ID_STATUS_IO_W,
    ID_STATUS_MAX_CPU_PROCESS,
    ID_STATUS_MAX_IO_PROCESS,
    ID_STATUS_SELECTEDWORKINGSET,
    ID_STATUS_SELECTEDPRIVATEBYTES,
};

VOID StatusBarLoadDefault(
    VOID
    )
{
    if (!StatusBarItemList)
        StatusBarItemList = PhCreateList(MAX_DEFAULT_STATUSBAR_ITEMS);

    for (ULONG i = 0; i < MAX_DEFAULT_STATUSBAR_ITEMS; i++)
    {
        PhAddItemList(StatusBarItemList, UlongToPtr(StatusBarItems[i]));
    }
}

VOID StatusBarLoadSettings(
    VOID
    )
{
    LONG64 buttonCount = 0;
    PPH_STRING settingsString;
    PH_STRINGREF remaining;
    PH_STRINGREF part;

    settingsString = PhGetStringSetting(SETTING_NAME_STATUSBAR_CONFIG);

    if (PhIsNullOrEmptyString(settingsString))
    {
        // Load default settings
        StatusBarLoadDefault();
        goto CleanupExit;
    }

    remaining = PhGetStringRef(settingsString);

    // Query the number of buttons to insert
    if (!PhSplitStringRefAtChar(&remaining, L'|', &part, &remaining))
    {
        // Load default settings
        StatusBarLoadDefault();
        goto CleanupExit;
    }

    if (!PhStringToInteger64(&part, 10, &buttonCount))
    {
        // Load default settings
        StatusBarLoadDefault();
        goto CleanupExit;
    }

    StatusBarItemList = PhCreateList((ULONG)buttonCount);

    for (ULONG i = 0; i < (ULONG)buttonCount; i++)
    {
        PH_STRINGREF idPart;
        ULONG64 idInteger;

        if (remaining.Length == 0)
            break;

        PhSplitStringRefAtChar(&remaining, L'|', &idPart, &remaining);

        if (PhStringToInteger64(&idPart, 10, &idInteger))
        {
            PhInsertItemList(StatusBarItemList, i, UlongToPtr((ULONG)idInteger));
        }
    }

CleanupExit:
    PhClearReference(&settingsString);
}

VOID StatusBarSaveSettings(
    VOID
    )
{
    PPH_STRING settingsString;
    PH_STRING_BUILDER stringBuilder;

    PhInitializeStringBuilder(&stringBuilder, 100);

    PhAppendFormatStringBuilder(
        &stringBuilder,
        L"%lu|",
        StatusBarItemList->Count
        );

    for (ULONG i = 0; i < StatusBarItemList->Count; i++)
    {
        PhAppendFormatStringBuilder(
            &stringBuilder,
            L"%lu|",
            PtrToUlong(StatusBarItemList->Items[i])
            );
    }

    if (stringBuilder.String->Length != 0)
        PhRemoveEndStringBuilder(&stringBuilder, 1);

    settingsString = PH_AUTO(PhFinalStringBuilderString(&stringBuilder));
    PhSetStringSetting2(SETTING_NAME_STATUSBAR_CONFIG, &settingsString->sr);
}

VOID StatusBarResetSettings(
    VOID
    )
{
    PhClearList(StatusBarItemList);

    StatusBarLoadDefault();
}

PWSTR StatusBarGetText(
    _In_ ULONG CommandID
    )
{
    switch (CommandID)
    {
    case ID_STATUS_CPUUSAGE:
        return L"CPU usage";
    case ID_STATUS_PHYSICALMEMORY:
        return L"Physical memory";
    case ID_STATUS_NUMBEROFPROCESSES:
        return L"Number of processes";
    case ID_STATUS_COMMITCHARGE:
        return L"Commit charge";
    case ID_STATUS_FREEMEMORY:
        return L"Free physical memory";
    case ID_STATUS_NUMBEROFTHREADS:
        return L"Number of threads";
    case ID_STATUS_NUMBEROFHANDLES:
        return L"Number of handles";
    case ID_STATUS_NUMBEROFVISIBLEITEMS:
        return L"Number of visible items";
    case ID_STATUS_NUMBEROFSELECTEDITEMS:
        return L"Number of selected items";
    case ID_STATUS_INTERVALSTATUS:
        return L"Interval status";
    case ID_STATUS_IO_RO:
        return L"I/O read+other";
    case ID_STATUS_IO_W:
        return L"I/O write";
    case ID_STATUS_MAX_CPU_PROCESS:
        return L"Max. CPU process";
    case ID_STATUS_MAX_IO_PROCESS:
        return L"Max. I/O process";
    case ID_STATUS_SELECTEDWORKINGSET:
        return L"Selected process WS";
    case ID_STATUS_SELECTEDPRIVATEBYTES:
        return L"Selected process private bytes";
    }

    return L"ERROR";
}

VOID StatusBarShowMenu(
    _In_ HWND WindowHandle
    )
{
    PPH_EMENU menu;
    PPH_EMENU_ITEM menuItem;
    PPH_EMENU_ITEM selectedItem;
    POINT cursorPos;

    GetCursorPos(&cursorPos);

    menu = PhCreateEMenu();
    menuItem = PhCreateEMenuItem(0, COMMAND_ID_ENABLE_SEARCHBOX, L"Customize...", NULL, NULL);
    PhInsertEMenuItem(menu, menuItem, ULONG_MAX);

    selectedItem = PhShowEMenu(
        menu,
        WindowHandle,
        PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_BOTTOM,
        cursorPos.x,
        cursorPos.y
        );

    if (selectedItem && selectedItem->Id != ULONG_MAX)
    {
        StatusBarShowCustomizeDialog(WindowHandle);

        StatusBarUpdate(TRUE);
    }

    PhDestroyEMenu(menu);
}

VOID StatusBarUpdate(
    _In_ BOOLEAN ResetMaxWidths
    )
{
    static ULONG64 lastTickCount = 0;
    ULONG count;
    ULONG i;
    HDC hdc;
    BOOLEAN resetMaxWidths = FALSE;
    ULONG widths[MAX_STATUSBAR_ITEMS];
    SIZE_T textLength[MAX_STATUSBAR_ITEMS];
    WCHAR text[MAX_STATUSBAR_ITEMS][0x80];

    if (ResetMaxWidths)
        resetMaxWidths = TRUE;

    if (!StatusBarItemList || StatusBarItemList->Count == 0)
    {
        // The status bar doesn't cope well with 0 parts.
        widths[0] = -1;
        SendMessage(StatusBarHandle, SB_SETPARTS, 1, (LPARAM)widths);
        SendMessage(StatusBarHandle, SB_SETTEXT, 0, (LPARAM)L"");
        return;
    }

    hdc = GetDC(StatusBarHandle);
    SelectFont(hdc, GetWindowFont(StatusBarHandle));

    // Reset max. widths for Max. CPU Process and Max. I/O Process parts once in a while.
    {
        ULONG64 tickCount;

        tickCount = NtGetTickCount64();

        if (tickCount - lastTickCount >= 10 * 1000)
        {
            resetMaxWidths = TRUE;
            lastTickCount = tickCount;
        }
    }

    count = 0;
    memset(text, 0, sizeof(text));
    memset(textLength, 0, sizeof(textLength));

    for (i = 0; i < StatusBarItemList->Count; i++)
    {
        SIZE size;
        ULONG width;
        SIZE_T length;

        switch (PtrToUlong(StatusBarItemList->Items[i]))
        {
        case ID_STATUS_CPUUSAGE:
            {
                FLOAT cpuUsage = SystemStatistics.CpuKernelUsage + SystemStatistics.CpuUserUsage;
                PH_FORMAT format[3];

                PhInitFormatS(&format[0], L"CPU usage: ");
                PhInitFormatF(&format[1], cpuUsage * 100, 2);
                PhInitFormatS(&format[2], L"%");

                PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), &textLength[count]);
            }
            break;
        case ID_STATUS_COMMITCHARGE:
            {
                ULONG commitUsage;
                FLOAT commitFraction;
                PH_FORMAT format[5];

                if (!SystemStatistics.Performance)
                    break;

                commitUsage = SystemStatistics.Performance->CommittedPages;
                commitFraction = (FLOAT)commitUsage / SystemStatistics.Performance->CommitLimit * 100;

                PhInitFormatS(&format[0], L"Commit charge: ");
                PhInitFormatSize(&format[1], UInt32x32To64(commitUsage, PAGE_SIZE));
                PhInitFormatS(&format[2], L" (");
                PhInitFormatF(&format[3], commitFraction, 2);
                PhInitFormatS(&format[4], L"%)");

                PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), &textLength[count]);
            }
            break;
        case ID_STATUS_PHYSICALMEMORY:
            {
                ULONG physicalUsage;
                FLOAT physicalFraction;
                PH_FORMAT format[5];

                if (!SystemStatistics.Performance)
                    break;

                physicalUsage = PhSystemBasicInformation.NumberOfPhysicalPages - SystemStatistics.Performance->AvailablePages;
                physicalFraction = (FLOAT)physicalUsage / PhSystemBasicInformation.NumberOfPhysicalPages * 100;

                PhInitFormatS(&format[0], L"Physical memory: ");
                PhInitFormatSize(&format[1], UInt32x32To64(physicalUsage, PAGE_SIZE));
                PhInitFormatS(&format[2], L" (");
                PhInitFormatF(&format[3], physicalFraction, 2);
                PhInitFormatS(&format[4], L"%)");

                PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), &textLength[count]);
            }
            break;
        case ID_STATUS_FREEMEMORY:
            {
                ULONG physicalFree;
                FLOAT physicalFreeFraction;
                PH_FORMAT format[5];

                if (!SystemStatistics.Performance)
                    break;

                physicalFree = SystemStatistics.Performance->AvailablePages;
                physicalFreeFraction = (FLOAT)physicalFree / PhSystemBasicInformation.NumberOfPhysicalPages * 100;

                PhInitFormatS(&format[0], L"Free memory: ");
                PhInitFormatSize(&format[1], UInt32x32To64(physicalFree, PAGE_SIZE));
                PhInitFormatS(&format[2], L" (");
                PhInitFormatF(&format[3], physicalFreeFraction, 2);
                PhInitFormatS(&format[4], L"%)");

                PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), &textLength[count]);
            }
            break;
        case ID_STATUS_NUMBEROFPROCESSES:
            {
                PH_FORMAT format[2];

                PhInitFormatS(&format[0], L"Processes: ");
                PhInitFormatI64UGroupDigits(&format[1], SystemStatistics.NumberOfProcesses);

                PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), &textLength[count]);
            }
            break;
        case ID_STATUS_NUMBEROFTHREADS:
            {
                PH_FORMAT format[2];

                PhInitFormatS(&format[0], L"Threads: ");
                PhInitFormatI64UGroupDigits(&format[1], SystemStatistics.NumberOfThreads);

                PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), &textLength[count]);
            }
            break;
        case ID_STATUS_NUMBEROFHANDLES:
            {
                PH_FORMAT format[2];

                PhInitFormatS(&format[0], L"Handles: ");
                PhInitFormatI64UGroupDigits(&format[1], SystemStatistics.NumberOfHandles);

                PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), &textLength[count]);
            }
            break;
        case ID_STATUS_IO_RO:
            {
                PH_FORMAT format[2];

                PhInitFormatS(&format[0], L"I/O R+O: ");
                PhInitFormatSize(&format[1], (SystemStatistics.IoReadDelta.Delta + SystemStatistics.IoOtherDelta.Delta));

                PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), &textLength[count]);
            }
            break;
        case ID_STATUS_IO_W:
            {
                PH_FORMAT format[2];

                PhInitFormatS(&format[0], L"I/O W: ");
                PhInitFormatSize(&format[1], SystemStatistics.IoWriteDelta.Delta);

                PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), &textLength[count]);
            }
            break;
        case ID_STATUS_MAX_CPU_PROCESS:
            {
                PPH_PROCESS_ITEM processItem;

                if (SystemStatistics.MaxCpuProcessId && (processItem = PhReferenceProcessItem(SystemStatistics.MaxCpuProcessId)))
                {
                    if (!PH_IS_FAKE_PROCESS_ID(processItem->ProcessId))
                    {
                        PH_FORMAT format[6];

                        PhInitFormatSR(&format[0], processItem->ProcessName->sr);
                        PhInitFormatS(&format[1], L" (");
                        PhInitFormatI64U(&format[2], HandleToUlong(processItem->ProcessId));
                        PhInitFormatS(&format[3], L"): ");
                        PhInitFormatF(&format[4], processItem->CpuUsage * 100, 2);
                        PhInitFormatS(&format[5], L"%");

                        PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), &textLength[count]);
                    }
                    else
                    {
                        PH_FORMAT format[4];

                        PhInitFormatSR(&format[0], processItem->ProcessName->sr);
                        PhInitFormatS(&format[1], L": ");
                        PhInitFormatF(&format[2], processItem->CpuUsage * 100, 2);
                        PhInitFormatS(&format[3], L"%)");

                        PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), &textLength[count]);
                    }

                    PhDereferenceObject(processItem);
                }
                else
                {
                    PH_FORMAT format[1];

                    PhInitFormatS(&format[0], L"-");

                    PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), &textLength[count]);
                }
            }
            break;
        case ID_STATUS_MAX_IO_PROCESS:
            {
                PPH_PROCESS_ITEM processItem;

                if (SystemStatistics.MaxIoProcessId && (processItem = PhReferenceProcessItem(SystemStatistics.MaxIoProcessId)))
                {
                    if (!PH_IS_FAKE_PROCESS_ID(processItem->ProcessId))
                    {
                        PH_FORMAT format[5];

                        PhInitFormatSR(&format[0], processItem->ProcessName->sr);
                        PhInitFormatS(&format[1], L" (");
                        PhInitFormatI64U(&format[2], HandleToUlong(processItem->ProcessId));
                        PhInitFormatS(&format[3], L"): ");
                        PhInitFormatSize(&format[4], processItem->IoReadDelta.Delta + processItem->IoWriteDelta.Delta + processItem->IoOtherDelta.Delta);

                        PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), &textLength[count]);
                    }
                    else
                    {
                        PH_FORMAT format[3];

                        PhInitFormatSR(&format[0], processItem->ProcessName->sr);
                        PhInitFormatS(&format[1], L": ");
                        PhInitFormatSize(&format[2], processItem->IoReadDelta.Delta + processItem->IoWriteDelta.Delta + processItem->IoOtherDelta.Delta);

                        PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), &textLength[count]);
                    }

                    PhDereferenceObject(processItem);
                }
                else
                {
                    PH_FORMAT format[1];

                    PhInitFormatS(&format[0], L"-");

                    PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), &textLength[count]);
                }
            }
            break;
        case ID_STATUS_NUMBEROFVISIBLEITEMS:
            {
                HWND tnHandle;

                if (tnHandle = GetCurrentTreeNewHandle())
                {
                    PH_FORMAT format[2];

                    PhInitFormatS(&format[0], L"Visible: ");
                    PhInitFormatI64UGroupDigits(&format[1], TreeNew_GetFlatNodeCount(tnHandle));

                    PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), &textLength[count]);
                }
                else
                {
                    PH_FORMAT format[1];

                    PhInitFormatS(&format[0], L"Visible: N/A");

                    PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), &textLength[count]);
                }
            }
            break;
        case ID_STATUS_NUMBEROFSELECTEDITEMS:
            {
                HWND tnHandle;

                if (tnHandle = GetCurrentTreeNewHandle())
                {
                    ULONG j;
                    ULONG visibleCount;
                    ULONG selectedCount;
                    PH_FORMAT format[2];

                    visibleCount = TreeNew_GetFlatNodeCount(tnHandle);
                    selectedCount = 0;

                    for (j = 0; j < visibleCount; j++)
                    {
                        if (TreeNew_GetFlatNode(tnHandle, j)->Selected)
                            selectedCount++;
                    }

                    PhInitFormatS(&format[0], L"Selected: ");
                    PhInitFormatI64UGroupDigits(&format[1], selectedCount);

                    PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), &textLength[count]);
                }
                else
                {
                    PH_FORMAT format[1];

                    PhInitFormatS(&format[0], L"Selected: N/A");

                    PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), &textLength[count]);
                }
            }
            break;
        case ID_STATUS_INTERVALSTATUS:
            {
                PH_FORMAT format[1];

                if (UpdateAutomatically)
                {
                    switch (PhGetIntegerSetting(L"UpdateInterval"))
                    {
                    case 500:
                        PhInitFormatS(&format[0], L"Interval: Fast");
                        break;
                    case 1000:
                        PhInitFormatS(&format[0], L"Interval: Normal");
                        break;
                    case 2000:
                        PhInitFormatS(&format[0], L"Interval: Below normal");
                        break;
                    case 5000:
                        PhInitFormatS(&format[0], L"Interval: Slow");
                        break;
                    case 10000:
                        PhInitFormatS(&format[0], L"Interval: Very slow");
                        break;
                    default:
                        PhInitFormatS(&format[0], L"Interval: N/A");
                        break;
                    }
                }
                else
                {
                    PhInitFormatS(&format[0], L"Interval: Paused");
                }

                PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), &textLength[count]);
            }
            break;
        case ID_STATUS_SELECTEDWORKINGSET:
            {
                PPH_PROCESS_ITEM* processes;
                ULONG numberOfProcesses;
                SIZE_T value = 0;
                PH_FORMAT format[2];

                PhGetSelectedAndPropagateProcessItems(&processes, &numberOfProcesses);
                PhReferenceObjects(processes, numberOfProcesses);

                for (ULONG j = 0; j < numberOfProcesses; j++)
                {
                    value += processes[j]->VmCounters.WorkingSetSize;
                }

                if (value)
                {
                    PhInitFormatS(&format[0], L"Selected WS: ");
                    PhInitFormatSize(&format[1], value);
                    PhFormatToBuffer(format, 2, text[count], sizeof(text[count]), &textLength[count]);
                }
                else
                {
                    PhInitFormatS(&format[0], L"Selected WS: N/A");
                    PhFormatToBuffer(format, 1, text[count], sizeof(text[count]), &textLength[count]);
                }

                PhDereferenceObjects(processes, numberOfProcesses);
                PhFree(processes);
            }
            break;
        case ID_STATUS_SELECTEDPRIVATEBYTES:
            {
                PPH_PROCESS_ITEM* processes;
                ULONG numberOfProcesses;
                SIZE_T value = 0;
                PH_FORMAT format[2];

                PhGetSelectedAndPropagateProcessItems(&processes, &numberOfProcesses);
                PhReferenceObjects(processes, numberOfProcesses);

                for (ULONG j = 0; j < numberOfProcesses; j++)
                {
                    value += processes[j]->VmCounters.PagefileUsage;
                }

                if (value)
                {
                    PhInitFormatS(&format[0], L"Selected private bytes: ");
                    PhInitFormatSize(&format[1], value);
                    PhFormatToBuffer(format, 2, text[count], sizeof(text[count]), &textLength[count]);
                }
                else
                {
                    PhInitFormatS(&format[0], L"Selected private bytes: N/A");
                    PhFormatToBuffer(format, 1, text[count], sizeof(text[count]), &textLength[count]);
                }

                PhDereferenceObjects(processes, numberOfProcesses);
                PhFree(processes);
            }
            break;
        }

        if (textLength[count] > sizeof(UNICODE_NULL))
            length = textLength[count] - sizeof(UNICODE_NULL);
        else
            length = 0;

        if (!GetTextExtentPoint32(hdc, text[count], (UINT)(length / sizeof(WCHAR)), &size))
            size.cx = 200;

        if (count != 0)
            widths[count] = widths[count - 1];
        else
            widths[count] = 0;

        width = size.cx + 10;

        if (resetMaxWidths)
            StatusBarMaxWidths[count] = 0;

        if (width <= StatusBarMaxWidths[count])
            width = StatusBarMaxWidths[count];
        else
            StatusBarMaxWidths[count] = width;

        widths[count] += width;

        count++;
    }

    ReleaseDC(StatusBarHandle, hdc);

    // Note: Suspend redrawing until after updating the statusbar text
    // otherwise SB_SETTEXT repaints multiple times during our loop. (dmex)
    SendMessage(StatusBarHandle, WM_SETREDRAW, FALSE, 0);
    SendMessage(StatusBarHandle, SB_SETPARTS, count, (LPARAM)widths);

    for (i = 0; i < count; i++)
    {
        SendMessage(StatusBarHandle, SB_SETTEXT, i, (LPARAM)text[i]);
    }

    SendMessage(StatusBarHandle, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(StatusBarHandle, NULL, TRUE);
}
