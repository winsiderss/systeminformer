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

#ifdef _M_IX86

__declspec(naked) int __cdecl ph_equal_string(wchar_t *s1, wchar_t *s2, size_t len)
{
    __asm
    {
        push    esi
        push    edi

        mov     ecx, [esp+0xc+0x8] // len
        mov     eax, [esp+0xc+0x8] // len

        and     ecx, -2 // round down to even number
        mov     esi, [esp+0xc+0x0] // s1
        mov     edi, [esp+0xc+0x4] // s2
        jz      zero_or_one

loop_start:
        mov     edx, [esi]
        cmp     edx, [edi]
        jnz     return0

        add     esi, 4
        add     edi, 4
        sub     ecx, 2
        jnz     loop_start

        // Either we've reached the end, or the length was odd.
        test    eax, 1
        jnz     odd_length

return1:
        mov     eax, 1
        pop     edi
        pop     esi
        ret

zero_or_one:
        test    eax, eax
        jz      return1

odd_length:
        mov     dx, [esi]
        cmp     dx, [edi]
        jz      return1

return0:
        xor     eax, eax
        pop     edi
        pop     esi
        ret
    }
}

__declspec(naked) unsigned long __cdecl ph_crc32(unsigned long crc, char *buf, size_t len)
{
    __asm
    {
        push    esi

        mov     eax, [esp+0x8+0x0] // crc
        mov     ecx, [esp+0x8+0x8] // len
        mov     esi, [esp+0x8+0x4] // buf

        xor     edx, edx
        jecxz   done
        not     eax

        // while (len--)
        //     crc = (crc >> 8) ^ table[(crc ^ *buf++) & 0xff];
loop_start:
        mov     dl, [esi]
        inc     esi
        xor     dl, al
        shr     eax, 8
        xor     eax, [PhCrc32Table+edx*4]

        sub     ecx, 1
        jnz     loop_start

        not     eax
done:
        pop     esi
        ret
    }
}

#else

int __cdecl ph_equal_string(wchar_t *s1, wchar_t *s2, size_t len)
{
    size_t l;

    l = len & -2; // round down to power of 2

    if (l)
    {
        while (TRUE)
        {
            if (*(PULONG)s1 != *(PULONG)s2)
                return 0;

            s1 += 2;
            s2 += 2;
            l -= 2;

            if (!l)
                break;
        }
    }

    if (len & 1)
        return *s1 == *s2;
    else
        return 1;
}

unsigned long __cdecl ph_crc32(unsigned long crc, char *buf, size_t len)
{
    crc ^= 0xffffffff;

    while (len--)
        crc = (crc >> 8) ^ PhCrc32Table[(crc ^ *buf++) & 0xff];

    return crc ^ 0xffffffff;
}

#endif

VOID FASTCALL PhxpfAddInt32Fallback(
    __inout PLONG A,
    __in PLONG B,
    __in ULONG Count
    )
{
    while (Count--)
        *A++ += *B++;
}

/**
 * Adds one array of integers to another.
 *
 * \param A The destination array to which the source 
 * array is added. The array must be 16 byte aligned.
 * \param B The source array. The array must be 16 
 * byte aligned.
 * \param Count The number of elements.
 */
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

/**
 * Adds one array of integers to another.
 *
 * \param A The destination array to which the source 
 * array is added.
 * \param B The source array.
 * \param Count The number of elements.
 */
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

/**
 * Divides one array of numbers by another.
 *
 * \param A The destination array, divided by 
 * the source array.
 * \param B The source array.
 * \param Count The number of elements.
 */
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

/**
 * Divides an array of numbers by a number.
 *
 * \param A The destination array, divided by 
 * \a B.
 * \param B The number.
 * \param Count The number of elements.
 */
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
