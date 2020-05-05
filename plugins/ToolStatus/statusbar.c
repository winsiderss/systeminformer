/*
 * Process Hacker ToolStatus -
 *   statusbar main
 *
 * Copyright (C) 2010-2013 wj32
 * Copyright (C) 2011-2020 dmex
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
ULONG StatusBarMaxWidths[MAX_STATUSBAR_ITEMS];
// Note: no lock is needed because we only ever modify the list on this same thread.
PPH_LIST StatusBarItemList = NULL;
ULONG StatusBarItems[MAX_STATUSBAR_ITEMS] =
{
    // Default items (displayed)
    { ID_STATUS_CPUUSAGE },
    { ID_STATUS_PHYSICALMEMORY },
    { ID_STATUS_FREEMEMORY },
    // Available items (hidden)
    { ID_STATUS_COMMITCHARGE },
    { ID_STATUS_NUMBEROFPROCESSES },
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
        PhAddItemList(StatusBarItemList, UlongToPtr(StatusBarItems[i]));
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

    if (PhIsNullOrEmptyString(settingsString))
    {
        // Load default settings
        StatusBarLoadDefault();
        return;
    }

    remaining = PhGetStringRef(settingsString);

    // Query the number of buttons to insert
    if (!PhSplitStringRefAtChar(&remaining, L'|', &part, &remaining))
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

        PhSplitStringRefAtChar(&remaining, L'|', &idPart, &remaining);

        if (PhStringToInteger64(&idPart, 10, &idInteger))
        {
            PhInsertItemList(StatusBarItemList, i, UlongToPtr((ULONG)idInteger));
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
    }

    return L"ERROR";
}

VOID StatusBarShowMenu(
    VOID
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
        PhMainWndHandle,
        PH_EMENU_SHOW_LEFTRIGHT,
        PH_ALIGN_LEFT | PH_ALIGN_BOTTOM,
        cursorPos.x,
        cursorPos.y
        );

    if (selectedItem && selectedItem->Id != ULONG_MAX)
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
    ULONG widths[MAX_STATUSBAR_ITEMS];
    WCHAR text[MAX_STATUSBAR_ITEMS][0x80];

    if (ProcessesUpdatedCount != 3)
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

    for (i = 0; i < StatusBarItemList->Count; i++)
    {
        SIZE size;
        ULONG width;

        switch (PtrToUlong(StatusBarItemList->Items[i]))
        {
        case ID_STATUS_CPUUSAGE:
            {
                FLOAT cpuUsage = SystemStatistics.CpuKernelUsage + SystemStatistics.CpuUserUsage;
                PH_FORMAT format[3];

                PhInitFormatS(&format[0], L"CPU usage: ");
                PhInitFormatF(&format[1], (DOUBLE)cpuUsage * 100, 2);
                PhInitFormatS(&format[2], L"%");

                PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), NULL);
            }
            break;
        case ID_STATUS_COMMITCHARGE:
            {
                ULONG commitUsage = SystemStatistics.Performance->CommittedPages;
                FLOAT commitFraction = (FLOAT)commitUsage / SystemStatistics.Performance->CommitLimit * 100;
                PH_FORMAT format[5];

                PhInitFormatS(&format[0], L"Commit charge: ");
                PhInitFormatSize(&format[1], UInt32x32To64(commitUsage, PAGE_SIZE));
                PhInitFormatS(&format[2], L" (");
                PhInitFormatF(&format[3], commitFraction, 2);
                PhInitFormatS(&format[4], L"%)");

                PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), NULL);
            }
            break;
        case ID_STATUS_PHYSICALMEMORY:
            {
                ULONG physicalUsage = PhSystemBasicInformation.NumberOfPhysicalPages - SystemStatistics.Performance->AvailablePages;
                FLOAT physicalFraction = (FLOAT)physicalUsage / PhSystemBasicInformation.NumberOfPhysicalPages * 100;
                PH_FORMAT format[5];

                PhInitFormatS(&format[0], L"Physical memory: ");
                PhInitFormatSize(&format[1], UInt32x32To64(physicalUsage, PAGE_SIZE));
                PhInitFormatS(&format[2], L" (");
                PhInitFormatF(&format[3], physicalFraction, 2);
                PhInitFormatS(&format[4], L"%)");

                PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), NULL);
            }
            break;
        case ID_STATUS_FREEMEMORY:
            {
                ULONG physicalFree = SystemStatistics.Performance->AvailablePages;
                FLOAT physicalFreeFraction = (FLOAT)physicalFree / PhSystemBasicInformation.NumberOfPhysicalPages * 100;
                PH_FORMAT format[5];

                PhInitFormatS(&format[0], L"Free memory: ");
                PhInitFormatSize(&format[1], UInt32x32To64(physicalFree, PAGE_SIZE));
                PhInitFormatS(&format[2], L" (");
                PhInitFormatF(&format[3], physicalFreeFraction, 2);
                PhInitFormatS(&format[4], L"%)");

                PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), NULL);
            }
            break;
        case ID_STATUS_NUMBEROFPROCESSES:
            {
                PH_FORMAT format[2];

                PhInitFormatS(&format[0], L"Processes: ");
                PhInitFormatI64UGroupDigits(&format[1], SystemStatistics.NumberOfProcesses);

                PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), NULL);
            }
            break;
        case ID_STATUS_NUMBEROFTHREADS:
            {
                PH_FORMAT format[2];

                PhInitFormatS(&format[0], L"Threads: ");
                PhInitFormatI64UGroupDigits(&format[1], SystemStatistics.NumberOfThreads);

                PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), NULL);
            }
            break;
        case ID_STATUS_NUMBEROFHANDLES:
            {
                PH_FORMAT format[2];

                PhInitFormatS(&format[0], L"Handles: ");
                PhInitFormatI64UGroupDigits(&format[1], SystemStatistics.NumberOfHandles);

                PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), NULL);
            }
            break;
        case ID_STATUS_IO_RO:
            {
                PH_FORMAT format[2];

                PhInitFormatS(&format[0], L"I/O R+O: ");
                PhInitFormatSize(&format[1], (SystemStatistics.IoReadDelta.Delta + SystemStatistics.IoOtherDelta.Delta));

                PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), NULL);
            }
            break;
        case ID_STATUS_IO_W:
            {
                PH_FORMAT format[2];

                PhInitFormatS(&format[0], L"I/O W: ");
                PhInitFormatSize(&format[1], SystemStatistics.IoWriteDelta.Delta);

                PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), NULL);
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
                        PhInitFormatF(&format[4], (DOUBLE)processItem->CpuUsage * 100, 2);
                        PhInitFormatS(&format[5], L"%");

                        PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), NULL);
                    }
                    else
                    {
                        PH_FORMAT format[4];

                        PhInitFormatSR(&format[0], processItem->ProcessName->sr);
                        PhInitFormatS(&format[1], L": ");
                        PhInitFormatF(&format[2], (DOUBLE)processItem->CpuUsage * 100, 2);
                        PhInitFormatS(&format[3], L"%)");

                        PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), NULL);
                    }

                    PhDereferenceObject(processItem);
                }
                else
                {
                    PH_FORMAT format[1];

                    PhInitFormatS(&format[0], L"-");

                    PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), NULL);
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

                        PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), NULL);
                    }
                    else
                    {
                        PH_FORMAT format[3];

                        PhInitFormatSR(&format[0], processItem->ProcessName->sr);
                        PhInitFormatS(&format[1], L": ");
                        PhInitFormatSize(&format[2], processItem->IoReadDelta.Delta + processItem->IoWriteDelta.Delta + processItem->IoOtherDelta.Delta);

                        PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), NULL);
                    }

                    PhDereferenceObject(processItem);
                }
                else
                {
                    PH_FORMAT format[1];

                    PhInitFormatS(&format[0], L"-");

                    PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), NULL);
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

                    PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), NULL);
                }
                else
                {
                    PH_FORMAT format[1];

                    PhInitFormatS(&format[0], L"Visible: N/A");

                    PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), NULL);
                }
            }
            break;
        case ID_STATUS_NUMBEROFSELECTEDITEMS:
            {
                HWND tnHandle;

                if (tnHandle = GetCurrentTreeNewHandle())
                {
                    ULONG i;
                    ULONG visibleCount;
                    ULONG selectedCount;
                    PH_FORMAT format[2];

                    visibleCount = TreeNew_GetFlatNodeCount(tnHandle);
                    selectedCount = 0;

                    for (i = 0; i < visibleCount; i++)
                    {
                        if (TreeNew_GetFlatNode(tnHandle, i)->Selected)
                            selectedCount++;
                    }

                    PhInitFormatS(&format[0], L"Selected: ");
                    PhInitFormatI64UGroupDigits(&format[1], selectedCount);

                    PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), NULL);
                }
                else
                {
                    PH_FORMAT format[1];

                    PhInitFormatS(&format[0], L"Selected: N/A");

                    PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), NULL);
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

                PhFormatToBuffer(format, RTL_NUMBER_OF(format), text[count], sizeof(text[count]), NULL);
            }
            break;
        }

        if (!GetTextExtentPoint32(hdc, text[count], (INT)PhCountStringZ(text[count]), &size))
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

    SendMessage(StatusBarHandle, SB_SETPARTS, count, (LPARAM)widths);

    for (i = 0; i < count; i++)
    {
        SendMessage(StatusBarHandle, SB_SETTEXT, i, (LPARAM)text[i]);
    }
}
