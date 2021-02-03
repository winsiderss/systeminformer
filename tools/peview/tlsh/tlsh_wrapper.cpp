/*
 * Process Hacker -
 *   PE viewer
 *
 * Copyright (C) 2021 dmex
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
#include "tlsh.h"
#include "tlsh_wrapper.h"

EXTERN_C
BOOLEAN
NTAPI
PvGetTlshBufferHash(
    _In_ PVOID Buffer,
    _In_ SIZE_T BufferLength,
    _Out_ PPH_STRING* HashResult
    )
{
    const char* tlshHashString = nullptr;

    if (BufferLength < MIN_DATA_LENGTH)
        return FALSE;
    if (BufferLength > UINT_MAX) // tlsh limited to 4gb
        return FALSE;

    Tlsh* tlshHash = new Tlsh();

    tlshHash->final(
        (const unsigned char*)Buffer,
        (unsigned int)BufferLength,
        0
        );

    if (!tlshHash->isValid())
        return FALSE;

    tlshHashString = tlshHash->getHash(TRUE);

    if (tlshHashString == nullptr || tlshHashString[0] == ANSI_NULL)
        return FALSE;

    *HashResult = PhZeroExtendToUtf16((PSTR)tlshHashString);

    return TRUE;
}
