/******************************************************************
*                                                                 *
*  ntstrsafe.h -- This module defines safer C library string      *
*                 routine replacements for drivers. These are     *
*                 meant to make C a bit more safe in reference    *
*                 to security and robustness. A similar file,     *
*                 strsafe.h, is available for applications.       *
*                                                                 *
*  Copyright (c) Microsoft Corp.  All rights reserved.            *
*                                                                 *
******************************************************************/
#ifndef _NTSTRSAFE_H_INCLUDED_
#define _NTSTRSAFE_H_INCLUDED_
#if (_MSC_VER > 1000)
#pragma once
#endif


#include <stdio.h>          // for _vsnprintf, _vsnwprintf, getc, getwc
#include <string.h>         // for memset
#include <stdarg.h>         // for va_start, etc.
#include <specstrings.h>    // for _In_, etc.
#include <winapifamily.h>   // for WINAPI_FAMILY_PARTITION()

#ifndef NTSTRSAFE_NO_UNICODE_STRING_FUNCTIONS
#include <ntdef.h>          // for UNICODE_STRING, etc.
#endif

#if !defined(_W64)
#if !defined(__midl) && (defined(_X86_) || defined(_M_IX86) || defined(_ARM_) || defined(_M_ARM)) && (_MSC_VER >= 1300)
#define _W64 __w64
#else
#define _W64
#endif
#endif

#if defined(_M_MRX000) || defined(_M_ALPHA) || defined(_M_PPC) || defined(_M_IA64) || defined(_M_AMD64) || defined(_M_ARM) || defined(_M_ARM64)
#define ALIGNMENT_MACHINE
#define UNALIGNED __unaligned
#if defined(_WIN64)
#define UNALIGNED64 __unaligned
#else
#define UNALIGNED64
#endif
#else
#undef ALIGNMENT_MACHINE
#define UNALIGNED
#define UNALIGNED64
#endif

// typedefs
#ifdef  _WIN64
typedef unsigned __int64    size_t;
#else
typedef _W64 unsigned int   size_t;
#endif

#ifndef _NTSTATUS_DEFINED
#define _NTSTATUS_DEFINED
typedef _Return_type_success_(return >= 0) long NTSTATUS;
#endif

typedef unsigned long DWORD;


#ifndef SORTPP_PASS
// compiletime asserts (failure results in error C2118: negative subscript)
#define C_ASSERT(e) typedef char __C_ASSERT__[(e)?1:-1]
#else
#define C_ASSERT(e)
#endif

#ifdef __cplusplus
#define EXTERN_C    extern "C"
#else
#define EXTERN_C    extern
#endif

// use the new secure crt functions if available
#ifndef NTSTRSAFE_USE_SECURE_CRT
#if defined(__GOT_SECURE_LIB__) && (__GOT_SECURE_LIB__ >= 200402L)
#define NTSTRSAFE_USE_SECURE_CRT 0
#else
#define NTSTRSAFE_USE_SECURE_CRT 0
#endif
#endif  // !NTSTRSAFE_USE_SECURE_CRT

#ifdef _M_CEE_PURE
#define NTSTRSAFEDDI      __inline NTSTATUS __clrcall
#else
#define NTSTRSAFEDDI      __inline NTSTATUS __stdcall
#endif

#if defined(NTSTRSAFE_LIB_IMPL) || defined(NTSTRSAFE_LIB)
#define NTSTRSAFEWORKERDDI    EXTERN_C NTSTATUS __stdcall
#else
#define NTSTRSAFEWORKERDDI    static NTSTRSAFEDDI
#endif

// The following steps are *REQUIRED* if ntstrsafe.h is used for drivers on:
//     Windows 2000
//     Windows Millennium Edition
//     Windows 98 Second Edition
//     Windows 98
//
// 1. #define NTSTRSAFE_LIB before including the ntstrsafe.h header file.
// 2. Add ntstrsafe.lib to the TARGET_LIBS line in SOURCES
//
// Drivers running on XP and later can skip these steps to create a smaller
// driver by running the functions inline.
#if defined(NTSTRSAFE_LIB)
#pragma comment(lib, "ntstrsafe.lib")
#endif

#pragma warning(push)
#pragma warning(disable: 28210) // Because not all PREFast versions like _Always_ equally.

// The user can request no "Cb" or no "Cch" fuctions, but not both
#if defined(NTSTRSAFE_NO_CB_FUNCTIONS) && defined(NTSTRSAFE_NO_CCH_FUNCTIONS)
#error cannot specify both NTSTRSAFE_NO_CB_FUNCTIONS and NTSTRSAFE_NO_CCH_FUNCTIONS !!
#endif

// The user may override NTSTRSAFE_MAX_CCH, but it must always be less than INT_MAX
#ifndef NTSTRSAFE_MAX_CCH
#define NTSTRSAFE_MAX_CCH     2147483647  // max buffer size, in characters, that we support (same as INT_MAX)
#endif
C_ASSERT(NTSTRSAFE_MAX_CCH <= 2147483647);
C_ASSERT(NTSTRSAFE_MAX_CCH > 1);

#define NTSTRSAFE_MAX_LENGTH  (NTSTRSAFE_MAX_CCH - 1)   // max buffer length, in characters, that we support

// The user may override NTSTRSAFE_UNICODE_STRING_MAX_CCH, but it must always be less than (USHORT_MAX / sizeof(wchar_t))
#ifndef NTSTRSAFE_UNICODE_STRING_MAX_CCH
#define NTSTRSAFE_UNICODE_STRING_MAX_CCH    (0xffff / sizeof(wchar_t))  // max buffer size, in characters, for a UNICODE_STRING
#endif
C_ASSERT(NTSTRSAFE_UNICODE_STRING_MAX_CCH <= (0xffff / sizeof(wchar_t)));
C_ASSERT(NTSTRSAFE_UNICODE_STRING_MAX_CCH > 1);


// Flags for controling the Ex functions
//
//      STRSAFE_FILL_BYTE(0xFF)                         0x000000FF  // bottom byte specifies fill pattern
#define STRSAFE_IGNORE_NULLS                            0x00000100  // treat null string pointers as TEXT("") -- don't fault on NULL buffers
#define STRSAFE_FILL_BEHIND_NULL                        0x00000200  // on success, fill in extra space behind the null terminator with fill pattern
#define STRSAFE_FILL_ON_FAILURE                         0x00000400  // on failure, overwrite pszDest with fill pattern and null terminate it
#define STRSAFE_NULL_ON_FAILURE                         0x00000800  // on failure, set *pszDest = TEXT('\0')
#define STRSAFE_NO_TRUNCATION                           0x00001000  // instead of returning a truncated result, copy/append nothing to pszDest and null terminate it

// Flags for controling UNICODE_STRING Ex functions
//
//      STRSAFE_FILL_BYTE(0xFF)                         0x000000FF  // bottom byte specifies fill pattern
//      STRSAFE_IGNORE_NULLS                            0x00000100  // don't fault on NULL UNICODE_STRING pointers, and treat null pszSrc as L""
#define STRSAFE_FILL_BEHIND                             0x00000200  // on success, fill in extra space at the end of the UNICODE_STRING Buffer with fill pattern
//      STRSAFE_FILL_ON_FAILURE                         0x00000400  // on failure, fill the UNICODE_STRING Buffer with fill pattern and set the Length to 0
#define STRSAFE_ZERO_LENGTH_ON_FAILURE                  0x00000800  // on failure, set the UNICODE_STRING Length to 0
//      STRSAFE_NO_TRUNCATION                           0x00001000  // instead of returning a truncated result, copy/append nothing to UNICODE_STRING Buffer


#define STRSAFE_VALID_FLAGS                     (0x000000FF | STRSAFE_IGNORE_NULLS | STRSAFE_FILL_BEHIND_NULL | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION)
#define STRSAFE_UNICODE_STRING_VALID_FLAGS      (0x000000FF | STRSAFE_IGNORE_NULLS | STRSAFE_FILL_BEHIND | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE | STRSAFE_NO_TRUNCATION)

// helper macro to set the fill character and specify buffer filling
#define STRSAFE_FILL_BYTE(x)                    ((DWORD)((x & 0x000000FF) | STRSAFE_FILL_BEHIND_NULL))
#define STRSAFE_FAILURE_BYTE(x)                 ((DWORD)((x & 0x000000FF) | STRSAFE_FILL_ON_FAILURE))

#define STRSAFE_GET_FILL_PATTERN(dwFlags)       ((int)(dwFlags & 0x000000FF))


// Deprecated, use the non STRSAFE_ prefixed types instead (e.g. LPSTR or PSTR) as they are the same as these.
typedef _Null_terminated_ char* NTSTRSAFE_PSTR;
typedef _Null_terminated_ const char* NTSTRSAFE_PCSTR;
typedef _Null_terminated_ wchar_t* NTSTRSAFE_PWSTR;
typedef _Null_terminated_ const wchar_t* NTSTRSAFE_PCWSTR;
typedef _Null_terminated_ const wchar_t UNALIGNED* NTSTRSAFE_PCUWSTR;

// Deprecated, use the base types instead.
// RtlStrings where the string is NOT guaranteed to be null terminated (does not have _Null_terminated_).
typedef  const char* STRSAFE_PCNZCH;
typedef  const wchar_t* STRSAFE_PCNZWCH;
typedef  const wchar_t UNALIGNED* STRSAFE_PCUNZWCH;



// prototypes for the worker functions

NTSTRSAFEWORKERDDI
RtlStringLengthWorkerA(
    _In_reads_or_z_(cchMax) STRSAFE_PCNZCH psz,
    _In_ _In_range_(<=, NTSTRSAFE_MAX_CCH) size_t cchMax,
    _Out_opt_ _Deref_out_range_(<, cchMax) _Deref_out_range_(<=, _String_length_(psz)) size_t* pcchLength);



NTSTRSAFEWORKERDDI
RtlStringLengthWorkerW(
    _In_reads_or_z_(cchMax) STRSAFE_PCNZWCH psz,
    _In_ _In_range_(<=, NTSTRSAFE_MAX_CCH) size_t cchMax,
    _Out_opt_ _Deref_out_range_(<, cchMax) _Deref_out_range_(<=, _String_length_(psz)) size_t* pcchLength);

#ifdef ALIGNMENT_MACHINE
NTSTRSAFEWORKERDDI
RtlUnalignedStringLengthWorkerW(
    _In_reads_or_z_(cchMax) STRSAFE_PCUNZWCH psz,
    _In_ _In_range_(<=, NTSTRSAFE_MAX_CCH) size_t cchMax,
    _Out_opt_ _Deref_out_range_(<, cchMax) size_t* pcchLength);
#endif  // ALIGNMENT_MACHINE



_When_(_Old_(*ppszSrc) != NULL, _Unchanged_(*ppszSrc))
_When_(_Old_(*ppszSrc) == NULL, _At_(*ppszSrc, _Post_z_))
NTSTRSAFEWORKERDDI
RtlStringExValidateSrcA(
    _Inout_ _Deref_post_notnull_ STRSAFE_PCNZCH* ppszSrc,
    _Inout_opt_
        _Deref_out_range_(<, cchMax)
        _Deref_out_range_(<=, _Old_(*pcchToRead)) size_t* pcchToRead,
    _In_ const size_t cchMax,
    _In_ DWORD dwFlags);



_When_(_Old_(*ppszSrc) != NULL, _Unchanged_(*ppszSrc))
_When_(_Old_(*ppszSrc) == NULL, _At_(*ppszSrc, _Post_z_))
NTSTRSAFEWORKERDDI
RtlStringExValidateSrcW(
    _Inout_ _Deref_post_notnull_ STRSAFE_PCNZWCH* ppszSrc,
    _Inout_opt_
        _Deref_out_range_(<, cchMax)
        _Deref_out_range_(<=, _Old_(*pcchToRead)) size_t* pcchToRead,
    _In_ const size_t cchMax,
    _In_ DWORD dwFlags);



_Post_satisfies_(cchDest > 0 && cchDest <= cchMax)
NTSTRSAFEWORKERDDI
RtlStringValidateDestA(
    _In_reads_opt_(cchDest) STRSAFE_PCNZCH pszDest,
    _In_ size_t cchDest,
    _In_ const size_t cchMax);

_Post_satisfies_(cchDest > 0 && cchDest <= cchMax)
NTSTRSAFEWORKERDDI
RtlStringValidateDestAndLengthA(
    _In_reads_opt_(cchDest) NTSTRSAFE_PCSTR pszDest,
    _In_ size_t cchDest,
    _Out_ _Deref_out_range_(0, cchDest - 1) size_t* pcchDestLength,
    _In_ const size_t cchMax);



_Post_satisfies_(cchDest > 0 && cchDest <= cchMax)
NTSTRSAFEWORKERDDI
RtlStringValidateDestW(
    _In_reads_opt_(cchDest) STRSAFE_PCNZWCH pszDest,
    _In_ size_t cchDest,
    _In_ const size_t cchMax);

_Post_satisfies_(cchDest > 0 && cchDest <= cchMax)
NTSTRSAFEWORKERDDI
RtlStringValidateDestAndLengthW(
    _In_reads_opt_(cchDest) NTSTRSAFE_PCWSTR pszDest,
    _In_ size_t cchDest,
    _Out_ _Deref_out_range_(0, cchDest - 1) size_t* pcchDestLength,
    _In_ const size_t cchMax);



NTSTRSAFEWORKERDDI
RtlStringExValidateDestA(
    _In_reads_opt_(cchDest) STRSAFE_PCNZCH pszDest,
    _In_ size_t cchDest,
    _In_ const size_t cchMax,
    _In_ DWORD dwFlags);

NTSTRSAFEWORKERDDI
RtlStringExValidateDestAndLengthA(
    _In_reads_opt_(cchDest) NTSTRSAFE_PCSTR pszDest,
    _In_ size_t cchDest,
    _Out_ _Deref_out_range_(0, (cchDest>0?cchDest-1:0)) size_t* pcchDestLength,
    _In_ const size_t cchMax,
    _In_ DWORD dwFlags);



NTSTRSAFEWORKERDDI
RtlStringExValidateDestW(
    _In_reads_opt_(cchDest) STRSAFE_PCNZWCH pszDest,
    _In_ size_t cchDest,
    _In_ const size_t cchMax,
    _In_ DWORD dwFlags);

NTSTRSAFEWORKERDDI
RtlStringExValidateDestAndLengthW(
    _In_reads_opt_(cchDest) NTSTRSAFE_PCWSTR pszDest,
    _In_ size_t cchDest,
    _Out_ _Deref_out_range_(0, (cchDest>0?cchDest-1:0)) size_t* pcchDestLength,
    _In_ const size_t cchMax,
    _In_ DWORD dwFlags);



NTSTRSAFEWORKERDDI
RtlStringCopyWorkerA(
    _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
    _In_ _In_range_(1, NTSTRSAFE_MAX_CCH) size_t cchDest,
    _Always_(_Out_opt_ _Deref_out_range_(<=, (cchToCopy < cchDest) ? cchToCopy : (cchDest - 1))) size_t* pcchNewDestLength,
    _In_reads_or_z_(cchToCopy) STRSAFE_PCNZCH pszSrc,
    _In_ _In_range_(<, NTSTRSAFE_MAX_CCH) size_t cchToCopy);



NTSTRSAFEWORKERDDI
RtlStringCopyWorkerW(
    _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
    _In_ _In_range_(1, NTSTRSAFE_MAX_CCH) size_t cchDest,
    _Always_(_Out_opt_ _Deref_out_range_(<=, (cchToCopy < cchDest) ? cchToCopy : (cchDest - 1))) size_t* pcchNewDestLength,
    _In_reads_or_z_(cchToCopy) STRSAFE_PCNZWCH pszSrc,
    _In_ _In_range_(<, NTSTRSAFE_MAX_CCH) size_t cchToCopy);



NTSTRSAFEWORKERDDI
RtlStringVPrintfWorkerA(
    _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
    _In_ _In_range_(1, NTSTRSAFE_MAX_CCH) size_t cchDest,
    _Always_(_Out_opt_ _Deref_out_range_(<=, cchDest - 1)) size_t* pcchNewDestLength,
    _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
    _In_ va_list argList);




NTSTRSAFEWORKERDDI
RtlStringVPrintfWorkerW(
        _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
        _In_ _In_range_(1, NTSTRSAFE_MAX_CCH) size_t cchDest,
        _Always_(_Out_opt_ _Deref_out_range_(<=, cchDest - 1)) size_t* pcchNewDestLength,
        _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
        _In_ va_list argList);




NTSTRSAFEWORKERDDI
RtlStringExHandleFillBehindNullA(
        _Inout_updates_bytes_(cbRemaining) NTSTRSAFE_PSTR pszDestEnd,
        _In_ size_t cbRemaining,
        _In_ DWORD dwFlags);



NTSTRSAFEWORKERDDI
RtlStringExHandleFillBehindNullW(
        _Inout_updates_bytes_(cbRemaining) NTSTRSAFE_PWSTR pszDestEnd,
        _In_ size_t cbRemaining,
        _In_ DWORD dwFlags);



_Success_(1)  // always succeeds, no exit tests needed
    NTSTRSAFEWORKERDDI
    RtlStringExHandleOtherFlagsA(
            _Out_writes_bytes_(cbDest) NTSTRSAFE_PSTR pszDest,
            _In_ _In_range_(1, NTSTRSAFE_MAX_CCH * sizeof(char)) size_t cbDest,
            _In_ _In_range_(0, cbDest>sizeof(char)?(cbDest / sizeof(char)) - 1:0) size_t cchOriginalDestLength,
            _Outptr_result_buffer_(*pcchRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
            _Out_ _Deref_out_range_(0, cbDest / sizeof(char)) size_t* pcchRemaining,
            _In_ DWORD dwFlags);



_Success_(1)  // always succeeds, no exit tests needed
    NTSTRSAFEWORKERDDI
    RtlStringExHandleOtherFlagsW(
            _Out_writes_bytes_(cbDest) NTSTRSAFE_PWSTR pszDest,
            _In_ _In_range_(1, NTSTRSAFE_MAX_CCH * sizeof(wchar_t)) size_t cbDest,
            _In_ _In_range_(0, cbDest>sizeof(wchar_t)?(cbDest / sizeof(wchar_t)) - 1:0) size_t cchOriginalDestLength,
            _Outptr_result_buffer_(*pcchRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
            _Out_ _Deref_out_range_(0, cbDest / sizeof(wchar_t)) size_t* pcchRemaining,
            _In_ DWORD dwFlags);




#ifndef NTSTRSAFE_NO_UNICODE_STRING_FUNCTIONS

_At_(DestinationString->Buffer, _Post_equal_to_(pszSrc))
_At_(DestinationString->Length, _Post_equal_to_(_String_length_(pszSrc) * sizeof(WCHAR)))
_At_(DestinationString->MaximumLength, _Post_equal_to_((_String_length_(pszSrc)+1) * sizeof(WCHAR)))
NTSTRSAFEWORKERDDI
RtlUnicodeStringInitWorker(
        _Out_ PUNICODE_STRING DestinationString,
        _In_opt_ NTSTRSAFE_PCWSTR pszSrc,
        _In_ const size_t cchMax,
        _In_ DWORD dwFlags);

NTSTRSAFEWORKERDDI
RtlUnicodeStringValidateWorker(
        _In_opt_ PCUNICODE_STRING SourceString,
        _In_ const size_t cchMax,
        _In_ DWORD dwFlags);

_Post_satisfies_(*pcchSrcLength*sizeof(wchar_t) == SourceString->MaximumLength)
    NTSTRSAFEWORKERDDI
    RtlUnicodeStringValidateSrcWorker(
            _In_ PCUNICODE_STRING SourceString,
            _Outptr_result_buffer_(*pcchSrcLength) wchar_t** ppszSrc,
            _Out_ size_t* pcchSrcLength,
            _In_ const size_t cchMax,
            _In_ DWORD dwFlags);

_Post_satisfies_(*pcchDest*sizeof(wchar_t) == DestinationString->MaximumLength)
    NTSTRSAFEWORKERDDI
    RtlUnicodeStringValidateDestWorker(
            _In_ PCUNICODE_STRING DestinationString,
            _Outptr_result_buffer_(*pcchDest) wchar_t** ppszDest,
            _Out_ size_t* pcchDest,
            _Out_opt_ size_t* pcchDestLength,
            _In_ const size_t cchMax,
            _In_ DWORD dwFlags);

NTSTRSAFEWORKERDDI
RtlStringCopyWideCharArrayWorker(
        _Out_writes_(cchDest) NTSTRSAFE_PWSTR pszDest,
        _In_ size_t cchDest,
        _Out_opt_ size_t* pcchNewDestLength,
        _In_reads_(cchSrcLength) const wchar_t* pszSrc,
        _In_ size_t cchSrcLength);

NTSTRSAFEWORKERDDI
RtlWideCharArrayCopyStringWorker(
        _Out_writes_to_(cchDest, *pcchNewDestLength) wchar_t* pszDest,
        _In_ size_t cchDest,
        _Out_ size_t* pcchNewDestLength,
        _In_ NTSTRSAFE_PCWSTR pszSrc,
        _In_ size_t cchToCopy);

NTSTRSAFEWORKERDDI
RtlWideCharArrayCopyWorker(
        _Out_writes_to_(cchDest, *pcchNewDestLength) wchar_t* pszDest,
        _In_ size_t cchDest,
        _Out_ size_t* pcchNewDestLength,
        _In_reads_(cchSrcLength) const wchar_t* pszSrc,
        _In_ size_t cchSrcLength);

NTSTRSAFEWORKERDDI
RtlWideCharArrayVPrintfWorker(
        _Out_writes_to_(cchDest, *pcchNewDestLength) wchar_t* pszDest,
        _In_ size_t cchDest,
        _Out_ size_t* pcchNewDestLength,
        _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
        _In_ va_list argList);

NTSTRSAFEWORKERDDI
RtlUnicodeStringExHandleFill(
        _Out_writes_(cchRemaining) wchar_t* pszDestEnd,
        _In_ size_t cchRemaining,
        _In_ DWORD dwFlags);

NTSTRSAFEWORKERDDI
RtlUnicodeStringExHandleOtherFlags(
        _Inout_updates_(cchDest) wchar_t* pszDest,
        _In_ size_t cchDest,
        _In_ size_t cchOriginalDestLength,
        _Out_ size_t* pcchNewDestLength,
        _Outptr_result_buffer_(*pcchRemaining) wchar_t** ppszDestEnd,
        _Out_ size_t* pcchRemaining,
        _In_ DWORD dwFlags);

#endif  // !NTSTRSAFE_NO_UNICODE_STRING_FUNCTIONS



// To allow this to stand alone.
#define __WARNING_CYCLOMATIC_COMPLEXITY 28734
#define __WARNING_USING_UNINIT_VAR 6001
#define __WARNING_RETURN_UNINIT_VAR 6101
#define __WARNING_DEREF_NULL_PTR 6011
#define __WARNING_MISSING_ZERO_TERMINATION2 6054
#define __WARNING_INVALID_PARAM_VALUE_1 6387
#define __WARNING_INCORRECT_ANNOTATION 26007
#define __WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY 26015
#define __WARNING_PRECONDITION_NULLTERMINATION_VIOLATION 26035
#define __WARNING_POSTCONDITION_NULLTERMINATION_VIOLATION 26036
#define __WARNING_HIGH_PRIORITY_OVERFLOW_POSTCONDITION 26045
#define __WARNING_RANGE_POSTCONDITION_VIOLATION 26061
#define __WARNING_POTENTIAL_RANGE_POSTCONDITION_VIOLATION 26071
#define __WARNING_INVALID_PARAM_VALUE_3 28183
#define __WARNING_RETURNING_BAD_RESULT 28196
#define __WARNING_BANNED_API_USAGE 28719
#define __WARNING_POST_EXPECTED 28210

#pragma warning(push)
#if _MSC_VER <= 1400
#pragma warning(disable: 4616)  // turn off warning out of range so prefast pragmas won't show
// show up in build.wrn/build.err
#endif
#pragma warning(disable : 4996) // 'function': was declared deprecated
#pragma warning(disable : 4995) // name was marked as #pragma deprecated
#pragma warning(disable : 4793) // vararg causes native code generation
#pragma warning(disable : __WARNING_CYCLOMATIC_COMPLEXITY)


#ifndef NTSTRSAFE_LIB_IMPL

#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCchCopy(
  _Out_writes_(cchDest) _Always_(_Post_z_) LPTSTR  pszDest,
  _In_  size_t  cchDest,
  _In_  LPCTSTR pszSrc
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strcpy'.
  The size of the destination buffer (in characters) is a parameter and
  this function will not write past the end of this buffer and it will
  ALWAYS null terminate the destination buffer (unless it is zero length).

  This routine is not a replacement for strncpy.  That function will pad the
  destination string with extra null termination characters if the count is
  greater than the length of the source string, and it will fail to null
  terminate the destination string if the source string length is greater
  than or equal to the count. You can not blindly use this instead of strncpy:
  it is common for code to use it to "patch" strings and you would introduce
  errors if the code started null terminating in the middle of the string.

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the string was copied without truncation and null terminated,
  otherwise it will return a failure code. In failure cases as much of
  pszSrc will be copied to pszDest as possible, and pszDest will be null
  terminated.

Arguments:

pszDest        -   destination string

cchDest        -   size of destination buffer in characters.
length must be = (_tcslen(src) + 1) to hold all of the
source including the null terminator

pszSrc         -   source string which must be null terminated

Notes:
Behavior is undefined if source and destination strings overlap.

pszDest and pszSrc should not be NULL. See RtlStringCchCopyEx if you require
the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied and the
resultant dest string was null terminated

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result and is
null terminated. This is useful for situations where
truncation is ok

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/


NTSTRSAFEDDI
    RtlStringCchCopyA(
            _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cchDest,
            _In_ NTSTRSAFE_PCSTR pszSrc)
{
    NTSTATUS status;

    status = RtlStringValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        status = RtlStringCopyWorkerA(pszDest,
                cchDest,
                NULL,
                pszSrc,
                NTSTRSAFE_MAX_LENGTH);
    }
    else if (cchDest > 0)
    {
        *pszDest = '\0';
    }

    return status;
}



NTSTRSAFEDDI
    RtlStringCchCopyW(
            _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cchDest,
            _In_ NTSTRSAFE_PCWSTR pszSrc)
{
    NTSTATUS status;

    status = RtlStringValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        status = RtlStringCopyWorkerW(pszDest,
                cchDest,
                NULL,
                pszSrc,
                NTSTRSAFE_MAX_LENGTH);
    }
    else if (cchDest > 0)
    {
        *pszDest = L'\0';
    }

    return status;
}


#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS

#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCbCopy(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) LPTSTR pszDest,
  _In_  size_t cbDest,
  _In_  LPCTSTR pszSrc
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strcpy'.
  The size of the destination buffer (in bytes) is a parameter and this
  function will not write past the end of this buffer and it will ALWAYS
  null terminate the destination buffer (unless it is zero length).

  This routine is not a replacement for strncpy.  That function will pad the
  destination string with extra null termination characters if the count is
  greater than the length of the source string, and it will fail to null
  terminate the destination string if the source string length is greater
  than or equal to the count. You can not blindly use this instead of strncpy:
  it is common for code to use it to "patch" strings and you would introduce
  errors if the code started null terminating in the middle of the string.

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the string was copied without truncation and null terminated,
  otherwise it will return a failure code. In failure cases as much of pszSrc
  will be copied to pszDest as possible, and pszDest will be null terminated.

Arguments:

pszDest        -   destination string

cbDest         -   size of destination buffer in bytes.
length must be = ((_tcslen(src) + 1) * sizeof(TCHAR)) to
hold all of the source including the null terminator

pszSrc         -   source string which must be null terminated

Notes:
Behavior is undefined if source and destination strings overlap.

pszDest and pszSrc should not be NULL.  See RtlStringCbCopyEx if you require
the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied and the
resultant dest string was null terminated

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result and is
null terminated. This is useful for situations where
truncation is ok

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/


NTSTRSAFEDDI
    RtlStringCbCopyA(
            _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cbDest,
            _In_ NTSTRSAFE_PCSTR pszSrc)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);

    status = RtlStringValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        status = RtlStringCopyWorkerA(pszDest,
                cchDest,
                NULL,
                pszSrc,
                NTSTRSAFE_MAX_LENGTH);
    }
    else if (cchDest > 0)
    {
        *pszDest = '\0';
    }

    return status;
}



#pragma warning(push)
#pragma warning(disable : __WARNING_POSTCONDITION_NULLTERMINATION_VIOLATION)

NTSTRSAFEDDI
    RtlStringCbCopyW(
            _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cbDest,
            _In_ NTSTRSAFE_PCWSTR pszSrc)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);

    status = RtlStringValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        status = RtlStringCopyWorkerW(pszDest,
                cchDest,
                NULL,
                pszSrc,
                NTSTRSAFE_MAX_LENGTH);
    }
    else if (cchDest > 0)
    {
        *pszDest = L'\0';
    }

    return status;
}

#pragma warning(pop)


#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCchCopyEx(
  _Out_writes_(cchDest) _Always_(_Post_z_) LPTSTR  pszDest         OPTIONAL,
  _In_  size_t  cchDest,
  _In_  LPCTSTR pszSrc          OPTIONAL,
  _Outptr_opt_result_buffer_(*pcchRemaining) LPTSTR* ppszDestEnd     OPTIONAL,
  _Out_opt_ size_t* pcchRemaining   OPTIONAL,
  _In_  DWORD   dwFlags
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strcpy' with
  some additional parameters.  In addition to functionality provided by
  RtlStringCchCopy, this routine also returns a pointer to the end of the
  destination string and the number of characters left in the destination string
  including the null terminator. The flags parameter allows additional controls.

Arguments:

pszDest         -   destination string

cchDest         -   size of destination buffer in characters.
length must be = (_tcslen(pszSrc) + 1) to hold all of
the source including the null terminator

pszSrc          -   source string which must be null terminated

ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
pointer to the end of the destination string.  If the
function copied any data, the result will point to the
null termination character

pcchRemaining   -   if pcchRemaining is non-null, the function will return the
number of characters left in the destination string,
including the null terminator

dwFlags         -   controls some details of the string copy:

STRSAFE_FILL_BEHIND_NULL
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer
behind the null terminator

STRSAFE_IGNORE_NULLS
treat NULL string pointers like empty strings (TEXT("")).
this flag is useful for emulating functions like lstrcpy

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer, and it will
be null terminated. This will overwrite any truncated
string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_NO_TRUNCATION /
STRSAFE_NULL_ON_FAILURE
if the function fails, the destination buffer will be set
to the empty string. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
Behavior is undefined if source and destination strings overlap.

pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
may be NULL.  An error may still be returned even though NULLS are ignored
due to insufficient space.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied and the
resultant dest string was null terminated

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result and is
null terminated. This is useful for situations where
truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/


NTSTRSAFEDDI
    RtlStringCchCopyExA(
            _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cchDest,
            _In_ NTSTRSAFE_PCSTR pszSrc,
            _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
            _Out_opt_ size_t* pcchRemaining,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;

    status = RtlStringExValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcA(&pszSrc, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;

                if (cchDest != 0)
                {
                    *pszDest = '\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if (*pszSrc != '\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerA(pszDest,
                        cchDest,
                        &cchCopied,
                        pszSrc,
                        NTSTRSAFE_MAX_LENGTH);

                pszDestEnd = pszDest + cchCopied;
                cchRemaining = cchDest - cchCopied;

                if (NT_SUCCESS(status)                           &&
                        (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                        (cchRemaining > 1))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = cchRemaining * sizeof(char);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = '\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(char) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
            cbDest = cchDest * sizeof(char);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                    cbDest,
                    0,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }
    else if (cchDest > 0)
    {
        *pszDest = '\0';
    }

    return status;
}



NTSTRSAFEDDI
    RtlStringCchCopyExW(
            _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cchDest,
            _In_ NTSTRSAFE_PCWSTR pszSrc,
            _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
            _Out_opt_ size_t* pcchRemaining,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;

    status = RtlStringExValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcW(&pszSrc, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;

                if (cchDest != 0)
                {
                    *pszDest = L'\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerW(pszDest,
                        cchDest,
                        &cchCopied,
                        pszSrc,
                        NTSTRSAFE_MAX_LENGTH);

                pszDestEnd = pszDest + cchCopied;
                cchRemaining = cchDest - cchCopied;

                if (NT_SUCCESS(status)                           &&
                        (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                        (cchRemaining > 1))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = cchRemaining * sizeof(wchar_t);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = L'\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(wchar_t) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            cbDest = cchDest * sizeof(wchar_t);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                    cbDest,
                    0,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }
    else if (cchDest > 0)
    {
        *pszDest = L'\0';
    }

    return status;
}


#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCbCopyEx(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) LPTSTR  pszDest         OPTIONAL,
  _In_  size_t  cbDest,
  _In_  LPCTSTR pszSrc          OPTIONAL,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) LPTSTR* ppszDestEnd     OPTIONAL,
  _Out_opt_ size_t* pcbRemaining    OPTIONAL,
  _In_  DWORD   dwFlags
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strcpy' with
  some additional parameters.  In addition to functionality provided by
  RtlStringCbCopy, this routine also returns a pointer to the end of the
  destination string and the number of bytes left in the destination string
  including the null terminator. The flags parameter allows additional controls.

Arguments:

pszDest         -   destination string

cbDest          -   size of destination buffer in bytes.
length must be ((_tcslen(pszSrc) + 1) * sizeof(TCHAR)) to
hold all of the source including the null terminator

pszSrc          -   source string which must be null terminated

ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
pointer to the end of the destination string.  If the
function copied any data, the result will point to the
null termination character

pcbRemaining    -   pcbRemaining is non-null,the function will return the
number of bytes left in the destination string,
including the null terminator

dwFlags         -   controls some details of the string copy:

STRSAFE_FILL_BEHIND_NULL
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer
behind the null terminator

STRSAFE_IGNORE_NULLS
treat NULL string pointers like empty strings (TEXT("")).
this flag is useful for emulating functions like lstrcpy

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer, and it will
be null terminated. This will overwrite any truncated
string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_NO_TRUNCATION /
STRSAFE_NULL_ON_FAILURE
if the function fails, the destination buffer will be set
to the empty string. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
Behavior is undefined if source and destination strings overlap.

pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
may be NULL.  An error may still be returned even though NULLS are ignored
due to insufficient space.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied and the
resultant dest string was null terminated

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result and is
null terminated. This is useful for situations where
truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/


NTSTRSAFEDDI
    RtlStringCbCopyExA(
            _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cbDest,
            _In_ NTSTRSAFE_PCSTR pszSrc,
            _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
            _Out_opt_ size_t* pcbRemaining,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);

    status = RtlStringExValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcA(&pszSrc, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;

                if (cchDest != 0)
                {
                    *pszDest = '\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if (*pszSrc != '\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
                else
                {
                    // for consistency with other use in this case...
                    __analysis_assume_nullterminated(pszDest);
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerA(pszDest,
                        cchDest,
                        &cchCopied,
                        pszSrc,
                        NTSTRSAFE_MAX_LENGTH);

                pszDestEnd = pszDest + cchCopied;
                cchRemaining = cchDest - cchCopied;

                if (NT_SUCCESS(status)                           &&
                        (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                        (cchRemaining > 1))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = '\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                    cbDest,
                    0,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
            }
        }
    }
    else if (cchDest > 0)
    {
        *pszDest = '\0';
    }

    return status;
}



#pragma warning(push)
#pragma warning(disable: __WARNING_POSTCONDITION_NULLTERMINATION_VIOLATION)

NTSTRSAFEDDI
    RtlStringCbCopyExW(
            _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cbDest,
            _In_ NTSTRSAFE_PCWSTR pszSrc,
            _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
            _Out_opt_ size_t* pcbRemaining,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);

    status = RtlStringExValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcW(&pszSrc, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;

                if (cchDest != 0)
                {
                    *pszDest = L'\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
                else
                {
                    // for consistency with other use in this case...
                    __analysis_assume_nullterminated(pszDest);
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerW(pszDest,
                        cchDest,
                        &cchCopied,
                        pszSrc,
                        NTSTRSAFE_MAX_LENGTH);

                pszDestEnd = pszDest + cchCopied;
                cchRemaining = cchDest - cchCopied;

                if (NT_SUCCESS(status) && (dwFlags & STRSAFE_FILL_BEHIND_NULL))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = L'\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                    cbDest,
                    0,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
            }
        }
    }
    else if (cchDest > 0)
    {
        *pszDest = L'\0';
    }

    return status;
}

#pragma warning(pop)


#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCchCopyN(
  _Out_writes_(cchDest) _Always_(_Post_z_) LPTSTR  pszDest,
  _In_  size_t  cchDest,
  _In_reads_or_z_(cchToCopy)  LPCTSTR pszSrc,
  _In_  size_t  cchToCopy
  );


  Routine Description:

  This routine is a safer version of the C built-in function 'strncpy'.
  The size of the destination buffer (in characters) is a parameter and
  this function will not write past the end of this buffer and it will
  ALWAYS null terminate the destination buffer (unless it is zero length).

  This routine is meant as a replacement for strncpy, but it does behave
  differently. This function will not pad the destination buffer with extra
  null termination characters if cchToCopy is greater than the length of pszSrc.

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the entire string or the first cchToCopy characters were copied
  without truncation and the resultant destination string was null terminated,
  otherwise it will return a failure code. In failure cases as much of pszSrc
  will be copied to pszDest as possible, and pszDest will be null terminated.

Arguments:

pszDest        -   destination string

cchDest        -   size of destination buffer in characters.
length must be = (_tcslen(src) + 1) to hold all of the
source including the null terminator

pszSrc         -   source string

cchToCopy      -   maximum number of characters to copy from source string,
not including the null terminator.

Notes:
Behavior is undefined if source and destination strings overlap.

pszDest and pszSrc should not be NULL. See RtlStringCchCopyNEx if you require
the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied and the
resultant dest string was null terminated

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result and is
null terminated. This is useful for situations where
truncation is ok

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/


NTSTRSAFEDDI
    RtlStringCchCopyNA(
            _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cchDest,
            _In_reads_or_z_(cchToCopy) STRSAFE_PCNZCH pszSrc,
            _In_ size_t cchToCopy)
{
    NTSTATUS status;

    status = RtlStringValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        if (cchToCopy > NTSTRSAFE_MAX_LENGTH)
        {
            status = STATUS_INVALID_PARAMETER;

            *pszDest = '\0';
        }
        else
        {
            status = RtlStringCopyWorkerA(pszDest,
                    cchDest,
                    NULL,
                    pszSrc,
                    cchToCopy);
        }
    }
    else if (cchDest > 0)
    {
        *pszDest = '\0';
    }

    return status;
}



NTSTRSAFEDDI
    RtlStringCchCopyNW(
            _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cchDest,
            _In_reads_or_z_(cchToCopy) STRSAFE_PCNZWCH pszSrc,
            _In_ size_t cchToCopy)
{
    NTSTATUS status;

    status = RtlStringValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        if (cchToCopy > NTSTRSAFE_MAX_LENGTH)
        {
            status = STATUS_INVALID_PARAMETER;

            *pszDest = L'\0';
        }
        else
        {
            status = RtlStringCopyWorkerW(pszDest,
                    cchDest,
                    NULL,
                    pszSrc,
                    cchToCopy);
        }
    }
    else if (cchDest > 0)
    {
        *pszDest = L'\0';
    }

    return status;
}


#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCbCopyN(
  _Out_writes_bytes_(cbDest) LPTSTR  pszDest,
  _In_  size_t  cbDest,
  _In_  LPCTSTR pszSrc,
  _In_  size_t  cbToCopy
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncpy'.
  The size of the destination buffer (in bytes) is a parameter and this
  function will not write past the end of this buffer and it will ALWAYS
  null terminate the destination buffer (unless it is zero length).

  This routine is meant as a replacement for strncpy, but it does behave
  differently. This function will not pad the destination buffer with extra
  null termination characters if cbToCopy is greater than the size of pszSrc.

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the entire string or the first cbToCopy characters were
  copied without truncation and the resultant destination string was null
  terminated, otherwise it will return a failure code. In failure cases as
  much of pszSrc will be copied to pszDest as possible, and pszDest will be
  null terminated.

Arguments:

pszDest        -   destination string

cbDest         -   size of destination buffer in bytes.
length must be = ((_tcslen(src) + 1) * sizeof(TCHAR)) to
hold all of the source including the null terminator

pszSrc         -   source string

cbToCopy       -   maximum number of bytes to copy from source string,
not including the null terminator.

Notes:
Behavior is undefined if source and destination strings overlap.

pszDest and pszSrc should not be NULL.  See RtlStringCbCopyEx if you require
the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied and the
resultant dest string was null terminated

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result and is
null terminated. This is useful for situations where
truncation is ok

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/


NTSTRSAFEDDI
    RtlStringCbCopyNA(
            _Out_writes_bytes_(cbDest) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cbDest,
            _In_reads_bytes_(cbToCopy) STRSAFE_PCNZCH pszSrc,
            _In_ size_t cbToCopy)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);

    status = RtlStringValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        size_t cchToCopy = cbToCopy / sizeof(char);

        if (cchToCopy > NTSTRSAFE_MAX_LENGTH)
        {
            status = STATUS_INVALID_PARAMETER;

            *pszDest = '\0';
        }
        else
        {
            status = RtlStringCopyWorkerA(pszDest,
                    cchDest,
                    NULL,
                    pszSrc,
                    cchToCopy);
        }
    }

    return status;
}



#pragma warning(push)
#pragma warning(disable : __WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY)

NTSTRSAFEDDI
    RtlStringCbCopyNW(
            _Out_writes_bytes_(cbDest) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cbDest,
            _In_reads_bytes_(cbToCopy) STRSAFE_PCNZWCH pszSrc,
            _In_ size_t cbToCopy)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);

    status = RtlStringValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        size_t cchToCopy = cbToCopy / sizeof(wchar_t);

        if (cchToCopy > NTSTRSAFE_MAX_LENGTH)
        {
            status = STATUS_INVALID_PARAMETER;

            *pszDest = L'\0';
        }
        else
        {
            status = RtlStringCopyWorkerW(pszDest,
                    cchDest,
                    NULL,
                    pszSrc,
                    cchToCopy);
        }
    }

    return status;
}

#pragma warning(pop)


#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCchCopyNEx(
  _Out_writes_(cchDest) _Always_(_Post_z_) LPTSTR  pszDest         OPTIONAL,
  _In_  size_t  cchDest,
  _In_  LPCTSTR pszSrc          OPTIONAL,
  _In_  size_t  cchToCopy,
  _Outptr_opt_result_buffer_(*pcchRemaining) LPTSTR* ppszDestEnd OPTIONAL,
  _Out_opt_ size_t* pcchRemaining OPTIONAL,
  _In_  DWORD   dwFlags
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncpy' with
  some additional parameters.  In addition to functionality provided by
  RtlStringCchCopyN, this routine also returns a pointer to the end of the
  destination string and the number of characters left in the destination
  string including the null terminator. The flags parameter allows
  additional controls.

  This routine is meant as a replacement for strncpy, but it does behave
  differently. This function will not pad the destination buffer with extra
  null termination characters if cchToCopy is greater than the length of pszSrc.

Arguments:

pszDest         -   destination string

cchDest         -   size of destination buffer in characters.
length must be = (_tcslen(pszSrc) + 1) to hold all of
the source including the null terminator

pszSrc          -   source string

cchToCopy       -   maximum number of characters to copy from the source
string

ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
pointer to the end of the destination string.  If the
function copied any data, the result will point to the
null termination character

pcchRemaining   -   if pcchRemaining is non-null, the function will return the
number of characters left in the destination string,
including the null terminator

dwFlags         -   controls some details of the string copy:

STRSAFE_FILL_BEHIND_NULL
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer
behind the null terminator

STRSAFE_IGNORE_NULLS
treat NULL string pointers like empty strings (TEXT("")).
this flag is useful for emulating functions like lstrcpy

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer, and it will
be null terminated. This will overwrite any truncated
string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_NO_TRUNCATION /
STRSAFE_NULL_ON_FAILURE
if the function fails, the destination buffer will be set
to the empty string. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
Behavior is undefined if source and destination strings overlap.

pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified. If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
may be NULL. An error may still be returned even though NULLS are ignored
due to insufficient space.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied and the
resultant dest string was null terminated

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result and is
null terminated. This is useful for situations where
truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/


NTSTRSAFEDDI
    RtlStringCchCopyNExA(
            _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cchDest,
            _In_reads_or_z_(cchToCopy) STRSAFE_PCNZCH pszSrc,
            _In_ size_t cchToCopy,
            _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
            _Out_opt_ size_t* pcchRemaining,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;

    status = RtlStringExValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcA(&pszSrc, &cchToCopy, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;

                if (cchDest != 0)
                {
                    *pszDest = '\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if ((cchToCopy != 0) && (*pszSrc != '\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerA(pszDest,
                        cchDest,
                        &cchCopied,
                        pszSrc,
                        cchToCopy);

                pszDestEnd = pszDest + cchCopied;
                cchRemaining = cchDest - cchCopied;

                if (NT_SUCCESS(status)                           &&
                        (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                        (cchRemaining > 1))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = cchRemaining * sizeof(char);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = '\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(char) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
            cbDest = cchDest * sizeof(char);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                    cbDest,
                    0,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }
    else if (cchDest > 0)
    {
        *pszDest = '\0';
    }

    return status;
}



NTSTRSAFEDDI
    RtlStringCchCopyNExW(
            _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cchDest,
            _In_reads_or_z_(cchToCopy) STRSAFE_PCNZWCH pszSrc,
            _In_ size_t cchToCopy,
            _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
            _Out_opt_ size_t* pcchRemaining,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;

    status = RtlStringExValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcW(&pszSrc, &cchToCopy, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;

                if (cchDest != 0)
                {
                    *pszDest = L'\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if ((cchToCopy != 0) && (*pszSrc != L'\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerW(pszDest,
                        cchDest,
                        &cchCopied,
                        pszSrc,
                        cchToCopy);

                pszDestEnd = pszDest + cchCopied;
                cchRemaining = cchDest - cchCopied;

                if (NT_SUCCESS(status)                           &&
                        (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                        (cchRemaining > 1))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = cchRemaining * sizeof(wchar_t);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = L'\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(wchar_t) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            cbDest = cchDest * sizeof(wchar_t);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                    cbDest,
                    0,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }
    else if (cchDest > 0)
    {
        *pszDest = L'\0';
    }

    return status;
}


#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCbCopyNEx(
  _Out_writes_bytes_(cbDest) LPTSTR  pszDest         OPTIONAL,
  _In_  size_t  cbDest,
  _In_  LPCTSTR pszSrc          OPTIONAL,
  _In_  size_t  cbToCopy,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) LPTSTR* ppszDestEnd     OPTIONAL,
  _Out_opt_ size_t* pcbRemaining    OPTIONAL,
  _In_  DWORD   dwFlags
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncpy' with
  some additional parameters.  In addition to functionality provided by
  RtlStringCbCopyN, this routine also returns a pointer to the end of the
  destination string and the number of bytes left in the destination string
  including the null terminator. The flags parameter allows additional controls.

  This routine is meant as a replacement for strncpy, but it does behave
  differently. This function will not pad the destination buffer with extra
  null termination characters if cbToCopy is greater than the size of pszSrc.

Arguments:

pszDest         -   destination string

cbDest          -   size of destination buffer in bytes.
length must be ((_tcslen(pszSrc) + 1) * sizeof(TCHAR)) to
hold all of the source including the null terminator

pszSrc          -   source string

cbToCopy        -   maximum number of bytes to copy from source string

ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
pointer to the end of the destination string.  If the
function copied any data, the result will point to the
null termination character

pcbRemaining    -   pcbRemaining is non-null,the function will return the
number of bytes left in the destination string,
including the null terminator

dwFlags         -   controls some details of the string copy:

STRSAFE_FILL_BEHIND_NULL
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer
behind the null terminator

STRSAFE_IGNORE_NULLS
treat NULL string pointers like empty strings (TEXT("")).
this flag is useful for emulating functions like lstrcpy

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer, and it will
be null terminated. This will overwrite any truncated
string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_NO_TRUNCATION /
STRSAFE_NULL_ON_FAILURE
if the function fails, the destination buffer will be set
to the empty string. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
Behavior is undefined if source and destination strings overlap.

pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
may be NULL.  An error may still be returned even though NULLS are ignored
due to insufficient space.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied and the
resultant dest string was null terminated

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result and is
null terminated. This is useful for situations where
truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/


NTSTRSAFEDDI
    RtlStringCbCopyNExA(
            _Out_writes_bytes_(cbDest) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cbDest,
            _In_reads_bytes_(cbToCopy) STRSAFE_PCNZCH pszSrc,
            _In_ size_t cbToCopy,
            _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
            _Out_opt_ size_t* pcbRemaining,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);

    status = RtlStringExValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;
        size_t cchToCopy = cbToCopy / sizeof(char);

        status = RtlStringExValidateSrcA(&pszSrc, &cchToCopy, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;

                if (cchDest != 0)
                {
                    *pszDest = '\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if ((cchToCopy != 0) && (*pszSrc != '\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
                else
                {
                    // for consistency with other use in this case...
                    __analysis_assume_nullterminated(pszDest);
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerA(pszDest,
                        cchDest,
                        &cchCopied,
                        pszSrc,
                        cchToCopy);

                pszDestEnd = pszDest + cchCopied;
                cchRemaining = cchDest - cchCopied;

                if (NT_SUCCESS(status)                           &&
                        (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                        (cchRemaining > 1))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = '\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                    cbDest,
                    0,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
            }
        }
    }

    return status;
}



#pragma warning(push)
#pragma warning(disable : __WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY)

NTSTRSAFEDDI
    RtlStringCbCopyNExW(
            _Out_writes_bytes_(cbDest) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cbDest,
            _In_reads_bytes_(cbToCopy) STRSAFE_PCNZWCH pszSrc,
            _In_ size_t cbToCopy,
            _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
            _Out_opt_ size_t* pcbRemaining,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);

    status = RtlStringExValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;
        size_t cchToCopy = cbToCopy / sizeof(wchar_t);

        status = RtlStringExValidateSrcW(&pszSrc, &cchToCopy, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;

                if (cchDest != 0)
                {
                    *pszDest = L'\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if ((cchToCopy != 0) && (*pszSrc != L'\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
                else
                {
                    // for consistency with other use in this case...
                    __analysis_assume_nullterminated(pszDest);
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerW(pszDest,
                        cchDest,
                        &cchCopied,
                        pszSrc,
                        cchToCopy);

                pszDestEnd = pszDest + cchCopied;
                cchRemaining = cchDest - cchCopied;

                if (NT_SUCCESS(status) && (dwFlags & STRSAFE_FILL_BEHIND_NULL))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = L'\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                    cbDest,
                    0,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
            }
        }
    }

    return status;
}

#pragma warning(pop)


#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCchCat(
  _Inout_updates_(cchDest) _Always_(_Post_z_) LPTSTR  pszDest,
  _In_     size_t  cchDest,
  _In_     LPCTSTR pszSrc
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strcat'.
  The size of the destination buffer (in characters) is a parameter and this
  function will not write past the end of this buffer and it will ALWAYS
  null terminate the destination buffer (unless it is zero length).

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the string was concatenated without truncation and null terminated,
  otherwise it will return a failure code. In failure cases as much of pszSrc
  will be appended to pszDest as possible, and pszDest will be null
  terminated.

Arguments:

pszDest     -  destination string which must be null terminated

cchDest     -  size of destination buffer in characters.
length must be = (_tcslen(pszDest) + _tcslen(pszSrc) + 1)
to hold all of the combine string plus the null
terminator

pszSrc      -  source string which must be null terminated

Notes:
Behavior is undefined if source and destination strings overlap.

pszDest and pszSrc should not be NULL.  See RtlStringCchCatEx if you require
the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all concatenated and
the resultant dest string was null terminated

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the operation
failed due to insufficient space. When this error occurs,
the destination buffer is modified to contain a truncated
version of the ideal result and is null terminated. This
is useful for situations where truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/


NTSTRSAFEDDI
    RtlStringCchCatA(
            _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cchDest,
            _In_ NTSTRSAFE_PCSTR pszSrc)
{
    NTSTATUS status;
    size_t cchDestLength;

    status = RtlStringValidateDestAndLengthA(pszDest,
            cchDest,
            &cchDestLength,
            NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        status = RtlStringCopyWorkerA(pszDest + cchDestLength,
                cchDest - cchDestLength,
                NULL,
                pszSrc,
                NTSTRSAFE_MAX_LENGTH);
    }

    return status;
}



NTSTRSAFEDDI
    RtlStringCchCatW(
            _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cchDest,
            _In_ NTSTRSAFE_PCWSTR pszSrc)
{
    NTSTATUS status;
    size_t cchDestLength;

    status = RtlStringValidateDestAndLengthW(pszDest,
            cchDest,
            &cchDestLength,
            NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        status = RtlStringCopyWorkerW(pszDest + cchDestLength,
                cchDest - cchDestLength,
                NULL,
                pszSrc,
                NTSTRSAFE_MAX_LENGTH);
    }

    return status;
}


#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCbCat(
  _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) LPTSTR  pszDest,
  _In_     size_t  cbDest,
  _In_     LPCTSTR pszSrc
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strcat'.
  The size of the destination buffer (in bytes) is a parameter and this
  function will not write past the end of this buffer and it will ALWAYS
  null terminate the destination buffer (unless it is zero length).

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the string was concatenated without truncation and null terminated,
  otherwise it will return a failure code. In failure cases as much of pszSrc
  will be appended to pszDest as possible, and pszDest will be null
  terminated.

Arguments:

pszDest     -  destination string which must be null terminated

cbDest      -  size of destination buffer in bytes.
length must be = ((_tcslen(pszDest) + _tcslen(pszSrc) + 1) * sizeof(TCHAR)
to hold all of the combine string plus the null
terminator

pszSrc      -  source string which must be null terminated

Notes:
Behavior is undefined if source and destination strings overlap.

pszDest and pszSrc should not be NULL.  See RtlStringCbCatEx if you require
the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all concatenated and
the resultant dest string was null terminated

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the operation
failed due to insufficient space. When this error occurs,
the destination buffer is modified to contain a truncated
version of the ideal result and is null terminated. This
is useful for situations where truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/


NTSTRSAFEDDI
    RtlStringCbCatA(
            _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cbDest,
            _In_ NTSTRSAFE_PCSTR pszSrc)
{
    NTSTATUS status;
    size_t cchDestLength;
    size_t cchDest = cbDest / sizeof(char);

    status = RtlStringValidateDestAndLengthA(pszDest,
            cchDest,
            &cchDestLength,
            NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        status = RtlStringCopyWorkerA(pszDest + cchDestLength,
                cchDest - cchDestLength,
                NULL,
                pszSrc,
                NTSTRSAFE_MAX_LENGTH);
    }

    return status;
}



NTSTRSAFEDDI
    RtlStringCbCatW(
            _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cbDest,
            _In_ NTSTRSAFE_PCWSTR pszSrc)
{
    NTSTATUS status;
    size_t cchDestLength;
    size_t cchDest = cbDest / sizeof(wchar_t);

    status = RtlStringValidateDestAndLengthW(pszDest,
            cchDest,
            &cchDestLength,
            NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        status = RtlStringCopyWorkerW(pszDest + cchDestLength,
                cchDest - cchDestLength,
                NULL,
                pszSrc,
                NTSTRSAFE_MAX_LENGTH);
    }

    return status;
}


#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCchCatEx(
  _Inout_updates_(cchDest) _Always_(_Post_z_) LPTSTR  pszDest         OPTIONAL,
  _In_     size_t  cchDest,
  _In_     LPCTSTR pszSrc          OPTIONAL,
  _Outptr_opt_result_buffer_(*pcchRemaining)    LPTSTR* ppszDestEnd     OPTIONAL,
  _Out_opt_    size_t* pcchRemaining   OPTIONAL,
  _In_     DWORD   dwFlags
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strcat' with
  some additional parameters.  In addition to functionality provided by
  RtlStringCchCat, this routine also returns a pointer to the end of the
  destination string and the number of characters left in the destination string
  including the null terminator. The flags parameter allows additional controls.

Arguments:

pszDest         -   destination string which must be null terminated

cchDest         -   size of destination buffer in characters
length must be (_tcslen(pszDest) + _tcslen(pszSrc) + 1)
to hold all of the combine string plus the null
terminator.

pszSrc          -   source string which must be null terminated

ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
pointer to the end of the destination string.  If the
function appended any data, the result will point to the
null termination character

pcchRemaining   -   if pcchRemaining is non-null, the function will return the
number of characters left in the destination string,
including the null terminator

dwFlags         -   controls some details of the string copy:

STRSAFE_FILL_BEHIND_NULL
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer
behind the null terminator

STRSAFE_IGNORE_NULLS
treat NULL string pointers like empty strings (TEXT("")).
this flag is useful for emulating functions like lstrcat

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer, and it will
be null terminated. This will overwrite any pre-existing
or truncated string

STRSAFE_NULL_ON_FAILURE
if the function fails, the destination buffer will be set
to the empty string. This will overwrite any pre-existing or
truncated string

STRSAFE_NO_TRUNCATION
if the function returns STATUS_BUFFER_OVERFLOW, pszDest
will not contain a truncated string, it will remain unchanged.

Notes:
Behavior is undefined if source and destination strings overlap.

pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
may be NULL.  An error may still be returned even though NULLS are ignored
due to insufficient space.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all concatenated and
the resultant dest string was null terminated

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the operation
failed due to insufficient space. When this error
occurs, the destination buffer is modified to contain
a truncated version of the ideal result and is null
terminated. This is useful for situations where
truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/


NTSTRSAFEDDI
    RtlStringCchCatExA(
            _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cchDest,
            _In_ NTSTRSAFE_PCSTR pszSrc,
            _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
            _Out_opt_ size_t* pcchRemaining,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDestLength;

    status = RtlStringExValidateDestAndLengthA(pszDest,
            cchDest,
            &cchDestLength,
            NTSTRSAFE_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;

        status = RtlStringExValidateSrcA(&pszSrc, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining <= 1)
            {
                // only fail if there was actually src data to append
                if (*pszSrc != '\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerA(pszDestEnd,
                        cchRemaining,
                        &cchCopied,
                        pszSrc,
                        NTSTRSAFE_MAX_LENGTH);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;

                if (NT_SUCCESS(status)                           &&
                        (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                        (cchRemaining > 1))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = cchRemaining * sizeof(char);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(char) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
            cbDest = cchDest * sizeof(char);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                    cbDest,
                    cchDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }

    return status;
}



NTSTRSAFEDDI
    RtlStringCchCatExW(
            _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cchDest,
            _In_ NTSTRSAFE_PCWSTR pszSrc,
            _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
            _Out_opt_ size_t* pcchRemaining,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDestLength;

    status = RtlStringExValidateDestAndLengthW(pszDest,
            cchDest,
            &cchDestLength,
            NTSTRSAFE_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;

        status = RtlStringExValidateSrcW(&pszSrc, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining <= 1)
            {
                // only fail if there was actually src data to append
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerW(pszDestEnd,
                        cchRemaining,
                        &cchCopied,
                        pszSrc,
                        NTSTRSAFE_MAX_LENGTH);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;

                if (NT_SUCCESS(status)                           &&
                        (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                        (cchRemaining > 1))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = cchRemaining * sizeof(wchar_t);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(char) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
            cbDest = cchDest * sizeof(wchar_t);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                    cbDest,
                    cchDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }

    return status;
}


#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCbCatEx(
  _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) LPTSTR  pszDest         OPTIONAL,
  _In_     size_t  cbDest,
  _In_     LPCTSTR pszSrc          OPTIONAL,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining)    LPTSTR* ppszDestEnd     OPTIONAL,
  _Out_opt_    size_t* pcbRemaining    OPTIONAL,
  _In_     DWORD   dwFlags
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strcat' with
  some additional parameters.  In addition to functionality provided by
  RtlStringCbCat, this routine also returns a pointer to the end of the
  destination string and the number of bytes left in the destination string
  including the null terminator. The flags parameter allows additional controls.

Arguments:

pszDest         -   destination string which must be null terminated

cbDest          -   size of destination buffer in bytes.
length must be ((_tcslen(pszDest) + _tcslen(pszSrc) + 1) * sizeof(TCHAR)
to hold all of the combine string plus the null
terminator.

pszSrc          -   source string which must be null terminated

ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
pointer to the end of the destination string.  If the
function appended any data, the result will point to the
null termination character

pcbRemaining    -   if pcbRemaining is non-null, the function will return
the number of bytes left in the destination string,
including the null terminator

dwFlags         -   controls some details of the string copy:

STRSAFE_FILL_BEHIND_NULL
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer
behind the null terminator

STRSAFE_IGNORE_NULLS
treat NULL string pointers like empty strings (TEXT("")).
this flag is useful for emulating functions like lstrcat

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer, and it will
be null terminated. This will overwrite any pre-existing
or truncated string

STRSAFE_NULL_ON_FAILURE
if the function fails, the destination buffer will be set
to the empty string. This will overwrite any pre-existing or
truncated string

STRSAFE_NO_TRUNCATION
if the function returns STATUS_BUFFER_OVERFLOW, pszDest
will not contain a truncated string, it will remain unchanged.

Notes:
Behavior is undefined if source and destination strings overlap.

pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
may be NULL.  An error may still be returned even though NULLS are ignored
due to insufficient space.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all concatenated
and the resultant dest string was null terminated

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the operation
failed due to insufficient space. When this error
occurs, the destination buffer is modified to contain
a truncated version of the ideal result and is null
terminated. This is useful for situations where
truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/


NTSTRSAFEDDI
    RtlStringCbCatExA(
            _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cbDest,
            _In_ NTSTRSAFE_PCSTR pszSrc,
            _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
            _Out_opt_ size_t* pcbRemaining,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);
    size_t cchDestLength;

    status = RtlStringExValidateDestAndLengthA(pszDest,
            cchDest,
            &cchDestLength,
            NTSTRSAFE_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;

        status = RtlStringExValidateSrcA(&pszSrc, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining <= 1)
            {
                // only fail if there was actually src data to append
                if (*pszSrc != '\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerA(pszDestEnd,
                        cchRemaining,
                        &cchCopied,
                        pszSrc,
                        NTSTRSAFE_MAX_LENGTH);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;

                if (NT_SUCCESS(status)                           &&
                        (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                        (cchRemaining > 1))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                    cbDest,
                    cchDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
            }
        }
    }

    return status;
}



NTSTRSAFEDDI
    RtlStringCbCatExW(
            _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cbDest,
            _In_ NTSTRSAFE_PCWSTR pszSrc,
            _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
            _Out_opt_ size_t* pcbRemaining,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);
    size_t cchDestLength;

    status = RtlStringExValidateDestAndLengthW(pszDest,
            cchDest,
            &cchDestLength,
            NTSTRSAFE_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;

        status = RtlStringExValidateSrcW(&pszSrc, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining <= 1)
            {
                // only fail if there was actually src data to append
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerW(pszDestEnd,
                        cchRemaining,
                        &cchCopied,
                        pszSrc,
                        NTSTRSAFE_MAX_LENGTH);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;

                if (NT_SUCCESS(status) && (dwFlags & STRSAFE_FILL_BEHIND_NULL))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                    cbDest,
                    cchDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
            }
        }
    }

    return status;
}


#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCchCatN(
  _Inout_updates_(cchDest) _Always_(_Post_z_) LPTSTR  pszDest,
  _In_     size_t  cchDest,
  _In_     LPCTSTR pszSrc,
  _In_     size_t  cchToAppend
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncat'.
  The size of the destination buffer (in characters) is a parameter as well as
  the maximum number of characters to append, excluding the null terminator.
  This function will not write past the end of the destination buffer and it will
  ALWAYS null terminate pszDest (unless it is zero length).

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if all of pszSrc or the first cchToAppend characters were appended
  to the destination string and it was null terminated, otherwise it will
  return a failure code. In failure cases as much of pszSrc will be appended
  to pszDest as possible, and pszDest will be null terminated.

Arguments:

pszDest         -   destination string which must be null terminated

cchDest         -   size of destination buffer in characters.
length must be (_tcslen(pszDest) + min(cchToAppend, _tcslen(pszSrc)) + 1)
to hold all of the combine string plus the null
terminator.

pszSrc          -   source string

cchToAppend     -   maximum number of characters to append

Notes:
Behavior is undefined if source and destination strings overlap.

pszDest and pszSrc should not be NULL. See RtlStringCchCatNEx if you require
the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if all of pszSrc or the first cchToAppend characters
were concatenated to pszDest and the resultant dest
string was null terminated

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the operation
failed due to insufficient space. When this error
occurs, the destination buffer is modified to contain
a truncated version of the ideal result and is null
terminated. This is useful for situations where
truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/


NTSTRSAFEDDI
    RtlStringCchCatNA(
            _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cchDest,
            _In_reads_or_z_(cchToAppend) STRSAFE_PCNZCH pszSrc,
            _In_ size_t cchToAppend)
{
    NTSTATUS status;
    size_t cchDestLength;

    status = RtlStringValidateDestAndLengthA(pszDest,
            cchDest,
            &cchDestLength,
            NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        if (cchToAppend > NTSTRSAFE_MAX_LENGTH)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            status = RtlStringCopyWorkerA(pszDest + cchDestLength,
                    cchDest - cchDestLength,
                    NULL,
                    pszSrc,
                    cchToAppend);
        }
    }

    return status;
}



NTSTRSAFEDDI
    RtlStringCchCatNW(
            _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cchDest,
            _In_reads_or_z_(cchToAppend) STRSAFE_PCNZWCH pszSrc,
            _In_ size_t cchToAppend)
{
    NTSTATUS status;
    size_t cchDestLength;

    status = RtlStringValidateDestAndLengthW(pszDest,
            cchDest,
            &cchDestLength,
            NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        if (cchToAppend > NTSTRSAFE_MAX_LENGTH)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            status = RtlStringCopyWorkerW(pszDest + cchDestLength,
                    cchDest - cchDestLength,
                    NULL,
                    pszSrc,
                    cchToAppend);
        }
    }

    return status;
}


#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCbCatN(
  _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) LPTSTR  pszDest,
  _In_     size_t  cbDest,
  _In_     LPCTSTR pszSrc,
  _In_     size_t  cbToAppend
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncat'.
  The size of the destination buffer (in bytes) is a parameter as well as
  the maximum number of bytes to append, excluding the null terminator.
  This function will not write past the end of the destination buffer and it will
  ALWAYS null terminate pszDest (unless it is zero length).

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if all of pszSrc or the first cbToAppend bytes were appended
  to the destination string and it was null terminated, otherwise it will
  return a failure code. In failure cases as much of pszSrc will be appended
  to pszDest as possible, and pszDest will be null terminated.

Arguments:

pszDest         -   destination string which must be null terminated

cbDest          -   size of destination buffer in bytes.
length must be ((_tcslen(pszDest) + min(cbToAppend / sizeof(TCHAR), _tcslen(pszSrc)) + 1) * sizeof(TCHAR)
to hold all of the combine string plus the null
terminator.

pszSrc          -   source string

cbToAppend      -   maximum number of bytes to append

Notes:
Behavior is undefined if source and destination strings overlap.

pszDest and pszSrc should not be NULL. See RtlStringCbCatNEx if you require
the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if all of pszSrc or the first cbToAppend bytes were
concatenated to pszDest and the resultant dest string
was null terminated

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the operation
failed due to insufficient space. When this error
occurs, the destination buffer is modified to contain
a truncated version of the ideal result and is null
terminated. This is useful for situations where
truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/


NTSTRSAFEDDI
    RtlStringCbCatNA(
            _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cbDest,
            _In_reads_bytes_(cbToAppend) STRSAFE_PCNZCH pszSrc,
            _In_ size_t cbToAppend)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);
    size_t cchDestLength;

    status = RtlStringValidateDestAndLengthA(pszDest,
            cchDest,
            &cchDestLength,
            NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        size_t cchToAppend = cbToAppend / sizeof(char);

        if (cchToAppend > NTSTRSAFE_MAX_LENGTH)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            status = RtlStringCopyWorkerA(pszDest + cchDestLength,
                    cchDest - cchDestLength,
                    NULL,
                    pszSrc,
                    cchToAppend);
        }
    }

    return status;
}



NTSTRSAFEDDI
    RtlStringCbCatNW(
            _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cbDest,
            _In_reads_bytes_(cbToAppend) STRSAFE_PCNZWCH pszSrc,
            _In_ size_t cbToAppend)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);
    size_t cchDestLength;

    status = RtlStringValidateDestAndLengthW(pszDest,
            cchDest,
            &cchDestLength,
            NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        size_t cchToAppend = cbToAppend / sizeof(wchar_t);

        if (cchToAppend > NTSTRSAFE_MAX_LENGTH)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            status = RtlStringCopyWorkerW(pszDest + cchDestLength,
                    cchDest - cchDestLength,
                    NULL,
                    pszSrc,
                    cchToAppend);
        }
    }

    return status;
}


#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCchCatNEx(
  _Inout_updates_(cchDest) _Always_(_Post_z_) LPTSTR  pszDest         OPTIONAL,
  _In_     size_t  cchDest,
  _In_     LPCTSTR pszSrc          OPTIONAL,
  _In_     size_t  cchToAppend,
  _Outptr_opt_result_buffer_(*pcchRemaining)    LPTSTR* ppszDestEnd     OPTIONAL,
  _Out_opt_    size_t* pcchRemaining   OPTIONAL,
  _In_     DWORD   dwFlags
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncat', with
  some additional parameters.  In addition to functionality provided by
  RtlStringCchCatN, this routine also returns a pointer to the end of the
  destination string and the number of characters left in the destination string
  including the null terminator. The flags parameter allows additional controls.

Arguments:

pszDest         -   destination string which must be null terminated

cchDest         -   size of destination buffer in characters.
length must be (_tcslen(pszDest) + min(cchToAppend, _tcslen(pszSrc)) + 1)
to hold all of the combine string plus the null
terminator.

pszSrc          -   source string

cchToAppend     -   maximum number of characters to append

ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
pointer to the end of the destination string.  If the
function appended any data, the result will point to the
null termination character

pcchRemaining   -   if pcchRemaining is non-null, the function will return the
number of characters left in the destination string,
including the null terminator

dwFlags         -   controls some details of the string copy:

STRSAFE_FILL_BEHIND_NULL
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer
behind the null terminator

STRSAFE_IGNORE_NULLS
treat NULL string pointers like empty strings (TEXT(""))

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer, and it will
be null terminated. This will overwrite any pre-existing
or truncated string

STRSAFE_NULL_ON_FAILURE
if the function fails, the destination buffer will be set
to the empty string. This will overwrite any pre-existing or
truncated string

STRSAFE_NO_TRUNCATION
if the function returns STATUS_BUFFER_OVERFLOW, pszDest
will not contain a truncated string, it will remain unchanged.

Notes:
Behavior is undefined if source and destination strings overlap.

pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
may be NULL.  An error may still be returned even though NULLS are ignored
due to insufficient space.

Return Value:

STATUS_SUCCESS -   if all of pszSrc or the first cchToAppend characters
were concatenated to pszDest and the resultant dest
string was null terminated

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the operation
failed due to insufficient space. When this error
occurs, the destination buffer is modified to contain
a truncated version of the ideal result and is null
terminated. This is useful for situations where
truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/


NTSTRSAFEDDI
    RtlStringCchCatNExA(
            _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cchDest,
            _In_reads_or_z_(cchToAppend) STRSAFE_PCNZCH pszSrc,
            _In_ size_t cchToAppend,
            _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
            _Out_opt_ size_t* pcchRemaining,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDestLength;

    status = RtlStringExValidateDestAndLengthA(pszDest,
            cchDest,
            &cchDestLength,
            NTSTRSAFE_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;

        status = RtlStringExValidateSrcA(&pszSrc, &cchToAppend, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining <= 1)
            {
                // only fail if there was actually src data to append
                if ((cchToAppend != 0) && (*pszSrc != '\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerA(pszDestEnd,
                        cchRemaining,
                        &cchCopied,
                        pszSrc,
                        cchToAppend);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;

                if (NT_SUCCESS(status)                           &&
                        (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                        (cchRemaining > 1))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = cchRemaining * sizeof(char);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(char) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
            cbDest = cchDest * sizeof(char);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                    cbDest,
                    cchDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }

    return status;
}



NTSTRSAFEDDI
    RtlStringCchCatNExW(
            _Inout_updates_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cchDest,
            _In_reads_or_z_(cchToAppend) STRSAFE_PCNZWCH pszSrc,
            _In_ size_t cchToAppend,
            _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
            _Out_opt_ size_t* pcchRemaining,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDestLength;

    status = RtlStringExValidateDestAndLengthW(pszDest,
            cchDest,
            &cchDestLength,
            NTSTRSAFE_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;

        status = RtlStringExValidateSrcW(&pszSrc, &cchToAppend, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining <= 1)
            {
                // only fail if there was actually src data to append
                if ((cchToAppend != 0) && (*pszSrc != L'\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerW(pszDestEnd,
                        cchRemaining,
                        &cchCopied,
                        pszSrc,
                        cchToAppend);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;

                if (NT_SUCCESS(status)                           &&
                        (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                        (cchRemaining > 1))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = cchRemaining * sizeof(wchar_t);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(wchar_t) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            cbDest = cchDest * sizeof(wchar_t);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                    cbDest,
                    cchDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }

    return status;
}


#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCbCatNEx(
  _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) LPTSTR  pszDest         OPTIONAL,
  _In_     size_t  cbDest,
  _In_     LPCTSTR pszSrc          OPTIONAL,
  _In_     size_t  cbToAppend,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining)    LPTSTR* ppszDestEnd     OPTIONAL,
  _Out_opt_    size_t* pcchRemaining   OPTIONAL,
  _In_     DWORD   dwFlags
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncat', with
  some additional parameters.  In addition to functionality provided by
  RtlStringCbCatN, this routine also returns a pointer to the end of the
  destination string and the number of bytes left in the destination string
  including the null terminator. The flags parameter allows additional controls.

Arguments:

pszDest         -   destination string which must be null terminated

cbDest          -   size of destination buffer in bytes.
length must be ((_tcslen(pszDest) + min(cbToAppend / sizeof(TCHAR), _tcslen(pszSrc)) + 1) * sizeof(TCHAR)
to hold all of the combine string plus the null
terminator.

pszSrc          -   source string

cbToAppend      -   maximum number of bytes to append

ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
pointer to the end of the destination string.  If the
function appended any data, the result will point to the
null termination character

pcbRemaining    -   if pcbRemaining is non-null, the function will return the
number of bytes left in the destination string,
including the null terminator

dwFlags         -   controls some details of the string copy:

STRSAFE_FILL_BEHIND_NULL
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer
behind the null terminator

STRSAFE_IGNORE_NULLS
treat NULL string pointers like empty strings (TEXT(""))

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer, and it will
be null terminated. This will overwrite any pre-existing
or truncated string

STRSAFE_NULL_ON_FAILURE
if the function fails, the destination buffer will be set
to the empty string. This will overwrite any pre-existing or
truncated string

STRSAFE_NO_TRUNCATION
if the function returns STATUS_BUFFER_OVERFLOW, pszDest
will not contain a truncated string, it will remain unchanged.

Notes:
Behavior is undefined if source and destination strings overlap.

pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
may be NULL.  An error may still be returned even though NULLS are ignored
due to insufficient space.

Return Value:

STATUS_SUCCESS -   if all of pszSrc or the first cbToAppend bytes were
concatenated to pszDest and the resultant dest string
was null terminated

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the operation
failed due to insufficient space. When this error
occurs, the destination buffer is modified to contain
a truncated version of the ideal result and is null
terminated. This is useful for situations where
truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/


NTSTRSAFEDDI
    RtlStringCbCatNExA(
            _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cbDest,
            _In_reads_bytes_(cbToAppend) STRSAFE_PCNZCH pszSrc,
            _In_ size_t cbToAppend,
            _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
            _Out_opt_ size_t* pcbRemaining,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);
    size_t cchDestLength;

    status = RtlStringExValidateDestAndLengthA(pszDest,
            cchDest,
            &cchDestLength,
            NTSTRSAFE_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;
        size_t cchToAppend = cbToAppend / sizeof(char);

        status = RtlStringExValidateSrcA(&pszSrc, &cchToAppend, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining <= 1)
            {
                // only fail if there was actually src data to append
                if ((cchToAppend != 0) && (*pszSrc != '\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerA(pszDestEnd,
                        cchRemaining,
                        &cchCopied,
                        pszSrc,
                        cchToAppend);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;

                if (NT_SUCCESS(status)                           &&
                        (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                        (cchRemaining > 1))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                    cbDest,
                    cchDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
            }
        }
    }

    return status;
}



#pragma warning(push)
#pragma warning(disable : __WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY)

NTSTRSAFEDDI
    RtlStringCbCatNExW(
            _Inout_updates_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cbDest,
            _In_reads_bytes_(cbToAppend) STRSAFE_PCNZWCH pszSrc,
            _In_ size_t cbToAppend,
            _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
            _Out_opt_ size_t* pcbRemaining,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);
    size_t cchDestLength;

    status = RtlStringExValidateDestAndLengthW(pszDest,
            cchDest,
            &cchDestLength,
            NTSTRSAFE_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;
        size_t cchToAppend = cbToAppend / sizeof(wchar_t);

        status = RtlStringExValidateSrcW(&pszSrc, &cchToAppend, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining <= 1)
            {
                // only fail if there was actually src data to append
                if ((cchToAppend != 0) && (*pszSrc != L'\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWorkerW(pszDestEnd,
                        cchRemaining,
                        &cchCopied,
                        pszSrc,
                        cchToAppend);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;

                if (NT_SUCCESS(status) && (dwFlags & STRSAFE_FILL_BEHIND_NULL))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                    cbDest,
                    cchDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
            }
        }
    }

    return status;
}

#pragma warning(pop)


#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCchVPrintf(
  _Out_writes_(cchDest) _Always_(_Post_z_) LPTSTR  pszDest,
  _In_  size_t  cchDest,
  _In_ _Printf_format_string_  LPCTSTR pszFormat,
  _In_  va_list argList
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'vsprintf'.
  The size of the destination buffer (in characters) is a parameter and
  this function will not write past the end of this buffer and it will
  ALWAYS null terminate the destination buffer (unless it is zero length).

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the string was printed without truncation and null terminated,
  otherwise it will return a failure code. In failure cases it will return
  a truncated version of the ideal result.

Arguments:

pszDest     -  destination string

cchDest     -  size of destination buffer in characters
length must be sufficient to hold the resulting formatted
string, including the null terminator.

pszFormat   -  format string which must be null terminated

argList     -  va_list from the variable arguments according to the
stdarg.h convention

Notes:
Behavior is undefined if destination, format strings or any arguments
strings overlap.

pszDest and pszFormat should not be NULL.  See RtlStringCchVPrintfEx if you
require the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
the resultant string and it was null terminated.

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the print
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result and is
null terminated. This is useful for situations where
truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/


#pragma warning(push)
#pragma warning(disable: __WARNING_POSTCONDITION_NULLTERMINATION_VIOLATION)

NTSTRSAFEDDI
    RtlStringCchVPrintfA(
            _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cchDest,
            _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
            _In_ va_list argList)
{
    NTSTATUS status;

    status = RtlStringValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        status = RtlStringVPrintfWorkerA(pszDest,
                cchDest,
                NULL,
                pszFormat,
                argList);
    }
    else if (cchDest > 0)
    {
        *pszDest = '\0';
    }

    return status;
}

#pragma warning(pop)



#pragma warning(push)
#pragma warning(disable: __WARNING_POSTCONDITION_NULLTERMINATION_VIOLATION)

NTSTRSAFEDDI
    RtlStringCchVPrintfW(
            _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cchDest,
            _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
            _In_ va_list argList)
{
    NTSTATUS status;

    status = RtlStringValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        status = RtlStringVPrintfWorkerW(pszDest,
                cchDest,
                NULL,
                pszFormat,
                argList);
    }
    else if (cchDest > 0)
    {
        *pszDest = L'\0';
    }

    return status;
}

#pragma warning(pop)


#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCbVPrintf(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) LPTSTR  pszDest,
  _In_ size_t  cbDest,
  _In_ _Printf_format_string_ LPCTSTR pszFormat,
  _In_ va_list argList
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'vsprintf'.
  The size of the destination buffer (in bytes) is a parameter and
  this function will not write past the end of this buffer and it will
  ALWAYS null terminate the destination buffer (unless it is zero length).

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the string was printed without truncation and null terminated,
  otherwise it will return a failure code. In failure cases it will return
  a truncated version of the ideal result.

Arguments:

pszDest     -  destination string

cbDest      -  size of destination buffer in bytes
length must be sufficient to hold the resulting formatted
string, including the null terminator.

pszFormat   -  format string which must be null terminated

argList     -  va_list from the variable arguments according to the
stdarg.h convention

Notes:
Behavior is undefined if destination, format strings or any arguments
strings overlap.

pszDest and pszFormat should not be NULL.  See RtlStringCbVPrintfEx if you
require the handling of NULL values.


Return Value:

STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
the resultant string and it was null terminated.

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the print
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result and is
null terminated. This is useful for situations where
truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/


NTSTRSAFEDDI
    RtlStringCbVPrintfA(
            _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cbDest,
            _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
            _In_ va_list argList)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);

    status = RtlStringValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        status = RtlStringVPrintfWorkerA(pszDest,
                cchDest,
                NULL,
                pszFormat,
                argList);
    }
    else if (cchDest > 0)
    {
        *pszDest = '\0';
    }

    return status;
}



#pragma warning(push)
#pragma warning(disable : __WARNING_POSTCONDITION_NULLTERMINATION_VIOLATION)

NTSTRSAFEDDI
    RtlStringCbVPrintfW(
            _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cbDest,
            _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
            _In_ va_list argList)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);

    status = RtlStringValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        status = RtlStringVPrintfWorkerW(pszDest,
                cchDest,
                NULL,
                pszFormat,
                argList);
    }
    else if (cchDest > 0)
    {
        *pszDest = '\0';
    }

    return status;
}

#pragma warning(pop)


#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef _M_CEE_PURE

#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCchPrintf(
  _Out_writes_(cchDest) _Always_(_Post_z_) LPTSTR  pszDest,
  _In_ size_t  cchDest,
  _In_ _Printf_format_string_  LPCTSTR pszFormat,
  ...
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'sprintf'.
  The size of the destination buffer (in characters) is a parameter and
  this function will not write past the end of this buffer and it will
  ALWAYS null terminate the destination buffer (unless it is zero length).

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the string was printed without truncation and null terminated,
  otherwise it will return a failure code. In failure cases it will return
  a truncated version of the ideal result.

Arguments:

pszDest     -  destination string

cchDest     -  size of destination buffer in characters
length must be sufficient to hold the resulting formatted
string, including the null terminator.

pszFormat   -  format string which must be null terminated

...         -  additional parameters to be formatted according to
the format string

Notes:
Behavior is undefined if destination, format strings or any arguments
strings overlap.

pszDest and pszFormat should not be NULL.  See RtlStringCchPrintfEx if you
require the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
the resultant string and it was null terminated.

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the print
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result and is
null terminated. This is useful for situations where
truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/


NTSTRSAFEDDI
    RtlStringCchPrintfA(
            _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cchDest,
            _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
            ...)
{
    NTSTATUS status;

    status = RtlStringValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        va_list argList;

        va_start(argList, pszFormat);

        status = RtlStringVPrintfWorkerA(pszDest,
                cchDest,
                NULL,
                pszFormat,
                argList);

        va_end(argList);
    }
    else if (cchDest > 0)
    {
        *pszDest = '\0';
    }

    return status;
}



NTSTRSAFEDDI
    RtlStringCchPrintfW(
            _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cchDest,
            _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
            ...)
{
    NTSTATUS status;

    status = RtlStringValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        va_list argList;

        va_start(argList, pszFormat);

        status = RtlStringVPrintfWorkerW(pszDest,
                cchDest,
                NULL,
                pszFormat,
                argList);

        va_end(argList);
    }
    else if (cchDest > 0)
    {
        *pszDest = '\0';
    }

    return status;
}


#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCbPrintf(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) LPTSTR  pszDest,
  _In_ size_t  cbDest,
  _In_ _Printf_format_string_ LPCTSTR pszFormat,
  ...
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'sprintf'.
  The size of the destination buffer (in bytes) is a parameter and
  this function will not write past the end of this buffer and it will
  ALWAYS null terminate the destination buffer (unless it is zero length).

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the string was printed without truncation and null terminated,
  otherwise it will return a failure code. In failure cases it will return
  a truncated version of the ideal result.

Arguments:

pszDest     -  destination string

cbDest      -  size of destination buffer in bytes
length must be sufficient to hold the resulting formatted
string, including the null terminator.

pszFormat   -  format string which must be null terminated

...         -  additional parameters to be formatted according to
the format string

Notes:
Behavior is undefined if destination, format strings or any arguments
strings overlap.

pszDest and pszFormat should not be NULL.  See RtlStringCbPrintfEx if you
require the handling of NULL values.


Return Value:

STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
the resultant string and it was null terminated.

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the print
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result and is
null terminated. This is useful for situations where
truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/


NTSTRSAFEDDI
    RtlStringCbPrintfA(
            _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cbDest,
            _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
            ...)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);

    status = RtlStringValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        va_list argList;

        va_start(argList, pszFormat);

        status = RtlStringVPrintfWorkerA(pszDest,
                cchDest,
                NULL,
                pszFormat,
                argList);

        va_end(argList);
    }
    else if (cchDest > 0)
    {
        *pszDest = '\0';
    }

    return status;
}



#pragma warning(push)
#pragma warning(disable : __WARNING_POSTCONDITION_NULLTERMINATION_VIOLATION)

NTSTRSAFEDDI
    RtlStringCbPrintfW(
            _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cbDest,
            _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
            ...)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);

    status = RtlStringValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH);

    if (NT_SUCCESS(status))
    {
        va_list argList;

        va_start(argList, pszFormat);

        status = RtlStringVPrintfWorkerW(pszDest,
                cchDest,
                NULL,
                pszFormat,
                argList);

        va_end(argList);
    }
    else if (cchDest > 0)
    {
        *pszDest = L'\0';
    }

    return status;
}

#pragma warning(pop)


#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCchPrintfEx(
  _Out_writes_(cchDest) _Always_(_Post_z_) LPTSTR  pszDest         OPTIONAL,
  _In_  size_t  cchDest,
  _Outptr_opt_result_buffer_(*pcchRemaining) LPTSTR* ppszDestEnd     OPTIONAL,
  _Out_opt_ size_t* pcchRemaining   OPTIONAL,
  _In_ DWORD   dwFlags,
  _In_ _Printf_format_string_ LPCTSTR pszFormat       OPTIONAL,
  ...
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'sprintf' with
  some additional parameters.  In addition to functionality provided by
  RtlStringCchPrintf, this routine also returns a pointer to the end of the
  destination string and the number of characters left in the destination string
  including the null terminator. The flags parameter allows additional controls.

Arguments:

pszDest         -   destination string

cchDest         -   size of destination buffer in characters.
length must be sufficient to contain the resulting
formatted string plus the null terminator.

ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
pointer to the end of the destination string.  If the
function printed any data, the result will point to the
null termination character

pcchRemaining   -   if pcchRemaining is non-null, the function will return
the number of characters left in the destination string,
including the null terminator

dwFlags         -   controls some details of the string copy:

STRSAFE_FILL_BEHIND_NULL
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer
behind the null terminator

STRSAFE_IGNORE_NULLS
treat NULL string pointers like empty strings (TEXT(""))

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer, and it will
be null terminated. This will overwrite any truncated
string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_NO_TRUNCATION /
STRSAFE_NULL_ON_FAILURE
if the function fails, the destination buffer will be set
to the empty string. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

pszFormat       -   format string which must be null terminated

...             -   additional parameters to be formatted according to
the format string

Notes:
Behavior is undefined if destination, format strings or any arguments
strings overlap.

pszDest and pszFormat should not be NULL unless the STRSAFE_IGNORE_NULLS
flag is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and
pszFormat may be NULL.  An error may still be returned even though NULLS
are ignored due to insufficient space.

Return Value:

STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
the resultant string and it was null terminated.

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the print
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result and is
null terminated. This is useful for situations where
truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/


#pragma warning(push)
#pragma warning(disable : __WARNING_INCORRECT_ANNOTATION)

NTSTRSAFEDDI
    RtlStringCchPrintfExA(
            _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cchDest,
            _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
            _Out_opt_ size_t* pcchRemaining,
            _In_ DWORD dwFlags,
            _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
            ...)
{
    NTSTATUS status;

    status = RtlStringExValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcA(&pszFormat, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;

                if (cchDest != 0)
                {
                    *pszDest = '\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually a non-empty format string
                if (*pszFormat != '\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchNewDestLength = 0;
                va_list argList;

                va_start(argList, pszFormat);

                status = RtlStringVPrintfWorkerA(pszDest,
                        cchDest,
                        &cchNewDestLength,
                        pszFormat,
                        argList);

                va_end(argList);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status)                           &&
                        (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                        (cchRemaining > 1))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = cchRemaining * sizeof(char);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = '\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(char) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
            cbDest = cchDest * sizeof(char);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                    cbDest,
                    0,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }
    else if (cchDest > 0)
    {
        *pszDest = '\0';
    }

    return status;
}

#pragma warning(pop)



#pragma warning(push)
#pragma warning(disable : __WARNING_INCORRECT_ANNOTATION)

NTSTRSAFEDDI
    RtlStringCchPrintfExW(
            _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cchDest,
            _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
            _Out_opt_ size_t* pcchRemaining,
            _In_ DWORD dwFlags,
            _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
            ...)
{
    NTSTATUS status;

    status = RtlStringExValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcW(&pszFormat, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;

                if (cchDest != 0)
                {
                    *pszDest = L'\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually a non-empty format string
                if (*pszFormat != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchNewDestLength = 0;
                va_list argList;

                va_start(argList, pszFormat);

                status = RtlStringVPrintfWorkerW(pszDest,
                        cchDest,
                        &cchNewDestLength,
                        pszFormat,
                        argList);

                va_end(argList);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status)                           &&
                        (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                        (cchRemaining > 1))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = cchRemaining * sizeof(wchar_t);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = L'\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(wchar_t) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            cbDest = cchDest * sizeof(wchar_t);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                    cbDest,
                    0,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }
    else if (cchDest > 0)
    {
        *pszDest = L'\0';
    }

    return status;
}

#pragma warning(pop)


#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCbPrintfEx(
  _Out_writes_bytes_(cbDest) _Always_(_Post_z_) LPTSTR  pszDest         OPTIONAL,
  _In_ size_t  cbDest,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) LPTSTR* ppszDestEnd     OPTIONAL,
  _Out_opt_ size_t* pcbRemaining    OPTIONAL,
  _In_ DWORD   dwFlags,
  _In_ _Printf_format_string_ LPCTSTR pszFormat       OPTIONAL,
  ...
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'sprintf' with
  some additional parameters.  In addition to functionality provided by
  RtlStringCbPrintf, this routine also returns a pointer to the end of the
  destination string and the number of bytes left in the destination string
  including the null terminator. The flags parameter allows additional controls.

Arguments:

pszDest         -   destination string

cbDest          -   size of destination buffer in bytes.
length must be sufficient to contain the resulting
formatted string plus the null terminator.

ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
pointer to the end of the destination string.  If the
function printed any data, the result will point to the
null termination character

pcbRemaining    -   if pcbRemaining is non-null, the function will return
the number of bytes left in the destination string,
including the null terminator

dwFlags         -   controls some details of the string copy:

STRSAFE_FILL_BEHIND_NULL
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer
behind the null terminator

STRSAFE_IGNORE_NULLS
treat NULL string pointers like empty strings (TEXT(""))

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer, and it will
be null terminated. This will overwrite any truncated
string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_NO_TRUNCATION /
STRSAFE_NULL_ON_FAILURE
if the function fails, the destination buffer will be set
to the empty string. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

pszFormat       -   format string which must be null terminated

...             -   additional parameters to be formatted according to
the format string

Notes:
Behavior is undefined if destination, format strings or any arguments
strings overlap.

pszDest and pszFormat should not be NULL unless the STRSAFE_IGNORE_NULLS
flag is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and
pszFormat may be NULL.  An error may still be returned even though NULLS
are ignored due to insufficient space.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all concatenated and
the resultant dest string was null terminated

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the print
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result and is
null terminated. This is useful for situations where
truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/


#pragma warning(push)
#pragma warning(disable : __WARNING_INCORRECT_ANNOTATION)

NTSTRSAFEDDI
    RtlStringCbPrintfExA(
            _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cbDest,
            _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
            _Out_opt_ size_t* pcbRemaining,
            _In_ DWORD dwFlags,
            _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
            ...)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);

    status = RtlStringExValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcA(&pszFormat, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;

                if (cchDest != 0)
                {
                    *pszDest = '\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually a non-empty format string
                if (*pszFormat != '\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
                else
                {
                    // for consistency with other use in this case...
                    __analysis_assume_nullterminated(pszDest);
                }
            }
            else
            {
                size_t cchNewDestLength = 0;
                va_list argList;

                va_start(argList, pszFormat);

                status = RtlStringVPrintfWorkerA(pszDest,
                        cchDest,
                        &cchNewDestLength,
                        pszFormat,
                        argList);

                va_end(argList);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status)                           &&
                        (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                        (cchRemaining > 1))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = '\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                    cbDest,
                    0,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
            }
        }
    }
    else if (cchDest > 0)
    {
        *pszDest = '\0';
    }

    return status;
}

#pragma warning(pop)



#pragma warning(push)
#pragma warning(disable : __WARNING_INCORRECT_ANNOTATION)
#pragma warning(disable: __WARNING_POSTCONDITION_NULLTERMINATION_VIOLATION)

NTSTRSAFEDDI
    RtlStringCbPrintfExW(
            _Out_writes_bytes_(cbDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cbDest,
            _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
            _Out_opt_ size_t* pcbRemaining,
            _In_ DWORD dwFlags,
            _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
            ...)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);

    status = RtlStringExValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcW(&pszFormat, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;

                if (cchDest != 0)
                {
                    *pszDest = L'\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually a non-empty format string
                if (*pszFormat != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
                else
                {
                    // for consistency with other use in this case...
                    __analysis_assume_nullterminated(pszDest);
                }
            }
            else
            {
                size_t cchNewDestLength = 0;
                va_list argList;

                va_start(argList, pszFormat);

                status = RtlStringVPrintfWorkerW(pszDest,
                        cchDest,
                        &cchNewDestLength,
                        pszFormat,
                        argList);

                va_end(argList);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status) && (dwFlags & STRSAFE_FILL_BEHIND_NULL))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = L'\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                    cbDest,
                    0,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
            }
        }
    }
    else if (cchDest > 0)
    {
        *pszDest = L'\0';
    }

    return status;
}

#pragma warning(pop)


#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS

#endif  // !_M_CEE_PURE


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCchVPrintfEx(
  _Out_writes_(cchDest) _Always_(_Post_z_) LPTSTR  pszDest         OPTIONAL,
  _In_ size_t  cchDest,
  _Outptr_opt_result_buffer_(*pcchRemaining) LPTSTR* ppszDestEnd     OPTIONAL,
  _Out_opt_ size_t* pcchRemaining   OPTIONAL,
  _In_ DWORD   dwFlags,
  _In_ _Printf_format_string_ LPCTSTR pszFormat       OPTIONAL,
  _In_ va_list argList
  );


  Routine Description:

  This routine is a safer version of the C built-in function 'vsprintf' with
  some additional parameters.  In addition to functionality provided by
  RtlStringCchVPrintf, this routine also returns a pointer to the end of the
  destination string and the number of characters left in the destination string
  including the null terminator. The flags parameter allows additional controls.

Arguments:

pszDest         -   destination string

cchDest         -   size of destination buffer in characters.
length must be sufficient to contain the resulting
formatted string plus the null terminator.

ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
pointer to the end of the destination string.  If the
function printed any data, the result will point to the
null termination character

pcchRemaining   -   if pcchRemaining is non-null, the function will return
the number of characters left in the destination string,
including the null terminator

dwFlags         -   controls some details of the string copy:

STRSAFE_FILL_BEHIND_NULL
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer
behind the null terminator

STRSAFE_IGNORE_NULLS
treat NULL string pointers like empty strings (TEXT(""))

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer, and it will
be null terminated. This will overwrite any truncated
string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_NO_TRUNCATION /
STRSAFE_NULL_ON_FAILURE
if the function fails, the destination buffer will be set
to the empty string. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

pszFormat       -   format string which must be null terminated

argList         -   va_list from the variable arguments according to the
stdarg.h convention

Notes:
Behavior is undefined if destination, format strings or any arguments
strings overlap.

pszDest and pszFormat should not be NULL unless the STRSAFE_IGNORE_NULLS
flag is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and
pszFormat may be NULL.  An error may still be returned even though NULLS
are ignored due to insufficient space.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all concatenated and
the resultant dest string was null terminated

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the print
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result and is
null terminated. This is useful for situations where
truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/


#pragma warning(push)
#pragma warning(disable: __WARNING_POSTCONDITION_NULLTERMINATION_VIOLATION)
#pragma warning(disable : __WARNING_INCORRECT_ANNOTATION)

NTSTRSAFEDDI
    RtlStringCchVPrintfExA(
            _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cchDest,
            _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
            _Out_opt_ size_t* pcchRemaining,
            _In_ DWORD dwFlags,
            _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
            _In_ va_list argList)
{
    NTSTATUS status;

    status = RtlStringExValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcA(&pszFormat, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;

                if (cchDest != 0)
                {
                    *pszDest = '\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually a non-empty format string
                if (*pszFormat != '\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchNewDestLength = 0;

                status = RtlStringVPrintfWorkerA(pszDest,
                        cchDest,
                        &cchNewDestLength,
                        pszFormat,
                        argList);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status)                           &&
                        (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                        (cchRemaining > 1))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = cchRemaining * sizeof(char);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = '\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(char) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
            cbDest = cchDest * sizeof(char);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                    cbDest,
                    0,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }
    else if (cchDest > 0)
    {
        *pszDest = '\0';
    }

    return status;
}

#pragma warning(pop)



#pragma warning(push)
#pragma warning(disable: __WARNING_POSTCONDITION_NULLTERMINATION_VIOLATION)
#pragma warning(disable : __WARNING_INCORRECT_ANNOTATION)

NTSTRSAFEDDI
    RtlStringCchVPrintfExW(
            _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cchDest,
            _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
            _Out_opt_ size_t* pcchRemaining,
            _In_ DWORD dwFlags,
            _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
            _In_ va_list argList)
{
    NTSTATUS status;

    status = RtlStringExValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcW(&pszFormat, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;

                if (cchDest != 0)
                {
                    *pszDest = L'\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually a non-empty format string
                if (*pszFormat != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchNewDestLength = 0;

                status = RtlStringVPrintfWorkerW(pszDest,
                        cchDest,
                        &cchNewDestLength,
                        pszFormat,
                        argList);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status)                           &&
                        (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                        (cchRemaining > 1))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = cchRemaining * sizeof(wchar_t);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = L'\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(wchar_t) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            cbDest = cchDest * sizeof(wchar_t);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                    cbDest,
                    0,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }
    else if (cchDest > 0)
    {
        *pszDest = L'\0';
    }

    return status;
}

#pragma warning(pop)


#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCbVPrintfEx(
  _Out_writes_bytes_(cbDest) LPTSTR  pszDest         OPTIONAL,
  _In_ size_t  cbDest,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) LPTSTR* ppszDestEnd     OPTIONAL,
  _Out_opt_ size_t* pcbRemaining    OPTIONAL,
  _In_ DWORD   dwFlags,
  _In_ _Printf_format_string_ LPCTSTR pszFormat       OPTIONAL,
  _In_ va_list argList
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'vsprintf' with
  some additional parameters.  In addition to functionality provided by
  RtlStringCbVPrintf, this routine also returns a pointer to the end of the
  destination string and the number of characters left in the destination string
  including the null terminator. The flags parameter allows additional controls.

Arguments:

pszDest         -   destination string

cbDest          -   size of destination buffer in bytes.
length must be sufficient to contain the resulting
formatted string plus the null terminator.

ppszDestEnd     -   if ppszDestEnd is non-null, the function will return
a pointer to the end of the destination string.  If the
function printed any data, the result will point to the
null termination character

pcbRemaining    -   if pcbRemaining is non-null, the function will return
the number of bytes left in the destination string,
including the null terminator

dwFlags         -   controls some details of the string copy:

STRSAFE_FILL_BEHIND_NULL
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer
behind the null terminator

STRSAFE_IGNORE_NULLS
treat NULL string pointers like empty strings (TEXT(""))

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer, and it will
be null terminated. This will overwrite any truncated
string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_NO_TRUNCATION /
STRSAFE_NULL_ON_FAILURE
if the function fails, the destination buffer will be set
to the empty string. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

pszFormat       -   format string which must be null terminated

argList         -   va_list from the variable arguments according to the
stdarg.h convention

Notes:
Behavior is undefined if destination, format strings or any arguments
strings overlap.

pszDest and pszFormat should not be NULL unless the STRSAFE_IGNORE_NULLS
flag is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and
pszFormat may be NULL.  An error may still be returned even though NULLS
are ignored due to insufficient space.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all concatenated and
the resultant dest string was null terminated

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

STATUS_BUFFER_OVERFLOW /
NTSTATUS_CODE(status) == ERROR_INSUFFICIENT_BUFFER
-   this return value is an indication that the print
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result and is
null terminated. This is useful for situations where
truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/


#pragma warning(push)
#pragma warning(disable : __WARNING_INCORRECT_ANNOTATION)

NTSTRSAFEDDI
    RtlStringCbVPrintfExA(
            _Out_writes_bytes_(cbDest) NTSTRSAFE_PSTR pszDest,
            _In_ size_t cbDest,
            _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
            _Out_opt_ size_t* pcbRemaining,
            _In_ DWORD dwFlags,
            _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
            _In_ va_list argList)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(char);

    status = RtlStringExValidateDestA(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcA(&pszFormat, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;

                if (cchDest != 0)
                {
                    *pszDest = '\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually a non-empty format string
                if (*pszFormat != '\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
                else
                {
                    // for consistency with other use in this case...
                    __analysis_assume_nullterminated(pszDest);
                }
            }
            else
            {
                size_t cchNewDestLength = 0;

                status = RtlStringVPrintfWorkerA(pszDest,
                        cchDest,
                        &cchNewDestLength,
                        pszFormat,
                        argList);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status)                           &&
                        (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                        (cchRemaining > 1))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                    cbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullA(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = '\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsA(pszDest,
                    cbDest,
                    0,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(char) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
                *pcbRemaining = (cchRemaining * sizeof(char)) + (cbDest % sizeof(char));
            }
        }
    }

    return status;
}

#pragma warning(pop)



#pragma warning(push)
#pragma warning(disable : __WARNING_INCORRECT_ANNOTATION)

NTSTRSAFEDDI
    RtlStringCbVPrintfExW(
            _Out_writes_bytes_(cbDest) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cbDest,
            _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
            _Out_opt_ size_t* pcbRemaining,
            _In_ DWORD dwFlags,
            _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
            _In_ va_list argList)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);

    status = RtlStringExValidateDestW(pszDest, cchDest, NTSTRSAFE_MAX_CCH, dwFlags);

    if (NT_SUCCESS(status))
    {
        NTSTRSAFE_PWSTR pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlStringExValidateSrcW(&pszFormat, NULL, NTSTRSAFE_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;

                if (cchDest != 0)
                {
                    *pszDest = L'\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually a non-empty format string
                if (*pszFormat != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
                else
                {
                    // for consistency with other use in this case...
                    __analysis_assume_nullterminated(pszDest);
                }
            }
            else
            {
                size_t cchNewDestLength = 0;

                status = RtlStringVPrintfWorkerW(pszDest,
                        cchDest,
                        &cchNewDestLength,
                        pszFormat,
                        argList);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status) && (dwFlags & STRSAFE_FILL_BEHIND_NULL))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = L'\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                    cbDest,
                    0,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
            }
        }
    }

    return status;
}

#pragma warning(pop)


#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCchLength(
  _In_reads_or_z_(cchMax) LPCTSTR psz,
  _In_ _In_range_(1, NTSTRSAFE_MAX_CCH) size_t  cchMax,
  _Out_opt_ _Deref_out_range_(<, cchMax) _Deref_out_range_(<=, _String_length_(psz)) size_t* pcchLength  OPTIONAL
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strlen'.
  It is used to make sure a string is not larger than a given length, and
  it optionally returns the current length in characters not including
  the null terminator.

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the string is non-null and the length including the null
  terminator is less than or equal to cchMax characters.

Arguments:

psz         -   string to check the length of

cchMax      -   maximum number of characters including the null terminator
that psz is allowed to contain

pcch        -   if the function succeeds and pcch is non-null, the current length
in characters of psz excluding the null terminator will be returned.
This out parameter is equivalent to the return value of strlen(psz)

Notes:
psz can be null but the function will fail

cchMax should be greater than zero or the function will fail

Return Value:

STATUS_SUCCESS -   psz is non-null and the length including the null
terminator is less than or equal to cchMax characters

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/


#pragma warning(push)
#pragma warning(disable:__WARNING_POTENTIAL_RANGE_POSTCONDITION_VIOLATION)

_Must_inspect_result_
NTSTRSAFEDDI
    RtlStringCchLengthA(
            _In_reads_or_z_(cchMax) STRSAFE_PCNZCH psz,
            _In_ _In_range_(1, NTSTRSAFE_MAX_CCH) size_t cchMax,
            _Out_opt_ _Deref_out_range_(<, cchMax) _Deref_out_range_(<=, _String_length_(psz)) size_t* pcchLength)
{
    NTSTATUS status;

    if ((psz == NULL) || (cchMax > NTSTRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringLengthWorkerA(psz, cchMax, pcchLength);
    }

    if (!NT_SUCCESS(status) && pcchLength)
    {
        *pcchLength = 0;
    }

    return status;
}

#pragma warning(pop)



#pragma warning(push)
#pragma warning(disable:__WARNING_POTENTIAL_RANGE_POSTCONDITION_VIOLATION)

_Must_inspect_result_
NTSTRSAFEDDI
    RtlStringCchLengthW(
            _In_reads_or_z_(cchMax) STRSAFE_PCNZWCH psz,
            _In_ _In_range_(1, NTSTRSAFE_MAX_CCH) size_t cchMax,
            _Out_opt_ _Deref_out_range_(<, cchMax) _Deref_out_range_(<=, _String_length_(psz)) size_t* pcchLength)
{
    NTSTATUS status;

    if ((psz == NULL) || (cchMax > NTSTRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringLengthWorkerW(psz, cchMax, pcchLength);
    }

    if (!NT_SUCCESS(status) && pcchLength)
    {
        *pcchLength = 0;
    }

    return status;
}

#pragma warning(pop)


#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlStringCbLength(
  _In_reads_or_z_(cbMax) LPCTSTR psz,
  _In_ _In_range_(1, NTSTRSAFE_MAX_CCH * sizeof(TCHAR)) size_t  cbMax,
  _Out_opt_ _Deref_out_range_(<, cbMax) size_t* pcbLength   OPTIONAL
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strlen'.
  It is used to make sure a string is not larger than a given length, and
  it optionally returns the current length in bytes not including
  the null terminator.

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the string is non-null and the length including the null
  terminator is less than or equal to cbMax bytes.

Arguments:

psz         -   string to check the length of

cbMax       -   maximum number of bytes including the null terminator
that psz is allowed to contain

pcb         -   if the function succeeds and pcb is non-null, the current length
in bytes of psz excluding the null terminator will be returned.
This out parameter is equivalent to the return value of strlen(psz) * sizeof(TCHAR)

Notes:
psz can be null but the function will fail

cbMax should be greater than or equal to sizeof(TCHAR) or the function will fail

Return Value:

STATUS_SUCCESS -   psz is non-null and the length including the null
terminator is less than or equal to cbMax bytes

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/


_Must_inspect_result_
NTSTRSAFEDDI
    RtlStringCbLengthA(
            _In_reads_or_z_(cbMax) STRSAFE_PCNZCH psz,
            _In_ _In_range_(1, NTSTRSAFE_MAX_CCH * sizeof(char)) size_t cbMax,
            _Out_opt_ _Deref_out_range_(<, cbMax) size_t* pcbLength)
{
    NTSTATUS status;
    size_t cchMax = cbMax / sizeof(char);
    size_t cchLength = 0;

    if ((psz == NULL) || (cchMax > NTSTRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringLengthWorkerA(psz, cchMax, &cchLength);
    }

    if (pcbLength)
    {
        if (NT_SUCCESS(status))
        {
            // safe to multiply cchLength * sizeof(char) since cchLength < NTSTRSAFE_MAX_CCH and sizeof(char) is 1
            *pcbLength = cchLength * sizeof(char);
        }
        else
        {
            *pcbLength = 0;
        }
    }

    return status;
}



_Must_inspect_result_
NTSTRSAFEDDI
    RtlStringCbLengthW(
            _In_reads_or_z_(cbMax / sizeof(wchar_t)) STRSAFE_PCNZWCH psz,
            _In_ _In_range_(1, NTSTRSAFE_MAX_CCH * sizeof(wchar_t)) size_t cbMax,
            _Out_opt_ _Deref_out_range_(<, cbMax - 1) size_t* pcbLength)
{
    NTSTATUS status;
    size_t cchMax = cbMax / sizeof(wchar_t);
    size_t cchLength = 0;

    if ((psz == NULL) || (cchMax > NTSTRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlStringLengthWorkerW(psz, cchMax, &cchLength);
    }

    if (pcbLength)
    {
        if (NT_SUCCESS(status))
        {
            // safe to multiply cchLength * sizeof(wchar_t) since cchLength < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            *pcbLength = cchLength * sizeof(wchar_t);
        }
        else
        {
            *pcbLength = 0;
        }
    }

    return status;
}


#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS

#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlUnalignedStringCchLength(
  _In_ LPCUTSTR    psz,
  _In_ _In_range_(1, NTSTRSAFE_MAX_CCH) size_t  cchMax,
  _Out_opt_ _Deref_out_range_(<, cchMax) size_t*     pcchLength  OPTIONAL
  );

  Routine Description:

  This routine is a version of RtlStringCchLength that accepts an unaligned string pointer.

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the string is non-null and the length including the null
  terminator is less than or equal to cchMax characters.

Arguments:

psz         -   string to check the length of

cchMax      -   maximum number of characters including the null terminator
that psz is allowed to contain

pcch        -   if the function succeeds and pcch is non-null, the current length
in characters of psz excluding the null terminator will be returned.
This out parameter is equivalent to the return value of strlen(psz)

Notes:
psz can be null but the function will fail

cchMax should be greater than zero or the function will fail

Return Value:

STATUS_SUCCESS -   psz is non-null and the length including the null
terminator is less than or equal to cchMax characters

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/


#ifdef ALIGNMENT_MACHINE
_Must_inspect_result_
NTSTRSAFEDDI
    RtlUnalignedStringCchLengthW(
            _In_reads_or_z_(cchMax) STRSAFE_PCUNZWCH psz,
            _In_ _In_range_(1, NTSTRSAFE_MAX_CCH) size_t cchMax,
            _Out_opt_ _Deref_out_range_(<, cchMax) size_t* pcchLength)
{
    NTSTATUS status;

    if ((psz == NULL) || (cchMax > NTSTRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlUnalignedStringLengthWorkerW(psz, cchMax, pcchLength);
    }

    if (!NT_SUCCESS(status) && pcchLength)
    {
        *pcchLength = 0;
    }

    return status;
}

#else
#define RtlUnalignedStringCchLengthW   RtlStringCchLengthW
#endif  // !ALIGNMENT_MACHINE


#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS

#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlUnalignedStringCbLength(
  _In_reads_or_z_(cbMax) LPCUTSTR    psz,
  _In_ _In_range_(1, NTSTRSAFE_MAX_CCH * sizeof(TCHAR)) size_t  cbMax,
  _Out_opt_ _Deref_out_range_(<, cbMax) size_t*   pcbLength   OPTIONAL
  );

  Routine Description:

  This routine is a version of RtlStringCbLength that accepts an unaligned string pointer.

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the string is non-null and the length including the null
  terminator is less than or equal to cbMax bytes.

Arguments:

psz         -   string to check the length of

cbMax       -   maximum number of bytes including the null terminator
that psz is allowed to contain

pcb         -   if the function succeeds and pcb is non-null, the current length
in bytes of psz excluding the null terminator will be returned.
This out parameter is equivalent to the return value of strlen(psz) * sizeof(TCHAR)

Notes:
psz can be null but the function will fail

cbMax should be greater than or equal to sizeof(TCHAR) or the function will fail

Return Value:

STATUS_SUCCESS -   psz is non-null and the length including the null
terminator is less than or equal to cbMax bytes

failure        -   you can use the macro NTSTATUS_CODE() to get a win32
error code for all hresult failure cases

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/


#ifdef ALIGNMENT_MACHINE
_Must_inspect_result_
NTSTRSAFEDDI
    RtlUnalignedStringCbLengthW(
            _In_reads_or_z_(cbMax / sizeof(wchar_t)) STRSAFE_PCUNZWCH psz,
            _In_ _In_range_(1, NTSTRSAFE_MAX_CCH * sizeof(wchar_t)) size_t cbMax,
            _Out_opt_ _Deref_out_range_(<, cbMax - 1) size_t* pcbLength)
{
    NTSTATUS status;
    size_t cchMax = cbMax / sizeof(wchar_t);
    size_t cchLength = 0;

    if ((psz == NULL) || (cchMax > NTSTRSAFE_MAX_CCH))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlUnalignedStringLengthWorkerW(psz, cchMax, &cchLength);
    }

    if (pcbLength)
    {
        if (NT_SUCCESS(status))
        {
            // safe to multiply cchLength * sizeof(wchar_t) since cchLength < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            *pcbLength = cchLength * sizeof(wchar_t);
        }
        else
        {
            *pcbLength = 0;
        }
    }

    return status;
}
#else
#define RtlUnalignedStringCbLengthW    RtlStringCbLengthW

#endif  // !ALIGNMENT_MACHINE


#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS



#ifndef NTSTRSAFE_NO_UNICODE_STRING_FUNCTIONS

#pragma warning(push)
#pragma warning(disable : __WARNING_INCORRECT_ANNOTATION)
#pragma warning(disable : __WARNING_HIGH_PRIORITY_OVERFLOW_POSTCONDITION)

/*++

  NTSTATUS
  RtlUnicodeStringInit(
  _Out_ PUNICODE_STRING DestinationString,
  _In_opt_ NTSTRSAFE_PCWSTR pszSrc              OPTIONAL
  );

  Routine Description:

  The RtlUnicodeStringInit function initializes a counted unicode string from
  pszSrc.

  This function returns an NTSTATUS value.  It returns STATUS_SUCCESS if the
  counted unicode string was sucessfully initialized from pszSrc. In failure
  cases the unicode string buffer will be set to NULL, and the Length and
  MaximumLength members will be set to zero.

Arguments:

DestinationString - pointer to the counted unicode string to be initialized

pszSrc            - source string which must be null or null terminated

Notes:
DestinationString should not be NULL. See RtlUnicodeStringInitEx if you require
the handling of NULL values.

Return Value:

STATUS_SUCCESS -

failure        -   the operation did not succeed

STATUS_INVALID_PARAMETER
-   this return value is an indication that the source string
was too large and DestinationString could not be initialized

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/

_At_(DestinationString->Buffer, _Post_equal_to_(pszSrc))
_At_(DestinationString->Length, _Post_equal_to_(_String_length_(pszSrc) * sizeof(WCHAR)))
_At_(DestinationString->MaximumLength, _Post_equal_to_((_String_length_(pszSrc)+1) * sizeof(WCHAR)))
NTSTRSAFEDDI
RtlUnicodeStringInit(
        _Out_ PUNICODE_STRING DestinationString,
        _In_opt_ NTSTRSAFE_PCWSTR pszSrc)
{
    return RtlUnicodeStringInitWorker(DestinationString,
            pszSrc,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            0);
}


/*++

  NTSTATUS
  RtlUnicodeStringInitEx(
  _Out_ PUNICODE_STRING DestinationString,
  _In_opt_  NTSTRSAFE_PCWSTR pszSrc              OPTIONAL,
  _In_ DWORD           dwFlags
  );

  Routine Description:

  In addition to functionality provided by RtlUnicodeStringInit, this routine
  includes the flags parameter allows additional controls.

  This function returns an NTSTATUS value.  It returns STATUS_SUCCESS if the
  counted unicode string was sucessfully initialized from pszSrc. In failure
  cases the unicode string buffer will be set to NULL, and the Length and
  MaximumLength members will be set to zero.

Arguments:

DestinationString - pointer to the counted unicode string to be initialized

pszSrc            - source string which must be null terminated

dwFlags           - controls some details of the initialization:

STRSAFE_IGNORE_NULLS
do not fault on a NULL DestinationString pointer

Return Value:

STATUS_SUCCESS -

failure        -   the operation did not succeed

STATUS_INVALID_PARAMETER

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/

_At_(DestinationString->Buffer, _Post_equal_to_(pszSrc))
_At_(DestinationString->Length, _Post_equal_to_(_String_length_(pszSrc) * sizeof(WCHAR)))
_At_(DestinationString->MaximumLength, _Post_equal_to_((_String_length_(pszSrc)+1) * sizeof(WCHAR)))
NTSTRSAFEDDI
RtlUnicodeStringInitEx(
        _Out_ PUNICODE_STRING DestinationString,
        _In_opt_ NTSTRSAFE_PCWSTR pszSrc,
        _In_ DWORD dwFlags)
{
    NTSTATUS status;

    if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlUnicodeStringInitWorker(DestinationString,
                pszSrc,
                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                dwFlags);
    }

    if (!NT_SUCCESS(status) && DestinationString)
    {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        DestinationString->Buffer = NULL;
    }

    return status;
}


/*++

  NTSTATUS
  RtlUnicodeStringValidate(
  _In_ PCUNICODE_STRING    SourceString
  );

  Routine Description:

  The RtlUnicodeStringValidate function checks the counted unicode string to make
  sure that is is valid.

  This function returns an NTSTATUS value.  It returns STATUS_SUCCESS if the
  counted unicode string is valid.

Arguments:

SourceString   - pointer to the counted unicode string to be checked

Notes:
SourceString should not be NULL. See RtlUnicodeStringValidateEx if you require
the handling of NULL values.

Return Value:

STATUS_SUCCESS -   SourceString is a valid counted unicode string

failure        -   the operation did not succeed

STATUS_INVALID_PARAMETER
-   this return value is an indication that SourceString is not a valid
counted unicode string

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/

NTSTRSAFEDDI
RtlUnicodeStringValidate(
        _In_ PCUNICODE_STRING SourceString)
{
    return RtlUnicodeStringValidateWorker(SourceString, NTSTRSAFE_UNICODE_STRING_MAX_CCH, 0);
}


/*++

  NTSTATUS
  RtlUnicodeStringValidateEx(
  _In_ PCUNICODE_STRING    SourceString     OPTIONAL,
  _In_ DWORD               dwFlags
  );

  Routine Description:

  In addition to functionality provided by RtlUnicodeStringValidate, this routine
  includes the flags parameter allows additional controls.

  This function returns an NTSTATUS value.  It returns STATUS_SUCCESS if the
  counted unicode string is valid.

Arguments:

SourceString   - pointer to the counted unicode string to be checked

dwFlags        - controls some details of the validation:

STRSAFE_IGNORE_NULLS
allows SourceString to be NULL (will return STATUS_SUCCESS for this case).

Return Value:

STATUS_SUCCESS -   SourceString is a valid counted unicode string

failure        -   the operation did not succeed

STATUS_INVALID_PARAMETER
-   this return value is an indication that the source string
is not a valide counted unicode string given the flags passed.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/

NTSTRSAFEDDI
RtlUnicodeStringValidateEx(
        _In_ PCUNICODE_STRING SourceString,
        _In_ DWORD dwFlags)
{
    NTSTATUS status;

    if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        status = RtlUnicodeStringValidateWorker(SourceString, NTSTRSAFE_UNICODE_STRING_MAX_CCH, dwFlags);
    }

    return status;
}


/*++

  NTSTATUS
  RtlStringCchCopyUnicodeString(
  _Out_writes_(cchDest) PWSTR               pszDest,
  _In_ size_t              cchDest,
  _In_ PCUNICODE_STRING    SourceString,
  );

  Routine Description:

  This routine copies a PUNICODE_STRING to a PWSTR. This function will not
  write past the end of this buffer and it will ALWAYS null terminate the
  destination buffer (unless it is zero length).

  This function returns an NTSTATUS value, and not a pointer. It returns
  STATUS_SUCCESS if the string was copied without truncation, otherwise it
  will return a failure code. In failure cases as much of SourceString will be
  copied to pszDest as possible.

Arguments:

pszDest        -   destination string

cchDest        -   size of destination buffer in characters.
length must be = ((DestinationString->Length / sizeof(wchar_t)) + 1)
to hold all of the source and null terminate the string.

SourceString   -   pointer to the counted unicode source string

Notes:
Behavior is undefined if source and destination strings overlap.

SourceString and pszDest should not be NULL.  See RtlStringCchCopyUnicodeStringEx
if you require the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied and the
resultant dest string was null terminated

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result and is
null terminated. This is useful for situations where
truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/

NTSTRSAFEDDI
    RtlStringCchCopyUnicodeString(
            _Out_writes_(cchDest) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cchDest,
            _In_ PCUNICODE_STRING SourceString)
{
    NTSTATUS status;

    status = RtlStringValidateDestW(pszDest,
            cchDest,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH);
    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                &pszSrc,
                &cchSrcLength,
                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                0);

        if (NT_SUCCESS(status))
        {
            status = RtlStringCopyWideCharArrayWorker(pszDest,
                    cchDest,
                    NULL,
                    pszSrc,
                    cchSrcLength);
        }
        else
        {
            *pszDest = L'\0';
        }
    }

    return status;
}


/*++

  NTSTATUS
  RtlStringCbCopyUnicodeString(
  _Out_writes_bytes_(cbDest) PWSTR               pszDest,
  _In_ size_t              cbDest,
  _In_ PCUNICODE_STRING    SourceString,
  );

  Routine Description:

  This routine copies a PUNICODE_STRING to a PWSTR. This function will not
  write past the end of this buffer and it will ALWAYS null terminate the
  destination buffer (unless it is zero length).

  This function returns an NTSTATUS value, and not a pointer. It returns
  STATUS_SUCCESS if the string was copied without truncation, otherwise it
  will return a failure code. In failure cases as much of SourceString will be
  copied to pszDest as possible.

Arguments:

pszDest        -   destination string

cbDest         -   size of destination buffer in bytes.
length must be = (DestinationString->Length + sizeof(wchar_t))
to hold all of the source and null terminate the string.

SourceString   -   pointer to the counted unicode source string

Notes:
Behavior is undefined if source and destination strings overlap.

SourceString and pszDest should not be NULL.  See RtlStringCbCopyUnicodeStringEx
if you require the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied and the
resultant dest string was null terminated

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result and is
null terminated. This is useful for situations where
truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/

#pragma warning(push)
#pragma warning(disable : __WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY)

NTSTRSAFEDDI
    RtlStringCbCopyUnicodeString(
            _Out_writes_bytes_(cbDest) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cbDest,
            _In_ PCUNICODE_STRING SourceString)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);

    status = RtlStringValidateDestW(pszDest,
            cchDest,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH);
    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                &pszSrc,
                &cchSrcLength,
                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                0);

        if (NT_SUCCESS(status))
        {
            status = RtlStringCopyWideCharArrayWorker(pszDest,
                    cchDest,
                    NULL,
                    pszSrc,
                    cchSrcLength);
        }
        else
        {
            *pszDest = L'\0';
        }
    }

    return status;
}

#pragma warning(pop)

/*++

  NTSTATUS
  RtlStringCchCopyUnicodeStringEx(
  _Out_writes_(cchDest) PWSTR               pszDest         OPTIONAL,
  _In_ size_t              cchDest,
  _In_ PCUNICODE_STRING    SourceString    OPTIONAL,
  _Outptr_opt_result_buffer_(*pcchRemaining) PWSTR*              ppszDestEnd     OPTIONAL,
  _Out_opt_ size_t*             pcchRemaining   OPTIONAL,
  _In_ DWORD               dwFlags
  );

  Routine Description:

  This routine copies a PUNICODE_STRING to a PWSTR. In addition to
  functionality provided by RtlStringCchCopyUnicodeString, this routine also
  returns a pointer to the end of the destination string and the number of
  characters left in the destination string including the null terminator.
  The flags parameter allows additional controls.

Arguments:

pszDest         -   destination string

cchDest         -   size of destination buffer in characters.
length must be = ((DestinationString->Length / sizeof(wchar_t)) + 1)
to hold all of the source and null terminate the string.

SourceString    -   pointer to the counted unicode source string

ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
pointer to the end of the destination string.  If the
function copied any data, the result will point to the
null termination character

pcchRemaining   -   if pcchRemaining is non-null, the function will return the
number of characters left in the destination string,
including the null terminator

dwFlags         -   controls some details of the string copy:

STRSAFE_FILL_BEHIND_NULL
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer
behind the null terminator

STRSAFE_IGNORE_NULLS
treat NULL string pointers like empty strings (TEXT("")).
this flag is useful for emulating functions like lstrcpy

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer, and it will
be null terminated. This will overwrite any truncated
string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_NO_TRUNCATION /
STRSAFE_NULL_ON_FAILURE
if the function fails, the destination buffer will be set
to the empty string. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
Behavior is undefined if source and destination strings overlap.

pszDest and SourceString should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and SourceString
may be NULL.  An error may still be returned even though NULLS are ignored
due to insufficient space.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied and the
resultant dest string was null terminated

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result. This is
useful for situations where truncation is ok

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/

NTSTRSAFEDDI
    RtlStringCchCopyUnicodeStringEx(
            _Out_writes_(cchDest) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cchDest,
            _In_ PCUNICODE_STRING SourceString,
            _Outptr_opt_result_buffer_(*pcchRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
            _Out_opt_ size_t* pcchRemaining,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;

    status = RtlStringExValidateDestW(pszDest,
            cchDest,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        wchar_t* pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                &pszSrc,
                &cchSrcLength,
                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;

                if (cchDest != 0)
                {
                    *pszDest = L'\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if (cchSrcLength != 0)
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWideCharArrayWorker(pszDest,
                        cchDest,
                        &cchCopied,
                        pszSrc,
                        cchSrcLength);

                pszDestEnd = pszDest + cchCopied;
                cchRemaining = cchDest - cchCopied;

                if (NT_SUCCESS(status)                           &&
                        (dwFlags & STRSAFE_FILL_BEHIND_NULL)    &&
                        (cchRemaining > 1))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = cchRemaining * sizeof(wchar_t);

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = L'\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cchDest != 0))
        {
            size_t cbDest;

            // safe to multiply cchDest * sizeof(wchar_t) since cchDest < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
            cbDest = cchDest * sizeof(wchar_t);

            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                    cbDest,
                    0,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcchRemaining)
            {
                *pcchRemaining = cchRemaining;
            }
        }
    }

    return status;
}


/*++

  NTSTATUS
  RtlStringCbCopyUnicodeStringEx(
  _Out_writes_bytes_(cbDest) PWSTR               pszDest         OPTIONAL,
  _In_ size_t              cbDest,
  _In_ PCUNICODE_STRING    SourceString    OPTIONAL,
  _Outptr_opt_result_bytebuffer_(*pcbRemaining) PWSTR*              ppszDestEnd     OPTIONAL,
  _Out_opt_ size_t*             pcbRemaining    OPTIONAL,
  _In_ DWORD               dwFlags
  );

  Routine Description:

  This routine copies a PUNICODE_STRING to a PWSTR. In addition to
  functionality provided by RtlStringCbCopyUnicodeString, this routine also
  returns a pointer to the end of the destination string and the number of
  characters left in the destination string including the null terminator.
  The flags parameter allows additional controls.

Arguments:

pszDest         -   destination string

cchDest         -   size of destination buffer in characters.
length must be = ((DestinationString->Length / sizeof(wchar_t)) + 1)
to hold all of the source and null terminate the string.

SourceString    -   pointer to the counted unicode source string

ppszDestEnd     -   if ppszDestEnd is non-null, the function will return a
pointer to the end of the destination string.  If the
function copied any data, the result will point to the
null termination character

pcbRemaining    -   pcbRemaining is non-null,the function will return the
number of bytes left in the destination string,
including the null terminator

dwFlags         -   controls some details of the string copy:

STRSAFE_FILL_BEHIND_NULL
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer
behind the null terminator

STRSAFE_IGNORE_NULLS
treat NULL string pointers like empty strings (TEXT("")).
this flag is useful for emulating functions like lstrcpy

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer, and it will
be null terminated. This will overwrite any truncated
string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_NO_TRUNCATION /
STRSAFE_NULL_ON_FAILURE
if the function fails, the destination buffer will be set
to the empty string. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
Behavior is undefined if source and destination strings overlap.

pszDest and SourceString should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and SourceString
may be NULL.  An error may still be returned even though NULLS are ignored
due to insufficient space.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied and the
resultant dest string was null terminated

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result. This is
useful for situations where truncation is ok

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/

NTSTRSAFEDDI
    RtlStringCbCopyUnicodeStringEx(
            _Out_writes_bytes_(cbDest) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cbDest,
            _In_ PCUNICODE_STRING SourceString,
            _Outptr_opt_result_bytebuffer_(*pcbRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
            _Out_opt_ size_t* pcbRemaining,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;
    size_t cchDest = cbDest / sizeof(wchar_t);

    status = RtlStringExValidateDestW(pszDest,
            cchDest,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        wchar_t* pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                &pszSrc,
                &cchSrcLength,
                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;

                if (cchDest != 0)
                {
                    *pszDest = L'\0';
                }
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if (cchSrcLength != 0)
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlStringCopyWideCharArrayWorker(pszDest,
                        cchDest,
                        &cchCopied,
                        pszSrc,
                        cchSrcLength);

                pszDestEnd = pszDest + cchCopied;
                cchRemaining = cchDest - cchCopied;

                if (NT_SUCCESS(status) && (dwFlags & STRSAFE_FILL_BEHIND_NULL))
                {
                    size_t cbRemaining;

                    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                    cbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));

                    // handle the STRSAFE_FILL_BEHIND_NULL flag
                    RtlStringExHandleFillBehindNullW(pszDestEnd, cbRemaining, dwFlags);
                }
            }
        }
        else
        {
            if (cchDest != 0)
            {
                *pszDest = L'\0';
            }
        }

        if (!NT_SUCCESS(status)                                                                              &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE)) &&
                (cbDest != 0))
        {
            // handle the STRSAFE_FILL_ON_FAILURE, STRSAFE_NULL_ON_FAILURE, and STRSAFE_NO_TRUNCATION flags
            RtlStringExHandleOtherFlagsW(pszDest,
                    cbDest,
                    0,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (ppszDestEnd)
            {
                *ppszDestEnd = pszDestEnd;
            }

            if (pcbRemaining)
            {
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_MAX_CCH and sizeof(wchar_t) is 2
                *pcbRemaining = (cchRemaining * sizeof(wchar_t)) + (cbDest % sizeof(wchar_t));
            }
        }
    }

    return status;
}


/*++

  NTSTATUS
  RtlUnicodeStringCopyString(
  _Inout_ PUNICODE_STRING DestinationString,
  _In_  LPCTSTR         pszSrc
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strcpy' for
  UNICODE_STRINGs.

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the string was copied without truncation, otherwise it
  will return a failure code. In failure cases as much of pszSrc will be
  copied to DestinationString as possible.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

pszSrc              -   source string which must be null terminated

Notes:
Behavior is undefined if source and destination strings overlap.

DestinationString and pszSrc should not be NULL.  See RtlUnicodeStringCopyStringEx
if you require the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result. This is
useful for situations where truncation is ok

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/

NTSTRSAFEDDI
RtlUnicodeStringCopyString(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ NTSTRSAFE_PCWSTR pszSrc)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            NULL,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            0);

    if (NT_SUCCESS(status))
    {
        size_t cchNewDestLength = 0;

        status = RtlWideCharArrayCopyStringWorker(pszDest,
                cchDest,
                &cchNewDestLength,
                pszSrc,
                NTSTRSAFE_UNICODE_STRING_MAX_CCH);

        // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
        DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
    }

    return status;
}


/*++

  NTSTATUS
  RtlUnicodeStringCopy(
  _Inout_ PUNICODE_STRING    DestinationString,
  _In_ PCUNICODE_STRING    SourceString
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strcpy' for
  UNICODE_STRINGs.

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the string was copied without truncation, otherwise it
  will return a failure code. In failure cases as much of SourceString
  will be copied to Dest as possible.

Arguments:

DestinationString   - pointer to the counted unicode destination string

SourceString        -  pointer to the counted unicode source string

Notes:
Behavior is undefined if source and destination strings overlap.

DestinationString and SourceString should not be NULL.  See RtlUnicodeStringCopyEx
if you require the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result. This is
useful for situations where truncation is ok

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/

NTSTRSAFEDDI
RtlUnicodeStringCopy(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ PCUNICODE_STRING SourceString)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            NULL,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            0);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        size_t cchNewDestLength = 0;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                &pszSrc,
                &cchSrcLength,
                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                0);

        if (NT_SUCCESS(status))
        {
            status = RtlWideCharArrayCopyWorker(pszDest,
                    cchDest,
                    &cchNewDestLength,
                    pszSrc,
                    cchSrcLength);
        }

        // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
        DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
    }

    return status;
}


/*++

  NTSTATUS
  RtlUnicodeStringCopyStringEx(
  _Inout_ PUNICODE_STRING DestinationString   OPTIONAL,
  _In_ LPCTSTR         pszSrc              OPTIONAL,
  _Out_opt_ PUNICODE_STRING RemainingString     OPTIONAL,
  _In_ DWORD           dwFlags
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strcpy' for
  UNICODE_STRINGs with some additional parameters. In addition to the
  functionality provided by RtlUnicodeStringCopyString, this routine also
  returns a PUNICODE_STRING which points to the end of the destination
  string. The flags parameter allows additional controls.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

pszSrc              -   source string which must be null terminated

RemainingString     -   if RemainingString is non-null, the function will format
the pointer with the remaining buffer and number of
bytes left in the destination string

dwFlags             -   controls some details of the string copy:

STRSAFE_FILL_BEHIND
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer

STRSAFE_IGNORE_NULLS
do not fault if DestinationString is null and treat NULL pszSrc like
empty strings (L""). This flag is useful for emulating
functions like lstrcpy

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer. This will
overwrite any truncated string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_NO_TRUNCATION /
STRSAFE_ZERO_LENGTH_ON_FAILURE
if the function fails, the destination Length will be set
to zero. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
Behavior is undefined if source and destination strings overlap.

DestinationString and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and pszSrc
may be NULL.  An error may still be returned even though NULLS are ignored.

Behavior is undefined if DestinationString and RemainingString are the same pointer.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result.
This is useful for situations where truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCopyStringEx(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ NTSTRSAFE_PCWSTR pszSrc,
        _Out_opt_ PUNICODE_STRING RemainingString,
        _In_ DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            NULL,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;
        size_t cchNewDestLength = 0;

        status = RtlStringExValidateSrcW(&pszSrc, NULL, NTSTRSAFE_UNICODE_STRING_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                status = RtlWideCharArrayCopyStringWorker(pszDest,
                        cchDest,
                        &cchNewDestLength,
                        pszSrc,
                        NTSTRSAFE_UNICODE_STRING_MAX_CCH);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status)              &&
                        (dwFlags & STRSAFE_FILL_BEHIND) &&
                        (cchRemaining != 0))
                {
                    // handle the STRSAFE_FILL_BEHIND flag
                    RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                }
            }
        }


        if (!NT_SUCCESS(status)                                                                                      &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
                (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                    cchDest,
                    0,
                    &cchNewDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}


/*++

  NTSTATUS
  RtlUnicodeStringCopyEx(
  _Inout_ PUNICODE_STRING     DestinationString   OPTIONAL,
  _In_  PCUNICODE_STRING    SourceString        OPTIONAL,
  _Out_opt_ PUNICODE_STRING     RemainingString     OPTIONAL,
  _In_  DWORD               dwFlags
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strcpy' for
  UNICODE_STRINGs with some additional parameters. In addition to the
  functionality provided by RtlUnicodeStringCopy, this routine
  also returns a PUNICODE_STRING which points to the end of the destination
  string. The flags parameter allows additional controls.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

SourceString        -   pointer to the counted unicode source string

RemainingString     -   if RemainingString is non-null, the function will format
the pointer with the remaining buffer and number of
bytes left in the destination string

dwFlags             -   controls some details of the string copy:

STRSAFE_FILL_BEHIND
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer

STRSAFE_IGNORE_NULLS
do not fault if DestinationString is null and treat NULL pszSrc like
empty strings (L""). This flag is useful for emulating
functions like lstrcpy

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer. This will
overwrite any truncated string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_NO_TRUNCATION /
STRSAFE_ZERO_LENGTH_ON_FAILURE
if the function fails, the destination Length will be set
to zero. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
Behavior is undefined if source and destination strings overlap.

DestinationString and SourceString should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and SourceString
may be NULL.  An error may still be returned even though NULLS are ignored.

Behavior is undefined if DestinationString and RemainingString are the same pointer.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result.
This is useful for situations where truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCopyEx(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ PCUNICODE_STRING SourceString,
        _Out_opt_ PUNICODE_STRING RemainingString,
        _In_ DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            NULL,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        wchar_t* pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;
        size_t cchNewDestLength = 0;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                &pszSrc,
                &cchSrcLength,
                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if (cchSrcLength != 0)
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                status = RtlWideCharArrayCopyWorker(pszDest,
                        cchDest,
                        &cchNewDestLength,
                        pszSrc,
                        cchSrcLength);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status)              &&
                        (dwFlags & STRSAFE_FILL_BEHIND) &&
                        (cchRemaining != 0))
                {
                    // handle the STRSAFE_FILL_BEHIND flag
                    RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
                (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                    cchDest,
                    0,
                    &cchNewDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlUnicodeStringCchCopyStringN(
  _Inout_ PUNICODE_STRING DestinationString,
  _In_ LPCTSTR         pszSrc,
  _In_ size_t          cchToCopy
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncpy' for
  PUNICODE_STRINGs.

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the entire string or the first cchToCopy characters were
  copied without truncation, otherwise it will return a failure code. In
  failure cases as much of pszSrc will be copied to DestinationString as possible.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

pszSrc              -   source string

cchToCopy           -   maximum number of characters to copy from source string,
not including the null terminator.

Notes:
Behavior is undefined if source and destination strings overlap.

DestinationString and pszSrc should not be NULL. See RtlUnicodeStringCchCopyStringNEx if
you require the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied

failure        -   the operation did not succeed


STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result. This is
useful for situations where truncation is ok

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/

NTSTRSAFEDDI
RtlUnicodeStringCchCopyStringN(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ NTSTRSAFE_PCWSTR pszSrc,
        _In_ size_t cchToCopy)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            NULL,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            0);

    if (NT_SUCCESS(status))
    {
        size_t cchNewDestLength = 0;

        if (cchToCopy > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            status = RtlWideCharArrayCopyStringWorker(pszDest,
                    cchDest,
                    &cchNewDestLength,
                    pszSrc,
                    cchToCopy);
        }

        // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
        DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlUnicodeStringCbCopyStringN(
  _Inout_ PUNICODE_STRING DestinationString,
  _In_ LPCTSTR         pszSrc,
  _In_ size_t          cbToCopy
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncpy' for
  PUNICODE_STRINGs.

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the entire string or the first cbToCopy bytes were
  copied without truncation, otherwise it will return a failure code. In
  failure cases as much of pszSrc will be copied to DestinationString as possible.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

pszSrc              -   source string

cbToCopy            -   maximum number of bytes to copy from source string,
not including the null terminator.

Notes:
Behavior is undefined if source and destination strings overlap.

DestinationString and pszSrc should not be NULL.  See RtlUnicodeStringCopyCbStringEx if you require
the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result. This is
useful for situations where truncation is ok

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/

NTSTRSAFEDDI
RtlUnicodeStringCbCopyStringN(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ NTSTRSAFE_PCWSTR pszSrc,
        _In_ size_t cbToCopy)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            NULL,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            0);

    if (NT_SUCCESS(status))
    {
        size_t cchNewDestLength = 0;
        size_t cchToCopy = cbToCopy / sizeof(wchar_t);

        if (cchToCopy > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            status = RtlWideCharArrayCopyStringWorker(pszDest,
                    cchDest,
                    &cchNewDestLength,
                    pszSrc,
                    cchToCopy);
        }

        // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
        DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlUnicodeStringCchCopyN(
  _Inout_ PUNICODE_STRING     DestinationString,
  _In_  PCUNICODE_STRING    SourceString,
  _In_  size_t              cchToCopy
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncpy' for
  PUNICODE_STRINGs.

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the entire string or the first cchToCopy characters were
  copied without truncation, otherwise it will return a failure code. In
  failure cases as much of SourceString will be copied to DestinationString as possible.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

SourceString        -   pointer to the counted unicode source string

cchToCopy           -   maximum number of characters to copy from source string,
not including the null terminator.

Notes:
Behavior is undefined if source and destination strings overlap.

DestinationString and SourceString should not be NULL. See RtlUnicodeStringCchCopyNEx
if you require the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result. This is
useful for situations where truncation is ok

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/

NTSTRSAFEDDI
RtlUnicodeStringCchCopyN(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ PCUNICODE_STRING SourceString,
        _In_ size_t cchToCopy)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            NULL,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            0);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        size_t cchNewDestLength = 0;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                &pszSrc,
                &cchSrcLength,
                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                0);

        if (NT_SUCCESS(status))
        {
            if (cchToCopy > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                if (cchSrcLength < cchToCopy)
                {
                    cchToCopy = cchSrcLength;
                }

                status = RtlWideCharArrayCopyWorker(pszDest,
                        cchDest,
                        &cchNewDestLength,
                        pszSrc,
                        cchToCopy);
            }
        }

        // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
        DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlUnicodeStringCbCopyN(
  _Inout_ PUNICODE_STRING     DestinationString,
  _In_  PCUNICODE_STRING    SourceString,
  _In_  size_t              cbToCopy
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncpy' for
  PUNICODE_STRINGs.

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the entire string or the first cbToCopy bytes were
  copied without truncation, otherwise it will return a failure code. In
  failure cases as much of SourceString will be copied to DestinationString as possible.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

SourceString        -   pointer to the counted unicode source string

cbToCopy            -   maximum number of bytes to copy from source string,
not including the null terminator.

Notes:
Behavior is undefined if source and destination strings overlap.

DestinationString and SourceString should not be NULL.  See RtlUnicodeStringCbCopyNEx
if you require the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result. This is
useful for situations where truncation is ok

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/


NTSTRSAFEDDI
RtlUnicodeStringCbCopyN(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ PCUNICODE_STRING SourceString,
        _In_ size_t cbToCopy)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            NULL,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            0);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        size_t cchNewDestLength = 0;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                &pszSrc,
                &cchSrcLength,
                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                0);

        if (NT_SUCCESS(status))
        {
            size_t cchToCopy = cbToCopy / sizeof(wchar_t);

            if (cchToCopy > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                if (cchSrcLength < cchToCopy)
                {
                    cchToCopy = cchSrcLength;
                }

                status = RtlWideCharArrayCopyWorker(pszDest,
                        cchDest,
                        &cchNewDestLength,
                        pszSrc,
                        cchToCopy);
            }
        }

        // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
        DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlUnicodeStringCchCopyStringNEx(
  _Inout_ PUNICODE_STRING DestinationString   OPTIONAL,
  _In_ LPCTSTR         pszSrc              OPTIONAL,
  _In_ size_t          cchToCopy,
  _Out_opt_ PUNICODE_STRING RemainingString     OPTIONAL,
  _In_ DWORD           dwFlags
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncpy' with
  some additional parameters and for PUNICODE_STRINGs. In addition to the
  functionality provided by RtlUnicodeStringCchCopyStringN, this routine also
  returns a PUNICODE_STRING which points to the end of the destination
  string. The flags parameter allows additional controls.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

pszSrc              -   source string

cchToCopy           -   maximum number of characters to copy from source string

RemainingString     -   if RemainingString is non-null, the function will format
the pointer with the remaining buffer and number of
bytes left in the destination string

dwFlags             -   controls some details of the string copy:

STRSAFE_FILL_BEHIND
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer

STRSAFE_IGNORE_NULLS
do not fault if DestinationString is null and treat NULL pszSrc like
empty strings (L""). This flag is useful for emulating
functions like lstrcpy

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer. This will
overwrite any truncated string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_NO_TRUNCATION /
STRSAFE_ZERO_LENGTH_ON_FAILURE
if the function fails, the destination Length will be set
to zero. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
Behavior is undefined if source and destination strings overlap.

pszDest and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified.  If STRSAFE_IGNORE_NULLS is passed, both pszDest and pszSrc
may be NULL.  An error may still be returned even though NULLS are ignored
due to insufficient space.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result.
This is useful for situations where truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCchCopyStringNEx(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ NTSTRSAFE_PCWSTR pszSrc,
        _In_ size_t cchToCopy,
        _Out_opt_ PUNICODE_STRING RemainingString,
        _In_ DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            NULL,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;
        size_t cchNewDestLength = 0;

        status = RtlStringExValidateSrcW(&pszSrc, &cchToCopy, NTSTRSAFE_UNICODE_STRING_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if ((cchToCopy != 0) && (*pszSrc != L'\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                status = RtlWideCharArrayCopyStringWorker(pszDest,
                        cchDest,
                        &cchNewDestLength,
                        pszSrc,
                        cchToCopy);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status)              &&
                        (dwFlags & STRSAFE_FILL_BEHIND) &&
                        (cchRemaining != 0))
                {
                    // handle the STRSAFE_FILL_BEHIND flag
                    RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
                (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                    cchDest,
                    0,
                    &cchNewDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlUnicodeStringCbCopyStringNEx(
  _Inout_ PUNICODE_STRING DestinationString   OPTIONAL,
  _In_ LPCTSTR         pszSrc              OPTIONAL,
  _In_ size_t          cbToCopy,
  _Out_opt_ PUNICODE_STRING RemainingString     OPTIONAL,
  _In_ DWORD           dwFlags
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncpy' with
  some additional parameters and for PUNICODE_STRINGs. In addition to the
  functionality provided by RtlUnicodeStringCbCopyStringN, this routine also
  returns a PUNICODE_STRING which points to the end of the destination
  string. The flags parameter allows additional controls.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

pszSrc              -   source string

cbToCopy            -   maximum number of bytes to copy from source string

RemainingString     -   if RemainingString is non-null, the function will format
the pointer with the remaining buffer and number of
bytes left in the destination string

dwFlags             -   controls some details of the string copy:

STRSAFE_FILL_BEHIND
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer

STRSAFE_IGNORE_NULLS
do not fault if DestinationString is null and treat NULL pszSrc like
empty strings (L""). This flag is useful for emulating
functions like lstrcpy

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer. This will
overwrite any truncated string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_NO_TRUNCATION /
STRSAFE_ZERO_LENGTH_ON_FAILURE
if the function fails, the destination Length will be set
to zero. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.


Notes:
Behavior is undefined if source and destination strings overlap.

DestinationString and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and pszSrc
may be NULL.  An error may still be returned even though NULLS are ignored
due to insufficient space.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result.
This is useful for situations where truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCbCopyStringNEx(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ NTSTRSAFE_PCWSTR pszSrc,
        _In_ size_t cbToCopy,
        _Out_opt_ PUNICODE_STRING RemainingString,
        _In_ DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            NULL,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;
        size_t cchNewDestLength = 0;
        size_t cchToCopy = cbToCopy / sizeof(wchar_t);

        status = RtlStringExValidateSrcW(&pszSrc, &cchToCopy, NTSTRSAFE_UNICODE_STRING_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually src data to copy
                if ((cchToCopy != 0) && (*pszSrc != L'\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                status = RtlWideCharArrayCopyStringWorker(pszDest,
                        cchDest,
                        &cchNewDestLength,
                        pszSrc,
                        cchToCopy);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status)              &&
                        (dwFlags & STRSAFE_FILL_BEHIND) &&
                        (cchRemaining != 0))
                {
                    // handle the STRSAFE_FILL_BEHIND flag
                    RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
                (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                    cchDest,
                    0,
                    &cchNewDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlUnicodeStringCchCopyNEx(
  _Inout_ PUNICODE_STRING     DestinationString   OPTIONAL,
  _In_  PCUNICODE_STRING    SourceString        OPTIONAL,
  _In_  size_t              cchToCopy,
  _Out_opt_ PUNICODE_STRING RemainingString     OPTIONAL,
  _In_  DWORD               dwFlags
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncpy' with
  some additional parameters and for PUNICODE_STRINGs. In addition to the
  functionality provided by RtlUnicodeStringCchCopyN, this
  routine also returns a PUNICODE_STRING which points to the end of the
  destination string. The flags parameter allows additional controls.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

SourceString        -   pointer to the counted unicode source string

cchToCopy           -   maximum number of characters to copy from source string

RemainingString     -   if RemainingString is non-null, the function will format
the pointer with the remaining buffer and number of
bytes left in the destination string

dwFlags             -   controls some details of the string copy:

STRSAFE_FILL_BEHIND
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer

STRSAFE_IGNORE_NULLS
do not fault if DestinationString is null and treat NULL SourceString like
empty strings (L""). This flag is useful for emulating
functions like lstrcpy

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer. This will
overwrite any truncated string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_NO_TRUNCATION /
STRSAFE_ZERO_LENGTH_ON_FAILURE
if the function fails, the destination Length will be set
to zero. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.
Notes:
Behavior is undefined if source and destination strings overlap.

DestinationString and SourceString should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and SourceString
may be NULL.  An error may still be returned even though NULLS are ignored
due to insufficient space.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result.
This is useful for situations where truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCchCopyNEx(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ PCUNICODE_STRING SourceString,
        _In_ size_t cchToCopy,
        _Out_opt_ PUNICODE_STRING RemainingString,
        _In_ DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            NULL,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        wchar_t* pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;
        size_t cchNewDestLength = 0;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                &pszSrc,
                &cchSrcLength,
                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                dwFlags);

        if (NT_SUCCESS(status))
        {
            if (cchToCopy > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                if (cchSrcLength < cchToCopy)
                {
                    cchToCopy = cchSrcLength;
                }

                if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
                {
                    status = STATUS_INVALID_PARAMETER;
                }
                else if (cchDest == 0)
                {
                    // only fail if there was actually src data to copy
                    if (cchToCopy != 0)
                    {
                        if (pszDest == NULL)
                        {
                            status = STATUS_INVALID_PARAMETER;
                        }
                        else
                        {
                            status = STATUS_BUFFER_OVERFLOW;
                        }
                    }
                }
                else
                {
                    status = RtlWideCharArrayCopyWorker(pszDest,
                            cchDest,
                            &cchNewDestLength,
                            pszSrc,
                            cchToCopy);

                    pszDestEnd = pszDest + cchNewDestLength;
                    cchRemaining = cchDest - cchNewDestLength;

                    if (NT_SUCCESS(status)              &&
                            (dwFlags & STRSAFE_FILL_BEHIND) &&
                            (cchRemaining != 0))
                    {
                        // handle the STRSAFE_FILL_BEHIND flag
                        RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                    }
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
                (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                    cchDest,
                    0,
                    &cchNewDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlUnicodeStringCbCopyNEx(
  _Inout_ PUNICODE_STRING     DestinationString   OPTIONAL,
  _In_  PCUNICODE_STRING    SourceString        OPTIONAL,
  _In_  size_t              cbToCopy,
  _Out_opt_ PUNICODE_STRING RemainingString     OPTIONAL,
  _In_  DWORD               dwFlags
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncpy' with
  some additional parameters and for PUNICODE_STRINGs. In addition to the
  functionality provided by RtlUnicodeStringCbCopyN, this
  routine also returns a PUNICODE_STRING which points to the end of the
  destination string. The flags parameter allows additional controls.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

SourceString        -   pointer to the counted unicode source string

cbToCopy            -   maximum number of bytes to copy from source string

RemainingString     -   if RemainingString is non-null, the function will format
the pointer with the remaining buffer and number of
bytes left in the destination string

dwFlags             -   controls some details of the string copy:

STRSAFE_FILL_BEHIND
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer

STRSAFE_IGNORE_NULLS
do not fault if DestinationString is null and treat NULL SourceString like
empty strings (L""). This flag is useful for emulating
functions like lstrcpy

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer. This will
overwrite any truncated string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_NO_TRUNCATION /
STRSAFE_ZERO_LENGTH_ON_FAILURE
if the function fails, the destination Length will be set
to zero. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

Notes:
Behavior is undefined if source and destination strings overlap.

DestinationString and SourceString should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and SourceString
may be NULL.  An error may still be returned even though NULLS are ignored
due to insufficient space.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all copied

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the copy
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result.
This is useful for situations where truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCbCopyNEx(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ PCUNICODE_STRING SourceString,
        _In_ size_t cbToCopy,
        _Out_opt_ PUNICODE_STRING RemainingString,
        _In_ DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            NULL,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        wchar_t* pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;
        size_t cchNewDestLength = 0;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                &pszSrc,
                &cchSrcLength,
                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                dwFlags);

        if (NT_SUCCESS(status))
        {
            size_t cchToCopy = cbToCopy / sizeof(wchar_t);

            if (cchToCopy > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                if (cchSrcLength < cchToCopy)
                {
                    cchToCopy = cchSrcLength;
                }

                if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
                {
                    status = STATUS_INVALID_PARAMETER;
                }
                else if (cchDest == 0)
                {
                    // only fail if there was actually src data to copy
                    if (cchToCopy != 0)
                    {
                        if (pszDest == NULL)
                        {
                            status = STATUS_INVALID_PARAMETER;
                        }
                        else
                        {
                            status = STATUS_BUFFER_OVERFLOW;
                        }
                    }
                }
                else
                {
                    status = RtlWideCharArrayCopyWorker(pszDest,
                            cchDest,
                            &cchNewDestLength,
                            pszSrc,
                            cchToCopy);

                    pszDestEnd = pszDest + cchNewDestLength;
                    cchRemaining = cchDest - cchNewDestLength;

                    if (NT_SUCCESS(status)              &&
                            (dwFlags & STRSAFE_FILL_BEHIND) &&
                            (cchRemaining != 0))
                    {
                        // handle the STRSAFE_FILL_BEHIND flag
                        RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                    }
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
                (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                    cchDest,
                    0,
                    &cchNewDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


/*++

  NTSTATUS
  RtlUnicodeStringCatString(
  _Inout_  PUNICODE_STRING DestinationString,
  _In_     LPCTSTR         pszSrc
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strcat' for
  UNICODE_STRINGs.

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the string was concatenated without truncation, otherwise
  it will return a failure code. In failure cases as much of pszSrc will be
  appended to DestinationString as possible.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

pszSrc              -   source string which must be null terminated

Notes:
Behavior is undefined if source and destination strings overlap.

DestinationString and pszSrc should not be NULL.  See RtlUnicodeStringCatStringEx
if you require the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all concatenated

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result.
This is useful for situations where truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/

NTSTRSAFEDDI
RtlUnicodeStringCatString(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ NTSTRSAFE_PCWSTR pszSrc)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            &cchDestLength,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            0);

    if (NT_SUCCESS(status))
    {
        size_t cchCopied = 0;

        status = RtlWideCharArrayCopyStringWorker(pszDest + cchDestLength,
                cchDest - cchDestLength,
                &cchCopied,
                pszSrc,
                NTSTRSAFE_UNICODE_STRING_MAX_CCH);

        // safe to multiply (cchDestLength + cchCopied) * sizeof(wchar_t) since (cchDestLength + cchCopied) < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
        DestinationString->Length = (USHORT)((cchDestLength + cchCopied) * sizeof(wchar_t));
    }

    return status;
}


/*++

  NTSTATUS
  RtlUnicodeStringCat(
  _Inout_  PUNICODE_STRING     DestinationString,
  _In_     PCUNICODE_STRING    SourceString
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strcat' for
  UNICODE_STRINGs.

  This function returns an NTSTATUS value, and not a pointer.  It returns
  STATUS_SUCCESS if the string was concatenated without truncation, otherwise
  it will return a failure code. In failure cases as much of SourceString will be
  appended to DestinationString as possible.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

SourceString        -   pointer to the counted unicode source string

Notes:
Behavior is undefined if source and destination strings overlap.

DestinationString and pszSrc should not be NULL.  See RtlUnicodeStringCatEx
if you require the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all concatenated

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result.
This is useful for situations where truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function.

--*/

NTSTRSAFEDDI
RtlUnicodeStringCat(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ PCUNICODE_STRING SourceString)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            &cchDestLength,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            0);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                &pszSrc,
                &cchSrcLength,
                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                0);

        if (NT_SUCCESS(status))
        {
            size_t cchCopied = 0;

            status = RtlWideCharArrayCopyWorker(pszDest + cchDestLength,
                    cchDest - cchDestLength,
                    &cchCopied,
                    pszSrc,
                    cchSrcLength);

            // safe to multiply (cchDestLength + cchCopied) * sizeof(wchar_t) since (cchDestLength + cchCopied) < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)((cchDestLength + cchCopied) * sizeof(wchar_t));
        }
    }

    return status;
}


/*++

  NTSTATUS
  RtlUnicodeStringCatStringEx(
  _Inout_ PUNICODE_STRING DestinationString   OPTTONAL,
  _In_      LPCTSTR         pszSrc              OPTIONAL,
  _Out_opt_ PUNICODE_STRING RemainingString     OPTIONAL,
  _In_      DWORD           dwFlags
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strcat' for
  PUNICODE_STRINGs with some additional parameters.  In addition to the
  functionality provided by RtlUnicodeStringCatString, this routine
  also returns a PUNICODE_STRING which points to the end of the destination
  string. The flags parameter allows additional controls.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

pszSrc              -   source string which must be null terminated

RemainingString     -   if RemainingString is non-null, the function will format
the pointer with the remaining buffer and number of
bytes left in the destination string

dwFlags             -   controls some details of the string copy:

STRSAFE_FILL_BEHIND
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer

STRSAFE_IGNORE_NULLS
do not fault if DestinationString is null and treat NULL pszSrc like
empty strings (L""). This flag is useful for emulating
functions like lstrcpy

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer. This will
overwrite any truncated string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_ZERO_LENGTH_ON_FAILURE
if the function fails, the destination Length will be set
to zero. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

STRSAFE_NO_TRUNCATION
if the function returns STATUS_BUFFER_OVERFLOW, pszDest
will not contain a truncated string, it will remain unchanged.

Notes:
Behavior is undefined if source and destination strings overlap or if
DestinationString and RemainingString are the same pointer.

DestinationString and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and pszSrc
may be NULL.  An error may still be returned even though NULLS are ignored
due to insufficient space.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all concatenated

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result.
This is useful for situations where truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCatStringEx(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ NTSTRSAFE_PCWSTR pszSrc,
        _Out_opt_ PUNICODE_STRING RemainingString,
        _In_ DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            &cchDestLength,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;
        size_t cchNewDestLength = cchDestLength;

        status = RtlStringExValidateSrcW(&pszSrc, NULL, NTSTRSAFE_UNICODE_STRING_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining == 0)
            {
                // only fail if there was actually src data to append
                if (*pszSrc != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlWideCharArrayCopyStringWorker(pszDestEnd,
                        cchRemaining,
                        &cchCopied,
                        pszSrc,
                        NTSTRSAFE_UNICODE_STRING_MAX_CCH);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;

                cchNewDestLength = cchDestLength + cchCopied;

                if (NT_SUCCESS(status)              &&
                        (dwFlags & STRSAFE_FILL_BEHIND) &&
                        (cchRemaining != 0))
                {
                    // handle the STRSAFE_FILL_BEHIND flag
                    RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
                (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                    cchDest,
                    cchDestLength,
                    &cchNewDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}


/*++

  NTSTATUS
  RtlUnicodeStringCatEx(
  _Inout_   PUNICODE_STRING     DestinationString   OPTIONAL,
  _In_      PCUNICODE_STRING    SourceString        OPTIONAL,
  _Out_opt_ PUNICODE_STRING     RemainingString     OPTIONAL,
  _In_      DWORD               dwFlags
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strcat' for
  PUNICODE_STRINGs with some additional parameters. In addition to the
  functionality provided by RtlUnicodeStringCat, this routine
  also returns a PUNICODE_STRING which points to the end of the destination
  string. The flags parameter allows additional controls.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

SourceString        -   pointer to the counted unicode source string

RemainingString     -   if RemainingString is non-null, the function will format
the pointer with the remaining buffer and number of
bytes left in the destination string

dwFlags             -   controls some details of the string copy:

STRSAFE_FILL_BEHIND
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer

STRSAFE_IGNORE_NULLS
do not fault if DestinationString is null and treat NULL pszSrc like
empty strings (L""). This flag is useful for emulating
functions like lstrcpy

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer. This will
overwrite any truncated string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_ZERO_LENGTH_ON_FAILURE
if the function fails, the destination Length will be set
to zero. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

STRSAFE_NO_TRUNCATION
if the function returns STATUS_BUFFER_OVERFLOW, pszDest
will not contain a truncated string, it will remain unchanged.

Notes:
Behavior is undefined if source and destination strings overlap or if
DestinationString and RemainingString are the same pointer.

DestinationString and SourceString should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and SourceString
may be NULL.  An error may still be returned even though NULLS are ignored
due to insufficient space.

Return Value:

STATUS_SUCCESS -   if there was source data and it was all concatenated

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result.
This is useful for situations where truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCatEx(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ PCUNICODE_STRING SourceString,
        _Out_opt_ PUNICODE_STRING RemainingString,
        _In_ DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            &cchDestLength,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        wchar_t* pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;
        size_t cchNewDestLength = cchDestLength;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                &pszSrc,
                &cchSrcLength,
                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining == 0)
            {
                // only fail if there was actually src data to append
                if (cchSrcLength != 0)
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlWideCharArrayCopyWorker(pszDestEnd,
                        cchRemaining,
                        &cchCopied,
                        pszSrc,
                        cchSrcLength);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;

                cchNewDestLength = cchDestLength + cchCopied;

                if (NT_SUCCESS(status)              &&
                        (dwFlags & STRSAFE_FILL_BEHIND) &&
                        (cchRemaining != 0))
                {
                    // handle the STRSAFE_FILL_BEHIND flag
                    RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
                (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                    cchDest,
                    cchDestLength,
                    &cchNewDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlUnicodeStringCchCatStringN(
  _Inout_  PUNICODE_STRING DestinationString,
  _In_     LPCTSTR         pszSrc,
  _In_     size_t          cchToAppend
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncat' for
  PUNICODE_STRINGs.

  This function returns an NTSTATUS value, and not a pointer. It returns
  STATUS_SUCCESS if all of pszSrc or the first cchToAppend characters were
  appended to the destination string, otherwise it will return a failure
  code. In failure cases as much of pszSrc will be appended to DestinationString as
  possible.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

pszSrc              -   source string

cchToAppend         -   maximum number of characters to append

Notes:
Behavior is undefined if source and destination strings overlap.

DestinationString and pszSrc should not be NULL. See RtlUnicodeStringCchCatStringNEx if
you require the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if all of pszSrc or the first cchToAppend characters were
concatenated to DestinationString

failure        -   the operation did not succeed


STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result. This is
useful for situations where truncation is ok

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCchCatStringN(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ NTSTRSAFE_PCWSTR pszSrc,
        _In_ size_t cchToAppend)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            &cchDestLength,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            0);

    if (NT_SUCCESS(status))
    {
        if (cchToAppend > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            size_t cchCopied = 0;

            status = RtlWideCharArrayCopyStringWorker(pszDest + cchDestLength,
                    cchDest - cchDestLength,
                    &cchCopied,
                    pszSrc,
                    cchToAppend);

            // safe to multiply (cchDestLength + cchCopied) * sizeof(wchar_t) since (cchDestLength + cchCopied) < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)((cchDestLength + cchCopied) * sizeof(wchar_t));
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlUnicodeStringCbCatStringN(
  _Inout_   PUNICODE_STRING DestinationString,
  _In_      LPCTSTR         pszSrc,
  _In_      size_t          cbToAppend
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncat' for
  PUNICODE_STRINGs.

  This function returns an NTSTATUS value, and not a pointer. It returns
  STATUS_SUCCESS if all of pszSrc or the first cbToAppend bytes were
  appended to the destination string, otherwise it will return a failure
  code. In failure cases as much of pszSrc will be appended to DestinationString as
  possible.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

pszSrc              -   source string

cbToAppend          -   maximum number of bytes to append

Notes:
Behavior is undefined if source and destination strings overlap.

DestinationString and pszSrc should not be NULL. See RtlUnicodeStringCbCatStringNEx if
you require the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if all of pszSrc or the first cbToAppend bytes were
concatenated to pszDest

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result. This is
useful for situations where truncation is ok

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCbCatStringN(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ NTSTRSAFE_PCWSTR pszSrc,
        _In_ size_t cbToAppend)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            &cchDestLength,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            0);

    if (NT_SUCCESS(status))
    {
        size_t cchToAppend = cbToAppend / sizeof(wchar_t);

        if (cchToAppend > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            size_t cchCopied = 0;

            status = RtlWideCharArrayCopyStringWorker(pszDest + cchDestLength,
                    cchDest - cchDestLength,
                    &cchCopied,
                    pszSrc,
                    cchToAppend);

            // safe to multiply (cchDestLength + cchCopied) * sizeof(wchar_t) since (cchDestLength + cchCopied) < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)((cchDestLength + cchCopied) * sizeof(wchar_t));
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlUnicodeStringCchCatN(
  _Inout_  PUNICODE_STRING     DestinationString,
  _In_     PCUNICODE_STRING    SourceString,
  _In_     size_t              cchToAppend
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncat' for
  PUNICODE_STRINGs.

  This function returns an NTSTATUS value, and not a pointer. It returns
  STATUS_SUCCESS if all of SourceString or the first cchToAppend characters were
  appended to the destination string, otherwise it will return a failure
  code. In failure cases as much of SourceString will be appended to DestinationString as
  possible.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

SourceString        -   pointer to the counted unicode source string

cchToAppend         -   maximum number of characters to append

Notes:
Behavior is undefined if source and destination strings overlap.

DestinationString and SourceString should not be NULL. See RtlUnicodeStringCchCatNEx if
you require the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if all of SourceString or the first cchToAppend characters were
concatenated to DestinationString

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result. This is
useful for situations where truncation is ok

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCchCatN(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ PCUNICODE_STRING SourceString,
        _In_ size_t cchToAppend)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            &cchDestLength,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            0);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                &pszSrc,
                &cchSrcLength,
                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                0);

        if (NT_SUCCESS(status))
        {
            if (cchToAppend > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                size_t cchCopied = 0;

                if (cchSrcLength < cchToAppend)
                {
                    cchToAppend = cchSrcLength;
                }

                status = RtlWideCharArrayCopyWorker(pszDest + cchDestLength,
                        cchDest - cchDestLength,
                        &cchCopied,
                        pszSrc,
                        cchToAppend);

                // safe to multiply (cchDestLength + cchCopied) * sizeof(wchar_t) since (cchDestLength + cchCopied) < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                DestinationString->Length = (USHORT)((cchDestLength + cchCopied) * sizeof(wchar_t));
            }
        }
    }

    return status;
}
#endif // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlUnicodeStringCbCatN(
  _Inout_ PUNICODE_STRING     DestinationString,
  _In_    PCUNICODE_STRING    SourceString,
  _In_    size_t              cbToAppend
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncat' for
  PUNICODE_STRINGs.

  This function returns an NTSTATUS value, and not a pointer. It returns
  STATUS_SUCCESS if all of SourceString or the first cbToAppend bytes were
  appended to the destination string, otherwise it will return a failure
  code. In failure cases as much of SourceString will be appended to DestinationString as
  possible.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

SourceString        -   pointer to the counted unicode source string

cbToAppend          -   maximum number of bytes to append

Notes:
Behavior is undefined if source and destination strings overlap.

DestinationString and SourceString should not be NULL. See RtlUnicodeStringCbCatNEx if
you require the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if all of SourceString or the first cbToAppend bytes were
concatenated to pszDest

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result. This is
useful for situations where truncation is ok

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCbCatN(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ PCUNICODE_STRING SourceString,
        _In_ size_t cbToAppend)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            &cchDestLength,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            0);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                &pszSrc,
                &cchSrcLength,
                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                0);

        if (NT_SUCCESS(status))
        {
            size_t cchToAppend = cbToAppend / sizeof(wchar_t);

            if (cchToAppend > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                size_t cchCopied = 0;

                if (cchSrcLength < cchToAppend)
                {
                    cchToAppend = cchSrcLength;
                }

                status = RtlWideCharArrayCopyWorker(pszDest + cchDestLength,
                        cchDest - cchDestLength,
                        &cchCopied,
                        pszSrc,
                        cchToAppend);

                // safe to multiply (cchDestLength + cchCopied) * sizeof(wchar_t) since (cchDestLength + cchCopied) < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                DestinationString->Length = (USHORT)((cchDestLength + cchCopied) * sizeof(wchar_t));
            }
        }
    }

    return status;
}
#endif // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlUnicodeStringCchCatStringNEx(
  _Inout_ PUNICODE_STRING DestinationString   OPTIONAL,
  _In_    LPCTSTR         pszSrc              OPTIONAL,
  _In_    size_t          cchToAppend,
  _Out_opt_ PUNICODE_STRING RemainingString     OPTIONAL,
  _In_    DWORD           dwFlags
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncat', with
  some additional parameters and for PUNICODE_STRINGs. In addition to the
  functionality provided by RtlUnicodeStringCchCatStringN, this routine
  also returns a PUNICODE_STRING which points to the end of the destination
  string. The flags parameter allows additional controls.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

pszSrc              -   source string

cchToAppend         -   maximum number of characters to append

RemainingString     -   if RemainingString is non-null, the function will format
the pointer with the remaining buffer and number of
bytes left in the destination string

dwFlags             -   controls some details of the string copy:

STRSAFE_FILL_BEHIND
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer

STRSAFE_IGNORE_NULLS
do not fault if DestinationString is null and treat NULL pszSrc like
empty strings (L""). This flag is useful for emulating
functions like lstrcpy

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer. This will
overwrite any truncated string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_ZERO_LENGTH_ON_FAILURE
if the function fails, the destination Length will be set
to zero. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

STRSAFE_NO_TRUNCATION
if the function returns STATUS_BUFFER_OVERFLOW, pszDest
will not contain a truncated string, it will remain unchanged.

Notes:
Behavior is undefined if source and destination strings overlap.

DestinationString and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and pszSrc
may be NULL.  An error may still be returned even though NULLS are ignored
due to insufficient space.

Return Value:

STATUS_SUCCESS -   if all of pszSrc or the first cchToAppend characters were
concatenated to DestinationString

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result.
This is useful for situations where truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCchCatStringNEx(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ NTSTRSAFE_PCWSTR pszSrc,
        _In_ size_t cchToAppend,
        _Out_opt_ PUNICODE_STRING RemainingString,
        _In_ DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            &cchDestLength,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;
        size_t cchNewDestLength = cchDestLength;

        status = RtlStringExValidateSrcW(&pszSrc, &cchToAppend, NTSTRSAFE_UNICODE_STRING_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining == 0)
            {
                // only fail if there was actually src data to append
                if ((cchToAppend != 0) && (*pszSrc != L'\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlWideCharArrayCopyStringWorker(pszDestEnd,
                        cchRemaining,
                        &cchCopied,
                        pszSrc,
                        cchToAppend);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;

                cchNewDestLength = cchDestLength + cchCopied;

                if (NT_SUCCESS(status)              &&
                        (dwFlags & STRSAFE_FILL_BEHIND) &&
                        (cchRemaining != 0))
                {
                    // handle the STRSAFE_FILL_BEHIND flag
                    RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
                (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                    cchDest,
                    cchDestLength,
                    &cchNewDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlUnicodeStringCbCatStringNEx(
  _Inout_                PUNICODE_STRING DestinationString   OPTIONAL,
  _In_   LPCTSTR         pszSrc              OPTIONAL,
  _In_                   size_t          cbToAppend,
  _Out_opt_              PUNICODE_STRING RemainingString     OPTIONAL,
  _In_                   DWORD           dwFlags
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncat', with
  some additional parameters and for PUNICODE_STRINGs. In addition to the
  functionality provided by RtlUnicodeStringCbCatStringN, this routine
  also returns a PUNICODE_STRING which points to the end of the destination
  string. The flags parameter allows additional controls.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

pszSrc              -   source string

cbToAppend          -   maximum number of bytes to append

RemainingString     -   if RemainingString is non-null, the function will format
the pointer with the remaining buffer and number of
bytes left in the destination string

dwFlags             -   controls some details of the string copy:

STRSAFE_FILL_BEHIND
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer

STRSAFE_IGNORE_NULLS
do not fault if DestinationString is null and treat NULL pszSrc like
empty strings (L""). This flag is useful for emulating
functions like lstrcpy

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer. This will
overwrite any truncated string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_ZERO_LENGTH_ON_FAILURE
if the function fails, the destination Length will be set
to zero. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

STRSAFE_NO_TRUNCATION
if the function returns STATUS_BUFFER_OVERFLOW, pszDest
will not contain a truncated string, it will remain unchanged.

Notes:
Behavior is undefined if source and destination strings overlap.

DestinationString and pszSrc should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and pszSrc
may be NULL.  An error may still be returned even though NULLS are ignored
due to insufficient space.

Return Value:

STATUS_SUCCESS -   if all of pszSrc or the first cbToAppend bytes were
concatenated to pszDest

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result.
This is useful for situations where truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCbCatStringNEx(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ NTSTRSAFE_PCWSTR pszSrc,
        _In_ size_t cbToAppend,
        _Out_opt_ PUNICODE_STRING RemainingString,
        _In_ DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            &cchDestLength,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;
        size_t cchNewDestLength = cchDestLength;
        size_t cchToAppend = cbToAppend / sizeof(wchar_t);

        status = RtlStringExValidateSrcW(&pszSrc, &cchToAppend, NTSTRSAFE_UNICODE_STRING_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchRemaining == 0)
            {
                // only fail if there was actually src data to append
                if ((cchToAppend != 0) && (*pszSrc != L'\0'))
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                size_t cchCopied = 0;

                status = RtlWideCharArrayCopyStringWorker(pszDestEnd,
                        cchRemaining,
                        &cchCopied,
                        pszSrc,
                        cchToAppend);

                pszDestEnd = pszDestEnd + cchCopied;
                cchRemaining = cchRemaining - cchCopied;

                cchNewDestLength = cchDestLength + cchCopied;

                if (NT_SUCCESS(status)              &&
                        (dwFlags & STRSAFE_FILL_BEHIND) &&
                        (cchRemaining != 0))
                {
                    // handle the STRSAFE_FILL_BEHIND flag
                    RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
                (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                    cchDest,
                    cchDestLength,
                    &cchNewDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


#ifndef NTSTRSAFE_NO_CCH_FUNCTIONS
/*++

  NTSTATUS
  RtlUnicodeStringCchCatNEx(
  _Inout_   PUNICODE_STRING     DestinationString   OPTIONAL,
  _In_      PCUNICODE_STRING    SourceString        OPTIONAL,
  _In_      size_t              cchToAppend,
  _Out_opt_ PUNICODE_STRING     RemainingString     OPTIONAL,
  _In_      DWORD               dwFlags
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncat', with
  some additional parameters and for PUNICODE_STRINGs. In addition to the
  functionality provided by RtlUnicodeStringCchCatN, this routine
  also returns a PUNICODE_STRING which points to the end of the destination
  string. The flags parameter allows additional controls.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

SourceString        -   pointer to the counted unicode source string

cchToAppend         -   maximum number of characters to append

RemainingString     -   if RemainingString is non-null, the function will format
the pointer with the remaining buffer and number of
bytes left in the destination string

dwFlags             -   controls some details of the string copy:

STRSAFE_FILL_BEHIND
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer

STRSAFE_IGNORE_NULLS
do not fault if DestinationString is null and treat NULL SourceString like
empty strings (L""). This flag is useful for emulating
functions like lstrcpy

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer. This will
overwrite any truncated string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_ZERO_LENGTH_ON_FAILURE
if the function fails, the destination Length will be set
to zero. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

STRSAFE_NO_TRUNCATION
if the function returns STATUS_BUFFER_OVERFLOW, pszDest
will not contain a truncated string, it will remain unchanged.

Notes:
Behavior is undefined if source and destination strings overlap.

DestinationString and SourceString should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and SourceString
may be NULL.  An error may still be returned even though NULLS are ignored
due to insufficient space.

Return Value:

STATUS_SUCCESS -   if all of SourceString or the first cchToAppend characters were
concatenated to DestinationString

failure        -   the operation did not succeed


STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result.
This is useful for situations where truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/

#pragma warning(push)
#pragma warning(disable : __WARNING_PRECONDITION_NULLTERMINATION_VIOLATION)

NTSTRSAFEDDI
RtlUnicodeStringCchCatNEx(
        _Inout_   PUNICODE_STRING  DestinationString,
        _In_      PCUNICODE_STRING SourceString,
        _In_      size_t           cchToAppend,
        _Out_opt_ PUNICODE_STRING  RemainingString,
        _In_      DWORD            dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            &cchDestLength,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        wchar_t* pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;
        size_t cchNewDestLength = cchDestLength;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                &pszSrc,
                &cchSrcLength,
                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                dwFlags);

        if (NT_SUCCESS(status))
        {
            if (cchToAppend > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                if (cchSrcLength < cchToAppend)
                {
                    cchToAppend = cchSrcLength;
                }

                if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
                {
                    status = STATUS_INVALID_PARAMETER;
                }
                else if (cchRemaining == 0)
                {
                    // only fail if there was actually src data to append
                    if (cchToAppend != 0)
                    {
                        if (pszDest == NULL)
                        {
                            status = STATUS_INVALID_PARAMETER;
                        }
                        else
                        {
                            status = STATUS_BUFFER_OVERFLOW;
                        }
                    }
                }
                else
                {
                    size_t cchCopied = 0;

                    status = RtlWideCharArrayCopyStringWorker(pszDestEnd,
                            cchRemaining,
                            &cchCopied,
                            pszSrc,
                            cchToAppend);

                    pszDestEnd = pszDestEnd + cchCopied;
                    cchRemaining = cchRemaining - cchCopied;

                    cchNewDestLength = cchDestLength + cchCopied;

                    if (NT_SUCCESS(status)              &&
                            (dwFlags & STRSAFE_FILL_BEHIND) &&
                            (cchRemaining != 0))
                    {
                        // handle the STRSAFE_FILL_BEHIND flag
                        RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                    }
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
                (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                    cchDest,
                    cchDestLength,
                    &cchNewDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}

#pragma warning(pop)

#endif  // !NTSTRSAFE_NO_CCH_FUNCTIONS


#ifndef NTSTRSAFE_NO_CB_FUNCTIONS
/*++

  NTSTATUS
  RtlUnicodeStringCbCatNEx(
  _Inout_   PUNICODE_STRING     DestinationString   OPTIONAL,
  _In_      PCUNICODE_STRING    SourceString        OPTIONAL,
  _In_      size_t              cbToAppend,
  _Out_opt_ PUNICODE_STRING     RemainingString     OPTIONAL,
  _In_      DWORD               dwFlags
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'strncat', with
  some additional parameters and for PUNICODE_STRINGs. In addition to the
  functionality provided by RtlUnicodeStringCbCatN, this routine
  also returns a PUNICODE_STRING which points to the end of the destination
  string. The flags parameter allows additional controls.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

SourceString        -   pointer to the counted unicode source string

cbToAppend          -   maximum number of bytes to append

RemainingString     -   if RemainingString is non-null, the function will format
the pointer with the remaining buffer and number of
bytes left in the destination string

dwFlags             -   controls some details of the string copy:

STRSAFE_FILL_BEHIND
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer

STRSAFE_IGNORE_NULLS
do not fault if DestinationString is null and treat NULL SourceString like
empty strings (L""). This flag is useful for emulating
functions like lstrcpy

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer. This will
overwrite any truncated string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_ZERO_LENGTH_ON_FAILURE
if the function fails, the destination Length will be set
to zero. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

STRSAFE_NO_TRUNCATION
if the function returns STATUS_BUFFER_OVERFLOW, pszDest
will not contain a truncated string, it will remain unchanged.

Notes:
Behavior is undefined if source and destination strings overlap.

DestinationString and SourceString should not be NULL unless the STRSAFE_IGNORE_NULLS flag
is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and SourceString
may be NULL.  An error may still be returned even though NULLS are ignored
due to insufficient space.

Return Value:

STATUS_SUCCESS -   if all of SourceString or the first cbToAppend bytes were
concatenated to DestinationString

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result.
This is useful for situations where truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringCbCatNEx(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ PCUNICODE_STRING SourceString,
        _In_ size_t cbToAppend,
        _Out_opt_ PUNICODE_STRING RemainingString,
        _In_ DWORD dwFlags)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;
    size_t cchDestLength;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            &cchDestLength,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszSrc;
        size_t cchSrcLength;
        wchar_t* pszDestEnd = pszDest + cchDestLength;
        size_t cchRemaining = cchDest - cchDestLength;
        size_t cchNewDestLength = cchDestLength;

        status = RtlUnicodeStringValidateSrcWorker(SourceString,
                &pszSrc,
                &cchSrcLength,
                NTSTRSAFE_UNICODE_STRING_MAX_CCH,
                dwFlags);

        if (NT_SUCCESS(status))
        {
            size_t cchToAppend = cbToAppend / sizeof(wchar_t);

            if (cchToAppend > NTSTRSAFE_UNICODE_STRING_MAX_CCH)
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                if (cchSrcLength < cchToAppend)
                {
                    cchToAppend = cchSrcLength;
                }

                if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
                {
                    status = STATUS_INVALID_PARAMETER;
                }
                else if (cchRemaining == 0)
                {
                    // only fail if there was actually src data to append
                    if (cchToAppend != 0)
                    {
                        if (pszDest == NULL)
                        {
                            status = STATUS_INVALID_PARAMETER;
                        }
                        else
                        {
                            status = STATUS_BUFFER_OVERFLOW;
                        }
                    }
                }
                else
                {
                    size_t cchCopied = 0;

                    status = RtlWideCharArrayCopyWorker(pszDestEnd,
                            cchRemaining,
                            &cchCopied,
                            pszSrc,
                            cchToAppend);

                    pszDestEnd = pszDestEnd + cchCopied;
                    cchRemaining = cchRemaining - cchCopied;

                    cchNewDestLength = cchDestLength + cchCopied;

                    if (NT_SUCCESS(status)              &&
                            (dwFlags & STRSAFE_FILL_BEHIND) &&
                            (cchRemaining != 0))
                    {
                        // handle the STRSAFE_FILL_BEHIND flag
                        RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                    }
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
                (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                    cchDest,
                    cchDestLength,
                    &cchNewDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}
#endif  // !NTSTRSAFE_NO_CB_FUNCTIONS


/*++

  NTSTATUS
  RtlUnicodeStringVPrintf(
  _Inout_                      PUNICODE_STRING DestinationString,
  _In_ _Printf_format_string_  PCWSTR          pszFormat,
  _In_                         va_list         argList
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'vsprintf' for
  PUNICODE_STRINGs.

  This function returns an NTSTATUS value, and not a pointer. It returns
  STATUS_SUCCESS if the string was printed without truncation, otherwise it
  will return a failure code. In failure cases it will return a truncated
  version of the ideal result.

Arguments:

DestinationString   -  pointer to the counted unicode destination string

pszFormat           -  format string which must be null terminated

argList             -  va_list from the variable arguments according to the
stdarg.h convention

Notes:
Behavior is undefined if destination, format strings or any arguments
strings overlap.

DestinationString and pszFormat should not be NULL. See RtlUnicodeStringVPrintfEx if you
require the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
the resultant string

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the print
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result. This is
useful for situations where truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringVPrintf(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
        _In_ va_list argList)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            NULL,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            0);

    if (NT_SUCCESS(status))
    {
        size_t cchNewDestLength = 0;

        status = RtlWideCharArrayVPrintfWorker(pszDest,
                cchDest,
                &cchNewDestLength,
                pszFormat,
                argList);

        // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
        DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
    }

    return status;
}


/*++

  NTSTATUS
  RtlUnicodeStringVPrintfEx(
  _Inout_              PUNICODE_STRING DestinationString   OPTIONAL,
  _Out_opt_            PUNICODE_STRING RemainingString     OPTIONAL,
  _In_                 DWORD   dwFlags,
  _In_ _Printf_format_string_ PCWSTR  pszFormat                   OPTIONAL,
  _In_                 va_list argList
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'vsprintf' with
  some additional parameters for PUNICODE_STRING. In addition to the
  functionality provided by RtlUnicodeStringVPrintf, this routine also
  returns a PUNICODE_STRING which points to the end of the destination
  string. The flags parameter allows additional controls.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

RemainingString     -   if RemainingString is non-null, the function will format
the pointer with the remaining buffer and number of
bytes left in the destination string

dwFlags             -   controls some details of the string copy:

STRSAFE_FILL_BEHIND
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer

STRSAFE_IGNORE_NULLS
do not fault if DestinationString is null and treat NULL pszFormat like
empty strings (L"").

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer. This will
overwrite any truncated string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_NO_TRUNCATION /
STRSAFE_ZERO_LENGTH_ON_FAILURE
if the function fails, the destination Length will be set
to zero. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

pszFormat           -   format string which must be null terminated

argList             -   va_list from the variable arguments according to the
stdarg.h convention

Notes:
Behavior is undefined if destination, format strings or any arguments
strings overlap.

DestinationString and pszFormat should not be NULL unless the STRSAFE_IGNORE_NULLS
flag is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and
pszFormat may be NULL.  An error may still be returned even though NULLS
are ignored due to insufficient space.

Return Value:

STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
the resultant string

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the print
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result. This is
useful for situations where truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringVPrintfEx(
        _Inout_ PUNICODE_STRING DestinationString,
        _Out_opt_ PUNICODE_STRING RemainingString,
        _In_ DWORD dwFlags,
        _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
        _In_ va_list argList)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            NULL,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;
        size_t cchNewDestLength = 0;

        status = RtlStringExValidateSrcW(&pszFormat, NULL, NTSTRSAFE_UNICODE_STRING_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually a non-empty format string
                if (*pszFormat != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                status = RtlWideCharArrayVPrintfWorker(pszDest,
                        cchDest,
                        &cchNewDestLength,
                        pszFormat,
                        argList);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status)              &&
                        (dwFlags & STRSAFE_FILL_BEHIND) &&
                        (cchRemaining != 0))
                {
                    // handle the STRSAFE_FILL_BEHIND flag
                    RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
                (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                    cchDest,
                    0,
                    &cchNewDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}


#ifndef _M_CEE_PURE

/*++

  NTSTATUS
  RtlUnicodeStringPrintf(
  _Inout_                     PUNICODE_STRING DestinationString,
  _In_ _Printf_format_string_ PCWSTR          pszFormat,
  ...
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'sprintf' for
  PUNICODE_STRINGs.

  This function returns an NTSTATUS value, and not a pointer. It returns
  STATUS_SUCCESS if the string was printed without truncation, otherwise it
  will return a failure code. In failure cases it will return a truncated
  version of the ideal result.

Arguments:

DestinationString   -  pointer to the counted unicode destination string

pszFormat           -  format string which must be null terminated

...                 -  additional parameters to be formatted according to
the format string

Notes:
Behavior is undefined if destination, format strings or any arguments
strings overlap.

DestinationString and pszFormat should not be NULL.  See RtlUnicodeStringPrintfEx if you
require the handling of NULL values.

Return Value:

STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
the resultant string

failure        -   the operation did not succeed

STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the print
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result. This is
useful for situations where truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringPrintf(
        _Inout_ PUNICODE_STRING DestinationString,
        _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
        ...)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            NULL,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            0);

    if (NT_SUCCESS(status))
    {
        va_list argList;
        size_t cchNewDestLength = 0;

        va_start(argList, pszFormat);

        status = RtlWideCharArrayVPrintfWorker(pszDest,
                cchDest,
                &cchNewDestLength,
                pszFormat,
                argList);

        va_end(argList);

        // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
        DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
    }

    return status;
}


/*++

  NTSTATUS
  RtlUnicodeStringPrintfEx(
  _Inout_              PUNICODE_STRING DestinationString   OPTIONAL,
  _Out_opt_            PUNICODE_STRING RemainingString     OPTIONAL,
  _In_                 DWORD           dwFlags,
  _In_ _Printf_format_string_ PCWSTR          pszFormat           OPTIONAL,
  ...
  );

  Routine Description:

  This routine is a safer version of the C built-in function 'sprintf' with
  some additional parameters for PUNICODE_STRINGs. In addition to the
  functionality provided by RtlUnicodeStringPrintf, this routine also
  returns a PUNICODE_STRING which points to the end of the destination
  string. The flags parameter allows additional controls.

Arguments:

DestinationString   -   pointer to the counted unicode destination string

RemainingString     -   if RemainingString is non-null, the function will format
the pointer with the remaining buffer and number of
bytes left in the destination string

dwFlags             -   controls some details of the string copy:

STRSAFE_FILL_BEHIND
if the function succeeds, the low byte of dwFlags will be
used to fill the uninitialize part of destination buffer

STRSAFE_IGNORE_NULLS
do not fault if DestinationString is null and treat NULL pszFormat like
empty strings (L"").

STRSAFE_FILL_ON_FAILURE
if the function fails, the low byte of dwFlags will be
used to fill all of the destination buffer. This will
overwrite any truncated string returned when the failure is
STATUS_BUFFER_OVERFLOW

STRSAFE_NO_TRUNCATION /
STRSAFE_ZERO_LENGTH_ON_FAILURE
if the function fails, the destination Length will be set
to zero. This will overwrite any truncated string
returned when the failure is STATUS_BUFFER_OVERFLOW.

pszFormat           -   format string which must be null terminated

...                 -   additional parameters to be formatted according to
the format string

Notes:
Behavior is undefined if destination, format strings or any arguments
strings overlap.

DestinationString and pszFormat should not be NULL unless the STRSAFE_IGNORE_NULLS
flag is specified.  If STRSAFE_IGNORE_NULLS is passed, both DestinationString and
pszFormat may be NULL.  An error may still be returned even though NULLS
are ignored due to insufficient space.

Return Value:

STATUS_SUCCESS -   if there was sufficient space in the dest buffer for
the resultant string

failure        -   the operation did not succeed


STATUS_BUFFER_OVERFLOW
Note: This status has the severity class Warning - IRPs completed with this
status do have their data copied back to user mode
-   this return value is an indication that the print
operation failed due to insufficient space. When this
error occurs, the destination buffer is modified to
contain a truncated version of the ideal result. This is
useful for situations where truncation is ok.

It is strongly recommended to use the NT_SUCCESS() macro to test the
return value of this function

--*/

NTSTRSAFEDDI
RtlUnicodeStringPrintfEx(
        _Inout_ PUNICODE_STRING DestinationString,
        _Out_opt_ PUNICODE_STRING RemainingString,
        _In_ DWORD dwFlags,
        _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
        ...)
{
    NTSTATUS status;
    wchar_t* pszDest;
    size_t cchDest;

    status = RtlUnicodeStringValidateDestWorker(DestinationString,
            &pszDest,
            &cchDest,
            NULL,
            NTSTRSAFE_UNICODE_STRING_MAX_CCH,
            dwFlags);

    if (NT_SUCCESS(status))
    {
        wchar_t* pszDestEnd = pszDest;
        size_t cchRemaining = cchDest;
        size_t cchNewDestLength = 0;

        status = RtlStringExValidateSrcW(&pszFormat, NULL, NTSTRSAFE_UNICODE_STRING_MAX_CCH, dwFlags);

        if (NT_SUCCESS(status))
        {
            if (dwFlags & (~STRSAFE_UNICODE_STRING_VALID_FLAGS))
            {
                status = STATUS_INVALID_PARAMETER;
            }
            else if (cchDest == 0)
            {
                // only fail if there was actually a non-empty format string
                if (*pszFormat != L'\0')
                {
                    if (pszDest == NULL)
                    {
                        status = STATUS_INVALID_PARAMETER;
                    }
                    else
                    {
                        status = STATUS_BUFFER_OVERFLOW;
                    }
                }
            }
            else
            {
                va_list argList;

                va_start(argList, pszFormat);

                status = RtlWideCharArrayVPrintfWorker(pszDest,
                        cchDest,
                        &cchNewDestLength,
                        pszFormat,
                        argList);

                va_end(argList);

                pszDestEnd = pszDest + cchNewDestLength;
                cchRemaining = cchDest - cchNewDestLength;

                if (NT_SUCCESS(status)              &&
                        (dwFlags & STRSAFE_FILL_BEHIND) &&
                        (cchRemaining != 0))
                {
                    // handle the STRSAFE_FILL_BEHIND flag
                    RtlUnicodeStringExHandleFill(pszDestEnd, cchRemaining, dwFlags);
                }
            }
        }

        if (!NT_SUCCESS(status)                                                                                      &&
                (dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_ZERO_LENGTH_ON_FAILURE))  &&
                (cchDest != 0))
        {
            // handle the STRSAFE_NO_TRUNCATION, STRSAFE_FILL_ON_FAILURE, and STRSAFE_ZERO_LENGTH_ON_FAILURE flags
            RtlUnicodeStringExHandleOtherFlags(pszDest,
                    cchDest,
                    0,
                    &cchNewDestLength,
                    &pszDestEnd,
                    &cchRemaining,
                    dwFlags);
        }

        if (DestinationString)
        {
            // safe to multiply cchNewDestLength * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
            DestinationString->Length = (USHORT)(cchNewDestLength * sizeof(wchar_t));
        }

        if (NT_SUCCESS(status) || (status == STATUS_BUFFER_OVERFLOW))
        {
            if (RemainingString)
            {
                RemainingString->Length = 0;
                // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                RemainingString->MaximumLength = (USHORT)(cchRemaining * sizeof(wchar_t));
                RemainingString->Buffer = pszDestEnd;
            }
        }
    }

    return status;
}

#endif  // !_M_CEE_PURE

#pragma warning(pop)

#endif  // !NTSTRSAFE_NO_UNICODE_STRING_FUNCTIONS



#endif  // !NTSTRSAFE_LIB_IMPL

// Below here are the worker functions that actually do the work

#if defined(NTSTRSAFE_LIB_IMPL) || !defined(NTSTRSAFE_LIB)

#pragma warning(push)
#pragma warning(disable:__WARNING_RETURNING_BAD_RESULT)


NTSTRSAFEWORKERDDI
    RtlStringLengthWorkerA(
            _In_reads_or_z_(cchMax) STRSAFE_PCNZCH psz,
            _In_ _In_range_(<=, NTSTRSAFE_MAX_CCH) size_t cchMax,
            _Out_opt_ _Deref_out_range_(<, cchMax) _Deref_out_range_(<=, _String_length_(psz)) size_t* pcchLength)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchOriginalMax = cchMax;

    while (cchMax && (*psz != '\0'))
    {
        psz++;
        cchMax--;
    }

    if (cchMax == 0)
    {
        // the string is longer than cchMax
        status = STATUS_INVALID_PARAMETER;
    }

    if (pcchLength)
    {
        if (NT_SUCCESS(status))
        {
            *pcchLength = cchOriginalMax - cchMax;
        }
        else
        {
            *pcchLength = 0;
        }
    }

    return status;
}



NTSTRSAFEWORKERDDI
    RtlStringLengthWorkerW(
            _In_reads_or_z_(cchMax) STRSAFE_PCNZWCH psz,
            _In_ _In_range_(<=, NTSTRSAFE_MAX_CCH) size_t cchMax,
            _Out_opt_ _Deref_out_range_(<, cchMax) _Deref_out_range_(<=, _String_length_(psz)) size_t* pcchLength)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchOriginalMax = cchMax;

    while (cchMax && (*psz != L'\0'))
    {
        psz++;
        cchMax--;
    }

    if (cchMax == 0)
    {
        // the string is longer than cchMax
        status = STATUS_INVALID_PARAMETER;
    }

    if (pcchLength)
    {
        if (NT_SUCCESS(status))
        {
            *pcchLength = cchOriginalMax - cchMax;
        }
        else
        {
            *pcchLength = 0;
        }
    }

    return status;
}

#ifdef ALIGNMENT_MACHINE
NTSTRSAFEWORKERDDI
    RtlUnalignedStringLengthWorkerW(
            _In_reads_or_z_(cchMax) STRSAFE_PCUNZWCH psz,
            _In_ _In_range_(<=, NTSTRSAFE_MAX_CCH) size_t cchMax,
            _Out_opt_ _Deref_out_range_(<, cchMax) size_t* pcchLength)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchOriginalMax = cchMax;

    while (cchMax && (*psz != L'\0'))
    {
        psz++;
        cchMax--;
    }

    if (cchMax == 0)
    {
        // the string is longer than cchMax
        status = STATUS_INVALID_PARAMETER;
    }

    if (pcchLength)
    {
        if (NT_SUCCESS(status))
        {
            *pcchLength = cchOriginalMax - cchMax;
        }
        else
        {
            *pcchLength = 0;
        }
    }

    return status;
}
#endif  // ALIGNMENT_MACHINE


#pragma warning(pop)

// Intentionally allow null deref when STRSAFE_IGNORE_NULLS is not present.
#pragma warning(push)
#pragma warning(disable : __WARNING_DEREF_NULL_PTR)
#pragma warning(disable : __WARNING_INVALID_PARAM_VALUE_1)
#pragma warning(disable : __WARNING_INVALID_PARAM_VALUE_3)
#pragma warning(disable : __WARNING_RETURNING_BAD_RESULT)
#pragma warning(disable : __WARNING_MISSING_ZERO_TERMINATION2)


    _When_(_Old_(*ppszSrc) != NULL, _Unchanged_(*ppszSrc))
_When_(_Old_(*ppszSrc) == NULL, _At_(*ppszSrc, _Post_z_))
    NTSTRSAFEWORKERDDI
    RtlStringExValidateSrcA(
            _Inout_ _Deref_post_notnull_ STRSAFE_PCNZCH* ppszSrc,
            _Inout_opt_
            _Deref_out_range_(<, cchMax)
            _Deref_out_range_(<=, _Old_(*pcchToRead)) size_t* pcchToRead,
            _In_ const size_t cchMax,
            _In_ DWORD dwFlags)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (pcchToRead && (*pcchToRead >= cchMax))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else if ((dwFlags & STRSAFE_IGNORE_NULLS) && (*ppszSrc == NULL))
    {
        *ppszSrc = "";

        if (pcchToRead)
        {
            *pcchToRead = 0;
        }
    }

    return status;
}



    _When_(_Old_(*ppszSrc) != NULL, _Unchanged_(*ppszSrc))
_When_(_Old_(*ppszSrc) == NULL, _At_(*ppszSrc, _Post_z_))
    NTSTRSAFEWORKERDDI
    RtlStringExValidateSrcW(
            _Inout_ _Deref_post_notnull_ STRSAFE_PCNZWCH* ppszSrc,
            _Inout_opt_
            _Deref_out_range_(<, cchMax)
            _Deref_out_range_(<=, _Old_(*pcchToRead)) size_t* pcchToRead,
            _In_ const size_t cchMax,
            _In_ DWORD dwFlags)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (pcchToRead && (*pcchToRead >= cchMax))
    {
        status = STATUS_INVALID_PARAMETER;
    }
    else if ((dwFlags & STRSAFE_IGNORE_NULLS) && (*ppszSrc == NULL))
    {
        *ppszSrc = L"";

        if (pcchToRead)
        {
            *pcchToRead = 0;
        }
    }

    return status;
}


#pragma warning(pop)    // allow null deref


#pragma warning(push)
#pragma warning(disable : 4100) // Unused parameter (pszDest)
_Post_satisfies_(cchDest > 0 && cchDest <= cchMax)
    NTSTRSAFEWORKERDDI
    RtlStringValidateDestA(
            _In_reads_opt_(cchDest) STRSAFE_PCNZCH pszDest,
            _In_ size_t cchDest,
            _In_ const size_t cchMax)
{
    NTSTATUS status = STATUS_SUCCESS;

    if ((cchDest == 0) || (cchDest > cchMax))
    {
        status = STATUS_INVALID_PARAMETER;
    }

    return status;
}
#pragma warning(pop)

// Intentionally allow null deref when STRSAFE_IGNORE_NULLS is not present.
#pragma warning(push)
#pragma warning(disable : __WARNING_DEREF_NULL_PTR)
#pragma warning(disable : __WARNING_INVALID_PARAM_VALUE_1)
#pragma warning(disable : __WARNING_RANGE_POSTCONDITION_VIOLATION)

_Post_satisfies_(cchDest > 0 && cchDest <= cchMax)
    NTSTRSAFEWORKERDDI
    RtlStringValidateDestAndLengthA(
            _In_reads_opt_(cchDest) NTSTRSAFE_PCSTR pszDest,
            _In_ size_t cchDest,
            _Out_ _Deref_out_range_(0, cchDest - 1) size_t* pcchDestLength,
            _In_ const size_t cchMax)
{
    NTSTATUS status;

    status = RtlStringValidateDestA(pszDest, cchDest, cchMax);

    if (NT_SUCCESS(status))
    {
        status = RtlStringLengthWorkerA(pszDest, cchDest, pcchDestLength);
    }
    else
    {
        *pcchDestLength = 0;
    }

    return status;
}
// End intentionally allow null deref.
#pragma warning(pop)



#pragma warning(push)
#pragma warning(disable : 4100) // Unused parameter (pszDest)
_Post_satisfies_(cchDest > 0 && cchDest <= cchMax)
    NTSTRSAFEWORKERDDI
    RtlStringValidateDestW(
            _In_reads_opt_(cchDest) STRSAFE_PCNZWCH pszDest,
            _In_ size_t cchDest,
            _In_ const size_t cchMax)
{
    NTSTATUS status = STATUS_SUCCESS;

    if ((cchDest == 0) || (cchDest > cchMax))
    {
        status = STATUS_INVALID_PARAMETER;
    }

    return status;
}
#pragma warning(pop)

// Intentionally allow null deref when STRSAFE_IGNORE_NULLS is not present.
#pragma warning(push)
#pragma warning(disable : __WARNING_DEREF_NULL_PTR)
#pragma warning(disable : __WARNING_INVALID_PARAM_VALUE_1)
#pragma warning(disable : __WARNING_RANGE_POSTCONDITION_VIOLATION)

_Post_satisfies_(cchDest > 0 && cchDest <= cchMax)
    NTSTRSAFEWORKERDDI
    RtlStringValidateDestAndLengthW(
            _In_reads_opt_(cchDest) NTSTRSAFE_PCWSTR pszDest,
            _In_ size_t cchDest,
            _Out_ _Deref_out_range_(0, cchDest - 1) size_t* pcchDestLength,
            _In_ const size_t cchMax)
{
    NTSTATUS status;

    status = RtlStringValidateDestW(pszDest, cchDest, cchMax);

    if (NT_SUCCESS(status))
    {
        status = RtlStringLengthWorkerW(pszDest, cchDest, pcchDestLength);
    }
    else
    {
        *pcchDestLength = 0;
    }

    return status;
}
// End intentionally allow null deref.
#pragma warning(pop)



NTSTRSAFEWORKERDDI
    RtlStringExValidateDestA(
            _In_reads_opt_(cchDest) STRSAFE_PCNZCH pszDest,
            _In_ size_t cchDest,
            _In_ const size_t cchMax,
            _In_ DWORD dwFlags)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (dwFlags & STRSAFE_IGNORE_NULLS)
    {
        if (((pszDest == NULL) && (cchDest != 0))   ||
                (cchDest > cchMax))
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        status = RtlStringValidateDestA(pszDest, cchDest, cchMax);
    }

    return status;
}

// Intentionally allow null deref when STRSAFE_IGNORE_NULLS is not present.
#pragma warning(push)
#pragma warning(disable : __WARNING_DEREF_NULL_PTR)
#pragma warning(disable : __WARNING_INVALID_PARAM_VALUE_1)
NTSTRSAFEWORKERDDI
    RtlStringExValidateDestAndLengthA(
            _In_reads_opt_(cchDest) NTSTRSAFE_PCSTR pszDest,
            _In_ size_t cchDest,
            _Out_ _Deref_out_range_(0, (cchDest>0?cchDest-1:0)) size_t* pcchDestLength,
            _In_ const size_t cchMax,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;

    if (dwFlags & STRSAFE_IGNORE_NULLS)
    {
        status = RtlStringExValidateDestA(pszDest, cchDest, cchMax, dwFlags);

        if (!NT_SUCCESS(status) || (cchDest == 0))
        {
            *pcchDestLength = 0;
        }
        else
        {
            status = RtlStringLengthWorkerA(pszDest, cchDest, pcchDestLength);
        }
    }
    else
    {
        status = RtlStringValidateDestAndLengthA(pszDest,
                cchDest,
                pcchDestLength,
                cchMax);
    }

    return status;
}
// End intentionally allow null deref.
#pragma warning(pop)



NTSTRSAFEWORKERDDI
    RtlStringExValidateDestW(
            _In_reads_opt_(cchDest) STRSAFE_PCNZWCH pszDest,
            _In_ size_t cchDest,
            _In_ const size_t cchMax,
            _In_ DWORD dwFlags)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (dwFlags & STRSAFE_IGNORE_NULLS)
    {
        if (((pszDest == NULL) && (cchDest != 0))   ||
                (cchDest > cchMax))
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        status = RtlStringValidateDestW(pszDest, cchDest, cchMax);
    }

    return status;
}

// Intentionally allow null deref when STRSAFE_IGNORE_NULLS is not present.
#pragma warning(push)
#pragma warning(disable : __WARNING_DEREF_NULL_PTR)
#pragma warning(disable : __WARNING_INVALID_PARAM_VALUE_1)
NTSTRSAFEWORKERDDI
    RtlStringExValidateDestAndLengthW(
            _In_reads_opt_(cchDest) NTSTRSAFE_PCWSTR pszDest,
            _In_ size_t cchDest,
            _Out_ _Deref_out_range_(0, (cchDest>0?cchDest-1:0)) size_t* pcchDestLength,
            _In_ const size_t cchMax,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;

    if (dwFlags & STRSAFE_IGNORE_NULLS)
    {
        status = RtlStringExValidateDestW(pszDest, cchDest, cchMax, dwFlags);

        if (!NT_SUCCESS(status) || (cchDest == 0))
        {
            *pcchDestLength = 0;
        }
        else
        {
            status = RtlStringLengthWorkerW(pszDest, cchDest, pcchDestLength);
        }
    }
    else
    {
        status = RtlStringValidateDestAndLengthW(pszDest,
                cchDest,
                pcchDestLength,
                cchMax);
    }

    return status;
}
// End intentionally allow null deref.
#pragma warning(pop)


#pragma warning(push)
#pragma warning(disable:__WARNING_RETURNING_BAD_RESULT)


NTSTRSAFEWORKERDDI
    RtlStringCopyWorkerA(
            _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ _In_range_(1, NTSTRSAFE_MAX_CCH) size_t cchDest,
            _Always_(_Out_opt_ _Deref_out_range_(<=, (cchToCopy < cchDest) ? cchToCopy : (cchDest - 1))) size_t* pcchNewDestLength,
            _In_reads_or_z_(cchToCopy) STRSAFE_PCNZCH pszSrc,
            _In_ _In_range_(<, NTSTRSAFE_MAX_CCH) size_t cchToCopy)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchNewDestLength = 0;

    // ASSERT(cchDest != 0);

    while (cchDest && cchToCopy && (*pszSrc != '\0'))
    {
        *pszDest++ = *pszSrc++;
        cchDest--;
        cchToCopy--;

        cchNewDestLength++;
    }

    if (cchDest == 0)
    {
        // we are going to truncate pszDest
        pszDest--;
        cchNewDestLength--;

        status = STATUS_BUFFER_OVERFLOW;
    }

    *pszDest = '\0';

    if (pcchNewDestLength)
    {
        *pcchNewDestLength = cchNewDestLength;
    }

    return status;
}



NTSTRSAFEWORKERDDI
    RtlStringCopyWorkerW(
            _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ _In_range_(1, NTSTRSAFE_MAX_CCH) size_t cchDest,
            _Always_(_Out_opt_ _Deref_out_range_(<=, (cchToCopy < cchDest) ? cchToCopy : (cchDest - 1))) size_t* pcchNewDestLength,
            _In_reads_or_z_(cchToCopy) STRSAFE_PCNZWCH pszSrc,
            _In_ _In_range_(<, NTSTRSAFE_MAX_CCH) size_t cchToCopy)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchNewDestLength = 0;

    // ASSERT(cchDest != 0);

    while (cchDest && cchToCopy && (*pszSrc != L'\0'))
    {
        *pszDest++ = *pszSrc++;
        cchDest--;
        cchToCopy--;

        cchNewDestLength++;
    }

    if (cchDest == 0)
    {
        // we are going to truncate pszDest
        pszDest--;
        cchNewDestLength--;

        status = STATUS_BUFFER_OVERFLOW;
    }

    *pszDest = L'\0';

    if (pcchNewDestLength)
    {
        *pcchNewDestLength = cchNewDestLength;
    }

    return status;
}


#pragma warning(pop)


NTSTRSAFEWORKERDDI
    RtlStringVPrintfWorkerA(
            _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PSTR pszDest,
            _In_ _In_range_(1, NTSTRSAFE_MAX_CCH) size_t cchDest,
            _Always_(_Out_opt_ _Deref_out_range_(<=, cchDest - 1)) size_t* pcchNewDestLength,
            _In_ _Printf_format_string_ NTSTRSAFE_PCSTR pszFormat,
            _In_ va_list argList)
{
    NTSTATUS status = STATUS_SUCCESS;
    int iRet;
    size_t cchMax;
    size_t cchNewDestLength = 0;

    // leave the last space for the null terminator
    cchMax = cchDest - 1;

#if (NTSTRSAFE_USE_SECURE_CRT == 1) && !defined(NTSTRSAFE_LIB_IMPL)
    iRet = _vsnprintf_s(pszDest, cchDest, cchMax, pszFormat, argList);
#else
#pragma warning(push)
#pragma warning(disable: __WARNING_BANNED_API_USAGE)// "STRSAFE not included"
    iRet = _vsnprintf(pszDest, cchMax, pszFormat, argList);
#pragma warning(pop)
#endif
    // ASSERT((iRet < 0) || (((size_t)iRet) <= cchMax));

    if ((iRet < 0) || (((size_t)iRet) > cchMax))
    {
        // need to null terminate the string
        pszDest += cchMax;
        *pszDest = '\0';

        cchNewDestLength = cchMax;

        // we have truncated pszDest
        status = STATUS_BUFFER_OVERFLOW;
    }
    else if (((size_t)iRet) == cchMax)
    {
        // need to null terminate the string
        pszDest += cchMax;
        *pszDest = '\0';

        cchNewDestLength = cchMax;
    }
    else
    {
        cchNewDestLength = (size_t)iRet;
    }

    if (pcchNewDestLength)
    {
        *pcchNewDestLength = cchNewDestLength;
    }

    return status;
}




NTSTRSAFEWORKERDDI
    RtlStringVPrintfWorkerW(
            _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
            _In_ _In_range_(1, NTSTRSAFE_MAX_CCH) size_t cchDest,
            _Always_(_Out_opt_ _Deref_out_range_(<=, cchDest - 1)) size_t* pcchNewDestLength,
            _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
            _In_ va_list argList)
{
    NTSTATUS status = STATUS_SUCCESS;
    int iRet;
    size_t cchMax;
    size_t cchNewDestLength = 0;

    // leave the last space for the null terminator
    cchMax = cchDest - 1;

#if (NTSTRSAFE_USE_SECURE_CRT == 1) && !defined(NTSTRSAFE_LIB_IMPL)
    iRet = _vsnwprintf_s(pszDest, cchDest, cchMax, pszFormat, argList);
#else
#pragma warning(push)
#pragma warning(disable: __WARNING_BANNED_API_USAGE)// "STRSAFE not included"
    iRet = _vsnwprintf(pszDest, cchMax, pszFormat, argList);
#pragma warning(pop)
#endif
    // ASSERT((iRet < 0) || (((size_t)iRet) <= cchMax));

    if ((iRet < 0) || (((size_t)iRet) > cchMax))
    {
        // need to null terminate the string
        pszDest += cchMax;
        *pszDest = L'\0';

        cchNewDestLength = cchMax;

        // we have truncated pszDest
        status = STATUS_BUFFER_OVERFLOW;
    }
    else if (((size_t)iRet) == cchMax)
    {
        // need to null terminate the string
        pszDest += cchMax;
        *pszDest = L'\0';

        cchNewDestLength = cchMax;
    }
    else
    {
        cchNewDestLength = (size_t)iRet;
    }

    if (pcchNewDestLength)
    {
        *pcchNewDestLength = cchNewDestLength;
    }

    return status;
}




NTSTRSAFEWORKERDDI
    RtlStringExHandleFillBehindNullA(
            _Inout_updates_bytes_(cbRemaining) NTSTRSAFE_PSTR pszDestEnd,
            _In_ size_t cbRemaining,
            _In_ DWORD dwFlags)
{
    if (cbRemaining > sizeof(char))
    {
        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), cbRemaining - sizeof(char));
    }

    return STATUS_SUCCESS;
}



NTSTRSAFEWORKERDDI
    RtlStringExHandleFillBehindNullW(
            _Inout_updates_bytes_(cbRemaining) NTSTRSAFE_PWSTR pszDestEnd,
            _In_ size_t cbRemaining,
            _In_ DWORD dwFlags)
{
    if (cbRemaining > sizeof(wchar_t))
    {
        memset(pszDestEnd + 1, STRSAFE_GET_FILL_PATTERN(dwFlags), cbRemaining - sizeof(wchar_t));
    }

    return STATUS_SUCCESS;
}


// ignore false positives due to complexity of bitflag usage
#pragma warning(push)
#pragma warning(disable : __WARNING_USING_UNINIT_VAR)
#pragma warning(disable : __WARNING_RETURN_UNINIT_VAR)
#pragma warning(disable : __WARNING_MISSING_ZERO_TERMINATION2)
#pragma warning(disable : __WARNING_POSTCONDITION_NULLTERMINATION_VIOLATION)
#pragma warning(disable:__WARNING_POTENTIAL_RANGE_POSTCONDITION_VIOLATION)


_Success_(1)  // always succeeds, no exit tests needed
    NTSTRSAFEWORKERDDI
    RtlStringExHandleOtherFlagsA(
            _Out_writes_bytes_(cbDest) NTSTRSAFE_PSTR pszDest,
            _In_ _In_range_(1, NTSTRSAFE_MAX_CCH * sizeof(char)) size_t cbDest,
            _In_ _In_range_(0, cbDest>sizeof(char)?(cbDest / sizeof(char)) - 1:0) size_t cchOriginalDestLength,
            _Outptr_result_buffer_(*pcchRemaining) NTSTRSAFE_PSTR* ppszDestEnd,
            _Out_ _Deref_out_range_(0, cbDest / sizeof(char)) size_t* pcchRemaining,
            _In_ DWORD dwFlags)
{
    size_t cchDest = cbDest / sizeof(char);

    _Analysis_assume_(dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE));

    if ((cchDest > 0) && (dwFlags & STRSAFE_NO_TRUNCATION))
    {
        char* pszOriginalDestEnd;

        pszOriginalDestEnd = pszDest + cchOriginalDestLength;

        *ppszDestEnd = pszOriginalDestEnd;
        *pcchRemaining = cchDest - cchOriginalDestLength;

        // null terminate the end of the original string
        *pszOriginalDestEnd = '\0';
    }

    if (dwFlags & STRSAFE_FILL_ON_FAILURE)
    {
        memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

        if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
        {
            *ppszDestEnd = pszDest;
            *pcchRemaining = cchDest;
            _Analysis_assume_(*pszDest == '\0');
        }
        else if (cchDest > 0)
        {
            char* pszDestEnd;

            pszDestEnd = pszDest + cchDest - 1;

            *ppszDestEnd = pszDestEnd;
            *pcchRemaining = 1;

            // null terminate the end of the string
            *pszDestEnd = L'\0';
        }
    }

    if ((cchDest > 0) && (dwFlags & STRSAFE_NULL_ON_FAILURE))
    {
        *ppszDestEnd = pszDest;
        *pcchRemaining = cchDest;

        // null terminate the beginning of the string
        *pszDest = '\0';
    }

    return STATUS_SUCCESS;
}



_Success_(1)  // always succeeds, no exit tests needed
    NTSTRSAFEWORKERDDI
    RtlStringExHandleOtherFlagsW(
            _Out_writes_bytes_(cbDest) NTSTRSAFE_PWSTR pszDest,
            _In_ _In_range_(1, NTSTRSAFE_MAX_CCH * sizeof(wchar_t)) size_t cbDest,
            _In_ _In_range_(0, cbDest>sizeof(wchar_t)?(cbDest / sizeof(wchar_t)) - 1:0) size_t cchOriginalDestLength,
            _Outptr_result_buffer_(*pcchRemaining) NTSTRSAFE_PWSTR* ppszDestEnd,
            _Out_ _Deref_out_range_(0, cbDest / sizeof(wchar_t)) size_t* pcchRemaining,
            _In_ DWORD dwFlags)
{
    size_t cchDest = cbDest / sizeof(wchar_t);

    _Analysis_assume_(dwFlags & (STRSAFE_NO_TRUNCATION | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE));

    if ((cchDest > 0) && (dwFlags & STRSAFE_NO_TRUNCATION))
    {
        wchar_t* pszOriginalDestEnd;

        pszOriginalDestEnd = pszDest + cchOriginalDestLength;

        *ppszDestEnd = pszOriginalDestEnd;
        *pcchRemaining = cchDest - cchOriginalDestLength;

        // null terminate the end of the original string
        *pszOriginalDestEnd = L'\0';
    }

    if (dwFlags & STRSAFE_FILL_ON_FAILURE)
    {
        memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

        if (STRSAFE_GET_FILL_PATTERN(dwFlags) == 0)
        {
            *ppszDestEnd = pszDest;
            *pcchRemaining = cchDest;
            _Analysis_assume_(cchDest == 0 || *pszDest == L'\0');
        }
        else if (cchDest > 0)
        {
            wchar_t* pszDestEnd;

            pszDestEnd = pszDest + cchDest - 1;

            *ppszDestEnd = pszDestEnd;
            *pcchRemaining = 1;

            // null terminate the end of the string
            *pszDestEnd = L'\0';
        }
    }

    if ((cchDest > 0) && (dwFlags & STRSAFE_NULL_ON_FAILURE))
    {
        *ppszDestEnd = pszDest;
        *pcchRemaining = cchDest;

        // null terminate the beginning of the string
        *pszDest = L'\0';
    }

    return STATUS_SUCCESS;
}


#pragma warning(pop)   // ignore false positives due to complexity of bitflag usage


#ifndef NTSTRSAFE_NO_UNICODE_STRING_FUNCTIONS

#pragma warning(push)
#pragma warning(disable : __WARNING_INCORRECT_ANNOTATION)
#pragma warning(disable : __WARNING_HIGH_PRIORITY_OVERFLOW_POSTCONDITION)

_Use_decl_annotations_
NTSTRSAFEWORKERDDI
RtlUnicodeStringInitWorker(
        PUNICODE_STRING DestinationString,
        NTSTRSAFE_PCWSTR pszSrc,
        const size_t cchMax,
        DWORD dwFlags)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (DestinationString || !(dwFlags & STRSAFE_IGNORE_NULLS))
    {
        memset(DestinationString, 0, sizeof(*DestinationString));
    }

    if (pszSrc)
    {
        size_t cchSrcLength;

        status = RtlStringLengthWorkerW(pszSrc, cchMax, &cchSrcLength);

        if (NT_SUCCESS(status))
        {
            if (DestinationString)
            {
                size_t cbLength;

                // safe to multiply cchSrcLength * sizeof(wchar_t) since cchSrcLength < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
                cbLength = cchSrcLength * sizeof(wchar_t);

                DestinationString->Length = (USHORT)cbLength;
                // safe to add cbLength + sizeof(wchar_t) since cchSrcLength < NTSTRSAFE_UNICODE_STRING_MAX_CCH
                DestinationString->MaximumLength = (USHORT)(cbLength + sizeof(wchar_t));
                DestinationString->Buffer = (PWSTR)pszSrc;
            }
            else
            {
                status = STATUS_INVALID_PARAMETER;
            }
        }
    }

    return status;
}

// Intentionally allow null deref in error cases.
#pragma warning(push)
#pragma warning(disable : __WARNING_DEREF_NULL_PTR)
#pragma warning(disable : __WARNING_INVALID_PARAM_VALUE_1)
#pragma warning(disable : __WARNING_RETURNING_BAD_RESULT)

NTSTRSAFEWORKERDDI
RtlUnicodeStringValidateWorker(
        _In_opt_ PCUNICODE_STRING SourceString,
        _In_ const size_t cchMax,
        _In_ DWORD dwFlags)
{
    NTSTATUS status = STATUS_SUCCESS;

    if (SourceString || !(dwFlags & STRSAFE_IGNORE_NULLS))
    {
        if (((SourceString->Length % sizeof(wchar_t)) != 0)         ||
                ((SourceString->MaximumLength % sizeof(wchar_t)) != 0)  ||
                (SourceString->Length > SourceString->MaximumLength)    ||
                (SourceString->MaximumLength > (cchMax * sizeof(wchar_t))))
        {
            status = STATUS_INVALID_PARAMETER;
        }
        else if ((SourceString->Buffer == NULL) &&
                ((SourceString->Length != 0) || (SourceString->MaximumLength != 0)))
        {
            status = STATUS_INVALID_PARAMETER;
        }
    }

    return status;
}

#pragma warning(push)
#pragma warning(disable : __WARNING_RANGE_POSTCONDITION_VIOLATION)

_Post_satisfies_(*pcchSrcLength*sizeof(wchar_t) == SourceString->MaximumLength)
    NTSTRSAFEWORKERDDI
    RtlUnicodeStringValidateSrcWorker(
            _In_ PCUNICODE_STRING SourceString,
            _Outptr_result_buffer_(*pcchSrcLength) wchar_t** ppszSrc,
            _Out_ size_t* pcchSrcLength,
            _In_ const size_t cchMax,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;

    *ppszSrc = NULL;
    *pcchSrcLength = 0;

    status = RtlUnicodeStringValidateWorker(SourceString, cchMax, dwFlags);

    if (NT_SUCCESS(status))
    {
        if (SourceString)
        {
            *ppszSrc = SourceString->Buffer;
            *pcchSrcLength = SourceString->Length / sizeof(wchar_t);
        }

        if ((*ppszSrc == NULL) && (dwFlags & STRSAFE_IGNORE_NULLS))
        {
            *ppszSrc = (wchar_t*)L"";
        }
    }

    return status;
}

#pragma warning(pop)
// End intentionally allow null deref.
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : __WARNING_RANGE_POSTCONDITION_VIOLATION)

_Post_satisfies_(*pcchDest*sizeof(wchar_t) == DestinationString->MaximumLength)
    NTSTRSAFEWORKERDDI
    RtlUnicodeStringValidateDestWorker(
            _In_ PCUNICODE_STRING DestinationString,
            _Outptr_result_buffer_(*pcchDest) wchar_t** ppszDest,
            _Out_ size_t* pcchDest,
            _Out_opt_ size_t* pcchDestLength,
            _In_ const size_t cchMax,
            _In_ DWORD dwFlags)
{
    NTSTATUS status;

    *ppszDest = NULL;
    *pcchDest = 0;

    if (pcchDestLength)
    {
        *pcchDestLength = 0;
    }

    status = RtlUnicodeStringValidateWorker(DestinationString, cchMax, dwFlags);

    if (NT_SUCCESS(status) && DestinationString)
    {
        *ppszDest = DestinationString->Buffer;
        *pcchDest = DestinationString->MaximumLength / sizeof(wchar_t);

        if (pcchDestLength)
        {
            *pcchDestLength = DestinationString->Length / sizeof(wchar_t);
        }
    }

    return status;
}

#pragma warning(pop)

NTSTRSAFEWORKERDDI
    RtlStringCopyWideCharArrayWorker(
            _Out_writes_(cchDest) NTSTRSAFE_PWSTR pszDest,
            _In_ size_t cchDest,
            _Out_opt_ size_t* pcchNewDestLength,
            _In_reads_(cchSrcLength) const wchar_t* pszSrc,
            _In_ size_t cchSrcLength)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchNewDestLength = 0;

    // ASSERT(cchDest != 0);

    while (cchDest && cchSrcLength)
    {
        *pszDest++ = *pszSrc++;
        cchDest--;
        cchSrcLength--;

        cchNewDestLength++;
    }

    if (cchDest == 0)
    {
        // we are going to truncate pszDest
        pszDest--;
        cchNewDestLength--;

        status = STATUS_BUFFER_OVERFLOW;
    }

    *pszDest = L'\0';

    if (pcchNewDestLength)
    {
        *pcchNewDestLength = cchNewDestLength;
    }

    return status;
}

NTSTRSAFEWORKERDDI
    RtlWideCharArrayCopyStringWorker(
            _Out_writes_to_(cchDest, *pcchNewDestLength) wchar_t* pszDest,
            _In_ size_t cchDest,
            _Out_ size_t* pcchNewDestLength,
            _In_ NTSTRSAFE_PCWSTR pszSrc,
            _In_ size_t cchToCopy)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchNewDestLength = 0;

    while (cchDest && cchToCopy && (*pszSrc != L'\0'))
    {
        *pszDest++ = *pszSrc++;
        cchDest--;
        cchToCopy--;

        cchNewDestLength++;
    }

    if ((cchDest == 0) && (cchToCopy != 0) && (*pszSrc != L'\0'))
    {
        // we are going to truncate pszDest
        status = STATUS_BUFFER_OVERFLOW;
    }

    *pcchNewDestLength = cchNewDestLength;

    return status;
}

NTSTRSAFEWORKERDDI
    RtlWideCharArrayCopyWorker(
            _Out_writes_to_(cchDest, *pcchNewDestLength) wchar_t* pszDest,
            _In_ size_t cchDest,
            _Out_ size_t* pcchNewDestLength,
            _In_reads_(cchSrcLength) const wchar_t* pszSrc,
            _In_ size_t cchSrcLength)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t cchNewDestLength = 0;

    while (cchDest && cchSrcLength)
    {
        *pszDest++ = *pszSrc++;
        cchDest--;
        cchSrcLength--;

        cchNewDestLength++;
    }

    if ((cchDest == 0) && (cchSrcLength != 0))
    {
        // we are going to truncate pszDest
        status = STATUS_BUFFER_OVERFLOW;
    }

    *pcchNewDestLength = cchNewDestLength;

    return status;
}

NTSTRSAFEWORKERDDI
    RtlWideCharArrayVPrintfWorker(
            _Out_writes_to_(cchDest, *pcchNewDestLength) wchar_t* pszDest,
            _In_ size_t cchDest,
            _Out_ size_t* pcchNewDestLength,
            _In_ _Printf_format_string_ NTSTRSAFE_PCWSTR pszFormat,
            _In_ va_list argList)
{
    NTSTATUS status = STATUS_SUCCESS;
    int iRet;

#pragma warning(push)
#pragma warning(disable: __WARNING_BANNED_API_USAGE)// "STRSAFE not included"
    iRet = _vsnwprintf(pszDest, cchDest, pszFormat, argList);
#pragma warning(pop)
    // ASSERT((iRet < 0) || (((size_t)iRet) <= cchMax));

    if ((iRet < 0) || (((size_t)iRet) > cchDest))
    {
        *pcchNewDestLength = cchDest;

        // we have truncated pszDest
        status = STATUS_BUFFER_OVERFLOW;
    }
    else
    {
        *pcchNewDestLength = (size_t)iRet;
    }

    return status;
}

NTSTRSAFEWORKERDDI
    RtlUnicodeStringExHandleFill(
            _Out_writes_(cchRemaining) wchar_t* pszDestEnd,
            _In_ size_t cchRemaining,
            _In_ DWORD dwFlags)
{
    size_t cbRemaining;

    // safe to multiply cchRemaining * sizeof(wchar_t) since cchRemaining < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
    cbRemaining = cchRemaining * sizeof(wchar_t);

    memset(pszDestEnd, STRSAFE_GET_FILL_PATTERN(dwFlags), cbRemaining);

    return STATUS_SUCCESS;
}

// ignore false positives due to complexity of bitflag usage
#pragma warning(push)
#pragma warning(disable : __WARNING_USING_UNINIT_VAR)
#pragma warning(disable : __WARNING_RETURN_UNINIT_VAR)

NTSTRSAFEWORKERDDI
    RtlUnicodeStringExHandleOtherFlags(
            _Inout_updates_(cchDest) wchar_t* pszDest,
            _In_ size_t cchDest,
            _In_ size_t cchOriginalDestLength,
            _Out_ size_t* pcchNewDestLength,
            _Outptr_result_buffer_(*pcchRemaining) wchar_t** ppszDestEnd,
            _Out_ size_t* pcchRemaining,
            _In_ DWORD dwFlags)
{
    if (dwFlags & STRSAFE_NO_TRUNCATION)
    {
        *ppszDestEnd = pszDest + cchOriginalDestLength;
        *pcchRemaining = cchDest - cchOriginalDestLength;

        *pcchNewDestLength = cchOriginalDestLength;
    }

    if (dwFlags & STRSAFE_FILL_ON_FAILURE)
    {
        size_t cbDest;

        // safe to multiply cchDest * sizeof(wchar_t) since cchDest < NTSTRSAFE_UNICODE_STRING_MAX_CCH and sizeof(wchar_t) is 2
        cbDest = cchDest * sizeof(wchar_t);

        memset(pszDest, STRSAFE_GET_FILL_PATTERN(dwFlags), cbDest);

        *ppszDestEnd = pszDest;
        *pcchRemaining = cchDest;

        *pcchNewDestLength = 0;
    }

    if (dwFlags & STRSAFE_ZERO_LENGTH_ON_FAILURE)
    {
        *ppszDestEnd = pszDest;
        *pcchRemaining = cchDest;

        *pcchNewDestLength = 0;
    }

    return STATUS_SUCCESS;
}

#pragma warning(pop)

#pragma warning(pop)

#endif  // !NTSTRSAFE_NO_UNICODE_STRING_FUNCTIONS


#endif  // defined(NTSTRSAFE_LIB_IMPL) || !defined(NTSTRSAFE_LIB)


// Do not call these functions, they are worker functions for internal use within this file
#ifdef DEPRECATE_SUPPORTED
#pragma deprecated(RtlStringLengthWorkerA)
#pragma deprecated(RtlStringLengthWorkerW)
#pragma deprecated(RtlUnalignedStringLengthWorkerW)
#pragma deprecated(RtlStringExValidateSrcA)
#pragma deprecated(RtlStringExValidateSrcW)
#pragma deprecated(RtlStringValidateDestA)
#pragma deprecated(RtlStringValidateDestAndLengthA)
#pragma deprecated(RtlStringValidateDestW)
#pragma deprecated(RtlStringValidateDestAndLengthW)
#pragma deprecated(RtlStringExValidateDestA)
#pragma deprecated(RtlStringExValidateDestAndLengthA)
#pragma deprecated(RtlStringExValidateDestW)
#pragma deprecated(RtlStringExValidateDestAndLengthW)
#pragma deprecated(RtlStringCopyWorkerA)
#pragma deprecated(RtlStringCopyWorkerW)
#pragma deprecated(RtlStringVPrintfWorkerA)
#pragma deprecated(RtlStringVPrintfWorkerW)
#pragma deprecated(RtlStringExHandleFillBehindNullA)
#pragma deprecated(RtlStringExHandleFillBehindNullW)
#pragma deprecated(RtlStringExHandleOtherFlagsA)
#pragma deprecated(RtlStringExHandleOtherFlagsW)
#pragma deprecated(RtlUnicodeStringInitWorker)
#pragma deprecated(RtlUnicodeStringValidateWorker)
#pragma deprecated(RtlUnicodeStringValidateSrcWorker)
#pragma deprecated(RtlUnicodeStringValidateDestWorker)
#pragma deprecated(RtlStringCopyWideCharArrayWorker)
#pragma deprecated(RtlWideCharArrayCopyStringWorker)
#pragma deprecated(RtlWideCharArrayCopyWorker)
#pragma deprecated(RtlWideCharArrayVPrintfWorker)
#pragma deprecated(RtlUnicodeStringExHandleFill)
#pragma deprecated(RtlUnicodeStringExHandleOtherFlags)
#else
#define RtlStringLengthWorkerA             RtlStringLengthWorkerA_instead_use_StringCchLengthA_or_StringCbLengthA
#define RtlStringLengthWorkerW             RtlStringLengthWorkerW_instead_use_StringCchLengthW_or_StringCbLengthW
#define RtlUnalignedStringLengthWorkerW    RtlUnalignedStringLengthWorkerW_instead_use_UnalignedStringCchLengthW
#define RtlStringExValidateSrcA            RtlStringExValidateSrcA_do_not_call_this_function
#define RtlStringExValidateSrcW            RtlStringExValidateSrcW_do_not_call_this_function
#define RtlStringValidateDestA             RtlStringValidateDestA_do_not_call_this_function
#define RtlStringValidateDestAndLengthA    RtlStringValidateDestAndLengthA_do_not_call_this_function
#define RtlStringValidateDestW             RtlStringValidateDestW_do_not_call_this_function
#define RtlStringValidateDestAndLengthW    RtlStringValidateDestAndLengthW_do_not_call_this_function
#define RtlStringExValidateDestA           RtlStringExValidateDestA_do_not_call_this_function
#define RtlStringExValidateDestAndLengthA  RtlStringExValidateDestAndLengthA_do_not_call_this_function
#define RtlStringExValidateDestW           RtlStringExValidateDestW_do_not_call_this_function
#define RtlStringExValidateDestAndLengthW  RtlStringExValidateDestAndLengthW_do_not_call_this_function
#define RtlStringCopyWorkerA               RtlStringCopyWorkerA_instead_use_StringCchCopyA_or_StringCbCopyA
#define RtlStringCopyWorkerW               RtlStringCopyWorkerW_instead_use_StringCchCopyW_or_StringCbCopyW
#define RtlStringVPrintfWorkerA            RtlStringVPrintfWorkerA_instead_use_StringCchVPrintfA_or_StringCbVPrintfA
#define RtlStringVPrintfWorkerW            RtlStringVPrintfWorkerW_instead_use_StringCchVPrintfW_or_StringCbVPrintfW
#define RtlStringExHandleFillBehindNullA   RtlStringExHandleFillBehindNullA_do_not_call_this_function
#define RtlStringExHandleFillBehindNullW   RtlStringExHandleFillBehindNullW_do_not_call_this_function
#define RtlStringExHandleOtherFlagsA       RtlStringExHandleOtherFlagsA_do_not_call_this_function
#define RtlStringExHandleOtherFlagsW       RtlStringExHandleOtherFlagsW_do_not_call_this_function
#define RtlUnicodeStringInitWorker          RtlUnicodeStringInitWorker_instead_use_RtlUnicodeStringInit_or_RtlUnicodeStringInitEx
#define RtlUnicodeStringValidateWorker      RtlUnicodeStringValidateWorker_instead_use_RtlUnicodeStringValidate_or_RtlUnicodeStringValidateEx
#define RtlUnicodeStringValidateSrcWorker   RtlUnicodeStringValidateSrcWorker_do_not_call_this_function
#define RtlUnicodeStringValidateDestWorker  RtlUnicodeStringValidateDestWorker_do_not_call_this_function
#define RtlStringCopyWideCharArrayWorker    RtlStringCopyWideCharArrayWorker_instead_use_RtlStringCchCopyUnicodeString_or_RtlStringCbCopyUnicodeString
#define RtlWideCharArrayCopyStringWorker    RtlWideCharArrayCopyStringWorker_instead_use_RtlUnicodeStringCopyString_or_RtlUnicodeStringCopyStringEx
#define RtlWideCharArrayCopyWorker          RtlWideCharArrayCopyWorker_instead_use_RtlUnicodeStringCopy_or_RtlUnicodeStringCopyEx
#define RtlWideCharArrayVPrintfWorker       RtlWideCharArrayVPrintfWorker_instead_use_RtlUnicodeStringVPrintf_or_RtlUnicodeStringPrintf
#define RtlUnicodeStringExHandleFill        RtlUnicodeStringExHandleFill_do_not_call_this_function
#define RtlUnicodeStringExHandleOtherFlags  RtlUnicodeStringExHandleOtherFlags_do_not_call_this_function
#endif // !DEPRECATE_SUPPORTED


#pragma warning(pop)
#pragma warning(pop)

#endif  // _NTSTRSAFE_H_INCLUDED_
