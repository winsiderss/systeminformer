/*
 * Copyright (c) 2026 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 */

#include <phbase.h>
#include <phintrin.h>
#include "include\base64.h"

static const CHAR PhBase64EncodeTable[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#ifndef _ARM64_
/**
 * \brief SSSE3 base64 encode of one 12-input-byte block to 16 output chars
 * (Wojciech Muła's pshufb-based encoder). Produces the standard alphabet
 * (A-Za-z0-9+/), identical to PhBase64EncodeTable.
 */
static __m128i PhpBase64EncodeBlock(
    _In_ __m128i Input
    )
{
    __m128i in;
    __m128i t0;
    __m128i t1;
    __m128i t2;
    __m128i t3;
    __m128i indices;
    __m128i mask;
    const __m128i lut = _mm_setr_epi8(65, 71, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -19, -16, 0, 0);

    // Rearrange so each 32-bit lane holds 3 source bytes ready for 6-bit splitting.
    in = _mm_shuffle_epi8(Input, _mm_set_epi8(10, 11, 9, 10, 7, 8, 6, 7, 4, 5, 3, 4, 1, 2, 0, 1));

    // Split each lane into four 6-bit fields (0..63).
    t0 = _mm_and_si128(in, _mm_set1_epi32(0x0fc0fc00));
    t1 = _mm_mulhi_epu16(t0, _mm_set1_epi32(0x04000040));
    t2 = _mm_and_si128(in, _mm_set1_epi32(0x003f03f0));
    t3 = _mm_mullo_epi16(t2, _mm_set1_epi32(0x01000010));
    indices = _mm_or_si128(t1, t3);

    // Translate 6-bit indices to ASCII via range-offset LUT.
    mask = _mm_cmpgt_epi8(indices, _mm_set1_epi8(25));
    t0 = _mm_subs_epu8(indices, _mm_set1_epi8(51));
    t0 = _mm_sub_epi8(t0, mask);
    return _mm_add_epi8(indices, _mm_shuffle_epi8(lut, t0));
}
#endif

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

#ifndef _ARM64_
    // SSSE3: encode 12 input bytes -> 16 output chars per iteration. The 16-byte
    // load overshoots the 12 consumed bytes by 4, so require 16 readable bytes;
    // the trailing groups (and any padding) fall through to the scalar loop.
    if (PhHasSSSE3)
    {
        while (inputIndex + 16 <= InputLength)
        {
            __m128i block = _mm_loadu_si128((__m128i const*)&Input[inputIndex]);
            _mm_storeu_si128((__m128i*)&Output[outputIndex], PhpBase64EncodeBlock(block));
            inputIndex += 12;
            outputIndex += 16;
        }
    }
#endif

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
