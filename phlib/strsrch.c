/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010
 *     dmex    2017-2023
 *     jxy-s   2024
 *
 */

#include <ph.h>
#include <strsrch.h>

typedef struct _PH_STRING_SEARCH_CONEXT
{
    ULONG MinimumLength;
    BOOLEAN ExtendedCharSet;
    PPH_STRING_SEARCH_CALLBACK Callback;
    PVOID CallbackContext;
    WCHAR Buffer[PAGE_SIZE * 2];
} PH_STRING_SEARCH_CONEXT, *PPH_STRING_SEARCH_CONEXT;

typedef enum _PH_CHAR_TYPE
{
    PhCharTypeNone,
    PhCharTypePrintable,
    PhCharTypeNULL,
    PhCharTypeUTF16High,
} PH_CHAR_TYPE, *PPH_CHAR_TYPE;

typedef enum _PH_CHAR_PATTERN
{
    PhCharPatternNone = 0,
    PhCharPatternASCII,
    PhCharPatternUnicode,
} PH_CHAR_PATTERN, *PPH_CHAR_PATTERN;

FORCEINLINE
PH_CHAR_TYPE PhpClassifyByte(
    _In_ BYTE Byte,
    _In_ BOOLEAN ExtendedCharSet,
    _In_ BOOLEAN CheckUTF16High
    )
{
    if (Byte == 0)
    {
        return PhCharTypeNULL;
    }
    else if (ExtendedCharSet ? PhCharIsPrintableEx[Byte] : PhCharIsPrintable[Byte])
    {
        return PhCharTypePrintable;
    }
    else if (CheckUTF16High &&
             !PhIsUTF16HighSurrogateHighByte[Byte] &&
             !PhIsUTF16LowSurrogateHighByte[Byte] &&
             PhIsUTF16StandaloneHighByte[Byte] &&
             (ExtendedCharSet ? TRUE : PhIsUTF16PrintableHighByte[Byte]))
    {
        return PhCharTypeUTF16High;
    }
    else
    {
        return PhCharTypeNone;
    }
}

BOOLEAN PhpSearchStrings(
    _In_ PPH_STRING_SEARCH_CONEXT Context,
    _In_reads_bytes_(Length) PBYTE Buffer,
    _In_ SIZE_T Length
    )
{
    BYTE byte = 0; // current byte
    BYTE byte1 = 0; // byte at position i-1
    BYTE byte2 = 0; // byte at position i-2
    BYTE byte3 = 0; // byte at position i-3
    PH_CHAR_TYPE charType = PhCharTypeNone;  // classification of byte
    PH_CHAR_TYPE charType1 = PhCharTypeNone; // classification of byte1
    PH_CHAR_TYPE charType2 = PhCharTypeNone; // classification of byte2
    PH_CHAR_TYPE charType3 = PhCharTypeNone; // classification of byte3
    PH_CHAR_PATTERN pattern = PhCharPatternNone;
    ULONG length = 0;

    for (SIZE_T i = 0; i < Length; i++)
    {
        BOOLEAN checkUTF8High = FALSE;

        byte = Buffer[i];

        // Classify the current byte.
        // Only check for UTF-16 high bytes when extended character sets is
        // enabled and NOT preceded by UTF-16 high or NULL. This prevents
        // misclassifying low bytes in UTF-16 sequences as high bytes.

        if (Context->ExtendedCharSet &&
            charType1 != PhCharTypeUTF16High &&
            charType1 != PhCharTypeNULL)
        {
            checkUTF8High = TRUE;
        }

        charType = PhpClassifyByte(byte, Context->ExtendedCharSet, checkUTF8High);

        // Pattern: [Printable][Printable][Printable] - ANSI string detection
        if (charType2 == PhCharTypePrintable &&
            charType1 == PhCharTypePrintable &&
            charType == PhCharTypePrintable)
        {
            if (pattern == PhCharPatternNone)
            {
                assert(length == 0);
                pattern = PhCharPatternASCII;
                Context->Buffer[length++] = byte2;
                Context->Buffer[length++] = byte1;
                Context->Buffer[length++] = byte;
            }
            else if (pattern == PhCharPatternASCII)
            {
                if (length < RTL_NUMBER_OF(Context->Buffer))
                    Context->Buffer[length++] = byte;
            }
            else if (length >= Context->MinimumLength)
            {
                goto CreateResult;
            }
            else
            {
                length = 0;
                pattern = PhCharPatternNone;
            }
        }
        // Pattern: [Printable][NULL][Printable] - ASCII-Unicode string detection
        else if (charType2 == PhCharTypePrintable &&
                 charType1 == PhCharTypeNULL &&
                 charType == PhCharTypePrintable)
        {
            if (pattern == PhCharPatternNone)
            {
                assert(length == 0);
                pattern = PhCharPatternUnicode;
                Context->Buffer[length++] = byte2;
                Context->Buffer[length++] = byte;
            }
            else if (pattern == PhCharPatternUnicode)
            {
                if (length < RTL_NUMBER_OF(Context->Buffer))
                    Context->Buffer[length++] = byte;
            }
            else if (length >= Context->MinimumLength)
            {
                goto CreateResult;
            }
            else
            {
                length = 0;
                pattern = PhCharPatternNone;
            }
        }
        // Pattern: [NULL][Printable][NULL] - ASCII-Unicode continuation
        // Handles the NULL between printable characters in ASCII-Unicode strings
        else if (pattern == PhCharPatternUnicode &&
                 charType2 == PhCharTypeNULL &&
                 charType1 == PhCharTypePrintable &&
                 charType == PhCharTypeNULL)
        {
            NOTHING; // Keep tracking, waiting for next printable or UTF-16 high byte
        }
        // Pattern: [NULL][Printable][NULL][None] - ASCII-Unicode to UTF-16 BMP transition
        // Handles transition point where ASCII-Unicode meets UTF-16 BMP
        else if (pattern == PhCharPatternUnicode &&
                 charType3 == PhCharTypeNULL &&
                 charType2 == PhCharTypePrintable &&
                 charType1 == PhCharTypeNULL &&
                 charType == PhCharTypeNone)
        {
            NOTHING; // Keep tracking, next byte should be UTF-16 high byte
        }
        // Pattern: [UTF16High|NULL][!UTF16High][UTF16High] - UTF-16 BMP character detection
        // Detects UTF-16 characters: [low_byte][high_byte]
        else if ((charType2 == PhCharTypeUTF16High || charType2 == PhCharTypeNULL) &&
                 charType1 != PhCharTypeUTF16High &&
                 charType == PhCharTypeUTF16High)
        {
            if (pattern == PhCharPatternNone)
            {
                assert(length == 0);
                pattern = PhCharPatternUnicode;
                Context->Buffer[length++] = MAKEWORD(byte1, byte);
            }
            else if (pattern == PhCharPatternUnicode)
            {
                if (length < RTL_NUMBER_OF(Context->Buffer))
                    Context->Buffer[length++] = MAKEWORD(byte1, byte);
            }
            else if (length >= Context->MinimumLength)
            {
                goto CreateResult;
            }
            else
            {
                length = 0;
                pattern = PhCharPatternNone;
            }
        }
        // Pattern: [UTF16High|NULL][!UTF16High][UTF16High][!UTF16High] - UTF-16 BMP continuation
        // Handles the low byte of the next character in a UTF-16 sequence
        else if (pattern == PhCharPatternUnicode &&
                 (charType3 == PhCharTypeUTF16High || charType3 == PhCharTypeNULL) &&
                 charType2 != PhCharTypeUTF16High &&
                 charType1 == PhCharTypeUTF16High &&
                 charType != PhCharTypeUTF16High)
        {
            NOTHING; // Keep tracking, waiting for next UTF-16 high byte
        }
        // Pattern: [!UTF16High][UTF16High][Printable][NULL] - UTF-16 BMP to ASCII-Unicode transition
        // Handles transition from UTF-16 BMP back to ASCII-Unicode
        else if (pattern == PhCharPatternUnicode &&
                 charType3 != PhCharTypeUTF16High &&
                 charType2 == PhCharTypeUTF16High &&
                 charType1 == PhCharTypePrintable &&
                 charType == PhCharTypeNULL)
        {
            if (length < RTL_NUMBER_OF(Context->Buffer))
                Context->Buffer[length++] = byte1;
        }
        else if (pattern != PhCharPatternNone &&
                 length >= Context->MinimumLength)
        {
            goto CreateResult;
        }
        else
        {
            length = 0;
            pattern = PhCharPatternNone;
        }

        goto AfterCreateResult;

CreateResult:
        {
            PH_STRING_SEARCH_RESULT result;
            ULONG lengthInBytes;
            ULONG bias;
            BOOLEAN isWide;
            BOOLEAN isFinished;

            lengthInBytes = length;
            bias = (charType == PhCharTypePrintable);
            isWide = (pattern != PhCharPatternASCII);

            if (isWide)
            {
                lengthInBytes *= 2;
            }

            result.Unicode = isWide;
            result.Address = PTR_ADD_OFFSET(Buffer, i - bias - lengthInBytes);
            result.Length = lengthInBytes;
            result.String = PhCreateStringEx(
                Context->Buffer,
                2 * min(length, RTL_NUMBER_OF(Context->Buffer))
                );

            isFinished = Context->Callback(&result, Context->CallbackContext);

            PhClearReference(&result.String);
            pattern = PhCharPatternNone;
            length = 0;

            if (isFinished)
                return TRUE;
        }
AfterCreateResult:

        byte3 = byte2;
        byte2 = byte1;
        byte1 = byte;
        charType3 = charType2;
        charType2 = charType1;
        charType1 = charType;
    }

    return FALSE;
}

NTSTATUS PhSearchStrings(
    _In_ ULONG MinimumLength,
    _In_ BOOLEAN ExtendedCharSet,
    _In_ PPH_STRING_SEARCH_NEXT_BUFFER NextBuffer,
    _In_ PPH_STRING_SEARCH_CALLBACK Callback,
    _In_opt_ PVOID Context
    )
{
    NTSTATUS status;
    PPH_STRING_SEARCH_CONEXT context;
    PVOID buffer;
    SIZE_T length;

    if (!MinimumLength)
        return STATUS_INVALID_PARAMETER;

    context = PhAllocateZero(sizeof(PH_STRING_SEARCH_CONEXT));
    context->MinimumLength = MinimumLength;
    context->ExtendedCharSet = ExtendedCharSet;
    context->Callback = Callback;
    context->CallbackContext = Context;

    buffer = NULL;
    length = 0;

    while (NT_SUCCESS(status = NextBuffer(&buffer, &length, Context)))
    {
        if (!length)
            break;

        if (PhpSearchStrings(context, buffer, length))
            break;
    }

    PhFree(context);

    return status;
}
