/*
 * Process Hacker ToolStatus -
 *   statusbar main
 *
 * Copyright (C) 2011-2016 dmex
 * Copyright (C) 2010-2013 wj32
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

#include "toolstatus.h"

HWND StatusBarHandle = NULL;
ULONG ProcessesUpdatedCount = 0;
ULONG StatusBarMaxWidths[MAX_STATUSBAR_ITEMS];
// Note: no lock is needed because we only ever modify the list on this same thread.
PPH_LIST StatusBarItemList = NULL;
ULONG StatusBarItems[MAX_STATUSBAR_ITEMS] =
{
    // Default items (displayed)
    { ID_STATUS_CPUUSAGE },
    { ID_STATUS_PHYSICALMEMORY },
    { ID_STATUS_NUMBEROFPROCESSES },
    // Available items (hidden)
    { ID_STATUS_COMMITCHARGE },
    { ID_STATUS_FREEMEMORY },
    { ID_STATUS_NUMBEROFTHREADS },
    { ID_STATUS_NUMBEROFHANDLES },
    { ID_STATUS_NUMBEROFVISIBLEITEMS,  },
    { ID_STATUS_NUMBEROFSELECTEDITEMS,  },
    { ID_STATUS_INTERVALSTATUS },
    { ID_STATUS_IO_RO },
    { ID_STATUS_IO_W },
    { ID_STATUS_MAX_CPU_PROCESS },
    { ID_STATUS_MAX_IO_PROCESS },
};

VOID StatusBarLoadDefault(
    VOID
    )
{
    if (!StatusBarItemList)
        StatusBarItemList = PhCreateList(MAX_DEFAULT_STATUSBAR_ITEMS);

    for (ULONG i = 0; i < MAX_DEFAULT_STATUSBAR_ITEMS; i++)
    {
        PSTATUSBAR_ITEM statusItem;

        statusItem = PhAllocate(sizeof(STATUSBAR_ITEM));
        memset(statusItem, 0, sizeof(STATUSBAR_ITEM));

        statusItem->Id = StatusBarItems[i];

        PhAddItemList(StatusBarItemList, statusItem);
    }
}

VOID StatusBarLoadSettings(
    VOID
    )
{
    ULONG64 buttonCount = 0;
    PPH_STRING settingsString;
    PH_STRINGREF remaining;
    PH_STRINGREF part;

    settingsString = PhaGetStringSetting(SETTING_NAME_STATUSBAR_CONFIG);
    remaining = settingsString->sr;

    if (remaining.Length == 0)
    {
        // Load default settings
        StatusBarLoadDefault();
        return;
    }

    // Query the number of buttons to insert
    if (!PhSplitStringRefAtChar(&remaining, '|', &part, &remaining))
    {
        // Load default settings
        StatusBarLoadDefault();
        return;
    }

    if (!PhStringToInteger64(&part, 10, &buttonCount))
    {
        // Load default settings
        StatusBarLoadDefault();
        return;
    }

    StatusBarItemList = PhCreateList((ULONG)buttonCount);

    for (ULONG i = 0; i < (ULONG)buttonCount; i++)
    {
        PH_STRINGREF idPart;
        ULONG64 idInteger;

        if (remaining.Length == 0)
            break;

        PhSplitStringRefAtChar(&remaining, '|', &idPart, &remaining);

        if (PhStringToInteger64(&idPart, 10, &idInteger))
        {
            PSTATUSBAR_ITEM statusItem;

            statusItem = PhAllocate(sizeof(STATUSBAR_ITEM));
            memset(statusItem, 0, sizeof(STATUSBAR_ITEM));

            statusItem->Id = (ULONG)idInteger;

            PhInsertItemList(StatusBarItemList, i, statusItem);
        }
    }
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
        PSTATUSBAR_ITEM statusItem = StatusBarItemList->Items[i];

        PhAppendFormatStringBuilder(
            &stringBuilder,
            L"%lu|",
            statusItem->Id
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
    for (ULONG i = 0; i < StatusBarItemList->Count; i++)
    {
        PhFree(StatusBarItemList->Items[i]);
    }

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
    }

    return L"ERROR";
}

VOID StatusBarShowMenu(
    VOID
    )
{
    PPH_EMENU menu;
    PPH_EMENU_ITEM selectedItem;
    POINT cursorPos;

    GetCursorPos(&cursorPos);

    menu = PhCreateEMenu();
    PhInsertEMenuItem(menu, PhCreateEMenuItem(0, COMMAND_ID_ENABLE_SEARCHBOX, L"Customize...", NULL, NULL), -1);

    selectedItem = PhShowEMenu(
        menu,
        PhMainWndHandle,
        PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_BOTTOM,
        cursorPos.x,
        cursorPos.y
        );

    if (selectedItem && selectedItem->Id != -1)
    {
        StatusBarShowCustomizeDialog();

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
    PPH_STRING text[MAX_STATUSBAR_ITEMS];
    ULONG widths[MAX_STATUSBAR_ITEMS];

    if (ProcessesUpdatedCount < 2)
        return;

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
    SelectObject(hdc, (HFONT)SendMessage(StatusBarHandle, WM_GETFONT, 0, 0));

    // Reset max. widths for Max. CPU Process and Max. I/O Process parts once in a while.
    {
        LARGE_INTEGER tickCount;

        PhQuerySystemTime(&tickCount);

        if (tickCount.QuadPart - lastTickCount >= 10 * PH_TICKS_PER_SEC)
        {
            resetMaxWidths = TRUE;
            lastTickCount = tickCount.QuadPart;
        }
    }

    count = 0;

    for (i = 0; i < StatusBarItemList->Count; i++)
    {
        SIZE size;
        ULONG width;
        PSTATUSBAR_ITEM statusItem;

        statusItem = StatusBarItemList->Items[i];

        switch (statusItem->Id)
        {
        case ID_STATUS_CPUUSAGE:
            {
                text[count] = PhFormatString(
                    L"CPU Usage: %.2f%%",
                    (SystemStatistics.CpuKernelUsage + SystemStatistics.CpuUserUsage) * 100
                    );
            }
            break;
        case ID_STATUS_COMMITCHARGE:
            {
                ULONG commitUsage = SystemStatistics.Performance->CommittedPages;
                FLOAT commitFraction = (FLOAT)commitUsage / SystemStatistics.Performance->CommitLimit * 100;

                text[count] = PhFormatString(
                    L"Commit charge: %s (%.2f%%)",
                    PhaFormatSize(UInt32x32To64(commitUsage, PAGE_SIZE), -1)->Buffer,
                    commitFraction
                    );
            }
            break;
        case ID_STATUS_PHYSICALMEMORY:
            {
                ULONG physicalUsage = PhSystemBasicInformation.NumberOfPhysicalPages - SystemStatistics.Performance->AvailablePages;
                FLOAT physicalFraction = (FLOAT)physicalUsage / PhSystemBasicInformation.NumberOfPhysicalPages * 100;

                text[count] = PhFormatString(
                    L"Physical memory: %s (%.2f%%)",
                    PhaFormatSize(UInt32x32To64(physicalUsage, PAGE_SIZE), -1)->Buffer,
                    physicalFraction
                    );
            }
            break;
        case ID_STATUS_FREEMEMORY:
            {
                ULONG physicalFree = SystemStatistics.Performance->AvailablePages;
                FLOAT physicalFreeFraction = (FLOAT)physicalFree / PhSystemBasicInformation.NumberOfPhysicalPages * 100;

                text[count] = PhFormatString(
                    L"Free memory: %s (%.2f%%)",
                    PhaFormatSize(UInt32x32To64(physicalFree, PAGE_SIZE), -1)->Buffer,
                    physicalFreeFraction
                    );
            }
            break;
        case ID_STATUS_NUMBEROFPROCESSES:
            {
                text[count] = PhConcatStrings2(
                    L"Processes: ",
                    PhaFormatUInt64(SystemStatistics.NumberOfProcesses, TRUE)->Buffer
                    );
            }
            break;
        case ID_STATUS_NUMBEROFTHREADS:
            {
                text[count] = PhConcatStrings2(
                    L"Threads: ",
                    PhaFormatUInt64(SystemStatistics.NumberOfThreads, TRUE)->Buffer
                    );
            }
            break;
        case ID_STATUS_NUMBEROFHANDLES:
            {
                text[count] = PhConcatStrings2(
                    L"Handles: ",
                    PhaFormatUInt64(SystemStatistics.NumberOfHandles, TRUE)->Buffer
                    );
            }
            break;
        case ID_STATUS_IO_RO:
            {
                text[count] = PhConcatStrings2(
                    L"I/O R+O: ",
                    PhaFormatSize(SystemStatistics.IoReadDelta.Delta + SystemStatistics.IoOtherDelta.Delta, -1)->Buffer
                    );
            }
            break;
        case ID_STATUS_IO_W:
            {
                text[count] = PhConcatStrings2(
                    L"I/O W: ",
                    PhaFormatSize(SystemStatistics.IoWriteDelta.Delta, -1)->Buffer
                    );
            }
            break;
        case ID_STATUS_MAX_CPU_PROCESS:
            {
                PPH_PROCESS_ITEM processItem;

                if (SystemStatistics.MaxCpuProcessId && (processItem = PhReferenceProcessItem(SystemStatistics.MaxCpuProcessId)))
                {
                    if (!PH_IS_FAKE_PROCESS_ID(processItem->ProcessId))
                    {
                        text[count] = PhFormatString(
                            L"%s (%lu): %.2f%%",
                            processItem->ProcessName->Buffer,
                            HandleToUlong(processItem->ProcessId),
                            processItem->CpuUsage * 100
                            );
                    }
                    else
                    {
                        text[count] = PhFormatString(
                            L"%s: %.2f%%",
                            processItem->ProcessName->Buffer,
                            processItem->CpuUsage * 100
                            );
                    }

                    PhDereferenceObject(processItem);
                }
                else
                {
                    text[count] = PhCreateString(L"-");
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
                        text[count] = PhFormatString(
                            L"%s (%lu): %s",
                            processItem->ProcessName->Buffer,
                            HandleToUlong(processItem->ProcessId),
                            PhaFormatSize(processItem->IoReadDelta.Delta + processItem->IoWriteDelta.Delta + processItem->IoOtherDelta.Delta, -1)->Buffer
                            );
                    }
                    else
                    {
                        text[count] = PhFormatString(
                            L"%s: %s",
                            processItem->ProcessName->Buffer,
                            PhaFormatSize(processItem->IoReadDelta.Delta + processItem->IoWriteDelta.Delta + processItem->IoOtherDelta.Delta, -1)->Buffer
                            );
                    }

                    PhDereferenceObject(processItem);
                }
                else
                {
                    text[count] = PhCreateString(L"-");
                }
            }
            break;
        case ID_STATUS_NUMBEROFVISIBLEITEMS:
            {
                HWND tnHandle = NULL;

                tnHandle = GetCurrentTreeNewHandle();

                if (tnHandle)
                {
                    ULONG visibleCount = 0;

                    visibleCount = TreeNew_GetFlatNodeCount(tnHandle);

                    text[count] = PhFormatString(
                        L"Visible: %lu",
                        visibleCount
                        );
                }
                else
                {
                    text[count] = PhCreateString(
                        L"Visible: N/A"
                        );
                }
            }
            break;
        case ID_STATUS_NUMBEROFSELECTEDITEMS:
            {
                HWND tnHandle = NULL;

                tnHandle = GetCurrentTreeNewHandle();

                if (tnHandle)
                {
                    ULONG visibleCount = 0;
                    ULONG selectedCount = 0;

                    visibleCount = TreeNew_GetFlatNodeCount(tnHandle);

                    for (ULONG i = 0; i < visibleCount; i++)
                    {
                        if (TreeNew_GetFlatNode(tnHandle, i)->Selected)
                            selectedCount++;
                    }

                    text[count] = PhFormatString(
                        L"Selected: %lu",
                        selectedCount
                        );
                }
                else
                {
                    text[count] = PhCreateString(
                        L"Selected: N/A"
                        );
                }
            }
            break;
        case ID_STATUS_INTERVALSTATUS:
            {
                ULONG interval;

                interval = PhGetIntegerSetting(L"UpdateInterval");

                if (UpdateAutomatically)
                {
                    switch (interval)
                    {
                    case 500:
                        text[count] = PhCreateString(L"Interval: Fast");
                        break;
                    case 1000:
                        text[count] = PhCreateString(L"Interval: Normal");
                        break;
                    case 2000:
                        text[count] = PhCreateString(L"Interval: Below normal");
                        break;
                    case 5000:
                        text[count] = PhCreateString(L"Interval: Slow");
                        break;
                    case 10000:
                        text[count] = PhCreateString(L"Interval: Very slow");
                        break;
                    }
                }
                else
                {
                    text[count] = PhCreateString(L"Interval: Paused");
                }
            }
            break;
        }

        if (resetMaxWidths)
            StatusBarMaxWidths[count] = 0;

        if (!GetTextExtentPoint32(hdc, text[count]->Buffer, (ULONG)text[count]->Length / sizeof(WCHAR), &size))
            size.cx = 200;

        if (count != 0)
            widths[count] = widths[count - 1];
        else
            widths[count] = 0;

        width = size.cx + 10;

        if (width <= StatusBarMaxWidths[count])
        {
            width = StatusBarMaxWidths[count];
        }
        else
        {
            StatusBarMaxWidths[count] = width;
        }

        widths[count] += width;

        count++;
    }

    ReleaseDC(StatusBarHandle, hdc);

    SendMessage(StatusBarHandle, SB_SETPARTS, count, (LPARAM)widths);

    for (i = 0; i < count; i++)
    {
        SendMessage(StatusBarHandle, SB_SETTEXT, i, (LPARAM)text[i]->Buffer);
        PhDereferenceObject(text[i]);
    }
}