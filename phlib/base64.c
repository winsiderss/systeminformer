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

FORCEINLINE
BOOLEAN
PhBase64IsWhitespace(
    _In_ UCHAR Value
    )
{
    return Value == ' ' || Value == '\t' || Value == '\r' || Value == '\n';
}

/**
 * \brief SSSE3 base64 encode of one 12-input-byte block to 16 output chars
 * (Wojciech Muła's pshufb-based encoder). Produces the standard alphabet
 * (A-Za-z0-9+/), identical to PhBase64EncodeTable.
 */
static PH_INT128 PhpBase64EncodeBlock(
    _In_ PH_INT128 Input
    )
{
    PH_INT128 in;
    PH_INT128 t0;
    PH_INT128 t1;
    PH_INT128 t2;
    PH_INT128 t3;
    PH_INT128 indices;
    PH_INT128 mask;
    const PH_INT128 lut = PhSetReverseINT128by8(65, 71, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -19, -16, 0, 0);

    // Rearrange so each 32-bit lane holds 3 source bytes ready for 6-bit splitting.
    in = PhShuffleINT128by8(Input, PhSetReverseINT128by8(1, 0, 2, 1, 4, 3, 5, 4, 7, 6, 8, 7, 10, 9, 11, 10));

    // Split each lane into four 6-bit fields (0..63).
    t0 = PhAndINT128(in, PhSetINT128by32(0x0fc0fc00));
    t1 = PhMultiplyHighUINT128by16(t0, PhSetINT128by32(0x04000040));
    t2 = PhAndINT128(in, PhSetINT128by32(0x003f03f0));
    t3 = PhMultiplyLowINT128by16(t2, PhSetINT128by32(0x01000010));
    indices = PhOrINT128(t1, t3);

    // Translate 6-bit indices to ASCII via range-offset LUT.
    mask = PhCompareGtINT128by8(indices, PhSetINT128by8(25));
    t0 = PhSubSaturateUINT128by8(indices, PhSetINT128by8(51));
    t0 = PhSubINT128by8(t0, mask);
    return PhAddINT128by8(indices, PhShuffleINT128by8(lut, t0));
}

_Success_(return)
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

    // Vector path (SSSE3/NEON): encode 12 input bytes -> 16 output chars per iteration.
    // The 16-byte load overshoots the 12 consumed bytes by 4, so require 16 readable
    // bytes; the trailing groups (and any padding) fall through to the scalar loop.
    if (PhHasShuffleBytes)
    {
        while (inputIndex + 16 <= InputLength)
        {
            PH_INT128 block = PhLoadINT128U((PLONG)&Input[inputIndex]);
            PhStoreINT128U((PLONG)&Output[outputIndex], PhpBase64EncodeBlock(block));
            inputIndex += 12;
            outputIndex += 16;
        }
    }

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


#ifndef _ARM64_

// Translate alphabet bytes and pack each four sextets into three bytes.
// Whitespace, padding and invalid bytes are handled by the scalar decoder.
static BOOLEAN PhpBase64DecodeBlock128(
    _In_ __m128i Input,
    _Out_ __m128i* Output
    )
{
    __m128i upper = _mm_and_si128(
        _mm_cmpgt_epi8(Input, _mm_set1_epi8('A' - 1)),
        _mm_cmpgt_epi8(_mm_set1_epi8('Z' + 1), Input)
        );
    __m128i lower = _mm_and_si128(
        _mm_cmpgt_epi8(Input, _mm_set1_epi8('a' - 1)),
        _mm_cmpgt_epi8(_mm_set1_epi8('z' + 1), Input)
        );
    __m128i digit = _mm_and_si128(
        _mm_cmpgt_epi8(Input, _mm_set1_epi8('0' - 1)),
        _mm_cmpgt_epi8(_mm_set1_epi8('9' + 1), Input)
        );
    __m128i plus = _mm_cmpeq_epi8(Input, _mm_set1_epi8('+'));
    __m128i slash = _mm_cmpeq_epi8(Input, _mm_set1_epi8('/'));
    __m128i valid;
    __m128i value;

    valid = _mm_or_si128(_mm_or_si128(upper, lower),
        _mm_or_si128(digit, _mm_or_si128(plus, slash)));

    if ((USHORT)_mm_movemask_epi8(valid) != 0xffff)
        return FALSE;

    value = _mm_or_si128(
        _mm_and_si128(upper, _mm_sub_epi8(Input, _mm_set1_epi8('A'))),
        _mm_and_si128(lower, _mm_sub_epi8(Input, _mm_set1_epi8('a' - 26)))
        );
    value = _mm_or_si128(value,
        _mm_and_si128(digit, _mm_add_epi8(Input, _mm_set1_epi8(52 - '0'))));
    value = _mm_or_si128(value, _mm_and_si128(plus, _mm_set1_epi8(62)));
    value = _mm_or_si128(value, _mm_and_si128(slash, _mm_set1_epi8(63)));
    value = _mm_maddubs_epi16(value, _mm_set1_epi16(0x0140));
    value = _mm_madd_epi16(value, _mm_set1_epi32(0x00011000));
    *Output = _mm_shuffle_epi8(value, _mm_setr_epi8(2, 1, 0, 6, 5, 4, 10, 9, 8, 14, 13, 12, -1, -1, -1, -1));
    return TRUE;
}

// Translate alphabet bytes and pack each four sextets into three bytes.
// Whitespace, padding and invalid bytes are handled by the scalar decoder.
static BOOLEAN PhpBase64DecodeBlock256(
    _In_ __m256i Input,
    _Out_ __m256i* Output
    )
{
    __m256i upper = _mm256_and_si256(
        _mm256_cmpgt_epi8(Input, _mm256_set1_epi8('A' - 1)),
        _mm256_cmpgt_epi8(_mm256_set1_epi8('Z' + 1), Input)
        );
    __m256i lower = _mm256_and_si256(
        _mm256_cmpgt_epi8(Input, _mm256_set1_epi8('a' - 1)),
        _mm256_cmpgt_epi8(_mm256_set1_epi8('z' + 1), Input)
        );
    __m256i digit = _mm256_and_si256(
        _mm256_cmpgt_epi8(Input, _mm256_set1_epi8('0' - 1)),
        _mm256_cmpgt_epi8(_mm256_set1_epi8('9' + 1), Input)
        );
    __m256i plus = _mm256_cmpeq_epi8(Input, _mm256_set1_epi8('+'));
    __m256i slash = _mm256_cmpeq_epi8(Input, _mm256_set1_epi8('/'));
    __m256i valid;
    __m256i value;

    valid = _mm256_or_si256(_mm256_or_si256(upper, lower),
        _mm256_or_si256(digit, _mm256_or_si256(plus, slash)));

    if ((ULONG)_mm256_movemask_epi8(valid) != 0xffffffff)
        return FALSE;

    value = _mm256_or_si256(
        _mm256_and_si256(upper, _mm256_sub_epi8(Input, _mm256_set1_epi8('A'))),
        _mm256_and_si256(lower, _mm256_sub_epi8(Input, _mm256_set1_epi8('a' - 26)))
        );
    value = _mm256_or_si256(value,
        _mm256_and_si256(digit, _mm256_add_epi8(Input, _mm256_set1_epi8(52 - '0'))));
    value = _mm256_or_si256(value, _mm256_and_si256(plus, _mm256_set1_epi8(62)));
    value = _mm256_or_si256(value, _mm256_and_si256(slash, _mm256_set1_epi8(63)));
    value = _mm256_maddubs_epi16(value, _mm256_set1_epi16(0x0140));
    value = _mm256_madd_epi16(value, _mm256_set1_epi32(0x00011000));
    *Output = _mm256_shuffle_epi8(value, _mm256_broadcastsi128_si256(_mm_setr_epi8(2, 1, 0, 6, 5, 4, 10, 9, 8, 14, 13, 12, -1, -1, -1, -1)));
    return TRUE;
}
#endif

_Success_(return)
BOOLEAN PhBase64Decode(
    _In_reads_(InputLength) PCSTR Input,
    _In_ SIZE_T InputLength,
    _Out_writes_bytes_(OutputLength) PUCHAR Output,
    _In_ SIZE_T OutputLength,
    _Out_opt_ PSIZE_T ResultLength
    )
{
    BOOLEAN seenPadding = FALSE;
    SIZE_T outputIndex = 0;
    SIZE_T index = 0;
    SIZE_T quartetCount = 0;
    CHAR quartet[4];

    if (ResultLength)
        *ResultLength = 0;

    while (index < InputLength)
    {
        CHAR value;

#ifndef _ARM64_
        if (!seenPadding && !quartetCount)
        {
            if (PhHasAVX && InputLength - index >= 32 && OutputLength - outputIndex >= 24)
            {
                __m256i decoded;
                BOOLEAN valid = PhpBase64DecodeBlock256(
                    _mm256_loadu_si256((__m256i const*)&Input[index]),
                    &decoded
                    );

                if (valid)
                {
                    UCHAR bytes[32];

                    _mm256_storeu_si256((__m256i*)bytes, decoded);
                    // Each 128-bit lane contains 12 bytes. Store exactly 24,
                    // including when the caller supplies an exact-size buffer.
                    memcpy(&Output[outputIndex], bytes, 12);
                    memcpy(&Output[outputIndex + 12], bytes + 16, 12);
                    index += 32;
                    outputIndex += 24;
                }

                PhZeroUpper();

                if (valid)
                    continue;
            }

            if (PhHasSSSE3 && InputLength - index >= 16 && OutputLength - outputIndex >= 12)
            {
                __m128i decoded;

                if (PhpBase64DecodeBlock128(_mm_loadu_si128((__m128i const*)&Input[index]), &decoded))
                {
                    UCHAR bytes[16];

                    _mm_storeu_si128((__m128i*)bytes, decoded);
                    memcpy(&Output[outputIndex], bytes, 12);
                    index += 16;
                    outputIndex += 12;
                    continue;
                }
            }
        }
#endif

        value = Input[index++];

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
