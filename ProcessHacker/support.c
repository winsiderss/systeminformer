/*
 * Process Hacker - 
 *   application support functions
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

#include <phapp.h>
#include <settings.h>

VOID PhSearchOnlineString(
    __in HWND hWnd,
    __in PWSTR String
    )
{
    PPH_STRING searchEngine = PhGetStringSetting(L"SearchEngine");
    ULONG indexOfReplacement = PhStringIndexOfString(searchEngine, 0, L"%s");

    if (indexOfReplacement != -1)
    {
        PPH_STRING stringBefore;
        PPH_STRING stringAfter;
        PPH_STRING newString;

        // Replace "%s" with the string.

        stringBefore = PhSubstring(searchEngine, 0, indexOfReplacement);
        stringAfter = PhSubstring(
            searchEngine,
            indexOfReplacement + 2,
            searchEngine->Length / 2 - indexOfReplacement - 2
            );

        newString = PhConcatStrings(
            3,
            stringBefore->Buffer,
            String,
            stringAfter->Buffer
            );
        PhShellExecute(hWnd, newString->Buffer, NULL);

        PhDereferenceObject(newString);
        PhDereferenceObject(stringAfter);
        PhDereferenceObject(stringBefore);
    }

    PhDereferenceObject(searchEngine);
}
