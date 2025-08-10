/*
 * Copyright (c) 2022 Winsider Seminars & Solutions, Inc.  All rights reserved.
 *
 * This file is part of System Informer.
 *
 * Authors:
 *
 *     wj32    2010-2015
 *     dmex    2017-2023
 *
 */

#ifndef _PH_PHSUP_H
#define _PH_PHSUP_H

// This header file provides some useful definitions specific to phlib.

//
// Workaround for macro redefinition conflicts between Clang and MSVC.
// Save and restore these definitions.
//
// See: https://github.com/llvm/llvm-project/issues/36748
//
// C:\Program Files\LLVM\lib\clang\20\include\xmmintrin.h(2195,9): error: '_MM_HINT_T0' macro redefined [-Werror,-Wmacro-redefined]
//  2195 | #define _MM_HINT_T0  3
//       |         ^
// C:\Program Files (x86)\Windows Kits\10\\include\10.0.26100.0\\um\winnt.h(3649,9): note: previous definition is here
//  3649 | #define _MM_HINT_T0     1
//       |
//
// Workaround for unused function warnings.
// Narrowly suppress -Wunused-function warnings.
//
// C:\Program Files\LLVM\lib\clang\20\include\amxcomplexintrin.h(139,13): error: unused function '__tile_cmmimfp16ps' [-Werror,-Wunused-function]
//   139 | static void __tile_cmmimfp16ps(__tile1024i *dst, __tile1024i src0,
//       |             ^~~~~~~~~~~~~~~~~~
// C:\Program Files\LLVM\lib\clang\20\include\amxcomplexintrin.h(162,13): error: unused function '__tile_cmmrlfp16ps' [-Werror,-Wunused-function]
//   162 | static void __tile_cmmrlfp16ps(__tile1024i *dst, __tile1024i src0,
//       |             ^~~~~~~~~~~~~~~~~~
//
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#ifdef _MM_HINT_T0
#pragma push_macro("_MM_HINT_T0")
#pragma push_macro("_MM_HINT_T1")
#pragma push_macro("_MM_HINT_T2")
#undef _MM_HINT_T0
#undef _MM_HINT_T1
#undef _MM_HINT_T2
#define PH_SAVED_MM_HINT_MACROS
#endif // _MM_HINT_T0
#endif // __clang__
#include <intrin.h>
#ifdef __clang__
#pragma clang diagnostic pop
#ifdef PH_SAVED_MM_HINT_MACROS
#pragma pop_macro("_MM_HINT_T2")
#pragma pop_macro("_MM_HINT_T1")
#pragma pop_macro("_MM_HINT_T0")
#undef PH_SAVED_MM_HINT_MACROS
#endif // PH_SAVED_MM_HINT_MACROS
#endif // __clang__

#include <wchar.h>
#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <locale.h>
#include <inttypes.h>
#include <math.h>
#include <malloc.h>
#include <float.h>

//
// Memory
//

#define PTR_ADD_OFFSET(Pointer, Offset) ((PVOID)((ULONG_PTR)(Pointer) + (ULONG_PTR)(Offset)))
#define PTR_SUB_OFFSET(Pointer, Offset) ((PVOID)((ULONG_PTR)(Pointer) - (ULONG_PTR)(Offset)))

#define PH_LARGE_BUFFER_SIZE (256 * 1024 * 1024)

#define XMM_SIZE sizeof(__m128i)
#define XMM_OFFSET(p) ((XMM_SIZE - 1) & (ULONG_PTR)(p))
#define XMM_CHARS (XMM_SIZE / sizeof(wchar_t))
#define XMM_CHAR_ALIGNED(p) (0 == (((ULONG_PTR)(p) + sizeof(wchar_t) - 1) & (0-sizeof(wchar_t)) & (XMM_SIZE-1)))
#define XMM_PAGE_SAFE(p) (PAGE_OFFSET(p) <= (PAGE_SIZE - XMM_SIZE))

//
// Exceptions
//

#define PhRaiseStatus(Status) RtlRaiseStatus(Status)

#define SIMPLE_EXCEPTION_FILTER(Condition) \
    ((Condition) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)

//
// Compiler
//

#ifdef DEBUG
#define ASSUME_ASSERT(Expression) assert(Expression)
#define ASSUME_NO_DEFAULT assert(FALSE)
#else
#define ASSUME_ASSERT(Expression) __assume(Expression)
#define ASSUME_NO_DEFAULT __assume(FALSE)
#endif

//
// Math
//

#if defined(_M_IX86)
#define UInt32Add32To64(a, b) ((unsigned __int64)(((unsigned __int64)((unsigned int)(a))) + ((unsigned int)(b)))) // Avoids warning C26451 (dmex)
#define UInt32Sub32To64(a, b) ((unsigned __int64)(((unsigned __int64)((unsigned int)(a))) - ((unsigned int)(b))))
#define UInt32Div32To64(a, b) ((unsigned __int64)(((unsigned __int64)((unsigned int)(a))) / ((unsigned int)(b))))
#define UInt32Mul32To64(a, b) ((unsigned __int64)(((unsigned __int64)((unsigned int)(a))) / ((unsigned int)(b))))
#else
#define UInt32Add32To64(a, b) (((unsigned __int64)((unsigned int)(a))) + ((unsigned __int64)((unsigned int)(b)))) // from UInt32x32To64 (dmex)
#define UInt32Sub32To64(a, b) (((unsigned __int64)((unsigned int)(a))) - ((unsigned __int64)((unsigned int)(b))))
#define UInt32Div32To64(a, b) (((unsigned __int64)((unsigned int)(a))) / ((unsigned __int64)((unsigned int)(b))))
#define UInt32Mul32To64(a, b) (((unsigned __int64)((unsigned int)(a))) * ((unsigned __int64)((unsigned int)(b))))
#endif

//
// Time
//

#define PH_TICKS_PER_NS ((LONG64)1 * 10)
#define PH_TICKS_PER_MS (PH_TICKS_PER_NS * 1000)
#define PH_TICKS_PER_SEC (PH_TICKS_PER_MS * 1000)
#define PH_TICKS_PER_MIN (PH_TICKS_PER_SEC * 60)
#define PH_TICKS_PER_HOUR (PH_TICKS_PER_MIN * 60)
#define PH_TICKS_PER_DAY (PH_TICKS_PER_HOUR * 24)

#define PH_TICKS_PARTIAL_NS(Ticks) (((ULONG64)(Ticks) / PH_TICKS_PER_NS) % 1000000)
#define PH_TICKS_PARTIAL_MS(Ticks) (((ULONG64)(Ticks) / PH_TICKS_PER_MS) % 1000)
#define PH_TICKS_PARTIAL_SEC(Ticks) (((ULONG64)(Ticks) / PH_TICKS_PER_SEC) % 60)
#define PH_TICKS_PARTIAL_MIN(Ticks) (((ULONG64)(Ticks) / PH_TICKS_PER_MIN) % 60)
#define PH_TICKS_PARTIAL_HOURS(Ticks) (((ULONG64)(Ticks) / PH_TICKS_PER_HOUR) % 24)
#define PH_TICKS_PARTIAL_DAYS(Ticks) ((ULONG64)(Ticks) / PH_TICKS_PER_DAY)

#define PH_TIMEOUT_MS PH_TICKS_PER_MS
#define PH_TIMEOUT_SEC PH_TICKS_PER_SEC

//
// Annotations
//

/**
 * Indicates that a function assumes the specified number of references are available for the
 * object.
 *
 * \remarks Usually functions reference objects if they store them for later usage; this annotation
 * specifies that the caller must supply these extra references itself. In effect these references
 * are "transferred" to the function and must not be used. E.g. if you create an object and
 * immediately call a function with _Assume_refs_(1), you may no longer use the object since that
 * one reference you held is no longer yours.
 */
#define _Assume_refs_(count)

#define _Callback_

/**
 * Indicates that a function may raise a software exception.
 *
 * \remarks Do not use this annotation for temporary usages of exceptions, e.g. unimplemented
 * functions.
 */
#define _May_raise_

/**
 * Indicates that a function requires the specified value to be aligned at the specified number of
 * bytes.
 */
#define _Needs_align_(align)

//
// Casts
//

// Zero extension and sign extension macros
#define C_1uTo2(x) ((unsigned short)(unsigned char)(x))
#define C_1sTo2(x) ((unsigned short)(signed char)(x))
#define C_1uTo4(x) ((unsigned int)(unsigned char)(x))
#define C_1sTo4(x) ((unsigned int)(signed char)(x))
#define C_2uTo4(x) ((unsigned int)(unsigned short)(x))
#define C_2sTo4(x) ((unsigned int)(signed short)(x))
#define C_4uTo8(x) ((unsigned __int64)(unsigned int)(x))
#define C_4sTo8(x) ((unsigned __int64)(signed int)(x))

//
// Sorting
//

typedef enum _PH_SORT_ORDER
{
    NoSortOrder = 0,
    AscendingSortOrder,
    DescendingSortOrder
} PH_SORT_ORDER, *PPH_SORT_ORDER;

FORCEINLINE LONG PhModifySort(
    _In_ LONG Result,
    _In_ PH_SORT_ORDER Order
    )
{
    switch (Order)
    {
    case AscendingSortOrder:
        return Result;
    case DescendingSortOrder:
        return -Result;
    default:
        return Result;
    }
}

#define PH_BUILTIN_COMPARE(value1, value2) \
    if ((value1) > (value2)) \
        return 1; \
    else if ((value1) < (value2)) \
        return -1; \
    \
    return 0

FORCEINLINE int charcmp(
    _In_ signed char value1,
    _In_ signed char value2
    )
{
    return C_1sTo4(value1 - value2);
}

FORCEINLINE int ucharcmp(
    _In_ unsigned char value1,
    _In_ unsigned char value2
    )
{
    PH_BUILTIN_COMPARE(value1, value2);
}

FORCEINLINE int shortcmp(
    _In_ signed short value1,
    _In_ signed short value2
    )
{
    return C_2sTo4(value1 - value2);
}

FORCEINLINE int ushortcmp(
    _In_ unsigned short value1,
    _In_ unsigned short value2
    )
{
    PH_BUILTIN_COMPARE(value1, value2);
}

FORCEINLINE int intcmp(
    _In_ int value1,
    _In_ int value2
    )
{
    return value1 - value2;
}

FORCEINLINE int uintcmp(
    _In_ unsigned int value1,
    _In_ unsigned int value2
    )
{
    PH_BUILTIN_COMPARE(value1, value2);
}

FORCEINLINE int int64cmp(
    _In_ __int64 value1,
    _In_ __int64 value2
    )
{
    PH_BUILTIN_COMPARE(value1, value2);
}

FORCEINLINE int uint64cmp(
    _In_ unsigned __int64 value1,
    _In_ unsigned __int64 value2
    )
{
    PH_BUILTIN_COMPARE(value1, value2);
}

FORCEINLINE int intptrcmp(
    _In_ LONG_PTR value1,
    _In_ LONG_PTR value2
    )
{
    PH_BUILTIN_COMPARE(value1, value2);
}

FORCEINLINE int uintptrcmp(
    _In_ ULONG_PTR value1,
    _In_ ULONG_PTR value2
    )
{
    PH_BUILTIN_COMPARE(value1, value2);
}

FORCEINLINE int singlecmp(
    _In_ float value1,
    _In_ float value2
    )
{
    PH_BUILTIN_COMPARE(value1, value2);
}

FORCEINLINE int doublecmp(
    _In_ double value1,
    _In_ double value2
    )
{
    PH_BUILTIN_COMPARE(value1, value2);
}

FORCEINLINE int wcsicmp2(
    _In_opt_ PCWSTR Value1,
    _In_opt_ PCWSTR Value2
    )
{
    if (Value1 && Value2)
        return _wcsicmp(Value1, Value2);
    else if (!Value1)
        return !Value2 ? 0 : -1;
    else
        return 1;
}

typedef int (__cdecl *PC_COMPARE_FUNCTION)(void *, const void *, const void *);

//
// Synchronization
//

FORCEINLINE LONG_PTR __InterlockedExchangeAddPointer(
    _Inout_ _Interlocked_operand_ LONG_PTR volatile *Addend,
    _In_ LONG_PTR Value
    )
{
#ifdef _WIN64
    return (LONG_PTR)_InterlockedExchangeAdd64((volatile LONG64*)Addend, (LONG64)Value);
#else
    return (LONG_PTR)_InterlockedExchangeAdd((volatile LONG*)Addend, (LONG)Value);
#endif
}

#define _InterlockedExchangeAddPointer __InterlockedExchangeAddPointer

FORCEINLINE LONG_PTR _InterlockedIncrementPointer(
    _Inout_ _Interlocked_operand_ LONG_PTR volatile *Addend
    )
{
#ifdef _WIN64
    return (LONG_PTR)_InterlockedIncrement64((volatile LONG64*)Addend);
#else
    return (LONG_PTR)_InterlockedIncrement((volatile LONG*)Addend);
#endif
}

FORCEINLINE LONG_PTR __InterlockedDecrementPointer(
    _Inout_ _Interlocked_operand_ LONG_PTR volatile *Addend
    )
{
#ifdef _WIN64
    return (LONG_PTR)_InterlockedDecrement64((volatile LONG64*)Addend);
#else
    return (LONG_PTR)_InterlockedDecrement((volatile LONG*)Addend);
#endif
}

#define _InterlockedDecrementPointer __InterlockedDecrementPointer

FORCEINLINE BOOLEAN _InterlockedBitTestAndResetPointer(
    _Inout_ _Interlocked_operand_ LONG_PTR volatile *Base,
    _In_ LONG_PTR Bit
    )
{
#ifdef _WIN64
    return _interlockedbittestandreset64((volatile LONG64*)Base, (LONG64)Bit);
#else
    return _interlockedbittestandreset((volatile LONG*)Base, (LONG)Bit);
#endif
}

FORCEINLINE BOOLEAN _InterlockedBitTestAndSetPointer(
    _Inout_ _Interlocked_operand_ LONG_PTR volatile *Base,
    _In_ LONG_PTR Bit
    )
{
#ifdef _WIN64
    return _interlockedbittestandset64((volatile LONG64*)Base, (LONG64)Bit);
#else
    return _interlockedbittestandset((volatile LONG*)Base, (LONG)Bit);
#endif
}

FORCEINLINE BOOLEAN _InterlockedIncrementNoZero(
    _Inout_ _Interlocked_operand_ LONG volatile *Addend
    )
{
    LONG value;
    LONG newValue;

    value = *Addend;

    while (TRUE)
    {
        if (value == 0)
            return FALSE;

        if ((newValue = _InterlockedCompareExchange(
            Addend,
            value + 1,
            value
            )) == value)
        {
            return TRUE;
        }

        value = newValue;
    }
}

FORCEINLINE BOOLEAN _InterlockedIncrementPositive(
    _Inout_ _Interlocked_operand_ LONG volatile *Addend
    )
{
    LONG value;
    LONG newValue;

    value = *Addend;

    while (TRUE)
    {
        if (value <= 0)
            return FALSE;

        if ((newValue = _InterlockedCompareExchange(
            Addend,
            value + 1,
            value
            )) == value)
        {
            return TRUE;
        }

        value = newValue;
    }
}

FORCEINLINE
PVOID
_InterlockedReadPointer(
    _Inout_ _Interlocked_operand_ volatile PVOID *Base
    )
{
    return _InterlockedCompareExchangePointer(Base, NULL, NULL);
}

FORCEINLINE
PVOID
_InterlockedWritePointer(
    _Inout_ _Interlocked_operand_ volatile PVOID* Base,
    _In_ PVOID Value
    )
{
    return _InterlockedExchangePointer(Base, Value);
}

#ifndef InterlockedReadPointer
#define InterlockedReadPointer _InterlockedReadPointer
#endif
#ifndef InterlockedWritePointer
#define InterlockedWritePointer _InterlockedWritePointer
#endif

//
// Strings
//

#define PH_INT32_STR_LEN 12
#define PH_INT32_STR_LEN_1 (PH_INT32_STR_LEN + 1)

#define PH_INT64_STR_LEN 50
#define PH_INT64_STR_LEN_1 (PH_INT64_STR_LEN + 1)

#define PH_PTR_STR_LEN 24
#define PH_PTR_STR_LEN_1 (PH_PTR_STR_LEN + 1)

#define PH_HEX_CHARS L"0123456789abcdef"

#pragma warning(push)
#pragma warning(disable:4996)

FORCEINLINE
VOID
PhPrintInt32(
    _Out_writes_(PH_INT32_STR_LEN_1) CONST PWSTR Destination,
    _In_ CONST LONG Int32
    )
{
    _ltow(Int32, Destination, 10);
}

FORCEINLINE
VOID
PhPrintUInt32(
    _Out_writes_(PH_INT32_STR_LEN_1) CONST PWSTR Destination,
    _In_ CONST ULONG UInt32
    )
{
    _ultow(UInt32, Destination, 10);
}

FORCEINLINE
VOID
PhPrintUInt32IX(
    _Out_writes_(PH_PTR_STR_LEN_1) CONST PWSTR Destination,
    _In_ CONST ULONG UInt32
    )
{
    _ultow(UInt32, Destination, 16);
}

FORCEINLINE
VOID
PhPrintInt64(
    _Out_writes_(PH_INT64_STR_LEN_1) CONST PWSTR Destination,
    _In_ CONST LONG64 Int64
    )
{
    _i64tow(Int64, Destination, 10);
}

FORCEINLINE
VOID
PhPrintUInt64(
    _Out_writes_(PH_INT64_STR_LEN_1) CONST PWSTR Destination,
    _In_ CONST ULONG64 UInt64
    )
{
    _ui64tow(UInt64, Destination, 10);
}

FORCEINLINE
VOID
PhPrintPointer(
    _Out_writes_(PH_PTR_STR_LEN_1) PWSTR Destination,
    _In_opt_ PVOID Pointer
    )
{
    Destination[0] = L'0';
    Destination[1] = L'x';
#ifdef _WIN64
    _ui64tow((ULONG64)Pointer, &Destination[2], 16);
#else
    _ultow((ULONG)Pointer, &Destination[2], 16);
#endif
}

#pragma warning(pop)

FORCEINLINE
VOID
PhPrintPointerPadZeros(
    _Out_writes_(PH_PTR_STR_LEN_1) PWSTR Destination,
    _In_ PVOID Pointer
    )
{
    ULONG_PTR ptr = (ULONG_PTR)Pointer;
    PWSTR dest = Destination;

    *dest++ = L'0';
    *dest++ = L'x';

#ifdef _WIN64
    *dest++ = PH_HEX_CHARS[ptr >> 60];
    *dest++ = PH_HEX_CHARS[(ptr & 0x0f00000000000000) >> 56];
    *dest++ = PH_HEX_CHARS[(ptr & 0x00f0000000000000) >> 52];
    *dest++ = PH_HEX_CHARS[(ptr & 0x000f000000000000) >> 48];
    *dest++ = PH_HEX_CHARS[(ptr & 0x0000f00000000000) >> 44];
    *dest++ = PH_HEX_CHARS[(ptr & 0x00000f0000000000) >> 40];
    *dest++ = PH_HEX_CHARS[(ptr & 0x000000f000000000) >> 36];
    *dest++ = PH_HEX_CHARS[(ptr & 0x0000000f00000000) >> 32];
    *dest++ = PH_HEX_CHARS[(ptr & 0x00000000f0000000) >> 28];
    *dest++ = PH_HEX_CHARS[(ptr & 0x000000000f000000) >> 24];
    *dest++ = PH_HEX_CHARS[(ptr & 0x0000000000f00000) >> 20];
    *dest++ = PH_HEX_CHARS[(ptr & 0x00000000000f0000) >> 16];
    *dest++ = PH_HEX_CHARS[(ptr & 0x000000000000f000) >> 12];
    *dest++ = PH_HEX_CHARS[(ptr & 0x0000000000000f00) >> 8];
    *dest++ = PH_HEX_CHARS[(ptr & 0x00000000000000f0) >> 4];
    *dest++ = PH_HEX_CHARS[ptr & 0x000000000000000f];
#else
    *dest++ = PH_HEX_CHARS[ptr >> 28];
    *dest++ = PH_HEX_CHARS[(ptr & 0x0f000000) >> 24];
    *dest++ = PH_HEX_CHARS[(ptr & 0x00f00000) >> 20];
    *dest++ = PH_HEX_CHARS[(ptr & 0x000f0000) >> 16];
    *dest++ = PH_HEX_CHARS[(ptr & 0x0000f000) >> 12];
    *dest++ = PH_HEX_CHARS[(ptr & 0x00000f00) >> 8];
    *dest++ = PH_HEX_CHARS[(ptr & 0x000000f0) >> 4];
    *dest++ = PH_HEX_CHARS[ptr & 0x0000000f];
#endif

    *dest = UNICODE_NULL;
}

// Misc.

FORCEINLINE
ULONG64
PhRoundNumber(
    _In_ CONST ULONG64 Value,
    _In_ CONST ULONG64 Granularity
    )
{
    return (Value + Granularity / 2) / Granularity * Granularity;
}

FORCEINLINE
ULONG
PhMultiplyDivide(
    _In_ CONST ULONG Number,
    _In_ CONST ULONG Numerator,
    _In_ CONST ULONG Denominator
    )
{
    //return (((ULONG64)Number * (ULONG64)Numerator + (ULONG64)Denominator / 2) / (ULONG64)Denominator);
    return UInt32Div32To64(UInt32Add32To64(UInt32Mul32To64(Number, Numerator), UInt32Div32To64(Denominator, 2)), Denominator);
}

FORCEINLINE
LONG
PhMultiplyDivideSigned(
    _In_ CONST LONG Number,
    _In_ CONST ULONG Numerator,
    _In_ CONST ULONG Denominator
    )
{
    if (Number >= 0)
        return (LONG)PhMultiplyDivide(Number, Numerator, Denominator);
    else
        return -(LONG)PhMultiplyDivide(-Number, Numerator, Denominator);
}

FORCEINLINE
VOID
PhProbeAddress(
    _In_ CONST PVOID UserAddress,
    _In_ CONST SIZE_T UserLength,
    _In_ CONST PVOID BufferAddress,
    _In_ CONST SIZE_T BufferLength,
    _In_ CONST ULONG Alignment
    )
{
    if (UserLength != 0)
    {
        if (((ULONG_PTR)UserAddress & (Alignment - 1)) != 0)
            PhRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);

        if (
            ((ULONG_PTR)UserAddress + UserLength < (ULONG_PTR)UserAddress) ||
            ((ULONG_PTR)UserAddress < (ULONG_PTR)BufferAddress) ||
            ((ULONG_PTR)UserAddress + UserLength > (ULONG_PTR)BufferAddress + BufferLength)
            )
            PhRaiseStatus(STATUS_ACCESS_VIOLATION);
    }
}

/**
 * Probes a user address for read access and checks if the specified user address is readable
 * and within the bounds of the buffer.
 *
 * @param UserAddress The address to probe.
 * @param UserLength The length of the memory to probe.
 * @param BufferAddress The base address of the buffer.
 * @param BufferLength The length of the buffer.
 * @param Alignment The required alignment of the address.
 */
FORCEINLINE
VOID
PhProbeForRead(
    _In_ CONST PVOID UserAddress,
    _In_ CONST SIZE_T UserLength,
    _In_ CONST PVOID BufferAddress,
    _In_ CONST SIZE_T BufferLength,
    _In_ CONST ULONG Alignment
    )
{
    if (UserLength != 0)
    {
        PhProbeAddress(UserAddress, UserLength, BufferAddress, BufferLength, Alignment);

        // Align the UserLength to the nearest page boundary.
        SIZE_T length = (SIZE_T)ALIGN_UP_BY(UserLength, PAGE_SIZE);

        // Iterate over each page and ensure the address is valid and accessible.
        for (SIZE_T offset = 0; offset < length; offset += PAGE_SIZE)
        {
            // Ensure the address does not overflow
            if ((ULONG_PTR)UserAddress + offset < (ULONG_PTR)UserAddress)
            {
                PhRaiseStatus(STATUS_ACCESS_VIOLATION);
            }

            *((volatile char*)UserAddress + offset);
        }
    }
}

FORCEINLINE
VOID
PhProbeForWrite(
    _In_ CONST PVOID UserAddress,
    _In_ CONST SIZE_T UserLength,
    _In_ CONST PVOID BufferAddress,
    _In_ CONST SIZE_T BufferLength,
    _In_ CONST ULONG Alignment
    )
{
    if (UserLength != 0)
    {
        PhProbeAddress(UserAddress, UserLength, BufferAddress, BufferLength, Alignment);

        // Align the UserLength to the nearest page boundary.
        SIZE_T length = (SIZE_T)ALIGN_UP_BY(UserLength, PAGE_SIZE);

        // Iterate over each page and ensure the address is valid and accessible.
        for (SIZE_T offset = 0; offset < length; offset += PAGE_SIZE)
        {
            // Ensure the address does not overflow
            if ((ULONG_PTR)UserAddress + offset < (ULONG_PTR)UserAddress)
            {
                PhRaiseStatus(STATUS_ACCESS_VIOLATION);
            }

            *((volatile char*)UserAddress + offset) = *((volatile char*)UserAddress + offset);
        }
    }
}

FORCEINLINE
PLARGE_INTEGER
PhTimeoutFromMilliseconds(
    _Out_ CONST PLARGE_INTEGER Timeout,
    _In_ CONST ULONG Milliseconds
    )
{
    if (Milliseconds == INFINITE)
        Timeout->QuadPart = MINLONGLONG;
    else
        Timeout->QuadPart = -(LONGLONG)UInt32x32To64(Milliseconds, PH_TIMEOUT_MS);

    return Timeout;
}

#endif
