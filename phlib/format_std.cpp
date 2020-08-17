/*
 * Process Hacker -
 *   string formatting
 *
 * Copyright (C) 2020 dmex
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
#include <charconv>

using namespace std;

_Success_(return)
BOOLEAN PhFormatDoubleToUtf8(
    _In_ DOUBLE Value,
    _In_ ULONG Type,
    _In_ ULONG Precision,
    _Out_writes_bytes_opt_(BufferLength) PSTR Buffer,
    _In_opt_ SIZE_T BufferLength,
    _Out_opt_ PSIZE_T ReturnLength
    )
{
    chars_format format = chars_format::fixed;
    to_chars_result result;
    SIZE_T returnLength;
    CHAR buffer[_CVTBUFSIZE + 1];

    if (Type & FormatStandardForm)
        format = chars_format::general;
    else if (Type & FormatHexadecimalForm)
        format = chars_format::hex;

    result = to_chars(
        buffer,
        end(buffer),
        Value,
        format,
        Precision
        );

    if (result.ec == errc::value_too_large)
        return FALSE;

    returnLength = result.ptr - buffer;

    if (returnLength == 0)
        return FALSE;

    // This could be removed in favor of directly passing the input buffer to std:to_chars but
    // for now use memcpy so that failures writing a value don't touch the input buffer (dmex)
    memcpy_s(Buffer, BufferLength, buffer, returnLength);
    Buffer[returnLength] = ANSI_NULL;

    if (ReturnLength)
        *ReturnLength = returnLength;

    return TRUE;
}
