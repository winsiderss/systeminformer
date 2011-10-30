/*
 * Process Hacker Extended Tools -
 *   utility functions
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

VOID EtFormatRate(
    __in ULONG64 ValuePerPeriod,
    __inout PPH_STRING *Buffer,
    __out_opt PPH_STRINGREF String
    )
{
    ULONG64 number;

    number = ValuePerPeriod;
    number *= 1000;
    number /= PhGetIntegerSetting(L"UpdateInterval");

    if (number != 0)
    {
        PH_FORMAT format[2];

        PhInitFormatSize(&format[0], number);
        PhInitFormatS(&format[1], L"/s");
        PhSwapReference2(Buffer, PhFormat(format, 2, 0));

        if (String)
            *String = (*Buffer)->sr;
    }
}
