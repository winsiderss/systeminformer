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
#define PTR_ALIGN(Pointer, Align) ((PVOID)(((ULONG_PTR)(Pointer) + (Align) - 1) & ~((Align) - 1)))
#define REBASE_ADDRESS(Pointer, OldBase, NewBase) \
    ((PVOID)((ULONG_PTR)(Pointer) - (ULONG_PTR)(OldBase) + (ULONG_PTR)(NewBase)))

#define PAGE_SIZE 0x1000

#define PH_LARGE_BUFFER_SIZE (16 * 1024 * 1024)

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

// Time

#define PH_TICKS_PER_NS ((LONG64)1 * 10)
#define PH_TICKS_PER_MS (PH_TICKS_PER_NS * 1000)
#define PH_TICKS_PER_SEC (PH_TICKS_PER_MS * 1000)
#define PH_TICKS_PER_MIN (PH_TICKS_PER_SEC * 60)
#define PH_TICKS_PER_HOUR (PH_TICKS_PER_MIN * 60)
#define PH_TICKS_PER_DAY (PH_TICKS_PER_HOUR * 24)

#define PH_TICKS_PARTIAL_MS(Ticks) (((ULONG64)(Ticks) / PH_TICKS_PER_MS) % 1000)
#define PH_TICKS_PARTIAL_SEC(Ticks) (((ULONG64)(Ticks) / PH_TICKS_PER_SEC) % 60)
#define PH_TICKS_PARTIAL_MIN(Ticks) (((ULONG64)(Ticks) / PH_TICKS_PER_MIN) % 60)
#define PH_TICKS_PARTIAL_HOURS(Ticks) (((ULONG64)(Ticks) / PH_TICKS_PER_HOUR) % 24)
#define PH_TICKS_PARTIAL_DAYS(Ticks) ((ULONG64)(Ticks) / PH_TICKS_PER_DAY)

#define PH_TIMEOUT_MS PH_TICKS_PER_MS
#define PH_TIMEOUT_SEC PH_TICKS_PER_SEC

// Annotations

/**
 * Indicates that a function assumes the relevant
 * locks have been acquired.
 */
#define __assumeLocked

/**
 * Indicates that a function assumes the specified
 * number of references are available for the object.
 *
 * \remarks Usually functions reference objects if they
 * store them for later usage; this annotation specifies
 * that the caller must supply these extra references
 * itself. In effect these references are "transferred"
 * to the function and must not be used. E.g. if you
 * create an object and immediately call a function
 * with __assumeRefs(1), you may no longer use the object
 * since that one reference you held is no longer yours.
 */
#define __assumeRefs(count)

/**
 * Indicates that a function may raise a software
 * exception.
 *
 * \remarks Do not use this annotation for
 * temporary usages of exceptions, e.g. unimplemented
 * functions.
 */
#define __mayRaise

/**
 * Indicates that a function requires the specified
 * value to be aligned at the specified number of bytes.
 */
#define __needsAlign(align)

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
    __in LONG Result,
    __in PH_SORT_ORDER Order
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
    __in signed char value1,
    __in signed char value2
    )
{
    return C_1sTo4(value1 - value2);
}

FORCEINLINE int ucharcmp(
    __in unsigned char value1,
    __in unsigned char value2
    )
{
    PH_BUILTIN_COMPARE(value1, value2);
}

FORCEINLINE int shortcmp(
    __in signed short value1,
    __in signed short value2
    )
{
    return C_2sTo4(value1 - value2);
}

FORCEINLINE int ushortcmp(
    __in unsigned short value1,
    __in unsigned short value2
    )
{
    PH_BUILTIN_COMPARE(value1, value2);
}

FORCEINLINE int intcmp(
    __in int value1,
    __in int value2
    )
{
    return value1 - value2;
}

FORCEINLINE int uintcmp(
    __in unsigned int value1,
    __in unsigned int value2
    )
{
    PH_BUILTIN_COMPARE(value1, value2);
}

FORCEINLINE int int64cmp(
    __in __int64 value1,
    __in __int64 value2
    )
{
    PH_BUILTIN_COMPARE(value1, value2);
}

FORCEINLINE int uint64cmp(
    __in unsigned __int64 value1,
    __in unsigned __int64 value2
    )
{
    PH_BUILTIN_COMPARE(value1, value2);
}

FORCEINLINE int intptrcmp(
    __in LONG_PTR value1,
    __in LONG_PTR value2
    )
{
    PH_BUILTIN_COMPARE(value1, value2);
}

FORCEINLINE int uintptrcmp(
    __in ULONG_PTR value1,
    __in ULONG_PTR value2
    )
{
    PH_BUILTIN_COMPARE(value1, value2);
}

FORCEINLINE int singlecmp(
    __in float value1,
    __in float value2
    )
{
    PH_BUILTIN_COMPARE(value1, value2);
}

FORCEINLINE int doublecmp(
    __in double value1,
    __in double value2
    )
{
    PH_BUILTIN_COMPARE(value1, value2);
}

FORCEINLINE int wcsicmp2(
    __in_opt PWSTR Value1,
    __in_opt PWSTR Value2
    )
{
    if (Value1 && Value2)
        return wcsicmp(Value1, Value2);
    else if (!Value1)
        return !Value2 ? 0 : -1;
    else
        return 1;
}

// Synchronization

#ifdef _M_IX86

FORCEINLINE void *_InterlockedCompareExchangePointer(
    void *volatile *Destination,
    void *Exchange,
    void *Comparand
    )
{
    return (PVOID)_InterlockedCompareExchange(
        (PLONG_PTR)Destination,
        (LONG_PTR)Exchange,
        (LONG_PTR)Comparand
        );
}

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

FORCEINLINE LONG_PTR _InterlockedExchangeAddPointer(
    __inout LONG_PTR volatile *Addend,
    __in LONG_PTR Value
    )
{
#ifdef _M_IX86
    return (LONG_PTR)_InterlockedExchangeAdd((PLONG)Addend, (LONG)Value);
#else
    return (LONG_PTR)_InterlockedExchangeAdd64((PLONG64)Addend, (LONG64)Value);
#endif
}

FORCEINLINE LONG_PTR _InterlockedIncrementPointer(
    __inout LONG_PTR volatile *Addend
    )
{
#ifdef _M_IX86
    return (LONG_PTR)_InterlockedIncrement((PLONG)Addend);
#else
    return (LONG_PTR)_InterlockedIncrement64((PLONG64)Addend);
#endif
}

FORCEINLINE LONG_PTR _InterlockedDecrementPointer(
    __inout LONG_PTR volatile *Addend
    )
{
#ifdef _M_IX86
    return (LONG_PTR)_InterlockedDecrement((PLONG)Addend);
#else
    return (LONG_PTR)_InterlockedDecrement64((PLONG64)Addend);
#endif
}

FORCEINLINE BOOLEAN _InterlockedBitTestAndResetPointer(
    __inout LONG_PTR volatile *Base,
    __in LONG_PTR Bit
    )
{
#ifdef _M_IX86
    return _interlockedbittestandreset((PLONG)Base, (LONG)Bit);
#else
    return _interlockedbittestandreset64((PLONG64)Base, (LONG64)Bit);
#endif
}

FORCEINLINE BOOLEAN _InterlockedBitTestAndSetPointer(
    __inout LONG_PTR volatile *Base,
    __in LONG_PTR Bit
    )
{
#ifdef _M_IX86
    return _interlockedbittestandset((PLONG)Base, (LONG)Bit);
#else
    return _interlockedbittestandset64((PLONG64)Base, (LONG64)Bit);
#endif
}

FORCEINLINE BOOLEAN _InterlockedIncrementNoZero(
    __inout LONG volatile *Addend
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

// Strings

#define PH_INT32_STR_LEN 12
#define PH_INT32_STR_LEN_1 (PH_INT32_STR_LEN + 1)

#define PH_INT64_STR_LEN 50
#define PH_INT64_STR_LEN_1 (PH_INT64_STR_LEN + 1)

#define PH_PTR_STR_LEN 24
#define PH_PTR_STR_LEN_1 (PH_PTR_STR_LEN + 1)

#define STR_EQUAL(Str1, Str2) (strcmp(Str1, Str2) == 0)
#define WSTR_EQUAL(Str1, Str2) (wcscmp(Str1, Str2) == 0)

#define STR_IEQUAL(Str1, Str2) (stricmp(Str1, Str2) == 0)
#define WSTR_IEQUAL(Str1, Str2) (wcsicmp(Str1, Str2) == 0)

FORCEINLINE VOID PhPrintInt32(
    __out_ecount(PH_INT32_STR_LEN_1) PWSTR Destination,
    __in LONG Int32
    )
{
    _ltow(Int32, Destination, 10);
}

FORCEINLINE VOID PhPrintUInt32(
    __out_ecount(PH_INT32_STR_LEN_1) PWSTR Destination,
    __in ULONG UInt32
    )
{
    _ultow(UInt32, Destination, 10);
}

FORCEINLINE VOID PhPrintInt64(
    __out_ecount(PH_INT64_STR_LEN_1) PWSTR Destination,
    __in LONG64 Int64
    )
{
    _i64tow(Int64, Destination, 10);
}

FORCEINLINE VOID PhPrintUInt64(
    __out_ecount(PH_INT64_STR_LEN_1) PWSTR Destination,
    __in ULONG64 UInt64
    )
{
    _ui64tow(UInt64, Destination, 10);
}

FORCEINLINE VOID PhPrintPointer(
    __out_ecount(PH_PTR_STR_LEN_1) PWSTR Destination,
    __in PVOID Pointer
    )
{
    Destination[0] = '0';
    Destination[1] = 'x';
#ifdef _M_IX86
    _ultow((ULONG)Pointer, &Destination[2], 16);
#else
    _ui64tow((ULONG64)Pointer, &Destination[2], 16);
#endif
}

// Misc.

FORCEINLINE NTSTATUS PhGetLastWin32ErrorAsNtStatus()
{
    ULONG win32Result;

    // This is needed because NTSTATUS_FROM_WIN32 uses the argument multiple times.
    win32Result = GetLastError();

    return NTSTATUS_FROM_WIN32(win32Result);
}

FORCEINLINE ULONG PhCountBits(
    __in ULONG Value
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

FORCEINLINE ULONG PhRoundNumber(
    __in ULONG Value,
    __in ULONG Multiplier
    )
{
    ULONG newValue;

    newValue = Value / Multiplier * Multiplier;

    // This new value has the multiplier truncated.
    // E.g. 1099 / 100 * 100 = 1000.
    // If the difference is less than half the multiplier,
    // use the new value.
    // E.g.
    // 1099 -> 1000 (100). 1099 - 1000 >= 50, so use
    // the new value plus the multiplier.
    // 1010 -> 1000 (100). 1010 - 1000 < 50, so use
    // the new value.

    if (Value - newValue < Multiplier / 2)
        return newValue;
    else
        return newValue + Multiplier;
}

FORCEINLINE PVOID PhGetProcAddress(
    __in PWSTR LibraryName,
    __in PSTR ProcName
    )
{
    HMODULE module;

    module = GetModuleHandle(LibraryName);

    if (module)
        return GetProcAddress(module, ProcName);
    else
        return NULL;
}

FORCEINLINE VOID PhProbeAddress(
    __in PVOID UserAddress,
    __in SIZE_T UserLength,
    __in PVOID BufferAddress,
    __in SIZE_T BufferLength,
    __in ULONG Alignment
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
    __out PLARGE_INTEGER Timeout,
    __in ULONG Milliseconds
    )
{
    if (Milliseconds == INFINITE)
        return NULL;

    Timeout->QuadPart = -(LONGLONG)UInt32x32To64(Milliseconds, PH_TIMEOUT_MS);

    return Timeout;
}

#endif
