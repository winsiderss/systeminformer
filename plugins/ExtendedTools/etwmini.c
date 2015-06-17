/*
 * Process Hacker Extended Tools -
 *   ETW mini information section
 *
 * Copyright (C) 2015 wj32
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
#include "resource.h"
#include "etwmini.h"

VOID EtEtwMiniInformationInitializing(
    _In_ PPH_PLUGIN_MINIINFO_POINTERS Pointers
    )
{
    PH_MINIINFO_LIST_SECTION section;

    memset(&section, 0, sizeof(PH_MINIINFO_LIST_SECTION));
    section.Callback = EtpDiskListSectionCallback;
    Pointers->CreateListSection(L"Disk", 0, &section);

    memset(&section, 0, sizeof(PH_MINIINFO_LIST_SECTION));
    section.Callback = EtpNetworkListSectionCallback;

    Pointers->CreateListSection(L"Network", 0, &section);
}

BOOLEAN EtpDiskListSectionCallback(
    _In_ struct _PH_MINIINFO_LIST_SECTION *ListSection,
    _In_ PH_MINIINFO_LIST_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    switch (Message)
    {
    case MiListSectionTick:
        {
            PH_FORMAT format[4];

            PhInitFormatS(&format[0], L"Disk    R: ");
            PhInitFormatSize(&format[1], EtDiskReadDelta.Delta);
            format[1].Type |= FormatUsePrecision;
            format[1].Precision = 0;
            PhInitFormatS(&format[2], L"  W: ");
            PhInitFormatSize(&format[3], EtDiskWriteDelta.Delta);
            format[3].Type |= FormatUsePrecision;
            format[3].Precision = 0;
            ListSection->Section->Parameters->SetSectionText(ListSection->Section,
                PhAutoDereferenceObject(PhFormat(format, 4, 50)));
        }
        break;
    case MiListSectionSortProcessList:
        {
            PPH_MINIINFO_LIST_SECTION_SORT_LIST sortList = Parameter1;

            qsort(sortList->List->Items, sortList->List->Count,
                sizeof(PPH_PROCESS_NODE), EtpDiskListSectionProcessCompareFunction);
        }
        return TRUE;
    case MiListSectionAssignSortData:
        {
            PPH_MINIINFO_LIST_SECTION_ASSIGN_SORT_DATA assignSortData = Parameter1;
            PPH_LIST processes = assignSortData->ProcessGroup->Processes;
            ULONG64 diskReadDelta = 0;
            ULONG64 diskWriteDelta = 0;
            ULONG i;

            for (i = 0; i < processes->Count; i++)
            {
                PPH_PROCESS_ITEM processItem = processes->Items[i];
                PET_PROCESS_BLOCK block = EtGetProcessBlock(processItem);
                diskReadDelta += block->DiskReadRawDelta.Delta;
                diskWriteDelta += block->DiskWriteRawDelta.Delta;
            }

            assignSortData->SortData->UserData[0] = diskReadDelta;
            assignSortData->SortData->UserData[1] = diskWriteDelta;
        }
        return TRUE;
    case MiListSectionSortGroupList:
        {
            PPH_MINIINFO_LIST_SECTION_SORT_LIST sortList = Parameter1;

            qsort(sortList->List->Items, sortList->List->Count,
                sizeof(PPH_MINIINFO_LIST_SECTION_SORT_DATA), EtpDiskListSectionNodeCompareFunction);
        }
        return TRUE;
    case MiListSectionGetUsageText:
        {
            PPH_MINIINFO_LIST_SECTION_GET_USAGE_TEXT getUsageText = Parameter1;
            PPH_LIST processes = getUsageText->ProcessGroup->Processes;
            ULONG64 diskReadDelta = getUsageText->SortData->UserData[0];
            ULONG64 diskWriteDelta = getUsageText->SortData->UserData[1];
            PH_FORMAT format[1];

            PhInitFormatSize(&format[0], diskReadDelta + diskWriteDelta);
            PhMoveReference(&getUsageText->Line1, PhFormat(format, 1, 16));
        }
        return TRUE;
    }

    return FALSE;
}

int __cdecl EtpDiskListSectionProcessCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    int result;
    PPH_PROCESS_NODE node1 = *(PPH_PROCESS_NODE *)elem1;
    PPH_PROCESS_NODE node2 = *(PPH_PROCESS_NODE *)elem2;
    PET_PROCESS_BLOCK block1 = EtGetProcessBlock(node1->ProcessItem);
    PET_PROCESS_BLOCK block2 = EtGetProcessBlock(node2->ProcessItem);
    ULONG64 total1 = block1->DiskReadRawDelta.Delta + block1->DiskWriteRawDelta.Delta;
    ULONG64 total2 = block2->DiskReadRawDelta.Delta + block2->DiskWriteRawDelta.Delta;

    result = uint64cmp(total2, total1);

    if (result == 0)
        result = uint64cmp(block2->DiskReadRaw + block2->DiskWriteRaw, block1->DiskReadRaw + block1->DiskWriteRaw);
    if (result == 0)
        result = singlecmp(node2->ProcessItem->CpuUsage, node1->ProcessItem->CpuUsage);

    return result;
}

int __cdecl EtpDiskListSectionNodeCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PPH_MINIINFO_LIST_SECTION_SORT_DATA data1 = *(PPH_MINIINFO_LIST_SECTION_SORT_DATA *)elem1;
    PPH_MINIINFO_LIST_SECTION_SORT_DATA data2 = *(PPH_MINIINFO_LIST_SECTION_SORT_DATA *)elem2;

    return uint64cmp(data2->UserData[0] + data2->UserData[1], data1->UserData[0] + data1->UserData[1]);
}

BOOLEAN EtpNetworkListSectionCallback(
    _In_ struct _PH_MINIINFO_LIST_SECTION *ListSection,
    _In_ PH_MINIINFO_LIST_SECTION_MESSAGE Message,
    _In_opt_ PVOID Parameter1,
    _In_opt_ PVOID Parameter2
    )
{
    switch (Message)
    {
    case MiListSectionTick:
        {
            PH_FORMAT format[4];

            PhInitFormatS(&format[0], L"Network    R: ");
            PhInitFormatSize(&format[1], EtNetworkReceiveDelta.Delta);
            format[1].Type |= FormatUsePrecision;
            format[1].Precision = 0;
            PhInitFormatS(&format[2], L"  S: ");
            PhInitFormatSize(&format[3], EtNetworkSendDelta.Delta);
            format[3].Type |= FormatUsePrecision;
            format[3].Precision = 0;
            ListSection->Section->Parameters->SetSectionText(ListSection->Section,
                PhAutoDereferenceObject(PhFormat(format, 4, 50)));
        }
        break;
    case MiListSectionSortProcessList:
        {
            PPH_MINIINFO_LIST_SECTION_SORT_LIST sortList = Parameter1;

            qsort(sortList->List->Items, sortList->List->Count,
                sizeof(PPH_PROCESS_NODE), EtpNetworkListSectionProcessCompareFunction);
        }
        return TRUE;
    case MiListSectionAssignSortData:
        {
            PPH_MINIINFO_LIST_SECTION_ASSIGN_SORT_DATA assignSortData = Parameter1;
            PPH_LIST processes = assignSortData->ProcessGroup->Processes;
            ULONG64 networkReceiveDelta = 0;
            ULONG64 networkSendDelta = 0;
            ULONG i;

            for (i = 0; i < processes->Count; i++)
            {
                PPH_PROCESS_ITEM processItem = processes->Items[i];
                PET_PROCESS_BLOCK block = EtGetProcessBlock(processItem);
                networkReceiveDelta += block->NetworkReceiveRawDelta.Delta;
                networkSendDelta += block->NetworkSendRawDelta.Delta;
            }

            assignSortData->SortData->UserData[0] = networkReceiveDelta;
            assignSortData->SortData->UserData[1] = networkSendDelta;
        }
        return TRUE;
    case MiListSectionSortGroupList:
        {
            PPH_MINIINFO_LIST_SECTION_SORT_LIST sortList = Parameter1;

            qsort(sortList->List->Items, sortList->List->Count,
                sizeof(PPH_MINIINFO_LIST_SECTION_SORT_DATA), EtpNetworkListSectionNodeCompareFunction);
        }
        return TRUE;
    case MiListSectionGetUsageText:
        {
            PPH_MINIINFO_LIST_SECTION_GET_USAGE_TEXT getUsageText = Parameter1;
            PPH_LIST processes = getUsageText->ProcessGroup->Processes;
            ULONG64 networkReceiveDelta = getUsageText->SortData->UserData[0];
            ULONG64 networkSendDelta = getUsageText->SortData->UserData[1];
            PH_FORMAT format[1];

            PhInitFormatSize(&format[0], networkReceiveDelta + networkSendDelta);
            PhMoveReference(&getUsageText->Line1, PhFormat(format, 1, 16));
        }
        return TRUE;
    }

    return FALSE;
}

int __cdecl EtpNetworkListSectionProcessCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    int result;
    PPH_PROCESS_NODE node1 = *(PPH_PROCESS_NODE *)elem1;
    PPH_PROCESS_NODE node2 = *(PPH_PROCESS_NODE *)elem2;
    PET_PROCESS_BLOCK block1 = EtGetProcessBlock(node1->ProcessItem);
    PET_PROCESS_BLOCK block2 = EtGetProcessBlock(node2->ProcessItem);
    ULONG64 total1 = block1->NetworkReceiveRawDelta.Delta + block1->NetworkSendRawDelta.Delta;
    ULONG64 total2 = block2->NetworkReceiveRawDelta.Delta + block2->NetworkSendRawDelta.Delta;

    result = uint64cmp(total2, total1);

    if (result == 0)
        result = uint64cmp(block2->NetworkReceiveRaw + block2->NetworkSendRaw, block1->NetworkReceiveRaw + block1->NetworkSendRaw);
    if (result == 0)
        result = singlecmp(node2->ProcessItem->CpuUsage, node1->ProcessItem->CpuUsage);

    return result;
}

int __cdecl EtpNetworkListSectionNodeCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PPH_MINIINFO_LIST_SECTION_SORT_DATA data1 = *(PPH_MINIINFO_LIST_SECTION_SORT_DATA *)elem1;
    PPH_MINIINFO_LIST_SECTION_SORT_DATA data2 = *(PPH_MINIINFO_LIST_SECTION_SORT_DATA *)elem2;

    return uint64cmp(data2->UserData[0] + data2->UserData[1], data1->UserData[0] + data1->UserData[1]);
}
