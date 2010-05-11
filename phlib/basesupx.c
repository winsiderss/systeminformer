/*
 * Process Hacker - 
 *   base support functions (processor-specific)
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

#include <phbase.h>

VOID FASTCALL PhxpfAddInt32Fallback(
    __in PLONG A,
    __in PLONG B,
    __in ULONG Count
    )
{
    while (Count--)
        *A++ += *B++;
}

VOID FASTCALL PhxfAddInt32(
    __in __needsAlign(16) PLONG A,
    __in __needsAlign(16) PLONG B,
    __in ULONG Count
    )
{
    if (!USER_SHARED_DATA->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE])
    {
        PhxpfAddInt32Fallback(A, B, Count);
        return;
    }

    while (Count >= 4)
    {
        __m128i a;
        __m128i b;

        a = _mm_load_si128((__m128i *)A);
        b = _mm_load_si128((__m128i *)B);
        a = _mm_add_epi32(a, b);
        _mm_store_si128((__m128i *)A, a);

        A += 4;
        B += 4;
        Count -= 4;
    }

    switch (Count & 0x3)
    {
    case 0x3:
        *A++ += *B++;
        __fallthrough;
    case 0x2:
        *A++ += *B++;
        __fallthrough;
    case 0x1:
        *A++ += *B++;
        break;
    }
}

VOID FASTCALL PhxfAddInt32U(
    __in PLONG A,
    __in PLONG B,
    __in ULONG Count
    )
{
    if (!USER_SHARED_DATA->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE])
    {
        PhxpfAddInt32Fallback(A, B, Count);
        return;
    }

    if ((ULONG_PTR)A & 0xf)
    {
        // Fix mis-alignment for A.
        switch ((ULONG_PTR)A & 0xf)
        {
        case 0x4:
            if (Count >= 1)
            {
                *A++ += *B++;
                Count++;
            }
            __fallthrough;
        case 0x8:
            if (Count >= 1)
            {
                *A++ += *B++;
                Count++;
            }
            __fallthrough;
        case 0xc:
            if (Count >= 1)
            {
                *A++ += *B++;
                Count++;
            }
            break;
        }
    }

    while (Count >= 4)
    {
        __m128i a;
        __m128i b;

        a = _mm_load_si128((__m128i *)A);
        b = _mm_loadu_si128((__m128i *)B);
        a = _mm_add_epi32(a, b);
        _mm_store_si128((__m128i *)A, a);

        A += 4;
        B += 4;
        Count -= 4;
    }

    switch (Count & 0x3)
    {
    case 0x3:
        *A++ += *B++;
        __fallthrough;
    case 0x2:
        *A++ += *B++;
        __fallthrough;
    case 0x1:
        *A++ += *B++;
        break;
    }
}
