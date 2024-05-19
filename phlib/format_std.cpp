/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     dmex    2020
 *
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
