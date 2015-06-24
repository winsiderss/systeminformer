/*
 * Process Hacker ToolStatus -
 *   statusbar main
 *
 * Copyright (C) 2011-2015 dmex
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

HWND StatusBarHandle;
ULONG StatusMask;
ULONG ProcessesUpdatedCount;
static ULONG StatusBarMaxWidths[STATUS_COUNT];

VOID ShowStatusMenu(
    _In_ PPOINT Point
    )
{
    HMENU menu;
    HMENU subMenu;
    ULONG i;
    ULONG id;
    ULONG bit;

    menu = LoadMenu(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDR_STATUS)
        );

    subMenu = GetSubMenu(menu, 0);

    // Check the enabled items.
    for (i = STATUS_MINIMUM; i != STATUS_MAXIMUM; i <<= 1)
    {
        if (StatusMask & i)
        {
            switch (i)
            {
            case STATUS_CPUUSAGE:
                id = ID_STATUS_CPUUSAGE;
                break;
            case STATUS_COMMIT:
                id = ID_STATUS_COMMITCHARGE;
                break;
            case STATUS_PHYSICAL:
                id = ID_STATUS_PHYSICALMEMORY;
                break;
            case STATUS_NUMBEROFPROCESSES:
                id = ID_STATUS_NUMBEROFPROCESSES;
                break;
            case STATUS_NUMBEROFTHREADS:
                id = ID_STATUS_NUMBEROFTHREADS;
                break;
            case STATUS_NUMBEROFHANDLES:
                id = ID_STATUS_NUMBEROFHANDLES;
                break;
            case STATUS_IOREADOTHER:
                id = ID_STATUS_IO_RO;
                break;
            case STATUS_IOWRITE:
                id = ID_STATUS_IO_W;
                break;
            case STATUS_MAXCPUPROCESS:
                id = ID_STATUS_MAX_CPU_PROCESS;
                break;
            case STATUS_MAXIOPROCESS:
                id = ID_STATUS_MAX_IO_PROCESS;
                break;
            case STATUS_VISIBLEITEMS:
                id = ID_STATUS_NUMBEROFVISIBLEITEMS;
                break;
            case STATUS_SELECTEDITEMS:
                id = ID_STATUS_NUMBEROFSELECTEDITEMS;
                break;
            }

            CheckMenuItem(subMenu, id, MF_CHECKED);
        }
    }

    id = (ULONG)TrackPopupMenu(
        subMenu,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
        Point->x,
        Point->y,
        0,
        PhMainWndHandle,
        NULL
        );

    DestroyMenu(menu);

    switch (id)
    {
    case ID_STATUS_CPUUSAGE:
        bit = STATUS_CPUUSAGE;
        break;
    case ID_STATUS_COMMITCHARGE:
        bit = STATUS_COMMIT;
        break;
    case ID_STATUS_PHYSICALMEMORY:
        bit = STATUS_PHYSICAL;
        break;
    case ID_STATUS_NUMBEROFPROCESSES:
        bit = STATUS_NUMBEROFPROCESSES;
        break;
    case ID_STATUS_NUMBEROFTHREADS:
        bit = STATUS_NUMBEROFTHREADS;
        break;
    case ID_STATUS_NUMBEROFHANDLES:
        bit = STATUS_NUMBEROFHANDLES;
        break;
    case ID_STATUS_IO_RO:
        bit = STATUS_IOREADOTHER;
        break;
    case ID_STATUS_IO_W:
        bit = STATUS_IOWRITE;
        break;
    case ID_STATUS_MAX_CPU_PROCESS:
        bit = STATUS_MAXCPUPROCESS;
        break;
    case ID_STATUS_MAX_IO_PROCESS:
        bit = STATUS_MAXIOPROCESS;
        break;
    case ID_STATUS_NUMBEROFVISIBLEITEMS:
        bit = STATUS_VISIBLEITEMS;
        break;
    case ID_STATUS_NUMBEROFSELECTEDITEMS:
        bit = STATUS_SELECTEDITEMS;
        break;
    default:
        return;
    }

    StatusMask ^= bit;
    PhSetIntegerSetting(SETTING_NAME_ENABLE_STATUSMASK, StatusMask);

    UpdateStatusBar();
}

VOID UpdateStatusBar(
    VOID
    )
{
    static ULONG lastTickCount = 0;

    PPH_STRING text[STATUS_COUNT];
    ULONG widths[STATUS_COUNT];
    ULONG i;
    ULONG index;
    ULONG count;
    HDC hdc;
    PH_PLUGIN_SYSTEM_STATISTICS statistics;
    BOOLEAN resetMaxWidths = FALSE;

    if (ProcessesUpdatedCount < 2)
        return;

    if (!(StatusMask & (STATUS_MAXIMUM - 1)))
    {
        // The status bar doesn't cope well with 0 parts.
        widths[0] = -1;
        SendMessage(StatusBarHandle, SB_SETPARTS, 1, (LPARAM)widths);
        SendMessage(StatusBarHandle, SB_SETTEXT, 0, (LPARAM)L"");
        return;
    }

    PhPluginGetSystemStatistics(&statistics);

    hdc = GetDC(StatusBarHandle);
    SelectObject(hdc, (HFONT)SendMessage(StatusBarHandle, WM_GETFONT, 0, 0));

    // Reset max. widths for Max. CPU Process and Max. I/O Process parts once in a while.
    {
        ULONG tickCount;

        tickCount = GetTickCount();

        if (tickCount - lastTickCount >= 10000)
        {
            resetMaxWidths = TRUE;
            lastTickCount = tickCount;
        }
    }

    count = 0;
    index = 0;

    for (i = STATUS_MINIMUM; i != STATUS_MAXIMUM; i <<= 1)
    {
        if (StatusMask & i)
        {
            SIZE size;
            PPH_PROCESS_ITEM processItem;
            ULONG width;

            switch (i)
            {
            case STATUS_CPUUSAGE:
                {
                    text[count] = PhFormatString(
                        L"CPU Usage: %.2f%%", 
                        (statistics.CpuKernelUsage + statistics.CpuUserUsage) * 100
                        );
                }
                break;
            case STATUS_COMMIT:
                {
                    text[count] = PhFormatString(
                        L"Commit Charge: %.2f%%",
                        (FLOAT)statistics.CommitPages * 100 / statistics.Performance->CommitLimit
                        );
                }
                break;
            case STATUS_PHYSICAL:
                {
                    text[count] = PhFormatString(
                        L"Physical Memory: %.2f%%",
                        (FLOAT)statistics.PhysicalPages * 100 / PhSystemBasicInformation.NumberOfPhysicalPages
                        );
                }
                break;
            case STATUS_NUMBEROFPROCESSES:
                {
                    text[count] = PhConcatStrings2(
                        L"Processes: ",
                        PhaFormatUInt64(statistics.NumberOfProcesses, TRUE)->Buffer
                        );
                }
                break;
            case STATUS_NUMBEROFTHREADS:
                {
                    text[count] = PhConcatStrings2(
                        L"Threads: ",
                        PhaFormatUInt64(statistics.NumberOfThreads, TRUE)->Buffer
                        );
                }
                break;
            case STATUS_NUMBEROFHANDLES:
                {
                    text[count] = PhConcatStrings2(
                        L"Handles: ",
                        PhaFormatUInt64(statistics.NumberOfHandles, TRUE)->Buffer
                        );
                }
                break;
            case STATUS_IOREADOTHER:
                {
                    text[count] = PhConcatStrings2(
                        L"I/O R+O: ", 
                        PhaFormatSize(statistics.IoReadDelta.Delta + statistics.IoOtherDelta.Delta, -1)->Buffer
                        );
                }
                break;
            case STATUS_IOWRITE:
                {
                    text[count] = PhConcatStrings2(
                        L"I/O W: ", 
                        PhaFormatSize(statistics.IoWriteDelta.Delta, -1)->Buffer
                        );
                }
                break;
            case STATUS_MAXCPUPROCESS:
                {
                    if (statistics.MaxCpuProcessId && (processItem = PhReferenceProcessItem(statistics.MaxCpuProcessId)))
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

                    if (resetMaxWidths)
                        StatusBarMaxWidths[index] = 0;
                }
                break;
            case STATUS_MAXIOPROCESS:
                {
                    if (statistics.MaxIoProcessId && (processItem = PhReferenceProcessItem(statistics.MaxIoProcessId)))
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

                    if (resetMaxWidths)
                        StatusBarMaxWidths[index] = 0;
                }
                break;
            case STATUS_VISIBLEITEMS:
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
                        text[count] = PhFormatString(
                            L"Visible: N/A"
                            );
                    }
                }
                break;
            case STATUS_SELECTEDITEMS:
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
                        text[count] = PhFormatString(
                            L"Selected: N/A"
                            );
                    }
                }
                break;
            }

            if (!GetTextExtentPoint32(hdc, text[count]->Buffer, (ULONG)text[count]->Length / 2, &size))
                size.cx = 200;

            if (count != 0)
                widths[count] = widths[count - 1];
            else
                widths[count] = 0;

            width = size.cx + 10;

            if (width <= StatusBarMaxWidths[index])
                width = StatusBarMaxWidths[index];
            else
                StatusBarMaxWidths[index] = width;

            widths[count] += width;

            count++;
        }
        else
        {
            StatusBarMaxWidths[index] = 0;
        }

        index++;
    }

    ReleaseDC(StatusBarHandle, hdc);

    SendMessage(StatusBarHandle, SB_SETPARTS, count, (LPARAM)widths);

    for (i = 0; i < count; i++)
    {
        SendMessage(StatusBarHandle, SB_SETTEXT, i, (LPARAM)text[i]->Buffer);
        PhDereferenceObject(text[i]);
    }
}