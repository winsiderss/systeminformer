#ifndef BASE_H
#define BASE_H

#ifndef UNICODE
#define UNICODE
#endif

#include <ntbasic.h>
#include <intrin.h>
#include <wchar.h>
#include <assert.h>
#include <stdio.h>

// nonstandard extension used : nameless struct/union
#pragma warning(disable: 4201)
// nonstandard extension used : bit field types other than int
#pragma warning(disable: 4214)
// 'function': attributes not present on previous declaration
#pragma warning(disable: 4985)
// 'function': was declared deprecated
#pragma warning(disable: 4996)

#define FASTCALL __fastcall

#define SIMPLE_EXCEPTION_FILTER(Condition) \
    ((Condition) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)

#define PTR_ADD_OFFSET(Pointer, Offset) ((PVOID)((ULONG_PTR)(Pointer) + (ULONG_PTR)(Offset)))
#define PTR_SUB_OFFSET(Pointer, Offset) ((PVOID)((ULONG_PTR)(Pointer) - (ULONG_PTR)(Offset)))
#define REBASE_ADDRESS(Pointer, OldBase, NewBase) \
    ((PVOID)((ULONG_PTR)(Pointer) - (ULONG_PTR)(OldBase) + (ULONG_PTR)(NewBase)))

#define PAGE_SIZE 0x1000

#define WCHAR_LONG_TO_SHORT(Long) (((Long) & 0xff) | (((Long) & 0xff0000) >> 16))

#define PH_TICKS_PER_NS ((LONGLONG)1 * 10)
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

#define DPCS_PROCESS_ID ((HANDLE)-2)
#define INTERRUPTS_PROCESS_ID ((HANDLE)-3)

#define PH_LARGE_BUFFER_SIZE (16 * 1024 * 1024)

#define PhRaiseStatus(Status) RaiseException(Status, 0, 0, NULL)

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

// Sorting

typedef enum _PH_SORT_ORDER
{
    AscendingSortOrder,
    DescendingSortOrder,
    NoSortOrder
} PH_SORT_ORDER, *PPH_SORT_ORDER;

FORCEINLINE INT PhModifySort(
    __in INT Result,
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

FORCEINLINE int intcmp(
    __in int value1,
    __in int value2
    )
{
    PH_BUILTIN_COMPARE(value1, value2);
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

FORCEINLINE int uintptrcmp(
    __in ULONG_PTR value1,
    __in ULONG_PTR value2
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

// Misc.

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
    return GetProcAddress(GetModuleHandle(LibraryName), ProcName);
}

FORCEINLINE VOID PhPrintInt32(
    __out_ecount(PH_INT32_STR_LEN_1) PWSTR Destination,
    __in LONG Int32
    )
{
    _snwprintf(Destination, PH_INT32_STR_LEN, L"%d", Int32);
}

FORCEINLINE VOID PhPrintUInt32(
    __out_ecount(PH_INT32_STR_LEN_1) PWSTR Destination,
    __in ULONG UInt32
    )
{
    _snwprintf(Destination, PH_INT32_STR_LEN, L"%u", UInt32);
}

FORCEINLINE VOID PhPrintInt64(
    __out_ecount(PH_INT64_STR_LEN_1) PWSTR Destination,
    __in LONG64 Int64
    )
{
    _snwprintf(Destination, PH_INT64_STR_LEN, L"%I64d", Int64);
}

FORCEINLINE VOID PhPrintUInt64(
    __out_ecount(PH_INT64_STR_LEN_1) PWSTR Destination,
    __in ULONG64 UInt64
    )
{
    _snwprintf(Destination, PH_INT64_STR_LEN, L"%I64u", UInt64);
}

FORCEINLINE VOID PhPrintPointer(
    __out_ecount(PH_PTR_STR_LEN_1) PWSTR Destination,
    __in PVOID Pointer
    )
{
    _snwprintf(Destination, PH_PTR_STR_LEN, L"0x%Ix", Pointer);
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
            (((ULONG_PTR)UserAddress + UserLength) < (ULONG_PTR)UserAddress) ||
            (((ULONG_PTR)UserAddress + UserLength) > ((ULONG_PTR)BufferAddress + BufferLength))
            )
            PhRaiseStatus(STATUS_ACCESS_VIOLATION);
    }
}

#ifdef _M_IX86

FORCEINLINE PVOID _InterlockedCompareExchangePointer(
    __inout PVOID volatile *Destination,
    __in PVOID Exchange,
    __in PVOID Comparand
    )
{
    return (PVOID)_InterlockedCompareExchange(
        (PLONG_PTR)Destination,
        (LONG_PTR)Exchange,
        (LONG_PTR)Comparand
        );
}

FORCEINLINE PVOID _InterlockedExchangePointer(
    __inout PVOID volatile *Destination,
    __in PVOID Exchange
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

#endif
