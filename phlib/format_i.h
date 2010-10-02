{
    if (PhBeginInitOnce(&PhpFormatInitOnce))
    {
        WCHAR localeBuffer[4];

        if (
            GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, localeBuffer, 4) &&
            (localeBuffer[0] != 0 && localeBuffer[1] == 0)
            )
        {
            PhpFormatDecimalSeparator = localeBuffer[0];
        }

        if (
            GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, localeBuffer, 4) &&
            (localeBuffer[0] != 0 && localeBuffer[1] == 0)
            )
        {
            PhpFormatThousandSeparator = localeBuffer[0];
        }

        if (PhpFormatDecimalSeparator != '.')
            PhpFormatUserLocale = _create_locale(LC_ALL, "");

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

        // Save the currently used length so we can compute the 
        // part length later.
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

                count = wcslen(format->u.StringZ);
                ENSURE_BUFFER(count * sizeof(WCHAR));
                if (OK_BUFFER)
                    memcpy(buffer, format->u.StringZ, count * sizeof(WCHAR));
                ADVANCE_BUFFER(count * sizeof(WCHAR));
            }
            break;
        case AnsiStringFormatType:
        case AnsiStringZFormatType:
            {
                ULONG bytesInUnicodeString;
                PSTR ansiBuffer;
                SIZE_T ansiLength;

                if (format->Type == AnsiStringFormatType)
                {
                    ansiBuffer = format->u.AnsiString.Buffer;
                    ansiLength = format->u.AnsiString.Length;
                }
                else
                {
                    ansiBuffer = format->u.AnsiStringZ;
                    ansiLength = strlen(ansiBuffer);
                }

                if (NT_SUCCESS(RtlMultiByteToUnicodeSize(
                    &bytesInUnicodeString,
                    ansiBuffer,
                    (ULONG)ansiLength
                    )))
                {
                    ENSURE_BUFFER(bytesInUnicodeString);

                    if (!OK_BUFFER || NT_SUCCESS(RtlMultiByteToUnicodeN(
                        buffer,
                        bytesInUnicodeString,
                        NULL,
                        ansiBuffer,
                        (ULONG)ansiLength
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
        PCHAR integerToChar; \
        PWSTR temp; \
        ULONG tempCount; \
        ULONG r; \
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
        if (flags & PHP_FORMAT_NEGATIVE) \
        { \
            *temp-- = '-'; \
            tempCount++; \
        } \
        else if ((Format)->Type & FormatPrefixSign) \
        { \
            *temp-- = '+'; \
            tempCount++; \
        } \
        \
        temp++; \
        ENSURE_BUFFER(tempCount * sizeof(WCHAR)); \
        if (OK_BUFFER) \
            memcpy(buffer, temp, tempCount * sizeof(WCHAR)); \
        ADVANCE_BUFFER(tempCount * sizeof(WCHAR)); \
    } while (0)

#ifdef _M_IX86
        case IntPtrFormatType:
            int32 = format->u.IntPtr;
#endif
        case Int32FormatType:
            int32 = format->u.Int32;

            if ((LONG)int32 < 0)
            {
                int32 = -(LONG)int32;
                flags |= PHP_FORMAT_NEGATIVE;
            }

            goto CommonInt32Format;
#ifdef _M_IX86
        case UIntPtrFormatType:
            int32 = format->u.UIntPtr;
#endif
        case UInt32FormatType:
            int32 = format->u.UInt32;
CommonInt32Format:
            COMMON_INTEGER_FORMAT(int32, format);
            break;
#ifndef _M_IX86
        case IntPtrFormatType:
            int64 = format->u.IntPtr;
#endif
        case Int64FormatType:
            int64 = format->u.Int64;

            if ((LONG64)int64 < 0)
            {
                int64 = -(LONG64)int64;
                flags |= PHP_FORMAT_NEGATIVE;
            }

            goto CommonInt64Format;
#ifndef _M_IX86
        case UIntPtrFormatType:
            int64 = format->u.UIntPtr;
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
        CHAR c; \
        ULONG length; \
        \
        if ((Format)->Type & FormatUsePrecision) \
        { \
            precision = (Format)->Precision; \
            \
            if (precision > BUFFER_SIZE - _CVTBUFSIZE) \
                precision = BUFFER_SIZE - _CVTBUFSIZE; \
        } \
        else \
        { \
            precision = 6; \
        } \
        \
        c = 'f'; \
        \
        if ((Format)->Type & FormatStandardForm) \
            c = 'e'; \
        else if ((Format)->Type & FormatHexadecimalForm) \
            c = 'a'; \
        \
        if ((Format)->Type & FormatUpperCase) \
            c -= 32; /* uppercase the format type */ \
        \
        /* Use MS CRT routines to do the work. */ \
        \
        value = (Format)->u.Double; \
        _cfltcvt_l( \
            &value, \
            (PSTR)tempBuffer, \
            sizeof(tempBuffer), \
            c, \
            precision, \
            !!((Format)->Type & FormatUpperCase), \
            PhpFormatUserLocale \
            ); \
        \
        /* if (((Format)->Type & FormatForceDecimalPoint) && precision == 0) */ \
             /* _forcdecpt_l(tempBufferAnsi, PhpFormatUserLocale); */ \
        if ((Format)->Type & FormatCropZeros) \
            _cropzeros_l((PSTR)tempBuffer, PhpFormatUserLocale); \
        \
        length = (ULONG)strlen((PSTR)tempBuffer); \
        \
        if (((Format)->Type & FormatGroupDigits) && !((Format)->Type & (FormatStandardForm | FormatHexadecimalForm))) \
        { \
            PSTR whole; \
            PSTR decimalPoint; \
            ULONG wholeCount; \
            ULONG sepsCount; \
            ULONG copyCount; \
            ULONG needsSep; \
            \
            /* Find the first non-digit character and assume that is the */ \
            /* decimal point (or the end of the string). */ \
            \
            whole = (PSTR)tempBuffer; \
            decimalPoint = (PSTR)tempBuffer; \
            \
            while ((UCHAR)(*decimalPoint - '0') < 10) \
                decimalPoint++; \
            \
            /* Copy the characters to the output buffer, and at the same time */ \
            /* insert the separators. */ \
            \
            wholeCount = (ULONG)(decimalPoint - (PSTR)tempBuffer); \
            \
            if (wholeCount != 0) \
                sepsCount = (wholeCount + 2) / 3 - 1; \
            else \
                sepsCount = 0; \
            \
            ENSURE_BUFFER((length + sepsCount) * sizeof(WCHAR)); \
            \
            copyCount = wholeCount; \
            needsSep = (wholeCount + 2) % 3; \
            \
            if (OK_BUFFER) \
            { \
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
            usedLength += (wholeCount + sepsCount) * sizeof(WCHAR); \
            \
            /* Copy the rest. */ \
            \
            copyCount = length - wholeCount; \
            \
            if (!OK_BUFFER || NT_SUCCESS(RtlMultiByteToUnicodeN( \
                buffer, \
                copyCount * sizeof(WCHAR), \
                NULL, \
                decimalPoint, \
                copyCount \
                ))) \
            { \
                ADVANCE_BUFFER(copyCount * sizeof(WCHAR)); \
            } \
        } \
        else \
        { \
            /* We don't need to group digits, so directly copy the characters */ \
            /* to the output buffer. */ \
            \
            ENSURE_BUFFER(length * sizeof(WCHAR)); \
            \
            if (!OK_BUFFER || NT_SUCCESS(RtlMultiByteToUnicodeN( \
                buffer, \
                length * sizeof(WCHAR), \
                NULL, \
                (PSTR)tempBuffer, \
                length \
                ))) \
            { \
                ADVANCE_BUFFER(length * sizeof(WCHAR)); \
            } \
        } \
    } while (0)

        case DoubleFormatType:
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

                if (format->Type & FormatUsePrecision)
                    maxSizeUnit = format->Precision;
                else
                    maxSizeUnit = PhMaxSizeUnit;

                while (
                    s > 1024 &&
                    i < sizeof(PhpSizeUnitNamesCounted) / sizeof(PH_STRINGREF) &&
                    i < maxSizeUnit
                    )
                {
                    s /= 1024;
                    i++;
                }

                // Format the number, then append the unit name.

                doubleFormat.Type = DoubleFormatType | FormatUsePrecision | FormatCropZeros | FormatGroupDigits;
                doubleFormat.Precision = 2;
                doubleFormat.u.Double = s;
                COMMON_DOUBLE_FORMAT(&doubleFormat);

                ENSURE_BUFFER(sizeof(WCHAR) + PhpSizeUnitNamesCounted[i].Length);
                if (OK_BUFFER)
                {
                    *buffer = ' ';
                    memcpy(buffer + 1, PhpSizeUnitNamesCounted[i].Buffer, PhpSizeUnitNamesCounted[i].Length);
                }
                ADVANCE_BUFFER(sizeof(WCHAR) + PhpSizeUnitNamesCounted[i].Length);
            }
            break;
        }

ContinueLoop:
        partLength = usedLength - partLength;
    }
}
