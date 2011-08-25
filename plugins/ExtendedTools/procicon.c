/*
 * Process Hacker Extended Tools -
 *   process icon duplication
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

PET_PROCESS_ICON EtProcIconCreateProcessIcon(
    __in HICON Icon
    )
{
    PET_PROCESS_ICON processIcon;

    processIcon = PhAllocate(sizeof(ET_PROCESS_ICON));
    processIcon->RefCount = 1;
    processIcon->Icon = DuplicateIcon(NULL, Icon);

    return processIcon;
}

VOID EtProcIconReferenceProcessIcon(
    __inout PET_PROCESS_ICON ProcessIcon
    )
{
    _InterlockedIncrement(&ProcessIcon->RefCount);
}

VOID EtProcIconDereferenceProcessIcon(
    __inout PET_PROCESS_ICON ProcessIcon
    )
{
    if (_InterlockedDecrement(&ProcessIcon->RefCount) == 0)
    {
        DestroyIcon(ProcessIcon->Icon);
        PhFree(ProcessIcon);
    }
}

PET_PROCESS_ICON EtProcIconReferenceSmallProcessIcon(
    __inout PET_PROCESS_BLOCK Block
    )
{
    PET_PROCESS_ICON smallProcessIcon;

    smallProcessIcon = Block->SmallProcessIcon;

    if (!smallProcessIcon && PhTestEvent(&Block->ProcessItem->Stage1Event))
    {
        smallProcessIcon = EtProcIconCreateProcessIcon(Block->ProcessItem->SmallIcon);

        if (_InterlockedCompareExchangePointer(
            &Block->SmallProcessIcon,
            smallProcessIcon,
            NULL
            ) != NULL)
        {
            EtProcIconDereferenceProcessIcon(smallProcessIcon);
            smallProcessIcon = Block->SmallProcessIcon;
        }
    }

    if (smallProcessIcon)
    {
        EtProcIconReferenceProcessIcon(smallProcessIcon);
    }

    return smallProcessIcon;
}

VOID EtProcIconNotifyProcessDelete(
    __inout PET_PROCESS_BLOCK Block
    )
{
    if (Block->SmallProcessIcon)
    {
        EtProcIconDereferenceProcessIcon(Block->SmallProcessIcon);
    }
}
