/*
 * Process Hacker -
 *   Exception support functions
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

#ifndef _NTXCAPI_H
#define _NTXCAPI_H

NTSYSAPI
BOOLEAN
NTAPI
RtlDispatchException(
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PCONTEXT ContextRecord
    );

NTSYSAPI
DECLSPEC_NORETURN
VOID
NTAPI
RtlRaiseStatus(
    _In_ NTSTATUS Status
    );

NTSYSAPI
VOID
NTAPI
RtlRaiseException(
    _In_ PEXCEPTION_RECORD ExceptionRecord
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtContinue(
    _In_ PCONTEXT ContextRecord,
    _In_ BOOLEAN TestAlert
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRaiseException(
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PCONTEXT ContextRecord,
    _In_ BOOLEAN FirstChance
    );

__analysis_noreturn
NTSYSCALLAPI
VOID
NTAPI
RtlAssert(
    _In_ PVOID VoidFailedAssertion,
    _In_ PVOID VoidFileName,
    _In_ ULONG LineNumber,
    _In_opt_ PSTR MutableMessage
    );

#define RTL_ASSERT(exp) \
    ((!(exp)) ? (RtlAssert((PVOID)#exp, (PVOID)__FILE__, __LINE__, NULL), FALSE) : TRUE)
#define RTL_ASSERTMSG(msg, exp) \
    ((!(exp)) ? (RtlAssert((PVOID)#exp, (PVOID)__FILE__, __LINE__, msg), FALSE) : TRUE)
#define RTL_SOFT_ASSERT(_exp) \
    ((!(_exp)) ? (DbgPrint("%s(%d): Soft assertion failed\n   Expression: %s\n", __FILE__, __LINE__, #_exp), FALSE) : TRUE)
#define RTL_SOFT_ASSERTMSG(_msg, _exp) \
    ((!(_exp)) ? (DbgPrint("%s(%d): Soft assertion failed\n   Expression: %s\n   Message: %s\n", __FILE__, __LINE__, #_exp, (_msg)), FALSE) : TRUE)

#endif
