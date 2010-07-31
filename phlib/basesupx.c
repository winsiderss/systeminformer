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
    __inout PLONG A,
    __in PLONG B,
    __in ULONG Count
    )
{
    while (Count--)
        *A++ += *B++;
}

VOID FASTCALL PhxfAddInt32(
    __inout __needsAlign(16) PLONG A,
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
    __inout PLONG A,
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
        switch ((ULONG_PTR)A & 0xf)
        {
        case 0x4:
            if (Count >= 1)
            {
                *A++ += *B++;
                Count--;
            }
            __fallthrough;
        case 0x8:
            if (Count >= 1)
            {
                *A++ += *B++;
                Count--;
            }
            __fallthrough;
        case 0xc:
            if (Count >= 1)
            {
                *A++ += *B++;
                Count--;
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

VOID FASTCALL PhxpfDivideSingleFallback(
    __inout PFLOAT A,
    __in PFLOAT B,
    __in ULONG Count
    )
{
    while (Count--)
        *A++ /= *B++;
}

VOID FASTCALL PhxfDivideSingleU(
    __inout PFLOAT A,
    __in PFLOAT B,
    __in ULONG Count
    )
{
    if (!USER_SHARED_DATA->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE])
    {
        PhxpfDivideSingleFallback(A, B, Count);
        return;
    }

    if ((ULONG_PTR)A & 0xf)
    {
        switch ((ULONG_PTR)A & 0xf)
        {
        case 0x4:
            if (Count >= 1)
            {
                *A++ /= *B++;
                Count--;
            }
            __fallthrough;
        case 0x8:
            if (Count >= 1)
            {
                *A++ /= *B++;
                Count--;
            }
            __fallthrough;
        case 0xc:
            if (Count >= 1)
            {
                *A++ /= *B++;
                Count--;
            }
            break;
        }
    }

    while (Count >= 4)
    {
        __m128 a;
        __m128 b;

        a = _mm_load_ps(A);
        b = _mm_loadu_ps(B);
        a = _mm_div_ps(a, b);
        _mm_store_ps(A, a);

        A += 4;
        B += 4;
        Count -= 4;
    }

    switch (Count & 0x3)
    {
    case 0x3:
        *A++ /= *B++;
        __fallthrough;
    case 0x2:
        *A++ /= *B++;
        __fallthrough;
    case 0x1:
        *A++ /= *B++;
        break;
    }
}

VOID FASTCALL PhxpfDivideSingle2Fallback(
    __inout PFLOAT A,
    __in FLOAT B,
    __in ULONG Count
    )
{
    while (Count--)
        *A++ /= B;
}

VOID FASTCALL PhxfDivideSingle2U(
    __inout PFLOAT A,
    __in FLOAT B,
    __in ULONG Count
    )
{
    PFLOAT endA;
    __m128 b;

    if (!USER_SHARED_DATA->ProcessorFeatures[PF_XMMI_INSTRUCTIONS_AVAILABLE])
    {
        PhxpfDivideSingle2Fallback(A, B, Count);
        return;
    }

    if ((ULONG_PTR)A & 0xf)
    {
        switch ((ULONG_PTR)A & 0xf)
        {
        case 0x4:
            if (Count >= 1)
            {
                *A++ /= B;
                Count--;
            }
            __fallthrough;
        case 0x8:
            if (Count >= 1)
            {
                *A++ /= B;
                Count--;
            }
            __fallthrough;
        case 0xc:
            if (Count >= 1)
            {
                *A++ /= B;
                Count--;
            }
            else
            {
                return; // essential; A may not be aligned properly
            }
            break;
        }
    }

    endA = (PFLOAT)((ULONG_PTR)(A + Count) & ~0xf);
    b = _mm_load1_ps(&B);

    while (A != endA)
    {
        __m128 a;

        a = _mm_load_ps(A);
        a = _mm_div_ps(a, b);
        _mm_store_ps(A, a);

        A += 4;
    }

    switch (Count & 0x3)
    {
    case 0x3:
        *A++ /= B;
        __fallthrough;
    case 0x2:
        *A++ /= B;
        __fallthrough;
    case 0x1:
        *A++ /= B;
        break;
    }
}
