/*
 * Process Hacker - 
 *   base support functions (auto-dereference enabled)
 * 
 * Copyright (C) 2010 wj32
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

#include <phbase.h>

PPH_STRING PhaCreateString(
    __in PWSTR Buffer
    )
{
    return PHA_DEREFERENCE(PhCreateString(Buffer));
}

PPH_STRING PhaCreateStringEx(
    __in PWSTR Buffer,
    __in SIZE_T Length
    )
{
    return PHA_DEREFERENCE(PhCreateStringEx(Buffer, Length));
}

PPH_STRING PhaDuplicateString(
    __in PPH_STRING String
    )
{
    return PHA_DEREFERENCE(PhDuplicateString(String));
}

PPH_STRING PhaConcatStrings(
    __in ULONG Count,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Count);

    return PHA_DEREFERENCE(PhConcatStrings_V(Count, argptr));
}

PPH_STRING PhaPrintfString(
    __in PWSTR Format,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Format);

    return PHA_DEREFERENCE(PhPrintfString_V(Format, argptr));
}

PPH_STRING PhaLowerString(
    __in PPH_STRING String
    )
{
    PPH_STRING newString;

    newString = PhaDuplicateString(String);
    PhLowerString(newString);

    return newString;
}

PPH_STRING PhaUpperString(
    __in PPH_STRING String
    )
{
    PPH_STRING newString;

    newString = PhaDuplicateString(String);
    PhUpperString(newString);

    return newString;
}

PPH_STRING PhaSubstring(
    __in PPH_STRING String,
    __in ULONG StartIndex,
    __in ULONG Count
    )
{
    return PHA_DEREFERENCE(PhSubstring(String, StartIndex, Count));
}
