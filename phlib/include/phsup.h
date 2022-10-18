#ifndef _PH_PHSUP_H
#define _PH_PHSUP_H

// This header file provides some useful definitions specific to phlib.

#include <intrin.h>
#include <wchar.h>
#include <assert.h>
#include <stdio.h>

// Memory

#define PTR_ADD_OFFSET(Pointer, Offset) ((PVOID)((ULONG_PTR)(Pointer) + (ULONG_PTR)(Offset)))
#define PTR_SUB_OFFSET(Pointer, Offset) ((PVOID)((ULONG_PTR)(Pointer) - (ULONG_PTR)(Offset)))
#define ALIGN_UP_BY(Address, Align) (((ULONG_PTR)(Address) + (Align) - 1) & ~((Align) - 1))
#define ALIGN_UP_POINTER_BY(Pointer, Align) ((PVOID)ALIGN_UP_BY(Pointer, Align))
#define ALIGN_UP(Address, Type) ALIGN_UP_BY(Address, sizeof(Type))
#define ALIGN_UP_POINTER(Pointer, Type) ((PVOID)ALIGN_UP(Pointer, Type))
#define ALIGN_DOWN_BY(Address, Align) ((ULONG_PTR)(Address) & ~((ULONG_PTR)(Align) - 1))
#define ALIGN_DOWN_POINTER_BY(Pointer, Align) ((PVOID)ALIGN_DOWN_BY(Pointer, Align))
#define ALIGN_DOWN(Address, Type) ALIGN_DOWN_BY(Address, sizeof(Type))
#define ALIGN_DOWN_POINTER(Pointer, Type) ((PVOID)ALIGN_DOWN(Pointer, Type))

#define PAGE_SIZE 0x1000

#define PH_LARGE_BUFFER_SIZE (256 * 1024 * 1024)

// Exceptions

#define PhRaiseStatus(Status) RtlRaiseStatus(Status)

#define SIMPLE_EXCEPTION_FILTER(Condition) \
    ((Condition) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)

// Compiler

#ifdef DEBUG
#define ASSUME_ASSERT(Expression) assert(Expression)
#define ASSUME_NO_DEFAULT assert(FALSE)
#else
#define ASSUME_ASSERT(Expression) __assume(Expression)
#define ASSUME_NO_DEFAULT __assume(FALSE)
#endif

// Math

#define UInt32Add32To64(a, b) ((unsigned __int64)((unsigned __int64)(a) + ((unsigned __int64)(b)))) // Avoids warning C26451 (dmex)
#define UInt32Mul32To64(a, b) ((unsigned __int64)((unsigned __int64)(a) * ((unsigned __int64)(b))))

// Time

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

// Annotations

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

// Casts

// Zero extension and sign extension macros
#define C_1uTo2(x) ((unsigned short)(unsigned char)(x))
#define C_1sTo2(x) ((unsigned short)(signed char)(x))
#define C_1uTo4(x) ((unsigned int)(unsigned char)(x))
#define C_1sTo4(x) ((unsigned int)(signed char)(x))
#define C_2uTo4(x) ((unsigned int)(unsigned short)(x))
#define C_2sTo4(x) ((unsigned int)(signed short)(x))
#define C_4uTo8(x) ((unsigned __int64)(unsigned int)(x))
#define C_4sTo8(x) ((unsigned __int64)(signed int)(x))

// Sorting

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
    if (Order == AscendingSortOrder)
        return Result;
    else if (Order == DescendingSortOrder)
        return -Result;
    else
        return Result;
}

#define PH_BUILTIN_COMPARE(value1, value2) \
    if (value1 > value2) \
        return 1; \
    else if (value1 < value2) \
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
    _In_opt_ PWSTR Value1,
    _In_opt_ PWSTR Value2
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

// Synchronization

#ifndef _WIN64

#ifndef _InterlockedCompareExchangePointer
void *_InterlockedCompareExchangePointer(
    void *volatile *Destination,
    void *Exchange,
    void *Comparand
    );
#endif

#if (_MSC_VER < 1900)
#ifndef _InterlockedExchangePointer
FORCEINLINE void *_InterlockedExchangePointer(
    void *volatile *Destination,
    void *Exchange
    )
{
    return (PVOID)_InterlockedExchange(
        (PLONG_PTR)Destination,
        (LONG_PTR)Exchange
        );
}
#endif
#endif

#endif

FORCEINLINE LONG_PTR _InterlockedExchangeAddPointer(
    _Inout_ _Interlocked_operand_ LONG_PTR volatile *Addend,
    _In_ LONG_PTR Value
    )
{
#ifdef _WIN64
    return (LONG_PTR)_InterlockedExchangeAdd64((PLONG64)Addend, (LONG64)Value);
#else
    return (LONG_PTR)_InterlockedExchangeAdd((PLONG)Addend, (LONG)Value);
#endif
}

FORCEINLINE LONG_PTR _InterlockedIncrementPointer(
    _Inout_ _Interlocked_operand_ LONG_PTR volatile *Addend
    )
{
#ifdef _WIN64
    return (LONG_PTR)_InterlockedIncrement64((PLONG64)Addend);
#else
    return (LONG_PTR)_InterlockedIncrement((PLONG)Addend);
#endif
}

FORCEINLINE LONG_PTR _InterlockedDecrementPointer(
    _Inout_ _Interlocked_operand_ LONG_PTR volatile *Addend
    )
{
#ifdef _WIN64
    return (LONG_PTR)_InterlockedDecrement64((PLONG64)Addend);
#else
    return (LONG_PTR)_InterlockedDecrement((PLONG)Addend);
#endif
}

FORCEINLINE BOOLEAN _InterlockedBitTestAndResetPointer(
    _Inout_ _Interlocked_operand_ LONG_PTR volatile *Base,
    _In_ LONG_PTR Bit
    )
{
#ifdef _WIN64
    return _interlockedbittestandreset64((PLONG64)Base, (LONG64)Bit);
#else
    return _interlockedbittestandreset((PLONG)Base, (LONG)Bit);
#endif
}

FORCEINLINE BOOLEAN _InterlockedBitTestAndSetPointer(
    _Inout_ _Interlocked_operand_ LONG_PTR volatile *Base,
    _In_ LONG_PTR Bit
    )
{
#ifdef _WIN64
    return _interlockedbittestandset64((PLONG64)Base, (LONG64)Bit);
#else
    return _interlockedbittestandset((PLONG)Base, (LONG)Bit);
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

// Strings

#define PH_INT32_STR_LEN 12
#define PH_INT32_STR_LEN_1 (PH_INT32_STR_LEN + 1)

#define PH_INT64_STR_LEN 50
#define PH_INT64_STR_LEN_1 (PH_INT64_STR_LEN + 1)

#define PH_PTR_STR_LEN 24
#define PH_PTR_STR_LEN_1 (PH_PTR_STR_LEN + 1)

#define PH_HEX_CHARS L"0123456789abcdef"

FORCEINLINE VOID PhPrintInt32(
    _Out_writes_(PH_INT32_STR_LEN_1) PWSTR Destination,
    _In_ LONG Int32
    )
{
    _ltow(Int32, Destination, 10);
}

FORCEINLINE VOID PhPrintUInt32(
    _Out_writes_(PH_INT32_STR_LEN_1) PWSTR Destination,
    _In_ ULONG UInt32
    )
{
    _ultow(UInt32, Destination, 10);
}

FORCEINLINE VOID PhPrintInt64(
    _Out_writes_(PH_INT64_STR_LEN_1) PWSTR Destination,
    _In_ LONG64 Int64
    )
{
    _i64tow(Int64, Destination, 10);
}

FORCEINLINE VOID PhPrintUInt64(
    _Out_writes_(PH_INT64_STR_LEN_1) PWSTR Destination,
    _In_ ULONG64 UInt64
    )
{
    _ui64tow(UInt64, Destination, 10);
}

FORCEINLINE VOID PhPrintPointer(
    _Out_writes_(PH_PTR_STR_LEN_1) PWSTR Destination,
    _In_ PVOID Pointer
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

FORCEINLINE VOID PhPrintPointerPadZeros(
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

FORCEINLINE ULONG PhCountBits(
    _In_ ULONG Value
    )
{
    ULONG count = 0;

    while (Value)
    {
        count++;
        Value &= Value - 1;
    }

    return count;
}

FORCEINLINE ULONG PhCountBitsUlongPtr(
    _In_ ULONG_PTR Value
    )
{
    ULONG count = 0;

    while (Value)
    {
        count++;
        Value &= Value - 1;
    }

    return count;
}

FORCEINLINE ULONG64 PhRoundNumber(
    _In_ ULONG64 Value,
    _In_ ULONG64 Granularity
    )
{
    return (Value + Granularity / 2) / Granularity * Granularity;
}

FORCEINLINE ULONG PhMultiplyDivide(
    _In_ ULONG Number,
    _In_ ULONG Numerator,
    _In_ ULONG Denominator
    )
{
    return (ULONG)(((ULONG64)Number * (ULONG64)Numerator + Denominator / 2) / (ULONG64)Denominator);
}

FORCEINLINE LONG PhMultiplyDivideSigned(
    _In_ LONG Number,
    _In_ ULONG Numerator,
    _In_ ULONG Denominator
    )
{
    if (Number >= 0)
        return PhMultiplyDivide(Number, Numerator, Denominator);
    else
        return -(LONG)PhMultiplyDivide(-Number, Numerator, Denominator);
}

FORCEINLINE VOID PhProbeAddress(
    _In_ PVOID UserAddress,
    _In_ SIZE_T UserLength,
    _In_ PVOID BufferAddress,
    _In_ SIZE_T BufferLength,
    _In_ ULONG Alignment
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

FORCEINLINE PLARGE_INTEGER PhTimeoutFromMilliseconds(
    _Out_ PLARGE_INTEGER Timeout,
    _In_ ULONG Milliseconds
    )
{
    if (Milliseconds == INFINITE)
        Timeout->QuadPart = MINLONGLONG;
    else
        Timeout->QuadPart = -(LONGLONG)UInt32x32To64(Milliseconds, PH_TIMEOUT_MS);

    return Timeout;
}

#define PhTimeoutFromMillisecondsEx(Milliseconds) \
    &(LARGE_INTEGER) { .QuadPart = -(LONGLONG)UInt32x32To64(((ULONG)(Milliseconds)), PH_TIMEOUT_MS) }

FORCEINLINE NTSTATUS PhGetLastWin32ErrorAsNtStatus(
    VOID
    )
{
    ULONG win32Result;

    // This is needed because NTSTATUS_FROM_WIN32 uses the argument multiple times.
    win32Result = GetLastError();

    return NTSTATUS_FROM_WIN32(win32Result);
}

#endif
