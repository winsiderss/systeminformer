/*
 * Process Hacker - 
 *   string formatting
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

/*
 * This module provides a high-performance string formatting mechanism.
 * Instead of using format strings, the user supplies an array of 
 * structures.
 */

#include <phbase.h>
#include <locale.h>

extern ULONG PhMaxSizeUnit;

#define SMALL_BUFFER_LENGTH (PHOBJ_SMALL_OBJECT_SIZE - FIELD_OFFSET(PH_STRING, Buffer) - sizeof(WCHAR))
#define BUFFER_SIZE 512

#define PHP_FORMAT_NEGATIVE 0x1000

// Internal CRT routines needed for floating-point conversion

errno_t __cdecl _cfltcvt_l(double *arg, char *buffer, size_t sizeInBytes,
    int format, int precision, int caps, _locale_t plocinfo);

void __cdecl _cropzeros_l(char *_Buf, _locale_t _Locale);
void __cdecl _forcdecpt_l(char *_Buf, _locale_t _Locale);

// Keep in sync with PhSizeUnitNames
static PH_STRINGREF PhpSizeUnitNamesCounted[7] =
{
    PH_STRINGREF_INIT(L"B"),
    PH_STRINGREF_INIT(L"kB"),
    PH_STRINGREF_INIT(L"MB"),
    PH_STRINGREF_INIT(L"GB"),
    PH_STRINGREF_INIT(L"TB"),
    PH_STRINGREF_INIT(L"PB"),
    PH_STRINGREF_INIT(L"EB")
};

static PH_INITONCE PhpFormatInitOnce = PH_INITONCE_INIT;
static WCHAR PhpFormatDecimalSeparator = '.';
static WCHAR PhpFormatThousandSeparator = ',';
static _locale_t PhpFormatUserLocale = NULL;

PPH_STRING PhFormat(
    __in PPH_FORMAT Format,
    __in ULONG Count,
    __in_opt SIZE_T InitialCapacity
    )
{
    PPH_STRING string;
    SIZE_T allocatedLength;
    PWSTR buffer;
    SIZE_T usedLength;

    // Set up the buffer.

    if (InitialCapacity < SMALL_BUFFER_LENGTH)
        InitialCapacity = SMALL_BUFFER_LENGTH;

    string = PhCreateStringEx(NULL, InitialCapacity);
    allocatedLength = InitialCapacity;
    buffer = string->Buffer;
    usedLength = 0;

#undef ENSURE_BUFFER
#undef OK_BUFFER
#undef ADVANCE_BUFFER

#define ENSURE_BUFFER(NeededLength) \
    do { \
        if (allocatedLength < usedLength + (NeededLength)) \
        { \
            PPH_STRING newString; \
            \
            allocatedLength *= 2; \
            \
            if (allocatedLength < usedLength + (NeededLength)) \
                allocatedLength = usedLength + (NeededLength); \
            \
            newString = PhCreateStringEx(NULL, allocatedLength); \
            memcpy(newString->Buffer, string->Buffer, usedLength); \
            PhDereferenceObject(string); \
            string = newString; \
            buffer = string->Buffer + usedLength / sizeof(WCHAR); \
        } \
    } while (0)

#define OK_BUFFER (TRUE)

#define ADVANCE_BUFFER(Length) \
    do { buffer += (Length) / sizeof(WCHAR); usedLength += (Length); } while (0)

#include "format_i.h"

    string->Length = (USHORT)(usedLength);
    // Null-terminate the string.
    string->Buffer[usedLength / sizeof(WCHAR)] = 0;

    return string;
}

BOOLEAN PhFormatToBuffer(
    __in PPH_FORMAT Format,
    __in ULONG Count,
    __out_bcount_opt(BufferLength) PWSTR Buffer,
    __in_opt SIZE_T BufferLength,
    __out_opt PSIZE_T ReturnLength
    )
{
    PWSTR buffer;
    SIZE_T usedLength;
    BOOLEAN overrun;

    buffer = Buffer;
    usedLength = 0;
    overrun = FALSE;

#undef ENSURE_BUFFER
#undef OK_BUFFER
#undef ADVANCE_BUFFER

#define ENSURE_BUFFER(NeededLength) \
    do { \
        if (!overrun && (BufferLength < usedLength + (NeededLength))) \
            overrun = TRUE; \
    } while (0)

#define OK_BUFFER (Buffer && !overrun)

#define ADVANCE_BUFFER(Length) \
    do { \
        if (Buffer) \
            buffer += (Length) / sizeof(WCHAR); \
        usedLength += (Length); \
    } while (0)

#include "format_i.h"

    // Write the null-terminator.
    ENSURE_BUFFER(sizeof(WCHAR));
    if (OK_BUFFER)
        *buffer = 0;
    else if (Buffer && BufferLength != 0) // try to null-terminate even if this function fails
        *Buffer = 0;
    ADVANCE_BUFFER(sizeof(WCHAR));

    if (ReturnLength)
        *ReturnLength = usedLength;

    return OK_BUFFER;
}
