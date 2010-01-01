#ifndef BASE_H
#define BASE_H

#ifndef UNICODE
#define UNICODE
#endif

#include <ntwin.h>
#include <intrin.h>
#include <wchar.h>

// nonstandard extension used : nameless struct/union
#pragma warning(disable: 4201)
// nonstandard extension used : bit field types other than int
#pragma warning(disable: 4214)
// 'function': was declared deprecated
#pragma warning(disable: 4996)

#define PTR_ADD_OFFSET(Pointer, Offset) ((PVOID)((ULONG_PTR)(Pointer) + (ULONG_PTR)(Offset)))
#define REBASE_ADDRESS(Pointer, OldBase, NewBase) \
    ((PVOID)((ULONG_PTR)(Pointer) - (ULONG_PTR)(OldBase) + (ULONG_PTR)(NewBase)))

#define WCHAR_LONG_TO_SHORT(Long) (((Long) & 0xff) | (((Long) & 0xff0000) >> 16))

#define PH_TIMEOUT_TO_MS ((LONGLONG)1 * 10 * 1000)
#define PH_TIMEOUT_TO_SEC (PH_TIMEOUT_TO_MS * 1000)

#define PhRaiseStatus(Status) RaiseException(Status, 0, 0, NULL)

#define PH_INT_STR_LEN 10
#define PH_INT_STR_LEN_1 (PH_INT_STR_LEN + 1)

#define PH_PTR_STR_LEN 24
#define PH_PTR_STR_LEN_1 (PH_PTR_STR_LEN + 1)

FORCEINLINE VOID PhPrintInteger(
    __out PWSTR Destination,
    __in ULONG Integer
    )
{
    _snwprintf(Destination, PH_INT_STR_LEN, L"%d", Integer);
}

FORCEINLINE VOID PhPrintPointer(
    __out PWSTR Destination,
    __in PVOID Pointer
    )
{
    _snwprintf(Destination, PH_PTR_STR_LEN, L"0x%p", Pointer);
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

#endif
