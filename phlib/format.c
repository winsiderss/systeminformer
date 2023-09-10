/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2020
 *
 */

/*
 * This module provides a high-performance string formatting mechanism. Instead of using format
 * strings, the user supplies an array of structures. This system is 2-5 times faster than
 * printf-based functions.
 *
 * This file contains the public interfaces, while including the real formatting code from
 * elsewhere. There are currently two functions: PhFormat, which returns a string object containing
 * the formatted string, and PhFormatToBuffer, which writes the formatted string to a buffer. The
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
static WCHAR PhpFormatDecimalSeparator = L'.';
static WCHAR PhpFormatThousandSeparator = L',';
static _locale_t PhpFormatUserLocale = NULL;

VOID PhpFormatDoubleToUtf8Locale(
    _In_ DOUBLE Value,
    _In_ ULONG Type,
    _In_ INT32 Precision,
    _Out_writes_bytes_opt_(BufferLength) PSTR Buffer,
    _In_opt_ SIZE_T BufferLength
    )
{
    if (!PhFormatDoubleToUtf8(
        Value,
        Type,
        Precision,
        Buffer,
        BufferLength,
        NULL
        ))
    {
        if (Buffer)
            *Buffer = ANSI_NULL;
        return;
    }

    if (PhpFormatUserLocale && Buffer)
    {
        for (PCH c = Buffer; *c; ++c)
        {
            if (*c == '.')
            {
                *c = (CHAR)PhpFormatDecimalSeparator;
                break;
            }
        }
    }

    if (Type & FormatUpperCase)
    {
        if (PhpFormatUserLocale)
            _strupr_l(Buffer, PhpFormatUserLocale);
        else
            _strupr(Buffer);

        //for (PCH c = Buffer; *c; ++c)
        //    *c = RtlUpperChar(*c);
    }
}

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

        while ((*++Buffer = *stop++) != ANSI_NULL)
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
 * \param InitialCapacity The number of bytes to reserve initially for the string. If 0 is
 * specified, a default value is used.
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

    // If the specified initial capacity is too small (or zero), use the largest buffer size which
    // will still be eligible for allocation from the small object free list.
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
    string->Buffer[usedLength / sizeof(WCHAR)] = UNICODE_NULL;

    return string;
}

/**
 * Writes a formatted string to a buffer.
 *
 * \param Format An array of format structures.
 * \param Count The number of structures supplied in \a Format.
 * \param Buffer A buffer. If NULL, no data is written.
 * \param BufferLength The number of bytes available in \a Buffer, including space for the null
 * terminator.
 * \param ReturnLength The number of bytes required to hold the string, including the null
 * terminator.
 *
 * \return TRUE if the buffer was large enough and the string was written (i.e. \a BufferLength >=
 * \a ReturnLength), otherwise FALSE. In either case, the required number of bytes is stored in
 * \a ReturnLength.
 *
 * \remarks If the function fails but \a BufferLength != 0, a single null byte is written to the
 * start of \a Buffer.
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
        *buffer = UNICODE_NULL;
    else if (Buffer && BufferLength != 0) // try to null-terminate even if this function fails
        *Buffer = UNICODE_NULL;
    ADVANCE_BUFFER(sizeof(WCHAR));

    if (ReturnLength)
        *ReturnLength = usedLength;

    return OK_BUFFER;
}
