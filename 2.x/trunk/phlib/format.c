/*
 * Process Hacker -
 *   string formatting
 *
 * Copyright (C) 2010-2015 wj32
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
 * structures. This system is 2-5 times faster than printf-based functions.
 *
 * This file contains the public interfaces, while including the real
 * formatting code from elsewhere. There are currently two functions:
 * PhFormat, which returns a string object containing the formatted string,
 * and PhFormatToBuffer, which writes the formatted string to a buffer. The
 * latter is a bit faster due to the lack of resizing logic.
 */

#include <phbase.h>
#include <locale.h>

extern ULONG PhMaxSizeUnit;

#define SMALL_BUFFER_LENGTH (PH_OBJECT_SMALL_OBJECT_SIZE - FIELD_OFFSET(PH_STRING, Data) - sizeof(WCHAR))
#define BUFFER_SIZE 512

#define PHP_FORMAT_NEGATIVE 0x1
#define PHP_FORMAT_POSITIVE 0x2
#define PHP_FORMAT_PAD 0x4

// Internal CRT routine needed for floating-point conversion

errno_t __cdecl _cfltcvt_l(double *arg, char *buffer, size_t sizeInBytes,
    int format, int precision, int caps, _locale_t plocinfo);

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

#if (_MSC_VER >= 1900)

// See Source\10.0.10150.0\ucrt\convert\cvt.cpp in SDK v10.
errno_t __cdecl __acrt_fp_format(
    double const* const value,
    char*         const result_buffer,
    size_t        const result_buffer_count,
    char*         const scratch_buffer,
    size_t        const scratch_buffer_count,
    int           const format,
    int           const precision,
    UINT64        const options,
    _locale_t     const locale
    );

static errno_t __cdecl _cfltcvt_l(double *arg, char *buffer, size_t sizeInBytes,
    int format, int precision, int caps, _locale_t plocinfo)
{
    char scratch_buffer[_CVTBUFSIZE + 1];

    if (caps & 1)
        format -= 32; // Make uppercase

    return __acrt_fp_format(arg, buffer, sizeInBytes, scratch_buffer, sizeof(scratch_buffer),
        format, precision, 0, plocinfo);
}

#endif

// From Source\10.0.10150.0\ucrt\inc\corecrt_internal_stdio_output.h in SDK v10.
VOID PhpCropZeros(
    _Inout_ PCHAR Buffer,
    _In_ _locale_t Locale
    )
{
    CHAR decimalSeparator = (CHAR)PhpFormatDecimalSeparator;

    while (*Buffer && *Buffer != decimalSeparator)
        ++Buffer;

    if (*Buffer++)
    {
        while (*Buffer && *Buffer != 'e' && *Buffer != 'E')
            ++Buffer;

        PCHAR stop = Buffer--;

        while (*Buffer == '0')
            --Buffer;

        if (*Buffer == decimalSeparator)
            --Buffer;

        while ((*++Buffer = *stop++) != '\0')
            NOTHING;
    }
}

PPH_STRING PhpResizeFormatBuffer(
    _In_ PPH_STRING String,
    _Inout_ PSIZE_T AllocatedLength,
    _In_ SIZE_T UsedLength,
    _In_ SIZE_T NeededLength
    )
{
    PPH_STRING newString;
    SIZE_T allocatedLength;

    allocatedLength = *AllocatedLength;
    allocatedLength *= 2;

    if (allocatedLength < UsedLength + NeededLength)
        allocatedLength = UsedLength + NeededLength;

    newString = PhCreateStringEx(NULL, allocatedLength);
    memcpy(newString->Buffer, String->Buffer, UsedLength);
    PhDereferenceObject(String);

    *AllocatedLength = allocatedLength;

    return newString;
}

/**
 * Creates a formatted string.
 *
 * \param Format An array of format structures.
 * \param Count The number of structures supplied in \a Format.
 * \param InitialCapacity The number of bytes to reserve initially for
 * the string. If 0 is specified, a default value is used.
 */
PPH_STRING PhFormat(
    _In_reads_(Count) PPH_FORMAT Format,
    _In_ ULONG Count,
    _In_opt_ SIZE_T InitialCapacity
    )
{
    PPH_STRING string;
    SIZE_T allocatedLength;
    PWSTR buffer;
    SIZE_T usedLength;

    // Set up the buffer.

    // If the specified initial capacity is too small (or zero), use the
    // largest buffer size which will still be eligible for allocation from
    // the small object free list.
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
            string = PhpResizeFormatBuffer(string, &allocatedLength, usedLength, (NeededLength)); \
            buffer = string->Buffer + usedLength / sizeof(WCHAR); \
        } \
    } while (0)

#define OK_BUFFER (TRUE)

#define ADVANCE_BUFFER(Length) \
    do { buffer += (Length) / sizeof(WCHAR); usedLength += (Length); } while (0)

#include "format_i.h"

    string->Length = usedLength;
    // Null-terminate the string.
    string->Buffer[usedLength / sizeof(WCHAR)] = 0;

    return string;
}

/**
 * Writes a formatted string to a buffer.
 *
 * \param Format An array of format structures.
 * \param Count The number of structures supplied in \a Format.
 * \param Buffer A buffer. If NULL, no data is written.
 * \param BufferLength The number of bytes available in \a Buffer,
 * including space for the null terminator.
 * \param ReturnLength The number of bytes required to hold the
 * string, including the null terminator.
 *
 * \return TRUE if the buffer was large enough and the string was
 * written (i.e. \a BufferLength >= \a ReturnLength), otherwise
 * FALSE. In either case, the required number of bytes is stored
 * in \a ReturnLength.
 *
 * \remarks If the function fails but \a BufferLength != 0, a
 * single null byte is written to the start of \a Buffer.
 */
BOOLEAN PhFormatToBuffer(
    _In_reads_(Count) PPH_FORMAT Format,
    _In_ ULONG Count,
    _Out_writes_bytes_opt_(BufferLength) PWSTR Buffer,
    _In_opt_ SIZE_T BufferLength,
    _Out_opt_ PSIZE_T ReturnLength
    )
{
    PWSTR buffer;
    SIZE_T usedLength;
    BOOLEAN overrun;

    buffer = Buffer;
    usedLength = 0;
    overrun = FALSE;

    // Make sure we don't try to write anything if we don't have a buffer.
    if (!Buffer)
        overrun = TRUE;

#undef ENSURE_BUFFER
#undef OK_BUFFER
#undef ADVANCE_BUFFER

#define ENSURE_BUFFER(NeededLength) \
    do { \
        if (!overrun && (BufferLength < usedLength + (NeededLength))) \
            overrun = TRUE; \
    } while (0)

#define OK_BUFFER (!overrun)

#define ADVANCE_BUFFER(Length) \
    do { buffer += (Length) / sizeof(WCHAR); usedLength += (Length); } while (0)

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
