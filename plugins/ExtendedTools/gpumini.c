/*
 * Process Hacker Extended Tools -
 *   GPU mini information section
 *
 * Copyright (C) 2015 wj32
 * Copyright (C) 2016-2020 dmex
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
#include "gpumini.h"

VOID EtGpuMiniInformationInitializing(
    _In_ PPH_PLUGIN_MINIINFO_POINTERS Pointers
    )
{
    PH_MINIINFO_LIST_SECTION section;

    memset(&section, 0, sizeof(PH_MINIINFO_LIST_SECTION));
    section.Callback = EtpGpuListSectionCallback;
    Pointers->CreateListSection(L"GPU", 0, &section);
}

BOOLEAN EtpGpuListSectionCallback(
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
            PH_FORMAT format[3];

            // GPU    %.2f%%
            PhInitFormatS(&format[0], L"GPU    ");
            PhInitFormatF(&format[1], (DOUBLE)EtGpuNodeUsage * 100, 2);
            PhInitFormatC(&format[2], L'%');

            ListSection->Section->Parameters->SetSectionText(ListSection->Section,
                PH_AUTO_T(PH_STRING, PhFormat(format, RTL_NUMBER_OF(format), 0)));
        }
        break;
    case MiListSectionSortProcessList:
        {
            PPH_MINIINFO_LIST_SECTION_SORT_LIST sortList = Parameter1;

            if (!sortList)
                break;

            qsort(sortList->List->Items, sortList->List->Count,
                sizeof(PPH_PROCESS_NODE), EtpGpuListSectionProcessCompareFunction);
        }
        return TRUE;
    case MiListSectionAssignSortData:
        {
            PPH_MINIINFO_LIST_SECTION_ASSIGN_SORT_DATA assignSortData = Parameter1;
            PPH_LIST processes;
            FLOAT gpuUsage;
            ULONG i;

            if (!assignSortData)
                break;

            processes = assignSortData->ProcessGroup->Processes;
            gpuUsage = 0;

            for (i = 0; i < processes->Count; i++)
            {
                PPH_PROCESS_ITEM processItem = processes->Items[i];
                PET_PROCESS_BLOCK block = EtGetProcessBlock(processItem);
                gpuUsage += block->GpuNodeUtilization;
            }

            *(PFLOAT)assignSortData->SortData->UserData = gpuUsage;
        }
        return TRUE;
    case MiListSectionSortGroupList:
        {
            PPH_MINIINFO_LIST_SECTION_SORT_LIST sortList = Parameter1;

            if (!sortList)
                break;

            qsort(sortList->List->Items, sortList->List->Count,
                sizeof(PPH_MINIINFO_LIST_SECTION_SORT_DATA), EtpGpuListSectionNodeCompareFunction);
        }
        return TRUE;
    case MiListSectionGetUsageText:
        {
            PPH_MINIINFO_LIST_SECTION_GET_USAGE_TEXT getUsageText = Parameter1;
            PPH_LIST processes;
            FLOAT gpuUsage;
            PH_FORMAT format[2];

            if (!getUsageText)
                break;

            processes = getUsageText->ProcessGroup->Processes;
            gpuUsage = *(PFLOAT)getUsageText->SortData->UserData;

            // %.2f%%
            PhInitFormatF(&format[0], (DOUBLE)gpuUsage * 100, 2);
            PhInitFormatC(&format[1], L'%');

            PhMoveReference(&getUsageText->Line1, PhFormat(format, RTL_NUMBER_OF(format), 0));
        }
        return TRUE;
    }

    return FALSE;
}

int __cdecl EtpGpuListSectionProcessCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    int result;
    PPH_PROCESS_NODE node1 = *(PPH_PROCESS_NODE *)elem1;
    PPH_PROCESS_NODE node2 = *(PPH_PROCESS_NODE *)elem2;
    PET_PROCESS_BLOCK block1 = EtGetProcessBlock(node1->ProcessItem);
    PET_PROCESS_BLOCK block2 = EtGetProcessBlock(node2->ProcessItem);

    result = singlecmp(block2->GpuNodeUtilization, block1->GpuNodeUtilization);

    if (result == 0)
        result = uint64cmp(block2->GpuRunningTimeDelta.Value, block1->GpuRunningTimeDelta.Value);
    if (result == 0)
        result = singlecmp(node2->ProcessItem->CpuUsage, node1->ProcessItem->CpuUsage);

    return result;
}

int __cdecl EtpGpuListSectionNodeCompareFunction(
    _In_ const void *elem1,
    _In_ const void *elem2
    )
{
    PPH_MINIINFO_LIST_SECTION_SORT_DATA data1 = *(PPH_MINIINFO_LIST_SECTION_SORT_DATA *)elem1;
    PPH_MINIINFO_LIST_SECTION_SORT_DATA data2 = *(PPH_MINIINFO_LIST_SECTION_SORT_DATA *)elem2;

    return singlecmp(*(PFLOAT)data2->UserData, *(PFLOAT)data1->UserData);
}
