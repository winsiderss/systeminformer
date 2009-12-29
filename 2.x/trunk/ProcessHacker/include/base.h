#ifndef BASE_H
#define BASE_H

#ifndef UNICODE
#define UNICODE
#endif

#include <ntwin.h>

// We don't care about "deprecation".
#pragma warning(disable: 4996)

#define PH_INT_STR_LEN 10
#define PH_INT_STR_LEN_1 (PH_INT_STR_LEN + 1)

#define PTR_ADD_OFFSET(Pointer, Offset) ((PVOID)((ULONG_PTR)(Pointer) + (ULONG_PTR)(Offset)))

#define WCHAR_LONG_TO_SHORT(Long) (((Long) & 0xff) | (((Long) & 0xff0000) >> 16))

#define PH_TIMEOUT_TO_MS ((LONGLONG)1 * 10 * 1000)
#define PH_TIMEOUT_TO_SEC (PH_TIMEOUT_TO_MS * 1000)

#define PhRaiseStatus(Status) RaiseException(Status, 0, 0, NULL)

#endif
