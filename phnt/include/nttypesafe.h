/*
 * Integer literal/constant support macros for typesafe integer types.
 *
 * This file is part of System Informer.
 */

//
// Typed integer literal macros with MSVC compiler extensions and C99 portable fallbacks.
//
// MSVC:        Employs Microsoft integer literal suffixes (i8/ui8, i16/ui16, i32/ui32,
//              i64/ui64, i128/ui128) to enforce type correctness at compile time.
//              Example: UINT16_C(42) produces a uint16_t constant.
//
// Non-MSVC:    Standard typed literal syntax unavailable. Macros are pass-through for
//              smaller types (relying on type inference) or use standard C suffixes
//              (U, ULL, LL) for larger types. Trade-off: reduced type checking; casts
//              may suppress compile-time diagnostics for overflow/truncation.
//
// Naming:      "_C" suffix conforms to C99 conventions (INT32_C, UINT64_C, etc.)
//
// Organization (sorted by size, then logical grouping):
//
// 1. Boolean type                    - (1 byte) (C89/C99)
// 2. Character types                 - (1-2 bytes) (C89/C99)
// 3. Short types                     - (2 bytes) (C89/C99)
// 4. Half-pointer types              - (2-4 bytes) (Windows-specific)
// 5. Int types                       - (4 bytes) (C89/C99)
// 6. Long types                      - (4-8 bytes) (C89/C99)
// 7. Floating-point types            - (4-8 bytes) (C89/C99)
// 8. NT cardinal types               - (2-4 bytes) (Windows NT-specific)
// 9. Pointer-sized types             - (4-8 bytes) (C99 + platform-dependent)
// 10. Standard C99 fixed-width types - (1-8 bytes) (C99 core)
// 11. Windows-specific types         - (1-8 bytes) (Windows API)
// 12. 128-bit integer types          - (16 bytes) (MSVC extension)

#ifndef _NTTYPESAFE_H_INCLUDED_
#define _NTTYPESAFE_H_INCLUDED_

#include <stdint.h>

//
// Boolean type - (1 byte) (C89/C99)
//

#ifndef BOOLEAN_C
#if defined(_MSC_VER)
#define BOOLEAN_C(x)  (x##ui8)
#else
#define BOOLEAN_C(x)  (x)
#endif
#endif

//
// Character types - (1-2 bytes) (C89/C99)
//

#ifndef CHAR_C
#if defined(_MSC_VER)
#define CHAR_C(x)    (x##i8)
#else
#define CHAR_C(x)    (x)
#endif
#endif

#ifndef CCHAR_C
#if defined(_MSC_VER)
#define CCHAR_C(x)   (x##i8)
#else
#define CCHAR_C(x)   (x)
#endif
#endif

#ifndef UCHAR_C
#if defined(_MSC_VER)
#define UCHAR_C(x)   (x##ui8)
#else
#define UCHAR_C(x)   (x)
#endif
#endif

#ifndef WCHAR_C
#if defined(_MSC_VER)
#define WCHAR_C(x)   (x##ui16)
#else
#define WCHAR_C(x)   (x)
#endif
#endif

//
// Short types - (2 bytes) (C89/C99)
//

#ifndef SHORT_C
#if defined(_MSC_VER)
#define SHORT_C(x)   (x##i16)
#else
#define SHORT_C(x)   (x)
#endif
#endif

#ifndef USHORT_C
#if defined(_MSC_VER)
#define USHORT_C(x)  (x##ui16)
#else
#define USHORT_C(x)  (x)
#endif
#endif

#ifndef WORD_C
#if defined(_MSC_VER)
#define WORD_C(x)    (x##ui16)
#else
#define WORD_C(x)    (x)
#endif
#endif

//
// Half-pointer types - (2-4 bytes) (Windows-specific)
// 32-bit on 64-bit platforms, 16-bit on 32-bit platforms
//

#ifndef HALF_PTR_C
#ifdef _WIN64
#if defined(_MSC_VER)
#define HALF_PTR_C(x) (x##i32)
#else
#define HALF_PTR_C(x) (x)
#endif
#else
#if defined(_MSC_VER)
#define HALF_PTR_C(x) (x##i16)
#else
#define HALF_PTR_C(x) (x)
#endif
#endif
#endif

#ifndef UHALF_PTR_C
#ifdef _WIN64
#if defined(_MSC_VER)
#define UHALF_PTR_C(x) (x##ui32)
#else
#define UHALF_PTR_C(x) (x##U)
#endif
#else
#if defined(_MSC_VER)
#define UHALF_PTR_C(x) (x##ui16)
#else
#define UHALF_PTR_C(x) (x)
#endif
#endif
#endif

#ifndef PHALF_PTR_C
#ifdef _WIN64
#if defined(_MSC_VER)
#define PHALF_PTR_C(x) (x##i32)
#else
#define PHALF_PTR_C(x) (x)
#endif
#else
#if defined(_MSC_VER)
#define PHALF_PTR_C(x) (x##i16)
#else
#define PHALF_PTR_C(x) (x)
#endif
#endif
#endif

#ifndef PUHALF_PTR_C
#ifdef _WIN64
#if defined(_MSC_VER)
#define PUHALF_PTR_C(x) (x##ui32)
#else
#define PUHALF_PTR_C(x) (x##U)
#endif
#else
#if defined(_MSC_VER)
#define PUHALF_PTR_C(x) (x##ui16)
#else
#define PUHALF_PTR_C(x) (x)
#endif
#endif
#endif

//
// Int types - (4 bytes) (C89/C99)
//

#ifndef INT_C
#if defined(_MSC_VER)
#define INT_C(x)     (x##i32)
#else
#define INT_C(x)     (x)
#endif
#endif

#ifndef UINT_C
#if defined(_MSC_VER)
#define UINT_C(x)    (x##ui32)
#else
#define UINT_C(x)    (x##U)
#endif
#endif

//
// Long types - (4-8 bytes) (C89/C99)
//

#ifndef LONG_C
#if defined(_MSC_VER)
#define LONG_C(x)    (x##i32)
#else
#define LONG_C(x)    (x)
#endif
#endif

#ifndef LONG32_C
#if defined(_MSC_VER)
#define LONG32_C(x)  (x##i32)
#else
#define LONG32_C(x)  (x)
#endif
#endif

#ifndef LONG64_C
#if defined(_MSC_VER)
#define LONG64_C(x)  (x##i64)
#else
#define LONG64_C(x)  (x##LL)
#endif
#endif

#ifndef LONGLONG_C
#if defined(_MSC_VER)
#define LONGLONG_C(x) (x##i64)
#else
#define LONGLONG_C(x) (x##LL)
#endif
#endif

#ifndef ULONG_C
#if defined(_MSC_VER)
#define ULONG_C(x)   (x##ui32)
#else
#define ULONG_C(x)   (x##U)
#endif
#endif

#ifndef ULONG32_C
#if defined(_MSC_VER)
#define ULONG32_C(x) (x##ui32)
#else
#define ULONG32_C(x) (x##U)
#endif
#endif

#ifndef ULONG64_C
#if defined(_MSC_VER)
#define ULONG64_C(x) (x##ui64)
#else
#define ULONG64_C(x) (x##ULL)
#endif
#endif

#ifndef ULONGLONG_C
#if defined(_MSC_VER)
#define ULONGLONG_C(x) (x##ui64)
#else
#define ULONGLONG_C(x) (x##ULL)
#endif
#endif

//
// Floating-point types - (4-8 bytes) (C89/C99)
//

#ifndef FLOAT_C
#define FLOAT_C(x)   (x##f)
#endif

#ifndef DOUBLE_C
#if defined(_MSC_VER)
#define DOUBLE_C(x)  (x##.0)
#else
#define DOUBLE_C(x)  (x)
#endif
#endif

//
// NT cardinal types - (2-4 bytes) (Windows NT-specific)
// Counted/cardinal types from NT native subsystem with 'C' prefix
//

#ifndef CSHORT_C
#if defined(_MSC_VER)
#define CSHORT_C(x)  (x##i16)
#else
#define CSHORT_C(x)  (x)
#endif
#endif

#ifndef CLONG_C
#if defined(_MSC_VER)
#define CLONG_C(x)   (x##ui32)
#else
#define CLONG_C(x)   (x##U)
#endif
#endif

#ifndef LOGICAL_C
#if defined(_MSC_VER)
#define LOGICAL_C(x) (x##ui32)
#else
#define LOGICAL_C(x) (x##U)
#endif
#endif

//
// Pointer-sized types - (4-8 bytes) (C99 + platform-dependent)
// Vary between 32-bit and 64-bit platforms
//

#ifndef INT_PTR_C
#ifdef _WIN64
#if defined(_MSC_VER)
#define INT_PTR_C(x) (x##i64)
#else
#define INT_PTR_C(x) (x##LL)
#endif
#else
#if defined(_MSC_VER)
#define INT_PTR_C(x) (x##i32)
#else
#define INT_PTR_C(x) (x)
#endif
#endif
#endif

#ifndef UINT_PTR_C
#ifdef _WIN64
#if defined(_MSC_VER)
#define UINT_PTR_C(x) (x##ui64)
#else
#define UINT_PTR_C(x) (x##ULL)
#endif
#else
#if defined(_MSC_VER)
#define UINT_PTR_C(x) (x##ui32)
#else
#define UINT_PTR_C(x) (x##U)
#endif
#endif
#endif

#ifndef LONG_PTR_C
#ifdef _WIN64
#if defined(_MSC_VER)
#define LONG_PTR_C(x) (x##i64)
#else
#define LONG_PTR_C(x) (x##LL)
#endif
#else
#if defined(_MSC_VER)
#define LONG_PTR_C(x) (x##i32)
#else
#define LONG_PTR_C(x) (x)
#endif
#endif
#endif

#ifndef ULONG_PTR_C
#ifdef _WIN64
#if defined(_MSC_VER)
#define ULONG_PTR_C(x) (x##ui64)
#else
#define ULONG_PTR_C(x) (x##ULL)
#endif
#else
#if defined(_MSC_VER)
#define ULONG_PTR_C(x) (x##ui32)
#else
#define ULONG_PTR_C(x) (x##U)
#endif
#endif
#endif

#ifndef SIZE_T_C
#ifdef _WIN64
#if defined(_MSC_VER)
#define SIZE_T_C(x) (x##ui64)
#else
#define SIZE_T_C(x) (x##ULL)
#endif
#else
#if defined(_MSC_VER)
#define SIZE_T_C(x) (x##ui32)
#else
#define SIZE_T_C(x) (x##U)
#endif
#endif
#endif

#ifndef SSIZE_T_C
#ifdef _WIN64
#if defined(_MSC_VER)
#define SSIZE_T_C(x) (x##i64)
#else
#define SSIZE_T_C(x) (x##LL)
#endif
#else
#if defined(_MSC_VER)
#define SSIZE_T_C(x) (x##i32)
#else
#define SSIZE_T_C(x) (x)
#endif
#endif
#endif

#ifndef PTRDIFF_T_C
#ifdef _WIN64
#if defined(_MSC_VER)
#define PTRDIFF_T_C(x) (x##i64)
#else
#define PTRDIFF_T_C(x) (x##LL)
#endif
#else
#if defined(_MSC_VER)
#define PTRDIFF_T_C(x) (x##i32)
#else
#define PTRDIFF_T_C(x) (x)
#endif
#endif
#endif

//
// Standard C99 fixed-width integer types - (1-8 bytes) (C99 core)
// Redefine with MSVC extensions for better type safety
//

#undef INT8_C
#undef UINT8_C
#undef INT16_C
#undef UINT16_C
#undef INT32_C
#undef UINT32_C
#undef INT64_C
#undef UINT64_C
#undef INTMAX_C
#undef UINTMAX_C

#if defined(_MSC_VER)
#define INT8_C(x)    (x##i8)
#define UINT8_C(x)   (x##ui8)
#define INT16_C(x)   (x##i16)
#define UINT16_C(x)  (x##ui16)
#define INT32_C(x)   (x##i32)
#define UINT32_C(x)  (x##ui32)
#define INT64_C(x)   (x##i64)
#define UINT64_C(x)  (x##ui64)
#define INTMAX_C(x)  (x##i64)
#define UINTMAX_C(x) (x##ui64)
#else
#define INT8_C(x)    (x)
#define UINT8_C(x)   (x)
#define INT16_C(x)   (x)
#define UINT16_C(x)  (x)
#define INT32_C(x)   (x)
#define UINT32_C(x)  (x##U)
#define INT64_C(x)   (x##LL)
#define UINT64_C(x)  (x##ULL)
#define INTMAX_C(x)  (x##LL)
#define UINTMAX_C(x) (x##ULL)
#endif

//
// Windows-specific types - (1-8 bytes) (Windows API)
//

#ifndef BYTE_C
#if defined(_MSC_VER)
#define BYTE_C(x)    (x##ui8)
#else
#define BYTE_C(x)    (x)
#endif
#endif

#ifndef DWORD_C
#if defined(_MSC_VER)
#define DWORD_C(x)   (x##ui32)
#else
#define DWORD_C(x)   (x##U)
#endif
#endif

#ifndef DWORD32_C
#if defined(_MSC_VER)
#define DWORD32_C(x) (x##ui32)
#else
#define DWORD32_C(x) (x##U)
#endif
#endif

#ifndef DWORD64_C
#if defined(_MSC_VER)
#define DWORD64_C(x) (x##ui64)
#else
#define DWORD64_C(x) (x##ULL)
#endif
#endif

#ifndef DWORDLONG_C
#if defined(_MSC_VER)
#define DWORDLONG_C(x) (x##ui64)
#else
#define DWORDLONG_C(x) (x##ULL)
#endif
#endif

#ifndef DWORD_PTR_C
#ifdef _WIN64
#if defined(_MSC_VER)
#define DWORD_PTR_C(x) (x##ui64)
#else
#define DWORD_PTR_C(x) (x##ULL)
#endif
#else
#if defined(_MSC_VER)
#define DWORD_PTR_C(x) (x##ui32)
#else
#define DWORD_PTR_C(x) (x##U)
#endif
#endif
#endif

#ifndef QWORD_C
#if defined(_MSC_VER)
#define QWORD_C(x)   (x##ui64)
#else
#define QWORD_C(x)   (x##ULL)
#endif
#endif

//
// 128-bit integer types - (16 bytes) (MSVC extension)
//

#ifndef INT128_C
#if defined(_MSC_VER)
#define INT128_C(x)  (x##i128)
#else
// 128-bit literals not supported on non-MSVC, use cast as fallback
#define INT128_C(x)  ((__int128)(x##LL))
#endif
#endif

#ifndef UINT128_C
#if defined(_MSC_VER)
#define UINT128_C(x) (x##ui128)
#else
// 128-bit literals not supported on non-MSVC, use cast as fallback
#define UINT128_C(x) ((unsigned __int128)(x##ULL))
#endif
#endif

#endif /* _NTTYPESAFE_H_INCLUDED_ */
