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

__declspec(naked) unsigned short __cdecl ph_chksum(unsigned long sum, unsigned short *buf, unsigned long count)
{
    __asm
    {
        push    esi

        mov     ecx, [esp+0x8+0x8] // count
        mov     eax, [esp+0x8+0x0] // sum
        mov     esi, [esp+0x8+0x4] // buf

        // The checksum involves summing the words in the buffer, adding the carry back
        // onto the low word of the checksum. We can do this more efficiently by
        // working with dwords.

        // Make sure the buffer has 4 byte alignment.
        test    esi, 0x2
        jz      do_2

        // Not aligned - process one word.
        add     ax, [esi]
        adc     ax, 0
        add     esi, 2
        sub     ecx, 1

        // Since the buffer is aligned, we start clearing bits of the
        // count starting with 2 words. Once we reach 16 words at a time,
        // the higher bits are cleared by a loop. The remaining 1 word is
        // then cleared.

do_2:
        // 2 words
        test    ecx, 2
        jz      do_4

        add     eax, [esi]
        adc     eax, 0

        add     esi, 4
        sub     ecx, 2

do_4:
        // 4 words
        test    ecx, 4
        jz      do_8

        add     eax, [esi]
        adc     eax, [esi+4]
        adc     eax, 0

        add     esi, 8
        sub     ecx, 4

do_8:
        // 8 words
        test    ecx, 8
        jz      do_16

        add     eax, [esi]
        adc     eax, [esi+4]
        adc     eax, [esi+8]
        adc     eax, [esi+12]
        adc     eax, 0

        add     esi, 16
        sub     ecx, 8

do_16:
        // 16 words
        mov     edx, ecx
        and     edx, 15
        sub     ecx, edx // ignore smaller chunks
        jz      do_1

do_16_loop_start:
        add     eax, [esi]
        adc     eax, [esi+4]
        adc     eax, [esi+8]
        adc     eax, [esi+12]
        adc     eax, [esi+16]
        adc     eax, [esi+20]
        adc     eax, [esi+24]
        adc     eax, [esi+28]
        adc     eax, 0

        add     esi, 32
        sub     ecx, 16
        jnz     do_16_loop_start

do_1:
        // 1 word
        test    edx, 1
        jz      done

        add     ax, [esi]
        adc     ax, 0

done:
        // Merge the top 16 bits with the bottom 16 bits.
        mov     dx, ax
        shr     eax, 16
        add     ax, dx
        adc     ax, 0

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

unsigned short __cdecl ph_chksum(unsigned long sum, unsigned short *buf, unsigned long count)
{
    while (count--)
    {
        sum += *buf++;
        sum = (sum >> 16) + (sum & 0xffff);
    }

    sum = (sum >> 16) + sum;

    return (unsigned short)sum;
}

#endif

VOID FASTCALL PhxpfFillMemoryUlongFallback(
    __inout PULONG Memory,
    __in ULONG Value,
    __in ULONG Count
    )
{
    if (Count != 0)
    {
        do
        {
            *Memory++ = Value;
        } while (--Count != 0);
    }
}

/**
 * Fills a memory block with a ULONG pattern.
 *
 * \param Memory The memory block. The block must be 
 * 4 byte aligned.
 * \param Value The ULONG pattern.
 * \param Count The number of elements.
 */
PHLIBAPI
VOID
FASTCALL
PhxfFillMemoryUlong(
    __inout PULONG Memory,
    __in ULONG Value,
    __in ULONG Count
    )
{
    __m128i pattern;
    ULONG count;

    if (!USER_SHARED_DATA->ProcessorFeatures[PF_XMMI64_INSTRUCTIONS_AVAILABLE])
    {
        PhxpfFillMemoryUlongFallback(Memory, Value, Count);
        return;
    }

    if ((ULONG_PTR)Memory & 0xf)
    {
        switch ((ULONG_PTR)Memory & 0xf)
        {
        case 0x4:
            if (Count >= 1)
            {
                *Memory++ = Value;
                Count--;
            }
            __fallthrough;
        case 0x8:
            if (Count >= 1)
            {
                *Memory++ = Value;
                Count--;
            }
            __fallthrough;
        case 0xc:
            if (Count >= 1)
            {
                *Memory++ = Value;
                Count--;
            }
            break;
        }
    }

    pattern = _mm_set1_epi32(Value);
    count = Count / 4;

    if (count != 0)
    {
        do
        {
            _mm_store_si128((__m128i *)Memory, pattern);
            Memory += 4;
        } while (--count != 0);
    }

    switch (Count & 0x3)
    {
    case 0x3:
        *Memory++ = Value;
        __fallthrough;
    case 0x2:
        *Memory++ = Value;
        __fallthrough;
    case 0x1:
        *Memory++ = Value;
        break;
    }
}

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
