/*
 * This file contains the actual formatting code used by various public interface functions.
 *
 * There are three macros defined by the parent function which control how this code writes the
 * formatted string:
 * * ENSURE_BUFFER - This macro is passed the number of bytes required whenever characters need to
 *   be written to the buffer. The macro can resize the buffer if needed.
 * * OK_BUFFER - This macro returns TRUE if it is OK to write to the buffer, otherwise FALSE when
 *   the buffer is too large, is not specified, or some other error has occurred.
 * * ADVANCE_BUFFER - This macro is passed the number of bytes written to the buffer and should
 *   increment the "buffer" pointer and "usedLength" counter.
 * In addition to these macros, the "buffer" and "usedLength" variables are assumed to be present.
 *
 * The below code defines many macros; this is so that composite formatting types can be constructed
 * (e.g. the "size" type).
 */

{
    if (PhBeginInitOnce(&PhpFormatInitOnce))
    {
        WCHAR localeBuffer[4];

        if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, localeBuffer, 4) &&
            (localeBuffer[0] != 0 && localeBuffer[1] == 0))
        {
            PhpFormatDecimalSeparator = localeBuffer[0];
        }

        if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, localeBuffer, 4) &&
            (localeBuffer[0] != 0 && localeBuffer[1] == 0))
        {
            PhpFormatThousandSeparator = localeBuffer[0];
        }

        if (PhpFormatDecimalSeparator != L'.')
            PhpFormatUserLocale = _wcreate_locale(LC_ALL, L"");

        PhEndInitOnce(&PhpFormatInitOnce);
    }

    while (Count--)
    {
        PPH_FORMAT format;
        SIZE_T partLength;
        WCHAR tempBuffer[BUFFER_SIZE];
        ULONG flags;
        ULONG int32;
        ULONG64 int64;

        format = Format++;

        // Save the currently used length so we can compute the part length later.
        partLength = usedLength;

        flags = 0;

        switch (format->Type & FormatTypeMask)
        {

        // Characters and Strings

        case CharFormatType:
            ENSURE_BUFFER(sizeof(WCHAR));
            if (OK_BUFFER)
                *buffer = format->u.Char;
            ADVANCE_BUFFER(sizeof(WCHAR));
            break;
        case StringFormatType:
            ENSURE_BUFFER(format->u.String.Length);
            if (OK_BUFFER)
                memcpy(buffer, format->u.String.Buffer, format->u.String.Length);
            ADVANCE_BUFFER(format->u.String.Length);
            break;
        case StringZFormatType:
            {
                SIZE_T count;

                count = PhCountStringZ(format->u.StringZ);
                ENSURE_BUFFER(count * sizeof(WCHAR));
                if (OK_BUFFER)
                    memcpy(buffer, format->u.StringZ, count * sizeof(WCHAR));
                ADVANCE_BUFFER(count * sizeof(WCHAR));
            }
            break;
        case MultiByteStringFormatType:
        case MultiByteStringZFormatType:
            {
                ULONG bytesInUnicodeString;
                PSTR multiByteBuffer;
                SIZE_T multiByteLength;

                if (format->Type == MultiByteStringFormatType)
                {
                    multiByteBuffer = format->u.MultiByteString.Buffer;
                    multiByteLength = format->u.MultiByteString.Length;
                }
                else
                {
                    multiByteBuffer = format->u.MultiByteStringZ;
                    multiByteLength = strlen(multiByteBuffer);
                }

                if (NT_SUCCESS(RtlMultiByteToUnicodeSize(
                    &bytesInUnicodeString,
                    multiByteBuffer,
                    (ULONG)multiByteLength
                    )))
                {
                    ENSURE_BUFFER(bytesInUnicodeString);

                    if (!OK_BUFFER || NT_SUCCESS(RtlMultiByteToUnicodeN(
                        buffer,
                        bytesInUnicodeString,
                        NULL,
                        multiByteBuffer,
                        (ULONG)multiByteLength
                        )))
                    {
                        ADVANCE_BUFFER(bytesInUnicodeString);
                    }
                }
            }
            break;

        // Integers

#define PROCESS_DIGIT(Input) \
    do { \
        r = (ULONG)(Input % radix); \
        Input /= radix; \
        *temp-- = integerToChar[r]; \
        tempCount++; \
    } while (0)

#define COMMON_INTEGER_FORMAT(Input, Format) \
    do { \
        ULONG radix; \
        PCCH integerToChar; \
        PWSTR temp; \
        ULONG tempCount; \
        ULONG r; \
        ULONG preCount; \
        ULONG padCount; \
        \
        radix = 10; \
        if (((Format)->Type & FormatUseRadix) && (Format)->Radix >= 2 && (Format)->Radix <= 69) \
            radix = (Format)->Radix; \
        integerToChar = PhIntegerToChar; \
        if ((Format)->Type & FormatUpperCase) \
            integerToChar = PhIntegerToCharUpper; \
        temp = tempBuffer + BUFFER_SIZE - 1; \
        tempCount = 0; \
        \
        if (Input != 0) \
        { \
            if ((Format)->Type & FormatGroupDigits) \
            { \
                ULONG needsSep = 0; \
                \
                do \
                { \
                    PROCESS_DIGIT(Input); \
                    \
                    if (++needsSep == 3 && Input != 0) /* get rid of trailing separator */ \
                    { \
                        *temp-- = PhpFormatThousandSeparator; \
                        tempCount++; \
                        needsSep = 0; \
                    } \
                } while (Input != 0); \
            } \
            else \
            { \
                do \
                { \
                    PROCESS_DIGIT(Input); \
                } while (Input != 0); \
            } \
        } \
        else \
        { \
            *temp-- = '0'; \
            tempCount++; \
        } \
        \
        preCount = 0; \
        \
        if (flags & PHP_FORMAT_NEGATIVE) \
            preCount++; \
        else if ((Format)->Type & FormatPrefixSign) \
            preCount++; \
        \
        if (((Format)->Type & FormatPadZeros) && !((Format)->Type & FormatGroupDigits)) \
        { \
            if (preCount + tempCount < (Format)->Width) \
            { \
                flags |= PHP_FORMAT_PAD; \
                padCount = (Format)->Width - (preCount + tempCount); \
                preCount += padCount; \
            } \
        } \
        \
        temp++; \
        ENSURE_BUFFER((preCount + tempCount) * sizeof(WCHAR)); \
        if (OK_BUFFER) \
        { \
            if (flags & PHP_FORMAT_NEGATIVE) \
                *buffer++ = L'-'; \
            else if ((Format)->Type & FormatPrefixSign) \
                *buffer++ = L'+'; \
            \
            if (flags & PHP_FORMAT_PAD) \
            { \
                wmemset(buffer, '0', padCount); \
                buffer += padCount; \
            } \
            \
            memcpy(buffer, temp, tempCount * sizeof(WCHAR)); \
            buffer += tempCount; \
        } \
        usedLength += (preCount + tempCount) * sizeof(WCHAR); \
    } while (0)

#ifndef _WIN64
        case IntPtrFormatType:
            int32 = format->u.IntPtr;
            goto CommonMaybeNegativeInt32Format;
#endif
        case Int32FormatType:
            int32 = format->u.Int32;

#ifndef _WIN64
CommonMaybeNegativeInt32Format:
#endif
            if ((LONG)int32 < 0)
            {
                int32 = -(LONG)int32;
                flags |= PHP_FORMAT_NEGATIVE;
            }

            goto CommonInt32Format;
#ifndef _WIN64
        case UIntPtrFormatType:
            int32 = format->u.UIntPtr;
            goto CommonInt32Format;
#endif
        case UInt32FormatType:
            int32 = format->u.UInt32;
CommonInt32Format:
            COMMON_INTEGER_FORMAT(int32, format);
            break;
#ifdef _WIN64
        case IntPtrFormatType:
            int64 = format->u.IntPtr;
            goto CommonMaybeNegativeInt64Format;
#endif
        case Int64FormatType:
            int64 = format->u.Int64;

#ifdef _WIN64
CommonMaybeNegativeInt64Format:
#endif
            if ((LONG64)int64 < 0)
            {
                int64 = -(LONG64)int64;
                flags |= PHP_FORMAT_NEGATIVE;
            }

            goto CommonInt64Format;
#ifdef _WIN64
        case UIntPtrFormatType:
            int64 = format->u.UIntPtr;
            goto CommonInt64Format;
#endif
        case UInt64FormatType:
            int64 = format->u.UInt64;
CommonInt64Format:
            COMMON_INTEGER_FORMAT(int64, format);
            break;

        // Floating point numbers

#define COMMON_DOUBLE_FORMAT(Format) \
    do { \
        ULONG precision; \
        DOUBLE value; \
        PSTR temp; \
        ULONG length; \
        \
        if ((Format)->Type & FormatUsePrecision) \
        { \
            precision = (Format)->Precision; \
            \
            if (precision > BUFFER_SIZE - 1 - _CVTBUFSIZE) \
                precision = BUFFER_SIZE - 1 - _CVTBUFSIZE; \
        } \
        else \
        { \
            precision = 6; \
        } \
        \
        value = (Format)->u.Double; \
        temp = (PSTR)tempBuffer + 1; /* leave one character so we can insert a prefix if needed */ \
        PhpFormatDoubleToUtf8Locale( \
            value, \
            (Format)->Type, \
            precision, \
            temp, \
            sizeof(tempBuffer) - 1 \
            ); \
        \
        /* if (((Format)->Type & FormatForceDecimalPoint) && precision == 0) */ \
             /* _forcdecpt_l(tempBufferAnsi, PhpFormatUserLocale); */ \
        if ((Format)->Type & FormatCropZeros) \
            PhpCropZeros(temp, PhpFormatUserLocale); \
        \
        length = (ULONG)strlen(temp); \
        \
        if (temp[0] == '-') \
        { \
            flags |= PHP_FORMAT_NEGATIVE; \
            temp++; \
            length--; \
        } \
        else if ((Format)->Type & FormatPrefixSign) \
        { \
            flags |= PHP_FORMAT_POSITIVE; \
        } \
        \
        if (((Format)->Type & FormatGroupDigits) && !((Format)->Type & (FormatStandardForm | FormatHexadecimalForm))) \
        { \
            PSTR whole; \
            PSTR decimalPoint; \
            ULONG wholeCount; \
            ULONG sepsCount; \
            ULONG ensureLength; \
            ULONG copyCount; \
            ULONG needsSep; \
            \
            /* Find the first non-digit character and assume that is the */ \
            /* decimal point (or the end of the string). */ \
            \
            whole = temp; \
            decimalPoint = temp; \
            \
            while ((UCHAR)(*decimalPoint - '0') < 10) \
                decimalPoint++; \
            \
            /* Copy the characters to the output buffer, and at the same time */ \
            /* insert the separators. */ \
            \
            wholeCount = (ULONG)(decimalPoint - temp); \
            \
            if (wholeCount != 0) \
                sepsCount = (wholeCount + 2) / 3 - 1; \
            else \
                sepsCount = 0; \
            \
            ensureLength = (length + sepsCount) * sizeof(WCHAR); \
            if (flags & (PHP_FORMAT_NEGATIVE | PHP_FORMAT_POSITIVE)) \
                ensureLength += sizeof(WCHAR); \
            ENSURE_BUFFER(ensureLength); \
            \
            copyCount = wholeCount; \
            needsSep = (wholeCount + 2) % 3; \
            \
            if (OK_BUFFER) \
            { \
                if (flags & PHP_FORMAT_NEGATIVE) \
                    *buffer++ = L'-'; \
                else if (flags & PHP_FORMAT_POSITIVE) \
                    *buffer++ = L'+'; \
                \
                while (copyCount--) \
                { \
                    *buffer++ = *whole++; \
                    \
                    if (needsSep-- == 0 && copyCount != 0) /* get rid of trailing separator */ \
                    { \
                        *buffer++ = PhpFormatThousandSeparator; \
                        needsSep = 2; \
                    } \
                } \
            } \
            \
            if (flags & (PHP_FORMAT_NEGATIVE | PHP_FORMAT_POSITIVE)) \
                usedLength += sizeof(WCHAR); \
            usedLength += (wholeCount + sepsCount) * sizeof(WCHAR); \
            \
            /* Copy the rest. */ \
            \
            copyCount = length - wholeCount; \
            \
            if (OK_BUFFER) \
            { \
                PhZeroExtendToUtf16Buffer(decimalPoint, copyCount, buffer); \
                ADVANCE_BUFFER(copyCount * sizeof(WCHAR)); \
            } \
        } \
        else \
        { \
            SIZE_T preLength; \
            SIZE_T padLength; \
            \
            /* Take care of the sign and zero padding. */ \
            preLength = 0; \
            \
            if (flags & (PHP_FORMAT_NEGATIVE | PHP_FORMAT_POSITIVE)) \
                preLength++; \
            \
            if ((Format)->Type & FormatPadZeros) \
            { \
                if (preLength + length < (Format)->Width) \
                { \
                    flags |= PHP_FORMAT_PAD; \
                    padLength = (Format)->Width - (preLength + length); \
                    preLength += padLength; \
                } \
            } \
            /* We don't need to group digits, so directly copy the characters */ \
            /* to the output buffer. */ \
            \
            ENSURE_BUFFER((preLength + length) * sizeof(WCHAR)); \
            \
            if (OK_BUFFER) \
            { \
                if (flags & PHP_FORMAT_NEGATIVE) \
                    *buffer++ = L'-'; \
                else if (flags & PHP_FORMAT_POSITIVE) \
                    *buffer++ = L'+'; \
                \
                if (flags & PHP_FORMAT_PAD) \
                { \
                    wmemset(buffer, '0', padLength); \
                    buffer += padLength; \
                } \
            } \
            \
            usedLength += preLength * sizeof(WCHAR); \
            \
            if (OK_BUFFER) \
            { \
                PhZeroExtendToUtf16Buffer((PSTR)temp, length, buffer); \
                ADVANCE_BUFFER(length * sizeof(WCHAR)); \
            } \
        } \
    } while (0)

        case DoubleFormatType:
            flags = 0;
            COMMON_DOUBLE_FORMAT(format);
            break;

        // Additional types

        case SizeFormatType:
            {
                ULONG i = 0;
                ULONG maxSizeUnit;
                DOUBLE s;
                PH_FORMAT doubleFormat;

                s = (DOUBLE)format->u.Size;

                if (format->u.Size == 0)
                {
                    ENSURE_BUFFER(sizeof(WCHAR));
                    if (OK_BUFFER)
                        *buffer = '0';
                    ADVANCE_BUFFER(sizeof(WCHAR));
                    goto ContinueLoop;
                }

                if (format->Type & FormatUseRadix)
                    maxSizeUnit = format->Radix;
                else
                    maxSizeUnit = PhMaxSizeUnit;

                while (
                    s >= 1000 &&
                    i < sizeof(PhpSizeUnitNamesCounted) / sizeof(PH_STRINGREF) &&
                    i < maxSizeUnit
                    )
                {
                    s /= 1024;
                    i++;
                }

                // Format the number, then append the unit name.

                doubleFormat.Type = DoubleFormatType | FormatUsePrecision | FormatCropZeros | FormatGroupDigits;
                doubleFormat.Precision = (format->Type & FormatUsePrecision) ? format->Precision : 2;
                doubleFormat.Width = 0; // stupid compiler
                doubleFormat.u.Double = s;
                flags = 0;
                COMMON_DOUBLE_FORMAT(&doubleFormat);

                ENSURE_BUFFER(sizeof(WCHAR) + PhpSizeUnitNamesCounted[i].Length);
                if (OK_BUFFER)
                {
                    *buffer = L' ';
                    memcpy(buffer + 1, PhpSizeUnitNamesCounted[i].Buffer, PhpSizeUnitNamesCounted[i].Length);
                }
                ADVANCE_BUFFER(sizeof(WCHAR) + PhpSizeUnitNamesCounted[i].Length);
            }
            break;
        }

ContinueLoop:
        partLength = usedLength - partLength;

        if (format->Type & (FormatLeftAlign | FormatRightAlign))
        {
            SIZE_T newLength;
            SIZE_T addLength;

            newLength = format->Width * sizeof(WCHAR);

            // We only pad and never truncate.
            if (partLength < newLength)
            {
                addLength = newLength - partLength;
                ENSURE_BUFFER(addLength);

                if (OK_BUFFER)
                {
                    WCHAR pad;

                    if (format->Type & FormatUsePad)
                        pad = format->Pad;
                    else
                        pad = L' ';

                    if (format->Type & FormatLeftAlign)
                    {
                        // Left alignment is easy; we just fill the remaining space with the pad
                        // character.
                        wmemset(buffer, pad, addLength / sizeof(WCHAR));
                    }
                    else
                    {
                        PWSTR start;

                        // Right alignment is much slower and involves moving the text forward, then
                        // filling in the space before it.
                        start = buffer - partLength / sizeof(WCHAR);
                        memmove(start + addLength / sizeof(WCHAR), start, partLength);
                        wmemset(start, pad, addLength / sizeof(WCHAR));
                    }
                }

                ADVANCE_BUFFER(addLength);
            }
        }
    }
}
