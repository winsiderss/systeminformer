/*
 * Exception support functions
 *
 * This file is part of System Informer.
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

#if (PHNT_VERSION >= PHNT_THRESHOLD)
typedef enum _KCONTINUE_TYPE
{
    KCONTINUE_UNWIND,
    KCONTINUE_RESUME,
    KCONTINUE_LONGJUMP,
    KCONTINUE_SET,
    KCONTINUE_LAST,
} KCONTINUE_TYPE;

typedef struct _KCONTINUE_ARGUMENT
{
    KCONTINUE_TYPE ContinueType;
    ULONG ContinueFlags;
    ULONGLONG Reserved[2];
} KCONTINUE_ARGUMENT, *PKCONTINUE_ARGUMENT;

#define KCONTINUE_FLAG_TEST_ALERT 0x00000001 // wbenny
#define KCONTINUE_FLAG_DELIVER_APC 0x00000002 // wbenny

NTSYSCALLAPI
NTSTATUS
NTAPI
NtContinueEx(
    _In_ PCONTEXT ContextRecord,
    _In_ PVOID ContinueArgument // PKCONTINUE_ARGUMENT and BOOLEAN are valid
    );

//FORCEINLINE
//NTSTATUS
//NtContinue(
//    _In_ PCONTEXT ContextRecord,
//    _In_ BOOLEAN TestAlert
//    )
//{
//    return NtContinueEx(ContextRecord, (PCONTINUE_ARGUMENT)TestAlert);
//}
#endif

NTSYSCALLAPI
NTSTATUS
NTAPI
NtRaiseException(
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PCONTEXT ContextRecord,
    _In_ BOOLEAN FirstChance
    );

NTSYSCALLAPI
DECLSPEC_NORETURN
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
