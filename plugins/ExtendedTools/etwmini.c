/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2015
 *
 */

#include "exttools.h"
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
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2
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
                PH_AUTO(PhFormat(format, 4, 50)));
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
            PPH_LIST processes;
            ULONG64 diskReadDelta;
            ULONG64 diskWriteDelta;
            ULONG i;

            processes = assignSortData->ProcessGroup->Processes;
            diskReadDelta = 0;
            diskWriteDelta = 0;

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
            PPH_LIST processes;
            ULONG64 diskReadDelta;
            ULONG64 diskWriteDelta;
            PH_FORMAT format[1];

            processes = getUsageText->ProcessGroup->Processes;
            diskReadDelta = getUsageText->SortData->UserData[0];
            diskWriteDelta = getUsageText->SortData->UserData[1];

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
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2
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
                PH_AUTO(PhFormat(format, 4, 50)));
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
            PPH_LIST processes;
            ULONG64 networkReceiveDelta;
            ULONG64 networkSendDelta;
            ULONG i;

            processes = assignSortData->ProcessGroup->Processes;
            networkReceiveDelta = 0;
            networkSendDelta = 0;

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
            PPH_LIST processes;
            ULONG64 networkReceiveDelta;
            ULONG64 networkSendDelta;
            PH_FORMAT format[1];

            processes = getUsageText->ProcessGroup->Processes;
            networkReceiveDelta = getUsageText->SortData->UserData[0];
            networkSendDelta = getUsageText->SortData->UserData[1];

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
