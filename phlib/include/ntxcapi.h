#ifndef NTXCAPI_H
#define NTXCAPI_H

NTSYSAPI
BOOLEAN
NTAPI
RtlDispatchException(
    __in PEXCEPTION_RECORD ExceptionRecord,
    __in PCONTEXT ContextRecord
    );

NTSYSAPI
DECLSPEC_NORETURN
VOID
NTAPI
RtlRaiseStatus(
    __in NTSTATUS Status
    );

NTSYSAPI
VOID
NTAPI
RtlRaiseException(
    __in PEXCEPTION_RECORD ExceptionRecord
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtContinue(
    __in PCONTEXT ContextRecord,
    __in BOOLEAN TestAlert
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRaiseException(
    __in PEXCEPTION_RECORD ExceptionRecord,
    __in PCONTEXT ContextRecord,
    __in BOOLEAN FirstChance
    );

#endif
