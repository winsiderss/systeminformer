/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2016
 *     dmex    2019-2023
 *
 */

#include <phbase.h>
#include <phintrnl.h>
#include <phintrin.h>
#include <phnative.h>
#include <circbuf.h>
#include <thirdparty.h>
#include <ntintsafe.h>

#include <trace.h>

#ifndef PH_NATIVE_STRING_CONVERSION
#define PH_NATIVE_STRING_CONVERSION 1
#endif

#ifndef PH_NATIVE_STRING_IN_STRING
#define PH_NATIVE_STRING_IN_STRING 1
#endif

/**
 * Determines the length of the specified string, in characters.
 *
 * \param String The string.
 */
SIZE_T PhCountStringZ(
    _In_ PCWSTR String
    )
{
#ifndef _ARM64_
    if (PhHasAVX)
    {
        PWSTR p;
        ULONG unaligned;
        __m256i b;
        __m256i z;
        ULONG mask;
        ULONG index;

        p = (PWSTR)((ULONG_PTR)String & ~0x1f);
        unaligned = (ULONG_PTR)String & 0x1f;
        z = _mm256_setzero_si256();

        if (unaligned != 0)
        {
            b = _mm256_loadu_si256((__m256i const*)p);
            b = _mm256_cmpeq_epi16(b, z);
            mask = _mm256_movemask_epi8(b) >> unaligned;

            if (_BitScanForward(&index, mask))
            {
                _mm256_zeroupper();
                return index / sizeof(WCHAR);
            }

            p += 32 / sizeof(WCHAR);
        }

        while (TRUE)
        {
            b = _mm256_load_si256((__m256i const*)p);
            b = _mm256_cmpeq_epi16(b, z);
            mask = _mm256_movemask_epi8(b);

            if (_BitScanForward(&index, mask))
            {
                _mm256_zeroupper();
                return (SIZE_T)(p - String) + index / sizeof(WCHAR);
            }

            p += 32 / sizeof(WCHAR);
        }
    }
    else
#endif
    if (PhHasIntrinsics)
    {
        PWSTR p;
        ULONG unaligned;
        PH_INT128 b;
        PH_INT128 z;
        ULONG mask;
        ULONG index;

        p = (PWSTR)((ULONG_PTR)String & ~0xf);
        unaligned = (ULONG_PTR)String & 0xf;
        z = PhSetZeroINT128();

        if (unaligned != 0)
        {
            b = PhLoadINT128U((PLONG)p);
            b = PhCompareEqINT128by16(b, z);
            mask = PhMoveMaskINT128by8(b) >> unaligned;

            if (_BitScanForward(&index, mask))
            {
                return index / sizeof(WCHAR);
            }

            p += 16 / sizeof(WCHAR);
        }

        while (TRUE)
        {
            b = PhLoadINT128((PLONG)p);
            b = PhCompareEqINT128by16(b, z);
            mask = PhMoveMaskINT128by8(b);

            if (_BitScanForward(&index, mask))
            {
                return (SIZE_T)(p - String) + index / sizeof(WCHAR);
            }

            p += 16 / sizeof(WCHAR);
        }
    }
    else
    {
        return wcslen(String);
    }
}

/**
 * Allocates space for and copies a byte string.
 *
 * \param String The string to duplicate.
 *
 * \return The new string, which can be freed using PhFree().
 */
PSTR PhDuplicateBytesZ(
    _In_ PCSTR String
    )
{
    PSTR newString;
    SIZE_T length;

    length = strlen(String) + sizeof(ANSI_NULL); // include the null terminator

    newString = PhAllocate(length);
    memcpy(newString, String, length);

    return newString;
}

/**
 * Allocates space for and copies a byte string.
 *
 * \param String The string to duplicate.
 *
 * \return The new string, which can be freed using PhFree(), or NULL if storage could not be
 * allocated.
 */
PSTR PhDuplicateBytesZSafe(
    _In_ PCSTR String
    )
{
    PSTR newString;
    SIZE_T length;

    length = strlen(String) + sizeof(ANSI_NULL); // include the null terminator

    newString = PhAllocateSafe(length);

    if (!newString)
        return NULL;

    memcpy(newString, String, length);

    return newString;
}

/**
 * Allocates space for and copies a 16-bit string.
 *
 * \param String The string to duplicate.
 *
 * \return The new string, which can be freed using PhFree().
 */
PWSTR PhDuplicateStringZ(
    _In_ PCWSTR String
    )
{
    PWSTR newString;
    SIZE_T length;

    length = PhCountStringZ(String) * sizeof(WCHAR) + sizeof(UNICODE_NULL); // include the null terminator

    newString = PhAllocate(length);
    memcpy(newString, String, length);

    return newString;
}

/**
 * Copies a string with optional null termination and a maximum number of characters.
 *
 * \param InputBuffer A pointer to the input string.
 * \param InputCount The maximum number of characters to copy, not including the null terminator.
 * Specify -1 for no limit.
 * \param OutputBuffer A pointer to the output buffer.
 * \param OutputCount The number of characters in the output buffer, including the null terminator.
 * \param ReturnCount A variable which receives the number of characters required to contain the
 * input string, including the null terminator. If the function returns TRUE, this variable contains
 * the number of characters written to the output buffer.
 * \return TRUE if the input string was copied to the output string, otherwise FALSE.
 * \remarks The function stops copying when it encounters the first null character in the input
 * string. It will also stop copying when it reaches the character count specified in \a InputCount,
 * if \a InputCount is not -1.
 */
NTSTATUS PhCopyBytesZ(
    _In_ PCSTR InputBuffer,
    _In_ SIZE_T InputCount,
    _Out_writes_opt_z_(OutputCount) PSTR OutputBuffer,
    _In_ SIZE_T OutputCount,
    _Out_opt_ PSIZE_T ReturnCount
    )
{
    NTSTATUS status;
    SIZE_T i;

    // Determine the length of the input string.

    if (InputCount != SIZE_MAX)
    {
        i = 0;

        while (i < InputCount && InputBuffer[i])
            i++;
    }
    else
    {
        i = strlen(InputBuffer);
    }

    // Copy the string if there is enough room.

    if (OutputBuffer && OutputCount >= i + sizeof(ANSI_NULL)) // need one character for null terminator
    {
        memcpy(OutputBuffer, InputBuffer, i);
        OutputBuffer[i] = ANSI_NULL;
        status = STATUS_SUCCESS;
    }
    else
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (ReturnCount)
    {
        *ReturnCount = i + sizeof(ANSI_NULL);
    }

    return status;
}

/**
 * Copies a string with optional null termination and a maximum number of characters.
 *
 * \param InputBuffer A pointer to the input string.
 * \param InputCount The maximum number of characters to copy, not including the null terminator.
 * Specify -1 for no limit.
 * \param OutputBuffer A pointer to the output buffer.
 * \param OutputCount The number of characters in the output buffer, including the null terminator.
 * \param ReturnCount A variable which receives the number of characters required to contain the
 * input string, including the null terminator. If the function returns TRUE, this variable contains
 * the number of characters written to the output buffer.
 * \return TRUE if the input string was copied to the output string, otherwise FALSE.
 * \remarks The function stops copying when it encounters the first null character in the input
 * string. It will also stop copying when it reaches the character count specified in \a InputCount,
 * if \a InputCount is not -1.
 */
NTSTATUS PhCopyStringZ(
    _In_ PCWSTR InputBuffer,
    _In_ SIZE_T InputCount,
    _Out_writes_opt_z_(OutputCount) PWSTR OutputBuffer,
    _In_ SIZE_T OutputCount,
    _Out_opt_ PSIZE_T ReturnCount
    )
{
    NTSTATUS status;
    SIZE_T i;

    // Determine the length of the input string.

    if (InputCount != SIZE_MAX)
    {
        i = 0;

        while (i < InputCount && InputBuffer[i])
            i++;
    }
    else
    {
        i = PhCountStringZ(InputBuffer);
    }

    // Copy the string if there is enough room.

    if (OutputBuffer && OutputCount >= i + 1) // need one character for null terminator
    {
        memcpy(OutputBuffer, InputBuffer, i * sizeof(WCHAR));
        OutputBuffer[i] = UNICODE_NULL;
        status = STATUS_SUCCESS;
    }
    else
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (ReturnCount)
    {
        *ReturnCount = i + 1;
    }

    return status;
}

/**
 * Copies a string with optional null termination and a maximum number of characters.
 *
 * \param InputBuffer A pointer to the input string.
 * \param InputCount The maximum number of characters to copy, not including the null terminator.
 * Specify -1 for no limit.
 * \param OutputBuffer A pointer to the output buffer.
 * \param OutputCount The number of characters in the output buffer, including the null terminator.
 * \param ReturnCount A variable which receives the number of characters required to contain the
 * input string, including the null terminator. If the function returns TRUE, this variable contains
 * the number of characters written to the output buffer.
 * \return TRUE if the input string was copied to the output string, otherwise FALSE.
 * \remarks The function stops copying when it encounters the first null character in the input
 * string. It will also stop copying when it reaches the character count specified in \a InputCount,
 * if \a InputCount is not -1.
 */
NTSTATUS PhCopyStringZFromBytes(
    _In_ PCSTR InputBuffer,
    _In_ SIZE_T InputCount,
    _Out_writes_opt_z_(OutputCount) PWSTR OutputBuffer,
    _In_ SIZE_T OutputCount,
    _Out_opt_ PSIZE_T ReturnCount
    )
{
    NTSTATUS status;
    SIZE_T i;

    // Determine the length of the input string.

    if (InputCount != SIZE_MAX)
    {
        i = 0;

        while (i < InputCount && InputBuffer[i])
            i++;
    }
    else
    {
        i = strlen(InputBuffer);
    }

    // Copy the string if there is enough room.

    if (OutputBuffer && OutputCount >= i + 1) // need one character for null terminator
    {
        PhZeroExtendToUtf16Buffer(InputBuffer, i, OutputBuffer);
        OutputBuffer[i] = UNICODE_NULL;
        status = STATUS_SUCCESS;
    }
    else
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (ReturnCount)
    {
        *ReturnCount = i + 1;
    }

    return status;
}

/**
 * Copies a string with optional null termination and a maximum number of characters.
 *
 * \param InputBuffer A pointer to the input string.
 * \param InputCount The maximum number of characters to copy, not including the null terminator.
 * Specify -1 for no limit.
 * \param OutputBuffer A pointer to the output buffer.
 * \param OutputCount The number of characters in the output buffer, including the null terminator.
 * \param ReturnCount A variable which receives the number of characters required to contain the
 * input string, including the null terminator. If the function returns TRUE, this variable contains
 * the number of characters written to the output buffer.
 * \return TRUE if the input string was copied to the output string, otherwise FALSE.
 * \remarks The function stops copying when it encounters the first null character in the input
 * string. It will also stop copying when it reaches the character count specified in \a InputCount,
 * if \a InputCount is not -1.
 */
NTSTATUS PhCopyStringZFromMultiByte(
    _In_ PCSTR InputBuffer,
    _In_ SIZE_T InputCount,
    _Out_writes_opt_z_(OutputCount) PWSTR OutputBuffer,
    _In_ SIZE_T OutputCount,
    _Out_opt_ PSIZE_T ReturnCount
    )
{
    NTSTATUS status;
    SIZE_T i;
    ULONG inputBytes;
    ULONG unicodeBytes;

    // Determine the length of the input string.

    if (InputCount != SIZE_MAX)
    {
        i = 0;

        while (i < InputCount && InputBuffer[i])
            i++;
    }
    else
    {
        i = strlen(InputBuffer);
    }

    status = RtlSizeTToULong(i, &inputBytes);

    if (!NT_SUCCESS(status))
    {
        if (ReturnCount)
            *ReturnCount = SIZE_MAX;

        return status;
    }

    // Determine the length of the output string.

    status = RtlMultiByteToUnicodeSize(
        &unicodeBytes,
        InputBuffer,
        inputBytes
        );

    if (!NT_SUCCESS(status))
    {
        if (ReturnCount)
            *ReturnCount = SIZE_MAX;

        return status;
    }

    // Convert the string to Unicode if there is enough room.

    if (OutputBuffer && OutputCount >= (unicodeBytes + sizeof(UNICODE_NULL)) / sizeof(WCHAR))
    {
        status = RtlMultiByteToUnicodeN(
            OutputBuffer,
            unicodeBytes,
            NULL,
            InputBuffer,
            inputBytes
            );

        if (NT_SUCCESS(status))
        {
            // RtlMultiByteToUnicodeN doesn't null terminate the string.
            *(PWCHAR)PTR_ADD_OFFSET(OutputBuffer, unicodeBytes) = UNICODE_NULL;
        }
    }
    else
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (ReturnCount)
    {
        *ReturnCount = (unicodeBytes + sizeof(UNICODE_NULL)) / sizeof(WCHAR);
    }

    return status;
}

/**
 * Copies a UTF-8 encoded string to a wide-character (UTF-16) string buffer.
 *
 * \param InputBuffer Pointer to the input UTF-8 string buffer.
 * \param InputCount Size, in bytes, of the input buffer.
 * \param OutputBuffer Pointer to the output wide-character string buffer. Can be NULL if OutputCount is 0.
 * \param OutputCount Size, in characters, of the output buffer.
 * \param ReturnCount Optional pointer to receive the number of characters written to the output buffer (excluding the null terminator).
 * \return NTSTATUS Successful or errant status.
 * \remarks The output buffer will be null-terminated if OutputCount is greater than zero.
 */
NTSTATUS PhCopyStringZFromUtf8(
    _In_ PCSTR InputBuffer,
    _In_ SIZE_T InputCount,
    _Out_writes_opt_z_(OutputCount) PWSTR OutputBuffer,
    _In_ SIZE_T OutputCount,
    _Out_opt_ PSIZE_T ReturnCount
    )
{
    NTSTATUS status;
    SIZE_T i;
    ULONG inputBytes;
    ULONG unicodeBytes;

    // Determine the length of the input string.

    if (InputCount != SIZE_MAX)
    {
        i = 0;

        while (i < InputCount && InputBuffer[i])
            i++;
    }
    else
    {
        i = strlen(InputBuffer);
    }

    status = RtlSizeTToULong(i, &inputBytes);

    if (!NT_SUCCESS(status))
    {
        if (ReturnCount)
            *ReturnCount = SIZE_MAX;

        return status;
    }

    // Determine the length of the output string.

    status = RtlUTF8ToUnicodeN(
        NULL,
        0,
        &unicodeBytes,
        InputBuffer,
        inputBytes
        );

    if (!NT_SUCCESS(status))
    {
        if (ReturnCount)
            *ReturnCount = SIZE_MAX;

        return status;
    }

    // Convert the string to Unicode if there is enough room.

    if (OutputBuffer && OutputCount >= (unicodeBytes + sizeof(UNICODE_NULL)) / sizeof(WCHAR))
    {
        status = RtlUTF8ToUnicodeN(
            OutputBuffer,
            unicodeBytes,
            NULL,
            InputBuffer,
            inputBytes
            );

        if (NT_SUCCESS(status))
        {
            // RtlUTF8ToUnicodeN doesn't null terminate the string.
            *(PWCHAR)PTR_ADD_OFFSET(OutputBuffer, unicodeBytes) = UNICODE_NULL;
        }
    }
    else
    {
        status = STATUS_BUFFER_TOO_SMALL;
    }

    if (ReturnCount)
    {
        *ReturnCount = (unicodeBytes + sizeof(UNICODE_NULL)) / sizeof(WCHAR);
    }

    return status;
}

FORCEINLINE LONG PhpCompareRightNatural(
    _In_ PCWSTR A,
    _In_ PCWSTR B
    )
{
    LONG bias = 0;

    for (; ; A++, B++)
    {
        if (!PhIsDigitCharacter(*A) && !PhIsDigitCharacter(*B))
        {
            return bias;
        }
        else if (!PhIsDigitCharacter(*A))
        {
            return -1;
        }
        else if (!PhIsDigitCharacter(*B))
        {
            return 1;
        }
        else if (*A < *B)
        {
            if (bias == 0)
                bias = -1;
        }
        else if (*A > *B)
        {
            if (bias == 0)
                bias = 1;
        }
        else if (!*A && !*B)
        {
            return bias;
        }
    }

    return 0;
}

FORCEINLINE LONG PhpCompareLeftNatural(
    _In_ PCWSTR A,
    _In_ PCWSTR B
    )
{
    for (; ; A++, B++)
    {
        if (!PhIsDigitCharacter(*A) && !PhIsDigitCharacter(*B))
        {
            return 0;
        }
        else if (!PhIsDigitCharacter(*A))
        {
            return -1;
        }
        else if (!PhIsDigitCharacter(*B))
        {
            return 1;
        }
        else if (*A < *B)
        {
            return -1;
        }
        else if (*A > *B)
        {
            return 1;
        }
    }

    return 0;
}

FORCEINLINE LONG PhpCompareStringZNatural(
    _In_ PCWSTR A,
    _In_ PCWSTR B,
    _In_ BOOLEAN IgnoreCase
    )
{
    /*  strnatcmp.c -- Perform 'natural order' comparisons of strings in C.
        Copyright (C) 2000, 2004 by Martin Pool <mbp sourcefrog net>

        This software is provided 'as-is', without any express or implied
        warranty.  In no event will the authors be held liable for any damages
        arising from the use of this software.

        Permission is granted to anyone to use this software for any purpose,
        including commercial applications, and to alter it and redistribute it
        freely, subject to the following restrictions:

        1. The origin of this software must not be misrepresented; you must not
         claim that you wrote the original software. If you use this software
         in a product, an acknowledgment in the product documentation would be
         appreciated but is not required.
        2. Altered source versions must be plainly marked as such, and must not be
         misrepresented as being the original software.
        3. This notice may not be removed or altered from any source distribution.

        This code has been modified for System Informer.
     */

    ULONG ai, bi;
    WCHAR ca, cb;
    LONG result;
    BOOLEAN fractional;

    ai = 0;
    bi = 0;

    while (TRUE)
    {
        ca = A[ai];
        cb = B[bi];

        /* Skip over leading spaces. */

        while (ca == ' ')
            ca = A[++ai];

        while (cb == ' ')
            cb = B[++bi];

        /* Process run of digits. */
        if (PhIsDigitCharacter(ca) && PhIsDigitCharacter(cb))
        {
            fractional = (ca == '0' || cb == '0');

            if (fractional)
            {
                if ((result = PhpCompareLeftNatural(A + ai, B + bi)) != 0)
                    return result;
            }
            else
            {
                if ((result = PhpCompareRightNatural(A + ai, B + bi)) != 0)
                    return result;
            }

            while (PhIsDigitCharacter(A[ai]))
                ai++;
            while (PhIsDigitCharacter(B[bi]))
                bi++;

            continue;
        }

        if (!ca && !cb)
        {
            /* Strings are considered the same. */
            return 0;
        }

        if (IgnoreCase)
        {
            ca = PhUpcaseUnicodeChar(ca);
            cb = PhUpcaseUnicodeChar(cb);
        }

        if (ca < cb)
            return -1;
        else if (ca > cb)
            return 1;

        ai++;
        bi++;
    }
}

/**
 * Compares two strings in natural sort order.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param IgnoreCase TRUE to perform a case-insensitive comparison, otherwise FALSE.
 * \return A value less than zero if \a String1 is less than \a String2, a value greater than zero if
 * \a String1 is greater than \a String2, or zero if the strings are equal.
 */
LONG PhCompareStringZNatural(
    _In_ PCWSTR String1,
    _In_ PCWSTR String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    return PhpCompareStringZNatural(String1, String2, IgnoreCase);
}

/**
 * Compares two strings.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param IgnoreCase TRUE to perform a case-insensitive comparison, otherwise FALSE.
 * \return A value less than zero if \a String1 is less than \a String2, a value greater than zero if
 * \a String1 is greater than \a String2, or zero if the strings are equal.
 */
LONG PhCompareStringRef(
    _In_ PCPH_STRINGREF String1,
    _In_ PCPH_STRINGREF String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    SIZE_T l1;
    SIZE_T l2;
    PWCHAR s1;
    PWCHAR s2;
    WCHAR c1;
    WCHAR c2;
    PWCHAR end;

    // Note: this function assumes that the difference between the lengths of the two strings can
    // fit inside a LONG.

    l1 = String1->Length;
    l2 = String2->Length;
    assert(!(l1 & 1));
    assert(!(l2 & 1));
    s1 = String1->Buffer;
    s2 = String2->Buffer;

    if (PhHasIntrinsics)
    {
        SIZE_T commonLength = l1 <= l2 ? l1 : l2;

#ifndef _ARM64_
        if (PhHasAVX)
        {
            SIZE_T length = commonLength / 32;

            if (length != 0)
            {
                if (IgnoreCase)
                {
                    do
                    {
                        __m256i b1 = _mm256_loadu_si256((__m256i const*)s1);
                        __m256i b2 = _mm256_loadu_si256((__m256i const*)s2);

                        // SIMD uppercase conversion
                        __m256i ub1 = PhUppercaseLatin1INT256by16(b1);
                        __m256i ub2 = PhUppercaseLatin1INT256by16(b2);

                        // Compare uppercased values
                        __m256i cmp = _mm256_cmpeq_epi16(ub1, ub2);
                        if ((ULONG)_mm256_movemask_epi8(cmp) != 0xffffffff)
                        {
                            _mm256_zeroupper();
                            goto CompareCharacters;
                        }

                        s1 += 32 / sizeof(WCHAR);
                        s2 += 32 / sizeof(WCHAR);
                    } while (--length != 0);

                    _mm256_zeroupper();
                }
                else
                {
                    do
                    {
                        __m256i b1 = _mm256_loadu_si256((__m256i const*)s1);
                        __m256i b2 = _mm256_loadu_si256((__m256i const*)s2);
                        __m256i cmp = _mm256_cmpeq_epi16(b1, b2);

                        if ((ULONG)_mm256_movemask_epi8(cmp) != 0xffffffff)
                        {
                            _mm256_zeroupper();
                            goto CompareCharacters;
                        }

                        s1 += 32 / sizeof(WCHAR);
                        s2 += 32 / sizeof(WCHAR);
                    } while (--length != 0);

                    _mm256_zeroupper();
                }
            }
        }
        else
#endif
        if (IgnoreCase)
        {
            SIZE_T length = commonLength / 16;

            if (length != 0)
            {
                do
                {
                    PH_INT128 b1 = PhLoadINT128U((PLONG)s1);
                    PH_INT128 b2 = PhLoadINT128U((PLONG)s2);

                    // SIMD uppercase conversion
                    PH_INT128 ub1 = PhUppercaseLatin1INT128by16(b1);
                    PH_INT128 ub2 = PhUppercaseLatin1INT128by16(b2);

                    // Compare uppercased values
                    PH_INT128 cmp = PhCompareEqINT128by16(ub1, ub2);
                    if (PhMoveMaskINT128by8(cmp) != 0xffff)
                    {
                        goto CompareCharacters;
                    }

                    s1 += 16 / sizeof(WCHAR);
                    s2 += 16 / sizeof(WCHAR);
                } while (--length != 0);
            }
        }
        else
        {
            SIZE_T length = commonLength / 16;

            if (length != 0)
            {
                do
                {
                    PH_INT128 b1 = PhLoadINT128U((PLONG)s1);
                    PH_INT128 b2 = PhLoadINT128U((PLONG)s2);
                    PH_INT128 cmp = PhCompareEqINT128by16(b1, b2);

                    if (PhMoveMaskINT128by8(cmp) != 0xffff)
                    {
                        goto CompareCharacters;
                    }

                    s1 += 16 / sizeof(WCHAR);
                    s2 += 16 / sizeof(WCHAR);
                } while (--length != 0);
            }
        }
    }

CompareCharacters:
    end = PTR_ADD_OFFSET(String1->Buffer, l1 <= l2 ? l1 : l2);

    if (IgnoreCase)
    {
        while (s1 != end)
        {
            c1 = *s1;
            c2 = *s2;

            if (c1 != c2)
            {
                c1 = PhUpcaseUnicodeChar(c1);
                c2 = PhUpcaseUnicodeChar(c2);

                if (c1 != c2)
                    return (LONG)c1 - (LONG)c2;
            }

            s1++;
            s2++;
        }
    }
    else
    {
        while (s1 != end)
        {
            c1 = *s1;
            c2 = *s2;

            if (c1 != c2)
                return (LONG)c1 - (LONG)c2;

            s1++;
            s2++;
        }
    }

    return (LONG)(l1 - l2);
}

/**
 * Determines if two strings are equal.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param IgnoreCase TRUE to perform a case-insensitive comparison, otherwise FALSE.
 * \return TRUE if the strings are equal, otherwise FALSE
 */
BOOLEAN PhEqualStringRef(
    _In_ PCPH_STRINGREF String1,
    _In_ PCPH_STRINGREF String2,
    _In_ BOOLEAN IgnoreCase
    )
{
    SIZE_T l1;
    SIZE_T l2;
    PWSTR s1;
    PWSTR s2;
    WCHAR c1;
    WCHAR c2;
    SIZE_T length;

    l1 = String1->Length;
    l2 = String2->Length;
    assert(!(l1 & 1));
    assert(!(l2 & 1));

    if (l1 != l2)
        return FALSE;

    s1 = String1->Buffer;
    s2 = String2->Buffer;

    if (PhHasIntrinsics)
    {
#ifndef _ARM64_
        if (PhHasAVX)
        {
            if (IgnoreCase)
            {
                length = l1 / 32;

                if (length != 0)
                {
                    do
                    {
                        __m256i b1 = _mm256_loadu_si256((__m256i const*)s1);
                        __m256i b2 = _mm256_loadu_si256((__m256i const*)s2);

                        // SIMD uppercase conversion
                        b1 = PhUppercaseLatin1INT256by16(b1);
                        b2 = PhUppercaseLatin1INT256by16(b2);

                        // Compare uppercased values
                        __m256i cmp = _mm256_cmpeq_epi16(b1, b2);
                        if ((ULONG)_mm256_movemask_epi8(cmp) != 0xffffffff)
                        {
                            _mm256_zeroupper();
                            l1 = (String1->Length / sizeof(WCHAR)) - (s1 - String1->Buffer);
                            goto CompareCharacters;
                        }

                        s1 += 32 / sizeof(WCHAR);
                        s2 += 32 / sizeof(WCHAR);
                    } while (--length != 0);

                    _mm256_zeroupper();
                }

                l1 = (l1 & 31) / sizeof(WCHAR);
            }
            else
            {
                length = l1 / 32;

                if (length != 0)
                {
                    do
                    {
                        __m256i b1 = _mm256_loadu_si256((__m256i const*)s1);
                        __m256i b2 = _mm256_loadu_si256((__m256i const*)s2);
                        __m256i cmp = _mm256_cmpeq_epi16(b1, b2);

                        if ((ULONG)_mm256_movemask_epi8(cmp) != 0xffffffff)
                        {
                            _mm256_zeroupper();
                            return FALSE;
                        }

                        s1 += 32 / sizeof(WCHAR);
                        s2 += 32 / sizeof(WCHAR);
                    } while (--length != 0);

                    _mm256_zeroupper();
                }

                l1 = (l1 & 31) / sizeof(WCHAR);
            }
        }
        else
#endif
        if (IgnoreCase)
        {
            length = l1 / 16;

            if (length != 0)
            {
                do
                {
                    PH_INT128 b1 = PhLoadINT128U((PLONG)s1);
                    PH_INT128 b2 = PhLoadINT128U((PLONG)s2);

                    // SIMD uppercase conversion
                    b1 = PhUppercaseLatin1INT128by16(b1);
                    b2 = PhUppercaseLatin1INT128by16(b2);

                    // Compare uppercased values
                    PH_INT128 cmp = PhCompareEqINT128by16(b1, b2);
                    if (PhMoveMaskINT128by8(cmp) != 0xffff)
                    {
                        // Mismatch - fall back to scalar
                        l1 = (String1->Length / sizeof(WCHAR)) - (s1 - String1->Buffer);
                        goto CompareCharacters;
                    }

                    s1 += 16 / sizeof(WCHAR);
                    s2 += 16 / sizeof(WCHAR);
                } while (--length != 0);
            }

            // Compare character-by-character because we have no more 16-byte blocks to compare.
            l1 = (l1 & 15) / sizeof(WCHAR);
        }
        else
        {
            length = l1 / 16;

            if (length != 0)
            {
                PH_INT128 b1;
                PH_INT128 b2;

                do
                {
                    b1 = PhLoadINT128U((PLONG)s1);
                    b2 = PhLoadINT128U((PLONG)s2);
                    b1 = PhCompareEqINT128by32(b1, b2);

                    if (PhMoveMaskINT128by8(b1) != 0xffff)
                    {
                        l1 = (String1->Length / sizeof(WCHAR)) - (s1 - String1->Buffer);
                        goto CompareCharacters;
                    }

                    s1 += 16 / sizeof(WCHAR);
                    s2 += 16 / sizeof(WCHAR);
                } while (--length != 0);
            }

            // Compare character-by-character because we have no more 16-byte blocks to compare.
            l1 = (l1 & 15) / sizeof(WCHAR);
        }
    }
    else
    {
        length = l1 / sizeof(ULONG_PTR);

        if (length != 0)
        {
            do
            {
                if (*(PULONG_PTR)s1 != *(PULONG_PTR)s2)
                {
                    l1 = (String1->Length / sizeof(WCHAR)) - (s1 - String1->Buffer);
                    goto CompareCharacters;
                }

                s1 += sizeof(ULONG_PTR) / sizeof(WCHAR);
                s2 += sizeof(ULONG_PTR) / sizeof(WCHAR);
            } while (--length != 0);
        }

        // Compare character-by-character because we have no more ULONG_PTR blocks to compare.
        l1 = (l1 & (sizeof(ULONG_PTR) - 1)) / sizeof(WCHAR);
    }

CompareCharacters:
    if (l1 != 0)
    {
        if (IgnoreCase)
        {
            do
            {
                c1 = *s1;
                c2 = *s2;

                if (c1 != c2)
                {
                    c1 = PhUpcaseUnicodeChar(c1);
                    c2 = PhUpcaseUnicodeChar(c2);

                    if (c1 != c2)
                        return FALSE;
                }

                s1++;
                s2++;
            } while (--l1 != 0);
        }
        else
        {
            do
            {
                c1 = *s1;
                c2 = *s2;

                if (c1 != c2)
                    return FALSE;

                s1++;
                s2++;
            } while (--l1 != 0);
        }
    }

    return TRUE;
}

/**
 * Locates a character in a string.
 *
 * \param String The string to search.
 * \param Character The character to search for.
 * \param IgnoreCase TRUE to perform a case-insensitive search, otherwise FALSE.
 * \return The index, in characters, of the first occurrence of \a Character in \a String1. If
 * \a Character was not found, -1 is returned.
 */
ULONG_PTR PhFindCharInStringRef(
    _In_ PCPH_STRINGREF String,
    _In_ WCHAR Character,
    _In_ BOOLEAN IgnoreCase
    )
{
    PWSTR buffer;
    SIZE_T length;

    buffer = String->Buffer;
    length = String->Length / sizeof(WCHAR);

    if (IgnoreCase)
    {
        if (Character <= 0xFF && Character != 0x00DF)
        {
#ifndef _ARM64_
            if (PhHasAVX)
            {
                SIZE_T length32;

                length32 = String->Length / 32;
                length &= 15;

                if (length32 != 0)
                {
                    __m256i pattern;
                    __m256i block;
                    ULONG mask;
                    ULONG index;
                    WCHAR upC = PhUpcaseUnicodeChar(Character);

                    pattern = _mm256_set1_epi16(upC);

                    do
                    {
                        block = _mm256_loadu_si256((__m256i*)buffer);
                        block = PhUppercaseLatin1INT256by16(block);
                        block = _mm256_cmpeq_epi16(block, pattern);
                        mask = _mm256_movemask_epi8(block);

                        if (mask != 0)
                        {
                            if (_BitScanForward(&index, mask))
                            {
                                _mm256_zeroupper();
                                return (buffer - String->Buffer) + index / 2;
                            }
                        }

                        buffer += 16;
                    } while (--length32 != 0);

                    _mm256_zeroupper();
                }
            }
            else
#endif
            if (PhHasIntrinsics)
            {
                SIZE_T length16;

                length16 = String->Length / 16;
                length &= 7;

                if (length16 != 0)
                {
                    PH_INT128 pattern;
                    PH_INT128 block;
                    ULONG mask;
                    ULONG index;
                    WCHAR upC = PhUpcaseUnicodeChar(Character);

                    pattern = PhSetINT128by16(upC);

                    do
                    {
                        block = PhLoadINT128U((PLONG)buffer);
                        block = PhUppercaseLatin1INT128by16(block);
                        block = PhCompareEqINT128by16(block, pattern);
                        mask = PhMoveMaskINT128by8(block);

                        if (_BitScanForward(&index, mask))
                        {
                            return (buffer - String->Buffer) + index / 2;
                        }

                        buffer += 8;
                    } while (--length16 != 0);
                }
            }
        }
        else
        {
            WCHAR upC = PhUpcaseUnicodeChar(Character);
            WCHAR lowC = PhDowncaseUnicodeChar(Character);

            if (upC == lowC)
            {
                // Not a casing character, use standard path.
                return PhFindCharInStringRef(String, Character, FALSE);
            }

#ifndef _ARM64_
            if (PhHasAVX)
            {
                SIZE_T length32;

                length32 = String->Length / 32;
                length &= 15;

                if (length32 != 0)
                {
                    __m256i patUp;
                    __m256i patLow;
                    __m256i block;
                    ULONG mask;
                    ULONG index;

                    patUp = _mm256_set1_epi16(upC);
                    patLow = _mm256_set1_epi16(lowC);

                    do
                    {
                        block = _mm256_loadu_si256((__m256i*)buffer);
                        block = _mm256_or_si256(_mm256_cmpeq_epi16(block, patUp), _mm256_cmpeq_epi16(block, patLow));
                        mask = _mm256_movemask_epi8(block);

                        if (mask != 0)
                        {
                            if (_BitScanForward(&index, mask))
                            {
                                _mm256_zeroupper();
                                return (buffer - String->Buffer) + index / 2;
                            }
                        }

                        buffer += 16;
                    } while (--length32 != 0);

                    _mm256_zeroupper();
                }
            }
            else
#endif
            if (PhHasIntrinsics)
            {
                SIZE_T length16;

                length16 = String->Length / 16;
                length &= 7;

                if (length16 != 0)
                {
                    PH_INT128 patUp;
                    PH_INT128 patLow;
                    PH_INT128 block;
                    ULONG mask;
                    ULONG index;

                    patUp = PhSetINT128by16(upC);
                    patLow = PhSetINT128by16(lowC);

                    do
                    {
                        block = PhLoadINT128U((PLONG)buffer);
                        block = PhOrINT128(PhCompareEqINT128by16(block, patUp), PhCompareEqINT128by16(block, patLow));
                        mask = PhMoveMaskINT128by8(block);

                        if (_BitScanForward(&index, mask))
                        {
                            return (buffer - String->Buffer) + index / 2;
                        }

                        buffer += 8;
                    } while (--length16 != 0);
                }
            }
        }

        if (length != 0)
        {
            WCHAR c;

            c = PhUpcaseUnicodeChar(Character);

            do
            {
                if (PhUpcaseUnicodeChar(*buffer) == c)
                {
                    return (ULONG_PTR)(buffer - String->Buffer);
                }

                buffer++;
            } while (--length != 0);
        }
    }
    else
    {
#ifndef _ARM64_
        if (PhHasAVX)
        {
            SIZE_T length32;

            length32 = String->Length / 32;
            length &= 15;

            if (length32 != 0)
            {
                __m256i pattern;
                __m256i block;
                ULONG mask;
                ULONG index;

                pattern = _mm256_set1_epi16(Character);

                do
                {
                    block = _mm256_loadu_si256((__m256i*)buffer);
                    block = _mm256_cmpeq_epi16(block, pattern);
                    mask = _mm256_movemask_epi8(block);

                    if (_BitScanForward(&index, mask))
                    {
                        _mm256_zeroupper();
                        return (buffer - String->Buffer) + index / 2;
                    }

                    buffer += 16;
                } while (--length32 != 0);

                _mm256_zeroupper();
            }
        }
        else
#endif
        if (PhHasIntrinsics)
        {
            SIZE_T length16;

            length16 = String->Length / 16;
            length &= 7;

            if (length16 != 0)
            {
                PH_INT128 pattern;
                PH_INT128 block;
                ULONG mask;
                ULONG index;

                pattern = PhSetINT128by16(Character);

                do
                {
                    block = PhLoadINT128U((PLONG)buffer);
                    block = PhCompareEqINT128by16(block, pattern);
                    mask = PhMoveMaskINT128by8(block);

                    if (_BitScanForward(&index, mask))
                    {
                        return (buffer - String->Buffer) + index / 2;
                    }

                    buffer += 16 / sizeof(WCHAR);
                } while (--length16 != 0);
            }
        }

        if (length != 0)
        {
            do
            {
                if (*buffer == Character)
                    return (ULONG_PTR)(buffer - String->Buffer);

                buffer++;
            } while (--length != 0);
        }
    }

    return SIZE_MAX;
}

/**
 * Locates a character in a string, searching backwards.
 *
 * \param String The string to search.
 * \param Character The character to search for.
 * \param IgnoreCase TRUE to perform a case-insensitive search, otherwise FALSE.
 * \return The index, in characters, of the last occurrence of \a Character in \a String1. If
 * \a Character was not found, -1 is returned.
 */
ULONG_PTR PhFindLastCharInStringRef(
    _In_ PCPH_STRINGREF String,
    _In_ WCHAR Character,
    _In_ BOOLEAN IgnoreCase
    )
{
    PWCHAR buffer;
    SIZE_T length;

    buffer = PTR_ADD_OFFSET(String->Buffer, String->Length);
    length = String->Length / sizeof(WCHAR);

    if (IgnoreCase)
    {
        if (Character <= 0xFF && Character != 0x00DF)
        {
#ifndef _ARM64_
            if (PhHasAVX)
            {
                SIZE_T length32;

                length32 = String->Length / 32;
                length &= 15;

                if (length32 != 0)
                {
                    __m256i pattern;
                    __m256i block;
                    ULONG mask;
                    ULONG index;
                    WCHAR upC = PhUpcaseUnicodeChar(Character);

                    pattern = _mm256_set1_epi16(upC);

                    do
                    {
                        buffer -= 16;
                        block = _mm256_loadu_si256((__m256i*)buffer);
                        block = PhUppercaseLatin1INT256by16(block);
                        block = _mm256_cmpeq_epi16(block, pattern);
                        mask = _mm256_movemask_epi8(block);

                        if (mask != 0)
                        {
                            if (_BitScanReverse(&index, mask))
                            {
                                _mm256_zeroupper();
                                return (buffer - String->Buffer) + index / 2;
                            }
                        }
                    } while (--length32 != 0);

                    _mm256_zeroupper();
                }
            }
            else
#endif
            if (PhHasIntrinsics)
            {
                SIZE_T length16;

                length16 = String->Length / 16;
                length &= 7;

                if (length16 != 0)
                {
                    PH_INT128 pattern;
                    PH_INT128 block;
                    ULONG mask;
                    ULONG index;
                    WCHAR upC = PhUpcaseUnicodeChar(Character);

                    pattern = PhSetINT128by16(upC);

                    do
                    {
                        buffer -= 8;
                        block = PhLoadINT128U((PLONG)buffer);
                        block = PhUppercaseLatin1INT128by16(block);
                        block = PhCompareEqINT128by16(block, pattern);
                        mask = PhMoveMaskINT128by8(block);

                        if (mask != 0)
                        {
                            if (_BitScanReverse(&index, mask))
                            {
                                return (buffer - String->Buffer) + index / 2;
                            }
                        }
                    } while (--length16 != 0);
                }
            }
        }
        else
        {
            WCHAR upC = PhUpcaseUnicodeChar(Character);
            WCHAR lowC = PhDowncaseUnicodeChar(Character);

            if (upC == lowC)
            {
                // Not a casing character, use standard path.
                return PhFindLastCharInStringRef(String, Character, FALSE);
            }

#ifndef _ARM64_
            if (PhHasAVX)
            {
                SIZE_T length32;

                length32 = String->Length / 32;
                length &= 15;

                if (length32 != 0)
                {
                    __m256i patUp;
                    __m256i patLow;
                    __m256i block;
                    ULONG mask;
                    ULONG index;

                    patUp = _mm256_set1_epi16(upC);
                    patLow = _mm256_set1_epi16(lowC);

                    do
                    {
                        buffer -= 16;
                        block = _mm256_loadu_si256((__m256i*)buffer);
                        block = _mm256_or_si256(_mm256_cmpeq_epi16(block, patUp), _mm256_cmpeq_epi16(block, patLow));
                        mask = _mm256_movemask_epi8(block);

                        if (mask != 0)
                        {
                            if (_BitScanReverse(&index, mask))
                            {
                                _mm256_zeroupper();
                                return (buffer - String->Buffer) + index / 2;
                            }
                        }
                    } while (--length32 != 0);

                    _mm256_zeroupper();
                }
            }
            else
#endif
            if (PhHasIntrinsics)
            {
                SIZE_T length16;

                length16 = String->Length / 16;
                length &= 7;

                if (length16 != 0)
                {
                    PH_INT128 patUp;
                    PH_INT128 patLow;
                    PH_INT128 block;
                    ULONG mask;
                    ULONG index;

                    patUp = PhSetINT128by16(upC);
                    patLow = PhSetINT128by16(lowC);

                    do
                    {
                        buffer -= 8;
                        block = PhLoadINT128U((PLONG)buffer);
                        block = PhOrINT128(PhCompareEqINT128by16(block, patUp), PhCompareEqINT128by16(block, patLow));
                        mask = PhMoveMaskINT128by8(block);

                        if (mask != 0)
                        {
                            if (_BitScanReverse(&index, mask))
                            {
                                return (buffer - String->Buffer) + index / 2;
                            }
                        }
                    } while (--length16 != 0);
                }
            }
        }

        if (length != 0)
        {
            WCHAR c;

            c = PhUpcaseUnicodeChar(Character);
            buffer--;

            do
            {
                if (PhUpcaseUnicodeChar(*buffer) == c)
                    return (ULONG_PTR)(buffer - String->Buffer);

                buffer--;
            } while (--length != 0);
        }
    }
    else
    {
#ifndef _ARM64_
        if (PhHasAVX)
        {
            SIZE_T length32;

            length32 = String->Length / 32;
            length &= 15;

            if (length32 != 0)
            {
                __m256i pattern;
                __m256i block;
                ULONG mask;
                ULONG index;

                pattern = _mm256_set1_epi16(Character);

                do
                {
                    buffer -= 16;
                    block = _mm256_loadu_si256((__m256i*)buffer);
                    block = _mm256_cmpeq_epi16(block, pattern);
                    mask = _mm256_movemask_epi8(block);

                    if (_BitScanReverse(&index, mask))
                    {
                        _mm256_zeroupper();
                        return (buffer - String->Buffer) + index / 2;
                    }
                    
                } while (--length32 != 0);

                _mm256_zeroupper();
            }
        }
        else
#endif
        if (PhHasIntrinsics)
        {
            SIZE_T length16;

            length16 = String->Length / 16;
            length &= 7;

            if (length16 != 0)
            {
                PH_INT128 pattern;
                PH_INT128 block;
                ULONG mask;
                ULONG index;

                pattern = PhSetINT128by16(Character);

                do
                {
                    buffer -= 8;
                    block = PhLoadINT128U((PLONG)buffer);
                    block = PhCompareEqINT128by16(block, pattern);
                    mask = PhMoveMaskINT128by8(block);

                    if (_BitScanReverse(&index, mask))
                    {
                        return (buffer - String->Buffer) + index / 2;
                    }
                    
                } while (--length16 != 0);
            }
        }

        if (length != 0)
        {
            buffer--;

            do
            {
                if (*buffer == Character)
                    return (ULONG_PTR)(buffer - String->Buffer);

                buffer--;
            } while (--length != 0);
        }
    }

    return SIZE_MAX;
}

/**
 * Locates a string in a string.
 *
 * \param String The string to search.
 * \param SubString The string to search for.
 * \param IgnoreCase TRUE to perform a case-insensitive search, otherwise FALSE.
 * \return The index, in characters, of the first occurrence of \a SubString in \a String. If
 * \a SubString was not found, -1 is returned.
 */
ULONG_PTR PhFindStringInStringRef(
    _In_ PCPH_STRINGREF String,
    _In_ PCPH_STRINGREF SubString,
    _In_ BOOLEAN IgnoreCase
    )
{
    SIZE_T length1;
    SIZE_T length2;

    length1 = String->Length / sizeof(WCHAR);
    length2 = SubString->Length / sizeof(WCHAR);

    // Can't be a substring if it's bigger than the first string.
    if (length2 > length1)
        return SIZE_MAX;
    // We always get a match if the substring is zero-length.
    if (length2 == 0)
        return 0;

#if defined(PH_NATIVE_STRING_IN_STRING)
    {
        PH_STRINGREF haystackRef;
        PH_STRINGREF sr1;
        PH_STRINGREF sr2;
        WCHAR c;
        PWSTR haystack;
        SIZE_T haystackCount;

        if (length2 == 1)
            return PhFindCharInStringRef(String, SubString->Buffer[0], IgnoreCase);

        haystack = String->Buffer;
        haystackCount = length1 - length2 + 1;

        sr2.Buffer = SubString->Buffer + 1;
        sr2.Length = SubString->Length - sizeof(WCHAR);
        sr1.Length = sr2.Length;

        if (IgnoreCase)
        {
            c = PhUpcaseUnicodeChar(SubString->Buffer[0]);

            while (haystackCount > 0)
            {
                haystackRef.Buffer = haystack;
                haystackRef.Length = haystackCount * sizeof(WCHAR);

                ULONG_PTR offset = PhFindCharInStringRef(&haystackRef, c, TRUE);
                if (offset == SIZE_MAX)
                    break;

                haystack += offset;
                haystackCount -= offset;

                sr1.Buffer = haystack + 1;
                if (PhEqualStringRef(&sr1, &sr2, TRUE))
                    return (ULONG_PTR)(haystack - String->Buffer);

                haystack++;
                haystackCount--;
            }
        }
        else
        {
            c = SubString->Buffer[0];

            while (haystackCount > 0)
            {
                haystackRef.Buffer = haystack;
                haystackRef.Length = haystackCount * sizeof(WCHAR);

                ULONG_PTR offset = PhFindCharInStringRef(&haystackRef, c, FALSE);
                if (offset == SIZE_MAX)
                    break;

                haystack += offset;
                haystackCount -= offset;

                sr1.Buffer = haystack + 1;
                if (PhEqualStringRef(&sr1, &sr2, FALSE))
                    return (ULONG_PTR)(haystack - String->Buffer);

                haystack++;
                haystackCount--;
            }
        }
    }
#else
    {
        PH_STRINGREF sr1;
        PH_STRINGREF sr2;
        WCHAR c;
        SIZE_T i;

        sr1.Buffer = String->Buffer;
        sr1.Length = SubString->Length - sizeof(WCHAR);
        sr2.Buffer = SubString->Buffer;
        sr2.Length = SubString->Length - sizeof(WCHAR);

        if (IgnoreCase)
        {
            c = PhUpcaseUnicodeChar(*sr2.Buffer++);

            for (i = length1 - length2 + 1; i != 0; i--)
            {
                if (PhUpcaseUnicodeChar(*sr1.Buffer++) == c && PhEqualStringRef(&sr1, &sr2, TRUE))
                {
                    goto FoundUString;
                }
            }
        }
        else
        {
            c = *sr2.Buffer++;

            for (i = length1 - length2 + 1; i != 0; i--)
            {
                if (*sr1.Buffer++ == c && PhEqualStringRef(&sr1, &sr2, FALSE))
                {
                    goto FoundUString;
                }
            }
        }

        return SIZE_MAX;
FoundUString:
        return (ULONG_PTR)(sr1.Buffer - String->Buffer - 1);
    }
#endif

    return SIZE_MAX;
}

/**
 * Splits a string.
 *
 * \param Input The input string.
 * \param Separator The character to split at.
 * \param FirstPart A variable which receives the part of \a Input before the separator. This may be
 * the same variable as \a Input. If the separator is not found in \a Input, this variable is set to
 * \a Input.
 * \param SecondPart A variable which receives the part of \a Input after the separator. This may be
 * the same variable as \a Input. If the separator is not found in \a Input, this variable is set to
 * an empty string.
 * \return TRUE if \a Separator was found in \a Input, otherwise FALSE.
 */
BOOLEAN PhSplitStringRefAtChar(
    _In_ PCPH_STRINGREF Input,
    _In_ WCHAR Separator,
    _Out_ PPH_STRINGREF FirstPart,
    _Out_ PPH_STRINGREF SecondPart
    )
{
    PH_STRINGREF input;
    ULONG_PTR index;

    input = *Input; // get a copy of the input because FirstPart/SecondPart may alias Input
    index = PhFindCharInStringRef(Input, Separator, FALSE);

    if (index == SIZE_MAX)
    {
        // The separator was not found.

        FirstPart->Buffer = Input->Buffer;
        FirstPart->Length = Input->Length;
        SecondPart->Buffer = NULL;
        SecondPart->Length = 0;

        return FALSE;
    }

    FirstPart->Buffer = input.Buffer;
    FirstPart->Length = index * sizeof(WCHAR);
    SecondPart->Buffer = PTR_ADD_OFFSET(input.Buffer, index * sizeof(WCHAR) + sizeof(UNICODE_NULL));
    SecondPart->Length = input.Length - index * sizeof(WCHAR) - sizeof(UNICODE_NULL);

    return TRUE;
}

/**
 * Splits a string at the last occurrence of a character.
 *
 * \param Input The input string.
 * \param Separator The character to split at.
 * \param FirstPart A variable which receives the part of \a Input before the separator. This may be
 * the same variable as \a Input. If the separator is not found in \a Input, this variable is set to
 * \a Input.
 * \param SecondPart A variable which receives the part of \a Input after the separator. This may be
 * the same variable as \a Input. If the separator is not found in \a Input, this variable is set to
 * an empty string.
 * \return TRUE if \a Separator was found in \a Input, otherwise FALSE.
 */
BOOLEAN PhSplitStringRefAtLastChar(
    _In_ PCPH_STRINGREF Input,
    _In_ WCHAR Separator,
    _Out_ PPH_STRINGREF FirstPart,
    _Out_ PPH_STRINGREF SecondPart
    )
{
    PH_STRINGREF input;
    ULONG_PTR index;

    input = *Input; // get a copy of the input because FirstPart/SecondPart may alias Input
    index = PhFindLastCharInStringRef(Input, Separator, FALSE);

    if (index == SIZE_MAX)
    {
        // The separator was not found.

        FirstPart->Buffer = Input->Buffer;
        FirstPart->Length = Input->Length;
        SecondPart->Buffer = NULL;
        SecondPart->Length = 0;

        return FALSE;
    }

    FirstPart->Buffer = input.Buffer;
    FirstPart->Length = index * sizeof(WCHAR);
    SecondPart->Buffer = PTR_ADD_OFFSET(input.Buffer, (index + 1) * sizeof(WCHAR));
    SecondPart->Length = input.Length - (index + 1) * sizeof(WCHAR);

    return TRUE;
}

/**
 * Splits a string.
 *
 * \param Input The input string.
 * \param Separator The string to split at.
 * \param IgnoreCase TRUE to perform a case-insensitive search, otherwise FALSE.
 * \param FirstPart A variable which receives the part of \a Input before the separator. This may be
 * the same variable as \a Input. If the separator is not found in \a Input, this variable is set to
 * \a Input.
 * \param SecondPart A variable which receives the part of \a Input after the separator. This may be
 * the same variable as \a Input. If the separator is not found in \a Input, this variable is set to
 * an empty string.
 * \return TRUE if \a Separator was found in \a Input, otherwise FALSE.
 */
BOOLEAN PhSplitStringRefAtString(
    _In_ PCPH_STRINGREF Input,
    _In_ PCPH_STRINGREF Separator,
    _In_ BOOLEAN IgnoreCase,
    _Out_ PPH_STRINGREF FirstPart,
    _Out_ PPH_STRINGREF SecondPart
    )
{
    PH_STRINGREF input;
    ULONG_PTR index;

    input = *Input; // get a copy of the input because FirstPart/SecondPart may alias Input
    index = PhFindStringInStringRef(Input, Separator, IgnoreCase);

    if (index == SIZE_MAX)
    {
        // The separator was not found.

        FirstPart->Buffer = Input->Buffer;
        FirstPart->Length = Input->Length;
        SecondPart->Buffer = NULL;
        SecondPart->Length = 0;

        return FALSE;
    }

    FirstPart->Buffer = input.Buffer;
    FirstPart->Length = index * sizeof(WCHAR);
    SecondPart->Buffer = PTR_ADD_OFFSET(input.Buffer, index * sizeof(WCHAR) + Separator->Length);
    SecondPart->Length = input.Length - index * sizeof(WCHAR) - Separator->Length;

    return TRUE;
}

/**
 * Splits a string.
 *
 * \param Input The input string.
 * \param Separator The character set, string or range to split at.
 * \param Flags A combination of flags.
 * \li \c PH_SPLIT_AT_CHAR_SET \a Separator specifies a character set. \a Input will be split at a
 * character which is contained in \a Separator.
 * \li \c PH_SPLIT_AT_STRING \a Separator specifies a string. \a Input will be split an occurrence
 * of \a Separator.
 * \li \c PH_SPLIT_AT_RANGE \a Separator specifies a range. The \a Buffer field contains a character
 * index cast to \c PWSTR and the \a Length field contains the length of the range, in bytes.
 * \li \c PH_SPLIT_CASE_INSENSITIVE Specifies a case-insensitive search.
 * \li \c PH_SPLIT_COMPLEMENT_CHAR_SET If used with \c PH_SPLIT_AT_CHAR_SET, the separator is a
 * character which is not contained in \a Separator.
 * \li \c PH_SPLIT_START_AT_END If used with \c PH_SPLIT_AT_CHAR_SET, the search is performed
 * starting from the end of the string.
 * \li \c PH_SPLIT_CHAR_SET_IS_UPPERCASE If used with \c PH_SPLIT_CASE_INSENSITIVE, specifies that
 * the character set in \a Separator contains only uppercase characters.
 * \param FirstPart A variable which receives the part of \a Input before the separator. This may be
 * the same variable as \a Input. If the separator is not found in \a Input, this variable is set to
 * \a Input.
 * \param SecondPart A variable which receives the part of \a Input after the separator. This may be
 * the same variable as \a Input. If the separator is not found in \a Input, this variable is set to
 * an empty string.
 * \param SeparatorPart A variable which receives the part of \a Input that is the separator. If the
 * separator is not found in \a Input, this variable is set to an empty string.
 * \return TRUE if a separator was found in \a Input, otherwise FALSE.
 */
BOOLEAN PhSplitStringRefEx(
    _In_ PCPH_STRINGREF Input,
    _In_ PCPH_STRINGREF Separator,
    _In_ ULONG Flags,
    _Out_ PPH_STRINGREF FirstPart,
    _Out_ PPH_STRINGREF SecondPart,
    _Out_opt_ PPH_STRINGREF SeparatorPart
    )
{
    PH_STRINGREF input;
    SIZE_T separatorIndex;
    SIZE_T separatorLength;
    PWCHAR charSet;
    SIZE_T charSetCount;
    BOOLEAN charSetTable[256];
    BOOLEAN charSetTableComplete;
    SIZE_T i;
    SIZE_T j;
    USHORT c;
    PWCHAR s;
    LONG_PTR direction;

    input = *Input; // Get a copy of the input because FirstPart/SecondPart/SeparatorPart may alias Input

    if (Flags & PH_SPLIT_AT_RANGE)
    {
        separatorIndex = (SIZE_T)Separator->Buffer;
        separatorLength = Separator->Length;

        if (separatorIndex == SIZE_MAX)
            goto SeparatorNotFound;

        goto SeparatorFound;
    }
    else if (Flags & PH_SPLIT_AT_STRING)
    {
        if (Flags & PH_SPLIT_START_AT_END)
        {
            // not implemented
            goto SeparatorNotFound;
        }

        separatorIndex = PhFindStringInStringRef(Input, Separator, !!(Flags & PH_SPLIT_CASE_INSENSITIVE));

        if (separatorIndex == SIZE_MAX)
            goto SeparatorNotFound;

        separatorLength = Separator->Length;
        goto SeparatorFound;
    }

    // Special case for character sets with only one character.
    if (!(Flags & PH_SPLIT_COMPLEMENT_CHAR_SET) && Separator->Length == sizeof(WCHAR))
    {
        if (!(Flags & PH_SPLIT_START_AT_END))
            separatorIndex = PhFindCharInStringRef(Input, Separator->Buffer[0], !!(Flags & PH_SPLIT_CASE_INSENSITIVE));
        else
            separatorIndex = PhFindLastCharInStringRef(Input, Separator->Buffer[0], !!(Flags & PH_SPLIT_CASE_INSENSITIVE));

        if (separatorIndex == SIZE_MAX)
            goto SeparatorNotFound;

        separatorLength = sizeof(WCHAR);
        goto SeparatorFound;
    }

    if (input.Length == 0)
        goto SeparatorNotFound;

    // Build the character set lookup table.

    charSet = Separator->Buffer;
    charSetCount = Separator->Length / sizeof(WCHAR);
    memset(charSetTable, 0, sizeof(charSetTable));
    charSetTableComplete = TRUE;

    for (i = 0; i < charSetCount; i++)
    {
        c = charSet[i];

        if (Flags & PH_SPLIT_CASE_INSENSITIVE)
            c = PhUpcaseUnicodeChar(c);

        charSetTable[c & 0xff] = TRUE;

        if (c >= 256)
            charSetTableComplete = FALSE;
    }

    // Perform the search.

    i = input.Length / sizeof(WCHAR);
    separatorLength = sizeof(WCHAR);

    if (!(Flags & PH_SPLIT_START_AT_END))
    {
        s = input.Buffer;
        direction = 1;
    }
    else
    {
        s = PTR_ADD_OFFSET(input.Buffer, input.Length - sizeof(WCHAR));
        direction = -1;
    }

    do
    {
        c = *s;

        if (Flags & PH_SPLIT_CASE_INSENSITIVE)
            c = PhUpcaseUnicodeChar(c);

        if (c < 256 && charSetTableComplete)
        {
            if (!(Flags & PH_SPLIT_COMPLEMENT_CHAR_SET))
            {
                if (charSetTable[c])
                    goto CharFound;
            }
            else
            {
                if (!charSetTable[c])
                    goto CharFound;
            }
        }
        else
        {
            if (!(Flags & PH_SPLIT_COMPLEMENT_CHAR_SET))
            {
                if (charSetTable[c & 0xff])
                {
                    if (!(Flags & PH_SPLIT_CASE_INSENSITIVE) || (Flags & PH_SPLIT_CHAR_SET_IS_UPPERCASE))
                    {
                        for (j = 0; j < charSetCount; j++)
                        {
                            if (charSet[j] == c)
                                goto CharFound;
                        }
                    }
                    else
                    {
                        for (j = 0; j < charSetCount; j++)
                        {
                            if (PhUpcaseUnicodeChar(charSet[j]) == c)
                                goto CharFound;
                        }
                    }
                }
            }
            else
            {
                if (charSetTable[c & 0xff])
                {
                    if (!(Flags & PH_SPLIT_CASE_INSENSITIVE) || (Flags & PH_SPLIT_CHAR_SET_IS_UPPERCASE))
                    {
                        for (j = 0; j < charSetCount; j++)
                        {
                            if (charSet[j] == c)
                                break;
                        }
                    }
                    else
                    {
                        for (j = 0; j < charSetCount; j++)
                        {
                            if (PhUpcaseUnicodeChar(charSet[j]) == c)
                                break;
                        }
                    }

                    if (j == charSetCount)
                        goto CharFound;
                }
                else
                {
                    goto CharFound;
                }
            }
        }

        s += direction;
    } while (--i != 0);

    goto SeparatorNotFound;

CharFound:
    separatorIndex = s - input.Buffer;

SeparatorFound:
    FirstPart->Buffer = input.Buffer;
    FirstPart->Length = separatorIndex * sizeof(WCHAR);
    SecondPart->Buffer = PTR_ADD_OFFSET(input.Buffer, separatorIndex * sizeof(WCHAR) + separatorLength);
    SecondPart->Length = input.Length - separatorIndex * sizeof(WCHAR) - separatorLength;

    if (SeparatorPart)
    {
        SeparatorPart->Buffer = input.Buffer + separatorIndex;
        SeparatorPart->Length = separatorLength;
    }

    return TRUE;

SeparatorNotFound:
    FirstPart->Buffer = input.Buffer;
    FirstPart->Length = input.Length;
    SecondPart->Buffer = NULL;
    SecondPart->Length = 0;

    if (SeparatorPart)
    {
        SeparatorPart->Buffer = NULL;
        SeparatorPart->Length = 0;
    }

    return FALSE;
}

VOID PhSplitStringRef(
    _In_ PCPH_STRINGREF Input,
    _In_ WCHAR Separator,
    _Out_ PVOID **Strings,
    _Out_ PULONG NumberOfStrings
    )
{
    PH_ARRAY array;
    PH_STRINGREF remainingPart;
    PH_STRINGREF part;

    PhInitializeArray(&array, sizeof(PPH_STRING), 4);
    remainingPart = *Input;

    while (PhSplitStringRefAtChar(&remainingPart, Separator, &part, &remainingPart))
    {
        if (part.Length > 0)
        {
            PPH_STRING string = PhCreateString2(&part);
            PhAddItemArray(&array, &string);
        }
    }

    if (remainingPart.Length > 0)
    {
        PPH_STRING string = PhCreateString2(&remainingPart);
        PhAddItemArray(&array, &string);
    }

    *NumberOfStrings = (ULONG)PhFinalArrayCount(&array);
    *Strings = (PVOID *)PhFinalArrayItems(&array);
}

VOID PhFreeStringArray(
    _In_ PVOID *Strings,
    _In_ ULONG NumberOfStrings
    )
{
    for (ULONG i = 0; i < NumberOfStrings; i++)
    {
        PhDereferenceObject(Strings[i]);
    }

    PhFree(Strings);
}

/**
 * Trims characters from the beginning and/or end of a string reference.
 *
 * \param String Pointer to a PH_STRINGREF structure representing the string to be trimmed. The string is modified in place.
 * \param CharSet Pointer to a PH_STRINGREF structure containing the set of characters to trim from the string.
 * \param Flags Specifies trimming options. Can be used to indicate whether to trim from the left, right, or both ends.
 * \remarks This function removes all characters specified in CharSet from the start and/or end of the string referenced by String,
 * depending on the Flags provided.
 */
VOID PhTrimStringRef(
    _Inout_ PPH_STRINGREF String,
    _In_ PCPH_STRINGREF CharSet,
    _In_ ULONG Flags
    )
{
    PWCHAR charSet;
    SIZE_T charSetCount;
    BOOLEAN charSetTable[256];
    BOOLEAN charSetTableComplete;
    SIZE_T i;
    SIZE_T j;
    USHORT c;
    SIZE_T trimCount;
    SIZE_T count;
    PWCHAR s;

    if (!String->Buffer || String->Length == 0 || CharSet->Length == 0)
        return;

    if (CharSet->Length == sizeof(WCHAR))
    {
        c = CharSet->Buffer[0];

        if (!(Flags & PH_TRIM_END_ONLY))
        {
            trimCount = 0;
            count = String->Length / sizeof(WCHAR);
            s = String->Buffer;

            while (count-- != 0)
            {
                if (*s++ != c)
                    break;

                trimCount++;
            }

            PhSkipStringRef(String, trimCount * sizeof(WCHAR));
        }

        if (!(Flags & PH_TRIM_START_ONLY))
        {
            trimCount = 0;
            count = String->Length / sizeof(WCHAR);
            s = PTR_ADD_OFFSET(String->Buffer, String->Length - sizeof(WCHAR));

            while (count-- != 0)
            {
                if (*s-- != c)
                    break;

                trimCount++;
            }

            String->Length -= trimCount * sizeof(WCHAR);
        }

        return;
    }

    // Build the character set lookup table.

    charSet = CharSet->Buffer;
    charSetCount = CharSet->Length / sizeof(WCHAR);
    memset(charSetTable, 0, sizeof(charSetTable));
    charSetTableComplete = TRUE;

    for (i = 0; i < charSetCount; i++)
    {
        c = charSet[i];
        charSetTable[c & 0xff] = TRUE;

        if (c >= 256)
            charSetTableComplete = FALSE;
    }

    // Trim the string.

    if (!(Flags & PH_TRIM_END_ONLY))
    {
        trimCount = 0;
        count = String->Length / sizeof(WCHAR);
        s = String->Buffer;

        while (count-- != 0)
        {
            c = *s++;

            if (!charSetTable[c & 0xff])
                break;

            if (!charSetTableComplete)
            {
                for (j = 0; j < charSetCount; j++)
                {
                    if (charSet[j] == c)
                        goto CharFound;
                }

                break;
            }

CharFound:
            trimCount++;
        }

        PhSkipStringRef(String, trimCount * sizeof(WCHAR));
    }

    if (!(Flags & PH_TRIM_START_ONLY))
    {
        trimCount = 0;
        count = String->Length / sizeof(WCHAR);
        s = PTR_ADD_OFFSET(String->Buffer, String->Length - sizeof(WCHAR));

        while (count-- != 0)
        {
            c = *s--;

            if (!charSetTable[c & 0xff])
                break;

            if (!charSetTableComplete)
            {
                for (j = 0; j < charSetCount; j++)
                {
                    if (charSet[j] == c)
                        goto CharFound2;
                }

                break;
            }

CharFound2:
            trimCount++;
        }

        String->Length -= trimCount * sizeof(WCHAR);
    }
}

/**
 * Creates a string object using a specified length.
 *
 * \param Buffer A null-terminated Unicode string.
 * \param Length The length, in bytes, of the string.
 */
PPH_STRING PhCreateStringEx(
    _In_opt_ PCWCHAR Buffer,
    _In_ SIZE_T Length
    )
{
    PPH_STRING string;

    string = PhCreateObject(
        UFIELD_OFFSET(PH_STRING, Data) + Length + sizeof(UNICODE_NULL),
        PhStringType
        );

    assert(!(Length & 1));
    string->Length = Length;
    string->Buffer = string->Data;
    *(PWCHAR)PTR_ADD_OFFSET(string->Buffer, Length) = UNICODE_NULL;

    if (Buffer)
    {
        memcpy(string->Buffer, Buffer, Length);
    }

    return string;
}

/**
 * Creates a string object using a specified length and mode.
 *
 * \param String A string reference to create a new string object from.
 * \param Flags A combination of PH_STRING flags.
 * \param TrimCharSet A string containing characters to trim. If NULL, no trimming is performed.
 */
PPH_STRING PhCreateString3(
    _In_ PCPH_STRINGREF String,
    _In_ ULONG Flags,
    _In_opt_ PCPH_STRINGREF TrimCharSet
    )
{
    PPH_STRING string;
    PH_STRINGREF sr;

    sr = *String;
    if (TrimCharSet)
        PhTrimStringRef(&sr, TrimCharSet, Flags & PH_STRING_TRIM_MASK);

    string = PhCreateObject(
        UFIELD_OFFSET(PH_STRING, Data) + sr.Length + sizeof(UNICODE_NULL), // Null terminator for compatibility
        PhStringType
        );

    assert(!(sr.Length & 1));
    string->Length = sr.Length;
    string->Buffer = string->Data;
    *(PWCHAR)PTR_ADD_OFFSET(string->Buffer, sr.Length) = UNICODE_NULL;

    if (!sr.Buffer)
        return string;

    if (!(Flags & PH_STRING_CASE_MASK))
    {
        memcpy(string->Buffer, sr.Buffer, sr.Length);
        return string;
    }

    if (Flags & PH_STRING_UPPER_CASE)
        PhUpperStringRefInto(&string->sr, &sr);
    else
        PhLowerStringRefInto(&string->sr, &sr);

    return string;
}

/**
 * Obtains a reference to a zero-length string.
 */
PPH_STRING PhReferenceEmptyString(
    VOID
    )
{
    PPH_STRING string;
    PPH_STRING newString;

    // Initial lightweight atomic read
    string = ReadPointerAcquire(&PhSharedEmptyString);

    if (!string)
    {
        newString = PhCreateStringEx(NULL, 0);

        // Final atomic compare-and-swap
        string = InterlockedCompareExchangePointer(
            &PhSharedEmptyString,
            newString,
            NULL
            );

        if (string)
        {
            PhDereferenceObject(newString);
        }
        else
        {
            string = newString; // success
        }
    }

    return PhReferenceObject(string);
}

/**
 * Concatenates multiple strings.
 *
 * \param Count The number of strings to concatenate.
 * \param ... The list of strings.
 */
PPH_STRING PhConcatStrings(
    _In_ ULONG Count,
    ...
    )
{
    PPH_STRING string;
    va_list argptr;

    va_start(argptr, Count);
    string = PhConcatStrings_V(Count, argptr);
    va_end(argptr);

    return string;
}

/**
 * Concatenates multiple strings.
 *
 * \param Count The number of strings to concatenate.
 * \param ArgPtr A pointer to an array of strings.
 */
PPH_STRING PhConcatStrings_V(
    _In_ ULONG Count,
    _In_ va_list ArgPtr
    )
{
    va_list argptr;
    ULONG i;
    SIZE_T totalLength = 0;
    SIZE_T stringLength;
    SIZE_T cachedLengths[PH_CONCAT_STRINGS_LENGTH_CACHE_SIZE];
    PWSTR arg;
    PPH_STRING string;

    // Compute the total length, in bytes, of the strings.

    argptr = ArgPtr;

    for (i = 0; i < Count; i++)
    {
        arg = va_arg(argptr, PWSTR);
        stringLength = PhCountStringZ(arg) * sizeof(WCHAR);
        totalLength += stringLength;

        if (i < PH_CONCAT_STRINGS_LENGTH_CACHE_SIZE)
            cachedLengths[i] = stringLength;
    }

    // Create the string.

    string = PhCreateStringEx(NULL, totalLength);
    totalLength = 0;

    // Append the strings one by one.

    argptr = ArgPtr;

    for (i = 0; i < Count; i++)
    {
        arg = va_arg(argptr, PWSTR);

        if (i < PH_CONCAT_STRINGS_LENGTH_CACHE_SIZE)
            stringLength = cachedLengths[i];
        else
            stringLength = PhCountStringZ(arg) * sizeof(WCHAR);

        memcpy(
            PTR_ADD_OFFSET(string->Buffer, totalLength),
            arg,
            stringLength
            );
        totalLength += stringLength;
    }

    return string;
}

/**
 * Concatenates two strings.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 */
PPH_STRING PhConcatStrings2(
    _In_ PCWSTR String1,
    _In_ PCWSTR String2
    )
{
    PPH_STRING string;
    SIZE_T length1;
    SIZE_T length2;

    length1 = PhCountStringZ(String1) * sizeof(WCHAR);
    length2 = PhCountStringZ(String2) * sizeof(WCHAR);
    string = PhCreateStringEx(NULL, length1 + length2);
    memcpy(
        string->Buffer,
        String1,
        length1
        );
    memcpy(
        PTR_ADD_OFFSET(string->Buffer, length1),
        String2,
        length2
        );

    return string;
}

/**
 * Concatenates two strings.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 */
PPH_STRING PhConcatStringRef2(
    _In_ PCPH_STRINGREF String1,
    _In_ PCPH_STRINGREF String2
    )
{
    PPH_STRING string;

    assert(!(String1->Length & 1));
    assert(!(String2->Length & 1));

    string = PhCreateStringEx(NULL, String1->Length + String2->Length);

    memcpy(
        string->Buffer,
        String1->Buffer,
        String1->Length
        );
    memcpy(
        PTR_ADD_OFFSET(string->Buffer, String1->Length),
        String2->Buffer,
        String2->Length
        );

    return string;
}

/**
 * Concatenates three strings.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param String3 The third string.
 */
PPH_STRING PhConcatStringRef3(
    _In_ PCPH_STRINGREF String1,
    _In_ PCPH_STRINGREF String2,
    _In_ PCPH_STRINGREF String3
    )
{
    PPH_STRING string;
    PCHAR buffer;

    assert(!(String1->Length & 1));
    assert(!(String2->Length & 1));
    assert(!(String3->Length & 1));

    string = PhCreateStringEx(NULL, String1->Length + String2->Length + String3->Length);

    buffer = (PCHAR)string->Buffer;
    memcpy(buffer, String1->Buffer, String1->Length);

    buffer += String1->Length;
    memcpy(buffer, String2->Buffer, String2->Length);

    buffer += String2->Length;
    memcpy(buffer, String3->Buffer, String3->Length);

    return string;
}

/**
 * Concatenates four strings.
 *
 * \param String1 The first string.
 * \param String2 The second string.
 * \param String3 The third string.
 * \param String4 The forth string.
 */
PPH_STRING PhConcatStringRef4(
    _In_ PCPH_STRINGREF String1,
    _In_ PCPH_STRINGREF String2,
    _In_ PCPH_STRINGREF String3,
    _In_ PCPH_STRINGREF String4
    )
{
    PPH_STRING string;
    PCHAR buffer;

    assert(!(String1->Length & 1));
    assert(!(String2->Length & 1));
    assert(!(String3->Length & 1));
    assert(!(String4->Length & 1));

    string = PhCreateStringEx(NULL, String1->Length + String2->Length + String3->Length + String4->Length);

    buffer = (PCHAR)string->Buffer;
    memcpy(buffer, String1->Buffer, String1->Length);

    buffer += String1->Length;
    memcpy(buffer, String2->Buffer, String2->Length);

    buffer += String2->Length;
    memcpy(buffer, String3->Buffer, String3->Length);

    buffer += String3->Length;
    memcpy(buffer, String4->Buffer, String4->Length);

    return string;
}

/**
 * Creates a string using format specifiers.
 *
 * \param Format The format-control string.
 * \param ... The list of arguments.
 */
PPH_STRING PhFormatString(
    _In_ _Printf_format_string_ PCWSTR Format,
    ...
    )
{
    PPH_STRING string;
    va_list argptr;

    va_start(argptr, Format);
    string = PhFormatString_V(Format, argptr);
    va_end(argptr);

    return string;
}

/**
 * Creates a string using format specifiers.
 *
 * \param Format The format-control string.
 * \param ArgPtr A pointer to the list of arguments.
 */
PPH_STRING PhFormatString_V(
    _In_ _Printf_format_string_ PCWSTR Format,
    _In_ va_list ArgPtr
    )
{
    PPH_STRING string;
    LONG length;

    length = _vscwprintf(Format, ArgPtr);

    if (length == INT_ERROR)
        return NULL;

    string = PhCreateStringEx(NULL, length * sizeof(WCHAR));
    _vsnwprintf(string->Buffer, length, Format, ArgPtr);

    return string;
}

/**
 * Creates a bytes object.
 *
 * \param Buffer An array of bytes.
 * \param Length The length of \a Buffer, in bytes.
 */
PPH_BYTES PhCreateBytesEx(
    _In_opt_ PCCH Buffer,
    _In_ SIZE_T Length
    )
{
    PPH_BYTES bytes;

    bytes = PhCreateObject(
        UFIELD_OFFSET(PH_BYTES, Data) + Length + sizeof(ANSI_NULL), // Null terminator for compatibility
        PhBytesType
        );

    bytes->Length = Length;
    bytes->Buffer = bytes->Data;
    bytes->Buffer[Length] = ANSI_NULL;

    if (Buffer)
    {
        memcpy(bytes->Buffer, Buffer, Length);
    }

    return bytes;
}

/**
 * Formats a string using the specified format and argument list.
 *
 * \param Format A printf-style format string.
 * \param ArgPtr A va_list containing the arguments to format.
 * \return A pointer to a PPH_BYTES structure containing the formatted string.
 */
PPH_BYTES PhFormatBytes_V(
    _In_ _Printf_format_string_ PCSTR Format,
    _In_ va_list ArgPtr
    )
{
    PPH_BYTES string;
    LONG length;

    length = _vscprintf(Format, ArgPtr);

    if (length == INT_ERROR)
        return NULL;

    string = PhCreateBytesEx(NULL, length * sizeof(CHAR));
    _vsnprintf(string->Buffer, length, Format, ArgPtr);

    return string;
}

/**
 * Formats a sequence of bytes according to a specified format string.
 *
 * \param Format A printf-style format string that specifies how the bytes should be formatted.
 * \param ... Additional arguments required by the format string.
 * \return A pointer to a PPH_BYTES structure containing the formatted bytes.
 */
PPH_BYTES PhFormatBytes(
    _In_ _Printf_format_string_ PCSTR Format,
    ...
    )
{
    PPH_BYTES string;
    va_list argptr;

    va_start(argptr, Format);
    string = PhFormatBytes_V(Format, argptr);
    va_end(argptr);

    return string;
}

BOOLEAN PhWriteUnicodeDecoder(
    _Inout_ PPH_UNICODE_DECODER Decoder,
    _In_ ULONG CodeUnit
    )
{
    switch (Decoder->Encoding)
    {
    case PH_UNICODE_UTF8:
        if (Decoder->InputCount >= 4)
            return FALSE;
        Decoder->u.Utf8.Input[Decoder->InputCount] = (UCHAR)CodeUnit;
        Decoder->InputCount++;
        return TRUE;
    case PH_UNICODE_UTF16:
        if (Decoder->InputCount >= 2)
            return FALSE;
        Decoder->u.Utf16.Input[Decoder->InputCount] = (USHORT)CodeUnit;
        Decoder->InputCount++;
        return TRUE;
    case PH_UNICODE_UTF32:
        if (Decoder->InputCount >= 1)
            return FALSE;
        Decoder->u.Utf32.Input = CodeUnit;
        Decoder->InputCount = 1;
        return TRUE;
    default:
        PhRaiseStatus(STATUS_UNSUCCESSFUL);
    }
}

_Success_(return)
BOOLEAN PhpReadUnicodeDecoder(
    _Inout_ PPH_UNICODE_DECODER Decoder,
    _Out_ PULONG CodeUnit
    )
{
    switch (Decoder->Encoding)
    {
    case PH_UNICODE_UTF8:
        if (Decoder->InputCount == 0)
            return FALSE;
        *CodeUnit = Decoder->u.Utf8.Input[0];
        Decoder->u.Utf8.Input[0] = Decoder->u.Utf8.Input[1];
        Decoder->u.Utf8.Input[1] = Decoder->u.Utf8.Input[2];
        Decoder->u.Utf8.Input[2] = Decoder->u.Utf8.Input[3];
        Decoder->InputCount--;
        return TRUE;
    case PH_UNICODE_UTF16:
        if (Decoder->InputCount == 0)
            return FALSE;
        *CodeUnit = Decoder->u.Utf16.Input[0];
        Decoder->u.Utf16.Input[0] = Decoder->u.Utf16.Input[1];
        Decoder->InputCount--;
        return TRUE;
    case PH_UNICODE_UTF32:
        if (Decoder->InputCount == 0)
            return FALSE;
        *CodeUnit = Decoder->u.Utf32.Input;
        Decoder->InputCount--;
        return TRUE;
    default:
        PhRaiseStatus(STATUS_UNSUCCESSFUL);
    }
}

VOID PhpUnreadUnicodeDecoder(
    _Inout_ PPH_UNICODE_DECODER Decoder,
    _In_ ULONG CodeUnit
    )
{
    switch (Decoder->Encoding)
    {
    case PH_UNICODE_UTF8:
        if (Decoder->InputCount >= 4)
            PhRaiseStatus(STATUS_UNSUCCESSFUL);
        Decoder->u.Utf8.Input[3] = Decoder->u.Utf8.Input[2];
        Decoder->u.Utf8.Input[2] = Decoder->u.Utf8.Input[1];
        Decoder->u.Utf8.Input[1] = Decoder->u.Utf8.Input[0];
        Decoder->u.Utf8.Input[0] = (UCHAR)CodeUnit;
        Decoder->InputCount++;
        break;
    case PH_UNICODE_UTF16:
        if (Decoder->InputCount >= 2)
            PhRaiseStatus(STATUS_UNSUCCESSFUL);
        Decoder->u.Utf16.Input[1] = Decoder->u.Utf16.Input[0];
        Decoder->u.Utf16.Input[0] = (USHORT)CodeUnit;
        Decoder->InputCount++;
        break;
    case PH_UNICODE_UTF32:
        if (Decoder->InputCount >= 1)
            PhRaiseStatus(STATUS_UNSUCCESSFUL);
        Decoder->u.Utf32.Input = CodeUnit;
        Decoder->InputCount = 1;
        break;
    default:
        PhRaiseStatus(STATUS_UNSUCCESSFUL);
    }
}

BOOLEAN PhpDecodeUtf8Error(
    _Inout_ PPH_UNICODE_DECODER Decoder,
    _Out_ PULONG CodePoint,
    _In_ ULONG Which
    )
{
    if (Which >= 4)
        PhpUnreadUnicodeDecoder(Decoder, Decoder->u.Utf8.CodeUnit4);
    if (Which >= 3)
        PhpUnreadUnicodeDecoder(Decoder, Decoder->u.Utf8.CodeUnit3);
    if (Which >= 2)
        PhpUnreadUnicodeDecoder(Decoder, Decoder->u.Utf8.CodeUnit2);

    *CodePoint = (ULONG)Decoder->u.Utf8.CodeUnit1 + 0xdc00;
    Decoder->State = 0;

    return TRUE;
}

_Use_decl_annotations_
BOOLEAN PhDecodeUnicodeDecoder(
    _Inout_ PPH_UNICODE_DECODER Decoder,
    _Out_ PULONG CodePoint
    )
{
    ULONG codeUnit;

    while (TRUE)
    {
        switch (Decoder->Encoding)
        {
        case PH_UNICODE_UTF8:
            if (!PhpReadUnicodeDecoder(Decoder, &codeUnit))
                return FALSE;

            switch (Decoder->State)
            {
            case 0:
                Decoder->u.Utf8.CodeUnit1 = (UCHAR)codeUnit;

                if (codeUnit < 0x80)
                {
                    *CodePoint = codeUnit;
                    return TRUE;
                }
                else if (codeUnit < 0xc2)
                {
                    return PhpDecodeUtf8Error(Decoder, CodePoint, 1);
                }
                else if (codeUnit < 0xe0)
                {
                    Decoder->State = 1; // 2 byte sequence
                    continue;
                }
                else if (codeUnit < 0xf0)
                {
                    Decoder->State = 2; // 3 byte sequence
                    continue;
                }
                else if (codeUnit < 0xf5)
                {
                    Decoder->State = 3; // 4 byte sequence
                    continue;
                }
                else
                {
                    return PhpDecodeUtf8Error(Decoder, CodePoint, 1);
                }

                break;
            case 1: // 2 byte sequence
                Decoder->u.Utf8.CodeUnit2 = (UCHAR)codeUnit;

                if ((codeUnit & 0xc0) == 0x80)
                {
                    *CodePoint = ((ULONG)Decoder->u.Utf8.CodeUnit1 << 6) + codeUnit - 0x3080;
                    Decoder->State = 0;
                    return TRUE;
                }
                else
                {
                    return PhpDecodeUtf8Error(Decoder, CodePoint, 2);
                }

                break;
            case 2: // 3 byte sequence (1)
                Decoder->u.Utf8.CodeUnit2 = (UCHAR)codeUnit;

                if (((codeUnit & 0xc0) == 0x80) &&
                    (Decoder->u.Utf8.CodeUnit1 != 0xe0 || codeUnit >= 0xa0))
                {
                    Decoder->State = 4; // 3 byte sequence (2)
                    continue;
                }
                else
                {
                    return PhpDecodeUtf8Error(Decoder, CodePoint, 2);
                }

                break;
            case 3: // 4 byte sequence (1)
                Decoder->u.Utf8.CodeUnit2 = (UCHAR)codeUnit;

                if (((codeUnit & 0xc0) == 0x80) &&
                    (Decoder->u.Utf8.CodeUnit1 != 0xf0 || codeUnit >= 0x90) &&
                    (Decoder->u.Utf8.CodeUnit1 != 0xf4 || codeUnit < 0x90))
                {
                    Decoder->State = 5; // 4 byte sequence (2)
                    continue;
                }
                else
                {
                    return PhpDecodeUtf8Error(Decoder, CodePoint, 2);
                }

                break;
            case 4: // 3 byte sequence (2)
                Decoder->u.Utf8.CodeUnit3 = (UCHAR)codeUnit;

                if ((codeUnit & 0xc0) == 0x80)
                {
                    *CodePoint =
                        ((ULONG)Decoder->u.Utf8.CodeUnit1 << 12) +
                        ((ULONG)Decoder->u.Utf8.CodeUnit2 << 6) +
                        codeUnit - 0xe2080;
                    Decoder->State = 0;
                    return TRUE;
                }
                else
                {
                    return PhpDecodeUtf8Error(Decoder, CodePoint, 3);
                }

                break;
            case 5: // 4 byte sequence (2)
                Decoder->u.Utf8.CodeUnit3 = (UCHAR)codeUnit;

                if ((codeUnit & 0xc0) == 0x80)
                {
                    Decoder->State = 6; // 4 byte sequence (3)
                    continue;
                }
                else
                {
                    return PhpDecodeUtf8Error(Decoder, CodePoint, 3);
                }

                break;
            case 6: // 4 byte sequence (3)
                Decoder->u.Utf8.CodeUnit4 = (UCHAR)codeUnit;

                if ((codeUnit & 0xc0) == 0x80)
                {
                    *CodePoint =
                        ((ULONG)Decoder->u.Utf8.CodeUnit1 << 18) +
                        ((ULONG)Decoder->u.Utf8.CodeUnit2 << 12) +
                        ((ULONG)Decoder->u.Utf8.CodeUnit3 << 6) +
                        codeUnit - 0x3c82080;
                    Decoder->State = 0;
                    return TRUE;
                }
                else
                {
                    return PhpDecodeUtf8Error(Decoder, CodePoint, 4);
                }

                break;
            }

            return FALSE;
        case PH_UNICODE_UTF16:
            if (!PhpReadUnicodeDecoder(Decoder, &codeUnit))
                return FALSE;

            switch (Decoder->State)
            {
            case 0:
                if (PH_UNICODE_UTF16_IS_HIGH_SURROGATE(codeUnit))
                {
                    Decoder->u.Utf16.CodeUnit = (USHORT)codeUnit;
                    Decoder->State = 1;
                    continue;
                }
                else
                {
                    *CodePoint = codeUnit;
                    return TRUE;
                }
                break;
            case 1:
                if (PH_UNICODE_UTF16_IS_LOW_SURROGATE(codeUnit))
                {
                    *CodePoint = PH_UNICODE_UTF16_TO_CODE_POINT(Decoder->u.Utf16.CodeUnit, codeUnit);
                    Decoder->State = 0;
                    return TRUE;
                }
                else
                {
                    *CodePoint = Decoder->u.Utf16.CodeUnit;
                    PhpUnreadUnicodeDecoder(Decoder, codeUnit);
                    Decoder->State = 0;
                    return TRUE;
                }
                break;
            }

            return FALSE;
        case PH_UNICODE_UTF32:
            if (PhpReadUnicodeDecoder(Decoder, CodePoint))
                return TRUE;
            return FALSE;
        default:
            return FALSE;
        }
    }
}

_Use_decl_annotations_
BOOLEAN PhEncodeUnicode(
    _In_ UCHAR Encoding,
    _In_ ULONG CodePoint,
    _Out_opt_ PVOID CodeUnits,
    _Out_ PULONG NumberOfCodeUnits
    )
{
    switch (Encoding)
    {
    case PH_UNICODE_UTF8:
        {
            PUCHAR codeUnits = CodeUnits;

            if (CodePoint < 0x80)
            {
                *NumberOfCodeUnits = 1;

                if (codeUnits)
                    codeUnits[0] = (UCHAR)CodePoint;
            }
            else if (CodePoint <= 0x7ff)
            {
                *NumberOfCodeUnits = 2;

                if (codeUnits)
                {
                    codeUnits[0] = (UCHAR)(CodePoint >> 6) + 0xc0;
                    codeUnits[1] = (UCHAR)(CodePoint & 0x3f) + 0x80;
                }
            }
            else if (CodePoint <= 0xffff)
            {
                *NumberOfCodeUnits = 3;

                if (codeUnits)
                {
                    codeUnits[0] = (UCHAR)(CodePoint >> 12) + 0xe0;
                    codeUnits[1] = (UCHAR)((CodePoint >> 6) & 0x3f) + 0x80;
                    codeUnits[2] = (UCHAR)(CodePoint & 0x3f) + 0x80;
                }
            }
            else if (CodePoint <= PH_UNICODE_MAX_CODE_POINT)
            {
                *NumberOfCodeUnits = 4;

                if (codeUnits)
                {
                    codeUnits[0] = (UCHAR)(CodePoint >> 18) + 0xf0;
                    codeUnits[1] = (UCHAR)((CodePoint >> 12) & 0x3f) + 0x80;
                    codeUnits[2] = (UCHAR)((CodePoint >> 6) & 0x3f) + 0x80;
                    codeUnits[3] = (UCHAR)(CodePoint & 0x3f) + 0x80;
                }
            }
            else
            {
                return FALSE;
            }
        }
        return TRUE;
    case PH_UNICODE_UTF16:
        {
            PUSHORT codeUnits = CodeUnits;

            if (CodePoint < 0x10000)
            {
                *NumberOfCodeUnits = 1;

                if (codeUnits)
                    codeUnits[0] = (USHORT)CodePoint;
            }
            else if (CodePoint <= PH_UNICODE_MAX_CODE_POINT)
            {
                *NumberOfCodeUnits = 2;

                if (codeUnits)
                {
                    codeUnits[0] = PH_UNICODE_UTF16_TO_HIGH_SURROGATE(CodePoint);
                    codeUnits[1] = PH_UNICODE_UTF16_TO_LOW_SURROGATE(CodePoint);
                }
            }
            else
            {
                return FALSE;
            }
        }
        return TRUE;
    case PH_UNICODE_UTF32:
        *NumberOfCodeUnits = 1;
        if (CodeUnits)
            *(PULONG)CodeUnits = CodePoint;
        return TRUE;
    default:
        return FALSE;
    }
}

/**
 * Converts an ASCII string to a UTF-16 string by zero-extending each byte.
 *
 * \param Input The original ASCII string.
 * \param InputLength The length of \a Input.
 * \param Output A buffer which will contain the converted string.
 */
VOID PhZeroExtendToUtf16Buffer(
    _In_reads_bytes_(InputLength) PCCH Input,
    _In_ SIZE_T InputLength,
    _Out_writes_bytes_(InputLength * sizeof(WCHAR)) PWCH Output
    )
{
    SIZE_T inputLength;

    inputLength = InputLength & -4;

    if (inputLength)
    {
        do
        {
            Output[0] = C_1uTo2(Input[0]);
            Output[1] = C_1uTo2(Input[1]);
            Output[2] = C_1uTo2(Input[2]);
            Output[3] = C_1uTo2(Input[3]);
            Input += 4;
            Output += 4;
            inputLength -= 4;
        } while (inputLength != 0);
    }

    switch (InputLength & 3)
    {
    case 3:
        *Output++ = C_1uTo2(*Input++);
    case 2:
        *Output++ = C_1uTo2(*Input++);
    case 1:
        *Output++ = C_1uTo2(*Input++);
    }
}

/**
 * Converts a zero-terminated ANSI string to a UTF-16 string, extending the input as needed.
 *
 * \param Input Pointer to the input ANSI string.
 * \param InputLength Length of the input string in bytes.
 * \return A pointer to a PPH_STRING containing the UTF-16 representation of the input string.
 */
PPH_STRING PhZeroExtendToUtf16Ex(
    _In_reads_bytes_(InputLength) PCCH Input,
    _In_ SIZE_T InputLength
    )
{
    PPH_STRING string;

    string = PhCreateStringEx(NULL, InputLength * sizeof(WCHAR));
    PhZeroExtendToUtf16Buffer(Input, InputLength, string->Buffer);

    return string;
}

/**
 * Converts a UTF-16 string to an ASCII byte array.
 *
 * \param Buffer Pointer to the UTF-16 string buffer to convert.
 * \param Length Length of the UTF-16 string, in characters.
 * \param Replacement Optional character to use when a UTF-16 character cannot be represented in ASCII.
 * \return Pointer to a PPH_BYTES structure containing the converted ASCII bytes.
 */
PPH_BYTES PhConvertUtf16ToAsciiEx(
    _In_ PCWCH Buffer,
    _In_ SIZE_T Length,
    _In_opt_ CHAR Replacement
    )
{
    PPH_BYTES bytes;
    PH_UNICODE_DECODER decoder;
    PWCH in;
    SIZE_T inRemaining;
    PCH out;
    SIZE_T outLength;
    ULONG codePoint;

    bytes = PhCreateBytesEx(NULL, Length / sizeof(WCHAR));
    PhInitializeUnicodeDecoder(&decoder, PH_UNICODE_UTF16);
    in = (PWCH)Buffer;
    inRemaining = Length / sizeof(WCHAR);
    out = bytes->Buffer;
    outLength = 0;

    while (inRemaining != 0)
    {
        PhWriteUnicodeDecoder(&decoder, (USHORT)*in);
        in++;
        inRemaining--;

        while (PhDecodeUnicodeDecoder(&decoder, &codePoint))
        {
            if (codePoint < 0x80)
            {
                *out++ = (CHAR)codePoint;
                outLength++;
            }
            else if (Replacement)
            {
                *out++ = Replacement;
                outLength++;
            }
        }
    }

    bytes->Length = outLength;
    bytes->Buffer[outLength] = ANSI_NULL;

    return bytes;
}

/**
 * Creates a string object from an existing null-terminated multi-byte string.
 *
 * \param Buffer A null-terminated multi-byte string.
 */
PPH_STRING PhConvertMultiByteToUtf16(
    _In_ PCSTR Buffer
    )
{
    return PhConvertMultiByteToUtf16Ex(
        Buffer,
        strlen(Buffer)
        );
}

/**
 * Creates a string object from an existing null-terminated multi-byte string.
 *
 * \param Buffer A null-terminated multi-byte string.
 * \param Length The number of bytes to use.
 */
PPH_STRING PhConvertMultiByteToUtf16Ex(
    _In_ PCSTR Buffer,
    _In_ SIZE_T Length
    )
{
    NTSTATUS status;
    PPH_STRING string;
    ULONG bufferLength;
    ULONG unicodeBytes;

    status = RtlSizeTToULong(Length, &bufferLength);

    if (!NT_SUCCESS(status))
        return NULL;

    status = RtlMultiByteToUnicodeSize(
        &unicodeBytes,
        Buffer,
        bufferLength
        );

    if (!NT_SUCCESS(status))
        return NULL;

    string = PhCreateStringEx(NULL, unicodeBytes);
    status = RtlMultiByteToUnicodeN(
        string->Buffer,
        (ULONG)string->Length,
        NULL,
        Buffer,
        bufferLength
        );

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(string);
        return NULL;
    }

    return string;
}

/**
 * Creates a multi-byte string from an existing null-terminated UTF-16 string.
 *
 * \param Buffer A null-terminated UTF-16 string.
 */
PPH_BYTES PhConvertUtf16ToMultiByte(
    _In_ PCWSTR Buffer
    )
{
    return PhConvertUtf16ToMultiByteEx(
        Buffer,
        PhCountStringZ(Buffer) * sizeof(WCHAR)
        );
}

/**
 * Creates a multi-byte string from an existing null-terminated UTF-16 string.
 *
 * \param Buffer A null-terminated UTF-16 string.
 * \param Length The number of bytes to use.
 */
PPH_BYTES PhConvertUtf16ToMultiByteEx(
    _In_ PCWCH Buffer,
    _In_ SIZE_T Length
    )
{
    NTSTATUS status;
    PPH_BYTES bytes;
    ULONG bufferLength;
    ULONG multiByteLength;

    status = RtlSizeTToULong(Length, &bufferLength);

    if (!NT_SUCCESS(status))
        return NULL;

    status = RtlUnicodeToMultiByteSize(
        &multiByteLength,
        Buffer,
        bufferLength
        );

    if (!NT_SUCCESS(status))
        return NULL;

    bytes = PhCreateBytesEx(NULL, multiByteLength);
    status = RtlUnicodeToMultiByteN(
        bytes->Buffer,
        (ULONG)bytes->Length,
        NULL,
        Buffer,
        bufferLength
        );

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(bytes);
        return NULL;
    }

    return bytes;
}

/**
 * Converts a UTF-8 encoded string to its UTF-16 equivalent and calculates the size in bytes of the resulting UTF-16 string.
 *
 * \param BytesInUtf16String Pointer to a variable that receives the size in bytes of the UTF-16 string.
 * \param Utf8String Pointer to the UTF-8 encoded input string.
 * \param BytesInUtf8String The size in bytes of the UTF-8 input string.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhConvertUtf8ToUtf16Size(
    _Out_ PSIZE_T BytesInUtf16String,
    _In_reads_bytes_(BytesInUtf8String) PCCH Utf8String,
    _In_ SIZE_T BytesInUtf8String
    )
{
#if defined(PH_NATIVE_STRING_CONVERSION)
    NTSTATUS status;
    ULONG bytesInUtf16String = 0;

    status = RtlUTF8ToUnicodeN(
        NULL,
        0,
        &bytesInUtf16String,
        Utf8String,
        (ULONG)BytesInUtf8String
        );

    if (NT_SUCCESS(status))
    {
        if (BytesInUtf16String)
        {
            *BytesInUtf16String = bytesInUtf16String;
        }
    }

    return status;
#else
    NTSTATUS status;
    PH_UNICODE_DECODER decoder;
    PCH in;
    SIZE_T inRemaining;
    SIZE_T bytesInUtf16String;
    ULONG codePoint;
    ULONG numberOfCodeUnits;

    status = STATUS_SUCCESS;
    PhInitializeUnicodeDecoder(&decoder, PH_UNICODE_UTF8);
    in = Utf8String;
    inRemaining = BytesInUtf8String;
    bytesInUtf16String = 0;

    while (inRemaining != 0)
    {
        PhWriteUnicodeDecoder(&decoder, (UCHAR)*in);
        in++;
        inRemaining--;

        while (PhDecodeUnicodeDecoder(&decoder, &codePoint))
        {
            if (PhEncodeUnicode(PH_UNICODE_UTF16, codePoint, NULL, &numberOfCodeUnits))
            {
                bytesInUtf16String += numberOfCodeUnits * sizeof(WCHAR);
            }
            else
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
        }

        if (!NT_SUCCESS(status))
            break;
    }

    *BytesInUtf16String = bytesInUtf16String;

    return status;
#endif
}

/**
 * Converts a UTF-8 encoded string to a UTF-16 encoded buffer.
 *
 * \param Utf16String Pointer to the buffer that receives the converted UTF-16 string.
 * \param MaxBytesInUtf16String The maximum number of bytes that can be written to Utf16String.
 * \param BytesInUtf16String Optional pointer that receives the number of bytes written to Utf16String.
 * \param Utf8String Pointer to the UTF-8 encoded input string.
 * \param BytesInUtf8String The number of bytes in the input UTF-8 string.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhConvertUtf8ToUtf16Buffer(
    _Out_writes_bytes_to_(MaxBytesInUtf16String, *BytesInUtf16String) PWCH Utf16String,
    _In_ SIZE_T MaxBytesInUtf16String,
    _Out_opt_ PSIZE_T BytesInUtf16String,
    _In_reads_bytes_(BytesInUtf8String) PCCH Utf8String,
    _In_ SIZE_T BytesInUtf8String
    )
{
#if defined(PH_NATIVE_STRING_CONVERSION)
    NTSTATUS status;
    ULONG bytesInUtf16String = 0;

    status = RtlUTF8ToUnicodeN(
        Utf16String,
        (ULONG)MaxBytesInUtf16String,
        &bytesInUtf16String,
        Utf8String,
        (ULONG)BytesInUtf8String
        );

    if (NT_SUCCESS(status))
    {
        if (BytesInUtf16String)
        {
            *BytesInUtf16String = bytesInUtf16String;
        }
    }

    return status;
#else
    NTSTATUS status;
    PH_UNICODE_DECODER decoder;
    PCH in;
    SIZE_T inRemaining;
    PWCH out;
    SIZE_T outRemaining;
    SIZE_T bytesInUtf16String;
    ULONG codePoint;
    USHORT codeUnits[2];
    ULONG numberOfCodeUnits;

    status = STATUS_SUCCESS;
    PhInitializeUnicodeDecoder(&decoder, PH_UNICODE_UTF8);
    in = Utf8String;
    inRemaining = BytesInUtf8String;
    out = Utf16String;
    outRemaining = MaxBytesInUtf16String / sizeof(WCHAR);
    bytesInUtf16String = 0;

    while (inRemaining != 0)
    {
        PhWriteUnicodeDecoder(&decoder, (UCHAR)*in);
        in++;
        inRemaining--;

        while (PhDecodeUnicodeDecoder(&decoder, &codePoint))
        {
            if (PhEncodeUnicode(PH_UNICODE_UTF16, codePoint, codeUnits, &numberOfCodeUnits))
            {
                bytesInUtf16String += numberOfCodeUnits * sizeof(WCHAR);

                if (outRemaining >= numberOfCodeUnits)
                {
                    *out++ = codeUnits[0];

                    if (numberOfCodeUnits >= 2)
                        *out++ = codeUnits[1];

                    outRemaining -= numberOfCodeUnits;
                }
                else
                {
                    status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }
            }
            else
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
        }

        if (!NT_SUCCESS(status))
            break;
    }

    if (BytesInUtf16String)
        *BytesInUtf16String = bytesInUtf16String;

    return status;
#endif
}

/**
 * Converts a UTF-8 encoded string to a UTF-16 encoded string.
 *
 * \param Buffer A pointer to a null-terminated UTF-8 encoded string.
 * \return A pointer to a PPH_STRING containing the converted UTF-16 string.
 * \remarks Returns NULL if the conversion fails.
 */
PPH_STRING PhConvertUtf8ToUtf16(
    _In_ PCSTR Buffer
    )
{
    return PhConvertUtf8ToUtf16Ex(
        Buffer,
        strlen(Buffer)
        );
}

/**
 * Converts a UTF-8 encoded string to a UTF-16 string.
 *
 * \param Buffer Pointer to the UTF-8 encoded input buffer.
 * \param Length Length of the input buffer in bytes.
 * \return A pointer to a PPH_STRING containing the converted UTF-16 string.
 * \remarks Returns NULL if the conversion fails.
 */
PPH_STRING PhConvertUtf8ToUtf16Ex(
    _In_ PCCH Buffer,
    _In_ SIZE_T Length
    )
{
    NTSTATUS status;
    PPH_STRING string;
    SIZE_T utf16Bytes;

    status = PhConvertUtf8ToUtf16Size(
        &utf16Bytes,
        Buffer,
        Length
        );

    if (!NT_SUCCESS(status))
        return NULL;

    string = PhCreateStringEx(NULL, utf16Bytes);
    status = PhConvertUtf8ToUtf16Buffer(
        string->Buffer,
        string->Length,
        NULL,
        Buffer,
        Length
        );

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(string);
        return NULL;
    }

    return string;
}

/**
 * Calculates the size in bytes required to store the UTF-8 encoded version of a given UTF-16 string.
 *
 * \param BytesInUtf8String Pointer to a variable that receives the required size in bytes for the UTF-8 string.
 * \param Utf16String Pointer to the UTF-16 string to be converted.
 * \param BytesInUtf16String The size in bytes of the input UTF-16 string.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhConvertUtf16ToUtf8Size(
    _Out_ PSIZE_T BytesInUtf8String,
    _In_reads_bytes_(BytesInUtf16String) PCWCH Utf16String,
    _In_ SIZE_T BytesInUtf16String
    )
{
#if defined(PH_NATIVE_STRING_CONVERSION)
    NTSTATUS status;
    ULONG bytesInUtf8String = 0;

    status = RtlUnicodeToUTF8N(
        NULL,
        0,
        &bytesInUtf8String,
        Utf16String,
        (ULONG)BytesInUtf16String
        );

    if (NT_SUCCESS(status))
    {
        if (BytesInUtf8String)
        {
            *BytesInUtf8String = bytesInUtf8String;
        }
    }

    return status;
#else
    NTSTATUS status;
    PH_UNICODE_DECODER decoder;
    PWCH in;
    SIZE_T inRemaining;
    SIZE_T bytesInUtf8String;
    ULONG codePoint;
    ULONG numberOfCodeUnits;

    status = STATUS_SUCCESS;
    PhInitializeUnicodeDecoder(&decoder, PH_UNICODE_UTF16);
    in = Utf16String;
    inRemaining = BytesInUtf16String / sizeof(WCHAR);
    bytesInUtf8String = 0;

    while (inRemaining != 0)
    {
        PhWriteUnicodeDecoder(&decoder, (USHORT)*in);
        in++;
        inRemaining--;

        while (PhDecodeUnicodeDecoder(&decoder, &codePoint))
        {
            if (PhEncodeUnicode(PH_UNICODE_UTF8, codePoint, NULL, &numberOfCodeUnits))
            {
                bytesInUtf8String += numberOfCodeUnits;
            }
            else
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
        }

        if (!NT_SUCCESS(status))
            break;
    }

    *BytesInUtf8String = bytesInUtf8String;

    return status;
#endif
}

/**
 * Converts a UTF-16 encoded string to a UTF-8 encoded buffer.
 *
 * \param Utf8String A pointer to the buffer that receives the UTF-8 encoded string. *
 * \param MaxBytesInUtf8String The maximum number of bytes that can be written to the Utf8String buffer.
 * \param BytesInUtf8String Optional pointer to a variable that receives the number of bytes written to Utf8String.
 * \param Utf16String A pointer to the UTF-16 encoded input string.
 * \param BytesInUtf16String The number of bytes in the UTF-16 encoded input string.
 * \return NTSTATUS Successful or errant status.
 */
NTSTATUS PhConvertUtf16ToUtf8Buffer(
    _Out_writes_bytes_to_(MaxBytesInUtf8String, *BytesInUtf8String) PCH Utf8String,
    _In_ SIZE_T MaxBytesInUtf8String,
    _Out_opt_ PSIZE_T BytesInUtf8String,
    _In_reads_bytes_(BytesInUtf16String) PCWCH Utf16String,
    _In_ SIZE_T BytesInUtf16String
    )
{
#if defined(PH_NATIVE_STRING_CONVERSION)
    NTSTATUS status;
    ULONG bytesInUtf8String = 0;

    status = RtlUnicodeToUTF8N(
        Utf8String,
        (ULONG)MaxBytesInUtf8String,
        &bytesInUtf8String,
        Utf16String,
        (ULONG)BytesInUtf16String
        );

    if (NT_SUCCESS(status))
    {
        if (BytesInUtf8String)
        {
            *BytesInUtf8String = bytesInUtf8String;
        }
    }

    return status;
#else
    NTSTATUS status;
    PH_UNICODE_DECODER decoder;
    PWCH in;
    SIZE_T inRemaining;
    PCH out;
    SIZE_T outRemaining;
    SIZE_T bytesInUtf8String;
    ULONG codePoint;
    UCHAR codeUnits[4];
    ULONG numberOfCodeUnits;

    PhInitializeUnicodeDecoder(&decoder, PH_UNICODE_UTF16);
    in = Utf16String;
    inRemaining = BytesInUtf16String / sizeof(WCHAR);
    out = Utf8String;
    outRemaining = MaxBytesInUtf8String;
    bytesInUtf8String = 0;

    while (inRemaining != 0)
    {
        PhWriteUnicodeDecoder(&decoder, (USHORT)*in);
        in++;
        inRemaining--;

        while (PhDecodeUnicodeDecoder(&decoder, &codePoint))
        {
            if (PhEncodeUnicode(PH_UNICODE_UTF8, codePoint, codeUnits, &numberOfCodeUnits))
            {
                bytesInUtf8String += numberOfCodeUnits;

                if (outRemaining >= numberOfCodeUnits)
                {
                    *out++ = codeUnits[0];

                    if (numberOfCodeUnits >= 2)
                        *out++ = codeUnits[1];
                    if (numberOfCodeUnits >= 3)
                        *out++ = codeUnits[2];
                    if (numberOfCodeUnits >= 4)
                        *out++ = codeUnits[3];

                    outRemaining -= numberOfCodeUnits;
                }
                else
                {
                    status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }
            }
            else
            {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }
        }

        if (!NT_SUCCESS(status))
            break;
    }

    if (BytesInUtf8String)
    {
        *BytesInUtf8String = bytesInUtf8String;
    }

    return status;
#endif
}

/**
 * Converts a UTF-16 encoded wide string to a UTF-8 encoded byte array.
 *
 * \param Buffer Pointer to a null-terminated UTF-16 string (PCWSTR) to be converted.
 * \return A pointer to a PPH_BYTES structure containing the UTF-8 encoded result.
 * \remarks Returns NULL if the conversion fails.
 */
PPH_BYTES PhConvertUtf16ToUtf8(
    _In_ PCWSTR Buffer
    )
{
    return PhConvertUtf16ToUtf8Ex(
        Buffer,
        PhCountStringZ(Buffer) * sizeof(WCHAR)
        );
}

PPH_BYTES PhConvertUtf16ToUtf8Ex(
    _In_ PCWCH Buffer,
    _In_ SIZE_T Length
    )
{
    NTSTATUS status;
    PPH_BYTES bytes;
    SIZE_T utf8Bytes;

    status = PhConvertUtf16ToUtf8Size(
        &utf8Bytes,
        Buffer,
        Length
        );

    if (!NT_SUCCESS(status))
        return NULL;

    bytes = PhCreateBytesEx(NULL, utf8Bytes);
    status = PhConvertUtf16ToUtf8Buffer(
        bytes->Buffer,
        bytes->Length,
        NULL,
        Buffer,
        Length
        );

    if (!NT_SUCCESS(status))
    {
        PhDereferenceObject(bytes);
        return NULL;
    }

    return bytes;
}

/**
 * Initializes a string builder object.
 *
 * \param StringBuilder A string builder object.
 * \param InitialCapacity The number of bytes to allocate initially.
 */
VOID PhInitializeStringBuilder(
    _Out_ PPH_STRING_BUILDER StringBuilder,
    _In_ SIZE_T InitialCapacity
    )
{
    // Make sure the initial capacity is even, as required for all string objects.
    if (InitialCapacity & 1)
        InitialCapacity++;

    StringBuilder->AllocatedLength = InitialCapacity;

    // Allocate a PH_STRING for the string builder.
    // We will dereference it and allocate a new one when we need to resize the string.

    StringBuilder->String = PhCreateStringEx(NULL, StringBuilder->AllocatedLength);

    // We will keep modifying the Length field of the string so that:
    // 1. We know how much of the string is used, and
    // 2. The user can simply get a reference to the string and use it as-is.

    StringBuilder->String->Length = 0;

    // Write the null terminator.
    StringBuilder->String->Buffer[0] = UNICODE_NULL;

    PHLIB_INC_STATISTIC(BaseStringBuildersCreated);
}

/**
 * Frees resources used by a string builder object.
 *
 * \param StringBuilder A string builder object.
 */
VOID PhDeleteStringBuilder(
    _Inout_ PPH_STRING_BUILDER StringBuilder
    )
{
    PhDereferenceObject(StringBuilder->String);
}

/**
 * Obtains a reference to the string constructed by a string builder object and frees resources used
 * by the object.
 *
 * \param StringBuilder A string builder object.
 *
 * \return A pointer to a string. You must free the string using PhDereferenceObject() when you no
 * longer need it.
 */
PPH_STRING PhFinalStringBuilderString(
    _Inout_ PPH_STRING_BUILDER StringBuilder
    )
{
    return StringBuilder->String;
}

VOID PhpResizeStringBuilder(
    _In_ PPH_STRING_BUILDER StringBuilder,
    _In_ SIZE_T NewCapacity
    )
{
    PPH_STRING newString;

    // Double the string size. If that still isn't enough room, just use the new length.

    StringBuilder->AllocatedLength *= 2;

    if (StringBuilder->AllocatedLength < NewCapacity)
        StringBuilder->AllocatedLength = NewCapacity;

    // Allocate a new string.
    newString = PhCreateStringEx(NULL, StringBuilder->AllocatedLength);

    // Copy the old string to the new string.
    memcpy(
        newString->Buffer,
        StringBuilder->String->Buffer,
        StringBuilder->String->Length + sizeof(UNICODE_NULL) // Include null terminator
        );

    // Copy the old string length.
    newString->Length = StringBuilder->String->Length;

    // Dereference the old string and replace it with the new string.
    PhMoveReference(&StringBuilder->String, newString);

    PHLIB_INC_STATISTIC(BaseStringBuildersResized);
}

FORCEINLINE VOID PhpWriteNullTerminatorStringBuilder(
    _In_ PPH_STRING_BUILDER StringBuilder
    )
{
    assert(!(StringBuilder->String->Length & 1));
    *(PWCHAR)PTR_ADD_OFFSET(StringBuilder->String->Buffer, StringBuilder->String->Length) = UNICODE_NULL;
}

/**
 * Appends a string to the end of a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param String The string to append. Specify NULL to simply reserve \a Length bytes.
 * \param Length The number of bytes to append.
 */
VOID PhAppendStringBuilderEx(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_opt_ PWCHAR String,
    _In_ SIZE_T Length
    )
{
    if (Length == 0)
        return;

    // See if we need to re-allocate the string.
    if (StringBuilder->AllocatedLength < StringBuilder->String->Length + Length)
    {
        PhpResizeStringBuilder(StringBuilder, StringBuilder->String->Length + Length);
    }

    // Copy the string, add the length, then write the null terminator.

    if (String)
    {
        memcpy(
            PTR_ADD_OFFSET(StringBuilder->String->Buffer, StringBuilder->String->Length),
            String,
            Length
            );
    }

    StringBuilder->String->Length += Length;
    PhpWriteNullTerminatorStringBuilder(StringBuilder);
}

/**
 * Appends a character to the end of a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param Character The character to append.
 */
VOID PhAppendCharStringBuilder(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ WCHAR Character
    )
{
    if (StringBuilder->AllocatedLength < StringBuilder->String->Length + sizeof(WCHAR))
    {
        PhpResizeStringBuilder(StringBuilder, StringBuilder->String->Length + sizeof(WCHAR));
    }

    *(PWCHAR)PTR_ADD_OFFSET(StringBuilder->String->Buffer, StringBuilder->String->Length) = Character;
    StringBuilder->String->Length += sizeof(WCHAR);
    PhpWriteNullTerminatorStringBuilder(StringBuilder);
}

/**
 * Appends a number of characters to the end of a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param Character The character to append.
 * \param Count The number of times to append the character.
 */
VOID PhAppendCharStringBuilder2(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ WCHAR Character,
    _In_ SIZE_T Count
    )
{
    if (Count == 0)
        return;

    // See if we need to re-allocate the string.
    if (StringBuilder->AllocatedLength < StringBuilder->String->Length + Count * sizeof(WCHAR))
    {
        PhpResizeStringBuilder(StringBuilder, StringBuilder->String->Length + Count * sizeof(WCHAR));
    }

    wmemset(
        PTR_ADD_OFFSET(StringBuilder->String->Buffer, StringBuilder->String->Length),
        Character,
        Count
        );

    StringBuilder->String->Length += Count * sizeof(WCHAR);
    PhpWriteNullTerminatorStringBuilder(StringBuilder);
}

/**
 * Appends a formatted string to the end of a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param Format The format-control string.
 * \param ... The list of strings.
 */
VOID PhAppendFormatStringBuilder(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ _Printf_format_string_ PCWSTR Format,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Format);
    PhAppendFormatStringBuilder_V(StringBuilder, Format, argptr);
    va_end(argptr);
}

/**
 * Appends a formatted string to the specified string builder using a variable argument list.
 *
 * \param StringBuilder A pointer to the string builder to which the formatted string will be appended.
 * \param Format A printf-style format string that specifies how to format the arguments.
 * \param ArgPtr A va_list containing the arguments to format according to the format string.
 */
VOID PhAppendFormatStringBuilder_V(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ _Printf_format_string_ PCWSTR Format,
    _In_ va_list ArgPtr
    )
{
    LONG length;
    SIZE_T lengthInBytes;

    length = _vscwprintf(Format, ArgPtr);

    if (length == INT_ERROR || length == 0)
        return;

    lengthInBytes = length * sizeof(WCHAR);

    if (StringBuilder->AllocatedLength < StringBuilder->String->Length + lengthInBytes)
        PhpResizeStringBuilder(StringBuilder, StringBuilder->String->Length + lengthInBytes);

    _vsnwprintf(
        PTR_ADD_OFFSET(StringBuilder->String->Buffer, StringBuilder->String->Length),
        length,
        Format,
        ArgPtr
        );

    StringBuilder->String->Length += lengthInBytes;
    PhpWriteNullTerminatorStringBuilder(StringBuilder);
}

/**
 * Inserts a string into a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param Index The index, in characters, at which to insert the string.
 * \param String The string to insert.
 */
VOID PhInsertStringBuilder(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ SIZE_T Index,
    _In_ PCPH_STRINGREF String
    )
{
    PhInsertStringBuilderEx(
        StringBuilder,
        Index,
        String->Buffer,
        String->Length
        );
}

/**
 * Inserts a string into a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param Index The index, in characters, at which to insert the string.
 * \param String The string to insert.
 */
VOID PhInsertStringBuilder2(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ SIZE_T Index,
    _In_ PCWSTR String
    )
{
    PhInsertStringBuilderEx(
        StringBuilder,
        Index,
        String,
        PhCountStringZ(String) * sizeof(WCHAR)
        );
}

/**
 * Inserts a string into a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param Index The index, in characters, at which to insert the string.
 * \param String The string to insert. Specify NULL to simply reserve \a Length bytes at \a Index.
 * \param Length The number of bytes to insert.
 */
VOID PhInsertStringBuilderEx(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ SIZE_T Index,
    _In_opt_ PCWCHAR String,
    _In_ SIZE_T Length
    )
{
    if (Length == 0)
        return;

    // See if we need to re-allocate the string.
    if (StringBuilder->AllocatedLength < StringBuilder->String->Length + Length)
    {
        PhpResizeStringBuilder(StringBuilder, StringBuilder->String->Length + Length);
    }

    if (Index * sizeof(WCHAR) < StringBuilder->String->Length)
    {
        // Create some space for the string.
        memmove(
            &StringBuilder->String->Buffer[Index + Length / sizeof(WCHAR)],
            &StringBuilder->String->Buffer[Index],
            StringBuilder->String->Length - Index * sizeof(WCHAR)
            );
    }

    if (String)
    {
        // Copy the new string.
        memcpy(
            &StringBuilder->String->Buffer[Index],
            String,
            Length
            );
    }

    StringBuilder->String->Length += Length;
    PhpWriteNullTerminatorStringBuilder(StringBuilder);
}

/**
 * Removes characters from a string builder string.
 *
 * \param StringBuilder A string builder object.
 * \param StartIndex The index, in characters, at which to begin removing characters.
 * \param Count The number of characters to remove.
 */
VOID PhRemoveStringBuilder(
    _Inout_ PPH_STRING_BUILDER StringBuilder,
    _In_ SIZE_T StartIndex,
    _In_ SIZE_T Count
    )
{
    // Overwrite the removed part with the part
    // behind it.

    memmove(
        &StringBuilder->String->Buffer[StartIndex],
        &StringBuilder->String->Buffer[StartIndex + Count],
        StringBuilder->String->Length - (Count + StartIndex) * sizeof(WCHAR)
        );
    StringBuilder->String->Length -= Count * sizeof(WCHAR);
    PhpWriteNullTerminatorStringBuilder(StringBuilder);
}

/**
 * Initializes a byte string builder object.
 *
 * \param BytesBuilder A byte string builder object.
 * \param InitialCapacity The number of bytes to allocate initially.
 */
VOID PhInitializeBytesBuilder(
    _Out_ PPH_BYTES_BUILDER BytesBuilder,
    _In_ SIZE_T InitialCapacity
    )
{
    BytesBuilder->AllocatedLength = InitialCapacity;
    BytesBuilder->Bytes = PhCreateBytesEx(NULL, BytesBuilder->AllocatedLength);
    BytesBuilder->Bytes->Length = 0;
    BytesBuilder->Bytes->Buffer[0] = ANSI_NULL;
}

/**
 * Frees resources used by a byte string builder object.
 *
 * \param BytesBuilder A byte string builder object.
 */
VOID PhDeleteBytesBuilder(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder
    )
{
    PhDereferenceObject(BytesBuilder->Bytes);
}

/**
 * Obtains a reference to the byte string constructed by a byte string builder object and frees
 * resources used by the object.
 *
 * \param BytesBuilder A byte string builder object.
 * \return A pointer to a byte string. You must free the byte string using PhDereferenceObject()
 * when you no longer need it.
 */
PPH_BYTES PhFinalBytesBuilderBytes(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder
    )
{
    return BytesBuilder->Bytes;
}

VOID PhpResizeBytesBuilder(
    _In_ PPH_BYTES_BUILDER BytesBuilder,
    _In_ SIZE_T NewCapacity
    )
{
    PPH_BYTES newBytes;

    // Double the byte string size. If that still isn't enough room, just use the new length.

    BytesBuilder->AllocatedLength *= 2;

    if (BytesBuilder->AllocatedLength < NewCapacity)
        BytesBuilder->AllocatedLength = NewCapacity;

    // Allocate a new byte string.
    newBytes = PhCreateBytesEx(NULL, BytesBuilder->AllocatedLength);

    // Copy the old byte string to the new byte string.
    memcpy(
        newBytes->Buffer,
        BytesBuilder->Bytes->Buffer,
        BytesBuilder->Bytes->Length + sizeof(ANSI_NULL) // Include null terminator
        );

    // Copy the old byte string length.
    newBytes->Length = BytesBuilder->Bytes->Length;

    // Dereference the old byte string and replace it with the new byte string.
    PhMoveReference(&BytesBuilder->Bytes, newBytes);
}

FORCEINLINE VOID PhpWriteNullTerminatorBytesBuilder(
    _In_ PPH_BYTES_BUILDER BytesBuilder
    )
{
    BytesBuilder->Bytes->Buffer[BytesBuilder->Bytes->Length] = ANSI_NULL;
}

/**
 * Appends a byte string to the end of a byte string builder string.
 *
 * \param BytesBuilder A byte string builder object.
 * \param Bytes The byte string to append.
 */
VOID PhAppendBytesBuilder(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder,
    _In_ PPH_BYTESREF Bytes
    )
{
    PhAppendBytesBuilderEx(
        BytesBuilder,
        Bytes->Buffer,
        Bytes->Length,
        0,
        NULL
        );
}

/**
 * Appends a byte string to the end of a byte string builder string.
 *
 * \param BytesBuilder A byte string builder object.
 * \param Buffer The byte string to append. Specify NULL to simply reserve \a Length bytes.
 * \param Length The number of bytes to append.
 * \param Alignment The required alignment. This should not be greater than 8.
 * \param Offset A variable which receives the byte offset of the appended or reserved bytes in the
 * byte string builder string.
 *
 * \return A pointer to the appended or reserved bytes.
 */
PVOID PhAppendBytesBuilderEx(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder,
    _In_opt_ PVOID Buffer,
    _In_ SIZE_T Length,
    _In_opt_ SIZE_T Alignment,
    _Out_opt_ PSIZE_T Offset
    )
{
    SIZE_T currentLength;

    currentLength = BytesBuilder->Bytes->Length;

    if (Length == 0)
        goto Done;

    if (Alignment)
        currentLength = ALIGN_UP_BY(currentLength, Alignment);

    // See if we need to re-allocate the byte string.
    if (BytesBuilder->AllocatedLength < currentLength + Length)
        PhpResizeBytesBuilder(BytesBuilder, currentLength + Length);

    // Copy the byte string, add the length, then write the null terminator.

    if (Buffer)
        memcpy(BytesBuilder->Bytes->Buffer + currentLength, Buffer, Length);

    BytesBuilder->Bytes->Length = currentLength + Length;
    PhpWriteNullTerminatorBytesBuilder(BytesBuilder);

Done:
    if (Offset)
        *Offset = currentLength;

    return BytesBuilder->Bytes->Buffer + currentLength;
}

/**
 * Appends formatted data to a bytes builder using a variable argument list.
 *
 * \param BytesBuilder Pointer to the bytes builder structure to which the formatted data will be appended.
 * \param Format A printf-style format string specifying how to format the data.
 * \param ArgPtr A variable argument list containing the values to format according to the Format string.
 */
VOID PhAppendFormatBytesBuilder_V(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder,
    _In_ _Printf_format_string_ PCSTR Format,
    _In_ va_list ArgPtr
    )
{
    LONG length;
    SIZE_T lengthInBytes;

    length = _vscprintf(Format, ArgPtr);

    if (length == INT_ERROR || length == 0)
        return;

    lengthInBytes = length * sizeof(CHAR);

    if (BytesBuilder->AllocatedLength < BytesBuilder->Bytes->Length + lengthInBytes)
        PhpResizeBytesBuilder(BytesBuilder, BytesBuilder->Bytes->Length + lengthInBytes);

    _vsnprintf(
        PTR_ADD_OFFSET(BytesBuilder->Bytes->Buffer, BytesBuilder->Bytes->Length),
        length,
        Format,
        ArgPtr
        );

    BytesBuilder->Bytes->Length += lengthInBytes;
    PhpWriteNullTerminatorBytesBuilder(BytesBuilder);
}

/**
 * Appends formatted data to a bytes builder.
 *
 * This function formats a string using the specified format and arguments,
 * then appends the resulting bytes to the provided bytes builder.
 *
 * \param BytesBuilder A pointer to the bytes builder structure to which the formatted bytes will be appended.
 * \param Format A printf-style format string specifying how to format the data.
 * \param ... Additional arguments to be formatted according to the format string.
 */
VOID PhAppendFormatBytesBuilder(
    _Inout_ PPH_BYTES_BUILDER BytesBuilder,
    _In_ _Printf_format_string_ PCSTR Format,
    ...
    )
{
    va_list argptr;

    va_start(argptr, Format);
    PhAppendFormatBytesBuilder_V(BytesBuilder, Format, argptr);
    va_end(argptr);
}

/**
 * Generates a hash code for a string.
 *
 * \param String The string to hash.
 * \param IgnoreCase TRUE for a case-insensitive hash function, otherwise FALSE.
 */
ULONG PhHashStringRefOriginal(
    _In_ PCPH_STRINGREF String,
    _In_ BOOLEAN IgnoreCase
    )
{
    ULONG hash = 0;
    SIZE_T count;
    PWCHAR p;

    if (String->Length == 0)
        return 0;

    count = String->Length / sizeof(WCHAR);
    p = String->Buffer;

    if (IgnoreCase)
    {
        while (count-- != 0)
        {
            hash ^= (USHORT)PhUpcaseUnicodeChar(*p++);
            hash *= 0x01000193;
        }
    }
    else
    {
        return PhHashBytes((PUCHAR)String->Buffer, String->Length);
    }

    return hash;
}

/**
 * Generates a hash code for a string.
 *
 * \param String The string to hash.
 * \param IgnoreCase TRUE for a case-insensitive hash function, otherwise FALSE.
 */
ULONG PhHashStringRef(
    _In_ PCPH_STRINGREF String,
    _In_ BOOLEAN IgnoreCase
    )
{
    ULONG hash = 0;
    SIZE_T count;
    PWCHAR p;

    if (String->Length == 0)
        return 0;

    count = String->Length / sizeof(WCHAR);
    p = String->Buffer;

    if (IgnoreCase)
    {
#ifndef _ARM64_
        if (PhHasAVX && count >= 16)
        {
            // AVX hash path for case-insensitive
            SIZE_T length32 = count / 16;
            WCHAR uppercased[16];

            for (SIZE_T i = 0; i < length32; i++)
            {
                __m256i chunk;

                chunk = _mm256_loadu_si256((__m256i const*)p);
                chunk = PhUppercaseLatin1INT256by16(chunk);
                _mm256_storeu_si256((__m256i*)uppercased, chunk);

                // Hash 16 uppercased characters
                for (ULONG j = 0; j < 16; j++)
                {
                    hash ^= (USHORT)uppercased[j];
                    hash *= 0x01000193;
                }

                p += 16;
            }

            _mm256_zeroupper();
            count %= 16;
        }
#endif

        if (PhHasIntrinsics && count >= 8)
        {
            // SIMD hash path for case-insensitive
            SIZE_T length16 = count / 8;
            WCHAR uppercased[8];

            for (SIZE_T i = 0; i < length16; i++)
            {
                PH_INT128 chunk;

                chunk = PhLoadINT128U((PLONG)p);
                chunk = PhUppercaseLatin1INT128by16(chunk);
                PhStoreINT128U((PLONG)uppercased, chunk);

                // Hash 8 uppercased characters
                for (ULONG j = 0; j < 8; j++)
                {
                    hash ^= (USHORT)uppercased[j];
                    hash *= 0x01000193;
                }

                p += 8;
            }

            count %= 8;
        }

        while (count-- != 0)
        {
            hash ^= (USHORT)PhUpcaseUnicodeChar(*p++);
            hash *= 0x01000193;
        }
    }
    else
    {
        return PhHashBytes((PUCHAR)String->Buffer, String->Length);
    }

    return hash;
}

/**
 * Computes a hash value for the specified string reference using the given hash algorithm.
 *
 * \param String Pointer to a PH_STRINGREF structure representing the string to hash.
 * \param IgnoreCase If TRUE, the hash computation ignores case differences; otherwise, case is considered.
 * \param HashAlgorithm The hash algorithm to use for computing the hash value.
 * \return The computed hash value as an unsigned long.
 */
ULONG PhHashStringRefEx(
    _In_ PCPH_STRINGREF String,
    _In_ BOOLEAN IgnoreCase,
    _In_ PH_STRING_HASH HashAlgorithm
    )
{
    switch (HashAlgorithm)
    {
    case PH_STRING_HASH_DEFAULT:
    case PH_STRING_HASH_FNV1A:
        return PhHashStringRef(String, IgnoreCase);
    case PH_STRING_HASH_X65599:
        {
            ULONG hash = 0;
            PWCHAR end;
            PWCHAR p;

            if (String->Length == 0)
                return 0;

            end = String->Buffer + (String->Length / sizeof(WCHAR));
            p = String->Buffer;

            if (IgnoreCase)
            {
#ifndef _ARM64_
                if (PhHasAVX && (end - p) >= 16)
                {
                    // AVX hash path for case-insensitive
                    SIZE_T count = (end - p) / 16;
                    WCHAR uppercased[16];

                    for (SIZE_T i = 0; i < count; i++)
                    {
                        __m256i chunk;

                        chunk = _mm256_loadu_si256((__m256i const*)p);
                        chunk = PhUppercaseLatin1INT256by16(chunk);
                        _mm256_storeu_si256((__m256i*)uppercased, chunk);

                        // Hash 16 uppercased characters
                        for (ULONG j = 0; j < 16; j++)
                        {
                            hash = ((65599 * (hash)) + (ULONG)uppercased[j]);
                        }

                        p += 16;
                    }

                    _mm256_zeroupper();
                }
#endif
                for (; p != end; p++)
                {
                    hash = ((65599 * (hash)) + (ULONG)(((*p) >= L'a' && (*p) <= L'z') ? (*p) - L'a' + L'A' : (*p)));
                }

                // Medium fast
                //UNICODE_STRING unicodeString;
                //
                //if (!PhStringRefToUnicodeString(String, &unicodeString))
                //    return 0;
                //
                //if (!NT_SUCCESS(RtlHashUnicodeString(&unicodeString, TRUE, HASH_STRING_ALGORITHM_X65599, &hash)))
                //    return 0;
                //
                // Slower than the above two (based on PhHashBytes) (dmex)
                //SIZE_T count = String->Length / sizeof(WCHAR);
                //PWCHAR p = String->Buffer;
                //do
                //{
                //    hash += (USHORT)PhUpcaseUnicodeChar(*p++); // __ascii_towupper(*p++);
                //    hash *= 0x1003F;
                //} while (--count != 0);
            }
            else
            {
#ifndef _ARM64_
                if (PhHasAVX && (end - p) >= 16)
                {
                    // AVX hash path for case-sensitive
                    SIZE_T count = (end - p) / 16;
                    WCHAR buffer[16];

                    for (SIZE_T i = 0; i < count; i++)
                    {
                        __m256i chunk;

                        chunk = _mm256_loadu_si256((__m256i const*)p);
                        _mm256_storeu_si256((__m256i*)buffer, chunk);

                        // Hash 16 characters
                        for (ULONG j = 0; j < 16; j++)
                        {
                            hash = ((65599 * (hash)) + (ULONG)buffer[j]);
                        }

                        p += 16;
                    }

                    _mm256_zeroupper();
                }
#endif
                for (; p != end; p++)
                {
                    hash = ((65599 * (hash)) + (ULONG)(*p));
                }
            }

            return hash;
        }
    case PH_STRING_HASH_XXH32:
        {
            if (String->Length == 0)
                return 0;

            return PhHashXXH32(String->Buffer, String->Length, 0);
        }
    }

    return 0;
}

/**
 * Converts a sequence of hexadecimal digits into a byte array.
 *
 * \param String A string containing hexadecimal digits to convert. The string must have an even
 * number of digits, because each pair of hexadecimal digits represents one byte. Example:
 * "129a2eff5c0b".
 * \param Buffer The output buffer.
 * \return TRUE if the string was successfully converted, otherwise FALSE.
 */
BOOLEAN PhHexStringToBuffer(
    _In_ PCPH_STRINGREF String,
    _Out_writes_bytes_(String->Length / sizeof(WCHAR) / 2) PUCHAR Buffer
    )
{
    SIZE_T i;
    SIZE_T length;

    // The string must have an even length.
    if ((String->Length / sizeof(WCHAR)) & 1)
        return FALSE;

    length = String->Length / sizeof(WCHAR) / 2;

    for (i = 0; i < length; i++)
    {
        Buffer[i] =
            (UCHAR)(PhCharToInteger[(UCHAR)String->Buffer[i * sizeof(WCHAR)]] << 4) +
            (UCHAR)PhCharToInteger[(UCHAR)String->Buffer[i * sizeof(WCHAR) + 1]];
    }

    return TRUE;
}

/**
 * Converts a hexadecimal string to a binary buffer.
 *
 * \param String Pointer to a PH_STRINGREF structure containing the hexadecimal string to convert.
 * \param BufferLength The length, in bytes, of the output buffer.
 * \param Buffer Pointer to the buffer that receives the converted binary data. Must be at least BufferLength bytes.
 * \return TRUE if the conversion was successful; FALSE otherwise.
 */
BOOLEAN PhHexStringToBufferEx(
    _In_ PCPH_STRINGREF String,
    _In_ SIZE_T BufferLength,
    _Out_writes_bytes_(BufferLength) PVOID Buffer
    )
{
    SIZE_T length;
    SIZE_T i;

    if ((String->Length / sizeof(WCHAR)) & 1)
        return FALSE;

    length = String->Length / sizeof(WCHAR) / 2;

    if (length > BufferLength)
        return FALSE;

    for (i = 0; i < length; i++)
    {
        ((PBYTE)Buffer)[i] =
            (BYTE)(PhCharToInteger[(BYTE)String->Buffer[i * sizeof(WCHAR)]] << 4) +
            (BYTE)PhCharToInteger[(BYTE)String->Buffer[i * sizeof(WCHAR) + 1]];
    }

    return TRUE;
}

/**
 * Converts a byte array into a sequence of hexadecimal digits.
 *
 * \param Buffer The input buffer.
 * \param Length The number of bytes to convert.
 * \return A string containing a sequence of hexadecimal digits.
 */
PPH_STRING PhBufferToHexString(
    _In_reads_bytes_(Length) PUCHAR Buffer,
    _In_ SIZE_T Length
    )
{
    return PhBufferToHexStringEx(Buffer, Length, FALSE);
}

/**
 * Converts a byte array into a sequence of hexadecimal digits.
 *
 * \param Buffer The input buffer.
 * \param Length The number of bytes to convert.
 * \param UpperCase TRUE to use uppercase characters, otherwise FALSE.
 * \return A string containing a sequence of hexadecimal digits.
 */
PPH_STRING PhBufferToHexStringEx(
    _In_reads_bytes_(Length) PUCHAR Buffer,
    _In_ SIZE_T Length,
    _In_ BOOLEAN UpperCase
    )
{
    PPH_STRING string;
    PCCH table;
    SIZE_T i;

    if (UpperCase)
        table = PhIntegerToCharUpper;
    else
        table = PhIntegerToChar;

    string = PhCreateStringEx(NULL, Length * sizeof(WCHAR) * 2);

    for (i = 0; i < Length; i++)
    {
        string->Buffer[i * sizeof(WCHAR)] = table[Buffer[i] >> 4];
        string->Buffer[i * sizeof(WCHAR) + 1] = table[Buffer[i] & 0xf];
    }

    return string;
}

/**
 * Converts a binary buffer to a hexadecimal string representation.
 *
 * \param InputBuffer Pointer to the input buffer containing binary data.
 * \param InputLength Length of the input buffer, in bytes.
 * \param UpperCase If TRUE, output hex digits in uppercase; otherwise, lowercase.
 * \param OutputBuffer Pointer to the buffer that receives the hexadecimal string.
 * \param OutputLength Size of the output buffer, in bytes.
 * \param ReturnLength Optional pointer to receive the number of bytes written to OutputBuffer.
 * \return TRUE if the conversion was successful and the output buffer was large enough; FALSE otherwise.
 */
_Use_decl_annotations_
BOOLEAN PhBufferToHexStringBuffer(
    _In_reads_bytes_(InputLength) PUCHAR InputBuffer,
    _In_ SIZE_T InputLength,
    _In_ BOOLEAN UpperCase,
    _Out_writes_bytes_to_(OutputLength, *ReturnLength) PWSTR OutputBuffer,
    _In_ SIZE_T OutputLength,
    _Out_opt_ PSIZE_T ReturnLength
    )
{
    PCCH table;
    ULONG i;

    if (OutputLength < InputLength * sizeof(WCHAR) * 2)
    {
        if (ReturnLength)
            *ReturnLength = InputLength * sizeof(WCHAR) * 2;
        return FALSE;
    }

    if (UpperCase)
        table = PhIntegerToCharUpper;
    else
        table = PhIntegerToChar;

    for (i = 0; i < InputLength; i++)
    {
        OutputBuffer[i * sizeof(WCHAR)] = table[InputBuffer[i] >> 4];
        OutputBuffer[i * sizeof(WCHAR) + 1] = table[InputBuffer[i] & 0xf];
    }

    OutputBuffer[i * sizeof(WCHAR)] = UNICODE_NULL;

    if (ReturnLength)
        *ReturnLength = i * sizeof(WCHAR) * 2;

    return TRUE;
}

/**
 * Converts a string to an integer.
 *
 * \param String The string to process.
 * \param Base The base which the string uses to represent the integer. The maximum value is 69.
 * \param Integer The resulting integer.
 */
BOOLEAN PhpStringToInteger64(
    _In_ PCPH_STRINGREF String,
    _In_ ULONG Base,
    _Out_opt_ PULONG64 Integer
    )
{
    BOOLEAN valid = TRUE;
    ULONG64 result;
    SIZE_T length;
    SIZE_T i;

    length = String->Length / sizeof(WCHAR);
    result = 0;

    for (i = 0; i < length; i++)
    {
        ULONG value;

        value = PhCharToInteger[(UCHAR)String->Buffer[i]];

        if (value < Base)
            result = result * Base + value;
        else
            valid = FALSE;
    }

    if (Integer)
    {
        *Integer = result;
    }

    return valid;
}

/**
 * Converts a string to an integer.
 *
 * \param String The string to process.
 * \param Base The base which the string uses to represent the integer. The maximum value is 69. If
 * the parameter is 0, the base is inferred from the string.
 * \param Integer The resulting integer.
 *
 * \remarks If \a Base is 0, the following prefixes may be used to indicate bases:
 *
 * \li \c 0x Base 16.
 * \li \c 0o Base 8.
 * \li \c 0b Base 2.
 * \li \c 0t Base 3.
 * \li \c 0q Base 4.
 * \li \c 0w Base 12.
 * \li \c 0r Base 32.
 *
 * If there is no recognized prefix, base 10 is used.
 */
_Use_decl_annotations_
BOOLEAN PhStringToInteger64(
    _In_ PCPH_STRINGREF String,
    _In_opt_ ULONG Base,
    _Out_opt_ PLONG64 Integer
    )
{
    BOOLEAN valid;
    ULONG64 result;
    PH_STRINGREF string;
    BOOLEAN negative;
    ULONG base;

    if (Base > 69)
        return FALSE;

    string = *String;
    negative = FALSE;

    if (string.Length != 0 && (string.Buffer[0] == L'-' || string.Buffer[0] == L'+'))
    {
        if (string.Buffer[0] == L'-')
            negative = TRUE;

        PhSkipStringRef(&string, sizeof(WCHAR));
    }

    // If the caller specified a base, don't perform any additional processing.

    if (Base)
    {
        base = Base;
    }
    else
    {
        base = 10;

        if (string.Length >= 2 * sizeof(WCHAR) && string.Buffer[0] == L'0')
        {
            switch (string.Buffer[1])
            {
            case L'x':
            case L'X':
                base = 16;
                break;
            case L'o':
            case L'O':
                base = 8;
                break;
            case L'b':
            case L'B':
                base = 2;
                break;
            case L't': // ternary
            case L'T':
                base = 3;
                break;
            case L'q': // quaternary
            case L'Q':
                base = 4;
                break;
            case L'w': // base 12
            case L'W':
                base = 12;
                break;
            case L'r': // base 32
            case L'R':
                base = 32;
                break;
            }

            if (base != 10)
                PhSkipStringRef(&string, 2 * sizeof(WCHAR));
        }
    }

    valid = PhpStringToInteger64(&string, base, &result);

    if (Integer)
        *Integer = negative ? -(LONG64)result : result;

    return valid;
}

/**
 * Converts a string reference to an unsigned 64-bit integer.
 *
 * \param String Pointer to a PH_STRINGREF structure containing the string to convert.
 * \param Base Optional base for conversion (e.g., 10 for decimal, 16 for hexadecimal).
 * \param Integer Optional pointer to a ULONG64 variable that receives the converted value.
 * \return TRUE if the conversion was successful; otherwise, FALSE.
 */
_Use_decl_annotations_
BOOLEAN PhStringToUInt64(
    _In_ PCPH_STRINGREF String,
    _In_opt_ ULONG Base,
    _Out_opt_ PULONG64 Integer
    )
{
    PH_STRINGREF string;
    ULONG base;

    if (Base > 69)
        return FALSE;

    string = *String;

    if (string.Length != 0 && (string.Buffer[0] == L'-' || string.Buffer[0] == L'+'))
    {
        PhSkipStringRef(&string, sizeof(WCHAR));
    }

    // If the caller specified a base, don't perform any additional processing.

    if (Base)
    {
        base = Base;
    }
    else
    {
        base = 10;

        if (string.Length >= 2 * sizeof(WCHAR) && string.Buffer[0] == L'0')
        {
            switch (string.Buffer[1])
            {
            case L'x':
            case L'X':
                base = 16;
                break;
            case L'o':
            case L'O':
                base = 8;
                break;
            case L'b':
            case L'B':
                base = 2;
                break;
            case L't': // ternary
            case L'T':
                base = 3;
                break;
            case L'q': // quaternary
            case L'Q':
                base = 4;
                break;
            case L'w': // base 12
            case L'W':
                base = 12;
                break;
            case L'r': // base 32
            case L'R':
                base = 32;
                break;
            }

            if (base != 10)
                PhSkipStringRef(&string, 2 * sizeof(WCHAR));
        }
    }

    return PhpStringToInteger64(&string, base, Integer);
}

BOOLEAN PhpStringToDouble(
    _In_ PCPH_STRINGREF String,
    _In_ ULONG Base,
    _Out_ DOUBLE *Double
    )
{
    BOOLEAN valid = TRUE;
    BOOLEAN dotSeen = FALSE;
    DOUBLE result;
    DOUBLE fraction;
    SIZE_T length;
    SIZE_T i;

    length = String->Length / sizeof(WCHAR);
    result = 0;
    fraction = 1;

    for (i = 0; i < length; i++)
    {
        if (String->Buffer[i] == L'.')
        {
            if (!dotSeen)
                dotSeen = TRUE;
            else
                valid = FALSE;
        }
        else
        {
            ULONG value;

            value = PhCharToInteger[(UCHAR)String->Buffer[i]];

            if (value < Base)
            {
                if (!dotSeen)
                {
                    result = result * Base + value;
                }
                else
                {
                    fraction /= Base;
                    result = result + value * fraction;
                }
            }
            else
            {
                valid = FALSE;
            }
        }
    }

    *Double = result;

    return valid;
}

/**
 * Converts a string to a double-precision floating point value.
 *
 * \param String The string to process.
 * \param Base Reserved.
 * \param Double The resulting double value.
 */
BOOLEAN PhStringToDouble(
    _In_ PCPH_STRINGREF String,
    _Reserved_ ULONG Base,
    _Out_opt_ DOUBLE *Double
    )
{
    BOOLEAN valid;
    DOUBLE result;
    PH_STRINGREF string;
    BOOLEAN negative;

    string = *String;
    negative = FALSE;

    if (string.Length != 0 && (string.Buffer[0] == L'-' || string.Buffer[0] == L'+'))
    {
        if (string.Buffer[0] == L'-')
            negative = TRUE;

        PhSkipStringRef(&string, sizeof(WCHAR));
    }

    valid = PhpStringToDouble(&string, 10, &result);

    if (Double)
        *Double = negative ? -result : result;

    return valid;
}

/**
 * Converts an integer to a string.
 *
 * \param Integer The integer to process.
 * \param Base The base which the integer is represented with. The maximum value is 69. The base
 * cannot be 1. If the parameter is 0, the base used is 10.
 * \param Signed TRUE if \a Integer is a signed value, otherwise FALSE.
 *
 * \return The resulting string, or NULL if an error occurred.
 */
PPH_STRING PhIntegerToString64(
    _In_ LONG64 Integer,
    _In_opt_ ULONG Base,
    _In_ BOOLEAN Signed
    )
{
    PH_FORMAT format;

    if (Base == 1 || Base > 69)
        return NULL;

    if (Signed)
        PhInitFormatI64D(&format, Integer);
    else
        PhInitFormatI64U(&format, Integer);

    if (Base != 0)
    {
        format.Type |= FormatUseRadix;
        format.Radix = (UCHAR)Base;
    }

    return PhFormat(&format, 1, 0);
}

// PhDoesNameContainWildCards and PhIsNameInExpression compared to
// RtlDoesNameContainWildCards and RtlIsNameInExpression (dmex)
// - Full wildcard support:
// - * -Standard wildcard - zero or more any chars
// - ? -Single character wildcard
// - < (ANSI_DOS_STAR) - Zero or more chars until dot
// - > (ANSI_DOS_QM) - Zero or one char (not dot)
// - " (ANSI_DOS_DOT) - Zero or one dot
// - DOS semantics:
// -  -DOS_STAR(<) stops matching at dots (for filename matching like file<.txt)
// -  -DOS_QM(>) matches 0 or 1 non-dot character.
// -  -DOS_DOT(") matches optional dot character.
//
// Example patterns:
// - *chrome* - matches anything with "chrome"
// - chrome.??? - matches chrome.exe, chrome.dll(3 - char extension)
// - file<.txt - matches file.txt, fileabc.txt(< stops at dot)
// - test>" - matches test, testa, test. (> is 0-1 char, " is optional dot)

// replacement RtlDoesNameContainWildCards (dmex)
/**
 * Determines whether a string contains wildcard characters.
 *
 * \param Expression A pointer to the string to be checked.
 * \return TRUE if one or more wildcard characters were found, FALSE otherwise.
 * \remarks The following are wildcard characters: *, ?, ANSI_DOS_STAR (<), ANSI_DOS_DOT ("), and ANSI_DOS_QM (>).
 */
BOOLEAN PhDoesNameContainWildCards(
    _In_ PCPH_STRINGREF Expression
    )
{
    for (SIZE_T i = 0; i < Expression->Length / sizeof(WCHAR); i++)
    {
        WCHAR c = Expression->Buffer[i];
        
        if (c == L'*' ||
            c == L'?' || 
            c == ANSI_DOS_STAR_W ||
            c == ANSI_DOS_DOT_W ||
            c == ANSI_DOS_QM_W
            )
        {
            return TRUE;
        }
    }
    
    return FALSE;
}

// replacement RtlIsNameInExpression (dmex)
/**
 * Determines whether a string matches the specified wildcard pattern.
 *
 * \param Expression A pointer to the pattern string. Can contain wildcards: *, ?, < (DOS_STAR), > (DOS_QM), " (DOS_DOT).
 * \param Name A pointer to the string to match against the pattern.
 * \param IgnoreCase TRUE for case-insensitive matching, FALSE for case-sensitive matching.
 * \return TRUE if the string matches the pattern, FALSE otherwise.
 * \remarks Wildcard semantics:
 *   * - Matches zero or more characters
 *   ? - Matches exactly one character
 *   < (ANSI_DOS_STAR) - Matches zero or more characters until dot or end
 *   > (ANSI_DOS_QM) - Matches zero or one character (not dot)
 *   " (ANSI_DOS_DOT) - Matches zero or one dot
 */
BOOLEAN PhIsNameInExpression(
    _In_ PCPH_STRINGREF Expression,
    _In_ PCPH_STRINGREF Name,
    _In_ BOOLEAN IgnoreCase
    )
{
    SIZE_T exprLen = Expression->Length / sizeof(WCHAR);
    SIZE_T nameLen = Name->Length / sizeof(WCHAR);
    SIZE_T e = 0; // expression index
    SIZE_T n = 0; // name index
    SIZE_T starE = SIZE_MAX; // last * or < position in expression
    SIZE_T starN = SIZE_MAX; // name position when * or < was encountered
    BOOLEAN dosStarMode = FALSE; // TRUE if last star was DOS_STAR (<)

    while (n < nameLen)
    {
        if (e < exprLen)
        {
            WCHAR exprChar = Expression->Buffer[e];
            WCHAR nameChar = Name->Buffer[n];
            WCHAR exprCharUpper = IgnoreCase ? PhUpcaseUnicodeChar(exprChar) : exprChar;
            WCHAR nameCharUpper = IgnoreCase ? PhUpcaseUnicodeChar(nameChar) : nameChar;

            // * - matches zero or more of any character
            if (exprChar == L'*')
            {
                starE = e++;
                starN = n;
                dosStarMode = FALSE;
                continue;
            }
            // < (ANSI_DOS_STAR) - matches zero or more characters until dot or end
            else if (exprChar == ANSI_DOS_STAR_W)
            {
                starE = e++;
                starN = n;
                dosStarMode = TRUE;
                continue;
            }
            // " (ANSI_DOS_DOT) - matches zero or one dot
            else if (exprChar == ANSI_DOS_DOT_W)
            {
                if (nameChar == L'.')
                {
                    e++;
                    n++;
                }
                else
                {
                    e++; // skip the DOS_DOT, match zero dots
                }
                continue;
            }
            // > (ANSI_DOS_QM) - matches zero or one character (not dot)
            else if (exprChar == ANSI_DOS_QM_W)
            {
                if (nameChar != L'.')
                {
                    e++;
                    n++; // consume one character
                }
                else
                {
                    e++; // skip DOS_QM, match zero characters
                }
                continue;
            }
            // ? - matches exactly one character
            else if (exprChar == L'?')
            {
                e++;
                n++;
                continue;
            }
            // Exact character match
            else if (exprCharUpper == nameCharUpper)
            {
                e++;
                n++;
                continue;
            }
        }

        // Mismatch - backtrack if we saw a * or <
        if (starE != SIZE_MAX)
        {
            e = starE + 1;
            n = ++starN;

            // If in DOS_STAR mode, stop backtracking at dot
            if (dosStarMode && starN < nameLen && Name->Buffer[starN] == L'.')
            {
                dosStarMode = FALSE;
                starE = SIZE_MAX; // can't backtrack past dot
            }
            continue;
        }

        return FALSE;
    }

    // Consume remaining wildcards in expression
    while (e < exprLen)
    {
        WCHAR c = Expression->Buffer[e];
        
        if (
            c == L'*' ||
            c == ANSI_DOS_STAR_W ||
            c == ANSI_DOS_DOT_W ||
            c == ANSI_DOS_QM_W
            )
        {
            e++;
        }
        else
        {
            break;
        }
    }

    return e == exprLen;
}

/**
 * Performs fuzzy matching between a pattern and a text string.
 *
 * \param Pattern A pointer to the search pattern.
 * \param Text A pointer to the text to match against.
 * \param IgnoreCase TRUE for case-insensitive matching, FALSE for case-sensitive matching.
 * \return TRUE if the text fuzzy-matches the pattern, FALSE otherwise.
 * \remarks Fuzzy matching allows characters from the pattern to appear in order
 * in the text, but not necessarily consecutively. his is useful for quick
 * filtering where users type a few key characters.
 */
BOOLEAN PhStringFuzzyMatch(
    _In_ PCPH_STRINGREF Pattern,
    _In_ PCPH_STRINGREF Text,
    _In_ BOOLEAN IgnoreCase
    )
{
    SIZE_T patternLength = Pattern->Length / sizeof(WCHAR);
    SIZE_T textLength = Text->Length / sizeof(WCHAR);
    SIZE_T p = 0; // pattern index
    SIZE_T t = 0; // text index

    if (patternLength == 0)
        return TRUE;
    if (textLength == 0)
        return FALSE;

    while (t < textLength && p < patternLength)
    {
        WCHAR pattern = Pattern->Buffer[p];
        WCHAR text = Text->Buffer[t];

        if (IgnoreCase)
        {
            pattern = PhUpcaseUnicodeChar(pattern);
            text = PhUpcaseUnicodeChar(text);
        }

        if (pattern == text)
        {
            p++; // advance on match
        }

        t++; // always advance text
    }

    // Match succeeds if entire pattern was consumed
    return (p == patternLength);
}

/**
 * Validate the string does not contain control or formatting content.
 *
 * Returns FALSE if the string is NULL, empty, all whitespace, or contains
 * control/formatting characters (RTLO, zero-width chars, etc.).
 *
 * \param String The string to validate.
 * \return TRUE if the string is valid; otherwise, FALSE.
 */
BOOLEAN PhIsControlOrFormattingString(
    _In_opt_ PPH_STRING String
    )
{
    SIZE_T length;
    BOOLEAN hasVisibleContent = FALSE;
    
    if (!String || String->Length == 0)
        return FALSE;
    
    length = String->Length / sizeof(WCHAR);
    
    for (SIZE_T i = 0; i < length; i++)
    {
        WCHAR c = String->Buffer[i];
        
        // Reject strings with control/formatting characters.
        if (PhIsControlOrFormattingUnicodeChar(c))
            return FALSE;
        
        // Check if we have non-whitespace content.
        if (!PhIsWhiteSpaceUnicodeChar(c))
        {
            hasVisibleContent = TRUE;
        }
    }
    
    // String must have at least one visible character
    return hasVisibleContent;
}

/**
 * Sanitizes a string in-place by replacing control and formatting characters.
 *
 * Replaces unicode characters (RTLO, zero-width chars, control chars, etc.)
 * with a visible replacement character directly in the string buffer.
 *
 * \param String The string to sanitize in-place.
 * \param ReplacementChar The character to use as a replacement (e.g., L'?' or PH_UNICODE_REPLACEMENT_CHARACTER).
 * \return The number of characters replaced.
 */
ULONG PhFilterControlOrFormattingString(
    _Inout_ PPH_STRINGREF String,
    _In_ WCHAR ReplacementChar
    )
{
    SIZE_T length;
    ULONG replaced = 0;
    
    if (!String || String->Length == 0)
        return 0;
    
    length = String->Length / sizeof(WCHAR);
    
    for (SIZE_T i = 0; i < length; i++)
    {
        if (PhIsControlOrFormattingUnicodeChar(String->Buffer[i]))
        {
            String->Buffer[i] = PH_UNICODE_REPLACEMENT_CHARACTER;
            replaced++;
        }
    }
    
    return replaced;
}
