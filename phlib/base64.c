/*
 * Copyright (c) 2026 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 */

#include <phbase.h>
#include "include\base64.h"

static const CHAR PhBase64EncodeTable[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const UCHAR PhBase64DecodeTable[256] =
{
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 62,   0xFF, 0xFF, 0xFF, 63,
    52,   53,   54,   55,   56,   57,   58,   59,   60,   61,   0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF,
    0xFF, 0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    10,   11,   12,   13,   14,
    15,   16,   17,   18,   19,   20,   21,   22,   23,   24,   25,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 26,   27,   28,   29,   30,   31,   32,   33,   34,   35,   36,   37,   38,   39,   40,
    41,   42,   43,   44,   45,   46,   47,   48,   49,   50,   51,   0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

static BOOLEAN PhBase64IsWhitespace(
    _In_ UCHAR Value
    )
{
    return Value == ' ' || Value == '\t' || Value == '\r' || Value == '\n';
}

BOOLEAN PhBase64Encode(
    _In_reads_bytes_(InputLength) const UCHAR* Input,
    _In_ SIZE_T InputLength,
    _Out_writes_z_(OutputLength) PSTR Output,
    _In_ SIZE_T OutputLength,
    _Out_opt_ PSIZE_T ResultLength
    )
{
    SIZE_T requiredLength = ((InputLength + 2) / 3) * 4 + 1;
    SIZE_T inputIndex = 0;
    SIZE_T outputIndex = 0;

    if (ResultLength)
        *ResultLength = 0;

    if (OutputLength < requiredLength)
        return FALSE;

    while (inputIndex + 3 <= InputLength)
    {
        ULONG value = ((ULONG)Input[inputIndex] << 16) | ((ULONG)Input[inputIndex + 1] << 8) | Input[inputIndex + 2];

        Output[outputIndex++] = PhBase64EncodeTable[(value >> 18) & 0x3F];
        Output[outputIndex++] = PhBase64EncodeTable[(value >> 12) & 0x3F];
        Output[outputIndex++] = PhBase64EncodeTable[(value >> 6) & 0x3F];
        Output[outputIndex++] = PhBase64EncodeTable[value & 0x3F];
        inputIndex += 3;
    }

    if (inputIndex < InputLength)
    {
        ULONG value = (ULONG)Input[inputIndex] << 16;

        Output[outputIndex++] = PhBase64EncodeTable[(value >> 18) & 0x3F];

        if (inputIndex + 1 < InputLength)
        {
            value |= (ULONG)Input[inputIndex + 1] << 8;
            Output[outputIndex++] = PhBase64EncodeTable[(value >> 12) & 0x3F];
            Output[outputIndex++] = PhBase64EncodeTable[(value >> 6) & 0x3F];
            Output[outputIndex++] = '=';
        }
        else
        {
            Output[outputIndex++] = PhBase64EncodeTable[(value >> 12) & 0x3F];
            Output[outputIndex++] = '=';
            Output[outputIndex++] = '=';
        }
    }

    Output[outputIndex] = ANSI_NULL;

    if (ResultLength)
        *ResultLength = outputIndex;

    return TRUE;
}

BOOLEAN PhBase64Decode(
    _In_reads_(InputLength) PCSTR Input,
    _In_ SIZE_T InputLength,
    _Out_writes_bytes_(OutputLength) PUCHAR Output,
    _In_ SIZE_T OutputLength,
    _Out_opt_ PSIZE_T ResultLength
    )
{
    SIZE_T outputIndex = 0;
    SIZE_T index = 0;
    CHAR quartet[4];
    SIZE_T quartetCount = 0;
    BOOLEAN seenPadding = FALSE;

    if (ResultLength)
        *ResultLength = 0;

    while (index < InputLength)
    {
        CHAR value = Input[index++];

        if (PhBase64IsWhitespace((UCHAR)value))
            continue;

        if (seenPadding)
            return FALSE;

        quartet[quartetCount++] = value;

        if (quartetCount == 4)
        {
            UCHAR decoded0;
            UCHAR decoded1;

            if (quartet[0] == '=' || quartet[1] == '=')
                return FALSE;

            decoded0 = PhBase64DecodeTable[(UCHAR)quartet[0]];
            decoded1 = PhBase64DecodeTable[(UCHAR)quartet[1]];

            if (decoded0 == 0xFF || decoded1 == 0xFF || decoded0 == 0xFE || decoded1 == 0xFE)
                return FALSE;

            if (quartet[2] == '=')
            {
                if (quartet[3] != '=')
                    return FALSE;

                if (OutputLength - outputIndex < 1)
                    return FALSE;

                Output[outputIndex++] = (UCHAR)((decoded0 << 2) | (decoded1 >> 4));
                seenPadding = TRUE;
            }
            else if (quartet[3] == '=')
            {
                UCHAR decoded2 = PhBase64DecodeTable[(UCHAR)quartet[2]];

                if (decoded2 == 0xFF || decoded2 == 0xFE)
                    return FALSE;

                if (OutputLength - outputIndex < 2)
                    return FALSE;

                Output[outputIndex++] = (UCHAR)((decoded0 << 2) | (decoded1 >> 4));
                Output[outputIndex++] = (UCHAR)((decoded1 << 4) | (decoded2 >> 2));
                seenPadding = TRUE;
            }
            else
            {
                UCHAR decoded2 = PhBase64DecodeTable[(UCHAR)quartet[2]];
                UCHAR decoded3 = PhBase64DecodeTable[(UCHAR)quartet[3]];

                if (decoded2 == 0xFF || decoded3 == 0xFF || decoded2 == 0xFE || decoded3 == 0xFE)
                    return FALSE;

                if (OutputLength - outputIndex < 3)
                    return FALSE;

                Output[outputIndex++] = (UCHAR)((decoded0 << 2) | (decoded1 >> 4));
                Output[outputIndex++] = (UCHAR)((decoded1 << 4) | (decoded2 >> 2));
                Output[outputIndex++] = (UCHAR)((decoded2 << 6) | decoded3);
            }
            quartetCount = 0;
        }
    }

    if (quartetCount != 0)
    {
        return FALSE;
    }

    if (ResultLength)
        *ResultLength = outputIndex;

    return TRUE;
}
